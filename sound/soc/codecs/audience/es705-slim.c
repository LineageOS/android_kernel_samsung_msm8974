/*
 * es705-slim.c  --  Audience eS705 ALSA SoC Audio driver
 *
 * Copyright 2011 Audience, Inc.
 *
 * Author: Greg Clemson <gclemson@audience.com>
 *
 * Code Updates:
 *	Genisim Tsilker <gtsilker@audience.com>
 *              - Code refactoring
 *              - FW download functions update
 *              - Rewrite esxxx SLIMBus write / read functions
 *              - Add write_then_read function
 *              - Unify log messages format
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define SLIMBUS_VER_2

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
#include "es705.h"
#include "es705-platform.h"
#include "es705-uart-common.h"

#define ES705_SLIM_1_PB_MAX_CHANS	2
#define ES705_SLIM_1_CAP_MAX_CHANS	2
#define ES705_SLIM_2_PB_MAX_CHANS	2
#define ES705_SLIM_2_CAP_MAX_CHANS	2
#define ES705_SLIM_3_PB_MAX_CHANS	4
#define ES705_SLIM_3_CAP_MAX_CHANS	2

#define ES705_SLIM_1_PB_OFFSET	0
#define ES705_SLIM_2_PB_OFFSET	2
#define ES705_SLIM_3_PB_OFFSET	4
#define ES705_SLIM_1_CAP_OFFSET	0
#define ES705_SLIM_2_CAP_OFFSET	2
#define ES705_SLIM_3_CAP_OFFSET	4

/*
 * Delay for receiving response can be up to 20 ms.
 * To minimize waiting time, response is checking
 * up to 20 times with 1ms delay.
 */
#define MAX_SMB_TRIALS	3
#define MAX_WRITE_THEN_READ_TRIALS	20
#define SMB_DELAY	1000

#if defined(SAMSUNG_ES705_FEATURE)
static int es705_slim_rx_port_to_ch[ES705_SLIM_RX_PORTS] = {
#ifdef CONFIG_WCD9306_CODEC
		152, 153, 154, 155, 128, 129,
#else
	152, 153, 154, 155, 134, 135, 136, 137
#endif /* CONFIG_WCD9306_CODEC */

};
#else
static int es705_slim_rx_port_to_ch[ES705_SLIM_RX_PORTS] = {
	152, 153, 154, 155, 134, 135
};
#endif

#if defined(SLIMBUS_VER_2)
static int es705_slim_tx_port_to_ch[ES705_SLIM_TX_PORTS] = {
	156, 157, 144, 145, 144, 145
};
#else
static int es705_slim_tx_port_to_ch[ES705_SLIM_TX_PORTS] = {
	156, 157, 138, 139, 143, 144
};
#endif

static int es705_slim_be_id[ES705_NUM_CODEC_SLIM_DAIS] = {
	ES705_SLIM_2_CAP, /* for ES705_SLIM_1_PB tx from es705 */
	ES705_SLIM_3_PB, /* for ES705_SLIM_1_CAP rx to es705 */
	ES705_SLIM_3_CAP, /* for ES705_SLIM_2_PB tx from es705 */
	-1, /* for ES705_SLIM_2_CAP */
	-1, /* for ES705_SLIM_3_PB */
	-1, /* for ES705_SLIM_3_CAP */
};

#ifdef CONFIG_SND_SOC_ES704_TEMP
static int dev_selected;
#endif

#ifdef CONFIG_ARCH_MSM8226
static struct regulator* es705_ldo;
#endif /* CONFIG_ARCH_MSM8226 */

static void es705_alloc_slim_rx_chan(struct slim_device *sbdev);
static void es705_alloc_slim_tx_chan(struct slim_device *sbdev);
static int es705_cfg_slim_rx(struct slim_device *sbdev, unsigned int *ch_num,
			     unsigned int ch_cnt, unsigned int rate);
static int es705_cfg_slim_tx(struct slim_device *sbdev, unsigned int *ch_num,
			     unsigned int ch_cnt, unsigned int rate);
static int es705_close_slim_rx(struct slim_device *sbdev, unsigned int *ch_num,
			       unsigned int ch_cnt);
static int es705_close_slim_tx(struct slim_device *sbdev, unsigned int *ch_num,
			       unsigned int ch_cnt);

static int es705_rx_ch_num_to_idx(int ch_num)
{
	int i;
	int idx = -1;

	for (i = 0; i < ES705_SLIM_RX_PORTS; i++) {
		if (ch_num == es705_slim_rx_port_to_ch[i]) {
			idx = i;
			break;
		}
	}

	return idx;
}

static int es705_tx_ch_num_to_idx(int ch_num)
{
	int i;
	int idx = -1;

	for (i = 0; i < ES705_SLIM_TX_PORTS; i++) {
		if (ch_num == es705_slim_tx_port_to_ch[i]) {
			idx = i;
			break;
		}
	}

	return idx;
}

/* es705 -> codec - alsa playback function */
static int es705_codec_cfg_slim_tx(struct es705_priv *es705, int dai_id)
{
	struct slim_device *sbdev = es705->gen0_client;
	int rc;

	dev_dbg(&sbdev->dev, "%s(): dai_id = %d\n", __func__, dai_id);
	/* start slim channels associated with id */
	rc = es705_cfg_slim_tx(es705->gen0_client,
			       es705->dai[DAI_INDEX(dai_id)].ch_num,
			       es705->dai[DAI_INDEX(dai_id)].ch_tot,
			       es705->dai[DAI_INDEX(dai_id)].rate);

	return rc;
}

/* es705 <- codec - alsa capture function */
static int es705_codec_cfg_slim_rx(struct es705_priv *es705, int dai_id)
{
	struct slim_device *sbdev = es705->gen0_client;
	int rc;

	dev_dbg(&sbdev->dev, "%s(): dai_id = %d\n", __func__, dai_id);
	/* start slim channels associated with id */
	rc = es705_cfg_slim_rx(es705->gen0_client,
			       es705->dai[DAI_INDEX(dai_id)].ch_num,
			       es705->dai[DAI_INDEX(dai_id)].ch_tot,
			       es705->dai[DAI_INDEX(dai_id)].rate);

	return rc;
}

/* es705 -> codec - alsa playback function */
static int es705_codec_close_slim_tx(struct es705_priv *es705, int dai_id)
{
	struct slim_device *sbdev = es705->gen0_client;
	int rc;

	dev_dbg(&sbdev->dev, "%s(): dai_id = %d\n", __func__, dai_id);
	/* close slim channels associated with id */
	rc = es705_close_slim_tx(es705->gen0_client,
				 es705->dai[DAI_INDEX(dai_id)].ch_num,
				 es705->dai[DAI_INDEX(dai_id)].ch_tot);

	return rc;
}

/* es705 <- codec - alsa capture function */
static int es705_codec_close_slim_rx(struct es705_priv *es705, int dai_id)
{
	struct slim_device *sbdev = es705->gen0_client;
	int rc;

	dev_dbg(&sbdev->dev, "%s(): dai_id = %d\n", __func__, dai_id);
	/* close slim channels associated with id */
	rc = es705_close_slim_rx(es705->gen0_client,
				 es705->dai[DAI_INDEX(dai_id)].ch_num,
				 es705->dai[DAI_INDEX(dai_id)].ch_tot);

	return rc;
}

static void es705_alloc_slim_rx_chan(struct slim_device *sbdev)
{
	struct es705_priv *es705_priv = slim_get_devicedata(sbdev);
	struct es705_slim_ch *rx = es705_priv->slim_rx;
	int i;
	int port_id;

	for (i = 0; i < ES705_SLIM_RX_PORTS; i++) {
		port_id = i;
		rx[i].ch_num = es705_slim_rx_port_to_ch[i];
		slim_get_slaveport(sbdev->laddr, port_id, &rx[i].sph,
				   SLIM_SINK);
		slim_query_ch(sbdev, rx[i].ch_num, &rx[i].ch_h);
	}
}

static void es705_alloc_slim_tx_chan(struct slim_device *sbdev)
{
	struct es705_priv *es705_priv = slim_get_devicedata(sbdev);
	struct es705_slim_ch *tx = es705_priv->slim_tx;
	int i;
	int port_id;

	for (i = 0; i < ES705_SLIM_TX_PORTS; i++) {
		port_id = i + 10; /* ES705_SLIM_RX_PORTS; */
		tx[i].ch_num = es705_slim_tx_port_to_ch[i];
		slim_get_slaveport(sbdev->laddr, port_id, &tx[i].sph,
				   SLIM_SRC);
		slim_query_ch(sbdev, tx[i].ch_num, &tx[i].ch_h);
	}
}

static int es705_cfg_slim_rx(struct slim_device *sbdev, unsigned int *ch_num,
			     unsigned int ch_cnt, unsigned int rate)
{
	struct es705_priv *es705_priv = slim_get_devicedata(sbdev);
	struct es705_slim_ch *rx = es705_priv->slim_rx;
	u16 grph;
	u32 sph[ES705_SLIM_RX_PORTS] = {0};
	u16 ch_h[ES705_SLIM_RX_PORTS] = {0};
	struct slim_ch prop;
	int i;
	int idx;
	int rc = 0;

	dev_info(&sbdev->dev, "%s(): ch_cnt = %d, rate = %d\n",
		__func__, ch_cnt, rate);

	for (i = 0; i < ch_cnt; i++) {
		dev_dbg(&sbdev->dev, "%s(): ch_num = %d\n",
			__func__, ch_num[i]);
		idx = es705_rx_ch_num_to_idx(ch_num[i]);
		ch_h[i] = rx[idx].ch_h;
		sph[i] = rx[idx].sph;
	}

	prop.prot = SLIM_AUTO_ISO;
	prop.baser = SLIM_RATE_4000HZ;
	prop.dataf = SLIM_CH_DATAF_NOT_DEFINED;
	prop.auxf = SLIM_CH_AUXF_NOT_APPLICABLE;
	prop.ratem = (rate/4000);
	prop.sampleszbits = 16;

	rc = slim_define_ch(sbdev, &prop, ch_h, ch_cnt, true, &grph);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s(): slim_define_ch() failed: %d\n",
			__func__, rc);
		goto slim_define_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		rc = slim_connect_sink(sbdev, &sph[i], 1, ch_h[i]);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s(): slim_connect_sink() failed: %d\n",
				__func__, rc);
			goto slim_connect_sink_error;
		}
	}
	rc = slim_control_ch(sbdev, grph, SLIM_CH_ACTIVATE, true);
	if (rc < 0) {
		dev_err(&sbdev->dev,
			"%s(): slim_control_ch() failed: %d\n",
			__func__, rc);
		goto slim_control_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		dev_info(&sbdev->dev, "%s(): ch_num = %d\n",
			__func__, ch_num[i]);
		idx = es705_rx_ch_num_to_idx(ch_num[i]);
		rx[idx].grph = grph;
	}
	return rc;
slim_control_ch_error:
slim_connect_sink_error:
	es705_close_slim_rx(sbdev, ch_num, ch_cnt);
slim_define_ch_error:
	return rc;
}

static int es705_cfg_slim_tx(struct slim_device *sbdev, unsigned int *ch_num,
			     unsigned int ch_cnt, unsigned int rate)
{
	struct es705_priv *es705_priv = slim_get_devicedata(sbdev);
	struct es705_slim_ch *tx = es705_priv->slim_tx;
	u16 grph;
	u32 sph[ES705_SLIM_TX_PORTS] = {0};
	u16 ch_h[ES705_SLIM_TX_PORTS] = {0};
	struct slim_ch prop;
	int i;
	int idx;
	int rc;

	dev_dbg(&sbdev->dev, "%s(): ch_cnt = %d, rate = %d\n",
		__func__, ch_cnt, rate);

	for (i = 0; i < ch_cnt; i++) {
		dev_dbg(&sbdev->dev, "%s(): ch_num = %d\n",
			__func__, ch_num[i]);
		idx = es705_tx_ch_num_to_idx(ch_num[i]);
		ch_h[i] = tx[idx].ch_h;
		sph[i] = tx[idx].sph;
	}

	prop.prot = SLIM_AUTO_ISO;
	prop.baser = SLIM_RATE_4000HZ;
	prop.dataf = SLIM_CH_DATAF_NOT_DEFINED;
	prop.auxf = SLIM_CH_AUXF_NOT_APPLICABLE;
	prop.ratem = (rate/4000);
	prop.sampleszbits = 16;

	rc = slim_define_ch(sbdev, &prop, ch_h, ch_cnt, true, &grph);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s(): slim_define_ch() failed: %d\n",
			__func__, rc);
		goto slim_define_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		rc = slim_connect_src(sbdev, sph[i], ch_h[i]);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s(): slim_connect_src() failed: %d\n",
				__func__, rc);
			dev_err(&sbdev->dev, "%s(): ch_num[0] = %d\n",
				__func__, ch_num[0]);
			goto slim_connect_src_error;
		}
	}
	rc = slim_control_ch(sbdev, grph, SLIM_CH_ACTIVATE, true);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s(): slim_control_ch() failed: %d\n",
			__func__, rc);
		goto slim_control_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		dev_info(&sbdev->dev, "%s(): ch_num = %d\n",
			__func__, ch_num[i]);
		idx = es705_tx_ch_num_to_idx(ch_num[i]);
		tx[idx].grph = grph;
	}
	return rc;
slim_control_ch_error:
slim_connect_src_error:
	es705_close_slim_tx(sbdev, ch_num, ch_cnt);
slim_define_ch_error:
	return rc;
}

static int es705_close_slim_rx(struct slim_device *sbdev, unsigned int *ch_num,
			       unsigned int ch_cnt)
{
	struct es705_priv *es705_priv = slim_get_devicedata(sbdev);
	struct es705_slim_ch *rx = es705_priv->slim_rx;
	u16 grph = 0;
	u32 sph[ES705_SLIM_RX_PORTS] = {0};
	int i;
	int idx;
	int rc;

	dev_dbg(&sbdev->dev, "%s(): ch_cnt = %d\n", __func__, ch_cnt);

	for (i = 0; i < ch_cnt; i++) {
		dev_dbg(&sbdev->dev, "%s(): ch_num = %d\n",
			__func__, ch_num[i]);
		idx = es705_rx_ch_num_to_idx(ch_num[i]);
		sph[i] = rx[idx].sph;
		grph = rx[idx].grph;
	}

	rc = slim_control_ch(sbdev, grph, SLIM_CH_REMOVE, true);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s(): slim_control_ch() failed: %d\n",
			__func__, rc);
		goto slim_control_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		dev_dbg(&sbdev->dev, "%s(): ch_num = %d\n",
			__func__, ch_num[i]);
		idx = es705_rx_ch_num_to_idx(ch_num[i]);
		rx[idx].grph = 0;
	}
	rc = slim_disconnect_ports(sbdev, sph, ch_cnt);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s(): slim_disconnect_ports() failed: %d\n",
			__func__, rc);
	}
slim_control_ch_error:
	return rc;
}

static int es705_close_slim_tx(struct slim_device *sbdev, unsigned int *ch_num,
			       unsigned int ch_cnt)
{
	struct es705_priv *es705_priv = slim_get_devicedata(sbdev);
	struct es705_slim_ch *tx = es705_priv->slim_tx;
	u16 grph = 0;
	u32 sph[ES705_SLIM_TX_PORTS] = {0};
	int i;
	int idx;
	int rc;

	dev_dbg(&sbdev->dev, "%s(): ch_cnt = %d\n", __func__, ch_cnt);

	for (i = 0; i < ch_cnt; i++) {
		dev_dbg(&sbdev->dev, "%s(): ch_num = %d\n",
			__func__, ch_num[i]);
		idx = es705_tx_ch_num_to_idx(ch_num[i]);
		sph[i] = tx[idx].sph;
		grph = tx[idx].grph;
	}

	rc = slim_control_ch(sbdev, grph, SLIM_CH_REMOVE, true);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s(): slim_connect_sink() failed: %d\n",
			__func__, rc);
		goto slim_control_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		dev_dbg(&sbdev->dev, "%s(): ch_num = %d\n",
			__func__, ch_num[i]);
		idx = es705_tx_ch_num_to_idx(ch_num[i]);
		tx[idx].grph = 0;
	}
	rc = slim_disconnect_ports(sbdev, sph, ch_cnt);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s(): slim_disconnect_ports() failed: %d\n",
			__func__, rc);
	}
slim_control_ch_error:
	return rc;
}

int es705_remote_cfg_slim_rx(int dai_id)
{
	struct es705_priv *es705 = &es705_priv;
	struct slim_device *sbdev = es705->gen0_client;
	int be_id;
	int rc = 0;

	dev_info(&sbdev->dev, "%s(): dai_id = %d\n", __func__, dai_id);

	if (dai_id != ES705_SLIM_1_PB
	    && dai_id != ES705_SLIM_2_PB)
		return rc;

	if (es705->dai[DAI_INDEX(dai_id)].ch_tot != 0) {
		/* start slim channels associated with id */
		rc = es705_cfg_slim_rx(sbdev,
				       es705->dai[DAI_INDEX(dai_id)].ch_num,
				       es705->dai[DAI_INDEX(dai_id)].ch_tot,
				       es705->dai[DAI_INDEX(dai_id)].rate);

		be_id = es705_slim_be_id[DAI_INDEX(dai_id)];
		es705->dai[DAI_INDEX(be_id)].ch_tot = es705->dai[DAI_INDEX(dai_id)].ch_tot;
		es705->dai[DAI_INDEX(be_id)].rate = es705->dai[DAI_INDEX(dai_id)].rate;
		rc = es705_codec_cfg_slim_tx(es705, be_id);
	}

	return rc;
}
EXPORT_SYMBOL_GPL(es705_remote_cfg_slim_rx);

int es705_remote_cfg_slim_tx(int dai_id)
{
	struct es705_priv *es705 = &es705_priv;
	struct slim_device *sbdev = es705->gen0_client;
	int be_id;
	int ch_cnt;
	int rc = 0;

	dev_info(&sbdev->dev, "%s(): dai_id = %d\n", __func__, dai_id);

	if (dai_id != ES705_SLIM_1_CAP)
		return rc;

	if (es705->dai[DAI_INDEX(dai_id)].ch_tot != 0) {
		/* start slim channels associated with id */
		ch_cnt = es705->ap_tx1_ch_cnt;
		rc = es705_cfg_slim_tx(es705->gen0_client,
				       es705->dai[DAI_INDEX(dai_id)].ch_num,
				       ch_cnt,
				       es705->dai[DAI_INDEX(dai_id)].rate);

		be_id = es705_slim_be_id[DAI_INDEX(dai_id)];
		es705->dai[DAI_INDEX(be_id)].ch_tot = es705->dai[DAI_INDEX(dai_id)].ch_tot;
		es705->dai[DAI_INDEX(be_id)].rate = es705->dai[DAI_INDEX(dai_id)].rate;
		rc = es705_codec_cfg_slim_rx(es705, be_id);
	}

	return rc;
}
EXPORT_SYMBOL_GPL(es705_remote_cfg_slim_tx);

int es705_remote_close_slim_rx(int dai_id)
{
	struct es705_priv *es705 = &es705_priv;
	struct slim_device *sbdev = es705->gen0_client;
	int be_id;
	int rc = 0;

	dev_info(&sbdev->dev, "%s(): dai_id = %d\n", __func__, dai_id);

	if (dai_id != ES705_SLIM_1_PB
	    && dai_id != ES705_SLIM_2_PB)
		return rc;

	if (es705->dai[DAI_INDEX(dai_id)].ch_tot != 0) {
		dev_info(&sbdev->dev, "%s(): dai_id = %d, ch_tot =%d\n",
				__func__, dai_id,
				es705->dai[DAI_INDEX(dai_id)].ch_tot);

		es705_close_slim_rx(es705->gen0_client,
				    es705->dai[DAI_INDEX(dai_id)].ch_num,
				    es705->dai[DAI_INDEX(dai_id)].ch_tot);

		be_id = es705_slim_be_id[DAI_INDEX(dai_id)];
		rc = es705_codec_close_slim_tx(es705, be_id);

		es705->dai[DAI_INDEX(dai_id)].ch_tot = 0;
	}

	return rc;
}
EXPORT_SYMBOL_GPL(es705_remote_close_slim_rx);

int es705_remote_close_slim_tx(int dai_id)
{
	struct es705_priv *es705 = &es705_priv;
	struct slim_device *sbdev = es705->gen0_client;
	int be_id;
	int ch_cnt;
	int rc = 0;

	dev_info(&sbdev->dev, "%s(): dai_id = %d\n", __func__, dai_id);

	if (dai_id != ES705_SLIM_1_CAP)
		return rc;

	if (es705->dai[DAI_INDEX(dai_id)].ch_tot != 0) {
		dev_info(&sbdev->dev, "%s(): dai_id = %d, ch_tot = %d\n",
				__func__, dai_id,
				es705->dai[DAI_INDEX(dai_id)].ch_tot);
#if defined(SAMSUNG_ES705_FEATURE)
		if (dai_id == ES705_SLIM_1_CAP)
#endif
			ch_cnt = es705->ap_tx1_ch_cnt;
		es705_close_slim_tx(es705->gen0_client,
				    es705->dai[DAI_INDEX(dai_id)].ch_num,
				    ch_cnt);
		be_id = es705_slim_be_id[DAI_INDEX(dai_id)];
		rc = es705_codec_close_slim_rx(es705, be_id);
		es705->dai[DAI_INDEX(dai_id)].ch_tot = 0;
	}

	return rc;
}
EXPORT_SYMBOL_GPL(es705_remote_close_slim_tx);

void es705_init_slim_slave(struct slim_device *sbdev)
{
	es705_alloc_slim_rx_chan(sbdev);
	es705_alloc_slim_tx_chan(sbdev);
}

static int es705_slim_read(struct es705_priv *es705, void *rspn, int len)
{
	int rc = 0;
	int i;
	struct slim_device *sbdev = es705->gen0_client;

	struct slim_ele_access msg = {
		.start_offset = ES705_READ_VE_OFFSET,
		.num_bytes = ES705_READ_VE_WIDTH,
		.comp = NULL,
	};
	BUG_ON(len < 0);

	for (i = 0; i < MAX_SMB_TRIALS; i++) {
		char buf[4] = {0};

		rc = slim_request_val_element(sbdev, &msg, buf, 4);
		memcpy(rspn, buf, 4);
		if (!rc)
			break;
		usleep_range(SMB_DELAY, SMB_DELAY);
	}
	if (i == MAX_SMB_TRIALS)
		dev_err(&sbdev->dev, "%s(): reach SLIMBus read trials (%d)\n",
			__func__, MAX_SMB_TRIALS);
	return rc;
}

static int es705_slim_write(struct es705_priv *es705,
				const void *buf, int len)
{
	struct slim_device *sbdev = es705->gen0_client;
	int rc = 0;
	int wr = 0;

	struct slim_ele_access msg = {
		.start_offset = ES705_WRITE_VE_OFFSET,
		.num_bytes = ES705_WRITE_VE_WIDTH,
		.comp = NULL,
	};

	BUG_ON(len < 0);

	while (wr < len) {
		int i;
		int sz = min(len - wr, ES705_WRITE_VE_WIDTH);

		/*
		 * As long as the caller expects the most significant
		 * bytes of the VE value written to be zero, this is
		 * valid.
		 */

		for (i = 0; i < MAX_SMB_TRIALS; i++) {
			if (sz < ES705_WRITE_VE_WIDTH)
				dev_dbg(&sbdev->dev,
					"%s(): write smaller than VE size %d < %d\n",
					__func__, sz, ES705_WRITE_VE_WIDTH);

			rc = slim_change_val_element(sbdev, &msg,
						(char *)buf + wr, sz);
			if (!rc)
				break;
			usleep_range(SMB_DELAY, SMB_DELAY);
		}
		if (i == MAX_SMB_TRIALS) {
			dev_err(&sbdev->dev, "%s(): reach MAX_TRIALS (%d)\n",
				__func__, MAX_SMB_TRIALS);
			break;
		}
		wr += sz;
	}
	return rc;
}

static int es705_slim_write_then_read(struct es705_priv *es705,
					const void *buf, int len,
					u32 *rspn, int match)
{
	struct slim_device *sbdev = es705->gen0_client;
	const u32 NOT_READY = 0;
	u32 response = NOT_READY;
	int rc = 0;
	int trials = 0;

	/*
	 * WARNING: for single success write is
	 * possible multiple reads, because of
	 * esxxx needs time to handle request
	 * In this case read function got 0x00000000
	 * what means no response from esxxx
	 */
		rc = es705_slim_write(es705, buf, len);
		if (rc)
		goto es705_slim_write_then_read_exit;
	do {
		usleep_range(SMB_DELAY, SMB_DELAY);

		rc = es705_slim_read(es705, &response, 4);
		if (rc)
			break;

		if (response != NOT_READY) {
			if (match && *rspn != response) {
				dev_err(&sbdev->dev, "%s(): unexpected response 0x%08x\n",
					__func__, response);
				rc = -EIO;
			}
			*rspn = response;
			break;
		} else {
			dev_err(&sbdev->dev, "%s(): response=0x%08x\n",
				__func__, response);
			rc = -EIO;
		}
		trials++;
	} while (trials < MAX_WRITE_THEN_READ_TRIALS);

es705_slim_write_then_read_exit:
	return rc;
}

int es705_slim_cmd(struct es705_priv *es705, u32 cmd, int sr, u32 *resp)
{
	int rc;

	dev_dbg(es705->dev, "%s(): cmd=0x%08x  sr=%i\n",
		__func__, cmd, sr);

	cmd = cpu_to_le32(cmd);
	if (sr) {
		dev_dbg(es705->dev, "%s(): Response not expected\n",
				__func__);
		rc = es705_slim_write(es705, &cmd, sizeof(cmd));
	} else {
		u32 rv;
		int match = 0;
		rc = es705_slim_write_then_read(es705, &cmd,
						sizeof(cmd), &rv, match);
		if (!rc) {
			*resp = le32_to_cpu(rv);
			dev_dbg(es705->dev, "%s(): resp=0x%08x\n",
				__func__, *resp);
		}
	}
	return rc;
}

int es705_slim_set_channel_map(struct snd_soc_dai *dai,
			       unsigned int tx_num, unsigned int *tx_slot,
			       unsigned int rx_num, unsigned int *rx_slot)
{
	struct snd_soc_codec *codec = dai->codec;
	/* local codec access */
	/* struct es705_priv *es705 = snd_soc_codec_get_drvdata(codec); */
	/* remote codec access */
	struct es705_priv *es705 = &es705_priv;
	int id = dai->id;
	int i;
	int rc = 0;

	dev_dbg(codec->dev, "%s(): dai->name = %s, dai->id = %d\n",
		__func__, dai->name, dai->id);

	if (id == ES705_SLIM_1_PB ||
	    id == ES705_SLIM_2_PB ||
	    id == ES705_SLIM_3_PB) {
		es705->dai[DAI_INDEX(id)].ch_tot = rx_num;
		es705->dai[DAI_INDEX(id)].ch_act = 0;
		for (i = 0; i < rx_num; i++)
			es705->dai[DAI_INDEX(id)].ch_num[i] = rx_slot[i];
	} else if (id == ES705_SLIM_1_CAP ||
		 id == ES705_SLIM_2_CAP ||
		 id == ES705_SLIM_3_CAP) {
		es705->dai[DAI_INDEX(id)].ch_tot = tx_num;
		es705->dai[DAI_INDEX(id)].ch_act = 0;
		for (i = 0; i < tx_num; i++) {
			es705->dai[DAI_INDEX(id)].ch_num[i] = tx_slot[i];
		}
	}

	return rc;
}
EXPORT_SYMBOL_GPL(es705_slim_set_channel_map);

int es705_slim_get_channel_map(struct snd_soc_dai *dai,
			       unsigned int *tx_num, unsigned int *tx_slot,
			       unsigned int *rx_num, unsigned int *rx_slot)
{
	struct snd_soc_codec *codec = dai->codec;
	/* local codec access */
	/* struct es705_priv *es705 = snd_soc_codec_get_drvdata(codec); */
	/* remote codec access */
	struct es705_priv *es705 = &es705_priv;
	struct es705_slim_ch *rx = es705->slim_rx;
	struct es705_slim_ch *tx = es705->slim_tx;
	int id = dai->id;
	int i;
	int rc = 0;

	dev_dbg(codec->dev, "%s(): dai->name = %s, dai->id = %d\n", __func__,
		dai->name, dai->id);

	if (id == ES705_SLIM_1_PB) {
		*rx_num = es705_dai[DAI_INDEX(id)].playback.channels_max;
		for (i = 0; i < *rx_num; i++) {
			rx_slot[i] = rx[ES705_SLIM_1_PB_OFFSET + i].ch_num;
		}
	} else if (id == ES705_SLIM_2_PB) {
		*rx_num = es705_dai[DAI_INDEX(id)].playback.channels_max;
		for (i = 0; i < *rx_num; i++) {
			rx_slot[i] = rx[ES705_SLIM_2_PB_OFFSET + i].ch_num;
		}
	} else if (id == ES705_SLIM_3_PB) {
		*rx_num = es705_dai[DAI_INDEX(id)].playback.channels_max;
		for (i = 0; i < *rx_num; i++) {
			rx_slot[i] = rx[ES705_SLIM_3_PB_OFFSET + i].ch_num;
		}
	} else if (id == ES705_SLIM_1_CAP) {
		*tx_num = es705_dai[DAI_INDEX(id)].capture.channels_max;
		for (i = 0; i < *tx_num; i++) {
			tx_slot[i] = tx[ES705_SLIM_1_CAP_OFFSET + i].ch_num;
		}
	} else if (id == ES705_SLIM_2_CAP) {
		*tx_num = es705_dai[DAI_INDEX(id)].capture.channels_max;
		for (i = 0; i < *tx_num; i++) {
			tx_slot[i] = tx[ES705_SLIM_2_CAP_OFFSET + i].ch_num;
		}
	} else if (id == ES705_SLIM_3_CAP) {
		*tx_num = es705_dai[DAI_INDEX(id)].capture.channels_max;
		for (i = 0; i < *tx_num; i++) {
			tx_slot[i] = tx[ES705_SLIM_3_CAP_OFFSET + i].ch_num;
		}
	}

	return rc;
}
EXPORT_SYMBOL_GPL(es705_slim_get_channel_map);

void es705_slim_map_channels(struct es705_priv *es705)
{
	/* front end for RX1 */
	es705->dai[DAI_INDEX(ES705_SLIM_1_PB)].ch_num[0] = 152;
	es705->dai[DAI_INDEX(ES705_SLIM_1_PB)].ch_num[1] = 153;
	/* back end for RX1 */

#if defined(SLIMBUS_VER_2)
	es705->dai[DAI_INDEX(ES705_SLIM_2_CAP)].ch_num[0] = 144;
	es705->dai[DAI_INDEX(ES705_SLIM_2_CAP)].ch_num[1] = 145;
#else
	es705->dai[DAI_INDEX(ES705_SLIM_2_CAP)].ch_num[0] = 138;
	es705->dai[DAI_INDEX(ES705_SLIM_2_CAP)].ch_num[1] = 139;
#endif
	/* front end for TX1 */
	es705->dai[DAI_INDEX(ES705_SLIM_1_CAP)].ch_num[0] = 156;
	es705->dai[DAI_INDEX(ES705_SLIM_1_CAP)].ch_num[1] = 157;
	/* back end for TX1 */
#ifdef CONFIG_WCD9306_CODEC
		es705->dai[DAI_INDEX(ES705_SLIM_3_PB)].ch_num[0] = 128;
		es705->dai[DAI_INDEX(ES705_SLIM_3_PB)].ch_num[1] = 129;
#else
	es705->dai[DAI_INDEX(ES705_SLIM_3_PB)].ch_num[0] = 134;
	es705->dai[DAI_INDEX(ES705_SLIM_3_PB)].ch_num[1] = 135;
	es705->dai[DAI_INDEX(ES705_SLIM_3_PB)].ch_num[2] = 136;
	es705->dai[DAI_INDEX(ES705_SLIM_3_PB)].ch_num[3] = 137;
#endif /* CONFIG_WCD9306_CODEC */

	/* front end for RX2 */
	es705->dai[DAI_INDEX(ES705_SLIM_2_PB)].ch_num[0] = 154;
	es705->dai[DAI_INDEX(ES705_SLIM_2_PB)].ch_num[1] = 155;
	/* back end for RX2 */

#if defined(SLIMBUS_VER_2)
	es705->dai[DAI_INDEX(ES705_SLIM_3_CAP)].ch_num[0] = 144;
	es705->dai[DAI_INDEX(ES705_SLIM_3_CAP)].ch_num[1] = 145;
#else
	es705->dai[DAI_INDEX(ES705_SLIM_3_CAP)].ch_num[0] = 143;
	es705->dai[DAI_INDEX(ES705_SLIM_3_CAP)].ch_num[1] = 144;
#endif
}

int es705_slim_hw_params(struct snd_pcm_substream *substream,
			 struct snd_pcm_hw_params *params,
			 struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	/* local codec access */
	/* struct es705_priv *es705 = snd_soc_codec_get_drvdata(codec); */
	/* remote codec access */
	struct es705_priv *es705 = &es705_priv;
	int id = dai->id;
	int channels;
	int rate;
	int rc = 0;

	dev_dbg(codec->dev, "%s(): dai->name = %s, dai->id = %d\n",
		__func__, dai->name, dai->id);

	channels = params_channels(params);
	switch (channels) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		es705->dai[DAI_INDEX(id)].ch_tot = channels;
		break;
	default:
		dev_err(codec->dev, "%s(): unsupported number of channels, %d\n",
			__func__, channels);
		return -EINVAL;
	}
	rate = params_rate(params);
	switch (rate) {
	case 8000:
	case 16000:
	case 32000:
	case 48000:
		es705->dai[DAI_INDEX(id)].rate = rate;
		break;
	default:
		dev_err(codec->dev, "%s(): unsupported rate, %d\n",
			__func__, rate);
		return -EINVAL;
	}

	return rc;
}
EXPORT_SYMBOL_GPL(es705_slim_hw_params);

static int es705_slim_vs_streaming(struct es705_priv *es705)
{
	int rc;
#ifdef VS_BUFFER_VIA_UART
/*
	rc = gpio_direction_input(es705->pdata->wakeup_gpio);
	if (rc < 0) {
		dev_err(es705_priv.dev, "%s(): set GPIO for UART failed",
			__func__);
		goto es705_vs_streaming_error;
	}
*/
	rc = es705_uart_open(es705);
	if (rc) {
		dev_err(es705->dev, "%s(): uart open error\n",
			__func__);
		goto es705_vs_streaming_error;
	}
	/* set speed to bootloader baud */
	es705_configure_tty(es705_priv.uart_dev.tty,
		UART_TTY_BAUD_RATE_BOOTLOADER, UART_TTY_STOP_BITS);
	/* wait until all data will be read by App */
/*
	rc = es705_uart_write(es705, data, sz);
	if (rc != sz) {
		dev_err(es705->dev, "%s(): uart write fail\n",
			__func__);
		rc = -EIO;
		goto es705_vs_streaming_error;
	}
	msleep(20000);
*/
	rc = es705_uart_close(es705);
	if (rc) {
		dev_err(es705->dev, "%s(): uart close error\n",
			__func__);
		goto es705_vs_streaming_error;
	}
/*
	rc = gpio_direction_output(es705->pdata->wakeup_gpio, 0);
	if (rc < 0)
		dev_err(es705_priv.dev, "%s(): set UART direction failed",
			__func__);
	}
*/
#else
	/*
	 * VS BUFFER via SLIMBus
	 * end point is
	 * 156 - 0x9c for pass through 1 chan mode
	 * 156 - 0x9c and 157 - 0x9d for pass through 2 channels mode
	 */
	u32 vs_stream_end_point = 0x8028809C;
	u32 rspn = vs_stream_end_point;
	int match = 1;
	u32 vs_stream_cmd = 0x90250202 | ES705_STREAM_ENABLE;

	/* select streaming pathID */
	dev_dbg(es705->dev, "%s(): Set VS Streaming PathID\n", __func__);
	rc = es705_slim_write_then_read(es705, &vs_stream_end_point,
			      sizeof(vs_stream_end_point), &rspn, match);
	if (rc) {
		dev_err(es705->dev, "%s(): Select VS stream Path ID Fail\n",
			__func__);
		goto es705_vs_streaming_error;
	}

	/* enable streaming */
	dev_dbg(es705->dev, "%s(): Enable VS Streaming\n", __func__);
	rc = es705_slim_write(es705, &vs_stream_cmd, 4);
	if (rc) {
		dev_err(es705->dev, "%s(): Enable VS streaming Fail\n",
			__func__);
		goto es705_vs_streaming_error;
	}

	/* TODO wait end of streaming and disable */
/*
	vs_stream = 0x90250200 | ES705_STREAM_DISABLE;
	rc = es705_slim_write(es705, &vs_stream_cmd, 4);
	if (rc)
		dev_err(es705->dev, "%s(): Disable VS streaming Fail\n",
			__func__);
*/
#endif
es705_vs_streaming_error:
	return rc;
}

static int es705_slim_boot_setup(struct es705_priv *es705)
{
	u32 boot_cmd = ES705_BOOT_CMD;
	u32 sync_cmd = (ES705_SYNC_CMD << 16) | ES705_SYNC_POLLING;
	u32 sbl_rspn = ES705_SBL_ACK;
	u32 ack_rspn = ES705_BOOT_ACK;
	int match = 1;
	int rc;

	dev_info(es705->dev, "%s(): prepare for fw download\n", __func__);
	rc = es705_slim_write_then_read(es705, &sync_cmd, sizeof(sync_cmd),
			      &sbl_rspn, match);
	if (rc) {
		dev_err(es705->dev, "%s(): SYNC_SBL fail\n", __func__);
		goto es705_slim_boot_setup_failed;
	}

	es705->mode = SBL;

	rc = es705_slim_write_then_read(es705, &boot_cmd, sizeof(boot_cmd),
			&ack_rspn, match);
	if (rc)
		dev_err(es705->dev, "%s(): BOOT_CMD fail\n", __func__);

es705_slim_boot_setup_failed:
	return rc;
}

static int es705_slim_boot_finish(struct es705_priv *es705)
{
	u32 sync_cmd;
	u32 sync_rspn;
	int match = 1;
	int rc = 0;

	dev_info(es705->dev, "%s(): finish fw download\n", __func__);
	if (es705->es705_power_state == ES705_SET_POWER_STATE_VS_OVERLAY) {
		sync_cmd = (ES705_SYNC_CMD << 16) | ES705_SYNC_INTR_RISING_EDGE;
		dev_info(es705->dev, "%s(): FW type : VOICESENSE\n", __func__);
	} else {
		sync_cmd = (ES705_SYNC_CMD << 16) | ES705_SYNC_POLLING;
		dev_info(es705->dev, "%s(): fw type : STANDARD\n", __func__);
	}
	sync_rspn = sync_cmd;

	/* Give the chip some time to become ready after firmware download. */
	msleep(20);
	/* finish es705 boot, check es705 readiness */
	rc = es705_slim_write_then_read(es705, &sync_cmd, sizeof(sync_cmd),
			&sync_rspn, match);
	if (rc)
		dev_err(es705->dev, "%s(): SYNC fail\n", __func__);
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

struct snd_soc_dai_ops es705_slim_port_dai_ops = {
	.set_fmt	= NULL,
	.set_channel_map	= es705_slim_set_channel_map,
	.get_channel_map	= es705_slim_get_channel_map,
	.set_tristate	= NULL,
	.digital_mute	= NULL,
	.startup	= NULL,
	.shutdown	= NULL,
	.hw_params	= es705_slim_hw_params,
	.hw_free	= NULL,
	.prepare	= NULL,
	.trigger	= NULL,
};

#define CODEC_ID "es705-codec"
#define CODEC_INTF_ID "es705-codec-intf"
#define CODEC_GEN0_ID "es705-codec-gen0"

static int es705_slim_device_up(struct slim_device *sbdev)
{
	struct es705_priv *priv;
	int rc;
#ifdef CONFIG_SND_SOC_ES704_TEMP
	dev_selected = 1;
#endif
	dev_info(&sbdev->dev, "%s(): name=%s\n", __func__, sbdev->name);
	dev_info(&sbdev->dev, "%s(): laddr=%d\n", __func__, sbdev->laddr);
	/* Start the firmware download in the workqueue context. */
	priv = slim_get_devicedata(sbdev);
	if (strncmp(sbdev->name, CODEC_INTF_ID,
		strnlen(CODEC_INTF_ID, SLIMBUS_NAME_SIZE)) == 0)
		return 0;

	rc = fw_download(priv);
	BUG_ON(rc != 0);
#if defined(SAMSUNG_ES705_FEATURE)
	if (priv->power_control)
		priv->power_control(ES705_SET_POWER_STATE_SLEEP,
				    ES705_POWER_STATE);
#endif
	return rc;
}

#if defined(SLIMBUS_VER_1)
static int es705_fw_thread(void *priv)
{
	struct es705_priv *es705 = (struct es705_priv  *)priv;

	do {
		slim_get_logical_addr(es705->gen0_client,
				      es705->gen0_client->e_addr,
				      6, &(es705->gen0_client->laddr));
		usleep_range(1000, 2000);
	} while (es705->gen0_client->laddr == 0xf0);
	dev_dbg(&es705->gen0_client->dev, "%s(): gen0_client LA = %d\n",
		__func__, es705->gen0_client->laddr);
	do {
		slim_get_logical_addr(es705->intf_client,
				      es705->intf_client->e_addr,
				      6, &(es705->intf_client->laddr));
		usleep_range(1000, 2000);
	} while (es705->intf_client->laddr == 0xf0);
	dev_dbg(&es705->intf_client->dev, "%s(): intf_client LA = %d\n",
		__func__, es705->intf_client->laddr);

	es705_slim_device_up(es705->gen0_client);
	return 0;
}
#endif

#ifdef CONFIG_SND_SOC_ES704_TEMP
#define CODEC_ID_ES704 "es704-codec"
#define CODEC_INTF_ID_ES704 "es704-ifd"
#define CODEC_INTF_PROP_ES704 "es704-slim-ifd"
#define CODEC_GEN0_ID_ES704 "es704-codec-gen0"
#define CODEC_ELEMENTAL_ADDR_ES704 "es704-slim-ifd-elemental-addr"
#endif

static int es705_dt_parse_slim_interface_dev_info(struct device *dev,
						struct slim_device *slim_ifd)
{
	int rc;
	struct property *prop;

	rc = of_property_read_string(dev->of_node,
			"es705-slim-ifd", &slim_ifd->name);
	if (rc) {
		dev_info(dev, "%s(): %s failed",
			__func__, dev->of_node->full_name);
		return -ENODEV;
	}

	prop = of_find_property(dev->of_node,
			"es705-slim-ifd-elemental-addr", NULL);
	if (!prop) {
		dev_info(dev, "%s(): %s failed",
			__func__, dev->of_node->full_name);
		return -ENODEV;
	} else if (prop->length != sizeof(slim_ifd->e_addr) ) {
		dev_err(dev, "%s(): invalid slim ifd addr. addr length = %d\n",
			__func__, prop->length);
		return -ENODEV;
	}

	memcpy(slim_ifd->e_addr, prop->value, sizeof(slim_ifd->e_addr) );
	return 0;
}

static int es705_slim_probe_dts(struct slim_device *sbdev)
{
	static struct slim_device intf_device;
	struct esxxx_platform_data *pdata = sbdev->dev.platform_data;
	int rc = 0;

	if (sbdev->dev.of_node) {
		dev_info(&sbdev->dev, "%s(): Platform data from device tree\n",
			 __func__);
		pdata = es705_populate_dt_pdata(&sbdev->dev);
		if (pdata == NULL) {
			dev_err(&sbdev->dev, "%s(): pdata is NULL", __func__);
			rc = -EIO;
			goto pdata_error;
		}

		rc = es705_dt_parse_slim_interface_dev_info(&sbdev->dev,
							    &intf_device);

		if (rc) {
			dev_info(&sbdev->dev, "%s(): Error, parsing slim interface\n",
				__func__);
			devm_kfree(&sbdev->dev, pdata);
			rc = -EINVAL;
			goto pdata_error;
		}
		sbdev->dev.platform_data = pdata;

		es705_priv.gen0_client = sbdev;
		es705_priv.intf_client = &intf_device;
	}
pdata_error:
	return rc;
}

static void es705_slim_probe_nodts(struct slim_device *sbdev)
{
	if (strncmp(sbdev->name, CODEC_INTF_ID,
		strnlen(CODEC_INTF_ID, SLIMBUS_NAME_SIZE)) == 0) {
		dev_dbg(&sbdev->dev, "%s(): interface device probe\n",
			__func__);
		es705_priv.intf_client = sbdev;
	}
	if (strncmp(sbdev->name, CODEC_GEN0_ID,
		strnlen(CODEC_GEN0_ID, SLIMBUS_NAME_SIZE)) == 0) {
		dev_dbg(&sbdev->dev, "%s(): generic device probe\n",
			__func__);
		es705_priv.gen0_client = sbdev;
	}
}

int es705_slim_wakeup_bus(struct es705_priv *es705)
{
	int rc;
	rc = slim_ctrl_clk_pause(es705->gen0_client->ctrl, 1, 0);
	return rc;
}

#ifdef CONFIG_ARCH_MSM8226
static int es705_regulator_init(struct device *dev)
{
	int ret;
	struct device_node *reg_node = NULL;

	reg_node = of_parse_phandle(dev->of_node, "vdd-2mic-core-supply", 0);
	if(reg_node)
	{
		es705_ldo = regulator_get(dev, "vdd-2mic-core");
		if (IS_ERR(es705_ldo)) {
				pr_err("[%s] could not get earjack_ldo, %ld\n", __func__, PTR_ERR(es705_ldo));
		}
		else
		{
			ret = regulator_enable(es705_ldo);
			if(ret < 0) {
				pr_err("%s():Failed to enable regulator.\n",
					__func__);
				goto err_reg_enable;
			} else
				regulator_set_mode(es705_ldo, REGULATOR_MODE_NORMAL);
		}
	}else
		pr_err("%s Audience LDO node not available\n",__func__);

err_reg_enable:
	if(es705_ldo)
		regulator_put(es705_ldo);

	return ret;
}

static int es705_regulator_deinit(void)
{
	if(es705_ldo)
	{
		int ret;

		ret = regulator_disable(es705_ldo);
		if(ret < 0) {
			pr_err("%s():Failed to disable regulator.\n",__func__);
		}
		regulator_put(es705_ldo);
	}

	return 0;
}
#endif /* CONFIG_ARCH_MSM8226 */

static int es705_slim_probe(struct slim_device *sbdev)
{
	int rc;

#if defined(SLIMBUS_VER_1)
	struct task_struct *thread = NULL;
#endif

	dev_info(&sbdev->dev, "%s(): sbdev->name = %s\n", __func__, sbdev->name);

#ifdef CONFIG_SND_SOC_ES704_TEMP
	if (!strcmp(sbdev->name, "es704-codec-gen0")) {
		dev_info(&sbdev->dev, "%s() ES704 device, wait some time\n", __func__);
		msleep(10);
		if (dev_selected) {
			dev_info(&sbdev->dev, "%s(): dev_selected is True, return\n", __func__);
			return -1;
		}
	}
#endif

#ifdef CONFIG_ARCH_MSM8226
		es705_regulator_init(&sbdev->dev);
#endif /* CONFIG_ARCH_MSM8226 */

	if (sbdev->dev.of_node) {
		rc = es705_slim_probe_dts(sbdev);
		if (rc)
			goto es705_core_probe_error;
	} else {
		es705_slim_probe_nodts(sbdev);
	}

	if (es705_priv.intf_client == NULL ||
	    es705_priv.gen0_client == NULL) {
		dev_dbg(&sbdev->dev, "%s(): incomplete initialization\n",
			__func__);
		return 0;
	}

	slim_set_clientdata(sbdev, &es705_priv);

	es705_priv.intf = ES705_SLIM_INTF;
	es705_priv.dev_read = es705_slim_read;
	es705_priv.dev_write = es705_slim_write;
	es705_priv.dev_write_then_read = es705_slim_write_then_read;
	es705_priv.vs_streaming = es705_slim_vs_streaming;

	es705_priv.streamdev = uart_streamdev;
	es705_priv.boot_setup = es705_slim_boot_setup;
	es705_priv.boot_finish = es705_slim_boot_finish;
	es705_priv.wakeup_bus = es705_slim_wakeup_bus;

	es705_priv.cmd = es705_slim_cmd;
	es705_priv.dev = &es705_priv.gen0_client->dev;
	rc = es705_core_probe(&es705_priv.gen0_client->dev);
	if (rc) {
		dev_err(&sbdev->dev, "%s(): es705_core_probe() failed %d\n",
			__func__, rc);
		goto es705_core_probe_error;
	}

#if defined(SLIMBUS_VER_1)
	thread = kthread_run(es705_fw_thread, &es705_priv, "audience thread");
	if (IS_ERR(thread)) {
		dev_err(&sbdev->dev, "%s(): can't create es705 firmware thread = %p\n",
			__func__, thread);
		return -1;
	}
#endif

	return 0;

es705_core_probe_error:
	dev_dbg(&sbdev->dev, "%s(): exit with error\n", __func__);
	return rc;
}

static int es705_slim_remove(struct slim_device *sbdev)
{
	struct esxxx_platform_data *pdata = sbdev->dev.platform_data;

	dev_dbg(&sbdev->dev, "%s(): sbdev->name = %s\n", __func__, sbdev->name);

#ifdef CONFIG_ARCH_MSM8226
		es705_regulator_deinit();
#endif /* CONFIG_ARCH_MSM8226 */

	es705_gpio_free(pdata);

	snd_soc_unregister_codec(&sbdev->dev);

	return 0;
}

static const struct slim_device_id es705_slim_id[] = {
	{ CODEC_ID, 0 },
	{ CODEC_INTF_ID, 0 },
	{ CODEC_GEN0_ID, 0 },
	{  }
};
MODULE_DEVICE_TABLE(slim, es705_slim_id);

struct slim_driver es705_slim_driver = {
	.driver = {
		.name = CODEC_ID,
		.owner = THIS_MODULE,
	},
	.probe = es705_slim_probe,
	.remove = es705_slim_remove,
#if defined(SLIMBUS_VER_2)
	.device_up = es705_slim_device_up,
#endif
	.id_table = es705_slim_id,
};

#ifdef CONFIG_SND_SOC_ES704_TEMP
static const struct slim_device_id es704_slim_id[] = {
	{ CODEC_ID_ES704, 0 },
	{ CODEC_INTF_ID_ES704, 0 },
	{ CODEC_GEN0_ID_ES704, 0 },
	{  }
};
MODULE_DEVICE_TABLE(slim, es704_slim_id);

struct slim_driver es704_slim_driver = {
	.driver = {
		.name = CODEC_ID_ES704,
		.owner = THIS_MODULE,
	},
	.probe = es705_slim_probe,
	.remove = es705_slim_remove,
#ifdef CONFIG_SLIMBUS_MSM_NGD
	.device_up = es705_slim_device_up,
#endif
	.id_table = es704_slim_id,
};
#endif

int es705_bus_init(struct es705_priv *es705)
{
	int rc;
	rc = slim_driver_register(&es705_slim_driver);
	if (!rc) {
		dev_info(es705->dev, "%s(): registered as SLIMBus\n", __func__);
		es705->intf = ES705_SLIM_INTF;
		/*
		es705_priv.device_read = ;
		es705_priv.device_write = ;
		*/

	}
#ifdef CONFIG_SND_SOC_ES704_TEMP
	rc = slim_driver_register(&es704_slim_driver);
	if (!rc)
		dev_info(es705->dev, "%s(): ES704 registered as SLIMBus\n", __func__);
#endif
	return rc;
}

MODULE_DESCRIPTION("ASoC ES705 driver");
MODULE_AUTHOR("Greg Clemson <gclemson@audience.com>");
MODULE_AUTHOR("Genisim Tsilker <gtsilker@audience.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:es705-codec");
