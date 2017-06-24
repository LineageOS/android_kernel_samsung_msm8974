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
 *01  Millet-All nc.chaudhary	Added the gpiomux settings for		30-Jan-2014
 *	variants			UART for LCIA Test failure
 ******************************************************************************************
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
/*Adding the GPIOMUX settings for suspend states */
static struct gpiomux_setting gpio_suspend_config[] = {
	{	/* IN/ NP/ L */
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_IN,
	},
	{	/* OUT/ NP/ L */
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},
	{	/* IN/ PD/ L */
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},
	{	/* OUT/ PD/ L */
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_OUT_LOW,
	},
};

#ifdef CONFIG_USB_EHCI_MSM_HSIC
#if !defined(CONFIG_MACH_T10_3G_OPEN)
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
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],  //OUT-PD-L
		},
	},
	{
		.gpio = 116,               /* HSIC_DATA */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],  //OUT-PD-L
		},
	},
};
#endif
#endif

#define KS8851_IRQ_GPIO 115

#define IN_PD_2MA_CFG(gpio_num) { \
		.gpio = gpio_num, \
		.settings ={\
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],\
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],\
		}\
}
#define MAKE_NC_CONFIG_INIT_SLEEP(gpio_num) IN_PD_2MA_CFG(gpio_num)

#define IN_NP_2MA_CFG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],\
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],\
	}\
}
#define HW_CHK_BIT_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)

#define OUT_NP_2MA_CFG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &gpio_suspend_config[1],\
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],\
	}\
}

#define OUT_PD_2MA_CFG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &gpio_suspend_config[3],\
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],\
	}\
}

#if defined(CONFIG_MACH_T10_WIFI_OPEN)
static struct msm_gpiomux_config tn10_wifi_open_nc_gpio_cfgs[] __initdata = {
	MAKE_NC_CONFIG_INIT_SLEEP(0),
	MAKE_NC_CONFIG_INIT_SLEEP(1),
	MAKE_NC_CONFIG_INIT_SLEEP(2),
	MAKE_NC_CONFIG_INIT_SLEEP(3),
	MAKE_NC_CONFIG_INIT_SLEEP(4),
	MAKE_NC_CONFIG_INIT_SLEEP(5),
	MAKE_NC_CONFIG_INIT_SLEEP(32),
	MAKE_NC_CONFIG_INIT_SLEEP(56),
	MAKE_NC_CONFIG_INIT_SLEEP(57),
	MAKE_NC_CONFIG_INIT_SLEEP(58),
	MAKE_NC_CONFIG_INIT_SLEEP(59),
	MAKE_NC_CONFIG_INIT_SLEEP(60),
	MAKE_NC_CONFIG_INIT_SLEEP(62),
	MAKE_NC_CONFIG_INIT_SLEEP(64),
	MAKE_NC_CONFIG_INIT_SLEEP(66),
	MAKE_NC_CONFIG_INIT_SLEEP(116),
};
#endif

#if defined(CONFIG_MACH_T10_3G_OPEN)
static struct msm_gpiomux_config tn10_3g_nc_gpio_cfgs[] __initdata = {
	MAKE_NC_CONFIG_INIT_SLEEP(2),
	MAKE_NC_CONFIG_INIT_SLEEP(3),
	MAKE_NC_CONFIG_INIT_SLEEP(4),
	MAKE_NC_CONFIG_INIT_SLEEP(5),
	MAKE_NC_CONFIG_INIT_SLEEP(27),
	MAKE_NC_CONFIG_INIT_SLEEP(51),
	MAKE_NC_CONFIG_INIT_SLEEP(56),
	MAKE_NC_CONFIG_INIT_SLEEP(62),
	MAKE_NC_CONFIG_INIT_SLEEP(116),
};

static struct msm_gpiomux_config tn10_3g_nc_gpio22_cfgs[] __initdata = {
	MAKE_NC_CONFIG_INIT_SLEEP(22),
};
#endif
static struct msm_gpiomux_config tn10_hw_chk_bit_cfgs[] __initdata = {
	HW_CHK_BIT_CFG(63),     // BIT 0
	HW_CHK_BIT_CFG(49),     // BIT 1
	HW_CHK_BIT_CFG(50),     // BIT 2
	HW_CHK_BIT_CFG(114),    // BIT 3
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

static struct gpiomux_setting gpio_keys_active = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_keys_suspend = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting gpio_keys_suspend_vd = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

#ifndef CONFIG_SND_SOC_MAX98504
#if !defined(CONFIG_SEC_T10_WIFI_COMMON)
static struct gpiomux_setting gpio_spi_cs_eth_config = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif
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
#define WCNSS_5WIRE_CFG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &wcnss_5wire_active_cfg,\
		[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,\
	}\
}

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
#define WCNSS_5GPIO_CFG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &wcnss_5gpio_active_cfg,\
		[GPIOMUX_SUSPENDED] = &wcnss_5gpio_suspend_cfg,\
	}\
}
#endif

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
		.gpio = 107,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_keys_active,
			[GPIOMUX_SUSPENDED] = &gpio_keys_suspend_vd,
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

static struct gpiomux_setting lcd_in_gpio_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
#define LCD_GPIO_CONFIG_1(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &lcd_in_gpio_cfg,\
		[GPIOMUX_SUSPENDED] = &lcd_in_gpio_cfg,\
	}\
}
#define LCD_GPIO_CONFIG_2(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &lcd_rst_act_cfg,\
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],\
	}\
}
#define LCD_GPIO_CONFIG_3(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &gpio_suspend_config[1],\
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],\
	}\
}
#define LCD_GPIO_CONFIG_4(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &lcd_rst_act_cfg,\
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],\
	}\
}
static struct msm_gpiomux_config msm_lcd_configs[] __initdata = {
	LCD_GPIO_CONFIG_4(25),	/* LVDS Ldo EN */
	LCD_GPIO_CONFIG_1(53),	/* LCD LDI INT */
	LCD_GPIO_CONFIG_1(20),	/* Backlight SDA */
	LCD_GPIO_CONFIG_1(21),	/* Backlight SCL */
	LCD_GPIO_CONFIG_2(74),	/* Backlight Reset */
	LCD_GPIO_CONFIG_2(51),	/*LVDS Reset*/
	LCD_GPIO_CONFIG_3(33),	/* Backlight PWM AP */
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

#define TSP_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)
#define I2C_GPIO_CFG(gpio_num) { \
	.gpio	= gpio_num,\
	.settings = {\
		[GPIOMUX_ACTIVE] = &gpio_i2c_config,\
		[GPIOMUX_SUSPENDED] = &gpio_i2c_config,\
	},\
}

#if defined(CONFIG_SEC_T10_PROJECT)
static struct gpiomux_setting gpio_i2c_config_tsp = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

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
		.gpio      = 18,		/* SW I2C_SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config_tsp,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_tsp,
		},
	},
	{
		.gpio      = 19,		/* SW I2C_SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config_tsp,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_tsp,
		},
	},
	TSP_CFG(55),		/* BLSP1 QUP5 I2C_SCL TSP GPIO */
#ifndef CONFIG_SND_SOC_MAX98504
#if !defined(CONFIG_SEC_T10_WIFI_COMMON)
	{
		.gpio      = 22,		/* BLSP1 QUP1 SPI_CS_ETH */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs_eth_config,
		},
	},
#endif
#endif
	I2C_GPIO_CFG(6),	/* BLSP1 QUP2 I2C_SDA */
	I2C_GPIO_CFG(7),	/* BLSP1 QUP2 I2C_SCL */
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

static struct gpiomux_setting goodix_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting goodix_reset_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
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
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],	// IN-PD-L
		},
	},
	{
		.gpio = 16,		/* RESET */
		.settings = {
			[GPIOMUX_ACTIVE] = &goodix_reset_act_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],	// IN-PD-L
		},
	},
	{
		.gpio = 17,		/* INT */
		.settings = {
			[GPIOMUX_ACTIVE] = &goodix_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],	// IN-PD-L
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

static struct msm_gpiomux_config msm_skuf_nfc_configs[] __initdata = {
	{					/*  NFC   WAKE */
		.gpio      = 5,
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_wake_act_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],	// OUT-NP-L
		},
	},
};

static struct gpiomux_setting sd_card_det_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config sd_card_det __initdata = {
	.gpio = 38,
	.settings = {
		[GPIOMUX_ACTIVE]    = &sd_card_det_active_config,
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],	// IN-NP-L
	},
};

#define GRIP_IRQ_CFG(gpio_num)	IN_NP_2MA_CFG(gpio_num)
#define ACCEL_IRQ_CFG(gpio_num) IN_PD_2MA_CFG(gpio_num)
#define HALL_IC_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)
static struct msm_gpiomux_config msm_nativesensors_configs[] __initdata = {
	GRIP_IRQ_CFG(66),	/* GRIP IRQ */
	ACCEL_IRQ_CFG(54),	/* ACCEL IRQ */
	HALL_IC_CFG(110),	/* Hall IC GPIOs */
};

#if defined(CONFIG_SEC_T10_WIFI_COMMON)

#define NFC_EN_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)
#define NFC_IRQ_CFG(gpio_num) IN_PD_2MA_CFG(gpio_num)
#define NFC_FW_CFG(gpio_num) OUT_NP_2MA_CFG(gpio_num)
static struct msm_gpiomux_config msm_nfc_configs[] __initdata = {
	NFC_IRQ_CFG(13),	/* NFC IRQ */
	NFC_EN_CFG(0),		/* NFC EN */
	NFC_FW_CFG(1),		/* NFC FIRMWARE */
};
#endif

static struct msm_gpiomux_config wcnss_5wire_interface[] = {
	WCNSS_5WIRE_CFG(40),
	WCNSS_5WIRE_CFG(41),
	WCNSS_5WIRE_CFG(42),
	WCNSS_5WIRE_CFG(43),
	WCNSS_5WIRE_CFG(44),
};

#ifdef CONFIG_WCNSS_IRIS_REGISTER_DUMP
static struct msm_gpiomux_config wcnss_5gpio_interface[] = {
	WCNSS_5GPIO_CFG(40),
	WCNSS_5GPIO_CFG(41),
	WCNSS_5GPIO_CFG(42),
	WCNSS_5GPIO_CFG(43),
	WCNSS_5GPIO_CFG(44),
};
#endif

static struct msm_gpiomux_config tsp_gpiomux_config[] = {

	{
		.gpio = 55, /* TSP_VENDOR1 */
		.settings = {
			[GPIOMUX_ACTIVE]	= &gpio_suspend_config[0], //IP NP and Low
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0], //IP NP and Low
		},
	},

	{
		.gpio = 73, /* TSP_VENDOR2 */
		.settings = {
			[GPIOMUX_ACTIVE]	= &gpio_suspend_config[0], //IP NP and Low
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0], //IP NP and Low
		},
	},

	{
		.gpio = 31, /* TOUCH_EN */
		.settings = {
			[GPIOMUX_ACTIVE]	= &gpio_suspend_config[1], //OP NP and Low
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1], //OP NP and Low
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
		.gpio = 16, /* VTCAM_1.8V */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[5],
			[GPIOMUX_SUSPENDED] = &cam_settings[5],
		},
	},
	{
		.gpio = 26, /* CAM_MCLK0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
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
		.gpio = 29, /* CCI_I2C_SDA0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],	// IN-NP-L
		},
	},
	{
		.gpio = 30, /* CCI_I2C_SCL0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],	// IN-NP-L
		},
	},
	{
		.gpio = 35, /* CAM2_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 36, /* CAM1_STANDBY_N */
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
		.gpio = 112, /* CAM1_ANALOG_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[5],
			[GPIOMUX_SUSPENDED] = &cam_settings[5],
		},
	},
};

static struct msm_gpiomux_config msm_sensor_configs_skuf_plus[] __initdata = {
#if !defined(CONFIG_SEC_T10_WIFI_COMMON)
	{
		.gpio = 22, /* CAM1_VDD */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
#endif
	{
		.gpio = 34, /* CAM1 VCM_PWDN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
};

#define TA_NCHG_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)
#define OVP_ENABLE_CFG(gpio_num) OUT_NP_2MA_CFG(gpio_num)
static struct msm_gpiomux_config msm_ta_nchg_configs[] = {
	TA_NCHG_CFG(10),	/* CHG SDA 1.8V */
	TA_NCHG_CFG(52),	/* TA_NCHG */
	TA_NCHG_CFG(115),	/* TA_INT_N */
	IN_PD_2MA_CFG(46),      /* Fuel alert*/
};

static struct msm_gpiomux_config ovp_enable_configs[] = {
	OVP_ENABLE_CFG(64),
};

#if defined (CONFIG_SAMSUNG_JACK)
#define EARJACK_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)
static struct msm_gpiomux_config msm_earjack_gpio_configs[] __initdata = {
	EARJACK_CFG(111),	/* EAR_SEND_END */
};
#endif


static struct gpiomux_setting lineout_en_gpio_configs = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.dir = GPIOMUX_OUT_LOW,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm_lineout_en_gpio_configs[] __initdata = {
	{
#if defined(CONFIG_SEC_T10_WIFI_COMMON)
		.gpio = 12, /* LINEOUT_EN */
#else
		.gpio = 23, /* LINEOUT_EN */
#endif
		.settings = {
			[GPIOMUX_ACTIVE]	= &lineout_en_gpio_configs,
			[GPIOMUX_SUSPENDED] = &lineout_en_gpio_configs,
		},
	}
};

static struct gpiomux_setting spk_en_gpio_configs = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.dir = GPIOMUX_OUT_LOW,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm_spk_en_gpio_configs[] __initdata = {
	{
		.gpio = 24, /* SPK_EN */
		.settings = {
			[GPIOMUX_ACTIVE]	= &spk_en_gpio_configs,
			[GPIOMUX_SUSPENDED] = &spk_en_gpio_configs,
		},
	}
};

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

extern int system_rev;

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
	msm_gpiomux_install(tsp_gpiomux_config,
				ARRAY_SIZE(tsp_gpiomux_config));

	msm_gpiomux_install(&sd_card_det, 1);
	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_goodix_configs,
				ARRAY_SIZE(msm_skuf_goodix_configs));
	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_nfc_configs,
				ARRAY_SIZE(msm_skuf_nfc_configs));

	msm_gpiomux_install_nowrite(msm_lcd_configs,
			ARRAY_SIZE(msm_lcd_configs));

	msm_gpiomux_install(msm_sensor_configs, ARRAY_SIZE(msm_sensor_configs));

	if (of_board_is_skuf())
		msm_gpiomux_install(msm_sensor_configs_skuf_plus,
			ARRAY_SIZE(msm_sensor_configs_skuf_plus));

	msm_gpiomux_install(msm_ta_nchg_configs, ARRAY_SIZE(msm_ta_nchg_configs));
	msm_gpiomux_install(ovp_enable_configs, ARRAY_SIZE(ovp_enable_configs));
	if (of_board_is_cdp() || of_board_is_mtp() || of_board_is_xpm())
		msm_gpiomux_install(usb_otg_sw_configs,
					ARRAY_SIZE(usb_otg_sw_configs));
	msm_gpiomux_install(msm_nativesensors_configs,ARRAY_SIZE(msm_nativesensors_configs));
	msm_gpiomux_sdc3_install();
#if defined(CONFIG_SEC_T10_WIFI_COMMON)
	msm_gpiomux_install(msm_nfc_configs,	ARRAY_SIZE(msm_nfc_configs));
#endif
	/*
	 * HSIC STROBE gpio is also used by the ethernet. Install HSIC
	 * gpio mux config only when HSIC is enabled. HSIC config will
	 * be disabled when ethernet config is enabled.
	 */
#ifdef CONFIG_USB_EHCI_MSM_HSIC
#if !defined(CONFIG_MACH_T10_3G_OPEN)
	if (machine_is_msm8926()) {
		msm_hsic_configs[0].gpio = 119; /* STROBE */
		msm_hsic_configs[1].gpio = 120; /* DATA */
	}
	msm_gpiomux_install(msm_hsic_configs, ARRAY_SIZE(msm_hsic_configs));
#endif
#endif
#if defined(CONFIG_SAMSUNG_JACK)
	msm_gpiomux_install(msm_earjack_gpio_configs, ARRAY_SIZE(msm_earjack_gpio_configs));
#endif
	msm_gpiomux_install(msm_lineout_en_gpio_configs, ARRAY_SIZE(msm_lineout_en_gpio_configs));
	msm_gpiomux_install(msm_spk_en_gpio_configs, ARRAY_SIZE(msm_spk_en_gpio_configs));
#ifdef CONFIG_SND_SOC_MAX98504
	{
		msm_gpiomux_install(msm8226_tertiary_mi2s_configs,ARRAY_SIZE(msm8226_tertiary_mi2s_configs));
		msm_gpiomux_install(msm8226_blsp_codec_configs,ARRAY_SIZE(msm8226_blsp_codec_configs));
	}
#endif
/* Install NC Configurations */
#if defined(CONFIG_MACH_T10_WIFI_OPEN)
	msm_gpiomux_install(tn10_wifi_open_nc_gpio_cfgs, ARRAY_SIZE(tn10_wifi_open_nc_gpio_cfgs));
#endif
	msm_gpiomux_install(tn10_hw_chk_bit_cfgs, ARRAY_SIZE(tn10_hw_chk_bit_cfgs));
#if defined (CONFIG_MACH_T10_3G_OPEN)
	msm_gpiomux_install(tn10_3g_nc_gpio_cfgs, ARRAY_SIZE(tn10_3g_nc_gpio_cfgs));
	if(system_rev >= 2)
		msm_gpiomux_install(tn10_3g_nc_gpio22_cfgs, ARRAY_SIZE(tn10_3g_nc_gpio22_cfgs));
#endif
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
