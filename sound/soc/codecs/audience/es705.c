/*
 * es705.c  --  Audience eS705 ALSA SoC Audio driver
 *
 * Copyright 2011 Audience, Inc.
 *
 * Author: Greg Clemson <gclemson@audience.com>
 *
 * Code Updates:
 *       Genisim Tsilker <gtsilker@audience.com>
 *            - Code refactoring
 *            - FW download functions update
 *            - Add optional UART VS FW download
 *            - Add external Clock support
 *            - Add 3d party support for RDB / WDB features
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG
#define SAMSUNG_ES705_FEATURE

/*
 * Uncomment WDB_RDB_OVER_SLIMBUS
 * to enable WDB / RDB operations over SLIMBus
 */
/*#define WDB_RDB_OVER_SLIMBUS*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/completion.h>
#include <linux/i2c.h>
#include <linux/slimbus/slimbus.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/version.h>
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
#include <linux/spi/spi.h>
#include <linux/esxxx.h>
#include <linux/wait.h>
#include "es705.h"
#include "es705-platform.h"
#include "es705-routes.h"
#include "es705-profiles.h"
#include "es705-slim.h"
#include "es705-i2c.h"
#include "es705-i2s.h"
#include "es705-spi.h"
#include "es705-cdev.h"
#include "es705-uart.h"
#include "es705-uart-common.h"
#include "es705-veq-params.h"

#define ES705_CMD_ACCESS_WR_MAX 2
#define ES705_CMD_ACCESS_RD_MAX 2
struct es705_api_access {
	u32 read_msg[ES705_CMD_ACCESS_RD_MAX];
	unsigned int read_msg_len;
	u32 write_msg[ES705_CMD_ACCESS_WR_MAX];
	unsigned int write_msg_len;
	unsigned int val_shift;
	unsigned int val_max;
};

#include "es705-access.h"

#define ENABLE_VS_DATA_DUMP
#define NARROW_BAND	0
#define WIDE_BAND	1
#define NETWORK_OFFSET	21
static int network_type = NARROW_BAND;

#if defined(FORCED_REROUTE_PRESET)
static int extra_vol_onoff = 0;
#endif

/* Route state for Internal state management */
enum es705_power_state {
ES705_POWER_FW_LOAD,
ES705_POWER_SLEEP,
ES705_POWER_SLEEP_PENDING,
ES705_POWER_AWAKE
};

static const char *power_state[] = {
	"boot",
	"sleep",
	"sleep pending",
	"awake",
};

static const char *power_state_cmd[] = {
	"not defined",
	"sleep",
	"mp_sleep",
	"mp_cmd",
	"normal",
	"vs_overlay",
	"vs_lowpwr",
};

/* codec private data TODO: move to runtime init */
struct es705_priv es705_priv = {
	.pm_state = ES705_POWER_AWAKE,

	.rx1_route_enable = 0,
	.tx1_route_enable = 0,
	.rx2_route_enable = 0,

	.vs_enable = 0,
	.vs_wakeup_keyword = 0,

	.ap_tx1_ch_cnt = 2,

	.es705_power_state = ES705_SET_POWER_STATE_NORMAL,
	.streamdev.intf = -1,
	.ns = 1,

	.wakeup_method = 0,

#if defined(SAMSUNG_ES705_FEATURE)
	.voice_wakeup_enable = 0,
	.voice_lpm_enable = 0,
	/* gpio wakeup : 0, uart wakeup : 1 */
	.use_uart_for_wakeup_gpio = 0,
	/* for tuning : 1 */
	.change_uart_config = 0,
	.internal_route_num = 5,
#endif
};

const char *esxxx_mode[] = {
	"SBL",
	"STANDARD",
	"VOICESENSE",
};

struct snd_soc_dai_driver es705_dai[];

static struct file *file_open(const char *path, int flags, int rights)
{
	mm_segment_t oldfs;
	int err = 0;
	struct file *filp = NULL;
	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, rights);
	set_fs(oldfs);
	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		dev_err(es705_priv.dev, "%s:Error (%d) Opening file %s",
				__func__, err, path);
		return NULL;
	}
	return filp;
}

static inline void file_close(struct file *file)
{
	filp_close(file, NULL);
}

static int file_read(struct file *file, unsigned long long offset,
		unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;
	oldfs = get_fs();
	set_fs(get_ds());
	ret = vfs_read(file, data, size, &offset);
	set_fs(oldfs);
	return ret;
}

static int abort_request = 0;
static int es705_vs_load(struct es705_priv *es705);
static int es705_write_then_read(struct es705_priv *es705,
				const void *buf, int len,
				u32 *rspn, int match);

#if defined(PREVENT_CALL_MUTE_WHEN_SWITCH_NB_AND_WB)
static void es705_restore_bwe_veq(void);
#endif

/* indexed by ES705 INTF number */
u32 es705_streaming_cmds[4] = {
	0x90250200,		/* ES705_SLIM_INTF */
	0x90250000,		/* ES705_I2C_INTF  */
	0x90250300,		/* ES705_SPI_INTF  */
	0x90250100,		/* ES705_UART_INTF */
};

#define SAMSUNG_ES70X_RESTORE_FW_IN_SLEEP
#define SAMSUNG_ES70X_RESTORE_FW_IN_WAKEUP

#if defined(SAMSUNG_ES70X_RESTORE_FW_IN_SLEEP) || defined(SAMSUNG_ES70X_RESTORE_FW_IN_WAKEUP)
static int cnt_restore_std_fw_in_sleep = 0;
static int cnt_restore_std_fw_in_wakeup = 0;

int restore_std_fw(struct es705_priv *es705)
{
	int rc;
	dev_info(es705->dev, "%s(): START!\n", __func__);

	es705->mode = SBL;
	es705_gpio_reset(es705);
	rc = es705_bootup(es705);

	dev_info(es705->dev, "%s(): rc = %d\n", __func__, rc);
	return rc;
}
#endif
#define SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY
#ifdef SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY
static int preset_delay_time = 5;
#endif

int es705_write_block(struct es705_priv *es705, const u32 *cmd_block)
{
	u32 api_word;
	u8 msg[4];
	int rc = 0;

	mutex_lock(&es705->api_mutex);
	while (*cmd_block != 0xffffffff) {
		api_word = cpu_to_le32(*cmd_block);
		memcpy(msg, (char *)&api_word, 4);
		es705->dev_write(es705, msg, 4);
#ifndef SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY
		usleep_range(1000, 1000);
#else /* SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY */
		if (msg[3] == 0x90 && msg[2] == 0x31)
			usleep_range(preset_delay_time*1000, preset_delay_time*1000);
		else
			usleep_range(1000, 1000);
#endif /* SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY */
		dev_dbg(es705->dev, "%s(): msg = 0x%02x%02x%02x%02x\n",
			__func__, msg[3], msg[2], msg[1], msg[0]);
		cmd_block++;
	}
	mutex_unlock(&es705->api_mutex);

	return rc;
}

/* Note: this may only end up being called in a api locked context. In
 * that case the mutex locks need to be removed.
 */
#define WDB_RDB_BLOCK_MAX_SIZE	512
int es705_read_vs_data_block(struct es705_priv *es705, char *dump_data,
			     unsigned max_size)
{
	/* This function is not re-entrant so avoid stack bloat. */
	u32 cmd = 0x802e0008;
	u32 resp;
	int ret;
	unsigned size;

	es705->vs_keyword_param_size = 0;
	mutex_lock(&es705->api_mutex);
	if (es705->rdb_wdb_open) {
		ret = es705->rdb_wdb_open(es705);
		if (ret < 0) {
			dev_err(es705->dev, "%s(): RDB open fail\n",
				__func__);
			goto rdb_open_error;
		}
	}

	if (es705->rdb_wdb_open)
		cmd = cpu_to_be32(cmd); /* over UART */
	else
		cmd = cpu_to_le32(cmd); /* over SLIMBus */

	es705->wdb_write(es705, (char *)&cmd, 4);
	ret = es705->rdb_read(es705, (char *)&resp, 4);
	if (ret < 0) {
		dev_dbg(es705->dev, "%s(): error sending request = %d\n",
			__func__, ret);
		goto OUT;
	}

	if (es705->rdb_wdb_open)
		resp = be32_to_cpu(resp); /* over UART */
	else
		resp = le32_to_cpu(resp); /* over SLIMBus */

	if ((resp & 0xffff0000) != 0x802e0000) {
		dev_err(es705->dev, "%s(): invalid read v-s data block size = 0x%08x\n",
			__func__, resp);
		goto OUT;
	}
	size = resp & 0xffff;
	dev_dbg(es705->dev, "%s(): resp=0x%08x size=%d\n", __func__, resp, size);
	BUG_ON(size == 0);
	BUG_ON(size > max_size);

	/*
	 * This assumes we need to transfer the block in 4 byte
	 * increments. This is true on slimbus, but may not hold true
	 * for other buses.
	 */
	while (es705->vs_keyword_param_size < size) {
		if (es705->rdb_wdb_open)
			/* over UART */
			ret = es705->rdb_read(es705, dump_data +
					es705->vs_keyword_param_size,
						size - es705->vs_keyword_param_size);
		else
			/* over SLIMBus */
			ret = es705->rdb_read(es705, dump_data +
					es705->vs_keyword_param_size,
						4);
		if (ret < 0) {
			dev_dbg(es705->dev, "%s(): error reading data block\n",
				__func__);
			es705->vs_keyword_param_size = 0;
			goto OUT;
		}
		es705->vs_keyword_param_size += ret;
		dev_dbg(es705->dev, "%s(): stored v-s read and saved of %d bytes\n",
			__func__, ret);
	}
	dev_dbg(es705->dev, "%s(): stored v-s keyword block of %d bytes\n",
		__func__, es705->vs_keyword_param_size);

OUT:
	if (es705->rdb_wdb_close)
		es705->rdb_wdb_close(es705);
rdb_open_error:
	mutex_unlock(&es705->api_mutex);
	if (ret < 0)
		dev_err(es705->dev, "%s(): v-s read data block failure=%d\n",
			__func__, ret);
	return ret;
}

int es705_write_vs_data_block(struct es705_priv *es705)
{
	u32 cmd;
	u32 resp;
	int ret = 0;
	u8 *dptr;
	u16 rem;
	u8 wdb[4];
	int end_data = 0;
	int sz;
	int blocks;

	if (es705->vs_keyword_param_size == 0) {
		dev_warn(es705->dev, "%s(): attempt to write empty keyword data block\n",
			__func__);
		return -ENOENT;
	}

	BUG_ON(es705->vs_keyword_param_size % 4 != 0);

	mutex_lock(&es705->api_mutex);
	/* Add code to support UART WDB feature */
	/* Get 512 blocks number */
	blocks = es705->vs_keyword_param_size / WDB_RDB_BLOCK_MAX_SIZE;
	sz = WDB_RDB_BLOCK_MAX_SIZE;
SEND_DATA:
	if (blocks) {
		cmd = 0x802f0000 | (sz & 0xffff);
		if (es705->rdb_wdb_open)
			cmd = cpu_to_be32(cmd);
		else
			cmd = cpu_to_le32(cmd);
	
		ret = es705->wdb_write(es705, (char *)&cmd, 4);
		if (ret < 0) {
			dev_err(es705->dev, "%s(): error writing cmd 0x%08x to device\n",
		    	__func__, cmd);
			goto EXIT;
		}
	
		ret = es705->rdb_read(es705, (char *)&resp, 4);
		if (ret < 0) {
			dev_dbg(es705->dev, "%s(): error sending request = %d\n",
				__func__, ret);
			goto EXIT;
		}

		if (es705->rdb_wdb_open)
			resp = be32_to_cpu(resp);
		else
			resp = le32_to_cpu(resp);
		dev_dbg(es705->dev, "%s(): resp=0x%08x\n", __func__, resp);
		if ((resp & 0xffff0000) != 0x802f0000) {
			dev_err(es705->dev, "%s(): invalid write data block 0x%08x\n",
				__func__, resp);
			goto EXIT;
		}

		if (es705->rdb_wdb_open) {
			/* send data over UART */
			do {
				ret = es705->wdb_write(es705,
					(char *)es705->vs_keyword_param, sz);
				if (ret < 0) {
					dev_err(es705->dev, "%s(): v-s wdb UART write error\n",
						__func__);
					goto EXIT;
				}
				dev_dbg(es705->dev, "%s(): v-s wdb %d bytes written\n",
					__func__, sz);
				
				resp = 0;
				ret = es705->rdb_read(es705, (char *)&resp, 4);
				if (ret < 0) {
					dev_err(es705->dev, "%s(): WDB ACK request fFAIL\n",
						__func__);
					goto EXIT;
				}
				resp = be32_to_cpu(resp);
				dev_dbg(es705->dev, "%s(): resp=0x%08x\n", __func__, resp);
				if (resp & 0xFFFF) {
					dev_err(es705->dev, "%s(): write WDB FAIL. ACK=0x%08x\n",
						__func__, resp);
					goto EXIT;
				}
			} while (--blocks);
		} else {
			/* send data over SLIMBus */
			dptr = es705->vs_keyword_param;
			for (rem = es705->vs_keyword_param_size; rem > 0; rem -= 4, dptr += 4) {
				wdb[0] = dptr[3];
				wdb[1] = dptr[2];
				wdb[2] = dptr[1];
				wdb[3] = dptr[0];
				ret = es705->wdb_write(es705, (char *)wdb, 4);
				if (ret < 0) {
					dev_err(es705->dev, "%s(): v-s WDB write over SLIMBus FAIL, offset=%hu\n",
						__func__, dptr - es705->vs_keyword_param);
					goto EXIT;
				}
				resp = 0;
				ret = es705->rdb_read(es705, (char *)&resp, 4);
				if (ret < 0) {
					dev_err(es705->dev, "%s(): WDB ACK request FAIL\n",
						__func__);
					goto EXIT;
				}
				resp = le32_to_cpu(resp);
				dev_dbg(es705->dev, "%s(): resp=0x%08x\n",
					__func__, resp);
				if (resp & 0xFFFF) {
					dev_err(es705->dev, "%s(): write WDB FAIL. ACK=0x%08x\n",
						__func__, resp);
					goto EXIT;
				}
			}
		}
	}
	if (!end_data) {
		sz = es705->vs_keyword_param_size -
				(es705->vs_keyword_param_size / WDB_RDB_BLOCK_MAX_SIZE) *
					WDB_RDB_BLOCK_MAX_SIZE;
		if (sz) {
			blocks = 1;
			end_data = 1;
			goto SEND_DATA;
		}
	}
	dev_dbg(es705->dev, "%s(): v-s wdb success\n", __func__);
EXIT:
	mutex_unlock(&es705->api_mutex);
	if (ret < 0)
		dev_err(es705->dev, "%s(): v-s wdb failed ret=%d\n",
			__func__, ret);
	return ret;
}


#ifdef FIXED_CONFIG
static void es705_fixed_config(struct es705_priv *es705)
{
	int rc;

	rc = es705_write_block(es705, es705_route_config[ROUTE_OFF].route);
}
#endif

static void es705_switch_route(long route_index)
{
	struct es705_priv *es705 = &es705_priv;
	int rc;

	if (route_index >= ROUTE_MAX) {
		dev_dbg(es705->dev, "%s(): new es705_internal_route = %ld is out of range\n",
			 __func__, route_index);
		return;
	}

	dev_dbg(es705->dev, "%s(): switch current es705_internal_route = %ld to new route = %ld\n",
		__func__, es705->internal_route_num, route_index);
	es705->internal_route_num = route_index;
	rc = es705_write_block(es705,
			  es705_route_config[es705->internal_route_num].route);
}

static void es705_switch_route_config(long route_index)
{
	struct es705_priv *es705 = &es705_priv;
	int rc;
	long route_config_idx = route_index; 

	if (!(route_index < ARRAY_SIZE(es705_route_configs))) {
		dev_err(es705->dev, 
				"%s(): new es705_internal_route = %ld is out of range\n",
			 __func__, route_index);
		return;
	}

	es705->current_bwe = 0;
	/* default veq on in the handset mode */
	if ((route_config_idx == 2) || (route_config_idx == 4)) {
		es705->current_veq_preset = 1;
		es705->current_veq = -1;
	}
	else
		es705->current_veq_preset = 0;

#if defined(PREVENT_CALL_MUTE_WHEN_SWITCH_NB_AND_WB)
	if (delayed_work_pending(&es705->reroute_work))
		cancel_delayed_work_sync(&es705->reroute_work);
#endif

	if (network_type == WIDE_BAND) {
		if (route_config_idx >= 0 &&
			route_config_idx < 5) {
				route_config_idx += NETWORK_OFFSET;
				dev_dbg(es705->dev,
					"%s() adjust wideband offset\n", __func__);
#if defined(PREVENT_CALL_MUTE_WHEN_SWITCH_NB_AND_WB)
				schedule_delayed_work(&es705->reroute_work,
								msecs_to_jiffies(es705->reroute_delay));
#endif
		}
	}
	else if (network_type == NARROW_BAND) {
		if (route_config_idx >= 0 + NETWORK_OFFSET &&
			route_config_idx < 5 + NETWORK_OFFSET) {
				route_config_idx -= NETWORK_OFFSET;
				dev_dbg(es705->dev,
					"%s() adjust narrowband offset\n", __func__);
#if defined(PREVENT_CALL_MUTE_WHEN_SWITCH_NB_AND_WB)
				schedule_delayed_work(&es705->reroute_work,
								msecs_to_jiffies(es705->reroute_delay));
#endif
		}
	}

	dev_info(es705->dev,
			"%s(): converts route_index = %ld to %ld and old num is %ld\n",
			__func__, route_index, route_config_idx, es705->internal_route_num);

	if (es705->internal_route_num != route_config_idx) {
		es705->internal_route_num = route_config_idx;
		rc = es705_write_block(es705, &es705_route_configs[route_config_idx][0]);
	}
}

/* Send a single command to the chip.
 *
 * If the SR (suppress response bit) is NOT set, will read the
 * response and cache it the driver object retrieve with es705_resp().
 *
 * Returns:
 * 0 - on success.
 * EITIMEDOUT - if the chip did not respond in within the expected time.
 * E* - any value that can be returned by the underlying HAL.
 */
int es705_cmd(struct es705_priv *es705, u32 cmd)
{
	int sr;
	int err;
	u32 resp;

	BUG_ON(!es705);
	sr = cmd & BIT(28);

	err = es705->cmd(es705, cmd, sr, &resp);
	if (err || sr)
		return err;

	if (resp == 0) {
		err = -ETIMEDOUT;
		dev_err(es705->dev, "%s(): no response to command 0x%08x\n",
			__func__, cmd);
	} else {
		es705->last_response = resp;
		get_monotonic_boottime(&es705->last_resp_time);
	}
	return err;
}

static void es705_switch_to_normal_mode(struct es705_priv *es705)
{
	int rc;
	int match = 1;
	u32 cmd = (ES705_SET_POWER_STATE << 16) | ES705_SET_POWER_STATE_NORMAL;
	u32 sync_cmd = (ES705_SYNC_CMD << 16) | ES705_SYNC_POLLING;
	u32 sync_rspn = sync_cmd;

	es705_cmd(es705, cmd);
	msleep(20);
	es705->pm_state = ES705_POWER_AWAKE;

	rc = es705_write_then_read(es705, &sync_cmd,
				sizeof(sync_cmd), &sync_rspn, match);
	if (rc)
		dev_err(es705->dev, "%s(): es705 Sync FAIL\n", __func__);
}

static int es705_switch_to_vs_mode(struct es705_priv *es705)
{
	int rc = 0;
	u32 cmd = (ES705_SET_POWER_STATE << 16) |
			ES705_SET_POWER_STATE_VS_OVERLAY;

	if (!abort_request) {
		rc = es705_cmd(es705, cmd);
		if (rc < 0) {
			dev_err(es705->dev, "%s(): Set VS SBL Fail", __func__);
		} else {
			msleep(20);
			dev_dbg(es705->dev, "%s(): VS Overlay Mode", __func__);
			rc = es705_vs_load(es705);
		}
	} else {
		es705->es705_power_state = ES705_SET_POWER_STATE_NORMAL;
		dev_info(es705->dev, "%s(): Skip to switch to vs mode", __func__);
	}
	return rc;
}

static unsigned int es705_read(struct snd_soc_codec *codec,
			       unsigned int reg)
{
	struct es705_priv *es705 = &es705_priv;
	struct es705_api_access *api_access;
	u32 api_word[2] = {0};
	char req_msg[8];
	u32 ack_msg;
	char *msg_ptr;
	unsigned int msg_len;
	unsigned int value;
	int match = 0;
	int rc;

	if (reg >= ES705_API_ADDR_MAX) {
		dev_err(es705->dev, "%s(): invalid address = 0x%04x\n",
			__func__, reg);
		return -EINVAL;
	}

	api_access = &es705_api_access[reg];
	msg_len = api_access->read_msg_len;
	memcpy((char *)api_word, (char *)api_access->read_msg, msg_len);
	switch (msg_len) {
	case 8:
		cpu_to_le32s(&api_word[1]);
	case 4:
		cpu_to_le32s(&api_word[0]);
	}
	memcpy(req_msg, (char *)api_word, msg_len);

	msg_ptr = req_msg;
	mutex_lock(&es705->api_mutex);
	rc = es705->dev_write_then_read(es705, msg_ptr, msg_len,
					&ack_msg, match);
	mutex_unlock(&es705->api_mutex);
	if (rc < 0) {
		dev_err(es705->dev, "%s(): es705_xxxx_write()", __func__);
		return rc;
	}
	memcpy((char *)&api_word[0], &ack_msg, 4);
	le32_to_cpus(&api_word[0]);
	value = api_word[0] & 0xffff;
	return value;
}

static int es705_write(struct snd_soc_codec *codec, unsigned int reg,
		       unsigned int value)
{
	struct es705_priv *es705 = &es705_priv;
	struct es705_api_access *api_access;
	u32 api_word[2] = {0};
	char msg[8];
	char *msg_ptr;
	int msg_len;
	unsigned int val_mask;
	int rc = 0;

	if (reg >= ES705_API_ADDR_MAX) {
		dev_err(es705->dev, "%s(): invalid address = 0x%04x\n",
			__func__, reg);
		return -EINVAL;
	}

	api_access = &es705_api_access[reg];
	msg_len = api_access->write_msg_len;
	val_mask = (1 << get_bitmask_order(api_access->val_max)) - 1;
	dev_info(es705->dev, "%s(): reg=%d val=%d msg_len = %d val_mask = 0x%08x\n",
		__func__, reg, value, msg_len, val_mask);
	memcpy((char *)api_word, (char *)api_access->write_msg, msg_len);
	switch (msg_len) {
	case 8:
		api_word[1] |= (val_mask & value);
		cpu_to_le32s(&api_word[1]);
		cpu_to_le32s(&api_word[0]);
		break;
	case 4:
		api_word[0] |= (val_mask & value);
		cpu_to_le32s(&api_word[0]);
		break;
	}
	memcpy(msg, (char *)api_word, msg_len);

	msg_ptr = msg;
	mutex_lock(&es705->api_mutex);
	rc = es705->dev_write(es705, msg_ptr, msg_len);
	mutex_unlock(&es705->api_mutex);
	return rc;
}

static int es705_write_then_read(struct es705_priv *es705,
				const void *buf, int len,
				u32 *rspn, int match)
{
	int rc;
	rc = es705->dev_write_then_read(es705, buf, len, rspn, match);
	return rc;
}

#if defined(SAMSUNG_ES705_FEATURE)
#define POLY 0x8408
#define BLOCK_PAYLOAD_SIZE 508
static unsigned short crc_update(char *buf, int length, unsigned short crc)
{
	unsigned char i;
	unsigned short data;

	while (length--) {
		data = (unsigned short)(*buf++) & 0xff;
		for (i = 0; i < 8; i++) {
			if ((crc & 1) ^ (data & 1))
				crc = (crc >> 1) ^ POLY;
			else
				crc >>= 1;
			data >>= 1;
		}
	}
	return (crc);

}

static void es705_write_sensory_vs_data_block(int type)
{
	int size, size4, i, rc = 0;
	const u8 *data;
	char *buf = NULL;
	unsigned char preamble[4];
	u32 cmd[] = {0x9017e021,0x90180001, 0xffffffff};
	u32 cmd_confirm[] = {0x9017e002, 0x90180001, 0xffffffff};
	unsigned short crc;
	u32 check_cmd;
	u32 check_rspn = 0;

	/* check the type (0 = grammar, 1 = net) */
	if (!type) {
		size = es705_priv.vs_grammar->size;
		data = es705_priv.vs_grammar->data;
	}
	else {
		size = es705_priv.vs_net->size;
		data = es705_priv.vs_net->data;
		cmd[1] = 0x90180002;
		cmd_confirm[0] = 0x9017e003;
	}

	/* rdb/wdb mode = 1 for the grammar file */
	rc = es705_write_block(&es705_priv, cmd);

	/* Add packet data and CRC and then download */
	buf = kzalloc((size + 2 + 3), GFP_KERNEL);
	if (!buf) {
		dev_err(es705_priv.dev, "%s(): kzalloc fail\n", __func__);
		return;
	}
	memcpy(buf, data, size);

	size4 = size + 2;
	size4 = ((size4 + 3) >> 2) << 2;
	size4 -= 2;

	while (size < size4)
		buf[size++] = 0;

	crc = crc_update(buf, size, 0xffff);
	buf[size++] = (unsigned char)(crc & 0xff);
	crc >>= 8;
	buf[size++] = (unsigned char)(crc & 0xff);

	for (i = 0; i < size; i += BLOCK_PAYLOAD_SIZE) {
		es705_priv.vs_keyword_param_size  = size - i;
		if (es705_priv.vs_keyword_param_size > BLOCK_PAYLOAD_SIZE)
			es705_priv.vs_keyword_param_size = BLOCK_PAYLOAD_SIZE;

		preamble[0] = 1;
		preamble[1] = 8;
		preamble[2] = 0;
		preamble[3] = (i == 0) ? 0 : 1;
		memcpy(es705_priv.vs_keyword_param, preamble, 4);
		memcpy(&es705_priv.vs_keyword_param[4], &buf[i], es705_priv.vs_keyword_param_size);
		es705_priv.vs_keyword_param_size += 4;
		es705_write_vs_data_block(&es705_priv);
		memset(es705_priv.vs_keyword_param, 0, ES705_VS_KEYWORD_PARAM_MAX);
	}
	kfree(buf);

	/* verify the download count and the CRC */
	check_cmd = 0x8016e031;
	check_rspn = 0;
	rc = es705_write_then_read(&es705_priv, &check_cmd, sizeof(check_cmd),
					 &check_rspn, 0);
	pr_info("%s: size = %x\n", __func__, check_rspn);

	check_cmd = 0x8016e02a;
	check_rspn = 0x80160000;
	rc = es705_write_then_read(&es705_priv, &check_cmd, sizeof(check_cmd),
					 &check_rspn, 1);
	if (rc)
		dev_err(es705_priv.dev, "%s(): es705 CRC check fail\n", __func__);

}

static int es705_write_sensory_vs_keyword(void)
{
	int rc = 0;
	u32 grammar_confirm[] = {0x9017e002, 0x90180001, 0xffffffff};
	u32 net_confirm[] = {0x9017e003, 0x90180001, 0xffffffff};
	u32 start_confirm[] = {0x9017e000, 0x90180001, 0xffffffff};

	dev_info(es705_priv.dev, "%s(): ********* START VS Keyword Download\n", __func__);

	if (abort_request) {
		dev_info(es705_priv.dev, "%s(): Skip to download VS Keyword\n", __func__);
		es705_switch_to_normal_mode(&es705_priv);
		return rc;
	}

	if (es705_priv.rdb_wdb_open) {
		rc = es705_priv.rdb_wdb_open(&es705_priv);
		if (rc < 0) {
			dev_err(es705_priv.dev, "%s(): WDB UART open FAIL\n", __func__);
			return rc;
		}
	}

	/* download keyword using WDB */
	es705_write_sensory_vs_data_block(0);
	es705_write_sensory_vs_data_block(1);

	if (es705_priv.rdb_wdb_close)
		es705_priv.rdb_wdb_close(&es705_priv);

	/* mark the grammar and net as valid */
	rc = es705_write_block(&es705_priv, grammar_confirm);
	rc = es705_write_block(&es705_priv, net_confirm);
	rc = es705_write_block(&es705_priv, start_confirm);
	dev_info(es705_priv.dev, "%s(): ********* END VS Keyword Download\n", __func__);

	return rc;
}
#endif

#if defined(PREVENT_CALL_MUTE_WHEN_SWITCH_NB_AND_WB)
static void es705_reroute(struct work_struct *w)
{
	struct es705_priv *es705 = &es705_priv;
	int rc;
	unsigned int value = 0;

	value = es705_read(NULL, ES705_CHANGE_STATUS);

	if (value) {
		dev_err(es705->dev,
				"%s(): route_status : 0x%04x, reroute to %ld\n",
				__func__, value, es705->internal_route_num);
		rc = es705_write_block(es705,
							&es705_route_configs[es705->internal_route_num][0]);
		es705_restore_bwe_veq();
	}
}
#endif

static ssize_t es705_route_status_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	int ret = 0;
	unsigned int value = 0;
	char *status_name = "Route Status";

	value = es705_read(NULL, ES705_CHANGE_STATUS);

	ret = snprintf(buf, PAGE_SIZE,
		       "%s=0x%04x\n",
		       status_name, value);

	return ret;
}

static DEVICE_ATTR(route_status, 0444, es705_route_status_show, NULL);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/route_status */

static ssize_t es705_route_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct es705_priv *es705 = &es705_priv;

	dev_dbg(es705->dev, "%s(): route=%ld\n",
		__func__, es705->internal_route_num);
	return snprintf(buf, PAGE_SIZE, "route=%ld\n",
			es705->internal_route_num);
}

static DEVICE_ATTR(route, 0444, es705_route_show, NULL);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/route */

static ssize_t es705_rate_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct es705_priv *es705 = &es705_priv;

	dev_dbg(es705->dev, "%s(): rate=%ld\n", __func__, es705->internal_rate);
	return snprintf(buf, PAGE_SIZE, "rate=%ld\n",
			es705->internal_rate);
}

static DEVICE_ATTR(rate, 0444, es705_rate_show, NULL);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/rate */

#define SIZE_OF_VERBUF 256
/* TODO: fix for new read/write. use es705_read() instead of BUS ops */
static ssize_t es705_fw_version_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int idx = 0;
	unsigned int value;
	char versionbuffer[SIZE_OF_VERBUF];
	char *verbuf = versionbuffer;

	memset(verbuf, 0, SIZE_OF_VERBUF);

	if (es705_priv.pm_state == ES705_POWER_AWAKE) {
		value = es705_read(NULL, ES705_FW_FIRST_CHAR);
		*verbuf++ = (value & 0x00ff);
		for (idx = 0; idx < (SIZE_OF_VERBUF-2); idx++) {
			value = es705_read(NULL, ES705_FW_NEXT_CHAR);
			*verbuf++ = (value & 0x00ff);
		}
		/* Null terminate the string*/
		*verbuf = '\0';
		dev_info(dev, "Audience fw ver %s\n", versionbuffer);
	} else
		dev_info(dev, "Audience is not awake\n");
	return snprintf(buf, PAGE_SIZE, "FW Version = %s\n", versionbuffer);
}

static DEVICE_ATTR(fw_version, 0444, es705_fw_version_show, NULL);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/fw_version */

static ssize_t es705_clock_on_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int ret = 0;

	return ret;
}

static DEVICE_ATTR(clock_on, 0444, es705_clock_on_show, NULL);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/clock_on */

static ssize_t es705_vs_keyword_parameters_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	int ret = 0;

	if (es705_priv.vs_keyword_param_size > 0) {
		memcpy(buf, es705_priv.vs_keyword_param,
		       es705_priv.vs_keyword_param_size);
		ret = es705_priv.vs_keyword_param_size;
		dev_dbg(dev, "%s(): keyword param size=%hu\n", __func__, ret);
	} else {
		dev_dbg(dev, "%s(): keyword param not set\n", __func__);
	}

	return ret;
}

static ssize_t es705_vs_keyword_parameters_set(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t count)
{
	int ret = 0;

	if (count <= ES705_VS_KEYWORD_PARAM_MAX) {
		memcpy(es705_priv.vs_keyword_param, buf, count);
		es705_priv.vs_keyword_param_size = count;
		dev_dbg(dev, "%s(): keyword param block set size = %zi\n",
			 __func__, count);
		ret = count;
	} else {
		dev_dbg(dev, "%s(): keyword param block too big = %zi\n",
			 __func__, count);
		ret = -ENOMEM;
	}

	return ret;
}

static DEVICE_ATTR(vs_keyword_parameters, 0644,
		   es705_vs_keyword_parameters_show,
		   es705_vs_keyword_parameters_set);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/vs_keyword_parameters */

static ssize_t es705_vs_status_show(struct device *dev,
			            struct device_attribute *attr,
				    char *buf)
{
	int ret = 0;
	unsigned int value = 0;
	char *status_name = "Voice Sense Status";
	/* Disable vs status read for interrupt to work */
	struct es705_priv *es705 = &es705_priv;

	mutex_lock(&es705->api_mutex);
	value = es705->vs_get_event;
	/* Reset the detection status after read */
	es705->vs_get_event = NO_EVENT;
	mutex_unlock(&es705->api_mutex);

	ret = snprintf(buf, PAGE_SIZE,
		       "%s=0x%04x\n",
		       status_name, value);

	return ret;
}

static DEVICE_ATTR(vs_status, 0444, es705_vs_status_show, NULL);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/vs_status */

static ssize_t es705_ping_status_show(struct device *dev,
			            struct device_attribute *attr,
				    char *buf)
{
	struct es705_priv *es705 = &es705_priv;
	int rc = 0;
	u32 sync_cmd = (ES705_SYNC_CMD << 16) | ES705_SYNC_POLLING;
	u32 sync_ack;
	char msg[4];
	char *status_name = "Ping";

	cpu_to_le32s(&sync_cmd);
	memcpy(msg, (char *)&sync_cmd, 4);
	rc = es705->dev_write(es705, msg, 4);
	if (rc < 0) {
		dev_err(es705->dev, "%s(): firmware load failed sync write\n",
			__func__);
	}
	msleep(20);
	memset(msg, 0, 4);
	rc = es705->dev_read(es705, msg, 4);
	if (rc < 0) {
		dev_err(es705->dev, "%s(): error reading sync ack rc=%d\n",
		       __func__, rc);
	}
	memcpy((char *)&sync_ack, msg, 4);
	le32_to_cpus(&sync_ack);
	dev_dbg(es705->dev, "%s(): sync_ack = 0x%08x\n", __func__, sync_ack);

	rc = snprintf(buf, PAGE_SIZE,
		       "%s=0x%08x\n",
		       status_name, sync_ack);

	return rc;
}

static DEVICE_ATTR(ping_status, 0444, es705_ping_status_show, NULL);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/ping_status */

static ssize_t es705_gpio_reset_set(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct es705_priv *es705 = &es705_priv;
	dev_dbg(es705->dev, "%s(): GPIO reset\n", __func__);
	es705->mode = SBL;
	es705_gpio_reset(es705);
	dev_dbg(es705->dev, "%s(): Ready for STANDARD download by proxy\n",
		__func__);
	return count;
}

static DEVICE_ATTR(gpio_reset, 0644, NULL, es705_gpio_reset_set);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/gpio_reset */


static ssize_t es705_overlay_mode_set(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct es705_priv *es705 = &es705_priv;
	int rc;
	int value = ES705_SET_POWER_STATE_VS_OVERLAY;

	dev_dbg(es705->dev, "%s(): Set Overlay mode\n", __func__);

	es705->mode = SBL;
	rc = es705_write(NULL, ES705_POWER_STATE , value);
	if (rc) {
		dev_err(es705_priv.dev, "%s(): Set Overlay mode failed\n",
			__func__);
	} else {
		msleep(50);
		es705_priv.es705_power_state = ES705_SET_POWER_STATE_VS_OVERLAY;
		/* wait until es705 SBL mode activating */
		dev_info(es705->dev, "%s(): After successful VOICESENSE download,"
			"Enable Event Intr to Host\n",
			__func__);
	}
	return count;
}

static DEVICE_ATTR(overlay_mode, 0644, NULL, es705_overlay_mode_set);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/overlay_mode */

static ssize_t es705_vs_event_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct es705_priv *es705 = &es705_priv;
	int rc;
	int value = ES705_SYNC_INTR_RISING_EDGE;

	dev_dbg(es705->dev, "%s(): Enable Voice Sense Event to Host\n",
		__func__);

	es705->mode = VOICESENSE;
	/* Enable Voice Sense Event INTR to Host */
	rc = es705_write(NULL, ES705_EVENT_RESPONSE, value);
	if (rc)
		dev_err(es705->dev, "%s(): Enable Event Intr fail\n",
			__func__);
	return count;
}

static DEVICE_ATTR(vs_event, 0644, NULL, es705_vs_event_set);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/vs_event */

static ssize_t es705_tuning_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct es705_priv *es705 = &es705_priv;
	unsigned int value = 0;

	sscanf(buf, "%d", &value);
	pr_info("%s : [ES705] uart_pin_config = %d\n", __func__, value);

	if (value == 0)
		es705->change_uart_config = 0;
	else
		es705->change_uart_config = 1;

	return count;
}

static DEVICE_ATTR(tuning, 0644, NULL, es705_tuning_set);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/tuning */
#define PATH_SIZE 100
static ssize_t es705_keyword_grammar_path_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	char path[PATH_SIZE], path2[PATH_SIZE] = {'\0'};
	char *index = 0;
	int rc = 0;

	if (strlen(buf) > PATH_SIZE) {
		dev_err(es705_priv.dev, "%s(): invalid buf length\n", __func__);
		return count;
	}

	sscanf(buf, "%s", path);
	pr_info("%s : [ES705] grammar path = %s\n", __func__, path);

	/* replace absolute path with relative path */
	index = strrchr(path, '/');
	if (index)
		strncpy(path2, index + 1, strlen(index + 1));
	else {
		pr_info("%s : [ES705] cannot find /\n", __func__);
		strncpy(path2, path, strlen(path));
	}
	pr_info("%s : [ES705] keyword_grammar_final_path = %s\n", __func__, path2);

	/* get the grammar file */
	rc = request_firmware((const struct firmware **)&es705_priv.vs_grammar,
			      path2, es705_priv.dev);
	if (rc) {
		dev_err(es705_priv.dev, "%s(): request_firmware(%s) failed %d\n",
			__func__, path2, rc);
		return count;
	}

	es705_priv.vs_grammar_set_flag = 1;

	return count;
}

static DEVICE_ATTR(keyword_grammar_path, 0664, NULL, es705_keyword_grammar_path_set);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/keyword_grammar_path */

static ssize_t es705_keyword_net_path_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	char path[PATH_SIZE], path2[PATH_SIZE] = {'\0'};
	char *index = 0;
	int rc = 0;

	if (strlen(buf) > PATH_SIZE) {
		dev_err(es705_priv.dev, "%s(): invalid buf length\n", __func__);
		return count;
	}

	sscanf(buf, "%s", path);
	pr_info("%s : [ES705] net path = %s\n", __func__, path);

	/* replace absolute path with relative path */
	index = strrchr(path, '/');
	if (index)
		strncpy(path2, index + 1, strlen(index + 1));
	else {
		pr_info("%s : [ES705] cannot find /\n", __func__);
		strncpy(path2, path, strlen(path));
	}
	pr_info("%s : [ES705] keyword_net_final_path = %s\n", __func__, path2);

	/* get the net file */
	rc = request_firmware((const struct firmware **)&es705_priv.vs_net,
			      path2, es705_priv.dev);
	if (rc) {
		dev_err(es705_priv.dev, "%s(): request_firmware(%s) failed %d\n",
			__func__, path2, rc);
		return count;
	}

	es705_priv.vs_net_set_flag = 1;

	return count;
}

static DEVICE_ATTR(keyword_net_path, 0664, NULL, es705_keyword_net_path_set);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/keyword_net_path */
static ssize_t es705_sleep_delay_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = 0;
	ret = snprintf(buf, PAGE_SIZE, "%d\n", es705_priv.sleep_delay);
	return ret;
}

static ssize_t es705_sleep_delay_set(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int rc;
	rc = kstrtoint(buf, 0, &es705_priv.sleep_delay);
	dev_info(es705_priv.dev, "%s(): sleep delay = %d\n",
		__func__, es705_priv.sleep_delay);
	return count;
}
static DEVICE_ATTR(sleep_delay, 0644, es705_sleep_delay_show, es705_sleep_delay_set);
/* /sys/devices/platform/msm_slim_ctrl.1/es705-codec-gen0/sleep_delay */

#if defined(PREVENT_CALL_MUTE_WHEN_SWITCH_NB_AND_WB)
static ssize_t es705_reroute_delay_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = 0;
	ret = snprintf(buf, PAGE_SIZE, "%d\n", es705_priv.reroute_delay);
	return ret;
}

static ssize_t es705_reroute_delay_set(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int rc;
	rc = kstrtoint(buf, 0, &es705_priv.reroute_delay);
	dev_info(es705_priv.dev, "%s(): reroute_delay = %d\n",
		__func__, es705_priv.reroute_delay);
	return count;
}
static DEVICE_ATTR(reroute_delay, 0644, es705_reroute_delay_show, es705_reroute_delay_set);
#endif

#ifdef SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY		
static ssize_t es705_preset_delay_time_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret = 0;
	ret = snprintf(buf, PAGE_SIZE, "%d\n", preset_delay_time);
	return ret;
}

static ssize_t es705_preset_delay_time_set(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int rc;
	rc = kstrtoint(buf, 0, &preset_delay_time);
	dev_info(es705_priv.dev, "%s(): preset_delay_time = %d\n",
		__func__, preset_delay_time);
	return count;
}
static DEVICE_ATTR(preset_delay, 0644, es705_preset_delay_time_show, es705_preset_delay_time_set);
#endif

static ssize_t es705_veq_filter_set(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct es705_priv *es705 = &es705_priv;
	struct file *fp = NULL;
	u8 *cur = NULL;
	int vol_step = es705->current_veq;
	int ret = 0, i=0;
	int filter_size = 0;
	int input = 0;

	sscanf(buf, "%d", &input);

	if (network_type == WIDE_BAND) {
		cur = &veq_coefficients_wb[vol_step][0];
		filter_size = 0x4A;
	} else {
		cur = &veq_coefficients_nb[vol_step][0];
		filter_size = 0x3E;
	}

	if (input == 1) {
		fp = file_open("/etc/filter.bin", O_RDONLY, 0);
		if (fp) {
			ret = file_read(fp, 0, cur, filter_size);

			if (ret < filter_size)
				dev_err(es705->dev, "%s : file read error\n", __func__);
			else
				dev_info(es705->dev, "%s : Success to change the values (%d): 0x%x",
						__func__, vol_step, ret);
			file_close(fp);
			fp = NULL;
		} else {
			dev_info(es705->dev, "%s : Current filter values (%d): ", __func__, vol_step);

			for (i=0; i < 10; i++)
				dev_info(es705->dev, "0x%02x ", *cur++);
		}
	}else if (input == 2) {
		dev_info(es705->dev, "%s : All filter values (%d): ", __func__, vol_step);

		for (i=0; i < filter_size; i++) {
			dev_info(es705->dev, "0x%02x ", *cur++);
		}
	} else
		dev_err(es705->dev, "%s : enter the value again\n", __func__);

	return count;
}
static DEVICE_ATTR(veq_filter, 0644, NULL, es705_veq_filter_set);

static ssize_t es705_veq_max_set(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct es705_priv *es705 = &es705_priv;
	u32 *cur = NULL;
	int vol_step = es705->current_veq;
	int i=0;
	int size = 0;
	int input = 0;

	sscanf(buf, "%d", &input);

	if (network_type == WIDE_BAND)
		cur = veq_max_gains_wb;
	else
		cur = veq_max_gains_nb;

	size = ARRAY_SIZE(veq_max_gains_nb);

	for (i=0; i < size; i++)
		dev_info(es705->dev, "%s : max gain (%d %d): ",
			__func__, i, (cur[i] & 0xff));
	cur[vol_step] = 0x90180000 | (input & 0xffff);
	dev_info(es705->dev, "%s : max gain is changed(%d %d): ",
		__func__, vol_step, (cur[vol_step] & 0xff));

	return count;
}
static DEVICE_ATTR(veq_max, 0644, NULL, es705_veq_max_set);

static ssize_t es705_veq_adj_set(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct es705_priv *es705 = &es705_priv;
	u32 *cur = NULL;
	int vol_step = es705->current_veq;
	int i=0;
	int size = 0;
	int input = 0;

	sscanf(buf, "%d", &input);

	if (network_type == WIDE_BAND)
		cur = veq_noise_estimate_adjs_wb;
	else
		cur = veq_noise_estimate_adjs_nb;

	size = ARRAY_SIZE(veq_noise_estimate_adjs_nb);

	for (i=0; i < size; i++)
		dev_info(es705->dev, "%s : adj gain (%d %d): ",
			__func__, i, (cur[i] & 0xff));
	cur[vol_step] = 0x90180000 | (input & 0xffff);
	dev_info(es705->dev, "%s : adj gain is changed(%d %d): ",
		__func__, vol_step, (cur[vol_step] & 0xff));

	return count;
}
static DEVICE_ATTR(veq_adj, 0644, NULL, es705_veq_adj_set);

static struct attribute *core_sysfs_attrs[] = {
	&dev_attr_route_status.attr,
	&dev_attr_route.attr,
	&dev_attr_rate.attr,
	&dev_attr_fw_version.attr,
	&dev_attr_clock_on.attr,
	&dev_attr_vs_keyword_parameters.attr,
	&dev_attr_vs_status.attr,
	&dev_attr_ping_status.attr,
	&dev_attr_gpio_reset.attr,
	&dev_attr_overlay_mode.attr,
	&dev_attr_vs_event.attr,
	&dev_attr_tuning.attr,
	&dev_attr_keyword_grammar_path.attr,
	&dev_attr_keyword_net_path.attr,
	&dev_attr_sleep_delay.attr,
#if defined(PREVENT_CALL_MUTE_WHEN_SWITCH_NB_AND_WB)
	&dev_attr_reroute_delay.attr,
#endif
#ifdef SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY		
	&dev_attr_preset_delay.attr,
#endif
	&dev_attr_veq_filter.attr,
	&dev_attr_veq_max.attr,
	&dev_attr_veq_adj.attr,
	NULL
};

static struct attribute_group core_sysfs = {
	.attrs = core_sysfs_attrs
};
extern unsigned int system_rev;

#if defined(CONFIG_MACH_KLTE_JPN)
#define UART_DOWNLOAD_WAKEUP_HWREV 7
#elif defined(CONFIG_MACH_KACTIVELTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_ATT) || defined(CONFIG_MACH_KSPORTSLTE_SPR) \
	|| defined(CONFIG_MACH_KACTIVELTE_SKT) || defined(CONFIG_SEC_S_PROJECT) || defined(CONFIG_MACH_KACTIVELTE_CAN) \
	|| defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_KACTIVELTE_KOR)
#define UART_DOWNLOAD_WAKEUP_HWREV 0
#else
#define UART_DOWNLOAD_WAKEUP_HWREV 6 /* HW rev0.7 */
#endif

static int es705_fw_download(struct es705_priv *es705, int fw_type)
{
	int rc = 0;

	dev_info(es705->dev, "%s(): fw download type %d begin\n",
							__func__, fw_type);
	mutex_lock(&es705->api_mutex);
	if (fw_type != VOICESENSE && fw_type != STANDARD) {
		dev_err(es705->dev, "%s(): Unexpected FW type\n", __func__);
		goto es705_fw_download_exit;
	}

	if (system_rev >= UART_DOWNLOAD_WAKEUP_HWREV || es705->uart_fw_download_rate)
		if (es705->uart_fw_download &&
				es705->uart_fw_download(es705, fw_type) >= 0) {
			es705->mode = fw_type;
			goto es705_fw_download_exit;
		}

	rc = es705->boot_setup(es705);
	if (rc) {
		dev_err(es705->dev, "%s(): fw download start error\n",
			__func__);
		goto es705_fw_download_exit;
	}

	dev_info(es705->dev, "%s(): write firmware image\n", __func__);
	if (fw_type == VOICESENSE)
		rc = es705->dev_write(es705, (char *)es705->vs->data,
					es705->vs->size);
	else
		rc = es705->dev_write(es705, (char *)es705->standard->data,
			es705->standard->size);
	if (rc) {
		dev_err(es705->dev, "%s(): firmware image write error\n",
			__func__);
		rc = -EIO;
		goto es705_fw_download_exit;
	}

	es705->mode = fw_type;
	rc = es705->boot_finish(es705);
	if (rc) {
		dev_err(es705->dev, "%s() fw download finish error\n",
			__func__);
			goto es705_fw_download_exit;
	}
	dev_info(es705->dev, "%s(): fw download type %d done\n",
							__func__, fw_type);

es705_fw_download_exit:
	mutex_unlock(&es705->api_mutex);
	return rc;
}

int es705_bootup(struct es705_priv *es705)
{
	int rc;
	int fw_max_retry_cnt = 10;
	BUG_ON(es705->standard->size == 0);

	mutex_lock(&es705->pm_mutex);
	es705->pm_state = ES705_POWER_FW_LOAD;
	mutex_unlock(&es705->pm_mutex);

do{
	rc = es705_fw_download(es705, STANDARD);
	if (rc) {
		dev_err(es705->dev, "%s(): STANDARD fw download error\n",
			__func__);
		es705_gpio_reset(es705);
	} else {
		mutex_lock(&es705->pm_mutex);
		es705->pm_state = ES705_POWER_AWAKE;
#if defined(SAMSUNG_ES705_FEATURE)
		es705->es705_power_state = ES705_SET_POWER_STATE_NORMAL;
#endif
		mutex_unlock(&es705->pm_mutex);
	}
}while( rc && fw_max_retry_cnt--);
	return rc;
}

static int es705_set_lpm(struct es705_priv *es705)
{
	int rc;
	const int max_retry_to_switch_to_lpm = 5;
	int retry = max_retry_to_switch_to_lpm;

	rc = es705_write(NULL, ES705_VS_INT_OSC_MEASURE_START, 0);
	if (rc) {
		dev_err(es705_priv.dev, "%s(): OSC Measure Start fail\n",
			__func__);
		goto es705_set_lpm_exit;
	}

	do {
		/* Wait 20ms before reading up to 5 times (total 100ms) */
		msleep(20);
		rc = es705_read(NULL, ES705_VS_INT_OSC_MEASURE_STATUS);
		if (rc < 0) {
			dev_err(es705_priv.dev, "%s(): OSC Measure Read Status fail\n",
				__func__);
			goto es705_set_lpm_exit;
		}
		dev_dbg(es705_priv.dev, "%s(): OSC Measure Status = 0x%04x\n",
			__func__, rc);
	} while (rc && --retry);

	if (rc) {
		dev_err(es705_priv.dev, "%s(): OSC Measure Read Status fail\n",
			__func__);
		goto es705_set_lpm_exit;
	}

	dev_dbg(es705_priv.dev, "%s(): Activate Low Power Mode\n", __func__);
	rc = es705_write(NULL, ES705_POWER_STATE,
			 ES705_SET_POWER_STATE_VS_LOWPWR);
	if (rc) {
		dev_err(es705_priv.dev, "%s(): Write cmd fail\n", __func__);
		goto es705_set_lpm_exit;
	}

	es705_priv.es705_power_state = ES705_SET_POWER_STATE_VS_LOWPWR;

	if (es705_priv.pdata->esxxx_clk_cb) {
		/* ext clock off */
		es705_priv.pdata->esxxx_clk_cb(0);
		dev_info(es705_priv.dev,
				"%s(): external clock off\n", __func__);
	}
es705_set_lpm_exit:
	return rc;
}

static int es705_vs_load(struct es705_priv *es705)
{
	int rc;
	BUG_ON(es705->vs->size == 0);

	/* wait es705 SBL mode */
	msleep(50);

	es705->es705_power_state = ES705_SET_POWER_STATE_VS_OVERLAY;
	mutex_lock(&es705->pm_mutex);
	rc = es705_fw_download(es705, VOICESENSE);
	mutex_unlock(&es705->pm_mutex);
	if (rc < 0) {
		dev_err(es705->dev, "%s(): FW download fail\n",
			__func__);
		goto es705_vs_load_fail;
	}
	msleep(50);
	/* Enable Voice Sense Event INTR to Host */
	rc = es705_write(NULL, ES705_EVENT_RESPONSE,
			ES705_SYNC_INTR_RISING_EDGE);
	if (rc) {
		dev_err(es705->dev, "%s(): Enable Event Intr fail\n",
			__func__);
		goto es705_vs_load_fail;
	}

es705_vs_load_fail:
	return rc;
}

static int register_snd_soc(struct es705_priv *priv);

int fw_download(void *arg)
{
	struct es705_priv *priv = (struct es705_priv *)arg;
	int rc;

	dev_info(priv->dev, "%s(): fw download\n", __func__);
	rc = es705_bootup(priv);
	dev_info(priv->dev, "%s(): bootup rc=%d\n", __func__, rc);

	rc = register_snd_soc(priv);
	dev_info(priv->dev, "%s(): register_snd_soc rc=%d\n", __func__, rc);

#ifdef FIXED_CONFIG
	es705_fixed_config(priv);
#endif

	dev_info(priv->dev, "%s(): release module\n", __func__);
	module_put(THIS_MODULE);
	return 0;
}

/* Hold the pm_mutex before calling this function */
#define CLK_OFF_DELAY 30
static int es705_sleep(struct es705_priv *es705)
{
	u32 cmd = (ES705_SET_SMOOTH << 16) | ES705_SET_SMOOTH_RATE;
	int rc;
#ifdef SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY
	u32 sync_cmd = (ES705_SYNC_CMD << 16) | ES705_SYNC_POLLING;
	u32 sync_rspn = sync_cmd;
	int match = 1;
	unsigned int value = 0;
#endif /* SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY */

	dev_info(es705->dev, "%s\n",__func__);

#if defined(FORCED_REROUTE_PRESET)
	if (delayed_work_pending(&es705->forced_reroute_work))
		cancel_delayed_work_sync(&es705->forced_reroute_work);
#endif

	mutex_lock(&es705->pm_mutex);

	es705->current_bwe = 0;
	es705->current_veq_preset = 0;
	es705->current_veq = -1;

#ifdef SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY
	rc = es705_write_then_read(es705, &sync_cmd, sizeof(sync_cmd),
								&sync_rspn, match);
	if (rc)
		dev_err(es705->dev, "%s(): send sync command failed rc = %d\n", __func__, rc);
#ifdef SAMSUNG_ES70X_RESTORE_FW_IN_SLEEP
	if (rc) {
		mutex_unlock(&es705->pm_mutex);	
		rc = restore_std_fw(es705);
		cnt_restore_std_fw_in_sleep++;
		mutex_lock(&es705->pm_mutex);
	}
	
	if (cnt_restore_std_fw_in_sleep)
		dev_info(es705->dev, "%s(): cnt_restore_std_fw_in_sleep = %d\n",
								__func__, cnt_restore_std_fw_in_sleep);
#endif /* SAMSUNG_ES70X_RESTORE_FW_IN_SLEEP */
#endif /* SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY */

	/* Avoid smoothing time */
	rc = es705_cmd(es705, cmd);
	if (rc < 0) {
		dev_err(es705->dev, "%s(): Reset Smooth Rate Fail",
			__func__);
		goto es705_sleep_exit;
	}
	
#ifdef SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY
	value = es705_read(NULL, ES705_CHANGE_STATUS);
	if (value)
		dev_err(es705->dev, "%s(): Route Status = 0x%04x\n", __func__, value);
#endif /* SAMSUNG_ES70X_BACK_TO_BACK_CMD_DELAY */

#ifdef AUDIENCE_VS_IMPLEMENTATION
	if (es705->voice_wakeup_enable) {
		rc = es705_switch_to_vs_mode(&es705_priv);
		if (rc) {
			dev_err(es705->dev, "%s(): Set VS Overlay FAIL", __func__);
		} else {
			/* Set PDM route */
			es705_switch_route(22);
			msleep(20);
			rc = es705_set_lpm(es705);
			if (rc) {
				dev_err(es705->dev, "%s(): Set LPM FAIL", __func__);
			} else {
				es705->pm_state = ES705_POWER_SLEEP;
				goto es705_sleep_exit;
			}
		}
	}
#endif
	/*
	 * write Set Power State Sleep - 0x9010_0001
	 * wait 20 ms, and then turn ext clock off
	 * There will not be any response after
	 * sleep command from chip
	 */
	cmd = (ES705_SET_POWER_STATE << 16) | ES705_SET_POWER_STATE_SLEEP;
	rc = es705_cmd(es705, cmd);
	if (rc)
	   	dev_err(es705->dev, "%s(): send sleep command failed rc = %d\n", __func__, rc);

	dev_dbg(es705->dev, "%s: wait %dms for execution\n",__func__, CLK_OFF_DELAY);
	msleep(CLK_OFF_DELAY);

	es705->es705_power_state = ES705_SET_POWER_STATE_SLEEP;
	es705->pm_state = ES705_POWER_SLEEP;

	if (es705->pdata->esxxx_clk_cb) {
		es705->pdata->esxxx_clk_cb(0);
		dev_info(es705->dev, "%s(): external clock off\n", __func__);
	}

es705_sleep_exit:
	dev_info(es705->dev, "%s(): Exit\n",__func__);
	mutex_unlock(&es705->pm_mutex);	
	return rc;
}

static void es705_delayed_sleep(struct work_struct *w)
{
	int ch_tot;
	int ports_active = (es705_priv.rx1_route_enable ||
		es705_priv.rx2_route_enable || es705_priv.tx1_route_enable);

	/*
	 * If there are active streams we do not sleep.
	 * Count the front end (FE) streams ONLY.
	 */

	ch_tot = 0;
	ch_tot += es705_priv.dai[ES705_SLIM_1_PB].ch_tot;
	ch_tot += es705_priv.dai[ES705_SLIM_2_PB].ch_tot;

	ch_tot += es705_priv.dai[ES705_SLIM_1_CAP].ch_tot;
	dev_dbg(es705_priv.dev, "%s %d active channels, ports_active: %d\n",
		__func__, ch_tot, ports_active);
/*	mutex_lock(&es705_priv.pm_mutex); */
	if ((ch_tot <= 0) && (ports_active == 0) &&
		(es705_priv.pm_state ==  ES705_POWER_SLEEP_PENDING))
		es705_sleep(&es705_priv);

/*	mutex_unlock(&es705_priv.pm_mutex); */
}

static void es705_sleep_request(struct es705_priv *es705)
{
	dev_dbg(es705->dev, "%s internal es705_power_state = %d\n",
		__func__, es705_priv.pm_state);

	mutex_lock(&es705->pm_mutex);
	if (es705->uart_state == UART_OPEN)
		es705_uart_close(es705);
	
	if (es705->pm_state == ES705_POWER_AWAKE) {
		schedule_delayed_work(&es705->sleep_work,
			msecs_to_jiffies(es705->sleep_delay));
		es705->pm_state = ES705_POWER_SLEEP_PENDING;
	}
	mutex_unlock(&es705->pm_mutex);
}

#define SYNC_DELAY 35
#define MAX_RETRY_WAKEUP_CNT 1
static int es705_wakeup(struct es705_priv *es705)
{
	int rc = 0;
	u32 sync_cmd = (ES705_SYNC_CMD << 16) | ES705_SYNC_POLLING;
	u32 sync_rspn = sync_cmd;
	int match = 1;
#ifndef SAMSUNG_ES70X_RESTORE_FW_IN_WAKEUP
	int retry = 0;	
	int retry_wakeup_cnt = 0;
#endif
	dev_info(es705->dev, "%s\n",__func__);
	/* 1 - clocks on
	 * 2 - wakeup 1 -> 0
	 * 3 - sleep 30 ms
	 * 4 - Send sync command (0x8000, 0x0001)
	 * 5 - Read sync ack
	 * 6 - wakeup 0 -> 1
	 */

	mutex_lock(&es705->pm_mutex);
#if defined(SAMSUNG_ES705_FEATURE)
	msm_slim_es705_func(es705_priv.gen0_client);
#endif

#if defined(FORCED_REROUTE_PRESET)
	if (delayed_work_pending(&es705->forced_reroute_work))
			cancel_delayed_work_sync(&es705->forced_reroute_work);
#endif

	if (delayed_work_pending(&es705->sleep_work) ||
		(es705->pm_state == ES705_POWER_SLEEP_PENDING)) {
		mutex_unlock(&es705->pm_mutex);
		cancel_delayed_work_sync(&es705->sleep_work);
		mutex_lock(&es705->pm_mutex);
		if (es705->pm_state == ES705_POWER_SLEEP_PENDING) {
			es705->pm_state = ES705_POWER_AWAKE;
			goto es705_wakeup_exit;
		}
	}

	/* Check if previous power state is not sleep then return */
	if (es705->pm_state != ES705_POWER_SLEEP) {
		dev_err(es705->dev, "%s(): no need to go to Normal Mode\n",
			__func__);
		goto es705_wakeup_exit;
	}

	if (es705->pdata->esxxx_clk_cb) {
		es705->pdata->esxxx_clk_cb(1);
		usleep_range(3000, 3100);
		dev_info(es705->dev,
				"%s(): external clock on\n", __func__);
	}
#ifndef SAMSUNG_ES70X_RESTORE_FW_IN_WAKEUP
RETRY_TO_WAKEUP:
#endif
	if (es705->change_uart_config) {
		es705_uart_pin_preset(es705);
	}

#if defined(SAMSUNG_ES705_FEATURE)
	if (es705_priv.use_uart_for_wakeup_gpio) {
		dev_info(es705->dev, "%s(): begin uart wakeup\n",
			__func__);
#endif
		es705_uart_es705_wakeup(es705);

#if !defined(SAMSUNG_ES705_FEATURE)
		if (es705->wakeup_bus)
			es705->wakeup_bus(es705);
#endif
#if defined(SAMSUNG_ES705_FEATURE)
	} else {
		es705_gpio_wakeup(es705);
	}
#endif

	if (es705->change_uart_config) {
		es705_uart_pin_postset(es705);
	}

	msleep(SYNC_DELAY);
#ifdef SAMSUNG_ES70X_RESTORE_FW_IN_WAKEUP
	rc = es705_write_then_read(es705, &sync_cmd, sizeof(sync_cmd),
								&sync_rspn, match);
	if (rc) {
		mutex_unlock(&es705->pm_mutex); 
		rc = restore_std_fw(es705);
		cnt_restore_std_fw_in_wakeup++;
		mutex_lock(&es705->pm_mutex);
	}
	
	if (cnt_restore_std_fw_in_wakeup)
		dev_info(es705->dev, "%s(): cnt_restore_std_fw_in_wakeup = %d\n",
								__func__, cnt_restore_std_fw_in_wakeup);
#else /* !SAMSUNG_ES70X_RESTORE_FW_IN_WAKEUP */		
	/* retry wake up */
	retry = 0;
	do {
		rc = es705_write_then_read(es705, &sync_cmd, sizeof(sync_cmd),
						 &sync_rspn, match);
		if (rc) {
			dev_info(es705->dev, "%s(): wait %dms wakeup, then ping SYNC to es705, retry(%d) rspn=0x%08x\n", __func__, SYNC_DELAY, retry, sync_rspn);
			msleep(SYNC_DELAY);
		}
		else
			break;
	} while (++retry < MAX_RETRY_WAKEUP_CNT);

	/* if wakeup fail Retry wakeup pin falling */
	if (rc) {
		if (retry_wakeup_cnt++ < MAX_RETRY_WAKEUP_CNT) {
			dev_info(es705->dev, "%s(): Retry wakeup pin falling (%d)\n", __func__, retry_wakeup_cnt);
			goto RETRY_TO_WAKEUP;
		}
	}
#endif /* SAMSUNG_ES70X_RESTORE_FW_IN_WAKEUP */

	if (rc) {
		dev_err(es705->dev, "%s(): es705 wakeup FAIL\n", __func__);
		if (es705->pdata->esxxx_clk_cb) {
			es705->pdata->esxxx_clk_cb(0);
			usleep_range(3000, 3100);
			dev_info(es705->dev,
					"%s(): external clock off\n", __func__);
		}
		goto es705_wakeup_exit;
	}
	if (es705->es705_power_state != ES705_SET_POWER_STATE_VS_LOWPWR &&
		es705->es705_power_state != ES705_SET_POWER_STATE_VS_OVERLAY)
		es705_switch_to_normal_mode(&es705_priv);

	dev_dbg(es705->dev, "%s(): wakeup success, SYNC response 0x%08x\n",
		__func__, sync_rspn);

es705_wakeup_exit:
	dev_info(es705->dev, "%s(): Exit\n",__func__);
	mutex_unlock(&es705->pm_mutex);
	return rc;
}

#if defined(SAMSUNG_ES705_FEATURE)
static void es705_read_write_power_control(int read_write)
{
	if (read_write)
		es705_priv.power_control(ES705_SET_POWER_STATE_NORMAL, ES705_POWER_STATE);
	else if (!(es705_priv.rx1_route_enable || es705_priv.rx2_route_enable || es705_priv.tx1_route_enable || es705_priv.voice_wakeup_enable))
		es705_priv.power_control(ES705_SET_POWER_STATE_SLEEP, ES705_POWER_STATE);
}
#endif

static int es705_put_control_value(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	/* struct snd_soc_codec *codec = es705_priv.codec; */
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc = 0;

	value = ucontrol->value.integer.value[0];
	rc = es705_write(NULL, reg, value);

	return 0;
}

static int es705_get_control_value(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	/* struct snd_soc_codec *codec = es705_priv.codec; */
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value =0;

	if (es705_priv.rx1_route_enable ||
		es705_priv.tx1_route_enable ||
		es705_priv.rx2_route_enable) {
	value = es705_read(NULL, reg);
	}
	ucontrol->value.integer.value[0] = value;

	return 0;
}

static int es705_put_control_enum(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e =
		(struct soc_enum *)kcontrol->private_value;
	unsigned int reg = e->reg;
	unsigned int value;
	int rc = 0;

	value = ucontrol->value.enumerated.item[0];
	rc = es705_write(NULL, reg, value);

	return 0;
}

static int es705_get_control_enum(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e =
		(struct soc_enum *)kcontrol->private_value;
	unsigned int reg = e->reg;
	unsigned int value=0;

	if (es705_priv.rx1_route_enable ||
		es705_priv.tx1_route_enable ||
		es705_priv.rx2_route_enable) {
	value = es705_read(NULL, reg);
}

	ucontrol->value.enumerated.item[0] = value;

	return 0;
}

static int es705_get_power_control_enum(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e =
		(struct soc_enum *)kcontrol->private_value;
	unsigned int reg = e->reg;
	unsigned int value;

	/* Don't read if already in Sleep Mode */
	if (es705_priv.pm_state == ES705_POWER_SLEEP)
		value = es705_priv.es705_power_state;
	else
		value = es705_read(NULL, reg);

	ucontrol->value.enumerated.item[0] = value;

	return 0;
}

static int es705_get_uart_fw_download_rate(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.enumerated.item[0] = es705_priv.uart_fw_download_rate;
	return 0;
}

static int es705_put_uart_fw_download_rate(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	/*
	 * 0 - no use UART
	 * 1 - use UART for FW download. Baud Rate 4.608 KBps
	 * 2 - use UART for FW download. Baud Rate 1 MBps
	 * 3 - use UART for FW download. Baud Rate 3 Mbps
	 */
	es705_priv.uart_fw_download_rate = ucontrol->value.enumerated.item[0];
	return 0;
}

static int es705_get_vs_stream_enable(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.enumerated.item[0] = es705_priv.vs_stream_enable;
	return 0;
}

static int es705_put_vs_stream_enable(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	es705_priv.vs_stream_enable = ucontrol->value.enumerated.item[0];
	return 0;
}

#if defined(SAMSUNG_ES705_FEATURE)
static int es705_sleep_power_control(unsigned int value, unsigned int reg)
{
	int rc = 0;

	switch (es705_priv.es705_power_state) {
	case ES705_SET_POWER_STATE_SLEEP :
		if (value == ES705_SET_POWER_STATE_NORMAL) {
			rc = es705_wakeup(&es705_priv);
			if (rc < 0) {
				dev_err(es705_priv.dev, "%s(): es705_wakeup failed\n",
					__func__);
				return rc;
			}
			rc = es705_write(NULL, reg, value);
			if (rc < 0) {
				dev_err(es705_priv.dev, "%s(): Power state command write failed\n",
					__func__);
				return rc;
			}
			/* Wait for 100ms to switch from Overlay mode */
			msleep(100);
			es705_priv.es705_power_state =
				ES705_SET_POWER_STATE_NORMAL;
			es705_priv.mode = STANDARD;
			es705_priv.pm_state = ES705_POWER_AWAKE;
		} else if (value == ES705_SET_POWER_STATE_VS_OVERLAY) {
			rc = es705_wakeup(&es705_priv);
			if (rc < 0) {
				dev_err(es705_priv.dev, "%s(): es705_wakeup failed\n",
					__func__);
				return rc;
			}
			rc = es705_switch_to_vs_mode(&es705_priv);
		}
		break;

	case ES705_SET_POWER_STATE_NORMAL :
	case ES705_SET_POWER_STATE_VS_OVERLAY :
		return -EINVAL;

	case ES705_SET_POWER_STATE_VS_LOWPWR :
		if (value == ES705_SET_POWER_STATE_VS_OVERLAY) {
			rc = es705_wakeup(&es705_priv);
			if (rc < 0) {
				dev_err(es705_priv.dev, "%s(): es705_wakeup failed\n",
					__func__);
				return rc;
			}
			es705_priv.es705_power_state =
				ES705_SET_POWER_STATE_VS_OVERLAY;
			es705_priv.pm_state = ES705_POWER_AWAKE;
		}
		break;

	default :
		return -EINVAL;
	}
	return rc;
}

static int es705_sleep_pending_power_control(unsigned int value,
						unsigned int reg)
{
	int rc = 0;
	int retry = 10;

	switch (es705_priv.es705_power_state) {
	case ES705_SET_POWER_STATE_SLEEP :
		return -EINVAL;

	case ES705_SET_POWER_STATE_NORMAL :
		if (value == ES705_SET_POWER_STATE_SLEEP)
			es705_sleep_request(&es705_priv);
		else if (value == ES705_SET_POWER_STATE_NORMAL) {
			rc = es705_wakeup(&es705_priv);
			if (rc < 0) {
				dev_err(es705_priv.dev, "%s(): es705_wakeup failed\n",
					__func__);
				return rc;
			}
			es705_priv.es705_power_state = \
				ES705_SET_POWER_STATE_NORMAL;
			es705_priv.mode = STANDARD;
			es705_priv.pm_state = ES705_POWER_AWAKE;
		} else if (value == ES705_SET_POWER_STATE_VS_OVERLAY) {
			rc = es705_wakeup(&es705_priv);
			if (rc < 0) {
				dev_err(es705_priv.dev, "%s(): es705_wakeup failed\n",
					__func__);
				return rc;
			}
			rc = es705_switch_to_vs_mode(&es705_priv);
			if (rc < 0)
				return rc;
		}
		break;

	case ES705_SET_POWER_STATE_VS_OVERLAY :
		if (value == ES705_SET_POWER_STATE_SLEEP)
			es705_sleep_request(&es705_priv);
		else if (value == ES705_SET_POWER_STATE_NORMAL) {
			rc = es705_wakeup(&es705_priv);
			if (rc < 0) {
				dev_err(es705_priv.dev, "%s(): es705_wakeup failed\n",
					__func__);
				return rc;
			}
			rc = es705_write(NULL, reg, value);
			if (rc < 0) {
				dev_err(es705_priv.dev, "%s(): Power state command write failed\n",
					__func__);
				return rc;
			}
			/* Wait for 100ms to switch from Overlay mode */
			msleep(100);
			es705_priv.es705_power_state = \
				ES705_SET_POWER_STATE_NORMAL;
			es705_priv.mode = STANDARD;
		} else if (value == ES705_SET_POWER_STATE_VS_LOWPWR) {
			rc = es705_write(NULL,
				ES705_VS_INT_OSC_MEASURE_START, 0);
			do {
				/* Wait 20ms each time before reading,
				Status may take 100ms to be done
				added retries */
				msleep(20);
				rc = es705_read(NULL,
				ES705_VS_INT_OSC_MEASURE_STATUS);
				if (rc == 0) {
					dev_dbg(es705_priv.dev, "%s(): Activate Low Power Mode\n",
						__func__);
					es705_write(NULL, reg, value);
					es705_priv.es705_power_state =\
					ES705_SET_POWER_STATE_VS_LOWPWR;
					es705_priv.pm_state =\
						ES705_POWER_SLEEP;
					return rc;
				}
			} while (retry--);
		}
		break;

	case ES705_SET_POWER_STATE_VS_LOWPWR:
	default:
		return -EINVAL;
	}

	return rc;
}

static int es705_awake_power_control(unsigned int value, unsigned int reg)
{
	int rc = 0;
	int retry = 10;

	switch (es705_priv.es705_power_state) {
	case ES705_SET_POWER_STATE_SLEEP:
		return -EINVAL;

	case ES705_SET_POWER_STATE_NORMAL:
		if (value == ES705_SET_POWER_STATE_SLEEP)
			es705_sleep_request(&es705_priv);
		else if (value == ES705_SET_POWER_STATE_VS_OVERLAY) {
			rc = es705_switch_to_vs_mode(&es705_priv);
			if (rc < 0)
				return rc;
		} else
			return -EINVAL;
		break;

	case ES705_SET_POWER_STATE_VS_OVERLAY:
		if (value == ES705_SET_POWER_STATE_SLEEP)
			es705_sleep_request(&es705_priv);
		else if (value == ES705_SET_POWER_STATE_NORMAL) {
			rc = es705_write(NULL, reg, value);
			if (rc < 0) {
				dev_err(es705_priv.dev, "%s(): Power state command write failed\n",
					__func__);
				return rc;
			}
			/* Wait for 100ms to switch from Overlay mode */
			msleep(100);
			es705_priv.es705_power_state =
				ES705_SET_POWER_STATE_NORMAL;
			es705_priv.mode = STANDARD;
		} else if (value == ES705_SET_POWER_STATE_VS_LOWPWR) {
			rc = es705_write(NULL,
				ES705_VS_INT_OSC_MEASURE_START, 0);
			do {
				/* Wait 20ms each time before reading,
				Status may take 100ms to be done
				added retries */
				msleep(20);
				rc = es705_read(NULL,
				ES705_VS_INT_OSC_MEASURE_STATUS);
				if (rc == 0) {
					dev_dbg(es705_priv.dev, "%s(): Activate Low Power Mode\n",
						__func__);
					es705_write(NULL, reg, value);

					/* Disable external clock */
					msleep(20);
					/* clocks off */
					if (es705_priv.pdata->esxxx_clk_cb) {
						es705_priv.pdata->esxxx_clk_cb(0);
						dev_info(es705_priv.dev,
								"%s(): external clock off\n", __func__);
					}
					es705_priv.es705_power_state =
						ES705_SET_POWER_STATE_VS_LOWPWR;
					es705_priv.pm_state =
						ES705_POWER_SLEEP;
					return rc;
				}
			} while (retry--);
		}
		break;

	case ES705_SET_POWER_STATE_VS_LOWPWR:
		dev_dbg(es705_priv.dev, "%s(): Set Overlay mode\n", __func__);
		es705_priv.mode = VOICESENSE;
		es705_priv.es705_power_state = ES705_SET_POWER_STATE_VS_OVERLAY;
		/* wait until es705 SBL mode activating */
		dev_dbg(es705_priv.dev, "%s(): Ready for VOICESENSE download by proxy\n",
			__func__);
		dev_info(es705_priv.dev, "%s(): After successful VOICESENSE download,"
			"Enable Event Intr to Host\n", __func__);
		break;
	default:
		return -EINVAL;
	}
	return rc;
}

static int es705_power_control(unsigned int value, unsigned int reg)
{
	int rc = 0;

	dev_info(es705_priv.dev, "%s(): entry pm state %d es705 state %d value %d\n",
		__func__, es705_priv.pm_state,
		es705_priv.es705_power_state, value);

	if (value == 0 || value == ES705_SET_POWER_STATE_MP_SLEEP ||
		value == ES705_SET_POWER_STATE_MP_CMD) {
		dev_err(es705_priv.dev, "%s(): Unsupported value in es705\n",
			__func__);
		return -EINVAL;
	}
	switch (es705_priv.pm_state) {
	case ES705_POWER_FW_LOAD:
		return -EINVAL;

	case ES705_POWER_SLEEP:
		rc = es705_sleep_power_control(value, reg);
		break;

	case ES705_POWER_SLEEP_PENDING:
		rc = es705_sleep_pending_power_control(value, reg);
		break;


	case ES705_POWER_AWAKE:
		rc = es705_awake_power_control(value, reg);
		break;

	default:
		dev_err(es705_priv.dev, "%s(): Unsupported pm state [%d] in es705\n",
			__func__, es705_priv.pm_state);
		break;
	}
	dev_info(es705_priv.dev, "%s(): exit pm state %d es705 state %d value %d\n",
		__func__, es705_priv.pm_state,
		es705_priv.es705_power_state, value);

	return rc;
}
#endif

#define MAX_RETRY_TO_SWITCH_TO_LOW_POWER_MODE	5
static int es705_put_power_control_enum(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	unsigned int value;
	int rc = 0;

	value = ucontrol->value.enumerated.item[0];
	dev_dbg(es705_priv.dev, "%s(): Previous power state = %s, power set cmd = %s\n",
		__func__, power_state[es705_priv.pm_state],
		power_state_cmd[es705_priv.es705_power_state]);
	dev_dbg(es705_priv.dev, "%s(): Requested power set cmd = %s\n",
		__func__, power_state_cmd[value]);

	if (value == 0 || value == ES705_SET_POWER_STATE_MP_SLEEP ||
		value == ES705_SET_POWER_STATE_MP_CMD) {
		dev_err(es705_priv.dev, "%s(): Unsupported state in es705\n",
			__func__);
		rc = -EINVAL;
		goto es705_put_power_control_enum_exit;
	} else {
		if ((es705_priv.pm_state == ES705_POWER_SLEEP) &&
			(value != ES705_SET_POWER_STATE_NORMAL) &&
			(value != ES705_SET_POWER_STATE_VS_OVERLAY)) {
			dev_err(es705_priv.dev, "%s(): ES705 is in sleep mode."
				" Select the Normal Mode or Overlay"
				" if in Low Power mode.\n", __func__);
			rc = -EPERM;
			goto  es705_put_power_control_enum_exit;
		}

		if (value == ES705_SET_POWER_STATE_SLEEP) {
			dev_dbg(es705_priv.dev, "%s(): Activate Sleep Request\n",
						__func__);
			es705_sleep_request(&es705_priv);
		} else if (value == ES705_SET_POWER_STATE_NORMAL) {
			/* Overlay mode doesn't need wakeup */
			if (es705_priv.es705_power_state !=
				ES705_SET_POWER_STATE_VS_OVERLAY) {
				rc = es705_wakeup(&es705_priv);
				if (rc)
					goto es705_put_power_control_enum_exit;
			} else {
				rc = es705_write(NULL, ES705_POWER_STATE,
						 value);
				if (rc) {
					dev_err(es705_priv.dev, "%s(): Power state command write failed\n",
						__func__);
					goto es705_put_power_control_enum_exit;
				}
				/* Wait for 100ms to switch from Overlay mode */
				msleep(100);
			}
			es705_priv.es705_power_state =
				ES705_SET_POWER_STATE_NORMAL;
			es705_priv.mode = STANDARD;

		} else if (value == ES705_SET_POWER_STATE_VS_LOWPWR) {
			if (es705_priv.es705_power_state ==
					ES705_SET_POWER_STATE_VS_OVERLAY) {
				rc = es705_set_lpm(&es705_priv);
				if (rc)
					dev_err(es705_priv.dev, "%s(): Can't switch to Low Power Mode\n",
						__func__);
				else
					es705_priv.pm_state = ES705_POWER_SLEEP;
			} else {
				dev_err(es705_priv.dev, "%s(): ES705 should be in VS Overlay"
					"mode. Select the VS Overlay Mode.\n",
					__func__);
				rc = -EINVAL;
			}
			goto es705_put_power_control_enum_exit;
		} else if (value == ES705_SET_POWER_STATE_VS_OVERLAY) {
			if (es705_priv.es705_power_state ==
					ES705_SET_POWER_STATE_VS_LOWPWR) {
				rc = es705_wakeup(&es705_priv);
				if (rc)
					goto es705_put_power_control_enum_exit;
				es705_priv.es705_power_state =
					     ES705_SET_POWER_STATE_VS_OVERLAY;
			} else {
				rc = es705_switch_to_vs_mode(&es705_priv);
				if (rc)
					goto es705_put_power_control_enum_exit;
			}
		}
	}

es705_put_power_control_enum_exit:
	dev_dbg(es705_priv.dev, "%s(): Current power state = %s, power set cmd = %s\n",
		__func__, power_state[es705_priv.pm_state],
		power_state_cmd[es705_priv.es705_power_state]);
	return rc;
}

static int es705_get_rx1_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es705_priv.rx1_route_enable;
	dev_dbg(es705_priv.dev, "%s(): rx1_route_enable = %d\n",
		__func__, es705_priv.rx1_route_enable);

	return 0;
}

static int es705_put_rx1_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	es705_priv.rx1_route_enable = ucontrol->value.integer.value[0];
	dev_dbg(es705_priv.dev, "%s(): rx1_route_enable = %d\n",
		__func__, es705_priv.rx1_route_enable);
#if defined(SAMSUNG_ES705_FEATURE)
	if (es705_priv.power_control) {
		if(es705_priv.rx1_route_enable)
			es705_priv.power_control(ES705_SET_POWER_STATE_NORMAL,
						 ES705_POWER_STATE);
		else if (!(es705_priv.rx1_route_enable ||
				es705_priv.rx2_route_enable ||
				es705_priv.tx1_route_enable))
			es705_priv.power_control(ES705_SET_POWER_STATE_SLEEP,
						ES705_POWER_STATE);
	}
#endif
	return 0;
}

static int es705_get_tx1_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es705_priv.tx1_route_enable;
	dev_dbg(es705_priv.dev, "%s(): tx1_route_enable = %d\n",
		__func__, es705_priv.tx1_route_enable);

	return 0;
}

static int es705_put_tx1_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	es705_priv.tx1_route_enable = ucontrol->value.integer.value[0];
	dev_dbg(es705_priv.dev, "%s(): tx1_route_enable = %d\n",
		__func__, es705_priv.tx1_route_enable);

#if defined(SAMSUNG_ES705_FEATURE)
	if (es705_priv.power_control) {
		if(es705_priv.tx1_route_enable)
			es705_priv.power_control(ES705_SET_POWER_STATE_NORMAL,
						 ES705_POWER_STATE);
		else if (!(es705_priv.rx1_route_enable ||
				es705_priv.rx2_route_enable ||
				es705_priv.tx1_route_enable))
			es705_priv.power_control(ES705_SET_POWER_STATE_SLEEP,
						ES705_POWER_STATE);
	}
#endif
return 0;
}

static int es705_get_rx2_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es705_priv.rx2_route_enable;
	dev_dbg(es705_priv.dev, "%s(): rx2_route_enable = %d\n",
		__func__, es705_priv.rx2_route_enable);

	return 0;
}

static int es705_put_rx2_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	es705_priv.rx2_route_enable = ucontrol->value.integer.value[0];
	dev_dbg(es705_priv.dev, "%s(): rx2_route_enable = %d\n",
		__func__, es705_priv.rx2_route_enable);

#if defined(SAMSUNG_ES705_FEATURE)
	if (es705_priv.power_control) {
		if(es705_priv.rx2_route_enable)
			es705_priv.power_control(ES705_SET_POWER_STATE_NORMAL,
						ES705_POWER_STATE);
		else if (!(es705_priv.rx1_route_enable ||
				es705_priv.rx2_route_enable ||
				es705_priv.tx1_route_enable))
			es705_priv.power_control(ES705_SET_POWER_STATE_SLEEP,
						ES705_POWER_STATE);
	}
#endif
return 0;
}

#if defined(SAMSUNG_ES705_FEATURE)
static int es705_get_default_keyword(void)
{
	char path[30];
	int rc;

	sprintf(path, "higalaxy_en_us_gram6.bin");
	dev_info(es705_priv.dev, "%s(): request default grammar\n", __func__);
	rc = request_firmware((const struct firmware **)&es705_priv.vs_grammar,
		      path, es705_priv.dev);
	if (rc)
		dev_err(es705_priv.dev, "%s(): request_firmware(%s) failed %d\n",
			__func__, path, rc);

	sprintf(path, "higalaxy_en_us_am.bin");
	dev_info(es705_priv.dev, "%s(): request default net\n", __func__);
	rc = request_firmware((const struct firmware **)&es705_priv.vs_net,
		      path, es705_priv.dev);
	if (rc)
		dev_err(es705_priv.dev, "%s(): request_firmware(%s) failed %d\n",
			__func__, path, rc);

	return rc;

}
static int es705_get_voice_wakeup_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es705_priv.voice_wakeup_enable;
	dev_dbg(es705_priv.dev, "%s(): voice_wakeup_enable = %d\n",
		__func__, es705_priv.voice_wakeup_enable);

	return 0;
}

static int es705_put_voice_wakeup_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	int rc = 0;
	u32 sync_cmd = (ES705_SYNC_CMD << 16) | ES705_SYNC_INTR_RISING_EDGE;
	u32 sync_rspn = sync_cmd;
	u32 cmd_lpsd[] = {0x9017e03c, 0x901800c8,
			0x9017e03d, 0x901804b0,
			0x9017e03e, 0x90180580,
			0x9017e03f, 0x90180000,
			0x9017e040, 0x90180002,
			0x9017e000, 0x90180002,
			0xffffffff};
	int match = 1;

	if (es705_priv.voice_wakeup_enable == ucontrol->value.integer.value[0]) {
		dev_info(es705_priv.dev, "%s(): skip to set voice_wakeup_enable[%d->%ld]\n",
			__func__, es705_priv.voice_wakeup_enable,
			ucontrol->value.integer.value[0]);
		return rc;
	}
	es705_priv.voice_wakeup_enable = ucontrol->value.integer.value[0];
	dev_info(es705_priv.dev, "%s(): voice_wakeup_enable = %d\n",
		__func__, es705_priv.voice_wakeup_enable);

	if (es705_priv.power_control) {
		if(es705_priv.voice_wakeup_enable == 1) { /* Voice wakeup */
			if (!es705_priv.vs_grammar_set_flag || !es705_priv.vs_net_set_flag)
				es705_get_default_keyword();
			es705_priv.power_control(ES705_SET_POWER_STATE_VS_OVERLAY, ES705_POWER_STATE);
			if (es705_priv.es705_power_state == ES705_SET_POWER_STATE_VS_OVERLAY) {
				es705_switch_route_config(27);
				rc = es705_write_then_read(&es705_priv, &sync_cmd,
								sizeof(sync_cmd), &sync_rspn, match);
				if (rc) {
					dev_err(es705_priv.dev, "%s(): es705 Sync FAIL\n", __func__);
				} else {
					rc = es705_write_sensory_vs_keyword();
					if (rc)
						dev_err(es705_priv.dev, "%s(): es705 keyword download FAIL\n",
							__func__);
				}
			}
		} else if(es705_priv.voice_wakeup_enable == 2) { /* Voice wakeup LPSD - for Baby cry */
			es705_priv.power_control(ES705_SET_POWER_STATE_VS_OVERLAY, ES705_POWER_STATE);
			if (es705_priv.es705_power_state == ES705_SET_POWER_STATE_VS_OVERLAY) {
				es705_switch_route_config(27);
				es705_write_block(&es705_priv, cmd_lpsd);
				rc = es705_write_then_read(&es705_priv, &sync_cmd, sizeof(sync_cmd),
							&sync_rspn, match);
				if (rc)
					dev_err(es705_priv.dev, "%s(): es705 Sync FAIL\n", __func__);
			}
		} else {
			if (es705_priv.es705_power_state == ES705_SET_POWER_STATE_VS_LOWPWR)
				es705_priv.power_control(ES705_SET_POWER_STATE_VS_OVERLAY, ES705_POWER_STATE);
			if (es705_priv.es705_power_state == ES705_SET_POWER_STATE_VS_OVERLAY) {
				es705_switch_route_config(5);
				es705_priv.power_control(ES705_SET_POWER_STATE_NORMAL, ES705_POWER_STATE);
				es705_read_write_power_control(0);
			}
		}
	}
	return rc;
}

static int es705_get_voice_lpm_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es705_priv.voice_lpm_enable;
	dev_dbg(es705_priv.dev, "%s(): voice_lpm_enable = %d\n",
		__func__, es705_priv.voice_lpm_enable);

	return 0;
}

static int es705_put_voice_lpm_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	if (es705_priv.voice_lpm_enable == ucontrol->value.integer.value[0]) {
		dev_info(es705_priv.dev, "%s(): skip to set voice_lpm_enable[%d->%ld]\n",
			__func__, es705_priv.voice_lpm_enable,
			ucontrol->value.integer.value[0]);
		return 0;
	}
	es705_priv.voice_lpm_enable = ucontrol->value.integer.value[0];

	dev_info(es705_priv.dev, "%s(): voice_lpm_enable = %d\n",
		__func__, es705_priv.voice_lpm_enable);

	if (!es705_priv.power_control) {
		dev_err(es705_priv.dev, "%s(): lpm enable error\n", __func__);
		return -ENODEV;
	}
	if (es705_priv.voice_lpm_enable)
		es705_priv.power_control(ES705_SET_POWER_STATE_VS_LOWPWR, ES705_POWER_STATE);
	else if (es705_priv.es705_power_state == ES705_SET_POWER_STATE_VS_LOWPWR) {
		es705_priv.power_control(ES705_SET_POWER_STATE_VS_OVERLAY, ES705_POWER_STATE);
		if (!es705_priv.voice_wakeup_enable) {
			es705_switch_route_config(5);
			es705_priv.power_control(ES705_SET_POWER_STATE_NORMAL, ES705_POWER_STATE);
			es705_read_write_power_control(0);
		}
	}

	return 0;
}

static int es705_get_vs_abort_value(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = abort_request;
	dev_dbg(es705_priv.dev, "%s(): abort request = %d\n",
		__func__, abort_request);
	return 0;
}

static int es705_put_vs_abort_value(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	abort_request = ucontrol->value.integer.value[0];
	dev_info(es705_priv.dev, "%s(): abort request = %d\n",
		__func__, abort_request);
	return 0;
}

static int es705_get_vs_make_internal_dump(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int es705_put_vs_make_internal_dump(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	u32 cmd_capture_mode[] = {0x9017e039, 0x90180001, 0xffffffff};
	int mode = ucontrol->value.integer.value[0];
	int rc;

	dev_info(es705_priv.dev, "%s(): start internal dump, mode=%d\n",
		__func__, mode);
	if (mode == 2)
		cmd_capture_mode[1] = 0x90180002;
	/* sensory pid capture mode = 1 for audio capture 960msec */
	rc = es705_write_block(&es705_priv, cmd_capture_mode);

	msleep(10);

	return rc;
}

static int es705_get_vs_make_external_dump(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

#define MAX_VS_RECORD_SIZE	65536 /* 64 KB */
static int es705_put_vs_make_external_dump(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	u32 cmd_rwdb_mode[] = {0x9017e021, 0x90180003, 0xffffffff};
	u32 cmd_get_capture_mode = 0x8016e039;
	u32 check_rspn;
	mm_segment_t old_fs;
	struct file *fp;
	int match = 1;
	char *dump_data;
	int rc;

	dev_info(es705_priv.dev, "%s(): start\n", __func__);

	/* check that 0xE039 = 255 */
	check_rspn = 0x801600ff;
	rc = es705_write_then_read(&es705_priv, &cmd_get_capture_mode,
			sizeof(cmd_get_capture_mode), &check_rspn, match);
	if (rc)
		dev_err(es705_priv.dev, "%s(): internal dump is failed rspn 0x%08x\n",
		__func__, check_rspn);
	else
		dev_info(es705_priv.dev, "%s(): internal dump is generated rspn 0x%08x\n",
		__func__, check_rspn);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	dump_data = kmalloc(MAX_VS_RECORD_SIZE, GFP_KERNEL);
	if (!dump_data) {
		dev_err(es705_priv.dev, "%s(): Memory allocation FAIL\n", __func__);
		rc = -ENOMEM;
		goto es705_put_vs_make_external_dump_exit;
	}

	fp = filp_open("/sdcard/vs_capture.bin", O_RDWR | O_CREAT, S_IRWXU);
	if (!fp) {
		dev_err(es705_priv.dev, "%s() : fail to open fp\n", __func__);
		rc = -ENOENT;
		set_fs(old_fs);
		goto es705_put_vs_make_external_dump_exit;
	}

	/* set the rwdbmode to 3 to upload the audio buffer contents via RDB */
	rc = es705_write_block(&es705_priv, cmd_rwdb_mode);

	es705_read_vs_data_block(&es705_priv, dump_data, MAX_VS_RECORD_SIZE);

	dev_info(es705_priv.dev, "%s(): pos %d write = %d bytes\n", __func__,
		 (int)fp->f_pos, es705_priv.vs_keyword_param_size);
	fp->f_pos = 0;
	vfs_write(fp, dump_data, es705_priv.vs_keyword_param_size, &fp->f_pos);
	filp_close(fp, NULL);

es705_put_vs_make_external_dump_exit:
	if (dump_data)
		kfree(dump_data);
	set_fs(old_fs);
	return rc;
}
#endif

#if !defined(SAMSUNG_ES705_FEATURE)
static int es705_get_wakeup_method_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es705_priv.wakeup_method;
	dev_dbg(es705_priv.dev, "%s(): es705 wakeup method by %s\n",
		__func__, es705_priv.wakeup_method ?
		"UART" : "wakeup GPIO");
	return 0;
}

static int es705_put_wakeup_method_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	es705_priv.wakeup_method = ucontrol->value.integer.value[0];
	dev_dbg(es705_priv.dev, "%s(): enable es705 wakeup by %s\n",
		__func__, es705_priv.wakeup_method ?
		"UART" : "wakeup GPIO");
	return 0;
}
#endif

static int es705_get_ns_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	dev_dbg(es705_priv.dev, "%s(): NS = %d\n",
		__func__, es705_priv.ns);
	ucontrol->value.enumerated.item[0] = es705_priv.ns;

	return 0;
}

static int es705_put_ns_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	int value = ucontrol->value.enumerated.item[0];
	int rc = 0;
	dev_dbg(es705_priv.dev, "%s(): NS = %d\n", __func__, value);

	es705_priv.ns = value;

	/* 0 = NS off, 1 = NS on*/
	if (value)
		rc = es705_write(NULL, ES705_PRESET,
			ES705_NS_ON_PRESET);
	else
		rc = es705_write(NULL, ES705_PRESET,
			ES705_NS_OFF_PRESET);

	return rc;
}

static int es705_get_sw_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int es705_put_sw_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	int value = ucontrol->value.enumerated.item[0];
	int rc = 0;
	dev_dbg(es705_priv.dev, "%s(): SW = %d\n", __func__, value);

	/* 0 = off, 1 = on*/
	if (value)
		rc = es705_write(NULL, ES705_PRESET,
			ES705_SW_ON_PRESET);
	else
		rc = es705_write(NULL, ES705_PRESET,
			ES705_SW_OFF_PRESET);

	return rc;
}

static int es705_get_sts_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int es705_put_sts_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	int value = ucontrol->value.enumerated.item[0];
	int rc = 0;
	dev_dbg(es705_priv.dev, "%s(): STS = %d\n", __func__, value);

	/* 0 = off, 1 = on*/
	if (value)
		rc = es705_write(NULL, ES705_PRESET,
			ES705_STS_ON_PRESET);
	else
		rc = es705_write(NULL, ES705_PRESET,
			ES705_STS_OFF_PRESET);

	return rc;
}

static int es705_get_rx_ns_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int es705_put_rx_ns_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	int value = ucontrol->value.enumerated.item[0];
	int rc = 0;
	dev_dbg(es705_priv.dev, "%s(): RX_NS = %d\n", __func__, value);

	/* 0 = off, 1 = on*/
	if (value)
		rc = es705_write(NULL, ES705_PRESET,
			ES705_RX_NS_ON_PRESET);
	else
		rc = es705_write(NULL, ES705_PRESET,
			ES705_RX_NS_OFF_PRESET);

	return rc;
}

static int es705_get_wnf_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int es705_put_wnf_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	int value = ucontrol->value.enumerated.item[0];
	int rc = 0;
	dev_dbg(es705_priv.dev, "%s(): WNF = %d\n", __func__, value);

	/* 0 = off, 1 = on */
	if (value)
		rc = es705_write(NULL, ES705_PRESET,
			ES705_WNF_ON_PRESET);
	else
		rc = es705_write(NULL, ES705_PRESET,
			ES705_WNF_OFF_PRESET);

	return rc;
}

static int es705_get_bwe_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int es705_put_bwe_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct es705_priv *es705 = &es705_priv;
	int value = ucontrol->value.enumerated.item[0];
	int rc = 0;

	dev_dbg(es705->dev, "%s(): BWE = %d\n", __func__, value);

	if (es705->pm_state != ES705_POWER_AWAKE) {
		dev_info(es705->dev,
				"%s(): can't bwe on/off, pm_state(%d)\n",
				__func__, es705->pm_state);
		return rc;
	}

	if (es705->current_bwe == value) {
		dev_info(es705->dev, "%s(): Avoid duplication value (%d)\n", __func__, value);
		return 0;
	}	

	if (network_type == WIDE_BAND) {
		dev_dbg(es705->dev, "%s(): WideBand does not need BWE feature\n", __func__);
		return rc;
	}
	
	/* 0 = off, 1 = on */
	if (value)
		rc = es705_write(NULL, ES705_PRESET,
			ES705_BWE_ON_PRESET);
	else
		rc = es705_write(NULL, ES705_PRESET,
			ES705_BWE_OFF_PRESET);

	es705->current_bwe = value;
	return rc;
}

static int es705_get_avalon_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int es705_put_avalon_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	int value = ucontrol->value.enumerated.item[0];
	int rc = 0;
	dev_dbg(es705_priv.dev, "%s(): Avalon Wind Noise = %d\n",
		__func__, value);

	/* 0 = off, 1 = on */
	if (value)
		rc = es705_write(NULL, ES705_PRESET,
			ES705_AVALON_WN_ON_PRESET);
	else
		rc = es705_write(NULL, ES705_PRESET,
			ES705_AVALON_WN_OFF_PRESET);

	return rc;
}

static int es705_get_vbb_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int es705_put_vbb_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	int value = ucontrol->value.enumerated.item[0];
	int rc = 0;
	dev_dbg(es705_priv.dev, "%s(): Virtual Bass Boost = %d\n",
		__func__, value);

	/* 0 = off, 1 = on */
	if (value)
		rc = es705_write(NULL, ES705_PRESET,
			ES705_VBB_ON_PRESET);
	else
		rc = es705_write(NULL, ES705_PRESET,
			ES705_VBB_OFF_PRESET);

	return rc;
}

static int es705_get_aud_zoom(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	dev_dbg(es705_priv.dev, "%s(): Zoom = %d\n",
		__func__, es705_priv.zoom);
	ucontrol->value.enumerated.item[0] = es705_priv.zoom;

	return 0;
}

static int es705_put_aud_zoom(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	int value = ucontrol->value.enumerated.item[0];
	int rc = 0;
	dev_dbg(es705_priv.dev, "%s(): Zoom = %d\n", __func__, value);

	es705_priv.zoom = value;

	if (value == ES705_AUD_ZOOM_NARRATOR) {
		rc = es705_write(NULL, ES705_PRESET,
			ES705_AUD_ZOOM_PRESET);
		rc = es705_write(NULL, ES705_PRESET,
			ES705_AUD_ZOOM_NARRATOR_PRESET);
	} else if (value == ES705_AUD_ZOOM_SCENE) {
		rc = es705_write(NULL, ES705_PRESET,
			ES705_AUD_ZOOM_PRESET);
		rc = es705_write(NULL, ES705_PRESET,
			ES705_AUD_ZOOM_SCENE_PRESET);
	} else if (value == ES705_AUD_ZOOM_NARRATION) {
		rc = es705_write(NULL, ES705_PRESET,
			ES705_AUD_ZOOM_PRESET);
		rc = es705_write(NULL, ES705_PRESET,
			ES705_AUD_ZOOM_NARRATION_PRESET);
	} else
		rc = es705_write(NULL, ES705_PRESET, 0);

	return rc;
}

static int es705_get_veq_preset_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	return 	es705_priv.current_veq_preset;
}

static int es705_put_veq_preset_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct es705_priv *es705 = &es705_priv;
	int value = ucontrol->value.enumerated.item[0];
	int rc = 0;

	dev_dbg(es705->dev, "%s(): VEQ Preset = %d\n", __func__, value);

	if (es705->pm_state != ES705_POWER_AWAKE) {
		dev_info(es705->dev,
				"%s(): can't bwe on/off, pm_state(%d)\n",
				__func__, es705->pm_state);
		return rc;
	}

	if (es705->current_veq_preset == value) {
		dev_info(es705->dev, "%s(): Avoid duplication value (%d)\n", __func__, value);
		return 0;
	}

	/* 0 = off, 1 = on */
	if (value)
		rc = es705_write(NULL, ES705_PRESET,
						ES705_VEQ_ON_PRESET);
	else
		rc = es705_write(NULL, ES705_PRESET,
						ES705_VEQ_OFF_PRESET);

	es705->current_veq_preset = value;
	return rc;
}

/* Get for streming is not avaiable. Tinymix "set" method first executes get
 * and then put method. Thus dummy get method is implemented. */
static int es705_get_streaming_select(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.enumerated.item[0] = -1;

	return 0;
}

int es705_remote_route_enable(struct snd_soc_dai *dai)
{
	dev_dbg(es705_priv.dev, "%s():dai->name = %s dai->id = %d\n",
		__func__, dai->name, dai->id);

	switch (dai->id) {
	case ES705_SLIM_1_PB:
		return es705_priv.rx1_route_enable;
	case ES705_SLIM_1_CAP:
		return es705_priv.tx1_route_enable;
	case ES705_SLIM_2_PB:
		return es705_priv.rx2_route_enable;
	default:
		return 0;
	}
}
EXPORT_SYMBOL_GPL(es705_remote_route_enable);

int es705_put_veq_block(int volume)
{
	struct es705_priv *es705 = &es705_priv;

	u32 cmd;
	u32 resp;
	int ret;
	u8 veq_coeff_size = 0;
	u32 fin_resp;
	u8 count = 0;
	u8 *wdbp = NULL;
	u8 wr = 0;
	int max_retry_cnt = 10;

	if (es705->pm_state != ES705_POWER_AWAKE) {
		dev_info(es705->dev,
				"%s(): can't set veq block, pm_state(%d)\n",
				__func__, es705->pm_state);
		return 0;
	}

	if ((volume > (sizeof(veq_max_gains_nb) / sizeof(veq_max_gains_nb[0])) - 1) ||
		(volume < 0)) {
		dev_info(es705->dev, "%s(): Invalid volume (%d)\n", __func__, volume);
		return 0;
	}

	if ((es705->current_veq_preset == 0) ||
		(es705->current_veq == volume)) {
		dev_info(es705->dev, "%s(): veq off or avoid duplication value(%d)\n", __func__, volume);
		return 0;
	}

	mutex_lock(&es705->api_mutex);

	es705->current_veq = volume;

	/* VEQ Max Gain */
	cmd = 0xB017003d;
	cmd = cpu_to_le32(cmd);
	ret = es705->dev_write(es705, (char *)&cmd, 4);
	if (ret < 0) {
		dev_err(es705->dev, "%s(): write veq max gain cmd 0x%08x failed\n",
		    __func__, cmd);
		goto EXIT;
	}

	/* 0x00 ~ 0x0f(15dB) */
	if (network_type == WIDE_BAND)
		cmd = veq_max_gains_wb[volume];
	else
		cmd = veq_max_gains_nb[volume];

	if (cmd > 0x9018000f)
		cmd = 0x9018000f;

	cmd = cpu_to_le32(cmd);
	ret = es705->dev_write(es705, (char *)&cmd, 4);
	dev_dbg(es705->dev, "%s(): write veq max gain 0x%08x to volume (%d)\n",
		    __func__, cmd, volume);	
	if (ret < 0) {
		dev_err(es705->dev, "%s(): write veq max gain 0x%08x failed\n",
		    __func__, cmd);
		goto EXIT;
	}

	/* VEQ Noise Estimate Adj */
	cmd = 0xB0170025;
	cmd = cpu_to_le32(cmd);
	ret = es705->dev_write(es705, (char *)&cmd, 4);
	if (ret < 0) {
		dev_err(es705->dev, "%s(): write veq estimate adj 0x%08x failed\n",
		    __func__, cmd);
		goto EXIT;
	}

	/* 0x00 ~ 0x1e(30dB) */
	if (network_type == WIDE_BAND)
		cmd = veq_noise_estimate_adjs_wb[volume];
	else
		cmd = veq_noise_estimate_adjs_nb[volume];
	if (cmd > 0x9018001e) cmd = 0x9018001e;
	cmd = cpu_to_le32(cmd);
	ret = es705->dev_write(es705, (char *)&cmd, 4);
	dev_dbg(es705->dev, "%s(): write veq estimate adj 0x%08x to volume (%d)\n",
		    __func__, cmd, volume);
	if (ret < 0) {
		dev_err(es705->dev, "%s(): write veq estimate adj 0x%08x failed\n",
		    __func__, cmd);
		goto EXIT;
	}

	/* VEQ Coefficients Filter */
	if (network_type == WIDE_BAND) {
		veq_coeff_size = 0x4A;
		wdbp = (char *)&veq_coefficients_wb[volume][0];
	}
	else {
		veq_coeff_size = 0x3E;
		wdbp = (char *)&veq_coefficients_nb[volume][0];
	}
	cmd = 0x802f0000 | (veq_coeff_size & 0xffff);
	cmd = cpu_to_le32(cmd);
	ret = es705->dev_write(es705, (char *)&cmd, 4);
	dev_dbg(es705->dev, "%s(): write veq coeff size 0x%08x\n",
		    __func__, cmd);
	if (ret < 0) {
		dev_err(es705->dev, "%s(): write veq coeff size 0x%08x failed\n",
		    __func__, cmd);
		goto EXIT;
	}

	usleep_range(10000, 10000);

	do {
		ret = es705->dev_read(es705, (char *)&resp,
				ES705_READ_VE_WIDTH);
		count++;
		usleep_range(2000, 2000);
	} while (resp != cmd && count < max_retry_cnt);

	if (resp != cmd) {
		dev_err(es705->dev, "%s(): error writing veq coeff size, resp is 0x%08x\n",
				__func__, resp);
		goto EXIT;
	}

	while (wr < veq_coeff_size) {
		int sz = min(veq_coeff_size - wr, ES705_WRITE_VE_WIDTH);

		if (sz < ES705_WRITE_VE_WIDTH) {
			cmd = 0;
			count = 0;
			while (sz>0) {
				cmd = cmd << 8;
				cmd = cmd | wdbp[wr++];
				sz--;
				count++;
			}
			cmd = cmd << ((ES705_WRITE_VE_WIDTH-count)*8);
		}
		else {
			cmd = *((int *)(wdbp+wr));
			cmd = cpu_to_be32(cmd);
	        }
		es705->dev_write(es705, (char *)&cmd, ES705_WRITE_VE_WIDTH);
		wr += sz;
	}

	usleep_range(10000, 10000);

	count = 0;
	do {
		fin_resp = 0xFFFFFFFF;
		ret = es705->dev_read(es705, (char *)&fin_resp,
				ES705_READ_VE_WIDTH);
		count++;
		usleep_range(2000, 2000);
	} while (fin_resp != 0x802f0000 && count < max_retry_cnt);

	if (fin_resp != 0x802f0000) {
		dev_err(es705->dev, "%s(): error writing veq coeff block, resp is 0x%08x\n",
				__func__, fin_resp);
		goto EXIT;
	}

	dev_info(es705->dev, "%s(): success\n", __func__);
	
EXIT:
	mutex_unlock(&es705->api_mutex);
	if (ret != 0)
		dev_err(es705->dev, "%s(): failed ret=%d\n",
			__func__, ret);
	return ret;
}
EXPORT_SYMBOL_GPL(es705_put_veq_block);

static int es705_put_internal_route_config(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	es705_switch_route_config(ucontrol->value.integer.value[0]);

	return 0;
}

static int es705_get_internal_route_config(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	struct es705_priv *es705 = &es705_priv;

	ucontrol->value.integer.value[0] = es705->internal_route_num;

	return 0;
}

#if defined(PREVENT_CALL_MUTE_WHEN_SWITCH_NB_AND_WB)
static void es705_restore_bwe_veq(void)
{
	struct es705_priv *es705 = &es705_priv;
	int rc;
	static int latest_bwe = -1;
	static int latest_veq = -1;
	static int latest_veq_preset = -1;
	
	dev_info(es705->dev, 
			"%s(): latest_bwe : %d, current_bwe : %d\n",
			__func__, latest_bwe, es705->current_bwe);

	dev_info(es705->dev, 
			"%s(): latest_veq : %d, current_veq : %d\n",
			__func__, latest_veq, es705->current_veq);

	dev_info(es705->dev, 
			"%s(): latest_veq_preset : %d, current_veq_preset : %d\n",
			__func__, latest_veq_preset, es705->current_veq_preset);

	/* Restore BWE */
	if (latest_bwe != es705->current_bwe) {
		latest_bwe = es705->current_bwe;
		if (network_type == NARROW_BAND) {
			/* 0 = off, 1 = on */
			if (es705->current_bwe)
				rc = es705_write(NULL, ES705_PRESET,
								ES705_BWE_ON_PRESET);
			else
				rc = es705_write(NULL, ES705_PRESET,
								ES705_BWE_OFF_PRESET);
		}
	}


	/* Restore VEQ */
	if (latest_veq != es705->current_veq) {
		latest_veq = es705->current_veq;
		es705_put_veq_block(es705->current_veq);
	}

	if (latest_veq_preset != es705->current_veq_preset) {
		latest_veq_preset = es705->current_veq_preset;
		/* 0 = off, 1 = on */
		if (es705->current_veq_preset)
			rc = es705_write(NULL, ES705_PRESET,
							ES705_VEQ_ON_PRESET);
		else
			rc = es705_write(NULL, ES705_PRESET,
							ES705_VEQ_OFF_PRESET);
	}
}
#endif

static int es705_put_network_type(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct es705_priv *es705 = &es705_priv;

	dev_info(es705->dev, "%s():network type = %ld pm_state = %d\n",
		__func__, ucontrol->value.integer.value[0], es705->pm_state);

	if (ucontrol->value.integer.value[0] == WIDE_BAND)
		network_type = WIDE_BAND;
	else
		network_type = NARROW_BAND;

	mutex_lock(&es705->pm_mutex);
	if (es705->pm_state == ES705_POWER_AWAKE) {
		es705_switch_route_config(es705->internal_route_num);
#if defined(PREVENT_CALL_MUTE_WHEN_SWITCH_NB_AND_WB)
		es705_restore_bwe_veq();
#endif
	}
	mutex_unlock(&es705->pm_mutex);
 
	return 0;
}

static int es705_get_network_type(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = network_type;

	return 0;
}

#if defined(FORCED_REROUTE_PRESET)
static void es705_forced_reroute(struct work_struct *w)
{
	struct es705_priv *es705 = &es705_priv;
	int rc = 0;
	int vol = 0;
	dev_info(es705->dev, "%s(): to %ld %d\n", __func__, es705->internal_route_num, extra_vol_onoff);

	rc = es705_write_block(es705, 
					&es705_route_configs[es705->internal_route_num][0]);

	if (extra_vol_onoff) {
		rc = es705_write(NULL, ES705_PRESET, 	ES705_VEQ_OFF_PRESET);
		
	} else {
		vol = es705->current_veq;
		es705->current_veq = -1;	
		es705->current_veq_preset = 1;

		es705_put_veq_block(vol);		
	}
	extra_vol_onoff = 0;
}

static int es705_forced_reroute_w(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct es705_priv *es705 = &es705_priv;

	if (delayed_work_pending(&es705->forced_reroute_work))
			cancel_delayed_work_sync(&es705->forced_reroute_work);

	dev_info(es705->dev, "%s(): put work task workqueue after delay(200ms)\n ", __func__);
	schedule_delayed_work(&es705->forced_reroute_work, msecs_to_jiffies(500));
	extra_vol_onoff = ucontrol->value.integer.value[0];

	return 0;
}
#endif

static int es705_put_internal_route(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	dev_info(es705_priv.dev, "%s : put internal route = %ld\n", __func__, ucontrol->value.integer.value[0]);
	es705_switch_route(ucontrol->value.integer.value[0]);
	return 0;
}

static int es705_get_internal_route(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct es705_priv *es705 = &es705_priv;

	ucontrol->value.integer.value[0] = es705->internal_route_num;

	return 0;
}

static int es705_put_internal_rate(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct es705_priv *es705 = &es705_priv;
	const u32 *rate_macro = NULL;
	int rc = 0;

	dev_dbg(es705->dev, "%s():es705->internal_rate = %d ucontrol = %d\n",
		__func__, (int)es705->internal_rate,
		(int)ucontrol->value.enumerated.item[0]);

	switch (ucontrol->value.enumerated.item[0]) {
	case RATE_NB:
		rate_macro = es705_route_config[es705->internal_route_num].nb;
		break;
	case RATE_WB:
		rate_macro = es705_route_config[es705->internal_route_num].wb;
		break;
	case RATE_SWB:
		rate_macro = es705_route_config[es705->internal_route_num].swb;
		break;
	case RATE_FB:
		rate_macro = es705_route_config[es705->internal_route_num].fb;
		break;
	default:
		break;
	}

	if (!rate_macro) {
		dev_err(es705->dev, "%s(): internal rate, %d, out of range\n",
			__func__, ucontrol->value.enumerated.item[0]);
		return -EINVAL;
	}

	es705->internal_rate = ucontrol->value.enumerated.item[0];
	rc = es705_write_block(es705, rate_macro);

	return rc;
}

static int es705_get_internal_rate(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct es705_priv *es705 = &es705_priv;

	ucontrol->value.enumerated.item[0] = es705->internal_rate;
	dev_dbg(es705->dev, "%s():es705->internal_rate = %d ucontrol = %d\n",
		__func__, (int)es705->internal_rate,
		(int)ucontrol->value.enumerated.item[0]);

	return 0;
}

static int es705_put_preset_value(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc = 0;

	value = ucontrol->value.integer.value[0];

	rc = es705_write(NULL, reg, value);
	if (rc) {
		dev_err(es705_priv.dev, "%s(): Set Preset failed\n",
			__func__);
		return rc;
	}

	es705_priv.preset = value;

	return rc;
}

static int es705_get_preset_value(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es705_priv.preset;

	return 0;
}

static int es705_get_audio_custom_profile(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int es705_put_audio_custom_profile(struct snd_kcontrol *kcontrol,
					  struct snd_ctl_elem_value *ucontrol)
{
	int index = ucontrol->value.integer.value[0];

	if (index < ES705_CUSTOMER_PROFILE_MAX)
		es705_write_block(&es705_priv,
				  &es705_audio_custom_profiles[index][0]);
	return 0;
}

static int es705_ap_put_tx1_ch_cnt(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	es705_priv.ap_tx1_ch_cnt = ucontrol->value.enumerated.item[0] + 1;
	pr_info("%s : cnt = %d\n", __func__, es705_priv.ap_tx1_ch_cnt);
	return 0;
}

static int es705_ap_get_tx1_ch_cnt(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct es705_priv *es705 = &es705_priv;

	ucontrol->value.enumerated.item[0] = es705->ap_tx1_ch_cnt - 1;

	return 0;
}

static const char * const es705_ap_tx1_ch_cnt_texts[] = {
	"One", "Two", "Three"
};
static const struct soc_enum es705_ap_tx1_ch_cnt_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0,
			ARRAY_SIZE(es705_ap_tx1_ch_cnt_texts),
			es705_ap_tx1_ch_cnt_texts);

static const char * const es705_vs_power_state_texts[] = {
	"None", "Sleep", "MP_Sleep", "MP_Cmd", "Normal", "Overlay", "Low_Power"
};
static const struct soc_enum es705_vs_power_state_enum =
	SOC_ENUM_SINGLE(ES705_POWER_STATE, 0,
			ARRAY_SIZE(es705_vs_power_state_texts),
			es705_vs_power_state_texts);

/* generic gain translation */
static int es705_index_to_gain(int min, int step, int index)
{
	return	min + (step * index);
}
static int es705_gain_to_index(int min, int step, int gain)
{
	return	(gain - min) / step;
}

/* dereverb gain */
static int es705_put_dereverb_gain_value(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc = 0;

	if (ucontrol->value.integer.value[0] <= 12) {
		value = es705_index_to_gain(-12, 1, ucontrol->value.integer.value[0]);
		rc = es705_write(NULL, reg, value);
	}

	return rc;
}

static int es705_get_dereverb_gain_value(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value=0;

	if (es705_priv.rx1_route_enable ||
		es705_priv.tx1_route_enable ||
		es705_priv.rx2_route_enable) {
	value = es705_read(NULL, reg);
}
	ucontrol->value.integer.value[0] = es705_gain_to_index(-12, 1, value);
	return 0;
}

/* bwe high band gain */
static int es705_put_bwe_high_band_gain_value(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc = 0;

	if (ucontrol->value.integer.value[0] <= 30) {
		value = es705_index_to_gain(-10, 1, ucontrol->value.integer.value[0]);
		rc = es705_write(NULL, reg, value);
	}

	return 0;
}

static int es705_get_bwe_high_band_gain_value(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value=0;

	if (es705_priv.rx1_route_enable ||
		es705_priv.tx1_route_enable ||
		es705_priv.rx2_route_enable) {
	value = es705_read(NULL, reg);
}
	ucontrol->value.integer.value[0] = es705_gain_to_index(-10, 1, value);

	return 0;
}

/* bwe max snr */
static int es705_put_bwe_max_snr_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc = 0;

	if (ucontrol->value.integer.value[0] <= 70) {
		value = es705_index_to_gain(-20, 1, ucontrol->value.integer.value[0]);
		rc = es705_write(NULL, reg, value);
	}

	return 0;
}

static int es705_get_bwe_max_snr_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value=0;

	if (es705_priv.rx1_route_enable ||
		es705_priv.tx1_route_enable ||
		es705_priv.rx2_route_enable) {
	value = es705_read(NULL, reg);
}
	ucontrol->value.integer.value[0] = es705_gain_to_index(-20, 1, value);
	return 0;
}

static const char * const es705_mic_config_texts[] = {
	"CT 2-mic", "FT 2-mic", "DV 1-mic", "EXT 1-mic", "BT 1-mic",
	"CT ASR 2-mic", "FT ASR 2-mic", "EXT ASR 1-mic", "FT ASR 1-mic",
};
static const struct soc_enum es705_mic_config_enum =
	SOC_ENUM_SINGLE(ES705_MIC_CONFIG, 0,
			ARRAY_SIZE(es705_mic_config_texts),
			es705_mic_config_texts);

static const char * const es705_aec_mode_texts[] = {
	"Off", "On", "rsvrd2", "rsvrd3", "rsvrd4", "On half-duplex"
};
static const struct soc_enum es705_aec_mode_enum =
	SOC_ENUM_SINGLE(ES705_AEC_MODE, 0, ARRAY_SIZE(es705_aec_mode_texts),
			es705_aec_mode_texts);

static const char * const es705_algo_rates_text[] = {
	"fs=8khz", "fs=16khz", "fs=24khz", "fs=48khz", "fs=96khz", "fs=192khz"
};
static const struct soc_enum es705_algo_sample_rate_enum =
	SOC_ENUM_SINGLE(ES705_ALGO_SAMPLE_RATE, 0,
			ARRAY_SIZE(es705_algo_rates_text),
			es705_algo_rates_text);
static const struct soc_enum es705_algo_mix_rate_enum =
	SOC_ENUM_SINGLE(ES705_MIX_SAMPLE_RATE, 0,
			ARRAY_SIZE(es705_algo_rates_text),
			es705_algo_rates_text);

static const char * const es705_internal_rate_text[] = {
	"NB", "WB", "SWB", "FB"
};
static const struct soc_enum es705_internal_rate_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0,
			ARRAY_SIZE(es705_internal_rate_text),
			es705_internal_rate_text);

static const char * const es705_algorithms_text[] = {
	"None", "VP", "Two CHREC", "AUDIO", "Four CHPASS"
};
static const struct soc_enum es705_algorithms_enum =
	SOC_ENUM_SINGLE(ES705_ALGO_SAMPLE_RATE, 0,
			ARRAY_SIZE(es705_algorithms_text),
			es705_algorithms_text);
static const char * const es705_off_on_texts[] = {
	"Off", "On"
};
static const char * const es705_audio_zoom_texts[] = {
	"disabled", "Narrator", "Scene", "Narration"
};
static const struct soc_enum es705_veq_enable_enum =
	SOC_ENUM_SINGLE(ES705_VEQ_ENABLE, 0, ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_dereverb_enable_enum =
	SOC_ENUM_SINGLE(ES705_DEREVERB_ENABLE, 0,
			ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_bwe_enable_enum =
	SOC_ENUM_SINGLE(ES705_BWE_ENABLE, 0, ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_bwe_post_eq_enable_enum =
	SOC_ENUM_SINGLE(ES705_BWE_POST_EQ_ENABLE, 0,
			ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_algo_processing_enable_enum =
	SOC_ENUM_SINGLE(ES705_ALGO_PROCESSING, 0,
			ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_ns_enable_enum =
	SOC_ENUM_SINGLE(ES705_PRESET, 0, ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_audio_zoom_enum =
	SOC_ENUM_SINGLE(ES705_PRESET, 0, ARRAY_SIZE(es705_audio_zoom_texts),
			es705_audio_zoom_texts);
static const struct soc_enum es705_rx_enable_enum =
	SOC_ENUM_SINGLE(ES705_RX_ENABLE, 0, ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_sw_enable_enum =
	SOC_ENUM_SINGLE(ES705_PRESET, 0, ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_sts_enable_enum =
	SOC_ENUM_SINGLE(ES705_PRESET, 0, ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_rx_ns_enable_enum =
	SOC_ENUM_SINGLE(ES705_PRESET, 0, ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_wnf_enable_enum =
	SOC_ENUM_SINGLE(ES705_PRESET, 0, ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_bwe_preset_enable_enum =
	SOC_ENUM_SINGLE(ES705_PRESET, 0, ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_avalon_wn_enable_enum =
	SOC_ENUM_SINGLE(ES705_PRESET, 0, ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);
static const struct soc_enum es705_vbb_enable_enum =
	SOC_ENUM_SINGLE(ES705_PRESET, 0, ARRAY_SIZE(es705_off_on_texts),
			es705_off_on_texts);

static int es705_put_power_state_enum(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int es705_get_power_state_enum(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}
static const char * const es705_power_state_texts[] = {
	"Sleep", "Active"
};
static const struct soc_enum es705_power_state_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0,
			ARRAY_SIZE(es705_power_state_texts),
			es705_power_state_texts);

static int es705_get_vs_enable(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.enumerated.item[0] = es705_priv.vs_enable;
	return 0;
}
static int es705_put_vs_enable(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	es705_priv.vs_enable = ucontrol->value.enumerated.item[0];
	es705_priv.vs_streaming(&es705_priv);
	return 0;
}

static int es705_get_vs_wakeup_keyword(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.enumerated.item[0] = es705_priv.vs_wakeup_keyword;
	return 0;
}

static int es705_put_vs_wakeup_keyword(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	es705_priv.vs_wakeup_keyword = ucontrol->value.enumerated.item[0];
	return 0;
}


static int es705_put_vs_stored_keyword(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	int op;
	int ret;

	op = ucontrol->value.enumerated.item[0];
	dev_dbg(es705_priv.dev, "%s(): op=%d\n", __func__, op);

	ret = 0;
	switch (op) {
	case 0:
		dev_dbg(es705_priv.dev, "%s(): keyword params put...\n",
			__func__);
		if (es705_priv.rdb_wdb_open) {
			ret = es705_priv.rdb_wdb_open(&es705_priv);
			if (ret < 0) {
				dev_err(es705_priv.dev, "%s(): WDB UART open FAIL\n", __func__);
				break;
			}
		}
		ret = es705_write_vs_data_block(&es705_priv);
		if (es705_priv.rdb_wdb_close) {
			ret = es705_priv.rdb_wdb_close(&es705_priv);
			if (ret < 0)
				dev_err(es705_priv.dev, "%s(): WDB UART close FAIL\n", __func__);
		}
		break;
	case 1:
		dev_dbg(es705_priv.dev, "%s(): keyword params get...\n",
			__func__);
		ret = es705_read_vs_data_block(&es705_priv,
					       (char *)es705_priv.vs_keyword_param,
					       ES705_VS_KEYWORD_PARAM_MAX);
		break;
	case 2:
		dev_dbg(es705_priv.dev, "%s(): keyword params clear...\n",
			__func__);
		es705_priv.vs_keyword_param_size = 0;
		break;
	default:
		BUG_ON(0);
	};

	return ret;
}

/* Voice Sense Detection Sensitivity */
static
int es705_put_vs_detection_sensitivity(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc = 0;

	value = ucontrol->value.integer.value[0];
	dev_dbg(es705_priv.dev, "%s(): ucontrol = %ld value = %d\n",
		__func__, ucontrol->value.integer.value[0], value);

	rc = es705_write(NULL, reg, value);

	return rc;
}

static
int es705_get_vs_detection_sensitivity(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value=0;

	if (es705_priv.rx1_route_enable ||
		es705_priv.tx1_route_enable ||
		es705_priv.rx2_route_enable) {
	value = es705_read(NULL, reg);
}
	ucontrol->value.integer.value[0] = value;

	dev_dbg(es705_priv.dev, "%s(): value = %d ucontrol = %ld\n",
		__func__, value, ucontrol->value.integer.value[0]);

	return 0;
}

static
int es705_put_vad_sensitivity(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc = 0;

	value = ucontrol->value.integer.value[0];

	dev_dbg(es705_priv.dev, "%s(): ucontrol = %ld value = %d\n",
		__func__, ucontrol->value.integer.value[0], value);

	rc = es705_write(NULL, reg, value);

	return rc;
}

static
int es705_get_vad_sensitivity(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value=0;

	if (es705_priv.rx1_route_enable ||
		es705_priv.tx1_route_enable ||
		es705_priv.rx2_route_enable) {
	value = es705_read(NULL, reg);
}
	ucontrol->value.integer.value[0] = value;

	dev_dbg(es705_priv.dev, "%s(): value = %d ucontrol = %ld\n",
		__func__, value, ucontrol->value.integer.value[0]);

	return 0;
}

static const char * const es705_vs_wakeup_keyword_texts[] = {
	"Default", "One", "Two", "Three", "Four"
};
static const struct soc_enum es705_vs_wakeup_keyword_enum =
	SOC_ENUM_SINGLE(ES705_VOICE_SENSE_SET_KEYWORD, 0,
			ARRAY_SIZE(es705_vs_wakeup_keyword_texts),
			es705_vs_wakeup_keyword_texts);

static const char * const es705_vs_event_texts[] = {
	"No Event", "Codec Event", "VS Keyword Event",
};
static const struct soc_enum es705_vs_event_enum =
	SOC_ENUM_SINGLE(ES705_VOICE_SENSE_EVENT, 0,
			ARRAY_SIZE(es705_vs_event_texts),
			es705_vs_event_texts);

static const char * const es705_vs_training_status_texts[] = {
	"Training", "Done",
};
static const struct soc_enum es705_vs_training_status_enum =
	SOC_ENUM_SINGLE(ES705_VOICE_SENSE_TRAINING_STATUS, 0,
			ARRAY_SIZE(es705_vs_training_status_texts),
			es705_vs_training_status_texts);

static const char * const es705_vs_training_record_texts[] = {
	"Stop", "Start",
};


static const char * const es705_vs_stored_keyword_texts[] = {
	"Put", "Get", "Clear"
};

static const struct soc_enum es705_vs_stored_keyword_enum =
	SOC_ENUM_SINGLE(ES705_VS_STORED_KEYWORD, 0,
			ARRAY_SIZE(es705_vs_stored_keyword_texts),
			es705_vs_stored_keyword_texts);

static const struct soc_enum es705_vs_training_record_enum =
	SOC_ENUM_SINGLE(ES705_VOICE_SENSE_TRAINING_RECORD, 0,
			ARRAY_SIZE(es705_vs_training_record_texts),
			es705_vs_training_record_texts);

static const char * const es705_vs_training_mode_texts[] = {
	"Detect builtin keyword", "Train keyword", "Detect user keyword"
};

static const struct soc_enum es705_vs_training_mode_enum =
	SOC_ENUM_SINGLE(ES705_VOICE_SENSE_TRAINING_MODE, 0,
			ARRAY_SIZE(es705_vs_training_mode_texts),
			es705_vs_training_mode_texts);


static struct snd_kcontrol_new es705_digital_ext_snd_controls[] = {
	SOC_SINGLE_EXT("ES705 RX1 Enable", SND_SOC_NOPM, 0, 1, 0,
		       es705_get_rx1_route_enable_value,
		       es705_put_rx1_route_enable_value),
	SOC_SINGLE_EXT("ES705 TX1 Enable", SND_SOC_NOPM, 0, 1, 0,
		       es705_get_tx1_route_enable_value,
		       es705_put_tx1_route_enable_value),
	SOC_SINGLE_EXT("ES705 RX2 Enable", SND_SOC_NOPM, 0, 1, 0,
		       es705_get_rx2_route_enable_value,
		       es705_put_rx2_route_enable_value),
	SOC_ENUM_EXT("Mic Config", es705_mic_config_enum,
		     es705_get_control_enum, es705_put_control_enum),
	SOC_ENUM_EXT("AEC Mode", es705_aec_mode_enum,
		     es705_get_control_enum, es705_put_control_enum),
#if defined(SAMSUNG_ES705_FEATURE)
	SOC_ENUM_EXT("VEQ Enable", es705_veq_enable_enum,
		     es705_get_veq_preset_value, es705_put_veq_preset_value),
#else
	SOC_ENUM_EXT("VEQ Enable", es705_veq_enable_enum,
		     es705_get_control_enum, es705_put_control_enum),
#endif			   
	SOC_ENUM_EXT("Dereverb Enable", es705_dereverb_enable_enum,
		     es705_get_control_enum, es705_put_control_enum),
	SOC_SINGLE_EXT("Dereverb Gain",
		       ES705_DEREVERB_GAIN, 0, 100, 0,
		       es705_get_dereverb_gain_value, es705_put_dereverb_gain_value),
	SOC_ENUM_EXT("BWE Enable", es705_bwe_enable_enum,
		     es705_get_control_enum, es705_put_control_enum),
	SOC_SINGLE_EXT("BWE High Band Gain",
		       ES705_BWE_HIGH_BAND_GAIN, 0, 100, 0,
		       es705_get_bwe_high_band_gain_value,
		       es705_put_bwe_high_band_gain_value),
	SOC_SINGLE_EXT("BWE Max SNR",
		       ES705_BWE_MAX_SNR, 0, 100, 0,
		       es705_get_bwe_max_snr_value, es705_put_bwe_max_snr_value),
	SOC_ENUM_EXT("BWE Post EQ Enable", es705_bwe_post_eq_enable_enum,
		     es705_get_control_enum, es705_put_control_enum),
	SOC_SINGLE_EXT("SLIMbus Link Multi Channel",
		       ES705_SLIMBUS_LINK_MULTI_CHANNEL, 0, 65535, 0,
		       es705_get_control_value, es705_put_control_value),
	SOC_ENUM_EXT("Set Power State", es705_power_state_enum,
		       es705_get_power_state_enum, es705_put_power_state_enum),
	SOC_ENUM_EXT("Algorithm Processing", es705_algo_processing_enable_enum,
		     es705_get_control_enum, es705_put_control_enum),
	SOC_ENUM_EXT("Algorithm Sample Rate", es705_algo_sample_rate_enum,
		     es705_get_control_enum, es705_put_control_enum),
	SOC_ENUM_EXT("Algorithm", es705_algorithms_enum,
		     es705_get_control_enum, es705_put_control_enum),
	SOC_ENUM_EXT("Mix Sample Rate", es705_algo_mix_rate_enum,
		     es705_get_control_enum, es705_put_control_enum),
	SOC_SINGLE_EXT("Internal Route",
		       SND_SOC_NOPM, 0, 100, 0, es705_get_internal_route,
		       es705_put_internal_route),
	SOC_ENUM_EXT("Internal Rate", es705_internal_rate_enum,
		      es705_get_internal_rate,
		      es705_put_internal_rate),
	SOC_SINGLE_EXT("Preset",
		       ES705_PRESET, 0, 65535, 0, es705_get_preset_value,
		       es705_put_preset_value),
	SOC_SINGLE_EXT("Audio Custom Profile",
		       SND_SOC_NOPM, 0, 100, 0, es705_get_audio_custom_profile,
		       es705_put_audio_custom_profile),
	SOC_ENUM_EXT("ES705-AP Tx Channels", es705_ap_tx1_ch_cnt_enum,
		     es705_ap_get_tx1_ch_cnt, es705_ap_put_tx1_ch_cnt),
	SOC_SINGLE_EXT("Voice Sense Enable",
		       ES705_VOICE_SENSE_ENABLE, 0, 1, 0,
		       es705_get_vs_enable, es705_put_vs_enable),
	SOC_ENUM_EXT("Voice Sense Set Wakeup Word",
		     es705_vs_wakeup_keyword_enum,
		     es705_get_vs_wakeup_keyword, es705_put_vs_wakeup_keyword),
	SOC_ENUM_EXT("Voice Sense Status",
		     es705_vs_event_enum,
		     es705_get_control_enum, NULL),
	SOC_ENUM_EXT("Voice Sense Training Mode",
			 es705_vs_training_mode_enum,
			 es705_get_control_enum, es705_put_control_enum),
	SOC_ENUM_EXT("Voice Sense Training Status",
		     es705_vs_training_status_enum,
		     es705_get_control_enum, NULL),
	SOC_ENUM_EXT("Voice Sense Training Record",
		     es705_vs_training_record_enum,
		     NULL, es705_put_control_enum),
	SOC_ENUM_EXT("Voice Sense Stored Keyword",
		     es705_vs_stored_keyword_enum,
		     NULL, es705_put_vs_stored_keyword),
	SOC_SINGLE_EXT("Voice Sense Detect Sensitivity",
			ES705_VOICE_SENSE_DETECTION_SENSITIVITY, 0, 10, 0,
			es705_get_vs_detection_sensitivity,
			es705_put_vs_detection_sensitivity),
	SOC_SINGLE_EXT("Voice Activity Detect Sensitivity",
			ES705_VOICE_ACTIVITY_DETECTION_SENSITIVITY, 0, 10, 0,
			es705_get_vad_sensitivity,
			es705_put_vad_sensitivity),
	SOC_ENUM_EXT("ES705 Power State", es705_vs_power_state_enum,
		     es705_get_power_control_enum,
		     es705_put_power_control_enum),
	SOC_ENUM_EXT("Noise Suppression", es705_ns_enable_enum,
		       es705_get_ns_value,
		       es705_put_ns_value),
	SOC_ENUM_EXT("Audio Zoom", es705_audio_zoom_enum,
		       es705_get_aud_zoom,
		       es705_put_aud_zoom),
	SOC_SINGLE_EXT("Enable/Disable Streaming PATH/Endpoint",
		       ES705_FE_STREAMING, 0, 65535, 0,
		       es705_get_streaming_select,
		       es705_put_control_value),
	SOC_ENUM_EXT("RX Enable", es705_rx_enable_enum,
		       es705_get_control_enum,
		       es705_put_control_enum),
	SOC_ENUM_EXT("Stereo Widening", es705_sw_enable_enum,
		       es705_get_sw_value,
		       es705_put_sw_value),
	SOC_ENUM_EXT("Speech Time Stretching", es705_sts_enable_enum,
			   es705_get_sts_value,
			   es705_put_sts_value),
	SOC_ENUM_EXT("RX Noise Suppression", es705_rx_ns_enable_enum,
			   es705_get_rx_ns_value,
			   es705_put_rx_ns_value),
	SOC_ENUM_EXT("Wind Noise Filter", es705_wnf_enable_enum,
			   es705_get_wnf_value,
			   es705_put_wnf_value),
	SOC_ENUM_EXT("BWE Preset", es705_bwe_preset_enable_enum,
			   es705_get_bwe_value,
			   es705_put_bwe_value),
	SOC_ENUM_EXT("AVALON Wind Noise", es705_avalon_wn_enable_enum,
			   es705_get_avalon_value,
			   es705_put_avalon_value),
	SOC_ENUM_EXT("Virtual Bass Boost", es705_vbb_enable_enum,
			   es705_get_vbb_value,
			   es705_put_vbb_value),
	SOC_SINGLE_EXT("UART FW Download Rate", SND_SOC_NOPM,
			0, 3, 0,
			es705_get_uart_fw_download_rate,
			es705_put_uart_fw_download_rate),
	SOC_SINGLE_EXT("Voice Sense Stream Enable", ES705_VS_STREAM_ENABLE,
		       0, 1, 0,
		       es705_get_vs_stream_enable, es705_put_vs_stream_enable),
#if defined(FORCED_REROUTE_PRESET)
	SOC_SINGLE_EXT("ES705 Reroute", SND_SOC_NOPM, 0, 1, 0,
		   NULL, es705_forced_reroute_w),
#endif
#if defined(SAMSUNG_ES705_FEATURE)
	SOC_SINGLE_EXT("ES705 Voice Wakeup Enable", SND_SOC_NOPM, 0, 2, 0,
		       es705_get_voice_wakeup_enable_value,
		       es705_put_voice_wakeup_enable_value),
	SOC_SINGLE_EXT("ES705 VS Abort", SND_SOC_NOPM, 0, 1, 0,
		       es705_get_vs_abort_value,
		       es705_put_vs_abort_value),
	SOC_SINGLE_EXT("ES705 VS Make Internal Dump", SND_SOC_NOPM, 0, 2, 0,
		       es705_get_vs_make_internal_dump,
		       es705_put_vs_make_internal_dump),
	SOC_SINGLE_EXT("ES705 VS Make External Dump", SND_SOC_NOPM, 0, 1, 0,
		       es705_get_vs_make_external_dump,
		       es705_put_vs_make_external_dump),
	SOC_SINGLE_EXT("ES705 Voice LPM Enable", SND_SOC_NOPM, 0, 1, 0,
		       es705_get_voice_lpm_enable_value,
		       es705_put_voice_lpm_enable_value),
	SOC_SINGLE_EXT("Internal Route Config", SND_SOC_NOPM, 0, 100, 0,
				es705_get_internal_route_config,
				es705_put_internal_route_config),
	SOC_SINGLE_EXT("Current Network Type", SND_SOC_NOPM, 0, 1, 0,
				es705_get_network_type,
				es705_put_network_type)
#endif
#if !defined(SAMSUNG_ES705_FEATURE)
	SOC_SINGLE_EXT("ES705 Wakeup Method", SND_SOC_NOPM, 0, 1, 0,
			es705_get_wakeup_method_value,
			es705_put_wakeup_method_value),
#endif
};

static int es705_set_bias_level(struct snd_soc_codec *codec,
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

#define ES705_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
			SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
			SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000 |\
			SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_192000)
#define ES705_SLIMBUS_RATES (SNDRV_PCM_RATE_48000)

#define ES705_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE |\
			SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE |\
			SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S32_BE)
#define ES705_SLIMBUS_FORMATS (SNDRV_PCM_FMTBIT_S16_LE |\
			SNDRV_PCM_FMTBIT_S16_BE)

struct snd_soc_dai_driver es705_dai[] = {
	{
		.name = "es705-slim-rx1",
		.id = ES705_SLIM_1_PB,
		.playback = {
			.stream_name = "SLIM_PORT-1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES705_SLIMBUS_RATES,
			.formats = ES705_SLIMBUS_FORMATS,
		},
		.ops = &es705_slim_port_dai_ops,
	},
	{
		.name = "es705-slim-tx1",
		.id = ES705_SLIM_1_CAP,
		.capture = {
			.stream_name = "SLIM_PORT-1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES705_SLIMBUS_RATES,
			.formats = ES705_SLIMBUS_FORMATS,
		},
		.ops = &es705_slim_port_dai_ops,
	},
	{
		.name = "es705-slim-rx2",
		.id = ES705_SLIM_2_PB,
		.playback = {
			.stream_name = "SLIM_PORT-2 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES705_SLIMBUS_RATES,
			.formats = ES705_SLIMBUS_FORMATS,
		},
		.ops = &es705_slim_port_dai_ops,
	},
	{
		.name = "es705-slim-tx2",
		.id = ES705_SLIM_2_CAP,
		.capture = {
			.stream_name = "SLIM_PORT-2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES705_SLIMBUS_RATES,
			.formats = ES705_SLIMBUS_FORMATS,
		},
		.ops = &es705_slim_port_dai_ops,
	},
	{
		.name = "es705-slim-rx3",
		.id = ES705_SLIM_3_PB,
		.playback = {
			.stream_name = "SLIM_PORT-3 Playback",
			.channels_min = 1,
			.channels_max = 4,
			.rates = ES705_SLIMBUS_RATES,
			.formats = ES705_SLIMBUS_FORMATS,
		},
		.ops = &es705_slim_port_dai_ops,
	},
	{
		.name = "es705-slim-tx3",
		.id = ES705_SLIM_3_CAP,
		.capture = {
			.stream_name = "SLIM_PORT-3 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES705_SLIMBUS_RATES,
			.formats = ES705_SLIMBUS_FORMATS,
		},
		.ops = &es705_slim_port_dai_ops,
	},
	{
		.name = "es705-porta",
		.id = ES705_I2S_PORTA,
		.playback = {
			.stream_name = "PORTA Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES705_RATES,
			.formats = ES705_FORMATS,
		},
		.capture = {
			.stream_name = "PORTA Capture",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES705_RATES,
			.formats = ES705_FORMATS,
		},
		.ops = &es705_i2s_port_dai_ops,
	},
	{
		.name = "es705-portb",
		.id = ES705_I2S_PORTB,
		.playback = {
			.stream_name = "PORTB Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES705_RATES,
			.formats = ES705_FORMATS,
		},
		.capture = {
			.stream_name = "PORTB Capture",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES705_RATES,
			.formats = ES705_FORMATS,
		},
		.ops = &es705_i2s_port_dai_ops,
	},
	{
		.name = "es705-portc",
		.id = ES705_I2S_PORTC,
		.playback = {
			.stream_name = "PORTC Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES705_RATES,
			.formats = ES705_FORMATS,
		},
		.capture = {
			.stream_name = "PORTC Capture",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES705_RATES,
			.formats = ES705_FORMATS,
		},
		.ops = &es705_i2s_port_dai_ops,
	},
	{
		.name = "es705-portd",
		.id = ES705_I2S_PORTD,
		.playback = {
			.stream_name = "PORTD Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES705_RATES,
			.formats = ES705_FORMATS,
		},
		.capture = {
			.stream_name = "PORTD Capture",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ES705_RATES,
			.formats = ES705_FORMATS,
		},
		.ops = &es705_i2s_port_dai_ops,
	},
};

#ifdef CONFIG_PM
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
static int es705_codec_suspend(struct snd_soc_codec *codec)
#else
static int es705_codec_suspend(struct snd_soc_codec *codec, pm_message_t state)
#endif
{
	struct es705_priv *es705 = snd_soc_codec_get_drvdata(codec);

	dev_dbg(es705->dev, "%s(): eS705 goes to sleep\n", __func__);
	es705_set_bias_level(codec, SND_SOC_BIAS_OFF);

	es705_sleep(es705);

	return 0;
}

static int es705_codec_resume(struct snd_soc_codec *codec)
{
	struct es705_priv *es705 = snd_soc_codec_get_drvdata(codec);

	dev_dbg(es705->dev, "%s(): eS705 waking up\n", __func__);
	es705_wakeup(es705);

	es705_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}
#else
#define es705_codec_suspend NULL
#define es705_codec_resume NULL
#endif

int es705_remote_add_codec_controls(struct snd_soc_codec *codec)
{
	int rc;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	rc = snd_soc_add_codec_controls(codec, es705_digital_ext_snd_controls,
				ARRAY_SIZE(es705_digital_ext_snd_controls));
#else
	rc = snd_soc_add_controls(codec, es705_digital_ext_snd_controls,
				ARRAY_SIZE(es705_digital_ext_snd_controls));
#endif
	if (rc)
		dev_err(codec->dev, "%s(): es705_digital_ext_snd_controls failed\n", __func__);

	return rc;
}

static int es705_codec_probe(struct snd_soc_codec *codec)
{
	struct es705_priv *es705 = snd_soc_codec_get_drvdata(codec);

	dev_dbg(codec->dev, "%s()\n", __func__);
	es705->codec = codec;

	codec->control_data = snd_soc_codec_get_drvdata(codec);

	es705_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	/*
	rc = snd_soc_add_controls(codec, es705_digital_snd_controls,
					ARRAY_SIZE(es705_digital_snd_controls));
	if (rc)
		dev_err(codec->dev, "%s(): es705_digital_snd_controls failed\n", __func__);
	*/

	return 0;
}

static int  es705_codec_remove(struct snd_soc_codec *codec)
{
	struct es705_priv *es705 = snd_soc_codec_get_drvdata(codec);

	es705_set_bias_level(codec, SND_SOC_BIAS_OFF);

	kfree(es705);

	return 0;
}

struct snd_soc_codec_driver soc_codec_dev_es705 = {
	.probe =	es705_codec_probe,
	.remove =	es705_codec_remove,
	.suspend =	es705_codec_suspend,
	.resume =	es705_codec_resume,
	.read =		es705_read,
	.write =	es705_write,
	.set_bias_level =	es705_set_bias_level,
};

static void es705_event_status(struct es705_priv *es705)
{
	int rc = 0;
	const u32 vs_get_key_word_status = 0x806D0000;
	u32 rspn;
	int match = 0;

	if (es705->es705_power_state == ES705_SET_POWER_STATE_VS_LOWPWR) {
		rc = es705_wakeup(es705);
		if (rc) {
			dev_err(es705->dev, "%s(): Get VS Status Fail\n",
				__func__);
			return;
	} else {
		/* wait VS status readiness */
		msleep(50);
	}
	}

	cpu_to_le32s(&vs_get_key_word_status);
	rc = es705_write_then_read(es705, &vs_get_key_word_status,
					sizeof(vs_get_key_word_status),
					&rspn, match);
	if (rc) {
		dev_err(es705->dev, "%s(): Get VS Status Fail\n", __func__);
		return;
	}

	if (es705->mode == VOICESENSE) {
#if !defined(SAMSUNG_ES705_FEATURE)
		u32 cmd = (ES705_SET_POWER_STATE << 16) |
				ES705_SET_POWER_STATE_NORMAL;
#endif
		le32_to_cpus(&rspn);
		rspn = rspn & 0xFFFF;
		/* Check VS detection status. */
		dev_info(es705->dev, "%s(): VS status 0x%04x\n",
			__func__, rspn);
		if ((rspn & 0x00FF) == KW_DETECTED) {
			dev_info(es705->dev, "%s(): Generate VS keyword detected event to User space\n",
				__func__);
			es705->vs_get_event = KW_DETECTED;
			es705_vs_event(es705);
			dev_info(es705_priv.dev, "%s(): VS keyword detected\n",
				__func__);
#if !defined(SAMSUNG_ES705_FEATURE)
			/* Switch FW to STANDARD */
			es705_cmd(es705, cmd);
			msleep(20);
#endif
			es705->pm_state = ES705_POWER_AWAKE;
#if defined(SAMSUNG_ES705_FEATURE)
			es705_priv.power_control(ES705_SET_POWER_STATE_VS_OVERLAY, ES705_POWER_STATE);
#endif
		}
	}
}

irqreturn_t es705_irq_event(int irq, void *irq_data)
{
	struct es705_priv *es705 = (struct es705_priv *)irq_data;
	u32 cmd_stop[] = {0x9017e000, 0x90180000, 0xffffffff};

	mutex_lock(&es705->api_mutex);
	dev_info(es705->dev, "%s(): %s mode, Interrupt event",
		__func__, esxxx_mode[es705->mode]);
	/* Get Event status, reset Interrupt */
	es705_event_status(es705);
	mutex_unlock(&es705->api_mutex);
#if defined(SAMSUNG_ES705_FEATURE)
	es705_write_block(&es705_priv, cmd_stop);
#endif
	return IRQ_HANDLED;
}

static int register_snd_soc(struct es705_priv *priv)
{
	int rc;
	int i;
	int ch_cnt;
	struct slim_device *sbdev = priv->gen0_client;

	es705_init_slim_slave(sbdev);

	dev_dbg(&sbdev->dev, "%s(): name = %s\n", __func__, sbdev->name);
	rc = snd_soc_register_codec(&sbdev->dev, &soc_codec_dev_es705,
				    es705_dai, ARRAY_SIZE(es705_dai));
	dev_dbg(&sbdev->dev, "%s(): rc = snd_soc_regsiter_codec() = %d\n",
		__func__, rc);

	/* allocate ch_num array for each DAI */
	for (i = 0; i < ES705_NUM_CODEC_SLIM_DAIS; i++) {
		switch (es705_dai[i].id) {
		case ES705_SLIM_1_PB:
		case ES705_SLIM_2_PB:
		case ES705_SLIM_3_PB:
			ch_cnt = es705_dai[i].playback.channels_max;
			break;
		case ES705_SLIM_1_CAP:
		case ES705_SLIM_2_CAP:
		case ES705_SLIM_3_CAP:
			ch_cnt = es705_dai[i].capture.channels_max;
			break;
		default:
			continue;
		}
		es705_priv.dai[i].ch_num =
			kzalloc((ch_cnt * sizeof(unsigned int)), GFP_KERNEL);
	}

	es705_slim_map_channels(priv);
	return rc;
}

int es705_core_probe(struct device *dev)
{
	struct esxxx_platform_data *pdata = dev->platform_data;
	int rc = 0;
#ifdef ES705_VDDCORE_MAX77826
	struct regulator *es705_vdd_core = NULL;
#endif
	const char *fw_filename = "audience-es705-fw.bin";
#ifndef CONFIG_ARCH_MSM8226
	const char *vs_filename = "audience-es705-vs.bin";
#endif /* CONFIG_ARCH_MSM8226 */

	if (pdata == NULL) {
		dev_err(dev, "%s(): pdata is NULL", __func__);
		rc = -EIO;
		goto pdata_error;
	}

	es705_priv.dev = dev;
	es705_priv.pdata = pdata;
	es705_priv.fw_requested = 0;
#ifdef WDB_RDB_OVER_SLIMBUS
	es705_priv.rdb_wdb_open = NULL;
	es705_priv.rdb_wdb_close = NULL;
#else
	es705_priv.rdb_wdb_open = es705_uart_open;
	es705_priv.rdb_wdb_close = es705_uart_close;
#endif
	if (es705_priv.rdb_wdb_open && es705_priv.rdb_wdb_close) {
		es705_priv.rdb_read = es705_uart_read;
		es705_priv.wdb_write = es705_uart_write;
	} else {
		es705_priv.rdb_read = es705_priv.dev_read;
		es705_priv.wdb_write = es705_priv.dev_write;
	}

	es705_clk_init(&es705_priv);
#if defined(SAMSUNG_ES705_FEATURE)
	/*
	 * Set UART interface as a default for STANDARD FW
	 * download at boot time. And select Baud Rate 3MBPs
	 */
	es705_priv.uart_fw_download = es705_uart_fw_download;
	if (system_rev >= UART_DOWNLOAD_WAKEUP_HWREV) {
		es705_priv.uart_fw_download_rate = 3;
		es705_priv.use_uart_for_wakeup_gpio = 1;
	} else {
		es705_priv.uart_fw_download_rate = 0;
		es705_priv.use_uart_for_wakeup_gpio = 0;
	}
	/* Select UART eS705 Wakeup mechanism */
	es705_priv.uart_es705_wakeup = es705_uart_es705_wakeup;
	es705_priv.power_control = es705_power_control;
#else
        /*
         * Set default interface for STANDARD FW
         * download at boot time.
         */
        es705_priv.uart_fw_download = NULL;
        es705_priv.uart_fw_download_rate = 0;
        /* Select GPIO eS705 Wakeup mechanism */
	es705_priv.uart_es705_wakeup = NULL;
#endif
	es705_priv.uart_state = UART_CLOSE;
	es705_priv.sleep_delay = ES705_SLEEP_DEFAULT_DELAY;
#if defined(PREVENT_CALL_MUTE_WHEN_SWITCH_NB_AND_WB)
	es705_priv.reroute_delay = ES705_REROUTE_INV;
#endif
#if defined(SAMSUNG_ES705_FEATURE)	
	es705_priv.current_bwe = 0;
	es705_priv.current_veq = -1;
	es705_priv.current_veq_preset = 0;
#endif
	mutex_init(&es705_priv.api_mutex);
	mutex_init(&es705_priv.pm_mutex);
	mutex_init(&es705_priv.streaming_mutex);

	init_waitqueue_head(&es705_priv.stream_in_q);

	/* No keyword parameters available until set. */
	es705_priv.vs_keyword_param_size = 0;

	rc = sysfs_create_group(&es705_priv.dev->kobj, &core_sysfs);
	if (rc) {
		dev_err(es705_priv.dev, "%s(): failed to create core sysfs entries: %d\n",
			__func__, rc);
	}

	INIT_DELAYED_WORK(&es705_priv.sleep_work, es705_delayed_sleep);
#if defined(PREVENT_CALL_MUTE_WHEN_SWITCH_NB_AND_WB)
	INIT_DELAYED_WORK(&es705_priv.reroute_work, es705_reroute);	
#endif
#if defined(FORCED_REROUTE_PRESET)
	INIT_DELAYED_WORK(&es705_priv.forced_reroute_work, es705_forced_reroute);	
#endif

#if defined(SAMSUNG_ES705_FEATURE)
	rc = es705_init_input_device(&es705_priv);
	if (rc < 0)
		goto init_input_device_error;
#endif
	rc = es705_gpio_init(&es705_priv);
	if (rc)
		goto gpio_init_error;

	rc = request_firmware((const struct firmware **)&es705_priv.standard,
			      fw_filename, es705_priv.dev);
	if (rc) {
		dev_err(es705_priv.dev, "%s(): request_firmware(%s) failed %d\n",
			__func__, fw_filename, rc);
		goto request_firmware_error;
	}

#ifndef CONFIG_ARCH_MSM8226
	rc = request_firmware((const struct firmware **)&es705_priv.vs,
			      vs_filename, es705_priv.dev);
	if (rc) {
		dev_err(es705_priv.dev, "%s(): request_firmware(%s) failed %d\n",
			__func__, vs_filename, rc);
		goto request_vs_firmware_error;
	}
#endif /* CONFIG_ARCH_MSM8226 */

#ifdef ES705_VDDCORE_MAX77826
#if defined(CONFIG_MACH_KACTIVELTE_KOR)
	es705_vdd_core = regulator_get(NULL, "max77826_ldo3");
#else
	es705_vdd_core = regulator_get(NULL, "max77826_ldo1");
#endif
	if (IS_ERR(es705_vdd_core)) {
		dev_err(dev, "%s(): es705 VDD CORE regulator_get fail\n", __func__);
		return rc;
	}
	regulator_set_voltage(es705_vdd_core, 1100000, 1100000);
	regulator_enable(es705_vdd_core);
	regulator_put(es705_vdd_core);
#endif

	if (pdata->esxxx_clk_cb) {
		pdata->esxxx_clk_cb(1);
		dev_info(es705_priv.dev,
				"%s(): external clock on\n", __func__);
		}

	usleep_range(5000, 5000);
	es705_gpio_reset(&es705_priv);

	es705_priv.pm_state = ES705_POWER_FW_LOAD;
	es705_priv.fw_requested = 1;

	return rc;

#ifndef CONFIG_ARCH_MSM8226
request_vs_firmware_error:
	release_firmware(es705_priv.standard);
#endif /* CONFIG_ARCH_MSM8226 */
request_firmware_error:
gpio_init_error:
#if defined(SAMSUNG_ES705_FEATURE)
init_input_device_error:
#endif
pdata_error:
	dev_dbg(es705_priv.dev, "%s(): exit with error\n", __func__);
	return rc;
}
EXPORT_SYMBOL_GPL(es705_core_probe);

static __init int es705_init(void)
{
	int rc = 0;

#if defined(CONFIG_SND_SOC_ES705_I2C)

	rc = es705_i2c_init(&es705_priv);

#elif defined(CONFIG_SND_SOC_ES705_SPI)

	rc = spi_register_driver(&es705_spi_driver);
	if (!rc) {
		pr_debug("%s() registered as SPI", __func__);
		es705_priv.intf = ES705_SPI_INTF;
	}
#else
	/* Bus specifc registration */
	/* FIXME: Temporary kludge until es705_bus_init abstraction
	 * is worked out */
#if !defined(CONFIG_SND_SOC_ES705_UART)
	rc = es705_bus_init(&es705_priv);
#else
	rc = es705_uart_bus_init(&es705_priv);
#endif

#endif

	if (rc) {
		pr_debug("Error registering Audience eS705 driver: %d\n", rc);
		goto INIT_ERR;
	}

#if !defined(CONFIG_SND_SOC_ES705_UART)
/* If CONFIG_SND_SOC_ES705_UART, UART probe will initialize char device
 * if a es705 is found */
	rc = es705_init_cdev(&es705_priv);
	if (rc) {
		pr_err("failed to initialize char device = %d\n", rc);
		goto INIT_ERR;
	}
#endif

INIT_ERR:
	return rc;
}
module_init(es705_init);

static __exit void es705_exit(void)
{
#if defined(SAMSUNG_ES705_FEATURE)
	es705_unregister_input_device(&es705_priv);
#endif
	if (es705_priv.fw_requested) {
		release_firmware(es705_priv.standard);
		release_firmware(es705_priv.vs);
	}
	es705_cleanup_cdev(&es705_priv);

#if defined(CONFIG_SND_SOC_ES705_I2C)
	i2c_del_driver(&es705_i2c_driver);
#else
	/* no support from QCOM to unregister
	 * slim_driver_unregister(&es705_slim_driver);
	 */
#endif
}
module_exit(es705_exit);


MODULE_DESCRIPTION("ASoC ES705 driver");
MODULE_AUTHOR("Greg Clemson <gclemson@audience.com>");
MODULE_AUTHOR("Genisim Tsilker <gtsilker@audience.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:es705-codec");
MODULE_FIRMWARE("audience-es705-fw.bin");
