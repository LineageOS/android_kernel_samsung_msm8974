/*
 * cypress_touchkey.c - Platform data for edp backlight driver
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
//#include <mach/cpufreq.h>
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
#include "mdss_edp.h"

struct edp_backlight_platform_data {
	unsigned	 int gpio_backlight_en;
	u32 en_gpio_flags;
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;
};

struct edp_backlight_info {
	struct i2c_client			*client;
	struct edp_backlight_platform_data	*pdata;
};

static struct edp_backlight_info *pinfo;

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

static void backlight_request_gpio(struct edp_backlight_platform_data *pdata)
{
	int ret;

	ret = gpio_request(pdata->gpio_backlight_en, "edp_backlight_en");
	if (ret) {
		printk(KERN_ERR "%s: unable to request backlight_en [%d]\n",
				__func__, pdata->gpio_sda);
		return;
	}
#if 0 //splash booting enable
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_backlight_en, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_scl, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_sda, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	gpio_set_value(pdata->gpio_backlight_en,0);
	pr_info("%s %d", __func__, gpio_get_value(pdata->gpio_backlight_en));
#else
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_scl, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_sda, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
#endif
}

static int edp_backlight_parse_dt(struct device *dev,
			struct edp_backlight_platform_data *pdata)
{
	struct device_node *np = dev->of_node;

	/* reset, irq gpio info */
	pdata->gpio_scl = of_get_named_gpio_flags(np, "edp,scl-gpio",
				0, &pdata->scl_gpio_flags);
	pdata->gpio_sda = of_get_named_gpio_flags(np, "edp,sda-gpio",
				0, &pdata->sda_gpio_flags);
	pdata->gpio_backlight_en = of_get_named_gpio_flags(np, "edp.en-gpio",
				0, &pdata->en_gpio_flags);

	pr_info("%s gpio_scl : %d , gpio_sda : %d en : %d", __func__, pdata->gpio_scl, pdata->gpio_sda, pdata->gpio_backlight_en);
	return 0;
}

static u8 ndra_setting[][2] ={
	{0x01, 0x80},
#if defined(CONFIG_MACH_VIENNA_LTE) || defined(CONFIG_MACH_LT03EUR)\
	|| defined(CONFIG_MACH_LT03SKT)	|| defined(CONFIG_MACH_LT03KTT)\
	|| defined(CONFIG_MACH_LT03LGT) || defined(CONFIG_MACH_V2LTEEUR)\
	|| defined(CONFIG_MACH_PICASSO)
	{0xA0, 0xFF},
	{0xA1, 0x5F},
#else
	{0xA0, 0xDD},
	{0xA1, 0x6D},
#endif
	{0x98, 0xA1},
	{0x9E, 0x21},
	{0xA2, 0x28},
	{0xA3, 0x5E},
	{0xA4, 0x72},
	{0xA5, 0x04},
	{0xA6, 0x40},
	{0xA7, 0xFB},
	{0xA8, 0x00},
	{0xA9, 0xA0},
	{0xAA, 0x0F},
	{0xAB, 0x00},
	{0xAC, 0x00},
	{0xAD, 0x00},
	{0xAE, 0x0E},
	{0xAF, 0x01},
};

extern void restore_set_tcon(void);
void edp_backlight_power_enable(void)
{
	int i;
	struct edp_backlight_info *info = pinfo;

	if (!info) {
		pr_info("%s error pinfo", __func__);
		return ;
	}

	gpio_set_value(info->pdata->gpio_backlight_en,1);

	for (i = 0; i < ARRAY_SIZE(ndra_setting) ;i++) {
		backlight_i2c_write(info->client, ndra_setting[i][0], ndra_setting[i][1], 1);
	}

	pr_info("%s LSI_NDRA ", __func__);
}

void edp_backlight_enable(void)
{
	int i;
	struct edp_backlight_info *info = pinfo;

	if (!info) {
		pr_info("%s error pinfo", __func__);
		return ;
	}

	gpio_set_value(info->pdata->gpio_backlight_en,1);

	restore_set_tcon();

	for (i = 0; i < ARRAY_SIZE(ndra_setting) ;i++) {
		backlight_i2c_write(info->client, ndra_setting[i][0], ndra_setting[i][1], 1);
	}

	pr_info("%s LSI_NDRA ", __func__);
}

void edp_backlight_disable(void)
{
	struct edp_backlight_info *info = pinfo;

	if (!info) {
		pr_info("%s error pinfo", __func__);
		return ;
	}

	gpio_set_value(info->pdata->gpio_backlight_en, 0);
}

int edp_backlight_status(void)
{
	struct edp_backlight_info *info = pinfo;

	if (!info) {
		pr_info("%s error pinfo", __func__);
		return -ENODEV;
	}

	return gpio_get_value(info->pdata->gpio_backlight_en);
}

static int __devinit edp_backlight_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct edp_backlight_platform_data *pdata;
	struct edp_backlight_info *info;

	int error = 0;

	pr_info("%s", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct edp_backlight_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_info(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		error = edp_backlight_parse_dt(&client->dev, pdata);
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

static int __devexit edp_backlight_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id edp_backlight_id[] = {
	{"edp_backlight", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, edp_backlight_id);

static struct of_device_id edp_backlight_match_table[] = {
	{ .compatible = "edp,backlight-control",},
	{ },
};

MODULE_DEVICE_TABLE(of, edp_backlight_id);

struct i2c_driver edp_backlight_driver = {
	.probe = edp_backlight_probe,
	.remove = edp_backlight_remove,
	.driver = {
		.name = "edp_backlight",
		.owner = THIS_MODULE,
		.of_match_table = edp_backlight_match_table,
		   },
	.id_table = edp_backlight_id,
};

static int __init edp_backlight_init(void)
{

	int ret = 0;

	ret = i2c_add_driver(&edp_backlight_driver);
	if (ret) {
		printk(KERN_ERR "edp_backlight_init registration failed. ret= %d\n",
			ret);
	}

	return ret;
}

static void __exit edp_backlight_exit(void)
{
	i2c_del_driver(&edp_backlight_driver);
}

module_init(edp_backlight_init);
module_exit(edp_backlight_exit);

MODULE_DESCRIPTION("edp backlight driver");
MODULE_LICENSE("GPL");
