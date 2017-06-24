/*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*/

#ifdef CONFIG_FELICA
#include "felica.h"
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/termios.h>
#include <linux/serial_core.h>
#include <linux/uaccess.h>

#ifdef F_WAKE_LOCK
#include <linux/wakelock.h>
#endif

#include <linux/types.h>

/* jmodel */
#if defined(CONFIG_ARCH_EXYNOS)
#include <mach/smc.h>
#elif defined(CONFIG_ARCH_APQ8064) || defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#include <mach/scm.h>
#endif

#include <asm/system_info.h>
#include <linux/of_gpio.h>
#include <linux/mfd/pm8xxx/gpio.h>


/******************************************************************************
 * log
 ******************************************************************************/
#include <mach/sec_debug.h>
#define NO_CHECK_TAMPER
#define SRIB_DIAG_ENABLED

/******************************************************************************
 * Board configuration
 ******************************************************************************/
/* jmodel */
#if defined(CONFIG_ARCH_EXYNOS)
#define  FELICA_UART1RX        EXYNOS5410_GPA0(4)
#endif

/* Hmodel */
#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if !defined(CONFIG_MACH_KLTE_KDI) && !defined(CONFIG_MACH_KLTE_DCM) && !defined(CONFIG_MACH_KLTE_SBM)
static int gfelica_irq_int_pin = -1;
#endif
static struct platform_device *felica_gpio_pdev;

#if defined(CONFIG_MACH_HLTEDCM)
static int g_uicc_initrev = 7;		//HW Rev 0.9
static int gfelica_sps_pin = 130;	//Select Power Supply
static int gfelica_hsel_pin = GPIO_PINID_NFC_HSEL;
#define HW_REV09_OR_10		7	// to fix hltedcm i2c h/w issue in REV 09/10.
extern void of_sii8240_hw_poweron(bool enable);  // to fix hltedcm i2c h/w issue in REV 09/10.  Defined in driver/video/msm/mhl_v2/sii8240.c
/* K MODEL */
#elif defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
static int gfelica_sps_pin = -1;	//Select Power Supply
static int gfelica_pon_pin = GPIO_FELICA_PON_REV06;	// PON
static int gfelica_hsel_pin = -1;
static int g_uicc_initrev = 0;
//#define NO_CHECK_TAMPER
//#define SRIB_DIAG_ENABLED
#elif defined(CONFIG_MACH_KLTE_KDI)
static int gfelica_pon_pin = GPIO_FELICA_PON_REV06;	// PON
static int gfelica_hsel_pin = -1;
static int gfelica_sps_pin = -1;
static int g_uicc_initrev = 0;
static int gfelica_uim_mon = 0;
//#define NO_CHECK_TAMPER
//#define SRIB_DIAG_ENABLED
#elif defined(CONFIG_MACH_HLTEKDI)
static int g_uicc_initrev = 4;
static int gfelica_sps_pin = -1;
static int gfelica_hsel_pin = GPIO_PINID_NFC_HSEL;
#elif defined(CONFIG_MACH_JS01LTEDCM)
static int g_uicc_initrev = 8;      // HW Rev 0.4
static int gfelica_sps_pin = 130;	// Select Power Supply
static int gfelica_hsel_pin = GPIO_PINID_NFC_HSEL;
#endif
#endif /* CONFIG_ARCH_MSM8974 */

/******************************************************************************
 * global variable
 ******************************************************************************/
static struct class *felica_class;

#ifdef F_WAKE_LOCK
struct wake_lock felica_wake_1;
struct wake_lock felica_wake_2;
static int tmout_1 = 3*1000; // wake lock timeout for INT irq.
#endif

/* storages for communicate to netlink */
static int gfa_open_cnt;
static int gfa_pid;
static int gfa_connect_flag;
static struct sock *gfanl_sk;
static char gfa_send_str[FELICA_NL_MSG_SIZE];
static char gfa_rcv_str[FELICA_NL_MSG_SIZE];
static int gfa_wait_flag;

/* R/W functions availability information storage */
static char gfelica_rw_status;

/* IRQ data storage for INT terminal monitoring */
struct felica_int_irqdata {
	struct delayed_work work;
	wait_queue_head_t read_wait;
	int irq_done;
	int open_flag;
};
static struct felica_int_irqdata gint_irq;
static struct felica_int_irqdata *pgint_irq = &gint_irq;

/* storages for access restriction */
static uid_t gmfc_uid = -1;
static uid_t gmfl_uid = -1;
static uid_t grwm_uid = -1;
static uid_t gdiag_uid = -1;
#ifdef CONFIG_NFC_FELICA
static uid_t gnfc_uid = -1;
struct file *pg_tty;
static int gnfc_open_cnt;
#endif /* CONFIG_NFC_FELICA */
/* package name's storage for access restriction */
static char gdiag_name[DIAG_NAME_MAXSIZE + 1];
#ifdef FELICA_UICC_FUNCTION
static char gproc_name[PROC_NAME_MAXSIZE + 1];
#endif
static uid_t gant_uid = -1;
static int gi2c_address;
static char gi2c_antaddress;
static char gi2c_lockaddress;
static struct i2c_msg gread_msgs[] = {
	{
		.addr	= 0,
		.flags	= 0,
		.len	= 0,
		.buf	= NULL,
	},
	{
		.addr	= 0,
		.flags	= 0,
		.len	= 0,
		.buf	= NULL,
	},
};

static struct i2c_msg gwrite_msgs[] = {
	{
		.addr	= 0,
		.flags	= 0,
		.len	= 0,
		.buf	= NULL,
	},
};

#ifdef CONFIG_NFC_FELICA
/******************************************************************************
 * Add global variable
 ******************************************************************************/
struct icc_poll_data {
	wait_queue_head_t read_wait;
	wait_queue_head_t rsp_wait;
	wait_queue_head_t dummy_wait;

	int handler_done;
	int rsp_done;
	struct delayed_work work;
	int device_status;
	int read_error;
	int open_flag;
	int	available_flag;
};

struct poll_data {
	wait_queue_head_t read_wait;
	int irq_handler_done;
	struct delayed_work work;
	int device_status;
	int read_error;
	int open_flag;
};

static struct icc_poll_data gfelica_poll_data;
static struct icc_poll_data g_available_data;
static struct icc_poll_data *available_d = &g_available_data;

static int guartcc_start_req = UARTCC_NFC_START_ENDPROC;
#ifdef FELICA_UICC_FUNCTION
static int guartcc_felica_status = UARTCC_FELICA_STATAUS_IN_INIT;
#else
static int guartcc_felica_status = UARTCC_FELICA_STATAUS_IDLE;
#endif
static int	g_cen_sts = 0;
static int	g_rfs_sts = 0;
static char	g_uicc_sts = 0x00;		// permissive mode issue

static int felica_varying_gpio_intu;
#ifdef P2P_FPGA_ALWAYS_ON
extern void set_fpga_felica_flag(int on);
#endif /* P2P_FPGA_ALWAYS_ON */
#endif /* CONFIG_NFC_FELICA */


/******************************************************************************
 * /dev/felica
 ******************************************************************************/

/* character device definition */
static int felica_uart_port;

static dev_t devid_felica_uart;
static struct cdev cdev_felica_uart;
static const struct file_operations fops_felica_uart = {
	.owner = THIS_MODULE,
	.open = felica_uart_open,
	.release = felica_uart_close,
	.read = felica_uart_read,
	.write = felica_uart_write,
	.fsync = felica_uart_sync,
	.unlocked_ioctl = felica_uart_ioctl,
};
struct felica_sem_data {
	struct semaphore felica_sem;
};
static struct felica_sem_data *dev_sem;

/*
 * initialize device
 */
static void felica_uart_init(void)
{
	int ret;
	struct device *device_felica_uart;
	FELICA_PR_DBG(" %s START", __func__);

	devid_felica_uart = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_uart, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_UART_NAME);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_uart, &fops_felica_uart);
	ret =
	    cdev_add(&cdev_felica_uart, devid_felica_uart, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_uart, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_uart =
	    device_create(felica_class, NULL, devid_felica_uart, NULL,
			  FELICA_UART_NAME);
	if (IS_ERR(device_felica_uart)) {
		cdev_del(&cdev_felica_uart);
		unregister_chrdev_region(devid_felica_uart, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)", __func__);
		return;
	}

	dev_sem = kmalloc(sizeof(struct felica_sem_data), GFP_KERNEL);
	if (!dev_sem) {
		cdev_del(&cdev_felica_uart);
		unregister_chrdev_region(devid_felica_uart, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(dev_sem malloc)", __func__);
		return;
	}
	sema_init(&dev_sem->felica_sem, 1);

#ifdef CONFIG_NFC_FELICA
	memset((void *)&gfelica_poll_data, 0x00, sizeof(struct icc_poll_data));
	init_waitqueue_head(&gfelica_poll_data.rsp_wait);
	gfelica_poll_data.rsp_done = 0;
	gfelica_poll_data.open_flag = 0;
#endif /* CONFIG_NFC_FELICA */

	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_uart), MINOR(devid_felica_uart));
}

/*
 * finalize device
 */
static void felica_uart_exit(void)
{
	kfree(dev_sem);
	device_destroy(felica_class, devid_felica_uart);
	cdev_del(&cdev_felica_uart);
	unregister_chrdev_region(devid_felica_uart, FELICA_MINOR_COUNT);

	FELICA_PR_DBG(" %s START", __func__);
}

/*
 * open device
 */
static int felica_uart_open(struct inode *inode, struct file *file)
{
	uid_t uid;
	int ret;
	FELICA_PR_DBG(" %s START", __func__);


	uid = __task_cred(current)->uid;
	if ((uid != gmfc_uid) && (uid != gdiag_uid) && (uid != gant_uid)) {
		FELICA_PR_ERR
		    (" %s END -EACCESS, uid=[%d], gmfc_uid=[%d], gdiag_uid=[%d]",
		     __func__, uid, gmfc_uid, gdiag_uid);
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
		return -EACCES;
#endif
	}

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_PR_ERR(" %s ERROR(down_interruptible)", \
				 __func__);
		return -ERESTARTSYS;
	}

	if (gfa_open_cnt == 0) {
#ifdef CONFIG_NFC_FELICA
		switch (guartcc_start_req) {
		case UARTCC_NFC_START_AUTOPOLLING:	/* AutoPoiing */
			{
				/* felica open-waiting for nfc autopolling */
				ret = felica_uart_open_wait_for_polling();
				if (ret < 0) {
					FELICA_PR_ERR( \
					" %s open-waiting fail= [%d]\n " \
					, __func__, ret);
					return ret;
				}
				FELICA_PR_INFO( \
					" %s open wait release\n", \
					__func__);
			}
			break;
		case UARTCC_NFC_START_ENDPROC:	/* Unused NFC */
			/* set felica status -> in use */
			uartcc_set_felica_status(UARTCC_FELICA_STATAUS_IN_USE);
			FELICA_PR_INFO( \
				" %s started using FeliCa=[%d]\n", \
				__func__, guartcc_start_req);
			break;
		default:	/* NFC in use */
			FELICA_PR_ERR(" -EFAULT %s NFC in use=[%d]\n", \
				__func__, guartcc_start_req);
			/* reset felica status -> idle */
			uartcc_set_felica_status(
				UARTCC_FELICA_STATAUS_IDLE);
			up(&dev_sem->felica_sem);
			return -EFAULT;
			break;
		}
#endif /* CONFIG_NFC_FELICA */
		memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
		memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);
		gfa_send_str[0] = FELICA_NL_REQ_OPEN;
		gfa_send_str[1] = felica_uart_port;

		ret = felica_nl_send_msg(2);

		if (ret == 0) {
			felica_nl_wait_ret_msg();
			if (gfa_rcv_str[1] == FELICA_NL_EFAILED) {
				FELICA_PR_ERR(" %s Open Fail", __func__);
#ifdef CONFIG_NFC_FELICA
				/* reset felica status -> idle */
				uartcc_set_felica_status(
					UARTCC_FELICA_STATAUS_IDLE);
#endif /* CONFIG_NFC_FELICA */
				up(&dev_sem->felica_sem);
				return -EFAULT;
			}
		} else {
			FELICA_PR_ERR(" %s felica_nl_send_msg Fail", \
					__func__);
#ifdef CONFIG_NFC_FELICA
			/* reset felica status -> idle */
			uartcc_set_felica_status(
				UARTCC_FELICA_STATAUS_IDLE);
#endif /* CONFIG_NFC_FELICA */
			up(&dev_sem->felica_sem);
			return -EFAULT;
		}
#ifdef F_WAKE_LOCK
		wake_lock(&felica_wake_2);
		FELICA_PR_INFO(" %s Wake Lock(2)", __func__);
#endif
	}
	gfa_open_cnt++;

	up(&dev_sem->felica_sem);

	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

#ifdef CONFIG_NFC_FELICA
/*
 * felica open-waiting for nfc autopolling
 */
static int felica_uart_open_wait_for_polling(void)
{
	int ret = 0;
	struct icc_poll_data *felica_poll = &gfelica_poll_data;

	FELICA_PR_DBG(" %s START\n", __func__);

	/* set felica status -> wait polling */
	uartcc_set_felica_status(UARTCC_FELICA_STATAUS_WAIT_POLLING);
	up(&dev_sem->felica_sem);
	ret = wait_event_interruptible(felica_poll->rsp_wait,
						felica_poll->rsp_done == 1);

	if (ret < 0) {
		/* reset felica status -> idle */
		uartcc_set_felica_status(UARTCC_FELICA_STATAUS_IDLE);
		FELICA_PR_ERR
		(" %s ERROR(wait_event_interruptible), ret=[%d]\n", \
				__func__, ret);
		return -EIO;
	}
	if (down_interruptible(&dev_sem->felica_sem)) {
		/* reset felica status -> idle */
		uartcc_set_felica_status(UARTCC_FELICA_STATAUS_IDLE);
		FELICA_PR_ERR(" %s ERROR(down_interruptible)\n", \
				 __func__);
		return -ERESTARTSYS;
	}

	/* set felica status -> in use */
	uartcc_set_felica_status(UARTCC_FELICA_STATAUS_IN_USE);
	felica_poll->rsp_done = 0;

	FELICA_PR_DBG(" %s END\n", __func__);
	return 0;
}
#endif /* CONFIG_NFC_FELICA */

/*
 * close device
 */
static int felica_uart_close(struct inode *inode, struct file *file)
{
	int ret;
	FELICA_PR_DBG(" %s START", __func__);

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_PR_ERR(" %s ERROR(down_interruptible)", \
					__func__);
		return -ERESTARTSYS;
	}

	gfa_open_cnt--;
	if (gfa_open_cnt == 0) {
		memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
		memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);
		gfa_send_str[0] = FELICA_NL_REQ_CLOSE;
		ret = felica_nl_send_msg(1);
		if (ret == 0) {
			felica_nl_wait_ret_msg();
			if (gfa_rcv_str[1] == FELICA_NL_EFAILED) {
				FELICA_PR_ERR(" %s Close Fail",\
						 __func__);
				gfa_open_cnt++;
				up(&dev_sem->felica_sem);
				return -EFAULT;
			}
#ifdef CONFIG_NFC_FELICA
			/* set felica status -> idle */
			uartcc_set_felica_status(UARTCC_FELICA_STATAUS_IDLE);
			if (1 == available_d->available_flag) {
				if (0 != g_cen_sts) {
					if (GPIO_VALUE_LOW != g_rfs_sts) {
						available_d->rsp_done = 1;
						FELICA_PR_INFO(
							" wake up available");
						wake_up_interruptible(
						&available_d->read_wait);
					}
				}
			}
#endif /* CONFIG_NFC_FELICA */
		} else {
			FELICA_PR_ERR(" %s felica_nl_send_msg Fail", \
					__func__);
			gfa_open_cnt++;
			up(&dev_sem->felica_sem);
			return -EFAULT;
		}
#ifdef F_WAKE_LOCK
		wake_unlock(&felica_wake_2);
		FELICA_PR_INFO(" %s Wake UnLock(2)", __func__);
#endif
	}

	up(&dev_sem->felica_sem);

	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_uart_read(struct file *file, char __user *buf,
				size_t len, loff_t *ppos)
{
	int ret = 0;
	int nlret;
	size_t wk_len = 0;

	FELICA_PR_DBG(" %s START", __func__);

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_PR_ERR(" %s ERROR(down_interruptible)", \
					 __func__);
		return -ERESTARTSYS;
	}

	memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
	memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);

	wk_len = len;
	if (FELICA_NL_MSG_DATA_SIZE < wk_len) {
		FELICA_PR_INFO(" %s  read max size over [%d]", __func__,
				 wk_len);
		wk_len = FELICA_NL_MSG_DATA_SIZE;
	}
	gfa_send_str[0] = FELICA_NL_REQ_READ;
	gfa_send_str[1] = (char)(wk_len >> 8);
	gfa_send_str[2] = (char)wk_len;
	nlret = felica_nl_send_msg(3);

	wk_len = 0;
	if (nlret == 0) {
		felica_nl_wait_ret_msg();
		if (gfa_rcv_str[1] == FELICA_NL_SUCCESS) {
			wk_len =
			    (((int)gfa_rcv_str[2] << 8) & 0xFF00) | \
				(int)gfa_rcv_str[3];
			FELICA_PR_DBG(" %s Rcv len [%d]", __func__, wk_len);
			FELICA_PR_DBG(
				" %s Rcv data [%x][%x][%x][%x][%x][%x][%x][%x][%x][%x]",
				__func__, gfa_rcv_str[4], gfa_rcv_str[5],
				gfa_rcv_str[6], gfa_rcv_str[7],
				gfa_rcv_str[8], gfa_rcv_str[9],
				gfa_rcv_str[10], gfa_rcv_str[11],
				gfa_rcv_str[12], gfa_rcv_str[13]);
			FELICA_PR_DBG(
				" %s Rcv data2 [%x][%x][%x][%x][%x][%x][%x][%x][%x][%x]",
				__func__, gfa_rcv_str[14], gfa_rcv_str[15],
				gfa_rcv_str[16], gfa_rcv_str[17],
				gfa_rcv_str[18], gfa_rcv_str[19],
				gfa_rcv_str[20], gfa_rcv_str[21],
				gfa_rcv_str[22], gfa_rcv_str[23]);
			FELICA_PR_DBG(
				" %s Rcv data3 [%x][%x][%x][%x][%x][%x][%x][%x][%x][%x]",
				__func__, gfa_rcv_str[24], gfa_rcv_str[25],
				gfa_rcv_str[26], gfa_rcv_str[27],
				gfa_rcv_str[28], gfa_rcv_str[29],
				gfa_rcv_str[30], gfa_rcv_str[31],
				gfa_rcv_str[32], gfa_rcv_str[33]);
			ret = copy_to_user(buf, &gfa_rcv_str[4], wk_len);
			if (ret != 0) {
				FELICA_PR_ERR
				    ("%s ERROR(copy_from_user), ret=[%d]",
					  __func__, ret);
				up(&dev_sem->felica_sem);
				return -EFAULT;
			}
			*ppos = *ppos + wk_len;
		} else {
			FELICA_PR_ERR(" %s FAIL", __func__);
			up(&dev_sem->felica_sem);
			return -EFAULT;
		}
	} else {
		FELICA_PR_ERR(" %s FAIL", __func__);
		up(&dev_sem->felica_sem);
		return -EFAULT;
	}

	up(&dev_sem->felica_sem);

	FELICA_PR_DBG(" %s END", __func__);
	return (ssize_t) wk_len;
}

/*
 * write operation
 */
static ssize_t felica_uart_write(struct file *file, const char __user *data,
				 size_t len, loff_t *ppos)
{
	int ret = 0;
	int nlret;
	size_t wk_len = 0;

	FELICA_PR_DBG(" %s START", __func__);

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_PR_ERR(" %s ERROR(down_interruptible)", \
				 __func__);
		return -ERESTARTSYS;
	}

	memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
	memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);

	wk_len = len;
	if (FELICA_NL_MSG_DATA_SIZE < wk_len) {
		FELICA_PR_INFO(" %s read max size over [%d]", __func__,
				 wk_len);
		wk_len = FELICA_NL_MSG_DATA_SIZE;
	}
	gfa_send_str[0] = FELICA_NL_REQ_WRITE;
	gfa_send_str[1] = (char)(wk_len >> 8);
	gfa_send_str[2] = (char)wk_len;
	ret = copy_from_user(&gfa_send_str[3], data, wk_len);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_from_user), ret=[%d]",
			       __func__, ret);
		up(&dev_sem->felica_sem);
		return -EFAULT;
	}
	nlret = felica_nl_send_msg(3 + len);
	if (nlret == 0) {
		wk_len = 0;
		felica_nl_wait_ret_msg();
		wk_len = (((int)gfa_rcv_str[2] << 8) & 0xFF00) | \
				(int)gfa_rcv_str[3];
		if (gfa_rcv_str[1] == FELICA_NL_EFAILED) {
			FELICA_PR_ERR(" %s Write Fail", __func__);
			up(&dev_sem->felica_sem);
			return -EINVAL;
		}
	} else {
		FELICA_PR_ERR(" %s felica_nl_send_msg Fail", __func__);
		up(&dev_sem->felica_sem);
		return -EINVAL;
	}
	up(&dev_sem->felica_sem);

	FELICA_PR_DBG(" %s END", __func__);
	return wk_len;
}

/*
 * sync operation
 */
static int felica_uart_sync(struct file *file, loff_t start,
		loff_t len, int datasync)
{
	FELICA_PR_DBG(" %s START", __func__);
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * available operation
 */
static long felica_uart_ioctl(struct file *file, unsigned int cmd,
			      unsigned long arg)
{
	unsigned int ret_str = 0;
	int ret;
	FELICA_PR_DBG(" %s START", __func__);
	FELICA_PR_INFO(" %s cmd = [0x%x]", __func__, cmd);

	/* Ignore [TCGETS:0x5401] and [TCSBRK:0x5409] */
	if (TCGETS == cmd || TCSBRK == cmd) {
		FELICA_PR_INFO(" %s cmd throw [0x%x]", __func__, cmd);
		return 0;
	}
	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_PR_ERR(" %s ERROR(down_interruptible)", \
				__func__);
		return -ERESTARTSYS;
	}

	memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
	memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);

	gfa_send_str[0] = FELICA_NL_REQ_AVAIABLE;
	ret = felica_nl_send_msg(1);

	if (ret == 0) {

		/* 20130216 add for APQ support -S- */
		/* 1ms sleep */
		usleep_range(1000, 1000);
		/* 20130216 add for APQ support -E- */

		felica_nl_wait_ret_msg();
		if (gfa_rcv_str[1] == FELICA_NL_SUCCESS) {
			/* create response data */
			ret_str =
			    (((unsigned int)gfa_rcv_str[2] << 8) & 0xFF00) | \
				(unsigned int)gfa_rcv_str[3];
			FELICA_PR_INFO(" Available Success data size [%d]", \
				ret_str);
		} else {
			FELICA_PR_ERR(" %s Available Fail", __func__);
			up(&dev_sem->felica_sem);
			return -EINVAL;
		}
	} else {
		FELICA_PR_ERR(" %s felica_nl_send_msg Fail", __func__);
		up(&dev_sem->felica_sem);
		return -EINVAL;
	}

	up(&dev_sem->felica_sem);

	FELICA_PR_DBG(" %s END", __func__);
	return put_user(ret_str, (unsigned int __user *)arg);

}

/*
 * create netlink socket
 */
static void felica_nl_init(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	gfa_connect_flag = 0;
	gfa_pid = 0;
	gfa_wait_flag = 0;
	memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
	memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);

	gfanl_sk =
	    netlink_kernel_create(&init_net, FELICA_NL_NETLINK_USER, 0,
				  felica_nl_recv_msg, NULL, THIS_MODULE);
	if (!gfanl_sk)
		FELICA_PR_ERR(" Error creating socket. %s\n", __func__);

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * release netlink socket
 */
static void felica_nl_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	netlink_kernel_release(gfanl_sk);

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * send message to FeliCa-Serial-Connector
 */
static int felica_nl_send_msg(int len)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb_out;
	int msg_size = 0;
	FELICA_PR_DBG(" %s START len: %d", __func__,len);

	if (!gfanl_sk) {
		FELICA_PR_ERR("Error Not creating socket. %s\n",
			       __func__);
		return 1;
	}
	if (gfa_pid == 0) {
		FELICA_PR_ERR("Error Not Rcv Connect Msg %s\n",
			       __func__);
		return 1;
	}

	msg_size = len;
	skb_out = nlmsg_new(msg_size, 0);

	if (!skb_out) {
		FELICA_PR_ERR(" Failed to allocate new skb_out %s\n", __func__);
		return 1;
	}
	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
	NETLINK_CB(skb_out).dst_group = 0;
	memcpy(NLMSG_DATA(nlh), gfa_send_str, msg_size);

	/* "skb_out" will release by netlink.*/
	nlmsg_unicast(gfanl_sk, skb_out, gfa_pid);

	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * receive message from FeliCa-Serial-Connector
 */
static void felica_nl_recv_msg(struct sk_buff *skb)
{

	struct nlmsghdr *nlh;
	struct sk_buff *wskb;

#ifdef FELICA_UICC_FUNCTION
	int init_flag = 0;
#endif

	if (!skb) {
		FELICA_PR_ERR(" %s ERROR(skb NULL)", __func__);
		return;
	}

	wskb = skb_get(skb);
	if (wskb && (wskb->len > NLMSG_SPACE(0))) {
		nlh = nlmsg_hdr(wskb);
		memcpy(gfa_rcv_str, NLMSG_DATA(nlh), sizeof(gfa_rcv_str));
		FELICA_PR_DBG(" %s START gfa_rcv_str[0]: 0x%x", __func__,gfa_rcv_str[0]);
		if ((gfa_rcv_str[0] == FELICA_NL_CONNECT_MSG)
		    && (gfa_connect_flag == 0)) {
			/* pid of sending process */
			gfa_pid = nlh->nlmsg_pid;

	if (felica_get_tamper_fuse_cmd() != 1)
			{
			/* jmodel */
#if defined(CONFIG_ARCH_EXYNOS)
			s3c_gpio_cfgall_range(FELICA_UART1RX, 2,\
			S3C_GPIO_SFN(2), S3C_GPIO_PULL_DOWN);
			felica_uart_port = 1;
#elif defined(CONFIG_ARCH_APQ8064)
			felica_uart_port = 2;
#elif defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_SBM)
			felica_uart_port = 2;
#elif defined(CONFIG_MACH_HLTEDCM)	|| defined(CONFIG_MACH_HLTEKDI) || defined(CONFIG_MACH_JS01LTEDCM)
			felica_uart_port = 1;
#endif

			felica_set_felica_info();
			felica_uart_init();
			felica_pon_init();
			felica_cen_init();
			felica_rfs_init();
			felica_rws_init();
			felica_ant_init();
#ifdef CONFIG_NFC_FELICA
			snfc_uart_init();
			cxd2235power_init();
			hsel_init();
			snfc_rfs_init();
			available_poll_init();
			snfc_cen_init();
#endif
			if (gdiag_name[0] != 0x00)
				felica_uid_init();
			}

			gfa_connect_flag = 1;
#ifdef FELICA_UICC_FUNCTION
			init_flag = 1;
			guartcc_felica_status = UARTCC_FELICA_STATAUS_IN_INIT;
			FELICA_PR_INFO(" RCV MSG [%d]", gfa_rcv_str[0]);
#endif
		} else if ((gfa_rcv_str[0] == FELICA_NL_RESPONCE)
				&& (gfa_pid == nlh->nlmsg_pid)) {
			/* wake up */
			gfa_wait_flag = 1;
#ifdef FELICA_UICC_FUNCTION
		} else if ((gfa_rcv_str[0] == FELICA_INIT_UICC_END)
				&& (gfa_pid == nlh->nlmsg_pid)) {
			FELICA_PR_INFO("RCV MSG FELICA_INIT_UICC_END [%d]",
			gfa_rcv_str[0]);
			if (gfa_rcv_str[1] == FELICA_NL_SUCCESS)
				FELICA_PR_INFO("UICC INIT END SUCCESS");
			else
				FELICA_PR_INFO("UICC INIT END FAIL");

			guartcc_felica_status = UARTCC_FELICA_STATAUS_IDLE;
#endif
		} else {
			FELICA_PR_ERR(" %s ERROR(RCV Undefine MSG) RCV MSG [%d], rcv pid [%d], gfa_pid [%d]",
				       __func__, gfa_rcv_str[0],nlh->nlmsg_pid,gfa_pid);
		}
	}
	kfree_skb(skb);

#ifdef FELICA_UICC_FUNCTION
	if (init_flag) {
		FELICA_PR_INFO("UICC INIT START");
		memset(gfa_send_str, 0, FELICA_NL_MSG_SIZE);
		memset(gfa_rcv_str, 0, FELICA_NL_MSG_SIZE);
		gfa_send_str[0] = FELICA_NL_INIT_END_RES;
		felica_nl_send_msg(1);
	}
#endif
	FELICA_PR_DBG(" %s END", __func__);
}
static void felica_set_felica_info(void)
{
	FELICA_PR_DBG(" %s START ", __func__);
	memset(gdiag_name, 0x00, DIAG_NAME_MAXSIZE + 1);
#ifdef FELICA_UICC_FUNCTION
	memset(gproc_name, 0x00, PROC_NAME_MAXSIZE + 1);
#endif
	gread_msgs[0].flags = gfa_rcv_str[MSG_READ1_FLAGS_OFFSET];
	gread_msgs[0].len = gfa_rcv_str[MSG_READ1_LEN_OFFSET];
	gread_msgs[1].flags = gfa_rcv_str[MSG_READ2_FLAGS_OFFSET];
	gread_msgs[1].len = gfa_rcv_str[MSG_READ2_LEN_OFFSET];
	gwrite_msgs[0].flags = gfa_rcv_str[MSG_WRITE_FLAGS_OFFSET];
	gwrite_msgs[0].len = gfa_rcv_str[MSG_WRITE_LEN_OFFSET];
	gi2c_lockaddress = gfa_rcv_str[MSG_LOCK_ADDR_OFFSET];
	gi2c_address = gfa_rcv_str[MSG_I2C_ADDR_OFFSET];

	memcpy(gdiag_name, &gfa_rcv_str[MSG_DIAG_NAME_OFFSET],
							DIAG_NAME_MAXSIZE);

	gmfc_uid =
	(((int)gfa_rcv_str[MSG_MFC_UID_FRONT_OFFSET] << 8) & 0xFF00) |
	(int)gfa_rcv_str[MSG_MFC_UID_BACK_OFFSET];

	gmfl_uid =
	(((int)gfa_rcv_str[MSG_MFL_UID_FRONT_OFFSET] << 8) & 0xFF00) |
	(int)gfa_rcv_str[MSG_MFL_UID_BACK_OFFSET];

	gi2c_antaddress = gfa_rcv_str[MSG_ANT_ADDR_OFFSET];
	gant_uid =
	(((int)gfa_rcv_str[MSG_ANT_UID_FRONT_OFFSET] << 8) & 0xFF00) |
	(int)gfa_rcv_str[MSG_ANT_UID_BACK_OFFSET];

#ifdef CONFIG_NFC_FELICA
	gnfc_uid =
	(((int)gfa_rcv_str[MSG_NFC_UID_FRONT_OFFSET] << 8) & 0xFF00) |
	(int)gfa_rcv_str[MSG_NFC_UID_BACK_OFFSET];

	snfc_cen_sts_init();
#endif
#ifdef FELICA_UICC_FUNCTION
	memcpy(gproc_name, &gfa_rcv_str[MSG_PROC_NAME_OFFSET],
							PROC_NAME_MAXSIZE);
#endif
	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * waiting to receive messages from FeliCa-Serial-Connector
 */
static void felica_nl_wait_ret_msg(void)
{
	unsigned int cnt = 0;
	FELICA_PR_DBG(" %s START ", __func__);

	while (1) {
		if (gfa_wait_flag == 1) {
			FELICA_PR_DBG(" %s sleep cnt [%d]", __func__,
					 cnt);
			break;
		}
		mdelay(1);
		cnt++;
	}
	gfa_wait_flag = 0;

	FELICA_PR_DBG(" %s END ", __func__);
}


#if defined(CONFIG_ARCH_EXYNOS)
static int felica_smc_read_oemflag(u32 ctrl_word, u32 *val)
{
	register u32 reg0 __asm__("r0");
	register u32 reg1 __asm__("r1");
	register u32 reg2 __asm__("r2");
	register u32 reg3 __asm__("r3");
	u32 idx = 0;

	for (idx = 0; reg2 != ctrl_word; idx++) {
		reg0 = -202;
		reg1 = 1;
		reg2 = idx;

		__asm__ volatile ("smc    0\n" : "+r" (reg0), "+r"(reg1),
				  "+r"(reg2), "+r"(reg3)
		    );
		if (reg1)
			return -1;
	}

	reg0 = -202;
	reg1 = 1;
	reg2 = idx;

	__asm__ volatile ("smc    0\n" : "+r" (reg0), "+r"(reg1),
		"+r"(reg2),  "+r"(reg3)
	    );
	if (reg1)
		return -1;

	*val = reg2;
	return 0;
}

static int felica_Cpu0(void)
{
	int ret = 0;
	struct cpumask mask = CPU_MASK_CPU0;

	FELICA_PR_INFO(" System has %d CPU's, we are on CPU #%d\n"
	    "\tBinding this process to CPU #0.\n"
	    "\tactive mask is %lx, setting it to mask=%lx\n",
	    nr_cpu_ids,
	    raw_smp_processor_id(), cpu_active_mask->bits[0], mask.bits[0]);

	ret = set_cpus_allowed_ptr(current, &mask);
	if (0 != ret)
		FELICA_PR_INFO(" set_cpus_allowed_ptr=%d.\n", ret);

	FELICA_PR_INFO(" And now we are on CPU #%d", raw_smp_processor_id());

	return ret;
}

static int felica_CpuAll(void)
{
	int ret = 0;
	struct cpumask mask = CPU_MASK_ALL;

	FELICA_PR_INFO(" System has %d CPU's, we are on CPU #%d\n"
	    "\tBinding this process to CPU #0.\n"
	    "\tactive mask is %lx, setting it to mask=%lx\n",
	    nr_cpu_ids,
	    raw_smp_processor_id(), cpu_active_mask->bits[0], mask.bits[0]);

	ret = set_cpus_allowed_ptr(current, &mask);
	if (0 != ret)
		FELICA_PR_INFO(" set_cpus_allowed_ptr=%d.\n", ret);

	FELICA_PR_INFO(" And now we are on CPU #%d", raw_smp_processor_id());

	return ret;
}


static uint8_t felica_get_tamper_fuse_cmd(void)
{
	u32 fuse_id = 0;
	int ret;

	ret = felica_Cpu0();
	if (0 != ret) {
		FELICA_PR_ERR(" changing core failed!");
		return -1;
	}

	FELICA_PR_INFO(" get_fuse");

	if (felica_smc_read_oemflag(0x80010001, (u32 *)&fuse_id) < 0) {
		FELICA_PR_ERR(" %s read flag error", __func__);
		return -1;
	}
	FELICA_PR_INFO(" Kernel Status[%x]", fuse_id);
	felica_CpuAll();

	return (uint8_t)fuse_id;
}

#elif defined(CONFIG_ARCH_APQ8064) || defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)

static uint8_t felica_get_tamper_fuse_cmd(void)
{

	uint32_t fuse_id = FELICA_HLOS_IMG_TAMPER_FUSE;
	void *cmd_buf;
	size_t cmd_len;
	size_t resp_len = 0;
	uint8_t resp_buf;

	FELICA_PR_DBG(" %s START ", __func__);

	resp_len = sizeof(resp_buf);
	cmd_buf = (void *)&fuse_id;
	cmd_len = sizeof(fuse_id);

	scm_call(FELICA_SCM_SVC_FUSE, FELICA_SCM_IS_SW_FUSE_BLOWN_ID, cmd_buf,
						cmd_len, &resp_buf, resp_len);
	FELICA_PR_DBG(" %s END resp_buf = %d\n",__func__, resp_buf);
	
#ifdef NO_CHECK_TAMPER
	FELICA_PR_ERR(" No checking tampered kernel just for test!\n");
	return 0;
#endif		

	return resp_buf;
}
#endif


/******************************************************************************
 * /dev/felica_pon
 *****************************************************************************/

/* character device definition */
static dev_t devid_felica_pon;
static struct cdev cdev_felica_pon;
static const struct file_operations fops_felica_pon = {
	.owner = THIS_MODULE,
	.open = felica_pon_open,
	.release = felica_pon_close,
	.read = felica_pon_read,
	.write = felica_pon_write,
};

/*
 * initialize device
 */
static void felica_pon_init(void)
{
	int ret;
	struct device *device_felica_pon;
	FELICA_PR_DBG(" %s START", __func__);
	
	devid_felica_pon = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_pon, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_PON_NAME);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_pon, &fops_felica_pon);
	ret = cdev_add(&cdev_felica_pon, devid_felica_pon, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_pon, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_pon =
	    device_create(felica_class, NULL, devid_felica_pon, NULL,
			  FELICA_PON_NAME);
	if (IS_ERR(device_felica_pon)) {
		cdev_del(&cdev_felica_pon);
		unregister_chrdev_region(devid_felica_pon, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)", __func__);
		return;
	}

	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_pon), MINOR(devid_felica_pon));
}

/*
 * finalize device
 */
static void felica_pon_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	device_destroy(felica_class, devid_felica_pon);
	cdev_del(&cdev_felica_pon);
	unregister_chrdev_region(devid_felica_pon, FELICA_MINOR_COUNT);

	FELICA_PR_DBG(" %s START", __func__);
}

/*
 * open device
 */
static int felica_pon_open(struct inode *inode, struct file *file)
{
	uid_t uid;
#ifdef FELICA_UICC_FUNCTION
	int ret = 0;
	char *cmdpos;
	static char cmdline[1025];
	static unsigned long start_adr, end_adr, leng;
#endif

#ifdef FELICA_UICC_FUNCTION
	start_adr = current->mm->arg_start;
	end_adr = current->mm->arg_end;
	leng = end_adr - start_adr;

	if (1024 < leng)
		leng = 1024;
	cmdpos = (char *)(current->mm->arg_start);
	ret = copy_from_user(cmdline, cmdpos, leng);
	cmdline[leng] = '\0';
	if(ret != 0)
	{
		FELICA_PR_ERR(" %s cmdline[%s] ret[%d]", __func__, cmdline, ret);
		return -EFAULT;
	}
#endif
	uid = __task_cred(current)->uid;
#ifdef FELICA_UICC_FUNCTION
	if (((uid != gmfc_uid) && (uid != gdiag_uid)
		&& (uid != gant_uid)) &&
		(strncmp(cmdline,gproc_name, leng) != 0)) {
#else
	if ((uid != gmfc_uid) && (uid != gdiag_uid)
	&& (uid != gant_uid)) {
#endif
		FELICA_PR_ERR
		    (" %s END -EACCES, uid=[%d], gmfc_uid=[%d], gdiag_uid=[%d]",
		     __func__, uid, gmfc_uid, gdiag_uid);
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
		return -EACCES;
#endif
	}

	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_pon_close(struct inode *inode, struct file *file)
{

#if defined(CONFIG_ARCH_EXYNOS)
	gpio_set_value(GPIO_PINID_FELICA_PON, GPIO_VALUE_LOW);
#elif defined(CONFIG_ARCH_APQ8064)
	ice_gpiox_set(GPIO_PINID_FELICA_PON, GPIO_VALUE_LOW);
#elif defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	gpio_set_value(gfelica_pon_pin, GPIO_VALUE_LOW);
#else
	gpio_set_value(GPIO_PINID_FELICA_PON, GPIO_VALUE_LOW);
#endif	// CONFIG_MACH_KLTE
#endif

	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_pon_read(struct file *file, char __user *buf, size_t len,
			       loff_t *ppos)
{
	int ret;
	char retparam;

#if defined(CONFIG_ARCH_EXYNOS)
	ret = gpio_get_value(GPIO_PINID_FELICA_PON);
#elif defined(CONFIG_ARCH_APQ8064)
	ret = ice_gpiox_get(GPIO_PINID_FELICA_PON);
#elif defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	ret = gpio_get_value(gfelica_pon_pin);
#else
	ret = gpio_get_value(GPIO_PINID_FELICA_PON);
#endif // CONFIG_MACH_KLTE
#endif
	FELICA_PR_INFO(" %s GPIO Read value of GPIO_PINID_FELICA_PON -> ret=[%d]", __func__,ret);

	if (ret == GPIO_VALUE_HIGH) {
		retparam = FELICA_PON_WIRED;
		FELICA_PR_INFO(" Wired interface Status is [%d][start]",
				 retparam);
	} else if (ret == GPIO_VALUE_LOW) {
		retparam = FELICA_PON_WIRELESS;
		FELICA_PR_INFO(" Wired interface Status is [%d][standby]",
				 retparam);
	} else {
		FELICA_PR_ERR(" %s ERROR(gpio_get_value), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	ret = copy_to_user(buf, &retparam, FELICA_PON_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	return FELICA_PON_DATA_LEN;
}

/*
 * write operation
 */
static ssize_t felica_pon_write(struct file *file, const char __user *data,
				size_t len, loff_t *ppos)
{
	char pon;
	int ret;
	int setparam;

	ret = copy_from_user(&pon, data, FELICA_PON_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_from_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}

	if (pon == FELICA_PON_WIRED) {
		setparam = GPIO_VALUE_HIGH;
		FELICA_PR_INFO(" Set wired interface to [%d][start]",
				 setparam);
	} else if (pon == FELICA_PON_WIRELESS) {
		setparam = GPIO_VALUE_LOW;
		FELICA_PR_INFO(" Set wired interface to [%d][standby]",
				 setparam);
	} else {
		FELICA_PR_ERR(" %s ERROR(copy_from_user), pon=[%d]",
			       __func__, pon);
		return -EINVAL;
	}

#if defined(CONFIG_ARCH_EXYNOS)
	gpio_set_value(GPIO_PINID_FELICA_PON, setparam);
#elif defined(CONFIG_ARCH_APQ8064)
	ice_gpiox_set(GPIO_PINID_FELICA_PON, setparam);
#elif defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if defined(CONFIG_MACH_KLTE_KDI)
	if(gfelica_uim_mon == 1) {
		FELICA_PR_ERR(" %s gfelica_uim_mon=%d\n", __func__, gfelica_uim_mon);
		gpio_set_value(gfelica_pon_pin, GPIO_VALUE_HIGH);		
		return FELICA_PON_DATA_LEN;
	}
	gpio_set_value(gfelica_pon_pin, setparam);
#elif defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	gpio_set_value(gfelica_pon_pin, setparam);
#else
	gpio_set_value(GPIO_PINID_FELICA_PON, setparam);
#endif // CONFIG_MACH_KLTE
#endif 

	return FELICA_PON_DATA_LEN;
}

/******************************************************************************
 * felica_i2c_driver
 ******************************************************************************/
static struct i2c_client *felica_i2c_client;
static const struct i2c_device_id felica_i2c_idtable[] = 
{
	{FELICA_I2C_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, felica_i2c_idtable);

#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
static struct of_device_id felica_i2c_match_table[] = 
{
	{ .compatible = "felica,felica-i2c",},
	{},
};
#endif


static struct i2c_driver felica_i2c_driver = 
{
	.probe = felica_i2c_probe,
	.remove = felica_i2c_remove,
	.driver = 
			{
		   .name = FELICA_I2C_NAME,
		   .owner = THIS_MODULE,
#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
		   .of_match_table = felica_i2c_match_table,
#endif
			},
	.id_table = felica_i2c_idtable,
};

/*
 * felica_i2c_init
 */
static void felica_i2c_init(void)
{
	int ret;
	FELICA_PR_DBG(" %s START", __func__);

	ret = i2c_add_driver(&felica_i2c_driver);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(i2c_add_driver), ret=[%d]",
			       __func__, ret);
		return;
	}

	FELICA_PR_DBG(" %s END", __func__);
	return;
}

/*
 * felica_i2c_exit
 */
static void felica_i2c_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);
	i2c_del_driver(&felica_i2c_driver);
	FELICA_PR_DBG(" %s END", __func__);
	return;
}

/*
 * probe device
 */
static int felica_i2c_probe(struct i2c_client *client,
			    const struct i2c_device_id *devid)
{
	FELICA_PR_INFO(" %s START", __func__);

	felica_i2c_client = client;
	if (!felica_i2c_client) {
		FELICA_PR_ERR(" %s ERROR(felica_i2c_client==NULL)", \
							__func__);
		return -EINVAL;
	}



	FELICA_PR_INFO(" %s END", __func__);
	return 0;
}

/*
 * remove device
 */
static int felica_i2c_remove(struct i2c_client *client)
{
	/* no operation */
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/******************************************************************************
 * /dev/felica_cen
 ******************************************************************************/

/* character device definition */
static dev_t devid_felica_cen;
static struct cdev cdev_felica_cen;
static const struct file_operations fops_felica_cen = {
	.owner = THIS_MODULE,
	.open = felica_cen_open,
	.release = felica_cen_close,
	.read = felica_cen_read,
	.write = felica_cen_write,
};

/*
 * felica_cen_init
 */
static void felica_cen_init(void)
{
	int ret;
	struct device *device_felica_cen;
	FELICA_PR_DBG(" %s START", __func__);

	devid_felica_cen = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_cen, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_CEN_NAME);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_cen, &fops_felica_cen);
	ret = cdev_add(&cdev_felica_cen, devid_felica_cen, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_cen, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_cen =
	    device_create(felica_class, NULL, devid_felica_cen, NULL,
			  FELICA_CEN_NAME);
	if (IS_ERR(device_felica_cen)) {
		cdev_del(&cdev_felica_cen);
		unregister_chrdev_region(devid_felica_cen, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)", __func__);
		return;
	}

	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_cen), MINOR(devid_felica_cen));
}

/*
 * felica_cen_exit
 */
static void felica_cen_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	device_destroy(felica_class, devid_felica_cen);
	cdev_del(&cdev_felica_cen);
	unregister_chrdev_region(devid_felica_cen, FELICA_MINOR_COUNT);

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * open device
 */
static int felica_cen_open(struct inode *inode, struct file *file)
{
	uid_t uid;
#ifdef FELICA_UICC_FUNCTION
	int ret = 0;
	char *cmdpos;
	static char cmdline[1025];
	static unsigned long start_adr, end_adr, leng;
#endif

	FELICA_PR_DBG(" %s START . system_rev=[%d]", __func__,system_rev);

	#if defined(CONFIG_MACH_HLTEDCM)
	//high
	if (system_rev == HW_REV09_OR_10)
	{
		FELICA_PR_INFO(" %s Making MHL_EN HIGH", __func__);
		of_sii8240_hw_poweron(FELICA_CEN_UNLOCK);
	}
	#endif
	
#ifdef FELICA_UICC_FUNCTION
	start_adr = current->mm->arg_start;
	end_adr = current->mm->arg_end;
	leng = end_adr - start_adr;

	if (1024 < leng)
		leng = 1024;
	cmdpos = (char *)(current->mm->arg_start);
	ret = copy_from_user(cmdline, cmdpos, leng);
	cmdline[leng] = '\0';
	if(ret != 0)
	{
		FELICA_PR_ERR(" %s cmdline[%s] ret[%d]", __func__, cmdline, ret);
		return -EFAULT;
	}
#endif

	uid = __task_cred(current)->uid;
	if (file->f_mode & FMODE_WRITE) {
#ifdef FELICA_UICC_FUNCTION
		if ((uid != gdiag_uid) && (uid != gmfl_uid)
		&& (strncmp(cmdline,gproc_name, leng) != 0)) {
#else
		if ((uid != gdiag_uid) && (uid != gmfl_uid)) {
#endif
	FELICA_PR_ERR(" %s END -EACCES, uid=[%d], gmfc_uid=[%d], gdiag_uid=[%d], gmfl_uid=[%d]\n", __func__, uid,gmfc_uid,gdiag_uid,gmfl_uid);
			
			#if defined(CONFIG_MACH_HLTEDCM)
			//low
			if (system_rev == HW_REV09_OR_10)
			{
				FELICA_PR_INFO(" %s Making MHL_EN LOW", __func__);
		//		of_sii8240_hw_poweron(FELICA_CEN_LOCK);
			}
			#endif
			
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
			return -EACCES;
#endif
		}
	}
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_cen_close(struct inode *inode, struct file *file)
{
	/* no operation */
	#if defined(CONFIG_MACH_HLTEDCM)
	//low
	if (system_rev == HW_REV09_OR_10)
	{
		FELICA_PR_INFO(" %s Making MHL_EN LOW", __func__);
	//	of_sii8240_hw_poweron(FELICA_CEN_LOCK);
	}
	#endif
	
	FELICA_PR_DBG(" %s END system_rev=[%d]", __func__,system_rev);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_cen_read(struct file *file, char __user *buf, \
			size_t len, loff_t *ppos)
{
	int ret;
	unsigned char address = gi2c_lockaddress;
	unsigned char read_buff = 0;
	struct i2c_msg read_msgs[2];

	read_msgs[0].flags = gread_msgs[0].flags;
	read_msgs[0].len = gread_msgs[0].len;
	read_msgs[1].flags = gread_msgs[1].flags;
	read_msgs[1].len = gread_msgs[1].len;

	read_msgs[0].addr = gi2c_address;
	read_msgs[0].buf = &address;
	read_msgs[1].addr = gi2c_address;
	read_msgs[1].buf = &read_buff;

	if (felica_i2c_client == NULL) {
		FELICA_PR_ERR(" felica_i2c_client is NULL %s -EIO",__func__);
		return -EIO;
	}

	ret = i2c_transfer(felica_i2c_client->adapter, &read_msgs[0], 1);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(i2c_transfer[0]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}
	ret = i2c_transfer(felica_i2c_client->adapter, &read_msgs[1], 1);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(i2c_transfer[1]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	read_buff &= FELICA_CONTROL_LOCK_MASK;

#ifdef CONFIG_NFC_FELICA
	g_cen_sts = read_buff;
#endif

	FELICA_PR_INFO(" %s g_cen_sts=[%d]", __func__, g_cen_sts);

	ret = copy_to_user(buf, &read_buff, FELICA_CEN_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	return FELICA_CEN_DATA_LEN;
}

/*
 * write operation
 */
static ssize_t felica_cen_write(struct file *file, const char __user *data,
				size_t len, loff_t *ppos)
{
	char cen;
	int ret;
	unsigned char write_buff[2];

	if (felica_i2c_client == NULL) {
		FELICA_PR_ERR(" felica_i2c_client is NULL %s",__func__);
		return -EIO;
	}

	gwrite_msgs[0].buf = &write_buff[0];
	gwrite_msgs[0].addr = gi2c_address;
	write_buff[0] = gi2c_lockaddress;

	ret = copy_from_user(&cen, data, FELICA_CEN_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_from_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	if (cen == FELICA_CEN_UNLOCK) {
		write_buff[1] = FELICA_CEN_SET_UNLOCK;
		FELICA_PR_INFO(" Set FeliCa-Lock status to [%d][UnLock]",
				 write_buff[1]);
	} else if (cen == FELICA_CEN_LOCK) {
		write_buff[1] = FELICA_CEN_SET_LOCK;
		FELICA_PR_INFO(" Set FeliCa-Lock status to [%d][Lock]",
				 write_buff[1]);
	} else {
		FELICA_PR_ERR(" %s ERROR(copy_from_user), cen=[%d]",
			       __func__, cen);
		return -EINVAL;
	}
	ret = i2c_transfer(felica_i2c_client->adapter, gwrite_msgs, 1);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(i2c_transfer), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}
#ifdef CONFIG_NFC_FELICA
	g_cen_sts = cen;
	if (1 == available_d->available_flag) {
		if (0 != g_cen_sts) {
			if (GPIO_VALUE_LOW != g_rfs_sts) {
				if (0 == guartcc_felica_status) {
					available_d->rsp_done = 1;
					FELICA_PR_INFO(" wake up available");
					wake_up_interruptible(
					&available_d->read_wait);
				}
			}
		}
	}

#endif
	FELICA_PR_INFO(" %s END, g_cen_sts =%d, g_rfs_sts = %d", __func__, g_cen_sts, g_rfs_sts);
	return FELICA_CEN_DATA_LEN;
}

/******************************************************************************
 * /dev/felica_rfs
 ******************************************************************************/

/* character device definition */
static dev_t devid_felica_rfs;
static struct cdev cdev_felica_rfs;
static const struct file_operations fops_felica_rfs = {
	.owner = THIS_MODULE,
	.open = felica_rfs_open,
	.release = felica_rfs_close,
	.read = felica_rfs_read,
};

/*
 * initialize device
 */
static void felica_rfs_init(void)
{
	int ret;
	struct device *device_felica_rfs;
	FELICA_PR_DBG(" %s START", __func__);

	devid_felica_rfs = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_rfs, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_RFS_NAME);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_rfs, &fops_felica_rfs);
	ret = cdev_add(&cdev_felica_rfs, devid_felica_rfs, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_rfs, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_rfs =
	    device_create(felica_class, NULL, devid_felica_rfs, NULL,
			  FELICA_RFS_NAME);
	if (IS_ERR(device_felica_rfs)) {
		cdev_del(&cdev_felica_rfs);
		unregister_chrdev_region(devid_felica_rfs, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)", __func__);
		return;
	}

	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_rfs), MINOR(devid_felica_rfs));
}

/*
 * finalize device
 */
static void felica_rfs_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	device_destroy(felica_class, devid_felica_rfs);
	cdev_del(&cdev_felica_rfs);
	unregister_chrdev_region(devid_felica_rfs, FELICA_MINOR_COUNT);

	FELICA_PR_DBG(" %s START", __func__);
}

/*
 * open device
 */
static int felica_rfs_open(struct inode *inode, struct file *file)
{
	uid_t uid;

	uid = __task_cred(current)->uid;

	if ((uid != gmfc_uid) && (uid != gdiag_uid)) {
		FELICA_PR_ERR
		    (" %s -EACCESS, uid=[%d], gmfc_uid=[%d], gdiag_uid=[%d]",
		     __func__, uid, gmfc_uid, gdiag_uid);
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
		return -EACCES;
#endif
	}

	FELICA_PR_DBG(" %s END, uid=[%d], gmfc_uid=[%d], gdiag_uid=[%d]",  __func__, uid, gmfc_uid, gdiag_uid);
	return 0;
}

/*
 * close device
 */
static int felica_rfs_close(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_rfs_read(struct file *file, char __user *buf, \
				size_t len, loff_t *ppos)
{
	int ret;
	char retparam;
	FELICA_PR_DBG(" %s START", __func__);

	ret = gpio_get_value(GPIO_PINID_FELICA_RFS);

	if (ret == GPIO_VALUE_HIGH) {
		retparam = FELICA_RFS_STANDBY;
		FELICA_PR_INFO(" Carrier-Wave Status is [%d][standby]",
				 retparam);
	} else if (ret == GPIO_VALUE_LOW) {
		retparam = FELICA_RFS_DETECTED;
		FELICA_PR_INFO(" Carrier-Wave Status is [%d][detected]",
				 retparam);
	} else {
		FELICA_PR_ERR(" %s ERROR(gpio_get_value), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	ret = copy_to_user(buf, &retparam, FELICA_RFS_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	FELICA_PR_DBG(" %s END", __func__);
	return FELICA_RFS_DATA_LEN;
}

/******************************************************************************
 * /dev/felica_rws
 ******************************************************************************/

/* character device definition */
static dev_t devid_felica_rws;
static struct cdev cdev_felica_rws;
static const struct file_operations fops_felica_rws = {
	.owner = THIS_MODULE,
	.open = felica_rws_open,
	.release = felica_rws_close,
	.read = felica_rws_read,
	.write = felica_rws_write,
};

/*
 * initialize device
 */
static void felica_rws_init(void)
{
	int ret;
	struct device *device_felica_rws;
	FELICA_PR_DBG(" %s START", __func__);

	devid_felica_rws = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_rws, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_RWS_NAME);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_rws, &fops_felica_rws);
	ret = cdev_add(&cdev_felica_rws, devid_felica_rws, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_rws, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_rws =
	    device_create(felica_class, NULL, devid_felica_rws, NULL,
			  FELICA_RWS_NAME);
	if (IS_ERR(device_felica_rws)) {
		cdev_del(&cdev_felica_rws);
		unregister_chrdev_region(devid_felica_rws, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)", __func__);
		return;
	}

	gfelica_rw_status = FELICA_RW_STATUS_INIT;

	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_rws), MINOR(devid_felica_rws));
}

/*
 * finalize device
 */
static void felica_rws_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	device_destroy(felica_class, devid_felica_rws);
	cdev_del(&cdev_felica_rws);
	unregister_chrdev_region(devid_felica_rws, FELICA_MINOR_COUNT);
}

/*
 * open device
 */
static int felica_rws_open(struct inode *inode, struct file *file)
{
	uid_t uid;
	FELICA_PR_DBG(" %s START", __func__);

	uid = __task_cred(current)->uid;
	if (file->f_mode & FMODE_WRITE) {
		if (uid != grwm_uid) {
			FELICA_PR_ERR(\
			" %s END -EACCES, uid=[%d],gmfc_uid=[%d],gdiag_uid=[%d]",
			     __func__, uid, gmfc_uid, gdiag_uid);
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
			return -EACCES;
#endif
		}
	} else {
		if ((uid != gmfc_uid) && (uid != grwm_uid)) {
			FELICA_PR_ERR(\
			" %s END -EACCES, uid=[%d],gmfc_uid=[%d],gdiag_uid=[%d]",
			     __func__, uid, gmfc_uid, gdiag_uid);
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
			return -EACCES;
#endif
		}
	}

	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_rws_close(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_rws_read(struct file *file, char __user *buf, size_t len,
			       loff_t *ppos)
{
	int ret;
	char retparam;

	if (gfelica_rw_status == FELICA_RW_STATUS_ENABLE) {
		retparam = FELICA_RW_STATUS_ENABLE;
		FELICA_PR_INFO(" ReaderWriterFunction status is [%d][enabled] %s",
				 retparam,__func__);
	} else if (gfelica_rw_status == FELICA_RW_STATUS_DISABLE) {
		retparam = FELICA_RW_STATUS_DISABLE;
		FELICA_PR_INFO
		    (" ReaderWriterFunction status is [%d][disabled] %s", retparam,__func__);
	} else {
		FELICA_PR_ERR(" %s ERROR(gfelica_rw_status), RWM=[%d]",
			       __func__, gfelica_rw_status);
		return -EIO;
	}

	ret = copy_to_user(buf, &retparam, FELICA_RWS_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	return FELICA_RWS_DATA_LEN;
}

/*
 * write operation
 */
static ssize_t felica_rws_write(struct file *file, const char __user *data,
				size_t len, loff_t *ppos)
{
	char work;
	int ret;
	FELICA_PR_DBG(" %s START", __func__);

	ret = copy_from_user(&work, data, FELICA_RWS_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_from_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}

	if (work == FELICA_RW_STATUS_ENABLE) {
		FELICA_PR_INFO
		    ("Set ReaderWriterFunction status to [%d][enable]", work);
	} else if (work == FELICA_RW_STATUS_DISABLE) {
		FELICA_PR_INFO
		    ("Set ReaderWriterFunction status to s[%d][disable]", work);
	} else {
		FELICA_PR_ERR(" %s ERROR(copy_from_user), work=[%d]",
			       __func__, work);
		return -EINVAL;
	}

	gfelica_rw_status = work;

	FELICA_PR_DBG(" %s END", __func__);
	return FELICA_RWS_DATA_LEN;
}

/******************************************************************************
 * /dev/felica_int_poll
 ******************************************************************************/

/* character device definition */
static dev_t devid_felica_int_poll;
static struct cdev cdev_felica_int_poll;
static const struct file_operations fops_felica_int_poll = {
	.owner = THIS_MODULE,
	.open = felica_int_poll_open,
	.release = felica_int_poll_close,
	.read = felica_int_poll_read,
	.poll = felica_int_poll_poll,
};
/*
 * top half of irq_handler
 */
static irqreturn_t felica_int_irq_handler(int irq, void *dev_id)
{
	FELICA_PR_DBG(" %s START", __func__);

#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	disable_irq_nosync(gpio_to_irq(GPIO_PINID_FELICA_INT));
#else
	disable_irq_nosync(gpio_to_irq(gfelica_irq_int_pin));
#endif	
#else
	disable_irq_nosync(gpio_to_irq(GPIO_PINID_FELICA_INT_REV03));
#endif /* CONFIG_ARCH_MSM8974 */
	schedule_delayed_work(&pgint_irq->work,
			      msecs_to_jiffies(FELICA_INT_DELAY_TIME));

	FELICA_PR_DBG(" %s END", __func__);
	return IRQ_HANDLED;
}

/*
 * bottom half of irq_handler
 */
static void felica_int_irq_work(struct work_struct *work)
{
	FELICA_PR_DBG(" %s START", __func__);
#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	enable_irq(gpio_to_irq(GPIO_PINID_FELICA_INT));
#else
	enable_irq(gpio_to_irq(gfelica_irq_int_pin));
#endif // CONFIG_MACH_KLTE
#else
	enable_irq(gpio_to_irq(GPIO_PINID_FELICA_INT_REV03));
#endif /* CONFIG_ARCH_MSM8974 */

	pgint_irq->irq_done = 1;

#ifdef F_WAKE_LOCK
	wake_lock_timeout(&felica_wake_1, msecs_to_jiffies(tmout_1));
	FELICA_PR_INFO(" %s Wake Lock(1)[%d]", __func__, tmout_1);
#endif
	wake_up_interruptible(&pgint_irq->read_wait);

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * initialize device
 */
static void felica_int_poll_init(void)
{
	int ret;
	struct device *device_felica_int_poll;
	
#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if !defined(CONFIG_MACH_KLTE_KDI) && !defined(CONFIG_MACH_KLTE_DCM) && !defined(CONFIG_MACH_KLTE_SBM)
	struct device *dev = &felica_gpio_pdev->dev;
	struct device_node *np;
#endif	// CONFIG_MACH_KLTE
#endif

	FELICA_PR_DBG(" %s START", __func__);

	devid_felica_int_poll = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_int_poll, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_INT_POLL_NAME);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_int_poll, &fops_felica_int_poll);
	ret =
	    cdev_add(&cdev_felica_int_poll, devid_felica_int_poll,
		     FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_int_poll,
					 FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_int_poll =
	    device_create(felica_class, NULL, devid_felica_int_poll, NULL,
			  FELICA_INT_POLL_NAME);
	if (IS_ERR(device_felica_int_poll)) {
		cdev_del(&cdev_felica_int_poll);
		unregister_chrdev_region(devid_felica_int_poll,
					 FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)", __func__);
		return;
	}

	memset(pgint_irq, 0x00, sizeof(struct felica_int_irqdata));
	INIT_DELAYED_WORK(&pgint_irq->work, felica_int_irq_work);
	init_waitqueue_head(&pgint_irq->read_wait);


#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if !defined(CONFIG_MACH_KLTE_KDI) && !defined(CONFIG_MACH_KLTE_DCM) && !defined(CONFIG_MACH_KLTE_SBM)
	if (!dev) {
		FELICA_PR_ERR(" %s ERROR NULL", __func__);
		return;
	}
	np  = dev->of_node;
	gfelica_irq_int_pin = of_get_named_gpio(np, "felica,irq-gpio", 0);
	FELICA_PR_INFO(" %s gfelica_irq_int_pin [%d]",
		__func__, gfelica_irq_int_pin);
#endif // CONFIG_MACH_KLTE

#if defined(CONFIG_MACH_HLTEKDI)
	gfelica_sps_pin = of_get_named_gpio(np, "felica,sps-gpio", 0);
	if(gfelica_sps_pin < 0)
	{
		FELICA_PR_ERR(" %s ERROR(), ret=[%d]",__func__, gfelica_sps_pin);
		return;
	}
	FELICA_PR_INFO(" %s gfelica_sps_pin [%d]",
		__func__, gfelica_sps_pin);
	ret = gpio_get_value(gfelica_sps_pin);
	FELICA_PR_INFO(" %s gfelica_sps_pin [%d]",
		__func__, ret);
#endif
#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_JS01LTEDCM)
	if (system_rev >= g_uicc_initrev) {
		gpio_tlmm_config(GPIO_CFG(gfelica_sps_pin, GPIOMUX_FUNC_GPIO,
				 GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				 GPIO_CFG_ENABLE);
		FELICA_PR_INFO(" %s gfelica_sps_pin dcm [%d]",
			__func__, gfelica_sps_pin);
		FELICA_PR_INFO(" %s gfelica_sps_pin dcm [%d]",
			__func__, gpio_get_value(gfelica_sps_pin));
	}
#endif

#if defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	ret = request_threaded_irq(
		  gpio_to_irq(GPIO_PINID_FELICA_INT),
		  NULL,
		  felica_int_irq_handler,
		  IRQF_TRIGGER_FALLING,
		  FELICA_INT_POLL_NAME,
		  (void *)pgint_irq);
	
	FELICA_PR_INFO(" %s INFO(request_irq)= %d GPIO=%d, ret=[%d]",
			       __func__, gpio_to_irq(GPIO_PINID_FELICA_INT),GPIO_PINID_FELICA_INT,ret);		  
#else
	ret = request_threaded_irq(
		  gpio_to_irq(gfelica_irq_int_pin),
		  NULL,
		  felica_int_irq_handler,
		  IRQF_TRIGGER_FALLING,
		  FELICA_INT_POLL_NAME,
		  (void *)pgint_irq);
#endif	// CONFIG_MACH_KLTE  
#else
	ret = request_irq(gpio_to_irq(GPIO_PINID_FELICA_INT_REV03),
		  felica_int_irq_handler,
		  IRQF_TRIGGER_FALLING,
		  FELICA_INT_POLL_NAME, (void *)pgint_irq);
#endif /* CONFIG_ARCH_MSM8974 */

	if (ret != 0) {
		device_destroy(felica_class, devid_felica_int_poll);
		cdev_del(&cdev_felica_int_poll);
		unregister_chrdev_region(devid_felica_int_poll,
					 FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(request_irq), ret=[%d]",
			       __func__,ret);
		return;
	}
#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	ret = enable_irq_wake(gpio_to_irq(GPIO_PINID_FELICA_INT));
	if (ret < 0) {
		free_irq(gpio_to_irq(GPIO_PINID_FELICA_INT), (void *)pgint_irq);
		device_destroy(felica_class, devid_felica_int_poll);
		cdev_del(&cdev_felica_int_poll);
		unregister_chrdev_region(devid_felica_int_poll,
					 FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(enable_irq_wake), ret=[%d]",
			       __func__, ret);
		return;
	}
#else
	ret = enable_irq_wake(gpio_to_irq(gfelica_irq_int_pin));
	if (ret < 0) {
		free_irq(gpio_to_irq(gfelica_irq_int_pin), (void *)pgint_irq);
		device_destroy(felica_class, devid_felica_int_poll);
		cdev_del(&cdev_felica_int_poll);
		unregister_chrdev_region(devid_felica_int_poll,
					 FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(enable_irq_wake), ret=[%d]",
			       __func__, ret);
		return;
	}
#endif	// CONFIG_MACH_KLTE
#else
	ret = enable_irq_wake(gpio_to_irq(GPIO_PINID_FELICA_INT_REV03));
	if (ret < 0) {
		free_irq(gpio_to_irq(
			GPIO_PINID_FELICA_INT_REV03),
			(void *)pgint_irq);
		device_destroy(felica_class, devid_felica_int_poll);
		cdev_del(&cdev_felica_int_poll);
		unregister_chrdev_region(devid_felica_int_poll,
					 FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(enable_irq_wake), ret=[%d]",
			       __func__, ret);
		return;
	}
#endif /* CONFIG_ARCH_MSM8974 */
	pgint_irq->irq_done = 0;

	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]", __func__, \
			 MAJOR(devid_felica_int_poll),
			 MINOR(devid_felica_int_poll));
}

/*
 * finalize device
 */
static void felica_int_poll_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);


#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	disable_irq(gpio_to_irq(GPIO_PINID_FELICA_INT));
	free_irq(gpio_to_irq(
		GPIO_PINID_FELICA_INT),
		(void *)pgint_irq);
#else		
	disable_irq(gpio_to_irq(gfelica_irq_int_pin));
	free_irq(gpio_to_irq(
		gfelica_irq_int_pin),
		(void *)pgint_irq);
#endif	// CONFIG_MACH_KLTE
#else
	disable_irq(gpio_to_irq(GPIO_PINID_FELICA_INT_REV03));
	free_irq(gpio_to_irq(
		GPIO_PINID_FELICA_INT_REV03),
		(void *)pgint_irq);
#endif
	device_destroy(felica_class, devid_felica_int_poll);
	cdev_del(&cdev_felica_int_poll);
	unregister_chrdev_region(
		devid_felica_int_poll,
		FELICA_MINOR_COUNT);

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * open device
 */
static int felica_int_poll_open(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_int_poll_close(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_int_poll_read(struct file *file, char __user *buf,
				    size_t len, loff_t *ppos)
{
	int ret;
	char retparam;
	FELICA_PR_DBG(" %s START", __func__);

	if (!pgint_irq->irq_done) {
		ret =
		    wait_event_interruptible(pgint_irq->read_wait,
					     pgint_irq->irq_done == 1);
		if (ret < 0) {
			FELICA_PR_ERR
			(" %s ERROR(wait_event_interruptible),ret=[%d]",\
			__func__, ret);
			return -EINTR;
		}
	}

#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	ret = gpio_get_value(GPIO_PINID_FELICA_INT);
#else	
	ret = gpio_get_value(gfelica_irq_int_pin);
#endif	// CONFIG_MACH_KLTE
#else
	ret = gpio_get_value(GPIO_PINID_FELICA_INT_REV03);
#endif
	if (ret == GPIO_VALUE_HIGH) {
		retparam = FELICA_INT_HIGH;
		FELICA_PR_INFO(" INT-PIN value is [%d][HIGH]", retparam);
	} else if (ret == GPIO_VALUE_LOW) {
		retparam = FELICA_INT_LOW;
		FELICA_PR_INFO(" INT-PIN value is [%d][LOW]", retparam);
	} else {
		FELICA_PR_ERR(" %s ERROR(gpio_get_value), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	ret = copy_to_user(buf, &retparam, FELICA_INT_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	pgint_irq->irq_done = 0;

	FELICA_PR_DBG(" %s END", __func__);
	return FELICA_INT_DATA_LEN;
}

/*
 * poll operation
 */
static unsigned int felica_int_poll_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	FELICA_PR_DBG(" %s START", __func__);

	poll_wait(file, &pgint_irq->read_wait, wait);
	if (pgint_irq->irq_done)
		mask = POLLIN | POLLRDNORM;

	FELICA_PR_DBG(" %s END", __func__);
	return mask;
}
/******************************************************************************
 * /dev/felica_uid
 ******************************************************************************/

/* character device definition */
static dev_t devid_felica_uid;
static struct cdev cdev_felica_uid;
static const struct file_operations fops_felica_uid = {
	.owner = THIS_MODULE,
	.open = felica_uid_open,
	.release = felica_uid_close,
	.unlocked_ioctl = felica_uid_ioctl,
};

/*
 * initialize device
 */
static void felica_uid_init(void)
{
	int ret;
	struct device *device_felica_uid;
	FELICA_PR_DBG(" %s START", __func__);

	devid_felica_uid = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_uid, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_UID_NAME);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_uid, &fops_felica_uid);
	ret = cdev_add(&cdev_felica_uid, devid_felica_uid, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_uid, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_uid =
	    device_create(felica_class, NULL, devid_felica_uid, NULL,
			  FELICA_UID_NAME);
	if (IS_ERR(device_felica_uid)) {
		cdev_del(&cdev_felica_uid);
		unregister_chrdev_region(devid_felica_uid, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)", __func__);
		return;
	}
	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_uid), MINOR(devid_felica_uid));
}

/*
 * finalize device
 */
static void felica_uid_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);
	if (gdiag_name[0] != 0x00) {
		device_destroy(felica_class, devid_felica_uid);
		cdev_del(&cdev_felica_uid);
		unregister_chrdev_region(devid_felica_uid, \
			FELICA_MINOR_COUNT);
	}
}

/*
 * open device
 */
static int felica_uid_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	char *cmdpos;
	static char cmdline[1025];
	static unsigned long start_adr, end_adr, leng;
	FELICA_PR_DBG(" %s START", __func__);

	start_adr = current->mm->arg_start;
	end_adr = current->mm->arg_end;
	leng = end_adr - start_adr;

	if (1024 < leng)
		leng = 1024;

	cmdpos = (char *)(current->mm->arg_start);
	ret = copy_from_user(cmdline, cmdpos, leng);
	cmdline[leng] = '\0';
	if(ret != 0)
	{
		FELICA_PR_ERR(" %s cmdline[%s] ret[%d]", __func__, cmdline, ret);
		return -EFAULT;
	}

	if (strncmp(cmdline, gdiag_name, leng) != 0) {
		FELICA_PR_INFO(" %s ERROR, %s gdiag %s", \
			__func__, cmdline, gdiag_name);
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
		return -EACCES;
#endif
	}

	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_uid_close(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * uid registration
 */
static long felica_uid_ioctl(struct file *file, unsigned int cmd,
			     unsigned long arg)
{
	FELICA_PR_DBG(" %s START, cmd=[%d]", __func__, cmd);

	switch (cmd) {
	case SET_FELICA_UID_DIAG:
		gdiag_uid = *((int *)arg);
		FELICA_PR_INFO("Set gdiag_uid to [%d]", gdiag_uid);
		break;
	default:
		FELICA_PR_ERR(" %s ERROR(unknown command)", __func__);
		break;
	}

	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/******************************************************************************
 * /dev/felica_ant
 ******************************************************************************/

/* character device definition */
static dev_t devid_felica_ant;
static struct cdev cdev_felica_ant;
static const struct file_operations fops_felica_ant = {
	.owner = THIS_MODULE,
	.open = felica_ant_open,
	.release = felica_ant_close,
	.read = felica_ant_read,
	.write = felica_ant_write,
};

/*
 * felica_ant_init
 */
static void felica_ant_init(void)
{
	int ret;
	struct device *device_felica_ant;
	FELICA_PR_DBG(" %s START", __func__);

	devid_felica_ant = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_felica_ant, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, FELICA_ANT_NAME);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_felica_ant, &fops_felica_ant);
	ret = cdev_add(&cdev_felica_ant, devid_felica_ant, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_felica_ant, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_felica_ant =
	    device_create(felica_class, NULL, devid_felica_ant, NULL,
			  FELICA_ANT_NAME);
	if (IS_ERR(device_felica_ant)) {
		cdev_del(&cdev_felica_ant);
		unregister_chrdev_region(devid_felica_ant, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)", __func__);
		return;
	}

	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_felica_ant), MINOR(devid_felica_ant));
}

/*
 * felica_ant_exit
 */
static void felica_ant_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	device_destroy(felica_class, devid_felica_ant);
	cdev_del(&cdev_felica_ant);
	unregister_chrdev_region(devid_felica_ant, FELICA_MINOR_COUNT);

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * open device
 */
static int felica_ant_open(struct inode *inode, struct file *file)
{
	uid_t uid;
	FELICA_PR_DBG(" %s START", __func__);

	#if defined(CONFIG_MACH_HLTEDCM)
	//high
	if (system_rev == HW_REV09_OR_10)
	{
		FELICA_PR_INFO(" %s Making MHL_EN HIGH", __func__);
		of_sii8240_hw_poweron(FELICA_CEN_UNLOCK);
	}
	#endif
	
	uid = __task_cred(current)->uid;
	if ((uid != gant_uid) && (uid != gdiag_uid)) {
		FELICA_PR_ERR(\
		" %s END -EACCES, uid=[%d], gant_uid=[%d]\n", __func__, uid, gant_uid);
		
		#if defined(CONFIG_MACH_HLTEDCM)
		//low
		if (system_rev == HW_REV09_OR_10)
		{
			FELICA_PR_INFO(" %s Making MHL_EN LOW", __func__);
	//		of_sii8240_hw_poweron(FELICA_CEN_LOCK);
		}
		#endif
		
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
		return -EACCES;
#endif
	}

	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int felica_ant_close(struct inode *inode, struct file *file)
{
	/* no operation */
	#if defined(CONFIG_MACH_HLTEDCM)
	//low
	if (system_rev == HW_REV09_OR_10)
	{
		FELICA_PR_INFO(" %s Making MHL_EN LOW", __func__);
	//	of_sii8240_hw_poweron(FELICA_CEN_LOCK);
	}	
	#endif
	
	FELICA_PR_DBG(" %s END system_rev=[%d]", __func__,system_rev);
	return 0;
}

/*
 * read operation
 */
static ssize_t felica_ant_read(struct file *file, char __user *buf, \
			size_t len, loff_t *ppos)
{
	int ret;
	unsigned char address = gi2c_antaddress;
	unsigned char read_buff = 0;
	struct i2c_msg read_msgs[2];

	read_msgs[0].flags = gread_msgs[0].flags;
	read_msgs[0].len = gread_msgs[0].len;
	read_msgs[1].flags = gread_msgs[1].flags;
	read_msgs[1].len = gread_msgs[1].len;

	read_msgs[0].addr = gi2c_address;
	read_msgs[0].buf = &address;
	read_msgs[1].addr = gi2c_address;
	read_msgs[1].buf = &read_buff;

	FELICA_PR_DBG(" %s START", __func__);
	if (felica_i2c_client == NULL) {
		FELICA_PR_ERR(" %s -EIO felica_i2c_client is NULL", \
			__func__);
		return -EIO;
	}

	ret = i2c_transfer(felica_i2c_client->adapter, &read_msgs[0], 1);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(i2c_transfer[0]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}
	ret = i2c_transfer(felica_i2c_client->adapter, &read_msgs[1], 1);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(i2c_transfer[1]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}
	FELICA_PR_INFO(" %s read_buff=[%d]", __func__, read_buff);

	ret = copy_to_user(buf, &read_buff, FELICA_ANT_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	FELICA_PR_DBG(" %s END", __func__);
	return FELICA_ANT_DATA_LEN;
}

/*
 * write operation
 */
static ssize_t felica_ant_write(struct file *file, const char __user *data,
				size_t len, loff_t *ppos)
{
	char ant;
	int ret;
	unsigned char write_buff[2];
	FELICA_PR_DBG(" %s START", __func__);

	if (felica_i2c_client == NULL) {
		FELICA_PR_ERR(" %s -EIO felica_i2c_client is NULL", \
			__func__);
		return -EIO;
	}

	ret = copy_from_user(&ant, data, FELICA_ANT_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_from_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	write_buff[0] = gi2c_antaddress;
	write_buff[1] = ant;
	gwrite_msgs[0].buf = &write_buff[0];
	gwrite_msgs[0].addr = gi2c_address;
	
	ret = i2c_transfer(felica_i2c_client->adapter, gwrite_msgs, 1);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(i2c_transfer), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	FELICA_PR_DBG(" %s END", __func__);
	return FELICA_ANT_DATA_LEN;
}

/******************************************************************************
 * Mobile FeliCa device driver initialization / termination function
 ******************************************************************************/

/*
 * to set initial value to each terminal
 */
static void felica_initialize_pin(void)
{
	FELICA_PR_DBG(" %s START", __func__);

#if defined(CONFIG_ARCH_EXYNOS)
	gpio_set_value(GPIO_PINID_FELICA_PON, GPIO_VALUE_LOW);
#elif defined(CONFIG_ARCH_APQ8064)
	ice_gpiox_set(GPIO_PINID_FELICA_PON, GPIO_VALUE_LOW);
#elif defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if !defined(CONFIG_MACH_KLTE_KDI) && !defined(CONFIG_MACH_KLTE_DCM) && !defined(CONFIG_MACH_KLTE_SBM)	// We are not doing the initialization here. felica_register_device() does it and sets up the initial pin value to LOW.
	gpio_set_value(GPIO_PINID_FELICA_PON, GPIO_VALUE_LOW);
#endif
#endif

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * to set final value to each terminal
 */
static void felica_finalize_pin(void)
{
	FELICA_PR_DBG(" %s START", __func__);

#if defined(CONFIG_ARCH_EXYNOS)
	gpio_set_value(GPIO_PINID_FELICA_PON, GPIO_VALUE_LOW);
#elif defined(CONFIG_ARCH_APQ8064)
	ice_gpiox_set(GPIO_PINID_FELICA_PON, GPIO_VALUE_LOW);
#elif defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if !defined (CONFIG_MACH_KLTE_KDI) && !defined (CONFIG_MACH_KLTE_DCM) && !defined(CONFIG_MACH_KLTE_SBM)	// We are not doing the initialization here. felica_register_device() does it and sets up the initial pin value to LOW.
	gpio_set_value(GPIO_PINID_FELICA_PON, GPIO_VALUE_LOW);
#endif
#endif

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * device driver registration
 */
static void felica_register_device(void)
{
#if defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	struct device *dev = &felica_gpio_pdev->dev;
	struct device_node *np;	
#endif 	

	FELICA_PR_DBG(" %s START", __func__);

#if defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	if (!dev) 
	{
		FELICA_PR_ERR(" %s ERROR NULL", __func__);
		return;
	}
	
	np  = dev->of_node;
	
	if(system_rev <=8)
	{
	gfelica_pon_pin = of_get_named_gpio(np, "felica,pon-gpio", 0);
	if(gfelica_pon_pin < 0)
	{
		FELICA_PR_ERR(" %s ERROR(), gfelica_pon_pin=[%d]",__func__, gfelica_pon_pin);
		return;
	}

	gpio_tlmm_config(GPIO_CFG(gfelica_pon_pin, GPIOMUX_FUNC_GPIO,
					GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
	FELICA_PR_INFO(" %s gfelica_pon_pin [%d]",__func__, gpio_get_value(gfelica_pon_pin));
	}
	
	gpio_set_value(gfelica_pon_pin, GPIO_VALUE_LOW);
	
	gfelica_hsel_pin = of_get_named_gpio (np, "felica,hsel-gpio", 0);
	if(gfelica_hsel_pin < 0)
	{
		FELICA_PR_ERR(" %s ERROR(), ret=[%d]",__func__, gfelica_hsel_pin);
		return;
	}
	FELICA_PR_INFO(" %s gfelica_hsel_pin [%d]",__func__, gfelica_hsel_pin);
			
	if (system_rev >= g_uicc_initrev) 
	{
		gpio_tlmm_config(GPIO_CFG(gfelica_hsel_pin, GPIOMUX_FUNC_GPIO,
				 GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				 GPIO_CFG_ENABLE);
		FELICA_PR_INFO(" %s gfelica_hsel_pin [%d]",__func__, gpio_get_value(gfelica_hsel_pin));
	}

	gpio_set_value(gfelica_hsel_pin, GPIO_VALUE_LOW);
	
#if defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	
	gfelica_sps_pin = of_get_named_gpio(np, "felica,sps-gpio", 0);
	if(gfelica_sps_pin < 0)
	{
		FELICA_PR_ERR(" %s ERROR(), ret=[%d]",__func__, gfelica_sps_pin);
		return;
	}
	FELICA_PR_INFO(" %s gfelica_sps_pin [%d]",__func__, gfelica_sps_pin);
			
	if (system_rev >= g_uicc_initrev) 
	{
		gpio_tlmm_config(GPIO_CFG(gfelica_sps_pin, GPIOMUX_FUNC_GPIO,
				 GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				 GPIO_CFG_ENABLE);
		FELICA_PR_INFO(" %s gfelica_sps_pin [%d]",__func__, gpio_get_value(gfelica_sps_pin));
	}

#endif	
#else
	felica_int_poll_init();
#endif	//CONFIG_MACH_KLTE

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * device driver deregistration
 */
static void felica_deregister_device(void)
{
	FELICA_PR_DBG(" %s START", __func__);
	felica_uid_exit();
#if !defined(CONFIG_ARCH_MSM8974) && !defined(CONFIG_ARCH_MSM8974PRO)
	felica_int_poll_exit();
#endif

	felica_ant_exit();
	felica_rws_exit();
	felica_rfs_exit();
	felica_cen_exit();
	felica_pon_exit();
	felica_uart_exit();

	FELICA_PR_DBG(" %s END", __func__);
}


#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
static struct of_device_id felica_of_match[] = 
{
	{	.compatible = "felica,gpio-table",
	},
	{}
};
static int __devexit felica_gpio_remove(struct platform_device *pdev)
{
	felica_int_poll_exit();
	return 0;
}
static struct platform_driver felica_of_driver = 
{
	.remove = __devexit_p(felica_gpio_remove),
	.driver = 
		{
		.name = FELICA_GPIO_DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = felica_of_match,
		},
};

static int __init felica_gpio_probe(struct platform_device *pdev)
{
	felica_gpio_pdev = pdev;
	if (felica_gpio_pdev == NULL) {
		FELICA_PR_ERR(" %s -EIO felica_gpio_pdev is NULL", \
			__func__);
		return -EIO;
	}
	felica_register_device();
	schedule_delayed_work(&pgint_irq->work, msecs_to_jiffies(10));
	return 0;
}
#endif

/*
 * The entry point for initialization module
 */
static int __init felica_init(void)
{
	FELICA_PR_DBG(" %s START", __func__);

#ifdef F_WAKE_LOCK
	wake_lock_init(&felica_wake_1, WAKE_LOCK_SUSPEND, "felica-int-1");
	wake_lock_init(&felica_wake_2, WAKE_LOCK_SUSPEND, "felica-int-2");
#endif
	felica_class = class_create(THIS_MODULE, "felica");
	if (IS_ERR(felica_class)) {
		FELICA_PR_ERR(" %s ERROR(class_create)", __func__);
		return PTR_ERR(felica_class);
	}

	felica_initialize_pin();
#if !defined(CONFIG_ARCH_MSM8974) && !defined(CONFIG_ARCH_MSM8974PRO)
	felica_register_device();
#endif
	felica_nl_init();
	felica_i2c_init();
	uicc_init();
	/* MFC UID registration */
#if !defined(CONFIG_ARCH_MSM8974) && !defined(CONFIG_ARCH_MSM8974PRO)
	schedule_delayed_work(&pgint_irq->work, msecs_to_jiffies(10));
#endif

#ifdef CONFIG_NFC_FELICA

	/* FELICA_INTU GPIO is changed Start*/
#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
	felica_varying_gpio_intu = GPIO_PINID_NFC_INTU;
#else
	if (system_rev > 0 && system_rev < 15)
		felica_varying_gpio_intu = GPIO_PINID_NFC_INTU_REV03;
	else
		felica_varying_gpio_intu = GPIO_PINID_NFC_INTU_REV06;
#endif
	FELICA_PR_INFO(" %s , system_rev=[%d]", __func__, system_rev);
	/* FELICA_INTU GPIO is changed End */
#if defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	felica_int_poll_init();
#endif // CONFIG_MACH_KLTE
	snfc_register_device();
	uartcc_init();

#endif /* CONFIG_NFC_FELICA */

	FELICA_PR_DBG(" %s END", __func__);
#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
	return platform_driver_probe(&felica_of_driver, felica_gpio_probe);
#else
	return 0;
#endif
}

/*
 * The entry point for the termination module
 */
static void __exit felica_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

#ifdef F_WAKE_LOCK
	wake_lock_destroy(&felica_wake_1);
	wake_lock_destroy(&felica_wake_2);
#endif
	uicc_exit();
	felica_i2c_exit();
	felica_nl_exit();
	felica_deregister_device();
#ifdef CONFIG_NFC_FELICA
	snfc_deregister_device();
	snfc_uart_exit();
	uartcc_exit();
	snfc_cen_exit();
#endif /* CONFIG_NFC_FELICA */
	felica_finalize_pin();
	class_destroy(felica_class);

	FELICA_PR_DBG(" %s END", __func__);
}


/******************************************************************************
 *
 *
 *
 *
 * Add driver code for CXD2235AGG NFC/Felica combo IC
 *
 *
 *
 *
 ******************************************************************************/
#ifdef CONFIG_NFC_FELICA

/******************************************************************************
 * /dev/snfc_hsel
 ******************************************************************************/

/* character device definition */
static dev_t devid_snfc_hsel;
static struct cdev			hsel_cdev;
static const struct file_operations hsel_fileops = {
	.owner   = THIS_MODULE,
	.read    = hsel_read,
	.write   = hsel_write,
	.open    = hsel_open,
	.release = hsel_release,
};

/* function prototype */
static int hsel_init(void)
{
	int				ret = 0;
	struct device *class_dev;
	devid_snfc_hsel = MKDEV(MISC_MAJOR, 0);

	FELICA_PR_DBG(" %s START", __func__);

	ret = alloc_chrdev_region(&devid_snfc_hsel, 0, HSEL_DEV_COUNT,
					HSEL_DEV_NAME);

	if (0 != ret) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
					__func__, ret);
		return ret;
	}

	cdev_init(&hsel_cdev, &hsel_fileops);
	hsel_cdev.owner = THIS_MODULE;

	ret = cdev_add(&hsel_cdev, devid_snfc_hsel, HSEL_DEV_COUNT);
	if (0 != ret) {
		unregister_chrdev_region(devid_snfc_hsel, HSEL_DEV_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]",
					__func__, ret);
		return ret;
	}

	class_dev = device_create(felica_class, NULL, devid_snfc_hsel,
					NULL, HSEL_DEV_NAME);
	if (0 != IS_ERR(class_dev)) {
		cdev_del(&hsel_cdev);
		unregister_chrdev_region(devid_snfc_hsel, HSEL_DEV_COUNT);
		ret = PTR_ERR(class_dev);
		FELICA_PR_ERR(" %s ERROR(device_create), ret=[%d]",
					__func__, ret);
		return ret;
	}

	FELICA_PR_DBG(" %s END", __func__);

	return ret;
}

/*
 * finalize device
 */
static void hsel_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	device_destroy(felica_class, devid_snfc_hsel);

	cdev_del(&hsel_cdev);
	unregister_chrdev_region(devid_snfc_hsel, HSEL_DEV_COUNT);

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * open device
 */
static int hsel_open(struct inode *inode, struct file *file)
{
	FELICA_PR_DBG(" %s START", __func__);
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int hsel_release(struct inode *inode, struct file *file)
{
	FELICA_PR_DBG(" %s START", __func__);
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}
/*
 * read operation
 */
static ssize_t hsel_read(struct file *file, char __user *buf, size_t len,
					loff_t *ppos)
{
	char	hsel_val;
	int		ret = -EINVAL;

	FELICA_PR_DBG(" %s START", __func__);

	if (NULL == buf) {
		FELICA_PR_ERR(" %s hsel_read param err", __func__);
		return -EIO;
	}

	if (1 > len) {
		FELICA_PR_ERR(
			" %s length check len = %d", __func__, len);
		return -EIO;
	}

#if defined(CONFIG_ARCH_EXYNOS)
	ret = gpio_get_value(GPIO_PINID_NFC_HSEL);
#elif defined(CONFIG_ARCH_APQ8064)
	ret = ice_gpiox_get(GPIO_PINID_NFC_HSEL);
#elif defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
	ret = gpio_get_value(gfelica_hsel_pin);
	#endif
	if (0 > ret) {
		FELICA_PR_ERR(" %s ERROR(gpio_get_value), ret=[%d]",
					__func__, ret);
		return ret;
	}
	if (0 == ret)
		hsel_val = 0;
	else
		hsel_val = 1;

	if (copy_to_user(buf, &hsel_val, 1)) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user)", __func__);
		return -EFAULT;
	}

	FELICA_PR_DBG(" %s END", __func__);

	return 1;
}

/*
 * write operation
 */
static ssize_t hsel_write(struct file *file, const char __user *data,\
				size_t len, loff_t *ppos)
{
	char	hsel_val;

	FELICA_PR_DBG(" %s START", __func__);

	if (NULL == data) {
		FELICA_PR_ERR(" %s hsel_write param err", __func__);
		return -EIO;
	}

	if (1 > len) {
		FELICA_PR_ERR(
			" %s length check len = %d",
			__func__, len);
		return -EIO;
	}

	if (copy_from_user(&hsel_val, data, 1)) {
		FELICA_PR_ERR(" %s ERROR(copy_from_user)", __func__);
		return -EFAULT;
	}
	if (0 == hsel_val) {
		FELICA_PR_INFO(" %s HSEL = Low", __func__);
	} else if (1 == hsel_val) {
		FELICA_PR_INFO(" %s HSEL = High", __func__);
	} else {
		FELICA_PR_ERR(" param err = %d", hsel_val);
		return -EIO;
	}

#if defined(CONFIG_ARCH_EXYNOS)
	gpio_set_value(GPIO_PINID_NFC_HSEL , hsel_val);
#elif defined(CONFIG_ARCH_APQ8064)
	ice_gpiox_set(GPIO_PINID_NFC_HSEL , hsel_val);
#elif defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
	gpio_set_value(gfelica_hsel_pin , hsel_val);
    #endif

	FELICA_PR_DBG(" %s END", __func__);
	return 1;
}


/******************************************************************************
 * /dev/snfc_intu_poll
 ******************************************************************************/

/* character device definition */
static dev_t devid_snfc_intu_poll;
static struct cdev			intu_poll_cdev;
static struct poll_data		g_intu_data;
static struct poll_data		*g_intu_d = &g_intu_data;
static const struct file_operations intu_poll_fileops = {
	.owner   = THIS_MODULE,
	.read    = intu_poll_read,
	.open    = intu_poll_open,
	.release = intu_poll_release,
/*	.poll    = intu_poll_poll,*/
};

/*
 * top half of irq_handler
 */
static irqreturn_t intu_poll_irq_handler(int irq, void *dev_id)
{
	struct poll_data *intu_d = g_intu_d;

	FELICA_PR_DBG(" %s START irq = %d", __func__, irq);

	disable_irq_nosync(gpio_to_irq(felica_varying_gpio_intu));

	schedule_delayed_work(&intu_d->work, 0);

	FELICA_PR_DBG(" %s END", __func__);

	return IRQ_HANDLED;
}

/*
 * bottom half of irq_handler
 */
static void intu_poll_work_func(struct work_struct *work)
{
	struct poll_data *intu_d = g_intu_d;
	int read_value			= 0;
	int old_value			= 0;
	unsigned long irqflag	= 0;

	FELICA_PR_DBG(" %s START", __func__);

	old_value = intu_d->device_status;

	read_value = gpio_get_value(felica_varying_gpio_intu);

	FELICA_PR_INFO(" %s read_value = %d old_value = %d",
					__func__, read_value, old_value);

	if (read_value < 0) {
		intu_d->read_error = read_value;
	} else if (read_value != old_value) {
		intu_d->device_status = read_value;
		intu_d->read_error = 0;

		if (intu_d->device_status == INTU_POLL_DEV_LOW)
			irqflag = IRQF_TRIGGER_HIGH | IRQF_SHARED;
		else
			irqflag = IRQF_TRIGGER_LOW | IRQF_SHARED;

		if (irq_set_irq_type(gpio_to_irq(felica_varying_gpio_intu),
				irqflag)) {
			FELICA_PR_ERR(
				" %s ERROR(set_irq_type), irqflag=[%ld]",
				__func__, irqflag);
		}
	}

	enable_irq(gpio_to_irq(felica_varying_gpio_intu));

	if (read_value != old_value || intu_d->read_error) {
		intu_d->irq_handler_done = 1;
		wake_up_interruptible(&intu_d->read_wait);
	}
	FELICA_PR_DBG(
		" %s END read_value = %d, old_value = %d,	intu_d->read_error = %d",
		__func__, read_value, old_value, intu_d->read_error);
}

/*
 * initialize device
 */
static int intu_poll_init(void)
{
	int ret = 0;
	struct device *class_dev;
	unsigned long irqflag = 0;

	devid_snfc_intu_poll = MKDEV(MISC_MAJOR, 0);

	FELICA_PR_DBG(" %s START", __func__);

	ret = alloc_chrdev_region(&devid_snfc_intu_poll,
		0, INTU_POLL_DEV_COUNT, INTU_POLL_DEV_NAME);
	if (ret) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
						__func__, ret);
		return ret;
	}

	cdev_init(&intu_poll_cdev, &intu_poll_fileops);
	intu_poll_cdev.owner = THIS_MODULE;

	ret = cdev_add(&intu_poll_cdev,
		devid_snfc_intu_poll,
		INTU_POLL_DEV_COUNT);
	if (ret) {
		unregister_chrdev_region(
			devid_snfc_intu_poll,
			INTU_POLL_DEV_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]",
						__func__, ret);
		return ret;
	}

	class_dev = device_create(felica_class,
		NULL, devid_snfc_intu_poll, NULL,
				 INTU_POLL_DEV_NAME);
	if (IS_ERR(class_dev)) {
		cdev_del(&intu_poll_cdev);
		unregister_chrdev_region(
			devid_snfc_intu_poll, INTU_POLL_DEV_COUNT);
		ret = PTR_ERR(class_dev);
		FELICA_PR_ERR(" %s ERROR(device_create), ret=[%d]",
						__func__, ret);
		return ret;
	}

	memset(g_intu_d, 0x00, sizeof(struct poll_data));

	INIT_DELAYED_WORK(&g_intu_d->work, intu_poll_work_func);

	init_waitqueue_head(&g_intu_d->read_wait);

	g_intu_d->open_flag = 0;

	if (g_intu_d->open_flag) {
		FELICA_PR_ERR(" %s only one time", __func__);
		return 0;
	}
	g_intu_d->open_flag = 1;

	ret = gpio_get_value(felica_varying_gpio_intu);
	if (ret < 0) {
		g_intu_d->open_flag = 0;
		FELICA_PR_ERR(" %s ERROR(gpio_get_value), ret=[%d]",
						__func__, ret);
		return -EIO;
	}
	g_intu_d->device_status = ret;

	if (g_intu_d->device_status == INTU_POLL_DEV_LOW)
		irqflag = IRQF_TRIGGER_HIGH | IRQF_SHARED;
	else
		irqflag = IRQF_TRIGGER_LOW | IRQF_SHARED;

	if (request_irq(gpio_to_irq(felica_varying_gpio_intu),
			intu_poll_irq_handler, irqflag, INTU_POLL_DEV_NAME,
			(void *)g_intu_d)) {
		g_intu_d->open_flag = 0;
		FELICA_PR_ERR(" %s ERROR(request_irq), irqflag=[%ld]",
						__func__, irqflag);
		return -EIO;
	}
	g_intu_d->irq_handler_done = 0;

	FELICA_PR_DBG(" %s END", __func__);

	return ret;
}

/*
 * finalize device
 */
static void intu_poll_exit(void)
{
	struct poll_data *intu_d = g_intu_d;

	FELICA_PR_DBG(" %s START", __func__);

	cancel_delayed_work(&intu_d->work);
	free_irq(gpio_to_irq(felica_varying_gpio_intu), (void *)intu_d);

	intu_d->open_flag = 0;

	cdev_del(&intu_poll_cdev);
	unregister_chrdev_region(devid_snfc_intu_poll, INTU_POLL_DEV_COUNT);
	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * open device
 */
static int intu_poll_open(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END", __func__);

	return 0;
}

/*
 * close device
 */
static int intu_poll_release(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END", __func__);

	return 0;
}

/*
 * read operation
 */
static ssize_t intu_poll_read(struct file *file, char __user *buf,
			size_t len, loff_t *ppos)
{
	struct poll_data *intu_d = g_intu_d;
	int ret;
	char cret;

	FELICA_PR_DBG(" %s START", __func__);

	if (len < 1) {
		FELICA_PR_ERR(
			" %s length check len = %d", __func__, len);
		return -EIO;
	}

	if (!intu_d->irq_handler_done) {
		if (file->f_flags & O_NONBLOCK) {
			FELICA_PR_ERR(" NONBLOCK");
			return -EAGAIN;
		}
		FELICA_PR_INFO(" NFC intu_poll wait irq");
		ret = wait_event_interruptible(intu_d->read_wait,
						intu_d->irq_handler_done == 1);
		if (-ERESTARTSYS == ret) {
			FELICA_PR_ERR(
				" %s -EINTR wait_event_interruptible ret = %d",
				__func__, ret);
			return -EINTR;
		}
	}

	if (intu_d->read_error) {
		intu_d->irq_handler_done = 0;
		intu_d->read_error = 0;
		FELICA_PR_ERR(" %s intu_d->read_error = %d",
						__func__, intu_d->read_error);
		return -EIO;
	}

	if (intu_d->device_status == INTU_POLL_DEV_HIGH)
		cret = 1;
	else
		cret = 0;

	len = 1;

	if (copy_to_user(buf, &cret, len)) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user)", __func__);
		return -EFAULT;
	}
	intu_d->irq_handler_done = 0;

	FELICA_PR_DBG(
		" %s END len = %d, cret = %d", __func__, len, cret);

	return len;
}


/******************************************************************************
 * /dev/snfc_available_poll
 ******************************************************************************/

/* character device definition */
static dev_t devid_snfc_abailable_poll;
static struct cdev available_poll_cdev;
static const struct file_operations available_poll_fileops = {
	.owner   = THIS_MODULE,
	.unlocked_ioctl   = available_poll_ioctl,
	.read    = available_poll_read,
	.open    = available_poll_open,
	.release = available_poll_release,
};

/*
 * initialize device
 */
static int available_poll_init(void)
{
	int           ret      = 0;
	struct device *class_dev;

	devid_snfc_abailable_poll = MKDEV(MISC_MAJOR, 0);

	FELICA_PR_DBG(" %s START", __func__);

	ret = alloc_chrdev_region(&devid_snfc_abailable_poll, 0,
		 AVAILABLE_POLL_DEV_COUNT, AVAILABLE_POLL_DEV_NAME);
	if (ret) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
						__func__, ret);
		return ret;
	}

	cdev_init(&available_poll_cdev, &available_poll_fileops);
	available_poll_cdev.owner = THIS_MODULE;

	ret = cdev_add(&available_poll_cdev, devid_snfc_abailable_poll,
					AVAILABLE_POLL_DEV_COUNT);
	if (ret) {
		unregister_chrdev_region(devid_snfc_abailable_poll,
					AVAILABLE_POLL_DEV_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]",
						__func__, ret);
		return ret;
	}

	class_dev = device_create(felica_class, NULL, devid_snfc_abailable_poll,
					NULL, AVAILABLE_POLL_DEV_NAME);

	if (IS_ERR(class_dev)) {
		cdev_del(&available_poll_cdev);
		unregister_chrdev_region(devid_snfc_abailable_poll,
						AVAILABLE_POLL_DEV_COUNT);
		ret = PTR_ERR(class_dev);
		FELICA_PR_ERR(" %s ERROR(device_create), ret=[%d]",
						__func__, ret);
		return ret;
	}

	memset((void *)&g_available_data, 0x00, sizeof(struct icc_poll_data));

	init_waitqueue_head(&g_available_data.read_wait);

	FELICA_PR_DBG(" %s END", __func__);

	return ret;
}

/*
 * finalize device
 */
static void available_poll_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	device_destroy(felica_class, devid_snfc_abailable_poll);

	cdev_del(&available_poll_cdev);
	unregister_chrdev_region(devid_snfc_abailable_poll,
					AVAILABLE_POLL_DEV_COUNT);

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * open device
 */
static int available_poll_open(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int available_poll_release(struct inode *inode, struct file *file)
{
	FELICA_PR_DBG(" %s START", __func__);
	if (g_available_data.available_flag == 1) {
		g_available_data.rsp_done = 1;
		wake_up_interruptible(&g_available_data.read_wait);
	}
	g_available_data.rsp_done = 0;
	g_available_data.available_flag = 0;
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t available_poll_read(struct file *file, char __user *buf,
				size_t len, loff_t *ppos)
{
	char	type = 0;
	int		nRet = -1;
	struct icc_poll_data *available_d = &g_available_data;

	FELICA_PR_DBG(" %s START", __func__);

	if ((!uartcc_is_idle_status()) ||
		(0 == g_cen_sts) ||
		(GPIO_VALUE_LOW == g_rfs_sts)) {

		available_d->available_flag = 1;
		nRet = wait_event_interruptible(available_d->read_wait,
					available_d->rsp_done == 1);
		if (nRet < 0) {
			FELICA_PR_ERR(
				" %s ERROR(wait_event_interruptible), ret=[%d]"
				, __func__, nRet);
			return -EIO;
		}
		type = 0x01;
		available_d->rsp_done		= 0;
		available_d->available_flag	= 0;
	} else {
		type = 0x01;
	}

	if (copy_to_user(buf, &type, len)) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user)", __func__);
		return -EFAULT;
	}

	FELICA_PR_DBG(" %s END read size = %d ", __func__, len);

	return len;
}

/*
 * available operation
 */
static long available_poll_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	long			nRet	= -EIO;

	FELICA_PR_DBG(" %s  START cmd = %x", __func__, cmd);
	switch (cmd) {
	case AVAILABLE_POLL_DEV_SET_PRE_LOCK:
		g_cen_sts = 0;
		nRet = 0;
		break;
	default:
		break;
	}
	FELICA_PR_DBG(" %s  END cmd = %x", __func__, cmd);
	return nRet;
}


/******************************************************************************
 * /dev/snfc_rfs_poll
 ******************************************************************************/

/* character device definition */
static dev_t devid_snfc_rfs_poll;
static struct cdev			rfs_poll_cdev;
static struct poll_data		g_rfs_data;
static struct poll_data		*g_rfs_d = &g_rfs_data;
static const struct file_operations rfs_poll_fileops = {
	.owner   = THIS_MODULE,
	.read    = rfs_poll_read,
	.open    = rfs_poll_open,
	.release = rfs_poll_release,
	.poll    = rfs_poll_poll,
};

/*
 * top half of irq_handler
 */
static irqreturn_t rfs_poll_irq_handler(int irq, void *dev_id)
{
	struct poll_data *rfs_d = g_rfs_d;

	FELICA_PR_DBG(" %s START irq = %d", __func__, irq);

	disable_irq_nosync(gpio_to_irq(GPIO_PINID_NFC_RFS_POLL));

	schedule_delayed_work(&rfs_d->work, msecs_to_jiffies(RFS_POLL_DELAY));

	FELICA_PR_DBG(" %s END", __func__);

	return IRQ_HANDLED;
}

/*
 * bottom half of irq_handler
 */
static void rfs_poll_work_func(struct work_struct *work)
{
	struct poll_data *rfs_d = g_rfs_d;
	int read_value	= 0;
	int old_value	= 0;
	unsigned long irqflag = 0;
	struct icc_poll_data *available_d = &g_available_data;

	FELICA_PR_DBG(" %s START", __func__);

	old_value = rfs_d->device_status;

	read_value = gpio_get_value(GPIO_PINID_NFC_RFS_POLL);

	FELICA_PR_INFO(" %s read_value = %d old_value = %d",
					__func__ , read_value, old_value);

	if (read_value < 0) {
		rfs_d->read_error = read_value;
	} else if (read_value != old_value) {
		g_rfs_sts = read_value;
		if (1 == available_d->available_flag) {
			if (0 != g_cen_sts) {
				if (GPIO_VALUE_LOW != g_rfs_sts) {
					if (uartcc_is_idle_status()) {
						FELICA_PR_INFO(
						" %s wake up available",
						__func__);
						available_d->rsp_done = 1;
						wake_up_interruptible(
						&available_d->read_wait);
					}
				}
			}
		}

		rfs_d->device_status = read_value;
		rfs_d->read_error = 0;

		if (rfs_d->device_status == RFS_POLL_DEV_LOW)
			irqflag = IRQF_TRIGGER_HIGH | IRQF_SHARED;
		else
			irqflag = IRQF_TRIGGER_LOW | IRQF_SHARED;

		if (irq_set_irq_type(
			gpio_to_irq(GPIO_PINID_NFC_RFS_POLL), irqflag)) {
			FELICA_PR_ERR(
				" %s ERROR(set_irq_type), irqflag=[%ld]",
				__func__, irqflag);
		}
	}

	enable_irq(gpio_to_irq(GPIO_PINID_NFC_RFS_POLL));

	if (read_value != old_value || rfs_d->read_error) {
		rfs_d->irq_handler_done = 1;
		wake_up_interruptible(&rfs_d->read_wait);
	}

	FELICA_PR_INFO(
		" %s END read_value = %d,old_value = %d,rfs_d->read_error = %d",
		__func__, read_value, old_value, rfs_d->read_error);
}

/*
 * initialize device
 */
static int rfs_poll_init(void)
{
	int ret = 0;
	struct device *class_dev;
	unsigned long irqflag = 0;

	devid_snfc_rfs_poll = MKDEV(MISC_MAJOR, 0);

	FELICA_PR_DBG(" %s START", __func__);

	ret = alloc_chrdev_region(&devid_snfc_rfs_poll , 0 , RFS_POLL_DEV_COUNT,
					RFS_POLL_DEV_NAME);
	if (ret) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region) ret=[%d]",
						__func__, ret);
		return ret;
	}

	cdev_init(&rfs_poll_cdev, &rfs_poll_fileops);
	rfs_poll_cdev.owner = THIS_MODULE;

	ret = cdev_add(&rfs_poll_cdev, devid_snfc_rfs_poll, RFS_POLL_DEV_COUNT);
	if (ret) {
		unregister_chrdev_region(
			devid_snfc_rfs_poll, RFS_POLL_DEV_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]",
						__func__, ret);
		return ret;
	}

	class_dev = device_create(felica_class, NULL, devid_snfc_rfs_poll, NULL,
					RFS_POLL_DEV_NAME);

	if (IS_ERR(class_dev)) {
		cdev_del(&rfs_poll_cdev);
		unregister_chrdev_region(
			devid_snfc_rfs_poll, RFS_POLL_DEV_COUNT);
		ret = PTR_ERR(class_dev);
		FELICA_PR_ERR(" %s ERROR(device_create), ret=[%d]",
						__func__, ret);
		return ret;
	}

	memset(g_rfs_d, 0x00, sizeof(struct poll_data));

	INIT_DELAYED_WORK(&g_rfs_d->work, rfs_poll_work_func);

	init_waitqueue_head(&g_rfs_d->read_wait);

	g_rfs_d->open_flag = 0;

	if (g_rfs_d->open_flag) {
		FELICA_PR_ERR(" %s only one time", __func__);
		return -EBUSY;
	}
	g_rfs_d->open_flag = 1;

	ret = gpio_get_value(GPIO_PINID_NFC_RFS_POLL);
	if (ret < 0) {
		g_rfs_d->open_flag = 0;
		FELICA_PR_ERR(" %s ERROR(gpio_get_value), ret=[%d]",
						__func__, ret);
		return -EIO;
	}
	g_rfs_d->device_status = ret;
	g_rfs_sts = ret;

	if (g_rfs_d->device_status == RFS_POLL_DEV_LOW)
		irqflag = IRQF_TRIGGER_HIGH | IRQF_SHARED;
	else
		irqflag = IRQF_TRIGGER_LOW | IRQF_SHARED;

	if (request_irq(gpio_to_irq(
		GPIO_PINID_NFC_RFS_POLL), rfs_poll_irq_handler,
		irqflag, RFS_POLL_DEV_NAME, (void *)g_rfs_d)) {
		g_rfs_d->open_flag = 0;
		FELICA_PR_ERR(" %s ERROR(request_irq), irqflag=[%ld]",
						__func__, irqflag);
		return -EIO;
	}
	g_rfs_d->irq_handler_done = 0;

	FELICA_PR_DBG(" %s END", __func__);

	return ret;
}

/*
 * finalize device
 */
static void rfs_poll_exit(void)
{
	struct poll_data *rfs_d = g_rfs_d;

	FELICA_PR_DBG(" %s START", __func__);

	cancel_delayed_work(&rfs_d->work);
	free_irq(gpio_to_irq(GPIO_PINID_NFC_RFS_POLL), (void *)rfs_d);

	rfs_d->open_flag = 0;

	cdev_del(&rfs_poll_cdev);
	unregister_chrdev_region(
		devid_snfc_rfs_poll, RFS_POLL_DEV_COUNT);

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * open operation
 */
static int rfs_poll_open(struct inode *inode, struct file *file)
{
	int uid_ret = 0;

	FELICA_PR_DBG(" %s START", __func__);

	uid_ret = snfc_uid_check();
	if (uid_ret < 0) {
		FELICA_PR_ERR
		    (" %s open fail=[%d]", __func__, uid_ret);
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
		return -EACCES;
#endif
	}

	FELICA_PR_DBG(" %s END", __func__);

	return 0;
}

/*
 * close operation
 */
static int rfs_poll_release(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END", __func__);

	return 0;
}

/*
 * poll operation
 */
static unsigned int rfs_poll_poll(struct file *file, poll_table *wait)
{
	struct poll_data *rfs_d = g_rfs_d;
	unsigned int mask = 0;

	FELICA_PR_DBG(" %s START", __func__);

	poll_wait(file, &rfs_d->read_wait, wait);
	if (rfs_d->irq_handler_done)
		mask = POLLIN | POLLRDNORM;

	FELICA_PR_DBG(" %s END mask = %d", __func__, mask);

	return mask;
}

/*
 * read operation
 */
static ssize_t rfs_poll_read(struct file *file, char __user *buf, size_t len,
					loff_t *ppos)
{
	struct poll_data *rfs_d = g_rfs_d;
	int ret;
	char cret;

	FELICA_PR_DBG(" %s START", __func__);

	if (len < 1) {
		FELICA_PR_ERR(
			" %s length check len = %d", __func__, len);
		return -EIO;
	}

	if (!rfs_d->irq_handler_done) {
		if (file->f_flags & O_NONBLOCK) {
			FELICA_PR_ERR(" %s NONBLOCK", __func__);
			return -EAGAIN;
		}
		FELICA_PR_INFO(
			" %s FeliCa rfs_poll wait irq", __func__);
		ret = wait_event_interruptible(rfs_d->read_wait,
						rfs_d->irq_handler_done == 1);
		if (-ERESTARTSYS == ret) {
			FELICA_PR_ERR(
			" %s -EINTR wait_event_interruptible ret=[%d]",
			__func__, ret);
			return -EINTR;
		}
	}

	if (rfs_d->read_error) {
		rfs_d->irq_handler_done = 0;
		rfs_d->read_error = 0;
		FELICA_PR_ERR(" %s rfs_d->read_error = %d",
						__func__, rfs_d->read_error);
		return -EIO;
	}

	if (rfs_d->device_status == RFS_POLL_DEV_HIGH)
		cret = RFS_POLL_RET_STS_HIGH;
	else
		cret = RFS_POLL_RET_STS_LOW;

	len = 1;

	if (copy_to_user(buf, &cret, len)) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user)", __func__);
		return -EFAULT;
	}
	rfs_d->irq_handler_done = 0;

	FELICA_PR_DBG(
		" %s END len = %d, cret = %d",
		__func__, len, cret);

	return len;
}


/******************************************************************************
 * /dev/snfc_pon
 ******************************************************************************/

/* character device definition */
static dev_t devid_snfc_pon;
static struct cdev			cxd2235power_cdev;
static const struct file_operations cxd2235power_fileops = {
	.owner   = THIS_MODULE,
	.write   = cxd2235power_write,
	.open    = cxd2235power_open,
	.release = cxd2235power_release,
};

/*
 * initialize device
 */
static int cxd2235power_init(void)
{
	int				ret = 0;
	struct device	*class_dev;

	devid_snfc_pon = MKDEV(MISC_MAJOR, 0);

	FELICA_PR_DBG(" %s START", __func__);

	ret = alloc_chrdev_region(&devid_snfc_pon , 0 , CXD2235_POWER_DEV_COUNT,
					CXD2235_POWER_DEV_NAME);
	if (ret) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
						__func__, ret);
		return ret;
	}

	cdev_init(&cxd2235power_cdev, &cxd2235power_fileops);
	cxd2235power_cdev.owner = THIS_MODULE;

	ret = cdev_add(&cxd2235power_cdev, devid_snfc_pon,
					CXD2235_POWER_DEV_COUNT);
	if (ret) {
		unregister_chrdev_region(
			devid_snfc_pon, CXD2235_POWER_DEV_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]",
						__func__, ret);
		return ret;
	}

	class_dev = device_create(felica_class, NULL, devid_snfc_pon, NULL,
					CXD2235_POWER_DEV_NAME);
	if (IS_ERR(class_dev)) {
		cdev_del(&cxd2235power_cdev);
		unregister_chrdev_region(
			devid_snfc_pon, CXD2235_POWER_DEV_COUNT);
		ret = PTR_ERR(class_dev);
		FELICA_PR_ERR(" %s ERROR(device_create), ret=[%d]",
						__func__, ret);
		return ret;
	}

	FELICA_PR_DBG(" %s END", __func__);

	return ret;
}

/*
 * finalize device
 */
static void cxd2235power_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	device_destroy(felica_class, devid_snfc_pon);
	cdev_del(&cxd2235power_cdev);
	unregister_chrdev_region(devid_snfc_pon, CXD2235_POWER_DEV_COUNT);

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * open operation
 */
static int cxd2235power_open(struct inode *inode, struct file *file)
{
	int uid_ret = 0;

	FELICA_PR_DBG(" %s START", __func__);

	uid_ret = snfc_uid_check();
	if (uid_ret < 0) {
		FELICA_PR_ERR
		    (" %s open fail=[%d]", __func__, uid_ret);
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
		return -EACCES;
#endif
	}

	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * close operation
 */
static int cxd2235power_release(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END", __func__);

	return 0;
}

/*
 * write operation
 */
static ssize_t cxd2235power_write(struct file *file, const char __user *data,
				size_t len, loff_t *ppos)
{
	char on;

	FELICA_PR_DBG(" %s START", __func__);

	if (len < 1) {
		FELICA_PR_ERR(
			" %s length check len = %d", __func__, len);
		return -EIO;
	}
	len = 1;
	if (copy_from_user(&on, data, len)) {
		FELICA_PR_ERR(" %s ERROR(copy_from_user)", __func__);
		return -EFAULT;
	}

#if defined(CONFIG_ARCH_EXYNOS)
	gpio_set_value(GPIO_PINID_NFC_PON , on);
#elif defined(CONFIG_ARCH_APQ8064)
	ice_gpiox_set(GPIO_PINID_NFC_PON, on);
#elif defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8974PRO)
#if defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_SBM)
	gpio_set_value(gfelica_pon_pin , on);	// PON is same for FELICA and NFC.
#else
	gpio_set_value(GPIO_PINID_NFC_PON , on);
#endif	// CONFIG_MACH_KLTE
#endif

	FELICA_PR_DBG(" %s END on = %d", __func__, on);

	return len;
}


/******************************************************************************
 * /dev/snfc_rfs
 ******************************************************************************/

/* character device definition */
static dev_t devid_snfc_rfs;
static struct cdev cdev_snfc_rfs;
static const struct file_operations fops_snfc_rfs = {
	.owner = THIS_MODULE,
	.open = snfc_rfs_open,
	.release = snfc_rfs_close,
	.read = snfc_rfs_read,
};

/*
 * initialize device
 */
static void snfc_rfs_init(void)
{
	int ret;
	struct device *device_snfc_rfs;
	FELICA_PR_DBG(" %s START", __func__);

	devid_snfc_rfs = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_snfc_rfs, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, SNFC_RFS_NAME);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_snfc_rfs, &fops_snfc_rfs);
	ret = cdev_add(&cdev_snfc_rfs, devid_snfc_rfs, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_snfc_rfs, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_snfc_rfs =
	    device_create(felica_class, NULL, devid_snfc_rfs, NULL,
			  SNFC_RFS_NAME);
	if (IS_ERR(device_snfc_rfs)) {
		cdev_del(&cdev_snfc_rfs);
		unregister_chrdev_region(devid_snfc_rfs, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)", __func__);
		return;
	}

	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_snfc_rfs), MINOR(devid_snfc_rfs));
}

/*
 * finalize device
 */
static void snfc_rfs_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	device_destroy(felica_class, devid_snfc_rfs);
	cdev_del(&cdev_snfc_rfs);
	unregister_chrdev_region(devid_snfc_rfs, FELICA_MINOR_COUNT);
}

/*
 * open device
 */
static int snfc_rfs_open(struct inode *inode, struct file *file)
{
	int uid_ret = 0;

	FELICA_PR_DBG(" %s START", __func__);

	uid_ret = snfc_uid_check();
	if (uid_ret < 0) {
		FELICA_PR_ERR
		    (" %s open fail=[%d]", __func__, uid_ret);
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
		return -EACCES;
#endif
	}

	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int snfc_rfs_close(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * read operation
 */
static ssize_t snfc_rfs_read(struct file *file, char __user *buf, \
				size_t len, loff_t *ppos)
{
	int ret;
	char retparam;
	FELICA_PR_DBG(" %s START", __func__);

	ret = gpio_get_value(GPIO_PINID_SNFC_RFS);

	if (ret == GPIO_VALUE_HIGH) {
		retparam = SNFC_RFS_STANDBY;
		FELICA_PR_INFO(" Carrier-Wave Status is [%d][standby]",
				 retparam);
	} else if (ret == GPIO_VALUE_LOW) {
		retparam = SNFC_RFS_DETECTED;
		FELICA_PR_INFO(" Carrier-Wave Status is [%d][detected]",
				 retparam);
	} else {
		FELICA_PR_ERR(" %s ERROR(gpio_get_value), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	ret = copy_to_user(buf, &retparam, SNFC_RFS_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	FELICA_PR_DBG(" %s END", __func__);
	return SNFC_RFS_DATA_LEN;
}

/******************************************************************************
 * /dev/snfc_uart
 ******************************************************************************/
/* character device definition */
static dev_t devid_snfc;
static struct cdev cdev_snfc;
static const struct file_operations fops_snfc = {
	.owner = THIS_MODULE,
	.open = snfc_uart_open,
	.release = snfc_uart_close,
	.read = snfc_uart_read,
	.write = snfc_uart_write,
	.unlocked_ioctl = snfc_uart_ioctl,
	.poll = snfc_uart_poll,
};

/*
 * initialize device
 */
static void snfc_uart_init(void)
{
	int ret;
	struct device *device_snfc;
	FELICA_PR_DBG(" %s START", __func__);

	devid_snfc = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret = alloc_chrdev_region(&devid_snfc, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, SNFC_UART_NAME);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_snfc, &fops_snfc);
	ret = cdev_add(&cdev_snfc, devid_snfc, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_snfc, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_snfc = device_create(felica_class, NULL, devid_snfc, NULL,
			  SNFC_UART_NAME);
	if (IS_ERR(device_snfc)) {
		cdev_del(&cdev_snfc);
		unregister_chrdev_region(devid_snfc, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)", __func__);
		return;
	}

	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_snfc), MINOR(devid_snfc));
}

/*
 * finalize device
 */
static void snfc_uart_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	device_destroy(felica_class, devid_snfc);
	cdev_del(&cdev_snfc);
	unregister_chrdev_region(devid_snfc, FELICA_MINOR_COUNT);
}

/*
 * open device
 */
static int snfc_uart_open(struct inode *inode, struct file *file)
{
	uid_t uid;
/*	struct termios termios;*/
	struct file *tty;
	int ret = 0;

	FELICA_PR_DBG(" %s START", __func__);

	/* check NFC uid */
	uid = __task_cred(current)->uid;
	if (uid != gnfc_uid) {
		FELICA_PR_ERR(\
		" %s END -EACCES, uid=[%d], gnfc_uid=[%d]\n", __func__, uid,gnfc_uid);
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
		return -EACCES;
#endif
	}

	file->private_data = dev_sem;
	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_PR_ERR(\
		" %s END, ERR[ERESTARTSYS]\n", __func__);
		return -ERESTARTSYS;
	}

	if (pg_tty != NULL) {
		FELICA_PR_ERR(
			" %s multi open result=%d", __func__, ret);
		gnfc_open_cnt++;
		up(&dev_sem->felica_sem);
		return ret;
	}

	tty = filp_open(UART_DEV_NAME, O_RDWR | O_NOCTTY | O_NONBLOCK, 0);

	if (IS_ERR(tty)) {
		pg_tty = NULL;
		ret = PTR_ERR(tty);
		up(&dev_sem->felica_sem);
		FELICA_PR_ERR(
			" %s filp_open result = %d", __func__, ret);
		return ret;
	}

	pg_tty = tty;
	gnfc_open_cnt++;
	ret = 0;
	up(&dev_sem->felica_sem);
	FELICA_PR_DBG(" %s END", __func__);

	return ret;
}

/*
 * close device
 */
static int snfc_uart_close(struct inode *inode, struct file *file)
{
	struct file *tty;

	FELICA_PR_DBG(" %s START", __func__);

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_PR_ERR(\
		" %s END, ERR[ERESTARTSYS]\n", __func__);
		return -ERESTARTSYS;
	}

	gnfc_open_cnt--;
	tty = NULL;
	if (gnfc_open_cnt == 0) {
		tty = pg_tty;
		if (tty) {
			filp_close(tty, 0);
			pg_tty = NULL;
		}
	}

	up(&dev_sem->felica_sem);
	FELICA_PR_DBG(" %s END", __func__);

	return 0;
}

/*
 * read operation
 */
static ssize_t snfc_uart_read(struct file *file, char __user *buf,\
			size_t len, loff_t *ppos)
{
	struct file *tty;
	ssize_t ret = -ENOSYS;

	FELICA_PR_DBG(" %s START", __func__);

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_PR_ERR(\
		" %s END, ERR[ERESTARTSYS]\n", __func__);
		return -ERESTARTSYS;
	}

	tty = pg_tty;
	if (tty && tty->f_op->read)
		ret = tty->f_op->read(tty, buf, len, ppos);

	up(&dev_sem->felica_sem);

	FELICA_PR_DBG(" %s END", __func__);

	return ret;
}

/*
 * write operation
 */
static ssize_t snfc_uart_write(struct file *file, const char __user *data, \
			size_t len, loff_t *ppos)
{
	struct file *tty;
	ssize_t ret = -ENOSYS;

	FELICA_PR_DBG(" %s START", __func__);

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_PR_ERR(\
		" %s END, ERR[ERESTARTSYS]\n", __func__);
		return -ERESTARTSYS;
	}

	tty = pg_tty;
	if (tty && tty->f_op->write)
		ret = tty->f_op->write(tty, data, len, ppos);

	up(&dev_sem->felica_sem);

	FELICA_PR_DBG(" %s END", __func__);

	return ret;
}

/*
 * ioctl operation
 */
static long snfc_uart_ioctl(struct file *file, unsigned int cmd, \
						unsigned long arg)
{
	struct file *tty;
	ssize_t ret = -ENOSYS;

	FELICA_PR_DBG(" %s START", __func__);

	if (down_interruptible(&dev_sem->felica_sem)) {
		FELICA_PR_ERR(\
		" %s END, ERR[ERESTARTSYS]\n", __func__);
		return -ERESTARTSYS;
	}

	tty = pg_tty;
	if (tty && tty->f_op->unlocked_ioctl)
		ret = tty->f_op->unlocked_ioctl(tty, cmd, arg);

	up(&dev_sem->felica_sem);

	FELICA_PR_DBG(" %s END", __func__);

	return ret;
}

/*
 * poll operation
 */
static unsigned int snfc_uart_poll(struct file *file,
		struct poll_table_struct *poll_table)
{
	struct file *tty;
	unsigned int ret = 0;

	FELICA_PR_DBG(" %s START", __func__);

	if (down_interruptible(&dev_sem->felica_sem))
		return 0;


	tty = pg_tty;
	if (tty && tty->f_op->poll)
		ret = tty->f_op->poll(tty, poll_table);

	up(&dev_sem->felica_sem);

	FELICA_PR_DBG(" %s END", __func__);

	return ret;
}

/******************************************************************************
 * /dev/uartcc
 ******************************************************************************/

/* character device definition */
static struct felica_sem_data *guartcc_sem;
static dev_t devid_uartcc;
static struct cdev cdev_uartcc;
static const struct file_operations fops_uartcc = {
	.owner = THIS_MODULE,
	.open = uartcc_open,
	.release = uartcc_close,
	.unlocked_ioctl = uartcc_ioctl,
};

/*
 * uartcc_init
 */
static void uartcc_init(void)
{
	int ret;
	struct device *device_uartcc;

	FELICA_PR_DBG(" %s START\n", __func__);

	devid_uartcc = MKDEV(UARTCC_MAJOR, UARTCC_MINOR);

	ret =
	    alloc_chrdev_region(&devid_uartcc, UARTCC_BASEMINOR,
				UARTCC_MINOR_COUNT, UARTCC_NAME);

	if (ret < 0) {
		FELICA_PR_ERR(
			" %s ERROR(alloc_chrdev_region), ret=[%d]\n",
			__func__, ret);
		return;
	}

	cdev_init(&cdev_uartcc, &fops_uartcc);
	ret = cdev_add(&cdev_uartcc, devid_uartcc, UARTCC_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_uartcc, UARTCC_MINOR_COUNT);
		FELICA_PR_ERR(
			" %s ERROR(cdev_add), ret=[%d]\n", __func__,
			ret);
		return;
	}

	device_uartcc =
	    device_create(felica_class, NULL, devid_uartcc, NULL,
			  UARTCC_NAME);
	if (IS_ERR(device_uartcc)) {
		cdev_del(&cdev_uartcc);
		unregister_chrdev_region(devid_uartcc, UARTCC_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)\n", __func__);
		return;
	}

	guartcc_sem = kmalloc(sizeof(struct felica_sem_data), GFP_KERNEL);
	if (!guartcc_sem) {
		cdev_del(&cdev_uartcc);
		unregister_chrdev_region(devid_uartcc, UARTCC_MINOR_COUNT);
		FELICA_PR_ERR(
			" %s ERROR(guartcc_sem malloc)\n", __func__);
		return;
	}
	sema_init(&guartcc_sem->felica_sem, 1);

	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]\n", __func__, \
			 MAJOR(devid_uartcc), MINOR(devid_uartcc));
}

/*
 * uartcc_exit
 */
static void uartcc_exit(void)
{
	FELICA_PR_DBG(" %s START\n", __func__);

	kfree(guartcc_sem);
	device_destroy(felica_class, devid_uartcc);
	cdev_del(&cdev_uartcc);
	unregister_chrdev_region(devid_uartcc, UARTCC_MINOR_COUNT);

	FELICA_PR_DBG(" %s END\n", __func__);
}

/*
 * open device
 */
static int uartcc_open(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END\n", __func__);
	return 0;
}

/*
 * close device
 */
static int uartcc_close(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END\n", __func__);
	return 0;
}

/*
 * available operation
 */
static long uartcc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int user_buf = 0;

	FELICA_PR_DBG(" %s START, cmd=[%d]\n", __func__, cmd);

	switch (cmd) {
	case UARTCC_SET_NFC_START_REQ:
		if (down_interruptible(&guartcc_sem->felica_sem)) {
			FELICA_PR_ERR(
				" %s ERROR(down_interruptible)\n",
				__func__);
			return -ERESTARTSYS;
		}

		ret = copy_from_user(
			&user_buf, (void __user *)arg, sizeof(user_buf));
		if (ret != 0) {
			FELICA_PR_ERR
			    (" %s ERROR(copy_from_user), ret=[%d]\n",
			     __func__, ret);
			up(&guartcc_sem->felica_sem);
			return -EFAULT;
		}

		guartcc_start_req = user_buf;
		/* felica start request */
		uartcc_felica_start_request();
		FELICA_PR_INFO(" %s Set nfc start request=[%d]\n",
			__func__, guartcc_start_req);

#ifdef P2P_FPGA_ALWAYS_ON
		switch (guartcc_start_req) {
		case SNFC_START_TARGET:
		case SNFC_START_INTU:
			set_fpga_felica_flag(1);
			break;
		case UARTCC_NFC_START_ENDPROC:
			break;
		default:
			set_fpga_felica_flag(0);
			break;
		}
		FELICA_PR_INFO(" %s [fpga]felica_status=[%d]\n",
			__func__, felica_status);
#endif /* P2P_FPGA_ALWAYS_ON */
		up(&guartcc_sem->felica_sem);
		break;
	case UARTCC_GET_FELICA_STATUS:
		if (down_interruptible(&guartcc_sem->felica_sem)) {
			FELICA_PR_ERR(
				" %s ERROR(down_interruptible)\n", \
				__func__);
			return -ERESTARTSYS;
		}

		user_buf = uartcc_get_felica_status();
		ret = copy_to_user(
			(int __user *)arg, &user_buf, sizeof(user_buf));
		if (ret != 0) {
			FELICA_PR_ERR
			    (" %s ERROR(copy_to_user), ret=[%d]\n", \
			     __func__, ret);
			up(&guartcc_sem->felica_sem);
			return -EFAULT;
		}
		FELICA_PR_INFO(" %s Get felica status=[%d]\n", \
			__func__, guartcc_felica_status);
		up(&guartcc_sem->felica_sem);
		break;
	case UARTCC_SET_FELICA_STATUS:
	case UARTCC_GET_NFC_START_REQ:
		FELICA_PR_INFO(" %s No operation cmd\n", __func__);
		break;
	default:
		FELICA_PR_ERR(" %s ERROR(unknown command)\n", __func__);
		ret = -EIO;
		break;
	}

	FELICA_PR_DBG(" %s END\n", __func__);
	return ret;
}

/*
 * felica start request
 */
static void uartcc_felica_start_request(void)
{
	struct icc_poll_data *felica_poll = &gfelica_poll_data;

	FELICA_PR_DBG(" %s START\n", __func__);
	FELICA_PR_INFO(" %s start_req=[%d], felica_status=[%d]\n", \
		__func__, guartcc_start_req, guartcc_felica_status);

	/* wake up condition
	 * - nfc star req is ENDPROC
	 * - felica usage is waiting AutoPolling
	 */
	if ((guartcc_start_req == UARTCC_NFC_START_ENDPROC) &&
		(uartcc_get_felica_status() ==
		UARTCC_FELICA_STATAUS_WAIT_POLLING)) {
		felica_poll->rsp_done = 1;
		wake_up_interruptible(&felica_poll->rsp_wait);
		FELICA_PR_INFO(
			" %s wake up felica start event\n", __func__);
	}

	FELICA_PR_DBG(" %s END\n", __func__);
}

/*
 * set felica status
 */
static void uartcc_set_felica_status(int status)
{
	FELICA_PR_DBG(" %s START, status=[%d]\n", __func__, status);

	guartcc_felica_status = status;

	FELICA_PR_INFO(" %s guartcc_felica_status=[%d]\n",
			__func__, guartcc_felica_status);
	FELICA_PR_DBG(" %s END\n", __func__);
}

/*
 * get felica status
 */
static int uartcc_get_felica_status(void)
{
	FELICA_PR_DBG(" %s START, guartcc_felica_status=[%d]\n", \
			__func__, guartcc_felica_status);
	FELICA_PR_DBG(" %s END\n", __func__);
#if defined(CONFIG_MACH_KLTE_KDI)
	if(gfelica_uim_mon == 1) {
		FELICA_PR_ERR(" %s gfelica_uim_mon=%d\n", __func__, gfelica_uim_mon);
		return UARTCC_FELICA_STATAUS_IN_INIT;
	}
	return guartcc_felica_status;
#else
	return guartcc_felica_status;
#endif
}

/*
 * check for idle state felica
 */
static unsigned int uartcc_is_idle_status(void)
{
	int ret = 0;
	int flc_cts = 0;

	FELICA_PR_DBG(" %s START\n", __func__);

	flc_cts = uartcc_get_felica_status();

	if (!flc_cts) {	/* idle */
		FELICA_PR_INFO(" %s felica status is idle\n", __func__);
		ret = 1;
	} else {			/* else state */
		FELICA_PR_INFO(" %s felica status isn't idle=[%d]\n", \
				__func__, flc_cts);
	}

	FELICA_PR_DBG(" %s END\n", __func__);
	return ret;
}
/******************************************************************************
 * /dev/felica_uicc
 ******************************************************************************/

/* character device definition */
static dev_t devid_uicc;
static struct cdev cdev_uicc;
static const struct file_operations fops_uicc = {
	.owner = THIS_MODULE,
	.open = uicc_open,
	.release = uicc_close,
	.read = uicc_read,			// permissive mode issue
	.unlocked_ioctl = uicc_ioctl,
};

/*
 * uartcc_init
 */
static void uicc_init(void)
{
	int ret;
	struct device *device_uicc;

	FELICA_PR_DBG(" %s START\n", __func__);

	devid_uicc = MKDEV(UICC_MAJOR, UICC_MINOR);

	ret =
	    alloc_chrdev_region(&devid_uicc, UICC_BASEMINOR,
				UICC_MINOR_COUNT, UICC_NAME);

	if (ret < 0) {
		FELICA_PR_ERR(
			" %s ERROR(alloc_chrdev_region), ret=[%d]\n",
			__func__, ret);
		return;
	}

	cdev_init(&cdev_uicc, &fops_uicc);
	ret = cdev_add(&cdev_uicc, devid_uicc, UICC_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_uicc, UICC_MINOR_COUNT);
		FELICA_PR_ERR(
			" %s ERROR(cdev_add), ret=[%d]\n", __func__,
			ret);
		return;
	}

	device_uicc =
	    device_create(felica_class, NULL, devid_uicc, NULL,
			  UICC_NAME);
	if (IS_ERR(device_uicc)) {
		cdev_del(&cdev_uicc);
		unregister_chrdev_region(devid_uicc, UICC_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)\n", __func__);
		return;
	}


	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]\n", __func__, \
			 MAJOR(devid_uicc), MINOR(devid_uicc));
}

/*
 * uicc_exit
 */
static void uicc_exit(void)
{
	FELICA_PR_DBG(" %s START\n", __func__);

	device_destroy(felica_class, devid_uicc);
	cdev_del(&cdev_uicc);
	unregister_chrdev_region(devid_uicc, UICC_MINOR_COUNT);

	FELICA_PR_DBG(" %s END\n", __func__);
}

/*
 * open device
 */
static int uicc_open(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END\n", __func__);
	return 0;
}

/*
 * close device
 */
static int uicc_close(struct inode *inode, struct file *file)
{
	/* no operation */
	FELICA_PR_DBG(" %s END\n", __func__);
	return 0;
}

/*
 * read operation  // permissive mode issue
 */
static ssize_t uicc_read(struct file *file, char __user *buf, \
			size_t len, loff_t *ppos)
{
	int ret;
	char read_buff = 0;

	read_buff = g_uicc_sts;

	ret = copy_to_user(buf, &read_buff, FELICA_UICC_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	FELICA_PR_DBG(" %s END", __func__);
	return FELICA_UICC_DATA_LEN;
}

/*
 * available operation
 */
static long uicc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	struct regulator *reg_l9;

	FELICA_PR_DBG(" %s START, cmd=[%d]\n", __func__, cmd);
	FELICA_PR_INFO(" %s START, system_rev=[%d] UICC_rev[%d]\n",
		__func__, system_rev,g_uicc_initrev);

	switch (cmd) {
	case UICC_POWER_ON:
		if(system_rev < g_uicc_initrev)
		{
			reg_l9 = regulator_get(NULL, "8941_l9");
			if (IS_ERR(reg_l9)) {
				FELICA_PR_ERR(" %s 2 could not get 8917_l9, rc = %ld\n",
				__func__, PTR_ERR(reg_l9));
				return UICC_ERROR;
			}
			FELICA_PR_INFO(" %s regulator_get, reg_l9 OK]", __func__);

			ret = regulator_set_voltage(reg_l9, 1800000, 1800000);
			if (ret) { 
				FELICA_PR_ERR(
				" %s ERROR(regulator_set_voltage 1.8V~1.8V), ret=[%d]"
				, __func__, ret);
				return UICC_ERROR;
			}
			FELICA_PR_INFO(
				" %s regulator_set_voltage(1.8v~1.8v), ret=[%d]"
				, __func__, ret);

			if (regulator_is_enabled(reg_l9)) {
				FELICA_PR_INFO(
					" %s L9(0V) is enabled. No need to do anything\n"
					, __func__);
			} else {
				ret = regulator_enable(reg_l9);
				if (ret) {
					FELICA_PR_ERR(
						" %s regulator_enable l9 failed, rc=%d\n"
						, __func__, ret);
					return UICC_ERROR;
				}
			}
			FELICA_PR_INFO(
				" %s regulator_enable, ret=[%d]", __func__, ret);  
		}
		break;

	case UICC_POWER_OFF:
		if(system_rev < g_uicc_initrev)
		{
			reg_l9 = regulator_get(NULL, "8941_l9");
			if (IS_ERR(reg_l9)) {
				FELICA_PR_ERR(" %s could not get 8917_l9, rc = %ld\n",
				__func__, PTR_ERR(reg_l9));
				return UICC_ERROR;
			}

			FELICA_PR_INFO(" %s regulator_get, reg_l9 OK]", __func__);

			if (regulator_is_enabled(reg_l9)) {
					FELICA_PR_INFO(
						" %s L9(0V) is enabled already\n",__func__);
				if (regulator_disable(reg_l9)) {
						FELICA_PR_ERR
						(" %s disable l9 failed\n", __func__);
					return UICC_ERROR;
				}
			} else {
					FELICA_PR_ERR
					(" %s regulator l9 already disabled", __func__);
				return UICC_ERROR;
			}
		}
		break;
	case UICC_POWER_SPS_PMIC:
		g_uicc_sts = 0x01;	// permissive mode issue
		if(system_rev >= g_uicc_initrev)
		{
#if defined(CONFIG_MACH_KLTE_KDI)
			FELICA_PR_ERR(" %s Select Power Supply control is missing in K-KDDI! This makes kernel panic! gfelica_sps_pin=%d\n", __func__, gfelica_sps_pin);
#else
			gpio_set_value(gfelica_sps_pin, GPIO_VALUE_HIGH);
			FELICA_PR_INFO(
				" %s Select Power Supply -> HI [%d][%d]\n", __func__,
				gfelica_sps_pin, gpio_get_value(gfelica_sps_pin));
#endif			
		}

		break;

	case UICC_POWER_UIM_MON:
#if defined(CONFIG_MACH_KLTE_KDI)
		FELICA_PR_INFO(" %s UICC_POWER_UIM_MON! make PON always high!\n", __func__);
		gfelica_uim_mon = 1;
		gpio_set_value(gfelica_pon_pin, GPIO_VALUE_HIGH);
#else
		FELICA_PR_ERR(" %s UICC_POWER_UIM_MON! this is only for K-KDDI\n", __func__);
#endif			
		break;

	default:
		FELICA_PR_ERR(" %s Undefine Paramater [%d]", __func__, cmd);
		return UICC_ERROR;
		break;
	}

	FELICA_PR_DBG(" %s END\n", __func__);
	return UICC_SUCCESS;
}

/******************************************************************************
 * common
 ******************************************************************************/
/*
 * add CXD2235 additional pin registration
 */
static void snfc_register_device(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	intu_poll_init();
	schedule_delayed_work(&g_intu_d->work, msecs_to_jiffies(10));
	rfs_poll_init();
	schedule_delayed_work(&g_rfs_d->work, msecs_to_jiffies(10));

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * add CXD2235 additional pin unregistration
 */
static void snfc_deregister_device(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	available_poll_exit();
	intu_poll_exit();
	snfc_rfs_exit();
	rfs_poll_exit();
	hsel_exit();
	cxd2235power_exit();

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * add read and store CEN status for snfc_available_poll device
 */
static ssize_t snfc_cen_sts_init(void)
{
	int ret;
	unsigned char address = gi2c_lockaddress;
	unsigned char read_buff = 0;
	struct i2c_msg read_msgs[2];

	read_msgs[0].flags = gread_msgs[0].flags;
	read_msgs[0].len = gread_msgs[0].len;
	read_msgs[1].flags = gread_msgs[1].flags;
	read_msgs[1].len = gread_msgs[1].len;

	read_msgs[0].addr = gi2c_address;
	read_msgs[0].buf = &address;
	read_msgs[1].addr = gi2c_address;
	read_msgs[1].buf = &read_buff;

	FELICA_PR_DBG(" %s START", __func__);
	if (felica_i2c_client == NULL) {
		FELICA_PR_ERR(" felica_i2c_client is NULL");
		return -EIO;
	}

	#if defined(CONFIG_MACH_HLTEDCM)
	//high
	if (system_rev == HW_REV09_OR_10)
	{
		FELICA_PR_INFO(" %s Making MHL_EN HIGH", __func__);
		of_sii8240_hw_poweron(FELICA_CEN_UNLOCK);
	}	
	#endif
	
	ret = i2c_transfer(felica_i2c_client->adapter, &read_msgs[0], 1);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(i2c_transfer[0]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}
	ret = i2c_transfer(felica_i2c_client->adapter, &read_msgs[1], 1);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(i2c_transfer[1]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	FELICA_PR_INFO(" %s read_buff=[%d]", __func__, read_buff);
	read_buff &= FELICA_CONTROL_LOCK_MASK;
	FELICA_PR_INFO(" %s read_buff=[%d]", __func__, read_buff);
	g_cen_sts = read_buff;

	#if defined(CONFIG_MACH_HLTEDCM)
	//low
	if (system_rev == HW_REV09_OR_10)
	{
		FELICA_PR_INFO(" %s Making MHL_EN LOW", __func__);
	//	of_sii8240_hw_poweron(FELICA_CEN_LOCK);
	}
	#endif
	
	FELICA_PR_DBG(" %s END", __func__);
	return FELICA_CEN_DATA_LEN;
}


/******************************************************************************
 * /dev/snfc_cen
 ******************************************************************************/
/* character device definition */
static dev_t devid_snfca_cen;
static struct cdev cdev_snfc_cen;
static const struct file_operations fops_snfc_cen = {
	.owner = THIS_MODULE,
	.open = snfc_cen_open,
	.release = snfc_cen_close,
	.read = snfc_cen_read,
};

/*
 * initialize device
 */
static void snfc_cen_init(void)
{
	int ret;
	struct device *device_snfc_cen;
	FELICA_PR_DBG(" %s START", __func__);

	devid_snfca_cen = MKDEV(FELICA_MAJOR, FELICA_MINOR);
	ret =
	    alloc_chrdev_region(&devid_snfca_cen, FELICA_BASEMINOR,
				FELICA_MINOR_COUNT, SNFC_CEN_NAME);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return;
	}

	cdev_init(&cdev_snfc_cen, &fops_snfc_cen);
	ret = cdev_add(&cdev_snfc_cen, devid_snfca_cen, FELICA_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devid_snfca_cen, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(cdev_add), ret=[%d]", __func__,
			       ret);
		return;
	}

	device_snfc_cen =
	    device_create(felica_class, NULL, devid_snfca_cen, NULL,
			  SNFC_CEN_NAME);
	if (IS_ERR(device_snfc_cen)) {
		cdev_del(&cdev_snfc_cen);
		unregister_chrdev_region(devid_snfca_cen, FELICA_MINOR_COUNT);
		FELICA_PR_ERR(" %s ERROR(device_create)", __func__);
		return;
	}

	FELICA_PR_DBG(" %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devid_snfca_cen), MINOR(devid_snfca_cen));
}

/*
 * finalize device
 */
static void snfc_cen_exit(void)
{
	FELICA_PR_DBG(" %s START", __func__);

	device_destroy(felica_class, devid_snfca_cen);
	cdev_del(&cdev_snfc_cen);
	unregister_chrdev_region(devid_snfca_cen, FELICA_MINOR_COUNT);

	FELICA_PR_DBG(" %s END", __func__);
}

/*
 * open device
 */
static int snfc_cen_open(struct inode *inode, struct file *file)
{
	uid_t uid;
#ifdef FELICA_UICC_FUNCTION
	int ret = 0;
	char *cmdpos;
	static char cmdline[1025];
	static unsigned long start_adr, end_adr, leng;
#endif

	FELICA_PR_DBG(" %s START . system_rev=[%d]", __func__,system_rev);

	#if defined(CONFIG_MACH_HLTEDCM)
	//high
	if (system_rev == HW_REV09_OR_10)
	{
		FELICA_PR_INFO(" %s Making MHL_EN HIGH", __func__);
		of_sii8240_hw_poweron(SNFC_CEN_UNLOCK);
	}
	#endif
	
#ifdef FELICA_UICC_FUNCTION
	start_adr = current->mm->arg_start;
	end_adr = current->mm->arg_end;
	leng = end_adr - start_adr;

	if (1024 < leng)
		leng = 1024;
	cmdpos = (char *)(current->mm->arg_start);
	ret = copy_from_user(cmdline, cmdpos, leng);
	cmdline[leng] = '\0';
	if(ret != 0)
	{
		FELICA_PR_ERR(" %s cmdline[%s] ret[%d]", __func__, cmdline, ret);
		return -EFAULT;
	}
#endif

	uid = __task_cred(current)->uid;
	if (file->f_mode & FMODE_WRITE) {
#ifdef FELICA_UICC_FUNCTION
		if ((uid != gdiag_uid) && (uid != gmfl_uid)
		&& (strncmp(cmdline,gproc_name, leng) != 0)) {
#else
		if ((uid != gdiag_uid) && (uid != gmfl_uid)) {
#endif
	FELICA_PR_ERR(" %s END -EACCES, uid=[%d], gmfc_uid=[%d], gdiag_uid=[%d], gmfl_uid=[%d]\n", __func__, uid,gmfc_uid,gdiag_uid,gmfl_uid);

			#if defined(CONFIG_MACH_HLTEDCM)
			//low
			if (system_rev == HW_REV09_OR_10)
			{
				FELICA_PR_INFO(" %s Making MHL_EN LOW", __func__);
		//		of_sii8240_hw_poweron(SNFC_CEN_LOCK);
			}
			#endif

#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
			return -EACCES;
#endif
		}
	}
	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

/*
 * close device
 */
static int snfc_cen_close(struct inode *inode, struct file *file)
{
	/* no operation */
	#if defined(CONFIG_MACH_HLTEDCM)
	//low
	if (system_rev == HW_REV09_OR_10)
	{
		FELICA_PR_INFO(" %s Making MHL_EN LOW", __func__);
	//	of_sii8240_hw_poweron(SNFC_CEN_LOCK);
	}
	#endif
	
	FELICA_PR_DBG(" %s END system_rev=[%d]", __func__,system_rev);
	return 0;
}

/*
 * read operation
 */
static ssize_t snfc_cen_read(struct file *file, char __user *buf, \
			size_t len, loff_t *ppos)
{
	int ret;
	unsigned char address = gi2c_lockaddress;
	unsigned char read_buff = 0;
	struct i2c_msg read_msgs[2];

	read_msgs[0].flags = gread_msgs[0].flags;
	read_msgs[0].len = gread_msgs[0].len;
	read_msgs[1].flags = gread_msgs[1].flags;
	read_msgs[1].len = gread_msgs[1].len;

	read_msgs[0].addr = gi2c_address;
	read_msgs[0].buf = &address;
	read_msgs[1].addr = gi2c_address;
	read_msgs[1].buf = &read_buff;

	if (felica_i2c_client == NULL) {
		FELICA_PR_ERR(" felica_i2c_client is NULL %s -EIO",__func__);
		return -EIO;
	}

	ret = i2c_transfer(felica_i2c_client->adapter, &read_msgs[0], 1);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(i2c_transfer[0]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}
	ret = i2c_transfer(felica_i2c_client->adapter, &read_msgs[1], 1);
	if (ret < 0) {
		FELICA_PR_ERR(" %s ERROR(i2c_transfer[1]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}

	read_buff &= SNFC_CONTROL_LOCK_MASK;

#ifdef CONFIG_NFC_FELICA
	g_cen_sts = read_buff;
#endif

	FELICA_PR_DBG(" %s g_cen_sts=[%d]", __func__, g_cen_sts);

	ret = copy_to_user(buf, &read_buff, SNFC_CEN_DATA_LEN);
	if (ret != 0) {
		FELICA_PR_ERR(" %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;

	return SNFC_CEN_DATA_LEN;
}

/*
 * uid check
 */
static int snfc_uid_check(void)
{
	uid_t uid;
	uid = __task_cred(current)->uid;

	FELICA_PR_DBG(" %s START", __func__);
	FELICA_PR_INFO(" %s gnfc_uid=[%d]", __func__, (int)gnfc_uid);

	if ((uid != gnfc_uid) && (uid != gdiag_uid)) {
		FELICA_PR_ERR
		    (" %s END, uid=[%d], gnfc_uid=[%d], gdiag_uid=[%d]", \
		     __func__, uid, gnfc_uid, gdiag_uid);
#ifdef SRIB_DIAG_ENABLED
		FELICA_PR_ERR(" SRIB-Diag enabled just for test\n");
#else
		return -1;
#endif
	}

	FELICA_PR_DBG(" %s END", __func__);
	return 0;
}

#endif              /* CONFIG_NFC_FELICA */
module_init(felica_init);
module_exit(felica_exit);
MODULE_DESCRIPTION("felica_dd");
MODULE_LICENSE("GPL v2");
#endif				/* CONFIG_FELICA */
