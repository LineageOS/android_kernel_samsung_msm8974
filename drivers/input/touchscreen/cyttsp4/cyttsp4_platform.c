/*
 * cyttsp4_platform.c
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
#include <linux/regulator/consumer.h>
#include "cyttsp4_regs.h"

#define CYTTSP4_I2C_IRQ_GPIO 17

#if defined(CONFIG_MACH_AFYONLTE_TMO) || defined(CONFIG_MACH_AFYONLTE_CAN) || (CONFIG_MACH_AFYONLTE_MTR)
extern int system_rev;
#endif


#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_PLATFORM_FW_UPGRADE
#include "Afyon_cyttsp4_img_CY05A200.h"
static struct cyttsp4_touch_firmware cyttsp4_firmware = {
	.img = cyttsp4_img,
	.size = ARRAY_SIZE(cyttsp4_img),
	.ver = cyttsp4_ver,
	.vsize = ARRAY_SIZE(cyttsp4_ver),
#ifdef SAMSUNG_TSP_INFO
	.hw_version = 0x05,
	.fw_version = 0xA200,
	.cfg_version = 0x65,
#endif
};
#else
static struct cyttsp4_touch_firmware cyttsp4_firmware = {
	.img = NULL,
	.size = 0,
	.ver = NULL,
	.vsize = 0,
};
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_PLATFORM_TTCONFIG_UPGRADE
#include "cyttsp4_params.h"
static struct touch_settings cyttsp4_sett_param_regs = {
	.data = (uint8_t *)&cyttsp4_param_regs[0],
	.size = ARRAY_SIZE(cyttsp4_param_regs),
	.tag = 0,
};

static struct touch_settings cyttsp4_sett_param_size = {
	.data = (uint8_t *)&cyttsp4_param_size[0],
	.size = ARRAY_SIZE(cyttsp4_param_size),
	.tag = 0,
};

static struct cyttsp4_touch_config cyttsp4_ttconfig = {
	.param_regs = &cyttsp4_sett_param_regs,
	.param_size = &cyttsp4_sett_param_size,
	.fw_ver = ttconfig_fw_ver,
	.fw_vsize = ARRAY_SIZE(ttconfig_fw_ver),
};
#else
static struct cyttsp4_touch_config cyttsp4_ttconfig = {
	.param_regs = NULL,
	.param_size = NULL,
	.fw_ver = NULL,
	.fw_vsize = 0,
};
#endif

struct cyttsp4_loader_platform_data _cyttsp4_loader_platform_data = {
	.fw = &cyttsp4_firmware,
	.ttconfig = &cyttsp4_ttconfig,
	.sdcard_path = "/storage/sdcard0/cypress_touchscreen/fw.bin",
	.flags = CY_LOADER_FLAG_CALIBRATE_AFTER_FW_UPGRADE,
};


/*************************************************************************************************
 * Power
 *************************************************************************************************/
extern int avdd_gpio;

static int cy_hw_power(int on, int use_irq, int irq_gpio)
{
	int ret = 0;
	
	static struct regulator *lvs1_1p8;

	pr_info("[TSP] power %s\n", on ? "on" : "off");

	if (!lvs1_1p8) {
		lvs1_1p8 = regulator_get(NULL, "8226_lvs1");
		if (IS_ERR(lvs1_1p8)) {
			pr_err("%s: could not get 8226_lvs1, rc = %ld\n",
				__func__, PTR_ERR(lvs1_1p8));
			return -1;
		}
	}

	if (on) {
		ret = regulator_enable(lvs1_1p8);
		if (ret) {
			pr_err("%s: enable lvs1_1p8 failed, rc=%d\n",
				__func__, ret);
			return -1;
		}
		pr_info("%s: tsp lvs1_1p8 on is finished.\n", __func__);

		/* Delay for tsp chip is ready for i2c before enable irq */
		//msleep(20);
		/* Enable the IRQ */
		if (use_irq) {
			//enable_irq(gpio_to_irq(irq_gpio));
			//pr_debug("Enabled IRQ %d for TSP\n", gpio_to_irq(irq_gpio));
		}
		#if defined(CONFIG_MACH_AFYONLTE_TMO) || defined(CONFIG_MACH_AFYONLTE_CAN) || (CONFIG_MACH_AFYONLTE_MTR)
		if(system_rev > 1){
			gpio_tlmm_config(GPIO_CFG(CYTTSP4_I2C_IRQ_GPIO, 0,
					GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
		}
		#endif
	} else {

	/* Disable the IRQ */
		if (use_irq) {
			//pr_debug("Disabling IRQ %d for TSP\n", gpio_to_irq(irq_gpio));
			//disable_irq(gpio_to_irq(irq_gpio));
		}

		if (regulator_is_enabled(lvs1_1p8))
			ret = regulator_disable(lvs1_1p8);
		else
			printk(KERN_ERR
				"%s: rugulator is(lvs1_1p8 disabled\n",
					__func__);
		if (ret) {
			pr_err("%s: disablelvs1_1p8 failed, rc=%d\n",
				__func__, ret);
			return -1;
		}
		pr_info("%s: tsp 1.8V off is finished.\n", __func__);
		#if defined(CONFIG_MACH_AFYONLTE_TMO) || defined(CONFIG_MACH_AFYONLTE_CAN) || (CONFIG_MACH_AFYONLTE_MTR)
		if(system_rev > 1){
			gpio_tlmm_config(GPIO_CFG(CYTTSP4_I2C_IRQ_GPIO, 0,
					GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
		}
		#endif

        /* Delay for 100 msec */
        msleep(100);
	}

	ret = gpio_direction_output(avdd_gpio, on);
	if (ret) {
		pr_err("[TKEY]%s: unable to set_direction for gpio[%d] %d\n",
			 __func__, avdd_gpio, ret);
		return -EINVAL;
	}
	msleep(50);
	return 0;
}
/*************************************************************************************************
 * 
 *************************************************************************************************/
int cyttsp4_xres(struct cyttsp4_core_platform_data *pdata,
		struct device *dev)
{
   int irq_gpio = pdata->irq_gpio;

	int rc = 0;
	printk(" The TOUCH  IRQ no in cyttsp4_xres() is %d",irq_gpio );
	cy_hw_power(0, true, irq_gpio);

	cy_hw_power(1, true, irq_gpio);

	return rc;
}

int cyttsp4_init(struct cyttsp4_core_platform_data *pdata,
		int on, struct device *dev)
{
	int rc = 0;
	int irq_gpio = pdata->irq_gpio;	
	
		if (on) {
		rc = gpio_request(irq_gpio, "TSP_INT");
		if(rc < 0){
			pr_err("%s: unable to request TSP_INT\n", __func__);
			return rc;
		}
		gpio_direction_input(irq_gpio);
		rc = gpio_request(avdd_gpio, "TSP_AVDD_gpio");
		if(rc < 0){
			pr_err("%s: unable to request TSP_AVDD_gpio\n", __func__);
			return rc;
		}
		#if defined(CONFIG_MACH_AFYONLTE_TMO) || defined(CONFIG_MACH_AFYONLTE_CAN) || (CONFIG_MACH_AFYONLTE_MTR)
		if(system_rev > 1){
			gpio_tlmm_config(GPIO_CFG(CYTTSP4_I2C_IRQ_GPIO, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
			}
		else
		#endif	
		{
			gpio_tlmm_config(GPIO_CFG(CYTTSP4_I2C_IRQ_GPIO, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
		}
		cy_hw_power(1, false, 0);
	} else {
		cy_hw_power(0, false, 0);
		gpio_free(irq_gpio);
	}
	dev_info(dev,
		"%s: INIT CYTTSP IRQ gpio=%d onoff=%d r=%d\n",
		__func__, irq_gpio, on, rc);
	return rc;
}

static int cyttsp4_wakeup(struct cyttsp4_core_platform_data *pdata,
		struct device *dev, atomic_t *ignore_irq)
{
	int irq_gpio = pdata->irq_gpio;
	
	return cy_hw_power(1, true, irq_gpio);
}

static int cyttsp4_sleep(struct cyttsp4_core_platform_data *pdata,
		struct device *dev, atomic_t *ignore_irq)
{
    int irq_gpio = pdata->irq_gpio;

	return cy_hw_power(0, true, irq_gpio);
}

int cyttsp4_power(struct cyttsp4_core_platform_data *pdata,
		int on, struct device *dev, atomic_t *ignore_irq)
{
	if (on)
		return cyttsp4_wakeup(pdata, dev, ignore_irq);

	return cyttsp4_sleep(pdata, dev, ignore_irq);
}

int cyttsp4_irq_stat(struct cyttsp4_core_platform_data *pdata,
		struct device *dev)
{
	return gpio_get_value(pdata->irq_gpio);
}

#ifdef CYTTSP4_DETECT_HW
int cyttsp4_detect(struct cyttsp4_core_platform_data *pdata,
		struct device *dev, cyttsp4_platform_read read)
{
	int retry = 3;
	int rc;
	char buf[1];

	while (retry--) {
		/* Perform reset, wait for 100 ms and perform read */
		dev_vdbg(dev, "%s: Performing a reset\n", __func__);
		pdata->xres(pdata, dev);
		msleep(100);
		rc = read(dev, 0, buf, 1);
		if (!rc)
			return 0;

		dev_vdbg(dev, "%s: Read unsuccessful, try=%d\n",
			__func__, 3 - retry);
	}

	return rc;
}
#endif



#if !defined(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICETREE_SUPPORT) &&\
	defined(CYTTSP4_PDATA_IN_PLATFORM_C)
#define CY_MAXX 540
#define CY_MAXY 960
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
#define CY_PROXIMITY_MIN_VAL	0
#define CY_PROXIMITY_MAX_VAL	1

#define CY_ABS_MIN_T 0

#define CY_ABS_MAX_T 15

#define CY_IGNORE_VALUE 0xFFFF

/* Button to keycode conversion */
static u16 cyttsp4_btn_keys[] = {
	/* use this table to map buttons to keycodes (see input.h) */
	//KEY_HOMEPAGE,		/* 172 */ /* Previously was KEY_HOME (102) */
				/* New Android versions use KEY_HOMEPAGE */
	KEY_RECENT,		/* 256 */
	KEY_BACK,		/* 158 */
	//KEY_SEARCH,		/* 217 */
	//KEY_VOLUMEDOWN,		/* 114 */
	//KEY_VOLUMEUP,		/* 115 */
	//KEY_CAMERA,		/* 212 */
	//KEY_POWER		/* 116 */
};

static struct touch_settings cyttsp4_sett_btn_keys = {
	.data = (uint8_t *)&cyttsp4_btn_keys[0],
	.size = ARRAY_SIZE(cyttsp4_btn_keys),
	.tag = 0,
};

static struct cyttsp4_core_platform_data _cyttsp4_core_platform_data = {
	.irq_gpio = CYTTSP4_I2C_IRQ_GPIO,
	//.rst_gpio = CYTTSP4_I2C_RST_GPIO,
	.xres = cyttsp4_xres,
	.init = cyttsp4_init,
	.power = cyttsp4_power,
	//.detect = cyttsp4_detect,
	.irq_stat = cyttsp4_irq_stat,
	.sett = {
		NULL,	/* Reserved */
		NULL,	/* Command Registers */
		NULL,	/* Touch Report */
		NULL,	/* Cypress Data Record */
		NULL,	/* Test Record */
		NULL,	/* Panel Configuration Record */
		NULL,	/* &cyttsp4_sett_param_regs, */
		NULL,	/* &cyttsp4_sett_param_size, */
		NULL,	/* Reserved */
		NULL,	/* Reserved */
		NULL,	/* Operational Configuration Record */
		NULL, /* &cyttsp4_sett_ddata, *//* Design Data Record */
		NULL, /* &cyttsp4_sett_mdata, *//* Manufacturing Data Record */
		NULL,	/* Config and Test Registers */
		&cyttsp4_sett_btn_keys,	/* button-to-keycode table */
	},
	.flags = CY_CORE_FLAG_POWEROFF_ON_SLEEP,
};

static const uint16_t cyttsp4_abs[] = {
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

struct touch_framework cyttsp4_framework = {
	.abs = (uint16_t *)&cyttsp4_abs[0],
	.size = ARRAY_SIZE(cyttsp4_abs),
	.enable_vkeys = 0,
};

static struct cyttsp4_mt_platform_data _cyttsp4_mt_platform_data = {
	.frmwrk = &cyttsp4_framework,
	.flags = CY_MT_FLAG_FLIP | CY_MT_FLAG_INV_Y,
	.inp_dev_name = CYTTSP4_MT_NAME,
};

static struct cyttsp4_btn_platform_data _cyttsp4_btn_platform_data = {
	.inp_dev_name = CYTTSP4_BTN_NAME,
};

struct cyttsp4_platform_data _cyttsp4_platform_data = {
	.core_pdata = &_cyttsp4_core_platform_data,
	.mt_pdata = &_cyttsp4_mt_platform_data,
	.btn_pdata = &_cyttsp4_btn_platform_data,
	.loader_pdata = &_cyttsp4_loader_platform_data,
};
#endif /* !defined(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICETREE_SUPPORT) */

