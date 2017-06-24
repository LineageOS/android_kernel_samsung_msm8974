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
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/qpnp/pin.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/pwm.h>
#include <linux/err.h>
#if defined (CONFIG_LCD_CLASS_DEVICE)
	#include <linux/lcd.h>
#endif
#include "mdss_fb.h"
//#include "mdnie_lite_tuning.h"
#include "mdss_video_enhance.h"
#include "mdss_dsi.h"
#include "mdss_s6e8aa0a_panel.h"
#include "dlog.h"
#include "mdss_mdp.h"
#include "mdss_mdp_rotator.h"


#if defined(CONFIG_LCD_CONNECTION_CHECK)
int lcd_connected_status;
#endif
#define DT_CMD_HDR 6
/* Needed to pass GPIO DVS for OLED_DET GPIO */

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
struct work_struct  err_fg_work;
static int err_fg_gpio;
static int esd_count;
static int esd_irq_on;
int err_fg_working;
#define ESD_DEBUG 1
#endif

static int panel_state;


#define GAMMA_SMART 2

DEFINE_LED_TRIGGER(bl_led_trigger);

static struct mdss_samsung_driver_data msd;
static struct mdss_panel_data mpd;

static int first_boot;

struct mutex bg_lock;
extern int poweroff_charging;
#if defined (CONFIG_LCD_CONNECTION_CHECK)
static int lcd_id;
#endif
static int lcd_id3;




static char elvss_cond_set[] = {
	0xB1, 0x04, 0x00,

/*	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,*/
};

static char panel_cond_aid_ref[] = {
	0xF8,
	0x19, 0x35, 0x00, 0x00, 0x00,
	0x94, 0x00, 0x3C, 0x7D, 0x10,
	0x27, 0x08, 0x6E, 0x00, 0x00,
	0x00, 0x00, 0x04, 0x08, 0x6E,
	0x00, 0x00, 0x00, 0x00, 0x08,
	0x08, 0x23, 0x37, 0xC0, 0xC1,
	0x01, 0x81, 0xC1, 0x00, 0xC3,
	0xF6, 0xF6, 0xC1,

/*	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,*/
};

static char etc_cond_set3_aid_ref[] = {
	0xD9,
	0x14, 0x40, 0x0C, 0xCB, 0xCE,
	0x6E, 0xC4, 0x07, 0xC0, 0x41,
	0xC1, 0x00, 0x60, 0x19,

/*	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,*/
};

static char oled_gamma_7500K[] = {
	0xFA,
	0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,

/*	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,*/
};

static char gamma_set_update[] = {
	0xF7,
	0x03,

/*	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,*/
};


static char acl_set_zero[] = {
	0xC1,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};



static char acl_on[] = {
	0xC0,
	0x01,

	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

static char acl_off[] = {
	0xC1,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
};


static char ACL_COND_SET_40[] = {
	0xC1,
	0x47, 0x53, 0x13, 0x53, 0x00, 0x00,
	0x02, 0xCF, 0x00, 0x00, 0x04, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x07, 0x0C, 0x12, 0x17, 0x1D,
	0x23, 0x28, 0x2E, 0x33, 0x39,

/*	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,*/
};

static char ACL_COND_SET_33[] = {
	0xC1,
	0x47, 0x53, 0x13, 0x53, 0x00, 0x00,
	0x02, 0xCF, 0x00, 0x00, 0x04, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x06, 0x0A, 0x0F, 0x14, 0x19,
	0x1D, 0x22, 0x27, 0x2B, 0x30,


};


static int lux_tbl_acl[] = {
	20, 30, 40, 50, 60,
	70, 80, 90, 100, 102, 104, 106, 108, 110,
	120, 130, 140, 150, 160,
	170, 180, 182, 184, 186, 188,190, 200, 210,
	220, 230, 240, 250, 260,
	270, 280, 290, 300
};

static char GAMMA_SmartDimming_COND_SET[] = {
	0xFA,
	0x01, 0x4A, 0x01, 0x4D, 0x7A,
	0x5D, 0xA5, 0x9C, 0xCA, 0xA4,
	0xBD, 0xDC, 0xBE, 0x93, 0xBD,
	0x95, 0xBA, 0xD2, 0xB7, 0x00,
	0x81, 0x00, 0x75, 0x00, 0xA5,

};

static char gamma_cond_set_4_8[] = {
	0xFA,
	0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
};

static char gamma_cond_300cd_4_8[] = {
	0x43, 0x14, 0x45, 0xAD,
	0xBE, 0xA9, 0xB0, 0xC3, 0xAF,
	0xC1, 0xCD, 0xC0, 0x95, 0xA2,
	0x91, 0xAC, 0xB5, 0xAA, 0x00,
	0xB0, 0x00, 0xA0, 0x00, 0xCC
};


#define LCD_ELVSS_RESULT_LIMIT_4_8 (0x9F)

static int GET_ELVSS_ID_4_8[] = {
	0x0C,// 0 = 20_dimming,
	0x0C,// 1 = 30
	0x0C,  // 2 = 40
	0x0C,//3
	0x0C,//4
	0x0C,//5 = 70
	0x0C,//6 = 80
	0x0C,//7 = 90
	0x0C,//8 = 100
	0x0C,//8 = 102
	0x0C,//8 = 104
	0x0C,//8 = 106
	0x0C,//8 = 108
	0x0C,//9 = 110,
	0x0B,//10= 120,
	0x0A,//11= 130,
	0x09,//12= 140,
	0x08,//13= 150,
	0x07,//14= 160,
	0x06,//15= 170,
	0x05,//16= 180,
	0x05,//16= 182,
	0x05,//16= 184,
	0x05,//16= 186,
	0x05,//16= 188,
	0x0a,//17= 190,
	0x09,//18= 200,
	0x08,//19= 210,
	0x07,//20= 220,
	0x06,//21= 230,
	0x06,//22= 240,
	0x05,//23= 250,
	0x04,//24= 260,
	0x03,//25= 270,
	0x02,// 26= 280,
	0x01,//27= 290,
	0x00//28= 300,
	};


static int GET_SMART_ACL_ID_4_8[] = {
	0x11,// 0 = 20_dimming,
	0x11,// 1 = 30
	0x11,  // 2 = 40
	0x11,//3
	0x11,//4
	0x11,//5 = 70
	0x11,//6 = 80
	0x11,//7 = 90
	0x11,//8 = 100
	0x11,//8 = 102
	0x11,//8 = 104
	0x11,//8 = 106
	0x11,//8 = 108
	0x11,//9 = 110,
	0x10,//10= 120,
	0x0F,//11= 130,
	0x0E,//12= 140,
	0x0D,//13= 150,
	0x0C,//14= 160,
	0x0B,//15= 170,
	0x0A,//16= 180,
	0x0A,//16= 180,
	0x0A,//16= 184,
	0x0A,//16= 186,
	0x0A,//16= 188,
	0x0F,//17= 190,
	0x0E,//18= 200,
	0x0D,//19= 210,
	0x0C,//20= 220,
	0x0B,//21= 230,
	0x0B,//22= 240,
	0x0A,//23= 250,
	0x09,//24= 260,
	0x08,//25= 270,
	0x07,// 26= 280,
	0x06,//27= 290,
	0x05//28= 300,
	};



static struct dsi_cmd_desc combined_ctrl[] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	 sizeof(oled_gamma_7500K)}, oled_gamma_7500K}
	,
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	 sizeof(panel_cond_aid_ref)}, panel_cond_aid_ref}
	,
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	 sizeof(etc_cond_set3_aid_ref)}, etc_cond_set3_aid_ref}
	,
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	 sizeof(gamma_set_update)}, gamma_set_update}
	,
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	 sizeof(elvss_cond_set)}, elvss_cond_set}
	,
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	sizeof(acl_on)},	acl_on}
	,
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	sizeof(ACL_COND_SET_40)}, ACL_COND_SET_40}
	,
};



static struct dsi_cmd_desc samsung_panel_gamma_update_cmds[4] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	 sizeof(oled_gamma_7500K)}, oled_gamma_7500K}
	,
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	 sizeof(panel_cond_aid_ref)}, panel_cond_aid_ref}
	,
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	 sizeof(etc_cond_set3_aid_ref)}, etc_cond_set3_aid_ref}
	,
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	 sizeof(gamma_set_update)}, gamma_set_update}
	,
};

static struct dsi_cmd_desc samsung_panel_elvss_update_cmds[] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	 sizeof(elvss_cond_set)}, elvss_cond_set},
};

static struct dsi_cmd_desc samsung_panel_elvss_update_cmds_4_8[] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	 sizeof(elvss_cond_set)}, elvss_cond_set},
};


static struct dsi_cmd_desc samsung_panel_acl_on_cmds[] = {
#if defined(CONFIG_MACH_S3VE3G_EUR)
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0,
		sizeof(acl_set_zero)}, acl_set_zero},
#else
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(acl_set_zero)}, acl_set_zero},
#endif
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(acl_on)}, acl_on}
};

static struct dsi_cmd_desc samsung_panel_acl_off_cmds[] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(acl_off)}, acl_off},
};

static struct dsi_cmd_desc samsung_panel_acl_update_cmds[] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
	 sizeof(ACL_COND_SET_40)}, ACL_COND_SET_40}
	,
};



static struct dsi_cmd_desc DSI_CMD_ACL_40 = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(ACL_COND_SET_40)}, ACL_COND_SET_40 };
static struct dsi_cmd_desc DSI_CMD_ACL_33 = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(ACL_COND_SET_33)}, ACL_COND_SET_33 };

static struct dsi_cmd_desc_LCD lcd_acl_table_4_8[] = {
	{0, "20", NULL},
	{33, "30", &DSI_CMD_ACL_33},
	{40, "40", &DSI_CMD_ACL_40},
	{40, "50", &DSI_CMD_ACL_40},
	{40, "60", &DSI_CMD_ACL_40},
	{40, "70", &DSI_CMD_ACL_40},
	{40, "80", &DSI_CMD_ACL_40},
	{40, "90", &DSI_CMD_ACL_40},
	{40, "100", &DSI_CMD_ACL_40},
	{40, "102", &DSI_CMD_ACL_40},
	{40, "104", &DSI_CMD_ACL_40},
	{40, "106", &DSI_CMD_ACL_40},
	{40, "108", &DSI_CMD_ACL_40},
	{40, "110", &DSI_CMD_ACL_40},
	{40, "120", &DSI_CMD_ACL_40},
	{40, "130", &DSI_CMD_ACL_40},
	{40, "140", &DSI_CMD_ACL_40},
	{40, "150", &DSI_CMD_ACL_40},
	{40, "160", &DSI_CMD_ACL_40},
	{40, "170", &DSI_CMD_ACL_40},
	{40, "180", &DSI_CMD_ACL_40},
	{40, "182", &DSI_CMD_ACL_40},
	{40, "184", &DSI_CMD_ACL_40},
	{40, "186", &DSI_CMD_ACL_40},
	{40, "188", &DSI_CMD_ACL_40},
	{40, "190", &DSI_CMD_ACL_40},
	{40, "200", &DSI_CMD_ACL_40},
	{40, "210", &DSI_CMD_ACL_40},
	{40, "220", &DSI_CMD_ACL_40},
	{40, "230", &DSI_CMD_ACL_40},
	{40, "240", &DSI_CMD_ACL_40},
	{40, "250", &DSI_CMD_ACL_40},
	{40, "260", &DSI_CMD_ACL_40},
	{40, "270", &DSI_CMD_ACL_40},
	{40, "280", &DSI_CMD_ACL_40},
	{40, "290", &DSI_CMD_ACL_40},
	{40, "300", &DSI_CMD_ACL_40},
};

void mdss_dsi_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *cmds, int cnt,int flag);
static int mdss_dsi_panel_off(struct mdss_panel_data *pdata);
static int mdss_dsi_panel_on(struct mdss_panel_data *pdata);
static void mdss_dsi_panel_bl_ctrl(struct mdss_panel_data *pdata,u32 bl_level);


static int get_candela_index(int bl_level)
{
	int backlightlevel;

	//brightness setting from platform is from 0 to 255
	 // But in this driver, brightness is only supported from 0 to 24

	switch (bl_level) {
	case 0 ... 20:
		backlightlevel = GAMMA_20CD;
		break;
	case 21 ... 39:
		backlightlevel = GAMMA_30CD;
		break;
	case 40 ... 49:
		backlightlevel = GAMMA_40CD;
		break;
	case 50 ... 59:
		backlightlevel = GAMMA_50CD;
		break;
	case 60 ... 69:
		backlightlevel = GAMMA_60CD;
		break;
	case 70 ... 79:
		backlightlevel = GAMMA_70CD;
		break;
	case 80 ... 89:
		backlightlevel = GAMMA_80CD;
		break;
	case 90 ... 99:
		backlightlevel = GAMMA_90CD;
		break;
	case 100 ... 101:
		backlightlevel = GAMMA_100CD;
		break;
	case 102 ... 103:
		backlightlevel = GAMMA_102CD;
		break;
	case 104 ... 105:
		backlightlevel = GAMMA_104CD;
		break;
	case 106 ... 107:
		backlightlevel = GAMMA_106CD;
		break;
	case 108 ... 109:
		backlightlevel = GAMMA_108CD;
		break;
	case 110 ... 119:
		backlightlevel = GAMMA_110CD;
		break;
	case 120 ... 129:
		backlightlevel = GAMMA_120CD;
		break;
	case 130 ... 139:
		backlightlevel = GAMMA_130CD;
		break;
	case 140 ... 149:
		backlightlevel = GAMMA_140CD;
		break;
	case 150 ... 159:
		backlightlevel = GAMMA_150CD;
		break;
	case 160 ... 169:
		backlightlevel = GAMMA_160CD;
		break;
	case 170 ... 179:
		backlightlevel = GAMMA_170CD;
		break;
	case 180 ... 181:
		backlightlevel = GAMMA_180CD;
		break;
	case 182 ... 183:
		backlightlevel = GAMMA_182CD;
		break;
	case 184 ... 185:
		backlightlevel = GAMMA_184CD;
		break;
	case 186 ... 187:
		backlightlevel = GAMMA_186CD;
		break;
	case 188 ... 189:
		backlightlevel = GAMMA_188CD;
		break;
	case 190 ... 199:
		backlightlevel = GAMMA_190CD;
		break;
	case 200 ... 209:
		backlightlevel = GAMMA_200CD;
		break;
	case 210 ... 214:
		backlightlevel = GAMMA_210CD;
		break;
	case 215 ... 219:
		backlightlevel = GAMMA_210CD;
		break;
	case 220 ... 224:
		backlightlevel = GAMMA_220CD;
		break;
	case 225 ... 229:
		backlightlevel = GAMMA_220CD;
		break;
	case 230 ... 234:
		backlightlevel = GAMMA_230CD;
		break;
	case 235 ... 239:
		backlightlevel = GAMMA_230CD;
		break;
	case 240 ... 244:
		backlightlevel = GAMMA_240CD;
		break;
	case 245 ... 249:
		backlightlevel = GAMMA_240CD;
		break;
	case 250 ... 254:
		backlightlevel = GAMMA_250CD;
		break;
	case 255:
		if (msd.dstat.auto_brightness > 3)
			backlightlevel = GAMMA_300CD;
		else
			backlightlevel = GAMMA_250CD;
		break;
	default:
		backlightlevel = GAMMA_20CD;
		break;
	}
	return backlightlevel;
}

static int is_acl_para_change(int bl_level)
{
	int cd = get_candela_index(bl_level);
	int change = 0;

	if (!lcd_acl_table_4_8[cd].lux)
		return 0;

	change = memcmp(samsung_panel_acl_update_cmds[0].payload,
			lcd_acl_table_4_8[cd].cmd->payload,
			lcd_acl_table_4_8[cd].cmd->dchdr.dlen);
	return change;
}

static int set_elvss_level_4_8(int bl_level)
{
	unsigned char calc_elvss;
	int cd;
	unsigned char elvss_pulse;
	cd = get_candela_index(bl_level);
	elvss_pulse = mpd.lcd_elvss_data[0];

	if (msd.dstat.acl_on)
	{
		calc_elvss = elvss_pulse + GET_SMART_ACL_ID_4_8[cd];
	}
	else
	{
	        calc_elvss = elvss_pulse + GET_ELVSS_ID_4_8[cd];
	}

	pr_debug("%s: elvss_pulse=0x%x,calc_elvss = 0x%x\n", __func__, elvss_pulse,calc_elvss);
	if (calc_elvss > LCD_ELVSS_RESULT_LIMIT_4_8)
		calc_elvss = LCD_ELVSS_RESULT_LIMIT_4_8;

	if (elvss_cond_set[2] == calc_elvss)
		return 1;

	elvss_cond_set[2] = calc_elvss;

	return 0;
}




#define aid_ratio_index 18
static int aid_below_110_ratio_table[10][2] = {
	//	RATIO
	{20,	0x88},
	{30,	0x7A},
	{40,	0x6C},
	{50,	0x5E},
	{60,	0x50},
	{70,	0x41},
	{80,	0x32},
	{90,	0x22},
	{100,	0x12},
	//upper 110CD
	{110,	0x42},
};
static int  aid_operation(int lux)
{
	int index;
	int ratio;
	static int aid_status;
	char panel_cond_aid_ref_1 = panel_cond_aid_ref[1] ;
	char panel_cond_aid_ref_18 = panel_cond_aid_ref[18];
	char etc_cond_set3_aid_ref_9 = etc_cond_set3_aid_ref[9];
	int no_change = 0;


		if (lux == 0) {
			panel_cond_aid_ref[1] = 0x19;
			panel_cond_aid_ref[18] = 0x04;
			etc_cond_set3_aid_ref[9] = 0x40;
			aid_status = 0;
		} else if (lux >= 190) {
			panel_cond_aid_ref[1] = 0x19;
			panel_cond_aid_ref[18] = 0x04;
			etc_cond_set3_aid_ref[9] = 0xC0;
			aid_status = 0;
		} else if (lux >= 188) {
			panel_cond_aid_ref[1] = 0x59;
			panel_cond_aid_ref[18] = 0x0D;
			etc_cond_set3_aid_ref[9] = 0xC0;
			aid_status = 0;
		} else if (lux >= 186) {
			panel_cond_aid_ref[1] = 0x59;
			panel_cond_aid_ref[18] = 0x1A;
			etc_cond_set3_aid_ref[9] = 0xC0;
			aid_status = 0;
		} else if (lux >= 184) {
			panel_cond_aid_ref[1] = 0x59;
			panel_cond_aid_ref[18] = 0x27;
			etc_cond_set3_aid_ref[9] = 0xC0;
			aid_status = 0;
		} else if (lux >= 182) {
			panel_cond_aid_ref[1] = 0x59;
			panel_cond_aid_ref[18] = 0x34;
			etc_cond_set3_aid_ref[9] = 0xC0;
			aid_status = 0;
		} else if (lux >= 110) {
			ratio = aid_below_110_ratio_table[9][1];
			panel_cond_aid_ref[1] = 0x59;
			panel_cond_aid_ref[18] = ratio;
			etc_cond_set3_aid_ref[9] = 0xC0;
			aid_status = 1;
		} else if (lux >= 108) {
			panel_cond_aid_ref[1] = 0x59;
			panel_cond_aid_ref[18] = 0x3D;
			etc_cond_set3_aid_ref[9] = 0xC0;
			aid_status = 1;
		} else if (lux >= 106) {

			panel_cond_aid_ref[1] = 0x59;
			panel_cond_aid_ref[18] = 0x34;
			etc_cond_set3_aid_ref[9] = 0xC0;
			aid_status = 1;
		} else if (lux >= 104) {

			panel_cond_aid_ref[1] = 0x59;
			panel_cond_aid_ref[18] = 0x2B;
			etc_cond_set3_aid_ref[9] = 0xC0;
			aid_status = 1;
		} else if (lux >= 102) {

			panel_cond_aid_ref[1] = 0x59;
			panel_cond_aid_ref[18] = 0x21;
			etc_cond_set3_aid_ref[9] = 0xC0;
			aid_status = 1;
		} else {
			index = (lux / 10) - 2;
			ratio = aid_below_110_ratio_table[index][1];
			panel_cond_aid_ref[1] = 0x59;
			panel_cond_aid_ref[18] = ratio;
			etc_cond_set3_aid_ref[9] = 0xC0;
			aid_status = 1;
		}
		pr_debug("%s brightness_level : %d adi_status:%d\n", __func__,lux, aid_status);

	no_change = (panel_cond_aid_ref_1 == panel_cond_aid_ref[1])
		&& (panel_cond_aid_ref_18 == panel_cond_aid_ref[18])
		&& (etc_cond_set3_aid_ref_9 == etc_cond_set3_aid_ref[9]);

	return !no_change;
}



static int set_gamma_level(int bl_level, unsigned char gamma_mode)
{
	int cd;
	int *lux_tbl = lux_tbl_acl;
	int aid_change = 0;

	cd = get_candela_index(bl_level);

	if (mpd.lcd_current_cd_idx == cd) {
		pr_info("backlight is not change\n");
		return -1;
	} else
	    msd.mpd->lcd_current_cd_idx = cd;

	if (gamma_mode == GAMMA_SMART) {

		//  SMART Dimming gamma_lux;
		char pBuffer[256];
		int i;
		int gamma_lux;

		gamma_lux = lux_tbl[cd];

		if (gamma_lux > SmartDimming_CANDELA_UPPER_LIMIT)
			gamma_lux = SmartDimming_CANDELA_UPPER_LIMIT;

		for (i = SmartDimming_GammaUpdate_Pos;
		     i < sizeof(GAMMA_SmartDimming_COND_SET); i++)
			GAMMA_SmartDimming_COND_SET[i] = 0;


		mpd.smart_s6e8aa0x01.brightness_level = gamma_lux;


			generate_gamma(&mpd.smart_s6e8aa0x01,
			&(GAMMA_SmartDimming_COND_SET[2]), GAMMA_SET_MAX);

		aid_change = aid_operation(gamma_lux);

		samsung_panel_gamma_update_cmds[0].dchdr.dlen =
		    sizeof(GAMMA_SmartDimming_COND_SET);
		samsung_panel_gamma_update_cmds[0].payload =
		    GAMMA_SmartDimming_COND_SET;
		pBuffer[0] = 0;
		for (i = 0; i < sizeof(GAMMA_SmartDimming_COND_SET); i++) {
			snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %02x",
				 GAMMA_SmartDimming_COND_SET[i]);
		}
		pr_debug("SD: %03d %s\n", gamma_lux, pBuffer);
		pr_info("bl_level:%d,cd:%d:Candela:%d aid_change:%d\n",bl_level, cd, gamma_lux, aid_change);
	}
	return aid_change;
}


static int set_acl_on_level(int bl_level)
{
	int cd;
	cd = get_candela_index(bl_level);

	if (!lcd_acl_table_4_8[cd].lux)
		return 1;

		samsung_panel_acl_update_cmds[0].dchdr.dlen =
		    lcd_acl_table_4_8[cd].cmd->dchdr.dlen;
		samsung_panel_acl_update_cmds[0].payload =
		    lcd_acl_table_4_8[cd].cmd->payload;
	return 0;
}


static int prepare_brightness_control_cmd_array(int lcd_type, int bl_level)
{
	int cmd_size = 0, aid_change = 0;
	unsigned char cmds_send_flag = 0;

	aid_change = set_gamma_level(bl_level, msd.dstat.gamma_mode);

	if (aid_change < 0)
		return -1;

	cmds_send_flag |= aid_change << 0;

	if (!set_elvss_level_4_8(bl_level)){
		cmds_send_flag |= 1<<1;
	}

	if (msd.dstat.acl_on) {
		int acl_change = is_acl_para_change(bl_level);
		int acl_30_40_case = set_acl_on_level(bl_level);
		if (acl_30_40_case && mpd.ldi_acl_stat == true) {

			cmds_send_flag |= 1<<2;
			mpd.ldi_acl_stat = false;
		}
		if (!acl_30_40_case) {
				if (mpd.ldi_acl_stat == false) {

					cmds_send_flag |= 0x3<<3;
					mpd.ldi_acl_stat = true;

				} else if (acl_change)
					cmds_send_flag |= 1<<4;
		}
	}

	if (cmds_send_flag & 0x4){
		combined_ctrl[cmd_size].payload = acl_off;
		combined_ctrl[cmd_size].dchdr.dlen = sizeof(acl_off);
		cmd_size++;
	}

	if (cmds_send_flag & 0x10) { // acl update

		combined_ctrl[cmd_size].payload =
			samsung_panel_acl_update_cmds[0].payload;
		combined_ctrl[cmd_size].dchdr.dlen =
			samsung_panel_acl_update_cmds[0].dchdr.dlen;
		cmd_size++;
	}

	combined_ctrl[cmd_size].payload =
		samsung_panel_gamma_update_cmds[0].payload;
	combined_ctrl[cmd_size].dchdr.dlen =
		samsung_panel_gamma_update_cmds[0].dchdr.dlen;
	cmd_size++;

	combined_ctrl[cmd_size].payload = gamma_set_update;
	combined_ctrl[cmd_size].dchdr.dlen = sizeof(gamma_set_update);
	cmd_size++;

	if (cmds_send_flag & 0x1) {
		// aid change
		combined_ctrl[cmd_size].payload = panel_cond_aid_ref;
		combined_ctrl[cmd_size].dchdr.dlen = sizeof(panel_cond_aid_ref);
		cmd_size++;

		combined_ctrl[cmd_size].payload = etc_cond_set3_aid_ref;
		combined_ctrl[cmd_size].dchdr.dlen = sizeof(etc_cond_set3_aid_ref);
		cmd_size++;
	}

	if (cmds_send_flag & 0x2) { // elvss change

		combined_ctrl[cmd_size].payload =
			samsung_panel_elvss_update_cmds[0].payload;
		combined_ctrl[cmd_size].dchdr.dlen =
			samsung_panel_elvss_update_cmds[0].dchdr.dlen;
		cmd_size++;
	}

	mpd.combined_ctrl.size = cmd_size;

	return cmds_send_flag;
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

void reset_gamma_level(void)
{
	pr_debug("reset_gamma_level\n");
	mpd.lcd_current_cd_idx = -1;


	msd.mpd->ldi_acl_stat = false;
	elvss_cond_set[2] = 0x00;

	panel_cond_aid_ref[1] = 0xff;
	panel_cond_aid_ref[18] = 0xff;
	etc_cond_set3_aid_ref[9] = 0xff;

}

void mdss_dsi_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *cmds, int cnt,int flag)
{
	struct dcs_cmd_req cmdreq;
#if defined(CONFIG_LCD_CONNECTION_CHECK)
	if (is_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}
#endif

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = cmds;
	cmdreq.cmds_cnt = cnt;

	if (flag & CMD_REQ_SINGLE_TX) {
		cmdreq.flags = CMD_REQ_SINGLE_TX | CMD_CLK_CTRL | CMD_REQ_COMMIT;
	}else
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;

	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);

}


static void mdss_dsi_panel_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl,
			struct dsi_panel_cmds *pcmds)
{
	struct dcs_cmd_req cmdreq;
#if defined(CONFIG_LCD_CONNECTION_CHECK)
	if (is_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}
#endif
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = pcmds->cmds;
	cmdreq.cmds_cnt = pcmds->cmd_cnt;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
}


u32 mdss_dsi_cmd_receive(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *cmd, int rlen)
{
        struct dcs_cmd_req cmdreq;

        memset(&cmdreq, 0, sizeof(cmdreq));
        cmdreq.cmds = cmd;
        cmdreq.cmds_cnt = 1;
        cmdreq.flags = CMD_REQ_RX | CMD_REQ_COMMIT;
        cmdreq.rlen = rlen;
        cmdreq.cb = NULL;

        // This mutex is to sync up with dynamic FPS changes
    	// so that DSI lockups shall not happen

    	BUG_ON(msd.ctrl_pdata == NULL);
//    	mutex_lock(&msd.ctrl_pdata->dfps_mutex);
        mdss_dsi_cmdlist_put(ctrl, &cmdreq);
//      mutex_unlock(&msd.ctrl_pdata->dfps_mutex);

         // blocked here, untill call back called
        return ctrl->rx_buf.len;
}


static void read_reg(char srcReg, int srcLength, char *destBuffer,
	      const int isUseMutex, struct msm_fb_data_type *pMFD)
{
	const int one_read_size = 4;

	const int loop_limit = 16;
	// first byte = read-register
	static char read_reg[2] = { 0xFF, 0x00 };
	static struct dsi_cmd_desc s6e8aa0_read_reg_cmd[] = {{
	{DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(read_reg)}, read_reg}, };
	// first byte is size of Register
	static char packet_size[] = { 0x04, 0 };
	static struct dsi_cmd_desc s6e8aa0_packet_size_cmd[] = {{
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0, sizeof(packet_size)}, packet_size}, };

	// second byte is Read-position
	static char reg_read_pos[] = { 0xB0, 0x00 };
	static struct dsi_cmd_desc s6e8aa0_read_pos_cmd[] = {{
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(reg_read_pos)},reg_read_pos}, };

	struct dsi_cmd_desc *cmd_desc;
	int cmd_size = 0;

	int flag = 0;


	int read_pos;
	int readed_size;
	int show_cnt;

	int i, j;
	char show_buffer[256];
	int show_buffer_pos = 0;

	read_reg[0] = srcReg;

	show_buffer_pos +=
	    snprintf(show_buffer, 256, "read_reg : %X[%d] : ",
		 srcReg, srcLength);

	read_pos = 0;
	readed_size = 0;

	cmd_desc = s6e8aa0_packet_size_cmd;
	cmd_size = ARRAY_SIZE(s6e8aa0_packet_size_cmd);

	packet_size[0] = (char)srcLength;

	mdss_dsi_cmds_send(msd.ctrl_pdata, cmd_desc, cmd_size, flag);


	cmd_desc = s6e8aa0_read_pos_cmd;
	cmd_size = ARRAY_SIZE(s6e8aa0_read_pos_cmd);
	show_cnt = 0;
	for (j = 0; j < loop_limit; j++) {
		reg_read_pos[1] = read_pos;

		mdss_dsi_cmds_send(msd.ctrl_pdata, cmd_desc, cmd_size, flag);

		readed_size = mdss_dsi_cmd_receive(msd.ctrl_pdata, s6e8aa0_read_reg_cmd, one_read_size);
		for (i = 0; i < readed_size; i++, show_cnt++) {
			show_buffer_pos +=
			 snprintf(show_buffer + show_buffer_pos, 256, "%02x ",
				    msd.ctrl_pdata->rx_buf.data[i]);
			if (destBuffer != NULL && show_cnt < srcLength) {
				destBuffer[show_cnt] =
				    msd.ctrl_pdata->rx_buf.data[i];
			}
		}
		show_buffer_pos += snprintf(show_buffer +
			show_buffer_pos, 256, ".");
		read_pos += readed_size;

		if (read_pos > srcLength)
			break;

	}

	if (j == loop_limit)
		show_buffer_pos +=
		    snprintf(show_buffer + show_buffer_pos, 256, "Overrun");

	pr_debug("%s\n", show_buffer);
}



static int find_mtp(struct mdss_panel_data *pdata)
{

	char first_mtp[MTP_DATA_SIZE]={0,};
	char second_mtp[MTP_DATA_SIZE]={0,};
	char third_mtp[MTP_DATA_SIZE]={0,};
	int mtp_size = MTP_DATA_SIZE;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	char pBuffer[256] = {0,};
       	int i;
	struct msm_fb_data_type *mfd;
       	char *mtp;
	int correct_mtp = 0;

#if defined(CONFIG_LCD_CONNECTION_CHECK)
	if (is_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return correct_mtp;
	}
#endif

	mtp = (char *)&(mpd.smart_s6e8aa0x01.MTP);
	mfd = (struct msm_fb_data_type *)registered_fb[0]->par;
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	msd.ctrl_pdata = ctrl;

	mdss_dsi_panel_cmds_send(ctrl, &ctrl->off_cmds);

	pr_debug("first time mpt read\n");
	read_reg(MTP_REGISTER, mtp_size, first_mtp, false, mfd);

	pr_debug("second time mpt read\n");
	read_reg(MTP_REGISTER, mtp_size, second_mtp, false, mfd);

	if (memcmp(first_mtp, second_mtp, mtp_size) != 0) {
		pr_debug("third time mpt read\n");
		read_reg(MTP_REGISTER, mtp_size, third_mtp, false, mfd);

		if (memcmp(first_mtp, third_mtp, mtp_size) == 0) {
			mdss_dsi_panel_cmds_send(ctrl, &ctrl->on_cmds);
			pr_debug("MTP data is used from first read mtp");
			memcpy(mtp, first_mtp, mtp_size);
			correct_mtp = 1;
		} else if (memcmp(second_mtp, third_mtp, mtp_size) == 0) {
			mdss_dsi_panel_cmds_send(ctrl, &ctrl->on_cmds);
			pr_debug("MTP data is used from second read mtp");
			memcpy(mtp, second_mtp, mtp_size);
			correct_mtp = 2;
		} else {
			mdss_dsi_panel_cmds_send(ctrl, &ctrl->on_cmds);
			pr_debug("MTP data is used 0 read mtp");
			memset(mtp, 0, mtp_size);
			correct_mtp = 0;
		}
	} else {
		mdss_dsi_panel_cmds_send(ctrl, &ctrl->on_cmds);
		pr_debug("MTP data is used from first read mtp\n");
		memcpy(mtp, first_mtp, mtp_size);
		correct_mtp = 1;
	}

	for (i = 0; i < MTP_DATA_SIZE; i++)
				snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %02x",
					mtp[i]);
				pr_debug("MTP: %s", pBuffer);

	return correct_mtp;
}



unsigned char mdss_dsi_panel_pwm_scaling(int level)
{
	unsigned char scaled_level;

	if (level >= MID_BRIGHTNESS_LEVEL) {
		scaled_level  = (level - MID_BRIGHTNESS_LEVEL) *
		(BL_MAX_BRIGHTNESS_LEVEL - BL_MID_BRIGHTNESS_LEVEL) / (MAX_BRIGHTNESS_LEVEL-MID_BRIGHTNESS_LEVEL) + BL_MID_BRIGHTNESS_LEVEL;
	} else if (level >= DIM_BRIGHTNESS_LEVEL) {
		scaled_level  = (level - DIM_BRIGHTNESS_LEVEL) *
		(BL_MID_BRIGHTNESS_LEVEL - BL_DIM_BRIGHTNESS_LEVEL) / (MID_BRIGHTNESS_LEVEL-DIM_BRIGHTNESS_LEVEL) + BL_DIM_BRIGHTNESS_LEVEL;
	} else if (level >= LOW_BRIGHTNESS_LEVEL) {
		scaled_level  = (level - LOW_BRIGHTNESS_LEVEL) *
		(BL_DIM_BRIGHTNESS_LEVEL - BL_LOW_BRIGHTNESS_LEVEL) / (DIM_BRIGHTNESS_LEVEL-LOW_BRIGHTNESS_LEVEL) + BL_LOW_BRIGHTNESS_LEVEL;
	}  else{
		if(poweroff_charging == 1)
			scaled_level  = level*BL_LOW_BRIGHTNESS_LEVEL/LOW_BRIGHTNESS_LEVEL;
		else
			scaled_level  = BL_MIN_BRIGHTNESS;
	}

	pr_info("level = [%d]: scaled_level = [%d]   autobrightness level:%d\n",level,scaled_level,msd.dstat.auto_brightness);

	return scaled_level;
}

#if defined (CONFIG_LCD_CLASS_DEVICE)
static char lcd_cabc[2] = {0x55, 0x0};	/* CABC COMMAND : default disabled */
static struct dsi_cmd_desc cabc_cmd= {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(lcd_cabc)},
	lcd_cabc
};

static void mdss_dsi_panel_cabc_dcs(struct mdss_dsi_ctrl_pdata *ctrl, int siop_status)
{

	struct dcs_cmd_req cmdreq;

	pr_debug("%s: cabc=%d\n", __func__, siop_status);
#if defined(CONFIG_LCD_CONNECTION_CHECK)
	if (is_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}
#endif
	lcd_cabc[1] = (unsigned char)siop_status;
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = &cabc_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
}
#endif

void mdss_dsi_s6e8aa0a_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	int rc=0;
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_err("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
	}

	printk("%s: enable = %d\n", __func__, enable);
	if (enable) {

		rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->disp_en_gpio, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_8MA),
					GPIO_CFG_ENABLE);
		if (rc)
		pr_err("enabling disp_en_gpio_2_2v[%d] failed, rc=%d\n", ctrl_pdata->disp_en_gpio, rc);

		rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->rst_gpio, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_PULL_UP,GPIO_CFG_8MA),
					GPIO_CFG_ENABLE);
		if (rc)
		pr_err("disabling rst_gpio failed[%d], rc=%d\n",ctrl_pdata->rst_gpio, rc);
#if 0
		rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->bl_on_gpio, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_8MA),
					GPIO_CFG_ENABLE);
		if (rc)
		pr_err("disabling bl_on_gpio failed, rc=%d\n",rc);
#endif
		mdelay(20);
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
//		gpio_set_value(ctrl_pdata->disp_en_gpio_2_2v, 1);
		mdelay(5);
//		gpio_set_value(ctrl_pdata->bl_on_gpio, 1);
		gpio_set_value((ctrl_pdata->rst_gpio), 1);
		msleep(20);
		gpio_set_value((ctrl_pdata->rst_gpio), 0);
		msleep(5);
		gpio_set_value((ctrl_pdata->rst_gpio), 1);
		msleep(20);
		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
						__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {

		if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);

		rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->disp_en_gpio, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
		if (rc)
		pr_err("disabling disp_en_gpio_2_2v failed, rc=%d\n",rc);

		rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->rst_gpio, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
		if (rc)
		pr_err("disabling rst_gpio failed, rc=%d\n",rc);
#if 0
		rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->bl_on_gpio, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
		if (rc)
		pr_err("disabling bl_on_gpio failed, rc=%d\n",rc);
		gpio_set_value(ctrl_pdata->bl_on_gpio, 0);
#endif

	//	gpio_set_value(ctrl_pdata->disp_en_gpio, 0);
		gpio_set_value((ctrl_pdata->rst_gpio), 0);
	}
}


static void mdss_dsi_panel_acl_ctrl(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct dsi_cmd_desc *cmd_desc;
	int cmd_size = 0;
	int flag = 0;

#ifdef CMD_DEBUG
	int i,j;
#endif

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

#if !defined(CONFIG_MACH_S3VE3G_EUR)
	 flag = CMD_REQ_SINGLE_TX;
#endif
	switch (enable) {

		case PANEL_ACL_ON:

			cmd_desc = mpd.acl_on.cmd;
			cmd_size = mpd.acl_on.size;

			break;

		case PANEL_ACL_OFF:

			cmd_desc = mpd.acl_off.cmd;
			cmd_size = mpd.acl_off.size;

			break;

		case PANEL_ACL_UPDATE:

			cmd_desc = mpd.acl_update.cmd;
			cmd_size = mpd.acl_update.size;

			break;

		default:
		pr_err("%s: Unknown acl_ctrl configuration\n",__func__);

		break;
	}

#ifdef CMD_DEBUG
				for (i = 0; i < cmd_size; i++)
				{
					for (j = 0; j < cmd_desc[i].dchdr.dlen; j++)
						pr_debug("%x ",cmd_desc[i].payload[j]);
					pr_debug("\n");
				}
#endif

				mdss_dsi_cmds_send(ctrl_pdata, cmd_desc, cmd_size, flag);


	return;
}


#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
static irqreturn_t err_fg_irq_handler(int irq, void *handle)
{
	if(esd_irq_on) {
		esd_irq_on = false;
		disable_irq_nosync(err_fg_gpio);
		cancel_work_sync(&err_fg_work);
		schedule_work(&err_fg_work);
		printk("%s : handler start\n", __func__);
	}

	printk("%s : handler end\n", __func__);

	return IRQ_HANDLED;
}
static void err_fg_work_func(struct work_struct *work)
{
	int bl_backup;
	struct mdss_panel_data *pdata = msd.mpd;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct msm_fb_data_type *mfd= msd.mfd;
	struct mdss_mdp_ctl *msdCtl = mfd_to_ctl(mfd);

	if(msd.mfd == NULL){
		pr_err("%s: mfd not initialized Skip ESD recovery\n", __func__);
		return;
	}
	if(pdata == NULL) {
		pr_err("%s: pdata not available... skipping update\n", __func__);
		return;
	}
	bl_backup=msd.mfd->bl_level;
	if( panel_state == MIPI_SUSPEND_STATE ){
		pr_err("%s: Display off Skip ESD recovery\n", __func__);
		return;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
						panel_data);

	printk("%s : start\n", __func__);
	err_fg_working = 1;

	mdss_mdp_rotator_unset(MDSS_MDP_ROT_SESSION_MASK);

	msd.mfd->fbi->esd_active = true;
	msd.mfd->fbi->fbops->fb_blank(FB_BLANK_POWERDOWN, msd.mfd->fbi);
	msd.mfd->fbi->fbops->fb_blank(FB_BLANK_UNBLANK, msd.mfd->fbi);

	if (msdCtl->vsync_handler.enabled == 0) {
		mdss_mdp_overlay_vsync_ctrl(msd.mfd, 1);
//	        mdss_mdp_irq_clear(msdCtl->mdata, MDSS_MDP_IRQ_INTF_VSYNC, msdCtl->intf_num);
//		mdss_mdp_irq_enable(MDSS_MDP_IRQ_INTF_VSYNC, msdCtl->intf_num);
	}

	esd_count++;
	err_fg_working = 0;
	msd.mfd->bl_level=bl_backup;
	mdss_dsi_panel_bl_ctrl(pdata,msd.mfd->bl_level);
	if(!esd_irq_on) {
		esd_irq_on = true;
		enable_irq(err_fg_gpio);
	}
	msd.mfd->fbi->esd_active = false;
	printk("%s : end\n", __func__);

	return;
}
#ifdef ESD_DEBUG
static ssize_t mipi_samsung_esd_check_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, 20, "esd count:%d \n",esd_count);

	return rc;
}
static ssize_t mipi_samsung_esd_check_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct msm_fb_data_type *mfd;
	mfd = platform_get_drvdata(msd.msm_pdev);

	err_fg_irq_handler(0, mfd);
	return 1;
}

static DEVICE_ATTR(esd_check, S_IRUGO , mipi_samsung_esd_check_show,\
			 mipi_samsung_esd_check_store);
#endif
#endif



static void mdss_dsi_panel_bl_ctrl(struct mdss_panel_data *pdata,
							u32 bl_level)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct dsi_cmd_desc *cmd_desc;
	int cmd_size = 0;
	int flag = 0;
	int cmds_sent;
	struct SMART_DIM *psmart;
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	int rc=0;
#endif
#ifdef CMD_DEBUG
	int i,j;
#endif

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
			panel_data);

	if(first_boot == 0)
	{

		msd.mfd = (struct msm_fb_data_type *)registered_fb[0]->par;
		panel_state = MIPI_RESUME_STATE;
		msd.mpd = pdata;

		msd.ctrl_pdata = ctrl_pdata;
		first_boot =1;

		if (!msd.dstat.is_elvss_loaded) {
			mpd.lcd_elvss_data[0] = lcd_id3;
			msd.dstat.is_elvss_loaded = true;
		}

		if (!msd.dstat.is_smart_dim_loaded) {

				psmart = &(mpd.smart_s6e8aa0x01);
				psmart->plux_table = mpd.lux_table;
				psmart->lux_table_max = mpd.lux_table_max_cnt;
				psmart->ldi_revision = 0x60;

				smart_dimming_init(psmart);

				msd.dstat.is_smart_dim_loaded = true;

			}

			get_min_lux_table(&(mpd.gamma_initial[2]),
						GAMMA_SET_MAX);
			reset_gamma_level();
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
#if defined (CONFIG_LCD_CONNECTION_CHECK)
			if(lcd_connected_status == 1){
#endif
					INIT_WORK(&err_fg_work, err_fg_work_func);

					rc = request_threaded_irq(err_fg_gpio, NULL, err_fg_irq_handler,
						IRQF_TRIGGER_LOW | IRQF_ONESHOT, "esd_detect", NULL);
					if (rc) {
						pr_err("%s : Failed to request_irq. :ret=%d", __func__, rc);
					}
					if(!esd_irq_on) {
						esd_irq_on = true;
						enable_irq(err_fg_gpio);
					}
#if defined (CONFIG_LCD_CONNECTION_CHECK)
	    }
#endif
#endif
	}

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	if (err_fg_working) {
		pr_info("[LCD] %s : esd is working!! return.. \n", __func__);
		return;
	}
#endif

	if(bl_level)
	msd.mfd->bl_previous = bl_level;

	switch (ctrl_pdata->bklt_ctrl) {

	case BL_DCS_CMD:

			if (mpd.prepare_brightness_control_cmd_array) {

				flag = 0;

				cmds_sent = prepare_brightness_control_cmd_array(0x60,bl_level);

				if (cmds_sent < 0){
					pr_info("cmds_sent: %x\n", cmds_sent);
					goto end;
				}

				cmd_desc = mpd.combined_ctrl.cmd;
				cmd_size = mpd.combined_ctrl.size;

#ifdef CMD_DEBUG
				for (i = 0; i < cmd_size; i++)
				{
					for (j = 0; j < cmd_desc[i].dchdr.dlen; j++)
						printk("%x ",cmd_desc[i].payload[j]);
					printk("\n");
				}
#endif

				mdss_dsi_cmds_send(ctrl_pdata, cmd_desc, cmd_size, flag);

				}

		break;

	default:
		pr_err("%s: Unknown bl_ctrl configuration\n",__func__);

		break;
	}

end:
	return;
}
int bl_first_update=0;
static int mdss_dsi_panel_on(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;

	msd.mfd = (struct msm_fb_data_type *)registered_fb[0]->par;
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	msd.ctrl_pdata = ctrl;
	msd.mpd = pdata;

	printk("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);

			get_min_lux_table(&(mpd.gamma_initial[2]),
						GAMMA_SET_MAX);
			reset_gamma_level();

	if (ctrl->on_cmds.cmd_cnt)
		mdss_dsi_panel_cmds_send(ctrl, &ctrl->on_cmds);

#if defined(CONFIG_MACH_S3VE3G_EUR)
	if(bl_first_update== 0)
		pr_err("to maintain ddefault brightness \n");
	else
		mdss_dsi_panel_bl_ctrl(pdata,msd.mfd->bl_previous);
#endif

	panel_state = MIPI_RESUME_STATE;
#if defined(CONFIG_LCD_CLASS_DEVICE)
	mdss_dsi_panel_cabc_dcs(ctrl, msd.dstat.siop_status);
#endif
#if defined(CONFIG_MDNIE_LITE_TUNING)
	is_negative_on();
#endif
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
#if defined (CONFIG_LCD_CONNECTION_CHECK)
	if(lcd_connected_status == 1){
#endif
		if(!esd_irq_on && !err_fg_working) {
			esd_irq_on = true;
			enable_irq(err_fg_gpio);
		}
#if defined (CONFIG_LCD_CONNECTION_CHECK)
	}
#endif
#endif
	printk("%s:-\n", __func__);
	return 0;
}

static int mdss_dsi_panel_off(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	msd.mfd = (struct msm_fb_data_type *)registered_fb[0]->par;

	if(panel_state == MIPI_SUSPEND_STATE)
		return 0;
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
#if defined (CONFIG_LCD_CONNECTION_CHECK)
	if(lcd_connected_status == 1){
#endif
		if(esd_irq_on) {
			esd_irq_on = false;
			disable_irq_nosync(err_fg_gpio);
			cancel_work_sync(&err_fg_work);
			printk("%s : esd_irq_off\n", __func__);
		}
#if defined (CONFIG_LCD_CONNECTION_CHECK)
	}
#endif
#endif

    printk("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);

	if (ctrl->off_cmds.cmd_cnt)
		mdss_dsi_panel_cmds_send(ctrl, &ctrl->off_cmds);

	panel_state = MIPI_SUSPEND_STATE;
	printk("%s:-\n", __func__);
	return 0;
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
			return -ENOMEM;
		}
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
	if (!pcmds->cmds){
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


	data = of_get_property(np, link_key, NULL);
	if (!strncmp(data, "dsi_hs_mode", 11))
		pcmds->link_state = DSI_HS_MODE;
	else
		pcmds->link_state = DSI_LP_MODE;
	pr_debug("%s: dcs_cmd=%x len=%d, cmd_cnt=%d link_state=%d\n", __func__,
		pcmds->buf[0], pcmds->blen, pcmds->cmd_cnt, pcmds->link_state);

	return 0;
}
static int mdss_panel_dt_get_dst_fmt(u32 bpp, char mipi_mode, u32 pixel_packing,
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
static int mdss_panel_parse_dt(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	u32	tmp;
	int rc, i, len, res[2];
	const char *data;
	static const char *pdest;
	struct mdss_panel_info *pinfo = &(ctrl_pdata->panel_data.panel_info);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-panel-width", &tmp);
	if (rc) {
		pr_err("%s:%d, panel width not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}

	pinfo->xres = (!rc ? tmp : 640);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-panel-height", &tmp);
	if (rc) {
		pr_err("%s:%d, panel height not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	pinfo->yres = (!rc ? tmp : 480);
	rc = of_property_read_u32_array(np, "qcom,mdss-pan-size", res, 2);
	if (rc == 0) {
		pinfo->physical_width= res[0];
		pinfo->physical_height= res[1];
	}

	rc = of_property_read_u32(np, "qcom,mdss-dsi-h-left-border", &tmp);
	pinfo->lcdc.xres_pad = (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-h-right-border", &tmp);
	if (!rc)
		pinfo->lcdc.xres_pad += tmp;
	rc = of_property_read_u32(np, "qcom,mdss-dsi-v-top-border", &tmp);
	pinfo->lcdc.yres_pad = (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-v-bottom-border", &tmp);
	if (!rc)
		pinfo->lcdc.yres_pad += tmp;
	rc = of_property_read_u32(np, "qcom,mdss-dsi-bpp", &tmp);
	if (rc) {
		pr_err("%s:%d, bpp not specified\n", __func__, __LINE__);
		return -EINVAL;
	}
	pinfo->bpp = (!rc ? tmp : 24);
	pinfo->mipi.mode = DSI_VIDEO_MODE;
	data = of_get_property(np, "qcom,mdss-dsi-panel-type", NULL);
	if (data && !strncmp(data, "dsi_cmd_mode", 12))
		pinfo->mipi.mode = DSI_CMD_MODE;
	rc = of_property_read_u32(np, "qcom,mdss-dsi-pixel-packing", &tmp);
	tmp = (!rc ? tmp : 0);
	rc = mdss_panel_dt_get_dst_fmt(pinfo->bpp,
		pinfo->mipi.mode, tmp,
		&(pinfo->mipi.dst_format));
	if (rc) {
		pr_debug("%s: problem determining dst format. Set Default\n",
			__func__);
		pinfo->mipi.dst_format =
			DSI_VIDEO_DST_FORMAT_RGB888;
	}

	pdest = of_get_property(np,
			"qcom,mdss-dsi-panel-destination", NULL);
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

	rc = of_property_read_u32(np, "qcom,mdss-dsi-h-front-porch", &tmp);
	pinfo->lcdc.h_front_porch = (!rc ? tmp : 6);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-h-back-porch", &tmp);
	pinfo->lcdc.h_back_porch = (!rc ? tmp : 6);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-h-pulse-width", &tmp);
	pinfo->lcdc.h_pulse_width = (!rc ? tmp : 2);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-h-sync-skew", &tmp);
	pinfo->lcdc.hsync_skew = (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-v-back-porch", &tmp);
	pinfo->lcdc.v_back_porch = (!rc ? tmp : 6);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-v-front-porch", &tmp);
	pinfo->lcdc.v_front_porch = (!rc ? tmp : 6);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-v-pulse-width", &tmp);
	pinfo->lcdc.v_pulse_width = (!rc ? tmp : 2);
	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-underflow-color", &tmp);
	pinfo->lcdc.underflow_clr = (!rc ? tmp : 0xff);

	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-border-color", &tmp);
	pinfo->lcdc.border_clr = (!rc ? tmp : 0);
	pinfo->bklt_ctrl = UNKNOWN_CTRL;
	data = of_get_property(np, "qcom,mdss-dsi-bl-pmic-control-type", NULL);
	if (data) {
		if (!strncmp(data, "bl_ctrl_wled", 12)) {
			led_trigger_register_simple("bkl-trigger",
				&bl_led_trigger);
			pr_debug("%s: SUCCESS-> WLED TRIGGER register\n",
				__func__);
			ctrl_pdata->bklt_ctrl = BL_WLED;
		} else if (!strncmp(data, "bl_ctrl_pwm", 11)) {
			ctrl_pdata->bklt_ctrl = BL_PWM;
			rc = of_property_read_u32(np,
				"qcom,mdss-dsi-bl-pmic-pwm-frequency", &tmp);
			if (rc) {
				pr_err("%s:%d, Error, panel pwm_period\n",
					__func__, __LINE__);
			return -EINVAL;
			}
			ctrl_pdata->pwm_period = tmp;
			rc = of_property_read_u32(np,
					"qcom,mdss-dsi-bl-pmic-bank-select", &tmp);
			if (rc) {
				pr_err("%s:%d, Error, dsi lpg channel\n",
 						__func__, __LINE__);
				return -EINVAL;
			}
			ctrl_pdata->pwm_lpg_chan = tmp;
			tmp = of_get_named_gpio(np,
				"qcom,mdss-dsi-pwm-gpio", 0);
			ctrl_pdata->pwm_pmic_gpio = tmp;
		} else if (!strncmp(data, "bl_ctrl_dcs", 11)) {
			ctrl_pdata->bklt_ctrl = BL_DCS_CMD;
 		}
	}
	rc = of_property_read_u32(np, "qcom,mdss-brightness-max-level", &tmp);
	pinfo->brightness_max = (!rc ? tmp : MDSS_MAX_BL_BRIGHTNESS);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-bl-min-level", &tmp);
	pinfo->bl_min = (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-bl-max-level", &tmp);
	pinfo->bl_max = (!rc ? tmp : 255);
	ctrl_pdata->bklt_max = pinfo->bl_max;

	rc = of_property_read_u32(np, "qcom,mdss-dsi-interleave-mode", &tmp);
	pinfo->mipi.interleave_mode = (!rc ? tmp : 0);

	pinfo->mipi.vsync_enable = of_property_read_bool(np,
		"qcom,mdss-dsi-te-check-enable");
	pinfo->mipi.hw_vsync_mode = of_property_read_bool(np,
		"qcom,mdss-dsi-te-using-te-pin");
	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-h-sync-pulse", &tmp);
	pinfo->mipi.pulse_mode_hsa_he = (!rc ? tmp : false);

	pinfo->mipi.hfp_power_stop = of_property_read_bool(np,
		"qcom,mdss-dsi-hfp-power-mode");
	pinfo->mipi.hsa_power_stop = of_property_read_bool(np,
		"qcom,mdss-dsi-hsa-power-mode");
	pinfo->mipi.hbp_power_stop = of_property_read_bool(np,
		"qcom,mdss-dsi-hbp-power-mode");
	pinfo->mipi.bllp_power_stop = of_property_read_bool(np,
		"qcom,mdss-dsi-bllp-power-mode");
	pinfo->mipi.eof_bllp_power_stop = of_property_read_bool(
		np, "qcom,mdss-dsi-bllp-eof-power-mode");
	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-traffic-mode", &tmp);
	pinfo->mipi.traffic_mode =
			(!rc ? tmp : DSI_NON_BURST_SYNCH_PULSE);

	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-te-dcs-command", &tmp);
	pinfo->mipi.insert_dcs_cmd =
			(!rc ? tmp : 1);

	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-te-v-sync-continue-lines", &tmp);
	pinfo->mipi.wr_mem_continue =
			(!rc ? tmp : 0x3c);

	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-te-v-sync-rd-ptr-irq-line", &tmp);
	pinfo->mipi.wr_mem_start =
			(!rc ? tmp : 0x2c);

	rc = of_property_read_u32(np,
		"qcom,mdss-dsi-te-pin-select", &tmp);
	pinfo->mipi.te_sel =
			(!rc ? tmp : 1);

	rc = of_property_read_u32(np, "qcom,mdss-dsi-virtual-channel-id", &tmp);
	pinfo->mipi.vc = (!rc ? tmp : 0);

	rc = of_property_read_u32(np, "qcom,mdss-dsi-color-order", &tmp);
	pinfo->mipi.rgb_swap = (!rc ? tmp : DSI_RGB_SWAP_RGB);

	rc = of_property_read_u32(np, "qcom,mdss-force-clk-lane-hs", &tmp);
	pinfo->mipi.force_clk_lane_hs = (!rc ? tmp : 0);

	pinfo->mipi.data_lane0 = of_property_read_bool(np,
		"qcom,mdss-dsi-lane-0-state");
	pinfo->mipi.data_lane1 = of_property_read_bool(np,
		"qcom,mdss-dsi-lane-1-state");
	pinfo->mipi.data_lane2 = of_property_read_bool(np,
		"qcom,mdss-dsi-lane-2-state");
	pinfo->mipi.data_lane3 = of_property_read_bool(np,
		"qcom,mdss-dsi-lane-3-state");

	rc = of_property_read_u32(np, "qcom,mdss-dsi-lane-map", &tmp);
	pinfo->mipi.dlane_swap = (!rc ? tmp : 0);

	rc = of_property_read_u32(np, "qcom,mdss-dsi-t-clk-pre", &tmp);
	pinfo->mipi.t_clk_pre = (!rc ? tmp : 0x24);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-t-clk-post", &tmp);
	pinfo->mipi.t_clk_post = (!rc ? tmp : 0x03);
	rc = of_property_read_u32(np, "qcom,mdss-dsi-stream", &tmp);
	pinfo->mipi.stream = (!rc ? tmp : 0);

	rc = of_property_read_u32(np, "qcom,mdss-dsi-mdp-trigger", &tmp);
	pinfo->mipi.mdp_trigger =
			(!rc ? tmp : DSI_CMD_TRIGGER_SW);
	if (pinfo->mipi.mdp_trigger > 6) {
		pr_err("%s:%d, Invalid mdp trigger. Forcing to sw trigger",
						 __func__, __LINE__);
		pinfo->mipi.mdp_trigger =
					DSI_CMD_TRIGGER_SW;
	}

	rc = of_property_read_u32(np, "qcom,mdss-dsi-dma-trigger", &tmp);
	pinfo->mipi.dma_trigger =
			(!rc ? tmp : DSI_CMD_TRIGGER_SW);
	if (pinfo->mipi.dma_trigger > 6) {
		pr_err("%s:%d, Invalid dma trigger. Forcing to sw trigger",
						 __func__, __LINE__);
		pinfo->mipi.dma_trigger =
					DSI_CMD_TRIGGER_SW;
	}
	data = of_get_property(np, "qcom,mdss-dsi-panel-mode-gpio-state", &tmp);
	if (data) {
		if (!strcmp(data, "high"))
			pinfo->mode_gpio_state = MODE_GPIO_HIGH;
		else if (!strcmp(data, "low"))
			pinfo->mode_gpio_state = MODE_GPIO_LOW;
	} else {
		pinfo->mode_gpio_state = MODE_GPIO_NOT_VALID;
	}

	rc = of_property_read_u32(np, "qcom,mdss-dsi-panel-frame-rate", &tmp);
	pinfo->mipi.frame_rate = (!rc ? tmp : 60);

	rc = of_property_read_u32(np, "qcom,mdss-dsi-panel-clock-rate", &tmp);
	pinfo->clk_rate = (!rc ? tmp : 0);

	data = of_get_property(np, "qcom,mdss-dsi-panel-timings", &len);
	if ((!data) || (len != 12)) {
		pr_err("%s:%d, Unable to read Phy timing settings",
		       __func__, __LINE__);
		goto error;
	}
	for (i = 0; i < len; i++)
		pinfo->mipi.dsi_phy_db.timing[i] = data[i];

		pinfo->mipi.dsi_phy_db.timing[8] = 0x20;

	mdss_dsi_parse_fbc_params(np, pinfo);
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->on_cmds,
		"qcom,mdss-dsi-on-command", "qcom,mdss-dsi-on-command-state");
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->off_cmds,
		"qcom,mdss-dsi-off-command", "qcom,mdss-dsi-off-command-state");

	return 0;
error:
	return -EINVAL;
}

#if defined(CONFIG_LCD_CLASS_DEVICE)

static ssize_t mipi_samsung_disp_acl_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, sizeof(buf), "%d\n", msd.dstat.acl_on);
	printk("acl status: %d\n", *buf);

	return rc;
}



static ssize_t mipi_samsung_disp_acl_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct msm_fb_data_type *mfd;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_data *pdata = msd.mpd;

	mfd = msd.mfd;
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
							panel_data);

	if (!msd.mfd->panel_power_on) {
		printk("%s: panel is off state. updating state value.\n",__func__);
		if (sysfs_streq(buf, "1") && !msd.dstat.acl_on)
		msd.dstat.acl_on = true;
		else if (sysfs_streq(buf, "0") && msd.dstat.acl_on)
			msd.dstat.acl_on = false;
		else
			pr_info("%s: Invalid argument!!", __func__);
	} else {
		if (sysfs_streq(buf, "1") && !msd.dstat.acl_on) {
			if (set_acl_on_level(msd.mfd->bl_level)){

			mdss_dsi_panel_acl_ctrl(pdata,PANEL_ACL_OFF);

		}else{
			mdss_dsi_panel_acl_ctrl(pdata,PANEL_ACL_ON);
			mdss_dsi_panel_acl_ctrl(pdata,PANEL_ACL_UPDATE);

		}
			msd.dstat.acl_on = true;
		} else if (sysfs_streq(buf, "0") && msd.dstat.acl_on) {
			mdss_dsi_panel_acl_ctrl(pdata,PANEL_ACL_OFF);
		msd.dstat.acl_on = false;
		}else{
			pr_info("%s: Invalid argument!!", __func__);
		}
	}

	return size;
}

static DEVICE_ATTR(power_reduce, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_disp_acl_show,
			mipi_samsung_disp_acl_store);


static ssize_t mdss_s6e8aa0a_disp_get_power(struct device *dev,
			struct device_attribute *attr, char *buf)
{

	pr_info("mipi_samsung_disp_get_power(0)\n");

	return 0;
}

static ssize_t mdss_s6e8aa0a_disp_set_power(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int power;
	if (sscanf(buf, "%u", &power) != 1)
		return -EINVAL;

	pr_info("mipi_samsung_disp_set_power:%d\n",power);

	return size;
}

static DEVICE_ATTR(lcd_power, S_IRUGO | S_IWUSR | S_IWGRP,
			mdss_s6e8aa0a_disp_get_power,
			mdss_s6e8aa0a_disp_set_power);

static ssize_t mdss_siop_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf(buf, 2, "%d\n",msd.dstat.siop_status);
	pr_info("%s :[MDSS_S6E8AA0A] CABC: %d\n", __func__, msd.dstat.siop_status);
	return rc;
}
static ssize_t mdss_siop_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{

	if (sysfs_streq(buf, "1") && !msd.dstat.siop_status)
		msd.dstat.siop_status = true;
	else if (sysfs_streq(buf, "0") && msd.dstat.siop_status)
		msd.dstat.siop_status = false;
	else
		pr_info("%s: Invalid argument!!", __func__);

	return size;

}

static DEVICE_ATTR(siop_enable, S_IRUGO | S_IWUSR | S_IWGRP,
			mdss_siop_enable_show,
			mdss_siop_enable_store);


static struct lcd_ops mdss_s6e8aa0a_disp_props = {

	.get_power = NULL,
	.set_power = NULL,

};

static ssize_t mdss_s6e8aa0a_auto_brightness_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf(buf, sizeof(buf), "%d\n",
					msd.dstat.auto_brightness);
	pr_info("%s : auto_brightness : %d\n", __func__, msd.dstat.auto_brightness);

	return rc;
}

static ssize_t mdss_s6e8aa0a_auto_brightness_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	static unsigned char prev_auto_brightness;
	struct mdss_panel_data *pdata = msd.mpd;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

#if defined(CONFIG_LCD_CONNECTION_CHECK)
	if (is_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return size;
	}
#endif

	if (sysfs_streq(buf, "0"))
		msd.dstat.auto_brightness = 0;
	else if (sysfs_streq(buf, "1"))
		msd.dstat.auto_brightness = 1;
	else if (sysfs_streq(buf, "2"))
		msd.dstat.auto_brightness = 2;
	else if (sysfs_streq(buf, "3"))
		msd.dstat.auto_brightness = 3;
	else if (sysfs_streq(buf, "4"))
		msd.dstat.auto_brightness = 4;
	else if (sysfs_streq(buf, "5"))
		msd.dstat.auto_brightness = 5;
	else if (sysfs_streq(buf, "6"))
		msd.dstat.auto_brightness = 6;
	else
		pr_info("%s: Invalid argument!!", __func__);

	if(prev_auto_brightness == msd.dstat.auto_brightness)
		return size;

	mdelay(1);

	mutex_lock(&bg_lock);

	if(msd.dstat.auto_brightness)
		msd.dstat.siop_status = true;
	else
		msd.dstat.siop_status = false;

        if(msd.mfd == NULL){
		pr_err("%s: mfd not initialized\n", __func__);
		mutex_unlock(&bg_lock);
		return size;
	}

	if( msd.mfd->panel_power_on == false){
	pr_err("%s: panel power off no bl ctrl\n", __func__);
		mutex_unlock(&bg_lock);
	return size;
	}

	if(pdata == NULL){
		pr_err("%s: pdata not available... skipping update\n", __func__);
		mutex_unlock(&bg_lock);
		return size;
	}


        ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);
	mdss_dsi_panel_cabc_dcs(ctrl_pdata, msd.dstat.siop_status);
	mutex_unlock(&bg_lock);
	prev_auto_brightness = msd.dstat.auto_brightness;
	pr_info("%s %d %d\n", __func__, msd.dstat.auto_brightness, msd.dstat.siop_status);
	return size;
}

static DEVICE_ATTR(auto_brightness, S_IRUGO | S_IWUSR | S_IWGRP,
			mdss_s6e8aa0a_auto_brightness_show,
			mdss_s6e8aa0a_auto_brightness_store);

static ssize_t mdss_disp_lcdtype_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char temp[20];

	snprintf(temp, 20, "SDC_AMS480GY01");
	strncat(buf, temp, 20);
	return strnlen(buf, 20);
}
static DEVICE_ATTR(lcd_type, S_IRUGO, mdss_disp_lcdtype_show, NULL);
unsigned int mdss_dsi_show_cabc(void )
{
	return msd.dstat.siop_status;
}

void mdss_dsi_store_cabc(unsigned int cabc)
{
	struct mdss_panel_data *pdata = msd.mpd;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if(msd.mfd == NULL){
		pr_err("%s: mfd not initialized\n", __func__);
		return;
	}

	if( msd.mfd->panel_power_on == false){
		pr_err("%s: panel power off no bl ctrl\n", __func__);
		return;
	}

	if(pdata == NULL){
		pr_err("%s: pdata not available... skipping update\n", __func__);
		return;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
						panel_data);
	if(msd.dstat.auto_brightness)
		return;
	mutex_lock(&bg_lock);
	msd.dstat.siop_status=cabc;
	mdss_dsi_panel_cabc_dcs(ctrl_pdata,msd.dstat.siop_status);
	mutex_unlock(&bg_lock);
	pr_info("%s :[MDSS_s6e8aa0a] CABC: %d\n", __func__,msd.dstat.siop_status);

}

#endif

#if defined(CONFIG_LCD_CONNECTION_CHECK)
int is_lcd_attached(void)
{
	return lcd_connected_status;
}
EXPORT_SYMBOL(is_lcd_attached);

static int __init lcd_attached_status(char *state)
{
	/*
	*	1 is lcd attached
	*	0 is lcd detached
	*/

	if (strncmp(state, "1", 1) == 0)
		lcd_connected_status = 1;
	else
		lcd_connected_status = 0;

	pr_info("%s %s", __func__, lcd_connected_status == 1 ?
				"lcd_attached" : "lcd_detached");
	return 1;
}
__setup("lcd_attached=", lcd_attached_status);

static int __init detect_lcd_panel_vendor(char* read_id)
{
	lcd_id = simple_strtol(read_id, NULL, 16);

	lcd_id3 = lcd_id & 0xff;

	if(lcd_id)
		lcd_connected_status = 1;
	else
		lcd_connected_status = 0;

	pr_info("detect_lcd_panel_vendor lcd_id = %x & lcd_id3 = %x\n",lcd_id,lcd_id3);
	pr_info("%s %s", __func__, lcd_connected_status == 1 ?
				"lcd_attached" : "lcd_detached");

	return 1;
}
__setup("lcd_id=0x", detect_lcd_panel_vendor);

#endif

int mdss_dsi_panel_init(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata,
	bool cmd_cfg_cont_splash)
{
	int rc = 0;
	static const char *panel_name;
	bool cont_splash_enabled;
/* Needed to pass GPIO DVS for OLED_DET GPIO*/
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	int disp_esd_gpio;
#endif

#if defined(CONFIG_LCD_CLASS_DEVICE)
	struct lcd_device *lcd_device;

#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
	struct backlight_device *bd = NULL;
#endif
#endif
#if defined(CONFIG_LCD_CLASS_DEVICE)
	struct device_node *np = NULL;
	struct platform_device *pdev = NULL;
	np = of_parse_phandle(node,
			"qcom,mdss-dsi-panel-controller", 0);
	if (!np) {
		pr_err("%s: Dsi controller node not initialized\n", __func__);
		return -EPROBE_DEFER;
	}

	pdev = of_find_device_by_node(np);
#endif
	mutex_init(&bg_lock);

#if defined(CONFIG_LCD_CONNECTION_CHECK)
	printk("%s: LCD attached status: %d !\n",
				__func__, is_lcd_attached());
#endif
#ifdef DDI_VIDEO_ENHANCE_TUNING
	mutex_init(&msd.lock);
#endif
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
		pr_info("%s: Panel Name = %s\n", __func__, panel_name);


	rc = mdss_panel_parse_dt(node, ctrl_pdata);
	if (rc) {
		pr_err("%s:%d panel dt parse failed\n", __func__, __LINE__);
		return rc;
	}

	if (cmd_cfg_cont_splash)
		cont_splash_enabled = of_property_read_bool(node,
				"qcom,cont-splash-enabled");
	else
		cont_splash_enabled = false;
	if (!cont_splash_enabled) {
		pr_info("%s:%d Continuous splash flag not found.\n",
				__func__, __LINE__);
		ctrl_pdata->panel_data.panel_info.cont_splash_enabled = 0;
	} else {
		pr_info("%s:%d Continuous splash flag enabled.\n",
				__func__, __LINE__);
		ctrl_pdata->panel_data.panel_info.cont_splash_enabled = 1;
	}

	ctrl_pdata->on = mdss_dsi_panel_on;
	ctrl_pdata->off = mdss_dsi_panel_off;
	ctrl_pdata->panel_reset = mdss_dsi_s6e8aa0a_panel_reset;
	ctrl_pdata->panel_data.set_backlight = mdss_dsi_panel_bl_ctrl;
	ctrl_pdata->mtp = find_mtp;

	msd.dstat.acl_on = false;

	mpd.gamma_update.cmd= samsung_panel_gamma_update_cmds;
	mpd.gamma_update.size =  ARRAY_SIZE(samsung_panel_gamma_update_cmds);

	mpd.elvss_update.cmd= samsung_panel_elvss_update_cmds;
	mpd.elvss_update.size= ARRAY_SIZE(samsung_panel_elvss_update_cmds);

	mpd.elvss_update_4_8.cmd = samsung_panel_elvss_update_cmds_4_8;
	mpd.elvss_update_4_8.size =	 ARRAY_SIZE(samsung_panel_elvss_update_cmds_4_8);

	mpd.acl_on.cmd = samsung_panel_acl_on_cmds;
	mpd.acl_on.size =	ARRAY_SIZE(samsung_panel_acl_on_cmds);

	mpd.acl_off.cmd = samsung_panel_acl_off_cmds;
	mpd.acl_off.size =  ARRAY_SIZE(samsung_panel_acl_off_cmds);

	mpd.acl_update.cmd = samsung_panel_acl_update_cmds;
	mpd.acl_update.size = ARRAY_SIZE(samsung_panel_acl_update_cmds);

	mpd.combined_ctrl.cmd = combined_ctrl;
	mpd.combined_ctrl.size = ARRAY_SIZE(combined_ctrl);

	mpd.gamma_initial = gamma_cond_set_4_8;
	mpd.gamma_smartdim_4_8 = gamma_cond_300cd_4_8;

	mpd.lcd_current_cd_idx = -1;
	mpd.lux_table = lux_tbl_acl;
	mpd.lux_table_max_cnt = ARRAY_SIZE(lux_tbl_acl);

	mpd.set_acl = set_acl_on_level;
	mpd.prepare_brightness_control_cmd_array = prepare_brightness_control_cmd_array;
	msd.dstat.gamma_mode = GAMMA_SMART;
	panel_cond_aid_ref[28] = 0x37;

#if defined(CONFIG_LCD_CLASS_DEVICE)
	lcd_device = lcd_device_register("panel", &pdev->dev, NULL,
					&mdss_s6e8aa0a_disp_props);

	if (IS_ERR(lcd_device)) {
		rc = PTR_ERR(lcd_device);
		printk(KERN_ERR "lcd : failed to register device\n");
		return rc;
	}

	sysfs_remove_file(&lcd_device->dev.kobj,&dev_attr_lcd_power.attr);

	rc = sysfs_create_file(&lcd_device->dev.kobj,&dev_attr_lcd_power.attr);

	if (rc) {
		pr_info("sysfs create fail-%s\n",dev_attr_lcd_power.attr.name);

	}
	rc = sysfs_create_file(&lcd_device->dev.kobj,
					&dev_attr_lcd_type.attr);
	if (rc) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_lcd_type.attr.name);
	}

	rc = sysfs_create_file(&lcd_device->dev.kobj,
					&dev_attr_power_reduce.attr);
	if (rc) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_power_reduce.attr.name);
	}

	rc= sysfs_create_file(&lcd_device->dev.kobj,
					&dev_attr_siop_enable.attr);
	if (rc) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_siop_enable.attr.name);
	}
#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
	bd = backlight_device_register("panel", &lcd_device->dev,
			NULL, NULL, NULL);
	if (IS_ERR(bd)) {
		rc = PTR_ERR(bd);
		pr_info("backlight : failed to register device\n");
		return rc;
	}
	rc= sysfs_create_file(&bd->dev.kobj,
					&dev_attr_auto_brightness.attr);
	if (rc) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_auto_brightness.attr.name);
	}



#endif
#endif
#if defined(CONFIG_MDNIE_LITE_TUNING)
		pr_info("[%s] CONFIG_MDNIE_LITE_TUNING ok ! initclass called!\n",__func__);
		init_mdnie_class();
		//mdnie_lite_tuning_init(&msd);
#endif
/* Needed to pass GPIO DVS for OLED_DET GPIO*/
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
#ifdef ESD_DEBUG
	rc = sysfs_create_file(&lcd_device->dev.kobj,
							&dev_attr_esd_check.attr);
	if (rc) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_esd_check.attr.name);
	}
#endif
	msd.msm_pdev = pdev;

	disp_esd_gpio =of_get_named_gpio(node,"qcom,oled-esd-gpio", 0);
	err_fg_gpio = gpio_to_irq(disp_esd_gpio);
	rc = gpio_request(disp_esd_gpio, "err_fg");
	if (rc) {
		pr_err("request gpio GPIO_ESD failed, ret=%d\n",rc);
		gpio_free(disp_esd_gpio);
		return rc;
	}
	gpio_tlmm_config(GPIO_CFG(disp_esd_gpio,  0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	rc = gpio_direction_input(disp_esd_gpio);
	if (unlikely(rc < 0)) {
		pr_err("%s: failed to set gpio %d as input (%d)\n",
			__func__, disp_esd_gpio, rc);
	}
#endif
	return 0;
}
