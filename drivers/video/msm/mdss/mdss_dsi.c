/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
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
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>

#include "mdss.h"
#include "mdss_fb.h"
#include "mdss_panel.h"
#include "mdss_dsi.h"
#include "mdss_debug.h"

int contsplash_lkstat = 0;
unsigned int gv_manufacture_id;
extern unsigned int system_rev;

int get_lcd_attached(void);

#if defined (CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL) || \
	defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL) || \
	defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WQXGA_PT_PANEL)  || \
	defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6TNMR7_PT_PANEL)  || \
	defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6E3HA1_PT_PANEL)  || \
	defined (CONFIG_GET_LCD_ATTACHED)
int get_samsung_lcd_attached(void);
int get_lcd_ldi_info(void);

#elif defined (CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)

int get_samsung_lcd_attached(void);
#endif

#if (defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_LDI_EA8061))\
		|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
int get_oled_id(void);
#endif

#if defined(CONFIG_GET_LCD_PCD_DETECTED)
int get_lcd_pcd_detected(void);
#endif

#if defined (CONFIG_FB_MSM_MDSS_DSI_DBG)
void xlog(const char *name, u32 data0, u32 data1, u32 data2, u32 data3, u32 data4, u32 data5);
#endif
#if !defined(CONFIG_FB_MSM_MIPI_JDI_TFT_VIDEO_FULL_HD_PT_PANEL)
extern int mdss_panel_get_dst_fmt(u32 bpp, char mipi_mode, u32 pixel_packing,char *dst_format);
#endif

static int mdss_dsi_regulator_init(struct platform_device *pdev)
{
	int ret = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct dsi_drv_cm_data *dsi_drv = NULL;

	if (!pdev) {
		pr_err("%s: invalid input\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = platform_get_drvdata(pdev);
	if (!ctrl_pdata) {
		pr_err("%s: invalid driver data\n", __func__);
		return -EINVAL;
	}

#if defined(CONFIG_GET_LCD_PCD_DETECTED)
	if (get_lcd_pcd_detected()) {
		pr_err("%s : pcd detected!!\n", __func__);
		return ret;
	}
#endif

	dsi_drv = &(ctrl_pdata->shared_pdata);

	pr_info("%s: vregn(%d)\n", __func__,
		ctrl_pdata->power_data.num_vreg);

	if (ctrl_pdata->power_data.num_vreg > 0) { // ctrl->pdata = 0
		/* vdd, vddio, vdda */
		ret = msm_dss_config_vreg(&pdev->dev,
			ctrl_pdata->power_data.vreg_config,
			ctrl_pdata->power_data.num_vreg, 1);

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL) || defined (CONFIG_FB_MSM_MIPI_MAGNA_OCTA_CMD_HD_PT_PANEL)
		dsi_drv->iovdd_vreg = devm_regulator_get(&pdev->dev, "iovdd");
		if (IS_ERR(dsi_drv->iovdd_vreg)) {
			pr_err("%s: could not get iovddreg, rc=%ld\n",
				__func__, PTR_ERR(dsi_drv->iovdd_vreg));
			return PTR_ERR(dsi_drv->iovdd_vreg);
		} else {
			pr_info("%s: vdd3 - VREG_LVS4 (i/o) init.. \n", __func__);
		}
#elif defined(CONFIG_FB_MSM_MDSS_TC_DSI2LVDS_WXGA_PANEL)
		ctrl_pdata->iovdd_vreg = devm_regulator_get(&pdev->dev, "vddio");
		if (IS_ERR(ctrl_pdata->iovdd_vreg)) {
			pr_err("%s: could not get VDD L5, rc=%ld\n",
				__func__, PTR_ERR(ctrl_pdata->iovdd_vreg));
			return PTR_ERR(ctrl_pdata->iovdd_vreg);
		} else {
			pr_info("%s: VDD L5 - VREG_15 (i/o) init.. \n", __func__);
		}
		regulator_set_voltage(ctrl_pdata->iovdd_vreg, 1200000, 1200000);
#elif defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL)
		ctrl_pdata->lcd_3p0_vreg = devm_regulator_get(&pdev->dev, "max77826_ldo13");

		if (IS_ERR(ctrl_pdata->lcd_3p0_vreg)) {
			pr_err("%s: could not get ldo13(lcd_3p0), rc=%ld\n",
				__func__, PTR_ERR(ctrl_pdata->lcd_3p0_vreg));
			return PTR_ERR(ctrl_pdata->lcd_3p0_vreg);
		} else {
			pr_info("%s: ldo13(lcd_3p0) init.. \n", __func__);
		}
		regulator_set_voltage(ctrl_pdata->lcd_3p0_vreg, 3000000, 3000000);
		usleep_range(5000, 5000);

		if ( system_rev >= 4)
			ctrl_pdata->lcd_1p8_vreg = devm_regulator_get(&pdev->dev, "max77826_ldo6");
		else
			ctrl_pdata->lcd_1p8_vreg = devm_regulator_get(&pdev->dev, "max77826_ldo14");

		if (IS_ERR(ctrl_pdata->lcd_1p8_vreg)) {
			pr_err("%s: could not get (lcd_1p8), rc=%ld\n",
				__func__, PTR_ERR(ctrl_pdata->lcd_1p8_vreg));
			return PTR_ERR(ctrl_pdata->lcd_1p8_vreg);
		} else {
			pr_info("%s: (lcd_1p8) init.. \n", __func__);
		}
		regulator_set_voltage(ctrl_pdata->lcd_1p8_vreg, 1800000, 1800000);
		usleep_range(10000, 10000);
#endif
	}

	return ret;
}

static int mdss_dsi_panel_power_on(struct mdss_panel_data *pdata, int enable)
{
	int ret = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = &pdata->panel_info;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

#if defined(CONFIG_GET_LCD_PCD_DETECTED)
	if (get_lcd_pcd_detected()) {
		pr_err("%s : pcd detected!!\n", __func__);
		return ret;
	}
#endif

	if (pinfo->alpm_event) {
		if (enable && pinfo->alpm_event(CHECK_PREVIOUS_STATUS))
			return 0;
		else if (!enable && pinfo->alpm_event(CHECK_CURRENT_STATUS))
			return 0;

		pr_debug("[ALPM_DEBUG]%s, LDO control, enable : %d\n",
					__func__, enable);
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	pr_info("%s: enable=%d vregn(%d)\n", __func__,
		enable, ctrl_pdata->power_data.num_vreg);

	if (pdata->panel_info.dynamic_switch_pending)
		return 0;

	if (enable) {
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_YOUM_CMD_FULL_HD_PT_PANEL) \
	|| defined(CONFIG_MIPI_LCD_S6E3FA0_FORCE_VIDEO_MODE) \
	|| defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_JS01LTESBM) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WQXGA_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MDSS_S6E8AA0A_HD_PANEL) \
	|| defined(CONFIG_FB_MSM_MDSS_HX8369B_TFT_VIDEO_WVGA_PT_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_S6E88A0_QHD_VIDEO_PT_PANEL)
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			pr_info("%s : Set High LCD Enable GPIO \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
		}
#elif defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			pr_info("%s : Set High LCD Enable GPIO \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
		}
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio2)) {
			pr_info("%s : Set High LCD Enable GPIO2 \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio2), 1);
		}
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6TNMR7_PT_PANEL)
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			pr_info("%s : Set High LCD Enable GPIO (3.3V) \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
		}
		usleep_range(15000, 15000);
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio2)) {
			pr_info("%s : Set High TCON Enable GPIO (1.8V) \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio2), 1);
		}
#elif defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			pr_info("%s : Set High LCD Enable GPIO \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
		}
#elif  defined(CONFIG_FB_MSM_MDSS_SDC_WXGA_PANEL)
		if ((gpio_is_valid(ctrl_pdata->disp_en_gpio)) && (get_lcd_attached() != 0)) {
			pr_info("%s : Set High LCD Enable GPIO SDC \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
		}
#elif defined(CONFIG_FB_MSM_MDSS_TC_DSI2LVDS_WXGA_PANEL)
		regulator_set_optimum_mode(ctrl_pdata->iovdd_vreg, 100000);
		regulator_enable(ctrl_pdata->iovdd_vreg);
		mdelay(1);
		if ((gpio_is_valid(ctrl_pdata->disp_en_gpio)) && (get_lcd_attached() != 0)) {
			gpio_tlmm_config(GPIO_CFG(ctrl_pdata->disp_en_gpio, 0,
				GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
			pr_info("%s : Set High LCD Enable GPIO SDC \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
		}
		mdelay(1);
#elif defined(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			pr_info("%s : Set High LCD Enable GPIO \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
		}
		mdelay(2);
		if (gpio_is_valid(ctrl_pdata->disp_en_vsp_gpio)) {
			pr_info("%s : Set High LCD Enable VSP GPIO \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_vsp_gpio), 1);
		}
		mdelay(2);
		if (gpio_is_valid(ctrl_pdata->disp_en_vsn_gpio)) {
			pr_info("%s : Set High LCD Enable VSN GPIO \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_vsn_gpio), 1);
		}
		mdelay(1);
#elif defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL)
		ret = regulator_set_optimum_mode(ctrl_pdata->lcd_1p8_vreg, 100000);
		if (ret < 0) {
			pr_err("set_optimum_mode lcd_1p8_vreg failed, rc=%d\n", ret);
			return -EINVAL;
		}
		usleep_range(3000, 3000);
		ret = regulator_enable(ctrl_pdata->lcd_1p8_vreg);
		if (ret) {
			pr_err("enable lcd_1p8_vreg failed, rc=%d\n", ret);
			return -ENODEV;
		}
		pr_info("%s : lcd_1p8 regulator enable!!\n", __func__);

		ret = regulator_set_optimum_mode(ctrl_pdata->lcd_3p0_vreg, 100000);
		if (ret < 0) {
			pr_err("set_optimum_mode lcd_3p0_vreg failed, rc=%d\n", ret);
			return -EINVAL;
		}
		usleep_range(3000, 3000);
		ret = regulator_enable(ctrl_pdata->lcd_3p0_vreg);
		if (ret) {
			pr_err("enable lcd_3p0_vreg failed, rc=%d\n", ret);
			return -ENODEV;
		}
		pr_info("%s : lcd_3p0 regulator enable!!\n", __func__);

		usleep_range(5000, 5000);
#endif


		if (ctrl_pdata->power_data.num_vreg > 0) {

		ret = msm_dss_enable_vreg(
			ctrl_pdata->power_data.vreg_config,
			ctrl_pdata->power_data.num_vreg, 1);
		if (ret) {
			pr_err("%s:Failed to enable vregs.rc=%d\n",
				__func__, ret);
					return ret;
		}
			if (ctrl_pdata->panel_extra_power){
				ret = ctrl_pdata->panel_extra_power(pdata,1);

				if (ret) {
					pr_err("%s: Failed to enable extra power.rc=%d\n",
						__func__, ret);
					return ret;
				}
			}
		}
#if defined(CONFIG_FB_MSM_MIPI_JDI_TFT_VIDEO_FULL_HD_PT_PANEL)
		mdelay(20);
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			pr_info("%s : Set High LCD Enable GPIO \n", __func__);
			gpio_tlmm_config(GPIO_CFG(ctrl_pdata->disp_en_gpio, 0,
				GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
		}
		if (gpio_is_valid(ctrl_pdata->bl_on_gpio)) {
			pr_info("%s : Set High Backlight Enable GPIO \n", __func__);
			gpio_set_value((ctrl_pdata->bl_on_gpio), 1);
		}
#endif
		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
						__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
		/*panel reset function moved on lp11 state */
	} else {

		ctrl_pdata->panel_reset(pdata, 0);

		if (ctrl_pdata->power_data.num_vreg > 0) {

			if (ctrl_pdata->panel_extra_power){
				  ret = ctrl_pdata->panel_extra_power(pdata,0);

				if (ret) {
					pr_err("%s: Failed to disable extra power.rc=%d\n",
						__func__, ret);
							return ret;
				}
			}
#if defined(CONFIG_FB_MSM_MDSS_TC_DSI2LVDS_WXGA_PANEL)
			regulator_set_optimum_mode(ctrl_pdata->iovdd_vreg, 100);
			regulator_disable(ctrl_pdata->iovdd_vreg);
			mdelay(1);
#elif defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL)
			if (regulator_is_enabled(ctrl_pdata->lcd_3p0_vreg)) {
				ret = regulator_disable(ctrl_pdata->lcd_3p0_vreg);
				if (ret) {
					pr_err("disable lcd_3p0_vreg failed, rc=%d\n", ret);
					return -ENODEV;
				} else
					pr_info("%s : lcd_3p0 regulator disable!!\n", __func__);
			}
			if (regulator_is_enabled(ctrl_pdata->lcd_1p8_vreg)) {
				ret = regulator_disable(ctrl_pdata->lcd_1p8_vreg);
				if (ret) {
					pr_err("disable lcd_1p8_vreg failed, rc=%d\n", ret);
					return -ENODEV;
				} else
					pr_info("%s : lcd_1p8 regulator disable!!\n", __func__);
			}
#endif

			ret = msm_dss_enable_vreg(
				ctrl_pdata->power_data.vreg_config,
				ctrl_pdata->power_data.num_vreg, 0);
			if (ret) {
				pr_err("%s: Failed to disable vregs.rc=%d\n",
					__func__, ret);
						return ret;
			}

		}
	}

	pr_debug("%s: --\n", __func__);
	return ret;
}

static void mdss_dsi_put_dt_vreg_data(struct device *dev,
	struct dss_module_power *module_power)
{
	if (!module_power) {
		pr_err("%s: invalid input\n", __func__);
		return;
	}

	if (module_power->vreg_config) {
		devm_kfree(dev, module_power->vreg_config);
		module_power->vreg_config = NULL;
	}
	module_power->num_vreg = 0;
}

static int mdss_dsi_get_dt_vreg_data(struct device *dev,
	struct dss_module_power *mp)
{
	int i = 0, rc = 0;
	u32 tmp = 0;
	struct device_node *of_node = NULL, *supply_node = NULL;

	if (!dev || !mp) {
		pr_err("%s: invalid input\n", __func__);
		rc = -EINVAL;
		return rc;
	}

	of_node = dev->of_node;

	mp->num_vreg = 0;
	for_each_child_of_node(of_node, supply_node) {
		if (!strncmp(supply_node->name, "qcom,platform-supply-entry",
						26)) {
#ifdef CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL
			if(!strncmp(supply_node->name, "qcom,platform-supply-entry1", 28)) {
				pr_err("%s : VDD(l22) register skip!! (%s) \n", __func__, supply_node->name);
				continue;
			}
#endif
			++mp->num_vreg;
		}
	}
	if (mp->num_vreg == 0) {
		pr_debug("%s: no vreg\n", __func__);
		goto novreg;
	} else {
		pr_debug("%s: vreg found. count=%d\n", __func__, mp->num_vreg);
	}

	mp->vreg_config = devm_kzalloc(dev, sizeof(struct dss_vreg) *
		mp->num_vreg, GFP_KERNEL);
	if (!mp->vreg_config) {
		pr_err("%s: can't alloc vreg mem\n", __func__);
		rc = -ENOMEM;
		goto error;
	}

	for_each_child_of_node(of_node, supply_node) {
		if (!strncmp(supply_node->name, "qcom,platform-supply-entry",
						26)) {
			const char *st = NULL;
			/* vreg-name */
			rc = of_property_read_string(supply_node,
				"qcom,supply-name", &st);
			if (rc) {
				pr_err("%s: error reading name. rc=%d\n",
					__func__, rc);
				goto error;
			}
			snprintf(mp->vreg_config[i].vreg_name,
				ARRAY_SIZE((mp->vreg_config[i].vreg_name)),
				"%s", st);
#ifdef CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL
			if(!strncmp(mp->vreg_config[i].vreg_name, "vdd", 4)) {
				pr_err("%s : VDD(l22) setting skip!!\n", __func__);
				continue;
			}
#endif

			/* vreg-min-voltage */
			rc = of_property_read_u32(supply_node,
				"qcom,supply-min-voltage", &tmp);
			if (rc) {
				pr_err("%s: error reading min volt. rc=%d\n",
					__func__, rc);
				goto error;
			}
			mp->vreg_config[i].min_voltage = tmp;

			/* vreg-max-voltage */
			rc = of_property_read_u32(supply_node,
				"qcom,supply-max-voltage", &tmp);
			if (rc) {
				pr_err("%s: error reading max volt. rc=%d\n",
					__func__, rc);
				goto error;
			}
			mp->vreg_config[i].max_voltage = tmp;

			/* enable-load */
			rc = of_property_read_u32(supply_node,
				"qcom,supply-enable-load", &tmp);
			if (rc) {
				pr_err("%s: error reading enable load. rc=%d\n",
					__func__, rc);
				goto error;
			}
			mp->vreg_config[i].enable_load = tmp;

			/* disable-load */
			rc = of_property_read_u32(supply_node,
				"qcom,supply-disable-load", &tmp);
			if (rc) {
				pr_err("%s: error reading disable load. rc=%d\n",
					__func__, rc);
				goto error;
			}
			mp->vreg_config[i].disable_load = tmp;

			/* pre-sleep */
			rc = of_property_read_u32(supply_node,
				"qcom,supply-pre-on-sleep", &tmp);
			if (rc) {
				pr_debug("%s: error reading supply pre sleep value. rc=%d\n",
					__func__, rc);
			}
			mp->vreg_config[i].pre_on_sleep = (!rc ? tmp : 0);

			rc = of_property_read_u32(supply_node,
				"qcom,supply-pre-off-sleep", &tmp);
			if (rc) {
				pr_debug("%s: error reading supply pre sleep value. rc=%d\n",
					__func__, rc);
			}
			mp->vreg_config[i].pre_off_sleep = (!rc ? tmp : 0);

			/* post-sleep */
			rc = of_property_read_u32(supply_node,
				"qcom,supply-post-on-sleep", &tmp);
			if (rc) {
				pr_debug("%s: error reading supply post sleep value. rc=%d\n",
					__func__, rc);
			}
			mp->vreg_config[i].post_on_sleep = (!rc ? tmp : 0);

			rc = of_property_read_u32(supply_node,
				"qcom,supply-post-off-sleep", &tmp);
			if (rc) {
				pr_debug("%s: error reading supply post sleep value. rc=%d\n",
					__func__, rc);
			}
			mp->vreg_config[i].post_off_sleep = (!rc ? tmp : 0);

			pr_debug("%s: %s min=%d, max=%d, enable=%d, disable=%d, preonsleep=%d, postonsleep=%d, preoffsleep=%d, postoffsleep=%d\n",
				__func__,
				mp->vreg_config[i].vreg_name,
				mp->vreg_config[i].min_voltage,
				mp->vreg_config[i].max_voltage,
				mp->vreg_config[i].enable_load,
				mp->vreg_config[i].disable_load,
				mp->vreg_config[i].pre_on_sleep,
				mp->vreg_config[i].post_on_sleep,
				mp->vreg_config[i].pre_off_sleep,
				mp->vreg_config[i].post_off_sleep
				);
			++i;
		}
	}

	return rc;

error:
	if(mp){
		if (mp->vreg_config && dev) {
			devm_kfree(dev, mp->vreg_config);
			mp->vreg_config = NULL;
		}
	}
novreg:
	if(mp){
		mp->num_vreg = 0;
	}
	return rc;
}

#define ULPS_REQUEST_BITS 0x001f
#define ULPS_EXIT_BITS	 0x1f00
#define ULPS_LANE_STATUS_BITS 0x1f00
#define CTRL_OFFSET 0xAC
#define STATUS_OFFSET 0xA8
static int mipi_ulps_mode(struct mdss_dsi_ctrl_pdata *ctrl_pdata,int enter)
{
	uint32_t dsi0LaneCtrlReg = MIPI_INP(ctrl_pdata->ctrl_base + CTRL_OFFSET);
	uint32_t dsi0LaneStatusReg = MIPI_INP(ctrl_pdata->ctrl_base + STATUS_OFFSET);

	pr_debug("[ALPM_DEBUG] mipi_ulps_mode++: dsi0LaneStatusReg 0x%x\n", dsi0LaneStatusReg);

	if(enter) //enter into the mode
	{
		MIPI_OUTP(ctrl_pdata->ctrl_base + CTRL_OFFSET, dsi0LaneCtrlReg | ULPS_REQUEST_BITS);
		usleep(1000);
		pr_debug("[ALPM_DEBUG] entering into the ulps mode\n");
	}
	else //exit from the mode
	{
		MIPI_OUTP(ctrl_pdata->ctrl_base + CTRL_OFFSET, dsi0LaneCtrlReg | ULPS_EXIT_BITS);

		pr_debug("[ALPM_DEBUG] exiting from the ulps mode\n");
		usleep(2000);

		//Exit/ request bits clear (requirement)
		dsi0LaneCtrlReg = MIPI_INP(ctrl_pdata->ctrl_base + CTRL_OFFSET);
		dsi0LaneCtrlReg &= ~ULPS_REQUEST_BITS;
		MIPI_OUTP(ctrl_pdata->ctrl_base + CTRL_OFFSET, dsi0LaneCtrlReg);
		dsi0LaneCtrlReg &= ~ULPS_EXIT_BITS;
		MIPI_OUTP(ctrl_pdata->ctrl_base + CTRL_OFFSET, dsi0LaneCtrlReg);
	}
	return true;
}

static int mdss_dsi_get_panel_cfg(char *panel_cfg)
{
	int rc;
	struct mdss_panel_cfg *pan_cfg = NULL;

	if (!panel_cfg)
		return MDSS_PANEL_INTF_INVALID;

	pan_cfg = mdss_panel_intf_type(MDSS_PANEL_INTF_DSI);
	if (IS_ERR(pan_cfg)) {
		return PTR_ERR(pan_cfg);
	} else if (!pan_cfg) {
		panel_cfg[0] = 0;
		return 0;
	}

	pr_info("%s:%d: cfg:[%s]\n", __func__, __LINE__,
		 pan_cfg->arg_cfg);
	rc = strlcpy(panel_cfg, pan_cfg->arg_cfg,
		     sizeof(pan_cfg->arg_cfg));
	return rc;
}

static int mdss_dsi_off(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *panel_info = NULL;
	struct mdss_panel_info *pinfo = &pdata->panel_info;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	if (!pdata->panel_info.panel_power_on) {
		pr_warn("%s:%d Panel already off.\n", __func__, __LINE__);
		return 0;
	}
#if defined (CONFIG_FB_MSM_MDSS_DSI_DBG)
	xlog(__func__,pdata->panel_info.panel_power_on,0,0, 0,0,0);
#endif

	pdata->panel_info.panel_power_on = 0;

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	mutex_lock(&ctrl_pdata->mutex);
	panel_info = &ctrl_pdata->panel_data.panel_info;
	pr_info("%s+: ctrl=%pK ndx=%d\n", __func__,
				ctrl_pdata, ctrl_pdata->ndx);

	if (pinfo->alpm_event && pinfo->alpm_event(CHECK_CURRENT_STATUS))
		mipi_ulps_mode(ctrl_pdata, 1);

	if((pdata->panel_info.type == MIPI_CMD_PANEL) && (ctrl_pdata->ndx == DSI_CTRL_0)
		&& (!ctrl_pdata->shared_pdata.broadcast_enable)) {
		ret = gpio_tlmm_config(GPIO_CFG(
					ctrl_pdata->disp_te_gpio, 0,
					GPIO_CFG_INPUT,
					GPIO_CFG_PULL_DOWN,
					GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);

		if (ret) {
			pr_err("%s: unable to config tlmm = %d\n",
				__func__, ctrl_pdata->disp_te_gpio);
			gpio_free(ctrl_pdata->disp_te_gpio);
			mutex_unlock(&ctrl_pdata->mutex);
			return -ENODEV;
		}
	}
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6TNMR7_PT_PANEL)
	if (pdata->panel_info.type == MIPI_CMD_PANEL)
		mdss_dsi_clk_ctrl(ctrl_pdata, DSI_ALL_CLKS, 1);
#endif

	/* disable DSI controller */
	mdss_dsi_controller_cfg(0, pdata);

	/* disable DSI phy */
	mdss_dsi_phy_disable(ctrl_pdata);

	mdss_dsi_clk_ctrl(ctrl_pdata, DSI_ALL_CLKS, 0);

	ret = mdss_dsi_panel_power_on(pdata, 0);
	if (ret) {
		mutex_unlock(&ctrl_pdata->mutex);
		pr_err("%s: Panel power off failed\n", __func__);
		return ret;
	}

	if (panel_info->dynamic_fps
	    && (panel_info->dfps_update == DFPS_SUSPEND_RESUME_MODE)
	    && (panel_info->new_fps != panel_info->mipi.frame_rate))
		panel_info->mipi.frame_rate = panel_info->new_fps;

	mutex_unlock(&ctrl_pdata->mutex);
	pr_info("%s-:\n", __func__);

	return ret;
}

#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL)
extern int flip;
#endif

static void __mdss_dsi_ctrl_setup(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo;
	struct mipi_panel_info *mipi;
	u32 clk_rate;
	u32 hbp, hfp, vbp, vfp, hspw, vspw, width, height;
	u32 ystride, bpp, data, dst_bpp;
	u32 dummy_xres, dummy_yres;
	u32 hsync_period, vsync_period;

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	pinfo = &pdata->panel_info;

	clk_rate = pdata->panel_info.clk_rate;
	clk_rate = min(clk_rate, pdata->panel_info.clk_max);

	dst_bpp = pdata->panel_info.fbc.enabled ?
		(pdata->panel_info.fbc.target_bpp) : (pinfo->bpp);

	hbp = mult_frac(pdata->panel_info.lcdc.h_back_porch, dst_bpp,
			pdata->panel_info.bpp);
	hfp = mult_frac(pdata->panel_info.lcdc.h_front_porch, dst_bpp,
			pdata->panel_info.bpp);
	vbp = mult_frac(pdata->panel_info.lcdc.v_back_porch, dst_bpp,
			pdata->panel_info.bpp);
	vfp = mult_frac(pdata->panel_info.lcdc.v_front_porch, dst_bpp,
			pdata->panel_info.bpp);
	hspw = mult_frac(pdata->panel_info.lcdc.h_pulse_width, dst_bpp,
			pdata->panel_info.bpp);
	vspw = pdata->panel_info.lcdc.v_pulse_width;
	width = mult_frac(pdata->panel_info.xres, dst_bpp,
			pdata->panel_info.bpp);
	height = pdata->panel_info.yres;

	if (pdata->panel_info.type == MIPI_VIDEO_PANEL) {
		dummy_xres = pdata->panel_info.lcdc.xres_pad;
		dummy_yres = pdata->panel_info.lcdc.yres_pad;
	}

	vsync_period = vspw + vbp + height + dummy_yres + vfp;
	hsync_period = hspw + hbp + width + dummy_xres + hfp;

	mipi = &pdata->panel_info.mipi;
	if (pdata->panel_info.type == MIPI_VIDEO_PANEL) {
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x24,
			((hspw + hbp + width + dummy_xres) << 16 |
			(hspw + hbp)));
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x28,
			((vspw + vbp + height + dummy_yres) << 16 |
			(vspw + vbp)));
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x2C,
				((vsync_period - 1) << 16)
				| (hsync_period - 1));

		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x30, (hspw << 16));
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x34, 0);
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x38, (vspw << 16));

	} else {		/* command mode */
		if (mipi->dst_format == DSI_CMD_DST_FORMAT_RGB888)
			bpp = 3;
		else if (mipi->dst_format == DSI_CMD_DST_FORMAT_RGB666)
			bpp = 3;
		else if (mipi->dst_format == DSI_CMD_DST_FORMAT_RGB565)
			bpp = 2;
		else
			bpp = 3;	/* Default format set to RGB888 */

		ystride = width * bpp + 1;

		/* DSI_COMMAND_MODE_MDP_STREAM_CTRL */
		data = (ystride << 16) | (mipi->vc << 8) | DTYPE_DCS_LWRITE;
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x60, data);
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x58, data);

		/* DSI_COMMAND_MODE_MDP_STREAM_TOTAL */
		data = height << 16 | width;
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x64, data);
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x5C, data);
	}
}

static inline bool __mdss_dsi_ulps_feature_enabled(
	struct mdss_panel_data *pdata)
{
	return pdata->panel_info.ulps_feature_enabled;
}

static int mdss_dsi_ulps_config_sub(struct mdss_dsi_ctrl_pdata *ctrl_pdata,
	int enable)
{
	int ret = 0;
	struct mdss_panel_data *pdata = NULL;
	struct mipi_panel_info *pinfo = NULL;
	u32 lane_status = 0;
	u32 active_lanes = 0;

	if (!ctrl_pdata) {
		pr_err("%s: invalid input\n", __func__);
		return -EINVAL;
	}

	pdata = &ctrl_pdata->panel_data;
	if (!pdata) {
		pr_err("%s: Invalid panel data\n", __func__);
		return -EINVAL;
	}
	pinfo = &pdata->panel_info.mipi;

	if (!__mdss_dsi_ulps_feature_enabled(pdata)) {
		pr_debug("%s: ULPS feature not supported. enable=%d\n",
			__func__, enable);
		return -ENOTSUPP;
	}

	if (enable && !ctrl_pdata->ulps) {
		/* No need to configure ULPS mode when entering suspend state */
		if (!pdata->panel_info.panel_power_on) {
			pr_err("%s: panel off. returning\n", __func__);
			goto error;
		}

		if (__mdss_dsi_clk_enabled(ctrl_pdata, DSI_LINK_CLKS)) {
			pr_err("%s: cannot enter ulps mode if dsi clocks are on\n",
				__func__);
			ret = -EPERM;
			goto error;
		}

		ret = mdss_dsi_clk_ctrl(ctrl_pdata, DSI_ALL_CLKS, 1);
		if (ret) {
			pr_err("%s: Failed to enable clocks. rc=%d\n",
				__func__, ret);
			goto error;
		}

		/*
		 * ULPS Entry Request.
		 * Wait for a short duration to ensure that the lanes
		 * enter ULP state.
		 */
		MIPI_OUTP(ctrl_pdata->ctrl_base + 0x0AC, 0x01F);
		usleep(100);

		/* Check to make sure that all active data lanes are in ULPS */
		if (pinfo->data_lane3)
			active_lanes |= BIT(11);
		if (pinfo->data_lane2)
			active_lanes |= BIT(10);
		if (pinfo->data_lane1)
			active_lanes |= BIT(9);
		if (pinfo->data_lane0)
			active_lanes |= BIT(8);
		active_lanes |= BIT(12); /* clock lane */
		lane_status = MIPI_INP(ctrl_pdata->ctrl_base + 0xA8);
		if (lane_status & active_lanes) {
			pr_err("%s: ULPS entry req failed. Lane status=0x%08x\n",
				__func__, lane_status);
			ret = -EINVAL;
			mdss_dsi_clk_ctrl(ctrl_pdata, DSI_ALL_CLKS, 0);
			goto error;
		}

		/* Enable MMSS DSI Clamps */
		MIPI_OUTP(ctrl_pdata->mmss_misc_io.base + 0x14, 0x3FF);
		MIPI_OUTP(ctrl_pdata->mmss_misc_io.base + 0x14, 0x83FF);

		wmb();

		MIPI_OUTP(ctrl_pdata->mmss_misc_io.base + 0x108, 0x1);
		/* disable DSI controller */
		mdss_dsi_controller_cfg(0, pdata);

		mdss_dsi_clk_ctrl(ctrl_pdata, DSI_ALL_CLKS, 0);
		ctrl_pdata->ulps = true;
	} else if (ctrl_pdata->ulps) {
		ret = mdss_dsi_clk_ctrl(ctrl_pdata, DSI_BUS_CLKS, 1);
		if (ret) {
			pr_err("%s: Failed to enable bus clocks. rc=%d\n",
				__func__, ret);
			goto error;
		}

		MIPI_OUTP(ctrl_pdata->mmss_misc_io.base + 0x108, 0x0);
		mdss_dsi_phy_init(pdata);

		__mdss_dsi_ctrl_setup(pdata);
		mdss_dsi_sw_reset(pdata);
		mdss_dsi_host_init(pdata);
		mdss_dsi_op_mode_config(pdata->panel_info.mipi.mode,
			pdata);

		/*
		 * ULPS Entry Request. This is needed because, after power
		 * collapse and reset, the DSI controller resets back to
		 * idle state and not ULPS.
		 * Wait for a short duration to ensure that the lanes
		 * enter ULP state.
		 */
		MIPI_OUTP(ctrl_pdata->ctrl_base + 0x0AC, 0x01F);
		usleep(100);

		/* Disable MMSS DSI Clamps */
		MIPI_OUTP(ctrl_pdata->mmss_misc_io.base + 0x14, 0x3FF);
		MIPI_OUTP(ctrl_pdata->mmss_misc_io.base + 0x14, 0x0);

		ret = mdss_dsi_clk_ctrl(ctrl_pdata, DSI_LINK_CLKS, 1);
		if (ret) {
			pr_err("%s: Failed to enable link clocks. rc=%d\n",
				__func__, ret);
			mdss_dsi_clk_ctrl(ctrl_pdata, DSI_BUS_CLKS, 0);
			goto error;
		}

		/*
		 * ULPS Exit Request
		 * Hardware requirement is to wait for at least 1ms
		 */
		MIPI_OUTP(ctrl_pdata->ctrl_base + 0x0AC, 0x1F00);
		usleep(1000);
		MIPI_OUTP(ctrl_pdata->ctrl_base + 0x0AC, 0x0);

		/*
		 * Wait for a short duration before enabling
		 * data transmission
		 */
		usleep(100);

		lane_status = MIPI_INP(ctrl_pdata->ctrl_base + 0xA8);
		mdss_dsi_clk_ctrl(ctrl_pdata, DSI_LINK_CLKS, 0);
		mdss_dsi_clk_ctrl(ctrl_pdata, DSI_BUS_CLKS, 0);
		ctrl_pdata->ulps = false;
	}

	pr_debug("%s: DSI lane status = 0x%08x. Ulps %s\n", __func__,
		lane_status, enable ? "enabled" : "disabled");

error:
	return ret;
}
#if !defined(CONFIG_FB_MSM_MIPI_JDI_TFT_VIDEO_FULL_HD_PT_PANEL)
static int mdss_dsi_update_panel_config(struct mdss_dsi_ctrl_pdata *ctrl_pdata,
				int mode)
{
	int ret = 0;
	struct mdss_panel_info *pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (mode == DSI_CMD_MODE) {
		pinfo->mipi.mode = DSI_CMD_MODE;
		pinfo->type = MIPI_CMD_PANEL;
		pinfo->mipi.vsync_enable = 1;
		pinfo->mipi.hw_vsync_mode = 1;
	} else {	/*video mode*/
		pinfo->mipi.mode = DSI_VIDEO_MODE;
		pinfo->type = MIPI_VIDEO_PANEL;
		pinfo->mipi.vsync_enable = 0;
		pinfo->mipi.hw_vsync_mode = 0;
	}

	ctrl_pdata->panel_mode = pinfo->mipi.mode;
	mdss_panel_dt_get_dst_fmt(pinfo->bpp, pinfo->mipi.mode,
			pinfo->mipi.pixel_packing, &(pinfo->mipi.dst_format));
	pinfo->cont_splash_enabled = 0;

	return ret;
}
#endif
static int mdss_dsi_ulps_config(struct mdss_dsi_ctrl_pdata *ctrl,
	int enable)
{
	int rc;
	struct mdss_dsi_ctrl_pdata *mctrl = NULL;

	if (&ctrl->mmss_misc_io == NULL) {
		pr_err("%s: mmss_misc_io is NULL. ULPS not valid\n", __func__);
		return -EINVAL;
	}

	if (mdss_dsi_is_slave_ctrl(ctrl)) {
		mctrl = mdss_dsi_get_master_ctrl();
		if (!mctrl) {
			pr_err("%s: Unable to get master control\n", __func__);
			return -EINVAL;
		}
	}

	if (mctrl) {
		pr_debug("%s: configuring ulps (%s) for master ctrl%d\n",
			__func__, (enable ? "on" : "off"), ctrl->ndx);
		rc = mdss_dsi_ulps_config_sub(mctrl, enable);
		if (rc)
			return rc;
	}

	pr_debug("%s: configuring ulps (%s) for ctrl%d\n",
		__func__, (enable ? "on" : "off"), ctrl->ndx);
	return mdss_dsi_ulps_config_sub(ctrl, enable);
}

#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL)
extern int flip;
#endif

int mdss_dsi_on(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct mdss_panel_info *pinfo;
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6TNMR7_PT_PANEL)
	u32 tmp;
#endif
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	if (pdata->panel_info.panel_power_on) {
		pr_warn("%s:%d Panel already on.\n", __func__, __LINE__);
		return 0;
	}
#if defined (CONFIG_FB_MSM_MDSS_DSI_DBG)
	xlog(__func__,pdata->panel_info.panel_power_on,0,0, 0,0,0);
#endif

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	pr_info("%s+: ctrl=%pK ndx=%d\n",
				__func__, ctrl_pdata, ctrl_pdata->ndx);

	pinfo = &pdata->panel_info;
	mipi = &pdata->panel_info.mipi;

#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL)
	gpio_set_value(ctrl_pdata->lcd_sel_gpio, flip);
	usleep_range(2000, 2000);
	pr_info("%s : flip [%s]\n", __func__, flip ? "CLOSE" : "OPEN");
#endif

	if (pinfo->alpm_event && pinfo->alpm_event(CHECK_PREVIOUS_STATUS))
			mipi_ulps_mode(ctrl_pdata, 0);

	if((pdata->panel_info.type == MIPI_CMD_PANEL) && (ctrl_pdata->ndx == DSI_CTRL_0)) {
		ret = gpio_tlmm_config(GPIO_CFG(
				ctrl_pdata->disp_te_gpio, 1,
				GPIO_CFG_INPUT,
				GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);

		if (ret) {
			pr_err("%s: unable to config tlmm = %d\n",
				__func__, ctrl_pdata->disp_te_gpio);
			gpio_free(ctrl_pdata->disp_te_gpio);
			return -ENODEV;
		} else {
			pr_info("%s: success [disp_te_gpio] gpio_ltmm_config..\n",__func__);
		}
	}

	if(ctrl_pdata->ndx == DSI_CTRL_0) {
		ret = mdss_dsi_panel_power_on(pdata, 1);
		if (ret) {
		pr_err("%s:Panel power on failed. rc=%d\n", __func__, ret);
			return ret;
		}
	}


#if	!defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WQXGA_PT_PANEL) && \
	!defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL)
	if (get_lcd_attached() == 0) {
		pr_err("%s : lcd is not attached..\n",__func__);

		mdss_dsi_panel_power_on(pdata, 0);
		pdata->panel_info.panel_power_on = 0;

		return 0;
	}
#endif

	mdss_dsi_clk_ctrl(ctrl_pdata, DSI_BUS_CLKS, 1);
	pdata->panel_info.panel_power_on = 1;

	mdss_dsi_phy_sw_reset((ctrl_pdata->ctrl_base));
	mdss_dsi_phy_init(pdata);
	mdss_dsi_clk_ctrl(ctrl_pdata, DSI_BUS_CLKS, 0);

	mdss_dsi_clk_ctrl(ctrl_pdata, DSI_ALL_CLKS, 1);

	__mdss_dsi_ctrl_setup(pdata);
	mdss_dsi_sw_reset(pdata);
	mdss_dsi_host_init(pdata);

#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6TNMR7_PT_PANEL)
	/* LP11 */
	tmp = MIPI_INP((ctrl_pdata->ctrl_base) + 0xac);
#if defined(CONFIG_FB_MSM_MDSS_SHARP_HD_PANEL)
	MIPI_OUTP((ctrl_pdata->ctrl_base) + 0xac, tmp & -(1<< 28));
#else
	MIPI_OUTP((ctrl_pdata->ctrl_base) + 0xac, 0x1F << 16);
#endif
	wmb();
#if defined(CONFIG_FB_MSM_MDSS_TC_DSI2LVDS_WXGA_PANEL) || \
	defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL) || \
	defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	mdelay(1);
#else
	msleep(20);
#endif
	/* LP11 */

	ctrl_pdata->panel_reset(pdata, 1);

	MIPI_OUTP((ctrl_pdata->ctrl_base) + 0xac, tmp);

	if (mipi->force_clk_lane_hs) {
		u32 tmp;

		tmp = MIPI_INP((ctrl_pdata->ctrl_base) + 0xac);
		tmp |= (1<<28);
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0xac, tmp);
		wmb();
	}

	if (pdata->panel_info.type == MIPI_CMD_PANEL)
		mdss_dsi_clk_ctrl(ctrl_pdata, DSI_ALL_CLKS, 0);
#else
	ctrl_pdata->panel_reset(pdata, 1);
#endif

	pr_info("%s-:\n", __func__);
	return 0;
}

#if defined(CONFIG_FB_MSM_MDSS_S6E8AA0A_HD_PANEL)
static int mdss_MTP_read(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	int ret=0;

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	ctrl_pdata->mtp(pdata);

	return ret;
}
#endif

static int mdss_dsi_unblank(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	pr_debug("%s+:\n", __func__);

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	mipi  = &pdata->panel_info.mipi;

	if (!(ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT)) {
		if (!pdata->panel_info.dynamic_switch_pending) {
			ret = ctrl_pdata->on(pdata);
			if (ret) {
				pr_err("%s: unable to initialize the panel\n",
							__func__);
				return ret;
			}
		}
		ctrl_pdata->ctrl_state |= CTRL_STATE_PANEL_INIT;
	}

#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL) && \
	!defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6TNMR7_PT_PANEL) && \
	!defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_CMD_HD_PT_PANEL)
	if (pdata->panel_info.type == MIPI_CMD_PANEL) {
		if (mipi->vsync_enable && mipi->hw_vsync_mode
			&& gpio_is_valid(ctrl_pdata->disp_te_gpio)) {
				mdss_dsi_set_tear_on(ctrl_pdata);
		}
	}
#endif

	pr_debug("%s-:\n", __func__);

	return ret;
}

static int mdss_dsi_blank(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	pr_debug("%s+:\n", __func__);

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	mipi = &pdata->panel_info.mipi;

	if (__mdss_dsi_ulps_feature_enabled(pdata) &&
		(ctrl_pdata->ulps)) {
		/* Disable ULPS mode before blanking the panel */
		ret = mdss_dsi_ulps_config(ctrl_pdata, 0);
		if (ret) {
			pr_err("%s: failed to exit ULPS mode. rc=%d\n",
				__func__, ret);
			return ret;
		}
        }
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL) && \
	! defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
	if (pdata->panel_info.type == MIPI_VIDEO_PANEL &&
			ctrl_pdata->off_cmds.link_state == DSI_LP_MODE) {
		mdss_dsi_sw_reset(pdata);
		mdss_dsi_host_init(pdata);
	}
#endif
	mdss_dsi_op_mode_config(DSI_CMD_MODE, pdata);

	if (pdata->panel_info.dynamic_switch_pending) {
		pr_info("%s: switching to %s mode\n", __func__,
			(pdata->panel_info.mipi.mode ? "video" : "command"));
		if (pdata->panel_info.type == MIPI_CMD_PANEL) {
			ctrl_pdata->switch_mode(pdata, DSI_VIDEO_MODE);
		} else if (pdata->panel_info.type == MIPI_VIDEO_PANEL) {
			ctrl_pdata->switch_mode(pdata, DSI_CMD_MODE);
			mdss_dsi_set_tear_off(ctrl_pdata);
		}
	}

#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL) && \
	!defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6TNMR7_PT_PANEL) && \
	!defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_CMD_HD_PT_PANEL)
	if (pdata->panel_info.type == MIPI_CMD_PANEL) {
		if (mipi->vsync_enable && mipi->hw_vsync_mode
			&& gpio_is_valid(ctrl_pdata->disp_te_gpio)) {
			mdss_dsi_set_tear_off(ctrl_pdata);
		}
	}
#endif

	if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
		if (!pdata->panel_info.dynamic_switch_pending) {
			ret = ctrl_pdata->off(pdata);
			if (ret) {
				pr_err("%s: Panel OFF failed\n", __func__);
				return ret;
			}
		}
		ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
	}
	pr_debug("%s-:End\n", __func__);
	return ret;
}

int mdss_dsi_cont_splash_on(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	pr_info("%s:%d DSI on for continuous splash.\n", __func__, __LINE__);

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	mipi = &pdata->panel_info.mipi;

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	pr_debug("%s+: ctrl=%pK ndx=%d\n", __func__,
				ctrl_pdata, ctrl_pdata->ndx);

	WARN((ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT),
		"Incorrect Ctrl state=0x%x\n", ctrl_pdata->ctrl_state);

	mdss_dsi_sw_reset(pdata);
	mdss_dsi_host_init(pdata);
	mdss_dsi_op_mode_config(mipi->mode, pdata);
	pr_debug("%s-:End\n", __func__);
	return ret;
}

static int mdss_dsi_dfps_config(struct mdss_panel_data *pdata, int new_fps)
{
	int rc = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	u32 dsi_ctrl;

	pr_debug("%s+:\n", __func__);

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (!ctrl_pdata->panel_data.panel_info.dynamic_fps) {
		pr_err("%s: Dynamic fps not enabled for this panel\n",
					__func__);
		return -EINVAL;
	}

	if (new_fps !=
		ctrl_pdata->panel_data.panel_info.mipi.frame_rate) {
		if (pdata->panel_info.dfps_update
			== DFPS_IMMEDIATE_PORCH_UPDATE_MODE) {
			u32 hsync_period, vsync_period;
			u32 new_dsi_v_total, current_dsi_v_total;
			vsync_period =
				mdss_panel_get_vtotal(&pdata->panel_info);
			hsync_period =
				mdss_panel_get_htotal(&pdata->panel_info);
			current_dsi_v_total =
				MIPI_INP((ctrl_pdata->ctrl_base) + 0x2C);
			new_dsi_v_total =
				((vsync_period - 1) << 16) | (hsync_period - 1);
			MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x2C,
				(current_dsi_v_total | 0x8000000));
			if (new_dsi_v_total & 0x8000000) {
				MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x2C,
					new_dsi_v_total);
			} else {
				MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x2C,
					(new_dsi_v_total | 0x8000000));
				MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x2C,
					(new_dsi_v_total & 0x7ffffff));
			}
			pdata->panel_info.mipi.frame_rate = new_fps;
		} else {
			rc = mdss_dsi_clk_div_config
				(&ctrl_pdata->panel_data.panel_info, new_fps);
			if (rc) {
				pr_err("%s: unable to initialize the clk dividers\n",
								__func__);
				return rc;
			}
			ctrl_pdata->pclk_rate =
				pdata->panel_info.mipi.dsi_pclk_rate;
			ctrl_pdata->byte_clk_rate =
				pdata->panel_info.clk_rate / 8;

			if (pdata->panel_info.dfps_update
					== DFPS_IMMEDIATE_CLK_UPDATE_MODE) {
				dsi_ctrl = MIPI_INP((ctrl_pdata->ctrl_base) +
						    0x0004);
				pdata->panel_info.mipi.frame_rate = new_fps;
				dsi_ctrl &= ~0x2;
				MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x0004,
								dsi_ctrl);
				mdss_dsi_controller_cfg(true, pdata);
				mdss_dsi_clk_ctrl(ctrl_pdata, DSI_ALL_CLKS, 0);
				mdss_dsi_clk_ctrl(ctrl_pdata, DSI_ALL_CLKS, 1);
				dsi_ctrl |= 0x2;
				MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x0004,
								dsi_ctrl);
			}
		}
	} else {
		pr_debug("%s: Panel is already at this FPS\n", __func__);
	}

	return rc;
}

static int mdss_dsi_ctl_partial_update(struct mdss_panel_data *pdata)
{
	int rc = -EINVAL;
	u32 data;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	/* DSI_COMMAND_MODE_MDP_STREAM_CTRL */
	data = (((pdata->panel_info.roi_w * 3) + 1) << 16) |
			(pdata->panel_info.mipi.vc << 8) | DTYPE_DCS_LWRITE;
	MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x60, data);
	MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x58, data);

	/* DSI_COMMAND_MODE_MDP_STREAM_TOTAL */
	data = pdata->panel_info.roi_h << 16 | pdata->panel_info.roi_w;
	MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x64, data);
	MIPI_OUTP((ctrl_pdata->ctrl_base) + 0x5C, data);

	if (ctrl_pdata->partial_update_fnc)
		rc = ctrl_pdata->partial_update_fnc(pdata);

	if (rc) {
		pr_err("%s: unable to initialize the panel\n",
				__func__);
		return rc;
	}

	return rc;
}

int mdss_dsi_register_recovery_handler(struct mdss_dsi_ctrl_pdata *ctrl,
	struct mdss_panel_recovery *recovery)
{
	mutex_lock(&ctrl->mutex);
	ctrl->recovery = recovery;
	mutex_unlock(&ctrl->mutex);
	return 0;
}

static int mdss_dsi_event_handler(struct mdss_panel_data *pdata,
				  int event, void *arg)
{
	int rc = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	pr_debug("%s+:event=%d\n", __func__, event);

	MDSS_XLOG(event, arg, ctrl_pdata->ndx, 0x3333);
#if defined (CONFIG_FB_MSM_MDSS_DSI_DBG)
	xlog(__func__, event, (int)arg, ctrl_pdata->ndx, 0, 0, 0x3333);
#endif

	switch (event) {
	case MDSS_EVENT_UNBLANK:
		rc = mdss_dsi_on(pdata);
		mdss_dsi_op_mode_config(pdata->panel_info.mipi.mode,
							pdata);
		if (ctrl_pdata->on_cmds.link_state == DSI_LP_MODE)
			rc = mdss_dsi_unblank(pdata);
		break;
	case MDSS_EVENT_PANEL_ON:
		ctrl_pdata->ctrl_state |= CTRL_STATE_MDP_ACTIVE;
		ctrl_pdata->mdp_tg_on = 1;
		if (ctrl_pdata->on_cmds.link_state == DSI_HS_MODE)
			rc = mdss_dsi_unblank(pdata);
		break;
	case MDSS_EVENT_BLANK:
		if (ctrl_pdata->off_cmds.link_state == DSI_HS_MODE)
			rc = mdss_dsi_blank(pdata);
		break;
	case MDSS_EVENT_PANEL_OFF:
		ctrl_pdata->ctrl_state &= ~CTRL_STATE_MDP_ACTIVE;
		if (ctrl_pdata->off_cmds.link_state == DSI_LP_MODE)
			rc = mdss_dsi_blank(pdata);
		rc = mdss_dsi_off(pdata);
		break;
#if defined(CONFIG_FB_MSM_MDSS_S6E8AA0A_HD_PANEL)
	case MTP_READ:
		rc = mdss_MTP_read(pdata);
		break;
#endif
	case MDSS_EVENT_FB_REGISTERED:
		if (ctrl_pdata->registered) {
			pr_debug("%s:event=%d, calling panel registered callback \n",
				 __func__, event);
			rc = ctrl_pdata->registered(pdata);

			/*
			 *  Okay, since framebuffer is registered, display the kernel logo if needed
			*/
#if 0
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL)
			if ((!ctrl_pdata->panel_data.panel_info.cont_splash_enabled)
					&& (ctrl_pdata->panel_data.panel_info.early_lcd_on))
				load_samsung_boot_logo();

#endif
#endif
		}
		break;
	case MDSS_EVENT_CONT_SPLASH_FINISH:

#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_CMD_HD_PT_PANEL)
		if (ctrl_pdata->off_cmds.link_state == DSI_HS_MODE){
			ctrl_pdata->ctrl_state |= CTRL_STATE_PANEL_INIT;
                        rc = mdss_dsi_blank(pdata);
		}
#else
		if (ctrl_pdata->off_cmds.link_state == DSI_LP_MODE)
                    rc = mdss_dsi_blank(pdata);
#endif
		ctrl_pdata->ctrl_state &= ~CTRL_STATE_MDP_ACTIVE;
		rc = mdss_dsi_cont_splash_on(pdata);
		break;
	case MDSS_EVENT_PANEL_CLK_CTRL:
#ifdef DSI_CLK_DEBUG
		pr_err("[QCT_TEST] %s : ndx(%d) arg(%d) ++\n",
			__func__, ctrl_pdata->ndx, (int)arg);
#endif
		mdss_dsi_clk_req(ctrl_pdata, (int)arg);
#ifdef DSI_CLK_DEBUG
		pr_err("[QCT_TEST] %s : ndx(%d) arg(%d) --\n",
			__func__, ctrl_pdata->ndx, (int)arg);
#endif
		break;
	case MDSS_EVENT_DSI_CMDLIST_KOFF:
		mdss_dsi_cmdlist_commit(ctrl_pdata, 1);
		break;
	case MDSS_EVENT_PANEL_UPDATE_FPS:
		if (arg != NULL) {
			rc = mdss_dsi_dfps_config(pdata, (int)arg);
			pr_debug("%s:update fps to = %d\n",
				__func__, (int)arg);
		}
		break;
	case MDSS_EVENT_CONT_SPLASH_BEGIN:
		if (ctrl_pdata->off_cmds.link_state == DSI_HS_MODE) {
			/* Panel is Enabled in Bootloader */
			rc = mdss_dsi_blank(pdata);
		}
		break;
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL)
	case MDSS_EVENT_FIRST_FRAME_UPDATE:
		pr_info("MDSS_FIRST_FRAME_UPDATE\n");
#if !defined(CONFIG_FB_MSM_MDSS_SDC_WXGA_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_TC_DSI2LVDS_WXGA_PANEL)\
		&& !defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_CPT_QHD_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_S6E8AA0A_HD_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_HX8369B_TFT_VIDEO_WVGA_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MIPI_S6E88A0_QHD_VIDEO_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MIPI_JDI_TFT_VIDEO_FULL_HD_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_SHARP_HD_PANEL)\
		&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
		ctrl_pdata->mdp_tg_on = 1;
	/*Event is send only if cont_splash feature is enabled */
		if (ctrl_pdata->off_cmds.link_state == DSI_HS_MODE) {
			/* Panel is Enabled in Bootloader */
			ctrl_pdata->ctrl_state |= CTRL_STATE_PANEL_INIT;
			rc = mdss_dsi_blank(pdata);
		}
#endif
		break;
#endif
	case MDSS_EVENT_ENABLE_PARTIAL_UPDATE:
		rc = mdss_dsi_ctl_partial_update(pdata);
		break;
	case MDSS_EVENT_DSI_ULPS_CTRL:
		rc = mdss_dsi_ulps_config(ctrl_pdata, (int)arg);
		break;
	case MDSS_EVENT_REGISTER_RECOVERY_HANDLER:
		rc = mdss_dsi_register_recovery_handler(ctrl_pdata,
			(struct mdss_panel_recovery *)arg);
		break;
#if !defined(CONFIG_FB_MSM_MIPI_JDI_TFT_VIDEO_FULL_HD_PT_PANEL)
	case MDSS_EVENT_DSI_DYNAMIC_SWITCH:
		rc = mdss_dsi_update_panel_config(ctrl_pdata,
					(int)(unsigned long) arg);
		break;
#endif
	default:
		if(ctrl_pdata->event_handler)
			rc = ctrl_pdata->event_handler(event);
		else
			pr_err("%s: unhandled event=%d\n", __func__, event);
		break;
	}
	pr_debug("%s-:event=%d, rc=%d\n", __func__, event, rc);
	return rc;
}

static struct device_node *mdss_dsi_pref_prim_panel(
		struct platform_device *pdev)
{
	struct device_node *dsi_pan_node = NULL;

	pr_debug("%s:%d: Select primary panel from dt\n",
					__func__, __LINE__);

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
	if ( !get_lcd_ldi_info()){/* MAGNA_PANEL */
		dsi_pan_node = of_parse_phandle(pdev->dev.of_node,
						"qcom,dsi-pref-prim-pan-magna", 0);
	} else /* SLSI_PANEL */
#elif (defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_LDI_EA8061))\
		|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
	if (get_oled_id() == 0x0){  /*magna*/
		dsi_pan_node = of_parse_phandle(pdev->dev.of_node,
						"qcom,dsi-pref-prim-pan2", 0);
	} else
#endif
	{
		dsi_pan_node = of_parse_phandle(pdev->dev.of_node,
						"qcom,dsi-pref-prim-pan", 0);
	}

	if (!dsi_pan_node)
		pr_err("%s:can't find panel phandle\n", __func__);

	return dsi_pan_node;
}

/**
 * mdss_dsi_find_panel_of_node(): find device node of dsi panel
 * @pdev: platform_device of the dsi ctrl node
 * @panel_cfg: string containing intf specific config data
 *
 * Function finds the panel device node using the interface
 * specific configuration data. This configuration data is
 * could be derived from the result of bootloader's GCDB
 * panel detection mechanism. If such config data doesn't
 * exist then this panel returns the default panel configured
 * in the device tree.
 *
 * returns pointer to panel node on success, NULL on error.
 */
static struct device_node *mdss_dsi_find_panel_of_node(
		struct platform_device *pdev, char *panel_cfg)
{
	struct device_node *dsi_pan_node = NULL;

	dsi_pan_node = mdss_dsi_pref_prim_panel(pdev);

	return dsi_pan_node;
}

struct mutex dual_clk_lock;

static int __devinit mdss_dsi_ctrl_probe(struct platform_device *pdev)
{
	int rc = 0;
	u32 index;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct device_node *dsi_pan_node = NULL;
	char panel_cfg[MDSS_MAX_PANEL_LEN];
	const char *ctrl_name;
	bool cmd_cfg_cont_splash = true;

	if (!mdss_is_ready()) {
		pr_err("%s: MDP not probed yet!\n", __func__);
		return -EPROBE_DEFER;
	}

	if (!pdev->dev.of_node) {
		pr_err("DSI driver only supports device tree probe\n");
		return -ENOTSUPP;
	}
#if !defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION) && \
	!defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WQXGA_PT_PANEL) && \
	!defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL) && \
	!defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
	if (get_lcd_attached() == 0) {
		pr_err("%s : lcd is not attached..\n",__func__);
		return -ENODEV;
	}
#endif

	ctrl_pdata = platform_get_drvdata(pdev);
	if (!ctrl_pdata) {
		ctrl_pdata = devm_kzalloc(&pdev->dev,
					  sizeof(struct mdss_dsi_ctrl_pdata),
					  GFP_KERNEL);
		if (!ctrl_pdata) {
			pr_err("%s: FAILED: cannot alloc dsi ctrl\n",
			       __func__);
			rc = -ENOMEM;
			goto error_no_mem;
		}
		platform_set_drvdata(pdev, ctrl_pdata);
	}

	ctrl_pdata->mdss_util = mdss_get_util_intf();
	if (ctrl_pdata->mdss_util == NULL) {
		pr_err("Failed to get mdss utility functions\n");
		rc = -ENODEV;
		goto error_no_mem;
	}

	ctrl_name = of_get_property(pdev->dev.of_node, "label", NULL);
	if (!ctrl_name)
		pr_info("%s:%d, DSI Ctrl name not specified\n",
			__func__, __LINE__);
	else
		pr_info("%s: DSI Ctrl name = %s\n",
			__func__, ctrl_name);

	rc = of_property_read_u32(pdev->dev.of_node,
				  "cell-index", &index);
	if (rc) {
		dev_err(&pdev->dev,
			"%s: Cell-index not specified, rc=%d\n",
			__func__, rc);
		goto error_no_mem;
	}

	if (index == 0)
		pdev->id = 1;
	else
		pdev->id = 2;

	if (index == 0)
		mutex_init(&dual_clk_lock);

	rc = of_platform_populate(pdev->dev.of_node,
				  NULL, NULL, &pdev->dev);
	if (rc) {
		dev_err(&pdev->dev,
			"%s: failed to add child nodes, rc=%d\n",
			__func__, rc);
		goto error_no_mem;
	}

	/* Parse the regulator information */
	rc = mdss_dsi_get_dt_vreg_data(&pdev->dev,
				       &ctrl_pdata->power_data);
	if (rc) {
		pr_err("%s: failed to get vreg data from dt. rc=%d\n",
		       __func__, rc);
		goto error_vreg;
	}

	/* DSI panels can be different between controllers */
	rc = mdss_dsi_get_panel_cfg(panel_cfg);
	if (!rc)
		/* dsi panel cfg not present */
		pr_warn("%s:%d:dsi specific cfg not present\n",
			__func__, __LINE__);

	/* find panel device node */
	dsi_pan_node = mdss_dsi_find_panel_of_node(pdev, panel_cfg);
	if (!dsi_pan_node) {
		pr_err("%s: can't find panel node %s\n", __func__, panel_cfg);
		goto error_pan_node;
	}

	cmd_cfg_cont_splash = mdss_panel_get_boot_cfg() ? true : false;

	rc = mdss_dsi_panel_init(dsi_pan_node, ctrl_pdata, cmd_cfg_cont_splash);
	if (rc) {
		pr_err("%s: dsi panel init failed\n", __func__);
		goto error_pan_node;
	}

	rc = dsi_panel_device_register(dsi_pan_node, ctrl_pdata);
	if (rc) {
		pr_err("%s: dsi panel dev reg failed\n", __func__);
		goto error_pan_node;
	}

	pr_info("%s: Dsi Ctrl->%d initialized\n", __func__, index);

	return 0;

error_pan_node:
	of_node_put(dsi_pan_node);
error_vreg:
	mdss_dsi_put_dt_vreg_data(&pdev->dev, &ctrl_pdata->power_data);
error_no_mem:
	devm_kfree(&pdev->dev, ctrl_pdata);

	return rc;
}

static int __devexit mdss_dsi_ctrl_remove(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = platform_get_drvdata(pdev);

	if (!ctrl_pdata) {
		pr_err("%s: no driver data\n", __func__);
		return -ENODEV;
	}

	if (msm_dss_config_vreg(&pdev->dev,
			ctrl_pdata->power_data.vreg_config,
			ctrl_pdata->power_data.num_vreg, 1) < 0)
		pr_err("%s: failed to de-init vregs\n", __func__);
	mdss_dsi_put_dt_vreg_data(&pdev->dev, &ctrl_pdata->power_data);
	mfd = platform_get_drvdata(pdev);
	msm_dss_iounmap(&ctrl_pdata->mmss_misc_io);
	msm_dss_iounmap(&ctrl_pdata->phy_io);
	msm_dss_iounmap(&ctrl_pdata->ctrl_io);
	return 0;
}

struct device dsi_dev;

int mdss_dsi_retrieve_ctrl_resources(struct platform_device *pdev, int mode,
			struct mdss_dsi_ctrl_pdata *ctrl)
{
	int rc = 0;
	u32 index;

	rc = of_property_read_u32(pdev->dev.of_node, "cell-index", &index);
	if (rc) {
		dev_err(&pdev->dev,
			"%s: Cell-index not specified, rc=%d\n",
						__func__, rc);
		return rc;
	}

	if (index == 0) {
		if (mode != DISPLAY_1) {
			pr_err("%s:%d Panel->Ctrl mapping is wrong",
				       __func__, __LINE__);
			return -EPERM;
		}
	} else if (index == 1) {
		if (mode != DISPLAY_2) {
			pr_err("%s:%d Panel->Ctrl mapping is wrong",
				       __func__, __LINE__);
			return -EPERM;
		}
	} else {
		pr_err("%s:%d Unknown Ctrl mapped to panel",
			       __func__, __LINE__);
		return -EPERM;
	}

	rc = msm_dss_ioremap_byname(pdev, &ctrl->ctrl_io, "dsi_ctrl");
	if (rc) {
		pr_err("%s:%d unable to remap dsi ctrl resources",
			       __func__, __LINE__);
		return rc;
	}

	ctrl->ctrl_base = ctrl->ctrl_io.base;
	ctrl->reg_size = ctrl->ctrl_io.len;

	rc = msm_dss_ioremap_byname(pdev, &ctrl->phy_io, "dsi_phy");
	if (rc) {
		pr_err("%s:%d unable to remap dsi phy resources",
			       __func__, __LINE__);
		return rc;
	}

	pr_info("%s: ctrl_base=%pK ctrl_size=%x phy_base=%pK phy_size=%x\n",
		__func__, ctrl->ctrl_base, ctrl->reg_size, ctrl->phy_io.base,
		ctrl->phy_io.len);

	rc = msm_dss_ioremap_byname(pdev, &ctrl->mmss_misc_io,
		"mmss_misc_phys");
	if (rc) {
		pr_debug("%s:%d mmss_misc IO remap failed\n",
			__func__, __LINE__);
	}

	return 0;
}

#ifdef DEBUG_LDI_STATUS
int read_ldi_status(void);
#endif

void mdss_dsi_dump_power_clk(struct mdss_panel_data *pdata, int flag) {
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
	u8 rc, te_count = 0;
	u8 te_max = 250;
#endif
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
		return;

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)

	if (pdata->panel_info.type == MIPI_CMD_PANEL) {
		pr_info(" ============ waiting for TE ============\n");
		for (te_count = 0 ; te_count < te_max ; te_count++)
		{
			rc = gpio_get_value(ctrl_pdata->disp_te_gpio);
			if(rc != 0)
			{
				pr_info("%s: gpio_get_value(ctrl_pdata->disp_te_gpio) =%d\n",
					__func__, rc);
				break;
			}

			udelay(80);
		}
	}
#endif

	pr_info(" ============ dump power & clk start ============\n");
	if ((ctrl_pdata->shared_pdata).vdd_vreg)
		pr_info("vdd_vreg : %d\n", regulator_is_enabled((ctrl_pdata->shared_pdata).vdd_vreg));
	if ((ctrl_pdata->shared_pdata).vdd_io_vreg)
		pr_info("vdd_io_vreg : %d\n", regulator_is_enabled((ctrl_pdata->shared_pdata).vdd_io_vreg));
	if ((ctrl_pdata->shared_pdata).vdda_vreg)
		pr_info("vdda_vreg : %d\n", regulator_is_enabled((ctrl_pdata->shared_pdata).vdda_vreg));
#if 0
	clock_debug_print_clock2(ctrl_pdata->pixel_clk);
	clock_debug_print_clock2(ctrl_pdata->byte_clk);
	clock_debug_print_clock2(ctrl_pdata->esc_clk);
#endif
	pr_info("%s: ctrl ndx=%d clk_cnt=%d\n",
			__func__, ctrl_pdata->ndx, ctrl_pdata->clk_cnt);
	pr_info(" ============ dump power & clk end ============\n");
	pr_info(" === check manufacture ID cf) EVT0 0xXXXX0X / EVT1 0xXXXX2X ===\n");
	pr_info(" Current LDI manufacture ID = 0x%x	\n", gv_manufacture_id);
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)

	if (pdata->panel_info.type == MIPI_CMD_PANEL) {
		if(te_count == te_max)
		{
			pr_info("LDI doesn't generate TE/ manufacture ID = 0x%x", gv_manufacture_id);
#ifdef DEBUG_LDI_STATUS
			if(flag)
			{
				if(read_ldi_status())
				pr_err("%s : Can not read LDI status\n",__func__);
			}
#endif
			panic("LDI doesn't generate TE/ manufacture ID = 0x%x", gv_manufacture_id);
		}
	}

#endif
}

enum of_gpio_flags test_flags;
int dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	struct mipi_panel_info *mipi;
	int rc, broard_cast;
	int  i, len;
	struct device_node *dsi_ctrl_np = NULL;
	struct platform_device *ctrl_pdev = NULL;
	const char *data;
	struct mdss_panel_info *pinfo = &(ctrl_pdata->panel_data.panel_info);

	pr_info("%s : ++ \n",__func__);

	mipi  = &(pinfo->mipi);

	pinfo->type =
		((mipi->mode == DSI_VIDEO_MODE)
			? MIPI_VIDEO_PANEL : MIPI_CMD_PANEL);

	rc = mdss_dsi_clk_div_config(pinfo, mipi->frame_rate);
	if (rc) {
		pr_err("%s: unable to initialize the clk dividers\n", __func__);
		return rc;
	}

	dsi_ctrl_np = of_parse_phandle(pan_node,
				"qcom,mdss-dsi-panel-controller", 0);
	if (!dsi_ctrl_np) {
		pr_err("%s: Dsi controller node not initialized\n", __func__);
		return -EPROBE_DEFER;
	}

	ctrl_pdev = of_find_device_by_node(dsi_ctrl_np);
	ctrl_pdata = platform_get_drvdata(ctrl_pdev);
	if (!ctrl_pdata) {
		pr_err("%s: no dsi ctrl driver data\n", __func__);
		return -EINVAL;
	}

	rc = mdss_dsi_regulator_init(ctrl_pdev);
	if (rc) {
		pr_err("%s: failed to init regulator, rc=%d\n",
						__func__, rc);
		return rc;
	}
#if defined(CONFIG_FB_MSM_MDSS_S6E8AA0A_HD_PANEL) || defined(CONFIG_FB_MSM_MDSS_SHARP_HD_PANEL)
	data = of_get_property(ctrl_pdev->dev.of_node,
		"qcom,platform-strength-ctrl", &len);
	if ((!data) || (len != 2)) {
		pr_err("%s:%d, Unable to read Phy Strength ctrl settings",
			__func__, __LINE__);
		return -EINVAL;
	}
	pinfo->mipi.dsi_phy_db.strength[0] = data[0];
	pinfo->mipi.dsi_phy_db.strength[1] = data[1];

	data = of_get_property(ctrl_pdev->dev.of_node,
		"qcom,platform-regulator-settings", &len);
	if ((!data) || (len != 7)) {
		pr_err("%s:%d, Unable to read Phy regulator settings",
			__func__, __LINE__);
		return -EINVAL;
	}
	for (i = 0; i < len; i++) {
		pinfo->mipi.dsi_phy_db.regulator[i]
			= data[i];
	}
#endif
	rc = of_property_read_u32(pan_node,
					  "qcom,mdss-pan-broadcast-mode",&broard_cast);
	ctrl_pdata->shared_pdata.broadcast_enable = (!rc ? broard_cast : 0);

	pr_info("%s:%d, broadcast (%d)",__func__, __LINE__,
		ctrl_pdata->shared_pdata.broadcast_enable);

	if (pinfo->pdest == DISPLAY_1) {
		data = of_get_property(ctrl_pdev->dev.of_node,
			"qcom,platform-bist-ctrl", &len);
		if ((!data) || (len != 6)) {
			pr_err("%s:%d, Unable to read Phy Bist Ctrl settings",
				__func__, __LINE__);
			return -EINVAL;
		}
		for (i = 0; i < len; i++) {
			pinfo->mipi.dsi_phy_db.bistctrl[i]
				= data[i];
		}

		data = of_get_property(ctrl_pdev->dev.of_node,
			"qcom,platform-lane-config", &len);
		if ((!data) || (len != 45)) {
			pr_err("%s:%d, Unable to read Phy lane configure settings",
				__func__, __LINE__);
			return -EINVAL;
		}
		for (i = 0; i < len; i++) {
			pinfo->mipi.dsi_phy_db.lanecfg[i] =
				data[i];
		}
	}
/*
	ctrl_pdata->shared_pdata.broadcast_enable = of_property_read_bool(
		pan_node, "qcom,mdss-dsi-panel-broadcast-mode");
*/

	pinfo->panel_max_fps = mdss_panel_get_framerate(pinfo);
	pinfo->panel_max_vtotal = mdss_panel_get_vtotal(pinfo);

#if 1
	ctrl_pdata->disp_en_gpio = of_get_named_gpio(pan_node,
		"qcom,enable-gpio", 0);
#else
	ctrl_pdata->disp_en_gpio = of_get_named_gpio(ctrl_pdev->dev.of_node,
		"qcom,platform-enable-gpio", 0);
#endif
	pr_err("%s:%d, Disp_en_gpio (%d)",__func__, __LINE__,ctrl_pdata->disp_en_gpio );

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WQXGA_PT_PANEL)
	if (get_lcd_attached() == 0) {
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			pr_info("%s : Set Low LCD Enable GPIO \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
		}
	}
#endif

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_err("%s:%d, Disp_en gpio not specified\n",
						__func__, __LINE__);
	} else {
		if (ctrl_pdata->panel_data.panel_info.pdest == DISPLAY_1) {
			rc = gpio_request(ctrl_pdata->disp_en_gpio, "disp_enable");
			if (rc) {
					pr_err("request disp_en gpio failed, rc=%d\n",
				       rc);
				gpio_free(ctrl_pdata->disp_en_gpio);
				return -ENODEV;
	}
#if defined(CONFIG_FB_MSM_MDSS_S6E8AA0A_HD_PANEL) \
	|| defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_JDI_TFT_VIDEO_FULL_HD_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
			else {
				rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->disp_en_gpio, 0,
							GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_8MA),
							GPIO_CFG_ENABLE);
				if (rc)
					pr_err("request disp_en_gpio  failed, rc=%d\n",rc);
			}
#endif
		}
	}
#if defined (CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
	if (get_lcd_attached() == 0) {
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			pr_info("%s : Set Low LCD Enable GPIO \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
		}
	}
#endif

#if defined(CONFIG_FB_MSM_MDSS_HX8369B_TFT_VIDEO_WVGA_PT_PANEL) || defined(CONFIG_FB_MSM_MIPI_JDI_TFT_VIDEO_FULL_HD_PT_PANEL)
	ctrl_pdata->bl_on_gpio = of_get_named_gpio(pan_node,
						     "qcom,bl-ctrl-gpio", 0);
	if (!gpio_is_valid(ctrl_pdata->bl_on_gpio)) {
		pr_err("%s:%dbl_on_gpio gpio not specified\n",
						__func__, __LINE__);
	} else {
		rc = gpio_request(ctrl_pdata->bl_on_gpio, "backlight_enable");
		if (rc) {
			pr_err("request bl_on_gpio   failed, rc=%d\n",rc);
			gpio_free(ctrl_pdata->bl_on_gpio);
		}else {
			rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->bl_on_gpio, 0,
						GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_8MA),
						GPIO_CFG_ENABLE);
			if (rc)
			pr_err("request BL ON  gpio failed, rc=%d\n",rc);
		}
	}

#endif
#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MIPI_JDI_TFT_VIDEO_FULL_HD_PT_PANEL)
	ctrl_pdata->disp_en_gpio2 = of_get_named_gpio(pan_node,
						 "qcom,enable-gpio2", 0);

	pr_err("%s:%d, Disp_en_gpio2 (%d)",__func__, __LINE__,ctrl_pdata->disp_en_gpio2 );

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio2)) {
		pr_err("%s:%d, Disp_en gpio2 not specified\n",
						__func__, __LINE__);
	} else {
		if (ctrl_pdata->panel_data.panel_info.pdest == DISPLAY_1) {
			rc = gpio_request(ctrl_pdata->disp_en_gpio2, "disp_enable2");
			if (rc) {
					pr_err("request disp_en gpio2 failed, rc=%d\n",
					   rc);
				gpio_free(ctrl_pdata->disp_en_gpio2);
			}
		}
	}
#endif
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6TNMR7_PT_PANEL)
	ctrl_pdata->disp_en_gpio2 = of_get_named_gpio(pan_node,
						 "qcom,enable-gpio2", 0);

	pr_err("%s:%d, Disp_en_gpio2 (%d)",__func__, __LINE__,ctrl_pdata->disp_en_gpio2 );

	ctrl_pdata->tcon_ready_gpio = of_get_named_gpio(pan_node,
						 "qcom,tcon-ready-gpio", 0);

	pr_err("%s:%d, tcon_ready gpio (%d)",__func__, __LINE__,ctrl_pdata->tcon_ready_gpio );
	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio2)) {
		pr_err("%s:%d, Disp_en gpio2 not specified\n",
						__func__, __LINE__);
	} else {
		if (ctrl_pdata->panel_data.panel_info.pdest == DISPLAY_1) {
			rc = gpio_request(ctrl_pdata->disp_en_gpio2, "disp_enable2");
			if (rc) {
					pr_err("request disp_en gpio2 failed, rc=%d\n",
					   rc);
				gpio_free(ctrl_pdata->disp_en_gpio2);
			}
		}
	}
	if (!gpio_is_valid(ctrl_pdata->tcon_ready_gpio)) {
		pr_err("%s:%d, tcon_ready gpio not specified\n",
						__func__, __LINE__);
	} else {
		if (ctrl_pdata->panel_data.panel_info.pdest == DISPLAY_2) {
			rc = gpio_request(ctrl_pdata->tcon_ready_gpio, "tcon_ready");
			if (rc) {
					pr_err("request tcon_ready gpio failed, rc=%d\n",
					   rc);
				gpio_free(ctrl_pdata->tcon_ready_gpio);
			}
		}
	}
#endif
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
	ctrl_pdata->lcd_crack_det= of_get_named_gpio(pan_node,"qcom,lcd-crack-det-gpio", 0);
	if (gpio_is_valid(ctrl_pdata->lcd_crack_det)) {
		rc = gpio_request(ctrl_pdata->lcd_crack_det, "lcd_crack_det");
		if (rc) {
			pr_err("request lcd_crack_det gpio failed, rc=%d\n",rc);
			gpio_free(ctrl_pdata->lcd_crack_det);
			return -ENODEV;
		}
		rc = gpio_tlmm_config(GPIO_CFG(
				ctrl_pdata->lcd_crack_det, 1,
				GPIO_CFG_INPUT,
				GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);

		if (rc) {
			pr_err("%s: unable to config tlmm = %d\n",__func__, ctrl_pdata->lcd_crack_det);
			gpio_free(ctrl_pdata->lcd_crack_det);
			return -ENODEV;
		}

		rc = gpio_direction_input(ctrl_pdata->lcd_crack_det);
		if (rc) {
			pr_err("set_direction for disp_en gpio failed, rc=%d\n",rc);
			gpio_free(ctrl_pdata->lcd_crack_det);
			return -ENODEV;
		}
		pr_debug("%s: lcd_crack_det=%d\n", __func__, ctrl_pdata->lcd_crack_det);
	} else {
		pr_err("%s:%d, lcd_crack_det gpio not specified\n",__func__, __LINE__);
	}

	ctrl_pdata->expander_enble_gpio= of_get_named_gpio(pan_node,"qcom,expander-enable-gpio", 0);
	if (gpio_is_valid(ctrl_pdata->expander_enble_gpio))
		pr_err("%s:%d, expander_enble_gpio gpio not specified\n",__func__, __LINE__);
#endif
#if defined(CONFIG_FB_MSM_MDSS_SHARP_HD_PANEL)
	ctrl_pdata->bl_on_gpio = of_get_named_gpio(ctrl_pdev->dev.of_node,
						     "qcom,bl-on-gpio", 0);
	if (!gpio_is_valid(ctrl_pdata->bl_on_gpio)) {
		pr_err("%s:%dbl_on_gpio gpio not specified\n",
						__func__, __LINE__);
	} else {

		rc = gpio_request(ctrl_pdata->bl_on_gpio, "backlight_enable");
		if (rc) {
			pr_err("request bl_on_gpio   failed, rc=%d\n",
			       rc);
			gpio_free(ctrl_pdata->bl_on_gpio);
		}else {
			rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->bl_on_gpio, 0,
						GPIO_CFG_OUTPUT,GPIO_CFG_PULL_UP,GPIO_CFG_8MA),
						GPIO_CFG_ENABLE);
			if (rc)
			pr_err("request BL ON  gpio failed, rc=%d\n",rc);
		}
	}
	ctrl_pdata->disp_en_gpio_p = of_get_named_gpio(ctrl_pdev->dev.of_node,
						     "qcom,disp-on-gpio-p", 0);
	rc = gpio_request(ctrl_pdata->disp_en_gpio_p, "disp_en_gpio_p");
	if (rc) {
		pr_err("request disp_en_gpio_p gpio failed, rc=%d\n",
			rc);
		gpio_free(ctrl_pdata->disp_en_gpio_p);

	}else{
		rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->disp_en_gpio_p, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_PULL_UP,GPIO_CFG_8MA),
					GPIO_CFG_ENABLE);
		if (rc)
		pr_err("request disp_en_gpio_p  failed, rc=%d\n",rc);
	}
		ctrl_pdata->disp_en_gpio_n = of_get_named_gpio(ctrl_pdev->dev.of_node,
						     "qcom,disp-on-gpio-n", 0);
	rc = gpio_request(ctrl_pdata->disp_en_gpio_n, "disp_en_gpio_n");
	if (rc) {
		pr_err("request disp_en_gpio_n gpio failed, rc=%d\n",
			rc);
		gpio_free(ctrl_pdata->disp_en_gpio_n);

	}else{
		rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->disp_en_gpio_n, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_PULL_UP,GPIO_CFG_8MA),
					GPIO_CFG_ENABLE);
		if (rc)
		pr_err("request disp_en_gpio_n failed, rc=%d\n",rc);
	}
#endif
#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_WXGA_PT_DUAL_PANEL)
	ctrl_pdata->lcd_sel_gpio = of_get_named_gpio(pan_node, "qcom,lcd-sel-gpio", 0);
	if (!gpio_is_valid(ctrl_pdata->lcd_sel_gpio)) {
		pr_err("%s:%d, lcd_sel_gpio not specified\n", __func__, __LINE__);
	} else {
		rc = gpio_request(ctrl_pdata->lcd_sel_gpio, "lcd_sel");
		if (rc) {
			pr_err("request lcd_sel gpio failed, rc=%d\n", rc);
			gpio_free(ctrl_pdata->lcd_sel_gpio);
			return -ENODEV;
		}
		pr_info("%s: lcd_sel_gpio = %d\n", __func__, ctrl_pdata->lcd_sel_gpio);

		rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->lcd_sel_gpio, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_8MA),
					GPIO_CFG_ENABLE);

		if (rc) {
			pr_err("%s: unable to lcd_sel config tlmm = %d\n",
				__func__, ctrl_pdata->lcd_sel_gpio);
			gpio_free(ctrl_pdata->lcd_sel_gpio);
			return -ENODEV;
		}
/*
		rc = gpio_direction_output(ctrl_pdata->lcd_sel_gpio, 1);
		if (rc) {
			pr_err("set_direction for lcd_sel gpio failed, rc=%d\n",
			       rc);
			gpio_free(ctrl_pdata->lcd_sel_gpio);
			return -ENODEV;
		}
*/
	}
#endif
#if defined(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
	ctrl_pdata->disp_en_vsp_gpio = of_get_named_gpio(pan_node, "qcom,enable-vsp-gpio", 0);
	ctrl_pdata->disp_en_vsn_gpio = of_get_named_gpio(pan_node, "qcom,enable-vsn-gpio", 0);

	pr_err("%s:%d, Disp_en_vsp_gpio (%d)",__func__, __LINE__,ctrl_pdata->disp_en_vsp_gpio );
	pr_err("%s:%d, Disp_en_vsn_gpio (%d)",__func__, __LINE__,ctrl_pdata->disp_en_vsn_gpio );

	if (!gpio_is_valid(ctrl_pdata->disp_en_vsp_gpio)) {
		pr_err("%s:%d, Disp_en vsp gpio not specified\n",
						__func__, __LINE__);
	} else {
		if (ctrl_pdata->panel_data.panel_info.pdest == DISPLAY_1) {
			rc = gpio_request(ctrl_pdata->disp_en_vsp_gpio, "disp_vsp_enable");
			if (rc) {
					pr_err("request disp_vsp_en gpio failed, rc=%d\n",
					   rc);
				gpio_free(ctrl_pdata->disp_en_vsp_gpio);
			}
		}
	}

	if (!gpio_is_valid(ctrl_pdata->disp_en_vsn_gpio)) {
		pr_err("%s:%d, Disp_en vsn gpio not specified\n",
						__func__, __LINE__);
	} else {
		if (ctrl_pdata->panel_data.panel_info.pdest == DISPLAY_1) {
			rc = gpio_request(ctrl_pdata->disp_en_vsn_gpio, "disp_vsn_enable");
			if (rc) {
					pr_err("request disp_vsn_en gpio failed, rc=%d\n",
					   rc);
				gpio_free(ctrl_pdata->disp_en_vsn_gpio);
			}
		}
	}
#endif

	if (pinfo->type == MIPI_CMD_PANEL) {
		ctrl_pdata->disp_te_gpio = of_get_named_gpio(pan_node,
							     "qcom,te-gpio", 0);

		pr_err("%s:%d, Disp_te_gpio (%d)",__func__, __LINE__,ctrl_pdata->disp_te_gpio );

		if (gpio_is_valid(ctrl_pdata->disp_te_gpio) &&
						pinfo->type == MIPI_CMD_PANEL &&
							pinfo->pdest == DISPLAY_1) {
			rc = gpio_request(ctrl_pdata->disp_te_gpio, "disp_te");
			if (rc) {
				pr_err("request TE gpio failed, rc=%d\n",
				       rc);
				gpio_free(ctrl_pdata->disp_te_gpio);
				return -ENODEV;
			}
			rc = gpio_tlmm_config(GPIO_CFG(
					ctrl_pdata->disp_te_gpio, 1,
					GPIO_CFG_INPUT,
					GPIO_CFG_PULL_DOWN,
					GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);

			if (rc) {
				pr_err("%s: unable to config tlmm = %d\n",
					__func__, ctrl_pdata->disp_te_gpio);
				gpio_free(ctrl_pdata->disp_te_gpio);
				return -ENODEV;
			}

			rc = gpio_direction_input(ctrl_pdata->disp_te_gpio);
			if (rc) {
				pr_err("set_direction for disp_en gpio failed, rc=%d\n",
				       rc);
				gpio_free(ctrl_pdata->disp_te_gpio);
				return -ENODEV;
			}
			pr_debug("%s: te_gpio=%d\n", __func__,
						ctrl_pdata->disp_te_gpio);
		} else {
			pr_err("%s:%d, Disp_te gpio not specified\n",
							__func__, __LINE__);
		}
	}

#if defined(CONFIG_FB_MSM_MDSS_SHARP_HD_PANEL)
		ctrl_pdata->rst_gpio = of_get_named_gpio(ctrl_pdev->dev.of_node,
						 "qcom,platform-reset-gpio", 0);
#else
	ctrl_pdata->rst_gpio = of_get_named_gpio(pan_node,
						 "qcom,rst-gpio", 0);
#endif

	pr_err("%s:%d, Disp_rst_gpio (%d)",__func__, __LINE__,ctrl_pdata->rst_gpio );

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_err("%s:%d, Disp_reset gpio not specified\n",
						__func__, __LINE__);
	} else {
		if (ctrl_pdata->panel_data.panel_info.pdest == DISPLAY_1) {
			rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
			if (rc) {
				pr_err("request reset gpio failed, rc=%d\n",
					rc);
				gpio_free(ctrl_pdata->rst_gpio);
				if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
					gpio_free(ctrl_pdata->disp_en_gpio);
#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
				if (gpio_is_valid(ctrl_pdata->disp_en_gpio2))
					gpio_free(ctrl_pdata->disp_en_gpio2);
#elif defined(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
				if (gpio_is_valid(ctrl_pdata->disp_en_vsp_gpio))
					gpio_free(ctrl_pdata->disp_en_vsp_gpio);
				if (gpio_is_valid(ctrl_pdata->disp_en_vsn_gpio))
					gpio_free(ctrl_pdata->disp_en_vsn_gpio);
#endif
#if defined(CONFIG_FB_MSM_MIPI_JDI_TFT_VIDEO_FULL_HD_PT_PANEL)
				if (gpio_is_valid(ctrl_pdata->bl_on_gpio))
					gpio_free(ctrl_pdata->bl_on_gpio);
#endif
				return -ENODEV;
			}
		}
	}

	if (pinfo->mode_gpio_state != MODE_GPIO_NOT_VALID) {

		ctrl_pdata->mode_gpio = of_get_named_gpio(
					ctrl_pdev->dev.of_node,
					"qcom,platform-mode-gpio", 0);
		if (!gpio_is_valid(ctrl_pdata->mode_gpio)) {
			pr_info("%s:%d, mode gpio not specified\n",
							__func__, __LINE__);
		} else {
			rc = gpio_request(ctrl_pdata->mode_gpio, "panel_mode");
			if (rc) {
				pr_err("request panel mode gpio failed,rc=%d\n",
									rc);
				gpio_free(ctrl_pdata->mode_gpio);
				if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
					gpio_free(ctrl_pdata->disp_en_gpio);
#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
				if (gpio_is_valid(ctrl_pdata->disp_en_gpio2))
					gpio_free(ctrl_pdata->disp_en_gpio2);
#elif defined(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
				if (gpio_is_valid(ctrl_pdata->disp_en_vsp_gpio))
					gpio_free(ctrl_pdata->disp_en_vsp_gpio);
				if (gpio_is_valid(ctrl_pdata->disp_en_vsn_gpio))
					gpio_free(ctrl_pdata->disp_en_vsn_gpio);
#endif
#if defined(CONFIG_FB_MSM_MIPI_JDI_TFT_VIDEO_FULL_HD_PT_PANEL)
				if (gpio_is_valid(ctrl_pdata->bl_on_gpio))
					gpio_free(ctrl_pdata->bl_on_gpio);
#endif
				if (gpio_is_valid(ctrl_pdata->rst_gpio))
					gpio_free(ctrl_pdata->rst_gpio);
				if (gpio_is_valid(ctrl_pdata->disp_te_gpio))
					gpio_free(ctrl_pdata->disp_te_gpio);
				return -ENODEV;
			}
		}
	}

	if (mdss_dsi_clk_init(ctrl_pdev, ctrl_pdata)) {
		pr_err("%s: unable to initialize Dsi ctrl clks\n", __func__);
		return -EPERM;
	}

	if (mdss_dsi_retrieve_ctrl_resources(ctrl_pdev,
					     pinfo->pdest,
					     ctrl_pdata)) {
		pr_err("%s: unable to get Dsi controller res\n", __func__);
		return -EPERM;
	}

	ctrl_pdata->panel_data.event_handler = mdss_dsi_event_handler;

	if (ctrl_pdata->status_mode == ESD_REG)
		ctrl_pdata->check_status = mdss_dsi_reg_status_check;
	else if (ctrl_pdata->status_mode == ESD_BTA)
		ctrl_pdata->check_status = mdss_dsi_bta_status_check;


	if (ctrl_pdata->status_mode == ESD_MAX) {
		pr_err("%s: Using default BTA for ESD check\n", __func__);
		ctrl_pdata->check_status = mdss_dsi_bta_status_check;
	}

#if !(defined(CONFIG_FB_MSM_MDSS_TC_DSI2LVDS_WXGA_PANEL) || defined(CONFIG_BACKLIGHT_IC_KTD2801))
	if (ctrl_pdata->bklt_ctrl == BL_PWM)
		mdss_dsi_panel_pwm_cfg(ctrl_pdata);
#endif

	mdss_dsi_ctrl_init(ctrl_pdata);
	/*
	 * register in mdp driver
	 */

	ctrl_pdata->pclk_rate = mipi->dsi_pclk_rate;
	ctrl_pdata->byte_clk_rate = pinfo->clk_rate / 8;
	pr_debug("%s: pclk=%d, bclk=%d\n", __func__,
			ctrl_pdata->pclk_rate, ctrl_pdata->byte_clk_rate);

	ctrl_pdata->ctrl_state = CTRL_STATE_UNKNOWN;

	if (pinfo->cont_splash_enabled) {
		pr_info("%s : splash enabled..panel_power_on (1)\n", __func__);
		pinfo->panel_power_on = 1;
		if(ctrl_pdata->ndx == DSI_CTRL_0) {
			rc = mdss_dsi_panel_power_on(&(ctrl_pdata->panel_data), 1);
			if (rc) {
				pr_err("%s: Panel power on failed\n", __func__);
				return rc;
			}
		}

		mdss_dsi_clk_ctrl(ctrl_pdata, DSI_ALL_CLKS, 1);
#if (defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL) || \
	defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL))
		ctrl_pdata->ctrl_state |= (CTRL_STATE_PANEL_INIT | CTRL_STATE_MDP_ACTIVE);
#else
		ctrl_pdata->ctrl_state |= CTRL_STATE_MDP_ACTIVE;
#endif
		ctrl_pdata->mdp_tg_on = 1;
	} else {
		pr_info("%s : splash disabled..panel_power_on (0)\n", __func__);
		pinfo->panel_power_on = 0;
	}

	rc = mdss_register_panel(ctrl_pdev, &(ctrl_pdata->panel_data));
	if (rc) {
		pr_err("%s: unable to register MIPI DSI panel\n", __func__);
		if (ctrl_pdata->rst_gpio)
			gpio_free(ctrl_pdata->rst_gpio);
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
			gpio_free(ctrl_pdata->disp_en_gpio);
#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio2))
			gpio_free(ctrl_pdata->disp_en_gpio2);
#elif defined(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
		if (gpio_is_valid(ctrl_pdata->disp_en_vsp_gpio))
			gpio_free(ctrl_pdata->disp_en_vsp_gpio);
		if (gpio_is_valid(ctrl_pdata->disp_en_vsn_gpio))
			gpio_free(ctrl_pdata->disp_en_vsn_gpio);
#endif
#if defined(CONFIG_FB_MSM_MIPI_JDI_TFT_VIDEO_FULL_HD_PT_PANEL)
		if (gpio_is_valid(ctrl_pdata->bl_on_gpio))
			gpio_free(ctrl_pdata->bl_on_gpio);
#endif
		return rc;
	}

	if (pinfo->pdest == DISPLAY_1) {
		mdss_debug_register_base("dsi0",
			ctrl_pdata->ctrl_base, ctrl_pdata->reg_size);
		ctrl_pdata->ndx = 0;
	} else {
		mdss_debug_register_base("dsi1",
			ctrl_pdata->ctrl_base, ctrl_pdata->reg_size);
		ctrl_pdata->ndx = 1;
	}

	pr_info("%s: Panel data initialized\n", __func__);
	return 0;
}

static const struct of_device_id mdss_dsi_ctrl_dt_match[] = {
	{.compatible = "qcom,mdss-dsi-ctrl"},
	{}
};
MODULE_DEVICE_TABLE(of, mdss_dsi_ctrl_dt_match);

static struct platform_driver mdss_dsi_ctrl_driver = {
	.probe = mdss_dsi_ctrl_probe,
	.remove = __devexit_p(mdss_dsi_ctrl_remove),
	.shutdown = NULL,
	.driver = {
		.name = "mdss_dsi_ctrl",
		.of_match_table = mdss_dsi_ctrl_dt_match,
	},
};

static int mdss_dsi_register_driver(void)
{
	return platform_driver_register(&mdss_dsi_ctrl_driver);
}

static int __init mdss_dsi_driver_init(void)
{
	int ret;

	pr_info("%s ++ \n",__func__);
	ret = mdss_dsi_register_driver();
	if (ret) {
		pr_err("mdss_dsi_register_driver() failed!\n");
		return ret;
	}
	pr_info("%s -- \n",__func__);
	return ret;
}
module_init(mdss_dsi_driver_init);

static void __exit mdss_dsi_driver_cleanup(void)
{
	platform_driver_unregister(&mdss_dsi_ctrl_driver);
}
module_exit(mdss_dsi_driver_cleanup);

int get_lcd_attached(void)
{
#if defined (CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL) || \
	defined (CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL) || \
	defined (CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WQXGA_PT_PANEL) || \
	defined (CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6TNMR7_PT_PANEL) || \
	defined (CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQXGA_S6E3HA1_PT_PANEL) || \
	defined (CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION) || \
	defined (CONFIG_GET_LCD_ATTACHED)
	return get_samsung_lcd_attached();
#else
	return 1;
#endif
}
EXPORT_SYMBOL(get_lcd_attached);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DSI controller driver");
MODULE_AUTHOR("Chandan Uddaraju <chandanu@codeaurora.org>");
