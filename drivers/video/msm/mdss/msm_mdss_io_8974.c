/* Copyright (c) 2012-2015, The Linux Foundation. All rights reserved.
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
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>

#include <mach/clk.h>
#include <mach/msm_iomap.h>
#include <mach/clk-provider.h>

#include "mdss.h"
#include "mdss_dsi.h"
#include "mdss_edp.h"

#define SW_RESET BIT(2)
#define SW_RESET_PLL BIT(0)
#define PWRDN_B BIT(7)

static struct dsi_clk_desc dsi_pclk;

int mdss_dsi_clk_init(struct platform_device *pdev,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	struct device *dev = NULL;
	int rc = 0;

	if (!pdev) {
		pr_err("%s: Invalid pdev\n", __func__);
		goto mdss_dsi_clk_err;
	}

	dev = &pdev->dev;
	ctrl_pdata->mdp_core_clk = clk_get(dev, "mdp_core_clk");
	if (IS_ERR(ctrl_pdata->mdp_core_clk)) {
		rc = PTR_ERR(ctrl_pdata->mdp_core_clk);
		pr_err("%s: Unable to get mdp core clk. rc=%d\n",
			__func__, rc);
		goto mdss_dsi_clk_err;
	}

	ctrl_pdata->ahb_clk = clk_get(dev, "iface_clk");
	if (IS_ERR(ctrl_pdata->ahb_clk)) {
		rc = PTR_ERR(ctrl_pdata->ahb_clk);
		pr_err("%s: Unable to get mdss ahb clk. rc=%d\n",
			__func__, rc);
		goto mdss_dsi_clk_err;
	}

	ctrl_pdata->axi_clk = clk_get(dev, "bus_clk");
	if (IS_ERR(ctrl_pdata->axi_clk)) {
		rc = PTR_ERR(ctrl_pdata->axi_clk);
		pr_err("%s: Unable to get axi bus clk. rc=%d\n",
			__func__, rc);
		goto mdss_dsi_clk_err;
	}

	if ((ctrl_pdata->panel_data.panel_info.type == MIPI_CMD_PANEL) ||
		ctrl_pdata->panel_data.panel_info.mipi.dynamic_switch_enabled) {
		ctrl_pdata->mmss_misc_ahb_clk = clk_get(dev, "core_mmss_clk");
		if (IS_ERR(ctrl_pdata->mmss_misc_ahb_clk)) {
			rc = PTR_ERR(ctrl_pdata->mmss_misc_ahb_clk);
			pr_err("%s: Unable to get mmss misc ahb clk. rc=%d\n",
				__func__, rc);
			goto mdss_dsi_clk_err;
		}
	}

	ctrl_pdata->byte_clk = clk_get(dev, "byte_clk");
	if (IS_ERR(ctrl_pdata->byte_clk)) {
		rc = PTR_ERR(ctrl_pdata->byte_clk);
		pr_err("%s: can't find dsi_byte_clk. rc=%d\n",
			__func__, rc);
		ctrl_pdata->byte_clk = NULL;
		goto mdss_dsi_clk_err;
	}

	ctrl_pdata->pixel_clk = clk_get(dev, "pixel_clk");
	if (IS_ERR(ctrl_pdata->pixel_clk)) {
		rc = PTR_ERR(ctrl_pdata->pixel_clk);
		pr_err("%s: can't find dsi_pixel_clk. rc=%d\n",
			__func__, rc);
		ctrl_pdata->pixel_clk = NULL;
		goto mdss_dsi_clk_err;
	}

	ctrl_pdata->esc_clk = clk_get(dev, "core_clk");
	if (IS_ERR(ctrl_pdata->esc_clk)) {
		rc = PTR_ERR(ctrl_pdata->esc_clk);
		pr_err("%s: can't find dsi_esc_clk. rc=%d\n",
			__func__, rc);
		ctrl_pdata->esc_clk = NULL;
		goto mdss_dsi_clk_err;
	}

mdss_dsi_clk_err:
	if (rc)
		mdss_dsi_clk_deinit(ctrl_pdata);
	return rc;
}

void mdss_dsi_clk_deinit(struct mdss_dsi_ctrl_pdata  *ctrl_pdata)
{
	if (ctrl_pdata->byte_clk)
		clk_put(ctrl_pdata->byte_clk);
	if (ctrl_pdata->esc_clk)
		clk_put(ctrl_pdata->esc_clk);
	if (ctrl_pdata->pixel_clk)
		clk_put(ctrl_pdata->pixel_clk);
	if (ctrl_pdata->mmss_misc_ahb_clk)
		clk_put(ctrl_pdata->mmss_misc_ahb_clk);
	if (ctrl_pdata->axi_clk)
		clk_put(ctrl_pdata->axi_clk);
	if (ctrl_pdata->ahb_clk)
		clk_put(ctrl_pdata->ahb_clk);
	if (ctrl_pdata->mdp_core_clk)
		clk_put(ctrl_pdata->mdp_core_clk);
}

#define PREF_DIV_RATIO 27
struct dsiphy_pll_divider_config pll_divider_config;

int mdss_dsi_clk_div_config(struct mdss_panel_info *panel_info,
			    int frame_rate)
{
	u32 fb_divider, rate, vco;
	u32 div_ratio = 0;
	u32 pll_analog_posDiv = 1;
	u32 h_period, v_period;
	u32 dsi_pclk_rate;
	u8 lanes = 0, bpp;
	struct dsi_clk_mnd_table const *mnd_entry = mnd_table;

	if (panel_info->mipi.data_lane3)
		lanes += 1;
	if (panel_info->mipi.data_lane2)
		lanes += 1;
	if (panel_info->mipi.data_lane1)
		lanes += 1;
	if (panel_info->mipi.data_lane0)
		lanes += 1;

	switch (panel_info->mipi.dst_format) {
	case DSI_CMD_DST_FORMAT_RGB888:
	case DSI_VIDEO_DST_FORMAT_RGB888:
	case DSI_VIDEO_DST_FORMAT_RGB666_LOOSE:
		bpp = 3;
		break;
	case DSI_CMD_DST_FORMAT_RGB565:
	case DSI_VIDEO_DST_FORMAT_RGB565:
		bpp = 2;
		break;
	default:
		bpp = 3;	/* Default format set to RGB888 */
		break;
	}

	h_period = mdss_panel_get_htotal(panel_info);
	v_period = mdss_panel_get_vtotal(panel_info);

	if ((frame_rate !=
	     panel_info->mipi.frame_rate) ||
	    (!panel_info->clk_rate)) {
		h_period += panel_info->lcdc.xres_pad;
		v_period += panel_info->lcdc.yres_pad;

		if (lanes > 0) {
			panel_info->clk_rate =
			((h_period * v_period *
			  frame_rate * bpp * 8)
			   / lanes);
		} else {
			pr_err("%s: forcing mdss_dsi lanes to 1\n", __func__);
			panel_info->clk_rate =
				(h_period * v_period * frame_rate * bpp * 8);
		}
	}
	pll_divider_config.clk_rate = panel_info->clk_rate;


	if (pll_divider_config.clk_rate == 0)
		pll_divider_config.clk_rate = 454000000;

	rate = (pll_divider_config.clk_rate / 2)
			 / 1000000; /* Half Bit Clock In Mhz */

	if (rate < 43) {
		vco = rate * 16;
		div_ratio = 16;
		pll_analog_posDiv = 8;
	} else if (rate < 85) {
		vco = rate * 8;
		div_ratio = 8;
		pll_analog_posDiv = 4;
	} else if (rate < 170) {
		vco = rate * 4;
		div_ratio = 4;
		pll_analog_posDiv = 2;
	} else if (rate < 340) {
		vco = rate * 2;
		div_ratio = 2;
		pll_analog_posDiv = 1;
	} else {
		/* DSI PLL Direct path configuration */
		vco = rate * 1;
		div_ratio = 1;
		pll_analog_posDiv = 1;
	}

	/* find the mnd settings from mnd_table entry */
	for (; mnd_entry < mnd_table + ARRAY_SIZE(mnd_table); ++mnd_entry) {
		if (((mnd_entry->lanes) == lanes) &&
			((mnd_entry->bpp) == bpp))
			break;
	}

	if (mnd_entry == mnd_table + ARRAY_SIZE(mnd_table)) {
		pr_err("%s: requested Lanes, %u & BPP, %u, not supported\n",
			__func__, lanes, bpp);
		return -EINVAL;
	}
	fb_divider = ((vco * PREF_DIV_RATIO) / 27);
	pll_divider_config.fb_divider = fb_divider;
	pll_divider_config.ref_divider_ratio = PREF_DIV_RATIO;
	pll_divider_config.bit_clk_divider = div_ratio;
	pll_divider_config.byte_clk_divider =
			pll_divider_config.bit_clk_divider * 8;
	pll_divider_config.analog_posDiv = pll_analog_posDiv;
	pll_divider_config.digital_posDiv =
			(mnd_entry->pll_digital_posDiv) * div_ratio;

	if ((mnd_entry->pclk_d == 0)
		|| (mnd_entry->pclk_m == 1)) {
		dsi_pclk.mnd_mode = 0;
		dsi_pclk.src = 0x3;
		dsi_pclk.pre_div_func = (mnd_entry->pclk_n - 1);
	} else {
		dsi_pclk.mnd_mode = 2;
		dsi_pclk.src = 0x3;
		dsi_pclk.m = mnd_entry->pclk_m;
		dsi_pclk.n = mnd_entry->pclk_n;
		dsi_pclk.d = mnd_entry->pclk_d;
	}
	dsi_pclk_rate = (((pll_divider_config.clk_rate) * lanes)
				      / (8 * bpp));

	if ((dsi_pclk_rate < 3300000) || (dsi_pclk_rate > 250000000))
		dsi_pclk_rate = 35000000;
	panel_info->mipi.dsi_pclk_rate = dsi_pclk_rate;

	return 0;
}

static int mdss_dsi_bus_clk_start(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;

	pr_debug("%s: ndx=%d\n", __func__, ctrl_pdata->ndx);

	rc = clk_prepare_enable(ctrl_pdata->mdp_core_clk);
	if (rc) {
		pr_err("%s: failed to enable mdp_core_clock. rc=%d\n",
							 __func__, rc);
		goto error;
	}

	rc = clk_prepare_enable(ctrl_pdata->ahb_clk);
	if (rc) {
		pr_err("%s: failed to enable ahb clock. rc=%d\n", __func__, rc);
		clk_disable_unprepare(ctrl_pdata->mdp_core_clk);
		goto error;
	}

	rc = clk_prepare_enable(ctrl_pdata->axi_clk);
	if (rc) {
		pr_err("%s: failed to enable ahb clock. rc=%d\n", __func__, rc);
		clk_disable_unprepare(ctrl_pdata->ahb_clk);
		clk_disable_unprepare(ctrl_pdata->mdp_core_clk);
		goto error;
	}

	if (ctrl_pdata->mmss_misc_ahb_clk) {
		rc = clk_prepare_enable(ctrl_pdata->mmss_misc_ahb_clk);
		if (rc) {
			pr_err("%s: failed to enable mmss misc ahb clk.rc=%d\n",
				__func__, rc);
			clk_disable_unprepare(ctrl_pdata->axi_clk);
			clk_disable_unprepare(ctrl_pdata->ahb_clk);
			clk_disable_unprepare(ctrl_pdata->mdp_core_clk);
			goto error;
		}
	}

error:
	return rc;
}

static void mdss_dsi_bus_clk_stop(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	if (ctrl_pdata->mmss_misc_ahb_clk)
		clk_disable_unprepare(ctrl_pdata->mmss_misc_ahb_clk);
	clk_disable_unprepare(ctrl_pdata->axi_clk);
	clk_disable_unprepare(ctrl_pdata->ahb_clk);
	clk_disable_unprepare(ctrl_pdata->mdp_core_clk);
}

static int mdss_dsi_link_clk_set_rate(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
#if defined(CONFIG_SEC_ATLANTIC_PROJECT) || defined(CONFIG_SEC_PATEK_PROJECT)
	u32 esc_clk_rate = 12000000;
#else
	u32 esc_clk_rate = 19200000;
#endif
	int rc = 0;

	if (!ctrl_pdata) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	if (!ctrl_pdata->panel_data.panel_info.cont_splash_enabled) {
		pr_debug("%s: Set clk rates: pclk=%d, byteclk=%d escclk=%d\n",
			__func__, ctrl_pdata->pclk_rate,
			ctrl_pdata->byte_clk_rate, esc_clk_rate);
		rc = clk_set_rate(ctrl_pdata->esc_clk, esc_clk_rate);
		if (rc) {
			pr_err("%s: dsi_esc_clk - clk_set_rate failed\n",
				__func__);
			goto error;
		}

		rc =  clk_set_rate(ctrl_pdata->byte_clk,
			ctrl_pdata->byte_clk_rate);
		if (rc) {
			pr_err("%s: dsi_byte_clk - clk_set_rate failed\n",
				__func__);
			goto error;
		}

		rc = clk_set_rate(ctrl_pdata->pixel_clk, ctrl_pdata->pclk_rate);
		if (rc) {
			pr_err("%s: dsi_pixel_clk - clk_set_rate failed\n",
				__func__);
			goto error;
		}
	}

error:
	return rc;
}

static int mdss_dsi_link_clk_start(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;

	rc = mdss_dsi_link_clk_set_rate(ctrl_pdata);
	if (rc) {
		pr_err("%s: failed to set clk rates. rc=%d\n",
			__func__, rc);
		goto error;
	}

	rc = clk_prepare_enable(ctrl_pdata->esc_clk);
	if (rc) {
		pr_err("%s: Failed to enable dsi esc clk\n", __func__);
		goto esc_clk_err;
	}

	rc = clk_prepare_enable(ctrl_pdata->byte_clk);
	if (rc) {
		pr_err("%s: Failed to enable dsi byte clk\n", __func__);
		goto byte_clk_err;
	}

	rc = clk_prepare_enable(ctrl_pdata->pixel_clk);
	if (rc) {
		pr_err("%s: Failed to enable dsi pixel clk\n", __func__);
		goto pixel_clk_err;
	}

	ctrl_pdata->mdss_dsi_clk_on = 1;

	return rc;

pixel_clk_err:
	clk_disable_unprepare(ctrl_pdata->byte_clk);
byte_clk_err:
	clk_disable_unprepare(ctrl_pdata->esc_clk);
esc_clk_err:
error:
	return rc;
}

static void mdss_dsi_link_clk_stop(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	if (!ctrl_pdata) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}
	pr_debug("%s: ndx=%d\n", __func__, ctrl_pdata->ndx);

	clk_disable_unprepare(ctrl_pdata->esc_clk);
	clk_disable_unprepare(ctrl_pdata->pixel_clk);
	clk_disable_unprepare(ctrl_pdata->byte_clk);
}

static int __mdss_dsi_update_clk_cnt(u32 *clk_cnt, int enable)
{
	int changed = 0;

	if (enable) {
		if (*clk_cnt == 0)
			changed++;
		(*clk_cnt)++;
	} else {
		if (*clk_cnt != 0) {
			(*clk_cnt)--;
			if (*clk_cnt == 0)
				changed++;
		} else {
			pr_debug("%s: clk cnt already zero\n", __func__);
		}
	}

	return changed;
}

static int mdss_dsi_clk_ctrl_sub(struct mdss_dsi_ctrl_pdata *ctrl,
	u8 clk_type, int enable)
{
	int rc = 0;

	if (!ctrl) {
		pr_err("%s: Invalid arg\n", __func__);
		return -EINVAL;
	}

	pr_debug("%s: ndx=%d clk_type=%08x enable=%d\n", __func__,
		ctrl->ndx, clk_type, enable);

	if (enable) {
		if (clk_type & DSI_BUS_CLKS) {
			rc = mdss_dsi_bus_clk_start(ctrl);
			if (rc) {
				pr_err("Failed to start bus clocks. rc=%d\n",
					rc);
				goto error;
			}
		}
		if (clk_type & DSI_LINK_CLKS) {
			rc = mdss_dsi_link_clk_start(ctrl);
			if (rc) {
				pr_err("Failed to start link clocks. rc=%d\n",
					rc);
				if (clk_type & DSI_BUS_CLKS)
					mdss_dsi_bus_clk_stop(ctrl);
				goto error;
			}
		}
	} else {
		if (clk_type & DSI_LINK_CLKS)
			mdss_dsi_link_clk_stop(ctrl);
		if (clk_type & DSI_BUS_CLKS)
			mdss_dsi_bus_clk_stop(ctrl);
	}

error:
	return rc;
}

static DEFINE_MUTEX(dsi_clk_lock); /* per system */

bool __mdss_dsi_clk_enabled(struct mdss_dsi_ctrl_pdata *ctrl, u8 clk_type)
{
	bool bus_enabled = true;
	bool link_enabled = true;

	mutex_lock(&dsi_clk_lock);
	if (clk_type & DSI_BUS_CLKS)
		bus_enabled = ctrl->bus_clk_cnt ? true : false;
	if (clk_type & DSI_LINK_CLKS)
		link_enabled = ctrl->link_clk_cnt ? true : false;
	mutex_unlock(&dsi_clk_lock);

	return bus_enabled && link_enabled;
}

int mdss_dsi_clk_ctrl(struct mdss_dsi_ctrl_pdata *ctrl,
	u8 clk_type, int enable)
{
	int rc = 0;
	int changed = 0, m_changed = 0;
	struct mdss_dsi_ctrl_pdata *mctrl = NULL;

	if (!ctrl) {
		pr_err("%s: Invalid arg\n", __func__);
		return -EINVAL;
	}

	/*
	 * In broadcast mode, we need to enable clocks for the
	 * master controller as well when enabling clocks for the
	 * slave controller
	 */
	if (mdss_dsi_is_slave_ctrl(ctrl)) {
		mctrl = mdss_dsi_get_master_ctrl();
		if (!mctrl)
			pr_warn("%s: Unable to get master control\n", __func__);
	}

	pr_debug("%s++: ndx=%d clk_type=%d bus_clk_cnt=%d link_clk_cnt=%d",
		__func__, ctrl->ndx, clk_type, ctrl->bus_clk_cnt,
		ctrl->link_clk_cnt);
	pr_debug("%s++: mctrl=%s m_bus_clk_cnt=%d m_link_clk_cnt=%d\n, enable=%d\n",
		__func__, mctrl ? "yes" : "no", mctrl ? mctrl->bus_clk_cnt : -1,
		mctrl ? mctrl->link_clk_cnt : -1, enable);

	mutex_lock(&dsi_clk_lock);
	if (clk_type & DSI_BUS_CLKS) {
		changed = __mdss_dsi_update_clk_cnt(&ctrl->bus_clk_cnt,
			enable);
		if (changed && mctrl)
			m_changed = __mdss_dsi_update_clk_cnt(
				&mctrl->bus_clk_cnt, enable);
	}

	if (clk_type & DSI_LINK_CLKS) {
		changed += __mdss_dsi_update_clk_cnt(&ctrl->link_clk_cnt,
			enable);
		if (changed && mctrl)
			m_changed += __mdss_dsi_update_clk_cnt(
				&mctrl->link_clk_cnt, enable);
	}
#if defined (CONFIG_FB_MSM_MDSS_DSI_DBG)
	xlog(__func__,ctrl->ndx, enable,changed,m_changed,ctrl->bus_clk_cnt,mctrl?mctrl->bus_clk_cnt:0xbbb);
#endif
	if (changed) {
		if (enable && m_changed) {
			rc = mdss_dsi_clk_ctrl_sub(mctrl, clk_type, enable);
			if (rc) {
				pr_err("Failed to start mctrl clocks. rc=%d\n",
					rc);
				goto error_mctrl_start;
			}
		}

		rc = mdss_dsi_clk_ctrl_sub(ctrl, clk_type, enable);
		if (rc) {
			pr_err("Failed to %s ctrl clocks. rc=%d\n",
				(enable ? "start" : "stop"), rc);
			goto error_ctrl;
		}

		if (!enable && m_changed) {
			rc = mdss_dsi_clk_ctrl_sub(mctrl, clk_type, enable);
			if (rc) {
				pr_err("Failed to stop mctrl clocks. rc=%d\n",
					rc);
				goto error_mctrl_stop;
			}
		}
	}
	goto no_error;

error_mctrl_stop:
	mdss_dsi_clk_ctrl_sub(ctrl, clk_type, enable ? 0 : 1);
error_ctrl:
	if (enable && m_changed)
		mdss_dsi_clk_ctrl_sub(mctrl, clk_type, 0);
error_mctrl_start:
	if (clk_type & DSI_BUS_CLKS) {
		if (mctrl)
			__mdss_dsi_update_clk_cnt(&mctrl->bus_clk_cnt,
				enable ? 0 : 1);
		__mdss_dsi_update_clk_cnt(&ctrl->bus_clk_cnt, enable ? 0 : 1);
	}
	if (clk_type & DSI_LINK_CLKS) {
		if (mctrl)
			__mdss_dsi_update_clk_cnt(&mctrl->link_clk_cnt,
				enable ? 0 : 1);
		__mdss_dsi_update_clk_cnt(&ctrl->link_clk_cnt, enable ? 0 : 1);
	}

no_error:
	mutex_unlock(&dsi_clk_lock);
	pr_debug("%s++: ndx=%d clk_type=%d bus_clk_cnt=%d link_clk_cnt=%d changed=%d",
		__func__, ctrl->ndx, clk_type, ctrl->bus_clk_cnt,
		ctrl->link_clk_cnt, changed);
	pr_debug("%s++: mctrl=%s m_bus_clk_cnt=%d m_link_clk_cnt=%d\n, m_changed=%d, enable=%d\n",
		__func__, mctrl ? "yes" : "no", mctrl ? mctrl->bus_clk_cnt : -1,
		mctrl ? mctrl->link_clk_cnt : -1, m_changed, enable);

	return rc;
}

void mdss_dsi_phy_sw_reset(unsigned char *ctrl_base)
{
	/* start phy sw reset */
	MIPI_OUTP(ctrl_base + 0x12c, 0x0001);
	udelay(1000);
	wmb();
	/* end phy sw reset */
	MIPI_OUTP(ctrl_base + 0x12c, 0x0000);
	udelay(100);
	wmb();
}

void mdss_dsi_phy_disable(struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct mdss_dsi_ctrl_pdata *ctrl0 = NULL;

	if (ctrl == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	/*
	 * In dual-dsi configuration, the phy should be disabled for the
	 * first controller only when the second controller is disabled.
	 * This is true regardless of whether broadcast mode is enabled
	 * or not.
	 */
	if ((ctrl->ndx == DSI_CTRL_0) &&
		mdss_dsi_get_ctrl_by_index(DSI_CTRL_1)) {
		pr_debug("%s: Dual dsi detected. skipping config for ctrl%d\n",
			__func__, ctrl->ndx);
		return;
	}

	if (ctrl->ndx == DSI_CTRL_1) {
		ctrl0 = mdss_dsi_get_ctrl_by_index(DSI_CTRL_0);
		if (ctrl0) {
			MIPI_OUTP(ctrl0->phy_io.base + 0x0170, 0x000);
			MIPI_OUTP(ctrl0->phy_io.base + 0x0298, 0x000);
		} else {
			pr_warn("%s: Unable to get control%d\n",
				__func__, DSI_CTRL_0);
		}
	}

	MIPI_OUTP(ctrl->phy_io.base + 0x0170, 0x000);
	MIPI_OUTP(ctrl->phy_io.base + 0x0298, 0x000);

	/*
	 * Wait for the registers writes to complete in order to
	 * ensure that the phy is completely disabled
	 */
	wmb();
}

void mdss_dsi_phy_init(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_phy_ctrl *pd;
	int i, off, ln, offset;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL, *temp_ctrl = NULL;

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	if (!ctrl_pdata) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}
	temp_ctrl = ctrl_pdata;

	pd = &(((ctrl_pdata->panel_data).panel_info.mipi).dsi_phy_db);

	/* Strength ctrl 0 */
	MIPI_OUTP((ctrl_pdata->phy_io.base) + 0x0184, pd->strength[0]);

	/*
	 * Phy regulator ctrl settings.
	 * In dual dsi configuration, the second controller also uses
	 * the regulators of the first controller, irrespective of whether
	 * broadcast mode is enabled or not.
	 */
	if (ctrl_pdata->ndx == DSI_CTRL_1) {
		temp_ctrl = mdss_dsi_get_ctrl_by_index(DSI_CTRL_0);
		if (!temp_ctrl) {
			pr_err("%s: Unable to get master ctrl\n", __func__);
			return;
		}
	}

	/* Regulator ctrl 0 */
	MIPI_OUTP((temp_ctrl->phy_io.base) + 0x280, 0x0);
	/* Regulator ctrl - CAL_PWR_CFG */
	MIPI_OUTP((temp_ctrl->phy_io.base) + 0x298, pd->regulator[6]);

	/* Regulator ctrl - TEST */
	MIPI_OUTP((temp_ctrl->phy_io.base) + 0x294, pd->regulator[5]);
	/* Regulator ctrl 3 */
	MIPI_OUTP((temp_ctrl->phy_io.base) + 0x28c, pd->regulator[3]);
	/* Regulator ctrl 2 */
	MIPI_OUTP((temp_ctrl->phy_io.base) + 0x288, pd->regulator[2]);
	/* Regulator ctrl 1 */
	MIPI_OUTP((temp_ctrl->phy_io.base) + 0x284, pd->regulator[1]);
	/* Regulator ctrl 0 */
	MIPI_OUTP((temp_ctrl->phy_io.base) + 0x280, pd->regulator[0]);
	/* Regulator ctrl 4 */
	MIPI_OUTP((temp_ctrl->phy_io.base) + 0x290, pd->regulator[4]);

	/* LDO ctrl 0 */
	if ((ctrl_pdata->panel_data).panel_info.pdest == DISPLAY_1)
		MIPI_OUTP((ctrl_pdata->phy_io.base) + 0x1dc, 0x00);
	else
		MIPI_OUTP((ctrl_pdata->phy_io.base) + 0x1dc, 0x00);

	off = 0x0140;	/* phy timing ctrl 0 - 11 */
	for (i = 0; i < 12; i++) {
		MIPI_OUTP((ctrl_pdata->phy_io.base) + off, pd->timing[i]);
		wmb();
		off += 4;
	}

	/* MMSS_DSI_0_PHY_DSIPHY_CTRL_1 */
	MIPI_OUTP((ctrl_pdata->phy_io.base) + 0x0174, 0x00);
	/* MMSS_DSI_0_PHY_DSIPHY_CTRL_0 */
	MIPI_OUTP((ctrl_pdata->phy_io.base) + 0x0170, 0x5f);
	wmb();

	/* Strength ctrl 1 */
	MIPI_OUTP((ctrl_pdata->phy_io.base) + 0x0188, pd->strength[1]);
	wmb();

	/* 4 lanes + clk lane configuration */
	/* lane config n * (0 - 4) & DataPath setup */
	for (ln = 0; ln < 5; ln++) {
		off = (ln * 0x40);
		for (i = 0; i < 9; i++) {
			offset = i + (ln * 9);
			MIPI_OUTP((ctrl_pdata->phy_io.base) + off,
							pd->lanecfg[offset]);
			wmb();
			off += 4;
		}
	}

	/* MMSS_DSI_0_PHY_DSIPHY_CTRL_0 */
	MIPI_OUTP((ctrl_pdata->phy_io.base) + 0x0170, 0x5f);
	wmb();

	/* DSI_0_PHY_DSIPHY_GLBL_TEST_CTRL */
	if ((ctrl_pdata->panel_data).panel_info.pdest == DISPLAY_1)
		MIPI_OUTP((ctrl_pdata->phy_io.base) + 0x01d4, 0x01);
	else
		MIPI_OUTP((ctrl_pdata->phy_io.base) + 0x01d4, 0x00);
	wmb();

	off = 0x01b4;	/* phy BIST ctrl 0 - 5 */
	for (i = 0; i < 6; i++) {
		MIPI_OUTP((ctrl_pdata->phy_io.base) + off, pd->bistctrl[i]);
		wmb();
		off += 4;
	}

}

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
/* EDP phy configuration settings */
void mdss_edp_phy_sw_reset(unsigned char *edp_base)
{
	/* phy sw reset */
	edp_write(edp_base + 0x74, 0x100); /* EDP_PHY_CTRL */
	wmb();
	usleep(1);
	edp_write(edp_base + 0x74, 0x000); /* EDP_PHY_CTRL */
	wmb();
	usleep(1);

	/* phy PLL sw reset */
	edp_write(edp_base + 0x74, 0x001); /* EDP_PHY_CTRL */
	wmb();
	usleep(1);
	edp_write(edp_base + 0x74, 0x000); /* EDP_PHY_CTRL */
	wmb();
	usleep(1);
}

void mdss_edp_hw_powerup(unsigned char *edp_base, int enable)
{
	int ret = 0;

	if (enable) {
		/* EDP_PHY_EDPPHY_GLB_PD_CTL */
		edp_write(edp_base + 0x52c, 0x3f);
		/* EDP_PHY_EDPPHY_GLB_CFG */
		edp_write(edp_base + 0x528, 0x1);
		/* EDP_PHY_PLL_UNIPHY_PLL_GLB_CFG */
		edp_write(edp_base + 0x620, 0xf);
		/* EDP_AUX_CTRL */
		ret = edp_read(edp_base + 0x300);
		edp_write(edp_base + 0x300, ret | 0x1);
	} else {
		/* EDP_PHY_EDPPHY_GLB_PD_CTL */
		edp_write(edp_base + 0x52c, 0xc0);
	}
}

void mdss_edp_pll_configure(unsigned char *edp_base, int rate)
{
	if (rate == 810000000) {
		edp_write(edp_base + 0x60c, 0x18);
		edp_write(edp_base + 0x664, 0x5);
		edp_write(edp_base + 0x600, 0x0);
		edp_write(edp_base + 0x638, 0x36);
		edp_write(edp_base + 0x63c, 0x69);
		edp_write(edp_base + 0x640, 0xff);
		edp_write(edp_base + 0x644, 0x2f);
		edp_write(edp_base + 0x648, 0x0);
		edp_write(edp_base + 0x66c, 0x0a);
		edp_write(edp_base + 0x674, 0x01);
		edp_write(edp_base + 0x684, 0x5a);
		edp_write(edp_base + 0x688, 0x0);
		edp_write(edp_base + 0x68c, 0x60);
		edp_write(edp_base + 0x690, 0x0);
		edp_write(edp_base + 0x694, 0x2a);
		edp_write(edp_base + 0x698, 0x3);
		edp_write(edp_base + 0x65c, 0x10);
		edp_write(edp_base + 0x660, 0x1a);
		edp_write(edp_base + 0x604, 0x0);
		edp_write(edp_base + 0x624, 0x0);
		edp_write(edp_base + 0x628, 0x0);

		edp_write(edp_base + 0x620, 0x1);
		edp_write(edp_base + 0x620, 0x5);
		edp_write(edp_base + 0x620, 0x7);
		edp_write(edp_base + 0x620, 0xf);
	} else if (rate >= 268500000) {
		edp_write(edp_base + 0x664, 0x5); /* UNIPHY_PLL_LKDET_CFG2 */
		edp_write(edp_base + 0x600, 0x1); /* UNIPHY_PLL_REFCLK_CFG */
		edp_write(edp_base + 0x638, 0x36); /* UNIPHY_PLL_SDM_CFG0 */
		edp_write(edp_base + 0x63c, 0x62); /* UNIPHY_PLL_SDM_CFG1 */
		edp_write(edp_base + 0x640, 0x0); /* UNIPHY_PLL_SDM_CFG2 */
		edp_write(edp_base + 0x644, 0x28); /* UNIPHY_PLL_SDM_CFG3 */
		edp_write(edp_base + 0x648, 0x0); /* UNIPHY_PLL_SDM_CFG4 */
		edp_write(edp_base + 0x64c, 0x80); /* UNIPHY_PLL_SSC_CFG0 */
		edp_write(edp_base + 0x650, 0x0); /* UNIPHY_PLL_SSC_CFG1 */
		edp_write(edp_base + 0x654, 0x0); /* UNIPHY_PLL_SSC_CFG2 */
		edp_write(edp_base + 0x658, 0x0); /* UNIPHY_PLL_SSC_CFG3 */
		edp_write(edp_base + 0x66c, 0xa); /* UNIPHY_PLL_CAL_CFG0 */
		edp_write(edp_base + 0x674, 0x1); /* UNIPHY_PLL_CAL_CFG2 */
		edp_write(edp_base + 0x684, 0x5a); /* UNIPHY_PLL_CAL_CFG6 */
		edp_write(edp_base + 0x688, 0x0); /* UNIPHY_PLL_CAL_CFG7 */
		edp_write(edp_base + 0x68c, 0x60); /* UNIPHY_PLL_CAL_CFG8 */
		edp_write(edp_base + 0x690, 0x0); /* UNIPHY_PLL_CAL_CFG9 */
		edp_write(edp_base + 0x694, 0x46); /* UNIPHY_PLL_CAL_CFG10 */
		edp_write(edp_base + 0x698, 0x5); /* UNIPHY_PLL_CAL_CFG11 */
		edp_write(edp_base + 0x65c, 0x10); /* UNIPHY_PLL_LKDET_CFG0 */
		edp_write(edp_base + 0x660, 0x1a); /* UNIPHY_PLL_LKDET_CFG1 */
		edp_write(edp_base + 0x604, 0x0); /* UNIPHY_PLL_POSTDIV1_CFG */
		edp_write(edp_base + 0x624, 0x0); /* UNIPHY_PLL_POSTDIV2_CFG */
		edp_write(edp_base + 0x628, 0x0); /* UNIPHY_PLL_POSTDIV3_CFG */

		edp_write(edp_base + 0x620, 0x1); /* UNIPHY_PLL_GLB_CFG */
		edp_write(edp_base + 0x620, 0x5); /* UNIPHY_PLL_GLB_CFG */
		edp_write(edp_base + 0x620, 0x7); /* UNIPHY_PLL_GLB_CFG */
		edp_write(edp_base + 0x620, 0xf); /* UNIPHY_PLL_GLB_CFG */
	} else {
		pr_err("%s: Unknown configuration rate\n", __func__);
	}
}

void mdss_edp_enable_aux(unsigned char *edp_base, int enable)
{
	if (!enable) {
		edp_write(edp_base + 0x300, 0); /* EDP_AUX_CTRL */
		return;
	}

	/*reset AUX */
	edp_write(edp_base + 0x300, BIT(1)); /* EDP_AUX_CTRL */
	edp_write(edp_base + 0x300, 0); /* EDP_AUX_CTRL */

	/* Enable AUX */
	edp_write(edp_base + 0x300, BIT(0)); /* EDP_AUX_CTRL */

	edp_write(edp_base + 0x550, 0x2c); /* AUX_CFG0 */
	edp_write(edp_base + 0x308, 0x24924924); /* INTR_STATUS */
	edp_write(edp_base + 0x568, 0xff); /* INTR_MASK */
}

void mdss_edp_enable_mainlink(unsigned char *edp_base, int enable)
{
	u32 data;

	data = edp_read(edp_base + 0x004);
	data &= ~BIT(0);

	if (enable) {
		data |= 0x3;
		edp_write(edp_base + 0x004, data);
		edp_write(edp_base + 0x004, 0x1);
	} else {
		data |= 0x0;
		edp_write(edp_base + 0x004, data);
	}
}

void mdss_edp_enable_lane_bist(unsigned char *edp_base, int lane, int enable)
{
	unsigned char *addr_ln_bist_cfg, *addr_ln_pd_ctrl;

	/* EDP_PHY_EDPPHY_LNn_PD_CTL */
	addr_ln_pd_ctrl = edp_base + 0x404 + (0x40 * lane);
	/* EDP_PHY_EDPPHY_LNn_BIST_CFG0 */
	addr_ln_bist_cfg = edp_base + 0x408 + (0x40 * lane);

	if (enable) {
		edp_write(addr_ln_pd_ctrl, 0x0);
		edp_write(addr_ln_bist_cfg, 0x10);

	} else {
		edp_write(addr_ln_pd_ctrl, 0xf);
		edp_write(addr_ln_bist_cfg, 0x10);
	}
}
#endif
void mdss_edp_clk_deinit(struct mdss_edp_drv_pdata *edp_drv)
{
	if (edp_drv->aux_clk)
		clk_put(edp_drv->aux_clk);
	if (edp_drv->pixel_clk)
		clk_put(edp_drv->pixel_clk);
	if (edp_drv->ahb_clk)
		clk_put(edp_drv->ahb_clk);
	if (edp_drv->link_clk)
		clk_put(edp_drv->link_clk);
	if (edp_drv->mdp_core_clk)
		clk_put(edp_drv->mdp_core_clk);
}

int mdss_edp_clk_init(struct mdss_edp_drv_pdata *edp_drv)
{
	struct device *dev = &(edp_drv->pdev->dev);

	edp_drv->aux_clk = clk_get(dev, "core_clk");
	if (IS_ERR(edp_drv->aux_clk)) {
		pr_err("%s: Can't find aux_clk", __func__);
		edp_drv->aux_clk = NULL;
		goto mdss_edp_clk_err;
	}

	edp_drv->pixel_clk = clk_get(dev, "pixel_clk");
	if (IS_ERR(edp_drv->pixel_clk)) {
		pr_err("%s: Can't find pixel_clk", __func__);
		edp_drv->pixel_clk = NULL;
		goto mdss_edp_clk_err;
	}

	edp_drv->ahb_clk = clk_get(dev, "iface_clk");
	if (IS_ERR(edp_drv->ahb_clk)) {
		pr_err("%s: Can't find ahb_clk", __func__);
		edp_drv->ahb_clk = NULL;
		goto mdss_edp_clk_err;
	}

	edp_drv->link_clk = clk_get(dev, "link_clk");
	if (IS_ERR(edp_drv->link_clk)) {
		pr_err("%s: Can't find link_clk", __func__);
		edp_drv->link_clk = NULL;
		goto mdss_edp_clk_err;
	}

	/* need mdss clock to receive irq */
	edp_drv->mdp_core_clk = clk_get(dev, "mdp_core_clk");
	if (IS_ERR(edp_drv->mdp_core_clk)) {
		pr_err("%s: Can't find mdp_core_clk", __func__);
		edp_drv->mdp_core_clk = NULL;
		goto mdss_edp_clk_err;
	}

	return 0;

mdss_edp_clk_err:
	mdss_edp_clk_deinit(edp_drv);
	return -EPERM;
}

int mdss_edp_aux_clk_enable(struct mdss_edp_drv_pdata *edp_drv)
{
	int ret;

	if (clk_set_rate(edp_drv->aux_clk, 19200000) < 0)
		pr_err("%s: aux_clk - clk_set_rate failed\n",
					__func__);

	ret = clk_enable(edp_drv->aux_clk);
	if (ret) {
		pr_err("%s: Failed to enable aux clk\n", __func__);
		goto c2;
	}

	ret = clk_enable(edp_drv->ahb_clk);
	if (ret) {
		pr_err("%s: Failed to enable ahb clk\n", __func__);
		goto c1;
	}

	/* need mdss clock to receive irq */
	ret = clk_enable(edp_drv->mdp_core_clk);
	if (ret) {
		pr_err("%s: Failed to enable mdp_core_clk\n", __func__);
		goto c0;
	}

	return 0;
c0:
	clk_disable(edp_drv->ahb_clk);
c1:
	clk_disable(edp_drv->aux_clk);
c2:
	return ret;

}

void mdss_edp_aux_clk_disable(struct mdss_edp_drv_pdata *edp_drv)
{
	clk_disable(edp_drv->aux_clk);
	clk_disable(edp_drv->ahb_clk);
	clk_disable(edp_drv->mdp_core_clk);
}

int mdss_edp_clk_enable(struct mdss_edp_drv_pdata *edp_drv)
{
	int ret;

	if (edp_drv->clk_on) {
		pr_info("%s: edp clks are already ON\n", __func__);
		return 0;
	}

	pr_info("%s: aux_rate=%d\n", __func__, edp_drv->aux_rate);
	if (clk_set_rate(edp_drv->link_clk, edp_drv->link_rate * 27000000) < 0)
		pr_err("%s: link_clk - clk_set_rate failed\n",
					__func__);

	if (clk_set_rate(edp_drv->aux_clk, edp_drv->aux_rate) < 0)
		pr_err("%s: aux_clk - clk_set_rate failed\n",
					__func__);

	if (clk_set_rate(edp_drv->pixel_clk, edp_drv->pixel_rate) < 0)
		pr_err("%s: pixel_clk - clk_set_rate failed\n",
					__func__);

	ret = clk_enable(edp_drv->aux_clk);
	if (ret) {
		pr_err("%s: Failed to enable aux clk\n", __func__);
		goto c4;
	}
	ret = clk_enable(edp_drv->pixel_clk);
	if (ret) {
		pr_err("%s: Failed to enable pixel clk\n", __func__);
		goto c3;
	}
	ret = clk_enable(edp_drv->ahb_clk);
	if (ret) {
		pr_err("%s: Failed to enable ahb clk\n", __func__);
		goto c2;
	}
	ret = clk_enable(edp_drv->link_clk);
	if (ret) {
		pr_err("%s: Failed to enable link clk\n", __func__);
		goto c1;
	}
	ret = clk_enable(edp_drv->mdp_core_clk);
	if (ret) {
		pr_err("%s: Failed to enable mdp_core_clk\n", __func__);
		goto c0;
	}

	edp_drv->clk_on = 1;

	return 0;

c0:
	clk_disable(edp_drv->link_clk);
c1:
	clk_disable(edp_drv->ahb_clk);
c2:
	clk_disable(edp_drv->pixel_clk);
c3:
	clk_disable(edp_drv->aux_clk);
c4:
	return ret;
}

void mdss_edp_clk_disable(struct mdss_edp_drv_pdata *edp_drv)
{
	if (edp_drv->clk_on == 0) {
		pr_info("%s: edp clks are already OFF\n", __func__);
		return;
	}

	clk_disable(edp_drv->aux_clk);
	clk_disable(edp_drv->pixel_clk);
	clk_disable(edp_drv->ahb_clk);
	clk_disable(edp_drv->link_clk);
	clk_disable(edp_drv->mdp_core_clk);

	edp_drv->clk_on = 0;
}

int mdss_edp_prepare_aux_clocks(struct mdss_edp_drv_pdata *edp_drv)
{
	int ret;

	/* ahb clock should be prepared first */
	ret = clk_prepare(edp_drv->ahb_clk);
	if (ret) {
		pr_err("%s: Failed to prepare ahb clk\n", __func__);
		goto c3;
	}
	ret = clk_prepare(edp_drv->aux_clk);
	if (ret) {
		pr_err("%s: Failed to prepare aux clk\n", __func__);
		goto c2;
	}

	/* need mdss clock to receive irq */
	ret = clk_prepare(edp_drv->mdp_core_clk);
	if (ret) {
		pr_err("%s: Failed to prepare mdp_core clk\n", __func__);
		goto c1;
	}

	return 0;
c1:
	clk_unprepare(edp_drv->aux_clk);
c2:
	clk_unprepare(edp_drv->ahb_clk);
c3:
	return ret;

}

void mdss_edp_unprepare_aux_clocks(struct mdss_edp_drv_pdata *edp_drv)
{
	clk_unprepare(edp_drv->mdp_core_clk);
	clk_unprepare(edp_drv->aux_clk);
	clk_unprepare(edp_drv->ahb_clk);
}

int mdss_edp_prepare_clocks(struct mdss_edp_drv_pdata *edp_drv)
{
	int ret;

	/* ahb clock should be prepared first */
	ret = clk_prepare(edp_drv->ahb_clk);
	if (ret) {
		pr_err("%s: Failed to prepare ahb clk\n", __func__);
		goto c4;
	}
	ret = clk_prepare(edp_drv->aux_clk);
	if (ret) {
		pr_err("%s: Failed to prepare aux clk\n", __func__);
		goto c3;
	}
	ret = clk_prepare(edp_drv->pixel_clk);
	if (ret) {
		pr_err("%s: Failed to prepare pixel clk\n", __func__);
		goto c2;
	}
	ret = clk_prepare(edp_drv->link_clk);
	if (ret) {
		pr_err("%s: Failed to prepare link clk\n", __func__);
		goto c1;
	}
	ret = clk_prepare(edp_drv->mdp_core_clk);
	if (ret) {
		pr_err("%s: Failed to prepare mdp_core clk\n", __func__);
		goto c0;
	}

	return 0;
c0:
	clk_unprepare(edp_drv->link_clk);
c1:
	clk_unprepare(edp_drv->pixel_clk);
c2:
	clk_unprepare(edp_drv->aux_clk);
c3:
	clk_unprepare(edp_drv->ahb_clk);
c4:
	return ret;
}

void mdss_edp_unprepare_clocks(struct mdss_edp_drv_pdata *edp_drv)
{
	clk_unprepare(edp_drv->mdp_core_clk);
	clk_unprepare(edp_drv->aux_clk);
	clk_unprepare(edp_drv->pixel_clk);
	clk_unprepare(edp_drv->link_clk);
	/* ahb clock should be last one to disable */
	clk_unprepare(edp_drv->ahb_clk);
}

void mdss_edp_clk_debug(unsigned char *edp_base, unsigned char *mmss_cc_base)
{
	u32 da4, da0, d32c;
	u32 dc4, dc0, d330;

	/* pixel clk */
	da0  = edp_read(mmss_cc_base + 0x0a0);
	da4  = edp_read(mmss_cc_base + 0x0a4);
	d32c = edp_read(mmss_cc_base + 0x32c);

	/* main link clk */
	dc0  = edp_read(mmss_cc_base + 0x0c0);
	dc4  = edp_read(mmss_cc_base + 0x0c4);
	d330 = edp_read(mmss_cc_base + 0x330);

	pr_err("%s: da0=%x da4=%x d32c=%x dc0=%x dc4=%x d330=%x\n", __func__,
	(int)da0, (int)da4, (int)d32c, (int)dc0, (int)dc4, (int)d330);

}
