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
#define WLAN_CLK	44
#define WLAN_SET	43
#define WLAN_DATA0	42
#define WLAN_DATA1	41
#define WLAN_DATA2	40
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
static struct gpiomux_setting hsic_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm_hsic_configs[] = {
	{
		.gpio = 119,               /* HSIC_STROBE */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],
		},
	},
	{
		.gpio = 116,               /* HSIC_DATA */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],
		},
	},
};
#endif

#define KS8851_IRQ_GPIO 115
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
#define IN_PD_2MA_CFG(gpio_num) { \
	.gpio = gpio_num, \
	.settings ={\
		[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],\
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],\
	}\
}
#define NC_GPIO_CONFIG(gpio_num) IN_PD_2MA_CFG(gpio_num)
#define OUT_PD_2MA_CFG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &gpio_suspend_config[3],\
		[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],\
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

static struct gpiomux_setting gpio_spi_stm_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_spi_stm_sus_config[] = {
	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},
	{
		.func = GPIOMUX_FUNC_2,
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
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

#define WCNSS_5WIRE_CFG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &wcnss_5wire_active_cfg,\
		[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,\
	}\
}

static struct gpiomux_setting gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

#define I2C_GPIO_CONFIG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &gpio_i2c_config,\
		[GPIOMUX_SUSPENDED] = &gpio_i2c_config,\
	}\
}

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

#define LCD_ESD_DET(gpio_num) IN_NP_2MA_CFG(gpio_num)

static struct msm_gpiomux_config msm_lcd_configs[] __initdata = {
	{
		.gpio = 25,		/* LCD Reset */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_rst_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_rst_sus_cfg,
		},
	},
	{
		.gpio = 60,		/* LCD ON*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_pwr_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_pwr_sus_cfg,
		},
	},
	LCD_ESD_DET(79),
	LCD_ESD_DET(80),
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
		.gpio      = 12,		/* UART TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 13,		/* UART RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &rx_gpio_uart_config,
		},
	},
	{
		.gpio      = 16,		/* UART TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 17,		/* UART RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &rx_gpio_uart_config,
		},
	},
	{
		.gpio      = 55,		/* BLSP1 QUP1 SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_tkey_active_config,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_tkey_suspend_config,
		},
	},
	{
		.gpio      = 56,		/* BLSP1 QUP1 SPI_DATA_MISO */
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
	{
		.gpio      = 4,	/* BLSP1 QUP1 SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_stm_config,
			[GPIOMUX_SUSPENDED] = &gpio_spi_stm_sus_config[0],
		},
	},
	{
		.gpio      = 5,		/* BLSP1 QUP1 SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_stm_config,
			[GPIOMUX_SUSPENDED] = &gpio_spi_stm_sus_config[0],
		},
	},
	{
		.gpio      = 6,		/* BLSP1 QUP1 SPI_CS1 */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_stm_config,
			[GPIOMUX_SUSPENDED] = &gpio_spi_stm_sus_config[1],
		},
	},
	{
		.gpio      = 7,		/* BLSP1 QUP1 SPI_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_stm_config,
			[GPIOMUX_SUSPENDED] = &gpio_spi_stm_sus_config[1],
		},
	},
	I2C_GPIO_CONFIG(18),		/* BLSP1 QUP5 I2C_SDA */
	I2C_GPIO_CONFIG(19),		/* BLSP1 QUP5 I2C_SCL */
};
#define HRM_IRQ_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)
static struct msm_gpiomux_config msm_hrm_configs[] __initdata = {
	HRM_IRQ_CFG(63),		 /* HRM IRQ */
};

static struct gpiomux_setting ssp_setting[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_HIGH,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},
};

#define SSPN_CONFIG(gpio_num, n) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &ssp_setting[n],\
		[GPIOMUX_SUSPENDED] = &ssp_setting[n],\
	}\
}
static struct msm_gpiomux_config msm_sensorhub_configs[] __initdata = {
	SSPN_CONFIG(64, 0),			/* MCU INT 1 IRQ */
	SSPN_CONFIG(65, 0),			/* MCU INT 2 IRQ */
	SSPN_CONFIG(118, 1),			/* AP INT IRQ */
	SSPN_CONFIG(104, 0),			/* RESET */
	SSPN_CONFIG(111, 2),			/* HRM EXT LDO EN */
};

#define NFC_IRQ_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)
#define NFC_FW_CFG(gpio_num) OUT_NP_2MA_CFG(gpio_num)
#define NFC_EN_CFG(gpio_num) OUT_PD_2MA_CFG(gpio_num)
static struct msm_gpiomux_config msm_nfc_configs[] __initdata = {
	NFC_IRQ_CFG(46),	/* NFC IRQ */
};

#define BGM_IRQ_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)
#define BGM_IN_CFG(gpio_num) IN_NP_2MA_CFG(gpio_num)
#define BGM_OUT_CFG(gpio_num) OUT_PD_2MA_CFG(gpio_num)
static struct msm_gpiomux_config msm_bgm_configs[] __initdata = {
	BGM_IN_CFG(62),
	BGM_IN_CFG(115),
};

static struct msm_gpiomux_config msm_synaptics_configs[] __initdata = {
	{
		.gpio = 120,
		.settings = {
			[GPIOMUX_ACTIVE] = &synaptics_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_int_sus_cfg,
		},
	},
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
	NC_GPIO_CONFIG(2),
	NC_GPIO_CONFIG(3),
	NC_GPIO_CONFIG(4),
};

static struct msm_gpiomux_config msm_skuf_goodix_configs[] __initdata = {
	{
		.gpio = 15,		/* LDO EN */
		.settings = {
			[GPIOMUX_ACTIVE] = &goodix_ldo_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio = 17,		/* INT */
		.settings = {
			[GPIOMUX_ACTIVE] = &goodix_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	I2C_GPIO_CONFIG(18),		/* BLSP1 QUP5 I2C_SDA */
	I2C_GPIO_CONFIG(19),		/* BLSP1 QUP5 I2C_SCL */
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
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
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
		.func = GPIOMUX_FUNC_GPIO, /*suspend*/  /* 6 */
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_OUT_LOW,
	},
};

#define CAM_COMMON_CONFIG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &cam_settings[3],\
		[GPIOMUX_SUSPENDED] = &cam_settings[4],\
	}\
}
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
	CAM_COMMON_CONFIG(2),	/* CCI_I2C_SDA_EEPROM */
	CAM_COMMON_CONFIG(3),	/* CCI_I2C_SCL_EEPROM */
	CAM_COMMON_CONFIG(33),	/* CCI_I2C_SDA_AF */
	CAM_COMMON_CONFIG(34),	/* CCI_I2C_SCL_AF */
	CAM_COMMON_CONFIG(35),  /* CAM2_STANDBY_N */
	CAM_COMMON_CONFIG(36),	/* CAM1_STANDBY_N */
	CAM_COMMON_CONFIG(37),	/* CAM1_RST_N */
	CAM_COMMON_CONFIG(28),	/* CAM2_RST_N */
	{
		.gpio = 113, /* CAM_ANALOG_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[6],
		},
	},
};

static struct msm_gpiomux_config msm_sensor_configs_skuf_plus[] __initdata = {
	CAM_COMMON_CONFIG(34),	/* CAM1 VCM_PWDN */
};

#if defined(CONFIG_SENSORS_VFS61XX)
static struct gpiomux_setting gpio_spi_ocp_flag_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
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
		/* BTP_INT */
		.gpio = 110,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_irq_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_irq_config,
		},
	},
	{
		/* BTP_RST_N */
		.gpio = 114,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_rst_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
	{
		/* BTP_LDO */
		.gpio = 438,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_rst_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
	{
		/* BTP_LDO2 */
		.gpio = 435,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_rst_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
	{
		/* BTP_OCP_EN */
		.gpio = 441,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_rst_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
	{
		/* BTP_OCP_FLAG */
		.gpio = 38,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_ocp_flag_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_ocp_flag_config,
		},
	},
};
#endif


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
};

#ifdef CONFIG_SND_SOC_ES705
static struct gpiomux_setting es705_intrevent_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config es705_config[] __initdata = {
	{
		.gpio	= 0,		/* es705 uart tx */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio	= 1,		/* es705 uart rx */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio	= 305,		/* es705 2mic int */
		.settings = {
			[GPIOMUX_SUSPENDED] = &es705_intrevent_config,
		},
	},
};
#endif /* CONFIG_SND_SOC_ES705 */

#if defined (CONFIG_SAMSUNG_JACK)
#define EAR_SWITCH_CFG(gpio_num) IN_PD_2MA_CFG(gpio_num)
static struct msm_gpiomux_config msm_earjack_gpio_configs[] __initdata = {
	EAR_SWITCH_CFG(35),	/* EAR_SWITCH */
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
#define TERTIARY_MI2S_CONFIG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &pri_mi2s_act_cfg,\
		[GPIOMUX_SUSPENDED] = &pri_mi2s_sus_cfg,\
	}\
}
static struct msm_gpiomux_config msm8226_tertiary_mi2s_configs[] __initdata = {
	TERTIARY_MI2S_CONFIG(49),	/* qua mi2s sck */
	TERTIARY_MI2S_CONFIG(50),
	TERTIARY_MI2S_CONFIG(51),
	TERTIARY_MI2S_CONFIG(52),
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

static struct gpiomux_setting wcdcodec_reset_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.dir = GPIOMUX_OUT_LOW,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config wcdcodec_reset_cfg[] __initdata = {
	{
		.gpio = 72,
		.settings = {
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

#define SDC3_COMMON_CONFIG(gpio_num) { \
	.gpio = gpio_num, \
	.settings = {\
		[GPIOMUX_ACTIVE] = &sdc3_cmd_data_0_3_actv_cfg,\
		[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,\
	}\
}
static struct msm_gpiomux_config msm8226_sdc3_configs[] __initdata = {
	SDC3_COMMON_CONFIG(39),		/* DAT3 */
	SDC3_COMMON_CONFIG(40),		/* DAT2 */
	{
		/* DAT1 */
		.gpio      = 41,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_data_1_suspend_cfg,
		},
	},
	SDC3_COMMON_CONFIG(42),		/* DAT0 */
	SDC3_COMMON_CONFIG(43),		/* cmd */
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
#if defined(CONFIG_MACH_HESTIALTE_EUR) || defined(CONFIG_MACH_HESTIALTE_ATT)

static struct msm_gpiomux_config msm_chg_configs[] = {
        IN_NP_2MA_CFG(14),  /* FUEL_CHG_SDA */
        IN_NP_2MA_CFG(51),  /* CHG_DET */
        IN_NP_2MA_CFG(54),  /* TA_nCHG */
        IN_NP_2MA_CFG(66),  /* BATT_ALARM */
        IN_NP_2MA_CFG(119), /* INOK */
};

#endif

extern int system_rev;

/*NC GPIOs configuration*/
static struct msm_gpiomux_config nc_gpio_cfgs[] __initdata = {
	NC_GPIO_CONFIG(75),
	NC_GPIO_CONFIG(76),
	NC_GPIO_CONFIG(77),
	NC_GPIO_CONFIG(78),
	NC_GPIO_CONFIG(79),
	NC_GPIO_CONFIG(80),
	NC_GPIO_CONFIG(81),
	NC_GPIO_CONFIG(82),
	NC_GPIO_CONFIG(83),
	NC_GPIO_CONFIG(84),
	NC_GPIO_CONFIG(85),
	NC_GPIO_CONFIG(86),
	NC_GPIO_CONFIG(88),
	NC_GPIO_CONFIG(90),
	NC_GPIO_CONFIG(91),
	NC_GPIO_CONFIG(92),
	NC_GPIO_CONFIG(93),
	NC_GPIO_CONFIG(94),
	NC_GPIO_CONFIG(97),
	NC_GPIO_CONFIG(98),
	NC_GPIO_CONFIG(110),
	NC_GPIO_CONFIG(111),
	NC_GPIO_CONFIG(113),
};

static struct msm_gpiomux_config hw_chkbits_cfg[] __initdata = {
	HW_CHK_BIT_CFG(24),
	HW_CHK_BIT_CFG(67),
	HW_CHK_BIT_CFG(116),
	HW_CHK_BIT_CFG(117),
};

extern int poweroff_charging;

void __init msm8226_init_gpiomux(void)
{
	int rc;

	rc = msm_gpiomux_init_dt();
	if (rc) {
		pr_err("%s failed %d\n", __func__, rc);
		return;
	}

	/* Battery charging and BMS GPIO */
#if defined(CONFIG_MACH_HESTIALTE_EUR) || defined(CONFIG_MACH_HESTIALTE_ATT)
	msm_gpiomux_install(msm_chg_configs, ARRAY_SIZE(msm_chg_configs));
#endif

	msm_gpiomux_install(msm_hrm_configs,
		ARRAY_SIZE(msm_hrm_configs));

	msm_gpiomux_install(msm_sensorhub_configs,
		ARRAY_SIZE(msm_sensorhub_configs));

	msm_gpiomux_install(msm_nfc_configs,
		ARRAY_SIZE(msm_nfc_configs));

	msm_gpiomux_install(msm_bgm_configs,
		ARRAY_SIZE(msm_bgm_configs));

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	msm_gpiomux_install(msm_eth_configs, ARRAY_SIZE(msm_eth_configs));
#endif
	msm_gpiomux_install(msm_keypad_configs,
			ARRAY_SIZE(msm_keypad_configs));

	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_blsp_configs,
			ARRAY_SIZE(msm_skuf_blsp_configs));
	else {
		if(system_rev <6) {
			msm_blsp_configs[6].gpio=22;
			msm_blsp_configs[7].gpio=23;
		}
		msm_gpiomux_install(msm_blsp_configs,
			ARRAY_SIZE(msm_blsp_configs));
	}

	msm_gpiomux_install(wcnss_5wire_interface,
				ARRAY_SIZE(wcnss_5wire_interface));

	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_goodix_configs,
				ARRAY_SIZE(msm_skuf_goodix_configs));
	if(system_rev <6) {
		msm_synaptics_configs[0].gpio=55;
		msm_gpiomux_install(msm_synaptics_configs,
				ARRAY_SIZE(msm_synaptics_configs));
	}

	if (of_board_is_skuf())
		msm_gpiomux_install(msm_skuf_nfc_configs,
				ARRAY_SIZE(msm_skuf_nfc_configs));

	msm_gpiomux_install_nowrite(msm_lcd_configs,
			ARRAY_SIZE(msm_lcd_configs));

	msm_gpiomux_install(msm_sensor_configs, ARRAY_SIZE(msm_sensor_configs));

	if (of_board_is_skuf())
		msm_gpiomux_install(msm_sensor_configs_skuf_plus,
			ARRAY_SIZE(msm_sensor_configs_skuf_plus));

#if defined(CONFIG_SENSORS_VFS61XX)
	msm_gpiomux_install(msm_fingerprint_configs, ARRAY_SIZE(msm_fingerprint_configs));
#endif

	msm_gpiomux_install(msm_auxpcm_configs,
			ARRAY_SIZE(msm_auxpcm_configs));

	if (of_board_is_cdp() || of_board_is_mtp() || of_board_is_xpm())
		msm_gpiomux_install(usb_otg_sw_configs,
					ARRAY_SIZE(usb_otg_sw_configs));

	/*
	 * gpio mux settings for the NC GPIOs
	 */
	msm_gpiomux_install(nc_gpio_cfgs,
			ARRAY_SIZE(nc_gpio_cfgs));

	msm_gpiomux_install(hw_chkbits_cfg, ARRAY_SIZE(hw_chkbits_cfg));
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
#ifdef CONFIG_SND_SOC_ES705
	msm_gpiomux_install(es705_config,	ARRAY_SIZE(es705_config));
#endif /* CONFIG_SND_SOC_ES705 */
#if defined(CONFIG_SAMSUNG_JACK)
	msm_gpiomux_install(msm_earjack_gpio_configs, ARRAY_SIZE(msm_earjack_gpio_configs));
#endif
	if(!poweroff_charging)
		msm_gpiomux_install(wcdcodec_reset_cfg, ARRAY_SIZE(wcdcodec_reset_cfg));
#ifdef CONFIG_SND_SOC_MAX98504
		msm_gpiomux_install(msm8226_tertiary_mi2s_configs,ARRAY_SIZE(msm8226_tertiary_mi2s_configs));
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
