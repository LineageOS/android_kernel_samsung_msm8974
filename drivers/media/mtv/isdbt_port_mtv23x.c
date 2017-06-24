/*
*
* drivers/media/tdmb/isdbt_port_mtv23x.c
*
* isdbt driver
*
* Copyright (C) (2014, Samsung Electronics)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fcntl.h>

#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#include <linux/fs.h>
#include <linux/uaccess.h>

#include <linux/time.h>
#include <linux/timer.h>

#include <linux/vmalloc.h>

#include "isdbt.h"
#include "isdbt_port_mtv23x.h"

#ifndef VM_RESERVED		/* for kernel 3.10 */
#define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

extern void mtv23x_set_port_if(unsigned long interface);

static bool mtv23x_on_air = false;

struct MTV23x_CB *mtv23x_cb_ptr = NULL;

#ifdef _MTV_KERNEL_FILE_DUMP_ENABLE
	struct file *mtv_ts_filp = NULL;
#endif

static void tsb_enqueue(unsigned char *ts_chunk)
{
#ifdef DEBUG_TSP_BUF
	int readi, writei, num_euqueued_tsp, num_euqueued_seg;
#endif
	struct TSB_CB_INFO *tsb_cb = &mtv23x_cb_ptr->tsb_cb;

	if (!tsb_cb->tsbd) {
		DMBERR("Not memory mapped\n");
		return;
	}

	if (tsb_cb->tsbd->op_enabled) {
		/* Check if the specified tspb is the allocated tspb? */
		if (ts_chunk == tsb_cb->seg_buf[tsb_cb->enqueue_seg_idx]) {
			/* Update the next index of write-tsp. */
			tsb_cb->tsbd->write_idx = tsb_cb->avail_write_tspb_idx;

			/* Update the next index of segment. */
			tsb_cb->enqueue_seg_idx = tsb_cb->avail_seg_idx;

#ifdef DEBUG_TSP_BUF
			readi = tsb_cb->tsbd->read_idx;
			writei = tsb_cb->tsbd->write_idx;

			if (writei > readi)
				num_euqueued_tsp = writei - readi;
			else if (writei < readi)
				num_euqueued_tsp = tsb_cb->num_total_tsp - (readi - writei);
			else
				num_euqueued_tsp = 0;

			mtv23x_cb_ptr->max_enqueued_tsp_cnt
				= MAX(mtv23x_cb_ptr->max_enqueued_tsp_cnt, num_euqueued_tsp);

			num_euqueued_seg = num_euqueued_tsp / tsb_cb->num_tsp_per_seg;
			mtv23x_cb_ptr->max_enqueued_seg_cnt
				= MAX(mtv23x_cb_ptr->max_enqueued_seg_cnt, num_euqueued_seg);
#endif
		} else
			DMBERR("Invalid the enqueuing chunk address!\n");
	}
}

/* Get a TS buffer */
static U8 *tsb_get(void)
{
	int readi;
	int nwi; /* Next index of tsp buffer to be write. */
	struct TSB_CB_INFO *tsb_cb = &mtv23x_cb_ptr->tsb_cb;
	unsigned char *tspb = NULL;
	int num_tsp_per_seg = tsb_cb->num_tsp_per_seg;
#ifdef DEBUG_TSP_BUF
	int num_used_segment; /* Should NOT zero. */
	int write_seg_idx, read_seg_idx;
#endif

	if (!tsb_cb->tsbd) {
		DMBERR("Not memory mapped\n");
		return NULL;
	}

	if (tsb_cb->tsbd->op_enabled) {
		readi = tsb_cb->tsbd->read_idx;

		/* Get the next avaliable index of segment to be write in the next time. */
		nwi = tsb_cb->avail_write_tspb_idx + num_tsp_per_seg;
		if (nwi >= tsb_cb->num_total_tsp)
			nwi = 0;

		if ((readi < nwi) || (readi >= (nwi + num_tsp_per_seg))) {
			tspb = tsb_cb->seg_buf[tsb_cb->avail_seg_idx];

			/* Update the writting index of tsp buffer. */
			tsb_cb->avail_write_tspb_idx = nwi;

			/* Update the avaliable index of segment to be write in the next time. */
			if (++tsb_cb->avail_seg_idx >= tsb_cb->num_total_seg)
				tsb_cb->avail_seg_idx = 0;

#ifdef DEBUG_TSP_BUF
			write_seg_idx = tsb_cb->avail_seg_idx;
			read_seg_idx = readi / num_tsp_per_seg;

			if (write_seg_idx > read_seg_idx)
				num_used_segment = write_seg_idx - read_seg_idx;
			else
				num_used_segment
					= tsb_cb->num_total_seg - (read_seg_idx - write_seg_idx);

			DMBMSG("wseg_idx(%d), rseg_idx(%d), num_used_segment(%d)\n",
						write_seg_idx, read_seg_idx, num_used_segment);

			mtv23x_cb_ptr->max_alloc_seg_cnt
				= MAX(mtv23x_cb_ptr->max_alloc_seg_cnt, num_used_segment);
#endif

			//DMBMSG("@@ readi(%d), next_writei(%d), avail_seg_idx(%d), tspb(0x%08lX)\n",
			//		readi, nwi, tsb_cb->avail_seg_idx, (unsigned long)tspb);
		} else
			DMBERR("Full tsp buffer.\n");
	}

	return tspb;
}

static inline void tsb_free_mapping_area(void)
{
	int i;
	unsigned int order;
	struct TSB_CB_INFO *tsb_cb = &mtv23x_cb_ptr->tsb_cb;

	order = get_order(tsb_cb->seg_size);
	for (i = 0; i < tsb_cb->num_total_seg; i++) {
		if (tsb_cb->seg_buf[i]) {
			//DMBMSG("SEG[%d]: seg_buf(0x%lX)\n", i, (unsigned long)tsb_cb->seg_buf[i]);
			free_pages((unsigned long)tsb_cb->seg_buf[i], order);
			tsb_cb->seg_buf[i] = NULL;
		}
	}

	tsb_cb->seg_bufs_allocated = false;
	tsb_cb->seg_size = 0;
	tsb_cb->num_total_seg = 0;

	if (tsb_cb->tsbd) {
		order = get_order(tsb_cb->desc_size);
		free_pages((unsigned long)tsb_cb->tsbd, order);

		tsb_cb->tsbd = NULL;
		tsb_cb->desc_size = 0;
	}
}

static inline int tsb_alloc_mapping_area(unsigned int desc_size,
										unsigned int seg_size, int num_seg)
{
	int i, ret;
	unsigned int order;
	struct TSB_CB_INFO *tsb_cb = &mtv23x_cb_ptr->tsb_cb;

	/* Allocate the TSB descriptor. */
	order = get_order(desc_size);
	tsb_cb->tsbd
		= (struct TSB_DESC_INFO *)__get_dma_pages(GFP_KERNEL, order);
	if (!tsb_cb->tsbd) {
		DMBMSG("DESC allocation error\n");
		return -ENOMEM;
	}

	/* Allocate the TSB segments. */
	order = get_order(seg_size);
	DMBMSG("SEG order(%u)\n", order);

	if (order > MAX_ORDER) {
		DMBMSG("Invalid page order value of segment (%u)\n", order);
		ret = -ENOMEM;
		goto free_tsb;
	}

	for (i = 0; i < num_seg; i++) {
		tsb_cb->seg_buf[i] = (U8 *)__get_dma_pages(GFP_KERNEL, order);
		if (!tsb_cb->seg_buf[i]) {
			DMBMSG("SEG[%u] allocation error\n", i);
			ret = -ENOMEM;
			goto free_tsb;
		}
	}

	tsb_cb->seg_bufs_allocated = true;

	DMBMSG("Success\n");

	return 0;

free_tsb:
	tsb_free_mapping_area();

	return ret;
}

static void mtv23x_mmap_close(struct vm_area_struct *vma)
{
	DMBMSG("Entered. mmap_completed(%d)\n", mtv23x_cb_ptr->tsb_cb.mmap_completed);

	mtv23x_cb_ptr->tsb_cb.mmap_completed = false;

	DMBMSG("Leaved...\n");
}

static const struct vm_operations_struct mtv23x_mmap_ops = {
	.close = mtv23x_mmap_close,
};

static int mtv23x_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret, num_total_seg;
	unsigned int i, mmap_size, desc_size, seg_size;
	unsigned long pfn;
	unsigned long start = vma->vm_start;
	struct TSB_CB_INFO *tsb_cb = &mtv23x_cb_ptr->tsb_cb;

	vma->vm_flags |= VM_RESERVED;
	vma->vm_ops = &mtv23x_mmap_ops;

	mmap_size = vma->vm_end - vma->vm_start;

#if 0
	DMBMSG("mmap_size(0x%X), vm_start(0x%lX), vm_page_prot(0x%lX)\n",
				mmap_size, vma->vm_start, vma->vm_page_prot);
#endif

	if (mmap_size & (~PAGE_MASK)) {
		DMBERR("Must align with PAGE size\n");
		return -EINVAL;
	}

	if (tsb_cb->mmap_completed == true) {
		DMBERR("Already mapped!\n");
		return 0;
	}

	seg_size = vma->vm_pgoff << PAGE_SHIFT;
	num_total_seg = (mmap_size - PAGE_SIZE) / seg_size;

	desc_size = mmap_size - (num_total_seg * seg_size);

	/* Save */
	tsb_cb->desc_size = desc_size;
	tsb_cb->seg_size = seg_size;
	tsb_cb->num_total_seg = num_total_seg;

#if 1
	DMBMSG("mmap_size(%u), seg_size(%u) #seg(%d), desc_size(%u)\n",
				mmap_size, seg_size, num_total_seg, desc_size);
#endif

	if (num_total_seg > MAX_NUM_TSB_SEG) {
		DMBERR("Too large request #seg! kernel(%u), req(%d)\n",
			MAX_NUM_TSB_SEG, num_total_seg);
		return -ENOMEM;
	}

	if (desc_size > MAX_TSB_DESC_SIZE) {
		DMBERR("Too large request desc size! kernel(%u), req(%u)\n",
			MAX_TSB_DESC_SIZE, desc_size);
		return -ENOMEM;
	}

	if (seg_size > MAX_TSB_SEG_SIZE) {
		DMBERR("Too large request seg size! kernel(%u), req(%u)\n",
			MAX_TSB_SEG_SIZE, seg_size);
		return -ENOMEM;
	}

	if (!tsb_cb->tsbd) {
		DMBERR("TSB DESC was NOT allocated!\n");
		return -ENOMEM;
	}

	if (tsb_cb->seg_bufs_allocated == false) {
		DMBERR("TSB SEG are NOT allocated!\n");
		return -ENOMEM;
	}

	/* Map the shared informations. */
	pfn = virt_to_phys(tsb_cb->tsbd) >> PAGE_SHIFT;
	if (remap_pfn_range(vma, vma->vm_start, pfn, desc_size, vma->vm_page_prot)) {
		DMBERR("HDR remap_pfn_range() error!\n");
		ret = -EAGAIN;
		goto out;
	}

	/* Init descriptor except the addres of segments */
	tsb_cb->tsbd->op_enabled = 0;
	tsb_cb->tsbd->read_idx = 0;
	tsb_cb->tsbd->write_idx = 0;

#if 0
	DMBMSG("tsbd(0x%lX), pfn(0x%lX), start(0x%lX)\n",
		(unsigned long)tsb_cb->tsbd, pfn, start);
#endif

	start += desc_size; /* Avdance VMA. */

	/* Allocate and map the TSP buffer segments. */
	for (i = 0; i < num_total_seg; i++) {
		pfn = virt_to_phys(tsb_cb->seg_buf[i]) >> PAGE_SHIFT;

#if 0
		DMBMSG("SEG[%d]: seg_buf(0x%lX) pfn(0x%lX) start(0x%lX)\n",
				i, (unsigned long)tsb_cb->seg_buf[i], pfn, start);
#endif

		if (remap_pfn_range(vma, start, pfn, seg_size, vma->vm_page_prot)) {
			DMBERR("SEG[%u] remap_pfn_range() error!\n", i);
			ret = -EAGAIN;
			goto out;
		}

		tsb_cb->tsbd->seg_base[i] = start;
		start += seg_size;
	}

	tsb_cb->mmap_completed = true;

	return 0;

out:
	return ret;
}

static void mtv23x_power_off(void)
{
	DMBMSG("mtv23x_power_off\n");

	if (mtv23x_cb_ptr->is_power_on) {
		mtv23x_on_air = false;
//		isdbt_control_irq(false);
		isdbt_control_gpio(false);

		mtv23x_cb_ptr->is_power_on = false;
		mtv23x_cb_ptr->tsout_enabled = false;

		RTV_GUARD_DEINIT;
	}
}

static INLINE int __mtv23x_power_on(unsigned long arg)
{
    int ret;
	enum E_RTV_BANDWIDTH_TYPE bandwidth;
	IOCTL_ISDBT_POWER_ON_INFO __user *argp
		= (IOCTL_ISDBT_POWER_ON_INFO __user *)arg;

#if defined(RTV_IF_SPI) || defined(RTV_IF_CSI656_RAW_8BIT_ENABLE)
	int i;

	for (i = 0; i < MAX_NUM_RTV_SERVICE; i++) {
		if (get_user(mtv23x_cb_ptr->intr_size[i],
				&argp->spi_intr_size[i]))
			return -EFAULT;

		DMBMSG("intr_size[%d]: %u\n", i, mtv23x_cb_ptr->intr_size[i]);
	}

	mtv23x_cb_ptr->cfged_tsp_chunk_size = 0;
#endif

	if (get_user(bandwidth,	&argp->bandwidth))
		return -EFAULT;

	mtv23x_cb_ptr->tsout_enabled = false;
	mtv23x_cb_ptr->cfged_svc = RTV_SERVICE_INVALID;

	ret = rtvMTV23x_Initialize(bandwidth);
	if (ret != RTV_SUCCESS) {
		DMBERR("Tuner initialization failed: %d\n", ret);
		if (put_user(ret, &argp->tuner_err_code))
			return -EFAULT;

		return -EIO; /* error occurred during the open() */
	}

	return ret;
}

static int mtv23x_power_on(unsigned long arg)
{
	int ret;
	IOCTL_ISDBT_POWER_ON_INFO __user *argp
			= (IOCTL_ISDBT_POWER_ON_INFO __user *)arg;

	DMBMSG("mtv23x_power_on\n");

	if (mtv23x_cb_ptr->is_power_on) {
		return 0;
	} else {
		isdbt_control_gpio(true);

		RTV_GUARD_INIT;

		mtv23x_set_port_if((unsigned long)isdbt_get_if_handle());

		ret = __mtv23x_power_on(arg);
		if (ret) {
			DMBERR("Tuner initialization failed: %d\n", ret);
				isdbt_control_gpio(false);

			if (put_user(ret, &argp->tuner_err_code))
				return -EFAULT;

			return ret;
		} else {
//			isdbt_control_irq(true);
			mtv23x_cb_ptr->is_power_on = true;
			return 0;
		}
	}
}

#if defined(RTV_IF_SPI) || defined(RTV_IF_SPI_TSIFx)
static unsigned long diff_jiffies_1st, diff_jiffies0, hours_cnt;
#endif

#define ISDBT_VALID_SVC_MASK	(1<<RTV_SERVICE_UHF_ISDBT_1seg)


/*============================================================================
 * Test IO control commands(0 ~ 10)
 *==========================================================================*/
static int test_register_io(unsigned long arg, unsigned int cmd)
{
	int ret = 0;
	unsigned int page, addr, write_data, read_cnt, i;
	U8 value;
#if defined(RTV_IF_SPI) || defined(RTV_IF_SPI_TSIFx)
	unsigned long diff_jiffies1;
	unsigned int elapsed_ms;
	unsigned long param1;
#endif
	U8 *reg_read_buf;
	U8 *src_ptr, *dst_ptr;
	IOCTL_REG_ACCESS_INFO __user *argp
				= (IOCTL_REG_ACCESS_INFO __user *)arg;

	if (mtv23x_cb_ptr->is_power_on == FALSE) {			
		DMBMSG("[mtv] Power Down state!Must Power ON\n");
		return -EFAULT;
	}

	if (get_user(page, &argp->page))
		return -EFAULT;

	if (get_user(addr, &argp->addr))
		return -EFAULT;

	RTV_GUARD_LOCK;

	switch (cmd) {
	case IOCTL_TEST_REG_SINGLE_READ:
		RTV_REG_MAP_SEL(page);
		value = RTV_REG_GET(addr);
		if (put_user(value, &argp->read_data[0])) {
			ret = -EFAULT;
			goto regio_exit;
		}
		break;

	case IOCTL_TEST_REG_BURST_READ:
		if (get_user(read_cnt, &argp->read_cnt)) {
			ret = -EFAULT;
			goto regio_exit;
		}

		reg_read_buf = kmalloc(MAX_NUM_MTV_REG_READ_BUF, GFP_KERNEL);
		if (reg_read_buf == NULL) {
			DMBERR("Register buffer allocation error\n");
			ret = -ENOMEM;
			goto regio_exit;
		}

		RTV_REG_MAP_SEL(page);
		RTV_REG_BURST_GET(addr, reg_read_buf, read_cnt);
		src_ptr = &reg_read_buf[0];
		dst_ptr = argp->read_data;

		for (i = 0; i< read_cnt; i++, src_ptr++, dst_ptr++) {
			if(put_user(*src_ptr, dst_ptr)) {
				ret = -EFAULT;
				break;
			}
		}

		kfree(reg_read_buf);
		break;

	case IOCTL_TEST_REG_WRITE:
		if (get_user(write_data, &argp->write_data)) {
			ret = -EFAULT;
			goto regio_exit;
		}

		RTV_REG_MAP_SEL(page);
		RTV_REG_SET(addr, write_data);
		break;

	case IOCTL_TEST_REG_SPI_MEM_READ:
	#if defined(RTV_IF_SPI)
		if (get_user(write_data, &argp->write_data)) {
			ret = -EFAULT;
			goto regio_exit;
		}

		if (get_user(read_cnt, &argp->read_cnt)) {
			ret = -EFAULT;
			goto regio_exit;
		}

		if (get_user(param1, &argp->param1)) {
			ret = -EFAULT;
			goto regio_exit;
		}

		reg_read_buf = kmalloc(MAX_NUM_MTV_REG_READ_BUF, GFP_KERNEL);
		if (reg_read_buf == NULL) {
			DMBERR("Register buffer allocation error\n");
			ret = -ENOMEM;
			goto regio_exit;
		}

		if (param1 == 0) {
			diff_jiffies_1st = diff_jiffies0 = get_jiffies_64();
			hours_cnt = 0;
			DMBMSG("START [AGING SPI Memory Test with Single IO]\n");
		}

		RTV_REG_MAP_SEL(page);
		RTV_REG_SET(addr, write_data);

		RTV_REG_MAP_SEL(SPI_MEM_PAGE);
		RTV_REG_BURST_GET(0x10, reg_read_buf, read_cnt);

		RTV_REG_MAP_SEL(page);
		value = RTV_REG_GET(addr);

		diff_jiffies1 = get_jiffies_64();
		elapsed_ms = jiffies_to_msecs(diff_jiffies1-diff_jiffies0);
		if (elapsed_ms >= (1000 * 60 * 60)) {
			diff_jiffies0 = get_jiffies_64(); /* Re-start */
			hours_cnt++;
			DMBMSG("\t %lu hours elaspesed...\n", hours_cnt);
		}

		if (write_data != value) {
			unsigned int min, sec;
			elapsed_ms = jiffies_to_msecs(diff_jiffies1-diff_jiffies_1st);
			sec = elapsed_ms / 1000;
			min = sec / 60;			
			DMBMSG("END [AGING SPI Memory Test with Single IO]\n");
			DMBMSG("Total minutes: %u\n", min);
		}

		if (put_user(value, &argp->read_data[0]))
			ret = -EFAULT;

		kfree(reg_read_buf);
	#else
		DMBERR("Not SPI interface\n");
	#endif
		break;

	case IOCTL_TEST_REG_ONLY_SPI_MEM_READ:
	#if defined(RTV_IF_SPI)
		if (get_user(read_cnt, &argp->read_cnt)) {
			ret = -EFAULT;
			goto regio_exit;
		}

		if (get_user(write_data, &argp->write_data)) {
			ret = -EFAULT;
			goto regio_exit;
		}

		reg_read_buf = kmalloc(MAX_NUM_MTV_REG_READ_BUF, GFP_KERNEL);
		if (reg_read_buf == NULL) {
			DMBERR("Register buffer allocation error\n");
			ret = -ENOMEM;
			goto regio_exit;
		}

		if (write_data == 0) /* only one-time page selection */
			RTV_REG_MAP_SEL(page);

		RTV_REG_BURST_GET(addr, reg_read_buf, read_cnt);
	
		src_ptr = reg_read_buf;
		dst_ptr = argp->read_data;

		for (i = 0; i< read_cnt; i++, src_ptr++, dst_ptr++) {
			if(put_user(*src_ptr, dst_ptr)) {
				ret = -EFAULT;
				break;
			}
		}
		kfree(reg_read_buf);
	#else
		DMBERR("Not SPI interface\n");
	#endif
		break;

	default:
		break;
	}

regio_exit:
	RTV_GUARD_FREE;

	return 0;
}

static int test_gpio(unsigned long arg, unsigned int cmd)
{
	unsigned int pin, value;
	IOCTL_GPIO_ACCESS_INFO __user *argp = (IOCTL_GPIO_ACCESS_INFO __user *)arg;

	if (get_user(pin, &argp->pin))
		return -EFAULT;

	switch (cmd) {
	case IOCTL_TEST_GPIO_SET:
		if(get_user(value, &argp->value))
			return -EFAULT;

		gpio_set_value(pin, value);
		break;

	case IOCTL_TEST_GPIO_GET:
		value = gpio_get_value(pin);
		if(put_user(value, &argp->value))
			return -EFAULT;
	}

	return 0;
}

static void test_power_on_off(unsigned int cmd)
{
	switch (cmd) {
	case IOCTL_TEST_MTV_POWER_ON:
		DMBMSG("IOCTL_TEST_MTV_POWER_ON\n");

		if (mtv23x_cb_ptr->is_power_on == FALSE) {
			isdbt_control_gpio(true);
			rtvMTV23x_Initialize(RTV_BW_MODE_430KHZ);

			mtv23x_cb_ptr->is_power_on = TRUE;
		}
		break;

	case IOCTL_TEST_MTV_POWER_OFF:	
		if(mtv23x_cb_ptr->is_power_on == TRUE) {
			isdbt_control_gpio(false);
			mtv23x_cb_ptr->is_power_on = FALSE;
		}
		break;	
	}
}


/*==============================================================================
 * TDMB IO control commands(30 ~ 49)
 *============================================================================*/ 
static INLINE int mtv23x_get_signal_qual_info(unsigned long arg)
{
	IOCTL_ISDBT_SIGNAL_QUAL_INFO sig;
	void __user *argp = (void __user *)arg;

	sig.lock_mask = rtvMTV23x_GetLockStatus();	
	sig.rssi = rtvMTV23x_GetRSSI();

	sig.ber_layer_A = rtvMTV23x_GetBER();
	sig.ber_layer_B = rtvMTV23x_GetBER2();

	sig.per_layer_A = rtvMTV23x_GetPER();
	sig.per_layer_B = rtvMTV23x_GetPER2();

	sig.cnr_layer_A = rtvMTV23x_GetCNR_LayerA();
	sig.ant_level_layer_A = rtvMTV23x_GetAntennaLevel_1seg(sig.cnr_layer_A);

	sig.cnr_layer_B = rtvMTV23x_GetCNR_LayerB();
	sig.ant_level_layer_B = rtvMTV23x_GetAntennaLevel(sig.cnr_layer_B);

	if (copy_to_user(argp, &sig, sizeof(IOCTL_ISDBT_SIGNAL_QUAL_INFO)))
		return -EFAULT;

	SHOW_ISDBT_DEBUG_STAT;

	return 0;
}

static INLINE int mtv23x_get_cnr(unsigned long arg)
{
	int cnr = (int)rtvMTV23x_GetCNR();

	if (put_user(cnr, (int *)arg))
		return -EFAULT;

	SHOW_ISDBT_DEBUG_STAT;

	return 0;
}

static INLINE int mtv23x_get_rssi(unsigned long arg)
{
	int rssi = rtvMTV23x_GetRSSI();

	if (put_user(rssi, (int *)arg))
		return -EFAULT;

	SHOW_ISDBT_DEBUG_STAT;

	return 0;
}

static INLINE int mtv23x_get_ber_per_info(unsigned long arg)
{
	IOCTL_ISDBT_BER_PER_INFO info;
	void __user *argp = (void __user *)arg;

	info.ber_layer_A = rtvMTV23x_GetBER();
	info.ber_layer_B = rtvMTV23x_GetBER2();

	info.per_layer_A = rtvMTV23x_GetPER();
	info.per_layer_B = rtvMTV23x_GetPER2();

	if (copy_to_user(argp, &info, sizeof(IOCTL_ISDBT_BER_PER_INFO)))
		return -EFAULT;

	SHOW_ISDBT_DEBUG_STAT;

	return 0;
}

static INLINE void mtv23x_disable_standby_mode(unsigned long arg)
{
	rtvMTV23x_StandbyMode(0);
}

static INLINE void mtv23x_enable_standby_mode(unsigned long arg)
{
	rtvMTV23x_StandbyMode(1);
}

static INLINE int mtv23x_get_lock_status(unsigned long arg)
{
	unsigned int lock_mask = rtvMTV23x_GetLockStatus();

	if (put_user(lock_mask, (unsigned int *)arg))
		return -EFAULT;

	return 0;
}

static INLINE int mtv23x_get_signal_info(unsigned long arg)
{
	IOCTL_ISDBT_SIGNAL_INFO sig;
	void __user *argp = (void __user *)arg;

	sig.lock_mask = rtvMTV23x_GetLockStatus();	
	sig.ber = rtvMTV23x_GetBER();	 
	sig.cnr = rtvMTV23x_GetCNR(); 
	sig.per = rtvMTV23x_GetPER(); 
	sig.rssi = rtvMTV23x_GetRSSI();
	sig.ant_level = rtvMTV23x_GetAntennaLevel(sig.cnr);

	if (copy_to_user(argp, &sig, sizeof(IOCTL_ISDBT_SIGNAL_INFO)))
		return -EFAULT;

	SHOW_ISDBT_DEBUG_STAT;

	return 0;
}

static INLINE int mtv23x_set_channel(unsigned long arg)
{
	int ret = 0;
	unsigned int freq_khz, subch_id, intr_size;
	enum E_RTV_SERVICE_TYPE svc_type;
	enum E_RTV_BANDWIDTH_TYPE bw;
	IOCTL_ISDBT_SET_CH_INFO __user *argp
				= (IOCTL_ISDBT_SET_CH_INFO __user *)arg;

	if (get_user(freq_khz, &argp->freq_khz))
		return -EFAULT;

	if (get_user(subch_id, &argp->subch_id))
		return -EFAULT;

	if (get_user(svc_type, &argp->svc_type))
		return -EFAULT;

	if (get_user(bw, &argp->bandwidth))
		return -EFAULT;

	if (!((1<<svc_type) & ISDBT_VALID_SVC_MASK)) {
		DMBERR("Invaild service type: %d\n", svc_type);
		mtv23x_cb_ptr->cfged_svc = RTV_SERVICE_INVALID;
		return -EINVAL;
	}

	DMBMSG("freq_khz(%u), subch_id(%u), svc_type(%u), bandwidth(%d)\n",
			freq_khz, subch_id, svc_type, bw);

#if defined(RTV_IF_SPI)
	intr_size = mtv23x_cb_ptr->intr_size[svc_type];
#else
	intr_size = 0;
#endif

	mtv23x_cb_ptr->cfged_svc = svc_type;
	mtv23x_cb_ptr->freq_khz = freq_khz;

	ret = rtvMTV23x_SetFrequency(freq_khz, subch_id, svc_type, bw, intr_size);
	if (ret != RTV_SUCCESS) {
		DMBERR("failed: %d\n", ret);
		if (put_user(ret, &argp->tuner_err_code))
			return -EFAULT;

		return -EIO;
	}

	DMBMSG("Leave...\n");

	return 0;
}

static INLINE int mtv23x_scan_channel(unsigned long arg)
{
	int ret;
	unsigned int freq_khz, subch_id, intr_size;
	enum E_RTV_SERVICE_TYPE svc_type;
	enum E_RTV_BANDWIDTH_TYPE bw;
	IOCTL_ISDBT_SCAN_INFO __user *argp
				= (IOCTL_ISDBT_SCAN_INFO __user *)arg;

	if (get_user(freq_khz, &argp->freq_khz))
		return -EFAULT;

	if (get_user(subch_id, &argp->subch_id))
		return -EFAULT;

	if (get_user(svc_type, &argp->svc_type))
		return -EFAULT;

	if (get_user(bw, &argp->bandwidth))
		return -EFAULT;

	if (!((1<<svc_type) & ISDBT_VALID_SVC_MASK)) {
		DMBERR("Invaild service type: %d\n", svc_type);
		mtv23x_cb_ptr->cfged_svc = RTV_SERVICE_INVALID;
		return -EINVAL;
	}

#if defined(RTV_IF_SPI)
	intr_size = mtv23x_cb_ptr->intr_size[svc_type];
#else
	intr_size = 0;
#endif

	mtv23x_cb_ptr->cfged_svc = svc_type;
	mtv23x_cb_ptr->freq_khz = freq_khz;

	ret = rtvMTV23x_ScanFrequency(freq_khz, subch_id, svc_type, bw, intr_size);
	if (ret == RTV_SUCCESS)
		return 0;
	else {
		if(ret != RTV_CHANNEL_NOT_DETECTED)
			DMBERR("Device error: %d\n", ret);

		/* Copy the tuner error-code to application */
		if (put_user(ret, &argp->tuner_err_code))
			return -EFAULT;

		return -EINVAL;
	}
}

static INLINE int mtv23x_disable_ts_out(void)
{
	int ret = 0;

	DMBMSG("Enter\n");

	if (!mtv23x_cb_ptr->tsout_enabled) {
		DMBMSG("Already TS out Disabled\n");
		return 0;
	}

	mtv23x_cb_ptr->tsout_enabled = false;

	rtvMTV23x_DisableStreamOut();

#ifdef _MTV_KERNEL_FILE_DUMP_ENABLE
	mtv_ts_dump_kfile_close();
#endif

	DMBMSG("Leave\n");

	return ret;
}

static INLINE int mtv23x_enable_ts_out(void)
{
	int ret = 0;
#if defined(RTV_IF_SPI) || defined(RTV_IF_CSI656_RAW_8BIT_ENABLE)
	struct TSB_CB_INFO *tsb_cb = &mtv23x_cb_ptr->tsb_cb;
#endif

	//DMBMSG("ENTER\n");

	if (mtv23x_cb_ptr->tsout_enabled) {
		DMBMSG("Already TS out Enabled\n");
		return 0;
	}

	RESET_DEBUG_INTR_STAT;
	RESET_DEBUG_TSPB_STAT;

	if (!((1<<mtv23x_cb_ptr->cfged_svc) & ISDBT_VALID_SVC_MASK)) {
		DMBERR("Invaild configured service type: %d\n",
				mtv23x_cb_ptr->cfged_svc);
		mtv23x_cb_ptr->cfged_svc = RTV_SERVICE_INVALID;
		return -EINVAL;
	}

#if defined(RTV_IF_SPI) || defined(RTV_IF_CSI656_RAW_8BIT_ENABLE)
	/* Setup the tsb_cb stuff to process interrupt. */
	tsb_cb->avail_seg_idx = 0;
	tsb_cb->avail_write_tspb_idx = 0;
	tsb_cb->enqueue_seg_idx = 0;

	if (mtv23x_cb_ptr->intr_size[mtv23x_cb_ptr->cfged_svc]
			!= mtv23x_cb_ptr->cfged_tsp_chunk_size) {
		mtv23x_cb_ptr->cfged_tsp_chunk_size
			= mtv23x_cb_ptr->intr_size[mtv23x_cb_ptr->cfged_svc];

		tsb_cb->num_tsp_per_seg
			= mtv23x_cb_ptr->intr_size[mtv23x_cb_ptr->cfged_svc]
					/ RTV_TSP_XFER_SIZE;

		tsb_cb->num_total_tsp
			= tsb_cb->num_tsp_per_seg * tsb_cb->num_total_seg;
	}

	DMBMSG("svc_type(%d), #tsp_per_seg(%u), #seg(%u), #total_tsp(%u)\n",
		mtv23x_cb_ptr->cfged_svc, tsb_cb->num_tsp_per_seg,
		tsb_cb->num_total_seg, tsb_cb->num_total_tsp);
#endif

#ifdef _MTV_KERNEL_FILE_DUMP_ENABLE
	ret = mtv_ts_dump_kfile_open(mtv23x_cb_ptr->freq_khz);
	if (ret != 0)
		return ret;
#endif

	mtv23x_cb_ptr->tsout_enabled = true;

	rtvMTV23x_EnableStreamOut();

	//DMBMSG("END\n");

	return ret;
}

static long mtv23x_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	mutex_lock(&mtv23x_cb_ptr->ioctl_lock);
	
	switch (cmd) {
	case IOCTL_ISDBT_POWER_ON:
		ret = isdbt_power_on(arg);
		if (ret)
			isdbt_power_off();
		break;

	case IOCTL_ISDBT_POWER_OFF:
		mtv23x_disable_ts_out(); 
		isdbt_power_off();
		break;

	case IOCTL_ISDBT_SCAN_CHANNEL:
		ret = mtv23x_scan_channel(arg);
		break;
			
	case IOCTL_ISDBT_SET_CHANNEL:
		ret = mtv23x_set_channel(arg);		
		break;

	case IOCTL_ISDBT_START_TS:
		ret = mtv23x_enable_ts_out();
		break;

	case IOCTL_ISDBT_STOP_TS:
		mtv23x_disable_ts_out();
		break;			

	case IOCTL_ISDBT_GET_LOCK_STATUS:
		ret = mtv23x_get_lock_status(arg);
		break;

	case IOCTL_ISDBT_GET_SIGNAL_INFO:
		ret = mtv23x_get_signal_info(arg);
		break;

	case IOCTL_ISDBT_SUSPEND:
		mtv23x_enable_standby_mode(arg);
		break;

	case IOCTL_ISDBT_RESUME:
		mtv23x_disable_standby_mode(arg);
		break;

	case IOCTL_ISDBT_GET_BER_PER_INFO:
		ret = mtv23x_get_ber_per_info(arg);
		break;

	case IOCTL_ISDBT_GET_RSSI:
		ret = mtv23x_get_rssi(arg);
		break;

	case IOCTL_ISDBT_GET_CNR:
		ret = mtv23x_get_cnr(arg);
		break;

	case IOCTL_ISDBT_GET_SIGNAL_QUAL_INFO:
		ret = mtv23x_get_signal_qual_info(arg);
		break;

	/* Test IO command */
	case IOCTL_TEST_GPIO_SET:
	case IOCTL_TEST_GPIO_GET:
		ret = test_gpio(arg, cmd);
		break;

	case IOCTL_TEST_MTV_POWER_ON:	
	case IOCTL_TEST_MTV_POWER_OFF:
		test_power_on_off(cmd);
		break;		

	case IOCTL_TEST_REG_SINGLE_READ:
	case IOCTL_TEST_REG_BURST_READ:
	case IOCTL_TEST_REG_WRITE:
	case IOCTL_TEST_REG_SPI_MEM_READ:
	case IOCTL_TEST_REG_ONLY_SPI_MEM_READ:
		ret = test_register_io(arg, cmd);
		break;
	
	default:
		DMBERR("Invalid ioctl command: 0x%X\n", cmd);
		ret = -ENOIOCTLCMD;
		break;
	}

	mutex_unlock(&mtv23x_cb_ptr->ioctl_lock);

	return ret;
}

irqreturn_t mtv23x_irq_handler(int irq, void *param)
{
	U8 *tspb = NULL; /* reset */
	UINT intr_size;
	U8 istatus;

	if (mtv23x_cb_ptr->tsout_enabled == false) {
		return IRQ_HANDLED;
	}

	/*
	만약 처음이면 
#ifdef SCHED_FIFO_USE
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
	sched_setscheduler(current, SCHED_FIFO, &param);
#else
	set_user_nice(current, -20);
#endif	
	*/

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(SPI_CTRL_PAGE);

	intr_size = rtvMTV23x_GetInterruptSize();

	/* Read the register of interrupt status. */
	istatus = RTV_REG_GET(0x10);
	//DMBMSG("$$istatus(0x%02X)\n", istatus);

	if (istatus & SPI_UNDERFLOW_INTR) {
		RTV_REG_SET(0x2A, 1);
		RTV_REG_SET(0x2A, 0);
		DMBMSG("UDF: 0x%02X\n", istatus);
		goto exit_isr;
	}

	if (istatus & (SPI_THRESHOLD_INTR|SPI_OVERFLOW_INTR)) {
		/* Allocate a TS buffer from shared memory. */
		tspb = tsb_get();
		if (tspb) {
			RTV_REG_MAP_SEL(SPI_MEM_PAGE);
			RTV_REG_BURST_GET(0x10, tspb, intr_size); 

#ifdef _MTV_KERNEL_FILE_DUMP_ENABLE
			mtv_ts_dump_kfile_write(tspb, intr_size);
#endif

#if 0
			{
				UINT i;
				const U8 *tspb = (const U8 *)tspb;	
				
				for (i = 0; i < size/188; i++, tsp_buf_ptr += 188) {
					DMBMSG("[%d] 0x%02X 0x%02X 0x%02X 0x%02X | 0x%02X\n",
						i, tsp_buf_ptr[0], tsp_buf_ptr[1],
						tsp_buf_ptr[2], tsp_buf_ptr[3],
						tsp_buf_ptr[187]);
				}
			}
#endif
	
			/* Enqueue */
			tsb_enqueue(tspb);

			DMB_LEVEL_INTR_INC;

			if (istatus & SPI_OVERFLOW_INTR) {
				DMB_OVF_INTR_INC;
				DMBMSG("OVF: 0x%02X\n", istatus);
			}
		} else {
			RTV_REG_SET(0x2A, 1); /* SRAM init */
			RTV_REG_SET(0x2A, 0);
#ifdef DEBUG_TSP_BUF
			mtv23x_cb_ptr->alloc_tspb_err_cnt++;
#endif
			DMBERR("No more TSP buffer from pool.\n");
		}
	} else
		DMBMSG("No data interrupt (0x%02X)\n", istatus);
	
exit_isr:
	RTV_GUARD_FREE;

	return IRQ_HANDLED;
}

static int mtv23x_remove(void)
{
	mutex_lock(&mtv23x_cb_ptr->ioctl_lock);
	mtv23x_power_off();
	mutex_unlock(&mtv23x_cb_ptr->ioctl_lock);
	mutex_destroy(&mtv23x_cb_ptr->ioctl_lock);

	tsb_free_mapping_area();

	if (mtv23x_cb_ptr) {
		kfree(mtv23x_cb_ptr);
		mtv23x_cb_ptr = NULL;
	}

	return 0;
}

static int mtv23x_probe(void)
{
	int ret;

	mtv23x_cb_ptr = kzalloc(sizeof(struct MTV23x_CB), GFP_KERNEL);
	if (!mtv23x_cb_ptr) {
		DMBERR("MTV222 CB allocating error!\n");
		return false;
	}

	ret = tsb_alloc_mapping_area(MAX_TSB_DESC_SIZE, MAX_TSB_SEG_SIZE,
									MAX_NUM_TSB_SEG);
	if (ret) {
		kfree(mtv23x_cb_ptr);
		mtv23x_cb_ptr = NULL;

		return ret;
	}

	mutex_init(&mtv23x_cb_ptr->ioctl_lock);

	return ret;
}

static struct isdbt_drv_func raontech_mtv23x_drv_func = {
	.probe = mtv23x_probe,
	.remove = mtv23x_remove,

	.power_on = mtv23x_power_on,
	.power_off = mtv23x_power_off,

	.mmap = mtv23x_mmap,
	.ioctl = mtv23x_ioctl,

	.irq_handler = mtv23x_irq_handler,
};

struct isdbt_drv_func *mtv23x_drv_func(void)
{
	DMBMSG("isdbt_drv_func : mtv23x\n");
	return &raontech_mtv23x_drv_func;
}

