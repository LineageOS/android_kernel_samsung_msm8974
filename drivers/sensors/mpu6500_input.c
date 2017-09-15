/*
	$License:
	Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	$
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/wakelock.h>
#include <linux/hrtimer.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>


#include <linux/sensors_core.h>
#include <linux/sensor/mpu6500_platformdata.h>
#include "./mpu6500_selftest.h"
#include "./mpu6500_input.h"

#define DEBUG 0
#define MPU6500_MODE_NORMAL	0
#define MPU6500_MODE_SLEEP	2
#define MPU6500_MODE_WAKE_UP	3

#define MAX_GYRO	32767
#define MIN_GYRO	-32768

#define MPU6500_DEFAULT_DELAY            200000000LL

#define MPU6500_LOGTIME		10
#define LOG_RESULT_LOCATION(x) {\
	printk(KERN_ERR "%s:%s:%d result=%d\n",__FILE__,__func__,__LINE__, x);\
}\

#define CHECK_RESULT(x) {\
		result = x;\
		if (unlikely(result)) \
			LOG_RESULT_LOCATION(result);\
	}

#ifndef MIN
#define	MIN(a, b)		(((a) < (b))?(a):(b))
#endif

#ifndef MAX
#define	MAX(a, b)		(((a) > (b))?(a):(b))
#endif

#define MPU6500_ACCEL_LPF_GAIN(x)				(((x)*8)/10)

#define INT_SRC_ORIENT 0x02
#define INT_SRC_DISPLAY_ORIENT  0x08
#define DMP_MASK_DIS_ORIEN       0xC0
#define DMP_DIS_ORIEN_SHIFT      6

#define MPU6500_GYRO_SPC_CFG	0x49
#define MPU6500_REG_BANK_SEL	0x76
#define MPU6500_CFG_SET_BIT	0x20

#define MPU6500_CALIB_FILENAME	"//data//mpu6500.cal"
#define MPU6500_ACCEL_CAL_PATH	"/efs/calibration_data"
#define MPU6500_GYRO_CAL_PATH	"/efs/gyro_cal_data"
#define MODULE_NAME_ACCEL	"accelerometer_sensor"
#define MODULE_NAME_GYRO	"gyro_sensor"

struct mpu6500_v {
	union {
		s16 v[3];
		struct {
			s16 x;
			s16 y;
			s16 z;
		};
	};
};

struct motion_int_data {
	unsigned char pwr_mnt[2];
	unsigned char cfg;
	unsigned char accel_cfg;
	unsigned char gyro_cfg;
	unsigned char int_cfg;
	unsigned char smplrt_div;
	bool is_set;
	unsigned char accel_cfg2;
};

struct mpu6500_input_data {
	struct i2c_client *client;
	struct input_dev *accel_input;
	struct input_dev *gyro_input;
	struct motion_int_data mot_data;
	struct mutex mutex;
	struct wake_lock reactive_wake_lock;
	atomic_t accel_enable;
	ktime_t accel_delay;

	struct mpu6500_v acc_data;
	struct mpu6500_v gyro_data;

	atomic_t gyro_enable;
	ktime_t gyro_delay;
	atomic_t reactive_state;
	atomic_t reactive_enable;

	atomic_t motion_recg_enable;
	unsigned long motion_recg_st_time; //start-up time of motion interrupt

	unsigned char gyro_pwr_mgnt[2];
	unsigned char int_pin_cfg;

	u16 enabled_sensors;
	u16 sleep_sensors;
	u32 chip_pos;
	int current_delay;
	int irq;
	int count_logtime;
	int count_logtime_gyro;

	int gyro_bias[3];

	u8 mode;
	struct mpu6500_platform_data *pdata;

#ifdef CONFIG_INPUT_MPU6500_POLLING
	struct delayed_work accel_work;
	struct delayed_work gyro_work;
#endif
	struct device *accel_sensor_device;
	struct device *gyro_sensor_device;
	s16 acc_cal[3];
	bool factory_mode;
	struct regulator *lvs1_1p8;
	struct regulator *str_l19;
};

struct mpu6500_input_cfg {
	int dummy;
};
static struct mpu6500_input_data *gb_mpu_data;

static int mpu6500_input_activate_devices(struct mpu6500_input_data *data,
					  int sensors, bool enable);


static void mpu6500_proc_msleep(unsigned int msecs,
	struct hrtimer_sleeper *sleeper, int sigs)
{
	enum hrtimer_mode mode = HRTIMER_MODE_REL;
	int state = sigs ? TASK_INTERRUPTIBLE : TASK_UNINTERRUPTIBLE;

	hrtimer_init(&sleeper->timer, CLOCK_MONOTONIC, mode);
	sleeper->timer._softexpires = ktime_set(0, msecs*NSEC_PER_MSEC);
	hrtimer_init_sleeper(sleeper, current);

	do {
		set_current_state(state);
		hrtimer_start(&sleeper->timer,
				sleeper->timer._softexpires, mode);
		if (sleeper->task)
			schedule();
		hrtimer_cancel(&sleeper->timer);
		mode = HRTIMER_MODE_ABS;
	} while (sleeper->task && !(sigs && signal_pending(current)));
}

void mpu6500_msleep(unsigned int msecs)
{
	struct hrtimer_sleeper sleeper;

	mpu6500_proc_msleep(msecs, &sleeper, 0);
}

void mpu6500_msleep_interruptible(unsigned int msecs)
{
	struct hrtimer_sleeper sleeper;

	mpu6500_proc_msleep(msecs, &sleeper, 1);
}

int mpu6500_i2c_write(struct i2c_client *i2c_client,
		      unsigned int len, unsigned char *data)
{
	struct i2c_msg msgs[1];
	int res;

	if (NULL == data || NULL == i2c_client)
		return -EINVAL;

	msgs[0].addr = i2c_client->addr;
	msgs[0].flags = 0;	/* write */
	msgs[0].buf = (unsigned char *)data;
	msgs[0].len = len;

	res = i2c_transfer(i2c_client->adapter, msgs, 1);
	if (res < 1)
		return res;
	else
		return 0;
}

int mpu6500_i2c_read(struct i2c_client *i2c_client,
		     unsigned int len, unsigned char *data)
{
	struct i2c_msg msgs[2];
	int res;

	if (NULL == data || NULL == i2c_client)
		return -EINVAL;

	msgs[0].addr = i2c_client->addr;
	msgs[0].flags = I2C_M_RD;
	msgs[0].buf = data;
	msgs[0].len = len;

	res = i2c_transfer(i2c_client->adapter, msgs, 1);
	if (res < 1)
		return res;
	else
		return 0;
}

int mpu6500_i2c_write_single_reg(struct i2c_client *i2c_client,
				 unsigned char reg, unsigned char value)
{

	unsigned char data[2];

	data[0] = reg;
	data[1] = value;

	return mpu6500_i2c_write(i2c_client, 2, data);
}

int mpu6500_i2c_read_reg(struct i2c_client *i2c_client,
			 unsigned char reg, unsigned int len,
			 unsigned char *data)
{
	struct i2c_msg msgs[2];
	int res;

	if (NULL == data || NULL == i2c_client)
		return -EINVAL;

	msgs[0].addr = i2c_client->addr;
	msgs[0].flags = 0;	/* write */
	msgs[0].buf = &reg;
	msgs[0].len = 1;

	msgs[1].addr = i2c_client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].buf = data;
	msgs[1].len = (u16)len;

	res = i2c_transfer(i2c_client->adapter, msgs, 2);
	if (res < 1)
		return res;
	else
		return 0;
}

int mpu6500_i2c_read_fifo(struct i2c_client *i2c_client,
			  unsigned short length, unsigned char *data)
{
	int result;
	unsigned short bytes_read = 0;

	while (bytes_read < length) {
		unsigned short this_len = length - bytes_read;

		result =
		    mpu6500_i2c_read_reg(i2c_client, MPUREG_FIFO_R_W, this_len,
					 &data[bytes_read]);
		if (result) {
			return result;
		}
		bytes_read += this_len;
	}

	return 0;
}

int mpu6500_i2c_memory_write(struct i2c_client *i2c_client,
			unsigned short mem_addr, unsigned int len, unsigned char const *data)
{
	unsigned char bank[2];
	unsigned char addr[2];
	unsigned char buf[513];

	struct i2c_msg msgs[3];
	int res;

	if (!data || !i2c_client)
		return -EINVAL;

	if (len >= (sizeof(buf) - 1))
		return -ENOMEM;

	bank[0] = MPUREG_BANK_SEL;
	bank[1] = mem_addr >> 8;

	addr[0] = MPUREG_MEM_START_ADDR;
	addr[1] = mem_addr & 0xFF;

	buf[0] = MPUREG_MEM_R_W;
	memcpy(buf + 1, data, len);

	/* write message */
	msgs[0].addr = i2c_client->addr;
	msgs[0].flags = 0;
	msgs[0].buf = bank;
	msgs[0].len = sizeof(bank);

	msgs[1].addr = i2c_client->addr;
	msgs[1].flags = 0;
	msgs[1].buf = addr;
	msgs[1].len = sizeof(addr);

	msgs[2].addr = i2c_client->addr;
	msgs[2].flags = 0;
	msgs[2].buf = (unsigned char *)buf;
	msgs[2].len = len + 1;

	res = i2c_transfer(i2c_client->adapter, msgs, 3);
	if (res != 3) {
		if (res >= 0)
			res = -EIO;
		return res;
	} else {
		return 0;
	}

}

int mpu6500_i2c_memory_read(struct i2c_client *i2c_client,
		unsigned short mem_addr, unsigned int len, unsigned char *data)
{
	unsigned char bank[2];
	unsigned char addr[2];
	unsigned char buf;

	struct i2c_msg msgs[4];
	int res;

	if (!data || !i2c_client)
		return -EINVAL;

	bank[0] = MPUREG_BANK_SEL;
	bank[1] = mem_addr >> 8;

	addr[0] = MPUREG_MEM_START_ADDR;
	addr[1] = mem_addr & 0xFF;

	buf = MPUREG_MEM_R_W;

	/* write message */
	msgs[0].addr = i2c_client->addr;
	msgs[0].flags = 0;
	msgs[0].buf = bank;
	msgs[0].len = sizeof(bank);

	msgs[1].addr = i2c_client->addr;
	msgs[1].flags = 0;
	msgs[1].buf = addr;
	msgs[1].len = sizeof(addr);

	msgs[2].addr = i2c_client->addr;
	msgs[2].flags = 0;
	msgs[2].buf = &buf;
	msgs[2].len = 1;

	msgs[3].addr = i2c_client->addr;
	msgs[3].flags = I2C_M_RD;
	msgs[3].buf = data;
	msgs[3].len = len;

	res = i2c_transfer(i2c_client->adapter, msgs, 4);
	if (res != 4) {
		if (res >= 0)
			res = -EIO;
		return res;
	} else {
		return 0;
	}

}

static int mpu6500_input_set_mode(struct mpu6500_input_data *data, u8 mode)
{
	int err = 0;
	data->mode = mode;

	if (mode == MPU6500_MODE_SLEEP) {
		err = mpu6500_input_activate_devices(data,
			MPU6500_SENSOR_ACCEL | MPU6500_SENSOR_GYRO, false);
	} else if (mode == MPU6500_MODE_NORMAL) {
		if (atomic_read(&data->accel_enable))
			err = mpu6500_input_activate_devices(data,
				MPU6500_SENSOR_ACCEL, true);
		if (atomic_read(&data->gyro_enable))
			err = mpu6500_input_activate_devices(data,
				MPU6500_SENSOR_GYRO, true);
	}
	return err;
}

static void mpu6500_input_report_accel_xyz(struct mpu6500_input_data *data)
{
	u8 regs[6];
	int result;

	result = mpu6500_i2c_read_reg(data->client, MPUREG_ACCEL_XOUT_H, 6, regs);
	if (result) {
		pr_err("[SENSOR] %s: i2c_read err= %d\n", __func__, result);
		return;
	}

	data->acc_data.x = ((s16) ((s16) regs[0] << 8)) | regs[1];
	data->acc_data.y = ((s16) ((s16) regs[2] << 8)) | regs[3];
	data->acc_data.z = ((s16) ((s16) regs[4] << 8)) | regs[5];

	remap_sensor_data(data->acc_data.v, data->chip_pos);

	input_report_rel(data->accel_input, REL_X, data->acc_data.x - data->acc_cal[0]);
	input_report_rel(data->accel_input, REL_Y, data->acc_data.y - data->acc_cal[1]);
	input_report_rel(data->accel_input, REL_Z, data->acc_data.z - data->acc_cal[2]);

	if ((ktime_to_ns(data->accel_delay) * (int64_t)data->count_logtime)
		>= ((int64_t)MPU6500_LOGTIME * NSEC_PER_SEC)) {
		pr_info("[SENSOR] %s, %d, %d, %d (Count = %d)\n",
			__func__, data->acc_data.x, data->acc_data.y, data->acc_data.z, data->count_logtime);
		data->count_logtime = 0;
	} else
		data->count_logtime++;
	input_sync(data->accel_input);
}

static void mpu6500_input_report_gyro_xyz(struct mpu6500_input_data *data)
{
	u8 regs[6];
	s16 raw_tmp[3];
	int result;

	result = mpu6500_i2c_read_reg(data->client, MPUREG_GYRO_XOUT_H, 6, regs);

	if (result) {
		pr_err("[SENSOR] %s: i2c_read err= %d\n", __func__, result);
		return;
	}

	raw_tmp[0] = (((s16) ((s16) regs[0] << 8)) | regs[1]);
	raw_tmp[1] = (((s16) ((s16) regs[2] << 8)) | regs[3]);
	raw_tmp[2] = (((s16) ((s16) regs[4] << 8)) | regs[5]);

	data->gyro_data.x = raw_tmp[0] - (s16) data->gyro_bias[0];
	data->gyro_data.y = raw_tmp[1] - (s16) data->gyro_bias[1];
	data->gyro_data.z = raw_tmp[2] - (s16) data->gyro_bias[2];

	if (!(data->gyro_data.x >> 15 == raw_tmp[0] >> 15) &&\
		!((s16) data->gyro_bias[0] >> 15 == raw_tmp[0] >> 15)) {
		pr_info("[SENSOR] %s GYRO X is overflowed!!!\n", __func__);
		data->gyro_data.x = (data->gyro_data.x >= 0 ? MIN_GYRO : MAX_GYRO);

	}
	if (!(data->gyro_data.y >> 15 == raw_tmp[1] >> 15) &&\
		!((s16) data->gyro_bias[1] >> 15 == raw_tmp[1] >> 15)) {
		pr_info("[SENSOR] %s GYRO Y is overflowed!!!\n", __func__);
		data->gyro_data.y = (data->gyro_data.y >= 0 ? MIN_GYRO : MAX_GYRO);

	}
	if (!(data->gyro_data.z >> 15 == raw_tmp[2] >> 15) &&\
		!((s16) data->gyro_bias[2] >> 15 == raw_tmp[2] >> 15)) {
		pr_info("[SENSOR] %s GYRO Z is overflowed!!!\n", __func__);
		data->gyro_data.z = (data->gyro_data.z >= 0 ? MIN_GYRO : MAX_GYRO);
	}

	remap_sensor_data(data->gyro_data.v, data->chip_pos);

	input_report_rel(data->gyro_input, REL_RX, data->gyro_data.x);
	input_report_rel(data->gyro_input, REL_RY, data->gyro_data.y);
	input_report_rel(data->gyro_input, REL_RZ, data->gyro_data.z);


	if ((ktime_to_ns(data->gyro_delay) * (int64_t)data->count_logtime)
		>= ((int64_t)MPU6500_LOGTIME * NSEC_PER_SEC)) {
		pr_info("[SENSOR] %s, %d, %d, %d (Count = %d)\n",
			__func__, data->gyro_data.x, data->gyro_data.y, data->gyro_data.z, data->count_logtime_gyro);
		data->count_logtime_gyro = 0;
	} else
		data->count_logtime_gyro++;
	input_sync(data->gyro_input);
}

static irqreturn_t mpu6500_input_irq_thread(int irq, void *dev)
{
	struct mpu6500_input_data *data = (struct mpu6500_input_data *)dev;
	struct motion_int_data *mot_data = &data->mot_data;
	unsigned char reg;
	unsigned long timediff = 0;
	int result;

	if (!atomic_read(&data->reactive_enable)) {
#ifdef CONFIG_INPUT_MPU6500_LP
		if (IS_LP_ENABLED(data->enabled_sensors))
			mpu6500_input_report_fifo_data(data);
		else {
#ifndef CONFIG_INPUT_MPU6500_POLLING
			if (data->enabled_sensors & MPU6500_SENSOR_ACCEL)
				mpu6500_input_report_accel_xyz(data);
			if (data->enabled_sensors & MPU6500_SENSOR_GYRO)
				mpu6500_input_report_gyro_xyz(data);
#endif
		}
#else
#ifndef CONFIG_INPUT_MPU6500_POLLING
		if (data->enabled_sensors & MPU6500_SENSOR_ACCEL)
			mpu6500_input_report_accel_xyz(data);

		if (data->enabled_sensors & MPU6500_SENSOR_GYRO)
			mpu6500_input_report_gyro_xyz(data);
#endif
#endif
	} else {
		result = mpu6500_i2c_read_reg(data->client,
			MPUREG_INT_STATUS, 1, &reg);
		if (result) {
			pr_err("[SENSOR] %s: i2c_read err= %d\n", __func__, result);
			goto done;
		}

		timediff = jiffies_to_msecs(jiffies - data->motion_recg_st_time);
		/* ignore motion interrupt happened in 100ms to skip intial erronous interrupt */
		if (timediff < 1000 && !(data->factory_mode)) {
			pr_debug("[SENSOR] %s: timediff = %ld msec\n",
				__func__, timediff);
			goto done;
		}
		if (reg & (1 << 6) || data->factory_mode) {
			/* handle motion recognition */
			atomic_set(&data->reactive_state, true);
			data->factory_mode = false;
			pr_info("[SENSOR] %s: motion interrupt happened\n", __func__);
			/* disable motion int */
                   mpu6500_i2c_write_single_reg(data->client, MPUREG_INT_ENABLE, mot_data->int_cfg);
			wake_lock_timeout(&data->reactive_wake_lock, msecs_to_jiffies(2000));
		}
	}
done:
	return IRQ_HANDLED;
}

static int mpu6500_input_set_fsr(struct mpu6500_input_data *data, int fsr)
{
	unsigned char fsr_mask;
	int result;
	unsigned char reg;

	if (fsr <= 2000) {
		fsr_mask = 0x00;
	} else if (fsr <= 4000) {
		fsr_mask = 0x08;
	} else if (fsr <= 8000) {
		fsr_mask = 0x10;
	} else {		/* fsr = [8001, oo) */
		fsr_mask = 0x18;
	}

	result =
	    mpu6500_i2c_read_reg(data->client, MPUREG_ACCEL_CONFIG, 1, &reg);
	if (result) {
		LOG_RESULT_LOCATION(result);
		return result;
	}
	result =
	    mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_CONFIG,
					 reg | fsr_mask);
	if (result) {
		LOG_RESULT_LOCATION(result);
		return result;
	}

	return result;
}

#ifdef CONFIG_MPU6500_LP_MODE
static int mpu6500_input_set_lp_mode(struct mpu6500_input_data *data,
				     unsigned char lpa_freq)
{
	unsigned char b = 0;
	/* Reducing the duration setting for lp mode */
	b = 0x1;
	mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_INT_ENABLE, b);
	/* Setting the cycle bit and LPA wake up freq */
	mpu6500_i2c_read_reg(data->client, MPUREG_PWR_MGMT_1, 1, &b);
	b |= BIT_CYCLE | BIT_PD_PTAT;
	mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_1, b);
	mpu6500_i2c_read_reg(data->client, MPUREG_PWR_MGMT_2, 1, &b);
	b |= lpa_freq & BITS_LPA_WAKE_CTRL;
	mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_2, b);

	return 0;
}
#endif
static int mpu6500_input_set_fp_mode(struct mpu6500_input_data *data)
{
	unsigned char b;

	/* Resetting the cycle bit and LPA wake up freq */
	mpu6500_i2c_read_reg(data->client, MPUREG_PWR_MGMT_1, 1, &b);
	b &= ~BIT_CYCLE & ~BIT_PD_PTAT;
	mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_1, b);
	mpu6500_i2c_read_reg(data->client, MPUREG_PWR_MGMT_2, 1, &b);
	b &= ~BITS_LPA_WAKE_CTRL;
	mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_2, b);
	/* Resetting the duration setting for fp mode */
	b = (unsigned char)10 / ACCEL_MOT_DUR_LSB;
	mpu6500_i2c_write_single_reg(data->client,
					MPUREG_ACCEL_INTEL_ENABLE, b);

	return 0;
}

static int mpu6500_input_set_odr(struct mpu6500_input_data *data, int odr)
{
	int result;
	unsigned char b;

	if (!data->enabled_sensors)
		return 0;

	b = (unsigned char)(odr);

	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_SMPLRT_DIV, b));

	mpu6500_i2c_read_reg(data->client, MPUREG_PWR_MGMT_1, 1, &b);
	b &= BIT_CYCLE;
	if (b == BIT_CYCLE) {
	  printk(KERN_INFO " Accel LP - > FP mode. \n ");
	  mpu6500_input_set_fp_mode(data);
	}

	return result;
}

#if defined(CONFIG_MPU6500_ADJUST_SMART_ALERT)
static int mpu6500_input_set_motion_interrupt(struct mpu6500_input_data *data,
					      int enable, bool factory_test)
{
	struct motion_int_data *mot_data = &data->mot_data;
	unsigned char reg;

	atomic_set(&data->reactive_state, false);

	if (enable) {
		mpu6500_i2c_read_reg(data->client, MPUREG_INT_STATUS, 1, &reg);
		printk(KERN_INFO "@@Initialize motion interrupt : INT_STATUS=%x\n", reg);

		reg = 0x01;		// Make cycle and sleep bit 0
		mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_1, reg);
		msleep(50);

		reg = 0x0;		// Clear gyro and accel config
		mpu6500_i2c_write_single_reg(data->client, MPUREG_CONFIG, reg);
		mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_CONFIG, reg);

		reg = 0x08;		// Set accel fchoice 0 to use lp accel low power odr
		mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_CONFIG2, reg);

		reg = 0x05;		// Set frequency of wake up (7.81Hz)
		mpu6500_i2c_write_single_reg(data->client, MPUREG_LP_ACCEL_ODR, reg);

		if (factory_test)
			reg = 0x41;		// Enable motion interrupt and raw data ready
		else
			reg = 0x40;     // Enable motion interrupt
		mpu6500_i2c_write_single_reg(data->client, MPUREG_INT_ENABLE, reg);

		reg = 0xC0;		// Enable wake on motion detection logic
		mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_INTEL_CTRL, reg);

		if (factory_test)
			reg = 0x00;
		else
			reg = 0x30;		// Set Motion Threshold. (1LSB = 4mg)
		mpu6500_i2c_write_single_reg(data->client, MPUREG_WOM_THR, reg);

		if (!factory_test) {
			reg = 0x07;	// Put gyro in standby and accel running
			mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_2, reg);

			reg = 0x1;
			reg |= 0x20;	// Set the cycle bit to be 1 (LP Mode)
			mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_1, reg);
		}
		data->motion_recg_st_time = jiffies;
	} else {
		if (mot_data->is_set) {
			mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_1,
				mot_data->pwr_mnt[0]);
			msleep(50);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_2,
				mot_data->pwr_mnt[1]);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_CONFIG,
				mot_data->cfg);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_CONFIG,
				mot_data->accel_cfg);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_CONFIG2,
				mot_data->accel_cfg2);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_GYRO_CONFIG,
				mot_data->gyro_cfg);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_INT_ENABLE,
				mot_data->int_cfg);
			reg = 0xff; /* Motion Duration =1 ms */
			mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_INTEL_ENABLE, reg);
			/* Motion Threshold =1mg, based on the data sheet. */
			reg = 0xff;
			mpu6500_i2c_write_single_reg(data->client, MPUREG_WOM_THR, reg);
			mpu6500_i2c_read_reg(data->client, MPUREG_INT_STATUS, 1, &reg);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_SMPLRT_DIV, mot_data->smplrt_div);
			pr_info("%s: disable interrupt\n", __func__);
		}
	}
	mot_data->is_set = enable;

	return 0;
}
#else
static int mpu6500_input_set_motion_interrupt(struct mpu6500_input_data *data,
					      int enable, bool factory_test)
{
	struct motion_int_data *mot_data = &data->mot_data;
	unsigned char reg;

	atomic_set(&data->reactive_state, false);

	if (enable) {
		/* 1) initialize */
		mpu6500_i2c_read_reg(data->client, MPUREG_INT_STATUS, 1, &reg);
		printk(KERN_INFO "@@Initialize motion interrupt : INT_STATUS=%x\n", reg);


		/* Power up the chip and clear the cycle bit. Full power */
		reg = 0x01;
		mpu6500_i2c_write_single_reg(data->client,
					     MPUREG_PWR_MGMT_1, reg);
		mdelay(50);

		/* 2. mpu& accel config */
		if (factory_test)
			reg = 0x0; /*260Hz LPF */
		else
			reg = 0x1; /*44Hz LPF */
		mpu6500_i2c_write_single_reg(data->client, MPUREG_CONFIG, reg);

		reg = 0x0; /* Clear Accel Config. */
		mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_CONFIG, reg);

		reg = 0x08;
		mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_CONFIG2, reg);


		/* 3. set motion thr & dur */
		if (factory_test)
			reg = 0x41;	/* Make the motion & drdy enable */
		else
			reg = 0x40;	/* Make the motion interrupt enable */
		mpu6500_i2c_write_single_reg(data->client, MPUREG_INT_ENABLE, reg);

		reg = 4;	// 3.91 Hz (low power accel odr)
		mpu6500_i2c_write_single_reg(data->client, MPUREG_LP_ACCEL_ODR, reg);

		reg = 0xC0;	/* Motion Duration =1 ms */
		mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_INTEL_CTRL, reg);

		/* Motion Threshold =1mg, based on the data sheet. */
		if (factory_test)
			reg = 0x00;
		else
			reg = 0x0C; // 0x4B;
		mpu6500_i2c_write_single_reg(data->client, MPUREG_WOM_THR, reg);

		if (!factory_test) {
			/* 5. */
			/* Steps to setup the lp mode for PWM-2 register */
			reg = mot_data->pwr_mnt[1];
			reg |= (BITS_LPA_WAKE_20HZ); /* the freq of wakeup */
			reg |= 0x07; /* put gyro in standby. */
			reg &= ~(BIT_STBY_XA | BIT_STBY_YA | BIT_STBY_ZA);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_2, reg);

			reg = 0x1;
			reg |= 0x20; /* Set the cycle bit to be 1. LP MODE */
			reg &= ~0x08; /* Clear the temp disp bit. */
			mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_1, reg & ~BIT_SLEEP);
		}
		data->motion_recg_st_time = jiffies;
	} else {
		if (mot_data->is_set) {
			mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_1,
				mot_data->pwr_mnt[0]);
			msleep(50);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_PWR_MGMT_2,
				mot_data->pwr_mnt[1]);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_CONFIG,
				mot_data->cfg);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_CONFIG,
				mot_data->accel_cfg);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_CONFIG2,
				mot_data->accel_cfg2);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_GYRO_CONFIG,
				mot_data->gyro_cfg);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_INT_ENABLE,
				mot_data->int_cfg);
			reg = 0xff; /* Motion Duration =1 ms */
			mpu6500_i2c_write_single_reg(data->client, MPUREG_ACCEL_INTEL_ENABLE, reg);
			/* Motion Threshold =1mg, based on the data sheet. */
			reg = 0xff;
			mpu6500_i2c_write_single_reg(data->client, MPUREG_WOM_THR, reg);
			mpu6500_i2c_read_reg(data->client, MPUREG_INT_STATUS, 1, &reg);
			mpu6500_i2c_write_single_reg(data->client, MPUREG_SMPLRT_DIV, mot_data->smplrt_div);
			pr_info("%s: disable interrupt\n", __func__);
		}
	}
	mot_data->is_set = enable;

	return 0;
}
#endif

static int mpu6500_input_set_irq(struct mpu6500_input_data *data, unsigned char irq)
{
	int result;

	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_INT_ENABLE, irq));

	return result;
}

static int mpu6500_input_suspend_accel(struct mpu6500_input_data *data)
{
	unsigned char reg;
	int result;

	CHECK_RESULT(mpu6500_i2c_read_reg
		     (data->client, MPUREG_PWR_MGMT_2, 1, &reg));

	reg |= (BIT_STBY_XA | BIT_STBY_YA | BIT_STBY_ZA);
	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_PWR_MGMT_2, reg));

	return result;
}

static int mpu6500_input_resume_accel(struct mpu6500_input_data *data)
{
	int result = 0;
	unsigned char reg;

	CHECK_RESULT(mpu6500_i2c_read_reg
		     (data->client, MPUREG_PWR_MGMT_1, 1, &reg));

	if (reg & BIT_SLEEP) {
		CHECK_RESULT(mpu6500_i2c_write_single_reg(data->client,
							  MPUREG_PWR_MGMT_1,
							  reg & ~BIT_SLEEP));
	}

	msleep(2);

	CHECK_RESULT(mpu6500_i2c_read_reg
		     (data->client, MPUREG_PWR_MGMT_2, 1, &reg));

	reg &= ~(BIT_STBY_XA | BIT_STBY_YA | BIT_STBY_ZA);
	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_PWR_MGMT_2, reg));

	/* settings */

	/*----- LPF configuration  : 41hz ---->*/
	reg = MPU_FILTER_41HZ;
	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_ACCEL_CONFIG2, reg));
	/*<----- LPF configuration  : 44hz ---- */

	CHECK_RESULT(mpu6500_i2c_read_reg
		     (data->client, MPUREG_ACCEL_CONFIG, 1, &reg));
	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_ACCEL_CONFIG, reg | 0x0));
	CHECK_RESULT(mpu6500_input_set_fsr(data, 2000));

	return result;

}

static int mpu6500_input_activate_accel(struct mpu6500_input_data *data,
				       bool enable)
{
	int result;

	if (enable) {
		result = mpu6500_input_resume_accel(data);
		if (result) {
			LOG_RESULT_LOCATION(result);
			return result;
		} else {
			data->enabled_sensors |= MPU6500_SENSOR_ACCEL;
		}
	} else {
		result = mpu6500_input_suspend_accel(data);
		if (result == 0) {
			data->enabled_sensors &= ~MPU6500_SENSOR_ACCEL;
		}
	}

	return result;
}

static int mpu6500_input_suspend_gyro(struct mpu6500_input_data *data)
{
	int result = 0;

	CHECK_RESULT(mpu6500_i2c_read_reg
		     (data->client, MPUREG_PWR_MGMT_1, 2, data->gyro_pwr_mgnt));

	data->gyro_pwr_mgnt[1] |= (BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG);

	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_PWR_MGMT_2, data->gyro_pwr_mgnt[1]));

	return result;
}

static int mpu6500_input_resume_gyro(struct mpu6500_input_data *data)
{
	int result = 0;
	unsigned regs[2] = { 0, };

	CHECK_RESULT(mpu6500_i2c_read_reg
		     (data->client, MPUREG_PWR_MGMT_1, 2, data->gyro_pwr_mgnt));

	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_PWR_MGMT_1,
		      data->gyro_pwr_mgnt[0] & ~BIT_SLEEP));

	data->gyro_pwr_mgnt[1] &= ~(BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG);

	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_PWR_MGMT_2, data->gyro_pwr_mgnt[1]));

	regs[0] = MPU_FS_500DPS << 3;
	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_GYRO_CONFIG, regs[0]));

	regs[0] = MPU_FILTER_41HZ | 0x18;
	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_CONFIG, regs[0]));

	return result;
}

static int mpu6500_input_activate_gyro(struct mpu6500_input_data *data,
				       bool enable)
{
	int result;
	if (enable) {
		result = mpu6500_input_resume_gyro(data);
		if (result) {
			LOG_RESULT_LOCATION(result);
			return result;
		} else {
			data->enabled_sensors |= MPU6500_SENSOR_GYRO;
		}
	} else {
		result = mpu6500_input_suspend_gyro(data);
		if (result == 0) {
			data->enabled_sensors &= ~MPU6500_SENSOR_GYRO;
		}
	}

	return result;
}

static int mpu6500_set_delay(struct mpu6500_input_data *data)
{
	int result = 0;
	int delay = 200;

	if (data->enabled_sensors & MPU6500_SENSOR_ACCEL) {
		delay = MIN(delay, ktime_to_ms(data->accel_delay));
	}

	if (data->enabled_sensors & MPU6500_SENSOR_GYRO) {
		delay = MIN(delay, ktime_to_ms(data->gyro_delay));
	}

	data->current_delay = delay;

	if (data->enabled_sensors & MPU6500_SENSOR_ACCEL ||
		data->enabled_sensors & MPU6500_SENSOR_GYRO) {
		CHECK_RESULT(mpu6500_input_set_odr(data,
			data->current_delay));
#ifndef CONFIG_INPUT_MPU6500_POLLING
		if (!atomic_read(&data->reactive_enable))
			CHECK_RESULT(mpu6500_input_set_irq(data,
				BIT_RAW_RDY_EN));
#endif
	}
	return result;
}

static int mpu6500_input_activate_devices(struct mpu6500_input_data *data,
					  int sensors, bool enable)
{
	int result = 0;

	/* disable reactive alert when any sensors turn on */
	if (atomic_read(&data->motion_recg_enable)) {
		if (enable && (sensors != 0))
			mpu6500_input_set_motion_interrupt(data, false, false);
	}

	if (sensors & MPU6500_SENSOR_ACCEL) {
		CHECK_RESULT(mpu6500_input_activate_accel(data, enable));
	}
	if (sensors & MPU6500_SENSOR_GYRO) {
		CHECK_RESULT(mpu6500_input_activate_gyro(data, enable));
	}

	if (data->enabled_sensors) {
		CHECK_RESULT(mpu6500_set_delay(data));
	} else {
		unsigned char reg;
		data->accel_delay = ns_to_ktime(MPU6500_DEFAULT_DELAY);
		data->gyro_delay = ns_to_ktime(MPU6500_DEFAULT_DELAY);
		/*enable reactive alert when all sensors go off*/
		if (atomic_read(&data->motion_recg_enable))
			mpu6500_input_set_motion_interrupt(data, true,
				data->factory_mode);
		else {
			CHECK_RESULT(mpu6500_input_set_irq(data, 0x0));

			CHECK_RESULT(mpu6500_i2c_read_reg(data->client,
				MPUREG_PWR_MGMT_1, 1, &reg));

			if (!(reg & BIT_SLEEP))
				CHECK_RESULT(mpu6500_i2c_write_single_reg(
					data->client, MPUREG_PWR_MGMT_1,
					 reg | BIT_SLEEP));
		}
	}

	return result;
}
static void mpu6500_set_65XX_gyro_config(struct i2c_client *i2c_client)
{

	unsigned char d, cfg;

	mpu6500_i2c_read_reg(i2c_client, MPU6500_REG_BANK_SEL, 1, &cfg);

	mpu6500_i2c_write_single_reg(i2c_client, MPU6500_REG_BANK_SEL,
			cfg | MPU6500_CFG_SET_BIT);

	mpu6500_i2c_read_reg(i2c_client, MPU6500_GYRO_SPC_CFG, 1, &d);

	d |= 1;
	mpu6500_i2c_write_single_reg(i2c_client, MPU6500_GYRO_SPC_CFG, d);

	mpu6500_i2c_write_single_reg(i2c_client, MPU6500_REG_BANK_SEL, cfg);
}

static int __devinit mpu6500_input_initialize(struct mpu6500_input_data *data, const struct mpu6500_input_cfg
					      *cfg)
{
	int result;

	data->int_pin_cfg = BIT_INT_ANYRD_2CLEAR;

	data->current_delay = -1;
	data->enabled_sensors = 0;

	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_PWR_MGMT_1, BIT_H_RESET));
	msleep(100);

	CHECK_RESULT(mpu6500_i2c_write_single_reg
		     (data->client, MPUREG_INT_PIN_CFG,
		      data->int_pin_cfg | BIT_BYPASS_EN));

	mpu6500_set_65XX_gyro_config(data->client);

	return mpu6500_input_set_mode(data, MPU6500_MODE_SLEEP);
}

static int accel_open_calibration(struct mpu6500_input_data *data)
{
	struct file *cal_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(MPU6500_ACCEL_CAL_PATH,
		O_RDONLY, S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("[SENSOR] %s: - Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->read(cal_filp,
		(char *)&data->acc_cal,
			3 * sizeof(s16), &cal_filp->f_pos);
	if (err != 3 * sizeof(s16)) {
		pr_err("[SENSOR] %s: - Can't read the cal data from file\n", __func__);
		err = -EIO;
	}

	pr_info("[SENSOR] %s: - (%d,%d,%d)\n", __func__,
		data->acc_cal[0], data->acc_cal[1], data->acc_cal[2]);

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}

static ssize_t mpu6500_input_accel_enable_show(struct device *dev,
					       struct device_attribute *attr,
					       char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6500_input_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->accel_enable));

}

static ssize_t mpu6500_input_accel_enable_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6500_input_data *data = input_get_drvdata(input_data);
#ifdef CONFIG_INPUT_MPU6500_POLLING
	struct motion_int_data *mot_data = &data->mot_data;
#endif
	int value = simple_strtoul(buf, NULL, 10);

	if (value == 1)
		accel_open_calibration(data);

	pr_info("[SENSOR] %s : enable = %d\n", __func__, value);

	mutex_lock(&data->mutex);

#ifdef CONFIG_INPUT_MPU6500_POLLING
	if (value && !atomic_read(&data->accel_enable)) {
		if (mot_data->is_set)
			mpu6500_input_set_motion_interrupt(
				data, false, data->factory_mode);

		mpu6500_input_activate_devices(data,
			MPU6500_SENSOR_ACCEL, true);

		schedule_delayed_work(&data->accel_work, msecs_to_jiffies(ktime_to_ms(data->accel_delay)));
	}
	if (!value && atomic_read(&data->accel_enable)) {
		cancel_delayed_work_sync(&data->accel_work);
		mpu6500_input_activate_devices(data,
			MPU6500_SENSOR_ACCEL, false);
		if (atomic_read(&data->reactive_enable))
			mpu6500_input_set_motion_interrupt(
				data, true, data->factory_mode);
	}
#else
	if (value && !atomic_read(&data->accel_enable)) {
		mpu6500_input_activate_devices(data, MPU6500_SENSOR_ACCEL, true);
	}
	if (!value && atomic_read(&data->accel_enable)) {
		mpu6500_input_activate_devices(data, MPU6500_SENSOR_ACCEL, false);
	}
#endif
	atomic_set(&data->accel_enable, value);
	mutex_unlock(&data->mutex);

	return count;
}

static ssize_t mpu6500_input_accel_delay_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6500_input_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%lld\n", ktime_to_ns(data->accel_delay));

}

static ssize_t mpu6500_input_accel_delay_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6500_input_data *data = input_get_drvdata(input_data);
	int64_t delay;
	int ret = 0;

	ret = kstrtoll(buf, 10, &delay);
	if (ret) {
		pr_err("[SENSOR]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	pr_info("[SENSOR] %s : delay = %lld\n", __func__, delay);

	mutex_lock(&data->mutex);

    data->accel_delay = ns_to_ktime(delay);

	mpu6500_set_delay(data);

	mutex_unlock(&data->mutex);

	return count;
}

static int gyro_open_calibration(struct mpu6500_input_data *data)
{
	struct file *cal_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(MPU6500_GYRO_CAL_PATH,
		O_RDONLY, S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("[SENSOR] %s: - Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->read(cal_filp,
		(char *)&data->gyro_bias, 3 * sizeof(int),
			&cal_filp->f_pos);
	if (err != 3 * sizeof(int)) {
		pr_err("[SENSOR] %s: - Can't read the cal data from file\n", __func__);
		err = -EIO;
	}

	pr_info("[SENSOR] %s: - (%d,%d,%d)\n", __func__,
		data->gyro_bias[0], data->gyro_bias[1],	data->gyro_bias[2]);

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}

static int gyro_do_calibrate(struct mpu6500_input_data *data)
{
	struct file *cal_filp;
	int err;
	mm_segment_t old_fs;

	/* selftest was doing 2000dps condition, change to 500dps */
	data->gyro_bias[0] = data->gyro_bias[0] << 2;
	data->gyro_bias[1] = data->gyro_bias[1] << 2;
	data->gyro_bias[2] = data->gyro_bias[2] << 2;

	pr_info("[SENSOR] %s: - cal data (%d,%d,%d)\n", __func__,
		data->gyro_bias[0], data->gyro_bias[1], data->gyro_bias[2]);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(MPU6500_GYRO_CAL_PATH,
			O_CREAT | O_TRUNC | O_WRONLY,
			S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("[SENSOR] %s: - Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&data->gyro_bias, 3 * sizeof(int),
			&cal_filp->f_pos);
	if (err != 3 * sizeof(int)) {
		pr_err("[SENSOR] %s: - Can't write the cal data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}


static ssize_t mpu6500_input_gyro_enable_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6500_input_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->gyro_enable));

}

static ssize_t mpu6500_input_gyro_enable_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6500_input_data *data = input_get_drvdata(input_data);
#ifdef CONFIG_INPUT_MPU6500_POLLING
	struct motion_int_data *mot_data = &data->mot_data;
#endif
	int value = simple_strtoul(buf, NULL, 10);

	if (value == 1)
		gyro_open_calibration(data);

	pr_info("[SENSOR] %s : enable = %d\n", __func__, value);
	mutex_lock(&data->mutex);

#ifdef CONFIG_INPUT_MPU6500_POLLING
	if (value && !atomic_read(&data->gyro_enable)) {
		if (mot_data->is_set)
			mpu6500_input_set_motion_interrupt(
				data, false, data->factory_mode);
		mpu6500_input_activate_devices(data,
			MPU6500_SENSOR_GYRO, true);
		schedule_delayed_work(&data->gyro_work,
			msecs_to_jiffies(ktime_to_ms(data->gyro_delay)));
	}
	if (!value && atomic_read(&data->gyro_enable)) {
		cancel_delayed_work_sync(&data->gyro_work);
		mpu6500_input_activate_devices(data,
			MPU6500_SENSOR_GYRO, false);
		if (atomic_read(&data->reactive_enable))
			mpu6500_input_set_motion_interrupt(
				data, true, data->factory_mode);
	}
#endif

	atomic_set(&data->gyro_enable, value);
	mutex_unlock(&data->mutex);

	return count;
}

static ssize_t mpu6500_input_gyro_delay_show(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6500_input_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%lld\n", ktime_to_ns(data->gyro_delay));
}

static ssize_t mpu6500_input_gyro_delay_store(struct device *dev,
					      struct device_attribute *attr,
					      const char *buf, size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6500_input_data *data = input_get_drvdata(input_data);
	int64_t delay;
	int ret = 0;

	ret = kstrtoll(buf, 10, &delay);
	if (ret) {
		pr_err("[SENSOR]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	pr_info("[SENSOR] %s : delay = %lld\n", __func__, delay);

	mutex_lock(&data->mutex);

    data->gyro_delay = ns_to_ktime(delay);

	mpu6500_set_delay(data);

	mutex_unlock(&data->mutex);

	return count;
}

static ssize_t mpu6500_input_motion_recg_enable_show(struct device *dev,
						     struct device_attribute
						     *attr, char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6500_input_data *data = input_get_drvdata(input_data);

	return sprintf(buf, "%d\n", atomic_read(&data->motion_recg_enable));

}

static ssize_t mpu6500_input_motion_recg_enable_store(struct device *dev,
						      struct device_attribute
						      *attr, const char *buf,
						      size_t count)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6500_input_data *data = input_get_drvdata(input_data);

	int value = simple_strtoul(buf, NULL, 10);

	mutex_lock(&data->mutex);

	atomic_set(&data->motion_recg_enable, value);

	mpu6500_input_set_motion_interrupt(data, value, false);

	mutex_unlock(&data->mutex);

	return count;
}

static ssize_t mpu6500_input_gyro_self_test_show(struct device *dev,
						 struct device_attribute *attr,
						 char *buf)
{
	struct input_dev *input_data = to_input_dev(dev);
	struct mpu6500_input_data *data = input_get_drvdata(input_data);

	int scaled_gyro_bias[3] = { 0 };	//absolute gyro bias scaled by 1000 times. (gyro_bias x 1000)
	int scaled_gyro_rms[3] = { 0 };	//absolute gyro rms scaled by 1000 times. (gyro_bias x 1000)
	int packet_count[3] = { 0 };
	int result;

	mutex_lock(&data->mutex);

	result = mpu6500_selftest_run(data->client,
				      packet_count,
				      scaled_gyro_bias,
				      scaled_gyro_rms, data->gyro_bias);
	if (!result) {
		//store calibration to file
		gyro_do_calibrate(data);
	} else {
		data->gyro_bias[0] = 0;
		data->gyro_bias[1] = 0;
		data->gyro_bias[2] = 0;
	}

	mutex_unlock(&data->mutex);

	return sprintf(buf, "%d "
		       "%d %d %d "
		       "%d.%03d %d.%03d %d.%03d "
		       "%d.%03d %d.%03d %d.%03d ",
		       result,
		       packet_count[0], packet_count[1], packet_count[2],
		       (int)abs(scaled_gyro_bias[0] / 1000),
		       (int)abs(scaled_gyro_bias[0]) % 1000,
		       (int)abs(scaled_gyro_bias[1] / 1000),
		       (int)abs(scaled_gyro_bias[1]) % 1000,
		       (int)abs(scaled_gyro_bias[2] / 1000),
		       (int)abs(scaled_gyro_bias[2]) % 1000,
		       scaled_gyro_rms[0] / 1000,
		       (int)abs(scaled_gyro_rms[0]) % 1000,
		       scaled_gyro_rms[1] / 1000,
		       (int)abs(scaled_gyro_rms[1]) % 1000,
		       scaled_gyro_rms[2] / 1000,
		       (int)abs(scaled_gyro_rms[2]) % 1000);
}

static struct device_attribute dev_attr_acc_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	mpu6500_input_accel_enable_show, mpu6500_input_accel_enable_store);
static struct device_attribute dev_attr_gyro_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	mpu6500_input_gyro_enable_show, mpu6500_input_gyro_enable_store);
static struct device_attribute dev_attr_acc_delay =
	__ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
	mpu6500_input_accel_delay_show, mpu6500_input_accel_delay_store);
static struct device_attribute dev_attr_gyro_delay =
	__ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
	mpu6500_input_gyro_delay_show, mpu6500_input_gyro_delay_store);
static DEVICE_ATTR(self_test, S_IRUGO, mpu6500_input_gyro_self_test_show, NULL);
static DEVICE_ATTR(mot_recg_enable, S_IRUGO | S_IWUSR | S_IWGRP,
		   mpu6500_input_motion_recg_enable_show,
		   mpu6500_input_motion_recg_enable_store);

static struct attribute *accel_attributes[] = {
	&dev_attr_acc_enable.attr,
	&dev_attr_acc_delay.attr,
	&dev_attr_mot_recg_enable.attr,
	NULL
};

static struct attribute *gyro_attributes[] = {
	&dev_attr_gyro_enable.attr,
	&dev_attr_gyro_delay.attr,
	&dev_attr_self_test.attr,
	NULL
};

static struct attribute_group accel_attribute_group = {
	.attrs = accel_attributes
};

static struct attribute_group gyro_attribute_group = {
	.attrs = gyro_attributes
};

void  mpu6500_vdd_on(struct mpu6500_input_data *data, bool onoff)
{
	int ret;
	if (data->str_l19 == NULL) {
		data->str_l19 =regulator_get(&data->client->dev, "8226_l19");
		if (IS_ERR(data->str_l19)){
			pr_err("%s: regulator_get failed for 8226_l19\n", __func__);
			return ;
		}
		ret = regulator_set_voltage(data->str_l19, 2850000, 2850000);
		if (ret)
			pr_err("%s: error vsensor_2p85 setting voltage ret=%d\n",__func__, ret);
	}
	if (data->lvs1_1p8 == NULL) {
		data->lvs1_1p8 = regulator_get(&data->client->dev, "8226_lvs1");
		if (IS_ERR(data->lvs1_1p8)){
			pr_err("%s: regulator_get failed for 8226_l6\n", __func__);
			return ;
		}
		ret = regulator_set_voltage(data->lvs1_1p8, 1800000, 1800000);
		if (ret)
			pr_err("%s: error vreg_2p8 setting voltage ret=%d\n",__func__, ret);
	}
	if (onoff == 1) {
		ret = regulator_enable(data->str_l19);
		if (ret)
			pr_err("%s: error enablinig regulator info->vdd_2p85\n", __func__);

		ret = regulator_enable(data->lvs1_1p8);
		if (ret)
			pr_err("%s: error enablinig regulator info->vreg_1p8\n", __func__);
		}
	else if (onoff == 0) {
		if (regulator_is_enabled(data->str_l19)) {
			ret = regulator_disable(data->str_l19);
			if (ret)
				pr_err("%s: error vdd_2p85 disabling regulator\n",__func__);
	}
		if (regulator_is_enabled(data->lvs1_1p8)) {
			ret = regulator_disable(data->lvs1_1p8);
			if (ret)
				pr_err("%s: error vreg_1p8 disabling regulator\n",__func__);
		}
		}
	msleep(30);
	return;
}


static void mpu6500_request_gpio(struct mpu6500_platform_data *pdata)
{
	int ret;
	pr_info("[MPU6500] request gpio\n");
	ret = gpio_request(pdata->gpio_scl, "mpu6500_scl");
	if (ret) {
		pr_err("[MPU6500]%s: unable to request mpu6500_scl [%d]\n",
				__func__, pdata->gpio_scl);
		return;
				}
	ret = gpio_request(pdata->gpio_sda, "mpu6500_sda");
	if (ret) {
		pr_err("[MPU6500]%s: unable to request mpu6500_sda [%d]\n",
				__func__, pdata->gpio_sda);
		return;
			}

	ret = gpio_request(pdata->gpio_int, "mpu6500_irq");
	if (ret) {
		pr_err("[MPU6500]%s: unable to request mpu6500_irq [%d]\n",
				__func__, pdata->gpio_int);
		return;
			}

			}



/* samsung factory test */

static int read_accel_raw_xyz(struct mpu6500_input_data *data)
{
	u8 regs[6];
	int result;

	result = mpu6500_i2c_read_reg(data->client, MPUREG_ACCEL_XOUT_H, 6, regs);
	if (result) {
		pr_err("[SENSOR] %s: i2c_read err= %d\n", __func__, result);
		return result;
	}

	data->acc_data.x = ((s16) ((s16) regs[0] << 8)) | regs[1];
	data->acc_data.y = ((s16) ((s16) regs[2] << 8)) | regs[3];
	data->acc_data.z = ((s16) ((s16) regs[4] << 8)) | regs[5];

	remap_sensor_data(data->acc_data.v, data->chip_pos);

	return 0;
}

static int accel_do_calibrate(struct mpu6500_input_data *data, int enable)
{
	struct file *cal_filp;
	int sum[3] = { 0, };
	int err;
	int i;
	mm_segment_t old_fs = {0};

	if (!(data->enabled_sensors & MPU6500_SENSOR_ACCEL)) {
		mpu6500_input_resume_accel(data);
		usleep_range(10000, 11000);
	}

	for (i = 0; i < ACC_CAL_TIME; i++) {
		err = read_accel_raw_xyz(data);
		if (err < 0) {
			pr_err("[SENSOR] %s: accel_read_accel_raw_xyz() "
				"failed in the %dth loop\n", __func__, i);
			goto done;
		}
		usleep_range(10000, 11000);
		sum[0] += data->acc_data.x/ACC_CAL_DIV;
		sum[1] += data->acc_data.y/ACC_CAL_DIV;
		sum[2] += data->acc_data.z/ACC_CAL_DIV;
	}

	if (!(data->enabled_sensors & MPU6500_SENSOR_ACCEL))
		mpu6500_input_suspend_accel(data);

	if (enable) {
		data->acc_cal[0] =
			(sum[0] / ACC_CAL_TIME) * ACC_CAL_DIV;
		data->acc_cal[1] =
			(sum[1] / ACC_CAL_TIME) * ACC_CAL_DIV;
		if(sum[2] > 0)
			data->acc_cal[2] =
			((sum[2] / ACC_CAL_TIME) - ACC_IDEAL) * ACC_CAL_DIV;
		else if(sum[2] < 0)
			data->acc_cal[2] =
			((sum[2] / ACC_CAL_TIME) + ACC_IDEAL) * ACC_CAL_DIV;

	} else {
		data->acc_cal[0] = 0;
		data->acc_cal[1] = 0;
		data->acc_cal[2] = 0;
	}

	pr_info("[SENSOR] %s: cal data (%d,%d,%d)\n", __func__,
		data->acc_cal[0], data->acc_cal[1], data->acc_cal[2]);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(MPU6500_ACCEL_CAL_PATH,
			O_CREAT | O_TRUNC | O_WRONLY,
			S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("[SENSOR] %s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&data->acc_cal, 3 * sizeof(s16),
			&cal_filp->f_pos);
	if (err != 3 * sizeof(s16)) {
		pr_err("[SENSOR] %s: Can't write the cal data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}




static ssize_t mpu6500_input_reactive_enable_show(struct device *dev,
					struct device_attribute
						*attr, char *buf)
{
	struct mpu6500_input_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		atomic_read(&data->reactive_state));
}

static ssize_t mpu6500_input_reactive_enable_store(struct device *dev,
					struct device_attribute
						*attr, const char *buf,
							size_t count)
{
	struct mpu6500_input_data *data = dev_get_drvdata(dev);
	bool onoff = false;
	unsigned long value = 0;
	struct motion_int_data *mot_data = &data->mot_data;

	if (strict_strtoul(buf, 10, &value))
		return -EINVAL;

	if (value == 1) {
		onoff = true;
	} else if (value == 0) {
		onoff = false;
	} else if (value == 2) {
		onoff = true;
		data->factory_mode = true;
	} else {
		pr_err("[SENSOR] %s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

#ifdef CONFIG_INPUT_MPU6500_POLLING
	if (!value) {
		disable_irq_wake(data->client->irq);
		disable_irq(data->client->irq);
	} else {
		enable_irq(data->client->irq);
		enable_irq_wake(data->client->irq);
	}
#endif

	mutex_lock(&data->mutex);
	if (onoff) {
		pr_info("[SENSOR] %s: enable\n", __func__);
		atomic_set(&data->reactive_enable, true);
		if (!mot_data->is_set) {
			mpu6500_i2c_read_reg(data->client,
				MPUREG_PWR_MGMT_1, 2,
					mot_data->pwr_mnt);
			mpu6500_i2c_read_reg(data->client,
				MPUREG_CONFIG, 1, &mot_data->cfg);
			mpu6500_i2c_read_reg(data->client,
				MPUREG_ACCEL_CONFIG, 1,
					&mot_data->accel_cfg);
			mpu6500_i2c_read_reg(data->client,
				MPUREG_GYRO_CONFIG, 1,
					&mot_data->gyro_cfg);
			mpu6500_i2c_read_reg(data->client,
				MPUREG_INT_ENABLE, 1,
					&mot_data->int_cfg);
			mpu6500_i2c_read_reg(data->client,
				MPUREG_SMPLRT_DIV, 1,
					&mot_data->smplrt_div);
		}
		mpu6500_input_set_motion_interrupt(data,
			true, data->factory_mode);
	} else {
		pr_info("[SENSOR] %s: disable\n", __func__);
		mpu6500_input_set_motion_interrupt(data,
			false, data->factory_mode);
		atomic_set(&data->reactive_enable, false);
		if (data->factory_mode)
			data->factory_mode = false;
	}	mutex_unlock(&data->mutex);

	pr_info("[SENSOR] %s: onoff = %d, state =%d OUT\n", __func__,
		atomic_read(&data->motion_recg_enable),
		atomic_read(&data->reactive_state));
	return count;
}

static struct device_attribute dev_attr_reactive_alert =
	__ATTR(reactive_alert, S_IRUGO | S_IWUSR | S_IWGRP,
		mpu6500_input_reactive_enable_show,
			mpu6500_input_reactive_enable_store);

static ssize_t mpu6500_power_on(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mpu6500_input_data *data = dev_get_drvdata(dev);
	int count = 0;

	dev_dbg(dev, "this_client = %d\n", (int)data->client);
	count = sprintf(buf, "%d\n", (data->client != NULL ? 1 : 0));

	return count;
}
static struct device_attribute dev_attr_power_on =
	__ATTR(power_on, S_IRUSR | S_IRGRP, mpu6500_power_on,
		NULL);

static ssize_t mpu6500_get_temp(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mpu6500_input_data *data = dev_get_drvdata(dev);
	int count = 0;
	short temperature = 0;
	unsigned char reg[2];
	int result;


	CHECK_RESULT(mpu6500_i2c_read_reg
		(data->client, MPUREG_TEMP_OUT_H, 2, reg));

	temperature = (short) (((reg[0]) << 8) | reg[1]);
	temperature = ((temperature / 334) + 21);

	pr_info("[SENSOR] %s: read temperature = %d\n", __func__, temperature);

	count = sprintf(buf, "%d\n", temperature);

	return count;
}
static struct device_attribute dev_attr_temperature =
	__ATTR(temperature, S_IRUSR | S_IRGRP, mpu6500_get_temp,
		NULL);

static ssize_t mpu6500_input_gyro_selftest_show(struct device *dev,
						 struct device_attribute *attr,
						 char *buf)
{
	struct mpu6500_input_data *data = dev_get_drvdata(dev);
	/* absolute gyro bias scaled by 1000 times. (gyro_bias x 1000) */
	int scaled_gyro_bias[3] = {0};
	/* absolute gyro rms scaled by 1000 times. (gyro_bias x 1000) */
	int scaled_gyro_rms[3] = {0};
	int packet_count[3] = {0};
	int ratio[3] = {0};
	int hw_result;
	int result;

	mutex_lock(&data->mutex);

	result = mpu6500_selftest_run(data->client,
				      packet_count,
				      scaled_gyro_bias,
				      scaled_gyro_rms, data->gyro_bias);

	hw_result = mpu6500_gyro_hw_self_check(data->client, ratio);

	pr_info("%s, result = %d, hw_result = %d\n", __func__,
			result, hw_result);

	if (!result) {
		/* store calibration to file */
		gyro_do_calibrate(data);
	} else {
		data->gyro_bias[0] = 0;
		data->gyro_bias[1] = 0;
		data->gyro_bias[2] = 0;
	}

	mutex_unlock(&data->mutex);

	return sprintf(buf, "%d,"
			"%d.%03d,%d.%03d,%d.%03d,"
			"%d.%03d,%d.%03d,%d.%03d,"
			"%d.%01d,%d.%01d,%d.%01d,"
			"%d.%03d,%d.%03d,%d.%03d\n",
			result | hw_result,
			(int)abs(scaled_gyro_bias[0] / 1000),
			(int)abs(scaled_gyro_bias[0]) % 1000,
			(int)abs(scaled_gyro_bias[1] / 1000),
			(int)abs(scaled_gyro_bias[1]) % 1000,
			(int)abs(scaled_gyro_bias[2] / 1000),
			(int)abs(scaled_gyro_bias[2]) % 1000,
			scaled_gyro_rms[0] / 1000,
			(int)abs(scaled_gyro_rms[0]) % 1000,
			scaled_gyro_rms[1] / 1000,
			(int)abs(scaled_gyro_rms[1]) % 1000,
			scaled_gyro_rms[2] / 1000,
			(int)abs(scaled_gyro_rms[2]) % 1000,
			(int)abs(ratio[0]/10),
			(int)abs(ratio[0])%10,
			(int)abs(ratio[1]/10),
			(int)abs(ratio[1])%10,
			(int)abs(ratio[2]/10),
			(int)abs(ratio[2])%10,
			(int)abs(packet_count[0] / 100),
			(int)abs(packet_count[0]) % 100,
			(int)abs(packet_count[1] / 100),
			(int)abs(packet_count[1]) % 100,
			(int)abs(packet_count[2] / 100),
			(int)abs(packet_count[2]) % 100);
}

static struct device_attribute dev_attr_selftest =
	__ATTR(selftest, S_IRUSR | S_IRGRP,
		mpu6500_input_gyro_selftest_show,
		NULL);

static ssize_t acc_data_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mpu6500_input_data *data = dev_get_drvdata(dev);

	if (!(data->enabled_sensors & MPU6500_SENSOR_ACCEL)) {
		mpu6500_input_resume_accel(data);
		usleep_range(10000, 11000);
		read_accel_raw_xyz(data);
		mpu6500_input_suspend_accel(data);
		usleep_range(10000, 11000);
	}

	return sprintf(buf, "%d, %d, %d\n", data->acc_data.x - data->acc_cal[0], 
		   data->acc_data.y - data->acc_cal[1], data->acc_data.z - data->acc_cal[2]);
}
static struct device_attribute dev_attr_raw_data =
	__ATTR(raw_data, S_IRUSR | S_IRGRP, acc_data_read,
		NULL);

static ssize_t accel_calibration_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct mpu6500_input_data *data = dev_get_drvdata(dev);
	int count = 0;
	s16 x, y, z;
	int err;

	x = data->acc_cal[0];
	y = data->acc_cal[1];
	z = data->acc_cal[2];
	pr_info("[SENSOR]: accel_calibration_show %d %d %d\n",
		x, y, z);

	if (!x && !y && !z)
		err = -1;
	else
		err = 1;

	count = sprintf(buf, "%d %d %d %d\n", err, x, y, z);

	return count;
}
static ssize_t accel_calibration_store(struct device *dev,
				struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct mpu6500_input_data *data = dev_get_drvdata(dev);
	int err;
	int count;
	unsigned long enable;
	s16 x;
	s16 y;
	s16 z;
	char tmp[64];

	if (strict_strtoul(buf, 10, &enable))
		return -EINVAL;
	err = accel_do_calibrate(data, enable);
	if (err < 0)
		pr_err("[SENSOR] %s: accel_do_calibrate() failed\n",
			__func__);
	x = data->acc_cal[0];
	y = data->acc_cal[1];
	z = data->acc_cal[2];

	pr_info("[SENSOR]: accel_calibration_store %d %d %d\n",
		x, y, z);
	if (err > 0)
		err = 0;
	count = sprintf(tmp, "%d\n", err);

	return count;
}
static struct device_attribute dev_attr_calibration =
	__ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
		accel_calibration_show, accel_calibration_store);

static ssize_t accel_vendor_show(struct device *dev,
				struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%s\n", "INVENSENSE");
}
static struct device_attribute dev_attr_accel_sensor_vendor =
	__ATTR(vendor, S_IRUSR | S_IRGRP, accel_vendor_show, NULL);
static struct device_attribute dev_attr_gyro_sensor_vendor =
	__ATTR(vendor, S_IRUSR | S_IRGRP, accel_vendor_show, NULL);

static ssize_t accel_name_show(struct device *dev,
				struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%s\n", "MPU6500");
}
static struct device_attribute dev_attr_accel_sensor_name =
	__ATTR(name, S_IRUSR | S_IRGRP, accel_name_show, NULL);
static struct device_attribute dev_attr_gyro_sensor_name =
	__ATTR(name, S_IRUSR | S_IRGRP, accel_name_show, NULL);

static struct device_attribute *gyro_sensor_attrs[] = {
	&dev_attr_power_on,
	&dev_attr_temperature,
	&dev_attr_gyro_sensor_vendor,
	&dev_attr_gyro_sensor_name,
	&dev_attr_selftest,
	NULL,
};

static struct device_attribute *accel_sensor_attrs[] = {
	&dev_attr_raw_data,
	&dev_attr_calibration,
	&dev_attr_reactive_alert,
	&dev_attr_accel_sensor_vendor,
	&dev_attr_accel_sensor_name,
	NULL,
};

static int mpu6500_misc_open(struct inode *inode, struct file *file)
{
	int ret = 0;
#if DEBUG
	pr_info("%s\n", __func__);
#endif

	return ret;
}

static int mpu6500_misc_release(struct inode *inode, struct file *file)
{
#if DEBUG
	pr_info("%s\n", __func__);
#endif

	return 0;
}

static const struct file_operations mpu6500_misc_fops = {
	.owner = THIS_MODULE,
	.open = mpu6500_misc_open,
	.release = mpu6500_misc_release,
};

static struct miscdevice mpu6500_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mpu6500",
	.fops = &mpu6500_misc_fops,
};


#ifdef CONFIG_INPUT_MPU6500_POLLING
static void mpu6500_work_func_acc(struct work_struct *work)
{
	struct mpu6500_input_data *data =
		container_of((struct delayed_work *)work,
			struct mpu6500_input_data, accel_work);
	mpu6500_input_report_accel_xyz(data);

	if (ktime_to_ms(data->accel_delay) < 60) {
		usleep_range(ktime_to_ms(data->accel_delay) * 1000,
			ktime_to_ms(data->accel_delay) * 1100);
		schedule_delayed_work(&data->accel_work, 0);
	} else {
		schedule_delayed_work(&data->accel_work,
			msecs_to_jiffies(
			ktime_to_ms(data->accel_delay)));
	}
}

static void mpu6500_work_func_gyro(struct work_struct *work)
{
	struct mpu6500_input_data *data =
		container_of((struct delayed_work *)work,
			struct mpu6500_input_data, gyro_work);

	mpu6500_input_report_gyro_xyz(data);

	if (ktime_to_ms(data->gyro_delay) < 60) {
		usleep_range(ktime_to_ms(data->gyro_delay) * 1000,
			ktime_to_ms(data->gyro_delay) * 1100);
		schedule_delayed_work(&data->gyro_work, 0);
	} else {
		schedule_delayed_work(&data->gyro_work,
			msecs_to_jiffies(
			ktime_to_ms(data->gyro_delay)));
	}
}
#endif

#ifdef CONFIG_OF

/* device tree parsing function */
static int mpu6500_parse_dt(struct device *dev,
			struct  mpu6500_platform_data *pdata)
{

	struct device_node *np = dev->of_node;

	/*scl,sda and irq */
	pdata->i2c_pull_up = of_property_read_bool(np, "invensense,i2c-pull-up");

	pdata->gpio_scl = of_get_named_gpio_flags(np,"invensense,scl-gpio",
	0, &pdata->scl_gpio_flags);

	pdata->gpio_sda = of_get_named_gpio_flags(np, "invensense,sda-gpio",
	0, &pdata->sda_gpio_flags);
	/*device tree node properties can be parsed this way*/
	pdata->gpio_int = of_get_named_gpio_flags(np, "invensense,irq-gpio",
	0, &pdata->irq_gpio_flags);

	/* accel and gyro calibration path */

	pdata->acc_cal_path = of_get_property(np, "invensense,acc_cal_path",
	NULL);
	pdata->gyro_cal_path = of_get_property(np, "invensense,gyro_cal_path",
	NULL);
	

	pr_err("mpu6500_parse_dt complete, SCL:%d SDA:%d IRQ:%d\n",pdata->gpio_scl, pdata->gpio_sda, pdata->gpio_int );

	return 0;
}
#else
static int mpu6500_parse_dt(struct device *dev,
struct  mpu6k_input_platform_data)
{
	return -ENODEV;
}
#endif
static int mpu6500_accel_input_init(struct mpu6500_input_data *data)
{
	struct input_dev *idev;
	int error;

	idev = input_allocate_device();
	if (!idev)
		return -ENOMEM;

	data->accel_input = idev;
	idev->name = MODULE_NAME_ACCEL;
	idev->id.bustype = BUS_I2C;

	input_set_capability(data->accel_input, EV_REL, REL_X);
	input_set_capability(data->accel_input, EV_REL, REL_Y);
	input_set_capability(data->accel_input, EV_REL, REL_Z);
	input_set_drvdata(data->accel_input, data);

	error = input_register_device(data->accel_input);
	if (error) {
		input_free_device(data->accel_input);
		return error;
	}

	error = sensors_create_symlink(&data->accel_input->dev.kobj, idev->name);
	if (error < 0) {
		input_unregister_device(data->accel_input);
		return error;
	}

	error = sysfs_create_group(&idev->dev.kobj,
				&accel_attribute_group);
	if (error) {
		pr_err("%s: could not create sysfs group\n", __func__);
		sensors_remove_symlink(&data->accel_input->dev.kobj,
			data->accel_input->name);
		input_unregister_device(data->accel_input);
		return error;
	}

	atomic_set(&data->accel_enable, 0);
	data->accel_delay = ns_to_ktime(MPU6500_DEFAULT_DELAY);
	data->gyro_delay = ns_to_ktime(MPU6500_DEFAULT_DELAY);
	atomic_set(&data->motion_recg_enable, 0);
	atomic_set(&data->reactive_state, 0);
	return 0;
}

static int mpu6500_gyro_input_init(struct mpu6500_input_data *data)
{
	struct input_dev *idev;
	int error;

	idev = input_allocate_device();
	if (!idev)
		return -ENOMEM;

	data->gyro_input = idev;
	idev->name = MODULE_NAME_GYRO;
	idev->id.bustype = BUS_I2C;

	input_set_capability(data->gyro_input, EV_REL, REL_RX);
	input_set_capability(data->gyro_input, EV_REL, REL_RY);
	input_set_capability(data->gyro_input, EV_REL, REL_RZ);
	input_set_drvdata(data->gyro_input, data);

	error = input_register_device(data->gyro_input);
	if (error) {
		input_free_device(data->gyro_input);
		return error;
	}

	error = sensors_create_symlink(&data->gyro_input->dev.kobj, idev->name);
	if (error < 0) {
		input_unregister_device(data->gyro_input);
		return error;
	}

	error = sysfs_create_group(&idev->dev.kobj,
				&gyro_attribute_group);
	if (error) {
		pr_err("%s: could not create sysfs group\n", __func__);
		sensors_remove_symlink(&data->gyro_input->dev.kobj,
			data->gyro_input->name);
		input_unregister_device(data->gyro_input);
		return error;
	}

	return 0;
}

static int __devinit mpu6500_input_probe(struct i2c_client *client,
					 const struct i2c_device_id *id)
{
	const struct mpu6500_input_cfg *cfg;
	struct mpu6500_input_data *data;
	struct mpu6500_platform_data *pdata;
	int error = 0;
	unsigned char whoami = 0;
	int ret = -ENODEV;
	int retry = 5;

	pr_info("[SENSOR] %s: IN\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[SENSOR] %s: - i2c_check_functionality error\n",
			__func__);
		goto exit;
	}
	if (client-> dev.of_node) {
		pdata = devm_kzalloc (&client->dev ,
			sizeof (struct mpu6500_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		error = mpu6500_parse_dt(&client->dev, pdata);
		if (error)
			return error;
	} 
	else
		{
		/* get platform data */
		pdata = client->dev.platform_data;
	
	if (!pdata)
		return -EINVAL;
	}
	mpu6500_request_gpio(pdata);

	data = kzalloc(sizeof(struct mpu6500_input_data), GFP_KERNEL);
	if (!data)
		goto err_kzalloc;

	wake_lock_init(&data->reactive_wake_lock, WAKE_LOCK_SUSPEND,
		"reactive_wake_lock");

	i2c_set_clientdata(client, data);
	data->client = client;
	data->pdata = pdata;
#if defined(CONFIG_SEC_BERLUTI_PROJECT)
	data->chip_pos = MPU6500_BOTTOM_LEFT_LOWER;
#elif defined(CONFIG_SEC_S3VE_PROJECT) || defined(CONFIG_SEC_VICTOR3GDSDTV_PROJECT)
	data->chip_pos = MPU6500_TOP_LEFT_UPPER;
#else
	data->chip_pos = MPU6500_BOTTOM_RIGHT_LOWER;
#endif
	gb_mpu_data = data;

	mpu6500_vdd_on(data, 1);

	do {
		retry--;
		/* Check if the device is there or not. */
		error = mpu6500_i2c_read_reg(client, MPUREG_WHOAMI, 1, &whoami);
		pr_info("%s : addr = 0x%x, whoami = 0x%x, error = 0x%x \n", __func__,
			client->addr,whoami, error);
		if (error < 0) {
			pr_err("%s: checking i2c error.(%d), retry %d\n",
				__func__, error, retry);
			msleep(20);
		} else {
			break;
		}
	} while (retry);
	if (error < 0) {
		pr_err("[SENSOR] %s: failed : threre is no such device.\n",
			__func__);
		goto err_checking_device;
	}

	if (!(whoami == MPU6500_ID || whoami == MPU6515_ID)) {
		pr_err("[SENSOR] %s: mpu6500 probe failed\n", __func__);
		error = -EIO;
		goto err_checking_device;
	}

	error = mpu6500_input_initialize(data, cfg);
	if (error)
		goto err_checking_device;

#ifdef CONFIG_INPUT_MPU6500_POLLING
	INIT_DELAYED_WORK(&data->accel_work, mpu6500_work_func_acc);
	INIT_DELAYED_WORK(&data->gyro_work, mpu6500_work_func_gyro);
#endif
	mutex_init(&data->mutex);

	error = mpu6500_accel_input_init(data);
	if (error)
		goto err_accel_input_init;

	error = mpu6500_gyro_input_init(data);
	if (error)
		goto err_gyro_input_init;

	error = misc_register(&mpu6500_misc_device);
	if (error)
		goto err_misc_register_failed;

	pm_runtime_enable(&client->dev);

	error = sensors_register(data->accel_sensor_device,
		data, accel_sensor_attrs,
			"accelerometer_sensor");
	if (error) {
		pr_err("[SENSOR] %s: cound not register\
			accelerometer sensor device(%d).\n",
			__func__, error);
		goto acc_sensor_register_failed;
	}

	error = sensors_register(data->gyro_sensor_device,
		data, gyro_sensor_attrs,
			"gyro_sensor");
	if (error) {
		pr_err("[SENSOR] %s: cound not register\
			gyro sensor device(%d).\n",
			__func__, error);
		goto gyro_sensor_register_failed;
	}

	ret = request_threaded_irq(client->irq, NULL, mpu6500_input_irq_thread,
		IRQF_TRIGGER_RISING | IRQF_ONESHOT, MPU6500_INPUT_DRIVER, data);
	if (ret < 0) {
		pr_err("[SENSOR] %s: - can't allocate irq.\n", __func__);
		goto exit_reactive_irq;
	}

	disable_irq(client->irq);

	pr_info("[SENSOR] %s: success\n", __func__);
	return 0;

exit_reactive_irq:
	sensors_unregister(data->gyro_sensor_device, accel_sensor_attrs);
gyro_sensor_register_failed:
	sensors_unregister(data->accel_sensor_device, accel_sensor_attrs);
acc_sensor_register_failed:
	misc_deregister(&mpu6500_misc_device);
err_misc_register_failed:
	sensors_remove_symlink(&data->gyro_input->dev.kobj,
			data->gyro_input->name);
	input_unregister_device(data->gyro_input);
err_gyro_input_init:
	sensors_remove_symlink(&data->accel_input->dev.kobj,
			data->accel_input->name);
	input_unregister_device(data->accel_input);
err_accel_input_init:
	mutex_destroy(&data->mutex);
err_checking_device:
	wake_lock_destroy(&data->reactive_wake_lock);
	kfree(data);
err_kzalloc:
exit:
	pr_err("[SENSOR] %s: - Probe fail!\n", __func__);
	return error;
}

static int __devexit mpu6500_input_remove(struct i2c_client *client)
{
	struct mpu6500_input_data *data = i2c_get_clientdata(client);
	if (data == NULL)
		return 0;

	pm_runtime_disable(&client->dev);

	if (client->irq > 0) {
		free_irq(client->irq, data);
		input_unregister_device(data->accel_input);
		input_unregister_device(data->gyro_input);
	}

	kfree(data);

	return 0;
}

static int mpu6500_input_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mpu6500_input_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_INPUT_MPU6500_POLLING
		if (atomic_read(&data->accel_enable))
			cancel_delayed_work_sync(&data->accel_work);
		if (atomic_read(&data->gyro_enable))
			cancel_delayed_work_sync(&data->gyro_work);
#endif
	if (!atomic_read(&data->reactive_enable)) {
#ifndef CONFIG_INPUT_MPU6500_POLLING
		disable_irq_wake(client->irq);
		disable_irq(client->irq);
#endif
		if (atomic_read(&data->accel_enable) ||
			atomic_read(&data->gyro_enable))
			mpu6500_input_set_mode(data, MPU6500_MODE_SLEEP);
	} else {
		disable_irq(client->irq);
	}
	return 0;
}

static int mpu6500_input_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mpu6500_input_data *data = i2c_get_clientdata(client);

	if (data == NULL)
		return 0;

	if (!atomic_read(&data->reactive_enable)) {
#ifndef CONFIG_INPUT_MPU6500_POLLING
		enable_irq(client->irq);
		enable_irq_wake(client->irq);
#endif
		if (atomic_read(&data->accel_enable) ||
			atomic_read(&data->gyro_enable))
			mpu6500_input_set_mode(data, MPU6500_MODE_NORMAL);
	} else {
		enable_irq(client->irq);
	}
#ifdef CONFIG_INPUT_MPU6500_POLLING
		if (atomic_read(&data->accel_enable))
			schedule_delayed_work(&data->accel_work, 0);
		if (atomic_read(&data->gyro_enable))
			schedule_delayed_work(&data->gyro_work, 0);
#endif
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id mpu6500_match_table[] = {
	{ .compatible = "invensense,mpu_6500",},
	{},
};
#else
#define mpu6500_match_table NULL
#endif
static const struct i2c_device_id mpu6500_input_id[] = {
	{"mpu6500_input", 0},
	{}
};

//MODULE_DEVICE_TABLE(i2c, mpu6500_input_id);

static const struct dev_pm_ops mpu6500_dev_pm_ops = {
	.suspend = mpu6500_input_suspend,
	.resume = mpu6500_input_resume,
};

static struct i2c_driver mpu6500_input_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = MPU6500_INPUT_DRIVER,
		   .of_match_table = mpu6500_match_table,
		   .pm = &mpu6500_dev_pm_ops
		   },
	.class = I2C_CLASS_HWMON,
	.id_table = mpu6500_input_id,
	.probe = mpu6500_input_probe,
	.remove = __devexit_p(mpu6500_input_remove),
};

static int __init mpu6500_init(void)
{
	int result = i2c_add_driver(&mpu6500_input_driver);

	printk(KERN_INFO "[SENSOR] mpu6500_init()\n");

	return result;
}

static void __exit mpu6500_exit(void)
{
	printk(KERN_INFO "[SENSOR] mpu6500_exit()\n");

	i2c_del_driver(&mpu6500_input_driver);
}

MODULE_AUTHOR("Tae-Soo Kim <tskim@invensense.com>");
MODULE_DESCRIPTION("MPU6500 driver");
MODULE_LICENSE("GPL");

module_init(mpu6500_init);
module_exit(mpu6500_exit);
