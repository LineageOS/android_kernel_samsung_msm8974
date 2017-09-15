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
#include <linux/power_supply.h>

#if defined(CONFIG_USB_SWITCH_TSU6721) || defined(CONFIG_USB_SWITCH_RT8973) || defined(CONFIG_SM5502_MUIC)
#include <linux/i2c/tsu6721.h>
#define DEBUG_STATUS		1
#endif

#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif

#ifdef CONFIG_USB_SWITCH_FSA9485
#include <linux/i2c/fsa9485.h>
#include <linux/power_supply.h>
#endif

#if defined(CONFIG_USB_SWITCH_RT8973)
#include <linux/platform_data/rt8973.h>
#endif

#if defined(CONFIG_SM5502_MUIC)
#include <linux/i2c/sm5502.h>
#endif

#if defined(CONFIG_SM5504_MUIC)
#include <linux/i2c/sm5504.h>
#define DEBUG_STATUS    1
#endif

#define BATT_SEARCH_CNT_MAX	10
#if defined(CONFIG_MFD_MAX77803) || defined(CONFIG_MFD_MAX77888) \
	|| defined(CONFIG_MFD_MAX77804K)
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/gpio_keys.h>
#include <linux/gpio_event.h>

#include <linux/regulator/machine.h>
#include <linux/regulator/max8649.h>
#include <linux/regulator/fixed.h>
#include <linux/mfd/wm8994/pdata.h>
#if defined(CONFIG_MFD_MAX77803)
#include <linux/mfd/max77803.h>
#include <linux/mfd/max77803-private.h>
#elif defined(CONFIG_MFD_MAX77804K)
#include <linux/mfd/max77804k.h>
#include <linux/mfd/max77804k-private.h>
#elif defined(CONFIG_MFD_MAX77888)
#include <linux/mfd/max77888.h>
#include <linux/mfd/max77888-private.h>
#endif
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/gpio.h>

#include <linux/power_supply.h>

#include <linux/switch.h>

#ifdef CONFIG_MACH_STRETTO
#include <mach/stretto-gpio.h>
#endif

#include "devices.h"

#include <linux/pm_runtime.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>

#ifdef CONFIG_JACK_MON
#include <linux/jack.h>
#endif

#endif

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI) ||\
	defined(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_G) ||\
	defined(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_H)
#include <linux/i2c/synaptics_rmi.h>
#endif

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT224S_KS02)
#include <linux/i2c/mxt224s_ks02.h>
#endif

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1664S)
#include <linux/i2c/mxts.h>
#endif

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1188S)
#include <linux/i2c/mxts_1188s.h>
#endif

#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCH) || defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY_KS01)
#include <linux/i2c/cypress_touchkey.h>
#endif

#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY)
#include <linux/i2c/touchkey_i2c.h>
#endif

#if defined(CONFIG_MFD_MAX77803) || defined(CONFIG_MFD_MAX77888) \
	|| defined(CONFIG_MFD_MAX77804K)
#include <linux/sec_class.h>
#if defined (CONFIG_VIDEO_MHL_V2) || defined (CONFIG_VIDEO_MHL_SII8246)
static int MHL_Connected;
#endif
#ifdef CONFIG_TOUCHSCREEN_FTS
#include <linux/i2c/fts.h>
#endif

#ifdef CONFIG_KEYBOARD_CYPRESS_TKEY_HL
#include <linux/i2c/touchkey_hl.h>
#endif
static struct switch_dev switch_dock = {
	.name = "dock",
};

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);
#endif

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_BT532)
extern void bt532_charger_status_cb(int status);
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

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1664S) || defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1188S)
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

#if defined(CONFIG_TOUCHSCREEN_MMS252) || defined(CONFIG_TOUCHSCREEN_MMS300)
struct tsp_callbacks *charger_callbacks;
struct tsp_callbacks {
        void (*inform_charger)(struct tsp_callbacks *tsp_cb, bool mode);
};

void melfas_register_callback(void *cbv)
{
	struct tsp_callbacks *cb = cbv;
	charger_callbacks = cb;
	pr_debug("[TSP] melfas_register_callback\n");
}
EXPORT_SYMBOL(melfas_register_callback);
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

#ifdef FTS_SUPPORT_TA_MODE
struct fts_callbacks *fts_charger_callbacks;
void fts_charger_infom(int cable_type)
{
	if (fts_charger_callbacks && fts_charger_callbacks->inform_charger)
		fts_charger_callbacks->inform_charger(fts_charger_callbacks, cable_type);
}
void fts_register_callback(void *cb)
{
	fts_charger_callbacks = cb;
}
#endif

#if defined(CONFIG_MFD_MAX77803)
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
void max77803_muic_usb_cb(u8 usb_mode);
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
	.name = "max77803",
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
		goto err_create_file;
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

err_create_file:
	device_destroy(sec_class,switch_dev->devt);
err:
	return -1;
};

int current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
extern int poweroff_charging;
int max77803_muic_charger_cb(enum cable_type_muic cable_type)
{
#ifdef CONFIG_CHARGER_MAX77803
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct power_supply *psy_ps = power_supply_get_by_name("ps");
	union power_supply_propval value;
	static enum cable_type_muic previous_cable_type = CABLE_TYPE_NONE_MUIC;
#endif
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

#ifdef FTS_SUPPORT_TA_MODE
	fts_charger_infom(cable_type);
#endif

#ifdef CONFIG_JACK_MON
	switch (cable_type) {
	case CABLE_TYPE_OTG_MUIC:
	case CABLE_TYPE_NONE_MUIC:
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
	case CABLE_TYPE_MHL_MUIC:
	case CABLE_TYPE_CHARGING_CABLE_MUIC:
		is_cable_attached = false;
		break;
	case CABLE_TYPE_USB_MUIC:
#ifdef CONFIG_CHARGER_MAX77803
		value.intval = POWER_SUPPLY_TYPE_USB;
#endif
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

#ifdef CONFIG_CHARGER_MAX77803
	pr_info("%s: cable type for charger: cable_type(%d), previous_cable_type(%d)\n",
			__func__, cable_type, previous_cable_type);
	/*  charger setting */
	if (previous_cable_type == cable_type) {
		pr_info("%s: SKIP cable setting\n", __func__);
		goto skip;
	}

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
#ifdef CONFIG_CONTROL_OTG_POPUP
		current_cable_type = POWER_SUPPLY_TYPE_OTG;
		break;
#else
		goto skip;
#endif
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
		break;
	case CABLE_TYPE_INCOMPATIBLE_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_UNKNOWN;
		break;
	case CABLE_TYPE_CHARGING_CABLE_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_POWER_SHARING;
		break;
	default:
		pr_err("%s: invalid type for charger:%d\n",
				__func__, cable_type);
		goto skip;
	}

	if (!psy || !psy->set_property || !psy_ps || !psy_ps->set_property) {
		pr_err("%s: fail to get battery/ps psy\n", __func__);
	} else {
		if (current_cable_type == POWER_SUPPLY_TYPE_POWER_SHARING) {
			value.intval = current_cable_type;
			psy_ps->set_property(psy_ps, POWER_SUPPLY_PROP_ONLINE, &value);
		} else {
			if (previous_cable_type == CABLE_TYPE_CHARGING_CABLE_MUIC) {
				value.intval = current_cable_type;
				psy_ps->set_property(psy_ps, POWER_SUPPLY_PROP_ONLINE, &value);
			} else {
				value.intval = current_cable_type;
				psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
			}
		}
	}
	previous_cable_type = cable_type;
#endif
skip:
#ifdef CONFIG_JACK_MON
	jack_event_handler("charger", is_cable_attached);
#endif

	return 0;
}

int max77803_get_jig_state(void)
{
	pr_info("%s: %d\n", __func__, is_jig_attached);
	return is_jig_attached;
}
EXPORT_SYMBOL(max77803_get_jig_state);

void max77803_set_jig_state(int jig_state)
{
	pr_info("%s: %d\n", __func__, jig_state);
	is_jig_attached = jig_state;
}

int max77803_muic_set_safeout(int path);

extern void set_ncm_ready(bool);
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
#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_F_PROJECT)
extern unsigned int system_rev;
#define GPIO_REDRIVER_EN 129
#define GPIO_REDRIVER_EN_HSGLTE_REV04 52
extern u8 usb30en;
void set_redriver_power(int on)
{
unsigned int redriver_en_gpio = GPIO_REDRIVER_EN;

#if defined(CONFIG_MACH_HLTE_CHN_CMCC) || defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
	if (system_rev >= 12) {
		redriver_en_gpio = GPIO_REDRIVER_EN_HSGLTE_REV04;
	}
	else	{
		redriver_en_gpio = GPIO_REDRIVER_EN;
	}
#endif
	if (system_rev >= 5) {
		pr_info("usb: support usb 3.0 (rev: %d)\n", system_rev);
		if (on)
			gpio_set_value(redriver_en_gpio,1);
		else
			gpio_set_value(redriver_en_gpio,0);

		pr_info("usb: value of redrvEn: %d\n",
			gpio_get_value(redriver_en_gpio));

	} else
		pr_info("usb: Can't support usb 3.0 (rev: %d)\n", system_rev);
}
#endif
#if defined(CONFIG_MACH_JACTIVESKT)
void set_redriver_power(int on)
{
    return;
}
#endif
/* usb cable call back function */
void max77803_muic_usb_cb(u8 usb_mode)
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
			max77803_muic_set_safeout(AP_USB_MODE);
#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_F_PROJECT)
			set_redriver_power(usb_mode);
#endif
		pr_info("usb: dwc3 power supply set(%d)", usb_mode);
		power_supply_set_present(psy, usb_mode);
		if (usb_mode == USB_CABLE_DETACHED) {
#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_F_PROJECT)
			set_ncm_ready(0);
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
void max77803_muic_mhl_cb(int attached)
{
	pr_info("%s: MUIC attached: %d\n", __func__, attached);
	if (attached == MAX77803_MUIC_ATTACHED) {
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

bool max77803_muic_is_mhl_attached(void)
{
	return 0;
}

void max77803_muic_dock_cb(int type)
{
	pr_info("%s: MUIC attached: %d\n", __func__, type);

#ifdef CONFIG_JACK_MON
	jack_event_handler("cradle", type);
#endif
	switch_set_state(&switch_dock, type);
}

void max77803_muic_init_cb(void)
{
	int ret;
#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_F_PROJECT)
	unsigned int redriver_en_gpio = GPIO_REDRIVER_EN;

#if defined(CONFIG_MACH_HLTE_CHN_CMCC) || defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
	if (system_rev >= 12) {
		redriver_en_gpio = GPIO_REDRIVER_EN_HSGLTE_REV04;
	}
	else	{
		redriver_en_gpio = GPIO_REDRIVER_EN;
	}
#endif
#endif
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
	gpio_tlmm_config(GPIO_CFG(redriver_en_gpio, 0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_set_value(redriver_en_gpio,0);
#endif

}

#ifdef CONFIG_USB_HOST_NOTIFY
int max77803_muic_host_notify_cb(int enable)
{
	int ret = 0;
	sec_otg_notify(enable ? HNOTIFY_OTG_POWER_ON : HNOTIFY_OTG_POWER_OFF);
	ret = sec_get_notification(HNOTIFY_MODE);
	pr_info("%s: host_notify mode : %d\n", __func__, ret);
	return ret;
}
#endif

int max77803_muic_set_safeout(int path)
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

struct max77803_muic_data max77803_muic = {
	.usb_cb = max77803_muic_usb_cb,
	.charger_cb = max77803_muic_charger_cb,
	.mhl_cb = max77803_muic_mhl_cb,
	.is_mhl_attached = max77803_muic_is_mhl_attached,
	.set_safeout = max77803_muic_set_safeout,
	.init_cb = max77803_muic_init_cb,
	.dock_cb = max77803_muic_dock_cb,
#ifdef CONFIG_USB_HOST_NOTIFY
	.host_notify_cb = max77803_muic_host_notify_cb,
#else
	.host_notify_cb = NULL,
#endif
	.gpio_usb_sel = -1,
	.jig_state = max77803_set_jig_state,
};

device_initcall(midas_sec_switch_init);
#endif
#if defined(CONFIG_MFD_MAX77804K)
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
void max77804k_muic_usb_cb(u8 usb_mode);
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
	.name = "max77804k",
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
int max77804k_muic_charger_cb(enum cable_type_muic cable_type)
{
#ifdef CONFIG_CHARGER_MAX77804K
	struct power_supply *psy = power_supply_get_by_name("battery");
	union power_supply_propval value;
	static enum cable_type_muic previous_cable_type = CABLE_TYPE_NONE_MUIC;
#endif
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
#ifdef CONFIG_CHARGER_MAX77804K
		value.intval = POWER_SUPPLY_TYPE_USB;
#endif
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

#ifdef CONFIG_CHARGER_MAX77804K
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

	if (!psy || !psy->set_property)
		pr_err("%s: fail to get battery psy\n", __func__);
	else {
		value.intval = current_cable_type;
		psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	}
skip:
#endif
#ifdef CONFIG_JACK_MON
	jack_event_handler("charger", is_cable_attached);
#endif

	return 0;
}

int max77804k_get_jig_state(void)
{
	pr_info("%s: %d\n", __func__, is_jig_attached);
	return is_jig_attached;
}
EXPORT_SYMBOL(max77804k_get_jig_state);

void max77804k_set_jig_state(int jig_state)
{
	pr_info("%s: %d\n", __func__, jig_state);
	is_jig_attached = jig_state;
}

int max77804k_muic_set_safeout(int path);

extern void set_ncm_ready(bool);
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

#if defined CONFIG_SEC_RUBENS_PROJECT
extern void sec_otg_set_vbus_state(int);
#endif
/* usb cable call back function */
void max77804k_muic_usb_cb(u8 usb_mode)
{
	struct power_supply *psy;
	pr_info("%s: MUIC attached: %d\n", __func__, usb_mode);

#if defined CONFIG_SEC_RUBENS_PROJECT
	if (usb_mode == USB_CABLE_DETACHED || usb_mode == USB_CABLE_ATTACHED){
		pr_info("msm_otg_set vbus_state(%d) \n",usb_mode);
	}

	sec_otg_set_vbus_state(usb_mode);
#endif

	psy = power_supply_get_by_name("dwc-usb");
	if (!psy) {
		pr_info("%s: couldn't get usb power supply\n", __func__);
#if defined CONFIG_SEC_RUBENS_PROJECT
		//TODO:remove comments once battery/fuel gauge is up
		//return;
#else
		return;
#endif
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
			max77804k_muic_set_safeout(AP_USB_MODE);

		pr_info("usb: dwc3 power supply set(%d)", usb_mode);
#if defined CONFIG_SEC_RUBENS_PROJECT
		//TODO: remove comments once battery/fuel gauge is up
		//power_supply_set_present(psy, usb_mode);
#else
		//Enabled for K-project
		power_supply_set_present(psy, usb_mode);
#endif
		if (usb_mode == USB_CABLE_DETACHED) {
			//set_ncm_ready(0);
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
void max77804k_muic_mhl_cb(int attached)
{
	pr_info("%s: MUIC attached: %d\n", __func__, attached);
	if (attached == MAX77804K_MUIC_ATTACHED) {
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

bool max77804k_muic_is_mhl_attached(void)
{
	return 0;
}

void max77804k_muic_dock_cb(int type)
{
	pr_info("%s: MUIC attached: %d\n", __func__, type);

#ifdef CONFIG_JACK_MON
	jack_event_handler("cradle", type);
#endif
	switch_set_state(&switch_dock, type);
}

void max77804k_muic_init_cb(void)
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

}

#ifdef CONFIG_USB_HOST_NOTIFY
int max77804k_muic_host_notify_cb(int enable)
{
	int ret = 0;
	sec_otg_notify(enable ? HNOTIFY_OTG_POWER_ON : HNOTIFY_OTG_POWER_OFF);
	ret = sec_get_notification(HNOTIFY_MODE);
	pr_info("%s: host_notify mode : %d\n", __func__, ret);
	return ret;
}
#endif

int max77804k_muic_set_safeout(int path)
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
#if defined CONFIG_SEC_RUBENS_PROJECT
//TODO - remove comments once battery/fuel gauge is up
#if 0
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
#endif
#else
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
#endif
	}

	return 0;
}

struct max77804k_muic_data max77804k_muic = {
	.usb_cb = max77804k_muic_usb_cb,
	.charger_cb = max77804k_muic_charger_cb,
	.mhl_cb = max77804k_muic_mhl_cb,
	.is_mhl_attached = max77804k_muic_is_mhl_attached,
	.set_safeout = max77804k_muic_set_safeout,
	.init_cb = max77804k_muic_init_cb,
	.dock_cb = max77804k_muic_dock_cb,
#ifdef CONFIG_USB_HOST_NOTIFY
	.host_notify_cb = max77804k_muic_host_notify_cb,
#else
	.host_notify_cb = NULL,
#endif
	.gpio_usb_sel = -1,
	.jig_state = max77804k_set_jig_state,
};

device_initcall(midas_sec_switch_init);
#endif

#if defined(CONFIG_MFD_MAX77888)
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
#if defined(CONFIG_MFD_MAX77888)
void max77888_muic_usb_cb(u8 usb_mode);
#endif
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
	.name = "max77888",
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
int max77888_muic_charger_cb(enum cable_type_muic cable_type)
{
#ifdef CONFIG_CHARGER_MAX77888
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct power_supply *psy_ps = power_supply_get_by_name("ps");
	union power_supply_propval value;
	static enum cable_type_muic previous_cable_type = CABLE_TYPE_NONE_MUIC;
#endif
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
	case CABLE_TYPE_CHARGING_CABLE_MUIC:
		is_cable_attached = false;
		break;
	case CABLE_TYPE_USB_MUIC:
#ifdef CONFIG_CHARGER_MAX77888
		value.intval = POWER_SUPPLY_TYPE_USB;
#endif
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
	case CABLE_TYPE_LANHUB_MUIC:
	case CABLE_TYPE_CARDOCK_MUIC:
	case CABLE_TYPE_DESKDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
	case CABLE_TYPE_MMDOCK_MUIC:
	case CABLE_TYPE_INCOMPATIBLE_MUIC:
		is_cable_attached = true;
		break;
	default:
		pr_err("%s: invalid type:%d\n", __func__, cable_type);
		return -EINVAL;
	}
#endif

#ifdef CONFIG_CHARGER_MAX77888
	pr_info("%s: cable type for charger: cable_type(%d), previous_cable_type(%d)\n",
			__func__, cable_type, previous_cable_type);

	/*  charger setting */
	if (previous_cable_type == cable_type) {
		pr_info("%s: SKIP cable setting\n", __func__);
		goto skip;
	}

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
		current_cable_type = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_CDP_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_USB_CDP;
		break;
	case CABLE_TYPE_LANHUB_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_MAINS;
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
	case CABLE_TYPE_CHARGING_CABLE_MUIC:
		current_cable_type = POWER_SUPPLY_TYPE_POWER_SHARING;
		break;
	case CABLE_TYPE_MMDOCK_MUIC:
		return 0;
	default:
		pr_err("%s: invalid type for charger:%d\n",
				__func__, cable_type);
		goto skip;
	}

	if (!psy || !psy->set_property || !psy_ps || !psy_ps->set_property) {
		pr_err("%s: fail to get battery/ps psy\n", __func__);
	} else {
		if (current_cable_type == POWER_SUPPLY_TYPE_POWER_SHARING) {
			value.intval = current_cable_type;
			psy_ps->set_property(psy_ps, POWER_SUPPLY_PROP_ONLINE, &value);
		} else {
			if (previous_cable_type == CABLE_TYPE_CHARGING_CABLE_MUIC) {
				value.intval = current_cable_type;
				psy_ps->set_property(psy_ps, POWER_SUPPLY_PROP_ONLINE, &value);
			} else {
				value.intval = current_cable_type;
				psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
			}
		}
	}
	previous_cable_type = cable_type;
#endif
skip:
#ifdef CONFIG_JACK_MON
	jack_event_handler("charger", is_cable_attached);
#endif

	return 0;
}

int max77888_get_jig_state(void)
{
	pr_info("%s: %d\n", __func__, is_jig_attached);
	return is_jig_attached;
}
EXPORT_SYMBOL(max77888_get_jig_state);

void max77888_set_jig_state(int jig_state)
{
	pr_info("%s: %d\n", __func__, jig_state);
	is_jig_attached = jig_state;
}

int max77888_muic_set_safeout(int path);

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
void max77888_muic_usb_cb(u8 usb_mode)
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
		if (usb_mode == USB_CABLE_ATTACHED)
			max77888_muic_set_safeout(AP_USB_MODE);
#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_F_PROJECT)
			set_redriver_power(usb_mode);
#endif
		pr_info("usb: dwc3 power supply set(%d)", usb_mode);
		power_supply_set_present(psy, usb_mode);
		if (usb_mode == USB_CABLE_DETACHED) {
			//set_ncm_ready(0);
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
		if (usb_mode == USB_POWERED_HOST_DETACHED) {
			pr_info("USB Host HNOTIFY_SMARTDOCK_OFF");
			sec_otg_notify(HNOTIFY_SMARTDOCK_OFF);
		} else {
			pr_info("USB Host HNOTIFY_SMARTDOCK_ON");
			sec_otg_notify(HNOTIFY_SMARTDOCK_ON);
		}
	} else if (usb_mode == USB_LANHUB_DETACHED
		|| usb_mode == USB_LANHUB_ATTACHED) {
		if (usb_mode == USB_LANHUB_DETACHED) {
			pr_info("USB Host HNOTIFY_LANHUB_OFF");
			sec_otg_notify(HNOTIFY_LANHUB_OFF);
		} else {
			pr_info("USB Host HNOTIFY_LANHUB_ON");
			sec_otg_notify(HNOTIFY_LANHUB_ON);
		}
	}
#endif
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
void max77888_muic_mhl_cb(int attached)
{
	pr_info("%s: MUIC attached: %d\n", __func__, attached);
	if (attached == MAX77888_MUIC_ATTACHED) {
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

bool max77888_muic_is_mhl_attached(void)
{
	return 0;
}

void max77888_muic_dock_cb(int type)
{
	pr_info("%s: MUIC attached: %d\n", __func__, type);

#ifdef CONFIG_JACK_MON
	jack_event_handler("cradle", type);
#endif
	switch_set_state(&switch_dock, type);
}

void max77888_muic_init_cb(void)
{
	int ret;

	/* for CarDock, DeskDock */
	ret = switch_dev_register(&switch_dock);

	pr_info("%s: MUIC ret=%d\n", __func__, ret);

	if (ret < 0)
		pr_err("Failed to register dock switch. %d\n", ret);

#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_F_PROJECT)
	/* set gpio to enable redriver for USB3.0 */
	gpio_tlmm_config(GPIO_CFG(GPIO_REDRIVER_EN, 0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_set_value(GPIO_REDRIVER_EN,0);
#endif

}

#ifdef CONFIG_USB_HOST_NOTIFY
int max77888_muic_host_notify_cb(int enable)
{
	int ret = 0;
	sec_otg_notify(enable ? HNOTIFY_OTG_POWER_ON : HNOTIFY_OTG_POWER_OFF);
	ret = sec_get_notification(HNOTIFY_MODE);
	pr_info("%s: host_notify mode : %d\n", __func__, ret);
	return ret;
}
#endif

int max77888_muic_set_safeout(int path)
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

struct max77888_muic_data max77888_muic = {
	.usb_cb = max77888_muic_usb_cb,
	.charger_cb = max77888_muic_charger_cb,
	.mhl_cb = max77888_muic_mhl_cb,
	.is_mhl_attached = max77888_muic_is_mhl_attached,
	.set_safeout = max77888_muic_set_safeout,
	.init_cb = max77888_muic_init_cb,
	.dock_cb = max77888_muic_dock_cb,
#ifdef CONFIG_USB_HOST_NOTIFY
	.host_notify_cb = max77888_muic_host_notify_cb,
#else
	.host_notify_cb = NULL,
#endif
	.gpio_usb_sel = -1,
	.jig_state = max77888_set_jig_state,
};

device_initcall(midas_sec_switch_init);
#endif
/* Adding callback support for tsu6721 MUIC and SM5502 MUIC also */
#if defined(CONFIG_USB_SWITCH_TSU6721) || defined(CONFIG_USB_SWITCH_RT8973) || defined(CONFIG_SM5502_MUIC)  || defined(CONFIG_SM5504_MUIC)
#include <linux/switch.h>

int current_cable_type = POWER_SUPPLY_TYPE_BATTERY;

struct switch_dev switch_dock = {
      .name = "dock",
};

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);
EXPORT_SYMBOL(switch_dock);

extern void sec_otg_set_vbus_state(int);

static enum cable_type_t set_cable_status;
int msm8930_get_cable_status(void) {return (int)set_cable_status; }

/* support for LPM charging */
//#ifdef CONFIG_SAMSUNG_LPM_MODE
#if defined(CONFIG_BATTERY_SAMSUNG) || defined(CONFIG_QPNP_SEC_CHARGER)
bool sec_bat_is_lpm(void)
{
	return (bool)poweroff_charging;
}
#endif

int sec_bat_get_cable_status(void)
{
	int rc;
	struct power_supply *psy;
	union power_supply_propval value;

	psy = power_supply_get_by_name("battery");
	switch (set_cable_status) {
	case CABLE_TYPE_MISC:
		value.intval = POWER_SUPPLY_TYPE_MISC;
		break;
	case CABLE_TYPE_USB:
		value.intval = POWER_SUPPLY_TYPE_USB;
		break;
	case CABLE_TYPE_AC:
	case CABLE_TYPE_AUDIO_DOCK:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_UARTOFF:
		value.intval = POWER_SUPPLY_TYPE_UARTOFF;
		break;
	case CABLE_TYPE_CARDOCK:
		value.intval = POWER_SUPPLY_TYPE_CARDOCK;
		break;
	case CABLE_TYPE_CDP:
		value.intval = POWER_SUPPLY_TYPE_USB_CDP;
		break;
	case CABLE_TYPE_INCOMPATIBLE:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_DESK_DOCK:
		value.intval = POWER_SUPPLY_TYPE_MISC;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
	if (set_cable_status == CABLE_TYPE_UARTOFF)
	value.intval = POWER_SUPPLY_TYPE_UARTOFF;
		break;
	default:
                pr_err("%s: LPM boot with invalid cable :%d\n", __func__,set_cable_status);
	}

	rc = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,&value);

	if (rc) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, rc);
        }

	return (int)set_cable_status;
}
//#endif

#endif

#if defined(CONFIG_USB_SWITCH_TSU6721) || defined(CONFIG_USB_SWITCH_RT8973) || defined(CONFIG_SM5502_MUIC)
void tsu6721_oxp_callback(int state)
{
#if 0 //ovp stub-implemented on completion
	bool ovp_state;
	if (state == 1) {
		ovp_state = true;
		/*ENABLE*/
	} else if (state == 0) {
		ovp_state = false;
		/*DISABLE*/
	}
#endif
}

int tsu6721_dock_init(void)
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
#if defined(DEBUG_STATUS)
static int status_count;
#endif
void tsu6721_callback(enum cable_type_t cable_type, int attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	//Initialize power supply to battery, till power supply decided.
	struct power_supply *psy = power_supply_get_by_name("battery");
	static enum cable_type_t previous_cable_type = CABLE_TYPE_NONE;

	pr_info("%s, called : cable_type :%d \n",__func__, cable_type);
#if defined(CONFIG_TOUCHSCREEN_MXTS) ||defined(CONFIG_TOUCHSCREEN_MXT224E)
        if (charger_callbacks && charger_callbacks->inform_charger)
                charger_callbacks->inform_charger(charger_callbacks,
                attached);
#endif

	set_cable_status = attached ? cable_type : CABLE_TYPE_NONE;

	switch (cable_type) {
	case CABLE_TYPE_USB:

#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s USB Cable status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s USB Cable Status detached (%d) \n", __func__,status_count);
               }
#endif
		sec_otg_set_vbus_state(attached);
		break;
	case CABLE_TYPE_AC:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Charger status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s charger status detached (%d) \n", __func__,status_count);
               }
#endif
		break;
	case CABLE_TYPE_UARTOFF:
	case CABLE_TYPE_JIG_UART_OFF_VB:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s UART Status attached (%d), VBUS: %s\n",__func__,
					status_count,((cable_type == CABLE_TYPE_UARTOFF ? "No": "Yes")));
               } else {
                       status_count = status_count-1;
                       pr_err("%s UART status detached (%d), VBUS: %s\n", __func__,
					status_count,((cable_type == CABLE_TYPE_UARTOFF ? "No": "Yes")));
               }
#endif
		break;
	case CABLE_TYPE_JIG:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s JIG cable status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s JIG cable status detached (%d) \n", __func__,status_count);
               }
#endif
		return;
	case CABLE_TYPE_CDP:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s CDP status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s CDP Status detached (%d) \n", __func__,status_count);
               }
#endif
		sec_otg_set_vbus_state(attached);
		break;
	case CABLE_TYPE_OTG:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s OTG status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s OTG status detached (%d) \n", __func__,status_count);
               }
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
		sec_otg_notify(attached ? HNOTIFY_ID : HNOTIFY_ID_PULL);
#endif

	       return;
	case CABLE_TYPE_AUDIO_DOCK:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Audiodock status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Audiodock status detached (%d) \n", __func__,status_count);
               }
#endif
		return;
	case CABLE_TYPE_CARDOCK:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Cardock status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Cardock status detached (%d) \n", __func__,status_count);
               }
#endif
		switch_set_state(&switch_dock, attached ? 2 : 0);
		break;
	case CABLE_TYPE_DESK_DOCK:
	case CABLE_TYPE_DESK_DOCK_NO_VB:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Deskdock %s status attached (%d) \n",__func__,
				((cable_type == CABLE_TYPE_DESK_DOCK)? "VBUS" : "NO Vbus"),status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Deskdock %s status detached (%d) \n", __func__,
				((cable_type == CABLE_TYPE_DESK_DOCK)? "VBUS" : "NO Vbus"),status_count);
               }
#endif
		//skip set state for Just TA detach-with target docked.
		if (cable_type != CABLE_TYPE_DESK_DOCK_NO_VB)
			switch_set_state(&switch_dock, attached);
		break;
	case CABLE_TYPE_INCOMPATIBLE:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Incompatible Charger status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Incomabtible Charger status detached (%d) \n", __func__,status_count);
               }
#endif
		break;
	default:
		break;
	}

	if (previous_cable_type == set_cable_status) {
		pr_info("%s: SKIP cable setting\n", __func__);
		return;
	}
	previous_cable_type = set_cable_status;

	for (i = 0; i < BATT_SEARCH_CNT_MAX; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == BATT_SEARCH_CNT_MAX) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}
	if (psy == NULL || psy->set_property == NULL) {
		pr_err("%s: battery ps doesn't support set_property()\n",
				__func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_MISC:
		value.intval = POWER_SUPPLY_TYPE_MISC;
		break;
	case CABLE_TYPE_USB:
		value.intval = POWER_SUPPLY_TYPE_USB;
		break;
	case CABLE_TYPE_AC:
	case CABLE_TYPE_AUDIO_DOCK:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_CARDOCK:
		value.intval = POWER_SUPPLY_TYPE_CARDOCK;
		break;
	case CABLE_TYPE_CDP:
		value.intval = POWER_SUPPLY_TYPE_USB_CDP;
		break;
	case CABLE_TYPE_INCOMPATIBLE:
		value.intval = POWER_SUPPLY_TYPE_UNKNOWN;
		break;
	case CABLE_TYPE_DESK_DOCK:
		value.intval = POWER_SUPPLY_TYPE_MISC;
		break;
        case CABLE_TYPE_JIG_UART_OFF_VB:
                value.intval = POWER_SUPPLY_TYPE_UARTOFF;
                break;
	case CABLE_TYPE_JIG:
	case CABLE_TYPE_DESK_DOCK_NO_VB:
	case CABLE_TYPE_UARTOFF:
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("%s: invalid cable :%d\n", __func__, set_cable_status);
		return;
	}
#if defined(CONFIG_MACH_CRATERTD_CHN_3G)
	current_cable_type = value.intval;
#endif
	pr_info("%s setting cable type(%d)\n",__func__, value.intval);

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	if (ret) {
                pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
                        __func__, ret);
        }
}

struct tsu6721_platform_data tsu6721_pdata = {
	.callback = tsu6721_callback,
	.dock_init = tsu6721_dock_init,
	.oxp_callback = tsu6721_oxp_callback,
	.mhl_sel = NULL,

};

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

#if defined(CONFIG_USB_SWITCH_RT8973)
//extern sec_battery_platform_data_t sec_battery_pdata;

//unsigned int lpcharge;
//EXPORT_SYMBOL(lpcharge);

//int current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
//EXPORT_SYMBOL(current_cable_type);

u8 attached_cable;

int rt8973_dock_init(void)
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

#ifdef MUIC_SUPPORT_CARDOCK_FUNCTION
void rt8973_jig_callback(jig_type_t type, uint8_t attached)
{
	//Check JIG cable type and whether dock finished the initialization or not (rt8973_dock_init)
	if (type == JIG_UART_BOOT_ON && switch_dock.dev != NULL) {
		if (attached)
			pr_err("%s Cardock status attached \n",__func__);
		else
			pr_err("%s Cardock status detached \n", __func__);

		switch_set_state(&switch_dock, attached ? 2 : 0);
	}
	return;
}
#endif

void sec_charger_cb(u8 cable_type)
{
	union power_supply_propval value;
	struct power_supply *psy = power_supply_get_by_name("battery");
    pr_info("%s: cable type (0x%02x)\n", __func__, cable_type);
	attached_cable = cable_type;

	switch (cable_type) {
	case MUIC_RT8973_CABLE_TYPE_NONE:
	case MUIC_RT8973_CABLE_TYPE_UNKNOWN:
		current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
        set_cable_status = CABLE_TYPE_NONE;
		break;
	case MUIC_RT8973_CABLE_TYPE_USB:
	case MUIC_RT8973_CABLE_TYPE_CDP:
	case MUIC_RT8973_CABLE_TYPE_L200K_SPEC_USB:
		current_cable_type = POWER_SUPPLY_TYPE_USB;
        set_cable_status = CABLE_TYPE_USB;
		break;
	case MUIC_RT8973_CABLE_TYPE_REGULAR_TA:
		current_cable_type = POWER_SUPPLY_TYPE_MAINS;
        set_cable_status = CABLE_TYPE_AC;
		break;
	case MUIC_RT8973_CABLE_TYPE_OTG:
		goto skip;
	case MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF:
	/*
		if (!gpio_get_value(mfp_to_gpio(GPIO008_GPIO_8))) {
			pr_info("%s cable type POWER_SUPPLY_TYPE_UARTOFF\n", __func__);
			current_cable_type = POWER_SUPPLY_TYPE_UARTOFF;
		}
		else {
		current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
		}*/
		current_cable_type = POWER_SUPPLY_TYPE_BATTERY;

		break;
	case MUIC_RT8973_CABLE_TYPE_JIG_USB_ON:
	case MUIC_RT8973_CABLE_TYPE_JIG_USB_OFF:
		current_cable_type = POWER_SUPPLY_TYPE_USB;
        set_cable_status = CABLE_TYPE_USB;
		break;
	case MUIC_RT8973_CABLE_TYPE_0x1A:
	case MUIC_RT8973_CABLE_TYPE_TYPE1_CHARGER:
		current_cable_type = POWER_SUPPLY_TYPE_MAINS;
        set_cable_status = CABLE_TYPE_AC;
		break;
	case MUIC_RT8973_CABLE_TYPE_0x15:
		current_cable_type = POWER_SUPPLY_TYPE_MISC;
        set_cable_status = CABLE_TYPE_AC;
		break;
	case MUIC_RT8973_CABLE_TYPE_ATT_TA:
		current_cable_type = POWER_SUPPLY_TYPE_MISC;
        set_cable_status = CABLE_TYPE_AC;
		break;
	case MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF_WITH_VBUS:
		current_cable_type = POWER_SUPPLY_TYPE_UARTOFF;
		break;
	default:
		pr_err("%s: invalid type for charger:%d\n",
			__func__, cable_type);
		current_cable_type = POWER_SUPPLY_TYPE_UNKNOWN;
        set_cable_status = CABLE_TYPE_NONE;
		goto skip;
	}

	if (!psy || !psy->set_property)
		pr_err("%s: fail to get battery psy\n", __func__);
	else {
                value.intval = current_cable_type;
		psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	}

skip:
	return;
}
EXPORT_SYMBOL(sec_charger_cb);

void rt8973_usb_cb(uint8_t attached) {
	pr_info("%s USB Cable is %s\n",
		__func__, attached ? "attached" : "detached");
	sec_otg_set_vbus_state(attached);
}
#ifdef CONFIG_USB_HOST_NOTIFY
void rt8973_otg_cb(uint8_t attached)
{
	pr_info("rt8973_otg_cb attached %d\n", attached);
	current_cable_type = POWER_SUPPLY_TYPE_OTG;

	if (attached) {
		pr_info("%s USB Host attached", __func__);
		sec_otg_notify(HNOTIFY_ID);
	} else {
		pr_info("%s USB Host detached", __func__);
		sec_otg_notify(HNOTIFY_ID_PULL);
	}
}
#endif
struct rt8973_platform_data  rt8973_pdata = {
#ifdef CONFIG_MACH_KANAS3G_CTC
    .irq_gpio = 83,
#else
    .irq_gpio = 82,
#endif
    .cable_chg_callback = NULL,
    .ocp_callback = NULL,
    .otp_callback = NULL,
    .ovp_callback = NULL,
    .usb_callback = rt8973_usb_cb,
    .uart_callback = NULL,
#ifdef CONFIG_USB_HOST_NOTIFY
    .otg_callback = rt8973_otg_cb,
#else
    .otg_callback = NULL,
#endif
    .dock_init = rt8973_dock_init,
#ifdef MUIC_SUPPORT_CARDOCK_FUNCTION
    .jig_callback = rt8973_jig_callback,
#else
    .jig_callback = NULL,
#endif
};

/*static struct i2c_board_info rtmuic_i2c_boardinfo[] __initdata = {

    {
        I2C_BOARD_INFO("rt8973", 0x28>>1),
        .platform_data = &rt8973_pdata,
    },

};*/

//////////////////////////////////////////////////////////////////////////////////////////////////
#endif

#ifdef CONFIG_USB_SWITCH_FSA9485

static enum cable_type_t set_cable_status;
int current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
int msm8930_get_cable_status(void) {return (int)set_cable_status; }

extern int poweroff_charging;

/* support for LPM charging */
bool sec_bat_is_lpm(void)
{
	return (bool)poweroff_charging;
}

int sec_bat_get_cable_status(void)
{
	int rc;
	struct power_supply *psy;
	union power_supply_propval value;

	psy = power_supply_get_by_name("battery");
	switch (set_cable_status) {
	case CABLE_TYPE_MISC:
		value.intval = POWER_SUPPLY_TYPE_MISC;
		break;
	case CABLE_TYPE_USB:
		value.intval = POWER_SUPPLY_TYPE_USB;
		break;
	case CABLE_TYPE_AC:
	case CABLE_TYPE_AUDIO_DOCK:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_UARTOFF:
		value.intval = POWER_SUPPLY_TYPE_UARTOFF;
		break;
	case CABLE_TYPE_CARDOCK:
		value.intval = POWER_SUPPLY_TYPE_CARDOCK;
		break;
	case CABLE_TYPE_CDP:
		value.intval = POWER_SUPPLY_TYPE_USB_CDP;
		break;
	case CABLE_TYPE_INCOMPATIBLE:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_DESK_DOCK:
		value.intval = POWER_SUPPLY_TYPE_MISC;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
	if (set_cable_status == CABLE_TYPE_UARTOFF)
	value.intval = POWER_SUPPLY_TYPE_UARTOFF;
		break;
	default:
                pr_err("%s: LPM boot with invalid cable :%d\n", __func__,set_cable_status);
	}

	rc = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,&value);

	if (rc) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, rc);
        }

	return (int)set_cable_status;
}

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

static void fsa9485_muic_mhl_cb(int attached)
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

	value.intval = current_cable_type;
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

	value.intval = current_cable_type;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}
#endif

void fsa9485_set_mhl_cable(bool attached) {
	union power_supply_propval value;

	pr_info("%s: MHL Cable setting in LPM Mode%d\n", __func__, attached);

	if (poweroff_charging && attached) {
		value.intval = POWER_SUPPLY_TYPE_MHL_USB;
		current_cable_type = value.intval;
		value.intval = current_cable_type;
	} else {
		pr_info("%s: Ignore Cable setting, Not LPM mode\n", __func__);
	}
}
EXPORT_SYMBOL(fsa9485_set_mhl_cable);

bool fsa9485_muic_is_mhl_attached(void)
{
	return 0;
}

static void fsa9485_otg_cb(bool attached)
{
	pr_info("fsa9485_otg_cb attached %d\n", attached);
	current_cable_type = POWER_SUPPLY_TYPE_OTG;

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

static void fsa9485_charging_cable_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_charging_cable_cb attached %d\n", attached);

	set_cable_status =
		attached ? CABLE_TYPE_CHARGING_CABLE : CABLE_TYPE_NONE;

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("ps");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get ps\n", __func__);
		return;
	}

	switch (set_cable_status) {
	case CABLE_TYPE_CHARGING_CABLE:
		value.intval = POWER_SUPPLY_TYPE_POWER_SHARING;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}

	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE,
		&value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}

#if defined(CONFIG_SEC_VIENNA_PROJECT) || defined(CONFIG_SEC_V2_PROJECT)
extern int sec_qcom_usb_rdrv;
#endif

#ifdef CONFIG_SEC_BERLUTI_PROJECT
extern void sec_otg_set_vbus_state(int);
#endif

static void fsa9485_usb_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_usb_cb attached %d\n", attached);
	set_cable_status = attached ? CABLE_TYPE_USB : CABLE_TYPE_NONE;

#ifndef CONFIG_SEC_BERLUTI_PROJECT
        /* Vienna SS - USB 3.0 redriver enable/disable */
#if defined(CONFIG_SEC_VIENNA_PROJECT) || defined(CONFIG_SEC_V2_PROJECT)
	gpio_set_value(sec_qcom_usb_rdrv, attached);
	pr_info("%s sec_qcom_usb_rdrv = %d, enable=%d\n",
		__func__,
		sec_qcom_usb_rdrv,
		attached);
#endif

	psy = power_supply_get_by_name("dwc-usb");
	if (!psy) {
		pr_info("%s: couldn't get usb power supply\n", __func__);
		return;
	}
	pr_info("%s: MUIC attached: %d\n", __func__, attached);

	power_supply_set_present(psy, attached);
#else
	sec_otg_set_vbus_state(attached);
#endif

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1664S) || defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1188S)
	mxt_tsp_charger_infom(attached);
#endif
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

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	value.intval = current_cable_type;
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

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1664S) || defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1188S)
	mxt_tsp_charger_infom(attached);
#endif

#ifdef CONFIG_TOUCHSCREEN_MMS144
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, attached);
#endif

	value.intval = current_cable_type;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}

static void fsa9485_in_charger_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("%s attached %d\n", __func__, attached);
	set_cable_status = attached ? CABLE_TYPE_INCOMPATIBLE : CABLE_TYPE_NONE;

	switch (set_cable_status) {
	case CABLE_TYPE_INCOMPATIBLE:
		value.intval = POWER_SUPPLY_TYPE_UNKNOWN;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}
	current_cable_type = value.intval;

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1664S) || defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1188S)
	mxt_tsp_charger_infom(attached);
#endif

#ifdef CONFIG_TOUCHSCREEN_MMS144
	if (charger_callbacks && charger_callbacks->inform_charger)
		charger_callbacks->inform_charger(charger_callbacks, attached);
#endif

	value.intval = current_cable_type;
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

#ifndef CONFIG_SEC_BERLUTI_PROJECT
	psy = power_supply_get_by_name("dwc-usb");
	if (!psy) {
		pr_info("%s: couldn't get usb power supply\n", __func__);
		return;
	}
	pr_info("%s: MUIC attached: %d\n", __func__, attached);

	power_supply_set_present(psy, attached);
#else
	sec_otg_set_vbus_state(attached);
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

	value.intval = current_cable_type;
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

	switch(attached) {
	case FSA9485_ATTACHED_DESK_DOCK:
	case FSA9485_ATTACHED_CAR_DOCK:
		set_cable_status = CABLE_TYPE_CARDOCK;
		break;
	default:
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

	value.intval = current_cable_type;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}

#ifndef CONFIG_SEC_BERLUTI_PROJECT
#ifdef CONFIG_MUIC_FSA9485_SUPPORT_LANHUB
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
		sec_otg_notify(HNOTIFY_LANHUB_ON);
	}else{
		pr_info("USB Host HNOTIFY_LANHUB_OFF");
		sec_otg_notify(HNOTIFY_LANHUB_OFF);
	}
#endif

	switch (set_cable_status) {
	case CABLE_TYPE_LANHUB:
		value.intval = POWER_SUPPLY_TYPE_LAN_HUB;
		break;
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("invalid status:%d\n", attached);
		return;
	}

	current_cable_type = value.intval;

	value.intval = current_cable_type;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
}

static void fsa9485_lanhubta_cb(bool attached)
{
	union power_supply_propval value;
	int i;
	struct power_supply *psy;

	pr_info("fsa9485_lanhubta_cb attached %d\n", attached);

	set_cable_status =
		attached ? CABLE_TYPE_LANHUB : POWER_SUPPLY_TYPE_OTG;

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
		pr_info("USB Host HNOTIFY_LANHUB_N_TA_ON\n");
		sec_otg_notify(HNOTIFY_LANHUBTA_ON);
	} else {
		pr_info("USB Host HNOTIFY_LANHUB_ON\n");
		sec_otg_notify(HNOTIFY_LANHUBTA_OFF);
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
}
#endif
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

	value.intval = current_cable_type;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}

//	msm_otg_set_smartdock_state(attached);
}

static void fsa9485_mmdock_cb(bool attached)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;

	pr_info("fsa9485_mmdock_cb attached %d\n", attached);

	switch_set_state(&switch_dock, attached);
	set_cable_status =
		attached ? CABLE_TYPE_MM_DOCK : CABLE_TYPE_NONE;

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
	case CABLE_TYPE_MM_DOCK:
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

	value.intval = current_cable_type;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}

}

static void fsa9485_audio_dock_cb(bool attached)
{
	pr_info("fsa9485_audio_dock_cb attached %d\n", attached);

//	msm_otg_set_smartdock_state(attached);
}
#endif

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

			value.intval = current_cable_type;
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
	.charge_cb	= fsa9485_charging_cable_cb,
	.usb_cb		= fsa9485_usb_cb,
	.charger_cb	= fsa9485_charger_cb,
	.in_charger_cb	= fsa9485_in_charger_cb,
	.uart_cb	= fsa9485_uart_cb,
	.jig_cb		= fsa9485_jig_cb,
	.dock_cb	= fsa9485_dock_cb,
	.dock_init	= fsa9485_dock_init,
	.usb_cdp_cb	= fsa9485_usb_cdp_cb,
#ifndef CONFIG_SEC_BERLUTI_PROJECT
#ifdef CONFIG_MUIC_FSA9485_SUPPORT_LANHUB
	.lanhub_cb	= fsa9485_lanhub_cb,
	.lanhubta_cb	= fsa9485_lanhubta_cb,
#endif
	.smartdock_cb	= fsa9485_smartdock_cb,
	.audio_dock_cb	= fsa9485_audio_dock_cb,
	.mmdock_cb		= fsa9485_mmdock_cb,
#else
#ifdef CONFIG_MUIC_FSA9485_SUPPORT_LANHUB
	.lanhub_cb	= NULL,
	.lanhubta_cb	= NULL,
#endif
	.smartdock_cb	= NULL,
	.audio_dock_cb	= NULL,
#endif
#ifdef CONFIG_VIDEO_MHL_V2
	.mhl_cb = fsa9485_muic_mhl_cb,
#endif
};

#endif

/*Adding Support for SM5502 MUIC Call Backs*/
#if defined(CONFIG_SM5502_MUIC)

extern int check_sm5502_jig_state(void);
void sm5502_oxp_callback(int state)
{
#if 0 //ovp stub-implemented on completion
	bool ovp_state;
	if (state == 1) {
		ovp_state = true;
		/*ENABLE*/
	} else if (state == 0) {
		ovp_state = false;
		/*DISABLE*/
	}
#endif
}

int sm5502_dock_init(void)
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
#if defined(DEBUG_STATUS)
static int status_count;
#endif

#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
void sm5502_lanhub_callback(enum cable_type_t cable_type, int attached, bool lanhub_ta)
{
	union power_supply_propval value;
	struct power_supply *psy;
	int i, ret = 0;

	pr_info("SM5502 Lanhub Callback called, cable %d, attached %d, TA: %s \n",
				cable_type,attached,(lanhub_ta ? "Yes":"No"));
	if(lanhub_ta)
		set_cable_status = attached ? CABLE_TYPE_LANHUB : POWER_SUPPLY_TYPE_OTG;
	else
		set_cable_status = attached ? CABLE_TYPE_LANHUB : CABLE_TYPE_NONE;

	pr_info("%s:sm5502 cable type : %d", __func__, set_cable_status);

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}

	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	if (!psy || !psy->set_property)
                pr_err("%s: fail to set battery psy\n", __func__);
        else {
		switch (set_cable_status) {
		case CABLE_TYPE_LANHUB:
			value.intval = POWER_SUPPLY_TYPE_LAN_HUB;
			break;
	        case POWER_SUPPLY_TYPE_OTG:
	        case CABLE_TYPE_NONE:
			value.intval = lanhub_ta ? POWER_SUPPLY_TYPE_OTG : POWER_SUPPLY_TYPE_BATTERY;
			break;
		default:
	                pr_err("invalid status:%d\n", attached);
	                return;
		}

	        ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
		if (ret) {
			pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
				__func__, ret);
	        }

	}

#ifdef CONFIG_USB_HOST_NOTIFY
	if (attached){
		if(lanhub_ta) {
			pr_info("USB Host HNOTIFY LANHUB_N_TA_ON\n");
		}
		else{
			pr_info("USB Host HNOTIFY_LANHUB_ON\n");
			sec_otg_notify(HNOTIFY_LANHUB_ON);
		}
	}else{
		if(lanhub_ta) {
                        pr_info("USB Host HNOTIFY LANHUB ON\n");
                }
                else{
			pr_info("USB Host HNOTIFY_LANHUB_OFF");
			sec_otg_notify(HNOTIFY_LANHUB_OFF);
		}
	}
#endif

}
#endif

void sm5502_callback(enum cable_type_t cable_type, int attached)
{
	union power_supply_propval value;
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct power_supply *psy_ps = power_supply_get_by_name("ps");
	static enum cable_type_t previous_cable_type = CABLE_TYPE_NONE;
	pr_info("%s, called : cable_type :%d \n",__func__, cable_type);
#if defined(CONFIG_TOUCHSCREEN_MXTS) ||defined(CONFIG_TOUCHSCREEN_MXT224E) || defined(CONFIG_TOUCHSCREEN_MMS252) || defined(CONFIG_TOUCHSCREEN_MMS300)
        if (charger_callbacks && charger_callbacks->inform_charger)
                charger_callbacks->inform_charger(charger_callbacks,
                attached);
#endif
#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT1188S)
	mxt_tsp_charger_infom(attached);
#endif

	set_cable_status = attached ? cable_type : CABLE_TYPE_NONE;

if(!poweroff_charging){
#if defined(CONFIG_TOUCHSCREEN_ZINITIX_BT532)
	bt532_charger_status_cb(set_cable_status);
#endif
}
	switch (cable_type) {
	case CABLE_TYPE_USB:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s USB Cable status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s USB Cable Status detached (%d) \n", __func__,status_count);
               }
#endif
		sec_otg_set_vbus_state(attached);
		break;
	case CABLE_TYPE_AC:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Charger status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s charger status detached (%d) \n", __func__,status_count);
               }
#endif
		break;
	case CABLE_TYPE_UARTOFF:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s UART Status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s UART status detached (%d) \n", __func__,status_count);
               }
#endif
		break;
	case CABLE_TYPE_JIG_UART_OFF_VB:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s UART OFF VBUS Status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s UART OFF VBUS status detached (%d) \n", __func__,status_count);
               }
#endif
		break;
	case CABLE_TYPE_JIG:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s JIG cable status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s JIG cable status detached (%d) \n", __func__,status_count);
               }
#endif
		return;
	case CABLE_TYPE_CDP:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s CDP status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s CDP Status detached (%d) \n", __func__,status_count);
               }
#endif
		sec_otg_set_vbus_state(attached);
		break;
	case CABLE_TYPE_OTG:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s OTG status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s OTG status detached (%d) \n", __func__,status_count);
               }
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
		sec_otg_notify(attached ? HNOTIFY_ID : HNOTIFY_ID_PULL);
#endif
	       return;
	case CABLE_TYPE_AUDIO_DOCK:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Audiodock status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Audiodock status detached (%d) \n", __func__,status_count);
               }
#endif
		return;
	case CABLE_TYPE_CARDOCK:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Cardock status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Cardock status detached (%d) \n", __func__,status_count);
               }
#endif
		switch_set_state(&switch_dock, attached ? 2 : 0);
		break;
#if defined(CONFIG_SEC_MILLET_PROJECT) || defined(CONFIG_SEC_MATISSE_PROJECT)
	case CABLE_TYPE_SMART_DOCK:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s SMART status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s SMART status detached (%d) \n", __func__,status_count);
               }
#endif
		switch_set_state(&switch_dock, attached ? 8 : 0); // 8 for CABLE_TYPE_SMART_DOCK:
	    break;
#endif
	case CABLE_TYPE_DESK_DOCK:
	case CABLE_TYPE_DESK_DOCK_NO_VB:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Deskdock %s status attached (%d) \n",__func__,
				((cable_type == CABLE_TYPE_DESK_DOCK)? "VBUS" : "NOVBUS"),status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Deskdock %s status detached (%d) \n", __func__,
				((cable_type == CABLE_TYPE_DESK_DOCK)? "VBUS" : "NOVBUS"),status_count);
               }
#endif
		if(cable_type != CABLE_TYPE_DESK_DOCK_NO_VB)
			switch_set_state(&switch_dock, attached);
		break;
	case CABLE_TYPE_219KUSB:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s 219K USB status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s 219K USB status detached (%d) \n", __func__,status_count);
               }
#endif
		sec_otg_set_vbus_state(attached);
		break;
	case CABLE_TYPE_CHARGING_CABLE:
		if (attached)
			value.intval = POWER_SUPPLY_TYPE_POWER_SHARING;
		else
			value.intval = POWER_SUPPLY_TYPE_BATTERY;

		if (psy_ps) {
			if (psy_ps->set_property(psy_ps, POWER_SUPPLY_PROP_ONLINE, &value)) {
				pr_err("%s: fail to set power sharing ONLINE property\n",__func__);
			}
		}
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Phone Charging cable status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Phone Charging cable status detached (%d) \n", __func__,status_count);
               }
#endif
		break;
	case CABLE_TYPE_INCOMPATIBLE:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Incompatible Charger status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Incomabtible Charger status detached (%d) \n", __func__,status_count);
               }
#endif
		break;
	default:
		break;
	}

	if (previous_cable_type == set_cable_status) {
		pr_info("%s: SKIP cable setting\n", __func__);
		return;
	}
	previous_cable_type = set_cable_status;

#if defined(CONFIG_FUELGAUGE_MAX17050)
	if(check_sm5502_jig_state())
	{
		struct power_supply *fuel_psy = power_supply_get_by_name("sec-fuelgauge");
		if (!fuel_psy || !fuel_psy->set_property)
			pr_err("%s: fail to get sec-fuelgauge psy\n", __func__);
		else {
			fuel_psy->set_property(fuel_psy, POWER_SUPPLY_PROP_CHARGE_TYPE, &value);
		}
	}
#endif
#if defined(CONFIG_QPNP_BMS)
  if(check_sm5502_jig_state())
  {
	  struct power_supply *fuel_psy = power_supply_get_by_name("bms");
	  if (!fuel_psy || !fuel_psy->set_property)
		pr_err("%s: fail to get BMS psy\n", __func__);
		else {
			fuel_psy->set_property(fuel_psy, POWER_SUPPLY_PROP_CHARGE_TYPE, &value);
		}
	}
#endif

	switch (set_cable_status) {
	case CABLE_TYPE_MISC:
		value.intval = POWER_SUPPLY_TYPE_MISC;
		break;
	case CABLE_TYPE_USB:
		value.intval = POWER_SUPPLY_TYPE_USB;
		break;
	case CABLE_TYPE_219KUSB:
	case CABLE_TYPE_AC:
	case CABLE_TYPE_AUDIO_DOCK:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_CARDOCK:
		value.intval = POWER_SUPPLY_TYPE_CARDOCK;
		break;
	case CABLE_TYPE_CDP:
		value.intval = POWER_SUPPLY_TYPE_USB_CDP;
		break;
	case CABLE_TYPE_INCOMPATIBLE:
		value.intval = POWER_SUPPLY_TYPE_UNKNOWN;
		break;
	case CABLE_TYPE_DESK_DOCK:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
    case CABLE_TYPE_JIG_UART_OFF_VB:
                value.intval = POWER_SUPPLY_TYPE_UARTOFF;
                break;
	case CABLE_TYPE_JIG:
	case CABLE_TYPE_DESK_DOCK_NO_VB:
	case CABLE_TYPE_UARTOFF:
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
#if defined(CONFIG_SEC_MILLET_PROJECT) || defined(CONFIG_SEC_MATISSE_PROJECT)
	case CABLE_TYPE_SMART_DOCK:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
#endif
	default:
		pr_err("%s: invalid cable :%d\n", __func__, set_cable_status);
		return;
	}
	current_cable_type = value.intval;
	pr_info("%s:MUIC setting the cable type as (%d)\n",__func__,value.intval);

	if (!psy || !psy->set_property)
		pr_err("%s: fail to get battery psy\n", __func__);
	else {
		psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	}
}

struct sm5502_platform_data sm5502_pdata = {
	.callback = sm5502_callback,
#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
	.lanhub_cb = sm5502_lanhub_callback,
#endif
	.dock_init = sm5502_dock_init,
	.oxp_callback = sm5502_oxp_callback,
	.mhl_sel = NULL,

};
#endif /*End of SM5502 MUIC Callbacks*/

#if defined(CONFIG_SM5504_MUIC)
/* callbacks & Handlers for the SM5504 MUIC*/
/* support for the LPM Charging*/
extern void sec_otg_set_vbus_state(int);
static enum cable_type_t set_cable_status;

#if defined(CONFIG_QPNP_BMS)
extern int check_sm5504_jig_state(void);
#endif

void sm5504_oxp_callback(int state)
{
#if 0 //ovp stub-implemented on completion
	bool ovp_state;
	if (state == 1) {
		ovp_state = true;
		/*ENABLE*/
	} else if (state == 0) {
		ovp_state = false;
		/*DISABLE*/
	}
#endif
}

int sm5504_dock_init(void)
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

#if defined(DEBUG_STATUS)
static int status_count;
#endif

#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
void sm5504_lanhub_callback(enum cable_type_t cable_type, int attached, bool lanhub_ta)
{
	union power_supply_propval value;
	struct power_supply *psy;
	int i, ret = 0;

	pr_info("SM5504 Lanhub Callback called, cable %d, attached %d, TA: %s \n",
				cable_type,attached,(lanhub_ta ? "Yes":"No"));
	if(lanhub_ta)
		set_cable_status = attached ? CABLE_TYPE_LANHUB : POWER_SUPPLY_TYPE_OTG;
	else
		set_cable_status = attached ? CABLE_TYPE_LANHUB : CABLE_TYPE_NONE;

	pr_info("%s:sm5504 cable type : %d", __func__, set_cable_status);

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}

	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		return;
	}

	if (!psy || !psy->set_property)
                pr_err("%s: fail to set battery psy\n", __func__);
        else {
		switch (set_cable_status) {
		case CABLE_TYPE_LANHUB:
			value.intval = POWER_SUPPLY_TYPE_LAN_HUB;
			break;
	        case POWER_SUPPLY_TYPE_OTG:
	        case CABLE_TYPE_NONE:
			value.intval = lanhub_ta ? POWER_SUPPLY_TYPE_OTG : POWER_SUPPLY_TYPE_BATTERY;
			break;
		default:
	                pr_err("%s invalid status:%d\n", __func__,attached);
	                return;
		}

	        ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
		if (ret) {
			pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
				__func__, ret);
	        }

	}

#ifdef CONFIG_USB_HOST_NOTIFY
	if (attached){
		if(lanhub_ta) {
			pr_info("USB Host HNOTIFY LANHUB_N_TA_ON\n");
		}
		else{
			pr_info("USB Host HNOTIFY_LANHUB_ON\n");
			sec_otg_notify(HNOTIFY_LANHUB_ON);
		}
	}else{
		if(lanhub_ta) {
                        pr_info("USB Host HNOTIFY LANHUB ON\n");
                }
                else{
			pr_info("USB Host HNOTIFY_LANHUB_OFF");
			sec_otg_notify(HNOTIFY_LANHUB_OFF);
		}
	}
#endif

}
#endif

#if defined(CONFIG_TOUCHSCREEN_ZINITIX_BT541) || defined(USE_TSP_TA_CALLBACKS)
struct tsp_callbacks *charger_callbacks;
#endif

void sm5504_callback(enum cable_type_t cable_type, int attached)
{
	union power_supply_propval value;
	struct power_supply *psy = power_supply_get_by_name("battery");
	static enum cable_type_t previous_cable_type = CABLE_TYPE_NONE;
	pr_info("%s, called : cable_type :%d \n",__func__, cable_type);

	set_cable_status = attached ? cable_type : CABLE_TYPE_NONE;

	switch (cable_type) {
	case CABLE_TYPE_USB:
#if defined(CONFIG_TOUCHSCREEN_ZINITIX_BT541) || defined(USE_TSP_TA_CALLBACKS)
		if (charger_callbacks && charger_callbacks->inform_charger)
			charger_callbacks->inform_charger(charger_callbacks,
			attached);
#endif
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s USB Cable status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s USB Cable Status detached (%d) \n", __func__,status_count);
               }
#endif
		sec_otg_set_vbus_state(attached);
		break;
	case CABLE_TYPE_AC:
#if defined(CONFIG_TOUCHSCREEN_ZINITIX_BT541) || defined(USE_TSP_TA_CALLBACKS)
		if (charger_callbacks && charger_callbacks->inform_charger)
			charger_callbacks->inform_charger(charger_callbacks,
			attached);
#endif
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Charger status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s charger status detached (%d) \n", __func__,status_count);
               }
#endif
		break;
	case CABLE_TYPE_UARTOFF:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s UART Status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s UART status detached (%d) \n", __func__,status_count);
               }
#endif
		break;
	case CABLE_TYPE_JIG_UART_OFF_VB:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s UART OFF VBUS Status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s UART OFF VBUS status detached (%d) \n", __func__,status_count);
               }
#endif
		break;
	case CABLE_TYPE_JIG:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s JIG cable status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s JIG cable status detached (%d) \n", __func__,status_count);
               }
#endif
		return;
	case CABLE_TYPE_CDP:
#if defined(CONFIG_TOUCHSCREEN_ZINITIX_BT541) || defined(USE_TSP_TA_CALLBACKS)
		if (charger_callbacks && charger_callbacks->inform_charger)
			charger_callbacks->inform_charger(charger_callbacks,
			attached);
#endif
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s CDP status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s CDP Status detached (%d) \n", __func__,status_count);
               }
#endif
		sec_otg_set_vbus_state(attached);
		break;
	case CABLE_TYPE_OTG:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s OTG status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s OTG status detached (%d) \n", __func__,status_count);
               }
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
		sec_otg_notify(attached ? HNOTIFY_ID : HNOTIFY_ID_PULL);
#endif
	       return;
	case CABLE_TYPE_AUDIO_DOCK:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Audiodock status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Audiodock status detached (%d) \n", __func__,status_count);
               }
#endif
		return;
	case CABLE_TYPE_CARDOCK:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Cardock status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Cardock status detached (%d) \n", __func__,status_count);
               }
#endif
		switch_set_state(&switch_dock, attached ? 2 : 0);
		break;
	case CABLE_TYPE_DESK_DOCK:
	case CABLE_TYPE_DESK_DOCK_NO_VB:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Deskdock %s status attached (%d) \n",__func__,
				((cable_type == CABLE_TYPE_DESK_DOCK)? "VBUS" : "NOVBUS"),status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Deskdock %s status detached (%d) \n", __func__,
				((cable_type == CABLE_TYPE_DESK_DOCK)? "VBUS" : "NOVBUS"),status_count);
               }
#endif
		switch_set_state(&switch_dock, attached);
		break;
	case CABLE_TYPE_219KUSB:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s 219K USB status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s 219K USB status detached (%d) \n", __func__,status_count);
               }
#endif
		sec_otg_set_vbus_state(attached);
		break;
	case CABLE_TYPE_INCOMPATIBLE:
#if defined(DEBUG_STATUS)
               if (attached)
               {
                       status_count = status_count+1;
                       pr_err("%s Incompatible Charger status attached (%d) \n",__func__, status_count);
               } else {
                       status_count = status_count-1;
                       pr_err("%s Incomabtible Charger status detached (%d) \n", __func__,status_count);
               }
#endif
		break;
	default:
		break;
	}

	if (previous_cable_type == set_cable_status) {
		pr_info("%s: SKIP cable setting\n", __func__);
		return;
	}
	previous_cable_type = set_cable_status;

#if defined(CONFIG_FUELGAUGE_MAX17050)
	if(check_sm5504_jig_state())
	{
		struct power_supply *fuel_psy = power_supply_get_by_name("sec-fuelgauge");
		if (!fuel_psy || !fuel_psy->set_property)
			pr_err("%s: fail to get sec-fuelgauge psy\n", __func__);
		else {
			fuel_psy->set_property(fuel_psy, POWER_SUPPLY_PROP_CHARGE_TYPE, &value);
		}
	}
#endif
#if defined(CONFIG_QPNP_BMS)
	if(check_sm5504_jig_state())
	{
	  struct power_supply *fuel_psy = power_supply_get_by_name("bms");
	  if (!fuel_psy || !fuel_psy->set_property)
		pr_err("%s: fail to get BMS psy\n", __func__);
		else {
			fuel_psy->set_property(fuel_psy, POWER_SUPPLY_PROP_CHARGE_TYPE, &value);
		}
	}
#endif

	switch (set_cable_status) {
	case CABLE_TYPE_MISC:
		value.intval = POWER_SUPPLY_TYPE_MISC;
		break;
	case CABLE_TYPE_USB:
		value.intval = POWER_SUPPLY_TYPE_USB;
		break;
	case CABLE_TYPE_219KUSB:
	case CABLE_TYPE_AC:
	case CABLE_TYPE_AUDIO_DOCK:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
	case CABLE_TYPE_CARDOCK:
		value.intval = POWER_SUPPLY_TYPE_CARDOCK;
		break;
	case CABLE_TYPE_CDP:
		value.intval = POWER_SUPPLY_TYPE_USB_CDP;
		break;
	case CABLE_TYPE_INCOMPATIBLE:
		value.intval = POWER_SUPPLY_TYPE_UNKNOWN;
		break;
	case CABLE_TYPE_DESK_DOCK:
		value.intval = POWER_SUPPLY_TYPE_MAINS;
		break;
        case CABLE_TYPE_JIG_UART_OFF_VB:
                value.intval = POWER_SUPPLY_TYPE_UARTOFF;
                break;
	case CABLE_TYPE_DESK_DOCK_NO_VB:
	case CABLE_TYPE_UARTOFF:
	case CABLE_TYPE_NONE:
		value.intval = POWER_SUPPLY_TYPE_BATTERY;
		break;
	default:
		pr_err("%s: invalid cable :%d\n", __func__, set_cable_status);
		return;
	}
	current_cable_type = value.intval;
	pr_info("%s:MUIC setting the cable type as (%d)\n",__func__,value.intval);
	if (!psy || !psy->set_property)
		pr_err("%s: fail to get battery psy\n", __func__);
	else {
		psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	}
}

struct sm5504_platform_data sm5504_pdata = {
	.callback = sm5504_callback,
#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
	.lanhub_cb = sm5504_lanhub_callback,
#endif
	.dock_init = sm5504_dock_init,
	.oxp_callback = sm5504_oxp_callback,
	.mhl_sel = NULL,

};
#endif /*End of SM5504 MUIC Callbacks*/

