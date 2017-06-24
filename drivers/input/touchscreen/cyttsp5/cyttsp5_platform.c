/*
 * cyttsp5_platform.c
 * Cypress TrueTouch(TM) Standard Product V4 Platform Module.
 * For use with Cypress Txx4xx parts.
 * Supported parts include:
 * TMA4XX
 * TMA1036
 *
 * Copyright (C) 2013 Cypress Semiconductor
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

#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/cyttsp5_core.h>
#include <linux/cyttsp5_platform.h>

#define GPIO_TSP_nINT_SECURE	17

#if defined(CONFIG_SEC_ATLANTIC_PROJECT)
#include <linux/of_gpio.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP5_PLATFORM_FW_UPGRADE
/* #include "TSG5M_Kmini_rev_0C HW0x01FW0x0200.h" */
#include "cyttsp5_firmware.h"

static struct cyttsp5_touch_firmware cyttsp5_firmware = {
	.img = cyttsp4_img,
	.size = ARRAY_SIZE(cyttsp4_img),
	.ver = cyttsp4_ver,
	.vsize = ARRAY_SIZE(cyttsp4_ver),
	.hw_version = CY_HW_VERSION,
	.fw_version = CY_FW_VERSION,
};
#else
static struct cyttsp5_touch_firmware cyttsp5_firmware = {
	.img = NULL,
	.size = 0,
	.ver = NULL,
	.vsize = 0,
};
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP5_PLATFORM_TTCONFIG_UPGRADE
#include "cyttsp5_params.h"
static struct touch_settings cyttsp5_sett_param_regs = {
	.data = (uint8_t *)&cyttsp4_param_regs[0],
	.size = ARRAY_SIZE(cyttsp4_param_regs),
	.tag = 0,
};

static struct touch_settings cyttsp5_sett_param_size = {
	.data = (uint8_t *)&cyttsp4_param_size[0],
	.size = ARRAY_SIZE(cyttsp4_param_size),
	.tag = 0,
};

static struct cyttsp5_touch_config cyttsp5_ttconfig = {
	.param_regs = &cyttsp5_sett_param_regs,
	.param_size = &cyttsp5_sett_param_size,
	.fw_ver = ttconfig_fw_ver,
	.fw_vsize = ARRAY_SIZE(ttconfig_fw_ver),
};
#else
static struct cyttsp5_touch_config cyttsp5_ttconfig = {
	.param_regs = NULL,
	.param_size = NULL,
	.fw_ver = NULL,
	.fw_vsize = 0,
};
#endif

struct cyttsp5_loader_platform_data _cyttsp5_loader_platform_data = {
	.fw = &cyttsp5_firmware,
	.ttconfig = &cyttsp5_ttconfig,
	.flags = CY_LOADER_FLAG_CALIBRATE_AFTER_FW_UPGRADE,
};

int enabled = 0;

#ifdef CONFIG_SEC_ATLANTIC_PROJECT
extern int avdd_gpio;
struct regulator *iovdd_vreg;
#endif

static int cyttsp5_hw_power(struct cyttsp5_core_platform_data *pdata, int on)
{

#if !defined(CONFIG_SEC_ATLANTIC_PROJECT)
	int pwr_1p8 = pdata->pwr_1p8;
	int pwr_3p3 = pdata->pwr_3p3;
#endif


#ifdef CONFIG_SEC_ATLANTIC_PROJECT

	int ret;

        if(on){
                if (iovdd_vreg)
                        regulator_enable(iovdd_vreg);
        }else{
			if (iovdd_vreg)
				regulator_disable(iovdd_vreg);
        }

	ret = gpio_direction_output(avdd_gpio, on);
	if (ret) {
		pr_err("[TSP]%s: unable to set_direction for gpio[%d] %d\n",
			 __func__, avdd_gpio, ret);
		return -EINVAL;
	}
#else
	int ret;


	ret = gpio_direction_output(pwr_1p8, on);
	if (ret) {
		pr_err("[TKEY]%s: unable to set_direction for vdd_led [%d]\n",
			 __func__, pwr_1p8);
		return -EINVAL;
	}
	ret = gpio_direction_output(pwr_3p3, on);
	if (ret) {
		pr_err("[TKEY]%s: unable to set_direction for vdd_led [%d]\n",
			 __func__, pwr_3p3);
		return -EINVAL;
	}
	msleep(50);
#endif
	return 0;
}

int cyttsp5_xres(struct cyttsp5_core_platform_data *pdata,
		struct device *dev)
{
	int rc;

	rc = cyttsp5_hw_power(pdata, 0);
	if (rc) {
		dev_err(dev, "%s: Fail power down HW\n", __func__);
		goto exit;
	}

	msleep(200);

	rc = cyttsp5_hw_power(pdata, 1);
	if (rc) {
		dev_err(dev, "%s: Fail power up HW\n", __func__);
		goto exit;
	}
	msleep(100);

exit:
	return rc;
}

int cyttsp5_init(struct cyttsp5_core_platform_data *pdata,
		int on, struct device *dev)
{
	int irq_gpio = pdata->irq_gpio;
	int rc = 0;
#if !defined(CONFIG_SEC_ATLANTIC_PROJECT)
	int pwr_1p8 = pdata->pwr_1p8;
	int pwr_3p3 = pdata->pwr_3p3;
 #endif

#ifdef CONFIG_SEC_ATLANTIC_PROJECT
	iovdd_vreg = regulator_get(dev,"vddo");
        if (IS_ERR(iovdd_vreg)){
                iovdd_vreg = NULL;
                dev_err(dev, "[TSP] iovdd_vreg error %ld\n",
						IS_ERR(iovdd_vreg));
        }
#endif

	enabled = 0;

	if (on) {
		gpio_request(irq_gpio, "TSP_INT");
		gpio_direction_input(irq_gpio);
#ifdef CONFIG_SEC_ATLANTIC_PROJECT
		gpio_request(avdd_gpio, "TSP_AVDD_gpio");
#else
		gpio_request(pwr_1p8, "TSP_1p8_GPIO");
		gpio_request(pwr_3p3, "TSP_3p3_GPIO");
#endif
		cyttsp5_hw_power(pdata, 1);
	} else {
		cyttsp5_hw_power(pdata, 0);
		if(!gpio_is_valid(irq_gpio))
			gpio_free(irq_gpio);
	}

	dev_info(dev,
		"%s: INIT CYTTSP IRQ gpio=%d onoff=%d r=%d\n",
		__func__, irq_gpio, on, rc);
	return rc;
}

int cyttsp5_power(struct cyttsp5_core_platform_data *pdata,
		int on, struct device *dev, atomic_t *ignore_irq)
{
	return cyttsp5_hw_power(pdata, on);
}

int cyttsp5_irq_stat(struct cyttsp5_core_platform_data *pdata,
		struct device *dev)
{
	return gpio_get_value(pdata->irq_gpio);
}

#ifndef CONFIG_TOUCHSCREEN_CYTTSP5_DEVICETREE_SUPPORT // platform data of board file
#include <linux/input.h>
#include <linux/export.h>

#include <linux/cyttsp5_core.h>
#include <linux/cyttsp5_platform.h>

#define CYTTSP5_HID_DESC_REGISTER 1

#define CY_MAXX 720
#define CY_MAXY 1280
#define CY_MINX 0
#define CY_MINY 0

#define CY_ABS_MIN_X CY_MINX
#define CY_ABS_MIN_Y CY_MINY
#define CY_ABS_MAX_X CY_MAXX
#define CY_ABS_MAX_Y CY_MAXY
#define CY_ABS_MIN_P 0
#define CY_ABS_MAX_P 255
#define CY_ABS_MIN_W 0
#define CY_ABS_MAX_W 255

#define CY_ABS_MIN_T 0

#define CY_ABS_MAX_T 15

#define CY_IGNORE_VALUE 0xFFFF

static struct cyttsp5_core_platform_data _cyttsp5_core_platform_data = {
	.irq_gpio = GPIO_TSP_nINT_SECURE,
	.hid_desc_register = CYTTSP5_HID_DESC_REGISTER,
	.xres = cyttsp5_xres,
	.init = cyttsp5_init,
	.power = cyttsp5_power,
	.irq_stat = cyttsp5_irq_stat,
	.sett = {
		NULL,	/* Reserved */
		NULL,	/* Command Registers */
		NULL,	/* Touch Report */
		NULL,	/* Cypress Data Record */
		NULL,	/* Test Record */
		NULL,	/* Panel Configuration Record */
		NULL,	/* &cyttsp5_sett_param_regs, */
		NULL,	/* &cyttsp5_sett_param_size, */
		NULL,	/* Reserved */
		NULL,	/* Reserved */
		NULL,	/* Operational Configuration Record */
		NULL, /* &cyttsp5_sett_ddata, *//* Design Data Record */
		NULL, /* &cyttsp5_sett_mdata, *//* Manufacturing Data Record */
		NULL,	/* Config and Test Registers */
		NULL, /* &cyttsp5_sett_btn_keys, */	/* button-to-keycode table */
	},
	//.flags = CY_FLAG_CORE_POWEROFF_ON_SLEEP,
};

static const uint16_t cyttsp5_abs[] = {
	ABS_MT_POSITION_X, CY_ABS_MIN_X, CY_ABS_MAX_X, 0, 0,
	ABS_MT_POSITION_Y, CY_ABS_MIN_Y, CY_ABS_MAX_Y, 0, 0,
	ABS_MT_PRESSURE, CY_ABS_MIN_P, CY_ABS_MAX_P, 0, 0,
	CY_IGNORE_VALUE, CY_ABS_MIN_W, CY_ABS_MAX_W, 0, 0,
	ABS_MT_TRACKING_ID, CY_ABS_MIN_T, CY_ABS_MAX_T, 0, 0,
	ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0,
	ABS_MT_TOUCH_MINOR, 0, 255, 0, 0,
	ABS_MT_ORIENTATION, -128, 127, 0, 0,
	ABS_MT_TOOL_TYPE, 0, MT_TOOL_MAX, 0, 0,
};

struct touch_framework cyttsp5_framework = {
	.abs = (uint16_t *)&cyttsp5_abs[0],
	.size = ARRAY_SIZE(cyttsp5_abs),
	.enable_vkeys = 0,
};

static struct cyttsp5_mt_platform_data _cyttsp5_mt_platform_data = {
	.frmwrk = &cyttsp5_framework,
	.flags = CY_MT_FLAG_NONE,
	.inp_dev_name = "sec_touchscreen",
};

#if !defined(CONFIG_SEC_ATLANTIC_PROJECT)
static struct cyttsp5_btn_platform_data _cyttsp5_btn_platform_data = {
	.inp_dev_name = "sec_touchkey",
};
#endif

struct cyttsp5_platform_data _cyttsp5_platform_data = {
	.core_pdata = &_cyttsp5_core_platform_data,
	.mt_pdata = &_cyttsp5_mt_platform_data,
	.loader_pdata = &_cyttsp5_loader_platform_data,
#if !defined(CONFIG_SEC_ATLANTIC_PROJECT) && !defined(CONFIG_SEC_PATEK_PROJECT)
	.btn_pdata = &_cyttsp5_btn_platform_data,
#endif
};

EXPORT_SYMBOL(_cyttsp5_platform_data);

#endif //#if 1

