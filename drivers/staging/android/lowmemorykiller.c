/* drivers/misc/lowmemorykiller.c
 *
 * The lowmemorykiller driver lets user-space specify a set of memory thresholds
 * where processes with a range of oom_score_adj values will get killed. Specify
 * the minimum oom_score_adj values in
 * /sys/module/lowmemorykiller/parameters/adj and the number of free pages in
 * /sys/module/lowmemorykiller/parameters/minfree. Both files take a comma
 * separated list of numbers in ascending order.
 *
 * For example, write "0,8" to /sys/module/lowmemorykiller/parameters/adj and
 * "1024,4096" to /sys/module/lowmemorykiller/parameters/minfree to kill
 * processes with a oom_score_adj value of 8 or higher when the free memory
 * drops below 4096 pages and kill processes with a oom_score_adj value of 0 or
 * higher when the free memory drops below 1024 pages.
 *
 * The driver considers memory used for caches to be free, but if a large
 * percentage of the cached memory is locked this can be very inaccurate
 * and processes may not get killed until the normal oom killer is triggered.
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/swap.h>
#include <linux/rcupdate.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/swap.h>
#include <linux/fs.h>
#include <linux/zcache.h>

#include <linux/ratelimit.h>

#define LMK_COUNT_READ

#ifdef LMK_COUNT_READ
static uint32_t lmk_count = 0;
#endif

#ifdef CONFIG_SEC_OOM_KILLER
#define MULTIPLE_OOM_KILLER
#define OOM_COUNT_READ
#endif

#ifdef OOM_COUNT_READ
static uint32_t oom_count = 0;
#endif

#ifdef MULTIPLE_OOM_KILLER
#define OOM_DEPTH 5
#endif

static uint32_t lowmem_debug_level = 1;
static int lowmem_adj[6] = {
	0,
	1,
	6,
	12,
};
static int lowmem_adj_size = 4;
static int lowmem_minfree[6] = {
	3 * 512,	/* 6MB */
	2 * 1024,	/* 8MB */
	4 * 1024,	/* 16MB */
	16 * 1024,	/* 64MB */
};
static int lowmem_minfree_size = 4;

static unsigned long lowmem_deathpending_timeout;

#define lowmem_print(level, x...)			\
	do {						\
		if (lowmem_debug_level >= (level))	\
			pr_info(x);			\
	} while (0)
#if defined(CONFIG_SEC_DEBUG_LMK_MEMINFO)
static void dump_tasks_info(void)
{
	struct task_struct *p;
	struct task_struct *task;

	pr_info("[ pid ]   uid	tgid total_vm	   rss swap cpu oom_adj oom_score_adj name\n");
	for_each_process(p) {
		/* check unkillable tasks */
		if (is_global_init(p))
			continue;
		if (p->flags & PF_KTHREAD)
			continue;

		task = find_lock_task_mm(p);
		if (!task) {
			/*
			* This is a kthread or all of p's threads have already
			* detached their mm's.	There's no need to report
			* them; they can't be oom killed anyway.
			*/
			continue;
		}

		pr_info("[%5d] %5d %5d %8lu %8lu %8lu %3u	 %3d	     %5d %s\n",
		task->pid, task_uid(task), task->tgid,
		task->mm->total_vm, get_mm_rss(task->mm), get_mm_counter(task->mm, MM_SWAPENTS),
		task_cpu(task), task->signal->oom_adj,
		task->signal->oom_score_adj, task->comm);
		task_unlock(task);
	}
}
#endif

static int test_task_flag(struct task_struct *p, int flag)
{
	struct task_struct *t = p;

	do {
		task_lock(t);
		if (test_tsk_thread_flag(t, flag)) {
			task_unlock(t);
			return 1;
		}
		task_unlock(t);
	} while_each_thread(p, t);

	return 0;
}

static DEFINE_MUTEX(scan_mutex);

#if defined(CONFIG_CMA_PAGE_COUNTING)
#define SSWAP_LMK_THRESHOLD	(30720 * 2)
#define CMA_PAGE_RATIO		70
#endif

#if defined(CONFIG_ZSWAP)
extern atomic_t zswap_pool_pages;
extern atomic_t zswap_stored_pages;
#endif

static int lowmem_shrink(struct shrinker *s, struct shrink_control *sc)
{
	struct task_struct *tsk;
	struct task_struct *selected = NULL;
	int rem = 0;
	int tasksize;
	int i;
	int min_score_adj = OOM_SCORE_ADJ_MAX + 1;
	int minfree = 0;
	int selected_tasksize = 0;
	int selected_oom_score_adj;
#ifdef CONFIG_SAMP_HOTNESS
	int selected_hotness_adj = 0;
#endif
	int array_size = ARRAY_SIZE(lowmem_adj);
	int other_free;
	int other_file;
	unsigned long nr_to_scan = sc->nr_to_scan;
#ifdef CONFIG_SEC_DEBUG_LMK_MEMINFO
	static DEFINE_RATELIMIT_STATE(lmk_rs, DEFAULT_RATELIMIT_INTERVAL, 1);
#endif
	unsigned long nr_cma_free;
	struct reclaim_state *reclaim_state = current->reclaim_state;
#if defined(CONFIG_CMA_PAGE_COUNTING)
	unsigned long nr_cma_inactive_file;
	unsigned long nr_cma_active_file;
	unsigned long cma_page_ratio;
	bool is_active_high;
	bool flag = 0;
#endif

	if (nr_to_scan > 0) {
		if (mutex_lock_interruptible(&scan_mutex) < 0)
			return 0;
	}

	other_free = global_page_state(NR_FREE_PAGES) - totalreserve_pages;

	nr_cma_free = global_page_state(NR_FREE_CMA_PAGES);
#ifdef CONFIG_ZSWAP
	if (!current_is_kswapd() || sc->priority <= 6)
#endif
		other_free -= nr_cma_free;

#if defined(CONFIG_CMA_PAGE_COUNTING)
	nr_cma_inactive_file = global_page_state(NR_CMA_INACTIVE_FILE);
	nr_cma_active_file = global_page_state(NR_CMA_ACTIVE_FILE);
	cma_page_ratio = 100 * global_page_state(NR_CMA_INACTIVE_FILE) /
				global_page_state(NR_INACTIVE_FILE);
	is_active_high = (global_page_state(NR_ACTIVE_FILE) >
				global_page_state(NR_INACTIVE_FILE)) ? 1 : 0;
#endif
	other_file = global_page_state(NR_FILE_PAGES) - global_page_state(NR_SHMEM) + zcache_pages();

#if defined(CONFIG_CMA_PAGE_COUNTING) && defined(CONFIG_EXCLUDE_LRU_LIVING_IN_CMA)
	if (get_nr_swap_pages() < SSWAP_LMK_THRESHOLD && cma_page_ratio >= CMA_PAGE_RATIO
			&& !is_active_high) {
		other_file = other_file - (nr_cma_inactive_file + nr_cma_active_file);
		flag = 1;
	}
#endif
	if (global_page_state(NR_SHMEM) + total_swapcache_pages() < other_file)
		other_file -= global_page_state(NR_SHMEM) + total_swapcache_pages();
	else
		other_file = 0;

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;
	if (lowmem_minfree_size < array_size)
		array_size = lowmem_minfree_size;
	for (i = 0; i < array_size; i++) {
		minfree = lowmem_minfree[i];
		if (other_free < minfree && other_file < minfree) {
			min_score_adj = lowmem_adj[i];
			break;
		}
	}
	if (nr_to_scan > 0)
		lowmem_print(3, "lowmem_shrink %lu, %x, ofree %d %d, ma %d\n",
				nr_to_scan, sc->gfp_mask, other_free,
				other_file, min_score_adj);
	rem = global_page_state(NR_ACTIVE_ANON) +
		global_page_state(NR_ACTIVE_FILE) +
		global_page_state(NR_INACTIVE_ANON) +
		global_page_state(NR_INACTIVE_FILE);
	if (nr_to_scan <= 0 || min_score_adj == OOM_SCORE_ADJ_MAX + 1) {
		lowmem_print(5, "lowmem_shrink %lu, %x, return %d\n",
			     nr_to_scan, sc->gfp_mask, rem);

		if (nr_to_scan > 0)
			mutex_unlock(&scan_mutex);

		return rem;
	}
	selected_oom_score_adj = min_score_adj;

	rcu_read_lock();
	for_each_process(tsk) {
		struct task_struct *p;
		int oom_score_adj;
#ifdef CONFIG_SAMP_HOTNESS
		int hotness_adj = 0;
#endif

		if (tsk->flags & PF_KTHREAD)
			continue;

		/* if task no longer has any memory ignore it */
		if (test_task_flag(tsk, TIF_MM_RELEASED))
			continue;

		if (time_before_eq(jiffies, lowmem_deathpending_timeout)) {
			if (test_task_flag(tsk, TIF_MEMDIE)) {
				rcu_read_unlock();
				/* give the system time to free up the memory */
				msleep_interruptible(20);
				mutex_unlock(&scan_mutex);
				return 0;
			}
		}

		p = find_lock_task_mm(tsk);
		if (!p)
			continue;

		oom_score_adj = p->signal->oom_score_adj;
		if (oom_score_adj < min_score_adj) {
			task_unlock(p);
			continue;
		}
		tasksize = get_mm_rss(p->mm);
#if defined(CONFIG_ZSWAP)
		if (atomic_read(&zswap_stored_pages)) {
			lowmem_print(3, "shown tasksize : %d\n", tasksize);
			tasksize += atomic_read(&zswap_pool_pages) * get_mm_counter(p->mm, MM_SWAPENTS)
				/ atomic_read(&zswap_stored_pages);
			lowmem_print(3, "real tasksize : %d\n", tasksize);
		}
#endif

#ifdef CONFIG_SAMP_HOTNESS
		hotness_adj = p->signal->hotness_adj;
#endif
		task_unlock(p);
		if (tasksize <= 0)
			continue;
		if (selected) {
#ifdef CONFIG_SAMP_HOTNESS
			if (min_score_adj <= lowmem_adj[4]) {
#endif
			if (oom_score_adj < selected_oom_score_adj)
				continue;
			if (oom_score_adj == selected_oom_score_adj &&
			    tasksize <= selected_tasksize)
				continue;
#ifdef CONFIG_SAMP_HOTNESS
			} else {
				if (hotness_adj > selected_hotness_adj)
					continue;
				if (hotness_adj == selected_hotness_adj && tasksize <= selected_tasksize)
					continue;
			}
#endif
		}
		selected = p;
		selected_tasksize = tasksize;
		selected_oom_score_adj = oom_score_adj;
#ifdef CONFIG_SAMP_HOTNESS
		selected_hotness_adj = hotness_adj;
#endif
		lowmem_print(2, "select %d (%s), adj %d, size %d, to kill\n",
			     p->pid, p->comm, oom_score_adj, tasksize);
	}
	if (selected) {
#if defined(CONFIG_CMA_PAGE_COUNTING)
#ifdef CONFIG_SAMP_HOTNESS
		lowmem_print(1, "send sigkill to %d (%s), adj %d, size %d, "
			"ofree %d, ofile %d(%c), is_kswapd %d - "
			"cma_free %lu priority %d cma_i_file %lu cma_a_file %lu, hotness %d\n",
			selected->pid, selected->comm,
			selected_oom_score_adj, selected_tasksize,
			other_free, other_file, flag ? '-' : '+',
			!!current_is_kswapd(),
			nr_cma_free, sc->priority,
			nr_cma_inactive_file, nr_cma_active_file, selected_hotness_adj);
#else
		lowmem_print(1, "send sigkill to %d (%s), adj %d, size %d, "
			"ofree %d, ofile %d(%c), is_kswapd %d - "
			"cma_free %lu priority %d cma_i_file %lu cma_a_file %lu\n",
			selected->pid, selected->comm,
			selected_oom_score_adj, selected_tasksize,
			other_free, other_file, flag ? '-' : '+',
			!!current_is_kswapd(),
			nr_cma_free, sc->priority,
			nr_cma_inactive_file, nr_cma_active_file);
#endif

#else
#ifdef CONFIG_SAMP_HOTNESS
		lowmem_print(1, "send sigkill to %d (%s), adj %d, size %d, "
				"free memory = %d, reclaimable memory = %d "
				"is_kswapd %d cma_free %lu priority %d, hotness %d\n",
				selected->pid, selected->comm,
				selected_oom_score_adj, selected_tasksize,
				other_free, other_file,
				!!current_is_kswapd(),
				nr_cma_free, sc->priority, selected_hotness_adj);
#else
		lowmem_print(1, "send sigkill to %d (%s), adj %d, size %d, "
				"free memory = %d, reclaimable memory = %d "
				"is_kswapd %d cma_free %lu priority %d\n",
				selected->pid, selected->comm,
				selected_oom_score_adj, selected_tasksize,
				other_free, other_file,
				!!current_is_kswapd(),
				nr_cma_free, sc->priority);
#endif
#endif
		lowmem_deathpending_timeout = jiffies + HZ;
		send_sig(SIGKILL, selected, 0);
		set_tsk_thread_flag(selected, TIF_MEMDIE);
		rem -= selected_tasksize;
		rcu_read_unlock();
#ifdef LMK_COUNT_READ
                lmk_count++;
#endif

#ifdef CONFIG_SEC_DEBUG_LMK_MEMINFO
		if ((selected_oom_score_adj < lowmem_adj[5]) && __ratelimit(&lmk_rs)) {
			lowmem_print(1, "lowmem_shrink %lu, %x, ofree %d %d, ma %d\n",
					nr_to_scan, sc->gfp_mask, other_free,
					other_file, min_score_adj);
			show_mem(SHOW_MEM_FILTER_NODES);
			dump_tasks_info();
		}
#endif
		/* give the system time to free up the memory */
		msleep_interruptible(20);
		if(reclaim_state)
			reclaim_state->reclaimed_slab = selected_tasksize;
	} else
		rcu_read_unlock();

	lowmem_print(4, "lowmem_shrink %lu, %x, return %d\n",
		     nr_to_scan, sc->gfp_mask, rem);
	mutex_unlock(&scan_mutex);
	return rem;
}

/*
 * CONFIG_SEC_OOM_KILLER : klaatu@sec
 *
 * The way to select victim by oom-killer provided by
 * linux kernel is totally different from android policy.
 * Hence, it makes more sense that we select the oom victim
 * as android does when LMK is invoked.
 *
*/
#ifdef CONFIG_SEC_OOM_KILLER

static int android_oom_handler(struct notifier_block *nb,
				      unsigned long val, void *data)
{
	struct task_struct *tsk;
#ifdef MULTIPLE_OOM_KILLER
	struct task_struct *selected[OOM_DEPTH] = {NULL,};
#else
	struct task_struct *selected = NULL;
#endif
	int rem = 0;
	int tasksize;
	int i;
	int min_score_adj = OOM_SCORE_ADJ_MAX + 1;
#ifdef MULTIPLE_OOM_KILLER
	int selected_tasksize[OOM_DEPTH] = {0,};
	int selected_oom_score_adj[OOM_DEPTH] = {OOM_ADJUST_MAX,};
	int all_selected_oom = 0;
	int max_selected_oom_idx = 0;
#else
	int selected_tasksize = 0;
	int selected_oom_score_adj;
#endif
#ifdef CONFIG_SEC_DEBUG_LMK_MEMINFO
	static DEFINE_RATELIMIT_STATE(oom_rs, DEFAULT_RATELIMIT_INTERVAL/5, 1);
#endif

	unsigned long *freed = data;
#if defined(CONFIG_CMA_PAGE_COUNTING)
	unsigned long nr_cma_free;
	unsigned long nr_cma_inactive_file;
	unsigned long nr_cma_active_file;
	int other_free;
	int other_file;

	nr_cma_free = global_page_state(NR_FREE_CMA_PAGES);
	other_free = global_page_state(NR_FREE_PAGES) - nr_cma_free;

	nr_cma_inactive_file = global_page_state(NR_CMA_INACTIVE_FILE);
	nr_cma_active_file = global_page_state(NR_CMA_ACTIVE_FILE);
	other_file = global_page_state(NR_FILE_PAGES) + zcache_pages() -
					global_page_state(NR_SHMEM) -
					total_swapcache_pages -
					nr_cma_inactive_file -
					nr_cma_active_file;
#endif

	/* show status */
	pr_warning("%s invoked Android-oom-killer: "
		"oom_adj=%d, oom_score_adj=%d\n",
		current->comm, current->signal->oom_adj,
		current->signal->oom_score_adj);
#ifdef CONFIG_SEC_DEBUG_LMK_MEMINFO
	if (__ratelimit(&oom_rs)) {
		dump_stack();
		show_mem(SHOW_MEM_FILTER_NODES);
		dump_tasks_info();
	}
#endif

	min_score_adj = 0;
#ifdef MULTIPLE_OOM_KILLER
	for (i = 0; i < OOM_DEPTH; i++)
		selected_oom_score_adj[i] = min_score_adj;
#else
	selected_oom_score_adj = min_score_adj;
#endif

	read_lock(&tasklist_lock);
	for_each_process(tsk) {
		struct task_struct *p;
		int oom_score_adj;
#ifdef MULTIPLE_OOM_KILLER
		int is_exist_oom_task = 0;
#endif

		if (tsk->flags & PF_KTHREAD)
			continue;

		p = find_lock_task_mm(tsk);
		if (!p)
			continue;

		oom_score_adj = p->signal->oom_score_adj;
		if (oom_score_adj < min_score_adj) {
			task_unlock(p);
			continue;
		}
		tasksize = get_mm_rss(p->mm);
		task_unlock(p);
		if (tasksize <= 0)
			continue;

		lowmem_print(2, "oom: ------ %d (%s), adj %d, size %d\n",
			     p->pid, p->comm, oom_score_adj, tasksize);
#ifdef MULTIPLE_OOM_KILLER
		if (all_selected_oom < OOM_DEPTH) {
			for (i = 0; i < OOM_DEPTH; i++) {
				if (!selected[i]) {
					is_exist_oom_task = 1;
					max_selected_oom_idx = i;
					break;
				}
			}
		} else if (selected_oom_score_adj[max_selected_oom_idx] < oom_score_adj ||
			(selected_oom_score_adj[max_selected_oom_idx] == oom_score_adj &&
			selected_tasksize[max_selected_oom_idx] < tasksize)) {
			is_exist_oom_task = 1;
		}

		if (is_exist_oom_task) {
			selected[max_selected_oom_idx] = p;
			selected_tasksize[max_selected_oom_idx] = tasksize;
			selected_oom_score_adj[max_selected_oom_idx] = oom_score_adj;

			if (all_selected_oom < OOM_DEPTH)
				all_selected_oom++;

			if (all_selected_oom == OOM_DEPTH) {
				for (i = 0; i < OOM_DEPTH; i++) {
					if (selected_oom_score_adj[i] < selected_oom_score_adj[max_selected_oom_idx])
						max_selected_oom_idx = i;
					else if (selected_oom_score_adj[i] == selected_oom_score_adj[max_selected_oom_idx] &&
						selected_tasksize[i] < selected_tasksize[max_selected_oom_idx])
						max_selected_oom_idx = i;
				}
			}

			lowmem_print(2, "oom: max_selected_oom_idx(%d) select %d (%s), adj %d, \
					size %d, to kill\n",
				max_selected_oom_idx, p->pid, p->comm, oom_score_adj, tasksize);
		}
#else
		if (selected) {
			if (oom_score_adj < selected_oom_score_adj)
				continue;
			if (oom_score_adj == selected_oom_score_adj &&
			    tasksize <= selected_tasksize)
				continue;
		}
		selected = p;
		selected_tasksize = tasksize;
		selected_oom_score_adj = oom_score_adj;
		lowmem_print(2, "oom: select %d (%s), adj %d, size %d, to kill\n",
			     p->pid, p->comm, oom_score_adj, tasksize);
#endif
	}
#ifdef MULTIPLE_OOM_KILLER
	for (i = 0; i < OOM_DEPTH; i++) {
		if (selected[i]) {
#if defined(CONFIG_CMA_PAGE_COUNTING)
			lowmem_print(1, "oom: send sigkill to %d (%s), adj %d, "
				"size %d ofree %d ofile %d "
				"cma_free %lu cma_i_file %lu cma_a_file %lu\n",
				selected[i]->pid, selected[i]->comm,
				selected_oom_score_adj[i],
				selected_tasksize[i],
				other_free, other_file,
				nr_cma_free, nr_cma_inactive_file, nr_cma_active_file);
#else
			lowmem_print(1, "oom: send sigkill to %d (%s), adj %d,\
				     size %d\n",
				     selected[i]->pid, selected[i]->comm,
				     selected_oom_score_adj[i],
				     selected_tasksize[i]);
#endif
			send_sig(SIGKILL, selected[i], 0);
			rem -= selected_tasksize[i];
			*freed += (unsigned long)selected_tasksize[i];
#ifdef OOM_COUNT_READ
			oom_count++;
#endif

		}
	}
#else
	if (selected) {
		lowmem_print(1, "Killing '%s' (%d), adj %d,\n" \
				"   to free %ldkB on behalf of '%s' (%d) because\n" \
				"   cache %ldkB is below limit %ldkB for oom_score_adj %d\n" \
				"   Free memory is %ldkB above reserved\n",
			     selected->comm, selected->pid,
			     selected_oom_score_adj,
			     selected_tasksize * (long)(PAGE_SIZE / 1024),
			     current->comm, current->pid,
			     other_file * (long)(PAGE_SIZE / 1024),
			     minfree * (long)(PAGE_SIZE / 1024),
			     min_score_adj,
			     other_free * (long)(PAGE_SIZE / 1024));
		send_sig(SIGKILL, selected, 0);
		set_tsk_thread_flag(selected, TIF_MEMDIE);
		rem -= selected_tasksize;
		*freed += (unsigned long)selected_tasksize;
#ifdef OOM_COUNT_READ
		oom_count++;
#endif
	}
#endif
	read_unlock(&tasklist_lock);

	lowmem_print(2, "oom: get memory %lu", *freed);
	return rem;
}

static struct notifier_block android_oom_notifier = {
	.notifier_call = android_oom_handler,
};
#endif /* CONFIG_SEC_OOM_KILLER */

static struct shrinker lowmem_shrinker = {
	.shrink = lowmem_shrink,
	.seeks = DEFAULT_SEEKS * 16
};

static int __init lowmem_init(void)
{
	register_shrinker(&lowmem_shrinker);
#ifdef CONFIG_SEC_OOM_KILLER
	register_oom_notifier(&android_oom_notifier);
#endif

	return 0;
}

static void __exit lowmem_exit(void)
{
	unregister_shrinker(&lowmem_shrinker);
}

#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES
static int lowmem_oom_adj_to_oom_score_adj(int oom_adj)
{
	if (oom_adj == OOM_ADJUST_MAX)
		return OOM_SCORE_ADJ_MAX;
	else
		return (oom_adj * OOM_SCORE_ADJ_MAX) / -OOM_DISABLE;
}

static void lowmem_autodetect_oom_adj_values(void)
{
	int i;
	int oom_adj;
	int oom_score_adj;
	int array_size = ARRAY_SIZE(lowmem_adj);

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;

	if (array_size <= 0)
		return;

	oom_adj = lowmem_adj[array_size - 1];
	if (oom_adj > OOM_ADJUST_MAX)
		return;

	oom_score_adj = lowmem_oom_adj_to_oom_score_adj(oom_adj);
	if (oom_score_adj <= OOM_ADJUST_MAX)
		return;

	lowmem_print(1, "lowmem_shrink: convert oom_adj to oom_score_adj:\n");
	for (i = 0; i < array_size; i++) {
		oom_adj = lowmem_adj[i];
		oom_score_adj = lowmem_oom_adj_to_oom_score_adj(oom_adj);
		lowmem_adj[i] = oom_score_adj;
		lowmem_print(1, "oom_adj %d => oom_score_adj %d\n",
			     oom_adj, oom_score_adj);
	}
}

static int lowmem_adj_array_set(const char *val, const struct kernel_param *kp)
{
	int ret;

	ret = param_array_ops.set(val, kp);

	/* HACK: Autodetect oom_adj values in lowmem_adj array */
	lowmem_autodetect_oom_adj_values();

	return ret;
}

static int lowmem_adj_array_get(char *buffer, const struct kernel_param *kp)
{
	return param_array_ops.get(buffer, kp);
}

static void lowmem_adj_array_free(void *arg)
{
	param_array_ops.free(arg);
}

static struct kernel_param_ops lowmem_adj_array_ops = {
	.set = lowmem_adj_array_set,
	.get = lowmem_adj_array_get,
	.free = lowmem_adj_array_free,
};

static const struct kparam_array __param_arr_adj = {
	.max = ARRAY_SIZE(lowmem_adj),
	.num = &lowmem_adj_size,
	.ops = &param_ops_int,
	.elemsize = sizeof(lowmem_adj[0]),
	.elem = lowmem_adj,
};
#endif

module_param_named(cost, lowmem_shrinker.seeks, int, S_IRUGO | S_IWUSR);
#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES
__module_param_call(MODULE_PARAM_PREFIX, adj,
		    &lowmem_adj_array_ops,
		    .arr = &__param_arr_adj,
		    S_IRUGO | S_IWUSR, -1);
__MODULE_PARM_TYPE(adj, "array of int");
#else
module_param_array_named(adj, lowmem_adj, int, &lowmem_adj_size,
			 S_IRUGO | S_IWUSR);
#endif
module_param_array_named(minfree, lowmem_minfree, uint, &lowmem_minfree_size,
			 S_IRUGO | S_IWUSR);
module_param_named(debug_level, lowmem_debug_level, uint, S_IRUGO | S_IWUSR);
#ifdef LMK_COUNT_READ
module_param_named(lmkcount, lmk_count, uint, S_IRUGO);
#endif


#ifdef OOM_COUNT_READ
module_param_named(oomcount, oom_count, uint, S_IRUGO);
#endif

module_init(lowmem_init);
module_exit(lowmem_exit);

MODULE_LICENSE("GPL");

