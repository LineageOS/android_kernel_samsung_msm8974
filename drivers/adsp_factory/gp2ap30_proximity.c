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
#define VENDOR    "Sharp"
#define CHIP_ID   "GP2AP30"

static ssize_t proximity_vendor_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t proximity_name_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}
/*
static ssize_t proximity_state_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	 struct cm3323_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u,%u,%u,%u\n",
	data->color[0], data->color[1],
	data->color[2], data->color[3]);
	return 0;
}*/

static ssize_t proximity_data_show(struct device *dev,
struct device_attribute *attr, char *buf)
{

    struct adsp_data *data = dev_get_drvdata(dev);
	static int count;		/*count for proximity average */

    if( !(data->data_ready_flag & 1 << ADSP_FACTORY_MODE_PROX) ){
      adsp_unicast(ADSP_FACTORY_MODE_PROX,NETLINK_MESSAGE_GET_RAW_DATA,0,0);
    }
    while( !(data->data_ready_flag & 1 << ADSP_FACTORY_MODE_PROX) )
      msleep(20);

	data->prox_average[count] = data->sensor_data[ADSP_FACTORY_MODE_PROX].y;
	count++;
	if (count == PROX_READ_NUM)
		count = 0;
    return snprintf(buf, PAGE_SIZE, "%d\n",
          data->sensor_data[ADSP_FACTORY_MODE_PROX].y);

}

static ssize_t proximity_avg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);

	int min = 0, max = 0, avg = 0;
	int i;
	int proximity_value = 0;

	for (i = 0; i < PROX_READ_NUM; i++) {
		proximity_value = data->prox_average[i];
		if (proximity_value > 0) {

			avg += proximity_value;

			if (!i)
				min = proximity_value;
			else if (proximity_value < min)
				min = proximity_value;

			if (proximity_value > max)
				max = proximity_value;
		}
	}
	avg /= i;

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d\n", min, avg, max);
}

static ssize_t proximity_thresh_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);

	adsp_unicast(ADSP_FACTORY_MODE_PROX,NETLINK_MESSAGE_GET_CALIB_DATA,0,0);
	while( !(data->calib_ready_flag & 1 << ADSP_FACTORY_MODE_PROX) )
			msleep(20);

	data->calib_ready_flag |= 0 << ADSP_FACTORY_MODE_PROX;
	return sprintf(buf, "%d\n", data->sensor_calib_data[ADSP_FACTORY_MODE_PROX].y);
}

static ssize_t proximity_offset_pass_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);

	adsp_unicast(ADSP_FACTORY_MODE_PROX,NETLINK_MESSAGE_GET_CALIB_DATA,0,0);

	while( !(data->calib_ready_flag & 1 << ADSP_FACTORY_MODE_PROX) )
		msleep(20);

	data->calib_ready_flag |= 0 << ADSP_FACTORY_MODE_PROX;

	return sprintf(buf, "%d\n", data->sensor_calib_data[ADSP_FACTORY_MODE_PROX].x);
}


static ssize_t proximity_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);

	adsp_unicast(ADSP_FACTORY_MODE_PROX,NETLINK_MESSAGE_GET_CALIB_DATA,0,0);

	while( !(data->calib_ready_flag & 1 << ADSP_FACTORY_MODE_PROX) )
		msleep(20);

	data->calib_ready_flag |= 0 << ADSP_FACTORY_MODE_PROX;

	printk("prox_offset_pass_show %d\n",data->sensor_calib_data[ADSP_FACTORY_MODE_PROX].x);

	return sprintf(buf, "%d %d\n", data->sensor_calib_data[ADSP_FACTORY_MODE_PROX].x, data->sensor_calib_data[ADSP_FACTORY_MODE_PROX].y);
}

static ssize_t proximity_cal_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	adsp_unicast(ADSP_FACTORY_MODE_PROX,NETLINK_MESSAGE_CALIB_STORE_DATA,0,0);

	while( !(data->calib_store_ready_flag & 1 << ADSP_FACTORY_MODE_PROX) )
		msleep(20);

	if(data->sensor_calib_result[ADSP_FACTORY_MODE_PROX].nCalibstoreresult < 0)
		pr_err("[FACTORY]: %s - prox_do_calibrate() failed\n", __func__);

	data->calib_store_ready_flag |= 0 << ADSP_FACTORY_MODE_PROX;

	return size;
}


int proximity_factory_init(struct adsp_data *data)
{
	return 0;
}
int proximity_factory_exit(struct adsp_data *data)
{
	return 0;
}
int proximity_factory_receive_data(struct adsp_data *data)
{
	pr_err("(%s): factory \n", __func__);
	return 0;
}

static struct adsp_fac_ctl adsp_fact_cb = {
	.init_fnc = proximity_factory_init,
	.exit_fnc = proximity_factory_exit,
	.receive_data_fnc = proximity_factory_receive_data
};

static DEVICE_ATTR(name, 0664, proximity_name_show, NULL);
static DEVICE_ATTR(vendor, 0664, proximity_vendor_show, NULL);
static DEVICE_ATTR(state, 0664, proximity_data_show, NULL);
static DEVICE_ATTR(raw_data, 0664, proximity_data_show, NULL);
static DEVICE_ATTR(thresh, 0664, proximity_thresh_show, NULL);
static DEVICE_ATTR(prox_avg, 0664,proximity_avg_show, NULL);
static DEVICE_ATTR(offset_pass, 0664,proximity_offset_pass_show, NULL);
static DEVICE_ATTR(prox_cal, 0664,proximity_cal_show, proximity_cal_store);



static struct device_attribute *proximity_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_state,
	&dev_attr_raw_data,
	&dev_attr_thresh,
	&dev_attr_prox_avg,
	&dev_attr_offset_pass,
	&dev_attr_prox_cal,
	NULL,
};


static int __devinit gp2ap30proximity_factory_init(void)
{
	adsp_factory_register("proximity_sensor",ADSP_FACTORY_MODE_PROX,proximity_attrs,&adsp_fact_cb);
	pr_err("(%s): factory \n", __func__);
	return 0;
}
static void __devexit gp2ap30proximity_factory_exit(void)
{
	pr_err("(%s): factory \n", __func__);
}
module_init(gp2ap30proximity_factory_init);
module_exit(gp2ap30proximity_factory_exit);

