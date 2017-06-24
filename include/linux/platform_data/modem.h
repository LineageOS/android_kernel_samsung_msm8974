/*
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
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

#ifndef __MODEM_IF_H__
#define __MODEM_IF_H__

enum modem_t {
	IMC_XMM6260,
	IMC_XMM6262,
	VIA_CBP71,
	VIA_CBP72,
	SEC_CMC221,
	QC_MDM6600,
	QC_ESC6270,
	QC_MSM8x55,
	QC_MSM7x27,
	SPRD_SC6500,
	DUMMY,
	MAX_MODEM_TYPE
};

enum dev_format {
	IPC_FMT,
	IPC_RAW,
	IPC_RFS,
	IPC_CMD,
	IPC_BOOT,
	IPC_BOOT_2,
	IPC_MULTI_RAW,
	IPC_RAMDUMP,
	MAX_DEV_FORMAT,
};

enum modem_io {
	IODEV_MISC,
	IODEV_NET,
	IODEV_DUMMY,
};

enum modem_link {
	LINKDEV_UNDEFINED,
	LINKDEV_MIPI,
	LINKDEV_DPRAM,
	LINKDEV_SPI,
	LINKDEV_USB,
	LINKDEV_HSIC,
	LINKDEV_C2C,
	LINKDEV_MAX,
};
#define LINKTYPE(modem_link) (1u << (modem_link))

enum modem_network {
	UMTS_NETWORK,
	CDMA_NETWORK,
	LTE_NETWORK,
	TDSCDMA_NETWORK,
};

enum sipc_ver {
	NO_SIPC_VER = 0,
	SIPC_VER_40 = 40,
	SIPC_VER_41 = 41,
	SIPC_VER_42 = 42,
	SIPC_VER_50 = 50,
	MAX_SIPC_VER,
};

enum sipc_dev_type {
	IPC_FMT_DEV = 0,
	IPC_RAW_DEV,
	IPC_RFS_DEV,
	MAX_IPC_DEV,
};

/**
 * struct modem_io_t - declaration for io_device
 * @name:	device name
 * @id:		contain format & channel information
 *		(id & 11100000b)>>5 = format  (eg, 0=FMT, 1=RAW, 2=RFS)
 *		(id & 00011111b)    = channel (valid only if format is RAW)
 * @format:	device format
 * @io_type:	type of this io_device
 * @links:	list of link_devices to use this io_device
 *		for example, if you want to use DPRAM and USB in an io_device.
 *		.links = LINKTYPE(LINKDEV_DPRAM) | LINKTYPE(LINKDEV_USB)
 * @tx_link:	when you use 2+ link_devices, set the link for TX.
 *		If define multiple link_devices in @links,
 *		you can receive data from them. But, cannot send data to all.
 *		TX is only one link_device.
 *
 * This structure is used in board-*-modem.c
 */
struct modem_io_t {
	char *name;
	int id;
	enum dev_format format;
	enum modem_io io_type;
	enum modem_link links;
	enum modem_link tx_link;
};

/* platform data */
struct modem_data {
	char *name;

	unsigned gpio_cp_on;
	unsigned gpio_cp_off;
	unsigned gpio_reset_req_n;
	unsigned gpio_pda_active;
	unsigned gpio_phone_active;
	unsigned gpio_cp_dump_int;
	unsigned gpio_ap_dump_int;
	unsigned gpio_sim_detect;

#ifdef CONFIG_GSM_MODEM_SPRD6500
	unsigned gpio_ap_cp_int1;
	unsigned gpio_ap_cp_int2;
	unsigned gpio_uart_sel;

	unsigned gpio_ipc_mrdy;
	unsigned gpio_ipc_srdy;
	unsigned gpio_ipc_sub_mrdy;
	unsigned gpio_ipc_sub_srdy;
#endif

#ifdef CONFIG_SEC_DUAL_MODEM_MODE
		unsigned gpio_sim_sel;
#endif


	/* Switch with 2 links in a modem */
	unsigned gpio_dynamic_switching;

	/* modem component */
	enum modem_network  modem_net;
	enum modem_t modem_type;
	enum modem_link link_types;
	char *link_name;

	/* SIPC version */
	enum sipc_ver ipc_version;

	/* Information of IO devices */
	unsigned num_iodevs;
	struct modem_io_t *iodevs;

	/* Handover with 2+ link devices */
	bool use_handover;
};

#define LOG_TAG "mif: "

#define mif_err(fmt, ...) \
	pr_err(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_debug(fmt, ...) \
	pr_debug(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_info(fmt, ...) \
	pr_info(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_trace(fmt, ...) \
	printk(KERN_DEBUG "mif: %s: %d: called(%pF): " fmt, \
		__func__, __LINE__, __builtin_return_address(0), ##__VA_ARGS__)

#endif
