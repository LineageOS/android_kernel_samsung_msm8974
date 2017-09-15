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
#ifdef CONFIG_LCD_CLASS_DEVICE
	#include <linux/lcd.h>
#endif
#include "mdss_fb.h"
#include "mdss_dsi.h"
#include "mdss_samsung_dsi_panel_msm8x26.h"
#if defined(CONFIG_MDNIE_TFT_MSM8X26)
#include "mdnie_tft_msm8x26.h"
#endif

#if defined(DDI_VIDEO_ENHANCE_TUNING)
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#endif

static int lcd_attached;
static int lcd_id;
int get_lcd_attached(void);

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
struct work_struct  err_fg_work;
static int err_fg_gpio;
static int esd_count;
static int err_fg_working;
#define ESD_DEBUG 1
#endif
DEFINE_LED_TRIGGER(bl_led_trigger);
static struct mdss_samsung_driver_data msd;

#if SINGLE_WIRE_BL_CTRL

#define MIN_BRIGHTNESS_VALUE 20
#define MAX_BRIGHTNESS_VALUE 255
#define DIMMING_VALUE		32
struct brt_value{
	int level;				/* Platform setting values */
	int tune_level;	      /* Chip Setting values */
};

struct brt_value brt_table_ktd[] = {
	{ MIN_BRIGHTNESS_VALUE, 32 }, /*Min pulse 32 */ 
	{ 22, 31 }, 
	{ 34, 30 },
	{ 45, 29},
	{ 55, 28 },
	{ 65, 27},
	{ 75, 26 },
	{ 85, 25 },
	{ 95, 24 },
	{ 115, 23 },
	{ 125, 22 },
	{ 135, 21 },
	{ 145, 20 },
	{ 155, 19 },
	{ 165, 18 }, /* Default */
	{ 175, 17 },
	{ 185, 16 },
	{ 195, 15 },
	{ 205, 14 },
	{ 215, 13 },
	{ 225, 12 },
	{ 235, 11 },
	{ 245, 10 },
	{ MAX_BRIGHTNESS_VALUE, 9 },
};

#define MAX_BRT_STAGE_KTD (int)(sizeof(brt_table_ktd)/sizeof(struct brt_value))

spinlock_t bl_lock;
static void lcd_backlight_control(int num)
{
    volatile int limit;
    limit = num;

spin_lock(&bl_lock);
    for(;limit>0;limit--)
    {
       udelay(2);
       gpio_set_value(msd.lcd_on_gpio,0);
       udelay(2);
       gpio_set_value(msd.lcd_on_gpio,1);
    }
spin_unlock(&bl_lock);
}

int real_level = 14;

static int swire_set_backlight(int user_intensity)
{
	int tune_level = 0;
	int pulse;
	int i;
	if(user_intensity > 0) {
        if(user_intensity < MIN_BRIGHTNESS_VALUE) {
			tune_level = DIMMING_VALUE; /* DIMMING */
		}else if (user_intensity == MAX_BRIGHTNESS_VALUE)
        {
        	tune_level = brt_table_ktd[MAX_BRT_STAGE_KTD-1].tune_level;
		}else
        {
             for(i = 0; i < MAX_BRT_STAGE_KTD; i++) {
                    if(user_intensity <= brt_table_ktd[i].level ) {
                        tune_level = brt_table_ktd[i].tune_level;
                        break;
					}
			}
		}
	}else{
		 pr_err("%s: [ZERO]user_intensity :%d\n",__func__,user_intensity);
		gpio_set_value(msd.lcd_on_gpio,0);
		real_level = 0;
		return 0;
	}
	if (real_level==tune_level){
        return 0;
    }else{
        if(tune_level<=0){
            pr_err("%s: invalid tune level\n",__func__);
			return 0;
        }else{
	        if( real_level <= tune_level){
                pulse = tune_level - real_level;
            }else{
                pulse = 32 - (real_level - tune_level);
            }

            if (pulse==0){
                return 0;
			}
			pr_err("%s: [BACKLIGHT]: update_status ==>user_intensity:%d   tune_level : %d 	real_level : %d & pulse = %d\n",__func__,user_intensity, tune_level, real_level, pulse);
            lcd_backlight_control(pulse);
		}

        real_level = tune_level;
        return 0;
    }

}

#endif
//extern int poweroff_charging;
void mdss_dsi_panel_pwm_cfg(struct mdss_dsi_ctrl_pdata *ctrl)
{
	ctrl->pwm_bl = pwm_request(ctrl->pwm_lpg_chan, "lcd-bklt");
	if (ctrl->pwm_bl == NULL || IS_ERR(ctrl->pwm_bl)) {
		pr_err("%s: Error: lpg_chan=%d pwm request failed",
				__func__, ctrl->pwm_lpg_chan);
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

	if (level == 0) {
		if (ctrl->pwm_enabled)
			pwm_disable(ctrl->pwm_bl);
		ctrl->pwm_enabled = 0;
		return;
	}

	duty = level * ctrl->pwm_period;
	duty /= ctrl->bklt_max;

	pr_debug("%s: bklt_ctrl=%d pwm_period=%d pwm_gpio=%d pwm_lpg_chan=%d\n",
			__func__, ctrl->bklt_ctrl, ctrl->pwm_period,
				ctrl->pwm_pmic_gpio, ctrl->pwm_lpg_chan);

	pr_debug("%s: ndx=%d level=%d duty=%d\n", __func__,
					ctrl->ndx, level, duty);

	if (ctrl->pwm_enabled) {
		pwm_disable(ctrl->pwm_bl);
		ctrl->pwm_enabled = 0;
	}

	ret = pwm_config(ctrl->pwm_bl, duty, ctrl->pwm_period);
	if (ret) {
		pr_err("%s: pwm_config() failed err=%d.\n", __func__, ret);
		return;
	}

	ret = pwm_enable(ctrl->pwm_bl);
	if (ret)
		pr_err("%s: pwm_enable() failed err=%d\n", __func__, ret);
	ctrl->pwm_enabled = 1;
}

static char dcs_cmd[2] = {0x54, 0x00}; /* DTYPE_DCS_READ */
static struct dsi_cmd_desc dcs_read_cmd = {
	{DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(dcs_cmd)},
	dcs_cmd
};

u32 mdss_dsi_dcs_read(struct mdss_dsi_ctrl_pdata *ctrl,
			char cmd0, char cmd1)
{
	struct dcs_cmd_req cmdreq;
	if (get_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return 0;
	}
	dcs_cmd[0] = cmd0;
	dcs_cmd[1] = cmd1;
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = &dcs_read_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_RX | CMD_REQ_COMMIT;
	cmdreq.rlen = 1;
	cmdreq.cb = NULL; /* call back */
	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
	/*
	 * blocked here, until call back called
	 */

	return 0;
}
void mdss_dsi_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_cmd_desc *cmds, int cnt,int flag)
{
	struct dcs_cmd_req cmdreq;
	if (get_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}
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
	if (get_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}
	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = pcmds->cmds;
	cmdreq.cmds_cnt = pcmds->cmd_cnt;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
}

#if !SINGLE_WIRE_BL_CTRL
unsigned char mdss_dsi_panel_pwm_scaling(int level)
{
	unsigned char scaled_level;

	if (level >= MAX_BRIGHTNESS_LEVEL)
		scaled_level  = BL_MAX_BRIGHTNESS_LEVEL;
	else if (level >= MID_BRIGHTNESS_LEVEL) {
		scaled_level  = (level - MID_BRIGHTNESS_LEVEL) *
		(BL_MAX_BRIGHTNESS_LEVEL - BL_MID_BRIGHTNESS_LEVEL) / (MAX_BRIGHTNESS_LEVEL-MID_BRIGHTNESS_LEVEL) + BL_MID_BRIGHTNESS_LEVEL;
	} else if (level >= DIM_BRIGHTNESS_LEVEL) {
		scaled_level  = (level - DIM_BRIGHTNESS_LEVEL) *
		(BL_MID_BRIGHTNESS_LEVEL - BL_DIM_BRIGHTNESS_LEVEL) / (MID_BRIGHTNESS_LEVEL-DIM_BRIGHTNESS_LEVEL) + BL_DIM_BRIGHTNESS_LEVEL;
	} else if (level >= LOW_BRIGHTNESS_LEVEL) {
		scaled_level  = (level - LOW_BRIGHTNESS_LEVEL) *
		(BL_DIM_BRIGHTNESS_LEVEL - BL_LOW_BRIGHTNESS_LEVEL) / (DIM_BRIGHTNESS_LEVEL-LOW_BRIGHTNESS_LEVEL) + BL_LOW_BRIGHTNESS_LEVEL;
	}  else{
		scaled_level  = BL_MIN_BRIGHTNESS;
	}

	pr_info("%s  level = [%d]: scaled_level = [%d] \n",__func__,level,scaled_level);

	return scaled_level;
}

static char led_pwm1[2] = {0x51, 0x0};	/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc backlight_cmd = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(led_pwm1)},
	led_pwm1
};

static void mdss_dsi_panel_bklt_dcs(struct mdss_dsi_ctrl_pdata *ctrl, int level)
{
	struct dcs_cmd_req cmdreq;
	pr_debug("%s: level=%d\n", __func__, level);
	if (get_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}
	led_pwm1[1] = mdss_dsi_panel_pwm_scaling(level);

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = &backlight_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
}
#endif

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
	if (get_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}
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
extern int system_rev;
void mdss_dsi_cpt_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
		return;
	}

	pr_info("%s:enable = %d\n", __func__, enable);
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {
		mdelay(20);
		if (gpio_is_valid(msd.lcd_en_gpio)){
			gpio_tlmm_config(GPIO_CFG(msd.lcd_en_gpio, 0,
				GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
			gpio_set_value(msd.lcd_en_gpio,1);
		}
		mdelay(1);
		if (gpio_is_valid(ctrl_pdata->rst_gpio)) {
			gpio_tlmm_config(GPIO_CFG(ctrl_pdata->rst_gpio, 0,
						GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_8MA),
						GPIO_CFG_ENABLE);
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			msleep(20);
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			msleep(1);
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			msleep(20);
		}
		if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
			if (pinfo->mode_gpio_state == MODE_GPIO_HIGH)
				gpio_set_value((ctrl_pdata->mode_gpio), 1);
			else if (pinfo->mode_gpio_state == MODE_GPIO_LOW)
				gpio_set_value((ctrl_pdata->mode_gpio), 0);
		}
		if( system_rev >= 5)
			if (gpio_is_valid(msd.lcd_io_1p8_en_gpio)) {
			gpio_tlmm_config(GPIO_CFG(msd.lcd_io_1p8_en_gpio, 0,
						GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_8MA),
						GPIO_CFG_ENABLE);
			gpio_set_value((msd.lcd_io_1p8_en_gpio), 1);
		}
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
			gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
						__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
		gpio_tlmm_config(GPIO_CFG(ctrl_pdata->rst_gpio, 0,
				GPIO_CFG_OUTPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
				GPIO_CFG_DISABLE);
		if (gpio_is_valid(ctrl_pdata->rst_gpio))
			gpio_set_value((ctrl_pdata->rst_gpio), 0);

		gpio_tlmm_config(GPIO_CFG(msd.lcd_on_gpio, 0,
				GPIO_CFG_OUTPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
				GPIO_CFG_DISABLE);
		if (gpio_is_valid(msd.lcd_on_gpio))
			gpio_set_value((msd.lcd_on_gpio), 0);

		if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
		if(system_rev != 1){
			if (gpio_is_valid(msd.lcd_en_gpio)){

				gpio_tlmm_config(GPIO_CFG(msd.lcd_en_gpio, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
					GPIO_CFG_DISABLE);
				gpio_set_value(msd.lcd_en_gpio,0);
			}
		}
		if( system_rev >= 5)
			if (gpio_is_valid(msd.lcd_io_1p8_en_gpio)) {
				gpio_tlmm_config(GPIO_CFG(msd.lcd_io_1p8_en_gpio, 0,
					GPIO_CFG_OUTPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
					GPIO_CFG_DISABLE);
			gpio_set_value((msd.lcd_io_1p8_en_gpio), 0);
		}
	}
	return;
}

static char caset[] = {0x2a, 0x00, 0x00, 0x03, 0x00};	/* DTYPE_DCS_LWRITE */
static char paset[] = {0x2b, 0x00, 0x00, 0x05, 0x00};	/* DTYPE_DCS_LWRITE */

static struct dsi_cmd_desc partial_update_enable_cmd[] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(caset)}, caset},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(paset)}, paset},
};

static int mdss_dsi_panel_partial_update(struct mdss_panel_data *pdata)
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

static void mdss_dsi_panel_bl_ctrl(struct mdss_panel_data *pdata,
							u32 bl_level)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if (get_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	/*
	 * Some backlight controllers specify a minimum duty cycle
	 * for the backlight brightness. If the brightness is less
	 * than it, the controller can malfunction.
	 */

	if ((bl_level < pdata->panel_info.bl_min) && (bl_level != 0))
		bl_level = pdata->panel_info.bl_min;
	switch (ctrl_pdata->bklt_ctrl) {
	case BL_WLED:
		led_trigger_event(bl_led_trigger, bl_level);
		break;
	case BL_PWM:
		mdss_dsi_panel_bklt_pwm(ctrl_pdata, bl_level);
		break;
	case BL_DCS_CMD:
#if SINGLE_WIRE_BL_CTRL
		swire_set_backlight(bl_level);
#else
		mdss_dsi_panel_bklt_dcs(ctrl_pdata, bl_level);
#endif
		break;
	default:
		pr_err("%s: Unknown bl_ctrl configuration\n",
			__func__);
		break;
	}
}

static int mdss_dsi_panel_on(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	u32 tmp;
	msd.mfd = (struct msm_fb_data_type *)registered_fb[0]->par;
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	msd.mpd = pdata;
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	pr_debug("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);

	if (ctrl->on_cmds.cmd_cnt)
	{	
		mdss_dsi_panel_cmds_send(ctrl, &ctrl->on_cmds);

			tmp = MIPI_INP((ctrl->ctrl_base) + 0xac);
			tmp |= (1<<28);
			MIPI_OUTP((ctrl->ctrl_base) + 0xac, tmp);
			wmb();
		 mdss_dsi_panel_cmds_send(ctrl,&ctrl->disp_on_cmd);
	}
	msd.mfd->resume_state = MIPI_RESUME_STATE;

#if defined(CONFIG_LCD_CLASS_DEVICE)
	mdss_dsi_panel_cabc_dcs(ctrl, msd.dstat.siop_status);
#endif
#if defined(CONFIG_MDNIE_TFT_MSM8X26)
	is_negative_on();
#endif
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
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
#if SINGLE_WIRE_BL_CTRL
	real_level = 0;
#endif
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	if (!err_fg_working) {
		disable_irq_nosync(err_fg_gpio);
		cancel_work_sync(&err_fg_work);
	}
#endif
	pr_debug("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);
	if (ctrl->off_cmds.cmd_cnt)
		mdss_dsi_panel_cmds_send(ctrl, &ctrl->off_cmds);
	mdelay(2);

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
			kfree(buf);
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

static int mdss_panel_parse_dt_gpio(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;

	msd.lcd_en_gpio = of_get_named_gpio(np,
						     "qcom,lcd-en-gpio", 0);
	if (!gpio_is_valid(msd.lcd_en_gpio)) {
		pr_err("%s:%d lcd_en_gpio  not specified\n",
						__func__, __LINE__);
	} else {
		rc = gpio_request(msd.lcd_en_gpio, "lcd_enable");
		if (rc) {
			pr_err("request lcd_en_gpio   failed, rc=%d\n",
			       rc);
			gpio_free(msd.lcd_en_gpio);
		}
		else{
			rc = gpio_tlmm_config(GPIO_CFG(msd.lcd_en_gpio, 0,
						GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
			if(rc)
				pr_err("%s: tlmm config of lcd_en_gpio failed\n", __func__);
		}
	}

	msd.lcd_on_gpio = of_get_named_gpio(np, "qcom,lcd-on-gpio", 0);
	if (!gpio_is_valid(msd.lcd_on_gpio)) {
		pr_err("%s:%d, lcd_on gpio not specified\n",
						__func__, __LINE__);
	} else {
		rc = gpio_request(msd.lcd_on_gpio, "lcd_on");
		if (rc) {
			pr_err("request lcd_on gpio failed, rc=%d\n",
				rc);
			gpio_free(msd.lcd_on_gpio);

		}
		else{
			rc = gpio_tlmm_config(GPIO_CFG(msd.lcd_on_gpio, 0,
						GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
			if(rc)
				pr_err("%s: tlmm config of lcd_on_gpio failed\n", __func__);
		}
				
	
	}
	if( system_rev >= 5) {
		msd.lcd_io_1p8_en_gpio = of_get_named_gpio(np,
							     "qcom,lcd-io-1p8v-en-gpio", 0);
		if (!gpio_is_valid(msd.lcd_io_1p8_en_gpio)) {
			pr_err("%s:%d lcd_io_1p8_en_gpio  not specified\n",
							__func__, __LINE__);
		} else {
			rc = gpio_request(msd.lcd_io_1p8_en_gpio, "lcd_enable");
			if (rc) {
				pr_err("request lcd_io_1p8_en_gpio   failed, rc=%d\n",
				       rc);
				gpio_free(msd.lcd_io_1p8_en_gpio);
			}
			else{
				rc = gpio_tlmm_config(GPIO_CFG(msd.lcd_io_1p8_en_gpio, 0,
							GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
							GPIO_CFG_ENABLE);
				if(rc)
					pr_err("%s: tlmm config of lcd_io_1p8_en_gpio failed\n", __func__);
			}
		}
	}
	return 0;
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
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->on_cmds,
		"qcom,mdss-dsi-on-command", "qcom,mdss-dsi-on-command-state");
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->disp_on_cmd,
		"qcom,mdss-display-on-command", "qcom,mdss-dsi-on-command-state");
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->off_cmds,
		"qcom,mdss-dsi-off-command", "qcom,mdss-dsi-off-command-state");

	return 0;
error:
	return -EINVAL;
}

#if defined(CONFIG_LCD_CLASS_DEVICE)

static ssize_t mdss_dsi_disp_get_power(struct device *dev,
			struct device_attribute *attr, char *buf)
{

	pr_info("mipi_samsung_disp_get_power(0)\n");

	return 0;
}

static ssize_t mdss_dsi_disp_set_power(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int power;
	if (sscanf(buf, "%u", &power) != 1)
		return -EINVAL;

	pr_info("mipi_samsung_disp_set_power:%d\n",power);

	return size;
}

static DEVICE_ATTR(lcd_power, S_IRUGO | S_IWUSR | S_IWGRP,
			mdss_dsi_disp_get_power,
			mdss_dsi_disp_set_power);

static ssize_t mdss_siop_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf(buf, 2, "%d\n",msd.dstat.siop_status);
	pr_info("%s :[MDSS_SDC] CABC: %d\n", __func__, msd.dstat.siop_status);
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


static struct lcd_ops mdss_dsi_disp_props = {

	.get_power = NULL,
	.set_power = NULL,

};

static ssize_t mdss_disp_lcdtype_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char temp[20];

	snprintf(temp, 20, "INH_534490");
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
	if( msd.mfd->panel_power_on == false){
		pr_err("%s: panel power off no bl ctrl\n", __func__);
		return;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
						panel_data);


	msd.dstat.siop_status=cabc;
	mdss_dsi_panel_cabc_dcs(ctrl_pdata,msd.dstat.siop_status);
	pr_info("%s :[MDSS_CPT] CABC: %d\n", __func__,msd.dstat.siop_status);

}

#endif
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
static irqreturn_t err_fg_irq_handler(int irq, void *handle)
{
	pr_info("%s : handler start", __func__);
	disable_irq_nosync(err_fg_gpio);
	schedule_work(&err_fg_work);
	pr_info("%s : handler end", __func__);

	return IRQ_HANDLED;
}
static void err_fg_work_func(struct work_struct *work)
{
	int bl_backup, ret=0;
	struct mdss_panel_data *pdata = msd.mpd;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mipi_panel_info *mipi;
	char *envp[2] = {"PANEL_ALIVE=0", NULL};
	if(msd.mfd == NULL){
		pr_err("%s: mfd not initialized Skip ESD recovery\n", __func__);
		return;
	}
	if(pdata == NULL){
		pr_err("%s: pdata not available... skipping update\n", __func__);
		return;
	}
	bl_backup=msd.mfd->bl_level;
	mipi  = &pdata->panel_info.mipi;
	if( msd.mfd->panel_power_on == false){
		pr_err("%s: Display off Skip ESD recovery\n", __func__);
		return;
	}
	if(err_fg_working) {
		pr_err("%s: ESD refresh ongoing Skip ESD recovery\n", __func__);
		return;
	}
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
						panel_data);

	pr_info("%s : start", __func__);
	err_fg_working = 1;
	pdata->panel_info.panel_dead = true;
	ret = kobject_uevent_env(
			&msd.mfd->fbi->dev->kobj,
						KOBJ_CHANGE, envp);
	pr_err("%s: Panel has gone bad, sending uevent - %s\n",	__func__, envp[0]);
	mdss_dsi_panel_bl_ctrl(pdata, bl_backup);
	esd_count++;
	err_fg_working = 0;

	pr_info("%s : end", __func__);
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

#ifdef DDI_VIDEO_ENHANCE_TUNING
#define MAX_FILE_NAME 128
#define TUNING_FILE_PATH "/sdcard/"
#define TUNE_FIRST_SIZE 8
#define TUNE_SECOND_SIZE 17
#define TUNE_THIRD_SIZE 25
#define TUNE_FOURTH_SIZE 25
#define TUNE_FIFTH_SIZE 25
#define TUNE_SIXTH_SIZE 19

static char tuning_file[MAX_FILE_NAME];
static char mdni_tuning1[TUNE_FIRST_SIZE];
static char mdni_tuning2[TUNE_SECOND_SIZE];
static char mdni_tuning3[TUNE_THIRD_SIZE];
static char mdni_tuning4[TUNE_FOURTH_SIZE];
static char mdni_tuning5[TUNE_FIFTH_SIZE];
static char mdni_tuning6[TUNE_SIXTH_SIZE];

static struct dsi_cmd_desc mdni_tune_cmd[] = {
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(mdni_tuning6)}, mdni_tuning6},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(mdni_tuning5)}, mdni_tuning5},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(mdni_tuning4)}, mdni_tuning4},
	{{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(mdni_tuning3)}, mdni_tuning3},
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
	struct mdss_panel_data *pdata = msd.mpd;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
						panel_data);

	cmd_step = 0;
	cmd_pos = 0;

	for (data_pos = 0; data_pos < len;) {
		if (*(src + data_pos) == '0') {
			if (*(src + data_pos + 1) == 'x') {
				if (!cmd_step) {
					mdni_tuning1[cmd_pos] =
					char_to_dec(*(src + data_pos + 2),
							*(src + data_pos + 3));
				} else if(cmd_step == 1){
					mdni_tuning2[cmd_pos] =
					char_to_dec(*(src + data_pos + 2),
							*(src + data_pos + 3));
				} else if(cmd_step == 2){
					mdni_tuning3[cmd_pos] =
					char_to_dec(*(src + data_pos + 2),
							*(src + data_pos + 3));
				} else if(cmd_step == 3){
					mdni_tuning4[cmd_pos] =
					char_to_dec(*(src + data_pos + 2),
							*(src + data_pos + 3));
				} else if(cmd_step == 4){
					mdni_tuning5[cmd_pos] =
					char_to_dec(*(src + data_pos + 2),
							*(src + data_pos + 3));
				} else if(cmd_step == 5){
					mdni_tuning6[cmd_pos] =
					char_to_dec(*(src + data_pos + 2),
							*(src + data_pos + 3));
				}

				data_pos += 3;
				cmd_pos++;

				if (cmd_pos == TUNE_FIRST_SIZE && !cmd_step) {
					cmd_pos = 0;
					cmd_step = 1;
				}
				else if((cmd_pos == TUNE_SECOND_SIZE) && (cmd_step == 1)){
					cmd_pos = 0;
					cmd_step = 2;
				}
				else if((cmd_pos == TUNE_THIRD_SIZE) && (cmd_step == 2)){
					cmd_pos = 0;
					cmd_step = 3;
				}
				else if((cmd_pos == TUNE_FOURTH_SIZE) && (cmd_step == 3)){
					cmd_pos = 0;
					cmd_step = 4;
				}
				else if((cmd_pos == TUNE_FIFTH_SIZE) && (cmd_step == 4)){
					cmd_pos = 0;
					cmd_step = 5;
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
	for (data_pos = 0; data_pos < TUNE_THIRD_SIZE ; data_pos++)
		printk(KERN_INFO"0x%x ", mdni_tuning3[data_pos]);
	printk(KERN_INFO "\n");
	for (data_pos = 0; data_pos < TUNE_FOURTH_SIZE ; data_pos++)
		printk(KERN_INFO"0x%x ", mdni_tuning4[data_pos]);
	printk(KERN_INFO "\n");
	for (data_pos = 0; data_pos < TUNE_FIFTH_SIZE ; data_pos++)
		printk(KERN_INFO"0x%x ", mdni_tuning5[data_pos]);
	printk(KERN_INFO "\n");
	for (data_pos = 0; data_pos < TUNE_SIXTH_SIZE ; data_pos++)
		printk(KERN_INFO"0x%x ", mdni_tuning6[data_pos]);
	printk(KERN_INFO "\n");


	mutex_lock(&msd.lock);
	mdss_dsi_cmds_send(ctrl_pdata, mdni_tune_cmd, ARRAY_SIZE(mdni_tune_cmd),0);
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

static ssize_t tuning_show(struct  device *dev,
				struct device_attribute *attr,char *buf)
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

	pr_info( "%s: LCD_ID = 0x%X, lcd_attached =%d", __func__,lcd_id, lcd_attached);

	return 0;
}

__setup( "lcd_id=0x", get_lcd_id_cmdline );
int mdss_dsi_panel_init(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata,
	bool cmd_cfg_cont_splash)
{
	int rc = 0;
	static const char *panel_name;
	bool cont_splash_enabled;
	bool partial_update_enabled;
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
#if SINGLE_WIRE_BL_CTRL
	spin_lock_init(&bl_lock);
#endif

	printk("%s: LCD attached status: %d !\n",
				__func__, get_samsung_lcd_attached());

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
	rc = mdss_panel_parse_dt_gpio(node, ctrl_pdata);
	if (rc) {
		pr_err("%s:%d panel dt gpio parse failed\n", __func__, __LINE__);
		return rc;
	}

	/*if (cmd_cfg_cont_splash)*/
		cont_splash_enabled = of_property_read_bool(node,
				"qcom,cont-splash-enabled");
	/*else
		cont_splash_enabled = false;*/
	if (!cont_splash_enabled) {
		pr_info("%s:%d Continuous splash flag not found.\n",
				__func__, __LINE__);
		ctrl_pdata->panel_data.panel_info.cont_splash_enabled = 0;
	} else {
		pr_info("%s:%d Continuous splash flag enabled.\n",
				__func__, __LINE__);
		ctrl_pdata->panel_data.panel_info.cont_splash_enabled = 1;
	}
	partial_update_enabled = of_property_read_bool(node,
						"qcom,partial-update-enabled");
	if (partial_update_enabled) {
		pr_info("%s:%d Partial update enabled.\n", __func__, __LINE__);
		ctrl_pdata->panel_data.panel_info.partial_update_enabled = 1;
		ctrl_pdata->partial_update_fnc = mdss_dsi_panel_partial_update;
	} else {
		pr_info("%s:%d Partial update disabled.\n", __func__, __LINE__);
		ctrl_pdata->panel_data.panel_info.partial_update_enabled = 0;
		ctrl_pdata->partial_update_fnc = NULL;
	}

	ctrl_pdata->on = mdss_dsi_panel_on;
	ctrl_pdata->off = mdss_dsi_panel_off;
	ctrl_pdata->panel_data.set_backlight = mdss_dsi_panel_bl_ctrl;
	ctrl_pdata->panel_reset = mdss_dsi_cpt_panel_reset;

#if defined(CONFIG_LCD_CLASS_DEVICE)
	lcd_device = lcd_device_register("panel", &pdev->dev, NULL,
					&mdss_dsi_disp_props);

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
#endif
#endif
#if defined(DDI_VIDEO_ENHANCE_TUNING)
	rc = sysfs_create_file(&lcd_device->dev.kobj,
			&dev_attr_tuning.attr);
	if(rc) {
		pr_info("sysfs create fail-%s\n",
			dev_attr_tuning.attr.name);
	}
#endif

#if defined(CONFIG_MDNIE_TFT_MSM8X26)
		pr_info("[%s] CONFIG_MDNIE_TFT feature ok ! initclass called!\n",__func__);
		init_mdnie_class();
		mdnie_tft_init(&msd);
#endif
#ifdef CONFIG_BACKLIGHT_LP8556
	samsung_bl_init();
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

	INIT_WORK(&err_fg_work, err_fg_work_func);

	disp_esd_gpio =of_get_named_gpio(node,"qcom,esd-det-gpio", 0);
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

	rc = request_threaded_irq(err_fg_gpio, NULL, err_fg_irq_handler, 
		IRQF_TRIGGER_HIGH | IRQF_ONESHOT, "esd_detect", NULL);
	if (rc) {
		pr_err("%s : Failed to request_irq. :ret=%d", __func__, rc);
	}

	disable_irq(err_fg_gpio);
#endif

	return 0;
}
