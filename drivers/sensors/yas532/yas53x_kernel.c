/*
 * Copyright (c) 2013 Yamaha Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/of_gpio.h>

#include "yas53x_drv.c"
#include "../sensors_core.h"

#define MODULE_NAME			"magnetic_sensor"
#define VENDOR_NAME			"YAMAHA"
#define CHIP_NAME			"yas532"

#define YAS53X_MIN_DELAY		(200) /* msec */
#define YAS53X_LOG_TIME			10000

struct yas53x_data {
	struct i2c_client *client;
	struct input_dev *input;
	struct device *factory_device;
	struct delayed_work work;
	struct mutex mutex, enable_mutex;
	int8_t hard_offset[3];
	int32_t delay;
	atomic_t enable;
	int count_logtime;
};

static struct i2c_client *this_client;

static int yas53x_i2c_open(void)
{
	pr_info("[SENSOR] %s\n", __func__);
	return 0;
}

static int yas53x_i2c_close(void)
{
	pr_info("[SENSOR] %s\n", __func__);
	return 0;
}

static int yas53x_i2c_write(uint8_t addr, const uint8_t *buf, int len)
{
	uint8_t tmp[16];
	if (sizeof(tmp) - 1 < len)
		return -1;
	tmp[0] = addr;
	memcpy(&tmp[1], buf, len);
	if (i2c_master_send(this_client, tmp, len + 1) < 0)
		return -1;
	return 0;
}

static int yas53x_i2c_read(uint8_t addr, uint8_t *buf, int len)
{
	struct i2c_msg msg[2];
	int err;
	msg[0].addr = this_client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &addr;
	msg[1].addr = this_client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = buf;
	err = i2c_transfer(this_client->adapter, msg, 2);
	if (err != 2) {
		dev_err(&this_client->dev,
				"i2c_transfer() read error: "
				"slave_addr=%02x, reg_addr=%02x, err=%d\n",
				this_client->addr, addr, err);
		return err;
	}
	return 0;
}

static void yas53x_msleep(int ms)
{
	usleep_range(ms * 1000, (ms + 1) * 1000);
}

static void yas53x_current_time(uint32_t *msec)
{
	*msec = jiffies_to_msecs(jiffies);
}

static struct yas_mag_driver this_drv = {
	.callback = {
		.device_open	= yas53x_i2c_open,
		.device_close	= yas53x_i2c_close,
		.device_write	= yas53x_i2c_write,
		.device_read	= yas53x_i2c_read,
		.msleep		= yas53x_msleep,
		.current_time	= yas53x_current_time,
	},
};

static void yas53x_input_work_func(struct work_struct *work)
{
	struct yas53x_data *data
		= container_of((struct delayed_work *)work,
			struct yas53x_data, work);
	struct yas_mag_data mag;
	int32_t delay;
	uint32_t time_before, time_after;
	int rt = 0;

	mutex_lock(&data->mutex);
	yas53x_current_time(&time_before);
	rt = this_drv.measure(&mag);
	if (rt & YAS_REPORT_HARD_OFFSET_CHANGED)
		this_drv.get_offset(data->hard_offset);
	yas53x_current_time(&time_after);
	delay = data->delay - (time_after - time_before);
	mutex_unlock(&data->mutex);

	/* report magnetic data in [nT] */
	input_report_rel(data->input, REL_X, mag.xyz.v[0]);
	input_report_rel(data->input, REL_Y, mag.xyz.v[1]);
	input_report_rel(data->input, REL_Z, mag.xyz.v[2]);
	input_sync(data->input);

	if (data->delay * data->count_logtime > YAS53X_LOG_TIME) {
		pr_info("[SENSOR] %s, %d, %d, %d\n",
			__func__, mag.xyz.v[0], mag.xyz.v[1], mag.xyz.v[2]);
		data->count_logtime = 0;
	} else
		data->count_logtime++;
	if (delay <= 0)
		delay = 1;
	schedule_delayed_work(&data->work, msecs_to_jiffies(delay));
}

static int yas53x_enable(struct yas53x_data *data)
{
	pr_info("[SENSOR] %s\n", __func__);
	if (!atomic_cmpxchg(&data->enable, 0, 1))
		schedule_delayed_work(&data->work, 0);
	return 0;
}

static int yas53x_disable(struct yas53x_data *data)
{
	pr_info("[SENSOR] %s\n", __func__);
	if (atomic_cmpxchg(&data->enable, 1, 0))
		cancel_delayed_work_sync(&data->work);
	return 0;
}

/* Input sysfs interface */
static ssize_t yas53x_delay_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct yas53x_data *data = dev_get_drvdata(dev);
	int32_t delay;
	mutex_lock(&data->mutex);
	delay = data->delay;
	mutex_unlock(&data->mutex);
	pr_info("[SENSOR] %s, delay = %d\n", __func__, delay);
	return sprintf(buf, "%d\n", delay);
}

static ssize_t yas53x_delay_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct yas53x_data *data = dev_get_drvdata(dev);
	long value;
	if (kstrtol(buf, 10, &value) < 0)
		return -EINVAL;
	value /= 1000000;
	if (value < 0)
		value = 0;
	if (YAS53X_MIN_DELAY < value)
		value = YAS53X_MIN_DELAY;
	mutex_lock(&data->mutex);
	data->delay = value;
	mutex_unlock(&data->mutex);
	pr_info("[SENSOR] %s, delay = %d\n", __func__, data->delay);
	return count;
}

static ssize_t yas53x_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct yas53x_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", atomic_read(&data->enable));
}

static ssize_t yas53x_enable_store(struct device *dev, 
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct yas53x_data *data = dev_get_drvdata(dev);
	int value;
	if (kstrtoint(buf, 10, &value) < 0)
		return -EINVAL;
	pr_info("[SENSOR] %s: enable = %d\n", __func__, value);
	if (value)
		yas53x_enable(data);
	else
		yas53x_disable(data);
	return count;
}

static DEVICE_ATTR(poll_delay, S_IRUGO|S_IWUSR|S_IWGRP,
	yas53x_delay_show, yas53x_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
	yas53x_enable_show, yas53x_enable_store);

static struct attribute *yas53x_attributes[] = {
	&dev_attr_poll_delay.attr,
	&dev_attr_enable.attr,
	NULL
};
static struct attribute_group yas53x_attribute_group = {
	.attrs = yas53x_attributes
};

/* sensors sysfs interface */
static ssize_t yas53x_self_test_show(struct device *dev,
	struct device_attribute *attr,	char *buf)
{
	struct yas53x_data *data = dev_get_drvdata(dev);
	struct yas_self_test_result r;
	int rt;
	s8 err[7] = { 0, };

	mutex_lock(&data->mutex);
	rt = this_drv.self_test(&r);
	mutex_unlock(&data->mutex);

	if (unlikely(r.id != 0x2))
		err[0] = -1;
	if (unlikely(r.xy1y2[0] < -30 || r.xy1y2[0] > 30))
		err[3] = -1;
	if (unlikely(r.xy1y2[1] < -30 || r.xy1y2[1] > 30))
		err[3] = -1;
	if (unlikely(r.xy1y2[2] < -30 || r.xy1y2[2] > 30))
		err[3] = -1;
	if (unlikely(r.sx < 17 || r.sy < 22))
		err[5] = -1;
	if (unlikely(r.xyz[0] < -600 || r.xyz[0] > 600))
		err[6] = -1;
	if (unlikely(r.xyz[1] < -600 || r.xyz[1] > 600))
		err[6] = -1;
	if (unlikely(r.xyz[2] < -600 || r.xyz[2] > 600))
		err[6] = -1;

	pr_info("[SENSOR] %s\n"
		"[SENSOR] Test1 - err = %d, id = %d\n"
		"[SENSOR] Test3 - err = %d\n"
		"[SENSOR] Test4 - err = %d, offset = %d,%d,%d\n"
		"[SENSOR] Test5 - err = %d, direction = %d\n"
		"[SENSOR] Test6 - err = %d, sensitivity = %d,%d\n"
		"[SENSOR] Test7 - err = %d, offset = %d,%d,%d\n"
		"[SENSOR] Test2 - err = %d\n", __func__,
		err[0], r.id, err[2], err[3], r.xy1y2[0], r.xy1y2[1],
		r.xy1y2[2], err[4], r.dir, err[5], r.sx, r.sy, 
		err[6], r.xyz[0], r.xyz[1], r.xyz[2], err[1]);

	return sprintf(buf,
			"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			err[0], r.id, err[2], err[3], r.xy1y2[0],
			r.xy1y2[1], r.xy1y2[2], err[4], r.dir,
			err[5], r.sx, r.sy, err[6],
			r.xyz[0], r.xyz[1], r.xyz[2], err[1]);
}

static ssize_t yas53x_self_test_noise_show(struct device *dev,
	struct device_attribute *attr,	char *buf)
{
	struct yas53x_data *data = dev_get_drvdata(dev);
	struct yas_vector raw_xyz;
	int rt;
	mutex_lock(&data->mutex);
	rt = this_drv.self_test_noise(&raw_xyz);
	mutex_unlock(&data->mutex);

	return sprintf(buf, "%d,%d,%d\n", raw_xyz.v[0], raw_xyz.v[1],
			raw_xyz.v[2]);
}

static ssize_t yas53x_self_test_noise_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	pr_info("[SENSOR] %s\n", __func__);

	return size;
}
static ssize_t yas53x_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t yas53x_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_NAME);
}

static DEVICE_ATTR(name, S_IRUGO, yas53x_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, yas53x_vendor_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO | S_IWUSR | S_IWGRP,
	yas53x_self_test_noise_show, yas53x_self_test_noise_store);
static DEVICE_ATTR(selftest, S_IRUGO, yas53x_self_test_show, NULL);

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_raw_data,
	&dev_attr_selftest,
	NULL,
};

static int yas53x_input_init(struct yas53x_data *data)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_X);
	input_set_capability(dev, EV_REL, REL_Y);
	input_set_capability(dev, EV_REL, REL_Z);
	input_set_drvdata(dev, data);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		return ret;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0) {
		input_unregister_device(dev);
		return ret;
	}

	/* sysfs node creation */
	ret = sysfs_create_group(&dev->dev.kobj, &yas53x_attribute_group);
	if (ret < 0) {
		sensors_remove_symlink(&data->input->dev.kobj,
			data->input->name);
		input_unregister_device(dev);
		return ret;
	}

	data->input = dev;
	pr_info("[SENSOR] %s\n", __func__);
	return 0;
}

static int yas53x_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct yas53x_data *data = NULL;
	int ret = -ENODEV;

	pr_info("[SENSOR]: %s - Probe Start!\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[SENSOR]: %s - i2c_check_functionality error\n",
			__func__);
		goto exit;
	}

	data = kzalloc(sizeof(struct yas53x_data), GFP_KERNEL);
	if (data == NULL) {
		pr_err("[SENSOR]: %s - kzalloc error\n", __func__);
		ret = -ENOMEM;
		goto exit_kzalloc;
	}

	i2c_set_clientdata(client, data);
	data->client = client;

	ret = yas53x_input_init(data);
	if (ret < 0)
		goto exit_input_init;

	sensors_register(data->factory_device, data, sensor_attrs, MODULE_NAME);

	/* workqueue init */
	INIT_DELAYED_WORK(&data->work, yas53x_input_work_func);
	mutex_init(&data->mutex);
	mutex_init(&data->enable_mutex);

	atomic_set(&data->enable, 0);
	data->delay = YAS53X_MIN_DELAY;
	this_client = client;

	ret = yas_mag_driver_init(&this_drv);
	if (ret < 0) {
		printk(KERN_ERR "yas_mag_driver_init failed[%d]\n", ret);
		goto err;
	}
	ret = this_drv.init();
	if (ret < 0) {
		printk(KERN_ERR "this_drv.init() failed[%d]\n", ret);
		goto err;
	}
	this_drv.set_position(CONFIG_INPUT_YAS_MAGNETOMETER_POSITION);
	this_drv.get_offset(data->hard_offset);

	pr_info("[SENSOR]: %s - Probe done!\n",	__func__);
	return 0;

err:
	if (data != NULL) {
		if (data->input != NULL) {
			mutex_destroy(&data->mutex);
			mutex_destroy(&data->enable_mutex);
			sensors_unregister(data->factory_device, sensor_attrs);
			sysfs_remove_group(&data->input->dev.kobj,
			   &yas53x_attribute_group);
			sensors_remove_symlink(&data->input->dev.kobj,
				data->input->name);
			input_unregister_device(data->input);
		}
		kfree(data);
	}
	return ret;
exit_input_init:
	kfree(data);
exit_kzalloc:
exit:
	pr_err("[SENSOR]: %s - Probe fail!\n", __func__);
	return ret;
}

static int yas53x_remove(struct i2c_client *client)
{
	struct yas53x_data *data = i2c_get_clientdata(client);
	if (data != NULL) {
		yas53x_disable(data);
		this_drv.term();
		sysfs_remove_group(&data->input->dev.kobj,
				&yas53x_attribute_group);
		input_unregister_device(data->input);
		kfree(data);
	}
	pr_info("[SENSOR] %s\n", __func__);
	return 0;
}

static int yas53x_suspend(struct device *dev)
{
	struct yas53x_data *data = dev_get_drvdata(dev);
	if (atomic_read(&data->enable))
		cancel_delayed_work_sync(&data->work);
	pr_info("[SENSOR] %s\n", __func__);
	return 0;
}

static int yas53x_resume(struct device *dev)
{
	struct yas53x_data *data = dev_get_drvdata(dev);
	if (atomic_read(&data->enable))
		schedule_delayed_work(&data->work, 0);
	pr_info("[SENSOR] %s\n", __func__);
	return 0;
}

static struct of_device_id yas53x_match_table[] = {
	{ .compatible = "yamaha,yas532",},
	{},
};

/*
static struct i2c_device_id yas53x_id[] = {
	{ "yas53x_match_table", 0},
	{}
};
*/

static struct i2c_device_id yas53x_id[] = {
	{"yas532", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, yas53x_id);


static const struct dev_pm_ops yas53x_pm_ops = {
	.suspend = yas53x_suspend,
	.resume = yas53x_resume
};

static struct i2c_driver yas53x_i2c_driver = {
	.driver = {
		.name	= CHIP_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = yas53x_match_table,
		.pm = &yas53x_pm_ops
	},

	.id_table	= yas53x_id,
	.probe		= yas53x_probe,
	.remove		= yas53x_remove,
};

static int __init yas53x_init(void)
{
	pr_info("[SENSOR] %s\n", __func__);
	return i2c_add_driver(&yas53x_i2c_driver);
}

static void __exit yas53x_term(void)
{
	i2c_del_driver(&yas53x_i2c_driver);
}

module_init(yas53x_init);
module_exit(yas53x_term);

MODULE_AUTHOR("Yamaha Corporation");
MODULE_DESCRIPTION("YAS53x Geomagnetic Sensor Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(YAS_VERSION);
