/*
 * max98505.c -- ALSA SoC MAX98505 driver
 *
 * Copyright 2013-2014 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/max98505.h>
#include "max98505.h"
#include "maxim_dsm.h"
#define SUPPORT_DEVICE_TREE

#ifdef SUPPORT_DEVICE_TREE
#include <linux/regulator/consumer.h>
#endif

static struct reg_default max98505_reg[] = {
	{ 0x00, 0x00 }, /* Battery Voltage Data */
	{ 0x01, 0x00 }, /* Boost Voltage Data */
	{ 0x02, 0x00 }, /* Live Status0 */
	{ 0x03, 0x00 }, /* Live Status1 */
	{ 0x04, 0x00 }, /* Live Status2 */
	{ 0x05, 0x00 }, /* State0 */
	{ 0x06, 0x00 }, /* State1 */
	{ 0x07, 0x00 }, /* State2 */
	{ 0x08, 0x00 }, /* Flag0 */
	{ 0x09, 0x00 }, /* Flag1 */
	{ 0x0A, 0x00 }, /* Flag2 */
	{ 0x0B, 0x00 }, /* IRQ Enable0 */
	{ 0x0C, 0x00 }, /* IRQ Enable1 */
	{ 0x0D, 0x00 }, /* IRQ Enable2 */
	{ 0x0E, 0x00 }, /* IRQ Clear0 */
	{ 0x0F, 0x00 }, /* IRQ Clear1 */
	{ 0x10, 0x00 }, /* IRQ Clear2 */
	{ 0x11, 0xC0 }, /* Map0 */
	{ 0x12, 0x00 }, /* Map1 */
	{ 0x13, 0x00 }, /* Map2 */
	{ 0x14, 0xF0 }, /* Map3 */
	{ 0x15, 0x00 }, /* Map4 */
	{ 0x16, 0xAB }, /* Map5 */
	{ 0x17, 0x89 }, /* Map6 */
	{ 0x18, 0x00 }, /* Map7 */
	{ 0x19, 0x00 }, /* Map8 */
	{ 0x1A, 0x06 }, /* DAI Clock Mode 1 */
	{ 0x1B, 0xC0 }, /* DAI Clock Mode 2 */
	{ 0x1C, 0x00 }, /* DAI Clock Divider Denominator MSBs */
	{ 0x1D, 0x00 }, /* DAI Clock Divider Denominator LSBs */
	{ 0x1E, 0xF0 }, /* DAI Clock Divider Numerator MSBs */
	{ 0x1F, 0x00 }, /* DAI Clock Divider Numerator LSBs */
	{ 0x20, 0x50 }, /* Format */
	{ 0x21, 0x00 }, /* TDM Slot Select */
	{ 0x22, 0x00 }, /* DOUT Configuration VMON */
	{ 0x23, 0x00 }, /* DOUT Configuration IMON */
	{ 0x24, 0x00 }, /* DOUT Configuration VBAT */
	{ 0x25, 0x00 }, /* DOUT Configuration VBST */
	{ 0x26, 0x00 }, /* DOUT Configuration FLAG */
	{ 0x27, 0xFF }, /* DOUT HiZ Configuration 1 */
	{ 0x28, 0xFF }, /* DOUT HiZ Configuration 2 */
	{ 0x29, 0xFF }, /* DOUT HiZ Configuration 3 */
	{ 0x2A, 0xFF }, /* DOUT HiZ Configuration 4 */
	{ 0x2B, 0x02 }, /* DOUT Drive Strength */
	{ 0x2C, 0x90 }, /* Filters */
	{ 0x2D, 0x00 }, /* Gain */
	{ 0x2E, 0x02 }, /* Gain Ramping */
	{ 0x2F, 0x00 }, /* Speaker Amplifier */
	{ 0x30, 0x0A }, /* Threshold */
	{ 0x31, 0x00 }, /* ALC Attack */
	{ 0x32, 0x80 }, /* ALC Atten and Release */
	{ 0x33, 0x00 }, /* ALC Infinite Hold Release */
	{ 0x34, 0x92 }, /* ALC Configuration */
	{ 0x35, 0x01 }, /* Boost Converter */
	{ 0x36, 0x00 }, /* Block Enable */
	{ 0x37, 0x00 }, /* Configuration */
	{ 0x38, 0x00 }, /* Global Enable */
	{ 0x3A, 0x00 }, /* Boost Limiter */
	{ 0xFF, 0x50 }, /* Revision ID */
};

static bool max98505_volatile_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX98505_R000_VBAT_DATA:
	case MAX98505_R001_VBST_DATA:
	case MAX98505_R002_LIVE_STATUS0:
	case MAX98505_R003_LIVE_STATUS1:
	case MAX98505_R004_LIVE_STATUS2:
	case MAX98505_R005_STATE0:
	case MAX98505_R006_STATE1:
	case MAX98505_R007_STATE2:
	case MAX98505_R008_FLAG0:
	case MAX98505_R009_FLAG1:
	case MAX98505_R00A_FLAG2:
	case MAX98505_R0FF_VERSION:
		return true;
	default:
		return false;
	}
}

static bool max98505_readable_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX98505_R00E_IRQ_CLEAR0:
	case MAX98505_R00F_IRQ_CLEAR1:
	case MAX98505_R010_IRQ_CLEAR2:
	case MAX98505_R033_ALC_HOLD_RLS:
		return false;
	default:
		return true;
	}
};

#ifdef SUPPORT_DEVICE_TREE
static int reg_set_optimum_mode_check(struct regulator *reg, int load_uA)
{
	return (regulator_count_voltages(reg) > 0) ?
		regulator_set_optimum_mode(reg, load_uA) : 0;
}

static int max98505_regulator_config(struct i2c_client *i2c, bool pullup, bool on)
{
	struct regulator *max98505_vcc_i2c;
	int rc;
    #define VCC_I2C_MIN_UV	1800000
    #define VCC_I2C_MAX_UV	1800000
	#define I2C_LOAD_UA		300000

	pr_info("%s: enter\n", __func__);

	if (pullup) {
		pr_info("%s: I2C PULL UP.\n", __func__);

		max98505_vcc_i2c = regulator_get(&i2c->dev, "vcc_i2c");
		if (IS_ERR(max98505_vcc_i2c)) {
			rc = PTR_ERR(max98505_vcc_i2c);
			pr_info("%s: regulator get failed rc=%d\n", __func__, rc);
			goto error_get_vtg_i2c;
		}
		if (regulator_count_voltages(max98505_vcc_i2c) > 0) {
			rc = regulator_set_voltage(max98505_vcc_i2c,
				VCC_I2C_MIN_UV, VCC_I2C_MAX_UV);
			if (rc) {
				pr_info("%s: regulator set_vtg failed rc=%d\n", __func__, rc);
				goto error_set_vtg_i2c;
			}
		}

		rc = reg_set_optimum_mode_check(max98505_vcc_i2c, I2C_LOAD_UA);
		if (rc < 0) {
			pr_info("%s: regulator vcc_i2c set_opt failed rc=%d\n", __func__, rc);
			goto error_reg_opt_i2c;
		}

		rc = regulator_enable(max98505_vcc_i2c);
		if (rc) {
			pr_info("%s: regulator vcc_i2c enable failed rc=%d\n", __func__, rc);
			goto error_reg_en_vcc_i2c;
		}

	}

	return 0;

	error_set_vtg_i2c:
		regulator_put(max98505_vcc_i2c);
	error_get_vtg_i2c:
	if (regulator_count_voltages(max98505_vcc_i2c) > 0)
		regulator_set_voltage(max98505_vcc_i2c, 0,
			VCC_I2C_MAX_UV);
	error_reg_en_vcc_i2c:
			if(pullup) 	reg_set_optimum_mode_check(max98505_vcc_i2c, 0);
	error_reg_opt_i2c:
			regulator_disable(max98505_vcc_i2c);

	return rc;
}
#endif

#ifdef USE_REG_DUMP
static void reg_dump(struct max98505_priv *max98505)
{
	int val_l;
	int i, j;

	static const struct {
		int start;
		int count;
	} reg_table[] = {
		{ 0x02, 0x03 },
		{ 0x1A, 0x1F },
		{ 0x3A, 0x01 },
		{ 0x00, 0x00 }
	};

	i = 0;
	while (reg_table[i].count != 0) {
		for(j = 0; j < reg_table[i].count; j++) {
			int addr = j + reg_table[i].start;
			regmap_read(max98505->regmap, addr, &val_l);
			pr_info("%s: reg 0x%02X, val_l 0x%02X\n",
					__func__, addr, val_l);
		}
		i++;
	}
}
#endif

static const unsigned int max98505_spk_tlv[] = {
	TLV_DB_RANGE_HEAD(1),
	1, 31, TLV_DB_SCALE_ITEM(-600, 100, 0),
};

static int max98505_spk_vol_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98505_priv *max98505 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = max98505->volume;

	return 0;
}

static int max98505_spk_vol_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98505_priv *max98505 = snd_soc_codec_get_drvdata(codec);
	unsigned int sel = ucontrol->value.integer.value[0];

	regmap_update_bits(max98505->regmap, MAX98505_R02D_GAIN,
			M98505_SPK_GAIN_MASK, sel << M98505_SPK_GAIN_SHIFT);

	max98505->volume = sel;

	return 0;
}

static int max98505_reg_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol, unsigned int reg,
		unsigned int mask, unsigned int shift)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98505_priv *max98505 = snd_soc_codec_get_drvdata(codec);
	int data;

	regmap_read(max98505->regmap, reg, &data);

	ucontrol->value.integer.value[0] =
		(data & mask) >> shift;

	return 0;
}

static int max98505_reg_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol, unsigned int reg,
		unsigned int mask, unsigned int shift)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct max98505_priv *max98505 = snd_soc_codec_get_drvdata(codec);
	unsigned int sel = ucontrol->value.integer.value[0];

	regmap_update_bits(max98505->regmap, reg, mask, sel << shift);

	return 0;
}

static int max98505_spk_ramp_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98505_reg_get(kcontrol, ucontrol, MAX98505_R02E_GAIN_RAMPING,
			M98505_SPK_RMP_EN_MASK, M98505_SPK_RMP_EN_SHIFT);
}

static int max98505_spk_ramp_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98505_reg_put(kcontrol, ucontrol, MAX98505_R02E_GAIN_RAMPING,
			M98505_SPK_RMP_EN_MASK, M98505_SPK_RMP_EN_SHIFT);
}

static int max98505_spk_zcd_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98505_reg_get(kcontrol, ucontrol, MAX98505_R02E_GAIN_RAMPING,
			M98505_SPK_ZCD_EN_MASK, M98505_SPK_ZCD_EN_SHIFT);
}

static int max98505_spk_zcd_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98505_reg_put(kcontrol, ucontrol, MAX98505_R02E_GAIN_RAMPING,
			M98505_SPK_ZCD_EN_MASK, M98505_SPK_ZCD_EN_SHIFT);
}

static int max98505_alc_en_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98505_reg_get(kcontrol, ucontrol, MAX98505_R030_THRESHOLD,
			M98505_ALC_EN_MASK, M98505_ALC_EN_SHIFT);
}

static int max98505_alc_en_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98505_reg_put(kcontrol, ucontrol, MAX98505_R030_THRESHOLD,
			M98505_ALC_EN_MASK, M98505_ALC_EN_SHIFT);
}

static int max98505_alc_threshold_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98505_reg_get(kcontrol, ucontrol, MAX98505_R030_THRESHOLD,
			M98505_ALC_TH_MASK, M98505_ALC_TH_SHIFT);
}

static int max98505_alc_threshold_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	return max98505_reg_put(kcontrol, ucontrol, MAX98505_R030_THRESHOLD,
			M98505_ALC_TH_MASK, M98505_ALC_TH_SHIFT);
}

static const char * max98505_boost_voltage_text[] = {"8.5V", "8.25V",
		"8.0V", "7.75V","7.5V","7.25V","7.0V","6.75V", "6.5V", "6.5V",
		"6.5V","6.5V","6.5V","6.5V","6.5V","6.5V"};

static const struct soc_enum max98505_boost_voltage_enum =
	SOC_ENUM_SINGLE(MAX98505_R037_CONFIGURATION, M98505_BST_VOUT_SHIFT, 15,
			max98505_boost_voltage_text);

static const struct snd_kcontrol_new max98505_snd_controls[] = {

	SOC_SINGLE_EXT_TLV("Speaker Volume", MAX98505_R02D_GAIN,
		M98505_SPK_GAIN_SHIFT, (1<<M98505_SPK_GAIN_WIDTH)-1, 0,
		max98505_spk_vol_get, max98505_spk_vol_put, max98505_spk_tlv),

	SOC_SINGLE_EXT("Speaker Ramp", 0, 0, 1, 0,
		max98505_spk_ramp_get, max98505_spk_ramp_put),

	SOC_SINGLE_EXT("Speaker ZCD", 0, 0, 1, 0,
		max98505_spk_zcd_get, max98505_spk_zcd_put),

	SOC_SINGLE_EXT("ALC Enable", 0, 0, 1, 0,
		max98505_alc_en_get, max98505_alc_en_put),

	SOC_SINGLE_EXT("ALC Threshold", 0, 0, (1<<M98505_ALC_TH_WIDTH)-1, 0,
		max98505_alc_threshold_get, max98505_alc_threshold_put),

	SOC_ENUM("Boost Output Voltage", max98505_boost_voltage_enum),
};

static int max98505_add_widgets(struct snd_soc_codec *codec)
{
	int ret;

	ret = snd_soc_add_codec_controls(codec, max98505_snd_controls,
		ARRAY_SIZE(max98505_snd_controls));

	return 0;
}

/* codec sample rate and n/m dividers parameter table */
static const struct { 
	u32 rate;
	u8  sr;
	u32 divisors[3][2];
} rate_table[] = {
	{ 8000, 0, {{  1,   375}, {5, 1764}, {  1,   384}}},
	{11025,	1, {{147, 40000}, {1,  256}, {147, 40960}}},
	{12000, 2, {{  1,   250}, {5, 1176}, {  1,   256}}},
	{16000, 3, {{  2,   375}, {5,  882}, {  1,   192}}},
	{22050, 4, {{147, 20000}, {1,  128}, {147, 20480}}},
	{24000, 5, {{  1,   125}, {5,  588}, {  1,   128}}},
    {32000, 6, {{  4,   375}, {5,  441}, {  1,    96}}},
	{44100, 7, {{147, 10000}, {1,   64}, {147, 10240}}},
	{48000, 8, {{  2,   125}, {5,  294}, {  1,    64}}},
};

static inline int max98505_rate_value(int rate, int clock, u8 *value, int *n, int *m)
{
	int ret = -EINVAL;
	int i;

	for (i = 0; i < ARRAY_SIZE(rate_table); i++) {
		if (rate_table[i].rate >= rate) {
			*value = rate_table[i].sr;
			*n = rate_table[i].divisors[clock][0];
			*m = rate_table[i].divisors[clock][1];
			ret = 0;
			break;
		}
	}

	pr_info("%s: sample rate is %d, returning %d\n", __func__, rate_table[i].rate, *value);

	return ret;
}

static int max98505_set_tdm_slot(struct snd_soc_dai *codec_dai,
		unsigned int tx_mask, unsigned int rx_mask, int slots, int slot_width)
{
	pr_info("%s: tx_mask 0x%X, rx_mask 0x%X, slots %d, slot width %d\n",
			__func__, tx_mask, rx_mask, slots, slot_width);
	return 0;
}

static void max98505_set_slave(struct max98505_priv *max98505)
{
	pr_info("%s: ENTER\n", __func__);

	/*
	 * 1. use BCLK instead of MCLK
	 */
	regmap_update_bits(max98505->regmap, MAX98505_R01A_DAI_CLK_MODE1,
			M98505_DAI_CLK_SOURCE_MASK, M98505_DAI_CLK_SOURCE_MASK);
	/*
	 * 2. set DAI to slave mode
	 */
	regmap_update_bits(max98505->regmap, MAX98505_R01B_DAI_CLK_MODE2,
			M98505_DAI_MAS_MASK, 0);
	/*
	 * 3. set BLCKs to LRCLKs to 64
	 */
	regmap_update_bits(max98505->regmap, MAX98505_R01B_DAI_CLK_MODE2,
			M98505_DAI_BSEL_MASK, M98505_DAI_BSEL_32);
	/*
	 * 4. set VMON slots
	 */
	regmap_update_bits(max98505->regmap, MAX98505_R022_DOUT_CFG_VMON,
			M98505_DAI_VMON_EN_MASK, M98505_DAI_VMON_EN_MASK);
	regmap_update_bits(max98505->regmap, MAX98505_R022_DOUT_CFG_VMON,
			M98505_DAI_VMON_SLOT_MASK, M98505_DAI_VMON_SLOT_00_01);
	/*
	 * 5. set IMON slots
	 */
	regmap_update_bits(max98505->regmap, MAX98505_R023_DOUT_CFG_IMON,
			M98505_DAI_IMON_EN_MASK, M98505_DAI_IMON_EN_MASK);
	regmap_update_bits(max98505->regmap, MAX98505_R023_DOUT_CFG_IMON,
			M98505_DAI_IMON_SLOT_MASK, M98505_DAI_IMON_SLOT_02_03);
}

static void max98505_set_master(struct max98505_priv *max98505)
{
	pr_info("%s: ENTER\n", __func__);

	/*
	 * 1. use MCLK for Left channel, right channel always BCLK
	 */
	regmap_update_bits(max98505->regmap, MAX98505_R01A_DAI_CLK_MODE1,
			M98505_DAI_CLK_SOURCE_MASK, 0);
	/*
	 * 2. set left channel DAI to master mode, right channel always slave
	 */
	regmap_update_bits(max98505->regmap, MAX98505_R01B_DAI_CLK_MODE2,
			M98505_DAI_MAS_MASK, M98505_DAI_MAS_MASK);
}

static int max98505_dai_set_fmt(struct snd_soc_dai *codec_dai,
				 unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct max98505_priv *max98505 = snd_soc_codec_get_drvdata(codec);
	struct max98505_cdata *cdata;
	unsigned int invert = 0;

	pr_info("%s: fmt 0x%08X\n", __func__, fmt);

	cdata = &max98505->dai[0];

	cdata->fmt = fmt;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		max98505_set_slave(max98505);
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		max98505_set_master(max98505);
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
	case SND_SOC_DAIFMT_CBM_CFS:
	default:
		dev_err(codec->dev, "DAI clock mode unsupported");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		pr_info("%s: set SND_SOC_DAIFMT_I2S\n", __func__);
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		pr_info("%s: set SND_SOC_DAIFMT_LEFT_J\n", __func__);
		break;
	case SND_SOC_DAIFMT_DSP_A:
		pr_info("%s: set SND_SOC_DAIFMT_DSP_A\n", __func__);
	default:
		dev_err(codec->dev, "DAI format unsupported, fmt:0x%x skip", fmt);
		//return -EINVAL;   //temp block
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		invert = M98505_DAI_WCI_MASK;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		invert = M98505_DAI_BCI_MASK;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		invert = M98505_DAI_BCI_MASK | M98505_DAI_WCI_MASK;
		break;
	default:
		dev_err(codec->dev, "DAI invert mode unsupported skip");
		//return -EINVAL;
	}

	regmap_update_bits(max98505->regmap, MAX98505_R020_FORMAT,
			M98505_DAI_BCI_MASK | M98505_DAI_BCI_MASK, invert);

	return 0;
}

static int max98505_set_bias_level(struct snd_soc_codec *codec,
				   enum snd_soc_bias_level level)
{
	codec->dapm.bias_level = level;
	return 0;
}

static int max98505_set_clock(struct max98505_priv *max98505, unsigned int rate)
{
	unsigned int clock;
	unsigned int mdll;
	unsigned int n;
	unsigned int m;
	u8 dai_sr;

	switch (max98505->sysclk) {
	case 6000000:
		clock = 0;
		mdll  = M98505_MDLL_MULT_MCLKx16;
		break;
	case 11289600:
		clock = 1;
		mdll  = M98505_MDLL_MULT_MCLKx8;
		break;
	case 12000000:
		clock = 0;
		mdll  = M98505_MDLL_MULT_MCLKx8;
		break;
	case 12288000:
		clock = 2;
		mdll  = M98505_MDLL_MULT_MCLKx8;
		break;
	default:
		dev_info(max98505->codec->dev, "unsupported sysclk %d\n",
					max98505->sysclk);
		return -EINVAL;
	}

	if (max98505_rate_value(rate, clock, &dai_sr, &n, &m))
		return -EINVAL;

	/*
	 * 1. set DAI_SR to correct LRCLK frequency
	 */
	regmap_update_bits(max98505->regmap, MAX98505_R01B_DAI_CLK_MODE2,
			M98505_DAI_SR_MASK, dai_sr << M98505_DAI_SR_SHIFT);
	/*
	 * 2. set DAI m divider
	 */
	regmap_write(max98505->regmap, MAX98505_R01C_DAI_CLK_DIV_M_MSBS,
			m >> 8);
	regmap_write(max98505->regmap, MAX98505_R01D_DAI_CLK_DIV_M_LSBS,
			m & 0xFF);
	/*
	 * 3. set DAI n divider
	 */
	regmap_write(max98505->regmap, MAX98505_R01E_DAI_CLK_DIV_N_MSBS,
			n >> 8);
	regmap_write(max98505->regmap, MAX98505_R01F_DAI_CLK_DIV_N_LSBS,
			n & 0xFF);
	/*
	 * 4. set MDLL
	 */
	regmap_update_bits(max98505->regmap, MAX98505_R01A_DAI_CLK_MODE1,
			M98505_MDLL_MULT_MASK, mdll << M98505_MDLL_MULT_SHIFT);

	return 0;
}

static int max98505_dai_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct max98505_priv *max98505 = snd_soc_codec_get_drvdata(codec);
	struct max98505_cdata *cdata;
	unsigned int rate;

	pr_info("%s: enter\n", __func__);

	cdata = &max98505->dai[0];

	rate = params_rate(params);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		pr_info("%s: set SNDRV_PCM_FORMAT_S16_LE\n", __func__);
		regmap_update_bits(max98505->regmap, MAX98505_R020_FORMAT,
				M98505_DAI_CHANSZ_MASK, M98505_DAI_CHANSZ_16);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		pr_info("%s: set SNDRV_PCM_FORMAT_S24_LE\n", __func__);
#ifdef RIVER
		regmap_update_bits(max98505->regmap, MAX98505_R020_FORMAT,
				M98505_DAI_CHANSZ_MASK, M98505_DAI_CHANSZ_32);
		pr_info("%s: (really set to 32 bits)\n", __func__);
#else
		regmap_update_bits(max98505->regmap, MAX98505_R020_FORMAT,
				M98505_DAI_CHANSZ_MASK, M98505_DAI_CHANSZ_24);
#endif
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		pr_info("%s: set SNDRV_PCM_FORMAT_S32_LE\n", __func__);
		regmap_update_bits(max98505->regmap, MAX98505_R020_FORMAT,
				M98505_DAI_CHANSZ_MASK, M98505_DAI_CHANSZ_32);
		break;
	default:
		pr_info("%s: format unsupported %d but skip", __func__, params_format(params));
		regmap_update_bits(max98505->regmap, MAX98505_R020_FORMAT,
				M98505_DAI_CHANSZ_MASK, M98505_DAI_CHANSZ_16);
	}

	return max98505_set_clock(max98505, rate);
}

static int max98505_dai_set_sysclk(struct snd_soc_dai *dai,
				   int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct max98505_priv *max98505 = snd_soc_codec_get_drvdata(codec);

	pr_info("%s: clk_id %d, freq %d, dir %d\n", __func__, clk_id, freq, dir);

	max98505->sysclk = freq;

	return 0;
}

static int max98505_dai_digital_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct max98505_priv *max98505 = snd_soc_codec_get_drvdata(codec_dai->codec);

	pr_info("%s: mute %d\n", __func__, mute);

	if (mute) {
		regmap_update_bits(max98505->regmap, MAX98505_R02D_GAIN,
			M98505_SPK_GAIN_MASK, 0x00);

		usleep_range(5000, 5000);

		regmap_update_bits(max98505->regmap, MAX98505_R038_GLOBAL_ENABLE,
			M98505_EN_MASK, 0x0);
	}
	else	{
		regmap_update_bits(max98505->regmap, MAX98505_R02D_GAIN,
			M98505_SPK_GAIN_MASK, max98505->volume);

		regmap_update_bits(max98505->regmap, MAX98505_R036_BLOCK_ENABLE,
			M98505_BST_EN_MASK | M98505_SPK_EN_MASK |
				M98505_ADC_IMON_EN_MASK | M98505_ADC_VMON_EN_MASK,
			M98505_BST_EN_MASK | M98505_SPK_EN_MASK |
				M98505_ADC_IMON_EN_MASK | M98505_ADC_VMON_EN_MASK);
		regmap_write(max98505->regmap, MAX98505_R038_GLOBAL_ENABLE,
			M98505_EN_MASK);
	}


#ifdef USE_REG_DUMP
	reg_dump(max98505);
#endif

	return 0;
}

#define MAX98505_RATES SNDRV_PCM_RATE_8000_48000
#define MAX98505_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops max98505_dai_ops = {
	.set_sysclk = max98505_dai_set_sysclk,
	.set_fmt = max98505_dai_set_fmt,
	.set_tdm_slot = max98505_set_tdm_slot,
	.hw_params = max98505_dai_hw_params,
	.digital_mute = max98505_dai_digital_mute,
};

static struct snd_soc_dai_driver max98505_dai[] = {
	{
		.name = "max98505-aif1",
		.playback = {
			.stream_name = "HiFi Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_48000,
			.formats = MAX98505_FORMATS,
		},
		.capture = {
			.stream_name = "HiFi Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_48000,
			.formats = MAX98505_FORMATS,
		},
		.ops = &max98505_dai_ops,
	}
};

static void max98505_handle_pdata(struct snd_soc_codec *codec)
{
	struct max98505_priv *max98505 = snd_soc_codec_get_drvdata(codec);
	struct max98505_pdata *pdata = max98505->pdata;

	if (!pdata) {
		dev_dbg(codec->dev, "No platform data\n");
		return;
	}
}

#ifdef CONFIG_PM
static int max98505_suspend(struct snd_soc_codec *codec)
{
	pr_info("%s: enter\n", __func__);

	return 0;
}

static int max98505_resume(struct snd_soc_codec *codec)
{
	pr_info("%s: enter\n", __func__);

	return 0;
}
#else
#define max98505_suspend NULL
#define max98505_resume NULL
#endif

static int max98505_probe(struct snd_soc_codec *codec)
{
	struct max98505_priv *max98505 = snd_soc_codec_get_drvdata(codec);
	struct max98505_cdata *cdata;
	int ret = 0;
	int reg = 0;

	dev_info(codec->dev, "MONO - built on %s at %s\n",
		__DATE__,
		__TIME__);
	dev_info(codec->dev, "build number %s\n", MAX98505_REVISION);

	max98505->codec = codec;
	codec->control_data = max98505->regmap;

	ret = snd_soc_codec_set_cache_io(codec, 8, 8, SND_SOC_I2C);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}

	max98505->sysclk = 12288000;
	max98505->volume = 0x07;

	cdata = &max98505->dai[0];
	cdata->rate = (unsigned)-1;
	cdata->fmt  = (unsigned)-1;

	reg = 0;
	ret = regmap_read(max98505->regmap, MAX98505_R0FF_VERSION, &reg);
	if ((ret < 0) || ((reg != MAX98505_VERSION) && (reg != MAX98505_VERSION1) && (reg != MAX98505_VERSION2))) {
		dev_err(codec->dev,
			"device initialization error (%d 0x%02X)\n",
			ret,
			reg);
		goto err_access;
	}
	dev_info(codec->dev, "device version 0x%02X\n", reg);

#if 0
	/* FOR DEBUGGING ONLY */
	regcache_cache_bypass(max98505->regmap, true);
	dev_info(codec->dev, "regmap cache bypass ENABLED!\n");
	/**********************/
#endif

	regmap_write(max98505->regmap, MAX98505_R038_GLOBAL_ENABLE, 0x00);

	/* It's not the default but we need to set DAI_DLY */
	regmap_write(max98505->regmap, MAX98505_R020_FORMAT, M98505_DAI_DLY_MASK);

	regmap_write(max98505->regmap, MAX98505_R021_TDM_SLOT_SELECT, 0xC8);

	regmap_write(max98505->regmap, MAX98505_R027_DOUT_HIZ_CFG1, 0xFF);
	regmap_write(max98505->regmap, MAX98505_R028_DOUT_HIZ_CFG2, 0xFF);
	regmap_write(max98505->regmap, MAX98505_R029_DOUT_HIZ_CFG3, 0xFF);
	regmap_write(max98505->regmap, MAX98505_R02A_DOUT_HIZ_CFG4, 0xF0);

	regmap_write(max98505->regmap, MAX98505_R02C_FILTERS, 0xD8);

//	regmap_write(max98505->regmap, MAX98505_R034_ALC_CONFIGURATION, 0xF8);
	regmap_write(max98505->regmap, MAX98505_R034_ALC_CONFIGURATION, 0x12);


	/*****************************************************************/
	/* Set boost output to minimum until DSM is implemented          */
	regmap_write(max98505->regmap, MAX98505_R037_CONFIGURATION, 0xF0);
	/*****************************************************************/

	// Disable ALC muting
	regmap_write(max98505->regmap, MAX98505_R03A_BOOST_LIMITER, 0xF8);

	regmap_update_bits(max98505->regmap, MAX98505_R02D_GAIN,
			M98505_DAC_IN_SEL_MASK, M98505_DAC_IN_SEL_DIV2_SUMMED_DAI);

	max98505_handle_pdata(codec);
	max98505_add_widgets(codec);

	ret = sysfs_create_group(&codec->dev->kobj, &maxim_attribute_group);
	if(ret) {
		pr_err("failed to create sysfs group [%d]", ret);
	}
err_access:

	pr_info("%s: exit %d\n", __func__, ret);
	ret = 0;   // temp
	return ret;
}

static int max98505_remove(struct snd_soc_codec *codec)
{
	pr_info("%s: enter\n", __func__);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_max98505 = {
	.probe            = max98505_probe,
	.remove           = max98505_remove,
	.set_bias_level   = max98505_set_bias_level,
	.suspend          = max98505_suspend,
	.resume           = max98505_resume,
};

static const struct regmap_config max98505_regmap = {
	.reg_bits         = 8,
	.val_bits         = 8,
	.max_register     = MAX98505_R0FF_VERSION,
	.reg_defaults     = max98505_reg,
	.num_reg_defaults = ARRAY_SIZE(max98505_reg),
	.volatile_reg     = max98505_volatile_register,
	.readable_reg     = max98505_readable_register,
	.cache_type       = REGCACHE_RBTREE,
};

static int max98505_i2c_probe(struct i2c_client *i2c_l,
			     const struct i2c_device_id *id)
{
	struct max98505_priv *max98505;
	int ret;

	pr_info("%s: enter, device '%s'\n", __func__, id->name);

#ifdef SUPPORT_DEVICE_TREE
	max98505_regulator_config(i2c_l, of_property_read_bool(i2c_l->dev.of_node,
			"max98505,i2c-pull-up"), 1);
#endif

	max98505 = kzalloc(sizeof(struct max98505_priv), GFP_KERNEL);
	if (max98505 == NULL)
		return -ENOMEM;

	max98505->devtype = id->driver_data;
	i2c_set_clientdata(i2c_l, max98505);
	max98505->control_data = i2c_l;
	max98505->pdata = i2c_l->dev.platform_data;

	max98505->regmap = regmap_init_i2c(i2c_l, &max98505_regmap);
	if (IS_ERR(max98505->regmap)) {
		ret = PTR_ERR(max98505->regmap);
		dev_err(&i2c_l->dev, "Failed to allocate regmap: %d\n", ret);
		goto err_out;
	}

	ret = snd_soc_register_codec(&i2c_l->dev, &soc_codec_dev_max98505,
			max98505_dai, ARRAY_SIZE(max98505_dai));

err_out:

	if (ret < 0) {
		if (max98505->regmap)
			regmap_exit(max98505->regmap);
		kfree(max98505);
	}
	dsm_misc_device_init();
	pr_info("%s: ret %d\n", __func__, ret);

	return ret;
}

static int max98505_i2c_remove(struct i2c_client *client)
{
	struct max98505_priv *max98505 = dev_get_drvdata(&client->dev);

	snd_soc_unregister_codec(&client->dev);
	if (max98505->regmap)
		regmap_exit(max98505->regmap);
	kfree(i2c_get_clientdata(client));

	pr_info("%s: exit\n", __func__);
	dsm_misc_device_deinit();

	return 0;
}

static const struct i2c_device_id max98505_i2c_id[] = {
	{ "max98505", MAX98505 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max98505_i2c_id);

static struct i2c_driver max98505_i2c_driver = {
	.driver = {
		.name = "max98505",
		.owner = THIS_MODULE,
	},
	.probe  = max98505_i2c_probe,
	.remove = max98505_i2c_remove,
	.id_table = max98505_i2c_id,
};

module_i2c_driver(max98505_i2c_driver);

MODULE_DESCRIPTION("ALSA SoC MAX98505 driver");
MODULE_AUTHOR("Ralph Birt <rdbirt@gmail.com>");
MODULE_LICENSE("GPL");
