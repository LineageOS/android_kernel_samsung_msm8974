/*
*  /driver/sensors/sensors_core.c
*
*  Copyright (C) 2011 Samsung Electronics Co.Ltd
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
*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#ifdef CONFIG_SENSOR_USE_SYMLINK
#include <linux/input.h>
#endif
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/adsp_sensors_core.h>

struct class *sensors_class;
EXPORT_SYMBOL_GPL(sensors_class);
#ifdef CONFIG_SENSOR_USE_SYMLINK
struct class *event_class;
EXPORT_SYMBOL_GPL(event_class);
struct device *event_dev;
EXPORT_SYMBOL_GPL(event_dev);
#endif
/**
* Create sysfs interface
*/
static void set_sensor_attr(struct device *dev,
struct device_attribute *attributes[])
{
	int i;

	for (i = 0; attributes[i] != NULL; i++) {
		if ((device_create_file(dev, attributes[i])) < 0) {
			printk(KERN_ERR"[SENSOR CORE] create_file attributes %d\n",
				i);
		}
	}
}
struct device *sensors_classdev_register(char *sensors_name)
{
	struct device *dev;
	int retval = -ENODEV;

	dev = device_create(sensors_class, NULL, 0,
		NULL, "%s", sensors_name);
	if (IS_ERR(dev))
		return ERR_PTR(retval);

	printk(KERN_INFO "Registered sensors device: %s\n", sensors_name);
	return dev;
}
EXPORT_SYMBOL_GPL(sensors_classdev_register);

/**
* sensors_classdev_unregister - unregisters a object of sensor device.
*
*/
void sensors_classdev_unregister(struct device *dev)
{
	device_unregister(dev);
}
EXPORT_SYMBOL_GPL(sensors_classdev_unregister);
#ifdef CONFIG_SENSOR_USE_SYMLINK
int sensors_initialize_symlink(struct input_dev *input_dev)
{
	int ret = 0;

	if (!event_class) {
		event_class = class_create(THIS_MODULE, "sensor_event");
		if (IS_ERR(event_class))
			return PTR_ERR(event_class);
	}

	if(!event_dev) {
		event_dev = device_create(event_class, NULL, 0, NULL, "%s", "symlink");
		if (IS_ERR(event_dev))
			return PTR_ERR(event_dev);
	}

	ret = sysfs_create_link(&event_dev->kobj,
		&input_dev->dev.kobj,
		input_dev->name);

	return ret;
}
EXPORT_SYMBOL_GPL(sensors_initialize_symlink);
void sensors_delete_symlink(struct input_dev *input_dev)
{
	if(event_dev)
		sysfs_delete_link(&event_dev->kobj,
		&input_dev->dev.kobj,
		input_dev->name);
}
EXPORT_SYMBOL_GPL(sensors_delete_symlink);
#endif
int sensors_register(struct device *dev, void *drvdata,
struct device_attribute *attributes[], char *name)
{
	int ret = 0;
	if (!sensors_class) {
		sensors_class = class_create(THIS_MODULE, "adsp");
		if (IS_ERR(sensors_class))
			return PTR_ERR(sensors_class);
	}

	dev = device_create(sensors_class, NULL, 0, drvdata, "%s", name);

	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		printk(KERN_ERR "[SENSORS CORE] device_create failed! [%d]\n",
			ret);
		return ret;
	}

	set_sensor_attr(dev, attributes);

	return 0;
}
EXPORT_SYMBOL_GPL(sensors_register);

void sensors_unregister(struct device *dev)
{
	device_unregister(dev);
}
EXPORT_SYMBOL_GPL(sensors_unregister);

static int __init sensors_class_init(void)
{
	printk(KERN_INFO"[SENSORS CORE] sensors_class_init\n");
	sensors_class = class_create(THIS_MODULE, "sensors");

	if (IS_ERR(sensors_class))
		return PTR_ERR(sensors_class);

#ifdef CONFIG_SENSOR_USE_SYMLINK
	event_class = class_create(THIS_MODULE, "sensor_event");

	if (IS_ERR(event_class))
		return PTR_ERR(event_class);

	event_dev = device_create(event_class, NULL, 0, NULL, "%s", "symlink");

	if (IS_ERR(event_dev)) {
		return PTR_ERR(event_dev);
	}
#endif
	sensors_class->dev_uevent = NULL;

	return 0;
}

static void __exit sensors_class_exit(void)
{
	class_destroy(sensors_class);
#ifdef CONFIG_SENSOR_USE_SYMLINK
	class_destroy(event_class);
	device_unregister(event_dev);
#endif
}

subsys_initcall(sensors_class_init);
module_exit(sensors_class_exit);

MODULE_DESCRIPTION("Universal sensors core class");
MODULE_AUTHOR("Ryunkyun Park <ryun.park@samsung.com>");
MODULE_LICENSE("GPL");
