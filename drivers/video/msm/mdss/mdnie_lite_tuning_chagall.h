/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifndef _MDNIE_LITE_TUNING_CHAGALL_H_
#define _MDNIE_LITE_TUNING_CHAGALL_H_

#include "mdss_samsung_dual_oled_cmd_wqxga_panel.h"

enum SCENARIO {
	mDNIe_UI_MODE,
	mDNIe_VIDEO_MODE,
	mDNIe_VIDEO_WARM_MODE,
	mDNIe_VIDEO_COLD_MODE,
	mDNIe_CAMERA_MODE,
	mDNIe_NAVI,
	mDNIe_GALLERY,
	mDNIe_VT_MODE,
	mDNIe_BROWSER_MODE,
	mDNIe_eBOOK_MODE,
	mDNIe_EMAIL_MODE,
#if defined(CONFIG_TDMB)
	mDNIe_DMB_MODE = 20,
	mDNIe_DMB_WARM_MODE,
	mDNIe_DMB_COLD_MODE,
#endif
	MAX_mDNIe_MODE,
#ifdef BROWSER_COLOR_TONE_SET
	mDNIe_BROWSER_TONE1 = 40,
	mDNIe_BROWSER_TONE2,
	mDNIe_BROWSER_TONE3,
#endif
};

enum BACKGROUND {
	DYNAMIC_MODE = 0,
	STANDARD_MODE,
	MOVIE_MODE,
	NATURAL_MODE,
	AUTO_MODE,
	MAX_BACKGROUND_MODE,
};

enum OUTDOOR {
	OUTDOOR_OFF_MODE = 0,
	OUTDOOR_ON_MODE,
	MAX_OUTDOOR_MODE,
};

enum ACCESSIBILITY {
    ACCESSIBILITY_OFF,
	NEGATIVE,
	COLOR_BLIND,
	SCREEN_CURTAIN,
	ACCESSIBILITY_MAX,
};

#if defined(CONFIG_TDMB)
enum DMB {
	DMB_MODE_OFF = -1,
	DMB_MODE,
	DMB_WARM_MODE,
	DMB_COLD_MODE,
	MAX_DMB_MODE,
};
#endif

struct mdnie_lite_tun_type {
	bool mdnie_enable;
	enum SCENARIO scenario;
	enum BACKGROUND background;
	enum OUTDOOR outdoor;
	enum ACCESSIBILITY accessibility;
#if defined(CONFIG_TDMB)
	enum DMB dmb;
#endif
	int scr_white_red;
	int scr_white_green;
	int scr_white_blue;
};

void mdnie_lite_tuning_init(struct mipi_samsung_driver_data *msd);
void init_mdnie_class(void);
void is_negative_on(void);
void coordinate_tunning(int x, int y);
void mDNIe_Set_Mode(void);

#endif /*_MDNIE_LITE_TUNING_H_*/
