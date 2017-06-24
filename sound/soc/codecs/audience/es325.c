/*
 * es325.c  --  Audience eS325 ALSA SoC Audio driver
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

#define FIXED_CONFIG

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/completion.h>
#include <linux/i2c.h>
#include <linux/slimbus/slimbus.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/kthread.h>
#include <linux/device.h>
#include <linux/pm_runtime.h>
#include <linux/mutex.h>
#include <linux/slimbus/slimbus.h>
#include <linux/esxxx.h>
#include "es325.h"
#include "es325-slim.h"
#include "es325-i2c.h"
#include "es325-i2s.h"

#define ES325_CMD_ACCESS_WR_MAX 2
#define ES325_CMD_ACCESS_RD_MAX 2
struct es325_api_access {
	u32 read_msg[ES325_CMD_ACCESS_RD_MAX];
	unsigned int read_msg_len;
	u32 write_msg[ES325_CMD_ACCESS_WR_MAX];
	unsigned int write_msg_len;
	unsigned int val_shift;
	unsigned int val_max;
};

#include "es325-access.h"

/* codec private data */
struct es325_priv es325_priv = {
	.rx1_route_enable = 0,
	.tx1_route_enable = 0,
	.rx2_route_enable = 0,

	.ap_tx1_ch_cnt = 2,
};

static unsigned int es325_valid_power_states[] = {
	ES325_POWER_STATE_SLEEP,
	ES325_POWER_STATE_NORMAL,
};

#define ES325_CUSTOMER_PROFILE_MAX 4
static u32 es325_audio_custom_profiles[ES325_CUSTOMER_PROFILE_MAX][20] = {
	{
		0xffffffff	/* terminate */
	},
	{
		0xffffffff	/* terminate */
	},
	{
		0xffffffff	/* terminate */
	},
	{
		0xffffffff	/* terminate */
	},
};
#define ES325_INTERNAL_ROUTE_MAX 16
static long es325_internal_route_num;
static u32 es325_internal_route_configs[ES325_INTERNAL_ROUTE_MAX][20] = {
	/* [0]: Audio route reset */
	{
		0xb04e0000,	/* Gain rate change = 0 */
		0x905c0000,	/* stop route */
		0xffffffff	/* terminate */
	},
	/* [1]: Audio playback, 1 channel */
	{
		0xb05c0004,	/* Algo = passthrough */
		0xb04c0001,	/* Algo rate = 16kHz */
		0xb05a18a0,	/* SBUS.Rx0 -> AUDIN1 */
		0xb05a4cac,	/* SBUS.Tx2 <- AUDOUT1 */
		0x901c0000,	/* Algo processing = off (COMMIT) */
		0xffffffff	/* terminate */
	},
	/* [2]: Audio playback, 2 channels */
	{
		0xb05c0004,	/* Algo = passthrough */
		0xb04c0001,	/* Algo rate = 16kHz */
		0xb05a18a0,	/* SBUS.Rx0 -> AUDIN1 */
		0xb05a1ca1,	/* SBUS.Rx1 -> AUDIN2 */
		0xb05a4cac,	/* SBUS.Tx2 <- AUDOUT1 */
		0xb05a50ad,	/* SBUS.Tx3 <- AUDOUT2 */
		0x901c0000,	/* Algo processing = off (COMMIT) */ 
		0xffffffff	/* terminate */
	},
	/* [3]: Audio record, 1 channel */
	{ 
		0xb05c0004,	/* Algo = passthrough */
		0xb04c0001,	/* Algo rate = 16kHz */
		0xb05a18a4,	/* SBUS.Rx4 -> AUDIN1 */
		0xb05a4caa,	/* SBUS.Tx0 <- AUDOUT1 */
		0x901c0000,	/* Algo processing = off (COMMIT) */
		0xffffffff	/* terminate */
	},
	/* [4]: Audio record, 2 channels */
	{ 
		0xb05c0004,	/* Algo = passthrough */
		0xb04c0001,	/* Algo rate = 16kHz */
		0xb05a18a4,	/* SBUS.Rx4 -> AUDIN1 */
		0xb05a1ca5,	/* SBUS.Rx5 -> AUDIN2 */
		0xb05a4caa,	/* SBUS.Tx0 <- AUDOUT1 */
		0xb05a50ab,	/* SBUS.Tx1 <- AUDOUT2 */
		0x901c0000,	/* Algo processing = off (COMMIT) */
		0xffffffff	/* terminate */
	},
	/* [5]: 1-mic Headset */
	{ 
		0xb05c0001,	/* Algo = Voice processing */
		0xb04c0001,	/* Algo rate = 16kHz */
		0xb05e0001,	/* Mix rate = 16 kHz */
		0xb05a28a0,	/* SBUS.Rx0 -> UITONE1 (host to 325) */
		0xb05a14a2,	/* SBUS.Rx2 -> FEIN (host to 325) */
		0xb05a04a4,	/* SBUS.Rx4 -> PRI (codec to 325) */
		0xb05a40aa,	/* SBUS.Tx0 <- CSOUT (325 to host) */
		0xb05a44ae,	/* SBUS.Tx4 <- FEOUT1 (325 to codec) */
		0xb05a48af,	/* SBUS.Tx5 <- FEOUT2 (325 to codec) */
		0x901c0000,	/* Algo processing = off (COMMIT) */
		0xffffffff	/* terminate */
	},
	/* [6]: 1-mic CS Voice (CT) */
	{ 
		0xb05c0001,	/* Algo = Voice processing */
		0xb04c0001,	/* Algo rate = 16kHz */
		0xb05e0001,	/* Mix rate = 16 kHz */
		0xb05a28a0,	/* SBUS.Rx0 -> UITONE1 (host to 325) */
		0xb05a14a2,	/* SBUS.Rx2 -> FEIN (host to 325) */
		0xb05a04a4,	/* SBUS.Rx4 -> PRI (codec to 325) */
		0xb05a40aa,	/* SBUS.Tx0 <- CSOUT (325 to host) */
		0xb05a44ae,	/* SBUS.Tx4 <- FEOUT1 (325 to codec) */
		0xb0170002,	/* Mic config = ... */
		0xb0180002,	/*      ... 1-MIC CT */
		0x901c0000,	/* Algo processing = off  */
		0xffffffff	/* terminate */
	},
	/* [7]: 2-mic CS Voice (CT) */
	{
		0xb05c0001,	/* Algo = Voice processing */
		0xb04c0001,	/* Algo rate = 16 kHz */
		0xb05e0001,	/* Mix rate = 16 kHz */
		0xb05a28a0,	/* SBUS.Rx0 -> UITONE1 (host to 325) */
		0xb05a14a2,	/* SBUS.Rx2 -> FEIN (host to 325) */
		0xb05a04a4,	/* SBUS.Rx4 -> PRI (codec to 325) */
		0xb05a08a5,	/* SBUS.Rx5 -> SEC (codec to 325) */
		0xb05a40aa,	/* SBUS.Tx0 <- CSOUT (325 to host) */
		0xb05a44ae,	/* SBUS.Tx4 <- FEOUT1 (325 to codec) */
		0xb0170002,	/* Mic config = ... */
		0xb0180000,	/*      ... 2-MIC CT */
		0x901c0000,	/* Algo processing = off (COMMIT) */
		0xffffffff	/* terminate */
	},
	/* [8]: 1-mic VOIP (CT) */
	{
		0xb05c0001,	/* Algo = Voice processing */
		0xb04c0001,	/* Algo rate = 16 kHz */
		0xb05e0001,	/* Mix rate = 16 kHz */
		0xb05a14a0,	/* SBUS.Rx0 -> FEIN */
		0xb05a04a4,	/* SBUS.Rx4 -> PRI (codec to 325) */
		0xb05a40aa,	/* SBUS.Tx0 <- CSOUT (325 to host) */
		0xb05a44ac,	/* SBUS.Tx2 <- FEOUT1 */
		0xb0170002,	/* Mic config = ... */
		0xb0180002,	/*      ... 1-MIC CT */
		0x901c0000,	/* Algo processing = off (COMMIT) */
		0xffffffff	/* terminate */
	},
	/* [9]: 2-mic VOIP (CT) */
	{
		0xb05c0001,	/* Algo = Voice processing */
		0xb04c0001,	/* Algo rate = 16 kHz */
		0xb05e0001,	/* Mix rate = 16 kHz */
		0xb05a14a0,	/* SBUS.Rx0 -> FEIN */
		0xb05a04a4,	/* SBUS.Rx4 -> PRI (codec to 325) */
		0xb05a08a5,	/* SBUS.Rx5 -> SEC (codec to 325) */
		0xb05a40aa,	/* SBUS.Tx0 <- CSOUT (325 to host) */
		0xb05a44ac,	/* SBUS.Tx2 <- FEOUT1 */
		0xb0170002,	/* Mic config = ... */
		0xb0180000,	/*      ... 2-MIC CT */
		0x901c0000,	/* Algo processing = off (COMMIT) */
		0xffffffff	/* terminate */
	},
	/* [10]: 1-mic CS Voice (FT) */
	{ 
		0xb05c0001,	/* Algo = Voice processing */
		0xb04c0001,	/* Algo rate = 16kHz */
		0xb05e0001,	/* Mix rate = 16 kHz */
		0xb05a28a0,	/* SBUS.Rx0 -> UITONE1 (host to 325) */
		0xb05a14a2,	/* SBUS.Rx2 -> FEIN (host to 325) */
		0xb05a04a4,	/* SBUS.Rx4 -> PRI (codec to 325) */
		0xb05a40aa,	/* SBUS.Tx0 <- CSOUT (325 to host) */
		0xb05a44ae,	/* SBUS.Tx4 <- FEOUT1 (325 to codec) */
		0xb0170002,	/* Mic config = ... */
		0xb0180002,	/*      ... 1-MIC FT */
		0x901c0000,	/* Algo processing = off  */
		0xffffffff	/* terminate */
	},
	/* [11]: 2-mic CS Voice (FT) */
	{
		0xb05c0001,	/* Algo = Voice processing */
		0xb04c0001,	/* Algo rate = 16 kHz */
		0xb05e0001,	/* Mix rate = 16 kHz */
		0xb05a28a0,	/* SBUS.Rx0 -> UITONE1 (host to 325) */
		0xb05a14a2,	/* SBUS.Rx2 -> FEIN (host to 325) */
		0xb05a04a4,	/* SBUS.Rx4 -> PRI (codec to 325) */
		0xb05a08a5,	/* SBUS.Rx5 -> SEC (codec to 325) */
		0xb05a40aa,	/* SBUS.Tx0 <- CSOUT (325 to host) */
		0xb05a44ae,	/* SBUS.Tx4 <- FEOUT1 (325 to codec) */
		0xb0170002,	/* Mic config = ... */
		0xb0180001,	/*      ... 2-MIC FT */
		0x901c0000,	/* Algo processing = off (COMMIT) */
		0xffffffff	/* terminate */
	},
	/* [12]: 1-mic VOIP (FT) */
	{
		0xb05c0001,	/* Algo = Voice processing */
		0xb04c0001,	/* Algo rate = 16 kHz */
		0xb05e0001,	/* Mix rate = 16 kHz */
		0xb05a14a0,	/* SBUS.Rx0 -> FEIN */
		0xb05a04a4,	/* SBUS.Rx4 -> PRI (codec to 325) */
		0xb05a40aa,	/* SBUS.Tx0 <- CSOUT (325 to host) */
		0xb05a44ac,	/* SBUS.Tx2 <- FEOUT1 */
		0xb0170002,	/* Mic config = ... */
		0xb0180002,	/*      ... 1-MIC FT */
		0x901c0000,	/* Algo processing = off (COMMIT) */
		0xffffffff	/* terminate */
	},
	/* [13]: 2-mic VOIP (FT) */
	{
		0xb05c0001,	/* Algo = Voice processing */
		0xb04c0001,	/* Algo rate = 16 kHz */
		0xb05e0001,	/* Mix rate = 16 kHz */
		0xb05a14a0,	/* SBUS.Rx0 -> FEIN */
		0xb05a04a4,	/* SBUS.Rx4 -> PRI (codec to 325) */
		0xb05a08a5,	/* SBUS.Rx5 -> SEC (codec to 325) */
		0xb05a40aa,	/* SBUS.Tx0 <- CSOUT (325 to host) */
		0xb05a44ac,	/* SBUS.Tx2 <- FEOUT1 */
		0xb0170002,	/* Mic config = ... */
		0xb0180001,	/*      ... 2-MIC FT */
		0x901c0000,	/* Algo processing = off (COMMIT) */
		0xffffffff	/* terminate */
	},
};

struct snd_soc_dai_driver es325_dai[];

static void es325_fixed_config(struct es325_priv *es325);
/* static void es325_ping(struct es325_priv *es325); */

static int es325_write_block(struct es325_priv *es325, u32 *cmd_block)
{
	u32 api_word;
	u8 msg[4];
	int rc = 0;

	pr_debug("%s(): pm_runtime_get_sync()\n", __func__);
	pr_debug("%s(): mutex lock\n", __func__);
	mutex_lock(&es325->api_mutex);
	while (*cmd_block != 0xffffffff) {
		if (es325_priv.intf == ES325_I2C_INTF)
			api_word = cpu_to_be32(*cmd_block);
		else
			api_word = cpu_to_le32(*cmd_block);

		memcpy(msg, (char *)&api_word, 4);
		es325->dev_write(es325, msg, 4);
		usleep_range(1000, 1000);
		pr_debug("%s(): msg = %02x%02x%02x%02x\n", __func__,
			msg[0], msg[1], msg[2], msg[3]);
		cmd_block++;
	}
	pr_debug("%s(): mutex unlock\n", __func__);
	mutex_unlock(&es325->api_mutex);
	pr_debug("%s(): pm_runtime_put_autosuspend()\n", __func__);

	return rc;
}
#ifdef FIXED_CONFIG
static void es325_fixed_config(struct es325_priv *es325)
{
	int rc;

	rc = es325_write_block(es325, &es325_internal_route_configs[6][0]);
}
#endif

static void es325_switch_route(long route_index)
{
	struct es325_priv *es325 = &es325_priv;
	int rc;

	if (route_index > ES325_INTERNAL_ROUTE_MAX) {
		pr_debug("%s(): new es325_internal_route = %ld is out of range\n",
			 __func__, route_index);
		return;
	}

	pr_debug("%s():switch current es325_internal_route = %ld to new route = %ld\n",
		__func__, es325_internal_route_num, route_index);
	es325_internal_route_num = route_index;
	rc = es325_write_block(es325,
			  &es325_internal_route_configs[es325_internal_route_num][0]);
}

/*
static void es325_ping(struct es325_priv *es325)
{
	unsigned int value = 0;

	value = es325_read(NULL, ES325_CHANGE_STATUS);

	pr_debug("%s(): ping ack = %04x\n", __func__, value);
}
*/

static unsigned int es325_read(struct snd_soc_codec *codec,
			       unsigned int reg)
{
	struct es325_priv *es325 = &es325_priv;
	struct es325_api_access *api_access;
	u32 api_word[2] = {0};
	char req_msg[8];
	char ack_msg[8];
	char *msg_ptr;
	unsigned int msg_len;
	unsigned int value;
	int rc;

	if (reg > ES325_API_ADDR_MAX) {
		pr_err("%s(): invalid address = 0x%04x\n", __func__, reg);
		return -EINVAL;
	}

	api_access = &es325_api_access[reg];
	msg_len = api_access->read_msg_len;
	memcpy((char *)api_word, (char *)api_access->read_msg, msg_len);
	switch (msg_len) {
	case 8:
		{
			if (es325_priv.intf == ES325_I2C_INTF)
				cpu_to_be32s(&api_word[1]);
			else
				cpu_to_le32s(&api_word[1]);
		}
	case 4:
		{
			if (es325_priv.intf == ES325_I2C_INTF)
				cpu_to_be32s(&api_word[0]);
			else
				cpu_to_le32s(&api_word[0]);
		}
	}
	memcpy(req_msg, (char *)api_word, msg_len);

	msg_ptr = req_msg;
	pr_debug("%s(): mutex lock\n", __func__);
	mutex_lock(&es325->api_mutex);
	rc = es325->dev_write(es325, msg_ptr, 4);
	if (rc < 0) {
		pr_err("%s(): es325_xxxx_write()", __func__);
		pr_debug("%s(): mutex unlock\n", __func__);
		mutex_unlock(&es325->api_mutex);
		return rc;
	}
	msleep(20);
	rc = es325->dev_read(es325, ack_msg, 4);
	pr_debug("%s(): mutex unlock\n", __func__);
	mutex_unlock(&es325->api_mutex);
	if (rc < 0) {
		pr_err("%s(): es325_xxxx_read()", __func__);
		return rc;
	}
	memcpy((char *)&api_word[0], ack_msg, 4);
	if (es325_priv.intf == ES325_I2C_INTF)
		be32_to_cpus(&api_word[0]);
	else
		le32_to_cpus(&api_word[0]);
	value = api_word[0] & 0xffff;

	return value;
}

static int es325_write(struct snd_soc_codec *codec, unsigned int reg,
		       unsigned int value)
{
	struct es325_priv *es325 = &es325_priv;
	struct es325_api_access *api_access;
	u32 api_word[2] = {0};
	char msg[8];
	char *msg_ptr;
	int msg_len;
	unsigned int val_mask;
	int i;
	int rc = 0;

	if (reg > ES325_API_ADDR_MAX) {
		pr_err("%s(): invalid address = 0x%04x\n", __func__, reg);
		return -EINVAL;
	}

	api_access = &es325_api_access[reg];
	msg_len = api_access->write_msg_len;
	val_mask = (1 << get_bitmask_order(api_access->val_max)) - 1;
	pr_info("GAC: %s(): msg_len = %d\n", __func__, msg_len);
	pr_info("GAC: %s(): val_mask = 0x%08x\n", __func__, val_mask);
	memcpy((char *)api_word, (char *)api_access->write_msg, msg_len);
	switch (msg_len) {
	case 8:
		api_word[1] |= (val_mask & value);
		break;
	case 4:
		api_word[0] |= (val_mask & value);
		break;
	}
	switch (msg_len) {
	case 8:
		{
			if (es325_priv.intf == ES325_I2C_INTF)
				cpu_to_be32s(&api_word[1]);
			else
				cpu_to_le32s(&api_word[1]);
		}
	case 4:
		{
			if (es325_priv.intf == ES325_I2C_INTF)
				cpu_to_be32s(&api_word[0]);
			else
				cpu_to_le32s(&api_word[0]);
		}
	}
	memcpy(msg, (char *)api_word, msg_len);

	msg_ptr = msg;
	pr_debug("%s(): mutex lock\n", __func__);
	mutex_lock(&es325->api_mutex);
	for (i = msg_len; i > 0; i -= 4) {
		rc = es325->dev_write(es325, msg_ptr, 4);
		usleep_range(5000, 5000);
		if (rc < 0) {
			pr_err("%s(): es325_xxxx_write()", __func__);
			pr_debug("%s(): mutex unlock\n", __func__);
			mutex_unlock(&es325->api_mutex);
			return rc;
		}
		msg_ptr += 4;
	}
	pr_debug("%s(): mutex unlock\n", __func__);
	mutex_unlock(&es325->api_mutex);

	return rc;
}

static ssize_t es325_route_status_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	int ret = 0;
	unsigned int value = 0;
	char *status_name = "Route Status";

	value = es325_read(NULL, ES325_CHANGE_STATUS);

	ret = snprintf(buf, PAGE_SIZE,
		       "%s=0x%04x\n",
		       status_name, value);

	return ret;
}

static DEVICE_ATTR(route_status, 0444, es325_route_status_show, NULL);
/* /sys/devices/platform/msm_slim_ctrl.1/es325-codec-gen0/route_status */

static ssize_t es325_route_config_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	pr_debug("%s(): route=%ld\n", __func__, es325_internal_route_num);
	return snprintf(buf, PAGE_SIZE, "route=%ld\n",
		       es325_internal_route_num);
}

static DEVICE_ATTR(route_config, 0444, es325_route_config_show, NULL);
/* /sys/devices/platform/msm_slim_ctrl.1/es325-codec-gen0/route_config */

#define SIZE_OF_VERBUF 256
/* TODO: fix for new read/write. use es325_read() instead of BUS ops */
static ssize_t es325_fw_version_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int idx = 0;
	unsigned int value;
	char versionbuffer[SIZE_OF_VERBUF];
	char *verbuf = versionbuffer;

	memset(verbuf, 0, SIZE_OF_VERBUF);

	value = es325_read(NULL, ES325_FW_FIRST_CHAR);
	*verbuf++ = (value & 0x00ff);
	for (idx = 0; idx < (SIZE_OF_VERBUF-2); idx++) {
		value = es325_read(NULL, ES325_FW_NEXT_CHAR);
		*verbuf++ = (value & 0x00ff);
	}
	/* Null terminate the string*/
	*verbuf = '\0';
	pr_info("Audience fw ver %s\n", versionbuffer);
	return snprintf(buf, PAGE_SIZE, "FW Version = %s\n", versionbuffer);
}

static DEVICE_ATTR(fw_version, 0444, es325_fw_version_show, NULL);
/* /sys/devices/platform/msm_slim_ctrl.1/es325-codec-gen0/fw_version */

static ssize_t es325_txhex_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	pr_debug("%s called\n", __func__);
	return 0;
}

/* TODO: fix for new read write */
static ssize_t es325_txhex_set(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct es325_priv *es325 = &es325_priv;
	u8 cmd[128];
	int cmdlen;
	int offset;
	u8 resp[4];
	int rc = 0;

	pr_debug("%s called\n", __func__);
	pr_debug("%s count=%i\n", __func__, count);

	/* No command sequences larger than 128 bytes. */
	BUG_ON(count > (128 * 2) + 1);
	/* Expect a even number of hexadecimal digits terminated by a
	 * newline. */
	BUG_ON(!(count & 1));

	hex2bin(cmd, buf, count / 2);
	BUG_ON(rc != 0);
	pr_debug("%s rc==%i\n", __func__, rc);
	cmdlen = count / 2;
	offset = 0;
	pr_debug("%s cmdlen=%i\n", __func__, cmdlen);
	pr_debug("%s(): mutex lock\n", __func__);
	mutex_lock(&es325->api_mutex);
	while (offset < cmdlen) {
		/* Commands must be written in 4 byte blocks. */
		int wrsize = (cmdlen - offset > 4) ? 4 : cmdlen - offset;
		/* TODO: mutex, cpu_to_le32() */
		es325->dev_write(es325, &cmd[offset], wrsize);
		usleep_range(10000, 10000);
		es325->dev_read(es325, resp, 4);
		pr_debug("%s: %02x%02x%02x%02x\n", __func__,
			 resp[0], resp[1], resp[2], resp[3]);
		offset += wrsize;
	}
	pr_debug("%s(): mutex unlock\n", __func__);
	mutex_unlock(&es325->api_mutex);

	return count;
}

static DEVICE_ATTR(txhex, 0644, es325_txhex_show, es325_txhex_set);
/* /sys/devices/platform/msm_slim_ctrl.1/es325-codec-gen0/txhex */

static ssize_t es325_clock_on_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int ret = 0;

	return ret;
}

static DEVICE_ATTR(clock_on, 0444, es325_clock_on_show, NULL);
/* /sys/devices/platform/msm_slim_ctrl.1/es325-codec-gen0/clock_on */

#define ES325_FW_LOAD_BUF_SZ 4


int es325_bootup(struct es325_priv *es325)
{
	u32 boot_cmd = ES325_BOOT_CMD;
	u32 boot_ack;
	u32 sync_cmd = (ES325_SYNC_CMD << 16) | ES325_SYNC_POLLING;
	u32 sync_ack;
	char msg[4];
	unsigned int buf_frames;
	char *buf_ptr;
	int rc;

	pr_debug("%s()\n", __func__);

	pr_debug("%s(): mutex lock\n", __func__);
	mutex_lock(&es325->api_mutex);
	pr_debug("%s(): write ES325_BOOT_CMD = 0x%08x\n", __func__, boot_cmd);
#ifdef CONFIG_SND_SOC_ES325_SLIM
	cpu_to_le32s(&boot_cmd);
#endif
#ifdef CONFIG_SND_SOC_ES325_I2C
	cpu_to_be32s(&boot_cmd);
#endif
	memcpy(msg, (char *)&boot_cmd, 4);
	rc = es325->dev_write(es325, msg, 4);
	if (rc < 0) {
		pr_err("%s(): firmware load failed boot write\n", __func__);
		goto es325_bootup_failed;
	}
	usleep_range(1000, 1000);
	memset(msg, 0, 4);
	rc = es325->dev_read(es325, msg, 4);
	if (rc < 0) {
		pr_err("%s(): firmware load failed boot ack\n", __func__);
		goto es325_bootup_failed;
	}
	memcpy((char *)&boot_ack, msg, 4);
#ifdef CONFIG_SND_SOC_ES325_SLIM
	cpu_to_le32s(&boot_ack);
#endif
	pr_debug("%s(): boot_ack = 0x%08x\n", __func__, boot_ack);
	if (boot_ack != ES325_BOOT_ACK) {
		pr_err("%s(): firmware load failed boot ack pattern", __func__);
		rc = -EIO;
		goto es325_bootup_failed;
	}

	pr_debug("%s(): write firmware image\n", __func__);
	/* send image */
	buf_frames = es325->fw->size / ES325_FW_LOAD_BUF_SZ;
	buf_ptr = (char *)es325->fw->data;
	for ( ; buf_frames; --buf_frames, buf_ptr += ES325_FW_LOAD_BUF_SZ) {
		rc = es325->dev_write(es325, buf_ptr, ES325_FW_LOAD_BUF_SZ);
		if (rc < 0) {
			pr_err("%s(): firmware load failed\n", __func__);
			rc = -EIO;
			goto es325_bootup_failed;
		}
	}
	if (es325->fw->size % ES325_FW_LOAD_BUF_SZ) {
		rc = es325->dev_write(es325, buf_ptr,
				      es325->fw->size % ES325_FW_LOAD_BUF_SZ);
		if (rc < 0) {
			pr_err("%s(): firmware load failed\n", __func__);
			rc = -EIO;
			goto es325_bootup_failed;
		}
	}

	/* Give the chip some time to become ready after firmware
	 * download. */
	msleep(20);

#ifdef CONFIG_SND_SOC_ES325_SLIM
	cpu_to_le32s(&sync_cmd);
#endif
#ifdef CONFIG_SND_SOC_ES325_I2C
	cpu_to_be32s(&sync_cmd);
#endif
	pr_debug("%s(): write ES325_SYNC_CMD = 0x%08x\n", __func__, sync_cmd);
	memcpy(msg, (char *)&sync_cmd, 4);
	rc = es325->dev_write(es325, msg, 4);
	if (rc < 0) {
		pr_err("%s(): firmware load failed sync write\n", __func__);
		goto es325_bootup_failed;
	}
	msleep(100);
	memset(msg, 0, 4);
	rc = es325->dev_read(es325, msg, 4);
	if (rc < 0) {
		pr_err("%s(): error reading sync ack rc=%d\n",
		       __func__, rc);
		goto es325_bootup_failed;
	}
	memcpy((char *)&sync_ack, msg, 4);
#ifdef CONFIG_SND_SOC_ES325_SLIM
	le32_to_cpus(&sync_ack);
#endif
#ifdef CONFIG_SND_SOC_ES325_I2C
	be32_to_cpus(&sync_ack);
#endif
	pr_debug("%s(): sync_ack = 0x%08x\n", __func__, sync_ack);
	if (sync_ack != ES325_SYNC_ACK) {
		pr_err("%s(): firmware load failed sync ack pattern", __func__);
		rc = -EIO;
		goto es325_bootup_failed;
	}
	pr_debug("%s(): firmware load succes", __func__);

es325_bootup_failed:
	pr_debug("%s(): mutex unlock\n", __func__);
	mutex_unlock(&es325->api_mutex);

	return rc;
}

static int register_snd_soc(struct es325_priv *priv);

int fw_download(void *arg)
{
	struct es325_priv *priv = (struct es325_priv *)arg;
	int rc;

	pr_debug("%s(): called\n", __func__);
	pr_debug("%s(): es325 gen0 LA=%d\n", __func__, priv->gen0_client->laddr);
	rc = es325_bootup(priv);
	pr_debug("%s(): bootup rc=%d\n", __func__, rc);
	release_firmware(priv->fw);

	rc = register_snd_soc(priv);
	pr_debug("%s(): register_snd_soc rc=%d\n", __func__, rc);

#ifdef FIXED_CONFIG
	es325_fixed_config(priv);
#endif

	pr_debug("%s(): release module\n", __func__);
	module_put(THIS_MODULE);
	return 0;
}

static int es325_sleep(struct es325_priv *es325)
{
	int rc;
	u32 reg = ES325_POWER_STATE;
	u32 power_state = ES325_POWER_STATE_SLEEP;
	u32 smooth_mute = ES325_SMOOTH_MUTE;
	char msg[4];

	cpu_to_le32s(&smooth_mute);
	memset(msg, 0, 4);
	memcpy(msg, (char *)&smooth_mute, 4);
	rc = es325->dev_write(es325, msg, 4);
	if (rc < 0) {
		pr_err("%s(): Smooth mute write failed\n", __func__);
		return rc;
	}
	/* Send Sleep Command */
	rc = es325_write(NULL, reg, power_state);
	if (rc < 0) {
		pr_err("%s(): failed power state write\n",
				__func__);
		return rc;
	}
	/* sleep range 20 ms to 25 ms */
	usleep_range(20000, 25000);

	/* Clock off */
	if (es325->pdata->esxxx_clk_cb)
		es325->pdata->esxxx_clk_cb(0);

	return rc;
}

static int es325_wakeup(struct es325_priv *es325)
{
	u32 sync_cmd = (ES325_SYNC_CMD << 16) | ES325_SYNC_POLLING;
	u32 sync_ack;
	int retryCount = 10, rc;
	char msg[4];

	pr_debug("%s()\n", __func__);

	/* Clock On */
	if (es325->pdata->esxxx_clk_cb)
		es325->pdata->esxxx_clk_cb(0);

	/* Wakeup GPIO H */
	gpio_set_value(es325->pdata->wakeup_gpio, 1);

	/* Sleep range 1 ms to 3 ms */
	usleep_range(1000, 3000);
	gpio_set_value(es325->pdata->wakeup_gpio, 0);

	do {
		/* Sleep 30 ms */
		msleep(30);
		pr_debug("%s(): write ES325_SYNC_CMD = 0x%08x\n",
				__func__, sync_cmd);
		cpu_to_le32s(&sync_cmd);
		memset(msg, 0, 4);
		memcpy(msg, (char *)&sync_cmd, 4);
		rc = es325->dev_write(es325, msg, 4);
		if (!rc) {
			pr_info("%s(): wakeup sync write success\n", __func__);
			break;
		} else
			pr_err("%s(): wakeup sync write failed\n", __func__);

	} while (--retryCount);

	if (retryCount == 0)
		goto es325_wakeup_gpio;

	/* reset retry count */
	retryCount = 10;
	do {
		msleep(30);
		memset(msg, 0, 4);
		rc = es325->dev_read(es325, msg, 4);
		if (rc < 0) {
			pr_err("%s(): error wakeup reading sync ack rc=%d\n",
		       __func__, rc);
			continue;
		}
		memcpy((char *)&sync_ack, msg, 4);
		le32_to_cpus(&sync_ack);
		pr_debug("%s(): sync_ack = 0x%08x\n", __func__, sync_ack);
		if (sync_ack != ES325_SYNC_ACK) {
			pr_err("%s(): failed sync ack pattern", __func__);
			rc = -EIO;
		}

		break;

	} while (--retryCount);

es325_wakeup_gpio:
	gpio_set_value(es325->pdata->wakeup_gpio, 1);

	return rc;
}

static int es325_put_control_value(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	/* struct snd_soc_codec *codec = es325_priv.codec; */
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc = 0;

	value = ucontrol->value.integer.value[0];
	rc = es325_write(NULL, reg, value);

	return 0;
}

static int es325_get_control_value(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	/* struct snd_soc_codec *codec = es325_priv.codec; */
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;

	value = es325_read(NULL, reg);
	ucontrol->value.integer.value[0] = value;

	return 0;
}

static int es325_put_control_enum(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e =
		(struct soc_enum *)kcontrol->private_value;
	unsigned int reg = e->reg;
	unsigned int value;
	int rc = 0;

	value = ucontrol->value.enumerated.item[0];
	rc = es325_write(NULL, reg, value);

	return 0;
}

static int es325_get_control_enum(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e =
		(struct soc_enum *)kcontrol->private_value;
	unsigned int reg = e->reg;
	unsigned int value;

	value = es325_read(NULL, reg);
	ucontrol->value.enumerated.item[0] = value;

	return 0;
}

static int es325_get_rx1_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es325_priv.rx1_route_enable;
	pr_debug("%s(): rx1_route_enable = %d\n", __func__,
		es325_priv.rx1_route_enable);

	return 0;
}

static int es325_put_rx1_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	es325_priv.rx1_route_enable = ucontrol->value.integer.value[0];
	pr_debug("%s(): rx1_route_enable = %d\n", __func__,
		es325_priv.rx1_route_enable);

	return 0;
}

static int es325_get_tx1_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es325_priv.tx1_route_enable;
	pr_debug("%s(): tx1_route_enable = %d\n", __func__,
		es325_priv.tx1_route_enable);

	return 0;
}

static int es325_put_tx1_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	es325_priv.tx1_route_enable = ucontrol->value.integer.value[0];
	pr_debug("%s(): tx1_route_enable = %d\n", __func__,
		es325_priv.tx1_route_enable);

	return 0;
}

static int es325_get_rx2_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es325_priv.rx2_route_enable;
	pr_debug("%s(): rx2_route_enable = %d\n", __func__,
		es325_priv.rx2_route_enable);

	return 0;
}

static int es325_put_rx2_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	es325_priv.rx2_route_enable = ucontrol->value.integer.value[0];
	pr_debug("%s(): rx2_route_enable = %d\n", __func__,
		es325_priv.rx2_route_enable);

	return 0;
}

int es325_remote_route_enable(struct snd_soc_dai *dai)
{
	pr_debug("%s():dai->name = %s dai->id = %d\n", __func__,
		 dai->name, dai->id);

	switch (dai->id) {
	case ES325_SLIM_1_PB:
		return es325_priv.rx1_route_enable;
	case ES325_SLIM_1_CAP:
		return es325_priv.tx1_route_enable;
	case ES325_SLIM_2_PB:
		return es325_priv.rx2_route_enable;
	default:
		return 0;
	}
}
EXPORT_SYMBOL_GPL(es325_remote_route_enable);

static int es325_put_internal_route_config(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	es325_switch_route(ucontrol->value.integer.value[0]);
	return 0;
}

static int es325_get_internal_route_config(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es325_internal_route_num;

	return 0;
}

static int es325_get_audio_custom_profile(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int es325_put_audio_custom_profile(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	int index = ucontrol->value.integer.value[0];

	if (index < ES325_CUSTOMER_PROFILE_MAX)
		es325_write_block(&es325_priv,
				  &es325_audio_custom_profiles[index][0]);
	return 0;
}

static int es325_ap_put_tx1_ch_cnt(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	es325_priv.ap_tx1_ch_cnt = ucontrol->value.enumerated.item[0] + 1;
	return 0;
}

static int es325_ap_get_tx1_ch_cnt(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.enumerated.item[0] = es325_internal_route_num - 1;

	return 0;
}

static const char * const es325_ap_tx1_ch_cnt_texts[] = {
	"One", "Two"
};
static const struct soc_enum es325_ap_tx1_ch_cnt_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0,
			ARRAY_SIZE(es325_ap_tx1_ch_cnt_texts),
			es325_ap_tx1_ch_cnt_texts);

/* generic gain translation */
static int es325_index_to_gain(int min, int step, int index)
{
	return	min + (step * index);
}
static int es325_gain_to_index(int min, int step, int gain)
{
	return	(gain - min) / step;
}

/* dereverb gain */
static int es325_put_dereverb_gain_value(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc = 0;

	if (ucontrol->value.integer.value[0] <= 12) {
		value = es325_index_to_gain(-12, 1, ucontrol->value.integer.value[0]);
		rc = es325_write(NULL, reg, value);
	}

	return rc;
}

static int es325_get_dereverb_gain_value(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;

	value = es325_read(NULL, reg);
	ucontrol->value.integer.value[0] = es325_gain_to_index(-12, 1, value);

	return 0;
}

/* bwe high band gain */
static int es325_put_bwe_high_band_gain_value(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc = 0;

	if (ucontrol->value.integer.value[0] <= 30) {
		value = es325_index_to_gain(-10, 1, ucontrol->value.integer.value[0]);
		rc = es325_write(NULL, reg, value);
	}

	return 0;
}

static int es325_get_bwe_high_band_gain_value(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;

	value = es325_read(NULL, reg);
	ucontrol->value.integer.value[0] = es325_gain_to_index(-10, 1, value);

	return 0;
}

/* bwe max snr */
static int es325_put_bwe_max_snr_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc = 0;

	if (ucontrol->value.integer.value[0] <= 70) {
		value = es325_index_to_gain(-20, 1, ucontrol->value.integer.value[0]);
		rc = es325_write(NULL, reg, value);
	}

	return 0;
}

static int es325_get_bwe_max_snr_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;

	value = es325_read(NULL, reg);
	ucontrol->value.integer.value[0] = es325_gain_to_index(-20, 1, value);

	return 0;
}

static const char * const es325_mic_config_texts[] = {
	"CT 2-mic", "FT 2-mic", "DV 1-mic", "EXT 1-mic", "BT 1-mic",
	"CT ASR 2-mic", "FT ASR 2-mic", "EXT ASR 1-mic", "FT ASR 1-mic",
};
static const struct soc_enum es325_mic_config_enum =
	SOC_ENUM_SINGLE(ES325_MIC_CONFIG, 0,
			ARRAY_SIZE(es325_mic_config_texts),
			es325_mic_config_texts);

static const char * const es325_aec_mode_texts[] = {
	"Off", "On", "rsvrd2", "rsvrd3", "rsvrd4", "On half-duplex"
};
static const struct soc_enum es325_aec_mode_enum =
	SOC_ENUM_SINGLE(ES325_AEC_MODE, 0, ARRAY_SIZE(es325_aec_mode_texts),
			es325_aec_mode_texts);

static const char * const es325_algo_rates_text[] = {
	"fs=8khz", "fs=16khz", "fs=24khz", "fs=48khz", "fs=96khz", "fs=192khz"
};
static const struct soc_enum es325_algo_sample_rate_enum =
	SOC_ENUM_SINGLE(ES325_ALGO_SAMPLE_RATE, 0,
			ARRAY_SIZE(es325_algo_rates_text),
			es325_algo_rates_text);
static const struct soc_enum es325_algo_mix_rate_enum =
	SOC_ENUM_SINGLE(ES325_MIX_SAMPLE_RATE, 0,
			ARRAY_SIZE(es325_algo_rates_text),
			es325_algo_rates_text);

static const char * const es325_algorithms_text[] = {
	"None", "VP", "Two CHREC", "AUDIO", "Four CHPASS"
};
static const struct soc_enum es325_algorithms_enum =
	SOC_ENUM_SINGLE(ES325_ALGO_SAMPLE_RATE, 0,
			ARRAY_SIZE(es325_algorithms_text),
			es325_algorithms_text);

static const char * const es325_off_on_texts[] = {
	"Off", "On"
};
static const struct soc_enum es325_veq_enable_enum =
	SOC_ENUM_SINGLE(ES325_VEQ_ENABLE, 0, ARRAY_SIZE(es325_off_on_texts),
			es325_off_on_texts);
static const struct soc_enum es325_dereverb_enable_enum =
	SOC_ENUM_SINGLE(ES325_DEREVERB_ENABLE, 0,
			ARRAY_SIZE(es325_off_on_texts),
			es325_off_on_texts);
static const struct soc_enum es325_bwe_enable_enum =
	SOC_ENUM_SINGLE(ES325_BWE_ENABLE, 0, ARRAY_SIZE(es325_off_on_texts),
			es325_off_on_texts);
static const struct soc_enum es325_bwe_post_eq_enable_enum =
	SOC_ENUM_SINGLE(ES325_BWE_POST_EQ_ENABLE, 0,
			ARRAY_SIZE(es325_off_on_texts),
			es325_off_on_texts);
static const struct soc_enum es325_algo_processing_enable_enum =
	SOC_ENUM_SINGLE(ES325_ALGO_PROCESSING, 0,
			ARRAY_SIZE(es325_off_on_texts),
			es325_off_on_texts);

static int es325_update_power_state(struct es325_priv *es325, int new_state)
{
	int rc = 0;

	if (new_state == es325->pm_state) {
		pr_info("[%s]: No Power State change\n", __func__);
		return rc;
	}

	switch (es325->pm_state) {
	case ES325_POWER_STATE_NORMAL:
		if (new_state == ES325_POWER_STATE_SLEEP) {
			new_state = ES325_POWER_STATE_SLEEP_PENDING;
			/* Defer Sleep */
			schedule_delayed_work(&es325->sleep_work,
				msecs_to_jiffies(ES325_SLEEP_DELAY));
			break;
		}
	case ES325_POWER_STATE_SLEEP_REQUESTED:

		rc = es325_sleep(es325);
		if (rc < 0) {
			/* Sleep Power State Failed,
			 * setting Active power state in es325->pm_state
			 */
			new_state = ES325_POWER_STATE_NORMAL;
		}
		break;
	case ES325_POWER_STATE_SLEEP_PENDING:
		if (new_state == ES325_POWER_STATE_NORMAL)
			cancel_delayed_work(&es325->sleep_work);

		break;
	case ES325_POWER_STATE_SLEEP:

		es325_wakeup(es325);
		break;
	}
	es325->pm_state = new_state;

	return rc;
}

static void es325_delayed_sleep(struct work_struct *work_req)
{
	struct delayed_work *d_work = to_delayed_work(work_req);
	struct es325_priv *es325 = container_of(d_work, struct es325_priv,
			sleep_work);
	int ch_tot, ports_active;

	mutex_lock(&es325->pm_mutex);
	ports_active = (es325->rx1_route_enable || es325->rx2_route_enable ||
			es325->tx1_route_enable);

	ch_tot = es325->dai[ES325_SLIM_1_PB - 1].ch_tot;
	ch_tot += es325->dai[ES325_SLIM_2_PB - 1].ch_tot;
	ch_tot += es325->dai[ES325_SLIM_1_CAP - 1].ch_tot;

	pr_info("%s %d active channels, ports_active: %d\n", __func__,
				ch_tot, ports_active);
	if ((ch_tot > 0) || (ports_active != 0)) {

		/* There are still active stream which needs to be
		 * freed first.
		 *
		 */

		es325->pm_state = ES325_POWER_STATE_NORMAL;
		pr_info("%s: Active streams, do not SLEEP\n", __func__);

	} else if (es325->pm_state == ES325_POWER_STATE_NORMAL) {

		/* Race between put_power_state() and delayed_sleep()
		 * can cause the power_state to be changed to NORMAL if
		 * put_power_state() wins and acquires the mutex.
		 * Do nothing in that case
		 */

		pr_info("%s: Chip is awake, do not SLEEP\n", __func__);

	} else if (es325->pm_state == ES325_POWER_STATE_SLEEP_PENDING) {

		/* Going to send SLEEP command to chip. It takes 30 msec
		 * to put the chip into sleep. Changing the power_state
		 * in software little bit early
		 */

		es325->pm_state = ES325_POWER_STATE_SLEEP_REQUESTED;
		es325_update_power_state(es325, ES325_POWER_STATE_SLEEP);

	}
	mutex_unlock(&es325->pm_mutex);
}


static int es325_put_power_state_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct es325_priv *es325 = &es325_priv;
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int new_state;
	int rc = 0;

	if (ucontrol->value.enumerated.item[0] >= e->max) {
		pr_err("[%s]:Value out of range\n", __func__);
		return -EINVAL;
	}

	new_state =
		es325_valid_power_states[ucontrol->value.enumerated.item[0]];

	mutex_lock(&es325->pm_mutex);

	rc = es325_update_power_state(es325, new_state);

	mutex_unlock(&es325->pm_mutex);

	return rc;
}

static int es325_get_power_state_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	int value = -EINVAL;
	int i, mapped_state;
	struct es325_priv *es325 = &es325_priv;

	switch (es325->pm_state) {

	case ES325_POWER_STATE_SLEEP_PENDING:
	case ES325_POWER_STATE_SLEEP_REQUESTED:
		mapped_state = ES325_POWER_STATE_SLEEP;
		break;
	default:
		mapped_state = es325->pm_state;
		break;
	}

	for (i = 0; i < sizeof(es325_valid_power_states); i++) {
		if (es325_valid_power_states[i] == mapped_state) {
			value = i;
			break;
		}
	}
	ucontrol->value.enumerated.item[0] = value;

	return (value > 0) ? 0 : value;
}
static const char * const es325_power_state_texts[] = {
	"Sleep", "Active"
};
static const struct soc_enum es325_power_state_enum =
	SOC_ENUM_SINGLE(ES325_POWER_STATE, 0,
			ARRAY_SIZE(es325_power_state_texts),
			es325_power_state_texts);

static struct snd_kcontrol_new es325_digital_ext_snd_controls[] = {
	/* commit controls */
	SOC_SINGLE_EXT("ES325 RX1 Enable", SND_SOC_NOPM, 0, 1, 0,
		       es325_get_rx1_route_enable_value,
		       es325_put_rx1_route_enable_value),
	SOC_SINGLE_EXT("ES325 TX1 Enable", SND_SOC_NOPM, 0, 1, 0,
		       es325_get_tx1_route_enable_value,
		       es325_put_tx1_route_enable_value),
	SOC_SINGLE_EXT("ES325 RX2 Enable", SND_SOC_NOPM, 0, 1, 0,
		       es325_get_rx2_route_enable_value,
		       es325_put_rx2_route_enable_value),
	SOC_ENUM_EXT("Mic Config", es325_mic_config_enum,
		     es325_get_control_enum, es325_put_control_enum),
	SOC_ENUM_EXT("AEC Mode", es325_aec_mode_enum,
		     es325_get_control_enum, es325_put_control_enum),
	SOC_ENUM_EXT("VEQ Enable", es325_veq_enable_enum,
		     es325_get_control_enum, es325_put_control_enum),
	SOC_ENUM_EXT("Dereverb Enable", es325_dereverb_enable_enum,
		     es325_get_control_enum, es325_put_control_enum),
	SOC_SINGLE_EXT("Dereverb Gain",
		       ES325_DEREVERB_GAIN, 0, 100, 0,
		       es325_get_dereverb_gain_value, es325_put_dereverb_gain_value),
	SOC_ENUM_EXT("BWE Enable", es325_bwe_enable_enum,
		     es325_get_control_enum, es325_put_control_enum),
	SOC_SINGLE_EXT("BWE High Band Gain",
		       ES325_BWE_HIGH_BAND_GAIN, 0, 100, 0,
		       es325_get_bwe_high_band_gain_value,
		       es325_put_bwe_high_band_gain_value),
	SOC_SINGLE_EXT("BWE Max SNR",
		       ES325_BWE_MAX_SNR, 0, 100, 0,
		       es325_get_bwe_max_snr_value, es325_put_bwe_max_snr_value),
	SOC_ENUM_EXT("BWE Post EQ Enable", es325_bwe_post_eq_enable_enum,
		     es325_get_control_enum, es325_put_control_enum),
	SOC_SINGLE_EXT("SLIMbus Link Multi Channel",
		       ES325_SLIMBUS_LINK_MULTI_CHANNEL, 0, 65535, 0,
		       es325_get_control_value, es325_put_control_value),
	SOC_ENUM_EXT("Set Power State", es325_power_state_enum,
		       es325_get_power_state_enum, es325_put_power_state_enum),
	SOC_ENUM_EXT("Algorithm Processing", es325_algo_processing_enable_enum,
		     es325_get_control_enum, es325_put_control_enum),
	SOC_ENUM_EXT("Algorithm Sample Rate", es325_algo_sample_rate_enum,
		     es325_get_control_enum, es325_put_control_enum),
	SOC_ENUM_EXT("Algorithm", es325_algorithms_enum,
		     es325_get_control_enum, es325_put_control_enum),
	SOC_ENUM_EXT("Mix Sample Rate", es325_algo_mix_rate_enum,
		     es325_get_control_enum, es325_put_control_enum),
	SOC_SINGLE_EXT("Internal Route Config",
		       SND_SOC_NOPM, 0, 100, 0, es325_get_internal_route_config,
		       es325_put_internal_route_config),
	SOC_SINGLE_EXT("Audio Custom Profile",
		       SND_SOC_NOPM, 0, 100, 0, es325_get_audio_custom_profile,
		       es325_put_audio_custom_profile),
	SOC_ENUM_EXT("ES325-AP Tx Channels", es325_ap_tx1_ch_cnt_enum,
		     es325_ap_get_tx1_ch_cnt, es325_ap_put_tx1_ch_cnt)
};

static int es325_set_bias_level(struct snd_soc_codec *codec,
				enum snd_soc_bias_level level)
{
	int rc = 0;

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		break;

	case SND_SOC_BIAS_STANDBY:
		break;

	case SND_SOC_BIAS_OFF:
		break;
	}
	codec->dapm.bias_level = level;

	return rc;
}

#define ES325_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
			SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
			SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000 |\
			SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_192000)
#define ES325_SLIMBUS_RATES (SNDRV_PCM_RATE_48000)

#define ES325_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE |\
			SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE |\
			SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S32_BE)
#define ES325_SLIMBUS_FORMATS (SNDRV_PCM_FMTBIT_S16_LE |\
			SNDRV_PCM_FMTBIT_S16_BE)

struct snd_soc_dai_driver es325_dai[] = {
	{
		.name = "es325-slim-rx1",
		.id = ES325_SLIM_1_PB,
		.playback = {
			.stream_name = "SLIM_PORT-1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES325_SLIMBUS_RATES,
			.formats = ES325_SLIMBUS_FORMATS,
		},
		.ops = &es325_slim_port_dai_ops,
	},
	{
		.name = "es325-slim-tx1",
		.id = ES325_SLIM_1_CAP,
		.capture = {
			.stream_name = "SLIM_PORT-1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES325_SLIMBUS_RATES,
			.formats = ES325_SLIMBUS_FORMATS,
		},
		.ops = &es325_slim_port_dai_ops,
	},
	{
		.name = "es325-slim-rx2",
		.id = ES325_SLIM_2_PB,
		.playback = {
			.stream_name = "SLIM_PORT-2 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES325_SLIMBUS_RATES,
			.formats = ES325_SLIMBUS_FORMATS,
		},
		.ops = &es325_slim_port_dai_ops,
	},
	{
		.name = "es325-slim-tx2",
		.id = ES325_SLIM_2_CAP,
		.capture = {
			.stream_name = "SLIM_PORT-2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES325_SLIMBUS_RATES,
			.formats = ES325_SLIMBUS_FORMATS,
		},
		.ops = &es325_slim_port_dai_ops,
	},
	{
		.name = "es325-slim-rx3",
		.id = ES325_SLIM_3_PB,
		.playback = {
			.stream_name = "SLIM_PORT-3 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES325_SLIMBUS_RATES,
			.formats = ES325_SLIMBUS_FORMATS,
		},
		.ops = &es325_slim_port_dai_ops,
	},
	{
		.name = "es325-slim-tx3",
		.id = ES325_SLIM_3_CAP,
		.capture = {
			.stream_name = "SLIM_PORT-3 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES325_SLIMBUS_RATES,
			.formats = ES325_SLIMBUS_FORMATS,
		},
		.ops = &es325_slim_port_dai_ops,
	},
	{
		.name = "es325-porta",
		.id = ES325_I2S_PORTA,
		.playback = {
			.stream_name = "PORTA Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES325_RATES,
			.formats = ES325_FORMATS,
		},
		.capture = {
			.stream_name = "PORTA Capture",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES325_RATES,
			.formats = ES325_FORMATS,
		},
		.ops = &es325_i2s_port_dai_ops,
	},
	{
		.name = "es325-portb",
		.id = ES325_I2S_PORTB,
		.playback = {
			.stream_name = "PORTB Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES325_RATES,
			.formats = ES325_FORMATS,
		},
		.capture = {
			.stream_name = "PORTB Capture",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES325_RATES,
			.formats = ES325_FORMATS,
		},
		.ops = &es325_i2s_port_dai_ops,
	},
	{
		.name = "es325-portc",
		.id = ES325_I2S_PORTC,
		.playback = {
			.stream_name = "PORTC Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES325_RATES,
			.formats = ES325_FORMATS,
		},
		.capture = {
			.stream_name = "PORTC Capture",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES325_RATES,
			.formats = ES325_FORMATS,
		},
		.ops = &es325_i2s_port_dai_ops,
	},
	{
		.name = "es325-portd",
		.id = ES325_I2S_PORTD,
		.playback = {
			.stream_name = "PORTD Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES325_RATES,
			.formats = ES325_FORMATS,
		},
		.capture = {
			.stream_name = "PORTD Capture",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES325_RATES,
			.formats = ES325_FORMATS,
		},
		.ops = &es325_i2s_port_dai_ops,
	},
};

#ifdef CONFIG_PM
static int es325_codec_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
	struct es325_priv *es325 = snd_soc_codec_get_drvdata(codec);

	es325_set_bias_level(codec, SND_SOC_BIAS_OFF);

	mutex_lock(&es325->pm_mutex);
	es325_update_power_state(es325,
			ES325_POWER_STATE_SLEEP);
	mutex_unlock(&es325->pm_mutex);

	return 0;
}

static int es325_codec_resume(struct snd_soc_codec *codec)
{
	struct es325_priv *es325 = snd_soc_codec_get_drvdata(codec);

	mutex_lock(&es325->pm_mutex);

	if (es325->pm_state == ES325_POWER_STATE_SLEEP_PENDING) {
		cancel_delayed_work(&es325->sleep_work);
		es325->pm_state = ES325_POWER_STATE_NORMAL;
	} else {
		es325_update_power_state(es325,
				ES325_POWER_STATE_NORMAL);
	}
	mutex_unlock(&es325->pm_mutex);

	es325_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}
#else
#define es325_codec_suspend NULL
#define es325_codec_resume NULL
#endif

int es325_remote_add_codec_controls(struct snd_soc_codec *codec)
{
	int rc;

	dev_dbg(codec->dev, "%s()\n", __func__);

	rc = snd_soc_add_controls(codec, es325_digital_ext_snd_controls,
					ARRAY_SIZE(es325_digital_ext_snd_controls));
	if (rc)
		dev_err(codec->dev, "%s(): es325_digital_ext_snd_controls failed\n", __func__);

	return rc;
}

static int es325_codec_probe(struct snd_soc_codec *codec)
{
	struct es325_priv *es325 = snd_soc_codec_get_drvdata(codec);

	dev_dbg(codec->dev, "%s()\n", __func__);
	es325->codec = codec;

	codec->control_data = snd_soc_codec_get_drvdata(codec);

	es325_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	/*
	rc = snd_soc_add_controls(codec, es325_digital_snd_controls,
					ARRAY_SIZE(es325_digital_snd_controls));
	if (rc)
		dev_err(codec->dev, "%s(): es325_digital_snd_controls failed\n", __func__);
	*/

	return 0;
}

static int  es325_codec_remove(struct snd_soc_codec *codec)
{
	struct es325_priv *es325 = snd_soc_codec_get_drvdata(codec);

	es325_set_bias_level(codec, SND_SOC_BIAS_OFF);

	kfree(es325);

	return 0;
}

struct snd_soc_codec_driver soc_codec_dev_es325 = {
	.probe =	es325_codec_probe,
	.remove =	es325_codec_remove,
	.suspend =	es325_codec_suspend,
	.resume =	es325_codec_resume,
	.read =		es325_read,
	.write =	es325_write,
	.set_bias_level =	es325_set_bias_level,
};

static int register_snd_soc(struct es325_priv *priv)
{
	int rc;
	int i;
	int ch_cnt;
	struct slim_device *sbdev = priv->gen0_client;

	es325_init_slim_slave(sbdev);

	dev_dbg(&sbdev->dev, "%s(): name = %s\n", __func__, sbdev->name);
	rc = snd_soc_register_codec(&sbdev->dev, &soc_codec_dev_es325,
				    es325_dai, ARRAY_SIZE(es325_dai));
	dev_dbg(&sbdev->dev, "%s(): rc = snd_soc_regsiter_codec() = %d\n",
		__func__, rc);

	/* allocate ch_num array for each DAI */
	for (i = 0; i < ARRAY_SIZE(es325_dai); i++) {
		switch (es325_dai[i].id) {
		case ES325_SLIM_1_PB:
		case ES325_SLIM_2_PB:
		case ES325_SLIM_3_PB:
			ch_cnt = es325_dai[i].playback.channels_max;
			break;
		case ES325_SLIM_1_CAP:
		case ES325_SLIM_2_CAP:
		case ES325_SLIM_3_CAP:
			ch_cnt = es325_dai[i].capture.channels_max;
			break;
		default:
			continue;
		}
		es325_priv.dai[i].ch_num =
			kzalloc((ch_cnt * sizeof(unsigned int)), GFP_KERNEL);
	}
	return rc;
}

int es325_core_probe(struct device *dev)
{
	struct esxxx_platform_data *pdata = dev->platform_data;
	int rc = 0;

	if (pdata == NULL) {
		dev_err(dev, "%s(): pdata is NULL", __func__);
		rc = -EIO;
		goto pdata_error;
	}

	es325_priv.dev = dev;
	es325_priv.pdata = pdata;
	if (pdata->esxxx_clk_cb)
		pdata->esxxx_clk_cb(1);

	mutex_init(&es325_priv.api_mutex);
	mutex_init(&es325_priv.pm_mutex);

	es325_priv.pm_state = ES325_POWER_STATE_NORMAL;
	INIT_DELAYED_WORK(&es325_priv.sleep_work, es325_delayed_sleep);

	rc = device_create_file(es325_priv.dev, &dev_attr_route_status);
	if (rc)
		dev_err(es325_priv.dev, "%s(): route_status sysfs create\n",
			__func__);
	rc = device_create_file(es325_priv.dev, &dev_attr_route_config);
	if (rc)
		dev_err(es325_priv.dev, "%s(): route_config sysfs create\n",
			__func__);
	rc = device_create_file(es325_priv.dev, &dev_attr_clock_on);
	if (rc)
		dev_err(es325_priv.dev, "%s(): clock_on sysfs create\n",
			__func__);
	rc = device_create_file(es325_priv.dev, &dev_attr_txhex);
	if (rc)
		dev_err(es325_priv.dev, "%s(): txhex sysfs create\n",
			__func__);
	rc = device_create_file(es325_priv.dev, &dev_attr_fw_version);
	if (rc)
		dev_err(es325_priv.dev, "%s(): fw_version sysfs create\n",
			__func__);

	dev_dbg(es325_priv.dev, "%s(): reset_gpio = %d\n", __func__,
		pdata->reset_gpio);
	if (pdata->reset_gpio != -1) {
		rc = gpio_request(pdata->reset_gpio, "es325_reset");
		if (rc < 0) {
			dev_err(es325_priv.dev, "%s(): es325_reset request failed",
				__func__);
			goto reset_gpio_request_error;
		}
		rc = gpio_direction_output(pdata->reset_gpio, 1);
		if (rc < 0) {
			dev_err(es325_priv.dev, "%s(): es325_reset direction failed",
				__func__);
			goto reset_gpio_direction_error;
		}
		gpio_set_value(pdata->reset_gpio, 1);
	}
	else {
		dev_warn(es325_priv.dev, "%s(): es325_reset undefined\n",
			 __func__);
	}

	dev_dbg(es325_priv.dev, "%s(): wakeup_gpio = %d\n", __func__,
		pdata->wakeup_gpio);

	if (pdata->wakeup_gpio != -1) {
		rc = gpio_request(pdata->wakeup_gpio, "es325_wakeup");
		if (rc < 0) {
			dev_err(es325_priv.dev, "%s(): es325_wakeup request failed",
				__func__);
			goto wakeup_gpio_request_error;
		}
		rc = gpio_direction_output(pdata->wakeup_gpio, 1);
		if (rc < 0) {
			dev_err(es325_priv.dev, "%s(): es325_wakeup direction failed",
				__func__);
			goto wakeup_gpio_direction_error;
		}
	}
	else {
		dev_warn(es325_priv.dev, "%s(): es325_wakeup undefined\n",
			 __func__);
	}

	dev_dbg(es325_priv.dev, "%s(): gpioa_gpio = %d\n", __func__,
		pdata->gpioa_gpio);
	if (pdata->gpioa_gpio != -1) {
		rc = gpio_request(pdata->gpioa_gpio, "es325_gpioa");
		if (rc < 0) {
			dev_err(es325_priv.dev, "%s(): es325_gpioa request failed",
				__func__);
			goto gpioa_gpio_request_error;
		}
		rc = gpio_direction_input(pdata->gpioa_gpio);
		if (rc < 0) {
			dev_err(es325_priv.dev, "%s(): es325_gpioa direction failed",
				__func__);
			goto gpioa_gpio_direction_error;
		}
	}
	else {
		dev_warn(es325_priv.dev, "%s(): es325_gpioa undefined\n",
			 __func__);
	}

	dev_dbg(es325_priv.dev, "%s(): gpiob_gpio = %d\n", __func__,
		pdata->gpiob_gpio);

	if (pdata->gpiob_gpio != -1) {
		rc = gpio_request(pdata->gpiob_gpio, "es325_gpiob");
		if (rc < 0) {
			dev_err(es325_priv.dev, "%s(): es325_gpiob request failed",
				__func__);
			goto gpiob_gpio_request_error;
		}
		rc = gpio_direction_input(pdata->gpiob_gpio);
		if (rc < 0) {
			dev_err(es325_priv.dev, "%s(): es325_gpiob direction failed",
				__func__);
			goto gpiob_gpio_direction_error;
		}
	}
	else {
		dev_warn(es325_priv.dev, "%s(): es325_gpiob undefined\n",
			 __func__);
	}
	
	return rc;

gpiob_gpio_direction_error:
	gpio_free(pdata->gpiob_gpio);
gpiob_gpio_request_error:
gpioa_gpio_direction_error:
	gpio_free(pdata->gpioa_gpio);
gpioa_gpio_request_error:
wakeup_gpio_direction_error:
	gpio_free(pdata->wakeup_gpio);
wakeup_gpio_request_error:
reset_gpio_direction_error:
	gpio_free(pdata->reset_gpio);
reset_gpio_request_error:
pdata_error:
	dev_dbg(es325_priv.dev, "%s(): exit with error\n", __func__);

	return rc;
}
EXPORT_SYMBOL_GPL(es325_core_probe);

static __init int es325_init(void)
{
	int rc = 0;

	pr_debug("%s()", __func__);
#ifdef CONFIG_SND_SOC_ES325_SLIM
	rc = slim_driver_register(&es325_slim_driver);
	if (!rc) {
		pr_debug("%s() registered as SLIMBUS", __func__);
		es325_priv.intf = ES325_SLIM_INTF;
		/*
		   es325_priv.device_read = ;
		   es325_priv.device_write = ;
		 */
	}
#endif
#ifdef CONFIG_SND_SOC_ES325_I2C
	rc = i2c_add_driver(&es325_i2c_driver);
	if (!rc) {
		pr_debug("%s() registered as I2C", __func__);
		es325_priv.intf = ES325_I2C_INTF;
		/*
		   es325_priv.device_read = ;
		   es325_priv.device_write = ;
		 */
	}
#endif

	return rc;
}
module_init(es325_init);

static __exit void es325_exit(void)
{
	pr_debug("%s()\n", __func__);
#if defined(CONFIG_SND_SOC_ES325_I2C)
	i2c_del_driver(&es325_i2c_driver);
#else
	/* no support from QCOM to unregister
	 * slim_driver_unregister(&es325_slim_driver);
	 */
#endif
}
module_exit(es325_exit);


MODULE_DESCRIPTION("ASoC ES325 driver");
MODULE_AUTHOR("Greg Clemson <gclemson@audience.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:es325-codec");
MODULE_FIRMWARE("audience-es325-fw.bin");
