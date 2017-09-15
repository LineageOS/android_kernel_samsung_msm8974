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
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/spmi.h>
#include <linux/io.h>
#include <mach/isdbt_tuner_pdata.h>
#include <mach/sec_debug.h>
#include <linux/regulator/machine.h>

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

int bbm_xtal_freq;
unsigned int fc8300_xtal_freq;

#ifndef BBM_I2C_TSIF
#define RING_BUFFER_SIZE	(188 * 320 * 17)
#endif

/* GPIO(RESET & INTRRUPT) Setting */
#define FC8300_NAME		"isdbt"
static struct isdbt_platform_data *isdbt_pdata;

#define TS0_5PKT_LENGTH	(188 * 5)
#define TS0_32PKT_LENGTH (188 * 32)

#ifndef BBM_I2C_TSIF
u8 static_ringbuffer[RING_BUFFER_SIZE];
#endif

enum ISDBT_MODE driver_mode = ISDBT_POWEROFF;
static DEFINE_MUTEX(ringbuffer_lock);

static DECLARE_WAIT_QUEUE_HEAD(isdbt_isr_wait);

static long 	open_cnt = 0;        /* OPEN counter             */
static long   	moni_cnt = 0;		 /* Monitor counter			 */

extern unsigned int system_rev;

#ifdef CONFIG_ISDBT_SPMI
static unsigned int spmi_addr;
#endif

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

#ifdef CONFIG_ISDBT_SPMI
struct isdbt_qpnp_data
{
	struct spmi_device *spmi;
};

static struct isdbt_qpnp_data *isdbt_spmi;

static int qpnp_isdbt_clk_probe(struct spmi_device *spmi)
{
	int rc;
	u8 reg = 0x00;
	struct isdbt_qpnp_data *spmi_data;

	pr_info("%s called\n", __func__);
	spmi_data = devm_kzalloc(&spmi->dev, sizeof(struct isdbt_qpnp_data), GFP_KERNEL); //CID 25032
	if(!spmi_data)
		return -ENOMEM;

	spmi_data->spmi = spmi;
	isdbt_spmi = spmi_data;
	spmi_addr = 0x5546;
	rc = spmi_ext_register_readl(spmi->ctrl, spmi->sid, spmi_addr, &reg, 1);
	if (rc) {
		pr_err("Unable to read from addr=0x%x, rc(%d)\n", spmi_addr, rc);
	}

	pr_info("%s Register 0x%x contains 0x%x\n", __func__,spmi_addr,  reg);

	return 0;
}

static int qpnp_isdbt_clk_remove(struct spmi_device *spmi)
{
	u8 reg = 0x00;
	int rc = spmi_ext_register_writel(spmi->ctrl, spmi->sid, spmi_addr,&reg, 1);
	if (rc) {
		pr_err("Unable to write from addr=%x, rc(%d)\n", spmi_addr, rc);
	}
	return 0;
}
static struct of_device_id spmi_match_table[] = {
	{
		.compatible = "qcom,qpnp-clkrf2"
	},
	{}
};

static struct spmi_driver qpnp_isdbt_clk_driver = {
	.driver = {
		.name = "qcom,qpnp-isdbclk",
		.of_match_table = spmi_match_table,
	},
	.probe = qpnp_isdbt_clk_probe,
	.remove = qpnp_isdbt_clk_remove,
};
#endif

static int tuner_ioctl_set_monitor_mode ( struct file* FIle, 
											unsigned int cmd, 
											unsigned long arg )
{
	int ret = 0;
	
	pr_info ("tuner_ioctl_set_monitor_mode << Start >> ");
	
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
			pr_info (" tuner_ioctl_set_monitor_mode under counter = %ld => 0", moni_cnt );
			moni_cnt = 0;
		}
	}
	pr_info ("tuner_ioctl_set_monitor_mode << End >> : moni_cnt = %ld", 
				 moni_cnt );

	return ( ret );
}

static int tuner_ioctl_get_open_count ( struct file*  FIle, 
										  unsigned int cmd, 
										  unsigned long arg )
{
	TUNER_STS_DATA *arg_data;
    int				ret = 0;
	unsigned long 	temp_open = 0;
	
	pr_info ("tuner_ioctl_get_open_count << Start >> : open = %ld", 
				 ( open_cnt - moni_cnt ));

	/* Parameter check */
	arg_data = (TUNER_STS_DATA*)arg;
	
	if ( NULL == arg_data )
	{
		pr_err("Parameter Error : arg = NULL");
		return ( -1 );
	}
	/* state check */
	if ( open_cnt < moni_cnt ) 
	{
		pr_err("tuner_ioctl_get_open_count Error : open = %ld, moni = %ld", 
					 open_cnt, moni_cnt );
		return ( -1 );
	}
	temp_open = (open_cnt - moni_cnt);
	
	/* Copy to User Area */
	ret = put_user ( temp_open, (unsigned long __user *)&(arg_data->open_cnt) );
	
	if ( 0 != ret )
	{
		pr_err("tuner_ioctl_get_open_count put_user(arg_data->open_cnt) Error : ret = %d", ret );
		return ( -1 );
	}
	
	/* Copy to User Area */
	ret = put_user ( moni_cnt, (unsigned long __user *)&(arg_data->moni_cnt) );
	if ( 0 != ret )
	{
		pr_err("tuner_ioctl_get_open_count put_user(arg_data->moni_cnt) Error : ret = %d", ret );
		return ( -1 );
	}	
	
	pr_info("tuner_ioctl_get_open_count << End >>");
	pr_info(" Open Count Result    : %ld", open_cnt );
	pr_info(" Monitor Count Result : %ld", moni_cnt );
	
	return ( 0 );
}

int isdbt_hw_setting(void)
{
	int err;
	pr_info("isdbt_hw_setting \n");

#ifdef CONFIG_ISDBT_F_TYPE_ANTENNA
	err = gpio_request(isdbt_pdata->gpio_tmm_sw, "isdbt_tmm_sw");
	if (err) {
		pr_info("isdbt_hw_setting: Couldn't request isdbt_tmm_sw\n");
		goto ISDBT_TMM_SW_ERR;
	}
	gpio_direction_output(isdbt_pdata->gpio_tmm_sw, 0);
#endif

	err = gpio_request(isdbt_pdata->gpio_en, "isdbt_en");
	if (err) {
		pr_info("isdbt_hw_setting: Couldn't request isdbt_en\n");
		goto ISBT_EN_ERR;
	}
	gpio_direction_output(isdbt_pdata->gpio_en, 0);
	err =	gpio_request(isdbt_pdata->gpio_rst, "isdbt_rst");
	if (err) {
		pr_info("isdbt_hw_setting: Couldn't request isdbt_rst\n");
		goto ISDBT_RST_ERR;
	}
	gpio_direction_output(isdbt_pdata->gpio_rst, 0);
	
#ifndef BBM_I2C_TSIF
	err =	gpio_request(isdbt_pdata->gpio_int, "isdbt_irq");
	if (err) {
		pr_info("isdbt_hw_setting: Couldn't request isdbt_irq\n");
		goto ISDBT_INT_ERR;
	}
	
	gpio_direction_input(isdbt_pdata->gpio_int);

	err = request_irq(gpio_to_irq(isdbt_pdata->gpio_int), isdbt_irq
		, IRQF_DISABLED | IRQF_TRIGGER_FALLING, FC8300_NAME, NULL);

	if (err < 0) {
		print_log(0,
			"isdbt_hw_setting: couldn't request gpio	\
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
#ifdef CONFIG_ISDBT_F_TYPE_ANTENNA
ISDBT_TMM_SW_ERR:
	gpio_free(isdbt_pdata->gpio_tmm_sw);
#endif
	return err;
}


static void isdbt_gpio_init(void)
{
	pr_info("%s\n",__func__);

#ifdef CONFIG_ISDBT_F_TYPE_ANTENNA
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_tmm_sw, GPIOMUX_FUNC_GPIO,
						GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
#endif

#ifdef CONFIG_MACH_KLIMT_LTE_DCM
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_en, GPIOMUX_FUNC_GPIO,
						GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
#else
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_en, GPIOMUX_FUNC_GPIO,
						GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
#endif

#ifdef CONFIG_MACH_CHAGALL_KDI
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_rst, GPIOMUX_FUNC_GPIO,
						GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
#else
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_rst, GPIOMUX_FUNC_GPIO,
						GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
#endif

#ifdef BBM_I2C_TSIF
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_int, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
#else
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_int, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
#endif
	isdbt_hw_setting();
}

#ifdef CONFIG_ISDBT_F_TYPE_ANTENNA
void isdbt_sw_ldo_on(void)
{
	int ret=0;
	struct regulator *max77826_ldo9;

	pr_info("%s set ldo ON\n", __func__);
	max77826_ldo9 = regulator_get(NULL, "max77826_ldo9");
	if(max77826_ldo9)
	{
		if (regulator_is_enabled(max77826_ldo9) > 0)
		{
			pr_info("%s LDO already ON\n", __func__);
			return;
		}
		ret = regulator_set_voltage(max77826_ldo9,2100000, 2100000);
		if (unlikely(ret < 0)){
				pr_err("ISDBT ERROR max77826_ldo9 set voltage failed.\n");
		}

		regulator_enable(max77826_ldo9);
		regulator_put(max77826_ldo9);
	}
	else
	{
		pr_err("%s ERROR !! LDO not found!!\n", __func__);
	}


}

void isdbt_sw_ldo_off(void)
{
	struct regulator *max77826_ldo9;
	pr_info("%s set ldo OFF\n", __func__);
	max77826_ldo9 = regulator_get(NULL, "max77826_ldo9");
	if(max77826_ldo9)
	{
		if (regulator_is_enabled(max77826_ldo9) > 0)
		{
			regulator_disable(max77826_ldo9);
			regulator_put(max77826_ldo9);
		}
	}
}
#endif


/*POWER_ON & HW_RESET & INTERRUPT_CLEAR */
void isdbt_hw_init(void)
{
	int i = 0;

#ifdef CONFIG_ISDBT_SPMI
	u8 reg = 0x00;
#endif

	while (driver_mode == ISDBT_DATAREAD) {
		msWait(100);
		if (i++ > 5)
			break;
	}

	pr_info("isdbt_hw_init \n");

	gpio_set_value(isdbt_pdata->gpio_rst, 0);
	gpio_set_value(isdbt_pdata->gpio_en, 1);
	msleep(2); /* fc8300 chipspec is 1ms */
#ifdef CONFIG_ISDBT_SPMI
	pr_info("%s, Enabling ISDBT_CLK\n", __func__);
	reg = 0x80;
	if (isdbt_spmi) {
		int rc = spmi_ext_register_writel(isdbt_spmi->spmi->ctrl, isdbt_spmi->spmi->sid, spmi_addr,&reg, 1);
		if (rc)
			pr_err("%s, Unable to write from addr=0x%x, rc(%d)\n", __func__, spmi_addr, rc);
	} else {
		pr_err("%s ERROR !! isdbt_spmi is NULL !!\n", __func__);
	}
#endif
	msleep(1); /* fc8300 chipspec is 360us */
	gpio_set_value(isdbt_pdata->gpio_rst, 1);
	driver_mode = ISDBT_POWERON;
}

/*POWER_OFF */
void isdbt_hw_deinit(void)
{
#ifdef CONFIG_ISDBT_SPMI
	u8 reg = 0x00;
#endif

	pr_info("isdbt_hw_deinit \n");
	driver_mode = ISDBT_POWEROFF;
#ifdef CONFIG_ISDBT_SPMI
	pr_info("%s, Turning ISDBT_CLK off\n", __func__);

	reg = 0x00;
	if(isdbt_spmi) {
		int rc = spmi_ext_register_writel(isdbt_spmi->spmi->ctrl, isdbt_spmi->spmi->sid, spmi_addr,&reg, 1);

		if (rc) {
				pr_err("%s, Unable to write from addr=%x, rc(%d)\n", __func__, spmi_addr, rc );
		}
	}
	else {
		pr_err("%s ERROR !! isdbt_spmi is NULL !!\n", __func__);
	}
#endif
	gpio_set_value(isdbt_pdata->gpio_en, 0);
	gpio_set_value(isdbt_pdata->gpio_rst, 0);
}

#ifndef BBM_I2C_TSIF
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

static int isdbt_thread(void *hDevice)
{
	struct ISDBT_INIT_INFO_T *hInit = (struct ISDBT_INIT_INFO_T *)hDevice;

	set_user_nice(current, -20);

	pr_info("isdbt_kthread enter\n");

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

	pr_info("isdbt_kthread exit\n");

	return 0;
}
#endif

const struct file_operations isdbt_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl		= isdbt_ioctl,
	.open		= isdbt_open,
#ifndef BBM_I2C_TSIF
	.read		= isdbt_read,
#endif
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

	pr_info("isdbt open\n");
	open_cnt++;
	hOpen = kmalloc(sizeof(struct ISDBT_OPEN_INFO_T), GFP_KERNEL);
	if(!hOpen)
	{
		pr_err("hOpen malloc failed ENOMEM\n");
		return -ENOMEM;
	}
#ifndef BBM_I2C_TSIF
	hOpen->buf = &static_ringbuffer[0];
	/*kmalloc(RING_BUFFER_SIZE, GFP_KERNEL);*/
#endif
	hOpen->isdbttype = 0;

	list_add(&(hOpen->hList), &(hInit->hHead));

	hOpen->hInit = (HANDLE *)hInit;

#ifndef BBM_I2C_TSIF
	if (hOpen->buf == NULL) {
		pr_info("ring buffer malloc error\n");
		return -ENOMEM;
	}

	fci_ringbuffer_init(&hOpen->RingBuffer, hOpen->buf, RING_BUFFER_SIZE);
#endif

	filp->private_data = hOpen;

	return 0;
}

#ifndef BBM_I2C_TSIF
ssize_t isdbt_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	s32 avail;
	s32 non_blocking = filp->f_flags & O_NONBLOCK;
	struct ISDBT_OPEN_INFO_T *hOpen
		= (struct ISDBT_OPEN_INFO_T *)filp->private_data;
	struct fci_ringbuffer *cibuf = &hOpen->RingBuffer;
	ssize_t len, read_len = 0;

	if (!cibuf->data || !count)	{
		/*pr_info(" return 0\n"); */
		return 0;
	}

	if (non_blocking && (fci_ringbuffer_empty(cibuf)))	{
		/*pr_info("return EWOULDBLOCK\n"); */
		return -EWOULDBLOCK;
	}

	if (wait_event_interruptible(cibuf->queue,
		!fci_ringbuffer_empty(cibuf))) {
		pr_info("return ERESTARTSYS\n");
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
#endif

int isdbt_release(struct inode *inode, struct file *filp)
{
	struct ISDBT_OPEN_INFO_T *hOpen;

	pr_info("isdbt_release\n");

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

		if (hOpen != NULL)
		{
		hOpen->isdbttype = 0;

		list_del(&(hOpen->hList));
		pr_info("isdbt_release hList\n");
		//	if(hOpen->buf)
		//	kfree(hOpen->buf);
		pr_info("isdbt_release buf\n");

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
	//pr_info("[FC8300] IOCTL_ISDBT_RESET \n");
		res = bbm_com_reset(hInit, DIV_BROADCAST);
		pr_info("[FC8300] IOCTL_ISDBT_RESET \n");
		break;
	case IOCTL_ISDBT_INIT:
		pr_info("[FC8300] IOCTL_ISDBT_INIT \n");
		res = bbm_com_i2c_init(hInit, FCI_HPI_TYPE);
		pr_info("[FC8300] IOCTL_ISDBT_INIT bbm_com_i2c_init res =%d \n",res);
		res |= bbm_com_probe(hInit, DIV_BROADCAST);
		if (res) {
			pr_info("FC8300 Initialize Fail \n");
			break;
		}
		pr_info("[FC8300] IOCTL_ISDBT_INIT bbm_com_probe success \n");
		res |= bbm_com_init(hInit, DIV_BROADCAST);
		pr_info("[FC8300] IOCTL_ISDBT_INITbbm_com_init \n");
		#if 0
		res |= bbm_com_tuner_select(hInit
			, DIV_BROADCAST, FC8300_TUNER, ISDBT_13SEG);
		#endif
		break;
	case IOCTL_ISDBT_BYTE_READ:
		pr_info("[FC8300] IOCTL_ISDBT_BYTE_READ \n");
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
			pr_info("[FC8300] IOCTL_ISDBT_TUNER_SET_FREQ freq=%d subch=%d\n",f_rf,subch);
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
		}
		break;
	case IOCTL_ISDBT_TUNER_SELECT:
	pr_info("[FC8300] IOCTL_ISDBT_TUNER_SELECT \n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = bbm_com_tuner_select(hInit
			, DIV_BROADCAST, (u32)info.buff[0], (u32)info.buff[1]);

	if (((u32)info.buff[1] == ISDBTMM_13SEG) || ((u32)info.buff[1] == ISDBT_13SEG)) {
			bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_START, 0);
			bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_END
					, TS0_32PKT_LENGTH - 1);
			bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_THR
					, TS0_32PKT_LENGTH / 2 - 1);
			   print_log(hInit, "[FC8300] TUNER THRESHOLD: %d \n"
			   , TS0_32PKT_LENGTH / 2 - 1);
                }
		else
		{
			bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_START, 0);
			bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_END
				, TS0_5PKT_LENGTH - 1);
			bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_THR
				, TS0_5PKT_LENGTH / 2 - 1);
			print_log(hInit, "[FC8300] TUNER THRESHOLD: %d \n"
			, TS0_5PKT_LENGTH / 2 - 1);
		}

		print_log(hInit, "[FC8300] IOCTL_ISDBT_TUNER_SELECT %d \n"
		, (u32)info.buff[1]);

		break;
	case IOCTL_ISDBT_TS_START:
	pr_info("[FC8300] IOCTL_ISDBT_TS_START \n");
		hOpen->isdbttype = TS_TYPE;

		break;
	case IOCTL_ISDBT_TS_STOP:
	pr_info("[FC8300] IOCTL_ISDBT_TS_STOP \n");
		hOpen->isdbttype = 0;

		break;
	case IOCTL_ISDBT_POWER_ON:
	pr_info("[FC8300] IOCTL_ISDBT_POWER_ON \n");
		
		isdbt_hw_init();
		res = bbm_com_probe(hInit, DIV_BROADCAST);
		if (res) {
			pr_info("FC8300 IOCTL_ISDBT_POWER_ON FAIL \n");
			isdbt_hw_deinit();
		} else {
			pr_info("FC8300 IOCTL_ISDBT_POWER_ON SUCCESS\n");
		}
#ifdef CONFIG_ISDBT_F_TYPE_ANTENNA
		isdbt_sw_ldo_on();
#endif
		break;
	case IOCTL_ISDBT_POWER_OFF:
	pr_info("[FC8300] IOCTL_ISDBT_POWER_OFF \n");
#ifdef CONFIG_ISDBT_F_TYPE_ANTENNA
		isdbt_sw_ldo_off();
#endif
		isdbt_hw_deinit();

		break;
	case IOCTL_ISDBT_SCAN_STATUS:
	pr_info("[FC8300] IOCTL_ISDBT_SCAN_STATUS \n");
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
		
	case IOCTL_ISDBT_TUNER_PKT_MODE:

		pr_info("[FC8300] IOCTL_ISDBT_TUNER_PKT_MODE \n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		if (!err) {
			bbm_byte_write(hInit, DIV_MASTER, BBM_BUF_ENABLE, 0x00); // buffer disable

			if ((u32)info.buff[0] == ISDBT_INTERRUPT_32_PKT) {
				pr_info("[FC8300] IOCTL_ISDBT_TUNER_PKT_MODE ISDBT_INTERRUPT_32_PKT\n");
				bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_START, 0);
				bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_END
						, TS0_32PKT_LENGTH - 1);
				bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_THR
						, TS0_32PKT_LENGTH / 2 - 1);
				   print_log(hInit, "[FC8300] TUNER THRESHOLD: %d \n"
				   , TS0_32PKT_LENGTH / 2 - 1);
			 }
			else
			{
				pr_info("[FC8300] IOCTL_ISDBT_TUNER_PKT_MODE ISDBT_INTERRUPT_5_PKT\n");
				bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_START, 0);
				bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_END
					, TS0_5PKT_LENGTH - 1);
				bbm_com_word_write(hInit, DIV_BROADCAST, BBM_BUF_TS0_THR
					, TS0_5PKT_LENGTH / 2 - 1);
				print_log(hInit, "[FC8300] TUNER THRESHOLD: %d \n"
				, TS0_5PKT_LENGTH / 2 - 1);
			}

			bbm_byte_write(hInit, DIV_MASTER, BBM_BUF_ENABLE, 0x01); // buffer enable

			print_log(hInit, "[FC8300] IOCTL_ISDBT_TUNER_PKT_MODE %lu \n"
			, info.buff[0]);
		}
		res = err;
		break;

	default:
		pr_info("isdbt ioctl error!\n");
		res = BBM_NOK;
		break;
	}

	if (err < 0) {
		pr_info("copy to/from user fail : %d", err);
		res = BBM_NOK;
	}
	return res;
}

#ifdef CONFIG_ISDBT_F_TYPE_ANTENNA

static struct device isdbt_sysfs_dev = {
	.init_name = "isdbt",
};

static ssize_t isdbt_signal_source_store(struct device *dev,
			     struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int state;
	if (sscanf(buf, "%u", &state) != 1) {
		pr_info("%s: invalid state:%u\n", __func__, state);
		return -EINVAL;
	}

	pr_info("%s: state:%u system_rev:%u\n", __func__, state, system_rev);

	/*0: F-type cable, 1: Antenna */
	if(system_rev <= 2) /*Rev 0.2 gpio: F-type cable:0, Antenna:1 */
	{
		if (state == 0) {
			pr_info("%s: state:%u Enabling F type cable by setting TMM_SW to LOW\n", __func__, state);
			gpio_set_value_cansleep(isdbt_pdata->gpio_tmm_sw, 0);
		} else if (state == 1) {
			pr_info("%s: state:%u Enabling antenna by setting TMM_SW to HIGH\n", __func__, state);
			gpio_set_value_cansleep(isdbt_pdata->gpio_tmm_sw, 1);
		} else {
			return -EINVAL;
		}
	}
	else	/*Rev 0.3 gpio: F-type cable:1, Antenna:0 */
	{
		if (state == 0) {
			pr_info("%s: state:%u Enabling F type cable by setting TMM_SW to HIGH\n", __func__, state);
			gpio_set_value_cansleep(isdbt_pdata->gpio_tmm_sw, 1);
		} else if (state == 1) {
			pr_info("%s: state:%u Enabling antenna by setting TMM_SW to LOW\n", __func__, state);
			gpio_set_value_cansleep(isdbt_pdata->gpio_tmm_sw, 0);
		} else {
			return -EINVAL;
		}
	}
	return size;
}
/*
static ssize_t isdbt_signal_source_show(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	int ret;
	ret = gpio_get_value_cansleep(isdbt_pdata->gpio_tmm_sw);
	pr_info("%s: gpio_tmm_sw state:%d\n", __func__, ret);
	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}
*/
static DEVICE_ATTR(isdbt_signal_source, (S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP), NULL, isdbt_signal_source_store);
#endif

static struct isdbt_platform_data *isdbt_populate_dt_pdata(struct device *dev)
{
	struct isdbt_platform_data *pdata;

	pr_info("%s\n", __func__);
	pdata =  devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		pr_err("%s : could not allocate memory for platform data\n", __func__);
		goto err;
	}

	of_property_read_u32(dev->of_node, "qcom,isdb-bbm-xtal-freq", &bbm_xtal_freq);
	if (bbm_xtal_freq < 0)
	{
		pr_err("%s : can not find the isdbt-bbmxtal-freq in the dt, set to : 26000\n", __func__);
		bbm_xtal_freq = 26000;
	}

	pdata->gpio_en = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-pwr-en", 0);
	if (pdata->gpio_en < 0)
		of_property_read_u32(dev->of_node, "qcom,isdb-gpio-pwr-en", &pdata->gpio_en );
	if (pdata->gpio_en < 0)
	{
		pr_err("%s : can not find the isdbt-detect-gpio gpio_en in the dt\n", __func__);
		goto alloc_err;
	} else {
		pr_info("%s : isdbt-detect-gpio gpio_en =%d\n", __func__, pdata->gpio_en);
	}

	pdata->gpio_rst = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-rst", 0);
	if (pdata->gpio_rst < 0)
		of_property_read_u32(dev->of_node, "qcom,isdb-gpio-rst", &pdata->gpio_rst);
	if (pdata->gpio_rst < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_rst in the dt\n", __func__);
		goto alloc_err;
	} else {
		pr_info("%s : isdbt-detect-gpio gpio_rst =%d\n", __func__, pdata->gpio_rst);
	}

	pdata->gpio_int = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-irq", 0);
	if (pdata->gpio_int < 0)
		of_property_read_u32(dev->of_node, "qcom,isdb-gpio-irq", &pdata->gpio_int);
	if (pdata->gpio_int < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio in the gpio_int dt\n", __func__);
		goto alloc_err;
	} else {
		pr_info("%s : isdbt-detect-gpio gpio_int =%d\n", __func__, pdata->gpio_int);
	}
		
	pdata->gpio_i2c_sda = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-i2c_sda", 0);
	if (pdata->gpio_i2c_sda < 0)
		of_property_read_u32(dev->of_node, "qcom,isdb-gpio-i2c_sda", &pdata->gpio_i2c_sda);
	if (pdata->gpio_i2c_sda < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_i2c_sda in the dt\n", __func__);
		goto alloc_err;
	} else {
		pr_info("%s : isdbt-detect-gpio gpio_i2c_sda=%d\n", __func__, pdata->gpio_i2c_sda);
	}

	pdata->gpio_i2c_scl = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-i2c_scl", 0);
	if (pdata->gpio_i2c_scl < 0)
		of_property_read_u32(dev->of_node, "qcom,isdb-gpio-i2c_scl", &pdata->gpio_i2c_scl);
	if (pdata->gpio_i2c_scl < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_i2c_scl in the dt\n", __func__);
		goto alloc_err;
	} else {
		pr_info("%s : isdbt-detect-gpio gpio_i2c_scl=%d\n", __func__, pdata->gpio_i2c_scl);
	}

#ifdef CONFIG_ISDBT_F_TYPE_ANTENNA
	pdata->gpio_tmm_sw = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-tmm_sw", 0);
	if (pdata->gpio_tmm_sw < 0)
		of_property_read_u32(dev->of_node, "qcom,isdb-gpio-tmm_sw", &pdata->gpio_tmm_sw);
	if (pdata->gpio_tmm_sw < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_tmm_sw in the dt\n", __func__);
		goto alloc_err;
	} else {
		pr_info("%s : isdbt-detect-gpio gpio_tmm_sw=%d\n", __func__, pdata->gpio_tmm_sw);
	}
#endif

	return pdata;
alloc_err:
	devm_kfree(dev, pdata);
err:
	return NULL;
}

static int isdbt_probe(struct platform_device *pdev)
{
	int res=0;
	pr_info("%s\n", __func__);

	open_cnt         = 0;
	isdbt_pdata = isdbt_populate_dt_pdata(&pdev->dev);
	if (!isdbt_pdata) {
		pr_err("%s : isdbt_pdata is NULL.\n", __func__);
		return -ENODEV;
	}
	
	isdbt_gpio_init();
	fc8300_xtal_freq = bbm_xtal_freq;
	
	res = misc_register(&fc8300_misc_device);

	if (res < 0) {
		pr_info("isdbt init fail : %d\n", res);
		return res;
	}




	hInit = kmalloc(sizeof(struct ISDBT_INIT_INFO_T), GFP_KERNEL);
	

#if defined(BBM_I2C_TSIF) || defined(BBM_I2C_SPI)
	res = bbm_com_hostif_select(hInit, BBM_I2C);
	pr_info("isdbt host interface select BBM_I2C!\n");
#else
	pr_info("isdbt host interface select BBM_SPI !\n");
	res = bbm_com_hostif_select(hInit, BBM_SPI);
#endif

	if (res)
		pr_info("isdbt host interface select fail!\n");

#ifndef BBM_I2C_TSIF
	if (!isdbt_kthread)	{
		pr_info("kthread run\n");
		isdbt_kthread = kthread_run(isdbt_thread
			, (void *)hInit, "isdbt_thread");
	}
#endif

#ifdef CONFIG_ISDBT_F_TYPE_ANTENNA
	res = device_register(&isdbt_sysfs_dev);
	if(res){
		pr_err("[W1] error register isdbt_sysfs_dev device\n");
	} else {
		res = sysfs_create_file(&isdbt_sysfs_dev.kobj, &dev_attr_isdbt_signal_source.attr);
		if(res < 0)
			pr_info("couldn't create sysfs for F-type cable\n");
		else
			pr_info("created sysfs for F-type cable\n");
	}
#endif

	INIT_LIST_HEAD(&(hInit->hHead));
	return 0;



}
static int isdbt_remove(struct platform_device *pdev)
{
        pr_info("ISDBT remove\n");
	return 0;
}

static int isdbt_suspend(struct platform_device *pdev, pm_message_t mesg)
{
       int value;
#ifdef CONFIG_ISDBT_SPMI
	u8 reg = 0x00;
	pr_info("%s, Turning ISDBT_CLK off\n", __func__);

	if(isdbt_spmi) {
		int rc = spmi_ext_register_writel(isdbt_spmi->spmi->ctrl, isdbt_spmi->spmi->sid, spmi_addr,&reg, 1);
		if (rc) {
				pr_err("%s, Unable to write from addr=%x, rc(%d)\n", __func__, spmi_addr, rc );
		}
	}
	else {
		pr_err("%s ERROR !! isdbt_spmi is NULL !!\n", __func__);
	}
#endif
	
       value = gpio_get_value_cansleep(isdbt_pdata->gpio_en);
       pr_info("%s  value = %d\n",__func__,value);
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

	pr_info("isdbt_fc8300_init started\n");

#ifdef CONFIG_ISDBT_SPMI
	res = spmi_driver_register(&qpnp_isdbt_clk_driver);
	if(res < 0){
		pr_err("Error : qpnp isdbt clk init fail : %d\n", res);
	}
#endif

//	res = misc_register(&fc8300_misc_device);
	res = platform_driver_register(&isdb_fc8300_driver);
	if (res < 0) {
		pr_info("isdbt init fail : %d\n", res);
		return res;
	}



	return 0;
}

void isdbt_exit(void)
{
	pr_info("isdb_fc8300_exit \n");
	
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
#ifdef CONFIG_ISDBT_F_TYPE_ANTENNA
	gpio_free(isdbt_pdata->gpio_tmm_sw);
	sysfs_remove_file(&isdbt_sysfs_dev.kobj, &dev_attr_isdbt_signal_source.attr);
	device_unregister(&isdbt_sysfs_dev);
#endif

	bbm_com_hostif_deselect(hInit);

	isdbt_hw_deinit();
	platform_driver_unregister(&isdb_fc8300_driver);
	misc_deregister(&fc8300_misc_device);

	if (hInit != NULL)
		kfree(hInit);
#ifdef CONFIG_ISDBT_SPMI
	spmi_driver_unregister(&qpnp_isdbt_clk_driver);
#endif
}

module_init(isdbt_init);
module_exit(isdbt_exit);

MODULE_LICENSE("Dual BSD/GPL");

