/*
 * escore-slim.c  --  Slimbus interface for Audience earSmart chips
 *
 * Copyright 2011 Audience, Inc.
 *
 * Author: Greg Clemson <gclemson@audience.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "escore.h"
#include "escore-slim.h"

static void escore_alloc_slim_rx_chan(struct slim_device *sbdev);
static void escore_alloc_slim_tx_chan(struct slim_device *sbdev);
static int escore_cfg_slim_rx(struct slim_device *sbdev, unsigned int *ch_num,
			     unsigned int ch_cnt, unsigned int rate);
static int escore_cfg_slim_tx(struct slim_device *sbdev, unsigned int *ch_num,
			     unsigned int ch_cnt, unsigned int rate);
static int escore_close_slim_rx(struct slim_device *sbdev, unsigned int *ch_num,
			       unsigned int ch_cnt);
static int escore_close_slim_tx(struct slim_device *sbdev, unsigned int *ch_num,
			       unsigned int ch_cnt);
static int escore_rx_ch_num_to_idx(struct escore_priv *escore, int ch_num);
static int escore_tx_ch_num_to_idx(struct escore_priv *escore, int ch_num);

static void escore_alloc_slim_rx_chan(struct slim_device *sbdev)
{
	struct escore_priv *escore_priv = slim_get_devicedata(sbdev);
	struct escore_slim_ch *rx = escore_priv->slim_rx;
	int i;
	int port_id;

	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	for (i = 0; i < escore_priv->slim_rx_ports; i++) {
		port_id = i;
		rx[i].ch_num = escore_priv->slim_rx_port_to_ch_map[i];
		slim_get_slaveport(sbdev->laddr, port_id, &rx[i].sph,
				   SLIM_SINK);
		slim_query_ch(sbdev, rx[i].ch_num, &rx[i].ch_h);
	}
}

static void escore_alloc_slim_tx_chan(struct slim_device *sbdev)
{
	struct escore_priv *escore_priv = slim_get_devicedata(sbdev);
	struct escore_slim_ch *tx = escore_priv->slim_tx;
	int i;
	int port_id;

	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	for (i = 0; i < escore_priv->slim_tx_ports; i++) {
		port_id = i + 10;
		tx[i].ch_num = escore_priv->slim_tx_port_to_ch_map[i];
		slim_get_slaveport(sbdev->laddr, port_id, &tx[i].sph,
				   SLIM_SRC);
		slim_query_ch(sbdev, tx[i].ch_num, &tx[i].ch_h);
	}
}


static int escore_rx_ch_num_to_idx(struct escore_priv *escore, int ch_num)
{
	int i;
	int idx = -1;

	pr_debug("%s(ch_num = %d)\n", __func__, ch_num);

	for (i = 0; i < escore->slim_rx_ports; i++) {
		if (ch_num == escore->slim_rx_port_to_ch_map[i]) {
			idx = i;
			break;
		}
	}

	return idx;
}

static int escore_tx_ch_num_to_idx(struct escore_priv *escore, int ch_num)
{
	int i;
	int idx = -1;

	pr_debug("%s(ch_num = %d)\n", __func__, ch_num);

	for (i = 0; i < escore->slim_tx_ports; i++) {
		if (ch_num == escore->slim_tx_port_to_ch_map[i]) {
			idx = i;
			break;
		}
	}

	return idx;
}

static int escore_cfg_slim_rx(struct slim_device *sbdev, unsigned int *ch_num,
			     unsigned int ch_cnt, unsigned int rate)
{
	struct escore_priv *escore_priv = slim_get_devicedata(sbdev);
	struct escore_slim_ch *rx = escore_priv->slim_rx;
	u16 grph;
	u32 *sph;
	u16 *ch_h;
	struct slim_ch prop;
	int i;
	int idx;
	int rc = 0;

	dev_dbg(&sbdev->dev, "%s(ch_cnt = %d, rate = %d)\n", __func__,
		ch_cnt, rate);

	sph = kmalloc(sizeof(u32)*escore_priv->slim_rx_ports, GFP_KERNEL);
	if (!sph)
		return -ENOMEM;
	ch_h = kmalloc(sizeof(u32)*escore_priv->slim_rx_ports, GFP_KERNEL);
	if (!ch_h) {
		kfree(sph);
		return -ENOMEM;
	}

	for (i = 0; i < ch_cnt; i++) {
		idx = escore_rx_ch_num_to_idx(escore_priv, ch_num[i]);
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
			dev_err(&sbdev->dev,
				"%s(): slim_connect_sink() failed: %d\n",
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
		idx = escore_rx_ch_num_to_idx(escore_priv, ch_num[i]);
		rx[idx].grph = grph;
	}
	kfree(sph);
	kfree(ch_h);
	return rc;
slim_control_ch_error:
slim_connect_sink_error:
	escore_close_slim_rx(sbdev, ch_num, ch_cnt);
slim_define_ch_error:
	kfree(sph);
	kfree(ch_h);
	return rc;
}

static int escore_cfg_slim_tx(struct slim_device *sbdev, unsigned int *ch_num,
			     unsigned int ch_cnt, unsigned int rate)
{
	struct escore_priv *escore_priv = slim_get_devicedata(sbdev);
	struct escore_slim_ch *tx = escore_priv->slim_tx;
	u16 grph;
	u32 *sph;
	u16 *ch_h;
	struct slim_ch prop;
	int i;
	int idx;
	int rc;

	dev_dbg(&sbdev->dev, "%s(ch_cnt = %d, rate = %d)\n", __func__,
		ch_cnt, rate);

	sph = kmalloc(sizeof(u32)*escore_priv->slim_rx_ports, GFP_KERNEL);
	if (!sph)
		return -ENOMEM;
	ch_h = kmalloc(sizeof(u32)*escore_priv->slim_rx_ports, GFP_KERNEL);
	if (!ch_h) {
		kfree(sph);
		return -ENOMEM;
	}


	for (i = 0; i < ch_cnt; i++) {
		idx = escore_tx_ch_num_to_idx(escore_priv, ch_num[i]);
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
			dev_err(&sbdev->dev,
				"%s(): slim_connect_src() failed: %d\n",
				__func__, rc);
			dev_err(&sbdev->dev,
				"%s(): ch_num[0] = %d\n",
				__func__, ch_num[0]);
			goto slim_connect_src_error;
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
		idx = escore_tx_ch_num_to_idx(escore_priv, ch_num[i]);
		tx[idx].grph = grph;
	}

	kfree(sph);
	kfree(ch_h);
	return rc;
slim_control_ch_error:
slim_connect_src_error:
	escore_close_slim_tx(sbdev, ch_num, ch_cnt);
slim_define_ch_error:
	kfree(sph);
	kfree(ch_h);
	return rc;
}

static int escore_close_slim_rx(struct slim_device *sbdev, unsigned int *ch_num,
			       unsigned int ch_cnt)
{
	struct escore_priv *escore_priv = slim_get_devicedata(sbdev);
	struct escore_slim_ch *rx = escore_priv->slim_rx;
	u16 grph = 0;
	u32 *sph;
	int i;
	int idx;
	int rc;

	sph = kmalloc(sizeof(u32)*escore_priv->slim_rx_ports, GFP_KERNEL);
	if (!sph)
		return -ENOMEM;

	dev_dbg(&sbdev->dev, "%s(ch_cnt = %d)\n", __func__, ch_cnt);

	for (i = 0; i < ch_cnt; i++) {
		idx = escore_rx_ch_num_to_idx(escore_priv, ch_num[i]);
		sph[i] = rx[idx].sph;
		grph = rx[idx].grph;
	}

	rc = slim_control_ch(sbdev, grph, SLIM_CH_REMOVE, true);
	if (rc < 0) {
		dev_err(&sbdev->dev,
			"%s(): slim_control_ch() failed: %d\n",
			__func__, rc);
		goto slim_control_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		idx = escore_rx_ch_num_to_idx(escore_priv, ch_num[i]);
		rx[idx].grph = 0;
	}
	rc = slim_disconnect_ports(sbdev, sph, ch_cnt);
	if (rc < 0) {
		dev_err(&sbdev->dev,
			"%s(): slim_disconnect_ports() failed: %d\n",
			__func__, rc);
	}
slim_control_ch_error:
	kfree(sph);
	return rc;
}

static int escore_close_slim_tx(struct slim_device *sbdev, unsigned int *ch_num,
			       unsigned int ch_cnt)
{
	struct escore_priv *escore_priv = slim_get_devicedata(sbdev);
	struct escore_slim_ch *tx = escore_priv->slim_tx;
	u16 grph = 0;
	u32 *sph;
	int i;
	int idx;
	int rc;

	sph = kmalloc(sizeof(u32)*escore_priv->slim_tx_ports, GFP_KERNEL);
	if (!sph)
		return -ENOMEM;

	dev_dbg(&sbdev->dev, "%s(ch_cnt = %d)\n", __func__, ch_cnt);

	for (i = 0; i < ch_cnt; i++) {
		idx = escore_tx_ch_num_to_idx(escore_priv, ch_num[i]);
		sph[i] = tx[idx].sph;
		grph = tx[idx].grph;
	}

	rc = slim_control_ch(sbdev, grph, SLIM_CH_REMOVE, true);
	if (rc < 0) {
		dev_err(&sbdev->dev,
			"%s(): slim_connect_sink() failed: %d\n",
			__func__, rc);
		goto slim_control_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		idx = escore_tx_ch_num_to_idx(escore_priv, ch_num[i]);
		tx[idx].grph = 0;
	}
	rc = slim_disconnect_ports(sbdev, sph, ch_cnt);
	if (rc < 0) {
		dev_err(&sbdev->dev,
			"%s(): slim_disconnect_ports() failed: %d\n",
			__func__, rc);
	}
slim_control_ch_error:
	kfree(sph);
	return rc;
}

int escore_remote_cfg_slim_rx(int dai_id)
{
	struct escore_priv *escore = &escore_priv;
	int rc = 0;

	dev_dbg(escore->dev, "%s(dai_id = %d)\n", __func__, dai_id);

	if (escore->remote_cfg_slim_rx)
		rc = escore->remote_cfg_slim_rx(dai_id);
	return rc;
}
EXPORT_SYMBOL_GPL(escore_remote_cfg_slim_rx);

int escore_remote_cfg_slim_tx(int dai_id)
{
	struct escore_priv *escore = &escore_priv;
	int rc = 0;

	dev_dbg(escore->dev, "%s(dai_id = %d)\n", __func__, dai_id);

	if (escore->remote_cfg_slim_tx)
		rc = escore->remote_cfg_slim_tx(dai_id);
	return rc;
}
EXPORT_SYMBOL_GPL(escore_remote_cfg_slim_tx);

int escore_remote_close_slim_rx(int dai_id)
{
	struct escore_priv *escore = &escore_priv;
	int rc = 0;

	dev_dbg(escore->dev, "%s(dai_id = %d)\n", __func__, dai_id);

	if (escore->remote_close_slim_rx)
		rc = escore->remote_close_slim_rx(dai_id);
	return rc;
}
EXPORT_SYMBOL_GPL(escore_remote_close_slim_rx);

int escore_remote_close_slim_tx(int dai_id)
{
	struct escore_priv *escore = &escore_priv;
	int rc = 0;

	dev_dbg(escore->dev, "%s(dai_id = %d)\n", __func__, dai_id);

	if (escore->remote_close_slim_tx)
		rc = escore->remote_close_slim_tx(dai_id);
	return rc;
}
EXPORT_SYMBOL_GPL(escore_remote_close_slim_tx);

void escore_init_slim_slave(struct slim_device *sbdev)
{
	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	escore_alloc_slim_rx_chan(sbdev);
	escore_alloc_slim_tx_chan(sbdev);
}

int escore_slim_read(struct escore_priv *escore, void *buf, int len)
{
	struct slim_device *sbdev = escore->gen0_client;
	DECLARE_COMPLETION_ONSTACK(read_done);
	struct slim_ele_access msg = {
		.start_offset = ES_READ_VE_OFFSET,
		.num_bytes = ES_READ_VE_WIDTH,
		.comp = NULL,
	};
	int rc;

	rc = slim_request_val_element(sbdev, &msg, buf, len);
	if (rc != 0)
		dev_err(&sbdev->dev, "%s: read failed rc=%d\n",
			__func__, rc);

	return rc;
}

int escore_slim_write(struct escore_priv *escore, const void *buf, int len)
{
	struct slim_device *sbdev = escore->gen0_client;
	struct slim_ele_access msg = {
		.start_offset = ES_WRITE_VE_OFFSET,
		.num_bytes = ES_WRITE_VE_WIDTH,
		.comp = NULL,
	};
	int ret;
	int wr;

	BUG_ON(len < 0);

	ret = wr = 0;
	while (wr < len) {
		int sz = min(len - wr, ES_WRITE_VE_WIDTH);

		/* As long as the caller expects the most significant
		 * bytes of the VE value written to be zero, this is
		 * valid.
		 */
		if (sz < ES_WRITE_VE_WIDTH)
			dev_dbg(&sbdev->dev,
				"write smaller than VE size %d < %d\n",
				sz, ES_WRITE_VE_WIDTH);

		ret = slim_change_val_element(sbdev, &msg, buf + wr, sz);
		if (ret) {
			dev_err(&sbdev->dev, "%s: failed ret=%d\n",
				__func__, ret);
			break;
		}
		wr += sz;
	}

	return ret;
}

int escore_slim_cmd(struct escore_priv *escore, u32 cmd, int sr, u32 *resp)
{
	int err;
	u32 rv;

	pr_debug("escore: cmd=0x%08x  sr=%i\n", cmd, sr);

	cmd = cpu_to_le32(cmd);
	err = escore_slim_write(escore, &cmd, sizeof(cmd));
	if (err || sr)
		return err;

	/* The response must be actively read. Maximum response time
	 * is 20ms.
	 */
	msleep(20);
	err = escore_slim_read(escore, &rv, sizeof(rv));
	if (!err)
		*resp = le32_to_cpu(rv);
	pr_debug("core: %s resp=0x%08x\n", __func__, *resp);
	return err;
}

static int escore_slim_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	return 0;
}

int escore_slim_set_channel_map(struct snd_soc_dai *dai,
			       unsigned int tx_num, unsigned int *tx_slot,
			       unsigned int rx_num, unsigned int *rx_slot)
{
	return 0;
}
EXPORT_SYMBOL_GPL(escore_slim_set_channel_map);

int escore_slim_get_channel_map(struct snd_soc_dai *dai,
			       unsigned int *tx_num, unsigned int *tx_slot,
			       unsigned int *rx_num, unsigned int *rx_slot)
{
	return 0;
}
EXPORT_SYMBOL_GPL(escore_slim_get_channel_map);

static int escore_slim_set_tristate(struct snd_soc_dai *dai, int tristate)
{
	return 0;
}

static int escore_slim_port_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

int escore_slim_startup(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;

	dev_dbg(codec->dev, "%s() dai->name = %s, dai->id = %d\n", __func__,
			dai->name, dai->id);

	if (codec && codec->dev && codec->dev->parent)
		pm_runtime_get_sync(codec->dev->parent);

	return 0;
}
EXPORT_SYMBOL_GPL(escore_slim_startup);

void escore_slim_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;

	dev_dbg(codec->dev, "%s() dai->name = %s, dai->id = %d\n", __func__,
			dai->name, dai->id);

	if (codec && codec->dev && codec->dev->parent) {
		pm_runtime_mark_last_busy(codec->dev->parent);
		pm_runtime_put(codec->dev->parent);
	}

	return;
}
EXPORT_SYMBOL_GPL(escore_slim_shutdown);

int escore_slim_hw_params(struct snd_pcm_substream *substream,
			 struct snd_pcm_hw_params *params,
			 struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	/* local codec access */
	struct escore_priv *escore = snd_soc_codec_get_drvdata(codec);
	/* remote codec access */
	int id = dai->id;
	int channels;
	int rate;
	int ret = 0;
	int channel_dir;

	dev_dbg(codec->dev, "%s() dai->name = %s, dai->id = %d\n", __func__,
			dai->name, dai->id);

	channels = params_channels(params);
	switch (channels) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		escore->dai[id - 1].ch_tot = channels;
		break;
	default:
		dev_err(codec->dev,
			"%s(): unsupported number of channels, %d\n",
			__func__, channels);
		return -EINVAL;
	}
	rate = params_rate(params);
	switch (rate) {
	case 8000:
	case 16000:
	case 24000:
	case 32000:
	case 48000:
		escore->dai[id - 1].rate = rate;
		break;
	default:
		dev_err(codec->dev,
			"%s(): unsupported rate, %d\n",
			__func__, rate);
		return -EINVAL;
	}

	if (escore->local_slim_ch_cfg && escore->channel_dir) {
		channel_dir = escore->channel_dir(id);
		if (channel_dir == ES_SLIM_CH_RX) {
			ret = escore_cfg_slim_rx(escore->gen0_client,
					escore->dai[dai->id - 1].ch_num,
					escore->dai[dai->id - 1].ch_tot,
					escore->dai[dai->id - 1].rate);
			escore->dai[dai->id - 1].ch_act = 1;
		} else if (channel_dir == ES_SLIM_CH_TX) {
			ret = escore_cfg_slim_tx(escore->gen0_client,
					escore->dai[dai->id - 1].ch_num,
					escore->dai[dai->id - 1].ch_tot,
					escore->dai[dai->id - 1].rate);
			escore->dai[dai->id - 1].ch_act = 1;
		} else
			return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(escore_slim_hw_params);

static int escore_slim_hw_free(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	/* local codec access */
	struct escore_priv *escore = snd_soc_codec_get_drvdata(codec);
	/* remote codec access */
	int id = dai->id;
	int ret = 0;
	int channel_dir;


	if (escore->local_slim_ch_cfg && escore->channel_dir) {
		channel_dir = escore->channel_dir(id);
		if (channel_dir == ES_SLIM_CH_RX &&
				escore->dai[dai->id - 1].ch_tot) {
			ret = escore_close_slim_rx(escore->gen0_client,
					escore->dai[dai->id - 1].ch_num,
					escore->dai[dai->id - 1].ch_tot);
			escore->dai[dai->id - 1].ch_act = 0;
		} else if (channel_dir == ES_SLIM_CH_TX &&
				escore->dai[dai->id - 1].ch_tot) {
			ret = escore_close_slim_tx(escore->gen0_client,
					escore->dai[dai->id - 1].ch_num,
					escore->dai[dai->id - 1].ch_tot);
			escore->dai[dai->id - 1].ch_act = 0;
		} else
			return -EINVAL;
	}

	return 0;
}

static int escore_slim_prepare(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	return 0;
}

int escore_slim_trigger(struct snd_pcm_substream *substream,
		       int cmd, struct snd_soc_dai *dai)
{
	return 0;
}
EXPORT_SYMBOL_GPL(escore_slim_trigger);

struct snd_soc_dai_ops escore_slim_port_dai_ops = {
	.set_fmt	= escore_slim_set_dai_fmt,
	.set_channel_map	= escore_slim_set_channel_map,
	.get_channel_map	= escore_slim_get_channel_map,
	.set_tristate	= escore_slim_set_tristate,
	.digital_mute	= escore_slim_port_mute,
	.startup	= escore_slim_startup,
	.shutdown	= escore_slim_shutdown,
	.hw_params	= escore_slim_hw_params,
	.hw_free	= escore_slim_hw_free,
	.prepare	= escore_slim_prepare,
	.trigger	= escore_slim_trigger,
};

int escore_slim_boot_setup(struct escore_priv *escore)
{
	u32 boot_cmd = ES_SLIM_BOOT_CMD;
	u32 boot_ack;
	char msg[4];
	int rc;

	pr_debug("%s()\n", __func__);
	pr_debug("%s(): write ES_BOOT_CMD = 0x%08x\n", __func__, boot_cmd);
	cpu_to_le32s(&boot_cmd);
	memcpy(msg, (char *)&boot_cmd, 4);
	rc = escore->dev_write(escore, msg, 4);
	if (rc < 0) {
		pr_err("%s(): firmware load failed boot write\n", __func__);
		goto escore_bootup_failed;
	}
	usleep_range(1000, 1000);
	memset(msg, 0, 4);
	rc = escore->dev_read(escore, msg, 4);
	if (rc < 0) {
		pr_err("%s(): firmware load failed boot ack\n", __func__);
		goto escore_bootup_failed;
	}
	memcpy((char *)&boot_ack, msg, 4);
	cpu_to_le32s(&boot_ack);
	pr_debug("%s(): boot_ack = 0x%08x\n", __func__, boot_ack);
	if (boot_ack != ES_SLIM_BOOT_ACK) {
		pr_err("%s(): firmware load failed boot ack pattern", __func__);
		rc = -EIO;
		goto escore_bootup_failed;
	}

escore_bootup_failed:
	return rc;
}


static int escore_slim_device_up(struct slim_device *sbdev);
static int escore_fw_thread(void *priv)
{
	struct escore_priv *escore = (struct escore_priv  *)priv;

	do {
		slim_get_logical_addr(escore->gen0_client,
				      escore->gen0_client->e_addr,
				      6, &(escore->gen0_client->laddr));
		usleep_range(1000, 2000);
	} while (escore->gen0_client->laddr == 0xf0);
	dev_dbg(&escore->gen0_client->dev, "%s(): gen0_client LA = %d\n",
		__func__, escore->gen0_client->laddr);
	do {
		slim_get_logical_addr(escore->intf_client,
				      escore->intf_client->e_addr,
				      6, &(escore->intf_client->laddr));
		usleep_range(1000, 2000);
	} while (escore->intf_client->laddr == 0xf0);
	dev_dbg(&escore->intf_client->dev, "%s(): intf_client LA = %d\n",
		__func__, escore->intf_client->laddr);

	escore_slim_device_up(escore->gen0_client);
	return 0;
}

static int escore_slim_probe(struct slim_device *sbdev)
{
	struct esxxx_platform_data *pdata = sbdev->dev.platform_data;
	int rc;
	struct task_struct *thread = NULL;

	dev_dbg(&sbdev->dev, "%s(): sbdev->name = %s\n", __func__, sbdev->name);

	if (strncmp(sbdev->name, "earSmart-codec-intf",
				SLIMBUS_NAME_SIZE) == 0) {
		dev_dbg(&sbdev->dev, "%s(): interface device probe\n",
			__func__);
		escore_priv.intf_client = sbdev;
	}
	if (strncmp(sbdev->name, "earSmart-codec-gen0",
				SLIMBUS_NAME_SIZE) == 0) {
		dev_dbg(&sbdev->dev, "%s(): generic device probe\n",
			__func__);
		escore_priv.gen0_client = sbdev;
	}

	if (escore_priv.intf_client == NULL ||
	    escore_priv.gen0_client == NULL) {
		dev_dbg(&sbdev->dev, "%s() incomplete initialization\n",
			__func__);
		return 0;
	}
	if (pdata == NULL) {
		dev_err(&sbdev->dev, "%s(): pdata is NULL", __func__);
		rc = -EIO;
		goto pdata_error;
	}

	slim_set_clientdata(sbdev, &escore_priv);

	escore_priv.intf = ES_SLIM_INTF;
	escore_priv.dev_read = escore_slim_read;
	escore_priv.dev_write = escore_slim_write;
	escore_priv.cmd = escore_slim_cmd;
	escore_priv.boot_setup = escore_slim_boot_setup;
	escore_priv.dev = &escore_priv.gen0_client->dev;
	rc = escore_priv.probe(&escore_priv.gen0_client->dev);
	if (rc) {
		dev_err(&sbdev->dev, "%s(): escore_core_probe() failed %d\n",
			__func__, rc);
		goto escore_core_probe_error;
	}

#if !defined(ES_DEVICE_UP)
	thread = kthread_run(escore_fw_thread, &escore_priv, "audience thread");
	if (IS_ERR(thread)) {
		dev_err(&sbdev->dev, "%s(): can't create firmware thread:%p\n",
				__func__, thread);
		return -ENOMEM;
	}
#endif

	return 0;

escore_core_probe_error:
pdata_error:
	dev_dbg(&sbdev->dev, "%s(): exit with error\n", __func__);
	return rc;
}

static int escore_slim_remove(struct slim_device *sbdev)
{
	struct esxxx_platform_data *pdata = sbdev->dev.platform_data;

	dev_dbg(&sbdev->dev, "%s(): sbdev->name = %s\n", __func__, sbdev->name);

	gpio_free(pdata->reset_gpio);
	gpio_free(pdata->wakeup_gpio);
	gpio_free(pdata->gpioa_gpio);

	snd_soc_unregister_codec(&sbdev->dev);

	return 0;
}

static int register_snd_soc(struct escore_priv *escore_priv)
{
	int rc;
	struct slim_device *sbdev = escore_priv->gen0_client;

	escore_init_slim_slave(sbdev);

	dev_dbg(&sbdev->dev, "%s(): name = %s\n", __func__, sbdev->name);

	rc = snd_soc_register_codec(&sbdev->dev,
			escore_priv->soc_codec_dev_escore,
			escore_priv->escore_dai, escore_priv->escore_dai_nr);

	dev_dbg(&sbdev->dev, "%s(): rc = snd_soc_regsiter_codec() = %d\n",
		__func__, rc);

	escore_priv->slim_setup(escore_priv);

	return rc;
}

int fw_download(void *arg)
{
	struct escore_priv *escore = (struct escore_priv *)arg;
	int rc;

	rc = escore->bootup(escore);
	release_firmware(escore->fw);

	rc = register_snd_soc(escore);
	pr_debug("%s(): register_snd_soc rc=%d\n", __func__, rc);

	module_put(THIS_MODULE);
	return 0;
}

static int escore_slim_device_up(struct slim_device *sbdev)
{
	struct escore_priv *priv;
	int rc;

	dev_dbg(&sbdev->dev, "%s: name=%s\n", __func__, sbdev->name);
	dev_dbg(&sbdev->dev, "%s: laddr=%d\n", __func__, sbdev->laddr);

	/* Start the firmware download in the workqueue context. */
	priv = slim_get_devicedata(sbdev);
	if (strncmp(sbdev->name, "escore-codec-intf",
				SLIMBUS_NAME_SIZE) == 0)
		return 0;

	rc = fw_download(priv);
	BUG_ON(rc != 0);

	return rc;
}

static int escore_slim_suspend(struct device *dev)
{
	dev_dbg(dev, "%s()\n", __func__);
	return 0;
}

static int escore_slim_resume(struct device *dev)
{
	dev_dbg(dev, "%s()\n", __func__);
	return 0;
}

static int escore_slim_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "%s()\n", __func__);
	return 0;
}

static int escore_slim_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "%s()\n", __func__);
	return 0;
}

static int escore_slim_runtime_idle(struct device *dev)
{
	dev_dbg(dev, "%s()\n", __func__);
	return 0;
}

static const struct dev_pm_ops escore_slim_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(
		escore_slim_suspend,
		escore_slim_resume
	)
	SET_RUNTIME_PM_OPS(
		escore_slim_runtime_suspend,
		escore_slim_runtime_resume,
		escore_slim_runtime_idle
	)
};

static const struct slim_device_id escore_slim_id[] = {
	{ "earSmart-codec", 0 },
	{ "earSmart-codec-intf", 0 },
	{ "earSmart-codec-gen0", 0 },
	{  }
};
MODULE_DEVICE_TABLE(slim, escore_slim_id);

struct slim_driver escore_slim_driver = {
	.driver = {
		.name = "earSmart-codec",
		.owner = THIS_MODULE,
		.pm = &escore_slim_dev_pm_ops,
	},
	.probe = escore_slim_probe,
	.remove = escore_slim_remove,
#if defined(ES_DEVICE_UP)
	.device_up = escore_slim_device_up,
#endif
	.id_table = escore_slim_id,
};

int escore_slimbus_init()
{
	int rc;
	rc = slim_driver_register(&escore_slim_driver);
	if (!rc)
		pr_debug("%s() registered as SLIMBUS", __func__);
	return rc;
}

MODULE_DESCRIPTION("ASoC ES driver");
MODULE_AUTHOR("Greg Clemson <gclemson@audience.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:escore-codec");
