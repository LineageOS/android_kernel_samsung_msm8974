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

static ssize_t light_vendor_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t light_name_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static ssize_t light_lux_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	/* struct cm3323_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u,%u,%u,%u\n",
	data->color[0], data->color[1],
	data->color[2], data->color[3]);*/
	return 0;
}

static ssize_t light_data_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
    struct adsp_data *data = dev_get_drvdata(dev);

    if( !(data->data_ready_flag & 1 << ADSP_FACTORY_MODE_LIGHT) ){
      adsp_unicast(ADSP_FACTORY_MODE_LIGHT,NETLINK_MESSAGE_GET_RAW_DATA,0,0);
    }
    while( !(data->data_ready_flag & 1 << ADSP_FACTORY_MODE_LIGHT) )
      msleep(20);

    return snprintf(buf, PAGE_SIZE, "%d,%d\n",
      data->sensor_data[ADSP_FACTORY_MODE_LIGHT].y,
      data->sensor_data[ADSP_FACTORY_MODE_LIGHT].z);

	return 0;
}

int light_factory_init(struct adsp_data *data)
{
	return 0;
}
int light_factory_exit(struct adsp_data *data)
{
	return 0;
}
int light_factory_receive_data(struct adsp_data *data)
{
	pr_err("(%s): factory \n", __func__);
	return 0;
}

static struct adsp_fac_ctl adsp_fact_cb = {
	.init_fnc = light_factory_init,
	.exit_fnc = light_factory_exit,
	.receive_data_fnc = light_factory_receive_data
};


static DEVICE_ATTR(name, 0664, light_name_show, NULL);
static DEVICE_ATTR(vendor, 0664, light_vendor_show, NULL);
static DEVICE_ATTR(lux, 0664, light_lux_show, NULL);
static DEVICE_ATTR(raw_data, 0664, light_data_show, NULL);

static struct device_attribute *light_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_lux,
	&dev_attr_raw_data,
	NULL,
};


static int __devinit gp2alight_factory_init(void)
{
	adsp_factory_register("light_sensor",ADSP_FACTORY_MODE_LIGHT,light_attrs,&adsp_fact_cb);
	pr_err("(%s): factory \n", __func__);
	return 0;
}
static void __devexit gp2alight_factory_exit(void)
{
	pr_err("(%s): factory \n", __func__);
}
module_init(gp2alight_factory_init);
module_exit(gp2alight_factory_exit);

