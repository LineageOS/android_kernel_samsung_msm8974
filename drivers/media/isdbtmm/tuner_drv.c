/*
*
* drivers/media/isdbtmm/tuner_drv.c
*
* MM Tuner Driver
*
* Copyright (C) (2013, Samsung Electronics)
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


/******************************************************************************
 * include
 ******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <mach/isdbtmm_pdata.h>
#include <mach/gpiomux.h>
#include <linux/of_gpio.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
#include <linux/mutex.h>
#endif

#include "tuner_drv.h"
#include "tuner.h"

#if defined(CONFIG_TMM_ANT_DET)
/* for delay(sleep) */
#include <linux/delay.h>
/* for mutex */
#include <linux/mutex.h>
/*using copy to user */
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <asm/mach/irq.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#endif



/******************************************************************************
 * data
 ******************************************************************************/
/* Mmap Address */
void *mem_p;

/* poll control */
wait_queue_head_t g_tuner_poll_wait_queue;       /* poll queue              */
spinlock_t        g_tuner_lock;                  /* spin lock               */
unsigned long     g_tuner_wakeup_flag;           /* poll wait flag          */

/* interrupt factor */
unsigned char g_tuner_intcnd_f;                  /* INTCNDD_F register      */
unsigned char g_tuner_intcnd_s;                  /* INTCNDD_S register      */
unsigned char g_tuner_intst_f;
unsigned char g_tuner_intst_s;

/* kernel_thread */
struct task_struct *g_tuner_kthread_id;          /* identifier              */
u32                 g_tuner_kthread_flag;        /* flag                    */
wait_queue_head_t   g_tuner_kthread_wait_queue;  /* wait queue              */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
/* mutex */
struct mutex g_tuner_mutex;                      /* exclusive control       */
#endif

#if defined(CONFIG_TMM_ANT_DET)
#define DPRINTK printk
static struct wake_lock isdbtmm_ant_wlock;
#endif

struct isdbtmm_platform_data *isdbtmm_pdata;


/******************************************************************************
 * function
 ******************************************************************************/
static ssize_t tuner_module_entry_read( struct file* FIle,
                                        char* Buffer,
                                        size_t Count,
                                        loff_t* OffsetPosition );
static ssize_t tuner_module_entry_write( struct file* FIle,
                                         const char* Buffer,
                                         size_t Count,
                                         loff_t* OffsetPosition );
static unsigned int tuner_module_entry_poll( struct file *file,
                                           struct poll_table_struct *poll_tbl );

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
static int tuner_module_entry_ioctl( struct inode* Inode,
                                     struct file* FIle, 
                                     unsigned int uCommand,
                                     unsigned long uArgument );
#else  /* LINUX_VERSION_CODE */ 
static long tuner_module_entry_ioctl( struct file *file,
                                      unsigned int uCommand,
                                      unsigned long uArgument );
#endif /* LINUX_VERSION_CODE */
static int tuner_module_entry_open( struct inode* Inode,
                                    struct file* FIle );
static int tuner_module_entry_close( struct inode* Inode,
                                     struct file* FIle );
static int tuner_probe( struct platform_device *pdev );
static int tuner_remove( struct platform_device *pdev );

/* entry point */
static struct file_operations TunerFileOperations = {
								.owner   = THIS_MODULE,
								.read    = tuner_module_entry_read,
								.write   = tuner_module_entry_write,
								.poll    = tuner_module_entry_poll,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
								.ioctl   = tuner_module_entry_ioctl,
#else  /* LINUX_VERSION_CODE */
 								.unlocked_ioctl = tuner_module_entry_ioctl,
#endif /* LINUX_VERSION_CODE */
								.open    = tuner_module_entry_open,
								.release = tuner_module_entry_close
													};

static const struct of_device_id isdbtmm_match_table[] = {
	{   .compatible = "isdbtmm_pdata",
	},
	{}
};
													
static struct platform_driver mmtuner_driver = {
										.probe  = tuner_probe,
										.remove = tuner_remove,
										.driver = { .name = "tmmi2c",
											        .owner = THIS_MODULE,
													.of_match_table = isdbtmm_match_table,
											      }
											   };

static struct platform_device *mmtuner_device;
static struct class 	*device_class;
/* Add Start 20121218 No_1 */
static unsigned long 	open_cnt = 0;        /* OPEN counter             */
static long   	moni_cnt = 0;		 /* Monitor counter			 */
/* Add End 20121218 No_1 */

#ifndef TUNER_CONFIG_IRQ_PC_LINUX
irqreturn_t tuner_interrupt( int irq, void *dev_id );
#else  /* TUNER_CONFIG_IRQ_PC_LINUX */
int tuner_interrupt( void );
#endif /* TUNER_CONFIG_IRQ_PC_LINUX */
/******************************************************************************
 * code area
 ******************************************************************************/
#if defined(CONFIG_TMM_ANT_DET)
enum {
	ISDBTMM_ANT_OPEN = 0,
	ISDBTMM_ANT_CLOSE,
	ISDBTMM_ANT_UNKNOWN,
};
enum {
	ISDBTMM_ANT_DET_LOW = 0,
	ISDBTMM_ANT_DET_HIGH,
};

static struct input_dev *isdbtmm_ant_input;
static int isdbtmm_check_ant;
static int ant_prev_status;

#define ISDBTMM_ANT_WAIT_INIT_TIME	500000 /* us */
#define ISDBTMM_ANT_CHECK_DURATION 50000 /* us */
#define ISDBTMM_ANT_CHECK_COUNT 10
#define ISDBTMM_ANT_WLOCK_TIMEOUT \
		((ISDBTMM_ANT_CHECK_DURATION * ISDBTMM_ANT_CHECK_COUNT * 2) / 500000)
static int isdbtmm_ant_det_check_value(void)
{
	int loop = 0, cur_val = 0;
	int ret = ISDBTMM_ANT_UNKNOWN;

	isdbtmm_check_ant = 1;

	DPRINTK("%s ant_prev_status(%d)\n",
		__func__, ant_prev_status);

	usleep_range(ISDBTMM_ANT_WAIT_INIT_TIME, ISDBTMM_ANT_WAIT_INIT_TIME); /* wait initial noise */

	for (loop = 0; loop < ISDBTMM_ANT_CHECK_COUNT; loop++) {
		usleep_range(ISDBTMM_ANT_CHECK_DURATION, ISDBTMM_ANT_CHECK_DURATION);
		cur_val = gpio_get_value_cansleep(isdbtmm_pdata->gpio_ant_det);
		if (ant_prev_status == cur_val)
			break;
	}

	if (loop == ISDBTMM_ANT_CHECK_COUNT) {
		if (ant_prev_status == ISDBTMM_ANT_DET_LOW
				&& cur_val == ISDBTMM_ANT_DET_HIGH) {
			ret = ISDBTMM_ANT_OPEN;
		} else if (ant_prev_status == ISDBTMM_ANT_DET_HIGH
				&& cur_val == ISDBTMM_ANT_DET_LOW) {
			ret = ISDBTMM_ANT_CLOSE;
		}

		ant_prev_status = cur_val;
	}

	isdbtmm_check_ant = 0;

	DPRINTK("%s cnt(%d) cur(%d) prev(%d)\n",
		__func__, loop, cur_val, ant_prev_status);

	return ret;
}

static int isdbtmm_ant_det_ignore_irq(void)
{
	DPRINTK("chk_ant=%d\n", isdbtmm_check_ant);
	return isdbtmm_check_ant;
}

static void isdbtmm_ant_det_work_func(struct work_struct *work)
{
	if (!isdbtmm_ant_input) {
		DPRINTK("%s: input device is not registered\n", __func__);
		return;
	}

	switch (isdbtmm_ant_det_check_value()) {
	case ISDBTMM_ANT_OPEN:
		input_report_key(isdbtmm_ant_input, KEY_DMB_ANT_DET_UP, 1);
		input_report_key(isdbtmm_ant_input, KEY_DMB_ANT_DET_UP, 0);
		input_sync(isdbtmm_ant_input);
		DPRINTK("%s : ISDBTMM_ANT_OPEN\n", __func__);
		break;
	case ISDBTMM_ANT_CLOSE:
		input_report_key(isdbtmm_ant_input, KEY_DMB_ANT_DET_DOWN, 1);
		input_report_key(isdbtmm_ant_input, KEY_DMB_ANT_DET_DOWN, 0);
		input_sync(isdbtmm_ant_input);
		DPRINTK("%s : ISDBTMM_ANT_CLOSE\n", __func__);
		break;
	case ISDBTMM_ANT_UNKNOWN:
		DPRINTK("%s : ISDBTMM_ANT_UNKNOWN\n", __func__);
		break;
	default:
		break;
	}
}

static struct workqueue_struct *isdbtmm_ant_det_wq;
static DECLARE_WORK(isdbtmm_ant_det_work, isdbtmm_ant_det_work_func);
static bool isdbtmm_ant_det_reg_input(struct platform_device *pdev)
{
	struct input_dev *input;
	int err;

	DPRINTK("%s\n", __func__);

	input = input_allocate_device();
	if (!input) {
		DPRINTK("Can't allocate input device\n");
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
		DPRINTK("Can't register dmb_ant_det key: %d\n", err);
		goto free_input_dev;
	}
	isdbtmm_ant_input = input;
	ant_prev_status = gpio_get_value_cansleep(isdbtmm_pdata->gpio_ant_det);

	return true;

free_input_dev:
	input_free_device(input);
	return false;
}

static void isdbtmm_ant_det_unreg_input(void)
{
	DPRINTK("%s\n", __func__);
	if (isdbtmm_ant_input) {
		input_unregister_device(isdbtmm_ant_input);
		isdbtmm_ant_input = NULL;
	}
}
static bool isdbtmm_ant_det_create_wq(void)
{
	DPRINTK("%s\n", __func__);
	isdbtmm_ant_det_wq = create_singlethread_workqueue("isdbtmm_ant_det_wq");
	if (isdbtmm_ant_det_wq)
		return true;
	else
		return false;
}

static bool isdbtmm_ant_det_destroy_wq(void)
{
	DPRINTK("%s\n", __func__);
	if (isdbtmm_ant_det_wq) {
		flush_workqueue(isdbtmm_ant_det_wq);
		destroy_workqueue(isdbtmm_ant_det_wq);
		isdbtmm_ant_det_wq = NULL;
	}
	return true;
}

static irqreturn_t isdbtmm_ant_det_irq_handler(int irq, void *dev_id)
{


DPRINTK("%s queue_work called\n", __func__);
	if (isdbtmm_ant_det_ignore_irq())
		return IRQ_HANDLED;

	wake_lock_timeout(&isdbtmm_ant_wlock, ISDBTMM_ANT_WLOCK_TIMEOUT * HZ);

	if (isdbtmm_ant_det_wq) {
		int ret = queue_work(isdbtmm_ant_det_wq, &isdbtmm_ant_det_work);
		if (ret == 0)
			DPRINTK("%s queue_work fail\n", __func__);
	}

	return IRQ_HANDLED;
}

static bool isdbtmm_ant_det_irq_set(bool set)
{
	bool ret = true;


	DPRINTK("%s set= %d\n", __func__,set);

	if (set) {
	
		int irq_ret;
			isdbtmm_pdata->irq_ant_det = gpio_to_irq(isdbtmm_pdata->gpio_ant_det);
			irq_set_irq_type(isdbtmm_pdata->irq_ant_det
					, IRQ_TYPE_EDGE_BOTH);

		irq_ret = request_irq(isdbtmm_pdata->irq_ant_det
						, isdbtmm_ant_det_irq_handler
						, IRQF_DISABLED
						, "isdbtmm_ant_det"
						, NULL);
		if (irq_ret < 0) {
			DPRINTK("Error:%s request_irq %d\r\n", __func__, irq_ret);
			ret = false;
		}
		enable_irq_wake(isdbtmm_pdata->irq_ant_det);
		DPRINTK("%s irq = %d \r\n", __func__, isdbtmm_pdata->irq_ant_det);
	} else {
		disable_irq_wake(isdbtmm_pdata->irq_ant_det);
		free_irq(isdbtmm_pdata->irq_ant_det, NULL);
	}

	return ret;
}
#endif

static struct isdbtmm_platform_data *isdbtmm_populate_dt_pdata(struct device *dev)
{
	struct isdbtmm_platform_data *pdata;
	
	printk("%s\n", __func__);
	pdata =  devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		pr_err("%s : could not allocate memory for platform data\n", __func__);
		goto err;
	}

#ifdef CONFIG_TMM_ANT_DET
	pdata->gpio_ant_det = of_get_named_gpio(dev->of_node, "qcom,isdbtmm-ant-det-gpio", 0);
	if (pdata->gpio_ant_det < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_ant_det in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_ant_det =%d\n", __func__, pdata->gpio_ant_det);
#endif

	pdata->gpio_en = of_get_named_gpio(dev->of_node, "qcom,isdbtmm-gpio-pwr-en", 0);
	if (pdata->gpio_en < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_en in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_en =%d\n", __func__, pdata->gpio_en);
		
	pdata->gpio_rst = of_get_named_gpio(dev->of_node, "qcom,isdbtmm-gpio-rst", 0);
	if (pdata->gpio_rst < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_rst in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_rst =%d\n", __func__, pdata->gpio_rst);
		
	pdata->gpio_int = of_get_named_gpio(dev->of_node, "qcom,isdbtmm-gpio-irq", 0);
	if (pdata->gpio_int < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio in the gpio_int dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_int =%d\n", __func__, pdata->gpio_int);
		
	pdata->gpio_spi_do = of_get_named_gpio(dev->of_node, "qcom,isdbtmm-gpio-spi_do", 0);
	if (pdata->gpio_spi_do < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio in the gpio_spi_do dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_spi_do =%d\n", __func__, pdata->gpio_spi_do);
		
	pdata->gpio_spi_di = of_get_named_gpio(dev->of_node, "qcom,isdbtmm-gpio-spi_di", 0);
	if (pdata->gpio_spi_di < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio in the gpio_spi_di dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_spi_di =%d\n", __func__, pdata->gpio_spi_di);
		
	pdata->gpio_spi_cs = of_get_named_gpio(dev->of_node, "qcom,isdbtmm-gpio-spi_cs", 0);
	if (pdata->gpio_spi_cs < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_spi_cs in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_spi_cs=%d\n", __func__, pdata->gpio_spi_cs);
		
	pdata->gpio_spi_clk = of_get_named_gpio(dev->of_node, "qcom,isdbtmm-gpio-spi_clk", 0);
	if (pdata->gpio_spi_clk < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_spi_clk in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_spi_clk=%d\n", __func__, pdata->gpio_spi_clk);
		
	pdata->gpio_i2c_sda = of_get_named_gpio(dev->of_node, "qcom,isdbtmm-gpio-i2c_sda", 0);
	if (pdata->gpio_i2c_sda < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_i2c_sda in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_i2c_sda=%d\n", __func__, pdata->gpio_i2c_sda);
		
	pdata->gpio_i2c_scl = of_get_named_gpio(dev->of_node, "qcom,isdbtmm-gpio-i2c_scl", 0);
	if (pdata->gpio_i2c_scl < 0) {
		pr_err("%s : can not find the isdbt-detect-gpio gpio_i2c_scl in the dt\n", __func__);
		goto alloc_err;
	} else
		pr_info("%s : isdbt-detect-gpio gpio_i2c_scl=%d\n", __func__, pdata->gpio_i2c_scl);
		
		
	return pdata;
alloc_err:
	devm_kfree(dev, pdata);
err:
	return NULL;
}




/******************************************************************************
 *    function:   tuner_probe
 *    brief   :   probe control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   pdev
 *    output  :   none
 ******************************************************************************/
static int tuner_probe(struct platform_device *pdev)
{
	int ret;
	
    printk("mmtuner_probe: Called.\n");
    /* tuner register */
    if (register_chrdev(TUNER_CONFIG_DRV_MAJOR, TUNER_CONFIG_DRIVER_NAME, 
    											&TunerFileOperations))
    {
        ERROR_PRINT("mmtuner_probe: register_chrdev()\
                     Failed Major:%d.", TUNER_CONFIG_DRV_MAJOR);
        return -1;
    }

	isdbtmm_pdata = isdbtmm_populate_dt_pdata(&pdev->dev);
	if (!isdbtmm_pdata) {
		pr_err("%s : isdbt_pdata is NULL.\n", __func__);
		return -ENODEV;
	}
	

	
    /* initialization of external variable */
    init_waitqueue_head( &g_tuner_poll_wait_queue );
    spin_lock_init( &g_tuner_lock );
    g_tuner_wakeup_flag = TUNER_OFF;
    g_tuner_intcnd_f = 0x00;
    g_tuner_intcnd_s = 0x00;
	open_cnt         = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
    mutex_init(&g_tuner_mutex);
#endif
	
    printk("tuner_probe: END.\n");

#if defined(CONFIG_TMM_ANT_DET)
	wake_lock_init(&isdbtmm_ant_wlock, WAKE_LOCK_SUSPEND, "isdbtmm_ant_wlock");



	if (!isdbtmm_ant_det_reg_input(pdev))
		goto err_reg_input;
	if (!isdbtmm_ant_det_create_wq())
		goto free_reg_input;
	if (!isdbtmm_ant_det_irq_set(true))
		goto free_ant_det_wq;
#endif


	/* NORMAL END */
	return 0;
	
#if defined(CONFIG_TMM_ANT_DET)
free_ant_det_wq:
	isdbtmm_ant_det_destroy_wq();

free_reg_input:
	isdbtmm_ant_det_unreg_input();

err_reg_input:
	ret = -EFAULT;
#endif

	/* ERROR END */
	return ret;
}

/******************************************************************************
 *    function:   tuner_remove
 *    brief   :   remove control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   pdev
 *    output  :   none
 ******************************************************************************/
static int tuner_remove(struct platform_device *pdev)
{
    printk("tuner_remove: Called.\n");
    TRACE();

    /* release of the interrupt */
    tuner_drv_release_interrupt();

    /* tuner unregister */
    unregister_chrdev(TUNER_CONFIG_DRV_MAJOR, TUNER_CONFIG_DRIVER_NAME);

    printk("tuner_remove: END.\n");

#if defined(CONFIG_TMM_ANT_DET)
	isdbtmm_ant_det_unreg_input();
	isdbtmm_ant_det_destroy_wq();
	isdbtmm_ant_det_irq_set(false);
	wake_lock_destroy(&isdbtmm_ant_wlock);
#endif

    return 0;
}


/******************************************************************************
 *    function:   tuner_kernel_thread
 *    brief   :   kernel_thread of mmtuner driver
 *    date    :   2011.09.16
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   none
 *    output  :   none
 ******************************************************************************/
int tuner_kernel_thread( void * arg )
{
    int ret = 0;
    unsigned long flags;
    unsigned long ktread_flg;
    mm_segment_t    oldfs;
    struct sched_param  param; 

    struct i2c_adapter	*adap;
    struct i2c_msg		msgs[4];
    unsigned char		buff[3];
    unsigned char		bufs[3];

    printk("tuner_kernel_thread: START.\n");

    /* initialization of internal variables */
    ret = 0;
    flags = 0;
    ktread_flg = 0;
	adap = NULL;
    param.sched_priority = TUNER_CONFIG_KTH_PRI;

    daemonize( "tuner_kthread" ); 
    oldfs = get_fs();
    set_fs( KERNEL_DS );
    ret = sched_setscheduler( g_tuner_kthread_id, SCHED_FIFO, &param );
    set_fs( oldfs );

    buff[0] = (unsigned char)TUNER_DRV_ADR_INTCND_F;
    bufs[0] = (unsigned char)TUNER_DRV_ADR_INTCND_S;

    while(1)
    {
        DEBUG_PRINT("tuner_kernel_thread waiting... ");
        wait_event_interruptible( g_tuner_kthread_wait_queue, 
        						  g_tuner_kthread_flag );

        spin_lock_irqsave( &g_tuner_lock, flags );
        ktread_flg = g_tuner_kthread_flag;
        g_tuner_kthread_flag &= ~TUNER_KTH_IRQHANDLER;
        spin_unlock_irqrestore( &g_tuner_lock, flags);

        memset( msgs, 0x00, sizeof(struct i2c_msg) * 4 );
        adap = i2c_get_adapter(TUNER_CONFIG_I2C_BUSNUM);
        if (adap == NULL) {
    	    TRACE();
    	    break;
        }

        /* get interrupt factor  */
        if ( ( ktread_flg & TUNER_KTH_IRQHANDLER ) == TUNER_KTH_IRQHANDLER )
        {
            DEBUG_PRINT("tuner_kernel_thread IRQHANDLER start ");

			buff[1] = buff[2] = 0;
			bufs[1] = bufs[2] = 0;

            /* read data */
            /* INTCND_F */
            msgs[0].addr	= TUNER_SLAVE_ADR_M1;
            msgs[0].flags	= 0;	/* write */
            msgs[0].len		= 1;
            msgs[0].buf		= &buff[0];
            msgs[1].addr	= TUNER_SLAVE_ADR_M1;
            msgs[1].flags	= I2C_M_RD;
            msgs[1].len		= 2;
            msgs[1].buf		= buff+1;
            msgs[2].addr	= TUNER_SLAVE_ADR_M2;
            msgs[2].flags	= 0;	/* write */
            msgs[2].len		= 1;
            msgs[2].buf		= &bufs[0];
            msgs[3].addr	= TUNER_SLAVE_ADR_M2;
            msgs[3].flags	= I2C_M_RD;
            msgs[3].len		= 2;
            msgs[3].buf		= bufs+1;

            ret = i2c_transfer(adap, msgs, 4);
            if (ret < 0) {
            	TRACE();
               	break;
            }
            DEBUG_PRINT(
            	"read        slv:0x%02x adr:0x%02x len:%-4d 0x%02x ... 0x%02x ",
                msgs[0].addr, *(msgs[0].buf), msgs[1].len, 
            	msgs[1].buf[0], msgs[1].buf[1]);
            DEBUG_PRINT(
            	"read        slv:0x%02x adr:0x%02x len:%-4d 0x%02x ... 0x%02x ",
				msgs[2].addr, *(msgs[2].buf), msgs[3].len, 
            	msgs[3].buf[0], msgs[3].buf[1]);

            g_tuner_intcnd_f	|= buff[1];
            g_tuner_intst_f		|= buff[2];
            g_tuner_intcnd_s	|= bufs[1];
            g_tuner_intst_s		|= bufs[2];

            DEBUG_PRINT( "// IRQ factor update: INTCND_F:0x%02x INTST_F:0x%02x"
            		,g_tuner_intcnd_f, g_tuner_intst_f );
            DEBUG_PRINT( "// IRQ factor update: INTCND_S:0x%02x INTST_S:0x%02x"
            		,g_tuner_intcnd_s, g_tuner_intst_s );
            
            memset( msgs, 0x00, sizeof(struct i2c_msg) * 4 );
            msgs[0].addr	= TUNER_SLAVE_ADR_M1;
            msgs[0].flags	= 0;	/* write */
            msgs[0].len		= 2;
            msgs[0].buf		= buff;
            msgs[1].addr	= TUNER_SLAVE_ADR_M2;
            msgs[1].flags	= 0;	/* write */
            msgs[1].len		= 2;
            msgs[1].buf		= bufs;
            ret = i2c_transfer(adap, msgs, 2);
            if (ret < 0) {
            	TRACE();
                i2c_put_adapter(adap);
                break;
            }
            i2c_put_adapter(adap);

        	/* poll wait release */
            g_tuner_wakeup_flag = TUNER_ON;
            wake_up( &g_tuner_poll_wait_queue );

            DEBUG_PRINT("tuner_interrupt end ");

#ifdef TUNER_CONFIG_IRQ_LEVELTRIGGER
            tuner_drv_enable_interrupt();
#endif /* TUNER_CONFIG_IRQ_LEVELTRIGGER */
        }

        /* thread stop set */
        if ( ( ktread_flg & TUNER_KTH_END ) == TUNER_KTH_END )
        {
            DEBUG_PRINT("tuner_kernel_thread KTH_END start ");
            spin_lock_irqsave( &g_tuner_lock, flags );
            g_tuner_kthread_flag &= ~TUNER_KTH_END;
            spin_unlock_irqrestore( &g_tuner_lock, flags );
            break;
        }
    }

    INFO_PRINT("tuner_kernel_thread: END. ");

    return 0;
}


/******************************************************************************
 *    function:   tuner_drv_start
 *    brief   :   initialization control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   none
 *    output  :   none
 ******************************************************************************/
/* Modify Start 20121218 No_3 */
#if 1
int tuner_drv_start(void)
#else
static int __init tuner_drv_start(void)
#endif
/* Modify End 20121218 No_3 */
{
    int ret =0;
    struct device *dev = NULL;

    printk("mmtuner_tuner_drv_start: Called\n");

    /* driver register */
    ret = platform_driver_register(&mmtuner_driver);

    if( ret != 0 )
    {
        ERROR_PRINT("init_module: Error:\
                     failed in platform_driver_register.");
        return ret;
    }
    
    /* device alloc */
    mmtuner_device = platform_device_alloc(TUNER_CONFIG_DRIVER_NAME, -1);

    if (!mmtuner_device)
    {
        ERROR_PRINT("init_module: Error: failed in platform_device_alloc.");
        platform_driver_unregister(&mmtuner_driver);
        return -ENOMEM;
    }
    
    /* device add */
    ret = platform_device_add(mmtuner_device);
    if ( ret )
    {
        ERROR_PRINT("init_module: Error: failed in platform_device_add.");
        platform_device_put(mmtuner_device);
        platform_driver_unregister(&mmtuner_driver);
        return ret;
    }

	device_class = class_create(THIS_MODULE, TUNER_CONFIG_DRIVER_NAME);
    if (IS_ERR(device_class)) 
    {
        ERROR_PRINT("init_module: Error: failed in class_create.");
        platform_device_put(mmtuner_device);
        platform_driver_unregister(&mmtuner_driver);
        return PTR_ERR(device_class);
    }

    dev = device_create ( device_class, 
    					  NULL, 
    					  MKDEV(TUNER_CONFIG_DRV_MAJOR, 
    					  TUNER_CONFIG_DRV_MINOR), 
    					  NULL, 
    					  TUNER_CONFIG_DRIVER_NAME);

    if(IS_ERR(dev))
    {
        ERROR_PRINT("init_module: Error: failed in device_create.");
        platform_device_put(mmtuner_device);
        platform_driver_unregister(&mmtuner_driver);
        return PTR_ERR(dev);
    }
   
    /* thread creat */
    g_tuner_kthread_flag = TUNER_KTH_NONE;

    init_waitqueue_head( &g_tuner_kthread_wait_queue );

    g_tuner_kthread_id = kthread_create( tuner_kernel_thread,
                                         NULL,
                                         "tuner_kthread" );
    if( IS_ERR( g_tuner_kthread_id ) )
    {
        g_tuner_kthread_id = NULL;
        platform_device_put(mmtuner_device);
        platform_driver_unregister(&mmtuner_driver);
        return -EIO;
    }

    wake_up_process( g_tuner_kthread_id );

    printk("mmtuner_tuner_drv_start: END\n");
    return 0;
}

/******************************************************************************
 *    function:   tuner_drv_end
 *    brief   :   exit control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 ******************************************************************************/
/* Modify Start 20121218 No_3 */
#if 1
void tuner_drv_end(void)
#else
static void __exit tuner_drv_end(void)
#endif
/* Modify End 20121218 No_3 */
{
    printk("mmtuner_tuner_drv_end: Called\n");

    /* thread stop flag */
    g_tuner_kthread_flag |= TUNER_KTH_END;
    if( waitqueue_active( &g_tuner_kthread_wait_queue ))
    {
        wake_up( &g_tuner_kthread_wait_queue );
    }

    /* thread stop */
    if( g_tuner_kthread_id )
    {
        kthread_stop( g_tuner_kthread_id );
    }

    /* device destroy */
    device_destroy(device_class, MKDEV(TUNER_CONFIG_DRV_MAJOR, 
    								   TUNER_CONFIG_DRV_MINOR));
    /* class destroy */
    class_destroy(device_class);
    /* device unregister */
    platform_device_unregister(mmtuner_device);
    /* driver unregister */
    platform_driver_unregister(&mmtuner_driver);

    printk("mmtuner_tuner_drv_end: END\n");
}

/******************************************************************************
 *    function:   tuner_module_entry_open
 *    brief   :   open control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   Inode
 *            :   FIle
 *    output  :   none
 ******************************************************************************/
static int tuner_module_entry_open(struct inode* Inode, struct file* FIle)
{

    printk("tuner_module_entry_open: Called\n");

#ifdef  TUNER_CONFIG_DRV_MULTI      /* allow multiple open */
    open_cnt++;
#else  /* TUNER_CONFIG_DRV_MULTI */ /* not allow multiple open */
	/* already open */
    if( open_cnt > 0 )
    {
        printk("tuner_module_entry_open: open error\n");
        return -1;
    }
	/* first open */
    else
    {
        printk("tuner_module_entry_open: open_cnt = 1\n");
        open_cnt++;
    }
#endif /* TUNER_CONFIG_DRV_MULTI */
    INFO_PRINT("tuner_module_entry_open: end << open cnt = %ld >>", 
    			open_cnt );

    return 0;
}

/******************************************************************************
 *    function:   tuner_module_entry_close
 *    brief   :   close control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   Inode
 *            :   FIle
 *    output  :   none
 ******************************************************************************/
static int tuner_module_entry_close(struct inode* Inode, struct file* FIle)
{
    struct devone_data *dev;



    printk("tuner_module_entry_close: Called\n");

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
        /* interrupt release */
        tuner_drv_release_interrupt();
    
        if( FIle == NULL )
        {
            return -1;
        }

        dev = FIle->private_data;

        if( dev )
        {
            kfree( dev );
        }
	}

    return 0;

}

/******************************************************************************
 *    function:   tuner_module_entry_read
 *    brief   :   read control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   FIle
 *            :   Buffer
 *            :   Count
 *            :   OffsetPosition
 *    output  :   none
 ******************************************************************************/
static ssize_t tuner_module_entry_read(struct file * FIle, char * Buffer,
                                 size_t Count, loff_t * OffsetPosition)
{
    return 0;

}

/******************************************************************************
 *    function:   tuner_module_entry_write
 *    brief   :   write control of a driver
 *    date    :   2011.10.31
 *    author  :   K.Okawa(KXD14)
 *
 *    return  :    0		normal exit
 *            :   -1		error exit
 *    input   :   FIle
 *            :   Buffer	[slave][reg.addr][data-0][data-1]...[data-(n-1)]
 *            :   Count		(>=3) n+2
 *            :   OffsetPosition
 *    output  :   none
 ******************************************************************************/
static ssize_t tuner_module_entry_write(struct file* FIle,
		const char* Buffer, size_t Count, loff_t* OffsetPosition)
{
    int				ret;
    unsigned long	copy_ret;
    unsigned char	*buf;				/* pointer to data area */

    struct i2c_adapter	*adap;
    struct i2c_msg		msgs[1];

    /* argument check */
    if (Count < 3) {
    	TRACE();
    	return -EINVAL;
    }

    /* memory allocation for data area */
    buf = (unsigned char *)vmalloc(Count);
    if (buf == NULL) {
        return -EINVAL;
    }

    copy_ret = copy_from_user(buf, Buffer, Count);
    if (copy_ret != 0) {
        vfree(buf);
        return -EINVAL;
    }

    /* get i2c adapter */
    adap = i2c_get_adapter(TUNER_CONFIG_I2C_BUSNUM);
    if (adap == NULL) {
    	TRACE();
    	vfree(buf);
    	return -EINVAL;
    }

    /* construct i2c message */
    memset(msgs, 0x00, sizeof(struct i2c_msg) * 1);

    msgs[0].addr	= buf[0];
    msgs[0].flags	= 0;		/* write */
    msgs[0].len		= Count - 1;
    msgs[0].buf		= buf + 1;

    ret = i2c_transfer(adap, msgs, 1);
    if (ret < 0) {
    	TRACE();
    	i2c_put_adapter(adap);
    	vfree(buf);
    	return -EINVAL;
    }
    DEBUG_PRINT(
    		"write        slv:0x%02x adr:0x%02x len:%-4d 0x%02x ... 0x%02x ",
    		buf[0], buf[1], Count-2, buf[2], buf[Count-1]);

    vfree(buf);
    return ret;
}

/* Add Start 20121218 No_1 */
/******************************************************************************
 *    function:   SMT113J_IOCTL_GET_OPEN_COUNT
 *    brief   :   ioctl control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   Inode
 *            :   FIle
 *            :   uCommand
 *            :   uArgument
 *    output  :   none
 ******************************************************************************/
static int SMT113J_IOCTL_GET_OPEN_COUNT ( struct file*  FIle, 
										  unsigned int cmd, 
										  unsigned long arg )
{
	TUNER_STS_DATA *arg_data;
    int				ret = 0;
	unsigned long 	temp_open = 0,
				  	temp_moni = 0;
	
	DEBUG_PRINT ("SMT113J_IOCTL_GET_OPEN_COUNT << Start >> : open = %ld", 
				 ( open_cnt - moni_cnt ));

	/* Parameter check */
	arg_data = (TUNER_STS_DATA*)arg;
	
	if ( NULL == arg_data )
	{
		ERROR_PRINT ("Parameter Error : arg = NULL");
		return ( -1 );
	}
	/* state check */
	if ( open_cnt < moni_cnt ) 
	{
		ERROR_PRINT ("-> Status Error : open = %ld, moni = %ld", 
					 open_cnt, moni_cnt );
		return ( -1 );
	}
	temp_open = (open_cnt - moni_cnt);
	temp_moni = moni_cnt;
	
	/* Copy to User Area */
	ret = put_user ( temp_open, (unsigned long __user *)&(arg_data->open_cnt) );
	
	if ( 0 != ret )
	{
		ERROR_PRINT ("-> put_user(arg_data->open_cnt) Error : ret = %d", ret );
		return ( -1 );
	}
	
	/* Copy to User Area */
	ret = put_user ( moni_cnt, (unsigned long __user *)&(arg_data->moni_cnt) );
	if ( 0 != ret )
	{
		ERROR_PRINT ("-> put_user(arg_data->moni_cnt) Error : ret = %d", ret );
		return ( -1 );
	}	
	
	DEBUG_PRINT ("SMT113J_IOCTL_GET_OPEN_COUNT << End >>");
	DEBUG_PRINT ("-> Open Count Result    : %ld", open_cnt );
	DEBUG_PRINT ("-> Monitor Count Result : %ld", moni_cnt );
	
	return ( 0 );
}

/******************************************************************************
 *    function:   SMT113J_IOCTL_SET_MONITOR_MODE
 *    brief   :   ioctl control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   Inode
 *            :   FIle
 *            :   uCommand
 *            :   uArgument
 *    output  :   none
 ******************************************************************************/
static int SMT113J_IOCTL_SET_MONITOR_MODE ( struct file* FIle, 
											unsigned int cmd, 
											unsigned long arg )
{
	int ret = 0;
	
	DEBUG_PRINT ("SMT113J_IOCTL_SET_MONITOR_MODE << Start >> ");
	
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
			DEBUG_PRINT (" -> under counter = %ld => 0", moni_cnt );
			moni_cnt = 0;
		}
	}
	DEBUG_PRINT ("SMT113J_IOCTL_SET_MONITOR_MODE << End >> : moni_cnt = %ld", 
				 moni_cnt );

	return ( ret );
}
/* Add End 20121218 No_1 */

/******************************************************************************
 *    function:   tuner_module_entry_ioctl
 *    brief   :   ioctl control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   Inode
 *            :   FIle
 *            :   uCommand
 *            :   uArgument
 *    output  :   none
 ******************************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
static int tuner_module_entry_ioctl(struct inode* Inode, struct file* FIle,
                              unsigned int uCommand, unsigned long uArgument)
#else  /* LINUX_VERSION_CODE */
static long tuner_module_entry_ioctl(struct file *file,
                              unsigned int uCommand, unsigned long uArgument)
#endif /* LINUX_VERSION_CODE */

{
    int                   ret;
    TUNER_DATA_RW         data;
    unsigned long         copy_ret;
    int                   param;
    TUNER_DATA_RW         event_status[ TUNER_EVENT_REGNUM ];

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
    /* get lock */
    mutex_lock(&g_tuner_mutex);
#endif

    /* argument check */
    if( uArgument == 0 )
    {
        TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
        /* lock release */
        mutex_unlock(&g_tuner_mutex);
#endif  
        return -EINVAL;
    }
    
    switch( uCommand )
    {
        /* read data */
        case TUNER_IOCTL_VALGET:
            copy_ret = copy_from_user( &data,
                                       &( *(TUNER_DATA_RW *)uArgument ),
                                       sizeof( TUNER_DATA_RW ));
            if( copy_ret != 0 )
            {
                TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* lock release */
                mutex_unlock(&g_tuner_mutex);
#endif  
                return -EINVAL;
            }

            ret = tuner_drv_hw_access( uCommand, &data, 1 );

            if( ret == 0 )
            {
                copy_ret = copy_to_user( &( *(TUNER_DATA_RW *)uArgument ),
                                         &data,
                                         sizeof( TUNER_DATA_RW ));
                if( copy_ret != 0 )
                {
                    TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                    /* lock release */
                    mutex_unlock(&g_tuner_mutex);
#endif  
                    return -EINVAL;
                }
            }

            break;


        /* write data */
        case TUNER_IOCTL_VALSET:
            copy_ret = copy_from_user( &data,
                                       &( *(TUNER_DATA_RW *)uArgument ),
                                       sizeof( TUNER_DATA_RW ));
            if( copy_ret != 0 )
            {
                TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* lock release */
                mutex_unlock(&g_tuner_mutex);
#endif  
                return -EINVAL;
            }

            ret = tuner_drv_hw_access( uCommand, &data, 1 );
            break;

        case TUNER_IOCTL_VALGET_EVENT:
            /* INTCND_F */
            copy_ret = copy_to_user( &( *( unsigned char *)uArgument ),
                                     &g_tuner_intcnd_f,
                                     sizeof( unsigned char ));
            if (copy_ret != 0) {
                TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* lock release */
                mutex_unlock(&g_tuner_mutex);
#endif  
                return -EINVAL;
            }
            /* INTCND_S */
            copy_ret = copy_to_user( &( *( unsigned char *)( uArgument + 1 )),
                                     &g_tuner_intcnd_s,
                                     sizeof( unsigned char ));
            if (copy_ret != 0) {
                TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* lock release */
                mutex_unlock(&g_tuner_mutex);
#endif
                return -EINVAL;
            }
            /* INTST_F */
            copy_ret = copy_to_user( &( *( unsigned char *)(uArgument + 2)),
                                     &g_tuner_intst_f,
                                     sizeof( unsigned char ));
            if (copy_ret != 0) {
                TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* lock release */
                mutex_unlock(&g_tuner_mutex);
#endif
                return -EINVAL;
            }
            /* INTCND_F */
            copy_ret = copy_to_user( &( *( unsigned char *)(uArgument + 3)),
                                     &g_tuner_intst_s,
                                     sizeof( unsigned char ));
            if (copy_ret != 0) {
                TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* lock release */
                mutex_unlock(&g_tuner_mutex);
#endif  
                return -EINVAL;
            }

            DEBUG_PRINT( "// IRQ factor send: INTCND_F:0x%02x INTST_F:0x%02x"
            		,g_tuner_intcnd_f, g_tuner_intst_f );
            DEBUG_PRINT( "// IRQ factor send: INTCND_S:0x%02x INTST_S:0x%02x"
            		,g_tuner_intcnd_s, g_tuner_intst_s );

            /* initialization */
            g_tuner_intcnd_f = 0x00;
            g_tuner_intcnd_s = 0x00;
            g_tuner_intst_f = 0x00;
            g_tuner_intst_s = 0x00;

            ret = copy_ret;

            break;
        /* event set */
        case TUNER_IOCTL_VALSET_EVENT:
        	DEBUG_PRINT("*** VALSET_EVENT ***");
            copy_ret = copy_from_user( &data,
                                       &( *(TUNER_DATA_RW *)uArgument ),
                                       sizeof( TUNER_DATA_RW ));
            if( copy_ret != 0 )
            {
                TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* lock release */
                mutex_unlock(&g_tuner_mutex);
#endif  
                return -EINVAL;
            }

            /* when 1st time of event setting, be enable interrupt */
    		/* slave address */
            event_status[0].slave_adr = TUNER_SLAVE_ADR_M1;
    		/* reg. address  */
            event_status[0].adr       = REG_INTDEF1_F;
    		/* start bit position */
            event_status[0].sbit      = SIG_ENS_INTDEF1_F;
    		/* end bit position */
            event_status[0].ebit      = SIG_ENE_INTDEF1_F;
    		/* clear for read */
            event_status[0].param     = 0x00;
    		/* enable bit mask */
            event_status[0].enabit    = SIG_ENA_INTDEF1_F;
            event_status[1].slave_adr = TUNER_SLAVE_ADR_M1;
            event_status[1].adr       = REG_INTDEF2_F;
            event_status[1].sbit      = SIG_ENS_INTDEF2_F;
            event_status[1].ebit      = SIG_ENE_INTDEF2_F;
            event_status[1].param     = 0x00;
            event_status[1].enabit    = SIG_ENA_INTDEF2_F;
            event_status[2].slave_adr = TUNER_SLAVE_ADR_M2;
            event_status[2].adr       = REG_INTDEF1_S;
            event_status[2].sbit      = SIG_ENS_INTDEF1_S;
            event_status[2].ebit      = SIG_ENE_INTDEF1_S;
            event_status[2].param     = 0x00;
            event_status[2].enabit    = SIG_ENA_INTDEF1_S;
            event_status[3].slave_adr = TUNER_SLAVE_ADR_M2;
            event_status[3].adr       = REG_INTDEF2_S;
            event_status[3].sbit      = SIG_ENS_INTDEF2_S;
            event_status[3].ebit      = SIG_ENE_INTDEF2_S;
            event_status[3].param     = 0x00;
            event_status[3].enabit    = SIG_ENA_INTDEF2_S;

            ret = tuner_drv_hw_access( TUNER_IOCTL_VALGET, 
            						   event_status, 
            						   TUNER_EVENT_REGNUM );
            
            if( ret != 0 )
            {
                TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* lock release */
                mutex_unlock(&g_tuner_mutex);
#endif
                return -EINVAL;  
            }

            if (((event_status[0].param & event_status[0].enabit) == 0x00) &&
            	((event_status[1].param & event_status[1].enabit) == 0x00) &&
                ((event_status[2].param & event_status[2].enabit) == 0x00) &&
                ((event_status[3].param & event_status[3].enabit) == 0x00))
            {
            	DEBUG_PRINT("*** REQUEST IRQ ***");
                ret = tuner_drv_set_interrupt();
                
                if( ret != 0 )
                {
                    TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* lock release */
                mutex_unlock(&g_tuner_mutex);
#endif
                return -EINVAL;  
                }
            }

            ret = tuner_drv_hw_access( TUNER_IOCTL_VALSET, &data, 1 );

            break;
        /* event release */
        case TUNER_IOCTL_VALREL_EVENT:
            copy_ret = copy_from_user( &data,
                                       &( *(TUNER_DATA_RW *)uArgument ),
                                       sizeof( TUNER_DATA_RW ));
            if( copy_ret != 0 )
            {
                TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* lock release */
                mutex_unlock(&g_tuner_mutex);
#endif  
                return -EINVAL;
            }

            ret = tuner_drv_hw_access( TUNER_IOCTL_VALSET, &data, 1 );

            if( ret != 0 )
            {
                TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* lock release */
                mutex_unlock(&g_tuner_mutex);
#endif  
                return -EINVAL;
            }
    		/* slave address */
            event_status[0].slave_adr = TUNER_SLAVE_ADR_M1;
    		/* reg. address  */
            event_status[0].adr       = REG_INTDEF1_F;
    		/* start bit position */
            event_status[0].sbit      = SIG_ENS_INTDEF1_F;
    		/* end bit position */
            event_status[0].ebit      = SIG_ENE_INTDEF1_F;
    		/* clear for read */
            event_status[0].param     = 0x00;
    		/* enable bit mask */
    		event_status[0].enabit    = SIG_ENA_INTDEF1_F;
            event_status[1].slave_adr = TUNER_SLAVE_ADR_M1;
            event_status[1].adr       = REG_INTDEF2_F;
            event_status[1].sbit      = SIG_ENS_INTDEF2_F;
            event_status[1].ebit      = SIG_ENE_INTDEF2_F;
            event_status[1].param     = 0x00;
            event_status[1].enabit    = SIG_ENA_INTDEF2_F;
            event_status[2].slave_adr = TUNER_SLAVE_ADR_M2;
            event_status[2].adr       = REG_INTDEF1_S;
            event_status[2].sbit      = SIG_ENS_INTDEF1_S;
            event_status[2].ebit      = SIG_ENE_INTDEF1_S;
            event_status[2].param     = 0x00;
            event_status[2].enabit    = SIG_ENA_INTDEF1_S;
            event_status[3].slave_adr = TUNER_SLAVE_ADR_M2;
            event_status[3].adr       = REG_INTDEF2_S;
            event_status[3].sbit      = SIG_ENS_INTDEF2_S;
            event_status[3].ebit      = SIG_ENE_INTDEF2_S;
            event_status[3].param     = 0x00;
            event_status[3].enabit    = SIG_ENA_INTDEF2_S;

            ret = tuner_drv_hw_access( TUNER_IOCTL_VALGET, 
            						   event_status, 
            						   TUNER_EVENT_REGNUM );

            if (((event_status[0].param & event_status[0].enabit) == 0x00) &&
                ((event_status[1].param & event_status[1].enabit) == 0x00) &&
                ((event_status[2].param & event_status[2].enabit) == 0x00) &&
                ((event_status[3].param & event_status[3].enabit) == 0x00))
            {
            	DEBUG_PRINT("*** release IRQ REQUEST ***");
                tuner_drv_release_interrupt();
            }

            break;
        case TUNER_IOCTL_VALSET_POWER:
            copy_ret = copy_from_user( &param,
                                       &( *( int * )uArgument ),
                                       sizeof( int ));
            if( copy_ret != 0 )
            {
                TRACE();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
                /* lock release */
                mutex_unlock(&g_tuner_mutex);
#endif  
                return -EINVAL;
            }
            ret = tuner_drv_ctl_power( param );

            break;

/* Add Start 20121218 No_1 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
        case TUNER_IOCTL_VALGET_OPENCNT:
			ret = SMT113J_IOCTL_GET_OPEN_COUNT ( FIle->private_data, 
												 uCommand, 
												 uArgument );
			break;
    	
        case TUNER_IOCTL_VALSET_MONICNT:
			ret = SMT113J_IOCTL_SET_MONITOR_MODE ( FIle->private_data, 
												   uCommand, 
												   uArgument );
			break;
#else  /* LINUX_VERSION_CODE */
        case TUNER_IOCTL_VALGET_OPENCNT:
			ret = SMT113J_IOCTL_GET_OPEN_COUNT ( file->private_data, 
												 uCommand, 
												 uArgument );
			break;
    	
        case TUNER_IOCTL_VALSET_MONICNT:
			ret = SMT113J_IOCTL_SET_MONITOR_MODE ( file->private_data, 
												   uCommand, 
												   uArgument );
			break;
#endif /* LINUX_VERSION_CODE */
/* Add End 20121218 No_1 */
    	
    	default:
            TRACE();
            ret = -EINVAL;
            break;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
    /* lock release */
    mutex_unlock(&g_tuner_mutex);
#endif
	
    return ret;
}
/******************************************************************************
 *    function:   tuner_module_entry_poll
 *    brief   :   poll control of a driver
 *    date    :   2011.08.23
 *    author  :   M.Takahashi(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   file
 *            :   poll_tbl
 *    output  :   none
 ******************************************************************************/
static unsigned int tuner_module_entry_poll(
                        struct file *file,
                        struct poll_table_struct *poll_tbl )
{
    unsigned long tuner_flags;
    unsigned int  tuner_mask;


    /* Initialization */
    tuner_mask = 0;

    /* poll wait */
    poll_wait( file, &g_tuner_poll_wait_queue, poll_tbl );

    /* interrupt disable */
    spin_lock_irqsave( &g_tuner_lock, tuner_flags );

    if( g_tuner_wakeup_flag == TUNER_ON )
    {
        tuner_mask = ( POLLIN | POLLRDNORM );
    }
    
    g_tuner_wakeup_flag = TUNER_OFF;

    /* interrupt enable */
    spin_unlock_irqrestore( &g_tuner_lock, tuner_flags );

    return tuner_mask;
}

/******************************************************************************
 *    function:   tuner_interrupt
 *    brief   :   interrpu control of a driver
 *    date    :   2011.08.23
 *    author  :   M.Takahashi(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   irq
 *            :   dev_id
 *    output  :   none
 ******************************************************************************/
#ifndef TUNER_CONFIG_IRQ_PC_LINUX
irqreturn_t tuner_interrupt( int irq, void *dev_id )
#else  /* TUNER_CONFIG_IRQ_PC_LINUX */
int tuner_interrupt( void )
#endif /* TUNER_CONFIG_IRQ_PC_LINUX */
{
    DEBUG_PRINT("tuner_interrupt start ");

    g_tuner_kthread_flag |= TUNER_KTH_IRQHANDLER;
    if( waitqueue_active( &g_tuner_kthread_wait_queue ))
    {
#ifdef TUNER_CONFIG_IRQ_LEVELTRIGGER
        tuner_drv_disable_interrupt();
#endif /* TUNER_CONFIG_IRQ_LEVELTRIGGER */
        wake_up( &g_tuner_kthread_wait_queue );
    }
    else
    {
        DEBUG_PRINT("tuner_interrupt waitqueue_active err!!! ");
    }

    DEBUG_PRINT("tuner_interrupt end ");

    return IRQ_HANDLED;
}

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("MM Tuner Driver");
MODULE_LICENSE("GPL v2");
