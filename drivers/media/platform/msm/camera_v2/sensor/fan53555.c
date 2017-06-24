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

#define DRV_NAME "fan53555-dcdc"

DEFINE_MSM_MUTEX(fan53555_mut);
typedef unsigned char BYTE;

#define REG_ADDR_VSEL0			0x00
#define REG_DATA_VSEL0_CLEAR		0x3f
#define VSEL0_BUCK_EN0_SHIFT		7
#define VSEL0_BUCK_EN0			(0x01 << VSEL0_BUCK_EN0_SHIFT)
#define VSEL0_INIT_VAL			VSEL0_BUCK_EN0 | FAN53555_VOUT_1P00
#define REG_ADDR_PID				0x03

// FAN53555UC08X : Vout = 0.60V + NSELx * 10mV
enum{
	FAN53555_VOUT_OFF = 0,
	FAN53555_VOUT_ON,
};

enum{
	FAN53555_VOUT_0P60 = 0,
	FAN53555_VOUT_0P61,
	FAN53555_VOUT_0P62,
	FAN53555_VOUT_0P63,
	FAN53555_VOUT_0P64,
	FAN53555_VOUT_0P65,
	FAN53555_VOUT_0P66,
	FAN53555_VOUT_0P67,
	FAN53555_VOUT_0P68,
	FAN53555_VOUT_0P69,

	FAN53555_VOUT_0P70 = 10,
	FAN53555_VOUT_0P71,
	FAN53555_VOUT_0P72,
	FAN53555_VOUT_0P73,
	FAN53555_VOUT_0P74,
	FAN53555_VOUT_0P75,
	FAN53555_VOUT_0P76,
	FAN53555_VOUT_0P77,
	FAN53555_VOUT_0P78,
	FAN53555_VOUT_0P79,

	FAN53555_VOUT_0P80 = 20,
	FAN53555_VOUT_0P81,
	FAN53555_VOUT_0P82,
	FAN53555_VOUT_0P83,
	FAN53555_VOUT_0P84,
	FAN53555_VOUT_0P85,
	FAN53555_VOUT_0P86,
	FAN53555_VOUT_0P87,
	FAN53555_VOUT_0P88,
	FAN53555_VOUT_0P89,

	FAN53555_VOUT_0P90 = 30,
	FAN53555_VOUT_0P91,
	FAN53555_VOUT_0P92,
	FAN53555_VOUT_0P93,
	FAN53555_VOUT_0P94,
	FAN53555_VOUT_0P95,
	FAN53555_VOUT_0P96,
	FAN53555_VOUT_0P97,
	FAN53555_VOUT_0P98,
	FAN53555_VOUT_0P99,

	FAN53555_VOUT_1P00 = 40,
	FAN53555_VOUT_1P01,
	FAN53555_VOUT_1P02,  // VSEL0 default, 08X
	FAN53555_VOUT_1P03,
	FAN53555_VOUT_1P04,
	FAN53555_VOUT_1P05,
	FAN53555_VOUT_1P06,
	FAN53555_VOUT_1P07,
	FAN53555_VOUT_1P08,
	FAN53555_VOUT_1P09,

	FAN53555_VOUT_1P10 = 50,
	FAN53555_VOUT_1P11,
	FAN53555_VOUT_1P12,
	FAN53555_VOUT_1P13,
	FAN53555_VOUT_1P14,
	FAN53555_VOUT_1P15,  // VSEL1 default, 08X
	FAN53555_VOUT_1P16,
	FAN53555_VOUT_1P17,
	FAN53555_VOUT_1P18,
	FAN53555_VOUT_1P19,

	FAN53555_VOUT_1P20 = 60,
	FAN53555_VOUT_1P21,
	FAN53555_VOUT_1P22,
	FAN53555_VOUT_1P23,
};

struct fan53555_dev {
	struct i2c_client *client;
	struct mutex lock;	/* protect cached dir, dat_out */
} *fan_dev;

struct fan53555_platform_data {
	void *context;
};


int fan53555_enable_vsel0(struct i2c_client *client, int on_off)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, REG_ADDR_VSEL0);
	if(ret < 0){
		pr_err("%s: read error = %d , try again", __func__, ret);
		ret = i2c_smbus_read_byte_data(client, REG_ADDR_VSEL0);
		if (ret < 0)
			pr_err("%s: read 2nd error = %d", __func__, ret);
	}

	ret &= (~VSEL0_BUCK_EN0);
	ret |= (on_off << VSEL0_BUCK_EN0_SHIFT);

	ret = i2c_smbus_write_byte_data(client, REG_ADDR_VSEL0, (BYTE)ret);
	if (ret < 0){
		pr_err("%s: write error = %d , try again", __func__, ret);
		ret = i2c_smbus_write_byte_data(client, REG_ADDR_VSEL0, (BYTE)ret);
		if (ret < 0)
			pr_err("%s: write 2nd error = %d", __func__, ret);
	}
	return ret;
}

int fan53555_set_vsel0_vout(struct i2c_client *client, int vout)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, REG_ADDR_VSEL0);
	if(ret < 0){
		pr_err("%s: read error = %d , try again", __func__, ret);
		ret = i2c_smbus_read_byte_data(client, REG_ADDR_VSEL0);
		if (ret < 0)
			pr_err("%s: read 2nd error = %d", __func__, ret);
	}

	ret &= (~REG_DATA_VSEL0_CLEAR);
	ret |= vout;

	ret = i2c_smbus_write_byte_data(client, REG_ADDR_VSEL0, (BYTE)ret);
	if (ret < 0){
		pr_err("%s: write error = %d , try again", __func__, ret);
		ret = i2c_smbus_write_byte_data(client, REG_ADDR_VSEL0, (BYTE)ret);
		if (ret < 0)
			pr_err("%s: write 2nd error = %d", __func__, ret);
	}
	return ret;
}

static int fan53555_read(struct i2c_client *client, BYTE read_addr)
{
	int ret = 0;

	if (client == NULL) {
		pr_err("[syscamera][%s::%d][ERR] client is NULL \n", __func__, __LINE__);
		return 0;
	}

	ret = i2c_smbus_read_byte_data(client, read_addr);
	if (ret < 0)
		pr_err("[syscamera][%s::%d]Read Error [%d]\n", __func__, __LINE__, ret);
	else
		pr_err("[syscamera][%s::%d]Register Read[%x::%x]\n", __func__, __LINE__, read_addr, ret);

	return ret;
}

void fan53555_set_voltage(int version)
{
	pr_err("[syscamera][%s::%d][BIN_INFO::%d] \n", __FUNCTION__, __LINE__, version);

	if ( fan_dev == NULL ) {
		pr_err("[syscamera][%s::%d][ERR] fan_dev is NULL \n", __func__, __LINE__);
		return;
	} else {
		if ( fan_dev->client == NULL) {
			pr_err("[syscamera][%s::%d][ERR] client is NULL \n", __func__, __LINE__);
			return;
		}
	}

	switch(version) {
	case 0:/*BIN1*/
		fan53555_set_vsel0_vout(fan_dev->client, FAN53555_VOUT_0P88);
		break;
	case 1:/*BIN2*/
		fan53555_set_vsel0_vout(fan_dev->client, FAN53555_VOUT_0P90);
		break;
	case 2:/*BIN3*/
		fan53555_set_vsel0_vout(fan_dev->client, FAN53555_VOUT_0P93);
		break;
	case 3:/*BIN4*/
		fan53555_set_vsel0_vout(fan_dev->client, FAN53555_VOUT_0P95);
		break;
	case 4:/*BIN5*/
		fan53555_set_vsel0_vout(fan_dev->client, FAN53555_VOUT_0P98);
		break;
	case 5:/*BIN6*/
	default:
		fan53555_set_vsel0_vout(fan_dev->client, FAN53555_VOUT_1P00);
		break;
	}
	return;
};
EXPORT_SYMBOL(fan53555_set_voltage);

static int fan53555_parse_dt(struct device *dev,
		struct fan53555_platform_data *pdata)
{
	dev->platform_data = pdata;
	pr_err("[syscamera][%s::%d]\n", __func__, __LINE__);
	return 0;
}

static int fan53555_dcdc_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct fan53555_platform_data *pdata;
	int ret;

	pr_err("[syscamera][%s::%d]\n", __func__, __LINE__);

	pdata = devm_kzalloc(&client->dev,
		sizeof(struct fan53555_platform_data),
		GFP_KERNEL);

	if (!pdata) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	ret = fan53555_parse_dt(&client->dev, pdata);
	if (ret) {
		pr_err("[syscamera][%s] fan53555 parse dt failed\n", __func__);
		return ret;
	}

	if (pdata == NULL) {
		pr_err("[syscamera][%s::%d]missing platform data\n", __func__, __LINE__);
		return -ENODEV;
	}

	fan_dev = kzalloc(sizeof(*fan_dev), GFP_KERNEL);
	if (fan_dev == NULL) {
		pr_err("[syscamera][%s::%d]failed to alloc memory\n", __func__, __LINE__);
		return -ENOMEM;
	}
	fan_dev->client = client;

	mutex_init(&fan_dev->lock);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		pr_err("[syscamera][%s::%d]SMBUS Byte Data not Supported\n", __func__, __LINE__);
		kfree(fan_dev);
		fan_dev = NULL;
		return -EIO;
	}

	ret = fan53555_read(fan_dev->client, REG_ADDR_PID);
	pr_err("[syscamera][%s::%d][PID::%d]",__FUNCTION__, __LINE__, ret);

	fan53555_set_vsel0_vout(fan_dev->client, VSEL0_INIT_VAL);//set vout default 1.0V
	fan53555_enable_vsel0(fan_dev->client, FAN53555_VOUT_ON);//vout on

	i2c_set_clientdata(client, fan_dev);

	return 0;
}

static int fan53555_dcdc_remove(struct i2c_client *client)
{
	kfree(fan_dev);
	fan_dev = NULL;
	return 0;
}

static struct of_device_id fan53555_dt_ids[] = {
	{ .compatible = "fan53555,fan53555-dcdc",},
};


static const struct i2c_device_id fan53555_id[] = {
	{DRV_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, fan53555_id);

struct i2c_driver fan53555_driver = {
	.probe = fan53555_dcdc_probe,
	.remove = fan53555_dcdc_remove,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = of_match_ptr(fan53555_dt_ids),
	},
	.id_table = fan53555_id,
};

static int __init fan53555_init_module(void)
{
	int32_t rc = 0;
	rc = i2c_add_driver(&fan53555_driver);
	pr_info("[syscamera][%s::%d][rc::%d]\n", __func__, __LINE__, rc);
	return rc;
}

static void __exit fan53555_exit_module(void)
{
	i2c_del_driver(&fan53555_driver);
	pr_info("[syscamera][%s::%d]\n", __func__, __LINE__);
	return;
}

module_init(fan53555_init_module);
module_exit(fan53555_exit_module);
MODULE_DESCRIPTION("fan53555");
MODULE_LICENSE("GPL v2");
