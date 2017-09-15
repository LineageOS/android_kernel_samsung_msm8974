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

#ifdef CONFIG_WCNSS_IRIS_REGISTER_DUMP
#include <linux/gpio.h>
#endif
#include <linux/init.h>
#include <linux/ioport.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>

#ifdef CONFIG_WCNSS_IRIS_REGISTER_DUMP
#define WLAN_CLK      44
#define WLAN_SET      43
#define WLAN_DATA0    42
#define WLAN_DATA1    41
#define WLAN_DATA2    40
#endif

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
#if 0
	{
		.gpio = 116,  /* HSIC_DATA */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
#endif
};
#endif

#define KS8851_IRQ_GPIO 115

static struct gpiomux_setting nc_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

#define NC_GPIO_CONFIG(gpio_num) { \
		.gpio = gpio_num, \
		.settings ={[GPIOMUX_SUSPENDED] = &nc_cfg,}\
}

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

static struct gpiomux_setting synaptics_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting synaptics_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

/*
static struct gpiomux_setting synaptics_reset_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
        .dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting synaptics_reset_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
        .dir = GPIOMUX_OUT_HIGH,
};
*/

static struct gpiomux_setting synaptics_tch_en_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};


static struct gpiomux_setting synaptics_tch_id_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
#if defined (CONFIG_MACH_RUBENSLTE_OPEN)
static struct gpiomux_setting hall_ic_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
#endif
static struct gpiomux_setting gpio_keys_active = {
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

#ifdef CONFIG_WCNSS_IRIS_REGISTER_DUMP
static struct gpiomux_setting wcnss_5gpio_suspend_cfg = {
        .func = GPIOMUX_FUNC_GPIO,
        .drv  = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting wcnss_5gpio_active_cfg = {
        .func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif

static struct gpiomux_setting gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

#if defined(CONFIG_SEC_RUBENS_PROJECT)

static struct gpiomux_setting sensor_gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

#ifdef CONFIG_MACH_RUBENSLTE_OPEN
static struct gpiomux_setting grip_sensor_gpio_i2c_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting grip_gpio_act_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting grip_gpio_sus_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting grip_irq_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
#endif

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
#endif

#if defined (CONFIG_MACH_RUBENSLTE_OPEN)
static struct msm_gpiomux_config msm_hall_nc[] __initdata = {
	NC_GPIO_CONFIG(37),
};
static struct msm_gpiomux_config msm_hall_ic[] __initdata = {
	{
		.gpio = 37,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hall_ic_cfg,
			[GPIOMUX_SUSPENDED] = &hall_ic_cfg,
		},
	}
};
#endif
static struct msm_gpiomux_config msm_keypad_configs[] __initdata = {
	{
		.gpio = 106,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_keys_active,
			[GPIOMUX_SUSPENDED] = &gpio_keys_active,
		},
	},
	{
		.gpio = 108,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_keys_active,
			[GPIOMUX_SUSPENDED] = &gpio_keys_active,
		},
	},
};

static struct gpiomux_setting lcd_rst_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,	
};

static struct gpiomux_setting lcd_rst_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting lcd_in_gpio_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm_lcd_configs[] __initdata = {
	{
		.gpio = 25,		/* LCD Reset */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_rst_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_rst_sus_cfg,
		},
	},
	{
		.gpio = 53,		/* LCD LDI INT */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_in_gpio_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_in_gpio_cfg,
		},
	},
	{
		.gpio = 20,		/* Backlight SDA */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_in_gpio_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_in_gpio_cfg,
		},
	},
	{
		.gpio = 21,		/* Backlight SCL */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_in_gpio_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_in_gpio_cfg,
		},
	},
	{
		.gpio =56,	/* Backlight Reset */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_rst_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_rst_sus_cfg,
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

static struct gpiomux_setting hw_chk_bit_gpio_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
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
		.gpio      = 32,		/* HW_CHK_BIT 1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &hw_chk_bit_gpio_cfg,
			[GPIOMUX_SUSPENDED] = &hw_chk_bit_gpio_cfg,
		},
	},
	{
		.gpio      = 63,		/* HW_CHK_BIT 0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &hw_chk_bit_gpio_cfg,
			[GPIOMUX_SUSPENDED] = &hw_chk_bit_gpio_cfg,
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
#if defined(CONFIG_SEC_RUBENS_PROJECT)
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
	{
		.gpio      = 2,		/* BLSP1 QUP2 I2C_SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &sensor_gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &sensor_gpio_i2c_config,
		},
	},
	{
		.gpio      = 3,		/* BLSP1 QUP2 I2C_SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &sensor_gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &sensor_gpio_i2c_config,
		},
	},
#endif
};

#if defined(CONFIG_SEC_RUBENS_PROJECT)
static struct msm_gpiomux_config light_prox_config[] __initdata = {
	{
		.gpio      = 67,		/* PROXY IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &prox_irq_config,
			[GPIOMUX_SUSPENDED] = &prox_irq_config,
		},
	},

};
#endif

#ifdef CONFIG_MACH_RUBENSLTE_OPEN
static struct msm_gpiomux_config grip_i2c_config[] __initdata = {
	{
		.gpio      = 0,		/* GRIP I2C_SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &grip_sensor_gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &grip_sensor_gpio_i2c_config,
		},
	},
	{
		.gpio      = 1,		/* GRIP I2C_SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &grip_sensor_gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &grip_sensor_gpio_i2c_config,
		},
	},
	{
		.gpio      = 22,		/* GRIP ADJ_DET_D */
		.settings = {
			[GPIOMUX_ACTIVE] = &grip_gpio_act_config,
			[GPIOMUX_SUSPENDED] = &grip_gpio_sus_config,
		},
	},
};
#endif

#if defined(CONFIG_MACH_RUBENSWIFI_OPEN)
static struct gpiomux_setting gpio_i2c_config_tsp = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config synaptics_wifi_rev0_configs[] __initdata = {

	{
		.gpio = 12,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config_tsp,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_tsp,
		},
	},
	{
		.gpio = 19,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config_tsp,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_tsp,
		},
	},	
};
#endif


static struct msm_gpiomux_config msm_synaptics_configs[] __initdata = {
/*	{
		.gpio = 34,
		.settings = {
			[GPIOMUX_ACTIVE] = &synaptics_reset_act_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_reset_sus_cfg,
		},
	},
*/
	{
		.gpio = 17,
		.settings = {
			[GPIOMUX_ACTIVE] = &synaptics_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_int_sus_cfg,
		},
	},
	{
		.gpio = 31,
		.settings = {
			[GPIOMUX_ACTIVE] = &synaptics_tch_en_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_tch_en_cfg,
		},
	},
	{
		.gpio = 55,
		.settings = {
			[GPIOMUX_ACTIVE] = &synaptics_tch_id_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_tch_id_cfg,
		},
	},
};


static struct gpiomux_setting msm_menu_key_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm_menu_key_configs[] __initdata = {
	{
		.gpio = 49,
		.settings = {
			[GPIOMUX_ACTIVE] = &msm_menu_key_cfg,
			[GPIOMUX_SUSPENDED] = &msm_menu_key_cfg,
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

/*
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
*/

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
#if 0
	{
		.gpio = 16,		/* RESET */
		.settings = {
			[GPIOMUX_ACTIVE] = &goodix_reset_act_cfg,
			[GPIOMUX_SUSPENDED] = &goodix_reset_sus_cfg,
		},
	},
#endif
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

#if defined(CONFIG_SEC_RUBENS_PROJECT)
static struct gpiomux_setting nfc_irq_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting nfc_enable_config = {
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting nfc_firmware_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config msm_nfc_configs[] __initdata = {
	{					/*  NFC_IRQ */
		.gpio      = 62,
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_irq_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_irq_cfg,
		},
	},
	{
		.gpio = 23, /* NFC EN */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_enable_config,
			[GPIOMUX_SUSPENDED] = &nfc_enable_config,
		},
	},
	{
		.gpio		= 24,		/* NFC FIRMWARE */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_firmware_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_firmware_cfg,
		},
	},
};
#endif

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

#if defined(CONFIG_SEC_RUBENS_PROJECT)
static struct msm_gpiomux_config msm_nativesensors_configs[] __initdata = {

#ifdef CONFIG_MACH_RUBENSLTE_OPEN
	{
		.gpio      = 66,		/* GRIP IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &grip_irq_config,
			[GPIOMUX_SUSPENDED] = &grip_irq_config,
		},
	},
#endif
	{
		.gpio	   = 54,		/* ACCEL IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &accel_irq_config,
			[GPIOMUX_SUSPENDED] = &accel_irq_config,
		},
	},
};
#endif

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

#ifdef CONFIG_WCNSS_IRIS_REGISTER_DUMP
static struct msm_gpiomux_config wcnss_5gpio_interface[] = {
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5gpio_suspend_cfg,
		},
	},
	{
		.gpio = 41,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5gpio_suspend_cfg,
		},
	},
	{
		.gpio = 42,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5gpio_suspend_cfg,
		},
	},
	{
		.gpio = 43,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5gpio_suspend_cfg,
		},
	},
	{
		.gpio = 44,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5gpio_suspend_cfg,
		},
	},
};
#endif

static struct gpiomux_setting gpio_suspend_config[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,  /* IN-NP */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,  /* O-LOW */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},
};

/* IN-NP-L */
#define IN_NP_2MA_CFG(gpio_num) { \
        .gpio = gpio_num, \
        .settings = {\
                [GPIOMUX_ACTIVE] = &gpio_suspend_config[0], \
                [GPIOMUX_SUSPENDED] = &gpio_suspend_config[0], \
        }\
}

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
	},
	{
		.func = GPIOMUX_FUNC_GPIO, /*active 1*/  /* 5 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_HIGH,
	},
	{
		.func = GPIOMUX_FUNC_GPIO, /*active 1*/  /* 6 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_GPIO, /*active 1*/  /* 7 */
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
		.gpio = 27, /* CAM_MCLK1 */
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
			[GPIOMUX_ACTIVE]    = &cam_settings[6],
			[GPIOMUX_SUSPENDED] = &cam_settings[6],
		},
	},
	{
		.gpio = 110, /* CAM1_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[3],
		},
	},
#ifdef CONFIG_MACH_RUBENSWIFI_OPEN
	{
		.gpio = 116, /* AF */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[3],
		},
	},
#else
	{
		.gpio = 120, /* AF */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[3],
		},
	},
#endif
	{
		.gpio = 112, /* CAM_ANALOG_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[3],
		},
	},
	{
		.gpio = 16, /* VTCAM_1.8V */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[3],
		},
	},
	{
		.gpio = 28, /* CAM2_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[3],
		},
	},
	{
		.gpio = 35, /* CAM2_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[6],
			[GPIOMUX_SUSPENDED] = &cam_settings[6],
		},
	},
	{
		.gpio = 64, /* CAM_FLASH_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[3],
		},
	},
	{
		.gpio = 65, /* CAM_TORCH_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[3],
		},
	},
};

static struct msm_gpiomux_config msm_sensor_configs_skuf_plus[] __initdata = {
	{
		.gpio = 22, /* CAM1_VDD */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 34, /* CAM1 VCM_PWDN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
};


#if defined (CONFIG_SAMSUNG_JACK)
static struct gpiomux_setting earjack_gpio_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO, /*active 1*/ /* 0 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting earjack_gpio_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO, /*suspend*/ /* 1 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm_earjack_gpio_configs[] __initdata = {
	{
		.gpio = 111, /* EAR_SEND_END */
		.settings = {
			[GPIOMUX_ACTIVE]    = &earjack_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &earjack_gpio_suspend_cfg,
		},
	},
	{
		.gpio = 69,
		.settings = {
			[GPIOMUX_ACTIVE]    = &earjack_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &earjack_gpio_suspend_cfg,
		},
	},
};
#endif
#ifdef CONFIG_SND_SOC_MAX98504
static struct gpiomux_setting gpio_i2c_codec_active_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.dir = GPIOMUX_IN,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting gpio_i2c_codec_suspend_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.dir = GPIOMUX_IN,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting  pri_mi2s_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting  pri_mi2s_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm8226_blsp_codec_configs[] __initdata = {
	{					/*	MAX_SDA   */
		.gpio	   = 22,		/* BLSP1 QUP3 I2C_DAT */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_codec_active_config,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_codec_suspend_config,
		},
	},
	{					/*	MAX_SCL   */
		.gpio	   = 23,		/* BLSP1 QUP3 I2C_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_codec_active_config,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_codec_suspend_config,
		},
	},
};
static struct gpiomux_setting  gpio_amp_int_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.dir = GPIOMUX_IN,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm8226_amp_int_configs[] __initdata = {
	{
		.gpio	   = 24,	/*	AMP_INT_N   */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_amp_int_config,
			[GPIOMUX_SUSPENDED] = &gpio_amp_int_config,
		},
	},
};

static struct msm_gpiomux_config msm8226_tertiary_mi2s_configs[] __initdata = {
	{
		.gpio	= 49,		/* qua mi2s sck */
		.settings = {
			[GPIOMUX_SUSPENDED] = &pri_mi2s_sus_cfg,
			[GPIOMUX_ACTIVE] = &pri_mi2s_act_cfg,
		},
	},
	{
		.gpio	= 50,
		.settings = {
			[GPIOMUX_SUSPENDED] = &pri_mi2s_sus_cfg,
			[GPIOMUX_ACTIVE] = &pri_mi2s_act_cfg,
		},
	},
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_SUSPENDED] = &pri_mi2s_sus_cfg,
			[GPIOMUX_ACTIVE] = &pri_mi2s_act_cfg,
		},
	},
	{
		.gpio = 52,
		.settings = {
			[GPIOMUX_SUSPENDED] = &pri_mi2s_sus_cfg,
			[GPIOMUX_ACTIVE] = &pri_mi2s_act_cfg,
		},
	},

};
#endif /* CONFIG_SND_SOC_MAX98504 */


static struct gpiomux_setting usb_otg_sw_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config usb_otg_sw_configs[] __initdata = {
	{
		.gpio = 67,
		.settings = {
			[GPIOMUX_SUSPENDED] = &usb_otg_sw_cfg,
		},
	},
};
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
			[GPIOMUX_SUSPENDED] = &wcdcodec_reset_suspend_cfg,
			[GPIOMUX_ACTIVE] = &wcdcodec_reset_active_cfg,
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

extern int system_rev;

extern int poweroff_charging;

#if defined (CONFIG_MACH_RUBENSLTE_OPEN)
static struct msm_gpiomux_config rubens_nc_gpio_cfgs[] __initdata = {
	NC_GPIO_CONFIG(4),
	NC_GPIO_CONFIG(22),
	NC_GPIO_CONFIG(33),
	NC_GPIO_CONFIG(45),
	NC_GPIO_CONFIG(60),
	NC_GPIO_CONFIG(75),
	NC_GPIO_CONFIG(76),
	NC_GPIO_CONFIG(77),
	NC_GPIO_CONFIG(78),
	NC_GPIO_CONFIG(79),
	NC_GPIO_CONFIG(83),
	NC_GPIO_CONFIG(85),
	NC_GPIO_CONFIG(86),
	NC_GPIO_CONFIG(87),
	NC_GPIO_CONFIG(91),
	NC_GPIO_CONFIG(92),
	NC_GPIO_CONFIG(93),
	NC_GPIO_CONFIG(94),
	NC_GPIO_CONFIG(107),
	NC_GPIO_CONFIG(115),
	NC_GPIO_CONFIG(117),
	NC_GPIO_CONFIG(118),
};
#endif

#if defined (CONFIG_MACH_RUBENSWIFI_OPEN)
static struct msm_gpiomux_config rubens_nc_gpio_cfgs[] __initdata = {
	NC_GPIO_CONFIG(0),
	NC_GPIO_CONFIG(1),
	NC_GPIO_CONFIG(4),
	NC_GPIO_CONFIG(22),
	NC_GPIO_CONFIG(37),
	NC_GPIO_CONFIG(57),
	NC_GPIO_CONFIG(58),
	NC_GPIO_CONFIG(59),
	NC_GPIO_CONFIG(60),
	NC_GPIO_CONFIG(66),
	NC_GPIO_CONFIG(107),
};
#endif
/*Battery Charger/Fuel-gauge GPIOs*/
static struct msm_gpiomux_config msm_chg_fg_configs[] = {
	IN_NP_2MA_CFG(10),	/* FUEL_SDA */
	IN_NP_2MA_CFG(11),	/* FUEL_SCL */
	IN_NP_2MA_CFG(14),	/* IF_PMIC_SDA */
	IN_NP_2MA_CFG(109),	/* FUEL_ALERT_N */
};

void __init msm8226_init_gpiomux(void)
{
	int rc;

	rc = msm_gpiomux_init_dt();
	if (rc) {
		pr_err("%s failed %d\n", __func__, rc);
		return;
	}

	/* Battery Charger/Fuel-gauge GPIOs */
	msm_gpiomux_install(msm_chg_fg_configs, ARRAY_SIZE(msm_chg_fg_configs));

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
#if defined(CONFIG_MACH_RUBENSWIFI_OPEN)
	if(system_rev == 0)
		msm_gpiomux_install(synaptics_wifi_rev0_configs,
			ARRAY_SIZE(synaptics_wifi_rev0_configs));	
#endif

	msm_gpiomux_install(wcnss_5wire_interface,
				ARRAY_SIZE(wcnss_5wire_interface));

	msm_gpiomux_install(&sd_card_det, 1);
	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_goodix_configs,
				ARRAY_SIZE(msm_skuf_goodix_configs));

	msm_gpiomux_install(msm_synaptics_configs,
				ARRAY_SIZE(msm_synaptics_configs));
	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_nfc_configs,
				ARRAY_SIZE(msm_skuf_nfc_configs));

	msm_gpiomux_install_nowrite(msm_lcd_configs,
			ARRAY_SIZE(msm_lcd_configs));

	msm_gpiomux_install(msm_sensor_configs, ARRAY_SIZE(msm_sensor_configs));

	if (of_board_is_skuf())
		msm_gpiomux_install(msm_sensor_configs_skuf_plus,
			ARRAY_SIZE(msm_sensor_configs_skuf_plus));

	if (of_board_is_cdp() || of_board_is_mtp() || of_board_is_xpm())
		msm_gpiomux_install(usb_otg_sw_configs,
					ARRAY_SIZE(usb_otg_sw_configs));
#if defined(CONFIG_SEC_RUBENS_PROJECT)
	msm_gpiomux_install(light_prox_config, ARRAY_SIZE(light_prox_config));
#ifdef CONFIG_MACH_RUBENSLTE_OPEN
	msm_gpiomux_install(grip_i2c_config, ARRAY_SIZE(grip_i2c_config));
#endif
	msm_gpiomux_install(msm_nativesensors_configs,ARRAY_SIZE(msm_nativesensors_configs));
	msm_gpiomux_install(msm_nfc_configs, ARRAY_SIZE(msm_nfc_configs));
#endif
#if defined (CONFIG_MACH_RUBENSLTE_OPEN)
	if(system_rev > 2)
		msm_gpiomux_install(msm_hall_nc,
                        ARRAY_SIZE(msm_hall_nc));
	else
		msm_gpiomux_install(msm_hall_ic,
                        ARRAY_SIZE(msm_hall_ic));
	msm_gpiomux_install(rubens_nc_gpio_cfgs, ARRAY_SIZE(rubens_nc_gpio_cfgs));
#endif

#if defined (CONFIG_MACH_RUBENSWIFI_OPEN)
	msm_gpiomux_install(rubens_nc_gpio_cfgs, ARRAY_SIZE(rubens_nc_gpio_cfgs));
#endif
	msm_gpiomux_sdc3_install();

#if defined(CONFIG_SAMSUNG_JACK)
	msm_gpiomux_install(msm_earjack_gpio_configs, ARRAY_SIZE(msm_earjack_gpio_configs));
#endif
	if(!poweroff_charging)
	msm_gpiomux_install(wcdcodec_reset_cfg, ARRAY_SIZE(wcdcodec_reset_cfg));

#ifdef CONFIG_SND_SOC_MAX98504
	{
		msm_gpiomux_install(msm8226_tertiary_mi2s_configs,ARRAY_SIZE(msm8226_tertiary_mi2s_configs));
		msm_gpiomux_install(msm8226_blsp_codec_configs,ARRAY_SIZE(msm8226_blsp_codec_configs));
		msm_gpiomux_install(msm8226_amp_int_configs,ARRAY_SIZE(msm8226_amp_int_configs));
	}
#endif
	msm_gpiomux_install(msm_menu_key_configs,
				ARRAY_SIZE(msm_menu_key_configs));
}

#ifdef CONFIG_WCNSS_IRIS_REGISTER_DUMP
static void wcnss_switch_to_gpio(void)
{
	/* Switch MUX to GPIO */
	msm_gpiomux_install(wcnss_5gpio_interface,
			ARRAY_SIZE(wcnss_5gpio_interface));

	/* Ensure GPIO config */
	gpio_direction_input(WLAN_DATA2);
	gpio_direction_input(WLAN_DATA1);
	gpio_direction_input(WLAN_DATA0);
	gpio_direction_output(WLAN_SET, 0);
	gpio_direction_output(WLAN_CLK, 0);
}

static void wcnss_switch_to_5wire(void)
{
	msm_gpiomux_install(wcnss_5wire_interface,
			ARRAY_SIZE(wcnss_5wire_interface));
}

u32 wcnss_rf_read_reg(u32 rf_reg_addr)
{
	int count = 0;
	u32 rf_cmd_and_addr = 0;
	u32 rf_data_received = 0;
	u32 rf_bit = 0;

	wcnss_switch_to_gpio();

	/* Reset the signal if it is already being used. */
	gpio_set_value(WLAN_SET, 0);
	gpio_set_value(WLAN_CLK, 0);

	/* We start with cmd_set high WLAN_SET = 1. */
	gpio_set_value(WLAN_SET, 1);

	gpio_direction_output(WLAN_DATA0, 1);
	gpio_direction_output(WLAN_DATA1, 1);
	gpio_direction_output(WLAN_DATA2, 1);

	gpio_set_value(WLAN_DATA0, 0);
	gpio_set_value(WLAN_DATA1, 0);
	gpio_set_value(WLAN_DATA2, 0);

	/* Prepare command and RF register address that need to sent out.
	 * Make sure that we send only 14 bits from LSB.
	 */
	rf_cmd_and_addr  = (((WLAN_RF_READ_REG_CMD) |
		(rf_reg_addr << WLAN_RF_REG_ADDR_START_OFFSET)) &
		WLAN_RF_READ_CMD_MASK);

	for (count = 0; count < 5; count++) {
		gpio_set_value(WLAN_CLK, 0);

		rf_bit = (rf_cmd_and_addr & 0x1);
		gpio_set_value(WLAN_DATA0, rf_bit ? 1 : 0);
		rf_cmd_and_addr = (rf_cmd_and_addr >> 1);

		rf_bit = (rf_cmd_and_addr & 0x1);
		gpio_set_value(WLAN_DATA1, rf_bit ? 1 : 0);
		rf_cmd_and_addr = (rf_cmd_and_addr >> 1);

		rf_bit = (rf_cmd_and_addr & 0x1);
		gpio_set_value(WLAN_DATA2, rf_bit ? 1 : 0);
		rf_cmd_and_addr = (rf_cmd_and_addr >> 1);

		/* Send the data out WLAN_CLK = 1 */
		gpio_set_value(WLAN_CLK, 1);
	}

	/* Pull down the clock signal */
	gpio_set_value(WLAN_CLK, 0);

	/* Configure data pins to input IO pins */
	gpio_direction_input(WLAN_DATA0);
	gpio_direction_input(WLAN_DATA1);
	gpio_direction_input(WLAN_DATA2);

	for (count = 0; count < 2; count++) {
		gpio_set_value(WLAN_CLK, 1);
		gpio_set_value(WLAN_CLK, 0);
	}

	rf_bit = 0;
	for (count = 0; count < 6; count++) {
		gpio_set_value(WLAN_CLK, 1);
		gpio_set_value(WLAN_CLK, 0);

		rf_bit = gpio_get_value(WLAN_DATA0);
		rf_data_received |= (rf_bit << (count * 3 + 0));

		if (count != 5) {
			rf_bit = gpio_get_value(WLAN_DATA1);
			rf_data_received |= (rf_bit << (count * 3 + 1));

			rf_bit = gpio_get_value(WLAN_DATA2);
			rf_data_received |= (rf_bit << (count * 3 + 2));
		}
	}

	gpio_set_value(WLAN_SET, 0);
	wcnss_switch_to_5wire();

	return rf_data_received;
}
#endif
