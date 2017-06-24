/* Copyright (c) 2012, Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef JDI_DSI_PANEL_H
#define JDI_DSI_PANEL_H
#define MAX_PANEL_NAME_SIZE 100
#define RECOVERY_BRIGHTNESS 180
#define LCD_DEBUG(X, ...) pr_info("[LCD]%s:"X, __func__, ## __VA_ARGS__);

#define AUTO_BRIGHTNESS_CABC_FUNCTION

#include "smart_dimming.h"

enum mipi_samsung_cmd_list {

	PANEL_READY_TO_ON, // 0
	PANEL_DISP_OFF, // 1
	PANEL_DISPLAY_ON, // 2
	PANEL_DISPLAY_OFF, // 3
	PANEL_DISPLAY_UNBLANK, // 4
	PANEL_DISPLAY_BLANK, // 5
	PANEL_ALL_PIXEL_OFF, // 6
	PANEL_BRIGHT_CTRL, // 7
	PANEL_MTP_ENABLE, // 8
	PANEL_MTP_DISABLE, // 9
	PANEL_NEED_FLIP, // 10
#if defined(AUTO_BRIGHTNESS_CABC_FUNCTION)
	PANEL_CABC_ENABLE,//11
	PANEL_CABC_DISABLE,//12
#endif
	PANEL_LATE_ON, // 13
	PANEL_EARLY_OFF, // 14
	PANEL_TOUCHSENSING_ON, // 15
	PANEL_TOUCHSENSING_OFF, // 16
	PANEL_TEAR_ON, // 17
	PANEL_TEAR_OFF, //18
	PANEL_HSYNC_ON, // 19
	PANEL_HSYNC_OFF, // 20
};
enum {
	MIPI_RESUME_STATE,
	MIPI_SUSPEND_STATE,
};
struct cmd_map {
	int *bl_level;
	int *cmd_idx;
	int size;
};

struct candella_lux_map {
	int *lux_tab;
	char *pwm_tab;
	char *cabc_pwm_tab;
	int *cmd_idx;
	int lux_tab_size;
	int bkl[256];
};

struct display_status {
	unsigned char cabc_on;
	unsigned char is_smart_dim_loaded;
	unsigned char is_mdnie_loaded;
	unsigned char auto_brightness;
	unsigned char recovery_boot_mode;
	unsigned char on;
	unsigned char wait_disp_on;
	int curr_elvss_idx;
	int curr_cabc_idx;
	int curr_aid_idx;
	int curr_gamma_idx;
	int bright_level;

	int temperature;
	char temperature_value;
	int temper_need_update;
	int siop_status;
	int hbm_mode;
};

struct mipi_samsung_driver_data {
	struct display_status dstat;

	struct msm_fb_data_type *mfd;
	struct mdss_panel_data *pdata;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata;

	struct mutex lock;

#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
	char panel_name[MAX_PANEL_NAME_SIZE];
	int panel;
	unsigned int manufacture_id;
	struct smartdim_conf *sdimconf;
};

enum {
	PANEL_TFT_FHD_S6D6FA0,
};

struct panel_hrev {
	char *name;
	int panel_code;
};

#endif
void mdss_dsi_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *cmds, int cnt, int flag);
static struct mipi_samsung_driver_data msd;
extern int mipi_samsung_cabc_onoff ( int cabc_set );