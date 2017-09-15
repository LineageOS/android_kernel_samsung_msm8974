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

#include <linux/io.h>
#include <mach/isdbt_tuner_pdata.h>

#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <mach/gpiomux.h>
#include <linux/of_gpio.h>

#include "fc8150.h"
#include "bbm.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fc8150_regs.h"
#include "fc8150_isr.h"
#include "fci_hal.h"
#include "fc8150_i2c.h"
#include <asm/system_info.h>

struct ISDBT_INIT_INFO_T *hInit;

#define RING_BUFFER_SIZE	(128 * 1024)  /* kmalloc max 128k */

/* GPIO(RESET & INTRRUPT) Setting */
#define FC8150_NAME		"isdbt"
static struct isdbt_platform_data *isdbt_pdata;



enum ISDBT_MODE driver_mode = ISDBT_POWEROFF;
static DEFINE_MUTEX(ringbuffer_lock);

static DECLARE_WAIT_QUEUE_HEAD(isdbt_isr_wait);

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

int isdbt_hw_setting(void)
{
	int err;
	PRINTF(0, "isdbt_hw_setting \n");

	err = gpio_request(isdbt_pdata->gpio_en, "isdbt_en");
	if (err) {
		PRINTF(0, "isdbt_hw_setting: Couldn't request isdbt_en\n");
		goto ISBT_EN_ERR;
	}
	gpio_direction_output(isdbt_pdata->gpio_en, 0);

	err = 	gpio_request(isdbt_pdata->gpio_rst, "isdbt_rst");
	if (err) {
		PRINTF(0, "isdbt_hw_setting: Couldn't request isdbt_rst\n");
		goto ISDBT_RST_ERR;
	}
	gpio_direction_output(isdbt_pdata->gpio_rst, 1);

	err = 	gpio_request(isdbt_pdata->gpio_int, "isdbt_irq");
	if (err) {
		PRINTF(0, "isdbt_hw_setting: Couldn't request isdbt_irq\n");
		goto ISDBT_INT_ERR;
	}
	
#ifndef BBM_I2C_TSIF
	gpio_direction_input(isdbt_pdata->gpio_int);

	err = request_irq(gpio_to_irq(isdbt_pdata->gpio_int), isdbt_irq
		, IRQF_DISABLED | IRQF_TRIGGER_RISING, FC8150_NAME, NULL);

	if (err < 0) {
		PRINTF(0,
			"isdbt_hw_setting: couldn't request gpio	\
			interrupt %d reason(%d)\n"
			, gpio_to_irq(isdbt_pdata->gpio_int), err);
	goto request_isdbt_irq;
	}

	err =	gpio_request(isdbt_pdata->gpio_i2c_sda, "isdbt_i2c_sda");
	if (err) {
		pr_err("isdbt_hw_setting: Couldn't request gpio_i2c_sda\n");
		//goto ISDBT_INT_ERR;
		err = 0;
	}
	//gpio_free(isdbt_pdata->gpio_i2c_sda);
	
	err =	gpio_request(isdbt_pdata->gpio_i2c_scl, "isdbt_i2c_scl");
	if (err) {
		pr_err("isdbt_hw_setting: Couldn't request gpio_i2c_scl\n");
	//	goto ISDBT_INT_ERR;
	err = 0;
	}
//gpio_free(isdbt_pdata->gpio_i2c_scl);
#endif
	return 0;
#ifndef BBM_I2C_TSIF
request_isdbt_irq:
	gpio_free(isdbt_pdata->gpio_int);
#endif
ISDBT_INT_ERR:
	gpio_free(isdbt_pdata->gpio_rst);
ISDBT_RST_ERR:
	gpio_free(isdbt_pdata->gpio_en);
ISBT_EN_ERR:
	return err;
}


static void isdbt_gpio_init(void)
{
	pr_err("%s\n",__func__);

					 
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

	PRINTF(0, "isdbt_hw_init \n");

	gpio_set_value(isdbt_pdata->gpio_en, 1);
	gpio_set_value(isdbt_pdata->gpio_rst, 1);
	mdelay(5);
	gpio_set_value(isdbt_pdata->gpio_rst, 0);
	mdelay(1);
	gpio_set_value(isdbt_pdata->gpio_rst, 1);

	driver_mode = ISDBT_POWERON;
}

/*POWER_OFF */
void isdbt_hw_deinit(void)
{
	PRINTF(0, "isdbt_hw_deinit \n");
	driver_mode = ISDBT_POWEROFF;
	gpio_set_value(isdbt_pdata->gpio_en, 0);
	gpio_set_value(isdbt_pdata->gpio_rst, 0);
}

int data_callback(u32 hDevice, u8 *data, int len)
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
				/*PRINTF(hDevice, "f"); */
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


#ifndef BBM_I2C_TSIF
static int isdbt_thread(void *hDevice)
{
	struct ISDBT_INIT_INFO_T *hInit = (struct ISDBT_INIT_INFO_T *)hDevice;

	set_user_nice(current, -20);

	PRINTF(hInit, "isdbt_kthread enter\n");

	BBM_TS_CALLBACK_REGISTER((u32)hInit, data_callback);

	while (1) {

		wait_event_interruptible(isdbt_isr_wait,
			isdbt_isr_sig || kthread_should_stop());

		if (driver_mode == ISDBT_POWERON) {
			driver_mode = ISDBT_DATAREAD;
			BBM_ISR(hInit);
			driver_mode = ISDBT_POWERON;
		}

		isdbt_isr_sig = 0;

		if (kthread_should_stop())
			break;
	}

	BBM_TS_CALLBACK_DEREGISTER();

	PRINTF(hInit, "isdbt_kthread exit\n");

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

static struct miscdevice fc8150_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = FC8150_NAME,
    .fops = &isdbt_fops,
};

int isdbt_open(struct inode *inode, struct file *filp)
{
	struct ISDBT_OPEN_INFO_T *hOpen;

	PRINTF(hInit, "isdbt open\n");

	hOpen = kmalloc(sizeof(struct ISDBT_OPEN_INFO_T), GFP_KERNEL);

	hOpen->buf = kmalloc(RING_BUFFER_SIZE, GFP_KERNEL);
	hOpen->isdbttype = 0;

	list_add(&(hOpen->hList), &(hInit->hHead));

	hOpen->hInit = (HANDLE *)hInit;

	if (hOpen->buf == NULL) {
		PRINTF(hInit, "ring buffer malloc error\n");
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
		/*PRINTF(hInit, " return 0\n"); */
		return 0;
	}

	if (non_blocking && (fci_ringbuffer_empty(cibuf)))	{
		/*PRINTF(hInit, "return EWOULDBLOCK\n"); */
		return -EWOULDBLOCK;
	}

	if (wait_event_interruptible(cibuf->queue,
		!fci_ringbuffer_empty(cibuf))) {
		PRINTF(hInit, "return ERESTARTSYS\n");
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

int isdbt_release(struct inode *inode, struct file *filp)
{
	struct ISDBT_OPEN_INFO_T *hOpen;

	PRINTF(hInit, "isdbt_release\n");

	hOpen = filp->private_data;

	hOpen->isdbttype = 0;

	list_del(&(hOpen->hList));
	kfree(hOpen->buf);
	kfree(hOpen);

	return 0;
}

int fc8150_if_test(void)
{
	int res = 0;
	int i;
	u16 wdata = 0;
	u32 ldata = 0;
	u8 data = 0;
	u8 temp = 0;

	PRINTF(0, "fc8150_if_test Start!!!\n");
	for (i = 0 ; i < 100 ; i++) {
		BBM_BYTE_WRITE(0, 0xa4, i & 0xff);
		BBM_BYTE_READ(0, 0xa4, &data);
		if ((i & 0xff) != data) {
			PRINTF(0, "fc8150_if_btest!   i=0x%x, data=0x%x\n"
				, i & 0xff, data);
			res = 1;
		}
	}

	for (i = 0 ; i < 100 ; i++) {
		BBM_WORD_WRITE(0, 0xa4, i & 0xffff);
		BBM_WORD_READ(0, 0xa4, &wdata);
		if ((i & 0xffff) != wdata) {
			PRINTF(0, "fc8150_if_wtest!   i=0x%x, data=0x%x\n"
				, i & 0xffff, wdata);
			res = 1;
		}
	}

	for (i = 0 ; i < 100 ; i++) {
		BBM_LONG_WRITE(0, 0xa4, i & 0xffffffff);
		BBM_LONG_READ(0, 0xa4, &ldata);
		if ((i & 0xffffffff) != ldata) {
			PRINTF(0, "fc8150_if_ltest!   i=0x%x, data=0x%x\n"
				, i & 0xffffffff, ldata);
			res = 1;
		}
	}

	for (i = 0 ; i < 100 ; i++) {
		temp = i & 0xff;
		BBM_TUNER_WRITE(NULL, 0x52, 0x01, &temp, 0x01);
		BBM_TUNER_READ(NULL, 0x52, 0x01, &data, 0x01);
		if ((i & 0xff) != data)
			PRINTF(0, "FC8150 tuner test (0x%x,0x%x)\n"
			, i & 0xff, data);
	}

	PRINTF(0, "fc8150_if_test End!!!\n");

	return res;
}

#ifndef BBM_I2C_TSIF
void isdbt_isr_check(HANDLE hDevice)
{
	u8 isr_time = 0;

	BBM_WRITE(hDevice, BBM_BUF_INT, 0x00);

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
		res = BBM_RESET(hInit);
		break;
	case IOCTL_ISDBT_INIT:
#ifdef BBM_I2C_TSIF
		res = BBM_I2C_INIT(hInit, FCI_BYPASS_TYPE);
#else
		res = BBM_I2C_INIT(hInit, FCI_I2C_TYPE);
#endif
		res |= BBM_PROBE(hInit);
		if (res) {
			PRINTF(hInit, "FC8150 Initialize Fail \n");
			break;
		}
		res |= BBM_INIT(hInit);
		break;
	case IOCTL_ISDBT_BYTE_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_BYTE_READ(hInit, (u16)info.buff[0]
			, (u8 *)(&info.buff[1]));
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_WORD_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_WORD_READ(hInit, (u16)info.buff[0]
			, (u16 *)(&info.buff[1]));
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_LONG_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_LONG_READ(hInit, (u16)info.buff[0]
			, (u32 *)(&info.buff[1]));
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_BULK_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_BULK_READ(hInit, (u16)info.buff[0]
			, (u8 *)(&info.buff[2]), info.buff[1]);
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_BYTE_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_BYTE_WRITE(hInit, (u16)info.buff[0]
			, (u8)info.buff[1]);
		break;
	case IOCTL_ISDBT_WORD_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_WORD_WRITE(hInit, (u16)info.buff[0]
			, (u16)info.buff[1]);
		break;
	case IOCTL_ISDBT_LONG_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_LONG_WRITE(hInit, (u16)info.buff[0]
			, (u32)info.buff[1]);
		break;
	case IOCTL_ISDBT_BULK_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_BULK_WRITE(hInit, (u16)info.buff[0]
			, (u8 *)(&info.buff[2]), info.buff[1]);
		break;
	case IOCTL_ISDBT_TUNER_READ:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_TUNER_READ(hInit, (u8)info.buff[0]
			, (u8)info.buff[1],  (u8 *)(&info.buff[3])
			, (u8)info.buff[2]);
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_TUNER_WRITE:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_TUNER_WRITE(hInit, (u8)info.buff[0]
			, (u8)info.buff[1], (u8 *)(&info.buff[3])
			, (u8)info.buff[2]);
		break;
	case IOCTL_ISDBT_TUNER_SET_FREQ:
		{
			u32 f_rf;
			PRINTF(hInit, "IOCTL_ISDBT_TUNER_SET_FREQ\n");
			err = copy_from_user((void *)&info, (void *)arg, size);

			f_rf = ((u32)info.buff[0] - 13) * 6000 + 473143;
#ifndef BBM_I2C_TSIF
			isdbt_isr_check(hInit);
#endif
			res = BBM_TUNER_SET_FREQ(hInit, f_rf);
#ifndef BBM_I2C_TSIF
			mutex_lock(&ringbuffer_lock);
			fci_ringbuffer_flush(&hOpen->RingBuffer);
			mutex_unlock(&ringbuffer_lock);
			BBM_WRITE(hInit, BBM_BUF_INT, 0x01);
#endif
		}
		break;
	case IOCTL_ISDBT_TUNER_SELECT:
	PRINTF(hInit, "IOCTL_ISDBT_TUNER_SELECT\n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_TUNER_SELECT(hInit, (u32)info.buff[0], 0);
		break;
	case IOCTL_ISDBT_TS_START:
	PRINTF(hInit, "IOCTL_ISDBT_TS_START\n");
		hOpen->isdbttype = TS_TYPE;
		break;
	case IOCTL_ISDBT_TS_STOP:
	PRINTF(hInit, "IOCTL_ISDBT_TS_STOP\n");
		hOpen->isdbttype = 0;
		break;
	case IOCTL_ISDBT_POWER_ON:
	PRINTF(hInit, "IOCTL_ISDBT_POWER_ON\n");
		isdbt_hw_init();
		break;
	case IOCTL_ISDBT_POWER_OFF:
	PRINTF(hInit, "IOCTL_ISDBT_POWER_OFF\n");
		isdbt_hw_deinit();
		break;
	case IOCTL_ISDBT_SCAN_STATUS:
		res = BBM_SCAN_STATUS(hInit);
		break;
	case IOCTL_ISDBT_TUNER_GET_RSSI:
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_TUNER_GET_RSSI(hInit, (s32 *)&info.buff[0]);
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	default:
		PRINTF(hInit, "isdbt ioctl error!\n");
		res = BBM_NOK;
		break;
	}

	if (err < 0) {
		PRINTF(hInit, "copy to/from user fail : %d", err);
		res = BBM_NOK;
	}
	return res;
}



static struct isdbt_platform_data *isdbt_populate_dt_pdata(struct device *dev)
{
	struct isdbt_platform_data *pdata;
	
	pr_err("%s\n", __func__);
	pdata =  devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		pr_err("%s : could not allocate memory for platform data\n", __func__);
		goto err;
	}



	pdata->gpio_en = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-pwr-en", 0);
	if (pdata->gpio_en < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_en in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_err("%s : isdbt-detect-gpio gpio_en =%d\n", __func__, pdata->gpio_en);
		
	pdata->gpio_rst = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-rst", 0);
	if (pdata->gpio_rst < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_rst in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_err("%s : isdbt-detect-gpio gpio_rst =%d\n", __func__, pdata->gpio_rst);
		
	pdata->gpio_int = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-irq", 0);
	if (pdata->gpio_int < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio in the gpio_int dt\n", __func__);
		goto alloc_err;
	} else
		pr_err("%s : isdbt-detect-gpio gpio_int =%d\n", __func__, pdata->gpio_int);
		
	pdata->gpio_i2c_sda = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-i2c_sda", 0);
	if (pdata->gpio_i2c_sda < 0) {
			pr_err("%s : can not find the isdbt-detect-gpio gpio_i2c_sda in the dt\n", __func__);
			goto alloc_err;
	} else
			pr_err("%s : isdbt-detect-gpio gpio_i2c_sda=%d\n", __func__, pdata->gpio_i2c_sda);

	pdata->gpio_i2c_scl = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-i2c_scl", 0);
	if (pdata->gpio_i2c_scl < 0) {
			pr_err("%s : can not find the isdbt-detect-gpio gpio_i2c_scl in the dt\n", __func__);
			goto alloc_err;
	} else
			pr_err("%s : isdbt-detect-gpio gpio_i2c_scl=%d\n", __func__, pdata->gpio_i2c_scl);
		

	return pdata;
alloc_err:
	devm_kfree(dev, pdata);
err:
	return NULL;
}

static int isdbt_probe(struct platform_device *pdev)
{
	s32 res;
	pr_err("%s\n", __func__);

	isdbt_pdata = isdbt_populate_dt_pdata(&pdev->dev);
	if (!isdbt_pdata) {
		pr_err("%s : isdbt_pdata is NULL.\n", __func__);
		return -ENODEV;
	}
	
	isdbt_gpio_init();

	res = misc_register(&fc8150_misc_device);

	if (res < 0) {
		PRINTF(hInit, "isdbt init fail : %d\n", res);
		return res;
	}



	hInit = kmalloc(sizeof(struct ISDBT_INIT_INFO_T), GFP_KERNEL);

#ifdef BBM_I2C_TSIF
	res = BBM_HOSTIF_SELECT(hInit, BBM_I2C);
		//mdelay(1000);
		//fc8150_tsif_setting(hInit); //shubham
#else
	res = BBM_HOSTIF_SELECT(hInit, BBM_SPI);
#endif

	if (res)
		PRINTF(hInit, "isdbt host interface select fail!\n");

#ifndef BBM_I2C_TSIF
	if (!isdbt_kthread)	{
		PRINTF(hInit, "kthread run\n");
		isdbt_kthread = kthread_run(isdbt_thread
			, (void *)hInit, "isdbt_thread");
	}
#endif

	INIT_LIST_HEAD(&(hInit->hHead));

	return 0;
}
static int isdbt_remove(struct platform_device *pdev)
{
        pr_err("ISDBT remove\n");
	return 0;
}

static int isdbt_suspend(struct platform_device *pdev, pm_message_t mesg)
{
       int value;
	
	
       value = gpio_get_value_cansleep(isdbt_pdata->gpio_en);
       pr_err("%s  value = %d\n",__func__,value);
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

static struct platform_driver isdb_fc8150_driver = {
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

	pr_err("isdbt_fc8150_init started\n");


	res = platform_driver_register(&isdb_fc8150_driver);
	if (res < 0) {
		pr_err("isdbt init fail : %d\n", res);
		return res;
	}



	return 0;
}

void isdbt_exit(void)
{
	PRINTF(hInit, "isdbt isdbt_exit \n");

	free_irq(gpio_to_irq(isdbt_pdata->gpio_int), NULL);
#ifndef BBM_I2C_TSIF	
	gpio_free(isdbt_pdata->gpio_int);
#endif
	gpio_free(isdbt_pdata->gpio_rst);
	gpio_free(isdbt_pdata->gpio_en);
	gpio_free(isdbt_pdata->gpio_i2c_sda);
	gpio_free(isdbt_pdata->gpio_i2c_scl);
#ifndef BBM_I2C_TSIF	
	if (isdbt_kthread)
		kthread_stop(isdbt_kthread);

	isdbt_kthread = NULL;
#endif

	BBM_HOSTIF_DESELECT(hInit);

	isdbt_hw_deinit();
	platform_driver_unregister(&isdb_fc8150_driver);
	misc_deregister(&fc8150_misc_device);

	kfree(hInit);
}

module_init(isdbt_init);
module_exit(isdbt_exit);

MODULE_LICENSE("Dual BSD/GPL");

