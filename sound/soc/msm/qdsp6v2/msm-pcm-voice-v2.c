/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/control.h>
#include <asm/dma.h>

#include "msm-pcm-voice-v2.h"
#include "q6voice.h"

static struct msm_voice voice_info[VOICE_SESSION_INDEX_MAX];

static struct snd_pcm_hardware msm_pcm_hardware = {

	.info =                 (SNDRV_PCM_INFO_INTERLEAVED |
				SNDRV_PCM_INFO_PAUSE |
				SNDRV_PCM_INFO_RESUME),
	.formats =              SNDRV_PCM_FMTBIT_S16_LE,
	.rates =                SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000,
	.rate_min =             8000,
	.rate_max =             16000,
	.channels_min =         1,
	.channels_max =         1,

	.buffer_bytes_max =     4096 * 2,
	.period_bytes_min =     2048,
	.period_bytes_max =     4096,
	.periods_min =          2,
	.periods_max =          4,

	.fifo_size =            0,
};
static bool is_volte(struct msm_voice *pvolte)
{
	if (pvolte == &voice_info[VOLTE_SESSION_INDEX])
		return true;
	else
		return false;
}

static bool is_voice2(struct msm_voice *pvoice2)
{
	if (pvoice2 == &voice_info[VOICE2_SESSION_INDEX])
		return true;
	else
		return false;
}

static bool is_qchat(struct msm_voice *pqchat)
{
	if (pqchat == &voice_info[QCHAT_SESSION_INDEX])
		return true;
	else
		return false;
}

static bool is_vowlan(struct msm_voice *pvowlan)
{
	if (pvowlan == &voice_info[VOWLAN_SESSION_INDEX])
		return true;
	else
		return false;
}

static uint32_t get_session_id(struct msm_voice *pvoc)
{
	uint32_t session_id = 0;

	if (is_volte(pvoc))
		session_id = voc_get_session_id(VOLTE_SESSION_NAME);
	else if (is_voice2(pvoc))
		session_id = voc_get_session_id(VOICE2_SESSION_NAME);
	else if (is_qchat(pvoc))
		session_id = voc_get_session_id(QCHAT_SESSION_NAME);
	else if (is_vowlan(pvoc))
		session_id = voc_get_session_id(VOWLAN_SESSION_NAME);
	else
		session_id = voc_get_session_id(VOICE_SESSION_NAME);

	return session_id;
}


static int msm_pcm_playback_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msm_voice *prtd = runtime->private_data;

	pr_debug("%s\n", __func__);

	if (!prtd->playback_start)
		prtd->playback_start = 1;

	return 0;
}

static int msm_pcm_capture_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msm_voice *prtd = runtime->private_data;

	pr_debug("%s\n", __func__);

	if (!prtd->capture_start)
		prtd->capture_start = 1;

	return 0;
}
static int msm_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msm_voice *voice;

	if (!strncmp("VoLTE", substream->pcm->id, 5)) {
		voice = &voice_info[VOLTE_SESSION_INDEX];
		pr_debug("%s: Open VoLTE Substream Id=%s\n",
			 __func__, substream->pcm->id);
	} else if (!strncmp("Voice2", substream->pcm->id, 6)) {
		voice = &voice_info[VOICE2_SESSION_INDEX];
		pr_debug("%s: Open Voice2 Substream Id=%s\n",
			 __func__, substream->pcm->id);
	} else if (!strncmp("QCHAT", substream->pcm->id, 5)) {
		voice = &voice_info[QCHAT_SESSION_INDEX];
		pr_debug("%s: Open QCHAT Substream Id=%s\n",
			 __func__, substream->pcm->id);
	} else if (!strncmp("VoWLAN", substream->pcm->id, 6)) {
		voice = &voice_info[VOWLAN_SESSION_INDEX];
		pr_debug("%s: Open VoWLAN Substream Id=%s\n",
			 __func__, substream->pcm->id);
	} else {
		voice = &voice_info[VOICE_SESSION_INDEX];
		pr_debug("%s: Open VOICE Substream Id=%s\n",
			 __func__, substream->pcm->id);
	}
	mutex_lock(&voice->lock);

	runtime->hw = msm_pcm_hardware;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		voice->playback_substream = substream;
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		voice->capture_substream = substream;

	voice->instance++;
	pr_debug("%s: Instance = %d, Stream ID = %s\n",
			__func__ , voice->instance, substream->pcm->id);
	runtime->private_data = voice;

	mutex_unlock(&voice->lock);

	return 0;
}
static int msm_pcm_playback_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msm_voice *prtd = runtime->private_data;

	pr_debug("%s\n", __func__);

	if (prtd->playback_start)
		prtd->playback_start = 0;

	prtd->playback_substream = NULL;

	return 0;
}
static int msm_pcm_capture_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msm_voice *prtd = runtime->private_data;

	pr_debug("%s\n", __func__);

	if (prtd->capture_start)
		prtd->capture_start = 0;
	prtd->capture_substream = NULL;

	return 0;
}
static int msm_pcm_close(struct snd_pcm_substream *substream)
{

	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msm_voice *prtd = runtime->private_data;
	uint32_t session_id = 0;
	int ret = 0;

	mutex_lock(&prtd->lock);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = msm_pcm_playback_close(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		ret = msm_pcm_capture_close(substream);

	prtd->instance--;
	if (!prtd->playback_start && !prtd->capture_start) {
		pr_debug("end voice call\n");

		session_id = get_session_id(prtd);
		if (session_id)
			voc_end_voice_call(session_id);
	}
	mutex_unlock(&prtd->lock);

	return ret;
}
static int msm_pcm_prepare(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msm_voice *prtd = runtime->private_data;
	uint32_t session_id = 0;

	mutex_lock(&prtd->lock);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = msm_pcm_playback_prepare(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		ret = msm_pcm_capture_prepare(substream);

	if (prtd->playback_start && prtd->capture_start) {
		session_id = get_session_id(prtd);
		if (session_id)
			voc_start_voice_call(session_id);
	}
	mutex_unlock(&prtd->lock);

	return ret;
}

static int msm_pcm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{

	pr_debug("%s: Voice\n", __func__);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	return 0;
}

static int msm_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msm_voice *prtd = runtime->private_data;
	uint32_t session_id = 0;

	pr_debug("%s: cmd = %d\n", __func__, cmd);

	session_id = get_session_id(prtd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_STOP:
		pr_debug("Start & Stop Voice call not handled in Trigger.\n");
	break;
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		pr_debug("%s: resume call session_id = %d\n", __func__,
			 session_id);
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			ret = msm_pcm_playback_prepare(substream);
		else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			ret = msm_pcm_capture_prepare(substream);
		if (prtd->playback_start && prtd->capture_start) {
			if (session_id)
				voc_resume_voice_call(session_id);
		}
	break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pr_debug("%s: pause call session_id=%d\n",
			 __func__, session_id);
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (prtd->playback_start)
				prtd->playback_start = 0;
		} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			if (prtd->capture_start)
				prtd->capture_start = 0;
		}
		if (session_id)
			voc_standby_voice_call(session_id);
		break;
	default:
		ret = -EINVAL;
	break;
	}
	return ret;
}

static int msm_pcm_ioctl(struct snd_pcm_substream *substream,
			 unsigned int cmd, void *arg)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct msm_voice *prtd = runtime->private_data;
	uint32_t session_id = get_session_id(prtd);
	enum voice_lch_mode lch_mode;
	int ret = 0;

	switch (cmd) {
	case SNDRV_VOICE_IOCTL_LCH:
		if (copy_from_user(&lch_mode, (void *)arg,
				   sizeof(enum voice_lch_mode))) {
			pr_err("%s: Copy from user failed, size %d\n", __func__,
			       sizeof(enum voice_lch_mode));

			ret = -EFAULT;
			break;
		}

		pr_debug("%s: %s lch_mode:%d\n",
			 __func__, substream->pcm->id, lch_mode);

		switch (lch_mode) {
		case VOICE_LCH_START:
		case VOICE_LCH_STOP:
			ret = voc_set_lch(session_id, lch_mode);
			break;

		default:
			pr_err("%s: Invalid LCH MODE %d\n", __func__, lch_mode);

			ret = -EFAULT;
		}

		break;
	default:
		pr_debug("%s: Falling into default snd_lib_ioctl cmd 0x%x\n",
			 __func__, cmd);

		ret = snd_pcm_lib_ioctl(substream, cmd, arg);
		break;
	}

	if (!ret)
		pr_debug("%s: ret %d\n", __func__, ret);
	else
		pr_err("%s: cmd 0x%x failed %d\n", __func__, cmd, ret);

	return ret;
}

#ifdef CONFIG_SND_SOC_ES705
int es705_put_veq_block(int volume);
#endif
#if defined(CONFIG_SND_SOC_ES325) || defined (CONFIG_SND_SOC_ES325_ATLANTIC) && !defined(CONFIG_SEC_LOCALE_KOR_H) && !defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
int es325_set_VEQ_max_gain(int volume);
#endif
static int msm_voice_gain_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
#if defined(CONFIG_SEC_DEVIDE_RINGTONE_GAIN)
	int ret = 0;
	int volume = ucontrol->value.integer.value[0];
	uint32_t session_id = ucontrol->value.integer.value[1];
	int ramp_duration = ucontrol->value.integer.value[2];
	bool is_ringback = false;

	if ((volume < 0) || (ramp_duration < 0)
		|| (ramp_duration > MAX_RAMP_DURATION)) {
		pr_err(" %s Invalid arguments", __func__);

		ret = -EINVAL;
		goto done;
	}

	if (volume & 0x200) {
		pr_debug("%s: ringback tone volume ctrl\n", __func__);
		volume -= 0x200;
		is_ringback = true;
	}

	pr_debug("%s: volume: %d session_id: %#x ramp_duration: %d\n", __func__,
		volume, session_id, ramp_duration);

	voc_set_rx_vol_step(session_id, RX_PATH,
			(is_ringback ? (volume+100) : volume), ramp_duration);
#else
	int ret = 0;
	int volume = ucontrol->value.integer.value[0];
	uint32_t session_id = ucontrol->value.integer.value[1];
	int ramp_duration = ucontrol->value.integer.value[2];

	if ((volume < 0) || (ramp_duration < 0)
		|| (ramp_duration > MAX_RAMP_DURATION)) {
		pr_err(" %s Invalid arguments", __func__);

		ret = -EINVAL;
		goto done;
	}

	pr_debug("%s: volume: %d session_id: %#x ramp_duration: %d\n", __func__,
		volume, session_id, ramp_duration);

	voc_set_rx_vol_step(session_id, RX_PATH, volume, ramp_duration);
#endif

#ifdef CONFIG_SND_SOC_ES705
#if !defined(CONFIG_MACH_KLTE_KOR)
	es705_put_veq_block(volume);
#endif
#endif
#if defined(CONFIG_SND_SOC_ES325) || defined (CONFIG_SND_SOC_ES325_ATLANTIC) && !defined(CONFIG_SEC_LOCALE_KOR_H) && !defined(CONFIG_SEC_LOCALE_KOR_FRESCO) && !defined(CONFIG_SEC_JACTIVE_PROJECT)
	es325_set_VEQ_max_gain(volume);
#endif
done:
	return ret;
}

static int msm_voice_mute_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	int mute = ucontrol->value.integer.value[0];
	uint32_t session_id = ucontrol->value.integer.value[1];
	int ramp_duration = ucontrol->value.integer.value[2];

	if ((mute < 0) || (mute > 1) || (ramp_duration < 0)
		|| (ramp_duration > MAX_RAMP_DURATION)) {
		pr_err(" %s Invalid arguments", __func__);

		ret = -EINVAL;
		goto done;
	}

	pr_debug("%s: mute=%d session_id=%#x ramp_duration=%d\n", __func__,
		mute, session_id, ramp_duration);

	ret = voc_set_tx_mute(session_id, TX_PATH, mute, ramp_duration);

done:
	return ret;
}

static int msm_voice_tx_device_mute_put(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	int mute = ucontrol->value.integer.value[0];
	uint32_t session_id = ucontrol->value.integer.value[1];
	int ramp_duration = ucontrol->value.integer.value[2];

	if ((mute < 0) || (mute > 1) || (ramp_duration < 0) ||
	    (ramp_duration > MAX_RAMP_DURATION)) {
		pr_err(" %s Invalid arguments", __func__);

		ret = -EINVAL;
		goto done;
	}

	pr_debug("%s: mute=%d session_id=%#x ramp_duration=%d\n", __func__,
		 mute, session_id, ramp_duration);

	ret = voc_set_device_mute(session_id, VSS_IVOLUME_DIRECTION_TX,
				  mute, ramp_duration);

done:
	return ret;
}

static int msm_voice_rx_device_mute_put(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	int mute = ucontrol->value.integer.value[0];
	uint32_t session_id = ucontrol->value.integer.value[1];
	int ramp_duration = ucontrol->value.integer.value[2];

	if ((mute < 0) || (mute > 1) || (ramp_duration < 0) ||
	    (ramp_duration > MAX_RAMP_DURATION)) {
		pr_err(" %s Invalid arguments", __func__);

		ret = -EINVAL;
		goto done;
	}

	pr_debug("%s: mute=%d session_id=%#x ramp_duration=%d\n", __func__,
		 mute, session_id, ramp_duration);

	voc_set_device_mute(session_id, VSS_IVOLUME_DIRECTION_RX,
			    mute, ramp_duration);

done:
	return ret;
}

static int msm_loopback_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int loopback_enable = ucontrol->value.integer.value[0];

	pr_debug("%s: loopback enable=%d\n", __func__, loopback_enable);

	voc_set_loopback_enable(loopback_enable);
	return 0;
}

static int msm_loopback_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = voc_get_loopback_enable();
	 return 0;
}

static int msm_roaming_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int roaming_enable = ucontrol->value.integer.value[0];

	pr_debug("%s: roaming enable=%d\n", __func__, roaming_enable);

	voc_set_roaming_enable(roaming_enable);
	return 0;
}

static int msm_roaming_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = voc_get_roaming_enable();
	 return 0;
}

static const char const *tty_mode[] = {"OFF", "HCO", "VCO", "FULL"};
static const struct soc_enum msm_tty_mode_enum[] = {
		SOC_ENUM_SINGLE_EXT(4, tty_mode),
};

static int msm_voice_tty_mode_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] =
		voc_get_tty_mode(voc_get_session_id(VOICE_SESSION_NAME));
	return 0;
}

static int msm_voice_tty_mode_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int tty_mode = ucontrol->value.integer.value[0];

	pr_debug("%s: tty_mode=%d\n", __func__, tty_mode);

	voc_set_tty_mode(voc_get_session_id(VOICE_SESSION_NAME), tty_mode);
	voc_set_tty_mode(voc_get_session_id(VOICE2_SESSION_NAME), tty_mode);
	voc_set_tty_mode(voc_get_session_id(VOLTE_SESSION_NAME), tty_mode);
	voc_set_tty_mode(voc_get_session_id(VOWLAN_SESSION_NAME), tty_mode);

	return 0;
}

static int msm_voice_slowtalk_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int st_enable = ucontrol->value.integer.value[0];
	uint32_t session_id = ucontrol->value.integer.value[1];

	pr_debug("%s: st enable=%d session_id=%#x\n", __func__, st_enable,
		 session_id);

	voc_set_pp_enable(session_id,
			  MODULE_ID_VOICE_MODULE_ST, st_enable);

	return 0;
}

#ifdef CONFIG_SEC_DHA_SOL_MAL
static int msm_sec_dha_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

static int msm_sec_dha_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int i = 0;

	int dha_mode = ucontrol->value.integer.value[0];
	int dha_select = ucontrol->value.integer.value[1];
#if defined(CONFIG_AUDIO_DUAL_CP) || defined(CONFIG_MACH_KLTE_CTC) \
	|| defined(CONFIG_MACH_KLTE_CMCCDUOS) || defined(CONFIG_MACH_KLTE_CUDUOS) \
	|| defined(CONFIG_MACH_MEGA23GEUR_OPEN)  || defined(CONFIG_MACH_MS01_EUR_3G) \
	|| defined(CONFIG_MACH_MEGA2LTE_KTT) || defined(CONFIG_MACH_ATLANTIC3GEUR_OPEN) \
	|| defined(CONFIG_MACH_MS01_EUR_LTE) || defined(CONFIG_MACH_MS01_KOR_LTE) || defined(CONFIG_DSDA_VIA_UART) || defined(CONFIG_MACH_KLTE_LTNDUOS)
	uint32_t session_id = ucontrol->value.integer.value[14];
#endif
	short dha_param[12] = {0,};
	for (i = 0; i < 12; i++) {
		dha_param[i] = (short)ucontrol->value.integer.value[2+i];
		pr_debug("msm_dha_put : param - %d\n", dha_param[i]);
	}

#if defined(CONFIG_AUDIO_DUAL_CP) || defined(CONFIG_MACH_KLTE_CTC) \
	|| defined(CONFIG_MACH_KLTE_CMCCDUOS) || defined(CONFIG_MACH_KLTE_CUDUOS) \
	|| defined(CONFIG_MACH_MEGA23GEUR_OPEN) || defined(CONFIG_MACH_MS01_EUR_3G) \
	|| defined(CONFIG_MACH_MEGA2LTE_KTT) || defined(CONFIG_MACH_ATLANTIC3GEUR_OPEN) \
	|| defined(CONFIG_MACH_MS01_EUR_LTE) || defined(CONFIG_MACH_MS01_KOR_LTE) || defined(CONFIG_DSDA_VIA_UART) || defined(CONFIG_MACH_KLTE_LTNDUOS)
	pr_info("%s: session_id=%#x\n", __func__, session_id);
	return voice_sec_set_dha_data(session_id,
		dha_mode, dha_select, dha_param);
#else
	return voice_sec_set_dha_data(voc_get_session_id(VOICE_SESSION_NAME),
		dha_mode, dha_select, dha_param);
#endif
}
#endif /*CONFIG_SEC_DHA_SOL_MAL*/

static int msm_voice_slowtalk_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] =
		voc_get_pp_enable(voc_get_session_id(VOICE_SESSION_NAME),
				MODULE_ID_VOICE_MODULE_ST);
	return 0;
}

static struct snd_kcontrol_new msm_voice_controls[] = {
	SOC_SINGLE_MULTI_EXT("Voice Rx Device Mute", SND_SOC_NOPM, 0, VSID_MAX,
				0, 3, NULL, msm_voice_rx_device_mute_put),
	SOC_SINGLE_MULTI_EXT("Voice Tx Device Mute", SND_SOC_NOPM, 0, VSID_MAX,
				0, 3, NULL, msm_voice_tx_device_mute_put),
	SOC_SINGLE_MULTI_EXT("Voice Tx Mute", SND_SOC_NOPM, 0, VSID_MAX,
				0, 3, NULL, msm_voice_mute_put),
	SOC_SINGLE_MULTI_EXT("Voice Rx Gain", SND_SOC_NOPM, 0, VSID_MAX, 0, 3,
				NULL, msm_voice_gain_put),
	SOC_ENUM_EXT("TTY Mode", msm_tty_mode_enum[0], msm_voice_tty_mode_get,
				msm_voice_tty_mode_put),
	SOC_SINGLE_MULTI_EXT("Slowtalk Enable", SND_SOC_NOPM, 0, VSID_MAX, 0, 2,
				msm_voice_slowtalk_get, msm_voice_slowtalk_put),
#ifdef CONFIG_SEC_DHA_SOL_MAL
#if defined(CONFIG_AUDIO_DUAL_CP) || defined(CONFIG_MACH_KLTE_CTC) \
	|| defined(CONFIG_MACH_KLTE_CMCCDUOS) || defined(CONFIG_MACH_KLTE_CUDUOS) \
	|| defined(CONFIG_MACH_MEGA23GEUR_OPEN) || defined(CONFIG_MACH_MS01_EUR_3G) \
	|| defined(CONFIG_MACH_MEGA2LTE_KTT) || defined(CONFIG_MACH_ATLANTIC3GEUR_OPEN) \
	|| defined(CONFIG_MACH_MS01_EUR_LTE) || defined(CONFIG_MACH_MS01_KOR_LTE) || defined(CONFIG_DSDA_VIA_UART) || defined(CONFIG_MACH_KLTE_LTNDUOS)
	SOC_SINGLE_MULTI_EXT("Sec Set DHA data", SND_SOC_NOPM, 0, VSID_MAX, 0, 15,
				msm_sec_dha_get, msm_sec_dha_put),
#else
	SOC_SINGLE_MULTI_EXT("Sec Set DHA data", SND_SOC_NOPM, 0, 65535, 0, 14,
				msm_sec_dha_get, msm_sec_dha_put),
#endif
#endif	/* CONFIG_SEC_DHA_SOL_MAL */
	SOC_SINGLE_EXT("Loopback Enable", SND_SOC_NOPM, 0, 1, 0,
				msm_loopback_get, msm_loopback_put),
	SOC_SINGLE_EXT("Roaming Enable", SND_SOC_NOPM, 0, 1, 0,
			msm_roaming_get, msm_roaming_put),
};

static struct snd_pcm_ops msm_pcm_ops = {
	.open			= msm_pcm_open,
	.hw_params		= msm_pcm_hw_params,
	.close			= msm_pcm_close,
	.prepare		= msm_pcm_prepare,
	.trigger		= msm_pcm_trigger,
	.ioctl			= msm_pcm_ioctl,
};


static int msm_asoc_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	int ret = 0;

	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);
	return ret;
}

static int msm_pcm_voice_probe(struct snd_soc_platform *platform)
{
	snd_soc_add_platform_controls(platform, msm_voice_controls,
					ARRAY_SIZE(msm_voice_controls));

	return 0;
}

static struct snd_soc_platform_driver msm_soc_platform = {
	.ops		= &msm_pcm_ops,
	.pcm_new	= msm_asoc_pcm_new,
	.probe		= msm_pcm_voice_probe,
};

static __devinit int msm_pcm_probe(struct platform_device *pdev)
{
	int rc;

	if (!is_voc_initialized()) {
		pr_debug("%s: voice module not initialized yet, deferring probe()\n",
		       __func__);

		rc = -EPROBE_DEFER;
		goto done;
	}

	rc = voc_alloc_cal_shared_memory();
	if (rc == -EPROBE_DEFER) {
		pr_debug("%s: memory allocation for calibration deferred %d\n",
			 __func__, rc);

		goto done;
	} else if (rc < 0) {
		pr_err("%s: memory allocation for calibration failed %d\n",
		       __func__, rc);
	}

	if (pdev->dev.of_node)
		dev_set_name(&pdev->dev, "%s", "msm-pcm-voice");

	pr_debug("%s: dev name %s\n", __func__, dev_name(&pdev->dev));
	rc = snd_soc_register_platform(&pdev->dev,
				       &msm_soc_platform);

done:
	return rc;
}

static int msm_pcm_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static const struct of_device_id msm_voice_dt_match[] = {
	{.compatible = "qcom,msm-pcm-voice"},
	{}
};
MODULE_DEVICE_TABLE(of, msm_voice_dt_match);

static struct platform_driver msm_pcm_driver = {
	.driver = {
		.name = "msm-pcm-voice",
		.owner = THIS_MODULE,
		.of_match_table = msm_voice_dt_match,
	},
	.probe = msm_pcm_probe,
	.remove = __devexit_p(msm_pcm_remove),
};

static int __init msm_soc_platform_init(void)
{
	int i = 0;

	memset(&voice_info, 0, sizeof(voice_info));

	for (i = 0; i < VOICE_SESSION_INDEX_MAX; i++)
		mutex_init(&voice_info[i].lock);

	return platform_driver_register(&msm_pcm_driver);
}
module_init(msm_soc_platform_init);

static void __exit msm_soc_platform_exit(void)
{
	platform_driver_unregister(&msm_pcm_driver);
}
module_exit(msm_soc_platform_exit);

MODULE_DESCRIPTION("Voice PCM module platform driver");
MODULE_LICENSE("GPL v2");
