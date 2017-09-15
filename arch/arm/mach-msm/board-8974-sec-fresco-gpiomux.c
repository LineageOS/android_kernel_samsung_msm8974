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

#include <linux/init.h>
#include <linux/ioport.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>

#define KS8851_IRQ_GPIO -1 // 94 // 94 is es325 pin

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
		.dir = GPIOMUX_IN,
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

static struct gpiomux_setting ap2mdm_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mdm2ap_status_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};
#if 0   // MCU Pin
static struct gpiomux_setting mdm2ap_errfatal_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};
#endif

static struct gpiomux_setting mdm2ap_pblrdy = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};


static struct gpiomux_setting ap2mdm_soft_reset_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting ap2mdm_wakeup = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config mdm_configs[] __initdata = {
	/* AP2MDM_STATUS */
	{
		.gpio = 105,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* MDM2AP_STATUS */
	{
		.gpio = 46,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_status_cfg,
		}
	},
#if 0  // MCU Pin
	/* MDM2AP_ERRFATAL */
	{
		.gpio = 82,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_errfatal_cfg,
		}
	},
#endif
	/* AP2MDM_ERRFATAL */
	{
		.gpio = 106,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* AP2MDM_SOFT_RESET, aka AP2MDM_PON_RESET_N */
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_soft_reset_cfg,
		}
	},
	/* AP2MDM_WAKEUP */
	{
		.gpio = 104,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_wakeup,
		}
	},
	/* MDM2AP_PBL_READY*/
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_pblrdy,
		}
	},
};

#ifdef CONFIG_W1_SLAVE_DS28EL15
static struct gpiomux_setting gpio_cover_id_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct msm_gpiomux_config msm8974_cover_id_config[] __initdata = {
	{
		.gpio = 69,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_cover_id_config,
			[GPIOMUX_SUSPENDED] = &gpio_cover_id_config,
		},
	},
};
#endif

static struct gpiomux_setting gpio_uart_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting gpio_uart_rx_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting slimbus = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

#if defined(CONFIG_SENSORS_SSP)
static struct gpiomux_setting gpio_spi11_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_spi11_config2 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_spi11_sus_config[] = {
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
#endif

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct gpiomux_setting gpio_eth_config = {
	.pull = GPIOMUX_PULL_UP,
	.drv = GPIOMUX_DRV_2MA,
	.func = GPIOMUX_FUNC_GPIO,
};

static struct gpiomux_setting blsp7_i2c_config = {
    .func = GPIOMUX_FUNC_4,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_spi_cs2_config = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};
static struct gpiomux_setting gpio_spi_susp_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
static struct gpiomux_setting gpio_spi_cs1_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};
static struct gpiomux_setting gpio_spi_cs3_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
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

#if 0 // MCU pin
static struct gpiomux_setting gpio_epm_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};
#endif
static struct gpiomux_setting gpio_epm_marker_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};
#if 0
static struct gpiomux_setting wcnss_5wire_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};
//DEV/Platform/System/Combination/MSM8974/k
static struct gpiomux_setting wcnss_5wire_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif
static struct gpiomux_setting ath_gpio_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting ath_gpio_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	/*
	 * Please keep I2C GPIOs drive-strength at minimum (2ma). It is a
	 * workaround for HW issue of glitches caused by rapid GPIO current-
	 * change.
	 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_i2c_config_9 = {
	.func = GPIOMUX_FUNC_4,
	/*
	* Please keep I2C GPIOs drive-strength at minimum (2ma). It is a
	* workaround for HW issue of glitches caused by rapid GPIO current-
	* change.
	*/
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_i2c_config_4 = {
	.func = GPIOMUX_FUNC_4,
	/*
	* Please keep I2C GPIOs drive-strength at minimum (2ma). It is a
	* workaround for HW issue of glitches caused by rapid GPIO current-
	* change.
	*/
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting gpio_i2c_config_5 = {
	.func = GPIOMUX_FUNC_5,
	/*
	* Please keep I2C GPIOs drive-strength at minimum (2ma). It is a
	* workaround for HW issue of glitches caused by rapid GPIO current-
	* change.
	*/
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

#if defined(CONFIG_MACH_H3GDUOS)
static struct gpiomux_setting lcd_en_act_cfg = {
	.func = GPIOMUX_FUNC_2, //GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting lcd_en_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO, 
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

#else
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
};

#endif

static struct gpiomux_setting taiko_reset = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting taiko_int = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting es325_reset = {
	.pull = GPIOMUX_PULL_NONE,
	.drv = GPIOMUX_DRV_2MA,
	.func = GPIOMUX_FUNC_GPIO,
	.dir = GPIOMUX_OUT_HIGH,
};

#if defined(CONFIG_SEC_NFC_I2C)
static struct gpiomux_setting nfc_gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
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
#elif defined(CONFIG_NFC_PN547)
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
#endif

#if 0//!defined(CONFIG_SENSORS_SSP)
static struct gpiomux_setting hap_lvl_shft_suspended_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hap_lvl_shft_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};
static struct msm_gpiomux_config hap_lvl_shft_config[] __initdata = {
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_SUSPENDED] = &hap_lvl_shft_suspended_config,
			[GPIOMUX_ACTIVE] = &hap_lvl_shft_active_config,
		},
	},
};
#endif

#if defined(CONFIG_SEC_NFC_I2C) || defined(CONFIG_NFC_PN547)
static struct msm_gpiomux_config msm_nfc_configs[] __initdata = {
	{
		.gpio      = 59,		/* NFC IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_irq_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_irq_cfg,
		},
	},
	{
		.gpio		= 63,		/* NFC FIRMWARE */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_firmware_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_firmware_cfg,
		},
	},
};
#endif

#if 0 // MCU pin
static struct gpiomux_setting hsic_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hsic_hub_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};
#endif

#if 0 // MCU pin
static struct msm_gpiomux_config msm_hsic_hub_configs[] = {
	{
		.gpio = 50,               /* HSIC_HUB_INT_N */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_hub_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
};
#endif

#ifdef CONFIG_VIDEO_MHL_V2
static struct gpiomux_setting mhl_hpd_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mhl_hpd_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif

static struct gpiomux_setting hdmi_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hdmi_ddc_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hdmi_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting hdmi_active_2_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm_hdmi_configs[] __initdata = {
#ifndef CONFIG_VIDEO_MHL_V2
	{
		.gpio = 31,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
#endif
	{
		.gpio = 34,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
};

static struct msm_gpiomux_config hw_rev_configs[] __initdata = {
	{
		.gpio      = 16,		/* HW_REV(0) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio      = 14,		/* HW_REV(1) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio      = 13,		/* HW_REV(2) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio      = 90,		/* HW_REV(3) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
};

static struct msm_gpiomux_config gpio_nc_configs[] __initdata = {
	GPIOMUX_SET_NC(91),		/* TSP_ID */
	GPIOMUX_SET_NC(101),		/* 2Touch_ID */
#if !defined(CONFIG_SENSORS_VFS61XX)	// from H.HW Rev08, fingerprint sensor removed.
	GPIOMUX_SET_NC(23),
	GPIOMUX_SET_NC(24),
	GPIOMUX_SET_NC(25),
	GPIOMUX_SET_NC(26),
	GPIOMUX_SET_NC(28),
	GPIOMUX_SET_NC(64),
	GPIOMUX_SET_NC(130),
#endif
#if defined(CONFIG_MACH_HLTETMO)
	GPIOMUX_SET_NC(142),
	GPIOMUX_SET_NC(143),
#endif
#if defined(CONFIG_SEC_LOCALE_KOR)
	GPIOMUX_SET_NC(103),
#endif
};

static struct msm_gpiomux_config msm_hdmi_ddc_configs[] __initdata = {
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_ddc_suspend_cfg,
		},
	},
	{
		.gpio = 33,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_ddc_suspend_cfg,
		},
	},
};

#ifdef CONFIG_VIDEO_MHL_V2
static struct msm_gpiomux_config mhl_configs[] __initdata = {
	{
		.gpio = 31,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mhl_hpd_active_cfg,
			[GPIOMUX_SUSPENDED] = &mhl_hpd_suspend_cfg,
		},
	},
};
#endif

#ifdef CONFIG_LEDS_AN30259A
	static struct gpiomux_setting an30259a_i2c_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config an30259a_led_config[] __initdata = {
	{
		.gpio	= 11,		/* SVC_LED_SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &an30259a_i2c_config,
		},
	},
	{
		.gpio	= 10,		/* SVC_LED_SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &an30259a_i2c_config,
		},
	},
};
#endif

#if !defined(CONFIG_BT_BCM4335) && !defined(CONFIG_BT_BCM4339)
static struct gpiomux_setting gpio_uart7_active_cfg = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_uart7_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm_blsp2_uart7_configs[] __initdata = {
	{
		.gpio	= 41,	/* BLSP2 UART7 TX */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_uart7_active_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_uart7_suspend_cfg,
		},
	},
	{
		.gpio	= 42,	/* BLSP2 UART7 RX */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_uart7_active_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_uart7_suspend_cfg,
		},
	},
	{
		.gpio	= 44,	/* BLSP2 UART7 RFR */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_uart7_active_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_uart7_suspend_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_rumi_blsp_configs[] __initdata = {
	{
		.gpio      = 45,	/* BLSP2 UART8 TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 46,	/* BLSP2 UART8 RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
};
#endif
static struct msm_gpiomux_config msm_lcd_configs[] __initdata = {
	{
		.gpio = 58,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_en_sus_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_epm_configs[] __initdata = {
#if 0
	{
		.gpio      = 81,		/* EPM enable */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_epm_config,
		},
	},
#endif
#if 0 // es325 pin
	{
		.gpio      = 85,		/* EPM MARKER2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_epm_marker_config,
		},
	},
#endif	
	{
		.gpio      = 96,		/* EPM MARKER1 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_epm_marker_config,
		},
	},
};

static struct msm_gpiomux_config msm_blsp_configs[] __initdata = {
#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	{
		.gpio      = 0,		/* BLSP1 QUP SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs3_config,
		},
	},
	{
		.gpio      = 1,		/* BLSP1 QUP SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs3_config,
		},
	},
	{
		.gpio	  = 2,		/* BLSP1 QUP SPI_DATA_SSN */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs3_config,
		},
	},
	{
		.gpio      = 3,		/* BLSP1 QUP SPI_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs3_config,
		},
	},
	{
		.gpio      = 9,		/* BLSP1 QUP SPI_CS2A_N */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_cs2_config,
			[GPIOMUX_SUSPENDED] = &gpio_spi_susp_config,
		},
	},
	{
		.gpio      = 8,		/* BLSP1 QUP SPI_CS1_N */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_cs1_config,
			[GPIOMUX_SUSPENDED] = &gpio_spi_susp_config,
		},
	},
#endif
	{
		.gpio      = 6,		/* BLSP1 QUP2 I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 7,		/* BLSP1 QUP2 I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
		},
	},
 	{
		.gpio      = 25,		/* BLSP5 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_i2c_config_4,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_4,
		},
	},
	{
		.gpio      = 26,		/* BLSP5 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_i2c_config_5,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_5,
		},
	},
#if !defined(CONFIG_INPUT_WACOM_HL) 
	{
		.gpio      = 29,                /* BLSP6 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 30,                /* BLSP6 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
#endif
	{
	    .gpio      = 43,		/* BLSP7 QUP2 I2C_DAT */
	    .settings = {
		[GPIOMUX_SUSPENDED] = &blsp7_i2c_config,
	    },
	},
	{
	    .gpio      = 44,		/* BLSP7 QUP2 I2C_CLK */
	    .settings = {
		[GPIOMUX_SUSPENDED] = &blsp7_i2c_config,
	    },
	},
	{
		.gpio      = 51,                /* BLSP9 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_9,
		},
	},
	{
		.gpio      = 52,                /* BLSP9 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_9,
		},
	},
#if defined(CONFIG_SENSORS)
	{
		.gpio      = 83,		/* BLSP11 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 84,		/* BLSP11 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
#endif
#if defined(CONFIG_SENSORS_SSP)
	{
		.gpio      = 81,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi11_config2,
			[GPIOMUX_SUSPENDED] = &gpio_spi11_sus_config[2],
		},
	},
	{
		.gpio      = 82,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi11_config2,
			[GPIOMUX_SUSPENDED] = &gpio_spi11_sus_config[2],
		},
	},
	{
		.gpio      = 83,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi11_config,
			[GPIOMUX_SUSPENDED] = &gpio_spi11_sus_config[1],
		},
	},
	{
		.gpio      = 84,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi11_config,
			[GPIOMUX_SUSPENDED] = &gpio_spi11_sus_config[0],
		},
	},
#endif
	{
		.gpio      = 4,			/* BLSP2 UART TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 5,			/* BLSP2 UART RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_rx_config,
		},
	},
#if defined(CONFIG_SEC_NFC_I2C) || defined(CONFIG_NFC_PN547)
	{
		.gpio      = 55, /* BLSP10 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &nfc_gpio_i2c_config,
		},
	},
	{
		.gpio      = 56, /* BLSP10 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &nfc_gpio_i2c_config,
		},
	},
#else
	{
		.gpio      = 56,		/* BLSP2 QUP4 I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 55,		/* BLSP2 QUP4 I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
#endif
};

static struct msm_gpiomux_config msm8974_slimbus_config[] __initdata = {
	{
		.gpio	= 70,		/* slimbus clk */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
	{
		.gpio	= 71,		/* slimbus data */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
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
		.func = GPIOMUX_FUNC_1,	/*active 1*/ /* 5 */
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_1,	/*active 1*/ /* 6 */
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_NONE,
	},
};

static struct msm_gpiomux_config msm_sensor_configs[] __initdata = {
	{
		.gpio = 15, /* CAM_MCLK0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
#if 0	
	{
		.gpio = 16, /* CAM_MCLK1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
#endif
	{
		.gpio = 17, /* CAM_MCLK2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 18, /* WEBCAM1_RESET_N / CAM_MCLK3 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 19, /* CCI_I2C_SDA0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio = 20, /* CCI_I2C_SCL0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio = 21, /* CCI_I2C_SDA1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio = 22, /* CCI_I2C_SCL1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
#if 0
	{
		.gpio = 23, /* FLASH_LED_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 24, /* FLASH_LED_NOW */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
#if 0
	{
		.gpio = 25, /* WEBCAM2_RESET_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 26, /* CAM_IRQ */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
#endif
	{
		.gpio = 27, /* OIS_SYNC */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#if 0
	{
		.gpio = 28, /* WEBCAM1_STANDBY */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
#if 0
	{
		.gpio = 89, /* CAM1_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
#if 0
	{
		.gpio = 90, /* CAM1_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
#if 0//!defined(CONFIG_SENSORS_SSP)
	{
		.gpio = 91, /* CAM2_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
	{
		.gpio = 92, /* VT_CAM_ID */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
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
	.gpio = 62,
	.settings = {
		[GPIOMUX_ACTIVE]    = &sd_card_det_active_config,
		[GPIOMUX_SUSPENDED] = &sd_card_det_sleep_config,
	},
};
#ifdef CONFIG_MACH_H3GDUOS
 static struct gpiomux_setting cam_mclk2_suspend_settings = {
	.func = GPIOMUX_FUNC_GPIO, 
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
#endif
static struct msm_gpiomux_config msm_sensor_configs_dragonboard[] __initdata = {
	{
		.gpio = 15, /* CAM_MCLK0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
	{
		.gpio = 16, /* CAM_MCLK1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
#ifdef CONFIG_MACH_H3GDUOS
	{
		.gpio = 17, /* CAM_MCLK2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_mclk2_suspend_settings,
		},
	},
#else
	{
		.gpio = 17, /* CAM_MCLK2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
#endif
	{
		.gpio = 18, /* WEBCAM1_RESET_N / CAM_MCLK3 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 19, /* CCI_I2C_SDA0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 20, /* CCI_I2C_SCL0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 21, /* CCI_I2C_SDA1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 22, /* CCI_I2C_SCL1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 23, /* FLASH_LED_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 24, /* FLASH_LED_NOW */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 25, /* WEBCAM2_RESET_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 26, /* CAM_IRQ */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
	{
		.gpio = 27, /* OIS_SYNC */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 28, /* WEBCAM1_STANDBY */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#if 0 // MCU pin
	{
		.gpio = 89, /* CAM1_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
	{
		.gpio = 90, /* CAM1_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#if 0//!defined(CONFIG_SENSORS_SSP)
	{
		.gpio = 91, /* CAM2_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
#if 0 // es325 pin	
	{
		.gpio = 94, /* CAM2_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif	
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

/* Primary AUXPCM port sharing GPIO lines with Primary MI2S */
static struct msm_gpiomux_config msm8974_pri_pri_auxpcm_configs[] __initdata = {
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
	{
		.gpio = 67,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 68,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
};

static struct gpiomux_setting gpio_audio_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_audio_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_audio_pullnone_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
static struct msm_gpiomux_config msm8974_audio_configs[] __initdata = {
	{
		.gpio      = 76,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_audio_active_config,
			[GPIOMUX_SUSPENDED] = &gpio_audio_suspend_config,
		},
	},
	{
		.gpio      = 77,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_audio_pullnone_config,
			[GPIOMUX_SUSPENDED] = &gpio_audio_pullnone_config,
		},
	},
};

/* Primary AUXPCM port sharing GPIO lines with Tertiary MI2S */
static struct msm_gpiomux_config msm8974_pri_ter_auxpcm_configs[] __initdata = {
#if 0 // MCU pin
	{
		.gpio = 74,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
#endif
	{
		.gpio = 75,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 76,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
};

#if 0 /* Not used at Mondrian */
static struct msm_gpiomux_config msm8974_sec_auxpcm_configs[] __initdata = {
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 81,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 82,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
};
#endif

#if 0
static struct msm_gpiomux_config wcnss_5wire_interface[] = {
	{
		.gpio = 36,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 37,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 38,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 39,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
};
#endif

static struct msm_gpiomux_config es325_config[] __initdata = {
	{
		.gpio	= 94,		/* ES325_RESET */
		.settings = {
			[GPIOMUX_SUSPENDED] = &es325_reset,
		},
	},
	{
		.gpio	= 76,		/* ES325_INT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
};

static struct msm_gpiomux_config ath_gpio_configs[] = {
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_ACTIVE]    = &ath_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &ath_gpio_suspend_cfg,
		},
	},
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_ACTIVE]    = &ath_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &ath_gpio_suspend_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_taiko_config[] __initdata = {
	{
		.gpio	= 78,		/* SYS_RST_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &taiko_reset,
		},
	},
	{
		.gpio	= 72,		/* CDC_INT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &taiko_int,
		},
	},
};

#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE) || defined(CONFIG_BCM4339) || defined(CONFIG_BCM4339_MODULE)

/* MSM8974 WLAN_HOST_WAKE GPIO Number */
#define GPIO_WL_HOST_WAKE 54

static struct gpiomux_setting wlan_host_wakeup_setting[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
};

static struct msm_gpiomux_config wlan_host_wakeup_configs[] __initdata = {
	{
		.gpio = GPIO_WL_HOST_WAKE,
		.settings = {
			[GPIOMUX_ACTIVE] = &wlan_host_wakeup_setting[0],
			[GPIOMUX_SUSPENDED] = &wlan_host_wakeup_setting[0],
		}
	},
};

static void msm_gpiomux_wlan_host_wakeup_install(void)
{
	msm_gpiomux_install(wlan_host_wakeup_configs,
				ARRAY_SIZE(wlan_host_wakeup_configs));
}
#endif /* defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE) || defined(CONFIG_BCM4339) || defined(CONFIG_BCM4339_MODULE) */

#ifdef CONFIG_MFD_MAX77888
static struct gpiomux_setting muic_init_cfg = {
    .func = GPIOMUX_FUNC_GPIO,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_NONE,
    .dir = GPIOMUX_IN,
};

static struct gpiomux_setting muic_sleep_cfg = {
    .func = GPIOMUX_FUNC_GPIO,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_NONE,
    .dir = GPIOMUX_IN,
};
#if 0 // Sensor hub pin
static struct msm_gpiomux_config msm8974_muic_configs[] __initdata = {
    {
        .gpio = 83, /* IF_PMIC_SCL_1.8V */
        .settings = {
            [GPIOMUX_ACTIVE] = &muic_init_cfg,
            [GPIOMUX_SUSPENDED] = &muic_sleep_cfg,
        },
    },
    {
        .gpio = 84, /* IF_PMIC_SDA_1.8V */
        .settings = {
            [GPIOMUX_ACTIVE] = &muic_init_cfg,
            [GPIOMUX_SUSPENDED] = &muic_sleep_cfg,
        },
    },
};
#endif
#endif

#if defined(CONFIG_SENSORS_SSP)
static struct gpiomux_setting ssp_setting[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_HIGH,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
};


static struct msm_gpiomux_config ssp_configs[] __initdata = {
	{
		.gpio = 50,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
	{
		.gpio = 74,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
	{
		.gpio = 89,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[0],
			[GPIOMUX_SUSPENDED] = &ssp_setting[0],
		},
	},
};
#endif

#if defined(CONFIG_SENSORS)
static struct gpiomux_setting prox_irq_config = {
    .func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
static struct gpiomux_setting mag_irq_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};
static struct gpiomux_setting accel_irq_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};
static struct msm_gpiomux_config msm_sensors_configs[] __initdata = {
	{
		.gpio      = 89,        /* PROXY IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &prox_irq_config,
			[GPIOMUX_SUSPENDED] = &prox_irq_config,
		},
	},
	{
		.gpio      = 73,        /* ACCEL IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &accel_irq_config,
			[GPIOMUX_SUSPENDED] = &accel_irq_config,
		},
	},
	{
		.gpio      = 74,        /* MAG IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &mag_irq_config,
			[GPIOMUX_SUSPENDED] = &mag_irq_config,
		},
	},
};
#endif

static struct gpiomux_setting sdc3_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdc3_cmd_data_0_3_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdc3_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdc3_clk_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config msm8974_sdc3_configs[] __initdata = {
	{
		/* DAT3 */
		.gpio      = 35,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* DAT2 */
		.gpio      = 36,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* DAT1 */
		.gpio      = 37,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* DAT0 */
		.gpio      = 38,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* CMD */
		.gpio      = 39,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_suspend_cfg,
		},
	},
	{
		/* CLK */
		.gpio      = 40,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc3_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc3_clk_suspend_cfg,
		},
	},
};

static void msm_gpiomux_sdc3_install(void)
{
	msm_gpiomux_install(msm8974_sdc3_configs,
			    ARRAY_SIZE(msm8974_sdc3_configs));
}

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct gpiomux_setting sdc4_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdc4_cmd_data_0_3_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdc4_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sdc4_data_1_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm8974_sdc4_configs[] __initdata = {
	{
		/* DAT3 */
		.gpio      = 92,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
#if 0 // es325 pin	
	{
		/* DAT2 */
		.gpio      = 94,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
#endif	
	{
		/* DAT1 */
		.gpio      = 95,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_data_1_suspend_cfg,
		},
	},
	{
		/* DAT0 */
		.gpio      = 96,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
#if 0//!defined(CONFIG_SENSORS_SSP)
	{
		/* CMD */
		.gpio      = 91,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
#endif
	{
		/* CLK */
		.gpio      = 93,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
};

static void msm_gpiomux_sdc4_install(void)
{
	msm_gpiomux_install(msm8974_sdc4_configs,
			    ARRAY_SIZE(msm8974_sdc4_configs));
}
#else
static void msm_gpiomux_sdc4_install(void) {}
#endif /* CONFIG_MMC_MSM_SDC4_SUPPORT */

#if 0 //defined(CONFIG_BT_BCM4335) || defined(CONFIG_BT_BCM4339)
static struct msm_gpiomux_config msm8974_btuart_configs[] __initdata = {
	{
		/* TXD */
		.gpio      = 45,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		/* RXD */
		.gpio      = 46,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		/* CTS */
		.gpio      = 47,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		/* RTS */
		.gpio      = 48,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
};

static void msm_gpiomux_btuart_install(void)
{
	msm_gpiomux_install(msm8974_btuart_configs,
			    ARRAY_SIZE(msm8974_btuart_configs));
}
#endif

static struct msm_gpiomux_config apq8074_dragonboard_ts_config[] __initdata = {
	{
		/* BLSP1 QUP I2C_DATA */
		.gpio      = 2,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		/* BLSP1 QUP I2C_CLK */
		.gpio      = 3,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
};

#ifdef CONFIG_USB_SWITCH_FSA9485
static struct gpiomux_setting gpio_muic_sleep_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
static struct msm_gpiomux_config vienna_muic_configs[] __initdata = {
	{
		.gpio      = 32,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_muic_sleep_config,
		},
	},
	{
		.gpio      = 33,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_muic_sleep_config,
		},
	},
};
static struct msm_gpiomux_config vienna_r10_muic_configs[] __initdata = {
	{
		.gpio      = 55,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_muic_sleep_config,
		},
	},
	{
		.gpio      = 56,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_muic_sleep_config,
		},
	},
};
#endif

#ifdef CONFIG_INPUT_WACOM_HL
static struct gpiomux_setting gpio_wacom_sleep_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_wacom_active_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_wacom_irq_sleep_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_wacom_irq_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm8974_wacom_configs[] __initdata = {
	{
		/* PEN_SDA_1.8V */
		.gpio = 29,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_wacom_sleep_config,
			[GPIOMUX_ACTIVE] = &gpio_wacom_active_config,
		},
	},
	{
		/* PEN_SCL_1.8V */
		.gpio = 30,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_wacom_sleep_config,
			[GPIOMUX_ACTIVE] = &gpio_wacom_active_config,
		},
	},
	{
		/* PEN_IRQ_1.8V */
		.gpio = 144,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_wacom_irq_sleep_config,
		},
	},
	{
		/* PEN_PDCT_1.8V */
		.gpio = 145,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_wacom_irq_sleep_config,
			[GPIOMUX_ACTIVE] = &gpio_wacom_irq_active_config,
		},
	},

};
#endif

extern unsigned int system_rev;
void __init msm_8974_init_gpiomux(void)
{
	int rc;

	rc = msm_gpiomux_init_dt();
	if (rc) {
		pr_err("%s failed %d\n", __func__, rc);
		return;
	}

	pr_err("%s:%d socinfo_get_version %x\n", __func__, __LINE__,
		socinfo_get_version());
	if (socinfo_get_version() >= 0x20000)
		msm_tlmm_misc_reg_write(TLMM_SPARE_REG, 0xf);

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	if (!(of_board_is_dragonboard() && machine_is_apq8074()))
		msm_gpiomux_install(msm_eth_configs, \
			ARRAY_SIZE(msm_eth_configs));
#endif
	msm_gpiomux_install(msm_blsp_configs, ARRAY_SIZE(msm_blsp_configs));

#ifdef CONFIG_MFD_MAX77888
#if 0 // MCU Pin
	msm_gpiomux_install(msm8974_muic_configs, ARRAY_SIZE(msm8974_muic_configs));
#endif
#endif

#ifdef CONFIG_W1_SLAVE_DS28EL15
	msm_gpiomux_install(msm8974_cover_id_config, ARRAY_SIZE(msm8974_cover_id_config));
#endif

#if !defined(CONFIG_BT_BCM4335) && !defined(CONFIG_BT_BCM4339)
	msm_gpiomux_install(msm_blsp2_uart7_configs,
			 ARRAY_SIZE(msm_blsp2_uart7_configs));
#endif
#if 0//defined(CONFIG_BT_BCM4335) || defined(CONFIG_BT_BCM4339)
	msm_gpiomux_btuart_install();
#endif

	if (of_board_is_liquid())
		msm_gpiomux_install_nowrite(ath_gpio_configs,
					ARRAY_SIZE(ath_gpio_configs));
	msm_gpiomux_install(msm8974_slimbus_config,
			ARRAY_SIZE(msm8974_slimbus_config));

	msm_gpiomux_install(hw_rev_configs, ARRAY_SIZE(hw_rev_configs));
	msm_gpiomux_install(gpio_nc_configs, ARRAY_SIZE(gpio_nc_configs));

#if 0//!defined(CONFIG_SENSORS_SSP)
		msm_gpiomux_install(hap_lvl_shft_config,
				ARRAY_SIZE(hap_lvl_shft_config));
#endif

	if (of_board_is_dragonboard() && machine_is_apq8074())
		msm_gpiomux_install(msm_sensor_configs_dragonboard, \
				ARRAY_SIZE(msm_sensor_configs_dragonboard));
	else
		msm_gpiomux_install(msm_sensor_configs, \
				ARRAY_SIZE(msm_sensor_configs));

	msm_gpiomux_install(&sd_card_det, 1);
	msm_gpiomux_sdc3_install();

	if (!(of_board_is_dragonboard() && machine_is_apq8074()))
		msm_gpiomux_sdc4_install();

	msm_gpiomux_install(msm_taiko_config, ARRAY_SIZE(msm_taiko_config));
	msm_gpiomux_install(es325_config, ARRAY_SIZE(es325_config));
	msm_gpiomux_install(msm8974_audio_configs, ARRAY_SIZE(msm8974_audio_configs));	
	msm_gpiomux_install(msm8974_pri_pri_auxpcm_configs,
				 ARRAY_SIZE(msm8974_pri_pri_auxpcm_configs));
#if 0 // MCU pin
	msm_gpiomux_install(msm_hsic_hub_configs,
				ARRAY_SIZE(msm_hsic_hub_configs));
#endif
#if defined(CONFIG_SENSORS_SSP)
	msm_gpiomux_install(ssp_configs,
				ARRAY_SIZE(ssp_configs));
#endif

#if defined(CONFIG_SENSORS)
	msm_gpiomux_install(msm_sensors_configs,
		ARRAY_SIZE(msm_sensors_configs));
#endif
	msm_gpiomux_install(msm_hdmi_configs, ARRAY_SIZE(msm_hdmi_configs));
	if (system_rev > 5)
		msm_gpiomux_install(msm_hdmi_ddc_configs,
				ARRAY_SIZE(msm_hdmi_ddc_configs));
#ifdef CONFIG_VIDEO_MHL_V2
	if(system_rev > 1)
		msm_gpiomux_install(mhl_configs, ARRAY_SIZE(mhl_configs));
#endif
#if 0
	msm_gpiomux_install(wcnss_5wire_interface,
				ARRAY_SIZE(wcnss_5wire_interface));
#endif
#if defined(CONFIG_SEC_NFC_I2C) || defined(CONFIG_NFC_PN547)
	msm_gpiomux_install(msm_nfc_configs,
	ARRAY_SIZE(msm_nfc_configs));
#endif
	if (of_board_is_liquid() ||
	    (of_board_is_dragonboard() && machine_is_apq8074()))
		msm_gpiomux_install(msm8974_pri_ter_auxpcm_configs,
				 ARRAY_SIZE(msm8974_pri_ter_auxpcm_configs));
	else
		msm_gpiomux_install(msm8974_pri_pri_auxpcm_configs,
				 ARRAY_SIZE(msm8974_pri_pri_auxpcm_configs));

#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE) || defined(CONFIG_BCM4339) || defined(CONFIG_BCM4339_MODULE)
	msm_gpiomux_wlan_host_wakeup_install();
#endif /* defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE) || defined(CONFIG_BCM4339) || defined(CONFIG_BCM4339_MODULE) */

#ifdef CONFIG_USB_SWITCH_FSA9485
	if (system_rev >= 10)
		msm_gpiomux_install(vienna_r10_muic_configs,
				ARRAY_SIZE(vienna_r10_muic_configs));
	else
		msm_gpiomux_install(vienna_muic_configs,
				ARRAY_SIZE(vienna_muic_configs));
#endif

	if (of_board_is_cdp()) {
#if 0 /* Not used at Mondrian */
		msm_gpiomux_install(msm8974_sec_auxpcm_configs,
				 ARRAY_SIZE(msm8974_sec_auxpcm_configs));
#endif
	} else if (of_board_is_liquid() || of_board_is_fluid() ||
						of_board_is_mtp())
		msm_gpiomux_install(msm_epm_configs,
				ARRAY_SIZE(msm_epm_configs));

	msm_gpiomux_install_nowrite(msm_lcd_configs,
			ARRAY_SIZE(msm_lcd_configs));

#if !defined(CONFIG_BT_BCM4335) && !defined(CONFIG_BT_BCM4339)
	if (of_board_is_rumi())
		msm_gpiomux_install(msm_rumi_blsp_configs,
				    ARRAY_SIZE(msm_rumi_blsp_configs));
#endif

#if defined(CONFIG_LEDS_AN30259A)
	msm_gpiomux_install(an30259a_led_config,
			ARRAY_SIZE(an30259a_led_config));
#endif

#ifdef CONFIG_INPUT_WACOM_HL
	msm_gpiomux_install(msm8974_wacom_configs,
		ARRAY_SIZE(msm8974_wacom_configs));
#endif

	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_MDM)
		msm_gpiomux_install(mdm_configs,
			ARRAY_SIZE(mdm_configs));

	if (of_board_is_dragonboard() && machine_is_apq8074())
		msm_gpiomux_install(apq8074_dragonboard_ts_config,
			ARRAY_SIZE(apq8074_dragonboard_ts_config));
}
