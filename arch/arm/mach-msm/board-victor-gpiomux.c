/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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
/****************************** Revision History ******************************************
 *CH# Product		author		Description				Date
 *-----------------------------------------------------------------------------------------
 *01
 *	variants
 ******************************************************************************************
 */

#include <linux/init.h>
#include <linux/ioport.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>

#ifdef CONFIG_USB_EHCI_MSM_HSIC
static struct gpiomux_setting hsic_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting hsic_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm_hsic_configs[] = {
	{
		.gpio = 115,  /* HSIC_STROBE */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
	{
		.gpio = 116,  /* HSIC_DATA */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
};
#endif

#define KS8851_IRQ_GPIO 115

static struct gpiomux_setting gpio_suspend_config[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,  /* IN-NP */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,  /* OUT-NP */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,  /* IN-PD */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,  /* OUT-PD */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_OUT_LOW,
	},
};

#define GPIOMUX_SET_NC(n) \
	{ \
		.gpio	   = n, \
		.settings = { \
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2], \
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2], \
		}, \
	}

static struct msm_gpiomux_config gpio_nc_configs[] __initdata = {
	GPIOMUX_SET_NC(0),
	GPIOMUX_SET_NC(1),
	GPIOMUX_SET_NC(2),
	GPIOMUX_SET_NC(3),
	GPIOMUX_SET_NC(22),
	GPIOMUX_SET_NC(23),
	GPIOMUX_SET_NC(27),
	GPIOMUX_SET_NC(33),
#if !defined(CONFIG_MACH_VICTORLTE_CTC)
	GPIOMUX_SET_NC(34),
#endif
	GPIOMUX_SET_NC(36),
	GPIOMUX_SET_NC(45),
	GPIOMUX_SET_NC(46),
	GPIOMUX_SET_NC(49),
	GPIOMUX_SET_NC(50),
	GPIOMUX_SET_NC(56),
	GPIOMUX_SET_NC(60),
	GPIOMUX_SET_NC(63),
	GPIOMUX_SET_NC(64),
	GPIOMUX_SET_NC(66),
	GPIOMUX_SET_NC(77),
	GPIOMUX_SET_NC(81),
	GPIOMUX_SET_NC(82),
	GPIOMUX_SET_NC(95),
	GPIOMUX_SET_NC(96),
	GPIOMUX_SET_NC(107),
	GPIOMUX_SET_NC(110),
	GPIOMUX_SET_NC(113),
	GPIOMUX_SET_NC(114),
	GPIOMUX_SET_NC(119),
	GPIOMUX_SET_NC(120),
};

static struct msm_gpiomux_config hw_rev_configs[] __initdata = {
	{
		.gpio = 15,	/* HW_REV(0) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 14,	/* HW_REV(1) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 13,	/* HW_REV(2) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 16,	/* HW_REV(3) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
};

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct gpiomux_setting gpio_eth_config = {
	.pull = GPIOMUX_PULL_UP,
	.drv = GPIOMUX_DRV_2MA,
	.func = GPIOMUX_FUNC_GPIO,
};

static struct msm_gpiomux_config msm_eth_configs[] = {
	{
		.gpio = KS8851_IRQ_GPIO,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_eth_config,
		}
	},
};
#endif

static struct gpiomux_setting muic_active_config[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	}
};

static struct gpiomux_setting muic_suspend_config[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	}
};


static struct msm_gpiomux_config muic_gpio_config[] __initdata = {
	{
		.gpio = 4,
		.settings = {
			[GPIOMUX_ACTIVE]    = &muic_active_config[0],
			[GPIOMUX_SUSPENDED] = &muic_suspend_config[0],
		},
	},
#if !defined(CONFIG_MACH_VICTORLTE_CTC)
	{
		.gpio = 5,
		.settings = {
			[GPIOMUX_ACTIVE]    = &muic_active_config[0],
			[GPIOMUX_SUSPENDED] = &muic_suspend_config[0],
		},
	},
#endif
};


static struct gpiomux_setting synaptics_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting synaptics_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting synaptics_reset_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting synaptics_reset_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting chg_pm8226_gpio_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_keys_active = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting gpio_keys_suspend = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};


static struct gpiomux_setting wcnss_5wire_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting wcnss_5wire_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm_keypad_configs[] __initdata = {
	{
		.gpio = 106,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_keys_active,
			[GPIOMUX_SUSPENDED] = &gpio_keys_suspend,
		},
	},
	{
		.gpio = 108,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_keys_active,
			[GPIOMUX_SUSPENDED] = &gpio_keys_suspend,
		},
	},
};


static struct gpiomux_setting lcd_det_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting lcd_det_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting lcd_rst_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting lcd_rst_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting lcd_en_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting lcd_en_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting lcd_flm_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting lcd_flm_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};



static struct msm_gpiomux_config msm_lcd_configs[] __initdata = {
	{
		.gpio = 25,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_rst_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_rst_sus_cfg,
		},
	},
	{
		.gpio = 28,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_en_sus_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_lcd_det_configs[] __initdata = {
	{
		.gpio = 33,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_det_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_det_sus_cfg,
		},
	},
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_flm_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_flm_sus_cfg,
		},
	},
};

static struct gpiomux_setting sensor_gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting accel_irq_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting prox_irq_config = {
    .func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting proxy_en_config = {
    .func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config msm_nativesensors_configs[] __initdata = {
	{
		.gpio      = 34,		/* PROXY_EN */
		.settings = {
			[GPIOMUX_ACTIVE] = &proxy_en_config,
			[GPIOMUX_SUSPENDED] = &proxy_en_config,
		},
	},
	{
		.gpio      = 65,		/* PROXY IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &prox_irq_config,
			[GPIOMUX_SUSPENDED] = &prox_irq_config,
		},
	},
	{
		.gpio	   = 67,		/* ACCEL IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &accel_irq_config,
			[GPIOMUX_SUSPENDED] = &accel_irq_config,
		},
	},
};

static struct gpiomux_setting gpio_uart_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE, /*should be pulled None for UART */
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting rx_gpio_uart_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN, /*LCIA Test failure*/
	.dir = GPIOMUX_OUT_HIGH,
};

static struct msm_gpiomux_config msm_blsp_configs[] __initdata = {
	{
		.gpio      = 8,		/* UART TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 9,		/* UART RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &rx_gpio_uart_config,
		},
	},
	{
		.gpio      = 18,		/* BLSP1 QUP5 I2C_SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 19,		/* BLSP1 QUP5 I2C_SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 6,		/* BLSP1 QUP2 I2C_SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &sensor_gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &sensor_gpio_i2c_config,
		},
	},
	{
		.gpio      = 7,		/* BLSP1 QUP2 I2C_SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &sensor_gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &sensor_gpio_i2c_config,
		},
	},
#if defined(CONFIG_NFC_PN547) || defined(CONFIG_BCM2079X_NFC_I2C)
	{					/*	NFC   */
		.gpio	   = 10,		/* BLSP1 QUP3 I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{					/*	NFC   */
		.gpio	   = 11,		/* BLSP1 QUP3 I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
#endif
};

static struct msm_gpiomux_config msm_synaptics_configs[] __initdata = {
	{
		.gpio = 16,
		.settings = {
			[GPIOMUX_ACTIVE] = &synaptics_reset_act_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_reset_sus_cfg,
		},
	},
	{
		.gpio = 17,
		.settings = {
			[GPIOMUX_ACTIVE] = &synaptics_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_int_sus_cfg,
		},
	},
};

static struct msm_gpiomux_config chg_pm8226_configs[] __initdata = {
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_ACTIVE] = &chg_pm8226_gpio_cfg,
			[GPIOMUX_SUSPENDED] = &chg_pm8226_gpio_cfg,
		},
	},
};

static struct gpiomux_setting gpio_nc_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting goodix_ldo_en_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting goodix_ldo_en_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting goodix_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting goodix_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting goodix_reset_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting goodix_reset_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm_skuf_blsp_configs[] __initdata = {
	{
		.gpio      = 2,		/* NC */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_nc_cfg,
		},
	},
	{
		.gpio      = 3,		/* NC */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_nc_cfg,
		},
	},
	{
		.gpio      = 4,		/* NC */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_nc_cfg,
		},
	},
	{
		.gpio      = 14,	/* NC */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_nc_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_skuf_goodix_configs[] __initdata = {
	{
		.gpio = 15,		/* LDO EN */
		.settings = {
			[GPIOMUX_ACTIVE] = &goodix_ldo_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &goodix_ldo_en_sus_cfg,
		},
	},
	{
		.gpio = 16,		/* RESET */
		.settings = {
			[GPIOMUX_ACTIVE] = &goodix_reset_act_cfg,
			[GPIOMUX_SUSPENDED] = &goodix_reset_sus_cfg,
		},
	},
	{
		.gpio = 17,		/* INT */
		.settings = {
			[GPIOMUX_ACTIVE] = &goodix_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &goodix_int_sus_cfg,
		},
	},
	{
		.gpio      = 18,		/* BLSP1 QUP5 I2C_SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 19,		/* BLSP1 QUP5 I2C_SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
};

static struct gpiomux_setting nfc_wake_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting nfc_wake_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config msm_skuf_nfc_configs[] __initdata = {
	{					/*  NFC   WAKE */
		.gpio      = 5,
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_wake_act_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_wake_sus_cfg,
		},
	},
};

static struct gpiomux_setting nfc_wake_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm_nfc_configs[] __initdata = {
	{					/*  NFC_IRQ */
		.gpio      = 21,
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_wake_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_wake_cfg,
		},
	},
};

static struct gpiomux_setting sd_card_det_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting sd_card_det_sleep_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config sd_card_det __initdata = {
	.gpio = 38,
	.settings = {
		[GPIOMUX_ACTIVE]    = &sd_card_det_active_config,
		[GPIOMUX_SUSPENDED] = &sd_card_det_sleep_config,
	},
};

static struct msm_gpiomux_config wcnss_5wire_interface[] = {
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 41,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 42,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 43,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 44,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
};

static struct gpiomux_setting cam_settings[] = {
	{
		.func = GPIOMUX_FUNC_1, /*active 1*/ /* 0 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_1, /*suspend*/ /* 1 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},

	{
		.func = GPIOMUX_FUNC_1, /*i2c suspend*/ /* 2 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 0*/ /* 3 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend 0*/ /* 4 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_OUT_LOW,
	},
	{
		.func = GPIOMUX_FUNC_1, /*active 1*/ /* 0 */
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_1, /*suspend*/ /* 1 */
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_GPIO, /*active 1*/ /* 0 */ //7 Haarika
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},
	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend*/ /* 1 */ //8
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},
	{
		.func = GPIOMUX_FUNC_1, /*active 1*/ /* 9 */
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_1, /*suspend*/ /* 10 */
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_1, /*active 1*/ /* 11 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_1, /*suspend*/ /* 12 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
};

static struct msm_gpiomux_config msm_sensor_configs[] __initdata = {
	{
		.gpio = 26, /* CAM_MCLK0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
	{
		.gpio = 29, /* CCI_I2C_SDA0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 30, /* CCI_I2C_SCL0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 36, /* CAM1_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = 37, /* CAM1_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 35, /* CAM2_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = 112, /* CAM_ANALOG_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
};

static struct msm_gpiomux_config msm_sensor_configs_skuf_plus[] __initdata = {
	{
		.gpio = 22, /* CAM1_VDD */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
#if !defined(CONFIG_MACH_VICTORLTE_CTC)
	{
		.gpio = 34, /* CAM1 VCM_PWDN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
#endif
};

static struct gpiomux_setting codec_int_gpio_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO, /*active 1*/ /* 0 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting codec_int_gpio_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO, /*suspend*/ /* 1 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm_codec_int_gpio_configs[] __initdata = {
	{
		.gpio = 68, /* CODEC_INT */
		.settings = {
			[GPIOMUX_ACTIVE]    = &codec_int_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &codec_int_gpio_suspend_cfg,
		},
	},
};

#if defined (CONFIG_SAMSUNG_JACK)
static struct gpiomux_setting ear_switch_gpio_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO, /*active 1*/ /* 0 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting ear_switch_gpio_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO, /*suspend*/ /* 1 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting ear_det_gpio_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO, /*active 1*/ /* 0 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting ear_det_gpio_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO, /*suspend*/ /* 1 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm_earjack_gpio_configs[] __initdata = {
	{
		.gpio = 35, /* EAR_SWITCH */
		.settings = {
			[GPIOMUX_ACTIVE]    = &ear_switch_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &ear_switch_gpio_suspend_cfg,
		},
	},
	{
		.gpio = 69, /* EAR_3.5_DET */
		.settings = {
			[GPIOMUX_ACTIVE]    = &ear_det_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &ear_det_gpio_suspend_cfg,
		},
	},
};
#endif

static struct gpiomux_setting wcdcodec_reset_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.dir = GPIOMUX_OUT_HIGH,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting wcdcodec_reset_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.dir = GPIOMUX_OUT_HIGH,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config wcdcodec_reset_cfg[] __initdata = {
	{
		.gpio = 72,
		.settings = {
			[GPIOMUX_ACTIVE] = &wcdcodec_reset_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcdcodec_reset_suspend_cfg,
		},
	},
};

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct gpiomux_setting sdc3_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdc3_cmd_data_0_3_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdc3_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sdc3_data_1_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm8226_sdc3_configs[] __initdata = {
	{
		/* DAT3 */
		.gpio      = 39,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* DAT2 */
		.gpio      = 40,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* DAT1 */
		.gpio      = 41,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_data_1_suspend_cfg,
		},
	},
	{
		/* DAT0 */
		.gpio      = 42,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* CMD */
		.gpio      = 43,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* CLK */
		.gpio      = 44,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
};

static void msm_gpiomux_sdc3_install(void)
{
	msm_gpiomux_install(msm8226_sdc3_configs,
			    ARRAY_SIZE(msm8226_sdc3_configs));
}
#else
static void msm_gpiomux_sdc3_install(void) {}
#endif /* CONFIG_MMC_MSM_SDC3_SUPPORT */

extern int poweroff_charging;

void __init msm8226_init_gpiomux(void)
{
	int rc;

	rc = msm_gpiomux_init_dt();
	if (rc) {
		pr_err("%s failed %d\n", __func__, rc);
		return;
	}
#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	msm_gpiomux_install(msm_eth_configs, ARRAY_SIZE(msm_eth_configs));
#endif
	msm_gpiomux_install(msm_keypad_configs,
			ARRAY_SIZE(msm_keypad_configs));

	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_blsp_configs,
			ARRAY_SIZE(msm_skuf_blsp_configs));
	else
		msm_gpiomux_install(msm_blsp_configs,
			ARRAY_SIZE(msm_blsp_configs));

	msm_gpiomux_install(wcnss_5wire_interface,
				ARRAY_SIZE(wcnss_5wire_interface));

	msm_gpiomux_install(&sd_card_det, 1);
	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_goodix_configs,
				ARRAY_SIZE(msm_skuf_goodix_configs));
	else
		msm_gpiomux_install(msm_synaptics_configs,
				ARRAY_SIZE(msm_synaptics_configs));

	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_nfc_configs,
				ARRAY_SIZE(msm_skuf_nfc_configs));
	else
		msm_gpiomux_install(msm_nfc_configs,
				ARRAY_SIZE(msm_nfc_configs));

	msm_gpiomux_install(msm_sensor_configs, ARRAY_SIZE(msm_sensor_configs));

	if (of_board_is_skuf())
		msm_gpiomux_install(msm_sensor_configs_skuf_plus,
			ARRAY_SIZE(msm_sensor_configs_skuf_plus));

	msm_gpiomux_sdc3_install();
	msm_gpiomux_install(msm_nativesensors_configs,ARRAY_SIZE(msm_nativesensors_configs));

	msm_gpiomux_install(hw_rev_configs, ARRAY_SIZE(hw_rev_configs));
	msm_gpiomux_install(gpio_nc_configs, ARRAY_SIZE(gpio_nc_configs));
	msm_gpiomux_install(muic_gpio_config, ARRAY_SIZE(muic_gpio_config));

	msm_gpiomux_install(msm_codec_int_gpio_configs, ARRAY_SIZE(msm_codec_int_gpio_configs));
#if defined(CONFIG_SAMSUNG_JACK)
	msm_gpiomux_install(msm_earjack_gpio_configs, ARRAY_SIZE(msm_earjack_gpio_configs));
#endif

	msm_gpiomux_install(chg_pm8226_configs, ARRAY_SIZE(chg_pm8226_configs));

	if(!poweroff_charging)
	msm_gpiomux_install(wcdcodec_reset_cfg, ARRAY_SIZE(wcdcodec_reset_cfg));

        msm_gpiomux_install(msm_lcd_det_configs, ARRAY_SIZE(msm_lcd_det_configs));
	msm_gpiomux_install_nowrite(msm_lcd_configs,ARRAY_SIZE(msm_lcd_configs));
}
