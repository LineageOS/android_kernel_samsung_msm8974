/* Copyright (c) 2012, Samsung Electronics. All rights reserved.
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
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include "mdss_dsi.h"
#include "mdss_samsung_dsi_panel.h"
#include "mdss_fb.h"

#if defined(CONFIG_MDNIE_LITE_TUNING)
#include "mdnie_lite_tuning.h"
#endif

#define DDI_VIDEO_ENHANCE_TUNING
#if defined(DDI_VIDEO_ENHANCE_TUNING)
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#endif

#include <asm/system_info.h>
#if defined(CONFIG_MIPI_LCD_S6E3FA0_FORCE_VIDEO_MODE)
#define CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL 1
#endif
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_YOUM_CMD_FULL_HD_PT_PANEL)
#define SMART_ACL
#define HBM_RE
#define TEMPERATURE_ELVSS_S6E3FA0
#define PARTIAL_UPDATE
#endif
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL)
#define LDI_FPS_CHANGE
#define LDI_ADJ_VDDM_OFFSET
#define FORCE_500CD
#endif

#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
#define TEMPERATURE_ELVSS_S6E8AA4
#define SMART_ACL
#define HBM_RE
#define NOT_USING_ACL_CONT
#endif
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL)
//#define TEMPERATURE_ELVSS
#define HBM_RE
#endif

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
#define NOT_USING_ACL_CONT
#endif

#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_LDI_EA8061)
#define VIRTUAL_GAMMA
#endif

#if defined(CONFIG_MACH_KS01SKT) || defined(CONFIG_MACH_KS01KTT) || defined(CONFIG_MACH_KS01LGT) \
	|| defined(CONFIG_MACH_HLTESKT) || defined(CONFIG_MACH_HLTELGT) || defined(CONFIG_MACH_HLTEKTT)
#define octa_manufacture_date
#endif

#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL)
#define SMART_ACL
#define NOT_USING_ACL_CONT
#endif

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
#define SMART_ACL
#define NOT_USING_ACL_CONT
#define HBM_RE
#endif

#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
#define TEMPERATURE_ELVSS
#define SMART_ACL
#define NOT_USING_ACL_CONT
#define HBM_RE
#define octa_manufacture_date
#endif

#define DT_CMD_HDR 6

#if defined(octa_manufacture_date)
static struct dsi_cmd nv_date_read_cmds;
char mdate_buffer[10];
#endif

static struct dsi_buf dsi_panel_tx_buf;
static struct dsi_buf dsi_panel_rx_buf;

#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)
static struct dsi_cmd magna_read_pos_cmds;
static struct dsi_cmd magna_read_run_cmds;
#endif

static struct dsi_cmd nv_mdnie_read_cmds;
static struct dsi_cmd nv_mtp_read_cmds;
static struct dsi_cmd nv_mtp_read_register_set_cmds;
#if defined(HBM_RE)
static struct dsi_cmd nv_mtp_hbm_read_cmds;
static struct dsi_cmd nv_mtp_hbm2_read_cmds;
static struct dsi_cmd hbm_gamma_cmds_list;
static struct dsi_cmd hbm_etc_cmds_evt0_second_list;
static struct dsi_cmd hbm_etc_cmds_evt1_list;
static struct dsi_cmd hbm_etc_cmds_evt1_second_list;
static struct dsi_cmd hbm_etc_cmds_evt1_H_revI_list;
static struct dsi_cmd hbm_etc_cmds_evt1_H_revJ_list;

static struct dsi_cmd hbm_etc_cmds_list;
#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)\
	|| defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
static struct dsi_cmd nv_mtp_hbm3_read_cmds;
static struct dsi_cmd hbm_hbm_off_elvss_cmds;
#endif
#elif defined(CONFIG_HBM_PSRE)
static struct dsi_cmd nv_mtp_hbm_read_cmds;
static struct dsi_cmd nv_mtp_hbm2_read_cmds;
static struct dsi_cmd nv_mtp_hbm3_read_cmds;
static struct dsi_cmd nv_hbm_elvss_offset_cmds;
static struct dsi_cmd nv_production_day_cmds;
static struct dsi_cmd hbm_gamma_cmds_list;
static struct dsi_cmd hbm_etc_cmds_list;
#endif
static struct dsi_cmd nv_enable_cmds;
static struct dsi_cmd nv_disable_cmds;
#ifdef DEBUG_LDI_STATUS
static struct dsi_cmd ldi_debug_cmds;
#endif
#ifdef LDI_FPS_CHANGE
static struct dsi_cmd read_ldi_fps_cmds;
static struct dsi_cmd write_ldi_fps_cmds;
#endif
#ifdef LDI_ADJ_VDDM_OFFSET
static struct dsi_cmd read_vddm_ref_cmds;
static struct dsi_cmd write_vddm_offset_cmds;
#endif

#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
static struct dsi_cmd test_key_enable_cmds;
static struct dsi_cmd test_key_disable_cmds;
#endif

static struct dsi_cmd manufacture_id_cmds;
static struct dsi_cmd display_qcom_on_cmds;
static struct dsi_cmd display_qcom_off_cmds;
static struct dsi_cmd display_on_cmd;
static struct dsi_cmd display_off_cmd;
static struct dsi_cmd display_blank_cmd;
static struct dsi_cmd display_unblank_cmd;
static struct dsi_cmd acl_off_cmd;
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
static struct dsi_cmd touchsensing_on_cmd;
static struct dsi_cmd touchsensing_off_cmd;
#endif
#if defined(PARTIAL_UPDATE)
static struct dsi_cmd partialdisp_on_cmd;
static struct dsi_cmd partialdisp_off_cmd;
#endif
#if defined(FORCE_500CD)
static struct dsi_cmd force_500;
#endif
/*
contains the list of acl commands
available for different brightness levels
*/
static struct dsi_cmd acl_cmds_list;
static struct dsi_cmd elvss_cmds_list;
static struct dsi_cmd aid_cmds_list;
static struct dsi_cmd aid_cmds_list_350;
static struct dsi_cmd gamma_cmds_list;
#if !defined(NOT_USING_ACL_CONT)
static struct dsi_cmd aclcont_cmds_list;
#endif
#if defined(TEMPERATURE_ELVSS_S6E3FA0) || defined(TEMPERATURE_ELVSS_S6E8AA4)
static struct dsi_cmd elvss_lowtemp_cmds_list;
#endif
#if defined(TEMPERATURE_ELVSS)
static struct dsi_cmd elvss_lowtemp_cmds_list;
static struct dsi_cmd elvss_tempcompen_cmds_list;
#endif
#if defined(SMART_ACL)
static struct dsi_cmd smart_acl_elvss_cmds_list;
#endif
#if defined(CONFIG_LCD_CRACK_RECOVERY)
static struct dsi_cmd lcd_crack_rec_cmd_list;
#endif

/*
contains mapping between bl_level and
index number of corresponding acl command
in acl command list
*/
static struct cmd_map acl_map_table;
static struct cmd_map elvss_map_table;
static struct cmd_map aid_map_table;
static struct candella_lux_map candela_map_table;
static struct candella_lux_map candela_map_table_350;

#if defined(SMART_ACL)
static struct cmd_map smart_acl_elvss_map_table;
#endif

static struct mipi_samsung_driver_data msd;
/*List of supported Panels with HW revision detail
 * (one structure per project)
 * {hw_rev,"label string given in panel dtsi file"}
 * */
static struct  panel_hrev panel_supp_cdp[]= {
	{"samsung amoled 720p video mode dsi S6E8AA3X01 panel", PANEL_720P_AMOLED_S6E8AA3X01},
	{"samsung amoled 1080p video mode dsi S6E8FA0 panel", PANEL_1080P_OCTA_S6E8FA0},
	{"samsung amoled 1080p video mode dsi S6E3FA0 panel", PANEL_1080P_OCTA_S6E3FA0},
	{"samsung amoled 1080p command mode dsi S6E3FA0 panel", PANEL_1080P_OCTA_S6E3FA0_CMD},
	{"samsung amoled 1080p command mode dsi S6E3FA1 panel", PANEL_1080P_YOUM_S6E3FA1_CMD},
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
	{"samsung amoled WVGA video mode dsi S6E88A0 panel", PANEL_WVGA_OCTA_S6E88A0},
#endif
	{"magna amoled 720p video mode dsi D53D6EA8061V panel", PANEL_720P_OCTA_D53D6EA8061V},
	{"samsung amoled 720p video mode dsi S6E8AA0A01 panel", PANEL_720P_OCTA_S6E8AA0},
	{"samsung amoled 720p video mode dsi EA8061 panel", PANEL_720P_OCTA_EA8061_VIDEO},
	{"samsung amoled 720p video mode dsi S6E8AA4 panel", PANEL_720P_OCTA_S6E8AA4_VIDEO},
	{"samsung amoled 720p video mode dsi D53D6EA8061V panel", PANEL_720P_OCTA_D53D6EA8061V_VIDEO},
	{"samsung amoled qhd video mode dsi S6E88A0 panel",PANEL_QHD_OCTA_S6E88A0_VIDEO},
	{"samsung octa hd video mode dsi EA8061 panel",PANEL_HD_OCTA_D53D6EA8061_VIDEO},
	{NULL}
};

static struct dsi_cmd_desc brightness_packet[] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
};
#ifdef LDI_ADJ_VDDM_OFFSET
unsigned int ldi_vddm_lut[128][2] = {
	{0, 13}, {1, 13}, {2, 14}, {3, 15}, {4, 16}, {5, 17}, {6, 18}, {7, 19}, {8, 20}, {9, 21},
	{10, 22}, {11, 23}, {12, 24}, {13, 25}, {14, 26}, {15, 27}, {16, 28}, {17, 29}, {18, 30}, {19, 31},
	{20, 32}, {21, 33}, {22, 34}, {23, 35}, {24, 36}, {25, 37}, {26, 38}, {27, 39}, {28, 40}, {29, 41},
	{30, 42}, {31, 43}, {32, 44}, {33, 45}, {34, 46}, {35, 47}, {36, 48}, {37, 49}, {38, 50}, {39, 51},
	{40, 52}, {41, 53}, {42, 54}, {43, 55}, {44, 56}, {45, 57}, {46, 58}, {47, 59}, {48, 60}, {49, 61},
	{50, 62}, {51, 63}, {52, 63}, {53, 63}, {54, 63}, {55, 63}, {56, 63}, {57, 63}, {58, 63}, {59, 63},
	{60, 63}, {61, 63}, {62, 63}, {63, 63}, {64, 12}, {65, 11}, {66, 10}, {67, 9}, {68, 8}, {69, 7},
	{70, 6}, {71, 5}, {72, 4}, {73, 3}, {74, 2}, {75, 1}, {76, 64}, {77, 65}, {78, 66}, {79, 67},
	{80, 68}, {81, 69}, {82, 70}, {83, 71}, {84, 72}, {85, 73}, {86, 74}, {87, 75}, {88, 76}, {89, 77},
	{90, 78}, {91, 79}, {92, 80}, {93, 81}, {94, 82}, {95, 83}, {96, 84}, {97, 85}, {98, 86}, {99, 87},
	{100, 88}, {101, 89}, {102, 90}, {103, 91}, {104, 92}, {105, 93}, {106, 94}, {107, 95}, {108, 96}, {109, 97},
	{110, 98}, {111, 99}, {112, 100}, {113, 101}, {114, 102}, {115, 103}, {116, 104}, {117, 105}, {118, 106}, {119, 107},
	{120, 108}, {121, 109}, {122, 110}, {123, 111}, {124, 112}, {125, 113}, {126, 114}, {127, 115},
};
#endif

#define MAX_BR_PACKET_SIZE sizeof(brightness_packet)/sizeof(struct dsi_cmd_desc)

DEFINE_LED_TRIGGER(bl_led_trigger);

static struct mdss_dsi_phy_ctrl phy_params;

static int lcd_attached = 1;
static int lcd_id = 0;
static char board_rev;

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
#define ESD_DEBUG 1

struct work_struct  err_fg_work;
static int err_fg_gpio = 0;	/* PM_GPIO4 */
static int esd_count = 0;
static int err_fg_working = 0;
static int err_fg_enable = 0;
#endif
#if defined(CONFIG_LCD_CRACK_RECOVERY)
struct work_struct lcd_crack_rec_work;
static int lcd_crack_gpio = 0;
static int lcd_crack_rec_working = 0;
static int lcd_crack_rec_enable = 0;
#endif
static int mipi_samsung_disp_send_cmd(
		enum mipi_samsung_cmd_list cmd,
		unsigned char lock);

#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
extern void mdss_dsi_panel_touchsensing(int enable);
#endif
int get_lcd_attached(void);

#if (defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_LDI_EA8061))\
		|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
/* fresco ldi id3 */
#define EVT0_REV_A 0x80
#define EVT1_REV_B_C 0xA1
#define EVT1_REV_D 0xA2

/*
oled id : msm gpio 91
1: s.lsi
0: magna
 */
static int fresco_oled_id = 1;

int get_oled_id(void);
#endif

int set_panel_rev(unsigned int id)
{
	switch (id & 0xFF) {
		/* H OCTA panel */
		case 0x00:
			pr_info("%s : 0x00 EVT0_ID \n",__func__);
			msd.id3 = EVT0_ID;
			break;
		case 0x01:
			pr_info("%s : 0x01 EVT0_SECOND_ID \n",__func__);
			msd.id3 = EVT0_SECOND_ID;
			break;
		case 0x21:
			pr_info("%s : 0x21 EVT1_ID \n",__func__);
			msd.id3 = EVT1_ID;
			break;
		case 0x22:
			pr_info("%s : 0x22 EVT1_SECOND_ID \n",__func__);
			msd.id3 = EVT1_SECOND_ID;
			break;
		case 0x23:
			pr_info("%s : 0x23 EVT1_H_REV_I \n",__func__);
			msd.id3 = EVT1_H_REV_I;
			msd.panel_350cd = 1;
			break;
		case 0x24:
			pr_info("%s : 0x24 EVT1_H_REV_J \n",__func__);
			msd.id3 = EVT1_H_REV_J;
			msd.panel_350cd = 1;
			break;

		/* F YOUM panel */
		case 0x10:
			pr_info("%s : 0x10 EVT0_F_REV_A \n",__func__);
			msd.id3 = EVT0_F_REV_A;
			break;
		case 0x11:
			pr_info("%s : 0x11 EVT0_F_REV_E \n",__func__);
			msd.id3 = EVT0_F_REV_E;
			msd.panel_350cd = 1;
			break;
		case 0x12:
			pr_info("%s : 0x12 EVT0_F_REV_F \n",__func__);
			msd.id3 = EVT0_F_REV_F;
			msd.panel_350cd = 1;
			break;
		case 0x32:
			pr_info("%s : 0x32 EVT2_F_REV_G \n",__func__);
			msd.id3 = EVT2_F_REV_G;
			msd.panel_350cd = 1;
			break;
		case 0x43:
#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
			pr_info("%s : 0x43 Fresco_kor_magna panel \n",__func__);
			msd.panel_350cd = 0;
#else
			pr_info("%s : 0x43 EVT2_Fresco_REV_G \n",__func__);
			msd.id3 = EVT2_FRESCO_REV_G;
			msd.panel_350cd = 1;
#endif
			break;
		case 0x82:
			pr_info("%s : 0x82 EVT0_EA8061V_REV_A \n",__func__);
			msd.id3 = EVT0_EA8061V_REV_A;
			msd.panel_350cd = 1;
			break;
		case 0x84:
			pr_info("%s : 0x84 EVT0_EA8061V_KMINI_REV_A \n",__func__);
			msd.id3 = EVT0_EA8061V_KMINI_REV_A;
			msd.panel_350cd = 1;
			break;
		case 0x40:
			pr_info("%s : 0x40 EVT2_EA8061_Hestia_REV_A \n",__func__);
			msd.id3 = EVT2_EA8061_HESTIA_REV_A;
			msd.panel_350cd = 1;
			break;
		case 0x47:
#if defined(CONFIG_SEC_HESTIA_PROJECT)
			pr_info("%s : 0x47 used for HESTIA EVT2_EA8061_Hestia_REV_I \n",__func__);
			msd.id3 = EVT2_EA8061_HESTIA_REV_I;
			msd.panel_350cd = 1;
#else
			pr_info("%s : 0x47 need to check \n",__func__);
			msd.id3 = 0;
			msd.panel_350cd = 0;
#endif
			break;
		case 0x48:
			pr_info("%s : 0x48 EVT2_EA8061_Hestia_REV_J \n",__func__);
			msd.id3 = EVT2_EA8061_HESTIA_REV_J;
			msd.panel_350cd = 1;
			break;
		case 0x95:
			pr_info("%s : 0x82 EVT2_EA8061V_REV_C \n",__func__);
			msd.id3 = EVT2_EA8061V_REV_C;
			msd.panel_350cd = 1;
			break;
		case 0x96:
			pr_info("%s : 0x96 EVT2_EA8061V_REV_D \n",__func__);
			msd.id3 = EVT2_EA8061V_REV_D;
			msd.panel_350cd = 1;
			break;
		case 0x97:
			pr_info("%s : 0x96 EVT2_EA8061V_REV_E \n",__func__);
			msd.id3 = EVT2_EA8061V_REV_E;
			msd.panel_350cd = 1;
			break;
#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
		/*FRESCO LSI panel*/
		case 0x80: /*evt 0 rev A*/
			pr_info("%s : 0x80 EVT0_REV_A \n",__func__);
			msd.id3 = EVT0_REV_A;
			msd.panel_350cd = 1;
			break;
		case 0xA1: /*evt 1	 rev B,  rev C*/
			pr_info("%s : 0xA1 EVT1_REV_B or EVT1_REV_C\n",__func__);
			msd.id3 = EVT1_REV_B_C;
			msd.panel_350cd = 1;
			break;
		case 0xA2:/*evt 1  rev D*/
			pr_info("%s : 0xA2 EVT1_REV_D \n",__func__);
			msd.id3 = EVT1_REV_D;
			msd.panel_350cd = 1;
			break;
#endif
		default:
			pr_err("%s : can't find panel id.. \n", __func__);
			return -EINVAL;
			break;
	}

	return 1;
}

void mdss_dsi_panel_pwm_cfg(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ret;

	if (!gpio_is_valid(ctrl->pwm_pmic_gpio)) {
		pr_err("%s: pwm_pmic_gpio=%d Invalid\n", __func__,
			ctrl->pwm_pmic_gpio);
		return;
	}

	ret = gpio_request(ctrl->pwm_pmic_gpio, "disp_pwm");
		if (ret) {
			pr_err("%s: pwm_pmic_gpio=%d request failed\n", __func__,
					ctrl->pwm_pmic_gpio);
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

	pr_debug("%s: bklt_ctrl=%d pwm_period=%d pwm_pmic_gpio=%d pwm_lpg_chan=%d\n",
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

static char dcs_cmd[2] = {0x54, 0x00}; /* DTYPE_DCS_READ */
static struct dsi_cmd_desc dcs_read_cmd = {
	{DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(dcs_cmd)},
	dcs_cmd
};

static void dcs_read_cb(int data)
{
	pr_info("%s: bklt_ctrl=%x\n", __func__, data);
}

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
	cmdreq.rlen = 1;
	cmdreq.cb = dcs_read_cb; /* call back */
	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
	/*
	 * blocked here, untill call back called
	 */
	return 0;
}

void mdss_dsi_samsung_panel_reset(struct mdss_panel_data *pdata, int enable)
{
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
	int rc =0;
#endif
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);


	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_debug("%s:%d, enable line not configured\n",
			   __func__, __LINE__);
	}

#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL)
	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio2)) {
		pr_debug("%s:%d, enable line2 not configured\n",
			   __func__, __LINE__);
	}
#endif

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
	}

	pr_debug("%s: enable = %d\n", __func__, enable);

	if (enable) {
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
		if (gpio_is_valid(ctrl_pdata->expander_enble_gpio)) {
			gpio_set_value((ctrl_pdata->expander_enble_gpio), 1);
			mdelay(5);
		}
		if (gpio_is_valid(ctrl_pdata->rst_gpio)) {
			rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->rst_gpio, 0,
				GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
			if (rc)
				pr_err("request rst_gpio failed, rc=%d\n",rc);
		}
#endif
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)||\
	defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			gpio_tlmm_config(GPIO_CFG(ctrl_pdata->disp_en_gpio, 0,
				GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
			wmb();
		}
		if (gpio_is_valid(ctrl_pdata->rst_gpio)) {
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			mdelay(3);
			wmb();
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			mdelay(10);
			wmb();
		}
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL)
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
			wmb();
		}
		if (gpio_is_valid(ctrl_pdata->rst_gpio)) {
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			msleep(20);
			wmb();
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			udelay(200);
			wmb();
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			msleep(20);
			wmb();
		}
#else
		if (gpio_is_valid(ctrl_pdata->rst_gpio)) {
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			msleep(20);
			wmb();
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			udelay(200);
			wmb();
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			msleep(20);
			wmb();
		}
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
			wmb();
		}
#endif
#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL)
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio2)) {
			gpio_set_value((ctrl_pdata->disp_en_gpio2), 1);
			wmb();
		}
#endif
#if defined(CONFIG_LCD_CRACK_RECOVERY)
		if(gpio_is_valid(lcd_crack_gpio))
		{
			rc = gpio_tlmm_config(GPIO_CFG(lcd_crack_gpio, 0,
					GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
			if (rc)
			{
				pr_err("request lcd_crack_gpio failed, rc=%d\n",rc);
			}
		}
#endif

		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
				__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
		if (gpio_is_valid(ctrl_pdata->rst_gpio)) {
			gpio_tlmm_config(GPIO_CFG(ctrl_pdata->rst_gpio, 0,
				GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
				GPIO_CFG_DISABLE);
		gpio_set_value((ctrl_pdata->rst_gpio), 0);
		}
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			if(ctrl_pdata->disp_en_gpio == 115)
				gpio_tlmm_config(GPIO_CFG(ctrl_pdata->disp_en_gpio, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
					GPIO_CFG_DISABLE);
			else
				gpio_tlmm_config(GPIO_CFG(ctrl_pdata->disp_en_gpio, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
					GPIO_CFG_DISABLE);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
		}
#else
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
		}
#endif
	if (gpio_is_valid(ctrl_pdata->rst_gpio))
		gpio_set_value((ctrl_pdata->rst_gpio), 0);
#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL)
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio2)) {
			gpio_set_value((ctrl_pdata->disp_en_gpio2), 0);
		}
#endif

#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
		if (gpio_is_valid(ctrl_pdata->expander_enble_gpio))
			gpio_set_value((ctrl_pdata->expander_enble_gpio), 0);
#endif
#if defined(CONFIG_LCD_CRACK_RECOVERY)
		if(gpio_is_valid(lcd_crack_gpio))
		{
			rc = gpio_tlmm_config(GPIO_CFG(lcd_crack_gpio, 0,
					GPIO_CFG_INPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
					GPIO_CFG_DISABLE);
			if (rc)
				pr_err("request lcd_crack_gpio failed, rc=%d\n",rc);
			else
				gpio_set_value(lcd_crack_gpio, 0);
		}
#endif
	}
}

static int get_candela_value(int bl_level)
{
	if (msd.panel_350cd)
		return candela_map_table_350.lux_tab[candela_map_table_350.bkl[bl_level]];
	else
		return candela_map_table.lux_tab[candela_map_table.bkl[bl_level]];
}

static int get_cmd_idx(int bl_level)
{
	if (msd.panel_350cd)
		return candela_map_table_350.cmd_idx[candela_map_table_350.bkl[bl_level]];
	else
		return candela_map_table.cmd_idx[candela_map_table.bkl[bl_level]];
}

#if defined(CONFIG_LCD_CLASS_DEVICE)
static ssize_t mipi_samsung_disp_get_power(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct msm_fb_data_type *mfd = msd.mfd;
	int rc;

	if (unlikely(!mfd))
		return -ENODEV;
	if (unlikely(mfd->key != MFD_KEY))
		return -EINVAL;

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n", mfd->panel_power_on);
	pr_info("mipi_samsung_disp_get_power(%d)\n", mfd->panel_power_on);

	return rc;
}

static ssize_t mipi_samsung_disp_set_power(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int power;
	struct msm_fb_data_type *mfd = msd.mfd;

	if (sscanf(buf, "%u", &power) != 1)
		return -EINVAL;

	if (power == mfd->panel_power_on)
		return 0;

	if (power) {
		mfd->fbi->fbops->fb_blank(FB_BLANK_UNBLANK, mfd->fbi);
		mfd->fbi->fbops->fb_pan_display(&mfd->fbi->var, mfd->fbi);
		mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
	} else {
		mfd->fbi->fbops->fb_blank(FB_BLANK_POWERDOWN, mfd->fbi);
	}

	pr_info("mipi_samsung_disp_set_power\n");

	return size;
}

static ssize_t mipi_samsung_disp_lcdtype_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char temp[100];
	switch (msd.panel) {
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
		case PANEL_WVGA_OCTA_S6E88A0:
			snprintf(temp, 20, "SMD_AMS367AV01");
					break;
#endif
		case PANEL_1080P_OCTA_S6E8FA0:
			snprintf(temp, 20, "SDC_AMS499QP01");
					break;
		case PANEL_1080P_OCTA_S6E3FA0:
		case PANEL_1080P_OCTA_S6E3FA0_CMD:
			snprintf(temp, 20, "SDC_AMS568AT01");
					break;
		case PANEL_1080P_YOUM_S6E3FA1_CMD:
			snprintf(temp, 20, "SDC_AMB568AU01");
					break;
		case PANEL_720P_OCTA_D53D6EA8061V:
			snprintf(temp, 20, "SDC_D53D6EA8061V");
					break;
		case PANEL_720P_OCTA_S6E8AA0:
			snprintf(temp, 20, "SDC_AMS480GY01");
					break;
		case PANEL_720P_OCTA_D53D6EA8061V_VIDEO:
			snprintf(temp, 20, "SDC_200283");
					break;
		case PANEL_HD_OCTA_D53D6EA8061_VIDEO:
			if (msd.manufacture_id)
				snprintf(temp, 20, "SDC_%x\n",msd.manufacture_id);
					break;
		case PANEL_720P_OCTA_S6E8AA4_VIDEO:
			snprintf(temp, 20, "SDC_AMS549BU05");
					break;
		case PANEL_QHD_OCTA_S6E88A0_VIDEO:
			snprintf(temp, 20, "SDC_AMS517CY01");
					break;
		default :
			snprintf(temp, strnlen(msd.panel_name, 100),
								msd.panel_name);
					break;
	}
	strlcat(buf, temp, 100);
	return strnlen(buf, 100);
}

static ssize_t mipi_samsung_disp_windowtype_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char temp[15];
	int id1, id2, id3;
	id1 = (msd.manufacture_id & 0x00FF0000) >> 16;
	id2 = (msd.manufacture_id & 0x0000FF00) >> 8;
	id3 = msd.manufacture_id & 0xFF;

	snprintf(temp, PAGE_SIZE, "%x %x %x\n",	id1, id2, id3);
	strlcat(buf, temp, 15);
	return strnlen(buf, 15);
}


static ssize_t mipi_samsung_disp_acl_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n", msd.dstat.acl_on);
	pr_info("acl status: %d\n", *buf);

	return rc;
}

/*
	This function takes aid_map_table and uses cd_idx,
	to get the index of the command in aid command list.

*/
static struct dsi_cmd get_aid_aor_control_set(int cd_idx)
{
	struct dsi_cmd aid_control = {0,};
	int cmd_idx = 0, payload_size = 0;
	char *p_payload, *c_payload;
	int p_idx = msd.dstat.curr_aid_idx;

	if (!aid_map_table.size || !(cd_idx < aid_map_table.size))
		goto end;

	/* Get index in the aid command list*/
	cmd_idx = aid_map_table.cmd_idx[cd_idx];

	if(msd.id3 == EVT1_H_REV_I\
		|| msd.id3 == EVT1_H_REV_J\
		|| msd.id3 == EVT0_EA8061V_REV_A\
		|| msd.id3 == EVT2_EA8061V_REV_C\
		|| msd.id3 == EVT2_EA8061V_REV_D\
		|| msd.id3 == EVT2_EA8061V_REV_E\
		|| msd.id3 == EVT2_FRESCO_REV_G\
		|| msd.id3 == EVT0_EA8061V_KMINI_REV_A\
		|| msd.id3 == EVT2_EA8061_HESTIA_REV_I\
		||msd.id3 == EVT2_EA8061_HESTIA_REV_J\
		|| msd.id3 == EVT2_EA8061_HESTIA_REV_A)
		c_payload = aid_cmds_list_350.cmd_desc[cmd_idx].payload;
	else
		c_payload = aid_cmds_list.cmd_desc[cmd_idx].payload;

	/* Check if current & previous commands are same */
	if (p_idx >= 0) {
		if (msd.id3 == EVT1_H_REV_I\
			|| msd.id3 == EVT1_H_REV_J\
			|| msd.id3 == EVT0_EA8061V_REV_A\
			|| msd.id3 == EVT2_EA8061V_REV_C\
			|| msd.id3 == EVT2_EA8061V_REV_D\
			|| msd.id3 == EVT2_EA8061V_REV_E\
			|| msd.id3 == EVT2_FRESCO_REV_G\
			|| msd.id3 == EVT0_EA8061V_KMINI_REV_A\
			|| msd.id3 == EVT2_EA8061_HESTIA_REV_I\
			||msd.id3 == EVT2_EA8061_HESTIA_REV_J\
			|| msd.id3 == EVT2_EA8061_HESTIA_REV_A){
			p_payload = aid_cmds_list_350.cmd_desc[p_idx].payload;
			payload_size = aid_cmds_list_350.cmd_desc[p_idx].dchdr.dlen;
		} else {
			p_payload = aid_cmds_list.cmd_desc[p_idx].payload;
			payload_size = aid_cmds_list.cmd_desc[p_idx].dchdr.dlen;
		}
		if (!memcmp(p_payload, c_payload, payload_size))
			goto end;
	}

	/* Get the command desc */
	if(msd.id3 == EVT1_H_REV_I\
		|| msd.id3 == EVT1_H_REV_J\
		|| msd.id3 == EVT0_EA8061V_REV_A\
		|| msd.id3 == EVT2_EA8061V_REV_C\
		|| msd.id3 == EVT2_EA8061V_REV_D\
		|| msd.id3 == EVT2_EA8061V_REV_E\
		|| msd.id3 == EVT2_FRESCO_REV_G\
		|| msd.id3 == EVT0_EA8061V_KMINI_REV_A\
		|| msd.id3 == EVT2_EA8061_HESTIA_REV_I\
		||msd.id3 == EVT2_EA8061_HESTIA_REV_J\
		|| msd.id3 == EVT2_EA8061_HESTIA_REV_A)
		aid_control.cmd_desc = &(aid_cmds_list_350.cmd_desc[cmd_idx]);
	else
		aid_control.cmd_desc = &(aid_cmds_list.cmd_desc[cmd_idx]);

	aid_control.num_of_cmds = 1;
	msd.dstat.curr_aid_idx = cmd_idx;

	switch (msd.panel) {
	case PANEL_720P_AMOLED_S6E8AA3X01:
		/* Do any panel specfific customization here */
		break;
	}
end:
	return aid_control;
}

/*
	This function takes acl_map_table and uses cd_idx,
	to get the index of the command in elvss command list.

*/
#if !defined(NOT_USING_ACL_CONT)
static struct dsi_cmd get_aclcont_control_set(void)
{
	struct dsi_cmd aclcont_control = {0,};
	int cmd_idx = 0;
	int acl_cond = msd.dstat.curr_acl_cond;

	if (acl_cond) /* already acl condition setted */
		goto end;

	/* Get the command desc */
	aclcont_control.cmd_desc = &(aclcont_cmds_list.cmd_desc[cmd_idx]);
	aclcont_control.num_of_cmds = 1;
	msd.dstat.curr_acl_cond = 1;

end:
	return aclcont_control;
}
#endif

/*
	This function takes acl_map_table and uses cd_idx,
	to get the index of the command in elvss command list.

*/
static struct dsi_cmd get_acl_control_set(int cd_idx)
{
	struct dsi_cmd acl_control = {0,};
	int cmd_idx = 0, payload_size = 0;
	char *p_payload, *c_payload;
	int p_idx = msd.dstat.curr_acl_idx;

	if (!acl_map_table.size || !(cd_idx < acl_map_table.size))
		goto end;

	/* Get index in the acl command list*/
	cmd_idx = acl_map_table.cmd_idx[cd_idx];
	c_payload = acl_cmds_list.cmd_desc[cmd_idx].payload;

	/* Check if current & previous commands are same */
	if (p_idx >= 0) {
		p_payload = acl_cmds_list.cmd_desc[p_idx].payload;
		payload_size = acl_cmds_list.cmd_desc[p_idx].dchdr.dlen;
		if (!memcmp(p_payload, c_payload, payload_size))
			goto end;
	}

	/* Get the command desc */
	acl_control.cmd_desc = &(acl_cmds_list.cmd_desc[cmd_idx]);
	acl_control.num_of_cmds = 1;
	msd.dstat.curr_acl_idx = cmd_idx;

	switch (msd.panel) {
	case PANEL_720P_AMOLED_S6E8AA3X01:
		/* Do any panel specfific customization here */
		break;
	}
end:
	return acl_control;
}

static struct dsi_cmd get_acl_control_off_set(void)
{
	struct dsi_cmd acl_control = {0,};
	int p_idx = msd.dstat.curr_acl_idx;

	/* Check if current & previous commands are same */
	if (p_idx == 0) {
		/* already acl off */
		goto end;
	}

	/* Get the command desc */
	acl_control.cmd_desc = &(acl_cmds_list.cmd_desc[0]); /* idx 0 : ACL OFF */
	acl_control.num_of_cmds = 1;

	msd.dstat.curr_acl_idx = 0;
	msd.dstat.curr_acl_cond = 0;

end:
	return acl_control;
}

#if defined(TEMPERATURE_ELVSS_S6E3FA0) || defined(TEMPERATURE_ELVSS_S6E8AA4)
// ELVSS TEMPERATURE COMPENSATION for S6E3FA0
static struct dsi_cmd get_elvss_tempcompen_control_set(void)
{
	struct dsi_cmd elvss_tempcompen_control = {0,};

	/* Get the command desc */
	if (msd.dstat.temperature >= 0) {
		pr_debug("%s temp >= 0 \n",__func__);
		elvss_lowtemp_cmds_list.cmd_desc[1].payload[1] = 0x19;
		elvss_lowtemp_cmds_list.cmd_desc[2].payload[1] = 0x88;
	} else if (msd.dstat.temperature > -20) {
		pr_debug("%s 0 > temp > -20 \n",__func__);
		elvss_lowtemp_cmds_list.cmd_desc[1].payload[1] = 0x00;
		elvss_lowtemp_cmds_list.cmd_desc[2].payload[1] = 0x8C;
	} else {
		pr_debug("%s temp <= -20 \n",__func__);
		elvss_lowtemp_cmds_list.cmd_desc[1].payload[1] = 0x94;
		elvss_lowtemp_cmds_list.cmd_desc[2].payload[1] = 0x8C;
	}
	elvss_tempcompen_control.cmd_desc = elvss_lowtemp_cmds_list.cmd_desc;
	elvss_tempcompen_control.num_of_cmds = elvss_lowtemp_cmds_list.num_of_cmds;

	return elvss_tempcompen_control;
}
#endif

#if defined(TEMPERATURE_ELVSS)
// ELVSS TEMPERATURE COMPENSATION
static struct dsi_cmd get_elvss_tempcompen_control_set(void)
{
	struct dsi_cmd elvss_tempcompen_control = {0,};
	int cmd_idx = 0;

	/* Get the command desc */
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
	if(msd.dstat.temperature_value <= 0)
		elvss_tempcompen_cmds_list.cmd_desc[cmd_idx].payload[1] = msd.dstat.temperature_value + 0x80;
	else
		elvss_tempcompen_cmds_list.cmd_desc[cmd_idx].payload[1] = msd.dstat.temperature_value;
#else
	elvss_tempcompen_cmds_list.cmd_desc[cmd_idx].payload[6] = msd.dstat.temperature_value;
#endif
	elvss_tempcompen_control.cmd_desc = &(elvss_tempcompen_cmds_list.cmd_desc[cmd_idx]);
	elvss_tempcompen_control.num_of_cmds = 1;

	return elvss_tempcompen_control;
}

#endif
/*
	This function takes acl_map_table and uses cd_idx,
	to get the index of the command in elvss command list.

*/
#ifdef SMART_ACL
static struct dsi_cmd get_elvss_control_set(int cd_idx)
{
	struct dsi_cmd elvss_control = {0,};
	int cmd_idx = 0;
	char *payload;

	pr_info("%s for SMART_ACL\n",__func__);

	if (!elvss_map_table.size || !(cd_idx < elvss_map_table.size) ||
			!smart_acl_elvss_map_table.size ||
			!(cd_idx < smart_acl_elvss_map_table.size)) {
		pr_err("%s failed mapping elvss table\n",__func__);
		goto end;
	}

	/* Get the command desc */
	if(msd.dstat.acl_on || msd.dstat.siop_status) {
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
	if ((msd.id3 == EVT2_EA8061V_REV_D) || (msd.id3 == EVT2_EA8061V_REV_E) )
		smart_acl_elvss_cmds_list.cmd_desc[cmd_idx].payload[1] = 0x4C;
#endif
		cmd_idx = smart_acl_elvss_map_table.cmd_idx[cd_idx];
		payload = smart_acl_elvss_cmds_list.cmd_desc[cmd_idx].payload;
#if defined(TEMPERATURE_ELVSS_S6E8AA4)
		elvss_lowtemp_cmds_list.cmd_desc[2].payload[2] = smart_acl_elvss_cmds_list.cmd_desc[cmd_idx].payload[2];
#endif
		elvss_control.cmd_desc = &(smart_acl_elvss_cmds_list.cmd_desc[cmd_idx]);
		pr_info("ELVSS for SMART_ACL cd_idx=%d, cmd_idx=%d\n", cd_idx, cmd_idx);
	} else {
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
	if ((msd.id3 == EVT2_EA8061V_REV_D) || (msd.id3 == EVT2_EA8061V_REV_E) )
		elvss_cmds_list.cmd_desc[cmd_idx].payload[1] = 0x5C;
#endif
		cmd_idx = elvss_map_table.cmd_idx[cd_idx];
		payload = elvss_cmds_list.cmd_desc[cmd_idx].payload;
#if defined(TEMPERATURE_ELVSS_S6E8AA4)
		elvss_lowtemp_cmds_list.cmd_desc[2].payload[2] = elvss_cmds_list.cmd_desc[cmd_idx].payload[2];
#endif
		elvss_control.cmd_desc = &(elvss_cmds_list.cmd_desc[cmd_idx]);
		pr_info("ELVSS for normal cd_idx=%d, cmd_idx=%d\n", cd_idx, cmd_idx);
	}

	/*HBM off : B6H(ELVSS SET) 21st para <- default set : Get value in mdss_dsi_panel_dimming_init() */
#if defined(HBM_RE) && defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
		if(msd.dstat.hbm_mode) {
			elvss_control.cmd_desc = hbm_hbm_off_elvss_cmds.cmd_desc;
			elvss_control.num_of_cmds = hbm_hbm_off_elvss_cmds.num_of_cmds;
		}
#endif

#if defined(TEMPERATURE_ELVSS)
#if !defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
	// ELVSS lOW TEMPERATURE
	if (msd.dstat.auto_brightness != 6) // if HBM is not set
	{
		if (msd.dstat.temperature <= -20)
		{
			elvss_control.cmd_desc = &(elvss_lowtemp_cmds_list.cmd_desc[cmd_idx]);
		}else {

		}
	}
#endif
#endif
	elvss_control.num_of_cmds = 1;
	msd.dstat.curr_elvss_idx = cmd_idx;

end:
	return elvss_control;

}

#else
static struct dsi_cmd get_elvss_control_set(int cd_idx)
{
	struct dsi_cmd elvss_control = {0,};
	int cmd_idx = 0, payload_size = 0;
	char *p_payload, *c_payload;
	int p_idx = msd.dstat.curr_elvss_idx;

	if (!elvss_map_table.size || !(cd_idx < elvss_map_table.size))
	{
		pr_err("%s invalid elvss mapping \n",__func__);
		goto end;
	}


	/* Get index in the acl command list*/
	cmd_idx = elvss_map_table.cmd_idx[cd_idx];

	c_payload = elvss_cmds_list.cmd_desc[cmd_idx].payload;

	/* Check if current & previous commands are same */
	if (p_idx >= 0) {
		p_payload = elvss_cmds_list.cmd_desc[p_idx].payload;
		payload_size = elvss_cmds_list.cmd_desc[p_idx].dchdr.dlen;

		if (msd.dstat.curr_elvss_idx == cmd_idx ||
			!memcmp(p_payload, c_payload, payload_size))
			goto end;
	}

	/* Get the command desc */
#if defined(TEMPERATURE_ELVSS)
	// ELVSS lOW TEMPERATURE
	if (msd.dstat.auto_brightness != 6) // will be changed to 7
	{
		if (msd.dstat.temperature <= -20)
		{
			elvss_control.cmd_desc = &(elvss_lowtemp_cmds_list.cmd_desc[cmd_idx]);
		}else {
#endif
	elvss_control.cmd_desc = &(elvss_cmds_list.cmd_desc[cmd_idx]);
#if defined(TEMPERATURE_ELVSS)
		}
	}
#endif
	elvss_control.num_of_cmds = 1;
	msd.dstat.curr_elvss_idx = cmd_idx;

	switch (msd.panel) {
	case PANEL_720P_AMOLED_S6E8AA3X01:
		/* Do any panel specfific customization here */
		break;
	}
end:
	return elvss_control;

}
#endif

static struct dsi_cmd get_gamma_control_set(int candella)
{
	struct dsi_cmd gamma_control = {0,};
	/* Just a safety check to ensure smart dimming data is initialised well */
	BUG_ON(msd.sdimconf->generate_gamma == NULL);
	msd.sdimconf->generate_gamma(candella, &gamma_cmds_list.cmd_desc[0].payload[1]);

	gamma_control.cmd_desc = &(gamma_cmds_list.cmd_desc[0]);
	gamma_control.num_of_cmds = gamma_cmds_list.num_of_cmds;
	return gamma_control;
}

static int update_bright_packet(int cmd_count, struct dsi_cmd *cmd_set)
{
	int i = 0;

	if (cmd_count > (MAX_BR_PACKET_SIZE - 1)) /*cmd_count is index, if cmd_count >12 then panic*/
		panic("over max brightness_packet size(%d).. !!", MAX_BR_PACKET_SIZE);

	for (i = 0; i < cmd_set->num_of_cmds; i++) {
		brightness_packet[cmd_count].payload = \
			cmd_set->cmd_desc[i].payload;
		brightness_packet[cmd_count].dchdr.dlen = \
			cmd_set->cmd_desc[i].dchdr.dlen;
		brightness_packet[cmd_count].dchdr.dtype = \
		cmd_set->cmd_desc[i].dchdr.dtype;
		brightness_packet[cmd_count].dchdr.wait = \
		cmd_set->cmd_desc[i].dchdr.wait;
		cmd_count++;
	}

	return cmd_count;
}

#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
static struct dsi_cmd get_testKey_set(int enable)/*F0 or F0 F1*/
{
	struct dsi_cmd testKey = {0,};

	if (enable)
		testKey.cmd_desc = &(test_key_enable_cmds.cmd_desc[0]);
	else
		testKey.cmd_desc = &(test_key_disable_cmds.cmd_desc[0]);

	testKey.num_of_cmds = test_key_disable_cmds.num_of_cmds;

	return testKey;
}
#endif

#if defined(HBM_RE) || defined(CONFIG_HBM_PSRE)
static struct dsi_cmd get_hbm_etc_control_set(void)
{
	struct dsi_cmd etc_hbm_control = {0,};

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL)
	/* Get the command desc */
	if (msd.id3 == EVT0_ID) {
		pr_info("%s for EVT0_FIRST\n",__func__);
		etc_hbm_control.cmd_desc = &(hbm_etc_cmds_list.cmd_desc[0]);
		etc_hbm_control.num_of_cmds = hbm_etc_cmds_list.num_of_cmds;
	} else if (msd.id3 == EVT0_SECOND_ID) {
		pr_info("%s for EVT0_SECOND\n",__func__);
		etc_hbm_control.cmd_desc = &(hbm_etc_cmds_evt0_second_list.cmd_desc[0]);
		etc_hbm_control.num_of_cmds = hbm_etc_cmds_evt0_second_list.num_of_cmds;
	} else if (msd.id3 == EVT1_ID) {
		pr_info("%s for EVT1_FIRST\n",__func__);
		etc_hbm_control.cmd_desc = &(hbm_etc_cmds_evt1_list.cmd_desc[0]);
		etc_hbm_control.num_of_cmds = hbm_etc_cmds_evt1_list.num_of_cmds;
	} else if (msd.id3 == EVT1_SECOND_ID) {
		pr_info("%s for EVT1_SECOND\n",__func__);
		etc_hbm_control.cmd_desc = &(hbm_etc_cmds_evt1_second_list.cmd_desc[0]);
		etc_hbm_control.num_of_cmds = hbm_etc_cmds_evt1_second_list.num_of_cmds;
	} else if (msd.id3 == EVT1_H_REV_I) {
		pr_info("%s for EVT1_revI\n",__func__);
		etc_hbm_control.cmd_desc = &(hbm_etc_cmds_evt1_H_revI_list.cmd_desc[0]);
		etc_hbm_control.num_of_cmds = hbm_etc_cmds_evt1_H_revI_list.num_of_cmds;
	} else if (msd.id3 == EVT1_H_REV_J) {
		pr_info("%s for EVT1_revI\n",__func__);
		etc_hbm_control.cmd_desc = &(hbm_etc_cmds_evt1_H_revJ_list.cmd_desc[0]);
		etc_hbm_control.num_of_cmds = hbm_etc_cmds_evt1_H_revJ_list.num_of_cmds;
	}
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_YOUM_CMD_FULL_HD_PT_PANEL)
	if ((msd.id3 == EVT0_F_REV_A) || (msd.id3 == EVT0_F_REV_E) ||
		(msd.id3 == EVT0_F_REV_F) || (msd.id3 == EVT2_F_REV_G)) {
		pr_info("%s for F\n",__func__);
		etc_hbm_control.cmd_desc = &(hbm_etc_cmds_evt0_second_list.cmd_desc[0]);
		etc_hbm_control.num_of_cmds = hbm_etc_cmds_evt0_second_list.num_of_cmds;
	}
#else
#if defined(TEMPERATURE_ELVSS_S6E8AA4)
		if (msd.dstat.temperature > 0) {
			pr_debug("%s temp > 0 \n",__func__);
			hbm_etc_cmds_list.cmd_desc[0].payload[1] = 0x88;
		} else {
			pr_debug("%s temp <= 0 \n",__func__);
			hbm_etc_cmds_list.cmd_desc[0].payload[1] = 0x8C;
		}
#endif
	etc_hbm_control.cmd_desc = &(hbm_etc_cmds_list.cmd_desc[0]);
	etc_hbm_control.num_of_cmds = hbm_etc_cmds_list.num_of_cmds;
#endif

	return etc_hbm_control;
}

static struct dsi_cmd get_hbm_gamma_control_set(void)
{
	struct dsi_cmd gamma_hbm_control = {0,};

	/* Get the command desc */
	gamma_hbm_control.cmd_desc = &(hbm_gamma_cmds_list.cmd_desc[0]);
	gamma_hbm_control.num_of_cmds = hbm_gamma_cmds_list.num_of_cmds;

	return gamma_hbm_control;
}

static int make_brightcontrol_hbm_set(int bl_level)
{

	struct dsi_cmd hbm_etc_control = {0,};
	struct dsi_cmd gamma_control = {0,};
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	struct dsi_cmd testKey = {0,};
#endif

	int cmd_count = 0;

	if (msd.dstat.hbm_mode) {
		pr_err("%s : already hbm mode! return .. \n", __func__);
		return 0;
	}
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	testKey = get_testKey_set(1);
	cmd_count = update_bright_packet(cmd_count, &testKey);
#endif

	/*gamma*/
	gamma_control = get_hbm_gamma_control_set();
	cmd_count = update_bright_packet(cmd_count, &gamma_control);

	hbm_etc_control = get_hbm_etc_control_set();
	cmd_count = update_bright_packet(cmd_count, &hbm_etc_control);

#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	testKey = get_testKey_set(0);
	cmd_count = update_bright_packet(cmd_count, &testKey);
#endif
	/* for non hbm mode : reset */
	msd.dstat.curr_elvss_idx = -1;
	msd.dstat.curr_acl_idx = -1;
	msd.dstat.curr_aid_idx = -1;
	msd.dstat.curr_acl_cond = 0;

	LCD_DEBUG("bright_level: HBM %d\n", cmd_count);
	return cmd_count;

}

#if defined(FORCE_500CD)
static struct dsi_cmd get_force_500cd_control_set(void)
{
	struct dsi_cmd force_500cd_etc_control = {0,};

	force_500cd_etc_control.cmd_desc = &(force_500.cmd_desc[0]);
	force_500cd_etc_control.num_of_cmds = force_500.num_of_cmds;

	return force_500cd_etc_control;
}
static int make_force_500cd_set(int bl_level)
{

	struct dsi_cmd force500_etc_control = {0,};
	struct dsi_cmd gamma_control = {0,};

	int cmd_count = 0;

	if (msd.dstat.hbm_mode) {
		pr_err("%s : already hbm mode! return .. \n", __func__);
		return 0;
	}

	/*gamma*/
	gamma_control = get_hbm_gamma_control_set();
	cmd_count = update_bright_packet(cmd_count, &gamma_control);

	force500_etc_control = get_force_500cd_control_set();
	cmd_count = update_bright_packet(cmd_count, &force500_etc_control);

	/* for non hbm mode : reset */
	msd.dstat.curr_elvss_idx = -1;
	msd.dstat.curr_acl_idx = -1;
	msd.dstat.curr_aid_idx = -1;
	msd.dstat.curr_acl_cond = 0;

	LCD_DEBUG("bright_level: Force 500Cd %d\n", cmd_count);
	return cmd_count;

}
#endif
#endif
static int make_brightcontrol_set(int bl_level)
{

	struct dsi_cmd elvss_control = {0, 0, 0, 0, 0};
	struct dsi_cmd acl_control = {0, 0, 0, 0, 0};
	struct dsi_cmd aid_control = {0, 0, 0, 0, 0};
	struct dsi_cmd gamma_control = {0, 0, 0, 0, 0};
#if !defined(NOT_USING_ACL_CONT)
	struct dsi_cmd aclcont_control = {0, 0, 0, 0, 0};
#endif
#if defined(TEMPERATURE_ELVSS_S6E3FA0) || defined(TEMPERATURE_ELVSS_S6E8AA4)
	struct dsi_cmd temperature_elvss_control = {0, 0, 0, 0, 0};
#endif

#if defined(TEMPERATURE_ELVSS)
	struct dsi_cmd elvss_compen_control = {0, 0, 0, 0, 0};
#endif
	int cmd_count = 0, cd_idx = 0, cd_level =0;
	/* level2 enable */
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	struct dsi_cmd testKey = get_testKey_set(1);
	cmd_count = update_bright_packet(cmd_count, &testKey);
#endif

	cd_idx = get_cmd_idx(bl_level);
	cd_level = get_candela_value(bl_level);

	/* aid/aor */
	aid_control = get_aid_aor_control_set(cd_idx);
	cmd_count = update_bright_packet(cmd_count, &aid_control);

	/* acl */
	if (msd.dstat.acl_on||msd.dstat.siop_status) {
#if !defined(NOT_USING_ACL_CONT)
		aclcont_control = get_aclcont_control_set();
		cmd_count = update_bright_packet(cmd_count, &aclcont_control);
#endif
		acl_control = get_acl_control_set(cd_idx);
		cmd_count = update_bright_packet(cmd_count, &acl_control);
	} else {
		/* acl off (hbm off) */
		acl_control = get_acl_control_off_set();
		cmd_count = update_bright_packet(cmd_count, &acl_control);
	}

	/*elvss*/
	elvss_control = get_elvss_control_set(cd_idx);
	cmd_count = update_bright_packet(cmd_count, &elvss_control);

#if defined(TEMPERATURE_ELVSS_S6E3FA0) || defined(TEMPERATURE_ELVSS_S6E8AA4)
	// ELVSS TEMPERATURE COMPENSATION
	// ELVSS for Temperature set cmd should be sent after normal elvss set cmd
	temperature_elvss_control = get_elvss_tempcompen_control_set();
	cmd_count = update_bright_packet(cmd_count, &temperature_elvss_control);
#endif

#if defined(TEMPERATURE_ELVSS)
	// ELVSS TEMPERATURE COMPENSATION
	if (msd.dstat.temper_need_update) {
		elvss_compen_control = get_elvss_tempcompen_control_set();
		cmd_count = update_bright_packet(cmd_count, &elvss_compen_control);
		msd.dstat.temper_need_update = 0;
	}
#endif

	/*gamma*/

#if (defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL) \
		&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_LDI_EA8061))\
		|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
// kyNam_131228_
	if( get_oled_id() == 0)
	{
		int i,j;
		cmd_count = 0;

		/* If you cannot use AID-dimming, you can use temporary-gamma */
		for( i = 1; i<=30; i++ )
		{
			// kyNam_140117_ sample for Fresco_KOR
			/* calculator : 5<x<160 -> 92<gamma<113 */
			if( bl_level < 160 ) j = (113-92)*(bl_level-5)/(160-5) +92;
			/* calculator : 160<x<255 -> 113<gamma<130 */
			else j = (130-113)*(bl_level-160)/(255-160) +113;

			if( i==1 || i==3 || i==5 ) j = ((j*2)>>8);
			if( i==2 || i==4 || i==6 ) j*=2;

			gamma_cmds_list.cmd_desc[1].payload[i] = (j & 0xFF);
		}

		gamma_control.cmd_desc = &(gamma_cmds_list.cmd_desc[0]);
		gamma_control.num_of_cmds = gamma_cmds_list.num_of_cmds;
		cmd_count = update_bright_packet(cmd_count, &gamma_control);
	}
	else
	{
		gamma_control = get_gamma_control_set(cd_level);
		cmd_count = update_bright_packet(cmd_count, &gamma_control);
	}
#else
	gamma_control = get_gamma_control_set(cd_level);
	cmd_count = update_bright_packet(cmd_count, &gamma_control);
#endif
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	testKey = get_testKey_set(0);
	cmd_count = update_bright_packet(cmd_count, &testKey);
#endif
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL) || defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
	LCD_DEBUG("bright_level: %d, candela_idx: %d( %d cd ), "\
		"cmd_count(aid,acl,elvss,gamma)::(%d,%d,%d,%d)%d\n",
#elif defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
	|| defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	LCD_DEBUG("bright_level: %d, candela_idx: %d( %d cd ), "\
		"cmd_count(aid,acl,elvss,temperature,gamma)::(%d,%d,%d,%d)%d\n",
#elif defined(TEMPERATURE_ELVSS_S6E3FA0)
	LCD_DEBUG("bright_level: %d, candela_idx: %d( %d cd ), "\
		"cmd_count(aid,acl,acl_ctrl,elvss,temperature,gamma)::(%d,%d,%d,%d,%d,%d)%d\n",
#else
	LCD_DEBUG("bright_level: %d, candela_idx: %d( %d cd ), "\
		"cmd_count(aid,acl,acl_ctrl,elvss,temperature,gamma)::(%d,%d,%d,%d,%d)%d\n",
#endif
		msd.dstat.bright_level, cd_idx, cd_level,
		aid_control.num_of_cmds,
		acl_control.num_of_cmds,
#if !defined(NOT_USING_ACL_CONT)
		aclcont_control.num_of_cmds,
#endif
		elvss_control.num_of_cmds,
#if defined(TEMPERATURE_ELVSS_S6E3FA0)
		temperature_elvss_control.num_of_cmds,
#endif
		gamma_control.num_of_cmds,
		cmd_count);
	return cmd_count;

}

static ssize_t mipi_samsung_disp_acl_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct msm_fb_data_type *mfd = msd.mfd;
	int	acl_set;

	acl_set = msd.dstat.acl_on;
	if (sysfs_streq(buf, "1"))
		acl_set = true;
	else if (sysfs_streq(buf, "0"))
		acl_set = false;
	else
		pr_info("%s: Invalid argument!!", __func__);

	if (mfd->panel_power_on) {
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
		if (acl_set) {
			msd.dstat.acl_on = true;
		} else {
			msd.dstat.acl_on = false;
			msd.dstat.curr_acl_idx = -1;
		}

		pr_info("%s: acl on : acl %d", __func__, msd.dstat.acl_on);

		mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
#else
		if (acl_set && !(msd.dstat.acl_on||msd.dstat.siop_status)) {
			msd.dstat.acl_on = true;
			pr_info("%s: acl on  : acl %d, siop %d", __func__,
					msd.dstat.acl_on, msd.dstat.siop_status);
			mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);

		} else if (!acl_set && msd.dstat.acl_on && !msd.dstat.siop_status) {
			msd.dstat.acl_on = false;
			msd.dstat.curr_acl_idx = -1;
			pr_info("%s: acl off : acl %d, siop %d", __func__,
					msd.dstat.acl_on, msd.dstat.siop_status);
			if(msd.dstat.auto_brightness == 6)
				pr_info("%s: HBM mode No ACL off!!", __func__);
#ifdef CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL
			else
				mipi_samsung_disp_send_cmd(PANEL_ACL_OFF, true);
#endif
#ifdef SMART_ACL
			/* If SMART_ACL enabled, elvss table shoud be set again */
			mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
#endif

		} else {
			msd.dstat.acl_on = acl_set;
			pr_info("%s: skip but acl update!! acl %d, siop %d", __func__,
				msd.dstat.acl_on, msd.dstat.siop_status);
		}
#endif
	}else {
		pr_info("%s: panel is off state. updating state value.\n",
			__func__);
		msd.dstat.acl_on = acl_set;
	}

	return size;
}

#if defined(octa_manufacture_date)
static ssize_t manufacture_date_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	u16 year;
	u8 month;
	u8 day;

	pr_info("C8 41th : %02x\n", mdate_buffer[0]);
	pr_info("C8 42th : %02x\n", mdate_buffer[1]);

	year = ((mdate_buffer[0] & 0xF0)>>4) + 2011;
	month = mdate_buffer[0] & 0x0F;
	day = mdate_buffer[1] & 0x1F;

	sprintf(buf, "%d, %d, %d\n", year, month, day);

	return strlen(buf);
}
#endif

static ssize_t mipi_samsung_disp_siop_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n", msd.dstat.siop_status);
	pr_info("siop status: %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_disp_siop_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct msm_fb_data_type *mfd = msd.mfd;
	int siop_set;

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
	msd.dstat.siop_status = false;
	return size;
#endif

	siop_set = msd.dstat.siop_status;
	if (sysfs_streq(buf, "1"))
		siop_set = true;
	else if (sysfs_streq(buf, "0"))
		siop_set = false;
	else
		pr_info("%s: Invalid argument!!", __func__);

	if (mfd->panel_power_on) {
		if (siop_set && !(msd.dstat.acl_on||msd.dstat.siop_status)) {
			msd.dstat.siop_status = true;
			mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
			pr_info("%s: acl on  : acl %d, siop %d", __func__,
				msd.dstat.acl_on, msd.dstat.siop_status);

		} else if (!siop_set && !msd.dstat.acl_on && msd.dstat.siop_status) {
			mutex_lock(&msd.lock);
			msd.dstat.siop_status = false;
			msd.dstat.curr_acl_idx = -1;
			if(msd.dstat.auto_brightness == 6)
				pr_info("%s: HBM mode No ACL off!!", __func__);
#ifdef CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL
			else
				mipi_samsung_disp_send_cmd(PANEL_ACL_OFF, false);
#endif
#ifdef SMART_ACL
			/* If SMART_ACL enabled, elvss table shoud be set again */
			mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, false);
#endif
			mutex_unlock(&msd.lock);
			pr_info("%s: acl off : acl %d, siop %d", __func__,
				msd.dstat.acl_on, msd.dstat.siop_status);

		} else {
			msd.dstat.siop_status = siop_set;
			pr_info("%s: skip but siop update!! acl %d, siop %d", __func__,
				msd.dstat.acl_on, msd.dstat.siop_status);
		}
	}else {
	msd.dstat.siop_status = siop_set;
	pr_info("%s: panel is off state. updating state value.\n",
		__func__);
	}

	return size;
}

#ifdef LDI_FPS_CHANGE
static unsigned int current_ldi_fps=0;
static unsigned int current_ldi_fps_otp=0;
unsigned int current_change_ldi_fps=0;
static int mipi_samsung_read_nv_mem(struct mdss_panel_data *pdata, struct dsi_cmd *nv_read_cmds, char *buffer);

int ldi_fps(unsigned int input_fps)
{
	int dest_fps_delta;
	const int proper_fps = 60300;

	pr_info("%s: input_fps=%d\n", __func__, input_fps);

	if((msd.manufacture_id & 0xFF) <= 0x01) {
		pr_err("%s:LDI EVT0 Not Support. Skip!! \n",__func__);
		return 0;
	}

	if(input_fps < 57000 || input_fps > 63000) {
		pr_err("%s:Invalid input_fps : %d\n",__func__,input_fps);
		return 0;
	}

	if(msd.mfd->resume_state == MIPI_RESUME_STATE) {
		dest_fps_delta = (proper_fps - (int)input_fps)/200;
		if(dest_fps_delta == 0) {
			pr_info("%s::No FPS Delta, Skip!! \n",__func__);
			return 1;
		}
		dest_fps_delta *= -1;
		pr_info("%s:dest_fps_delta = %d \n", __func__, dest_fps_delta);
		pr_info("%s:current_ldi_fps_register_value=0x%x\n",__func__,current_ldi_fps);
		pr_info("%s:dest_ldi_fps_register_value=0x%x\n",__func__,current_ldi_fps+dest_fps_delta);
		current_ldi_fps = current_ldi_fps + dest_fps_delta;
		if(current_ldi_fps < 0x35 || current_ldi_fps > 0x55)
			panic("LDI FPS Check input_fps");
		write_ldi_fps_cmds.cmd_desc[1].payload[1] = current_ldi_fps;
		mipi_samsung_disp_send_cmd(PANEL_MTP_ENABLE, true);
		mipi_samsung_disp_send_cmd(PANEL_LDI_FPS_CHANGE, true);
		mipi_samsung_disp_send_cmd(PANEL_MTP_DISABLE, true);
	} else {
		pr_err("%s:Panel is off state!!\n",__func__);
		return 0;
	}

	return 1;
}
EXPORT_SYMBOL(ldi_fps);
#endif

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

static ssize_t mipi_samsung_backlight_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n",
					msd.dstat.bright_level );
	pr_info("backlight : %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_backlight_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int bl_level = atoi(buf);

	pr_info("%s : level (%d)\n",__func__,bl_level);

	msd.dstat.bright_level = bl_level;
	mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);

	return size;
}

static ssize_t mipi_samsung_auto_brightness_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n",
					msd.dstat.auto_brightness);
	pr_info("auto_brightness: %d\n", *buf);

	return rc;
}

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL)
static unsigned int mipi_samsung_manufacture_id(struct mdss_panel_data *pdata);
static int mdss_dsi_panel_dimming_init(struct mdss_panel_data *pdata);
#endif

static ssize_t mipi_samsung_auto_brightness_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	static int first_auto_br = 0;

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL)
	if (!msd.manufacture_id) {
		msd.manufacture_id = mipi_samsung_manufacture_id(msd.pdata);
		mdss_dsi_panel_dimming_init(msd.pdata);
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
	else if (sysfs_streq(buf, "6")) // HBM mode
		msd.dstat.auto_brightness = 6;
	else if (sysfs_streq(buf, "7"))
		msd.dstat.auto_brightness = 7;
	else
		pr_info("%s: Invalid argument!!", __func__);

	if (!first_auto_br) {
		pr_info("%s : skip first auto brightness store (%d) (%d)!!\n",
				__func__, msd.dstat.auto_brightness, msd.dstat.bright_level);
		first_auto_br++;
		return size;
	}

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
	if (msd.dstat.on == 1 && msd.mfd->resume_state == MIPI_RESUME_STATE) {
		pr_info("%s %d %d\n", __func__, msd.dstat.auto_brightness, msd.dstat.bright_level);
		mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
	} else {
		pr_info("%s : panel is off state!! on=%d, state=%d\n", __func__, msd.dstat.on, msd.mfd->resume_state);
	}
#elif defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
	if (msd.mfd->resume_state == MIPI_RESUME_STATE) {
		mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
#if defined(CONFIG_MDNIE_LITE_TUNING)
		mDNIe_Set_Mode(); // LOCAL CE tuning
#endif
		pr_info("%s %d %d\n", __func__, msd.dstat.auto_brightness, msd.dstat.bright_level);
	} else {
		pr_info("%s : panel is off state!!\n", __func__);
	}
#else
	if (msd.mfd->resume_state == MIPI_RESUME_STATE) {
		mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
		pr_info("%s %d %d\n", __func__, msd.dstat.auto_brightness, msd.dstat.bright_level);
	} else {
		pr_info("%s : panel is off state!!\n", __func__);
	}
#endif

	return size;
}

#if defined(TEMPERATURE_ELVSS) || defined(TEMPERATURE_ELVSS_S6E3FA0) || defined(TEMPERATURE_ELVSS_S6E8AA4)
static ssize_t mipi_samsung_temperature_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, 40,"-20, -19, 0, 1, 30, 40\n");

	pr_info("%s msd.mpd->temperature : %d msd.mpd->temperature_value : 0x%x", __func__,
				msd.dstat.temperature, msd.dstat.temperature_value);

	return rc;
}

static ssize_t mipi_samsung_temperature_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int temp;

	sscanf(buf, "%d" , &msd.dstat.temperature);

	temp = msd.dstat.temperature;

	if (temp > 0)
		msd.dstat.temperature_value = (char)temp;
	else {
		temp *= -1;
		msd.dstat.temperature_value = (char)temp;
		msd.dstat.temperature_value |=0x80;
	}

	msd.dstat.temper_need_update = 1;

	if(msd.mfd->resume_state == MIPI_RESUME_STATE) {
		mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
		pr_info("mipi_samsung_temperature_store %d\n", msd.dstat.bright_level);
		pr_info("%s msd.dstat.temperature : %d msd.dstat.temperature_value : 0x%x", __func__,
				msd.dstat.temperature, msd.dstat.temperature_value);
	} else {
		pr_info("%s: skip but temperature update!! temperature %d, temperature_value %d", __func__,
				msd.dstat.temperature, msd.dstat.temperature_value);
	}

	return size;
}
#endif
#if defined(PARTIAL_UPDATE)
static int partial_disp_range[2];

static ssize_t mipi_samsung_disp_partial_disp_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

		rc = snprintf((char *)buf, 40,"partial display range %d to %d \n", partial_disp_range[0], partial_disp_range[1]);

		pr_info("partial display range %d to %d \n", partial_disp_range[0], partial_disp_range[1]);

	return rc;
}

static ssize_t mipi_samsung_disp_partial_disp_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%d %d" , &partial_disp_range[0], &partial_disp_range[1]);
	pr_info("%s: partial_disp range[0] = 0x%x, range[1] = 0x%x\n", __func__, partial_disp_range[0], partial_disp_range[1]);


	if(partial_disp_range[0] > 1919 || partial_disp_range[1] > 1919)
	{
		pr_err("%s:Invalid Input\n",__func__);
		return -EINVAL;
	}
	partialdisp_on_cmd.cmd_desc[0].payload[1] = (partial_disp_range[0] >> 8) & 0xFF;/*select msb 1byte*/
	partialdisp_on_cmd.cmd_desc[0].payload[2] = partial_disp_range[0] & 0xFF;
	partialdisp_on_cmd.cmd_desc[0].payload[3] = (partial_disp_range[1] >> 8) & 0xFF;/*select msb 1byte*/
	partialdisp_on_cmd.cmd_desc[0].payload[4] = partial_disp_range[1] & 0xFF;

	if (msd.dstat.on) {
		if (partial_disp_range[0] || partial_disp_range[1])
			mipi_samsung_disp_send_cmd(PANEL_PARTIAL_ON, true);
		else
			mipi_samsung_disp_send_cmd(PANEL_PARTIAL_OFF, true);
	} else {
		pr_info("%s : LCD is off state\n", __func__);
		return -EINVAL;
	}

	pr_info("%s: partialdisp_on_cmd = 0x%x\n", __func__, partialdisp_on_cmd.cmd_desc[0].payload[1]);
	pr_info("%s: partialdisp_on_cmd = 0x%x\n", __func__, partialdisp_on_cmd.cmd_desc[0].payload[2]);
	pr_info("%s: partialdisp_on_cmd = 0x%x\n", __func__, partialdisp_on_cmd.cmd_desc[0].payload[3]);
	pr_info("%s: partialdisp_on_cmd = 0x%x\n", __func__, partialdisp_on_cmd.cmd_desc[0].payload[4]);

	return size;
}
#endif

#if defined(FORCE_500CD)
static ssize_t mipi_samsung_force_500cd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	pr_info("Node for make brightness as 500Cd \n");

	return 0;
}

static ssize_t mipi_samsung_force_500cd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	sscanf(buf, "%d " , &input);
	pr_info("%s: input = %d\n", __func__, input);


	if (msd.dstat.on) {
		if(input) {
			pr_info("Force 500Cd Enable\n");
			mipi_samsung_disp_send_cmd(PANEl_FORCE_500CD, true);
			pr_info("Finish to make 500Cd \n");
		} else {
			pr_info("Force 500Cd Disable\n");
			mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
			pr_info("Finish to Disable 500Cd \n");
		}
	} else {
		pr_info("%s : LCD is off state\n", __func__);
		return -EINVAL;
	}

	return size;
}
#endif

static struct lcd_ops mipi_samsung_disp_props = {
	.get_power = NULL,
	.set_power = NULL,
};


static DEVICE_ATTR(lcd_power, S_IRUGO | S_IWUSR,
			mipi_samsung_disp_get_power,
			mipi_samsung_disp_set_power);

static DEVICE_ATTR(lcd_type, S_IRUGO, mipi_samsung_disp_lcdtype_show, NULL);

static DEVICE_ATTR(window_type, S_IRUGO,
			mipi_samsung_disp_windowtype_show, NULL);


static DEVICE_ATTR(power_reduce, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_disp_acl_show,
			mipi_samsung_disp_acl_store);
static DEVICE_ATTR(auto_brightness, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_auto_brightness_show,
			mipi_samsung_auto_brightness_store);

static DEVICE_ATTR(backlight, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_backlight_show,
			mipi_samsung_backlight_store);

#if defined(TEMPERATURE_ELVSS) || defined( TEMPERATURE_ELVSS_S6E3FA0) || defined(TEMPERATURE_ELVSS_S6E8AA4)
static DEVICE_ATTR(temperature, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_temperature_show,
			mipi_samsung_temperature_store);
#endif

static DEVICE_ATTR(siop_enable, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_disp_siop_show,
			mipi_samsung_disp_siop_store);

#if defined(PARTIAL_UPDATE)
static DEVICE_ATTR(partial_disp, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_disp_partial_disp_show,
			mipi_samsung_disp_partial_disp_store);
#endif

#if defined(octa_manufacture_date)
static DEVICE_ATTR(manufacture_date, S_IRUGO,
			manufacture_date_show, NULL);
#endif

#if defined(FORCE_500CD)
static DEVICE_ATTR(force_500cd, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_force_500cd_show,
			mipi_samsung_force_500cd_store);

#endif

#endif

#if !defined(CONFIG_FB_MSM_EDP_SAMSUNG)
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
#endif

void mdss_dsi_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *cmds, int cnt,int flag)
{
	struct dcs_cmd_req cmdreq;
	if (get_lcd_attached() == 0)
	{
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return;
	}

	memset(&cmdreq, 0, sizeof(cmdreq));

	if (flag & CMD_REQ_SINGLE_TX) {
		cmdreq.flags = CMD_REQ_SINGLE_TX | CMD_CLK_CTRL | CMD_REQ_COMMIT;
	}else
		cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;

	cmdreq.cmds = cmds;
	cmdreq.cmds_cnt = cnt;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	/*
	 * This mutex is to sync up with dynamic FPS changes
	 * so that DSI lockups shall not happen
	 */
	BUG_ON(msd.ctrl_pdata == NULL);
	mutex_lock(&msd.ctrl_pdata->dfps_mutex);
	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
	mutex_unlock(&msd.ctrl_pdata->dfps_mutex);
}

u32 mdss_dsi_cmd_receive(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *cmd, int rlen)
{
    struct dcs_cmd_req cmdreq;
	char *buf;

	buf = kmalloc(sizeof(rlen), GFP_KERNEL);
    memset(&cmdreq, 0, sizeof(cmdreq));
    cmdreq.cmds = cmd;
    cmdreq.cmds_cnt = 1;
    cmdreq.flags = CMD_REQ_RX | CMD_REQ_COMMIT;
	cmdreq.rbuf = buf;
    cmdreq.rlen = rlen;
    cmdreq.cb = NULL; /* call back */
    /*
	 * This mutex is to sync up with dynamic FPS changes
	 * so that DSI lockups shall not happen
	 */
	BUG_ON(msd.ctrl_pdata == NULL);
	mutex_lock(&msd.ctrl_pdata->dfps_mutex);
    mdss_dsi_cmdlist_put(ctrl, &cmdreq);
    mutex_unlock(&msd.ctrl_pdata->dfps_mutex);
    /*
     * blocked here, untill call back called
     */
    kfree(buf);
    return ctrl->rx_buf.len;
}

static int samsung_nv_read(struct dsi_cmd_desc *desc, char *destBuffer,
		int srcLength, struct mdss_panel_data *pdata, int startoffset)
{
	int loop_limit = 0;
	/* first byte is size of Register */
	static char packet_size[] = { 0x07, 0 };
	static struct dsi_cmd_desc s6e8aa0_packet_size_cmd = {
		{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0, sizeof(packet_size)},
		packet_size };

	/* second byte is Read-position */
	static char reg_read_pos[] = { 0xB0, 0x00 };
	static struct dsi_cmd_desc s6e8aa0_read_pos_cmd = {
		{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(reg_read_pos)},
		reg_read_pos };

	int read_pos = startoffset;
	int read_count = 0;
	int show_cnt;
	int i, j;
	char show_buffer[256];
	int show_buffer_pos = 0;
	int read_size = 0;

	show_buffer_pos +=
		snprintf(show_buffer, 256, "read_reg : %X[%d] : ",
		desc[0].payload[0], srcLength);

	loop_limit = (srcLength + packet_size[0] - 1)
				/ packet_size[0];
	mdss_dsi_cmds_send(msd.ctrl_pdata, &(s6e8aa0_packet_size_cmd), 1, 0);
	show_cnt = 0;

	for (j = 0; j < loop_limit; j++) {
		reg_read_pos[1] = read_pos;
		read_size = ((srcLength - read_pos + startoffset) < packet_size[0]) ?
					(srcLength - read_pos + startoffset) : packet_size[0];

		mdss_dsi_cmds_send(msd.ctrl_pdata, &(s6e8aa0_read_pos_cmd), 1, 0);

		read_count = mdss_dsi_cmd_receive(msd.ctrl_pdata, desc, read_size);

		for (i = 0; i < read_count; i++, show_cnt++) {
			show_buffer_pos += snprintf(show_buffer +
						show_buffer_pos, 256, "%02x ",
						msd.ctrl_pdata->rx_buf.data[i]);
			if (destBuffer != NULL && show_cnt < srcLength) {
					destBuffer[show_cnt] =
					msd.ctrl_pdata->rx_buf.data[i];
			}
		}
		show_buffer_pos += snprintf(show_buffer +
				show_buffer_pos, 256, ".");
		read_pos += read_count;

		if (read_pos-startoffset >= srcLength)
			break;
	}

	pr_info("%s\n", show_buffer);
	return read_pos-startoffset;
}

static int mipi_samsung_read_nv_mem(struct mdss_panel_data *pdata, struct dsi_cmd *nv_read_cmds, char *buffer)
{
	int nv_size = 0;
	int nv_read_cnt = 0;
	int i = 0, j = 0;

#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL)
	j = 5; // do not repeat
#elif defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	||defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	j = 3;
#endif

#if !defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
	mipi_samsung_disp_send_cmd(PANEL_NV_MTP_READ_REGISTER_SET_CMDS, true);
	mipi_samsung_disp_send_cmd(PANEL_MTP_ENABLE, true);
#endif

	for (i = 0; i < nv_read_cmds->num_of_cmds; i++)
		nv_size += nv_read_cmds->read_size[i];
	pr_info("nv_size= %d, nv_read_cmds->num_of_cmds = %d", nv_size, nv_read_cmds->num_of_cmds);
	for (i = 0; i < nv_read_cmds->num_of_cmds; i++) {
		int count = 0;
		int read_size = nv_read_cmds->read_size[i];
		int read_startoffset = nv_read_cmds->read_startoffset[i];
		do {
			count = samsung_nv_read(&(nv_read_cmds->cmd_desc[i]),
					&buffer[nv_read_cnt], read_size, pdata, read_startoffset);
			if (j++ == 5)
				break;
		} while(buffer[0] != 0x0 && buffer[0] != 0x1);
		nv_read_cnt += count;
		if (count != read_size)
			pr_err("Error reading LCD NV data !!!!\n");
	}
#if !defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
	mipi_samsung_disp_send_cmd(PANEL_MTP_DISABLE, true);
#endif

	return nv_read_cnt;
}

#ifdef DEBUG_LDI_STATUS
int read_ldi_status(void)
{
	struct dsi_buf *rp, *tp;
	int i;

	if (!ldi_debug_cmds.num_of_cmds)
		return 1;

	if(!msd.dstat.on) {
		pr_err("%s can not read because of panel off \n", __func__);
		return 1;
	}

	tp = &dsi_panel_tx_buf;
	rp = &dsi_panel_rx_buf;

	mdss_dsi_cmd_receive(msd.ctrl_pdata,
		&ldi_debug_cmds.cmd_desc[0],
		ldi_debug_cmds.read_size[0]);

	pr_info("%s: LDI 0Ah Register Value = 0x%x (Normal Case:0x9C)\n", __func__, *msd.ctrl_pdata->rx_buf.data);

	mdss_dsi_cmd_receive(msd.ctrl_pdata,
		&ldi_debug_cmds.cmd_desc[1],
		ldi_debug_cmds.read_size[1]);

	pr_info("%s: LDI 0Eh Register Value  = 0x%x (Normal Case:0x80)\n", __func__, *msd.ctrl_pdata->rx_buf.data);

	mdss_dsi_cmd_receive(msd.ctrl_pdata,
		&ldi_debug_cmds.cmd_desc[2],
		ldi_debug_cmds.read_size[2]);
	for(i=0 ; i<8 ; i++) {
		pr_info("%s: LDI EAh Register Value[%d]  = 0x%x \n", __func__,i, msd.ctrl_pdata->rx_buf.data[i]);
	}


	return 0;

}
EXPORT_SYMBOL(read_ldi_status);
#endif

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_YOUM_CMD_FULL_HD_PT_PANEL)
int reading_id = 0;
#endif

#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_LDI_EA8061)
static int mipi_magna_ea8061_read(struct dsi_cmd_desc *cmd, int rlen)
{
	magna_read_pos_cmds.cmd_desc[0].payload[1] = cmd->payload[0];
	mdss_dsi_cmds_send(msd.ctrl_pdata, magna_read_pos_cmds.cmd_desc, 1, 0);
	return mdss_dsi_cmd_receive(msd.ctrl_pdata, &magna_read_run_cmds.cmd_desc[0], rlen);
}

static unsigned int mipi_samsung_manufacture_id(struct mdss_panel_data *pdata)
{
	struct dsi_buf *rp, *tp;

	unsigned int id = 0 ;

	return 0xFF1043; // kyNam_140116_ after Read in kernel, LCD is Over-brightness.

	if (get_lcd_attached() == 0)
	{
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return id;
	}

	if (!manufacture_id_cmds.num_of_cmds)
		return 0;

	tp = &dsi_panel_tx_buf;
	rp = &dsi_panel_rx_buf;

	mipi_magna_ea8061_read( &manufacture_id_cmds.cmd_desc[0], manufacture_id_cmds.read_size[0] );
	id = *((unsigned int *)msd.ctrl_pdata->rx_buf.data);
	gv_manufacture_id = id;

	pr_info("%s: manufacture_id=%x\n", __func__, id);
	return id;
}

#else

static unsigned int mipi_samsung_manufacture_id(struct mdss_panel_data *pdata)
{
	struct dsi_buf *rp, *tp;

	unsigned int id = 0 ;

	if (get_lcd_attached() == 0)
	{
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return id;
	}

	if (!manufacture_id_cmds.num_of_cmds)
		return 0;

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_YOUM_CMD_FULL_HD_PT_PANEL)
	reading_id = 1;
#endif
	tp = &dsi_panel_tx_buf;
	rp = &dsi_panel_rx_buf;

	mdss_dsi_cmd_receive(msd.ctrl_pdata,
		&manufacture_id_cmds.cmd_desc[0],
		manufacture_id_cmds.read_size[0]);

	pr_info("%s: manufacture_id1=%x\n", __func__, *msd.ctrl_pdata->rx_buf.data);

	id = (*((unsigned int *)msd.ctrl_pdata->rx_buf.data) & 0xFF);
	id <<= 8;

	mdss_dsi_cmd_receive(msd.ctrl_pdata,
		&manufacture_id_cmds.cmd_desc[1],
		manufacture_id_cmds.read_size[1]);
	pr_info("%s: manufacture_id2=%x\n", __func__, *msd.ctrl_pdata->rx_buf.data);
	id |= (*((unsigned int *)msd.ctrl_pdata->rx_buf.data) & 0xFF);
	id <<= 8;

	mdss_dsi_cmd_receive(msd.ctrl_pdata,
		&manufacture_id_cmds.cmd_desc[2],
		manufacture_id_cmds.read_size[2]);
	pr_info("%s: manufacture_id3=%x\n", __func__, *msd.ctrl_pdata->rx_buf.data);
	id |= (*((unsigned int *)msd.ctrl_pdata->rx_buf.data) & 0xFF);
	gv_manufacture_id = id;

	pr_info("%s: manufacture_id=%x\n", __func__, id);
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_YOUM_CMD_FULL_HD_PT_PANEL)
	reading_id = 0;
#endif

#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
	if(id != lcd_id)
	{
		pr_err("%s : manufacture id %x->%x\n", __func__, id, lcd_id);
		id = lcd_id;
	}
#endif
	if (set_panel_rev(id) < 0)
		pr_err("%s : can't find panel id.. \n", __func__);

	return id;
}
#endif
static void mdss_dsi_panel_bl_ctrl(struct mdss_panel_data *pdata,
							u32 bl_level)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

#if !defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL) \
	&& !defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL)\
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	mipi_samsung_disp_send_cmd(PANEL_MTP_ENABLE, true);
#endif

	/*Dont need to send backlight command if display off*/
	if (msd.mfd->resume_state != MIPI_RESUME_STATE)
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
	{
		msd.dstat.bright_level = bl_level;
		return;
	}
#else
		return;
#endif

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	if (!ctrl_pdata) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	switch (ctrl_pdata->bklt_ctrl) {
		case BL_WLED:
			led_trigger_event(bl_led_trigger, bl_level);
			break;
		case BL_PWM:
			mdss_dsi_panel_bklt_pwm(ctrl_pdata, bl_level);
			break;
		case BL_DCS_CMD:
			msd.dstat.bright_level = bl_level;
			mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
			break;
		default:
			pr_err("%s: Unknown bl_ctrl configuration\n",
				__func__);
			break;
	}

#if !defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL) \
	&& !defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL)\
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	mipi_samsung_disp_send_cmd(PANEL_MTP_DISABLE, true);
#endif
}

static int mipi_samsung_disp_send_cmd(
		enum mipi_samsung_cmd_list cmd,
		unsigned char lock)
{
/* fix build error temporary */
/*	struct msm_fb_data_type *mfd = msd.mfd;*/
	struct dsi_cmd_desc *cmd_desc;
	int cmd_size = 0;
	int flag = 0;
#ifdef CMD_DEBUG
	int i,j;
#endif

	if (get_lcd_attached() == 0)
	{
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return -ENODEV;
	}

	if (lock)
		mutex_lock(&msd.lock);

	switch (cmd) {
		case PANEL_NV_MTP_READ_REGISTER_SET_CMDS:
			cmd_desc = nv_mtp_read_register_set_cmds.cmd_desc;
			cmd_size = nv_mtp_read_register_set_cmds.num_of_cmds;
			break;
		case PANEL_READY_TO_ON:
			cmd_desc = display_qcom_on_cmds.cmd_desc;
			cmd_size = display_qcom_on_cmds.num_of_cmds;
			break;
		case PANEL_DISP_OFF:
			cmd_desc = display_qcom_off_cmds.cmd_desc;
			cmd_size = display_qcom_off_cmds.num_of_cmds;
			break;
		case PANEL_DISPLAY_ON:
			cmd_desc = display_on_cmd.cmd_desc;
			cmd_size = display_on_cmd.num_of_cmds;
			break;
		case PANEL_DISPLAY_OFF:
			cmd_desc = display_off_cmd.cmd_desc;
			cmd_size = display_off_cmd.num_of_cmds;
			break;
		case PANEL_DISPLAY_UNBLANK:
			cmd_desc = display_unblank_cmd.cmd_desc;
			cmd_size = display_unblank_cmd.num_of_cmds;
			break;
		case PANEL_DISPLAY_BLANK:
			cmd_desc = display_blank_cmd.cmd_desc;
			cmd_size = display_blank_cmd.num_of_cmds;
			break;

		case PANEL_BRIGHT_CTRL:
			cmd_desc = brightness_packet;

			/* Single Tx use for DSI_VIDEO_MODE Only */
			if(msd.pdata->panel_info.mipi.mode == DSI_VIDEO_MODE)
				flag = CMD_REQ_SINGLE_TX;
			else
				flag = 0;

			msd.dstat.recent_bright_level = msd.dstat.bright_level;
#if defined(HBM_RE) || defined(CONFIG_HBM_PSRE)
			if(msd.dstat.auto_brightness >= 6 && msd.dstat.bright_level == 255) {
				cmd_size = make_brightcontrol_hbm_set(msd.dstat.bright_level);
				msd.dstat.hbm_mode = 1;
			} else {
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
				if(msd.dstat.hbm_mode)
					mdss_dsi_cmds_send(msd.ctrl_pdata, hbm_hbm_off_elvss_cmds.cmd_desc, hbm_hbm_off_elvss_cmds.num_of_cmds, flag);
#endif
				cmd_size = make_brightcontrol_set(msd.dstat.bright_level);
				msd.dstat.hbm_mode = 0;
			}
#else
			cmd_size = make_brightcontrol_set(msd.dstat.bright_level);
#endif
			if (msd.mfd->resume_state != MIPI_RESUME_STATE) {
				pr_info("%s : panel is off state!!\n", __func__);
				goto unknown_command;
			}
			udelay(300);
			break;
		case PANEL_MTP_ENABLE:
			cmd_desc = nv_enable_cmds.cmd_desc;
			cmd_size = nv_enable_cmds.num_of_cmds;
			break;
		case PANEL_MTP_DISABLE:
			cmd_desc = nv_disable_cmds.cmd_desc;
			cmd_size = nv_disable_cmds.num_of_cmds;
			break;
		case PANEL_NEED_FLIP:
			/*
				May be required by Panel Like Fusion3
			*/
			break;
		case PANEL_ACL_ON:
			/*
				May be required by panel like D2,Commanche
			*/
			break;
		case PANEL_ACL_OFF:
			cmd_desc = acl_off_cmd.cmd_desc;
			cmd_size = acl_off_cmd.num_of_cmds;
			break;
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
		case PANEL_TOUCHSENSING_ON:
			cmd_desc = touchsensing_on_cmd.cmd_desc;
			cmd_size = touchsensing_on_cmd.num_of_cmds;
			break;
		case PANEL_TOUCHSENSING_OFF:
			cmd_desc = touchsensing_off_cmd.cmd_desc;
			cmd_size = touchsensing_off_cmd.num_of_cmds;
			break;
#endif
#ifdef LDI_FPS_CHANGE
		case PANEL_LDI_FPS_CHANGE:
			cmd_desc = write_ldi_fps_cmds.cmd_desc;
			cmd_size = write_ldi_fps_cmds.num_of_cmds;
			break;
#endif
#ifdef LDI_ADJ_VDDM_OFFSET
		case PANEL_LDI_SET_VDDM_OFFSET:
			cmd_desc = write_vddm_offset_cmds.cmd_desc;
			cmd_size = write_vddm_offset_cmds.num_of_cmds;
			break;
#endif
#if defined(PARTIAL_UPDATE)
		case PANEL_PARTIAL_ON:
			cmd_desc = partialdisp_on_cmd.cmd_desc;
			cmd_size = partialdisp_on_cmd.num_of_cmds;
			break;

		case PANEL_PARTIAL_OFF:
			cmd_desc = partialdisp_off_cmd.cmd_desc;
			cmd_size = partialdisp_off_cmd.num_of_cmds;
			break;
#endif

#if defined(FORCE_500CD)
		case PANEl_FORCE_500CD:
			cmd_desc = brightness_packet;
			msd.dstat.recent_bright_level = msd.dstat.bright_level;
			cmd_size = make_force_500cd_set(msd.dstat.bright_level);
			break;
#endif
#if defined(CONFIG_LCD_CRACK_RECOVERY)
		case PANEL_CRACK_RECOVERY:
			cmd_desc = lcd_crack_rec_cmd_list.cmd_desc;
			cmd_size = lcd_crack_rec_cmd_list.num_of_cmds;
#endif
		default:
			pr_err("%s : unknown_command.. \n", __func__);
			goto unknown_command;
			;
	}

	if (!cmd_size) {
		pr_err("%s : cmd_size is zero!.. \n", __func__);
		goto unknown_command;
	}

#ifdef CMD_DEBUG
	for (i = 0; i < cmd_size; i++)
	{
		for (j = 0; j < cmd_desc[i].dchdr.dlen; j++)
			printk("%x ",cmd_desc[i].payload[j]);
		printk("\n");
	}
#endif


#ifdef MDP_RECOVERY
	if (!mdss_recovery_start)
		mdss_dsi_cmds_send(msd.ctrl_pdata, cmd_desc, cmd_size, flag);
	else
		pr_err ("%s : Can't send command during mdss_recovery_start\n", __func__);
#else
	mdss_dsi_cmds_send(msd.ctrl_pdata, cmd_desc, cmd_size, flag);
#endif

	if (lock)
		mutex_unlock(&msd.lock);

	return 0;

unknown_command:
	LCD_DEBUG("Undefined command\n");

	if (lock)
		mutex_unlock(&msd.lock);

	return -EINVAL;
}

#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
void mdss_dsi_panel_touchsensing(int enable)
{
	if(!msd.dstat.on)
	{
		pr_err("%s: No panel on! %d\n", __func__, enable);
		return;
	}
#if defined(CONFIG_MACH_KS01SKT) || defined(CONFIG_MACH_KS01KTT)\
		|| defined(CONFIG_MACH_KS01LGT) || defined(CONFIG_MACH_JACTIVESKT)\
		|| defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_JS01LTESBM)
	if(enable)
		mipi_samsung_disp_send_cmd(PANEL_TOUCHSENSING_ON, true);
#else
	if(enable)
		mipi_samsung_disp_send_cmd(PANEL_TOUCHSENSING_ON, true);
	else
		mipi_samsung_disp_send_cmd(PANEL_TOUCHSENSING_OFF, true);
#endif
}
#endif
static int mdss_dsi_panel_registered(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	if (pdata == NULL) {
			pr_err("%s: Invalid input data\n", __func__);
			return -EINVAL;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	msd.mfd = (struct msm_fb_data_type *)registered_fb[0]->par;
	msd.pdata = pdata;
	msd.ctrl_pdata = ctrl_pdata;
#if defined(CONFIG_MDNIE_LITE_TUNING)
	pr_info("[%s] CONFIG_MDNIE_LITE_TUNING ok ! init class called!\n",
		__func__);
	mdnie_lite_tuning_init(&msd);
#endif
	/* Set the initial state to Suspend until it is switched on */
	msd.mfd->resume_state = MIPI_SUSPEND_STATE;
	pr_info("%s:%d, Panel registered succesfully\n", __func__, __LINE__);
	return 0;
}

static int mdss_dsi_panel_dimming_init(struct mdss_panel_data *pdata)
{

#if defined(CONFIG_MDNIE_LITE_TUNING)
	char temp[4];
	int	x, y;
#endif
#if defined(HBM_RE) || defined(CONFIG_HBM_PSRE)
	char hbm_buffer[21];
#endif
#ifdef LDI_FPS_CHANGE
	char ldi_fps_buffer;
#endif
#ifdef LDI_ADJ_VDDM_OFFSET
	unsigned int vddm_offset;
	char vol_ref_buffer;
#endif

	if (get_lcd_attached() == 0)
	{
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return 0;
	}

	/* If the ID is not read yet, then read it*/
	if (!msd.manufacture_id)
		msd.manufacture_id = mipi_samsung_manufacture_id(pdata);

	if (!msd.dstat.is_smart_dim_loaded) {
		switch (msd.panel) {
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
			case PANEL_WVGA_OCTA_S6E88A0:
				msd.sdimconf = smart_S6E88A0_get_conf();
				break;
#elif defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
	|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
			case PANEL_720P_OCTA_S6E8AA4_VIDEO:
				msd.sdimconf = smart_S6E8AA4_get_conf();
				break;
			case PANEL_720P_OCTA_EA8061_VIDEO:
#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
				msd.sdimconf = smart_S6E8AA4_get_conf();
				break;
#else
				msd.sdimconf = smart_S6E8FA0_get_conf();
				break;
#endif
#else
			case PANEL_1080P_OCTA_S6E8FA0:
			case PANEL_720P_OCTA_D53D6EA8061V:
			case PANEL_720P_OCTA_S6E8AA0:
			case PANEL_720P_OCTA_D53D6EA8061V_VIDEO:
			case PANEL_QHD_OCTA_S6E88A0_VIDEO:
			case PANEL_HD_OCTA_D53D6EA8061_VIDEO:
				msd.sdimconf = smart_S6E8FA0_get_conf();
				break;
			case PANEL_1080P_OCTA_S6E3FA0:
			case PANEL_1080P_OCTA_S6E3FA0_CMD:
			case PANEL_1080P_YOUM_S6E3FA1_CMD:
				msd.sdimconf = smart_S6E3FA0_get_conf();
				break;
#endif
		}

		/* Just a safety check to ensure smart dimming data is initialised well */
		BUG_ON(msd.sdimconf == NULL);
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
	mipi_samsung_disp_send_cmd(PANEL_MTP_ENABLE, true);
#endif
		/* Set the mtp read buffer pointer and read the NVM value*/
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_read_cmds, msd.sdimconf->mtp_buffer);

#if defined(octa_manufacture_date)
		mipi_samsung_read_nv_mem(pdata, &nv_date_read_cmds, mdate_buffer);
#endif

#if (defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL) \
		&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_LDI_EA8061))\
		|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
// kyNam_131228_
//#if defined(VIRTUAL_GAMMA) // kyNam_131228_
		if (get_oled_id() == 0)
		{
			memset( msd.sdimconf->mtp_buffer, 0, sizeof( struct MTP_OFFSET ) );
			msd.mfd->resume_state = MIPI_RESUME_STATE;
			return 0;
		}
//#endif
#endif

#ifdef LDI_FPS_CHANGE
		if(msd.id3 >= 0x21) {
			mipi_samsung_read_nv_mem(msd.pdata, &read_ldi_fps_cmds, &ldi_fps_buffer);
			current_ldi_fps_otp = (unsigned int)(ldi_fps_buffer & 0x7F);
			if(current_ldi_fps_otp < 0x35 || current_ldi_fps_otp > 0x55) {
				pr_err("%s:ldi_fps register otp value read fail = 0x%x\n", __func__, current_ldi_fps_otp);
				panic("LDI FPS Register OTP Value Read Fail");
			} else
				pr_info("%s:ldi_fps register otp value = 0x%x\n", __func__, current_ldi_fps_otp);
		} else
			pr_err("%s:LDI EVT0 Not Support\n",__func__);

#endif
#ifdef LDI_ADJ_VDDM_OFFSET
		mipi_samsung_read_nv_mem(msd.pdata, &read_vddm_ref_cmds, &vol_ref_buffer);
		vddm_offset=(unsigned int)(vol_ref_buffer & 0x7F);
		pr_info("%s:vddm_offset = %d , ldi_vddm_lut[%d][1] = %d \n", __func__, vddm_offset, vddm_offset, ldi_vddm_lut[vddm_offset][1]);
		write_vddm_offset_cmds.cmd_desc[3].payload[1] = ldi_vddm_lut[vddm_offset][1];
#endif

		/* Initialize smart dimming related	things here */

		/* lux_tab setting for 350cd */
		if (msd.panel_350cd) {
			msd.sdimconf->lux_tab = &candela_map_table_350.lux_tab[0];
			msd.sdimconf->lux_tabsize = candela_map_table_350.lux_tab_size;
		} else { /* lux_tab setting for 300cd */
			msd.sdimconf->lux_tab = &candela_map_table.lux_tab[0];
			msd.sdimconf->lux_tabsize = candela_map_table.lux_tab_size;
		}

		if (msd.id3 == EVT1_H_REV_I) {
			display_qcom_on_cmds.cmd_desc[4].payload[2] = 0x08;
			display_qcom_on_cmds.cmd_desc[4].payload[4] = 0x08;
		} else if (msd.id3 == EVT1_H_REV_J) {
			display_qcom_on_cmds.cmd_desc[4].payload[2] = 0x06;
			display_qcom_on_cmds.cmd_desc[4].payload[4] = 0x06;
		}

		msd.sdimconf->man_id = msd.manufacture_id;
		/* Just a safety check to ensure smart dimming data is initialised well */
		BUG_ON(msd.sdimconf->init == NULL);
		msd.sdimconf->init();
		msd.dstat.is_smart_dim_loaded = true;

#if defined(HBM_RE)
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm_read_cmds, hbm_buffer);
		memcpy(&hbm_gamma_cmds_list.cmd_desc[1].payload[1], hbm_buffer, 21);
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm2_read_cmds, hbm_buffer);
		memcpy(&hbm_etc_cmds_list.cmd_desc[3].payload[18], hbm_buffer, 1);
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm3_read_cmds, hbm_buffer);
		memcpy(&hbm_hbm_off_elvss_cmds.cmd_desc[5].payload[18], hbm_buffer, 1);
#else
		/* Read mtp (C8h 34th ~ 40th) for HBM */
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm_read_cmds, hbm_buffer);
		memcpy(&hbm_gamma_cmds_list.cmd_desc[0].payload[1], hbm_buffer, 6);

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL)
		/* octa panel Read C8h 40th -> write B6h 17th */
		if((msd.id3 == EVT0_SECOND_ID) && hbm_etc_cmds_evt0_second_list.cmd_desc)
			memcpy(&hbm_etc_cmds_evt0_second_list.cmd_desc[2].payload[17], hbm_buffer+6, 1);
		if((msd.id3 == EVT1_ID) && hbm_etc_cmds_evt1_list.cmd_desc)
			memcpy(&hbm_etc_cmds_evt1_list.cmd_desc[2].payload[17], hbm_buffer+6, 1);
		if((msd.id3 == EVT1_SECOND_ID) && hbm_etc_cmds_evt1_second_list.cmd_desc)
			memcpy(&hbm_etc_cmds_evt1_second_list.cmd_desc[1].payload[17], hbm_buffer+6, 1);
		if((msd.id3 == EVT1_H_REV_I) && hbm_etc_cmds_evt1_H_revI_list.cmd_desc)
			memcpy(&hbm_etc_cmds_evt1_H_revI_list.cmd_desc[1].payload[17], hbm_buffer+6, 1);
		if((msd.id3 == EVT1_H_REV_J) && hbm_etc_cmds_evt1_H_revJ_list.cmd_desc)
			memcpy(&hbm_etc_cmds_evt1_H_revJ_list.cmd_desc[1].payload[17], hbm_buffer+6, 1);
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_YOUM_CMD_FULL_HD_PT_PANEL)
		/* youm panel */
		if(((msd.id3 == EVT0_F_REV_A) || (msd.id3 == EVT0_F_REV_E) ||
			(msd.id3 == EVT0_F_REV_F) || (msd.id3 == EVT2_F_REV_G))
			&& hbm_etc_cmds_evt0_second_list.cmd_desc)
			memcpy(&hbm_etc_cmds_evt0_second_list.cmd_desc[2].payload[17], hbm_buffer+6, 1);
#elif defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
		memcpy(&hbm_etc_cmds_list.cmd_desc[0].payload[21], hbm_buffer+6, 1);

		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm3_read_cmds, hbm_buffer);
		memcpy(&hbm_hbm_off_elvss_cmds.cmd_desc[0].payload[21], hbm_buffer, 1);

		/* LSI panel EVT1_rev C :  set RVdd*/
		if((msd.id3 == EVT1_REV_D) && (get_oled_id()))
			display_qcom_on_cmds.cmd_desc[3].payload[3] = 0x00;
#elif defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
		memcpy(&hbm_etc_cmds_list.cmd_desc[4].payload[1], hbm_buffer+6, 1);
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm3_read_cmds, hbm_buffer);
		memcpy(&hbm_hbm_off_elvss_cmds.cmd_desc[2].payload[1], hbm_buffer, 1);
#endif
		/* for rev I panel */
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm2_read_cmds, hbm_buffer);
		memcpy(&hbm_gamma_cmds_list.cmd_desc[0].payload[7], hbm_buffer, 15);
#endif
#elif defined(CONFIG_HBM_PSRE)
		/* Read mtp (B5h 13th ~ 18th) for HBM Write Gamma ( CAh 1st ~ 6th )*/
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm_read_cmds, hbm_buffer);
		memcpy(&hbm_gamma_cmds_list.cmd_desc[0].payload[1], hbm_buffer, 6);
		/* Read mtp (B5h 26th ~ 28th) for HBM Write Gamma ( CAh 7th ~ 9th )*/
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm2_read_cmds, hbm_buffer);
		memcpy(&hbm_gamma_cmds_list.cmd_desc[0].payload[7], hbm_buffer, 3);
		/* Read mtp (B6h 3rd ~ 14th) for HBM Write Gamma ( CAh 10th ~ 21th )*/
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm3_read_cmds, hbm_buffer);
		memcpy(&hbm_gamma_cmds_list.cmd_desc[0].payload[10], hbm_buffer, 12);
		/* Read mtp (B5h 19th) for HBM ELVSS OFFSET */
		mipi_samsung_read_nv_mem(pdata, &nv_hbm_elvss_offset_cmds, hbm_buffer);
		memcpy(&hbm_etc_cmds_list.cmd_desc[2].payload[1], hbm_buffer, 1);
#if defined(CMD_DEBUG)
{
		int i,j;

		for (i = 0; i < hbm_gamma_cmds_list.num_of_cmds; i++)
		{
			printk("[HBM] hbm_gamma_cmds_list : ");

			for (j = 0; j < hbm_gamma_cmds_list.cmd_desc[i].dchdr.dlen; j++)
				printk("%02x ",hbm_gamma_cmds_list.cmd_desc[i].payload[j]);
			printk("\n");
		}

		for (i = 0; i < hbm_etc_cmds_list.num_of_cmds; i++)
		{
			printk("[HBM] hbm_etc_cmds_list : ");

			for (j = 0; j < hbm_etc_cmds_list.cmd_desc[i].dchdr.dlen; j++)
				printk("%02x ",hbm_etc_cmds_list.cmd_desc[i].payload[j]);
			printk("\n");
		}
}
#endif
		/* Read mtp (B5h 24th ~ 25th) for Panel Production Day */
		mipi_samsung_read_nv_mem(pdata, &nv_production_day_cmds, hbm_buffer);
#endif

		/*
		 * Since dimming is loaded, we can assume that device is out of suspend state
		 * and can accept backlight commands.
		 */
#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
	if(!msd.dstat.recovery_boot_mode)
		msd.mfd->resume_state = MIPI_RESUME_STATE;
#else
		msd.mfd->resume_state = MIPI_RESUME_STATE;
#endif
	}

#if defined(CONFIG_MDNIE_LITE_TUNING)
	/* MDNIe tuning initialisation*/
	if (!msd.dstat.is_mdnie_loaded) {
		mipi_samsung_read_nv_mem(pdata, &nv_mdnie_read_cmds, temp);
		x =  temp[0] << 8 | temp[1];	/* X */
		y = temp[2] << 8 | temp[3];	/* Y */
		coordinate_tunning(x, y);
		msd.dstat.is_mdnie_loaded = true;
	}
#endif
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)
	mipi_samsung_disp_send_cmd(PANEL_MTP_DISABLE, true);
#endif
	return 0;
}


#if defined(CONFIG_MIPI_LCD_S6E3FA0_FORCE_VIDEO_MODE)
void force_dsi_video_mode(struct mdss_panel_common_pdata *panel_data){
	int i = 0;

	struct mipi_panel_info *mipi;

	mipi  = &pinfo->mipi;

	/* Configure Panel for VIDEO mode (F2 Command) */
	for(;i <  display_qcom_on_cmds.num_of_cmds; i++){
		if (display_qcom_on_cmds.cmd_desc[i].payload[0] == 0xF2) {
			display_qcom_on_cmds.cmd_desc[i].payload[1] = 0x02;
			break;
		}
	}
	pinfo->mipi.mode = DSI_VIDEO_MODE;
	mipi->dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	LCD_DEBUG("<<<<<<<<<<<VIDEO MODE Enforced for S6E8FA0>>>>>>>>\n");
}
#endif

#if defined(CONFIG_DUAL_LCD)
struct mdss_panel_data *mdss_dsi_switching = NULL;
#endif
#if defined(CONFIG_MDNIE_LITE_TUNING) \
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL) \
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL)
static bool dsi_first_init = true;
#endif

static int mdss_dsi_panel_on(struct mdss_panel_data *pdata)
{
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;

#if defined(CONFIG_DUAL_LCD)
	msd.lcd_panel_cmds = 1;
#endif

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
			panel_data);

	mipi  = &pdata->panel_info.mipi;

#if defined(CONFIG_DUAL_LCD)
{
	int ret;

	if(mdss_dsi_switching == NULL) {
		msd.dstat.bright_level = RECOVERY_BRIGHTNESS;
		mdss_dsi_switching = pdata;
	}

	LCD_DEBUG("lcd_sel [%s]\n", msd.dstat.lcd_sel ? "CLOSE" : "OPEN");

	ret = gpio_direction_output(msd.ctrl_pdata->lcd_sel_gpio, msd.dstat.lcd_sel);
	if (ret) {
		pr_err("%s : lcd_sel_gpio [%d]\n", __func__, msd.ctrl_pdata->lcd_sel_gpio);
	}
}
#endif

	pr_info("mdss_dsi_panel_on DSI_MODE = %d ++\n",mipi->mode);
	pr_debug("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);

	if (!msd.manufacture_id)
		msd.manufacture_id = mipi_samsung_manufacture_id(pdata);
	if (!msd.dstat.is_smart_dim_loaded)
		mdss_dsi_panel_dimming_init(pdata);

#ifdef LDI_FPS_CHANGE
	/* Restore current ldi_fps register value to OTP value because of ldi reset */
	current_ldi_fps = current_ldi_fps_otp;
	pr_info("%s:ldi_fps register value = 0x%x\n", __func__, current_ldi_fps);
#endif

	mipi_samsung_disp_send_cmd(PANEL_READY_TO_ON, true);

	/* Recovery Mode : Set some default brightness */
	if (msd.dstat.recovery_boot_mode) {
		msd.dstat.bright_level = RECOVERY_BRIGHTNESS;
		mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
	}

	/* Init Index Values */
	msd.dstat.curr_elvss_idx = -1;
	msd.dstat.curr_acl_idx = -1;
	msd.dstat.curr_aid_idx = -1;
	msd.dstat.hbm_mode = 0;
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
	msd.dstat.on = 1;
#endif
	msd.dstat.wait_disp_on = 1;
	msd.mfd->resume_state = MIPI_RESUME_STATE;
#ifdef LDI_ADJ_VDDM_OFFSET
	mipi_samsung_disp_send_cmd(PANEL_LDI_SET_VDDM_OFFSET, true);
#endif

#if defined(CONFIG_MDNIE_LITE_TUNING)
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL)
	is_negative_on();
#else
	if (!dsi_first_init)
		is_negative_on();
	else
		dsi_first_init = false;
#endif
#endif

#if defined(PARTIAL_UPDATE)
	if (partial_disp_range[0] || partial_disp_range[1])
		mipi_samsung_disp_send_cmd(PANEL_PARTIAL_ON, true);
#endif

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
	mipi_samsung_disp_send_cmd(PANEL_DISPLAY_ON, true);
#endif

	pr_info("mdss_dsi_panel_on--\n");

#if defined(CONFIG_DUAL_LCD)
	mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
#else
	msd.dstat.bright_level = msd.dstat.recent_bright_level;
	mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
#endif

#if defined(CONFIG_DUAL_LCD)
	msd.lcd_panel_cmds = 0;
#endif
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
	msd.dstat.on = 1;
#endif

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	if(err_fg_enable)
	{
		pr_info("[lcd] mdss_dsi_panel_on end %d\n", gpio_get_value(err_fg_gpio));
		enable_irq(gpio_to_irq(err_fg_gpio));
	}
#endif
#if defined(CONFIG_LCD_CRACK_RECOVERY)
	if(lcd_crack_rec_enable)
	{
		pr_info("[lcd] mdss_dsi_panel_on end %d\n", gpio_get_value(lcd_crack_gpio));
		enable_irq(gpio_to_irq(lcd_crack_gpio));
	}
#endif
	return 0;
}

static int mdss_dsi_panel_off(struct mdss_panel_data *pdata)
{
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;

#if defined(CONFIG_DUAL_LCD)
	LCD_DEBUG("lcd_sel [%s]\n", msd.dstat.lcd_sel ? "CLOSE" : "OPEN");
	msd.lcd_panel_cmds = 1;
#endif

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	pr_info("mdss_dsi_panel_off ++\n");
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	pr_debug("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);

	mipi  = &pdata->panel_info.mipi;

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	if (err_fg_enable && !err_fg_working && msd.dstat.on) {
		disable_irq_nosync(gpio_to_irq(err_fg_gpio));
		cancel_work_sync(&err_fg_work);
	}
#endif
#if defined(CONFIG_LCD_CRACK_RECOVERY)
	if (lcd_crack_rec_enable && !lcd_crack_rec_working && msd.dstat.on) {
		disable_irq_nosync(gpio_to_irq(lcd_crack_gpio));
		cancel_work_sync(&lcd_crack_rec_work);
	}
#endif
	msd.dstat.on = 0;
	msd.mfd->resume_state = MIPI_SUSPEND_STATE;
	ctrl->dsi_err_cnt = 0;

	mipi_samsung_disp_send_cmd(PANEL_DISP_OFF, true);

	pr_info("mdss_dsi_panel_off --\n");

#if defined(CONFIG_DUAL_LCD)
	msd.lcd_panel_cmds = 0;
#endif

	return 0;
}

#if defined(CONFIG_DUAL_LCD)
DEFINE_MUTEX(mdss_switching_mutex);

int samsung_switching_lcd(int flip)
{
	int ret = 0;
	int before_lcd_sel = 0;

	if(msd.lcd_panel_cmds) {
		LCD_DEBUG("Waiting for complete about panel_on or panel_off!\n");
		msleep(500);
	}

	before_lcd_sel = msd.dstat.lcd_sel;

	msd.dstat.lcd_sel=!flip; //Change LCD SEL

	if(mdss_dsi_switching == NULL){
		pr_err("%s: mdss_dsi_switching NULL!\n",__func__);
		return 0;
	}

	if (get_lcd_attached() == 0)
	{
		pr_err("%s: get_lcd_attached(0)!\n",__func__);
		return 0;
	}

	LCD_DEBUG("msd.dstat.on=%d, before=%s, after=%s +\n",
			msd.dstat.on,
			before_lcd_sel ? "CLOSE" : "OPEN",
			flip ? "OPEN" : "CLOSE"
			);

	mutex_lock(&mdss_switching_mutex);

	if(msd.dstat.on && (before_lcd_sel == flip)) {
		ret = mdss_dsi_panel_off(msd.pdata);
		if(ret)
			pr_err("%s: mdss_dsi_panel_off error\n",__func__);

		/* Init Index Values */
		msd.dstat.curr_elvss_idx = -1;
		msd.dstat.curr_acl_idx = -1;
		msd.dstat.curr_aid_idx = -1;
		msd.dstat.hbm_mode = 0;

		ret = mdss_dsi_panel_on(msd.pdata);
		if(ret)
			pr_err("%s: mdss_dsi_panel_on error\n",__func__);
	}

	mutex_unlock(&mdss_switching_mutex);

	LCD_DEBUG(" -\n");

	return ret;
}
EXPORT_SYMBOL(samsung_switching_lcd);
#endif

static int mdss_samsung_parse_candella_lux_mapping_table(struct device_node *np,
		struct candella_lux_map *table, char *keystring)
{
		const __be32 *data;
		int  data_offset, len = 0 , i = 0;
		int  cdmap_start=0, cdmap_end=0;

		data = of_get_property(np, keystring, &len);
		if (!data) {
			pr_err("%s:%d, Unable to read table %s ",
				__func__, __LINE__, keystring);
			return -EINVAL;
		}
		if ((len % 4) != 0) {
			pr_err("%s:%d, Incorrect table entries for %s",
						__func__, __LINE__, keystring);
			return -EINVAL;
		}
		table->lux_tab_size = len / (sizeof(int)*4);
		table->lux_tab = kzalloc((sizeof(int) * table->lux_tab_size), GFP_KERNEL);
		if (!table->lux_tab)
			return -ENOMEM;

		table->cmd_idx = kzalloc((sizeof(int) * table->lux_tab_size), GFP_KERNEL);
		if (!table->cmd_idx)
			goto error;

		data_offset = 0;
		for (i = 0 ; i < table->lux_tab_size; i++) {
			table->cmd_idx[i]= be32_to_cpup(&data[data_offset++]);	/* 1rst field => <idx> */
			cdmap_start = be32_to_cpup(&data[data_offset++]);		/* 2nd field => <from> */
			cdmap_end = be32_to_cpup(&data[data_offset++]);			/* 3rd field => <till> */
			table->lux_tab[i] = be32_to_cpup(&data[data_offset++]);	/* 4th field => <candella> */
			/* Fill the backlight level to lux mapping array */
			do{
				table->bkl[cdmap_start++] = i;
			}while(cdmap_start <= cdmap_end);
		}
		return 0;
error:
	kfree(table->lux_tab);

	return -ENOMEM;
}

static int mdss_samsung_parse_panel_table(struct device_node *np,
		struct cmd_map *table, char *keystring)
{
		const __be32 *data;
		int  data_offset, len = 0 , i = 0;

		data = of_get_property(np, keystring, &len);
		if (!data) {
			pr_err("%s:%d, Unable to read table %s ",
				__func__, __LINE__, keystring);
			return -EINVAL;
		}
		if ((len % 2) != 0) {
			pr_err("%s:%d, Incorrect table entries for %s",
						__func__, __LINE__, keystring);
			return -EINVAL;
		}
		table->size = len / (sizeof(int)*2);
		table->bl_level = kzalloc((sizeof(int) * table->size), GFP_KERNEL);
		if (!table->bl_level)
			return -ENOMEM;

		table->cmd_idx = kzalloc((sizeof(int) * table->size), GFP_KERNEL);
		if (!table->cmd_idx)
			goto error;

		data_offset = 0;
		for (i = 0 ; i < table->size; i++) {
			table->bl_level[i] = be32_to_cpup(&data[data_offset++]);
			table->cmd_idx[i] = be32_to_cpup(&data[data_offset++]);
		}

		return 0;
error:
	kfree(table->cmd_idx);

	return -ENOMEM;
}

static int mdss_samsung_parse_panel_cmd(struct device_node *np,
		struct dsi_cmd *commands, char *keystring)
{

		const char *data;
		int type, len = 0, i = 0;
		char *bp;
		struct dsi_ctrl_hdr *dchdr;
		int is_read = 0;

		data = of_get_property(np, keystring, &len);
		if (!data) {
			pr_err("%s:%d, Unable to read %s",
				__func__, __LINE__, keystring);
			return -ENOMEM;
		}

		commands->cmds_buff = kzalloc(sizeof(char) * len, GFP_KERNEL);
		if (!commands->cmds_buff)
			return -ENOMEM;

		memcpy(commands->cmds_buff, data, len);
		commands->cmds_len = len;

		/* scan dcs commands */
		bp = commands->cmds_buff;
		while (len > sizeof(*dchdr)) {
			dchdr = (struct dsi_ctrl_hdr *)bp;
			dchdr->dlen = ntohs(dchdr->dlen);

		if (dchdr->dlen >200)
			goto error2;

			bp += sizeof(*dchdr);
			len -= sizeof(*dchdr);
			bp += dchdr->dlen;
			len -= dchdr->dlen;
			commands->num_of_cmds++;

			type = dchdr->dtype;
			if (type == DTYPE_GEN_READ ||
				type == DTYPE_GEN_READ1 ||
				type == DTYPE_GEN_READ2 ||
				type == DTYPE_DCS_READ)	{
				/* Read command :last byte contain read size, read start */
				bp += 2;
				len -= 2;
				is_read = 1;
			}
		}

		if (len != 0) {
			pr_err("%s: dcs OFF command byte Error, len=%d", __func__, len);
			commands->cmds_len = 0;
			commands->num_of_cmds = 0;
			goto error2;
		}

		if (is_read) {
			/*
				Allocate an array which will store the number
				for bytes to read for each read command
			*/
			commands->read_size = kzalloc(sizeof(char) * \
					commands->num_of_cmds, GFP_KERNEL);
			if (!commands->read_size) {
				pr_err("%s:%d, Unable to read NV cmds",
					__func__, __LINE__);
				goto error2;
			}
			commands->read_startoffset = kzalloc(sizeof(char) * \
					commands->num_of_cmds, GFP_KERNEL);
			if (!commands->read_startoffset) {
				pr_err("%s:%d, Unable to read NV cmds",
					__func__, __LINE__);
				goto error1;
			}
		}

		commands->cmd_desc = kzalloc(commands->num_of_cmds
					* sizeof(struct dsi_cmd_desc),
						GFP_KERNEL);
		if (!commands->cmd_desc)
			goto error1;

		bp = commands->cmds_buff;
		len = commands->cmds_len;
		for (i = 0; i < commands->num_of_cmds; i++) {
			dchdr = (struct dsi_ctrl_hdr *)bp;
			len -= sizeof(*dchdr);
			bp += sizeof(*dchdr);
			commands->cmd_desc[i].dchdr = *dchdr;
			commands->cmd_desc[i].payload = bp;
			bp += dchdr->dlen;
			len -= dchdr->dlen;
			if (is_read)
			{
				commands->read_size[i] = *bp++;
				commands->read_startoffset[i] = *bp++;
				len -= 2;
			}
		}

		return 0;

error1:
	kfree(commands->read_size);
error2:
	kfree(commands->cmds_buff);

	return -EINVAL;

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
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL)
static void mdss_panel_parse_te_params(struct device_node *np,
				       struct mdss_panel_info *panel_info)
{

	u32 tmp;
	int rc = 0;
	/*
	 * TE default: dsi byte clock calculated base on 70 fps;
	 * around 14 ms to complete a kickoff cycle if te disabled;
	 * vclk_line base on 60 fps; write is faster than read;
	 * init == start == rdptr;
	 */
	panel_info->te.tear_check_en =
		!of_property_read_bool(np, "qcom,mdss-tear-check-disable");
	rc = of_property_read_u32
		(np, "qcom,mdss-tear-check-sync-cfg-height", &tmp);
	panel_info->te.sync_cfg_height = (!rc ? tmp : 0xfff0);
	rc = of_property_read_u32
		(np, "qcom,mdss-tear-check-sync-init-val", &tmp);
	panel_info->te.vsync_init_val = (!rc ? tmp : panel_info->yres);
	rc = of_property_read_u32
		(np, "qcom,mdss-tear-check-sync-threshold-start", &tmp);
	panel_info->te.sync_threshold_start = (!rc ? tmp : 4);
	rc = of_property_read_u32
		(np, "qcom,mdss-tear-check-sync-threshold-continue", &tmp);
	panel_info->te.sync_threshold_continue = (!rc ? tmp : 4);
	rc = of_property_read_u32(np, "qcom,mdss-tear-check-start-pos", &tmp);
	panel_info->te.start_pos = (!rc ? tmp : panel_info->yres);
	rc = of_property_read_u32
		(np, "qcom,mdss-tear-check-rd-ptr-trigger-intr", &tmp);
	panel_info->te.rd_ptr_irq = (!rc ? tmp : panel_info->yres + 1);
	rc = of_property_read_u32(np, "qcom,mdss-tear-check-frame-rate", &tmp);
	panel_info->te.refx100 = (!rc ? tmp : 6000);
}
#endif
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
			goto exit_free;
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
		goto exit_free;
	}

	pcmds->cmds = kzalloc(cnt * sizeof(struct dsi_cmd_desc),
						GFP_KERNEL);
	if (!pcmds->cmds)
		goto exit_free;

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

	/*Set default link state to LP Mode*/
	pcmds->link_state = DSI_LP_MODE;

	if (link_key) {
		data = of_get_property(np, link_key, NULL);
		if (data && !strcmp(data, "dsi_hs_mode"))
			pcmds->link_state = DSI_HS_MODE;
		else
			pcmds->link_state = DSI_LP_MODE;
	}

	pr_info("%s: dcs_cmd=%x len=%d, cmd_cnt=%d link_state=%d\n", __func__,
		pcmds->buf[0], pcmds->blen, pcmds->cmd_cnt, pcmds->link_state);

	return 0;

exit_free:
	kfree(buf);
	return -ENOMEM;
}

static int mdss_panel_parse_dt(struct device_node *np,
					struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	u32 res[6], tmp;
	u32 fbc_res[7];
	int rc, i, len;
	const char *data;
	static const char *bl_ctrl_type, *pdest;
	struct mdss_panel_info *pinfo = &(ctrl_pdata->panel_data.panel_info);
	bool fbc_enabled = false;

	rc = of_property_read_u32_array(np, "qcom,mdss-pan-res", res, 2);
	if (rc) {
		pr_err("%s:%d, panel resolution not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	pinfo->xres = (!rc ? res[0] : 640);
	pinfo->yres= (!rc ? res[1] : 480);


	rc = of_property_read_u32_array(np, "qcom,mdss-pan-size", res, 2);
	if (rc == 0) {
		pinfo->width = res[0];
		pinfo->height = res[1];
	}

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	err_fg_gpio = of_get_named_gpio(np, "qcom,esd-irq-gpio", 0);
	if (!gpio_is_valid(err_fg_gpio)) {
		pr_err("%s:%d, esd gpio not specified\n",
						__func__, __LINE__);
	} else {
		rc = gpio_request(err_fg_gpio, "esd_enable");
		if (rc) {
			pr_err("request esd gpio failed, rc=%d\n",
			       rc);
			gpio_free(err_fg_gpio);
			return -ENODEV;
		}
#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
		if(get_oled_id()) err_fg_enable = 1; // s.lsi only
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
		err_fg_enable = 1;
#endif
	}
#endif
#if defined(CONFIG_LCD_CRACK_RECOVERY)
	lcd_crack_gpio= of_get_named_gpio(np,"qcom,lcd-crack-det-gpio", 0);
		if (!gpio_is_valid(lcd_crack_gpio)) {
			pr_err("%s:%d, lcd_crack_gpio not specified\n",
				__func__, __LINE__);
		} else {
			 	rc = gpio_tlmm_config(GPIO_CFG(lcd_crack_gpio, 0,
					GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
					if (rc) {
						pr_err("request lcd_crack_gpio failed, rc=%d\n",rc);
				}
	}
#endif
	rc = of_property_read_u32_array(np, "qcom,mdss-pan-active-res", res, 2);
	if (rc == 0) {
		pinfo->lcdc.xres_pad =
			pinfo->xres - res[0];
		pinfo->lcdc.yres_pad =
			pinfo->yres - res[1];
	}

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
		pr_debug("%s: SUCCESS-> WLED TRIGGER register\n", __func__);

		pinfo->bklt_ctrl = BL_WLED;
	} else if (!strncmp(bl_ctrl_type, "bl_ctrl_pwm", 11)) {
		ctrl_pdata->bklt_ctrl = BL_PWM;

		rc = of_property_read_u32(np, "qcom,dsi-pwm-period", &tmp);
		if (rc) {
			pr_err("%s:%d, Error, dsi pwm_period\n",
					__func__, __LINE__);
			return -EINVAL;
		}
		ctrl_pdata->pwm_period = tmp;

		rc = of_property_read_u32(np, "qcom,dsi-lpg-channel", &tmp);
		if (rc) {
			pr_err("%s:%d, Error, dsi lpg channel\n",
					__func__, __LINE__);
			return -EINVAL;
		}
		ctrl_pdata->pwm_lpg_chan = tmp;

		tmp = of_get_named_gpio(np, "qcom,dsi-pwm-gpio", 0);
		ctrl_pdata->pwm_pmic_gpio =  tmp;
	} else if (!strncmp(bl_ctrl_type, "bl_ctrl_dcs_cmds", 12)) {
		pr_debug("%s: SUCCESS-> DCS CMD BACKLIGHT register\n",
			 __func__);
		ctrl_pdata->bklt_ctrl = BL_DCS_CMD;
	} else {
		pr_debug("%s: Unknown backlight control\n", __func__);
		ctrl_pdata->bklt_ctrl = UNKNOWN_CTRL;
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

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-traffic-mode", &tmp);
	pinfo->mipi.traffic_mode = (!rc ? tmp : DSI_NON_BURST_SYNCH_PULSE);

	rc = of_property_read_u32(np, "qcom,mdss-pan-insert-dcs-cmd", &tmp);
	pinfo->mipi.insert_dcs_cmd = (!rc ? tmp : 1);

	rc = of_property_read_u32(np, "qcom,mdss-pan-wr-mem-continue", &tmp);
	pinfo->mipi.wr_mem_continue = (!rc ? tmp : 0x3c);

	rc = of_property_read_u32(np, "qcom,mdss-pan-wr-mem-start", &tmp);
	pinfo->mipi.wr_mem_start = (!rc ? tmp : 0x2c);

#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL)
	rc = of_property_read_u32(np, "qcom,mdss-pan-te-sel", &tmp);
	pinfo->mipi.te_sel = (!rc ? tmp : 1);
#endif
	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-dst-format", &tmp);
	pinfo->mipi.dst_format = (!rc ? tmp : DSI_VIDEO_DST_FORMAT_RGB888);

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-vc", &tmp);
	pinfo->mipi.vc = (!rc ? tmp : 0);

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-rgb-swap", &tmp);
	pinfo->mipi.rgb_swap = (!rc ? tmp : DSI_RGB_SWAP_RGB);

	rc = of_property_read_u32(np, "qcom,mdss-force-clk-lane-hs", &tmp);
	pinfo->mipi.force_clk_lane_hs = (!rc ? tmp : 0);

	rc = of_property_read_u32(np, "samsung,mdss-early-lcd-on", &tmp);
			pinfo->early_lcd_on = (!rc ? tmp : 0);
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

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-tx-eot-append", &tmp);
	pinfo->mipi.tx_eot_append = (!rc ? tmp : 0);

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-mdp-tr", &tmp);
	pinfo->mipi.mdp_trigger =
			(!rc ? tmp : DSI_CMD_TRIGGER_SW);
	if (pinfo->mipi.mdp_trigger > 6) {
		pr_err("%s:%d, Invalid mdp trigger. Forcing to sw trigger",
						 __func__, __LINE__);
		pinfo->mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	}

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-dma-tr", &tmp);
	pinfo->mipi.dma_trigger = (!rc ? tmp : DSI_CMD_TRIGGER_SW);
	if (pinfo->mipi.dma_trigger > 6) {
		pr_err("%s:%d, Invalid dma trigger. Forcing to sw trigger",
						 __func__, __LINE__);
		pinfo->mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	}

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-frame-rate", &tmp);
	pinfo->mipi.frame_rate = (!rc ? tmp : 60);

	rc = of_property_read_u32(np, "qcom,mdss-pan-clk-rate", &tmp);
	pinfo->clk_rate = (!rc ? tmp : 0);

	data = of_get_property(np, "qcom,panel-phy-regulatorSettings", &len);
	if ((!data) || (len != 7)) {
		pr_err("%s:%d, Unable to read Phy regulator settings",
		       __func__, __LINE__);
		return -EINVAL;
	}
	for (i = 0; i < len; i++)
		phy_params.regulator[i] = data[i];

	data = of_get_property(np, "qcom,panel-phy-timingSettings", &len);
	if ((!data) || (len != 12)) {
		pr_err("%s:%d, Unable to read Phy timing settings",
		       __func__, __LINE__);
		return -EINVAL;
	}
	for (i = 0; i < len; i++)
		phy_params.timing[i] = data[i];

	data = of_get_property(np, "qcom,panel-phy-strengthCtrl", &len);
	if ((!data) || (len != 2)) {
		pr_err("%s:%d, Unable to read Phy Strength ctrl settings",
		       __func__, __LINE__);
		return -EINVAL;
	}
	phy_params.strength[0] = data[0];
	phy_params.strength[1] = data[1];

	data = of_get_property(np, "qcom,panel-phy-bistCtrl", &len);
	if ((!data) || (len != 6)) {
		pr_err("%s:%d, Unable to read Phy Bist Ctrl settings",
		       __func__, __LINE__);
		return -EINVAL;
	}
	for (i = 0; i < len; i++)
		phy_params.bistctrl[i] = data[i];

	data = of_get_property(np, "qcom,panel-phy-laneConfig", &len);
	if ((!data) || (len != 45)) {
		pr_err("%s:%d, Unable to read Phy lane configure settings",
		       __func__, __LINE__);
		return -EINVAL;
	}
	for (i = 0; i < len; i++)
		phy_params.lanecfg[i] = data[i];

	pinfo->mipi.dsi_phy_db = phy_params;

		fbc_enabled = of_property_read_bool(np,
			"qcom,fbc-enabled");
	if (fbc_enabled) {
		pr_debug("%s:%d FBC panel enabled.\n", __func__, __LINE__);
		pinfo->fbc.enabled = 1;

		rc = of_property_read_u32_array(np,
				"qcom,fbc-mode", fbc_res, 7);
		pinfo->fbc.target_bpp =
			(!rc ?	fbc_res[0] : pinfo->bpp);
		pinfo->fbc.comp_mode = (!rc ? fbc_res[1] : 0);
		pinfo->fbc.qerr_enable =
			(!rc ? fbc_res[2] : 0);
		pinfo->fbc.cd_bias = (!rc ? fbc_res[3] : 0);
		pinfo->fbc.pat_enable = (!rc ? fbc_res[4] : 0);
		pinfo->fbc.vlc_enable = (!rc ? fbc_res[5] : 0);
		pinfo->fbc.bflc_enable =
			(!rc ? fbc_res[6] : 0);

		rc = of_property_read_u32_array(np,
				"qcom,fbc-budget-ctl", fbc_res, 3);
		pinfo->fbc.line_x_budget =
			(!rc ? fbc_res[0] : 0);
		pinfo->fbc.block_x_budget =
			(!rc ? fbc_res[1] : 0);
		pinfo->fbc.block_budget =
			(!rc ? fbc_res[2] : 0);

		rc = of_property_read_u32_array(np,
				"qcom,fbc-lossy-mode", fbc_res, 4);
		pinfo->fbc.lossless_mode_thd =
			(!rc ? fbc_res[0] : 0);
		pinfo->fbc.lossy_mode_thd =
			(!rc ? fbc_res[1] : 0);
		pinfo->fbc.lossy_rgb_thd =
			(!rc ? fbc_res[2] : 0);
		pinfo->fbc.lossy_mode_idx =
			(!rc ? fbc_res[3] : 0);

	} else {
		pr_debug("%s:%d Panel does not support FBC.\n",
				__func__, __LINE__);
		pinfo->fbc.enabled = 0;
		pinfo->fbc.target_bpp =
			pinfo->bpp;
	}
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->on_cmds,
		"qcom,panel-on-cmds", "qcom,on-cmds-dsi-state");

	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->off_cmds,
		"qcom,panel-off-cmds", "qcom,off-cmds-dsi-state");


#if 1
	mdss_samsung_parse_panel_cmd(np, &display_qcom_on_cmds,
				"qcom,panel-on-cmds");

	mdss_samsung_parse_panel_cmd(np, &display_qcom_off_cmds,
				"qcom,panel-off-cmds");

#endif
#if defined(octa_manufacture_date)
	mdss_samsung_parse_panel_cmd(np, &nv_date_read_cmds,
				"samsung,panel-nv-mdate-read-cmds");
#endif
	mdss_samsung_parse_panel_cmd(np, &nv_mtp_read_cmds,
				"samsung,panel-nv-mtp-read-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_mtp_read_register_set_cmds,
				"samsung,panel-mtp-read-cmds");
#if defined(HBM_RE)
	mdss_samsung_parse_panel_cmd(np, &nv_mtp_hbm_read_cmds,
				"samsung,panel-nv-mtp-read-hbm-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_mtp_hbm2_read_cmds,
				"samsung,panel-nv-mtp-read-hbm2-cmds");
	mdss_samsung_parse_panel_cmd(np, &hbm_etc_cmds_list,
					"samsung,panel-etc-hbm-cmds");
#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)\
	|| defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	mdss_samsung_parse_panel_cmd(np, &nv_mtp_hbm3_read_cmds,
				"samsung,panel-nv-mtp-read-hbm3-cmds");
	mdss_samsung_parse_panel_cmd(np, &hbm_hbm_off_elvss_cmds,
					"samsung,panel-hbm-off-elvss-cmds");
#endif
	mdss_samsung_parse_panel_cmd(np, &hbm_etc_cmds_evt0_second_list,
					"samsung,panel-etc-hbm-evt0-second-cmds");
	mdss_samsung_parse_panel_cmd(np, &hbm_etc_cmds_evt1_list,
					"samsung,panel-etc-hbm-evt1-cmds");
	mdss_samsung_parse_panel_cmd(np, &hbm_etc_cmds_evt1_second_list,
					"samsung,panel-etc-hbm-evt1-second-cmds");
	mdss_samsung_parse_panel_cmd(np, &hbm_etc_cmds_evt1_H_revI_list,
					"samsung,panel-etc-hbm-evt1-revI-cmds");
	mdss_samsung_parse_panel_cmd(np, &hbm_etc_cmds_evt1_H_revJ_list,
					"samsung,panel-etc-hbm-evt1-revJ-cmds");
	mdss_samsung_parse_panel_cmd(np, &hbm_gamma_cmds_list,
					"samsung,panel-gamma-hbm-cmds-list");
#elif defined(CONFIG_HBM_PSRE)
	mdss_samsung_parse_panel_cmd(np, &nv_mtp_hbm_read_cmds,
				"samsung,panel-nv-mtp-read-hbm-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_mtp_hbm2_read_cmds,
				"samsung,panel-nv-mtp-read-hbm2-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_mtp_hbm3_read_cmds,
				"samsung,panel-nv-mtp-read-hbm3-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_hbm_elvss_offset_cmds,
				"samsung,panel-nv-hbm-elvss-offset-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_production_day_cmds,
				"samsung,panel-nv-production-day-cmds");
	mdss_samsung_parse_panel_cmd(np, &hbm_gamma_cmds_list,
					"samsung,panel-gamma-hbm-cmds-list");
	mdss_samsung_parse_panel_cmd(np, &hbm_etc_cmds_list,
					"samsung,panel-etc-hbm-cmds");
#endif
	mdss_samsung_parse_panel_cmd(np, &nv_mdnie_read_cmds,
					"samsung,panel-nv-mdnie-read-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_enable_cmds,
				"samsung,panel-nv-read-enable-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_disable_cmds,
				"samsung,panel-nv-read-disable-cmds");
	mdss_samsung_parse_panel_cmd(np, &manufacture_id_cmds,
				"samsung,panel-manufacture-id-read-cmds");
#ifdef DEBUG_LDI_STATUS
	mdss_samsung_parse_panel_cmd(np, &ldi_debug_cmds,
				"samsung,panel-ldi-debug-read-cmds");
#endif
#ifdef LDI_FPS_CHANGE
	mdss_samsung_parse_panel_cmd(np, &read_ldi_fps_cmds,
				"samsung,panel-ldi-fps-read-cmds");
	mdss_samsung_parse_panel_cmd(np, &write_ldi_fps_cmds,
				"samsung,panel-ldi-fps-write-cmds");
#endif
#ifdef LDI_ADJ_VDDM_OFFSET
	mdss_samsung_parse_panel_cmd(np, &read_vddm_ref_cmds,
				"samsung,panel-ldi-vddm-read-cmds");
	mdss_samsung_parse_panel_cmd(np, &write_vddm_offset_cmds,
				"samsung,panel-ldi-vddm-offset-write-cmds");
#endif
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)
	mdss_samsung_parse_panel_cmd(np, &magna_read_pos_cmds,
				"magna,panel-read-pos-cmds");
	mdss_samsung_parse_panel_cmd(np, &magna_read_run_cmds,
				"magna,panel-read-run-cmds");
#endif
	mdss_samsung_parse_panel_cmd(np, &display_on_cmd,
				"qcom,panel-display-on-cmds");
	mdss_samsung_parse_panel_cmd(np, &display_off_cmd,
				"qcom,panel-display-off-cmds");
	mdss_samsung_parse_panel_cmd(np, &display_unblank_cmd,
				"qcom,panel-display-unblank-cmds");
	mdss_samsung_parse_panel_cmd(np, &display_blank_cmd,
				"qcom,panel-display-blank-cmds");
	mdss_samsung_parse_panel_cmd(np, &acl_off_cmd,
				"samsung,panel-acl-off-cmds");

	mdss_samsung_parse_panel_cmd(np, &acl_cmds_list,
				"samsung,panel-acl-cmds-list");
#if !defined(NOT_USING_ACL_CONT)
	mdss_samsung_parse_panel_cmd(np, &aclcont_cmds_list,
				"samsung,panel-aclcont-cmds-list");
#endif
#if defined(TEMPERATURE_ELVSS_S6E3FA0) || defined(TEMPERATURE_ELVSS_S6E8AA4)
	mdss_samsung_parse_panel_cmd(np, &elvss_lowtemp_cmds_list,
			"samsung,panel-elvss-lowtemp-cmds-list");
#endif
#if defined(TEMPERATURE_ELVSS)
	mdss_samsung_parse_panel_cmd(np, &elvss_lowtemp_cmds_list,
			"samsung,panel-elvss-lowtemp-cmds-list");
	mdss_samsung_parse_panel_cmd(np, &elvss_tempcompen_cmds_list,
			"samsung,panel-elvss-cmds-tempcompen-list");
#endif
	mdss_samsung_parse_panel_cmd(np, &gamma_cmds_list,
					"samsung,panel-gamma-cmds-list");
	mdss_samsung_parse_panel_cmd(np, &elvss_cmds_list,
				"samsung,panel-elvss-cmds-list");
	mdss_samsung_parse_panel_cmd(np, &aid_cmds_list,
				"samsung,panel-aid-cmds-list");
	mdss_samsung_parse_panel_cmd(np, &aid_cmds_list_350,
				"samsung,panel-aid-cmds-list-350");
#if defined(SMART_ACL)
	mdss_samsung_parse_panel_cmd(np, &smart_acl_elvss_cmds_list,
				"samsung,panel-smart-acl-elvss-cmds-list");
#endif
#if defined(CONFIG_LCD_CRACK_RECOVERY)
	mdss_samsung_parse_panel_cmd(np, &lcd_crack_rec_cmd_list,
				"samsung,panel-crack-rec-cmds-list");
#endif
	/* Process the mapping tables */
	mdss_samsung_parse_panel_table(np, &aid_map_table,
				"samsung,panel-aid-map-table");
	mdss_samsung_parse_panel_table(np, &acl_map_table,
				"samsung,panel-acl-map-table");
	mdss_samsung_parse_panel_table(np, &elvss_map_table,
				"samsung,panel-elvss-map-table");
#if defined(SMART_ACL)
	mdss_samsung_parse_panel_table(np, &smart_acl_elvss_map_table,
				"samsung,panel-smart-acl-elvss-map-table");
#endif
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_FULL_HD_PT_PANEL)
	 mdss_panel_parse_te_params(np, pinfo);
#endif
	/* Process the lux value table */
	mdss_samsung_parse_candella_lux_mapping_table(np, &candela_map_table,
				"samsung,panel-candella-mapping-table");
	mdss_samsung_parse_candella_lux_mapping_table(np, &candela_map_table_350,
				"samsung,panel-candella-mapping-table-350");

	/* Process touch sensing */
#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
	mdss_samsung_parse_panel_cmd(np, &touchsensing_on_cmd,
				"qcom,panel-touchsensing-on-cmds");
	mdss_samsung_parse_panel_cmd(np, &touchsensing_off_cmd,
				"qcom,panel-touchsensing-off-cmds");
#endif

#if defined(PARTIAL_UPDATE)
	mdss_samsung_parse_panel_cmd(np, &partialdisp_on_cmd,
				"samsung,panel-ldi-partial-disp-on");
	mdss_samsung_parse_panel_cmd(np, &partialdisp_off_cmd,
					"samsung,panel-ldi-partial-disp-off");
#endif
#if defined(FORCE_500CD)
	mdss_samsung_parse_panel_cmd(np, &force_500,
				"samsung,panel-force-500cd-cmds");
#endif
#if defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	mdss_samsung_parse_panel_cmd(np, &test_key_enable_cmds,
		"samsung,panel-test-key-enable-cmds");
	mdss_samsung_parse_panel_cmd(np, &test_key_disable_cmds,
		"samsung,panel-test-key-disable-cmds");
#endif
	return 0;

	return -EINVAL;
}

#if defined(CONFIG_HAS_EARLYSUSPEND)
static void mipi_samsung_disp_early_suspend(struct early_suspend *h)
{
	msd.mfd->resume_state = MIPI_SUSPEND_STATE;

	LCD_DEBUG("------");
}

static void mipi_samsung_disp_late_resume(struct early_suspend *h)
{

	msd.mfd->resume_state = MIPI_RESUME_STATE;

	LCD_DEBUG("------");
}
#endif

static int is_panel_supported(const char *panel_name)
{
	int i = 0;

	if (panel_name == NULL)
		return -EINVAL;

#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MIPI_S6E8AA0A_720P_PT_PANEL)\
		|| defined(CONFIG_FB_MSM_MDSS_MSM8X26)
	if(!of_machine_is_compatible("qcom,msm8226-mtp"))
		return -EINVAL;
#else
	if(!of_machine_is_compatible("qcom,msm8974-cdp"))
		return -EINVAL;
#endif

	while(panel_supp_cdp[i].name != NULL)	{
		if(!strcmp(panel_name,panel_supp_cdp[i].name))
			break;
		i++;
	}

	if( i < ARRAY_SIZE(panel_supp_cdp)) {
		memcpy(msd.panel_name, panel_name, MAX_PANEL_NAME_SIZE);
		msd.panel = panel_supp_cdp[i].panel_code;
		return 0;
	}
	return -EINVAL;
}

#ifdef DDI_VIDEO_ENHANCE_TUNING
#define MAX_FILE_NAME 128
#define TUNING_FILE_PATH "/sdcard/"
#define TUNE_FIRST_SIZE 5
#define TUNE_SECOND_SIZE 108
static char tuning_file[MAX_FILE_NAME];

static char mdni_tuning1[TUNE_FIRST_SIZE];
static char mdni_tuning2[TUNE_SECOND_SIZE];

static struct dsi_cmd_desc mdni_tune_cmd[] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(mdni_tuning2)}, mdni_tuning2},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(mdni_tuning1)}, mdni_tuning1},
};

static char char_to_dec(char data1, char data2)
{
	char dec;

	dec = 0;

	if (data1 >= 'a') {
		data1 -= 'a';
		data1 += 10;
	} else if (data1 >= 'A') {
		data1 -= 'A';
		data1 += 10;
	} else
		data1 -= '0';

	dec = data1 << 4;

	if (data2 >= 'a') {
		data2 -= 'a';
		data2 += 10;
	} else if (data2 >= 'A') {
		data2 -= 'A';
		data2 += 10;
	} else
		data2 -= '0';

	dec |= data2;

	return dec;
}
static void sending_tune_cmd(char *src, int len)
{
	int data_pos;
	int cmd_step;
	int cmd_pos;

	cmd_step = 0;
	cmd_pos = 0;

	for (data_pos = 0; data_pos < len;) {
		if (*(src + data_pos) == '0') {
			if (*(src + data_pos + 1) == 'x') {
				if (!cmd_step) {
					mdni_tuning1[cmd_pos] =
					char_to_dec(*(src + data_pos + 2),
							*(src + data_pos + 3));
				} else {
					mdni_tuning2[cmd_pos] =
					char_to_dec(*(src + data_pos + 2),
							*(src + data_pos + 3));
				}

				data_pos += 3;
				cmd_pos++;

				if (cmd_pos == TUNE_FIRST_SIZE && !cmd_step) {
					cmd_pos = 0;
					cmd_step = 1;
				}
			} else
				data_pos++;
		} else {
			data_pos++;
		}
	}

	printk(KERN_INFO "\n");
	for (data_pos = 0; data_pos < TUNE_FIRST_SIZE ; data_pos++)
		printk(KERN_INFO "0x%x ", mdni_tuning1[data_pos]);
	printk(KERN_INFO "\n");
	for (data_pos = 0; data_pos < TUNE_SECOND_SIZE ; data_pos++)
		printk(KERN_INFO"0x%x ", mdni_tuning2[data_pos]);
	printk(KERN_INFO "\n");



	mutex_lock(&msd.lock);


	mdss_dsi_cmds_send(msd.ctrl_pdata, mdni_tune_cmd, ARRAY_SIZE(mdni_tune_cmd), 0);

	mutex_unlock(&msd.lock);

}

static void load_tuning_file(char *filename)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret;
	mm_segment_t fs;

	pr_info("%s called loading file name : [%s]\n", __func__,
	       filename);

	fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR "%s File open failed\n", __func__);
		goto err;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	pr_info("%s Loading File Size : %ld(bytes)", __func__, l);

	dp = kmalloc(l + 10, GFP_KERNEL);
	if (dp == NULL) {
		pr_info("Can't not alloc memory for tuning file load\n");
		filp_close(filp, current->files);
		goto err;
	}
	pos = 0;
	memset(dp, 0, l);

	pr_info("%s before vfs_read()\n", __func__);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	pr_info("%s after vfs_read()\n", __func__);

	if (ret != l) {
		pr_info("vfs_read() filed ret : %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		goto err;
	}

	filp_close(filp, current->files);

	set_fs(fs);

	sending_tune_cmd(dp, l);

	kfree(dp);

	return;
err:
	set_fs(fs);
}

static ssize_t tuning_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	int ret = 0;

	ret = snprintf(buf, MAX_FILE_NAME, "Tunned File Name : %s\n",
								tuning_file);

	return ret;
}

static ssize_t tuning_store(struct device *dev,
			    struct device_attribute *attr, const char *buf,
			    size_t size)
{
	char *pt;
	memset(tuning_file, 0, sizeof(tuning_file));
	snprintf(tuning_file, MAX_FILE_NAME, "%s%s", TUNING_FILE_PATH, buf);

	pt = tuning_file;
	while (*pt) {
		if (*pt == '\r' || *pt == '\n') {
			*pt = 0;
			break;
		}
		pt++;
	}

	pr_info("%s:%s\n", __func__, tuning_file);

	load_tuning_file(tuning_file);

	return size;
}



static DEVICE_ATTR(tuning, 0664, tuning_show, tuning_store);
#endif
static int samsung_dsi_panel_event_handler(int event)
{
	pr_debug("SS DSI Event Handler");
	switch (event) {
		case MDSS_EVENT_FRAME_UPDATE:
			if(msd.dstat.wait_disp_on) {
				mipi_samsung_disp_send_cmd(PANEL_DISPLAY_ON, true);
				msd.dstat.wait_disp_on = 0;
			}
			break;
#if defined(CONFIG_MDNIE_LITE_TUNING) \
	&& !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_WVGA_S6E88A0_PT_PANEL)
		case MDSS_EVENT_MDNIE_DEFAULT_UPDATE:
			is_negative_on();
			break;
#endif
		default:
			pr_err("%s : unknown event \n", __func__);
			break;

	}

	return 0;
}
static int mdss_dsi_panel_blank(struct mdss_panel_data *pdata, int blank)
{
	if(blank) {
		pr_debug("%s:%d, blanking panel\n", __func__, __LINE__);
		mipi_samsung_disp_send_cmd(PANEL_DISPLAY_BLANK, false);
	}
	else {
		pr_debug("%s:%d, unblanking panel\n", __func__, __LINE__);
		mipi_samsung_disp_send_cmd(PANEL_DISPLAY_UNBLANK, false);
	}
	return 0;
}

#if defined(CONFIG_LCD_CLASS_DEVICE)
static struct attribute *panel_sysfs_attributes[] = {
	&dev_attr_lcd_power.attr,
	&dev_attr_lcd_type.attr,
	&dev_attr_window_type.attr,
	&dev_attr_power_reduce.attr,
	&dev_attr_siop_enable.attr,
#if defined(TEMPERATURE_ELVSS) || defined(TEMPERATURE_ELVSS_S6E3FA0) || defined(TEMPERATURE_ELVSS_S6E8AA4)
	&dev_attr_temperature.attr,
#endif
#if defined(PARTIAL_UPDATE)
	&dev_attr_partial_disp.attr,
#endif
#if defined(FORCE_500CD)
	&dev_attr_force_500cd.attr,
#endif
#if defined(octa_manufacture_date)
	&dev_attr_manufacture_date.attr,
#endif
	NULL
};
static const struct attribute_group panel_sysfs_group = {
	.attrs = panel_sysfs_attributes,
};

#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
static struct attribute *bl_sysfs_attributes[] = {
	&dev_attr_auto_brightness.attr,
	&dev_attr_backlight.attr,
	NULL
};
static const struct attribute_group bl_sysfs_group = {
	.attrs = bl_sysfs_attributes,
};
#endif
#endif

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
static irqreturn_t err_fg_irq_handler(int irq, void *handle)
{
	pr_info("%s handler start irq=%d", __func__, irq);

	if(err_fg_working) return IRQ_HANDLED;

	err_fg_working = 1;
	disable_irq_nosync(gpio_to_irq(err_fg_gpio));
	schedule_work(&err_fg_work);
	pr_info("%s : handler end", __func__);

	return IRQ_HANDLED;
}

static void err_fg_work_func(struct work_struct *work)
{
	struct msm_fb_data_type *mfd = msd.mfd;

	pr_info("%s : start", __func__);

	if (mfd->panel_power_on) {
		int bl_backup = msd.dstat.bright_level;

		mutex_lock(&mfd->power_state);
		mutex_lock(&mfd->ctx_lock);

		msd.pdata->event_handler(msd.pdata, MDSS_EVENT_BLANK, NULL);
		msd.pdata->event_handler(msd.pdata, MDSS_EVENT_PANEL_OFF, NULL);
		mdelay(20);
		msd.pdata->event_handler(msd.pdata, MDSS_EVENT_UNBLANK, NULL);
		msd.pdata->event_handler(msd.pdata, MDSS_EVENT_PANEL_ON, NULL);

		mutex_unlock(&mfd->ctx_lock);
		mutex_unlock(&mfd->power_state);

		/* Restore brightness */
		msd.dstat.bright_level = bl_backup;
		mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
	}
	esd_count++;
	err_fg_working = 0;

	pr_info("%s end", __func__);
	return;
}

#ifdef ESD_DEBUG
static ssize_t mipi_samsung_esd_check_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, 20, "esd count %d\n", esd_count);

	return rc;
}
static ssize_t mipi_samsung_esd_check_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct msm_fb_data_type *mfd = msd.mfd;

	err_fg_irq_handler(0, mfd);
	return 1;
}

static DEVICE_ATTR(esd_check, S_IRUGO , mipi_samsung_esd_check_show,\
			 mipi_samsung_esd_check_store);
#endif
#endif

#if defined(CONFIG_LCD_CRACK_RECOVERY)
static irqreturn_t lcd_crack_irq_handler(int irq, void *handle)
{
	pr_info("%s handler start irq=%d", __func__, irq);
	if(lcd_crack_rec_working) return IRQ_HANDLED;
	lcd_crack_rec_working = 1;
	disable_irq_nosync(gpio_to_irq(lcd_crack_gpio));
	schedule_work(&lcd_crack_rec_work);
	pr_info("%s : handler end", __func__);
	return IRQ_HANDLED;
}
static void lcd_crack_rec_work_func(struct work_struct *work)
{
	pr_info("%s : start", __func__);
	mipi_samsung_disp_send_cmd(PANEL_CRACK_RECOVERY, true);
	lcd_crack_rec_working = 0;
	pr_info("%s end", __func__);
	return;
}
static ssize_t mipi_samsung_lcd_crack_det_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;
	rc = snprintf((char *)buf, 20, " lcd_crack_detect_show  \n");
	return rc;
}
static ssize_t mipi_samsung_lcd_crack_det_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct msm_fb_data_type *mfd = msd.mfd;
	lcd_crack_irq_handler(0, mfd);
	printk("%s: ********** inside ********\n",__func__);
	return size;
}
static DEVICE_ATTR(lcd_crack_check, S_IRUGO , mipi_samsung_lcd_crack_det_show,\
			 mipi_samsung_lcd_crack_det_store);
#endif
int mdss_dsi_panel_init(struct device_node *node, struct mdss_dsi_ctrl_pdata *ctrl_pdata,
				bool cmd_cfg_conf_splash)
{
	int rc = 0;
	static const char *panel_name;
	bool cont_splash_enabled = false;
#if defined(CONFIG_LCD_CLASS_DEVICE)
	struct lcd_device *lcd_device;
#endif
#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
	struct backlight_device *bd = NULL;
#endif
	pr_debug("%s:%d", __func__, __LINE__);

	if (!node)
		return -ENODEV;

	panel_name = of_get_property(node, "label", NULL);
	if (!panel_name)
		pr_info("%s:%d, panel name not specified\n",
						__func__, __LINE__);
	else
		pr_info("%s: Panel Name = %s\n", __func__, panel_name);

	if(is_panel_supported(panel_name)) {
		LCD_DEBUG("Panel : %s is not supported:",panel_name);
		return -1;
	}

	rc = mdss_panel_parse_dt(node, ctrl_pdata);
	if (rc)
		return rc;

	ctrl_pdata->on = mdss_dsi_panel_on;
	ctrl_pdata->off = mdss_dsi_panel_off;
	ctrl_pdata->bl_fnc = mdss_dsi_panel_bl_ctrl;
	ctrl_pdata->panel_reset = mdss_dsi_samsung_panel_reset;
	ctrl_pdata->registered = mdss_dsi_panel_registered;
	ctrl_pdata->dimming_init = mdss_dsi_panel_dimming_init;
	ctrl_pdata->event_handler = samsung_dsi_panel_event_handler;
	ctrl_pdata->panel_blank = mdss_dsi_panel_blank;
	ctrl_pdata->panel_data.set_backlight = mdss_dsi_panel_bl_ctrl;

	/* Init driver  specific data */
	mutex_init(&msd.lock);

	msd.dstat.on = 0;
	msd.dstat.recent_bright_level = 255;

#if !defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
	if (cmd_cfg_conf_splash)
		cont_splash_enabled = of_property_read_bool(node,
				"qcom,cont-splash-enabled");
	else
		cont_splash_enabled = false;
#else
	cont_splash_enabled = of_property_read_bool(node,
			"qcom,cont-splash-enabled");
#endif

#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_VIDEO_720P_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_S6E8AA0A_720P_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_S6E88A0_QHD_VIDEO_PT_PANEL) \
	|| defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
	cont_splash_enabled = of_property_read_bool(node,
			"qcom,cont-splash-enabled");
#endif
	if (get_lcd_attached() == 0)
		cont_splash_enabled = 0;
	if (!cont_splash_enabled) {
		pr_info("%s:%d Continuous splash flag not found.\n",
				__func__, __LINE__);
		ctrl_pdata->panel_data.panel_info.cont_splash_enabled = 0;
	} else {
		pr_info("%s:%d Continuous splash flag enabled.\n",
				__func__, __LINE__);

		ctrl_pdata->panel_data.panel_info.cont_splash_enabled = 1;
	}

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	if(err_fg_enable)
	{
		INIT_WORK(&err_fg_work, err_fg_work_func);

		rc = request_threaded_irq(gpio_to_irq(err_fg_gpio),
			NULL, err_fg_irq_handler,  IRQF_TRIGGER_RISING | IRQF_ONESHOT, "esd_detect", NULL);
		if (rc) {
			pr_err("%s : Failed to request_irq.:ret=%d", __func__, rc);
		}

		disable_irq(gpio_to_irq(err_fg_gpio));
	}
#endif
#if defined(CONFIG_LCD_CRACK_RECOVERY)
	if(lcd_crack_rec_enable)
	{
		INIT_WORK(&lcd_crack_rec_work, lcd_crack_rec_work_func);

		rc = request_threaded_irq(gpio_to_irq(lcd_crack_gpio),
			NULL, lcd_crack_irq_handler,  IRQF_TRIGGER_RISING | IRQF_ONESHOT, "lcd_crack_detect", NULL);
		if (rc) {
			pr_err("%s : Failed to request_irq.:ret=%d", __func__, rc);
		}
		disable_irq(gpio_to_irq(lcd_crack_gpio));
	}
#endif
#if defined(CONFIG_LCD_CLASS_DEVICE)
	lcd_device = lcd_device_register("panel", NULL, NULL,
					&mipi_samsung_disp_props);

	if (IS_ERR(lcd_device)) {
		rc = PTR_ERR(lcd_device);
		printk(KERN_ERR "lcd : failed to register device\n");
		return rc;
	}

	sysfs_remove_file(&lcd_device->dev.kobj,
		&dev_attr_lcd_power.attr);

	rc = sysfs_create_group(&lcd_device->dev.kobj, &panel_sysfs_group);
	if (rc) {
		pr_err("Failed to create panel sysfs group..\n");
		sysfs_remove_group(&lcd_device->dev.kobj, &panel_sysfs_group);
	}

#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
	bd = backlight_device_register("panel", &lcd_device->dev,
						NULL, NULL, NULL);
	if (IS_ERR(bd)) {
		rc = PTR_ERR(bd);
		pr_info("backlight : failed to register device\n");
		return rc;
	}

	rc = sysfs_create_group(&bd->dev.kobj, &bl_sysfs_group);
	if (rc) {
		pr_err("Failed to create backlight sysfs group..\n");
		sysfs_remove_group(&bd->dev.kobj, &bl_sysfs_group);
	}
#endif
#endif

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
#ifdef ESD_DEBUG
	rc= sysfs_create_file(&lcd_device->dev.kobj,
							&dev_attr_esd_check.attr);
	if (rc) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_esd_check.attr.name);
	}
#endif
#endif
#if defined(CONFIG_LCD_CRACK_RECOVERY)

	rc= sysfs_create_file(&lcd_device->dev.kobj,
							&dev_attr_lcd_crack_check.attr);
	if (rc) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_lcd_crack_check.attr.name);
	}
#endif
#if defined(CONFIG_MDNIE_LITE_TUNING)
	pr_info("[%s] CONFIG_MDNIE_LITE_TUNING ok ! init class called!\n",
		__func__);
	init_mdnie_class();
#endif

#if defined(DDI_VIDEO_ENHANCE_TUNING)
	rc = sysfs_create_file(&lcd_device->dev.kobj,
			&dev_attr_tuning.attr);
	if (rc) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_tuning.attr.name);
	}
#endif


#if defined(CONFIG_HAS_EARLYSUSPEND)
	msd.early_suspend.suspend = mipi_samsung_disp_early_suspend;
	msd.early_suspend.resume = mipi_samsung_disp_late_resume;
	msd.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN-1;
	register_early_suspend(&msd.early_suspend);
#endif
	/*
	 * unless panel is powered on don't set the state to true
	 *  This is to handle cases like cont splash or lpm in which panel
	 *  will be powered on later on.
	 */
	msd.dstat.on = 0;
	return 0;
}

int get_samsung_lcd_attached(void)
{
	return lcd_attached;
}
EXPORT_SYMBOL(get_samsung_lcd_attached);

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

#if defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
	if(get_oled_id()==0) // magna only
		msd.manufacture_id = lcd_id;
#endif
	pr_info( "%s: LCD_ID = 0x%X, lcd_attached =%d", __func__,lcd_id, lcd_attached);

	return 0;
}

__setup( "lcd_id=0x", get_lcd_id_cmdline );

#if defined(CONFIG_DUAL_LCD)
static int __init lcd_sel_status(char *mode)
{
	msd.dstat.lcd_sel = simple_strtol(mode, NULL, 10);
	LCD_DEBUG("lcd_sel=%d", msd.dstat.lcd_sel);
	return 1;
}
__setup("lcd_sel=", lcd_sel_status);
#endif

#if (defined(CONFIG_FB_MSM_MDSS_MAGNA_OCTA_VIDEO_720P_PT_PANEL)\
		&& !defined(CONFIG_FB_MSM_MDSS_MAGNA_LDI_EA8061))\
		|| defined(CONFIG_FB_MSM_MDSS_SAMSUNG_OCTA_VIDEO_720P_PT_PANEL)
int get_oled_id(void)
{
/*
	id3 7bit : 0 -> magna(EA8061), 1-> lsi(S6EAA4)
*/
	if(lcd_id&0x00080) return 1;
	return 0;
}
EXPORT_SYMBOL(get_oled_id);

static int __init get_oled_id_cmdline(char *mode)
{
	/*
	oled id : msm gpio 91
	1: s.lsi
	0: magna
	 */
	fresco_oled_id = simple_strtol(mode, NULL, 10);
	LCD_DEBUG("oled_id=%d", fresco_oled_id);
	return 1;
}
__setup("oled_id=", get_oled_id_cmdline);
#endif

static int __init mdss_panel_current_hw_rev(char *rev)
{
	/*
	*	1 is recovery booting
	*	0 is normal booting
	*/

	board_rev = atoi(rev);

	pr_info("%s : %d", __func__, board_rev);

	return 1;
}

__setup("samsung.board_rev=", mdss_panel_current_hw_rev);

MODULE_DESCRIPTION("Samsung DSI panel driver");
MODULE_AUTHOR("Krishna Kishor Jha <krishna.jha@samsung.com>");
MODULE_LICENSE("GPL");
