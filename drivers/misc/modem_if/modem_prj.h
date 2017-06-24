/*
 * Copyright (C) 2012 Samsung Electronics.
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

#ifndef __MODEM_PRJ_H__
#define __MODEM_PRJ_H__

#include <linux/wait.h>
#include <linux/miscdevice.h>
#include <linux/skbuff.h>
#include <linux/wakelock.h>
#include <linux/rbtree.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>

#if 0	//#if defined(CONFIG_LINK_DEVICE_SPI)
#define CONFIG_LINK_DEVICE_SPI_DEBUG
#define CONFIG_LINK_DEVICE_SPI_RFS_DEBUG
#endif

#define MAX_CPINFO_SIZE		512

#define MAX_LINK_DEVTYPE	3
#define MAX_FMT_DEVS	10
#define MAX_RAW_DEVS	32
#define MAX_RFS_DEVS	10
#define MAX_NUM_IO_DEV	(MAX_FMT_DEVS + MAX_RAW_DEVS + MAX_RFS_DEVS)

#define IOCTL_MODEM_ON	_IO('o', 0x19)
#define IOCTL_MODEM_OFF	_IO('o', 0x20)
#define IOCTL_MODEM_RESET	_IO('o', 0x21)
#define IOCTL_MODEM_BOOT_ON	_IO('o', 0x22)
#define IOCTL_MODEM_BOOT_OFF	_IO('o', 0x23)
#define IOCTL_MODEM_BOOT_DONE		_IO('o', 0x24)

#define IOCTL_MODEM_PROTOCOL_SUSPEND	_IO('o', 0x25)
#define IOCTL_MODEM_PROTOCOL_RESUME	_IO('o', 0x26)

#define IOCTL_MODEM_STATUS	_IO('o', 0x27)
#define IOCTL_MODEM_DL_START	_IO('o', 0x28)
#define IOCTL_MODEM_FW_UPDATE	_IO('o', 0x29)
#define IOCTL_MODEM_NET_SUSPEND	_IO('o', 0x30)
#define IOCTL_MODEM_NET_RESUME	_IO('o', 0x31)
#define IOCTL_MODEM_DUMP_START	_IO('o', 0x32)
#define IOCTL_MODEM_DUMP_UPDATE	_IO('o', 0x33)
#define IOCTL_MODEM_FORCE_CRASH_EXIT _IO('o', 0x34)
#define IOCTL_MODEM_CP_UPLOAD _IO('o', 0x35)
#define IOCTL_MODEM_DUMP_RESET _IO('o', 0x36)

#if defined(CONFIG_SEC_DUAL_MODEM_MODE)
#define IOCTL_MODEM_SWITCH_MODEM	_IO('o', 0x37)
#endif

#define IOCTL_DPRAM_SEND_BOOT	_IO('o', 0x40)
#define IOCTL_DPRAM_INIT_STATUS	_IO('o', 0x43)

#define IOCTL_DPRAM_CHECK_CP_BIN	_IO('o', 0xd4)
#define IOCTL_DPRAM_UPDATED_CP_BIN	_IO('o', 0xd5)
/*
 * MAX_RXDATA_SIZE is used at making skb, when it called with page size
 * it need more bytes to allocate itself (Ex, cache byte, shared info,
 * padding...)
 * So, give restriction to allocation size below 1 page to prevent
 * big pages broken.
 */
#define MAX_RXDATA_SIZE	0x0E00	/* 4 * 1024 - 512 */
#define MAX_IPC_TX_SIZE	1024
#define MAX_MTU_TX_DATA_SIZE	1550

#define HDLC_HEADER_MAX_SIZE	6 /* fmt 3, raw 6, rfs 6  or sipc5 */
#define MAX_LINK_PADDING_SIZE	3

#define PSD_DATA_CHID_BEGIN	0x2A
#define PSD_DATA_CHID_END	0x38
#define IP6VERSION	6
#define SOURCE_MAC_ADDR	{0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}

#define FMT_WAKE_TIME	(HZ/2)
#define RFS_WAKE_TIME	(HZ*3)
#define RAW_WAKE_TIME	(HZ*6)

#define CP_LOOPBACK_CHANNEL	30


/* Does modem ctl structure will use state ? or status defined below ?*/
/* Be careful!! below sequence shouldn't be changed*/
enum modem_state {
	STATE_OFFLINE,
	STATE_CRASH_RESET, /* silent reset */
	STATE_CRASH_EXIT, /* cp ramdump */
	STATE_BOOTING,
	STATE_ONLINE,
	STATE_NV_REBUILDING, /* <= rebuilding start */
	STATE_LOADER_DONE,
	STATE_SIM_ATTACH,
	STATE_SIM_DETACH,
#if defined(CONFIG_SEC_DUAL_MODEM_MODE)
	STATE_MODEM_SWITCH,
#endif
};

enum com_state {
	COM_NONE,
	COM_ONLINE,
	COM_HANDSHAKE,
	COM_BOOT,
	COM_CRASH,
	COM_BOOT_EBL,
};

enum link_mode {
	LINK_MODE_OFFLINE = 0,
	LINK_MODE_BOOT,
	LINK_MODE_IPC,
	LINK_MODE_DLOAD,
	LINK_MODE_ULOAD,
};


struct sim_state {
	bool online; /* SIM is online? */
	bool changed; /* online is changed? */
};

struct header_data {
	char hdr[HDLC_HEADER_MAX_SIZE];
	unsigned len;
	unsigned frag_len;
	char start; /* hdlc start header 0x7F or 0b11111000 */
};

struct fmt_hdr {
	u16 len;
	u8 control;
} __packed;

struct raw_hdr {
	u32 len;
	u8 channel;
	u8 control;
} __packed;

struct rfs_hdr {
	u32 len;
	u8 cmd;
	u8 id;
} __packed;

struct sipc_fmt_hdr {
	u16 len;
	u8  msg_seq;
	u8  ack_seq;
	u8  main_cmd;
	u8  sub_cmd;
	u8  cmd_type;
} __packed;

struct vnet {
	struct io_device *iod;
};

/* for fragmented data from link devices */
struct fragmented_data {
	struct sk_buff *skb_recv;
	struct header_data h_data;

	/* page alloc fail retry*/
	unsigned realloc_offset;
};
#define fragdata(iod, ld) (&(iod)->fragments[(ld)->link_type])

/** struct skbuff_priv - private data of struct sk_buff
 * this is matched to char cb[48] of struct sk_buff
 */
struct skbuff_private {
	struct io_device *iod;
	struct link_device *ld;
	struct io_device *real_iod; /* for rx multipdp */
};

static inline struct skbuff_private *skbpriv(struct sk_buff *skb)
{
	BUILD_BUG_ON(sizeof(struct skbuff_private) > sizeof(skb->cb));
	return (struct skbuff_private *)&skb->cb;
}

struct io_device {
	/* rb_tree node for an io device */
	struct rb_node node_chan;
	struct rb_node node_fmt;

	/* Name of the IO device */
	char *name;

	atomic_t opened;

	/* Wait queue for the IO device */
	wait_queue_head_t wq;

	/* Misc and net device structures for the IO device */
	struct miscdevice miscdev;
	struct net_device *ndev;

	/* ID and Format for channel on the link */
	unsigned id;
	enum modem_link link_types;
	enum dev_format format;
	enum modem_io io_typ;
	enum modem_network net_typ;

	bool use_handover;	/* handover 2+ link devices */

	/* SIPC version */
	enum sipc_ver ipc_version;

	/* Rx queue of sk_buff */
	struct sk_buff_head sk_rx_q;

	struct fragmented_data fragments[LINKDEV_MAX];

	/* for multi-frame */
	struct sk_buff *skb[128];

	/* called from linkdevice when a packet arrives for this iodevice */
	int (*recv)(struct io_device *iod, struct link_device *ld,
					const char *data, unsigned int len);

	/* inform the IO device that the modem is now online or offline or
	 * crashing or whatever...
	 */
	void (*modem_state_changed)(struct io_device *iod, enum modem_state);

	/* inform the IO device that the SIM is not inserting or removing */
	void (*sim_state_changed)(struct io_device *iod, bool sim_online);

	struct modem_ctl *mc;
	struct modem_shared *msd;

	struct wake_lock wakelock;
	long waketime;

	/* DO NOT use __current_link directly
	 * you MUST use skbpriv(skb)->ld in mc, link, etc..
	 */
	struct link_device *__current_link;
};
#define to_io_device(misc) container_of(misc, struct io_device, miscdev)

/* get_current_link, set_current_link don't need to use locks.
 * In ARM, set_current_link and get_current_link are compiled to
 * each one instruction (str, ldr) as atomic_set, atomic_read.
 * And, the order of set_current_link and get_current_link is not important.
 */
#define get_current_link(iod) ((iod)->__current_link)
#define set_current_link(iod, ld) ((iod)->__current_link = (ld))

struct link_device {
	struct list_head  list;
	char *name;

	enum modem_link link_type;
	unsigned aligned;

	/* Modem control */
	struct modem_ctl *mc;

	/* Modem shared data */
	struct modem_shared *msd;

	/* Operation mode of the link device */
	enum link_mode mode;

	struct io_device *fmt_iods[4];

	/* TX queue of socket buffers */
	struct sk_buff_head sk_fmt_tx_q;
	struct sk_buff_head sk_raw_tx_q;
	struct sk_buff_head sk_rfs_tx_q;

	bool raw_tx_suspended; /* for misc dev */
	struct completion raw_tx_resumed_by_cp;

	struct workqueue_struct *tx_wq;
	struct workqueue_struct *tx_raw_wq;
	struct work_struct tx_work;
	struct delayed_work tx_delayed_work;

	enum com_state com_state;

	/* init communication - setting link driver */
	int (*init_comm)(struct link_device *ld, struct io_device *iod);

	/* terminate communication */
	void (*terminate_comm)(struct link_device *ld, struct io_device *iod);

	/* called by an io_device when it has a packet to send over link
	 * - the io device is passed so the link device can look at id and
	 *   format fields to determine how to route/format the packet
	 */
	int (*send)(struct link_device *ld, struct io_device *iod,
				struct sk_buff *skb);

	//SPI_SETUP
	int (*ioctl)(struct link_device *ld, struct io_device *iod,
			unsigned cmd, unsigned long _arg);
};

/** rx_alloc_skb - allocate an skbuff and set skb's iod, ld
 * @length:	length to allocate
 * @iod:	struct io_device *
 * @ld:		struct link_device *
 *
 * %NULL is returned if there is no free memory.
 */
static inline struct sk_buff *rx_alloc_skb(unsigned int length,
		struct io_device *iod, struct link_device *ld)
{
	struct sk_buff *skb;

	if (iod->format == IPC_MULTI_RAW || iod->format == IPC_RAW)
		skb = dev_alloc_skb(length);
	else
		skb = alloc_skb(length, GFP_KERNEL);

	if (likely(skb)) {
		skbpriv(skb)->iod = iod;
		skbpriv(skb)->ld = ld;
	}
	return skb;
}

struct modemctl_ops {
	int (*modem_on) (struct modem_ctl *);
	int (*modem_off) (struct modem_ctl *);
	int (*modem_reset) (struct modem_ctl *);
	int (*modem_boot_on) (struct modem_ctl *);
	int (*modem_boot_off) (struct modem_ctl *);
	int (*modem_force_crash_exit) (struct modem_ctl *);
	int (*modem_dump_reset) (struct modem_ctl *);
};

/* for IPC Logger */
struct mif_storage {
	char *addr;
	unsigned int cnt;
};

/* modem_shared - shared data for all io/link devices and a modem ctl
 * msd : mc : iod : ld = 1 : 1 : M : N
 */
struct modem_shared {
	/* list of link devices */
	struct list_head link_dev_list;

	/* rb_tree root of io devices. */
	struct rb_root iodevs_tree_chan; /* group by channel */
	struct rb_root iodevs_tree_fmt; /* group by dev_format */

	/* for IPC Logger */
	struct mif_storage storage;
	spinlock_t lock;
};

struct modem_ctl {
	struct device *dev;
	char *name;
	struct modem_data *mdm_data;

	struct modem_shared *msd;

	enum modem_state phone_state;
	struct sim_state sim_state;

	unsigned gpio_cp_on;
	unsigned gpio_cp_off;
//temp disable bringup DKLee	unsigned gpio_reset_req_n;
	unsigned gpio_pda_active;
	unsigned gpio_phone_active;
	unsigned gpio_cp_dump_int;
	unsigned gpio_ap_dump_int;
	unsigned gpio_sim_detect;
	unsigned gpio_cp_pwr_check;
	unsigned gpio_dynamic_switching;

#ifdef CONFIG_GSM_MODEM_SPRD6500
	unsigned gpio_ap_cp_int1;
	unsigned gpio_ap_cp_int2;
	unsigned gpio_uart_sel;
#endif

#ifdef CONFIG_SEC_DUAL_MODEM_MODE
	unsigned gpio_sim_sel;
//temp disable bringup DKLee	unsigned gpio_cp_ctrl1;
#endif

	

	int irq_phone_active;
	int irq_cp_dump_int;
	int irq_sim_detect;

	struct modemctl_ops ops;
	struct io_device *iod;
	struct io_device *bootd;

};

int sipc4_init_io_device(struct io_device *iod);
int sipc5_init_io_device(struct io_device *iod);

#if defined(CONFIG_GSM_MODEM_SPRD6500)
extern int spi_sema_init(void);
extern int sprd_boot_done;
extern void if_spi_thread_restart(void);
#endif
#endif
