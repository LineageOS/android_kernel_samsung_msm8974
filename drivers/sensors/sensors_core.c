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

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/input.h>
#include "sensors_core.h"
#ifdef CONFIG_ADSP_FACTORY
#include <linux/kernel.h>
#endif

struct class *sensors_class;
struct class *sensors_event_class;
static struct device *symlink_dev;
static atomic_t sensor_count;
static struct device *sensor_dev;
static struct input_dev *meta_input_dev;

struct axis_remap {
	int src_x:3;
	int src_y:3;
	int src_z:3;

	int sign_x:2;
	int sign_y:2;
	int sign_z:2;
};

/*!
 * @Description:
 * definitions of placement of sensors
 * P0 - P3: view from top of device
 * P4 - P7: view from bottom of device
 *
 * P0 / P4:
 * Y of device aligned with Y of OS (i.e: Android)
 *                  y
 *                  ^
 *                  |
 *                  |
 *                  |
 *                  o------> x
 *
 *
 * P1 / P5:
 * Y of device aligned with Y of OS (i.e.: Android)
 * rotated by 90 degrees clockwise
 *
 *                  o------> y
 *                  |
 *                  |
 *                  |
 *                  v x
 *
 *
 * P2 / P6:
 * Y of device aligned with Y of OS (i.e.: Android)
 * rotated by 180 degrees clockwise
 *
 *         x <------o
 *                  |
 *                  |
 *                  |
 *                  v
 *                  y
 *
 *
 * P3 / P7:
 * Y of device aligned with Y of OS (i.e.: Android)
 * rotated by 270 degrees clockwise
 *
 *                  x
 *                  ^
 *                  |
 *                  |
 *                  |
 *         y <------o
 *
 */

#define MAX_AXIS_REMAP_TAB_SZ 8 /* P0~P7 */

static const struct axis_remap axis_table[MAX_AXIS_REMAP_TAB_SZ] = {
	/* src_x src_y src_z  sign_x  sign_y  sign_z */
	{  0,    1,    2,     1,      1,      1 }, /* P0 */
	{  1,    0,    2,     1,     -1,      1 }, /* P1 */
	{  0,    1,    2,    -1,     -1,      1 }, /* P2 */
	{  1,    0,    2,    -1,      1,      1 }, /* P3 */
	{  0,    1,    2,    -1,      1,     -1 }, /* P4 */
	{  1,    0,    2,    -1,     -1,     -1 }, /* P5 */
	{  0,    1,    2,     1,     -1,     -1 }, /* P6 */
	{  1,    0,    2,     1,      1,     -1 }, /* P7 */
};

void remap_sensor_data(s16 *val, u32 idx)
{
	s16 tmp[3];

	if (idx < MAX_AXIS_REMAP_TAB_SZ) {
		tmp[0] = val[axis_table[idx].src_x] * axis_table[idx].sign_x;
		tmp[1] = val[axis_table[idx].src_y] * axis_table[idx].sign_y;
		tmp[2] = val[axis_table[idx].src_z] * axis_table[idx].sign_z;

		memcpy(val, &tmp, sizeof(tmp));
	}
}

void remap_sensor_data_32(int *val, u32 idx)
{
	int tmp[3];

	if (idx < MAX_AXIS_REMAP_TAB_SZ) {
		tmp[0] = val[axis_table[idx].src_x] * axis_table[idx].sign_x;
		tmp[1] = val[axis_table[idx].src_y] * axis_table[idx].sign_y;
		tmp[2] = val[axis_table[idx].src_z] * axis_table[idx].sign_z;

		memcpy(val, &tmp, sizeof(tmp));
	}
}

int sensors_create_symlink(struct kobject *target, const char *name)
{
	int err = 0;

	if (symlink_dev == NULL) {
		pr_err("%s, symlink_dev is NULL!!!\n", __func__);
		return err ;
	}

	err = sysfs_create_link(&symlink_dev->kobj, target, name);
	if (err < 0) {
		pr_err("%s, %s failed!(%d)\n", __func__, name, err);
		return err;
	}

	return err;
}

void sensors_remove_symlink(struct kobject *target, const char *name)
{
	if (symlink_dev == NULL)
		pr_err("%s, symlink_dev is NULL!!!\n", __func__);
	else
		sysfs_delete_link(&symlink_dev->kobj, target, name);
}

static ssize_t set_flush(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	int64_t dTemp;
	u8 sensor_type = 0;

	if (kstrtoll(buf, 10, &dTemp) < 0)
		return -EINVAL;

	sensor_type = (u8)dTemp;

	input_report_rel(meta_input_dev, REL_DIAL, 1);
	input_report_rel(meta_input_dev, REL_HWHEEL, sensor_type + 1);
	input_sync(meta_input_dev);

	pr_info("[SENSOR] flush %d\n", sensor_type);
	return size;
}

static DEVICE_ATTR(flush, S_IWUSR | S_IWGRP, NULL, set_flush);

static struct device_attribute *sensor_attr[] = {
	&dev_attr_flush,
	NULL,
};

static void set_sensor_attr(struct device *dev,
		struct device_attribute *attributes[])
{
	int i;

	for (i = 0; attributes[i] != NULL; i++)
		if ((device_create_file(dev, attributes[i])) < 0)
			pr_err("[SENSOR CORE] device_create_file failed"\
				"(dev, attributes[%d])\n", i);
}

int sensors_register(struct device *dev, void *drvdata,
		struct device_attribute *attributes[], char *name)
{
	int ret = 0;

	if (sensors_class == NULL) {
		sensors_class = class_create(THIS_MODULE, "sensors");
		if (IS_ERR(sensors_class))
			return PTR_ERR(sensors_class);
	}

	dev = device_create(sensors_class, NULL, 0, drvdata, "%s", name);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("[SENSORS CORE] device_create failed for %s [%d]\n", name, ret);
		return ret;
	}

	set_sensor_attr(dev, attributes);
	atomic_inc(&sensor_count);

	return ret;
}

void sensors_unregister(struct device *dev,
		struct device_attribute *attributes[])
{
	int i;

	for (i = 0; attributes[i] != NULL; i++)
		device_remove_file(dev, attributes[i]);
}

void destroy_sensor_class(void)
{
	if (sensors_class) {
		if (sensor_dev) {
			device_destroy(sensors_class, sensor_dev->devt);
			sensor_dev = NULL;
		}
		class_destroy(sensors_class);
		sensors_class = NULL;
	}

	if (sensors_event_class) {
		device_destroy(sensors_event_class, symlink_dev->devt);
		class_destroy(sensors_event_class);
		symlink_dev = NULL;
		sensors_event_class = NULL;
	}
}

int sensors_meta_init(void)
{
	int ret;

	meta_input_dev = input_allocate_device();
	if (!meta_input_dev) {
		pr_err("[SENSOR CORE] failed alloc meta dev\n");
		return -ENOMEM;
	}

	meta_input_dev->name = "meta_event";
	input_set_capability(meta_input_dev, EV_REL, REL_HWHEEL);
	input_set_capability(meta_input_dev, EV_REL, REL_DIAL);

	ret = input_register_device(meta_input_dev);
	if (ret < 0) {
		pr_err("[SENSOR CORE] failed register meta dev\n");
		input_free_device(meta_input_dev);
	}

	ret = sensors_create_symlink(&meta_input_dev->dev.kobj,
		meta_input_dev->name);
	if (ret < 0) {
		pr_err("[SENSOR CORE] failed create meta symlink\n");
		input_unregister_device(meta_input_dev);
		input_free_device(meta_input_dev);
	}

	return ret;
}

void sensors_meta_clean(void)
{
	sensors_remove_symlink(&meta_input_dev->dev.kobj,
		meta_input_dev->name);
	input_unregister_device(meta_input_dev);
	input_free_device(meta_input_dev);
}

#ifdef CONFIG_ADSP_FACTORY

extern  struct class* get_adsp_sensor_class( void );
#endif

static int __init sensors_class_init(void)
{
	pr_info("[SENSORS CORE] sensors_class_init\n");

#ifdef CONFIG_ADSP_FACTORY
	sensors_class = get_adsp_sensor_class();
#else
	sensors_class = class_create(THIS_MODULE, "sensors");
	if (IS_ERR(sensors_class)) {
		pr_err("%s, create sensors_class is failed.(err=%ld)\n",
			__func__, IS_ERR(sensors_class));
		return PTR_ERR(sensors_class);
	}
#endif
	/* For symbolic link */
	sensors_event_class = class_create(THIS_MODULE, "sensor_event");
	if (IS_ERR(sensors_event_class)) {
		pr_err("%s, create sensors_class is failed.(err=%ld)\n",
			__func__, IS_ERR(sensors_event_class));
		class_destroy(sensors_class);
		return PTR_ERR(sensors_event_class);
	}

	symlink_dev = device_create(sensors_event_class, NULL, 0, NULL,
		"%s", "symlink");
	if (IS_ERR(symlink_dev)) {
		pr_err("[SENSORS CORE] symlink_dev create failed![%ld]\n",
			IS_ERR(symlink_dev));

		class_destroy(sensors_class);
		class_destroy(sensors_event_class);
		return PTR_ERR(symlink_dev);
	}

	/* Creating sensor_dev for flush sysfs node */
	sensors_register(sensor_dev, NULL, sensor_attr, "sensor_dev");

	atomic_set(&sensor_count, 0);
	sensors_class->dev_uevent = NULL;

	sensors_meta_init();

	return 0;
}

static void __exit sensors_class_exit(void)
{
	if (meta_input_dev)
		sensors_meta_clean();

	if (sensors_class || sensors_event_class) {
		if (sensor_dev) {
			device_destroy(sensors_class, sensor_dev->devt);
			sensor_dev = NULL;
		}
		class_destroy(sensors_class);
		sensors_class = NULL;
		class_destroy(sensors_event_class);
		sensors_event_class = NULL;
	}
}

subsys_initcall(sensors_class_init);
module_exit(sensors_class_exit);

MODULE_DESCRIPTION("Universal sensors core class");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
