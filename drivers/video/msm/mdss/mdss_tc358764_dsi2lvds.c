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

#ifdef CONFIG_BACKLIGHT_LP8556
#include "backlight_LP8556.h"
#endif

#define DT_CMD_HDR 6
#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
struct work_struct  err_fg_work;
static int err_fg_gpio;
static int esd_count;
static int err_fg_working;
#define ESD_DEBUG 1
#endif

static int lcd_attached;
static int lcd_id;
int get_lcd_attached(void);
extern int system_rev;
void __iomem *virt_mmss_gp0_base;
#define MMSS_GP0_BASE 0xFD8C3420
#define MMSS_GP0_SIZE  0x28

#if defined(CONFIG_TC358764_I2C_CONTROL)
struct i2c_client *lvds_i2c_client;
struct tc35876x_i2c_platform_data {
	unsigned	 int gpio_backlight_en;
	u32 en_gpio_flags;
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;
};

struct tc35876x_i2c_info {
	struct i2c_client			*client;
	struct tc35876x_i2c_platform_data	*pdata;
};

static struct tc35876x_i2c_info *i2c_info;
static int tc35876x_i2c_read_reg(u16 reg)
{
	u8 tx_data[2], rx_data[4];
	int ret, val;
	struct i2c_msg msg[2] = {
		/* first write slave position to i2c devices */
		{ i2c_info->client->addr, 0, ARRAY_SIZE(tx_data), tx_data },
		/* Second read data from position */
		{ i2c_info->client->addr, I2C_M_RD, ARRAY_SIZE(rx_data), rx_data}
	};

	tx_data[0] = (reg >> 8) & 0xff;
	tx_data[1] = reg & 0xff;

	ret = i2c_transfer(i2c_info->client->adapter, msg, 2);
	if (unlikely(ret < 0)) {
		pr_err(" [%s] i2c read failed  on reg 0x%04x error %d\n",
				__func__,  reg, ret);
		return ret;
	}
	if (unlikely(ret < ARRAY_SIZE(msg))) {
		pr_err("%s: reg 0x%04x msgs %d\n" ,
				__func__, reg, ret);
		return -EAGAIN;
	}
	val = (int)rx_data[0] << 24 | ((int)(rx_data[1]) << 16) |
		((int)(rx_data[2]) << 8) | ((int)(rx_data[3]));
	return val;
}
static int tc35876x_write_reg(u16 reg, u32 value)
{
	int ret;
	u8 tx_data[6];
	struct i2c_msg msg[] = {
		{i2c_info->client->addr, 0, 6, tx_data }
	};

	/* NOTE: Register address big-endian, data little-endian. */
	tx_data[0] = (reg >> 8) & 0xff;
	tx_data[1] = reg & 0xff;
	tx_data[2] = value & 0xff;
	tx_data[3] = (value >> 8) & 0xff;
	tx_data[4] = (value >> 16) & 0xff;
	tx_data[5] = (value >> 24) & 0xff;

	ret = i2c_transfer(i2c_info->client->adapter, msg, ARRAY_SIZE(msg));
	if (unlikely(ret < 0)) {
		pr_err("%s: i2c write failed reg 0x%04x val 0x%08x error %d\n",
				__func__, reg, value, ret);
		return ret;
	}
	if (unlikely(ret < ARRAY_SIZE(msg))) {
		pr_err("%s: reg 0x%04x val 0x%08x msgs %d\n",
				__func__, reg, value, ret);
		return -EAGAIN;
	}
	return ret;
}
void mdss_i2c_panel_cmds_send(void)
{
	int id;

	if (get_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}

	id=tc35876x_i2c_read_reg(0x0580);
	pr_info("%s: ID :- %X\n",__func__, id);
	/**************************************************					
	TC358764/65XBG DSI Basic Parameters.  Following 10 setting should be pefromed in LP mode						
	**************************************************/
	tc35876x_write_reg(0x013C,	0x00030005);
	tc35876x_write_reg(0x0114,	0x000000003);
	tc35876x_write_reg(0x0164,	0x000000003);
	tc35876x_write_reg(0x0168,	0x000000003);
	tc35876x_write_reg(0x016C,	0x000000003);
	tc35876x_write_reg(0x0170,	0x000000003);
	tc35876x_write_reg(0x0134,	0x00000001F);
	tc35876x_write_reg(0x0210,	0x00000001F);
	tc35876x_write_reg(0x0104,	0x00000001);
	tc35876x_write_reg(0x0204,	0x00000001);
	/**************************************************	
	TC358764/65XBG Timing and mode setting	
	**************************************************/
	tc35876x_write_reg(0x450,	0x03F00120);
	tc35876x_write_reg(0x454,	0x0032001C);
	tc35876x_write_reg(0x458,	0x00320500);
	tc35876x_write_reg(0x45C,	0x00040002);
	tc35876x_write_reg(0x460,	0x000A0320);
	tc35876x_write_reg(0x464,	0x00000001);
	tc35876x_write_reg(0x4A0,	0x00448006);
	//More than 100us	
	udelay(200);
	tc35876x_write_reg(0x4A0,	0x00048006);
	tc35876x_write_reg(0x504,	0x00000004);
	/**************************************************	
	TC358764/65XBG LVDS Color mapping setting	
	**************************************************/
	tc35876x_write_reg(0x480,	0x03020100);
	tc35876x_write_reg(0x484,	0x08050704);
	tc35876x_write_reg(0x488,	0x0F0E0A09);
	tc35876x_write_reg(0x48C,	0x100D0C0B);
	tc35876x_write_reg(0x490,	0x12111716);
	tc35876x_write_reg(0x494,	0x1B151413);
	tc35876x_write_reg(0x498,	0x061A1918);
	/**************************************************	
	TC358764/65XBG LVDS enable	
	**************************************************/
	tc35876x_write_reg(0x49C,	0x00000001);
}

#endif
DEFINE_LED_TRIGGER(bl_led_trigger);

static struct mdss_samsung_driver_data msd;


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

	if (get_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}
	pr_debug("%s: level=%d\n", __func__, level);

	led_pwm1[1] = mdss_dsi_panel_pwm_scaling(level);

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = &backlight_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);
}

void mdss_dsi_tc358764_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int rc=0;

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
		mdelay(1);
		if (gpio_is_valid(msd.lcd_en_gpio))
			gpio_set_value_cansleep(msd.lcd_en_gpio,(1<<1));//PMIC GPIO


		mdelay(1);
		if (gpio_is_valid(ctrl_pdata->rst_gpio)) {
			gpio_tlmm_config(GPIO_CFG(ctrl_pdata->rst_gpio, 0,
				GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			mdelay(1);
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			mdelay(1);
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			mdelay(1);
		}

		if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
			if (pinfo->mode_gpio_state == MODE_GPIO_HIGH)
				gpio_set_value((ctrl_pdata->mode_gpio), 1);
			else if (pinfo->mode_gpio_state == MODE_GPIO_LOW)
				gpio_set_value((ctrl_pdata->mode_gpio), 0);
		}

		if (gpio_is_valid(msd.bl_ldi_en)) {
			rc = gpio_tlmm_config(GPIO_CFG(msd.bl_ldi_en, 0,
				GPIO_CFG_INPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
				GPIO_CFG_DISABLE);
			if (rc)
				pr_err("tlmm config bl_ldi_en failed, rc=%d\n",rc);
		}

		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
						__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
		if (gpio_is_valid(ctrl_pdata->rst_gpio)) {
			gpio_tlmm_config(GPIO_CFG(ctrl_pdata->rst_gpio, 0,
				GPIO_CFG_OUTPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
				GPIO_CFG_DISABLE);
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
		}
		if (gpio_is_valid(msd.lcd_en_gpio))
			gpio_set_value_cansleep(msd.lcd_en_gpio,0);
#if defined(CONFIG_MACH_MATISSELTE_USC) || defined(CONFIG_MACH_MATISSELTE_OPEN)
		if (gpio_is_valid(msd.bl_ldi_en)) {
			gpio_tlmm_config(GPIO_CFG(msd.bl_ldi_en, 0,
				GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
				GPIO_CFG_DISABLE);
		}
#else
		if (gpio_is_valid(msd.bl_ldi_en)) {
			gpio_tlmm_config(GPIO_CFG(msd.bl_ldi_en, 0,
				GPIO_CFG_INPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
				GPIO_CFG_DISABLE);
		}
#endif
		if (gpio_is_valid(msd.bl_sda)) {
			rc = gpio_tlmm_config(GPIO_CFG(msd.bl_sda, 0,
				GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
				GPIO_CFG_DISABLE);
			if (rc)
				pr_err("tlmm config bl_sda failed, rc=%d\n",rc);
		}
		if (gpio_is_valid(msd.bl_scl)) {
			rc = gpio_tlmm_config(GPIO_CFG(msd.bl_scl, 0,
				GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
				GPIO_CFG_DISABLE);
			if (rc)
				pr_err("tlmm config bl_scl failed, rc=%d\n",rc);
		}
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			gpio_tlmm_config(GPIO_CFG(ctrl_pdata->disp_en_gpio,0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
		}
		mdelay(500);
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
extern void mdss_dsi_panel_bklt_pwm( int level);
static void mdss_dsi_panel_bl_ctrl(struct mdss_panel_data *pdata,
							u32 bl_level)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	if (get_lcd_attached() == 0)
	{
		printk("%s: LCD not connected!\n",__func__);
		return;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	
	pr_err("%s:@@@@@@@@@@@@@@@@@@@ bklt_ctrl:%d :bl_level=%d\n", __func__,ctrl_pdata->bklt_ctrl,bl_level);
	switch (ctrl_pdata->bklt_ctrl) {
	case BL_WLED:
		led_trigger_event(bl_led_trigger, bl_level);
		break;
	case BL_PWM:
		bl_level=mdss_dsi_panel_pwm_scaling(bl_level);
		mdss_dsi_panel_bklt_pwm(bl_level);
		break;
	case BL_DCS_CMD:
		mdss_dsi_panel_bklt_dcs(ctrl_pdata, bl_level);
		break;
	default:
		pr_err("%s: Unknown bl_ctrl configuration\n",
			__func__);
		break;
	}
}
extern void pwm_backlight_enable(void);
static int mdss_dsi_panel_on(struct mdss_panel_data *pdata)
{
	int rc =0;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	msd.mfd = (struct msm_fb_data_type *)registered_fb[0]->par;
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	msd.mpd = pdata;
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	pr_debug("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);	

#if !defined(CONFIG_TC358764_I2C_CONTROL)
	if (ctrl->on_cmds.cmd_cnt)
		mdss_dsi_panel_cmds_send(ctrl, &ctrl->on_cmds);
#else
	mdss_i2c_panel_cmds_send();
#endif
	msd.mfd->resume_state = MIPI_RESUME_STATE;
#if defined(CONFIG_MDNIE_TFT_MSM8X26)
	is_negative_on();
#endif
	msleep(300);
	if (gpio_is_valid(msd.bl_rst_gpio)) {
			gpio_tlmm_config(GPIO_CFG(msd.bl_rst_gpio, 0,
				GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
			gpio_direction_output(msd.bl_rst_gpio, 1);
		}
	if (gpio_is_valid(msd.bl_sda)) {
		rc = gpio_tlmm_config(GPIO_CFG(msd.bl_sda, 0,
			GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		if (rc)
			pr_err("tlmm config bl_sda failed, rc=%d\n",rc);
	}
	if (gpio_is_valid(msd.bl_scl)) {
		rc = gpio_tlmm_config(GPIO_CFG(msd.bl_scl, 0,
			GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		if (rc)
			pr_err("tlmm config bl_scl failed, rc=%d\n",rc);
	}
	mdelay(2);
	pwm_backlight_enable();
	mdelay(2);
	if (gpio_is_valid(msd.bl_ap_pwm)) {
		gpio_tlmm_config(GPIO_CFG(msd.bl_ap_pwm,3, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
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

	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

#if defined(CONFIG_ESD_ERR_FG_RECOVERY)
	if (!err_fg_working) {
		disable_irq_nosync(err_fg_gpio);
		cancel_work_sync(&err_fg_work);
	}
#endif
	pr_debug("%s: ctrl=%pK ndx=%d\n", __func__, ctrl, ctrl->ndx);
	if (gpio_is_valid(msd.bl_ap_pwm)) {
			gpio_tlmm_config(GPIO_CFG(msd.bl_ap_pwm,0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
			gpio_set_value(msd.bl_ap_pwm, 0);
	}
	msleep(10);
	if (gpio_is_valid(msd.bl_rst_gpio)) {
			gpio_tlmm_config(GPIO_CFG(msd.bl_rst_gpio,0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
			gpio_set_value(msd.bl_rst_gpio, 0);
	}
	msleep(190);
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

static int mdss_panel_parse_dt_gpio(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;

	msd.lcd_en_gpio = of_get_named_gpio(np,
						     "qcom,lcd-en-gpio", 0);
	if (!gpio_is_valid(msd.lcd_en_gpio)) {
		pr_err("%s:%d lcd_en_gpio gpio not specified\n",
						__func__, __LINE__);
	} else {
		rc = gpio_request(msd.lcd_en_gpio, "lcd_enable");
		if (rc) {
			pr_err("request lcd_en_gpio   failed, rc=%d\n",
			       rc);
			gpio_free(msd.lcd_en_gpio);
		}
	}

	msd.bl_rst_gpio= of_get_named_gpio(np,
						     "qcom,bl-rst-gpio", 0);
	if (!gpio_is_valid(msd.bl_rst_gpio)) {
		pr_err("%s:%d, bl_rst gpio not specified\n",
						__func__, __LINE__);
	} else {
		rc = gpio_request(msd.bl_rst_gpio, "bl_rst");
		if (rc) {
			pr_err("request bl_rst gpio failed, rc=%d\n",
				rc);
			gpio_free(msd.bl_rst_gpio);

		 }else{
			rc = gpio_tlmm_config(GPIO_CFG(msd.bl_rst_gpio, 0,
						GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
			if (rc) {
				pr_err("request bl_rst failed, rc=%d\n",rc);
			}
		}
	}
	msd.bl_ap_pwm= of_get_named_gpio(np,
						     "qcom,bl-wled", 0);
	if (!gpio_is_valid(msd.bl_ap_pwm)) {
		pr_err("%s:%d, bl_ap_pwm gpio not specified\n",
						__func__, __LINE__);
	} else {
		rc = gpio_request(msd.bl_ap_pwm, "bl_ap_pwm");
		if (rc) {
			pr_err("request bl_ap_pwm gpio failed, rc=%d\n",
				rc);
		gpio_free(msd.bl_ap_pwm);
	 }else{
			rc = gpio_tlmm_config(GPIO_CFG(msd.bl_ap_pwm, 3,
						GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
						GPIO_CFG_ENABLE);
			if (rc) {
				pr_err("request bl_ap_pwm gpio failed, rc=%d\n",rc);
			}
		}
	}

	msd.bl_ldi_en = of_get_named_gpio(np,
						     "qcom,lcd_ldi_int", 0);
	if (!gpio_is_valid(msd.bl_ldi_en)) {
		pr_err("%s:%d, bl_rst gpio not specified\n",
						__func__, __LINE__);
	} else {
		rc = gpio_request(msd.bl_ldi_en, "bl_ldi_en");
		if (rc) {
			pr_err("request bl_ldi_en gpio failed, rc=%d\n",
				rc);
			gpio_free(msd.bl_ldi_en);

		} else {
			if(system_rev>4)
			rc = gpio_tlmm_config(GPIO_CFG(msd.bl_ldi_en, 0,
					GPIO_CFG_INPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),
					GPIO_CFG_DISABLE);
			else
				rc = gpio_tlmm_config(GPIO_CFG(msd.bl_ldi_en, 0,
					GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
			if (rc)
				pr_err("tlmm config bl_ldi_en failed, rc=%d\n",rc);
		}
	}
	msd.bl_sda = of_get_named_gpio(np,
						     "qcom,bl-sda-gpio", 0);
	if (!gpio_is_valid(msd.bl_sda)) {
		pr_err("%s:%d, bl_sda gpio not specified\n",
						__func__, __LINE__);
	} else {
			rc=gpio_tlmm_config(GPIO_CFG(msd.bl_sda, 0,
					GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
			if (rc)
				pr_err("tlmm config bl_sda failed, rc=%d\n",rc);

	}
	msd.bl_scl = of_get_named_gpio(np,
						     "qcom,bl-scl-gpio", 0);
	if (!gpio_is_valid(msd.bl_scl)) {
		pr_err("%s:%d, bl_scl gpio not specified\n",
						__func__, __LINE__);
	}  else {
			rc=gpio_tlmm_config(GPIO_CFG(msd.bl_scl, 0,
					GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
			if (rc)
				pr_err("tlmm config bl_scl failed, rc=%d\n",rc);
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

static struct lcd_ops mdss_dsi_disp_props = {

	.get_power = NULL,
	.set_power = NULL,

};

static ssize_t mdss_disp_lcdtype_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char temp[20];

	snprintf(temp, 20, "SMD_LTL101AL06");
	strncat(buf, temp, 20);
	return strnlen(buf, 20);
}
static DEVICE_ATTR(lcd_type, S_IRUGO, mdss_disp_lcdtype_show, NULL);
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
	int bl_backup;
	struct mdss_panel_data *pdata = msd.mpd;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	int ret = 0;

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

	pr_info("%s : start", __func__);
	err_fg_working = 1;
	ctrl_pdata->panel_reset(pdata, 0);
	mdss_dsi_op_mode_config(DSI_CMD_MODE, pdata);
	if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
	ret = ctrl_pdata->off(pdata);
		if (ret) {
			pr_err("%s: Panel OFF failed\n", __func__);
			return;
		}
		ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
	}
	msleep(10);
	ctrl_pdata->panel_reset(pdata, 1);
	if (!(ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT)) {
		ret = ctrl_pdata->on(pdata);
		if (ret) {
			pr_err("%s: unable to initialize the panel\n",
							__func__);
			return;
		}
		ctrl_pdata->ctrl_state |= CTRL_STATE_PANEL_INIT;
	}

	esd_count++;
	err_fg_working = 0;

	mdss_dsi_panel_bl_ctrl(pdata, bl_backup);
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

	printk("%s: LCD attached status: %d !\n",
				__func__, get_samsung_lcd_attached());

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

	virt_mmss_gp0_base = ioremap(MMSS_GP0_BASE,MMSS_GP0_SIZE);
	if(virt_mmss_gp0_base == NULL) {
		pr_err("%s: I/O remap failed \n", __func__);
		return 0;
	}

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
	ctrl_pdata->panel_reset = mdss_dsi_tc358764_panel_reset;
	ctrl_pdata->panel_data.set_backlight = mdss_dsi_panel_bl_ctrl;

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

	rc = request_threaded_irq(err_fg_gpio, NULL, err_fg_irq_handler, 
		IRQF_TRIGGER_HIGH | IRQF_ONESHOT, "esd_detect", NULL);
	if (rc) {
		pr_err("%s : Failed to request_irq. :ret=%d", __func__, rc);
	}

	disable_irq(err_fg_gpio);
#endif

	return 0;
}
#if defined(CONFIG_TC358764_I2C_CONTROL)
static int tc35876x_i2c_parse_dt(struct device *dev,
			struct tc35876x_i2c_platform_data *pdata)
{
	struct device_node *np = dev->of_node;

	/* reset, irq gpio info */
	pdata->gpio_scl = of_get_named_gpio_flags(np, "lvds,scl-gpio",
				0, &pdata->scl_gpio_flags);
	pdata->gpio_sda = of_get_named_gpio_flags(np, "lvds,sda-gpio",
				0, &pdata->sda_gpio_flags);

	pr_info("%s gpio_scl : %d , gpio_sda : %d ", __func__, pdata->gpio_scl, pdata->gpio_sda);
	return 0;
}
static void tc35876x_i2c_request_gpio(struct tc35876x_i2c_platform_data *pdata)
{
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_scl, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_sda, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);

}

static int __devinit tc35876x_i2c_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct tc35876x_i2c_platform_data *pdata;
	struct tc35876x_i2c_info *info;

	int error = 0;

	pr_info("%s", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct tc35876x_i2c_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_info(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		error = tc35876x_i2c_parse_dt(&client->dev, pdata);
		if (error)
			return error;
	} else
		pdata = client->dev.platform_data;

	tc35876x_i2c_request_gpio(pdata);

	i2c_info = info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_info(&client->dev, "%s: fail to memory allocation.\n", __func__);
		return -ENOMEM;
	}

	info->client = client;
	info->pdata = pdata;

	i2c_set_clientdata(client, info);

	return error;
}

static int __devexit tc35876x_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id tc35876x_i2c_id[] = {
	{"lcd-i2c", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, tc35876x_i2c_id);

static struct of_device_id tc35876x_i2c_match_table[] = {
	{ .compatible = "lcd,lvds-i2c",},
	{ },
};

MODULE_DEVICE_TABLE(of, tc35876x_i2c_id);

struct i2c_driver tc35876x_i2c_driver = {
	.probe = tc35876x_i2c_probe,
	.remove = tc35876x_i2c_remove,
	.driver = {
		.name = "tc35876x_i2c",
		.owner = THIS_MODULE,
		.of_match_table = tc35876x_i2c_match_table,
		   },
	.id_table = tc35876x_i2c_id,
};

static int __init tc35876x_i2c_init(void)
{

	int ret = 0;

	ret = i2c_add_driver(&tc35876x_i2c_driver);
	if (ret) {
		printk(KERN_ERR "tc35876x_i2c_init registration failed. ret= %d\n",
			ret);
	}

	return ret;
}

static void __exit tc35876x_i2c_exit(void)
{
	i2c_del_driver(&tc35876x_i2c_driver);
}

module_init(tc35876x_i2c_init);
module_exit(tc35876x_i2c_exit);

MODULE_DESCRIPTION("tc35876x i2c driver");
MODULE_LICENSE("GPL");
#endif
