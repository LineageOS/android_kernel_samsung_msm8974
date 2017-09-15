/* hscdtd008a_i2c.c
 *
 * GeoMagneticField device driver for I2C (HSCDTD008A)
 *
 * Copyright (C) 2012 ALPS ELECTRIC CO., LTD. All Rights Reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#undef CONFIG_HAS_EARLYSUSPEND

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/err.h>
#include <linux/of_gpio.h>
#include "sensors_core.h"
#define I2C_RETRY_DELAY  5
#define I2C_RETRIES      5

#define I2C_HSCD_ADDR    (0x0c)    /* 000 1100    */

#define HSCD_DRIVER_NAME	"HSCDTD008"
#define CHIP_DEV_NAME		"HSCDTD008A"
#define CHIP_DEV_VENDOR		"ALPS"

#undef ALPS_DEBUG
#define alps_dbgmsg(str, args...) pr_debug("%s: " str, __func__, ##args)
#define alps_errmsg(str, args...) pr_err("%s: " str, __func__, ##args)
#define alps_info(str, args...) pr_info("%s: " str, __func__, ##args)


#define HSCD_STB         0x0C
#define HSCD_XOUT        0x10
#define HSCD_YOUT        0x12
#define HSCD_ZOUT        0x14
#define HSCD_XOUT_H      0x11
#define HSCD_XOUT_L      0x10
#define HSCD_YOUT_H      0x13
#define HSCD_YOUT_L      0x12
#define HSCD_ZOUT_H      0x15
#define HSCD_ZOUT_L      0x14

#define HSCD_STATUS      0x18
#define HSCD_CTRL1       0x1b
#define HSCD_CTRL2       0x1c
#define HSCD_CTRL3       0x1d
#define HSCD_CTRL4       0x1e

#define HSCD_TCS_TIME    10000    /* Measure temp. of every 10 sec */
/* hscdtd008a chip id */
#define DEVICE_ID	0x49
/* hscd magnetic sensor chip identification register */
#define WHO_AM_I	0x0F
#define RETRY_COUNT	10

static struct i2c_driver hscd_driver;
static struct i2c_client *this_client;
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend hscd_early_suspend_handler;
#endif

struct hscd_power_data {
	struct regulator *regulator_vdd;
	struct regulator *regulator_vio;
};

struct hscd_platform_data{
	unsigned int irq_gpio;
	u32 irq_gpio_flags;
	const u8 *reg_vdd;
	const u8 *reg_vio;
};

static struct hscd_power_data hscd_power;

static atomic_t flgEna;
static atomic_t delay;
static atomic_t flgSuspend;
static int      tcs_thr;
static int      tcs_cnt;


static int hscd_i2c_readm(char *rxData, int length)
{
	int err;
	int tries = 0;

	struct i2c_msg msgs[] = {
		{
			.addr  = this_client->addr,
			.flags = 0,
			.len   = 1,
			.buf   = rxData,
		},
		{
			.addr  = this_client->addr,
			.flags = I2C_M_RD,
			.len   = length,
			.buf   = rxData,
		},
	};

	do {
		err = i2c_transfer(this_client->adapter, msgs, 2);
	} while ((err != 2) && (++tries < I2C_RETRIES));

	if (err != 2) {
		dev_err(&this_client->adapter->dev, "read transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}

static int hscd_i2c_writem(char *txData, int length)
{
	int err;
	int tries = 0;
#ifdef ALPS_DEBUG
	int i;
#endif

	struct i2c_msg msg[] = {
		{
			.addr  = this_client->addr,
			.flags = 0,
			.len   = length,
			.buf   = txData,
		},
	};

#ifdef ALPS_DEBUG
	pr_debug("%s : ", __func__);
	for (i = 0; i < length; i++)
		pr_debug("0X%02X, ", txData[i]);
	pr_debug("\n");
#endif

	do {
		err = i2c_transfer(this_client->adapter, msg, 1);
	} while ((err != 1) && (++tries < I2C_RETRIES));

	if (err != 1) {
		dev_err(&this_client->adapter->dev, "write transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}

static int hscd_power_on(int onoff)
{
	int err = 0;

	alps_info("is called\n");
	if (onoff == 1)
	{
		if (hscd_power.regulator_vdd) {
			err = regulator_enable(hscd_power.regulator_vdd);
			if (err) {
				alps_errmsg("Couldn't enable VDD %d\n", err);
				return err;
			}
		}

		if (hscd_power.regulator_vio) {
			err = regulator_enable(hscd_power.regulator_vio);
			if (err) {
				alps_errmsg("Couldn't enable VIO %d\n", err);
				return err;
			}
		}
	}
	else if (onoff == 0) {
			if (hscd_power.regulator_vdd) {
				regulator_disable(hscd_power.regulator_vdd);
				/*regulator_put(hscd_power.regulator_vdd);*/
			}
			if (hscd_power.regulator_vio) {
				regulator_disable(hscd_power.regulator_vio);
				/*regulator_put(hscd_power.regulator_vio);*/
			}
		}
	msleep(60);
	return err;
}

int hscd_self_test_A(void)
{
	u8 sx[2], cr1[1];

	if ( (this_client == NULL) || (atomic_read(&flgSuspend) == 1) )
	{
		alps_errmsg("ALPS_ERR hscd_self_test_A validation fail\n");
		return -1;
	}

	alps_info("is called\n");

	/* Control register1 backup  */
	cr1[0] = HSCD_CTRL1;
	if (hscd_i2c_readm(cr1, 1))
		return 1;
#ifdef ALPS_DEBUG
	else
		alps_dbgmsg("Control resister1 value, %02X\n", cr1[0]);
#endif
	mdelay(1);

	/* Move active Mode (force state) */
	sx[0] = HSCD_CTRL1;
	sx[1] = 0x8A;
	if (hscd_i2c_writem(sx, 2))
		return 1;

	/* Get inital value of self-test-A register */
	sx[0] = HSCD_STB;
	hscd_i2c_readm(sx, 1);

	mdelay(1);

	sx[0] = HSCD_STB;
	if (hscd_i2c_readm(sx, 1))
		return 1;
#ifdef ALPS_DEBUG
	else
		alps_dbgmsg("Self test A register value, %02X\n", sx[0]);
#endif
	if (sx[0] != 0x55) {
		alps_errmsg("Err! self-test-A, initial value is %02X\n", sx[0]);
		return 2;
	}

	/* do self-test*/
	sx[0] = HSCD_CTRL3;
	sx[1] = 0x10;
	if (hscd_i2c_writem(sx, 2))
		return 1;
	mdelay(3);

	/* Get 1st value of self-test-A register */
	sx[0] = HSCD_STB;
	if (hscd_i2c_readm(sx, 1))
		return 1;
#ifdef ALPS_DEBUG
	else
		alps_dbgmsg("Self test register value, %02X\n", sx[0]);
#endif
	if (sx[0] != 0xAA) {
		alps_errmsg("Err! self-test, 1st value is %02X\n", sx[0]);
		return 3;
	}
	mdelay(3);

	/* Get 2nd value of self-test register */
	sx[0] = HSCD_STB;
	if (hscd_i2c_readm(sx, 1))
		return 1;
#ifdef ALPS_DEBUG
	else
		alps_dbgmsg("Self test register value, %02X\n", sx[0]);
#endif
	if (sx[0] != 0x55) {
		alps_errmsg("Err! self-test, 2nd value is %02X\n", sx[0]);
		return 4;
	}

	/* Resume */
	sx[0] = HSCD_CTRL1;
	sx[1] = cr1[0];
	if (hscd_i2c_writem(sx, 2))
		return 1;

	return 0;
}
EXPORT_SYMBOL(hscd_self_test_A);

int hscd_self_test_B(void)
{
	if ( (this_client == NULL) || atomic_read(&flgSuspend) == 1)
	{
		alps_errmsg("ALPS_ERR hscd_self_test_B validation fail\n");
		return -1;
	}

	alps_info("is called\n");

	return 0;
}
EXPORT_SYMBOL(hscd_self_test_B);
static int hscd_soft_reset(void)
{
	int rc;
	u8 buf[2];

	#ifdef ALPS_DEBUG
		alps_dbgmsg("Software Reset\n");
	#endif

	buf[0] = HSCD_CTRL3;
	buf[1] = 0x80;
	rc = hscd_i2c_writem(buf, 2);
	msleep(20);

	return rc;
}

static int hscd_tcs_setup(void)
{
	int rc;
	u8 buf[2];

	buf[0] = HSCD_CTRL3;
	buf[1] = 0x02;
	rc = hscd_i2c_writem(buf, 2);
	msleep(20);

	tcs_thr = HSCD_TCS_TIME / atomic_read(&delay);
	tcs_cnt = 0;

	return rc;
}

static int hscd_force_setup(void)
{
	u8 buf[2];
	buf[0] = HSCD_CTRL3;
	buf[1] = 0x40;

	return hscd_i2c_writem(buf, 2);
}

int hscd_get_magnetic_field_data(int *xyz)
{
	int err = -1;
	int i;
	u8 sx[6] = {0, };
	if ( (this_client == NULL) || ( atomic_read(&flgSuspend) == 1 ) )
	{
		alps_errmsg("ALPS_ERR hscd_get_magnetic_field_data validation fail\n");
		return err;
	}

	sx[0] = HSCD_XOUT;

	err = hscd_i2c_readm(sx, 6);
	if (err < 0) {
		alps_errmsg("Fail to read data from i2c\n");
		return err;
	}

	for (i = 0; i < 3; i++)
		xyz[i] = (int) ((short)((sx[2*i + 1] << 8) | (sx[2*i])));

#ifdef ALPS_DEBUG
	/*** DEBUG OUTPUT - REMOVE ***/
	alps_dbgmsg("x = %d, y = %d, z = %d\n", xyz[0], xyz[1], xyz[2]);
	/*** <end> DEBUG OUTPUT - REMOVE ***/
#endif

	if (++tcs_cnt > tcs_thr)
		hscd_tcs_setup();
	hscd_force_setup();

	return err;
}
EXPORT_SYMBOL(hscd_get_magnetic_field_data);

void hscd_activate(int flgatm, int flg, int dtime)
{
	u8 buf[2];
	int Ena = atomic_read(&flgEna);

	if (this_client == NULL)
		return;
	else if ((atomic_read(&delay) == dtime)
				&& (atomic_read(&flgEna) == flg)
				&& (flgatm == 1))
		return;

	alps_info("is called\n");

	if (flg != 0) {
		buf[0] = HSCD_CTRL4;	/* 15 bit signed value */
		buf[1] = 0x90;
		hscd_i2c_writem(buf, 2);
		mdelay(1);

		tcs_cnt = tcs_cnt * atomic_read(&delay) / dtime;
		tcs_thr = HSCD_TCS_TIME / dtime;
	}
	if ((!flg) && (Ena)) {
		hscd_soft_reset();
	} else if (!flg) {
		buf[0] = HSCD_CTRL1;
		buf[1] = 0x0A;
		hscd_i2c_writem(buf, 2);
	} else if ((flg) && (!Ena)) {
		buf[0] = HSCD_CTRL1;
		buf[1] = 0x8A;
		hscd_i2c_writem(buf, 2);
		mdelay(1);
		hscd_tcs_setup();
		hscd_force_setup();
	}

	if (flgatm) {
		atomic_set(&flgEna, flg);
		atomic_set(&delay, dtime);
	}
}
EXPORT_SYMBOL(hscd_activate);
/*
static void hscd_register_init(void)
{
	u8  buf[2];

	alps_info("is called\n");

	buf[0] = HSCD_CTRL3;
	buf[1] = 0x80;
	hscd_i2c_writem(buf, 2);

	mdelay(5);

	atomic_set(&delay, 100);
	hscd_activate(0, 1, atomic_read(&delay));
	hscd_get_magnetic_field_data(v);

	alps_dbgmsg("x = %d, y = %d, z = %d\n", v[0], v[1], v[2]);

	hscd_activate(0, 0, atomic_read(&delay));
}
*/

static ssize_t selftest_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int result1, result2;

	if (!atomic_read(&flgEna))
		hscd_power_on(1);

	result1 = hscd_self_test_A();
	result2 = hscd_self_test_B();

	/*if (!atomic_read(&flgEna))
		hscd_power_off();*/

	if (result1 == 0)
		result1 = 1;
	else
		result1 = 0;

	if (result2 == 0)
		result2 = 1;
	else
		result2 = 0;

	alps_info("result, A = %d, B = %d\n", result1, result2);

	return snprintf(buf, PAGE_SIZE, "%d, %d\n", result1, result2);
}

static ssize_t status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int result;

	if (!atomic_read(&flgEna))
		hscd_power_on(1);

	result = hscd_self_test_B();

	/*if (!atomic_read(&flgEna))
		hscd_power_off();*/

	if (result == 0)
		result = 1;
	else
		result = 0;

	return snprintf(buf, PAGE_SIZE, "%d,%d\n", result, 0);
}

static ssize_t adc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int data[3];

	if (!atomic_read(&flgEna))
		hscd_activate(0, 1, 100);

	msleep(20);

	hscd_get_magnetic_field_data(data);
	alps_dbgmsg("x = %d, y = %d, z = %d\n", data[0], data[1], data[2]);

	if (!atomic_read(&flgEna))
		hscd_activate(0, 0, 100);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
					data[0], data[1], data[2]);
}

static ssize_t name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_DEV_NAME);
}

static ssize_t vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_DEV_VENDOR);
}


static ssize_t mag_raw_data_read(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int xyz[3] = {0, };

	alps_dbgmsg("is called\n");

	if (!atomic_read(&flgEna)) {
		hscd_activate(0, 1, 100);
		msleep(20);
	}

	hscd_get_magnetic_field_data(xyz);

	if (!atomic_read(&flgEna))
		hscd_activate(0, 0, 100);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
					xyz[0], xyz[1], xyz[2]);
}

static DEVICE_ATTR(selftest, S_IRUGO | S_IWUSR | S_IWGRP,
	selftest_show, NULL);
static DEVICE_ATTR(status, S_IRUGO | S_IWUSR | S_IWGRP,
	status_show, NULL);
static DEVICE_ATTR(adc, S_IRUGO | S_IWUSR | S_IWGRP,
	adc_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO | S_IWUSR | S_IWGRP,
	mag_raw_data_read, NULL);
static DEVICE_ATTR(name, S_IRUGO | S_IWUSR | S_IWGRP, name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO | S_IWUSR | S_IWGRP, vendor_show, NULL);

static struct device_attribute *magnetic_attrs[] = {
	&dev_attr_selftest,
	&dev_attr_status,
	&dev_attr_adc,
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_raw_data,
	NULL,
};

#ifdef CONFIG_OF
/* device tree parsing */
static int hscd_parse_dt(struct device *dev,struct hscd_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	pdata->irq_gpio = of_get_named_gpio_flags(np, "HSCDTD008-i2c,irq_gpio",
	0, &pdata->irq_gpio_flags);
	pdata->reg_vdd = of_get_property(np,"HSCDTD008-i2c,reg_vdd", NULL);
	pdata->reg_vio = of_get_property(np,"HSCDTD008-i2c,reg_vio", NULL);
	return 0;
}
#else
static int hscd_parse_dt(struct device *dev,
struct hscd_platform_data *pdata)
{
	return -ENODEV;
}
#endif

static int hscd_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret = 0;
	struct device *magnetic_device = NULL;
#ifdef CONFIG_OF
	int err;
	struct hscd_platform_data *platform_data;
#endif

	alps_info("is called\n");

	this_client = client;

	pr_info("\n hscd_probe  start");

		platform_data = devm_kzalloc (&client->dev,
			sizeof(struct hscd_platform_data), GFP_KERNEL);
		if(!platform_data) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		err = hscd_parse_dt(&client->dev, platform_data);
		if(err)
			return err;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->adapter->dev, "client not i2c capable (hscd_probe)\n");
		ret = -ENOMEM;
		goto exit;
	}

	hscd_power.regulator_vdd = NULL;
	hscd_power.regulator_vio = NULL;
	hscd_power.regulator_vdd = regulator_get(&client->dev,platform_data->reg_vdd);
	if (IS_ERR(hscd_power.regulator_vdd)) {
		ret = PTR_ERR(hscd_power.regulator_vdd);
		hscd_power.regulator_vdd = NULL;
		alps_errmsg("Failed to get hscd_i2c_vdd %d\n", ret);
		goto err_setup_regulator;
	}

	hscd_power.regulator_vio = regulator_get(&client->dev,platform_data->reg_vio);
	if (IS_ERR(hscd_power.regulator_vio)) {
		ret = PTR_ERR(hscd_power.regulator_vio);
		hscd_power.regulator_vio = NULL;
		alps_errmsg("Failed to get hscd_i2c_vio %d\n", ret);
		goto err_setup_regulator;
	}
	/* turn on the power */
	hscd_power_on(1);

	hscd_soft_reset();

	/* read chip id */
	ret = i2c_smbus_read_byte_data(this_client, WHO_AM_I);
	alps_info("Device ID = 0x%x, Reading ID = 0x%x\n", DEVICE_ID, ret);

	if (ret == DEVICE_ID) /* Normal Operation */
		ret = 0;
	else {
		if (ret < 0)
			alps_errmsg("i2c for reading chip id failed\n");
		else {
			alps_errmsg("Device identification failed\n");
			ret = -ENODEV;
		}
		goto err_setup_regulator;
	}
	err=sensors_register(magnetic_device, NULL, magnetic_attrs, "magnetic_sensor");
	//sensors_register(magnetic_device, NULL, magnetic_attrs,
	//					"magnetic_sensor");
	if (err < 0)
		return err;

	atomic_set(&flgEna, 0);
	atomic_set(&delay, 100);
	atomic_set(&flgSuspend, 0);

	tcs_cnt = 0;
	tcs_thr = HSCD_TCS_TIME / atomic_read(&delay);

	alps_info("is Successful\n");
	pr_info("\n hscd probe success");
	return 0;

err_setup_regulator:
	if (hscd_power.regulator_vdd) {
		regulator_disable(hscd_power.regulator_vdd);
		regulator_put(hscd_power.regulator_vdd);
	}
	if (hscd_power.regulator_vio) {
		regulator_disable(hscd_power.regulator_vio);
		regulator_put(hscd_power.regulator_vio);
	}
exit:
	this_client = NULL;
	alps_errmsg("Failed (errno = %d)\n", ret);

	return ret;
}

static int __devexit hscd_remove(struct i2c_client *client)
{
	alps_info("is called\n");

	hscd_activate(0, 0, atomic_read(&delay));
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&hscd_early_suspend_handler);
#endif

	if (hscd_power.regulator_vdd) {
		regulator_disable(hscd_power.regulator_vdd);
		regulator_put(hscd_power.regulator_vdd);
	}
	if (hscd_power.regulator_vio) {
		regulator_disable(hscd_power.regulator_vio);
		regulator_put(hscd_power.regulator_vio);
	}

	this_client = NULL;

	return 0;
}

static int hscd_suspend(struct i2c_client *client, pm_message_t mesg)
{
	alps_info("is called\n");

	atomic_set(&flgSuspend, 1);
	if (atomic_read(&flgEna))
		hscd_activate(0, 0, atomic_read(&delay));
#if !(defined(CONFIG_MACH_CRATERQ_CHN_OPEN)||defined(CONFIG_MACH_MS01_CHN_CTC)||defined(CONFIG_MACH_BAFFIN2_CHN_CMCC) || defined(CONFIG_SEC_GNOTE_PROJECT) )
	hscd_power_on(0);
#endif
	return 0;
}

static int hscd_resume(struct i2c_client *client)
{
	alps_info("is called\n");

	atomic_set(&flgSuspend, 0);
	if (atomic_read(&flgEna)) {
		atomic_set(&flgEna, 0);
		hscd_activate(1, 1 , atomic_read(&delay));
	}
#if !(defined(CONFIG_MACH_CRATERQ_CHN_OPEN)||defined(CONFIG_MACH_MS01_CHN_CTC)||defined(CONFIG_MACH_BAFFIN2_CHN_CMCC) || defined(CONFIG_SEC_GNOTE_PROJECT) ) 
	hscd_power_on(1);
#endif
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void hscd_early_suspend(struct early_suspend *handler)
{

	alps_info("is called\n");

	hscd_suspend(this_client, PMSG_SUSPEND);
}

static void hscd_early_resume(struct early_suspend *handler)
{
	alps_info("is called\n");

	hscd_resume(this_client);
}
#endif

static const struct i2c_device_id ALPS_id[] = {
	{ HSCD_DRIVER_NAME, 0 },
	{ }
};

#ifdef CONFIG_OF
static struct of_device_id magnetic_match_table[] = {
	{.compatible = "magnetic,HSCDTD008",},
	{},
};
#else
#define magnetic_match_table NULL
#endif

static struct i2c_driver hscd_driver = {
	.probe    = hscd_probe,
	.remove   = hscd_remove,
	.id_table = ALPS_id,
	.driver   = {
		.name = HSCD_DRIVER_NAME,
		.of_match_table = magnetic_match_table,
	},
	.suspend  = hscd_suspend,
	.resume   = hscd_resume,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend hscd_early_suspend_handler = {
	.suspend = hscd_early_suspend,
	.resume  = hscd_early_resume,
};
#endif

static int __init hscd_init(void)
{
	alps_info("is called\n");

	return i2c_add_driver(&hscd_driver);
}

static void __exit hscd_exit(void)
{
	alps_info("is called\n");

	i2c_del_driver(&hscd_driver);
}

module_init(hscd_init);
module_exit(hscd_exit);

MODULE_DESCRIPTION("Alps HSCDTD008A Compass Device");
MODULE_AUTHOR("ALPS ELECTRIC CO., LTD.");
MODULE_LICENSE("GPL v2");
