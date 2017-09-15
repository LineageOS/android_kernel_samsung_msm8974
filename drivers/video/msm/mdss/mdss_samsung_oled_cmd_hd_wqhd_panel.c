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
#include "mdss_samsung_oled_cmd_hd_wqhd_panel.h"
#include "mdss_fb.h"
#include "mdss_debug.h"

#if defined(CONFIG_MDNIE_LITE_TUNING)
#include "mdnie_lite_tuning.h"
#endif

#define DDI_VIDEO_ENHANCE_TUNING
#if defined(DDI_VIDEO_ENHANCE_TUNING)
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#endif

#include <asm/system_info.h>

#define SMART_ACL
#define HBM_RE
#define TEMPERATURE_ELVSS
#define PARTIAL_UPDATE

#define TEST_RESOLUTION /*for sysfs of panel  resolution*/

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
#define _ALPM_GAMMA_COPY_0xBC_
#define ALPM_MODE
#endif

#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_CMD_HD_PT_PANEL)

/*normally,err fg intterupt cannot detect esd in still img. on command mode panle:
So this one makes the err fg interrup go low level with lcd on seq. (in dtsi) when ddi reset is abnormal  */

#define CONFIG_ESD_FG_RECOVERY
#define CONFIG_PCD_CHECK  /*to check pcd*/
#endif

static struct dsi_buf dsi_panel_tx_buf;
static struct dsi_buf dsi_panel_rx_buf;

static struct dsi_cmd display_on_seq;
static struct dsi_cmd display_off_seq;

static struct dsi_cmd hsync_on_seq;

static struct dsi_cmd display_on_cmd;
static struct dsi_cmd display_off_cmd;

static struct dsi_cmd test_key_enable_cmds;
static struct dsi_cmd test_key_disable_cmds;

static struct dsi_cmd nv_mtp_read_cmds;
static struct dsi_cmd nv_enable_cmds;
static struct dsi_cmd nv_disable_cmds;
static struct dsi_cmd manufacture_id_cmds;
static struct dsi_cmd manufacture_date_cmds;
static struct dsi_cmd ddi_id_cmds;
static struct dsi_cmd rddpm_cmds;

static struct dsi_cmd mtp_read_sysfs_cmds;

static struct dsi_cmd acl_off_cmd;
static struct cmd_map acl_map_table;
static struct dsi_cmd acl_cmds_list;

static struct candella_lux_map candela_map_table;

static struct dsi_cmd aclcont_cmds_list;
static struct dsi_cmd gamma_cmds_list;

static struct dsi_cmd elvss_cmds_list;

static struct cmd_map aid_map_table;
static struct dsi_cmd aid_cmds_list;

#if defined(HBM_RE)
static struct dsi_cmd nv_mtp_hbm_read_cmds;
static struct dsi_cmd nv_mtp_hbm2_read_cmds;
static struct dsi_cmd hbm_gamma_cmds_list;
static struct dsi_cmd hbm_etc_cmds_list;
static struct dsi_cmd hbm_off_cmd;
#endif
static struct dsi_cmd nv_mtp_elvss_read_cmds;

#if defined(CONFIG_MDNIE_LITE_TUNING)
static struct dsi_cmd nv_mdnie_read_cmds;
#endif
#ifdef DEBUG_LDI_STATUS
static struct dsi_cmd ldi_debug_cmds;
#endif
#if defined(TEMPERATURE_ELVSS)
static struct dsi_cmd elvss_lowtemp_cmds_list;
static struct dsi_cmd elvss_lowtemp2_cmds_list;
#endif
#if defined(SMART_ACL)
static struct dsi_cmd smart_acl_elvss_cmds_list;
static struct cmd_map smart_acl_elvss_map_table;
#endif
#if defined(PARTIAL_UPDATE)
static struct dsi_cmd partialdisp_on_cmd;
static struct dsi_cmd partialdisp_off_cmd;
static int partial_disp_range[2];
#endif
#if defined(ALPM_MODE)
/* ALPM mode on/off command */
static struct dsi_cmd alpm_on_seq;
static struct dsi_cmd alpm_off_seq;
/* Partial area command for ALPM mode */
static struct dsi_cmd alpm_set_partial_area_cmd;
/* ALPM gamma command for 10cd, 60cd */
static struct dsi_cmd alpm_gamma_read_cmd;
static struct dsi_cmd alpm_set_gamma_cmd;
static struct dsi_cmd alpm_set_gamma_10cd_cmd;
static struct dsi_cmd alpm_set_gamma_60cd_cmd;
/* Status variable for each command */
static int alpm_disp_range[2];
static int alpm_partial_area_index;
static int alpm_gamma_index;
static int alpm_gamma_para_index;
static int alpm_gamma_para_length;
#endif
#if defined(DYNAMIC_FPS_USE_TE_CTRL)
int dynamic_fps_use_te_ctrl;
#endif
#if defined(CONFIG_LCD_HMT)
static struct dsi_cmd hmt_aid_ready_to_foward_cmd;
static struct dsi_cmd hmt_dual_scan_full_enable;
static struct dsi_cmd hmt_dual_scan_disable;
static struct dsi_cmd hmt_reverse_enable;
static struct dsi_cmd hmt_reverse_disable;
static struct cmd_map aid_map_table_hmt;
static struct dsi_cmd aid_cmds_list_hmt;
static struct dsi_cmd aid_cmds_list_hmt_single;
static struct dsi_cmd hmt_aid_cmd;
static struct candella_lux_map candela_map_table_hmt;
extern int skip_te_enable;
#define HMT_AID_INDEX 9
#if 0
static struct dsi_cmd hmt_change_porch_cmds;
static struct dsi_cmd hmt_level_enable_cmds;
static struct dsi_cmd hmt_level_disable_cmds;
static struct dsi_cmd force_500;
static struct dsi_cmd hmt_aid_ready_to_reverse_cmd;
static struct dsi_cmd hmt_dual_scan_half_enable;
static struct dsi_cmd hmt_read_ldi_fps_cmds;
static struct dsi_cmd hmt_write_ldi_fps_cmds;
static struct dsi_cmd hmt_hbm_etc_cmds;
#define HMT_REVERSE_INDEX 50
static int hmt_hbm_input = 0;
#endif

#endif

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
static struct dsi_cmd panel_set_te_osc_b;
static struct dsi_cmd panel_set_te_restore;
static struct dsi_cmd panel_set_te;
static struct dsi_cmd panel_set_te_1;
static struct dsi_cmd panel_set_te_2;
static struct dsi_cmd panel_osc_type_read_cmds;
extern int te_set_done;
#endif

static struct mipi_samsung_driver_data msd;
/*List of supported Panels with HW revision detail
 * (one structure per project)
 * {hw_rev,"label string given in panel dtsi file"}
 * */
static struct  panel_hrev panel_supp_cdp[]= {
	{"samsung amoled 1080p video mode dsi S6E8FA0 panel", PANEL_FHD_OCTA_S6E8FA0},
	{"samsung amoled 1080p video mode dsi S6E3FA0 panel", PANEL_FHD_OCTA_S6E3FA0},
	{"samsung amoled 1080p command mode dsi S6E3FA0 panel", PANEL_FHD_OCTA_S6E3FA0_CMD},
	{"samsung amoled 1080p command mode dsi S6E3FA2 panel",PANEL_FHD_OCTA_S6E3FA2_CMD },
	{"magna amoled 1080p command mode dsi ea8064g panel",PANEL_FHD_OCTA_EA8064G_CMD },
	{"magna amoled hd command mode dsi ea8064g panel",PANEL_HD_OCTA_EA8064G_CMD },
	{"samsung amoled wqhd command mode dsi1 S6E3HA0 panel", PANEL_WQHD_OCTA_S6E3HA0_CMD },
	{"samsung amoled wqhd command mode dsi0 S6E3HA0 panel", PANEL_WQHD_OCTA_S6E3HA0_CMD },
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
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 0}, NULL},
};

#define MAX_BR_PACKET_SIZE sizeof(brightness_packet)/sizeof(struct dsi_cmd_desc)

DEFINE_LED_TRIGGER(bl_led_trigger);

static struct mdss_dsi_phy_ctrl phy_params;

static int lcd_attached = 1;
static int lcd_id = 0;
static int lcd_lsi_ldi = 0;

#if defined(CONFIG_ESD_FG_RECOVERY)
#define ESD_DEBUG 1

struct work_struct  err_fg_work;
static int err_fg_gpio = 0;	/* PM_GPIO4 */
static int esd_count = 0;
int err_fg_working = 0;
#endif

#if defined(CONFIG_PCD_CHECK)
static int pcd_gpio = 0;
#endif

#if defined(CONFIG_LCD_CLASS_DEVICE) && defined(DDI_VIDEO_ENHANCE_TUNING)
#define MAX_FILE_NAME 128
#define TUNING_FILE_PATH "/sdcard/"
#define TUNE_FIRST_SIZE 22
#define TUNE_SECOND_SIZE 128

static char tuning_file[MAX_FILE_NAME];
static char mdni_tuning1[TUNE_FIRST_SIZE];
static char mdni_tuning2[TUNE_SECOND_SIZE];

static struct dsi_cmd_desc mdni_tune_cmd[] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(mdni_tuning2)}, mdni_tuning2},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(mdni_tuning1)}, mdni_tuning1},
};
#endif

static int mipi_samsung_disp_send_cmd(
		enum mipi_samsung_cmd_list cmd,
		unsigned char lock);
extern void mdss_dsi_panel_touchsensing(int enable);
int get_lcd_attached(void);
void set_samsung_lcd_attached(int set);
int get_lcd_id(void);
int get_lcd_ldi_info(void);
#if defined(ALPM_MODE)
/*
 * APIs for ALPM mode
 * alpm_store()			: Check or store status like alpm mode status or brightness level
 * alpm_gamma_read()	: Read six gamma value for alpm mode from RBCh register
						  the first three value will use for 60cd gamma
						  the next three value will use for 10cd gamma
						  two gamma set will write to RBBh register as 7th, 8th, 9th parameter
 * alpm_copy_cmds()		: Copy alpm command from array to array with given length
 */
static void alpm_store(u8 mode);
static void alpm_gamma_read(void);
static void alpm_copy_cmds(char *dest, char *src, u8 size);
static void alpm_cmd_debug(char *src, int size);
#endif

int set_panel_rev(unsigned int id)
{
#if defined(CONFIG_FB_MSM_MIPI_MAGNA_OCTA_CMD_HD_PT_PANEL)
	if (msd.panel == PANEL_HD_OCTA_EA8064G_CMD) {
		switch (id & 0xFF) {
			case 0x00:
				pr_info("%s : 0x00 EVT0_S_hd_REVA \n",__func__);
				msd.id3 = EVT0_S_hd_REVA;
				break;
			case 0x01:
				pr_info("%s : 0x01 EVT0_S_hd_REVC \n",__func__);
				msd.id3 = EVT0_S_hd_REVC;
				break;
			case 0x02:
				pr_info("%s : 0x02 EVT1_S_hd_REVD \n",__func__);
				msd.id3 = EVT1_S_hd_REVD;
				break;
			case 0x03:
				pr_info("%s : 0x03 EVT1_S_hd_REVF \n",__func__);
				msd.id3 = EVT1_S_hd_REVF;
				break;
			case 0x04:
				pr_info("%s : 0x04 EVT1_S_hd_REVG \n",__func__);
				msd.id3 = EVT1_S_hd_REVG;
				break;
			default:
				if(get_lcd_id())
					msd.id3 = (get_lcd_id()&0xFF);
				else
					msd.id3 = EVT1_S_hd_REVG;

				pr_info("%s : can't find panel id..but set as (0x%x) \n", __func__,msd.id3 );
				break;
		}
	}
#else
	if (msd.panel == PANEL_FHD_OCTA_S6E3FA2_CMD) {
		switch (id & 0xFF) {
			case 0x00:
				pr_info("%s : 0x00 EVT0_K_fhd_REVB \n",__func__);
				msd.id3 = EVT0_K_fhd_REVB;
				break;
			case 0x01:
				pr_info("%s : 0x01 EVT0_K_fhd_REVF \n",__func__);
				msd.id3 = EVT0_K_fhd_REVF;
				break;
			case 0x02:
				pr_info("%s : 0x02 EVT0_K_fhd_REVG \n",__func__);
				msd.id3 = EVT0_K_fhd_REVG;
				break;
			case 0x12:
				pr_info("%s : 0x12 EVT1_K_fhd_REVH \n",__func__);
				msd.id3 = EVT1_K_fhd_REVH;
				break;
			case 0x13:
				pr_info("%s : 0x13 EVT1_K_fhd_REVI \n",__func__);
				msd.id3 = EVT1_K_fhd_REVI;
				break;
			default:
				if(get_lcd_id())
					msd.id3 = (get_lcd_id()&0xFF);
				else
					msd.id3 = EVT1_K_fhd_REVI;

				pr_info("%s : can't find panel id..but set as (0x%x) \n", __func__,msd.id3 );
				break;

		}
	} else {	/*PANEL_FHD_OCTA_EA8064G_CMD  */
		switch (id & 0xFF) {
			case 0x50:
				pr_info("%s : 0x50 EVT2_K_fhd_magna_REVA \n",__func__);
				msd.id3 = EVT2_K_fhd_magna_REVA;
				break;
			case 0x61:
				pr_info("%s : 0x61 EVT2_K_fhd_magna_REVB \n",__func__);
				msd.id3 = EVT2_K_fhd_magna_REVB;
				break;
			case 0x72:
				pr_info("%s : 0x72 EVT2_K_fhd_magna_REVC \n",__func__);
				msd.id3 = EVT2_K_fhd_magna_REVC;
				break;
			default:
				if(get_lcd_id())
					msd.id3 = (get_lcd_id()&0xFF);
				else
					msd.id3 = EVT2_K_fhd_magna_REVC;

				pr_info("%s : can't find panel id..but set as (0x%x) \n", __func__,msd.id3 );

				break;
		}
	}
#endif
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

static int mdss_dsi_extra_power(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	int ret = 0 ;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return ret;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	pr_info("%s: ++ enable(%d) ndx(%d)\n",
			   __func__,enable, ctrl_pdata->ndx );

	if(ctrl_pdata->ndx == DSI_CTRL_1)
		return ret;

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_info("%s:%d, disp_en_gpio not configured\n",
			   __func__, __LINE__);
		return ret;
	}

	if (enable) {
			pr_info("%s: lvs4(VDD3) enable.\n",__func__);

			ret = regulator_enable( /*VDD3 :1.8V*/
				(ctrl_pdata->shared_pdata).iovdd_vreg);
			if (ret) {
				pr_err("%s: Failed to enable iovdd regulator.\n",
					__func__);
				return ret;
			}

			usleep_range(5000, 5000);

			pr_debug("%s: disp_en_gpio set high  \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);/* VDDR :1.5*/
			usleep_range(5000, 5000);

	}
	else{
			pr_debug("%s: disp_en_gpio set low  \n", __func__);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);/* VDDR :1.5*/
			usleep_range(5000, 5000);

			pr_info("%s lvs4(VDD3) diable\n", __func__);
			mdelay(1);
			ret = regulator_disable(
				(ctrl_pdata->shared_pdata).iovdd_vreg);
			if (ret) {
				pr_err("%s: Failed to disable regulator.\n",
					__func__);
			}

			usleep_range(5000, 5000);
	}
	pr_debug("%s: --\n", __func__);
	return ret;
}

void mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = &msd.pdata->panel_info;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	if (pinfo->alpm_event) {
		if (enable && pinfo->alpm_event(CHECK_PREVIOUS_STATUS))
			return;
		else if (!enable && pinfo->alpm_event(CHECK_CURRENT_STATUS)) {
			pinfo->alpm_event(STORE_CURRENT_STATUS);
			return;
		}
		pr_debug("[ALPM_DEBUG] %s: Panel reset, enable : %d\n",
					__func__, enable);

	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);


	pr_info("%s: enable(%d) ndx(%d)\n",
			   __func__,enable, ctrl_pdata->ndx );

	if(ctrl_pdata->ndx == DSI_CTRL_1)
		return;

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_err("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
		return;
	}

	if (enable) {
		gpio_set_value((ctrl_pdata->rst_gpio), 1);
		usleep_range(5000, 5000);
		wmb();
		gpio_set_value((ctrl_pdata->rst_gpio), 0);
		usleep_range(5000, 5000);
		wmb();
		gpio_set_value((ctrl_pdata->rst_gpio), 1);
		usleep_range(7000, 7000);
		wmb();

	} else {
		gpio_set_value((ctrl_pdata->rst_gpio), 0);

	}
}

static char caset[] = {0x2a, 0x00, 0x00, 0x03, 0x00};	/* DTYPE_DCS_LWRITE */
static char paset[] = {0x2b, 0x00, 0x00, 0x05, 0x00};	/* DTYPE_DCS_LWRITE */
static struct dsi_cmd_desc partial_update_enable_cmd[] = {
	{{DTYPE_DCS_LWRITE, 0, 0, 0, 0, sizeof(caset)}, caset},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(paset)}, paset},
};
int mdss_dsi_panel_partial_update(struct mdss_panel_data *pdata)
{
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct dcs_cmd_req cmdreq;
	int rc = 0;
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	mipi  = &pdata->panel_info.mipi;
	pr_debug("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);

	caset[1] = (((pdata->panel_info.roi_x) & 0xFF00) >> 8);
	caset[2] = (((pdata->panel_info.roi_x) & 0xFF));
	caset[3] = (((pdata->panel_info.roi_x - 1 + pdata->panel_info.roi_w)
								& 0xFF00) >> 8);
	caset[4] = (((pdata->panel_info.roi_x - 1 + pdata->panel_info.roi_w)
								& 0xFF));
	partial_update_enable_cmd[0].payload = caset;
	paset[1] = (((pdata->panel_info.roi_y) & 0xFF00) >> 8);
	paset[2] = (((pdata->panel_info.roi_y) & 0xFF));
	paset[3] = (((pdata->panel_info.roi_y - 1 + pdata->panel_info.roi_h)
								& 0xFF00) >> 8);
	paset[4] = (((pdata->panel_info.roi_y - 1 + pdata->panel_info.roi_h)
								& 0xFF));
	partial_update_enable_cmd[1].payload = paset;
	pr_debug("%s: enabling partial update\n", __func__);
        pr_debug("%s: x=%d y=%d w=%d h=%d\n",
                        __func__,  pdata->panel_info.roi_x, pdata->panel_info.roi_y,
                                        pdata->panel_info.roi_w, pdata->panel_info.roi_h);
        pr_debug("%s:{0x%0x,0x%0x,0x%0x,0x%0x,0x%0x}\n",__func__,
                        caset[0],caset[1],caset[2],caset[3],caset[4]);
        pr_debug("%s:{0x%0x,0x%0x,0x%0x,0x%0x,0x%0x}\n",__func__,
                        paset[0],paset[1],paset[2],paset[3],paset[4]);
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = partial_update_enable_cmd;
	cmdreq.cmds_cnt = 2;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
	return rc;
}

static int get_candela_value(int bl_level)
{
	return candela_map_table.lux_tab[candela_map_table.bkl[bl_level]];
}

static int get_cmd_idx(int bl_level)
{
	return candela_map_table.cmd_idx[candela_map_table.bkl[bl_level]];
}

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

#if defined(HBM_RE)

static struct dsi_cmd get_hbm_off_set(void)
{
	struct dsi_cmd hbm_off = {0,};

	hbm_off.cmd_desc = &(hbm_off_cmd.cmd_desc[0]);
	hbm_off.num_of_cmds = 1;

	return hbm_off;
}
#endif
/*
	This function takes aid_map_table and uses cd_idx,
	to get the index of the command in aid command list.

*/
#if defined(CONFIG_LCD_HMT)
static struct dsi_cmd get_aid_aor_control_set_hmt(int cd_idx)
{
	struct dsi_cmd aid_control = {0,};
	int cmd_idx = 0;
	char *c_payload;

	if (!aid_map_table_hmt.size || !(cd_idx < aid_map_table_hmt.size))
		goto end;

	/* Get index in the aid command list*/
	cmd_idx = aid_map_table_hmt.cmd_idx[cd_idx];

	if (msd.hmt_stat.hmt_dual)
		c_payload = aid_cmds_list_hmt.cmd_desc[cmd_idx].payload;
	else
		c_payload = aid_cmds_list_hmt_single.cmd_desc[cmd_idx].payload;

	/* Get the command desc */
	if (msd.hmt_stat.hmt_dual)
		aid_control.cmd_desc = &(aid_cmds_list_hmt.cmd_desc[cmd_idx]);
	else
		aid_control.cmd_desc = &(aid_cmds_list_hmt_single.cmd_desc[cmd_idx]);

	aid_control.num_of_cmds = 1;
	msd.dstat.curr_aid_idx = cmd_idx;

end:
	return aid_control;
}
#endif

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
		c_payload = aid_cmds_list.cmd_desc[cmd_idx].payload;


		/* Check if current & previous commands are same */
		if (p_idx >= 0) {
			p_payload = aid_cmds_list.cmd_desc[p_idx].payload;
			payload_size = aid_cmds_list.cmd_desc[p_idx].dchdr.dlen;

			if (!memcmp(p_payload, c_payload, payload_size))
				goto end;
		}

	/* Get the command desc */
	aid_control.cmd_desc = &(aid_cmds_list.cmd_desc[cmd_idx]);

	aid_control.num_of_cmds = 1;
	msd.dstat.curr_aid_idx = cmd_idx;

end:
	return aid_control;
}

/*
	This function takes acl_map_table and uses cd_idx,
	to get the index of the command in elvss command list.

*/

static struct dsi_cmd get_acl_control_on_set(void)
{
	struct dsi_cmd aclcont_control = {0,};
	int acl_cond = msd.dstat.curr_acl_cond;

	if (acl_cond) /* already acl condition setted */
		goto end;

	/* Get the command desc */
	aclcont_control.cmd_desc = aclcont_cmds_list.cmd_desc;
	aclcont_control.num_of_cmds = aclcont_cmds_list.num_of_cmds;
	msd.dstat.curr_acl_cond = 1;

	pr_info("%s #(%d)\n",
				__func__, aclcont_cmds_list.num_of_cmds);

end:
	return aclcont_control;
}

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
	acl_control.cmd_desc = acl_off_cmd.cmd_desc; /* idx 0 : ACL OFF */
	acl_control.num_of_cmds = acl_off_cmd.num_of_cmds;

	pr_info("%s #(%d)\n",
				__func__, acl_off_cmd.num_of_cmds);

	msd.dstat.curr_acl_idx = 0;
	msd.dstat.curr_acl_cond = 0;

end:
	return acl_control;
}

#if defined(TEMPERATURE_ELVSS)
// ELVSS TEMPERATURE COMPENSATION for S6E3FA0
static struct dsi_cmd get_elvss_tempcompen_control_set(void)
{
	struct dsi_cmd elvss_tempcompen_control = {0,};

	pr_debug("%s for ELVSS CONTROL acl(%d), temp(%d)\n",
				__func__, msd.dstat.acl_on, msd.dstat.temperature);

	/* Get the command desc */
	if (msd.dstat.temperature > 0) {
		pr_debug("%s temp > 0 \n",__func__);
		elvss_lowtemp_cmds_list.cmd_desc[1].payload[1] = 0x19; // B8
	} else if (msd.dstat.temperature > -20) {
		pr_debug("%s 0 >= temp > -20 \n",__func__);
		elvss_lowtemp_cmds_list.cmd_desc[1].payload[1] = 0x00; // B8
	} else {
		pr_debug("%s temp <= -20 \n",__func__);
		elvss_lowtemp_cmds_list.cmd_desc[1].payload[1] = 0x94; // B8
	}

	pr_info("%s for ELVSS CONTROL acl(%d), temp(%d) B8(0x%x) \n",
				__func__, msd.dstat.acl_on, msd.dstat.temperature,
				elvss_lowtemp_cmds_list.cmd_desc[1].payload[1]);

	elvss_tempcompen_control.cmd_desc = elvss_lowtemp_cmds_list.cmd_desc;
	elvss_tempcompen_control.num_of_cmds = elvss_lowtemp_cmds_list.num_of_cmds;

	return elvss_tempcompen_control;
}

static struct dsi_cmd get_elvss_tempcompen_control_set2(void)
{
	struct dsi_cmd elvss_tempcompen_control2 = {0,};

	pr_debug("%s for ELVSS CONTROL acl2(%d), temp(%d)\n",
				__func__, msd.dstat.acl_on, msd.dstat.temperature);

	/* Get the command desc */
	if (msd.dstat.temperature > -20) /*b6 21th para*/
		elvss_lowtemp2_cmds_list.cmd_desc[1].payload[1] = msd.dstat.elvss_value;
	else /*temp <= -20 : b6 21th para-0x05*/
		elvss_lowtemp2_cmds_list.cmd_desc[1].payload[1] = (msd.dstat.elvss_value - 0x05);

	pr_info("%s for ELVSS CONTROL acl(%d), temp(%d) B0(0x%x) B6(0x%x)\n",
				__func__, msd.dstat.acl_on, msd.dstat.temperature,
				elvss_lowtemp2_cmds_list.cmd_desc[0].payload[1],
				elvss_lowtemp2_cmds_list.cmd_desc[1].payload[1]);

	elvss_tempcompen_control2.cmd_desc = elvss_lowtemp2_cmds_list.cmd_desc;
	elvss_tempcompen_control2.num_of_cmds = elvss_lowtemp2_cmds_list.num_of_cmds;

	return elvss_tempcompen_control2;
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

	pr_debug("%s for SMART_ACL acl(%d), temp(%d)\n",
			__func__, msd.dstat.acl_on, msd.dstat.temperature);

	if (!smart_acl_elvss_map_table.size || !(cd_idx < smart_acl_elvss_map_table.size) ||
			!smart_acl_elvss_map_table.size ||
			!(cd_idx < smart_acl_elvss_map_table.size)) {
		pr_err("%s failed mapping elvss table\n",__func__);
		goto end;
	}

	cmd_idx = smart_acl_elvss_map_table.cmd_idx[cd_idx];

	/* Get the command desc */
	if(msd.dstat.acl_on || msd.dstat.siop_status) {
		if(msd.panel == PANEL_FHD_OCTA_EA8064G_CMD){
			if (msd.dstat.temperature > 0)
				smart_acl_elvss_cmds_list.cmd_desc[cmd_idx].payload[1] = 0x48;
			else
				smart_acl_elvss_cmds_list.cmd_desc[cmd_idx].payload[1] = 0x4C;
		}else if(msd.panel == PANEL_HD_OCTA_EA8064G_CMD){
				smart_acl_elvss_cmds_list.cmd_desc[cmd_idx].payload[1] = 0x4C;
		}else {
			if (msd.dstat.temperature > 0)
				smart_acl_elvss_cmds_list.cmd_desc[cmd_idx].payload[1] = 0x88;
			else
				smart_acl_elvss_cmds_list.cmd_desc[cmd_idx].payload[1] = 0x8C;
		}
			payload = smart_acl_elvss_cmds_list.cmd_desc[cmd_idx].payload;
			elvss_control.cmd_desc = &(smart_acl_elvss_cmds_list.cmd_desc[cmd_idx]);

		pr_debug("ELVSS for SMART_ACL cd_idx=%d, cmd_idx=%d\n", cd_idx, cmd_idx);
	} else {
		if(msd.panel == PANEL_FHD_OCTA_EA8064G_CMD){
			if (msd.dstat.temperature > 0)
				elvss_cmds_list.cmd_desc[cmd_idx].payload[1] = 0x58;
			else
				elvss_cmds_list.cmd_desc[cmd_idx].payload[1] = 0x5C;
		}else if(msd.panel == PANEL_HD_OCTA_EA8064G_CMD){
				elvss_cmds_list.cmd_desc[cmd_idx].payload[1] = 0x5C;
		}else{
			if (msd.dstat.temperature > 0)
				elvss_cmds_list.cmd_desc[cmd_idx].payload[1] = 0x98;
			else
				elvss_cmds_list.cmd_desc[cmd_idx].payload[1] = 0x9C;
		}
			payload = elvss_cmds_list.cmd_desc[cmd_idx].payload;
			elvss_control.cmd_desc = &(elvss_cmds_list.cmd_desc[cmd_idx]);

		pr_debug("ELVSS for normal cd_idx=%d, cmd_idx=%d\n", cd_idx, cmd_idx);
	}

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

	if (!smart_acl_elvss_map_table.size || !(cd_idx < smart_acl_elvss_map_table.size))
	{
		pr_err("%s invalid elvss mapping \n",__func__);
		goto end;
	}

	/* Get index in the acl command list*/
	cmd_idx = smart_acl_elvss_map_table.cmd_idx[cd_idx];

	c_payload = elvss_cmds_list.cmd_desc[cmd_idx].payload;

	/* Check if current & previous commands are same */
	if (p_idx >= 0) {
		p_payload = elvss_cmds_list.cmd_desc[p_idx].payload;
		payload_size = elvss_cmds_list.cmd_desc[p_idx].dchdr.dlen;

		if (msd.dstat.curr_elvss_idx == cmd_idx ||
			!memcmp(p_payload, c_payload, payload_size))
			goto end;
	}

	elvss_control.cmd_desc = &(elvss_cmds_list.cmd_desc[cmd_idx]);

	elvss_control.num_of_cmds = 1;
	msd.dstat.curr_elvss_idx = cmd_idx;

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

	if (cmd_count > (MAX_BR_PACKET_SIZE - 1))/*cmd_count is index, if cmd_count >16 then panic*/
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

#if defined(ALPM_MODE)
	alpm_cmd_debug(brightness_packet[cmd_count].payload,
					brightness_packet[cmd_count].dchdr.dlen);
#endif

	return cmd_count;
}

#if defined(HBM_RE)
static struct dsi_cmd get_hbm_etc_control_set(void)
{
	struct dsi_cmd etc_hbm_control = {0,};

	/* Get the command desc */
	etc_hbm_control.cmd_desc = &(hbm_etc_cmds_list.cmd_desc[0]);
	etc_hbm_control.num_of_cmds = hbm_etc_cmds_list.num_of_cmds;

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
	struct dsi_cmd elvss_control = {0,};
	struct dsi_cmd testKey = {0, 0, 0, 0, 0};

	int cmd_count = 0, cd_idx = 0;

	cd_idx = get_cmd_idx(bl_level);

	if (msd.dstat.hbm_mode) {
		pr_err("%s : already hbm mode! return .. \n", __func__);
		return 0;
	}

	testKey = get_testKey_set(1);
	cmd_count = update_bright_packet(cmd_count, &testKey);

    if(msd.panel == PANEL_FHD_OCTA_S6E3FA2_CMD){
		gamma_control = get_hbm_gamma_control_set();
		cmd_count = update_bright_packet(cmd_count, &gamma_control);
   }else if( msd.panel == PANEL_FHD_OCTA_EA8064G_CMD ){
		elvss_control = get_elvss_control_set(cd_idx);
		cmd_count = update_bright_packet(cmd_count, &elvss_control);
    }

	hbm_etc_control = get_hbm_etc_control_set();
	cmd_count = update_bright_packet(cmd_count, &hbm_etc_control);

	testKey = get_testKey_set(0);
	cmd_count = update_bright_packet(cmd_count, &testKey);

	/* for non hbm mode : reset */
	msd.dstat.curr_elvss_idx = -1;
	msd.dstat.curr_acl_idx = -1;
	msd.dstat.curr_opr_idx = -1;
	msd.dstat.curr_aid_idx = -1;
	msd.dstat.curr_acl_cond = 0;

	LCD_DEBUG("HBM : %d\n", cmd_count);
	return cmd_count;

}
#endif

#if 0
#if defined(CONFIG_LCD_HMT)
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

#if defined(CONFIG_LCD_HMT)
static struct dsi_cmd get_gamma_control_set_hmt(int candella, int dual)
{
	struct dsi_cmd gamma_control = {0,};
	/* Just a safety check to ensure smart dimming data is initialised well */

	if (dual) {
		BUG_ON(msd.sdimconf_hmt->generate_gamma == NULL);
		msd.sdimconf_hmt->generate_gamma(candella, &gamma_cmds_list.cmd_desc[0].payload[1], dual);
	} else {
		BUG_ON(msd.sdimconf_hmt_single->generate_gamma == NULL);
		msd.sdimconf_hmt_single->generate_gamma(candella, &gamma_cmds_list.cmd_desc[0].payload[1], dual);
	}

	gamma_control.cmd_desc = &(gamma_cmds_list.cmd_desc[0]);
	gamma_control.num_of_cmds = gamma_cmds_list.num_of_cmds;
	return gamma_control;
}

static int get_candela_value_hmt(int bl_level)
{
	return candela_map_table_hmt.lux_tab[candela_map_table_hmt.bkl[bl_level]];
}

static int get_cmd_idx_hmt(int bl_level)
{
	return candela_map_table_hmt.cmd_idx[candela_map_table_hmt.bkl[bl_level]];
}

static int make_brightcontrol_set_hmt(int bl_level)
{
	struct dsi_cmd aid_control = {0,};
	struct dsi_cmd gamma_control = {0,};
	struct dsi_cmd testKey = {0,};

	int cmd_count = 0, cd_idx = 0, cd_level =0;

	cd_idx = get_cmd_idx_hmt(bl_level);
	cd_level = get_candela_value_hmt(bl_level);

	pr_info("%s : HMT : cd_idx (%d), cd_level (%d) \n", __func__, cd_idx, cd_level);

	/* level2 enable */
	testKey = get_testKey_set(1);
	cmd_count = update_bright_packet(cmd_count, &testKey);

	/* aid/aor */
	aid_control = get_aid_aor_control_set_hmt(cd_idx);
	cmd_count = update_bright_packet(cmd_count, &aid_control);

	/*gamma*/
	gamma_control = get_gamma_control_set_hmt(cd_level, msd.hmt_stat.hmt_dual);
	cmd_count = update_bright_packet(cmd_count, &gamma_control);

	/* level2 disable */
	testKey = get_testKey_set(0);
	cmd_count = update_bright_packet(cmd_count, &testKey);

	return cmd_count;

}

#endif
static int make_brightcontrol_set(int bl_level)
{
	struct dsi_cmd aid_control = {0,};
	struct dsi_cmd acl_control = {0,};
	struct dsi_cmd acl_on_cont = {0,};
	struct dsi_cmd acl_off_cont = {0,};
	struct dsi_cmd elvss_control = {0,};

#if defined(HBM_RE)
	struct dsi_cmd hbm_off_control = {0,};
#endif
	struct dsi_cmd gamma_control = {0,};
	struct dsi_cmd testKey = {0,};
#if defined(TEMPERATURE_ELVSS)
	struct dsi_cmd temperature_elvss_control = {0,};
	struct dsi_cmd temperature_elvss_control2 = {0,};
#endif
	int cmd_count = 0, cd_idx = 0, cd_level =0;

	cd_idx = get_cmd_idx(bl_level);
	cd_level = get_candela_value(bl_level);

	testKey = get_testKey_set(1);
	cmd_count = update_bright_packet(cmd_count, &testKey);

	aid_control = get_aid_aor_control_set(cd_idx);

	cmd_count = update_bright_packet(cmd_count, &aid_control);

#if defined(HBM_RE)
	if ((msd.panel == PANEL_HD_OCTA_EA8064G_CMD)||(msd.panel == PANEL_FHD_OCTA_EA8064G_CMD)) {
		/*hbm off cmd*/
		if(msd.dstat.hbm_mode == 1){
			hbm_off_control = get_hbm_off_set();/*53 00*/
			cmd_count = update_bright_packet(cmd_count, &hbm_off_control);
		}
	}
#endif
	/* acl */
	if (msd.dstat.acl_on||msd.dstat.siop_status) {
		acl_on_cont = get_acl_control_on_set(); /*b5 51/29*/
		cmd_count = update_bright_packet(cmd_count, &acl_on_cont);
		acl_control = get_acl_control_set(cd_idx); /*55 02*/
		cmd_count = update_bright_packet(cmd_count, &acl_control);
	} else {
		/* acl off */
		acl_off_cont = get_acl_control_off_set(); /*b5 41,55 00 */
		cmd_count = update_bright_packet(cmd_count, &acl_off_cont);
	}

	/*elvss*/
	elvss_control = get_elvss_control_set(cd_idx);
	cmd_count = update_bright_packet(cmd_count, &elvss_control);

#if defined(TEMPERATURE_ELVSS)
	/* ELVSS TEMPERATURE COMPENSATION*/
	/* ELVSS for Temperature set cmd should be sent after normal elvss set cmd*/
	if (msd.dstat.elvss_need_update) {
		temperature_elvss_control = get_elvss_tempcompen_control_set();
		cmd_count = update_bright_packet(cmd_count, &temperature_elvss_control);

	if (((msd.panel == PANEL_FHD_OCTA_S6E3FA2_CMD)  && (msd.id3 >= EVT0_K_fhd_REVG)) || \
           (msd.panel == PANEL_FHD_OCTA_EA8064G_CMD) ) {
		temperature_elvss_control2 = get_elvss_tempcompen_control_set2();
		cmd_count = update_bright_packet(cmd_count, &temperature_elvss_control2);
	}
		msd.dstat.elvss_need_update = 0;
	}
#endif

	/*gamma*/
	gamma_control = get_gamma_control_set(cd_level);
	cmd_count = update_bright_packet(cmd_count, &gamma_control);

	testKey = get_testKey_set(0);
	cmd_count = update_bright_packet(cmd_count, &testKey);

#if defined(TEMPERATURE_ELVSS)
	LCD_DEBUG("bright_level: %d, candela_idx: %d( %d cd ), "\
		"cmd_count(aid,acl,elvss,temperature,gamma)::(%d,%d,%d,%d,%d)%d,id3(0x%x)\n",
#else
	LCD_DEBUG("bright_level: %d, candela_idx: %d( %d cd ), "\
		"cmd_count(aid,acl,elvss,temperature,gamma)::(%d,%d,%d,%d)%d,id3(0x%x)\n",
#endif
		msd.dstat.bright_level, cd_idx, cd_level,
		aid_control.num_of_cmds,
		msd.dstat.acl_on | msd.dstat.siop_status,
		elvss_control.num_of_cmds,
#if defined(TEMPERATURE_ELVSS)
		temperature_elvss_control.num_of_cmds,
#endif
		gamma_control.num_of_cmds,
		cmd_count,
		msd.id3);
	return cmd_count;

}

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

	if (!get_lcd_id()) {
		printk("%s: get_my_check(0)!\n",__func__);
		return;
	}

	if (get_lcd_attached() == 0) {
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return;
	}

	memset(&cmdreq, 0, sizeof(cmdreq));

	if (flag & CMD_REQ_SINGLE_TX) {
		cmdreq.flags = CMD_REQ_SINGLE_TX | CMD_CLK_CTRL | CMD_REQ_COMMIT;
	} else
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

	if (!get_lcd_id()) {
		printk("%s: get_my_check(0)!\n",__func__);
		return -ENODEV;
	}

	if (get_lcd_attached() == 0) {
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return 0;
	}

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
						show_buffer_pos, (256 - show_buffer_pos), "%02x ",
						msd.ctrl_pdata->rx_buf.data[i]);
			if (destBuffer != NULL && show_cnt < srcLength) {
					destBuffer[show_cnt] =
					msd.ctrl_pdata->rx_buf.data[i];
			}
		}
		show_buffer_pos += snprintf(show_buffer +
				show_buffer_pos, (256 - show_buffer_pos), ".");
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
	int i = 0;
	mipi_samsung_disp_send_cmd(PANEL_MTP_ENABLE, true);

	for (i = 0; i < nv_read_cmds->num_of_cmds; i++)
		nv_size += nv_read_cmds->read_size[i];

	pr_debug("nv_size= %d, nv_read_cmds->num_of_cmds = %d\n", nv_size, nv_read_cmds->num_of_cmds);

	for (i = 0; i < nv_read_cmds->num_of_cmds; i++) {
		int count = 0;
		int read_size = nv_read_cmds->read_size[i];
		int read_startoffset = nv_read_cmds->read_startoffset[i];

		count = samsung_nv_read(&(nv_read_cmds->cmd_desc[i]),
				&buffer[nv_read_cnt], read_size, pdata, read_startoffset);
		nv_read_cnt += count;
		if (count != read_size)
			pr_err("Error reading LCD NV data count(%d), read_size(%d)!!!!\n",count,read_size);
	}

	mipi_samsung_disp_send_cmd(PANEL_MTP_DISABLE, true);

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

static void mipi_samsung_manufacture_date_read(struct mdss_panel_data *pdata)
{
	char date[2];
	int year, month, day;
	int manufacture_date;

	/* Read mtp (C8h 41,42th) for manufacture date */
	mipi_samsung_read_nv_mem(pdata, &manufacture_date_cmds, date);

	year = date[0] & 0xf0;
	year >>= 4;
	year += 2011; // 0 = 2011 year
	month = date[0] & 0x0f;
	day = date[1] & 0x1f;

	manufacture_date = year * 10000 + month * 100 + day;

	pr_info("manufacture_date = (%d) - year(%d) month(%d) day(%d)\n",
		manufacture_date, year, month, day);

	msd.manufacture_date = manufacture_date;
}

static void mipi_samsung_ddi_id_read(struct mdss_panel_data *pdata)
{
	char ddi_id[5];

	/* Read mtp (D6h 1~5th) for ddi id */
	mipi_samsung_read_nv_mem(pdata, &ddi_id_cmds, ddi_id);

	memcpy(msd.ddi_id, ddi_id, 5);

	pr_info("%s : %02x %02x %02x %02x %02x\n", __func__,
		msd.ddi_id[0], msd.ddi_id[1], msd.ddi_id[2], msd.ddi_id[3], msd.ddi_id[4]);
}

static unsigned int mipi_samsung_manufacture_id(struct mdss_panel_data *pdata)
{
	struct dsi_buf *rp, *tp;

	unsigned int id = 0 ;

#if defined(CAMERA_LP)
	return 0x501401;
#endif

	if (!get_lcd_id()) {
		printk("%s: get_my_check(0)!\n",__func__);
		return -ENODEV;
	}

	if (get_lcd_attached() == 0)
	{
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return id;
	}

	if (!manufacture_id_cmds.num_of_cmds) {
		pr_err("%s : manufacture id cmds num is zero..\n",__func__);
		return 0;
	}

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

	return id;
}

static void mdss_dsi_panel_bl_ctrl(struct mdss_panel_data *pdata,
							u32 bl_level)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	/*Dont need to send backlight command if display off*/
	if (msd.mfd->resume_state != MIPI_RESUME_STATE)
		return;

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
}

static int mipi_samsung_disp_send_cmd(
		enum mipi_samsung_cmd_list cmd,
		unsigned char lock)
{
	struct dsi_cmd_desc *cmd_desc;
	int cmd_size = 0;
	int flag = 0;
#ifdef CMD_DEBUG
	int i,j;
#endif

	if (!get_lcd_id()) {
		printk("%s: get_my_check(0)!\n",__func__);
		return -ENODEV;
	}

	if (get_lcd_attached() == 0) {
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return -ENODEV;
	}

	if (lock)
		mutex_lock(&msd.lock);

	switch (cmd) {
		case PANEL_DISPLAY_ON_SEQ:
			cmd_desc = display_on_seq.cmd_desc;
			cmd_size = display_on_seq.num_of_cmds;
			break;
		case PANEL_DISPLAY_OFF_SEQ:
			cmd_desc = display_off_seq.cmd_desc;
			cmd_size = display_off_seq.num_of_cmds;
			break;
		case PANEL_DISPLAY_ON:
			cmd_desc = display_on_cmd.cmd_desc;
			cmd_size = display_on_cmd.num_of_cmds;
			break;
		case PANEL_DISPLAY_OFF:
			cmd_desc = display_off_cmd.cmd_desc;
			cmd_size = display_off_cmd.num_of_cmds;
			break;
		case PANEL_HSYNC_ON:
			cmd_desc = hsync_on_seq.cmd_desc;
			cmd_size = hsync_on_seq.num_of_cmds;
			break;
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
		case PANEL_SET_TE_OSC_B:
			cmd_desc = panel_set_te_osc_b.cmd_desc;
			cmd_size = panel_set_te_osc_b.num_of_cmds;
			break;
		case PANEL_SET_TE_RESTORE:
			cmd_desc = panel_set_te_restore.cmd_desc;
			cmd_size = panel_set_te_restore.num_of_cmds;
			break;
		case PANEL_SET_TE:
			cmd_desc = panel_set_te.cmd_desc;
			cmd_size = panel_set_te.num_of_cmds;
			break;
		case PANEL_SET_TE_1:
			cmd_desc = panel_set_te_1.cmd_desc;
			cmd_size = panel_set_te_1.num_of_cmds;
			break;
		case PANEL_SET_TE_2:
			cmd_desc = panel_set_te_2.cmd_desc;
			cmd_size = panel_set_te_2.num_of_cmds;
			break;
#endif
		case PANEL_BRIGHT_CTRL:
#if defined(CAMERA_LP)
			goto err;
#endif
#if defined(CONFIG_LCD_HMT)
			if (msd.hmt_stat.hmt_dual != HMT_OFF) {
				pr_err("hmt_dual is on!! do not set brightness..\n");
				goto err;
			}
#endif

			cmd_desc = brightness_packet;

#if defined(CONFIG_LCD_FORCE_VIDEO_MODE)
			flag = 0;
#else
			/* Single Tx use for DSI_VIDEO_MODE Only */
			if(msd.pdata->panel_info.mipi.mode == DSI_VIDEO_MODE)
				flag = CMD_REQ_SINGLE_TX;
			else
				flag = 0;
#endif
			msd.dstat.recent_bright_level = msd.dstat.bright_level;

#if defined(HBM_RE)
			if(msd.dstat.auto_brightness >= 6 && msd.dstat.bright_level == 255) {
				cmd_size = make_brightcontrol_hbm_set(msd.dstat.bright_level);
				msd.dstat.hbm_mode = 1;
			} else {
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
#if defined(ALPM_MODE)
		case PANEL_ALPM_ON:
			cmd_desc = alpm_on_seq.cmd_desc;
			cmd_size = alpm_on_seq.num_of_cmds;
			break;
		case PANEL_ALPM_OFF:
			cmd_desc = alpm_off_seq.cmd_desc;
			cmd_size = alpm_off_seq.num_of_cmds;
			break;
		case PANEL_ALPM_SET_PARTIAL_AREA:
			cmd_desc = alpm_set_partial_area_cmd.cmd_desc;
			cmd_size = alpm_set_partial_area_cmd.num_of_cmds;
			break;
		case PANEL_ALPM_SET_BL:
			cmd_desc = alpm_set_gamma_cmd.cmd_desc;
			cmd_size = alpm_set_gamma_cmd.num_of_cmds;
			break;
#endif
#if defined(CONFIG_LCD_CLASS_DEVICE) && defined(DDI_VIDEO_ENHANCE_TUNING)
		case MDNIE_ADB_TEST:
			cmd_desc = mdni_tune_cmd;
			cmd_size = ARRAY_SIZE(mdni_tune_cmd);
			break;
#endif
#if defined(CONFIG_LCD_HMT)
		case PANEL_HMT_BRIGHT:
			cmd_desc = brightness_packet;
			cmd_size = make_brightcontrol_set_hmt(msd.hmt_stat.hmt_bl_level);
			break;
		case PANEL_HMT_AID_READY_TO_FOWARD:
			cmd_desc = hmt_aid_ready_to_foward_cmd.cmd_desc;
			cmd_size = hmt_aid_ready_to_foward_cmd.num_of_cmds;
			break;
		case PANEL_DUAL_SCAN_FULL_ENABLE:
			cmd_desc = hmt_dual_scan_full_enable.cmd_desc;
			cmd_size = hmt_dual_scan_full_enable.num_of_cmds;
			break;
		case PANEL_DUAL_SCAN_DISABLE:
			cmd_desc = hmt_dual_scan_disable.cmd_desc;
			cmd_size = hmt_dual_scan_disable.num_of_cmds;
			break;
		case PANEL_HMT_AID:
			cmd_desc = hmt_aid_cmd.cmd_desc;
			cmd_size = hmt_aid_cmd.num_of_cmds;
			break;
		case PANEL_HMT_REVERSE_ENABLE:
			cmd_desc = hmt_reverse_enable.cmd_desc;
			cmd_size = hmt_reverse_enable.num_of_cmds;
			break;
		case PANEL_HMT_REVERSE_DISABLE:
			cmd_desc = hmt_reverse_disable.cmd_desc;
			cmd_size = hmt_reverse_disable.num_of_cmds;
			break;
#if 0
		case PANEL_HMT_CHANGE_PORCH:
			cmd_desc = hmt_change_porch_cmds.cmd_desc;
			cmd_size = hmt_change_porch_cmds.num_of_cmds;
			break;
		case PANEl_FORCE_500CD:
			cmd_desc = brightness_packet;
			if(msd.dstat.bright_level)
				msd.dstat.recent_bright_level = msd.dstat.bright_level;
			cmd_size = make_force_500cd_set(msd.dstat.bright_level);
			break;
		case PANEL_HMT_AID_READY_TO_REVERSE:
			cmd_desc = hmt_aid_ready_to_reverse_cmd.cmd_desc;
			cmd_size = hmt_aid_ready_to_reverse_cmd.num_of_cmds;
			break;
		case PANEL_DUAL_SCAN_HALF_ENABLE:
			cmd_desc = hmt_dual_scan_half_enable.cmd_desc;
			cmd_size = hmt_dual_scan_half_enable.num_of_cmds;
			break;
		case PANEL_HMT_CHANGE_FPS:
			cmd_desc = hmt_write_ldi_fps_cmds.cmd_desc;
			cmd_size = hmt_write_ldi_fps_cmds.num_of_cmds;
			break;
		case PANEL_HMT_HBM_ENABLE:
			if(msd.dstat.bright_level)
				msd.dstat.recent_bright_level = msd.dstat.bright_level;
			cmd_desc = brightness_packet;
			cmd_size = hmt_bright_set(true);
			break;
#endif
#endif
		default:
			pr_err("%s : unknown_command.. \n", __func__);
			goto unknown_command;
			break;
	}

	if (!cmd_size) {
		pr_err("%s : cmd_size is zero!.. \n", __func__);
		goto err;
	}

#ifdef CMD_DEBUG
	for (i = 0; i < cmd_size; i++) {
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

	pr_info("%s done..\n", __func__);

	return 0;

unknown_command:
	LCD_DEBUG("Undefined command\n");

err:
	if (lock)
		mutex_unlock(&msd.lock);

	return -EINVAL;
}

void mdss_dsi_panel_touchsensing(int enable)
{
	if(!msd.dstat.on)
	{
		pr_err("%s: No panel on! %d\n", __func__, enable);
		return;
	}

	if(enable)
		mipi_samsung_disp_send_cmd(PANEL_TOUCHSENSING_ON, true);
	else
		mipi_samsung_disp_send_cmd(PANEL_TOUCHSENSING_OFF, true);
}

static void mdss_dsi_panel_read_func(struct mdss_panel_data *pdata)
{

#if defined(CONFIG_MDNIE_LITE_TUNING)
	char temp[4];
	int	x, y;
#endif
#if defined(HBM_RE)
	char hbm_buffer[20];
#endif
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
	char read_buffer[1];
#endif

	pr_info("%s : ++\n",__func__);

#if defined(CAMERA_LP)
	return;
#endif

	if (!get_lcd_id()) {
		printk("%s: get_my_check(0)!\n",__func__);
		return;
	}

	if (get_lcd_attached() == 0) {
		pr_err("%s: get_lcd_attached(0)!\n",__func__);
		return;
	}

	mipi_samsung_manufacture_date_read(pdata);
	mipi_samsung_ddi_id_read(pdata);

#if defined(HBM_RE)
	if (msd.panel == PANEL_FHD_OCTA_S6E3FA2_CMD){
		/* Read mtp (C8h 34th ~ 40th) for HBM */
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm_read_cmds, hbm_buffer);
		memcpy(&hbm_gamma_cmds_list.cmd_desc[0].payload[1], hbm_buffer, 7);

		/* octa panel Read C8h 40th -> write B6h 21th */
		if(hbm_etc_cmds_list.cmd_desc)
			memcpy(&hbm_etc_cmds_list.cmd_desc[1].payload[21], hbm_buffer+6, 1);

		/* Read mtp (C8h 73th ~ 87th) for HBM */
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm2_read_cmds, hbm_buffer);
		memcpy(&hbm_gamma_cmds_list.cmd_desc[0].payload[7], hbm_buffer, 15);
	}
#endif
	/* Read mtp (B6h 21th) for elvss*/
	mipi_samsung_read_nv_mem(pdata, &nv_mtp_elvss_read_cmds, hbm_buffer);
	msd.dstat.elvss_value = hbm_buffer[0];

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
	if (get_lcd_ldi_info()) {
		mipi_samsung_read_nv_mem(pdata, &panel_osc_type_read_cmds, read_buffer);
		panel_set_te_restore.cmd_desc[2].payload[1] = read_buffer[0];
	}
#endif

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

	msd.dstat.is_panel_read_done = true;

	pr_info("%s : --\n",__func__);

	return;
}

static int mdss_dsi_panel_dimming_init(struct mdss_panel_data *pdata)
{
	pr_info("%s : ++\n",__func__);

#if defined(CAMERA_LP)
	return 0;
#endif

	switch (msd.panel) {
		case PANEL_FHD_OCTA_S6E3FA0:
		case PANEL_FHD_OCTA_S6E3FA0_CMD:
		case PANEL_FHD_OCTA_S6E3FA2_CMD:
		case PANEL_WQHD_OCTA_S6E3HA0_CMD:
		case PANEL_HD_OCTA_EA8064G_CMD:
		case PANEL_FHD_OCTA_EA8064G_CMD:
			msd.sdimconf = smart_S6E3_get_conf();
			break;
	}

	/* Just a safety check to ensure smart dimming data is initialised well */
	BUG_ON(msd.sdimconf == NULL);

	/* Set the mtp read buffer pointer and read the NVM value*/
	mipi_samsung_read_nv_mem(pdata, &nv_mtp_read_cmds, msd.sdimconf->mtp_buffer);

	/* Initialize smart dimming related	things here */
	/* lux_tab setting for 350cd */
	msd.sdimconf->lux_tab = &candela_map_table.lux_tab[0];
	msd.sdimconf->lux_tabsize = candela_map_table.lux_tab_size;
	msd.sdimconf->man_id = msd.manufacture_id;

	/* Just a safety check to ensure smart dimming data is initialised well */
	BUG_ON(msd.sdimconf->init == NULL);
	msd.sdimconf->init();

	msd.dstat.temperature = 20; // default temperature
	msd.dstat.elvss_need_update = 1;

	msd.dstat.is_smart_dim_loaded = true;

	pr_info("%s : --\n",__func__);

	return 0;
}

#if defined(CONFIG_LCD_HMT)
static int mdss_dsi_panel_dimming_init_HMT(struct mdss_panel_data *pdata)
{
	int size;
	pr_info("[HMT] %s : ++\n",__func__);

#if defined(CAMERA_LP)
	return 0;
#endif

	msd.hmt_stat.hmt_on = 0;
	msd.hmt_stat.hmt_bl_level = 0;
	msd.hmt_stat.hmt_dual = HMT_OFF;
	msd.hmt_stat.hmt_reverse = 0;
#if 0
	msd.hmt_stat.hmt_porch = 0;
	msd.hmt_stat.hmt_aid = 99;
#endif

	switch (msd.panel) {
		case PANEL_FHD_OCTA_S6E3FA2_CMD:
		case PANEL_WQHD_OCTA_S6E3HA0_CMD:
			msd.sdimconf_hmt = smart_S6E3_get_conf_hmt(1);
			msd.sdimconf_hmt_single = smart_S6E3_get_conf_hmt(0);
			break;
	}

	/* s dimming for dual scan */
	/* Just a safety check to ensure smart dimming data is initialised well */
	BUG_ON(msd.sdimconf_hmt == NULL);

	/* Set the mtp read buffer pointer and read the NVM value*/
	size = mipi_samsung_read_nv_mem(pdata, &nv_mtp_read_cmds, msd.sdimconf_hmt->mtp_buffer);

	/* Initialize smart dimming related	things here */
	/* lux_tab setting for 350cd */
	msd.sdimconf_hmt->lux_tab = &candela_map_table_hmt.lux_tab[0];
	msd.sdimconf_hmt->lux_tabsize = candela_map_table_hmt.lux_tab_size;
	msd.sdimconf_hmt->man_id = msd.manufacture_id;

	/* Just a safety check to ensure smart dimming data is initialised well */
	BUG_ON(msd.sdimconf_hmt->init == NULL);
	msd.sdimconf_hmt->init(1);

	pr_info("[HMT] smart dimming done for dual scan!\n");

	/* s dimming for single scan */
	BUG_ON(msd.sdimconf_hmt_single == NULL);

	size = mipi_samsung_read_nv_mem(pdata, &nv_mtp_read_cmds, msd.sdimconf_hmt_single->mtp_buffer);

	msd.sdimconf_hmt_single->lux_tab = &candela_map_table_hmt.lux_tab[0];
	msd.sdimconf_hmt_single->lux_tabsize = candela_map_table_hmt.lux_tab_size;
	msd.sdimconf_hmt_single->man_id = msd.manufacture_id;

	BUG_ON(msd.sdimconf_hmt_single->init == NULL);
	msd.sdimconf_hmt_single->init(0);

	pr_info("[HMT] smart dimming done for single scan!\n");

	msd.dstat.is_hmt_smart_dim_loaded = true;

	pr_debug("[HMT] %s : --\n",__func__);

	return 0;
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

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
	te_set_done = TE_SET_INIT;
#endif

	if(!msd.mfd)
	{
		pr_info("%s mds.mfd is null!!\n",__func__);
	} else
		pr_info("%s mds.mfd is ok!!\n",__func__);
#if defined(CONFIG_MDNIE_LITE_TUNING)
	pr_info("[%s] CONFIG_MDNIE_LITE_TUNING ok ! mdnie_lite_tuning_init called!\n",
		__func__);
	mdnie_lite_tuning_init(&msd);
#endif
	/* Set the initial state to Suspend until it is switched on */
	msd.mfd->resume_state = MIPI_SUSPEND_STATE;
	pr_info("%s:%d, Panel registered succesfully\n", __func__, __LINE__);
	return 0;
}

#if defined(CONFIG_DUAL_LCD)
struct mdss_panel_data *mdss_dsi_switching = NULL;
int IsSwitching = 0;
extern int dsi_clk_on;
#endif

static int mdss_dsi_panel_on(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct mdss_panel_info *pinfo = &msd.pdata->panel_info;

#if defined(CONFIG_LCD_HMT)
	static int is_first = 1;
#endif

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	pr_info("%s : ++\n", __func__);

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
			panel_data);

	msd.ctrl_pdata = ctrl;

	pr_debug("mdss_dsi_panel_on DSI_MODE = %d ++\n",msd.pdata->panel_info.mipi.mode);
	pr_info("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);

	if (ctrl->shared_pdata.broadcast_enable) {
		if (ctrl->ndx == DSI_CTRL_0) {
			pr_info("%s: Broadcast mode. 1st ctrl(0). return..\n",__func__);
			goto end;
		}
	}

	if (!msd.manufacture_id) {
		msd.manufacture_id = mipi_samsung_manufacture_id(pdata);

		if (set_panel_rev(msd.manufacture_id) < 0)
			pr_err("%s : can't find panel id.. \n", __func__);
	}

#if defined(ALPM_MODE)
	if (pinfo->alpm_event)
		alpm_gamma_read();
#endif

	if (!msd.dstat.is_panel_read_done)
		mdss_dsi_panel_read_func(pdata);

	if (!msd.dstat.is_smart_dim_loaded)
		mdss_dsi_panel_dimming_init(pdata);

#if defined(CONFIG_LCD_HMT)
	if (!msd.dstat.is_hmt_smart_dim_loaded)
		mdss_dsi_panel_dimming_init_HMT(pdata);
#endif
	/* Normaly the else is working for PANEL_DISP_ON_SEQ */
	if (pinfo->alpm_event) {
		if (!pinfo->alpm_event(CHECK_PREVIOUS_STATUS))
			mipi_samsung_disp_send_cmd(PANEL_DISPLAY_ON_SEQ, true);
	} else {
		mipi_samsung_disp_send_cmd(PANEL_DISPLAY_ON_SEQ, true);

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
		if (get_lcd_ldi_info()) {
			if (te_set_done == TE_SET_DONE) {
				mipi_samsung_disp_send_cmd(PANEL_SET_TE, true);
			} else
				msleep(100);
		}
#endif
	}

	if ((msd.panel == PANEL_FHD_OCTA_S6E3FA2_CMD) && (msd.id3 >= EVT0_K_fhd_REVG))
		mipi_samsung_disp_send_cmd(PANEL_HSYNC_ON, true);

	/* Recovery Mode : Set some default brightness */
	if (msd.dstat.recovery_boot_mode) {
		msd.dstat.bright_level = RECOVERY_BRIGHTNESS;
		mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
	}

	/* Init Index Values */
	msd.dstat.curr_elvss_idx = -1;
	msd.dstat.curr_acl_idx = -1;
	msd.dstat.curr_opr_idx = -1;
	msd.dstat.curr_aid_idx = -1;
	msd.dstat.on = 1;
	msd.dstat.wait_disp_on = 1;

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
	if (get_lcd_ldi_info()) {
		if (te_set_done == TE_SET_INIT)
			te_set_done = TE_SET_READY;
	}
#endif

	/*default acl off(caps on :b5 41) in on seq. */
	msd.dstat.curr_acl_idx = 0;
	msd.dstat.curr_acl_cond = 0;

	msd.mfd->resume_state = MIPI_RESUME_STATE;

	/* ALPM Mode Change */
	if (pinfo->alpm_event) {
		if (!pinfo->alpm_event(CHECK_PREVIOUS_STATUS)\
				&& pinfo->alpm_event(CHECK_CURRENT_STATUS)) {
			/* Turn On ALPM Mode */
			mipi_samsung_disp_send_cmd(PANEL_ALPM_ON, true);
			pinfo->alpm_event(STORE_CURRENT_STATUS);
			pr_info("[ALPM_DEBUG] %s: Send ALPM mode on cmds\n", __func__);
		} else if (pinfo->alpm_event(CHECK_CURRENT_STATUS)\
					&& pinfo->alpm_event(CHECK_PREVIOUS_STATUS)) {
			/* Send partial command */
			mipi_samsung_disp_send_cmd(PANEL_ALPM_SET_PARTIAL_AREA, true);
			mipi_samsung_disp_send_cmd(PANEL_ALPM_SET_BL, true);
			pr_info("[ALPM_DEBUG] %s: Send Partial update and birghtness cmds\n", __func__);
		} else if (!pinfo->alpm_event(CHECK_CURRENT_STATUS)\
					&& pinfo->alpm_event(CHECK_PREVIOUS_STATUS)) {
			/* Turn Off ALPM Mode */
			mipi_samsung_disp_send_cmd(PANEL_ALPM_OFF, true);
			pinfo->alpm_event(CLEAR_MODE_STATUS);
			pr_info("[ALPM_DEBUG] %s: Send ALPM off cmds\n", __func__);
		}
	}

#if defined(PARTIAL_UPDATE)
	if (partial_disp_range[0] || partial_disp_range[1])
		mipi_samsung_disp_send_cmd(PANEL_PARTIAL_ON, true);
#endif

	msd.dstat.bright_level = msd.dstat.recent_bright_level;
	mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);

#if defined(CONFIG_MDNIE_LITE_TUNING)
	is_negative_on();
#endif

#if defined(CONFIG_LCD_HMT)
	if (!is_first) {
		if (msd.hmt_stat.hmt_dual != HMT_OFF) {
			pr_info("hmt dual (%d), setting for HMT!\n", msd.hmt_stat.hmt_dual);
			hmt_dual_scan_update();
			hmt_bright_update();
		}
	} else
		is_first = 0;
#endif

#if defined(CONFIG_ESD_FG_RECOVERY)
	if (err_fg_working){
		err_fg_working = 0;
		esd_count++;
	}

	enable_irq(gpio_to_irq(err_fg_gpio));
	pr_info("%s: ESD enable irq (%d)esd_count(%d)\n", __func__,
		gpio_get_value(err_fg_gpio),esd_count);
#endif

end:
	pr_info("%s : --\n", __func__);
	return 0;
}

static int mdss_dsi_panel_off(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct mdss_panel_info *pinfo = &msd.pdata->panel_info;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	pr_info("%s : ++\n",__func__);

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	msd.ctrl_pdata = ctrl;

#if defined(CONFIG_ESD_FG_RECOVERY)
	if (!err_fg_working && msd.dstat.on) {
		disable_irq_nosync(gpio_to_irq(err_fg_gpio));
		cancel_work_sync(&err_fg_work);
		pr_info("%s: ESD disable irq (%d)\n", __func__, gpio_get_value(err_fg_gpio));
	}
#endif
	msd.dstat.on = 0;
	msd.mfd->resume_state = MIPI_SUSPEND_STATE;

	pr_info("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);

	if (ctrl->shared_pdata.broadcast_enable) {
		if (ctrl->ndx == DSI_CTRL_0) {
			pr_info("%s: Broadcast mode. 1st ctrl(0). return..\n",__func__);
			goto end;
		}
	}

	pr_info("DISPLAY_OFF\n");
	if (pinfo->alpm_event && pinfo->alpm_event(CHECK_CURRENT_STATUS))
			pr_info("[ALPM_DEBUG] %s: Skip to send panel off cmds\n", __func__);
	else
		mipi_samsung_disp_send_cmd(PANEL_DISPLAY_OFF_SEQ, true);

end:
	pr_info("%s : --\n",__func__);
	return 0;
}

#if defined(CONFIG_DUAL_LCD)
int samsung_switching_lcd(int flip)
{
	int ret = 0;

	msd.dstat.lcd_sel=!flip; //Change LCD SEL

	if(mdss_dsi_switching == NULL)
		return 0;

	if (get_lcd_attached() == 0)
	{
		pr_err("%s: get_lcd_attached(0)!\n",__func__);
		return -ENODEV;
	}

	LCD_DEBUG("msd.dstat.on=%d, lcd_sel=%d +\n", msd.dstat.on, msd.dstat.lcd_sel);

	if(!msd.dstat.on && dsi_clk_on) {
		int retry=5;
		while(retry>0 && !msd.dstat.on)  {
			msleep(100);
			retry--;
		}
	}

	if(msd.dstat.on && dsi_clk_on) {
		IsSwitching = 1;

		ret = mdss_dsi_panel_off(msd.pdata);
		if(ret)
			pr_err("%s: mdss_dsi_panel_off error\n",__func__);

		msd.dstat.bright_level = msd.mfd->bl_level;

		/* Init Index Values */
		msd.dstat.curr_elvss_idx = -1;
		msd.dstat.curr_acl_idx = -1;
		msd.dstat.curr_aid_idx = -1;
		msd.dstat.hbm_mode = 0;

		ret = mdss_dsi_panel_on(msd.pdata);
		if(ret)
			pr_err("%s: mdss_dsi_panel_on error\n",__func__);

		IsSwitching = 0;
	}

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

		commands->num_of_cmds = 0;

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

static int mdss_dsi_parse_panel_features(struct device_node *np,
	struct mdss_dsi_ctrl_pdata *ctrl)
{
	struct mdss_panel_info *pinfo;

	if (!np || !ctrl) {
		pr_err("%s: Invalid arguments\n", __func__);
		return -ENODEV;
	}

	pinfo = &ctrl->panel_data.panel_info;
	pinfo->cont_splash_enabled = of_property_read_bool(np,
		"qcom,cont-splash-enabled");

	pinfo->partial_update_enabled = of_property_read_bool(np,
		"qcom,partial-update-enabled");
	pr_info("%s:%d Partial update %s\n", __func__, __LINE__,
		(pinfo->partial_update_enabled ? "enabled" : "disabled"));
	if (pinfo->partial_update_enabled)
		ctrl->partial_update_fnc = mdss_dsi_panel_partial_update;

	pinfo->ulps_feature_enabled = of_property_read_bool(np,
		"qcom,ulps-enabled");
	pr_info("%s: ulps feature %s", __func__,
		(pinfo->ulps_feature_enabled ? "enabled" : "disabled"));

	return 0;
}
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

static void mdss_dsi_parse_roi_alignment(struct device_node *np,
		struct mdss_panel_info *pinfo)
{
	int len = 0;
	u32 value[6];
	struct property *data;
	data = of_find_property(np, "qcom,panel-roi-alignment", &len);
	len /= sizeof(u32);
	if (!data || (len != 6)) {
		pr_debug("%s: Panel roi alignment not found", __func__);
	} else {
		int rc = of_property_read_u32_array(np,
				"qcom,panel-roi-alignment", value, len);
		if (rc)
			pr_debug("%s: Error reading panel roi alignment values",
					__func__);
		else {
			pinfo->xstart_pix_align = value[0];
			pinfo->width_pix_align = value[1];
			pinfo->ystart_pix_align = value[2];
			pinfo->height_pix_align = value[3];
            pinfo->min_width = value[4];
			pinfo->min_height = value[5];
		}

		pr_debug("%s: ROI alignment: [%d, %d, %d, %d, %d, %d]",
				__func__, pinfo->xstart_pix_align,
				pinfo->width_pix_align, pinfo->ystart_pix_align,
				pinfo->height_pix_align, pinfo->min_width,
				pinfo->min_height);
	}
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
		pinfo->yres = (!rc ? res[1] : 480);

	rc = of_property_read_u32_array(np, "qcom,mdss-pan-size", res, 2);
	if (rc == 0) {
		pinfo->physical_width = (!rc ? res[0] : -1);
		pinfo->physical_height = (!rc ? res[1] : -1);
	}

	pr_debug("Panel Physical Width=%d, Height=%d\n",
		pinfo->physical_width,
		pinfo->physical_height);

	rc = of_property_read_u32_array(np, "qcom,mdss-pan-active-res", res, 2);
	if (rc == 0) {
		pinfo->lcdc.xres_pad =
			pinfo->xres - res[0];
		pinfo->lcdc.yres_pad =
			pinfo->yres - res[1];
	}

#if defined(CONFIG_ESD_FG_RECOVERY)
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
		}
#endif

#if defined(CONFIG_PCD_CHECK)
	pcd_gpio = of_get_named_gpio(np, "qcom,pcd-irq-gpio", 0);
	if (!gpio_is_valid(pcd_gpio)) {
		pr_err("%s:%d, pcd gpio not specified\n",
						__func__, __LINE__);
	} else {
		rc = gpio_request(pcd_gpio, "pcd_enable");
		if (rc) {
			pr_err("request pcd gpio failed, rc=%d\n",
				   rc);
			gpio_free(pcd_gpio);
			return -ENODEV;
			}
	}
#endif
	rc = of_property_read_u32(np, "qcom,mdss-pan-bpp", &tmp);
	if (rc) {
		pr_err("%s:%d, panel bpp not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	pinfo->bpp = (!rc ? tmp : 24);

	pdest = of_get_property(np, "qcom,mdss-pan-dest", NULL);
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

	rc = of_property_read_u32_array(np,	"qcom,mdss-pan-porch-values", res, 6);
	pinfo->lcdc.h_back_porch = (!rc ? res[0] : 6);
	pinfo->lcdc.h_pulse_width = (!rc ? res[1] : 2);
	pinfo->lcdc.h_front_porch = (!rc ? res[2] : 6);
	pinfo->lcdc.v_back_porch = (!rc ? res[3] : 6);
	pinfo->lcdc.v_pulse_width = (!rc ? res[4] : 2);
	pinfo->lcdc.v_front_porch = (!rc ? res[5] : 6);

	rc = of_property_read_u32(np, "qcom,mdss-pan-underflow-clr", &tmp);
	pinfo->lcdc.underflow_clr = (!rc ? tmp : 0xff);

	bl_ctrl_type = of_get_property(np, "qcom,mdss-pan-bl-ctrl", NULL);

	if ((bl_ctrl_type) && (!strncmp(bl_ctrl_type, "bl_ctrl_wled", 12))) {
		led_trigger_register_simple("bkl-trigger", &bl_led_trigger);
		pr_debug("%s: SUCCESS-> WLED TRIGGER register\n", __func__);

		pinfo->bklt_ctrl = BL_WLED;
	} else if (!strncmp(bl_ctrl_type, "bl_ctrl_pwm", 11)) {
		pinfo->bklt_ctrl = BL_PWM;

		rc = of_property_read_u32(np, "qcom,dsi-pwm-period", &tmp);
		if (rc) {
			pr_err("%s:%d, Error, dsi pwm_period\n",
					__func__, __LINE__);
			return -EINVAL;
		}
		pinfo->pwm_period = tmp;

		rc = of_property_read_u32(np, "qcom,dsi-lpg-channel", &tmp);
		if (rc) {
			pr_err("%s:%d, Error, dsi lpg channel\n",
					__func__, __LINE__);
			return -EINVAL;
		}
		pinfo->pwm_lpg_chan = tmp;

		tmp = of_get_named_gpio(np, "qcom,dsi-pwm-gpio", 0);
		pinfo->pwm_pmic_gpio =  tmp;
	} else if (!strncmp(bl_ctrl_type, "bl_ctrl_dcs_cmds", 12)) {
		pr_debug("%s: SUCCESS-> DCS CMD BACKLIGHT register\n",
			 __func__);
		pinfo->bklt_ctrl = BL_DCS_CMD;
	} else {
		pr_debug("%s: Unknown backlight control\n", __func__);
		pinfo->bklt_ctrl = UNKNOWN_CTRL;
	}

	rc = of_property_read_u32(np, "qcom,mdss-brightness-max-level", &tmp);
	pinfo->brightness_max = (!rc ? tmp : MDSS_MAX_BL_BRIGHTNESS);

	rc = of_property_read_u32_array(np,
		"qcom,mdss-pan-bl-levels", res, 2);
	pinfo->bl_min = (!rc ? res[0] : 0);
	pinfo->bl_max = (!rc ? res[1] : 255);

	rc = of_property_read_u32(np, "qcom,mdss-pan-dsi-mode", &tmp);
	pinfo->mipi.mode = (!rc ? tmp : DSI_VIDEO_MODE);

#if defined(CONFIG_LCD_FORCE_VIDEO_MODE)
	pinfo->mipi.mode = DSI_VIDEO_MODE;
#endif

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
		"qcom,mdss-pan-dsi-dst-format", &tmp);
	pinfo->mipi.dst_format =
			(!rc ? tmp : DSI_VIDEO_DST_FORMAT_RGB888);

#if defined(CONFIG_LCD_FORCE_VIDEO_MODE)
	pinfo->mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
#endif

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

	pinfo->mipi.rx_eot_ignore = of_property_read_bool(np,
		"qcom,mdss-dsi-rx-eot-ignore");
	pinfo->mipi.tx_eot_append = of_property_read_bool(np,
		"qcom,mdss-dsi-tx-eot-append");

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
		"qcom,panel-display-on-cmds", "qcom,on-cmds-dsi-state");
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->off_cmds,
		"qcom,panel-display-off-cmds", "qcom,off-cmds-dsi-state");

	mdss_samsung_parse_panel_cmd(np, &display_on_seq,
				"qcom,panel-display-on-seq");

	mdss_samsung_parse_panel_cmd(np, &hsync_on_seq,
				"qcom,panel-hsync-on-seq");

	mdss_samsung_parse_panel_cmd(np, &display_off_seq,
				"qcom,panel-display-off-seq");
	mdss_samsung_parse_panel_cmd(np, &display_on_cmd,
				"qcom,panel-display-on-cmds");
	mdss_samsung_parse_panel_cmd(np, &display_off_cmd,
				"qcom,panel-display-off-cmds");

	mdss_samsung_parse_panel_cmd(np, &test_key_enable_cmds,
				"samsung,panel-test-key-enable-cmds");
	mdss_samsung_parse_panel_cmd(np, &test_key_disable_cmds,
				"samsung,panel-test-key-disable-cmds");

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
	mdss_samsung_parse_panel_cmd(np, &panel_set_te_osc_b,
					"samsung,panel-set-te-osc-b");
	mdss_samsung_parse_panel_cmd(np, &panel_set_te_restore,
					"samsung,panel-set-te-restore");
	mdss_samsung_parse_panel_cmd(np, &panel_set_te_1,
					"samsung,panel-set-te1");
	mdss_samsung_parse_panel_cmd(np, &panel_set_te_2,
					"samsung,panel-set-te2");
	mdss_samsung_parse_panel_cmd(np, &panel_set_te,
					"samsung,panel-set-te");
	mdss_samsung_parse_panel_cmd(np, &panel_osc_type_read_cmds,
					"samsung,panel-osc-type-read");
#endif

	mdss_samsung_parse_panel_cmd(np, &nv_mtp_read_cmds,
				"samsung,panel-nv-mtp-read-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_enable_cmds,
				"samsung,panel-nv-read-enable-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_disable_cmds,
				"samsung,panel-nv-read-disable-cmds");

	mdss_samsung_parse_panel_cmd(np, &manufacture_id_cmds,
				"samsung,panel-manufacture-id-read-cmds");
	mdss_samsung_parse_panel_cmd(np, &manufacture_date_cmds,
				"samsung,panel-manufacture-date-read-cmds");
	mdss_samsung_parse_panel_cmd(np, &ddi_id_cmds,
				"samsung,panel-ddi-id-read-cmds");
	mdss_samsung_parse_panel_cmd(np, &rddpm_cmds,
					"samsung,panel-rddpm-read-cmds");

	mdss_samsung_parse_panel_cmd(np, &mtp_read_sysfs_cmds,
				"samsung,panel-mtp-read-sysfs-cmds");

	mdss_samsung_parse_panel_cmd(np, &acl_off_cmd,
				"samsung,panel-acl-off-cmds");

	mdss_samsung_parse_panel_cmd(np, &acl_cmds_list,
				"samsung,panel-acl-cmds-list");

	mdss_samsung_parse_panel_cmd(np, &aclcont_cmds_list,
				"samsung,panel-aclcont-cmds-list");

	mdss_samsung_parse_panel_cmd(np, &gamma_cmds_list,
				"samsung,panel-gamma-cmds-list");

	if ((msd.panel == PANEL_FHD_OCTA_S6E3FA2_CMD)&& (msd.id3 <= EVT1_K_fhd_REVH))
		mdss_samsung_parse_panel_cmd(np, &elvss_cmds_list,
				"samsung,panel-elvss-cmds-revG-list");
	else
		mdss_samsung_parse_panel_cmd(np, &elvss_cmds_list,
					"samsung,panel-elvss-cmds-list");

	if((msd.panel == PANEL_FHD_OCTA_S6E3FA2_CMD)&&(msd.id3 <= EVT0_K_fhd_REVF)){
		mdss_samsung_parse_panel_table(np, &aid_map_table,
					"samsung,panel-aid-map-revF-table");
		mdss_samsung_parse_panel_cmd(np, &aid_cmds_list,
					"samsung,panel-aid-cmds-revF-list");
	}else if((msd.panel == PANEL_FHD_OCTA_S6E3FA2_CMD)&&((msd.id3 == EVT1_K_fhd_REVH) || (msd.id3 == EVT0_K_fhd_REVG))){
		mdss_samsung_parse_panel_table(np, &aid_map_table,
					"samsung,panel-aid-map-revG-table");
		mdss_samsung_parse_panel_cmd(np, &aid_cmds_list,
					"samsung,panel-aid-cmds-revG-list");
	}else if((msd.panel == PANEL_HD_OCTA_EA8064G_CMD)&&(msd.id3 == EVT1_S_hd_REVD)){
		mdss_samsung_parse_panel_table(np, &aid_map_table,
					"samsung,panel-aid-map-table");
		mdss_samsung_parse_panel_cmd(np, &aid_cmds_list,
					"samsung,panel-aid-cmds-revD-list");
	}else if((msd.panel == PANEL_HD_OCTA_EA8064G_CMD)&&(msd.id3 == EVT1_S_hd_REVF)){
		mdss_samsung_parse_panel_table(np, &aid_map_table,
					"samsung,panel-aid-map-table");
		mdss_samsung_parse_panel_cmd(np, &aid_cmds_list,
					"samsung,panel-aid-cmds-revF-list");
	}else if((msd.panel == PANEL_HD_OCTA_EA8064G_CMD)&&(msd.id3 >= EVT1_S_hd_REVG)){
		mdss_samsung_parse_panel_table(np, &aid_map_table,
					"samsung,panel-aid-map-table");
		mdss_samsung_parse_panel_cmd(np, &aid_cmds_list,
					"samsung,panel-aid-cmds-revG-list");
	}else{
		mdss_samsung_parse_panel_table(np, &aid_map_table,
					"samsung,panel-aid-map-table");
		mdss_samsung_parse_panel_cmd(np, &aid_cmds_list,
					"samsung,panel-aid-cmds-list");
	}

	mdss_samsung_parse_panel_table(np, &acl_map_table,
				"samsung,panel-acl-map-table");
	mdss_dsi_parse_roi_alignment(np, pinfo);

	mdss_panel_parse_te_params(np, pinfo);

	/* Process the lux value table */
	mdss_samsung_parse_candella_lux_mapping_table(np, &candela_map_table,
				"samsung,panel-candella-mapping-table");

#if defined(CONFIG_LCD_HMT)
	mdss_samsung_parse_panel_cmd(np, &aid_cmds_list_hmt,
				"samsung,panel-aid-cmds-list-hmt");
	mdss_samsung_parse_panel_cmd(np, &aid_cmds_list_hmt_single,
				"samsung,panel-aid-cmds-list-hmt-single");
	mdss_samsung_parse_panel_table(np, &aid_map_table_hmt,
				"samsung,panel-aid-map-table-hmt");
	mdss_samsung_parse_candella_lux_mapping_table(np, &candela_map_table_hmt,
				"samsung,panel-candella-mapping-table-hmt");
#endif

#if defined(HBM_RE)
	if (msd.panel == PANEL_FHD_OCTA_S6E3FA2_CMD) {
		mdss_samsung_parse_panel_cmd(np, &nv_mtp_hbm_read_cmds,
				"samsung,panel-nv-mtp-read-hbm-cmds");
	    mdss_samsung_parse_panel_cmd(np, &nv_mtp_hbm2_read_cmds,
				"samsung,panel-nv-mtp-read-hbm2-cmds");
		mdss_samsung_parse_panel_cmd(np, &hbm_gamma_cmds_list,
					"samsung,panel-gamma-hbm-cmds-list");
		if(msd.id3 <= EVT0_K_fhd_REVF)
			mdss_samsung_parse_panel_cmd(np, &hbm_etc_cmds_list,
						"samsung,panel-etc-hbm-revE-cmds");
	} else {	/* MAGNA_PANEL*/
		mdss_samsung_parse_panel_cmd(np, &hbm_off_cmd,
					"samsung,panel-hbm-off-enable-cmds");
	}
	mdss_samsung_parse_panel_cmd(np, &hbm_etc_cmds_list,
			"samsung,panel-etc-hbm-cmds");
#endif
	mdss_samsung_parse_panel_cmd(np, &nv_mtp_elvss_read_cmds,
			"samsung,panel-nv-mtp-read-elvss-cmds");

#if defined(CONFIG_MDNIE_LITE_TUNING)
	mdss_samsung_parse_panel_cmd(np, &nv_mdnie_read_cmds,
					"samsung,panel-nv-mdnie-read-cmds");
#endif
#ifdef DEBUG_LDI_STATUS
	mdss_samsung_parse_panel_cmd(np, &ldi_debug_cmds,
				"samsung,panel-ldi-debug-read-cmds");
#endif
#if defined(TEMPERATURE_ELVSS)
	mdss_samsung_parse_panel_cmd(np, &elvss_lowtemp_cmds_list,
				"samsung,panel-elvss-lowtemp-cmds-list");

	mdss_samsung_parse_panel_cmd(np, &elvss_lowtemp2_cmds_list,
				"samsung,panel-elvss-lowtemp2-cmds-list");
#endif
#if defined(SMART_ACL)
	if ((msd.panel == PANEL_FHD_OCTA_S6E3FA2_CMD)&& (msd.id3 <= EVT1_K_fhd_REVH)){
		mdss_samsung_parse_panel_cmd(np, &smart_acl_elvss_cmds_list,
					"samsung,panel-smart-acl-elvss-cmds-revG-list");
		mdss_samsung_parse_panel_table(np, &smart_acl_elvss_map_table,
					"samsung,panel-smart-acl-elvss-map-revG-table");
   }else {
		mdss_samsung_parse_panel_cmd(np, &smart_acl_elvss_cmds_list,
					"samsung,panel-smart-acl-elvss-cmds-list");
		mdss_samsung_parse_panel_table(np, &smart_acl_elvss_map_table,
					"samsung,panel-smart-acl-elvss-map-table");
   	}
#endif
#if defined(PARTIAL_UPDATE)
	mdss_samsung_parse_panel_cmd(np, &partialdisp_on_cmd,
				"samsung,panel-ldi-partial-disp-on");
	mdss_samsung_parse_panel_cmd(np, &partialdisp_off_cmd,
				"samsung,panel-ldi-partial-disp-off");
#endif
#if defined(ALPM_MODE)
	mdss_samsung_parse_panel_cmd(np, &alpm_on_seq,
				"samsung,panel-alpm-on-seq");
	mdss_samsung_parse_panel_cmd(np, &alpm_off_seq,
				"samsung,panel-alpm-off-seq");
	mdss_samsung_parse_panel_cmd(np, &alpm_set_partial_area_cmd,
				"samsung,panel-alpm-partial-area");
	of_property_read_u32(np, "samsung,panel-alpm-partial-area-index",
					&alpm_partial_area_index);
	/* Gamma setting cmds */
	mdss_samsung_parse_panel_cmd(np, &alpm_gamma_read_cmd,
				"samsung,panel-alpm-gamma-read-cmd");
	mdss_samsung_parse_panel_cmd(np, &alpm_set_gamma_10cd_cmd,
				"samsung,panel-alpm-gamma-10cd");
	mdss_samsung_parse_panel_cmd(np, &alpm_set_gamma_60cd_cmd,
				"samsung,panel-alpm-gamma-60cd");
	mdss_samsung_parse_panel_cmd(np, &alpm_set_gamma_cmd,
				"samsung,panel-alpm-gamma-cmds");
	of_property_read_u32(np, "samsung,panel-alpm-gamma-index",
					&alpm_gamma_index);
	of_property_read_u32(np, "samsung,panel-alpm-gamma-para-index",
					&alpm_gamma_para_index);
	of_property_read_u32(np, "samsung,panel-alpm-gamma-para-length",
					&alpm_gamma_para_length);
#endif
#if defined(CONFIG_LCD_HMT)
	mdss_samsung_parse_panel_cmd(np, &hmt_aid_ready_to_foward_cmd,
				"samsung,panel-hmt-aid-ready-to-foward");
	mdss_samsung_parse_panel_cmd(np, &hmt_dual_scan_full_enable,
				"samsung,panel-hmt-dual-scan-enable");
	mdss_samsung_parse_panel_cmd(np, &hmt_dual_scan_disable,
				"samsung,panel-hmt-dual-scan-disable");
	mdss_samsung_parse_panel_cmd(np, &hmt_reverse_enable,
				"samsung,panel-hmt-reverse-enable");
	mdss_samsung_parse_panel_cmd(np, &hmt_reverse_disable,
				"samsung,panel-hmt-reverse-disable");
	mdss_samsung_parse_panel_cmd(np, &hmt_aid_cmd,
				"samsung,panel-hmt-aid");
#if 0
	mdss_samsung_parse_panel_cmd(np, &hmt_change_porch_cmds,
				"samsung,panel-hmt-porch-cmds");
	mdss_samsung_parse_panel_cmd(np, &hmt_level_enable_cmds,
				"samsung,panel-level-enable");
	mdss_samsung_parse_panel_cmd(np, &hmt_level_disable_cmds,
				"samsung,panel-level-disable");
	mdss_samsung_parse_panel_cmd(np, &force_500,
				"samsung,panel-force-500cd-cmds");
	mdss_samsung_parse_panel_cmd(np, &hmt_aid_ready_to_reverse_cmd,
				"samsung,panel-hmt-aid-ready-to-reverse");
	mdss_samsung_parse_panel_cmd(np, &hmt_dual_scan_half_enable,
				"samsung,panel-hmt-dual-scan-half-enable");
	mdss_samsung_parse_panel_cmd(np, &hmt_read_ldi_fps_cmds,
				"samsung,panel-hmt-ldi-fps-read-cmds");
	mdss_samsung_parse_panel_cmd(np, &hmt_write_ldi_fps_cmds,
				"samsung,panel-hmt-ldi-fps-write-cmds");
	mdss_samsung_parse_panel_cmd(np, &hmt_hbm_etc_cmds,
				"samsung,panel-hmt-hbm-etc-cmds");
#endif

#endif


	rc = mdss_dsi_parse_panel_features(np, ctrl_pdata);
	if (rc) {
		pr_err("%s: failed to parse panel features\n", __func__);
		goto error;
	}


	return 0;
error:
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

	if(get_lcd_id())
		msd.id3 = (get_lcd_id()&0xFF);

	while(panel_supp_cdp[i].name != NULL)	{
		if(!strcmp(panel_name,panel_supp_cdp[i].name))
			break;
		i++;
	}

	if (i < ARRAY_SIZE(panel_supp_cdp)) {
		memcpy(msd.panel_name, panel_name, MAX_PANEL_NAME_SIZE);
		msd.panel = panel_supp_cdp[i].panel_code;
		return 0;
	}
	return -EINVAL;
}

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)

struct te_fctrl_lookup_table te_fctrl_lookup_table[] = {
	{33333, 0x0 },
	{22999, 0x32},
	{22861, 0x33},
	{22726, 0x34},
	{22609, 0x35},
	{22490, 0x36},
	{22363, 0x37},
	{22237, 0x38},
	{22121, 0x39},
	{22005, 0x3A},
	{21880, 0x3B},
	{21761, 0x3C},
	{21649, 0x3D},
	{21535, 0x3E},
	{21461, 0x3F},
	{21385, 0x40},
	{21275, 0x41},
	{21170, 0x42},
	{21056, 0x43},
	{20943, 0x44},
	{20834, 0x45},
	{20723, 0x46},
	{20614, 0x47},
	{20506, 0x48},
	{20406, 0x49},
	{20311, 0x4A},
	{20206, 0x4B},
	{20099, 0x4C},
	{20003, 0x4D},
	{19903, 0x4E},
	{19808, 0x4F},
	{19717, 0x50},
	{19621, 0x51},
	{19526, 0x52},
	{19428, 0x53},
	{19331, 0x54},
	{19244, 0x55},
	{19153, 0x56},
	{19056, 0x57},
	{18963, 0x58},
	{18879, 0x59},
	{18794, 0x5A},
	{18702, 0x5B},
	{18610, 0x5C},
	{18522, 0x5D},
	{18438, 0x5E},
	{18359, 0x5F},
	{18277, 0x60},
	{18196, 0x61},
	{18116, 0x62},
	{18030, 0x63},
	{17943, 0x64},
	{17862, 0x65},
	{17783, 0x66},
	{17704, 0x67},
	{17625, 0x68},
	{17545, 0x69},
	{17464, 0x6A},
	{17384, 0x6B},
	{17306, 0x6C},
	{17231, 0x6D},
	{17158, 0x6E},
	{17089, 0x6F},
	{17016, 0x70},
	{16944, 0x71},
	{16874, 0x72},
	{16797, 0x73},
	{16717, 0x74},
	{16646, 0x75},
	{16578, 0x76},
	{16505, 0x77},
	{16433, 0x78},
	{16365, 0x79},
	{16299, 0x7A},
	{16228, 0x7B},
	{16152, 0x7C},
	{16089, 0x7D},
	{16027, 0x7E},
	{15973, 0x7F},
	{15921, 0x80},
	{15855, 0x81},
	{15787, 0x82},
	{15720, 0x83},
	{15657, 0x84},
	{15594, 0x85},
	{15532, 0x86},
	{15466, 0x87},
	{15401, 0x88},
	{15342, 0x89},
	{15283, 0x8A},
	{14286, 0x8B},
};
struct te_offset_lookup_table te_offset_lookup_table[] = {
	{19654, 0  },
	{19551, -34},
	{19448, -33},
	{19354, -32},
	{19264, -31},
	{19164, -30},
	{19063, -29},
	{18971, -28},
	{18877, -27},
	{18786, -26},
	{18701, -25},
	{18610, -24},
	{18519, -23},
	{18427, -22},
	{18335, -21},
	{18252, -20},
	{18166, -19},
	{18074, -18},
	{17986, -17},
	{17906, -16},
	{17825, -15},
	{17738, -14},
	{17650, -13},
	{17567, -12},
	{17487, -11},
	{17412, -10},
	{17334, -9 },
	{17257, -8 },
	{17182, -7 },
	{17101, -6 },
	{17018, -5 },
	{16941, -4 },
	{16866, -3 },
	{16818, -2 },
	{16518, 0  },
	{16488, 2 },
	{16414, 3 },
	{16343, 4 },
	{16273, 5 },
	{16207, 6 },
	{16139, 7 },
	{16070, 8 },
	{16005, 9 },
	{15931, 10},
	{15855, 11},
	{15788, 12},
	{15723, 13},
	{15654, 14},
	{15586, 15},
	{15521, 16},
	{15459, 17},
	{15391, 18},
	{15320, 19},
	{15259, 20},
	{15200, 21},
	{15149, 22},
	{15100, 23},
	{15037, 24},
	{14973, 25},
	{14909, 26},
	{14849, 27},
	{14790, 28},
	{14731, 29},
	{14669, 30},
	{14607, 31},
	{14551, 32},
	{14495, 33},
	{14435, 34},
};

extern int te;
extern int te_cnt;
extern struct completion te_check_comp;

static char check_te_step1(void)
{
	int ret;
	int size = sizeof(te_fctrl_lookup_table)/(sizeof(int)+sizeof(char)) - 1;
	int i;
	char fctrl;

	pr_info("[%s] ++ \n", __func__);

	INIT_COMPLETION(te_check_comp);

	te_cnt = 0;
	ret = wait_for_completion_timeout(&te_check_comp,
						msecs_to_jiffies(16 * 4));
	if (!ret) {
		pr_err("[ERROR] te_check_comp timeout!!\n");
		return 0;
	}

	if (te != 0) {
		pr_info("[%s] first TE = %d\n", __func__, te);
		for (i = 0; i < size; i++) {
			if (te < te_fctrl_lookup_table[i].te && te >= te_fctrl_lookup_table[i+1].te) {
				fctrl = te_fctrl_lookup_table[i+1].value;
				break;
			}
		}

		if (i == size) {
			pr_err("[%s] out of range ... (%d)\n", __func__, te);
			return 0;
		}

		pr_info("[%s] fctrl = %x\n", __func__, fctrl);

		panel_set_te_1.cmd_desc[3].payload[1] = fctrl;
		mipi_samsung_disp_send_cmd(PANEL_SET_TE_1, true);
	} else {
		pr_info("[%s] TE is 0..\n", __func__);
		return 0;
	}

	pr_info("[%s] -- \n", __func__);

	return fctrl;
}

static int check_te_step2(char fctrl)
{
	int ret;
	int size = sizeof(te_offset_lookup_table)/(sizeof(int)*2) - 1;
	int i;
	int offset;

	pr_info("[%s] ++ \n", __func__);

	INIT_COMPLETION(te_check_comp);

	te_cnt = 0;
	ret = wait_for_completion_timeout(&te_check_comp,
						msecs_to_jiffies(16 * 4));
	if (!ret) {
		pr_err("[ERROR] te_check_comp timeout!!\n");
		return -1;
	}

	if (te != 0) {
		pr_info("[%s] second TE = %d\n", __func__, te);
		for (i = 0; i < size; i++) {
			if (te < te_offset_lookup_table[i].te && te >= te_offset_lookup_table[i+1].te) {
				offset = te_offset_lookup_table[i+1].offset;
				break;
			}
		}
		if (i == size) {
			pr_err("[%s] out of range ... (%d)\n", __func__, te);
			return -1;
		}

		pr_info("[%s] offset = %d\n", __func__, offset);

		panel_set_te_2.cmd_desc[3].payload[1] = fctrl + offset;
		panel_set_te.cmd_desc[4].payload[1] = fctrl + offset;
		mipi_samsung_disp_send_cmd(PANEL_SET_TE_2, true);
	} else {
		pr_info("[%s] TE is 0..\n", __func__);
		return -1;
	}

	pr_info("[%s] -- \n", __func__);

	return 0;
}

static int check_te_step3(void)
{
	int ret;

	pr_info("[%s] ++ \n", __func__);

	INIT_COMPLETION(te_check_comp);

	te_cnt = 0;
	ret = wait_for_completion_timeout(&te_check_comp,
						msecs_to_jiffies(16 * 4));
	if (!ret) {
		pr_err("[ERROR] te_check_comp timeout!!\n");
		return -1;
	}

	// +5% = 17.499 , -5% = 15.832
	if (te >= (16666 * 105 / 100) || te <= (16666 * 95 / 100)) {
		pr_err("[%s] TE is not correct!! (%d) back to OSC type A..\n", __func__, te);
		return -1;
	} else
		pr_info("[%s] finals TE is (%d) - OK\n", __func__, te);

	pr_info("[%s] -- \n", __func__);

	return 0;
}
#endif

static int samsung_dsi_panel_event_handler(int event)
{
	char rddpm_buf;
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
	char fctrl;
#endif

	switch (event) {
		case MDSS_EVENT_FRAME_UPDATE:
			if(msd.dstat.wait_disp_on) {
				mipi_samsung_disp_send_cmd(PANEL_DISPLAY_ON, true);

				mipi_samsung_read_nv_mem(msd.pdata, &rddpm_cmds, &rddpm_buf);
#ifdef CONFIG_PANEL_RECOVERY
				if (rddpm_buf != 0x9c) {
					char *envp[2] = {"PANEL_ALIVE=0", NULL};
					struct device *dev = msd.mfd->fbi->dev;

					kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
					pr_err("Panel has gone bad, sending uevent - %s\n", envp[0]);
				}
#endif
				msd.dstat.wait_disp_on = 0;
				pr_info("DISPLAY_ON\n");
			}
			break;
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
		case MDSS_EVENT_TE_UPDATE:
			if (te_set_done == TE_SET_READY) {

				te_set_done = TE_SET_START;

				mipi_samsung_disp_send_cmd(PANEL_SET_TE_OSC_B, true);

				fctrl = check_te_step1();
				if (!fctrl)
					return -1;

				if (check_te_step2(fctrl) < 0)
					return -1;

				if (check_te_step3() < 0)
					return -1;

				te_set_done = TE_SET_DONE;
			}
			break;
		case MDSS_EVENT_TE_RESTORE:
			pr_info("RESTORE_TE (OSC TYPE A)\n");
			mipi_samsung_disp_send_cmd(PANEL_SET_TE_RESTORE, true);
			te_set_done = TE_SET_FAIL;
			break;
#endif
#if defined(CONFIG_MDNIE_LITE_TUNING)
		case MDSS_EVENT_MDNIE_DEFAULT_UPDATE:
			pr_info("%s : send CONFIG_MDNIE_LITE_TUNING... \n",__func__);
			is_negative_on();
			break;
#endif
		default:
			pr_err("%s : unknown event (%d)\n", __func__, event);
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
#if defined(DDI_VIDEO_ENHANCE_TUNING)
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
				}else if (cmd_pos == TUNE_SECOND_SIZE && cmd_step) {/*blocking overflow*/
					cmd_pos = 0;
					break;
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
		printk(KERN_INFO "0x%x ", mdni_tuning2[data_pos]);
	printk(KERN_INFO "\n");

	mipi_samsung_disp_send_cmd(PANEL_MTP_ENABLE, true);

	mipi_samsung_disp_send_cmd(MDNIE_ADB_TEST, true);

	mipi_samsung_disp_send_cmd(PANEL_MTP_DISABLE, true);

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

int mdnie_adb_test;
void copy_tuning_data_from_adb(char *data1, char *data2);
static ssize_t tuning_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	int ret = 0;

	ret = snprintf(buf, MAX_FILE_NAME, "Tunned File Name : %s\n",
								tuning_file);

	mdnie_adb_test = 0;

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

#if defined(CONFIG_MDNIE_LITE_TUNING)
	copy_tuning_data_from_adb(mdni_tuning2, mdni_tuning1);
#endif

	mdnie_adb_test = 1;

	return size;
}

static DEVICE_ATTR(tuning, 0664, tuning_show, tuning_store);
#endif

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

	if(msd.manufacture_id){
		snprintf(temp, 20, "SDC_%x\n",msd.manufacture_id);
	}else{
		pr_info("no manufacture id\n");

/*
		switch (msd.panel) {
			case PANEL_FHD_OCTA_S6E3FA0:
			case PANEL_FHD_OCTA_S6E3FA0_CMD:
				snprintf(temp, 20, "SDC_AMS568AT01\n");
				break;
			case PANEL_FHD_OCTA_S6E3FA2_CMD:
				snprintf(temp, 20, "SDC_AMS520BQ01\n");
				break;
			case PANEL_WQHD_OCTA_S6E3HA0_CMD:
				snprintf(temp, 20, "SDC_AMS520BR01\n");
				break;
			default :
				snprintf(temp, strnlen(msd.panel_name, 100),
									msd.panel_name);
				break;
*/
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

	snprintf(temp, sizeof(temp), "%x %x %x\n",	id1, id2, id3);
	strlcat(buf, temp, 15);
	return strnlen(buf, 15);
}

static ssize_t mipi_samsung_disp_manufacture_date_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char temp[30];

	snprintf((char *)temp, sizeof(temp), "manufacture date : %d\n", msd.manufacture_date);
	strlcat(buf, temp, 30);

	pr_info("manufacture date : %d\n", msd.manufacture_date);

	return strnlen(buf, 30);
}

static ssize_t mipi_samsung_disp_ddi_id_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char temp[30];

	snprintf((char *)temp, sizeof(temp), "ddi id : %02x %02x %02x %02x %02x\n",
		msd.ddi_id[0], msd.ddi_id[1], msd.ddi_id[2], msd.ddi_id[3], msd.ddi_id[4]);
	strlcat(buf, temp, 30);

	pr_info("%s : %02x %02x %02x %02x %02x\n", __func__,
		msd.ddi_id[0], msd.ddi_id[1], msd.ddi_id[2], msd.ddi_id[3], msd.ddi_id[4]);

	return strnlen(buf, 30);
}

#if defined(TEST_RESOLUTION)
static ssize_t mipi_samsung_disp_panel_res_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char temp[100];

	switch (msd.panel) {
		case PANEL_FHD_OCTA_S6E3FA2_CMD:
		case PANEL_FHD_OCTA_EA8064G_CMD:
		case PANEL_HD_OCTA_EA8064G_CMD: /*temp*/
			snprintf(temp, 10, "FHD\n");
			break;
		default :
			snprintf(temp, 10, "FHD\n");
			break;
	}
	strlcat(buf, temp, 100);

	return strnlen(buf, 100);

}
#endif

static ssize_t mipi_samsung_disp_acl_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n", msd.dstat.acl_on);
	pr_info("acl status: %d\n", *buf);

	return rc;
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

	msd.dstat.elvss_need_update = 1;

	if (mfd->panel_power_on) {
		if (acl_set && !(msd.dstat.acl_on||msd.dstat.siop_status)) {
			msd.dstat.acl_on = true;
			pr_info("%s: acl on  : acl %d, siop %d", __func__,
					msd.dstat.acl_on, msd.dstat.siop_status);
			mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);

		} else if (!acl_set && msd.dstat.acl_on && !msd.dstat.siop_status) {
			msd.dstat.acl_on = false;
			msd.dstat.curr_acl_idx = -1;
			msd.dstat.curr_opr_idx = -1;
			pr_info("%s: acl off : acl %d, siop %d", __func__,
					msd.dstat.acl_on, msd.dstat.siop_status);
			if(msd.dstat.auto_brightness == 6)
				pr_info("%s: HBM mode No ACL off!!", __func__);
#ifdef SMART_ACL
			/* If SMART_ACL enabled, elvss table shoud be set again */
			mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
#endif

		} else {
			msd.dstat.acl_on = acl_set;
			pr_info("%s: skip but acl update!! acl %d, siop %d", __func__,
				msd.dstat.acl_on, msd.dstat.siop_status);
		}

	}else {
		pr_info("%s: panel is off state. updating state value.\n",
			__func__);
		msd.dstat.acl_on = acl_set;
	}

	return size;
}

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
			msd.dstat.curr_opr_idx = -1;
			if(msd.dstat.auto_brightness == 6)
				pr_info("%s: HBM mode No ACL off!!", __func__);
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

static ssize_t mipi_samsung_aid_log_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc = 0;

	if (msd.dstat.is_smart_dim_loaded)
		msd.sdimconf->print_aid_log();
	else
		pr_err("smart dim is not loaded..\n");

	return rc;
}

#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
static ssize_t mipi_samsung_auto_brightness_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n",
					msd.dstat.auto_brightness);
	pr_info("auto_brightness: %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_auto_brightness_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	static int first_auto_br = 0;

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

	msd.dstat.elvss_need_update = 1;

	if (msd.mfd->resume_state == MIPI_RESUME_STATE) {
		mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);

#if defined(CONFIG_MDNIE_LITE_TUNING)
		mDNIe_Set_Mode(); // LOCAL CE tuning
#endif
		pr_info("%s %d %d\n", __func__, msd.dstat.auto_brightness, msd.dstat.bright_level);
	} else {
		pr_info("%s : panel is off state!!\n", __func__);
	}

	return size;
}

static ssize_t mipi_samsung_read_mtp_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int addr, len, start;
	char *read_buf = NULL;

	sscanf(buf, "%x %d %x" , &addr, &len, &start);

	pr_info("%x %d %x\n", addr, len, start);

	if (len > 0x100){
		pr_info("len(%d)is over\n", len);
		return size; /*let the len has limitation*/
	}

	read_buf = kmalloc(len * sizeof(char), GFP_KERNEL);

	mtp_read_sysfs_cmds.cmd_desc[0].payload[0] = addr; // addr
	mtp_read_sysfs_cmds.cmd_desc[0].payload[1] = len; // size
	mtp_read_sysfs_cmds.cmd_desc[0].payload[2] = start; // start

	mtp_read_sysfs_cmds.read_size = kzalloc(sizeof(char) * \
					mtp_read_sysfs_cmds.num_of_cmds, GFP_KERNEL);
	mtp_read_sysfs_cmds.read_startoffset = kzalloc(sizeof(char) * \
					mtp_read_sysfs_cmds.num_of_cmds, GFP_KERNEL);

	mtp_read_sysfs_cmds.read_size[0] = len;
	mtp_read_sysfs_cmds.read_startoffset[0] = start;

	pr_info("%x %x %x %x %x %x %x %x %x\n",
		mtp_read_sysfs_cmds.cmd_desc[0].dchdr.dtype,
		mtp_read_sysfs_cmds.cmd_desc[0].dchdr.last,
		mtp_read_sysfs_cmds.cmd_desc[0].dchdr.vc,
		mtp_read_sysfs_cmds.cmd_desc[0].dchdr.ack,
		mtp_read_sysfs_cmds.cmd_desc[0].dchdr.wait,
		mtp_read_sysfs_cmds.cmd_desc[0].dchdr.dlen,
		mtp_read_sysfs_cmds.cmd_desc[0].payload[0],
		mtp_read_sysfs_cmds.cmd_desc[0].payload[1],
		mtp_read_sysfs_cmds.cmd_desc[0].payload[2]);

	mipi_samsung_read_nv_mem(msd.pdata, &mtp_read_sysfs_cmds, read_buf);

	kfree(read_buf);
	return size;
}

#endif

#if defined(TEMPERATURE_ELVSS)
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

	msd.dstat.elvss_need_update = 1;

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

	if(partial_disp_range[0] > msd.pdata->panel_info.yres-1
		|| partial_disp_range[1] > msd.pdata->panel_info.yres-1)
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

#if defined(ALPM_MODE)
static ssize_t mipi_samsung_alpm_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;
	static struct mdss_panel_info *pinfo = NULL;
	int current_status = 0;

	if (msd.pdata && unlikely(!pinfo))
		pinfo = &msd.pdata->panel_info;

	if (pinfo && pinfo->alpm_event)
		current_status = (int)pinfo->alpm_event(CHECK_CURRENT_STATUS);

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n", current_status);
	pr_info("[ALPM_DEBUG] %s: alpm display range %d to %d, current status : %d \n",\
					 __func__, alpm_disp_range[0], alpm_disp_range[1], current_status);

	return rc;
}

static ssize_t mipi_samsung_alpm_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int mode = 0;
	int disp_range[2] = {0, };
	struct mdss_panel_info *pinfo = &msd.pdata->panel_info;
	struct display_status *dstat = &msd.dstat;

	sscanf(buf, "%d %d %d" , &mode, &disp_range[0], &disp_range[1]);
	pr_info("[ALPM_DEBUG] %s: mode : %d, alpm range[0] = 0x%x, range[1] = 0x%x\n",
			__func__, mode, disp_range[0], disp_range[1]);

	if(disp_range[0] > msd.pdata->panel_info.yres-1
		|| disp_range[1] > msd.pdata->panel_info.yres-1) {
		pr_err("[ALPM_DEBUG] %s: Invalid Input\n", __func__);
		return -EINVAL;
	}

	/* Update partial area for ALPM cmds if that changed */
	if (disp_range[0] || disp_range[1]) {
		if (alpm_disp_range[0] != disp_range[0]\
			 || alpm_disp_range[1] != disp_range[1]) {
			pr_info("[ALPM_DEBUG] %s: Update ALPM Partial Area\n", __func__);

			alpm_disp_range[0] = disp_range[0];
			alpm_disp_range[1] = disp_range[1];

			/*
			 * Update partial area from ALPM on cmds
			 * That will use during ALPM on or switch mode to ALPM
			 * from None ALPM
			 */
			alpm_on_seq.cmd_desc[alpm_partial_area_index].payload[1] = \
				(alpm_disp_range[0] >> 8) & 0xFF; /*select msb 1byte*/
			alpm_on_seq.cmd_desc[alpm_partial_area_index].payload[2] = \
				alpm_disp_range[0] & 0xFF;
			alpm_on_seq.cmd_desc[alpm_partial_area_index].payload[3] = \
				(alpm_disp_range[1] >> 8) & 0xFF; /*select msb 1byte*/
			alpm_on_seq.cmd_desc[alpm_partial_area_index].payload[4] = \
				alpm_disp_range[1] & 0xFF;

			/*
			 * Update partial area from ALPM partial area cmds
			 * This will use during update partial areai for every frame
			 * if ALPM or none ALPM mode already on.
			 */
			alpm_set_partial_area_cmd.cmd_desc[0].payload[1] = \
				alpm_on_seq.cmd_desc[alpm_partial_area_index].payload[1];
			alpm_set_partial_area_cmd.cmd_desc[0].payload[2] = \
				alpm_on_seq.cmd_desc[alpm_partial_area_index].payload[2];
			alpm_set_partial_area_cmd.cmd_desc[0].payload[3] = \
				alpm_on_seq.cmd_desc[alpm_partial_area_index].payload[3];
			alpm_set_partial_area_cmd.cmd_desc[0].payload[4] = \
				alpm_on_seq.cmd_desc[alpm_partial_area_index].payload[4];
		}
	}

	/*
	 * Possible mode status for Blank(0) or Unblank(1)
	 *	* Blank *
	 *		1) ALPM_MODE_ON
	 *			-> That will set during wakeup
	 *	* Unblank *
	 *		1) NORMAL_MODE_ON
	 *			-> That will send partial update commands
	 */
	if (mode == ALPM_MODE_ON) {
		alpm_store(mode);
		/*
		 * This will work if the ALPM must be on or chagne partial area
		 * if that already in the status of unblank
		 */
		if (dstat->on) {
			if (!pinfo->alpm_event(CHECK_PREVIOUS_STATUS)\
					&& pinfo->alpm_event(CHECK_CURRENT_STATUS)) {
				/* Turn On ALPM Mode */
				mipi_samsung_disp_send_cmd(PANEL_ALPM_ON, true);
				if (dstat->wait_disp_on == 0) {
					msleep(20); //wait 1 frame(more than 16ms)
					mipi_samsung_disp_send_cmd(PANEL_DISPLAY_ON, true);
				}
				pinfo->alpm_event(STORE_CURRENT_STATUS);
				pr_info("[ALPM_DEBUG] %s: Send ALPM mode on cmds\n", __func__);
			} else {
				if (disp_range[0] || disp_range[1]) {
					mipi_samsung_disp_send_cmd(PANEL_ALPM_SET_PARTIAL_AREA, true);
					pr_info("[ALPM_DEBUG] %s: Send Partial update cmds\n", __func__);
				}
			}
		}
	} else if (mode == NORMAL_MODE_ON) {
		if (dstat->on) {
			if (disp_range[0] || disp_range[1]) {
				mipi_samsung_disp_send_cmd(PANEL_ALPM_SET_PARTIAL_AREA, true);
				pr_info("[ALPM_DEBUG] :%s Send Partial update cmds\n", __func__);
			}
		}
	} else if (mode == MODE_OFF) {
		if (pinfo->alpm_event) {
			alpm_store(mode);
			if (pinfo->alpm_event(CHECK_PREVIOUS_STATUS) == ALPM_MODE_ON) {
				if (dstat->on) {
					mipi_samsung_disp_send_cmd(PANEL_ALPM_OFF, true);
					msleep(20); //wait 1 frame(more than 16ms)
					mipi_samsung_disp_send_cmd(PANEL_DISPLAY_ON, true);
					pinfo->alpm_event(CLEAR_MODE_STATUS);
				}
				pr_info("[ALPM_DEBUG] %s: Send ALPM off cmds\n", __func__);
			}
		} else {
			mipi_samsung_disp_send_cmd(PANEL_PARTIAL_OFF, true);
			pr_info("[ALPM_DEBUG] %s: Send Partial off cmds\n", __func__);
		}
	} else {
		pr_info("[ALPM_DEBUG] %s: no operation \n:", __func__);
	}

	pr_info("%s: alpm_on_seq = 0x%x\n", __func__,\
				alpm_on_seq.cmd_desc[alpm_partial_area_index].payload[1]);
	pr_info("%s: alpm_on_seq = 0x%x\n", __func__,\
				alpm_on_seq.cmd_desc[alpm_partial_area_index].payload[2]);
	pr_info("%s: alpm_on_seq = 0x%x\n", __func__,\
				alpm_on_seq.cmd_desc[alpm_partial_area_index].payload[3]);
	pr_info("%s: alpm_on_seq = 0x%x\n", __func__,\
				alpm_on_seq.cmd_desc[alpm_partial_area_index].payload[4]);

	return size;
}
static ssize_t mipi_samsung_alpm_backlight_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;
	static struct mdss_panel_info *pinfo = NULL;
	static int bl_level = 60;

	if (unlikely(!pinfo) && msd.pdata)
		pinfo = &msd.pdata->panel_info;

	if (pinfo && pinfo->alpm_event)
		bl_level = (int)pinfo->alpm_event(CHECK_BL_VALUE);

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n", bl_level);
	pr_info("[ALPM_DEBUG] %s: Brightness level : %d \n",\
					 __func__, bl_level);

	return rc;
}

static ssize_t mipi_samsung_alpm_backlight_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int bl_level = 0, mode = 0;
	static int prev_bl_level = 0;
	struct mdss_panel_info *pinfo = &msd.pdata->panel_info;
	struct display_status *dstat = &msd.dstat;

	sscanf(buf, "%d" , &bl_level);

	if (pinfo->alpm_event) {
		mode = pinfo->alpm_event(CHECK_CURRENT_STATUS);
		prev_bl_level = pinfo->alpm_event(CHECK_BL_VALUE);
		pr_info("[ALPM_DEBUG] %s: Brightness level : %d\n",
				__func__, bl_level);

		/*
		 * The bl_level must be same with STORE_BL_10CD or STORE_BL_60CD
		 * because ALPM mode only can support 10cd and 60cd
		 */
		if (bl_level != STORE_BL_10CD\
				&& bl_level != STORE_BL_60CD) {
			pr_info("[ALPM_DEBUG] %s: Invalid brightness level(%d)\n",\
						 __func__, bl_level);
			goto error;
		}

		if (bl_level != prev_bl_level) {
			pinfo->alpm_event(bl_level);

			if (bl_level == STORE_BL_10CD) {
				alpm_copy_cmds(&alpm_set_gamma_cmd.cmd_desc[1].payload[alpm_gamma_para_index],\
						&alpm_set_gamma_10cd_cmd.cmd_desc[0].payload[alpm_gamma_para_index],\
						(u8)alpm_gamma_para_length);
			} else if (bl_level == STORE_BL_60CD) {
				alpm_copy_cmds(&alpm_set_gamma_cmd.cmd_desc[1].payload[alpm_gamma_para_index],\
						&alpm_set_gamma_60cd_cmd.cmd_desc[0].payload[alpm_gamma_para_index],\
						(u8)alpm_gamma_para_length);
			}

			alpm_copy_cmds(&alpm_on_seq.cmd_desc[alpm_gamma_index].payload[alpm_gamma_para_index],\
						&alpm_set_gamma_cmd.cmd_desc[1].payload[alpm_gamma_para_index],\
						(u8)alpm_gamma_para_length);

			alpm_cmd_debug(alpm_set_gamma_cmd.cmd_desc[1].payload,\
							alpm_set_gamma_cmd.cmd_desc[1].dchdr.dlen);
			alpm_cmd_debug(alpm_on_seq.cmd_desc[alpm_gamma_index].payload,\
							alpm_on_seq.cmd_desc[alpm_gamma_index].dchdr.dlen);
			if (dstat->on) {
				/*
				 * The 10/60 cd backlight can not change in UNBLANK status
				 * Because that cause tearing effect so that is not recomended
				 */
				/* mipi_samsung_disp_send_cmd(PANEL_ALPM_SET_BL, true); */
			} else {
				/* Empty Body */
			}
		} else {
			pr_info("[ALPM_DEBUG] %s: Do not update birhgtness"\
						"(Current level %d, Requested level %d\n",\
						__func__, prev_bl_level, bl_level);
		}
	}

error:
	return size;
}

static void alpm_cmd_debug(char *src, int size)
{
#ifdef ALPM_DEBUG
	char *bp;
	int i;
	bp = src;

	pr_info("[ALPM_DEBUG] %s: ", __func__);
	for (i = 0; i < size; i++)
		printk("%x ", *bp++);
	printk("\n");
#endif
}

static void alpm_copy_cmds(char *dest, char *src, u8 size)
{
	u8 i;

	for (i = 0; i < size; i++)
		dest[i] = src[i];
}

static void alpm_gamma_read_func(void)
{
#ifdef _ALPM_GAMMA_COPY_0xBC_
	u8 i = 0, gamma_status = 0;
#endif
	static u8 gamma_read_done;

	if (gamma_read_done)
		return;

	gamma_read_done = 1;

	if (get_lcd_attached() == 0) {
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return;
	}
#ifdef _ALPM_GAMMA_COPY_0xBC_
	if (!alpm_gamma_read_cmd.num_of_cmds) {
		pr_err("%s : alpm gamma read cmd num is zero..\n",__func__);
		return;
	}

	pr_info("[ALPM_DEBUG] %s : alpm gamma read ..\n", __func__);
	mipi_samsung_disp_send_cmd(PANEL_MTP_ENABLE, true);
	mdss_dsi_cmd_receive(msd.ctrl_pdata,
		&alpm_gamma_read_cmd.cmd_desc[0],
		alpm_gamma_read_cmd.read_size[0]);

	mipi_samsung_disp_send_cmd(PANEL_MTP_DISABLE, true);
	pr_info("[ALPM_DEBUG] %s: Gamma Value(%x, %x, %x, %x, %x, %x)\n",
				__func__, msd.ctrl_pdata->rx_buf.data[0],
				msd.ctrl_pdata->rx_buf.data[1],
				msd.ctrl_pdata->rx_buf.data[2],
				msd.ctrl_pdata->rx_buf.data[3],
				msd.ctrl_pdata->rx_buf.data[4],
				msd.ctrl_pdata->rx_buf.data[5]);

	for (i = 0; i < alpm_gamma_para_length * 2; i++)
		gamma_status |= (u8)msd.ctrl_pdata->rx_buf.data[i];

	if (!gamma_status) {
		pr_err("[ALPM_DEBUG] %s : Fail to read gamma\n", __func__);
		/*
		 * Make the gamma command as null packet
		 * because there is not gamma data to send
		 */
		alpm_set_gamma_cmd.cmd_desc[1].payload[0] = 0x0;
		alpm_set_gamma_cmd.cmd_desc[1].dchdr.dlen = 1;
		alpm_on_seq.cmd_desc[alpm_gamma_index].payload[0] = 0x0;
		alpm_on_seq.cmd_desc[alpm_gamma_index].dchdr.dlen = 1;
	}

	/* Copy gamma value to 10cd, 60cd cmd set */
	alpm_copy_cmds(&alpm_set_gamma_60cd_cmd.cmd_desc[0].payload[alpm_gamma_para_index],\
					&msd.ctrl_pdata->rx_buf.data[0],\
					(u8)alpm_gamma_para_length);
	alpm_copy_cmds(&alpm_set_gamma_10cd_cmd.cmd_desc[0].payload[alpm_gamma_para_index],\
					&msd.ctrl_pdata->rx_buf.data[3],\
					(u8)alpm_gamma_para_length);
#endif
	/* Init gamma cmd for alpm on cmd*/
	alpm_copy_cmds(&alpm_on_seq.cmd_desc[alpm_gamma_index].payload[alpm_gamma_para_index],\
					&alpm_set_gamma_60cd_cmd.cmd_desc[0].payload[alpm_gamma_para_index],\
					(u8)alpm_gamma_para_length);

	/* Print out gamma cmd for debug purpose */
	alpm_cmd_debug(alpm_set_gamma_10cd_cmd.cmd_desc[0].payload,\
					alpm_set_gamma_10cd_cmd.cmd_desc[0].dchdr.dlen);
	alpm_cmd_debug(alpm_set_gamma_60cd_cmd.cmd_desc[0].payload,\
					alpm_set_gamma_60cd_cmd.cmd_desc[0].dchdr.dlen);
	alpm_cmd_debug(alpm_on_seq.cmd_desc[alpm_gamma_index].payload,
					alpm_on_seq.cmd_desc[alpm_gamma_index].dchdr.dlen);
}

#endif
#if defined(DYNAMIC_FPS_USE_TE_CTRL)
static ssize_t dynamic_fps_use_te_ctrl_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, 40,"dynamic_fps_use_te_ctrl %d \n", dynamic_fps_use_te_ctrl);

	pr_info("dynamic_fps_use_te_ctrl %d \n", dynamic_fps_use_te_ctrl);

	return rc;
}

static ssize_t dynamic_fps_use_te_ctrl_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%d" , &dynamic_fps_use_te_ctrl);
	return size;
}
#endif

static struct lcd_ops mipi_samsung_disp_props = {
	.get_power = NULL,
	.set_power = NULL,
};

#if defined(ALPM_MODE)
/*
 * This will use to enable/disable or check the status of ALPM
 * * Description for STATUS_OR_EVENT_FLAG *
 *	1) ALPM_MODE_ON
 *	2) NORMAL_MODE_ON
 *		-> Set by user using sysfs(/sys/class/lcd/panel/alpm)
 *			The value will save to current_status
 *	3) CHECK_CURRENT_STATUS
 *		-> Check current status
 *			that will return current status like ALPM_MODE_ON, NORMAL_MODE_ON or MODE_OFF
 *	4) CHECK_PREVIOUS_STATUS
 *		-> Check previous status that will return previous status like
 *			 ALPM_MODE_ON, NORMAL_MODE_ON or MODE_OFF
 *	5) STORE_CURRENT_STATUS
 *		-> Store current status to previous status because that will use
 *			for next turn on sequence
 *	6) CLEAR_MODE_STATUS
 *		-> Clear current and previous status as MODE_OFF status that can use with
 *	* Usage *
 *		Call function "alpm_event_func(STATUS_OR_EVENT_FLAG)"
 */
u8 alpm_event_func(u8 flag)
{
	static u8 current_status = 0;
	static u8 previous_status = 0;
	static u8 brightness = 60; /* Default brightness level is 60cd */
	u8 ret = 0;

	switch (flag) {
		case ALPM_MODE_ON:
			current_status = ALPM_MODE_ON;
			break;
		case NORMAL_MODE_ON:
			/*current_status = NORMAL_MODE_ON;*/
			break;
		case MODE_OFF:
			current_status = MODE_OFF;
			break;
		case CHECK_CURRENT_STATUS:
			ret = current_status;
			break;
		case CHECK_PREVIOUS_STATUS:
			ret = previous_status;
			break;
		case STORE_CURRENT_STATUS:
			previous_status = current_status;
			break;
		case CLEAR_MODE_STATUS:
			previous_status = 0;
			current_status = 0;
			break;
		case CHECK_BL_VALUE:
			ret = brightness;
			break;
		case STORE_BL_10CD:
			brightness = STORE_BL_10CD;
			break;
		case STORE_BL_60CD:
			brightness = STORE_BL_60CD;
			break;
		default:
			break;
	}

	pr_debug("[ALPM_DEBUG] current_status : %d, previous_status : %d, ret : %d\n",\
				 current_status, previous_status, ret);

	return ret;
}

static void alpm_gamma_read()
{
	struct mdss_panel_info *pinfo = &msd.pdata->panel_info;

	/* Register ALPM gamm read function */
	if(unlikely(!pinfo->alpm_gamma_read))
		pinfo->alpm_gamma_read = alpm_gamma_read_func;

	pinfo->alpm_gamma_read();
}

static void alpm_store(u8 mode)
{
	struct mdss_panel_info *pinfo = &msd.pdata->panel_info;

	/* Register ALPM event function */
	if (unlikely(!pinfo->alpm_event))
		pinfo->alpm_event = alpm_event_func;

	pinfo->alpm_event(mode);
}
#endif

#if defined(CONFIG_ESD_FG_RECOVERY)
static irqreturn_t err_fg_irq_handler(int irq, void *handle)
{
	struct msm_fb_data_type *mfd = msd.mfd;

	if(err_fg_working || !(mfd->panel_power_on) || mdss_fb_get_first_cmt_flag()) return IRQ_HANDLED;

	pr_info("%s handler + irq(%d) state(%d)", __func__, irq, gpio_get_value(err_fg_gpio));
	err_fg_working = 1;
	disable_irq_nosync(gpio_to_irq(err_fg_gpio));
	schedule_work(&err_fg_work);
	pr_info("%s : handler - state(%d)", __func__, gpio_get_value(err_fg_gpio));

	return IRQ_HANDLED;
}

void err_fg_work_func(struct work_struct *work)
{
	struct msm_fb_data_type *mfd = msd.mfd;
	struct mdss_panel_data *pdata = msd.pdata;
	char *envp[2] = {"PANEL_ALIVE=0", NULL};
	struct device *dev = msd.mfd->fbi->dev;

	pr_info("%s : start irqstate(%d)", __func__, gpio_get_value(err_fg_gpio));

	if(msd.mfd == NULL){
		pr_err("%s: mfd not initialized Skip ESD recovery\n", __func__);
		return;
	}

	if (mfd->panel_power_on) {
		pdata->panel_info.panel_dead = true; /*for cmd mode panel only*/

		kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
		pr_err("Panel has gone bad, sending uevent - %s\n", envp[0]);
	}

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

static DEVICE_ATTR(esd_check, /*S_IRUGO*/0664 , mipi_samsung_esd_check_show,\
			 mipi_samsung_esd_check_store);
#endif
#endif

#if defined(CONFIG_LCD_HMT)
int get_hmt_aid_index(int level)
{
	int ndx;

	if (level == 315)
		ndx = 0;
	else if (level == 280)
		ndx = 1;
	else if (level == 245)
		ndx = 2;
	else if (level == 210)
		ndx = 3;
	else if (level == 175)
		ndx = 4;
	else if (level == 140)
		ndx = 5;
	else if (level == 105)
		ndx = 6;
	else if (level == 70)
		ndx = 7;
	else if (level == 35)
		ndx = 8;
	else
		ndx = 8;

	pr_info("[HMT] %s : (%d)\n", __func__, ndx);

	return ndx;
}

static char hmt_aid_scan_data_fhd[HMT_AID_INDEX][2][3] = {
		/* Single Scan ,    Dual Scan */
		{{0x00,0xC2,0x08}, {0x00,0x60,0x08}}, // 0 OFF DUTY 10%
		{{0x01,0x84,0x08}, {0x00,0xC2,0x08}}, // 1 OFF DUTY 20%
		{{0x02,0x44,0x08}, {0x01,0x22,0x08}}, // 2 OFF DUTY 30%
		{{0x03,0x06,0x08}, {0x01,0x84,0x08}}, // 3 OFF DUTY 40%
		{{0x03,0xC8,0x08}, {0x01,0xE4,0x08}}, // 4 OFF DUTY 50%
		{{0x04,0x8A,0x08}, {0x02,0x44,0x08}}, // 5 OFF DUTY 60%
		{{0x05,0x4C,0x08}, {0x02,0xA6,0x08}}, // 6 OFF DUTY 70%
		{{0x06,0x0C,0x08}, {0x03,0x06,0x08}}, // 7 OFF DUTY 80%
		{{0x06,0xCE,0x08}, {0x03,0x68,0x08}}, // 8 OFF DUTY 90%
};

#define SET_HMT_AID_FOR_CMD_FHD(a,b)\
	hmt_aid_cmd.cmd_desc[2].payload[1] = \
		hmt_aid_scan_data_fhd[a][b][0];\
	hmt_aid_cmd.cmd_desc[2].payload[2] = \
		hmt_aid_scan_data_fhd[a][b][1];\
	hmt_aid_cmd.cmd_desc[2].payload[8] = \
		hmt_aid_scan_data_fhd[a][b][2];

int hmt_aid_update_fhd(void)
{
	pr_info ("== HMT AID Duty Update (fhd) Start == \n");

	SET_HMT_AID_FOR_CMD_FHD(msd.hmt_stat.hmt_aid, msd.hmt_stat.hmt_dual);

	pr_info("Foward AID, Input=%d, dual=%d, 1st=0x%x, 2nd=0x%x, 10th=0x%x\n",
		msd.hmt_stat.hmt_aid, msd.hmt_stat.hmt_dual,
		hmt_aid_cmd.cmd_desc[2].payload[1],
		hmt_aid_cmd.cmd_desc[2].payload[2],
		hmt_aid_cmd.cmd_desc[2].payload[8]);

	mipi_samsung_disp_send_cmd(PANEL_HMT_AID, true);

	pr_info ("== HMT AID Duty Update End == \n");

	return 0;
}

static char hmt_aid_scan_data_wqhd[HMT_AID_INDEX][2][3] = {
		/* Single Scan ,    Dual Scan */
		{{0x01,0x02,0x00}, {0x00,0x81,0x00}}, // 0 OFF DUTY 10%
		{{0x02,0x03,0x00}, {0x01,0x02,0x00}}, // 1 OFF DUTY 20%
		{{0x03,0x04,0x00}, {0x01,0x82,0x00}}, // 2 OFF DUTY 30%
		{{0x04,0x06,0x00}, {0x02,0x03,0x00}}, // 3 OFF DUTY 40%
		{{0x05,0x08,0x00}, {0x02,0x84,0x00}}, // 4 OFF DUTY 50%
		{{0x06,0x0C,0x00}, {0x03,0x0C,0x00}}, // 5 OFF DUTY 60%
		{{0x07,0x0C,0x00}, {0x03,0x8C,0x00}}, // 6 OFF DUTY 70%
		{{0x08,0x0D,0x00}, {0x04,0x06,0x00}}, // 7 OFF DUTY 80%
		{{0x09,0x0E,0x00}, {0x05,0x88,0x00}}, // 8 OFF DUTY 90%
};

#define SET_HMT_AID_FOR_CMD_WQHD(a,b)\
	hmt_aid_cmd.cmd_desc[2].payload[1] = \
		hmt_aid_scan_data_wqhd[a][b][0];\
	hmt_aid_cmd.cmd_desc[2].payload[2] = \
		hmt_aid_scan_data_wqhd[a][b][1];\
	hmt_aid_cmd.cmd_desc[2].payload[10] = \
		hmt_aid_scan_data_wqhd[a][b][2];

int hmt_aid_update_wqhd(void)
{
	pr_info ("== HMT AID Duty Update (wqhd) Start == \n");

	SET_HMT_AID_FOR_CMD_WQHD(msd.hmt_stat.hmt_aid, msd.hmt_stat.hmt_dual);

	pr_info("Foward AID, Input=%d, dual=%d, 1st=0x%x, 2nd=0x%x, 10th=0x%x\n",
		msd.hmt_stat.hmt_aid, msd.hmt_stat.hmt_dual,
		hmt_aid_cmd.cmd_desc[2].payload[1],
		hmt_aid_cmd.cmd_desc[2].payload[2],
		hmt_aid_cmd.cmd_desc[2].payload[10]);

	mipi_samsung_disp_send_cmd(PANEL_HMT_AID, true);

	pr_info ("== HMT AID Duty Update End == \n");

	return 0;
}

/*
 * level 80, 95, 115, 130 -> update AID + GAMMA
 * level 105, 140 -> update only AID
 */

int hmt_bright_update(void)
{
	if (msd.hmt_stat.hmt_dual != HMT_OFF) {

		if (msd.hmt_stat.hmt_reverse) {
			msd.hmt_stat.hmt_aid = HMT_AID_INDEX - 1 - msd.hmt_stat.hmt_aid; // reverse
			if (get_lcd_panel_res()) // 1 : FHD
					hmt_aid_update_fhd();
				else // 0 : WQHD
					hmt_aid_update_wqhd();

		} else {
		if (msd.hmt_stat.hmt_bl_level == 105 || msd.hmt_stat.hmt_bl_level == 140) {
			msd.hmt_stat.hmt_aid = get_hmt_aid_index(msd.hmt_stat.hmt_bl_level);
			if (get_lcd_panel_res()) // 1 : FHD
				hmt_aid_update_fhd();
			else // 0 : WQHD
				hmt_aid_update_wqhd();
		} else
			mipi_samsung_disp_send_cmd(PANEL_HMT_BRIGHT, true);
		}
	} else {
		mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
	}

	return 0;
}

int hmt_dual_scan_update(void)
{
	if (msd.hmt_stat.hmt_dual == HMT_SINGLE_SCAN) {
		pr_info("Single Scan Enable ++ \n");
		skip_te_enable = 0;
		mipi_samsung_disp_send_cmd(PANEL_DUAL_SCAN_DISABLE, true);
		pr_info("Single Scan Enable -- \n");
	} else if (msd.hmt_stat.hmt_dual == HMT_DUAL_SCAN) {
		pr_info("Dual Scan Enable ++ \n");
		skip_te_enable = 1;
		mipi_samsung_disp_send_cmd(PANEL_DUAL_SCAN_DISABLE, true);
		mipi_samsung_disp_send_cmd(PANEL_DUAL_SCAN_FULL_ENABLE, true);
		pr_info("Dual Scan Enable -- \n");
	} else if (msd.hmt_stat.hmt_dual == HMT_OFF) {
		pr_info("HMT OFF.. \n");
		skip_te_enable = 0;
		msd.hmt_stat.hmt_reverse = 0;
		mipi_samsung_disp_send_cmd(PANEL_DUAL_SCAN_DISABLE, true);
	} else {
		pr_err("Invalid input (%d).. \n", msd.hmt_stat.hmt_dual);
		pr_err("-1 : HMT off \n");
		pr_err("0 : Single Scan \n");
		pr_err("1 : Dual   Scan \n");
	}

	return 0;
}

int hmt_reverse_update(void)
{
	if (msd.hmt_stat.hmt_reverse) {
		pr_info("REVERSE ENABLE ++\n");
		mipi_samsung_disp_send_cmd(PANEL_HMT_REVERSE_ENABLE, true);
		pr_info("REVERSE ENABLE --\n");
	} else {
		pr_info("REVERSE DISABLE ++ \n");
		mipi_samsung_disp_send_cmd(PANEL_HMT_REVERSE_DISABLE, true);
		pr_info("REVERSE DISABLE -- \n");
	}

	return 0;
}

int hmt_update(void)
{
	hmt_dual_scan_update();
	hmt_reverse_update();

	return 0;
}

#if 0
static int hmt_porch_update(void)
{
	pr_info("%s: input = %d\n", __func__, msd.hmt_stat.hmt_porch);

	if (msd.hmt_stat.hmt_porch > 16 || msd.hmt_stat.hmt_porch < 0) {
		pr_err("%s : Invalid Porch Value \n", __func__);
		return -EINVAL;
	}

	if (msd.hmt_stat.hmt_dual == 0) {
		pr_err("%s : Dual Scan shoud be enabled\n", __func__);
		return -EINVAL;
	}

	if (msd.dstat.on) {
		if (msd.hmt_stat.hmt_porch > 0) {
			hmt_change_porch_cmds.cmd_desc[0].payload[3] = 0x03 + (msd.hmt_stat.hmt_porch * 0x8);
			hmt_change_porch_cmds.cmd_desc[0].payload[4] = 0x77 - msd.hmt_stat.hmt_porch;
			if(msd.hmt_stat.hmt_dual == 1) /* Half Scan */ {
				hmt_change_porch_cmds.cmd_desc[1].payload[3] = 0x03;
				hmt_change_porch_cmds.cmd_desc[1].payload[4] = 0xBF - (msd.hmt_stat.hmt_porch * 4);
			} else { /* Full Scan */
				hmt_change_porch_cmds.cmd_desc[1].payload[3] = 0x07;
				hmt_change_porch_cmds.cmd_desc[1].payload[4] = 0x7F - (msd.hmt_stat.hmt_porch * 4);
			}

		} else {
			hmt_change_porch_cmds.cmd_desc[0].payload[3] = 0x03;
			hmt_change_porch_cmds.cmd_desc[0].payload[4] = 0x77;

			if (msd.hmt_stat.hmt_dual == 1) /* Half Scan */ {
				hmt_change_porch_cmds.cmd_desc[1].payload[3] = 0x03;
				hmt_change_porch_cmds.cmd_desc[1].payload[4] = 0xBF;
			} else { /* Full Scan */
				hmt_change_porch_cmds.cmd_desc[1].payload[3] = 0x07;
				hmt_change_porch_cmds.cmd_desc[1].payload[4] = 0x7F;
			}

		}
		mipi_samsung_disp_send_cmd(PANEL_HMT_CHANGE_PORCH, true);
	} else {
		pr_info("%s : LCD is off state\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int hmt_fps_update(int input_fps)
{
	static char original_fps_buffer;
	static int original_fps=0;
	int destination_fps=0;
	pr_info("%s: input = %d\n", __func__, input_fps);

	if (!original_fps) {
		mipi_samsung_read_nv_mem(msd.pdata, &hmt_read_ldi_fps_cmds, &original_fps_buffer);
		original_fps = (unsigned int)(original_fps_buffer & 0x7F);
		pr_info("Original FPS otp value = 0x%x\n",original_fps);
	}

	if (msd.dstat.on) {
		switch (input_fps) {
			case 76 :
				pr_info("Change FPS to 76hz\n");
				destination_fps = original_fps - 0x34;
				break;
			case 70 :
				pr_info("Change FPS to 70hz\n");
				destination_fps = original_fps - 0x23;
				break;
			case 0 :
				pr_info("Change FPS to Original\n");
				destination_fps = original_fps;
				break;
			default : pr_info("Invalid FPS Input!!\n");
				return -EINVAL;
				break;
		}
		pr_info("Destination FPS register value = 0x%x\n", destination_fps);
		hmt_write_ldi_fps_cmds.cmd_desc[3].payload[1] = destination_fps;
		mipi_samsung_disp_send_cmd(PANEL_HMT_CHANGE_FPS, true);
	} else {
		pr_info("%s : LCD is off state\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static struct dsi_cmd get_hbm_level_set(int enable)
{
	struct dsi_cmd hmt_hbm_level_cmd = {0,};

	if (enable) {
		hmt_hbm_level_cmd.cmd_desc = &(hmt_level_enable_cmds.cmd_desc[0]);
		hmt_hbm_level_cmd.num_of_cmds = hmt_level_enable_cmds.num_of_cmds;
	} else {
		hmt_hbm_level_cmd.cmd_desc = &(hmt_level_disable_cmds.cmd_desc[0]);
		hmt_hbm_level_cmd.num_of_cmds = hmt_level_disable_cmds.num_of_cmds;
	}

	return hmt_hbm_level_cmd;
}

static struct dsi_cmd get_hmt_hbm_control_set(void)
{
	struct dsi_cmd hmt_hbm_etc_control = {0,};

	hmt_hbm_etc_control.cmd_desc = &(hmt_hbm_etc_cmds.cmd_desc[0]);
	hmt_hbm_etc_control.num_of_cmds = hmt_hbm_etc_cmds.num_of_cmds;

	return hmt_hbm_etc_control;
}

static int hmt_bright_set(int hbm_enable)
{
	struct dsi_cmd hmt_hbm_level_enable = {0,};
	struct dsi_cmd hmt_hbm_etc_control = {0,};
	struct dsi_cmd hmt_hbm_elvss = {0,};
	struct dsi_cmd gamma_control = {0,};
	struct dsi_cmd aid_control = {0,};
	struct dsi_cmd hmt_hbm_level_disable = {0,};

	int cmd_count = 0;
	int cd_level =0;
	int cd_idx =0;

	/* Level 2,3 Enable */
	hmt_hbm_level_enable = get_hbm_level_set(true);
	cmd_count = update_bright_packet(cmd_count, &hmt_hbm_level_enable);

	if (hbm_enable) { /* HBM Enalbe for HMT */
		/* hbm_gamma */
		gamma_control = get_hbm_gamma_control_set();
		cmd_count = update_bright_packet(cmd_count, &gamma_control);

		/* hbm_etc */
		hmt_hbm_etc_control = get_hmt_hbm_control_set();
		cmd_count = update_bright_packet(cmd_count, &hmt_hbm_etc_control);
	} else { /* HBM Disable */
		cd_level = get_candela_value(msd.dstat.bright_level);
		cd_idx = get_cmd_idx(msd.dstat.bright_level);

		/* aid/aor */
		if (msd.hmt_stat.hmt_aid == 99) {
			aid_control = get_aid_aor_control_set(cd_idx);
			cmd_count = update_bright_packet(cmd_count, &aid_control);
		}

		/* ELVSS */
		hmt_hbm_elvss = get_elvss_control_set(cd_idx);
		cmd_count = update_bright_packet(cmd_count, &hmt_hbm_elvss);

		/* GAMMA */
		gamma_control = get_gamma_control_set(cd_level);
		cmd_count = update_bright_packet(cmd_count, &gamma_control);
	}

	/* Level 2,3 Disable */
	hmt_hbm_level_disable = get_hbm_level_set(false);
	cmd_count = update_bright_packet(cmd_count, &hmt_hbm_level_disable);

	return cmd_count;

}

static ssize_t mipi_samsung_hmt_change_fps_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t mipi_samsung_hmt_change_fps_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;

	sscanf(buf, "%d " , &input);
	hmt_fps_update(input);

	return size;
}

static ssize_t mipi_samsung_hmt_change_porch_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n", msd.hmt_stat.hmt_porch);
	pr_info("[HMT] porch input : %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_hmt_change_porch_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%d " , &msd.hmt_stat.hmt_porch);
	pr_info("[HMT] %s: input = %d\n", __func__, msd.hmt_stat.hmt_porch);

	if (msd.hmt_stat.hmt_on)
		hmt_porch_update();
	else
		pr_err("[HMT] %s : hmt is not on!\n", __func__);

	return size;
}

static ssize_t hmt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t hmt_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	static int hmt_cmd, hmt_cmd_index;

	sscanf(buf, "%d %d" , &hmt_cmd, &hmt_cmd_index);
	pr_info("%s: hmt cmd = %d, hmt cmd index = %d\n", __func__, hmt_cmd, hmt_cmd_index);

	switch (hmt_cmd) {
		case HMT_ENABLE:
			break;
		case HMT_DUAL_SCAN:
			msd.hmt_stat.hmt_dual = hmt_cmd_index;
			hmt_dual_scan_update();
			break;
		case HMT_AID:
			msd.hmt_stat.hmt_aid = hmt_cmd_index;
			hmt_aid_update();
			break;
#if 0
		case HMT_HBM:
			if(hmt_cmd_index)
				mipi_samsung_disp_send_cmd(PANEl_FORCE_500CD, true);
			break;
		case HMT_FPS:
			hmt_fps_update(hmt_cmd_index);
			break;
#endif
		default:
			pr_info("Invalid Input Value = %d \n",hmt_cmd);
			return -EINVAL;
			break;
	}
	return size;
}

static ssize_t mipi_samsung_force_500cd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, 40, "partial display range %d to %d \n",
				partial_disp_range[0], partial_disp_range[1]);

	pr_info("partial display range %d to %d \n",
				partial_disp_range[0], partial_disp_range[1]);

	return rc;
}

static ssize_t mipi_samsung_force_500cd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;

	sscanf(buf, "%d " , &input);
	pr_info("%s: input = %d\n", __func__, input);

	if (msd.dstat.on) {
		if (input) {
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

static ssize_t mipi_samsung_hmt_aid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n", msd.hmt_stat.hmt_aid);
	pr_info("[HMT] aid input : %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_hmt_aid_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;

	sscanf(buf, "%d" , &input);
	pr_info("[HMT] %s: input (%d)\n", __func__, input);

	if (!msd.dstat.on) {
		pr_err("[HMT] panel is off!\n");
		return size;
	}

	if (msd.hmt_stat.hmt_dual == HMT_OFF) {
		pr_info("[HMT] hmt is off!\n");
		return size;
	}

	if
(input >= HMT_AID_INDEX) {
		pr_err("[HMT] out(%d) of index(%d)!\n", input, HMT_AID_INDEX);
		return size;
	}

	if (msd.hmt_stat.hmt_aid == input) {
		pr_err("[HMT] hmt aid already %d!\n", msd.hmt_stat.hmt_aid);
		return size;
	}

	msd.hmt_stat.hmt_aid = input;

	if (msd.hmt_stat.hmt_reverse)
		hmt_bright_update();
	else
		pr_err("[HMT] %s : not reserve mode!\n", __func__);

	return size;
}

static ssize_t mipi_samsung_hmt_bright_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n", msd.hmt_stat.hmt_bl_level);
	pr_info("[HMT] hmt bright : %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_hmt_bright_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;

	sscanf(buf, "%d" , &input);
	pr_info("[HMT] %s: input (%d)\n", __func__, input);

	if (!msd.dstat.on) {
		pr_err("[HMT] panel is off!\n");
		return size;
	}

	if (msd.hmt_stat.hmt_dual == HMT_OFF) {
		pr_info("[HMT] hmt is off!\n");
		return size;
	}

	if (msd.hmt_stat.hmt_bl_level == input) {
		pr_err("[HMT] hmt bright already %d!\n", msd.hmt_stat.hmt_bl_level);
		return size;
	}

	msd.hmt_stat.hmt_bl_level = input;
	hmt_bright_update();

	return size;
}

static ssize_t mipi_samsung_hmt_dual_scan_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n", msd.hmt_stat.hmt_dual);
	pr_info("[HMT] dual scan input : %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_hmt_dual_scan_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;

	sscanf(buf, "%d" , &input);
	pr_info("[HMT] %s: input (%d)\n", __func__, input);

	if (!msd.dstat.on) {
		pr_err("[HMT] panel is off!\n");
		return size;
	}

	if (msd.hmt_stat.hmt_dual == input) {
		pr_info("[HMT] hmt already %s scan mode!\n", msd.hmt_stat.hmt_dual?"dual":"single");
		return size;
	}

	msd.hmt_stat.hmt_dual = input;
	//hmt_dual_scan_update();
	hmt_update();
	hmt_bright_update();

	return size;
}

static ssize_t mipi_samsung_hmt_reverse_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, PAGE_SIZE, "%d\n", msd.hmt_stat.hmt_dual);
	pr_info("[HMT] reverse input : %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_hmt_reverse_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;

	sscanf(buf, "%d" , &input);
	pr_info("[HMT] %s: input (%d)\n", __func__, input);

	if (!msd.dstat.on) {
		pr_err("[HMT] panel is off!\n");
		return size;
	}

	if (msd.hmt_stat.hmt_dual == HMT_OFF) {
		pr_info("[HMT] hmt is off!\n");
		return size;
	}

	msd.hmt_stat.hmt_reverse = input;
	//hmt_reverse_update();
	hmt_update();
	hmt_bright_update();

	return size;
}

#endif

static DEVICE_ATTR(lcd_power, S_IRUGO | S_IWUSR,
			mipi_samsung_disp_get_power,
			mipi_samsung_disp_set_power);
static DEVICE_ATTR(lcd_type, S_IRUGO,
			mipi_samsung_disp_lcdtype_show,
			NULL);
static DEVICE_ATTR(window_type, S_IRUGO,
			mipi_samsung_disp_windowtype_show, NULL);
static DEVICE_ATTR(manufacture_date, S_IRUGO,
			mipi_samsung_disp_manufacture_date_show, NULL);
static DEVICE_ATTR(ddi_id, S_IRUGO,
			mipi_samsung_disp_ddi_id_show, NULL);
static DEVICE_ATTR(power_reduce, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_disp_acl_show,
			mipi_samsung_disp_acl_store);
static DEVICE_ATTR(siop_enable, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_disp_siop_show,
			mipi_samsung_disp_siop_store);
static DEVICE_ATTR(read_mtp, S_IRUGO | S_IWUSR | S_IWGRP,
			NULL,
			mipi_samsung_read_mtp_store);
#if defined(TEMPERATURE_ELVSS)
static DEVICE_ATTR(temperature, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_temperature_show,
			mipi_samsung_temperature_store);
static DEVICE_ATTR(aid_log, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_aid_log_show,
			NULL);
#endif
#if defined(PARTIAL_UPDATE)
static DEVICE_ATTR(partial_disp, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_disp_partial_disp_show,
			mipi_samsung_disp_partial_disp_store);
#endif
#if defined(ALPM_MODE)
static DEVICE_ATTR(alpm, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_alpm_show,
			mipi_samsung_alpm_store);
static DEVICE_ATTR(alpm_backlight, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_alpm_backlight_show,
			mipi_samsung_alpm_backlight_store);
#endif
#if defined(DYNAMIC_FPS_USE_TE_CTRL)
static DEVICE_ATTR(dynamic_fps_use_te, S_IRUGO | S_IWUSR | S_IWGRP,
			dynamic_fps_use_te_ctrl_show,
			dynamic_fps_use_te_ctrl_store);
#endif
#if defined(TEST_RESOLUTION)
static DEVICE_ATTR(panel_res, S_IRUGO,
			mipi_samsung_disp_panel_res_show,
			NULL);
#endif
#if defined(CONFIG_LCD_HMT)
static DEVICE_ATTR(hmt_bright, S_IRUGO | S_IWUGO | S_IXUGO,
			mipi_samsung_hmt_bright_show,
			mipi_samsung_hmt_bright_store);
static DEVICE_ATTR(hmt_dual_scan, S_IRUGO | S_IWUGO | S_IXUGO,
			mipi_samsung_hmt_dual_scan_show,
			mipi_samsung_hmt_dual_scan_store);
static DEVICE_ATTR(hmt_reverse, S_IRUGO | S_IWUGO | S_IXUGO,
			mipi_samsung_hmt_reverse_show,
			mipi_samsung_hmt_reverse_store);
static DEVICE_ATTR(hmt_aid, S_IRUGO | S_IWUGO | S_IXUGO,
			mipi_samsung_hmt_aid_show,
			mipi_samsung_hmt_aid_store);
#if 0
static DEVICE_ATTR(hmt_change_porch, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_hmt_change_porch_show,
			mipi_samsung_hmt_change_porch_store);
static DEVICE_ATTR(hmt, S_IRUGO | S_IWUSR | S_IWGRP,
			hmt_show, hmt_store);
static DEVICE_ATTR(force_500cd, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_force_500cd_show,
			mipi_samsung_force_500cd_store);

static DEVICE_ATTR(hmt_change_fps, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_hmt_change_fps_show,
			mipi_samsung_hmt_change_fps_store);
#endif
#endif

static struct attribute *panel_sysfs_attributes[] = {
	&dev_attr_lcd_power.attr,
	&dev_attr_lcd_type.attr,
	&dev_attr_window_type.attr,
	&dev_attr_manufacture_date.attr,
	&dev_attr_ddi_id.attr,
	&dev_attr_power_reduce.attr,
	&dev_attr_siop_enable.attr,
	&dev_attr_aid_log.attr,
	&dev_attr_read_mtp.attr,
#if defined(TEMPERATURE_ELVSS)
	&dev_attr_temperature.attr,
#endif
#if defined(PARTIAL_UPDATE)
	&dev_attr_partial_disp.attr,
#endif
#if defined(ALPM_MODE)
	&dev_attr_alpm.attr,
	&dev_attr_alpm_backlight.attr,
#endif
#if defined(DYNAMIC_FPS_USE_TE_CTRL)
	&dev_attr_dynamic_fps_use_te.attr,
#endif
#if defined(TEST_RESOLUTION)
	&dev_attr_panel_res.attr,
#endif
#if defined(CONFIG_LCD_HMT)
	&dev_attr_hmt_bright.attr,
	&dev_attr_hmt_dual_scan.attr,
	&dev_attr_hmt_reverse.attr,
	&dev_attr_hmt_aid.attr,
#if 0
	&dev_attr_hmt_change_fps.attr,
	&dev_attr_hmt_change_porch.attr,
	&dev_attr_hmt.attr,
	&dev_attr_force_500cd.attr,
#endif

#endif
	NULL
};
static const struct attribute_group panel_sysfs_group = {
	.attrs = panel_sysfs_attributes,
};

#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
static DEVICE_ATTR(auto_brightness, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_auto_brightness_show,
			mipi_samsung_auto_brightness_store);

static struct attribute *bl_sysfs_attributes[] = {
	&dev_attr_auto_brightness.attr,
	NULL
};
static const struct attribute_group bl_sysfs_group = {
	.attrs = bl_sysfs_attributes,
};
#endif
static int sysfs_enable;
static int mdss_samsung_create_sysfs(void)
{
	int rc = 0;

	struct lcd_device *lcd_device;
#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
	struct backlight_device *bd = NULL;
#endif

	/* sysfs creat func should be called one time in dual dsi mode */
	if (sysfs_enable)
		return 0;

	lcd_device = lcd_device_register("panel", NULL, NULL,
					&mipi_samsung_disp_props);

	if (IS_ERR(lcd_device)) {
		rc = PTR_ERR(lcd_device);
		pr_err("Failed to register lcd device..\n");
		return rc;
	}

	sysfs_remove_file(&lcd_device->dev.kobj,
		&dev_attr_lcd_power.attr);

	rc = sysfs_create_group(&lcd_device->dev.kobj, &panel_sysfs_group);
	if (rc) {
		pr_err("Failed to create panel sysfs group..\n");
		sysfs_remove_group(&lcd_device->dev.kobj, &panel_sysfs_group);
		return rc;
	}

#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
	bd = backlight_device_register("panel", &lcd_device->dev,
						NULL, NULL, NULL);
	if (IS_ERR(bd)) {
		rc = PTR_ERR(bd);
		pr_err("backlight : failed to register device\n");
		return rc;
	}

	rc = sysfs_create_group(&bd->dev.kobj, &bl_sysfs_group);
	if (rc) {
		pr_err("Failed to create backlight sysfs group..\n");
		sysfs_remove_group(&bd->dev.kobj, &bl_sysfs_group);
		return rc;
	}
#endif

#if defined(CONFIG_ESD_FG_RECOVERY)
#ifdef ESD_DEBUG
	rc= sysfs_create_file(&lcd_device->dev.kobj,
							&dev_attr_esd_check.attr);
	if (rc) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_esd_check.attr.name);
	}
#endif
#endif

#if defined(DDI_VIDEO_ENHANCE_TUNING)
     rc = sysfs_create_file(&lcd_device->dev.kobj,
		  	 &dev_attr_tuning.attr);
    if (rc) {
	   pr_err("sysfs create fail-%s\n",
				   dev_attr_tuning.attr.name);
	   return rc;
	}
#endif

	sysfs_enable = 1;

	pr_info("%s: done!! \n", __func__);

	return rc;
}
#endif

int mdss_dsi_panel_init(struct device_node *node, struct mdss_dsi_ctrl_pdata *ctrl_pdata,
				bool cmd_cfg_cont_splash)
{
	int rc = 0;
	static const char *panel_name;
	bool cont_splash_enabled;
	struct mdss_panel_info *pinfo;
	struct mdss_debug_data *mdd =
				(struct mdss_debug_data *)((mdss_mdp_get_mdata())->debug_inf.debug_data);

	pr_debug("%s: ++ \n", __func__);


	if (!node || !ctrl_pdata) {
		pr_err("%s: Invalid arguments\n", __func__);
		return -ENODEV;
	}

	pinfo = &ctrl_pdata->panel_data.panel_info;

	panel_name = of_get_property(node, "label", NULL);
	if (!panel_name)
		pr_info("%s:%d, panel name not specified\n",
						__func__, __LINE__);
	else
		pr_info("%s: Panel Name = %s\n", __func__, panel_name);

	if (is_panel_supported(panel_name))
		LCD_DEBUG("Panel : %s is not supported:",panel_name);

	rc = mdss_panel_parse_dt(node, ctrl_pdata);
	if (rc)
		return rc;

	if (cmd_cfg_cont_splash)
		cont_splash_enabled = of_property_read_bool(node,
				"qcom,cont-splash-enabled");
	else
		cont_splash_enabled = false;

	if (!cmd_cfg_cont_splash)
	pinfo->cont_splash_enabled = false;

	pr_info("%s: Continuous splash %s", __func__,
	pinfo->cont_splash_enabled ? "enabled" : "disabled");

#if defined(CONFIG_ESD_FG_RECOVERY)
	INIT_WORK(&err_fg_work, err_fg_work_func);

	rc = request_threaded_irq(gpio_to_irq(err_fg_gpio),
		NULL, err_fg_irq_handler,  IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "esd_detect", NULL);
	if (rc) {
		pr_err("%s : Failed to request_irq.:ret=%d", __func__, rc);
	}

	disable_irq(gpio_to_irq(err_fg_gpio));
#endif

#if defined(CONFIG_PCD_CHECK)
	pr_info("%s : start pcd irqstate(%d)", __func__, gpio_get_value(pcd_gpio));
#endif
	ctrl_pdata->on = mdss_dsi_panel_on;
	ctrl_pdata->off = mdss_dsi_panel_off;
	ctrl_pdata->event_handler = samsung_dsi_panel_event_handler;
	ctrl_pdata->bl_fnc = mdss_dsi_panel_bl_ctrl;
	ctrl_pdata->panel_reset = mdss_dsi_panel_reset;
	ctrl_pdata->panel_extra_power = mdss_dsi_extra_power;
	ctrl_pdata->registered = mdss_dsi_panel_registered;
	ctrl_pdata->dimming_init = mdss_dsi_panel_dimming_init;
	ctrl_pdata->panel_blank = mdss_dsi_panel_blank;
	ctrl_pdata->bklt_ctrl = ctrl_pdata->panel_data.panel_info.bklt_ctrl;
	ctrl_pdata->panel_data.set_backlight = mdss_dsi_panel_bl_ctrl;

	mutex_init(&msd.lock);
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_CMD_WQHD_PT_PANEL)
	init_completion(&te_check_comp);
#endif
	msd.dstat.on = 0;
	msd.dstat.recent_bright_level = 255;

#if defined(CONFIG_LCD_CLASS_DEVICE)
	rc = mdss_samsung_create_sysfs();
	if (rc) {
		pr_err("Failed to create sysfs for lcd driver..\n");
		return rc;
	}
#endif

#if defined(CONFIG_MDNIE_LITE_TUNING)
	pr_info("[%s] CONFIG_MDNIE_LITE_TUNING ok ! init class called!\n",
		__func__);
	init_mdnie_class();
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
	msd.early_suspend.suspend = mipi_samsung_disp_early_suspend;
	msd.early_suspend.resume = mipi_samsung_disp_late_resume;
	msd.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN-1;
	register_early_suspend(&msd.early_suspend);
#endif

	if (get_lcd_attached() == 0) {
		pr_err("%s: get_lcd_attached(0)!.. set VIDEO mode..\n",__func__);

		pinfo->type = MIPI_VIDEO_PANEL;
		pinfo->mipi.mode = DSI_VIDEO_MODE;
		pinfo->mipi.traffic_mode = DSI_BURST_MODE;
		pinfo->mipi.bllp_power_stop = true;
		pinfo->mipi.te_sel = 0;
		pinfo->mipi.vsync_enable = 0;
		pinfo->mipi.hw_vsync_mode = 0;
		pinfo->mipi.force_clk_lane_hs = true;
		pinfo->mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
		pinfo->cont_splash_enabled = false;
		pinfo->esd_check_enabled = false;
		ctrl_pdata->on_cmds.link_state = DSI_LP_MODE;
		ctrl_pdata->off_cmds.link_state = DSI_LP_MODE;
		mdd->logd.xlog_enable = 0;

		set_samsung_lcd_attached(1);
	}

	pr_info("%s : --\n",__func__);

	return 0;
}

int get_lcd_id(void)
{
	return lcd_id;
}
EXPORT_SYMBOL(get_lcd_id);

int get_lcd_ldi_info(void)
{
	return lcd_lsi_ldi;
}
EXPORT_SYMBOL(get_lcd_ldi_info);

int get_samsung_lcd_attached(void)
{
	return lcd_attached;

}

void set_samsung_lcd_attached(int set)
{
	lcd_attached = set;
}

EXPORT_SYMBOL(get_samsung_lcd_attached);

static int __init lcd_panel_info(char *mode)
{
	/*
	*	1 is K FHD LSI LDI (default)
	*	0 is K FHD MAGNA LDI
	*/

	if (strncmp(mode, "1", 1) == 0)
		lcd_lsi_ldi = 1;
	else
		lcd_lsi_ldi = 0;

#if defined(CONFIG_LCD_FORCE_VIDEO_MODE)
	lcd_lsi_ldi = 1;
#endif

	pr_info("%s %s", __func__, lcd_lsi_ldi == 1 ?
				"k FHD LSI" : "k FHD MAG");
	return 1;
}
__setup("Panelres=", lcd_panel_info);

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
	lcd_id = 0x501401;/*revF*/
#endif

	pr_info( "%s: LCD_ID = 0x%X, lcd_attached =%d", __func__,lcd_id, lcd_attached);

	return 0;
}

__setup( "lcd_id=0x", get_lcd_id_cmdline );

MODULE_DESCRIPTION("Samsung DSI panel driver");
MODULE_AUTHOR("JYHONG <jyhong@samsung.com>/ KRCHO <kr0124.cho@samsung.com>");
MODULE_LICENSE("GPL");
