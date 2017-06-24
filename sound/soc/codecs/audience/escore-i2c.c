/*
 * escore-i2c.c  --  I2C interface for Audience earSmart chips
 *
 * Copyright 2011 Audience, Inc.
 *
 * Author: Greg Clemson <gclemson@audience.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG
#include "escore.h"
#include "escore-i2c.h"
#include "es-d202.h"

int escore_i2c_read(struct escore_priv *escore, void *buf, int len)
{
	struct i2c_msg msg[] = {
		{
			.addr = escore->i2c_client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf,
		},
	};
	int rc = 0;

	rc = i2c_transfer(escore->i2c_client->adapter, msg, 1);
	/*
	 * i2c_transfer returns number of messages executed. Since we
	 * are always sending only 1 msg, return value should be 1 for
	 * success case
	 */
	if (rc != 1) {
		pr_err("%s(): i2c_transfer() failed, rc = %d, msg_len = %d\n",
			__func__, rc, len);
		return -EIO;
	} else {
		return 0;
	}
}

int escore_i2c_write(struct escore_priv *escore, const void *buf, int len)
{
	struct i2c_msg msg;
	int max_xfer_len = ES_MAX_I2C_XFER_LEN;
	int rc = 0, written = 0, xfer_len;

	msg.addr = escore->i2c_client->addr;
	msg.flags = 0;

	while (written < len) {
		xfer_len = min(len - written, max_xfer_len);

		msg.len = xfer_len;
		msg.buf = (void *)(buf + written);
		rc = i2c_transfer(escore->i2c_client->adapter, &msg, 1);
		if (rc != 1) {
			pr_err("%s(): i2c_transfer() failed, rc:%d\n",
					__func__, rc);
			return -EIO;
		}
		written += xfer_len;
	}
	return 0;
}

int escore_i2c_cmd(struct escore_priv *escore, u32 cmd, int sr, u32 *resp)
{
	int err;
	u32 rv;

	pr_info("escore: cmd=0x%08x  sr=%i\n", cmd, sr);

	cmd = cpu_to_be32(cmd);
	err = escore_i2c_write(escore, &cmd, sizeof(cmd));
	if (err || sr)
		return err;

	/* The response must be actively read. Maximum response time
	 * is 20ms.
	 */
	msleep(20);
	err = escore_i2c_read(escore, &rv, sizeof(rv));
	if (!err)
		*resp = be32_to_cpu(rv);
	pr_info("core: %s resp=0x%08x\n", __func__, *resp);
	return err;
}

int escore_i2c_boot_setup(struct escore_priv *escore)
{
	u16 boot_cmd = ES_I2C_BOOT_CMD;
	u16 boot_ack = 0;
	char msg[2];
	int rc;

	/* Reset Board */
	gpio_set_value(escore->pdata->reset_gpio, 0);
	msleep(20);
	gpio_set_value(escore->pdata->reset_gpio, 1);
	msleep(20);

	pr_info("%s()\n", __func__);
	pr_info("%s(): write ES_BOOT_CMD = 0x%04x\n", __func__, boot_cmd);
	cpu_to_be16s(&boot_cmd);
	memcpy(msg, (char *)&boot_cmd, 2);
	rc = escore->dev_write(escore, msg, 2);
	if (rc < 0) {
		pr_err("%s(): firmware load failed boot write\n", __func__);
		goto escore_bootup_failed;
	}
	usleep_range(1000, 1000);
	memset(msg, 0, 2);
	rc = escore->dev_read(escore, msg, 2);
	if (rc < 0) {
		pr_err("%s(): firmware load failed boot ack\n", __func__);
		goto escore_bootup_failed;
	}
	memcpy((char *)&boot_ack, msg, 2);
	pr_info("%s(): boot_ack = 0x%04x\n", __func__, boot_ack);
	if (boot_ack != ES_I2C_BOOT_ACK) {
		pr_err("%s(): firmware load failed boot ack pattern", __func__);
		rc = -EIO;
		goto escore_bootup_failed;
	}

escore_bootup_failed:
	return rc;
}

static int escore_i2c_probe(struct i2c_client *i2c,
			   const struct i2c_device_id *id)
{
	struct esxxx_platform_data *pdata = i2c->dev.platform_data;
	int rc;

	dev_dbg(&i2c->dev, "%s() i2c->name = %s\n", __func__, i2c->name);

	escore_priv.i2c_client = i2c;

	if (pdata == NULL) {
		dev_err(&i2c->dev, "%s(): pdata is NULL", __func__);
		rc = -EIO;
		goto pdata_error;
	}

	i2c_set_clientdata(i2c, &escore_priv);

	escore_priv.intf = ES_I2C_INTF;
	escore_priv.dev_read = escore_i2c_read;
	escore_priv.dev_write = escore_i2c_write;
	escore_priv.boot_setup = escore_i2c_boot_setup;
	escore_priv.cmd = escore_i2c_cmd;
	escore_priv.dev = &i2c->dev;
	escore_priv.streamdev = es_i2c_streamdev;

	rc = escore_priv.probe(&i2c->dev);
	if (rc) {
		dev_err(&i2c->dev, "%s(): escore_core_probe() failed %d\n",
			__func__, rc);
		goto escore_core_probe_error;
	}

	rc = escore_priv.bootup(&escore_priv);
	if (rc) {
		dev_err(&i2c->dev, "%s(): escore_bootup failed %d\n",
				__func__, rc);
		goto bootup_error;
	}
	release_firmware(escore_priv.fw);

	rc = snd_soc_register_codec(&i2c->dev,
			escore_priv.soc_codec_dev_escore,
			escore_priv.escore_dai,
			escore_priv.escore_dai_nr);

	dev_dbg(&i2c->dev, "%s(): rc = snd_soc_regsiter_codec() = %d\n",
			__func__, rc);
	es_d202_fill_cmdcache(escore_priv.codec);


	return rc;

bootup_error:
escore_core_probe_error:
pdata_error:
	dev_dbg(&i2c->dev, "%s(): exit with error\n", __func__);
	return rc;
}

static int escore_i2c_remove(struct i2c_client *i2c)
{
	struct esxxx_platform_data *pdata = i2c->dev.platform_data;

	gpio_free(pdata->reset_gpio);
	gpio_free(pdata->wakeup_gpio);
	gpio_free(pdata->gpioa_gpio);

	snd_soc_unregister_codec(&i2c->dev);

	kfree(i2c_get_clientdata(i2c));

	return 0;
}

static int escore_i2c_suspend(struct device *dev)
{
	return 0;
}

static int escore_i2c_resume(struct device *dev)
{
	return 0;
}

static int escore_i2c_runtime_suspend(struct device *dev)
{
	return 0;
}

static int escore_i2c_runtime_resume(struct device *dev)
{
	return 0;
}

static int escore_i2c_runtime_idle(struct device *dev)
{
	return 0;
}

struct es_stream_device es_i2c_streamdev = {
	.read = escore_i2c_read,
	.intf = ES_I2C_INTF,
};

int escore_i2c_init(void)
{
	int rc;

	rc = i2c_add_driver(&escore_i2c_driver);
	if (!rc)
		pr_info("%s() registered as I2C", __func__);

	else
		pr_err("%s(): i2c_add_driver failed, rc = %d", __func__, rc);

	return rc;
}

static const struct dev_pm_ops escore_i2c_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(
		escore_i2c_suspend,
		escore_i2c_resume
	)
	SET_RUNTIME_PM_OPS(
		escore_i2c_runtime_suspend,
		escore_i2c_runtime_resume,
		escore_i2c_runtime_idle
	)
};

static const struct i2c_device_id escore_i2c_id[] = {
	{ "earSmart", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, escore_i2c_id);

struct i2c_driver escore_i2c_driver = {
	.driver = {
		.name = "earSmart-codec",
		.owner = THIS_MODULE,
		.pm = &escore_i2c_dev_pm_ops,
	},
	.probe = escore_i2c_probe,
	.remove = escore_i2c_remove,
	.id_table = escore_i2c_id,
};

MODULE_DESCRIPTION("Audience earSmart I2C core driver");
MODULE_AUTHOR("Greg Clemson <gclemson@audience.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:earSmart-codec");
