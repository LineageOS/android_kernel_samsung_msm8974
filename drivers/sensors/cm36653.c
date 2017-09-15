/* driver/sensor/cm36653.c
 * Copyright (c) 2011 SAMSUNG
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

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include "sensors_core.h"

/* For debugging */
#undef	CM36653_DEBUG

#define	VENDOR		"CAPELLA"
#define	CHIP_ID		"CM36653"
#define MODULE_NAME_LIGHT	"light_sensor"
#define MODULE_NAME_PROX	"proximity_sensor"

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
#define REG_GREEN		0x09
#define REG_BLUE		0x0A
#define REG_WHITE		0x0B

/* Proximity sensor */
#define REG_PS_CONF1		0x03
#define REG_PS_THD		0x05
#define REG_PS_CANC		0x06
#define REG_PS_DATA		0x07

#define ALS_REG_NUM	2
#define PS_REG_NUM	3

#define MSK_L(x)	(x & 0xff)
#define MSK_H(x)	((x & 0xff00) >> 8)

/* Intelligent Cancelation*/
#define CM36653_CANCELATION
#ifdef CM36653_CANCELATION
#define CANCELATION_FILE_PATH	"/efs/prox_cal"
#define CANCELATION_THRESHOLD	0x0806
#endif

#define PROX_READ_NUM	40
 /*lightsnesor log time 6SEC 200mec X 30*/
#define LIGHT_LOG_TIME	30
#define LIGHT_ADD_STARTTIME 300000000
enum {
	LIGHT_ENABLED = BIT(0),
	PROXIMITY_ENABLED = BIT(1),
};

/* register settings */
static u8 als_reg_setting[ALS_REG_NUM][2] = {
	{REG_CS_CONF1, 0x00},	/* enable */
	{REG_CS_CONF1, 0x01},	/* disable */
};

/* Change threshold value on the midas-sensor.c */
enum {
	PS_CONF1 = 0,
	PS_THD,
	PS_CANCEL,
};
enum {
	REG_ADDR = 0,
	CMD,
};

static u16 ps_reg_init_setting[PS_REG_NUM][2] = {
	{REG_PS_CONF1, 0x430C},	/* REG_PS_CONF1 */
	{REG_PS_THD, 0x0A08},	/* REG_PS_THD */
	{REG_PS_CANC, 0x00},	/* REG_PS_CANC */
};

/* driver data */
struct cm36653_data {
	struct i2c_client *i2c_client;
	struct wake_lock prx_wake_lock;
	struct input_dev *proximity_input_dev;
	struct input_dev *light_input_dev;
	struct mutex power_lock;
	struct mutex read_lock;
	struct hrtimer light_timer;
	struct hrtimer prox_timer;
	struct workqueue_struct *light_wq;
	struct workqueue_struct *prox_wq;
	struct work_struct work_light;
	struct work_struct work_prox;
	struct device *proximity_dev;
	struct device *light_dev;
	ktime_t light_poll_delay;
	ktime_t prox_poll_delay;
	int irq;
	int prox_int;
	int prox_led_on;
	bool is_led_on;
	u32 threshold;
	u8 power_state;
	int avg[3];
	u16 color[4];
	int count_log_time;
#ifdef CM36653_CANCELATION
	u16 default_thresh;
#endif
};

int cm36653_i2c_read_byte(struct cm36653_data *cm36653,
	u8 command, u8 *val)
{
	int err = 0;
	int retry = 3;
	struct i2c_msg msg[1];
	struct i2c_client *client = cm36653->i2c_client;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	/* send slave address & command */
	msg->addr = client->addr;
	msg->flags = I2C_M_RD;
	msg->len = 1;
	msg->buf = val;

	while (retry--) {
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0)
			return err;
	}
	pr_err("[SENSOR] %s: i2c read failed at addr 0x%x: %d\n", __func__,
		client->addr, err);
	return err;
}

int cm36653_i2c_read_word(struct cm36653_data *cm36653, u8 command,
			  u16 *val)
{
	int err = 0;
	int retry = 3;
	struct i2c_client *client = cm36653->i2c_client;
	struct i2c_msg msg[2];
	unsigned char data[2] = {0,};
	u16 value = 0;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		/* send slave address & command */
		msg[0].addr = client->addr;
		msg[0].flags = I2C_M_WR;
		msg[0].len = 1;
		msg[0].buf = &command;

		/* read word data */
		msg[1].addr = client->addr;
		msg[1].flags = I2C_M_RD;
		msg[1].len = 2;
		msg[1].buf = data;

		err = i2c_transfer(client->adapter, msg, 2);

		if (err >= 0) {
			value = (u16)data[1];
			*val = (value << 8) | (u16)data[0];
			return err;
		}
	}
	printk(KERN_ERR "[SENSOR] %s, i2c transfer error ret=%d\n", __func__, err);
	return err;
}

int cm36653_i2c_write_byte(struct cm36653_data *cm36653, u8 command,
			   u8 val)
{
	int err = 0;
	struct i2c_client *client = cm36653->i2c_client;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int retry = 3;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		data[0] = command;
		data[1] = val;

		/* send slave address & command */
		msg->addr = client->addr;
		msg->flags = I2C_M_WR;
		msg->len = 2;
		msg->buf = data;

		err = i2c_transfer(client->adapter, msg, 1);

		if (err >= 0)
			return 0;
	}
	pr_err("[SENSOR] %s, i2c transfer error(%d)\n", __func__, err);
	return err;
}

int cm36653_i2c_write_word(struct cm36653_data *cm36653, u8 command,
			   u16 val)
{
	int err = 0;
	struct i2c_client *client = cm36653->i2c_client;
	int retry = 3;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		err = i2c_smbus_write_word_data(client, command, val);
		if (err >= 0)
			return 0;
	}
	pr_err("[SENSOR] %s, i2c transfer error(%d)\n", __func__, err);
	return err;
}

void prox_led_onoff(struct cm36653_data *cm36653, bool onoff)
{
	if (onoff) {
		gpio_set_value(cm36653->prox_led_on, 1);
		msleep(20);
	} else
		gpio_set_value(cm36653->prox_led_on, 0);
	cm36653->is_led_on = onoff;

	usleep_range(1000, 1100);
	pr_info("[SENSOR] %s: onoff = %s, led_on_gpio = %s\n",
		__func__, onoff ? "on" : "off",
		gpio_get_value(cm36653->prox_led_on) ? "high" : "low");
}

static void cm36653_light_enable(struct cm36653_data *cm36653)
{
	/* enable setting */
	cm36653_i2c_write_byte(cm36653, REG_CS_CONF1,
		als_reg_setting[0][1]);
	hrtimer_start(&cm36653->light_timer, cm36653->light_poll_delay,
		HRTIMER_MODE_REL);
}

static void cm36653_light_disable(struct cm36653_data *cm36653)
{
	/* disable setting */
	cm36653_i2c_write_byte(cm36653, REG_CS_CONF1,
		als_reg_setting[1][1]);
	hrtimer_cancel(&cm36653->light_timer);
	cancel_work_sync(&cm36653->work_light);
}

/* sysfs */
static ssize_t cm36653_poll_delay_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(cm36653->light_poll_delay));
}

static ssize_t cm36653_poll_delay_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	mutex_lock(&cm36653->power_lock);
	if (new_delay != ktime_to_ns(cm36653->light_poll_delay)) {
		cm36653->light_poll_delay = ns_to_ktime(new_delay);
		if (cm36653->power_state & LIGHT_ENABLED) {
			cm36653_light_disable(cm36653);
			cm36653_light_enable(cm36653);
		}
		pr_info("[SENSOR] %s, poll_delay = %lld\n", __func__, new_delay);
	}
	mutex_unlock(&cm36653->power_lock);

	return size;
}

static ssize_t light_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("[SENSOR] %s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&cm36653->power_lock);
	pr_info("[SENSOR] %s,new_value=%d\n", __func__, new_value);
	if (new_value && !(cm36653->power_state & LIGHT_ENABLED)) {
		cm36653->power_state |= LIGHT_ENABLED;
		cm36653_light_enable(cm36653);
	} else if (!new_value && (cm36653->power_state & LIGHT_ENABLED)) {
		cm36653_light_disable(cm36653);
		cm36653->power_state &= ~LIGHT_ENABLED;
	}
	mutex_unlock(&cm36653->power_lock);
	return size;
}

static ssize_t light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (cm36653->power_state & LIGHT_ENABLED) ? 1 : 0);
}

#ifdef CM36653_CANCELATION
static int proximity_open_cancelation(struct cm36653_data *data)
{
	struct file *cancel_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cancel_filp = filp_open(CANCELATION_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cancel_filp)) {
		err = PTR_ERR(cancel_filp);
		if (err != -ENOENT)
			pr_err("[SENSOR] %s: Can't open cancelation file\n", __func__);
		set_fs(old_fs);
		return err;
	}

	err = cancel_filp->f_op->read(cancel_filp,
		(char *)&ps_reg_init_setting[PS_CANCEL][CMD],
		sizeof(u8), &cancel_filp->f_pos);
	if (err != sizeof(u8)) {
		pr_err("[SENSOR] %s: Can't read the cancel data from file\n", __func__);
		err = -EIO;
	}

	/*If there is an offset cal data. */
	if (ps_reg_init_setting[PS_CANCEL][CMD] != 0)
		ps_reg_init_setting[PS_THD][CMD] = CANCELATION_THRESHOLD;

	pr_info("[SENSOR] %s: proximity ps_data = %d, ps_thresh = 0x%x\n",
		__func__, ps_reg_init_setting[PS_CANCEL][CMD],
		ps_reg_init_setting[PS_THD][CMD]);

	filp_close(cancel_filp, current->files);
	set_fs(old_fs);

	return err;
}

static int proximity_store_cancelation(struct device *dev, bool do_calib)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);
	struct file *cancel_filp = NULL;
	mm_segment_t old_fs;
	int err = 0;
	u16 ps_data = 0;

	if (do_calib) {
		mutex_lock(&cm36653->read_lock);
		cm36653_i2c_read_word(cm36653,
			REG_PS_DATA, &ps_data);
		ps_reg_init_setting[PS_CANCEL][CMD] = 0xff & ps_data;
		mutex_unlock(&cm36653->read_lock);

		ps_reg_init_setting[PS_THD][CMD] = CANCELATION_THRESHOLD;
	} else { /* reset */
		ps_reg_init_setting[PS_CANCEL][CMD] = 0;
		ps_reg_init_setting[PS_THD][CMD] = cm36653->default_thresh;
	}
	cm36653_i2c_write_word(cm36653, REG_PS_THD,
		ps_reg_init_setting[PS_THD][CMD]);
	cm36653_i2c_write_byte(cm36653, REG_PS_CANC,
		ps_reg_init_setting[PS_CANCEL][CMD]);
	pr_info("[SENSOR] %s: prox_cal = 0x%x, prox_thresh = 0x%x\n",
		__func__, ps_reg_init_setting[PS_CANCEL][CMD],
		ps_reg_init_setting[PS_THD][CMD]);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cancel_filp = filp_open(CANCELATION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0666);
	if (IS_ERR(cancel_filp)) {
		pr_err("[SENSOR] %s: Can't open cancelation file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cancel_filp);
		return err;
	}

	err = cancel_filp->f_op->write(cancel_filp,
		(char *)&ps_reg_init_setting[PS_CANCEL][CMD],
		sizeof(u8), &cancel_filp->f_pos);
	if (err != sizeof(u8)) {
		pr_err("[SENSOR] %s: Can't write the cancel data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cancel_filp, current->files);
	set_fs(old_fs);

	if (!do_calib) /* delay for clearing */
		msleep(150);

	return err;
}

static ssize_t proximity_cancel_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	bool do_calib;
	int err;

	if (sysfs_streq(buf, "1")) /* calibrate cancelation value */
		do_calib = true;
	else if (sysfs_streq(buf, "0")) /* reset cancelation value */
		do_calib = false;
	else {
		pr_debug("[SENSOR] %s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	err = proximity_store_cancelation(dev, do_calib);
	if (err < 0) {
		pr_err("[SENSOR] %s: proximity_store_cancelation() failed\n", __func__);
		return err;
	}

	return size;
}

static ssize_t proximity_cancel_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d,%d\n", ps_reg_init_setting[PS_CANCEL][CMD],
		ps_reg_init_setting[PS_THD][CMD] >> 8 & 0xff);
}
#endif

static ssize_t proximity_enable_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("[SENSOR] %s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&cm36653->power_lock);
	pr_info("[SENSOR] %s, new_value = %d, threshold = %d\n", __func__, new_value,
		ps_reg_init_setting[PS_THD][CMD] >> 8 & 0xff);
	if (new_value && !(cm36653->power_state & PROXIMITY_ENABLED)) {
		u8 val = 1;
		int i, err = 0;

		cm36653->power_state |= PROXIMITY_ENABLED;

		if (cm36653->prox_led_on)
			prox_led_onoff(cm36653, true);
#ifdef CM36653_CANCELATION
		/* open cancelation data */
		err = proximity_open_cancelation(cm36653);
		if (err < 0 && err != -ENOENT)
			pr_err("[SENSOR] %s: proximity_open_cancelation() failed\n",
				__func__);
#endif
		/* enable settings */
		for (i = 0; i < PS_REG_NUM; i++) {
			cm36653_i2c_write_word(cm36653,
				ps_reg_init_setting[i][REG_ADDR],
				ps_reg_init_setting[i][CMD]);
		}

		val = gpio_get_value(cm36653->prox_int);
		/* 0 is close, 1 is far */
		input_report_abs(cm36653->proximity_input_dev,
			ABS_DISTANCE, val);
		input_sync(cm36653->proximity_input_dev);

		enable_irq(cm36653->irq);
		enable_irq_wake(cm36653->irq);
	} else if (!new_value && (cm36653->power_state & PROXIMITY_ENABLED)) {
		cm36653->power_state &= ~PROXIMITY_ENABLED;

		disable_irq_wake(cm36653->irq);
		disable_irq(cm36653->irq);

		/* disable settings */
		cm36653_i2c_write_word(cm36653, REG_PS_CONF1, 0x0001);

		if (cm36653->prox_led_on)
			prox_led_onoff(cm36653, false);
	}
	mutex_unlock(&cm36653->power_lock);
	return size;
}

static ssize_t proximity_enable_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n",
		       (cm36653->power_state & PROXIMITY_ENABLED) ? 1 : 0);
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   cm36653_poll_delay_show, cm36653_poll_delay_store);

static struct device_attribute dev_attr_light_enable =
__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	light_enable_show, light_enable_store);

static struct device_attribute dev_attr_proximity_enable =
__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_enable_show, proximity_enable_store);

static struct attribute *light_sysfs_attrs[] = {
	&dev_attr_light_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group light_attribute_group = {
	.attrs = light_sysfs_attrs,
};

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_proximity_enable.attr,
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};

/* sysfs for vendor & name */
static ssize_t cm36653_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t cm36653_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}
static struct device_attribute dev_attr_prox_sensor_vendor =
	__ATTR(vendor, S_IRUSR | S_IRGRP, cm36653_vendor_show, NULL);
static struct device_attribute dev_attr_light_sensor_vendor =
	__ATTR(vendor, S_IRUSR | S_IRGRP, cm36653_vendor_show, NULL);
static struct device_attribute dev_attr_prox_sensor_name =
	__ATTR(name, S_IRUSR | S_IRGRP, cm36653_name_show, NULL);
static struct device_attribute dev_attr_light_sensor_name =
	__ATTR(name, S_IRUSR | S_IRGRP, cm36653_name_show, NULL);

/* proximity sysfs */
static ssize_t proximity_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);

	return sprintf(buf, "%d,%d,%d\n", cm36653->avg[0],
		cm36653->avg[1], cm36653->avg[2]);
}

static ssize_t proximity_avg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);
	bool new_value = false;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("[SENSOR] %s, invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	pr_info("[SENSOR] %s, average enable = %d\n", __func__, new_value);
	mutex_lock(&cm36653->power_lock);
	if (new_value) {
		if (!(cm36653->power_state & PROXIMITY_ENABLED)) {
			if (cm36653->prox_led_on)
				prox_led_onoff(cm36653, true);

			cm36653_i2c_write_word(cm36653, REG_PS_CONF1,
				ps_reg_init_setting[PS_CONF1][CMD]);
		}
		hrtimer_start(&cm36653->prox_timer, cm36653->prox_poll_delay,
			HRTIMER_MODE_REL);
	} else if (!new_value) {
		hrtimer_cancel(&cm36653->prox_timer);
		cancel_work_sync(&cm36653->work_prox);
		if (!(cm36653->power_state & PROXIMITY_ENABLED)) {
			cm36653_i2c_write_word(cm36653, REG_PS_CONF1,
				0x0001);
			if (cm36653->prox_led_on)
				prox_led_onoff(cm36653, false);
		}
	}
	mutex_unlock(&cm36653->power_lock);

	return size;
}

static ssize_t proximity_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);
	u16 ps_data;

	mutex_lock(&cm36653->power_lock);
	if (!(cm36653->power_state & PROXIMITY_ENABLED)) {
		if (cm36653->prox_led_on)
			prox_led_onoff(cm36653, true);

		cm36653_i2c_write_word(cm36653, REG_PS_CONF1,
			ps_reg_init_setting[PS_CONF1][CMD]);
	}

	mutex_lock(&cm36653->read_lock);
	cm36653_i2c_read_word(cm36653, REG_PS_DATA,
		&ps_data);
	mutex_unlock(&cm36653->read_lock);

	if (!(cm36653->power_state & PROXIMITY_ENABLED)) {
		cm36653_i2c_write_word(cm36653, REG_PS_CONF1,
				0x0001);
		if (cm36653->prox_led_on)
			prox_led_onoff(cm36653, false);
	}
	mutex_unlock(&cm36653->power_lock);

	return sprintf(buf, "%d\n", (u8)(0xff & ps_data));
}

static ssize_t proximity_thresh_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "prox_threshold = %d\n",
		ps_reg_init_setting[PS_THD][CMD] >> 8 & 0xff);
}

static ssize_t proximity_thresh_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);
	u8 thresh_value = ps_reg_init_setting[PS_THD][CMD] >> 8 & 0xff;
	int err;

	err = kstrtou8(buf, 10, &thresh_value);
	if (err < 0)
		pr_err("[SENSOR] %s, kstrtou8 failed.", __func__);

	if (thresh_value > 2) {
		ps_reg_init_setting[PS_THD][CMD] =
			(u16)(thresh_value << 8 | (thresh_value - 2));
		err = cm36653_i2c_write_word(cm36653,
				REG_PS_THD,	ps_reg_init_setting[PS_THD][CMD]);
		if (err < 0)
			pr_err("[SENSOR] %s: cm36653_ps_reg is failed. %d\n", __func__,
			       err);
		pr_info("[SENSOR] %s, new threshold = 0x%x\n",
			__func__, thresh_value);
		msleep(150);
	} else
		pr_err("[SENSOR] %s, wrong threshold value(0x%x)!!\n",
			__func__, thresh_value);

	return size;
}

#ifdef CM36653_CANCELATION
static DEVICE_ATTR(prox_cal, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_cancel_show, proximity_cancel_store);
#endif
static DEVICE_ATTR(prox_avg, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_avg_show, proximity_avg_store);
static DEVICE_ATTR(state, S_IRUGO,
	proximity_state_show, NULL);
static DEVICE_ATTR(prox_thresh, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_thresh_show, proximity_thresh_store);
static struct device_attribute dev_attr_prox_raw =
	__ATTR(raw_data, S_IRUGO, proximity_state_show, NULL);

static struct device_attribute *prox_sensor_attrs[] = {
	&dev_attr_prox_sensor_vendor,
	&dev_attr_prox_sensor_name,
	&dev_attr_prox_cal,
	&dev_attr_prox_avg,
	&dev_attr_state,
	&dev_attr_prox_thresh,
	&dev_attr_prox_raw,
	NULL,
};

/* light sysfs */
static ssize_t light_lux_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);

	return sprintf(buf, "%u,%u,%u,%u\n",
		cm36653->color[0]+1, cm36653->color[1]+1,
		cm36653->color[2]+1, cm36653->color[3]+1);
}

static ssize_t light_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);

	return sprintf(buf, "%u,%u,%u,%u\n",
		cm36653->color[0]+1, cm36653->color[1]+1,
		cm36653->color[2]+1, cm36653->color[3]+1);
}

static DEVICE_ATTR(lux, S_IRUGO, light_lux_show, NULL);
static struct device_attribute dev_attr_light_raw =
	__ATTR(raw_data, S_IRUGO, light_data_show, NULL);

static struct device_attribute *light_sensor_attrs[] = {
	&dev_attr_light_sensor_vendor,
	&dev_attr_light_sensor_name,
	&dev_attr_lux,
	&dev_attr_light_raw,
	NULL,
};

/* interrupt happened due to transition/change of near/far proximity state */
irqreturn_t cm36653_irq_thread_fn(int irq, void *data)
{
	struct cm36653_data *cm36653 = data;
	u8 val = 1;
#ifdef CM36653_DEBUG
	static int count;
#endif
	u16 ps_data = 0;

	val = gpio_get_value(cm36653->prox_int);
	cm36653_i2c_read_word(cm36653, REG_PS_DATA, &ps_data);
#ifdef CM36653_DEBUG
	pr_info("[SENSOR] %s: count = %d\n", __func__, count++);
#endif

	if (cm36653->power_state & PROXIMITY_ENABLED) {
		/* 0 is close, 1 is far */
		input_report_abs(cm36653->proximity_input_dev, ABS_DISTANCE, val);
		input_sync(cm36653->proximity_input_dev);
	}

	wake_lock_timeout(&cm36653->prx_wake_lock, 3 * HZ);

	pr_info("[SENSOR] %s: val = %d, ps_data = %d (close:0, far:1)\n",
		__func__, val, (u8)(0xff & ps_data));

	return IRQ_HANDLED;
}

static int cm36653_setup_reg(struct cm36653_data *cm36653)
{
	int err = 0, i = 0;
	u16 tmp = 0;

	/* ALS initialization */
	err = cm36653_i2c_write_byte(cm36653,
			als_reg_setting[0][0],
			als_reg_setting[0][1]);
	if (err < 0) {
		pr_err("[SENSOR] %s: cm36653_als_reg is failed. %d\n", __func__,
			err);
		return err;
	}
	/* PS initialization */
	for (i = 0; i < PS_REG_NUM; i++) {
		err = cm36653_i2c_write_word(cm36653,
			ps_reg_init_setting[i][REG_ADDR],
			ps_reg_init_setting[i][CMD]);
		if (err < 0) {
			pr_err("[SENSOR] %s: cm36653_ps_reg is failed. %d\n", __func__,
			       err);
			return err;
		}
	}

	/* printing the inital proximity value with no contact */
	msleep(50);
	mutex_lock(&cm36653->read_lock);
	err = cm36653_i2c_read_word(cm36653, REG_PS_DATA, &tmp);
	mutex_unlock(&cm36653->read_lock);
	if (err < 0) {
		pr_err("[SENSOR] %s: read ps_data failed\n", __func__);
		err = -EIO;
	}
	pr_err("[SENSOR] %s: initial proximity value = %d\n",
		__func__, (u8)(0xff & tmp));

	/* turn off */
	cm36653_i2c_write_byte(cm36653, REG_CS_CONF1, 0x01);
	cm36653_i2c_write_word(cm36653, REG_PS_CONF1, 0x0001);

	pr_info("[SENSOR] %s is success.", __func__);
	return err;
}

static int cm36653_setup_gpio(struct cm36653_data *cm36653)
{
	int rc = -EIO;

	rc = gpio_request(cm36653->prox_led_on, "gpio_prox_led_on");
	if (rc < 0) {
		pr_err("[SENSOR] %s: gpio %d request failed (%d)\n",
		       __func__, cm36653->prox_led_on, rc);
		return rc;
	}

	gpio_direction_output(cm36653->prox_led_on, 0);

	rc = gpio_request(cm36653->prox_int, "gpio_proximity_out");
	if (rc < 0) {
		pr_err("[SENSOR] %s: gpio %d request failed (%d)\n",
		       __func__, cm36653->prox_int, rc);
		return rc;
	}

	rc = gpio_direction_input(cm36653->prox_int);
	if (rc < 0) {
		pr_err("[SENSOR] %s: failed to set gpio %d as input (%d)\n",
		       __func__, cm36653->prox_int, rc);
		goto err_gpio_direction_input;
	}

	cm36653->irq = gpio_to_irq(cm36653->prox_int);
	rc = request_threaded_irq(cm36653->irq, NULL,
				  cm36653_irq_thread_fn,
				  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				  "proximity_int", cm36653);
	if (rc < 0) {
		pr_err("[SENSOR] %s: request_irq(%d) failed for gpio %d (%d)\n",
		       __func__, cm36653->irq, cm36653->prox_int, rc);
		goto err_request_irq;
	}

	/* start with interrupts disabled */
	disable_irq(cm36653->irq);

	pr_err("[SENSOR] %s, success\n", __func__);

	goto done;

err_request_irq:
err_gpio_direction_input:
	gpio_free(cm36653->prox_int);
	gpio_free(cm36653->prox_led_on);
done:
	return rc;
}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart cm36653_light_timer_func(struct hrtimer *timer)
{
	struct cm36653_data *cm36653
	    = container_of(timer, struct cm36653_data, light_timer);
	queue_work(cm36653->light_wq, &cm36653->work_light);
	hrtimer_forward_now(&cm36653->light_timer, cm36653->light_poll_delay);
	return HRTIMER_RESTART;
}

static void cm36653_work_func_light(struct work_struct *work)
{
	struct cm36653_data *cm36653 = container_of(work, struct cm36653_data,
						    work_light);

	mutex_lock(&cm36653->read_lock);
	cm36653_i2c_read_word(cm36653, REG_RED, &cm36653->color[0]);
	cm36653_i2c_read_word(cm36653, REG_GREEN, &cm36653->color[1]);
	cm36653_i2c_read_word(cm36653, REG_BLUE, &cm36653->color[2]);
	cm36653_i2c_read_word(cm36653, REG_WHITE, &cm36653->color[3]);
	mutex_unlock(&cm36653->read_lock);

	input_report_rel(cm36653->light_input_dev, REL_RED,
		cm36653->color[0]+1);
	input_report_rel(cm36653->light_input_dev, REL_GREEN,
		cm36653->color[1]+1);
	input_report_rel(cm36653->light_input_dev, REL_BLUE,
		cm36653->color[2]+1);
	input_report_rel(cm36653->light_input_dev, REL_WHITE,
		cm36653->color[3]+1);
	input_sync(cm36653->light_input_dev);

	if ((ktime_to_ms(cm36653->light_poll_delay) * (int64_t)cm36653->count_log_time)
		>= ((int64_t)LIGHT_LOG_TIME * MSEC_PER_SEC)) {
		pr_info("[SENSOR] %s, red = %u green = %u blue = %u white = %u\n",
			__func__, cm36653->color[0]+1, cm36653->color[1]+1,
			cm36653->color[2]+1, cm36653->color[3]+1);
		cm36653->count_log_time = 0;
	} else
		cm36653->count_log_time++;

#ifdef CM36653_DEBUG
	pr_info("[SENSOR] %s, red = %u green = %u blue = %u white = %u\n",
		__func__, cm36653->color[0]+1, cm36653->color[1]+1,
		cm36653->color[2]+1, cm36653->color[3]+1);
#endif
}

static void proxsensor_get_avg_val(struct cm36653_data *cm36653)
{
	int min = 0, max = 0, avg = 0;
	int i;
	u16 ps_data = 0;

	for (i = 0; i < PROX_READ_NUM; i++) {
		msleep(40);
		cm36653_i2c_read_word(cm36653, REG_PS_DATA,
			&ps_data);
		avg += (u8)(0xff & ps_data);

		if (!i)
			min = ps_data;
		else if (ps_data < min)
			min = ps_data;

		if (ps_data > max)
			max = ps_data;
	}
	avg /= PROX_READ_NUM;

	cm36653->avg[0] = min;
	cm36653->avg[1] = avg;
	cm36653->avg[2] = max;
}

static void cm36653_work_func_prox(struct work_struct *work)
{
	struct cm36653_data *cm36653 = container_of(work, struct cm36653_data,
						  work_prox);
	proxsensor_get_avg_val(cm36653);
}

static enum hrtimer_restart cm36653_prox_timer_func(struct hrtimer *timer)
{
	struct cm36653_data *cm36653
			= container_of(timer, struct cm36653_data, prox_timer);
	queue_work(cm36653->prox_wq, &cm36653->work_prox);
	hrtimer_forward_now(&cm36653->prox_timer, cm36653->prox_poll_delay);
	return HRTIMER_RESTART;
}

static int cm36653_parse_dt(struct cm36653_data *data, struct device *dev)
{
	struct device_node *dNode = dev->of_node;
	enum of_gpio_flags flags;

	if (dNode == NULL)
		return -ENODEV;

	data->prox_int = of_get_named_gpio_flags(dNode,
		"cm36653-i2c,prox_int-gpio", 0, &flags);
	if (data->prox_int < 0) {
		pr_err("[SENSOR]: %s - get prox_int error\n", __func__);
		return -ENODEV;
	}

	data->prox_led_on = of_get_named_gpio_flags(dNode,
		"cm36653-i2c,prox_led_en-gpio", 0, &flags);
	if (data->prox_led_on < 0) {
		pr_err("[SENSOR]: %s - get prox_led_en error\n", __func__);
		return -ENODEV;
	}

	if (of_property_read_u32(dNode,
			"cm36653-i2c,threshold", &data->threshold) < 0)
		data->threshold = 0x0806;

	pr_info("[SENSOR] %s success, threshold = 0x%X\n",
		__func__, data->threshold);
	return 0;
}

static int cm36653_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct cm36653_data *cm36653 = NULL;

	pr_info("[SENSOR] %s: Probe Start!\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[SENSOR] %s: i2c functionality check failed!\n", __func__);
		return ret;
	}

	cm36653 = kzalloc(sizeof(struct cm36653_data), GFP_KERNEL);
	if (!cm36653) {
		pr_err
		    ("[SENSOR] %s: failed to alloc memory for RGB sensor module data\n",
		     __func__);
		return -ENOMEM;
	}

	cm36653->i2c_client = client;
	i2c_set_clientdata(client, cm36653);

	mutex_init(&cm36653->power_lock);
	mutex_init(&cm36653->read_lock);

	ret = cm36653_parse_dt(cm36653, &client->dev);
		if (ret < 0) {
		pr_err("[SENSOR] %s: - of_node error\n", __func__);
		ret = -ENODEV;
		goto err_of_node;
	}

	/* setup gpio */
	ret = cm36653_setup_gpio(cm36653);
	if (ret) {
		pr_err("[SENSOR] %s: could not setup gpio\n", __func__);
		goto err_setup_gpio;
	}

	prox_led_onoff(cm36653, true);

	/* wake lock init for proximity sensor */
	wake_lock_init(&cm36653->prx_wake_lock, WAKE_LOCK_SUSPEND,
		       "prx_wake_lock");

	/* Check if the device is there or not. */
	ret = cm36653_i2c_write_byte(cm36653, REG_CS_CONF1, 0x01);
	if (ret < 0) {
		pr_err("[SENSOR] %s: cm36653 is not connected.(%d)\n", __func__, ret);
		goto err_setup_reg;
	}
	/* setup initial registers */
	ps_reg_init_setting[PS_THD][CMD] = cm36653->threshold;

#ifdef CM36653_CANCELATION
	cm36653->default_thresh = ps_reg_init_setting[PS_THD][CMD];
#endif
	ret = cm36653_setup_reg(cm36653);
	if (ret < 0) {
		pr_err("[SENSOR] %s: could not setup regs\n", __func__);
		goto err_setup_reg;
	}

	prox_led_onoff(cm36653, false);

	/* allocate proximity input_device */
	cm36653->proximity_input_dev = input_allocate_device();
	if (!cm36653->proximity_input_dev) {
		pr_err("[SENSOR] %s: could not allocate proximity input device\n",
		       __func__);
		ret = -ENOMEM;
		goto err_input_allocate_device_proximity;
	}

	input_set_drvdata(cm36653->proximity_input_dev, cm36653);
	cm36653->proximity_input_dev->name = MODULE_NAME_PROX;
	input_set_capability(cm36653->proximity_input_dev, EV_ABS,
			     ABS_DISTANCE);
	input_set_abs_params(cm36653->proximity_input_dev, ABS_DISTANCE, 0, 1,
			     0, 0);

	ret = input_register_device(cm36653->proximity_input_dev);
	if (ret < 0) {
		input_free_device(cm36653->proximity_input_dev);
		pr_err("[SENSOR] %s: could not register input device\n", __func__);
		goto err_input_register_device_proximity;
	}

	ret = sensors_create_symlink(&cm36653->proximity_input_dev->dev.kobj,
					cm36653->proximity_input_dev->name);
	if (ret < 0) {
		pr_err("[SENSOR] %s: create_symlink error\n", __func__);
		goto err_sensors_create_symlink_prox;
	}

	ret = sysfs_create_group(&cm36653->proximity_input_dev->dev.kobj,
				 &proximity_attribute_group);
	if (ret) {
		pr_err("[SENSOR] %s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_proximity;
	}

	/* For factory test mode, we use timer to get average proximity data. */
	/* prox_timer settings. we poll for light values using a timer. */
	hrtimer_init(&cm36653->prox_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cm36653->prox_poll_delay = ns_to_ktime(2000 * NSEC_PER_MSEC);/*2 sec*/
	cm36653->prox_timer.function = cm36653_prox_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	cm36653->prox_wq = create_singlethread_workqueue("cm36653_prox_wq");
	if (!cm36653->prox_wq) {
		ret = -ENOMEM;
		pr_err("[SENSOR] %s: could not create prox workqueue\n", __func__);
		goto err_create_prox_workqueue;
	}
	/* this is the thread function we run on the work queue */
	INIT_WORK(&cm36653->work_prox, cm36653_work_func_prox);

	/* allocate lightsensor input_device */
	cm36653->light_input_dev = input_allocate_device();
	if (!cm36653->light_input_dev) {
		pr_err("[SENSOR] %s: could not allocate light input device\n", __func__);
		ret = -ENOMEM;
		goto err_input_allocate_device_light;
	}

	input_set_drvdata(cm36653->light_input_dev, cm36653);
	cm36653->light_input_dev->name = MODULE_NAME_LIGHT;
	input_set_capability(cm36653->light_input_dev, EV_REL, REL_RED);
	input_set_capability(cm36653->light_input_dev, EV_REL, REL_GREEN);
	input_set_capability(cm36653->light_input_dev, EV_REL, REL_BLUE);
	input_set_capability(cm36653->light_input_dev, EV_REL, REL_WHITE);

	ret = input_register_device(cm36653->light_input_dev);
	if (ret < 0) {
		input_free_device(cm36653->light_input_dev);
		pr_err("%s: could not register input device\n", __func__);
		goto err_input_register_device_light;
	}

	ret = sensors_create_symlink(&cm36653->light_input_dev->dev.kobj,
					cm36653->light_input_dev->name);
	if (ret < 0) {
		pr_err("[SENSOR] %s: create_symlink error\n", __func__);
		goto err_sensors_create_symlink_light;
	}

	ret = sysfs_create_group(&cm36653->light_input_dev->dev.kobj,
				 &light_attribute_group);
	if (ret) {
		pr_err("[SENSOR] %s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}

	/* light_timer settings. we poll for light values using a timer. */
	hrtimer_init(&cm36653->light_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cm36653->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	cm36653->light_timer.function = cm36653_light_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	cm36653->light_wq = create_singlethread_workqueue("cm36653_light_wq");
	if (!cm36653->light_wq) {
		ret = -ENOMEM;
		pr_err("[SENSOR] %s: could not create light workqueue\n", __func__);
		goto err_create_light_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&cm36653->work_light, cm36653_work_func_light);

	/* set sysfs for proximity sensor */
	ret = sensors_register(cm36653->proximity_dev,
		cm36653, prox_sensor_attrs,
			"proximity_sensor");
	if (ret) {
		pr_err("[SENSOR] %s: cound not register\
			proximity sensor device(%d).\n",
			__func__, ret);
		goto prox_sensor_register_failed;
	}

	/* set sysfs for light sensor */
	ret = sensors_register(cm36653->light_dev,
		cm36653, light_sensor_attrs,
			"light_sensor");
	if (ret) {
		pr_err("[SENSOR] %s: cound not register\
			light sensor device(%d).\n",
			__func__, ret);
		goto light_sensor_register_failed;
	}

	pr_info("[SENSOR] %s is success.\n", __func__);
	goto done;

/* error, unwind it all */
light_sensor_register_failed:
	sensors_unregister(cm36653->proximity_dev, prox_sensor_attrs);
prox_sensor_register_failed:
	destroy_workqueue(cm36653->light_wq);
err_create_light_workqueue:
	sysfs_remove_group(&cm36653->light_input_dev->dev.kobj,
			   &light_attribute_group);
err_sysfs_create_group_light:
	sensors_remove_symlink(&cm36653->light_input_dev->dev.kobj,
			cm36653->light_input_dev->name);
err_sensors_create_symlink_light:
	input_unregister_device(cm36653->light_input_dev);
err_input_register_device_light:
err_input_allocate_device_light:
	destroy_workqueue(cm36653->prox_wq);
err_create_prox_workqueue:
	sysfs_remove_group(&cm36653->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
err_sysfs_create_group_proximity:
	sensors_remove_symlink(&cm36653->proximity_input_dev->dev.kobj,
			cm36653->proximity_input_dev->name);
err_sensors_create_symlink_prox:
	input_unregister_device(cm36653->proximity_input_dev);
err_input_register_device_proximity:
err_input_allocate_device_proximity:
err_setup_reg:
	wake_lock_destroy(&cm36653->prx_wake_lock);
	prox_led_onoff(cm36653, false);
	free_irq(cm36653->irq, cm36653);
	gpio_free(cm36653->prox_int);
	gpio_free(cm36653->prox_led_on);
err_setup_gpio:
err_of_node:
	mutex_destroy(&cm36653->read_lock);
	mutex_destroy(&cm36653->power_lock);
	kfree(cm36653);
done:
	return ret;
}

static int cm36653_i2c_remove(struct i2c_client *client)
{
	struct cm36653_data *cm36653 = i2c_get_clientdata(client);

	/* free irq */
	if (cm36653->power_state & PROXIMITY_ENABLED) {
		disable_irq_wake(cm36653->irq);
		disable_irq(cm36653->irq);
	}
	free_irq(cm36653->irq, cm36653);
	gpio_free(cm36653->prox_int);

	/* device off */
	if (cm36653->power_state & LIGHT_ENABLED)
		cm36653_light_disable(cm36653);
	if (cm36653->power_state & PROXIMITY_ENABLED) {
		cm36653_i2c_write_byte(cm36653, REG_PS_CONF1, 0x01);
		if (cm36653->prox_led_on)
			prox_led_onoff(cm36653, false);
	}

	/* destroy workqueue */
	destroy_workqueue(cm36653->light_wq);
	destroy_workqueue(cm36653->prox_wq);

	/* sysfs destroy */
	sensors_unregister(cm36653->light_dev, light_sensor_attrs);
	sensors_unregister(cm36653->proximity_dev, prox_sensor_attrs);
	sensors_remove_symlink(&cm36653->light_input_dev->dev.kobj,
			cm36653->light_input_dev->name);
	sensors_remove_symlink(&cm36653->proximity_input_dev->dev.kobj,
			cm36653->proximity_input_dev->name);

	/* input device destroy */
	sysfs_remove_group(&cm36653->light_input_dev->dev.kobj,
			   &light_attribute_group);
	input_unregister_device(cm36653->light_input_dev);
	sysfs_remove_group(&cm36653->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
	input_unregister_device(cm36653->proximity_input_dev);

	/* lock destroy */
	mutex_destroy(&cm36653->read_lock);
	mutex_destroy(&cm36653->power_lock);
	wake_lock_destroy(&cm36653->prx_wake_lock);

	kfree(cm36653);

	return 0;
}

static int cm36653_suspend(struct device *dev)
{
	/* We disable power only if proximity is disabled.  If proximity
	   is enabled, we leave power on because proximity is allowed
	   to wake up device.  We remove power without changing
	   cm36653->power_state because we use that state in resume.
	 */
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);

	if (cm36653->power_state & LIGHT_ENABLED)
		cm36653_light_disable(cm36653);

	return 0;
}

static int cm36653_resume(struct device *dev)
{
	struct cm36653_data *cm36653 = dev_get_drvdata(dev);

	if (cm36653->power_state & LIGHT_ENABLED)
		cm36653_light_enable(cm36653);

	return 0;
}

static struct of_device_id cm36653_match_table[] = {
	{ .compatible = "cm36653-i2c",},
	{},
};

static const struct i2c_device_id cm36653_device_id[] = {
	{"cm36653_match_table", 0},
	{}
};

static const struct dev_pm_ops cm36653_pm_ops = {
	.suspend = cm36653_suspend,
	.resume = cm36653_resume
};

static struct i2c_driver cm36653_i2c_driver = {
	.driver = {
		   .name = CHIP_ID,
		   .owner = THIS_MODULE,
		   .of_match_table = cm36653_match_table,
		   .pm = &cm36653_pm_ops
	},
	.probe = cm36653_i2c_probe,
	.remove = cm36653_i2c_remove,
	.id_table = cm36653_device_id,
};

static int __init cm36653_init(void)
{
	return i2c_add_driver(&cm36653_i2c_driver);
}

static void __exit cm36653_exit(void)
{
	i2c_del_driver(&cm36653_i2c_driver);
}

module_init(cm36653_init);
module_exit(cm36653_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("RGB Sensor device driver for cm36653");
MODULE_LICENSE("GPL");
