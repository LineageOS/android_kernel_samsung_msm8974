/*
 * cyttsp5_bus.c
 * Cypress TrueTouch(TM) Standard Product V5 Bus Driver.
 * For use with Cypress Txx5xx parts.
 * Supported parts include:
 * TMA5XX
 *
 * Copyright (C) 2012-2013 Cypress Semiconductor
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * Author: Aleksej Makarov aleksej.makarov@sonyericsson.com
 * Modified by: Cypress Semiconductor for complete set of TTSP Bus interfaces.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contact Cypress Semiconductor at www.cypress.com <ttdrivers@cypress.com>
 *
 */

#include <linux/cyttsp5_bus.h>

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/limits.h>

static DEFINE_MUTEX(core_lock);
static LIST_HEAD(adapter_list);
static LIST_HEAD(core_dev_list);
static LIST_HEAD(cyttsp5_dev_list);

struct bus_type cyttsp5_bus_type;

static void cyttsp5_dev_release(struct device *dev)
{
	put_device(dev->parent);
}

static struct device_type cyttsp5_dev_type = {
	.release = cyttsp5_dev_release
};

static struct device_type cyttsp5_core_type = {
	.release = cyttsp5_dev_release
};

static void cyttsp5_initialize_device(struct cyttsp5_device *dev,
		struct cyttsp5_device_info const *dev_info)
{
	dev->name = dev_info->name;
	dev->core_id = dev_info->core_id;
	dev->dev.platform_data = dev_info->platform_data;
}

static void _cyttsp5_reinitialize_device(struct cyttsp5_device *dev)
{
	void *platform_data = dev->dev.platform_data;

	memset(&dev->dev, 0, sizeof(dev->dev));
	dev->dev.platform_data = platform_data;
	dev->core = NULL;
}

static void cyttsp5_initialize_core(struct cyttsp5_core *core,
		struct cyttsp5_core_info const *core_info)
{
	core->name = core_info->name;
	core->id = core_info->id;
	core->adap_id = core_info->adap_id;
	core->dev.platform_data = core_info->platform_data;
}

static void _cyttsp5_reinitialize_core(struct cyttsp5_core *core)
{
	void *platform_data = core->dev.platform_data;

	memset(&core->dev, 0, sizeof(core->dev));
	core->dev.platform_data = platform_data;
	core->adap = NULL;
}

static int _cyttsp5_register_dev(struct cyttsp5_device *pdev,
		struct cyttsp5_core *core)
{
	int ret;

	/* Check if the device is registered with the system */
	if (device_is_registered(&pdev->dev))
		return -EEXIST;

	pdev->core = core;
	pdev->dev.parent = get_device(&core->dev);
	pdev->dev.bus = &cyttsp5_bus_type;
	pdev->dev.type = &cyttsp5_dev_type;
	dev_set_name(&pdev->dev, "%s.%s", pdev->name,  core->id);

	ret = device_register(&pdev->dev);
	dev_dbg(&pdev->dev,
		"%s: Registering device '%s'. Parent at '%s', err = %d\n",
		 __func__, dev_name(&pdev->dev),
		 dev_name(pdev->dev.parent), ret);
	if (ret) {
		dev_err(&pdev->dev, "%s: failed to register device, err %d\n",
			__func__, ret);
		pdev->core = NULL;
	}
	return ret;
}

static void _cyttsp5_unregister_dev(struct cyttsp5_device *pdev)
{
	/* Check if the device is registered with the system */
	if (!device_is_registered(&pdev->dev))
		return;

	dev_dbg(&pdev->dev, "%s: Unregistering device '%s'.\n",
		__func__, dev_name(&pdev->dev));
	device_unregister(&pdev->dev);
}

static int _cyttsp5_register_core(struct cyttsp5_core *pdev,
		struct cyttsp5_adapter *adap)
{
	int ret;

	/* Check if the device is registered with the system */
	if (device_is_registered(&pdev->dev))
		return -EEXIST;

	pdev->adap = adap;
	pdev->dev.parent = get_device(adap->dev);
	pdev->dev.bus = &cyttsp5_bus_type;
	pdev->dev.type = &cyttsp5_core_type;
	dev_set_name(&pdev->dev, "%s.%s", pdev->id,  adap->id);

	ret = device_register(&pdev->dev);
	dev_dbg(&pdev->dev,
		"%s: Registering device '%s'. Parent at '%s', err = %d\n",
		 __func__, dev_name(&pdev->dev),
		 dev_name(pdev->dev.parent), ret);
	if (ret) {
		dev_err(&pdev->dev, "%s: failed to register device, err %d\n",
			__func__, ret);
		pdev->adap = NULL;
	}
	return ret;
}

static void _cyttsp5_unregister_core(struct cyttsp5_core *pdev)
{
	/* Check if the core is registered with the system */
	if (!device_is_registered(&pdev->dev))
		return;

	dev_dbg(&pdev->dev, "%s: Unregistering core '%s'.\n",
		__func__, dev_name(&pdev->dev));
	device_unregister(&pdev->dev);
}

static void _cyttsp5_unregister_and_reinitialize_devices(
		struct cyttsp5_core *core)
{
	struct cyttsp5_device *dev;

	list_for_each_entry(dev, &cyttsp5_dev_list, node)
		if (dev->core == core) {
			_cyttsp5_unregister_dev(dev);
			_cyttsp5_reinitialize_device(dev);
		}
}

static struct cyttsp5_adapter *find_adapter(char const *adap_id)
{
	struct cyttsp5_adapter *a;

	list_for_each_entry(a, &adapter_list, node)
		if (!strncmp(a->id, adap_id, NAME_MAX))
			return a;
	return NULL;
}

int cyttsp5_add_adapter(char const *id, struct cyttsp5_ops const *ops,
		struct device *parent)
{
	int rc = 0;
	struct cyttsp5_adapter *a;

	if (!parent) {
		dev_err(parent, "%s: need parent for '%s'\n", __func__, id);
		return -EINVAL;
	}
	mutex_lock(&core_lock);
	if (find_adapter(id)) {
		dev_err(parent, "%s: adapter '%s' already exists\n",
				__func__, id);
		rc = -EEXIST;
		goto fail;
	}
	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a) {
		dev_err(parent, "%s: failed to allocate adapter '%s'\n",
				__func__, id);
		rc = -ENOMEM;
		goto fail;
	}
	memcpy(a->id, id, sizeof(a->id));
	a->id[sizeof(a->id) - 1] = 0;
	a->read_default = ops->read_default;
	a->read_default_nosize = ops->read_default_nosize;
	a->write_read_specific = ops->write_read_specific;
	a->dev = parent;
	list_add(&a->node, &adapter_list);
	dev_dbg(parent, "%s: '%s' added to adapter_list\n", __func__, id);
	rescan_cores(a);
fail:
	mutex_unlock(&core_lock);
	return rc;
}

int cyttsp5_del_adapter(char const *id)
{
	int rc = 0;
	struct cyttsp5_adapter *adap;
	struct cyttsp5_core *core;

	mutex_lock(&core_lock);
	adap = find_adapter(id);
	if (!adap) {
		pr_err("%s: adapter '%s' does not exist\n",
			__func__, id);
		rc = -ENODEV;
		goto fail;
	}

	/* Unregister core and devices linked to this adapter
	 * This is to prevent core and devices get probed until
	 * their corresponding adapter is re-added
	 */
	list_for_each_entry(core, &core_dev_list, node) {
		if (core->adap != adap)
			continue;
		_cyttsp5_unregister_and_reinitialize_devices(core);
		_cyttsp5_unregister_core(core);
		_cyttsp5_reinitialize_core(core);
	}

	list_del(&adap->node);
	kfree(adap);
	pr_debug("%s: '%s' removed from adapter_list\n", __func__, id);
fail:
	mutex_unlock(&core_lock);
	return rc;
}


