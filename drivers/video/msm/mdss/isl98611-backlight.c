/*
 * isl98611-backlight.c - Platform data for isl98611 backlight driver
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
#include <linux/of_gpio.h>

#include "mdss.h"
#include "mdss_panel.h"
#include "mdss_mdp.h"
#include "mdss_edp.h"

struct isl98611_backlight_platform_data {
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;
};

struct isl98611_backlight_info {
	struct i2c_client			*client;
	struct isl98611_backlight_platform_data	*pdata;
};

static struct isl98611_backlight_info *pinfo;

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

static void backlight_request_gpio(struct isl98611_backlight_platform_data *pdata)
{
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_scl, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_sda, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
}

static int isl98611_backlight_parse_dt(struct device *dev,
			struct isl98611_backlight_platform_data *pdata)
{
	struct device_node *np = dev->of_node;

	/* reset, irq gpio info */
	pdata->gpio_scl = of_get_named_gpio_flags(np, "isl98611,scl-gpio",
				0, &pdata->scl_gpio_flags);
	pdata->gpio_sda = of_get_named_gpio_flags(np, "isl98611,sda-gpio",
				0, &pdata->sda_gpio_flags);

	pr_info("%s gpio_scl : %d , gpio_sda : %d", __func__, pdata->gpio_scl, pdata->gpio_sda);
	return 0;
}

void isl98611_backlight_initialize(void)
{
	struct isl98611_backlight_info *info = pinfo;

	if (!info) {
		pr_info("%s error pinfo", __func__);
		return ;
	}

	backlight_i2c_write(info->client, 0x13, 0xC0, 1); // Hybrid dimming -> Analog dimming
	backlight_i2c_write(info->client, 0x12, 0x7F, 1); // LED  Forward current : 25mA -> 20mA

	pr_info("%s isl98611 init", __func__);
}

static int __devinit isl98611_backlight_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct isl98611_backlight_platform_data *pdata;
	struct isl98611_backlight_info *info;

	int error = 0;

	pr_info("%s", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct isl98611_backlight_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_info(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		error = isl98611_backlight_parse_dt(&client->dev, pdata);
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

static int __devexit isl98611_backlight_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id isl98611_backlight_id[] = {
	{"isl98611_backlight", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, isl98611_backlight_id);

static struct of_device_id isl98611_backlight_match_table[] = {
	{ .compatible = "isl98611,backlight-control",},
	{ },
};

MODULE_DEVICE_TABLE(of, isl98611_backlight_id);

struct i2c_driver isl98611_backlight_driver = {
	.probe = isl98611_backlight_probe,
	.remove = isl98611_backlight_remove,
	.driver = {
		.name = "isl98611_backlight",
		.owner = THIS_MODULE,
		.of_match_table = isl98611_backlight_match_table,
		   },
	.id_table = isl98611_backlight_id,
};

static int __init isl98611_backlight_init(void)
{

	int ret = 0;

	ret = i2c_add_driver(&isl98611_backlight_driver);
	if (ret) {
		printk(KERN_ERR "isl98611_backlight_init registration failed. ret= %d\n",
			ret);
	}

	return ret;
}

static void __exit isl98611_backlight_exit(void)
{
	i2c_del_driver(&isl98611_backlight_driver);
}

module_init(isl98611_backlight_init);
module_exit(isl98611_backlight_exit);

MODULE_DESCRIPTION("isl98611 backlight driver");
MODULE_LICENSE("GPL");
