/*
 *  drivers/usb/notify/usb_notify_sysfs.c
 *
 * Copyright (C) 2015 Samsung, Inc.
 * Author: Dongrak Shin <dongrak.shin@samsung.com>
 *
*/
#define pr_fmt(fmt) "usb_notify: " fmt

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/usb_notify_sysfs.h>

struct notify_data {
	struct class *usb_notify_class;
	atomic_t device_count;
};

static struct notify_data usb_notify_data;

static int is_valid_cmd(char *cur_cmd, char *prev_cmd)
{
	pr_info("%s : current state=%s, previous state=%s\n",
		__func__, cur_cmd, prev_cmd);

	if (!strcmp(cur_cmd, "ON") ||
			!strncmp(cur_cmd, "ON_ALL_", 7)) {
		if (!strcmp(prev_cmd, "ON") ||
				!strncmp(prev_cmd, "ON_ALL_", 7)) {
			goto ignore;
		} else if (!strncmp(prev_cmd, "ON_HOST_", 8)) {
			goto all;
		} else if (!strncmp(prev_cmd, "ON_CLIENT_", 10)) {
			goto all;
		} else if (!strcmp(prev_cmd, "OFF")) {
			goto all;
		} else {
			goto invalid;
		}
	} else if (!strcmp(cur_cmd, "OFF")) {
		if (!strcmp(prev_cmd, "ON") ||
				!strncmp(prev_cmd, "ON_ALL_", 7)) {
			goto off;
		} else if (!strncmp(prev_cmd, "ON_HOST_", 8)) {
			goto off;
		} else if (!strncmp(prev_cmd, "ON_CLIENT_", 10)) {
			goto off;
		} else if (!strcmp(prev_cmd, "OFF")) {
			goto ignore;
		} else {
			goto invalid;
		}
	} else if (!strncmp(cur_cmd, "ON_HOST_", 8)) {
		if (!strcmp(prev_cmd, "ON") ||
				!strncmp(prev_cmd, "ON_ALL_", 7)) {
			goto host;
		} else if (!strncmp(prev_cmd, "ON_HOST_", 8)) {
			goto ignore;
		} else if (!strncmp(prev_cmd, "ON_CLIENT_", 10)) {
			goto host;
		} else if (!strcmp(prev_cmd, "OFF")) {
			goto host;
		} else {
			goto invalid;
		}
	} else if (!strncmp(cur_cmd, "ON_CLIENT_", 10)) {
		if (!strcmp(prev_cmd, "ON") ||
				!strncmp(prev_cmd, "ON_ALL_", 7)) {
			goto client;
		} else if (!strncmp(prev_cmd, "ON_HOST_", 8)) {
			goto client;
		} else if (!strncmp(prev_cmd, "ON_CLIENT_", 10)) {
			goto ignore;
		} else if (!strcmp(prev_cmd, "OFF")) {
			goto client;
		} else {
			goto invalid;
		}
	} else {
		goto invalid;
	}
host:
	pr_err("%s cmd=%s is accepted.\n", __func__, cur_cmd);
	return NOTIFY_BLOCK_TYPE_HOST;
client:
	pr_err("%s cmd=%s is accepted.\n", __func__, cur_cmd);
	return NOTIFY_BLOCK_TYPE_CLIENT;
all:
	pr_err("%s cmd=%s is accepted.\n", __func__, cur_cmd);
	return NOTIFY_BLOCK_TYPE_ALL;
off:
	pr_err("%s cmd=%s is accepted.\n", __func__, cur_cmd);
	return NOTIFY_BLOCK_TYPE_NONE;
ignore:
	pr_err("%s cmd=%s is ignored but saved.\n", __func__, cur_cmd);
	return -EEXIST;
invalid:
	pr_err("%s cmd=%s is invalid.\n", __func__, cur_cmd);
	return -EINVAL;
}

static ssize_t disable_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	pr_info("read disable_state %s\n", udev->disable_state_cmd);
	return sprintf(buf, "%s\n", udev->disable_state_cmd);
}

static ssize_t disable_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_notify_dev *udev = (struct usb_notify_dev *)
		dev_get_drvdata(dev);

	char *disable;
	int size_ret, param = -EINVAL;
	size_t ret = -ENOMEM;

	if (size > MAX_DISABLE_STR_LEN) {
		pr_err("%s size(%zu) is too long.\n", __func__, size);
		goto error;
	}

	disable = kzalloc(size+1, GFP_KERNEL);
	if (!disable)
		goto error;

	size_ret = sscanf(buf, "%s", disable);

	if (udev->set_disable) {
		param = is_valid_cmd(disable, udev->disable_state_cmd);
		if (param == -EINVAL) {
			ret = param;
		} else {
			if (param != -EEXIST)
				udev->set_disable(udev, param);
			memset(udev->disable_state_cmd, 0,
				sizeof(udev->disable_state_cmd));
			strncpy(udev->disable_state_cmd,
				disable, strlen(disable));
			ret = size;
		}
	} else
		pr_err("set_disable func is NULL\n");
	kfree(disable);
error:
	return ret;
}

static DEVICE_ATTR(disable, 0664, disable_show, disable_store);

static struct attribute *usb_notify_attrs[] = {
	&dev_attr_disable.attr,
	NULL,
};

static struct attribute_group usb_notify_attr_grp = {
	.attrs = usb_notify_attrs,
};

static int create_usb_notify_class(void)
{
	if (!usb_notify_data.usb_notify_class) {
		usb_notify_data.usb_notify_class
			= class_create(THIS_MODULE, "usb_notify");
		if (IS_ERR(usb_notify_data.usb_notify_class))
			return PTR_ERR(usb_notify_data.usb_notify_class);
		atomic_set(&usb_notify_data.device_count, 0);
	}

	return 0;
}

int usb_notify_dev_register(struct usb_notify_dev *udev)
{
	int ret;

	if (!usb_notify_data.usb_notify_class) {
		ret = create_usb_notify_class();
		if (ret < 0)
			return ret;
	}

	udev->index = atomic_inc_return(&usb_notify_data.device_count);
	udev->dev = device_create(usb_notify_data.usb_notify_class, NULL,
		MKDEV(0, udev->index), NULL, udev->name);
	if (IS_ERR(udev->dev))
		return PTR_ERR(udev->dev);

	udev->disable_state = 0;
	strncpy(udev->disable_state_cmd, "OFF", sizeof(udev->disable_state_cmd)-1);
	ret = sysfs_create_group(&udev->dev->kobj, &usb_notify_attr_grp);
	if (ret < 0) {
		device_destroy(usb_notify_data.usb_notify_class,
				MKDEV(0, udev->index));
		return ret;
	}

	dev_set_drvdata(udev->dev, udev);
	return 0;
}
EXPORT_SYMBOL_GPL(usb_notify_dev_register);

void usb_notify_dev_unregister(struct usb_notify_dev *udev)
{
	sysfs_remove_group(&udev->dev->kobj, &usb_notify_attr_grp);
	device_destroy(usb_notify_data.usb_notify_class, MKDEV(0, udev->index));
	dev_set_drvdata(udev->dev, NULL);
}
EXPORT_SYMBOL_GPL(usb_notify_dev_unregister);

int usb_notify_class_init(void)
{
	return create_usb_notify_class();
}
EXPORT_SYMBOL_GPL(usb_notify_class_init);

void usb_notify_class_exit(void)
{
	class_destroy(usb_notify_data.usb_notify_class);
}
EXPORT_SYMBOL_GPL(usb_notify_class_exit);
