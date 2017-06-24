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
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>

#include "sensors_core.h"
#include "bmg160_reg.h"

#define VENDOR_NAME                        "BOSCH"
#define MODEL_NAME                         "BMG160"
#define MODULE_NAME                        "gyro_sensor"

#define I2C_M_WR                           0 /* for i2c Write */
#define I2c_M_RD                           1 /* for i2c Read */
#define READ_DATA_LENTH                    6

#define CALIBRATION_FILE_PATH              "/efs/gyro_calibration_data"
#define CALIBRATION_DATA_AMOUNT            20
#define SELFTEST_DATA_AMOUNT               64
#define SELFTEST_LIMITATION_OF_ERROR       5250

#define BMG160_DEFAULT_DELAY               200000000LL
#define	BMG160_CHIP_ID                     0x0F

#define BMG160_TOP_UPPER_RIGHT             0
#define BMG160_TOP_LOWER_RIGHT             1
#define BMG160_TOP_LOWER_LEFT              2
#define BMG160_TOP_UPPER_LEFT              3
#define BMG160_BOTTOM_UPPER_RIGHT          4
#define BMG160_BOTTOM_LOWER_RIGHT          5
#define BMG160_BOTTOM_LOWER_LEFT           6
#define BMG160_BOTTOM_UPPER_LEFT           7

struct bmg160_v {
	union {
		s16 v[3];
		struct {
			s16 x;
			s16 y;
			s16 z;
		};
	};
};

struct bmg160_p {
	struct i2c_client *client;
	struct input_dev *input;
	struct device *factory_device;
	struct bmg160_v gyrodata;
	struct bmg160_v caldata;
	struct work_struct work;
	struct hrtimer gyro_timer;
	struct workqueue_struct *gyro_wq;
	ktime_t poll_delay;
	atomic_t enable;

	u32 chip_pos;
	int gyro_dps;
	int gyro_int;
	int gyro_drdy;
	u64 timestamp;
        u64 old_timestamp;
};

static int bmg160_open_calibration(struct bmg160_p *);

static int bmg160_i2c_read(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *buf)
{
	int ret;
	struct i2c_msg msg[2];

	msg[0].addr = client->addr;
	msg[0].flags = I2C_M_WR;
	msg[0].len = 1;
	msg[0].buf = &reg_addr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - i2c read error %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int bmg160_i2c_write(struct i2c_client *client,
		unsigned char reg_addr, unsigned char buf)
{
	int ret;
	struct i2c_msg msg;
	unsigned char w_buf[2];

	w_buf[0] = reg_addr;
	w_buf[1] = buf;

	msg.addr = client->addr;
	msg.flags = I2C_M_WR;
	msg.len = 2;
	msg.buf = (char *)w_buf;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - i2c write error %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int bmg160_get_bw(struct bmg160_p *data, unsigned char *bandwidth)
{
	int ret;
	unsigned char temp;

	ret = bmg160_i2c_read(data->client, BMG160_BW_ADDR__REG, &temp);
	*bandwidth = BMG160_GET_BITSLICE(temp, BMG160_BW_ADDR);

	return ret;
}

static int bmg160_get_autosleepdur(struct bmg160_p *data,
		unsigned char *duration)
{
	int ret = 0;
	unsigned char temp;

	ret = bmg160_i2c_read(data->client,
			BMG160_MODE_LPM2_ADDR_AUTOSLEEPDUR__REG, &temp);

	*duration = BMG160_GET_BITSLICE(temp,
			BMG160_MODE_LPM2_ADDR_AUTOSLEEPDUR);

	return ret;
}

static int bmg160_set_autosleepdur(struct bmg160_p *data,
		unsigned char duration, unsigned char bandwith)
{
	int ret = 0;
	unsigned char temp, autosleepduration;

	ret = bmg160_i2c_read(data->client,
			BMG160_MODE_LPM2_ADDR_AUTOSLEEPDUR__REG, &temp);

	switch (bandwith) {
	case BMG160_No_Filter:
		if (duration > BMG160_4ms_AutoSleepDur)
			autosleepduration = duration;
		else
			autosleepduration = BMG160_4ms_AutoSleepDur;
		break;
	case BMG160_BW_230Hz:
		if (duration > BMG160_4ms_AutoSleepDur)
			autosleepduration = duration;
		else
			autosleepduration = BMG160_4ms_AutoSleepDur;
		break;
	case BMG160_BW_116Hz:
		if (duration > BMG160_4ms_AutoSleepDur)
			autosleepduration = duration;
		else
			autosleepduration = BMG160_4ms_AutoSleepDur;
		break;
	case BMG160_BW_47Hz:
		if (duration > BMG160_5ms_AutoSleepDur)
			autosleepduration = duration;
		else
			autosleepduration = BMG160_5ms_AutoSleepDur;
		break;
	case BMG160_BW_23Hz:
		if (duration > BMG160_10ms_AutoSleepDur)
			autosleepduration = duration;
		else
		autosleepduration = BMG160_10ms_AutoSleepDur;
		break;
	case BMG160_BW_12Hz:
		if (duration > BMG160_20ms_AutoSleepDur)
			autosleepduration = duration;
		else
		autosleepduration = BMG160_20ms_AutoSleepDur;
		break;
	case BMG160_BW_64Hz:
		if (duration > BMG160_10ms_AutoSleepDur)
			autosleepduration = duration;
		else
			autosleepduration = BMG160_10ms_AutoSleepDur;
		break;
	case BMG160_BW_32Hz:
		if (duration > BMG160_20ms_AutoSleepDur)
			autosleepduration = duration;
		else
			autosleepduration = BMG160_20ms_AutoSleepDur;
		break;
	default:
		autosleepduration = BMG160_No_AutoSleepDur;
	}

	temp = BMG160_SET_BITSLICE(temp, BMG160_MODE_LPM2_ADDR_AUTOSLEEPDUR,
			autosleepduration);

	ret += bmg160_i2c_write(data->client,
			BMG160_MODE_LPM2_ADDR_AUTOSLEEPDUR__REG, temp);

	return ret;
}

static int bmg160_get_mode(struct bmg160_p *data, unsigned char *mode)
{
	int ret = 0;
	unsigned char buf1 = 0;
	unsigned char buf2 = 0;
	unsigned char buf3 = 0;

	ret = bmg160_i2c_read(data->client, BMG160_MODE_LPM1_ADDR, &buf1);
	ret += bmg160_i2c_read(data->client, BMG160_MODE_LPM2_ADDR, &buf2);

	buf1  = (buf1 & 0xA0) >> 5;
	buf3  = (buf2 & 0x40) >> 6;
	buf2  = (buf2 & 0x80) >> 7;

	if (buf3 == 0x01)
		*mode = BMG160_MODE_ADVANCEDPOWERSAVING;
	else if ((buf1 == 0x00) && (buf2 == 0x00))
		*mode = BMG160_MODE_NORMAL;
	else if ((buf1 == 0x01) || (buf1 == 0x05))
		*mode = BMG160_MODE_DEEPSUSPEND;
	else if ((buf1 == 0x04) && (buf2 == 0x00))
		*mode = BMG160_MODE_SUSPEND;
	else if ((buf1 == 0x04) && (buf2 == 0x01))
		*mode = BMG160_MODE_FASTPOWERUP;

	return ret;
}

static int bmg160_set_range(struct bmg160_p *data, unsigned char range)
{
	int ret = 0;
	unsigned char temp;

	ret = bmg160_i2c_read(data->client,
			BMG160_RANGE_ADDR_RANGE__REG, &temp);
	temp = BMG160_SET_BITSLICE(temp, BMG160_RANGE_ADDR_RANGE, range);
	ret += bmg160_i2c_write(data->client,
			BMG160_RANGE_ADDR_RANGE__REG, temp);

	return ret;
}

static int bmg160_set_bw(struct bmg160_p *data, unsigned char bandwidth)
{
	int ret = 0;
	unsigned char temp, autosleepduration, mode = 0;

	bmg160_get_mode(data, &mode);
	if (mode == BMG160_MODE_ADVANCEDPOWERSAVING) {
		bmg160_get_autosleepdur(data, &autosleepduration);
		bmg160_set_autosleepdur(data, autosleepduration, bandwidth);
	}

	ret = bmg160_i2c_read(data->client, BMG160_BW_ADDR__REG, &temp);
	temp = BMG160_SET_BITSLICE(temp, BMG160_BW_ADDR, bandwidth);
	ret += bmg160_i2c_write(data->client, BMG160_BW_ADDR__REG, temp);

	pr_info("[SENSOR]: %s - bandwidth = %u, ret = %d\n", __func__, bandwidth, ret);
	return ret;
}

static int bmg160_set_mode(struct bmg160_p *data, unsigned char mode)
{
	int ret = 0;
	unsigned char buf1, buf2, buf3;
	unsigned char autosleepduration;
	unsigned char v_bw_u8r;

	ret = bmg160_i2c_read(data->client, BMG160_MODE_LPM1_ADDR, &buf1);
	ret += bmg160_i2c_read(data->client, BMG160_MODE_LPM2_ADDR, &buf2);

	switch (mode) {
	case BMG160_MODE_NORMAL:
		buf1 = BMG160_SET_BITSLICE(buf1, BMG160_MODE_LPM1, 0);
		buf2 = BMG160_SET_BITSLICE(buf2,
			BMG160_MODE_LPM2_ADDR_FAST_POWERUP, 0);
		buf3 = BMG160_SET_BITSLICE(buf2,
			BMG160_MODE_LPM2_ADDR_ADV_POWERSAVING, 0);
		ret += bmg160_i2c_write(data->client,
				BMG160_MODE_LPM1_ADDR, buf1);
		mdelay(1);
		ret += bmg160_i2c_write(data->client,
				BMG160_MODE_LPM2_ADDR, buf3);
		break;
	case BMG160_MODE_DEEPSUSPEND:
		buf1 = BMG160_SET_BITSLICE(buf1, BMG160_MODE_LPM1, 1);
		buf2 = BMG160_SET_BITSLICE(buf2,
			BMG160_MODE_LPM2_ADDR_FAST_POWERUP, 0);
		buf3 = BMG160_SET_BITSLICE(buf2,
			BMG160_MODE_LPM2_ADDR_ADV_POWERSAVING, 0);
		ret += bmg160_i2c_write(data->client,
				BMG160_MODE_LPM1_ADDR, buf1);
		mdelay(1);
		ret += bmg160_i2c_write(data->client,
				BMG160_MODE_LPM2_ADDR, buf3);
		break;
	case BMG160_MODE_SUSPEND:
		buf1 = BMG160_SET_BITSLICE(buf1, BMG160_MODE_LPM1, 4);
		buf2 = BMG160_SET_BITSLICE(buf2,
			BMG160_MODE_LPM2_ADDR_FAST_POWERUP, 0);
		buf3 = BMG160_SET_BITSLICE(buf2,
			BMG160_MODE_LPM2_ADDR_ADV_POWERSAVING, 0);
		ret += bmg160_i2c_write(data->client,
				BMG160_MODE_LPM1_ADDR, buf1);
		mdelay(1);
		ret += bmg160_i2c_write(data->client,
				BMG160_MODE_LPM2_ADDR, buf3);
		break;
	case BMG160_MODE_FASTPOWERUP:
		buf1 = BMG160_SET_BITSLICE(buf1, BMG160_MODE_LPM1, 4);
		buf2 = BMG160_SET_BITSLICE(buf2,
			BMG160_MODE_LPM2_ADDR_FAST_POWERUP, 1);
		buf3 = BMG160_SET_BITSLICE(buf2,
			BMG160_MODE_LPM2_ADDR_ADV_POWERSAVING, 0);
		ret += bmg160_i2c_write(data->client,
				BMG160_MODE_LPM1_ADDR, buf1);
		mdelay(1);
		ret += bmg160_i2c_write(data->client,
				BMG160_MODE_LPM2_ADDR, buf3);
		break;
	case BMG160_MODE_ADVANCEDPOWERSAVING:
		/* Configuring the proper settings for auto
		sleep duration */
		bmg160_get_bw(data, &v_bw_u8r);
		bmg160_get_autosleepdur(data, &autosleepduration);
		bmg160_set_autosleepdur(data, autosleepduration, v_bw_u8r);
		ret += bmg160_i2c_read(data->client,
				BMG160_MODE_LPM2_ADDR, &buf2);
		/* Configuring the advanced power saving mode*/
		buf1 = BMG160_SET_BITSLICE(buf1, BMG160_MODE_LPM1, 0);
		buf2 = BMG160_SET_BITSLICE(buf2,
			BMG160_MODE_LPM2_ADDR_FAST_POWERUP, 0);
		buf3 = BMG160_SET_BITSLICE(buf2,
			BMG160_MODE_LPM2_ADDR_ADV_POWERSAVING, 1);
		ret += bmg160_i2c_write(data->client,
				BMG160_MODE_LPM1_ADDR, buf1);
		mdelay(1);
		ret += bmg160_i2c_write(data->client,
				BMG160_MODE_LPM2_ADDR, buf3);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	pr_info("[SENSOR]: %s - mode = %u, ret = %d\n", __func__, mode, ret);
	return ret;
}

static int bmg160_read_gyro_xyz(struct bmg160_p *data, struct bmg160_v *gyro)
{
	int ret = 0, i;
	unsigned char temp[READ_DATA_LENTH];

#ifdef CONFIG_SENSORS_BMI058
	for (i = 0; i < READ_DATA_LENTH; i++) {
		ret += bmg160_i2c_read(data->client,
				BMG160_RATE_Y_LSB_VALUEY__REG + i, &temp[i]);
	}

	temp[0] = BMG160_GET_BITSLICE(temp[0], BMG160_RATE_Y_LSB_VALUEY);
	gyro->y = (short)((((short)((signed char)temp[1])) << 8) | (temp[0]));

	temp[2] = BMG160_GET_BITSLICE(temp[2], BMG160_RATE_X_LSB_VALUEX);
	gyro->x = (short)((((short)((signed char)temp[3])) << 8) | (temp[2]));
#else
	for (i = 0; i < READ_DATA_LENTH; i++) {
		ret += bmg160_i2c_read(data->client,
				BMG160_RATE_X_LSB_VALUEX__REG + i, &temp[i]);
	}

	temp[0] = BMG160_GET_BITSLICE(temp[0], BMG160_RATE_X_LSB_VALUEX);
	gyro->x = (short)((((short)((signed char)temp[1])) << 8) | (temp[0]));

	temp[2] = BMG160_GET_BITSLICE(temp[2], BMG160_RATE_Y_LSB_VALUEY);
	gyro->y = (short)((((short)((signed char)temp[3])) << 8) | (temp[2]));
#endif

	temp[4] = BMG160_GET_BITSLICE(temp[4], BMG160_RATE_Z_LSB_VALUEZ);
	gyro->z = (short)((((short)((signed char)temp[5])) << 8) | (temp[4]));

	remap_sensor_data(gyro->v, data->chip_pos);

	if (data->gyro_dps == BMG160_RANGE_250DPS) {
		gyro->x = gyro->x >> 1;
		gyro->y = gyro->y >> 1;
		gyro->z = gyro->z >> 1;
	} else if (data->gyro_dps == BMG160_RANGE_2000DPS) {
		gyro->x = gyro->x << 2;
		gyro->y = gyro->y << 2;
		gyro->z = gyro->z << 2;
	}

	return ret;
}

static enum hrtimer_restart bmg160_timer_func(struct hrtimer *timer)
{
	struct bmg160_p *data = container_of(timer,
					struct bmg160_p, gyro_timer);

	if (!work_pending(&data->work))
		queue_work(data->gyro_wq, &data->work);

	hrtimer_forward_now(&data->gyro_timer, data->poll_delay);

	return HRTIMER_RESTART;
}

static void bmg160_work_func(struct work_struct *work)
{
	int ret;
	struct bmg160_v gyro;
	struct bmg160_p *data = container_of(work, struct bmg160_p, work);
	struct timespec ts;
	int time_hi, time_lo;
	u64 delay = ktime_to_ns(data->poll_delay);

	ts = ktime_to_timespec(alarm_get_elapsed_realtime());
	data->timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

	ret = bmg160_read_gyro_xyz(data, &gyro);
	if (ret < 0)
		return;

	if (data->old_timestamp != 0 &&
	   ((data->timestamp - data->old_timestamp) > ktime_to_ms(data->poll_delay) * 1800000LL)) {

		u64 shift_timestamp = delay >> 1;
		u64 timestamp = 0ULL;
		//u64 diff = 0ULL;

		for (timestamp = data->old_timestamp + delay; timestamp < data->timestamp - shift_timestamp; timestamp+=delay) {
				time_hi = (int)((timestamp & TIME_HI_MASK) >> TIME_HI_SHIFT);
				time_lo = (int)(timestamp & TIME_LO_MASK);

				input_report_rel(data->input, REL_RX, gyro.x - data->caldata.x);
				input_report_rel(data->input, REL_RY, gyro.y - data->caldata.y);
				input_report_rel(data->input, REL_RZ, gyro.z - data->caldata.z);
				input_report_rel(data->input, REL_X, time_hi);
				input_report_rel(data->input, REL_Y, time_lo);
				input_sync(data->input);
				data->gyrodata = gyro;
		}
	}

	time_hi = (int)((data->timestamp & TIME_HI_MASK) >> TIME_HI_SHIFT);
	time_lo = (int)(data->timestamp & TIME_LO_MASK);

	input_report_rel(data->input, REL_RX, gyro.x - data->caldata.x);
	input_report_rel(data->input, REL_RY, gyro.y - data->caldata.y);
	input_report_rel(data->input, REL_RZ, gyro.z - data->caldata.z);
	input_report_rel(data->input, REL_X, time_hi);
	input_report_rel(data->input, REL_Y, time_lo);
	input_sync(data->input);
	data->gyrodata = gyro;
	data->old_timestamp = data->timestamp;
}

static void bmg160_set_enable(struct bmg160_p *data, int enable)
{
	if (enable == ON) {
		hrtimer_start(&data->gyro_timer, data->poll_delay,
		      HRTIMER_MODE_REL);
	} else {
		hrtimer_cancel(&data->gyro_timer);
		cancel_work_sync(&data->work);
	}
}

static ssize_t bmg160_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct bmg160_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&data->enable));
}

static ssize_t bmg160_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u8 enable;
	int ret, pre_enable;
	struct bmg160_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &enable);
	if (ret) {
		pr_err("[SENSOR]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	pr_info("[SENSOR]: %s - new_value = %u\n", __func__, enable);
	pre_enable = atomic_read(&data->enable);

	if (enable) {
		if (pre_enable == OFF) {
			data->old_timestamp = 0LL;
			bmg160_open_calibration(data);
			bmg160_set_mode(data, BMG160_MODE_NORMAL);
			atomic_set(&data->enable, ON);
			bmg160_set_enable(data, ON);
		}
	} else {
		if (pre_enable == ON) {
			atomic_set(&data->enable, OFF);
			bmg160_set_mode(data, BMG160_MODE_SUSPEND);
			bmg160_set_enable(data, OFF);
		}
	}

	return size;
}

static ssize_t bmg160_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct bmg160_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%lld\n",
			ktime_to_ns(data->poll_delay));
}

static ssize_t bmg160_delay_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t delay;
	struct bmg160_p *data = dev_get_drvdata(dev);

	ret = kstrtoll(buf, 10, &delay);
	if (ret) {
		pr_err("[SENSOR]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	if (delay <= 5000000LL)
		bmg160_set_bw(data, BMG160_BW_116Hz);
	else
		bmg160_set_bw(data, BMG160_BW_32Hz);

	data->poll_delay = ns_to_ktime(delay);
	pr_info("[SENSOR]: %s - poll_delay = %lld\n", __func__, delay);

	if (atomic_read(&data->enable) == ON) {
		bmg160_set_mode(data, BMG160_MODE_SUSPEND);
		bmg160_set_enable(data, OFF);
		bmg160_set_mode(data, BMG160_MODE_NORMAL);
		bmg160_set_enable(data, ON);
	}

	return size;
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		bmg160_delay_show, bmg160_delay_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		bmg160_enable_show, bmg160_enable_store);

static struct attribute *bmg160_attributes[] = {
	&dev_attr_poll_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group bmg160_attribute_group = {
	.attrs = bmg160_attributes
};

static ssize_t bmg160_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t bmg160_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", MODEL_NAME);
}

static int bmg160_open_calibration(struct bmg160_p *data)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);

		data->caldata.x = 0;
		data->caldata.y = 0;
		data->caldata.z = 0;

		pr_info("[SENSOR]: %s - No Calibration\n", __func__);

		return ret;
	}

	ret = cal_filp->f_op->read(cal_filp, (char *)&data->caldata.v,
		3 * sizeof(s16), &cal_filp->f_pos);
	if (ret != 3 * sizeof(s16)) {
		pr_err("[SENSOR] %s: - Can't read the cal data\n", __func__);
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	pr_info("[SENSOR]: open gyro calibration %d, %d, %d\n",
		data->caldata.x, data->caldata.y, data->caldata.z);

	if ((data->caldata.x == 0) && (data->caldata.y == 0)
		&& (data->caldata.z == 0))
		return -EIO;

	return ret;
}

static int bmg160_save_calibration(struct bmg160_p *data)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *cal_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (IS_ERR(cal_filp)) {
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);

		data->caldata.x = 0;
		data->caldata.y = 0;
		data->caldata.z = 0;

		pr_err("[SENSOR]: %s - cal_filp open failed(%d)\n",
			__func__, ret);

		return ret;
	}

	ret = cal_filp->f_op->write(cal_filp, (char *)&data->caldata.v,
		3 * sizeof(s16), &cal_filp->f_pos);
	if (ret != 3 * sizeof(s16)) {
		pr_err("[SENSOR]: %s - Can't write the caldata to file\n",
			__func__);
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	pr_info("[SENSOR]: save gyro calibration %d, %d, %d\n",
		data->caldata.x, data->caldata.y, data->caldata.z);

	return ret;
}

static int bmg160_get_caldata(struct bmg160_p *data)
{
	int sum[3] = { 0, };
	int cnt;
	struct bmg160_v gyro;

	for (cnt = 0; cnt < CALIBRATION_DATA_AMOUNT; cnt++) {
		bmg160_read_gyro_xyz(data, &gyro);
		sum[0] += gyro.x;
		sum[1] += gyro.y;
		sum[2] += gyro.z;
		mdelay(10);
	}

	data->caldata.x = (sum[0] / CALIBRATION_DATA_AMOUNT);
	data->caldata.y = (sum[1] / CALIBRATION_DATA_AMOUNT);
	data->caldata.z = (sum[2] / CALIBRATION_DATA_AMOUNT);

	return 0;
}

static ssize_t bmg160_calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	struct bmg160_p *data = dev_get_drvdata(dev);

	ret = bmg160_open_calibration(data);
	if (ret < 0)
		pr_err("[SENSOR]: %s - calibration open failed(%d)\n",
			__func__, ret);

	pr_info("[SENSOR]: %s - cal data %d %d %d - ret : %d\n", __func__,
		data->caldata.x, data->caldata.y, data->caldata.z, ret);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n", ret, data->caldata.x,
			data->caldata.y, data->caldata.z);
}

static ssize_t bmg160_calibration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t dEnable;
	struct bmg160_p *data = dev_get_drvdata(dev);

	ret = kstrtoll(buf, 10, &dEnable);
	if (ret < 0)
		return ret;

	if (atomic_read(&data->enable) == ON)
		bmg160_set_enable(data, OFF);
	else
		bmg160_set_mode(data, BMG160_MODE_NORMAL);

	msleep(100);

	if (dEnable == 1)
		bmg160_get_caldata(data);
	else
		memset(&data->caldata, 0, sizeof(struct bmg160_v));

	bmg160_save_calibration(data);

	if (atomic_read(&data->enable) == ON)
		bmg160_set_enable(data, ON);
	else
		bmg160_set_mode(data, BMG160_MODE_SUSPEND);

	return size;
}

static ssize_t bmg160_raw_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct bmg160_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == OFF) {
		bmg160_set_mode(data, BMG160_MODE_NORMAL);
		msleep(30);
		bmg160_read_gyro_xyz(data, &data->gyrodata);
		bmg160_set_mode(data, BMG160_MODE_SUSPEND);
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->gyrodata.x - data->caldata.x,
		data->gyrodata.y - data->caldata.y,
		data->gyrodata.z - data->caldata.z);
}

static ssize_t bmg160_get_temp(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char tmp;
	s8 temperature;

	struct bmg160_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == OFF)
		bmg160_set_mode(data, BMG160_MODE_NORMAL);

	msleep(100);

	bmg160_i2c_read(data->client, BMG160_TEMP_ADDR, &tmp);
	temperature = 24 + ((s8)tmp / 2);

	if (atomic_read(&data->enable) == OFF)
		bmg160_set_mode(data, BMG160_MODE_SUSPEND);

	pr_info("[SENSOR]: %s - temperature = %d\n", __func__, temperature);

	return sprintf(buf, "%d\n", temperature);
}

static ssize_t bmg160_selftest_dps_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int newdps = 0;
	struct bmg160_p *data = dev_get_drvdata(dev);

	sscanf(buf, "%d", &newdps);

	if (newdps == 250)
		data->gyro_dps = BMG160_RANGE_250DPS;
	else if (newdps == 500)
		data->gyro_dps = BMG160_RANGE_500DPS;
	else if (newdps == 2000)
		data->gyro_dps = BMG160_RANGE_2000DPS;
	else
		data->gyro_dps = BMG160_RANGE_500DPS;

	bmg160_set_range(data, data->gyro_dps);
	pr_info("[SENSOR]: %s - dps = %d\n", __func__, data->gyro_dps);

	return size;
}

static ssize_t bmg160_selftest_dps_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct bmg160_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->gyro_dps);
}

static unsigned char bmg160_selftest(struct bmg160_p *data)
{
	int ret = 0;
	unsigned char bist = 0;
	unsigned char rateok = 0;

	ret = bmg160_i2c_read(data->client, BMG160_SELF_TEST_ADDR, &bist);
	rateok = BMG160_GET_BITSLICE(bist, BMG160_SELF_TEST_ADDR_RATEOK);
	bist = BMG160_SET_BITSLICE(bist, BMG160_SELF_TEST_ADDR_TRIGBIST, 1);
	ret += bmg160_i2c_write(data->client,
			BMG160_SELF_TEST_ADDR_TRIGBIST__REG, bist);

	/* Waiting time to complete the selftest process */
	mdelay(10);

	/* Reading Selftest result bir bist_failure */
	ret += bmg160_i2c_read(data->client,
			BMG160_SELF_TEST_ADDR_BISTFAIL__REG, &bist);
	if (ret < 0)
		pr_err("[SENSOR]: %s - i2c failed %d\n", __func__, ret);

	bist = !(BMG160_GET_BITSLICE(bist, BMG160_SELF_TEST_ADDR_BISTFAIL));

	pr_info("[SENSOR]: %s - rate %u, bist %u\n", __func__, rateok, bist);

	return (rateok && bist);
}

static int bmg160_selftest_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char bist, selftest = 0;
	int datax_check = 0;
	int datay_check = 0;
	int dataz_check = 0;
	int sum[3], cnt;
	struct bmg160_v avg;
	struct bmg160_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == ON)
		bmg160_set_enable(data, OFF);
	else
		bmg160_set_mode(data, BMG160_MODE_NORMAL);

	msleep(100);
	bist = bmg160_selftest(data);
	if (bist == 0)
		selftest |= 1;

	data->gyro_dps = BMG160_RANGE_2000DPS;
	bmg160_set_range(data, data->gyro_dps);

	msleep(100);
	memset(sum, 0, sizeof(int) * 3);
	for (cnt = 0; cnt < SELFTEST_DATA_AMOUNT; cnt++) {
		bmg160_read_gyro_xyz(data, &avg);
		sum[0] += avg.x;
		sum[1] += avg.y;
		sum[2] += avg.z;
	}

	avg.x = (s16)(sum[0] / SELFTEST_DATA_AMOUNT);
	avg.y = (s16)(sum[1] / SELFTEST_DATA_AMOUNT);
	avg.z = (s16)(sum[2] / SELFTEST_DATA_AMOUNT);

	datax_check = (int)abs((2000 * (long)avg.x * 1000) / 32768);
	datay_check = (int)abs((2000 * (long)avg.y * 1000) / 32768);
	dataz_check = (int)abs((2000 * (long)avg.z * 1000) / 32768);

	pr_info("[SENSOR]: %s - x = %d.%03d, y = %d.%03d, z= %d.%03d\n",
		__func__, (datax_check / 1000), (datax_check % 1000),
		(datay_check / 1000), (datay_check % 1000),
		(dataz_check / 1000), (dataz_check % 1000));

	data->gyro_dps = BMG160_RANGE_500DPS;
	bmg160_set_range(data, data->gyro_dps);

	if ((datax_check <= SELFTEST_LIMITATION_OF_ERROR)
		&& (datay_check <= SELFTEST_LIMITATION_OF_ERROR)
		&& (dataz_check <= SELFTEST_LIMITATION_OF_ERROR)) {
		pr_info("[SENSOR]: %s - Gyro zero rate OK!\n", __func__);
		bmg160_get_caldata(data);
		bmg160_save_calibration(data);
	} else {
		pr_info("[SENSOR]: %s - Gyro zero rate NG!\n", __func__);
		selftest |= 1;
	}

	if (atomic_read(&data->enable) == ON)
		bmg160_set_enable(data, ON);
	else
		bmg160_set_mode(data, BMG160_MODE_SUSPEND);

	if (selftest == 0)
		pr_info("[SENSOR]: %s - Gyro selftest Pass!\n", __func__);
	else
		pr_err("[SENSOR]: %s - Gyro selftest fail!\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%u,%u,%d.%03d,%d.%03d,%d.%03d\n",
			selftest ? 0 : 1, bist,
			(datax_check / 1000), (datax_check % 1000),
			(datay_check / 1000), (datay_check % 1000),
			(dataz_check / 1000), (dataz_check % 1000));
}

static DEVICE_ATTR(name, S_IRUGO, bmg160_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, bmg160_vendor_show, NULL);
static DEVICE_ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
	bmg160_calibration_show, bmg160_calibration_store);
static DEVICE_ATTR(raw_data, S_IRUGO, bmg160_raw_data_show, NULL);
static DEVICE_ATTR(temperature, S_IRUGO, bmg160_get_temp, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, bmg160_selftest_show, NULL);
static DEVICE_ATTR(selftest_dps, S_IRUGO | S_IWUSR | S_IWGRP,
	bmg160_selftest_dps_show, bmg160_selftest_dps_store);

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_calibration,
	&dev_attr_raw_data,
	&dev_attr_temperature,
	&dev_attr_selftest,
	&dev_attr_selftest_dps,
	NULL,
};

static void bmg160_setup_pin(struct bmg160_p *data)
{
	int ret;

	ret = gpio_request(data->gyro_int, "GYRO_INT");
	if (ret < 0) {
		pr_err("[SENSOR] %s - gpio %d request failed (%d)\n",
			__func__, data->gyro_int, ret);
	} else {
		ret = gpio_direction_input(data->gyro_int);
		if (ret < 0)
			pr_err("[SENSOR]: %s - failed to set gpio %d as input"
				" (%d)\n", __func__, data->gyro_int, ret);
		gpio_free(data->gyro_int);
	}

	ret = gpio_request(data->gyro_drdy, "GYRO_DRDY");
	if (ret < 0) {
		pr_err("[SENSOR]: %s - gpio %d request failed (%d)\n",
			__func__, data->gyro_drdy, ret);
	} else {
		ret = gpio_direction_input(data->gyro_drdy);
		if (ret < 0)
			pr_err("[SENSOR]: %s - failed to set gpio %d as input"
				" (%d)\n", __func__, data->gyro_drdy, ret);
		gpio_free(data->gyro_drdy);
	}
}

static int bmg160_input_init(struct bmg160_p *data)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_RX);
	input_set_capability(dev, EV_REL, REL_RY);
	input_set_capability(dev, EV_REL, REL_RZ);
	input_set_capability(dev, EV_REL, REL_X); /* time_hi */
	input_set_capability(dev, EV_REL, REL_Y); /* time_lo */

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
	ret = sysfs_create_group(&dev->dev.kobj, &bmg160_attribute_group);
	if (ret < 0) {
		sensors_remove_symlink(&data->input->dev.kobj,
			data->input->name);
		input_unregister_device(dev);
		return ret;
	}

	data->input = dev;
	return 0;
}

static void bmg160_parse_dt(struct bmg160_p *data, struct device *dev)
{
	struct device_node *dNode = dev->of_node;
	enum of_gpio_flags flags;

	if (dNode == NULL) {
		pr_err("[SENSOR]: %s - can't find dNode\n", __func__);
		data->chip_pos = BMG160_TOP_LOWER_RIGHT;
		return;
	}

	if (of_property_read_u32(dNode,
			"bmg160-i2c,chip_pos", &data->chip_pos) < 0)
		data->chip_pos = BMG160_TOP_LOWER_RIGHT;

	data->gyro_int = of_get_named_gpio_flags(dNode,
		"bmg160-i2c,gyro_int-gpio", 0, &flags);
	if (data->gyro_int < 0) {
		pr_err("[SENSOR]: %s - get gyro_int failed\n", __func__);
		return;
	}
	data->gyro_drdy = of_get_named_gpio_flags(dNode,
		"bmg160-i2c,gyro_drdy-gpio", 0, &flags);
	if (data->gyro_drdy < 0) {
		pr_err("[SENSOR]: %s - gyro_drdy failed\n", __func__);
		return;
	}

	bmg160_setup_pin(data);
}

static int bmg160_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct bmg160_p *data = NULL;

	pr_info("[SENSOR]: %s - Probe Start!\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[SENSOR]: %s - i2c_check_functionality error\n",
			__func__);
		goto exit;
	}

	data = kzalloc(sizeof(struct bmg160_p), GFP_KERNEL);
	if (data == NULL) {
		pr_err("[SENSOR]: %s - kzalloc error\n", __func__);
		ret = -ENOMEM;
		goto exit_kzalloc;
	}

	bmg160_parse_dt(data, &client->dev);

	i2c_set_clientdata(client, data);
	data->client = client;

	/* read chip id */
	ret = i2c_smbus_read_word_data(data->client, BMG160_CHIP_ID_REG);
	if ((ret & 0x00ff) != BMG160_CHIP_ID) {
		pr_err("[SENSOR]: %s - chip id failed 0x%x\n",
				__func__, (unsigned int)ret & 0x00ff);
		ret = -ENODEV;
		goto exit_read_chipid;
	}

	/* input device init */
	ret = bmg160_input_init(data);
	if (ret < 0)
		goto exit_input_init;

	sensors_register(data->factory_device, data, sensor_attrs, MODULE_NAME);
	/* gyro_timer settings. we poll for light values using a timer. */
	hrtimer_init(&data->gyro_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->poll_delay = ns_to_ktime(BMG160_DEFAULT_DELAY);
	data->gyro_timer.function = bmg160_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	data->gyro_wq = create_singlethread_workqueue("gyro_wq");
	if (!data->gyro_wq) {
		ret = -ENOMEM;
		pr_err("[SENSOR]: %s - could not create workqueue\n", __func__);
		goto exit_create_workqueue;
	}

	/* workqueue init */
	INIT_WORK(&data->work, bmg160_work_func);
	atomic_set(&data->enable, OFF);

	data->gyro_dps = BMG160_RANGE_500DPS;
	bmg160_set_range(data, data->gyro_dps);
	bmg160_set_bw(data, BMG160_BW_32Hz);
	bmg160_set_mode(data, BMG160_MODE_SUSPEND);
	pr_info("[SENSOR]: %s - Probe done!(chip pos : %d)\n",
		__func__, data->chip_pos);

	return 0;

exit_create_workqueue:
	sensors_unregister(data->factory_device, sensor_attrs);
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);
	sysfs_remove_group(&data->input->dev.kobj, &bmg160_attribute_group);
	input_unregister_device(data->input);
exit_input_init:
exit_read_chipid:
	kfree(data);
exit_kzalloc:
exit:
	pr_err("[SENSOR]: %s - Probe fail!\n", __func__);

	return ret;
}

static void bmg160_shutdown(struct i2c_client *client)
{
	struct bmg160_p *data = (struct bmg160_p *)i2c_get_clientdata(client);

	pr_info("[SENSOR]: %s\n", __func__);
	if (atomic_read(&data->enable) == ON)
		bmg160_set_enable(data, OFF);

	atomic_set(&data->enable, OFF);
	bmg160_set_mode(data, BMG160_MODE_SUSPEND);
}

static int __devexit bmg160_remove(struct i2c_client *client)
{
	struct bmg160_p *data = (struct bmg160_p *)i2c_get_clientdata(client);

	if (atomic_read(&data->enable) == ON)
		bmg160_set_enable(data, OFF);

	atomic_set(&data->enable, OFF);
	bmg160_set_mode(data, BMG160_MODE_SUSPEND);

	sensors_unregister(data->factory_device, sensor_attrs);
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);

	sysfs_remove_group(&data->input->dev.kobj, &bmg160_attribute_group);
	input_unregister_device(data->input);
	kfree(data);

	return 0;
}

static int bmg160_suspend(struct device *dev)
{
	struct bmg160_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == ON) {
		bmg160_set_mode(data, BMG160_MODE_SUSPEND);
		bmg160_set_enable(data, OFF);
	}

	return 0;
}

static int bmg160_resume(struct device *dev)
{
	struct bmg160_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == ON) {
		bmg160_set_mode(data, BMG160_MODE_NORMAL);
		bmg160_set_enable(data, ON);
	}

	return 0;
}

static struct of_device_id bmg160_match_table[] = {
	{ .compatible = "bmg160-i2c",},
	{},
};

static const struct i2c_device_id bmg160_id[] = {
	{ "bmg160_match_table", 0 },
	{ }
};

static const struct dev_pm_ops bmg160_pm_ops = {
	.suspend = bmg160_suspend,
	.resume = bmg160_resume,
};

static struct i2c_driver bmg160_driver = {
	.driver = {
		.name	= MODEL_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = bmg160_match_table,
		.pm = &bmg160_pm_ops
	},
	.probe		= bmg160_probe,
	.shutdown	= bmg160_shutdown,
	.remove		= __devexit_p(bmg160_remove),
	.id_table	= bmg160_id,
};

static int __init bmg160_init(void)
{
	return i2c_add_driver(&bmg160_driver);
}

static void __exit bmg160_exit(void)
{
	i2c_del_driver(&bmg160_driver);
}
module_init(bmg160_init);
module_exit(bmg160_exit);

MODULE_DESCRIPTION("bmg160 gyroscope sensor driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
