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
#include <linux/vmalloc.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/module.h>

#include <linux/io.h>
#include <mach/isdbt_tuner_pdata.h>
#include <mach/sec_debug.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <mach/gpiomux.h>
#include <linux/of_gpio.h>

#include "fc8300.h"
#include "fc8300_i2c.h"
#include "bbm.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fc8300_regs.h"
#include "fc8300_isr.h"
#include "fci_hal.h"
#include <asm/system_info.h>


struct ISDBT_INIT_INFO_T *hInit;

#define RING_BUFFER_SIZE	(188 * 320 * 17)

/* GPIO(RESET & INTRRUPT) Setting */
#define FC8300_NAME		"isdbt"
static struct isdbt_platform_data *isdbt_pdata;

#define TS0_1SEG_LENGTH	(188 * 32)
#define TS0_TMM_13SEG_LENGTH (188 * 96)

u8 static_ringbuffer[RING_BUFFER_SIZE];

#ifdef TS_DROP_DEBUG
#define FEATURE_TS_CHECK
#ifdef FEATURE_TS_CHECK
u32 check_cnt_size;

#define MAX_DEMUX           2

/*
 * Sync Byte 0xb8
 */
#define SYNC_BYTE_INVERSION

struct pid_info {
	unsigned long count;
	unsigned long discontinuity;
	unsigned long continuity;
};

struct demux_info {
	struct pid_info  pids[8192];

	unsigned long    ts_packet_c;
	unsigned long    malformed_packet_c;
	unsigned long    tot_scraped_sz;
	unsigned long    packet_no;
	unsigned long    sync_err;
	unsigned long 	   sync_err_set;
};

static int is_sync(unsigned char *p)
{
	int syncword = p[0];
#ifdef SYNC_BYTE_INVERSION
	if (0x47 == syncword || 0xb8 == syncword)
		return 1;
#else
	if (0x47 == syncword)
		return 1;
#endif
	return 0;
}
static struct demux_info demux[MAX_DEMUX];

int print_pkt_log(void)
{
	unsigned long i = 0;

	print_log(NULL, "\nPKT_TOT : %d, SYNC_ERR : %d, SYNC_ERR_BIT : %d \
		, ERR_PKT : %d \n"
		, demux[0].ts_packet_c, demux[0].sync_err
		, demux[0].sync_err_set, demux[0].malformed_packet_c);

	for (i = 0; i < 8192; i++) {
		if (demux[0].pids[i].count > 0)
			print_log(NULL, "PID : %d, TOT_PKT : %d\
							, DISCONTINUITY : %d \n"
			, i, demux[0].pids[i].count
			, demux[0].pids[i].discontinuity);
	}
	return 0;
}

int put_ts_packet(int no, unsigned char *packet, int sz)
{
	unsigned char *p;
	int transport_error_indicator, pid, payload_unit_start_indicator;
	int continuity_counter, last_continuity_counter;
	int i;
	if ((sz % 188)) {
		print_log(NULL, "L : %d", sz);
	} else {
		for (i = 0; i < sz; i += 188) {
			p = packet + i;

			pid = ((p[1] & 0x1f) << 8) + p[2];

			demux[no].ts_packet_c++;
			if (!is_sync(packet + i)) {
				print_log(NULL, "S     ");
				demux[no].sync_err++;
				if (0x80 == (p[1] & 0x80))
					demux[no].sync_err_set++;
				print_log(NULL, "0x%x, 0x%x, 0x%x, 0x%x \n"
					, *p, *(p+1),  *(p+2), *(p+3));
				continue;
			}

			transport_error_indicator = (p[1] & 0x80) >> 7;
			if (1 == transport_error_indicator) {
				demux[no].malformed_packet_c++;
				continue;
			}

			payload_unit_start_indicator = (p[1] & 0x40) >> 6;

			demux[no].pids[pid].count++;

			continuity_counter = p[3] & 0x0f;

			if (demux[no].pids[pid].continuity == -1) {
				demux[no].pids[pid].continuity
					= continuity_counter;
			} else {
				last_continuity_counter
					= demux[no].pids[pid].continuity;

				demux[no].pids[pid].continuity
					= continuity_counter;

				if (((last_continuity_counter + 1) & 0x0f)
					!= continuity_counter) {
					demux[no].pids[pid].discontinuity++;
				}
			}
		}
	}
	return 0;
}


void create_tspacket_anal(void)
{
	int n, i;

	for (n = 0; n < MAX_DEMUX; n++) {
		memset((void *)&demux[n], 0, sizeof(demux[n]));

		for (i = 0; i < 8192; i++)
			demux[n].pids[i].continuity = -1;
	}

}
#endif
#endif
enum ISDBT_MODE driver_mode = ISDBT_POWEROFF;
static DEFINE_MUTEX(ringbuffer_lock);

static DECLARE_WAIT_QUEUE_HEAD(isdbt_isr_wait);

static long 	open_cnt = 0;        /* OPEN counter             */
static long   	moni_cnt = 0;		 /* Monitor counter			 */

#ifndef BBM_I2C_TSIF
static u8 isdbt_isr_sig;
static struct task_struct *isdbt_kthread;

static irqreturn_t isdbt_irq(int irq, void *dev_id)
{
	isdbt_isr_sig = 1;
	wake_up_interruptible(&isdbt_isr_wait);
	return IRQ_HANDLED;
}
#endif

static int tuner_ioctl_set_monitor_mode ( struct file* FIle, 
											unsigned int cmd, 
											unsigned long arg )
{
	int ret = 0;
	
	ISDB_PR_DBG("tuner_ioctl_set_monitor_mode << Start >> ");
	
	if ( 1 == arg )
	{
		/* Monitor Mode Start */
		moni_cnt++;
	}
	else
	{
		/* Monitor Mode Stop */
		moni_cnt--;
		if ( 0 > moni_cnt )
		{
			ISDB_PR_INFO(" tuner_ioctl_set_monitor_mode under counter = %ld => 0", moni_cnt );
			moni_cnt = 0;
		}
	}
	ISDB_PR_INFO("tuner_ioctl_set_monitor_mode << End >> : moni_cnt = %ld", 
				 moni_cnt );

	return ( ret );
}

static int tuner_ioctl_get_open_count ( struct file*  FIle, 
										  unsigned int cmd, 
										  unsigned long arg )
{
	TUNER_STS_DATA *arg_data;
    int				ret = 0;
	unsigned long 	temp_open = 0,
				  	temp_moni = 0;
	
	ISDB_PR_INFO("tuner_ioctl_get_open_count << Start >> : open = %ld", 
				 ( open_cnt - moni_cnt ));

	/* Parameter check */
	arg_data = (TUNER_STS_DATA*)arg;
	
	if ( NULL == arg_data )
	{
		ISDB_PR_ERR("Parameter Error : arg = NULL");
		return ( -1 );
	}
	/* state check */
	if ( open_cnt < moni_cnt ) 
	{
		ISDB_PR_ERR("tuner_ioctl_get_open_count Error : open = %ld, moni = %ld", 
					 open_cnt, moni_cnt );
		return ( -1 );
	}
	temp_open = (open_cnt - moni_cnt);
	temp_moni = moni_cnt;
	
	/* Copy to User Area */
	ret = put_user ( temp_open, (unsigned long __user *)&(arg_data->open_cnt) );
	
	if ( 0 != ret )
	{
		ISDB_PR_ERR("tuner_ioctl_get_open_count put_user(arg_data->open_cnt) Error : ret = %d", ret );
		return ( -1 );
	}
	
	/* Copy to User Area */
	ret = put_user ( moni_cnt, (unsigned long __user *)&(arg_data->moni_cnt) );
	if ( 0 != ret )
	{
		ISDB_PR_ERR("tuner_ioctl_get_open_count put_user(arg_data->moni_cnt) Error : ret = %d", ret );
		return ( -1 );
	}	
	
	ISDB_PR_DBG("tuner_ioctl_get_open_count << End >>");
	ISDB_PR_INFO(" Open Count Result    : %ld", open_cnt );
	ISDB_PR_INFO(" Monitor Count Result : %ld", moni_cnt );
	
	return ( 0 );
}

int isdbt_hw_setting(void)
{
	int err;
	ISDB_PR_INFO("isdbt_hw_setting \n");

	err = gpio_request(isdbt_pdata->gpio_en, "isdbt_en");
	if (err) {
		ISDB_PR_ERR("isdbt_hw_setting: Couldn't request isdbt_en\n");
		goto ISBT_EN_ERR;
	}
	gpio_direction_output(isdbt_pdata->gpio_en, 0);
	err =	gpio_request(isdbt_pdata->gpio_rst, "isdbt_rst");
	if (err) {
		ISDB_PR_ERR("isdbt_hw_setting: Couldn't request isdbt_rst\n");
		goto ISDBT_RST_ERR;
	}
	gpio_direction_output(isdbt_pdata->gpio_rst, 0);
	
#ifndef BBM_I2C_TSIF
	err =	gpio_request(isdbt_pdata->gpio_int, "isdbt_irq");
	if (err) {
		ISDB_PR_ERR("isdbt_hw_setting: Couldn't request isdbt_irq\n");
		goto ISDBT_INT_ERR;
	}
	
	gpio_direction_input(isdbt_pdata->gpio_int);

	err = request_irq(gpio_to_irq(isdbt_pdata->gpio_int), isdbt_irq
		, IRQF_DISABLED | IRQF_TRIGGER_FALLING, FC8300_NAME, NULL);

	if (err < 0) {
		ISDB_PR_ERR("isdbt_hw_setting: couldn't request gpio	\
			interrupt %d reason(%d)\n"
			, gpio_to_irq(isdbt_pdata->gpio_int), err);
	goto request_isdbt_irq;
	}
#endif	

	

	return 0;
#ifndef BBM_I2C_TSIF	
request_isdbt_irq:
	gpio_free(isdbt_pdata->gpio_int);
ISDBT_INT_ERR:
	gpio_free(isdbt_pdata->gpio_rst);
#endif	
ISDBT_RST_ERR:
	gpio_free(isdbt_pdata->gpio_en);
ISBT_EN_ERR:
	return err;
}


static void isdbt_gpio_init(void)
{
	ISDB_PR_INFO("%s\n",__func__);

					 
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_en, GPIOMUX_FUNC_GPIO,
						GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
	//gpio_set_value(isdbt_pdata->gpio_en, 0);

	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_rst, GPIOMUX_FUNC_GPIO,
						GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
	//gpio_set_value(isdbt_pdata->gpio_rst, 0);

	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_int, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
		
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_i2c_sda, GPIOMUX_FUNC_3,
					 GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
					 GPIO_CFG_ENABLE);		
	
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_i2c_scl, GPIOMUX_FUNC_3,
					 GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
					 GPIO_CFG_ENABLE);
/*
		gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_spi_di, GPIOMUX_FUNC_2,
					 GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
					 GPIO_CFG_ENABLE);
	
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_spi_do, GPIOMUX_FUNC_2,
					 GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
					 GPIO_CFG_ENABLE);
	
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_spi_cs, GPIOMUX_FUNC_2,
					 GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
					 GPIO_CFG_ENABLE);
	
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_spi_clk, GPIOMUX_FUNC_2,
					 GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
					 GPIO_CFG_ENABLE);				 
	*/				 
	isdbt_hw_setting();
}

/*POWER_ON & HW_RESET & INTERRUPT_CLEAR */
void isdbt_hw_init(void)
{
	int i = 0;

	while (driver_mode == ISDBT_DATAREAD) {
		msWait(100);
		if (i++ > 5)
			break;
	}

	ISDB_PR_INFO("isdbt_hw_init \n");

	gpio_set_value(isdbt_pdata->gpio_rst, 0);
	gpio_set_value(isdbt_pdata->gpio_en, 1);
	mdelay(5);
	gpio_set_value(isdbt_pdata->gpio_rst, 1);
	mdelay(5);	
					 
	driver_mode = ISDBT_POWERON;
}

/*POWER_OFF */
void isdbt_hw_deinit(void)
{
	ISDB_PR_INFO("isdbt_hw_deinit \n");
	driver_mode = ISDBT_POWEROFF;
	gpio_set_value(isdbt_pdata->gpio_en, 0);
	gpio_set_value(isdbt_pdata->gpio_rst, 0);
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
#ifdef TS_DROP_DEBUG
#if 0//def FEATURE_TS_CHECK
			if (!(len%188)) {
				put_ts_packet(0, data, len);
				check_cnt_size += len;

				if (check_cnt_size > 188*32*200) {
					print_pkt_log();
					check_cnt_size = 0;
				}
			}
#endif
#endif
			mutex_lock(&ringbuffer_lock);
			if (fci_ringbuffer_free(&hOpen->RingBuffer) < len) {
#ifdef TS_DROP_DEBUG
				print_log(NULL, "[FC8300] RingBuffer full  \n");
#endif
				/* return 0 */;
				FCI_RINGBUFFER_SKIP(&hOpen->RingBuffer, len);
			}
#ifdef TS_DROP_DEBUG
			print_log(NULL, "[FC8300] RingBuffer Write %d  \n", len);
#endif
			fci_ringbuffer_write(&hOpen->RingBuffer, data, len);
			
			ISDB_PR_DBG("data_callback : %d [0x%x][0x%x]\n", len, data[0], data[1]);

			wake_up_interruptible(&(hOpen->RingBuffer.queue));

			mutex_unlock(&ringbuffer_lock);
		}
#ifdef TS_DROP_DEBUG
		else
			print_log(NULL, "[FC8300] Data callback : TS Stop \n");
#endif
	}

	return 0;
}


#ifndef BBM_I2C_TSIF
static int isdbt_thread(void *hDevice)
{
	struct ISDBT_INIT_INFO_T *hInit = (struct ISDBT_INIT_INFO_T *)hDevice;

	set_user_nice(current, -20);

	ISDB_PR_INFO("isdbt_kthread enter\n");

	bbm_com_ts_callback_register((u32)hInit, data_callback);

	while (1) {
		wait_event_interruptible(isdbt_isr_wait,
			isdbt_isr_sig || kthread_should_stop());

		if (driver_mode == ISDBT_POWERON) {
			driver_mode = ISDBT_DATAREAD;
			bbm_com_isr(hInit);
			driver_mode = ISDBT_POWERON;
		}

		isdbt_isr_sig = 0;

		if (kthread_should_stop())
			break;
	}

	bbm_com_ts_callback_deregister();

	ISDB_PR_INFO("isdbt_kthread exit\n");

	return 0;
}
#endif

const struct file_operations isdbt_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl		= isdbt_ioctl,
	.open		= isdbt_open,
	.read		= isdbt_read,
	.release	= isdbt_release,
};

static struct miscdevice fc8300_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = FC8300_NAME,
    .fops = &isdbt_fops,
};

int isdbt_open(struct inode *inode, struct file *filp)
{
	struct ISDBT_OPEN_INFO_T *hOpen;

	ISDB_PR_INFO("isdbt open\n");
	open_cnt++;
	hOpen = kmalloc(sizeof(struct ISDBT_OPEN_INFO_T), GFP_KERNEL);

	hOpen->buf = &static_ringbuffer[0];
	/*kmalloc(RING_BUFFER_SIZE, GFP_KERNEL);*/
	hOpen->isdbttype = 0;

	list_add(&(hOpen->hList), &(hInit->hHead));

	hOpen->hInit = (HANDLE *)hInit;

	if (hOpen->buf == NULL) {
		ISDB_PR_ERR("ring buffer malloc error\n");
		return -ENOMEM;
	}

	fci_ringbuffer_init(&hOpen->RingBuffer, hOpen->buf, RING_BUFFER_SIZE);

	filp->private_data = hOpen;

	return 0;
}

ssize_t isdbt_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	s32 avail;
	s32 non_blocking = filp->f_flags & O_NONBLOCK;
	struct ISDBT_OPEN_INFO_T *hOpen
		= (struct ISDBT_OPEN_INFO_T *)filp->private_data;
	struct fci_ringbuffer *cibuf = &hOpen->RingBuffer;
	ssize_t len, read_len = 0;
	
	
	if (!cibuf->data || !count)	{
		ISDB_PR_INFO(" return 0\n");
		return 0;
	}

	if (non_blocking && (fci_ringbuffer_empty(cibuf)))	{
		return -EWOULDBLOCK;
	}

	if (wait_event_interruptible(cibuf->queue,
		!fci_ringbuffer_empty(cibuf))) {
		ISDB_PR_INFO("return ERESTARTSYS\n");
		return -ERESTARTSYS;
	}

	mutex_lock(&ringbuffer_lock);

	avail = fci_ringbuffer_avail(cibuf);

	if (count >= avail)
		len = avail;
	else
		len = count - (count % 188);


	read_len = fci_ringbuffer_read_user(cibuf, buf, len);
#ifdef TS_DROP_DEBUG
#ifdef FEATURE_TS_CHECK
	if (!(read_len%188)) {
		put_ts_packet(0, buf, read_len);
		check_cnt_size += read_len;
	
		if (check_cnt_size > 188*32*200) {
			print_pkt_log();
			check_cnt_size = 0;
		}
	}
	else
		print_log(NULL, "[FC8300] Read Len Error %d \n", read_len);
#endif
#endif

	mutex_unlock(&ringbuffer_lock);


#ifdef TS_DROP_DEBUG
	print_log(hInit, "[FC8300] RingBuffer Read %d  Buffer : %d\n", read_len, count);
#endif
	return read_len;
}

int isdbt_release(struct inode *inode, struct file *filp)
{
	struct ISDBT_OPEN_INFO_T *hOpen;

	ISDB_PR_INFO("isdbt_release\n");

	if( open_cnt <= 0 )
	{
        printk("tuner_module_entry_close: close error\n");
		open_cnt = 0;
		return -1;
	}
	else
	{
		open_cnt--;
	}

	/* close all open */
	if( open_cnt == 0 )
	{
	hOpen = filp->private_data;

		//prevent issue CID 27514
		if (hOpen != NULL)
		{
		hOpen->isdbttype = 0;

		list_del(&(hOpen->hList));
		ISDB_PR_INFO("isdbt_release hList\n");
	
		/*kfree(hOpen->buf);*/

		kfree(hOpen);
		}
	}

	return 0;
}


#ifndef BBM_I2C_TSIF
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
#endif

long isdbt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
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
		ISDB_PR_INFO("IOCTL_ISDBT_RESET \n");
		break;
	case IOCTL_ISDBT_INIT:

		ISDB_PR_DBG("IOCTL_ISDBT_INIT \n");

		res = bbm_com_i2c_init(hInit, FCI_HPI_TYPE);
		ISDB_PR_INFO("IOCTL_ISDBT_INIT bbm_com_i2c_init res =%d \n",res);
		res |= bbm_com_probe(hInit, DIV_BROADCAST);
		if (res) {
			ISDB_PR_ERR("FC8300 Initialize Fail \n");
			break;
		}
		ISDB_PR_DBG("IOCTL_ISDBT_INIT bbm_com_probe success \n");
		res |= bbm_com_init(hInit, DIV_BROADCAST);
		ISDB_PR_DBG("IOCTL_ISDBT_INITbbm_com_init \n");
		#if 0
		res |= bbm_com_tuner_select(hInit
			, DIV_BROADCAST, FC8300_TUNER, ISDBT_13SEG);
		#endif
		break;
	case IOCTL_ISDBT_BYTE_READ:
		ISDB_PR_INFO("IOCTL_ISDBT_BYTE_READ \n");
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
			u8 subch;
			err = copy_from_user((void *)&info, (void *)arg, size);

			f_rf = (u32)info.buff[0];
			subch = (u8)info.buff[1];

			ISDB_PR_DBG("IOCTL_ISDBT_TUNER_SET_FREQ \
							: f_rf[%d], subch[%d]\n", f_rf, subch);

#ifndef BBM_I2C_TSIF
			isdbt_isr_check(hInit);
#endif
			res = bbm_com_tuner_set_freq(hInit
				, DIV_BROADCAST, f_rf, subch);
#ifndef BBM_I2C_TSIF
			mutex_lock(&ringbuffer_lock);
			fci_ringbuffer_flush(&hOpen->RingBuffer);
			mutex_unlock(&ringbuffer_lock);
			bbm_com_write(hInit
				, DIV_BROADCAST, BBM_BUF_INT_ENABLE, 0x01);
#endif
			ISDB_PR_DBG("IOCTL_ISDBT_TUNER_SET_FREQ BUF En\n");

		}
		break;
	case IOCTL_ISDBT_TUNER_SELECT:
		ISDB_PR_INFO("IOCTL_ISDBT_TUNER_SELECT \n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_tuner_select(hInit
			, DIV_BROADCAST, (u32)info.buff[0], (u32)info.buff[1]);

		if (((u32)info.buff[1] != ISDBT_13SEG)
			&& ((u32)info.buff[1] != ISDBTMM_13SEG)) {
			bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_START, 0);
			bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_END
				, TS0_1SEG_LENGTH - 1);
			bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_THR
				, TS0_1SEG_LENGTH / 2 - 1);
			ISDB_PR_DBG("TUNER THRESHOLD: %d \n"
			, TS0_1SEG_LENGTH / 2 - 1);
		} else if ((u32)info.buff[1] == ISDBTMM_13SEG) {
                        bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_START, 0);
                        bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_END
                                , TS0_TMM_13SEG_LENGTH - 1);
                        bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_THR
                                , TS0_TMM_13SEG_LENGTH / 2 - 1);
			   ISDB_PR_DBG("TUNER THRESHOLD: %d \n"
			   , TS0_TMM_13SEG_LENGTH / 2 - 1);
                }
		ISDB_PR_DBG("IOCTL_ISDBT_TUNER_SELECT %d \n"
		, (u32)info.buff[1]);
		break;
	case IOCTL_ISDBT_TS_START:
		ISDB_PR_DBG("IOCTL_ISDBT_TS_START \n");
#ifdef TS_DROP_DEBUG
#ifdef FEATURE_TS_CHECK
		create_tspacket_anal();
		check_cnt_size = 0;
#endif
#endif
		hOpen->isdbttype = TS_TYPE;

		break;
	case IOCTL_ISDBT_TS_STOP:
		ISDB_PR_DBG("IOCTL_ISDBT_TS_STOP \n");
		hOpen->isdbttype = 0;

		break;
	case IOCTL_ISDBT_POWER_ON:
		ISDB_PR_DBG("IOCTL_ISDBT_POWER_ON \n");
		
		isdbt_hw_init();

		res = bbm_com_probe(hInit, DIV_BROADCAST);
		
		if (res) {
			ISDB_PR_INFO("FC8300 IOCTL_ISDBT_POWER_ON FAIL \n");
			isdbt_hw_deinit();
		} else {
			ISDB_PR_INFO("FC8300 IOCTL_ISDBT_POWER_ON SUCCESS\n");
		}

		break;
	case IOCTL_ISDBT_POWER_OFF:
		ISDB_PR_DBG("IOCTL_ISDBT_POWER_OFF \n");
		isdbt_hw_deinit();

		break;
	case IOCTL_ISDBT_SCAN_STATUS:
		ISDB_PR_DBG("IOCTL_ISDBT_SCAN_STATUS \n");
		res = bbm_com_scan_status(hInit, DIV_BROADCAST);
		break;
	case IOCTL_ISDBT_TUNER_GET_RSSI:
	
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_tuner_get_rssi(hInit
			, DIV_BROADCAST, (s32 *)&info.buff[0]);
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case TUNER_IOCTL_VALGET_OPENCNT:
		res = tuner_ioctl_get_open_count ( filp->private_data, 
											 cmd, 
											 arg );
		break;

	case TUNER_IOCTL_VALSET_MONICNT:
		res = tuner_ioctl_set_monitor_mode ( filp->private_data, 
											   cmd, 
											   arg );
		break;
	default:
		ISDB_PR_INFO("isdbt ioctl error!\n");
		res = BBM_NOK;
		break;
	}

	if (err < 0) {
		ISDB_PR_INFO("copy to/from user fail : %d", err);
		res = BBM_NOK;
	}
	return res;
}



static struct isdbt_platform_data *isdbt_populate_dt_pdata(struct device *dev)
{
	struct isdbt_platform_data *pdata;
	
	ISDB_PR_DBG("%s\n", __func__);
	pdata =  devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		ISDB_PR_ERR("%s : could not allocate memory for platform data\n", __func__);
		goto err;
	}



	pdata->gpio_en = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-pwr-en", 0);
	if (pdata->gpio_en < 0) {
		ISDB_PR_ERR("%s : can not find the isdbt-detect-gpio gpio_en in the dt\n", __func__);
		goto alloc_err;
	} else
		ISDB_PR_INFO("%s : isdbt-detect-gpio gpio_en =%d\n", __func__, pdata->gpio_en);
		
	pdata->gpio_rst = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-rst", 0);
	if (pdata->gpio_rst < 0) {
		ISDB_PR_ERR("%s : can not find the isdbt-detect-gpio gpio_rst in the dt\n", __func__);
		goto alloc_err;
	} else
		ISDB_PR_INFO("%s : isdbt-detect-gpio gpio_rst =%d\n", __func__, pdata->gpio_rst);
		
	pdata->gpio_int = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-irq", 0);
	if (pdata->gpio_int < 0) {
		ISDB_PR_ERR("%s : can not find the isdbt-detect-gpio in the gpio_int dt\n", __func__);
		goto alloc_err;
	} else
		ISDB_PR_INFO("%s : isdbt-detect-gpio gpio_int =%d\n", __func__, pdata->gpio_int);
		
	pdata->gpio_i2c_sda = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-i2c_sda", 0);
	if (pdata->gpio_i2c_sda < 0) {
			ISDB_PR_ERR("%s : can not find the isdbt-detect-gpio gpio_i2c_sda in the dt\n", __func__);
			goto alloc_err;
	} else
			ISDB_PR_INFO("%s : isdbt-detect-gpio gpio_i2c_sda=%d\n", __func__, pdata->gpio_i2c_sda);

	pdata->gpio_i2c_scl = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-i2c_scl", 0);
	if (pdata->gpio_i2c_scl < 0) {
			ISDB_PR_ERR("%s : can not find the isdbt-detect-gpio gpio_i2c_scl in the dt\n", __func__);
			goto alloc_err;
	} else
			ISDB_PR_INFO("%s : isdbt-detect-gpio gpio_i2c_scl=%d\n", __func__, pdata->gpio_i2c_scl);
	
	pdata->gpio_spi_do = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-spi_do", 0);
	if (pdata->gpio_spi_do < 0) {
		ISDB_PR_ERR("%s : can not find the isdbt-detect-gpio in the gpio_spi_do dt\n", __func__);
		goto alloc_err;
	} else
		ISDB_PR_INFO("%s : isdbt-detect-gpio gpio_spi_do =%d\n", __func__, pdata->gpio_spi_do);
		
	pdata->gpio_spi_di = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-spi_di", 0);
	if (pdata->gpio_spi_di < 0) {
		ISDB_PR_ERR("%s : can not find the isdbt-detect-gpio in the gpio_spi_di dt\n", __func__);
		goto alloc_err;
	} else
		ISDB_PR_INFO("%s : isdbt-detect-gpio gpio_spi_di =%d\n", __func__, pdata->gpio_spi_di);
		
	pdata->gpio_spi_cs = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-spi_cs", 0);
	if (pdata->gpio_spi_cs < 0) {
		ISDB_PR_ERR("%s : can not find the isdbt-detect-gpio gpio_spi_cs in the dt\n", __func__);
		goto alloc_err;
	} else
		ISDB_PR_INFO("%s : isdbt-detect-gpio gpio_spi_cs=%d\n", __func__, pdata->gpio_spi_cs);
		
	pdata->gpio_spi_clk = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-spi_clk", 0);
	if (pdata->gpio_spi_clk < 0) {
		ISDB_PR_ERR("%s : can not find the isdbt-detect-gpio gpio_spi_clk in the dt\n", __func__);
		goto alloc_err;
	} else
		ISDB_PR_INFO("%s : isdbt-detect-gpio gpio_spi_clk=%d\n", __func__, pdata->gpio_spi_clk);	

	return pdata;
alloc_err:
	devm_kfree(dev, pdata);
err:
	return NULL;
}

static int isdbt_probe(struct platform_device *pdev)
{
	int res=0;
	ISDB_PR_INFO("%s\n", __func__);

	open_cnt         = 0;
	isdbt_pdata = isdbt_populate_dt_pdata(&pdev->dev);
	if (!isdbt_pdata) {
		ISDB_PR_ERR("%s : isdbt_pdata is NULL.\n", __func__);
		return -ENODEV;
	}
	
	isdbt_gpio_init();
	
	res = misc_register(&fc8300_misc_device);

	if (res < 0) {
		ISDB_PR_INFO("isdbt init fail : %d\n", res);
		return res;
	}




	hInit = kmalloc(sizeof(struct ISDBT_INIT_INFO_T), GFP_KERNEL);
	

#if defined(BBM_I2C_TSIF) || defined(BBM_I2C_SPI)
	res = bbm_com_hostif_select(hInit, BBM_I2C);
	ISDB_PR_INFO("isdbt host interface select BBM_I2C!\n");
#else
	ISDB_PR_INFO("isdbt host interface select BBM_SPI !\n");
	res = bbm_com_hostif_select(hInit, BBM_SPI);
#endif

	if (res)
		ISDB_PR_INFO("isdbt host interface select fail!\n");

#ifndef BBM_I2C_TSIF
	if (!isdbt_kthread)	{
		ISDB_PR_INFO("kthread run\n");
		isdbt_kthread = kthread_run(isdbt_thread
			, (void *)hInit, "isdbt_thread");
	}
#endif

	INIT_LIST_HEAD(&(hInit->hHead));
	return 0;



}
static int isdbt_remove(struct platform_device *pdev)
{
        ISDB_PR_INFO("ISDBT remove\n");
	return 0;
}

static int isdbt_suspend(struct platform_device *pdev, pm_message_t mesg)
{
       int value;
	
	
       value = gpio_get_value_cansleep(isdbt_pdata->gpio_en);
       ISDB_PR_DBG("%s  value = %d\n",__func__,value);
       if(value == 1)
       {
          gpio_set_value(isdbt_pdata->gpio_en, 0);
       }       

	return 0;
}

static int isdbt_resume(struct platform_device *pdev)
{
	return 0;
}


static const struct of_device_id isdbt_match_table[] = {
	{   .compatible = "isdb_fc8300_pdata",
	},
	{}
};

static struct platform_driver isdb_fc8300_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name = "isdbt",
		.of_match_table = isdbt_match_table,
	},
	.probe	= isdbt_probe,
	.remove = isdbt_remove,
	.suspend = isdbt_suspend,
	.resume = isdbt_resume,
};

int isdbt_init(void)
{
	s32 res;

	ISDB_PR_INFO("isdbt_fc8300_init started\n");

//	res = misc_register(&fc8300_misc_device);
	res = platform_driver_register(&isdb_fc8300_driver);
	if (res < 0) {
		ISDB_PR_INFO("isdbt init fail : %d\n", res);
		return res;
	}



	return 0;
}

void isdbt_exit(void)
{
	ISDB_PR_INFO("isdb_fc8300_exit \n");
	
#ifndef BBM_I2C_TSIF
	free_irq(gpio_to_irq(isdbt_pdata->gpio_int), NULL);	
	gpio_free(isdbt_pdata->gpio_int);
#endif
	gpio_free(isdbt_pdata->gpio_rst);
	gpio_free(isdbt_pdata->gpio_en);
//	gpio_free(isdbt_pdata->gpio_i2c_sda);
//	gpio_free(isdbt_pdata->gpio_i2c_scl);
#ifndef BBM_I2C_TSIF	
	if (isdbt_kthread)
	kthread_stop(isdbt_kthread);
	isdbt_kthread = NULL;
#endif

	bbm_com_hostif_deselect(hInit);

	isdbt_hw_deinit();
	platform_driver_unregister(&isdb_fc8300_driver);
	misc_deregister(&fc8300_misc_device);

	if (hInit != NULL)
		kfree(hInit);
}

module_init(isdbt_init);
module_exit(isdbt_exit);

MODULE_LICENSE("Dual BSD/GPL");

