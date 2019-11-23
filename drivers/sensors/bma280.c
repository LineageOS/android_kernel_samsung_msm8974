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
#include <linux/wakelock.h>
#include <linux/regulator/consumer.h>
#include <mach/gpiomux.h>

#include "sensors_core.h"
#include "bma280_reg.h"

#define I2C_M_WR                        0 /* for i2c Write */
#define I2c_M_RD                        1 /* for i2c Read */
#define READ_DATA_LENTH                 6

#define VENDOR_NAME                     "BOSCH"
#define MODEL_NAME                      "BMA280"
#define MODULE_NAME                     "accelerometer_sensor"

#define CALIBRATION_FILE_PATH           "/efs/accel_calibration_data"
#define CALIBRATION_DATA_AMOUNT         20
#define MAX_ACCEL_1G			4096

#define BMA280_DEFAULT_DELAY            200000000LL
#define BMA280_CHIP_ID                  0xFB

#define CHIP_ID_RETRIES                 3
#define ACCEL_LOG_TIME                  15 /* 15 sec */

#define SLOPE_X_INT                     0
#define SLOPE_Y_INT                     1
#define SLOPE_Z_INT                     2

#define SLOPE_DURATION_VALUE            2
#define SLOPE_THRESHOLD_VALUE           0x0A

#define BMA280_TOP_UPPER_RIGHT          0
#define BMA280_TOP_LOWER_RIGHT          1
#define BMA280_TOP_LOWER_LEFT           2
#define BMA280_TOP_UPPER_LEFT           3
#define BMA280_BOTTOM_UPPER_RIGHT       4
#define BMA280_BOTTOM_LOWER_RIGHT       5
#define BMA280_BOTTOM_LOWER_LEFT        6
#define BMA280_BOTTOM_UPPER_LEFT        7

enum {
	OFF = 0,
	ON = 1
};

struct bma280_v {
	union {
		s16 v[3];
		struct {
			s16 x;
			s16 y;
			s16 z;
		};
	};
};

struct bma280_p {
	struct wake_lock reactive_wake_lock;
	struct i2c_client *client;
	struct input_dev *input;
	struct delayed_work irq_work;
	struct device *factory_device;
	struct bma280_v accdata;
	struct bma280_v caldata;
	struct mutex mode_mutex;
	struct hrtimer accel_timer;
	struct workqueue_struct *accel_wq;
	struct work_struct work;
	ktime_t poll_delay;
	atomic_t enable;

	u32 chip_pos;
	int recog_flag;
	int irq1;
	int irq_state;
	int acc_int1;
	int sda_gpio;
	int scl_gpio;
	int time_count;

	u64 old_timestamp;
};

static int bma280_open_calibration(struct bma280_p *);

static int bma280_i2c_recovery(struct bma280_p *data)
{
	int ret, i;
	struct gpiomux_setting old_config[2];
	struct gpiomux_setting recovery_config = {
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	};

	if ((data->sda_gpio < 0) || (data->scl_gpio < 0)) {
		pr_info("[SENSOR]: %s - no sda, scl gpio\n", __func__);
		return -1;
	}

	pr_info("[SENSOR] ################# %s #################\n", __func__);

	ret = msm_gpiomux_write(data->sda_gpio, GPIOMUX_ACTIVE,
			&recovery_config, &old_config[0]);
	if (ret < 0) {
		pr_err("[SENSOR]: %s sda_gpio have no active setting %d\n",
			__func__, ret);
		goto exit;
	}

	ret = msm_gpiomux_write(data->scl_gpio, GPIOMUX_ACTIVE,
			&recovery_config, &old_config[1]);
	if (ret < 0) {
		pr_err("[SENSOR]: %s scl_gpio have no active setting %d\n",
			__func__, ret);
		goto exit;
	}

	ret = gpio_request(data->sda_gpio, "SENSOR_SDA");
	if (ret < 0) {
		pr_err("[SENSOR]: %s - gpio %d request failed (%d)\n",
			__func__, data->sda_gpio, ret);
		goto exit;
	}

	ret = gpio_request(data->scl_gpio, "SENSOR_SCL");
	if (ret < 0) {
		pr_err("[SENSOR]: %s - gpio %d request failed (%d)\n",
			__func__, data->scl_gpio, ret);
		gpio_free(data->scl_gpio);
		goto exit;
	}

	ret = gpio_direction_output(data->sda_gpio, 1);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - failed to set gpio %d as output (%d)\n",
			__func__, data->sda_gpio, ret);
		goto exit_to_free;
	}

	ret = gpio_direction_output(data->scl_gpio, 1);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - failed to set gpio %d as output (%d)\n",
			__func__, data->scl_gpio, ret);
		goto exit_to_free;
	}

	for (i = 0; i < 5; i++) {
		udelay(100);
		gpio_set_value_cansleep(data->sda_gpio, 0);
		gpio_set_value_cansleep(data->scl_gpio, 0);
		udelay(100);
		gpio_set_value_cansleep(data->sda_gpio, 1);
		gpio_set_value_cansleep(data->scl_gpio, 1);
	}

	ret = gpio_direction_input(data->sda_gpio);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - failed to set gpio %d as input (%d)\n",
			__func__, data->sda_gpio, ret);
		goto exit_to_free;
	}

	ret = gpio_direction_input(data->scl_gpio);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - failed to set gpio %d as input (%d)\n",
			__func__, data->scl_gpio, ret);
		goto exit_to_free;
	}

exit_to_free:
	gpio_free(data->sda_gpio);
	gpio_free(data->scl_gpio);
exit:
	msm_gpiomux_write(data->sda_gpio, GPIOMUX_ACTIVE, &old_config[0], NULL);
	msm_gpiomux_write(data->scl_gpio, GPIOMUX_ACTIVE, &old_config[1], NULL);

	return ret;
}

static int bma280_i2c_read(struct bma280_p *data,
		unsigned char reg_addr, unsigned char *buf)
{
	int ret, retries = 0;
	struct i2c_msg msg[2];

	msg[0].addr = data->client->addr;
	msg[0].flags = I2C_M_WR;
	msg[0].len = 1;
	msg[0].buf = &reg_addr;

	msg[1].addr = data->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	do {
		ret = i2c_transfer(data->client->adapter, msg, 2);
		if (ret < 0)
			bma280_i2c_recovery(data);
		else
			break;
	} while (retries++ < 2);

	if (ret < 0) {
		pr_err("[SENSOR]: %s - i2c read error %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int bma280_i2c_write(struct bma280_p *data,
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
		if (ret < 0)
			bma280_i2c_recovery(data);
		else
			break;
	} while (retries++ < 2);

	if (ret < 0) {
		pr_err("[SENSOR]: %s - i2c write error %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int bma280_set_mode(struct bma280_p *data, unsigned char mode)
{
	int ret = 0;
	unsigned char buf1, buf2;

	mutex_lock(&data->mode_mutex);

	ret = bma280_i2c_read(data, BMA280_MODE_CTRL_REG, &buf1);
	ret += bma280_i2c_read(data, BMA280_LOW_NOISE_CTRL_REG, &buf2);

	switch (mode) {
	case BMA280_MODE_NORMAL:
		buf1  = BMA280_SET_BITSLICE(buf1, BMA280_MODE_CTRL, 0);
		buf2  = BMA280_SET_BITSLICE(buf2, BMA280_LOW_POWER_MODE, 0);
		ret += bma280_i2c_write(data, BMA280_MODE_CTRL_REG, buf1);
		mdelay(1);
		ret += bma280_i2c_write(data, BMA280_LOW_NOISE_CTRL_REG, buf2);
		break;
	case BMA280_MODE_LOWPOWER1:
		buf1  = BMA280_SET_BITSLICE(buf1, BMA280_MODE_CTRL, 2);
		buf2  = BMA280_SET_BITSLICE(buf2, BMA280_LOW_POWER_MODE, 0);
		ret += bma280_i2c_write(data, BMA280_MODE_CTRL_REG, buf1);
		mdelay(1);
		ret += bma280_i2c_write(data, BMA280_LOW_NOISE_CTRL_REG, buf2);
		break;
	case BMA280_MODE_SUSPEND:
		buf1  = BMA280_SET_BITSLICE(buf1, BMA280_MODE_CTRL, 4);
		buf2  = BMA280_SET_BITSLICE(buf2, BMA280_LOW_POWER_MODE, 0);
		ret += bma280_i2c_write(data, BMA280_LOW_NOISE_CTRL_REG, buf2);
		mdelay(1);
		ret += bma280_i2c_write(data, BMA280_MODE_CTRL_REG, buf1);
		break;
	case BMA280_MODE_DEEP_SUSPEND:
		buf1  = BMA280_SET_BITSLICE(buf1, BMA280_MODE_CTRL, 1);
		buf2  = BMA280_SET_BITSLICE(buf2, BMA280_LOW_POWER_MODE, 1);
		ret += bma280_i2c_write(data, BMA280_MODE_CTRL_REG, buf1);
		mdelay(1);
		ret += bma280_i2c_write(data, BMA280_LOW_NOISE_CTRL_REG, buf2);
		break;
	case BMA280_MODE_LOWPOWER2:
		buf1  = BMA280_SET_BITSLICE(buf1, BMA280_MODE_CTRL, 2);
		buf2  = BMA280_SET_BITSLICE(buf2, BMA280_LOW_POWER_MODE, 1);
		ret += bma280_i2c_write(data, BMA280_MODE_CTRL_REG, buf1);
		mdelay(1);
		ret += bma280_i2c_write(data, BMA280_LOW_NOISE_CTRL_REG, buf2);
		break;
	case BMA280_MODE_STANDBY:
		buf1  = BMA280_SET_BITSLICE(buf1, BMA280_MODE_CTRL, 4);
		buf2  = BMA280_SET_BITSLICE(buf2, BMA280_LOW_POWER_MODE, 1);
		ret += bma280_i2c_write(data, BMA280_LOW_NOISE_CTRL_REG, buf2);
		mdelay(1);
		ret += bma280_i2c_write(data, BMA280_MODE_CTRL_REG, buf1);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	pr_info("[SENSOR]: %s - change mode %u\n", __func__, mode);
	mutex_unlock(&data->mode_mutex);

	return ret;
}

static int bma280_set_range(struct bma280_p *data, unsigned char range)
{
	int ret = 0 ;
	unsigned char buf;

	ret = bma280_i2c_read(data, BMA280_RANGE_SEL_REG, &buf);

	switch (range) {
	case BMA280_RANGE_2G:
		buf = BMA280_SET_BITSLICE(buf, BMA280_RANGE_SEL, 3);
		break;
	case BMA280_RANGE_4G:
		buf = BMA280_SET_BITSLICE(buf, BMA280_RANGE_SEL, 5);
		break;
	case BMA280_RANGE_8G:
		buf = BMA280_SET_BITSLICE(buf, BMA280_RANGE_SEL, 8);
		break;
	case BMA280_RANGE_16G:
		buf = BMA280_SET_BITSLICE(buf, BMA280_RANGE_SEL, 12);
		break;
	default:
		buf = BMA280_SET_BITSLICE(buf, BMA280_RANGE_SEL, 3);
		break;
	}

	ret += bma280_i2c_write(data, BMA280_RANGE_SEL_REG, buf);

	return ret;
}

static int bma280_set_bandwidth(struct bma280_p *data,
		unsigned char bandwidth)
{
	int ret = 0;
	unsigned char buf;

	if (bandwidth <= 7 || bandwidth >= 15)
		bandwidth = BMA280_BW_250HZ;

	ret = bma280_i2c_read(data, BMA280_BANDWIDTH__REG, &buf);
	buf = BMA280_SET_BITSLICE(buf, BMA280_BANDWIDTH, bandwidth);
	ret += bma280_i2c_write(data, BMA280_BANDWIDTH__REG, buf);

	pr_info("[SENSOR]: %s - change bandwidth %u\n", __func__, bandwidth);
	return ret;
}

static int bma280_read_accel_xyz(struct bma280_p *data,	struct bma280_v *acc)
{
	int ret = 0, i;
	unsigned char buf[READ_DATA_LENTH];

	for (i = 0; i < READ_DATA_LENTH; i++) {
		ret += bma280_i2c_read(data,
				BMA280_ACC_Y14_LSB__REG + i, &buf[i]);
	}

	if (ret < 0)
		goto exit;

	acc->y = BMA280_GET_BITSLICE(buf[0], BMA280_ACC_Y14_LSB) |
			(BMA280_GET_BITSLICE(buf[1],
			BMA280_ACC_Y_MSB) << (BMA280_ACC_Y14_LSB__LEN));
	acc->y = acc->y << (sizeof(short) * 8 - (BMA280_ACC_Y14_LSB__LEN +
			BMA280_ACC_Y_MSB__LEN));
	acc->y = acc->y >> (sizeof(short) * 8 - (BMA280_ACC_Y14_LSB__LEN +
			BMA280_ACC_Y_MSB__LEN));

	acc->x = BMA280_GET_BITSLICE(buf[2], BMA280_ACC_X14_LSB) |
			(BMA280_GET_BITSLICE(buf[3], BMA280_ACC_X_MSB) <<
			(BMA280_ACC_X14_LSB__LEN));
	acc->x = acc->x << (sizeof(short) * 8 - (BMA280_ACC_X14_LSB__LEN +
			BMA280_ACC_X_MSB__LEN));
	acc->x = acc->x >> (sizeof(short) * 8 - (BMA280_ACC_X14_LSB__LEN +
			BMA280_ACC_X_MSB__LEN));

	acc->z = BMA280_GET_BITSLICE(buf[4], BMA280_ACC_Z14_LSB) |
			(BMA280_GET_BITSLICE(buf[5],
			BMA280_ACC_Z_MSB) << (BMA280_ACC_Z14_LSB__LEN));
	acc->z = acc->z << (sizeof(short) * 8 - (BMA280_ACC_Z14_LSB__LEN +
			BMA280_ACC_Z_MSB__LEN));
	acc->z = acc->z >> (sizeof(short) * 8 - (BMA280_ACC_Z14_LSB__LEN +
			BMA280_ACC_Z_MSB__LEN));

	remap_sensor_data(acc->v, data->chip_pos);

exit:
	return ret;
}

static enum hrtimer_restart bma280_timer_func(struct hrtimer *timer)
{
	struct bma280_p *data = container_of(timer,
					struct bma280_p, accel_timer);
	if (!work_pending(&data->work))
		queue_work(data->accel_wq, &data->work);

	hrtimer_forward_now(&data->accel_timer, data->poll_delay);

	return HRTIMER_RESTART;
}

static void bma280_work_func(struct work_struct *work)
{
	int ret;
	struct bma280_v acc;
	struct bma280_p *data = container_of(work, struct bma280_p, work);

	struct timespec ts;
	u64 timestamp_new;
	int time_hi, time_lo;
	//u64 diff = 0ULL;
	u64 delay = ktime_to_ns(data->poll_delay);

	get_monotonic_boottime(&ts);
	timestamp_new = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

	ret = bma280_read_accel_xyz(data, &acc);
	if (ret < 0)
		goto exit;

	data->accdata.x = acc.x - data->caldata.x;
	data->accdata.y = acc.y - data->caldata.y;
	data->accdata.z = acc.z - data->caldata.z;

	if (data->old_timestamp != 0 &&
	   ((timestamp_new - data->old_timestamp) > ktime_to_ms(data->poll_delay) * 1800000LL)) {

		u64 shift_timestamp = delay >> 1;
		u64 timestamp = 0ULL;

		for (timestamp = data->old_timestamp + delay; timestamp < timestamp_new - shift_timestamp; timestamp+=delay) {
				time_hi = (int)((timestamp & TIME_HI_MASK) >> TIME_HI_SHIFT);
				time_lo = (int)(timestamp & TIME_LO_MASK);
				input_report_rel(data->input, REL_X, data->accdata.x);
				input_report_rel(data->input, REL_Y, data->accdata.y);
				input_report_rel(data->input, REL_Z, data->accdata.z);
				input_report_rel(data->input, REL_DIAL, time_hi);
				input_report_rel(data->input, REL_MISC, time_lo);
				input_sync(data->input);
		}
	}

	time_hi = (int)((timestamp_new & TIME_HI_MASK) >> TIME_HI_SHIFT);
	time_lo = (int)(timestamp_new & TIME_LO_MASK);

	input_report_rel(data->input, REL_X, data->accdata.x);
	input_report_rel(data->input, REL_Y, data->accdata.y);
	input_report_rel(data->input, REL_Z, data->accdata.z);
	input_report_rel(data->input, REL_DIAL, time_hi);
	input_report_rel(data->input, REL_MISC, time_lo);
	input_sync(data->input);
	data->old_timestamp = timestamp_new;

exit:
	if ((ktime_to_ns(data->poll_delay) * (int64_t)data->time_count)
		>= ((int64_t)ACCEL_LOG_TIME * NSEC_PER_SEC)) {
		pr_info("[SENSOR]: %s - x = %d, y = %d, z = %d (ra:%d)\n",
			__func__, data->accdata.x, data->accdata.y,
			data->accdata.z, data->recog_flag);
		data->time_count = 0;
	} else
		data->time_count++;
}

static void bma280_set_enable(struct bma280_p *data, int enable)
{
	if (enable == ON) {
		hrtimer_start(&data->accel_timer, data->poll_delay,
		      HRTIMER_MODE_REL);
	} else {
		hrtimer_cancel(&data->accel_timer);
		cancel_work_sync(&data->work);
	}
}

static ssize_t bma280_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct bma280_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&data->enable));
}

static ssize_t bma280_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u8 enable;
	int ret, pre_enable;
	struct bma280_p *data = dev_get_drvdata(dev);

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
			bma280_open_calibration(data);
			bma280_set_mode(data, BMA280_MODE_NORMAL);
			atomic_set(&data->enable, ON);
			bma280_set_enable(data, ON);
		}
	} else {
		if (pre_enable == ON) {
			atomic_set(&data->enable, OFF);
			if (data->recog_flag == ON)
				bma280_set_mode(data, BMA280_MODE_LOWPOWER1);
			else
				bma280_set_mode(data, BMA280_MODE_SUSPEND);

			bma280_set_enable(data, OFF);
		}
	}

	return size;
}

static ssize_t bma280_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct bma280_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%lld\n",
			ktime_to_ns(data->poll_delay));
}

static ssize_t bma280_delay_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t delay;
	struct bma280_p *data = dev_get_drvdata(dev);

	ret = kstrtoll(buf, 10, &delay);
	if (ret) {
		pr_err("[SENSOR]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	if (delay <= 3000000LL)
		bma280_set_bandwidth(data, BMA280_BW_500HZ);
	else if (delay <= 5000000LL)
		bma280_set_bandwidth(data, BMA280_BW_125HZ);
	else if (delay <= 10000000LL)
		bma280_set_bandwidth(data, BMA280_BW_62_50HZ);
	else if (delay <= 20000000LL)
		bma280_set_bandwidth(data, BMA280_BW_31_25HZ);
	else
		bma280_set_bandwidth(data, BMA280_BW_7_81HZ);

	data->poll_delay = ns_to_ktime(delay);
	pr_info("[SENSOR]: %s - poll_delay = %lld\n", __func__, delay);

	if (atomic_read(&data->enable) == ON) {
		bma280_set_mode(data, BMA280_MODE_SUSPEND);
		bma280_set_enable(data, OFF);
		bma280_set_mode(data, BMA280_MODE_NORMAL);
		bma280_set_enable(data, ON);
	}

	return size;
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		bma280_delay_show, bma280_delay_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		bma280_enable_show, bma280_enable_store);

static struct attribute *bma280_attributes[] = {
	&dev_attr_poll_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group bma280_attribute_group = {
	.attrs = bma280_attributes
};


static ssize_t bma280_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t bma280_name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", MODEL_NAME);
}

static int bma280_open_calibration(struct bma280_p *data)
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

	pr_info("[SENSOR]: open accel calibration %d, %d, %d\n",
		data->caldata.x, data->caldata.y, data->caldata.z);

	if ((data->caldata.x == 0) && (data->caldata.y == 0)
		&& (data->caldata.z == 0))
		return -EIO;

	return ret;
}

static int bma280_do_calibrate(struct bma280_p *data, int enable)
{
	int sum[3] = { 0, };
	int ret = 0, cnt;
	struct file *cal_filp = NULL;
	struct bma280_v acc;
	mm_segment_t old_fs;

	if (enable) {
		data->caldata.x = 0;
		data->caldata.y = 0;
		data->caldata.z = 0;

		if (atomic_read(&data->enable) == ON)
			bma280_set_enable(data, OFF);
		else
			bma280_set_mode(data, BMA280_MODE_NORMAL);

		msleep(300);

		for (cnt = 0; cnt < CALIBRATION_DATA_AMOUNT; cnt++) {
			bma280_read_accel_xyz(data, &acc);
			sum[0] += acc.x;
			sum[1] += acc.y;
			sum[2] += acc.z;
			mdelay(10);
		}

		if (atomic_read(&data->enable) == ON)
			bma280_set_enable(data, ON);
		else
			bma280_set_mode(data, BMA280_MODE_SUSPEND);

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

	pr_info("[SENSOR]: %s - do accel calibrate %d, %d, %d\n", __func__,
		data->caldata.x, data->caldata.y, data->caldata.z);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (IS_ERR(cal_filp)) {
		pr_err("[SENSOR]: %s - Can't open calibration file\n",
			__func__);
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);
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

	return ret;
}

static ssize_t bma280_calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	struct bma280_p *data = dev_get_drvdata(dev);

	ret = bma280_open_calibration(data);
	if (ret < 0)
		pr_err("[SENSOR]: %s - calibration open failed(%d)\n",
			__func__, ret);

	pr_info("[SENSOR]: %s - cal data %d %d %d - ret : %d\n", __func__,
		data->caldata.x, data->caldata.y, data->caldata.z, ret);

	return snprintf(buf, PAGE_SIZE, "%d %d %d %d\n", ret, data->caldata.x,
			data->caldata.y, data->caldata.z);
}

static ssize_t bma280_calibration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t dEnable;
	struct bma280_p *data = dev_get_drvdata(dev);

	ret = kstrtoll(buf, 10, &dEnable);
	if (ret < 0)
		return ret;

	ret = bma280_do_calibrate(data, (int)dEnable);
	if (ret < 0)
		pr_err("[SENSOR]: %s - accel calibrate failed\n", __func__);

	return size;
}

static ssize_t bma280_raw_data_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct bma280_v acc;
	struct bma280_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == OFF) {
		bma280_set_mode(data, BMA280_MODE_NORMAL);
		msleep(20);
		bma280_read_accel_xyz(data, &acc);
		bma280_set_mode(data, BMA280_MODE_SUSPEND);

		acc.x = acc.x - data->caldata.x;
		acc.y = acc.y - data->caldata.y;
		acc.z = acc.z - data->caldata.z;
	} else {
		acc = data->accdata;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
			acc.x, acc.y, acc.z);
}

static void bma280_set_int_enable(struct bma280_p *data,
		unsigned char InterruptType , unsigned char value)
{
	unsigned char reg;

	bma280_i2c_read(data, BMA280_INT_ENABLE1_REG, &reg);

	switch (InterruptType) {
	case SLOPE_X_INT:
		/* Slope X Interrupt */
		reg = BMA280_SET_BITSLICE(reg, BMA280_EN_SLOPE_X_INT, value);
		break;
	case SLOPE_Y_INT:
		/* Slope Y Interrupt */
		reg = BMA280_SET_BITSLICE(reg, BMA280_EN_SLOPE_Y_INT, value);
		break;
	case SLOPE_Z_INT:
		/* Slope Z Interrupt */
		reg = BMA280_SET_BITSLICE(reg, BMA280_EN_SLOPE_Z_INT, value);
		break;
	default:
		break;
	}

	bma280_i2c_write(data, BMA280_INT_ENABLE1_REG, reg);
}

static void bma280_slope_enable(struct bma280_p *data,
		int enable, int factory_mode)
{
	unsigned char reg;

	if (enable == ON) {
		bma280_i2c_read(data, BMA280_EN_INT1_PAD_SLOPE__REG, &reg);
		reg = BMA280_SET_BITSLICE(reg, BMA280_EN_INT1_PAD_SLOPE, ON);
		bma280_i2c_write(data, BMA280_EN_INT1_PAD_SLOPE__REG, reg);

		bma280_i2c_read(data, BMA280_INT_MODE_SEL__REG, &reg);
		reg = BMA280_SET_BITSLICE(reg, BMA280_INT_MODE_SEL, 0x01);
		bma280_i2c_write(data, BMA280_INT_MODE_SEL__REG, reg);

		if (factory_mode == OFF) {
			bma280_i2c_read(data, BMA280_SLOPE_DUR__REG, &reg);
			reg = BMA280_SET_BITSLICE(reg, BMA280_SLOPE_DUR,
					SLOPE_DURATION_VALUE);
			bma280_i2c_write(data, BMA280_SLOPE_DUR__REG, reg);

			reg = SLOPE_THRESHOLD_VALUE;
			bma280_i2c_write(data, BMA280_SLOPE_THRES__REG, reg);

			bma280_set_int_enable(data, SLOPE_X_INT, ON);
			bma280_set_int_enable(data, SLOPE_Y_INT, ON);
			bma280_set_int_enable(data, SLOPE_Z_INT, ON);
		} else {
			bma280_i2c_read(data, BMA280_SLOPE_DUR__REG, &reg);
			reg = BMA280_SET_BITSLICE(reg, BMA280_SLOPE_DUR, 0x01);
			bma280_i2c_write(data, BMA280_SLOPE_DUR__REG, reg);

			reg = 0x00;
			bma280_i2c_write(data, BMA280_SLOPE_THRES__REG, reg);

			bma280_set_int_enable(data, SLOPE_Z_INT, ON);
			bma280_set_bandwidth(data, BMA280_BW_250HZ);
		}
	} else if (enable == OFF) {
		bma280_i2c_read(data, BMA280_EN_INT1_PAD_SLOPE__REG, &reg);
		reg = BMA280_SET_BITSLICE(reg, BMA280_EN_INT1_PAD_SLOPE, OFF);
		bma280_i2c_write(data, BMA280_EN_INT1_PAD_SLOPE__REG, reg);

		bma280_set_int_enable(data, SLOPE_X_INT, OFF);
		bma280_set_int_enable(data, SLOPE_Y_INT, OFF);
		bma280_set_int_enable(data, SLOPE_Z_INT, OFF);
	}
}

static ssize_t bma280_reactive_alert_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int enable = OFF, factory_mode = OFF;
	struct bma280_p *data = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "0")) {
		enable = OFF;
		factory_mode = OFF;
		pr_info("[SENSOR]: %s - disable\n", __func__);
	} else if (sysfs_streq(buf, "1")) {
		enable = ON;
		factory_mode = OFF;
		pr_info("[SENSOR]: %s - enable\n", __func__);
	} else if (sysfs_streq(buf, "2")) {
		enable = ON;
		factory_mode = ON;
		pr_info("[SENSOR]: %s - factory mode\n", __func__);
	} else {
		pr_err("[SENSOR]: %s - invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if ((enable == ON) && (data->recog_flag == OFF)) {
		data->irq_state = 0;

		bma280_slope_enable(data, ON, factory_mode);
		enable_irq(data->irq1);
		enable_irq_wake(data->irq1);

		data->recog_flag = ON;
		if (atomic_read(&data->enable) == OFF)
			bma280_set_mode(data, BMA280_MODE_LOWPOWER1);

		pr_info("[SENSOR]: %s - reactive alert is on!\n", __func__);
	} else if ((enable == OFF) && (data->recog_flag == ON)) {
		bma280_slope_enable(data, OFF, factory_mode);

		disable_irq_wake(data->irq1);
		disable_irq_nosync(data->irq1);

		data->recog_flag = OFF;
		if (atomic_read(&data->enable) == OFF)
			bma280_set_mode(data, BMA280_MODE_SUSPEND);

		pr_info("[SENSOR]: %s - reactive alert is off! irq = %d\n",
			__func__, data->irq_state);
	}

	return size;
}

static ssize_t bma280_reactive_alert_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct bma280_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->irq_state);
}

static DEVICE_ATTR(name, S_IRUGO, bma280_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, bma280_vendor_show, NULL);
static DEVICE_ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
	bma280_calibration_show, bma280_calibration_store);
static DEVICE_ATTR(raw_data, S_IRUGO, bma280_raw_data_read, NULL);
static DEVICE_ATTR(reactive_alert, S_IRUGO | S_IWUSR | S_IWGRP,
	bma280_reactive_alert_show, bma280_reactive_alert_store);

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_calibration,
	&dev_attr_raw_data,
	&dev_attr_reactive_alert,
	NULL,
};

static void bma280_irq_work_func(struct work_struct *work)
{
	struct bma280_p *data = container_of((struct delayed_work *)work,
		struct bma280_p, irq_work);

	bma280_set_int_enable(data, SLOPE_X_INT, OFF);
	bma280_set_int_enable(data, SLOPE_Y_INT, OFF);
	bma280_set_int_enable(data, SLOPE_Z_INT, OFF);
}

static irqreturn_t bma280_irq_thread(int irq, void *bma280_data_p)
{
	struct bma280_p *data = bma280_data_p;

	data->irq_state = 1;
	wake_lock_timeout(&data->reactive_wake_lock,
		msecs_to_jiffies(2000));
	schedule_delayed_work(&data->irq_work, msecs_to_jiffies(100));
	pr_info("###### [SENSOR]: %s reactive irq ######\n", __func__);

	return IRQ_HANDLED;
}

static int bma280_setup_pin(struct bma280_p *data)
{
	int ret;

	ret = gpio_request(data->acc_int1, "ACC_INT1");
	if (ret < 0) {
		pr_err("[SENSOR] %s - gpio %d request failed (%d)\n",
			__func__, data->acc_int1, ret);
		goto exit;
	}

	ret = gpio_direction_input(data->acc_int1);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - failed to set gpio %d as input (%d)\n",
			__func__, data->acc_int1, ret);
		goto exit_acc_int1;
	}

	goto exit;

exit_acc_int1:
	gpio_free(data->acc_int1);
exit:
	return ret;
}

static int bma280_input_init(struct bma280_p *data)
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
	input_set_capability(dev, EV_REL, REL_DIAL);
	input_set_capability(dev, EV_REL, REL_MISC);
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
	ret = sysfs_create_group(&dev->dev.kobj, &bma280_attribute_group);
	if (ret < 0) {
		sensors_remove_symlink(&data->input->dev.kobj,
			data->input->name);
		input_unregister_device(dev);
		return ret;
	}

	data->input = dev;
	return 0;
}

static int bma280_parse_dt(struct bma280_p *data, struct device *dev)
{
	struct device_node *dNode = dev->of_node;
	enum of_gpio_flags flags;

	if (dNode == NULL)
		return -ENODEV;

	data->acc_int1 = of_get_named_gpio_flags(dNode,
		"bma280-i2c,acc_int1-gpio", 0, &flags);
	if (data->acc_int1 < 0) {
		pr_err("[SENSOR]: %s - get acc_int1 error\n", __func__);
		return -ENODEV;
	}

	data->sda_gpio = of_get_named_gpio_flags(dNode,
		"bma280-i2c,sda", 0, &flags);
	if (data->sda_gpio < 0)
		pr_info("[SENSOR]: %s - no sda_gpio\n", __func__);

	data->scl_gpio = of_get_named_gpio_flags(dNode,
		"bma280-i2c,scl", 0, &flags);
	if (data->scl_gpio < 0)
		pr_info("[SENSOR]: %s - no scl_gpio\n", __func__);

	if (of_property_read_u32(dNode,
			"bma280-i2c,chip_pos", &data->chip_pos) < 0)
		data->chip_pos = BMA280_TOP_LOWER_RIGHT;

	return 0;
}

static int sensor_regulator_onoff(struct device *dev, bool onoff)
{
	struct regulator *sensor_vcc, *sensor_lvs1;

	sensor_vcc = devm_regulator_get(dev, "bma280-i2c,vdd");
	if (IS_ERR(sensor_vcc)) {
		pr_err("[SENSOR]: %s - cannot get sensor_vcc\n", __func__);
		return -ENOMEM;
	}

	sensor_lvs1 = devm_regulator_get(dev, "bma280-i2c,vdd-io");
	if (IS_ERR(sensor_lvs1)) {
		pr_err("[SENSOR]: %s - cannot get sensor_lvs1\n", __func__);
		devm_regulator_put(sensor_vcc);
		return -ENOMEM;
	}

	if (onoff) {
		regulator_enable(sensor_vcc);
		regulator_enable(sensor_lvs1);
	} else {
		regulator_disable(sensor_vcc);
		regulator_disable(sensor_lvs1);
	}

	devm_regulator_put(sensor_vcc);
	devm_regulator_put(sensor_lvs1);
	mdelay(5);

	return 0;
}

static int bma280_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = -ENODEV, i;
	struct bma280_p *data = NULL;

	pr_info("##########################################################\n");
	pr_info("[SENSOR]: %s - Probe Start!\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[SENSOR]: %s - i2c_check_functionality error\n",
			__func__);
		goto exit;
	}

	ret = sensor_regulator_onoff(&client->dev, true);
	if (ret < 0)
		pr_err("[SENSOR]: %s - No regulator\n", __func__);

	data = kzalloc(sizeof(struct bma280_p), GFP_KERNEL);
	if (data == NULL) {
		pr_err("[SENSOR]: %s - kzalloc error\n", __func__);
		ret = -ENOMEM;
		goto exit_kzalloc;
	}

	ret = bma280_parse_dt(data, &client->dev);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - of_node error\n", __func__);
		ret = -ENODEV;
		goto exit_of_node;
	}

	ret = bma280_setup_pin(data);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - could not setup pin\n", __func__);
		goto exit_setup_pin;
	}

	i2c_set_clientdata(client, data);
	data->client = client;
	mutex_init(&data->mode_mutex);
	wake_lock_init(&data->reactive_wake_lock, WAKE_LOCK_SUSPEND,
		       "reactive_wake_lock");

	/* read chip id */
	for (i = 0; i < CHIP_ID_RETRIES; i++) {
		ret = i2c_smbus_read_word_data(client, BMA280_CHIP_ID_REG);
		if ((ret & 0x00ff) != BMA280_CHIP_ID) {
			pr_err("[SENSOR]: %s - chip id failed 0x%x\n",
				__func__, (unsigned int)ret & 0x00ff);
		} else {
			pr_info("[SENSOR]: %s - chip id success 0x%x\n",
				__func__, (unsigned int)ret & 0x00ff);
			break;
		}
		msleep(20);
	}

	if (i >= CHIP_ID_RETRIES) {
		ret = -ENODEV;
		goto exit_read_chipid;
	}

	/* input device init */
	ret = bma280_input_init(data);
	if (ret < 0)
		goto exit_input_init;

	sensors_register(data->factory_device, data, sensor_attrs, MODULE_NAME);

	/* accel_timer settings. we poll for light values using a timer. */
	hrtimer_init(&data->accel_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->poll_delay = ns_to_ktime(BMA280_DEFAULT_DELAY);
	data->accel_timer.function = bma280_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	data->accel_wq = create_singlethread_workqueue("accel_wq");
	if (!data->accel_wq) {
		ret = -ENOMEM;
		pr_err("[SENSOR]: %s - could not create workqueue\n", __func__);
		goto exit_create_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&data->work, bma280_work_func);
	INIT_DELAYED_WORK(&data->irq_work, bma280_irq_work_func);

	data->irq1 = gpio_to_irq(data->acc_int1);

	ret = request_threaded_irq(data->irq1, NULL, bma280_irq_thread,
		IRQF_TRIGGER_RISING | IRQF_ONESHOT, "bma280_accel", data);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - can't allocate irq.\n", __func__);
		goto exit_request_threaded_irq;
	}

	disable_irq(data->irq1);

	atomic_set(&data->enable, OFF);
	data->time_count = 0;
	data->irq_state = 0;
	data->recog_flag = OFF;

	bma280_set_bandwidth(data, BMA280_BW_125HZ);
	bma280_set_range(data, BMA280_RANGE_2G);
	bma280_set_mode(data, BMA280_MODE_SUSPEND);

	pr_info("[SENSOR]: %s - Probe done!(chip pos : %d)\n",
		__func__, data->chip_pos);

	return 0;

exit_request_threaded_irq:
exit_create_workqueue:
	sensors_unregister(data->factory_device, sensor_attrs);
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);
	sysfs_remove_group(&data->input->dev.kobj, &bma280_attribute_group);
	input_unregister_device(data->input);
exit_input_init:
exit_read_chipid:
	mutex_destroy(&data->mode_mutex);
	wake_lock_destroy(&data->reactive_wake_lock);
	gpio_free(data->acc_int1);
exit_setup_pin:
exit_of_node:
	kfree(data);
exit_kzalloc:
exit:
	pr_err("[SENSOR]: %s - Probe fail!\n", __func__);
	return ret;
}

static void bma280_shutdown(struct i2c_client *client)
{
	struct bma280_p *data = (struct bma280_p *)i2c_get_clientdata(client);

	pr_info("[SENSOR]: %s\n", __func__);

	if (atomic_read(&data->enable) == ON)
		bma280_set_enable(data, OFF);

	atomic_set(&data->enable, OFF);
	bma280_set_mode(data, BMA280_MODE_SUSPEND);
}

static int __devexit bma280_remove(struct i2c_client *client)
{
	struct bma280_p *data = (struct bma280_p *)i2c_get_clientdata(client);

	if (atomic_read(&data->enable) == ON)
		bma280_set_enable(data, OFF);

	atomic_set(&data->enable, OFF);
	cancel_delayed_work_sync(&data->irq_work);

	bma280_set_mode(data, BMA280_MODE_SUSPEND);
	sensors_unregister(data->factory_device, sensor_attrs);
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);

	sysfs_remove_group(&data->input->dev.kobj, &bma280_attribute_group);
	input_unregister_device(data->input);

	free_irq(data->irq1, data);
	wake_lock_destroy(&data->reactive_wake_lock);
	mutex_destroy(&data->mode_mutex);
	gpio_free(data->acc_int1);
	kfree(data);

	return 0;
}

static int bma280_suspend(struct device *dev)
{
	struct bma280_p *data = dev_get_drvdata(dev);

	pr_info("[SENSOR]: %s\n", __func__);

	if (atomic_read(&data->enable) == ON) {
		if (data->recog_flag == ON)
			bma280_set_mode(data, BMA280_MODE_LOWPOWER1);
		else
			bma280_set_mode(data, BMA280_MODE_SUSPEND);

		bma280_set_enable(data, OFF);
	}

	return 0;
}

static int bma280_resume(struct device *dev)
{
	struct bma280_p *data = dev_get_drvdata(dev);

	pr_info("[SENSOR]: %s\n", __func__);

	if (atomic_read(&data->enable) == ON) {
		bma280_set_mode(data, BMA280_MODE_NORMAL);
		bma280_set_enable(data, ON);
	}

	return 0;
}

static struct of_device_id bma280_match_table[] = {
	{ .compatible = "bma280-i2c",},
	{},
};

static const struct i2c_device_id bma280_id[] = {
	{ "bma280_match_table", 0 },
	{ }
};

static const struct dev_pm_ops bma280_pm_ops = {
	.suspend = bma280_suspend,
	.resume = bma280_resume
};

static struct i2c_driver bma280_driver = {
	.driver = {
		.name	= MODEL_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = bma280_match_table,
		.pm = &bma280_pm_ops
	},
	.probe		= bma280_probe,
	.shutdown	= bma280_shutdown,
	.remove		= __devexit_p(bma280_remove),
	.id_table	= bma280_id,
};

static int __init bma280_init(void)
{
	return i2c_add_driver(&bma280_driver);
}

static void __exit bma280_exit(void)
{
	i2c_del_driver(&bma280_driver);
}

module_init(bma280_init);
module_exit(bma280_exit);

MODULE_DESCRIPTION("bma280 accelerometer sensor driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
