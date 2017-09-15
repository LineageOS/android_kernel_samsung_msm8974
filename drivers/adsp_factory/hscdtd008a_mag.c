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
#include "adsp.h"
#define VENDOR		"Alps"
#define CHIP_ID		"HSCDTD008A"

static struct work_struct timer_stop_data_work;
extern unsigned int raw_data_stream;

static ssize_t mag_vendor_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t mag_name_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static ssize_t raw_data_read(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct msg_data message;
	unsigned long timeout = jiffies + (2*HZ);
	struct adsp_data *data = dev_get_drvdata(dev);
	if( !(raw_data_stream &  ADSP_RAW_MAG) ){
		pr_err("Starting raw data  \n");
		raw_data_stream |= ADSP_RAW_MAG;
		message.sensor_type = ADSP_FACTORY_MODE_MAG;
		adsp_unicast(message,NETLINK_MESSAGE_GET_RAW_DATA,0,0);
	}
	while( !(data->data_ready_flag & 1 << ADSP_FACTORY_MODE_MAG) ){
		msleep(20);
		if (time_after(jiffies, timeout)) {
			return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",0,0,0);
		}
	}
	adsp_factory_start_timer(2000);
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->sensor_data[ADSP_FACTORY_MODE_MAG].x,
		data->sensor_data[ADSP_FACTORY_MODE_MAG].y,
		data->sensor_data[ADSP_FACTORY_MODE_MAG].z);
}

static ssize_t dac_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", 0, 0, 0);
}

static ssize_t status_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	//struct adsp_data *data = dev_get_drvdata(dev);
	int result = 0;
	//adsp_unicast(ADSP_FACTORY_MODE_MAG,NETLINK_MESSAGE_CALIB_STORE_DATA,0,0);

//	while( !(data->calib_store_ready_flag & 1 << ADSP_FACTORY_MODE_MAG) )
//		msleep(20);

//	if(data->sensor_calib_result[ADSP_FACTORY_MODE_MAG].nCalibstoreresult < 0)
//		pr_err("[FACTORY]: %s - accel_do_calibrate() failed\n", __func__);
//	data->calib_store_ready_flag = 0;

	if (result == 0)
		result = 1;
	else
		result = 0;
	return snprintf(buf, PAGE_SIZE, "%d,%d\n", result, 0);
}
static void mag_stop_raw_data_worker(struct work_struct *work)
{
	struct msg_data message;
	pr_err("(%s):  \n", __func__);
	message.sensor_type = ADSP_FACTORY_MODE_MAG;
	adsp_unicast(message,NETLINK_MESSAGE_STOP_RAW_DATA,0,0);
	return;
}
static ssize_t selttest_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int result1 = 0;
	int result2 = 0;

	struct msg_data message;
	message.sensor_type = ADSP_FACTORY_MODE_MAG;
	adsp_unicast(message,NETLINK_MESSAGE_SELFTEST_SHOW_DATA,0,0);

	while( !(data->selftest_ready_flag & 1 << ADSP_FACTORY_MODE_MAG) )
		msleep(20);

	if(data->sensor_selftest_result[ADSP_FACTORY_MODE_MAG].nSelftestresult1< 0)
		pr_err("[FACTORY]: %s - accel_do_calibrate() failed\n", __func__);
	data->selftest_ready_flag = 0;
	if (data->sensor_selftest_result[ADSP_FACTORY_MODE_MAG].nSelftestresult1 == 0)
		result1 = 1;
	else
		result1 = 0;
	if (data->sensor_selftest_result[ADSP_FACTORY_MODE_MAG].nSelftestresult2 == 0)
		result2 = 1;
	else
		result2 = 0;
	return snprintf(buf, PAGE_SIZE, "%d, %d\n", result1, result2);
}

int mag_factory_init(struct adsp_data *data)
{
	return 0;
}
int mag_factory_exit(struct adsp_data *data)
{
	return 0;
}
int mag_factory_receive_data(struct adsp_data *data,int cmd)
{
	pr_err("(%s): factory \n", __func__);
	switch(cmd)
	{
		case CALLBACK_REGISTER_SUCCESS:
			pr_info("[SENSOR] %s: mpu6515 registration success \n", __func__);
			break;
		case CALLBACK_TIMER_EXPIRED:
			pr_err("Raw data flag = %d\n", raw_data_stream);
			if( (raw_data_stream &  ADSP_RAW_MAG) ){
				raw_data_stream &= ~ADSP_RAW_MAG;
				schedule_work(&timer_stop_data_work);
			}
			break;
		default:
			break;
	}
	return 0;
}

static struct adsp_fac_ctl adsp_fact_cb = {
	.init_fnc = mag_factory_init,
	.exit_fnc = mag_factory_exit,
	.receive_data_fnc = mag_factory_receive_data
};

static DEVICE_ATTR(selftest, 0664, selttest_show, NULL);
static DEVICE_ATTR(status, 0664, status_show, NULL);
static DEVICE_ATTR(dac, 0644, dac_show, NULL);
static DEVICE_ATTR(adc, 0644,  raw_data_read, NULL);
static DEVICE_ATTR(name, 0440, mag_name_show, NULL);
static DEVICE_ATTR(vendor, 0440, mag_vendor_show, NULL);
static DEVICE_ATTR(raw_data, 0644, raw_data_read, NULL);

static struct device_attribute *mag_attrs[] = {
	&dev_attr_selftest,
	&dev_attr_status,
	&dev_attr_dac,
	&dev_attr_adc,
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_raw_data,
	NULL,
};

static int __devinit hscdtd008amag_factory_init(void)
{
	adsp_factory_register("magnetic_sensor",ADSP_FACTORY_MODE_MAG,mag_attrs,&adsp_fact_cb);
	pr_err("(%s): factory \n", __func__);
	INIT_WORK(&timer_stop_data_work,
				  mag_stop_raw_data_worker );
	return 0;
}
static void __devexit hscdtd008amag_factory_exit(void)
{
	pr_err("(%s): factory \n", __func__);
}
module_init(hscdtd008amag_factory_init);
module_exit(hscdtd008amag_factory_exit);
