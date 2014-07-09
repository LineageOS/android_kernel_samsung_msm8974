/*
 * drivers/cpufreq/cpufreq_limit.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *	Minsung Kim <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/cpufreq.h>
#include <linux/cpufreq_limit.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/suspend.h>

struct cpufreq_limit_handle {
	struct list_head node;
	unsigned long min;
	unsigned long max;
	char label[20];
};

static DEFINE_MUTEX(cpufreq_limit_lock);
static LIST_HEAD(cpufreq_limit_requests);

#if defined(CONFIG_ARCH_MSM8974PRO)
static int suspend_boost = 1190400;
module_param(suspend_boost, uint, 0644);
#endif

/**
 * cpufreq_limit_get - limit min_freq or max_freq, return cpufreq_limit_handle
 * @min_freq	limit minimum frequency (0: none)
 * @max_freq	limit maximum frequency (0: none)
 * @label	a literal description string of this request
 */
struct cpufreq_limit_handle *cpufreq_limit_get(unsigned long min_freq,
		unsigned long max_freq, char *label)
{
	struct cpufreq_limit_handle *handle;
	int i;

	if (max_freq && max_freq < min_freq)
		return ERR_PTR(-EINVAL);

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (!handle)
		return ERR_PTR(-ENOMEM);

	pr_debug("%s: %s,%lu,%lu\n", __func__, handle->label, handle->min,
			handle->max);

	handle->min = min_freq;
	handle->max = 0;

	if (strlen(label) < sizeof(handle->label))
		strcpy(handle->label, label);
	else
		strncpy(handle->label, label, sizeof(handle->label) - 1);

	mutex_lock(&cpufreq_limit_lock);
	list_add_tail(&handle->node, &cpufreq_limit_requests);
	mutex_unlock(&cpufreq_limit_lock);

	for_each_online_cpu(i)
		cpufreq_update_policy(i);

	return handle;
}

/**
 * cpufreq_limit_put - release of a limit of min_freq or max_freq, free
 *			a cpufreq_limit_handle
 * @handle	a cpufreq_limit_handle that has been requested
 */
int cpufreq_limit_put(struct cpufreq_limit_handle *handle)
{
	int i;

	if (handle == NULL || IS_ERR(handle))
		return -EINVAL;

	pr_debug("%s: %s,%lu,%lu\n", __func__, handle->label, handle->min,
			handle->max);

	mutex_lock(&cpufreq_limit_lock);
	list_del(&handle->node);
	mutex_unlock(&cpufreq_limit_lock);

	for_each_online_cpu(i)
		cpufreq_update_policy(i);

	kfree(handle);
	return 0;
}

static int cpufreq_limit_notifier_policy(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct cpufreq_policy *policy = data;
	struct cpufreq_limit_handle *handle;
	unsigned long min = 0, max = ULONG_MAX;

	if (val != CPUFREQ_ADJUST)
		goto done;

	mutex_lock(&cpufreq_limit_lock);
	list_for_each_entry(handle, &cpufreq_limit_requests, node) {
		if (handle->min > min)
			min = handle->min;
		if (handle->max && handle->max < max)
			max = handle->max;
	}

#ifdef CONFIG_SEC_PM
	pr_debug("CPUFREQ(%d): %s: umin=%d,umax=%d\n",
		policy->cpu, __func__, policy->user_policy.min, policy->user_policy.max);

	if (policy->user_policy.min > min)
		min = policy->user_policy.min;
	if (policy->user_policy.max && policy->user_policy.max < max)
		max = policy->user_policy.max;
#endif

	mutex_unlock(&cpufreq_limit_lock);

	if (!min && max == ULONG_MAX)
		goto done;

	if (!min)
		min = policy->cpuinfo.min_freq;
	if (max == ULONG_MAX)
		max = policy->cpuinfo.max_freq;

	pr_debug("%s: limiting cpu%d cpufreq to %lu,%lu", __func__,
			policy->cpu, min, max);

	cpufreq_verify_within_limits(policy, min, max);
done:
	return 0;

}

static struct notifier_block notifier_policy_block = {
	.notifier_call = cpufreq_limit_notifier_policy
};

static ssize_t show_cpufreq_limit_requests(struct kobject *kobj,
		struct attribute *attr, char *buf)
{
	struct cpufreq_limit_handle *handle;
	ssize_t len = 0;

	mutex_lock(&cpufreq_limit_lock);
	list_for_each_entry(handle, &cpufreq_limit_requests, node) {
		len += sprintf(buf + len, "%s\t%lu\t%lu\n", handle->label,
				handle->min, handle->max);
	}
	mutex_unlock(&cpufreq_limit_lock);

	return len;
}

static struct global_attr cpufreq_limit_requests_attr =
	__ATTR(cpufreq_limit_requests, 0444, show_cpufreq_limit_requests, NULL);

static struct attribute *limit_attributes[] = {
	&cpufreq_limit_requests_attr.attr,
	NULL,
};

static struct attribute_group limit_attr_group = {
	.attrs = limit_attributes,
	.name = "cpufreq_limit",
};

#if defined(CONFIG_ARCH_MSM8974PRO)
static int cpufreq_limit_suspend_handler(struct notifier_block *nb,
				unsigned long val, void *data)
{
	static struct cpufreq_limit_handle *cpufreq_suspend;

	switch (val) {
	case PM_SUSPEND_PREPARE:
		pr_info("%s: limit suspend_boost %d\n", __func__, suspend_boost);
		cpufreq_suspend = cpufreq_limit_min_freq(suspend_boost, "cpufreq_suspend");
		if (IS_ERR(cpufreq_suspend)) {
			pr_err("%s: fail to get the handle\n", __func__);
			goto out;
		}

		break;

	case PM_POST_SUSPEND:
		pr_info("%s: release suspend_boost\n", __func__);
		cpufreq_limit_put(cpufreq_suspend);
		break;

	default:
		return NOTIFY_DONE;
	}

out:
	return NOTIFY_OK;
}
#endif

static int __init cpufreq_limit_init(void)
{
	int ret;

	ret = cpufreq_register_notifier(&notifier_policy_block,
				CPUFREQ_POLICY_NOTIFIER);
	if (ret)
		return ret;

	ret = sysfs_create_group(cpufreq_global_kobject,
			&limit_attr_group);
	if (ret)
		return ret;

#if defined(CONFIG_ARCH_MSM8974PRO)
	pm_notifier(cpufreq_limit_suspend_handler, 0);
#endif
	return 0;
}

static void __exit cpufreq_limit_exit(void)
{
	cpufreq_unregister_notifier(&notifier_policy_block,
			CPUFREQ_POLICY_NOTIFIER);

	sysfs_remove_group(cpufreq_global_kobject, &limit_attr_group);
}

MODULE_AUTHOR("Minsung Kim <ms925.kim@samsung.com>");
MODULE_DESCRIPTION("'cpufreq_limit' - A driver to limit cpu frequency");
MODULE_LICENSE("GPL");

module_init(cpufreq_limit_init);
module_exit(cpufreq_limit_exit);
