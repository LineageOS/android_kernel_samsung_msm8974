/* /linux/drivers/misc/modem_if_v2/sipc5_io_device.c
 *
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

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/etherdevice.h>
#include <linux/ratelimit.h>
#include <linux/device.h>

#include <linux/platform_data/modem_v2.h>
#include "modem_prj.h"
#include "modem_utils.h"


#define SIPC5_CFG_MASK	0b11111000
#define SIPC5_SIZE_OF_CFG	1
#define SIPC5_CFG_PADDING_MASK	0b00000100
#define SIPC5_CFG_EXT_FIELD_MASK	0b00000010
#define SIPC5_CFG_CTL_FIELD_MASK	0b00000001

#define SIPC5_MIN_SIZE_OF_HEADER	3 /* Ch ID: 1B, Len: 2B */
#define SIPC5_MAX_SIZE_OF_HEADER	4 /* + Ex Field(Cont or Ex Len): 1B */

struct sipc5_hdr {
	u8 ch_id;
	u16 len;
	u8 ext_field;
} __packed;


static ssize_t show_waketime(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int msec;
	char *p = buf;
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct io_device *iod = container_of(miscdev, struct io_device,
			miscdev);

	msec = jiffies_to_msecs(iod->waketime);

	p += sprintf(buf, "raw waketime : %ums\n", msec);

	return p - buf;
}

static ssize_t store_waketime(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long msec;
	int ret;
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct io_device *iod = container_of(miscdev, struct io_device,
			miscdev);

	ret = strict_strtoul(buf, 10, &msec);
	if (ret)
		return count;

	iod->waketime = msecs_to_jiffies(msec);

	return count;
}

static struct device_attribute attr_waketime =
	__ATTR(waketime, S_IRUGO | S_IWUSR, show_waketime, store_waketime);

static int get_header_size(char cfg)
{
	if (cfg & SIPC5_CFG_EXT_FIELD_MASK)
		return SIPC5_MAX_SIZE_OF_HEADER;
	else
		return SIPC5_MIN_SIZE_OF_HEADER;
}

static int get_hdlc_size(char *buf)
{
	struct sipc5_hdr *sipc5_header;

	sipc5_header = (struct sipc5_hdr *)buf;
	return sipc5_header->len;
}

static u8 get_cfg_field(struct io_device *iod, struct link_device *ld,
			ssize_t count)
{
	u8 config = SIPC5_CFG_MASK;

	if (ld->aligned)
		config |= SIPC5_CFG_PADDING_MASK;

	switch (iod->format) {
	case IPC_FMT:
		if (count > MAX_IPC_TX_SIZE)
			config |= SIPC5_CFG_EXT_FIELD_MASK |
					SIPC5_CFG_CTL_FIELD_MASK;
		return config;

	case IPC_RAW:
	case IPC_MULTI_RAW:
		if (count > MAX_MTU_TX_DATA_SIZE)
			config |= SIPC5_CFG_EXT_FIELD_MASK |
					SIPC5_CFG_CTL_FIELD_MASK;
		return config;

	case IPC_RFS:
	case IPC_CMD:
	case IPC_BOOT:
	case IPC_RAMDUMP:
	default:
		if (count > 0xFFFF)
			config |= SIPC5_CFG_EXT_FIELD_MASK;
		return config;
	}
}

static void *get_header(struct io_device *iod, size_t count,
			char *frame_header_buf, u8 cfg)
{
	struct sipc5_hdr *sipc5_h = (struct sipc5_hdr *)frame_header_buf;

	switch (iod->format) {
	case IPC_FMT:
		sipc5_h->ch_id = iod->id;
		sipc5_h->len = (u16)count + get_header_size(cfg)
					+ SIPC5_SIZE_OF_CFG;

		/* ToDo - Add handling for over size data */
		sipc5_h->ext_field = 0;

		return (void *)frame_header_buf;

	case IPC_RAW:
	case IPC_MULTI_RAW:
		sipc5_h->ch_id = iod->id & 0x1F;
		sipc5_h->len = (u16)count + get_header_size(cfg)
					+ SIPC5_SIZE_OF_CFG;

		/* ToDo - Add handling for over size data */
		sipc5_h->ext_field = 0;

		return (void *)frame_header_buf;

	case IPC_RFS:
		sipc5_h->ch_id = iod->id;
		sipc5_h->len = (u16)count + get_header_size(cfg)
					+ SIPC5_SIZE_OF_CFG;

		/* ToDo - Add handling for over size data */
		sipc5_h->ext_field = 0;

		return (void *)frame_header_buf;

	case IPC_CMD:
	case IPC_BOOT:
	case IPC_RAMDUMP:
	default:
		return NULL;
	}
}

static inline int calc_padding_size(struct link_device *ld, unsigned len)
{
	if (ld->aligned)
		return (4 - (len & 0x3)) & 0x3;
	else
		return 0;
}

static inline int rx_hdlc_head_start_check(char *buf)
{
	/* check hdlc head and return size of start byte */
	return ((buf[0] & SIPC5_CFG_MASK) == SIPC5_CFG_MASK) ?
				SIPC5_SIZE_OF_CFG : -EBADMSG;
}

/* remove hdlc header and store IPC header */
static int rx_hdlc_head_check(struct io_device *iod, struct link_device *ld,
						char *buf, unsigned rest)
{
	struct header_data *hdr = &fragdata(iod, ld)->h_data;
	int head_size;
	int done_len = 0;
	int len = 0;

	/* first frame, remove start header 7F */
	if (!hdr->start) {
		len = rx_hdlc_head_start_check(buf);
		if (len < 0) {
			mif_err("Wrong HDLC start: 0x%x(%s)\n",
						*buf, iod->name);
			return len; /*Wrong hdlc start*/
		}
		mif_debug("check len : %d, rest : %d (%d)\n", len,
					rest, __LINE__);

		/* set the start flag of current packet */
		hdr->start = buf[0]; /* config field for sipc5 */
		hdr->len = 0;

		buf += len;
		done_len += len;
		rest -= len; /* rest, call by value */
	}
	mif_debug("check len : %d, rest : %d (%d)\n", len, rest,
				__LINE__);

	/* get header size without config field size */
	head_size = get_header_size(hdr->start);

	/* store the HDLC header to iod priv */
	if (hdr->len < head_size) {
		len = min(rest, head_size - hdr->len);
		memcpy(hdr->hdr + hdr->len, buf, len);
		hdr->len += len;
		done_len += len;
	}

	mif_debug("check done_len : %d, rest : %d (%d)\n", done_len,
				rest, __LINE__);
	return done_len;
}

static int rx_iodev_skb(struct sk_buff *skb);
static int rx_hdlc_data_check(struct io_device *iod, struct link_device *ld,
						char *buf, unsigned rest)
{
	struct header_data *hdr = &fragdata(iod, ld)->h_data;
	struct sk_buff *skb = fragdata(iod, ld)->skb_recv;
	int head_size = get_header_size(hdr->start);
	int data_size = get_hdlc_size(hdr->hdr) - head_size - SIPC5_SIZE_OF_CFG;
	int alloc_size;
	int len = 0;
	int done_len = 0;
	int rest_len = data_size - hdr->frag_len;
	int continue_len = fragdata(iod, ld)->realloc_offset;

	mif_debug("head_size : %d, data_size : %d (%d)\n", head_size,
				data_size, __LINE__);

	if (continue_len) {
		/* check the HDLC header*/
		if (rx_hdlc_head_start_check(buf) == SIPC5_SIZE_OF_CFG) {
			head_size = get_header_size(buf[0]);
			rest_len -= (head_size + SIPC5_SIZE_OF_CFG);
			continue_len += (head_size + SIPC5_SIZE_OF_CFG);
		}

		buf += continue_len;
		rest -= continue_len;
		done_len += continue_len;
		fragdata(iod, ld)->realloc_offset = 0;

		mif_debug("realloc_offset = %d\n", continue_len);
	}

	/* first payload data - alloc skb */
	if (!skb) {
		/* make skb data size under MAX_RXDATA_SIZE */
		alloc_size = min(data_size, MAX_RXDATA_SIZE);
		alloc_size = min(alloc_size, rest_len);

		/* exceptional case for RFS channel
		 * make skb for header info first
		 */
		if (iod->format == IPC_RFS && !hdr->frag_len) {
			skb = rx_alloc_skb(head_size, iod, ld);
			if (unlikely(!skb))
				return -ENOMEM;
			memcpy(skb_put(skb, head_size), hdr->hdr, head_size);
			rx_iodev_skb(skb);
		}

		/* allocate first packet for data, when its size exceed
		 * MAX_RXDATA_SIZE, this packet will split to
		 * multiple packets
		 */
		skb = rx_alloc_skb(alloc_size, iod, ld);
		if (unlikely(!skb)) {
			fragdata(iod, ld)->realloc_offset = continue_len;
			return -ENOMEM;
		}
		fragdata(iod, ld)->skb_recv = skb;
	}

	while (rest) {
		/* copy length cannot exceed rest_len */
		len = min_t(int, rest_len, rest);
		/* copy length should be under skb tailroom size */
		len = min(len, skb_tailroom(skb));
		/* when skb tailroom is bigger than MAX_RXDATA_SIZE
		 * restrict its size to MAX_RXDATA_SIZE just for convinience */
		len = min(len, MAX_RXDATA_SIZE);

		/* copy bytes to skb */
		memcpy(skb_put(skb, len), buf, len);

		/* adjusting variables */
		buf += len;
		rest -= len;
		done_len += len;
		rest_len -= len;
		hdr->frag_len += len;

		/* check if it is final for this packet sequence */
		if (!rest_len || !rest)
			break;

		/* more bytes are remain for this packet sequence
		 * pass fully loaded skb to rx queue
		 * and allocate another skb for continues data recv chain
		 */
		rx_iodev_skb(skb);
		fragdata(iod, ld)->skb_recv =  NULL;

		alloc_size = min(rest_len, MAX_RXDATA_SIZE);

		skb = rx_alloc_skb(alloc_size, iod, ld);
		if (unlikely(!skb)) {
			fragdata(iod, ld)->realloc_offset = done_len;
			return -ENOMEM;
		}
		fragdata(iod, ld)->skb_recv = skb;
	}
	mif_debug("rest : %d, alloc_size : %d , len : %d (%d)\n",
				rest, alloc_size, skb->len, __LINE__);

	return done_len;
}

static int rx_multipdp(struct sk_buff *skb)
{
	int err = 0;
	struct io_device *iod = skbpriv(skb)->real_iod;
	struct net_device *ndev = NULL;
	struct iphdr *ip_header = NULL;
	struct ethhdr *ehdr = NULL;
	const char source[ETH_ALEN] = SOURCE_MAC_ADDR;

	switch (iod->io_typ) {
	case IODEV_MISC:
		mif_debug("<%s> sk_rx_q.qlen = %d\n",
			iod->name, iod->sk_rx_q.qlen);
		skb_queue_tail(&iod->sk_rx_q, skb);
		wake_up(&iod->wq);
		return 0;

	case IODEV_NET:
		ndev = iod->ndev;
		if (!ndev)
			return NET_RX_DROP;

		skb->dev = ndev;
		ndev->stats.rx_packets++;
		ndev->stats.rx_bytes += skb->len;

		/* check the version of IP */
		ip_header = (struct iphdr *)skb->data;
		if (ip_header->version == IP6VERSION)
			skb->protocol = htons(ETH_P_IPV6);
		else
			skb->protocol = htons(ETH_P_IP);

		if (iod->use_handover) {
			skb_push(skb, sizeof(struct ethhdr));
			ehdr = (void *)skb->data;
			memcpy(ehdr->h_dest, ndev->dev_addr, ETH_ALEN);
			memcpy(ehdr->h_source, source, ETH_ALEN);
			ehdr->h_proto = skb->protocol;
			skb->ip_summed = CHECKSUM_UNNECESSARY;
			skb_reset_mac_header(skb);

			skb_pull(skb, sizeof(struct ethhdr));
		}

		if (in_irq())
			err = netif_rx(skb);
		else
			err = netif_rx_ni(skb);

		if (err != NET_RX_SUCCESS)
			mif_err("ERR: <%s> netif_rx fail (err %d)\n",
				iod->name, err);

		return err;

	default:
		mif_err("wrong io_type : %d\n", iod->io_typ);
		return -EINVAL;
	}
}

/* de-mux function draft */
static int rx_iodev_skb(struct sk_buff *skb)
{
	u8 ch;
	struct io_device *iod = skbpriv(skb)->iod;
	struct io_device *real_iod = NULL;
	struct link_device *ld = skbpriv(skb)->ld;
	struct sipc5_hdr *sipc5_header;

	switch (iod->format) {
	case IPC_RAW:
	case IPC_MULTI_RAW:
		sipc5_header =
			(struct sipc5_hdr *)fragdata(iod, ld)->h_data.hdr;
		ch = sipc5_header->ch_id;

		real_iod = link_get_iod_with_channel(ld, 0x20 | ch);
		if (!real_iod) {
			mif_err("wrong channel %d\n", ch);
			return -1;
		}
		skbpriv(skb)->real_iod = real_iod;

		return rx_multipdp(skb);

	case IPC_CMD:
	case IPC_FMT:
	case IPC_RFS:
	case IPC_BOOT:
	case IPC_RAMDUMP:
	default:
		skb_queue_tail(&iod->sk_rx_q, skb);
		mif_debug("wake up wq of %s\n", iod->name);
		wake_up(&iod->wq);
		return 0;
	}
}

static int rx_hdlc_packet(struct io_device *iod, struct link_device *ld,
		const char *data, unsigned recv_size)
{
	int rest = (int)recv_size;
	char *buf = (char *)data;
	int err = 0;
	int len = 0;
	unsigned rcvd = 0;

	if (rest <= 0)
		goto exit;

	mif_debug("RX_SIZE = %d, ld: %s\n", rest, ld->name);

	if (fragdata(iod, ld)->h_data.frag_len) {
		/*
		  If the fragdata(iod, ld)->h_data.frag_len field is
		  not zero, there is a HDLC frame that is waiting for more data
		  or HDLC_END in the skb (fragdata(iod, ld)->skb_recv).
		  In this case, rx_hdlc_head_check() must be skipped.
		*/
		goto data_check;
	}

next_frame:
	err = len = rx_hdlc_head_check(iod, ld, buf, rest);
	if (err < 0)
		goto exit;
	mif_debug("check len : %d, rest : %d (%d)\n", len, rest,
				__LINE__);

	buf += len;
	rest -= len;
	if (rest <= 0)
		goto exit;

data_check:
	/*
	  If the return value of rx_hdlc_data_check() is zero, there remains
	  only HDLC_END that will be received.
	*/
	err = len = rx_hdlc_data_check(iod, ld, buf, rest);
	if (err < 0)
		goto exit;
	mif_debug("check len : %d, rest : %d (%d)\n", len, rest,
				__LINE__);

	buf += len;
	rest -= len;

	if (!rest && fragdata(iod, ld)->h_data.frag_len) {
		/*
		  Data is being received and more data or HDLC_END does not
		  arrive yet, but there is no more data in the buffer. More
		  data may come within the next frame from the link device.
		*/
		return 0;
	} else if (rest <= 0)
		goto exit;

	/* At this point, one complete HDLC frame has been received. */

	/*
	  The padding size is applied for the next HDLC frame. Zero will be
	  returned by calc_padding_size() if the link device does not require
	  4-byte aligned access.
	*/
	rcvd = get_hdlc_size(fragdata(iod, ld)->h_data.hdr) +
				SIPC5_SIZE_OF_CFG;
	len = calc_padding_size(ld, rcvd);
	buf += len;
	rest -= len;
	if (rest < 0)
		goto exit;

	err = rx_iodev_skb(fragdata(iod, ld)->skb_recv);
	if (err < 0)
		goto exit;

	/* initialize header & skb */
	fragdata(iod, ld)->skb_recv = NULL;
	memset(&fragdata(iod, ld)->h_data, 0x00,
			sizeof(struct header_data));
	fragdata(iod, ld)->realloc_offset = 0;

	if (rest)
		goto next_frame;

exit:
	if (rest < 0)
		err = -ERANGE;

	if (err == -ENOMEM) {
		if (!(fragdata(iod, ld)->h_data.frag_len))
			memset(&fragdata(iod, ld)->h_data, 0x00,
				sizeof(struct header_data));
		return err;
	}

	if (err < 0 && fragdata(iod, ld)->skb_recv) {
		dev_kfree_skb_any(fragdata(iod, ld)->skb_recv);
		fragdata(iod, ld)->skb_recv = NULL;

		/* clear headers */
		memset(&fragdata(iod, ld)->h_data, 0x00,
				sizeof(struct header_data));
		fragdata(iod, ld)->realloc_offset = 0;
	}

	return err;
}

/* called from link device when a packet arrives for this io device */
static int io_dev_recv_data_from_link_dev(struct io_device *iod,
		struct link_device *ld, const char *data, unsigned int len)
{
	struct sk_buff *skb;
	int err;

	switch (iod->format) {
	case IPC_RFS:
	case IPC_FMT:
	case IPC_RAW:
	case IPC_MULTI_RAW:
		if (iod->waketime)
			wake_lock_timeout(&iod->wakelock, iod->waketime);
		err = rx_hdlc_packet(iod, ld, data, len);
		if (err < 0)
			mif_err("fail process HDLC frame\n");
		return err;

	case IPC_CMD:
	case IPC_BOOT:
	case IPC_RAMDUMP:
		/* save packet to sk_buff */
		skb = rx_alloc_skb(len, iod, ld);
		if (!skb) {
			mif_err("fail alloc skb (%d)\n", __LINE__);
			return -ENOMEM;
		}

		mif_debug("boot len : %d\n", len);

		memcpy(skb_put(skb, len), data, len);
		skb_queue_tail(&iod->sk_rx_q, skb);
		mif_debug("skb len : %d\n", skb->len);

		wake_up(&iod->wq);
		return len;

	default:
		return -EINVAL;
	}
}

/* inform the IO device that the modem is now online or offline or
 * crashing or whatever...
 */
static void io_dev_modem_state_changed(struct io_device *iod,
			enum modem_state state)
{
	iod->mc->phone_state = state;
	mif_err("modem state changed. (iod: %s, state: %d)\n",
		iod->name, state);

	if ((state == STATE_CRASH_RESET) || (state == STATE_CRASH_EXIT)
		|| (state == STATE_NV_REBUILDING))
		wake_up(&iod->wq);
}

/**
 * io_dev_sim_state_changed
 * @iod: IPC's io_device
 * @sim_online: SIM is online?
 */
static void io_dev_sim_state_changed(struct io_device *iod, bool sim_online)
{
	if (atomic_read(&iod->opened) == 0)
		mif_info("iod is not opened: %s\n",
				iod->name);
	else if (iod->mc->sim_state.online == sim_online)
		mif_info("sim state not changed.\n");
	else {
		iod->mc->sim_state.online = sim_online;
		iod->mc->sim_state.changed = true;

		mif_err("sim state changed. (iod: %s, state: "
				"[online=%d, changed=%d])\n",
				iod->name, iod->mc->sim_state.online,
				iod->mc->sim_state.changed);
		wake_up(&iod->wq);
	}
}

static int misc_open(struct inode *inode, struct file *filp)
{
	struct io_device *iod = to_io_device(filp->private_data);
	struct modem_shared *msd = iod->msd;
	struct link_device *ld;
	int ret;
	filp->private_data = (void *)iod;

	mif_err("iod = %s\n", iod->name);
	atomic_inc(&iod->opened);

	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld) && ld->init_comm) {
			ret = ld->init_comm(ld, iod);
			if (ret < 0) {
				mif_err("%s: init_comm error: %d\n",
						ld->name, ret);
				return ret;
			}
		}
	}

	return 0;
}

static int misc_release(struct inode *inode, struct file *filp)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct modem_shared *msd = iod->msd;
	struct link_device *ld;

	mif_err("iod = %s\n", iod->name);
	atomic_dec(&iod->opened);
	skb_queue_purge(&iod->sk_rx_q);

	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld) && ld->terminate_comm)
			ld->terminate_comm(ld, iod);
	}

	return 0;
}

static unsigned int misc_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct io_device *iod = (struct io_device *)filp->private_data;

	poll_wait(filp, &iod->wq, wait);

	if ((!skb_queue_empty(&iod->sk_rx_q))
				&& (iod->mc->phone_state != STATE_OFFLINE))
		return POLLIN | POLLRDNORM;
	else if ((iod->mc->phone_state == STATE_CRASH_RESET) ||
			(iod->mc->phone_state == STATE_CRASH_EXIT) ||
			(iod->mc->phone_state == STATE_NV_REBUILDING) ||
			iod->mc->sim_state.changed)
		return POLLHUP;
	else
		return 0;
}

static long misc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int s_state;
	struct io_device *iod = (struct io_device *)filp->private_data;
	char cpinfo_buf[530] = "CP Crash ";
	char str[TASK_COMM_LEN];

	mif_debug("cmd = 0x%x\n", cmd);

	switch (cmd) {
	case IOCTL_MODEM_ON:
		mif_debug("misc_ioctl : IOCTL_MODEM_ON\n");
		return iod->mc->ops.modem_on(iod->mc);

	case IOCTL_MODEM_OFF:
		mif_debug("misc_ioctl : IOCTL_MODEM_OFF\n");
		return iod->mc->ops.modem_off(iod->mc);

	case IOCTL_MODEM_RESET:
		mif_debug("misc_ioctl : IOCTL_MODEM_RESET\n");
		return iod->mc->ops.modem_reset(iod->mc);

	case IOCTL_MODEM_BOOT_DONE:
		mif_debug("misc_ioctl : IOCTL_MODEM_START\n");
		return 0;

	case IOCTL_MODEM_STATUS:
		mif_debug("misc_ioctl : IOCTL_MODEM_STATUS\n");

		if (iod->mc->sim_state.changed &&
			!strcmp(get_task_comm(str, get_current()), "rild")) {
			s_state = iod->mc->sim_state.online ?
					STATE_SIM_ATTACH : STATE_SIM_DETACH;
			iod->mc->sim_state.changed = false;

			mif_info("SIM states (%d) to %s\n", s_state, str);
			return s_state;
		}

		if (iod->mc->phone_state == STATE_NV_REBUILDING) {
			mif_info("send nv rebuild state : %d\n",
						iod->mc->phone_state);
			iod->mc->phone_state = STATE_ONLINE;
		}
		return iod->mc->phone_state;

	case IOCTL_MODEM_FORCE_CRASH_EXIT:
		mif_debug("misc_ioctl : IOCTL_MODEM_FORCE_CRASH_EXIT\n");
		if (iod->mc->ops.modem_force_crash_exit)
			return iod->mc->ops.modem_force_crash_exit(iod->mc);
		return -EINVAL;

	case IOCTL_MODEM_CP_UPLOAD:
		mif_err("misc_ioctl : IOCTL_MODEM_CP_UPLOAD\n");
		if (copy_from_user(cpinfo_buf + strlen(cpinfo_buf),
			(void __user *)arg, MAX_CPINFO_SIZE) != 0)
			panic("CP Crash");
		else
			panic(cpinfo_buf);
		return 0;

	default:
		mif_err("misc_ioctl : ioctl 0x%X is not defined.\n", cmd);
		return -EINVAL;
	}
}

static ssize_t misc_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *ppos)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct link_device *ld = get_current_link(iod);
	int frame_len = 0;
	char frame_header_buf[sizeof(struct sipc5_hdr)];
	struct sk_buff *skb;
	int err;
	size_t tx_size;
	u8 config = get_cfg_field(iod, ld, count);

	/* ToDo - Add handling for over size data */

	frame_len = SIPC5_SIZE_OF_CFG +
		    get_header_size(config) +
		    count;
	if (ld->aligned)
		frame_len += MAX_LINK_PADDING_SIZE;

	skb = alloc_skb(frame_len, GFP_KERNEL);
	if (!skb) {
		mif_err("fail alloc skb (%d)\n", __LINE__);
		return -ENOMEM;
	}

	switch (iod->format) {
	case IPC_CMD:
	case IPC_BOOT:
	case IPC_RAMDUMP:
		if (copy_from_user(skb_put(skb, count), buf, count) != 0) {
			dev_kfree_skb_any(skb);
			return -EFAULT;
		}
		break;

	case IPC_FMT:
	case IPC_RAW:
	case IPC_MULTI_RAW:
	case IPC_RFS:
	default:
		memcpy(skb_put(skb, SIPC5_SIZE_OF_CFG), (void *)&config,
					SIPC5_SIZE_OF_CFG);
		memcpy(skb_put(skb, get_header_size(config)),
				get_header(iod, count, frame_header_buf,
							config),
				get_header_size(config));
		if (copy_from_user(skb_put(skb, count), buf, count) != 0) {
			dev_kfree_skb_any(skb);
			return -EFAULT;
		}
		break;
	}

	if (ld->aligned)
		skb_put(skb, calc_padding_size(ld, skb->len));

	/* send data with sk_buff, link device will put sk_buff
	 * into the specific sk_buff_q and run work-q to send data
	 */
	tx_size = skb->len;

	skbpriv(skb)->iod = iod;
	skbpriv(skb)->ld = ld;

	err = ld->send(ld, iod, skb);
	if (err < 0)
		return err;

	if (err != tx_size)
		mif_err("WARNNING: wrong tx size: %s, format=%d "
			"count=%d, tx_size=%d, return_size=%d",
			iod->name, iod->format, count, tx_size, err);

	return count;
}

static ssize_t misc_read(struct file *filp, char *buf, size_t count,
			loff_t *f_pos)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct sk_buff *skb = NULL;
	int pktsize = 0;

	skb = skb_dequeue(&iod->sk_rx_q);
	if (!skb) {
		printk_ratelimited(KERN_ERR "mif: no data from sk_rx_q, "
			"modem_state : %d(%s)\n",
			iod->mc->phone_state, iod->name);
		return 0;
	}

	if (skb->len > count) {
		mif_err("skb len is too big = %d,%d!(%d)\n",
				count, skb->len, __LINE__);
		dev_kfree_skb_any(skb);
		return -EIO;
	}

	pktsize = skb->len;
	if (copy_to_user(buf, skb->data, pktsize) != 0) {
		dev_kfree_skb_any(skb);
		return -EIO;
	}
	dev_kfree_skb_any(skb);

	return pktsize;
}

static const struct file_operations misc_io_fops = {
	.owner = THIS_MODULE,
	.open = misc_open,
	.release = misc_release,
	.poll = misc_poll,
	.unlocked_ioctl = misc_ioctl,
	.write = misc_write,
	.read = misc_read,
};

static int vnet_open(struct net_device *ndev)
{
	struct vnet *vnet = netdev_priv(ndev);
	netif_start_queue(ndev);
	atomic_inc(&vnet->iod->opened);
	return 0;
}

static int vnet_stop(struct net_device *ndev)
{
	struct vnet *vnet = netdev_priv(ndev);
	atomic_dec(&vnet->iod->opened);
	netif_stop_queue(ndev);
	return 0;
}

static int vnet_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	int ret;
	int headroom = 0;
	int tailroom = 0;
	struct sk_buff *skb_new;
	struct vnet *vnet = netdev_priv(ndev);
	struct io_device *iod = vnet->iod;
	struct link_device *ld = get_current_link(iod);
	struct sipc5_hdr hd;
	u8 config = get_cfg_field(iod, ld, skb->len);
	int hd_size = get_header_size(config);

	/* ToDo - Add handling for over size data */

	/* When use `handover' with Network Bridge,
	 * user -> TCP/IP(kernel) -> bridge device -> TCP/IP(kernel) -> this.
	 *
	 * We remove the one ethernet header of skb before using skb->len,
	 * because the skb has two ethernet headers.
	 */
	if (iod->use_handover) {
		if (iod->id >= PSD_DATA_CHID_BEGIN &&
			iod->id <= PSD_DATA_CHID_END)
			skb_pull(skb, sizeof(struct ethhdr));
	}

	hd.len = skb->len + hd_size + SIPC5_SIZE_OF_CFG;
	hd.ext_field = 0;
	hd.ch_id = iod->id & 0x1F;

	headroom = hd_size + SIPC5_SIZE_OF_CFG;
	if (ld->aligned)
		tailroom = MAX_LINK_PADDING_SIZE;
	if (skb_headroom(skb) < headroom || skb_tailroom(skb) < tailroom) {
		skb_new = skb_copy_expand(skb, headroom, tailroom, GFP_ATOMIC);
		/* skb_copy_expand success or not, free old skb from caller */
		dev_kfree_skb_any(skb);
		if (!skb_new)
			return -ENOMEM;
	} else
		skb_new = skb;

	memcpy(skb_push(skb_new, hd_size), &hd, hd_size);
	memcpy(skb_push(skb_new, SIPC5_SIZE_OF_CFG), (void *)&config,
				SIPC5_SIZE_OF_CFG);
	skb_put(skb_new, calc_padding_size(ld, skb_new->len));

	skbpriv(skb_new)->iod = iod;
	skbpriv(skb_new)->ld = ld;

	ret = ld->send(ld, iod, skb_new);
	if (ret < 0) {
		netif_stop_queue(ndev);
		dev_kfree_skb_any(skb_new);
		return NETDEV_TX_BUSY;
	}

	ndev->stats.tx_packets++;
	ndev->stats.tx_bytes += skb->len;

	return NETDEV_TX_OK;
}

static struct net_device_ops vnet_ops = {
	.ndo_open = vnet_open,
	.ndo_stop = vnet_stop,
	.ndo_start_xmit = vnet_xmit,
};

static void vnet_setup(struct net_device *ndev)
{
	ndev->netdev_ops = &vnet_ops;
	ndev->type = ARPHRD_PPP;
	ndev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
	ndev->addr_len = 0;
	ndev->hard_header_len = 0;
	ndev->tx_queue_len = 1000;
	ndev->mtu = ETH_DATA_LEN;
	ndev->watchdog_timeo = 5 * HZ;
}

static void vnet_setup_ether(struct net_device *ndev)
{
	ndev->netdev_ops = &vnet_ops;
	ndev->type = ARPHRD_ETHER;
	ndev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST | IFF_SLAVE;
	ndev->addr_len = ETH_ALEN;
	random_ether_addr(ndev->dev_addr);
	ndev->hard_header_len = 0;
	ndev->tx_queue_len = 1000;
	ndev->mtu = ETH_DATA_LEN;
	ndev->watchdog_timeo = 5 * HZ;
}

int sipc5_init_io_device(struct io_device *iod)
{
	int ret = 0;
	struct vnet *vnet;

	/* get modem state from modem control device */
	iod->modem_state_changed = io_dev_modem_state_changed;

	/* to send SIM change event */
	iod->sim_state_changed = io_dev_sim_state_changed;

	/* get data from link device */
	iod->recv = io_dev_recv_data_from_link_dev;

	/* register misc or net drv */
	switch (iod->io_typ) {
	case IODEV_MISC:
		init_waitqueue_head(&iod->wq);
		skb_queue_head_init(&iod->sk_rx_q);

		iod->miscdev.minor = MISC_DYNAMIC_MINOR;
		iod->miscdev.name = iod->name;
		iod->miscdev.fops = &misc_io_fops;

		ret = misc_register(&iod->miscdev);
		if (ret)
			mif_err("failed to register misc io device : %s\n",
						iod->name);

		break;

	case IODEV_NET:
		skb_queue_head_init(&iod->sk_rx_q);
		if (iod->use_handover)
			iod->ndev = alloc_netdev(0, iod->name,
						vnet_setup_ether);
		else
			iod->ndev = alloc_netdev(0, iod->name, vnet_setup);

		if (!iod->ndev) {
			mif_err("failed to alloc netdev\n");
			return -ENOMEM;
		}

		ret = register_netdev(iod->ndev);
		if (ret)
			free_netdev(iod->ndev);

		mif_debug("(iod:0x%p)\n", iod);
		vnet = netdev_priv(iod->ndev);
		mif_debug("(vnet:0x%p)\n", vnet);
		vnet->iod = iod;

		break;

	case IODEV_DUMMY:
		skb_queue_head_init(&iod->sk_rx_q);

		iod->miscdev.minor = MISC_DYNAMIC_MINOR;
		iod->miscdev.name = iod->name;
		iod->miscdev.fops = &misc_io_fops;

		ret = misc_register(&iod->miscdev);
		if (ret)
			mif_err("failed to register misc io device : %s\n",
						iod->name);
		ret = device_create_file(iod->miscdev.this_device,
				&attr_waketime);
		if (ret)
			mif_err("failed to create sysfs file : %s\n",
					iod->name);

		break;

	default:
		mif_err("wrong io_type : %d\n", iod->io_typ);
		return -EINVAL;
	}

	mif_debug("%s(%d) : init_io_device() done : %d\n",
				iod->name, iod->io_typ, ret);
	return ret;
}

