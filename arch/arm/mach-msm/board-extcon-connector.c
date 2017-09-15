/*
 * Copyright (C) 2013 Samsung Electronics Co, Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/switch.h>
#include <linux/extcon.h>
#include <linux/sec_class.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/irq.h>
#include <linux/power_supply.h>
#include <linux/regulator/machine.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#ifndef CONFIG_USB_DWC3
#include <linux/mfd/max77804k.h>
#include <linux/host_notify.h>
#endif

static int jig_state;
struct muic_cable {
	struct work_struct work;
	struct delayed_work cable_init;
	struct notifier_block nb;
	struct extcon_specific_cable_nb extcon_nb;
	struct extcon_dev *edev;
	enum extcon_cable_name cable_type;
	int cable_state;
};

int get_jig_state(void)
{
	return jig_state;
}

static struct switch_dev switch_dock = {
	.name = "dock",
};
struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);
static struct muic_cable support_cable_list[] = {
#ifndef CONFIG_USB_DWC3
	{ .cable_type = EXTCON_USB, },
	{ .cable_type = EXTCON_CHARGE_DOWNSTREAM, },
	{ .cable_type = EXTCON_USB_HOST, },
	{ .cable_type = EXTCON_USB_HOST_5V, },
#if defined (CONFIG_MUIC_MAX77804K_SUPPORT_LANHUB)
	{ .cable_type = EXTCON_LANHUB, },
	{ .cable_type = EXTCON_LANHUB_TA, },
#endif
#endif
	{ .cable_type = EXTCON_DESKDOCK, },
	{ .cable_type = EXTCON_CARDOCK, },
	{ .cable_type = EXTCON_AUDIODOCK, },
	{ .cable_type = EXTCON_SMARTDOCK, },
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
	{ .cable_type = EXTCON_HMT,	},
#endif
	{ .cable_type = EXTCON_JIG_USBON, },
	{ .cable_type = EXTCON_JIG_USBOFF, },
	{ .cable_type = EXTCON_JIG_UARTON, },
	{ .cable_type = EXTCON_JIG_UARTOFF, },
};

static void muic_cable_event_worker(struct work_struct *work)
{
	struct muic_cable *cable =
			    container_of(work, struct muic_cable, work);

	pr_info("%s: '%s' is %s\n", __func__,
			extcon_cable_name[cable->cable_type],
			cable->cable_state ? "attached" : "detached");

	switch (cable->cable_type) {
#ifndef CONFIG_USB_DWC3
	case EXTCON_USB:	/*	USB cable VBUS update*/
	case EXTCON_CHARGE_DOWNSTREAM:
		sec_otg_set_vbus_state(cable->cable_state);
		break;
	case EXTCON_USB_HOST:
		if (cable->cable_state)
			sec_otg_notify(HNOTIFY_ID);
		else
			sec_otg_notify(HNOTIFY_ID_PULL);
		break;
	case EXTCON_USB_HOST_5V:
		if (cable->cable_state)
			sec_otg_notify(HNOTIFY_OTG_POWER_ON);
		else
			sec_otg_notify(HNOTIFY_OTG_POWER_OFF);
		break;
#if defined (CONFIG_MUIC_MAX77804K_SUPPORT_LANHUB)
	case EXTCON_LANHUB:
		if (cable->cable_state)
			sec_otg_notify(HNOTIFY_LANHUB_ON);
		else
			sec_otg_notify(HNOTIFY_LANHUB_OFF);
		break;
	case EXTCON_LANHUB_TA:
		if (cable->cable_state)
			sec_otg_notify(HNOTIFY_LANHUBTA_ON);
		else
			sec_otg_notify(HNOTIFY_LANHUBTA_OFF);
		break;
#endif
#endif
	case EXTCON_DESKDOCK:	/*	Deskdock	#1	*/
		switch_set_state(&switch_dock, cable->cable_state ? 1 : 0);
		break;
	case EXTCON_CARDOCK:	/*	Cardock		#2	*/
		switch_set_state(&switch_dock, cable->cable_state ? 2 : 0);
		break;
	case EXTCON_AUDIODOCK:	/*	Audiodock	#7	*/
		switch_set_state(&switch_dock, cable->cable_state ? 7 : 0);
		break;
	case EXTCON_SMARTDOCK:	/*	Smartdock	#8	*/
		switch_set_state(&switch_dock, cable->cable_state ? 8 : 0);
		break;
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
	case EXTCON_HMT:		/*	HMT			#11	*/
		switch_set_state(&switch_dock, cable->cable_state ? 11 : 0);
		break;
#endif
	case EXTCON_JIG_USBON:
	case EXTCON_JIG_USBOFF:
	case EXTCON_JIG_UARTON:
	case EXTCON_JIG_UARTOFF:
		jig_state = cable->cable_state;
		break;
	default:
		pr_err("%s: invalid cable value (%d, %d)\n", __func__,
					cable->cable_type, cable->cable_state);
		break;
	}
}


static int muic_cable_notifier(struct notifier_block *nb,
					unsigned long stat, void *ptr)
{
	struct muic_cable *cable =
			container_of(nb, struct muic_cable, nb);

	cable->cable_state = stat;
	schedule_work(&cable->work);

	return NOTIFY_DONE;
}


static int muic_init_cable_notify(void)
{
	int i, ret;
	struct muic_cable *cable = NULL;


	for (i = 0; i < ARRAY_SIZE(support_cable_list); i++) {
		cable = &support_cable_list[i];
		INIT_WORK(&cable->work, muic_cable_event_worker);
		cable->nb.notifier_call = muic_cable_notifier;

		ret = extcon_register_interest(&cable->extcon_nb,
				EXTCON_DEV_NAME,
				extcon_cable_name[cable->cable_type],
				&cable->nb);
		if (ret)
			pr_err("%s: fail to register extcon notifier(%s, %d)\n",
				__func__, extcon_cable_name[cable->cable_type],
				ret);

		cable->edev = cable->extcon_nb.edev;
		if (!cable->edev)
			pr_err("%s: fail to get extcon device\n", __func__);
	}

	return 0;
}

static __init int muic_init_switch(void)
{
	int ret;

	pr_info("register extcon notifier for JIG and docks\n");
	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_dev)) {
		pr_err("(%s): failed to created device (switch_dev)!\n",
				__func__);
		return -ENODEV;
	}

	ret = switch_dev_register(&switch_dock);
	if (ret < 0) {
		pr_err("Failed to register dock switch. %d\n",
				ret);
		return ret;
	}
	return 0;
}
device_initcall(muic_init_switch);
late_initcall(muic_init_cable_notify);
