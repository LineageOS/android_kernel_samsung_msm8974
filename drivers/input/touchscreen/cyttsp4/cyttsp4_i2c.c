/*
 * cyttsp4_i2c.c
 * Cypress TrueTouch(TM) Standard Product V4 I2C Driver module.
 * For use with Cypress Txx4xx parts.
 * Supported parts include:
 * TMA4XX
 * TMA1036
 *
 * Copyright (C) 2012 Cypress Semiconductor
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * Author: Aleksej Makarov <aleksej.makarov@sonyericsson.com>
 * Modified by: Cypress Semiconductor for test with device
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contact Cypress Semiconductor at www.cypress.com <ttdrivers@cypress.com>
 *
 */

#include "cyttsp4_regs.h"
#include <linux/i2c.h>

#ifdef CONFIG_SAMSUNG_LPM_MODE
extern int poweroff_charging;
#endif
#define CY_I2C_DATA_SIZE  (2 * 256)

static int cyttsp4_i2c_read_block_data(struct device *dev, u16 addr,
	int length, void *values, int max_xfer)
{
	struct i2c_client *client = to_i2c_client(dev);
	int trans_len;
	u8 client_addr;
	u8 addr_lo;
	struct i2c_msg msgs[2];
	int rc = -EINVAL;

	while (length > 0) {
		client_addr = client->addr | ((addr >> 8) & 0x1);
		addr_lo = addr & 0xFF;
		trans_len = min(length, max_xfer);

		memset(msgs, 0, sizeof(msgs));
		msgs[0].addr = client_addr;
		msgs[0].flags = 0;
		msgs[0].len = 1;
		msgs[0].buf = &addr_lo;

		msgs[1].addr = client_addr;
		msgs[1].flags = I2C_M_RD;
		msgs[1].len = trans_len;
		msgs[1].buf = values;

		rc = i2c_transfer(client->adapter, msgs, 2);
		if (rc != 2)
			goto exit;

		length -= trans_len;
		values += trans_len;
		addr += trans_len;
	}

exit:
	return (rc < 0) ? rc : rc != ARRAY_SIZE(msgs) ? -EIO : 0;
}

static int cyttsp4_i2c_write_block_data(struct device *dev, u16 addr,
	u8 *wr_buf, int length, const void *values, int max_xfer)
{
	struct i2c_client *client = to_i2c_client(dev);
	u8 client_addr;
	u8 addr_lo;
	int trans_len;
	struct i2c_msg msg;
	int rc = -EINVAL;

	while (length > 0) {
		client_addr = client->addr | ((addr >> 8) & 0x1);
		addr_lo = addr & 0xFF;
		trans_len = min(length, max_xfer);

		memset(&msg, 0, sizeof(msg));
		msg.addr = client_addr;
		msg.flags = 0;
		msg.len = trans_len + 1;
		msg.buf = wr_buf;

		wr_buf[0] = addr_lo;
		memcpy(&wr_buf[1], values, trans_len);

		/* write data */
		rc = i2c_transfer(client->adapter, &msg, 1);
		if (rc != 1)
			goto exit;

		length -= trans_len;
		values += trans_len;
		addr += trans_len;
	}

exit:
	return (rc < 0) ? rc : rc != 1 ? -EIO : 0;
}

static int cyttsp4_i2c_write(struct device *dev, u16 addr, u8 *wr_buf,
	const void *buf, int size, int max_xfer)
{
	int rc;

	pm_runtime_get_noresume(dev);
	rc = cyttsp4_i2c_write_block_data(dev, addr, wr_buf, size, buf,
		max_xfer);
	pm_runtime_put_noidle(dev);

	return rc;
}

static int cyttsp4_i2c_read(struct device *dev, u16 addr, void *buf, int size,
	int max_xfer)
{
	int rc;

	pm_runtime_get_noresume(dev);
	rc = cyttsp4_i2c_read_block_data(dev, addr, size, buf, max_xfer);
	pm_runtime_put_noidle(dev);

	return rc;
}

static struct cyttsp4_bus_ops cyttsp4_i2c_bus_ops = {
	.write = cyttsp4_i2c_write,
	.read = cyttsp4_i2c_read,
};

static struct of_device_id cyttsp4_i2c_of_match[] = {
	{ .compatible = "cy,cyttsp4_i2c_adapter", }, { }
};
MODULE_DEVICE_TABLE(of, cyttsp4_i2c_of_match);

static int cyttsp4_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *i2c_id)
{
	struct device *dev = &client->dev;
	const struct of_device_id *match;
	int rc;

	pr_err("%s: \n", __func__);
		 
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(dev, "I2C functionality not Supported\n");
		return -EIO;
	}

	match = of_match_device(of_match_ptr(cyttsp4_i2c_of_match), dev);
	if (match)
		cyttsp4_devtree_create_and_get_pdata(dev);

	rc = cyttsp4_probe(&cyttsp4_i2c_bus_ops, &client->dev, client->irq,
			CY_I2C_DATA_SIZE);

	if (rc && match)
		cyttsp4_devtree_clean_pdata(dev);

	return rc;
}

static int cyttsp4_i2c_remove(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	const struct of_device_id *match;
	struct cyttsp4_core_data *cd = i2c_get_clientdata(client);

	pr_debug("%s: \n", __func__);
	
	cyttsp4_release(cd);

	match = of_match_device(of_match_ptr(cyttsp4_i2c_of_match), dev);
	if (match)
		cyttsp4_devtree_clean_pdata(dev);

	return 0;
}

static const struct i2c_device_id cyttsp4_i2c_id[] = {
	{ CYTTSP4_I2C_NAME, 0 },  { }
};
MODULE_DEVICE_TABLE(i2c, cyttsp4_i2c_id);

static struct i2c_driver cyttsp4_i2c_driver = {
	.driver = {
		.name = CYTTSP4_I2C_NAME,
		.owner = THIS_MODULE,
		.pm = &cyttsp4_pm_ops,
		.of_match_table = cyttsp4_i2c_of_match,
	},
	.probe = cyttsp4_i2c_probe,
	.remove = cyttsp4_i2c_remove,
	.id_table = cyttsp4_i2c_id,
};

static int __init cyttsp4_i2c_init(void)
{
	int rc = 0;
	#ifdef CONFIG_SAMSUNG_LPM_MODE
		if (poweroff_charging) {
			pr_notice("%s : LPM Charging Mode!!\n", __func__);
			return rc;
		}
	#endif
	rc = i2c_add_driver(&cyttsp4_i2c_driver);

	pr_err("%s: Cypress TTSP v4 I2C Driver (Built %s) rc=%d\n",
		 __func__, CY_DRIVER_DATE, rc);
	return rc;
}
module_init(cyttsp4_i2c_init);

static void __exit cyttsp4_i2c_exit(void)
{
	i2c_del_driver(&cyttsp4_i2c_driver);
}
module_exit(cyttsp4_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cypress TrueTouch(R) Standard Product I2C driver");
MODULE_AUTHOR("Cypress Semiconductor <ttdrivers@cypress.com>");
