/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>

#include "sensors_core.h"

#define VENDOR_NAME	"CAPELLA"
#define MODEL_NAME	"CM3323"
#define MODULE_NAME	"light_sensor"

#define I2C_M_WR	0 /* for i2c Write */
#define I2c_M_RD	1 /* for i2c Read */

#define REL_RED         REL_HWHEEL
#define REL_GREEN       REL_DIAL
#define REL_BLUE        REL_WHEEL
#define REL_WHITE       REL_MISC

/* register addresses */
/* Ambient light sensor */
#define REG_CS_CONF1	0x00
#define REG_RED		0x08
#define REG_GREEN	0x09
#define REG_BLUE	0x0A
#define REG_WHITE	0x0B

#define LIGHT_LOG_TIME	15 /* 15 sec */
#define ALS_REG_NUM	2

enum {
	LIGHT_ENABLED = BIT(0),
	PROXIMITY_ENABLED = BIT(1),
};

/* register settings */
static const u8 als_reg_setting[ALS_REG_NUM][2] = {
	{REG_CS_CONF1, 0x00},	/* enable */
	{REG_CS_CONF1, 0x01},	/* disable */
};

#define CM3323_DEFAULT_DELAY		200000000LL

/* driver data */
struct cm3323_p {
	struct i2c_client *i2c_client;
	struct input_dev *input;
	struct device *light_dev;
	struct delayed_work work;
	atomic_t delay;

	u8 power_state;
	u16 color[4];
	int irq;
	int time_count;
#ifdef CONFIG_SEC_RUBENS_PROJECT
	struct regulator *vdd_2p85;
#endif
	u64 timestamp;
};

#ifdef CONFIG_SEC_RUBENS_PROJECT
static void sensor_power_on_vdd(struct cm3323_p *info, int onoff)
{
	int ret;
	if (info->vdd_2p85 == NULL) {
		info->vdd_2p85 =regulator_get(&info->i2c_client->dev, "8226_l19");
		if (IS_ERR(info->vdd_2p85)){
			pr_err("%s: regulator_get failed for 8226_l19\n", __func__);
			return ;
		}
		ret = regulator_set_voltage(info->vdd_2p85, 2850000, 2850000);
		if (ret)
			pr_err("%s: error vsensor_2p85 setting voltage ret=%d\n",__func__, ret);
	}
	if (onoff == 1) {
		ret = regulator_enable(info->vdd_2p85);
		if (ret)
			pr_err("%s: error enablinig regulator info->vdd_2p85\n", __func__);
	}
    else if (onoff == 0) {
		if (regulator_is_enabled(info->vdd_2p85)) {
			ret = regulator_disable(info->vdd_2p85);
			if (ret)
				pr_err("%s: error vdd_2p85 disabling regulator\n",__func__);
		}
	}
	msleep(30);
	return;
}
#endif



static int cm3323_i2c_read_word(struct cm3323_p *data,
		unsigned char reg_addr, u16 *buf)
{
	int ret;
	struct i2c_msg msg[2];
	unsigned char r_buf[2];

	msg[0].addr = data->i2c_client->addr;
	msg[0].flags = I2C_M_WR;
	msg[0].len = 1;
	msg[0].buf = &reg_addr;

	msg[1].addr = data->i2c_client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = r_buf;

	ret = i2c_transfer(data->i2c_client->adapter, msg, 2);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - i2c read error %d\n", __func__, ret);

		return ret;
	}

	*buf = (u16)r_buf[1];
	*buf = (*buf << 8) | (u16)r_buf[0];

	return 0;
}

static int cm3323_i2c_write(struct cm3323_p *data,
		unsigned char reg_addr, unsigned char buf)
{
	int ret = 0;
	struct i2c_msg msg;
	unsigned char w_buf[2];

	w_buf[0] = reg_addr;
	w_buf[1] = buf;

	/* send slave address & command */
	msg.addr = data->i2c_client->addr;
	msg.flags = I2C_M_WR;
	msg.len = 2;
	msg.buf = (char *)w_buf;

	ret = i2c_transfer(data->i2c_client->adapter, &msg, 1);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - i2c write error %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static void cm3323_light_enable(struct cm3323_p *data)
{
	cm3323_i2c_write(data, REG_CS_CONF1, als_reg_setting[0][1]);
	schedule_delayed_work(&data->work,
		nsecs_to_jiffies(atomic_read(&data->delay)));
}

static void cm3323_light_disable(struct cm3323_p *data)
{
	/* disable setting */
	cm3323_i2c_write(data, REG_CS_CONF1, als_reg_setting[1][1]);
	cancel_delayed_work_sync(&data->work);
}

static void cm3323_work_func_light(struct work_struct *work)
{
	struct timespec ts;
	int time_hi, time_lo;
	struct cm3323_p *data = container_of((struct delayed_work *)work,
			struct cm3323_p, work);
	unsigned long delay = nsecs_to_jiffies(atomic_read(&data->delay));

	ts = ktime_to_timespec(ktime_get_boottime());
	data->timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	time_lo = (int)(data->timestamp & TIME_LO_MASK);
	time_hi = (int)((data->timestamp & TIME_HI_MASK) >> TIME_HI_SHIFT);

	cm3323_i2c_read_word(data, REG_RED, &data->color[0]);
	cm3323_i2c_read_word(data, REG_GREEN, &data->color[1]);
	cm3323_i2c_read_word(data, REG_BLUE, &data->color[2]);
	cm3323_i2c_read_word(data, REG_WHITE, &data->color[3]);

	input_report_rel(data->input, REL_RED, data->color[0] + 1);
	input_report_rel(data->input, REL_GREEN, data->color[1] + 1);
	input_report_rel(data->input, REL_BLUE, data->color[2] + 1);
	input_report_rel(data->input, REL_WHITE, data->color[3] + 1);
	input_report_rel(data->input, REL_X, time_hi);
	input_report_rel(data->input, REL_Y, time_lo);
	input_sync(data->input);

	if (((int64_t)atomic_read(&data->delay) * (int64_t)data->time_count)
		>= ((int64_t)LIGHT_LOG_TIME * NSEC_PER_SEC)) {
		pr_info("[SENSOR]: %s - r = %u g = %u b = %u w = %u\n",
			__func__, data->color[0], data->color[1],
			data->color[2], data->color[3]);
		data->time_count = 0;
	} else {
		data->time_count++;
	}

	schedule_delayed_work(&data->work, delay);
}

/* sysfs */
static ssize_t cm3323_poll_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cm3323_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&data->delay));
}

static ssize_t cm3323_poll_delay_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm3323_p *data = dev_get_drvdata(dev);
	int64_t new_delay;
	int ret;

	ret = kstrtoll(buf, 10, &new_delay);
	if (ret) {
		pr_err("[SENSOR]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	atomic_set(&data->delay, new_delay);
	pr_info("[SENSOR]: %s - poll_delay = %lld\n", __func__, new_delay);

	return size;
}

static ssize_t light_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u8 enable;
	int ret;
	struct cm3323_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &enable);
	if (ret) {
		pr_err("[SENSOR]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	pr_info("[SENSOR]: %s - new_value = %u\n", __func__, enable);
	if (enable && !(data->power_state & LIGHT_ENABLED)) {
		data->power_state |= LIGHT_ENABLED;
		cm3323_light_enable(data);
	} else if (!enable && (data->power_state & LIGHT_ENABLED)) {
		cm3323_light_disable(data);
		data->power_state &= ~LIGHT_ENABLED;
	}

	return size;
}

static ssize_t light_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cm3323_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n",
		(data->power_state & LIGHT_ENABLED) ? 1 : 0);
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		cm3323_poll_delay_show, cm3323_poll_delay_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		light_enable_show, light_enable_store);

static struct attribute *light_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL,
};

static struct attribute_group light_attribute_group = {
	.attrs = light_sysfs_attrs,
};

/* light sysfs */
static ssize_t cm3323_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", MODEL_NAME);
}

static ssize_t cm3323_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t light_lux_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm3323_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u,%u,%u,%u\n",
			data->color[0], data->color[1],
			data->color[2], data->color[3]);
}

static ssize_t light_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm3323_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u,%u,%u,%u\n",
			data->color[0], data->color[1],
			data->color[2], data->color[3]);
}

static DEVICE_ATTR(name, S_IRUGO, cm3323_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, cm3323_vendor_show, NULL);
static DEVICE_ATTR(lux, S_IRUGO, light_lux_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO, light_data_show, NULL);

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_lux,
	&dev_attr_raw_data,
	NULL,
};

static int cm3323_setup_reg(struct cm3323_p *data)
{
	int ret = 0;

	/* ALS initialization */
	ret = cm3323_i2c_write(data,
			als_reg_setting[0][0],
			als_reg_setting[0][1]);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - cm3323_als_reg is failed. %d\n",
			__func__, ret);
		return ret;
	}

	/* turn off */
	cm3323_i2c_write(data, REG_CS_CONF1, 0x01);
	return ret;
}

static int cm3323_input_init(struct cm3323_p *data)
{
	int ret = 0;
	struct input_dev *dev;

	/* allocate lightsensor input_device */
	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_RED);
	input_set_capability(dev, EV_REL, REL_GREEN);
	input_set_capability(dev, EV_REL, REL_BLUE);
	input_set_capability(dev, EV_REL, REL_WHITE);
	input_set_capability(dev, EV_REL, REL_X);
	input_set_capability(dev, EV_REL, REL_Y);
	input_set_drvdata(dev, data);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(data->input);
		return ret;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0) {
		input_unregister_device(dev);
		return ret;
	}

	ret = sysfs_create_group(&dev->dev.kobj, &light_attribute_group);
	if (ret < 0) {
		sensors_remove_symlink(&data->input->dev.kobj,
			data->input->name);
		input_unregister_device(dev);
		return ret;
	}

	data->input = dev;
	return 0;
}

static int cm3323_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct cm3323_p *data = NULL;

	pr_info("[SENSOR]: %s - Probe Start!\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[SENSOR]: %s - i2c_check_functionality error\n",
			__func__);
		goto exit;
	}

	data = kzalloc(sizeof(struct cm3323_p), GFP_KERNEL);
	if (data == NULL) {
		pr_err("[SENSOR]: %s - kzalloc error\n", __func__);
		ret = -ENOMEM;
		goto exit_kzalloc;
	}

	data->i2c_client = client;
	i2c_set_clientdata(client, data);
#ifdef CONFIG_SEC_RUBENS_PROJECT
	sensor_power_on_vdd(data,1);
#endif
	/* Check if the device is there or not. */
	ret = cm3323_setup_reg(data);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - could not setup regs\n", __func__);
		goto exit_setup_reg;
	}

	/* input device init */
	ret = cm3323_input_init(data);
	if (ret < 0)
		goto exit_input_init;

	atomic_set(&data->delay, CM3323_DEFAULT_DELAY);
	data->time_count = 0;

	INIT_DELAYED_WORK(&data->work, cm3323_work_func_light);

	/* set sysfs for light sensor */
	sensors_register(data->light_dev, data, sensor_attrs, MODULE_NAME);
	pr_info("[SENSOR]: %s - Probe done!\n", __func__);

	return 0;

exit_input_init:
exit_setup_reg:
	kfree(data);
#ifdef CONFIG_SEC_RUBENS_PROJECT
	sensor_power_on_vdd(data,0);
#endif
exit_kzalloc:
exit:
	pr_err("[SENSOR]: %s - Probe fail!\n", __func__);
	return ret;
}

static void cm3323_shutdown(struct i2c_client *client)
{
	struct cm3323_p *data = i2c_get_clientdata(client);

	pr_info("[SENSOR]: %s\n", __func__);
	if (data->power_state & LIGHT_ENABLED)
		cm3323_light_disable(data);
}

static int __devexit cm3323_remove(struct i2c_client *client)
{
	struct cm3323_p *data = i2c_get_clientdata(client);

	/* device off */
	if (data->power_state & LIGHT_ENABLED)
		cm3323_light_disable(data);

	/* sysfs destroy */
	sensors_unregister(data->light_dev, sensor_attrs);
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);

	/* input device destroy */
	sysfs_remove_group(&data->input->dev.kobj, &light_attribute_group);
	input_unregister_device(data->input);
	kfree(data);

	return 0;
}

static int cm3323_suspend(struct device *dev)
{
	struct cm3323_p *data = dev_get_drvdata(dev);

	if (data->power_state & LIGHT_ENABLED) {
		pr_info("[SENSOR]: %s\n", __func__);
		cm3323_light_disable(data);
	}

	return 0;
}

static int cm3323_resume(struct device *dev)
{
	struct cm3323_p *data = dev_get_drvdata(dev);

	if (data->power_state & LIGHT_ENABLED) {
		pr_info("[SENSOR]: %s\n", __func__);
		cm3323_light_enable(data);
	}

	return 0;
}

static struct of_device_id cm3323_match_table[] = {
	{ .compatible = "cm3323-i2c",},
	{},
};

static const struct i2c_device_id cm3323_device_id[] = {
	{ "cm3323_match_table", 0 },
	{ }
};

static const struct dev_pm_ops cm3323_pm_ops = {
	.suspend = cm3323_suspend,
	.resume = cm3323_resume
};

static struct i2c_driver cm3323_i2c_driver = {
	.driver = {
		.name = MODEL_NAME,
		.owner = THIS_MODULE,
		.of_match_table = cm3323_match_table,
		.pm = &cm3323_pm_ops
	},
	.probe = cm3323_probe,
	.shutdown = cm3323_shutdown,
	.remove = __devexit_p(cm3323_remove),
	.id_table = cm3323_device_id,
};

static int __init cm3323_init(void)
{
	return i2c_add_driver(&cm3323_i2c_driver);
}

static void __exit cm3323_exit(void)
{
	i2c_del_driver(&cm3323_i2c_driver);
}

module_init(cm3323_init);
module_exit(cm3323_exit);

MODULE_DESCRIPTION("RGB Sensor device driver for cm3323");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
