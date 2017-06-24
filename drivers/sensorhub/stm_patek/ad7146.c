/**
 *\mainpage
 * AD7146 MLD Driver
\n
 * @copyright 2013 Analog Devices Inc.
\n
 * Licensed under the GPL Version 3 or later.
 * \date      April-2013
 * \version   Driver 1.3
 * \version   Android ICS 4.0
 * \version   Linux 3.0.15
 */


/**
 * \file ad7146.c
 * This file is the core driver part of AD7146 for Event interface
 * It also has routines for interrupt handling,suspend, resume,
 * initialization routines etc.
 * AD7146 MLD Driver
 * Copyright 2013 Analog Devices Inc.
 * Licensed under the GPL Version 3 or later.
 */

#include <linux/device.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/pm.h>
#include <linux/module.h>
#include <linux/wakelock.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/of_gpio.h>
#include "ad7146.h"
#include "ssp.h"
//#include "sensors_core.h"

#define VENDOR_NAME "ADI"
#define DEVICE_NAME "AD7146"

//#define AD7146_DEBUG
#define AD7146_DEBUG_L2

#define OFFSET_FILE_PATH	"/efs/grip_cal_data"

#define FULL_POWER_MASK (0xFFFC)

#define FIXED_TH		7000
#define CAL_HIGH_MAX	62000
#define CAL_HIGH_MIN	34250
#define CENTER_CDC		30000
#define CAL_LOW_MAX		29750
#define CAL_LOW_MIN		2000
#define LSB_FOR_CAP		5200
#define MAX_OFFSET		126
#define CAL_RET_SUCCESS		(2)
#define CAL_RET_EXIST		(1)
#define CAL_RET_NONE		(0)
#define CAL_RET_FAIL		(3)

#define DO_CALIBRATE	1
#define DONE_CALIBRATE	0

#define LD_POS_AFE_OFFSET(x) ((x & 0x003F) * ((x & 0x0080) ? -1 : 1))
#define LD_NEG_AFE_OFFSET(x) (((x & 0x3F00) >> 8) * ((x & 0x8000) ? 1 : -1))
#define ST_POS_AFE_OFFSET(x) (((x > 63) ? ((x - 63) << 8) | 0x003F : x) | 0x8000)
#define ST_NEG_AFE_OFFSET(x) (((-x > 63) ? (-x - 63) | 0x3F00 : (-x) << 8) | 0x0080)

#ifdef AD7146_DEBUG
#define AD7146_DRIVER_DBG(format, arg...) pr_info("[Grip]:"format,\
						 ## arg)
#else
#define AD7146_DRIVER_DBG(format, arg...) if (0)
#endif

#ifdef AD7146_DEBUG_L2
#define AD7146_DRIVER_DBG_L2(format, arg...) pr_info("[Grip]:"format,\
						    ## arg)
#else
#define AD7146_DRIVER_DBG_L2(format, arg...) if (0)
#endif

static struct ad7146_platform_data ad7146_i2c_platform_data = {
// For init touch mode
	.regs =  {
//Samsung GT - Latest Implementation 1.3 version - Initialization settings, 0x(address)(value)
		0x0080FFFF, 0x00811FEF, 0x00823600, 0x00832626,
		0x00840258, 0x00850258, 0x0086013A, 0x0087013A,
		0x0000C000, 0x00010000, 0x0002004B, 0x000301FF,
		0x0004FFFF, 0x0000C002, 0x00450D01, 0x00050000,
		0x00060000, 0x00070000,
	},
// Normal touch mode
	.normal_regs = {
//Samsung GT - Latest Implementation 1.3 version - Normal State settings
		0x0000C002, 0x00010001,
	},
	.fixed_th_full = 40000, // 0 ~ 65535
};

/**
Look up table for the Sensitivity values of the AD7146
*/
static int SS_LUT[16] = {
	2500, 2973, 3440, 3908,
	4379, 4847, 5315, 5783,
	6251, 6722, 7190, 7658,
	8128, 8596, 9064, 9532
};

/**
 *    The global mutex for the locking of the ISR.
 */
DEFINE_MUTEX(interrupt_thread_mutex);
/*
 This elaborates the sysfs attributes used in the driver
 */
/*--------------------------------------------------------------*/
static ssize_t show_dumpregs(struct device *dev,
			      struct device_attribute *attr, char *buf);
static struct device_attribute dev_attr_sensor_dump =
	__ATTR(status, S_IRUSR | S_IRGRP,
	show_dumpregs, NULL);
/*--------------------------------------------------------------*/
static ssize_t store_enable(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count);
static ssize_t show_enable(struct device *dev,
			      struct device_attribute *attr, char *buf);

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	show_enable, store_enable);
/*--------------------------------------------------------------*/
/**
SYSFS attributes list
*/
static struct attribute *ad7146_sysfs_entries[] = {
	&dev_attr_enable.attr,
	NULL
};
/**
SYSFS attribute Group
*/
static struct attribute_group ad7146_attr_group = {
	.name = NULL,
	.attrs = ad7146_sysfs_entries,
};

/**
  Writes to the Device register through i2C.
  Used to Write the data to the I2C client's Register through the i2c protocol

  @param data The data to be written
  @param reg The register address
  @param dev The Device Structure
  @return 0 on success

  @see ad7146_i2c_read
 */
static int ad7146_i2c_write(struct device *dev, unsigned short reg,
		unsigned short data)
{
	struct i2c_client *client = to_i2c_client(dev);
	int ret;
	u8 *_reg = (u8 *)&reg;
	u8 *_data = (u8 *)&data;

	u8 tx[4] = {
		_reg[1],
		_reg[0],
		_data[1],
		_data[0]
	};
	ret = i2c_master_send(client, tx, 4);
	if (ret < 0)
		dev_err(&client->dev, "I2C write error (%d)\n", ret);
	return ret;
}
/**
  Reads data from the Device register through i2C.
  This is used to read the data from the AD7146 I2C client

  @param dev The Device Structure (Standard linux call)
  @param reg The register address to be read.
  @param data The data Read from the Given address.
  @return The number of bytes transfered as an integer

  @see ad7146_i2c_write
 */

static int ad7146_i2c_read(struct device *dev, unsigned short reg,
		unsigned short *data)
{
	struct i2c_client *client = to_i2c_client(dev);
	int ret;
	u8 *_reg = (u8 *)&reg;
	u8 *_data = (u8 *)data;

	u8 tx[2] = {
		_reg[1],
		_reg[0]
	};
	u8 rx[2];

	ret = i2c_master_send(client, tx, 2);
	if (ret >= 0)
		ret = i2c_master_recv(client, rx, 2);

	if (unlikely(ret < 0)) {
		dev_err(&client->dev, "I2C read error (%d)\n", ret);
	} else {
		_data[0] = rx[1];
		_data[1] = rx[0];
	}

	return ret;
}

static int sensorshutdownmode(struct ad7146_chip *ad7146)
{
	unsigned short data = 0;

	mutex_lock(&interrupt_thread_mutex);

	ad7146->read(ad7146->dev, AD7146_PWR_CTRL, &data);
	data = (data | 0x03);
	ad7146->write(ad7146->dev, AD7146_PWR_CTRL, data);

	mutex_unlock(&interrupt_thread_mutex);
	return 0;
}

static void ad7146_force_cal(struct ad7146_chip *ad7146, int cal_time)
{
	unsigned short data;

	ad7146->write(ad7146->dev, STG_LOW_INT_EN_REG, DISABLE_INT);
	ad7146->write(ad7146->dev, STG_HIGH_INT_EN_REG, DISABLE_INT);

	ad7146->read(ad7146->dev, AD7146_AMB_COMP_CTRL0_REG, &data);
	data = data | AD7146_FORCED_CAL_MASK;
	ad7146->write(ad7146->dev, AD7146_AMB_COMP_CTRL0_REG, data);

	msleep(cal_time);

	ad7146->read(ad7146->dev, CDC_RESULT_S0_REG, &data);

	ad7146->write(ad7146->dev, STG_HIGH_INT_EN_REG, ENABLE_STG0);
	ad7146->write(ad7146->dev, STG_LOW_INT_EN_REG, ENABLE_STG0);
}

/**
  This function is used to indicate Full Grip state by event
  @param ad7146 The AD7146 chip structure pointer
  @return void

 */
static inline void indicatefullgripstate(struct ad7146_chip *ad7146)
{
	AD7146_DRIVER_DBG("indicatefullgripstate()\n");
	if ((ad7146->eventcheck == 1) && (ad7146->onoff_flags == 1)) {
		input_report_rel(ad7146->input, REL_MISC, EVENT_FULL_GRIP + 1);
		input_sync(ad7146->input);
	}
}

/**
  This function is used to indicate No Grip state by event
  @param ad7146 The AD7146 chip structure pointer
  @return void

 */
static inline void indicatenormalstate(struct ad7146_chip *ad7146)
{
	AD7146_DRIVER_DBG("indicatenormalstate()\n");
	if ((ad7146->eventcheck == 1) && (ad7146->onoff_flags == 1)) {
		input_report_rel(ad7146->input, REL_MISC, EVENT_NO_GRIP + 1);
		input_sync(ad7146->input);
	}
}

/**
This Function is used to determine the No Grip and
 enter to the NORMAL mode of operation
  @param ad7146 The Chip Structure.
  @return void - Nothing returned
 */

static void initnormalgrip(struct ad7146_chip *ad7146)
{
	unsigned int lcnt = 0;

	indicatenormalstate(ad7146);
	ad7146->write(ad7146->dev, STG_LOW_INT_EN_REG,
			DISABLE_INT);
	ad7146->write(ad7146->dev, STG_HIGH_INT_EN_REG,
			DISABLE_INT);

	for (lcnt = 0; lcnt < (sizeof(ad7146_i2c_platform_data.normal_regs)/sizeof(int));
	     lcnt++) {
		unsigned short addr;
		unsigned short value;
		addr = (unsigned short)((ad7146_i2c_platform_data.normal_regs[lcnt] &
				0xffff0000) >> 16);
		value = (unsigned short)(ad7146_i2c_platform_data.normal_regs[lcnt] &
				0x0000ffff);

		ad7146->write(ad7146->dev, addr, value);
	}

	ad7146_force_cal(ad7146, SLEEP_TIME_TO_CALI_INT);

	ad7146->pw_on_grip_status = DRIVER_STATE_NORMAL;
	AD7146_DRIVER_DBG("Forced Calibration in func %s\n", __func__);

	ad7146->write(ad7146->dev, STG_LOW_INT_EN_REG, ENABLE_STG0);
	ad7146->write(ad7146->dev, STG_HIGH_INT_EN_REG, ENABLE_STG0);
}

/**
This Function is used in the POWER_ON GRIP detection to
 determine the Full Grip status.
@param ad7146 The Chip Structure.
@return void - Nothing returned
*/

static void initfullgrip(struct ad7146_chip *ad7146)
{
	unsigned short sf_amb = 0;
	unsigned short cdc_data = 0;
	unsigned short theshold_low;

	indicatefullgripstate(ad7146);

	if (ad7146->cal_flags == CAL_RET_NONE)
		theshold_low = ad7146_i2c_platform_data.fixed_th_full;
	else
		theshold_low = ad7146_i2c_platform_data.cal_fixed_th_full;

	ad7146->write(ad7146->dev, AD7146_STG_CAL_EN_REG, ENABLE_STG0);
	ad7146->write(ad7146->dev, STG_0_LOW_THRESHOLD, theshold_low);
	/* Change the Ambient to set the theshold as a fixed value
	   DONOT CHANGE THE FOLLOWING LINE*/
	ad7146->write(ad7146->dev, DRIVER_STG0_SF_AMBIENT , FULL_SCALE_VALUE);
	ad7146->pw_on_grip_status = DRIVER_STATE_FULL_GRIP;

	ad7146->read(ad7146->dev, CDC_RESULT_S0_REG, &cdc_data);
	ad7146->read(ad7146->dev, STG_0_LOW_THRESHOLD, &theshold_low);
	ad7146->read(ad7146->dev, DRIVER_STG0_SF_AMBIENT , &sf_amb);
	AD7146_DRIVER_DBG_L2(" %s, AMB %d CurrentCDC %d theshold_low %d\n", __func__,
			     sf_amb, cdc_data, theshold_low);

	ad7146->write(ad7146->dev, STG_LOW_INT_EN_REG, ENABLE_STG0);
}

/**
  This function is used to identify and report the first grip status.
  @param ad7146 The AD7146 chip structure pointer
  @return void - Nothing Returned
*/

static int sensorgrip(struct ad7146_chip *ad7146)
{
	unsigned short conv0_avg = 0;
	unsigned short threshold;

	mutex_lock(&interrupt_thread_mutex);
	ad7146->read(ad7146->dev, CDC_RESULT_S0_REG, &conv0_avg);
	AD7146_DRIVER_DBG("conv0_avg = %5d\n", conv0_avg);

	if (conv0_avg < OVER_FLOW_SCALE_VALUE)
		conv0_avg = FULL_SCALE_VALUE;

	if (ad7146->cal_flags == CAL_RET_NONE)
		threshold = ad7146_i2c_platform_data.fixed_th_full;
	else
		threshold = ad7146_i2c_platform_data.cal_fixed_th_full;

	if (conv0_avg < threshold) {
		initnormalgrip(ad7146);
	} else {
		initfullgrip(ad7146);
	}

	mutex_unlock(&interrupt_thread_mutex) ;
	return 0;
}

/**
  This is to configure the device with the register set defined in platform file.
  Finally calibration is done and status registers will be cleared.
 * @param  ad7146 The Device structure
 * @return void  Nothing Returned
 */

static int ad7146_hw_init(struct ad7146_chip *ad7146)
{
	int lcnt = 0;

	mutex_lock(&interrupt_thread_mutex) ;
	/** configuration CDC and interrupts */
	for (lcnt = 0; lcnt < (sizeof(ad7146_i2c_platform_data.regs)/sizeof(int)); lcnt++) {
		unsigned short addr;
		unsigned short value;
		addr = (unsigned short)((ad7146_i2c_platform_data.regs[lcnt] &
					0xffff0000) >> 16);
		value = (unsigned short)(ad7146_i2c_platform_data.regs[lcnt] &
				0x0000ffff);
		if (addr == DRIVER_STG0_AFEOFFSET) {
			if (ad7146->cal_flags != CAL_RET_NONE)
				value = ad7146_i2c_platform_data.cal_offset;
			AD7146_DRIVER_DBG_L2(" %s, Addr %x Val %x\n", __func__, addr, value);
		}
		if (addr == AD7146_AMB_COMP_CTRL0_REG)
			value = value & 0xBFFF;

		ad7146->write(ad7146->dev, addr, value);
	}

	msleep(SLEEP_TIME_TO_CALI_INT);

	ad7146->prev_state_value = -1;
	ad7146->state_value = EVENT_NO_GRIP;
	ad7146->prevhigh = 0;

	AD7146_DRIVER_DBG("Force calibration done\n");
	mutex_unlock(&interrupt_thread_mutex);

	return 0;
}

/**
This Function is used for the creating a Register Dump of the registers of the AD7146.

  @param dev The Device Id and Information structure(Linux Standard argument)
  @param attr standard Linux Device attributes to the AD7146
  @param buf The buffer to store the data to be written
  @param count The count of bytes to be transfered to the Device

  \note This is evoked upon an echo request in /sys/../<Device> region.
  \note This also prints the results in the console for the user.
  @return count of data written
 */
static ssize_t show_dumpregs(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct ad7146_chip *ad7146 = dev_get_drvdata(dev);
	unsigned short u16temp;
	unsigned int u32_lpcnt = 0;

	mutex_lock(&interrupt_thread_mutex);
	pr_info("[Grip]: Bank 1 register\n");
	for (u32_lpcnt = 0; u32_lpcnt < 0x16; u32_lpcnt++) {
		ad7146->read(ad7146->dev, (unsigned short)u32_lpcnt, &u16temp);
		pr_info("[Grip]: Reg 0X%x val 0x%x\n",
		       u32_lpcnt, u16temp);
	}

	ad7146->read(ad7146->dev, (unsigned short)0x042, &u16temp);
	pr_info("[Grip]: Reg 0X0042 val 0x%x\n", u16temp);

	ad7146->read(ad7146->dev, (unsigned short)0x0045, &u16temp);
	pr_info("[Grip]: Reg 0X0045 val 0x%x\n", u16temp);

	pr_info("[Grip]: Bank 2 register - Config\n");
	for (u32_lpcnt = 0x080; u32_lpcnt < 0x090; u32_lpcnt++) {
		ad7146->read(ad7146->dev, (unsigned short)u32_lpcnt, &u16temp);
		pr_info("[Grip]: Reg 0X%x val 0x%x\n",
		       u32_lpcnt, u16temp);
	}

	pr_info("[Grip]: Bank 3 register - Results\n");
	for (u32_lpcnt = 0x0E0; u32_lpcnt < 0x128; u32_lpcnt++) {
		ad7146->read(ad7146->dev, (unsigned short)u32_lpcnt, &u16temp);
		pr_info("[Grip]: Reg 0X%x val 0x%x\n",
		       u32_lpcnt, u16temp);
	}

	mutex_unlock(&interrupt_thread_mutex);

	return sprintf(buf, "0\n");
}

static int ad7146_open_calibration(struct ad7146_chip *ad7146)
{
	struct file *offset_filp = NULL;
	unsigned short st_file[2] = {0};
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	offset_filp = filp_open(OFFSET_FILE_PATH, O_CREAT | O_RDONLY | O_SYNC, 0666);
	if (IS_ERR(offset_filp)) {
		pr_err("%s: no offset file\n", __func__);
		err = PTR_ERR(offset_filp);
		if (err != -ENOENT)
			pr_err("%s: Can't open cancelation file\n", __func__);
		set_fs(old_fs);
		return err;
	}

	err = offset_filp->f_op->read(offset_filp,
		(char *)&st_file, sizeof(unsigned short) * 2, &offset_filp->f_pos);
	if (err != sizeof(unsigned short) * 2) {
		pr_err("%s: Can't write the offset data to file\n", __func__);
		err = -EIO;
	}

	if ((st_file[0] != 0) && (st_file[1] != 0)) {
		ad7146->cal_flags = CAL_RET_EXIST;
		ad7146_i2c_platform_data.cal_offset = st_file[0];
		ad7146_i2c_platform_data.cal_fixed_th_full = st_file[1];
	} else {
		ad7146->cal_flags = CAL_RET_NONE;
		ad7146_i2c_platform_data.cal_offset = 0;
		ad7146_i2c_platform_data.cal_fixed_th_full = 0;
	}

	pr_info("[Grip]: %s, %d, %4x,%d\n", __func__, ad7146->cal_flags, st_file[0], st_file[1]);

	filp_close(offset_filp, current->files);
	set_fs(old_fs);

	return err;
}

/**
This Function is used to enable or to disable the device the Sysfs attribute is
given as "enable" writing a '0' Disables the device.
While writing a '1' enables the device.

  @param dev The Device Id and Information structure(Linux Standard argument)
  @param attr standard Linux Device attributes to the AD7146.
  @param buf The buffer to store the data to be written.
  @param count The count of bytes to be transfered to the Device.

  \note This is evoked upon an echo request in /sys/../<Device> region.
  \note This also prints the results in the console for the user.

@return count of data written.
*/
static ssize_t store_enable(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct ad7146_chip *ad7146 = dev_get_drvdata(dev);
	int val, err;

	err = kstrtoint(buf, 10, &val);

	if (err < 0) {
		pr_err("[Grip]: %s, kstrtoint failed\n", __func__);
	} else {
		pr_info("[Grip]: %s: enable %d val %d\n", __func__, ad7146->eventcheck, val);
		if ((val == 1) || (val == 0)) {
			if (ad7146->eventcheck != ((unsigned short) val)) {
				ad7146->eventcheck = (unsigned short) val;

				if (ad7146->eventcheck == 1) {
					ad7146_open_calibration(ad7146);
					ad7146_hw_init(ad7146);

					enable_irq(ad7146->irq);
					enable_irq_wake(ad7146->irq);

					sensorgrip(ad7146);
				} else {
					disable_irq_wake(ad7146->irq);
					disable_irq(ad7146->irq);

					ad7146->write(ad7146->dev, STG_HIGH_INT_EN_REG,
						      DISABLE_INT);
					ad7146->write(ad7146->dev, STG_LOW_INT_EN_REG,
						      DISABLE_INT);
					sensorshutdownmode(ad7146);
				}
			}
		}
	}
	return count;
}

/**
This Function is used to show the enabled status of the device.
Status '1' signifies the device is ENABLED,
while the status '0' signifies a DISABLED device.

  @param dev The Device Id and Information structure(Linux Standard argument)
  @param attr standard Linux Device attributes to the AD7146.
  @param buf The buffer to store the data to be written.

  \note This is evoked upon an cat request in /sys/../<Device> region.
  \note This also prints the results in the console for the user.

  @return The count of data written.
*/
static ssize_t show_enable(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct ad7146_chip  *ad7146 = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", ad7146->eventcheck);
}

/**
  This callback_tmrfn is the callback function from MLD state machine
  for the event intimation of the AD7146 device.
  Here the current & previous grip state are compared & the event is sent
  accordingly.

  @param  data unsigned long data
  @return 0 on success

 */
static inline void callback_tmrfn(unsigned long data)
{
	struct ad7146_chip *ad7146 = (struct ad7146_chip *)data;
	short state_value = ad7146->state_value;
	short prev_state_value = ad7146->prev_state_value;

	if (state_value != prev_state_value) {
		ad7146->prev_state_value = ad7146->state_value;
		if ((ad7146->eventcheck == 1) && (ad7146->onoff_flags == 1)) {
			input_report_rel(ad7146->input, REL_MISC,
				    (unsigned int)state_value + 1);
			input_sync(ad7146->input);
			pr_info("[Grip]: %s: state_value = %d\n", __func__, state_value);
		} else if ((ad7146->eventcheck == 1) && (ad7146->onoff_flags == 0)) {
			pr_info("[Grip]: %s, event locked by flags, %d\n", __func__, ad7146->onoff_flags);
		}
	}
}

/**
  This MLD State Machine is used to determine the current grip state

  @param ad7146 The AD7146 chip structure
  @return 0 on success

 */
static int sldstateMachine(struct ad7146_chip *ad7146)
{
	unsigned short high = (ad7146->high_status & 3);
	unsigned short prevhigh = (ad7146->prevhigh & 3);
	short state_value = ad7146->state_value;

	if ((high == 0) && (prevhigh == 0)) {
		state_value = EVENT_NO_GRIP;
		high = 0;
		ad7146->high_status = high;
		ad7146->state_value = state_value;
		return 0;
	}

	if ((high == 1) && (prevhigh == 0)) {
		AD7146_DRIVER_DBG("State = 1e\n");
		state_value = EVENT_FULL_GRIP;
	}

	if ((high == 0) && (prevhigh == 1)) {
		AD7146_DRIVER_DBG("State = 0x\n");
		state_value = EVENT_NO_GRIP;
	}

	prevhigh = high;
	ad7146->high_status = high;
	ad7146->prevhigh    = prevhigh;
	ad7146->state_value = state_value;
	callback_tmrfn((unsigned long)ad7146);
	return 0;
}


/**
 * \fn static int ad7146_hw_detect(struct ad7146_chip *ad7146)
 * This Routine reads the Device ID to confirm the existance
 * of the Device in the System.

 @param  ad7146 The Device structure
 @return 0 on Successful detection of the device,-ENODEV on err.
 */

static int ad7146_hw_detect(struct ad7146_chip *ad7146)
{
	unsigned short data;

	ad7146->read(ad7146->dev, AD7146_PARTID_REG, &data);
	switch (data & 0xFFF0) {
	case AD7146_PARTID:
		ad7146->product = AD7146_PRODUCT_ID;
		ad7146->version = data & 0xF;
		dev_info(ad7146->dev, "[Grip]: found AD7146 , rev:%d\n",
			 ad7146->version);
		return 0;

	default:
		dev_err(ad7146->dev,
			"[Grip]: ad7146 Not Found,ID %04x\n", data);
		return -ENODEV;
	}
}

/**
  This function is used check the stage & accordingly calculate the
  hysteresis compensation required.

  @param ad7146 The AD7146 chip structure pointer
  @return void - Nothing returned
*/
static void ad7146_hysteresis_comp(struct ad7146_chip *ad7146)
{
	unsigned short u16_high_threshold = 0;
	unsigned short u16_sf_ambient = 0;
	unsigned int result = 0;

	if (ad7146->high_status > (short)ad7146->prevhigh) {
		if ((ad7146->high_status & 1) && (ad7146->state_value != EVENT_FULL_GRIP)) {
			ad7146->read(ad7146->dev, DRIVER_STG0_HIGH_THRESHOLD,
					&u16_high_threshold);
			ad7146->read(ad7146->dev, DRIVER_STG0_SF_AMBIENT,
					&u16_sf_ambient);
			result = HYS(u16_sf_ambient, u16_high_threshold);
			ad7146->write(ad7146->dev, DRIVER_STG0_HIGH_THRESHOLD,
					(unsigned short)result);
			AD7146_DRIVER_DBG_L2(" %s, N STG0 HT 0x%d->0x%d\n", __func__,
					     u16_high_threshold, result);
		}

	} else if (ad7146->high_status < ad7146->prevhigh) {
		if ((!(ad7146->high_status & 1)) &&
		    (ad7146->prevhigh & 1)) {
			ad7146->read(ad7146->dev, DRIVER_STG0_HIGH_THRESHOLD,
					&u16_high_threshold);
			ad7146->read(ad7146->dev, DRIVER_STG0_SF_AMBIENT,
					&u16_sf_ambient);
			result = HYS_POS(u16_sf_ambient, u16_high_threshold);
			ad7146->write(ad7146->dev, DRIVER_STG0_HIGH_THRESHOLD,
					(unsigned short)result);
			AD7146_DRIVER_DBG_L2(" %s, P STG0 HT 0x%d->0x%d\n", __func__,
					     u16_high_threshold, result);
		}
	}
}
/**
  IRQ Handler -- Handles the Grip & Normal mode of proximity detection
  @param handle The data of the AD7146 Device
  @param irq The Interrupt Request queue to be assigned for the device.

  @return IRQ_HANDLED
 */
static irqreturn_t ad7146_isr(int irq, void *handle)
{
	struct ad7146_chip *ad7146 = handle;
	mutex_lock(&interrupt_thread_mutex) ;

	wake_lock_timeout(&ad7146->grip_wake_lock, 3 * HZ);

	if (!work_pending(&ad7146->work)) {
		schedule_work(&ad7146->work);
	} else {
	/*Cleared the interrupt for future intterupts to occur*/
		ad7146->read(ad7146->dev, STG_LOW_INT_STA_REG,
				&ad7146->low_status);
		ad7146->read(ad7146->dev, STG_HIGH_INT_STA_REG,
				&ad7146->high_status);
	}

	mutex_unlock(&interrupt_thread_mutex);
	return IRQ_HANDLED;
}

/**
  Interrupt work Handler -- Handles the Grip & Normal mode of proximity
  detection from the ISR
  @param work The work structure for the AD7146 chip

  @return void Nothing returned
 */
static void ad7146_interrupt_thread(struct work_struct *work)
{
	struct ad7146_chip *ad7146 =  container_of(work,
						   struct ad7146_chip, work);
	unsigned short data = 0;
	unsigned short threshold;

	mutex_lock(&interrupt_thread_mutex);

	ad7146->read(ad7146->dev, STG_HIGH_INT_STA_REG,
		     &ad7146->high_status);
	ad7146->read(ad7146->dev, STG_LOW_INT_STA_REG,
		     &ad7146->low_status);
	AD7146_DRIVER_DBG_L2(" %s, HS%x LS%x\n", __func__,
		ad7146->high_status, ad7146->low_status);

	if (ad7146->cal_flags == CAL_RET_NONE)
		threshold = ad7146_i2c_platform_data.fixed_th_full;
	else
		threshold = ad7146_i2c_platform_data.cal_fixed_th_full;

	ad7146->read(ad7146->dev, CDC_RESULT_S0_REG, &data);
	if (data < OVER_FLOW_SCALE_VALUE)
		data = FULL_SCALE_VALUE;
	pr_info("[Grip]: %s, data(%d)\n", __func__, data);

	if (ad7146->pw_on_grip_status == DRIVER_STATE_FULL_GRIP) { // init touch mode
		pr_info("[Grip]: %s, FULL_GRIP\n", __func__);
		if ((ad7146->low_status & 1) == 1) {
			if ((OVER_FLOW_SCALE_VALUE < data) &&
				(data <= (threshold))) {
				initnormalgrip(ad7146);
			} else {
				ad7146->write(ad7146->dev, DRIVER_STG0_SF_AMBIENT , FULL_SCALE_VALUE);
				ad7146->write(ad7146->dev, STG_0_LOW_THRESHOLD, threshold);
				pr_info("[Grip]: %s, reset Full Grip\n", __func__);
			}
		} else {
			if (data < threshold)
				initnormalgrip(ad7146);
		}
	} else { // normal mode
		pr_info("[Grip]: %s, NORMAL\n", __func__);
		if (ad7146->low_status != 0) {
			if (OVER_FLOW_SCALE_VALUE < data && data < FULL_SCALE_VALUE) {
				ad7146_force_cal(ad7146, SLEEP_TIME_TO_CALI_INT);
				AD7146_DRIVER_DBG("FCalib(%d) in Low INT\n", data);
			}
		} else {
			ad7146_hysteresis_comp(ad7146);
			sldstateMachine(ad7146);
		}
	}

	mutex_unlock(&interrupt_thread_mutex);
}

static int offset_write(struct ad7146_chip *ad7146, unsigned short data)
{
	ad7146->write(ad7146->dev, DRIVER_STG0_AFEOFFSET, data);
	msleep(SLEEP_TIME_TO_CALI_INT);

	ad7146->read(ad7146->dev, CDC_RESULT_S0_REG, &data);
	ad7146->read(ad7146->dev, DRIVER_STG0_AFEOFFSET, &data);
	return 0;
}

static int ad7146_offset_check(struct ad7146_chip *ad7146)
{
	int err = 0;
	short cal_offset;
	unsigned short data, offset;

	offset = (unsigned short)(ad7146_i2c_platform_data.regs[STAGE_AFE_OFFSET] &
			0x0000ffff);
	offset_write(ad7146, offset);

	ad7146->read(ad7146->dev, CDC_RESULT_S0_REG, &data);
	ad7146->read(ad7146->dev, DRIVER_STG0_AFEOFFSET, &offset);

	if (data < CAL_LOW_MIN || CAL_HIGH_MAX < data) {
		cal_offset = LD_POS_AFE_OFFSET(offset) + LD_NEG_AFE_OFFSET(offset);
		cal_offset += 8;
		pr_info("[Grip]: %s,(%d)\n", __func__, cal_offset);
		if (0 <= cal_offset && cal_offset <= MAX_OFFSET) {
			offset = ST_POS_AFE_OFFSET(cal_offset);
		} else if (-MAX_OFFSET <= cal_offset && cal_offset < 0) {
			offset = ST_NEG_AFE_OFFSET(cal_offset);
		} else {
			err = -1;
			pr_err("[Grip]: %s, Cal offset ERROR(%d)\n", __func__, cal_offset);
		}
		offset_write(ad7146, offset);
	} else {
		pr_info("[Grip]: %s, no change offset(%d)\n", __func__, data);
		return 0;
	}

	ad7146->read(ad7146->dev, CDC_RESULT_S0_REG, &data);
	ad7146->read(ad7146->dev, DRIVER_STG0_AFEOFFSET, &offset);

	if (data < CAL_LOW_MIN || CAL_HIGH_MAX < data) {
		cal_offset = LD_POS_AFE_OFFSET(offset) + LD_NEG_AFE_OFFSET(offset);
		if (err < 0)
			cal_offset -= 8;
		else
			cal_offset -= 16;
		pr_info("[Grip]: %s,(%d)\n", __func__, cal_offset);
		if (0 <= cal_offset && cal_offset <= MAX_OFFSET) {
			offset = ST_POS_AFE_OFFSET(cal_offset);
		} else if (-MAX_OFFSET <= cal_offset && cal_offset < 0) {
			offset = ST_NEG_AFE_OFFSET(cal_offset);
		} else {
			err = -1;
			pr_err("[Grip]: %s, Cal offset ERROR(%d)\n", __func__, cal_offset);
		}
		offset_write(ad7146, offset);
	} else {
		pr_info("[Grip]: %s, no change offset(%d)\n", __func__, data);
		return 0;
	}

	return err;
}

static int ad7146_calibration(struct ad7146_chip *ad7146, bool cal_flag)
{
	struct file *offset_filp = NULL;
	mm_segment_t old_fs;
	short cal_offset, count = 4;
	unsigned short data, offset, temp_offset, temp_fixed_th;
	unsigned short st_file[2] = {0};
	int err = 0;

	ad7146->write(ad7146->dev, STG_LOW_INT_EN_REG, DISABLE_INT);
	ad7146->write(ad7146->dev, STG_HIGH_INT_EN_REG, DISABLE_INT);
	ad7146->cal_flags = CAL_RET_FAIL;

	temp_offset = ad7146_i2c_platform_data.cal_offset;
	temp_fixed_th = ad7146_i2c_platform_data.cal_fixed_th_full;

	if (cal_flag) {
		ad7146->read(ad7146->dev, AD7146_PWR_CTRL, &data);
		data &= FULL_POWER_MASK;
		ad7146->write(ad7146->dev, AD7146_PWR_CTRL, data);
		msleep(SLEEP_TIME_TO_LOW_POWER + SLEEP_TIME_TO_CALI_INT);

		ad7146_offset_check(ad7146);

		while (count >= 0) {
			ad7146->read(ad7146->dev, CDC_RESULT_S0_REG, &data);
			ad7146->read(ad7146->dev, DRIVER_STG0_AFEOFFSET, &offset);

			cal_offset = LD_POS_AFE_OFFSET(offset) + LD_NEG_AFE_OFFSET(offset);
			pr_info("[Grip]: %s, %d, %x(%d), %d\n", __func__,
				data, offset, cal_offset, count);

			if ((CAL_LOW_MAX < data && data <= CAL_HIGH_MIN) &&
				(-MAX_OFFSET < cal_offset && cal_offset <= MAX_OFFSET)) {
				st_file[0] = offset;
				st_file[1] = data + FIXED_TH;

				ad7146_i2c_platform_data.cal_fixed_th_full = data + FIXED_TH;
				ad7146_i2c_platform_data.cal_offset = offset;
				ad7146->cal_flags = CAL_RET_SUCCESS;
				pr_info("[Grip]: %s, cal_fixed_th_full(%u), cal_offset(%u), cal_flags(%d)\n",
					__func__, ad7146_i2c_platform_data.cal_fixed_th_full,
					ad7146_i2c_platform_data.cal_offset, ad7146->cal_flags);

				break;
			} else if (CAL_LOW_MIN < data && data <= CAL_HIGH_MAX) {
				if (data > CENTER_CDC)
					cal_offset = cal_offset - count;
				else
					cal_offset = cal_offset + count;

				if (0 <= cal_offset && cal_offset <= MAX_OFFSET) {
					offset = ST_POS_AFE_OFFSET(cal_offset);
				} else if (-MAX_OFFSET <= cal_offset && cal_offset < 0) {
					offset = ST_NEG_AFE_OFFSET(cal_offset);
				} else {
					err = -1;
					pr_err("%s, offset ERROR(%d)\n", __func__, data);
					break;
				}
				offset_write(ad7146, offset);
			} else {
				err = -1;
				pr_err("%s, raw data ERROR(%d)\n", __func__, data);
				break;
			}
			if (count > 0)
				count /= 2;
			else
				count -= 1;
		}
	} else {
		st_file[0] = 0;
		st_file[1] = 0;

		ad7146->cal_flags = CAL_RET_NONE;
		ad7146_i2c_platform_data.cal_fixed_th_full = 0;
		ad7146_i2c_platform_data.cal_offset = 0;
		offset = (unsigned short)(ad7146_i2c_platform_data.regs[STAGE_AFE_OFFSET] & 0x0000ffff);
	}

	if (ad7146->cal_flags == CAL_RET_FAIL) {
		pr_err("[Grip]: %s, cal fail\n", __func__);
		if (ad7146_i2c_platform_data.cal_fixed_th_full != 0) {
			offset = ad7146_i2c_platform_data.cal_offset;
			ad7146->cal_flags = CAL_RET_EXIST;
		} else {
			offset = (unsigned short)(ad7146_i2c_platform_data.regs[STAGE_AFE_OFFSET] & 0x0000ffff);
			ad7146->cal_flags = CAL_RET_NONE;
		}
	} else {
		pr_info("[Grip]: %s, cal success\n", __func__);
	}

	offset_write(ad7146, offset);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	offset_filp = filp_open(OFFSET_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0666);
	if (IS_ERR(offset_filp)) {
		pr_err("[Grip]: %s: Can't open file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(offset_filp);
		goto save_fail;
	}

	err = offset_filp->f_op->write(offset_filp,
		(char *)&st_file, sizeof(unsigned short) * 2, &offset_filp->f_pos);
	if (err != sizeof(unsigned short) * 2) {
		pr_err("[Grip]: %s: Can't write the offset data to file\n", __func__);
		err = -EIO;
		filp_close(offset_filp, current->files);
		set_fs(old_fs);
		goto save_fail;
	}

	filp_close(offset_filp, current->files);
	set_fs(old_fs);

	return err;
save_fail:
	if (cal_flag) {
		if (ad7146->cal_flags == CAL_RET_SUCCESS) {
			ad7146_i2c_platform_data.cal_fixed_th_full = temp_fixed_th;
			ad7146_i2c_platform_data.cal_offset = temp_offset;
			if (temp_fixed_th == 0) {
				offset = (unsigned short)(ad7146_i2c_platform_data.regs[STAGE_AFE_OFFSET] & 0x0000ffff);
				ad7146->cal_flags = CAL_RET_NONE;
			} else {
				offset = temp_offset;
				ad7146->cal_flags = CAL_RET_EXIST;
			}
		}
	} else {
		ad7146_i2c_platform_data.cal_fixed_th_full = temp_fixed_th;
		ad7146_i2c_platform_data.cal_offset = temp_offset;
		if (temp_fixed_th == 0) {
			offset = (unsigned short)(ad7146_i2c_platform_data.regs[STAGE_AFE_OFFSET] & 0x0000ffff);
			ad7146->cal_flags = CAL_RET_NONE;
		} else {
			offset = temp_offset;
			ad7146->cal_flags = CAL_RET_EXIST;
		}
	}
	offset_write(ad7146, offset);
	return err;
}

static ssize_t ad7146_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", DEVICE_NAME);
}

static ssize_t ad7146_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t ad7146_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ad7146_chip *ad7146 = dev_get_drvdata(dev);
	unsigned short u16temp;

	if (ad7146->during_cal_flags == DONE_CALIBRATE) {
		ad7146->read(ad7146->dev, CDC_RESULT_S0_REG, &u16temp);

		if (u16temp < OVER_FLOW_SCALE_VALUE)
			u16temp = 0xFFFF;
		ad7146->during_cal_data = u16temp;
		pr_info("[Grip]: %s, raw_data : %d\n", __func__, u16temp);
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", ad7146->during_cal_data);
}

static ssize_t ad7146_onoff_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ad7146_chip *ad7146 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", ad7146->onoff_flags);
}

static ssize_t ad7146_onoff_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ad7146_chip *ad7146 = dev_get_drvdata(dev);
	int val, err;

	err = kstrtoint(buf, 10, &val);

	if (err < 0) {
		pr_err("[Grip]: %s, kstrtoint failed\n", __func__);
	} else {
		pr_info("[Grip]: %s: enable %d val %d\n", __func__, ad7146->onoff_flags, val);
		if ((val == 1) || (val == 0)) {
			if (ad7146->onoff_flags != ((unsigned char) val)) {
				ad7146->onoff_flags = (unsigned char) val;
				if ((ad7146->eventcheck == 1) && (ad7146->onoff_flags == 0)) {
						input_report_rel(ad7146->input, REL_MISC,
							EVENT_NO_GRIP + 1);
						input_sync(ad7146->input);
				} else if ((ad7146->eventcheck == 1) && (ad7146->onoff_flags == 1)) {
						input_report_rel(ad7146->input, REL_MISC,
							ad7146->state_value + 1);
						input_sync(ad7146->input);
						pr_info("[Grip]: %s : report state %d\n", __func__, ad7146->state_value);
				}
			}
		}
	}

	return size;
}

static ssize_t ad7146_threshold_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ad7146_chip *ad7146 = dev_get_drvdata(dev);
	unsigned int fixed_threshold, threshold;
	unsigned int sensitivity, data;
	unsigned short stg_th, stg_amb;

	if (ad7146->cal_flags == CAL_RET_NONE)
		fixed_threshold = ad7146_i2c_platform_data.fixed_th_full;
	else
		fixed_threshold = ad7146_i2c_platform_data.cal_fixed_th_full;

	data = (ad7146_i2c_platform_data.regs[4] & 0x0000ffff);
	sensitivity = (ad7146_i2c_platform_data.regs[3] & 0x0000ffff);
	threshold = (data / 4) + (((data - (data / 4)) / 16) * SS_LUT[((sensitivity & POS_SENS) >> 8)]) / 10000;
	threshold = (threshold * 194) / 100;

	ad7146->read(ad7146->dev, DRIVER_STG0_HIGH_THRESHOLD,
		&stg_th);
	ad7146->read(ad7146->dev, DRIVER_STG0_SF_AMBIENT,
		&stg_amb);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", (unsigned int)fixed_threshold,
		(unsigned int)threshold, (unsigned int) (threshold * (100 - HYS_PERCENT) / 100));
}

static ssize_t ad7146_calibration_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ad7146_chip *ad7146 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", (unsigned int)ad7146->cal_flags);
}

static ssize_t ad7146_calibration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ad7146_chip *ad7146 = dev_get_drvdata(dev);
	int val, err;

	err = kstrtoint(buf, 10, &val);

	if (err < 0) {
		pr_err("[Grip]: %s, kstrtoint failed\n", __func__);
	} else {
		ad7146->during_cal_flags = DO_CALIBRATE;

		if (ad7146->eventcheck != 1)
			ad7146_hw_init(ad7146);

		if (val == 0)
			ad7146_calibration(ad7146, false);
		else if (val == 1)
			ad7146_calibration(ad7146, true);
		else
			pr_err("[Grip]: %s, kstrtoint %d\n", __func__, val);

		ad7146->during_cal_flags = DONE_CALIBRATE;

		if (ad7146->eventcheck == 1)
			sensorgrip(ad7146);
		else
			sensorshutdownmode(ad7146);
	}
	return size;
}

static ssize_t ad7146_offset_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ad7146_chip *ad7146 = dev_get_drvdata(dev);
	unsigned short offset;
	short cal_offset;

	if (ad7146->cal_flags != CAL_RET_NONE)
		offset = ad7146_i2c_platform_data.cal_offset;
	else
		offset = (unsigned short)(ad7146_i2c_platform_data.regs[STAGE_AFE_OFFSET] & 0x0000ffff);

	cal_offset = LD_POS_AFE_OFFSET(offset) + LD_NEG_AFE_OFFSET(offset);

	return snprintf(buf, PAGE_SIZE, "%x,%d\n", offset, cal_offset);
}

static ssize_t ad7146_offset_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ad7146_chip *ad7146 = dev_get_drvdata(dev);
	int err;
	unsigned short data;
	short val;

	err = kstrtos16(buf, 10, &val);

	if (err < 0) {
		pr_err("[Grip]: %s, kstrtoint failed\n", __func__);
	} else {
		if (-MAX_OFFSET <= val && val <= MAX_OFFSET) {
			pr_info("[Grip]: %s, %d\n", __func__, val);
			if (val > 0)
				data = ST_POS_AFE_OFFSET(val);
			else
				data = ST_NEG_AFE_OFFSET(val);

			ad7146_i2c_platform_data.regs[STAGE_AFE_OFFSET] =
				(ad7146_i2c_platform_data.regs[STAGE_AFE_OFFSET] & 0xFFFF0000)
					+ ((unsigned int)data & 0x0000FFFF);

			ad7146->write(ad7146->dev, DRIVER_STG0_AFEOFFSET, data);
			ad7146_force_cal(ad7146, SLEEP_TIME_TO_CALI_INT);
		} else {
			pr_err("[Grip]: %s, Error for data val(%d)\n", __func__, val);
		}
	}

	return size;
}

static struct device_attribute dev_attr_sensor_name =
	__ATTR(name, S_IRUSR | S_IRGRP,
	ad7146_name_show, NULL);
static struct device_attribute dev_attr_sensor_vendor =
	__ATTR(vendor, S_IRUSR | S_IRGRP,
	ad7146_vendor_show, NULL);
static struct device_attribute dev_attr_sensor_raw_data =
	__ATTR(raw_data, S_IRUSR | S_IRGRP,
	ad7146_raw_data_show, NULL);
static struct device_attribute dev_attr_sensor_onoff =
	__ATTR(onoff, S_IRUGO | S_IWUSR | S_IWGRP,
	ad7146_onoff_show, ad7146_onoff_store);
static struct device_attribute dev_attr_sensor_threshold =
	__ATTR(threshold, S_IRUGO | S_IWUSR | S_IWGRP,
	ad7146_threshold_show, NULL);
static struct device_attribute dev_attr_sensor_calibration =
	__ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
	ad7146_calibration_show, ad7146_calibration_store);
static struct device_attribute dev_attr_sensor_offset =
	__ATTR(offset, S_IRUGO | S_IWUSR | S_IWGRP,
	ad7146_offset_show, ad7146_offset_store);


static struct device_attribute *ad7146_attrs[] = {
	&dev_attr_sensor_name,
	&dev_attr_sensor_vendor,
	&dev_attr_sensor_raw_data,
	&dev_attr_sensor_onoff,
	&dev_attr_sensor_threshold,
	&dev_attr_sensor_calibration,
	&dev_attr_sensor_offset,
	&dev_attr_sensor_dump,
	NULL,
};

static int ad7146_parse_dt(struct ad7146_chip *ad7146, struct device *dev)
{
	struct device_node *dNode = dev->of_node;
	enum of_gpio_flags flags;

	if (dNode == NULL)
		return -ENODEV;

	ad7146->grip_int = of_get_named_gpio_flags(dNode,
		"ad7146-i2c,grip_int-gpio", 0, &flags);
	if (ad7146->grip_int < 0) {
		pr_err("[Grip]: %s - get grip_int error\n", __func__);
		return -ENODEV;
	}

	return 0;
}
/**
  Device probe function
  All initialization routines are handled here like the ISR registration,
  Work creation,Input device registration,SYSFS attributes creation etc.

  @param i2c_client the i2c structure of the ad7146 device/client.
  @param i2c_device_id The i2c_device_id for the supported i2c device.

  @return 0 on success,and On failure -ENOMEM, -EINVAL ,etc., will be returned
 */
static int __devinit ad7146_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = -EINVAL;
	struct input_dev *input = NULL;
	struct device *dev = &client->dev;
	struct ad7146_chip *ad7146 = NULL;

	pr_info("[Grip]: %s: called", __func__);

	if (client == NULL) {
		pr_err("[Grip]: %s: Client doesn't exist\n", __func__);
		return ret;
	}
	ad7146 = kzalloc(sizeof(struct ad7146_chip), GFP_KERNEL);
	if (!ad7146) {
		pr_err("[Grip]: %s: Memory allocation fail\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

	ad7146->read = ad7146_i2c_read;
	ad7146->write = ad7146_i2c_write;
	ad7146->dev = dev;

	ret = ad7146_parse_dt(ad7146, &client->dev);
	if (ret < 0) {
		pr_err("[Grip]: %s: - of_node error\n", __func__);
		ret = -ENODEV;
		goto err_of_node;
	}

	i2c_set_clientdata(client, ad7146);

	INIT_WORK(&ad7146->work, ad7146_interrupt_thread);

	/*
	 * Allocate and register ad7146 input device
	 */
	input = input_allocate_device();
	if (!input) {
		pr_err("[Grip]: %s: could not allocate input device\n", __func__);
		ret = -ENOMEM;
		goto err_kzalloc_mem;
	}

	ad7146->input = input;
	input_set_drvdata(ad7146->input, ad7146);
	input->name = "grip_sensor";
	set_bit(EV_REL, ad7146->input->evbit);
	input_set_capability(ad7146->input, EV_REL, REL_MISC);

	ret = input_register_device(ad7146->input);
	if (ret) {
		pr_err("[Grip]: %s: could not input_register_device(input);\n", __func__);
		ret = -ENOMEM;
		goto err_input_register_device;
	}

	ret = sensors_create_symlink(&ad7146->input->dev.kobj,
					ad7146->input->name);
	if (ret < 0) {
		pr_err("[Grip]: %s: create_symlink error\n", __func__);
		goto err_sensors_create_symlink;
	}

	ad7146->eventcheck = 0;

	wake_lock_init(&ad7146->grip_wake_lock, WAKE_LOCK_SUSPEND, "grip_wake_lock");

	ret = gpio_request(ad7146->grip_int, "gpio_grip_int");
	pr_info("[Grip]: %s : gpio request %d\n", __func__, ad7146->grip_int);
	if (ret < 0) {
		pr_err("[Grip]: %s: gpio %d request failed (%d)\n",
		       __func__, ad7146->grip_int, ret);
		goto err_gpio_request;
	}

	ret = gpio_direction_input(ad7146->grip_int);
	if (ret < 0) {
		pr_err("[Grip]: %s: failed to set gpio %d as input (%d)\n",
		       __func__, ad7146->grip_int, ret);
		goto err_free_irq;
	}

	ad7146->irq = gpio_to_irq(ad7146->grip_int);
	pr_info("[Grip]: %s: irq %d\n", __func__, ad7146->irq);

	ret = request_threaded_irq(ad7146->irq, NULL, ad7146_isr,
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			dev_name(dev), ad7146);

	if (ret) {
		pr_err("[Grip]: %s: irq %d Driver init Failed", __func__, ad7146->irq);
		goto err_free_irq;
	}
	disable_irq(ad7146->irq);

	ret = sysfs_create_group(&input->dev.kobj, &ad7146_attr_group);
	if (ret) {
		pr_err("[Grip]: %s: cound not register sensor device\n", __func__);
		goto err_sysfs_create_input;
	}

	ret = sensors_register(ad7146->grip_dev, ad7146, ad7146_attrs, "grip_sensor");
	if (ret) {
		pr_err("[Grip]: %s: cound not register sensor device\n", __func__);
		goto err_sysfs_create_factory_grip;
	}

#ifdef CONFIG_SENSOR_USE_SYMLINK
	ret =  sensors_initialize_symlink(input);
	if (ret) {
		pr_err("[Grip]: %s: cound not make grip sensor symlink(%d).\n",
			__func__, ret);
		goto out_sensors_initialize_symlink;
	}
#endif

	/* check if the device is existing by reading device id of AD7146 */
	ret = ad7146_hw_detect(ad7146);
	if (ret)
		goto err_hw_detect;

	/* initialize and request sw/hw resources */
	sensorshutdownmode(ad7146);
	ad7146->onoff_flags = 1;

	pr_info("[Grip]: %s - probe done\n", __func__);
	return 0;
err_hw_detect:
	pr_err("[Grip]: %s: hw detect error\n", __func__);
#ifdef CONFIG_SENSOR_USE_SYMLINK
out_sensors_initialize_symlink:
#endif
err_sysfs_create_factory_grip:
	sysfs_remove_group(&input->dev.kobj, &ad7146_attr_group);
err_sysfs_create_input:
	free_irq(ad7146->irq, ad7146);
err_free_irq:
	gpio_free(ad7146->grip_int);
err_gpio_request:
	wake_lock_destroy(&ad7146->grip_wake_lock);
err_sensors_create_symlink:
	input_unregister_device(input);
err_input_register_device:
	input_free_device(input);
err_kzalloc_mem:
err_of_node:
	kfree(ad7146);
	pr_info("[Grip]: %s - probe failed(%d)\n", __func__, ret);
	return ret;
}

/**
  Removes the Device.
  This is used to Remove the device or the I2C client from the system

  @param client The Client Id to be removed
  @return 0 on success
 */
static int __devexit ad7146_i2c_remove(struct i2c_client *client)
{
	struct ad7146_chip *ad7146 = i2c_get_clientdata(client);

	pr_info("%s, Start\n", __func__);
	if (ad7146 != NULL) {
		if (ad7146->eventcheck == ENABLE_INTERRUPTS) {
			disable_irq(ad7146->irq);
			disable_irq_wake(ad7146->irq);
			ad7146->write(ad7146->dev, STG_COM_INT_EN_REG, DISABLE_INT);
			ad7146->write(ad7146->dev, STG_HIGH_INT_EN_REG, DISABLE_INT);
			ad7146->write(ad7146->dev, STG_LOW_INT_EN_REG, DISABLE_INT);
			msleep(SLEEP_TIME_TO_CALI_INT);
		}
		sysfs_remove_group(&ad7146->input->dev.kobj, &ad7146_attr_group);
		free_irq(ad7146->irq, ad7146);
		gpio_free(client->irq);
		input_unregister_device(ad7146->input);
		input_free_device(ad7146->input);
		kfree(ad7146);
	}
	return 0;
}

void ad7146_shutdown(struct i2c_client *client)
{
	struct ad7146_chip *ad7146 = i2c_get_clientdata(client);
	pr_info("%s, Start\n", __func__);
	if (ad7146 != NULL) {
		if (ad7146->eventcheck == ENABLE_INTERRUPTS) {
			disable_irq(ad7146->irq);
			disable_irq_wake(ad7146->irq);
			ad7146->write(ad7146->dev, STG_COM_INT_EN_REG, DISABLE_INT);
			ad7146->write(ad7146->dev, STG_HIGH_INT_EN_REG, DISABLE_INT);
			ad7146->write(ad7146->dev, STG_LOW_INT_EN_REG, DISABLE_INT);
			msleep(SLEEP_TIME_TO_CALI_INT);
		}
		sysfs_remove_group(&ad7146->input->dev.kobj, &ad7146_attr_group);
		free_irq(ad7146->irq, ad7146);
		gpio_free(client->irq);
		input_unregister_device(ad7146->input);
		input_free_device(ad7146->input);
		kfree(ad7146);
	}
}
/**
Device ID table for the AD7146 driver
*/
static int grip_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ad7146_chip *ad7146 = i2c_get_clientdata(client);

	pr_info("%s, check (%d)\n", __func__, ad7146->eventcheck);
/*	if (ad7146->eventcheck == 0)
		ad7146->hw->power_en(0);*/

	return 0;
}

static int grip_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ad7146_chip *ad7146 = i2c_get_clientdata(client);

	pr_info("%s, check (%d)\n", __func__, ad7146->eventcheck);
	if (ad7146->eventcheck == 0) {
		//ad7146->hw->power_en(1);
		msleep(10);
		sensorshutdownmode(ad7146);
	}

	return 0;
}

static struct of_device_id ad7146_match_table[] = {
	{ .compatible = "ad7146-i2c",},
	{},
};

static const struct i2c_device_id ad7146_id[] = {
	{"ad7146_match_table", 0},
	{}
/*
	{ "ad7146_SAR_NORM", 0 },
	{ "ad7146_SAR_PROX", 1 },
	{ "ad7146_SAR", 2 }, {},
*/
};

static const struct dev_pm_ops grip_dev_pm_ops = {
	.suspend = grip_i2c_suspend,
	.resume = grip_i2c_resume,
};

/**
  The file Operation Table
 */
struct i2c_driver ad7146_i2c_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = ad7146_match_table,
		.pm = &grip_dev_pm_ops,
	},
	.probe    = ad7146_probe,
	.shutdown = ad7146_shutdown,
	.remove   = __devexit_p(ad7146_i2c_remove),
	.id_table = ad7146_id,
};

/**
  This is an init function called during module insertion --
  calls in turn i2c driver probe function
 */
static __init int ad7146_i2c_init(void)
{
	pr_info("%s, start\n", __func__);
	return i2c_add_driver(&ad7146_i2c_driver);
}
module_init(ad7146_i2c_init);

/**
  Called during the module removal
 */
static __exit void ad7146_i2c_exit(void)
{
	i2c_del_driver(&ad7146_i2c_driver);
}

module_exit(ad7146_i2c_exit);
MODULE_DESCRIPTION("Analog Devices ad7146 MLD Driver");
MODULE_AUTHOR("Analog Devices");
MODULE_LICENSE("GPL");

