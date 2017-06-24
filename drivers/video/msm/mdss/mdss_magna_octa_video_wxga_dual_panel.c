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

#include "mdss_dsi.h"
#include "mdss_fb.h"

#include "mdss_magna_octa_video_wxga_dual_panel.h"

#if defined(CONFIG_MDNIE_LITE_TUNING)
#include "mdnie_lite_tuning.h"
#endif

#define DDI_VIDEO_ENHANCE_TUNING
#if defined(DDI_VIDEO_ENHANCE_TUNING)
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#endif

#include <asm/system_info.h>

//#define SMART_ACL
#define HBM_RE
static struct dsi_buf dsi_panel_tx_buf;
static struct dsi_buf dsi_panel_rx_buf;

static struct dsi_cmd display_on_seq;
static struct dsi_cmd display_off_seq;
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
static struct candella_lux_map candela_map_table;

static struct dsi_cmd acl_cmds_list;
static struct dsi_cmd aclcont_cmds_list;
static struct dsi_cmd gamma_cmds_list;
static struct dsi_cmd elvss_cmds_list;

static struct cmd_map aid_map_table;
static struct dsi_cmd aid_cmds_list;

static struct dsi_cmd temp_cmd;

#if defined(HBM_RE)
static struct dsi_cmd nv_mtp_hbm_read_cmds;
static struct dsi_cmd nv_mtp_hbm2_read_cmds;

static struct dsi_cmd hbm_gamma_cmds_list_main;
static struct dsi_cmd hbm_gamma_cmds_list_sub;

static struct dsi_cmd hbm_etc_cmds_list_main;
static struct dsi_cmd hbm_etc_cmds_list_sub;
static struct dsi_cmd hbm_etc_cmds_list;
#endif
#if defined(CONFIG_MDNIE_LITE_TUNING)
static struct dsi_cmd nv_mdnie_read_cmds;
#endif
#ifdef DEBUG_LDI_STATUS
static struct dsi_cmd ldi_debug_cmds;
#endif

static struct cmd_map smart_acl_elvss_map_table;

static struct dsi_cmd nv_mtp_elvss_read_cmds;

static struct dsi_cmd elvss_lowtemp_cmds_list;
static struct dsi_cmd elvss_common_cmds_list;

static struct dsi_cmd smart_elvss_hightemp_cmds_list;
static struct dsi_cmd smart_elvss_lowtemp_cmds_list;

static struct dsi_cmd smart_acl_hightemp_cmds_list;
static struct dsi_cmd smart_acl_lowtemp_cmds_list;

static struct mipi_samsung_driver_data msd;

static struct panel_hrev panel_supp_cdp[]= {
	{"samsung amoled wxga video mode dsi EA8061V panel", PANEL_WXGA_OCTA_EA8061V_VIDEO_DUAL},
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

static int lcd_attached = 1;
static int lcd_pcd = 1;
static int lcd_sel_lk = 0;
static int lcd_id = 0;
static int first_booting = 1;
static int force_switching = 0;

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
struct work_struct  err_fg_work;
static int err_fg_gpio = 0;
static int esd_count = 0;
int err_fg_working = 0;
#endif

extern unsigned int system_rev;
static int c_lcd_sel;

#if defined(CONFIG_LCD_CLASS_DEVICE) && defined(DDI_VIDEO_ENHANCE_TUNING)
#define MAX_FILE_NAME 128
#define TUNING_FILE_PATH "/sdcard/"
#define TUNE_FIRST_SIZE 5
#define TUNE_SECOND_SIZE 92

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

static int set_panel_rev(unsigned int id)
{
	switch (id & 0xFF) {
		case 0x40:
			pr_info("%s : 0x40 EA8061V_REV_B \n",__func__);
			msd.id3 = EA8061V_REV_B;
			break;
		case 0x41:
			pr_info("%s : 0x41 EA8061V_REV_C \n",__func__);
			msd.id3 = EA8061V_REV_C;
			break;
		case 0x42:
			pr_info("%s : 0x42 EVT0_K_fhd_REVG \n",__func__);
			msd.id3 = EA8061V_REV_E;
			break;
		case 0x43:
			pr_info("%s : 0x43 EVT1_K_fhd_REVH \n",__func__);
			msd.id3 = EA8061V_REV_F;
			break;
		default:
			if(get_lcd_id())
				msd.id3 = (get_lcd_id() & 0xFF);
			else
				msd.id3 = EA8061V_REV_F;

			pr_info("%s : can't find panel id..but set as (0x%x) \n", __func__,msd.id3 );
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

void mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	pr_info("%s: enable(%d) ndx(%d) gpio(%d)\n",
			   __func__,enable, ctrl_pdata->ndx, ctrl_pdata->rst_gpio);

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
		usleep_range(10000, 10000);
	}
}

static char caset[] = {0x2a, 0x00, 0x00, 0x03, 0x00};	/* DTYPE_DCS_LWRITE */
static char paset[] = {0x2b, 0x00, 0x00, 0x05, 0x00};	/* DTYPE_DCS_LWRITE */
static struct dsi_cmd_desc partial_update_enable_cmd[] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(caset)}, caset},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(paset)}, paset},
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

/*
	This function takes acl_map_table and uses cd_idx,
	to get the index of the command in elvss command list.
*/

static struct dsi_cmd get_elvss_control_set(int cd_idx)
{
	struct dsi_cmd elvss_control = {0,};
	int cmd_idx = 0;

	pr_info("%s acl(%d), temp(%d)\n",
			__func__, msd.dstat.acl_on, msd.dstat.temperature);

	if (!smart_acl_elvss_map_table.size || !(cd_idx < smart_acl_elvss_map_table.size) ||
			!smart_acl_elvss_map_table.size ||
			!(cd_idx < smart_acl_elvss_map_table.size)) {
		pr_err("%s failed mapping elvss table\n",__func__);
		goto end;
	}

	cmd_idx = smart_acl_elvss_map_table.cmd_idx[cd_idx];

	if (msd.dstat.temperature > 0)
		elvss_common_cmds_list.cmd_desc[0].payload[1] = 0x19; // B8
	else if (msd.dstat.temperature > -20)
		elvss_common_cmds_list.cmd_desc[0].payload[1] = 0x00; // B8
	else
		elvss_common_cmds_list.cmd_desc[0].payload[1] = 0x94; // B8

	if(msd.dstat.acl_on || msd.dstat.siop_status) {
		elvss_common_cmds_list.cmd_desc[1].payload[1] = 0x48;

		if (msd.dstat.temperature > 0)
			elvss_common_cmds_list.cmd_desc[1].payload[2] =
				smart_elvss_hightemp_cmds_list.cmd_desc[cmd_idx].payload[2];
		else
			elvss_common_cmds_list.cmd_desc[1].payload[2] =
				smart_elvss_lowtemp_cmds_list.cmd_desc[cmd_idx].payload[2];

		pr_debug("ELVSS for smart elvss cd_idx=%d, cmd_idx=%d\n", cd_idx, cmd_idx);
	} else {
		elvss_common_cmds_list.cmd_desc[1].payload[1] = 0x58;

		if (msd.dstat.temperature > 0)
			elvss_common_cmds_list.cmd_desc[1].payload[2] =
				smart_acl_hightemp_cmds_list.cmd_desc[cmd_idx].payload[2];
		else
			elvss_common_cmds_list.cmd_desc[1].payload[2] =
				smart_acl_lowtemp_cmds_list.cmd_desc[cmd_idx].payload[2];

		pr_debug("ELVSS for smart acl cd_idx=%d, cmd_idx=%d\n", cd_idx, cmd_idx);
	}

	elvss_control.cmd_desc = &(elvss_common_cmds_list.cmd_desc[0]);
	elvss_control.num_of_cmds = elvss_common_cmds_list.num_of_cmds;

	msd.dstat.curr_elvss_idx = cmd_idx;

end:
	return elvss_control;

}

static struct dsi_cmd get_elvss_lowtemp_control_set(int cd_idx)
{
	struct dsi_cmd elvss_lowtemp_control = {0,};
	int cmd_idx = 0;

	cmd_idx = smart_acl_elvss_map_table.cmd_idx[cd_idx];

	if(!c_lcd_sel)
		elvss_lowtemp_cmds_list.cmd_desc[1].payload[1] =
			msd.dstat.elvss_value_main - 0x0A;
	else
		elvss_lowtemp_cmds_list.cmd_desc[1].payload[1] =
			msd.dstat.elvss_value_sub - 0x0A;

	elvss_lowtemp_control.cmd_desc = &(elvss_lowtemp_cmds_list.cmd_desc[0]);
	elvss_lowtemp_control.num_of_cmds = elvss_lowtemp_cmds_list.num_of_cmds;

	return elvss_lowtemp_control;
}

static struct dsi_cmd get_gamma_control_set(int candella)
{
	struct dsi_cmd gamma_control = {0,};
	/* Just a safety check to ensure smart dimming data is initialised well */
	if (!c_lcd_sel) {
		BUG_ON(msd.sdimconf_main->generate_gamma == NULL);
		msd.sdimconf_main->generate_gamma(candella, &gamma_cmds_list.cmd_desc[0].payload[1]);
	} else {
		BUG_ON(msd.sdimconf_sub->generate_gamma == NULL);
		msd.sdimconf_sub->generate_gamma(candella, &gamma_cmds_list.cmd_desc[0].payload[1]);
	}

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

	return cmd_count;
}

#if defined(HBM_RE)
static struct dsi_cmd get_hbm_etc_control_set(void)
{
	struct dsi_cmd etc_hbm_control = {0,};

	if (!c_lcd_sel) {
		if (msd.dstat.acl_on || msd.dstat.siop_status)
			hbm_etc_cmds_list_main.cmd_desc[1].payload[1] = 0x48;
		else
			hbm_etc_cmds_list_main.cmd_desc[1].payload[1] = 0x58;

		/* Get the command desc */
		etc_hbm_control.cmd_desc = &(hbm_etc_cmds_list_main.cmd_desc[0]);
		etc_hbm_control.num_of_cmds = hbm_etc_cmds_list_main.num_of_cmds;
	} else {
		if (msd.dstat.acl_on || msd.dstat.siop_status)
			hbm_etc_cmds_list_sub.cmd_desc[1].payload[1] = 0x48;
		else
			hbm_etc_cmds_list_sub.cmd_desc[1].payload[1] = 0x58;

		/* Get the command desc */
		etc_hbm_control.cmd_desc = &(hbm_etc_cmds_list_sub.cmd_desc[0]);
		etc_hbm_control.num_of_cmds = hbm_etc_cmds_list_sub.num_of_cmds;
	}

	return etc_hbm_control;
}

static struct dsi_cmd get_hbm_gamma_control_set(void)
{
	struct dsi_cmd gamma_hbm_control = {0,};

	/* Get the command desc */
	if (!c_lcd_sel) {
		gamma_hbm_control.cmd_desc = &(hbm_gamma_cmds_list_main.cmd_desc[0]);
		gamma_hbm_control.num_of_cmds = hbm_gamma_cmds_list_main.num_of_cmds;
	} else {
		gamma_hbm_control.cmd_desc = &(hbm_gamma_cmds_list_sub.cmd_desc[0]);
		gamma_hbm_control.num_of_cmds = hbm_gamma_cmds_list_sub.num_of_cmds;
	}

	return gamma_hbm_control;
}

static int make_brightcontrol_hbm_set(int bl_level)
{

	struct dsi_cmd hbm_etc_control = {0,};
	struct dsi_cmd gamma_control = {0,};
	//struct dsi_cmd elvss_control = {0,};
	struct dsi_cmd testKey = {0, 0, 0, 0, 0};

	int cmd_count = 0, cd_idx = 0;

	cd_idx = get_cmd_idx(bl_level);

	if (msd.dstat.hbm_mode) {
		pr_err("%s : already hbm mode! return .. \n", __func__);
		return 0;
	}

	testKey = get_testKey_set(1);
	cmd_count = update_bright_packet(cmd_count, &testKey);

	/*gamma*/
	gamma_control = get_hbm_gamma_control_set();
	cmd_count = update_bright_packet(cmd_count, &gamma_control);


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


static int make_brightcontrol_set(int bl_level)
{
	struct dsi_cmd aid_control = {0,};
	struct dsi_cmd acl_control = {0,};
	struct dsi_cmd acl_on_cont = {0,};
	struct dsi_cmd acl_off_cont = {0,};
	struct dsi_cmd elvss_control = {0,};
	struct dsi_cmd elvss_lowtemp_control = {0,};
	struct dsi_cmd gamma_control = {0,};
	struct dsi_cmd testKey = {0,};

	int cmd_count = 0, cd_idx = 0, cd_level =0;

	cd_idx = get_cmd_idx(bl_level);
	cd_level = get_candela_value(bl_level);

	testKey = get_testKey_set(1);
	cmd_count = update_bright_packet(cmd_count, &testKey);

	/* aid/aor */
	aid_control = get_aid_aor_control_set(cd_idx);
	cmd_count = update_bright_packet(cmd_count, &aid_control);

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

	if (msd.dstat.temperature <= -20) {
		elvss_lowtemp_control = get_elvss_lowtemp_control_set(cd_idx);
		cmd_count = update_bright_packet(cmd_count, &elvss_lowtemp_control);
	}

	/*gamma*/
	gamma_control = get_gamma_control_set(cd_level);
	cmd_count = update_bright_packet(cmd_count, &gamma_control);

	testKey = get_testKey_set(0);
	cmd_count = update_bright_packet(cmd_count, &testKey);


	LCD_DEBUG("bright_level: %d, candela_idx: %d( %d cd ), "\
		"cmd_count(aid,acl,elvss,temperature,gamma)::(%d,%d,%d,%d)%d,id3(0x%x)\n",
		msd.dstat.bright_level, cd_idx, cd_level,
		aid_control.num_of_cmds,
		msd.dstat.acl_on | msd.dstat.siop_status,
		elvss_control.num_of_cmds,
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

	if (get_lcd_attached() == 0) {
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return 0;
	}

    memset(&cmdreq, 0, sizeof(cmdreq));
    cmdreq.cmds = cmd;
    cmdreq.cmds_cnt = 1;
    cmdreq.flags = CMD_REQ_RX | CMD_REQ_COMMIT;
    cmdreq.rlen = rlen;
	cmdreq.rbuf = ctrl->rx_buf.data;
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
		//show_buffer_pos += snprintf(show_buffer +
		//		show_buffer_pos, 256, ".");
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

	if (!c_lcd_sel)
		msd.manufacture_date_main = manufacture_date;
	else
		msd.manufacture_date_sub = manufacture_date;
}
#if 0
static void mipi_samsung_ddi_id_read(struct mdss_panel_data *pdata)
{
	char ddi_id[5];

	/* Read mtp (D6h 1~5th) for ddi id */
	mipi_samsung_read_nv_mem(pdata, &ddi_id_cmds, ddi_id);

	memcpy(msd.ddi_id, ddi_id, 5);

	pr_info("%s : %02x %02x %02x %02x %02x\n", __func__,
		msd.ddi_id[0], msd.ddi_id[1], msd.ddi_id[2], msd.ddi_id[3], msd.ddi_id[4]);
}
#endif
static unsigned int mipi_samsung_manufacture_id(struct mdss_panel_data *pdata)
{
	struct dsi_buf *rp, *tp;

	unsigned int id = 0 ;

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
	if (msd.mfd->resume_state != MIPI_RESUME_STATE) {
		msd.dstat.recent_bright_level = bl_level;
		pr_err("%s : suspend state! save bl level(%d)\n",
			__func__, bl_level);
		return;
	}

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

//#define CMD_DEBUG

int mipi_samsung_disp_send_cmd(
		enum mipi_samsung_cmd_list cmd,
		unsigned char lock)
{
	struct dsi_cmd_desc *cmd_desc;
	int cmd_size = 0;
	int flag = 0;
#ifdef CMD_DEBUG
	int i,j;
#endif

	if (get_lcd_attached() == 0) {
		printk("%s: get_lcd_attached(0)!\n",__func__);
		return -ENODEV;
	}

	if (lock)
		mutex_lock(&msd.lock);

	switch (cmd) {
		case PANEL_DISPLAY_ON_SEQ:
			flag = CMD_REQ_SINGLE_TX;
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
		case PANEL_BRIGHT_CTRL:
			cmd_desc = brightness_packet;

			/* Single Tx use for DSI_VIDEO_MODE Only */
			if(msd.pdata->panel_info.mipi.mode == DSI_VIDEO_MODE)
				flag = CMD_REQ_SINGLE_TX;
			else
				flag = 0;

			if (msd.dstat.bright_level)
				msd.dstat.recent_bright_level = msd.dstat.bright_level;
#if defined(HBM_RE)
			if (msd.dstat.auto_brightness >= 6 && msd.dstat.bright_level == 255) {
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
		case PANEL_ACL_OFF:
			cmd_desc = acl_off_cmd.cmd_desc;
			cmd_size = acl_off_cmd.num_of_cmds;
			break;
#if defined(CONFIG_LCD_CLASS_DEVICE) && defined(DDI_VIDEO_ENHANCE_TUNING)
		case MDNIE_ADB_TEST:
			cmd_desc = mdni_tune_cmd;
			cmd_size = ARRAY_SIZE(mdni_tune_cmd);
			break;
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


static void mdss_dsi_panel_read_func(struct mdss_panel_data *pdata)
{
#if defined(HBM_RE)
	char read_buffer[20];
#endif

	if (get_lcd_attached() == 0) {
		pr_err("%s: get_lcd_attached(0)!\n",__func__);
		return;
	}

#if defined(HBM_RE)
	if (!c_lcd_sel) {

		pr_info("%s : main octa ++\n",__func__);

		mipi_samsung_manufacture_date_read(pdata);

		/* Read mtp (C8h 34th ~ 40th) for HBM */
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm_read_cmds, read_buffer);
		memcpy(&hbm_gamma_cmds_list_main.cmd_desc[0].payload[1], read_buffer, 7);

		/* octa panel Read C8h 40th -> write B6h 1st */
		if(hbm_etc_cmds_list_main.cmd_desc)
			memcpy(&hbm_etc_cmds_list_main.cmd_desc[3].payload[1], read_buffer+6, 1);

		/* Read mtp (C8h 43th ~ 57th) for HBM */
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm2_read_cmds, read_buffer);
		memcpy(&hbm_gamma_cmds_list_main.cmd_desc[0].payload[7], read_buffer, 15);

		/* Read mtp (B6h 4th) for elvss*/
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_elvss_read_cmds, read_buffer);
		msd.dstat.elvss_value_main = read_buffer[0];

		pr_info("%s : main octa --\n",__func__);
	} else {
		pr_info("%s : sub octa ++\n",__func__);

		mipi_samsung_manufacture_date_read(pdata);

		/* Read mtp (C8h 34th ~ 40th) for HBM */
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm_read_cmds, read_buffer);
		memcpy(&hbm_gamma_cmds_list_sub.cmd_desc[0].payload[1], read_buffer, 7);

		/* octa panel Read C8h 40th -> write B6h 1st */
		if(hbm_etc_cmds_list_sub.cmd_desc)
			memcpy(&hbm_etc_cmds_list_sub.cmd_desc[3].payload[1], read_buffer+6, 1);

		/* Read mtp (C8h 43th ~ 57th) for HBM */
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_hbm2_read_cmds, read_buffer);
		memcpy(&hbm_gamma_cmds_list_sub.cmd_desc[0].payload[7], read_buffer, 15);

		/* Read mtp (B6h 4th) for elvss*/
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_elvss_read_cmds, read_buffer);
		msd.dstat.elvss_value_sub = read_buffer[0];

		pr_info("%s : sub octa --\n",__func__);
	}
#endif

	return;
}

static int mdss_dsi_panel_dimming_init(struct mdss_panel_data *pdata)
{
	if (!c_lcd_sel) {
		pr_info("%s : main octa ++\n",__func__);

		switch (msd.panel) {
			case PANEL_WXGA_OCTA_EA8061V_VIDEO_DUAL:
				msd.sdimconf_main = smart_S6E8FA0_get_conf_main();
				break;
		}

		/* Just a safety check to ensure smart dimming data is initialised well */
		BUG_ON(msd.sdimconf_main == NULL);

		/* Set the mtp read buffer pointer and read the NVM value*/
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_read_cmds, msd.sdimconf_main->mtp_buffer);

		/* Initialize smart dimming related	things here */
		/* lux_tab setting for 350cd */
		msd.sdimconf_main->lux_tab = &candela_map_table.lux_tab[0];
		msd.sdimconf_main->lux_tabsize = candela_map_table.lux_tab_size;
		msd.sdimconf_main->man_id = msd.manufacture_id;

		/* Just a safety check to ensure smart dimming data is initialised well */
		BUG_ON(msd.sdimconf_main->init == NULL);
		msd.sdimconf_main->init();

		pr_info("%s : main octa --\n",__func__);
	} else {
		pr_info("%s : sub octa ++\n",__func__);

		switch (msd.panel) {
			case PANEL_WXGA_OCTA_EA8061V_VIDEO_DUAL:
				msd.sdimconf_sub = smart_S6E8FA0_get_conf_sub();
				break;
		}

		/* Just a safety check to ensure smart dimming data is initialised well */
		BUG_ON(msd.sdimconf_sub == NULL);

		/* Set the mtp read buffer pointer and read the NVM value*/
		mipi_samsung_read_nv_mem(pdata, &nv_mtp_read_cmds, msd.sdimconf_sub->mtp_buffer);

		/* Initialize smart dimming related	things here */
		/* lux_tab setting for 350cd */
		msd.sdimconf_sub->lux_tab = &candela_map_table.lux_tab[0];
		msd.sdimconf_sub->lux_tabsize = candela_map_table.lux_tab_size;
		msd.sdimconf_sub->man_id = msd.manufacture_id;

		/* Just a safety check to ensure smart dimming data is initialised well */
		BUG_ON(msd.sdimconf_sub->init == NULL);
		msd.sdimconf_sub->init();

		pr_info("%s : sub octa --\n",__func__);
	}

	return 0;
}

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

	if(!msd.mfd)
		pr_info("%s : msd.mfd is null!!\n", __func__);
	else
		pr_info("%s : msd.mfd is ok!!\n", __func__);

#if defined(CONFIG_MDNIE_LITE_TUNING)
	pr_info("%s : CONFIG_MDNIE_LITE_TUNING ok ! mdnie_lite_tuning_init called!\n",
		__func__);
	mdnie_lite_tuning_init(&msd);
#endif
	/* Set the initial state to Suspend until it is switched on */
	msd.mfd->resume_state = MIPI_SUSPEND_STATE;
	pr_info("%s:%d, Panel registered succesfully\n", __func__, __LINE__);
	return 0;
}

static void main_panel_on(struct mdss_panel_data *pdata)
{
#if defined(CONFIG_MDNIE_LITE_TUNING)
	char temp[4];
#endif

	pr_info("%s : ++\n", __func__);

	if (!msd.dstat.main_panel_read_done) {
		mdss_dsi_panel_read_func(pdata);
		msd.dstat.main_panel_read_done = true;
	}

	if (!msd.dstat.main_smart_dim_loaded) {
		mdss_dsi_panel_dimming_init(pdata);
		msd.dstat.main_smart_dim_loaded = true;
	}

#if defined(CONFIG_MDNIE_LITE_TUNING)
	if (!msd.dstat.coordinate_main_loaded) {
		mipi_samsung_read_nv_mem(pdata, &nv_mdnie_read_cmds, temp);
		msd.dstat.coordinate_x =  temp[0] << 8 | temp[1];	/* X */
		msd.dstat.coordinate_y = temp[2] << 8 | temp[3];	/* Y */
		coordinate_tunning(msd.dstat.coordinate_x, msd.dstat.coordinate_y);

		msd.dstat.coordinate_main_loaded = 1;
	}
#endif

	pr_info("%s : --\n", __func__);

	return;
}

static void sub_panel_on(struct mdss_panel_data *pdata)
{
#if defined(CONFIG_MDNIE_LITE_TUNING)
	char temp[4];
#endif

	pr_info("%s : ++\n", __func__);

	if (!msd.dstat.sub_panel_read_done) {
		mdss_dsi_panel_read_func(pdata);
		msd.dstat.sub_panel_read_done = true;
	}

	if (!msd.dstat.sub_smart_dim_loaded) {
		mdss_dsi_panel_dimming_init(pdata);
		msd.dstat.sub_smart_dim_loaded = true;
	}

#if defined(CONFIG_MDNIE_LITE_TUNING)
	if (!msd.dstat.coordinate_sub_loaded) {
		mipi_samsung_read_nv_mem(pdata, &nv_mdnie_read_cmds, temp);
		msd.dstat.coordinate_x =  temp[0] << 8 | temp[1];	/* X */
		msd.dstat.coordinate_y = temp[2] << 8 | temp[3];	/* Y */
		coordinate_tunning(msd.dstat.coordinate_x, msd.dstat.coordinate_y);

		msd.dstat.coordinate_sub_loaded = 1;
	}
#endif

	pr_info("%s : --\n", __func__);

	return;
}

static int mdss_dsi_panel_on(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
//	struct mdss_panel_info *pinfo = &msd.pdata->panel_info;
	char rddpm_buf;
	int temp;

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

	c_lcd_sel = gpio_get_value(msd.ctrl_pdata->lcd_sel_gpio);
	usleep_range(2000, 2000);
	LCD_DEBUG("flip [%s]\n", c_lcd_sel ? "CLOSE" : "OPEN");

	mipi_samsung_disp_send_cmd(PANEL_MTP_ENABLE, true);

	if (!msd.manufacture_id) {
		msd.manufacture_id = mipi_samsung_manufacture_id(pdata);

		if (set_panel_rev(msd.manufacture_id) < 0)
			pr_err("%s : can't find panel id.. \n", __func__);
	}

	if (first_booting) {
		if (lcd_sel_lk != msd.dstat.lcd_sel) {
			temp = msd.dstat.lcd_sel;
			msd.dstat.lcd_sel = lcd_sel_lk;
		}
	}

	if (!c_lcd_sel)
		main_panel_on(pdata);
	else
		sub_panel_on(pdata);

	if (first_booting)
		msd.dstat.lcd_sel = temp;

	mipi_samsung_disp_send_cmd(PANEL_DISPLAY_ON_SEQ, true);

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

	/*default acl off(caps on :b5 41) in on seq. */
	msd.dstat.curr_acl_idx = 0;
	msd.dstat.curr_acl_cond = 0;

	msd.mfd->resume_state = MIPI_RESUME_STATE;

	// to prevent splash during wakeup
	if (msd.dstat.recent_bright_level) {
		msd.dstat.bright_level = msd.dstat.recent_bright_level;
		mipi_samsung_disp_send_cmd(PANEL_BRIGHT_CTRL, true);
	}

#if defined(CONFIG_MDNIE_LITE_TUNING)
	is_negative_on();
#endif

	mipi_samsung_disp_send_cmd(PANEL_DISPLAY_ON, true);
	pr_info("DISPLAY_ON\n");

	usleep_range(2000, 2000);
	mipi_samsung_read_nv_mem(msd.pdata, &rddpm_cmds, &rddpm_buf);

	if (rddpm_buf != 0x9c) {
		panic("dcdc boost up fail(%x), hw_rev(%d)", rddpm_buf, system_rev);
	}
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	enable_irq(gpio_to_irq(err_fg_gpio));
	pr_info("%s: ESD enable irq (%d)\n", __func__,gpio_get_value(err_fg_gpio));
#endif

	if (first_booting) {
		first_booting = 0;
		if (lcd_sel_lk != msd.dstat.lcd_sel) {
			pr_info("%s : lcd_sel_lk(%d) , msd.dstat.lcd_sel(%d)\n", __func__, lcd_sel_lk, msd.dstat.lcd_sel);
			force_switching = 1;
			samsung_switching_lcd(0);
			force_switching = 0;
		}
	}

	pr_info("%s : --\n", __func__);

	return 0;
}

static int mdss_dsi_panel_off(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	int lcd_sel;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	if (!msd.dstat.on) {
		pr_err("%s: already panel off !!\n", __func__);
		return -1;
	}

	pr_info("%s : ++\n",__func__);

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	msd.ctrl_pdata = ctrl;
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

	lcd_sel = gpio_get_value(msd.ctrl_pdata->lcd_sel_gpio);
	usleep_range(2000, 2000);
	LCD_DEBUG("flip [%s]\n", lcd_sel ? "CLOSE" : "OPEN");

	mipi_samsung_disp_send_cmd(PANEL_DISPLAY_OFF_SEQ, true);

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	if (!err_fg_working) {
		disable_irq_nosync(gpio_to_irq(err_fg_gpio));
		cancel_work_sync(&err_fg_work);
		pr_info("%s: ESD disable irq (%d)\n", __func__, gpio_get_value(err_fg_gpio));
	}
#endif

end:
	pr_info("%s : --\n",__func__);
	return 0;
}


#if defined(CONFIG_DUAL_LCD)

/*
 *  flip 1 = CLOSE (SUB) / flip 0 = OPEN (MAIN)
 *  lcd_sel 1 = CLOSE / lcd_sel 0 = OEPN
 */
int flip;
extern int poweroff_charging;

int samsung_switching_lcd(int flip_hall)
{
	int ret = 0;
	struct msm_fb_data_type *mfd = msd.mfd;

	char *envp[2] = {"PANEL_ALIVE=0", NULL};
	struct device *dev = msd.mfd->fbi->dev;

	if (!force_switching) {
		flip = flip_hall;

		if (msd.dstat.lcd_sel == flip) {
			pr_err("%s : same flip input lcd_sel(%d) flip(%d)\n",
				__func__, msd.dstat.lcd_sel, flip);
			goto err;
		}
	} else {
		pr_err("%s : force switching!!\n", __func__);
	}

	if (!msd.dstat.on || !mfd->panel_power_on) {
		pr_err("%s: already panel off (%d) (%d) !!\n",
			__func__, msd.dstat.on, mfd->panel_power_on);
	}

	pr_info("%s ++ flip(%d) poweroff_charging(%d)\n", __func__, flip, poweroff_charging);

	msd.dstat.lcd_sel = flip;

	if (!poweroff_charging) {
		kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
		pr_info("%s : sending uevent - %s\n", __func__, envp[0]);
	} else {
		mutex_lock(&mfd->power_state);
		mutex_lock(&mfd->ctx_lock);

		ret = mdss_dsi_panel_off(msd.pdata);
		if (ret) {
			pr_err("%s: mdss_dsi_panel_off error\n",__func__);
			goto err_unlock;
		}

		gpio_set_value(msd.ctrl_pdata->lcd_sel_gpio, msd.dstat.lcd_sel);
		usleep_range(2000, 2000);
		LCD_DEBUG("flip [%s]\n", msd.dstat.lcd_sel ? "CLOSE" : "OPEN");

		msd.dstat.bright_level = msd.mfd->bl_level;

		/* Init Index Values */
		msd.dstat.curr_elvss_idx = -1;
		msd.dstat.curr_acl_idx = -1;
		msd.dstat.curr_aid_idx = -1;
		msd.dstat.hbm_mode = 0;

		ret = mdss_dsi_panel_on(msd.pdata);
		if(ret)
			pr_err("%s: mdss_dsi_panel_on error\n",__func__);
err_unlock:
		mutex_unlock(&mfd->ctx_lock);
		mutex_unlock(&mfd->power_state);
	}
err:
	pr_info("%s -- \n", __func__);

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
	struct mdss_dsi_phy_ctrl phy_params;

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
		"qcom,mdss-pan-te-sel", &tmp);
	pinfo->mipi.te_sel =
		(!rc ? tmp : 1);

	rc = of_property_read_u32(np,
		"qcom,mdss-pan-dsi-dst-format", &tmp);
	pinfo->mipi.dst_format =
			(!rc ? tmp : DSI_VIDEO_DST_FORMAT_RGB888);

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

	for (i = 0; i < 4; i++)
		phy_params.ctrl[i] = 0;

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

	rc = of_property_read_u32(np, "qcom,mdss-pan-clk-rate", &tmp);
		pinfo->clk_rate = (!rc ? tmp : 0);

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	err_fg_gpio = of_get_named_gpio(np, "qcom,esd-irq-gpio", 0);
	if (!gpio_is_valid(err_fg_gpio)) {
		pr_err("%s:%d, esd gpio not specified\n",
						__func__, __LINE__);
	} else {
		rc = gpio_request(err_fg_gpio, "esd_enable");
		if (rc) {
			pr_err("request esd gpio failed, rc=%d\n", rc);
			gpio_free(err_fg_gpio);
			return -ENODEV;
			}
		}
#endif
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->on_cmds,
		"qcom,panel-display-on-cmds", "qcom,on-cmds-dsi-state");

	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->off_cmds,
		"qcom,panel-display-off-cmds", "qcom,off-cmds-dsi-state");

	mdss_samsung_parse_panel_cmd(np, &display_on_seq,
				"qcom,panel-display-on-seq");
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

	mdss_samsung_parse_panel_cmd(np, &nv_mtp_read_cmds,
				"samsung,panel-nv-mtp-read-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_enable_cmds,
				"samsung,panel-nv-read-enable-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_disable_cmds,
				"samsung,panel-nv-read-disable-cmds");

	mdss_samsung_parse_panel_cmd(np, &temp_cmd,
				"samsung,panel-temp-read-cmd");

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
	mdss_samsung_parse_panel_cmd(np, &elvss_cmds_list,
				"samsung,panel-elvss-cmds-list");
	mdss_samsung_parse_panel_cmd(np, &aid_cmds_list,
				"samsung,panel-aid-cmds-list");
	mdss_samsung_parse_panel_table(np, &aid_map_table,
				"samsung,panel-aid-map-table");

	mdss_samsung_parse_panel_table(np, &acl_map_table,
				"samsung,panel-acl-map-table");

	/* Process the lux value table */
	mdss_samsung_parse_candella_lux_mapping_table(np, &candela_map_table,
				"samsung,panel-candella-mapping-table");

#if defined(HBM_RE)
	mdss_samsung_parse_panel_cmd(np, &nv_mtp_hbm_read_cmds,
				"samsung,panel-nv-mtp-read-hbm-cmds");
	mdss_samsung_parse_panel_cmd(np, &nv_mtp_hbm2_read_cmds,
				"samsung,panel-nv-mtp-read-hbm2-cmds");

	mdss_samsung_parse_panel_cmd(np, &hbm_gamma_cmds_list_main,
				"samsung,panel-gamma-hbm-cmds-list-main");
	mdss_samsung_parse_panel_cmd(np, &hbm_gamma_cmds_list_sub,
				"samsung,panel-gamma-hbm-cmds-list-sub");

	mdss_samsung_parse_panel_cmd(np, &hbm_etc_cmds_list,
					"samsung,panel-etc-hbm-cmds");
	mdss_samsung_parse_panel_cmd(np, &hbm_etc_cmds_list_main,
					"samsung,panel-etc-hbm-cmds-main");
	mdss_samsung_parse_panel_cmd(np, &hbm_etc_cmds_list_sub,
					"samsung,panel-etc-hbm-cmds-sub");
#endif
#if defined(CONFIG_MDNIE_LITE_TUNING)
	mdss_samsung_parse_panel_cmd(np, &nv_mdnie_read_cmds,
					"samsung,panel-nv-mdnie-read-cmds");
#endif
#ifdef DEBUG_LDI_STATUS
	mdss_samsung_parse_panel_cmd(np, &ldi_debug_cmds,
				"samsung,panel-ldi-debug-read-cmds");
#endif

	mdss_samsung_parse_panel_table(np, &smart_acl_elvss_map_table,
				"samsung,panel-smart-acl-elvss-map-table");

	mdss_samsung_parse_panel_cmd(np, &nv_mtp_elvss_read_cmds,
				"samsung,panel-nv-mtp-read-elvss-cmds");

	mdss_samsung_parse_panel_cmd(np, &elvss_common_cmds_list,
				"samsung,panel-elvss-common-cmds-list");
	mdss_samsung_parse_panel_cmd(np, &elvss_lowtemp_cmds_list,
				"samsung,panel-elvss-lowtemp-cmds-list");

	mdss_samsung_parse_panel_cmd(np, &smart_elvss_hightemp_cmds_list,
				"samsung,panel-smart-elvss-hightemp-cmds-list");
	mdss_samsung_parse_panel_cmd(np, &smart_elvss_lowtemp_cmds_list,
				"samsung,panel-smart-elvss-lowtemp-cmds-list");

	mdss_samsung_parse_panel_cmd(np, &smart_acl_hightemp_cmds_list,
				"samsung,panel-smart-acl-hightemp-cmds-list");
	mdss_samsung_parse_panel_cmd(np, &smart_acl_lowtemp_cmds_list,
				"samsung,panel-smart-acl-lowtemp-cmds-list");

	return 0;
}

static int is_panel_supported(const char *panel_name)
{
	int i = 0;

	if (panel_name == NULL)
		return -EINVAL;

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


static int samsung_dsi_panel_event_handler(int event)
{
	pr_debug("%s : %d",__func__,event);
	switch (event) {
		case MDSS_EVENT_FRAME_UPDATE:
			if(msd.dstat.wait_disp_on) {
				pr_info("DISPLAY_ON\n");
				mipi_samsung_disp_send_cmd(PANEL_DISPLAY_ON, true);
				msd.dstat.wait_disp_on = 0;
			}
		break;
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

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
static irqreturn_t err_fg_irq_handler(int irq, void *handle)
{
	struct msm_fb_data_type *mfd = msd.mfd;

	if(err_fg_working || !(mfd->panel_power_on)) return IRQ_HANDLED;

	pr_info("%s : ++ irq=%d\n", __func__, irq);
	err_fg_working = 1;
	disable_irq_nosync(gpio_to_irq(err_fg_gpio));
	schedule_work(&err_fg_work);
	pr_info("%s : --\n", __func__);

	return IRQ_HANDLED;
}

void err_fg_work_func(struct work_struct *work)
{
	struct msm_fb_data_type *mfd = msd.mfd;
	struct mdss_panel_data *pdata = msd.pdata;

	pr_info("%s : ++\n", __func__);

	if (msd.mfd == NULL) {
		pr_err("%s: mfd is not initialized, Skip ESD recovery\n", __func__);
		return;
	}

	if (mfd->panel_power_on) {
		pdata->panel_info.panel_dead = true; // need for cmd panel

		mutex_lock(&mfd->power_state);
		mutex_lock(&mfd->ctx_lock);

		usleep_range(5000, 5000);
		msd.pdata->event_handler(msd.pdata, MDSS_EVENT_BLANK, NULL);
		msd.pdata->event_handler(msd.pdata, MDSS_EVENT_PANEL_OFF, NULL);
		usleep_range(5000, 5000);
		msd.pdata->event_handler(msd.pdata, MDSS_EVENT_UNBLANK, NULL);
		msd.pdata->event_handler(msd.pdata, MDSS_EVENT_PANEL_ON, NULL);
		usleep_range(5000, 5000);
		msd.mfd->mdp.kickoff_fnc(msd.mfd, NULL);

		mutex_unlock(&mfd->ctx_lock);
		mutex_unlock(&mfd->power_state);
	} else {
		pr_err("%s : panel is not on status!\n", __func__);
	}

	esd_count++;
	err_fg_working = 0;

	pr_info("%s -- (%d)\n", __func__, esd_count);

	return;
}
#endif


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
		return;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	pr_info("%s Loading File Size : %ld(bytes)", __func__, l);

	dp = kmalloc(l + 10, GFP_KERNEL);
	if (dp == NULL) {
		pr_info("Can't not alloc memory for tuning file load\n");
		filp_close(filp, current->files);
		return;
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
		return;
	}

	filp_close(filp, current->files);

	set_fs(fs);

	sending_tune_cmd(dp, l);

	kfree(dp);
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

	rc = snprintf((char *)buf, sizeof(buf), "%d\n", mfd->panel_power_on);
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

static ssize_t mipi_samsung_disp_lcd_switch(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int ret = 0;

	char *envp[2] = {"PANEL_ALIVE=0", NULL};
		struct device *dev2 = msd.mfd->fbi->dev;

	pr_info("%s ++ \n", __func__);

	/* switch */
	if (msd.dstat.lcd_sel == 1)
		flip = 0;
	else
		flip = 1;

	msd.dstat.lcd_sel = flip;
	kobject_uevent_env(&dev2->kobj, KOBJ_CHANGE, envp);
	pr_info("%s : sending uevent - %s\n", __func__, envp[0]);

	pr_info("%s -- \n", __func__);

	return ret;
}

static ssize_t mipi_samsung_disp_lcdtype_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char temp[100];

	if (msd.manufacture_id) {
		snprintf(temp, 20, "SDC_%x\n",msd.manufacture_id);
	} else {
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

	if (!c_lcd_sel) {
		snprintf((char *)temp, sizeof(temp), "manufacture date : %d\n", msd.manufacture_date_main);
		strlcat(buf, temp, 30);
		pr_info("main octa manufacture date : %d\n", msd.manufacture_date_main);
	} else {
		snprintf((char *)temp, sizeof(temp), "manufacture date : %d\n", msd.manufacture_date_sub);
		strlcat(buf, temp, 30);
		pr_info("sub octa manufacture date : %d\n", msd.manufacture_date_sub);
	}

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

static ssize_t mipi_samsung_disp_panel_res_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char temp[100];

	switch (msd.panel) {
		case PANEL_WXGA_OCTA_EA8061V_VIDEO_DUAL:
			snprintf(temp, 10, "WXGA\n");
			break;
		default :
			snprintf(temp, 10, "NO PANEL\n");
			break;
	}
	strlcat(buf, temp, 100);

	return strnlen(buf, 100);

}

static ssize_t mipi_samsung_disp_acl_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, sizeof(buf), "%d\n", msd.dstat.acl_on);
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

	rc = snprintf((char *)buf, sizeof(buf), "%d\n", msd.dstat.siop_status);
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

	if (msd.dstat.main_smart_dim_loaded || msd.dstat.sub_smart_dim_loaded) {
		if (!c_lcd_sel)
			msd.sdimconf_main->print_aid_log();
		else
			msd.sdimconf_sub->print_aid_log();
	} else
		pr_err("smart dim is not loaded..\n");

	return rc;
}

#if defined(CONFIG_BACKLIGHT_CLASS_DEVICE)
static ssize_t mipi_samsung_auto_brightness_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf((char *)buf, sizeof(buf), "%d\n",
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
		mDNIe_Set_Mode(); // LOCAL CE tuning
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

	if (len > 0x100) {
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

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
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
#endif

static struct lcd_ops mipi_samsung_disp_props = {
	.get_power = NULL,
	.set_power = NULL,
};

static DEVICE_ATTR(lcd_power, S_IRUGO | S_IWUSR,
			mipi_samsung_disp_get_power,
			mipi_samsung_disp_set_power);
static DEVICE_ATTR(lcd_type, S_IRUGO,
			mipi_samsung_disp_lcdtype_show,
			NULL);
static DEVICE_ATTR(lcd_switch, S_IRUGO,
			mipi_samsung_disp_lcd_switch,
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
static DEVICE_ATTR(aid_log, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_aid_log_show,
			NULL);
static DEVICE_ATTR(panel_res, S_IRUGO,
			mipi_samsung_disp_panel_res_show,
			NULL);
static DEVICE_ATTR(temperature, S_IRUGO | S_IWUSR | S_IWGRP,
			mipi_samsung_temperature_show,
			mipi_samsung_temperature_store);
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
static DEVICE_ATTR(esd_check, 0664 ,
			mipi_samsung_esd_check_show,
			mipi_samsung_esd_check_store);
#endif

static struct attribute *panel_sysfs_attributes[] = {
	&dev_attr_lcd_power.attr,
	&dev_attr_lcd_type.attr,
	&dev_attr_lcd_switch.attr,
	&dev_attr_window_type.attr,
	&dev_attr_manufacture_date.attr,
	&dev_attr_ddi_id.attr,
	&dev_attr_power_reduce.attr,
	&dev_attr_siop_enable.attr,
	&dev_attr_aid_log.attr,
	&dev_attr_read_mtp.attr,
	&dev_attr_panel_res.attr,
	&dev_attr_temperature.attr,
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

#if defined(DDI_VIDEO_ENHANCE_TUNING)
     rc = sysfs_create_file(&lcd_device->dev.kobj,
		  	 &dev_attr_tuning.attr);
    if (rc) {
	   pr_err("sysfs create fail-%s\n",
				   dev_attr_tuning.attr.name);
	   return rc;
	}
#endif

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	rc= sysfs_create_file(&lcd_device->dev.kobj,
							&dev_attr_esd_check.attr);
	if (rc) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_esd_check.attr.name);
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
	bool partial_update_enabled;

	pr_debug("%s: ++ \n", __func__);

	if (!node)
		return -ENODEV;

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

	pr_info("%s:%d cmd_cfg_cont_splash  = %d\n",
					__func__, __LINE__,cmd_cfg_cont_splash);

	cont_splash_enabled = of_property_read_bool(node,
				"qcom,cont-splash-enabled");

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
	ctrl_pdata->event_handler = samsung_dsi_panel_event_handler;
	ctrl_pdata->bl_fnc = mdss_dsi_panel_bl_ctrl;
	ctrl_pdata->panel_reset = mdss_dsi_panel_reset;
	ctrl_pdata->registered = mdss_dsi_panel_registered;
	ctrl_pdata->dimming_init = mdss_dsi_panel_dimming_init;
	ctrl_pdata->panel_blank = mdss_dsi_panel_blank;
	ctrl_pdata->bklt_ctrl = ctrl_pdata->panel_data.panel_info.bklt_ctrl;
	ctrl_pdata->panel_data.set_backlight = mdss_dsi_panel_bl_ctrl;

	mutex_init(&msd.lock);

	msd.dstat.on = 0;
	msd.dstat.recent_bright_level = 0;
	msd.dstat.lcd_sel = lcd_sel_lk;
	msd.dstat.temperature = 20; // default temperature

	partial_update_enabled =of_property_read_bool(node,"qcom,partial-update-enabled");
	if (partial_update_enabled) {
		pr_info("%s:%d Partial update enabled.\n", __func__, __LINE__);
		ctrl_pdata->panel_data.panel_info.partial_update_enabled = 1;
		ctrl_pdata->partial_update_fnc = mdss_dsi_panel_partial_update;
	} else {
		pr_info("%s:%d Partial update disabled.\n", __func__, __LINE__);
		ctrl_pdata->panel_data.panel_info.partial_update_enabled = 0;
		ctrl_pdata->partial_update_fnc = NULL;
	}

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	INIT_WORK(&err_fg_work, err_fg_work_func);

	rc = request_threaded_irq(gpio_to_irq(err_fg_gpio),
		NULL, err_fg_irq_handler,  IRQF_TRIGGER_RISING | IRQF_ONESHOT, "esd_detect", NULL);
	if (rc) {
		pr_err("%s : Failed to request_irq.:ret=%d", __func__, rc);
	}

	disable_irq(gpio_to_irq(err_fg_gpio));
#endif

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

	pr_debug("%s : --\n",__func__);

	return 0;
}

int get_lcd_id(void)
{
	return lcd_id;
}
EXPORT_SYMBOL(get_lcd_id);

int get_samsung_lcd_attached(void)
{
	return lcd_attached;

}
EXPORT_SYMBOL(get_samsung_lcd_attached);

int get_lcd_pcd_detected(void)
{
	//return !lcd_pcd;
	return !lcd_attached;
}
EXPORT_SYMBOL(get_lcd_pcd_detected);

static int __init get_lcd_sel_cmdline(char *val)
{
	lcd_sel_lk = simple_strtol(val, NULL, 0);
	pr_info("%s : lcd_sel from lk = %d\n", __func__, lcd_sel_lk);

	return 1;
}
__setup( "lcd_sel=0x", get_lcd_sel_cmdline );

static int __init get_pcd_cmdline(char *val)
{
	lcd_pcd = simple_strtol(val, NULL, 0);
	pr_info("%s : lcd_pcd = %d\n", __func__, lcd_pcd);

	return 1;
}
__setup( "lcd_pcd=0x", get_pcd_cmdline );

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

	if (!lcd_id) {
		lcd_attached = 0;
		lcd_pcd = 0;
	}

	pr_info( "%s: LCD_ID = 0x%X, lcd_attached = %d\n", __func__, lcd_id, lcd_attached);

	return 0;
}
__setup( "lcd_id=0x", get_lcd_id_cmdline );

MODULE_DESCRIPTION("Samsung EA8061V panel driver");
MODULE_AUTHOR("Jo Kwang Rae <kr0124.cho@samsung.com>");
MODULE_LICENSE("GPL");
