/*
 * CORERIVER TOUCHCORE touchkey fw update
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/i2c/touchkey_i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/wakelock.h>

#define TC360_FW_FLASH_RETRY	5
#define TC360_POWERON_DELAY	100
#define TC360_DCR_RD_RETRY	50
#define TC360_FW_VER_READ	5
#define TC360_FW_ER_MAX_LEN		0X8000
#define TC360_FW_WT_MAX_LEN		0X3000
#define TC360_ISP_ACK			1
#define TC360_ISP_SP_SIGNAL		0b010101011111000
#define TC360_NUM_OF_ISP_SP_SIGNAL	15
#define CORERIVER_FW_VER	0x1
#define DELAY_TIME	6
bool counting_timer;

static inline void setsda(struct cypress_touchkey_info *info, int state)
{
	if (state)
		gpio_direction_input(info->pdata->gpio_sda);
	else
		gpio_direction_output(info->pdata->gpio_sda, 0);
}

static inline void setscl(struct cypress_touchkey_info *info, int state)
{
	if (state)
		gpio_direction_input(info->pdata->gpio_scl);
	else
		gpio_direction_output(info->pdata->gpio_scl, 0);
}

static inline int getsda(struct cypress_touchkey_info *info)
{
	return gpio_get_value(info->pdata->gpio_sda);
}

static inline int getscl(struct cypress_touchkey_info *info)
{
	return gpio_get_value(info->pdata->gpio_scl);
}

static inline void sdalo(struct cypress_touchkey_info *info)
{
	setsda(info, 0);
	udelay((DELAY_TIME + 1) / 2);
}

static inline void sdahi(struct cypress_touchkey_info *info)
{
	setsda(info, 1);
	udelay((DELAY_TIME + 1) / 2);
}

static inline void scllo(struct cypress_touchkey_info *info)
{
	setscl(info, 0);
	udelay((DELAY_TIME + 1) / 2);
}

static int sclhi(struct cypress_touchkey_info *info)
{
#if defined(ISP_VERY_VERBOSE_DEBUG) || defined(ISP_VERBOSE_DEBUG)
	struct i2c_client *client = info->client;
#endif
	unsigned long start;
	int timeout = HZ / 20;
	int ex_count = 100000;

	setscl(info, 1);

	start = jiffies;
	while (!getscl(info)) {
		ex_count--;
		if (time_after(jiffies, start + timeout))
			return -ETIMEDOUT;
		else if (counting_timer && ex_count < 0)
			return -ETIMEDOUT;
	}

	if (jiffies != start)
#if defined(ISP_VERY_VERBOSE_DEBUG)
		dev_err(&client->dev, "%s: needed %ld jiffies for SCL to go "
			"high\n", __func__, jiffies - start);
#endif
	udelay(DELAY_TIME);

	return 0;
}

static void isp_start(struct cypress_touchkey_info *info)
{
	setsda(info, 0);
	udelay(DELAY_TIME);
	scllo(info);
}

static void isp_stop(struct cypress_touchkey_info *info)
{
	sdalo(info);
	sclhi(info);
	setsda(info, 1);
	udelay(DELAY_TIME);
}

static int isp_recvdbyte(struct cypress_touchkey_info *info)
{
	struct i2c_client *client = info->client;
	int i;
	u8 indata = 0;
	sdahi(info);
	for (i = 0; i < 8 ; i++) {
		if (sclhi(info) < 0) { /* timed out */
			dev_err(&client->dev, "%s: timeout at bit "
				"#%d\n", __func__, 7 - i);
			return -ETIMEDOUT;
		}

		indata = indata << 1;
		if (getsda(info))
			indata |= 0x01;

		setscl(info, 0);

		udelay(i == 7 ? DELAY_TIME / 2 : DELAY_TIME);
	}
	return indata;
}

static int isp_sendbyte(struct cypress_touchkey_info *info, u8 c)
{
	struct i2c_client *client = info->client;
	int i;
	int sb;
	int ack = 0;

	/* assert: scl is low */
	for (i = 7; i >= 0; i--) {
		sb = (c >> i) & 0x1;
		setsda(info, sb);
		udelay((DELAY_TIME + 1) / 2);

		if (sclhi(info) < 0) { /* timed out */
			dev_err(&client->dev, "%s: %#x, timeout at bit #%d\n",
				__func__, (int)c, i);
			return -ETIMEDOUT;
		}
		scllo(info);
	}
	sdahi(info);

	if (sclhi(info) < 0) { /* timed out */
		dev_err(&client->dev, "%s: %#x, timeout at bit #%d\n",
			__func__, (int)c, i);
		return -ETIMEDOUT;
	}

	ack = !getsda(info);

	scllo(info);

#if defined(ISP_VERY_VERBOSE_DEBUG)
	dev_info(&client->dev, "%s: %#x %s\n", __func__, (int)c,
		 ack ? "A" : "NA");
#endif
	return ack;
}

static int isp_master_recv(struct cypress_touchkey_info *info, u8 addr, u8 *val)
{
	struct i2c_client *client = info->client;
	int ret;
	int retries = 2;

retry:
	isp_start(info);

	ret = isp_sendbyte(info, addr);
	if (ret != TC360_ISP_ACK) {
		dev_err(&client->dev, "%s: %#x %s\n", __func__, addr, "NA");
		if (retries-- > 0) {
			dev_err(&client->dev, "%s: retry (%d)\n", __func__,
				retries);
			goto retry;
		}
		return -EIO;
	}
	*val = isp_recvdbyte(info);
	isp_stop(info);

	return 0;
}

static int isp_master_send(struct cypress_touchkey_info *info, u8 msg_1, u8 msg_2)
{
	struct i2c_client *client = info->client;
	int ret;
	int retries = 2;

retry:
	isp_start(info);
	ret = isp_sendbyte(info, msg_1);
	if (ret != TC360_ISP_ACK) {
		dev_err(&client->dev, "%s: %#x %s\n", __func__, msg_1, "NA");
		if (retries-- > 0) {
			dev_err(&client->dev, "%s: retry (%d)\n", __func__,
				retries);
			goto retry;
		}
		return -EIO;
	}
	ret = isp_sendbyte(info, msg_2);
	if (ret != TC360_ISP_ACK) {
		dev_err(&client->dev, "%s: %#x %s\n", __func__, msg_2, "NA");
		if (retries-- > 0) {
			dev_err(&client->dev, "%s: retry (%d)\n", __func__,
				retries);
			goto retry;
		}
		return -EIO;
	}
	isp_stop(info);

	return 0;
}

static void isp_sp_signal(struct cypress_touchkey_info *info)
{
	int i;
	unsigned long flags;

	local_irq_save(flags);
	for (i = TC360_NUM_OF_ISP_SP_SIGNAL - 1; i >= 0; i--) {
		int sb = (TC360_ISP_SP_SIGNAL >> i) & 0x1;
		setscl(info, sb);
		udelay(3);
		setsda(info, 0);
		udelay(10);
		setsda(info, 1);
		udelay(10);

		if (i == 5)
			udelay(30);
	}

	counting_timer = true;
	sclhi(info);
	counting_timer = false;

	local_irq_restore(flags);
}

static int raw_dbgir3(struct cypress_touchkey_info *info, u8 data2, u8 data1, u8 data0)
{
	struct i2c_client *client = info->client;
	int ret = 0;

	ret = ret | isp_master_send(info, 0xc2, data2);
	ret = ret | isp_master_send(info, 0xc4, data1);
	ret = ret | isp_master_send(info, 0xc6, data0);
	ret = ret | isp_master_send(info, 0xc0, 0x80);

	if (ret < 0) {
		dev_err(&client->dev, "fail to dbgir3 %#x,%#x,%#x (%d)\n",
			data2, data1, data0, ret);
		return ret;
	}

	return 0;
}

static int raw_dbgir2(struct cypress_touchkey_info *info, u8 data1, u8 data0)
{
	struct i2c_client *client = info->client;
	int ret = 0;

	ret = ret | isp_master_send(info, 0xc2, data1);
	ret = ret | isp_master_send(info, 0xc4, data0);
	ret = ret | isp_master_send(info, 0xc0, 0x80);

	if (ret < 0) {
		dev_err(&client->dev, "fail to dbgir2 %#x,%#x (%d)\n",
			data1, data0, ret);
		return ret;
	}

	return 0;
}

static int raw_spchl(struct cypress_touchkey_info *info, u8 data1, u8 data0)
{
	struct i2c_client *client = info->client;
	int ret = 0;

	ret = ret | isp_master_send(info, 0xd0, data1);
	ret = ret | isp_master_send(info, 0xd2, data0);

	if (ret < 0) {
		dev_err(&client->dev, "fail to spchl %#x,%#x (%d)\n",
			data1, data0, ret);
		return ret;
	}

	return 0;
}

static int isp_common_set(struct cypress_touchkey_info *info)
{
	struct i2c_client *client = info->client;
	int ret = 0;

	ret = ret | raw_dbgir3(info, 0x75 , 0x8f, 0x00);
	ret = ret | raw_dbgir3(info, 0x75 , 0xc6, 0x0e);
	ret = ret | raw_dbgir3(info, 0x75 , 0xf7, 0xc1);
	ret = ret | raw_dbgir3(info, 0x75 , 0xf7, 0x1e);
	ret = ret | raw_dbgir3(info, 0x75 , 0xf7, 0xec);
	ret = ret | raw_dbgir3(info, 0x75 , 0xf7, 0x81);

	if (ret < 0) {
		dev_err(&client->dev, "fail to %s (%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int isp_ers_timing_set(struct cypress_touchkey_info *info)
{
	struct i2c_client *client = info->client;
	int ret = 0;

	ret = ret | raw_dbgir3(info, 0x75, 0xf2, 0x90);
	ret = ret | raw_dbgir3(info, 0x75, 0xf3, 0xd0);
	ret = ret | raw_dbgir3(info, 0x75, 0xf4, 0x03);

	if (ret < 0) {
		dev_err(&client->dev, "fail to %s (%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int isp_pgm_timing_set(struct cypress_touchkey_info *info)
{
	struct i2c_client *client = info->client;
	int ret = 0;

	ret = ret | raw_dbgir3(info, 0x75, 0xf2, 0x94);
	ret = ret | raw_dbgir3(info, 0x75, 0xf3, 0x01);
	ret = ret | raw_dbgir3(info, 0x75, 0xf4, 0x00);

	if (ret < 0) {
		dev_err(&client->dev, "fail to %s (%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

static void reset_for_isp(struct cypress_touchkey_info *info)
{
	cypress_power_onoff(info, 0);

	gpio_direction_output(info->pdata->gpio_scl, 0);
	gpio_direction_output(info->pdata->gpio_sda, 0);

	msleep(TC360_POWERON_DELAY);

	gpio_direction_output(info->pdata->gpio_scl, 1);
	gpio_direction_output(info->pdata->gpio_sda, 1);

	cypress_power_onoff(info, 1);
	usleep_range(5000, 6000);
	dev_info(&info->client->dev, "%s: end\n", __func__);
}

static int tc360_erase_fw(struct cypress_touchkey_info *info)
{
	struct i2c_client *client = info->client;
	int ret;
	u16 addr = 0;
	int dcr_rd_cnt;
	u8 val;

	reset_for_isp(info);

	isp_sp_signal(info);
	ret = isp_common_set(info);
	if (ret < 0) {
		dev_err(&client->dev, "fail to %s (%d)\n", __func__, ret);
		return ret;
	}

	ret = isp_ers_timing_set(info);

	isp_master_send(info, 0xf8, 0x01);
	isp_master_send(info, 0xc8, 0xff);
	isp_master_send(info, 0xca, 0x42);

	while (addr < TC360_FW_ER_MAX_LEN) {
#if defined(ISP_DEBUG)
		dev_info(&client->dev, "fw erase addr=x0%4x\n", addr);
#endif
		raw_dbgir3(info, 0x75, 0xf1, 0x80);
		raw_dbgir3(info, 0x90, (u8)(addr >> 8), (u8)(addr & 0xff));

		raw_spchl(info, 0xff, 0x3a);
		isp_master_send(info, 0xc0, 0x14);


		val = 0;
		dcr_rd_cnt = TC360_DCR_RD_RETRY;
		do {
			isp_master_recv(info, 0xc1, &val);
			if (dcr_rd_cnt-- < 0) {
				dev_err(&client->dev, "%s: fail to update "
					"dcr\n", __func__);
				return -ENOSYS;
			}
			usleep_range(10000, 15000);
		} while (val != 0x12);
#if defined(ISP_VERBOSE_DEBUG)
			dev_info(&client->dev, "dcr_rd_cnt=%d\n", dcr_rd_cnt);
#endif
		addr += 0x400;
	}

	return 0;
}

static int tc360_write_fw(struct cypress_touchkey_info *info)
{
	u16 addr = 0;
	u8 val;
	struct i2c_client *client = info->client;

	reset_for_isp(info);

	isp_sp_signal(info);
	isp_common_set(info);

	isp_pgm_timing_set(info);
	isp_master_send(info, 0xf8, 0x01);
	isp_master_send(info, 0xc8, 0xff);
	isp_master_send(info, 0xca, 0x20);

	raw_dbgir3(info, 0x90, (u8)(addr >> 8), (u8)(addr & 0xff));
	raw_spchl(info, 0xff, 0x1e);

	while (addr < info->fw_img->fw_len) {
#if defined(ISP_DEBUG)
		dev_info(&client->dev, "fw write addr=%#x\n", addr);
#endif
		u8 __fw_data = info->fw_img->data[addr];
		raw_dbgir2(info, 0x74, __fw_data);
		raw_dbgir3(info, 0x75, 0xf1, 0x80);
		isp_master_send(info, 0xc0, 0x14);
#if defined(ISP_VERBOSE_DEBUG)
		dev_info(&client->dev, "dcr_rd_cnt=%d\n", dcr_rd_cnt);
#endif
		isp_master_recv(info, 0xd9, &val);
		
		if (info->fw_img->data[addr] != val) {
			dev_err(&client->dev, "fail to verify at %#x (%#x)\n",
				addr, info->fw_img->data[addr]);
			return -EIO;
		}

		/* increase address */
		isp_master_send(info, 0xc0, 0x08);
		addr++;
	}

	return 0;
}

int coreriver_fw_update(struct cypress_touchkey_info *info, bool force)
{
	struct i2c_client *client = info->client;
	int retries;
	int ret=0;
	dev_info(&client->dev, "%s : touchkey_update Start!!\n", __func__);

	if (force == true)
		retries = 1;
	else
		retries = TC360_FW_FLASH_RETRY;
erase_fw:
	ret = tc360_erase_fw(info);
	if (ret < 0) {
		dev_err(&client->dev, "fail to erase fw (%d)\n", ret);
		if (retries-- > 0) {
			dev_info(&client->dev, "retry esasing fw (%d)\n",
				 retries);
			goto erase_fw;
		} else {
			goto err;
		}
	}
	dev_info(&client->dev, "succeed in erasing fw\n");

	retries = TC360_FW_FLASH_RETRY;
write_fw:
	ret = tc360_write_fw(info);
	if (ret < 0) {
		dev_err(&client->dev, "fail to write fw (%d)\n", ret);
		if (retries-- > 0) {
			dev_info(&client->dev, "retry writing fw (%d)\n",
				 retries);
			goto write_fw;
		} else {
			goto err;
		}
	}
	dev_info(&client->dev, "succeed in writing/verify fw\n");
	
	cypress_power_onoff(info,0);
	msleep(TC360_POWERON_DELAY);
	cypress_power_onoff(info,1);
	msleep(150);

	retries = TC360_FW_VER_READ;
read_flashed_fw_ver:
	info->ic_fw_ver = i2c_smbus_read_byte_data(info->client,
			CORERIVER_FW_VER);
	dev_info(&client->dev,
		"%s : FW Ver 0x%02x\n", __func__, info->ic_fw_ver);

	if (info->ic_fw_ver < 0) {
		dev_err(&client->dev, "fail to read fw ver (%d)\n", info->ic_fw_ver);
		if (retries-- > 0) {
			dev_info(&client->dev, "retry read flash fw ver (%d)\n",
				 retries);
			goto read_flashed_fw_ver;
		} else {
			goto err;
		}
	}

	ret = 0;
	dev_info(&client->dev, "succeed in flashing fw\n");

	return ret;

err:
/* data is not deallocated for debugging. */
	dev_err(&client->dev, "fail to fw flash. driver is removed\n");
	return ret;
}
