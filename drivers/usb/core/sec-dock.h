/*
 * drivers/usb/core/sec-dock.h
 *
 * Copyright (C) 2013 Samsung Electronics
 * Author: Woo-kwang Lee <wookwang.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/power_supply.h>
#ifdef CONFIG_USB_SWITCH_FSA9485
#include <linux/i2c/fsa9485.h>
#endif

#define PSY_CHG_NAME "battery"
#define SMARTDOCK_INDEX	1
#define MMDOCK_INDEX		2

struct dev_table {
	struct usb_device_id dev;
	int index;
};

static struct dev_table enable_notify_hub_table[] = {
	{ .dev = { USB_DEVICE(0x0424, 0x2514), },
	   .index = SMARTDOCK_INDEX,
	}, /* SMART DOCK HUB 1 */
	{ .dev = { USB_DEVICE(0x1a40, 0x0101), },
	   .index = SMARTDOCK_INDEX,
	}, /* SMART DOCK HUB 2 */
	{ .dev = { USB_DEVICE(0x0424, 0x9512), },
	   .index = MMDOCK_INDEX,
	}, /* SMSC USB LAN HUB 9512 */
	{}
};

static struct dev_table essential_device_table[] = {
	{ .dev = { USB_DEVICE(0x08bb, 0x2704), },
	   .index = SMARTDOCK_INDEX,
	}, /* TI USB Audio DAC 1 */
	{ .dev = { USB_DEVICE(0x08bb, 0x27c4), },
	   .index = SMARTDOCK_INDEX,
	}, /* TI USB Audio DAC 2 */
	{ .dev = { USB_DEVICE(0x0424, 0xec00), },
	   .index = MMDOCK_INDEX,
	}, /* SMSC LAN Driver */
	{}
};

#ifdef CONFIG_USB_SWITCH_FSA9485
/* real battery driver notification function */
static void set_online(int host_state)
{
	struct power_supply *psy = power_supply_get_by_name(PSY_CHG_NAME);
	union power_supply_propval value;

	if (!psy) {
		pr_err("%s: fail to get %s psy\n", __func__, PSY_CHG_NAME);
		return;
	}
	if (host_state)
		value.intval = POWER_SUPPLY_TYPE_SMART_OTG;
	else
		value.intval = POWER_SUPPLY_TYPE_SMART_NOTG;

	psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	return;
}
#endif

#ifdef CONFIG_USB_SWITCH_FSA9485
/* real battery driver notification function for mmdock */
static void set_online_mmdock(int host_state)
{
	struct power_supply *psy = power_supply_get_by_name(PSY_CHG_NAME);
	union power_supply_propval value;

	if (!psy) {
		pr_err("%s: fail to get %s psy\n", __func__, PSY_CHG_NAME);
		return;
	}
	if (host_state)
		value.intval = POWER_SUPPLY_TYPE_SMART_OTG;
	else
		value.intval = POWER_SUPPLY_TYPE_SMART_NOTG;

	psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	return;
}
#endif

static int check_essential_device(struct usb_device *dev, int index)
{
	struct dev_table *id;
	int ret = 0;

	/* check VID, PID */
	for (id = essential_device_table; id->dev.match_flags; id++) {
		if ((id->dev.match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
				(id->dev.match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
				id->dev.idVendor == le16_to_cpu(dev->descriptor.idVendor) &&
				id->dev.idProduct == le16_to_cpu(dev->descriptor.idProduct) &&
				id->index == index) {
			ret = 1;
			break;
		}
	}
	return ret;
}

static int is_notify_hub(struct usb_device *dev)
{
	struct dev_table *id;
	struct usb_device *hdev;
	int ret = 0;

	hdev = dev->parent;
	if (!hdev)
		goto skip;
	/* check VID, PID */
	for (id = enable_notify_hub_table; id->dev.match_flags; id++) {
		if ((id->dev.match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
				(id->dev.match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
				id->dev.idVendor == le16_to_cpu(hdev->descriptor.idVendor) &&
				id->dev.idProduct == le16_to_cpu(hdev->descriptor.idProduct)) {
			ret = (hdev->parent
					&& (hdev->parent == dev->bus->root_hub)) ? id->index : 0;
			break;
		}
	}
skip:
	return ret;
}

static int call_battery_notify(struct usb_device *dev, bool bOnOff)
{
	struct usb_device *hdev;
	struct usb_device *udev;
	int index = 0;
	int count = 0;
	int port;

	index = is_notify_hub(dev);
	if (!index)
		goto skip;
	if (check_essential_device(dev, index))
		goto skip;

	hdev = dev->parent;
	for (port = 1; port <= hdev->maxchild; port++) {
		udev = hdev->children [port-1];
		if (udev) {
			if (!check_essential_device(udev, index)) {
				if (!bOnOff && (udev == dev))
					continue;
				else
					count++;
			}
		}
	}

	pr_info("%s : VID : 0x%x, PID : 0x%x, bOnOff=%d, count=%d\n", __func__,
		dev->descriptor.idVendor, dev->descriptor.idProduct,
			bOnOff, count);
#ifdef CONFIG_USB_SWITCH_FSA9485
	if (bOnOff) {
		if (count == 1) {
			if (check_mmdock_connect()) {
				if (index == SMARTDOCK_INDEX) {
					set_online(1);
					pr_info("%s : request smartdock charging current = 1000mA\n", __func__);
				} else if (index == MMDOCK_INDEX) {
					set_online_mmdock(1);
					pr_info("%s : request mmdock charging current = 900mA\n", __func__);
				}
			}
		}
	} else {
		if (!count) {
			if (check_mmdock_connect()) {
				if (index == SMARTDOCK_INDEX) {
					set_online(0);
					pr_info("%s : request smartdock charging current = 1700mA\n", __func__);
				} else if (index == MMDOCK_INDEX) {
					set_online_mmdock(0);
					pr_info("%s : request mmdock charging current = 1400mA\n", __func__);
				}
			}
		}
	}
#endif
skip:
	return 0;
}

