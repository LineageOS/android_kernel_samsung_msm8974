/*
 * Copyright (C) 2013 Samsung Electronics Co. Ltd.
 *  Hyuk Kang <hyuk78.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/host_notify.h>
#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/usb_notify_sysfs.h>
#endif
#include <linux/of_gpio.h>

#if defined (CONFIG_CHARGER_BQ24260) || defined (CONFIG_CHARGER_SMB358)
#include <linux/gpio.h>
#include <mach/rpm-regulator-smd.h>
#endif
#ifdef CONFIG_EXTCON
#include <linux/extcon.h>
#include <linux/power_supply.h>
#endif

#if defined(CONFIG_SEC_K_PROJECT)
	extern int sec_qcom_usb_rdrv;
#endif

struct dwc3_sec {
	struct notifier_block nb;
	struct dwc3_msm *dwcm;
};
static struct dwc3_sec sec_noti;

#ifdef CONFIG_USB_HOST_NOTIFY
static int booster_enable;
static int gpio_usb_vbus_msm;
#endif

#ifdef CONFIG_CHARGER_SMB1357
struct delayed_work	smb1357_late_power_work;
int smb1357_otg_control(int enable)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;
	int current_cable_type;

	pr_info("%s: enable(%d)\n", __func__, enable);

#ifdef CONFIG_USB_HOST_NOTIFY
	booster_enable = enable;
#endif
	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		schedule_delayed_work(&smb1357_late_power_work, msecs_to_jiffies(5000));
		return -1;
	}

	if (enable == 1)
		current_cable_type = POWER_SUPPLY_TYPE_OTG;
	else if (enable == 2)
		current_cable_type = POWER_SUPPLY_TYPE_LAN_HUB;
	else
		current_cable_type = POWER_SUPPLY_TYPE_BATTERY;

	value.intval = current_cable_type;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	msleep(500);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
	return ret;
}

static void smb1357_late_power(struct work_struct *work)
{
	struct dwc3_sec *snoti = &sec_noti;
	struct dwc3_msm *dwcm;
	
	if (!snoti) {
		pr_err("%s: dwc3_otg (snoti) is null\n", __func__);
		return;
	}

	dwcm = snoti->dwcm;
	if (!dwcm) {
		pr_err("%s: dwc3_otg (dwcm) is null\n", __func__);
		return;
	}
	
	pr_info("%s, ext_xceiv.id=%d\n", __func__, dwcm->ext_xceiv.id);

#ifdef CONFIG_USB_HOST_NOTIFY
	if (dwcm->ext_xceiv.id == DWC3_ID_GROUND) {
		if (gpio_usb_vbus_msm > 0) {
			if (gpio_get_value(gpio_usb_vbus_msm) == 0)
				smb1357_otg_control(booster_enable);
		} else {
			smb1357_otg_control(booster_enable);
		}
	}
#endif
}

static void usb_vbus_msm_init(struct dwc3_msm *dwcm, struct usb_phy *phy)
{
	INIT_DELAYED_WORK(&smb1357_late_power_work, smb1357_late_power);
}
#endif

#if defined (CONFIG_CHARGER_BQ24260) || defined (CONFIG_CHARGER_SMB358)
struct delayed_work	bq24260_late_power_work;
int bq24260_otg_control(int enable)
{
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;
	int current_cable_type;

	pr_info("%s: enable(%d)\n", __func__, enable);

#ifdef CONFIG_USB_HOST_NOTIFY
	booster_enable = enable;
#endif
	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("%s: fail to get battery ps\n", __func__);
		schedule_delayed_work(&bq24260_late_power_work, msecs_to_jiffies(5000));
		return -1;
	}

	if (enable == 1)
		current_cable_type = POWER_SUPPLY_TYPE_OTG;
	else if (enable == 2)
		current_cable_type = POWER_SUPPLY_TYPE_LAN_HUB;
	else
		current_cable_type = POWER_SUPPLY_TYPE_BATTERY;

	value.intval = current_cable_type;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);

	if (ret) {
		pr_err("%s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
	}
	return ret;
}

static void bq24260_late_power(struct work_struct *work)
{
	struct dwc3_sec *snoti = &sec_noti;
	struct dwc3_msm *dwcm;
	
	if (!snoti) {
		pr_err("%s: dwc3_otg (snoti) is null\n", __func__);
		return;
	}

	dwcm = snoti->dwcm;
	if (!dwcm) {
		pr_err("%s: dwc3_otg (dwcm) is null\n", __func__);
		return;
	}
	
	pr_info("%s, ext_xceiv.id=%d\n", __func__, dwcm->ext_xceiv.id);

#ifdef CONFIG_USB_HOST_NOTIFY
	if (dwcm->ext_xceiv.id == DWC3_ID_GROUND) {
		if (gpio_usb_vbus_msm > 0) {
			if (gpio_get_value(gpio_usb_vbus_msm) == 0)
				bq24260_otg_control(booster_enable);
		} else {
			bq24260_otg_control(booster_enable);
		}
	}
#endif
}

struct booster_data sec_booster = {
	.name = "bq24260",
	.boost = bq24260_otg_control,
};

#ifdef CONFIG_USB_HOST_NOTIFY
#if defined(CONFIG_MACH_VIENNA)
static struct rpm_regulator *s2a_regulator;

static void usb_vbus_s2a_force_pwm(unsigned int en)
{
	if (!s2a_regulator) {
		pr_err("%s, s2a_regulator is not init\n", __func__);
		return;
	}
	pr_info("%s, s2a_regulator %s mode\n", __func__,
			en ? "HPM" : "AUTO");
	rpm_regulator_set_mode(s2a_regulator,
			en ? RPM_REGULATOR_MODE_HPM : RPM_REGULATOR_MODE_AUTO);
}
#endif
#endif

static void usb_vbus_msm_init(struct dwc3_msm *dwcm, struct usb_phy *phy)
{
#ifdef CONFIG_USB_HOST_NOTIFY
	sec_otg_register_booster(&sec_booster);
#endif
	INIT_DELAYED_WORK(&bq24260_late_power_work, bq24260_late_power);
}
#endif


#ifdef CONFIG_USB_HOST_NOTIFY
static irqreturn_t msm_usb_vbus_msm_irq(int irq, void *data)
{
	struct dwc3_sec *snoti = &sec_noti;
	struct dwc3_msm *dwcm;
	int enable = gpio_get_value(gpio_usb_vbus_msm);
	pr_info("%s usb_vbus_msm=%d\n", __func__, enable);
	dwcm = snoti->dwcm;
	if (!dwcm) {
		pr_err("%s: dwc3_otg (dwcm) is null\n", __func__);
		return NOTIFY_BAD;
	}
	if (dwcm->ext_xceiv.id == DWC3_ID_GROUND && enable == 0 && booster_enable == 1) {
		pr_info("%s over current\n", __func__);
		sec_otg_notify(HNOTIFY_OVERCURRENT);
		return IRQ_HANDLED;
	}
	sec_otg_notify(enable ?
		HNOTIFY_OTG_POWER_ON : HNOTIFY_OTG_POWER_OFF);

#if defined(CONFIG_MACH_VIENNA)
	usb_vbus_s2a_force_pwm(enable);
#endif

	return IRQ_HANDLED;
}

static int get_vbus_detect_gpio(struct dwc3_msm *dwcm, struct device *dev)
{
	int ret;
	struct device_node *np = dev->of_node;	

	gpio_usb_vbus_msm = of_get_named_gpio(np, "qcom,vbus-detect-gpio", 0);
	if (gpio_usb_vbus_msm < 0) {
		pr_err("%s, cannot get vbus-detect-gpio, ret=%d\n", __func__, gpio_usb_vbus_msm);
		return gpio_usb_vbus_msm;
	}
	else
		pr_info("%s, can get vbus-detect-gpio, ret=%d\n", __func__, gpio_usb_vbus_msm);

	ret = gpio_tlmm_config(GPIO_CFG(gpio_usb_vbus_msm, 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	if (unlikely(ret)) {
	    pr_err("%s gpio_usb_vbus_msm gpio_tlmm_config failed. ret=%d\n", __func__, ret);
	    return ret;
	}

	pr_info("%s usb_vbus_msm=%d\n", __func__, gpio_get_value(gpio_usb_vbus_msm));
	ret = request_threaded_irq(gpio_to_irq(gpio_usb_vbus_msm),
						NULL, msm_usb_vbus_msm_irq,
						IRQF_TRIGGER_RISING |
						IRQF_TRIGGER_FALLING,
						"usb_vbus_msm", dwcm);
	if (ret)
		pr_err("%s request irq failed for usb_vbus_msm\n", __func__);
	else
		pr_info("%s request irq succeed for usb_vbus_msm\n", __func__);

#if defined(CONFIG_MACH_VIENNA)
	s2a_regulator = rpm_regulator_get(NULL, "8941_s2");
	if (IS_ERR_OR_NULL(s2a_regulator))
		pr_err("%s, could not get rpm regulator err\n", __func__);
	usb_vbus_s2a_force_pwm(gpio_get_value(gpio_usb_vbus_msm));
#endif

	return ret;
}
#endif

static int sec_otg_ext_notify(struct dwc3_msm *mdwc, int enable)
{
	mdwc->ext_xceiv.id = enable ? DWC3_ID_GROUND : DWC3_ID_FLOAT;

	if (atomic_read(&mdwc->in_lpm)) {
		dev_info(mdwc->dev, "%s: calling resume_work\n", __func__);
		dwc3_resume_work(&mdwc->resume_work.work);
	} else {
		dev_info(mdwc->dev, "%s: notifying xceiv event\n", __func__);
		if (mdwc->otg_xceiv)
			mdwc->ext_xceiv.notify_ext_events(mdwc->otg_xceiv->otg,
							DWC3_EVENT_XCEIV_STATE);
	}
	return 0;
}

int sec_handle_event(int enable)
{
	struct dwc3_sec *snoti = &sec_noti;
	struct dwc3_msm *dwcm;

	pr_info("%s: event %d\n", __func__, enable);

	if (!snoti) {
		pr_err("%s: dwc3_otg (snoti) is null\n", __func__);
		return NOTIFY_BAD;
	}

	dwcm = snoti->dwcm;
	if (!dwcm) {
		pr_err("%s: dwc3_otg (dwcm) is null\n", __func__);
		return NOTIFY_BAD;
	}

	if (enable) {
		pr_info("ID clear\n");
		sec_otg_ext_notify(dwcm, 1);
	} else {
		pr_info("ID set\n");
		sec_otg_ext_notify(dwcm, 0);
	}

	return 0;
}
EXPORT_SYMBOL(sec_handle_event);

static int sec_otg_notifications(struct notifier_block *nb,
				   unsigned long event, void *unused)
{
	struct dwc3_sec *snoti = container_of(nb, struct dwc3_sec, nb);
	struct dwc3_msm *dwcm;

	pr_info("%s: event %lu\n", __func__, event);

	if (!snoti) {
		pr_err("%s: dwc3_otg (snoti) is null\n", __func__);
		return NOTIFY_BAD;
	}

	dwcm = snoti->dwcm;
	if (!dwcm) {
		pr_err("%s: dwc3_otg (dwcm) is null\n", __func__);
		return NOTIFY_BAD;
	}

	switch (event) {
	case HNOTIFY_NONE: break;
	case HNOTIFY_VBUS: break;
	case HNOTIFY_ID:
		pr_info("ID clear\n");
		sec_otg_ext_notify(dwcm, 1);
		break;
	case HNOTIFY_CHARGER: break;
	case HNOTIFY_ENUMERATED: break;
	case HNOTIFY_ID_PULL:
		pr_info("ID set\n");
		sec_otg_ext_notify(dwcm, 0);
		break;
	case HNOTIFY_OVERCURRENT: break;
	case HNOTIFY_OTG_POWER_ON: break;
	case HNOTIFY_OTG_POWER_OFF: break;
	case HNOTIFY_SMARTDOCK_ON: 
		pr_info("ID clear\n");
		sec_otg_ext_notify(dwcm, 1);
		break;
	case HNOTIFY_SMARTDOCK_OFF: 
		pr_info("ID set\n");
		sec_otg_ext_notify(dwcm, 0);
		break;
	case HNOTIFY_AUDIODOCK_ON: break;
	case HNOTIFY_AUDIODOCK_OFF: break;
	default:
		break;
	}

	return NOTIFY_OK;
}

#ifdef CONFIG_EXTCON
struct sec_cable {
	struct work_struct work;
	struct notifier_block nb;
	struct extcon_specific_cable_nb extcon_nb;
	struct extcon_dev *edev;
	enum extcon_cable_name cable_type;
	int cable_state;
};

static struct sec_cable support_cable_list[] = {
	{ .cable_type = EXTCON_USB, },
#ifdef CONFIG_USB_HOST_NOTIFY
	{ .cable_type = EXTCON_USB_HOST, },
	{ .cable_type = EXTCON_USB_HOST_5V, },
	{ .cable_type = EXTCON_TA, },
	{ .cable_type = EXTCON_AUDIODOCK, },
	{ .cable_type = EXTCON_SMARTDOCK_TA, },
#endif
	{ .cable_type = EXTCON_SMARTDOCK_USB, },
	{ .cable_type = EXTCON_JIG_USBON, },
	{ .cable_type = EXTCON_CHARGE_DOWNSTREAM, },
};

/* USB3.0 Popup option */
#if defined(CONFIG_SEC_K_PROJECT)
extern u8 usb30en;
#endif

extern void set_ncm_ready(bool ready);
static void sec_usb_work(int usb_mode)
{
	struct power_supply *psy;

#if defined(CONFIG_SEC_K_PROJECT)
	gpio_set_value(sec_qcom_usb_rdrv, usb_mode);
	pr_info("%s klte_usb_rdrv_pin = %d, enable=%d\n",
		__func__,
		sec_qcom_usb_rdrv,
		usb_mode);
	if(!usb_mode)
/* USB3.0 Popup option */
		usb30en = 0;
#endif
	if(!usb_mode)
		set_ncm_ready(false);

	psy = power_supply_get_by_name("dwc-usb");
	pr_info("usb: dwc3 power supply set(%d)", usb_mode);
	power_supply_set_present(psy, usb_mode);
}

static void sec_cable_event_worker(struct work_struct *work)
{
	struct sec_cable *cable =
			    container_of(work, struct sec_cable, work);
#ifdef CONFIG_USB_HOST_NOTIFY
	int usb_block_mode;
#endif

	pr_info("sec otg: %s is %s\n",
		extcon_cable_name[cable->cable_type],
		cable->cable_state ? "attached" : "detached");

#ifdef CONFIG_USB_HOST_NOTIFY
	usb_block_mode = check_usb_block_type();
#endif

	switch (cable->cable_type) {
	case EXTCON_USB:
	case EXTCON_SMARTDOCK_USB:
	case EXTCON_JIG_USBON:
	case EXTCON_CHARGE_DOWNSTREAM:
#ifdef CONFIG_USB_HOST_NOTIFY
		if (usb_block_mode == NOTIFY_BLOCK_TYPE_NONE || usb_block_mode == NOTIFY_BLOCK_TYPE_HOST)
#endif
			sec_usb_work(cable->cable_state);
#ifdef CONFIG_USB_HOST_NOTIFY
		if (cable->cable_state)
			sec_otg_notify(HNOTIFY_VBUS);
		else
			sec_otg_notify(HNOTIFY_NONE);
#endif
		break;
#ifdef CONFIG_USB_HOST_NOTIFY
	case EXTCON_USB_HOST:
		if (cable->cable_state)
			sec_otg_notify(HNOTIFY_ID);
		else
			sec_otg_notify(HNOTIFY_ID_PULL);
		break;
	case EXTCON_TA: break;
	case EXTCON_AUDIODOCK:
		if (cable->cable_state)
			sec_otg_notify(HNOTIFY_AUDIODOCK_ON);
		else
			sec_otg_notify(HNOTIFY_AUDIODOCK_OFF);
		break;
	case EXTCON_SMARTDOCK_TA:
		if (cable->cable_state)
			sec_otg_notify(HNOTIFY_SMARTDOCK_ON);
		else
			sec_otg_notify(HNOTIFY_SMARTDOCK_OFF);
		break;
	case EXTCON_USB_HOST_5V:
		if (cable->cable_state)
			sec_otg_notify(HNOTIFY_OTG_POWER_ON);
		else
			sec_otg_notify(HNOTIFY_OTG_POWER_OFF);
		break;
#endif
	default : break;
	}
}

static int sec_cable_notifier(struct notifier_block *nb,
					unsigned long stat, void *ptr)
{
	struct sec_cable *cable =
			container_of(nb, struct sec_cable, nb);

	cable->cable_state = stat;

	schedule_work(&cable->work);

	return NOTIFY_DONE;
}

static int __init sec_otg_init_cable_notify(void)
{
	struct sec_cable *cable;
	int i;
	int ret;

	pr_info("%s register extcon notifier for usb and ta\n", __func__);
	for (i = 0; i < ARRAY_SIZE(support_cable_list); i++) {
		cable = &support_cable_list[i];

		INIT_WORK(&cable->work, sec_cable_event_worker);
		cable->nb.notifier_call = sec_cable_notifier;

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
device_initcall_sync(sec_otg_init_cable_notify);
#endif

static int sec_otg_init(struct dwc3_msm *dwcm, struct usb_phy *phy)
{
	int ret = 0;

	pr_info("%s: register notifier\n", __func__);
	sec_noti.nb.notifier_call = sec_otg_notifications;
	sec_noti.dwcm = dwcm;

#if defined (CONFIG_CHARGER_BQ24260) || defined (CONFIG_CHARGER_SMB358) || defined (CONFIG_CHARGER_SMB1357)
	usb_vbus_msm_init(dwcm, phy);
#endif

#if 0
	ret = usb_register_notifier(phy, &sec_noti.nb);
	if (ret) {
		pr_err("%s: usb_register_notifier failed\n", __func__);
		return ret;
	}
#endif
	return ret;
}
