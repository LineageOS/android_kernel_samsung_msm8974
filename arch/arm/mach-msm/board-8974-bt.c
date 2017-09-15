/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNOppU General Public License, version 2, as published by
 *  the Free Software Foundation (the "GPL").
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php,
 *  or by writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 *
 ************************************************************************************/
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/wakelock.h>

#include <net/bluetooth/bluetooth.h>
#include <asm/mach-types.h>
#include <mach/gpio.h>

MODULE_AUTHOR("Lin Ma <linm@broadcom.com>");
MODULE_DESCRIPTION("Broadcom Bluetooth GPIO and Lock");
MODULE_SUPPORTED_DEVICE("bcmbt");
MODULE_LICENSE("GPL");

#define GPIO_BTREG_ON	136

#define BCM_BT_NAME	"bcm_bt_bluetooth"
#define BTLOCK_NAME     "btlock"
#define BTLOCK_MINOR    224

/* BT lock waiting timeout, in second */
#define BTLOCK_TIMEOUT	2

#define PR(msg, ...) printk("####"msg, ##__VA_ARGS__)

struct btlock {
	int lock;
	int cookie;
};

static struct semaphore btlock;
static int count = 1;
static int owner_cookie = -1;

int bcm_bt_lock(int cookie)
{
	int ret;
	char cookie_msg[5] = {0};

	ret = down_timeout(&btlock, msecs_to_jiffies(BTLOCK_TIMEOUT*1000));
	if (ret == 0) {
		memcpy(cookie_msg, &cookie, sizeof(cookie));
		owner_cookie = cookie;
		count--;
		PR("btlock acquired cookie: %s\n", cookie_msg);
	}

	return ret;
}

void bcm_bt_unlock(int cookie)
{
	char owner_msg[5] = {0};
	char cookie_msg[5] = {0};

	memcpy(cookie_msg, &cookie, sizeof(cookie));
	if (owner_cookie == cookie) {
		owner_cookie = -1;
		if (count++ > 1)
			PR("error, release a lock that was not acquired**\n");
		up(&btlock);
		PR("btlock released, cookie: %s\n", cookie_msg);
	} else {
		memcpy(owner_msg, &owner_cookie, sizeof(owner_cookie));
		PR("ignore lock release,  cookie mismatch: %s owner %s \n", cookie_msg,
				owner_cookie == 0 ? "NULL" : owner_msg);
	}
}

EXPORT_SYMBOL(bcm_bt_lock);
EXPORT_SYMBOL(bcm_bt_unlock);

static int btlock_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int btlock_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t btlock_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	struct btlock lock_para;
	ssize_t ret = 0;

	if (count < sizeof(struct btlock))
		return -EINVAL;

	if (copy_from_user(&lock_para, buffer, sizeof(struct btlock))) {
		return -EFAULT;
	}

	if (lock_para.lock == 0) {
		bcm_bt_unlock(lock_para.cookie);
	} else if (lock_para.lock == 1) {
		ret = bcm_bt_lock(lock_para.cookie);
	} else if (lock_para.lock == 2) {
		ret = bcm_bt_lock(lock_para.cookie);
	}

	return ret;
}

static long btlock_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static const struct file_operations btlock_fops = {
	.owner   = THIS_MODULE,
	.open    = btlock_open,
	.release = btlock_release,
	.write   = btlock_write,
	.unlocked_ioctl = btlock_unlocked_ioctl,
};

static struct miscdevice btlock_misc = {
	.name  = BTLOCK_NAME,
	.minor = BTLOCK_MINOR,
	.fops  = &btlock_fops,
};

static int bcm_btlock_init(void)
{
	int ret;

	PR("init\n");

	ret = misc_register(&btlock_misc);
	if (ret != 0) {
		PR("Error: failed to register Misc driver,  ret = %d\n", ret);
		return ret;
	}
	sema_init(&btlock, 1);

	return ret;
}

static void bcm_btlock_exit(void)
{
	PR("btlock_exit:\n");

	misc_deregister(&btlock_misc);
}

#if 0
/*
 * Broadcom Bluetooth platform driver
 */
static int bcm_bt_rfkill_set_power(void *data, bool blocked)
{
	int lock_cookie_bt = 'B' | 'T'<<8 | '3'<<16 | '5'<<24;	/* cookie is "BT35" */

	printk("bcm_bt_rfkill_set_power\n");

	if (!blocked) {
		if (bcm_bt_lock(lock_cookie_bt) != 0)
			printk("** BT rfkill: timeout in acquiring bt lock**\n");

		pr_info("[BT] Bluetooth Power On. built at %s\n", __TIME__);
		gpio_set_value(GPIO_BTREG_ON, 1);
	} else {
		pr_info("[BT] Bluetooth Power Off. built at %s\n", __TIME__);
		gpio_set_value(GPIO_BTREG_ON, 0);
	}

	printk("return\n");
	return 0;
}

static const struct rfkill_ops bcm_bt_rfkill_ops = {
	.set_block = bcm_bt_rfkill_set_power,
};

static struct rfkill *bt_rfkill;

static int bcm_bt_probe(struct platform_device *pdev)
{
	int rc;

	PR("bcm_bt_probe\n");
	bcm_btlock_init();

	rc = gpio_request(GPIO_BTREG_ON, "bcm_bt_reg_on");
	if (unlikely(rc)) {
		pr_err("[BT] GPIO_BTREG_ON request failed.\n");
		goto err_gpio_btreg_on;
	}
	gpio_direction_output(GPIO_BTREG_ON, 0);

	bt_rfkill = rfkill_alloc("Broadcom Bluetooth", &pdev->dev,
				RFKILL_TYPE_BLUETOOTH, &bcm_bt_rfkill_ops,
				NULL);
	if (unlikely(!bt_rfkill)) {
		pr_err("[BT] bt_rfkill alloc failed.\n");
		rc =  -ENOMEM;
		goto err_rfkill_alloc;
	}

	rfkill_init_sw_state(bt_rfkill, false);
	rc = rfkill_register(bt_rfkill);
	if (unlikely(rc)) {
		pr_err("[BT] bt_rfkill register failed.\n");
		rc = -1;
		goto err_rfkill_register;
	}
	rfkill_set_sw_state(bt_rfkill, true);

	return rc;


err_rfkill_register:
	rfkill_destroy(bt_rfkill);
err_rfkill_alloc:
	gpio_free(GPIO_BTREG_ON);
err_gpio_btreg_on:
	return rc;
}

static int bcm_bt_remove(struct platform_device *pdev)
{
	rfkill_unregister(bt_rfkill);
	rfkill_destroy(bt_rfkill);

	gpio_free(GPIO_BTREG_ON);

	bcm_btlock_exit();

	return 0;
}
static struct platform_driver bcm_bt_platform_driver = {
	.probe = bcm_bt_probe,
	.remove = bcm_bt_remove,
	.driver = {
		   .name = BCM_BT_NAME,
		   .owner = THIS_MODULE,
	},
};

static int __init bcm_bt_init(void)
{
	return platform_driver_register(&bcm_bt_platform_driver);
}

static void __exit bcm_bt_exit(void)
{
	platform_driver_unregister(&bcm_bt_platform_driver);
}
#endif

module_init(bcm_btlock_init);
module_exit(bcm_btlock_exit);

#if 0
/*
 * Broadcom Bluetooth platform device(s)
 */

static struct platform_device bcm_bt_platform_device = {
	.name		= BCM_BT_NAME,
	.id		= -1,
};

static struct platform_device *bcm_bt_devs[] __initdata = {
	&bcm_bt_platform_device,
};

void __init bcm_bt_devs_init(void)
{
	platform_add_devices(bcm_bt_devs, ARRAY_SIZE(bcm_bt_devs));
}
#endif
