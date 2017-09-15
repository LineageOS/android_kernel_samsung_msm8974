/*
 * drivers/usb/otg/msm_otg_sec.c
 *
 * Copyright (c) 2013, Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/gpio.h>
#include <linux/host_notify.h>
#ifdef CONFIG_USB_SWITCH_FSA9485
#include <linux/i2c/fsa9485.h>
#endif
#ifdef CONFIG_MFD_MAX77693
#include <linux/mfd/max77693.h>
#endif

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "otg %s %d: " fmt, __func__, __LINE__

static int ulpi_write(struct usb_phy *phy, u32 val, u32 reg);
static int ulpi_read(struct usb_phy *phy, u32 reg);
static void msm_hsusb_vbus_power(struct msm_otg *motg, bool on);

#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
extern bool lanhub_ta_case;
#endif
#ifdef CONFIG_EXTCON_MAX77804K
extern int muic_otg_control(int enable);
#endif
#ifndef CONFIG_EXTCON_MAX77804K
int sec_battery_otg_control(int enable)
{
	union power_supply_propval value;
	struct power_supply *psy;
	int current_cable_type;
	int ret = 0;

	pr_info("%s: enable(%d)\n", __func__, enable);

	psy = power_supply_get_by_name("battery");
	if (!psy) {
		pr_err("%s: ERROR! failed to get battery!\n", __func__);
		return -1;
	}

#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
if (enable) {
		current_cable_type = lanhub_ta_case ? POWER_SUPPLY_TYPE_LAN_HUB : POWER_SUPPLY_TYPE_OTG;
		pr_info("%s: LANHUB+TA Case cable type change for the (%d) \n",
								__func__,current_cable_type);
}
#else
	if (enable)
		current_cable_type = POWER_SUPPLY_TYPE_OTG;
#endif
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
#endif

struct booster_data sec_booster_batt = {
	.name = "sec_battery",
#ifdef CONFIG_EXTCON_MAX77804K
	.boost = muic_otg_control,
#else
	.boost = sec_battery_otg_control,
#endif
};

int msm_otg_sec_power(bool on)
{
	int ret = 0;
	pr_info("msm_otg_sec_power: %d\n", on);
#if defined(CONFIG_MFD_MAX77693) || defined(CONFIG_EXTCON_MAX77804K)
	muic_otg_control(on);
#else
	ret = sec_battery_otg_control(on);
#endif
	return ret;
}

static void msm_otg_late_power_work(struct work_struct *w)
{
	struct msm_otg *motg = container_of((struct delayed_work *)w,
				struct msm_otg, late_power_work);
	int booster = sec_get_notification(HNOTIFY_BOOSTER);

	pr_info("%s, ID=%d, booster=%d\n", __func__,
			test_bit(ID, &motg->inputs), booster);

	if (!test_bit(ID, &motg->inputs) && (booster == NOTIFY_POWER_OFF)) {
		msm_hsusb_vbus_power(motg, 0);
		msleep(100);
		msm_hsusb_vbus_power(motg, 1);
	}
}

static void msm_otg_host_phy_tune(struct msm_otg *otg,
		u32 paramb, u32 paramc, u32 paramd)
{
	pr_info("ULPI 0x%x: 0x%x: 0x%x: 0x%x - orig\n",
			ulpi_read(&otg->phy, 0x80),
			ulpi_read(&otg->phy, 0x81),
			ulpi_read(&otg->phy, 0x82),
			ulpi_read(&otg->phy, 0x83));

	ulpi_write(&otg->phy, paramb, 0x81);
	ulpi_write(&otg->phy, paramc, 0x82);
	ulpi_write(&otg->phy, paramd, 0x83);

	pr_info("ULPI 0x%x: 0x%x: 0x%x: 0x%x\n",
			ulpi_read(&otg->phy, 0x80),
			ulpi_read(&otg->phy, 0x81),
			ulpi_read(&otg->phy, 0x82),
			ulpi_read(&otg->phy, 0x83));
	mdelay(100);
}

static int msm_otg_host_notify_set(struct msm_otg *motg, int state)
{
	pr_info("boost : %d\n", state);

	if (state)
		sec_otg_notify(HNOTIFY_OTG_POWER_ON);
	else
		sec_otg_notify(HNOTIFY_OTG_POWER_OFF);

	return state;
}

static void msm_otg_host_notify(struct msm_otg *motg, int on)
{
	pr_info("host_notify: %d, dock %d\n", on, motg->smartdock);

	if (on)
		msm_otg_host_phy_tune(motg, 0x33, 0xB, 0x13);
}

static int msm_host_notify_init(struct device *dev, struct msm_otg *motg)
{
	sec_otg_register_booster(&sec_booster_batt);
	INIT_DELAYED_WORK(&motg->late_power_work, msm_otg_late_power_work);
	return 0;
}

static int msm_host_notify_exit(struct device *dev, struct msm_otg *motg)
{
	cancel_delayed_work_sync(&motg->late_power_work);
	return 0;
}

/*
 * Exported functions
 */

void sec_otg_set_dock_state(int enable)
{
	struct msm_otg *motg = the_msm_otg;
	struct usb_phy *phy = &motg->phy;

	if (enable) {
		pr_info("DOCK : attached\n");
		motg->smartdock = true;
		clear_bit(ID, &motg->inputs);

		if (atomic_read(&motg->in_lpm)) {
			pr_info("DOCK : in LPM\n");
			pm_runtime_resume(phy->dev);
		}

		if (test_bit(B_SESS_VLD, &motg->inputs)) {
			pr_info("clear B_SESS_VLD\n");
			clear_bit(B_SESS_VLD, &motg->inputs);
		}
		/* use use non-reentrant wq, so that we don't run sm_work on multiple cpus */
		queue_work(system_nrt_wq, &motg->sm_work);

	} else {
		pr_info("DOCK : detached\n");
		motg->smartdock = false;
		set_bit(ID, &motg->inputs);
	}

}
EXPORT_SYMBOL(sec_otg_set_dock_state);

void sec_otg_set_id_state(int id)
{
	struct msm_otg *motg = the_msm_otg;
	struct usb_phy *phy = &motg->phy;

	pr_info("msm_otg_set_id_state is called, ID : %d\n", id);

	if (id)
		set_bit(ID, &motg->inputs);
	else
		clear_bit(ID, &motg->inputs);

	if (atomic_read(&motg->in_lpm)) {
		pr_info("msm_otg_set_id_state : in LPM\n");
		pm_runtime_resume(phy->dev);
	}
	/* defer work-handling until pm_resume callback is handled */
	if (motg->phy.state != OTG_STATE_UNDEFINED) {
		if (atomic_read(&motg->pm_suspended))
			motg->sm_work_pending = true;
		else
		/* use use non-reentrant wq, so that we don't run sm_work on multiple cpus */
			queue_work(system_nrt_wq, &motg->sm_work);
	}
}
EXPORT_SYMBOL(sec_otg_set_id_state);

void msm_otg_set_smartdock_state(bool online)
{
	struct msm_otg *motg = the_msm_otg;

	if (online) {
		dev_info(motg->phy.dev, "SMARTDOCK : ID set\n");
		motg->smartdock = false;
		set_bit(ID, &motg->inputs);
	} else {
		dev_info(motg->phy.dev, "SMARTDOCK : ID clear\n");
		motg->smartdock = true;
		clear_bit(ID, &motg->inputs);
	}
	if (test_bit(B_SESS_VLD, &motg->inputs))
		clear_bit(B_SESS_VLD, &motg->inputs);
	if (atomic_read(&motg->pm_suspended))
		motg->sm_work_pending = true;
	else
		queue_work(system_nrt_wq, &motg->sm_work);
}
EXPORT_SYMBOL_GPL(msm_otg_set_smartdock_state);

int sec_handle_event(int enable)
{
	sec_otg_set_id_state(!enable);
	return 0;
}
EXPORT_SYMBOL(sec_handle_event);
