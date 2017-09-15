/*
*  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>
#include "adsp.h"
#define VENDOR		"BOSCH"
#define CHIP_ID		"MPU6500"

/*static void adsp_timer_expire(unsigned long timer_var)
{
 pr_err("[FACTORY]: %s - adsp_timer_expire() Called\n", __func__);

adsp_unicast(ADSP_FACTORY_MODE_ACCEL,NETLINK_MESSAGE_STOP_RAW_DATA,0,0);
}*/


static ssize_t accel_vendor_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t accel_name_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static ssize_t accel_calibration_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int iCount = 0;
	adsp_unicast(ADSP_FACTORY_MODE_ACCEL,NETLINK_MESSAGE_GET_CALIB_DATA,0,0);

	while( !(data->calib_ready_flag & 1 << ADSP_FACTORY_MODE_ACCEL) )
		msleep(20);

	iCount = snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n",  data->sensor_calib_data[ADSP_FACTORY_MODE_ACCEL].result,
		data->sensor_calib_data[ADSP_FACTORY_MODE_ACCEL].x,
		data->sensor_calib_data[ADSP_FACTORY_MODE_ACCEL].y,
		data->sensor_calib_data[ADSP_FACTORY_MODE_ACCEL].z);
	data->calib_ready_flag = 0;

	return iCount;
}

static ssize_t accel_calibration_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	adsp_unicast(ADSP_FACTORY_MODE_ACCEL,NETLINK_MESSAGE_CALIB_STORE_DATA,0,0);

	while( !(data->calib_store_ready_flag & 1 << ADSP_FACTORY_MODE_ACCEL) )
		msleep(20);

	if(data->sensor_calib_result[ADSP_FACTORY_MODE_ACCEL].nCalibstoreresult < 0)
		pr_err("[FACTORY]: %s - accel_do_calibrate() failed\n", __func__);
	data->calib_store_ready_flag = 0;
	return size;
}

static ssize_t raw_data_read(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
  //  data->our_timer.expires = 2000 +jiffies;
//	data->our_timer.function = adsp_timer_expire;
//	mod_timer(&data->our_timer, jiffies + 2000);
	if( !(data->data_ready_flag & 1 << ADSP_FACTORY_MODE_ACCEL) ){
		adsp_unicast(ADSP_FACTORY_MODE_ACCEL,NETLINK_MESSAGE_GET_RAW_DATA,0,0);
	}
	while( !(data->data_ready_flag & 1 << ADSP_FACTORY_MODE_ACCEL) )
		msleep(20);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->sensor_data[ADSP_FACTORY_MODE_ACCEL].x,
		data->sensor_data[ADSP_FACTORY_MODE_ACCEL].y,
		data->sensor_data[ADSP_FACTORY_MODE_ACCEL].z);
}

int accel_factory_init(struct adsp_data *data)
{
	return 0;
}
int accel_factory_exit(struct adsp_data *data)
{
	return 0;
}
int accel_factory_receive_data(struct adsp_data *data)
{
	pr_err("(%s): factory \n", __func__);
	return 0;
}

static struct adsp_fac_ctl adsp_fact_cb = {
	.init_fnc = accel_factory_init,
	.exit_fnc = accel_factory_exit,
	.receive_data_fnc = accel_factory_receive_data
};

static DEVICE_ATTR(name, S_IRUGO, accel_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, accel_vendor_show, NULL);
static DEVICE_ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
	accel_calibration_show, accel_calibration_store);
static DEVICE_ATTR(raw_data, S_IRUGO, raw_data_read, NULL);

static struct device_attribute *acc_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_calibration,
	&dev_attr_raw_data,
	NULL,
};

static int __devinit mpu6500accel_factory_init(void)
{
	adsp_factory_register("accelerometer_sensor",ADSP_FACTORY_MODE_ACCEL,acc_attrs,&adsp_fact_cb);
	pr_err("(%s): factory \n", __func__);
	return 0;
}
static void __devexit mpu6500accel_factory_exit(void)
{
	pr_err("(%s): factory \n", __func__);
}
module_init(mpu6500accel_factory_init);
module_exit(mpu6500accel_factory_exit);
