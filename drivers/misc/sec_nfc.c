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
 *
 * Last update: 2014-01-20
 *
 */
#ifdef CONFIG_SEC_NFC_IF_I2C_GPIO
#define CONFIG_SEC_NFC_IF_I2C
#endif

#include <linux/wait.h>
#include <linux/delay.h>

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
#include <linux/wakelock.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_SEC_NFC_CLK_REQ
#include <linux/interrupt.h>
#include <linux/clk.h>
#endif
#if defined(CONFIG_NFC_N5_PMC8974_CLK_REQ) || defined(CONFIG_SEC_NFC_USE_8226_BBCLK2)
#include <linux/clk.h>
#endif

#ifndef CONFIG_SEC_NFC_IF_I2C
struct sec_nfc_i2c_info {};
#define sec_nfc_read			NULL
#define sec_nfc_write			NULL
#define sec_nfc_poll			NULL
#define sec_nfc_i2c_irq_clear(x)

#define SEC_NFC_GET_INFO(dev) platform_get_drvdata(to_platform_device(dev))

#else /* CONFIG_SEC_NFC_IF_I2C */
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/i2c.h>

#define SEC_NFC_GET_INFO(dev) i2c_get_clientdata(to_i2c_client(dev))
enum sec_nfc_irq {
	SEC_NFC_NONE,
	SEC_NFC_INT,
	SEC_NFC_SKIP,
};

struct sec_nfc_i2c_info {
	struct i2c_client *i2c_dev;
	struct mutex read_mutex;
	enum sec_nfc_irq read_irq;
	wait_queue_head_t read_wait;
	size_t buflen;
	u8 *buf;
};

#endif

struct sec_nfc_info {
	struct miscdevice miscdev;
	struct mutex mutex;
	enum sec_nfc_mode mode;
	struct device *dev;
	struct sec_nfc_platform_data *pdata;
	struct sec_nfc_i2c_info i2c_info;
#ifdef	CONFIG_SEC_NFC_CLK_REQ
	bool clk_ctl;
	bool clk_state;
#endif
	struct wake_lock wake_lock;
	struct regulator *L22;
	struct regulator *L6;
};

#ifndef CONFIG_SEC_NFC_NO_POWER_CONTROL
int nfc_power_onoff(struct sec_nfc_info *info, bool onoff);
#endif

#ifdef CONFIG_SEC_NFC_IF_I2C
static irqreturn_t sec_nfc_irq_thread_fn(int irq, void *dev_id)
{
	struct sec_nfc_info *info = dev_id;
	struct sec_nfc_platform_data *pdata = info->pdata;

	dev_dbg(info->dev, "[NFC] Read Interrupt is occurred!\n");

	if(gpio_get_value(pdata->irq) == 0) {
		dev_err(info->dev, "[NFC] Warning,irq-gpio state is low!\n");
		return IRQ_HANDLED;
    }

	mutex_lock(&info->i2c_info.read_mutex);
	/* Skip during power switching */
	if (info->i2c_info.read_irq == SEC_NFC_SKIP) {
		mutex_unlock(&info->i2c_info.read_mutex);
		return IRQ_HANDLED;
	}
	info->i2c_info.read_irq = SEC_NFC_INT;
	mutex_unlock(&info->i2c_info.read_mutex);

	wake_up_interruptible(&info->i2c_info.read_wait);

	if(!wake_lock_active(&info->wake_lock))
	{
		dev_dbg(info->dev, "%s: Set wake_lock_timeout for 2 sec. !!!\n", __func__);
		wake_lock_timeout(&info->wake_lock, 2 * HZ);
	}

	return IRQ_HANDLED;
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

	if (info->mode == SEC_NFC_MODE_OFF) {
		dev_err(info->dev, "sec_nfc is not enabled\n");
		ret = -ENODEV;
		goto out;
	}

	mutex_lock(&info->i2c_info.read_mutex);
	irq = info->i2c_info.read_irq;
	mutex_unlock(&info->i2c_info.read_mutex);
	if (irq == SEC_NFC_NONE) {
		if (file->f_flags & O_NONBLOCK) {
			dev_err(info->dev, "it is nonblock\n");
			ret = -EAGAIN;
			goto out;
		}
	}

	/* i2c recv */
	if (count > info->i2c_info.buflen)
		count = info->i2c_info.buflen;

	if (count > SEC_NFC_MSG_MAX_SIZE) {
		dev_err(info->dev, "user required wrong size :%d\n", count);
		ret = -EINVAL;
		goto out;
	}

	mutex_lock(&info->i2c_info.read_mutex);
	memset(info->i2c_info.buf, 0, count);
	ret = i2c_master_recv(info->i2c_info.i2c_dev, info->i2c_info.buf, count);
	dev_dbg(info->dev, "recv size : %d\n", ret);

	if (ret == -EREMOTEIO) {
		ret = -ERESTART;
		goto read_error;
	} else if (ret != count) {
		dev_err(info->dev, "read failed: return: %d count: %d\n",
			ret, count);
		//ret = -EREMOTEIO;
		goto read_error;
	}

	info->i2c_info.read_irq = SEC_NFC_NONE;
	mutex_unlock(&info->i2c_info.read_mutex);

	if (copy_to_user(buf, info->i2c_info.buf, ret)) {
		dev_err(info->dev, "copy failed to user\n");
		ret = -EFAULT;
	}

	goto out;

read_error:
	info->i2c_info.read_irq = SEC_NFC_NONE;
	mutex_unlock(&info->i2c_info.read_mutex);
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

	dev_dbg(info->dev, "%s: info: %p, count %zu\n", __func__,
		info, count);

	mutex_lock(&info->mutex);

	if (info->mode == SEC_NFC_MODE_OFF) {
		dev_err(info->dev, "sec_nfc is not enabled\n");
		ret = -ENODEV;
		goto out;
	}

	if (count > info->i2c_info.buflen)
		count = info->i2c_info.buflen;

	if (count > SEC_NFC_MSG_MAX_SIZE) {
		dev_err(info->dev, "user required wrong size :%d\n", count);
		ret = -EINVAL;
		goto out;
	}

	if (copy_from_user(info->i2c_info.buf, buf, count)) {
		dev_err(info->dev, "copy failed from user\n");
		ret = -EFAULT;
		goto out;
	}
	/* Skip interrupt during power switching
	 * It is released after first write */
	mutex_lock(&info->i2c_info.read_mutex);
	ret = i2c_master_send(info->i2c_info.i2c_dev, info->i2c_info.buf, count);
	if (info->i2c_info.read_irq == SEC_NFC_SKIP)
		info->i2c_info.read_irq = SEC_NFC_NONE;
	mutex_unlock(&info->i2c_info.read_mutex);

	if (ret == -EREMOTEIO) {
		dev_err(info->dev, "send failed: return: %d count: %d\n",
		ret, count);
		ret = -ERESTART;
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

static unsigned int sec_nfc_poll(struct file *file, poll_table *wait)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);
	enum sec_nfc_irq irq;

	int ret = 0;

	dev_dbg(info->dev, "%s: info: %p\n", __func__, info);

	mutex_lock(&info->mutex);

	if (info->mode == SEC_NFC_MODE_OFF) {
		dev_err(info->dev, "sec_nfc is not enabled\n");
		ret = -ENODEV;
		goto out;
	}

	poll_wait(file, &info->i2c_info.read_wait, wait);

	mutex_lock(&info->i2c_info.read_mutex);
	irq = info->i2c_info.read_irq;
	if (irq == SEC_NFC_INT)
		ret = (POLLIN | POLLRDNORM);
	mutex_unlock(&info->i2c_info.read_mutex);

out:
	mutex_unlock(&info->mutex);

	return ret;
}

void sec_nfc_i2c_irq_clear(struct sec_nfc_info *info)
{
	/* clear interrupt. Interrupt will be occured at power off */
	mutex_lock(&info->i2c_info.read_mutex);
	info->i2c_info.read_irq = SEC_NFC_NONE;
	mutex_unlock(&info->i2c_info.read_mutex);
}

int sec_nfc_i2c_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct sec_nfc_info *info = dev_get_drvdata(dev);
	struct sec_nfc_platform_data *pdata = info->pdata;
	int ret;

	pr_err("\n %s start", __func__);

	pr_err("info      : %p", info);
	pr_err("pdata     : %p", pdata);
	pr_err("pdata->irq: %d", pdata->irq);
	pr_err("client->irq:%d", client->irq);
	//pdata->irq = client->irq;
	info->i2c_info.buflen = SEC_NFC_MAX_BUFFER_SIZE;
	info->i2c_info.buf = kzalloc(SEC_NFC_MAX_BUFFER_SIZE, GFP_KERNEL);
	if (!info->i2c_info.buf) {
		dev_err(dev,
			"failed to allocate memory for sec_nfc_info->buf\n");
		return -ENOMEM;
	}
	info->i2c_info.i2c_dev = client;
	info->i2c_info.read_irq = SEC_NFC_NONE;
	mutex_init(&info->i2c_info.read_mutex);
	init_waitqueue_head(&info->i2c_info.read_wait);
	i2c_set_clientdata(client, info);
#ifndef CONFIG_SEC_NFC_NO_POWER_CONTROL
#if !defined(CONFIG_MACH_KSPORTSLTE_SPR)&& !defined(CONFIG_MACH_SLTE_SPR)
	nfc_power_onoff(info,1);
#endif
#endif
	ret = request_threaded_irq(client->irq, NULL, sec_nfc_irq_thread_fn,
			IRQF_TRIGGER_RISING | IRQF_ONESHOT, SEC_NFC_DRIVER_NAME,
			info);
	if (ret < 0) {
		dev_err(dev, "failed to register IRQ handler\n");
		kfree(info->i2c_info.buf);
		return ret;
	}

	pr_err("\n %s success", __func__);
	return 0;
}

void sec_nfc_i2c_remove(struct device *dev)
{
	struct sec_nfc_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->i2c_info.i2c_dev;
	struct sec_nfc_platform_data *pdata = info->pdata;
	free_irq(client->irq, info);
	gpio_free(pdata->irq);
}
#endif /* CONFIG_SEC_NFC_IF_I2C */

#ifdef	CONFIG_SEC_NFC_CLK_REQ

#ifdef CONFIG_SEC_NFC_USE_8226_RFCLK2
/* PMIC */
#define sec_nfc_clk_on(clk)				clk_prepare_enable(clk)
#define sec_nfc_clk_off(clk)			clk_disable_unprepare(clk)

#else
/* default GPIO */
#define sec_nfc_clk_on(clk)				gpio_set_value(clk, 1)
#define sec_nfc_clk_off(clk)			gpio_set_value(clk, 0)
#endif

static irqreturn_t sec_nfc_clk_irq_thread(int irq, void *dev_id)
{
	struct sec_nfc_info *info = dev_id;
	struct sec_nfc_platform_data *pdata = info->pdata;
	bool value;

	value = gpio_get_value(pdata->clk_req) > 0 ? false : true;

	if (value == info->clk_state)
		return IRQ_HANDLED;

	//pr_err("\n %s: %s", __func__, value ? "True" : "False");
	if (value)
		sec_nfc_clk_on(pdata->clk_enable);
	else
		sec_nfc_clk_off(pdata->clk_enable);

	info->clk_state = value;

	return IRQ_HANDLED;
}

void sec_nfc_clk_ctl_enable(struct sec_nfc_info *info)
{
	struct sec_nfc_platform_data *pdata = info->pdata;
	unsigned int irq = gpio_to_irq(pdata->clk_req);
	int ret;

	pr_err("\n %s start", __func__);

	if (info->clk_ctl)
		return;

	ret = gpio_request(pdata->clk_req, "nfc-ex-clk");
	if (ret) {
		dev_err(info->dev, "failed to get gpio ven\n");
		return;
	}

	info->clk_state = false;
	ret = request_threaded_irq(irq, NULL, sec_nfc_clk_irq_thread,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			SEC_NFC_DRIVER_NAME, info);
	if (ret < 0) {
		dev_err(info->dev, "failed to register CLK REQ IRQ handler\n");
	}
	info->clk_ctl = true;
}

void sec_nfc_clk_ctl_disable(struct sec_nfc_info *info)
{
	struct sec_nfc_platform_data *pdata = info->pdata;
	unsigned int irq = gpio_to_irq(pdata->clk_req);

	pr_err("\n %s start", __func__);

	if (!info->clk_ctl)
		return;

	free_irq(irq, info);
	gpio_free(pdata->clk_req);
	if (info->clk_state)
		sec_nfc_clk_off(pdata->clk_enable);

	info->clk_state = false;
	info->clk_ctl = false;
}
#else
#define sec_nfc_clk_ctl_enable(x)
#define sec_nfc_clk_ctl_disable(x)
#endif /* CONFIG_SEC_NFC_CLK_REQ */

static void sec_nfc_set_mode(struct sec_nfc_info *info,
					enum sec_nfc_mode mode)
{
	struct sec_nfc_platform_data *pdata = info->pdata;
	/* intfo lock is aleady gotten before calling this function */
	if (info->mode == mode) {
		dev_dbg(info->dev, "Power mode is already %d", mode);
		return;
	}
	pr_err("\n %s start %d", __func__, mode);
	/* intfo lock is aleady getten before calling this function */
	info->mode = mode;
#ifdef CONFIG_SEC_NFC_IF_I2C
	/* Skip during power switching */
	mutex_lock(&info->i2c_info.read_mutex);
	info->i2c_info.read_irq = SEC_NFC_SKIP;
	mutex_unlock(&info->i2c_info.read_mutex);
#endif
	gpio_set_value_cansleep(pdata->ven, 0);
	gpio_set_value(pdata->ven, SEC_NFC_PW_OFF);
	if (pdata->firm) gpio_set_value(pdata->firm, SEC_NFC_FW_OFF);

	if (mode == SEC_NFC_MODE_BOOTLOADER)
		if (pdata->firm) gpio_set_value(pdata->firm, SEC_NFC_FW_ON);

	if (mode != SEC_NFC_MODE_OFF) {
		msleep(SEC_NFC_VEN_WAIT_TIME);
	    gpio_set_value_cansleep(pdata->ven, 1);
		gpio_set_value(pdata->ven, SEC_NFC_PW_ON);
		sec_nfc_clk_ctl_enable(info);
#ifdef CONFIG_SEC_NFC_IF_I2C
		enable_irq_wake(info->i2c_info.i2c_dev->irq);
#endif
		msleep(SEC_NFC_VEN_WAIT_TIME/2);
	} else {
		sec_nfc_clk_ctl_disable(info);
#ifdef CONFIG_SEC_NFC_IF_I2C
		disable_irq_wake(info->i2c_info.i2c_dev->irq);
#endif
	}

	dev_dbg(info->dev, "Power mode is : %d\n", mode);
}

static long sec_nfc_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);
#ifdef CONFIG_SEC_NFC_PRODUCT_N5
	struct sec_nfc_platform_data *pdata = info->pdata;
#endif
	unsigned int new = (unsigned int)arg;
	int ret = 0;

	dev_dbg(info->dev, "%s: info: %p, cmd: 0x%x\n",
			__func__, info, cmd);

	mutex_lock(&info->mutex);

	switch (cmd) {
	case SEC_NFC_SET_MODE:
		dev_dbg(info->dev, "%s: SEC_NFC_SET_MODE\n", __func__);

		if (info->mode == new)
			break;

		if (new >= SEC_NFC_MODE_COUNT) {
			dev_err(info->dev, "wrong mode (%d)\n", new);
			ret = -EFAULT;
			break;
		}
		sec_nfc_set_mode(info, new);

		break;

#ifdef CONFIG_SEC_NFC_PRODUCT_N3
	case SEC_NFC_SLEEP:
	case SEC_NFC_WAKEUP:
		break;

#elif defined(CONFIG_SEC_NFC_PRODUCT_N5)
	case SEC_NFC_SLEEP:
		if (info->mode != SEC_NFC_MODE_BOOTLOADER) {
			if(wake_lock_active(&info->wake_lock))
				wake_unlock(&info->wake_lock);
			gpio_set_value(pdata->wake, SEC_NFC_WAKE_SLEEP);
		}
		break;

	case SEC_NFC_WAKEUP:
		if (info->mode != SEC_NFC_MODE_BOOTLOADER) {
			gpio_set_value(pdata->wake, SEC_NFC_WAKE_UP);
			if(!wake_lock_active(&info->wake_lock))
				wake_lock(&info->wake_lock);
		}
		break;
#endif

	default:
		dev_err(info->dev, "Unknow ioctl 0x%x\n", cmd);
		ret = -ENOIOCTLCMD;
		break;
	}

	mutex_unlock(&info->mutex);

	return ret;
}

static int sec_nfc_open(struct inode *inode, struct file *file)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);
	int ret = 0;

	pr_err("\n %s start", __func__);
	dev_dbg(info->dev, "%s: info : %p" , __func__, info);

	mutex_lock(&info->mutex);
	if (info->mode != SEC_NFC_MODE_OFF) {
		dev_err(info->dev, "sec_nfc is busy\n");
		ret = -EBUSY;
		goto out;
	}

	sec_nfc_set_mode(info, SEC_NFC_MODE_OFF);
out:
	mutex_unlock(&info->mutex);
	return ret;
}

static int sec_nfc_close(struct inode *inode, struct file *file)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);

	pr_err("\n %s start", __func__);
	dev_dbg(info->dev, "%s: info : %p" , __func__, info);

	mutex_lock(&info->mutex);
	sec_nfc_set_mode(info, SEC_NFC_MODE_OFF);
	mutex_unlock(&info->mutex);

	return 0;
}

static const struct file_operations sec_nfc_fops = {
	.owner		= THIS_MODULE,
	.read		= sec_nfc_read,
	.write		= sec_nfc_write,
	.poll		= sec_nfc_poll,
	.open		= sec_nfc_open,
	.release	= sec_nfc_close,
	.unlocked_ioctl	= sec_nfc_ioctl,
};

#ifndef CONFIG_SEC_NFC_NO_POWER_CONTROL
int nfc_power_onoff(struct sec_nfc_info *data, bool onoff)
{
	int ret = -1;
	if (!data->L22) {
		data->L22 = devm_regulator_get(data->dev, "nfc_ldo");
		if (!data->L22) {
			pr_err("%s: regulator pointer null 8941_L22, rc=%d\n",
				__func__, ret);
			return ret;
		}
		ret = regulator_set_voltage(data->L22, 3000000, 3000000);
		if (ret) {
			pr_err("%s: set voltage failed on 8941_L22, rc=%d\n",
				__func__, ret);
			return ret;
		}
	}

	if (!data->L6) {
		data->L6 = devm_regulator_get(data->dev, "vdd_nfc");
		if (!data->L6) {
			pr_err("%s: regulator pointer null 8941_l6, rc=%d\n",
				__func__, ret);
			return ret;
		}
/*        
		ret = regulator_set_voltage(data->L6, 1800000, 1800000);
		if (ret) {
			pr_err("%s: set voltage failed on 8941_l6, rc=%d\n",
				__func__, ret);
			return ret;
		}
*/
	}

	if(onoff){
#if !defined(CONFIG_MACH_ATLANTICLTE_VZW) && !defined(CONFIG_MACH_ATLANTICLTE_ATT) && !defined(CONFIG_MACH_ATLANTICLTE_USC)
		ret = regulator_enable(data->L22);
		regulator_set_mode(data->L22, REGULATOR_MODE_NORMAL);
		if (ret) {
			pr_err("%s: Failed to enable regulator L22.\n",
				__func__);
			return ret;
		}
#endif

		ret = regulator_enable(data->L6);
		if (ret) {
			pr_err("%s: Failed to enable regulator L6.\n",
				__func__);
			return ret;
		}

	}
	else {
#if !defined(CONFIG_MACH_ATLANTICLTE_VZW) && !defined(CONFIG_MACH_ATLANTICLTE_ATT) && !defined(CONFIG_MACH_ATLANTICLTE_USC)
		ret = regulator_disable(data->L22);
		if (ret) {
			pr_err("%s: Failed to disable regulatorL22.\n",
				__func__);
			return ret;
		}
#endif

		ret = regulator_disable(data->L6);
		if (ret) {
			pr_err("%s: Failed to disable regulator L6.\n",
				__func__);
			return ret;
		}

	}
	return 0;
}
#endif

#ifdef CONFIG_PM
static int sec_nfc_suspend(struct device *dev)
{
	struct sec_nfc_info *info = SEC_NFC_GET_INFO(dev);
	int ret = 0;

	mutex_lock(&info->mutex);

	if (info->mode == SEC_NFC_MODE_BOOTLOADER)
		ret = -EPERM;

	mutex_unlock(&info->mutex);

#ifndef CONFIG_SEC_NFC_NO_POWER_CONTROL
#if !defined(CONFIG_MACH_KSPORTSLTE_SPR)&& !defined(CONFIG_MACH_SLTE_SPR)
	nfc_power_onoff(info,0);
#endif
#endif
	return ret;
}

static int sec_nfc_resume(struct device *dev)
{
#ifndef CONFIG_SEC_NFC_NO_POWER_CONTROL
#if !defined(CONFIG_MACH_KSPORTSLTE_SPR)&& !defined(CONFIG_MACH_SLTE_SPR)
	struct sec_nfc_info *info = SEC_NFC_GET_INFO(dev);
	nfc_power_onoff(info,1);
#endif
#endif
	return 0;
}

static SIMPLE_DEV_PM_OPS(sec_nfc_pm_ops, sec_nfc_suspend, sec_nfc_resume);
#endif

#ifdef CONFIG_OF
/*device tree parsing*/
static int sec_nfc_parse_dt(struct device *dev,
	struct sec_nfc_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	pdata->ven = of_get_named_gpio_flags(np, "sec-nfc,ven-gpio",
		0, &pdata->ven_gpio_flags);
	pdata->firm = of_get_named_gpio_flags(np, "sec-nfc,firm-gpio",
		0, &pdata->firm_gpio_flags);
	pdata->wake = pdata->firm;
	pdata->irq = of_get_named_gpio_flags(np, "sec-nfc,irq-gpio",
		0, &pdata->irq_gpio_flags);
#ifdef CONFIG_SEC_NFC_CLK_REQ
	pdata->clk_req = of_get_named_gpio_flags(np, "sec-nfc,clk_req-gpio",
		0, &pdata->irq_gpio_flags);
#ifdef CONFIG_SEC_NFC_USE_8226_RFCLK2
	pdata->clk_enable = clk_get(dev, "nfc_clock");
#else
	pdata->clk_enable = of_get_named_gpio_flags(np, "sec-nfc,ext_clk-gpio",
		0, &pdata->irq_gpio_flags);
#endif
#endif
	if (pdata->firm < 0)
		of_property_read_u32(np, "sec-nfc,firm-expander-gpio", &pdata->firm);
	pdata->wake = pdata->firm;
	pr_info("%s: irq : %d, ven : %d, firm : %d\n",
			__func__, pdata->irq, pdata->ven, pdata->firm);
	return 0;
}
#else
static int sec_nfc_parse_dt(struct device *dev,
	struct sec_nfc_platform_data *pdata)
{
	return -ENODEV;
}
#endif

static int __sec_nfc_probe(struct device *dev)
{
	struct sec_nfc_info *info;
	struct sec_nfc_platform_data *pdata = NULL;
	int ret = 0;

	pr_err("\n %s start", __func__);
	if (dev->of_node) {
		pdata = devm_kzalloc(dev,
			sizeof(struct sec_nfc_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		ret = sec_nfc_parse_dt(dev, pdata);
		if (ret)
			return ret;
	} else {
		pdata = dev->platform_data;
	}

	if (!pdata) {
		dev_err(dev, "No platform data\n");
		ret = -ENOMEM;
		goto err_pdata;
	}

#ifdef CONFIG_SEC_NFC_USE_8226_BBCLK2
	pdata->nfc_clk = clk_get(NULL, "nfc_clock");
	if (IS_ERR(pdata->nfc_clk)) {
		ret = PTR_ERR(pdata->nfc_clk);
		printk(KERN_ERR "%s: Couldn't get D1 (%d)\n",
					__func__, ret);
	} else {
		if (clk_prepare_enable(pdata->nfc_clk))
			printk(KERN_ERR "%s: Couldn't prepare D1\n",
					__func__);
	}
#endif
#ifdef CONFIG_NFC_N5_PMC8974_CLK_REQ
	pdata->nfc_clk = clk_get(NULL, "nfc_clk");
	if (IS_ERR(pdata->nfc_clk)) {
		ret = PTR_ERR(pdata->nfc_clk);
		printk(KERN_ERR "%s: Couldn't get D1 (%d)\n",
					__func__, ret);
	} else {
		if (clk_prepare_enable(pdata->nfc_clk))
			printk(KERN_ERR "%s: Couldn't prepare D1\n",
					__func__);
	}
#endif

	info = kzalloc(sizeof(struct sec_nfc_info), GFP_KERNEL);
	if (!info) {
		dev_err(dev, "failed to allocate memory for sec_nfc_info\n");
		ret = -ENOMEM;
		goto err_info_alloc;
	}
	info->dev = dev;
	info->pdata = pdata;
	info->mode = SEC_NFC_MODE_OFF;

	mutex_init(&info->mutex);
	dev_set_drvdata(dev, info);

	info->miscdev.minor = MISC_DYNAMIC_MINOR;
	info->miscdev.name = SEC_NFC_DRIVER_NAME;
	info->miscdev.fops = &sec_nfc_fops;
	info->miscdev.parent = dev;
	ret = misc_register(&info->miscdev);
	if (ret < 0) {
		dev_err(dev, "failed to register Device\n");
		goto err_dev_reg;
	}

	ret = gpio_request(pdata->ven, "ven-gpio");
	if (ret) {
		dev_err(dev, "failed to get gpio ven\n");
		goto err_gpio_ven;
	}
	gpio_direction_output(pdata->ven, SEC_NFC_PW_OFF);

	if (pdata->firm)
	{
		ret = gpio_request(pdata->firm, "firm-gpio");
		if (ret) {
			dev_err(dev, "failed to get gpio firm\n");
			goto err_gpio_firm;
		}
		gpio_direction_output(pdata->firm, SEC_NFC_FW_OFF);
	}

	wake_lock_init(&info->wake_lock, WAKE_LOCK_SUSPEND, "NFCWAKE");

	dev_dbg(dev, "%s: info: %p, pdata %p\n", __func__, info, pdata);
	pr_err("\n %s success", __func__);

	return 0;

err_gpio_firm:
	gpio_free(pdata->ven);
err_gpio_ven:
err_dev_reg:
err_info_alloc:
	kfree(info);
err_pdata:
	return ret;
}

static int __sec_nfc_remove(struct device *dev)
{
	struct sec_nfc_info *info = dev_get_drvdata(dev);
	struct sec_nfc_platform_data *pdata = info->pdata;

	dev_dbg(info->dev, "%s\n", __func__);
#if defined(CONFIG_NFC_N5_PMC8974_CLK_REQ) || defined(CONFIG_SEC_NFC_USE_8226_BBCLK2)
	if (pdata->nfc_clk)
		clk_unprepare(pdata->nfc_clk);
#endif
	misc_deregister(&info->miscdev);
	sec_nfc_set_mode(info, SEC_NFC_MODE_OFF);
	gpio_set_value(pdata->firm, 0);
	gpio_set_value_cansleep(pdata->ven, 0);
	gpio_free(pdata->ven);
	if (pdata->firm) gpio_free(pdata->firm);
	wake_lock_destroy(&info->wake_lock);

	kfree(info);

	return 0;
}

#ifdef CONFIG_SEC_NFC_IF_I2C
MODULE_DEVICE_TABLE(i2c, sec_nfc_id_table);
typedef struct i2c_driver sec_nfc_driver_type;
#define SEC_NFC_INIT(driver)	i2c_add_driver(driver);
#define SEC_NFC_EXIT(driver)	i2c_del_driver(driver);

static int __devinit sec_nfc_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = 0;

	ret = __sec_nfc_probe(&client->dev);
	if (ret)
		return ret;

	if (sec_nfc_i2c_probe(client))
		__sec_nfc_remove(&client->dev);

	return ret;
}

static int __devexit sec_nfc_remove(struct i2c_client *client)
{
	sec_nfc_i2c_remove(&client->dev);
	return __sec_nfc_remove(&client->dev);
}

static struct i2c_device_id sec_nfc_id_table[] = {
	{ SEC_NFC_DRIVER_NAME, 0 },
	{ }
};

#else	/* CONFIG_SEC_NFC_IF_I2C */
MODULE_DEVICE_TABLE(platform, sec_nfc_id_table);
typedef struct platform_driver sec_nfc_driver_type;
#define SEC_NFC_INIT(driver)	platform_driver_register(driver);
#define SEC_NFC_EXIT(driver)	platform_driver_unregister(driver);

static int __devinit sec_nfc_probe(struct platform_device *pdev)
{
	return __sec_nfc_probe(&pdev->dev);
}

static int __devexit sec_nfc_remove(struct platform_device *pdev)
{
	return __sec_nfc_remove(&pdev->dev);
}

static struct platform_device_id sec_nfc_id_table[] = {
	{ SEC_NFC_DRIVER_NAME, 0 },
	{ }
};

#endif /* CONFIG_SEC_NFC_IF_I2C */

#ifdef CONFIG_OF
static struct of_device_id nfc_match_table[] = {
	{ .compatible = SEC_NFC_DRIVER_NAME,},
	{},
};
#else
#define nfc_match_table NULL
#endif

static sec_nfc_driver_type sec_nfc_driver = {
	.probe = sec_nfc_probe,
	.id_table = sec_nfc_id_table,
	.remove = sec_nfc_remove,
	.driver = {
		.name = SEC_NFC_DRIVER_NAME,
#ifdef CONFIG_PM
		.pm = &sec_nfc_pm_ops,
#endif
		.of_match_table = nfc_match_table,
	},
};

static int __init sec_nfc_init(void)
{
	return SEC_NFC_INIT(&sec_nfc_driver);
}

static void __exit sec_nfc_exit(void)
{
	SEC_NFC_EXIT(&sec_nfc_driver);
}

module_init(sec_nfc_init);
module_exit(sec_nfc_exit);

MODULE_DESCRIPTION("Samsung sec_nfc driver");
MODULE_LICENSE("GPL");
