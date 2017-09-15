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
#include <mach/isdbt_pdata.h>

#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <mach/gpiomux.h>
#include <linux/of_gpio.h>



#if defined(CONFIG_ISDBT_ANT_DET)
static struct wake_lock isdbt_ant_wlock;
#endif

#include "fc8150.h"
#include "bbm.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fc8150_regs.h"
#include "fc8150_isr.h"
#include "fci_hal.h"
#include <asm/system_info.h>


struct ISDBT_INIT_INFO_T *hInit;

#define RING_BUFFER_SIZE	(128 * 1024)  /* kmalloc max 128k */

/* GPIO(RESET & INTRRUPT) Setting */
#define FC8150_NAME		"isdbt"
static struct isdbt_platform_data *isdbt_pdata;

u8 static_ringbuffer[RING_BUFFER_SIZE];

enum ISDBT_MODE driver_mode = ISDBT_POWEROFF;
static DEFINE_MUTEX(ringbuffer_lock);

static DECLARE_WAIT_QUEUE_HEAD(isdbt_isr_wait);

static u8 isdbt_isr_sig;
static struct task_struct *isdbt_kthread;
#ifdef CONFIG_ISDBT_SPMI
static unsigned int spmi_addr;
#endif
void isdbt_hw_start(void);
void isdbt_hw_stop(void);

#ifdef CONFIG_ISDBT_SPMI
struct isdbt_qpnp_data
{
	struct spmi_device *spmi;
};

static struct isdbt_qpnp_data *isdbt_spmi;

static int __devinit qpnp_isdbt_clk_probe(struct spmi_device *spmi)
{

		int rc;
		u8 reg = 0x00;
		struct isdbt_qpnp_data *spmi_data;

		printk("%s called\n", __func__);
		spmi_data =  devm_kzalloc(&spmi->dev, sizeof(spmi_data), GFP_KERNEL);
		if(!spmi_data)
			return -ENOMEM;
		
		spmi_data->spmi = spmi;
		isdbt_spmi = spmi_data;
                if(system_rev >= 9)
                {
                    spmi_addr = 0x5546;
                }
                else
                {
                    spmi_addr = 0x5246;
                }
		rc = spmi_ext_register_readl(spmi->ctrl, spmi->sid, spmi_addr, &reg, 1);
		if (rc) {
			printk("Unable to read from addr=0x%x, rc(%d)\n", spmi_addr, rc);
		}

		printk("%s Register 0x%x contains 0x%x\n", __func__,spmi_addr,  reg);


		return 0;
}

static int __devexit qpnp_isdbt_clk_remove(struct spmi_device *spmi)
{
		u8 reg = 0x00;
		int rc = spmi_ext_register_writel(spmi->ctrl, spmi->sid, spmi_addr,&reg, 1);
		if (rc) {
				printk("Unable to write from addr=%x, rc(%d)\n", spmi_addr, rc);
		}
		return 0;
}
static struct of_device_id spmi_match_table[] = {
		    {  
                          .compatible = "qcom,qpnp-clkrf2"
			},
			{  
                          .compatible = "qcom,qpnp-clkbb2"
			}
};

static struct spmi_driver qpnp_isdbt_clk_driver = {
		    .driver     = {
					        .name   = "qcom,qpnp-isdbclk", 
		                    .of_match_table = spmi_match_table,
			 		    },
			.probe      = qpnp_isdbt_clk_probe,
			.remove     = __devexit_p(qpnp_isdbt_clk_remove),
};

#endif


static irqreturn_t isdbt_irq(int irq, void *dev_id)
{
	isdbt_isr_sig = 1;
	wake_up_interruptible(&isdbt_isr_wait);

	return IRQ_HANDLED;
}



int isdbt_hw_setting(void)
{
	int err;
	printk("isdbt_hw_setting\n");

	err = gpio_request(isdbt_pdata->gpio_en, "isdbt_en");
	if (err) {
		printk("isdbt_hw_setting: Couldn't request isdbt_en\n");
		goto ISBT_EN_ERR;
	}
	gpio_direction_output(isdbt_pdata->gpio_en, 0);	//moved to hw_init

	err =	gpio_request(isdbt_pdata->gpio_rst, "isdbt_rst");
	if (err) {
		printk("isdbt_hw_setting: Couldn't request isdbt_rst\n");
		goto ISDBT_RST_ERR;
	}
	gpio_direction_output(isdbt_pdata->gpio_rst, 1);

	err =	gpio_request(isdbt_pdata->gpio_int, "isdbt_irq");
	if (err) {
		printk("isdbt_hw_setting: Couldn't request isdbt_irq\n");
		goto ISDBT_INT_ERR;
	}
//	gpio_direction_input(isdbt_pdata->gpio_int);

#ifdef CONFIG_ISDBT_ANT_DET
	err =	gpio_request(isdbt_pdata->gpio_ant_det, "gpio_ant_det");
	if (err) {
		printk("isdbt_hw_setting: Couldn't request gpio_ant_det\n");
		goto ISDBT_ANT_DET_ERR;
	}
//	gpio_direction_input(isdbt_pdata->gpio_ant_det);
#endif

	
	return 0;
#ifdef CONFIG_ISDBT_ANT_DET
ISDBT_ANT_DET_ERR:
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
	printk("%s\n",__func__);
#ifdef CONFIG_ISDBT_FC8150_HKDI
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_spi_di, GPIOMUX_FUNC_1,
					 GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
					 GPIO_CFG_ENABLE);
	
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_spi_do, GPIOMUX_FUNC_1,
					 GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
					 GPIO_CFG_ENABLE);
	
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_spi_cs, GPIOMUX_FUNC_1,
					 GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
					 GPIO_CFG_ENABLE);
	
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_spi_clk, GPIOMUX_FUNC_1,
					 GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
					 GPIO_CFG_ENABLE);
#endif
					 
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

	printk("%s START\n", __func__);
	

	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_int, GPIOMUX_FUNC_GPIO,
        GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
        GPIO_CFG_ENABLE);
#ifdef CONFIG_ISDBT_ANT_DET
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_ant_det, GPIOMUX_FUNC_GPIO,
        GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
        GPIO_CFG_ENABLE);
#endif
	isdbt_hw_start();
}

void isdbt_hw_start(void)
{
#ifdef CONFIG_ISDBT_SPMI	
	u8 reg = 0x00;

#endif
	
#ifdef CONFIG_ISDBT_ANT_DET
	gpio_direction_input(isdbt_pdata->gpio_ant_det);
#endif
	//gpio_direction_output(isdbt_pdata->gpio_en, 1);
	gpio_set_value(isdbt_pdata->gpio_en, 1);

	gpio_direction_input(isdbt_pdata->gpio_int);	
	//gpio_direction_output(isdbt_pdata->gpio_rst, 1);
	//gpio_set_value(isdbt_pdata->gpio_rst, 1);
	mdelay(3);
#ifdef CONFIG_ISDBT_SPMI
	printk("%s, Enabling ISDBT_CLK\n", __func__);
	printk("%s Writing 0x80 to register 0x5246\n", __func__);

	reg = 0x80;
	if(isdbt_spmi)
	{
		int rc = spmi_ext_register_writel(isdbt_spmi->spmi->ctrl, isdbt_spmi->spmi->sid, spmi_addr,&reg, 1);
		if (rc) {
				printk("%s, Unable to write from addr=0x%x, rc(%d)\n", __func__, spmi_addr, rc);
		}
	}
	else
		printk("%s ERROR !! isdbt_spmi is NULL !!\n", __func__);
#endif	
	udelay(600);
	
	//mdelay(5);  // removed
	gpio_set_value(isdbt_pdata->gpio_rst, 0);
	mdelay(1);
	gpio_set_value(isdbt_pdata->gpio_rst, 1);

	driver_mode = ISDBT_POWERON;

	printk("%s \n END. driver_mode=%d ", __func__, driver_mode);	
}

/*POWER_OFF */
void isdbt_hw_stop(void)
{
#ifdef CONFIG_ISDBT_SPMI
	u8 reg = 0x00;
#endif	

	printk("%st\n", __func__);

	driver_mode = ISDBT_POWEROFF;
	
#ifdef CONFIG_ISDBT_SPMI	
	printk("%s, Tuning ISDBT_CLK off\n", __func__);
	printk("%s Writing 0x00 to register 0x%x\n", __func__, spmi_addr);                

	reg = 0x00;
	if(isdbt_spmi)
	{
		int rc = spmi_ext_register_writel(isdbt_spmi->spmi->ctrl, isdbt_spmi->spmi->sid, spmi_addr,&reg, 1);
 
		if (rc) {
				printk("%s, Unable to write from addr=%x, rc(%d)\n", __func__, spmi_addr, rc );
		}
	}
	else
		printk("%s ERROR !! isdbt_spmi is NULL !!\n", __func__);
#endif		
	
	gpio_set_value(isdbt_pdata->gpio_en, 0);

}

void isdbt_hw_deinit(void)
{

	printk("isdbt_hw_deinit\n");

	isdbt_hw_stop();
	
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_int, GPIOMUX_FUNC_GPIO,
        GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
        GPIO_CFG_ENABLE);
#ifdef CONFIG_ISDBT_ANT_DET	
	gpio_tlmm_config(GPIO_CFG(isdbt_pdata->gpio_ant_det, GPIOMUX_FUNC_GPIO,
        GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
        GPIO_CFG_ENABLE);
#endif

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

static int isdbt_thread(void *hDevice)
{
	struct ISDBT_INIT_INFO_T *hInit = (struct ISDBT_INIT_INFO_T *)hDevice;

	set_user_nice(current, -20);

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

	printk("isdbt_open. \n");
	hOpen = kmalloc(sizeof(struct ISDBT_OPEN_INFO_T), GFP_KERNEL);

//	hOpen->buf = kmalloc(RING_BUFFER_SIZE, GFP_KERNEL); //Fix for PLM P140326-05955
	hOpen->buf = &static_ringbuffer[0];
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
	//	printk("isdbt_read read 0\n");
		return 0;
	}

	if (non_blocking && (fci_ringbuffer_empty(cibuf)))	{
		/*PRINTF(hInit, "return EWOULDBLOCK\n"); */
		return -EWOULDBLOCK;
	}

	if (wait_event_interruptible(cibuf->queue,
		!fci_ringbuffer_empty(cibuf))) {
		printk("isdbt_read return ERESTARTSYS\n");
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
	/* kfree(hOpen->buf); */
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

	printk("fc8150_if_test Start!!!\n");
	for (i = 0 ; i < 100 ; i++) {
		BBM_BYTE_WRITE(0, 0xa4, i & 0xff);
		BBM_BYTE_READ(0, 0xa4, &data);
		if ((i & 0xff) != data) {
			printk("fc8150_if_btest!   i=0x%x, data=0x%x\n"
				, i & 0xff, data);
			res = 1;
		}
	}

	for (i = 0 ; i < 100 ; i++) {
		BBM_WORD_WRITE(0, 0xa4, i & 0xffff);
		BBM_WORD_READ(0, 0xa4, &wdata);
		if ((i & 0xffff) != wdata) {
			printk("fc8150_if_wtest!   i=0x%x, data=0x%x\n"
				, i & 0xffff, wdata);
			res = 1;
		}
	}

	for (i = 0 ; i < 100 ; i++) {
		BBM_LONG_WRITE(0, 0xa4, i & 0xffffffff);
		BBM_LONG_READ(0, 0xa4, &ldata);
		if ((i & 0xffffffff) != ldata) {
			printk("fc8150_if_ltest!   i=0x%x, data=0x%x\n"
				, i & 0xffffffff, ldata);
			res = 1;
		}
	}

	for (i = 0 ; i < 100 ; i++) {
		temp = i & 0xff;
		BBM_TUNER_WRITE(NULL, 0x52, 0x01, &temp, 0x01);
		BBM_TUNER_READ(NULL, 0x52, 0x01, &data, 0x01);
		if ((i & 0xff) != data)
			printk("FC8150 tuner test (0x%x,0x%x)\n"
			, i & 0xff, data);
	}

	printk("fc8150_if_test End!!!\n");

	return res;
}

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
		printk("IOCTL_ISDBT_RESET\n");
		res = BBM_RESET(hInit);
		break;
	case IOCTL_ISDBT_INIT:
		//printk("IOCTL_ISDBT_INIT\n");
		res = BBM_I2C_INIT(hInit, FCI_I2C_TYPE);
		res |= BBM_PROBE(hInit);
		if (res) {
			PRINTF(hInit, "FC8150 Initialize Fail \n");
			break;
		}
		res |= BBM_INIT(hInit);
		break;
	case IOCTL_ISDBT_BYTE_READ:
		//printk("IOCTL_ISDBT_BYTE_READ\n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_BYTE_READ(hInit, (u16)info.buff[0]
			, (u8 *)(&info.buff[1]));
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_WORD_READ:
		//printk("IOCTL_ISDBT_WORD_READ\n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_WORD_READ(hInit, (u16)info.buff[0]
			, (u16 *)(&info.buff[1]));
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_LONG_READ:
		//printk("IOCTL_ISDBT_LONG_READ\n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_LONG_READ(hInit, (u16)info.buff[0]
			, (u32 *)(&info.buff[1]));
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_BULK_READ:
		//printk("IOCTL_ISDBT_BULK_READ\n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_BULK_READ(hInit, (u16)info.buff[0]
			, (u8 *)(&info.buff[2]), info.buff[1]);
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_BYTE_WRITE:
		//printk("IOCTL_ISDBT_BYTE_WRITE\n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_BYTE_WRITE(hInit, (u16)info.buff[0]
			, (u8)info.buff[1]);
		break;
	case IOCTL_ISDBT_WORD_WRITE:
		//printk("IOCTL_ISDBT_WORD_WRITE\n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_WORD_WRITE(hInit, (u16)info.buff[0]
			, (u16)info.buff[1]);
		break;
	case IOCTL_ISDBT_LONG_WRITE:
		//printk("IOCTL_ISDBT_LONG_WRITE\n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_LONG_WRITE(hInit, (u16)info.buff[0]
			, (u32)info.buff[1]);
		break;
	case IOCTL_ISDBT_BULK_WRITE:
		//printk("IOCTL_ISDBT_BULK_WRITE\n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_BULK_WRITE(hInit, (u16)info.buff[0]
			, (u8 *)(&info.buff[2]), info.buff[1]);
		break;
	case IOCTL_ISDBT_TUNER_READ:
		printk("IOCTL_ISDBT_TUNER_READ\n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_TUNER_READ(hInit, (u8)info.buff[0]
			, (u8)info.buff[1],  (u8 *)(&info.buff[3])
			, (u8)info.buff[2]);
		err |= copy_to_user((void *)arg, (void *)&info, size);
		break;
	case IOCTL_ISDBT_TUNER_WRITE:
		printk("isdb IOCTL_ISDBT_TUNER_WRITE\n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_TUNER_WRITE(hInit, (u8)info.buff[0]
			, (u8)info.buff[1], (u8 *)(&info.buff[3])
			, (u8)info.buff[2]);
		break;
	case IOCTL_ISDBT_TUNER_SET_FREQ:
		printk("isdb IOCTL_ISDBT_TUNER_SET_FREQ\n");
		{
			u32 f_rf;
			err = copy_from_user((void *)&info, (void *)arg, size);

			f_rf = ((u32)info.buff[0] - 13) * 6000 + 473143;
			isdbt_isr_check(hInit);
			res = BBM_TUNER_SET_FREQ(hInit, f_rf);
			mutex_lock(&ringbuffer_lock);
			fci_ringbuffer_flush(&hOpen->RingBuffer);
			mutex_unlock(&ringbuffer_lock);
			BBM_WRITE(hInit, BBM_BUF_INT, 0x01);
		}
		break;
	case IOCTL_ISDBT_TUNER_SELECT:
		printk("isdb IOCTL_ISDBT_TUNER_SELECT\n");
		err = copy_from_user((void *)&info, (void *)arg, size);
		res = BBM_TUNER_SELECT(hInit, (u32)info.buff[0], 0);
		break;
	case IOCTL_ISDBT_TS_START:
		printk("isdb IOCTL_ISDBT_TS_START\n");
		hOpen->isdbttype = TS_TYPE;
		break;
	case IOCTL_ISDBT_TS_STOP:
		printk("isdb IOCTL_ISDBT_TS_STOP\n");
		hOpen->isdbttype = 0;
		break;
	case IOCTL_ISDBT_POWER_ON:
		printk("isdb ioctl IOCTL_ISDBT_POWER_ON\n");
		isdbt_hw_init();
		break;
	case IOCTL_ISDBT_POWER_OFF:
		printk("isdb ioctl IOCTL_ISDBT_POWER_OFF\n");
		isdbt_hw_deinit();
		break;
	case IOCTL_ISDBT_SCAN_STATUS:
		printk("isdb ioctl IOCTL_ISDBT_SCAN_STATUS\n");
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

#if defined(CONFIG_ISDBT_ANT_DET)
enum {
    ISDBT_ANT_OPEN = 0,
    ISDBT_ANT_CLOSE,
    ISDBT_ANT_UNKNOWN,
};
enum {
    ISDBT_ANT_DET_LOW = 0,
    ISDBT_ANT_DET_HIGH,
};


static struct input_dev *isdbt_ant_input;
static int isdbt_check_ant;
static int ant_prev_status;

#define ISDBT_ANT_CHECK_DURATION 500000 /* us */
#define ISDBT_ANT_CHECK_COUNT 2
#define ISDBT_ANT_WLOCK_TIMEOUT \
		((ISDBT_ANT_CHECK_DURATION * ISDBT_ANT_CHECK_COUNT * 2) / 1000000)
static int isdbt_ant_det_check_value(void)
{
    int loop = 0;
    int cur_val = 0, prev_val = 0;
    int ret = ISDBT_ANT_UNKNOWN;

	isdbt_check_ant = 1;

    prev_val = \
        ant_prev_status ? ISDBT_ANT_DET_LOW : ISDBT_ANT_DET_HIGH;
    for (loop = 0; loop < ISDBT_ANT_CHECK_COUNT; loop++) {
        usleep_range(ISDBT_ANT_CHECK_DURATION, ISDBT_ANT_CHECK_DURATION);
        cur_val = gpio_get_value_cansleep(isdbt_pdata->gpio_ant_det);

        if (prev_val != cur_val || ant_prev_status == cur_val)
            break;
        prev_val = cur_val;
    }

    if (loop == ISDBT_ANT_CHECK_COUNT) {
        if (ant_prev_status == ISDBT_ANT_DET_LOW
                && cur_val == ISDBT_ANT_DET_HIGH) {
            ret = ISDBT_ANT_OPEN;
        } else if (ant_prev_status == ISDBT_ANT_DET_HIGH
                && cur_val == ISDBT_ANT_DET_LOW) {
            ret = ISDBT_ANT_CLOSE;
        }

        ant_prev_status = cur_val;
    }

	isdbt_check_ant = 0;

    printk("%s cnt(%d) cur(%d) prev(%d)\n",
        __func__, loop, cur_val, ant_prev_status);

    return ret;
}

static int isdbt_ant_det_ignore_irq(void)
{
    //printk("chk_ant=%d sr=%d\n",
      //      isdbt_check_ant, system_rev);
    return isdbt_check_ant;
}
    

static void isdbt_ant_det_work_func(struct work_struct *work)
        {
 if (!isdbt_ant_input) {
        printk("%s: input device is not registered\n", __func__);
        return;
        }

    switch (isdbt_ant_det_check_value()) {
    case ISDBT_ANT_OPEN:
        input_report_key(isdbt_ant_input, KEY_DMB_ANT_DET_UP, 1);
        input_report_key(isdbt_ant_input, KEY_DMB_ANT_DET_UP, 0);
        input_sync(isdbt_ant_input);
        printk("%s : ISDBT_ANT_OPEN\n", __func__);
        break;
    case ISDBT_ANT_CLOSE:
		input_report_key(isdbt_ant_input, KEY_DMB_ANT_DET_DOWN, 1);
		input_report_key(isdbt_ant_input, KEY_DMB_ANT_DET_DOWN, 0);
		input_sync(isdbt_ant_input);
        printk("%s : ISDBT_ANT_CLOSE\n", __func__);
        break;
    case ISDBT_ANT_UNKNOWN:
        printk("%s : ISDBT_ANT_UNKNOWN\n", __func__);
        break;
    default:
        break;
	}

}

static struct workqueue_struct *isdbt_ant_det_wq;
static DECLARE_WORK(isdbt_ant_det_work, isdbt_ant_det_work_func);
static bool isdbt_ant_det_reg_input(struct platform_device *pdev)
{
	struct input_dev *input;
	int err;

	printk("%s\n", __func__);

	input = input_allocate_device();
	if (!input) {
		printk("Can't allocate input device\n");
		err = -ENOMEM;
	}
	set_bit(EV_KEY, input->evbit);
	set_bit(KEY_DMB_ANT_DET_UP & KEY_MAX, input->keybit);
	set_bit(KEY_DMB_ANT_DET_DOWN & KEY_MAX, input->keybit);
	input->name = "sec_dmb_key";
	input->phys = "sec_dmb_key/input0";
	input->dev.parent = &pdev->dev;

	err = input_register_device(input);
	if (err) {
		printk("Can't register dmb_ant_det key: %d\n", err);
		goto free_input_dev;
	}
	isdbt_ant_input = input;
        ant_prev_status = gpio_get_value_cansleep(isdbt_pdata->gpio_ant_det);

	return true;

free_input_dev:
	input_free_device(input);
	return false;
}

static void isdbt_ant_det_unreg_input(void)
{
	//printk("%s\n", __func__);
	input_unregister_device(isdbt_ant_input);
}
static bool isdbt_ant_det_create_wq(void)
{
printk("%s\n", __func__);
	isdbt_ant_det_wq = create_singlethread_workqueue("isdbt_ant_det_wq");
	if (isdbt_ant_det_wq)
		return true;
	else
		return false;
}

static bool isdbt_ant_det_destroy_wq(void)
{
	//printk("%s\n", __func__);
	if (isdbt_ant_det_wq) {
		flush_workqueue(isdbt_ant_det_wq);
		destroy_workqueue(isdbt_ant_det_wq);
		isdbt_ant_det_wq = NULL;
	}
	return true;
}

static irqreturn_t isdbt_ant_det_irq_handler(int irq, void *dev_id)
{
	

    if (isdbt_ant_det_ignore_irq())
		return IRQ_HANDLED;

	wake_lock_timeout(&isdbt_ant_wlock, ISDBT_ANT_WLOCK_TIMEOUT * HZ);

	if (isdbt_ant_det_wq) {
		int ret = 0;
		ret = queue_work(isdbt_ant_det_wq, &isdbt_ant_det_work);
		if (ret == 0)
			printk("%s queue_work fail\n", __func__);
	}

	return IRQ_HANDLED;

}

static bool isdbt_ant_det_irq_set(bool set)
{
	bool ret = true;
	
	//printk("%s\n", __func__);

	if (set) {
		int irq_ret;
		isdbt_pdata->irq_ant_det = gpio_to_irq(isdbt_pdata->gpio_ant_det);
		irq_set_irq_type(isdbt_pdata->irq_ant_det, IRQ_TYPE_EDGE_BOTH);
		irq_ret = request_irq(isdbt_pdata->irq_ant_det
						, isdbt_ant_det_irq_handler
						, IRQF_DISABLED
						, "isdbt_ant_det"
						, NULL);
		if (irq_ret < 0) {
			printk("%s %d\r\n", __func__, irq_ret);
			ret = false;
		}
	    enable_irq_wake(isdbt_pdata->irq_ant_det);
	} else {
	    disable_irq_wake(isdbt_pdata->irq_ant_det);
		free_irq(isdbt_pdata->irq_ant_det, NULL);
	}

	return ret;
}
#endif






static struct isdbt_platform_data *isdbt_populate_dt_pdata(struct device *dev)
{
	struct isdbt_platform_data *pdata;
	
	printk("%s\n", __func__);
	pdata =  devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		pr_err("%s : could not allocate memory for platform data\n", __func__);
		goto err;
	}

#ifdef CONFIG_ISDBT_ANT_DET
	pdata->gpio_ant_det = of_get_named_gpio(dev->of_node, "qcom,isdb-ant-det-gpio", 0);
	if (pdata->gpio_ant_det < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_ant_det in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_ant_det =%d\n", __func__, pdata->gpio_ant_det);
#endif

	pdata->gpio_en = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-pwr-en", 0);
	if (pdata->gpio_en < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_en in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_en =%d\n", __func__, pdata->gpio_en);
		
	pdata->gpio_rst = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-rst", 0);
	if (pdata->gpio_rst < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_rst in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_rst =%d\n", __func__, pdata->gpio_rst);
		
	pdata->gpio_int = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-irq", 0);
	if (pdata->gpio_int < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio in the gpio_int dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_int =%d\n", __func__, pdata->gpio_int);
		
	pdata->gpio_spi_do = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-spi_do", 0);
	if (pdata->gpio_spi_do < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio in the gpio_spi_do dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_spi_do =%d\n", __func__, pdata->gpio_spi_do);
		
	pdata->gpio_spi_di = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-spi_di", 0);
	if (pdata->gpio_spi_di < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio in the gpio_spi_di dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_spi_di =%d\n", __func__, pdata->gpio_spi_di);
		
	pdata->gpio_spi_cs = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-spi_cs", 0);
	if (pdata->gpio_spi_cs < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_spi_cs in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_spi_cs=%d\n", __func__, pdata->gpio_spi_cs);
		
	pdata->gpio_spi_clk = of_get_named_gpio(dev->of_node, "qcom,isdb-gpio-spi_clk", 0);
	if (pdata->gpio_spi_clk < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_spi_clk in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_spi_clk=%d\n", __func__, pdata->gpio_spi_clk);
		

	return pdata;
alloc_err:
	devm_kfree(dev, pdata);
err:
	return NULL;
}

static int isdbt_probe(struct platform_device *pdev)
{
	int res;
	printk("%s\n", __func__);

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


	
	res = request_irq(gpio_to_irq(isdbt_pdata->gpio_int), isdbt_irq
		, IRQF_DISABLED | IRQF_TRIGGER_RISING, FC8150_NAME, NULL);

	if (res < 0) {
		printk("isdbt_probe: couldn't request gpio");
		return res;
	}
		

	hInit = kmalloc(sizeof(struct ISDBT_INIT_INFO_T), GFP_KERNEL);

	res = BBM_HOSTIF_SELECT(hInit, BBM_SPI);

	if (res)
		PRINTF(hInit, "isdbt host interface select fail!\n");

         isdbt_hw_stop();

	if (!isdbt_kthread)	{
		PRINTF(hInit, "isdb kthread run\n");
		isdbt_kthread = kthread_run(isdbt_thread
			, (void *)hInit, "isdbt_thread");
	}

	INIT_LIST_HEAD(&(hInit->hHead));
#if defined(CONFIG_ISDBT_ANT_DET)	

	wake_lock_init(&isdbt_ant_wlock, WAKE_LOCK_SUSPEND, "isdbt_ant_wlock");

	if (!isdbt_ant_det_reg_input(pdev))
		goto err_reg_input;
	if (!isdbt_ant_det_create_wq())
		goto free_reg_input;
	if (!isdbt_ant_det_irq_set(true))
		goto free_ant_det_wq;

	return 0;
free_ant_det_wq:
	isdbt_ant_det_destroy_wq();
free_reg_input:
	isdbt_ant_det_unreg_input();
err_reg_input:
	return -EFAULT;
#else
	return 0;
#endif

}

static int isdbt_remove(struct platform_device *pdev)
{
        printk("ISDBT remove\n");
#if defined(CONFIG_ISDBT_ANT_DET)
	isdbt_ant_det_unreg_input();
	isdbt_ant_det_destroy_wq();
	isdbt_ant_det_irq_set(false);
	wake_lock_destroy(&isdbt_ant_wlock);
#endif
	return 0;
}

static int isdbt_suspend(struct platform_device *pdev, pm_message_t mesg)
{
       int value;
#ifdef CONFIG_ISDBT_SPMI	
	u8 reg = 0x00;
	printk("%s, Tuning ISDBT_CLK off\n", __func__);
	printk("%s Writing 0x00 to register 0x%x\n", __func__, spmi_addr);

	if(isdbt_spmi)
	{
		int rc = spmi_ext_register_writel(isdbt_spmi->spmi->ctrl, isdbt_spmi->spmi->sid, spmi_addr,&reg, 1);
		if (rc) {
				printk("%s, Unable to write from addr=%x, rc(%d)\n", __func__, spmi_addr, rc );
		}
	}
	else
		printk("%s ERROR !! isdbt_spmi is NULL !!\n", __func__);
#endif		
	
       value = gpio_get_value_cansleep(isdbt_pdata->gpio_en);
       printk("%s  value = %d\n",__func__,value);
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
	{
#ifdef CONFIG_ISDBT_FC8150_HKDI
	    .compatible = "isdb_pdata",
#else
	    .compatible = "isdb_fc8300_pdata",
#endif
	},
	{}
};



static struct platform_driver isdbt_driver = {
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
	PRINTF(hInit, "isdbt_init\n");

#ifdef CONFIG_ISDBT_SPMI
	res = spmi_driver_register(&qpnp_isdbt_clk_driver);
	if(res < 0){
			printk("Error : qpnp isdbt clk init fail : %d\n", res);
		}
	printk("qpnp isdbt clk init done \n");
	
#endif
	res = platform_driver_register(&isdbt_driver);
	if (res < 0) {
		PRINTF(hInit, "isdbt init fail : %d\n", res);
		return res;
	}
	return 0;
}


void isdbt_exit(void)
{
	PRINTF(hInit, "isdbt isdbt_exit\n");

	free_irq(gpio_to_irq(isdbt_pdata->gpio_int), NULL);
	gpio_free(isdbt_pdata->gpio_int);
	gpio_free(isdbt_pdata->gpio_rst);
	gpio_free(isdbt_pdata->gpio_en);

	kthread_stop(isdbt_kthread);
	isdbt_kthread = NULL;

	BBM_HOSTIF_DESELECT(hInit);

	isdbt_hw_deinit();
	platform_driver_unregister(&isdbt_driver);
	misc_deregister(&fc8150_misc_device);

	kfree(hInit);

#ifdef CONFIG_ISDBT_SPMI
	spmi_driver_unregister(&qpnp_isdbt_clk_driver);      
#endif
}

module_init(isdbt_init);
module_exit(isdbt_exit);

MODULE_LICENSE("Dual BSD/GPL");
