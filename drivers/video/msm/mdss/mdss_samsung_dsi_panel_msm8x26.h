/* Copyright (c) 2008-2013, The Linux Foundation. All rights reserved.
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

 #ifndef MDSS_SAMSUNG_PANEL_H
 #define MDSS_SAMSUNG_PANEL_H

 #include "mdss_panel.h"

#define MAX_BRIGHTNESS_LEVEL 255
#define MID_BRIGHTNESS_LEVEL 143
#define LOW_BRIGHTNESS_LEVEL 20
#define DIM_BRIGHTNESS_LEVEL 30
#if defined(CONFIG_FB_MSM_MDSS_TC_DSI2LVDS_WXGA_PANEL)
#if defined(CONFIG_MACH_MATISSELTE_VZW) || defined(CONFIG_MACH_MATISSELTE_USC)  
#define BL_MIN_BRIGHTNESS			1
#define BL_MAX_BRIGHTNESS_LEVEL		101
#define BL_MID_BRIGHTNESS_LEVEL		46
#define BL_LOW_BRIGHTNESS_LEVEL		1
#define BL_DIM_BRIGHTNESS_LEVEL		3
#define BL_DEFAULT_BRIGHTNESS		BL_MID_BRIGHTNESS_LEVEL
#elif defined(CONFIG_MACH_MATISSELTE_ATT)
#define BL_MIN_BRIGHTNESS			1
#define BL_MAX_BRIGHTNESS_LEVEL		105
#define BL_MID_BRIGHTNESS_LEVEL		52
#define BL_LOW_BRIGHTNESS_LEVEL		1
#define BL_DIM_BRIGHTNESS_LEVEL		3
#define BL_DEFAULT_BRIGHTNESS		BL_MID_BRIGHTNESS_LEVEL
#else
#define BL_MIN_BRIGHTNESS			1
#define BL_MAX_BRIGHTNESS_LEVEL		105
#define BL_MID_BRIGHTNESS_LEVEL		45
#define BL_LOW_BRIGHTNESS_LEVEL		1
#define BL_DIM_BRIGHTNESS_LEVEL		3
#define BL_DEFAULT_BRIGHTNESS		BL_MID_BRIGHTNESS_LEVEL
#endif
#elif !defined(CONFIG_FB_MSM_MDSS_SDC_WXGA_PANEL)
#define BL_MIN_BRIGHTNESS			3
#define BL_MAX_BRIGHTNESS_LEVEL		230
#define BL_MID_BRIGHTNESS_LEVEL		107
#define BL_LOW_BRIGHTNESS_LEVEL		3
#define BL_DIM_BRIGHTNESS_LEVEL		9
#define BL_DEFAULT_BRIGHTNESS		BL_MID_BRIGHTNESS_LEVEL
#endif

#if defined(CONFIG_FB_MSM_MDSS_CPT_QHD_PANEL)
#define SINGLE_WIRE_BL_CTRL 1
#define DT_CMD_HDR 6
#endif

enum {
	MIPI_RESUME_STATE,
	MIPI_SUSPEND_STATE,
};

 struct display_status{
	unsigned char auto_brightness;
	int bright_level;
	int siop_status;
	int wait_bl_on;
 };

 struct mdss_samsung_driver_data{
	struct dsi_buf sdc_tx_buf;
	struct msm_fb_data_type *mfd;
	struct dsi_buf sdc_rx_buf;
	struct mdss_panel_common_pdata *mdss_sdc_disp_pdata;
	struct mdss_panel_data *mpd;
	struct mutex lock;

#if defined(CONFIG_FB_MSM_MDSS_SDC_WXGA_PANEL)
	int bl_rst_gpio;
	int bl_ldi_en;
	int bl_sda;
	int bl_scl;
	int bl_ap_pwm;
#elif defined(CONFIG_FB_MSM_MDSS_CPT_QHD_PANEL)
	int lcd_en_gpio;
	int lcd_on_gpio;
	int lcd_io_1p8_en_gpio;
#elif defined(CONFIG_FB_MSM_MDSS_TC_DSI2LVDS_WXGA_PANEL)
	int lcd_en_gpio;
	int bl_ap_pwm;
	int bl_rst_gpio;
	int bl_wled;
	int bl_ldi_en;
	int bl_sda;
	int bl_scl;
#endif

#if defined(CONFIG_LCD_CLASS_DEVICE)
	struct platform_device *msm_pdev;
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
	struct display_status dstat;
 };

void mdnie_lite_tuning_init(struct mdss_samsung_driver_data* msd);
void mdss_dsi_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *cmds, int cnt,int flag);


#endif
