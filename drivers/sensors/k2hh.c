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

#define I2C_M_WR                      0 /* for i2c Write */
#define I2c_M_RD                      1 /* for i2c Read */
#define READ_DATA_LENTH               6

#define VENDOR_NAME                   "STM"
#define MODEL_NAME                    "K2HH"
#define MODULE_NAME                   "accelerometer_sensor"

#define CALIBRATION_FILE_PATH         "/efs/accel_calibration_data"
#define CALIBRATION_DATA_AMOUNT       20
#define MAX_ACCEL_1G                  16384

#define K2HH_DEFAULT_DELAY            200000000LL

#define CHIP_ID_RETRIES               3
#define ACCEL_LOG_TIME                15 /* 15 sec */

#define K2HH_TOP_UPPER_RIGHT          0
#define K2HH_TOP_LOWER_RIGHT          1
#define K2HH_TOP_LOWER_LEFT           2
#define K2HH_TOP_UPPER_LEFT           3
#define K2HH_BOTTOM_UPPER_RIGHT       4
#define K2HH_BOTTOM_LOWER_RIGHT       5
#define K2HH_BOTTOM_LOWER_LEFT        6
#define K2HH_BOTTOM_UPPER_LEFT        7

#define K2HH_MODE_SUSPEND             0
#define K2HH_MODE_NORMAL              1

#define SENSITIVITY_2G                61
#define SENSITIVITY_4G                122
#define SENSITIVITY_8G                244

#define K2HH_RANGE_2G                 0
#define K2HH_RANGE_4G                 1
#define K2HH_RANGE_8G                 2

#define WHOAMI_REG                    0x0F
#define AXISDATA_REG                  0x28

#define CTRL1_REG                     0x20
#define CTRL2_REG                     0x21
#define CTRL3_REG                     0x22
#define CTRL4_REG                     0x23
#define CTRL5_REG                     0x24
#define CTRL6_REG                     0x25
#define CTRL7_REG                     0x26
#define STATUS_REG                    0x27

/* CTRL1 */
#define CTRL1_HR_DISABLE              0x00
#define CTRL1_HR_ENABLE               0x80
#define CTRL1_HR_MASK                 0x80
#define CTRL1_BDU_ENABLE              0x08
#define CTRL1_BDU_MASK                0x08

/* CTRL2 */
#define CTRL2_IG1_INT1                0x08

/* CTRL3 */
#define CTRL3_IG1_INT1                0x08

/* CTRL7 */
#define CTRL7_LIR2                    0x08
#define CTRL7_LIR1                    0x04

#define ACC_PM_OFF                    0x00
#define ACC_ENABLE_ALL_AXES           0x07

#define INT_CFG1_REG                  0x30
#define INT_SRC1_REG                  0x31
#define K2HH_CHIP_ID                  0x41

#define	K2HH_ACC_FS_MASK              0x30
#define	K2HH_ACC_ODR_MASK             0x70
#define	K2HH_ACC_AXES_MASK            0x07

#define SELF_TEST_2G_MAX_LSB          24576
#define SELF_TEST_2G_MIN_LSB          1146

#define K2HH_ACC_FS_2G                0x00
#define K2HH_ACC_FS_4G                0x20
#define K2HH_ACC_FS_8G                0x30

#define INT_THSX1_REG                 0x32
#define INT_THSY1_REG                 0x33
#define INT_THSZ1_REG                 0x34

#define DYNAMIC_THRESHOLD             5000

enum {
	OFF = 0,
	ON = 1
};

struct k2hh_v {
	union {
		s16 v[3];
		struct {
			s16 x;
			s16 y;
			s16 z;
		};
	};
};

struct k2hh_p {
	struct wake_lock reactive_wake_lock;
	struct i2c_client *client;
	struct input_dev *input;
	struct delayed_work irq_work;
	struct device *factory_device;
	struct k2hh_v accdata;
	struct k2hh_v caldata;
	struct mutex mode_mutex;
	struct hrtimer accel_timer;
	struct workqueue_struct *accel_wq;
	struct work_struct work;
	struct regulator *l19;
	struct regulator *lvs1_1p8;
	ktime_t poll_delay;
	atomic_t enable;

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

	u64 old_timestamp;
};

#define ACC_ODR10		0x10	/*   10Hz output data rate */
#define ACC_ODR50		0x20	/*   50Hz output data rate */
#define ACC_ODR100		0x30	/*  100Hz output data rate */
#define ACC_ODR200		0x40	/*  200Hz output data rate */
#define ACC_ODR400		0x50	/*  400Hz output data rate */
#define ACC_ODR800		0x60	/*  800Hz output data rate */
#define ACC_ODR_MASK		0X70

struct k2hh_acc_odr {
	unsigned int cutoff_ms;
	unsigned int mask;
};

#define OUTPUT_ALWAYS_ANTI_ALIASED

const struct k2hh_acc_odr k2hh_acc_odr_table[] = {
	{  2, ACC_ODR800},
	{  3, ACC_ODR400},
	{  5, ACC_ODR200},
	{ 10, ACC_ODR100},
#ifndef OUTPUT_ALWAYS_ANTI_ALIASED
	{ 20, ACC_ODR50},
	{100, ACC_ODR10},
#endif
};

static int k2hh_open_calibration(struct k2hh_p *);
static int sensor_regulator_onoff(struct k2hh_p *data, bool onoff);
static int k2hh_i2c_recovery(struct k2hh_p *data)
{
	int ret = 0;
	struct gpiomux_setting old_config[2];
	struct gpiomux_setting recovery_config = {
		.func = GPIOMUX_FUNC_3,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_IN,
	};

	if ((data->sda_gpio < 0) || (data->scl_gpio < 0)) {
		pr_err("[SENSOR]: %s - no sda, scl gpio\n", __func__);
		return -1;
	}

	pr_err("[SENSOR] ################# %s #################\n", __func__);

	/*If LDO's are disabled then enable, Fallback LDO off scenario*/
	do {
		if( 0 == regulator_is_enabled(data->l19) ) {
			ret = regulator_enable(data->l19);
			if (ret){
				pr_err(KERN_ERR "%s: 8226_l19 enable failed (%d)\n",__func__, ret);
				break;
			}
			ret++;
		}
		if( 0 == regulator_is_enabled(data->lvs1_1p8) ) {
			ret = regulator_enable(data->lvs1_1p8);
			if (ret){
				pr_err(KERN_ERR "%s: 8226_l6 enable failed (%d)\n",__func__, ret);
				break;
			}
			ret++;
		}
		if(ret)
			msleep(30);
	}while(0);

	if(ret){
		pr_err("RECOVERY called as one of regulator was off:****\n Regulator \
			on/recovery complete****\n ");
	}


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

	ret = gpio_direction_input(data->sda_gpio);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - failed to set gpio %d as output (%d)\n",
			__func__, data->sda_gpio, ret);
		goto exit_to_free;
	}

	ret = gpio_direction_input(data->scl_gpio);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - failed to set gpio %d as output (%d)\n",
			__func__, data->scl_gpio, ret);
		goto exit_to_free;
	}

	/*Setting initial value after recovery*/
	udelay(100);
	gpio_set_value_cansleep(data->sda_gpio, 0);
	gpio_set_value_cansleep(data->scl_gpio, 0);
	udelay(100);
	gpio_set_value_cansleep(data->sda_gpio, 1);
	gpio_set_value_cansleep(data->scl_gpio, 1);


exit_to_free:
	gpio_free(data->sda_gpio);
	gpio_free(data->scl_gpio);
exit:
	msm_gpiomux_write(data->sda_gpio, GPIOMUX_ACTIVE, &old_config[0], NULL);
	msm_gpiomux_write(data->scl_gpio, GPIOMUX_ACTIVE, &old_config[1], NULL);

	return ret;
}

static int k2hh_i2c_read(struct k2hh_p *data,
		unsigned char reg_addr, unsigned char *buf, unsigned int len)
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
		else
			k2hh_i2c_recovery(data);
	} while (retries++ < 2);

	if (ret < 0) {
		pr_err("[SENSOR]: %s - i2c read error %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int k2hh_i2c_write(struct k2hh_p *data,
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
		else
			k2hh_i2c_recovery(data);
	} while (retries++ < 2);

	if (ret < 0) {
		pr_err("[SENSOR]: %s - i2c write error %d\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int k2hh_set_range(struct k2hh_p *data, unsigned char range)
{
	int ret = 0;
	unsigned char temp, new_range, buf, mask;

	switch (range) {
	case K2HH_RANGE_2G:
		new_range = K2HH_ACC_FS_2G;
		break;
	case K2HH_RANGE_4G:
		new_range = K2HH_ACC_FS_4G;
		break;
	case K2HH_RANGE_8G:
		new_range = K2HH_ACC_FS_8G;
		break;
	default:
		new_range = K2HH_ACC_FS_2G;
		break;
	}

	mask = K2HH_ACC_FS_MASK;
	ret = k2hh_i2c_read(data, CTRL4_REG, &temp, 1);
#ifndef OUTPUT_ALWAYS_ANTI_ALIASED
	buf = (mask & new_range) | ((~mask) & temp);
#else
	buf = 0xCC;
#endif
	ret += k2hh_i2c_write(data, CTRL4_REG, buf);

	return ret;
}

static int k2hh_set_odr(struct k2hh_p *data)
{
	int ret = 0, i;
	unsigned char buf, new_odr, mask, temp;

	/* Following, looks for the longest possible odr interval scrolling the
	 * odr_table vector from the end (shortest interval) backward (longest
	 * interval), to support the poll_interval requested by the system.
	 * It must be the longest interval lower then the poll interval.*/
	for (i = ARRAY_SIZE(k2hh_acc_odr_table) - 1; i >= 0; i--) {
		if ((k2hh_acc_odr_table[i].cutoff_ms <= \
			ktime_to_ms(data->poll_delay)) || (i == 0))
			break;
	}

	if (data->recog_flag == ON)
		i = ARRAY_SIZE(k2hh_acc_odr_table) - 1;

	new_odr = k2hh_acc_odr_table[i].mask;

	mask = K2HH_ACC_ODR_MASK;
	ret = k2hh_i2c_read(data, CTRL1_REG, &temp, 1);
	buf = ((mask & new_odr) | ((~mask) & temp));
	ret += k2hh_i2c_write(data, CTRL1_REG, buf);

	pr_info("[SENSOR]: %s - change odr %d\n", __func__, i);
	return ret;
}

static int k2hh_set_mode(struct k2hh_p *data, unsigned char mode)
{
	int ret = 0;
	unsigned char buf, mask, temp;

	mutex_lock(&data->mode_mutex);

	switch (mode) {
	case K2HH_MODE_NORMAL:
		mask = K2HH_ACC_AXES_MASK;
		ret = k2hh_i2c_read(data, CTRL1_REG, &temp, 1);
		buf = ((mask & ACC_ENABLE_ALL_AXES) | ((~mask) & temp));
		ret += k2hh_i2c_write(data, CTRL1_REG, buf);
		break;
	case K2HH_MODE_SUSPEND:
		mask = K2HH_ACC_AXES_MASK;
		ret = k2hh_i2c_read(data, CTRL1_REG, &temp, 1);
		buf = ((mask & ACC_PM_OFF) | ((~mask) & temp));
		ret += k2hh_i2c_write(data, CTRL1_REG, buf);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	pr_info("[SENSOR]: %s - change mode %u\n", __func__, mode);
	mutex_unlock(&data->mode_mutex);

	return ret;
}

static int k2hh_read_accel_xyz(struct k2hh_p *data, struct k2hh_v *acc)
{
	int ret = 0;
	struct k2hh_v rawdata;
	unsigned char buf[READ_DATA_LENTH];

	ret += k2hh_i2c_read(data, AXISDATA_REG, buf, READ_DATA_LENTH);

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

static enum hrtimer_restart k2hh_timer_func(struct hrtimer *timer)
{
	struct k2hh_p *data = container_of(timer,
					struct k2hh_p, accel_timer);

	if (!work_pending(&data->work))
		queue_work(data->accel_wq, &data->work);

	hrtimer_forward_now(&data->accel_timer, data->poll_delay);

	return HRTIMER_RESTART;
}

static void k2hh_work_func(struct work_struct *work)
{
	int ret;
	struct k2hh_v acc;
	struct k2hh_p *data = container_of(work, struct k2hh_p, work);
	struct timespec ts = ktime_to_timespec(alarm_get_elapsed_realtime());
	u64 timestamp_new = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	int time_hi, time_lo;

	ret = k2hh_read_accel_xyz(data, &acc);
	if (ret < 0)
		goto exit;

	data->accdata.x = acc.x - data->caldata.x;
	data->accdata.y = acc.y - data->caldata.y;
	data->accdata.z = acc.z - data->caldata.z;
	if (data->old_timestamp != 0 && 
		((timestamp_new - data->old_timestamp) > ktime_to_ms(data->poll_delay) * 1800000LL)) {
		u64 delay = ktime_to_ns(data->poll_delay);
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

static void k2hh_set_enable(struct k2hh_p *data, int enable)
{
	if (enable == ON) {
		hrtimer_start(&data->accel_timer, data->poll_delay,
		      HRTIMER_MODE_REL);
	} else {
		hrtimer_cancel(&data->accel_timer);
		cancel_work_sync(&data->work);
	}
}

static ssize_t k2hh_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct k2hh_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&data->enable));
}

static ssize_t k2hh_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u8 enable;
	int ret, pre_enable;
	struct k2hh_p *data = dev_get_drvdata(dev);

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
			sensor_regulator_onoff(data, true);

			k2hh_open_calibration(data);
			k2hh_set_range(data, K2HH_RANGE_2G);

			k2hh_set_mode(data, K2HH_MODE_NORMAL);
			atomic_set(&data->enable, ON);
			k2hh_set_enable(data, ON);
		}
	} else {
		if (pre_enable == ON) {
			atomic_set(&data->enable, OFF);
			k2hh_set_mode(data, K2HH_MODE_SUSPEND);
			k2hh_set_enable(data, OFF);

			sensor_regulator_onoff(data, false);
		}
	}

	return size;
}

static ssize_t k2hh_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct k2hh_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%lld\n",
			ktime_to_ns(data->poll_delay));
}

static ssize_t k2hh_delay_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t delay;
	struct k2hh_p *data = dev_get_drvdata(dev);

	ret = kstrtoll(buf, 10, &delay);
	if (ret) {
		pr_err("[SENSOR]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	if (delay > K2HH_DEFAULT_DELAY)
		delay = K2HH_DEFAULT_DELAY;

	data->poll_delay = ns_to_ktime(delay);
	k2hh_set_odr(data);

	if (atomic_read(&data->enable) == ON) {
		k2hh_set_mode(data, K2HH_MODE_SUSPEND);
		k2hh_set_mode(data, K2HH_MODE_NORMAL);
	}

	pr_info("[SENSOR]: %s - poll_delay = %lld\n", __func__, delay);
	return size;
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		k2hh_delay_show, k2hh_delay_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		k2hh_enable_show, k2hh_enable_store);

static struct attribute *k2hh_attributes[] = {
	&dev_attr_poll_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group k2hh_attribute_group = {
	.attrs = k2hh_attributes
};


static ssize_t k2hh_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t k2hh_name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", MODEL_NAME);
}

static int k2hh_open_calibration(struct k2hh_p *data)
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

static int k2hh_do_calibrate(struct k2hh_p *data, int enable)
{
	int sum[3] = { 0, };
	int ret = 0, cnt;
	struct file *cal_filp = NULL;
	struct k2hh_v acc;
	mm_segment_t old_fs;

	if (enable) {
		data->caldata.x = 0;
		data->caldata.y = 0;
		data->caldata.z = 0;

		if (atomic_read(&data->enable) == ON)
			k2hh_set_enable(data, OFF);
		else
			k2hh_set_mode(data, K2HH_MODE_NORMAL);

		msleep(300);

		for (cnt = 0; cnt < CALIBRATION_DATA_AMOUNT; cnt++) {
			k2hh_read_accel_xyz(data, &acc);
			sum[0] += acc.x;
			sum[1] += acc.y;
			sum[2] += acc.z;
			mdelay(10);
		}

		if (atomic_read(&data->enable) == ON)
			k2hh_set_enable(data, ON);
		else
			k2hh_set_mode(data, K2HH_MODE_SUSPEND);

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

static ssize_t k2hh_calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	struct k2hh_p *data = dev_get_drvdata(dev);

	ret = k2hh_open_calibration(data);
	if (ret < 0)
		pr_err("[SENSOR]: %s - calibration open failed(%d)\n",
			__func__, ret);

	pr_info("[SENSOR]: %s - cal data %d %d %d - ret : %d\n", __func__,
		data->caldata.x, data->caldata.y, data->caldata.z, ret);

	return snprintf(buf, PAGE_SIZE, "%d %d %d %d\n", ret, data->caldata.x,
			data->caldata.y, data->caldata.z);
}

static ssize_t k2hh_calibration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int64_t dEnable;
	struct k2hh_p *data = dev_get_drvdata(dev);

	ret = kstrtoll(buf, 10, &dEnable);
	if (ret < 0)
		return ret;

	ret = k2hh_do_calibrate(data, (int)dEnable);
	if (ret < 0)
		pr_err("[SENSOR]: %s - accel calibrate failed\n", __func__);

	return size;
}

static ssize_t k2hh_raw_data_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct k2hh_v acc;
	struct k2hh_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == OFF) {
		k2hh_set_mode(data, K2HH_MODE_NORMAL);
		msleep(20);
		k2hh_read_accel_xyz(data, &acc);
		k2hh_set_mode(data, K2HH_MODE_SUSPEND);

		acc.x = acc.x - data->caldata.x;
		acc.y = acc.y - data->caldata.y;
		acc.z = acc.z - data->caldata.z;
	} else {
		acc = data->accdata;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
			acc.x, acc.y, acc.z);
}

static ssize_t k2hh_reactive_alert_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned char threshx, threshy, threshz;
	int enable = OFF, factory_mode = OFF;
	struct k2hh_v acc;
	struct k2hh_p *data = dev_get_drvdata(dev);

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
		data->recog_flag = ON;

		if (factory_mode == ON) {
			k2hh_i2c_write(data, INT_THSX1_REG, 0x00);
			k2hh_i2c_write(data, INT_THSY1_REG, 0x00);
			k2hh_i2c_write(data, INT_THSZ1_REG, 0x00);
			k2hh_i2c_write(data, INT_CFG1_REG, 0x3f);
		} else {
			k2hh_set_odr(data);
			if (atomic_read(&data->enable) == OFF) {
				k2hh_set_mode(data, K2HH_MODE_NORMAL);
				msleep(20);
				k2hh_read_accel_xyz(data, &acc);
				k2hh_set_mode(data, K2HH_MODE_SUSPEND);
			} else {
				acc.x = data->accdata.x;
				acc.y = data->accdata.y;
				acc.z = data->accdata.z;
			}

			threshx = (abs(acc.v[data->axis_map_x]) \
					+ DYNAMIC_THRESHOLD) >> 8;
			threshy = (abs(acc.v[data->axis_map_y]) \
					+ DYNAMIC_THRESHOLD) >> 8;
			threshz = (abs(acc.v[data->axis_map_z]) \
					+ DYNAMIC_THRESHOLD) >> 8;

			k2hh_i2c_write(data, INT_THSX1_REG, threshx);
			k2hh_i2c_write(data, INT_THSY1_REG, threshy);
			k2hh_i2c_write(data, INT_THSZ1_REG, threshz);
			k2hh_i2c_write(data, INT_CFG1_REG, 0x0a);
		}

		k2hh_i2c_write(data, CTRL7_REG, CTRL7_LIR1);
		k2hh_i2c_write(data, CTRL3_REG, CTRL3_IG1_INT1);

		enable_irq(data->irq1);
		enable_irq_wake(data->irq1);

		pr_info("[SENSOR]: %s - reactive alert is on!\n", __func__);
	} else if ((enable == OFF) && (data->recog_flag == ON)) {
		k2hh_i2c_write(data, CTRL3_REG, 0x00);

		disable_irq_wake(data->irq1);
		disable_irq_nosync(data->irq1);
		data->recog_flag = OFF;
		pr_info("[SENSOR]: %s - reactive alert is off! irq = %d\n",
			__func__, data->irq_state);
	}

	return size;
}

static ssize_t k2hh_reactive_alert_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct k2hh_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->irq_state);
}

static ssize_t k2hh_selftest_show(struct device *dev,
       struct device_attribute *attr, char *buf)
{
	struct k2hh_p *data = dev_get_drvdata(dev);
	struct k2hh_v acc;
	unsigned char temp;
	int result = 1, i;
	ssize_t ret;
	s32 NO_ST[3] = {0, 0, 0};
	s32 ST[3] = {0, 0, 0};

	if (atomic_read(&data->enable) == OFF)
		k2hh_set_mode(data, K2HH_MODE_NORMAL);
	else
		k2hh_set_enable(data, OFF);

	k2hh_i2c_write(data, CTRL1_REG, 0x3f);
	k2hh_i2c_write(data, CTRL4_REG, 0x04);
	k2hh_i2c_write(data, CTRL5_REG, 0x00);
	k2hh_i2c_write(data, CTRL6_REG, 0x00);

	mdelay(80);

	k2hh_read_accel_xyz(data, &acc);

	for (i = 0; i < 5; i++) {
		while (1) {
			if (k2hh_i2c_read(data, STATUS_REG, &temp, 1) < 0) {
				pr_err("[SENSOR] %s: i2c error", __func__);
				goto exit_status_err;
			}

			if (temp & 0x08)
				break;
		}

		k2hh_read_accel_xyz(data, &acc);
		NO_ST[0] += acc.x;
		NO_ST[1] += acc.y;
		NO_ST[2] += acc.z;
	}
	NO_ST[0]  /= 5;
	NO_ST[1]  /= 5;
	NO_ST[2]  /= 5;

	k2hh_i2c_write(data, CTRL5_REG, 0x04);

	mdelay(80);

	k2hh_read_accel_xyz(data, &acc);

	for (i = 0; i < 5; i++) {
		while (1) {
			if (k2hh_i2c_read(data, STATUS_REG, &temp, 1) < 0) {
				pr_err("[SENSOR] %s: i2c error", __func__);
				goto exit_status_err;
			}

			if (temp & 0x08)
				break;
		}

		k2hh_read_accel_xyz(data, &acc);
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

		if ((SELF_TEST_2G_MIN_LSB > ST[i]) \
			|| (ST[i] > SELF_TEST_2G_MAX_LSB)) {
			pr_info("[SENSOR] %s: %d Out of range!! (%d)\n",
				__func__, i, ST[i]);
			result = 0;
		}
	}

	if (result)
		ret = sprintf(buf, "1,%d,%d,%d\n", ST[0], ST[1], ST[2]);
	else
		ret = sprintf(buf, "0,%d,%d,%d\n", ST[0], ST[1], ST[2]);

	goto exit;

exit_status_err:
	ret = sprintf(buf, "-1,0,0,0\n");	
exit:
	k2hh_i2c_write(data, CTRL1_REG, 0x00);
	k2hh_i2c_write(data, CTRL5_REG, 0x00);

	if (atomic_read(&data->enable) == OFF) {
		k2hh_set_mode(data, K2HH_MODE_SUSPEND);
	} else {
		k2hh_set_mode(data, K2HH_MODE_NORMAL);
		k2hh_set_enable(data, ON);
	}

	return ret;
}

static DEVICE_ATTR(selftest, S_IRUGO, k2hh_selftest_show, NULL);
static DEVICE_ATTR(name, S_IRUGO, k2hh_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, k2hh_vendor_show, NULL);
static DEVICE_ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
	k2hh_calibration_show, k2hh_calibration_store);
static DEVICE_ATTR(raw_data, S_IRUGO, k2hh_raw_data_read, NULL);
static DEVICE_ATTR(reactive_alert, S_IRUGO | S_IWUSR | S_IWGRP,
	k2hh_reactive_alert_show, k2hh_reactive_alert_store);

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_calibration,
	&dev_attr_raw_data,
	&dev_attr_reactive_alert,
	&dev_attr_selftest,
	NULL,
};

static void k2hh_irq_work_func(struct work_struct *work)
{
	struct k2hh_p *data = container_of((struct delayed_work *)work,
		struct k2hh_p, irq_work);
	unsigned char buf;

	k2hh_i2c_write(data, INT_CFG1_REG, 0x00);
	k2hh_i2c_read(data, INT_SRC1_REG, &buf, 1);
}

static irqreturn_t k2hh_irq_thread(int irq, void *k2hh_data_p)
{
	struct k2hh_p *data = k2hh_data_p;

	data->irq_state = 1;
	wake_lock_timeout(&data->reactive_wake_lock,
		msecs_to_jiffies(2000));
	schedule_delayed_work(&data->irq_work, msecs_to_jiffies(100));
	pr_info("###### [SENSOR]: %s reactive irq ######\n", __func__);

	return IRQ_HANDLED;
}

static int k2hh_setup_pin(struct k2hh_p *data)
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

	wake_lock_init(&data->reactive_wake_lock, WAKE_LOCK_SUSPEND,
		       "reactive_wake_lock");

	data->irq1 = gpio_to_irq(data->acc_int1);
	ret = request_threaded_irq(data->irq1, NULL, k2hh_irq_thread,
		IRQF_TRIGGER_RISING | IRQF_ONESHOT, "k2hh_accel", data);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - can't allocate irq.\n", __func__);
		goto exit_reactive_irq;
	}

	disable_irq(data->irq1);
	goto exit;

exit_reactive_irq:
	wake_lock_destroy(&data->reactive_wake_lock);
exit_acc_int1:
	gpio_free(data->acc_int1);
exit:
	return ret;
}

static int k2hh_input_init(struct k2hh_p *data)
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
	ret = sysfs_create_group(&dev->dev.kobj, &k2hh_attribute_group);
	if (ret < 0) {
		sensors_remove_symlink(&data->input->dev.kobj,
			data->input->name);
		input_unregister_device(dev);
		return ret;
	}

	data->input = dev;
	return 0;
}

static int k2hh_parse_dt(struct k2hh_p *data, struct device *dev)
{
	struct device_node *dNode = dev->of_node;
	enum of_gpio_flags flags;
	int ret;
	u32 temp;

	if (dNode == NULL)
		return -ENODEV;

	data->acc_int1 = of_get_named_gpio_flags(dNode,
			"stm,irq_gpio", 0, &flags);
	if (data->acc_int1 < 0) {
		pr_err("[SENSOR]: %s - get acc_int1 error\n", __func__);
		return -ENODEV;
	}

	data->sda_gpio = of_get_named_gpio_flags(dNode,
		"stm,sda", 0, &flags);
	if (data->sda_gpio < 0)
		pr_info("[SENSOR]: %s - no sda_gpio\n", __func__);

	data->scl_gpio = of_get_named_gpio_flags(dNode,
		"stm,scl", 0, &flags);
	if (data->scl_gpio < 0)
		pr_info("[SENSOR]: %s - no scl_gpio\n", __func__);

	ret = of_property_read_u32(dNode,"stm,axis_map_x", &temp);
	if ((data->axis_map_x > 2) || (ret < 0)) {
		pr_err("%s: invalid x axis_map value %u\n",
			__func__, data->axis_map_x);
		data->axis_map_x = 0;
	} else {
		data->axis_map_x= (u8)temp;
	}

	ret = of_property_read_u32(dNode,"stm,axis_map_y", &temp);
	if ((data->axis_map_y > 2) || (ret < 0)) {
		pr_err("%s: invalid y axis_map value %u\n",
			__func__, data->axis_map_y);
		data->axis_map_y = 1;
	} else {
		data->axis_map_y= (u8)temp;
	}

	ret = of_property_read_u32(dNode,"stm,axis_map_z", &temp);
	if ((data->axis_map_z > 2) || (ret < 0)) {
		pr_err("%s: invalid z axis_map value %u\n",
			__func__, data->axis_map_z);
		data->axis_map_z = 2;
	} else {
		data->axis_map_z= (u8)temp;
	}

	ret = of_property_read_u32(dNode,"stm,negate_x", &temp);
	if ((data->negate_x > 1) || (ret < 0)) {
		pr_err("%s: invalid x axis_map value %u\n",
			__func__, data->negate_x);
		data->negate_x = 0;
	} else {
		data->negate_x= (u8)temp;
	}

	ret = of_property_read_u32(dNode,"stm,negate_y", &temp);
	if ((data->negate_y > 1) || (ret < 0)) {
		pr_err("%s: invalid y axis_map value %u\n",
			__func__, data->negate_y);
		data->negate_y = 0;
	} else {
		data->negate_y= (u8)temp;
	}

	ret = of_property_read_u32(dNode,"stm,negate_z", &temp);
	if ((data->negate_z > 1) || (ret < 0)) {
		pr_err("%s: invalid z axis_map value %u\n",
			__func__, data->negate_z);
		data->negate_z = 0;
	} else {
		data->negate_z= (u8)temp;
	}

	return 0;
}

static int sensor_regulator_onoff(struct k2hh_p *data, bool onoff)
{
	int ret = -1;

	if (!data->l19) {
		data->l19 = regulator_get(&data->client->dev, "8226_l19");
		if (!data->l19) {
			pr_err("%s: regulator pointer null l19, rc=%d\n",
				__func__, ret);
			return ret;
		}
		ret = regulator_set_voltage(data->l19, 2850000, 2850000);
		if (ret) {
			pr_err("%s: set voltage failed on l19, rc=%d\n",
				__func__, ret);
			return ret;
		}
	}
	if (!data->lvs1_1p8) {
		data->lvs1_1p8 = regulator_get(&data->client->dev, "8226_lvs1");
		if(!data->lvs1_1p8){
			pr_err("%s: regulator_get for 8226_lvs1 failed\n",
				__func__);
			return 0;
		}
	}
	if(onoff){
		ret = regulator_enable(data->l19);
		if (ret) {
			pr_err("%s: Failed to enable regulator l19.\n",
				__func__);
			return ret;
		}
		ret = regulator_enable(data->lvs1_1p8);
		if (ret) {
			pr_err("%s: Failed to enable regulator lvs1_1p8.\n",
				__func__);
			return ret;
		}
		msleep(30);
	}
	else {
		ret = regulator_disable(data->l19);
		if (ret) {
			pr_err("%s: Failed to disable regulatorl19.\n",
				__func__);
			return ret;
		}
		ret = regulator_disable(data->lvs1_1p8);
		if (ret) {
			pr_err("%s: Failed to disable regulator lvs1_1p8.\n",
				__func__);
			return ret;
		}
	}

	return 0;
}

static int k2hh_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	u8 temp;
	int ret = -ENODEV, i;
	struct k2hh_p *data = NULL;

	pr_err("[SENSOR]: %s - Probe Start!\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[SENSOR]: %s - i2c_check_functionality error\n",
			__func__);
		goto exit;
	}

	data = kzalloc(sizeof(struct k2hh_p), GFP_KERNEL);
	if (data == NULL) {
		pr_err("[SENSOR]: %s - kzalloc error\n", __func__);
		ret = -ENOMEM;
		goto exit_kzalloc;
	}

	i2c_set_clientdata(client, data);
	data->client = client;

	ret = sensor_regulator_onoff(data, true);
	if (ret < 0)
		pr_err("[SENSOR]: %s - No regulator\n", __func__);

	ret = k2hh_parse_dt(data, &client->dev);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - of_node error\n", __func__);
		ret = -ENODEV;
		goto exit_of_node;
	}

	ret = k2hh_setup_pin(data);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - could not setup pin\n", __func__);
		goto exit_setup_pin;
	}

	mutex_init(&data->mode_mutex);

	/* read chip id */
	k2hh_set_mode(data, K2HH_MODE_NORMAL);
	for (i = 0; i < CHIP_ID_RETRIES; i++) {
		ret = k2hh_i2c_read(data, WHOAMI_REG, &temp, 1);
		if (temp != K2HH_CHIP_ID) {
			pr_err("[SENSOR]: %s - chip id failed 0x%x : %d\n",
				__func__, temp, ret);
		} else {
			pr_info("[SENSOR]: %s - chip id success 0x%x\n",
				__func__, temp);
			break;
		}
		msleep(20);
	}

	if (i >= CHIP_ID_RETRIES) {
		ret = -ENODEV;
		goto exit_read_chipid;
	}

	/* input device init */
	ret = k2hh_input_init(data);
	if (ret < 0)
		goto exit_input_init;

	sensors_register(data->factory_device, data, sensor_attrs, MODULE_NAME);

	/* accel_timer settings. we poll for light values using a timer. */
	hrtimer_init(&data->accel_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->poll_delay = ns_to_ktime(K2HH_DEFAULT_DELAY);
	data->accel_timer.function = k2hh_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	data->accel_wq = create_singlethread_workqueue("accel_wq");
	if (!data->accel_wq) {
		ret = -ENOMEM;
		pr_err("[SENSOR]: %s - could not create workqueue\n", __func__);
		goto exit_create_workqueue;
	}

	/* this is the thread function we run on the work queue */
	INIT_WORK(&data->work, k2hh_work_func);
	INIT_DELAYED_WORK(&data->irq_work, k2hh_irq_work_func);

	atomic_set(&data->enable, OFF);
	data->time_count = 0;
	data->irq_state = 0;
	data->recog_flag = OFF;

	k2hh_set_range(data, K2HH_RANGE_2G);
	k2hh_set_mode(data, K2HH_MODE_SUSPEND);

	/*power off the regulators, for next enable power will be on*/
	ret = sensor_regulator_onoff(data, false);

	pr_err("[SENSOR]: %s - Probe done!\n", __func__);

	return 0;

exit_create_workqueue:
	sensors_unregister(data->factory_device, sensor_attrs);
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);
	sysfs_remove_group(&data->input->dev.kobj, &k2hh_attribute_group);
	input_unregister_device(data->input);
exit_input_init:
exit_read_chipid:
	mutex_destroy(&data->mode_mutex);
	free_irq(data->irq1, data);
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

static void k2hh_shutdown(struct i2c_client *client)
{
	struct k2hh_p *data = (struct k2hh_p *)i2c_get_clientdata(client);

	pr_info("[SENSOR]: %s\n", __func__);

	if (atomic_read(&data->enable) == ON)
		k2hh_set_enable(data, OFF);

	atomic_set(&data->enable, OFF);
	k2hh_set_mode(data, K2HH_MODE_SUSPEND);
}

static int __devexit k2hh_remove(struct i2c_client *client)
{
	struct k2hh_p *data = (struct k2hh_p *)i2c_get_clientdata(client);

	if (atomic_read(&data->enable) == ON)
		k2hh_set_enable(data, OFF);

	atomic_set(&data->enable, OFF);
	cancel_delayed_work_sync(&data->irq_work);

	k2hh_set_mode(data, K2HH_MODE_SUSPEND);
	sensors_unregister(data->factory_device, sensor_attrs);
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);

	sysfs_remove_group(&data->input->dev.kobj, &k2hh_attribute_group);
	input_unregister_device(data->input);

	free_irq(data->irq1, data);
	wake_lock_destroy(&data->reactive_wake_lock);
	mutex_destroy(&data->mode_mutex);
	gpio_free(data->acc_int1);
	kfree(data);

	return 0;
}

static int k2hh_suspend(struct device *dev)
{
	struct k2hh_p *data = dev_get_drvdata(dev);

	pr_info("[SENSOR]: %s\n", __func__);

	if (atomic_read(&data->enable) == ON) {
		k2hh_set_mode(data, K2HH_MODE_SUSPEND);
		k2hh_set_enable(data, OFF);
	}

	return 0;
}

static int k2hh_resume(struct device *dev)
{
	struct k2hh_p *data = dev_get_drvdata(dev);

	pr_info("[SENSOR]: %s\n", __func__);

	if (atomic_read(&data->enable) == ON) {
		k2hh_set_mode(data, K2HH_MODE_NORMAL);
		k2hh_set_enable(data, ON);
	}

	return 0;
}

static struct of_device_id k2hh_match_table[] = {
	{ .compatible = "stm,k2hh",},
	{},
};

static const struct i2c_device_id k2hh_id[] = {
	{ "k2hh_match_table", 0 },
	{ }
};

static const struct dev_pm_ops k2hh_pm_ops = {
	.suspend = k2hh_suspend,
	.resume = k2hh_resume
};

static struct i2c_driver k2hh_driver = {
	.driver = {
		.name	= MODEL_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = k2hh_match_table,
		.pm = &k2hh_pm_ops
	},
	.probe		= k2hh_probe,
	.shutdown	= k2hh_shutdown,
	.remove		= __devexit_p(k2hh_remove),
	.id_table	= k2hh_id,
};

static int __init k2hh_init(void)
{
	return i2c_add_driver(&k2hh_driver);
}

static void __exit k2hh_exit(void)
{
	i2c_del_driver(&k2hh_driver);
}

module_init(k2hh_init);
module_exit(k2hh_exit);

MODULE_DESCRIPTION("k2hh accelerometer sensor driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");

