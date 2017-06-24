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

 #ifndef MDSS_MS01_PANEL_H
 #define MDSS_MS01_PANEL_H

 #include "mdss_panel.h"
#include "smart_mtp_s6e8aa0x01.h"
#include "smart_dimming.h"

#define MAX_BRIGHTNESS_LEVEL 255
#define MID_BRIGHTNESS_LEVEL 143
#define LOW_BRIGHTNESS_LEVEL 20
#define DIM_BRIGHTNESS_LEVEL 30

#define BL_MIN_BRIGHTNESS			6
#define BL_MAX_BRIGHTNESS_LEVEL		192
#define BL_MID_BRIGHTNESS_LEVEL		94
#define BL_LOW_BRIGHTNESS_LEVEL		7
#define BL_DIM_BRIGHTNESS_LEVEL		13
#define BL_DEFAULT_BRIGHTNESS		BL_MID_BRIGHTNESS_LEVEL

enum {
	MIPI_RESUME_STATE,
	MIPI_SUSPEND_STATE,
};

#define SmartDimming_CANDELA_UPPER_LIMIT (300)
#define MTP_DATA_SIZE (24)
#define MTP_DATA_SIZE_S6E63M0 (21)
#define MTP_DATA_SIZE_EA8868 (21)
#define ELVSS_DATA_SIZE (24)
#define MTP_REGISTER	(0xD3)
#define ELVSS_REGISTER	 (0xD4)
#define SmartDimming_GammaUpdate_Pos (2)


 struct display_status{
 	unsigned char auto_brightness;
	int bright_level;
	int siop_status;
	unsigned char acl_on;
	unsigned char gamma_mode; /* 1: 1.9 gamma, 0: 2.2 gamma */
	unsigned char is_smart_dim_loaded;
	unsigned char is_elvss_loaded;

 };

 struct mdss_samsung_driver_data{
 	struct dsi_buf samsung_tx_buf;
	struct msm_fb_data_type *mfd;
	struct dsi_buf samsung_rx_buf;
	struct mdss_panel_common_pdata *mdss_samsung_disp_pdata;
	struct mdss_panel_data *mpd;
	struct mutex lock;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata;

#if defined(CONFIG_LCD_CLASS_DEVICE)
	struct platform_device *msm_pdev;
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
	struct display_status dstat;
 };

 struct dsi_cmd_desc_LCD {
	 int lux;
	 char strID[8];
	 struct dsi_cmd_desc *cmd;
 };

 enum {
	 GAMMA_20CD,
	 GAMMA_30CD,
	 GAMMA_40CD,
	 GAMMA_50CD,
	 GAMMA_60CD,
	 GAMMA_70CD,
	 GAMMA_80CD,
	 GAMMA_90CD,
	 GAMMA_100CD,
	 GAMMA_102CD,
	 GAMMA_104CD,
	 GAMMA_106CD,
	 GAMMA_108CD,
	 GAMMA_110CD,
	 GAMMA_120CD,
	 GAMMA_130CD,
	 GAMMA_140CD,
	 GAMMA_150CD,
	 GAMMA_160CD,
	 GAMMA_170CD,
	 GAMMA_180CD,
	 GAMMA_182CD,
	 GAMMA_184CD,
	 GAMMA_186CD,
	 GAMMA_188CD,
	 GAMMA_190CD,
	 GAMMA_200CD,
	 GAMMA_210CD,
	 GAMMA_220CD,
	 GAMMA_230CD,
	 GAMMA_240CD,
	 GAMMA_250CD,
	 GAMMA_260CD,
	 GAMMA_270CD,
	 GAMMA_280CD,
	 GAMMA_290CD,
	 GAMMA_300CD,
 };


 enum mipi_samsung_cmd_list {

	PANEL_READY_TO_ON,
	PANEL_DISP_OFF,
	PANEL_DISPLAY_ON,
	PANEL_DISPLAY_OFF,
	PANEL_DISPLAY_UNBLANK,
	PANEL_DISPLAY_BLANK,
	PANEL_ALL_PIXEL_OFF,
	PANEL_BRIGHT_CTRL,
	PANEL_MTP_ENABLE,
	PANEL_MTP_DISABLE,
	PANEL_NEED_FLIP,
	PANEL_ACL_OFF,
	PANEL_ACL_ON,
        PANEL_ACL_UPDATE,
	PANEL_LATE_ON,
	PANEL_EARLY_OFF,
	PANEL_TOUCHSENSING_ON,
	PANEL_TOUCHSENSING_OFF,
	PANEL_TEAR_ON,
	PANEL_TEAR_OFF,
	PANEL_LDI_FPS_CHANGE,
	PANEL_LDI_SET_VDDM_OFFSET, /*LDI_ADJ_VDDM_OFFSET*/
	PANEL_PARTIAL_ON,
	PANEL_PARTIAL_OFF	
};

void mdnie_lite_tuning_init(struct mdss_samsung_driver_data* msd);
void mdss_dsi_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *cmds, int cnt,int flag);
#endif
