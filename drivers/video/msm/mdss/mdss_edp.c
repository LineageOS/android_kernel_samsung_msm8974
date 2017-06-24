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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/qpnp/pwm.h>
#include <linux/clk.h>
#include <linux/spinlock_types.h>
#include <linux/kthread.h>
#include <asm/system.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/dma.h>

#include "mdss.h"
#include "mdss_panel.h"
#include "mdss_mdp.h"
#include "mdss_edp.h"
#include "mdss_debug.h"
#include <linux/qpnp/pin.h>
#if defined(CONFIG_EDP_TCON_MDNIE)
#include "edp_tcon_mdnie.h"
#include <linux/ctype.h>
#include <asm/div64.h>
#endif

#define RGB_COMPONENTS		3
#define VDDA_MIN_UV			1800000	/* uV units */
#define VDDA_MAX_UV			1800000	/* uV units */
#define VDDA_UA_ON_LOAD		100000	/* uA units */
#define VDDA_UA_OFF_LOAD	100		/* uA units */

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
char eeprom_version[20];
#define MAX_PWM_RESOLUTION 511
#define BIT_SHIFT 22

static int duty_level_table[256] = {
4, 4, 4, 8, 8, 
8, 12, 16, 16, 16, 
20, 20, 24, 24, 28, 
28, 32, 32, 36, 36, 
40, 40, 44, 44, 48, 
48, 52, 52, 56, 56, 
60, 60, 64, 64, 68, 
68, 72, 72, 76, 76, 
80, 80, 84, 84, 88, 
88, 92, 92, 96, 96, 
100, 100, 104, 104, 108, 
108, 112, 112, 116, 116, 
120, 120, 124, 124, 128, 
128, 132, 132, 136, 136, 
140, 140, 144, 144, 148, 
148, 152, 152, 156, 156, 
160, 160, 164, 164, 168, 
168, 172, 172, 176, 176, 
180, 180, 184, 184, 188, 
188, 192, 192, 196, 196, 
200, 200, 204, 204, 208, 
208, 212, 212, 216, 216, 
220, 220, 226, 226, 226, 
230, 234, 234, 238, 238, 
242, 242, 246, 246, 250, 
250, 254, 254, 258, 258, 
262, 262, 266, 266, 270, 
270, 274, 274, 278, 278, 
282, 282, 286, 286, 290, 
290, 294, 294, 298, 298, 
302, 302, 306, 306, 310, 
310, 314, 314, 318, 318, 
322, 322, 326, 326, 330, 
330, 334, 334, 338, 338, 
342, 342, 346, 346, 350, 
350, 354, 354, 358, 358, 
362, 362, 366, 366, 370, 
370, 374, 374, 378, 378, 
382, 382, 386, 386, 390, 
390, 394, 394, 398, 398, 
402, 402, 406, 406, 410, 
410, 414, 414, 418, 418, 
422, 422, 426, 426, 430, 
430, 434, 434, 438, 438, 
442, 442, 446, 446, 450, 
450, 454, 454, 458, 458, 
462, 462, 466, 466, 470, 
470, 474, 474, 478, 478, 
483, 483, 487, 487, 491, 
491, 495, 495, 499, 499, 
503, 503, 507, 507, 511, 
511, 
};

static int duty_ratio_table[256] = {
1, 1, 1, 1, 2, 
2, 2, 3, 3, 3, 
4, 4, 5, 5, 5, 
5, 6, 6, 7, 7, 
8, 8, 9, 9, 9, 
9, 10, 10, 11, 11, 
12, 12, 12, 12, 13, 
13, 14, 14, 15, 15, 
16, 16, 16, 16, 17, 
17, 18, 18, 19, 19, 
19, 19, 20, 20, 21, 
21, 22, 22, 22, 22, 
23, 23, 24, 24, 25, 
25, 26, 26, 26, 26, 
27, 27, 28, 28, 29, 
29, 29, 29, 30, 30, 
31, 31, 32, 32, 33, 
33, 33, 33, 34, 34, 
35, 35, 36, 36, 36, 
36, 37, 37, 38, 38, 
39, 39, 40, 40, 40, 
40, 41, 41, 42, 42, 
43, 43, 43, 43, 44, 
45, 46, 46, 46, 46, 
47, 47, 48, 48, 49, 
49, 50, 50, 50, 50, 
51, 51, 52, 52, 53, 
53, 54, 54, 54, 54, 
55, 55, 56, 56, 57, 
57, 57, 57, 58, 58, 
59, 59, 60, 60, 61, 
61, 61, 61, 62, 62, 
63, 63, 64, 64, 64, 
64, 65, 65, 66, 66, 
67, 67, 68, 68, 68, 
68, 69, 69, 70, 70, 
71, 71, 72, 72, 72, 
72, 73, 73, 74, 74, 
75, 75, 75, 75, 76, 
76, 77, 77, 78, 78, 
79, 79, 79, 79, 80, 
80, 81, 81, 82, 82, 
82, 82, 83, 83, 84, 
84, 85, 85, 86, 86, 
86, 86, 87, 87, 88, 
88, 89, 89, 90, 90, 
90, 90, 91, 91, 92, 
92, 93, 93, 93, 93, 
94, 94, 95, 95, 96, 
96, 97, 97, 97, 97, 
98, 98, 99, 99, 100, 
100, 
};

extern void edp_backlight_enable(void);
extern void edp_backlight_disable(void);
extern void edp_backlight_power_enable(void);
extern int edp_backlight_status(void);
static struct completion edp_power_sync;
static int edp_power_state;
static int recovery_mode;
static int edp_power_state;

DEFINE_MUTEX(edp_power_state_chagne);
DEFINE_MUTEX(edp_event_state_chagne);
DEFINE_MUTEX(brightness_mutex);

int get_edp_power_state(void)
{
	return edp_power_state;
}

static struct qpnp_pin_cfg  LCD_EN_PM_GPIO_WAKE =
{
	.mode = 1, /*QPNP_PIN_MODE_DIG_OUT*/
	.output_type = 0, /*QPNP_PIN_OUT_BUF_CMOS*/
	.invert = 0, /*QPNP_PIN_INVERT_DISABLE*/
	.pull = 5, /*QPNP_PIN_PULL_NO*/
	.vin_sel = 2,
	.out_strength = 3, /*QPNP_PIN_OUT_STRENGTH_HIGH*/
	.src_sel = 0, /*QPNP_PIN_SEL_FUNC_CONSTANT*/
	.master_en = 1,
};


static struct qpnp_pin_cfg  LCD_EN_PM_GPIO_SLEEP =
{
	.mode = 1, /*QPNP_PIN_MODE_DIG_OUT*/
	.output_type = 0, /*QPNP_PIN_OUT_BUF_CMOS*/
	.invert = 0, /*QPNP_PIN_INVERT_DISABLE*/
	.pull = 4, /*QPNP_PIN_PULL_DN*/
	.vin_sel = 2,
	.out_strength = 3, /*QPNP_PIN_OUT_STRENGTH_HIGH*/
	.src_sel = 0, /*QPNP_PIN_SEL_FUNC_CONSTANT*/
	.master_en = 1,
};

static struct qpnp_pin_cfg  LCD_PWM_PM_GPIO_WAKE =
{
	.mode = 1, /*QPNP_PIN_MODE_DIG_OUT*/
	.output_type = 0, /*QPNP_PIN_OUT_BUF_CMOS*/
	.invert = 0, /*QPNP_PIN_INVERT_DISABLE*/
	.pull = 5, /*QPNP_PIN_PULL_NO*/
	.vin_sel = 2,
	.out_strength = 3, /*QPNP_PIN_OUT_STRENGTH_HIGH*/
	.src_sel = 3, /*QPNP_PIN_SEL_FUNC_2*/
	.master_en = 1,
};

static struct qpnp_pin_cfg  LCD_PWM_PM_GPIO_SLEEP =
{
	.mode = 1, /*QPNP_PIN_MODE_DIG_OUT*/
	.output_type = 0, /*QPNP_PIN_OUT_BUF_CMOS*/
	.invert = 0, /*QPNP_PIN_INVERT_DISABLE*/
	.pull = 5, /*QPNP_PIN_PULL_NO*/
	.vin_sel = 2,
	.out_strength = 3, /*QPNP_PIN_OUT_STRENGTH_HIGH*/
	.src_sel = 0, /*QPNP_PIN_SEL_FUNC_CONSTANT*/
	.master_en = 1,
};
#endif

#if defined(CONFIG_EDP_ESD_FUNCTION)
static int edp_esd_power_state;
#endif

#define DEFAULT_BL_LEVEL 114 /* 140/640 us duty ratio */
#define MIN_BL_LEVEL 2

#if defined(CONFIG_MACH_VIENNA_LTE)
#define EXTRA_POWER_REVSION 0x08
#else
#define EXTRA_POWER_REVSION 0x00
#endif

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
void edp_reg_dump(void)
{
	struct mdss_edp_drv_pdata *ep = get_global_ep();
	int start_addr = 0xFD923400;
	int i;

	pr_info("############ %s start ############", __func__);

	if (!ep)
		return;

	for(i=0; i <0x700; i+=0x0C) {
		pr_info("addr 0x%04x : 0x%08x 0x%08x 0x%08x 0x%08x", i + start_addr,
			edp_read(ep->base + i), edp_read(ep->base + i + 4),
			edp_read(ep->base + i + 8), edp_read(ep->base + i + 0xC));
	}

	pr_info("############ %s end ############", __func__);
};
#endif

/*
 * Set uA and enable vdda
 */
static int mdss_edp_regulator_on(struct mdss_edp_drv_pdata *edp_drv)
{
	int ret;

	ret = regulator_set_optimum_mode(edp_drv->vdda_vreg, VDDA_UA_ON_LOAD);
	if (ret < 0) {
		pr_err("%s: vdda_vreg set regulator mode failed.\n", __func__);
		return ret;
	}

	ret = regulator_enable(edp_drv->vdda_vreg);
	if (ret) {
		pr_err("%s: Failed to enable vdda_vreg regulator.\n", __func__);
		return ret;
	}

#if defined(CONFIG_EDP_EXTERNAL_POWER)
	/* ONLY VIENNA use LDO22, LT03 doesn't use LDO22*/
	if (system_rev >= EXTRA_POWER_REVSION) {
		ret = regulator_set_optimum_mode(edp_drv->i2c_vreg, VDDA_UA_ON_LOAD);
		if (ret < 0) {
			pr_err("%s: i2c_vreg set regulator mode failed.\n", __func__);
			return ret;
		}

		ret = regulator_enable(edp_drv->i2c_vreg);
		if (ret) {
			pr_err("%s: Failed to enable i2c_vreg regulator.\n", __func__);
			return ret;
		}
	}
#endif
	config_i2c_lane(true);
	return 0;
}

/*
 * Init regulator needed for edp, 8974_l12
 */
static int mdss_edp_regulator_init(struct mdss_edp_drv_pdata *edp_drv)
{
	int ret;

	edp_drv->vdda_vreg = devm_regulator_get(&(edp_drv->pdev->dev), "vdda");
	if (IS_ERR(edp_drv->vdda_vreg)) {
		pr_err("%s: Could not get 8941_l12, ret = %ld\n", __func__,
				PTR_ERR(edp_drv->vdda_vreg));
		return -ENODEV;
	}

	ret = regulator_set_voltage(edp_drv->vdda_vreg,
			VDDA_MIN_UV, VDDA_MAX_UV);
	if (ret) {
		pr_err("%s: vdda_vreg set_voltage failed, ret=%d\n", __func__,
				ret);
		return -EINVAL;
	}

#if defined(CONFIG_EDP_EXTERNAL_POWER)
	/* ONLY VIENNA use LDO22, LT03 doesn't use LDO22*/
	if (system_rev >= EXTRA_POWER_REVSION) {
		edp_drv->i2c_vreg = devm_regulator_get(&(edp_drv->pdev->dev), "i2c_vreg");
		if (IS_ERR(edp_drv->vdda_vreg)) {
			pr_err("%s: Could not get i2c_vreg, ret = %ld\n", __func__,
					PTR_ERR(edp_drv->i2c_vreg));
			return -ENODEV;
		}

		ret = regulator_set_voltage(edp_drv->i2c_vreg, 2500000, 2500000);
		if (ret) {
			pr_err("%s: i2c_vreg set_voltage failed, ret=%d\n", __func__,
					ret);
			return -EINVAL;
		}
	}
#endif
	config_i2c_lane(true);
	return 0;
}

/*
 * Disable vdda and set uA
 */
static int mdss_edp_regulator_off(struct mdss_edp_drv_pdata *edp_drv)
{
	int ret;

	ret = regulator_disable(edp_drv->vdda_vreg);
	if (ret) {
		pr_err("%s: Failed to disable vdda_vreg regulator.\n",
				__func__);
		return ret;
	}

	ret = regulator_set_optimum_mode(edp_drv->vdda_vreg, VDDA_UA_OFF_LOAD);
	if (ret < 0) {
		pr_err("%s: vdda_vreg set regulator mode failed.\n",
				__func__);
		return ret;
	}

#if defined(CONFIG_EDP_EXTERNAL_POWER)
	/* ONLY VIENNA use LDO22, LT03 doesn't use LDO22*/
	if (system_rev >= EXTRA_POWER_REVSION) {
		ret = regulator_disable(edp_drv->i2c_vreg);
		if (ret) {
			pr_err("%s: Failed to disable i2c_vreg regulator.\n",
					__func__);
			return ret;
		}

		ret = regulator_set_optimum_mode(edp_drv->i2c_vreg, VDDA_UA_OFF_LOAD);
		if (ret < 0) {
			pr_err("%s: i2c_vreg set regulator mode failed.\n",
					__func__);
			return ret;
		}
	}
#endif
	config_i2c_lane(false);

	return 0;
}

/*
 * Enables the gpio that supply power to the panel and enable the backlight
 */
static int mdss_edp_gpio_panel_en(struct mdss_edp_drv_pdata *edp_drv)
{
	int ret = 0;

	edp_drv->gpio_panel_en = of_get_named_gpio(edp_drv->pdev->dev.of_node,
			"gpio-panel-en", 0);
	if (!gpio_is_valid(edp_drv->gpio_panel_en)) {
		pr_err("%s: gpio_panel_en=%d not specified\n", __func__,
				edp_drv->gpio_panel_en);
		goto gpio_err;
	}

	ret = gpio_request(edp_drv->gpio_panel_en, "disp_enable");
	if (ret) {
		pr_err("%s: Request reset gpio_panel_en failed, ret=%d\n",
				__func__, ret);
		return ret;
	}

	ret = gpio_direction_output(edp_drv->gpio_panel_en, 1);
	if (ret) {
		pr_err("%s: Set direction for gpio_panel_en failed, ret=%d\n",
				__func__, ret);
		goto gpio_free;
	}

	return 0;

gpio_free:
	gpio_free(edp_drv->gpio_panel_en);
gpio_err:
	return -ENODEV;
}

static int mdss_edp_pwm_config(struct mdss_edp_drv_pdata *edp_drv)
{
	int ret = 0;

	ret = of_property_read_u32(edp_drv->pdev->dev.of_node,
			"qcom,panel-pwm-period", &edp_drv->pwm_period);
	if (ret) {
		pr_warn("%s: panel pwm period is not specified, %d", __func__,
				edp_drv->pwm_period);
		edp_drv->pwm_period = -EINVAL;
	}

	ret = of_property_read_u32(edp_drv->pdev->dev.of_node,
			"qcom,panel-lpg-channel", &edp_drv->lpg_channel);
	if (ret) {
		pr_warn("%s: panel lpg channel is not specified, %d", __func__,
				edp_drv->lpg_channel);
		edp_drv->lpg_channel = -EINVAL;
	}

	if (edp_drv->pwm_period != -EINVAL &&
		edp_drv->lpg_channel != -EINVAL) {
		edp_drv->bl_pwm = pwm_request(edp_drv->lpg_channel,
				"lcd-backlight");
		if (edp_drv->bl_pwm == NULL || IS_ERR(edp_drv->bl_pwm)) {
			pr_err("%s: pwm request failed", __func__);
			edp_drv->bl_pwm = NULL;
			return -EIO;
		}
	} else {
		edp_drv->bl_pwm = NULL;
	}

	edp_drv->gpio_panel_pwm = of_get_named_gpio(edp_drv->pdev->dev.of_node,
			"gpio-panel-pwm", 0);
	if (!gpio_is_valid(edp_drv->gpio_panel_pwm)) {
		pr_err("%s: gpio_panel_pwm=%d not specified\n", __func__,
				edp_drv->gpio_panel_pwm);
		goto edp_free_pwm;
	}

	ret = gpio_request(edp_drv->gpio_panel_pwm, "disp_pwm");
	if (ret) {
		pr_err("%s: Request reset gpio_panel_pwm failed, ret=%d\n",
				__func__, ret);
		goto edp_free_pwm;
	}

	return 0;

edp_free_pwm:
	pwm_free(edp_drv->bl_pwm);
	return -ENODEV;
}

void mdss_edp_set_backlight(struct mdss_panel_data *pdata, u32 bl_level)
{
	int ret = 0;
	struct mdss_edp_drv_pdata *edp_drv = NULL;
	int bl_max;
	unsigned long long llpwm_period, ll_pwm_resolution;
	int duty_level = 0; /* 0~255 */
	int duty_period = 0;

	if (bl_level < MIN_BL_LEVEL) {
		pr_err("%s : bl_level(%d) is too low.. set to MIN(3)\n", __func__, bl_level);
		bl_level = MIN_BL_LEVEL;
	}

	edp_drv = container_of(pdata, struct mdss_edp_drv_pdata, panel_data);
	if (!edp_drv) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	if (edp_drv->bl_pwm == NULL) {
		pr_err("%s: edp_drv->bl_pwm=NULL.\n", __func__);
		return;
	}

	mutex_lock(&brightness_mutex);

	bl_max = edp_drv->panel_data.panel_info.bl_max;
	if (bl_level > bl_max)
		bl_level = bl_max;

	duty_level = duty_level_table[bl_level];

	if (edp_drv->duty_level == duty_level) {
		pr_err("%s : same duty level..(%d) do not pwm_config..\n", __func__, duty_level);
		mutex_unlock(&brightness_mutex);
		return;
	}

	llpwm_period = edp_drv->pwm_period;
	llpwm_period <<=  BIT_SHIFT;
	llpwm_period *= duty_level;
	ll_pwm_resolution = MAX_PWM_RESOLUTION;
	do_div(llpwm_period, ll_pwm_resolution);
	duty_period = (llpwm_period >> BIT_SHIFT); 

	ret = pwm_config(edp_drv->bl_pwm, duty_period * NSEC_PER_USEC, edp_drv->pwm_period * NSEC_PER_USEC);
	if (ret) {
		pr_err("%s: pwm_config() failed err=%d.\n", __func__, ret);
		mutex_unlock(&brightness_mutex);
		return;
	}

	ret = pwm_enable(edp_drv->bl_pwm);
	if (ret) {
		pr_err("%s: pwm_enable() failed err=%d\n", __func__, ret);
		mutex_unlock(&brightness_mutex);
		return;
	}

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
	tcon_pwm_duty(duty_ratio_table[bl_level], 1);
#endif

#if defined(CONFIG_EDP_ESD_FUNCTION)
	edp_drv->current_bl = bl_level;
#endif
	edp_drv->duty_level = duty_level;

	mutex_unlock(&brightness_mutex);

	pr_info("%s bl_level : %d duty_level : %d duty_period : %d  duty_ratio : %d",
				__func__, bl_level, duty_level, duty_period,
				duty_ratio_table[bl_level]);
}

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
void set_backlight_first_kick_off(void)
{
	static int first_kick_off;
	int i;
	struct mdss_edp_drv_pdata *ep = get_global_ep();

	if (first_kick_off)
		return;

	if (ep == NULL)
		return ;

	edp_backlight_power_enable();

	for (i = 0; i < DEFAULT_BL_LEVEL; i+=2)
		mdss_edp_set_backlight(&ep->panel_data, i);

	first_kick_off = 1;
}
#endif

int mdss_edp_mainlink_ready(struct mdss_edp_drv_pdata *ep, u32 which)
{
	u32 data;
	int cnt = 10;

	while (--cnt) {
		data = edp_read(ep->base + 0x84); /* EDP_MAINLINK_READY */
		if (data & which) {
			pr_debug("%s: which=%x ready\n", __func__, which);
			return 1;
		}
		usleep(1000);
	}
	pr_err("%s: which=%x NOT ready\n", __func__, which);

	return 0;
}

void mdss_edp_mainlink_reset(struct mdss_edp_drv_pdata *ep)
{
	edp_write(ep->base + 0x04, 0x02); /* EDP_MAINLINK_CTRL */
	usleep(1000);
	edp_write(ep->base + 0x04, 0); /* EDP_MAINLINK_CTRL */
}

void mdss_edp_mainlink_ctrl(struct mdss_edp_drv_pdata *ep, int enable)
{
	u32 data;

	data = edp_read(ep->base + 0x04);
	data &= ~BIT(0);

	if (enable)
		data |= 0x1;

	edp_write(ep->base + 0x04, data);
}

void mdss_edp_state_ctrl(struct mdss_edp_drv_pdata *ep, u32 state)
{
	edp_write(ep->base + EDP_STATE_CTRL, state);
}

void mdss_edp_aux_reset(struct mdss_edp_drv_pdata *ep)
{
	/*reset AUX */
	edp_write(ep->base + 0x300, BIT(1) | BIT(0)); /* EDP_AUX_CTRL */
	usleep(1000);
	edp_write(ep->base + 0x300, 0); /* EDP_AUX_CTRL */
}

void mdss_edp_aux_ctrl(struct mdss_edp_drv_pdata *ep, int enable)
{
	u32 data;

	data = edp_read(ep->base + 0x300);
	if (enable)
		data |= 0x01;
	else
		data |= ~0x01;
	edp_write(ep->base + 0x300, data); /* EDP_AUX_CTRL */
}

void mdss_edp_phy_pll_reset(struct mdss_edp_drv_pdata *ep)
{
	/* EDP_PHY_CTRL */
	edp_write(ep->base + 0x74, 0x005); /* bit 0, 2 */
	usleep(1000);
	edp_write(ep->base + 0x74, 0x000); /* EDP_PHY_CTRL */
}

int mdss_edp_phy_pll_ready(struct mdss_edp_drv_pdata *ep)
{
	int cnt;
	u32 status = 0;

	cnt = 100;
	while (--cnt) {
		status = edp_read(ep->base + 0x6c0);
		if (status & 0x01)
			break;
		usleep(100);
	}

	pr_info("%s: PLL cnt=%d status=%x\n", __func__, cnt, (int)status);

	if (cnt <= 0) {
		pr_err("%s: PLL NOT ready\n", __func__);
		return 0;
	} else
		return 1;
}

int mdss_edp_phy_ready(struct mdss_edp_drv_pdata *ep)
{
	u32 status;

	status = edp_read(ep->base + 0x598);
	status &= 0x01;

	return status;
}

void mdss_edp_phy_power_ctrl(struct mdss_edp_drv_pdata *ep, int enable)
{
	if (enable) {
		/* EDP_PHY_EDPPHY_GLB_PD_CTL */
		edp_write(ep->base + 0x52c, 0x3f);
		/* EDP_PHY_EDPPHY_GLB_CFG */
		edp_write(ep->base + 0x528, 0x1);
		/* EDP_PHY_PLL_UNIPHY_PLL_GLB_CFG */
		edp_write(ep->base + 0x620, 0xf);
	} else {
		/* EDP_PHY_EDPPHY_GLB_PD_CTL */
		edp_write(ep->base + 0x52c, 0xc0);
	}
}

void mdss_edp_lane_power_ctrl(struct mdss_edp_drv_pdata *ep, int up)
{
	int i, off, max_lane;
	u32 data;

	max_lane = ep->lane_cnt;

	if (up)
		data = 0;	/* power up */
	else
		data = 0x7;	/* power down */

	/* EDP_PHY_EDPPHY_LNn_PD_CTL */
	for (i = 0; i < max_lane; i++) {
		off = 0x40 * i;
		edp_write(ep->base + 0x404 + off , data);
	}

	/* power down un used lane */
	data = 0x7;	/* power down */
	for (i = max_lane; i < EDP_MAX_LANE; i++) {
		off = 0x40 * i;
		edp_write(ep->base + 0x404 + off , data);
	}
}

void mdss_edp_clock_synchrous(struct mdss_edp_drv_pdata *ep, int sync)
{
	u32 data;
	u32 color;

	/* EDP_MISC1_MISC0 */
	data = edp_read(ep->base + 0x02c);

	if (sync)
		data |= 0x01;
	else
		data &= ~0x01;

	/* only legacy rgb mode supported */
	color = 0; /* 6 bits */
	if (ep->edid.color_depth == 8)
		color = 0x01;
	else if (ep->edid.color_depth == 10)
		color = 0x02;
	else if (ep->edid.color_depth == 12)
		color = 0x03;
	else if (ep->edid.color_depth == 16)
		color = 0x04;

	color <<= 5;    /* bit 5 to bit 7 */

	data |= color;
	/* EDP_MISC1_MISC0 */
	edp_write(ep->base + 0x2c, data);
}

/* voltage mode and pre emphasis cfg */
void mdss_edp_phy_vm_pe_init(struct mdss_edp_drv_pdata *ep)
{
	/* EDP_PHY_EDPPHY_GLB_VM_CFG0 */
	edp_write(ep->base + 0x510, 0x3);	/* vm only */
	/* EDP_PHY_EDPPHY_GLB_VM_CFG1 */
	edp_write(ep->base + 0x514, 0x64);
	/* EDP_PHY_EDPPHY_GLB_MISC9 */
	edp_write(ep->base + 0x518, 0x6c);
}

void mdss_edp_config_ctrl(struct mdss_edp_drv_pdata *ep)
{
	struct dpcd_cap *cap;
	struct display_timing_desc *dp;
	u32 data = 0;

	dp = &ep->edid.timing[0];

	cap = &ep->dpcd;

	data = ep->lane_cnt - 1;
	data <<= 4;

	if (cap->enhanced_frame)
		data |= 0x40;

	if (ep->edid.color_depth == 8) {
		/* 0 == 6 bits, 1 == 8 bits */
		data |= 0x100;	/* bit 8 */
	}

	if (!dp->interlaced)	/* progressive */
		data |= 0x04;

	data |= 0x03;	/* sycn clock & static Mvid */

	edp_write(ep->base + 0xc, data); /* EDP_CONFIGURATION_CTRL */
}

static void mdss_edp_sw_mvid_nvid(struct mdss_edp_drv_pdata *ep)
{
	edp_write(ep->base + 0x14, 0x217); /* EDP_SOFTWARE_MVID */
	edp_write(ep->base + 0x18, 0x21a); /* EDP_SOFTWARE_NVID */
}

static void mdss_edp_timing_cfg(struct mdss_edp_drv_pdata *ep)
{
	struct mdss_panel_info *pinfo;
	u32 total_ver, total_hor;
	u32 data;

	pinfo = &ep->panel_data.panel_info;

	pr_debug("%s: width=%d hporch= %d %d %d\n", __func__,
		pinfo->xres, pinfo->lcdc.h_back_porch,
		pinfo->lcdc.h_front_porch, pinfo->lcdc.h_pulse_width);

	pr_debug("%s: height=%d vporch= %d %d %d\n", __func__,
		pinfo->yres, pinfo->lcdc.v_back_porch,
		pinfo->lcdc.v_front_porch, pinfo->lcdc.v_pulse_width);

	total_hor = pinfo->xres + pinfo->lcdc.h_back_porch +
		pinfo->lcdc.h_front_porch + pinfo->lcdc.h_pulse_width;

	total_ver = pinfo->yres + pinfo->lcdc.v_back_porch +
			pinfo->lcdc.v_front_porch + pinfo->lcdc.v_pulse_width;

	data = total_ver;
	data <<= 16;
	data |= total_hor;
	edp_write(ep->base + 0x1c, data); /* EDP_TOTAL_HOR_VER */

	data = (pinfo->lcdc.v_back_porch + pinfo->lcdc.v_pulse_width);
	data <<= 16;
	data |= (pinfo->lcdc.h_back_porch + pinfo->lcdc.h_pulse_width);
	edp_write(ep->base + 0x20, data); /* EDP_START_HOR_VER_FROM_SYNC */

	data = pinfo->lcdc.v_pulse_width;
	data <<= 16;
	data |= pinfo->lcdc.h_pulse_width;
	edp_write(ep->base + 0x24, data); /* EDP_HSYNC_VSYNC_WIDTH_POLARITY */

	data = pinfo->yres;
	data <<= 16;
	data |= pinfo->xres;
	edp_write(ep->base + 0x28, data); /* EDP_ACTIVE_HOR_VER */
}

int mdss_edp_wait4train(struct mdss_edp_drv_pdata *edp_drv)
{
	int ret = 0;

	if (edp_drv->cont_splash)
		return ret;

	ret = wait_for_completion_timeout(&edp_drv->video_comp, 30);
	if (ret <= 0) {
		pr_err("%s: Link Train timedout\n", __func__);
		ret = -EINVAL;
	} else {
		ret = 0;
	}

	pr_debug("%s:\n", __func__);

	return ret;
}

static void mdss_edp_irq_enable(struct mdss_edp_drv_pdata *edp_drv);
static void mdss_edp_irq_disable(struct mdss_edp_drv_pdata *edp_drv);

extern void tcon_i2c_slave_change(void);
int mdss_edp_on(struct mdss_panel_data *pdata)
{
	struct mdss_edp_drv_pdata *edp_drv = NULL;
	int ret = 0;

	if (!pdata) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	edp_drv = container_of(pdata, struct mdss_edp_drv_pdata,
			panel_data);

	pr_info("%s:+, cont_splash=%d\n", __func__, edp_drv->cont_splash);

	mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON, false);

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
	mutex_lock(&edp_power_state_chagne);
	INIT_COMPLETION(edp_power_sync);
#endif
	if (!edp_drv->cont_splash) { /* vote for clocks */
		qpnp_pin_config(edp_drv->gpio_panel_pwm, &LCD_PWM_PM_GPIO_WAKE);
		qpnp_pin_config(edp_drv->gpio_panel_en, &LCD_EN_PM_GPIO_WAKE);

		mdss_edp_regulator_on(edp_drv);
		mdss_edp_phy_pll_reset(edp_drv);
		mdss_edp_aux_reset(edp_drv);
		mdss_edp_mainlink_reset(edp_drv);
		mdss_edp_aux_ctrl(edp_drv, 1);

		ret = mdss_edp_prepare_clocks(edp_drv);
		if (ret)
			return ret;

		mdss_edp_phy_power_ctrl(edp_drv, 1);

		ret = mdss_edp_clk_enable(edp_drv);
		if (ret) {
			mdss_edp_unprepare_clocks(edp_drv);
			return ret;
		}

		mdss_edp_phy_pll_ready(edp_drv);

		mdss_edp_lane_power_ctrl(edp_drv, 1);

		mdss_edp_clock_synchrous(edp_drv, 1);
		mdss_edp_phy_vm_pe_init(edp_drv);
		mdss_edp_config_ctrl(edp_drv);
		mdss_edp_sw_mvid_nvid(edp_drv);
		mdss_edp_timing_cfg(edp_drv);

		gpio_set_value(edp_drv->gpio_panel_en, 1);

		INIT_COMPLETION(edp_drv->idle_comp);
		mdss_edp_mainlink_ctrl(edp_drv, 1);
		edp_write(edp_drv->base +0x304, 1);
		mdss_edp_irq_enable(edp_drv);

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
		mutex_unlock(&edp_power_state_chagne);

		if (!wait_for_completion_timeout(&edp_power_sync, 3 * HZ)) {
			pr_err("%s: timeout error\n", __func__);
		}

#if defined(CONFIG_EDP_ESD_FUNCTION)
		edp_esd_power_state = 1;
#endif
#endif
	} else {
#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
		edp_power_state = 1;
		mutex_unlock(&edp_power_state_chagne);
#endif
		mdss_edp_irq_enable(edp_drv);
		tcon_i2c_slave_change();

		if (gpio_get_value(edp_drv->gpio_panel_hpd)) {
			tcon_interanl_clock();
			read_firmware_version(eeprom_version);
	}
	}

	mdss_edp_wait4train(edp_drv);

	edp_drv->cont_splash = 0;

	pr_info("%s:- %s\n", __func__, eeprom_version);
	return ret;
}

int mdss_edp_off(struct mdss_panel_data *pdata)
{
	struct mdss_edp_drv_pdata *edp_drv = NULL;
	int ret = 0;

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
	mutex_lock(&edp_power_state_chagne);
	edp_power_state = 0;
#if defined(CONFIG_EDP_ESD_FUNCTION)
	edp_esd_power_state = 0;
#endif
#endif
	edp_drv = container_of(pdata, struct mdss_edp_drv_pdata,
					panel_data);
	if (!edp_drv) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	pr_err("%s:+, cont_splash=%d\n", __func__, edp_drv->cont_splash);

	/* wait until link training is completed */
	mutex_lock(&edp_drv->train_mutex);

	INIT_COMPLETION(edp_drv->idle_comp);
	mdss_edp_state_ctrl(edp_drv, ST_PUSH_IDLE);

	ret = wait_for_completion_timeout(&edp_drv->idle_comp, 100);
	if (ret <= 0)
		pr_err("%s: idle pattern timedout\n", __func__);

	mdss_edp_state_ctrl(edp_drv, 0);

	mdss_edp_sink_power_state(edp_drv, SINK_POWER_OFF);

	mdss_edp_irq_disable(edp_drv);

	pwm_disable(edp_drv->bl_pwm);
#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
	edp_backlight_disable();
#endif
	gpio_set_value(edp_drv->gpio_panel_en, 0);


	mdss_edp_mainlink_reset(edp_drv);
	mdss_edp_mainlink_ctrl(edp_drv, 0);

	mdss_edp_lane_power_ctrl(edp_drv, 0);
	mdss_edp_phy_power_ctrl(edp_drv, 0);

	mdss_edp_clk_disable(edp_drv);
	mdss_edp_unprepare_clocks(edp_drv);

	mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF, false);
	mdss_edp_aux_ctrl(edp_drv, 0);

	mdss_edp_regulator_off(edp_drv);

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
	mutex_unlock(&edp_power_state_chagne);
	edp_drv->duty_level = 0;

	qpnp_pin_config(edp_drv->gpio_panel_pwm, &LCD_PWM_PM_GPIO_SLEEP);
	qpnp_pin_config(edp_drv->gpio_panel_en, &LCD_EN_PM_GPIO_SLEEP);
#endif
	msleep(100); /* NDRA needs some delay after shutdown power */
	pr_err("%s:-- %s\n", __func__, eeprom_version);

	mutex_unlock(&edp_drv->train_mutex);
	return 0;
}

int mdss_edp_off_cont_splash(struct mdss_panel_data *pdata)
{
	struct mdss_edp_drv_pdata *edp_drv = NULL;

	edp_drv = container_of(pdata, struct mdss_edp_drv_pdata,
				panel_data);

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
	edp_backlight_disable();
#endif
	mdss_edp_state_ctrl(edp_drv, ST_PUSH_IDLE);

	gpio_set_value(edp_drv->gpio_panel_en, 0);

	msleep(20);

	mdss_edp_state_ctrl(edp_drv, 0);
	mdss_edp_mainlink_reset(edp_drv);
	mdss_edp_mainlink_ctrl(edp_drv, 0);

	mdss_edp_lane_power_ctrl(edp_drv, 0);
	mdss_edp_phy_power_ctrl(edp_drv, 0);

	mdss_edp_aux_ctrl(edp_drv, 0);

	mdss_edp_clk_disable(edp_drv);
	mdss_edp_unprepare_clocks(edp_drv);

	mdss_edp_regulator_off(edp_drv);

	mutex_unlock(&edp_drv->train_mutex);
	pr_info("%s:-\n", __func__);
	return 0;
}

static int mdss_edp_event_handler(struct mdss_panel_data *pdata,
				  int event, void *arg)
{
	int rc = 0;
	struct mdss_edp_drv_pdata *edp_drv = NULL;

	edp_drv = container_of(pdata, struct mdss_edp_drv_pdata,
					panel_data);
	if (!edp_drv) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
	mutex_lock(&edp_event_state_chagne);
#endif

	pr_info("%s: event=%d\n", __func__, event);
	switch (event) {
	case MDSS_EVENT_RESET:
#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
		if (edp_backlight_status() > 0) {
			pwm_disable(edp_drv->bl_pwm);
			edp_backlight_disable();
		}
		break;
#endif
	case MDSS_EVENT_UNBLANK:
		rc = mdss_edp_on(pdata);
		break;
	case MDSS_EVENT_PANEL_OFF:
		rc = mdss_edp_off(pdata);
		break;
	default:
		pr_info("%s : Unknown event (%d)\n", __func__, event);
		break;
	}
#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
	mutex_unlock(&edp_event_state_chagne);
#endif

	return rc;
}

/*
 * Converts from EDID struct to mdss_panel_info
 */
static void mdss_edp_edid2pinfo(struct mdss_edp_drv_pdata *edp_drv)
{
	struct display_timing_desc *dp;
	struct mdss_panel_info *pinfo;
	struct edp_edid *edid = &edp_drv->edid;

	dp = &edp_drv->edid.timing[0];
	pinfo = &edp_drv->panel_data.panel_info;

	pinfo->clk_rate = dp->pclk;
	pr_debug("%s: pclk=%d\n", __func__, pinfo->clk_rate);

	pinfo->xres = dp->h_addressable + dp->h_border * 2;
	pinfo->yres = dp->v_addressable + dp->v_border * 2;

	pr_debug("%s: x=%d y=%d\n", __func__, pinfo->xres, pinfo->yres);
	pinfo->physical_width = edid->timing[0].width_mm;
	pinfo->physical_height = edid->timing[0].height_mm;

	pinfo->lcdc.h_back_porch = dp->h_blank - dp->h_fporch \
		- dp->h_sync_pulse;
	pinfo->lcdc.h_front_porch = dp->h_fporch;
	pinfo->lcdc.h_pulse_width = dp->h_sync_pulse;

	pr_debug("%s: hporch= %d %d %d\n", __func__,
		pinfo->lcdc.h_back_porch, pinfo->lcdc.h_front_porch,
		pinfo->lcdc.h_pulse_width);

	pinfo->lcdc.v_back_porch = dp->v_blank - dp->v_fporch \
		- dp->v_sync_pulse;
	pinfo->lcdc.v_front_porch = dp->v_fporch;
	pinfo->lcdc.v_pulse_width = dp->v_sync_pulse;

	pr_debug("%s: vporch= %d %d %d\n", __func__,
		pinfo->lcdc.v_back_porch, pinfo->lcdc.v_front_porch,
		pinfo->lcdc.v_pulse_width);

	pinfo->type = EDP_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = edp_drv->edid.color_depth * RGB_COMPONENTS;
	pinfo->fb_num = 2;

	pinfo->lcdc.border_clr = 0;	 /* black */
	pinfo->lcdc.underflow_clr = 0xff; /* blue */
	pinfo->lcdc.hsync_skew = 0;
}

static int __devexit mdss_edp_remove(struct platform_device *pdev)
{
	struct mdss_edp_drv_pdata *edp_drv = NULL;

	edp_drv = platform_get_drvdata(pdev);

	gpio_free(edp_drv->gpio_panel_en);
	mdss_edp_regulator_off(edp_drv);
	iounmap(edp_drv->base);
	iounmap(edp_drv->mmss_cc_base);
	edp_drv->base = NULL;

	return 0;
}

static int mdss_edp_device_register(struct mdss_edp_drv_pdata *edp_drv)
{
	int ret;
	u32 tmp;

	mdss_edp_edid2pinfo(edp_drv);
	edp_drv->panel_data.panel_info.bl_min = 1;
	edp_drv->panel_data.panel_info.bl_max = 255;
	ret = of_property_read_u32(edp_drv->pdev->dev.of_node,
		"qcom,mdss-brightness-max-level", &tmp);
	edp_drv->panel_data.panel_info.brightness_max =
		(!ret ? tmp : MDSS_MAX_BL_BRIGHTNESS);

	edp_drv->panel_data.panel_info.edp.frame_rate =
				DEFAULT_FRAME_RATE;/* 60 fps */

	edp_drv->panel_data.event_handler = mdss_edp_event_handler;
	edp_drv->panel_data.set_backlight = mdss_edp_set_backlight;

	edp_drv->panel_data.panel_info.cont_splash_enabled =
					edp_drv->cont_splash;

	ret = mdss_register_panel(edp_drv->pdev, &edp_drv->panel_data);
	if (ret) {
		dev_err(&(edp_drv->pdev->dev), "unable to register eDP\n");
		return ret;
	}

	pr_info("%s: eDP initialized\n", __func__);

	return 0;
}

/*
 * Retrieve edp base address
 */
static int mdss_edp_get_base_address(struct mdss_edp_drv_pdata *edp_drv)
{
	struct resource *res;

	res = platform_get_resource_byname(edp_drv->pdev, IORESOURCE_MEM,
			"edp_base");
	if (!res) {
		pr_err("%s: Unable to get the MDSS EDP resources", __func__);
		return -ENOMEM;
	}

	edp_drv->base_size = resource_size(res);
	edp_drv->base = ioremap(res->start, resource_size(res));
	if (!edp_drv->base) {
		pr_err("%s: Unable to remap EDP resources",  __func__);
		return -ENOMEM;
	}

	pr_info("%s: drv=%x base=%x size=%x\n", __func__,
		(int)edp_drv, (int)edp_drv->base, edp_drv->base_size);

	mdss_debug_register_base("edp",
			edp_drv->base, edp_drv->base_size);

	return 0;
}

static int mdss_edp_get_mmss_cc_base_address(struct mdss_edp_drv_pdata
		*edp_drv)
{
	struct resource *res;

	res = platform_get_resource_byname(edp_drv->pdev, IORESOURCE_MEM,
			"mmss_cc_base");
	if (!res) {
		pr_err("%s: Unable to get the MMSS_CC resources", __func__);
		return -ENOMEM;
	}

	edp_drv->mmss_cc_base = ioremap(res->start, resource_size(res));
	if (!edp_drv->mmss_cc_base) {
		pr_err("%s: Unable to remap MMSS_CC resources",  __func__);
		return -ENOMEM;
	}

	return 0;
}

static void mdss_edp_video_ready(struct mdss_edp_drv_pdata *ep)
{
	pr_debug("%s: edp_video_ready\n", __func__);
	complete(&ep->video_comp);
}

static void mdss_edp_idle_patterns_sent(struct mdss_edp_drv_pdata *ep)
{
	pr_debug("%s: idle_patterns_sent\n", __func__);
	complete(&ep->idle_comp);
}

static void mdss_edp_do_link_train(struct mdss_edp_drv_pdata *ep)
{
	if (ep->cont_splash)
		return;

	mdss_edp_link_train(ep);
}

static void mdss_edp_fill_edid_data(struct mdss_edp_drv_pdata *edp_drv)
{
	struct edp_edid *edid = &edp_drv->edid;
	unsigned int res[2];

	edid->id_name[0] = 'A';
	edid->id_name[0] = 'U';
	edid->id_name[0] = 'O';
	edid->id_name[0] = 0;
	edid->id_product = 0x305D;
	edid->version = 1;
	edid->revision = 4;
	edid->ext_block_cnt = 0;
	edid->video_intf = 0x5;
	edid->color_depth = 8;
	edid->dpm = 0;
	edid->color_format = 0;

#if defined(CONFIG_MACH_VIENNA)
	edid->timing[0].pclk = 267000000;

	edid->timing[0].h_addressable = 2560;
	edid->timing[0].h_blank = 164;
	edid->timing[0].h_fporch = 62;
	edid->timing[0].h_sync_pulse = 22;

	edid->timing[0].v_addressable = 1600;
	edid->timing[0].v_blank = 33;
	edid->timing[0].v_fporch = 6;
	edid->timing[0].v_sync_pulse = 6;
#else
	edid->timing[0].pclk = 274000000;

	edid->timing[0].h_addressable = 2560;
	edid->timing[0].h_blank = 170;
	edid->timing[0].h_fporch = 48;
	edid->timing[0].h_sync_pulse = 32;

	edid->timing[0].v_addressable = 1600;
	edid->timing[0].v_blank = 73;
	edid->timing[0].v_fporch = 3;
	edid->timing[0].v_sync_pulse = 6;
#endif

	edid->timing[0].width_mm = 271;
	edid->timing[0].height_mm = 172;
	edid->timing[0].h_border = 0;
	edid->timing[0].v_border = 0;
	edid->timing[0].interlaced = 0;
	edid->timing[0].stereo = 0;
	edid->timing[0].sync_type = 1;
	edid->timing[0].sync_separate = 1;
	edid->timing[0].vsync_pol = 0;
	edid->timing[0].hsync_pol = 0;

	if (!of_property_read_u32_array(edp_drv->pdev->dev.of_node, "qcom,mdss-pan-size", res, 2)) {
		edid->timing[0].width_mm = res[0];
		edid->timing[0].height_mm = res[1];
	}
}

static void mdss_edp_fill_dpcd_data(struct mdss_edp_drv_pdata *edp_drv)
{
	struct dpcd_cap *cap = &edp_drv->dpcd;

	cap->max_lane_count = 4;
	cap->max_link_rate = 10; /* FOR 2.7G  mdss_edp_clk_enable()*/
}

#if defined(CONFIG_EDP_ESD_FUNCTION)
void edp_esd_work_func(struct work_struct *work)
{
	struct fb_info *info = registered_fb[0];
	struct msm_fb_data_type *mfd = (struct msm_fb_data_type *)info->par;
	struct mdss_edp_drv_pdata *edp_drv = NULL;

	edp_drv = container_of(work, struct mdss_edp_drv_pdata, edp_esd_work);

	if (!edp_drv) {
		pr_err("%s: Invalid input data edp_drv", __func__);
		return ;
	}

	if (!mfd) {
		pr_err("%s: Invalid input data mfd", __func__);
		return ;
	}

	if (!edp_power_state) {
		pr_err("%s: edp_power_state is off", __func__);
		return ;
	}

	pr_info("%s start", __func__);

	edp_drv->panel_data.event_handler(&edp_drv->panel_data, MDSS_EVENT_PANEL_OFF, NULL);
	edp_drv->panel_data.event_handler(&edp_drv->panel_data, MDSS_EVENT_UNBLANK, NULL);

	mdss_edp_set_backlight(&edp_drv->panel_data, edp_drv->current_bl);

	pr_info("%s end", __func__);
}
#endif
static int count_recovery = 0;
static int edp_event_thread(void *data)
{
	struct mdss_edp_drv_pdata *ep;
	unsigned long flag;
	u32 todo = 0;

	ep = (struct mdss_edp_drv_pdata *)data;

	pr_info("%s: start\n", __func__);

	while (1) {
		wait_event(ep->event_q, (ep->event_pndx != ep->event_gndx));
		while (1) {
			spin_lock_irqsave(&ep->event_lock, flag);
			if (ep->event_pndx == ep->event_gndx) {
				spin_unlock_irqrestore(&ep->event_lock, flag);
				break;
			}
			todo = ep->event_todo_list[ep->event_gndx];
			ep->event_todo_list[ep->event_gndx++] = 0;
			ep->event_gndx %= HPD_EVENT_MAX;
			spin_unlock_irqrestore(&ep->event_lock, flag);

			pr_info("%s: todo=%x\n", __func__, todo);

			if (todo == 0)
				continue;

			if (todo & EV_EDID_READ)
				mdss_edp_edid_read(ep, 0);

			if (todo & EV_DPCD_CAP_READ)
				mdss_edp_dpcd_cap_read(ep);

			if (todo & EV_DPCD_STATUS_READ)
				mdss_edp_dpcd_status_read(ep);

			if (todo & EV_LINK_TRAIN) {
				if (!edp_power_state) {
					msleep(120);
					if (gpio_get_value(ep->gpio_panel_hpd)) {
						pr_err("%s : hpd detected count_recovery = %d \n", __func__, count_recovery);
						msleep(230); /* NDRA LDI REQUIREMENT  350ms delay*/
						tcon_interanl_clock();
						mdss_edp_do_link_train(ep);
#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
						edp_power_state = 1;
						edp_backlight_enable();
						mdss_edp_set_backlight(&ep->panel_data, ep->current_bl);
						complete(&edp_power_sync);
#endif
					}
					else {
						count_recovery++;
						gpio_set_value(ep->gpio_panel_en, 0);
						msleep(100);
						gpio_set_value(ep->gpio_panel_en, 1);
						pr_err("%s : hpd is not detected, do gpio_panel_en reset = %d \n", __func__, count_recovery);
					}
				}
			}
			if (todo & EV_VIDEO_READY)
				mdss_edp_video_ready(ep);

			if (todo & EV_IDLE_PATTERNS_SENT)
				mdss_edp_idle_patterns_sent(ep);

			if (todo & EV_IDLE_PATTERNS_SENT)
				mdss_edp_idle_patterns_sent(ep);
		}
	}
	return 0;
}

static void edp_send_events(struct mdss_edp_drv_pdata *ep, u32 events)
{
	spin_lock(&ep->event_lock);
	ep->event_todo_list[ep->event_pndx++] = events;
	ep->event_pndx %= HPD_EVENT_MAX;
	wake_up(&ep->event_q);
	spin_unlock(&ep->event_lock);
}

irqreturn_t edp_isr(int irq, void *ptr)
{
	struct mdss_edp_drv_pdata *ep = (struct mdss_edp_drv_pdata *)ptr;
	unsigned char *base = ep->base;
	u32 isr1, isr2, mask1, mask2;
	u32 ack;

	spin_lock(&ep->lock);
	isr1 = edp_read(base + 0x308);
	isr2 = edp_read(base + 0x30c);

	mask1 = isr1 & ep->mask1;
	mask2 = isr2 & ep->mask2;

	isr1 &= ~mask1;	/* remove masks bit */
	isr2 &= ~mask2;

	pr_err("%s: isr=%x mask=%x isr2=%x mask2=%x\n",
			__func__, isr1, mask1, isr2, mask2);

	ack = isr1 & EDP_INTR_STATUS1;
	ack <<= 1;	/* ack bits */
	ack |= mask1;
	edp_write(base + 0x308, ack);

	ack = isr2 & EDP_INTR_STATUS2;
	ack <<= 1;	/* ack bits */
	ack |= mask2;
	edp_write(base + 0x30c, ack);
	spin_unlock(&ep->lock);

	if (isr1 & EDP_INTR_HPD) {
#if defined(CONFIG_EDP_ESD_FUNCTION)
		/* FOR ESD FUNCTION */
		if (edp_esd_power_state)
			schedule_work(&ep->edp_esd_work);
		else
			edp_send_events(ep, EV_LINK_TRAIN);
#else
		isr1 &= ~EDP_INTR_HPD;	/* clear */
		edp_send_events(ep, EV_LINK_TRAIN);
#endif
	}

	if (isr2 & EDP_INTR_READY_FOR_VIDEO)
		edp_send_events(ep, EV_VIDEO_READY);

	if (isr2 & EDP_INTR_IDLE_PATTERNs_SENT)
		edp_send_events(ep, EV_IDLE_PATTERNS_SENT);

	if (isr1 && ep->aux_cmd_busy) {
		/* clear EDP_AUX_TRANS_CTRL */
		edp_write(base + 0x318, 0);
		/* read EDP_INTERRUPT_TRANS_NUM */
		ep->aux_trans_num = edp_read(base + 0x310);

		if (ep->aux_cmd_i2c)
			edp_aux_i2c_handler(ep, isr1);
		else
			edp_aux_native_handler(ep, isr1);
	}

	return IRQ_HANDLED;
}

struct mdss_hw mdss_edp_hw = {
	.hw_ndx = MDSS_HW_EDP,
	.ptr = NULL,
	.irq_handler = edp_isr,
};

static void mdss_edp_irq_enable(struct mdss_edp_drv_pdata *edp_drv)
{
	unsigned long flags;

	spin_lock_irqsave(&edp_drv->lock, flags);
	edp_write(edp_drv->base + 0x308, edp_drv->mask1);
	edp_write(edp_drv->base + 0x30c, edp_drv->mask2);
	spin_unlock_irqrestore(&edp_drv->lock, flags);

	mdss_enable_irq(&mdss_edp_hw);
}

static void mdss_edp_irq_disable(struct mdss_edp_drv_pdata *edp_drv)
{
	unsigned long flags;

	spin_lock_irqsave(&edp_drv->lock, flags);
	edp_write(edp_drv->base + 0x308, 0x0);
	edp_write(edp_drv->base + 0x30c, 0x0);
	spin_unlock_irqrestore(&edp_drv->lock, flags);

	mdss_disable_irq(&mdss_edp_hw);
}

static int mdss_edp_irq_setup(struct mdss_edp_drv_pdata *edp_drv)
{
	int ret = 0;

	edp_drv->gpio_panel_hpd = of_get_named_gpio_flags(
			edp_drv->pdev->dev.of_node, "gpio-panel-hpd", 0,
			&edp_drv->hpd_flags);

	if (!gpio_is_valid(edp_drv->gpio_panel_hpd)) {
		pr_err("%s gpio_panel_hpd %d is not valid ", __func__,
				edp_drv->gpio_panel_hpd);
		return -ENODEV;
	}

	ret = gpio_request(edp_drv->gpio_panel_hpd, "edp_hpd_irq_gpio");
	if (ret) {
		pr_err("%s unable to request gpio_panel_hpd %d", __func__,
				edp_drv->gpio_panel_hpd);
		return -ENODEV;
	}

	ret = gpio_tlmm_config(GPIO_CFG(
					edp_drv->gpio_panel_hpd,
					1,
					GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL,
					GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
	if (ret) {
		pr_err("%s: unable to config tlmm = %d\n", __func__,
				edp_drv->gpio_panel_hpd);
		gpio_free(edp_drv->gpio_panel_hpd);
		return -ENODEV;
	}

	ret = gpio_direction_input(edp_drv->gpio_panel_hpd);
	if (ret) {
		pr_err("%s unable to set direction for gpio_panel_hpd %d",
				__func__, edp_drv->gpio_panel_hpd);
		return -ENODEV;
	}

	mdss_edp_hw.ptr = (void *)(edp_drv);

	if (mdss_register_irq(&mdss_edp_hw))
		pr_err("%s: mdss_register_irq failed.\n", __func__);


	return 0;
}


static void mdss_edp_event_setup(struct mdss_edp_drv_pdata *ep)
{
	init_waitqueue_head(&ep->event_q);
	spin_lock_init(&ep->event_lock);

	kthread_run(edp_event_thread, (void *)ep, "mdss_edp_hpd");
}

static int __devinit mdss_edp_probe(struct platform_device *pdev)
{
	int ret;
	struct mdss_edp_drv_pdata *edp_drv;

#if 0	/*LK GCDB is not used yet */
	struct mdss_panel_intf_desc *idesc = NULL;
#endif
	if (!mdss_is_ready()) {
		pr_err("%s: MDP not probed yet!\n", __func__);
		return -EPROBE_DEFER;
	}

#if 0	/*LK GCDB is not used yet */
	idesc = mdss_panel_get_intf_desc(MDSS_PANEL_INTF_EDP, 0);
	if (!idesc) {
		pr_info("%s: not configured as prim\n", __func__);
		return -ENODEV;
	}
#endif

	edp_drv = devm_kzalloc(&pdev->dev, sizeof(*edp_drv), GFP_KERNEL);
	if (edp_drv == NULL) {
		pr_err("%s: Failed, could not allocate edp_drv", __func__);
		return -ENOMEM;
	}

	pr_info("%s", __func__);
#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
	set_global_ep(edp_drv);
#endif
	edp_drv->pdev = pdev;
	edp_drv->pdev->id = 1;
	edp_drv->clk_on = 0;
	edp_drv->aux_rate = 19200000;
	edp_drv->mask1 = EDP_INTR_MASK1;
	edp_drv->mask2 = EDP_INTR_MASK2;
	mutex_init(&edp_drv->emutex);
	spin_lock_init(&edp_drv->lock);

	ret = mdss_edp_get_base_address(edp_drv);
	if (ret)
		goto probe_err;

	ret = mdss_edp_get_mmss_cc_base_address(edp_drv);
	if (ret)
		goto edp_base_unmap;

	ret = mdss_edp_regulator_init(edp_drv);
	if (ret)
		goto mmss_cc_base_unmap;

	ret = mdss_edp_clk_init(edp_drv);
	if (ret)
		goto edp_clk_deinit;

	ret = mdss_edp_gpio_panel_en(edp_drv);
	if (ret)
		goto edp_clk_deinit;

	ret = mdss_edp_pwm_config(edp_drv);
	if (ret)
		goto edp_free_gpio_panel_en;

	mdss_edp_irq_setup(edp_drv);

	mdss_edp_aux_init(edp_drv);

	mdss_edp_event_setup(edp_drv);

	edp_drv->cont_splash = of_property_read_bool(pdev->dev.of_node,
			"qcom,cont-splash-enabled");

	pr_info("%s:cont_splash=%d\n", __func__, edp_drv->cont_splash);

	/* need mdss clock to receive irq */
	if (!edp_drv->cont_splash)
		mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON, false);

	/* only need aux and ahb clock for aux channel */
	mdss_edp_prepare_aux_clocks(edp_drv);
	mdss_edp_aux_clk_enable(edp_drv);

	if (!edp_drv->cont_splash) {
		mdss_edp_phy_pll_reset(edp_drv);
		mdss_edp_aux_reset(edp_drv);
		mdss_edp_mainlink_reset(edp_drv);
		mdss_edp_phy_power_ctrl(edp_drv, 1);
		mdss_edp_aux_ctrl(edp_drv, 1);
	}

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
	mdss_edp_fill_edid_data(edp_drv);
	mdss_edp_fill_dpcd_data(edp_drv);
	mdss_edp_fill_link_cfg(edp_drv); /* FOR SET TIMMING */
#else
	mdss_edp_irq_enable(edp_drv);

	mdss_edp_edid_read(edp_drv, 0);
	mdss_edp_dpcd_cap_read(edp_drv);
	mdss_edp_fill_link_cfg(edp_drv);

	mdss_edp_irq_disable(edp_drv);
#endif

	if (!edp_drv->cont_splash) {
		mdss_edp_aux_ctrl(edp_drv, 0);
		mdss_edp_phy_power_ctrl(edp_drv, 0);
	}

	mdss_edp_aux_clk_disable(edp_drv);
	mdss_edp_unprepare_aux_clocks(edp_drv);

	if (!edp_drv->cont_splash)
		mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF, false);

	if (edp_drv->cont_splash) { /* vote for clocks */
		mdss_edp_regulator_on(edp_drv);
		mdss_edp_prepare_clocks(edp_drv);
		mdss_edp_clk_enable(edp_drv);
	}

	mdss_edp_device_register(edp_drv);

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
	init_completion(&edp_power_sync);
#endif

#if defined(CONFIG_EDP_ESD_FUNCTION)
	INIT_WORK(&edp_drv->edp_esd_work, edp_esd_work_func);
#endif

#if defined(CONFIG_EDP_TCON_MDNIE)
	init_mdnie_class();
#endif

	pr_info("%s: done\n", __func__);

	return 0;


edp_free_gpio_panel_en:
	gpio_free(edp_drv->gpio_panel_en);
edp_clk_deinit:
	mdss_edp_clk_deinit(edp_drv);
	mdss_edp_regulator_off(edp_drv);
mmss_cc_base_unmap:
	iounmap(edp_drv->mmss_cc_base);
edp_base_unmap:
	iounmap(edp_drv->base);
probe_err:
	return ret;

}

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
static int __init edp_current_boot_mode(char *mode)
{
	/*
	*	1, 2 is recovery booting
	*	0 is normal booting
	*/

        if ((strncmp(mode, "1", 1) == 0)||(strncmp(mode, "2", 1) == 0))
		recovery_mode = 1;
	else
		recovery_mode = 0;

	pr_info("%s %s", __func__, recovery_mode ?
						"recovery" : "normal");
	return 1;
}
__setup("androidboot.boot_recovery=", edp_current_boot_mode);
#endif

static const struct of_device_id msm_mdss_edp_dt_match[] = {
	{.compatible = "qcom,mdss-edp"},
	{}
};
MODULE_DEVICE_TABLE(of, msm_mdss_edp_dt_match);

static struct platform_driver mdss_edp_driver = {
	.probe = mdss_edp_probe,
	.remove = __devexit_p(mdss_edp_remove),
	.shutdown = NULL,
	.driver = {
		.name = "mdss_edp",
		.of_match_table = msm_mdss_edp_dt_match,
	},
};

static int __init mdss_edp_init(void)
{
	int ret;

	ret = platform_driver_register(&mdss_edp_driver);
	if (ret) {
		pr_err("%s driver register failed", __func__);
		return ret;
	}

	return ret;
}
module_init(mdss_edp_init);

static void __exit mdss_edp_driver_cleanup(void)
{
	platform_driver_unregister(&mdss_edp_driver);
}
module_exit(mdss_edp_driver_cleanup);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("eDP controller driver");
