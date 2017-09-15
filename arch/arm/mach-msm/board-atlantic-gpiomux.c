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
 *01  Atlantic LTE	nc.chaudhary	Intial file creation			27-Jan-2014
 *	3G
 *02  Atlantic-All	nc.chaudhary	Added the gpiomux settings for		30-Jan-2014
 *	variants			UART for LCIA Test failure
 *02  Atlantic-ATT	nc.chaudhary	Added the gpiomux settings for		12-Feb-2014
 *    Kmini-ATT				NC GPIOs as per new schematics
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
	{       /* IN/ NP/ L */
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_IN,
	},
	{       /* OUT/ NP/ L */
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},
	{       /* IN/ PD/ L */
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},
	{       /* OUT/ PD/ L */
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_OUT_LOW,
	},
};

#ifdef CONFIG_USB_EHCI_MSM_HSIC
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
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],	//OUT-PD-L
		},
	},
	{
		.gpio = 116,               /* HSIC_DATA */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],	//OUT-PD-L
		},
	},
};
#endif

#define KS8851_IRQ_GPIO 115

/* IN-NP-L */
#define IN_NP_2MA_CFG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &gpio_suspend_config[0], \
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0], \
	}\
}
#define HW_CHK_BIT_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)

/* IN-PD-L */
#define IN_PD_2MA_CFG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],\
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],\
	}\
}
#define NC_GPIO_CONFIG(gpio_num) IN_PD_2MA_CFG(gpio_num)

/* OUT-NP-L */
#define OUT_NP_2MA_CFG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &gpio_suspend_config[1], \
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1], \
	}\
}
/* OUT-PD-L */
#define OUT_PD_2MA_CFG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &gpio_suspend_config[3], \
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3], \
	}\
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

static struct gpiomux_setting cypress_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
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
#endif

static struct gpiomux_setting gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_i2c_config_tsp = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting sensor_gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting nfc_hrm_gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

#if defined (CONFIG_MACH_ATLANTIC3GEUR_OPEN)
static struct gpiomux_setting hrm_leden_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};
#endif

#if !defined (CONFIG_MACH_ATLANTIC3GEUR_OPEN)
static struct gpiomux_setting nfc_ven_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting nfc_irq_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};
#endif

#if defined(CONFIG_SEC_ATLANTICLTE_COMMON)
#define EXPANDER_GPIO(gpio_num) IN_NP_2MA_CFG(gpio_num)
static struct msm_gpiomux_config expander_configs[] __initdata = {
        EXPANDER_GPIO(0),
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
		.gpio = 108,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_keys_active,
			[GPIOMUX_SUSPENDED] = &gpio_keys_suspend,
		},
	},
};

static struct gpiomux_setting lcd_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting lcd_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

#define LCD_ESD_DET(gpio_num) IN_NP_2MA_CFG(gpio_num)

static struct msm_gpiomux_config msm_lcd_configs[] __initdata = {
	{
		.gpio = 25,		/* LCD Reset */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_sus_cfg, //OUT-NP-L
		},
	},
	{
		.gpio = 115,		/* LCD_ON*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_sus_cfg, //OUT-PD-L
		},
	},
	{
		.gpio = 72,		/* LCD_ON*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_suspend_config[0], //IN-NP
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2], //IN-PD
		},
	},
LCD_ESD_DET(16),
};
static struct gpiomux_setting gpio_atlantic_cover_id_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct msm_gpiomux_config msm8974_cover_id_config[] __initdata = {
	{
		.gpio = 60,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_atlantic_cover_id_config,
			[GPIOMUX_SUSPENDED] = &gpio_atlantic_cover_id_config,
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
		.gpio      = 2,		/* BLSP1 QUP1 SPI_CS1 */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_cs_act_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],	// IN-PD-L
		},
	},
	{
		.gpio      = 3,		/* BLSP1 QUP1 SPI_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_act_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],	// IN-PD-L
		},
	},

#if !defined(CONFIG_SEC_ATLANTICLTE_COMMON)
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
};

#if !defined (CONFIG_MACH_ATLANTIC3GEUR_OPEN)
static struct msm_gpiomux_config msm_nfc_configs[] __initdata = {
	{
		.gpio      = 107,		/* NFC IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_irq_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_irq_cfg,
		},
	},
	{
		.gpio		= 110,		/* NFC VEN */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_ven_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_ven_cfg,
		},
	},
};
#endif

static struct msm_gpiomux_config msm_cypress_configs[] __initdata = {
	{
		.gpio = 17,
		.settings = {
			[GPIOMUX_ACTIVE] = &cypress_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],	// IN-PD-L
		},
	},
};

#if defined (CONFIG_MACH_ATLANTIC3GEUR_OPEN)
static struct gpiomux_setting nc_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config nc_configs[] __initdata = {
	{
		.gpio = 107,
		.settings = {
			[GPIOMUX_ACTIVE] = &nc_cfg,
			[GPIOMUX_SUSPENDED] = &nc_cfg,
		},
	},
};
#endif

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

#define PROXI_IRQ(gpio_num) IN_NP_2MA_CFG(gpio_num)
#define ACCEL_IRQ(gpio_num) IN_PD_2MA_CFG(gpio_num)
#define HRM_IRQ(gpio_num) IN_PD_2MA_CFG(gpio_num)
static struct msm_gpiomux_config msm_nativesensors_configs[] __initdata = {
	PROXI_IRQ(65),			/* PROXY IRQ */
	ACCEL_IRQ(64),			/* ACCEL IRQ */
	HRM_IRQ(63),			/* HRM IRQ */
#if defined(CONFIG_SEC_ATLANTIC3G_COMMON)
	{
		.gpio	   = 0,		/* HRM LED EN */
		.settings = {
			[GPIOMUX_ACTIVE] = &hrm_leden_config,
			[GPIOMUX_SUSPENDED] = &hrm_leden_config,
		},
	},
#endif
};

static struct msm_gpiomux_config wcnss_5wire_interface[] = {
	WCNSS_5WIRE_CFG(40),
	WCNSS_5WIRE_CFG(41),
	WCNSS_5WIRE_CFG(42),
	WCNSS_5WIRE_CFG(43),
	WCNSS_5WIRE_CFG(44),
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
	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend 0*/ /* 6 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
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
#if defined(CONFIG_SEC_ATLANTIC3G_COMMON)
	{
		.gpio = 37, /* CAM1_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
#endif
	{
		.gpio = 28, /* CAM2_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 53, /* CAM1_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 112, /* CAM_ANALOG_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[6],
		},
	},

#if defined(CONFIG_SEC_ATLANTIC3G_COMMON)
	{
		.gpio = 74, /* VT_CAM_STBY */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
#endif

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

#if defined(CONFIG_SENSORS_VFS61XX)
static struct gpiomux_setting gpio_spi_btp_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_spi_btp_irq_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_spi_btp_rst_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config msm_fingerprint_configs[] __initdata = {
	{
		/* MOSI */
		.gpio = 20,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],	// OUT-NP-L
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_config,
		},
	},
	{
		/* MISO */
		.gpio = 21,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_irq_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_config,
		},
	},
	{
		/* CS */
		.gpio = 22,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],	// OUT-NP-L
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_config,
		},
	},
	{
		/* CLK  */
		.gpio = 23,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],	// OUT-NP-L
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_config,
		},
	},
	{
		/* BTP_SLEEP */
		.gpio = 114,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
	{
		/* BTP_IRQ */
		.gpio = 62,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_irq_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_irq_config,
		},
	},
	{
		/* BTP_OCP_FLAG */
		.gpio = 38,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_irq_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_irq_config,
		},
	},
};
#endif

/* GPIO 64, 65, 66 are GYRO_INT, PROXI_INT, BATT_ALARM
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
#if !defined(CONFIG_MACH_MILLETLTE_OPEN) || !defined(CONFIG_SEC_ATLANTIC_PROJECT)
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
*/
#if defined (CONFIG_SAMSUNG_JACK)
#define EARJACK_CFG(gpio_num) IN_PD_2MA_CFG(gpio_num)
static struct msm_gpiomux_config msm_earjack_gpio_configs[] __initdata = {
	EARJACK_CFG(35),
};
#endif

#ifdef CONFIG_SND_SOC_ES325_ATLANTIC

static struct gpiomux_setting es325_active = {
	.pull = GPIOMUX_PULL_UP,
	.drv = GPIOMUX_DRV_8MA,
	.func = GPIOMUX_FUNC_GPIO,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting es325_reset = {
	.pull = GPIOMUX_PULL_UP,
	.drv = GPIOMUX_DRV_2MA,
	.func = GPIOMUX_FUNC_GPIO,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct msm_gpiomux_config msm_es325_config[] __initdata = {
	{
		.gpio	= 55,		/* SYS_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE] = &es325_active,
			[GPIOMUX_SUSPENDED] = &es325_reset,
		},
	},
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
/* GPIO 67 is HW_REV1
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
*/
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
#define TA_NCHG_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)

static struct msm_gpiomux_config msm_ta_nchg_configs[] = {
	TA_NCHG_CFG(54),
};

#if defined(CONFIG_SEC_ATLANTICLTE_COMMON)
static struct msm_gpiomux_config msm_lte_chg_configs[] = {
        IN_NP_2MA_CFG(14), /* FUEL_CHG_SDA */
        IN_NP_2MA_CFG(66), /* BATT ALARM*/
        IN_NP_2MA_CFG(119), /* INOK */
};
#endif

#if defined(CONFIG_SEC_ATLANTIC_PROJECT)

#define CHG_DET_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)

static struct msm_gpiomux_config msm_chg_det_configs[] = {
	CHG_DET_CFG(51),
};

#endif

extern int system_rev;

/*NC GPIOs configuration*/
#if defined(CONFIG_MACH_ATLANTICLTE_ATT)
static struct msm_gpiomux_config atlantic_att_nc_gpios[] __initdata = {
	NC_GPIO_CONFIG(37),
	NC_GPIO_CONFIG(45),
	NC_GPIO_CONFIG(46),
	NC_GPIO_CONFIG(77),
	NC_GPIO_CONFIG(78),
	NC_GPIO_CONFIG(79),
	NC_GPIO_CONFIG(80),
	NC_GPIO_CONFIG(81),
	NC_GPIO_CONFIG(82),
	NC_GPIO_CONFIG(83),
	NC_GPIO_CONFIG(93),
	NC_GPIO_CONFIG(94),
	NC_GPIO_CONFIG(97),
	NC_GPIO_CONFIG(98),
	NC_GPIO_CONFIG(104),
	NC_GPIO_CONFIG(111),
};
#endif

#if defined(CONFIG_MACH_ATLANTICLTE_USC)
static struct msm_gpiomux_config atlantic_usc_nc_gpios[] __initdata = {
	NC_GPIO_CONFIG(37),
};
#endif

/* HW_REV bits configuration, true for 3G, ATT, USC, VZW */
static struct msm_gpiomux_config atlantic_hw_rev_cfgs[] __initdata = {
	HW_CHK_BIT_CFG(24),	// BIT 0
	HW_CHK_BIT_CFG(67),	// BIT 1
	HW_CHK_BIT_CFG(13),	// BIT 2
	HW_CHK_BIT_CFG(12),	// BIT 3
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

#if defined (CONFIG_SEC_ATLANTICLTE_COMMON)
        msm_gpiomux_install(msm_lte_chg_configs, ARRAY_SIZE(msm_lte_chg_configs));
#endif

#if defined (CONFIG_SEC_ATLANTIC_PROJECT)
	msm_gpiomux_install(msm_chg_det_configs, ARRAY_SIZE(msm_chg_det_configs));
#endif

#if !defined (CONFIG_MACH_ATLANTIC3GEUR_OPEN)
	msm_gpiomux_install(msm_nfc_configs,
		ARRAY_SIZE(msm_nfc_configs));
#endif

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	msm_gpiomux_install(msm_eth_configs, ARRAY_SIZE(msm_eth_configs));
#endif

#if defined(CONFIG_SEC_ATLANTICLTE_COMMON)
	msm_gpiomux_install(expander_configs,
			ARRAY_SIZE(expander_configs));
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
		msm_gpiomux_install(msm_cypress_configs,
				ARRAY_SIZE(msm_cypress_configs));
#endif
	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_nfc_configs,
				ARRAY_SIZE(msm_skuf_nfc_configs));

	if (system_rev >= 1)
		msm_gpiomux_install(msm8974_cover_id_config, ARRAY_SIZE(msm8974_cover_id_config));
	else
		msm_gpiomux_install_nowrite(msm_lcd_configs,
			ARRAY_SIZE(msm_lcd_configs));

	msm_gpiomux_install(msm_sensor_configs, ARRAY_SIZE(msm_sensor_configs));

#ifdef CONFIG_SENSORS_VFS61XX
	msm_gpiomux_install(msm_fingerprint_configs, ARRAY_SIZE(msm_fingerprint_configs));
#endif

	if (of_board_is_skuf())
		msm_gpiomux_install(msm_sensor_configs_skuf_plus,
			ARRAY_SIZE(msm_sensor_configs_skuf_plus));
/*
	msm_gpiomux_install(msm_auxpcm_configs,
			ARRAY_SIZE(msm_auxpcm_configs));

	if (of_board_is_cdp() || of_board_is_mtp() || of_board_is_xpm())
		msm_gpiomux_install(usb_otg_sw_configs,
					ARRAY_SIZE(usb_otg_sw_configs));
*/
	msm_gpiomux_install(msm_nativesensors_configs,ARRAY_SIZE(msm_nativesensors_configs));

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
#ifdef CONFIG_SND_SOC_ES325_ATLANTIC
	msm_gpiomux_install(msm_es325_config, ARRAY_SIZE(msm_es325_config));
#endif
#ifdef CONFIG_SND_SOC_MAX98504
	msm_gpiomux_install(msm8226_tertiary_mi2s_configs,ARRAY_SIZE(msm8226_tertiary_mi2s_configs));
#endif
#if defined (CONFIG_MACH_ATLANTIC3GEUR_OPEN)
	msm_gpiomux_install(nc_configs, ARRAY_SIZE(nc_configs));
#endif
	/* Install HW_REV GPIO configurations */
	msm_gpiomux_install(atlantic_hw_rev_cfgs, ARRAY_SIZE(atlantic_hw_rev_cfgs));
	/* Install NC GPIO configurations */
#if defined(CONFIG_MACH_ATLANTICLTE_ATT)
        msm_gpiomux_install(atlantic_att_nc_gpios, ARRAY_SIZE(atlantic_att_nc_gpios));
#endif

#if defined(CONFIG_MACH_ATLANTICLTE_USC)
        msm_gpiomux_install(atlantic_usc_nc_gpios, ARRAY_SIZE(atlantic_usc_nc_gpios));
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
