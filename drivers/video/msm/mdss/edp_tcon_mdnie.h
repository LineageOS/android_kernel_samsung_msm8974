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
#ifndef _EDP_TCON_MDNIE_H_
#define _EDP_TCON_MDNIE_H_

#define NAME_STRING_MAX 30

extern char mdnie_app_name[][NAME_STRING_MAX];
extern char mdnie_mode_name[][NAME_STRING_MAX];

enum BYPASS {
	BYPASS_DISABLE = 0,
	BYPASS_ENABLE,
};

enum APP {
	UI_APP = 0,
	VIDEO_APP,
	VIDEO_WARM_APP,
	VIDEO_COLD_APP,
	CAMERA_APP,
	NAVI_APP,
	GALLERY_APP,
	VT_APP,
	BROWSER_APP,
	eBOOK_APP,
	EMAIL_APP,
	MAX_APP_MODE,
};

enum MODE {
	DYNAMIC_MODE = 0,
	STANDARD_MODE,
#if defined(NATURAL_MODE_ENABLE)
	NATURAL_MODE,
#endif
	MOVIE_MODE,
	AUTO_MODE,
	MAX_MODE,
};

enum ACCESSIBILITY {
	ACCESSIBILITY_OFF = 0,
	NEGATIVE,
	COLOR_BLIND,
	ACCESSIBILITY_MAX,
};

struct mdnie_lite_tun_type {
	int mdnie_enable;
	enum BYPASS mdnie_bypass;
	enum BYPASS cabc_bypass;

	enum APP mdnie_app;
	enum MODE mdnie_mode;
	enum ACCESSIBILITY mdnie_accessibility;
};

extern int mdnie_tune_cmd(short *tune_data, int len);
void init_mdnie_class(void);
int update_mdnie_register(void);
#endif /*_EDP_TCON_MDNIE_H_*/
