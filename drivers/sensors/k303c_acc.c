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
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/wakelock.h>
#include <linux/sensor/sensors_core.h>

#define I2C_M_WR			0 /* for i2c Write */

#define READ_DATA_LENTH			6

#define VENDOR_NAME			"STM"
#define MODEL_NAME			"K303C"
#define MODULE_NAME			"accelerometer_sensor"

#define CALIBRATION_FILE_PATH		"/efs/accel_calibration_data"
#define CALIBRATION_DATA_AMOUNT		20
#define MAX_ACCEL_1G			16384

#define K303C_DEFAULT_DELAY		200000000LL

#define CHIP_ID_RETRIES			3
#define ACCEL_LOG_TIME			15 /* 15 sec */

#define K303C_TOP_UPPER_RIGHT		0
#define K303C_TOP_LOWER_RIGHT		1
#define K303C_TOP_LOWER_LEFT		2
#define K303C_TOP_UPPER_LEFT		3
#define K303C_BOTTOM_UPPER_RIGHT	4
#define K303C_BOTTOM_LOWER_RIGHT	5
#define K303C_BOTTOM_LOWER_LEFT		6
#define K303C_BOTTOM_UPPER_LEFT		7

#define K303C_MODE_SUSPEND		0
#define K303C_MODE_NORMAL		1

#define SENSITIVITY_2G			61
#define SENSITIVITY_4G			122
#define SENSITIVITY_8G			244

#define K303C_RANGE_2G			0
#define K303C_RANGE_4G			1
#define K303C_RANGE_8G			2

#define WHOAMI_REG			0x0F
#define AXISDATA_REG			0x28

#define CTRL1_REG			0x20
#define CTRL2_REG			0x21
#define CTRL3_REG			0x22
#define CTRL4_REG			0x23
#define CTRL5_REG			0x24
#define CTRL6_REG			0x25
#define CTRL7_REG			0x26
#define STATUS_REG			0x27

/* CTRL1 */
#define CTRL1_HR_DISABLE		0x00
#define CTRL1_HR_ENABLE			0x80
#define CTRL1_HR_MASK			0x80
#define CTRL1_BDU_ENABLE		0x08
#define CTRL1_BDU_MASK			0x08

/* CTRL2 */
#define CTRL2_IG1_INT1			0x08

/* CTRL3 */
#define CTRL3_IG1_INT1			0x08

/* CTRL7 */
#define CTRL7_LIR2			0x08
#define CTRL7_LIR1			0x04

#define ACC_PM_OFF			0x00
#define ACC_ENABLE_ALL_AXES		0x07

#define INT_CFG1_REG			0x30
#define INT_SRC1_REG			0x31
#define K303C_CHIP_ID			0x41

#define K303C_ACC_FS_MASK		0x30
#define K303C_ACC_ODR_MASK		0x70
#define K303C_ACC_AXES_MASK		0x07

#define SELF_TEST_2G_MAX_LSB		24576
#define SELF_TEST_2G_MIN_LSB		1146

#define K303C_ACC_FS_2G			0x00
#define K303C_ACC_FS_4G			0x20
#define K303C_ACC_FS_8G			0x30

#define INT_THSX1_REG			0x32
#define INT_THSY1_REG			0x33
#define INT_THSZ1_REG			0x34

#define DYNAMIC_THRESHOLD		5000

enum {
	OFF = 0,
	ON = 1
};

struct k303c_acc_v {
	union {
		s16 v[3];
		struct {
			s16 x;
			s16 y;
			s16 z;
		};
	};
};

struct k303c_acc_p {
	struct wake_lock reactive_wake_lock;
	struct i2c_client *client;
	struct input_dev *input;
	struct delayed_work irq_work;
	struct device *factory_device;
	struct k303c_acc_v accdata;
	struct k303c_acc_v caldata;
	struct mutex mode_mutex;
	struct hrtimer accel_timer;
	struct workqueue_struct *accel_wq;
	struct work_struct work;
	struct regulator *vdd;
	struct regulator *vio;
	ktime_t poll_delay;
	atomic_t enable;

	struct regulator *reg_vdd;
	struct regulator *reg_vio;

	int recog_flag;
	int irq1;
	int irq_state;
	int acc_int1;
	int sda_gpio;
	int scl_gpio;
	int time_count;

	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;
};

#define ACC_ODR10		0x10	/*   10Hz output data rate */
#define ACC_ODR50		0x20	/*   50Hz output data rate */
#define ACC_ODR100		0x30	/*  100Hz output data rate */
#define ACC_ODR200		0x40	/*  200Hz output data rate */
#define ACC_ODR400		0x50	/*  400Hz output data rate */
#define ACC_ODR800		0x60	/*  800Hz output data rate */
#define ACC_ODR_MASK		0X70

struct k303c_acc_odr {
	unsigned int cutoff_ms;
	unsigned int mask;
};

#define OUTPUT_ALWAYS_ANTI_ALIASED

const struct k303c_acc_odr k303c_acc_odr_table[] = {
	{  2, ACC_ODR800},
	{  3, ACC_ODR400},
	{  5, ACC_ODR200},
	{ 10, ACC_ODR100},
#ifndef OUTPUT_ALWAYS_ANTI_ALIASED
	{ 20, ACC_ODR50},
	{100, ACC_ODR10},
#endif
};

#define MAX_VDD	3000000
#define AVG_VDD	2850000
#define AVG_VIO	1800000

static int k303c_acc_i2c_read(struct k303c_acc_p *data, unsigned char reg_addr,
	unsigned char *buf, unsigned int len)
{
	int ret, retries = 0;
	struct i2c_msg msg[2];

	msg[0].addr = data->client->addr;
	msg[0].flags = I2C_M_WR;
	msg[0].len = 1;
	msg[0].buf = &reg_addr;

	msg[1].addr = data->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = buf;

	do {
		ret = i2c_transfer(data->client->adapter, msg, 2);
		if (ret >= 0)
			break;
	} while (retries++ < 2);

	if (ret < 0) {
		pr_err("%s - i2c read error %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int k303c_acc_i2c_write(struct k303c_acc_p *data,
	unsigned char reg_addr, unsigned char buf)
{
	int ret, retries = 0;
	struct i2c_msg msg;
	unsigned char w_buf[2];

	w_buf[0] = reg_addr;
	w_buf[1] = buf;

	msg.addr = data->client->addr;
	msg.flags = I2C_M_WR;
	msg.len = 2;
	msg.buf = (char *)w_buf;

	do {
		ret = i2c_transfer(data->client->adapter, &msg, 1);
		if (ret >= 0)
			break;
	} while (retries++ < 2);

	if (ret < 0) {
		pr_err("%s - i2c write error %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int k303c_regulator_onoff(struct k303c_acc_p *data, bool onoff)
{
	int ret;

	if (IS_ERR(data->reg_vdd) || IS_ERR(data->reg_vio)) {
		pr_err("%s: Failed to enable regulator.\n", __func__);
		return -ENODEV;
	}

	if (onoff) {
		ret = regulator_enable(data->reg_vdd);
		if (ret) {
			pr_err("%s: Failed to enable vdd.\n", __func__);
			return ret;
		}
		ret = regulator_enable(data->reg_vio);
		if (ret) {
			pr_err("%s: Failed to enable vio.\n", __func__);
			return ret;
		}
		msleep(30);
	} else {
		ret = regulator_disable(data->reg_vdd);
		if (ret) {
			pr_err("%s: Failed to disable vdd.\n", __func__);
			return ret;
		}
		ret = regulator_disable(data->reg_vio);
		if (ret) {
			pr_err("%s: Failed to disable vio.\n", __func__);
			return ret;
		}
	}

	return 0;
}

static int k303c_acc_set_range(struct k303c_acc_p *data, unsigned char range)
{
	int ret = 0;
	unsigned char temp, new_range, buf, mask;

	switch (range) {
	case K303C_RANGE_2G:
		new_range = K303C_ACC_FS_2G;
		break;
	case K303C_RANGE_4G:
		new_range = K303C_ACC_FS_4G;
		break;
	case K303C_RANGE_8G:
		new_range = K303C_ACC_FS_8G;
		break;
	default:
		new_range = K303C_ACC_FS_2G;
		break;
	}

	mask = K303C_ACC_FS_MASK;
	ret = k303c_acc_i2c_read(data, CTRL4_REG, &temp, 1);
	buf = (mask & new_range) | ((~mask) & temp);
#ifdef OUTPUT_ALWAYS_ANTI_ALIASED
	buf |= 0xC8;
#endif
	ret += k303c_acc_i2c_write(data, CTRL4_REG, buf);

	return ret;
}

static int k303c_acc_set_odr(struct k303c_acc_p *data)
{
	int ret = 0, i;
	unsigned char buf, new_odr, mask, temp;

	pr_info("%s\n", __func__);

	/* Following, looks for the longest possible odr interval scrolling the
	 * odr_table vector from the end (shortest interval) backward (longest
	 * interval), to support the poll_interval requested by the system.
	 * It must be the longest interval lower then the poll interval.*/
	for (i = ARRAY_SIZE(k303c_acc_odr_table) - 1; i >= 0; i--) {
		if ((k303c_acc_odr_table[i].cutoff_ms <=
			ktime_to_ms(data->poll_delay)) || (i == 0))
			break;
	}

	if (data->recog_flag == ON)
		i = ARRAY_SIZE(k303c_acc_odr_table) - 1;

	new_odr = k303c_acc_odr_table[i].mask;

	mask = K303C_ACC_ODR_MASK;
	ret = k303c_acc_i2c_read(data, CTRL1_REG, &temp, 1);
	buf = ((mask & new_odr) | ((~mask) & temp));
	ret += k303c_acc_i2c_write(data, CTRL1_REG, buf);

	pr_info("%s - change odr %d\n", __func__, i);
	return ret;
}

static int k303c_acc_set_mode(struct k303c_acc_p *data, unsigned char mode)
{
	int ret = 0;
	unsigned char buf, mask, temp;
	static unsigned char acc_odr;

	mutex_lock(&data->mode_mutex);

	switch (mode) {
	case K303C_MODE_NORMAL:
		mask = K303C_ACC_ODR_MASK;
		ret = k303c_acc_i2c_read(data, CTRL1_REG, &temp, 1);
		buf = ((mask & acc_odr) | ((~mask) & temp));
		buf |= 0x8f;
		ret += k303c_acc_i2c_write(data, CTRL1_REG, buf);
		break;
	case K303C_MODE_SUSPEND:
		if (data->recog_flag == ON)
			break;

		mask = K303C_ACC_ODR_MASK;
		ret = k303c_acc_i2c_read(data, CTRL1_REG, &temp, 1);
		acc_odr = temp & mask;
		buf = ((mask & ACC_PM_OFF) | ((~mask) & temp));
		ret += k303c_acc_i2c_write(data, CTRL1_REG, buf);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&data->mode_mutex);
	msleep(20);

	return ret;
}

static int k303c_acc_read_accel_xyz(struct k303c_acc_p *data,
	struct k303c_acc_v *acc)
{
	int ret = 0;
	struct k303c_acc_v rawdata;
	unsigned char buf[READ_DATA_LENTH];

	ret += k303c_acc_i2c_read(data, AXISDATA_REG, buf, READ_DATA_LENTH);
	if (ret < 0)
		goto exit;

	rawdata.v[0] = ((s16) ((buf[1] << 8) | buf[0]));
	rawdata.v[1] = ((s16) ((buf[3] << 8) | buf[2]));
	rawdata.v[2] = ((s16) ((buf[5] << 8) | buf[4]));

	acc->v[0] = ((data->negate_x) ? (-rawdata.v[data->axis_map_x])
		   : (rawdata.v[data->axis_map_x]));
	acc->v[1] = ((data->negate_y) ? (-rawdata.v[data->axis_map_y])
		   : (rawdata.v[data->axis_map_y]));
	acc->v[2] = ((data->negate_z) ? (-rawdata.v[data->axis_map_z])
		   : (rawdata.v[data->axis_map_z]));

exit:
	return ret;
}

static void k303c_acc_set_enable(struct k303c_acc_p *data, int enable)
{
	pr_info("%s %d\n", __func__, enable);

	if (enable == ON) {
		hrtimer_start(&data->accel_timer, data->poll_delay,
		      HRTIMER_MODE_REL);
	} else {
		hrtimer_cancel(&data->accel_timer);
		cancel_work_sync(&data->work);
	}
}

static int k303c_acc_open_calibration(struct k303c_acc_p *data)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	pr_info("%s\n", __func__);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY,
		S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);

		data->caldata.x = 0;
		data->caldata.y = 0;
		data->caldata.z = 0;

		pr_info("%s - No Calibration\n", __func__);

		return ret;
	}

	ret = cal_filp->f_op->read(cal_filp, (char *)&data->caldata.v,
		3 * sizeof(s16), &cal_filp->f_pos);
	if (ret != 3 * sizeof(s16)) {
		pr_err("%s: - Can't read the cal data\n", __func__);
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	pr_info("%s open accel calibration %d, %d, %d\n", __func__,
		data->caldata.x, data->caldata.y, data->caldata.z);

	if ((data->caldata.x == 0) && (data->caldata.y == 0)
		&& (data->caldata.z == 0))
		return -EIO;

	return ret;
}

static int k303c_acc_do_calibrate(struct k303c_acc_p *data, int enable)
{
	int sum[3] = { 0, };
	int ret = 0, cnt;
	struct file *cal_filp = NULL;
	struct k303c_acc_v acc;
	mm_segment_t old_fs;

	pr_info("%s\n", __func__);

	if (enable) {
		data->caldata.x = 0;
		data->caldata.y = 0;
		data->caldata.z = 0;

		if (atomic_read(&data->enable) == ON)
			k303c_acc_set_enable(data, OFF);
		else
			k303c_acc_set_mode(data, K303C_MODE_NORMAL);

		msleep(300);

		for (cnt = 0; cnt < CALIBRATION_DATA_AMOUNT; cnt++) {
			k303c_acc_read_accel_xyz(data, &acc);
			sum[0] += acc.x;
			sum[1] += acc.y;
			sum[2] += acc.z;
			msleep(20);
		}

		if (atomic_read(&data->enable) == ON)
			k303c_acc_set_enable(data, ON);
		else
			k303c_acc_set_mode(data, K303C_MODE_SUSPEND);

		data->caldata.x = (sum[0] / CALIBRATION_DATA_AMOUNT);
		data->caldata.y = (sum[1] / CALIBRATION_DATA_AMOUNT);
		data->caldata.z = (sum[2] / CALIBRATION_DATA_AMOUNT);

		if (data->caldata.z > 0)
			data->caldata.z -= MAX_ACCEL_1G;
		else if (data->caldata.z < 0)
			data->caldata.z += MAX_ACCEL_1G;
	} else {
		data->caldata.x = 0;
		data->caldata.y = 0;
		data->caldata.z = 0;
	}

	pr_info("%s - do accel calibrate %d, %d, %d\n", __func__,
		data->caldata.x, data->caldata.y, data->caldata.z);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
		O_CREAT | O_TRUNC | O_WRONLY, S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("%s - Can't open calibration file\n", __func__);
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);
		return ret;
	}

	ret = cal_filp->f_op->write(cal_filp, (char *)&data->caldata.v,
		3 * sizeof(s16), &cal_filp->f_pos);
	if (ret != 3 * sizeof(s16)) {
		pr_err("%s - Can't write the caldata to file\n", __func__);
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

static ssize_t k303c_acc_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct k303c_acc_p *data = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&data->enable));
}

static ssize_t k303c_acc_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	u8 enable;
	int ret, pre_enable;
	struct k303c_acc_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &enable);
	if (ret) {
		pr_err("%s - Invalid Argument\n", __func__);
		return ret;
	}

	pre_enable = atomic_read(&data->enable);
	pr_info("%s new=%u, pre=%u\n", __func__, enable, pre_enable);

	if (enable) {
		if (pre_enable == OFF) {
			k303c_acc_open_calibration(data);
			k303c_acc_set_range(data, K303C_RANGE_2G);

			k303c_acc_set_enable(data, ON);
			k303c_acc_set_mode(data, K303C_MODE_NORMAL);
			atomic_set(&data->enable, ON);
		}
	} else {
		if (pre_enable == ON) {
			atomic_set(&data->enable, OFF);
			k303c_acc_set_mode(data, K303C_MODE_SUSPEND);
			k303c_acc_set_enable(data, OFF);
		}
	}

	return size;
}

static ssize_t k303c_acc_delay_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct k303c_acc_p *data = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%lld\n",
		ktime_to_ns(data->poll_delay));
}

static ssize_t k303c_acc_delay_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t delay;
	struct k303c_acc_p *data = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);

	ret = kstrtoll(buf, 10, &delay);
	if (ret) {
		pr_err("%s - Invalid Argument\n", __func__);
		return ret;
	}

	data->poll_delay = ns_to_ktime(delay);
	k303c_acc_set_odr(data);

	if (atomic_read(&data->enable) == ON) {
		k303c_acc_set_mode(data, K303C_MODE_SUSPEND);
		k303c_acc_set_mode(data, K303C_MODE_NORMAL);
	}

	pr_info("%s - poll_delay = %lld\n", __func__, delay);
	return size;
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		k303c_acc_delay_show, k303c_acc_delay_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		k303c_acc_enable_show, k303c_acc_enable_store);

static struct attribute *k303c_acc_attributes[] = {
	&dev_attr_poll_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group k303c_acc_attribute_group = {
	.attrs = k303c_acc_attributes
};


static ssize_t k303c_acc_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("%s\n", __func__);
	return snprintf(buf, PAGE_SIZE, "%s\n", MODEL_NAME);
}

static ssize_t k303c_acc_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("%s\n", __func__);
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t k303c_acc_calibration_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;
	struct k303c_acc_p *data = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);

	ret = k303c_acc_open_calibration(data);
	if (ret < 0)
		pr_err("%s - calibration open failed(%d)\n", __func__, ret);

	pr_info("%s - cal data %d %d %d - ret : %d\n", __func__,
		data->caldata.x, data->caldata.y, data->caldata.z, ret);

	return snprintf(buf, PAGE_SIZE, "%d %d %d %d\n", ret, data->caldata.x,
			data->caldata.y, data->caldata.z);
}

static ssize_t k303c_acc_calibration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t dEnable;
	struct k303c_acc_p *data = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);

	ret = kstrtoll(buf, 10, &dEnable);
	if (ret < 0)
		return ret;

	ret = k303c_acc_do_calibrate(data, (int)dEnable);
	if (ret < 0)
		pr_err("%s - accel calibrate failed\n", __func__);

	return size;
}

static ssize_t k303c_acc_raw_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct k303c_acc_v acc;
	struct k303c_acc_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == OFF) {
		k303c_acc_set_mode(data, K303C_MODE_NORMAL);
		k303c_acc_read_accel_xyz(data, &acc);
		k303c_acc_set_mode(data, K303C_MODE_SUSPEND);

		acc.x = acc.x - data->caldata.x;
		acc.y = acc.y - data->caldata.y;
		acc.z = acc.z - data->caldata.z;
	} else {
		acc = data->accdata;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", acc.x, acc.y, acc.z);
}

static ssize_t k303c_acc_reactive_alert_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct k303c_acc_p *data = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->irq_state);
}

static ssize_t k303c_acc_reactive_alert_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned char threshx, threshy, threshz;
	int enable = OFF, factory_mode = OFF;
	struct k303c_acc_v acc;
	struct k303c_acc_p *data = dev_get_drvdata(dev);

	pr_info("%s\n", __func__);

	if (sysfs_streq(buf, "0")) {
		enable = OFF;
		factory_mode = OFF;
		pr_info("%s - disable\n", __func__);
	} else if (sysfs_streq(buf, "1")) {
		enable = ON;
		factory_mode = OFF;
		pr_info("%s - enable\n", __func__);
	} else if (sysfs_streq(buf, "2")) {
		enable = ON;
		factory_mode = ON;
		pr_info("%s - factory mode\n", __func__);
	} else {
		pr_err("%s - invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if ((enable == ON) && (data->recog_flag == OFF)) {
		data->irq_state = 0;
		data->recog_flag = ON;

		if (factory_mode == ON) {
			k303c_acc_i2c_write(data, INT_THSX1_REG, 0x00);
			k303c_acc_i2c_write(data, INT_THSY1_REG, 0x00);
			k303c_acc_i2c_write(data, INT_THSZ1_REG, 0x00);
			k303c_acc_i2c_write(data, INT_CFG1_REG, 0x3f);
		} else {
			k303c_acc_set_odr(data);
			if (atomic_read(&data->enable) == OFF) {
				k303c_acc_set_mode(data, K303C_MODE_NORMAL);
				k303c_acc_read_accel_xyz(data, &acc);
				k303c_acc_set_mode(data, K303C_MODE_SUSPEND);
			} else {
				acc.x = data->accdata.x;
				acc.y = data->accdata.y;
				acc.z = data->accdata.z;
			}

			threshx = (abs(acc.v[data->axis_map_x])
					+ DYNAMIC_THRESHOLD) >> 8;
			threshy = (abs(acc.v[data->axis_map_y])
					+ DYNAMIC_THRESHOLD) >> 8;
			threshz = (abs(acc.v[data->axis_map_z])
					+ DYNAMIC_THRESHOLD) >> 8;

			k303c_acc_i2c_write(data, INT_THSX1_REG, threshx);
			k303c_acc_i2c_write(data, INT_THSY1_REG, threshy);
			k303c_acc_i2c_write(data, INT_THSZ1_REG, threshz);
			k303c_acc_i2c_write(data, INT_CFG1_REG, 0x0a);
		}

		k303c_acc_i2c_write(data, CTRL7_REG, CTRL7_LIR1);
		k303c_acc_i2c_write(data, CTRL3_REG, CTRL3_IG1_INT1);

		enable_irq(data->irq1);
		enable_irq_wake(data->irq1);

		pr_info("%s - reactive alert is on!\n", __func__);
	} else if ((enable == OFF) && (data->recog_flag == ON)) {
		k303c_acc_i2c_write(data, CTRL3_REG, 0x00);

		disable_irq_wake(data->irq1);
		disable_irq_nosync(data->irq1);
		data->recog_flag = OFF;
		pr_info("%s - reactive alert is off! irq = %d\n",
			__func__, data->irq_state);
	}

	return size;
}

static ssize_t k303c_acc_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct k303c_acc_p *data = dev_get_drvdata(dev);
	struct k303c_acc_v acc;
	unsigned char temp;
	int result = 1, i;
	ssize_t ret;
	s32 NO_ST[3] = {0, 0, 0};
	s32 ST[3] = {0, 0, 0};

	if (atomic_read(&data->enable) == OFF)
		k303c_acc_set_mode(data, K303C_MODE_NORMAL);
	else
		k303c_acc_set_enable(data, OFF);

	k303c_acc_i2c_write(data, CTRL1_REG, 0x3f);
	k303c_acc_i2c_write(data, CTRL4_REG, 0x04);
	k303c_acc_i2c_write(data, CTRL5_REG, 0x00);
	k303c_acc_i2c_write(data, CTRL6_REG, 0x00);

	msleep(80);

	k303c_acc_read_accel_xyz(data, &acc);

	for (i = 0; i < 5; i++) {
		while (1) {
			if (k303c_acc_i2c_read(data, STATUS_REG, &temp, 1)
				< 0) {
				pr_err("%s: i2c error", __func__);
				goto exit_status_err;
			}

			if (temp & 0x08)
				break;
		}

		k303c_acc_read_accel_xyz(data, &acc);
		NO_ST[0] += acc.x;
		NO_ST[1] += acc.y;
		NO_ST[2] += acc.z;
	}
	NO_ST[0]  /= 5;
	NO_ST[1]  /= 5;
	NO_ST[2]  /= 5;

	k303c_acc_i2c_write(data, CTRL5_REG, 0x04);

	msleep(80);

	k303c_acc_read_accel_xyz(data, &acc);

	for (i = 0; i < 5; i++) {
		while (1) {
			if (k303c_acc_i2c_read(data, STATUS_REG, &temp, 1)
				< 0) {
				pr_err("%s: i2c error", __func__);
				goto exit_status_err;
			}

			if (temp & 0x08)
				break;
		}

		k303c_acc_read_accel_xyz(data, &acc);
		ST[0] += acc.x;
		ST[1] += acc.y;
		ST[2] += acc.z;
	}

	ST[0] /= 5;
	ST[1] /= 5;
	ST[2] /= 5;

	for (i = 0; i < 3; i++) {
		ST[i] -= NO_ST[i];
		ST[i] = abs(ST[i]);

		if ((SELF_TEST_2G_MIN_LSB > ST[i])
			|| (ST[i] > SELF_TEST_2G_MAX_LSB)) {
			pr_info("%s: %d Out of range!! (%d)\n",
				__func__, i, ST[i]);
			result = 0;
		}
	}

	if (result)
		ret = snprintf(buf, 0xff, "1,%d,%d,%d\n", ST[0], ST[1], ST[2]);
	else
		ret = snprintf(buf, 0xff, "0,%d,%d,%d\n", ST[0], ST[1], ST[2]);

	goto exit;

exit_status_err:
	ret = snprintf(buf, 0xff, "-1,0,0,0\n");
exit:
	k303c_acc_i2c_write(data, CTRL1_REG, 0x00);
	k303c_acc_i2c_write(data, CTRL5_REG, 0x00);

	if (atomic_read(&data->enable) == OFF) {
		k303c_acc_set_mode(data, K303C_MODE_SUSPEND);
	} else {
		k303c_acc_set_mode(data, K303C_MODE_NORMAL);
		k303c_acc_set_enable(data, ON);
	}

	pr_info("%s %s\n", __func__, buf);

	return ret;
}

static DEVICE_ATTR(name, S_IRUGO, k303c_acc_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, k303c_acc_vendor_show, NULL);
static DEVICE_ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
	k303c_acc_calibration_show, k303c_acc_calibration_store);
static DEVICE_ATTR(raw_data, S_IRUGO, k303c_acc_raw_data_read, NULL);
static DEVICE_ATTR(reactive_alert, S_IRUGO | S_IWUSR | S_IWGRP,
	k303c_acc_reactive_alert_show, k303c_acc_reactive_alert_store);
static DEVICE_ATTR(selftest, S_IRUGO, k303c_acc_selftest_show, NULL);

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_calibration,
	&dev_attr_raw_data,
	&dev_attr_reactive_alert,
	&dev_attr_selftest,
	NULL,
};

static enum hrtimer_restart k303c_acc_timer_func(struct hrtimer *timer)
{
	struct k303c_acc_p *data = container_of(timer, struct k303c_acc_p,
		accel_timer);

	if (!work_pending(&data->work))
		queue_work(data->accel_wq, &data->work);

	hrtimer_forward_now(&data->accel_timer, data->poll_delay);
	return HRTIMER_RESTART;
}

static void k303c_acc_work_func(struct work_struct *work)
{
	int ret;
	struct k303c_acc_v acc;
	struct k303c_acc_p *data = container_of(work, struct k303c_acc_p,
		work);

	ret = k303c_acc_read_accel_xyz(data, &acc);
	if (ret < 0)
		goto exit;

	data->accdata.x = acc.x - data->caldata.x;
	data->accdata.y = acc.y - data->caldata.y;
	data->accdata.z = acc.z - data->caldata.z;

	input_report_rel(data->input, REL_X, data->accdata.x);
	input_report_rel(data->input, REL_Y, data->accdata.y);
	input_report_rel(data->input, REL_Z, data->accdata.z);
	input_sync(data->input);

exit:
	if ((ktime_to_ns(data->poll_delay) * (int64_t)data->time_count)
		>= ((int64_t)ACCEL_LOG_TIME * NSEC_PER_SEC)) {
		pr_info("%s - x = %d, y = %d, z = %d (ra:%d)\n", __func__,
			data->accdata.x, data->accdata.y, data->accdata.z,
			data->recog_flag);
		data->time_count = 0;
	} else
		data->time_count++;
}

static void k303c_acc_irq_work_func(struct work_struct *work)
{
	struct k303c_acc_p *data = container_of((struct delayed_work *)work,
		struct k303c_acc_p, irq_work);
	unsigned char buf;

	pr_info("%s\n", __func__);

	k303c_acc_i2c_write(data, INT_CFG1_REG, 0x00);
	k303c_acc_i2c_read(data, INT_SRC1_REG, &buf, 1);
}

static irqreturn_t k303c_acc_irq_thread(int irq, void *k303c_acc_data_p)
{
	struct k303c_acc_p *data = k303c_acc_data_p;

	pr_info("%s\n", __func__);

	data->irq_state = 1;
	wake_lock_timeout(&data->reactive_wake_lock, msecs_to_jiffies(2000));
	schedule_delayed_work(&data->irq_work, msecs_to_jiffies(100));

	return IRQ_HANDLED;
}

static int k303c_acc_parse_dt(struct k303c_acc_p *data, struct device *dev)
{
	struct device_node *dev_node = dev->of_node;
	enum of_gpio_flags flags;
	int ret;
	u32 temp;

	if (dev_node == NULL)
		return -ENODEV;

	data->acc_int1 = of_get_named_gpio_flags(dev_node,
			"k303c_acc,gpio_int1", 0, &flags);
	if (data->acc_int1 < 0) {
		pr_err("%s - get acc_int1 error\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(dev_node, "k303c_acc,axis_map_x", &temp);
	if ((data->axis_map_x > 2) || (ret < 0)) {
		pr_err("%s: invalid x axis_map value %u\n",
			__func__, data->axis_map_x);
		data->axis_map_x = 0;
	} else {
		data->axis_map_x = (u8)temp;
	}

	ret = of_property_read_u32(dev_node, "k303c_acc,axis_map_y", &temp);
	if ((data->axis_map_y > 2) || (ret < 0)) {
		pr_err("%s: invalid y axis_map value %u\n",
			__func__, data->axis_map_y);
		data->axis_map_y = 1;
	} else {
		data->axis_map_y = (u8)temp;
	}

	ret = of_property_read_u32(dev_node, "k303c_acc,axis_map_z", &temp);
	if ((data->axis_map_z > 2) || (ret < 0)) {
		pr_err("%s: invalid z axis_map value %u\n",
			__func__, data->axis_map_z);
		data->axis_map_z = 2;
	} else {
		data->axis_map_z = (u8)temp;
	}

	ret = of_property_read_u32(dev_node, "k303c_acc,negate_x", &temp);
	if ((data->negate_x > 1) || (ret < 0)) {
		pr_err("%s: invalid x axis_map value %u\n",
			__func__, data->negate_x);
		data->negate_x = 0;
	} else {
		data->negate_x = (u8)temp;
	}

	ret = of_property_read_u32(dev_node, "k303c_acc,negate_y", &temp);
	if ((data->negate_y > 1) || (ret < 0)) {
		pr_err("%s: invalid y axis_map value %u\n",
			__func__, data->negate_y);
		data->negate_y = 0;
	} else {
		data->negate_y = (u8)temp;
	}

	ret = of_property_read_u32(dev_node, "k303c_acc,negate_z", &temp);
	if ((data->negate_z > 1) || (ret < 0)) {
		pr_err("%s: invalid z axis_map value %u\n",
			__func__, data->negate_z);
		data->negate_z = 0;
	} else {
		data->negate_z = (u8)temp;
	}

	data->reg_vdd = devm_regulator_get(dev, "k303c_acc,vdd");
	if (IS_ERR(data->reg_vdd)) {
		pr_err("could not get vdd, %ld\n", PTR_ERR(data->reg_vdd));
	} else {
		if (!((AVG_VDD <= regulator_get_voltage(data->reg_vdd))
			&& (regulator_get_voltage(data->reg_vdd) <= MAX_VDD)))
			ret = regulator_set_voltage(data->reg_vdd,
				AVG_VDD, AVG_VDD);
		devm_regulator_put(data->reg_vdd);
		if (ret) {
			pr_err("%s: set voltage failed on vdd, rc=%d\n",
				__func__, ret);
			return ret;
		}
	}

	data->reg_vio = devm_regulator_get(dev, "k303c_acc,vio");
	if (IS_ERR(data->reg_vio)) {
		pr_err("%s, could not get vio, %ld\n",
			__func__, PTR_ERR(data->reg_vio));
	} else {
		ret = regulator_set_voltage(data->reg_vio, AVG_VIO, AVG_VIO);
		if (ret) {
			pr_err("%s: set voltage failed on vio rc=%d\n",
				__func__, ret);
			return ret;
		}
	}


	return 0;
}

static int k303c_acc_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	u8 temp;
	int ret = -ENODEV, i;
	struct k303c_acc_p *data = NULL;

	pr_info("[SENSOR] %s, Probe Start!\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s - i2c_check_functionality error\n",
			__func__);
		goto err_i2c_check_functionality;
	}

	data = kzalloc(sizeof(struct k303c_acc_p), GFP_KERNEL);
	if (data == NULL) {
		pr_err("%s - kzalloc error\n", __func__);
		ret = -ENOMEM;
		goto err_kzalloc;
	}

	i2c_set_clientdata(client, data);
	data->client = client;


	ret = k303c_acc_parse_dt(data, &client->dev);
	if (ret < 0) {
		pr_err("%s - of_node error\n", __func__);
		ret = -ENODEV;
		goto err_of_node;
	}

	ret = k303c_regulator_onoff(data, true);
	if (ret < 0) {
		pr_err("%s - No regulator\n", __func__);
		goto err_regulator_onoff;
	}

	ret = gpio_request(data->acc_int1, "ACCEL_nINT");
	if (ret < 0) {
		pr_err("%s - gpio %d request failed (%d)\n",
			__func__, data->acc_int1, ret);
		goto err_setup_pin;
	}

	ret = gpio_direction_input(data->acc_int1);
	if (ret < 0) {
		pr_err("%s - failed to set gpio %d as input (%d)\n",
			__func__, data->acc_int1, ret);
		goto err_acc_int1;
	}

	wake_lock_init(&data->reactive_wake_lock, WAKE_LOCK_SUSPEND,
		       "reactive_wake_lock");

	data->irq1 = gpio_to_irq(data->acc_int1);
	ret = request_threaded_irq(data->irq1, NULL, k303c_acc_irq_thread,
		IRQF_TRIGGER_RISING | IRQF_ONESHOT | IRQF_NO_SUSPEND,
		MODEL_NAME, data);
	if (ret < 0) {
		pr_err("%s - can't allocate irq.\n", __func__);
		goto err_reactive_irq;
	}

	disable_irq(data->irq1);

	mutex_init(&data->mode_mutex);

	k303c_acc_set_mode(data, K303C_MODE_NORMAL);
	for (i = 0; i < CHIP_ID_RETRIES; i++) {
		ret = k303c_acc_i2c_read(data, WHOAMI_REG, &temp, 1);
		if (temp != K303C_CHIP_ID) {
			pr_err("%s - chip id failed 0x%x : %d\n",
				__func__, temp, ret);
		} else {
			pr_info("%s - chip id success 0x%x\n",
				__func__, temp);
			break;
		}
		msleep(20);
	}

	if (i >= CHIP_ID_RETRIES) {
		ret = -ENODEV;
		goto err_read_chipid;
	}


	data->input = input_allocate_device();
	if (!data->input) {
		ret = -ENOMEM;
		pr_err("%s failed input_allocate_device\n", __func__);
		goto err_input_allocate_device;
	}
	data->input->name = MODULE_NAME;
	data->input->id.bustype = BUS_I2C;

	input_set_capability(data->input, EV_REL, REL_X);
	input_set_capability(data->input, EV_REL, REL_Y);
	input_set_capability(data->input, EV_REL, REL_Z);
	input_set_drvdata(data->input, data);

	ret = input_register_device(data->input);
	if (ret < 0) {
		pr_err("%s failed input_register_device\n", __func__);
		goto err_input_register_device;
	}

	ret = sensors_create_symlink(&data->input->dev.kobj, data->input->name);
	if (ret < 0) {
		pr_err("%s failed sensors_create_symlink\n", __func__);
		goto err_sensors_create_symlink;
	}

	ret = sysfs_create_group(&data->input->dev.kobj,
		&k303c_acc_attribute_group);
	if (ret < 0) {
		pr_err("%s failed sysfs_create_group\n", __func__);
		goto err_sysfs_create_group;
	}

	ret = sensors_register(data->factory_device, data, sensor_attrs,
		MODULE_NAME);
	if (ret < 0) {
		pr_err("%s failed sensors_register\n", __func__);
		goto err_sensors_register;
	}

	/* accel_timer settings. we poll for light values using a timer. */
	hrtimer_init(&data->accel_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->poll_delay = ns_to_ktime(K303C_DEFAULT_DELAY);
	data->accel_timer.function = k303c_acc_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	data->accel_wq = create_singlethread_workqueue("accel_wq");
	if (!data->accel_wq) {
		ret = -ENOMEM;
		pr_err("%s - could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&data->work, k303c_acc_work_func);
	INIT_DELAYED_WORK(&data->irq_work, k303c_acc_irq_work_func);

	atomic_set(&data->enable, OFF);
	data->time_count = 0;
	data->irq_state = 0;
	data->recog_flag = OFF;

	k303c_acc_set_range(data, K303C_RANGE_2G);
	k303c_acc_set_mode(data, K303C_MODE_SUSPEND);
	k303c_acc_i2c_write(data, CTRL2_REG, 0x20);

	pr_err("[SENSOR] %s, Probe done!\n", __func__);

	return 0;

	cancel_delayed_work_sync(&data->irq_work);
err_create_workqueue:
	sensors_unregister(data->factory_device, sensor_attrs);
err_sensors_register:
	sysfs_remove_group(&data->input->dev.kobj, &k303c_acc_attribute_group);
err_sysfs_create_group:
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);
err_sensors_create_symlink:
	input_unregister_device(data->input);
err_input_register_device:
	input_free_device(data->input);
err_input_allocate_device:
err_read_chipid:
	mutex_destroy(&data->mode_mutex);
	free_irq(data->irq1, data);
err_reactive_irq:
	wake_lock_destroy(&data->reactive_wake_lock);
err_acc_int1:
	gpio_free(data->acc_int1);
err_setup_pin:
err_regulator_onoff:
err_of_node:
	kfree(data);
err_kzalloc:
err_i2c_check_functionality:
	pr_err("[SENSOR] %s, Probe fail!\n", __func__);
	return ret;
}

static int k303c_acc_remove(struct i2c_client *client)
{
	struct k303c_acc_p *data = (struct k303c_acc_p *)
		i2c_get_clientdata(client);

	pr_info("%s\n", __func__);

	if (atomic_read(&data->enable) == ON) {
		k303c_acc_set_mode(data, K303C_MODE_SUSPEND);
		k303c_acc_set_enable(data, OFF);
		atomic_set(&data->enable, OFF);
	}

	cancel_delayed_work_sync(&data->irq_work);
	sensors_unregister(data->factory_device, sensor_attrs);
	sysfs_remove_group(&data->input->dev.kobj, &k303c_acc_attribute_group);
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);
	input_unregister_device(data->input);
	input_free_device(data->input);
	mutex_destroy(&data->mode_mutex);
	free_irq(data->irq1, data);
	wake_lock_destroy(&data->reactive_wake_lock);
	gpio_free(data->acc_int1);
	kfree(data);

	return 0;
}

static int k303c_acc_suspend(struct device *dev)
{
	struct k303c_acc_p *data = dev_get_drvdata(dev);

	pr_info("%s en:%d, ra:%d\n", __func__, atomic_read(&data->enable),
		data->recog_flag);

	if (atomic_read(&data->enable) == ON) {
		k303c_acc_set_mode(data, K303C_MODE_SUSPEND);
		k303c_acc_set_enable(data, OFF);
	}
	return 0;
}

static int k303c_acc_resume(struct device *dev)
{
	struct k303c_acc_p *data = dev_get_drvdata(dev);

	pr_info("%s en:%d, ra:%d\n", __func__, atomic_read(&data->enable),
		data->recog_flag);

	if (atomic_read(&data->enable) == ON) {
		k303c_acc_set_mode(data, K303C_MODE_NORMAL);
		k303c_acc_set_enable(data, ON);
	}
	return 0;
}

static struct of_device_id k303c_acc_table[] = {
	{ .compatible = "k303c_acc",},
	{},
};

static const struct i2c_device_id k303c_acc_id[] = {
	{ "k303c_acc_table", 0 },
	{ }
};

static const struct dev_pm_ops k303c_acc_pm_ops = {
	.suspend = k303c_acc_suspend,
	.resume = k303c_acc_resume
};

static struct i2c_driver k303c_acc_driver = {
	.driver = {
		.name	= MODEL_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = k303c_acc_table,
		.pm = &k303c_acc_pm_ops
	},
	.probe		= k303c_acc_probe,
	.remove		= k303c_acc_remove,
	.id_table	= k303c_acc_id,
};

static int __init k303c_acc_init(void)
{
	return i2c_add_driver(&k303c_acc_driver);
}

static void __exit k303c_acc_exit(void)
{
	i2c_del_driver(&k303c_acc_driver);
}

module_init(k303c_acc_init);
module_exit(k303c_acc_exit);

MODULE_DESCRIPTION("K303C accelerometer sensor driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
