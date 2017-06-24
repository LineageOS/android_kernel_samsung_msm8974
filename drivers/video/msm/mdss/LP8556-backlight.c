/*
 * LP8556-backlight.c - Platform data for lp8556 backlight driver
 *
 * Copyright (C) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <asm/unaligned.h>
#include <mach/board.h>
/*#include <mach/cpufreq.h>*/
#include <linux/input/mt.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/earlysuspend.h>
#include <linux/regulator/consumer.h>
#include <asm/mach-types.h>
#include <linux/device.h>
/* #include <mach/msm8974-gpio.h> */
#include <linux/of_gpio.h>

#include "mdss.h"
#include "mdss_panel.h"
#include "mdss_mdp.h"

struct lp8556_backlight_platform_data {
	unsigned	 int gpio_backlight_en;
	u32 en_gpio_flags;

	int gpio_sda;
	u32 sda_gpio_flags;

	int gpio_scl;
	u32 scl_gpio_flags;
};

struct lp8556_backlight_info {
	struct i2c_client			*client;
	struct lp8556_backlight_platform_data	*pdata;
};

struct lp8556_backlight_info *pinfo;

#if 0
static int backlight_i2c_read(struct i2c_client *client,
		u8 reg, u8 *val, unsigned int len)
{

	int err = 0;
	int retry = 3;

	while (retry--) {
		err = i2c_smbus_read_i2c_block_data(client,
				reg, len, val);
		if (err >= 0)
			return err;

		dev_info(&client->dev, "%s: i2c transfer error.\n", __func__);
	}
	return err;

}
#endif

static int backlight_i2c_write(struct i2c_client *client,
		u8 reg,  u8 val, unsigned int len)
{
	int err = 0;
	int retry = 3;
	u8 temp_val = val;

	while (retry--) {
		err = i2c_smbus_write_i2c_block_data(client,
				reg, len, &temp_val);
		if (err >= 0)
			return err;

		dev_info(&client->dev, "%s: i2c transfer error. %d\n", __func__, err);
	}
	return err;
}
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WQXGA_PT_PANEL)
int get_lcd_attached(void);
#endif
static void backlight_request_gpio(struct lp8556_backlight_platform_data *pdata)
{
	int ret;
	if (gpio_is_valid(pdata->gpio_backlight_en)) {
		ret = gpio_request(pdata->gpio_backlight_en, "lp8556_backlight_en");
		if (ret) {
			printk(KERN_ERR "%s: unable to request backlight_en [%d]\n",
				__func__, pdata->gpio_sda);
			return;
		}
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_backlight_en, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	}
	/* gpio_tlmm_config(GPIO_CFG(pdata->gpio_scl, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1); */
	/* gpio_tlmm_config(GPIO_CFG(pdata->gpio_sda, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1); */

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_TFT_VIDEO_WQXGA_PT_PANEL)
	if (get_lcd_attached() == 0) {
		if (gpio_is_valid(pdata->gpio_backlight_en)) {
			pr_info("%s : Set Low Backlight Enable GPIO \n", __func__);
			gpio_set_value(pdata->gpio_backlight_en, 0);
		}
	}
#endif

}

static int lp8556_backlight_parse_dt(struct device *dev,
			struct lp8556_backlight_platform_data *pdata)
{
	struct device_node *np = dev->of_node;

	/* reset, irq gpio info */
	pdata->gpio_scl = of_get_named_gpio_flags(np, "lp8556,scl-gpio",
				0, &pdata->scl_gpio_flags);
	pdata->gpio_sda = of_get_named_gpio_flags(np, "lp8556,sda-gpio",
				0, &pdata->sda_gpio_flags);
	pdata->gpio_backlight_en = of_get_named_gpio_flags(np, "lp8556,en-gpio",
				0, &pdata->en_gpio_flags);

	pr_info("%s gpio_scl : %d , gpio_sda : %d en : %d", __func__, pdata->gpio_scl, pdata->gpio_sda, pdata->gpio_backlight_en);
	return 0;
}

extern int system_rev;
/*
	0xA5, 0x24 //4 channel
	0xA5, 0x14 //5 channel
*/
#if defined(CONFIG_SEC_T10_PROJECT)
static u8 channel_setting[][2] = {
	{0x00, 0x45},
	{0x01, 0x73},
	{0x02, 0xB7},
};
#elif defined(CONFIG_FB_MSM_MDSS_TC_DSI2LVDS_WXGA_PANEL)
static u8 channel_setting[][2] ={
	{0x01, 0x80},
#if defined(CONFIG_MACH_MATISSELTE_ATT)
	{0xA7, 0xCB},
#endif
	{0xA5, 0x24},
	{0xA0, 0x44},
	{0xA1, 0x6C},
};
#elif defined(CONFIG_FB_MSM_MDSS_SDC_WXGA_PANEL)
#if defined(CONFIG_SEC_MILLETWIFI_COMMON)
static u8 channel_setting_rev4[][2] ={
	{0x01, 0x80},
	{0xA7, 0xFA},
	{0xA3, 0x5E},
	{0xA5, 0x34},
};
static u8 channel_setting[][2] ={
	{0x02, 0x04},
	{0x01, 0x07},
};
#elif defined(CONFIG_SEC_T8_PROJECT)
static u8 channel_setting[][2] ={
	{0x01, 0x80},
	{0xA0, 0xFF},
	{0xA1, 0xDF},
	{0xA3, 0x5E},
	{0xA5, 0x34},
};
#elif defined(CONFIG_SEC_RUBENS_PROJECT)
#if defined(CONFIG_SEC_RUBENSLTE_COMMON)
static u8 channel_setting[][2] ={
	{0x01, 0x85},
	{0xA7, 0xC7},
	{0xA3, 0x5E},
	{0xA5, 0x34},
};
#endif /* CONFIG_SEC_RUBENSLTE_COMMON */
static u8 channel_setting_rev1[][2] ={
	{0x01, 0x80},
	{0xA7, 0xC7},
	{0xA3, 0x5E},
	{0xA5, 0x34},
};
#else
static u8 channel_setting[][2] ={
	{0x01, 0x80},
	{0xA7, 0xC7},
	{0xA3, 0x5E},
	{0xA5, 0x34},
};
#endif
#else
static u8 channel_setting[][2] ={
	{0x01, 0x80},
	{0xA5, 0x14},
};
#endif
void pwm_backlight_enable(void)
{
	int i;
	struct lp8556_backlight_info *info = pinfo;

	if (!info) {
		pr_info("%s error pinfo", __func__);
		return ;
	}
	if (gpio_is_valid(info->pdata->gpio_backlight_en)) {
		if (gpio_get_value(info->pdata->gpio_backlight_en))
			return ;

		gpio_set_value(info->pdata->gpio_backlight_en,1);

		msleep(10);
	}
#if defined(CONFIG_MACH_T10_3G_OPEN)
	gpio_set_value(info->pdata->gpio_scl,1);
	gpio_set_value(info->pdata->gpio_sda,1);
	for (i = 0; i < ARRAY_SIZE(channel_setting) ;i++)
		backlight_i2c_write(info->client, channel_setting[i][0], channel_setting[i][1], 1);
#elif defined(CONFIG_SEC_MILLETWIFI_COMMON)
	if(system_rev >= 8) {
		for (i = 0; i < ARRAY_SIZE(channel_setting_rev4) ;i++)
			backlight_i2c_write(info->client, channel_setting_rev4[i][0], channel_setting_rev4[i][1], 1);
	} else {
		for (i = 0; i < ARRAY_SIZE(channel_setting) ;i++)
			backlight_i2c_write(info->client, channel_setting[i][0], channel_setting[i][1], 1);
	}
#elif defined(CONFIG_SEC_RUBENS_PROJECT)
#if defined(CONFIG_SEC_RUBENSLTE_COMMON)
	if(system_rev) {
		for (i = 0; i < ARRAY_SIZE(channel_setting_rev1) ;i++)
			backlight_i2c_write(info->client, channel_setting_rev1[i][0], channel_setting_rev1[i][1], 1);
	} else {
		for (i = 0; i < ARRAY_SIZE(channel_setting) ;i++)
			backlight_i2c_write(info->client, channel_setting[i][0], channel_setting[i][1], 1);
	}
#else
	for (i = 0; i < ARRAY_SIZE(channel_setting_rev1) ;i++)
		backlight_i2c_write(info->client, channel_setting_rev1[i][0], channel_setting_rev1[i][1], 1);
#endif /* !CONFIG_SEC_RUBENSLTE_COMMON */
#else
	for (i = 0; i < ARRAY_SIZE(channel_setting) ;i++)
		backlight_i2c_write(info->client, channel_setting[i][0], channel_setting[i][1], 1);
#endif

}

#if defined(CONFIG_SEC_RUBENS_PROJECT)
static u8 channel_backlight_control[][2] ={
	{0x00, 0x00},
};
void pwm_backlight_control_i2c(int scaled_level)
{
	int i;
	struct lp8556_backlight_info *info = pinfo;
	channel_backlight_control[0][1] =  scaled_level;

	if (!info) {
		pr_info("%s error pinfo", __func__);
		return ;
	}

	for (i = 0; i < ARRAY_SIZE(channel_backlight_control) ;i++)
		backlight_i2c_write(info->client, channel_backlight_control[i][0], channel_backlight_control[i][1], 1);
}
#endif

void pwm_backlight_disable(void)
{
	struct lp8556_backlight_info *info = pinfo;

	if (!info) {
		pr_info("%s error pinfo", __func__);
		return ;
	}
	if (gpio_is_valid(info->pdata->gpio_backlight_en)) {
		gpio_set_value(info->pdata->gpio_backlight_en, 0);
		msleep(10);
	}
}

static int __devinit lp8556_backlight_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct lp8556_backlight_platform_data *pdata;
	struct lp8556_backlight_info *info;
	int error = 0;

	pr_info("%s", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct lp8556_backlight_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_info(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		error = lp8556_backlight_parse_dt(&client->dev, pdata);
		if (error)
			return error;
	} else
		pdata = client->dev.platform_data;

	backlight_request_gpio(pdata);

	pinfo = info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_info(&client->dev, "%s: fail to memory allocation.\n", __func__);
		return -ENOMEM;
	}

	info->client = client;
	info->pdata = pdata;

	i2c_set_clientdata(client, info);

	return error;
}

static int __devexit lp8556_backlight_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id lp8556_backlight_id[] = {
	{"lp8556_backlight", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, lp8556_backlight_id);

static struct of_device_id lp8556_backlight_match_table[] = {
	{ .compatible = "lp8556,backlight-control",},
	{ },
};

MODULE_DEVICE_TABLE(of, lp8556_backlight_id);

struct i2c_driver lp8556_backlight_driver = {
	.probe = lp8556_backlight_probe,
	.remove = lp8556_backlight_remove,
	.driver = {
		.name = "lp8556_backlight",
		.owner = THIS_MODULE,
		.of_match_table = lp8556_backlight_match_table,
		   },
	.id_table = lp8556_backlight_id,
};

static int __init lp8556_backlight_init(void)
{

	int ret = 0;

	pr_info("%s", __func__);

	ret = i2c_add_driver(&lp8556_backlight_driver);
	if (ret) {
		printk(KERN_ERR "lp8556_backlight_init registration failed. ret= %d\n",
			ret);
	}

	return ret;
}

static void __exit lp8556_backlight_exit(void)
{
	i2c_del_driver(&lp8556_backlight_driver);
}

module_init(lp8556_backlight_init);
module_exit(lp8556_backlight_exit);

MODULE_DESCRIPTION("lp8556 backlight driver");
MODULE_LICENSE("GPL");
