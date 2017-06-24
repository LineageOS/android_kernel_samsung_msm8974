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
 *01  MEGA23G LTE	nc.chaudhary	Intial file creation			27-Jan-2014
 *	3G
 *02  MEGA23G-All	nc.chaudhary	Added the gpiomux settings for		30-Jan-2014
 *	variants			UART for LCIA Test failure
 *02  MEGA23G-ATT	nc.chaudhary	Added the gpiomux settings for		12-Feb-2014
 *    Kmini-ATT				NC GPIOs as per new schematics
 ******************************************************************************************
 */

#include <linux/init.h>
#include <linux/ioport.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>



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
	{
		.func = GPIOMUX_FUNC_GPIO,  /* IN-PD */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
};

static struct gpiomux_setting gpio_i2c_config_tsp = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting tkey_led_gpio = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

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
	{
		.gpio = 5,
		.settings = {
			[GPIOMUX_ACTIVE]    = &muic_active_config[0],
			[GPIOMUX_SUSPENDED] = &muic_suspend_config[0],
		},
	},
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

static struct gpiomux_setting sensor_gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting accel_irq_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting prox_irq_config = {
    .func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
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

static struct gpiomux_setting lcd_gpio_i2c_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
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
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting lcd_pwr_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting lcd_pwr_act_cfg2 = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting lcd_pwr_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting lcd_esd_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm_lcd_configs[] __initdata = {
	{
		.gpio = 0, 	/* LED_BACKLIGHT_SDA_1.8V */
		.settings = {
			[GPIOMUX_ACTIVE]	= &lcd_gpio_i2c_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_gpio_i2c_cfg,
		},
	},
	{
		.gpio = 1, 	/* LED_BACKLIGHT_SCL_1.8V */
		.settings = {
			[GPIOMUX_ACTIVE]	= &lcd_gpio_i2c_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_gpio_i2c_cfg,
		},
	},
	{
		.gpio = 25,		/* LCD_RESET_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_rst_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_rst_sus_cfg,
		},
	},
	{
		.gpio = 16,		/* LCD_ESD_DETECT */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_esd_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_esd_cfg,
		},
	},
	{
		.gpio = 66, 	/* LCD_ON */
		.settings = {
			[GPIOMUX_ACTIVE]	= &lcd_pwr_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_pwr_sus_cfg,
		},
	},
	{
		.gpio = 63, 	/* LCD_BLIC_ON */
		.settings = {
			[GPIOMUX_ACTIVE]	= &lcd_pwr_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_pwr_sus_cfg,
		},
	},
	{
		.gpio = 56, 	/* PANEL_ENP */
		.settings = {
			[GPIOMUX_ACTIVE]	= &lcd_pwr_act_cfg2,
			[GPIOMUX_SUSPENDED] = &lcd_pwr_sus_cfg,
		},
	},
	{
		.gpio = 31,		/* PANEL_ENN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_pwr_act_cfg2,
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
#if defined(CONFIG_TDMB)
static struct gpiomux_setting gpio_spi6_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
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
		.gpio      = 18,		/* BLSP1 QUP5 I2C_SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config_tsp,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_tsp,
		},
	},
	{
		.gpio      = 19,		/* BLSP1 QUP5 I2C_SCL */
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
#if defined(CONFIG_NFC_PN547)
	{
		.gpio	   = 14, 	/* BLSP1 QUP2 I2C_SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &sensor_gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &sensor_gpio_i2c_config,
		},
	},
	{
		.gpio	   = 15, 	/* BLSP1 QUP2 I2C_SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &sensor_gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &sensor_gpio_i2c_config,
		},
	},
#endif
#if defined(CONFIG_TDMB)
	{
		.gpio	   = 20,		/* BLSP1 QUP6 TDMB_SPI_MOSI */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi6_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio	   = 21,		/* BLSP1 QUP6 TDMB_SPI_MISO */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi6_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio	   = 22,		/* BLSP1 QUP6 TDMB_SPI_CS_N */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi6_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio	   = 23,		/* BLSP1 QUP6 TDMB_SPI_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi6_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
#endif
};

static struct gpiomux_setting gpio_nc_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
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
	{
		.gpio      = 4,		/* NC */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_nc_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_skuf_goodix_configs[] __initdata = {
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

#if defined(CONFIG_NFC_PN547)  // CONFIG_SEC_LENTIS_PROJECT
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

static struct gpiomux_setting nfc_en_cfg = {
    .func = GPIOMUX_FUNC_GPIO,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_NONE,
    .dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config msm_nfc_configs[] __initdata = {
    {
        .gpio      = 107,		/* NFC IRQ */
        .settings = {
        [GPIOMUX_ACTIVE] = &nfc_irq_cfg,
        [GPIOMUX_SUSPENDED] = &nfc_irq_cfg,
        },
    },
    {
        .gpio	= 52,		/* NFC FIRMWARE */
        .settings = {
        [GPIOMUX_ACTIVE] = &nfc_firmware_cfg,
        [GPIOMUX_SUSPENDED] = &nfc_firmware_cfg,
        },
    },
    {
        .gpio	= 104,		/* NFC ENABLE */
        .settings = {
        [GPIOMUX_ACTIVE] = &nfc_en_cfg,
        [GPIOMUX_SUSPENDED] = &nfc_en_cfg,
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

static struct msm_gpiomux_config msm_nativesensors_configs[] __initdata = {
	{
		.gpio      = 65,		/* PROXY IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &prox_irq_config,
			[GPIOMUX_SUSPENDED] = &prox_irq_config,
		},
	},
	{
		.gpio	   = 64,		/* ACCEL IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &accel_irq_config,
			[GPIOMUX_SUSPENDED] = &accel_irq_config,
		},
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
		.func = GPIOMUX_FUNC_GPIO, /*active 0*/ /* 6 */
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
			[GPIOMUX_ACTIVE]    = &cam_settings[6],
			[GPIOMUX_SUSPENDED] = &cam_settings[6],
		},
	},
	{
		.gpio = 28, /* CAM2_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[6],
			[GPIOMUX_SUSPENDED] = &cam_settings[6],
		},
	},
	{
		.gpio = 112, /* CAM_ANALOG_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[6],
			[GPIOMUX_SUSPENDED] = &cam_settings[6],
		},
	},
	{
		.gpio = 116, /* CAM_VT_STBY */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[6],
			[GPIOMUX_SUSPENDED] = &cam_settings[6],
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
		.gpio = 35, /* EAR_SWITCH */
		.settings = {
			[GPIOMUX_ACTIVE]    = &earjack_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &earjack_gpio_suspend_cfg,
		},
	},
		{
		.gpio = 69, /* EAR_DET */
		.settings = {
			[GPIOMUX_ACTIVE]    = &earjack_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &earjack_gpio_suspend_cfg,
		},
	}
};
#endif

#ifdef CONFIG_SND_SOC_MAX98505


static struct gpiomux_setting  gpio_i2c_max98504_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm8226_max98504_i2c_configs[] __initdata = {
	{
		.gpio	= 10,		/* max98505  sda */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_max98504_config,
			[GPIOMUX_ACTIVE] = &gpio_i2c_max98504_config,
		},
	},
	{
		.gpio	= 11,         /* max98505  scl */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_max98504_config,
			[GPIOMUX_ACTIVE] = &gpio_i2c_max98504_config,
		},
	},
};

#endif /* CONFIG_SND_SOC_MAX98505 */

#ifdef CONFIG_LEDS_KTD2026
static struct gpiomux_setting  gpio_i2c_ktd2026_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm_ktd2026_i2c_configs[] __initdata = {
	{
		.gpio = 10, /* ktd2026  sda */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_ktd2026_config,
			[GPIOMUX_ACTIVE] = &gpio_i2c_ktd2026_config,
		},
	},
	{
		.gpio = 11, /* ktd2026  scl */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_ktd2026_config,
			[GPIOMUX_ACTIVE] = &gpio_i2c_ktd2026_config,
		},
	},
};
#endif

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

static struct msm_gpiomux_config tkey_led_gpio_configs[] __initdata = {
	{
		.gpio      = 110,		
		.settings = {
			[GPIOMUX_ACTIVE] = &tkey_led_gpio,
			[GPIOMUX_SUSPENDED] = &tkey_led_gpio,
		},
	},
};

/* Cover ID configurations */

static struct gpiomux_setting gpio_cover_id_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct msm_gpiomux_config msm_cover_id_configs[] __initdata = {
	{
		.gpio      = 107,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_cover_id_config,
			[GPIOMUX_SUSPENDED] = &gpio_cover_id_config,
		},
	},
};


static struct gpiomux_setting hw_chk_bit_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config hw_chk_bit_configs[] __initdata = {
	{
		.gpio	= 12,         /* HW_CHK_BIT 3 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &hw_chk_bit_config,
			[GPIOMUX_ACTIVE] = &hw_chk_bit_config,
		},
	},
#if defined(CONFIG_MACH_MEGA2LTE_KTT)
	{
		.gpio	= 50,		  /* HW_CHK_BIT 2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &hw_chk_bit_config,
			[GPIOMUX_ACTIVE] = &hw_chk_bit_config,
		},
	},
	{
		.gpio	= 51,		  /* HW_CHK_BIT 1 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &hw_chk_bit_config,
			[GPIOMUX_ACTIVE] = &hw_chk_bit_config,
		},
	},
#else
	{
		.gpio	= 20,         /* HW_CHK_BIT 2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &hw_chk_bit_config,
			[GPIOMUX_ACTIVE] = &hw_chk_bit_config,
		},
	},
	{
		.gpio	= 23,         /* HW_CHK_BIT 1 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &hw_chk_bit_config,
			[GPIOMUX_ACTIVE] = &hw_chk_bit_config,
		},
	},
#endif
	{
		.gpio	= 24,         /* HW_CHK_BIT 0 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &hw_chk_bit_config,
			[GPIOMUX_ACTIVE] = &hw_chk_bit_config,
		},
	},
};

static struct gpiomux_setting tsp_id_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

#if defined(CONFIG_MACH_MEGA2LTE_KTT)
static struct msm_gpiomux_config tsp_id_configs[] __initdata = {
	{
		.gpio      = 72,
		.settings = {
			[GPIOMUX_ACTIVE] = &tsp_id_config,
			[GPIOMUX_SUSPENDED] = &tsp_id_config,
		},
	},
};
#else
static struct msm_gpiomux_config tsp_id_configs[] __initdata = {
	{
		.gpio      = 22,
		.settings = {
			[GPIOMUX_ACTIVE] = &tsp_id_config,
			[GPIOMUX_SUSPENDED] = &tsp_id_config,
		},
	},
};
#endif
extern int system_rev;

/*NC GPIOs configuration*/
static struct msm_gpiomux_config gpio_nc_configs[] __initdata = {
#if !defined(CONFIG_NFC_PN547)
	NC_GPIO_CONFIG(14),
	NC_GPIO_CONFIG(15),
#endif
	NC_GPIO_CONFIG(49),
	NC_GPIO_CONFIG(50),
	NC_GPIO_CONFIG(51),
#if !defined(CONFIG_NFC_PN547)
	NC_GPIO_CONFIG(52),
#endif
	NC_GPIO_CONFIG(62),
	NC_GPIO_CONFIG(72),
	NC_GPIO_CONFIG(73),
	NC_GPIO_CONFIG(75),
	NC_GPIO_CONFIG(88),
	NC_GPIO_CONFIG(89),
	NC_GPIO_CONFIG(90),
	NC_GPIO_CONFIG(93),
	NC_GPIO_CONFIG(94),
	NC_GPIO_CONFIG(97),
	NC_GPIO_CONFIG(98),
	NC_GPIO_CONFIG(99),
	NC_GPIO_CONFIG(100),
	NC_GPIO_CONFIG(103),
#if !defined(CONFIG_NFC_PN547)
	NC_GPIO_CONFIG(104),
	NC_GPIO_CONFIG(107),
#endif
	NC_GPIO_CONFIG(113),
	NC_GPIO_CONFIG(114),
	NC_GPIO_CONFIG(115),
};

void __init msm8226_init_gpiomux(void)
{
	int rc;

	rc = msm_gpiomux_init_dt();
	if (rc) {
		pr_err("%s failed %d\n", __func__, rc);
		return;
	}

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
	msm_gpiomux_install(msm_nativesensors_configs,ARRAY_SIZE(msm_nativesensors_configs));

	msm_gpiomux_install(muic_gpio_config, ARRAY_SIZE(muic_gpio_config));

	msm_gpiomux_install(tkey_led_gpio_configs, ARRAY_SIZE(tkey_led_gpio_configs));

	/* Cover ID */
	msm_gpiomux_install(msm_cover_id_configs,
		ARRAY_SIZE(msm_cover_id_configs));

	msm_gpiomux_install(hw_chk_bit_configs, ARRAY_SIZE(hw_chk_bit_configs));

	msm_gpiomux_install(tsp_id_configs, ARRAY_SIZE(tsp_id_configs));

#if defined(CONFIG_NFC_PN547)
	msm_gpiomux_install(msm_nfc_configs, ARRAY_SIZE(msm_nfc_configs));
#endif

	/*
	 * gpio mux settings for the NC GPIOs	
	 */
	msm_gpiomux_install(gpio_nc_configs,
			ARRAY_SIZE(gpio_nc_configs));

#if defined(CONFIG_SAMSUNG_JACK)
	msm_gpiomux_install(msm_earjack_gpio_configs, ARRAY_SIZE(msm_earjack_gpio_configs));
#endif
#ifdef CONFIG_SND_SOC_MAX98505
	msm_gpiomux_install(msm8226_max98504_i2c_configs,ARRAY_SIZE(msm8226_max98504_i2c_configs));
#endif
#ifdef CONFIG_LEDS_KTD2026
	msm_gpiomux_install(msm_ktd2026_i2c_configs,ARRAY_SIZE(msm_ktd2026_i2c_configs));
#endif
}
