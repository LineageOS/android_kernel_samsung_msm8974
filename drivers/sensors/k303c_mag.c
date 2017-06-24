/******************** (C) COPYRIGHT 2013 STMicroelectronics ********************
*
* File Name          : k303c_mag.c
* Authors            : MSH - C&I BU - Application Team
*		     : Matteo Dameno (matteo.dameno@st.com)
*		     : Denis Ciocca (denis.ciocca@st.com)
*		     : Both authors are willing to be considered the contact
*		     : and update points for the driver.
* Version            : V.1.0.0
* Date               : 2013/Jun/17
* Description        : K303C magnetometer driver
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
********************************************************************************
*******************************************************************************/
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/wakelock.h>
#include <linux/sensor/sensors_core.h>

#define VENDOR_NAME		"STM"
#define MODEL_NAME		"K303C"
#define MODULE_NAME		"magnetic_sensor"

#define K303C_MAG_DEV_NAME	"k303c_mag"

#define I2C_AUTO_INCREMENT	(0x80)
#define MS_TO_NS(x)		(x*1000000L)

#define MAG_G_MAX_POS		983520	/** max positive value mag [ugauss] */
#define MAG_G_MAX_NEG		983040	/** max negative value mag [ugauss] */

#define FUZZ			0
#define FLAT			0

#define MAG_SELF_TEST_16G_MAX_LSB	(-1711)
#define MAG_SELF_TEST_16G_MIN_LSB	(-5133)
#define MAG_SELF_TEST_16G_MAX_LSB_Z	(-171)
#define MAG_SELF_TEST_16G_MIN_LSB_Z	(-1711)

#define K303C_MAG_MIN_POLL_PERIOD_MS	13

/* Address registers */
#define REG_WHOAMI_ADDR		(0x0F)	/** Who am i address register */
#define REG_CNTRL1_ADDR		(0x20)	/** CNTRL1 address register */
#define REG_CNTRL2_ADDR		(0x21)	/** CNTRL2 address register */
#define REG_CNTRL3_ADDR		(0x22)	/** CNTRL3 address register */
#define REG_CNTRL4_ADDR		(0x23)	/** CNTRL4 address register */
#define REG_CNTRL5_ADDR		(0x24)	/** CNTRL5 address register */

#define REG_MAG_DATA_ADDR	(0x28)	/** Mag. data low address register */

/* Sensitivity */
#define SENSITIVITY_MAG_4G	146156	/**	ngauss/LSB	*/
#define SENSITIVITY_MAG_8G	292312	/**	ngauss/LSB	*/
#define SENSITIVITY_MAG_10G	365364	/**	ngauss/LSB	*/
#define SENSITIVITY_MAG_16G	584454	/**	ngauss/LSB	*/

/* ODR */
#define ODR_MAG_MASK		(0X1C)	/* Mask for odr change on mag */
#define K303C_MAG_ODR0_625	(0x00)	/* 0.625Hz output data rate */
#define K303C_MAG_ODR1_25	(0x04)	/* 1.25Hz output data rate */
#define K303C_MAG_ODR2_5	(0x08)	/* 2.5Hz output data rate */
#define K303C_MAG_ODR5		(0x0C)	/* 5Hz output data rate */
#define K303C_MAG_ODR10		(0x10)	/* 10Hz output data rate */
#define K303C_MAG_ODR20		(0x14)	/* 20Hz output data rate */
#define K303C_MAG_ODR40		(0x18)	/* 40Hz output data rate */
#define K303C_MAG_ODR80		(0x1C)	/* 80Hz output data rate */

/* Magnetometer Sensor Full Scale */
#define K303C_MAG_FS_MASK	(0x60)
#define K303C_MAG_FS_4G		(0x00)	/* Full scale 4 G */
#define K303C_MAG_FS_8G		(0x20)	/* Full scale 8 G */
#define K303C_MAG_FS_10G	(0x40)	/* Full scale 10 G */
#define K303C_MAG_FS_16G	(0x60)	/* Full scale 16 G */

/* Magnetic sensor mode */
#define MSMS_MASK		(0x03)	/* Mask magnetic sensor mode */
#define POWEROFF_MAG		(0x02)	/* Power Down */
#define CONTINUOS_CONVERSION	(0x00)	/* Continuos Conversion */

/* X and Y axis operative mode selection */
#define X_Y_PERFORMANCE_MASK		(0x60)
#define X_Y_LOW_PERFORMANCE		(0x00)
#define X_Y_MEDIUM_PERFORMANCE		(0x20)
#define X_Y_HIGH_PERFORMANCE		(0x40)
#define X_Y_ULTRA_HIGH_PERFORMANCE	(0x60)

/* Z axis operative mode selection */
#define Z_PERFORMANCE_MASK		(0x0c)
#define Z_LOW_PERFORMANCE		(0x00)
#define Z_MEDIUM_PERFORMANCE		(0x04)
#define Z_HIGH_PERFORMANCE		(0x08)
#define Z_ULTRA_HIGH_PERFORMANCE	(0x0c)

/* Default values loaded in probe function */
#define WHOIAM_VALUE		(0x3d)	/** Who Am I default value */
#define REG_DEF_CNTRL1		(0xE0)	/** CNTRL1 default value */
#define REG_DEF_CNTRL2		(0x60)	/** CNTRL2 default value */
#define REG_DEF_CNTRL3		(0x03)	/** CNTRL3 default value */
#define REG_DEF_CNTRL4		(0x0C)	/** CNTRL4 default value */
#define REG_DEF_CNTRL5		(0x40)	/** CNTRL5 default value */

#define REG_DEF_ALL_ZEROS	(0x00)

#define MAX_VDD	3000000
#define AVG_VDD	2850000
#define AVG_VIO	1800000


struct {
	unsigned int cutoff_us;
	u8 value;
} k303c_mag_odr_table[] = {
		{ 12, K303C_MAG_ODR80},
		{ 25, K303C_MAG_ODR40},
		{ 50, K303C_MAG_ODR20},
		{ 100, K303C_MAG_ODR10},
		{ 200, K303C_MAG_ODR5},
		{ 400, K303C_MAG_ODR2_5},
		{ 800, K303C_MAG_ODR1_25},
		{ 1600, K303C_MAG_ODR0_625},
};

struct interrupt_enable {
	atomic_t enable;
	u8 address;
	u8 mask;
};

struct interrupt_value {
	int value;
	u8 address;
};

struct k303c_mag_platform_data {

	unsigned int poll_interval;
	unsigned int min_interval;

	u8 fs_range;

	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;
};

struct k303c_mag_p {
	struct i2c_client *client;

	struct mutex lock;
	struct work_struct input_work_mag;
	struct workqueue_struct *mag_wq;
	struct hrtimer hr_timer_mag;
	ktime_t ktime_mag;
	struct device *factory_device;
	struct input_dev *input;

	int hw_initialized;
	/* hw_working=-1 means not tested yet */
	int hw_working;

	atomic_t enabled_mag;

	int on_before_suspend;
	int use_smbus;

	u32 sensitivity_mag;

	u8 xy_mode;
	u8 z_mode;

	struct regulator *reg_vdd;
	struct regulator *reg_vio;

	struct k303c_mag_platform_data *pdata;
	int (*power_onoff)(struct k303c_mag_p *data, bool onoff);
};

static const struct k303c_mag_platform_data default_k303c_mag_pdata = {
	.poll_interval = 100,
	.min_interval = K303C_MAG_MIN_POLL_PERIOD_MS,
	.fs_range = K303C_MAG_FS_16G,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,
};

struct reg_rw {
	u8 address;
	u8 default_value;
	u8 resume_value;
};

struct reg_r {
	u8 address;
	u8 value;
};

static struct dataus_registers {
	struct reg_r who_am_i;
	struct reg_rw cntrl1;
	struct reg_rw cntrl2;
	struct reg_rw cntrl3;
	struct reg_rw cntrl4;
	struct reg_rw cntrl5;
} dataus_registers = {
	.who_am_i.address = REG_WHOAMI_ADDR,
	.who_am_i.value = WHOIAM_VALUE,
	.cntrl1.address = REG_CNTRL1_ADDR,
	.cntrl1.default_value = REG_DEF_CNTRL1,
	.cntrl2.address = REG_CNTRL2_ADDR,
	.cntrl2.default_value = REG_DEF_CNTRL2,
	.cntrl3.address = REG_CNTRL3_ADDR,
	.cntrl3.default_value = REG_DEF_CNTRL3,
	.cntrl4.address = REG_CNTRL4_ADDR,
	.cntrl4.default_value = REG_DEF_CNTRL4,
	.cntrl5.address = REG_CNTRL5_ADDR,
	.cntrl5.default_value = REG_DEF_CNTRL5,
};

static int k303c_mag_i2c_read(struct k303c_mag_p *data, u8 *buf, int len)
{
	int ret;
	u8 reg = buf[0];
	u8 cmd = reg;


	if (len > 1)
		cmd = (I2C_AUTO_INCREMENT | reg);
	if (data->use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_read_byte_data(data->client, cmd);
			buf[0] = ret & 0xff;
#ifdef DEBUG
			dev_warn(&data->client->dev,
				"i2c_smbus_read_byte_data: ret=0x%02x, len:%d ,"
				"command=0x%02x, buf[0]=0x%02x\n",
				ret, len, cmd , buf[0]);
#endif
		} else if (len > 1) {
			ret = i2c_smbus_read_i2c_block_data(data->client,
								cmd, len, buf);
		} else
			ret = -1;

		if (ret < 0) {
			dev_err(&data->client->dev,
				"read transfer error: len:%d, command=0x%02x\n",
				len, cmd);
			return 0;
		}
		return len;
	}

	ret = i2c_master_send(data->client, &cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
		return ret;

	return i2c_master_recv(data->client, buf, len);
}

static int k303c_mag_i2c_write(struct k303c_mag_p *data, u8 *buf, int len)
{
	int ret;
	u8 reg, value;

	if (len > 1)
		buf[0] = (I2C_AUTO_INCREMENT | buf[0]);

	reg = buf[0];
	value = buf[1];

	if (data->use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_write_byte_data(data->client,
								reg, value);
#ifdef DEBUG
			dev_warn(&data->client->dev,
				"i2c_smbus_write_byte_data: ret=%d, len:%d, "
				"command=0x%02x, value=0x%02x\n",
				ret, len, reg , value);
#endif
			return ret;
		} else if (len > 1) {
			ret = i2c_smbus_write_i2c_block_data(data->client,
							reg, len, buf + 1);
			return ret;
		}
	}

	ret = i2c_master_send(data->client, buf, len+1);
	return (ret == len+1) ? 0 : ret;
}

static int k303c_mag_hw_init(struct k303c_mag_p *data)
{
	int err;
	u8 buf[6];

	buf[0] = dataus_registers.who_am_i.address;
	err = k303c_mag_i2c_read(data, buf, 1);

	if (err < 0) {
		dev_warn(&data->client->dev,
		"Error reading WHO_AM_I: is device available/working?\n");
		goto err_firstread;
	} else
		data->hw_working = 1;

	if (buf[0] != dataus_registers.who_am_i.value) {
		dev_err(&data->client->dev,
		"device unknown. Expected: 0x%02x, Replies: 0x%02x\n",
				dataus_registers.who_am_i.value, buf[0]);
		err = -1;
		goto err_unknown_device;
	}

	dataus_registers.cntrl1.resume_value =
					dataus_registers.cntrl1.default_value;
	dataus_registers.cntrl2.resume_value =
					dataus_registers.cntrl2.default_value;
	dataus_registers.cntrl3.resume_value =
					dataus_registers.cntrl3.default_value;
	dataus_registers.cntrl4.resume_value =
					dataus_registers.cntrl4.default_value;
	dataus_registers.cntrl5.resume_value =
					dataus_registers.cntrl5.default_value;

	buf[0] = dataus_registers.cntrl1.address;
	buf[1] = dataus_registers.cntrl1.default_value;
	buf[2] = dataus_registers.cntrl2.default_value;
	buf[3] = dataus_registers.cntrl3.default_value;
	buf[4] = dataus_registers.cntrl4.default_value;
	buf[5] = dataus_registers.cntrl5.default_value;

	err = k303c_mag_i2c_write(data, buf, 5);
	if (err < 0) {
		dev_warn(&data->client->dev,
		"Error initializing CLTR_REG registers\n");
		goto err_reginit;
	}

	data->xy_mode = X_Y_ULTRA_HIGH_PERFORMANCE;
	data->z_mode = Z_ULTRA_HIGH_PERFORMANCE;
	data->hw_initialized = 1;

	return 0;

err_reginit:
err_unknown_device:
err_firstread:
	data->hw_working = 0;
	data->hw_initialized = 0;
	return err;
}

static int sensor_regulator_onoff(struct k303c_mag_p *data, bool onoff)
{
	int ret = -1;

	if (IS_ERR_OR_NULL(data->reg_vdd) || IS_ERR_OR_NULL(data->reg_vio)) {
		pr_err("[SENSOR]: %s: Failed to enable regulator.\n", __func__);
		return -ENODEV;
	}

	if (onoff) {
		ret = regulator_enable(data->reg_vdd);
		if (ret) {
			pr_err("[SENSOR]: %s: Failed to enable regulator vdd.\n",
				__func__);
			return ret;
		}
		ret = regulator_enable(data->reg_vio);
		if (ret) {
			pr_err("[SENSOR]: %s: Failed to enable regulator vio.\n",
				__func__);
			return ret;
		}
		msleep(30);
	} else {
		ret = regulator_disable(data->reg_vdd);
		if (ret) {
			pr_err("[SENSOR]: %s: Failed to disable regulatorvdd.\n",
				__func__);
			return ret;
		}
		ret = regulator_disable(data->reg_vio);
		if (ret) {
			pr_err("[SENSOR]: %s: Failed to disable regulator vio.\n",
				__func__);
			return ret;
		}
	}

	return 0;
}

static int k303c_mag_device_power_off(struct k303c_mag_p *data)
{
	int err;
	u8 buf[2];

	buf[0] = dataus_registers.cntrl3.address;
	buf[1] = ((MSMS_MASK & POWEROFF_MAG) |
		((~MSMS_MASK) & dataus_registers.cntrl3.resume_value));

	err = k303c_mag_i2c_write(data, buf, 1);
	if (err < 0)
		dev_err(&data->client->dev,
			"magnetometer soft power off failed: %d\n", err);

	err = sensor_regulator_onoff(data, false);
	if (err < 0)
		return err;

	atomic_set(&data->enabled_mag, 0);

	return 0;
}

static int k303c_mag_device_power_on(struct k303c_mag_p *data)
{
	int err;
	u8 buf[6];

	err = sensor_regulator_onoff(data, true);
	if (err < 0)
		return err;

	buf[0] = dataus_registers.cntrl1.address;
	buf[1] = dataus_registers.cntrl1.resume_value;
	err = k303c_mag_i2c_write(data, buf, 1);
	if (err < 0)
		goto err_resume_datae;

	buf[0] = dataus_registers.cntrl3.address;
	buf[1] = ((MSMS_MASK & CONTINUOS_CONVERSION) |
		((~MSMS_MASK) & dataus_registers.cntrl3.resume_value));


	err = k303c_mag_i2c_write(data, buf, 1);
	if (err < 0)
		goto err_resume_datae;

	atomic_set(&data->enabled_mag, 1);

	return 0;

err_resume_datae:
	atomic_set(&data->enabled_mag, 0);
	dev_err(&data->client->dev,
		"magnetometer hw power on error 0x%02x,0x%02x: %d\n",
							buf[0], buf[1], err);
	return err;
}

static int k303c_mag_update_fs_range(struct k303c_mag_p *data,
	u8 new_fs_range)
{
	int err = -1;
	u32 sensitivity;
	u8 updated_val;
	u8 buf[2];

	switch (new_fs_range) {
	case K303C_MAG_FS_4G:
		sensitivity = SENSITIVITY_MAG_4G;
		break;
	case K303C_MAG_FS_8G:
		sensitivity = SENSITIVITY_MAG_8G;
		break;
	case K303C_MAG_FS_10G:
		sensitivity = SENSITIVITY_MAG_10G;
		break;
	case K303C_MAG_FS_16G:
		sensitivity = SENSITIVITY_MAG_16G;
		break;
	default:
		dev_err(&data->client->dev,
			"invalid magnetometer fs range requested: %u\n",
								new_fs_range);
		return -EINVAL;
	}

	buf[0] = dataus_registers.cntrl2.address;
	err = k303c_mag_i2c_read(data, buf, 1);
	if (err < 0)
		goto error;

	dataus_registers.cntrl2.resume_value = buf[0];
	updated_val = (K303C_MAG_FS_MASK & new_fs_range);
	buf[1] = updated_val;
	buf[0] = dataus_registers.cntrl2.address;

	err = k303c_mag_i2c_write(data, buf, 1);
	if (err < 0)
		goto error;
	dataus_registers.cntrl2.resume_value = updated_val;
	data->sensitivity_mag = sensitivity;

	return err;

error:
	dev_err(&data->client->dev,
		"update magnetometer fs range failed 0x%02x,0x%02x: %d\n",
							buf[0], buf[1], err);
	return err;
}

static int k303c_mag_update_odr(struct k303c_mag_p *data,
	unsigned int poll_interval_ms)
{
	int err = -1;
	u8 config[2];
	int i;

	for (i = ARRAY_SIZE(k303c_mag_odr_table) - 1; i >= 0; i--) {
		if ((k303c_mag_odr_table[i].cutoff_us <= poll_interval_ms)
								|| (i == 0))
			break;
	}

	config[1] = ((ODR_MAG_MASK & k303c_mag_odr_table[i].value) |
		((~ODR_MAG_MASK) & dataus_registers.cntrl1.resume_value));

	if (atomic_read(&data->enabled_mag)) {
		config[0] = dataus_registers.cntrl1.address;
		err = k303c_mag_i2c_write(data, config, 1);
		if (err < 0)
			goto error;
		dataus_registers.cntrl1.resume_value = config[1];
		data->ktime_mag = ktime_set(0, MS_TO_NS(poll_interval_ms));
	}

	return err;

error:
	dev_err(&data->client->dev,
		"update magnetometer odr failed 0x%02x,0x%02x: %d\n",
						config[0], config[1], err);

	return err;
}

static int k303c_mag_update_operative_mode(struct k303c_mag_p *data,
	int axis, u8 value)
{
	int err = -1;
	u8 config[2];
	u8 mask;
	u8 addr;

	if (axis == 0) {
		config[0] = REG_CNTRL1_ADDR;
		mask = X_Y_PERFORMANCE_MASK;
		addr = REG_CNTRL1_ADDR;
	} else {
		config[0] = REG_CNTRL4_ADDR;
		mask = Z_PERFORMANCE_MASK;
		addr = REG_CNTRL4_ADDR;
	}
	err = k303c_mag_i2c_read(data, config, 1);
	if (err < 0)
		goto error;
	config[1] = ((mask & value) |
		((~mask) & config[0]));

	config[0] = addr;
	err = k303c_mag_i2c_write(data, config, 1);
	if (err < 0)
		goto error;
	if (axis == 0)
		data->xy_mode = value;
	else
		data->z_mode = value;

	return err;

error:
	dev_err(&data->client->dev, "update operative mode failed 0x%02x,0x%02x: %d\n",
		config[0], config[1], err);

	return err;
}

static int k303c_mag_validate_polling(unsigned int *min_interval,
	unsigned int *poll_interval, unsigned int min, u8 *axis_map_x,
	u8 *axis_map_y, u8 *axis_map_z, struct i2c_client *client)
{
	*min_interval = max(min, *min_interval);
	*poll_interval = max(*poll_interval, *min_interval);

	if (*axis_map_x > 2 || *axis_map_y > 2 || *axis_map_z > 2) {
		dev_err(&client->dev,
			"invalid axis_map value x:%u y:%u z%u\n",
					*axis_map_x, *axis_map_y, *axis_map_z);
		return -EINVAL;
	}

	return 0;
}

static int k303c_mag_validate_negate(u8 *negate_x, u8 *negate_y, u8 *negate_z,
	struct i2c_client *client)
{
	if (*negate_x > 1 || *negate_y > 1 || *negate_z > 1) {
		dev_err(&client->dev,
			"invalid negate value x:%u y:%u z:%u\n",
					*negate_x, *negate_y, *negate_z);
		return -EINVAL;
	}
	return 0;
}

static int k303c_mag_validate_pdata(struct k303c_mag_p *data)
{
	int res = -1;

	res = k303c_mag_validate_polling(&data->pdata->min_interval,
				&data->pdata->poll_interval,
				(unsigned int)K303C_MAG_MIN_POLL_PERIOD_MS,
				&data->pdata->axis_map_x,
				&data->pdata->axis_map_y,
				&data->pdata->axis_map_z,
				data->client);
	if (res < 0)
		return -EINVAL;

	res = k303c_mag_validate_negate(&data->pdata->negate_x,
				&data->pdata->negate_y,
				&data->pdata->negate_z,
				data->client);
	if (res < 0)
		return -EINVAL;

	return 0;
}

static int k303c_mag_enable(struct k303c_mag_p *data)
{
	int err;

	if (!atomic_cmpxchg(&data->enabled_mag, 0, 1)) {
		err = k303c_mag_device_power_on(data);
		if (err < 0) {
			atomic_set(&data->enabled_mag, 0);
			return err;
		}
		hrtimer_start(&data->hr_timer_mag,
					data->ktime_mag, HRTIMER_MODE_REL);
	}

	return 0;
}

static int k303c_mag_disable(struct k303c_mag_p *data)
{
	if (atomic_cmpxchg(&data->enabled_mag, 1, 0)) {
		cancel_work_sync(&data->input_work_mag);
		hrtimer_cancel(&data->hr_timer_mag);
		k303c_mag_device_power_off(data);
	}

	return 0;
}

static void k303c_mag_input_cleanup(struct k303c_mag_p *data)
{
	input_unregister_device(data->input);
	input_free_device(data->input);
}

static ssize_t k303c_mag_delay_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	unsigned int val;
	struct k303c_mag_p *data = dev_get_drvdata(dev);

	mutex_lock(&data->lock);
	val = data->pdata->poll_interval;
	mutex_unlock(&data->lock);
	return snprintf(buf, 0xff, "%u\n", val);
}

static ssize_t k303c_mag_delay_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct k303c_mag_p *data = dev_get_drvdata(dev);
	unsigned long delay;
	unsigned int interval_ms;

	if (kstrtoul(buf, 10, &delay)) {
		pr_err("[SENSOR]: %s - set delay error\n", __func__);
		return -EINVAL;
	}

	interval_ms = (unsigned int)(delay / 1000000L);
	interval_ms = max_t(unsigned int, (unsigned int)interval_ms,
						data->pdata->min_interval);
	mutex_lock(&data->lock);
	data->pdata->poll_interval = (unsigned int)interval_ms;
	k303c_mag_update_odr(data, interval_ms);
	mutex_unlock(&data->lock);
	return size;
}

static ssize_t k303c_mag_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct k303c_mag_p *data = dev_get_drvdata(dev);
	int val = (int)atomic_read(&data->enabled_mag);
	return snprintf(buf, 0xff, "%d\n", val);
}

static ssize_t k303c_mag_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct k303c_mag_p *data = dev_get_drvdata(dev);
	unsigned long val;

	if (kstrtoul(buf, 10, &val))
		return -EINVAL;

	if (val)
		k303c_mag_enable(data);
	else
		k303c_mag_disable(data);

	return size;
}

static ssize_t attr_get_range_mag(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u8 val;
	int range = 2;
	struct k303c_mag_p *data = dev_get_drvdata(dev);

	mutex_lock(&data->lock);
	val = data->pdata->fs_range;
	switch (val) {
	case K303C_MAG_FS_4G:
		range = 4;
		break;
	case K303C_MAG_FS_8G:
		range = 8;
		break;
	case K303C_MAG_FS_10G:
		range = 10;
		break;
	case K303C_MAG_FS_16G:
		range = 16;
		break;
	}
	mutex_unlock(&data->lock);

	return snprintf(buf, 0xff, "%d\n", range);
}

static ssize_t attr_set_range_mag(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct k303c_mag_p *data = dev_get_drvdata(dev);
	unsigned long val;
	u8 range;
	int err;

	if (kstrtoul(buf, 10, &val))
		return -EINVAL;
	switch (val) {
	case 4:
		range = K303C_MAG_FS_4G;
		break;
	case 8:
		range = K303C_MAG_FS_8G;
		break;
	case 10:
		range = K303C_MAG_FS_10G;
		break;
	case 16:
		range = K303C_MAG_FS_16G;
		break;
	default:
		dev_err(&data->client->dev,
			"magnetometer invalid range request: %lu, discarded\n",
									val);
		return -EINVAL;
	}
	mutex_lock(&data->lock);
	err = k303c_mag_update_fs_range(data, range);
	if (err < 0) {
		mutex_unlock(&data->lock);
		return err;
	}
	data->pdata->fs_range = range;
	mutex_unlock(&data->lock);
	dev_info(&data->client->dev,
				"magnetometer range set to: %lu g\n", val);

	return size;
}

static ssize_t attr_get_xy_mode(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u8 val;
	char mode[13];
	struct k303c_mag_p *data = dev_get_drvdata(dev);

	mutex_lock(&data->lock);
	val = data->xy_mode;
	switch (val) {
	case X_Y_HIGH_PERFORMANCE:
		strlcpy(mode, "high", sizeof(mode));
		break;
	case X_Y_LOW_PERFORMANCE:
		strlcpy(mode, "low", sizeof(mode));
		break;
	case X_Y_MEDIUM_PERFORMANCE:
		strlcpy(mode, "medium", sizeof(mode));
		break;
	case X_Y_ULTRA_HIGH_PERFORMANCE:
		strlcpy(mode, "ultra_high", sizeof(mode));
		break;
	}
	mutex_unlock(&data->lock);
	return snprintf(buf, 0xff, "%s\n", mode);
}

static ssize_t attr_set_xy_mode(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct k303c_mag_p *data = dev_get_drvdata(dev);
	u8 mode;
	int err;

	err = strncmp(buf, "high", 4);
	if (err == 0) {
		mode = X_Y_HIGH_PERFORMANCE;
		goto valid;
	}
	err = strncmp(buf, "low", 3);
	if (err == 0) {
		mode = X_Y_LOW_PERFORMANCE;
		goto valid;
	}
	err = strncmp(buf, "medium", 6);
	if (err == 0) {
		mode = X_Y_MEDIUM_PERFORMANCE;
		goto valid;
	}
	err = strncmp(buf, "ultra_high", 10);
	if (err == 0) {
		mode = X_Y_ULTRA_HIGH_PERFORMANCE;
		goto valid;
	}
	goto error;

valid:
	err = k303c_mag_update_operative_mode(data, 0, mode);
	if (err < 0)
		goto error;

	dev_info(&data->client->dev,
				"magnetometer x_y op. mode set to: %s", buf);
	return size;

error:
	dev_err(&data->client->dev,
		"magnetometer invalid value request: %s, discarded\n", buf);

	return -EINVAL;
}

static ssize_t attr_get_z_mode(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u8 val;
	char mode[13];
	struct k303c_mag_p *data = dev_get_drvdata(dev);

	mutex_lock(&data->lock);
	val = data->z_mode;
	switch (val) {
	case Z_HIGH_PERFORMANCE:
		strlcpy(mode, "high", sizeof(mode));
		break;
	case Z_LOW_PERFORMANCE:
		strlcpy(mode, "low", sizeof(mode));
		break;
	case Z_MEDIUM_PERFORMANCE:
		strlcpy(mode, "medium", sizeof(mode));
		break;
	case Z_ULTRA_HIGH_PERFORMANCE:
		strlcpy(mode, "ultra_high", sizeof(mode));
		break;
	}
	mutex_unlock(&data->lock);
	return snprintf(buf, 0xff, "%s\n", mode);
}

static ssize_t attr_set_z_mode(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct k303c_mag_p *data = dev_get_drvdata(dev);
	u8 mode;
	int err;

	err = strncmp(buf, "high", 4);
	if (err == 0) {
		mode = Z_HIGH_PERFORMANCE;
		goto valid;
	}
	err = strncmp(buf, "low", 3);
	if (err == 0) {
		mode = Z_LOW_PERFORMANCE;
		goto valid;
	}
	err = strncmp(buf, "medium", 6);
	if (err == 0) {
		mode = Z_MEDIUM_PERFORMANCE;
		goto valid;
	}
	err = strncmp(buf, "ultra_high", 10);
	if (err == 0) {
		mode = Z_ULTRA_HIGH_PERFORMANCE;
		goto valid;
	}
	goto error;

valid:
	err = k303c_mag_update_operative_mode(data, 1, mode);
	if (err < 0)
		goto error;

	dev_info(&data->client->dev,
			"magnetometer z op. mode set to: %s", buf);
	return size;

error:
	dev_err(&data->client->dev,
		"magnetometer invalid value request: %s, discarded\n", buf);

	return -EINVAL;
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		k303c_mag_delay_show, k303c_mag_delay_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		k303c_mag_enable_show, k303c_mag_enable_store);

static struct attribute *k303c_mag_attributes[] = {
	&dev_attr_poll_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group k303c_mag_attribute_group = {
	.attrs = k303c_mag_attributes
};

static struct device_attribute attributes[] = {
	__ATTR(full_scale, 0666, attr_get_range_mag, attr_set_range_mag),
	__ATTR(x_y_opearative_mode, 0666, attr_get_xy_mode, attr_set_xy_mode),
	__ATTR(z_opearative_mode, 0666, attr_get_z_mode, attr_set_z_mode),
};

static void remove_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, attributes + i);
}

int k303c_mag_input_open(struct input_dev *input)
{
	struct k303c_mag_p *data = input_get_drvdata(input);
	dev_dbg(&data->client->dev, "%s\n", __func__);
	return 0;
}

void k303c_mag_input_close(struct input_dev *dev)
{
	struct k303c_mag_p *data = input_get_drvdata(dev);
	dev_dbg(&data->client->dev, "%s\n", __func__);
	k303c_mag_disable(data);
}

static int k303c_mag_get_data(struct k303c_mag_p *data, int *xyz)
{
	int err = -1;
	u8 mag_data[6];
	s32 hw_d[3] = { 0 };

	mag_data[0] = (REG_MAG_DATA_ADDR);
	err = k303c_mag_i2c_read(data, mag_data, 6);
	if (err < 0)
		return err;

	hw_d[0] = (s16)((mag_data[1] << 8) | mag_data[0]);
	hw_d[1] = (s16)((mag_data[3] << 8) | mag_data[2]);
	hw_d[2] = (s16)((mag_data[5] << 8) | mag_data[4]);

#ifdef DEBUG
	pr_debug("%s read x=0x%02x 0x%02x (regH regL), x=%d (dec) [LSB]\n",
		K303C_MAG_DEV_NAME, mag_data[1], mag_data[0], hw_d[0]);
	pr_debug("%s read y=0x%02x 0x%02x (regH regL), y=%d (dec) [LSB]\n",
		K303C_MAG_DEV_NAME, mag_data[3], mag_data[2], hw_d[1]);
	pr_debug("%s read z=0x%02x 0x%02x (regH regL), z=%d (dec) [LSB]\n",
		K303C_MAG_DEV_NAME, mag_data[5], mag_data[4], hw_d[2]);
#endif

	xyz[0] = ((data->pdata->negate_x) ? (-hw_d[data->pdata->axis_map_x])
		: (hw_d[data->pdata->axis_map_x]));
	xyz[1] = ((data->pdata->negate_y) ? (-hw_d[data->pdata->axis_map_y])
		: (hw_d[data->pdata->axis_map_y]));
	xyz[2] = ((data->pdata->negate_z) ? (-hw_d[data->pdata->axis_map_z])
		: (hw_d[data->pdata->axis_map_z]));

	return err;
}

static void k303c_mag_report_values(struct k303c_mag_p *data, int *xyz)
{
	input_report_abs(data->input, ABS_X, xyz[0]);
	input_report_abs(data->input, ABS_Y, xyz[1]);
	input_report_abs(data->input, ABS_Z, xyz[2]);
	input_sync(data->input);
}

static int k303c_mag_input_init(struct k303c_mag_p *data)
{
	int err;

	data->input = input_allocate_device();
	if (!data->input) {
		err = -ENOMEM;
		dev_err(&data->client->dev,
			"magnetometer input device allocation failed\n");
		goto err0;
	}

	data->input->open = k303c_mag_input_open;
	data->input->close = k303c_mag_input_close;
	data->input->name = MODULE_NAME;
	data->input->id.bustype = BUS_I2C;
	data->input->dev.parent = &data->client->dev;

	input_set_drvdata(data->input, data);

	set_bit(EV_ABS, data->input->evbit);

	input_set_abs_params(data->input, ABS_X,
				-MAG_G_MAX_NEG, MAG_G_MAX_POS, FUZZ, FLAT);
	input_set_abs_params(data->input, ABS_Y,
				-MAG_G_MAX_NEG, MAG_G_MAX_POS, FUZZ, FLAT);
	input_set_abs_params(data->input, ABS_Z,
				-MAG_G_MAX_NEG, MAG_G_MAX_POS, FUZZ, FLAT);

	err = input_register_device(data->input);
	if (err) {
		dev_err(&data->client->dev,
			"unable to register magnetometer input device %s\n",
				data->input->name);
		goto err1;
	}

	err = sensors_create_symlink(&data->input->dev.kobj,
					data->input->name);
	if (err < 0) {
		input_unregister_device(data->input);
		return err;
	}

	/* sysfs node creation */
	err = sysfs_create_group(&data->input->dev.kobj,
					&k303c_mag_attribute_group);
	if (err < 0) {
		sensors_remove_symlink(&data->input->dev.kobj,
			data->input->name);
		input_unregister_device(data->input);
		return err;
	}
	return 0;

err1:
	input_free_device(data->input);
err0:
	return err;
}

static void poll_function_work_mag(struct work_struct *input_work_mag)
{
	struct k303c_mag_p *data;
	int xyz[3] = { 0 };
	int err;

	data = container_of((struct work_struct *)input_work_mag,
			struct k303c_mag_p, input_work_mag);

	mutex_lock(&data->lock);

	if (atomic_read(&data->enabled_mag)) {
		err = k303c_mag_get_data(data, xyz);
		if (err < 0)
			dev_err(&data->client->dev,
					"get_magnetometer_data failed\n");
		else
			k303c_mag_report_values(data, xyz);
	}

	mutex_unlock(&data->lock);
	hrtimer_start(&data->hr_timer_mag, data->ktime_mag, HRTIMER_MODE_REL);
}

enum hrtimer_restart poll_function_read_mag(struct hrtimer *timer)
{
	struct k303c_mag_p *data;

	data = container_of((struct hrtimer *)timer,
				struct k303c_mag_p, hr_timer_mag);

	queue_work(data->mag_wq, &data->input_work_mag);
	return HRTIMER_NORESTART;
}

static int k303c_mag_parse_dt(struct k303c_mag_p *data, struct device *dev)
{
	struct device_node *dev_node = dev->of_node;
	int ret;
	u32 temp;

	if (dev_node == NULL)
		return -ENODEV;

	ret = of_property_read_u32(dev_node, "k303c_mag,axis_map_x", &temp);
	if ((data->pdata->axis_map_x > 2) || (ret < 0)) {
		pr_err("[SENSOR]: %s: invalid x axis_map value %u\n",
			__func__, data->pdata->axis_map_x);
		data->pdata->axis_map_x = 0;
	} else {
		data->pdata->axis_map_x = (u8)temp;
	}

	ret = of_property_read_u32(dev_node, "k303c_mag,axis_map_y", &temp);
	if ((data->pdata->axis_map_y > 2) || (ret < 0)) {
		pr_err("[SENSOR]: %s: invalid y axis_map value %u\n",
			__func__, data->pdata->axis_map_y);
		data->pdata->axis_map_y = 1;
	} else {
		data->pdata->axis_map_y = (u8)temp;
	}

	ret = of_property_read_u32(dev_node, "k303c_mag,axis_map_z", &temp);
	if ((data->pdata->axis_map_z > 2) || (ret < 0)) {
		pr_err("[SENSOR]: %s: invalid z axis_map value %u\n",
			__func__, data->pdata->axis_map_z);
		data->pdata->axis_map_z = 2;
	} else {
		data->pdata->axis_map_z = (u8)temp;
	}

	ret = of_property_read_u32(dev_node, "k303c_mag,negate_x", &temp);
	if ((data->pdata->negate_x > 1) || (ret < 0)) {
		pr_err("[SENSOR]: %s: invalid x axis_map value %u\n",
			__func__, data->pdata->negate_x);
		data->pdata->negate_x = 0;
	} else {
		data->pdata->negate_x = (u8)temp;
	}

	ret = of_property_read_u32(dev_node, "k303c_mag,negate_y", &temp);
	if ((data->pdata->negate_y > 1) || (ret < 0)) {
		pr_err("[SENSOR]: %s: invalid y axis_map value %u\n",
			__func__, data->pdata->negate_y);
		data->pdata->negate_y = 0;
	} else {
		data->pdata->negate_y = (u8)temp;
	}

	ret = of_property_read_u32(dev_node, "k303c_mag,negate_z", &temp);
	if ((data->pdata->negate_z > 1) || (ret < 0)) {
		pr_err("[SENSOR]: %s: invalid z axis_map value %u\n",
			__func__, data->pdata->negate_z);
		data->pdata->negate_z = 0;
	} else {
		data->pdata->negate_z = (u8)temp;
	}

	data->reg_vdd = devm_regulator_get(dev, "k303c_mag,vdd");
	if (IS_ERR(data->reg_vdd)) {
		pr_err("[SENSOR] could not get vdd, %ld\n",
			PTR_ERR(data->reg_vdd));
	} else {
		if (!((AVG_VDD <= regulator_get_voltage(data->reg_vdd))
			&& (regulator_get_voltage(data->reg_vdd) <= MAX_VDD)))
			ret = regulator_set_voltage(data->reg_vdd,
				AVG_VDD, AVG_VDD);
		if (ret) {
			pr_err("[SENSOR]: %s: set voltage failed on vdd, rc=%d\n",
				__func__, ret);
			return ret;
		}
	}

	data->reg_vio = devm_regulator_get(dev, "k303c_mag,vio");
	if (IS_ERR(data->reg_vio)) {
		pr_err("[SENSOR] could not get vio, %ld\n",
			PTR_ERR(data->reg_vio));
	} else {
		ret = regulator_set_voltage(data->reg_vio, AVG_VIO, AVG_VIO);
		if (ret) {
			pr_err("[SENSOR]: %s: set voltage failed on vio rc=%d\n",
				__func__, ret);
			return ret;
		}
	}
	return 0;
}

static ssize_t k303c_mag_adc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct k303c_mag_p *data;
	int xyz[3] = { 0 };
	int err;

	pr_info("%s\n", __func__);

	data = dev_get_drvdata(dev);

	mutex_lock(&data->lock);

	if (!atomic_read(&data->enabled_mag)) {
		k303c_mag_device_power_on(data);
		err = k303c_mag_get_data(data, xyz);
		k303c_mag_device_power_off(data);
	} else {
		err = k303c_mag_get_data(data, xyz);
	}

	mutex_unlock(&data->lock);

	if (err < 0)
		pr_err("%s k303c_mag_get_data err\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", xyz[0], xyz[1], xyz[2]);
}

static ssize_t k303c_mag_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("%s\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%s\n", MODEL_NAME);
}


static ssize_t k303c_mag_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct k303c_mag_p *data;
	int xyz[3] = { 0 };
	int err;

	pr_info("%s\n", __func__);

	data = dev_get_drvdata(dev);

	mutex_lock(&data->lock);

	if (!atomic_read(&data->enabled_mag)) {
		k303c_mag_device_power_on(data);
		err = k303c_mag_get_data(data, xyz);
		k303c_mag_device_power_off(data);
	} else {
		err = k303c_mag_get_data(data, xyz);
	}

	mutex_unlock(&data->lock);

	if (err < 0)
		pr_err("%s k303c_mag_get_data err\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", xyz[0], xyz[1], xyz[2]);
}

static ssize_t k303c_mag_selftest_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct k303c_mag_p *data = dev_get_drvdata(dev);
	int val, i, en_state = 0;
	ssize_t ret;
	u8 x[8];
	s32 NO_ST[3] = {0, 0, 0};
	s32 ST[3] = {0, 0, 0};
	s32 ST_MIN_LSB, ST_MAX_LSB;

	pr_info("%s\n", __func__);

	en_state = atomic_read(&data->enabled_mag);
	k303c_mag_disable(data);

	k303c_mag_device_power_on(data);

	x[0] = REG_CNTRL1_ADDR;
	x[1] = 0x1c;
	x[2] = 0x60;
	k303c_mag_i2c_write(data, x, 2);

	msleep(20);

	x[0] = REG_CNTRL3_ADDR;
	x[1] = 0x00;
	k303c_mag_i2c_write(data, x, 1);

	msleep(20);

	x[0] = REG_MAG_DATA_ADDR;
	k303c_mag_i2c_read(data, x, 6);

	for (i = 0; i < 6; i++) {
		while (1) {
			x[0] = 0x27;
			val = k303c_mag_i2c_read(data, x, 1);
			if (val < 0) {
				pr_err("[SENSOR] %s : I2C fail. (%d)\n",
					__func__, val);
				goto ST_EXIT;
			}
			if (x[0] & 0x08)
				break;
		}
		x[0] = REG_MAG_DATA_ADDR;
		k303c_mag_i2c_read(data, x, 6);
		if (i > 0) {
			NO_ST[0] += (s16)((x[1] << 8) | x[0]);
			NO_ST[1] += (s16)((x[3] << 8) | x[2]);
			NO_ST[2] += (s16)((x[5] << 8) | x[4]);
		}
	}
	NO_ST[0] /= 5;
	NO_ST[1] /= 5;
	NO_ST[2] /= 5;

	x[0] = REG_CNTRL1_ADDR;
	x[1] = 0x1d;
	k303c_mag_i2c_write(data, x, 1);

	msleep(60);

	x[0] = REG_MAG_DATA_ADDR;
	k303c_mag_i2c_read(data, x, 6);

	for (i = 0; i < 6; i++) {
		while (1) {
			x[0] = 0x27;
			val = k303c_mag_i2c_read(data, x, 1);
			if (val < 0) {
				pr_err("[SENSOR] %s : I2C fail. (%d)\n",
					__func__, val);
				goto ST_EXIT;
			}
			if (x[0] & 0x08)
				break;
		}
		x[0] = REG_MAG_DATA_ADDR;
		k303c_mag_i2c_read(data, x, 6);
		if (i > 0) {
			ST[0] += (s16)((x[1] << 8) | x[0]);
			ST[1] += (s16)((x[3] << 8) | x[2]);
			ST[2] += (s16)((x[5] << 8) | x[4]);
		}
	}
	ST[0] /= 5;
	ST[1] /= 5;
	ST[2] /= 5;

	for (val = 1, i = 0; i < 3; i++) {
		ST[i] -= NO_ST[i];

		switch (i) {
		case 0:
		case 1:
			ST_MIN_LSB = MAG_SELF_TEST_16G_MIN_LSB;
			ST_MAX_LSB = MAG_SELF_TEST_16G_MAX_LSB;
			break;
		case 2:
			ST_MIN_LSB = MAG_SELF_TEST_16G_MIN_LSB_Z;
			ST_MAX_LSB = MAG_SELF_TEST_16G_MAX_LSB_Z;
			break;
		default:
			ST_MIN_LSB = 0;
			ST_MAX_LSB = 0;
		}

		if ((ST_MIN_LSB > ST[i]) || (ST[i] > ST_MAX_LSB)) {
			pr_err("[SENSOR] %s :[%d]: Out of range!! (%d)\n",
				__func__, i, ST[i]);
			val = 0;
		}
	}

ST_EXIT:
	x[0] = REG_CNTRL1_ADDR;
	x[1] = 0x00;
	k303c_mag_i2c_write(data, x, 1);
	x[0] = REG_CNTRL3_ADDR;
	x[1] = 0x00;
	k303c_mag_i2c_write(data, x, 1);

	k303c_mag_device_power_off(data);

	if (en_state)
		k303c_mag_enable(data);

	if (val) {
		pr_info("[SENSOR] %s :Self test: OK (%d, %d, %d)\n",
			__func__, ST[0], ST[1], ST[2]);
		ret = snprintf(buf, PAGE_SIZE, "1,%d,%d,%d\n",
			ST[0], ST[1], ST[2]);
	} else {
		pr_info("[SENSOR] %s :Self test: NG (%d, %d, %d)\n",
			__func__, ST[0], ST[1], ST[2]);
		ret = snprintf(buf, PAGE_SIZE, "0,%d,%d,%d\n",
			ST[0], ST[1], ST[2]);
	}

	return ret;
}

static ssize_t k303c_mag_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct k303c_mag_p *data = dev_get_drvdata(dev);
	int err;
	u8 val;

	pr_info("%s\n", __func__);

	if (!atomic_read(&data->enabled_mag)) {
		k303c_mag_device_power_on(data);
		val = dataus_registers.who_am_i.address;
		err = k303c_mag_i2c_read(data, &val, 1);
		k303c_mag_device_power_off(data);
	} else {
		val = dataus_registers.who_am_i.address;
		err = k303c_mag_i2c_read(data, &val, 1);
	}

	if (err < 0) {
		pr_err("%s k303c_mag_i2c_read err:%d\n", __func__, err);
		err = 0;
	} else if (val != dataus_registers.who_am_i.value) {
		pr_err("%s value val:%d err:%d\n", __func__, val, err);
		err = 0;
	} else {
		err = 1;
	}
	return snprintf(buf, PAGE_SIZE, "%d,%d\n", err, 0);
}

static ssize_t k303c_mag_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("%s\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static DEVICE_ATTR(adc, S_IRUGO, k303c_mag_adc_show, NULL);
static DEVICE_ATTR(name, S_IRUGO, k303c_mag_name_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO, k303c_mag_raw_data_show, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, k303c_mag_selftest_show, NULL);
static DEVICE_ATTR(status, S_IRUGO, k303c_mag_status_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, k303c_mag_vendor_show, NULL);

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_adc,
	&dev_attr_name,
	&dev_attr_raw_data,
	&dev_attr_selftest,
	&dev_attr_status,
	&dev_attr_vendor,
	NULL,
};

static int k303c_mag_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct k303c_mag_p *data;

	u32 smbus_func = I2C_FUNC_SMBUS_BYTE_DATA |
		I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_I2C_BLOCK;

	int err = -1;
	pr_err("[SENSOR] %s, Start!\n", __func__);
	data = kzalloc(sizeof(struct k303c_mag_p), GFP_KERNEL);
	if (data == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to kzalloc err:%d\n", err);
		goto exit_kzalloc_failed;
	}

	data->use_smbus = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_warn(&client->dev, "client not i2c capable\n");
		if (i2c_check_functionality(client->adapter, smbus_func)) {
			data->use_smbus = 1;
			dev_warn(&client->dev, "client using SMBUS\n");
		} else {
			err = -ENODEV;
			dev_err(&client->dev, "client nor SMBUS capable\n");
			goto exit_check_functionality_failed;
		}
	}

	if (data->mag_wq == 0)
		data->mag_wq = create_workqueue("mag_wq");

	hrtimer_init(&data->hr_timer_mag, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->hr_timer_mag.function = &poll_function_read_mag;

	mutex_init(&data->lock);
	mutex_lock(&data->lock);

	data->client = client;
	i2c_set_clientdata(client, data);

	data->pdata = kmalloc(sizeof(*data->pdata), GFP_KERNEL);
	if (data->pdata == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
			"failed to allocate memory for pdata: %d\n", err);
		goto exit_pdata_kmalloc;
	}

	if (client->dev.platform_data == NULL) {
		memcpy(data->pdata, &default_k303c_mag_pdata,
						sizeof(*data->pdata));
		dev_info(&client->dev,
			"using default plaform_data for magnetometer\n");
	} else {
		memcpy(data->pdata, client->dev.platform_data,
						sizeof(*data->pdata));
	}

	err = k303c_mag_parse_dt(data, &client->dev);
	if (err < 0) {
		pr_err("[SENSOR] %s, of_node error\n", __func__);
		err = -ENODEV;
		goto exit_of_node;
	}

	err = k303c_mag_validate_pdata(data);
	if (err < 0) {
		dev_err(&client->dev, "failed to k303c_mag_validate_pdata\n");
		goto exit_kfree_pdata;
	}

	err = k303c_mag_hw_init(data);
	if (err < 0) {
		dev_err(&client->dev, "hw init failed: %d\n", err);
		goto err_hw_init;
	}

	err = k303c_mag_device_power_on(data);
	if (err < 0) {
		dev_err(&client->dev,
			"magnetometer power on failed: %d\n", err);
		goto err_pdata_init;
	}

	err = k303c_mag_update_fs_range(data, data->pdata->fs_range);
	if (err < 0) {
		dev_err(&client->dev,
			"update_fs_range on magnetometer failed\n");
		goto  err_power_off_mag;
	}

	err = k303c_mag_update_odr(data, data->pdata->poll_interval);
	if (err < 0) {
		dev_err(&client->dev, "update_odr on magnetometer failed\n");
		goto  err_power_off;
	}

	err = k303c_mag_input_init(data);
	if (err < 0) {
		dev_err(&client->dev, "magnetometer input init failed\n");
		goto err_power_off;
	}

	sensors_register(data->factory_device, data, sensor_attrs, MODULE_NAME);

	k303c_mag_device_power_off(data);

	INIT_WORK(&data->input_work_mag, poll_function_work_mag);

	mutex_unlock(&data->lock);
	pr_info("[SENSOR] %s done!\n", __func__);
	return 0;

err_power_off:
	k303c_mag_input_cleanup(data);
err_power_off_mag:
	k303c_mag_device_power_off(data);
err_hw_init:
err_pdata_init:
exit_kfree_pdata:
	mutex_unlock(&data->lock);
	if (!data->mag_wq) {
		flush_workqueue(data->mag_wq);
		destroy_workqueue(data->mag_wq);
	}
exit_of_node:
exit_pdata_kmalloc:
exit_check_functionality_failed:
	kfree(data);
exit_kzalloc_failed:
	pr_err("[SENSOR] %s: Driver Init failed\n", K303C_MAG_DEV_NAME);
	return err;
}

static int k303c_mag_resume(struct device *dev)
{
	struct k303c_mag_p *data = dev_get_drvdata(dev);

	if (data->on_before_suspend)
		return k303c_mag_enable(data);

	return 0;
}

static int k303c_mag_suspend(struct device *dev)
{
	struct k303c_mag_p *data = dev_get_drvdata(dev);

	data->on_before_suspend = atomic_read(&data->enabled_mag);
	k303c_mag_disable(data);

	return 0;
}

static int k303c_mag_remove(struct i2c_client *client)
{
	struct k303c_mag_p *data = i2c_get_clientdata(client);

	k303c_mag_disable(data);
	k303c_mag_input_cleanup(data);

	remove_sysfs_interfaces(&client->dev);

	if (!data->mag_wq) {
		flush_workqueue(data->mag_wq);
		destroy_workqueue(data->mag_wq);
	}

	devm_regulator_put(data->reg_vdd);
	devm_regulator_put(data->reg_vio);

	kfree(data);
	return 0;
}

static const struct i2c_device_id k303c_mag_id[] = {
		{ "k303c_mag", 0 },
		{ },
};
static struct of_device_id k303c_mag_table[] = {
	{ .compatible = "k303c_mag",},
	{},
};

MODULE_DEVICE_TABLE(i2c, k303c_mag_id);

static const struct dev_pm_ops k303c_mag_pm_ops = {
	.suspend = k303c_mag_suspend,
	.resume = k303c_mag_resume
};

static struct i2c_driver k303c_mag_driver = {
	.driver = {
			.owner = THIS_MODULE,
			.name = K303C_MAG_DEV_NAME,
			.of_match_table = k303c_mag_table,
			.pm = &k303c_mag_pm_ops
	},
	.probe = k303c_mag_probe,
	.remove = k303c_mag_remove,
	.id_table = k303c_mag_id,
};

static int __init k303c_mag_init(void)
{
	return i2c_add_driver(&k303c_mag_driver);
}

static void __exit k303c_mag_exit(void)
{
	i2c_del_driver(&k303c_mag_driver);
}

module_init(k303c_mag_init);
module_exit(k303c_mag_exit);

MODULE_DESCRIPTION("k303c magnetometer driver");
MODULE_AUTHOR("Matteo Dameno, Denis Ciocca, STMicroelectronics");
MODULE_LICENSE("GPL");
