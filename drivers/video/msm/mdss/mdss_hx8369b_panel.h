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

 #ifndef MDSS_HX8369B_PANEL_H
 #define MDSS_HX8369B_PANEL_H

 #include "mdss_panel.h"

#define MAX_BRIGHTNESS_LEVEL 255
#define MID_BRIGHTNESS_LEVEL 143
#define LOW_BRIGHTNESS_LEVEL 20
#define DIM_BRIGHTNESS_LEVEL 30

#define MAX_BRIGHTNESS_IN_BLU 32

#if defined(CONFIG_BACKLIGHT_IC_KTD2801)
#define BL_MIN_BRIGHTNESS			1
#define BL_MAX_BRIGHTNESS_LEVEL		105
#define BL_MID_BRIGHTNESS_LEVEL		45
#define BL_LOW_BRIGHTNESS_LEVEL		1
#define BL_DIM_BRIGHTNESS_LEVEL		3
#define BL_DEFAULT_BRIGHTNESS		BL_MID_BRIGHTNESS_LEVEL
#else
#define BL_MIN_BRIGHTNESS			6
#define BL_MAX_BRIGHTNESS_LEVEL		192
#define BL_MID_BRIGHTNESS_LEVEL		94
#define BL_LOW_BRIGHTNESS_LEVEL		7
#define BL_DIM_BRIGHTNESS_LEVEL		13
#define BL_DEFAULT_BRIGHTNESS		BL_MID_BRIGHTNESS_LEVEL
#endif
enum mipi_samsung_cmd_list {
	PANEL_READY_TO_ON_FAST,
	PANEL_READY_TO_ON,
	PANEL_READY_TO_OFF,
	PANEL_ON,
	PANEL_OFF,
	PANEL_LATE_ON,
	PANEL_EARLY_OFF,
	PANEL_GAMMA_UPDATE,
	PANEL_ELVSS_UPDATE,
	PANEL_ACL_ON,
	PANEL_ACL_OFF,
	PANEL_ACL_UPDATE,
	MTP_READ_ENABLE,
	PANEL_BRIGHT_CTRL,
};



enum {
	MIPI_RESUME_STATE,
	MIPI_SUSPEND_STATE,
};

 struct display_status{
 	unsigned char auto_brightness;
	int bright_level;
	int siop_status;
	unsigned char acl_on;
 };

 struct mdss_samsung_driver_data{
 	struct dsi_buf samsung_tx_buf;
	struct msm_fb_data_type *mfd;
	struct dsi_buf samsung_rx_buf;
	struct mdss_panel_common_pdata *mdss_samsung_disp_pdata;
	struct mdss_panel_data *mpd;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata;
	struct mutex lock;
	int bl_level;
#if defined(CONFIG_BACKLIGHT_IC_KTD2801)
	int bl_ap_pwm;
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
