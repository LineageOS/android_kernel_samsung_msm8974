/*
 *  sec-switch.c
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/switch.h>
#include "devices.h"

#ifdef CONFIG_USB_SWITCH_TSU6721
#include <linux/i2c/tsu6721.h>
#endif

#ifdef CONFIG_USB_SWITCH_FSA9485
#include <linux/i2c/fsa9485.h>
#include <linux/power_supply.h>
#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif
#endif

#ifdef CONFIG_MFD_MAX77828
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/gpio_keys.h>
#include <linux/gpio_event.h>

#include <linux/regulator/machine.h>
#include <linux/regulator/max8649.h>
#include <linux/regulator/fixed.h>
#include <linux/mfd/wm8994/pdata.h>
#include <linux/mfd/max77828.h>
#include <linux/mfd/max77828-private.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/gpio.h>

#include <linux/power_supply.h>

#ifdef CONFIG_MACH_STRETTO
#include <mach/stretto-gpio.h>
#endif

#include "devices.h"

#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif
#include <linux/pm_runtime.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>

#ifdef CONFIG_JACK_MON
#include <linux/jack.h>
#endif

#endif

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI) ||\
	defined(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_H) ||\
	defined(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_V2)
#include <linux/i2c/synaptics_rmi.h>
#endif

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT224S_KS02)
#include <linux/i2c/mxt224s_ks02.h>
#endif

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1664S)
#include <linux/i2c/mxts.h>
#endif

#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCH)
#include <linux/i2c/cypress_touchkey.h>
#endif

#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY)
#include <linux/i2c/touchkey_i2c.h>
#endif

#ifdef CONFIG_MFD_MAX77828
#include <linux/sec_class.h>
#if defined (CONFIG_VIDEO_MHL_V2) || defined (CONFIG_VIDEO_MHL_SII8246)
static int MHL_Connected;
#endif
static struct switch_dev switch_dock = {
	.name = "dock",
};

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);
#endif

#if defined(CONFIG_SEC_K_PROJECT)
extern unsigned int system_rev;
static unsigned int gpio_redriver_en;
void set_redriver_power(int on)
{
		pr_info("usb: support usb 3.0 (rev: %d)\n", system_rev);
		if (on)
			gpio_set_value(gpio_redriver_en,1);
		else
			gpio_set_value(gpio_redriver_en,0);

		pr_info("usb: value of redrvEn: %d\n",
			gpio_get_value(gpio_redriver_en));
}
#endif

#ifdef SYNAPTICS_RMI_INFORM_CHARGER
struct synaptics_rmi_callbacks *charger_callbacks;
void synaptics_tsp_charger_infom(int cable_type)
{
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, cable_type);
}
void synaptics_tsp_register_callback(struct synaptics_rmi_callbacks *cb)
{
	charger_callbacks = cb;
	pr_info("%s: [synaptics] charger callback!\n", __func__);
}
#endif

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT224S_KS02)
struct tsp_callbacks *charger_callbacks;
void mxt_tsp_charger_infom(int cable_type)
{
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, cable_type);
}
void mxt_tsp_register_callback(struct tsp_callbacks *cb)
{
	charger_callbacks = cb;
	pr_info("%s: [synaptics] charger callback!\n", __func__);
}
#endif

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1664S)
struct mxt_callbacks *mxt_charger_callbacks;
void mxt_tsp_charger_infom(bool cable_type)
{
	if (mxt_charger_callbacks && mxt_charger_callbacks->inform_charger)
		mxt_charger_callbacks->inform_charger(mxt_charger_callbacks, cable_type);

	pr_info("%s: %s\n", __func__, cable_type ? "connected" : "disconnected");

}
void mxt_tsp_register_callback(struct mxt_callbacks *cb)
{
	mxt_charger_callbacks = cb;
	pr_info("%s: [mxt] charger callback!\n", __func__);
}
#endif

#ifdef TK_INFORM_CHARGER
struct touchkey_callbacks *tk_charger_callbacks;
void touchkey_charger_infom(bool en)
{
	if (tk_charger_callbacks && tk_charger_callbacks->inform_charger)
		tk_charger_callbacks->inform_charger(tk_charger_callbacks, en);
}
void touchkey_register_callback(void *cb)
{
	tk_charger_callbacks = cb;
}
#endif

#ifdef CONFIG_MFD_MAX77828
/* charger cable state */
bool is_cable_attached;
bool is_jig_attached;

static ssize_t midas_switch_show_vbus(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int i;
	struct regulator *regulator;

	regulator = regulator_get(NULL, "safeout1");
	if (IS_ERR(regulator)) {
		pr_warn("%s: fail to get regulator\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	if (regulator_is_enabled(regulator))
		i = sprintf(buf, "VBUS is enabled\n");
	else
		i = sprintf(buf, "VBUS is disabled\n");
	regulator_put(regulator);

	return i;
}

static ssize_t midas_switch_store_vbus(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int disable, ret, usb_mode;
	struct regulator *regulator;
	/* struct s3c_udc *udc = platform_get_drvdata(&s3c_device_usbgadget); */

	if (!strncmp(buf, "0", 1))
		disable = 0;
	else if (!strncmp(buf, "1", 1))
		disable = 1;
	else {
		pr_warn("%s: Wrong command\n", __func__);
		return count;
	}

	pr_info("%s: disable=%d\n", __func__, disable);
	usb_mode =
	    disable ? USB_CABLE_DETACHED_WITHOUT_NOTI : USB_CABLE_ATTACHED;
	/* ret = udc->change_usb_mode(usb_mode); */
	ret = -1;
	if (ret < 0)
		pr_err("%s: fail to change mode!!!\n", __func__);

	regulator = regulator_get(NULL, "safeout1");
	if (IS_ERR(regulator)) {
		pr_warn("%s: fail to get regulator\n", __func__);
		return count;
	}

	if (disable) {
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
	} else {
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
	}
	regulator_put(regulator);

	return count;
}

DEVICE_ATTR(disable_vbus, 0664, midas_switch_show_vbus,
	    midas_switch_store_vbus);

#ifdef CONFIG_SEC_LOCALE_KOR
void max77828_muic_usb_cb(u8 usb_mode);
struct device *usb_lock;
int is_usb_locked;
EXPORT_SYMBOL_GPL(is_usb_locked);

static ssize_t switch_show_usb_lock(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (is_usb_locked)
		return snprintf(buf, PAGE_SIZE, "USB_LOCK");
	else
		return snprintf(buf, PAGE_SIZE, "USB_UNLOCK");
}

static ssize_t switch_store_usb_lock(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int lock;
	struct power_supply *psy;

	psy = power_supply_get_by_name("dwc-usb");
	if (!psy) {
		pr_info("%s: couldn't get usb power supply\n", __func__);
		return -EINVAL;
	}

	if (!strncmp(buf, "0", 1))
		lock = 0;
	else if (!strncmp(buf, "1", 1))
		lock = 1;
	else {
		pr_warn("%s: Wrong command\n", __func__);
		return count;
	}

	pr_info("%s: lock=%d\n", __func__, lock);

	if (lock != is_usb_locked) {
		is_usb_locked = lock;

		if (lock)
			power_supply_set_present(psy, USB_CABLE_DETACHED);
	}

	return count;
}

static DEVICE_ATTR(enable, 0664,
		   switch_show_usb_lock, switch_store_usb_lock);
#endif

#ifdef CONFIG_USB_HOST_NOTIFY
struct booster_data sec_booster = {
	.name = "max77828",
	.boost = muic_otg_control,
};
#endif

static int __init midas_sec_switch_init(void)
{
	int ret;
	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");

	if (IS_ERR(switch_dev)) {
		pr_err("Failed to create device(switch)!\n");
		goto err;
	}

	ret = device_create_file(switch_dev, &dev_attr_disable_vbus);
	if (ret) {
		pr_err("Failed to create device file(disable_vbus)!\n");
		goto err;
	}

#ifdef CONFIG_SEC_LOCALE_KOR
	usb_lock = device_create(sec_class, switch_dev,
				MKDEV(0, 0), NULL, ".usb_lock");

	if (IS_ERR(usb_lock))
		pr_err("Failed to create device (usb_lock)!\n");

	if (device_create_file(usb_lock, &dev_attr_enable) < 0)
		pr_err("Failed to create device file(.usblock/enable)!\n");
#endif

#ifdef CONFIG_USB_HOST_NOTIFY
	sec_otg_register_booster(&sec_booster);
#endif

	return 0;

err:
	return -1;
};

int current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
extern int poweroff_charging;
int max77828_muic_charger_cb(enum cable_type_muic cable_type)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;
	static enum cable_type_muic previous_cable_type = CABLE_TYPE_NONE_MUIC;
	pr_info("%s: cable type : %d\n", __func__, cable_type);

#ifdef SYNAPTICS_RMI_INFORM_CHARGER
	synaptics_tsp_charger_infom(cable_type);
#endif

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT224S_KS02)
	mxt_tsp_charger_infom(cable_type);
#endif

#ifdef TK_INFORM_CHARGER
	touchkey_charger_infom(cable_type);
#endif

#ifdef CONFIG_JACK_MON
	switch (cable_type) {
	case CABLE_TYPE_OTG_MUIC:
	case CABLE_TYPE_NONE_MUIC:
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
	case CABLE_TYPE_MHL_MUIC:
		is_cable_attached = false;
		break;
	case CABLE_TYPE_USB_MUIC:
		value.intval = POWER_SUPPLY_TYPE_USB;
	case CABLE_TYPE_JIG_USB_OFF_MUIC:
	case CABLE_TYPE_JIG_USB_ON_MUIC:
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		is_cable_attached = true;
		break;
	case CABLE_TYPE_MHL_VB_MUIC:
		is_cable_attached = true;
		break;
	case CABLE_TYPE_AUDIODOCK_MUIC:
	case CABLE_TYPE_TA_MUIC:
//ssong. test.
	case CABLE_TYPE_HV_TA_MUIC:

	case CABLE_TYPE_CARDOCK_MUIC:
	case CABLE_TYPE_DESKDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
	case CABLE_TYPE_INCOMPATIBLE_MUIC:
		is_cable_attached = true;
		break;
	default:
		pr_err("%s: invalid type:%d\n", __func__, cable_type);
		return -EINVAL;
	}
#endif

	/*  charger setting */
	if (previous_cable_type == cable_type) {
		pr_info("%s: SKIP cable setting\n", __func__);
		goto skip;
	}
	previous_cable_type = cable_type;

	switch (cable_type) {
	case CABLE_TYPE_NONE_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
		break;

	case CABLE_TYPE_MHL_VB_MUIC:
		if(poweroff_charging)
			current_cable_type = POWER_SUPPLY_TYPE_USB;
		else
			goto skip;
		break;
	case CABLE_TYPE_MHL_MUIC:
		if(poweroff_charging)
			current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
		else
			goto skip;
		break;
	case CABLE_TYPE_USB_MUIC:
	case CABLE_TYPE_JIG_USB_OFF_MUIC:
	case CABLE_TYPE_JIG_USB_ON_MUIC:
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_USB;
		break;
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_UARTOFF;
		break;
	case CABLE_TYPE_TA_MUIC:
	//ssong. test.
	case CABLE_TYPE_HV_TA_MUIC:

		current_cable_type = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_CDP_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_USB_CDP;
		break;
	case CABLE_TYPE_AUDIODOCK_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_MISC;
		break;
	case CABLE_TYPE_CARDOCK_MUIC:
	case CABLE_TYPE_DESKDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_OTG_MUIC:
		goto skip;
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
		break;
	case CABLE_TYPE_INCOMPATIBLE_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_UNKNOWN;
		break;
	default:
		pr_err("%s: invalid type for charger:%d\n",
				__func__, cable_type);
		goto skip;
	}

	pr_err("%s: [ssong] current_cable_type=0x%x\n", __func__, current_cable_type);

	if (!psy || !psy->set_property)
		pr_err("%s: fail to get battery psy\n", __func__);
	else {
		//ssong. test. value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
		value.intval = current_cable_type;
		pr_err("%s: [ssong] battery set_property(0x%x)\n", __func__, value.intval);

		psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	}
skip:
#ifdef CONFIG_JACK_MON
	jack_event_handler("charger", is_cable_attached);
#endif

	return 0;
}

int max77828_get_jig_state(void)
{
	pr_info("%s: %d\n", __func__, is_jig_attached);
	return is_jig_attached;
}
EXPORT_SYMBOL(max77828_get_jig_state);

void max77828_set_jig_state(int jig_state)
{
	pr_info("%s: %d\n", __func__, jig_state);
	is_jig_attached = jig_state;
}

int max77823_muic_set_safeout(int path);

extern void set_ncm_ready(bool);
#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_F_PROJECT)
extern unsigned int system_rev;
#define GPIO_REDRIVER_EN 129
extern u8 usb30en;
void set_redriver_power(int on)
{
	if (system_rev >= 5) {
		pr_info("usb: support usb 3.0 (rev: %d)\n", system_rev);
		if (on)
			gpio_set_value(GPIO_REDRIVER_EN,1);
		else
			gpio_set_value(GPIO_REDRIVER_EN,0);

		pr_info("usb: value of redrvEn: %d\n",
			gpio_get_value(GPIO_REDRIVER_EN));

	} else
		pr_info("usb: Can't support usb 3.0 (rev: %d)\n", system_rev);
}
#endif

/* usb cable call back function */
void max77828_muic_usb_cb(u8 usb_mode)
{
	struct power_supply *psy;

	psy = power_supply_get_by_name("dwc-usb");
	if (!psy) {
		pr_info("%s: couldn't get usb power supply\n", __func__);
		return;
	}

    pr_info("%s: MUIC attached: %d\n", __func__, usb_mode);

#ifdef CONFIG_SEC_LOCALE_KOR
	if (is_usb_locked) {
		pr_info("%s: usb locked by mdm\n", __func__);
		return;
	}
#endif

	if (usb_mode == USB_CABLE_DETACHED
		|| usb_mode == USB_CABLE_ATTACHED) {
#if defined(CONFIG_SEC_K_PROJECT)
                set_redriver_power(usb_mode);
#endif
		if (usb_mode == USB_CABLE_ATTACHED)
			max77823_muic_set_safeout(AP_USB_MODE);
#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_F_PROJECT)
			set_redriver_power(usb_mode);
#endif
		pr_info("usb: dwc3 power supply set(%d)", usb_mode);
		power_supply_set_present(psy, usb_mode);
		if (usb_mode == USB_CABLE_DETACHED) {
			set_ncm_ready(0);
#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_F_PROJECT)
			usb30en = 0;
			set_redriver_power(usb_mode);
#endif
		}
#ifdef CONFIG_USB_HOST_NOTIFY
	} else if (usb_mode == USB_OTGHOST_DETACHED
		|| usb_mode == USB_OTGHOST_ATTACHED) {

		if (usb_mode == USB_OTGHOST_DETACHED) {
			pr_info("USB Host detached");
			sec_otg_notify(HNOTIFY_ID_PULL);
		} else {
			pr_info("USB Host attached");
			sec_otg_notify(HNOTIFY_ID);
		}

	} else if (usb_mode == USB_POWERED_HOST_DETACHED
		|| usb_mode == USB_POWERED_HOST_ATTACHED) {
		if (usb_mode == USB_POWERED_HOST_DETACHED){
			pr_info("USB Host HNOTIFY_SMARTDOCK_OFF");
			sec_otg_notify(HNOTIFY_SMARTDOCK_OFF);
		}else{
			pr_info("USB Host HNOTIFY_SMARTDOCK_ON");
			sec_otg_notify(HNOTIFY_SMARTDOCK_ON);
		}
#endif
	}
}
#if defined (CONFIG_VIDEO_MHL_V2) || defined (CONFIG_VIDEO_MHL_SII8246)
static BLOCKING_NOTIFIER_HEAD(acc_mhl_notifier);
int acc_register_notifier(struct notifier_block *nb)
{
	int ret;
	ret = blocking_notifier_chain_register(&acc_mhl_notifier, nb);
	return ret;
}

int acc_unregister_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&acc_mhl_notifier, nb);
}

static int acc_notify(int event)
{
	int ret;
	ret = blocking_notifier_call_chain(&acc_mhl_notifier, event, NULL);
	return ret;
}
#endif

/*extern void MHL_On(bool on);*/
void max77828_muic_mhl_cb(int attached)
{
	pr_info("%s: MUIC attached: %d\n", __func__, attached);
	if (attached == MAX77828_MUIC_ATTACHED) {
		/*MHL_On(1);*/ /* GPIO_LEVEL_HIGH */
		pr_info("MHL Attached !!\n");
#if defined (CONFIG_VIDEO_MHL_V2) || defined (CONFIG_VIDEO_MHL_SII8246)
		if (!MHL_Connected) {
			acc_notify(1);
			MHL_Connected = 1;
		} else {
			pr_info("MHL Attached but ignored!!\n");
		}
#endif
	} else {
		/*MHL_On(0);*/ /* GPIO_LEVEL_LOW */
		pr_info("MHL Detached !!\n");
#if defined (CONFIG_VIDEO_MHL_V2) || defined (CONFIG_VIDEO_MHL_SII8246)
		acc_notify(0);
		MHL_Connected = 0;
#endif
	}
}

bool max77828_muic_is_mhl_attached(void)
{
	return 0;
}

void max77828_muic_dock_cb(int type)
{
	pr_info("%s: MUIC attached: %d\n", __func__, type);

#ifdef CONFIG_JACK_MON
	jack_event_handler("cradle", type);
#endif
	switch_set_state(&switch_dock, type);
}

void max77828_muic_init_cb(void)
{
	int ret;

	/* for CarDock, DeskDock */
	ret = switch_dev_register(&switch_dock);

	pr_info("%s: MUIC ret=%d\n", __func__, ret);

	if (ret < 0)
		pr_err("Failed to register dock switch. %d\n", ret);
#if defined(CONFIG_SEC_K_PROJECT)
        /* set gpio to enable redriver for USB3.0 */
        if(system_rev>=4)
                gpio_redriver_en = 312;
        else
                gpio_redriver_en = 129;

        gpio_tlmm_config(GPIO_CFG(gpio_redriver_en, 0, GPIO_CFG_OUTPUT,
                                        GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
        gpio_set_value(gpio_redriver_en,0);
#endif


#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_F_PROJECT)
	/* set gpio to enable redriver for USB3.0 */
	gpio_tlmm_config(GPIO_CFG(GPIO_REDRIVER_EN, 0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_set_value(GPIO_REDRIVER_EN,0);
#endif

}

#ifdef CONFIG_USB_HOST_NOTIFY
int max77828_muic_host_notify_cb(int enable)
{
	int ret = 0;
	sec_otg_notify(enable ? HNOTIFY_OTG_POWER_ON : HNOTIFY_OTG_POWER_OFF);
	ret = sec_get_notification(HNOTIFY_MODE);
	pr_info("%s: host_notify mode : %d\n", __func__, ret);
	return ret;
}
#endif

int max77823_muic_set_safeout(int path)
{
	struct regulator *regulator;

	pr_info("%s: MUIC safeout path=%d\n", __func__, path);

	if (path == CP_USB_MODE) {
		regulator = regulator_get(NULL, "safeout1");
		if (IS_ERR(regulator))
			return -ENODEV;
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);

		regulator = regulator_get(NULL, "safeout2");
		if (IS_ERR(regulator))
			return -ENODEV;
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
		regulator_put(regulator);
	} else {
		/* AP_USB_MODE || AUDIO_MODE */
		regulator = regulator_get(NULL, "safeout1");
		if (IS_ERR(regulator))
			return -ENODEV;
		if (!regulator_is_enabled(regulator))
			regulator_enable(regulator);
		regulator_put(regulator);

		regulator = regulator_get(NULL, "safeout2");
		if (IS_ERR(regulator))
			return -ENODEV;
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);
	}

	return 0;
}

struct max77828_muic_data max77828_muic = {
	.usb_cb = max77828_muic_usb_cb,
	.charger_cb = max77828_muic_charger_cb,
	.mhl_cb = max77828_muic_mhl_cb,
	.is_mhl_attached = max77828_muic_is_mhl_attached,
	.set_safeout = max77823_muic_set_safeout,
	.init_cb = max77828_muic_init_cb,
	.dock_cb = max77828_muic_dock_cb,
#ifdef CONFIG_USB_HOST_NOTIFY
	.host_notify_cb = max77828_muic_host_notify_cb,
#else
	.host_notify_cb = NULL,
#endif
	.gpio_usb_sel = -1,
	.jig_state = max77828_set_jig_state,
};

device_initcall(midas_sec_switch_init);
#endif


#ifdef CONFIG_USB_SWITCH_TSU6721

static enum cable_type_t set_cable_status;
int msm8930_get_cable_status(void) {return (int)set_cable_status; }

void muic_callback(enum cable_type_t cable_type, int state)
{
	set_cable_status = state ? cable_type : CABLE_TYPE_NONE;
		switch (cable_type) {
	case CABLE_TYPE_USB:
		pr_info("%s USB Cable is %s\n",
			__func__, state ? "attached" : "detached");
		break;
	case CABLE_TYPE_AC:
		pr_info("%s Charger is %s\n",
			__func__, state ? "attached" : "detached");
		break;
	case CABLE_TYPE_UARTOFF:
		pr_info("%s Uart is %s\n",
			__func__, state ? "attached" : "detached");
		break;
	case CABLE_TYPE_JIG:
		pr_info("%s Jig is %s\n",
			__func__, state ? "attached" : "detached");
		return;
	case CABLE_TYPE_CDP:
		pr_info("%s USB CDP is %s\n",
			__func__, state ? "attached" : "detached");
		break;
	case CABLE_TYPE_OTG:
		pr_info("%s OTG is %s\n",
			__func__, state ? "attached" : "detached");
		return;
	case CABLE_TYPE_AUDIO_DOCK:
		pr_info("%s Audiodock is %s\n",
			__func__, state ? "attached" : "detached");
		return;
	case CABLE_TYPE_CARDOCK:
		pr_info("%s Cardock is %s\n",
			__func__, state ? "attached" : "detached");
		break;
	case CABLE_TYPE_INCOMPATIBLE:
		pr_info("%s Incompatible Charger is %s\n",
			__func__, state ? "attached" : "detached");
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(muic_callback);

#endif

#ifdef CONFIG_USB_SWITCH_FSA9485

static enum cable_type_t set_cable_status;
int current_cable_type = POWER_SUPPLY_TYPE_BATTERY;

#ifdef CONFIG_VIDEO_MHL_V2
static BLOCKING_NOTIFIER_HEAD(acc_mhl_notifier);
int acc_register_notifier(struct notifier_block *nb)
{
	int ret;
	ret = blocking_notifier_chain_register(&acc_mhl_notifier, nb);
	return ret;
}

int acc_unregister_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&acc_mhl_notifier, nb);
}

static int acc_notify(int event)
{
	int ret;
	ret = blocking_notifier_call_chain(&acc_mhl_notifier, event, NULL);
	return ret;
}

static void fsa9485_muic_mhl_cb(bool attached)
{
	pr_info("%s : attached_status (%d)\n", __func__, attached);
	acc_notify(attached);
	return;
}
#endif

#ifdef CONFIG_MHL_NEW_CBUS_MSC_CMD
static void fsa9485_mhl_cb(bool attached, int mhl_charge)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_mhl_cb attached (%d), mhl_charge(%d)\n",
			attached, mhl_charge);

	if (attached) {
		switch (mhl_charge) {
		case 0:
		case 1:
			set_cable_status = CABLE_TYPE_USB;
			break;
		case 2:
			set_cable_status = CABLE_TYPE_AC;
			break;
		}
	} else {
		set_cable_status = CABLE_TYPE_NONE;
	}

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_USB:
		value.intval = POWER_SUPPLY_TYPE_USB;
		break;
	case CABLE_TYPE_AC:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("%s: invalid cable :%d\n", __func__, set_cable_status);
		return;
	}

	current_cable_type = value.intval;

	value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}
#else
static void fsa9485_mhl_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_mhl_cb attached %d\n", attached);
	set_cable_status = attached ? CABLE_TYPE_MISC : CABLE_TYPE_NONE;

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_MISC:
		value.intval = POWER_SUPPLY_TYPE_MISC;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("%s: invalid cable :%d\n", __func__, set_cable_status);
		return;
	}

	current_cable_type = value.intval;

	value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}
#endif

bool fsa9485_muic_is_mhl_attached(void)
{
	return 0;
}

static void fsa9485_otg_cb(bool attached)
{
#ifdef CONFIG_USB_HOST_NOTIFY
	static bool is_active;
	if (is_active == attached) {
		pr_info("fsa9485_otg_cb attached %d duplicated, skip\n", attached);
		return;
	}
	is_active = attached;
#endif
#if 0
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;
#endif

	pr_info("fsa9485_otg_cb attached %d\n", attached);

#if 0
	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	if (attached)
		current_cable_type = POWER_SUPPLY_TYPE_OTG;
	else
		current_cable_type = POWER_SUPPLY_TYPE_BATTERY;

	value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
#endif
#ifdef CONFIG_USB_HOST_NOTIFY
	if (attached) {
		pr_info("%s USB Host attached", __func__);
		sec_otg_notify(HNOTIFY_ID);
	} else {
		pr_info("%s USB Host detached", __func__);
		sec_otg_notify(HNOTIFY_ID_PULL);
	}
#endif
}

#if defined(CONFIG_MACH_VIENNAEUR) || defined(CONFIG_MACH_VIENNAVZW) || defined(CONFIG_MACH_V2LTEEUR) || defined(CONFIG_MACH_VIENNAKOR)
extern int vienna_usb_rdrv_pin;
#endif

static void fsa9485_usb_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_usb_cb attached %d\n", attached);
	set_cable_status = attached ? CABLE_TYPE_USB : CABLE_TYPE_NONE;

        /* Vienna SS - USB 3.0 redriver enable/disable */
#if defined(CONFIG_MACH_VIENNAEUR) || defined(CONFIG_MACH_VIENNAVZW) || defined(CONFIG_MACH_V2LTEEUR) || defined(CONFIG_MACH_VIENNAKOR)
	gpio_set_value(vienna_usb_rdrv_pin, attached);
	pr_info("%s vienna_usb_rdrv_pin = %d, enable=%d\n",
		__func__,
		vienna_usb_rdrv_pin,
		attached);
#endif

	psy = power_supply_get_by_name("dwc-usb");
	if (!psy) {
		pr_info("%s: couldn't get usb power supply\n", __func__);
		return;
	}
	pr_info("%s: MUIC attached: %d\n", __func__, attached);

	power_supply_set_present(psy, attached);

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1664S)
	mxt_tsp_charger_infom(attached);
#endif

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_USB:
		value.intval = POWER_SUPPLY_TYPE_USB;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("%s: invalid cable :%d\n", __func__, set_cable_status);
		return;
	}

	current_cable_type = value.intval;

	value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}

static void fsa9485_charger_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("%s attached %d\n", __func__, attached);
	set_cable_status = attached ? CABLE_TYPE_AC : CABLE_TYPE_NONE;

//	msm_otg_set_charging_state(attached);

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1664S)
	mxt_tsp_charger_infom(attached);
#endif

#ifdef CONFIG_TOUCHSCREEN_MMS144
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, attached);
#endif

	switch (set_cable_status) {
	case CABLE_TYPE_AC:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}

	current_cable_type = value.intval;

	value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}

static void fsa9485_uart_cb(bool attached)
{
	pr_info("fsa9485_uart_cb attached %d\n", attached);

	set_cable_status = attached ? CABLE_TYPE_UARTOFF : CABLE_TYPE_NONE;
}

static void fsa9485_jig_cb(bool attached)
{
	pr_info("fsa9485_jig_cb attached %d\n", attached);

	set_cable_status = attached ? CABLE_TYPE_JIG : CABLE_TYPE_NONE;
}

static void fsa9485_usb_cdp_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_usb_cdp_cb attached %d\n", attached);

	set_cable_status =
		attached ? CABLE_TYPE_CDP : CABLE_TYPE_NONE;

	psy = power_supply_get_by_name("dwc-usb");
	if (!psy) {
		pr_info("%s: couldn't get usb power supply\n", __func__);
		return;
	}
	pr_info("%s: MUIC attached: %d\n", __func__, attached);

	power_supply_set_present(psy, attached);


	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_CDP:
		value.intval = POWER_SUPPLY_TYPE_USB_CDP;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}

	current_cable_type = value.intval;

	value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}

static struct switch_dev switch_dock = {
	.name = "dock",
};

static void fsa9485_dock_cb(int attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9480_dock_cb attached %d\n", attached);
	switch_set_state(&switch_dock, attached);

	set_cable_status = attached ? CABLE_TYPE_CARDOCK : CABLE_TYPE_NONE;

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_CARDOCK:
		value.intval = POWER_SUPPLY_TYPE_CARDOCK;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}

	current_cable_type = value.intval;

	value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}

static void fsa9485_lanhub_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_lanhub_cb attached %d\n", attached);

	set_cable_status =
		attached ? CABLE_TYPE_LANHUB : CABLE_TYPE_NONE;

	pr_info("%s:fsa9485 cable type : %d", __func__, set_cable_status);

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

#ifdef CONFIG_USB_HOST_NOTIFY
	if (attached){
		pr_info("USB Host HNOTIFY_LANHUB_ON");
		sec_otg_notify(HNOTIFY_SMARTDOCK_ON);
	}else{
		pr_info("USB Host HNOTIFY_LANHUB_OFF");
		sec_otg_notify(HNOTIFY_SMARTDOCK_OFF);
	}
#endif

	switch (set_cable_status) {
	case CABLE_TYPE_LANHUB:
		value.intval = POWER_SUPPLY_TYPE_LAN_HUB;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_OTG;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}

	current_cable_type = value.intval;

	value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}

//	msm_otg_set_smartdock_state(attached);
}

static void fsa9485_smartdock_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_smartdock_cb attached %d\n", attached);

	set_cable_status =
		attached ? CABLE_TYPE_SMART_DOCK : CABLE_TYPE_NONE;

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_SMART_DOCK:
		value.intval = POWER_SUPPLY_TYPE_USB_CDP;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}

	current_cable_type = value.intval;

	value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}

//	msm_otg_set_smartdock_state(attached);
}

static void fsa9485_audio_dock_cb(bool attached)
{
	pr_info("fsa9485_audio_dock_cb attached %d\n", attached);

//	msm_otg_set_smartdock_state(attached);
}

static int fsa9485_dock_init(void)
{
	int ret;

	/* for CarDock, DeskDock */
	ret = switch_dev_register(&switch_dock);
	if (ret < 0) {
		pr_err("Failed to register dock switch. %d\n", ret);
		return ret;
	}
	return 0;
}

int msm8974_get_cable_type(void)
{
#ifdef CONFIG_WIRELESS_CHARGING
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return 0;
	}
#endif

	pr_info("cable type (%d) -----\n", set_cable_status);

	if (set_cable_status != CABLE_TYPE_NONE) {
		switch (set_cable_status) {
		case CABLE_TYPE_MISC:
#ifdef CONFIG_MHL_NEW_CBUS_MSC_CMD
			fsa9485_mhl_cb(1 , 0);
#else
			fsa9485_mhl_cb(1);
#endif
			break;
		case CABLE_TYPE_USB:
			fsa9485_usb_cb(1);
			break;
		case CABLE_TYPE_AC:
			fsa9485_charger_cb(1);
			break;
#ifdef CONFIG_WIRELESS_CHARGING
		case CABLE_TYPE_WPC:
			value.intval = POWER_SUPPLY_TYPE_WPC;
			current_cable_type = value.intval;

			value.intval = current_cable_type<<ONLINE_TYPE_MAIN_SHIFT;
			ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
			break;
#endif
		default:
			pr_err("invalid status:%d\n", set_cable_status);
			break;
		}
	}

	return current_cable_type;
}

struct fsa9485_platform_data fsa9485_pdata = {
	.otg_cb		= fsa9485_otg_cb,
	.usb_cb		= fsa9485_usb_cb,
	.charger_cb	= fsa9485_charger_cb,
	.uart_cb	= fsa9485_uart_cb,
	.jig_cb		= fsa9485_jig_cb,
	.dock_cb	= fsa9485_dock_cb,
	.dock_init	= fsa9485_dock_init,
	.usb_cdp_cb	= fsa9485_usb_cdp_cb,
	.lanhub_cb	= fsa9485_lanhub_cb,
	.smartdock_cb	= fsa9485_smartdock_cb,
	.audio_dock_cb	= fsa9485_audio_dock_cb,
	.mhl_cb = fsa9485_muic_mhl_cb,
};

#endif

