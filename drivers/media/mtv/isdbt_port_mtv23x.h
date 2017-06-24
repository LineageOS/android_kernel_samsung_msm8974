/*
*
* drivers/media/tdmb/isdbt_port_mtv23x.h
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

#ifndef __ISDBT_PORT_MTV23x_H__
#define __ISDBT_PORT_MTV23x_H__

#include <linux/ioctl.h>
#include "isdbt.h"

#include "mtv23x.h"
#include "mtv23x_internal.h"

#define ISDBT_IOC_MAGIC	'R'

/*
*/
#define DMB_DEBUG_MSG_ENABLE
	
#if defined(RTV_IF_SPI)
	/* Select debug options */
	//#define DEBUG_INTERRUPT
	//#define DEBUG_TSP_BUF
#endif
	
#define DMBERR(fmt, args...) \
		printk(KERN_ERR "MTV23x: %s(): " fmt, __func__, ## args)
	
#ifdef DMB_DEBUG_MSG_ENABLE
	#define DMBMSG(fmt, args...) \
			printk(KERN_INFO "MTV23x: %s(): " fmt, __func__, ## args)
#else 
	#define DMBMSG(x...)  do {} while (0)
#endif

/*############################################################################
# File dump Configuration
	* TS dump filename: /data/local/isdbt_ts_FREQ.ts
############################################################################*/
//#define _MTV_KERNEL_FILE_DUMP_ENABLE

#ifdef _MTV_KERNEL_FILE_DUMP_ENABLE
	extern struct file *mtv_ts_filp;
#endif

#if defined(CONFIG_ISDBT_SPI)
#define MAX_NUM_TSB_SEG		60

/* TS Buffer Descriptor information. */
struct TSB_DESC_INFO {
	/* Flag of operation enabled or not. */
	volatile int op_enabled; /* 0: disabled, 1: enabled */

	/* TSP buffer index which updated when read operation by App. */
	volatile int read_idx;

	/* TSP buffer index which update when write operation by Kernel. */
	volatile int write_idx;

	/* Mapping base address of TS buffer segments.	
	The number of allocating elements was configured by application. */
	unsigned long seg_base[MAX_NUM_TSB_SEG];
};

/* TS Buffer control block. */
struct TSB_CB_INFO {
	/* Index of available tsp segment to be write. */
	int avail_seg_idx;

	/* Index of available tsp buffer to be write. */
	int avail_write_tspb_idx;

	/* Index of tsp segment to be enqueued. */
	int enqueue_seg_idx;

	/* Number of buffering TSPs per segment configured by App. */
	int num_tsp_per_seg;

	/* Number of buffering TSPs in the kernel shared memory
	configured by App. */
	int num_total_tsp;

	/* Number of shared memory segments. */
	int num_total_seg;

	unsigned int desc_size;
	unsigned int seg_size;

	/* The pointer to the address of TSB descriptor
	which shared informations to be allocated. */
	struct TSB_DESC_INFO *tsbd;

	bool seg_bufs_allocated;

	bool mmap_completed;

	/* The pointer to the address of TS buffer segments to be allocated. */
	unsigned char *seg_buf[MAX_NUM_TSB_SEG];
};
#endif /* #if defined(CONFIG_ISDBT_SPI) */

/* ISDBT drvier Control Block */
struct MTV23x_CB {
	int pwr_en_pin_no; /* Pin number of POWER-EN */

#if defined(RTV_IF_TSIF_0) || defined(RTV_IF_TSIF_1) || defined(RTV_IF_SPI_SLAVE)
	#ifdef CONFIG_ISDBT_CAMIF
	struct TSB_CB_INFO tsb_cb;
	unsigned int intr_size[MAX_NUM_RTV_SERVICE]; /* Interrupt size */
	unsigned int cfged_tsp_chunk_size; /* Configured TSP chunk size */
	#endif /* CONFIG_ISDBT_CAMIF */
#endif

#if defined(RTV_IF_SPI) || defined(RTV_IF_SPI_TSIFx)
	#ifndef RTV_IF_SPI_TSIFx
	int irq_pin_no; /* IRQ number */

	struct TSB_CB_INFO tsb_cb;
	unsigned int intr_size[MAX_NUM_RTV_SERVICE]; /* Interrupt size */
	unsigned int cfged_tsp_chunk_size; /* Configured TSP chunk size */
	#endif /* #ifndef RTV_IF_SPI_TSIFx */
#endif


	struct mutex ioctl_lock;

	volatile bool is_power_on;
	unsigned int freq_khz;

	volatile bool tsout_enabled;
	enum E_RTV_SERVICE_TYPE cfged_svc; /* Configured service type */
	
#ifdef DEBUG_INTERRUPT
	unsigned long invalid_intr_cnt;
	unsigned long level_intr_cnt;
	unsigned long ovf_intr_cnt;
	unsigned long udf_intr_cnt;
#endif

#ifdef DEBUG_TSP_BUF
	unsigned int max_alloc_seg_cnt;
	unsigned int max_enqueued_seg_cnt;

	unsigned int max_enqueued_tsp_cnt;
	unsigned long alloc_tspb_err_cnt;
#endif
};

extern struct MTV23x_CB *mtv23x_cb_ptr;

/* The size of TSB descriptor for mmap */
#define MAX_TSB_DESC_SIZE 	PAGE_ALIGN(sizeof(struct TSB_DESC_INFO))

/* The size of TSB segment for mmap */
#define MAX_TSB_SEG_SIZE	PAGE_ALIGN(188 * 16)

#define TOTAL_TSB_MAPPING_SIZE\
	(MAX_TSB_DESC_SIZE + MAX_NUM_TSB_SEG*MAX_TSB_SEG_SIZE)


static inline int mtv_ts_dump_kfile_write(char *buf, size_t len)
{
#ifdef _MTV_KERNEL_FILE_DUMP_ENABLE
	mm_segment_t oldfs;
	struct file *filp;
	int ret = 0;

	if (mtv_ts_filp != NULL) {
		filp = mtv_ts_filp;
		oldfs = get_fs();
		set_fs(KERNEL_DS);
		ret = filp->f_op->write(filp, buf, len, &filp->f_pos);
		set_fs(oldfs);
		if (!ret)
			DMBERR("File write error (%d)\n", ret);
	}

	return ret;
#else
	return 0;
#endif
}

static inline void mtv_ts_dump_kfile_close(void)
{
#ifdef _MTV_KERNEL_FILE_DUMP_ENABLE
	if (mtv_ts_filp != NULL) {
		filp_close(mtv_ts_filp, NULL);
		mtv_ts_filp = NULL;
	}
#endif
}

static inline int mtv_ts_dump_kfile_open(unsigned int channel)
{
#ifdef _MTV_KERNEL_FILE_DUMP_ENABLE
	char fname[32];
	struct file *filp = NULL;

	if (mtv_ts_filp == NULL) {
		sprintf(fname, "/data/local/isdbt_ts_%u.ts", channel);
		filp = filp_open(fname, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR);

		if (IS_ERR(filp)) {
			filp = NULL;
			DMBERR("File open error: %s!\n", fname);
			return PTR_ERR(filp);
		}

		mtv_ts_filp = filp;

		DMBMSG("Kernel dump file opened(%s)\n", fname);
	} else {
		DMBERR("Already TS file opened! Should closed!\n");
		return -1;
	}
#endif

	return 0;
}


#ifdef DEBUG_TSP_BUF
static INLINE void reset_debug_tspb_stat(void)
{
	mtv23x_cb_ptr->max_alloc_seg_cnt = 0;
	mtv23x_cb_ptr->max_enqueued_seg_cnt = 0;
	mtv23x_cb_ptr->max_enqueued_tsp_cnt = 0;
	mtv23x_cb_ptr->alloc_tspb_err_cnt = 0;	
}
#define RESET_DEBUG_TSPB_STAT	reset_debug_tspb_stat()

#else
#define RESET_DEBUG_TSPB_STAT	do {} while (0)
#endif /* #ifdef DEBUG_TSP_BUF*/


#ifdef DEBUG_INTERRUPT
static inline void reset_debug_interrupt_stat(void)
{
	mtv23x_cb_ptr->invalid_intr_cnt = 0;
	mtv23x_cb_ptr->level_intr_cnt = 0;
	mtv23x_cb_ptr->ovf_intr_cnt = 0;
	mtv23x_cb_ptr->udf_intr_cnt = 0;	
}

#define RESET_DEBUG_INTR_STAT	reset_debug_interrupt_stat()
#define DMB_LEVEL_INTR_INC	mtv23x_cb_ptr->level_intr_cnt++;
#define DMB_INV_INTR_INC	mtv23x_cb_ptr->invalid_intr_cnt++;
#define DMB_OVF_INTR_INC	mtv23x_cb_ptr->ovf_intr_cnt++;
#define DMB_UNF_INTR_INC	mtv23x_cb_ptr->udf_intr_cnt++;

#else
#define RESET_DEBUG_INTR_STAT	do {} while (0)
#define DMB_LEVEL_INTR_INC	do {} while (0)
#define DMB_INV_INTR_INC	do {} while (0)
#define DMB_OVF_INTR_INC	do {} while (0)
#define DMB_UNF_INTR_INC	do {} while (0)
#endif /* #ifdef DEBUG_INTERRUPT*/


#if defined(DEBUG_TSP_BUF) && defined(DEBUG_INTERRUPT)
	#define SHOW_ISDBT_DEBUG_STAT	\
	do {	\
		DMBMSG("ovf(%ld), unf(%ld), inv(%ld), level(%ld),\n\
			\t max_alloc_seg(%u), max_enqueued_seg(%u),\n\
			\t max_enqueued_tsp(%u), alloc_err(%ld)\n",\
		mtv23x_cb_ptr->ovf_intr_cnt, mtv23x_cb_ptr->udf_intr_cnt,\
		mtv23x_cb_ptr->invalid_intr_cnt, mtv23x_cb_ptr->level_intr_cnt,\
		mtv23x_cb_ptr->max_alloc_seg_cnt,\
		mtv23x_cb_ptr->max_enqueued_seg_cnt,\
		mtv23x_cb_ptr->max_enqueued_tsp_cnt,\
		mtv23x_cb_ptr->alloc_tspb_err_cnt);\
	} while (0)
#elif !defined(DEBUG_TSP_BUF) && defined(DEBUG_INTERRUPT)
	#define SHOW_ISDBT_DEBUG_STAT	\
	do {	\
		DMBMSG("ovf(%ld), unf(%ld), inv(%ld), level(%ld)\n",\
			mtv23x_cb_ptr->ovf_intr_cnt, mtv23x_cb_ptr->udf_intr_cnt,\
			mtv23x_cb_ptr->invalid_intr_cnt, mtv23x_cb_ptr->level_intr_cnt);\
	} while (0)

#elif defined(DEBUG_TSP_BUF) && !defined(DEBUG_INTERRUPT)
	#define SHOW_ISDBT_DEBUG_STAT	\
	do {	\
		DMBMSG("max_alloc_seg(%u), max_enqueued_seg(%u)	max_enqueued_tsp(%u), alloc_err(%ld)\n",\
			mtv23x_cb_ptr->max_alloc_seg_cnt,\
			mtv23x_cb_ptr->max_enqueued_seg_cnt,\
			mtv23x_cb_ptr->max_enqueued_tsp_cnt,\
			mtv23x_cb_ptr->alloc_tspb_err_cnt);\
	} while (0)

#elif !defined(DEBUG_TSP_BUF) && !defined(DEBUG_INTERRUPT)
	#define SHOW_ISDBT_DEBUG_STAT		do {} while (0)
#endif


/*============================================================================
 * Test IO control commands(0~10)
 *==========================================================================*/
#define IOCTL_TEST_MTV_POWER_ON		_IO(ISDBT_IOC_MAGIC, 0)
#define IOCTL_TEST_MTV_POWER_OFF	_IO(ISDBT_IOC_MAGIC, 1)

#define MAX_NUM_MTV_REG_READ_BUF	(16 * 188)
typedef struct {
	unsigned int page; /* page value */
	unsigned int addr; /* input */

	unsigned int write_data;

	unsigned long param1;
	
	unsigned int read_cnt;
	unsigned char read_data[MAX_NUM_MTV_REG_READ_BUF]; /* output */
} IOCTL_REG_ACCESS_INFO;


#define IOCTL_TEST_REG_SINGLE_READ	_IOWR(ISDBT_IOC_MAGIC, 3, IOCTL_REG_ACCESS_INFO)
#define IOCTL_TEST_REG_BURST_READ	_IOWR(ISDBT_IOC_MAGIC, 4, IOCTL_REG_ACCESS_INFO)
#define IOCTL_TEST_REG_WRITE		_IOW(ISDBT_IOC_MAGIC, 5, IOCTL_REG_ACCESS_INFO)
#define IOCTL_TEST_REG_SPI_MEM_READ	_IOWR(ISDBT_IOC_MAGIC, 6, IOCTL_REG_ACCESS_INFO)
#define IOCTL_TEST_REG_ONLY_SPI_MEM_READ _IOWR(ISDBT_IOC_MAGIC, 7, IOCTL_REG_ACCESS_INFO)

typedef struct {
	unsigned int pin; /* input */
	unsigned int value; /* input for write. output for read.  */
} IOCTL_GPIO_ACCESS_INFO;
#define IOCTL_TEST_GPIO_SET	_IOW(ISDBT_IOC_MAGIC, 6, IOCTL_GPIO_ACCESS_INFO)
#define IOCTL_TEST_GPIO_GET	_IOWR(ISDBT_IOC_MAGIC, 7, IOCTL_GPIO_ACCESS_INFO)


/*============================================================================
* TDMB IO control commands(10 ~ 29)
*===========================================================================*/
typedef struct {
	int country_band_type; // for MTV222
	int bandwidth; // enum E_RTV_BANDWIDTH_TYPE
	unsigned int spi_intr_size[7]; // input
	int tuner_err_code;  // ouput
} IOCTL_ISDBT_POWER_ON_INFO;

typedef struct {
	unsigned int freq_khz; // input
	unsigned int subch_id;  // input
	int svc_type;   // input: enum E_RTV_SERVICE_TYPE
	int bandwidth;   // input: enum E_RTV_BANDWIDTH_TYPE
	int tuner_err_code;  // ouput
} IOCTL_ISDBT_SCAN_INFO;

typedef struct {
	unsigned int freq_khz; // input
	unsigned int subch_id;  // input
	int svc_type; // input: enum E_RTV_SERVICE_TYPE
	int bandwidth; // input: enum E_RTV_BANDWIDTH_TYPE
	int tuner_err_code; // ouput
} IOCTL_ISDBT_SET_CH_INFO;

typedef struct {
	unsigned int 	lock_mask;
	unsigned int	ant_level;
	unsigned int 	ber; // output
	unsigned int 	cnr; // output
	unsigned int 	per; // output
	int 		rssi; // output
} IOCTL_ISDBT_SIGNAL_INFO;

typedef struct {
	unsigned int	lock_mask;
	int				rssi;

	unsigned int	ber_layer_A;
	unsigned int	ber_layer_B;

	unsigned int	per_layer_A;
	unsigned int	per_layer_B;

	unsigned int	cnr_layer_A;
	unsigned int	cnr_layer_B;

	unsigned int	ant_level_layer_A;
	unsigned int	ant_level_layer_B;
} IOCTL_ISDBT_SIGNAL_QUAL_INFO;


#define IOCTL_ISDBT_POWER_ON	_IOWR(ISDBT_IOC_MAGIC, 10, IOCTL_ISDBT_POWER_ON_INFO)
#define IOCTL_ISDBT_POWER_OFF	_IO(ISDBT_IOC_MAGIC, 11)
#define IOCTL_ISDBT_SCAN_CHANNEL _IOWR(ISDBT_IOC_MAGIC,12, IOCTL_ISDBT_SCAN_INFO)
#define IOCTL_ISDBT_SET_CHANNEL	_IOWR(ISDBT_IOC_MAGIC,13, IOCTL_ISDBT_SET_CH_INFO)
#define IOCTL_ISDBT_START_TS	_IO(ISDBT_IOC_MAGIC, 14)
#define IOCTL_ISDBT_STOP_TS	_IO(ISDBT_IOC_MAGIC, 15)
#define IOCTL_ISDBT_GET_LOCK_STATUS _IOR(ISDBT_IOC_MAGIC,16, unsigned int)
#define IOCTL_ISDBT_GET_SIGNAL_INFO _IOR(ISDBT_IOC_MAGIC,17, IOCTL_ISDBT_SIGNAL_INFO)
#define IOCTL_ISDBT_SUSPEND		_IO(ISDBT_IOC_MAGIC, 18)
#define IOCTL_ISDBT_RESUME		_IO(ISDBT_IOC_MAGIC, 19)

typedef struct {
	unsigned int	ber_layer_A;
	unsigned int	ber_layer_B;
	unsigned int	per_layer_A;
	unsigned int	per_layer_B;
} IOCTL_ISDBT_BER_PER_INFO;
#define IOCTL_ISDBT_GET_BER_PER_INFO _IOR(ISDBT_IOC_MAGIC, 20, IOCTL_ISDBT_BER_PER_INFO)
#define IOCTL_ISDBT_GET_RSSI _IOR(ISDBT_IOC_MAGIC, 21, int)
#define IOCTL_ISDBT_GET_CNR _IOR(ISDBT_IOC_MAGIC, 22, int)
#define IOCTL_ISDBT_GET_SIGNAL_QUAL_INFO _IOR(ISDBT_IOC_MAGIC, 23, IOCTL_ISDBT_SIGNAL_QUAL_INFO)



#endif /* __ISDBT_PORT_MTV23x_H__*/




