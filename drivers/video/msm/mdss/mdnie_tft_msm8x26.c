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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fb.h>
#include <linux/msm_mdp.h>
#include <linux/ioctl.h>
#include <linux/lcd.h>

#include "mdss_fb.h"
#include "mdss_panel.h"
#include "mdss_dsi.h"
#include "mdss_samsung_dsi_panel_msm8x26.h"
#include "mdnie_tft_msm8x26.h"
#if defined(CONFIG_MACH_MS01_EUR_3G) || defined(CONFIG_MACH_MS01_EUR_LTE)
#include "mdnie_lite_tuning_data_ms01.h"
#elif defined(CONFIG_SEC_MILLET_PROJECT) || defined(CONFIG_SEC_DEGAS_PROJECT)
#include "mdnie_tft_data_millet.h"
#elif defined(CONFIG_SEC_T10_PROJECT) || defined(CONFIG_SEC_T8_PROJECT)
#include "mdnie_tft_data_t10_t8.h"
#elif defined(CONFIG_MACH_MEGA23GEUR_OPEN)
#include "mdnie_tft_data_mega23g.h"
#endif

int get_lcd_attached(void);

#define MDNIE_TFT_DEBUG

#ifdef MDNIE_TFT_DEBUG
#define DPRINT(x...)	printk(KERN_ERR "[mdnie lite] " x)
#else
#define DPRINT(x...)
#endif

#define MAX_LUT_SIZE	256
#if defined (CONFIG_FB_MSM_MDSS_SDC_WXGA_PANEL)
#define PAYLOAD1 mdni_tune_cmd[7]
#define PAYLOAD2 mdni_tune_cmd[6]
#define PAYLOAD3 mdni_tune_cmd[5]
#define PAYLOAD4 mdni_tune_cmd[4]
#define PAYLOAD5 mdni_tune_cmd[3]
#define PAYLOAD6 mdni_tune_cmd[2]

#define INPUT_PAYLOAD1(x) PAYLOAD1.payload = x
#define INPUT_PAYLOAD2(x) PAYLOAD2.payload = x
#define INPUT_PAYLOAD3(x) PAYLOAD3.payload = x
#define INPUT_PAYLOAD4(x) PAYLOAD4.payload = x
#define INPUT_PAYLOAD5(x) PAYLOAD5.payload = x
#define INPUT_PAYLOAD6(x) PAYLOAD6.payload = x
#elif defined(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
#define PAYLOAD1 mdni_tune_cmd[0]
#define PAYLOAD2 mdni_tune_cmd[1]
#define PAYLOAD3 mdni_tune_cmd[2]
#define PAYLOAD4 mdni_tune_cmd[3]

#define INPUT_PAYLOAD1(x) PAYLOAD1.payload = x
#define INPUT_PAYLOAD2(x) PAYLOAD2.payload = x
#define INPUT_PAYLOAD3(x) PAYLOAD3.payload = x
#define INPUT_PAYLOAD4(x) PAYLOAD4.payload = x
#else
#define PAYLOAD1 mdni_tune_cmd[3]
#define PAYLOAD2 mdni_tune_cmd[2]

#define INPUT_PAYLOAD1(x) PAYLOAD1.payload = x
#define INPUT_PAYLOAD2(x) PAYLOAD2.payload = x
#endif

int play_speed_1_5;

struct dsi_buf dsi_mdnie_tx_buf;

static struct mdss_samsung_driver_data *mdnie_msd;

struct mdnie_tft_type mdnie_tun_state = {
	.mdnie_enable = false,
	.scenario = mDNIe_UI_MODE,
	.background = STANDARD_MODE,
	.outdoor = OUTDOOR_OFF_MODE,
	.negative = mDNIe_NEGATIVE_OFF,
	.blind = ACCESSIBILITY_OFF,
};
#if defined(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
const char background_name[MAX_BACKGROUND_MODE][16] = {
	"DYNAMIC",
	"STANDARD",
	"MOVIE",
	"AUTO"
};
#else
const char background_name[MAX_BACKGROUND_MODE][16] = {
	"DYNAMIC",
	"STANDARD",
	"MOVIE",
	"NATURAL",
};
#endif
const char scenario_name[MAX_mDNIe_MODE][16] = {
	"UI_MODE",
	"VIDEO_MODE",
	"VIDEO_WARM_MODE",
	"VIDEO_COLD_MODE",
	"CAMERA_MODE",
	"NAVI",
	"GALLERY_MODE",
	"VT_MODE",
	"BROWSER",
	"eBOOK",
#if defined(CONFIG_TDMB)
	"DMB_MODE",
	"DMB_WARM_MODE",
	"DMB_COLD_MODE",
#endif
};

#if defined(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
const char accessibility_name[ACCESSIBILITY_MAX][20] = {
	"ACCESSIBILITY_OFF",
	"NEGATIVE_MODE",
	"COLOR_BLIND_MODE",
};
#endif

#if !defined (CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
static char level1_key[] = {
	0xF0,
	0x5A, 0x5A,
};

static char level2_key[] = {
	0xF1,
	0x5A, 0x5A,
};
#endif
static char tune_data1[MDNIE_TUNE_FIRST_SIZE] = {0,};
static char tune_data2[MDNIE_TUNE_SECOND_SIZE] = {0,};
#if defined (CONFIG_FB_MSM_MDSS_SDC_WXGA_PANEL) || defined (CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
static char tune_data3[MDNIE_TUNE_THIRD_SIZE] = {0,};
static char tune_data4[MDNIE_TUNE_FOURTH_SIZE] = {0,};
#if !defined (CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
static char tune_data5[MDNIE_TUNE_FIFTH_SIZE] = {0,};
static char tune_data6[MDNIE_TUNE_SIXTH_SIZE] = {0,};
#endif
#endif
static struct dsi_cmd_desc mdni_tune_cmd[] = {
#if !defined (CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 1,
		sizeof(level1_key)}, level1_key},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 1,
		sizeof(level2_key)}, level2_key},
#endif
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 2,
		sizeof(tune_data1)}, tune_data1},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 2,
		sizeof(tune_data2)}, tune_data2},
#if defined (CONFIG_FB_MSM_MDSS_SDC_WXGA_PANEL) || defined (CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 2,
		sizeof(tune_data3)}, tune_data3},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 2,
		sizeof(tune_data4)}, tune_data4},
#if !defined (CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 2,
		sizeof(tune_data5)}, tune_data5},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 2,
		sizeof(tune_data6)}, tune_data6},
#endif
#endif
};

void print_tun_data(void)
{
	int i;

#if defined (CONFIG_FB_MSM_MDSS_SDC_WXGA_PANEL)
	DPRINT("---- size2 : %d", PAYLOAD1.dchdr.dlen);
	for (i = 0; i < MDNIE_TUNE_SIXTH_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD1.payload[i]);
	DPRINT("\n");
	DPRINT("---- size3 : %d", PAYLOAD2.dchdr.dlen);
	for (i = 0; i < MDNIE_TUNE_FIFTH_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD2.payload[i]);
	DPRINT("\n");
		DPRINT("---- size4 : %d", PAYLOAD3.dchdr.dlen);
	for (i = 0; i < MDNIE_TUNE_FOURTH_SIZE ; i++)
	DPRINT("0x%x ", PAYLOAD3.payload[i]);
	DPRINT("\n");
	DPRINT("---- size5 : %d", PAYLOAD4.dchdr.dlen);
	for (i = 0; i < MDNIE_TUNE_THIRD_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD4.payload[i]);
	DPRINT("\n");
	DPRINT("---- size6 : %d", PAYLOAD5.dchdr.dlen);
	for (i = 0; i < MDNIE_TUNE_SECOND_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD5.payload[i]);
	DPRINT("\n");
	DPRINT("---- size7 : %d", PAYLOAD6.dchdr.dlen);
	for (i = 0; i < MDNIE_TUNE_FIRST_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD6.payload[i]);
	DPRINT("\n");
#elif defined (CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
	DPRINT("\n");
	DPRINT("---- size1 : %d", PAYLOAD1.dchdr.dlen);
	for (i = 0; i < MDNIE_TUNE_FIRST_SIZE ; i++)
	DPRINT("0x%x ", PAYLOAD1.payload[i]);
	DPRINT("\n");
	DPRINT("---- size2 : %d", PAYLOAD2.dchdr.dlen);
	for (i = 0; i < MDNIE_TUNE_SECOND_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD2.payload[i]);
	DPRINT("\n");
	DPRINT("---- size3 : %d", PAYLOAD3.dchdr.dlen);
	for (i = 0; i < MDNIE_TUNE_THIRD_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD3.payload[i]);
	DPRINT("\n");
	DPRINT("---- size4 : %d", PAYLOAD4.dchdr.dlen);
	for (i = 0; i < MDNIE_TUNE_FOURTH_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD4.payload[i]);
	DPRINT("\n");
#else
	DPRINT("\n");
	DPRINT("---- size1 : %d", PAYLOAD1.dchdr.dlen);
	for (i = 0; i < MDNIE_TUNE_SECOND_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD1.payload[i]);
	DPRINT("\n");
	DPRINT("---- size2 : %d", PAYLOAD2.dchdr.dlen);
	for (i = 0; i < MDNIE_TUNE_FIRST_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD2.payload[i]);
	DPRINT("\n");
#endif
}

void free_tun_cmd(void)
{
	memset(tune_data1, 0, MDNIE_TUNE_FIRST_SIZE);
	memset(tune_data2, 0, MDNIE_TUNE_SECOND_SIZE);
#if defined (CONFIG_FB_MSM_MDSS_SDC_WXGA_PANEL) || defined (CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
	memset(tune_data3, 0, MDNIE_TUNE_THIRD_SIZE);
	memset(tune_data4, 0, MDNIE_TUNE_FOURTH_SIZE);
#if !defined (CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
	memset(tune_data5, 0, MDNIE_TUNE_FIFTH_SIZE);
	memset(tune_data6, 0, MDNIE_TUNE_SIXTH_SIZE);
#endif
#endif
}
void sending_tuning_cmd(void)
{
	struct msm_fb_data_type *mfd;
	struct mdss_panel_data *pdata;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata;

	mfd = mdnie_msd->mfd;
	pdata = mdnie_msd->mpd;
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
						panel_data);

	mutex_lock(&mdnie_msd->lock);

	if (mfd->resume_state == MIPI_SUSPEND_STATE) {
		mutex_unlock(&mdnie_msd->lock);
		DPRINT(" power off!!\n");
	} else {
#ifdef MDNIE_TFT_DATA_DEBUG
		print_tun_data();
#else
		DPRINT(" send tuning cmd!!\n");
#endif
		mdss_dsi_cmds_send(ctrl_pdata, mdni_tune_cmd, ARRAY_SIZE(mdni_tune_cmd),0);

		mutex_unlock(&mdnie_msd->lock);
	}
}

void mDNIe_Set_Mode(enum Lcd_mDNIe_UI mode)
{
	struct msm_fb_data_type *mfd;
	mfd = mdnie_msd->mfd;

	DPRINT("mDNIe_Set_Mode start , mode(%d), background(%d)\n",
		mode, mdnie_tun_state.background);


	if (get_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}


	if (mfd->resume_state == MIPI_SUSPEND_STATE) {
		DPRINT("[ERROR] not ST_DSI_RESUME. do not send mipi cmd.\n");
		return;
	}
	if (!mdnie_tun_state.mdnie_enable) {
		DPRINT("[ERROR] mDNIE engine is OFF.\n");
		return;
	}

	if (mode < mDNIe_UI_MODE || mode >= MAX_mDNIe_MODE) {
		DPRINT("[ERROR] wrong Scenario mode value : %d\n",
			mode);
		return;
	}

	if (mdnie_tun_state.negative) {
		DPRINT("already negative mode(%d), do not set background(%d)\n",
			mdnie_tun_state.negative, mdnie_tun_state.background);
		return;
	}

	play_speed_1_5 = 0;

	/*
	*	Blind mode & Screen mode has separated menu.
	*	To make a sync below code added.
	*	Bline mode has priority than Screen mode
	*/
	if (mdnie_tun_state.blind == COLOR_BLIND)
		mode = mDNIE_BLINE_MODE;

#if !defined(CONFIG_SEC_MATISSE_PROJECT) && !defined(CONFIG_MDP_NEGATIVE_SUPPORT)
	switch (mode) {
#if defined (CONFIG_FB_MSM_MDSS_SDC_WXGA_PANEL)
	case mDNIe_UI_MODE:
		DPRINT(" = UI MODE =\n");
		INPUT_PAYLOAD1(UI_1);
		INPUT_PAYLOAD2(UI_2);
		INPUT_PAYLOAD3(UI_3);
		INPUT_PAYLOAD4(UI_4);
		INPUT_PAYLOAD5(UI_5);
		INPUT_PAYLOAD6(UI_6);
		break;

	case mDNIe_VIDEO_MODE:
		DPRINT(" = VIDEO MODE =\n");
		INPUT_PAYLOAD1(VIDEO_1);
		INPUT_PAYLOAD2(VIDEO_2);
		INPUT_PAYLOAD3(VIDEO_3);
		INPUT_PAYLOAD4(VIDEO_4);
		INPUT_PAYLOAD5(VIDEO_5);
		INPUT_PAYLOAD6(VIDEO_6);
		break;

	case mDNIe_VIDEO_WARM_MODE:
		DPRINT(" = VIDEO WARM MODE =\n");
		DPRINT("no data for WARM MODE..\n");
		break;

	case mDNIe_VIDEO_COLD_MODE:
		DPRINT(" = VIDEO COLD MODE =\n");
		DPRINT("no data for COLD MODE..\n");
		break;

	case mDNIe_CAMERA_MODE:
		DPRINT(" = CAMERA MODE =\n");
		INPUT_PAYLOAD1(CAMERA_1);
		INPUT_PAYLOAD2(CAMERA_2);
		INPUT_PAYLOAD3(CAMERA_3);
		INPUT_PAYLOAD4(CAMERA_4);
		INPUT_PAYLOAD5(CAMERA_5);
		INPUT_PAYLOAD6(CAMERA_6);
		break;

	case mDNIe_NAVI:
		DPRINT(" = NAVI MODE =\n");
		DPRINT("no data for NAVI MODE..\n");
		break;

	case mDNIe_GALLERY:
		DPRINT(" = GALLERY MODE =\n");
		INPUT_PAYLOAD1(GALLERY_1);
		INPUT_PAYLOAD2(GALLERY_2);
		INPUT_PAYLOAD3(GALLERY_3);
		INPUT_PAYLOAD4(GALLERY_4);
		INPUT_PAYLOAD5(GALLERY_5);
		INPUT_PAYLOAD6(GALLERY_6);
		break;

	case mDNIe_VT_MODE:
		DPRINT(" = VT MODE =\n");
		INPUT_PAYLOAD1(VT_1);
		INPUT_PAYLOAD2(VT_2);
		INPUT_PAYLOAD3(VT_3);
		INPUT_PAYLOAD4(VT_4);
		INPUT_PAYLOAD5(VT_5);
		INPUT_PAYLOAD6(VT_6);
		break;

#if defined(CONFIG_TDMB)
	case mDNIe_DMB_MODE:
		DPRINT(" = DMB MODE =\n");
		DPRINT("no data for DMB MODE..\n");
		break;

	case mDNIe_DMB_WARM_MODE:
		DPRINT(" = DMB WARM MODE =\n");
		DPRINT("no data for DMB  WARM MODE..\n");
		break;

	case mDNIe_DMB_COLD_MODE:
		DPRINT(" = DMB COLD MODE =\n");
		DPRINT("no data for DMB COLD MODE..\n");
		break;
#endif

	case mDNIe_BROWSER_MODE:
		DPRINT(" = BROWSER MODE =\n");
		INPUT_PAYLOAD1(BROWSER_1);
		INPUT_PAYLOAD2(BROWSER_2);
		INPUT_PAYLOAD3(BROWSER_3);
		INPUT_PAYLOAD4(BROWSER_4);
		INPUT_PAYLOAD5(BROWSER_5);
		INPUT_PAYLOAD6(BROWSER_6);
		break;

	case mDNIe_eBOOK_MODE:
		DPRINT(" = eBOOK MODE =\n");
		INPUT_PAYLOAD1(eBOOK_1);
		INPUT_PAYLOAD2(eBOOK_2);
		INPUT_PAYLOAD3(eBOOK_3);
		INPUT_PAYLOAD4(eBOOK_4);
		INPUT_PAYLOAD5(eBOOK_5);
		INPUT_PAYLOAD6(eBOOK_6);
		break;

	case mDNIe_EMAIL_MODE:
		DPRINT(" = EMAIL MODE =\n");
		INPUT_PAYLOAD1(eMAIL_1);
		INPUT_PAYLOAD2(eMAIL_2);
		INPUT_PAYLOAD3(eMAIL_3);
		INPUT_PAYLOAD4(eMAIL_4);
		INPUT_PAYLOAD5(eMAIL_5);
		INPUT_PAYLOAD6(eMAIL_6);
		break;

	case mDNIE_BLINE_MODE:
		DPRINT(" = BLIND MODE =\n");
		INPUT_PAYLOAD1(COLOR_BLIND_1);
		INPUT_PAYLOAD2(COLOR_BLIND_2);
		INPUT_PAYLOAD3(COLOR_BLIND_3);
		INPUT_PAYLOAD4(COLOR_BLIND_4);
		INPUT_PAYLOAD5(COLOR_BLIND_5);
		INPUT_PAYLOAD6(COLOR_BLIND_6);
		break;
#elif defined (CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
	case mDNIe_UI_MODE:
		DPRINT(" = UI MODE =\n");
		if (!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]) {
			pr_err("mdnie tune data is NULL!\n");
			return;
		} else {
			INPUT_PAYLOAD1(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0]);
			INPUT_PAYLOAD2(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1]);
			INPUT_PAYLOAD3(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2]);
			INPUT_PAYLOAD4(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]);

		}
		break;

	case mDNIe_VIDEO_MODE:
		DPRINT(" = VIDEO MODE =\n");
		if (!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]) {
			pr_err("mdnie tune data is NULL!\n");
			return;
		} else {
			INPUT_PAYLOAD1(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0]);
			INPUT_PAYLOAD2(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1]);
			INPUT_PAYLOAD3(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2]);
			INPUT_PAYLOAD4(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]);

		}

		break;

	case mDNIe_VIDEO_WARM_MODE:
		DPRINT(" = VIDEO WARM MODE =\n");
		DPRINT("no data for WARM MODE..\n");
		break;

	case mDNIe_VIDEO_COLD_MODE:
		DPRINT(" = VIDEO COLD MODE =\n");
		DPRINT("no data for COLD MODE..\n");
		break;

	case mDNIe_CAMERA_MODE:
		DPRINT(" = CAMERA MODE =\n");
		if (!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]) {
			pr_err("mdnie tune data is NULL!\n");
			return;
		} else {
			INPUT_PAYLOAD1(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0]);
			INPUT_PAYLOAD2(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1]);
			INPUT_PAYLOAD3(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2]);
			INPUT_PAYLOAD4(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]);

		}

		break;

	case mDNIe_NAVI:
		DPRINT(" = NAVI MODE =\n");
		DPRINT("no data for NAVI MODE..\n");
		break;

	case mDNIe_GALLERY:
		DPRINT(" = GALLERY MODE =\n");
		if (!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]) {
			pr_err("mdnie tune data is NULL!\n");
			return;
		} else {
			INPUT_PAYLOAD1(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0]);
			INPUT_PAYLOAD2(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1]);
			INPUT_PAYLOAD3(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2]);
			INPUT_PAYLOAD4(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]);

		}

		break;

	case mDNIe_VT_MODE:
		DPRINT(" = VT MODE =\n");
		if (!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]) {
			pr_err("mdnie tune data is NULL!\n");
			return;
		} else {
			INPUT_PAYLOAD1(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0]);
			INPUT_PAYLOAD2(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1]);
			INPUT_PAYLOAD3(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2]);
			INPUT_PAYLOAD4(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]);

		}

		break;

#if defined(CONFIG_TDMB)
	case mDNIe_DMB_MODE:
		DPRINT(" = DMB MODE =\n");
		DPRINT("no data for DMB MODE..\n");
		break;

	case mDNIe_DMB_WARM_MODE:
		DPRINT(" = DMB WARM MODE =\n");
		DPRINT("no data for DMB  WARM MODE..\n");
		break;

	case mDNIe_DMB_COLD_MODE:
		DPRINT(" = DMB COLD MODE =\n");
		DPRINT("no data for DMB COLD MODE..\n");
		break;
#endif

	case mDNIe_BROWSER_MODE:
		DPRINT(" = BROWSER MODE =\n");
		if (!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]) {
			pr_err("mdnie tune data is NULL!\n");
			return;
		} else {
			INPUT_PAYLOAD1(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0]);
			INPUT_PAYLOAD2(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1]);
			INPUT_PAYLOAD3(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2]);
			INPUT_PAYLOAD4(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]);

		}

		break;

	case mDNIe_eBOOK_MODE:
		DPRINT(" = eBOOK MODE =\n");
		if (!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]) {
			pr_err("mdnie tune data is NULL!\n");
			return;
		} else {
			INPUT_PAYLOAD1(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0]);
			INPUT_PAYLOAD2(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1]);
			INPUT_PAYLOAD3(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2]);
			INPUT_PAYLOAD4(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]);

		}

		break;

	case mDNIe_EMAIL_MODE:
		DPRINT(" = EMAIL MODE =\n");
		if (!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2] ||
			!mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]) {
			pr_err("mdnie tune data is NULL!\n");
			return;
		} else {
			INPUT_PAYLOAD1(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][0]);
			INPUT_PAYLOAD2(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][1]);
			INPUT_PAYLOAD3(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][2]);
			INPUT_PAYLOAD4(
				mdnie_tune_value[mdnie_tun_state.scenario][mdnie_tun_state.background][mdnie_tun_state.outdoor][3]);

		}

		break;

	case mDNIE_BLINE_MODE:
		DPRINT(" = BLIND MODE =\n");

		if (!blind_tune_value[mdnie_tun_state.blind][0] || !blind_tune_value[mdnie_tun_state.blind][1] ||
			!blind_tune_value[mdnie_tun_state.blind][2] || !blind_tune_value[mdnie_tun_state.blind][3]) {
			pr_err("mdnie tune data is NULL!\n");
			return;
		} else {
			INPUT_PAYLOAD1(blind_tune_value[mdnie_tun_state.blind][0]);
			INPUT_PAYLOAD2(blind_tune_value[mdnie_tun_state.blind][1]);
			INPUT_PAYLOAD3(blind_tune_value[mdnie_tun_state.blind][2]);
			INPUT_PAYLOAD4(blind_tune_value[mdnie_tun_state.blind][3]);
		}
		break;

#else
	case mDNIe_UI_MODE:
		DPRINT(" = UI MODE =\n");
		INPUT_PAYLOAD1(UI_1);
		INPUT_PAYLOAD2(UI_2);
		break;

	case mDNIe_VIDEO_MODE:
		DPRINT(" = VIDEO MODE =\n");
		INPUT_PAYLOAD1(VIDEO_1);
		INPUT_PAYLOAD2(VIDEO_2);
		break;

	case mDNIe_VIDEO_WARM_MODE:
		DPRINT(" = VIDEO WARM MODE =\n");
		DPRINT("no data for WARM MODE..\n");
		break;

	case mDNIe_VIDEO_COLD_MODE:
		DPRINT(" = VIDEO COLD MODE =\n");
		DPRINT("no data for COLD MODE..\n");
		break;

	case mDNIe_CAMERA_MODE:
		DPRINT(" = CAMERA MODE =\n");
		INPUT_PAYLOAD1(CAMERA_1);
		INPUT_PAYLOAD2(CAMERA_2);
		break;

	case mDNIe_NAVI:
		DPRINT(" = NAVI MODE =\n");
		DPRINT("no data for NAVI MODE..\n");
		break;

	case mDNIe_GALLERY:
		DPRINT(" = GALLERY MODE =\n");
		INPUT_PAYLOAD1(GALLERY_1);
		INPUT_PAYLOAD2(GALLERY_2);
		break;

	case mDNIe_VT_MODE:
		DPRINT(" = VT MODE =\n");
		INPUT_PAYLOAD1(VT_1);
		INPUT_PAYLOAD2(VT_2);
		break;

#if defined(CONFIG_TDMB)
	case mDNIe_DMB_MODE:
		DPRINT(" = DMB MODE =\n");
		INPUT_PAYLOAD1(TDMB_1);
		INPUT_PAYLOAD2(TDMB_2);
		break;

	case mDNIe_DMB_WARM_MODE:
		DPRINT(" = DMB WARM MODE =\n");
		DPRINT("no data for DMB  WARM MODE..\n");
		break;

	case mDNIe_DMB_COLD_MODE:
		DPRINT(" = DMB COLD MODE =\n");
		DPRINT("no data for DMB COLD MODE..\n");
		break;
#endif

	case mDNIe_BROWSER_MODE:
		DPRINT(" = BROWSER MODE =\n");
		INPUT_PAYLOAD1(BROWSER_1);
		INPUT_PAYLOAD2(BROWSER_2);
		break;

	case mDNIe_eBOOK_MODE:
		DPRINT(" = eBOOK MODE =\n");
		INPUT_PAYLOAD1(eBOOK_1);
		INPUT_PAYLOAD2(eBOOK_2);
		break;

	case mDNIe_EMAIL_MODE:
		DPRINT(" = EMAIL MODE =\n");
		INPUT_PAYLOAD1(eMAIL_1);
		INPUT_PAYLOAD2(eMAIL_2);
		break;

	case mDNIE_BLINE_MODE:
		DPRINT(" = BLIND MODE =\n");
		INPUT_PAYLOAD1(COLOR_BLIND_1);
		INPUT_PAYLOAD2(COLOR_BLIND_2);
		break;
#endif

	default:
		DPRINT("[%s] no option (%d)\n", __func__, mode);
		return;
	}
#endif
	sending_tuning_cmd();
	free_tun_cmd();

#if defined(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
		DPRINT("mDNIe_Set_Mode end , %s(%d), %s(%d),\n",
		scenario_name[mdnie_tun_state.scenario], mdnie_tun_state.scenario,
		background_name[mdnie_tun_state.background], mdnie_tun_state.background);
#else
		DPRINT("mDNIe_Set_Mode end , mode(%d), background(%d)\n",
		mode, mdnie_tun_state.background);
#endif
}
void is_negative_on(void)
{
	DPRINT("is negative Mode On = %d\n", mdnie_tun_state.negative);

	if (mdnie_tun_state.negative) {
		DPRINT("mDNIe_Set_Negative = %d\n", mdnie_tun_state.negative);
		DPRINT(" = NEGATIVE MODE =\n");

		mdss_negative_color(mdnie_tun_state.negative);
	} else {
		/* check the mode and tuning again when wake up*/
		DPRINT("negative off when resume, tuning again!\n");
		mdss_negative_color(mdnie_tun_state.negative);
#if !defined(CONFIG_SEC_MATISSE_PROJECT)  && !defined(CONFIG_MDP_NEGATIVE_SUPPORT)
		mDNIe_Set_Mode(mdnie_tun_state.scenario);
#endif
	}
}

void mDNIe_set_negative(enum Lcd_mDNIe_Negative negative)
{
	DPRINT("mDNIe_Set_Negative state:%d\n",negative);
	mdss_negative_color(negative);
}

void is_play_speed_1_5(int enable)
{
	play_speed_1_5 = enable;
}

/* ##########################################################
 * #
 * # MDNIE BG Sysfs node
 * #
 * ##########################################################*/

/* ##########################################################
 * #
 * #	0. Dynamic
 * #	1. Standard
 * #	2. Video
 * #	3. Natural
 * #
 * ##########################################################*/
#if !defined(CONFIG_SEC_MATISSE_PROJECT)  && !defined(CONFIG_MDP_NEGATIVE_SUPPORT)
static ssize_t mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 256, "Current Background Mode : %s\n",
		background_name[mdnie_tun_state.background]);
}

static ssize_t mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int value;
	struct msm_fb_data_type *mfd;
	mfd = mdnie_msd->mfd;

	sscanf(buf, "%d", &value);
	DPRINT("set background mode : %d\n", value);

	if (value < DYNAMIC_MODE || value >= MAX_BACKGROUND_MODE) {
		DPRINT("[ERROR] wrong backgound mode value : %d\n",
			value);
		return size;
	}

	mdnie_tun_state.background = value;

	if (mdnie_tun_state.negative) {
		DPRINT("already negative mode(%d), do not set background(%d)\n",
			mdnie_tun_state.negative, mdnie_tun_state.background);
	} else {
		DPRINT(" %s, input background(%d)\n",
			__func__, value);
		mutex_lock(&mfd->power_state);
		mDNIe_Set_Mode(mdnie_tun_state.scenario);
		mutex_unlock(&mfd->power_state);
	}

	return size;
}

static DEVICE_ATTR(mode, 0664, mode_show, mode_store);

static ssize_t scenario_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	DPRINT("called %s\n", __func__);

	DPRINT("Current Scenario Mode : %s\n",
		scenario_name[mdnie_tun_state.scenario]);

	return snprintf(buf, 256, "Current Scenario Mode : %s\n",
		scenario_name[mdnie_tun_state.scenario]);
}

static ssize_t scenario_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	int value;
	struct msm_fb_data_type *mfd;
	mfd = mdnie_msd->mfd;

	sscanf(buf, "%d", &value);

	if (value < mDNIe_UI_MODE || value >= MAX_mDNIe_MODE) {
		DPRINT("[ERROR] wrong Scenario mode value : %d\n",
			value);
		return size;
	}

	switch (value) {
	case SIG_MDNIE_UI_MODE:
		mdnie_tun_state.scenario = mDNIe_UI_MODE;
		break;

	case SIG_MDNIE_VIDEO_MODE:
		mdnie_tun_state.scenario = mDNIe_VIDEO_MODE;
		break;

	case SIG_MDNIE_VIDEO_WARM_MODE:
		mdnie_tun_state.scenario = mDNIe_VIDEO_WARM_MODE;
		break;

	case SIG_MDNIE_VIDEO_COLD_MODE:
		mdnie_tun_state.scenario = mDNIe_VIDEO_COLD_MODE;
		break;

	case SIG_MDNIE_CAMERA_MODE:
		mdnie_tun_state.scenario = mDNIe_CAMERA_MODE;
		break;

	case SIG_MDNIE_NAVI:
		mdnie_tun_state.scenario = mDNIe_NAVI;
		break;

	case SIG_MDNIE_GALLERY:
		mdnie_tun_state.scenario = mDNIe_GALLERY;
		break;

	case SIG_MDNIE_VT:
		mdnie_tun_state.scenario = mDNIe_VT_MODE;
		break;

	case SIG_MDNIE_BROWSER:
		mdnie_tun_state.scenario = mDNIe_BROWSER_MODE;
		break;

	case SIG_MDNIE_eBOOK:
		mdnie_tun_state.scenario = mDNIe_eBOOK_MODE;
		break;
	case SIG_MDNIE_EMAIL:
		mdnie_tun_state.scenario = mDNIe_EMAIL_MODE;
		break;

#ifdef BROWSER_COLOR_TONE_SET
	case SIG_MDNIE_BROWSER_TONE1:
		mdnie_tun_state.scenario = mDNIe_BROWSER_TONE1;
		break;
	case SIG_MDNIE_BROWSER_TONE2:
		mdnie_tun_state.scenario = mDNIe_BROWSER_TONE2;
		break;
	case SIG_MDNIE_BROWSER_TONE3:
		mdnie_tun_state.scenario = mDNIe_BROWSER_TONE3;
		break;
#endif


#if defined(CONFIG_TDMB)
	case SIG_MDNIE_DMB_MODE:
		mdnie_tun_state.scenario = mDNIe_DMB_MODE;
		break;
	case SIG_MDNIE_DMB_WARM_MODE:
		mdnie_tun_state.scenario = mDNIe_DMB_WARM_MODE;
		break;
	case SIG_MDNIE_DMB_COLD_MODE:
		mdnie_tun_state.scenario = mDNIe_DMB_COLD_MODE;
		break;
#endif

	default:
		DPRINT("scenario_store value is wrong : value(%d)\n",
		       value);
		break;
	}

	if (mdnie_tun_state.negative) {
		DPRINT("already negative mode(%d), do not set mode(%d)\n",
			mdnie_tun_state.negative, mdnie_tun_state.scenario);
	} else {
		DPRINT(" %s, input value = %d\n", __func__, value);
		mutex_lock(&mfd->power_state);
		mDNIe_Set_Mode(mdnie_tun_state.scenario);
		mutex_unlock(&mfd->power_state);
	}
	return size;
}
static DEVICE_ATTR(scenario, 0664, scenario_show,
		   scenario_store);

static ssize_t mdnieset_user_select_file_cmd_show(struct device *dev,
						  struct device_attribute *attr,
						  char *buf)
{
	unsigned int mdnie_ui = 0;
	DPRINT("called %s\n", __func__);

	return snprintf(buf, 256, "%u\n", mdnie_ui);
}

static ssize_t mdnieset_user_select_file_cmd_store(struct device *dev,
						   struct device_attribute
						   *attr, const char *buf,
						   size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	DPRINT
	("inmdnieset_user_select_file_cmd_store, input value = %d\n",
	     value);

	return size;
}

static DEVICE_ATTR(mdnieset_user_select_file_cmd, 0664,
		   mdnieset_user_select_file_cmd_show,
		   mdnieset_user_select_file_cmd_store);

static ssize_t mdnieset_init_file_cmd_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	char temp[] = "mdnieset_init_file_cmd_show\n\0";
	DPRINT("called %s\n", __func__);
	strcat(buf, temp);
	return strlen(buf);
}

static ssize_t mdnieset_init_file_cmd_store(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t size)
{
	int value;
	struct msm_fb_data_type *mfd;
	mfd = mdnie_msd->mfd;

	sscanf(buf, "%d", &value);
	DPRINT("mdnieset_init_file_cmd_store  : value(%d)\n", value);

	switch (value) {
	case 0:
		mdnie_tun_state.scenario = mDNIe_UI_MODE;
		break;

	default:
		printk(KERN_ERR
		       "mdnieset_init_file_cmd_store value is wrong : value(%d)\n",
		       value);
		break;
	}
	mutex_lock(&mfd->power_state);
	mDNIe_Set_Mode(mdnie_tun_state.scenario);
	mutex_unlock(&mfd->power_state);

	return size;
}

static DEVICE_ATTR(mdnieset_init_file_cmd, 0664, mdnieset_init_file_cmd_show,
		   mdnieset_init_file_cmd_store);

static ssize_t outdoor_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	DPRINT("called %s\n", __func__);
	return snprintf(buf, 256, "Current outdoor Value : %s\n",
		(mdnie_tun_state.outdoor == 0) ? "Disabled" : "Enabled");
}

static ssize_t outdoor_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t size)
{
	int value;
	struct msm_fb_data_type *mfd;
	mfd = mdnie_msd->mfd;

	sscanf(buf, "%d", &value);

	DPRINT("outdoor value = %d, scenario = %d\n",
		value, mdnie_tun_state.scenario);

	if (value < OUTDOOR_OFF_MODE || value >= MAX_OUTDOOR_MODE) {
		DPRINT("[ERROR] : wrong outdoor mode value : %d\n",
				value);
	}

	mdnie_tun_state.outdoor = value;

	if (mdnie_tun_state.negative) {
		DPRINT("already negative mode(%d), do not outdoor mode(%d)\n",
			mdnie_tun_state.negative, mdnie_tun_state.outdoor);
	} else {
		mutex_lock(&mfd->power_state);
		mDNIe_Set_Mode(mdnie_tun_state.scenario);
		mutex_unlock(&mfd->power_state);
	}

	return size;
}

static DEVICE_ATTR(outdoor, 0664, outdoor_show, outdoor_store);

static ssize_t playspeed_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	DPRINT("called %s\n", __func__);
	return snprintf(buf, 256, "%d\n", play_speed_1_5);
}

static ssize_t playspeed_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int value;
	sscanf(buf, "%d", &value);

	DPRINT("[Play Speed Set]play speed value = %d\n", value);

	is_play_speed_1_5(value);
	return size;
}
static DEVICE_ATTR(playspeed, 0664,
			playspeed_show,
			playspeed_store);

static ssize_t cabc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;
	unsigned char cabc;
	cabc = mdss_dsi_show_cabc();
	rc = snprintf((char *)buf, 1024, "%d\n",cabc);
	pr_info("%s :[MIPI2LVDS] CABC: %d\n", __func__, cabc);
	return rc;

}


static ssize_t cabc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{

	unsigned char cabc;
	cabc = mdss_dsi_show_cabc();

	if (sysfs_streq(buf, "1") && !cabc)
		cabc = true;
	else if (sysfs_streq(buf, "0") && cabc)
		cabc = false;
	else
		pr_info("%s: Invalid argument!!", __func__);
	mdss_dsi_store_cabc(cabc);

	return size;

}
static DEVICE_ATTR(cabc, 0664, cabc_show, cabc_store);
#endif

static ssize_t negative_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	DPRINT("called %s\n", __func__);
	return snprintf(buf, 256, "Current negative Value : %s\n",
		(mdnie_tun_state.negative == 0) ? "Disabled" : "Enabled");
}

static ssize_t negative_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	DPRINT
	    ("negative_store, input value = %d\n",
	     value);

	mdnie_tun_state.negative = value;

	mDNIe_set_negative(mdnie_tun_state.negative);
	DPRINT
	    ("negative_store, input value11 = %d\n",
	     value);
	return size;
}

static DEVICE_ATTR(negative, 0664,
		   negative_show,
		   negative_store);
static ssize_t accessibility_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
#if defined(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
	DPRINT("Current accessibility Mode : %s\n",
			accessibility_name[mdnie_tun_state.blind]);

	return snprintf(buf, 256, "Current accessibility Mode : %s\n",
			accessibility_name[mdnie_tun_state.blind]);

#else
	DPRINT("called %s\n", __func__);
	return snprintf(buf, 256, "%d\n", play_speed_1_5);
#endif
}

#if defined(CONFIG_SEC_MATISSE_PROJECT) || defined(CONFIG_MDP_NEGATIVE_SUPPORT)
static ssize_t accessibility_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int cmd_value;

	sscanf(buf, "%d", &cmd_value);

	if (cmd_value == NEGATIVE) {
		mdnie_tun_state.negative = mDNIe_NEGATIVE_ON;
		mdnie_tun_state.blind = ACCESSIBILITY_OFF;
	} else if (cmd_value == ACCESSIBILITY_OFF) {
		mdnie_tun_state.blind = ACCESSIBILITY_OFF;
		mdnie_tun_state.negative = mDNIe_NEGATIVE_OFF;
	} else
		pr_info("%s ACCESSIBILITY_MAX", __func__);
	is_negative_on();
	return size;
}
#else
static ssize_t accessibility_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int cmd_value;
	char buffer[MDNIE_COLOR_BLINDE_CMD] = {0,};
	int buffer2[MDNIE_COLOR_BLINDE_CMD/2] = {0,};
	int loop;
	char temp;
	struct msm_fb_data_type *mfd;
	mfd = mdnie_msd->mfd;

	sscanf(buf, "%d %x %x %x %x %x %x %x %x %x", &cmd_value,
		&buffer2[0], &buffer2[1], &buffer2[2], &buffer2[3], &buffer2[4],
		&buffer2[5], &buffer2[6], &buffer2[7], &buffer2[8]);

	for(loop = 0; loop < MDNIE_COLOR_BLINDE_CMD/2; loop++) {
		buffer2[loop] = buffer2[loop] & 0xFFFF;

		buffer[loop * 2] = (buffer2[loop] & 0xFF00) >> 8;
		buffer[loop * 2 + 1] = buffer2[loop] & 0xFF;
	}

	for(loop = 0; loop < MDNIE_COLOR_BLINDE_CMD; loop+=2) {
		temp = buffer[loop];
		buffer[loop] = buffer[loop + 1];
		buffer[loop + 1] = temp;
	}

	if (cmd_value == NEGATIVE) {
		mdnie_tun_state.negative = mDNIe_NEGATIVE_ON;
		mdnie_tun_state.blind = ACCESSIBILITY_OFF;
	} else if (cmd_value == COLOR_BLIND) {
		mdnie_tun_state.negative = mDNIe_NEGATIVE_OFF;
		mdnie_tun_state.blind = COLOR_BLIND;
#if defined(CONFIG_FB_MSM_MDSS_HX8394C_TFT_VIDEO_720P_PANEL)
	   memcpy(&COLOR_BLIND_4[22],buffer, MDNIE_COLOR_BLINDE_CMD);
#else
		memcpy(&COLOR_BLIND_2[MDNIE_COLOR_BLINDE_CMD],
				buffer, MDNIE_COLOR_BLINDE_CMD);
#endif
	} else if (cmd_value == ACCESSIBILITY_OFF) {
		mdnie_tun_state.blind = ACCESSIBILITY_OFF;
		mdnie_tun_state.negative = mDNIe_NEGATIVE_OFF;
	} else
		pr_info("%s ACCESSIBILITY_MAX", __func__);
	mutex_lock(&mfd->power_state);
	is_negative_on();
	mutex_unlock(&mfd->power_state);
	pr_info("%s cmd_value : %d", __func__, cmd_value);

	return size;
}
#endif

static DEVICE_ATTR(accessibility, 0664,
			accessibility_show,
			accessibility_store);


static struct class *mdnie_class;
struct device *tune_mdnie_dev;

void init_mdnie_class(void)
{

	DPRINT("start!\n");

	mdnie_class = class_create(THIS_MODULE, "mdnie");
	if (IS_ERR(mdnie_class))
		pr_err("Failed to create class(mdnie)!\n");

	tune_mdnie_dev =
	    device_create(mdnie_class, NULL, 0, NULL,
		  "mdnie");
	if (IS_ERR(tune_mdnie_dev))
		pr_err("Failed to create device(mdnie)!\n");

#if !defined(CONFIG_SEC_MATISSE_PROJECT)  && !defined(CONFIG_MDP_NEGATIVE_SUPPORT)
	if (device_create_file
	    (tune_mdnie_dev, &dev_attr_scenario) < 0)
		pr_err("Failed to create device file(%s)!\n",
	       dev_attr_scenario.attr.name);

	if (device_create_file
	    (tune_mdnie_dev,
	     &dev_attr_mdnieset_user_select_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_mdnieset_user_select_file_cmd.attr.name);

	if (device_create_file
	    (tune_mdnie_dev, &dev_attr_mdnieset_init_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_mdnieset_init_file_cmd.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_mode) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_mode.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_outdoor) < 0)
		pr_err("Failed to create device file(%s)!\n",
	       dev_attr_outdoor.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_playspeed) < 0)
		pr_err("Failed to create device file(%s)!=n",
			dev_attr_playspeed.attr.name);

	if (device_create_file(tune_mdnie_dev, &dev_attr_cabc) < 0) {
		pr_info("[mipi2lvds:ERROR] device_create_file(%s)\n",\
			dev_attr_cabc.attr.name);
	}
#endif
	if (device_create_file
		(tune_mdnie_dev, &dev_attr_accessibility) < 0)
		pr_err("Failed to create device file(%s)!=n",
			dev_attr_accessibility.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_negative) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_negative.attr.name);

	mdnie_tun_state.mdnie_enable = true;

	DPRINT("end!\n");
}
void mdnie_tft_init(struct mdss_samsung_driver_data *msd)
{
	mdnie_msd = msd;
	mutex_init(&mdnie_msd->lock);
}
