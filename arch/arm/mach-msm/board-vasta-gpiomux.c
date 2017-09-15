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
		.gpio = 115,               /* HSIC_STROBE */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
	{
		.gpio = 116,               /* HSIC_DATA */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
};
#endif

#define KS8851_IRQ_GPIO 115

/*Adding the GPIOMUX settings for the Not Connected GPIOs(NC)*/

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
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting synaptics_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gpio_i2c_tkey_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};
static struct gpiomux_setting gpio_i2c_tkey_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
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
#if !defined (CONFIG_SEC_MILLET_PROJECT) && !defined(CONFIG_SEC_MATISSE_PROJECT)
static struct gpiomux_setting gpio_spi_act_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_spi_cs_act_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};
static struct gpiomux_setting gpio_spi_susp_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif

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

#if defined(CONFIG_SEC_VASTA_PROJECT)
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
#endif

#if defined(CONFIG_SEC_VASTA_PROJECT)
static struct gpiomux_setting nfc_hrm_gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

#endif

static struct gpiomux_setting nfc_ven_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting nfc_irq_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting nfc_firmware_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

#if defined (CONFIG_SEC_VASTA_PROJECT)
static struct gpiomux_setting hrm_irq_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
#endif
#if defined (CONFIG_MACH_BERLUTI3G_EUR)
static struct gpiomux_setting gpio_uart_active_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gpio_uart_sleep_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_uart_rx_sleep_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gpio_i2c_tsp_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting gpio_i2c_tsp_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

static struct msm_gpiomux_config msm_keypad_configs[] __initdata = {
	{
		.gpio = 106,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_keys_active,
			[GPIOMUX_SUSPENDED] = &gpio_keys_suspend,
		},
	},
	{
		.gpio = 107,
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

static struct gpiomux_setting lcd_pwr_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting lcd_pwr_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};
/*
static struct gpiomux_setting lcd_esd_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
*/
static struct msm_gpiomux_config msm_lcd_configs[] __initdata = {
	{
		.gpio = 25,		/* LCD Reset */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_rst_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_rst_sus_cfg,
		},
	},
	{
		.gpio = 12, 	/* LCD_BLIC_ON */
		.settings = {
			[GPIOMUX_ACTIVE]	= &lcd_pwr_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_pwr_sus_cfg,
		},
	},
	{
		.gpio = 56, 	/* PANEL_ENP */
		.settings = {
			[GPIOMUX_ACTIVE]	= &lcd_pwr_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_pwr_sus_cfg,
		},
	},
	{
		.gpio = 60,		/* PANEL_ENN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_pwr_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_pwr_sus_cfg,
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
		.gpio      = 22,		/* BLSP1 QUP1 SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_tkey_active_config,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_tkey_suspend_config,
		},
	},
	{
		.gpio      = 23,		/* BLSP1 QUP1 SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_tkey_active_config,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_tkey_suspend_config,
		},
	},
	{
		.gpio      = 2,		/* BLSP1 QUP1 SPI_CS1 */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_cs_act_config,
			[GPIOMUX_SUSPENDED] = &gpio_spi_susp_config,
		},
	},
	{
		.gpio      = 3,		/* BLSP1 QUP1 SPI_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_act_config,
			[GPIOMUX_SUSPENDED] = &gpio_spi_susp_config,
		},
	},

#if !defined(CONFIG_SEC_MEGA23GLTE_COMMON)
	{
		.gpio      = 14,	/* BLSP1 QUP4 I2C_SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 15,	/* BLSP1 QUP4 I2C_SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
#endif
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

#if defined(CONFIG_SEC_VASTA_PROJECT)
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
		.gpio      = 10,		/* BLSP3 QUP1 I2C_SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_hrm_gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &nfc_hrm_gpio_i2c_config,
		},
	},
	{
		.gpio      = 11,		/* BLSP3 QUP1 I2C_SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_hrm_gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &nfc_hrm_gpio_i2c_config,
		},
	},
#endif
};
#if defined(CONFIG_SEC_VASTA_PROJECT)
static struct msm_gpiomux_config msm_hrm_configs[] __initdata = {
	{
		.gpio      = 63,		/* HRM IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &hrm_irq_cfg,
			[GPIOMUX_SUSPENDED] = &hrm_irq_cfg,
		},
	},
};
#endif

static struct msm_gpiomux_config msm_nfc_configs[] __initdata = {
	{
		.gpio      = 21,		/* NFC IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_irq_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_irq_cfg,
		},
	},
	{
		.gpio		= 62,		/* NFC FIRMWARE */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_firmware_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_firmware_cfg,
		},
	},
	{
		.gpio		= 20,		/* NFC VEN */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_ven_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_ven_cfg,
		},
	},
};
static struct msm_gpiomux_config msm_synaptics_configs[] __initdata = {
	{
		.gpio = 17,
		.settings = {
			[GPIOMUX_ACTIVE] = &synaptics_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_int_sus_cfg,
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
#if !defined(CONFIG_SEC_VASTA_PROJECT)
	{
		.gpio      = 4,		/* NC */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_nc_cfg,
		},
	},
#endif
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
#if !defined(CONFIG_SEC_VASTA_PROJECT)
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
#endif

static struct msm_gpiomux_config msm_skuf_nfc_configs[] __initdata = {
#if !defined(CONFIG_SEC_VASTA_PROJECT)
	{					/*  NFC   WAKE */
		.gpio      = 5,
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_wake_act_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_wake_sus_cfg,
		},
	},
#endif
};

#if defined(CONFIG_SEC_VASTA_PROJECT)
static struct msm_gpiomux_config msm_nativesensors_configs[] __initdata = {
	{
		.gpio      = 65,		/* PROXY IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &prox_irq_config,
			[GPIOMUX_SUSPENDED] = &prox_irq_config,
		},
	},
#if defined (CONFIG_SEC_VASTA_PROJECT)
	{
		.gpio	   = 64,		/* ACCEL IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &accel_irq_config,
			[GPIOMUX_SUSPENDED] = &accel_irq_config,
		},
	},
#endif
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
		.dir = GPIOMUX_OUT_LOW,
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
		.gpio = 2, /* CCI_I2C_SDA_EEPROM */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 3, /* CCI_I2C_SCL_EEPROM */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 33, /* CCI_I2C_SDA_AF */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 34, /* CCI_I2C_SCL_AF */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
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
		.gpio = 28, /* CAM2_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 112, /* CAM_ANALOG_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 114, /* VT_CAM_STBY */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
};

static struct msm_gpiomux_config msm_sensor_configs_skuf_plus[] __initdata = {
	{
		.gpio = 34, /* CAM1 VCM_PWDN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
};


static struct gpiomux_setting auxpcm_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting auxpcm_sus_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm_auxpcm_configs[] __initdata = {
	{
		.gpio = 64,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
#if !defined(CONFIG_MACH_MILLETLTE_OPEN) || !defined(CONFIG_SEC_VASTA_PROJECT)
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
#endif
};
#if defined (CONFIG_SAMSUNG_JACK)
static struct gpiomux_setting earjack_gpio_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO, /*active 1*/ /* 0 */
	.drv = GPIOMUX_DRV_2MA,
#if defined(CONFIG_SEC_MILLET_PROJECT) || defined(CONFIG_MACH_CRATERQ) || defined(CONFIG_SEC_MATISSE_PROJECT)
	.pull = GPIOMUX_PULL_NONE,
#else
	.pull = GPIOMUX_PULL_DOWN,
#endif
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting earjack_gpio_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO, /*suspend*/ /* 1 */
	.drv = GPIOMUX_DRV_2MA,
#if defined(CONFIG_SEC_MILLET_PROJECT) || defined(CONFIG_MACH_CRATERQ) || defined(CONFIG_SEC_MATISSE_PROJECT)
	.pull = GPIOMUX_PULL_NONE,
#else
	.pull = GPIOMUX_PULL_DOWN,
#endif
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm_earjack_gpio_configs[] __initdata = {
	{
#if defined(CONFIG_SEC_MILLET_PROJECT) || defined(CONFIG_SEC_MATISSE_PROJECT)
		.gpio = 111, /* EAR_SEND_END */
#else
		.gpio = 35, /* EAR_SWITCH */
#endif
		.settings = {
			[GPIOMUX_ACTIVE]    = &earjack_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &earjack_gpio_suspend_cfg,
		},
	}
};
#endif
#ifdef CONFIG_SND_SOC_MAX98504
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

/* Battery charging and BMS GPIO configuration */
static struct gpiomux_setting ta_nchg_cfg[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_IN,
	},
};

static struct msm_gpiomux_config msm_ta_nchg_configs[] = {
  {
     .gpio = 54,               /* TA_NCHG */
     .settings = {
      [GPIOMUX_ACTIVE] = &ta_nchg_cfg[0],
      [GPIOMUX_SUSPENDED] = &ta_nchg_cfg[0],
    },
  },
};


extern int system_rev;

/*NC GPIOs configuration*/
static struct msm_gpiomux_config afyon_nc_gpio_cfgs[] __initdata = {
	NC_GPIO_CONFIG(113),
	NC_GPIO_CONFIG(115),
	NC_GPIO_CONFIG(116),
	NC_GPIO_CONFIG(117),
	NC_GPIO_CONFIG(118),
};

void __init msm8226_init_gpiomux(void)
{
	int rc;

	rc = msm_gpiomux_init_dt();
	if (rc) {
		pr_err("%s failed %d\n", __func__, rc);
		return;
	}

/* Battery charging and BMS GPIO */
	msm_gpiomux_install(msm_ta_nchg_configs, ARRAY_SIZE(msm_ta_nchg_configs));


#if defined(CONFIG_SEC_VASTA_PROJECT)
	msm_gpiomux_install(msm_hrm_configs,
		ARRAY_SIZE(msm_hrm_configs));
#endif
	msm_gpiomux_install(msm_nfc_configs,
		ARRAY_SIZE(msm_nfc_configs));

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

	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_goodix_configs,
				ARRAY_SIZE(msm_skuf_goodix_configs));
	else
#if defined (CONFIG_MACH_BERLUTI3G_EUR)
		msm_gpiomux_install(msm_cypress_configs,
				ARRAY_SIZE(msm_cypress_configs));

		msm_gpiomux_install(msm_keyboad_cypress_configs,
				ARRAY_SIZE(msm_keyboad_cypress_configs));
#else
		msm_gpiomux_install(msm_synaptics_configs,
				ARRAY_SIZE(msm_synaptics_configs));
#endif
	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_nfc_configs,
				ARRAY_SIZE(msm_skuf_nfc_configs));

	msm_gpiomux_install_nowrite(msm_lcd_configs,
			ARRAY_SIZE(msm_lcd_configs));

	msm_gpiomux_install(msm_sensor_configs, ARRAY_SIZE(msm_sensor_configs));

	if (of_board_is_skuf())
		msm_gpiomux_install(msm_sensor_configs_skuf_plus,
			ARRAY_SIZE(msm_sensor_configs_skuf_plus));

	msm_gpiomux_install(msm_auxpcm_configs,
			ARRAY_SIZE(msm_auxpcm_configs));

	if (of_board_is_cdp() || of_board_is_mtp() || of_board_is_xpm())
		msm_gpiomux_install(usb_otg_sw_configs,
					ARRAY_SIZE(usb_otg_sw_configs));
#if defined(CONFIG_SEC_VASTA_PROJECT)
	msm_gpiomux_install(msm_nativesensors_configs,ARRAY_SIZE(msm_nativesensors_configs));
#endif

	/*
	 * gpio mux settings for the NC GPIOs	
	 */
	msm_gpiomux_install(afyon_nc_gpio_cfgs,
			ARRAY_SIZE(afyon_nc_gpio_cfgs));

	msm_gpiomux_sdc3_install();

	/*
	 * HSIC STROBE gpio is also used by the ethernet. Install HSIC
	 * gpio mux config only when HSIC is enabled. HSIC config will
	 * be disabled when ethernet config is enabled.
	 */
#ifdef CONFIG_USB_EHCI_MSM_HSIC
	if (machine_is_msm8926()) {
		msm_hsic_configs[0].gpio = 119; /* STROBE */
		msm_hsic_configs[1].gpio = 120; /* DATA */
	}
	msm_gpiomux_install(msm_hsic_configs, ARRAY_SIZE(msm_hsic_configs));
#endif
#if defined(CONFIG_SAMSUNG_JACK)
	msm_gpiomux_install(msm_earjack_gpio_configs, ARRAY_SIZE(msm_earjack_gpio_configs));
#endif
#ifdef CONFIG_SND_SOC_MAX98504
#if defined(CONFIG_MACH_MILLETLTE_OPEN)
				if ( system_rev >= 0 && system_rev < 3)
#elif defined (CONFIG_MACH_MILLET3G_EUR)
				if ( system_rev >= 2 && system_rev < 4)
#elif defined(CONFIG_MACH_MILLETWIFI_OPEN)
				if ( system_rev >= 0 && system_rev < 5)
#endif

		{
			msm_gpiomux_install(msm8226_tertiary_mi2s_configs,ARRAY_SIZE(msm8226_tertiary_mi2s_configs));
		}
#endif
}
