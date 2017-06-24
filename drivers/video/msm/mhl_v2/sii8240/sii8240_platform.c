
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/qpnp/pin.h>
#include <linux/sii8240.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/battery/sec_charger.h>
#include <linux/ctype.h>
/* #include <linux/barcode_emul.h> */
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#include "sii8240_platform.h"
#include "../../mdss/mdss_panel.h"
#include "../../mdss/mdss_hdmi_tx.h"
#include "../../mdss/mdss_hdmi_hdcp.h"

#if defined(CONFIG_MFD_MAX77803)
#define MFD_MAX778XX_COMMON
#include <linux/mfd/max77803-private.h>

#elif defined(CONFIG_MFD_MAX77888)
#define MFD_MAX778XX_COMMON
#include <linux/mfd/max77888-private.h>

#elif defined(CONFIG_MFD_MAX77804K)
#define MFD_MAX778XX_COMMON
#include <linux/mfd/max77804k-private.h>
#endif

#include <mach/gpiomux.h>

struct sii8240_platform_data *g_pdata;
#ifdef CONFIG_ARCH_MSM8974
extern struct hdmi_hdcp_ctrl *hdcp_ctrl_global;
#endif

static struct gpiomux_setting hdmi_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

#ifdef CONFIG_MACH_JACTIVESKT
static struct gpiomux_setting hdmi_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
#else
static struct gpiomux_setting hdmi_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif

static struct qpnp_pin_cfg  MHL_PIN_PM_GPIO_WAKE = {
	.mode = 1, /*QPNP_PIN_MODE_DIG_OUT*/
	.output_type = 0, /*QPNP_PIN_OUT_BUF_CMOS*/
	.invert = 0, /*QPNP_PIN_INVERT_DISABLE*/
	.pull = 5, /* QPNP_PIN_PULL_NO */
	.vin_sel = 2,
	.out_strength = 3, /*QPNP_PIN_OUT_STRENGTH_HIGH*/
	.src_sel = 0, /*QPNP_PIN_SEL_FUNC_CONSTANT*/
	.master_en = 1,
};

static struct qpnp_pin_cfg  MHL_PIN_PM_MPP_WAKE = {
	.mode = 1, /*QPNP_PIN_MODE_DIG_OUT*/
	.output_type = 0, /*QPNP_PIN_OUT_BUF_CMOS*/
	.invert = 0, /*QPNP_PIN_INVERT_DISABLE*/
	.pull = 1, /*PM8XXX_MPP_BI_PULLUP_OPEN*/
	.vin_sel = 2,
	.out_strength = 3, /*QPNP_PIN_OUT_STRENGTH_HIGH*/
	.src_sel = 0, /*QPNP_PIN_SEL_FUNC_CONSTANT*/
	.master_en = 1,
};

static struct qpnp_pin_cfg  MHL_PIN_PM_GPIO_MHL_RESET_SLEEP = {
	.mode = 0, /*QPNP_PIN_MODE_DIG_IN*/
	.output_type = 0, /*QPNP_PIN_OUT_BUF_CMOS*/
	.invert = 0, /*QPNP_PIN_INVERT_DISABLE*/
	.pull = 5, /* QPNP_PIN_PULL_NO */
	.vin_sel = 2,
	.out_strength = 3, /*QPNP_PIN_OUT_STRENGTH_HIGH*/
	.src_sel = 0, /*QPNP_PIN_SEL_FUNC_CONSTANT*/
	.master_en = 1,
};

static struct qpnp_pin_cfg  MHL_PIN_PM_GPIO_MHL_EN_SLEEP = {
	.mode = 1, /*QPNP_PIN_MODE_DIG_OUT*/
	.output_type = 0, /*QPNP_PIN_OUT_BUF_CMOS*/
	.invert = 0, /*QPNP_PIN_INVERT_DISABLE*/
	.pull = 4, /* QPNP_PIN_GPIO_PULL_DN */
	.vin_sel = 0,
	.out_strength = 3, /*QPNP_PIN_OUT_STRENGTH_HIGH*/
	.src_sel = 0, /*QPNP_PIN_SEL_FUNC_CONSTANT*/
	.master_en = 1,
};


static struct qpnp_pin_cfg  MHL_PIN_PM_MPP_SLEEP = {
	.mode = 0, /*QPNP_PIN_MODE_DIG_IN*/
	.output_type = 0, /*QPNP_PIN_OUT_BUF_CMOS*/
	.invert = 0, /*QPNP_PIN_INVERT_DISABLE*/
	.pull = 1, /*PM8XXX_MPP_BI_PULLUP_OPEN*/
	.vin_sel = 2,
	.out_strength = 3, /*QPNP_PIN_OUT_STRENGTH_HIGH*/
	.src_sel = 0, /*QPNP_PIN_SEL_FUNC_CONSTANT*/
	.master_en = 1,
};

static struct msm_gpiomux_config msm_hdmi_ddc_configs[] = {
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
};
static bool mhl_power_on;
#ifdef CONFIG_ARCH_MSM8974
int platform_ap_hdmi_hdcp_auth(struct sii8240_data *sii8240)
{
	int ret = 0;

	if (!sii8240->ap_hdcp_success) {
		if (!hdmi_hpd_status()) {
			pr_info("sii8240: HDMI hpd is low\n");
			return 0;
		}
		sii8240->mhl_ddc_bypass(true);
		hdcp_ctrl_global->hdcp_state = HDCP_STATE_AUTHENTICATING;
		ret = hdmi_hdcp_authentication_part1_start(hdcp_ctrl_global);
		sii8240->mhl_ddc_bypass(false);
		if (ret) {
			pr_err("%s: HDMI HDCP Auth Part I failed\n", __func__);
			return ret;
		}
		hdcp_ctrl_global->hdcp_state = HDCP_STATE_AUTHENTICATED;
		sii8240->ap_hdcp_success = true;
		msleep(100);
	}
	return ret;
}
#endif

bool platform_hdmi_hpd_status(void)
{
	return hdmi_hpd_status();
}
#if defined(CONFIG_OF)
/*FIXME, need to use more common/proper function
for checking a VBUS regardless of H/W charger IC*/
static bool sii8240_vbus_present(void)
{
	bool ret = true;
#ifdef MFD_MAX778XX_COMMON
	union power_supply_propval value;
	psy_do_property("sec-charger", get, POWER_SUPPLY_PROP_ONLINE, value);
	pr_info("sec-charger : %d\n", value.intval);
	if (value.intval == POWER_SUPPLY_TYPE_BATTERY
			|| value.intval == POWER_SUPPLY_TYPE_WIRELESS)
		ret = false;
#else
	struct sii8240_platform_data *pdata = g_pdata;
	if (pdata->gpio_ta_int > 0){
#ifdef CONFIG_CHARGER_SMB358
		msleep(300);
#endif
		ret = gpio_get_value_cansleep(pdata->gpio_ta_int) ?
								false : true;
	}
#endif
	pr_info("VBUS : %s in %s\n", ret ? "IN" : "OUT", __func__);
	return ret;
}

#ifdef CONFIG_SAMSUNG_LPM_MODE
extern int poweroff_charging;
#endif

static int sii8240_muic_get_charging_type(void)
{
#ifdef CONFIG_EXTCON
	struct sii8240_platform_data *pdata = g_pdata;
	if(pdata->is_smartdock == true)
		return -1;
	else
		return 1;
#else /* CONFIG_EXTCON */
#if defined(CONFIG_MFD_MAX77803)
	int muic_cable_type = max77803_muic_get_charging_type();
#elif defined(CONFIG_MFD_MAX77888)
	int muic_cable_type = max77888_muic_get_charging_type();
#elif defined(CONFIG_MFD_MAX77804K)
	int muic_cable_type = max77804k_muic_get_charging_type();
#else
	return 0;
#endif


#ifdef MFD_MAX778XX_COMMON
	pr_info("%s: muic cable_type = %d\n", __func__, muic_cable_type);

	switch (muic_cable_type) {
	case CABLE_TYPE_SMARTDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		return -1;
	default:
		break;
	}

	return 1;
#endif
#endif /* CONFIG_EXTCON */
}

static void sii8240_charger_mhl_cb(bool otg_enable, int charger)
{
	struct sii8240_platform_data *pdata = g_pdata;
	union power_supply_propval value;
	int i, ret = 0;
	struct power_supply *psy;
	pdata->charging_type = POWER_SUPPLY_TYPE_MISC;

	ret = sii8240_muic_get_charging_type();
	if (ret < 0) {
		pr_info("%s: It's not mhl cable type!\n", __func__);
		return;
	}

	pr_info("%s: otg_enable : %d, charger: %d\n", __func__, otg_enable, charger);

	if (charger == 0x00) {
		pr_info("%s() TA charger 500mA\n", __func__);
		pdata->charging_type = POWER_SUPPLY_TYPE_MHL_500;
	} else if (charger == 0x01) {
		pr_info("%s() TA charger 900mA\n", __func__);
		pdata->charging_type = POWER_SUPPLY_TYPE_MHL_900;
	} else if (charger == 0x02) {
		pr_info("%s() TA charger 1500mA\n", __func__);
		pdata->charging_type = POWER_SUPPLY_TYPE_MHL_1500;
	} else if (charger == 0x03) {
		pr_info("%s() USB charger\n", __func__);
		pdata->charging_type = POWER_SUPPLY_TYPE_MHL_USB;
	} else
		pdata->charging_type = POWER_SUPPLY_TYPE_BATTERY;

#ifdef CONFIG_MUIC_SUPPORT_MULTIMEDIA_DOCK
	pr_info("MMDock_code\n");
	if (pdata->is_multimediadock == true) {
		pr_info("MMDock platform variable was found true. Check otg value and update enum\n");
		if (otg_enable == true || charger == 0x00) {
			pr_info("MMDock_connected otg_enable = %d  charger = 0x%02x\n", otg_enable, charger);
            return;
        } else if (pdata->charging_type != POWER_SUPPLY_TYPE_BATTERY) {
			pdata->charging_type = (charger == 0x03) ? POWER_SUPPLY_TYPE_MDOCK_USB :POWER_SUPPLY_TYPE_MDOCK_TA;
			pr_info("sii8240 : %s MDOCK_TA with charger(0x%02x)\n", __func__, charger);
        }
	}
#endif

	if (otg_enable) {
		if (!sii8240_vbus_present()) {
#ifdef CONFIG_SAMSUNG_LPM_MODE
			if (!poweroff_charging) {
#else
			{
#endif
				if (pdata->muic_otg_set)
					pdata->muic_otg_set(true);
				pdata->charging_type = POWER_SUPPLY_TYPE_OTG;
			}
		}
	} else {
		if (pdata->muic_otg_set)
			pdata->muic_otg_set(false);
	}

	for (i = 0; i < 10; i++) {
		psy = power_supply_get_by_name("battery");
		if (psy)
			break;
	}
	if (i == 10) {
		pr_err("[ERROR] %s: fail to get battery ps\n", __func__);
		return;
	}
	value.intval = pdata->charging_type;
	ret = psy->set_property(psy, POWER_SUPPLY_PROP_ONLINE, &value);
	if (ret) {
		pr_err("[ERROR] %s: fail to set power_suppy ONLINE property(%d)\n",
			__func__, ret);
		return;
	}
}

static void sii8240_int_gpio_config(bool onoff)
{
	struct sii8240_platform_data *pdata = g_pdata;
	int rc = 0;

	if (onoff) {
		rc = gpio_tlmm_config(GPIO_CFG(pdata->gpio_mhl_irq, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
	} else {
		rc = gpio_tlmm_config(GPIO_CFG(pdata->gpio_mhl_irq, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
	}

	if (rc) {
		pr_err("[ERROR] %s: gpio_tlmm_config(%#x)=%d\n, onoff: %s",
				__func__, pdata->gpio_mhl_irq, rc, onoff ? "on": "off");
	}
}

static void of_sii8240_gpio_init(void)
{
	struct sii8240_platform_data *pdata = g_pdata;
	if (pdata->gpio_mhl_en > 0) {
		if (gpio_request(pdata->gpio_mhl_en, "mhl_en")) {
			pr_err("[ERROR] %s: unable to request gpio_mhl_en [%d]\n",
					__func__, pdata->gpio_mhl_en);
			return;
		}
		if (gpio_direction_output(pdata->gpio_mhl_en, 0)) {
			pr_err("[ERROR] %s: unable to  gpio_mhl_en low[%d]\n",
				__func__, pdata->gpio_mhl_en);
			return;
		}
	}

	if (pdata->gpio_mhl_reset > 0) {
		if (gpio_request(pdata->gpio_mhl_reset, "mhl_reset")) {
			pr_err("[ERROR] %s: unable to request gpio_mhl_reset [%d]\n",
					__func__, pdata->gpio_mhl_reset);
			return;
		}
		if (gpio_direction_output(pdata->gpio_mhl_reset, 0)) {
			pr_err("[ERROR] %s: unable to gpio_mhl_reset low[%d]\n",
				__func__, pdata->gpio_mhl_reset);
			return;
		}
	}

	if (pdata->drm_workaround)
		msm_gpiomux_install(msm_hdmi_ddc_configs, ARRAY_SIZE(msm_hdmi_ddc_configs));
/*
	if(pdata->gpio_barcode_emul) {
		ice_gpiox_get(FPGA_GPIO_MHL_EN);
		ice_gpiox_get(FPGA_GPIO_MHL_RST);
	}*/
}

static void of_sii8240_gpio_config(enum mhl_sleep_state sleep_status)
{
	struct sii8240_platform_data *pdata = g_pdata;
	int ret;

	pr_info("%s() %s - reset_pin_type(%d), en_pin_type(%d)\n", __func__,
		sleep_status ? "resume" : "suspend",
		pdata->gpio_mhl_reset_type, pdata->gpio_mhl_en_type);

	if (pdata->gpio_barcode_emul) {
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_mhl_reset, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_mhl_en, 0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
		return;
	}

	if (sleep_status == MHL_SUSPEND_STATE) {
		if (pdata->gpio_mhl_reset) {
			switch (pdata->gpio_mhl_reset_type) {
			case MHL_GPIO_AP_GPIO:
				gpio_tlmm_config(GPIO_CFG(pdata->gpio_mhl_reset, 0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
				break;
			case MHL_GPIO_PM_GPIO:
				ret = qpnp_pin_config(pdata->gpio_mhl_reset, &MHL_PIN_PM_GPIO_MHL_RESET_SLEEP);
				if (unlikely(ret < 0))
					pr_err("[ERROR] %s() set gpio_mhl_reset\n", __func__);
				break;
			case MHL_GPIO_PM_MPP:
				ret = qpnp_pin_config(pdata->gpio_mhl_reset, &MHL_PIN_PM_MPP_SLEEP);
				if (unlikely(ret < 0))
					pr_err("[ERROR] %s() set gpio_mhl_reset\n", __func__);
				break;
			default:
				break;
			}
		} else {
			pr_err("[ERROR] %s() gpio_mhl_reset is NULL\n", __func__);
		}

		if (pdata->gpio_mhl_en) {
			switch (pdata->gpio_mhl_en_type) {
			case MHL_GPIO_AP_GPIO:
				gpio_tlmm_config(GPIO_CFG(pdata->gpio_mhl_en, 0, GPIO_CFG_INPUT,
							GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
				break;
			case MHL_GPIO_PM_GPIO:
				ret = qpnp_pin_config(pdata->gpio_mhl_en, &MHL_PIN_PM_GPIO_MHL_EN_SLEEP);
				if (unlikely(ret < 0))
					pr_err("[ERROR] %s() set gpio_mhl_en\n", __func__);
				break;
			case MHL_GPIO_PM_MPP:
				ret = qpnp_pin_config(pdata->gpio_mhl_en, &MHL_PIN_PM_MPP_SLEEP);
				if (unlikely(ret < 0))
					pr_err("[ERROR] %s() set gpio_mhl_en\n", __func__);
				break;
			default:
				break;
			}
		} else {
			pr_err("[ERROR] %s() gpio_mhl_en is NULL\n", __func__);
		}

	/* suspend */
	} else if (sleep_status == MHL_RESUME_STATE) {

		if (pdata->gpio_mhl_reset) {
			switch (pdata->gpio_mhl_reset_type) {
			case MHL_GPIO_AP_GPIO:
				gpio_tlmm_config(GPIO_CFG(pdata->gpio_mhl_reset, 0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
				break;
			case MHL_GPIO_PM_GPIO:
				ret = qpnp_pin_config(pdata->gpio_mhl_reset, &MHL_PIN_PM_GPIO_WAKE);
				if (unlikely(ret < 0))
					pr_err("[ERROR] %s() set gpio_mhl_reset\n", __func__);
				break;
			case MHL_GPIO_PM_MPP:
				ret = qpnp_pin_config(pdata->gpio_mhl_reset, &MHL_PIN_PM_MPP_WAKE);
				if (unlikely(ret < 0))
					pr_err("[ERROR] %s() set gpio_mhl_reset\n", __func__);
				break;
			default:
				break;
			}
		} else {
			pr_err("[ERROR] %s() gpio_mhl_reset is NULL\n", __func__);
		}

		if (pdata->gpio_mhl_en) {
			switch (pdata->gpio_mhl_en_type) {
			case MHL_GPIO_AP_GPIO:
				gpio_tlmm_config(GPIO_CFG(pdata->gpio_mhl_en, 0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
				break;
			case MHL_GPIO_PM_GPIO:
				ret = qpnp_pin_config(pdata->gpio_mhl_en, &MHL_PIN_PM_GPIO_WAKE);
				if (unlikely(ret < 0))
					pr_err("[ERROR] %s() set gpio_mhl_en\n", __func__);
				break;
			case MHL_GPIO_PM_MPP:
				ret = qpnp_pin_config(pdata->gpio_mhl_en, &MHL_PIN_PM_MPP_WAKE);
				if (unlikely(ret < 0))
					pr_err("[ERROR] %s() set gpio_mhl_en\n", __func__);
				break;
			default:
				break;
			}
		} else {
			pr_err("[ERROR] %s() gpio_mhl_en is NULL\n", __func__);
		}
	}
#if defined(CONFIG_MACH_KACTIVELTE_DCM)
	gpio_tlmm_config (GPIO_CFG(pdata->gpio_mhl_sda, GPIOMUX_FUNC_3,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
	gpio_tlmm_config (GPIO_CFG(pdata->gpio_mhl_scl, GPIOMUX_FUNC_3,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
#endif
}

static void of_sii8240_hw_onoff(bool onoff)
{
	int ret;
	struct sii8240_platform_data *pdata = g_pdata;
	pr_info("%s: Onoff: %d\n", __func__, onoff);
	if (mhl_power_on == onoff) {
		pr_info("sii8240 : MHL power is already %d\n", onoff);
		return;
	}
	mhl_power_on = onoff;
	if (onoff) {
		/*
		if(pdata->gpio_barcode_emul)
			ice_gpiox_set(FPGA_GPIO_MHL_EN, onoff);
			*/

		if (pdata->gpio_mhl_en > 0)
			gpio_set_value_cansleep(pdata->gpio_mhl_en, onoff);

		if (pdata->vcc_1p2v) {
			ret = regulator_set_voltage(pdata->vcc_1p2v, 1200000, 1200000);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] regulator vcc_1p2v set_vtg failed rc\n");
				return;
			}

			ret = regulator_enable(pdata->vcc_1p2v);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] regulator vcc_1p2v enable failed rc\n");
				return;
			}
		}

		if (pdata->vcc_1p8v) {
			ret = regulator_set_voltage(pdata->vcc_1p8v, 1800000, 1800000);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] regulator vcc 1p8v set_vtg failed rc\n");
				goto err_regulator_1p8v;
			}

			ret = regulator_enable(pdata->vcc_1p8v);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] regulator vcc 1p8v enable failed rc\n");
				goto err_regulator_1p8v;
			}
		}

		if (pdata->vcc_3p3v) {
			ret = regulator_set_voltage(pdata->vcc_3p3v, 3300000, 3300000);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] regulator vcc_3p3v set_vtg failed rc\n");
				goto err_regulator_3p3v;
			}

			ret = regulator_enable(pdata->vcc_3p3v);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] regulator vcc_3p3v enable failed rc\n");
				goto err_regulator_3p3v;
			}
		}

	} else {
		/*
		if(pdata->gpio_barcode_emul)
			ice_gpiox_set(FPGA_GPIO_MHL_EN, onoff);
			*/

		if (pdata->gpio_mhl_en > 0)
			gpio_set_value_cansleep(pdata->gpio_mhl_en, onoff);

		if (pdata->vcc_1p2v) {
			ret = regulator_disable(pdata->vcc_1p2v);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] regulator vcc_1p2v disable failed rc\n");
				return;
			}
		}

		if (pdata->vcc_1p8v) {
			ret = regulator_disable(pdata->vcc_1p8v);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] regulator vcc_1p8v disable failed rc\n");
				return;
			}
		}

		if (pdata->vcc_3p3v) {
			ret = regulator_disable(pdata->vcc_3p3v);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] regulator vcc_3pv3 disable failed rc\n");
				return;
			}
		}

		usleep_range(10000, 20000);

		if (pdata->gpio_mhl_reset > 0)
			gpio_set_value_cansleep(pdata->gpio_mhl_reset, 0);
	}

	return;

err_regulator_3p3v:
	if (pdata->vcc_1p8v)
		regulator_disable(pdata->vcc_1p8v);
err_regulator_1p8v:
	if (pdata->vcc_1p2v)
		regulator_disable(pdata->vcc_1p2v);
}


static void of_sii8240_hw_reset(void)
{
	struct sii8240_platform_data *pdata = g_pdata;
	pr_info("%s: hw_reset\n" , __func__);
	if (pdata->gpio_barcode_emul) {
		/*
		usleep_range(10000, 20000);
		ice_gpiox_set(FPGA_GPIO_MHL_RST, 1);
		usleep_range(5000, 20000);
		ice_gpiox_set(FPGA_GPIO_MHL_RST, 0);
		usleep_range(10000, 20000);
		ice_gpiox_set(FPGA_GPIO_MHL_RST, 1);
		*/
	} else {
		usleep_range(10000, 20000);
		gpio_set_value_cansleep(pdata->gpio_mhl_reset, 1);
		usleep_range(5000, 20000);
		gpio_set_value_cansleep(pdata->gpio_mhl_reset, 0);
		usleep_range(10000, 20000);
		gpio_set_value_cansleep(pdata->gpio_mhl_reset, 1);
	}
	msleep(30);
}

enum mhl_gpio_type	of_sii8240_get_gpio_type(char *str_type)
{
	if (strcmp(str_type, "msmgpio") == 0)
		return MHL_GPIO_AP_GPIO;
	else if (strcmp(str_type, "pm_gpios") == 0)
		return MHL_GPIO_PM_GPIO;
	else if (strcmp(str_type, "pm_mpps") == 0)
		return MHL_GPIO_PM_MPP;
	else
		return MHL_GPIO_UNKNOWN_TYPE;
}

static int of_sii8240_parse_dt(void)
{
	struct sii8240_platform_data *pdata = g_pdata;
	struct device_node *np = pdata->tmds_client->dev.of_node;
	struct device *pdev = &pdata->tmds_client->dev;
	char *temp_string = NULL;

	struct platform_device *hdmi_pdev = NULL;
	struct device_node *hdmi_tx_node = NULL;

	pdata->gpio_mhl_irq = of_get_named_gpio_flags(np,
		"sii8240,gpio_mhl_irq", 0, NULL);
	if (pdata->gpio_mhl_irq > 0)
		pr_info("gpio: mhl_irq = %d\n", pdata->gpio_mhl_irq);

	pdata->gpio_mhl_reset = of_get_named_gpio_flags(np,
		"sii8240,gpio_mhl_reset", 0, NULL);
	if (pdata->gpio_mhl_reset > 0)
		pr_info("gpio: mhl_reset = %d\n", pdata->gpio_mhl_reset);

	if(of_property_read_string(np,
		"sii8240,gpio_mhl_reset_type",
		(const char **)&temp_string) == 0) {
		pdata->gpio_mhl_reset_type = of_sii8240_get_gpio_type(temp_string);
		pr_info("%s() gpio_mhl_reset_type = %d\n", __func__, pdata->gpio_mhl_reset_type);
	}

	pdata->gpio_mhl_wakeup = of_get_named_gpio_flags(np,
		"sii8240,gpio_mhl_wakeup", 0, NULL);
	if (pdata->gpio_mhl_wakeup > 0)
		pr_info("gpio: mhl_wakeup = %d\n", pdata->gpio_mhl_wakeup);

	pdata->gpio_mhl_scl = of_get_named_gpio_flags(np,
		"sii8240,gpio_mhl_scl", 0, NULL);
	if (pdata->gpio_mhl_scl > 0)
		pr_info("gpio: mhl_scl = %d\n",
					pdata->gpio_mhl_scl);

	pdata->gpio_mhl_sda = of_get_named_gpio_flags(np,
		"sii8240,gpio_mhl_sda", 0, NULL);
	if (pdata->gpio_mhl_sda > 0)
		pr_info("gpio: mhl_sda = %d\n", pdata->gpio_mhl_sda);

	pdata->gpio_mhl_en = of_get_named_gpio_flags(np,
		"sii8240,gpio_mhl_en", 0, NULL);
	if (pdata->gpio_mhl_en > 0)
		pr_info("gpio: mhl_en = %d\n", pdata->gpio_mhl_en);

	temp_string = NULL;
	if(of_property_read_string(np,
		"sii8240,gpio_mhl_en_type",
		(const char **)&temp_string) == 0) {
		pdata->gpio_mhl_en_type = of_sii8240_get_gpio_type(temp_string);
		pr_info("%s() gpio_mhl_en_type = %d\n", __func__, pdata->gpio_mhl_en_type);
	}

	pdata->gpio_ta_int = of_get_named_gpio_flags(np,
		"sii8240,gpio_ta_int", 0, NULL);
	if (pdata->gpio_ta_int > 0)
		pr_info("gpio: ta_int = %d\n", pdata->gpio_ta_int);

#ifdef	CONFIG_MACH_LT03_EUR
	pdata->swing_level = 0x0D; /*1 5*/
	pr_info("swing_level = 0x%X\n", pdata->swing_level);
#else
	if (!of_property_read_u32(np, "sii8240,swing_level",
				&pdata->swing_level))
		pr_info("swing_level = 0x%X\n", pdata->swing_level);
#endif
	if (!of_property_read_u32(np, "sii8240,damping",
				&pdata->damping))
		pr_info("damping = 0x%X\n", pdata->damping);
	else
		pdata->damping = BIT_MHLTX_CTL3_DAMPING_SEL_OFF;

	pdata->gpio_barcode_emul = of_property_read_bool(np,
			"sii8240,barcode_emul");
	pr_info("barcode_emul = %s\n",
			pdata->gpio_barcode_emul ? "true" : "false");

	pdata->drm_workaround = of_property_read_bool(np,
			"sii8240,drm_workaround");
	pr_info("drm_workaround = %s\n",
			pdata->drm_workaround ? "true" : "false");

	pdata->vcc_1p2v = regulator_get(pdev, "vcc_1p2v");
	if (IS_ERR(pdata->vcc_1p2v)) {
		pdata->vcc_1p2v = regulator_get(pdev, "max77826_ldo3");
		if (IS_ERR(pdata->vcc_1p2v)) {
			pr_err("sii8240,vcc_1p2v is not exist in device tree\n");
			pdata->vcc_1p2v = NULL;
		}
	}

	pdata->vcc_1p8v = regulator_get(pdev, "vcc_1p8v");
	if (IS_ERR(pdata->vcc_1p8v)) {
		pdata->vcc_1p8v = regulator_get(pdev, "max77826_ldo7");
		if (IS_ERR(pdata->vcc_1p8v)) {
			pr_err("sii8240,vcc_1p8v is not exist in device tree\n");
			pdata->vcc_1p8v = NULL;
		}
	}

	pdata->vcc_3p3v = regulator_get(pdev, "vcc_3p3v");
	if (IS_ERR(pdata->vcc_3p3v)) {
		pdata->vcc_3p3v = regulator_get(pdev, "max77826_ldo14");
		if (IS_ERR(pdata->vcc_3p3v)) {
			pr_err("sii8240,vcc_3p3v is not exist in device tree\n");
			pdata->vcc_3p3v = NULL;
		}
	}

	/* parse phandle for hdmi tx */
	hdmi_tx_node = of_parse_phandle(np, "qcom,hdmi-tx-map", 0);
	if (!hdmi_tx_node) {
		pr_err("%s: can't find hdmi phandle\n", __func__);
		goto finish;
	}

	hdmi_pdev = of_find_device_by_node(hdmi_tx_node);
	if (!hdmi_pdev) {
		pr_err("%s: can't find the device by node\n", __func__);
		goto finish;
	}
	pr_info("%s: hdmi_pdev [0X%x] to pdata->pdev\n",
	       __func__, (unsigned int)hdmi_pdev);

	pdata->hdmi_pdev = hdmi_pdev;
finish:

	return 0;
}

void sii8240_hdmi_register_mhl(void)
{
	struct sii8240_platform_data *pdata = g_pdata;
	struct msm_hdmi_mhl_ops *hdmi_mhl_ops;

	if (pdata->hdmi_pdev == NULL)
		return;

	hdmi_mhl_ops = kzalloc(sizeof(struct msm_hdmi_mhl_ops),
				    GFP_KERNEL);
	if (!hdmi_mhl_ops) {
		pr_err("%s: alloc hdmi mhl ops failed\n", __func__);
		return;
	}

	msm_hdmi_register_mhl(pdata->hdmi_pdev,
			hdmi_mhl_ops, NULL);

	pdata->hdmi_mhl_ops = hdmi_mhl_ops;
}

struct sii8240_platform_data *platform_init_data(struct i2c_client *client)
{
	struct sii8240_platform_data *pdata = NULL;
	pdata = kzalloc(sizeof(struct sii8240_platform_data), GFP_KERNEL);
	if (!pdata) {
		dev_err(&client->dev, "failed to allocate driver data\n");
		return NULL;
	}
	g_pdata = pdata;

	pdata->tmds_client = client;
	pdata->power = of_sii8240_hw_onoff;
	pdata->hw_reset = of_sii8240_hw_reset;
	pdata->gpio_cfg = of_sii8240_gpio_config;
	pdata->charger_mhl_cb = sii8240_charger_mhl_cb;
	pdata->vbus_present = sii8240_vbus_present;
	pdata->int_gpio_config =  sii8240_int_gpio_config;
#ifdef MFD_MAX778XX_COMMON
	pdata->muic_otg_set = muic_otg_control;
#endif
	of_sii8240_parse_dt();
	of_sii8240_gpio_init();
	sii8240_hdmi_register_mhl();

	return pdata;
}

#endif
