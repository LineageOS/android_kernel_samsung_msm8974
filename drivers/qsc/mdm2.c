/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/debugfs.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <asm/mach-types.h>
#include "mdm2.h"
#include <mach/restart.h>
#include <mach/subsystem_notif.h>
#include <mach/subsystem_restart.h>
#include <linux/msm_charm.h>
#include <mach/../../msm_watchdog.h>
#include <mach/../../devices.h>
#include <mach/../../clock.h>
#include "mdm_private.h"

#define MDM_PBLRDY_CNT		20

static int mdm_debug_mask;
extern int poweroff_charging;

static void mdm_peripheral_connect(struct mdm_modem_drv *mdm_drv)
{
	pr_err("%s %d\n", __func__,__LINE__);
	if (!mdm_drv->pdata->peripheral_platform_device)
		return;
	pr_err("%s %d\n", __func__,__LINE__);

	mutex_lock(&mdm_drv->peripheral_status_lock);
	if (mdm_drv->peripheral_status)
		goto out;
	pr_err("%s %d\n", __func__,__LINE__);
	platform_device_add(mdm_drv->pdata->peripheral_platform_device);
	pr_err("%s %d\n", __func__,__LINE__);
	mdm_drv->peripheral_status = 1;
out:
	mutex_unlock(&mdm_drv->peripheral_status_lock);
	pr_err("%s %d\n", __func__,__LINE__);
}

static void mdm_peripheral_disconnect(struct mdm_modem_drv *mdm_drv)
{
	if (!mdm_drv->pdata->peripheral_platform_device)
		return;

	mutex_lock(&mdm_drv->peripheral_status_lock);
	if (!mdm_drv->peripheral_status)
		goto out;
	platform_device_del(mdm_drv->pdata->peripheral_platform_device);
	mdm_drv->peripheral_status = 0;
out:
	mutex_unlock(&mdm_drv->peripheral_status_lock);
}

static void mdm_do_clean_reset(struct mdm_modem_drv *mdm_drv)
{
	/* mdm clean reset 
	 *
	 * Shutdown PMIC and up if needed
	 */

#if 0  // no need workardoun code.
	if (mdm_drv->need_clean_reset)	{
		gpio_direction_output(MDM_GPIO(AP2MDM_PMIC_PWR_EN), 0);
		mdelay(10);
		gpio_direction_output(MDM_GPIO(AP2MDM_PMIC_PWR_EN), 1);
		mdelay(10);

		mdm_drv->need_clean_reset = false;
	}
#endif
}

/* This function can be called from atomic context. */
static void mdm_toggle_soft_reset(struct mdm_modem_drv *mdm_drv)
{
	int soft_reset_direction_assert = 0,
	    soft_reset_direction_de_assert = 1;

	if (mdm_drv->pdata->soft_reset_inverted) {
		soft_reset_direction_assert = 1;
		soft_reset_direction_de_assert = 0;
	}
#if 0// TD_CDMA
	gpio_direction_output(mdm_drv->ap2mdm_soft_reset_gpio,
			soft_reset_direction_assert);
	/* Use mdelay because this function can be called from atomic
	 * context.
	 */
	mdelay(10);
	gpio_direction_output(mdm_drv->ap2mdm_soft_reset_gpio,
			soft_reset_direction_de_assert);
#endif//TD_CDMA
	gpio_direction_output(MDM_GPIO(AP2MDM_SOFT_RESET), 
			soft_reset_direction_assert);
	mdelay(10);
	
	mdm_do_clean_reset(mdm_drv);

	gpio_direction_output(MDM_GPIO(AP2MDM_SOFT_RESET), 
			soft_reset_direction_de_assert);

	gpio_direction_output(MDM_GPIO(AP2MDM_KPDPWR), 1);
	mdelay(1000);
	gpio_direction_output(MDM_GPIO(AP2MDM_KPDPWR), 0);

}

/* This function can be called from atomic context. */
static void mdm_atomic_soft_reset(struct mdm_modem_drv *mdm_drv)
{
	mdm_toggle_soft_reset(mdm_drv);
}

static void mdm_power_down_common(struct mdm_modem_drv *mdm_drv)
{
	int i = 0; 
	int soft_reset_direction =
		mdm_drv->pdata->soft_reset_inverted ? 1 : 0;

	mdm_peripheral_disconnect(mdm_drv);

#if 0	// because we don't use the graceful shutdown function, we don't need this checking 
	/* Wait for the modem to complete its power down actions. */ 
	for (i = 20; i > 0; i--) { 
		if (gpio_get_value(MDM_GPIO(MDM2AP_STATUS)) == 0) { 
			if (mdm_debug_mask & MDM_DEBUG_MASK_SHDN_LOG) 
				pr_debug("%s:id %d: mdm2ap_statuswent low, i=%d\n", 
				__func__, mdm_drv->device_id, i); 
			break; 
		} 
		msleep(100); 
	} 
#endif

	/* Assert the soft reset line whether mdm2ap_status went low or not */
	gpio_direction_output(MDM_GPIO(AP2MDM_SOFT_RESET), soft_reset_direction); 
	if (i == 0) { 
		pr_debug("%s:id %d: MDM2AP_STATUS never went low. Doing a hard reset\n", 
			__func__, mdm_drv->device_id); 
		gpio_direction_output(MDM_GPIO(AP2MDM_SOFT_RESET), soft_reset_direction); 
	}
	/* 
	* Currently, there is a debounce timer on the charm PMIC. It is 
	* necessary to hold the PMIC RESET low for ~3.5 seconds 
	* for the reset to fully take place. Sleep here to ensure the 
	* reset has occured before the function exits. 
	*/ 
	msleep(4000); 
}

static void mdm_do_first_power_on(struct mdm_modem_drv *mdm_drv)
{
#ifdef USE_MDM_MODEM
	int i;
	int pblrdy;
#endif
	gpio_direction_output(MDM_GPIO(AP2MDM_SOFT_RESET), 0);
	gpio_direction_output(MDM_GPIO(AP2MDM_PMIC_PWR_EN), 1);
	msleep(10);

	if (mdm_drv->power_on_count != 1) {
		pr_debug("%s:id %d: Calling fn when power_on_count != 1\n",
			   __func__, mdm_drv->device_id);
		return;
	}
	pr_err("%s \n", __func__);

	pr_err("%s:id %d: Powering on modem for the first time\n",
		   __func__, mdm_drv->device_id);
	mdm_peripheral_disconnect(mdm_drv);

	/* If this is the first power-up after a panic, the modem may still
	 * be in a power-on state, in which case we need to toggle the gpio
	 * instead of just de-asserting it. No harm done if the modem was
	 * powered down.
	 */

	if (!mdm_drv->pdata->no_reset_on_first_powerup)
		mdm_toggle_soft_reset(mdm_drv);

	/* If the device has a kpd pwr gpio then toggle it. */
	if (GPIO_IS_VALID(MDM_GPIO(AP2MDM_KPDPWR))) { // HSLEE qsc en
		/* Pull AP2MDM_KPDPWR gpio high and wait for PS_HOLD to settle,
		 * then	pull it back low.
		 */
		pr_err("%s:id %d: Pulling AP2MDM_KPDPWR gpio high\n",
				 __func__, mdm_drv->device_id);
		gpio_direction_output(MDM_GPIO(AP2MDM_KPDPWR), 1);
		//gpio_direction_output(MDM_GPIO(AP2MDM_STATUS), 1);
		msleep(1000);
		gpio_direction_output(MDM_GPIO(AP2MDM_KPDPWR), 0);
	}//else

		gpio_direction_output(MDM_GPIO(AP2MDM_STATUS), 1);

	msleep(5000);
	if (!gpio_get_value(MDM_GPIO(MDM2AP_STATUS)))	{
		pr_err("%s: QSC failed. reboot\n", __func__);

		gpio_direction_output(MDM_GPIO(AP2MDM_STATUS), 0);
		gpio_direction_output(MDM_GPIO(AP2MDM_PMIC_PWR_EN), 0);
		msleep(5000);
		gpio_direction_output(MDM_GPIO(AP2MDM_PMIC_PWR_EN), 1);
		msleep(10);
		gpio_direction_output(MDM_GPIO(AP2MDM_KPDPWR), 1);
		msleep(1000);
		gpio_direction_output(MDM_GPIO(AP2MDM_KPDPWR), 0);
		gpio_direction_output(MDM_GPIO(AP2MDM_STATUS), 1);
	}

	/* check if qsc is in dload mode */
	if (gpio_get_value(MDM_GPIO(MDM2AP_ERRFATAL)))	{
		pr_err("%s: QSC is in dload. reset\n", __func__);
		gpio_direction_output(MDM_GPIO(AP2MDM_SOFT_RESET), 1);
		mdelay(10);
		gpio_direction_output(MDM_GPIO(AP2MDM_SOFT_RESET), 0);
	}

#ifdef USE_MDM_MODEM
	if (!GPIO_IS_VALID(MDM_GPIO(MDM2AP_PBLRDY)))
		goto start_mdm_peripheral;

	for (i = 0; i  < MDM_PBLRDY_CNT; i++) {
		pblrdy = gpio_get_value(MDM_GPIO(MDM2AP_PBLRDY));
		if (pblrdy)
			break;
		usleep_range(5000, 5000);
	}
	pr_info("%s: id %d: pblrdy i:%d\n", __func__,
			 mdm_drv->device_id, i);


start_mdm_peripheral:
#endif	
	pr_err("%s %d\n", __func__,__LINE__);
	mdm_peripheral_connect(mdm_drv);
	msleep(200);
}

static void mdm_do_soft_power_on(struct mdm_modem_drv *mdm_drv)
{
#ifdef USE_MDM_MODEM
	int i;
	int pblrdy;
#endif

	pr_err("%s: id %d:  soft resetting mdm modem\n",
		   __func__, mdm_drv->device_id);
	mdm_peripheral_disconnect(mdm_drv);
	mdm_toggle_soft_reset(mdm_drv);

#ifdef USE_MDM_MODEM
	if (!GPIO_IS_VALID(MDM_GPIO(MDM2AP_PBLRDY)))
		goto start_mdm_peripheral;

	for (i = 0; i  < MDM_PBLRDY_CNT; i++) {
		pblrdy = gpio_get_value(MDM_GPIO(MDM2AP_PBLRDY));
		if (pblrdy)
			break;
		usleep_range(5000, 5000);
	}

	pr_info("%s: id %d: pblrdy i:%d\n", __func__,
			 mdm_drv->device_id, i);


start_mdm_peripheral:
#endif	

	mdm_peripheral_connect(mdm_drv);
	msleep(200);
}

static void mdm_power_on_common(struct mdm_modem_drv *mdm_drv)
{
	mdm_drv->power_on_count++;

	/* this gpio will be used to indicate apq readiness,
	 * de-assert it now so that it can be asserted later.
	 * May not be used.
	 */

	pr_err("%s \n", __func__);
	if (GPIO_IS_VALID(MDM_GPIO(AP2MDM_CHNLRDY)))
		gpio_direction_output(MDM_GPIO(AP2MDM_CHNLRDY), 0);

	/*
	 * If we did an "early power on" then ignore the very next
	 * power-on request because it would the be first request from
	 * user space but we're already powered on. Ignore it.
	 */
	if (mdm_drv->pdata->early_power_on &&
			(mdm_drv->power_on_count == 2))
		return;

	if(poweroff_charging){
		pr_debug("%s: do not power on in lpm\n", __func__);
		return;
	}

	if (mdm_drv->power_on_count == 1)
		mdm_do_first_power_on(mdm_drv);
	else
		mdm_do_soft_power_on(mdm_drv);
}

static void debug_state_changed(int value)
{
	mdm_debug_mask = value;
}

static void mdm_status_changed(struct mdm_modem_drv *mdm_drv, int value)
{

	if (!mdm_drv->pdata->peripheral_platform_device)
		return;
	
	pr_debug("%s: id %d: value:%d\n", __func__,
			 value, mdm_drv->device_id);


	if (value) {
		mdm_peripheral_disconnect(mdm_drv);
		msleep(100);
		mdm_peripheral_connect(mdm_drv);
		if (GPIO_IS_VALID(MDM_GPIO(AP2MDM_CHNLRDY)))
			gpio_direction_output(MDM_GPIO(AP2MDM_CHNLRDY), 0);
	}
}

static void mdm_image_upgrade(struct mdm_modem_drv *mdm_drv, int type)
{
	switch (type) {
	case APQ_CONTROLLED_UPGRADE:
		pr_info("%s: id %d: APQ controlled modem image upgrade\n",
				 __func__, mdm_drv->device_id);
		atomic_set(&mdm_drv->mdm_ready, 0);
		mdm_toggle_soft_reset(mdm_drv);
		break;
	case MDM_CONTROLLED_UPGRADE:
		pr_info("%s: id %d: MDM controlled modem image upgrade\n",
				 __func__, mdm_drv->device_id);
		atomic_set(&mdm_drv->mdm_ready, 0);
		/*
		 * If we have no image currently present on the modem, then we
		 * would be in PBL, in which case the status gpio would not go
		 * high.
		 */
		mdm_drv->disable_status_check = 1;
		if (GPIO_IS_VALID(MDM_GPIO(USB_SW))) {
			pr_debug("%s: id %d: Switching usb control to MDM\n",
					__func__, mdm_drv->device_id);
			gpio_direction_output(MDM_GPIO(USB_SW), 1);
		} else
			pr_err("%s: id %d: usb switch gpio unavailable\n",
				   __func__, mdm_drv->device_id);
		break;
	default:
		pr_err("%s: id %d: invalid upgrade type\n",
			   __func__, mdm_drv->device_id);
	}
}

static struct mdm_ops mdm_cb = {
	.power_on_mdm_cb = mdm_power_on_common,
	.reset_mdm_cb = mdm_power_on_common,
	.atomic_reset_mdm_cb = mdm_atomic_soft_reset,
	.power_down_mdm_cb = mdm_power_down_common,
	.debug_state_changed_cb = debug_state_changed,
	.status_cb = mdm_status_changed,
	.image_upgrade_cb = mdm_image_upgrade,
};

int mdm_get_ops(struct mdm_ops **mdm_ops)
{
	*mdm_ops = &mdm_cb;
	return 0;
}


