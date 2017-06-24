/*
 * max98504.c -- MAX98504 ALSA SoC Audio driver
 *
 * Copyright 2011-2012 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/max98504.h>
#include "max98504.h"
#include <linux/regulator/consumer.h>

#ifdef DEBUG_MAX98504
#define msg_maxim(format, args...)	\
	printk(KERN_INFO "[MAX98504_DEBUG] %s " format, __func__, ## args)
#else
#define msg_maxim(format, args...)
#endif

static const u8 max98504_reg_def[MAX98504_REG_CNT] = {
	[MAX98504_REG_01_INTERRUPT_STATUS] = 0,
	[MAX98504_REG_02_INTERRUPT_FLAGS] = 0,
	[MAX98504_REG_03_INTERRUPT_ENABLES] = 0,
	[MAX98504_REG_04_INTERRUPT_FLAG_CLEARS] = 0,
	[MAX98504_REG_10_GPIO_ENABLE] = 0,
	[MAX98504_REG_11_GPIO_CONFIG] = 0,
	[MAX98504_REG_12_WATCHDOG_ENABLE] = 0,
	[MAX98504_REG_13_WATCHDOG_CONFIG] = 0,
	[MAX98504_REG_14_WATCHDOG_CLEAR] = 0,
	[MAX98504_REG_15_CLOCK_MONITOR_ENABLE] = 0,
	[MAX98504_REG_16_PVDD_BROWNOUT_ENABLE] = 0,
	[MAX98504_REG_17_PVDD_BROWNOUT_CONFIG_1] = 0,
	[MAX98504_REG_18_PVDD_BROWNOUT_CONFIG_2] = 0,
	[MAX98504_REG_19_PVDD_BROWNOUT_CONFIG_3] = 0,
	[MAX98504_REG_1A_PVDD_BROWNOUT_CONFIG_4] = 0,
	[MAX98504_REG_20_PCM_RX_ENABLES] = 0,
	[MAX98504_REG_21_PCM_TX_ENABLES] = 0,
	[MAX98504_REG_22_PCM_TX_HIZ_CONTROL] = 0,
	[MAX98504_REG_23_PCM_TX_CHANNEL_SOURCES] = 0,
	[MAX98504_REG_24_PCM_MODE_CONFIG] = 0,
	[MAX98504_REG_25_PCM_DSP_CONFIG] = 0,
	[MAX98504_REG_26_PCM_CLOCK_SETUP] = 0,
	[MAX98504_REG_27_PCM_SAMPLE_RATE_SETUP] = 0,
	[MAX98504_REG_28_PCM_TO_SPEAKER_MONOMIX] = 0,
	[MAX98504_REG_30_PDM_TX_ENABLES] = 0,
	[MAX98504_REG_31_PDM_TX_HIZ_CONTROL] = 0,
	[MAX98504_REG_32_PDM_TX_CONTROL] = 0,
	[MAX98504_REG_33_PDM_RX_ENABLE] = 0,
	[MAX98504_REG_34_SPEAKER_ENABLE] = 0,
	[MAX98504_REG_35_SPEAKER_SOURCE_SELECT] = 0,
	[MAX98504_REG_36_MEASUREMENT_ENABLES] = 0,
	[MAX98504_REG_37_ANALOGUE_INPUT_GAIN] = 0,
	[MAX98504_REG_38_TEMPERATURE_LIMIT_CONFIG] = 0,
	[MAX98504_REG_39_ANALOGUE_SPARE] = 0,
	[MAX98504_REG_40_GLOBAL_ENABLE] = 0,
	[MAX98504_REG_41_SOFTWARE_RESET] = 0,
};

static struct {
	u8 read;
	u8 write;
	u8 vol;
} max98504_reg_access[MAX98504_REG_CNT] = {
	[MAX98504_REG_01_INTERRUPT_STATUS] = { 0xFF, 0x00, 0xFF },
	[MAX98504_REG_02_INTERRUPT_FLAGS] = { 0xFF, 0x00, 0xFF },
	[MAX98504_REG_03_INTERRUPT_ENABLES] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_04_INTERRUPT_FLAG_CLEARS] = { 0x00, 0xFF, 0xFF },
	[MAX98504_REG_10_GPIO_ENABLE] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_11_GPIO_CONFIG] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_12_WATCHDOG_ENABLE] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_13_WATCHDOG_CONFIG] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_14_WATCHDOG_CLEAR] = { 0x00, 0xFF, 0xFF },
	[MAX98504_REG_15_CLOCK_MONITOR_ENABLE] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_16_PVDD_BROWNOUT_ENABLE] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_17_PVDD_BROWNOUT_CONFIG_1] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_18_PVDD_BROWNOUT_CONFIG_2] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_19_PVDD_BROWNOUT_CONFIG_3] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_1A_PVDD_BROWNOUT_CONFIG_4] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_20_PCM_RX_ENABLES] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_21_PCM_TX_ENABLES] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_22_PCM_TX_HIZ_CONTROL] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_23_PCM_TX_CHANNEL_SOURCES] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_24_PCM_MODE_CONFIG] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_25_PCM_DSP_CONFIG] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_26_PCM_CLOCK_SETUP] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_27_PCM_SAMPLE_RATE_SETUP] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_28_PCM_TO_SPEAKER_MONOMIX] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_30_PDM_TX_ENABLES] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_31_PDM_TX_HIZ_CONTROL] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_32_PDM_TX_CONTROL] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_33_PDM_RX_ENABLE] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_34_SPEAKER_ENABLE] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_35_SPEAKER_SOURCE_SELECT] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_36_MEASUREMENT_ENABLES] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_37_ANALOGUE_INPUT_GAIN] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_38_TEMPERATURE_LIMIT_CONFIG] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_39_ANALOGUE_SPARE] = { 0xFF, 0xFF, 0x00 },
	[MAX98504_REG_40_GLOBAL_ENABLE] = { 0xFF, 0xFF, 0xFF },
	[MAX98504_REG_41_SOFTWARE_RESET] = { 0x00, 0xFF, 0xFF },
};

static int max98504_volatile_register
	(struct snd_soc_codec *codec, unsigned int reg)
{
	if (max98504_reg_access[reg].vol) {
		return 1;
	} else {
		/* Mark all volatile for 2nd Ev Kit i2c master */
		return 0;
	}
}

static int max98504_readable(struct snd_soc_codec *codec, unsigned int reg)
{
	if (reg >= MAX98504_REG_CNT)
		return 0;

	return max98504_reg_access[reg].read != 0;
}

static int max98504_reset(struct snd_soc_codec *codec)
{
	int ret;
	msg_maxim("\n");

	/* Reset the codec by writing to this write-only reset register */
	ret = snd_soc_write(codec, MAX98504_REG_41_SOFTWARE_RESET,
		M98504_SOFTWARE_RESET_MASK);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to reset codec: %d\n", ret);
		return ret;
	}

	msleep(20);

	return ret;
}

#ifdef USE_MAX98504_IRQ
static irqreturn_t max98504_interrupt(int irq, void *data)
{
	struct max98504_priv *max98504 = (struct max98504_priv *) data;

	unsigned int mask;
	unsigned int flag;

	regmap_read(max98504->regmap, MAX98504_REG_03_INTERRUPT_ENABLES, &mask);
	regmap_read(max98504->regmap, MAX98504_REG_02_INTERRUPT_FLAGS, &flag);

	msg_maxim("flag=0x%02x mask=0x%02x -> flag=0x%02x\n",
		flag, mask, flag & mask);

	flag &= mask;

	if (!flag)
		return IRQ_NONE;

	/* Send work to be scheduled */
	if (flag & M98504_INT_GENFAIL_EN_MASK)
		msg_maxim("M98504_INT_GENFAIL_EN_MASK active!");

	if (flag & M98504_INT_AUTHDONE_EN_MASK)
		msg_maxim("M98504_INT_AUTHDONE_EN_MASK active!");

	if (flag & M98504_INT_VBATBROWN_EN_MASK)
		msg_maxim("M98504_INT_VBATBROWN_EN_MASK active!");

	if (flag & M98504_INT_WATCHFAIL_EN_MASK)
		msg_maxim("M98504_INT_WATCHFAIL_EN_MASK active!");

	if (flag & M98504_INT_THERMWARN_END_EN_MASK)
		msg_maxim("M98504_INT_THERMWARN_END_EN_MASK active!");

	if (flag & M98504_INT_THERMWARN_BGN_EN_MASK)
		msg_maxim("M98504_INT_THERMWARN_BGN_EN_MASK active!\n");

	if (flag & M98504_INT_THERMSHDN_END_EN_MASK)
		msg_maxim("M98504_INT_THERMSHDN_END_EN_MASK active!\n");

	if (flag & M98504_INT_THERMSHDN_BGN_FLAG_MASK)
		msg_maxim("M98504_INT_THERMSHDN_BGN_FLAG_MASK active!\n");

	regmap_write(max98504->regmap, MAX98504_REG_04_INTERRUPT_FLAG_CLEARS,
		flag&0xff);

	return IRQ_HANDLED;
}
#endif

static int max98504_rxpcm_gain_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int sel = ucontrol->value.integer.value[0];

	msg_maxim("val=%d\n", sel);

	snd_soc_update_bits(codec, MAX98504_REG_25_PCM_DSP_CONFIG,
	    M98504_PCM_DSP_CFG_RX_GAIN_MASK,
		sel << M98504_PCM_DSP_CFG_RX_GAIN_SHIFT);

	return 0;
}

static int max98504_rxpcm_gain_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int val = snd_soc_read(codec, MAX98504_REG_25_PCM_DSP_CONFIG);

	val = (val & M98504_PCM_DSP_CFG_RX_GAIN_MASK) \
		>> M98504_PCM_DSP_CFG_RX_GAIN_SHIFT;

	ucontrol->value.integer.value[0] = val;
	msg_maxim("val=%d\n", val);

	return 0;
}

static int max98504_ain_gain_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int sel = ucontrol->value.integer.value[0];

	msg_maxim("val=%d\n", sel);

	snd_soc_update_bits(codec,
		MAX98504_REG_37_ANALOGUE_INPUT_GAIN,
		M98504_ANALOG_INPUT_GAIN_MASK,
		sel << M98504_ANALOG_INPUT_GAIN_SHIFT);
	return 0;
}

static int max98504_ain_gain_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int val =
		snd_soc_read(codec, MAX98504_REG_37_ANALOGUE_INPUT_GAIN);

	val = (val & M98504_ANALOG_INPUT_GAIN_MASK) \
		>> M98504_ANALOG_INPUT_GAIN_SHIFT;

	ucontrol->value.integer.value[0] = val;
	msg_maxim("val=%d\n", val);

	return 0;
}

static const unsigned int max98504_rxpcm_gain_tlv[] = {
	TLV_DB_RANGE_HEAD(1),
	0, 12, TLV_DB_SCALE_ITEM(0, 100, 0),
};

static const unsigned int max98504_ain_gain_tlv[] = {
	TLV_DB_RANGE_HEAD(1),
	0, 1, TLV_DB_SCALE_ITEM(1200, 600, 0),
};

static const char * const max98504_enableddisabled_text[] =\
	{"Disabled", "Enabled"};

static const struct soc_enum max98504_ispken_enum =
	SOC_ENUM_SINGLE(MAX98504_REG_36_MEASUREMENT_ENABLES,
		M98504_MEAS_I_EN_MASK,
		ARRAY_SIZE(max98504_enableddisabled_text),
		max98504_enableddisabled_text);

static const struct soc_enum max98504_vspken_enum =
	SOC_ENUM_SINGLE(MAX98504_REG_36_MEASUREMENT_ENABLES,
		M98504_MEAS_V_EN_MASK,
		ARRAY_SIZE(max98504_enableddisabled_text),
		max98504_enableddisabled_text);

static const char * const max98504_vbatbrown_code_text[] = \
	{"2.6V", "2.65V", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "3.7V"};

static const struct soc_enum max98504_brownout_code_enum =
	SOC_ENUM_SINGLE(MAX98504_REG_17_PVDD_BROWNOUT_CONFIG_1,
	M98504_PVDD_BROWNOUT_CFG1_CODE_SHIFT, 31, max98504_vbatbrown_code_text);

static const char * const max98504_vbatbrown_max_atten_text[] =\
	{"0dB", "1dB", "2dB", "3dB", "4dB", "5dB", "6dB"};

static const struct soc_enum max98504_brownout_max_atten_enum =
	SOC_ENUM_SINGLE(MAX98504_REG_17_PVDD_BROWNOUT_CONFIG_1,
		M98504_PVDD_BROWNOUT_CFG1_MAX_ATTEN_SHIFT,
		6, max98504_vbatbrown_max_atten_text);

static const char * const max98504_flt_mode_text[] = {"Voice", "Music"};

static const struct soc_enum max98504_pcm_rx_flt_mode_enum =
	SOC_ENUM_SINGLE(MAX98504_REG_25_PCM_DSP_CONFIG,
		M98504_PCM_DSP_CFG_RX_FLT_MODE_SHIFT,
		1, max98504_flt_mode_text);

static const char * const max98504_pcm_bsel_text[] =\
	{"Reserved", "Reserved", "32", "48", "64",\
	"Reserved", "128", "Reserved", "256"};

static const struct soc_enum max98504_pcm_bsel_enum =
	SOC_ENUM_SINGLE(MAX98504_REG_26_PCM_CLOCK_SETUP,
		M98504_PCM_CLK_SETUP_BSEL_SHIFT, 8, max98504_pcm_bsel_text);


static int max98504_set_speaker(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol) {
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98504_priv *max98504 = snd_soc_codec_get_drvdata(codec);

	uint32_t OnOff;

	OnOff = ucontrol->value.integer.value[0];
	msg_maxim("%s, OnOff:%d\n", __func__, OnOff);

	if (OnOff)	{
		snd_soc_update_bits(codec, MAX98504_REG_34_SPEAKER_ENABLE,
			M98504_SPK_EN_MASK, M98504_SPK_EN_MASK);
		snd_soc_update_bits(codec, MAX98504_REG_40_GLOBAL_ENABLE,
			M98504_GLOBAL_EN_MASK, M98504_GLOBAL_EN_MASK);
		max98504->status = 1;
	} else	{
		snd_soc_update_bits(codec, MAX98504_REG_40_GLOBAL_ENABLE,
			M98504_GLOBAL_EN_MASK, 0);
		snd_soc_update_bits(codec, MAX98504_REG_34_SPEAKER_ENABLE,
			M98504_SPK_EN_MASK, 0);
		max98504->status = 0;
	}
	return 0;
}

static int max98504_get_speaker(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol) {
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98504_priv *max98504 = snd_soc_codec_get_drvdata(codec);

	if (max98504->status > 0)
		ucontrol->value.integer.value[0] = 1;
	else
		ucontrol->value.integer.value[0] = 0;

	msg_maxim("%s, OnOff:%d\n", __func__,
		(int)ucontrol->value.integer.value[0]);

	return 0;
}

static const char * const spk_state_text[] = {"Disable", "Enable"};

static const struct soc_enum spk_state_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spk_state_text), spk_state_text),
};

#ifdef USE_DSM_LOG
#define DEFAULT_LOG_CLASS_NAME "dsm"
static const char *class_name_log;
static int max98504_get_dump_status(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = maxdsm_get_dump_status();
	return 0;
}
static int max98504_set_dump_status(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	unsigned int val = snd_soc_read(codec, MAX98504_REG_40_GLOBAL_ENABLE);

	if (val != 0)
		maxdsm_update_param();
	else
		msg_maxim("val:%d\n", val);

	return 0;
}
static ssize_t max98504_log_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return maxdsm_log_prepare(buf);
}

static DEVICE_ATTR(dsm_log, S_IRUGO, max98504_log_show, NULL);
static struct attribute *max98504_attributes[] = {
	&dev_attr_dsm_log.attr,
	NULL
};

static struct attribute_group max98504_attribute_group = {
	.attrs = max98504_attributes
};
#endif

static const struct snd_kcontrol_new max98504_snd_controls[] = {
	SOC_SINGLE("GPIO Pin Switch", MAX98504_REG_10_GPIO_ENABLE
		, M98504_GPIO_ENALBE_SHIFT, 1, 0),
	SOC_SINGLE("Watchdog Enable Switch", MAX98504_REG_12_WATCHDOG_ENABLE
		, M98504_WDOG_ENABLE_SHIFT, 1, 0),
	SOC_SINGLE("Watchdog Config Switch", MAX98504_REG_13_WATCHDOG_CONFIG
		, M98504_WDOG_CONFIG_SHIFT, 3, 0),
	SOC_SINGLE("Watchdog Clear Switch", MAX98504_REG_14_WATCHDOG_CLEAR
		, M98504_WDOG_CLEAR_SHIFT, 0xe9, 0),
	SOC_SINGLE("Clock Monitor Switch", MAX98504_REG_15_CLOCK_MONITOR_ENABLE
		, M98504_CMON_ENA_SHIFT, 1, 0),
	SOC_SINGLE("Brownout Protection Switch",
		MAX98504_REG_16_PVDD_BROWNOUT_ENABLE,
		M98504_CMON_ENA_SHIFT, 1, 0),
	SOC_ENUM("Brownout Threshold", max98504_brownout_code_enum),
	SOC_ENUM("Brownout Attenuation Value",
		max98504_brownout_max_atten_enum),
	SOC_SINGLE("Brownout Attack Hold Time",
		MAX98504_REG_18_PVDD_BROWNOUT_CONFIG_2,
		M98504_PVDD_BROWNOUT_CFG2_ATTK_HOLD_SHIFT, 255, 0),
	SOC_SINGLE("Brownout Timed Hold",
		MAX98504_REG_19_PVDD_BROWNOUT_CONFIG_3,
		M98504_PVDD_BROWNOUT_CFG3_TIMED_HOLD_SHIFT, 255, 0),
	SOC_SINGLE("Brownout Release", MAX98504_REG_1A_PVDD_BROWNOUT_CONFIG_4,
		M98504_PVDD_BROWNOUT_CFG4_RELEASE_SHIFT, 255, 0),
	SOC_SINGLE("PCM BCLK Edge", MAX98504_REG_24_PCM_MODE_CONFIG,
		M98504_PCM_MODE_CFG_BCLKEDGE_SHIFT, 1, 0),
	SOC_SINGLE("PCM Channel Select", MAX98504_REG_24_PCM_MODE_CONFIG,
		M98504_PCM_MODE_CFG_CHSEL_SHIFT, 1, 0),
	SOC_SINGLE("PCM Transmit Extra HiZ Switch",
		MAX98504_REG_24_PCM_MODE_CONFIG,
		M98504_PCM_MODE_CFG_TX_EXTRA_HIZ_SHIFT, 1, 0),
	SOC_SINGLE("PCM Output Dither Switch", MAX98504_REG_25_PCM_DSP_CONFIG,
		M98504_PCM_DSP_CFG_TX_DITH_EN_SHIFT, 1, 0),
	SOC_SINGLE("PCM Measurement DC Blocking Filter Switch",
		MAX98504_REG_25_PCM_DSP_CONFIG,
		M98504_PCM_DSP_CFG_MEAS_DCBLK_EN_SHIFT, 1, 0),
	SOC_SINGLE("PCM Input Dither Switch", MAX98504_REG_25_PCM_DSP_CONFIG,
		M98504_PCM_DSP_CFG_RX_DITH_EN_SHIFT, 1, 0),
	SOC_ENUM("PCM Output Filter Mode", max98504_pcm_rx_flt_mode_enum),
	SOC_SINGLE_EXT_TLV("PCM Rx Gain", MAX98504_REG_25_PCM_DSP_CONFIG,
		M98504_PCM_DSP_CFG_RX_GAIN_SHIFT,
		M98504_PCM_DSP_CFG_RX_GAIN_WIDTH - 1,
		1, max98504_rxpcm_gain_get, max98504_rxpcm_gain_set,
		max98504_rxpcm_gain_tlv),
	SOC_ENUM("PCM BCLK rate", max98504_pcm_bsel_enum),
	SOC_ENUM("Speaker Current Sense Enable", max98504_ispken_enum),
	SOC_ENUM("Speaker Voltage Sense Enable", max98504_vspken_enum),

	SOC_SINGLE_EXT_TLV("AIN Gain", MAX98504_REG_37_ANALOGUE_INPUT_GAIN,
		M98504_ANALOG_INPUT_GAIN_SHIFT,
		M98504_ANALOG_INPUT_GAIN_WIDTH - 1,
		1, max98504_ain_gain_get, max98504_ain_gain_set,
		max98504_ain_gain_tlv),
	SOC_SINGLE("AUTH_STATUS", MAX98504_REG_01_INTERRUPT_STATUS,
		0, M98504_INT_INTERRUPT_STATUS_MASK, 0),

	SOC_ENUM_EXT("SPK out", spk_state_enum[0],
		max98504_get_speaker, max98504_set_speaker),
#ifdef USE_DSM_LOG
	SOC_SINGLE_EXT("DSM LOG", SND_SOC_NOPM, 0, 3, 0,
		max98504_get_dump_status, max98504_set_dump_status),
#endif

};

#ifdef MAX98504_USE_DAPM
static const char * const spk_src_mux_text[] =\
	{"PCM", "AIN", "PDM_CH0", "PDM_CH1"};

static const struct soc_enum spk_src_mux_enum =
	SOC_ENUM_SINGLE(MAX98504_REG_35_SPEAKER_SOURCE_SELECT,
		M98504_SPK_SRC_SEL_SHIFT,
		ARRAY_SIZE(spk_src_mux_text), spk_src_mux_text);
static const struct snd_kcontrol_new max98504_spk_src_mux =
	SOC_DAPM_ENUM("SPK_SRC Mux", spk_src_mux_enum);

static const char * const digital_mono_mux_text[] = {"CH0", "CH1", "CHMIX"};

static const struct soc_enum digital_mono_mux_enum =
	SOC_ENUM_SINGLE(MAX98504_REG_28_PCM_TO_SPEAKER_MONOMIX,
		M98504_PCM_TO_SPK_MONOMIX_CFG_SHIFT,
		ARRAY_SIZE(digital_mono_mux_text), digital_mono_mux_text);
static const struct snd_kcontrol_new max98504_digital_mono_mux =
	SOC_DAPM_ENUM("DAC_MONOMIX Mux", digital_mono_mux_enum);

static const struct snd_soc_dapm_widget max98504_dapm_widgets[] = {
	SND_SOC_DAPM_SUPPLY("SHDN", MAX98504_REG_40_GLOBAL_ENABLE,
		M98504_GLOBAL_EN_SHIFT, 0, NULL, 0),
	SND_SOC_DAPM_INPUT("Voltage Data"),
	SND_SOC_DAPM_INPUT("Current Data"),
	SND_SOC_DAPM_INPUT("Analog Input"),

	SND_SOC_DAPM_ADC("ADCL", NULL, MAX98504_REG_36_MEASUREMENT_ENABLES,
					M98504_MEAS_V_EN_SHIFT, 0),
	SND_SOC_DAPM_ADC("ADCR", NULL, MAX98504_REG_36_MEASUREMENT_ENABLES,
					M98504_MEAS_I_EN_SHIFT, 0),

	SND_SOC_DAPM_AIF_OUT("AIFOUTL", "HiFi Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIFOUTR", "HiFi Capture", 1, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_AIF_IN("AIFINL", "HiFi Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("AIFINR", "HiFi Playback", 1, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_DAC("DACL", NULL, MAX98504_REG_20_PCM_RX_ENABLES,
					M98504_PCM_RX_EN_CH0_SHIFT, 0),
	SND_SOC_DAPM_DAC("DACR", NULL, MAX98504_REG_20_PCM_RX_ENABLES,
					M98504_PCM_RX_EN_CH1_SHIFT, 0),

	SND_SOC_DAPM_MUX("DAC Mono Mux", SND_SOC_NOPM, 0, 0,
		&max98504_digital_mono_mux),

	SND_SOC_DAPM_MUX("SPK Source Mux", SND_SOC_NOPM, 0, 0,
		&max98504_spk_src_mux),

	SND_SOC_DAPM_PGA("SPK Mono Out", MAX98504_REG_34_SPEAKER_ENABLE,
		M98504_SPK_EN_SHIFT, 0, NULL, 0),

	SND_SOC_DAPM_OUTPUT("SPKOUT"),
};

static const struct snd_soc_dapm_route max98504_audio_map[] = {
	{"ADCL", NULL, "Voltage Data"},
	{"ADCR", NULL, "Current Data"},

	{"AIFOUTL", NULL, "ADCL"},
	{"AIFOUTR", NULL, "ADCR"},

	{"AIFOUTL", NULL, "SHDN"},
	{"AIFOUTR", NULL, "SHDN"},
	{"AIFINL", NULL, "SHDN"},
	{"AIFINR", NULL, "SHDN"},

	{"DAC Mono Mux", "CH0", "DACL"},
	{"DAC Mono Mux", "CH1", "DACR"},
	{"DAC Mono Mux", "CHMIX", "DACL"},
	{"DAC Mono Mux", "CHMIX", "DACR"},

	{"SPK Source Mux", "PCM", "DAC Mono Mux"},
	{"SPK Source Mux", "AIN", "Analog Input"},
	{"SPK Mono Out", NULL, "SPK Source Mux"},

	{"SPKOUT", NULL, "SPK Mono Out"},
};
#endif

static int max98504_add_widgets(struct snd_soc_codec *codec)
{
	msg_maxim("\n");

	snd_soc_add_codec_controls(codec, max98504_snd_controls,
		ARRAY_SIZE(max98504_snd_controls));

	return 0;
}

/* codec sample rate config parameter table */
static const struct {
	u32 rate;
	u8  sr;
} rate_table[] = {
	{8000,  (0)},
	{11025,	(1)},
	{12000, (2)},
	{16000, (3)},
	{22050, (4)},
	{24000, (5)},
	{32000, (6)},
	{44100, (7)},
	{48000, (8)},
};

static inline int rate_value(int rate, u8 *value)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rate_table); i++) {
		if (rate_table[i].rate >= rate) {
			*value = rate_table[i].sr;
			return 0;
		}
	}

	*value = rate_table[0].sr;

	return -EINVAL;
}

/* #define TDM */
static int max98504_set_tdm_slot(struct snd_soc_dai *codec_dai,
	unsigned int tx_mask, unsigned int rx_mask, int slots, int slot_width)
{
	return 0;
}

static int max98504_dai_set_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct max98504_priv *max98504 = snd_soc_codec_get_drvdata(codec);
	struct max98504_cdata *cdata;
	u8 regval;

	msg_maxim("\n");

	cdata = &max98504->dai[0];

	if (fmt != cdata->fmt) {
		cdata->fmt = fmt;

		regval = 0;

		switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
		case SND_SOC_DAIFMT_CBS_CFS:
		case SND_SOC_DAIFMT_CBM_CFM:
			break;
		case SND_SOC_DAIFMT_CBS_CFM:
		case SND_SOC_DAIFMT_CBM_CFS:
		default:
			dev_err(codec->dev,
				"DAI clock mode unsupported");
			return -EINVAL;
		}

		switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:
			snd_soc_update_bits(codec,
				MAX98504_REG_24_PCM_MODE_CONFIG,
				M98504_PCM_MODE_CFG_FORMAT_MASK,
				M98504_PCM_MODE_CFG_FORMAT_I2S_MASK);
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			snd_soc_update_bits(codec,
				MAX98504_REG_24_PCM_MODE_CONFIG,
				M98504_PCM_MODE_CFG_FORMAT_MASK,
				M98504_PCM_MODE_CFG_FORMAT_LJ_MASK);
			break;
		case SND_SOC_DAIFMT_RIGHT_J:
			snd_soc_update_bits(codec,
				MAX98504_REG_24_PCM_MODE_CONFIG,
				M98504_PCM_MODE_CFG_FORMAT_MASK,
				M98504_PCM_MODE_CFG_FORMAT_RJ_MASK);
			break;
		case SND_SOC_DAIFMT_DSP_A:
			/* Not supported mode */
		default:
			dev_err(codec->dev,
				"DAI format unsupported, fmt:0x%d"
				, fmt);
			return -EINVAL;
		}

		switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
		case SND_SOC_DAIFMT_NB_IF:
			break;
		case SND_SOC_DAIFMT_IB_NF:
		case SND_SOC_DAIFMT_IB_IF:
			break;
		default:
			dev_err(codec->dev,
				"DAI invert mode unsupported");
			return -EINVAL;
		}

		snd_soc_write(codec, MAX98504_REG_26_PCM_CLOCK_SETUP, 0);
	}

	return 0;
}

#ifdef MAX98504_USE_DAPM
static int max98504_set_bias_level(struct snd_soc_codec *codec,
				   enum snd_soc_bias_level level)
{
	int ret;

	msg_maxim("level=%d\n", level);

	switch (level) {
	case SND_SOC_BIAS_ON:
		if (codec->dapm.bias_level == SND_SOC_BIAS_OFF) {
			ret = snd_soc_cache_sync(codec);

			if (ret != 0) {
				dev_err(codec->dev,
					"Failed to sync cache: %d\n"
					, ret);
				return ret;
			}
		}
		snd_soc_update_bits(codec,
			MAX98504_REG_40_GLOBAL_ENABLE,
			M98504_GLOBAL_EN_MASK, M98504_GLOBAL_EN_MASK);
	break;

	case SND_SOC_BIAS_PREPARE:
		break;

	case SND_SOC_BIAS_STANDBY:
	case SND_SOC_BIAS_OFF:
		snd_soc_update_bits(codec,
			MAX98504_REG_40_GLOBAL_ENABLE,
			M98504_GLOBAL_EN_MASK, 0x00);
		codec->cache_sync = 1;
		break;
	}

	codec->dapm.bias_level = level;

	return 0;
}
#endif
static int max98504_dai_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct max98504_priv *max98504 = snd_soc_codec_get_drvdata(codec);
	struct max98504_cdata *cdata;

	unsigned int rate;
	u8 regval;

	msg_maxim("\n");

	cdata = &max98504->dai[0];

	rate = params_rate(params);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		snd_soc_update_bits(codec,
			MAX98504_REG_24_PCM_MODE_CONFIG,
			M98504_PCM_MODE_CFG_CH_SIZE_MASK,
			M98504_PCM_MODE_CFG_CH_SIZE_8_MASK);
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		snd_soc_update_bits(codec,
			MAX98504_REG_24_PCM_MODE_CONFIG,
			M98504_PCM_MODE_CFG_CH_SIZE_MASK,
			M98504_PCM_MODE_CFG_CH_SIZE_16_MASK);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		snd_soc_update_bits(codec,
			MAX98504_REG_24_PCM_MODE_CONFIG,
			M98504_PCM_MODE_CFG_CH_SIZE_MASK,
			M98504_PCM_MODE_CFG_CH_SIZE_24_MASK);
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		snd_soc_update_bits(codec,
			MAX98504_REG_24_PCM_MODE_CONFIG,
			M98504_PCM_MODE_CFG_CH_SIZE_MASK,
			M98504_PCM_MODE_CFG_CH_SIZE_32_MASK);
		break;
	default:
		return -EINVAL;
	}

	if (rate_value(rate, &regval))
		return -EINVAL;

	/* Update sample rate mode */
	snd_soc_update_bits(codec, MAX98504_REG_27_PCM_SAMPLE_RATE_SETUP,
		M98504_PCM_SR_SETUP_SPK_SR_MASK,
		regval<<M98504_PCM_SR_SETUP_SPK_SR_SHIFT);

	snd_soc_update_bits(codec, MAX98504_REG_27_PCM_SAMPLE_RATE_SETUP,
		M98504_PCM_SR_SETUP_MEAS_SR_MASK,
		regval<<M98504_PCM_SR_SETUP_MEAS_SR_SHIFT);

	return 0;
}

static int max98504_dai_set_sysclk(struct snd_soc_dai *dai,
				   int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct max98504_priv *max98504 = snd_soc_codec_get_drvdata(codec);

	msg_maxim("clk_id;%d, freq:%d, dir:%d\n", clk_id, freq, dir);

	/* Requested clock frequency is already setup */
	if (freq == max98504->sysclk)
		return 0;

	max98504->sysclk = freq;

	return 0;
}

#ifdef MAX98504_USE_DAPM
static int max98504_dai_digital_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	msg_maxim("- mute:%d\n", mute);

	if (mute) {
		snd_soc_update_bits(codec, MAX98504_REG_34_SPEAKER_ENABLE,
			M98504_SPK_EN_MASK, 0);
	} else {
		snd_soc_update_bits(codec, MAX98504_REG_34_SPEAKER_ENABLE,
			M98504_SPK_EN_MASK, M98504_SPK_EN_MASK);
	}
	return 0;
}
#endif

#define MAX98504_RATES SNDRV_PCM_RATE_8000_48000
#define MAX98504_FORMATS \
	(SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE\
	| SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops max98504_dai_ops = {
	.set_sysclk = max98504_dai_set_sysclk,
	.set_fmt = max98504_dai_set_fmt,
	.set_tdm_slot = max98504_set_tdm_slot,
	.hw_params = max98504_dai_hw_params,
#ifdef MAX98504_USE_DAPM
	.digital_mute = max98504_dai_digital_mute,
#endif
};

static struct snd_soc_dai_driver max98504_dai[] = {
	{
		.name = "max98504-aif1",
		.playback = {
			.stream_name = "HiFi Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98504_RATES,
			.formats = MAX98504_FORMATS,
		},
		.capture = {
			.stream_name = "HiFi Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98504_RATES,
			.formats = MAX98504_FORMATS,
		},
		 .ops = &max98504_dai_ops,
	}
};

static void max98504_handle_pdata(struct snd_soc_codec *codec)
{
	struct max98504_priv *max98504 = snd_soc_codec_get_drvdata(codec);
	struct max98504_pdata *pdata = max98504->pdata;
	struct max98504_cfg_data *cfg_data = &pdata->cfg_data;

	u8 regval;

	msg_maxim("\n");

	if (!pdata) {
		dev_dbg(codec->dev, "No platform data\n");
		return;
	}

/* Configure Rx Mode */
	if (pdata->rx_mode == MODE_RX_PCM)	{
		regval = 0;
		if (cfg_data->rx_dither_en)
			regval |= M98504_PCM_DSP_CFG_RX_DITH_EN_MASK;
		if (cfg_data->rx_flt_mode)
			regval |= M98504_PCM_DSP_CFG_RX_FLT_MODE_MASK;
		snd_soc_update_bits(codec, MAX98504_REG_25_PCM_DSP_CONFIG,
			M98504_PCM_DSP_CFG_RX_DITH_EN_MASK|\
			M98504_PCM_DSP_CFG_RX_FLT_MODE_MASK, regval);
		snd_soc_write(codec,  MAX98504_REG_20_PCM_RX_ENABLES,
			(u8)cfg_data->rx_ch_en);
	} else if (pdata->rx_mode == MODE_RX_PDM0 || \
	pdata->rx_mode == MODE_RX_PDM1)	{
		snd_soc_write(codec,  MAX98504_REG_33_PDM_RX_ENABLE,
			M98504_PDM_RX_EN_MASK);
	}
	snd_soc_write(codec,  MAX98504_REG_35_SPEAKER_SOURCE_SELECT,
		(u8) (M98504_SPK_SRC_SEL_MASK & pdata->rx_mode));

	/* Configure Tx Mode */
	if (pdata->tx_mode == MODE_TX_PCM)	{
		regval = 0;
		if (cfg_data->tx_dither_en)
			regval |= M98504_PCM_DSP_CFG_TX_DITH_EN_MASK;
		if (cfg_data->meas_dc_block_en)
			regval |= M98504_PCM_DSP_CFG_MEAS_DCBLK_EN_MASK;
		snd_soc_update_bits(codec, MAX98504_REG_25_PCM_DSP_CONFIG,
			M98504_PCM_DSP_CFG_TX_DITH_EN_MASK|\
			M98504_PCM_DSP_CFG_MEAS_DCBLK_EN_MASK, regval);

		snd_soc_write(codec,  MAX98504_REG_21_PCM_TX_ENABLES,
			(u8)cfg_data->tx_ch_en);
		snd_soc_write(codec,  MAX98504_REG_22_PCM_TX_HIZ_CONTROL,
			(u8)cfg_data->tx_hiz_ch_en);
		snd_soc_write(codec,  MAX98504_REG_23_PCM_TX_CHANNEL_SOURCES,
			(u8)cfg_data->tx_ch_src);
	} else {
		snd_soc_write(codec,  MAX98504_REG_30_PDM_TX_ENABLES,
			(u8)cfg_data->tx_ch_en);
		snd_soc_write(codec,  MAX98504_REG_31_PDM_TX_HIZ_CONTROL,
			(u8)cfg_data->tx_hiz_ch_en);
		snd_soc_write(codec,  MAX98504_REG_32_PDM_TX_CONTROL,
			(u8)cfg_data->tx_ch_src);
	}

#ifndef MAX98504_USE_DAPM
	snd_soc_write(codec,  MAX98504_REG_36_MEASUREMENT_ENABLES,
		M98504_MEAS_I_EN_MASK | M98504_MEAS_V_EN_MASK);
#endif
}

static int max98504_suspend(struct snd_soc_codec *codec)
{
	msg_maxim("\n");
	return 0;
}

static int max98504_resume(struct snd_soc_codec *codec)
{
	msg_maxim("\n");
	return 0;
}

static int max98504_probe(struct snd_soc_codec *codec)
{
	struct max98504_priv *max98504 = snd_soc_codec_get_drvdata(codec);
	struct max98504_cdata *cdata;
	int ret = 0;

	msg_maxim("\n");

	max98504->codec = codec;

	codec->cache_sync = 1;

	ret = snd_soc_codec_set_cache_io(codec, 16, 8, SND_SOC_I2C);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}

	/* reset the codec, the DSP core, and disable all interrupts */
	ret = max98504_reset(codec);
	if (ret < 0)
		goto err_access;

	/* initialize private data */

	max98504->sysclk = (unsigned)-1;

	cdata = &max98504->dai[0];
	cdata->rate = (unsigned)-1;
	cdata->fmt  = (unsigned)-1;

	ret = snd_soc_read(codec, MAX98504_REG_7FFF_REV_ID);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to read device revision: %d\n",
			ret);
		goto err_access;
	}
	msg_maxim("REV ID=0x%d\n", ret);

	snd_soc_write(codec, MAX98504_REG_16_PVDD_BROWNOUT_ENABLE, 0x1);
	snd_soc_write(codec, MAX98504_REG_17_PVDD_BROWNOUT_CONFIG_1, 0x3);
	snd_soc_write(codec, MAX98504_REG_18_PVDD_BROWNOUT_CONFIG_2, 0x64);
	snd_soc_write(codec, MAX98504_REG_19_PVDD_BROWNOUT_CONFIG_3, 0xff);
	snd_soc_write(codec, MAX98504_REG_1A_PVDD_BROWNOUT_CONFIG_4, 0xff);

	max98504_handle_pdata(codec);
	max98504_add_widgets(codec);

#ifdef USE_DSM_LOG
	if (class_name_log != NULL)
		max98504->dev_log_class =
		class_create(THIS_MODULE, class_name_log);
	if (max98504->dev_log_class == NULL) {
		pr_err("%s: class_create fail.\n", __func__);
	}	else	{
		max98504->dev_log = device_create(max98504->dev_log_class, NULL,
						 1, NULL, "max98504");
		if (IS_ERR(max98504->dev_log)) {
			ret = sysfs_create_group(&codec->dev->kobj,
				&max98504_attribute_group);
			if (ret)
				msg_maxim(\
				"failed to create sysfs group [%d]", ret);
		} else {
			ret = sysfs_create_group(&max98504->dev_log->kobj,
				&max98504_attribute_group);
			if (ret)
				msg_maxim("failed to create sysfs group [%d]",
					ret);
		}
	}
#endif

	msg_maxim("done.");

err_access:
	return ret;
}

static int max98504_remove(struct snd_soc_codec *codec)
{
	msg_maxim("\n");

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_max98504 = {
	.probe   = max98504_probe,
	.remove  = max98504_remove,
	.suspend = max98504_suspend,
	.resume  = max98504_resume,
	#ifdef MAX98504_USE_DAPM
	.set_bias_level = max98504_set_bias_level,
	#endif
	.reg_cache_size = ARRAY_SIZE(max98504_reg_def),
	.reg_word_size = sizeof(u8),
	.reg_cache_default = max98504_reg_def,
	.readable_register = max98504_readable,
	.volatile_register = max98504_volatile_register,
	#ifdef MAX98504_USE_DAPM
	.dapm_widgets	  = max98504_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(max98504_dapm_widgets),
	.dapm_routes     = max98504_audio_map,
	.num_dapm_routes = ARRAY_SIZE(max98504_audio_map),
	#endif
};

static int reg_set_optimum_mode_check(struct regulator *reg, int load_uA)
{
	return (regulator_count_voltages(reg) > 0) ?
			regulator_set_optimum_mode(reg, load_uA) : 0;
}

static int max98504_regulator_config(struct i2c_client *i2c,
	bool pullup, bool on)
{
	struct regulator *max98504_vcc_i2c;
	int rc;
	#define VCC_I2C_MIN_UV	1800000
	#define VCC_I2C_MAX_UV	1800000
	#define I2C_LOAD_UA		300000

	msg_maxim("pullup=%d\n", pullup);

	if (pullup) {
		max98504_vcc_i2c = regulator_get(&i2c->dev, "vcc_i2c");

		if (IS_ERR(max98504_vcc_i2c)) {
			rc = PTR_ERR(max98504_vcc_i2c);
			pr_err("Regulator get failed rc=%d\n",	rc);
			return rc;
		}

		if (regulator_count_voltages(max98504_vcc_i2c) > 0) {
			rc = regulator_set_voltage(max98504_vcc_i2c,
				VCC_I2C_MIN_UV, VCC_I2C_MAX_UV);
			if (rc) {
				pr_err("regulator set_vtg failed rc=%d\n", rc);
				goto error_set_vtg_i2c;
			}
		}

		rc = reg_set_optimum_mode_check(max98504_vcc_i2c, I2C_LOAD_UA);
		if (rc < 0) {
			pr_err("Regulator vcc_i2c set_opt failed rc=%d\n", rc);
			goto error_reg_opt_i2c;
		}

		rc = regulator_enable(max98504_vcc_i2c);
		if (rc) {
			pr_err("Regulator vcc_i2c enable failed rc=%d\n", rc);
			goto error_reg_en_vcc_i2c;
		}
	}

	return 0;

error_reg_en_vcc_i2c:
	if (pullup)
		reg_set_optimum_mode_check(max98504_vcc_i2c, 0);
error_reg_opt_i2c:
	regulator_disable(max98504_vcc_i2c);
error_set_vtg_i2c:
	regulator_put(max98504_vcc_i2c);

	return rc;
}

static int max98504_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct max98504_priv *max98504;
	struct max98504_pdata *pdata;

	int ret;

	msg_maxim("\n");

	max98504_regulator_config(i2c, of_property_read_bool(i2c->dev.of_node,
		"max98504,i2c-pull-up"), 1);

	max98504 = kzalloc(sizeof(struct max98504_priv), GFP_KERNEL);
	if (max98504 == NULL)
		return -ENOMEM;

	max98504->devtype = id->driver_data;
	i2c_set_clientdata(i2c, max98504);
	max98504->control_data = i2c;

	max98504->pdata = devm_kzalloc(&i2c->dev,
		sizeof(struct max98504_pdata), GFP_KERNEL);
	if (!max98504->pdata) {
		dev_err(&i2c->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	} else
	pdata = max98504->pdata;

	if (i2c->dev.of_node) {
#ifdef USE_MAX98504_IRQ
		pdata->irq = of_get_named_gpio_flags(i2c->dev.of_node,
			"max98504,irq-gpio",
					0, NULL);
#endif
		ret = of_property_read_u32(i2c->dev.of_node, "max98504,rx_mode",
			&pdata->rx_mode);
		if (ret) {
			dev_err(&i2c->dev, "Failed to read rx_mode.\n");
			return -EINVAL;
		}

		ret = of_property_read_u32(i2c->dev.of_node, "max98504,tx_mode",
			&pdata->tx_mode);
		if (ret) {
			dev_err(&i2c->dev, "Failed to read tx_mode.\n");
			return -EINVAL;
		}

		ret = of_property_read_u32_array(i2c->dev.of_node,
			"max98504,cfg_data",
			(u32 *)&pdata->cfg_data,
			sizeof(struct max98504_cfg_data)/sizeof(u32));
		if (ret) {
			dev_err(&i2c->dev, "Failed to read cfg_data.\n");
			return -EINVAL;
		}
		#ifdef USE_DSM_LOG
		class_name_log = NULL;
		ret = of_property_read_string(i2c->dev.of_node,
			"max98504,log_class", &class_name_log);
		if (ret) {
			dev_err(&i2c->dev, "Failed to read log_class.\n");
			class_name_log = DEFAULT_LOG_CLASS_NAME;
		}
		#endif

		msg_maxim("rx_mode:%d, tx_mode:%d, tx_dither_en:%d, "\
			"rx_dither_en:%d, meas_dc_block_en:%d, "\
			"rx_flt_mode:%d, rx_ch_en:%d\n",
			pdata->rx_mode,
			pdata->tx_mode,
			pdata->cfg_data.tx_dither_en,
			pdata->cfg_data.rx_dither_en,
			pdata->cfg_data.meas_dc_block_en,
			pdata->cfg_data.rx_flt_mode,
			pdata->cfg_data.rx_ch_en);
		msg_maxim("tx_ch_en:%d, tx_hiz_ch_en:%d, tx_ch_src:%d, "\
			"auth_en:%d, wdog_time_out:%d\n",
			pdata->cfg_data.tx_ch_en,
			pdata->cfg_data.tx_hiz_ch_en,
			pdata->cfg_data.tx_ch_src,
			pdata->cfg_data.auth_en,
			pdata->cfg_data.wdog_time_out);
	}	else
	max98504->pdata = i2c->dev.platform_data;

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_max98504,
		max98504_dai, ARRAY_SIZE(max98504_dai));
	if (ret < 0)
		kfree(max98504);

	msg_maxim("ret=%d\n", ret);

#ifdef USE_MAX98504_IRQ
	if (gpio_is_valid(pdata->irq)) {
		/* configure touchscreen irq gpio */
		ret = gpio_request(pdata->irq, "max98504_irq_gpio");
		if (ret) {
			dev_err(&i2c->dev, "unable to request gpio [%d]\n",
						pdata->irq);
			goto err_irq_gpio_req;
		}
		ret = gpio_direction_input(pdata->irq);
		if (ret) {
			dev_err(&i2c->dev,
				"unable to set direction for gpio [%d]\n",
				pdata->irq);
			goto err_irq_gpio_req;
		}
		i2c->irq = gpio_to_irq(pdata->irq);
	} else {
		dev_err(&i2c->dev, "irq gpio not provided\n");
	}

	ret = request_threaded_irq(i2c->irq, NULL, max98504_interrupt,
			IRQF_TRIGGER_FALLING, "max98504_interrupt", max98504);
	if (ret)
		dev_err(&i2c->dev, "Failed to register interrupt\n");

err_irq_gpio_req:
	if (gpio_is_valid(pdata->irq))
		gpio_free(pdata->irq);
#endif

	return ret;
}

static int __devexit max98504_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));

	msg_maxim("\n");

	return 0;
}

static const struct i2c_device_id max98504_i2c_id[] = {
	{ "max98504", MAX98504 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max98504_i2c_id);

static struct i2c_driver max98504_i2c_driver = {
	.driver = {
		.name = "max98504",
		.owner = THIS_MODULE,
	},
	.probe  = max98504_i2c_probe,
	.remove = __devexit_p(max98504_i2c_remove),
	.id_table = max98504_i2c_id,
};

static int __init max98504_init(void)
{
	int ret;

	msg_maxim("%s\n", __func__);

	ret = i2c_add_driver(&max98504_i2c_driver);

	if (ret)
		pr_err("Failed to register MAX98504 I2C driver: %d\n", ret);
	else
		pr_info("MAX98504 driver built on %s at %s\n",
			__DATE__, __TIME__);

#ifdef CONFIG_SND_SOC_MAXIM_DSM
	maxdsm_init();
#endif

	return ret;
}
module_init(max98504_init);

static void __exit max98504_exit(void)
{
	i2c_del_driver(&max98504_i2c_driver);
#ifdef CONFIG_SND_SOC_MAXIM_DSM
	maxdsm_deinit();
#endif
}
module_exit(max98504_exit);

MODULE_DESCRIPTION("ALSA SoC MAX98504 driver");
MODULE_AUTHOR("Ryan Lee");
MODULE_LICENSE("GPL");