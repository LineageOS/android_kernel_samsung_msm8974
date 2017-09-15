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

#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>

#ifdef CONFIG_FELICA
#include <mach/hlte_felica_gpio.h>
#endif /* CONFIG_FELICA */
#define KS8851_IRQ_GPIO -1 // 94 // 94 is es325 pin
#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
#define WLAN_CLK	40
#define WLAN_SET	39
#define WLAN_DATA0	38
#define WLAN_DATA1	37
#define WLAN_DATA2	36
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
};

#define GPIOMUX_SET_NC(n) \
	{ \
		.gpio	   = n, \
		.settings = { \
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2], \
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2], \
		}, \
	}
#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
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
#if !defined(CONFIG_MACH_JS01LTEDCM)
static struct gpiomux_setting mdm2ap_pblrdy = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
#endif

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
#if !defined(CONFIG_MACH_JS01LTEDCM)	
	/* MDM2AP_PBL_READY*/
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_pblrdy,
		}
	},
#endif 	
};

#if defined(CONFIG_W1_SLAVE_DS28EL15) && !defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
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

#if defined(CONFIG_MACH_H3G_CHN_CMCC) || defined(CONFIG_MACH_H3GDUOS)
static struct gpiomux_setting gpio_synaptics_id_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config synaptics_tsp_id_configs[] __initdata = {
	{
		.gpio	   = 91,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_synaptics_id_config,
			[GPIOMUX_SUSPENDED] = &gpio_synaptics_id_config,
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

#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
static struct gpiomux_setting blsp6_i2c_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct gpiomux_setting gpio_eth_config = {
	.pull = GPIOMUX_PULL_UP,
	.drv = GPIOMUX_DRV_2MA,
	.func = GPIOMUX_FUNC_GPIO,
};

#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
static struct gpiomux_setting blsp7_i2c_config = {
    .func = GPIOMUX_FUNC_GPIO,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_DOWN,
};
#else
static struct gpiomux_setting blsp7_i2c_config = {
    .func = GPIOMUX_FUNC_4,
    .drv = GPIOMUX_DRV_2MA,
    .pull = GPIOMUX_PULL_NONE,
};
#endif

#if !defined(CONFIG_TDMB) && !defined(CONFIG_GSM_MODEM_SPRD6500)
static struct gpiomux_setting gpio_spi_cs2_config = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif


#if !defined(CONFIG_TDMB) && !defined(CONFIG_GSM_MODEM_SPRD6500)
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
#endif
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

#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE) || defined(CONFIG_GSM_MODEM_SPRD6500)
static struct gpiomux_setting gpio_spi_qup3_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
#if !defined(CONFIG_GSM_MODEM_SPRD6500)
static struct gpiomux_setting gpio_tdmb_int_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
static struct msm_gpiomux_config tdmb_int_config[] __initdata = {
	{
		.gpio = 18,	/* TDMB_ANT_INT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_tdmb_int_config,
			[GPIOMUX_ACTIVE] = &gpio_tdmb_int_config,
		},
	},
	{
		.gpio = 73,	/* TDMB_INT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
		},
	},
};
#endif
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
#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
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
#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
static struct msm_gpiomux_config wcnss_5gpio_interface[] = {
	{
	    .gpio = 36,
	    .settings = {
	            [GPIOMUX_ACTIVE]    = &wcnss_5gpio_active_cfg,
	            [GPIOMUX_SUSPENDED] = &wcnss_5gpio_suspend_cfg,
	    },
	},
	{
		.gpio = 37,
		.settings = {
				[GPIOMUX_ACTIVE]    = &wcnss_5gpio_active_cfg,
				[GPIOMUX_SUSPENDED] = &wcnss_5gpio_suspend_cfg,
		},
	},
	{
		.gpio = 38,
		.settings = {
				[GPIOMUX_ACTIVE]    = &wcnss_5gpio_active_cfg,
				[GPIOMUX_SUSPENDED] = &wcnss_5gpio_suspend_cfg,
		},
	},
	{
		.gpio = 39,
		.settings = {
				[GPIOMUX_ACTIVE]    = &wcnss_5gpio_active_cfg,
				[GPIOMUX_SUSPENDED] = &wcnss_5gpio_suspend_cfg,
		},
	},
	{
		.gpio = 40,
		.settings = {
				[GPIOMUX_ACTIVE]    = &wcnss_5gpio_active_cfg,
				[GPIOMUX_SUSPENDED] = &wcnss_5gpio_suspend_cfg,
		},
		},
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

#if defined(CONFIG_BCM2079X_NFC_I2C)
static struct gpiomux_setting nfc_irq_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
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

#if defined(CONFIG_BCM2079X_NFC_I2C) || defined(CONFIG_NFC_PN547)
static struct msm_gpiomux_config msm_nfc_configs[] __initdata = {
#if !defined(CONFIG_MACH_H3GDUOS) && !defined(CONFIG_MACH_JS01LTEZT)
	{
		.gpio      = 59,		/* NFC IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_irq_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_irq_cfg,
		},
	},
#else
	{
		.gpio      = 28,		/* NFC IRQ */
		.settings = {
			[GPIOMUX_ACTIVE] = &nfc_irq_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_irq_cfg,
		},
	},
#endif
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
#if !defined(CONFIG_MACH_H3G_CHN_CMCC) && !defined(CONFIG_MACH_H3GDUOS)
	GPIOMUX_SET_NC(91),		/* TSP_ID */
#endif
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
#if defined(CONFIG_MACH_HLTEKDI)
	{
		.gpio      = 55,		/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio      = 56,		/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
#elif defined(CONFIG_MACH_HLTEDCM)
	{
		.gpio      = 122,		/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio      = 124,		/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio      = 125,		/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio      = 135,		/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio      = 136,		/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
#endif
#if defined(CONFIG_MACH_JS01LTEDCM)
	GPIOMUX_SET_NC(12),
	GPIOMUX_SET_NC(29),
	GPIOMUX_SET_NC(30),
	GPIOMUX_SET_NC(80),
	GPIOMUX_SET_NC(103),
	GPIOMUX_SET_NC(105),
	GPIOMUX_SET_NC(106),
	GPIOMUX_SET_NC(107),
	GPIOMUX_SET_NC(112),
	GPIOMUX_SET_NC(113),
	GPIOMUX_SET_NC(114),
	GPIOMUX_SET_NC(115),
	GPIOMUX_SET_NC(118),
	GPIOMUX_SET_NC(119),
	GPIOMUX_SET_NC(122),
	GPIOMUX_SET_NC(124),
	GPIOMUX_SET_NC(125),
	GPIOMUX_SET_NC(127),
	GPIOMUX_SET_NC(135),
	GPIOMUX_SET_NC(136),
#endif
#if defined(CONFIG_SEC_LOCALE_KOR)
	GPIOMUX_SET_NC(103),
#endif
};

#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
static struct msm_gpiomux_config gpio_nc_configs_jsglte_chn_cmcc[] __initdata = {
	{
		.gpio     = 28,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 31,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 32,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 33,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 34,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 45,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 46,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 47,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 48,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
#if !defined(CONFIG_VIDEO_MHL_V2)
	{
		.gpio     = 51,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 52,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
#endif
	{
		.gpio     = 53,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 54,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 58,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 65,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 66,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 67,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 68,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 69,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 75,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 80,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 85,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 94,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 108,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 109,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 122,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 130,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 131,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio     = 132,			/* NC */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_suspend_config[2],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
};
#endif

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
	{
		.gpio      = 51,                /* BLSP9 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_i2c_config_9,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio      = 52,                /* BLSP9 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_ACTIVE]    = &gpio_i2c_config_9,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
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
#if defined(CONFIG_MACH_H3GDUOS)
static struct msm_gpiomux_config msm_lcd_configs[] __initdata = {
	{
		.gpio = 78,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_en_sus_cfg,
		},
	},
};
#else
static struct msm_gpiomux_config msm_lcd_configs[] __initdata = {
	{
		.gpio = 58,
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_en_sus_cfg,
		},
	},
};
#endif

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
#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE) || defined(CONFIG_GSM_MODEM_SPRD6500)
	{
		.gpio	   = 8,		/* BLSP1 QUP3 SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_qup3_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio	   = 9,		/* BLSP1 QUP3 SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_qup3_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio	   = 10,	/* BLSP1 QUP3 SPI_CS0_N */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_qup3_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	{
		.gpio	   = 11,	/* BLSP1 QUP3 SPI_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_spi_qup3_config,
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
#else
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
#if !defined(CONFIG_INPUT_WACOM) 
#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
    {
		.gpio      = 29,                /* BLSP6 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &blsp6_i2c_config,
		},
	},
	{
		.gpio      = 30,                /* BLSP6 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &blsp6_i2c_config,
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
#if !defined(CONFIG_VIDEO_MHL_V2)
	#if defined(CONFIG_MACH_HLTE_CHN_CMCC)|| defined(CONFIG_MACH_HLTE_CHN_TDOPEN)
	{
		.gpio      = 51,                /* NC */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
	#else
	{
		.gpio      = 51,                /* BLSP9 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_9,
		},
	},
	#endif
	{
		.gpio      = 52,                /* BLSP9 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_9,
		},
	},
#endif
#if ! defined(CONFIG_SENSORS_SSP)
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
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
#if defined(CONFIG_BCM2079X_NFC_I2C) || defined(CONFIG_NFC_PN547)
	{
		.gpio      = 55, /* BLSP10 QUP I2C_DAT */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 56, /* BLSP10 QUP I2C_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
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
#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
 	{
		.gpio = 15, /* CAM_MCLK0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
#else
	{
		.gpio = 15, /* CAM_MCLK0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
#endif
#if 0	
	{
		.gpio = 16, /* CAM_MCLK1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
#endif
#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
 	{
		.gpio = 17, /* CAM_MCLK2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
#else
	{
		.gpio = 17, /* CAM_MCLK2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
#endif
#if !defined(CONFIG_TDMB)
	{
		.gpio = 18, /* WEBCAM1_RESET_N / CAM_MCLK3 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
#endif
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
	{
		.gpio = 89, /* CAM1_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#if 0
	{
		.gpio = 90, /* CAM1_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
#endif
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
		.gpio = 92, /* VT_CAM_ID */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[2],
		},
	},
#if !defined(CONFIG_MACH_JS01LTEDCM)&&!defined(CONFIG_MACH_HLTE_CHN_CMCC)	
	{
		.gpio = 129, /* 8M_AVDD_LDO_EN */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[3],
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
#if !defined(CONFIG_TDMB)
	{
		.gpio = 18, /* WEBCAM1_RESET_N / CAM_MCLK3 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
#endif
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
#if !defined(CONFIG_SENSORS_SSP)
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

#ifdef CONFIG_PCM_ROUTE_VOICE_STUB
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
#endif /* CONFIG_PCM_ROUTE_VOICE_STUB */


#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
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
#if !defined(CONFIG_VIDEO_MHL_V2)
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_ACTIVE]    = &ath_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &ath_gpio_suspend_cfg,
		},
	},
#endif
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_ACTIVE]    = &ath_gpio_active_cfg,
			[GPIOMUX_SUSPENDED] = &ath_gpio_suspend_cfg,
		},
	},
};

#ifdef CONFIG_MACH_H3GDUOS
static struct msm_gpiomux_config msm_taiko_config[] __initdata = {
	{
		.gpio	= 110,		/* SYS_RST_N */
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
#else
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
#endif

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

#if !defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
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
#if !defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
static void msm_gpiomux_sdc3_install(void)
{
	msm_gpiomux_install(msm8974_sdc3_configs,
			    ARRAY_SIZE(msm8974_sdc3_configs));
}
#endif
#else
static void msm_gpiomux_sdc3_install(void) {}
#endif

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
#if !defined(CONFIG_SENSORS_SSP)
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

#ifdef CONFIG_INPUT_WACOM
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

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI)|| defined(CONFIG_MACH_HLTE_CHN_CMCC)
static struct gpiomux_setting gpio_pcd_int_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config msm8974_pcd_int_configs[] __initdata = {
	{
		.gpio = 100,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_pcd_int_config,
			[GPIOMUX_ACTIVE] = &gpio_pcd_int_config,
		},
	},
};
#endif

#ifdef CONFIG_FELICA

/* USE "GPIO_SHARED_I2C_SCL/SDA" 
 * ("GPIO_MHL_SCL/SDA" sets initial configuration)
 */
static struct gpiomux_setting felica_i2c_sda_setting = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting felica_i2c_scl_setting = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting felica_uart_tx_setting = {
	.func = GPIOMUX_FUNC_4,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting felica_uart_rx_setting = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting felica_rfs_setting = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
static struct gpiomux_setting felica_intu_setting = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};
static struct gpiomux_setting felica_hsel_setting = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting felica_pon_setting = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
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
		.gpio = GPIO_FELICA_HSEL,/*HSEL*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_hsel_setting,
			[GPIOMUX_SUSPENDED] = &felica_hsel_setting,
		},
	},
	{
		.gpio = GPIO_FELICA_PON,/*PON*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &felica_pon_setting,
			[GPIOMUX_SUSPENDED] = &felica_pon_setting,
		},
	},
	
};
#endif /* CONFIG_FELICA */

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

#if defined (CONFIG_W1_SLAVE_DS28EL15) && !defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
	msm_gpiomux_install(msm8974_cover_id_config, ARRAY_SIZE(msm8974_cover_id_config));
#endif

#if defined(CONFIG_MACH_H3G_CHN_CMCC) || defined(CONFIG_MACH_H3GDUOS)
	msm_gpiomux_install(synaptics_tsp_id_configs, ARRAY_SIZE(synaptics_tsp_id_configs));
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

#if !defined(CONFIG_SENSORS_SSP)
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
#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)

	if (system_rev < 3) {
		printk(KERN_ERR "%s, %d\n", __func__, __LINE__);
		msm_gpiomux_install(wcnss_5wire_interface,
					ARRAY_SIZE(wcnss_5wire_interface));
	}
	else {
		printk(KERN_ERR "%s, %d\n", __func__, __LINE__);
		msm_gpiomux_install(wcnss_5wire_interface,
					ARRAY_SIZE(wcnss_5wire_interface));
		msm_gpiomux_sdc3_install();
	}
#else
	msm_gpiomux_sdc3_install();
#endif

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
#if defined(CONFIG_BCM2079X_NFC_I2C) || defined(CONFIG_NFC_PN547)
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
#ifdef CONFIG_PCM_ROUTE_VOICE_STUB
		msm_gpiomux_install(msm8974_sec_quat_auxpcm_configs,
				 ARRAY_SIZE(msm8974_sec_quat_auxpcm_configs));
#endif /* CONFIG_PCM_ROUTE_VOICE_STUB */
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
#ifdef CONFIG_INPUT_WACOM
	msm_gpiomux_install(msm8974_wacom_configs,
		ARRAY_SIZE(msm8974_wacom_configs));
#endif

	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_MDM)
		msm_gpiomux_install(mdm_configs,
			ARRAY_SIZE(mdm_configs));

	if (of_board_is_dragonboard() && machine_is_apq8074())
		msm_gpiomux_install(apq8074_dragonboard_ts_config,
			ARRAY_SIZE(apq8074_dragonboard_ts_config));

#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
	msm_gpiomux_install(tdmb_int_config, ARRAY_SIZE(tdmb_int_config));
#endif

#ifdef CONFIG_FELICA
	msm_gpiomux_install(msm8974_felica_configs,
			ARRAY_SIZE(msm8974_felica_configs));
#endif /* CONFIG_FELICA */
#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI)|| defined(CONFIG_MACH_HLTE_CHN_CMCC)
	msm_gpiomux_install(msm8974_pcd_int_configs, ARRAY_SIZE(msm8974_pcd_int_configs));
#endif
#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
	msm_gpiomux_install(gpio_nc_configs_jsglte_chn_cmcc, ARRAY_SIZE(gpio_nc_configs_jsglte_chn_cmcc));
#endif
}
#if defined(CONFIG_MACH_JSGLTE_CHN_CMCC)
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
		 * Make sure that we send only 14 bits from LSB.+	 */
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

