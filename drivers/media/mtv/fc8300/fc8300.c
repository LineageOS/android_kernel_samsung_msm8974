/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fc8300.c

	Description : Driver source file

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	History :
	----------------------------------------------------------------------
*******************************************************************************/
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/slab.h>

#if defined(CONFIG_ISDBT_ANT_DET)
#include <linux/wakelock.h>
#include <linux/input.h>
#endif

#include "fc8300.h"
#include "bbm.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fc8300_regs.h"
#include "fc8300_isr.h"
#include "fci_hal.h"

struct ISDBT_INIT_INFO_T *hInit;

struct ISDBT_OPEN_INFO_T hOpen_Val;
#define RING_BUFFER_SIZE	(1024*1024)
u8 ringbuffer[RING_BUFFER_SIZE];

enum ISDBT_MODE driver_mode = ISDBT_POWEROFF;
static DEFINE_MUTEX(ringbuffer_lock);
static DECLARE_WAIT_QUEUE_HEAD(isdbt_isr_wait);
static u8 isdbt_isr_sig;

irqreturn_t isdbt_threaded_irq(int irq, void *dev_id)
{
	struct ISDBT_INIT_INFO_T *hInit = (struct ISDBT_INIT_INFO_T *)dev_id;

	if (driver_mode == ISDBT_POWERON) {
		driver_mode = ISDBT_DATAREAD;
		bbm_com_isr(hInit);
		driver_mode = ISDBT_POWERON;
	}

	return IRQ_HANDLED;
}

int data_callback(u32 hDevice, u8 bufid, u8 *data, int len)
{
	struct ISDBT_INIT_INFO_T *hInit;
	struct list_head *temp;
	hInit = (struct ISDBT_INIT_INFO_T *)hDevice;

	list_for_each(temp, &(hInit->hHead))
	{
		struct ISDBT_OPEN_INFO_T *hOpen;

		hOpen = list_entry(temp, struct ISDBT_OPEN_INFO_T, hList);

		if (hOpen->isdbttype == TS_TYPE) {
			mutex_lock(&ringbuffer_lock);
			if (fci_ringbuffer_free(&hOpen->RingBuffer) < len) {
				/*print_log(hDevice, "f"); */
				/* return 0 */;
				FCI_RINGBUFFER_SKIP(&hOpen->RingBuffer, len);
			}

			fci_ringbuffer_write(&hOpen->RingBuffer, data, len);

			wake_up_interruptible(&(hOpen->RingBuffer.queue));

			mutex_unlock(&ringbuffer_lock);
		}
	}

	return 0;
}

void isdbt_set_drv_mode(u32 mode)
{
	driver_mode = mode;
}

int isdbt_drv_open(struct inode *inode, struct file *filp)
{
	struct ISDBT_OPEN_INFO_T *hOpen;

	print_log(hInit, "isdbt open\n");

	hOpen = &hOpen_Val;
	hOpen->buf = &ringbuffer[0];
	/*kmalloc(RING_BUFFER_SIZE, GFP_KERNEL);*/
	hOpen->isdbttype = 0;

	list_add(&(hOpen->hList), &(hInit->hHead));

	hOpen->hInit = (HANDLE *)hInit;

	if (hOpen->buf == NULL) {
		print_log(hInit, "ring buffer malloc error\n");
		return -ENOMEM;
	}

	fci_ringbuffer_init(&hOpen->RingBuffer, hOpen->buf, RING_BUFFER_SIZE);

	filp->private_data = hOpen;

	return 0;
}

ssize_t isdbt_drv_read(struct file *filp
	, char *buf, size_t count, loff_t *f_pos)
{
	s32 avail;
	s32 non_blocking = filp->f_flags & O_NONBLOCK;
	struct ISDBT_OPEN_INFO_T *hOpen
		= (struct ISDBT_OPEN_INFO_T *)filp->private_data;
	struct fci_ringbuffer *cibuf = &hOpen->RingBuffer;
	ssize_t len, read_len = 0;

	if (!cibuf->data || !count)	{
		/*print_log(hInit, " return 0\n"); */
		return 0;
	}

	if (non_blocking && (fci_ringbuffer_empty(cibuf)))	{
		/*print_log(hInit, "return EWOULDBLOCK\n"); */
		return -EWOULDBLOCK;
	}

	if (wait_event_interruptible(cibuf->queue,
		!fci_ringbuffer_empty(cibuf))) {
		print_log(hInit, "return ERESTARTSYS\n");
		return -ERESTARTSYS;
	}

	mutex_lock(&ringbuffer_lock);

	avail = fci_ringbuffer_avail(cibuf);

	if (count >= avail)
		len = avail;
	else
		len = count - (count % 188);

	read_len = fci_ringbuffer_read_user(cibuf, buf, len);

	mutex_unlock(&ringbuffer_lock);

	return read_len;
}

int isdbt_drv_release(struct inode *inode, struct file *filp)
{
	struct ISDBT_OPEN_INFO_T *hOpen;

	print_log(hInit, "isdbt_release\n");

	hOpen = filp->private_data;

	hOpen->isdbttype = 0;

	list_del(&(hOpen->hList));

	return 0;
}

void isdbt_isr_check(HANDLE hDevice)
{
	u8 isr_time = 0;

	bbm_com_write(hDevice, DIV_BROADCAST, BBM_BUF_INT_ENABLE, 0x00);

	while (isr_time < 10) {
		if (!isdbt_isr_sig)
			break;

		msWait(10);
		isr_time++;
	}

}

long isdbt_drv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	s32 res = BBM_NOK;
	s32 err = 0;
	s32 size = 0;
	struct ISDBT_OPEN_INFO_T *hOpen;

	struct ioctl_info info;

	if (_IOC_TYPE(cmd) != IOCTL_MAGIC)
		return -EINVAL;
	if (_IOC_NR(cmd) >= IOCTL_MAXNR)
		return -EINVAL;

	hOpen = filp->private_data;

	size = _IOC_SIZE(cmd);

	switch (cmd) {
	case IOCTL_ISDBT_RESET:
		res = bbm_com_reset(hInit, DIV_BROADCAST);
		print_log(hInit, "[FC8300] IOCTL_ISDBT_RESET \n");
		break;
	case IOCTL_ISDBT_INIT:
		res = bbm_com_i2c_init(hInit, FCI_HPI_TYPE);
		if (res) {
			print_log(hInit
				, "FC8300 bbm_com_i2c_init Initialize Fail \n");
			break;
		}
		res |= bbm_com_probe(hInit, DIV_BROADCAST);
		if (res) {
			print_log(hInit
				, "FC8300  bbm_com_probe Initialize Fail \n");
			break;
		}
		res |= bbm_com_init(hInit, DIV_BROADCAST);
		res |= bbm_com_tuner_select(hInit
			, DIV_BROADCAST, FC8300_TUNER, ISDBT_13SEG);
		break;
	case IOCTL_ISDBT_BYTE_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_byte_read(hInit, DIV_BROADCAST, (u16)info.buff[0]
			, (u8 *)(&info.buff[1]));
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_WORD_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_word_read(hInit, DIV_BROADCAST, (u16)info.buff[0]
			, (u16 *)(&info.buff[1]));
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_LONG_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_long_read(hInit, DIV_BROADCAST, (u16)info.buff[0]
			, (u32 *)(&info.buff[1]));
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_BULK_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_bulk_read(hInit, DIV_BROADCAST, (u16)info.buff[0]
			, (u8 *)(&info.buff[2]), info.buff[1]);
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_BYTE_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_byte_write(hInit, DIV_BROADCAST, (u16)info.buff[0]
			, (u8)info.buff[1]);
		break;
	case IOCTL_ISDBT_WORD_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_word_write(hInit, DIV_BROADCAST, (u16)info.buff[0]
			, (u16)info.buff[1]);
		break;
	case IOCTL_ISDBT_LONG_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_long_write(hInit, DIV_BROADCAST, (u16)info.buff[0]
			, (u32)info.buff[1]);
		break;
	case IOCTL_ISDBT_BULK_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_bulk_write(hInit, DIV_BROADCAST, (u16)info.buff[0]
			, (u8 *)(&info.buff[2]), info.buff[1]);
		break;
	case IOCTL_ISDBT_TUNER_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_tuner_read(hInit, DIV_BROADCAST, (u8)info.buff[0]
			, (u8)info.buff[1],  (u8 *)(&info.buff[3])
			, (u8)info.buff[2]);
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_TUNER_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_tuner_write(hInit, DIV_BROADCAST, (u8)info.buff[0]
			, (u8)info.buff[1], (u8 *)(&info.buff[3])
			, (u8)info.buff[2]);
		break;
	case IOCTL_ISDBT_TUNER_SET_FREQ:
		{
			u32 f_rf;
			err = copy_from_user((void *)&info, (void *)arg, size);

			f_rf = (u32)info.buff[0] ;
			isdbt_isr_check(hInit);
			res = bbm_com_tuner_set_freq(hInit
				, DIV_MASTER, f_rf, 0x15);
			mutex_lock(&ringbuffer_lock);
			fci_ringbuffer_flush(&hOpen->RingBuffer);
			mutex_unlock(&ringbuffer_lock);
			bbm_com_write(hInit
				, DIV_BROADCAST, BBM_BUF_INT_ENABLE, 0x01);
		}
		break;
	case IOCTL_ISDBT_TUNER_SELECT:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_tuner_select(hInit
			, DIV_BROADCAST, (u32)info.buff[0], (u32)info.buff[1]);
		print_log(hInit, "[FC8300] IOCTL_ISDBT_TUNER_SELECT \n");
		break;
	case IOCTL_ISDBT_TS_START:
		hOpen->isdbttype = TS_TYPE;
		res = BBM_OK;
		print_log(hInit, "[FC8300] IOCTL_ISDBT_TS_START \n");
		break;
	case IOCTL_ISDBT_TS_STOP:
		hOpen->isdbttype = 0;
		res = BBM_OK;
		print_log(hInit, "[FC8300] IOCTL_ISDBT_TS_STOP \n");
		break;
	case IOCTL_ISDBT_POWER_ON:
		{
			int i = 0;
			while (driver_mode == ISDBT_DATAREAD) {
				msWait(100);
				if (i++ > 5)
					break;
			}
		}
		res = BBM_OK;
		print_log(hInit, "[FC8300] IOCTL_ISDBT_POWER_ON \n");
		break;
	case IOCTL_ISDBT_POWER_OFF:
		driver_mode = ISDBT_POWEROFF;
		res = BBM_OK;
		print_log(hInit, "[FC8300] IOCTL_ISDBT_POWER_OFF \n");
		break;
	case IOCTL_ISDBT_SCAN_STATUS:
		res = bbm_com_scan_status(hInit, DIV_BROADCAST);
		print_log(hInit
			, "[FC8300] IOCTL_ISDBT_SCAN_STATUS : %d\n", res);
		break;
	case IOCTL_ISDBT_TUNER_GET_RSSI:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_tuner_get_rssi(hInit
			, DIV_BROADCAST, (s32 *)&info.buff[0]);
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	default:
		print_log(hInit, "isdbt ioctl error!\n");
		res = BBM_NOK;
		break;
	}

	if (err < 0) {
		print_log(hInit, "copy to/from user fail : %d", err);
		res = BBM_NOK;
	}
	return res;
}

int isdbt_drv_probe(void)
{
	int res;

	hInit = kmalloc(sizeof(struct ISDBT_INIT_INFO_T), GFP_KERNEL);

	if (hInit == NULL) {
		print_log(hInit, "isdbt hInit malloc fail!\n");
		res = BBM_NOK;
		return res;
	}

	bbm_com_ts_callback_register((u32)hInit, data_callback);
	res = bbm_com_hostif_select(hInit, BBM_SPI);

	if (res)
		print_log(hInit, "isdbt host interface select fail!\n");

	print_log(hInit, "isdbt_drv_probe\n");

	INIT_LIST_HEAD(&(hInit->hHead));

	return res;
}

int isdbt_drv_remove(void)
{
	print_log(hInit, "ISDBT remove\n");

	bbm_com_ts_callback_deregister();
	bbm_com_hostif_deselect(hInit);

	kfree(hInit);
	return 0;
}

int isdbt_drv_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return 0;
}
