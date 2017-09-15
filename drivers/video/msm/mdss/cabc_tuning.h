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

#include <linux/mutex.h>

#include "mdss_dsi.h"

#ifndef _CABC_TUNING_H_
#define _CABC_TUNING_H_

#define MAX_FILE_NAME 128
#define TUNING_FILE_PATH "/sdcard/"

#define DPRINT(x...)	printk(KERN_ERR "[CABC Tuning] " x)

#define CABC_TUNE_FIRST_SIZE 7
#define CABC_TUNE_SECOND_SIZE 7
#define CABC_TUNE_THIRD_SIZE 18
#define CABC_TUNE_FOURTH_SIZE 36
#define CABC_TUNE_SELECT_SIZE 2

#define PAYLOAD1 cabc_tune_cmd[0]
#define PAYLOAD2 cabc_tune_cmd[1]
#define PAYLOAD3 cabc_tune_cmd[2]
#define PAYLOAD4 cabc_tune_cmd[3]
#define SELECT cabc_tune_cmd[4]

#define CABC_OFF 0
#define CABC_ON 1

#define INPUT_PAYLOAD1(x) PAYLOAD1.payload = x
#define INPUT_PAYLOAD2(x) PAYLOAD2.payload = x
#define INPUT_PAYLOAD3(x) PAYLOAD3.payload = x
#define INPUT_PAYLOAD4(x) PAYLOAD4.payload = x
#define INPUT_SELECT(x) SELECT.payload = x



enum LuxValue {
	CABC_LUX_0,	/* 0 ~ 500 Lux */
	CABC_LUX_1,	/* 500 ~ 5000 Lux */
	CABC_LUX_2,	/* 5000 ~ Lux */
	CABC_LUX_MAX,
};

enum CABC_UI {
	CABC_MODE_UI,
	CABC_MODE_VIDEO,
	CABC_MODE_MAX,
};

enum CABC_Negative {
	CABC_NEGATIVE_OFF = 0,
	CABC_NEGATIVE_ON,
};

enum CABC_Auto_Br {
	CABC_AUTO_BR_OFF = 0,
	CABC_AUTO_BR_ON,
	CABC_AUTO_BR_MAX
};

enum ACCESSIBILITY {
	ACCESSIBILITY_OFF = 0,
	NEGATIVE,
	COLOR_BLIND,
	ACCESSIBILITY_MAX,
};

struct cabc_tun_type {
	struct mutex cabc_mutex;
	bool cabc_enable;
	enum CABC_Auto_Br auto_br;
	enum LuxValue luxvalue;
	enum CABC_UI mode;
	enum CABC_Negative negative;
};

void cabc_tuning_init(struct mdss_dsi_ctrl_pdata *dsi_pdata);
void CABC_Set_Mode(void);
void update_lux(unsigned int input_lux);
int get_panel_power_state(void);

#endif

