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
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/qpnp/pin.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/pwm.h>
#include <linux/err.h>
#include <linux/lcd.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#if defined(CONFIG_CABC_TUNING)
#include "cabc_tuning.h"
#endif
#include "mdss_dsi.h"
#include "mdss_samsung_tft_video_dual_dsi_panel.h"
#define DT_CMD_HDR 6

DEFINE_LED_TRIGGER(bl_led_trigger);

static struct mdss_dsi_phy_ctrl phy_params;
static struct mipi_samsung_driver_data msd;
static struct mdss_dsi_ctrl_pdata *left_back_up_data;
static int bl_backup;

static int panel_power_state;
static char board_rev;
static int lcd_attached = 1;
static int lcd_id = 0;

int get_lcd_attached(void);

int get_panel_power_state(void)
{
	return panel_power_state;
}

void mdss_dsi_panel_pwm_cfg(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ret;

	if (!gpio_is_valid(ctrl->pwm_pmic_gpio)) {
		pr_err("%s: pwm_pmic_gpio=%d Invalid\n", __func__,
				ctrl->pwm_pmic_gpio);
		ctrl->pwm_pmic_gpio = -1;
		return;
	}

	ret = gpio_request(ctrl->pwm_pmic_gpio, "disp_pwm");
	if (ret) {
		pr_err("%s: pwm_pmic_gpio=%d request failed\n", __func__,
				ctrl->pwm_pmic_gpio);
		ctrl->pwm_pmic_gpio = -1;
		return;
	}

	ctrl->pwm_bl = pwm_request(ctrl->pwm_lpg_chan, "lcd-bklt");
	if (ctrl->pwm_bl == NULL || IS_ERR(ctrl->pwm_bl)) {
		pr_err("%s: lpg_chan=%d pwm request failed", __func__,
				ctrl->pwm_lpg_chan);
		gpio_free(ctrl->pwm_pmic_gpio);
		ctrl->pwm_pmic_gpio = -1;
	}
}

static void mdss_dsi_panel_bklt_pwm(struct mdss_dsi_ctrl_pdata *ctrl, int level)
{
	int ret;
	u32 duty;

	if (ctrl->pwm_bl == NULL) {
		pr_err("%s: no PWM\n", __func__);
		return;
	}

	duty = level * ctrl->pwm_period;
	duty /= ctrl->bklt_max;

	pr_debug("%s: bklt_ctrl=%d pwm_period=%d pwm_gpio=%d pwm_lpg_chan=%d\n",
			__func__, ctrl->bklt_ctrl, ctrl->pwm_period,
				ctrl->pwm_pmic_gpio, ctrl->pwm_lpg_chan);

	pr_debug("%s: ndx=%d level=%d duty=%d\n", __func__,
					ctrl->ndx, level, duty);

	ret = pwm_config(ctrl->pwm_bl, duty, ctrl->pwm_period);
	if (ret) {
		pr_err("%s: pwm_config() failed err=%d.\n", __func__, ret);
		return;
	}

	ret = pwm_enable(ctrl->pwm_bl);
	if (ret)
		pr_err("%s: pwm_enable() failed err=%d\n", __func__, ret);
}

static char dcs_cmd[2] = {0xda, 0x00}; /* DTYPE_DCS_READ */
static struct dsi_cmd_desc dcs_read_cmd = {
	{DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(dcs_cmd)},
	dcs_cmd
};

u32 mdss_dsi_dcs_read(struct mdss_dsi_ctrl_pdata *ctrl,
			char cmd0, char cmd1)
{
	struct dcs_cmd_req cmdreq;

	dcs_cmd[0] = cmd0;
	dcs_cmd[1] = cmd1;
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = &dcs_read_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_RX | CMD_REQ_COMMIT;
	cmdreq.rbuf = ctrl->rx_buf.data;
	cmdreq.rlen = 1;
	cmdreq.cb = NULL; /* call back */
	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
	/*
	 * blocked here, until call back called
	 */

	return 0;
}

static void mdss_dsi_panel_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl,
			struct dsi_panel_cmds *pcmds)
{
	struct dcs_cmd_req cmdreq;

	if (get_lcd_attached() == 0)
	{
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return ;
	}

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = pcmds->cmds;
	cmdreq.cmds_cnt = pcmds->cmd_cnt;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(left_back_up_data, &cmdreq);
}

static char led_pwm1[2] = {0x51, 0x0};	/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc backlight_cmd = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(led_pwm1)},
	led_pwm1
};

static void mdss_dsi_panel_bklt_dcs(struct mdss_dsi_ctrl_pdata *ctrl, int level)
{
	struct dcs_cmd_req cmdreq;

	if (get_lcd_attached() == 0)
	{
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return ;
	}

	pr_debug("%s: level=%d\n", __func__, level);

	led_pwm1[1] = (unsigned char)level;

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = &backlight_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
	if(ctrl->ndx == DSI_CTRL_0)
		mdss_dsi_cmdlist_put(left_back_up_data, &cmdreq);
}

void mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if(ctrl_pdata->ndx == DSI_CTRL_0)
		return;

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_info("%s:%d, enalbe gpio not configured\n",
			   __func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_info("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
		return;
	}
	pr_info("%s : dsi index = %d \n", __func__, ctrl_pdata->ndx);
	pr_info("%s: enable = %d\n", __func__, enable);
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			msleep(5);
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			msleep(12);
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			msleep(12);
		} else {
		}
	} else {
		gpio_set_value((ctrl_pdata->rst_gpio), 0);
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
	}
}

#define DOWN_COEF_VALUE 200
static void mdss_dsi_panel_bl_ctrl(struct mdss_panel_data *pdata,
							u32 bl_level)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (!get_panel_power_state()) {
		pr_info("%s : get_panel_power_state off", __func__);
		return;
	}
	pr_info("%s : bl_level = %d\n", __func__, bl_level);

	if(bl_level)
		bl_backup = bl_level;

	bl_level = (DOWN_COEF_VALUE * bl_level) / left_back_up_data->bklt_max;

	pr_info("%s : Actual bl_level = %d\n", __func__, bl_level);

	switch (left_back_up_data->bklt_ctrl) {
	case BL_WLED:
		led_trigger_event(bl_led_trigger, bl_level);
		break;
	case BL_PWM:
		mdss_dsi_panel_bklt_pwm(left_back_up_data, bl_level);
		break;
	case BL_DCS_CMD:
		pr_debug("%s : dsi index = %d\n", __func__, ctrl_pdata->ndx);
		mdss_dsi_panel_bklt_dcs(ctrl_pdata, bl_level);
		break;
	default:
		pr_err("%s: Unknown bl_ctrl configuration\n",
			__func__);
		break;
	}
}

static int mdss_dsi_panel_on(struct mdss_panel_data *pdata)
{
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	static int first_boot=1;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	mipi  = &pdata->panel_info.mipi;

	pr_info("%s: ctrl index=%d ++ \n", __func__, ctrl->ndx);



	if (ctrl->ndx == DSI_CTRL_1) {
		if (ctrl->on_cmds.cmd_cnt) {
			mdss_dsi_panel_cmds_send(ctrl, &ctrl->on_cmds);
		}
		panel_power_state = 1;

		if(bl_backup)
			mdss_dsi_panel_bl_ctrl(pdata, bl_backup);

		pwm_backlight_enable();

		if(first_boot) {
			pr_info("%s: first backlight set to 150 level \n",
				__func__);
			mdss_dsi_panel_bl_ctrl(pdata, 150);
			first_boot=0;
		}
		if (msd.dstat.recovery_boot_mode) {
			pr_info("%s: Recovery Mode backlight set to 180 level \n",
				__func__);
			msd.dstat.bright_level = RECOVERY_BRIGHTNESS;
			mdss_dsi_panel_bl_ctrl(pdata, RECOVERY_BRIGHTNESS);
		} else {
#if defined(CONFIG_CABC_TUNING)
			CABC_Set_Mode();
#endif
		}
	}

	pr_info("%s: ctrl index=%d -- \n", __func__, ctrl->ndx);

	return 0;
}

static int mdss_dsi_panel_off(struct mdss_panel_data *pdata)
{
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	panel_power_state = 0;

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	pr_info("%s: ctrl ndx=%d ++ \n", __func__, ctrl->ndx);

	mipi  = &pdata->panel_info.mipi;

	if (ctrl->ndx == DSI_CTRL_1) {
		pwm_backlight_disable();

		if (ctrl->off_cmds.cmd_cnt)
			mdss_dsi_panel_cmds_send(ctrl, &ctrl->off_cmds);
	}


	pr_info("%s: ctrl ndx=%d -- \n", __func__, ctrl->ndx);
	return 0;
}

int mdss_panel_dt_get_dst_fmt(u32 bpp, char mipi_mode, u32 pixel_packing,
				char *dst_format)
{
	int rc = 0;
	switch (bpp) {
	case 3:
		*dst_format = DSI_CMD_DST_FORMAT_RGB111;
		break;
	case 8:
		*dst_format = DSI_CMD_DST_FORMAT_RGB332;
		break;
	case 12:
		*dst_format = DSI_CMD_DST_FORMAT_RGB444;
		break;
	case 16:
		switch (mipi_mode) {
		case DSI_VIDEO_MODE:
			*dst_format = DSI_VIDEO_DST_FORMAT_RGB565;
			break;
		case DSI_CMD_MODE:
			*dst_format = DSI_CMD_DST_FORMAT_RGB565;
			break;
		default:
			*dst_format = DSI_VIDEO_DST_FORMAT_RGB565;
			break;
		}
		break;
	case 18:
		switch (mipi_mode) {
		case DSI_VIDEO_MODE:
			if (pixel_packing == 0)
				*dst_format = DSI_VIDEO_DST_FORMAT_RGB666;
			else
				*dst_format = DSI_VIDEO_DST_FORMAT_RGB666_LOOSE;
			break;
		case DSI_CMD_MODE:
			*dst_format = DSI_CMD_DST_FORMAT_RGB666;
			break;
		default:
			if (pixel_packing == 0)
				*dst_format = DSI_VIDEO_DST_FORMAT_RGB666;
			else
				*dst_format = DSI_VIDEO_DST_FORMAT_RGB666_LOOSE;
			break;
		}
		break;
	case 24:
		switch (mipi_mode) {
		case DSI_VIDEO_MODE:
			*dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
			break;
		case DSI_CMD_MODE:
			*dst_format = DSI_CMD_DST_FORMAT_RGB888;
			break;
		default:
			*dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
			break;
		}
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}


static int mdss_dsi_parse_dcs_cmds(struct device_node *np,
		struct dsi_panel_cmds *pcmds, char *cmd_key, char *link_key)
{
	const char *data;
	int blen = 0, len;
	char *buf, *bp;
	struct dsi_ctrl_hdr *dchdr;
	int i, cnt;

	data = of_get_property(np, cmd_key, &blen);
	if (!data) {
		pr_err("%s: failed, key=%s\n", __func__, cmd_key);
		return -ENOMEM;
	}

	buf = kzalloc(sizeof(char) * blen, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memcpy(buf, data, blen);

	/* scan dcs commands */
	bp = buf;
	len = blen;
	cnt = 0;
	while (len > sizeof(*dchdr)) {
		dchdr = (struct dsi_ctrl_hdr *)bp;
		dchdr->dlen = ntohs(dchdr->dlen);
		if (dchdr->dlen > len) {
			pr_err("%s: dtsi cmd=%x error, len=%d",
				__func__, dchdr->dtype, dchdr->dlen);
			kfree(buf);
			return -ENOMEM;
		}
		pr_debug("bp : %x, dlen : %x, len : %x",*bp,dchdr->dlen,len);
		bp += sizeof(*dchdr);
		len -= sizeof(*dchdr);
		bp += dchdr->dlen;
		len -= dchdr->dlen;
		cnt++;
	}

	if (len != 0) {
		pr_err("%s: dcs_cmd=%x len=%d error!",
				__func__, buf[0], blen);
		kfree(buf);
		return -ENOMEM;
	}

	pcmds->cmds = kzalloc(cnt * sizeof(struct dsi_cmd_desc),
						GFP_KERNEL);
	if (!pcmds->cmds) {
		kfree(buf);
		return -ENOMEM;
	}

	pcmds->cmd_cnt = cnt;
	pcmds->buf = buf;
	pcmds->blen = blen;

	bp = buf;
	len = blen;
	for (i = 0; i < cnt; i++) {
		dchdr = (struct dsi_ctrl_hdr *)bp;
		len -= sizeof(*dchdr);
		bp += sizeof(*dchdr);
		pcmds->cmds[i].dchdr = *dchdr;
		pcmds->cmds[i].payload = bp;
		bp += dchdr->dlen;
		len -= dchdr->dlen;
	}

	pcmds->link_state = DSI_LP_MODE; /* default */

	data = of_get_property(np, link_key, NULL);
	if (!strncmp(data, "dsi_hs_mode", 11))
		pcmds->link_state = DSI_HS_MODE;

	pr_debug("%s: dcs_cmd=%x len=%d, cmd_cnt=%d link_state=%d\n", __func__,
		pcmds->buf[0], pcmds->blen, pcmds->cmd_cnt, pcmds->link_state);

	return 0;
}

static struct lcd_ops mipi_samsung_disp_props = {
	.get_power = NULL,
	.set_power = NULL,
};

static ssize_t mdss_samsung_disp_lcdtype_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char temp[100];
	snprintf(temp, 20, "JDI_LPM084A082A");
	strlcat(buf, temp, 100);
	return strnlen(buf, 100);
}
static DEVICE_ATTR(lcd_type, S_IRUGO, mdss_samsung_disp_lcdtype_show, NULL);

static ssize_t store_lux(struct device *dev,
			    struct device_attribute *dev_attr,
			    const char *buf, size_t count)
{
	int ret;
	unsigned int input_lux;

	ret = kstrtouint(buf, 10, &input_lux);

	if (ret)
		return ret;

#if defined(CONFIG_CABC_TUNING)
	update_lux(input_lux);
#endif

	return count;
}
static DEVICE_ATTR(lux, 0664, NULL, store_lux);

static int mdss_dsi_parse_fbc_params(struct device_node *np,
				struct mdss_panel_info *panel_info)
{
	int rc, fbc_enabled = 0;
	u32 tmp;

	fbc_enabled = of_property_read_bool(np,	"qcom,mdss-dsi-fbc-enable");
	if (fbc_enabled) {
		pr_debug("%s:%d FBC panel enabled.\n", __func__, __LINE__);
		panel_info->fbc.enabled = 1;
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-bpp", &tmp);
		panel_info->fbc.target_bpp =	(!rc ? tmp : panel_info->bpp);
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-packing",
				&tmp);
		panel_info->fbc.comp_mode = (!rc ? tmp : 0);
		panel_info->fbc.qerr_enable = of_property_read_bool(np,
			"qcom,mdss-dsi-fbc-quant-error");
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-bias", &tmp);
		panel_info->fbc.cd_bias = (!rc ? tmp : 0);
		panel_info->fbc.pat_enable = of_property_read_bool(np,
				"qcom,mdss-dsi-fbc-pat-mode");
		panel_info->fbc.vlc_enable = of_property_read_bool(np,
				"qcom,mdss-dsi-fbc-vlc-mode");
		panel_info->fbc.bflc_enable = of_property_read_bool(np,
				"qcom,mdss-dsi-fbc-bflc-mode");
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-h-line-budget",
				&tmp);
		panel_info->fbc.line_x_budget = (!rc ? tmp : 0);
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-budget-ctrl",
				&tmp);
		panel_info->fbc.block_x_budget = (!rc ? tmp : 0);
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-block-budget",
				&tmp);
		panel_info->fbc.block_budget = (!rc ? tmp : 0);
		rc = of_property_read_u32(np,
				"qcom,mdss-dsi-fbc-lossless-threshold", &tmp);
		panel_info->fbc.lossless_mode_thd = (!rc ? tmp : 0);
		rc = of_property_read_u32(np,
				"qcom,mdss-dsi-fbc-lossy-threshold", &tmp);
		panel_info->fbc.lossy_mode_thd = (!rc ? tmp : 0);
		rc = of_property_read_u32(np, "qcom,mdss-dsi-fbc-rgb-threshold",
				&tmp);
		panel_info->fbc.lossy_rgb_thd = (!rc ? tmp : 0);
		rc = of_property_read_u32(np,
				"qcom,mdss-dsi-fbc-lossy-mode-idx", &tmp);
		panel_info->fbc.lossy_mode_idx = (!rc ? tmp : 0);
	} else {
		pr_debug("%s:%d Panel does not support FBC.\n",
				__func__, __LINE__);
		panel_info->fbc.enabled = 0;
		panel_info->fbc.target_bpp =
			panel_info->bpp;
	}
	return 0;
}

static int mdss_dsi_panel_registered(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
							panel_data);
	msd.mfd = registered_fb[0]->par;
	msd.pdata = pdata;
	msd.ctrl_pdata = ctrl_pdata;
	pr_info("%s:%d, panel_registered successfully\n", __func__, __LINE__);
	return 0;
}

static int mdss_panel_parse_dt(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	u32 res[6], tmp;
	int rc, i, len;
	const char *data;
	static const char *bl_ctrl_type, *pdest;
	struct mdss_panel_info *pinfo = &(ctrl_pdata->panel_data.panel_info);

	rc = of_property_read_u32_array(np, "qcom,mdss-pan-res", res, 2);
	if (rc) {
		pr_err("%s:%d, panel resolution not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	pinfo->xres = (!rc ? res[0] : 640);
	pinfo->yres = (!rc ? res[1] : 480);

	rc = of_property_read_u32_array(np, "qcom,mdss-pan-active-res", res, 2);
	if (rc == 0) {
		pinfo->lcdc.xres_pad =
			pinfo->xres - res[0];
		pinfo->lcdc.yres_pad =
			pinfo->yres - res[1];
	}

	rc = of_property_read_u32_array(np, "qcom,mdss-pan-physical-dimension", res, 2);
	pinfo->physical_width = (!rc ? res[0] : -1);
	pinfo->physical_height = (!rc ? res[1] : -1);

	pr_debug("Panel Physical Width=%d, Height=%d\n",
		pinfo->physical_width,
		pinfo->physical_height);

	rc = of_property_read_u32(np, "qcom,mdss-pan-bpp", &tmp);
	if (rc) {
		pr_err("%s:%d, panel bpp not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	pinfo->bpp = (!rc ? tmp : 24);

	pdest = of_get_property(np,
				"qcom,mdss-pan-dest", NULL);
	if (strlen(pdest) != 9) {
		pr_err("%s: Unknown pdest specified\n", __func__);
		return -EINVAL;
	}
	if (!strncmp(pdest, "display_1", 9))
		pinfo->pdest = DISPLAY_1;
	else if (!strncmp(pdest, "display_2", 9))
		pinfo->pdest = DISPLAY_2;
	else {
		pr_debug("%s: pdest not specified. Set Default\n",
							__func__);
		pinfo->pdest = DISPLAY_1;
	}

	rc = of_property_read_u32_array(np,
		"qcom,mdss-pan-porch-values", res, 6);
	pinfo->lcdc.h_back_porch = (!rc ? res[0] : 6);
	pinfo->lcdc.h_pulse_width = (!rc ? res[1] : 2);
	pinfo->lcdc.h_front_porch = (!rc ? res[2] : 6);
	pinfo->lcdc.v_back_porch = (!rc ? res[3] : 6);
	pinfo->lcdc.v_pulse_width = (!rc ? res[4] : 2);
	pinfo->lcdc.v_front_porch = (!rc ? res[5] : 6);

	rc = of_property_read_u32(np,
		"qcom,mdss-pan-underflow-clr", &tmp);
	pinfo->lcdc.underflow_clr = (!rc ? tmp : 0xff);

	bl_ctrl_type = of_get_property(np,
				  "qcom,mdss-pan-bl-ctrl", NULL);
	if ((bl_ctrl_type) && (!strncmp(bl_ctrl_type, "bl_ctrl_wled", 12))) {
		led_trigger_register_simple("bkl-trigger", &bl_led_trigger);
		pr_err("%s: SUCCESS-> WLED TRIGGER register\n", __func__);

		pinfo->bklt_ctrl = BL_WLED;
	} else if (!strncmp(bl_ctrl_type, "bl_ctrl_pwm", 11)) {
		ctrl_pdata->bklt_ctrl = BL_PWM;

		rc = of_property_read_u32(np, "qcom,pwm-period", &tmp);
		if (rc) {
			pr_err("%s:%d, Error, panel pwm_period\n",
						__func__, __LINE__);
			return -EINVAL;
		}
		ctrl_pdata->pwm_period = tmp;

		rc = of_property_read_u32(np, "qcom,pwm-lpg-channel", &tmp);
		if (rc) {
			pr_err("%s:%d, Error, dsi lpg channel\n",
						__func__, __LINE__);
			return -EINVAL;
		}
		ctrl_pdata->pwm_lpg_chan = tmp;

		tmp = of_get_named_gpio(np, "qcom,pwm-pmic-gpio", 0);
		ctrl_pdata->pwm_pmic_gpio =  tmp;
		pr_info("%s: pwm_pmic_gpio=%d\n",__func__, ctrl_pdata->pwm_pmic_gpio);

	} else if (!strncmp(bl_ctrl_type, "bl_ctrl_dcs", 11)) {
		pr_info("%s: BL DCS Control \n",__func__);
		pinfo->bklt_ctrl = BL_DCS_CMD;
	} else {
		pr_err("%s: Unknown backlight control\n", __func__);
		pinfo->bklt_ctrl = UNKNOWN_CTRL;
	}

	rc = of_property_read_u32(np, "qcom,mdss-brightness-max-level", &tmp);
	pinfo->brightness_max = (!rc ? tmp : MDSS_MAX_BL_BRIGHTNESS);

	rc = of_property_read_u32_array(np,
		"qcom,mdss-pan-bl-levels", res, 2);
	pinfo->bl_min = (!rc ? res[0] : 0);
	pinfo->bl_max = (!rc ? res[1] : 255);
	ctrl_pdata->bklt_max = pinfo->bl_max;

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-mode", &tmp);
	pinfo->mipi.mode = (!rc ? tmp : DSI_VIDEO_MODE);

	rc = of_property_read_u32(np, "qcom,mdss-vsync-enable", &tmp);
	pinfo->mipi.vsync_enable = (!rc ? tmp : 0);

	rc = of_property_read_u32(np, "qcom,mdss-hw-vsync-mode", &tmp);
	pinfo->mipi.hw_vsync_mode = (!rc ? tmp : 0);


	rc = of_property_read_u32(np,
		"qcom,mdss-pan-dsi-h-pulse-mode", &tmp);
	pinfo->mipi.pulse_mode_hsa_he = (!rc ? tmp : false);

	rc = of_property_read_u32_array(np,
		"qcom,mdss-pan-dsi-h-power-stop", res, 3);
	pinfo->mipi.hbp_power_stop = (!rc ? res[0] : false);
	pinfo->mipi.hsa_power_stop = (!rc ? res[1] : false);
	pinfo->mipi.hfp_power_stop = (!rc ? res[2] : false);

	rc = of_property_read_u32_array(np,
		"qcom,mdss-pan-dsi-bllp-power-stop", res, 2);
	pinfo->mipi.bllp_power_stop =
					(!rc ? res[0] : false);
	pinfo->mipi.eof_bllp_power_stop =
					(!rc ? res[1] : false);

	rc = of_property_read_u32(np,
		"qcom,mdss-pan-dsi-traffic-mode", &tmp);
	pinfo->mipi.traffic_mode =
			(!rc ? tmp : DSI_NON_BURST_SYNCH_PULSE);

	rc = of_property_read_u32(np,
		"qcom,mdss-pan-insert-dcs-cmd", &tmp);
	pinfo->mipi.insert_dcs_cmd =
			(!rc ? tmp : 1);

	rc = of_property_read_u32(np,
		"qcom,mdss-pan-wr-mem-continue", &tmp);
	pinfo->mipi.wr_mem_continue =
			(!rc ? tmp : 0x3c);

	rc = of_property_read_u32(np,
		"qcom,mdss-pan-wr-mem-start", &tmp);
	pinfo->mipi.wr_mem_start =
			(!rc ? tmp : 0x2c);

	rc = of_property_read_u32(np,
		"qcom,mdss-pan-te-sel", &tmp);
	pinfo->mipi.te_sel =
			(!rc ? tmp : 1);

	rc = of_property_read_u32(np,
		"qcom,mdss-pan-dsi-dst-format", &tmp);
	pinfo->mipi.dst_format =
			(!rc ? tmp : DSI_VIDEO_DST_FORMAT_RGB888);

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-vc", &tmp);
	pinfo->mipi.vc = (!rc ? tmp : 0);

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-rgb-swap", &tmp);
	pinfo->mipi.rgb_swap = (!rc ? tmp : DSI_RGB_SWAP_RGB);

	rc = of_property_read_u32(np, "qcom,mdss-force-clk-lane-hs", &tmp);
	pinfo->mipi.force_clk_lane_hs = (!rc ? tmp : 0);

	rc = of_property_read_u32_array(np,
		"qcom,mdss-pan-dsi-data-lanes", res, 4);
	pinfo->mipi.data_lane0 = (!rc ? res[0] : true);
	pinfo->mipi.data_lane1 = (!rc ? res[1] : false);
	pinfo->mipi.data_lane2 = (!rc ? res[2] : false);
	pinfo->mipi.data_lane3 = (!rc ? res[3] : false);

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-dlane-swap", &tmp);
	pinfo->mipi.dlane_swap = (!rc ? tmp : 0);

	rc = of_property_read_u32_array(np, "qcom,mdss-pan-dsi-t-clk", res, 2);
	pinfo->mipi.t_clk_pre = (!rc ? res[0] : 0x24);
	pinfo->mipi.t_clk_post = (!rc ? res[1] : 0x03);

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-stream", &tmp);
	pinfo->mipi.stream = (!rc ? tmp : 0);

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-mdp-tr", &tmp);
	pinfo->mipi.mdp_trigger =
			(!rc ? tmp : DSI_CMD_TRIGGER_SW);
	if (pinfo->mipi.mdp_trigger > 6) {
		pr_err("%s:%d, Invalid mdp trigger. Forcing to sw trigger",
						 __func__, __LINE__);
		pinfo->mipi.mdp_trigger =
					DSI_CMD_TRIGGER_SW;
	}

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-dma-tr", &tmp);
	pinfo->mipi.dma_trigger =
			(!rc ? tmp : DSI_CMD_TRIGGER_SW);
	if (pinfo->mipi.dma_trigger > 6) {
		pr_err("%s:%d, Invalid dma trigger. Forcing to sw trigger",
						 __func__, __LINE__);
		pinfo->mipi.dma_trigger =
					DSI_CMD_TRIGGER_SW;
	}

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-frame-rate", &tmp);
	pinfo->mipi.frame_rate = (!rc ? tmp : 60);

	rc = of_property_read_u32(np, "qcom,mdss-pan-clk-rate", &tmp);
	pinfo->clk_rate = (!rc ? tmp : 0);

	data = of_get_property(np, "qcom,panel-phy-regulatorSettings", &len);
	if ((!data) || (len != 7)) {
		pr_err("%s:%d, Unable to read Phy regulator settings",
		       __func__, __LINE__);
		goto error;
	}
	for (i = 0; i < len; i++)
		phy_params.regulator[i] = data[i];

	data = of_get_property(np, "qcom,panel-phy-timingSettings", &len);
	if ((!data) || (len != 12)) {
		pr_err("%s:%d, Unable to read Phy timing settings",
		       __func__, __LINE__);
		goto error;
	}
	for (i = 0; i < len; i++)
		phy_params.timing[i] = data[i];

	data = of_get_property(np, "qcom,panel-phy-strengthCtrl", &len);
	if ((!data) || (len != 2)) {
		pr_err("%s:%d, Unable to read Phy Strength ctrl settings",
		       __func__, __LINE__);
		goto error;
	}
	phy_params.strength[0] = data[0];
	phy_params.strength[1] = data[1];

	data = of_get_property(np, "qcom,panel-phy-bistCtrl", &len);
	if ((!data) || (len != 6)) {
		pr_err("%s:%d, Unable to read Phy Bist Ctrl settings",
		       __func__, __LINE__);
		goto error;
	}
	for (i = 0; i < len; i++)
		phy_params.bistctrl[i] = data[i];

	data = of_get_property(np, "qcom,panel-phy-laneConfig", &len);
	if ((!data) || (len != 45)) {
		pr_err("%s:%d, Unable to read Phy lane configure settings",
		       __func__, __LINE__);
		goto error;
	}
	for (i = 0; i < len; i++)
		phy_params.lanecfg[i] = data[i];

	pinfo->mipi.dsi_phy_db = phy_params;

	mdss_dsi_parse_fbc_params(np, pinfo);

	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->on_cmds,
		"qcom,panel-on-cmds", "qcom,on-cmds-dsi-state");

	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->off_cmds,
		"qcom,panel-off-cmds", "qcom,off-cmds-dsi-state");

	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->ce_on_cmds,
		"samsung,panel-ce-on-cmds", "qcom,on-cmds-dsi-state");
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->ce_off_cmds,
		"samsung,panel-ce-off-cmds", "qcom,off-cmds-dsi-state");
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->cabc_on_cmds,
		"samsung,panel-cabc-on-cmds", "qcom,on-cmds-dsi-state");
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->cabc_off_cmds,
		"samsung,panel-cabc-off-cmds", "qcom,off-cmds-dsi-state");
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->cabc_tune_cmds,
		"samsung,panel-cabc-tune-cmds", "qcom,off-cmds-dsi-state");

	return 0;

error:
	return -EINVAL;
}

int mdss_dsi_panel_init(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata,
	bool cmd_cfg_cont_splash)
{
	int rc = 0;
	static const char *panel_name;
	static int first_init = 0;
	bool cont_splash_enabled;
	struct lcd_device *lcd_device;

	if (!node) {
		pr_err("%s: no panel node\n", __func__);
		return -ENODEV;
	}

	pr_debug("%s:%d\n", __func__, __LINE__);

	panel_name = of_get_property(node, "qcom,mdss-dsi-panel-name", NULL);
	if (!panel_name)
		pr_info("%s:%d, Panel name not specified\n",
						__func__, __LINE__);
	else
		pr_err("%s: Panel Name = %s\n", __func__, panel_name);

	rc = mdss_panel_parse_dt(node, ctrl_pdata);
	if (rc) {
		pr_err("%s:%d panel dt parse failed\n", __func__, __LINE__);
		return rc;
	}

	cont_splash_enabled = of_property_read_bool(node,
				"qcom,cont-splash-enabled");

	if (!cont_splash_enabled) {
		pr_info("%s:%d Continuous splash flag not found.\n",
				__func__, __LINE__);
		ctrl_pdata->panel_data.panel_info.cont_splash_enabled = 0;
	} else {
		pr_info("%s:%d Continuous splash flag enabled.\n",
				__func__, __LINE__);

		ctrl_pdata->panel_data.panel_info.cont_splash_enabled = 1;
	}

	if (get_lcd_attached() == 0) {
		ctrl_pdata->panel_data.panel_info.cont_splash_enabled = 0;
	}

	ctrl_pdata->on = mdss_dsi_panel_on;
	ctrl_pdata->off = mdss_dsi_panel_off;
	ctrl_pdata->panel_reset = mdss_dsi_panel_reset;
	ctrl_pdata->bl_fnc= mdss_dsi_panel_bl_ctrl;
	ctrl_pdata->registered = mdss_dsi_panel_registered;
	ctrl_pdata->panel_data.set_backlight = mdss_dsi_panel_bl_ctrl;
	ctrl_pdata->bklt_ctrl = ctrl_pdata->panel_data.panel_info.bklt_ctrl;

	if(!left_back_up_data &&
			ctrl_pdata->panel_data.panel_info.pdest == DISPLAY_2) {
		pr_info("%s: dsi_ctrl_1 backup",__func__);
		left_back_up_data = ctrl_pdata;
	}

	if(!first_init) {
		mutex_init(&msd.lock);

		lcd_device = lcd_device_register("panel", NULL, NULL,
						&mipi_samsung_disp_props);

		if (IS_ERR(lcd_device)) {
			rc = PTR_ERR(lcd_device);
			return rc;
		}

		rc = sysfs_create_file(&lcd_device->dev.kobj,
				&dev_attr_lcd_type.attr);
		if (rc)
			pr_info(" can't create lcd_type sysfs\n");

		rc = sysfs_create_file(&lcd_device->dev.kobj,
				&dev_attr_lux.attr);

		if (rc)
			pr_info(" can't create lux sysfs\n");

		first_init = 1;
	}

#if defined(CONFIG_CABC_TUNING)
		cabc_tuning_init(ctrl_pdata);
#endif

	return 0;
}

static int atoi(const char *name)
{
	int val = 0;

	for (;; name++) {
		switch (*name) {
		case '0' ... '9':
			val = 10*val+(*name-'0');
			break;
		default:
			return val;
		}
	}
}

int get_samsung_lcd_attached(void)
{
	return lcd_attached;
}
EXPORT_SYMBOL(get_samsung_lcd_attached);

static int __init mdss_panel_current_hw_rev(char *rev)
{
	board_rev = atoi(rev);

	pr_info("%s : %d", __func__, board_rev);

	return 1;
}

__setup("samsung.board_rev=", mdss_panel_current_hw_rev);

static int __init current_boot_mode(char *mode)
{
	/*
	*	1,2 is recovery booting
	*	0 is normal booting
	*/
        if ((strncmp(mode, "1", 1) == 0)||(strncmp(mode, "2", 1) == 0))
		msd.dstat.recovery_boot_mode = 1;
	else
		msd.dstat.recovery_boot_mode = 0;

	pr_debug("%s %s", __func__, msd.dstat.recovery_boot_mode == 1 ?
						"recovery" : "normal");
	return 1;
}
__setup("androidboot.boot_recovery=", current_boot_mode);

static int __init get_lcd_id_cmdline(char *mode)
{
	char *pt;

	lcd_id = 0;
	if( mode == NULL ) return 1;
	for( pt = mode; *pt != 0; pt++ )
	{
		lcd_id <<= 4;
		switch(*pt)
		{
			case '0' ... '9' :
				lcd_id += *pt -'0';
			break;
			case 'a' ... 'f' :
				lcd_id += 10 + *pt -'a';
			break;
			case 'A' ... 'F' :
				lcd_id += 10 + *pt -'A';
			break;
		}
	}
	lcd_attached = ((lcd_id&0xFFFFFF)!=0x000000);

#if defined(CONFIG_LCD_FORCE_VIDEO_MODE)
	lcd_attached = 1;
#endif

	pr_info( "%s: LCD_ID = 0x%X, lcd_attached =%d", __func__,lcd_id, lcd_attached);

	return 0;
}

__setup( "lcd_id=0x", get_lcd_id_cmdline );
