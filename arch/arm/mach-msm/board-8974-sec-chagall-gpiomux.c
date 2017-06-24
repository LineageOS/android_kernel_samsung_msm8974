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

#if !defined(CONFIG_MACH_CHAGALL_KDI)
#define KS8851_IRQ_GPIO 94
#endif

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

#ifndef CONFIG_SENSORS_AK8963C
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

#ifndef CONFIG_SENSORS_VFS61XX
static struct gpiomux_setting ap2mdm_soft_reset_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

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
#ifndef CONFIG_SENSORS_AK8963C
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
#ifndef CONFIG_SENSORS_VFS61XX
	/* AP2MDM_SOFT_RESET, aka AP2MDM_PON_RESET_N */
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_soft_reset_cfg,
		}
	},
#endif
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

static struct gpiomux_setting gpio_uart_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting gpio_uart2_config = {
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

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct gpiomux_setting gpio_eth_config = {
	.pull = GPIOMUX_PULL_UP,
	.drv = GPIOMUX_DRV_2MA,
	.func = GPIOMUX_FUNC_GPIO,
};

static struct gpiomux_setting gpio_spi_cs2_config = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gpio_spi_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
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

#if defined(CONFIG_MACH_CHAGALL_KDI)
static struct gpiomux_setting blsp7_i2c_config = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
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
	{
		.func = GPIOMUX_FUNC_GPIO,  /* IN-PD */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,  /* OUT-PD */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_OUT_LOW,
	},
	{
		.func = GPIOMUX_FUNC_GPIO, /*  OUT-PU */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_UP,
		.dir = GPIOMUX_OUT_HIGH,
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

static struct msm_gpiomux_config hw_rev_configs[] __initdata = {
	{
		.gpio      = 28,		/* HW_REV(0) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio      = 13,		/* HW_REV(1) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio	   = 14,		/* HW_REV(2) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio	   = 16,		/* HW_REV(3) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
};
static struct msm_gpiomux_config gpio_nc_configs[] __initdata = {
#ifndef CONFIG_SENSORS_VFS61XX
	GPIOMUX_SET_NC(24),
#endif
	GPIOMUX_SET_NC(9),
	GPIOMUX_SET_NC(82),
#if !defined(CONFIG_MACH_CHAGALL_KDI)
	GPIOMUX_SET_NC(44),
	GPIOMUX_SET_NC(101),
	GPIOMUX_SET_NC(102),
#endif
	GPIOMUX_SET_NC(103),
	GPIOMUX_SET_NC(116),
	GPIOMUX_SET_NC(124),
	GPIOMUX_SET_NC(130),
	GPIOMUX_SET_NC(131),
	GPIOMUX_SET_NC(135),
	GPIOMUX_SET_NC(136),
	GPIOMUX_SET_NC(137),
	GPIOMUX_SET_NC(144),
	GPIOMUX_SET_NC(145),
#if defined(CONFIG_MACH_CHAGALL_SPR)
	GPIOMUX_SET_NC(105),
	GPIOMUX_SET_NC(113),
	GPIOMUX_SET_NC(114),
	GPIOMUX_SET_NC(115),
	GPIOMUX_SET_NC(117),
	GPIOMUX_SET_NC(127),
	GPIOMUX_SET_NC(138),
	GPIOMUX_SET_NC(139),
#endif
#if defined(CONFIG_MACH_CHAGALL_VZW)
	GPIOMUX_SET_NC(106),
#endif
#if defined(CONFIG_MACH_CHAGALL_USC)
	GPIOMUX_SET_NC(138),
	GPIOMUX_SET_NC(139),
#endif
#if defined(CONFIG_MACH_CHAGALL_KDI)
	GPIOMUX_SET_NC(110),
	GPIOMUX_SET_NC(112),
	GPIOMUX_SET_NC(113),
	GPIOMUX_SET_NC(114),
	GPIOMUX_SET_NC(125),
	GPIOMUX_SET_NC(127),
	GPIOMUX_SET_NC(129),
#endif
};
static struct msm_gpiomux_config gpio_nc_rev03_configs[] __initdata = {
#if !defined(CONFIG_MACH_CHAGALL_KDI)
	GPIOMUX_SET_NC(49),
	GPIOMUX_SET_NC(74),
#endif
#if defined(CONFIG_MACH_CHAGALL_VZW)
	GPIOMUX_SET_NC(80),
#endif
	GPIOMUX_SET_NC(89),
};
static struct gpiomux_setting gpio_epm_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

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
#if 0
static struct gpiomux_setting gpio_i2c_act_config = {
	.func = GPIOMUX_FUNC_3,
	/*
	 * Please keep I2C GPIOs drive-strength at minimum (2ma). It is a
	 * workaround for HW issue of glitches caused by rapid GPIO current-
	 * change.
	 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};
#endif
#ifndef	CONFIG_VIDEO_MHL_V2
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
#endif
static struct gpiomux_setting gpio_i2c_config_4 = {
	.func = GPIOMUX_FUNC_4,
	/*
	* Please keep I2C GPIOs drive-strength at minimum (2ma). It is a
	* workaround for HW issue of glitches caused by rapid GPIO current-
	* change.
	*/
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
static struct gpiomux_setting gpio_i2c_config_5 = {
	.func = GPIOMUX_FUNC_5,
	/*
	* Please keep I2C GPIOs drive-strength at minimum (2ma). It is a
	* workaround for HW issue of glitches caused by rapid GPIO current-
	* change.
	*/
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
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
};

static struct gpiomux_setting grip_i2c_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

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

#if defined(CONFIG_SENSORS_VFS61XX)
static struct gpiomux_setting gpio_spi_btp_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_spi_btp_clk_config = {
	.func = GPIOMUX_FUNC_3,
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

static struct msm_gpiomux_config msm8974_fingerprint_configs[] __initdata = {
	{
		/* MOSI */
		.gpio = 23,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_config,
		},
	},
	{
		/* MISO */
		.gpio = 24,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_irq_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_config,
		},
	},
	{
		/* CS */
		.gpio = 25,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_config,
		},
	},
	{
		/* CLK  */
		.gpio = 26,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_clk_config,
		},
	},
#if !defined(CONFIG_MACH_CHAGALL_KDI)
	{
		/* BTP_RST_N */
		.gpio = 73,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
	{
		/* BTP_INT */
		.gpio = 94,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_irq_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_irq_config,
		},
	},
#endif
	{
		/* BTP_LDO_1.8V */
		.gpio = 427,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_rst_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
	{
		/* BTP_LDO_3.3V */
		.gpio = 434,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_rst_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
	{
		/* BTP_OCP_FLAG */
		.gpio = 430,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_irq_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_irq_config,
		},
	},
};
#endif

#if defined(CONFIG_NFC_PN547) && defined(CONFIG_SEC_LOCALE_JPN)
static struct gpiomux_setting nfc_pn547_firmware_cfg = 
{
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
#if defined(CONFIG_MACH_CHAGALL_DCM)
static struct gpiomux_setting nfc_pn547_irq_cfg = 
{
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting nfc_pn547_ven_cfg = 
{
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
#endif
static struct gpiomux_setting nfc_pn547_i2c_cfg = 
{
	.func = GPIOMUX_FUNC_4,
	/*
	* Please keep I2C GPIOs drive-strength at minimum (2ma). It is a
	* workaround for HW issue of glitches caused by rapid GPIO current-
	* change.
	*/
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config msm_nfc_pn547_configs[] __initdata = 
{
#if defined(CONFIG_MACH_CHAGALL_KDI)
	{
		.gpio		= 49,		/* NFC FIRMWARE */
		.settings = 
		{
			[GPIOMUX_ACTIVE] = &nfc_pn547_firmware_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_pn547_firmware_cfg,
		},
	},
#endif	
#if defined(CONFIG_MACH_CHAGALL_DCM)
	{
		.gpio		= 119,		/* NFC FIRMWARE */
		.settings = 
		{
			[GPIOMUX_ACTIVE] = &nfc_pn547_firmware_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_pn547_firmware_cfg,
		},
	},	
	{
		.gpio		= 107,		/* NFC IRQ */
		.settings = 
		{
			[GPIOMUX_ACTIVE] = &nfc_pn547_irq_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_pn547_irq_cfg,
		},
	},
	{
		.gpio		= 118,		/* NFC VEN */
		.settings = 
		{
			[GPIOMUX_ACTIVE] = &nfc_pn547_ven_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_pn547_ven_cfg,
		},
	},
#endif		
	{
		.gpio      = 51,                /* NFC PN547 on BLSP9 QUP I2C_DAT */
		.settings = 
		{
			[GPIOMUX_ACTIVE] = &nfc_pn547_i2c_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_pn547_i2c_cfg,
		},
	},
	{
		.gpio      = 52,                /* NFC PN547 on BLSP9 QUP I2C_CLK */
		.settings = 
		{
			[GPIOMUX_ACTIVE] = &nfc_pn547_i2c_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_pn547_i2c_cfg,
		},
	},	
};
#endif

static struct msm_gpiomux_config msm_grip_configs[] __initdata = {
#if defined(CONFIG_MACH_CHAGALL_KDI)
	{
		.gpio      = 0,
		.settings = {
			[GPIOMUX_ACTIVE] = &grip_i2c_cfg,
			[GPIOMUX_SUSPENDED] = &grip_i2c_cfg,
		},
	},
	{
		.gpio      = 1,
		.settings = {
			[GPIOMUX_ACTIVE] = &grip_i2c_cfg,
			[GPIOMUX_SUSPENDED] = &grip_i2c_cfg,
		},
	},
#else
	{
		.gpio      = 60,
		.settings = {
			[GPIOMUX_ACTIVE] = &grip_i2c_cfg,
			[GPIOMUX_SUSPENDED] = &grip_i2c_cfg,
		},
	},
	{
		.gpio      = 61,
		.settings = {
			[GPIOMUX_ACTIVE] = &grip_i2c_cfg,
			[GPIOMUX_SUSPENDED] = &grip_i2c_cfg,
		},
	},
#endif
};

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

static struct gpiomux_setting sensor_cfg[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_UP,
		.dir = GPIOMUX_IN,
	},
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
		.dir = GPIOMUX_OUT_LOW,
	},
};

static struct msm_gpiomux_config msm_hsic_hub_configs[] = {
	{
		.gpio = 50,               /* HSIC_HUB_INT_N */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_hub_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
};

static struct gpiomux_setting hdmi_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
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

#if defined (CONFIG_MACH_CHAGALL_KDI)
static struct gpiomux_setting magnetic_sensor_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};
static struct gpiomux_setting acc_sensor_cfg = {
	.func = GPIOMUX_FUNC_GPIO,  /* IN-PU */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting chagall_muic_cfg = {
	.func = GPIOMUX_FUNC_GPIO,  /* IN-PU */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm_chagall_muic_configs[] __initdata = {
	{
		.gpio = 73,
		.settings = {
			[GPIOMUX_ACTIVE]    = &chagall_muic_cfg,
			[GPIOMUX_SUSPENDED] = &chagall_muic_cfg,
		},
	},
};
#endif

static struct msm_gpiomux_config msm_sensors_configs[] __initdata = {
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sensor_cfg[0],
			[GPIOMUX_SUSPENDED] = &sensor_cfg[0],
		},
	},
	
#if defined (CONFIG_MACH_CHAGALL_KDI)
	{
		.gpio      = 83,		/* BLSP11 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_ACTIVE] = &magnetic_sensor_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 84,		/* BLSP11 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &magnetic_sensor_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio = 122,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sensor_cfg[0],
			[GPIOMUX_SUSPENDED] = &sensor_cfg[1],
		},
	},
#endif
	{
		.gpio = 49,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sensor_cfg[0],
			[GPIOMUX_SUSPENDED] = &sensor_cfg[0],
		},
	},
	{
		.gpio = 74,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sensor_cfg[0],
			[GPIOMUX_SUSPENDED] = &sensor_cfg[0],
		},
	},
#if !(defined(CONFIG_TSPP) && defined(CONFIG_MACH_CHAGALL_KDI))
	{
		.gpio = 89,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sensor_cfg[0],
			[GPIOMUX_SUSPENDED] = &sensor_cfg[0],
		},
	},
#endif
	{
		.gpio = 82,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sensor_cfg[0],
			[GPIOMUX_SUSPENDED] = &sensor_cfg[0],
		},
	},
	{
		.gpio = 81,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sensor_cfg[1],
			[GPIOMUX_SUSPENDED] = &sensor_cfg[2],
		},
	},
#if defined(CONFIG_MACH_CHAGALL_KDI)
	{
		.gpio = 42,
		.settings = {
			[GPIOMUX_ACTIVE]    = &acc_sensor_cfg,
			[GPIOMUX_SUSPENDED] = &acc_sensor_cfg,
		},
	},
#endif
};

static struct msm_gpiomux_config msm_hdmi_configs[] __initdata = {
	{
		.gpio = 31,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
#ifndef CONFIG_VIDEO_MHL_V2
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 33,
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

#ifdef CONFIG_VIDEO_MHL_V2
#if defined (CONFIG_MACH_CHAGALL_KDI)
static struct gpiomux_setting mhl_active_cfg = {
        .func = GPIOMUX_FUNC_3,
        /*
         * Please keep I2C GPIOs drive-strength at minimum (2ma). It is a
         * workaround for HW issue of glitches caused by rapid GPIO current-
         * change.
         */
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mhl_suspend_cfg = {
        .func = GPIOMUX_FUNC_GPIO,
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_NONE,
};


static struct msm_gpiomux_config mhl_configs[] __initdata = {
        {
                .gpio      = 10, /* BLSP3 QUP I2C_DAT */
                .settings  = {
                        [GPIOMUX_ACTIVE]    = &mhl_active_cfg,
                        [GPIOMUX_SUSPENDED] = &mhl_suspend_cfg,
                },
        },
        {
                .gpio      = 11, /* BLSP3 QUP I2C_CLK */
                .settings  = {
                        [GPIOMUX_ACTIVE]    = &mhl_active_cfg,
                        [GPIOMUX_SUSPENDED] = &mhl_suspend_cfg,
                },
        },
};
#else
static struct gpiomux_setting mhl_active_cfg = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mhl_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config mhl_configs[] __initdata = {
	{
		.gpio      = 51, /* BLSP9 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_ACTIVE]    = &mhl_active_cfg,
			[GPIOMUX_SUSPENDED] = &mhl_suspend_cfg,
		},
	},
	{
		.gpio      = 52, /* BLSP9 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_ACTIVE]    = &mhl_active_cfg,
			[GPIOMUX_SUSPENDED] = &mhl_suspend_cfg,
		},
	},
};
#endif
#endif


#if defined(CONFIG_ISDBT_FC8300)
static struct gpiomux_setting isdb_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct msm_gpiomux_config msm8974_isdbt_configs[] __initdata = {
	{
		.gpio	   = 2,	/* I2C_SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &isdb_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio	   = 3,	/* I2C_SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &isdb_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
};
#endif

#if defined(CONFIG_TSPP) && defined(CONFIG_MACH_CHAGALL_KDI)
static struct gpiomux_setting isdb_tsif_config = {
        .func = GPIOMUX_FUNC_1,
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm8974_tsif_configs[] __initdata = {
        {
                .gpio   = 89,
                .settings = {
                        [GPIOMUX_ACTIVE] = &isdb_tsif_config,
                        [GPIOMUX_SUSPENDED] = &isdb_tsif_config,
                },
        },
        {
                .gpio   = 90,
                .settings = {
                        [GPIOMUX_ACTIVE] = &isdb_tsif_config,
                        [GPIOMUX_SUSPENDED] = &isdb_tsif_config,
                },
        },
        {
                .gpio   = 91,
                .settings = {
                        [GPIOMUX_ACTIVE] = &isdb_tsif_config,
                        [GPIOMUX_SUSPENDED] = &isdb_tsif_config,
                },
        },
        {
                .gpio   = 92,
                .settings = {
                        [GPIOMUX_ACTIVE] = &isdb_tsif_config,
                        [GPIOMUX_SUSPENDED] = &isdb_tsif_config,
                },
        },
};
#endif

#if !defined(CONFIG_BT_BCM4335) && !defined(CONFIG_BT_BCM4339) && !defined(CONFIG_BT_BCM4354)
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
#if !defined(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_H)
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
#endif
#if !defined(CONFIG_MACH_CHAGALL_KDI)
	{
		.gpio	= 44,	/* BLSP2 UART7 RFR */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_uart7_active_cfg,
			[GPIOMUX_SUSPENDED] = &gpio_uart7_suspend_cfg,
		},
	},
#endif
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
	{
		.gpio      = 81,		/* EPM enable */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_epm_config,
		},
	},
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
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 1,		/* BLSP1 QUP SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
#if !defined(CONFIG_ISDBT_FC8300)	
	{
		.gpio	  = 2,		/* BLSP1 QUP SPI_DATA_SSN */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs3_config,
		},
	},
	{
		.gpio      = 3,		/* BLSP1 QUP SPI_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
#endif	
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
/* TSP */
	{
		.gpio      = 6,		/* BLSP1 QUP2 I2C_DAT */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio      = 7,		/* BLSP1 QUP2 I2C_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
#if !defined (CONFIG_MACH_CHAGALL_KDI)
	{
		.gpio      = 10,		/* BLSP3 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio      = 11,		/* BLSP3 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
#endif
#ifdef CONFIG_SENSORS_VFS61XX
	{
		.gpio      = 25,		/* BLSP5 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_4,
		},
	},
	{
		.gpio      = 26,		/* BLSP5 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_5,
		},
	},
#endif
#if defined(CONFIG_MACH_CHAGALL_KDI)
/* SUB PMIC */
	{
		.gpio      = 29,                /* BLSP6 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio      = 30,                /* BLSP6 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
#else
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
#ifndef	CONFIG_VIDEO_MHL_V2
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
#endif
#if !defined (CONFIG_MACH_CHAGALL_KDI)
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
	{
		.gpio      = 4,			/* BLSP2 UART TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 5,			/* BLSP2 UART RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart2_config,
		},
	},
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
};
#if defined(CONFIG_MACH_CHAGALL_KDI) //Camera AF configs
static struct msm_gpiomux_config msm_af_configs[] __initdata = {
	{
		.gpio      = 43,		/* BLSP7 QUP2 I2C_DAT */
		.settings = {
			[GPIOMUX_ACTIVE] = &blsp7_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio      = 44,		/* BLSP7 QUP2 I2C_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &blsp7_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
};
#endif

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
#if defined(CONFIG_MACH_CHAGALL_KDI)
	{										/*spi active*/ /*7*/
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_12MA,
		.pull = GPIOMUX_PULL_NONE,
	},
#endif
};

static struct gpiomux_setting gpio_battery_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
static struct msm_gpiomux_config msm_battery_configs[] __initdata = {
	{
		.gpio = 76, /* IF_CON_SENSE */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_battery_config,
			[GPIOMUX_SUSPENDED] = &gpio_battery_config,
		},
	},
	{
		.gpio = 85, /* TA_nCHG */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_battery_config,
			[GPIOMUX_SUSPENDED] = &gpio_battery_config,
		},
	},
};

#if !defined(CONFIG_MACH_CHAGALL_KDI)
static struct gpiomux_setting gpio_batteryotg_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config msm_batteryotg_configs[] __initdata = {
	{
		.gpio = 69,
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_batteryotg_config,
			[GPIOMUX_SUSPENDED] = &gpio_batteryotg_config,
		},
	},
};
#endif

static struct msm_gpiomux_config msm_sensor_configs[] __initdata = {
	{
		.gpio = 15, /* CAM_MCLK0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio = 16, /* CAM_MCLK1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
	{
		.gpio = 17, /* CAM_MCLK2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
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
#ifndef CONFIG_SENSORS_VFS61XX
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
	{
		.gpio = 28, /* WEBCAM1_STANDBY */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#if !(defined(CONFIG_TSPP) && defined(CONFIG_MACH_CHAGALL_KDI))
	{
		.gpio = 89, /* CAM1_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 90, /* CAM1_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 91, /* CAM2_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 92, /* VT_CAM_ID */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
#endif
	{
		.gpio = 129, /* 8M_AVDD_LDO_EN */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],
		},
	},
#if defined(CONFIG_MACH_CHAGALL_KDI)			/*Camera eeprom spi configs*/
	{
		.gpio = 53, /* CIS_SPI_MOSI */
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[7],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 54, /* CIS_SPI_MISO */
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[7],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 55, /* CIS_SPI_MISO */
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[7],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 56, /* CIS_SPI_SCLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[7],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
#endif
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
	{
		.gpio = 17, /* CAM_MCLK2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
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
#ifndef CONFIG_SENSORS_VFS61XX
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
#endif
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
#if !(defined(CONFIG_TSPP) && defined(CONFIG_MACH_CHAGALL_KDI))
	{
		.gpio = 89, /* CAM1_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 90, /* CAM1_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 91, /* CAM2_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
#if !defined(CONFIG_MACH_CHAGALL_KDI)
#ifdef CONFIG_SENSORS_VFS61XX
	{
		.gpio = 94, /* CAM2_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
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

/* Primary AUXPCM port sharing GPIO lines with Tertiary MI2S */
static struct msm_gpiomux_config msm8974_pri_ter_auxpcm_configs[] __initdata = {
	{
		.gpio = 74,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
	{
		.gpio = 75,
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

static struct gpiomux_setting gpio_speaker_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config speaker_enable_config[] __initdata = {
#if defined(CONFIG_MACH_CHAGALL_KDI)
	{
		.gpio	= 135,		/* SPK_EN */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_speaker_suspend_config,
		},
	},
#else
	{
		.gpio	= 43,		/* SPK_EN */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_speaker_suspend_config,
		},
	},
#endif
};

static struct msm_gpiomux_config ear_send_end_config[] __initdata = {
	{
		.gpio	= 77,		/* EAR_SEND_END */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
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
	{
		.gpio	= 93,		/* CDC_INT2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
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
#if !(defined(CONFIG_TSPP) && defined(CONFIG_MACH_CHAGALL_KDI))
	{
		/* DAT3 */
		.gpio      = 92,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
#endif
#if !defined(CONFIG_MACH_CHAGALL_KDI)
#ifdef CONFIG_SENSORS_VFS61XX
	{
		/* DAT2 */
		.gpio      = 94,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
#endif
#endif
#if !(defined(CONFIG_TSPP) && defined(CONFIG_MACH_CHAGALL_KDI))
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

#if defined(CONFIG_BT_BCM4335) || defined(CONFIG_BT_BCM4339) || defined(CONFIG_BT_BCM4354)
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
static struct msm_gpiomux_config picasso_muic_configs[] __initdata = {
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

static struct gpiomux_setting tcon_ready_init_state = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting tcon_ready_sleep_state = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting sda_95_sleep_state = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

#if !defined(CONFIG_MACH_CHAGALL_KDI)
#if defined(CONFIG_KEYBOARD_TC360_TOUCHKEY)
static struct gpiomux_setting tkey_init_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm_tkey_configs[] __initdata = {
	{
		.gpio      = 29,		/* TKEY SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &tkey_init_sus_cfg,
		},
	},

	{
		.gpio      = 30,		/* TKEY SCL*/
		.settings = {
			[GPIOMUX_SUSPENDED] = &tkey_init_sus_cfg,
		},
	},
};
#endif
#endif
static struct gpiomux_setting scl_96_sleep_state = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm8974_lcd_config[] = {
	{
		.gpio = 63,	/* tcon_ready_config */
		.settings = {
			[GPIOMUX_ACTIVE] = &tcon_ready_init_state,
			[GPIOMUX_SUSPENDED] = &tcon_ready_sleep_state,
		},

		.gpio = 95,	/* tcon_ready_config */
		.settings = {
			[GPIOMUX_SUSPENDED] = &sda_95_sleep_state,
		},
		.gpio = 96,	/* tcon_ready_config */
		.settings = {
			[GPIOMUX_SUSPENDED] = &scl_96_sleep_state,
		},
	},
};


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
	msm_gpiomux_install(msm8974_muic_configs, ARRAY_SIZE(msm8974_muic_configs));
#endif
	msm_gpiomux_install(msm_battery_configs, ARRAY_SIZE(msm_battery_configs));
#if !defined(CONFIG_MACH_CHAGALL_KDI)
	msm_gpiomux_install(msm_batteryotg_configs, ARRAY_SIZE(msm_batteryotg_configs));
#endif

#if !defined(CONFIG_BT_BCM4335) && !defined(CONFIG_BT_BCM4339) && !defined(CONFIG_BT_BCM4354)
	msm_gpiomux_install(msm_blsp2_uart7_configs,
			 ARRAY_SIZE(msm_blsp2_uart7_configs));
#endif
#if defined(CONFIG_BT_BCM4335) || defined(CONFIG_BT_BCM4339) || defined(CONFIG_BT_BCM4354)
	msm_gpiomux_btuart_install();
#endif
#if defined(CONFIG_ISDBT_FC8300)
	msm_gpiomux_install(msm8974_isdbt_configs,ARRAY_SIZE(msm8974_isdbt_configs));
#endif
#if defined(CONFIG_TSPP) && defined(CONFIG_MACH_CHAGALL_KDI)
        msm_gpiomux_install(msm8974_tsif_configs,ARRAY_SIZE(msm8974_tsif_configs));
#endif
	if (of_board_is_liquid())
		msm_gpiomux_install_nowrite(ath_gpio_configs,
					ARRAY_SIZE(ath_gpio_configs));
	msm_gpiomux_install(msm8974_slimbus_config,
			ARRAY_SIZE(msm8974_slimbus_config));

		msm_gpiomux_install(hap_lvl_shft_config,
				ARRAY_SIZE(hap_lvl_shft_config));

	if (of_board_is_dragonboard() && machine_is_apq8074())
		msm_gpiomux_install(msm_sensor_configs_dragonboard, \
				ARRAY_SIZE(msm_sensor_configs_dragonboard));
	else
		msm_gpiomux_install(msm_sensor_configs, \
				ARRAY_SIZE(msm_sensor_configs));
#if defined(CONFIG_MACH_CHAGALL_KDI)
	msm_gpiomux_install(msm_af_configs,
				ARRAY_SIZE(msm_af_configs));
#endif

#ifdef CONFIG_SENSORS_VFS61XX
	msm_gpiomux_install(msm8974_fingerprint_configs,
		ARRAY_SIZE(msm8974_fingerprint_configs));
#endif

#if !defined(CONFIG_MACH_CHAGALL_KDI)
#if defined(CONFIG_KEYBOARD_TC360_TOUCHKEY)
	msm_gpiomux_install(msm_tkey_configs, ARRAY_SIZE(msm_tkey_configs));
#endif
#endif

	msm_gpiomux_install(&sd_card_det, 1);
	msm_gpiomux_sdc3_install();

	if (!(of_board_is_dragonboard() && machine_is_apq8074()))
		msm_gpiomux_sdc4_install();

	msm_gpiomux_install(msm_taiko_config, ARRAY_SIZE(msm_taiko_config));
	msm_gpiomux_install(speaker_enable_config, ARRAY_SIZE(speaker_enable_config));
	msm_gpiomux_install(ear_send_end_config, ARRAY_SIZE(ear_send_end_config));

	msm_gpiomux_install(msm_hsic_hub_configs,
				ARRAY_SIZE(msm_hsic_hub_configs));

	msm_gpiomux_install(msm_hdmi_configs, ARRAY_SIZE(msm_hdmi_configs));
#ifdef CONFIG_VIDEO_MHL_V2
	msm_gpiomux_install(mhl_configs, ARRAY_SIZE(mhl_configs));
#endif

#if 0
	msm_gpiomux_install(wcnss_5wire_interface,
				ARRAY_SIZE(wcnss_5wire_interface));
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
		msm_gpiomux_install(picasso_muic_configs,
				ARRAY_SIZE(picasso_muic_configs));
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

#if !defined(CONFIG_BT_BCM4335) && !defined(CONFIG_BT_BCM4339) && !defined(CONFIG_BT_BCM4354)
	if (of_board_is_rumi())
		msm_gpiomux_install(msm_rumi_blsp_configs,
				    ARRAY_SIZE(msm_rumi_blsp_configs));
#endif

	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_MDM)
		msm_gpiomux_install(mdm_configs,
			ARRAY_SIZE(mdm_configs));

	if (of_board_is_dragonboard() && machine_is_apq8074())
		msm_gpiomux_install(apq8074_dragonboard_ts_config,
			ARRAY_SIZE(apq8074_dragonboard_ts_config));

	msm_gpiomux_install(msm8974_lcd_config,
				ARRAY_SIZE(msm8974_lcd_config));
#if defined(CONFIG_MACH_CHAGALL_KDI)
	msm_gpiomux_install(msm_chagall_muic_configs,
			                        ARRAY_SIZE(msm_chagall_muic_configs));
#endif

	msm_gpiomux_install(msm_grip_configs, ARRAY_SIZE(msm_grip_configs));
	msm_gpiomux_install(msm_sensors_configs, ARRAY_SIZE(msm_sensors_configs));
	msm_gpiomux_install(gpio_nc_configs, ARRAY_SIZE(gpio_nc_configs));
	if (system_rev >= 3)
		msm_gpiomux_install(gpio_nc_rev03_configs, ARRAY_SIZE(gpio_nc_rev03_configs));

	msm_gpiomux_install(hw_rev_configs, ARRAY_SIZE(hw_rev_configs));
#if defined(CONFIG_NFC_PN547) && defined(CONFIG_SEC_LOCALE_JPN)
	msm_gpiomux_install(msm_nfc_pn547_configs,	ARRAY_SIZE(msm_nfc_pn547_configs));
#endif
}
