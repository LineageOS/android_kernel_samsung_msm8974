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

#if defined(CONFIG_SEC_VICTOR3GDSDTV_PROJECT)
#define EXPRESSWIRED
#endif
#if defined(CONFIG_BACKLIGHT_IC_KTD2801)
void __iomem *virt_mmss_gp0_base;
#define MMSS_GP0_BASE 0xFD8C3420
#define MMSS_GP0_SIZE  0x28
#endif

#ifdef EXPRESSWIRED
#include <linux/spinlock.h>
#include <linux/earlysuspend.h>
static DEFINE_SPINLOCK(bl_ctrl_lock);
#define GPIO_BL_CTRL 33
#define DATA_START 40
#define LOW_BIT_L 40
#define LOW_BIT_H 10
#define HIGH_BIT_L 10
#define HIGH_BIT_H 40
#define END_DATA_L 10
#define END_DATA_H 400
#endif

#if defined (CONFIG_LCD_CLASS_DEVICE)
#include <linux/lcd.h>
#endif
#include "mdss_fb.h"
#if defined(CONFIG_MDNIE_LITE_TUNING)
#include "mdnie_lite_tuning.h"
#endif
#if defined(CONFIG_MDNIE_VIDEO_ENHANCED)
#include "mdss_video_enhance.h"
#endif
#include "mdss_dsi.h"
#include "mdss_hx8369b_panel.h"
#include "dlog.h"

#define DDI_VIDEO_ENHANCE_TUNING
#if defined(DDI_VIDEO_ENHANCE_TUNING)
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#endif
#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
static int lcd_connected_status;
static char *panel_vendor = NULL;
int lcd_id;
#endif
#define DT_CMD_HDR 6
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
struct work_struct  err_fg_work;
static int err_fg_gpio;
static int esd_count;
static int err_fg_working;
#define ESD_DEBUG 1
#endif

#if defined(CONFIG_BACKLIGHT_IC_KTD253)
static int lcd_brightness = -1;
struct mutex bg_lock;
static DEFINE_SPINLOCK(bg_gpio_lock);
#endif

#if defined(CONFIG_BACKLIGHT_IC_KTD2801)
static int fisrt_boot_bl=1;
#endif

DEFINE_LED_TRIGGER(bl_led_trigger);

static struct mdss_samsung_driver_data msd;
extern int poweroff_charging;

int get_samsung_lcd_attached(void);

#ifdef EXPRESSWIRED
void ktd_backlight_set_brightness(int level)
{
	int i = 0;
	unsigned char brightness;
	int bit_map[8];
	brightness = level;
	printk("set_brightness : level(%d)\n", brightness);
	for(i = 0; i < 8; i++)
	{
		bit_map[i] = brightness & 0x01;
		brightness >>= 1;
	}
	spin_lock(&bl_ctrl_lock);
	gpio_set_value(GPIO_BL_CTRL, 1);
	udelay(DATA_START);
	for(i = 7; i >= 0; i--)
	{
		if(bit_map[i])
		{
			gpio_set_value(GPIO_BL_CTRL, 0);
			udelay(HIGH_BIT_L);
			gpio_set_value(GPIO_BL_CTRL, 1);
			udelay(HIGH_BIT_H);
		}
		else
		{
			gpio_set_value(GPIO_BL_CTRL, 0);
			udelay(LOW_BIT_L);
			gpio_set_value(GPIO_BL_CTRL, 1);
			udelay(LOW_BIT_H);
		}
	}
	gpio_set_value(GPIO_BL_CTRL, 0);
	udelay(END_DATA_L);
	gpio_set_value(GPIO_BL_CTRL, 1);
	udelay(END_DATA_H);
	spin_unlock(&bl_ctrl_lock);
	return;
}
#endif

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
#if !defined(CONFIG_BACKLIGHT_IC_KTD2801)
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

	pr_debug("%s: bklt_ctrl=%d pwm_period=%d pwm_gpio=%d pwm_lpg_chan=%d\n",
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
#endif
void mdss_dsi_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *cmds, int cnt,int flag)
{
	struct dcs_cmd_req cmdreq;
#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
	if (get_samsung_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}
#endif

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = cmds;
	cmdreq.cmds_cnt = cnt;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;


	mdss_dsi_cmdlist_put(ctrl, &cmdreq);

}

static void mdss_dsi_panel_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl,
			struct dsi_panel_cmds *pcmds)
{
	struct dcs_cmd_req cmdreq;
#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
	if (get_samsung_lcd_attached() == 0)
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


unsigned char mdss_dsi_panel_pwm_scaling(int level)
{
	unsigned char scaled_level;

	if (level >= MAX_BRIGHTNESS_LEVEL)
		scaled_level  = BL_MAX_BRIGHTNESS_LEVEL;
	else if (level >= MID_BRIGHTNESS_LEVEL) {
		scaled_level  = (level - MID_BRIGHTNESS_LEVEL) *
		(BL_MAX_BRIGHTNESS_LEVEL - BL_MID_BRIGHTNESS_LEVEL) / (MAX_BRIGHTNESS_LEVEL-MID_BRIGHTNESS_LEVEL) + BL_MID_BRIGHTNESS_LEVEL;
	} else if (level >= LOW_BRIGHTNESS_LEVEL) {
		scaled_level  = (level - LOW_BRIGHTNESS_LEVEL) *
		(BL_MID_BRIGHTNESS_LEVEL - BL_LOW_BRIGHTNESS_LEVEL) / (MID_BRIGHTNESS_LEVEL-LOW_BRIGHTNESS_LEVEL) + BL_LOW_BRIGHTNESS_LEVEL;
	}  else{
		if(poweroff_charging == 1)
			scaled_level  = level*BL_LOW_BRIGHTNESS_LEVEL/LOW_BRIGHTNESS_LEVEL;
		else
			scaled_level  = BL_MIN_BRIGHTNESS;
	}

	return scaled_level;
}

static char led_pwm1[2] = {0x51, 0xFF};	/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc backlight_cmd = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(led_pwm1)},
	led_pwm1
};

static void mdss_dsi_panel_bklt_dcs(struct mdss_dsi_ctrl_pdata *ctrl, int level)
{
	struct dcs_cmd_req cmdreq;

	pr_debug("%s: level=%d\n", __func__, level);
#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
	if (get_samsung_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}
#endif

	led_pwm1[1] = mdss_dsi_panel_pwm_scaling(level);

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = &backlight_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
}

void mdss_dsi_himax_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
		int rc=0;
		if (pdata == NULL) {
			pr_err("%s: Invalid input data\n", __func__);
			return;
		}

		ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
					panel_data);

		if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
			pr_debug("%s:%d, reset line not configured\n",
				   __func__, __LINE__);
			return;
		}

		pr_debug("%s: enable = %d\n", __func__, enable);
		if (enable) {

			rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->rst_gpio, 0,
						GPIO_CFG_OUTPUT,GPIO_CFG_PULL_UP,GPIO_CFG_8MA),
						GPIO_CFG_ENABLE);
			if (rc)
				pr_err("disabling rst_gpio failed, rc=%d\n",rc);

			mdelay(5);
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			msleep(20);
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			msleep(1);
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			msleep(120);

			if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
				pr_debug("%s: Panel Not properly turned OFF\n",
							__func__);
				ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
				pr_debug("%s: Reset panel done\n", __func__);
			}
		} else {

			rc = gpio_tlmm_config(GPIO_CFG(ctrl_pdata->rst_gpio, 0,
						GPIO_CFG_OUTPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
			if (rc)
				pr_err("disabling rst_gpio failed, rc=%d\n",rc);

			gpio_set_value((ctrl_pdata->rst_gpio), 0);

			msleep(1);

			if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
				gpio_set_value((ctrl_pdata->disp_en_gpio), 0);

			msleep(150);

		}

}

#if defined(CONFIG_BACKLIGHT_IC_KTD253)

static int lux_tbl[] = {
	 3,
	 4,  5,  6,  7,  8,  9, 10, 11, 12, 13,
	14, 15, 16, 17, 18, 19, 20, 21, 22,
	23, 24, 25, 26, 27, 28, 29, 30, 31, 32,  0,
};


static int get_candela_index(int bl_level)
{
	int backlightlevel;
	int cd;
		switch (bl_level) {
 		case 0:
			backlightlevel = 30;  /* 0 */
			break;
		case 1 ... 10:
			backlightlevel = 29;  /* 32 */ /* platform MIN 10 */
			break;
		case 11 ... 19:
			backlightlevel = 28;  /* 31 */
			break;
		case 20 ... 29:
			backlightlevel = 27;  /* 30 */
			break;
		case 30 ... 39:
			backlightlevel = 26;  /* 29 */
			break;
		case 40 ... 49:
			backlightlevel = 25;  /* 28 */
			break;
		case 50 ... 59:
			backlightlevel = 24;  /* 27 */
			break;
		case 60 ... 69:
			backlightlevel = 23;  /* 26 */
			break;
		case 70 ... 79:
			backlightlevel = 22;  /* 25 */
			break;
		case 80 ... 89:
			backlightlevel = 21;  /* 24 */
			break;
		case 90 ... 99:
			backlightlevel = 20;  /* 23 */
			break;
		case 100 ... 109:
			backlightlevel = 19;  /* 22 */
			break;
		case 110 ... 119:
			backlightlevel = 18;  /* 21 */
			break;
		case 120 ... 129:
			backlightlevel = 17;  /* 20 */
			break;
		case 130 ... 139:
			backlightlevel = 16;  /* 19 */
			break;
		case 140 ... 146:
			backlightlevel = 15;  /* 18 */ /* platform DEF 140 */
			break;
		case 147 ... 153:
			backlightlevel = 14;  /* 17 */
			break;
		case 154 ... 160:
			backlightlevel = 13;  /* 16 */
			break;
		case 161 ... 167:
			backlightlevel = 12;  /* 15 */
			break;
		case 168 ... 174:
			backlightlevel = 11;  /* 14 */
			break;
		case 175 ... 181:
			backlightlevel = 10;  /* 13 */
			break;
		case 182 ... 188:
			backlightlevel = 9;   /* 12 */
			break;
		case 189 ... 195:
			backlightlevel = 8;   /* 11 */
			break;
		case 196 ... 202:
			backlightlevel = 7 ;  /* 10 */
			break;
		case 203 ... 209:
			backlightlevel = 6;   /* 9 */
			break;
		case 210 ... 216:
			backlightlevel = 5;   /* 8 */
			break;
		case 217 ... 223:
			backlightlevel = 4;   /* 7 */
			break;
		case 224 ... 230:
			backlightlevel = 3;   /* 6 */
			break;
		case 231 ... 237:
			backlightlevel = 2;   /* 5 */
			break;
		case 238 ... 244:
			backlightlevel = 1;   /* 4 */
			break;
		case 245 ... 255:
			backlightlevel = 0;   /* 3 */ /* platform MAX 10 */
			break;
		default:
			backlightlevel = 29;  /* 32 */
			break;
		}

	cd = lux_tbl[backlightlevel];
	return cd;
}


void ktd253_set_brightness(int level, struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int pulse;
	int tune_level = 0;

	mutex_lock(&bg_lock);

	tune_level = level;

	if(level == 0){
		gpio_set_value((ctrl_pdata->bl_on_gpio), 0);
		lcd_brightness = tune_level;
		goto END;
	}

	if(tune_level != lcd_brightness)
	{
		if (!tune_level) {
			gpio_set_value(ctrl_pdata->bl_on_gpio, 0);
			mdelay(3);
			lcd_brightness = tune_level;
		} else {
			unsigned long irq_flags;

			if (unlikely(lcd_brightness < 0)) {
				int val = gpio_get_value(ctrl_pdata->bl_on_gpio);
				if (val) {
					lcd_brightness = 0;
				gpio_set_value(ctrl_pdata->bl_on_gpio, 0);
				mdelay(3);
					pr_info("LCD Baklight init in boot time on kernel\n");
				}
			}
			if (!lcd_brightness) {
				gpio_set_value(ctrl_pdata->bl_on_gpio, 1);
				udelay(3);
				lcd_brightness = 1;
			}

			pulse = (tune_level - lcd_brightness + MAX_BRIGHTNESS_IN_BLU)
							% MAX_BRIGHTNESS_IN_BLU;

			pr_err("lcd_brightness = %d, tune_level = %d,  pulse = %d\n",lcd_brightness,tune_level,pulse);

			spin_lock_irqsave(&bg_gpio_lock, irq_flags);
			for (; pulse > 0; pulse--) {

				gpio_set_value(ctrl_pdata->bl_on_gpio, 0);
				udelay(3);
				gpio_set_value(ctrl_pdata->bl_on_gpio, 1);
				udelay(3);
			}
			spin_unlock_irqrestore(&bg_gpio_lock, irq_flags);

			lcd_brightness = tune_level;

		}
	}
	mdelay(1);

END:
	mutex_unlock(&bg_lock);
}
#endif

#if defined(CONFIG_BACKLIGHT_IC_KTD2801)
extern void mdss_dsi_panel_bklt_pwm( int level);
#endif
static void mdss_dsi_panel_bl_ctrl(struct mdss_panel_data *pdata,
							u32 bl_level)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

#if  defined(CONFIG_BACKLIGHT_IC_KTD253)
	static int bl_level_old;
#endif

#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
	if (get_samsung_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
#if defined(CONFIG_BACKLIGHT_IC_KTD2801)
		gpio_set_value(msd.bl_ap_pwm,0);
#else
		gpio_set_value(12, 0);
#endif
		return;
	}
#endif

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	if( msd.mfd->panel_power_on == false){
		pr_err("%s: panel power off no bl ctrl\n", __func__);
		return;
	}
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	if (err_fg_working) {
		pr_info("[LCD] %s : esd is working!! return.. \n", __func__);
		return;
	}
#endif
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	switch (ctrl_pdata->bklt_ctrl) {
	case BL_WLED:
		led_trigger_event(bl_led_trigger, bl_level);
		break;
	case BL_PWM:
#if defined(CONFIG_BACKLIGHT_IC_KTD2801)
#ifdef EXPRESSWIRED
		ktd_backlight_set_brightness(bl_level);
#else
		pr_err("[MINO] %s --> bl_level[1] : %d\n",__func__, bl_level );
		bl_level=mdss_dsi_panel_pwm_scaling(bl_level);
		mdss_dsi_panel_bklt_pwm(bl_level);
		pr_err("[MINO] %s --> bl_level[2] : %d\n",__func__, bl_level );
#endif
#else
		mdss_dsi_panel_bklt_pwm(ctrl_pdata, bl_level);
#endif
		break;
	case BL_DCS_CMD:
			mdss_dsi_panel_bklt_dcs(ctrl_pdata, bl_level);
		break;
	case BL_GPIO_SWING:
#if defined(CONFIG_BACKLIGHT_IC_KTD253)
	if (bl_level_old == bl_level)
			return;

	pr_err("%s: bl_level = %d\n",__func__,bl_level);

	ktd253_set_brightness(get_candela_index(bl_level),ctrl_pdata);
	bl_level_old = bl_level;
#endif
		break;

	default:
		pr_err("%s: Unknown bl_ctrl configuration\n",
			__func__);
		break;
	}
}

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
static irqreturn_t err_fg_irq_handler(int irq, void *handle)
{
	printk("%s : handler start", __func__);
	disable_irq_nosync(err_fg_gpio);
	schedule_work(&err_fg_work);
	printk("%s : handler end", __func__);

	return IRQ_HANDLED;
}
static void err_fg_work_func(struct work_struct *work)
{
	int bl_backup;
	struct mdss_panel_data *pdata = msd.mpd;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if(msd.mfd == NULL){
		pr_err("%s: mfd not initialized Skip ESD recovery\n", __func__);
		return;
	}
	if(pdata == NULL){
		pr_err("%s: pdata not available... skipping update\n", __func__);
		return;
	}
	bl_backup=msd.mfd->bl_level;
	if( msd.mfd->panel_power_on == false){
		pr_err("%s: Display off Skip ESD recovery\n", __func__);
		return;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
						panel_data);

	printk("%s : start", __func__);
	err_fg_working = 1;
	gpio_direction_output(ctrl_pdata->bklt_ctrl, 0);
	msd.mfd->fbi->fbops->fb_blank(FB_BLANK_POWERDOWN, msd.mfd->fbi);
	msd.mfd->fbi->fbops->fb_blank(FB_BLANK_UNBLANK, msd.mfd->fbi);
	esd_count++;
	err_fg_working = 0;
	msd.mfd->bl_level=bl_backup;
	mdss_dsi_panel_bl_ctrl(pdata,msd.mfd->bl_level);
	printk("%s : end", __func__);
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


static int mdss_dsi_panel_on(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	static int first_boot = 1;
#endif
	msd.mfd = (struct msm_fb_data_type *)registered_fb[0]->par;
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	msd.mpd = pdata;
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	printk("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);

	if (ctrl->on_cmds.cmd_cnt)
		mdss_dsi_panel_cmds_send(ctrl, &ctrl->on_cmds);

	msd.mfd->resume_state = MIPI_RESUME_STATE;

#if defined(CONFIG_MDNIE_LITE_TUNING) || defined(CONFIG_MDNIE_VIDEO_ENHANCED)
	is_negative_on();
#endif
#if defined(CONFIG_BACKLIGHT_IC_KTD2801)
	mdelay(2);
	if (gpio_is_valid(msd.bl_ap_pwm)) {
		gpio_tlmm_config(GPIO_CFG(msd.bl_ap_pwm,3, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
	if((fisrt_boot_bl==1) && (lcd_connected_status ==1)){

#ifdef EXPRESSWIRED
		ktd_backlight_set_brightness(BL_MAX_BRIGHTNESS_LEVEL);
#else
		mdss_dsi_panel_bklt_pwm(BL_MAX_BRIGHTNESS_LEVEL);
#endif
		fisrt_boot_bl=0;
	}
#endif
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	if(first_boot) {
		int rc;

		INIT_WORK(&err_fg_work, err_fg_work_func);

		rc = request_threaded_irq(err_fg_gpio, NULL, err_fg_irq_handler,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT, "esd_detect", NULL);
		if (rc) {
			pr_err("%s : Failed to request_irq. :ret=%d", __func__, rc);
		}
		first_boot = 0;
	}

	enable_irq(err_fg_gpio);
#endif
	pr_err("%s:-\n", __func__);
	return 0;
}

static int mdss_dsi_panel_off(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	msd.mfd = (struct msm_fb_data_type *)registered_fb[0]->par;
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	if (!err_fg_working) {
		disable_irq_nosync(err_fg_gpio);
		cancel_work_sync(&err_fg_work);
	}
#endif
	printk("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);
#if defined(CONFIG_BACKLIGHT_IC_KTD2801)
	if (gpio_is_valid(msd.bl_ap_pwm)) {
			gpio_tlmm_config(GPIO_CFG(msd.bl_ap_pwm,0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
			gpio_set_value(msd.bl_ap_pwm, 0);
	}
#ifdef EXPRESSWIRED
	ktd_backlight_set_brightness(0);
#endif
#endif

	if (ctrl->off_cmds.cmd_cnt)
		mdss_dsi_panel_cmds_send(ctrl, &ctrl->off_cmds);

	msd.mfd->resume_state = MIPI_SUSPEND_STATE;
	pr_debug("%s:-\n", __func__);

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
		printk("%s:%d Panel does not support FBC.\n",
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
		u32 tmp;
		int rc, i, len;
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

		rc = of_property_read_u32(np,
			"qcom,mdss-pan-physical-width-dimension", &tmp);
		pinfo->physical_width = (!rc ? tmp : 0);
		rc = of_property_read_u32(np,
			"qcom,mdss-pan-physical-height-dimension", &tmp);
		pinfo->physical_height = (!rc ? tmp : 0);
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
#ifdef EXPRESSWIRED
				spin_lock(&bl_ctrl_lock);
				gpio_set_value(GPIO_BL_CTRL, 0);
				udelay(1500);
				udelay(1500);
				gpio_set_value(GPIO_BL_CTRL, 1);
				udelay(200);
				gpio_set_value(GPIO_BL_CTRL, 0);
				udelay(300);
				gpio_set_value(GPIO_BL_CTRL, 1);
				udelay(400);
				spin_unlock(&bl_ctrl_lock);
#endif

#if !defined(CONFIG_BACKLIGHT_IC_KTD2801)
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
#endif
#if defined(CONFIG_BACKLIGHT_IC_KTD2801)
#ifndef EXPRESSWIRED
				msd.bl_ap_pwm= of_get_named_gpio(np,
						"qcom,bl-wled", 0);
				if (!gpio_is_valid(msd.bl_ap_pwm)) {
				pr_err("%s:%d, bl_ap_pwm gpio not specified\n",
							__func__, __LINE__);
				} else {
					rc = gpio_request(msd.bl_ap_pwm, "bl_ap_pwm");
					if (rc) {
						pr_err("request bl_ap_pwm gpio failed, rc=%d\n",rc);
						gpio_free(msd.bl_ap_pwm);
					}else{
						rc = gpio_tlmm_config(GPIO_CFG(msd.bl_ap_pwm, 0,
						GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_8MA),GPIO_CFG_ENABLE);
					if (rc)
						pr_err("request bl_ap_pwm failed, rc=%d\n",rc);
					}
				}
#endif
#endif

			} else if (!strncmp(data, "bl_ctrl_dcs", 11)) {
				ctrl_pdata->bklt_ctrl = BL_DCS_CMD;
			}
#if defined(CONFIG_BACKLIGHT_IC_KTD253)
		else if (!strncmp(data, "bl_ctrl_gpio_swing", 18)) {
			ctrl_pdata->bklt_ctrl = BL_GPIO_SWING;
		}
#endif
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

		data = of_get_property(np,
			"qcom,platform-strength-ctrl", &len);
		if ((!data) || (len != 2)) {
			pr_err("%s:%d, Unable to read Phy Strength ctrl settings",
				__func__, __LINE__);
			return -EINVAL;
		}
		pinfo->mipi.dsi_phy_db.strength[0] = data[0];
		pinfo->mipi.dsi_phy_db.strength[1] = data[1];

		data = of_get_property(np,
			"qcom,platform-regulator-settings", &len);
		if ((!data) || (len != 7)) {
			pr_err("%s:%d, Unable to read Phy regulator settings",
				__func__, __LINE__);
			return -EINVAL;
		}
		for (i = 0; i < len; i++) {
			pinfo->mipi.dsi_phy_db.regulator[i]
				= data[i];
		}

		data = of_get_property(np, "qcom,mdss-dsi-panel-timings", &len);
		if ((!data) || (len != 12)) {
			pr_err("%s:%d, Unable to read Phy timing settings",
				   __func__, __LINE__);
			goto error;
		}
		for (i = 0; i < len; i++)
			pinfo->mipi.dsi_phy_db.timing[i] = data[i];

		pinfo->mipi.lp11_init = of_property_read_bool(np,
						"qcom,mdss-dsi-lp11-init");
		rc = of_property_read_u32(np, "qcom,mdss-dsi-init-delay-us", &tmp);
		pinfo->mipi.init_delay = (!rc ? tmp : 0);

		mdss_dsi_parse_fbc_params(np, pinfo);

		if(lcd_id == 0x55bc90){  //BOE
		mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->on_cmds,
			"qcom,mdss-dsi-on-command", "qcom,mdss-dsi-on-command-state");
		}else if(lcd_id == 0x558cc0){  //SDC
			mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->on_cmds,
					"qcom,mdss-dsi-on-sdc-command", "qcom,mdss-dsi-on-command-state");
		}else{
			mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->on_cmds,
					"qcom,mdss-dsi-on-command", "qcom,mdss-dsi-on-command-state");
		}

		mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->off_cmds,
			"qcom,mdss-dsi-off-command", "qcom,mdss-dsi-off-command-state");
		return 0;
	error:
		return -EINVAL;
}

#if defined(CONFIG_LCD_CLASS_DEVICE)

static ssize_t mdss_himax_disp_get_power(struct device *dev,
			struct device_attribute *attr, char *buf)
{

	pr_info("mipi_samsung_disp_get_power(0)\n");

	return 0;
}

static ssize_t mdss_himax_disp_set_power(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int power;
	if (sscanf(buf, "%u", &power) != 1)
		return -EINVAL;

	pr_info("mipi_samsung_disp_set_power:%d\n",power);

	return size;
}

static DEVICE_ATTR(lcd_power, S_IRUGO | S_IWUSR | S_IWGRP,
			mdss_himax_disp_get_power,
			mdss_himax_disp_set_power);

static ssize_t mdss_siop_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf(buf, 2, "%d\n",msd.dstat.siop_status);
	pr_info("%s :[MDSS_SHARP] CABC: %d\n", __func__, msd.dstat.siop_status);
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


static struct lcd_ops mdss_himax_disp_props = {

	.get_power = NULL,
	.set_power = NULL,

};

static ssize_t mdss_himax_auto_brightness_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf(buf, sizeof(buf), "%d\n",
					msd.dstat.auto_brightness);
	pr_info("%s : auto_brightness : %d\n", __func__, msd.dstat.auto_brightness);

	return rc;
}

static ssize_t mdss_himax_auto_brightness_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	static unsigned char prev_auto_brightness;

#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
		if (get_samsung_lcd_attached() == 0)
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

	if((msd.dstat.auto_brightness >=5 )|| (msd.dstat.auto_brightness == 0 ))
		msd.dstat.siop_status = false;
	else
		msd.dstat.siop_status = true;
	if(msd.mfd == NULL){
		pr_err("%s: mfd not initialized\n", __func__);
		return size;
	}

	if( msd.mfd->panel_power_on == false){
		pr_err("%s: panel power off no bl ctrl\n", __func__);
		return size;
	}

	prev_auto_brightness = msd.dstat.auto_brightness;
	pr_info("%s %d %d\n", __func__, msd.dstat.auto_brightness, msd.dstat.siop_status);
	return size;
}

static DEVICE_ATTR(auto_brightness, S_IRUGO | S_IWUSR | S_IWGRP,
			mdss_himax_auto_brightness_show,
			mdss_himax_auto_brightness_store);

static ssize_t mdss_disp_lcdtype_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char temp[20];

	snprintf(temp, 20, "INH_55BC90");
	strncat(buf, temp, 20);
	return strnlen(buf, 20);
}
static DEVICE_ATTR(lcd_type, S_IRUGO, mdss_disp_lcdtype_show, NULL);
#endif

#ifdef DDI_VIDEO_ENHANCE_TUNING
#define MAX_FILE_NAME 128
#define TUNING_FILE_PATH "/sdcard/"
#define TUNE_FIRST_SIZE 113
static char tuning_file[MAX_FILE_NAME];
static char mdni_tuning1[TUNE_FIRST_SIZE] ={ 0xCD, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static struct dsi_cmd_desc mdni_tune_cmd[] = {
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
	struct mdss_panel_data *pdata = msd.mpd;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
						panel_data);

	cmd_step = 0;
	cmd_pos = 1;
	for (data_pos = 0; data_pos < len;) {
		if (*(src + data_pos) == '0') {
			if (*(src + data_pos + 1) == 'x') {

				mdni_tuning1[cmd_pos] =
				char_to_dec(*(src + data_pos + 2),
						*(src + data_pos + 3));

				data_pos += 3;
				cmd_pos++;
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

	mdss_dsi_cmds_send(ctrl_pdata, mdni_tune_cmd, ARRAY_SIZE(mdni_tune_cmd),0);
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

	ret = snprintf(buf, MAX_FILE_NAME, "tuned file name : %s\n",
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
static DEVICE_ATTR(tuning, S_IRUGO | S_IWUSR | S_IWGRP,
			tuning_show,
			tuning_store);
#endif
#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
int get_samsung_lcd_attached(void)
{
	return lcd_connected_status;
}
EXPORT_SYMBOL(get_samsung_lcd_attached);

static int __init detect_lcd_panel_vendor(char* read_id)
{
	lcd_id = simple_strtol(read_id, NULL, 16);

	if(lcd_id == 0x55bc90)
		panel_vendor = "BOE panel";
	if(lcd_id == 0x558cc0)
		panel_vendor = "SDC panel";
	else{
		panel_vendor = "XX panel";
		pr_info("%s: manufacture id read may be faulty id[0x%x]\n", __func__, lcd_id);
	}

	lcd_connected_status = ((lcd_id&0xFFFFFF)!=0x000000);

	pr_err("%s: detected panel vendor --> %s [0x%x] lcd_attached = %d\n", __func__, panel_vendor, lcd_id,lcd_connected_status);

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
#if defined(CONFIG_BACKLIGHT_IC_KTD253)
	mutex_init(&bg_lock);
#endif

#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
	printk("%s: LCD attached status: %d !\n",
				__func__, get_samsung_lcd_attached());
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

#if defined(CONFIG_BACKLIGHT_IC_KTD2801)
	virt_mmss_gp0_base = ioremap(MMSS_GP0_BASE,MMSS_GP0_SIZE);
	if(virt_mmss_gp0_base == NULL) {
		pr_err("%s: I/O remap failed \n", __func__);
		return 0;
	}
#endif

	rc = mdss_panel_parse_dt(node, ctrl_pdata);
	if (rc) {
		pr_err("%s:%d panel dt parse failed\n", __func__, __LINE__);
		return rc;
	}

#if defined(CONFIG_SEC_VICTOR3GDSDTV_PROJECT)
		gpio_tlmm_config(GPIO_CFG(34, 0, GPIO_CFG_INPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_8MA),GPIO_CFG_ENABLE);
#else
		gpio_tlmm_config(GPIO_CFG(24, 0, GPIO_CFG_INPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_8MA),GPIO_CFG_ENABLE);
		gpio_set_value(24, 0);
#endif

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


#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
	pr_err("CONFIG_LCD_CONNECTION_CHECK is set kky\n");
	if (get_samsung_lcd_attached() == 0)
	{
		printk("%s: aaaLCD not connected.... Disabling Continous Splash!\n",__func__);
//		ctrl_pdata->panel_data.panel_info.cont_splash_enabled = 0;
	}
#endif

	ctrl_pdata->on = mdss_dsi_panel_on;
	ctrl_pdata->off = mdss_dsi_panel_off;
	ctrl_pdata->panel_reset = mdss_dsi_himax_panel_reset;
	ctrl_pdata->panel_data.set_backlight = mdss_dsi_panel_bl_ctrl;

#if defined(CONFIG_LCD_CLASS_DEVICE)
	lcd_device = lcd_device_register("panel", &pdev->dev, NULL,
					&mdss_himax_disp_props);

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
#if defined(DDI_VIDEO_ENHANCE_TUNING)
	rc = sysfs_create_file(&lcd_device->dev.kobj,
			&dev_attr_tuning.attr);
	if (rc) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_tuning.attr.name);
	}
#endif
#if defined(CONFIG_MDNIE_LITE_TUNING) || defined(CONFIG_MDNIE_VIDEO_ENHANCED)
		pr_info("[%s] CONFIG_MDNIE_LITE_TUNING ok ! initclass called!\n",__func__);
		init_mdnie_class();
#if defined(CONFIG_MDNIE_LITE_TUNING)
		mdnie_lite_tuning_init(&msd);
#endif
#endif
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

