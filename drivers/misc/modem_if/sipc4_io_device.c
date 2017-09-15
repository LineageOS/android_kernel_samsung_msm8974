/* /linux/drivers/misc/modem_if_v2/sipc4_io_device.c
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
#include <linux/export.h>

#include <linux/platform_data/modem.h>
//#include <linux/platform_data/modem_v2.h>
#include "modem_prj.h"
#include "modem_utils.h"

/*
 * MAX_RXDATA_SIZE is used at making skb, when it called with page size
 * it need more bytes to allocate itself (Ex, cache byte, shared info,
 * padding...)
 * So, give restriction to allocation size below 1 page to prevent
 * big pages broken.
 */
#define MAX_RXDATA_SIZE		0x0E00	/* 4 * 1024 - 512 */
#define MAX_BOOTDATA_SIZE	0x4008	/* EBL package format*/
#define MAX_MULTI_FMT_SIZE	0x4000	/* 16 * 1024 */



#define HDLC_START	0x7F
#define HDLC_END	0x7E
#define SIZE_OF_HDLC_START	1
#define SIZE_OF_HDLC_END	1

static const char hdlc_start[1] = { HDLC_START };
static const char hdlc_end[1] = { HDLC_END };


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

static int get_header_size(struct io_device *iod)
{
	switch (iod->format) {
	case IPC_FMT:
		return sizeof(struct fmt_hdr);

	case IPC_RAW:
	case IPC_MULTI_RAW:
		return sizeof(struct raw_hdr);

	case IPC_RFS:
		return sizeof(struct rfs_hdr);

	case IPC_BOOT:
	case IPC_BOOT_2:
		/* minimum size for transaction align */
		return 4;

	case IPC_RAMDUMP:
	default:
		return 0;
	}
}

static int get_hdlc_size(struct io_device *iod, char *buf)
{
	struct fmt_hdr *fmt_header;
	struct raw_hdr *raw_header;
	struct rfs_hdr *rfs_header;

	mif_debug("buf : %02x %02x %02x (%d)\n", *buf, *(buf + 1),
				*(buf + 2), __LINE__);

	switch (iod->format) {
	case IPC_FMT:
		fmt_header = (struct fmt_hdr *)buf;
		if (iod->mc->mdm_data->ipc_version == SIPC_VER_42)
			return fmt_header->len & 0x3FFF;
		else
		return fmt_header->len;
	case IPC_RAW:
	case IPC_MULTI_RAW:
		raw_header = (struct raw_hdr *)buf;
		return raw_header->len;
	case IPC_RFS:
		rfs_header = (struct rfs_hdr *)buf;
		return rfs_header->len;

	case IPC_CMD:
	case IPC_BOOT:
	case IPC_BOOT_2:
	case IPC_RAMDUMP:
	default:
		return 0;
	}
}

static void *get_header(struct io_device *iod, size_t count,
			char *frame_header_buf)
{
	struct fmt_hdr *fmt_h;
	struct raw_hdr *raw_h;
	struct rfs_hdr *rfs_h;

	switch (iod->format) {
	case IPC_FMT:
		fmt_h = (struct fmt_hdr *)frame_header_buf;

		fmt_h->len = count + sizeof(struct fmt_hdr);

		if (iod->mc->mdm_data->ipc_version == SIPC_VER_42 && iod->id==1)		{
			fmt_h->len = fmt_h->len | 0x4000;
		}

		fmt_h->control = 0;

		return (void *)frame_header_buf;

	case IPC_RAW:
	case IPC_MULTI_RAW:
		raw_h = (struct raw_hdr *)frame_header_buf;

		raw_h->len = count + sizeof(struct raw_hdr);
		raw_h->channel = iod->id & 0x1F;
		raw_h->control = 0;

		return (void *)frame_header_buf;

	case IPC_RFS:
		rfs_h = (struct rfs_hdr *)frame_header_buf;

		rfs_h->len = count + sizeof(struct raw_hdr);
		rfs_h->id = iod->id;

		return (void *)frame_header_buf;

	case IPC_CMD:
	case IPC_BOOT:
	case IPC_BOOT_2:
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
	return (buf[0] == HDLC_START) ? SIZE_OF_HDLC_START : -EBADMSG;
}

static inline int rx_hdlc_tail_check(char *buf)
{
	/* check hdlc tail and return size of tail byte */
	return (buf[0] == HDLC_END) ? SIZE_OF_HDLC_END : -EBADMSG;
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
		hdr->start = HDLC_START;
		hdr->len = 0;

		buf += len;
		done_len += len;
		rest -= len; /* rest, call by value */
	}
	mif_debug("check len : %d, rest : %d (%d)\n", len, rest,
				__LINE__);

	/* get header size without HDLC Start size */
	head_size = get_header_size(iod);

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
	int head_size = get_header_size(iod);
	int data_size = get_hdlc_size(iod, hdr->hdr) - head_size;
	int alloc_size=0;
	int len = 0;
	int done_len = 0;
	int rest_len = data_size - hdr->frag_len;
	int continue_len = fragdata(iod, ld)->realloc_offset;

	mif_debug("head_size : %d, data_size : %d (%d)\n", head_size,
				data_size, __LINE__);

	if (continue_len) {
		/* check the HDLC header*/
		if (rx_hdlc_head_start_check(buf) == SIZE_OF_HDLC_START) {
			rest_len -= (head_size + SIZE_OF_HDLC_START);
			continue_len += (head_size + SIZE_OF_HDLC_START);
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

/* handling modem intiated loopback packet
 * packet path: Modem -> LINK -> IOD -> LINK -> Modem
 */
static int rx_data_loopback(struct sk_buff *skb, struct io_device *iod,
			struct link_device *ld)
{
	int headroom, tailroom;
	struct sk_buff *skb_new;
	struct raw_hdr raw_header;

	mif_debug("CP LB DATA Received: size=%d\n", skb->len);

	headroom = sizeof(raw_header) + SIZE_OF_HDLC_START;
	tailroom = SIZE_OF_HDLC_END;
	if (unlikely(skb_headroom(skb) < headroom) ||
		unlikely(skb_tailroom(skb) < tailroom)) {
		skb_new = skb_copy_expand(skb, headroom, tailroom, GFP_ATOMIC);

		/* skb_copy_expand success or not, free old skb from caller */
		dev_kfree_skb_any(skb);
		if (!skb_new)
			return -ENOMEM;
	} else
		skb_new = skb;

	/* mark loopback header */
	raw_header.len = skb_new->len + sizeof(raw_header);
	raw_header.channel = CP_LOOPBACK_CHANNEL;
	raw_header.control = 0x03;

	/* fill header data and HDLC framing */
	memcpy(skb_push(skb_new, sizeof(raw_header)), &raw_header,
				sizeof(raw_header));
	memcpy(skb_push(skb_new, SIZE_OF_HDLC_START), hdlc_start,
				SIZE_OF_HDLC_START);
	memcpy(skb_put(skb_new, SIZE_OF_HDLC_END), hdlc_end, SIZE_OF_HDLC_END);

	skbpriv(skb_new)->iod = iod;
	skbpriv(skb_new)->ld = ld;

	ld->send(ld, iod, skb_new);
	return 0;
}


static int rx_multi_fmt_frame(struct sk_buff *rx_skb)
{
	struct io_device *iod = skbpriv(rx_skb)->iod;
	struct link_device *ld = skbpriv(rx_skb)->ld;
	struct fmt_hdr *fh =
		(struct fmt_hdr *)fragdata(iod, ld)->h_data.hdr;
	unsigned int id = fh->control & 0x7F;
	struct sk_buff *skb = iod->skb[id];
	unsigned char *data = fragdata(iod, ld)->skb_recv->data;
	unsigned int rcvd = fragdata(iod, ld)->skb_recv->len;

	if (!skb) {
		/* If there has been no multiple frame with this ID */
		if (!(fh->control & 0x80)) {
			/* It is a single frame because the "more" bit is 0. */
#if 0
			mif_err("\n<%s> Rx FMT frame (len %d)\n",
				iod->name, rcvd);
			print_sipc4_fmt_frame(data);
			mif_err("\n");
#endif
			skb_queue_tail(&iod->sk_rx_q,
					fragdata(iod, ld)->skb_recv);
			mif_debug("wake up wq of %s\n", iod->name);
			wake_up(&iod->wq);
			return 0;
		} else {
			struct fmt_hdr *fh = NULL;
			skb = rx_alloc_skb(MAX_MULTI_FMT_SIZE, iod, ld);
			if (!skb) {
				mif_err("<%d> alloc_skb fail\n",
					__LINE__);
				return -ENOMEM;
			}
			iod->skb[id] = skb;

			fh = (struct fmt_hdr *)data;
			mif_info("Start multi-frame (ID %d, len %d)",
				id, fh->len);
		}
	}

	/* Start multi-frame processing */

	memcpy(skb_put(skb, rcvd), data, rcvd);
	dev_kfree_skb_any(fragdata(iod, ld)->skb_recv);

	if (fh->control & 0x80) {
		/* The last frame has not arrived yet. */
		mif_info("Receiving (ID %d, %d bytes)\n",
			id, skb->len);
	} else {
		/* It is the last frame because the "more" bit is 0. */
		mif_info("The Last (ID %d, %d bytes received)\n",
			id, skb->len);
#if 0
		mif_err("\n<%s> Rx FMT frame (len %d)\n",
			iod->name, skb->len);
		print_sipc4_fmt_frame(skb->data);
		mif_err("\n");
#endif
		skb_queue_tail(&iod->sk_rx_q, skb);
		iod->skb[id] = NULL;
		mif_info("wake up wq of %s\n", iod->name);
		wake_up(&iod->wq);
	}

	return 0;
}

static int rx_multi_fmt_frame_sipc42(struct sk_buff *rx_skb)
{
	struct io_device *iod = skbpriv(rx_skb)->iod;
	struct link_device *ld = skbpriv(rx_skb)->ld;
	struct fmt_hdr *fh =
		(struct fmt_hdr *)fragdata(iod, ld)->h_data.hdr;
	unsigned int    id = fh->control & 0x7F;
	struct sk_buff *skb = iod->skb[id];
	unsigned char  *data = fragdata(iod, ld)->skb_recv->data;
	unsigned int    rcvd = fragdata(iod, ld)->skb_recv->len;

	u8 ch;
	struct io_device *real_iod = NULL;

	ch = (fh->len & 0xC000) >> 14;
	fh->len = fh->len & 0x3FFF;
	real_iod = ld->fmt_iods[ch];
	if (!real_iod) {
		mif_err("wrong channel %d\n", ch);
		return -1;
	}
	skbpriv(rx_skb)->real_iod = real_iod;

	if (!skb) {
		/* If there has been no multiple frame with this ID */
		if (!(fh->control & 0x80)) {
			/* It is a single frame because the "more" bit is 0. */
#if 0
			mif_err("\n<%s> Rx FMT frame (len %d)\n",
				iod->name, rcvd);
			print_sipc4_fmt_frame(data);
			mif_err("\n");
#endif
			skb_queue_tail(&real_iod->sk_rx_q,
					fragdata(iod, ld)->skb_recv);
			mif_debug("wake up wq of %s\n", iod->name);
			wake_up(&real_iod->wq);
			return 0;
		} else {
			struct fmt_hdr *fh = NULL;
			skb = rx_alloc_skb(MAX_MULTI_FMT_SIZE, real_iod, ld);
			if (!skb) {
				mif_err("alloc_skb fail\n");
				return -ENOMEM;
			}
			real_iod->skb[id] = skb;

			fh = (struct fmt_hdr *)data;
			mif_err("Start multi-frame (ID %d, len %d)",
				id, fh->len);
		}
	}

	/* Start multi-frame processing */

	memcpy(skb_put(skb, rcvd), data, rcvd);
	dev_kfree_skb_any(fragdata(real_iod, ld)->skb_recv);

	if (fh->control & 0x80) {
		/* The last frame has not arrived yet. */
		mif_err("Receiving (ID %d, %d bytes)\n",
			id, skb->len);
	} else {
		/* It is the last frame because the "more" bit is 0. */
		mif_err("The Last (ID %d, %d bytes received)\n",
			id, skb->len);
#if 0
		mif_err("\n<%s> Rx FMT frame (len %d)\n",
			iod->name, skb->len);
		print_sipc4_fmt_frame(skb->data);
		mif_err("\n");
#endif
		skb_queue_tail(&real_iod->sk_rx_q, skb);
		real_iod->skb[id] = NULL;
		mif_info("wake up wq of %s\n", real_iod->name);
		wake_up(&real_iod->wq);
	}

	return 0;
}

/* de-mux function draft */
static int rx_iodev_skb(struct sk_buff *skb)
{
	u8 ch;
	struct io_device *iod = skbpriv(skb)->iod;
	struct io_device *real_iod = NULL;
	struct link_device *ld = skbpriv(skb)->ld;
	struct raw_hdr *raw_header;
#ifdef CONFIG_LINK_DEVICE_SPI_RFS_DEBUG
	struct rfs_hdr *rfs_header;
#endif

	switch (iod->format) {
	case IPC_RAW:
	case IPC_MULTI_RAW:
		raw_header = (struct raw_hdr *)fragdata(iod, ld)->h_data.hdr;
		ch = raw_header->channel;

		if (ch == CP_LOOPBACK_CHANNEL)
			return rx_data_loopback(skb, iod, ld);

		real_iod = link_get_iod_with_channel(ld, 0x20 | ch);
		if (!real_iod) {
			mif_err("wrong channel %d\n", ch);
			return -1;
		}
		skbpriv(skb)->real_iod = real_iod;

		return rx_multipdp(skb);

	case IPC_CMD:
	case IPC_FMT:
#ifdef CONFIG_LINK_DEVICE_SPI_DEBUG
		mif_info("\n<%s> Rx FMT frame (len %d)\n",
			iod->name, fragdata(iod, ld)->skb_recv->len);
		print_sipc4_fmt_frame(fragdata(iod, ld)->skb_recv->data);
		mif_info("\n");
#endif
		if (iod->mc->mdm_data->ipc_version == SIPC_VER_42)
			return rx_multi_fmt_frame_sipc42(skb);
		else
			return rx_multi_fmt_frame(skb);

	case IPC_RFS:
#ifdef CONFIG_LINK_DEVICE_SPI_RFS_DEBUG
		rfs_header = (struct rfs_hdr *)fragdata(iod, ld)->h_data.hdr;
		mif_info("\n<%s> Rx RFS frame (CMD : 0x%02x) (ID : 0x%02x) (Len %d)\n",
			iod->name, rfs_header->cmd, rfs_header->id, rfs_header->len);
#endif
	case IPC_BOOT:
	case IPC_BOOT_2:
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

	mif_info("\n<%s> Rx FMT frame (len %d)\n",iod->name, rest);
#ifdef CONFIG_LINK_DEVICE_SPI_DEBUG
	mif_print_data((char*)data, rest);
	mif_info("\n");
#endif

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

	/* At this point, one HDLC frame except HDLC_END has been received. */

	err = len = rx_hdlc_tail_check(buf);
	if (err < 0) {
		mif_err("Wrong HDLC end: 0x%x(%s), rest: %d,"
			" recv_size:%d\n", *buf, iod->name, rest, recv_size);
		goto exit;
	}
	mif_debug("check len : %d, rest : %d (%d)\n", len, rest,
				__LINE__);
	buf += len;
	rest -= len;

	/* At this point, one complete HDLC frame has been received. */

	/*
	  The padding size is applied for the next HDLC frame. Zero will be
	  returned by calc_padding_size() if the link device does not require
	  4-byte aligned access.
	*/
	rcvd = get_hdlc_size(iod, fragdata(iod, ld)->h_data.hdr) +
				(SIZE_OF_HDLC_START + SIZE_OF_HDLC_END);
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
	case IPC_BOOT_2:
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
#if defined(CONFIG_SEC_DUAL_MODEM_MODE)
			(iod->mc->phone_state == STATE_MODEM_SWITCH) ||
#endif
			iod->mc->sim_state.changed)
		return POLLHUP;
	else
		return 0;
}

static long misc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int s_state;
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct link_device *ld = get_current_link(iod);
	char cpinfo_buf[530] = "CP Crash ";
	char str[TASK_COMM_LEN];
	int phone_active_value;

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

	case IOCTL_MODEM_BOOT_ON:
		mif_debug("misc_ioctl : IOCTL_MODEM_BOOT_ON\n");
		return iod->mc->ops.modem_boot_on(iod->mc);

	case IOCTL_MODEM_BOOT_OFF:
		mif_debug("misc_ioctl : IOCTL_MODEM_BOOT_OFF\n");
		return iod->mc->ops.modem_boot_off(iod->mc);

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

		if(iod->mc->phone_state==STATE_BOOTING)
		{
			phone_active_value =gpio_get_value(iod->mc->gpio_phone_active);
			mif_info("phone active value: %d / %d\n" ,iod->mc->gpio_phone_active, phone_active_value);

			if(phone_active_value)
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

	case IOCTL_MODEM_DUMP_RESET:
		mif_err("misc_ioctl : IOCTL_MODEM_DUMP_RESET\n");
		return iod->mc->ops.modem_dump_reset(iod->mc);

#if defined(CONFIG_SEC_DUAL_MODEM_MODE)
	case IOCTL_MODEM_SWITCH_MODEM:
		mif_err("misc_ioctl : IOCTL_MODEM_SWITCH_MODEM\n");
		iod->mc->phone_state = STATE_MODEM_SWITCH;
		wake_up(&iod->wq);
		return 0;
#endif


	default:
		/* If you need to handle the ioctl for specific link device,
		* then assign the link ioctl handler to ld->ioctl
		* It will be call for specific link ioctl */
		if (ld->ioctl)		//SPI_SETUP
			return ld->ioctl(ld, iod, cmd, arg);
		
		mif_err("misc_ioctl : ioctl 0x%X is not defined.\n", cmd);
		return -EINVAL;
	}
}

static size_t _boot_write(struct io_device *iod, const char __user *buf,
								size_t count)
{
	int rest_len = count, frame_len = 0;
	char *cur = (char *)buf;
	struct sk_buff *skb = NULL;
	struct link_device *ld = get_current_link(iod);
	int ret;

	while (rest_len) {
		frame_len = min(rest_len, MAX_BOOTDATA_SIZE);
		skb = alloc_skb(frame_len, GFP_KERNEL);
		if (!skb) {
			mif_err("fail alloc skb (%d)\n", __LINE__);
			return -ENOMEM;
		}
		if (copy_from_user(
				skb_put(skb, frame_len), cur, frame_len) != 0) {
			dev_kfree_skb_any(skb);
			return -EFAULT;
		}
		rest_len -= frame_len;
		cur += frame_len;

		skbpriv(skb)->iod = iod;
		skbpriv(skb)->ld = ld;

		ret = ld->send(ld, iod, skb);
		if (ret < 0) {
			dev_kfree_skb_any(skb);
			return ret;
		}
	}
	return count;
}

static ssize_t misc_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *ppos)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct link_device *ld = get_current_link(iod);
	int frame_len = 0;
	char frame_header_buf[sizeof(struct raw_hdr)];
	struct sk_buff *skb;
	int err;
	size_t tx_size;
#ifdef CONFIG_LINK_DEVICE_SPI_RFS_DEBUG
	struct rfs_hdr *rfs_header;
	char *frm;
#endif

	/* ToDo - Add handling for over size data */

	frame_len = SIZE_OF_HDLC_START +
		    get_header_size(iod) +
		    count +
		    SIZE_OF_HDLC_END;
	if (ld->aligned)
		frame_len += MAX_LINK_PADDING_SIZE;

	skb = alloc_skb(frame_len, GFP_KERNEL);
	if (!skb) {
		if (frame_len > MAX_BOOTDATA_SIZE && iod->format == IPC_BOOT) {
			mif_info("large alloc fail\n");
			return _boot_write(iod, buf, count);
		}
		mif_err("fail alloc skb (%d)\n", __LINE__);
		return -ENOMEM;
	}

	switch (iod->format) {
	case IPC_CMD:
	case IPC_BOOT:
	case IPC_BOOT_2:
	case IPC_RAMDUMP:
		if (copy_from_user(skb_put(skb, count), buf, count) != 0) {
			dev_kfree_skb_any(skb);
			return -EFAULT;
		}
		break;

	case IPC_RFS:
		memcpy(skb_put(skb, SIZE_OF_HDLC_START), hdlc_start,
				SIZE_OF_HDLC_START);
		if (copy_from_user(skb_put(skb, count), buf, count) != 0) {
			dev_kfree_skb_any(skb);
			return -EFAULT;
		}
		memcpy(skb_put(skb, SIZE_OF_HDLC_END), hdlc_end,
					SIZE_OF_HDLC_END);
		break;

	case IPC_FMT:
	case IPC_RAW:
	case IPC_MULTI_RAW:
	default:
		memcpy(skb_put(skb, SIZE_OF_HDLC_START), hdlc_start,
				SIZE_OF_HDLC_START);
		memcpy(skb_put(skb, get_header_size(iod)),
			get_header(iod, count, frame_header_buf),
			get_header_size(iod));
		if (copy_from_user(skb_put(skb, count), buf, count) != 0) {
			dev_kfree_skb_any(skb);
			return -EFAULT;
		}
		memcpy(skb_put(skb, SIZE_OF_HDLC_END), hdlc_end,
					SIZE_OF_HDLC_END);
		break;
	}

	if (ld->aligned)
		skb_put(skb, calc_padding_size(ld, skb->len));

#ifdef CONFIG_LINK_DEVICE_SPI_DEBUG
	if (iod->format == IPC_FMT) {
		mif_info("\n<%s> Tx HDLC FMT frame (len %d)\n",
			iod->name, skb->len);
		print_sipc4_hdlc_fmt_frame(skb->data);
		mif_info("\n");
	}
#endif
#ifdef CONFIG_LINK_DEVICE_SPI_RFS_DEBUG
	if (iod->format == IPC_RFS) {
		frm = (u8 *)(skb->data + 1);
		rfs_header = (struct rfs_hdr*)(frm);
		mif_info("\n<%s> Tx RFS frame (CMD : 0x%02x) (ID : 0x%02x) (Len %d)\n",
			iod->name, rfs_header->cmd, rfs_header->id, rfs_header->len);		
	}
#endif

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
	struct raw_hdr hd;

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

	hd.len = skb->len + sizeof(hd);
	hd.control = 0;
	hd.channel = iod->id & 0x1F;

	headroom = sizeof(hd) + sizeof(hdlc_start);
	tailroom = sizeof(hdlc_end);
	if (ld->aligned)
		tailroom += MAX_LINK_PADDING_SIZE;
	if (skb_headroom(skb) < headroom || skb_tailroom(skb) < tailroom) {
		skb_new = skb_copy_expand(skb, headroom, tailroom, GFP_ATOMIC);
		/* skb_copy_expand success or not, free old skb from caller */
		dev_kfree_skb_any(skb);
		if (!skb_new)
			return -ENOMEM;
	} else
		skb_new = skb;

	memcpy(skb_push(skb_new, sizeof(hd)), &hd, sizeof(hd));
	memcpy(skb_push(skb_new, sizeof(hdlc_start)), hdlc_start,
				sizeof(hdlc_start));
	memcpy(skb_put(skb_new, sizeof(hdlc_end)), hdlc_end, sizeof(hdlc_end));
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

int sipc4_init_io_device(struct io_device *iod)
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

	mif_info("%s(%d) : init_io_device() done : %d\n",
				iod->name, iod->io_typ, ret);
	return ret;
}

