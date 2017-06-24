/* drivers/motor/isa1400_vibrator.c
 *
 * Copyright (C) 2011 Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/hrtimer.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>
#include <linux/pwm.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/isa1400_vibrator.h>

#include "../staging/android/timed_output.h"
#include "ss_vibrator.h"

#define MOTOR_DEBUG	0

static int vib_work_lock = 0;
static struct wake_lock vib_wake_lock;

struct isa1400_vibrator_drvdata {
	struct timed_output_dev dev;
	struct hrtimer timer;
	struct work_struct work;
	struct pwm_device	*pwm;
	struct i2c_client *client;
	struct isa1400_vibrator_platform_data *pdata;
	struct mutex		mutex_lock;
	bool running;
	int timeout;
	int intensity;
};


static int isa1400_vibrator_i2c_write(struct i2c_client *client,
		u8 addr, u8 val)
{
	int error = 0;
	error = i2c_smbus_write_byte_data(client, addr, val);
	if (error)
		printk(KERN_ERR "[VIB] Failed to write addr=[0x%x], val=[0x%x]\n",
				addr, val);

#if MOTOR_DEBUG
	printk(KERN_DEBUG "[VIB] %s : 0x%x, 0x%x\n",
			__func__, addr, val);
#endif

	return error;
}

static void isa1400_reg_write(struct isa1400_vibrator_drvdata *data,
		u8 type)
{
	int i = 0, j = 0;

	for (i  = 0; i < ISA1400_REG_MAX; i++) {
		if (data->pdata->reg_data[i][0] == type){
			for (j = 2; j < data->pdata->reg_data[i][1]; j += 2){
				/*printk("[VIB] inside loop %d %d %d\n", j,
				  data->pdata->reg_data[i][j],
				  data->pdata->reg_data[i][j + 1]);*/
				isa1400_vibrator_i2c_write(data->client,
						data->pdata->reg_data[i][j],
						data->pdata->reg_data[i][j + 1]);

			}
		}
	}
}

static void isa1400_vibrator_hw_init(struct isa1400_vibrator_drvdata *data)
{
	data->pdata->gpio_en(true);
	data->pdata->clk_en(true);
	isa1400_reg_write(data, ISA1400_REG_INIT);
}

static void isa1400_vibrator_on(struct isa1400_vibrator_drvdata *data)
{
	isa1400_vibrator_hw_init(data);
	isa1400_reg_write(data, ISA1400_REG_START);
}

static void isa1400_vibrator_off(struct isa1400_vibrator_drvdata *data)
{
	if(vib_work_lock)
		return;

	isa1400_reg_write(data, ISA1400_REG_STOP);
	data->pdata->clk_en(false);
	data->pdata->gpio_en(false);
}


static struct isa1400_vibrator_drvdata	*g_drvdata;
int isa1400_i2c_write(u8 addr, int length, u8 *data)
{
	if (NULL == g_drvdata) {
		printk(KERN_ERR "[VIB] driver is not ready\n");
		return -EFAULT;
	}

	if (2 != length)
		printk(KERN_ERR "[VIB] length should be 2(len:%d)\n",
				length);

#if MOTOR_DEBUG
	printk(KERN_DEBUG "[VIB] data : %x, %x\n",
			data[0], data[1]);
#endif

	return isa1400_vibrator_i2c_write(g_drvdata->client,
			data[0], data[1]);
}

void isa1400_clk_config(int duty)
{
	if (NULL == g_drvdata) {
		printk(KERN_ERR "[VIB] driver is not ready\n");
		return ;
	}

#if MOTOR_DEBUG
	printk(KERN_DEBUG "[VIB] %s duty %d\n", __func__, duty);
#endif
	duty = (duty > 0) ? duty : (abs(duty) | 0x80);

	isa1400_vibrator_i2c_write(g_drvdata->client,
			ISA1400_REG_GAIN, duty);
	isa1400_vibrator_i2c_write(g_drvdata->client,
			ISA1400_REG_HPTEN, 0x01);
}

void vibe_set_intensity(int intensity)
{
	if (!g_drvdata->running) {
		g_drvdata->pdata->gpio_en(true);
	}

	if (0 == intensity)
		isa1400_clk_config(intensity);
	else {
		if (MAX_INTENSITY == intensity)
			intensity = 127;
		else if (0 != intensity) {
			int tmp = intensity;
			intensity = (tmp / 79);	// 79 := 10000 / 127
		}
		isa1400_clk_config(intensity);
	}
}

static enum hrtimer_restart isa1400_vibrator_timer_func(struct hrtimer *_timer)
{
	struct isa1400_vibrator_drvdata *data =
		container_of(_timer, struct isa1400_vibrator_drvdata, timer);

	data->timeout = 0;

	schedule_work(&data->work);
	return HRTIMER_NORESTART;
}

static void isa1400_vibrator_work(struct work_struct *_work)
{
	struct isa1400_vibrator_drvdata *data =
		container_of(_work, struct isa1400_vibrator_drvdata, work);
	if (0 == data->timeout) {
		if (!data->running)
			return ;

		data->running = false;
		isa1400_vibrator_off(data);
	} else {
		if (data->running)
			return ;

		data->running = true;
		isa1400_vibrator_on(data);
		vibe_set_intensity(data->intensity);
	}
}

static int isa1400_vibrator_get_time(struct timed_output_dev *_dev)
{
	struct isa1400_vibrator_drvdata	*data =
		container_of(_dev, struct isa1400_vibrator_drvdata, dev);

	if (hrtimer_active(&data->timer)) {
		ktime_t r = hrtimer_get_remaining(&data->timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	} else
		return 0;
}

static void isa1400_vibrator_enable(struct timed_output_dev *_dev, int value)
{
	struct isa1400_vibrator_drvdata	*data =
		container_of(_dev, struct isa1400_vibrator_drvdata, dev);

	printk(KERN_DEBUG "[VIB] time = %dms\n", value);
	mutex_lock(&data->mutex_lock);
	cancel_work_sync(&data->work);
	hrtimer_cancel(&data->timer);
	data->timeout = value;
	if (value > 0) {
		if (value > data->pdata->max_timeout)
			value = data->pdata->max_timeout;

		hrtimer_start(&data->timer,
				ns_to_ktime((u64)value * NSEC_PER_MSEC),
				HRTIMER_MODE_REL);
	}
	mutex_unlock(&data->mutex_lock);
	schedule_work(&data->work);
}

static ssize_t intensity_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	int ret = 0, set_intensity = 0; 

	ret = kstrtoint(buf, 0, &set_intensity);

	if ((set_intensity < 0) || (set_intensity > MAX_INTENSITY)) {
		pr_err("[VIB]: %sout of rage\n", __func__);
		return -EINVAL;
	}

	vibe_set_intensity(set_intensity);
	g_drvdata->intensity = set_intensity;

	return count;
}

static ssize_t intensity_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "intensity: %u\n", g_drvdata->intensity);
}

static DEVICE_ATTR(intensity, 0660, intensity_show, intensity_store);

static int __devinit isa1400_vibrator_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct isa1400_vibrator_platform_data *pdata =
		client->dev.platform_data;
	struct isa1400_vibrator_drvdata *ddata;
	int ret = 0;

	ddata = kzalloc(sizeof(struct isa1400_vibrator_drvdata), GFP_KERNEL);
	if (NULL == ddata) {
		printk(KERN_ERR "[VIB] Failed to alloc memory.\n");
		ret = -ENOMEM;
		goto err_free_mem;
	}

	ddata->client = client;
	if (NULL == pdata) {
		printk(KERN_ERR "[VIB] the platform data is empty.\n");
		goto err_platform_data;
	} else
		ddata->pdata = pdata;

	ddata->dev.name = "vibrator";
	ddata->dev.get_time = isa1400_vibrator_get_time;
	ddata->dev.enable = isa1400_vibrator_enable;

	hrtimer_init(&ddata->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ddata->timer.function = isa1400_vibrator_timer_func;
	INIT_WORK(&ddata->work, isa1400_vibrator_work);
	mutex_init(&ddata->mutex_lock);

	ddata->intensity = MAX_INTENSITY;

	i2c_set_clientdata(client, ddata);
	/*ddata->pdata->gpio_en(true);
	  isa1400_vibrator_hw_init(ddata);*/
	wake_lock_init(&vib_wake_lock, WAKE_LOCK_SUSPEND, "vib_present");
	ret = timed_output_dev_register(&ddata->dev);
	if (ret < 0) {
		printk(KERN_ERR "[VIB] Failed to register timed_output : -%d\n", ret);
		goto err_to_dev_reg;
	}

	ret = sysfs_create_file(&ddata->dev.dev->kobj, &dev_attr_intensity.attr);
	if (ret < 0) {
		pr_err("[VIB]: Failed to register sysfs intensity: %d\n", ret);
	}

	g_drvdata = ddata;

	return 0;

err_to_dev_reg:
err_platform_data:
	kfree(ddata);
err_free_mem:
	return ret;

}

static int isa1400_vibrator_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct isa1400_vibrator_drvdata *ddata  = i2c_get_clientdata(client);
	printk(KERN_INFO "[VIB] isa1400_vibrator_suspend called ");

	if(vib_work_lock){
		vib_work_lock = 0;
		ddata->pdata->gpio_en(false);
		ddata->pdata->clk_en(false);
	}

	return 0;
}

static int isa1400_vibrator_resume(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id isa1400_vibrator_device_id[] = {
	{"isa1400_vibrator", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, isa1400_vibrator_device_id);

static struct i2c_driver isa1400_vibrator_i2c_driver = {
	.driver = {
		.name = "isa1400_vibrator",
		.owner = THIS_MODULE,
	},
	.probe		= isa1400_vibrator_i2c_probe,
	.id_table	= isa1400_vibrator_device_id,
	.suspend	= isa1400_vibrator_suspend,
	.resume		= isa1400_vibrator_resume,
};

static int __init isa1400_vibrator_init(void)
{
	printk("[VIB] init called for 1400\n");
	return i2c_add_driver(&isa1400_vibrator_i2c_driver);
}

static void __exit isa1400_vibrator_exit(void)
{
	wake_lock_destroy(&vib_wake_lock);
	mutex_destroy(&g_drvdata->mutex_lock);
	timed_output_dev_unregister(&g_drvdata->dev);
	i2c_del_driver(&isa1400_vibrator_i2c_driver);
}

module_init(isa1400_vibrator_init);
module_exit(isa1400_vibrator_exit);

MODULE_AUTHOR("Samsung Corporation");
MODULE_DESCRIPTION("Vibrator driver for the isa1400 ic");
MODULE_LICENSE("GPL v2");

