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

#ifdef CONFIG_FELICA
#include <mach/klte_felica_gpio.h>
#endif /* CONFIG_FELICA */

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
#define KS8851_IRQ_GPIO 94
#endif

extern unsigned int system_rev;

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
#if !defined(CONFIG_TDMB) && !defined(CONFIG_TDMB_MODULE)
#if defined(CONFIG_MACH_KLTE_JPN)
	GPIOMUX_SET_NC(73),
#elif !defined(CONFIG_MACH_K3GDUOS_CTC)
	GPIOMUX_SET_NC(18),
	#if !defined(CONFIG_MACH_KLTE_LTNDUOS)
	GPIOMUX_SET_NC(73),
	#endif
#endif
#endif
#if !defined(CONFIG_MACH_KLTE_CHN)
#if defined(CONFIG_MACH_KLTE_KOR)
	GPIOMUX_SET_NC(18),	/* Not use TDMB_DET pin, even though CONFIG_TDMB is enable */
	GPIOMUX_SET_NC(104),
	GPIOMUX_SET_NC(105),
	GPIOMUX_SET_NC(111),
	GPIOMUX_SET_NC(117),
	GPIOMUX_SET_NC(118),
#endif
#if defined(CONFIG_MACH_KLTE_VZW) || defined(CONFIG_MACH_KLTE_LRA)
	GPIOMUX_SET_NC(105),
#endif
#if defined(CONFIG_MACH_K3GDUOS_CTC)
	GPIOMUX_SET_NC(111),
	GPIOMUX_SET_NC(114),
	GPIOMUX_SET_NC(115),
#endif
	GPIOMUX_SET_NC(112),
	GPIOMUX_SET_NC(113),
	GPIOMUX_SET_NC(116),
#if !defined(CONFIG_GSM_MODEM_SPRD6500)
	GPIOMUX_SET_NC(119),
	GPIOMUX_SET_NC(135),
#endif
#endif

#if defined(CONFIG_MACH_KLTE_SPR)
	GPIOMUX_SET_NC(110),
	GPIOMUX_SET_NC(124),
	GPIOMUX_SET_NC(125),
	GPIOMUX_SET_NC(136),
#endif
#if defined(CONFIG_MACH_KLTE_TMO) || defined(CONFIG_MACH_KLTE_MTR)
	GPIOMUX_SET_NC(114),
	GPIOMUX_SET_NC(115),
	GPIOMUX_SET_NC(118),
	GPIOMUX_SET_NC(123),
	GPIOMUX_SET_NC(126),
	GPIOMUX_SET_NC(127),
#endif
};

static struct msm_gpiomux_config gpio_rev05_nc_configs[] __initdata = {
#if !defined(CONFIG_TDMB) && \
    !defined(CONFIG_TDMB_MODULE) && \
    !defined(CONFIG_MACH_KLTE_SPR) && \
    !defined(CONFIG_MACH_KLTE_JPN) && \
    !defined(CONFIG_GSM_MODEM_SPRD6500)
	GPIOMUX_SET_NC(49),
	GPIOMUX_SET_NC(50),
	GPIOMUX_SET_NC(51),
	GPIOMUX_SET_NC(52),
#endif
};

static struct msm_gpiomux_config gpio_rev06_nc_configs[] __initdata = {
	GPIOMUX_SET_NC(131),
};

static struct msm_gpiomux_config gpio_rev07_only_nc_configs[] __initdata = {
	GPIOMUX_SET_NC(120),
	GPIOMUX_SET_NC(121),
};

static struct msm_gpiomux_config gpio_rev07_nc_configs[] __initdata = {
	GPIOMUX_SET_NC(137),

#if defined(CONFIG_MACH_KLTE_EUR)
	GPIOMUX_SET_NC(123),
	GPIOMUX_SET_NC(126),
	GPIOMUX_SET_NC(127),
#endif
#if defined(CONFIG_MACH_KLTE_ATT)
	GPIOMUX_SET_NC(105),
	GPIOMUX_SET_NC(111),
	GPIOMUX_SET_NC(115),
	GPIOMUX_SET_NC(123),
	GPIOMUX_SET_NC(126),
	GPIOMUX_SET_NC(127),
#endif
#if defined(CONFIG_MACH_KLTE_KOR)
	GPIOMUX_SET_NC(126),
	GPIOMUX_SET_NC(127),
#endif
#if defined(CONFIG_MACH_K3GDUOS_CTC)
	GPIOMUX_SET_NC(126),
#endif
};

static struct msm_gpiomux_config gpio_rev08_nc_configs[] __initdata = {
#if defined(CONFIG_MACH_KLTE_ATT)
	GPIOMUX_SET_NC(106),
#endif
#if defined(CONFIG_MACH_KLTE_DCM)
	GPIOMUX_SET_NC(9),     // REV 09
	GPIOMUX_SET_NC(104),
	GPIOMUX_SET_NC(105),
	GPIOMUX_SET_NC(106),   // CP
	GPIOMUX_SET_NC(107),
	GPIOMUX_SET_NC(111),
	GPIOMUX_SET_NC(112),
	GPIOMUX_SET_NC(113),
	GPIOMUX_SET_NC(114),
	GPIOMUX_SET_NC(116),
	GPIOMUX_SET_NC(127),
	GPIOMUX_SET_NC(135), // REV 08B
	GPIOMUX_SET_NC(136),
#endif
#if defined(CONFIG_MACH_KLTE_KDI)
	GPIOMUX_SET_NC(9),     // REV 06
	GPIOMUX_SET_NC(104),   // CP
	GPIOMUX_SET_NC(105),   // CP
	GPIOMUX_SET_NC(106),   // CP
	GPIOMUX_SET_NC(112),
	GPIOMUX_SET_NC(113),
	GPIOMUX_SET_NC(114),
	GPIOMUX_SET_NC(116),
	GPIOMUX_SET_NC(127),
	GPIOMUX_SET_NC(135), // REV 06
#endif
};

static struct msm_gpiomux_config hw_rev_configs[] __initdata = {
	{
		.gpio = 16,	/* HW_REV(0) */
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
		.gpio = 8,	/* HW_REV(3) */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
};

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

#if !defined(CONFIG_SENSORS_SSP)
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
#if !defined(CONFIG_SENSORS_SSP)
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

static struct gpiomux_setting gpio_uart_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

/*  This setting is for Factory UART (w/r HW defect) */
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

static struct gpiomux_setting es705_intrevent_config = {
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

static struct gpiomux_setting earjack_send_end = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

#if defined(CONFIG_SENSORS_SSP_STM)
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

static struct gpiomux_setting gpio_spi_cs2_config = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif

static struct gpiomux_setting gpio_spi_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_spi_cs3_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct gpiomux_setting gpio_spi_cs1_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
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

static struct gpiomux_setting gpio_ncp6335b_sleep_config = {
        .func = GPIOMUX_FUNC_GPIO,
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_ncp6335b_active_config = {
        .func = GPIOMUX_FUNC_GPIO,
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_NONE,
};


static struct gpiomux_setting blsp7_i2c_config = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE) || defined(CONFIG_GSM_MODEM_SPRD6500) || defined(CONFIG_ISDBT_FC8300_SPI) || defined(CONFIG_ISDBT_FC8150_SPI)
static struct gpiomux_setting gpio_spi9_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

#if !defined(CONFIG_SENSORS_SSP)
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

#if defined(CONFIG_WCNSS_CORE)
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

#if !defined(CONFIG_SND_SOC_ES705)
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
#endif

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

#if !defined(CONFIG_PCM_ROUTE_VOICE_STUB)
#if defined(CONFIG_MFD_MAX77804K)
static struct gpiomux_setting ifpmic_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting ifpmic_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config ifpmic_i2c_configs[] __initdata = {
	{
		.gpio      = 60,		/* IF PMIC SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &ifpmic_act_cfg,
			[GPIOMUX_SUSPENDED] = &ifpmic_sus_cfg,
		},
	},
	{
		.gpio      = 61,		/* IF PMIC SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &ifpmic_act_cfg,
			[GPIOMUX_SUSPENDED] = &ifpmic_sus_cfg,
		},
	},
};
#endif
#endif

#if !defined(CONFIG_SENSORS_SSP)
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
#endif
static struct msm_gpiomux_config hap_lvl_shft_config[] __initdata = {
#if !defined(CONFIG_SENSORS_SSP)
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_SUSPENDED] = &hap_lvl_shft_suspended_config,
			[GPIOMUX_ACTIVE] = &hap_lvl_shft_active_config,
		},
	},
#endif
};

#if !defined(CONFIG_TDMB) && !defined(CONFIG_GSM_MODEM_SPRD6500) && !defined(CONFIG_ISDBT_FC8300_SPI) && !defined(CONFIG_ISDBT_FC8150_SPI)
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
#if !defined(CONFIG_ISDBT_FC8300_SPI) && !defined(CONFIG_ISDBT_FC8150_SPI)
static struct gpiomux_setting mhl_suspend_cfg = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif
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

#ifdef CONFIG_FB_MSM_MDSS_HDMI_MHL_SII8334
static struct gpiomux_setting mhl_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mhl_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_OUT_HIGH,
};
#endif

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

#ifdef CONFIG_FB_MSM_MDSS_HDMI_MHL_SII8334
static struct msm_gpiomux_config msm_mhl_configs[] __initdata = {
	{
		/* mhl-sii8334 pwr */
		.gpio = 12,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mhl_suspend_config,
			[GPIOMUX_ACTIVE]    = &mhl_active_1_cfg,
		},
	},
#if !defined(CONFIG_SENSORS_SSP)
	{
		/* mhl-sii8334 intr */
		.gpio = 82,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mhl_suspend_config,
			[GPIOMUX_ACTIVE]    = &mhl_active_1_cfg,
		},
	},
#endif
};
#endif

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
	{
		.gpio = 34,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
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
#if !defined(CONFIG_ISDBT_FC8300_SPI) && !defined(CONFIG_ISDBT_FC8150_SPI)
	{
		.gpio      = 51, /* BLSP9 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &mhl_suspend_cfg,
		},
	},
	{
		.gpio      = 52, /* BLSP9 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &mhl_suspend_cfg,
		},
	},
#endif
};
#endif
static struct gpiomux_setting gpio_uart7_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct msm_gpiomux_config msm_blsp2_uart7_configs[] __initdata = {
	{
		.gpio	= 41,	/* BLSP2 UART7 TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart7_config,
		},
	},
	{
		.gpio	= 42,	/* BLSP2 UART7 RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart7_config,
		},
	},
};

#if !defined(CONFIG_BT_BCM4354)
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

static struct msm_gpiomux_config msm_epm_configs[] __initdata = {
#if !defined(CONFIG_SENSORS_SSP)
	{
		.gpio      = 81,		/* EPM enable */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_epm_config,
		},
	},
#endif
	{
		.gpio      = 85,		/* EPM MARKER2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_epm_marker_config,
		},
	},
#if !defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY)
	{
		.gpio      = 96,		/* EPM MARKER1 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_epm_marker_config,
		},
	},
#endif
};

static struct msm_gpiomux_config msm_blsp_configs[] __initdata = {
#if defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_SBM)
        {
                .gpio      = 2,         /* BLSP1 QUP*/
                .settings = {
                        [GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
                },
        },
        {
                .gpio      = 3,         /* BLSP1 QUP */
                .settings = {
                        [GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
                },
        },

#endif
#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	{
		.gpio      = 9,		/* BLSP1 QUP SPI_CS2A_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs2_config,
		},
	},
	{
		.gpio      = 8,		/* BLSP1 QUP SPI_CS1_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs1_config,
		},
	},
#endif
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
#if !defined(CONFIG_SENSORS_SSP_STM)
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
			[GPIOMUX_SUSPENDED] = &gpio_uart_rx_config,
		},
	},
#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE) || defined(CONFIG_GSM_MODEM_SPRD6500) || defined(CONFIG_ISDBT_FC8300_SPI) || defined(CONFIG_ISDBT_FC8150_SPI)
	{
		.gpio	   = 49,	/* BLSP2 QUP3 SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi9_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio	   = 50,	/* BLSP2 QUP3 SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi9_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio	   = 51,	/* BLSP2 QUP3 SPI_CS0_N */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi9_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio	   = 52,	/* BLSP2 QUP3 SPI_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi9_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
#endif
	{
		.gpio      = 53,		/* BLSP2 QUP4 SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio      = 54,		/* BLSP2 QUP4 SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio      = 56,		/* BLSP2 QUP4 SPI_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_ncp6335b_active_config,
			[GPIOMUX_SUSPENDED] = &gpio_ncp6335b_sleep_config,
		},
	},
	{
		.gpio      = 55,		/* BLSP2 QUP4 SPI_CS0_N */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_ncp6335b_active_config,
			[GPIOMUX_SUSPENDED] = &gpio_ncp6335b_sleep_config,
		},
	},
#if defined(CONFIG_SENSORS_SSP_STM)
	{
		.gpio	   = 81,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi11_config2,
			[GPIOMUX_SUSPENDED] = &gpio_spi11_sus_config[2],
		},
	},
	{
		.gpio	   = 82,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi11_config2,
			[GPIOMUX_SUSPENDED] = &gpio_spi11_sus_config[2],
		},
	},
	{
		.gpio	   = 83,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi11_config,
			[GPIOMUX_SUSPENDED] = &gpio_spi11_sus_config[1],
		},
	},
	{
		.gpio	   = 84,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi11_config,
			[GPIOMUX_SUSPENDED] = &gpio_spi11_sus_config[0],
		},
	},
#endif
};

static struct msm_gpiomux_config msm_cis_spi_configs[] __initdata = {
	{
		.gpio      = 0,		/* BLSP1 QUP SPI_MOSI */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 1,		/* BLSP1 QUP SPI_MISO */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
        {
		.gpio	   = 2, 	/* BLSP1 QUP SPI_CSN */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 3,		/* BLSP1 QUP SPI_SCLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
};

static struct msm_gpiomux_config msm_cis_spi_configs_rev04[] __initdata = {
	{
		.gpio      = 53,		/* BLSP2 QUP4 SPI_MOSI */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 54,		/* BLSP2 QUP4 SPI_MISO */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
        {
		.gpio	   = 55, 	/* BLSP2 QUP4 SPI_CSN */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 56,		/* BLSP2 QUP4 SPI_SCLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
#if defined(CONFIG_MACH_KLTE_MAX77828_JPN)
	{
		.gpio      = 52,		/* BLSP2 QUP4 SPI_CSN2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs3_config,
		},
	},
#else
	{
		.gpio      = 90,		/* BLSP2 QUP4 SPI_CSN2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs3_config,
		},
	},
#endif
};

static struct msm_gpiomux_config msm_af_configs[] __initdata = {
	{
		.gpio      = 47,		/* BLSP7 QUP2 I2C_DAT */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio      = 48,		/* BLSP7 QUP2 I2C_CLK */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_i2c_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
};

static struct msm_gpiomux_config msm_af_configs_rev04[] __initdata = {
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
#if !defined(CONFIG_MACH_K3GDUOS_CTC)
	{
		.gpio	= 76,		/* es705 2mic int */
		.settings = {
			[GPIOMUX_SUSPENDED] = &es705_intrevent_config,
		},
	},
#else
	{
		.gpio	= 110,		/* es705 2mic int */
		.settings = {
			[GPIOMUX_SUSPENDED] = &es705_intrevent_config,
		},
	},
#endif
	{
		.gpio	= 79,		/* es705 intr event */
		.settings = {
			[GPIOMUX_SUSPENDED] = &es705_intrevent_config,
		},
	},
};

static struct msm_gpiomux_config earjack_send_end_config[] __initdata = {
	{
		.gpio	= 77,		/* earjack send end */
		.settings = {
			[GPIOMUX_SUSPENDED] = &earjack_send_end,
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
		.func = GPIOMUX_FUNC_1, /*active 2*/ /* 5 */
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_NONE,
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
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config sd_card_det __initdata = {
	.gpio = 62,
	.settings = {
		[GPIOMUX_ACTIVE]    = &sd_card_det_active_config,
		[GPIOMUX_SUSPENDED] = &sd_card_det_sleep_config,
	},
};

static struct msm_gpiomux_config msm_sensor_configs[] __initdata = {
	{
		.gpio = 15, /* CAM_MCLK0 */
#if !defined(CONFIG_MACH_KLTE_CHN) && !defined(CONFIG_MACH_K3GDUOS_CTC)
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],
		},
#else
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[5],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],
		},
#endif
	},
	{
		.gpio = 17, /* CAM_MCLK2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[5],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],
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
	{
		.gpio = 28, /* COMP_SPI_INT */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
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
#if !defined(CONFIG_SENSORS_SSP)
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
#if !defined(CONFIG_SENSORS_SSP)
	{
		.gpio = 91, /* CAM2_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
	{
		.gpio = 94, /* CAM2_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
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
#if !defined(CONFIG_SENSORS_SSP)
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
#if !defined(CONFIG_SND_SOC_ES705)
	{
		.gpio = 76,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
#endif
#if !defined(CONFIG_SAMSUNG_JACK)
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
#endif
};

static struct msm_gpiomux_config msm8974_sec_auxpcm_configs[] __initdata = {
#if !defined(CONFIG_SND_SOC_ES705)
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
#endif
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &auxpcm_act_cfg,
		},
	},
#if !defined(CONFIG_SENSORS_SSP)
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
#endif
};

#if defined(CONFIG_PCM_ROUTE_VOICE_STUB)
static struct gpiomux_setting sec_auxpcm_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sec_auxpcm_sus_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sec_auxpcm_act_pu_cfg = {  // should be set PU because of SC6500 setting.
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sec_auxpcm_sus_pu_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm8974_sec_quat_auxpcm_configs[] __initdata = {
	{
		.gpio = 58,
		.settings = {
			[GPIOMUX_SUSPENDED] = &sec_auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &sec_auxpcm_act_cfg,
		},
	},
	{
		.gpio = 59,
		.settings = {
			[GPIOMUX_SUSPENDED] = &sec_auxpcm_sus_pu_cfg,
			[GPIOMUX_ACTIVE] = &sec_auxpcm_act_pu_cfg,
		},
	},
	{
		.gpio = 60,
		.settings = {
			[GPIOMUX_SUSPENDED] = &sec_auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &sec_auxpcm_act_cfg,
		},
	},
	{
		.gpio = 61,
		.settings = {
			[GPIOMUX_SUSPENDED] = &sec_auxpcm_sus_cfg,
			[GPIOMUX_ACTIVE] = &sec_auxpcm_act_cfg,
		},
	},
};
#endif

#if defined(CONFIG_WCNSS_CORE)
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

#if !defined(CONFIG_SND_SOC_ES705)
static struct msm_gpiomux_config ath_gpio_configs[] = {
	/* mhl use gpio 51 for the mhl interrupt
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_ACTIVE]    = &ath_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &ath_gpio_suspend_cfg,
		},
	},
	*/
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_ACTIVE]    = &ath_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &ath_gpio_suspend_cfg,
		},
	},
};
#endif

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

#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE) || \
        defined(CONFIG_BCM4339) || defined(CONFIG_BCM4339_MODULE) || \
        defined(CONFIG_BCM4354) || defined(CONFIG_BCM4354_MODULE)
/* MSM8974 WLAN_HOST_WAKE GPIO Number */
#if defined(CONFIG_MACH_KLTE_JPN_WLAN_OBSOLETE)
#define GPIO_WL_HOST_WAKE 73
#else
#define GPIO_WL_HOST_WAKE 92
#endif

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

#if defined(CONFIG_SENSORS_SSP)
static struct gpiomux_setting ssp_setting[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_UP,
	},
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
};

static struct msm_gpiomux_config ssp_configs[] __initdata = {
#if !defined(CONFIG_TDMB) && !defined(CONFIG_GSM_MODEM_SPRD6500) && !defined(CONFIG_ISDBT_FC8300_SPI) && !defined(CONFIG_ISDBT_FC8150_SPI)
	{
		.gpio = 50,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
#endif
	{
		.gpio = 74,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
#if defined(CONFIG_MACH_KLTE_JPN)
#if defined(CONFIG_MACH_KLTE_MAX77828_JPN)
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
#else
	{
		.gpio = 73,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
#endif
#else
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
#endif
#if defined(CONFIG_MACH_KLTE_MAX77828_JPN)
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[2],
			[GPIOMUX_SUSPENDED] = &ssp_setting[2],
		},
	},
#else
	{
		.gpio = 89,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[2],
			[GPIOMUX_SUSPENDED] = &ssp_setting[2],
		},
	},
#endif
};
static struct msm_gpiomux_config ssp_configs_rev04[] __initdata = {
#if defined(CONFIG_MACH_KLTE_JPN)
#if defined(CONFIG_MACH_KLTE_MAX77828_JPN)
	{
		.gpio = 9,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
#else
	{
		.gpio = 304,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
#endif
#else
	{
		.gpio = 9,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
#endif
	{
		.gpio = 74,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
#if defined(CONFIG_MACH_KLTE_JPN)
#if defined(CONFIG_MACH_KLTE_MAX77828_JPN)
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
#else
	{
		.gpio = 73,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
#endif
#else
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[1],
			[GPIOMUX_SUSPENDED] = &ssp_setting[1],
		},
	},
#endif
#if defined(CONFIG_MACH_KLTE_MAX77828_JPN)
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[2],
			[GPIOMUX_SUSPENDED] = &ssp_setting[2],
		},
	},
#else
	{
		.gpio = 89,
		.settings = {
			[GPIOMUX_ACTIVE] = &ssp_setting[2],
			[GPIOMUX_SUSPENDED] = &ssp_setting[2],
		},
	},
#endif
};

#if defined(CONFIG_MACH_K3GDUOS_CTC)
static struct msm_gpiomux_config ssp_configs_chn_rev05[] __initdata = {
	{
		.gpio = 119,
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
			[GPIOMUX_ACTIVE] = &ssp_setting[2],
			[GPIOMUX_SUSPENDED] = &ssp_setting[2],
		},
	},
};
#endif
#endif

#if defined(CONFIG_NFC_PN547) || defined(CONFIG_BCM2079X_NFC_I2C)

static struct gpiomux_setting nfc_i2c_config = {
        .func = GPIOMUX_FUNC_GPIO,
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_NONE,
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

static struct msm_gpiomux_config msm_nfc_configs[] __initdata = {
	{
		.gpio      = 55,		/* NFC SDA */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_i2c_config,
			[GPIOMUX_SUSPENDED] = &nfc_i2c_config,
		},
	},
	{
		.gpio      = 56,		/* NFC SCL */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_i2c_config,
			[GPIOMUX_SUSPENDED] = &nfc_i2c_config,
		},
	},
	{
		.gpio      = 59,		/* NFC IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_irq_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_irq_cfg,
		},
	},
	{
		.gpio	= 63,		/* NFC FIRMWARE */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_firmware_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_firmware_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_nfc_configs_rev05[] __initdata = {
	{
		/* BLSP1 QUP I2C_DATA */
		.gpio      = 2,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
		},
	},
	{
		/* BLSP1 QUP I2C_CLK */
		.gpio      = 3,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 59,		/* NFC IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_irq_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_irq_cfg,
		},
	},
	{
		.gpio	= 63,		/* NFC FIRMWARE */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_firmware_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_firmware_cfg,
		},
	},
};
#if defined(CONFIG_MACH_K3GDUOS_CTC)
static struct msm_gpiomux_config msm_nfc_configs_rev06[] __initdata = {
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
	{
		.gpio      = 73,		/* NFC IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_irq_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_irq_cfg,
		},
	},
};
#else
static struct msm_gpiomux_config msm_nfc_configs_rev06[] __initdata = {
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
	{
		.gpio      = 59,		/* NFC IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_irq_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_irq_cfg,
		},
	},
};
#endif
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

#if defined(CONFIG_MACH_KLTE_JPN)
// HRM_INT , DCM REV 08A , KDDI REV 06
static struct gpiomux_setting general_gpio_suspend_cfg1 = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};
#else
static struct gpiomux_setting general_gpio_suspend_cfg1 = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
#endif

static struct gpiomux_setting general_gpio_suspend_cfg2 = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm8974_remainder_configs[] __initdata = {
	{
		/*detect memory*/
		.gpio      = 94,
		.settings = {
			[GPIOMUX_SUSPENDED] = &general_gpio_suspend_cfg1,
		},
	},
	{
		/*FORCE_USB_BOOT*/
		.gpio      = 103,
		.settings = {
			[GPIOMUX_SUSPENDED] = &general_gpio_suspend_cfg2,
		},
	},
};

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
	{
		/* DAT2 */
		.gpio      = 94,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
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
	{
		/* CMD */
		.gpio      = 91,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdc4_suspend_cfg,
		},
	},
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

#ifdef CONFIG_GPIO_MC5587
static struct gpiomux_setting gpio_gpioexpander_sleep_config = {
        .func = GPIOMUX_FUNC_GPIO,
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_gpioexpander_active_config = {
/*        .func = GPIOMUX_FUNC_3,	*/
        .func = GPIOMUX_FUNC_GPIO,
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_gpioexpander_reset_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct msm_gpiomux_config msm8974_gpioexpander_configs[] __initdata = {
        {
                /* EXPANDER_SDA_1.8V */
                .gpio = 29,
                .settings = {
                        [GPIOMUX_SUSPENDED] = &gpio_gpioexpander_sleep_config,
                        [GPIOMUX_ACTIVE] = &gpio_gpioexpander_active_config,
                },
        },
        {
                /* EXPANDER_SCL_1.8V */
                .gpio = 30,
                .settings = {
                        [GPIOMUX_SUSPENDED] = &gpio_gpioexpander_sleep_config,
                        [GPIOMUX_ACTIVE] = &gpio_gpioexpander_active_config,
                },
        },
        {
                /* EXPANDER_RESET */
                .gpio = 145,
                .settings = {
                        [GPIOMUX_SUSPENDED] = &gpio_gpioexpander_reset_config,
                        [GPIOMUX_ACTIVE] = &gpio_gpioexpander_reset_config,
                },
        },
};
#endif

#if defined(CONFIG_BT_BCM4354)
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

static struct gpiomux_setting gpio_fuel_i2c_config = {
        .func = GPIOMUX_FUNC_GPIO,
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
static struct msm_gpiomux_config fuel_i2c_config[] __initdata = {
	{
		/* FUEL_SDA_1.8V */
		.gpio      = 87,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_fuel_i2c_config,
		},
	},
	{
		/* FUEL_SCL_1.8V */
		.gpio      = 88,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_fuel_i2c_config,
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
	{
		/* BTP_RST_N */
		.gpio = 57,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
	{
		/* BTP_INT */
		.gpio = 64,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_irq_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_irq_config,
		},
	},
	{
		/* BTP_LDO */
		.gpio = 130,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_rst_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
	{
		/* BTP_OCP_FLAG */
		.gpio = 144,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_irq_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_irq_config,
		},
	},
};
static struct msm_gpiomux_config msm8974_fingerprint_configs_08[] __initdata = {
#if defined(CONFIG_MACH_KLTE_JPN)
	{
		/* BTP_LDO */
		.gpio = 41,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_rst_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
#else
	{
		/* BTP_LDO */
		.gpio = 63,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_rst_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
#endif
#if defined(CONFIG_MACH_KLTE_LTNDUOS)
	{
		/* BTP_LDO_EN2 */
		.gpio = 73,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_btp_rst_config,
			[GPIOMUX_ACTIVE] = &gpio_spi_btp_rst_config,
		},
	},
#endif
};

#endif
#ifdef CONFIG_FELICA

/* USE "GPIO_SHARED_I2C_SCL/SDA"
 * ("GPIO_MHL_SCL/SDA" sets initial configuration)
 */
static struct gpiomux_setting felica_i2c_sda_setting = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting felica_i2c_scl_setting = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting felica_uart_tx_setting = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting felica_uart_rx_setting = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting felica_uart_tx_rev04_setting = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting felica_uart_rx_rev04_setting = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting felica_rfs_setting = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting felica_pon_rev06_setting = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting felica_intu_setting = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting felica_int_setting = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm8974_felica_configs[] __initdata = {
	{
		.gpio = GPIO_FELICA_I2C_SDA,/*I2C_SDA*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_i2c_sda_setting,
			[GPIOMUX_SUSPENDED] = &felica_i2c_sda_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_I2C_SCL,/*I2C_SCL*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_i2c_scl_setting,
			[GPIOMUX_SUSPENDED] = &felica_i2c_scl_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_UART_TX,/*2-pin UART_TX*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_uart_tx_setting,
			[GPIOMUX_SUSPENDED] = &felica_uart_tx_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_UART_RX,/*2-pin UART_RX*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_uart_rx_setting,
			[GPIOMUX_SUSPENDED] = &felica_uart_rx_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_RFS,/*RFS*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_rfs_setting,
			[GPIOMUX_SUSPENDED] = &felica_rfs_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_INTU,/*INTU*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_intu_setting,
			[GPIOMUX_SUSPENDED] = &felica_intu_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_INT,/*INT*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_int_setting,
			[GPIOMUX_SUSPENDED] = &felica_int_setting,
		},
	},
};

static struct msm_gpiomux_config msm8974_felica_configs_rev04[] __initdata = {
	{
		.gpio = GPIO_FELICA_I2C_SDA,/*I2C_SDA*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_i2c_sda_setting,
			[GPIOMUX_SUSPENDED] = &felica_i2c_sda_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_I2C_SCL,/*I2C_SCL*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_i2c_scl_setting,
			[GPIOMUX_SUSPENDED] = &felica_i2c_scl_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_UART_TX_REV04,/*2-pin UART_TX*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_uart_tx_rev04_setting,
			[GPIOMUX_SUSPENDED] = &felica_uart_tx_rev04_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_UART_RX_REV04,/*2-pin UART_RX*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_uart_rx_rev04_setting,
			[GPIOMUX_SUSPENDED] = &felica_uart_rx_rev04_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_RFS,/*RFS*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_rfs_setting,
			[GPIOMUX_SUSPENDED] = &felica_rfs_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_INTU,/*INTU*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_intu_setting,
			[GPIOMUX_SUSPENDED] = &felica_intu_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_INT,/*INT*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_int_setting,
			[GPIOMUX_SUSPENDED] = &felica_int_setting,
		},
	},
};

static struct msm_gpiomux_config msm8974_felica_configs_rev06[] __initdata = {
	{
		.gpio = GPIO_FELICA_I2C_SDA,/*I2C_SDA*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_i2c_sda_setting,
			[GPIOMUX_SUSPENDED] = &felica_i2c_sda_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_I2C_SCL,/*I2C_SCL*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_i2c_scl_setting,
			[GPIOMUX_SUSPENDED] = &felica_i2c_scl_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_UART_TX_REV04,/*2-pin UART_TX same as in REV 04 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_uart_tx_rev04_setting,
			[GPIOMUX_SUSPENDED] = &felica_uart_tx_rev04_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_UART_RX_REV04,/*2-pin UART_RX same as in REV 04 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_uart_rx_rev04_setting,
			[GPIOMUX_SUSPENDED] = &felica_uart_rx_rev04_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_PON_REV06,/*PON*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_pon_rev06_setting,
			[GPIOMUX_SUSPENDED] = &felica_pon_rev06_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_RFS,/*RFS*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_rfs_setting,
			[GPIOMUX_SUSPENDED] = &felica_rfs_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_INTU,/*INTU*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_intu_setting,
			[GPIOMUX_SUSPENDED] = &felica_intu_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_INT,/*INT*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_int_setting,
			[GPIOMUX_SUSPENDED] = &felica_int_setting,
		},
	},
};
#endif /* CONFIG_FELICA */

static struct msm_gpiomux_config msm_bat_id_config[] __initdata = {
	{
		.gpio      = 63,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
};

#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY)
static struct gpiomux_setting gpio_tkey_sleep_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm8974_tkey_configs[] __initdata = {
	{
		.gpio = 95,	/* TKEY_SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_tkey_sleep_config,
		},
	},
	{
		.gpio = 96,	/* TKEY_SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_tkey_sleep_config,
		},
	},
};
#endif

static struct gpiomux_setting gpio_oledid_sleep_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_oledid_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config gpio_oledid_config[] __initdata = {
	{
		.gpio      = 129,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_oledid_active_config,
			[GPIOMUX_SUSPENDED] = &gpio_oledid_sleep_config,
		},
	},
};

void __init msm_8974_init_gpiomux(void)
{
	int rc;

	rc = msm_gpiomux_init_dt();
	if (rc) {
		pr_err("%s failed %d\n", __func__, rc);
		return;
	}

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	if (!(of_board_is_dragonboard() && machine_is_apq8074()))
		msm_gpiomux_install(msm_eth_configs, \
			ARRAY_SIZE(msm_eth_configs));
#endif
	msm_gpiomux_install(msm_blsp_configs, ARRAY_SIZE(msm_blsp_configs));
#if defined(CONFIG_MACH_KLTE_DCM) || defined(CONFIG_MACH_KLTE_KDI) || defined(CONFIG_MACH_KLTE_SBM)
	if(system_rev <=8)	// FeliCa GPIO 42 Confliction.
	{
	msm_gpiomux_install(msm_blsp2_uart7_configs,
			 ARRAY_SIZE(msm_blsp2_uart7_configs));
	}
#else
	msm_gpiomux_install(msm_blsp2_uart7_configs,
			 ARRAY_SIZE(msm_blsp2_uart7_configs));
#endif
#if defined(CONFIG_BT_BCM4354)
	msm_gpiomux_btuart_install();
#endif
#if defined(CONFIG_WCNSS_CORE)
	msm_gpiomux_install(wcnss_5wire_interface,
				ARRAY_SIZE(wcnss_5wire_interface));
#endif

#if !defined(CONFIG_SND_SOC_ES705)
	if (of_board_is_liquid())
		msm_gpiomux_install_nowrite(ath_gpio_configs,
					ARRAY_SIZE(ath_gpio_configs));
#endif

	msm_gpiomux_install(msm8974_slimbus_config,
				ARRAY_SIZE(msm8974_slimbus_config));
	msm_gpiomux_install(msm_taiko_config,
				ARRAY_SIZE(msm_taiko_config));
	msm_gpiomux_install(earjack_send_end_config,
				ARRAY_SIZE(earjack_send_end_config));
	if (system_rev >= 4)
		msm_gpiomux_install(es705_config,
					ARRAY_SIZE(es705_config));

	msm_gpiomux_install(hap_lvl_shft_config,
				ARRAY_SIZE(hap_lvl_shft_config));

#if !defined(CONFIG_PCM_ROUTE_VOICE_STUB)
#if defined(CONFIG_MFD_MAX77804K)
	msm_gpiomux_install(ifpmic_i2c_configs, ARRAY_SIZE(ifpmic_i2c_configs));
#endif
#endif

	if (of_board_is_dragonboard() && machine_is_apq8074())
		msm_gpiomux_install(msm_sensor_configs_dragonboard, \
				ARRAY_SIZE(msm_sensor_configs_dragonboard));
	else
		msm_gpiomux_install(msm_sensor_configs, \
				ARRAY_SIZE(msm_sensor_configs));

	msm_gpiomux_install(&sd_card_det, 1);

	if (machine_is_apq8074() && (of_board_is_liquid() || \
	    of_board_is_dragonboard()))
		msm_gpiomux_sdc3_install();

#if !defined(CONFIG_WCNSS_CORE)
	msm_gpiomux_sdc3_install();
#endif

	if (!(of_board_is_dragonboard() && machine_is_apq8074()))
		msm_gpiomux_sdc4_install();

#if !defined(CONFIG_TDMB) && !defined(CONFIG_GSM_MODEM_SPRD6500) && !defined(CONFIG_ISDBT_FC8300_SPI) && !defined(CONFIG_ISDBT_FC8150_SPI)
	msm_gpiomux_install(msm_hsic_hub_configs,
				ARRAY_SIZE(msm_hsic_hub_configs));
#endif
#ifdef CONFIG_GPIO_MC5587
	if(system_rev < 2)
		msm_gpiomux_install(msm8974_gpioexpander_configs,
				ARRAY_SIZE(msm8974_gpioexpander_configs));
#endif

	msm_gpiomux_install(msm_hdmi_configs, ARRAY_SIZE(msm_hdmi_configs));
#ifdef CONFIG_VIDEO_MHL_V2
#if !defined(CONFIG_ISDBT_FC8300_SPI) && !defined(CONFIG_ISDBT_FC8150_SPI)
	if (system_rev >= 4)
	{
		 mhl_configs[1].gpio = 10; /* BLSP3 QUP I2C_DAT */
		 mhl_configs[1].settings[GPIOMUX_ACTIVE] =
					&gpio_i2c_config;
		 mhl_configs[1].settings[GPIOMUX_SUSPENDED] =
					&gpio_suspend_config[2];

		 mhl_configs[2].gpio = 11; /* BLSP3 QUP I2C_CLK */
		 mhl_configs[2].settings[GPIOMUX_ACTIVE] =
					&gpio_i2c_config;
		 mhl_configs[2].settings[GPIOMUX_SUSPENDED] =
					&gpio_suspend_config[2];
	}
#endif
	msm_gpiomux_install(mhl_configs, ARRAY_SIZE(mhl_configs));
#endif
#ifdef CONFIG_FB_MSM_MDSS_HDMI_MHL_SII8334
	if (of_board_is_fluid())
		msm_gpiomux_install(msm_mhl_configs,
				    ARRAY_SIZE(msm_mhl_configs));
#endif

	if (of_board_is_liquid() ||
	    (of_board_is_dragonboard() && machine_is_apq8074()))
		msm_gpiomux_install(msm8974_pri_ter_auxpcm_configs,
				 ARRAY_SIZE(msm8974_pri_ter_auxpcm_configs));
	else
		msm_gpiomux_install(msm8974_pri_pri_auxpcm_configs,
				 ARRAY_SIZE(msm8974_pri_pri_auxpcm_configs));

	msm_gpiomux_wlan_host_wakeup_install();

	if (of_board_is_cdp())
		msm_gpiomux_install(msm8974_sec_auxpcm_configs,
				 ARRAY_SIZE(msm8974_sec_auxpcm_configs));
	else if (of_board_is_liquid() || of_board_is_fluid() ||
						of_board_is_mtp())
		msm_gpiomux_install(msm_epm_configs,
				ARRAY_SIZE(msm_epm_configs));

#if defined(CONFIG_PCM_ROUTE_VOICE_STUB)
	if (system_rev >= 6)	{
		msm_gpiomux_install(msm8974_sec_quat_auxpcm_configs,
			ARRAY_SIZE(msm8974_sec_quat_auxpcm_configs));
	}
#endif
		msm_gpiomux_install(gpio_oledid_config,
					ARRAY_SIZE(gpio_oledid_config));

#if !defined(CONFIG_BT_BCM4354)
	if (of_board_is_rumi())
		msm_gpiomux_install(msm_rumi_blsp_configs,
				    ARRAY_SIZE(msm_rumi_blsp_configs));
#endif
#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY)
	msm_gpiomux_install(msm8974_tkey_configs,
		ARRAY_SIZE(msm8974_tkey_configs));
#endif
	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_MDM)
		msm_gpiomux_install(mdm_configs,
			ARRAY_SIZE(mdm_configs));

	if (of_board_is_dragonboard() && machine_is_apq8074())
		msm_gpiomux_install(apq8074_dragonboard_ts_config,
			ARRAY_SIZE(apq8074_dragonboard_ts_config));

	msm_gpiomux_install(fuel_i2c_config,
			ARRAY_SIZE(fuel_i2c_config));

#if defined(CONFIG_SENSORS_SSP)
#if defined(CONFIG_MACH_K3GDUOS_CTC)
	if(system_rev > 5)
		msm_gpiomux_install(ssp_configs_chn_rev05,
			ARRAY_SIZE(ssp_configs_chn_rev05));
	else if(system_rev <= 5 && system_rev > 3 )
		msm_gpiomux_install(ssp_configs_rev04,
			ARRAY_SIZE(ssp_configs_rev04));
#else
	if(system_rev > 3)
		msm_gpiomux_install(ssp_configs_rev04,
			ARRAY_SIZE(ssp_configs_rev04));
#endif
	else
		msm_gpiomux_install(ssp_configs, ARRAY_SIZE(ssp_configs));
#endif
#ifdef CONFIG_SENSORS_VFS61XX
	msm_gpiomux_install(msm8974_fingerprint_configs,
		ARRAY_SIZE(msm8974_fingerprint_configs));
#endif
#if defined(CONFIG_NFC_PN547) || defined(CONFIG_BCM2079X_NFC_I2C)
	if (system_rev > 5)
		msm_gpiomux_install(msm_nfc_configs_rev06,
			ARRAY_SIZE(msm_nfc_configs_rev06));
	else if (system_rev > 3)
		msm_gpiomux_install(msm_nfc_configs_rev05,
			ARRAY_SIZE(msm_nfc_configs_rev05));
	else
		msm_gpiomux_install(msm_nfc_configs,
			ARRAY_SIZE(msm_nfc_configs));
#endif

	if(system_rev > 3)
		msm_gpiomux_install(msm_cis_spi_configs_rev04,
			ARRAY_SIZE(msm_cis_spi_configs_rev04));
	else
		msm_gpiomux_install(msm_cis_spi_configs,
			ARRAY_SIZE(msm_cis_spi_configs));

	if(system_rev > 3)
		msm_gpiomux_install(msm_af_configs_rev04,
			ARRAY_SIZE(msm_af_configs_rev04));
	else
		msm_gpiomux_install(msm_af_configs,
			ARRAY_SIZE(msm_af_configs));
#ifdef CONFIG_FELICA
 if (system_rev <= 7)
 {
	msm_gpiomux_install(msm8974_felica_configs,
		ARRAY_SIZE(msm8974_felica_configs));
 }
 else if ((system_rev >= 8) && (system_rev < 9))
 {
	msm_gpiomux_install(msm8974_felica_configs_rev04,
			ARRAY_SIZE(msm8974_felica_configs_rev04));
 }
 else
 {
 	msm_gpiomux_install(msm8974_felica_configs_rev06,
			ARRAY_SIZE(msm8974_felica_configs_rev06));
 }
#endif /* CONFIG_FELICA */

	if(system_rev > 5)
		msm_gpiomux_install(msm_bat_id_config,
				    ARRAY_SIZE(msm_bat_id_config));

#ifdef CONFIG_SENSORS_VFS61XX
#if defined(CONFIG_MACH_KLTE_KOR)
	if (system_rev > 8)
#else
	if (system_rev > 7)
#endif
		msm_gpiomux_install(msm8974_fingerprint_configs_08,
			ARRAY_SIZE(msm8974_fingerprint_configs_08));
#endif

	msm_gpiomux_install(hw_rev_configs, ARRAY_SIZE(hw_rev_configs));
	msm_gpiomux_install(gpio_nc_configs, ARRAY_SIZE(gpio_nc_configs));
	if(system_rev >= 4)
		msm_gpiomux_install(gpio_rev05_nc_configs,
				    ARRAY_SIZE(gpio_rev05_nc_configs));
	if(system_rev <= 5)
		msm_gpiomux_install(gpio_rev06_nc_configs,
				    ARRAY_SIZE(gpio_rev06_nc_configs));
	if(system_rev == 6)
		msm_gpiomux_install(gpio_rev07_only_nc_configs,
				    ARRAY_SIZE(gpio_rev07_only_nc_configs));
	if(system_rev >= 6)
		msm_gpiomux_install(gpio_rev07_nc_configs,
				    ARRAY_SIZE(gpio_rev07_nc_configs));
	if(system_rev >= 7)
		msm_gpiomux_install(gpio_rev08_nc_configs,
				    ARRAY_SIZE(gpio_rev08_nc_configs));

	msm_gpiomux_install(msm8974_remainder_configs,
			    ARRAY_SIZE(msm8974_remainder_configs));
}
