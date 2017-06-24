/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
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
 */
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#include "msm_sensor.h"

#define DRV_NAME "camera-dcdc"

DEFINE_MSM_MUTEX(ncp6335b_mut);

struct ncp6335b_dev {
	struct i2c_client *client;
	struct mutex lock;      /* protect cached dir, dat_out */
} *ncp_dev;

struct ncp6335b_platform_data {
	void *context;
};

static int ncp6335b_write(struct i2c_client *client, uint16_t v_set)
{
	int ret = 0;

	if (client == NULL) {
		pr_err("[syscamera][%s::%d][ERR] client is NULL \n", __func__, __LINE__);
		return 0;
	}

	ret = i2c_smbus_write_byte_data(client, 0x14, 0x00);
	if (ret < 0)
		pr_err("[syscamera][%s::%d]Write Error [%d]\n", __func__, __LINE__, ret);

	ret = i2c_smbus_write_byte_data(client, 0x10, v_set); /*C8 : 1.05v, C0 : 1.0v*/
	if (ret < 0)
		pr_err("[syscamera][%s::%d]Write Error [%d]\n", __func__, __LINE__, ret);

	ret = i2c_smbus_write_byte_data(client, 0x11, v_set); /*C8 : 1.05v, C0 : 1.0v*/
	if (ret < 0)
		pr_err("[syscamera][%s::%d]Write Error [%d]\n", __func__, __LINE__, ret);

	return ret;
}

static int ncp6335b_read(struct i2c_client *client)
{
	int ret = 0;

	if (client == NULL) {
		pr_err("[syscamera][%s::%d][ERR] client is NULL \n", __func__, __LINE__);
		return 0;
	}

	ret = i2c_smbus_read_byte_data(client, 0x3);
	if (ret < 0)
		pr_err("[syscamera][%s::%d]Read Error [%d]\n", __func__, __LINE__, ret);
	pr_err("[syscamera][%s::%d]NCP6335B PID[%x]\n", __func__, __LINE__, ret);

	ret = i2c_smbus_read_byte_data(client, 0x10);
	if (ret < 0)
		pr_err("[syscamera][%s::%d]Read Error [%d]\n", __func__, __LINE__, ret);
	pr_err("[syscamera][%s::%d]NCP6335B [0x10 Read :: %x]\n", __func__, __LINE__, ret);

	ret = i2c_smbus_read_byte_data(client, 0x11);
	if (ret < 0)
		pr_err("[syscamera][%s::%d]Read Error [%d]\n", __func__, __LINE__, ret);
	pr_err("[syscamera][%s::%d]NCP6335B [0x11 Read :: %x]\n", __func__, __LINE__, ret);

	ret = i2c_smbus_read_byte_data(client, 0x14);
	if (ret < 0)
		pr_err("[syscamera][%s::%d]Read Error [%d]\n", __func__, __LINE__, ret);
	pr_err("[syscamera][%s::%d]NCP6335B [0x14 Read :: %x]\n", __func__, __LINE__, ret);

	return ret;
}

void ncp6335b_set_voltage(int version)
{
	pr_err("[syscamera][%s::%d][BIN_INFO::%d]", __FUNCTION__, __LINE__, version);

	if ( (ncp_dev->client == NULL) || (ncp_dev == NULL) ) {
		pr_err("[syscamera][%s::%d][ERR] client is NULL \n", __func__, __LINE__);
		return;
	}

	switch (version) {
	case 0: /*BIN1*/
		ncp6335b_write(ncp_dev->client, 0xAC);
		break;
	case 1: /*BIN2*/
		ncp6335b_write(ncp_dev->client, 0xB0);
		break;
	case 2: /*BIN3*/
		ncp6335b_write(ncp_dev->client, 0xB4);
		break;
	case 3: /*BIN4*/
		ncp6335b_write(ncp_dev->client, 0xB8);
		break;
	case 4: /*BIN5*/
		ncp6335b_write(ncp_dev->client, 0xBC);
		break;
	case 5: /*BIN6*/
	default:
		ncp6335b_write(ncp_dev->client, 0xC0);
		break;
	}
	return;
};
EXPORT_SYMBOL(ncp6335b_set_voltage);

static int ncp6335b_parse_dt(struct device *dev,
			     struct ncp6335b_platform_data *pdata)
{
	dev->platform_data = pdata;
	pr_err("[syscamera][%s::%d]\n", __func__, __LINE__);
	return 0;
}

static int __devinit camera_dcdc_probe(struct i2c_client *client,
				       const struct i2c_device_id *id)
{
	struct ncp6335b_platform_data *pdata;
	int ret;

	pr_err("[syscamera][%s::%d]\n", __func__, __LINE__);

	pdata = devm_kzalloc(&client->dev,
			     sizeof(struct ncp6335b_platform_data),
			     GFP_KERNEL);
	if (!pdata) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	ret = ncp6335b_parse_dt(&client->dev, pdata);
	if (ret) {
		pr_err("[%s] mc5587 parse dt failed\n", __func__);
		return ret;
	}

	if (pdata == NULL) {
		pr_err("[syscamera][%s::%d]missing platform data\n", __func__, __LINE__);
		return -ENODEV;
	}

	ncp_dev = kzalloc(sizeof(*ncp_dev), GFP_KERNEL);
	if (ncp_dev == NULL) {
		pr_err("[syscamera][%s::%d]failed to alloc memory\n", __func__, __LINE__);
		return -ENOMEM;
	}

	ncp_dev->client = client;

	mutex_init(&ncp_dev->lock);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		pr_err("[syscamera][%s::%d]SMBUS Byte Data not Supported\n", __func__, __LINE__);
		return -EIO;
	}

	ret = ncp6335b_read(ncp_dev->client);
	pr_err("[syscamera][%s::%d][pid::%d]", __FUNCTION__, __LINE__, ret);

	ret = ncp6335b_write(ncp_dev->client, 0xC0); //default 1.0V

	i2c_set_clientdata(client, ncp_dev);

	return 0;
}

static int __devexit camera_dcdc_remove(struct i2c_client *client)
{
	kfree(ncp_dev);
	return 0;
}

static struct of_device_id ncp6335b_dt_ids[] = {
	{ .compatible = "ncp6335b,camera-dcdc", },
};


static const struct i2c_device_id ncp6335b_id[] = {
	{ DRV_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, ncp6335b_id);

struct i2c_driver ncp6335b_driver = {
	.probe			= camera_dcdc_probe,
	.remove			= camera_dcdc_remove,
	.driver			= {
		.name		= DRV_NAME,
		.of_match_table = of_match_ptr(ncp6335b_dt_ids),
	},
	.id_table		= ncp6335b_id,
};

static int __init ncp6335b_init_module(void)
{
	int32_t rc = 0;

	rc = i2c_add_driver(&ncp6335b_driver);
	pr_err("[syscamera][%s::%d][rc::%d]\n", __func__, __LINE__, rc);
	return rc;
}

static void __exit ncp6335b_exit_module(void)
{
	i2c_del_driver(&ncp6335b_driver);
	pr_err("[syscamera][%s::%d]\n", __func__, __LINE__);
	return;
}

module_init(ncp6335b_init_module);
module_exit(ncp6335b_exit_module);
MODULE_DESCRIPTION("ncp6335b");
MODULE_LICENSE("GPL v2");
