/*
 * SAMSUNG NFC Controller
 *
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 * Author: Woonki Lee <woonki84.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#define DEBUG

#ifdef	CONFIG_SEC_NFC_I2C_GPIO
#define CONFIG_SEC_NFC_I2C
#endif

#include <linux/wait.h>
#include <linux/delay.h>

#ifdef CONFIG_SEC_NFC_I2C
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/i2c.h>
#endif

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/nfc/sec_nfc.h>
#include <linux/of_gpio.h>

// Security
#include <mach/scm.h>
// End of Security
#include <linux/regulator/consumer.h>
#include <mach/gpiomux.h>

#include <linux/wakelock.h>


#ifndef BU80003GUL

#define BU80003GUL
#define BU80003GUL_DEBUG

#ifdef BU80003GUL

#include "bu80003gul.h"
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/i2c.h>

#ifdef BU80003GUL_DEBUG
#define EPC_DEBUG(fmt, args...)		printk(KERN_DEBUG fmt, ## args)
#define EPC_INFO(fmt, args...)		printk(KERN_INFO fmt, ## args)
#define EPC_ERR(fmt, args...)		printk(KERN_ERR fmt, ## args)
#else
#define EPC_DEBUG(fmt, args...)
#define EPC_INFO(fmt, args...)
#define EPC_ERR(fmt, args...)
#endif


#define I2C_ADDR (0x56 >> 1)
#define I2C_LOCK_ADDR 0x02
#define I2C_ANT_ADDR 0x01

extern int poweroff_charging;

static struct i2c_msg gread_msgs[] = {
	{
		.addr	= 0,
		.flags	= 0,
		.len	= 1,
		.buf	= NULL,
	},
	{
		.addr	= 0,
		.flags	= I2C_M_RD,
		.len	= 1,
		.buf	= NULL,
	},
};

static struct i2c_msg gwrite_msgs[] = {
	{
		.addr	= 0,
		.flags	= 0,
		.len	= 2,
		.buf	= NULL,
	},
};


/*
 *	I2C device_id table
*/
static struct i2c_client *bu80003gul_i2c_client;
static const struct i2c_device_id bu80003gul_i2c_idtable[] =
{
	{BU80003GUL_I2C_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, bu80003gul_i2c_idtable);

/*
 * I2C match table
 */
static struct of_device_id bu80003gul_i2c_match_table[] =
{
	{ .compatible = "felica,felica-i2c",},
	{},
};

/*
 * I2C driver
 */
static struct i2c_driver bu80003gul_i2c_driver =
{
	.probe = bu80003gul_i2c_probe,
	.remove = bu80003gul_i2c_remove,
	.id_table = bu80003gul_i2c_idtable,
	.driver =
			{
		   .name = BU80003GUL_I2C_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = bu80003gul_i2c_match_table,
			},
};


/*
 * Name : bu80003gul_i2c_probe
 * Description : Probe the I2C device.
 */
static int bu80003gul_i2c_probe(struct i2c_client *client,
			    const struct i2c_device_id *devid)
{
	int ret;

	EPC_DEBUG("[MFDD] %s START", __func__);

	bu80003gul_i2c_client = client;
	if (!bu80003gul_i2c_client) {
		EPC_ERR("[MFDD] %s ERROR(bu80003gul_i2c_client==NULL)", \
							__func__);
		return -EINVAL;
	}

	ret = felica_epc_register();
	if (ret < 0) {
		EPC_ERR("[MFDD] %s Failed to register the device[ret:%d]\n",
				__func__, ret);
	}
	EPC_DEBUG("[MFDD] %s END", __func__);
	return 0;
}

/*
 * Name : bu80003gul_i2c_remove
 * Description : Remove the I2C device.
 */
static int bu80003gul_i2c_remove(struct i2c_client *client)
{
	bu80003gul_i2c_client = NULL;
	EPC_DEBUG("[MFDD] %s END", __func__);
	return 0;
}



/* felica_ant device related */

static dev_t devId_felica_epc;
static struct cdev cdev_felica_epc;
static const struct file_operations fops_felica_epc = {
	.owner = THIS_MODULE,
	.open = felica_epc_open,
	.read = felica_epc_read,
	.write = felica_epc_write,
	/* .unlocked_ioctl = felica_epc_ioctl, */
	.release = felica_epc_close,
};

int felica_epc_setLockState(int state)
{
	char cen;
        int ret;
        unsigned char write_buff[2];

	cen =1;

	gwrite_msgs[0].buf = &write_buff[0];
	gwrite_msgs[0].addr = I2C_ADDR;
	write_buff[0] = 0x02;
	write_buff[1] = state;
	ret = i2c_transfer(bu80003gul_i2c_client->adapter, gwrite_msgs, 1);
        if (ret < 0) {
                EPC_ERR(" %s ERROR(i2c_transfer), ret=[%d]",
                               __func__, ret);
                return -EIO;
        }

return ret;
}

static int felica_epc_register(void)
{
	struct device *device_felica_epc;
	int ret;

	devId_felica_epc = MKDEV(FELICA_EPC_MAJOR, FELICA_EPC_MINOR);
	ret = alloc_chrdev_region(&devId_felica_epc, FELICA_EPC_BASEMINOR,
				FELICA_EPC_MINOR_COUNT, FELICA_EPC_NAME);
	if (ret < 0) {
		EPC_ERR("[MFDD] %s ERROR(alloc_chrdev_region), ret=[%d]",
			       __func__, ret);
		return ret;
	}

	cdev_init(&cdev_felica_epc, &fops_felica_epc);
	ret = cdev_add(&cdev_felica_epc, devId_felica_epc, FELICA_EPC_MINOR_COUNT);
	if (ret < 0) {
		unregister_chrdev_region(devId_felica_epc, FELICA_EPC_MINOR_COUNT);
		EPC_ERR("[MFDD] %s ERROR(cdev_add), ret=[%d]", __func__,ret);
		return -EIO;
	}

	eeprom_class = class_create(THIS_MODULE, "felica_eeprom");
	if (IS_ERR(eeprom_class)) {
		EPC_ERR("[MFDD] %s ERROR(class_create)", __func__);
		return PTR_ERR(eeprom_class);
	}

	device_felica_epc = device_create(eeprom_class, NULL,
								devId_felica_epc, NULL,
								FELICA_EPC_NAME);
	if (IS_ERR(device_felica_epc)) {
		cdev_del(&cdev_felica_epc);
		unregister_chrdev_region(devId_felica_epc, FELICA_EPC_MINOR_COUNT);
		EPC_ERR("[MFDD] %s ERROR(device_create)", __func__);
		return -EINVAL;
	}

	EPC_DEBUG("[MFDD] %s END, major=[%d], minor=[%d]", __func__,
			 MAJOR(devId_felica_epc), MINOR(devId_felica_epc));

	return 0;
}

static void felica_epc_deregister(void)
{
	EPC_DEBUG("[MFDD] %s START", __func__);
	device_destroy(eeprom_class, devId_felica_epc);
	cdev_del(&cdev_felica_epc);
	unregister_chrdev_region(devId_felica_epc, FELICA_EPC_MINOR_COUNT);
	EPC_DEBUG("[MFDD] %s END", __func__);
	return;
}


static int felica_epc_open(struct inode *inode, struct file *file)
{
	if (felica_epc_setLockState(0x81) < 0)
	{
		EPC_ERR("[MFDD} Failed to UnLock the EEPROM. \n");
	}
	return 0;
}

static int felica_epc_close(struct inode *inode, struct file *file)
{
	if (felica_epc_setLockState(0x80) < 0)
	{
		EPC_ERR("[MFDD} Failed to UnLock the EEPROM. \n");
	}
	return 0;
}


static ssize_t felica_epc_read(struct file *file, char __user *buf, \
			size_t len, loff_t *ppos)
{
	int ret;
	unsigned char address = I2C_ANT_ADDR;
	unsigned char read_buff = 0;
	struct i2c_msg read_msgs[2];

	read_msgs[0].flags = gread_msgs[0].flags;
        read_msgs[0].len = gread_msgs[0].len;
        read_msgs[1].flags = gread_msgs[1].flags;
        read_msgs[1].len = gread_msgs[1].len;

        read_msgs[0].addr = I2C_ADDR;
        read_msgs[0].buf = &address;
        read_msgs[1].addr = I2C_ADDR;
        read_msgs[1].buf = &read_buff;

	if (bu80003gul_i2c_client == NULL) {
		EPC_ERR("[MFDD] bu80003gul_i2c_client is NULL %s -EIO",__func__);
		return -EIO;
	}

	ret = i2c_transfer(bu80003gul_i2c_client->adapter, &read_msgs[0], 1);
	if (ret < 0) {
		EPC_ERR("[MFDD] %s ERROR(i2c_transfer[0]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}
	ret = i2c_transfer(bu80003gul_i2c_client->adapter, &read_msgs[1], 1);
	if (ret < 0) {
		EPC_ERR("[MFDD] %s ERROR(i2c_transfer[1]), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}
	ret = copy_to_user(buf, &read_buff, len);
	if (ret != 0) {
		EPC_ERR("[MFDD] %s ERROR(copy_to_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	*ppos += 1;
	EPC_DEBUG("[MFDD] %s END\n", __func__);
	return 1; /* Only one byte at a time will be read. Hence return 1 on success. */

}


static ssize_t felica_epc_write(struct file *file, const char __user *data,
				size_t len, loff_t *ppos)
{
	int ret;
	char ant;
	unsigned char write_buff[2];

	if (bu80003gul_i2c_client == NULL) {
		EPC_ERR("[MFDD] bu80003gul_i2c_client is NULL %s",__func__);
		return -EIO;
	}

	gwrite_msgs[0].buf = &write_buff[0];
	gwrite_msgs[0].addr = I2C_ADDR;
	write_buff[0] = I2C_ANT_ADDR;

	ret = copy_from_user(&ant, data, len);
	if (ret != 0) {
		EPC_ERR("[MFDD] %s ERROR(copy_from_user), ret=[%d]",
			       __func__, ret);
		return -EFAULT;
	}
	write_buff[1] = ant;
	ret = i2c_transfer(bu80003gul_i2c_client->adapter, gwrite_msgs, 1);
	if (ret < 0) {
		EPC_ERR("[MFDD] %s ERROR(i2c_transfer), ret=[%d]",
			       __func__, ret);
		return -EIO;
	}
	EPC_DEBUG("[MFDD] %s END\n", __func__);
	return 1;
}


#endif /* BU80003GUL */


#ifdef	CONFIG_SEC_NFC_I2C
enum sec_nfc_irq {
	SEC_NFC_NONE,
	SEC_NFC_INT,
	SEC_NFC_TRY_AGAIN,
};
#endif

struct sec_nfc_info {
	struct miscdevice miscdev;
	struct mutex mutex;
	enum sec_nfc_state state;
	struct device *dev;
	struct sec_nfc_platform_data *pdata;
	struct wake_lock wake_lock;

#ifdef	CONFIG_SEC_NFC_I2C
	struct i2c_client *i2c_dev;
	struct mutex read_mutex;
	enum sec_nfc_irq read_irq;
	wait_queue_head_t read_wait;
	size_t buflen;
	u8 *buf;
#endif
};

#ifdef	CONFIG_SEC_NFC_I2C
static irqreturn_t sec_nfc_irq_thread_fn(int irq, void *dev_id)
{
	struct sec_nfc_info *info = dev_id;

	dev_dbg(info->dev, "IRQ\n");

	mutex_lock(&info->read_mutex);
	info->read_irq = SEC_NFC_INT;
	mutex_unlock(&info->read_mutex);

	wake_up_interruptible(&info->read_wait);

	return IRQ_HANDLED;
}

#endif

static int sec_nfc_set_state(struct sec_nfc_info *info,
					enum sec_nfc_state state)
{
	struct sec_nfc_platform_data *pdata = info->pdata;

	/* intfo lock is aleady getten before calling this function */
	info->state = state;

	gpio_set_value(pdata->ven, 0);
	gpio_set_value(pdata->firm, 0);

	if (state == SEC_NFC_ST_FIRM) {
		gpio_set_value(pdata->firm, 1);
		gpio_set_value(pdata->ven, 0);
	}

	msleep(SEC_NFC_VEN_WAIT_TIME);
	if (state != SEC_NFC_ST_OFF)
		gpio_set_value(pdata->ven, 1);

	msleep(SEC_NFC_VEN_WAIT_TIME);
	dev_dbg(info->dev, "Power state is : %d\n", state);

	return 0;
}

#ifdef CONFIG_SEC_NFC_I2C
static int sec_nfc_reset(struct sec_nfc_info *info)
{
	dev_err(info->dev, "i2c failed. return resatrt to M/W\n");

	sec_nfc_set_state(info, SEC_NFC_ST_NORM);

	return -ERESTART;
}

static ssize_t sec_nfc_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);
	enum sec_nfc_irq irq;
	int ret = 0;

	dev_dbg(info->dev, "%s: info: %p, count: %zu\n", __func__,
		info, count);

	mutex_lock(&info->mutex);

	if (info->state == SEC_NFC_ST_OFF) {
		dev_err(info->dev, "sec_nfc is not enabled\n");
		ret = -ENODEV;
		goto out;
	}

	mutex_lock(&info->read_mutex);
	irq = info->read_irq;
	mutex_unlock(&info->read_mutex);
	if (irq == SEC_NFC_NONE) {
		if (file->f_flags & O_NONBLOCK) {
			dev_err(info->dev, "it is nonblock\n");
			ret = -EAGAIN;
			goto out;
		}
	}
	dev_err(info->dev, "LWK irq %d\n", irq);

	/* i2c recv */
	if (count > info->buflen)
		count = info->buflen;

	if (count < SEC_NFC_MSG_MIN_SIZE || count > SEC_NFC_MSG_MAX_SIZE) {
		dev_err(info->dev, "user required wrong size\n");
		ret = -EINVAL;
		goto out;
	}

	mutex_lock(&info->read_mutex);
	ret = i2c_master_recv(info->i2c_dev, info->buf, count);
	dev_err(info->dev, "recv size : %d\n", ret);

	if (ret == -EREMOTEIO) {
		ret = sec_nfc_reset(info);
		goto read_error;
	} else if (ret != count) {
		dev_err(info->dev, "read failed: return: %d count: %d\n",
			ret, count);
		//ret = -EREMOTEIO;
		goto read_error;
	}

	info->read_irq = SEC_NFC_NONE;
	mutex_unlock(&info->read_mutex);

	if (copy_to_user(buf, info->buf, ret)) {
		dev_err(info->dev, "copy failed to user\n");
		ret = -EFAULT;
	}

	goto out;

read_error:
	info->read_irq = SEC_NFC_NONE;
	mutex_unlock(&info->read_mutex);
out:
	mutex_unlock(&info->mutex);

	return ret;
}

static ssize_t sec_nfc_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);
	int ret = 0;

	dev_err(info->dev, "%s: info: %p, count %zu\n", __func__,
		info, count);

	mutex_lock(&info->mutex);

	if (info->state == SEC_NFC_ST_OFF) {
		dev_err(info->dev, "sec_nfc is not enabled\n");
		ret = -ENODEV;
		goto out;
	}

	if (count > info->buflen)
		count = info->buflen;

	if (count < SEC_NFC_MSG_MIN_SIZE || count > SEC_NFC_MSG_MAX_SIZE) {
		dev_err(info->dev, "user required wrong size\n");
		ret = -EINVAL;
		goto out;
	}

	if (copy_from_user(info->buf, buf, count)) {
		dev_err(info->dev, "copy failed from user\n");
		ret = -EFAULT;
		goto out;
	}

//	usleep_range(6000, 10000);
    usleep_range(600, 1000);
	ret = i2c_master_send(info->i2c_dev, info->buf, count);

	if (ret == -EREMOTEIO) {
		ret = sec_nfc_reset(info);
		goto out;
	}

	if (ret != count) {
		dev_err(info->dev, "send failed: return: %d count: %d\n",
		ret, count);
		ret = -EREMOTEIO;
	}

out:
	mutex_unlock(&info->mutex);

	return ret;
}
#endif

#ifdef CONFIG_SEC_NFC_I2C
static unsigned int sec_nfc_poll(struct file *file, poll_table *wait)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);
	enum sec_nfc_irq irq;

	int ret = 0;

	dev_dbg(info->dev, "%s: info: %p\n", __func__, info);

	mutex_lock(&info->mutex);

	if (info->state == SEC_NFC_ST_OFF) {
		dev_err(info->dev, "sec_nfc is not enabled\n");
		ret = -ENODEV;
		goto out;
	}

	poll_wait(file, &info->read_wait, wait);

	mutex_lock(&info->read_mutex);
	irq = info->read_irq;
	if (irq == SEC_NFC_INT)
		ret = (POLLIN | POLLRDNORM);
	mutex_unlock(&info->read_mutex);

out:
	mutex_unlock(&info->mutex);

	return ret;
}
#endif

static long sec_nfc_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);
	unsigned int mode = (unsigned int)arg;
	int ret = 0;
        int firm;

	dev_dbg(info->dev, "%s: info: %p, cmd: 0x%x\n",
			__func__, info, cmd);

	mutex_lock(&info->mutex);

	switch (cmd) {
	case SEC_NFC_SET_MODE:
		dev_dbg(info->dev, "%s: SEC_NFC_SET_MODE\n", __func__);

		if (info->state == mode)
			break;

		if (mode >= SEC_NFC_ST_COUNT) {
			dev_err(info->dev, "wrong state (%d)\n", mode);
			ret = -EFAULT;
			break;
		}

		ret = sec_nfc_set_state(info, mode);
		if (ret < 0) {
			dev_err(info->dev, "enable failed\n");
			break;
		}

		break;

	case SEC_NFC_SET_UART_STATE:
		if (mode >= SEC_NFC_ST_COUNT) {
			dev_err(info->dev, "wrong state (%d)\n", mode);
			ret = -EFAULT;
		break;
	}

                firm = gpio_get_value(info->pdata->firm);
                pr_info("%s: [NFC] Firm pin = %d\n", __func__, firm);

		if(mode == SEC_NFC_ST_UART_ON) {
			gpio_set_value(info->pdata->firm, STATE_FIRM_HIGH);

			if(!wake_lock_active(&info->wake_lock)) {
				pr_info("%s: [NFC] wake lock.\n", __func__);
				wake_lock(&info->wake_lock);
			}
		} else if(mode == SEC_NFC_ST_UART_OFF) {
			gpio_set_value(info->pdata->firm, STATE_FIRM_LOW);

			if(wake_lock_active(&info->wake_lock)) {
				pr_info("%s: [NFC] wake unlock after 2 sec.\n", __func__);
				wake_lock_timeout(&info->wake_lock, 2 * HZ);
			}
		}
		else
			ret = -EFAULT;

                firm = gpio_get_value(info->pdata->firm);
                pr_info("%s: [NFC] Mode = %d, Firm pin = %d\n", __func__, mode, firm);

		break;

	default:
		dev_err(info->dev, "Unknow ioctl 0x%x\n", cmd);
		ret = -ENOIOCTLCMD;
		break;
	}

	mutex_unlock(&info->mutex);

	return ret;
}

// Security
uint8_t check_custom_kernel(void)
{

	uint32_t fuse_id = SEC_NFC_FUSE_ID;
	void *cmd_buf;
	size_t cmd_len;
	size_t resp_len = 0;
	uint8_t resp_buf;

	pr_info(" %s START \n", __func__);

	resp_len = sizeof(resp_buf);
	cmd_buf = (void *)&fuse_id;
	cmd_len = sizeof(fuse_id);

	scm_call(SEC_NFC_SVC_FUSE, SEC_NFC_IS_SW_FUSE_BLOWN_ID, cmd_buf,
						cmd_len, &resp_buf, resp_len);
	pr_info(" %s END resp_buf = %d\n",__func__, resp_buf);
	
	return resp_buf;
}
// End of Security


static int sec_nfc_parse_dt(struct device *dev,
	struct sec_nfc_platform_data *pdata)
{

	struct device_node *np = dev->of_node;
	pdata->ven = of_get_named_gpio_flags(np, "sec-nfc,pon-gpio",
		0, &pdata->pon_gpio_flags);
	pdata->firm = of_get_named_gpio_flags(np, "sec-nfc,rfs-gpio",
		0, &pdata->rfs_gpio_flags);
	pdata->tvdd = of_get_named_gpio_flags(np, "sec-nfc,tvdd-gpio",
		0, &pdata->tvdd_gpio_flags);
	return 0;
}

static int sec_nfc_open(struct inode *inode, struct file *file)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);
	int ret = 0;
	uid_t uid;
// Security
	dev_dbg(info->dev, "%s: info : %p" , __func__, info);

    // Check process
	uid = __task_cred(current)->uid;

	if (g_secnfc_uid != uid) {
		dev_err(info->dev, "%s: Un-authorized process. No access to device\n", __func__);
		return -EPERM;
	}
// End of Security

	mutex_lock(&info->mutex);
	if (info->state != SEC_NFC_ST_OFF) {
		dev_err(info->dev, "sec_nfc is busy\n");
		ret = -EBUSY;
		goto out;
	}

#ifdef CONFIG_SEC_NFC_I2C
	mutex_lock(&info->read_mutex);
	info->read_irq = SEC_NFC_NONE;
	mutex_unlock(&info->read_mutex);
	ret = sec_nfc_set_state(info, SEC_NFC_ST_NORM);
#endif
out:
	mutex_unlock(&info->mutex);
	return ret;
}

static int sec_nfc_close(struct inode *inode, struct file *file)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);

	dev_dbg(info->dev, "%s: info : %p" , __func__, info);

	mutex_lock(&info->mutex);
	sec_nfc_set_state(info, SEC_NFC_ST_OFF);
	mutex_unlock(&info->mutex);

	return 0;
}

static void sec_nfc_uart_suspend(void)
{
	int ret = 0;
	pr_info("%s: enter  start\n", __func__);
	ret = gpio_tlmm_config (GPIO_CFG(23, GPIOMUX_FUNC_GPIO,
					GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
	if (ret) {

		pr_err( "failed to configure GPIO_23. ret %d\n", ret);
	}

	ret = gpio_tlmm_config (GPIO_CFG(24, GPIOMUX_FUNC_GPIO,
					GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
	if (ret) {

		pr_err( "failed to configure GPIO_24. ret %d\n", ret);
	}

	ret = gpio_tlmm_config (GPIO_CFG(25, GPIOMUX_FUNC_GPIO,
					GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
	if (ret) {

		pr_err( "failed to configure GPIO_25. ret %d\n", ret);
	}

	ret = gpio_tlmm_config (GPIO_CFG(26, GPIOMUX_FUNC_GPIO,
					GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
	if (ret) {

		pr_err( "failed to configure GPIO_26. ret %d\n", ret);
	}
	pr_info("%s: exit\n", __func__);
}

static const struct file_operations sec_nfc_fops = {
	.owner		= THIS_MODULE,
#ifdef  CONFIG_SEC_NFC_I2C
	.read		= sec_nfc_read,
	.write		= sec_nfc_write,
	.poll		= sec_nfc_poll,
#endif
	.open		= sec_nfc_open,
	.release	= sec_nfc_close,
	.unlocked_ioctl	= sec_nfc_ioctl,
};

#ifdef CONFIG_PM
static int sec_nfc_suspend(struct device *dev)
{
#ifdef	CONFIG_SEC_NFC_I2C
	struct i2c_client *client = to_i2c_client(dev);
	struct sec_nfc_info *info = i2c_get_clientdata(client);
#else
	struct platform_device *pdev = to_platform_device(dev);
	struct sec_nfc_info *info = platform_get_drvdata(pdev);
#endif
//	struct sec_nfc_platform_data *pdata = dev->platform_data;

	int ret = 0;

	mutex_lock(&info->mutex);

	if (info->state == SEC_NFC_ST_FIRM)
		ret = -EPERM;

	mutex_unlock(&info->mutex);

	//pdata->cfg_gpio();
	return ret;
}

static int sec_nfc_resume(struct device *dev)
{
	//struct sec_nfc_platform_data *pdata = dev->platform_data;
	//pdata->cfg_gpio();
	return 0;
}

static SIMPLE_DEV_PM_OPS(sec_nfc_pm_ops, sec_nfc_suspend, sec_nfc_resume);
#endif

#ifdef	CONFIG_SEC_NFC_I2C
static int __devinit sec_nfc_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
#else
static int sec_nfc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
#endif
	struct sec_nfc_info *info = NULL;
	//struct sec_nfc_platform_data *pdata;
	struct sec_nfc_platform_data *pdata = NULL;
	int ret = 0;
	int err;

	pr_info("%s: enter - sec-nfc probe start\n", __func__);

    // Check tamper
    if (check_custom_kernel() == 1)
    {
//        pr_info("%s: The kernel is tampered. Couldn't initialize NFC. \n", __func__);
//        return -EPERM;
    }

	if(dev) {
		pr_info("%s: alloc for platform data\n", __func__);
		pdata = kzalloc(sizeof(struct sec_nfc_platform_data), GFP_KERNEL);

		if (!pdata) {
			dev_err(dev, "No platform data\n");
			ret = -ENOMEM;
			goto err_pdata;
		}
	}
	else {
		pr_info("%s: failed alloc platform data\n", __func__);
	}

	err = sec_nfc_parse_dt(dev, pdata);

	ret = gpio_request(pdata->tvdd, "nfc_tvdd");
		if (ret) {
			dev_err(dev, "failed to get gpio tvdd\n");
			goto err_gpio_tvdd;
		}
	ret = gpio_tlmm_config (GPIO_CFG(41, GPIOMUX_FUNC_GPIO,
					GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
	if (ret) {
		dev_err(dev, "failed to configure GPIO_41. ret %d \n", ret);
	} else {

		if (poweroff_charging) {
			pr_info("%s: [poweroff_charging] Setting the GPIO_41 pin LOW\n",__func__);
			gpio_set_value(41, 0);
			sec_nfc_uart_suspend();
		} else {
			pr_info("%s: [Normal case] Setting the GPIO_41 pin HIGH \n",__func__);
			gpio_set_value(41, 1);
		}
			pr_info("%s: Set the GPIO_41 (%d) to HIGH. \n", __func__, pdata->tvdd);
	}

	pr_info("gpio assign success!\n");

	info = kzalloc(sizeof(struct sec_nfc_info), GFP_KERNEL);
	if (!info) {
		dev_err(dev, "failed to allocate memory for sec_nfc_info\n");
		ret = -ENOMEM;
		kfree(pdata);
		goto err_info_alloc;
	}
	info->dev = dev;
	info->pdata = pdata;
	info->state = SEC_NFC_ST_OFF;

	mutex_init(&info->mutex);

	dev_set_drvdata(dev, info);

	//pdata->cfg_gpio();

#ifdef	CONFIG_SEC_NFC_I2C
	info->buflen = SEC_NFC_MAX_BUFFER_SIZE;
	info->buf = kzalloc(SEC_NFC_MAX_BUFFER_SIZE, GFP_KERNEL);
	if (!info->buf) {
		dev_err(dev,
			"failed to allocate memory for sec_nfc_info->buf\n");
		ret = -ENOMEM;
		kfree(pdata);
		goto err_buf_alloc;
	}
	info->i2c_dev = client;
	info->read_irq = SEC_NFC_NONE;
	mutex_init(&info->read_mutex);
	init_waitqueue_head(&info->read_wait);
	i2c_set_clientdata(client, info);

	ret = request_threaded_irq(pdata->irq, NULL, sec_nfc_irq_thread_fn,
			IRQF_TRIGGER_RISING | IRQF_ONESHOT, SEC_NFC_DRIVER_NAME,
			info);
	if (ret < 0) {
		dev_err(dev, "failed to register IRQ handler\n");
		goto err_irq_req;
	}

#endif

	wake_lock_init(&info->wake_lock, WAKE_LOCK_SUSPEND, "NFCWAKE");

	info->miscdev.minor = MISC_DYNAMIC_MINOR;
	info->miscdev.name = SEC_NFC_DRIVER_NAME;
	info->miscdev.fops = &sec_nfc_fops;
	info->miscdev.parent = dev;
	ret = misc_register(&info->miscdev);

	if (ret < 0) {
		dev_err(dev, "failed to register Device\n");
		goto err_dev_reg;
	}

	ret = gpio_request(pdata->ven, "nfc_ven");
	if (ret) {
		dev_err(dev, "failed to get gpio ven\n");
		goto err_gpio_ven;
	}

	ret = gpio_request(pdata->firm, "nfc_firm");
	if (ret) {
		dev_err(dev, "failed to get gpio firm\n");
		goto err_gpio_firm;
	}

	gpio_direction_output(pdata->ven, 0);
	gpio_direction_output(pdata->firm, 0);


	dev_dbg(dev, "%s: info: %p, pdata %p\n", __func__, info, pdata);

#ifdef BU80003GUL
/*	ret = i2c_add_driver(&bu80003gul_i2c_driver);
	if (ret) {
		dev_err(dev,"%s Failed to add the i2c driver for the EEPROM chip. ret:%d \n",__func__, ret);
		goto err_gpio_firm;
	}
*/
#endif /* BU80003GUL */

	pr_info("%s: exit - sec-nfc probe finish\n", __func__);

	return 0;
err_gpio_tvdd:
	gpio_free(pdata->tvdd);
err_gpio_firm:
	gpio_free(pdata->firm);
err_gpio_ven:
	gpio_free(pdata->ven);
err_dev_reg:
#ifdef	CONFIG_SEC_NFC_I2C
err_irq_req:
err_buf_alloc:
#endif
err_info_alloc:

	if (info != NULL) {
		if (info->pdata != NULL) {
			kfree(info->pdata);
		}
		wake_lock_destroy(&info->wake_lock);
	kfree(info);
	}
err_pdata:
	pr_info("%s: exit - sec-nfc probe finish with ERROR! - ret = 0x%X\n", __func__, ret);
	return ret;
}


static void sec_nfc_shutdown(struct platform_device *pdev)
{
	struct sec_nfc_info *info = dev_get_drvdata(&pdev->dev);
	struct sec_nfc_platform_data *pdata = info->pdata;

	if (pdata->tvdd)
		gpio_set_value(pdata->tvdd, 0);
}


#ifdef	CONFIG_SEC_NFC_I2C
static int __devexit sec_nfc_remove(struct i2c_client *client)
{
	struct sec_nfc_info *info = i2c_get_clientdata(client);
	struct sec_nfc_platform_data *pdata = client->dev.platform_data;
#else
static int sec_nfc_remove(struct platform_device *pdev)
{
	struct sec_nfc_info *info = dev_get_drvdata(&pdev->dev);
	struct sec_nfc_platform_data *pdata = pdev->dev.platform_data;
#endif
	if(info == NULL) {
		goto ERR_SEC_NFC_REMOVE_INFO;
	}
	dev_dbg(info->dev, "%s\n", __func__);

	misc_deregister(&info->miscdev);

	if (info->state != SEC_NFC_ST_OFF) {
		gpio_set_value(pdata->firm, 0);
		gpio_set_value(pdata->ven, 0);
	}
	if(info->pdata != NULL) {
		kfree(info->pdata);
	}
	kfree(info);
ERR_SEC_NFC_REMOVE_INFO:
	gpio_free(pdata->firm);
	gpio_free(pdata->ven);

#ifdef	CONFIG_SEC_NFC_I2C
	free_irq(pdata->irq, info);
#endif

	wake_lock_destroy(&info->wake_lock);

#ifdef BU80003GUL
	i2c_del_driver(&bu80003gul_i2c_driver);
	felica_epc_deregister(); /*Naveen : TODO*/
	class_destroy(eeprom_class);
#endif
	return 0;
}

#ifdef	CONFIG_SEC_NFC_I2C
static struct i2c_device_id sec_nfc_id_table[] = {
#else	/* CONFIG_SEC_NFC_I2C */
static struct platform_device_id sec_nfc_id_table[] = {
#endif
	{ SEC_NFC_DRIVER_NAME, 0 },
	{ }
};

static struct of_device_id nfc_match_table[] = {
	{ .compatible = SEC_NFC_DRIVER_NAME, },
	{},
};

#ifdef	CONFIG_SEC_NFC_I2C
MODULE_DEVICE_TABLE(i2c, sec_nfc_id_table);
static struct i2c_driver sec_nfc_driver = {
#else
MODULE_DEVICE_TABLE(platform, sec_nfc_id_table);
static struct platform_driver sec_nfc_driver = {
#endif
	.probe = sec_nfc_probe,
	.id_table = sec_nfc_id_table,
	.shutdown = sec_nfc_shutdown,
	.remove = sec_nfc_remove,
	.driver = {
		.name = SEC_NFC_DRIVER_NAME,
#ifdef CONFIG_PM
		.pm = &sec_nfc_pm_ops,
#endif
		.of_match_table = nfc_match_table,
	},
};

#ifdef	CONFIG_SEC_NFC_I2C
static int __init sec_nfc_init(void)
{
	return i2c_add_driver(&sec_nfc_driver);
}

static void __exit sec_nfc_exit(void)
{
	i2c_del_driver(&sec_nfc_driver);
}
#endif

#ifdef	CONFIG_SEC_NFC_I2C
module_init(sec_nfc_init);
module_exit(sec_nfc_exit);
#else
module_platform_driver(sec_nfc_driver);
module_i2c_driver(bu80003gul_i2c_driver);
#endif


MODULE_DESCRIPTION("Samsung sec_nfc driver");
MODULE_LICENSE("GPL");

#endif /* #ifndef BU80003GUL */
