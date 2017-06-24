/******************** (C) COPYRIGHT 2013 STMicroelectronics ********************
 *
 * File Name          : lis2hh_acc.c
 * Authors            : AMS - Motion Mems Division - Application Team
 *		      : Matteo Dameno (matteo.dameno@st.com)
 *		      : Denis Ciocca (denis.ciocca@st.com)
 *		      : Both authors are willing to be considered the contact
 *		      : and update points for the driver.
 * Version            : V.1.1.0
 * Date               : 2013/Mar/28
 * Description        : LIS2HH accelerometer sensor API
 *
 *******************************************************************************
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
 ******************************************************************************
 Revision 1.0.0 25/Feb/2013
  first revision
  supports sysfs;
 Revision 1.1.0 28/Mar/2013
  introduces hr_timers for polling;
 ******************************************************************************/

#include	<linux/err.h>
#include	<linux/errno.h>
#include	<linux/delay.h>
#include	<linux/fs.h>
#include	<linux/i2c.h>
#include	<linux/input.h>
#include	<linux/uaccess.h>
#include	<linux/workqueue.h>
#include	<linux/irq.h>
#include	<linux/gpio.h>
#include	<linux/interrupt.h>
#include	<linux/slab.h>
#include	<linux/kernel.h>
#include	<linux/device.h>
#include	<linux/module.h>
#include	<linux/moduleparam.h>
#include <linux/of_gpio.h>
#include <linux/math64.h>
#include <linux/regulator/consumer.h>
#include "sensors_core.h"
#include	"lis2hh.h"
#define DEBUG		1
#define VENDOR		"STM"
#define CHIP_ID		"K2HH"
#define MODULE_NAME "accelerometer_sensor"

#define G_MAX			7995148 /* (SENSITIVITY_8G*(2^15-1)) */
#define G_MIN			- 7995392 /* (-SENSITIVITY_8G*(2^15)   */
#define FUZZ			0
#define FLAT			0
#define I2C_RETRY_DELAY		5
#define I2C_RETRIES		5
#define I2C_AUTO_INCREMENT	(0x00)

#define MS_TO_NS(x)			(x*1000000L)

#define SENSITIVITY_2G		 61	/**	ug/LSB	*/
#define SENSITIVITY_4G		122	/**	ug/LSB	*/
#define SENSITIVITY_8G		244	/**	ug/LSB	*/


/* Accelerometer Sensor Operating Mode */
#define LIS2HH_ACC_ENABLE	(0x01)
#define LIS2HH_ACC_DISABLE	(0x00)

#define AXISDATA_REG		(0x28)
#define WHOAMI_LIS2HH_ACC	(0x41)	/*	Expctd content for WAI	*/
#define ALL_ZEROES		(0x00)
#define LIS2HH_ACC_PM_OFF	(0x00)
#define ACC_ENABLE_ALL_AXES	(0x07)

/* Register Auto-increase */
#define AC			(1 << 7)
/*	CONTROL REGISTERS	*/
#define TEMP_L			(0x0B)
#define TEMP_H			(0x0C)
#define WHO_AM_I		(0x0F)	/*	WhoAmI register		*/
#define ACT_THS			(0x1E)	/*	Activity Threshold	*/
#define ACT_DUR			(0x1F)	/*	Activity Duration	*/
/* ctrl 1: HR ODR2 ODR1 ODR0 BDU Zenable Yenable Xenable */
#define CTRL1			(0x20)	/*	control reg 1		*/
#define CTRL2			(0x21)	/*	control reg 2		*/
#define CTRL3			(0x22)	/*	control reg 3		*/
#define CTRL4			(0x23)	/*	control reg 4		*/
#define CTRL5			(0x24)	/*	control reg 5		*/
#define CTRL6			(0x25)	/*	control reg 6		*/
#define CTRL7			(0x26)	/*	control reg 7		*/
#define STATUS_REG		0x27
#define OUT_X_L			0x28
#define OUT_X_H			0x29
#define OUT_Y_L			0x2A
#define OUT_Y_H			0x2B
#define OUT_Z_L			0x2C
#define OUT_Z_H			0x2D
#define FIFO_CTRL		(0x2E)	/*	fifo control reg	*/
#define FIFO_SRC_REG		0x2F
#define INT_CFG1		(0x30)	/*	interrupt 1 config	*/
#define INT_SRC1		(0x31)	/*	interrupt 1 source	*/
#define INT_THSX1		(0x32)	/*	interrupt 1 threshold x	*/
#define INT_THSY1		(0x33)	/*	interrupt 1 threshold y	*/
#define INT_THSZ1		(0x34)	/*	interrupt 1 threshold z	*/
#define INT_DUR1		(0x35)	/*	interrupt 1 duration	*/

#define INT_CFG2		(0x36)	/*	interrupt 2 config	*/
#define INT_SRC2		(0x37)	/*	interrupt 2 source	*/
#define INT_THS2		(0x38)	/*	interrupt 2 threshold	*/
#define INT_DUR2		(0x39)	/*	interrupt 2 duration	*/

#define REF_XL			(0x3A)	/*	reference_l_x		*/
#define REF_XH			(0x3B)	/*	reference_h_x		*/
#define REF_YL			(0x3C)	/*	reference_l_y		*/
#define REF_YH			(0x3D)	/*	reference_h_y		*/
#define REF_ZL			(0x3E)	/*	reference_l_z		*/
#define REF_ZH			(0x3F)	/*	reference_h_z		*/
/*	end CONTROL REGISTRES	*/



#define ACC_ODR10		(0x10)	/*   10Hz output data rate */
#define ACC_ODR50		(0x20)	/*   50Hz output data rate */
#define ACC_ODR100		(0x30)	/*  100Hz output data rate */
#define ACC_ODR200		(0x40)	/*  200Hz output data rate */
#define ACC_ODR400		(0x50)	/*  400Hz output data rate */
#define ACC_ODR800		(0x60)	/*  800Hz output data rate */
#define ACC_ODR_MASK		(0X70)

/* Registers configuration Mask and settings */
/* CTRL1 */
#define CTRL1_HR_DISABLE	(0x00)
#define CTRL1_HR_ENABLE		(0x80)
#define CTRL1_HR_MASK		(0x80)
#define CTRL1_BDU_ENABLE	(0x08)
#define CTRL1_BDU_MASK		(0x08)

/* CTRL2 */
#define CTRL2_IG1_INT1		(0x08)

/* CTRL3 */
#define CTRL3_IG1_INT1		(0x08)
#define CTRL3_DRDY_INT1

/* CTRL4 */
#define CTRL4_IF_ADD_INC_EN	(0x04)
#define CTRL4_BW_SCALE_ODR_AUT	(0x00)
#define CTRL4_BW_SCALE_ODR_SEL	(0x08)
#define CTRL4_ANTALIAS_BW_400	(0x00)
#define CTRL4_ANTALIAS_BW_200	(0x40)
#define CTRL4_ANTALIAS_BW_100	(0x80)
#define CTRL4_ANTALIAS_BW_50	(0xC0)
#define CTRL4_ANTALIAS_BW_MASK	(0xC0)

/* CTRL5 */
#define CTRL5_HLACTIVE_L	(0x02)
#define CTRL5_HLACTIVE_H	(0x00)

/* CTRL6 */
#define CTRL6_IG2_INT2		(0x10)
#define CTRL6_DRDY_INT2		(0x01)

/* CTRL7 */
#define CTRL7_LIR2		(0x08)
#define CTRL7_LIR1		(0x04)
/* */

#define NO_MASK			(0xFF)

#define INT1_DURATION_MASK	(0x7F)
#define INT1_THRESHOLD_MASK	(0x7F)



/* RESUME STATE INDICES */
#define RES_CTRL1		0
#define RES_CTRL2		1
#define RES_CTRL3		2
#define RES_CTRL4		3
#define RES_CTRL5		4
#define RES_CTRL6		5
#define RES_CTRL7		6

#define RES_INT_CFG1		7
#define RES_INT_THSX1		8
#define RES_INT_THSY1		9
#define RES_INT_THSZ1		10
#define RES_INT_DUR1		11


#define RES_INT_CFG2		12
#define RES_INT_THS2		13
#define RES_INT_DUR2		14

#define RES_TEMP_CFG_REG	15
#define RES_REFERENCE_REG	16
#define RES_FIFO_CTRL	17

#define RESUME_ENTRIES		18
#define CAL_DATA_AMOUNT	20
/* end RESUME STATE INDICES */
#define PM_OFF			0x00
#define ODR400			0x70  /* 400Hz output data rate */
#define CALIBRATION_FILE_PATH	"/efs/calibration_data"
#define OUTPUT_ALWAYS_ANTI_ALIASED 1
#define DEFAULT_POWER_ON_SETTING (ODR400 | STATUS_REG)

#define SELF_TEST_2G_MAX_LSB	(24576)
#define SELF_TEST_2G_MIN_LSB	(1146)

struct {
	unsigned int cutoff_ms;
	unsigned int mask;
} lis2hh_acc_odr_table[] = {
		{    2, ACC_ODR800 },
		{    3, ACC_ODR400  },
		{    5, ACC_ODR200  },
		{   10, ACC_ODR100  },
#if(!OUTPUT_ALWAYS_ANTI_ALIASED)
		{   20, ACC_ODR50   },
		{  100, ACC_ODR10   },
#endif
};

struct k2hh_acc {
	s16 x;
	s16 y;
	s16 z;
};
static int int1_gpio = LIS2HH_ACC_DEFAULT_INT1_GPIO;
static int int2_gpio = LIS2HH_ACC_DEFAULT_INT2_GPIO;
module_param(int1_gpio, int, S_IRUGO);
module_param(int2_gpio, int, S_IRUGO);

struct lis2hh_acc_status {
	struct i2c_client *client;
	struct lis2hh_acc_platform_data *pdata;
	struct mutex lock;
	struct work_struct input_poll_work;
	struct hrtimer hr_timer_poll;
	ktime_t polling_ktime;
	struct workqueue_struct *hr_timer_poll_work_queue;
	struct input_dev *input_dev;
	int hw_initialized;
	/* hw_working=-1 means not tested yet */
	int hw_working;
	atomic_t enable;
	int on_before_suspend;
	int use_smbus;
	u8 sensitivity;
	u8 resume_state[RESUME_ENTRIES];
	struct device *dev;
	int irq1;
	struct work_struct irq1_work;
	struct workqueue_struct *irq1_work_queue;
	int irq2;
	struct work_struct irq2_work;
	struct workqueue_struct *irq2_work_queue;
	struct regulator *l19;
	struct regulator *lvs1_1p8;
	struct k2hh_acc cal_data;
#ifdef DEBUG
	u8 reg_addr;
#endif
};

/* sets default init values to be written in registers at probe stage */
static void lis2hh_acc_set_init_register_values(struct lis2hh_acc_status *stat)
{

	memset(stat->resume_state, 0, ARRAY_SIZE(stat->resume_state));

	stat->resume_state[RES_CTRL1] = (ALL_ZEROES | \
					CTRL1_HR_DISABLE | \
					CTRL1_BDU_ENABLE | \
					ACC_ENABLE_ALL_AXES);

	if(stat->pdata->gpio_int1 >= 0)
		stat->resume_state[RES_CTRL3] =
			(stat->resume_state[RES_CTRL3] | \
						CTRL3_IG1_INT1);

	stat->resume_state[RES_CTRL4] = (ALL_ZEROES | \
						CTRL4_IF_ADD_INC_EN);

	stat->resume_state[RES_CTRL5] = (ALL_ZEROES | \
						CTRL5_HLACTIVE_H);

	if(stat->pdata->gpio_int2 >= 0)
		stat->resume_state[RES_CTRL6] =
			(stat->resume_state[RES_CTRL6] | \
						CTRL6_IG2_INT2);

	stat->resume_state[RES_CTRL7] = (ALL_ZEROES | \
					CTRL7_LIR2 | CTRL7_LIR1);

}

int k2hh_power_on(struct lis2hh_acc_status *data, bool onoff)
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
			pr_err("%s: regulator_get for 8226_lvs1 failed\n", __func__);
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
	}
	else {
		ret = regulator_disable(data->l19);
		if (ret) {
			pr_err("%s: Failed to disable regulatorl19.\n",
				__func__);
			return ret;
		}
		ret = regulator_enable(data->lvs1_1p8);
		if (ret) {
			pr_err("%s: Failed to disable regulator lvs1_1p8.\n",
				__func__);
			return ret;
		}
	}
	return 0;
}

static int lis2hh_acc_i2c_read(struct lis2hh_acc_status *stat, u8 *buf,
									int len)
{
	int ret;
	u8 reg = buf[0];
	u8 cmd = reg;
	unsigned int ii;
	if (len > 1)
		cmd = (I2C_AUTO_INCREMENT | reg);
	if (stat->use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_read_byte_data(stat->client, cmd);
			buf[0] = ret & 0xff;
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_read_byte_data: ret=0x%02x, len:%d ,"
				"command=0x%02x, buf[0]=0x%02x\n",
				ret, len, cmd , buf[0]);
#endif
		} else if (len > 1) {
			ret = i2c_smbus_read_i2c_block_data(stat->client,
								cmd, len, buf);
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_read_i2c_block_data: ret:%d len:%d, "
				"command=0x%02x, ",
				ret, len, cmd);

			for (ii = 0; ii < len; ii++)
				printk(KERN_DEBUG "buf[%d]=0x%02x,",
								ii, buf[ii]);

			printk("\n");
#endif
		} else
			ret = -1;

		if (ret < 0) {
			dev_err(&stat->client->dev,
				"read transfer error: len:%d, command=0x%02x\n",
				len, cmd);
			return 0; /* failure */
		}
		return len; /* success */
	}

	ret = i2c_master_send(stat->client, &cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
		return ret;

	return i2c_master_recv(stat->client, buf, len);
}

static int lis2hh_acc_i2c_write(struct lis2hh_acc_status *stat, u8 *buf,
									int len)
{
	int ret;
	u8 reg, value;
	unsigned int ii;
	if (len > 1)
		buf[0] = (I2C_AUTO_INCREMENT | buf[0]);

	reg = buf[0];
	value = buf[1];

	if (stat->use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_write_byte_data(stat->client,
								reg, value);
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_write_byte_data: ret=%d, len:%d, "
				"command=0x%02x, value=0x%02x\n",
				ret, len, reg , value);
#endif
			return ret;
		} else if (len > 1) {
			ret = i2c_smbus_write_i2c_block_data(stat->client,
							reg, len, buf + 1);
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_write_i2c_block_data: ret=%d, "
				"len:%d, command=0x%02x, ",
				ret, len, reg);

			for (ii = 0; ii < (len + 1); ii++)
				printk(KERN_DEBUG "value[%d]=0x%02x,",
								ii, buf[ii]);

			printk("\n");
#endif
			return ret;
		}
	}

	ret = i2c_master_send(stat->client, buf, len+1);
	return (ret == len+1) ? 0 : ret;
}

static int lis2hh_acc_hw_init(struct lis2hh_acc_status *stat)
{
	int err = -1;
	u8 buf[7];

	pr_info("%s: hw init start\n", LIS2HH_ACC_DEV_NAME);

	buf[0] = WHO_AM_I;
	err = lis2hh_acc_i2c_read(stat, buf, 1);
	if (err < 0) {
		dev_warn(&stat->client->dev, "Error reading WHO_AM_I:"
				" is device available/working?\n");
		goto err_firstread;
	} else
		stat->hw_working = 1;

	if (buf[0] != WHOAMI_LIS2HH_ACC) {
		dev_err(&stat->client->dev,
			"device unknown. Expected: 0x%02x,"
			" Replies: 0x%02x\n",
			WHOAMI_LIS2HH_ACC, buf[0]);
		err = -1; /* choose the right coded error */
		goto err_unknown_device;
	}

	buf[0] = FIFO_CTRL;
	buf[1] = stat->resume_state[RES_FIFO_CTRL];
	err = lis2hh_acc_i2c_write(stat, buf, 1);
	if (err < 0)
		goto err_resume_state;

	buf[0] = INT_THSX1;
	buf[1] = stat->resume_state[RES_INT_THSX1];
	buf[2] = stat->resume_state[RES_INT_THSY1];
	buf[3] = stat->resume_state[RES_INT_THSZ1];
	buf[4] = stat->resume_state[RES_INT_DUR1];
	err = lis2hh_acc_i2c_write(stat, buf, 4);
	if (err < 0)
		goto err_resume_state;
	buf[0] = INT_CFG1;
	buf[1] = stat->resume_state[RES_INT_CFG1];
	err = lis2hh_acc_i2c_write(stat, buf, 1);
	if (err < 0)
		goto err_resume_state;


	buf[0] = CTRL2;
	buf[1] = stat->resume_state[RES_CTRL2];
	buf[2] = stat->resume_state[RES_CTRL3];
	buf[3] = stat->resume_state[RES_CTRL4];
	buf[4] = stat->resume_state[RES_CTRL5];
	buf[5] = stat->resume_state[RES_CTRL6];
	buf[6] = stat->resume_state[RES_CTRL7];
	err = lis2hh_acc_i2c_write(stat, buf, 6);
	if (err < 0)
		goto err_resume_state;

	buf[0] = CTRL1;
	buf[1] = stat->resume_state[RES_CTRL1];
	err = lis2hh_acc_i2c_write(stat, buf, 1);
	if (err < 0)
		goto err_resume_state;

	stat->hw_initialized = 1;
	pr_info("%s: hw init done\n", LIS2HH_ACC_DEV_NAME);
	return 0;

err_firstread:
	stat->hw_working = 0;
err_unknown_device:
err_resume_state:
	stat->hw_initialized = 0;
	dev_err(&stat->client->dev, "hw init error 0x%02x,0x%02x: %d\n", buf[0],
			buf[1], err);
	return err;
}

static void lis2hh_acc_device_power_off(struct lis2hh_acc_status *stat)
{
	int err;
	u8 buf[2] = { CTRL1, LIS2HH_ACC_PM_OFF };

	err = lis2hh_acc_i2c_write(stat, buf, 1);
	if (err < 0)
		dev_err(&stat->client->dev, "soft power off failed: %d\n", err);

	if (stat->pdata->power_off) {
		if (stat->pdata->gpio_int1 >= 0)
			disable_irq_nosync(stat->irq1);
		if (stat->pdata->gpio_int2 >= 0)
			disable_irq_nosync(stat->irq2);
		stat->pdata->power_off();
		stat->hw_initialized = 0;
	}
	if (stat->hw_initialized) {
		if (stat->pdata->gpio_int1 >= 0)
			disable_irq_nosync(stat->irq1);
		if (stat->pdata->gpio_int2 >= 0)
			disable_irq_nosync(stat->irq2);
		stat->hw_initialized = 0;
	}

}

static int lis2hh_acc_device_power_on(struct lis2hh_acc_status *stat)
{
	int err = -1;

	if (stat->pdata->power_on) {
		err = stat->pdata->power_on();
		if (err < 0) {
			dev_err(&stat->client->dev,
					"power_on failed: %d\n", err);
			return err;
		}
		if (stat->pdata->gpio_int1 >= 0)
			enable_irq(stat->irq1);
		if (stat->pdata->gpio_int2 >= 0)
			enable_irq(stat->irq2);
	}

	mdelay(30);

	if (!stat->hw_initialized) {
		err = lis2hh_acc_hw_init(stat);
		if (stat->hw_working == 1 && err < 0) {
			lis2hh_acc_device_power_off(stat);
			return err;
		}
	}

	if (stat->hw_initialized) {
		if (stat->pdata->gpio_int1 >= 0)
			enable_irq(stat->irq1);
		if (stat->pdata->gpio_int2 >= 0)
			enable_irq(stat->irq2);
	}
	return 0;
}


static int lis2hh_acc_update_fs_range(struct lis2hh_acc_status *stat,
							u8 new_fs_range)
{
	int err = -1;

	u8 sensitivity;
	u8 buf[2];
	u8 updated_val;
	u8 init_val;
	u8 new_val;
	u8 mask = LIS2HH_ACC_FS_MASK;

	switch (new_fs_range) {
	case LIS2HH_ACC_FS_2G:

		sensitivity = SENSITIVITY_2G;
		break;
	case LIS2HH_ACC_FS_4G:

		sensitivity = SENSITIVITY_4G;
		break;
	case LIS2HH_ACC_FS_8G:

		sensitivity = SENSITIVITY_8G;
		break;
	default:
		dev_err(&stat->client->dev, "invalid fs range requested: %u\n",
				new_fs_range);
		return -EINVAL;
	}


	/* Updates configuration register 4,
	* which contains fs range setting */
	buf[0] = CTRL4;
	err = lis2hh_acc_i2c_read(stat, buf, 1);
	if (err < 0)
		goto error;
	init_val = buf[0];
	stat->resume_state[RES_CTRL4] = init_val;
	new_val = new_fs_range;
	updated_val = ((mask & new_val) | ((~mask) & init_val));
	buf[1] = updated_val;
	buf[0] = CTRL4;
	err = lis2hh_acc_i2c_write(stat, buf, 1);
	if (err < 0)
		goto error;
	stat->resume_state[RES_CTRL4] = updated_val;
	stat->sensitivity = sensitivity;

	return err;
error:
	dev_err(&stat->client->dev,
			"update fs range failed 0x%02x,0x%02x: %d\n",
			buf[0], buf[1], err);

	return err;
}

static int lis2hh_acc_update_odr(struct lis2hh_acc_status *stat,
							int poll_interval_ms)
{
	int err;
	int i;
	u8 config[2];
	u8 updated_val;
	u8 init_val;
	u8 new_val;
	u8 mask = ACC_ODR_MASK;

	/* Following, looks for the longest possible odr interval scrolling the
	 * odr_table vector from the end (shortest interval) backward (longest
	 * interval), to support the poll_interval requested by the system.
	 * It must be the longest interval lower then the poll interval.*/
	for (i = ARRAY_SIZE(lis2hh_acc_odr_table) - 1; i >= 0; i--) {
		if ((lis2hh_acc_odr_table[i].cutoff_ms <= poll_interval_ms)
								|| (i == 0))
			break;
	}
	new_val = lis2hh_acc_odr_table[i].mask;

	/* Updates configuration register 1,
	* which contains odr range setting if enabled,
	* otherwise updates RES_CTRL1 for when it will */
	if (atomic_read(&stat->enable)) {
		config[0] = CTRL1;
		err = lis2hh_acc_i2c_read(stat, config, 1);
		if (err < 0)
			goto error;
		init_val = config[0];
		stat->resume_state[RES_CTRL1] = init_val;
		updated_val = ((mask & new_val) | ((~mask) & init_val));
		config[1] = updated_val;
		config[0] = CTRL1;
		err = lis2hh_acc_i2c_write(stat, config, 1);
		if (err < 0)
			goto error;
		stat->resume_state[RES_CTRL1] = updated_val;
		return err;
	} else {
		init_val = stat->resume_state[RES_CTRL1];
		updated_val = ((mask & new_val) | ((~mask) & init_val));
		stat->resume_state[RES_CTRL1] = updated_val;
		return 0;
	}

error:
	dev_err(&stat->client->dev,
			"update odr failed 0x%02x,0x%02x: %d\n",
			config[0], config[1], err);

	return err;
}



static int lis2hh_acc_register_write(struct lis2hh_acc_status *stat,
					u8 *buf, u8 reg_address, u8 new_value)
{
	int err = -1;

		/* Sets configuration register at reg_address
		 *  NOTE: this is a straight overwrite  */
		buf[0] = reg_address;
		buf[1] = new_value;
		err = lis2hh_acc_i2c_write(stat, buf, 1);
		if (err < 0)
			return err;
	return err;
}


static int lis2hh_acc_get_data(
				struct lis2hh_acc_status *stat, int *xyz)
{
	int err = -1;
	/* Data bytes from hardware xL, xH, yL, yH, zL, zH */
	u8 acc_data[6];
	/* x,y,z hardware data */
	s32 hw_d[3] = { 0 };

	mutex_lock(&stat->lock);
	acc_data[0] = (AXISDATA_REG);
	err = lis2hh_acc_i2c_read(stat, acc_data, 6);
	mutex_unlock(&stat->lock);
	if (err < 0)
		return err;

	hw_d[0] = ((s16) ((acc_data[1] << 8) | acc_data[0]));
	hw_d[1] = ((s16) ((acc_data[3] << 8) | acc_data[2]));
	hw_d[2] = ((s16) ((acc_data[5] << 8) | acc_data[4]));
#if 0
	hw_d[0] = hw_d[0] * stat->sensitivity;
	hw_d[1] = hw_d[1] * stat->sensitivity;
	hw_d[2] = hw_d[2] * stat->sensitivity;
#endif

	xyz[0] = ((stat->pdata->negate_x) ? (-hw_d[stat->pdata->axis_map_x])
		   : (hw_d[stat->pdata->axis_map_x]));
	xyz[1] = ((stat->pdata->negate_y) ? (-hw_d[stat->pdata->axis_map_y])
		   : (hw_d[stat->pdata->axis_map_y]));
	xyz[2] = ((stat->pdata->negate_z) ? (-hw_d[stat->pdata->axis_map_z])
		   : (hw_d[stat->pdata->axis_map_z]));

	xyz[0] = xyz[0] - stat->cal_data.x ;
	xyz[1] = xyz[1] - stat->cal_data.y ;
	xyz[2] = xyz[2] - stat->cal_data.z ;

	/*printk("%s read x=%d, y=%d, z=%d\n",
			LIS2HH_ACC_DEV_NAME, xyz[0], xyz[1], xyz[2]);*/


#ifdef DEBUG

	dev_dbg(&stat->client->dev,"%s read x=%d, y=%d, z=%d\n",
			LIS2HH_ACC_DEV_NAME, xyz[0], xyz[1], xyz[2]);

#endif
	return err;
}

static void lis2hh_acc_report_values(struct lis2hh_acc_status *stat,
					int *xyz)
{
	input_report_rel(stat->input_dev, REL_X, xyz[0]);
	input_report_rel(stat->input_dev, REL_Y, xyz[1]);
	input_report_rel(stat->input_dev, REL_Z, xyz[2]);
	input_sync(stat->input_dev);

}

static void lis2hh_acc_report_triple(struct lis2hh_acc_status *stat)
{
	int err;
	int xyz[3];

	err = lis2hh_acc_get_data(stat, xyz);
	if (err < 0)
		dev_err(&stat->client->dev, "get_data failed\n");
	else
		lis2hh_acc_report_values(stat, xyz);

}

static irqreturn_t lis2hh_acc_isr1(int irq, void *dev)
{
	struct lis2hh_acc_status *stat = dev;
	printk("lis2hh_acc lis2hh_acc_isr1 called END  \n");
	disable_irq_nosync(irq);
	queue_work(stat->irq1_work_queue, &stat->irq1_work);
	printk("%s: isr1 queued %d \n", LIS2HH_ACC_DEV_NAME,stat->irq1);
	printk("lis2hh_acc lis2hh_acc_isr1 called END  \n");
	return IRQ_HANDLED;
}

static irqreturn_t lis2hh_acc_isr2(int irq, void *dev)
{
	struct lis2hh_acc_status *stat = dev;

	disable_irq_nosync(irq);
	queue_work(stat->irq2_work_queue, &stat->irq2_work);
	pr_debug("%s: isr2 queued\n", LIS2HH_ACC_DEV_NAME);

	return IRQ_HANDLED;
}

static void lis2hh_acc_irq1_work_func(struct work_struct *work)
{

	struct lis2hh_acc_status *stat =
	container_of(work, struct lis2hh_acc_status, irq1_work);
	/* TODO  add interrupt service procedure.
		 ie:lis2hh_acc_get_int1_source(stat); */
	/* ; */
	pr_debug("%s: IRQ1 served\n", LIS2HH_ACC_DEV_NAME);
/* exit: */
	enable_irq(stat->irq1);
}

static void lis2hh_acc_irq2_work_func(struct work_struct *work)
{

	struct lis2hh_acc_status *stat =
	container_of(work, struct lis2hh_acc_status, irq2_work);
	/* TODO  add interrupt service procedure.
		 ie:lis2hh_acc_get_tap_source(stat); */
	/* ; */
	pr_debug("%s: IRQ2 served\n", LIS2HH_ACC_DEV_NAME);
/* exit: */
	enable_irq(stat->irq2);
}

static int k2hh_open_calibration(struct lis2hh_acc_status *data)
{
	struct file *cal_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		err = PTR_ERR(cal_filp);
		if (err != -ENOENT)
			pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		return err;
	}

	err = cal_filp->f_op->read(cal_filp,
		(char *)&data->cal_data, 3 * sizeof(s16), &cal_filp->f_pos);
	if (err != 3 * sizeof(s16)) {
		pr_err("%s: Can't read the cal data from file\n", __func__);
		err = -EIO;
	}

	printk("%s: (%d,%d,%d)\n", __func__,
		data->cal_data.x, data->cal_data.y, data->cal_data.z);

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return err;
}
static int lis2hh_acc_enable(struct lis2hh_acc_status *stat)
{
	int err;

	if (!atomic_cmpxchg(&stat->enable, 0, 1)) {
		err = lis2hh_acc_device_power_on(stat);
		if (err < 0) {
			atomic_set(&stat->enable, 0);
			return err;
		}
		err = k2hh_open_calibration(stat);
		if (err < 0 && err != -ENOENT)
			pr_err("%s: k2hh_open_calibration() failed\n",
				__func__);
		stat->polling_ktime = ktime_set(stat->pdata->poll_interval / 1000,
				MS_TO_NS(stat->pdata->poll_interval % 1000));
		hrtimer_start(&stat->hr_timer_poll,
					stat->polling_ktime, HRTIMER_MODE_REL);
	}
	return 0;
}

static int lis2hh_acc_disable(struct lis2hh_acc_status *stat)
{
	if (atomic_cmpxchg(&stat->enable, 1, 0)) {
		cancel_work_sync(&stat->input_poll_work);
		lis2hh_acc_device_power_off(stat);
	}

	return 0;
}


static ssize_t read_single_reg(struct device *dev, char *buf, u8 reg)
{
	ssize_t ret;
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);
	int err;

	u8 data = reg;
	err = lis2hh_acc_i2c_read(stat, &data, 1);
	if (err < 0)
		return err;
	ret = sprintf(buf, "0x%02x\n", data);
	return ret;

}

static int write_reg(struct device *dev, const char *buf, u8 reg,
		u8 mask, int resumeIndex)
{
	int err = -1;
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);
	u8 x[2];
	u8 new_val;
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;

	new_val = ((u8) val & mask);
	x[0] = reg;
	x[1] = new_val;
	err = lis2hh_acc_register_write(stat, x, reg, new_val);
	if (err < 0)
		return err;
	stat->resume_state[resumeIndex] = new_val;
	return err;
}

static ssize_t attr_get_polling_rate(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int val;
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);
	mutex_lock(&stat->lock);
	val = stat->pdata->poll_interval;
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_set_polling_rate(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	int err;
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);
	int64_t nsdelay;
	unsigned long interval_ms;

	if (kstrtoll(buf, 10, &nsdelay))
		return -EINVAL;
	if (!nsdelay)
		return -EINVAL;
	interval_ms = div_s64(nsdelay, 1000000);
	interval_ms = max((unsigned int)interval_ms, stat->pdata->min_interval);
	mutex_lock(&stat->lock);
	stat->pdata->poll_interval = interval_ms;
	err = lis2hh_acc_update_odr(stat, interval_ms);
	if(err >= 0) {
		stat->pdata->poll_interval = interval_ms;
		stat->polling_ktime = ktime_set(stat->pdata->poll_interval / 1000,
				MS_TO_NS(stat->pdata->poll_interval % 1000));
	}
	mutex_unlock(&stat->lock);
	return size;
}

static ssize_t attr_get_range(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	char val;
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);
	char range = 2;
	mutex_lock(&stat->lock);
	val = stat->pdata->fs_range ;
	switch (val) {
	case LIS2HH_ACC_FS_2G:
		range = 2;
		break;
	case LIS2HH_ACC_FS_4G:
		range = 4;
		break;
	case LIS2HH_ACC_FS_8G:
		range = 8;
		break;
	}
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%d\n", range);
}

static ssize_t attr_set_range(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);
	unsigned long val;
	u8 range;
	int err;
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	switch (val) {
	case 2:
		range = LIS2HH_ACC_FS_2G;
		break;
	case 4:
		range = LIS2HH_ACC_FS_4G;
		break;
	case 8:
		range = LIS2HH_ACC_FS_8G;
		break;
	default:
		dev_err(&stat->client->dev, "invalid range request: %lu,"
				" discarded\n", val);
		return -EINVAL;
	}
	mutex_lock(&stat->lock);
	err = lis2hh_acc_update_fs_range(stat, range);
	if (err < 0) {
		mutex_unlock(&stat->lock);
		return err;
	}
	stat->pdata->fs_range = range;
	mutex_unlock(&stat->lock);
	dev_info(&stat->client->dev, "range set to: %lu g\n", val);

	return size;
}

static ssize_t attr_get_enable(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);
	int val = atomic_read(&stat->enable);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_set_enable(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	if (val)
		lis2hh_acc_enable(stat);
	else
		lis2hh_acc_disable(stat);

	return size;
}

static ssize_t attr_set_intconfig1(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev, buf, INT_CFG1, NO_MASK, RES_INT_CFG1);
}

static ssize_t attr_get_intconfig1(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev, buf, INT_CFG1);
}

static ssize_t attr_set_duration1(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev, buf, INT_DUR1, INT1_DURATION_MASK, RES_INT_DUR1);
}

static ssize_t attr_get_duration1(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev, buf, INT_DUR1);
}

static ssize_t attr_set_threshx1(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev, buf, INT_THSX1, INT1_THRESHOLD_MASK, RES_INT_THSX1);
}

static ssize_t attr_get_threshx1(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev, buf, INT_THSX1);
}

static ssize_t attr_set_threshy1(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev, buf, INT_THSY1, INT1_THRESHOLD_MASK, RES_INT_THSY1);
}

static ssize_t attr_get_threshy1(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev, buf, INT_THSY1);
}

static ssize_t attr_set_threshz1(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev, buf, INT_THSZ1, INT1_THRESHOLD_MASK, RES_INT_THSZ1);
}

static ssize_t attr_get_threshz1(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev, buf, INT_THSZ1);
}

static ssize_t attr_get_source1(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return read_single_reg(dev, buf, INT_SRC1);
}


#ifdef DEBUG
/* PAY ATTENTION: These DEBUG functions don't manage resume_state */
static ssize_t attr_reg_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	int rc;
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);
	u8 x[2];
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;
	mutex_lock(&stat->lock);
	x[0] = stat->reg_addr;
	mutex_unlock(&stat->lock);
	x[1] = val;
	rc = lis2hh_acc_i2c_write(stat, x, 1);
	/*TODO: error need to be managed */
	if (rc < 0) {
		pr_err("%s: lis2hh_acc_i2c_write() failed\n", __func__);
		return rc;
	}
	return size;
}

static ssize_t attr_reg_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	ssize_t ret;
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);
	int rc;
	u8 data;

	mutex_lock(&stat->lock);
	data = stat->reg_addr;
	mutex_unlock(&stat->lock);
	rc = lis2hh_acc_i2c_read(stat, &data, 1);
	/*TODO: error need to be managed */
	if (rc < 0) {
		pr_err("%s: lis2hh_acc_i2c_read() failed\n", __func__);
		return rc;
	}
	ret = sprintf(buf, "0x%02x\n", data);
	return ret;
}

static ssize_t attr_addr_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);
	unsigned long val;
	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;
	mutex_lock(&stat->lock);
	stat->reg_addr = val;
	mutex_unlock(&stat->lock);
	return size;
}
#endif

static ssize_t k2hh_accel_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t k2hh_accel_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}
static ssize_t k2hh_fs_read(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct lis2hh_acc_status *data = dev_get_drvdata(dev);
	int xyz[3];
	lis2hh_acc_get_data(data,xyz);


	return sprintf(buf, "%d,%d,%d\n", xyz[0], xyz[1], xyz[2]);
}

static ssize_t k2hh_calibration_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int err;
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);

	err = k2hh_open_calibration(stat);
	if (err < 0)
		pr_err("%s: k2hh_open_calibration() failed\n", __func__);

	if (!stat->cal_data.x && !stat->cal_data.y && !stat->cal_data.z)
		err = -1;

	return sprintf(buf, "%d %d %d %d\n",
		err, stat->cal_data.x, stat->cal_data.y, stat->cal_data.z);
}



static int k2hh_do_calibrate(struct device *dev, bool do_calib)
{
	struct lis2hh_acc_status *acc_data = dev_get_drvdata(dev);
	struct file *cal_filp = NULL;
	int sum[3] = { 0, };
	int err = 0;
	int i;
	mm_segment_t old_fs;
	int xyz[3];
	if (do_calib) {

		acc_data->cal_data.x = 0;
		acc_data->cal_data.y = 0;
		acc_data->cal_data.z = 0;

		for (i = 0; i < CAL_DATA_AMOUNT; i++) {

			err = lis2hh_acc_get_data(acc_data,xyz);
			if (err < 0) {
				pr_err("%s: lis2hh_acc_get_data() "
					"failed in the %dth loop\n",
					__func__, i);
				return err;
			}

			sum[0] += xyz[0];
			sum[1] += xyz[1];
			sum[2] += xyz[2];
		}

		acc_data->cal_data.x = sum[0] / CAL_DATA_AMOUNT;
		acc_data->cal_data.y = sum[1] / CAL_DATA_AMOUNT;
		if (sum[2] >= 0)
			acc_data->cal_data.z = (sum[2] / CAL_DATA_AMOUNT)-16384;
		else
			acc_data->cal_data.z = (sum[2] / CAL_DATA_AMOUNT)+16384;
	} else {
		acc_data->cal_data.x = 0;
		acc_data->cal_data.y = 0;
		acc_data->cal_data.z = 0;
	}

	printk(KERN_INFO "%s: cal data (%d,%d,%d)\n", __func__,
	      acc_data->cal_data.x, acc_data->cal_data.y, acc_data->cal_data.z);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		return err;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&acc_data->cal_data, 3 * sizeof(s16), &cal_filp->f_pos);
	if (err != 3 * sizeof(s16)) {
		pr_err("%s: Can't write the cal data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return err;
}

static ssize_t k2hh_calibration_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{

	bool do_calib;
	int err;

	if (sysfs_streq(buf, "1"))
		do_calib = true;
	else if (sysfs_streq(buf, "0"))
		do_calib = false;
	else {
		pr_debug("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	err = k2hh_do_calibrate(dev, do_calib);
	if (err < 0) {
		pr_err("%s: k2hh_do_calibrate() failed\n", __func__);
		return err;
	}


	return count;
}

static ssize_t attr_get_selftest(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct lis2hh_acc_status *stat = dev_get_drvdata(dev);
	int val, i, en_state = 0;
	ssize_t ret;
	u8 x[8];
	s32 NO_ST[3] = {0, 0, 0};
	s32 ST[3] = {0, 0, 0};

	en_state = atomic_read(&stat->enable);
	lis2hh_acc_disable(stat);

	lis2hh_acc_device_power_on(stat);

	x[0] = CTRL1;
	x[1] = 0x3f;
	lis2hh_acc_i2c_write(stat, x, 1);
	x[0] = CTRL4;
	x[1] = 0x04;
	x[2] = 0x00;
	x[3] = 0x00;
	lis2hh_acc_i2c_write(stat, x, 3);

	mdelay(80);

	x[0] = AXISDATA_REG;
	lis2hh_acc_i2c_read(stat, x, 6);

	for (i = 0; i < 5; i++) {
		while (1) {
			x[0] = 0x27;
			val = lis2hh_acc_i2c_read(stat, x, 1);
			if (val < 0) {
				ret = sprintf(buf, "I2C fail. (%d)\n", val);
				goto ST_EXIT;
			}
			if (x[0] & 0x08)
				break;
		}
		x[0] = AXISDATA_REG;
		lis2hh_acc_i2c_read(stat, x, 6);
		NO_ST[0] += (s16)(x[1] << 8 | x[0]);
		NO_ST[1] += (s16)(x[3] << 8 | x[2]);
		NO_ST[2] += (s16)(x[5] << 8 | x[4]);
	}
	NO_ST[0] /= 5;
	NO_ST[1] /= 5;
	NO_ST[2] /= 5;

	x[0] = CTRL5;
	x[1] = 0x04;
	lis2hh_acc_i2c_write(stat, x, 1);

	mdelay(80);

	x[0] = AXISDATA_REG;
	lis2hh_acc_i2c_read(stat, x, 6);

	for (i = 0; i < 5; i++) {
		while (1) {
			x[0] = 0x27;
			val = lis2hh_acc_i2c_read(stat, x, 1);
			if (val < 0) {
				ret = sprintf(buf, "I2C fail. (%d)\n", val);
				goto ST_EXIT;
			}
			if (x[0] & 0x08)
				break;
		}
		x[0] = AXISDATA_REG;
		lis2hh_acc_i2c_read(stat, x, 6);
		ST[0] += (s16)(x[1] << 8 | x[0]);
		ST[1] += (s16)(x[3] << 8 | x[2]);
		ST[2] += (s16)(x[5] << 8 | x[4]);
	}
	ST[0] /= 5;
	ST[1] /= 5;
	ST[2] /= 5;

	for (val = 1, i = 0; i < 3; i++) {
		ST[i] -= NO_ST[i];
		ST[i] = abs(ST[i]);

		if ((SELF_TEST_2G_MIN_LSB > ST[i]) || (ST[i] > SELF_TEST_2G_MAX_LSB)) {
			pr_info("ST[%d]: Out of range!! (%d)\n", i, ST[i]);
			val = 0;
		}
	}

	if (val)
		ret = sprintf(buf, "1, %d, %d, %d \n", ST[0], ST[1], ST[2]);
	else
		ret = sprintf(buf, "0, %d, %d, %d \n", ST[0], ST[1], ST[2]);

ST_EXIT:
	x[0] = CTRL1;
	x[1] = 0x00;
	lis2hh_acc_i2c_write(stat, x, 1);
	x[0] = CTRL5;
	x[1] = 0x00;
	lis2hh_acc_i2c_write(stat, x, 1);

	lis2hh_acc_device_power_off(stat);

	if (en_state) lis2hh_acc_enable(stat);

	return ret;
}

/*
static ssize_t
k2hh_accel_position_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct lis2hh_acc_status *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->position);
}

static ssize_t
k2hh_accel_position_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t count)
{
	struct lis2hh_acc_status *data = dev_get_drvdata(dev);
	int err = 0;

	err = kstrtoint(buf, 10, &data->position);
	if (err < 0)
		pr_err("%s, kstrtoint failed.", __func__);

	return count;
}*/

	static DEVICE_ATTR(name, 0664, k2hh_accel_name_show, NULL);
	static DEVICE_ATTR(vendor, 0664, k2hh_accel_vendor_show, NULL);
	static DEVICE_ATTR(range, 0664, attr_get_range, attr_set_range);
	static DEVICE_ATTR(int1_config, 0664, attr_get_intconfig1, attr_set_intconfig1);
	static DEVICE_ATTR(int1_duration, 0664, attr_get_duration1, attr_set_duration1);
	static DEVICE_ATTR(int1_thresholdx, 0664, attr_get_threshx1, attr_set_threshx1);
	static DEVICE_ATTR(int1_thresholdy, 0664, attr_get_threshy1, attr_set_threshy1);
	static DEVICE_ATTR(int1_thresholdz, 0664, attr_get_threshz1, attr_set_threshz1);
	static DEVICE_ATTR(int1_source, 0444, attr_get_source1, NULL);
	static DEVICE_ATTR(raw_data, 0664, k2hh_fs_read, NULL);
	static DEVICE_ATTR(calibration, 0664, k2hh_calibration_show, k2hh_calibration_store);
	static DEVICE_ATTR(selftest, 0444, attr_get_selftest, NULL);
#ifdef DEBUG
	static DEVICE_ATTR(reg_value, 0600, attr_reg_get, attr_reg_set);
	static DEVICE_ATTR(reg_addr, 0200, NULL, attr_addr_set);
#endif

static DEVICE_ATTR(enable,S_IRUGO | S_IWUSR | S_IWGRP,attr_get_enable, attr_set_enable);
static DEVICE_ATTR(poll_delay,S_IRUGO | S_IWUSR | S_IWGRP,attr_get_polling_rate, attr_set_polling_rate);

static struct attribute *k2hh_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group k2hh_attribute_group = {
	.attrs = k2hh_attributes
};

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_range,
	&dev_attr_int1_config,
	&dev_attr_int1_duration,
	&dev_attr_int1_thresholdx,
	&dev_attr_int1_thresholdy,
	&dev_attr_int1_thresholdz,
	&dev_attr_int1_source,
	&dev_attr_raw_data,
	&dev_attr_calibration,
	&dev_attr_selftest,
#ifdef DEBUG
	&dev_attr_reg_value,
	&dev_attr_reg_addr,
#endif
	NULL,
};
/*
static int create_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto error;
	return 0;

error:
	for ( ; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	dev_err(dev, "%s:Unable to create interface\n", __func__);
	return -1;
}

static int remove_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, attributes + i);
	return 0;
}
*/
static void lis2hh_acc_input_poll_work_func(struct work_struct *work)
{
	struct lis2hh_acc_status *stat;

	stat = container_of((struct work_struct *) work,
			struct lis2hh_acc_status, input_poll_work);

	lis2hh_acc_report_triple(stat);

	if (atomic_read(&stat->enable))
		hrtimer_start(&stat->hr_timer_poll, stat->polling_ktime, HRTIMER_MODE_REL);
}

enum hrtimer_restart lis2hh_acc_hr_timer_poll_function(struct hrtimer *timer)
{
	struct lis2hh_acc_status *stat;

	stat = container_of((struct hrtimer *)timer,
				struct lis2hh_acc_status, hr_timer_poll);

	queue_work(stat->hr_timer_poll_work_queue, &stat->input_poll_work);
	return HRTIMER_NORESTART;
}
#if 0
int lis2hh_acc_input_open(struct input_dev *input)
{
	struct lis2hh_acc_status *stat = input_get_drvdata(input);
	dev_dbg(&stat->client->dev, "%s\n", __func__);
	return lis2hh_acc_enable(stat);
}

void lis2hh_acc_input_close(struct input_dev *dev)
{
	struct lis2hh_acc_status *stat = input_get_drvdata(dev);
	dev_dbg(&stat->client->dev, "%s\n", __func__);
	lis2hh_acc_disable(stat);
}
#endif

static int lis2hh_acc_validate_pdata(struct lis2hh_acc_status *stat)
{
	/* checks for correctness of minimal polling period */
	stat->pdata->min_interval =
		max((unsigned int)LIS2HH_ACC_MIN_POLL_PERIOD_MS,
						stat->pdata->min_interval);

	stat->pdata->poll_interval = max(stat->pdata->poll_interval,
			stat->pdata->min_interval);

	if (stat->pdata->axis_map_x > 2 ||
		stat->pdata->axis_map_y > 2 ||
		 stat->pdata->axis_map_z > 2) {
		dev_err(&stat->client->dev, "invalid axis_map value "
			"x:%u y:%u z%u\n", stat->pdata->axis_map_x,
					stat->pdata->axis_map_y,
						stat->pdata->axis_map_z);
		return -EINVAL;
	}

	/* Only allow 0 and 1 for negation boolean flag */
	if (stat->pdata->negate_x > 1 || stat->pdata->negate_y > 1
			|| stat->pdata->negate_z > 1) {
		dev_err(&stat->client->dev, "invalid negate value "
			"x:%u y:%u z:%u\n", stat->pdata->negate_x,
				stat->pdata->negate_y, stat->pdata->negate_z);
		return -EINVAL;
	}

	/* Enforce minimum polling interval */
	if (stat->pdata->poll_interval < stat->pdata->min_interval) {
		dev_err(&stat->client->dev, "minimum poll interval violated\n");
		return -EINVAL;
	}

	return 0;
}

static int lis2hh_acc_input_init(struct lis2hh_acc_status *stat)
{
	int err;

	INIT_WORK(&stat->input_poll_work, lis2hh_acc_input_poll_work_func);
	stat->input_dev = input_allocate_device();
	if (!stat->input_dev) {
		err = -ENOMEM;
		dev_err(&stat->client->dev, "input device allocation failed\n");
		goto err0;
	}
#if 0
	stat->input_dev->open = lis2hh_acc_input_open;
	stat->input_dev->close = lis2hh_acc_input_close;
#endif
	//stat->input_dev->name = LIS2HH_ACC_DEV_NAME;
	stat->input_dev->name = "accelerometer_sensor";
	stat->input_dev->id.bustype = BUS_I2C;
	//stat->input_dev->dev.parent = &stat->client->dev;
	input_set_capability(stat->input_dev, EV_REL, REL_X);
	input_set_capability(stat->input_dev, EV_REL, REL_Y);
	input_set_capability(stat->input_dev, EV_REL, REL_Z);

	input_set_drvdata(stat->input_dev, stat);
#if 0
	set_bit(EV_ABS, stat->input_dev->evbit);

	/*	next is used for interruptA sources data if the case */
	set_bit(ABS_MISC, stat->input_dev->absbit);
	/*	next is used for interruptB sources data if the case */
	set_bit(ABS_WHEEL, stat->input_dev->absbit);

	input_set_abs_params(stat->input_dev, ABS_X, G_MIN, G_MAX, FUZZ, FLAT);
	input_set_abs_params(stat->input_dev, ABS_Y, G_MIN, G_MAX, FUZZ, FLAT);
	input_set_abs_params(stat->input_dev, ABS_Z, G_MIN, G_MAX, FUZZ, FLAT);

	/*	next is used for interruptA sources data if the case */
	input_set_abs_params(stat->input_dev, ABS_MISC, INT_MIN, INT_MAX, 0, 0);
	/*	next is used for interruptB sources data if the case */
	input_set_abs_params(stat->input_dev, ABS_WHEEL, INT_MIN,
								INT_MAX, 0, 0);
#endif

	err = input_register_device(stat->input_dev);
	if (err) {
		dev_err(&stat->client->dev,
				"unable to register input device %s\n",
				stat->input_dev->name);
		goto err1;
	}
	err = sensors_create_symlink(&stat->input_dev->dev.kobj, stat->input_dev->name);
	if (err < 0) {
		input_unregister_device(stat->input_dev);
		return err;
	}
	/* Setup sysfs */
	err =sysfs_create_group(&stat->input_dev->dev.kobj,&k2hh_attribute_group);
	if (err < 0)
	 {
		sensors_remove_symlink(&stat->input_dev->dev.kobj,stat->input_dev->name);
		input_unregister_device(stat->input_dev);
		return err;
	}

	return 0;

err1:
	input_free_device(stat->input_dev);
err0:
	return err;
}

static void lis2hh_acc_input_cleanup(struct lis2hh_acc_status *stat)
{
	input_unregister_device(stat->input_dev);
	input_free_device(stat->input_dev);
}
#ifdef CONFIG_OF

/* device tree parsing function */
static int k2hh_parse_dt(struct device *dev,
			struct  lis2hh_acc_platform_data *pdata)
{
	unsigned int poll_interval;
	unsigned int min_interval;
	u32 axis_map_x;
	u32 axis_map_y;
	u32 axis_map_z;
	u32 negate_x;
	u32 negate_y;
	u32 negate_z;
	struct device_node *dNode = dev->of_node;

	if (dNode == NULL)
		return -ENODEV;

	pdata->gpio_int1 = of_get_named_gpio_flags(dNode, "stm,irq_gpio", 0, &pdata->int_flags);
	of_property_read_u32(dNode,"stm,axis_map_x" ,&axis_map_x);
	of_property_read_u32(dNode,"stm,axis_map_y" ,&axis_map_y);
	of_property_read_u32(dNode,"stm,axis_map_z" ,&axis_map_z);
	of_property_read_u32(dNode,"stm,negate_x" ,&negate_x);
	of_property_read_u32(dNode,"stm,negate_y" ,&negate_y);
	of_property_read_u32(dNode,"stm,negate_z" ,&negate_z);
	of_property_read_u32(dNode,"stm,poll_interval" ,&poll_interval);
	of_property_read_u32(dNode,"stm,min_interval" ,&min_interval);

	pdata->fs_range = LIS2HH_ACC_FS_2G;
	pdata->axis_map_x=axis_map_x;
	pdata->axis_map_y = axis_map_y;
	pdata->axis_map_z = axis_map_z;
	pdata->negate_x = negate_x;
	pdata->negate_y = negate_y;
	pdata->negate_z = negate_z;
	pdata->poll_interval = poll_interval;
	pdata->min_interval = min_interval;
	pdata->gpio_int2 = -1;
	if (pdata->gpio_int1 < 0) {
		pr_err("[SENSOR]: %s - get irq_gpio error\n", __func__);
		return -ENODEV;
	}
	printk(KERN_INFO "%s pull-up:%d  \n", __func__, pdata->gpio_int1);

	return 0;
}
#else
static int k2hh_parse_dt(struct device *dev,
struct  lis2hh_acc_platform_data)
{
	return -ENODEV;
}
#endif
static int lis2hh_acc_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{

	struct lis2hh_acc_status *stat;
	struct lis2hh_acc_platform_data *pdata=NULL;
	u32 smbus_func = (I2C_FUNC_SMBUS_BYTE_DATA |
			I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_I2C_BLOCK);

	int err = -1;

	dev_info(&client->dev, "probe start.\n");

	if (client-> dev.of_node) {
		pdata = devm_kzalloc (&client->dev ,
			sizeof (struct lis2hh_acc_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		err = k2hh_parse_dt(&client->dev, pdata);
		pr_info("%s: x=%d  ,  y= %d  , z = %d \n", __func__,
				pdata->axis_map_x, pdata->axis_map_y, pdata->axis_map_z);
		if (err) {
			printk("%s err_free_pdata \n",__func__ );
			goto  err_free_pdata;
		}

	} else {
		pdata = client->dev.platform_data;
		dev_err(&client->dev,
			"%s: K2hh  failed to align dtsi", __func__);
	}
	if (!pdata)
		return -EINVAL;

	stat = kzalloc(sizeof(struct lis2hh_acc_status), GFP_KERNEL);
	if (stat == NULL) {
		dev_err(&client->dev,
				"failed to allocate memory for module data\n");
		err = -ENOMEM;
		printk("%s exit \n",__func__ );
		goto exit;
	}

	stat->use_smbus = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_warn(&client->dev, "client not i2c capable\n");
		if (i2c_check_functionality(client->adapter, smbus_func)) {
			stat->use_smbus = 1;
			dev_warn(&client->dev, "client using SMBUS\n");
		} else {
			err = -ENODEV;
			dev_err(&client->dev, "client nor SMBUS capable\n");
			goto exit_check_functionality_failed;
		}
	}

	mutex_init(&stat->lock);
	mutex_lock(&stat->lock);

	stat->client = client;
	i2c_set_clientdata(client, stat);
	k2hh_power_on(stat,1);
	stat->pdata= pdata;
	stat->hr_timer_poll_work_queue = 0;
	err = lis2hh_acc_validate_pdata(stat);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto err_mutexunlock;
	}

	if (stat->pdata->init) {
		err = stat->pdata->init();
		if (err < 0) {
			dev_err(&client->dev, "init failed: %d\n", err);
			goto err_pdata_init;
		}
	}

	if (stat->pdata->gpio_int1 >= 0) {
		stat->irq1 = gpio_to_irq(stat->pdata->gpio_int1);
		pr_info("%s: %s has set irq1 to irq: %d, "
							"mapped on gpio:%d\n",
			LIS2HH_ACC_DEV_NAME, __func__, stat->irq1,
							stat->pdata->gpio_int1);
	}

	if (stat->pdata->gpio_int2 >= 0) {
		stat->irq2 = gpio_to_irq(stat->pdata->gpio_int2);
		pr_info("%s: %s has set irq2 to irq: %d, "
							"mapped on gpio:%d\n",
			LIS2HH_ACC_DEV_NAME, __func__, stat->irq2,
							stat->pdata->gpio_int2);
	}

	lis2hh_acc_set_init_register_values(stat);
	err = lis2hh_acc_device_power_on(stat);
	if (err < 0) {
		dev_err(&client->dev, "power on failed: %d\n", err);
		goto err_pdata_init;
	}

	atomic_set(&stat->enable, 1);

	err = lis2hh_acc_update_fs_range(stat, stat->pdata->fs_range);
	if (err < 0) {
		dev_err(&client->dev, "update_fs_range failed\n");
		goto  err_power_off;
	}
	err = lis2hh_acc_update_odr(stat, stat->pdata->poll_interval);
	if (err < 0) {
		dev_err(&client->dev, "update_odr failed\n");
		goto  err_power_off;
	}

	stat->hr_timer_poll_work_queue = create_workqueue("lis2hh_acc_hr_timer_poll_wq");
	hrtimer_init(&stat->hr_timer_poll, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	stat->hr_timer_poll.function = &lis2hh_acc_hr_timer_poll_function;
	err=sensors_register(stat->dev, stat, sensor_attrs, MODULE_NAME);
	if (err < 0)
		goto err_power_off;
	 //need to WORK CAK
	err = lis2hh_acc_input_init(stat);
	if (err < 0) {
		dev_err(&client->dev, "input init failed\n");
		goto err_remove_hr_work_queue;
	}

	lis2hh_acc_device_power_off(stat);
	/* As default, do not report information */
	atomic_set(&stat->enable, 0);

	if (stat->pdata->gpio_int1 >= 0) {
		INIT_WORK(&stat->irq1_work, lis2hh_acc_irq1_work_func);
		stat->irq1_work_queue =
			create_singlethread_workqueue("lis2hh_acc_wq1");
		if (!stat->irq1_work_queue) {
			err = -ENOMEM;
			dev_err(&client->dev,
					"cannot create work queue1: %d\n", err);
			goto err_remove_hr_work_queue;
		}
		err = request_irq(stat->irq1, lis2hh_acc_isr1,
			IRQF_TRIGGER_RISING, "lis2hh_acc_irq1", stat);
		if (err < 0) {
			dev_err(&client->dev, "request irq1 failed: %d\n", err);
			goto err_destoyworkqueue1;
		}
		disable_irq_nosync(stat->irq1);
	}

	if (stat->pdata->gpio_int2 >= 0) {
		INIT_WORK(&stat->irq2_work, lis2hh_acc_irq2_work_func);
		stat->irq2_work_queue =
			create_singlethread_workqueue("lis2hh_acc_wq2");
		if (!stat->irq2_work_queue) {
			err = -ENOMEM;
			dev_err(&client->dev,
					"cannot create work queue2: %d\n", err);
			goto err_free_irq1;
		}
		err = request_irq(stat->irq2, lis2hh_acc_isr2,
			IRQF_TRIGGER_RISING, "lis2hh_acc_irq2", stat);
		if (err < 0) {
			dev_err(&client->dev, "request irq2 failed: %d\n", err);
			goto err_destoyworkqueue2;
		}
		disable_irq_nosync(stat->irq2);
	}

	mutex_unlock(&stat->lock);
	dev_info(&client->dev, "%s: probed\n", LIS2HH_ACC_DEV_NAME);

	return 0;

err_destoyworkqueue2:
	if (stat->pdata->gpio_int2 >= 0)
		destroy_workqueue(stat->irq2_work_queue);
err_free_irq1:
	free_irq(stat->irq1, stat);
err_destoyworkqueue1:
	if (stat->pdata->gpio_int1 >= 0)
		destroy_workqueue(stat->irq1_work_queue);
err_remove_hr_work_queue:
	if(stat->hr_timer_poll_work_queue) {
			flush_workqueue(stat->hr_timer_poll_work_queue);
			destroy_workqueue(stat->hr_timer_poll_work_queue);
	}
err_power_off:
	lis2hh_acc_device_power_off(stat);
err_pdata_init:
	if (stat->pdata->exit)
		stat->pdata->exit();
err_mutexunlock:
	mutex_unlock(&stat->lock);
exit_check_functionality_failed:
	pr_err("%s: Driver Init failed\n", LIS2HH_ACC_DEV_NAME);
exit:
	kfree(stat);
err_free_pdata:
	kfree(pdata);

	return err;
}

static int __devexit lis2hh_acc_remove(struct i2c_client *client)
{

	struct lis2hh_acc_status *stat = i2c_get_clientdata(client);

	dev_info(&stat->client->dev, "driver removing\n");

	if (stat->pdata->gpio_int1 >= 0) {
		free_irq(stat->irq1, stat);
		gpio_free(stat->pdata->gpio_int1);
		destroy_workqueue(stat->irq1_work_queue);
	}

	if (stat->pdata->gpio_int2 >= 0) {
		free_irq(stat->irq2, stat);
		gpio_free(stat->pdata->gpio_int2);
		destroy_workqueue(stat->irq2_work_queue);
	}

	lis2hh_acc_disable(stat);
	lis2hh_acc_input_cleanup(stat);

	//remove_sysfs_interfaces(&client->dev);

	if(stat->hr_timer_poll_work_queue) {
			flush_workqueue(stat->hr_timer_poll_work_queue);
			destroy_workqueue(stat->hr_timer_poll_work_queue);
	}

	if (stat->pdata->exit)
		stat->pdata->exit();
	kfree(stat);

	return 0;
}

#ifdef CONFIG_PM
static int lis2hh_acc_resume(struct i2c_client *client)
{
	struct lis2hh_acc_status *stat = i2c_get_clientdata(client);

	if (stat->on_before_suspend)
		return lis2hh_acc_enable(stat);
	return 0;
}

static int lis2hh_acc_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct lis2hh_acc_status *stat = i2c_get_clientdata(client);

	stat->on_before_suspend = atomic_read(&stat->enable);
	return lis2hh_acc_disable(stat);
}
#else
#define lis2hh_acc_suspend	NULL
#define lis2hh_acc_resume	NULL
#endif /* CONFIG_PM */
#ifdef CONFIG_OF
static struct of_device_id k2hh_match_table[] = {
	{ .compatible = "stm,k2hh",},
	{},
};
#else
#define k2hh_match_table NULL
#endif
static const struct i2c_device_id lis2hh_acc_id[]
		= { { LIS2HH_ACC_DEV_NAME, 0 }, { }, };

MODULE_DEVICE_TABLE(i2c, lis2hh_acc_id);


static struct i2c_driver lis2hh_acc_driver = {
	.driver = {
			.owner = THIS_MODULE,
			.name = LIS2HH_ACC_DEV_NAME,
			.of_match_table = k2hh_match_table,
		  },
	.probe = lis2hh_acc_probe,
	.remove = __devexit_p(lis2hh_acc_remove),
	.suspend = lis2hh_acc_suspend,
	.resume = lis2hh_acc_resume,
	.id_table = lis2hh_acc_id,
};

module_i2c_driver(lis2hh_acc_driver)


MODULE_DESCRIPTION("lis2hh accelerometer sysfs driver");
MODULE_AUTHOR("Matteo Dameno, Denis Ciocca, STMicroelectronics");
MODULE_LICENSE("GPL");

