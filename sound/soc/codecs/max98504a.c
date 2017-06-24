/*
* max98504.c -- MAX98504 SoC Audio driver
*
* Copyright 2013-2014 Maxim Integrated Products
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <sound/soc.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <sound/tlv.h>
#include <sound/tlv.h>
#include <sound/max98504a.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>

#include "max98504a.h"

#ifdef DEBUG_MAX98504
#define msg_maxim(format, args...)	\
	printk(KERN_INFO "[MAX98504_DEBUG] %s " format, __func__, ## args)
#else
#define msg_maxim(format, args...)
#endif

static struct regmap *regmap;

static DEFINE_MUTEX(max98504_lock);

static struct reg_default max98504_reg[] = {
	{ MAX98504_REG_01_INTERRUPT_STATUS, 		0x00 },
	{ MAX98504_REG_02_INTERRUPT_FLAGS, 			0x00 },
	{ MAX98504_REG_03_INTERRUPT_ENABLES, 		0x00 },
	{ MAX98504_REG_04_INTERRUPT_FLAG_CLEARS, 	0x00 },
	{ MAX98504_REG_10_GPIO_ENABLE, 				0x00 },
	{ MAX98504_REG_11_GPIO_CONFIG, 				0x00 },
	{ MAX98504_REG_12_WATCHDOG_ENABLE,			0x00 },
	{ MAX98504_REG_13_WATCHDOG_CONFIG,			0x00 },
	{ MAX98504_REG_14_WATCHDOG_CLEAR,			0x00 },
	{ MAX98504_REG_15_CLOCK_MONITOR_ENABLE,		0x00 },
	{ MAX98504_REG_16_PVDD_BROWNOUT_ENABLE,		0x00 },
	{ MAX98504_REG_17_PVDD_BROWNOUT_CONFIG_1, 	0x00 },
	{ MAX98504_REG_18_PVDD_BROWNOUT_CONFIG_2, 	0x00 },
	{ MAX98504_REG_19_PVDD_BROWNOUT_CONFIG_3, 	0x00 },
	{ MAX98504_REG_1A_PVDD_BROWNOUT_CONFIG_4, 	0x00 },
	{ MAX98504_REG_20_PCM_RX_ENABLES,			0x00 },
	{ MAX98504_REG_21_PCM_TX_ENABLES,			0x00 },
	{ MAX98504_REG_22_PCM_TX_HIZ_CONTROL,		0x00 },
	{ MAX98504_REG_23_PCM_TX_CHANNEL_SOURCES, 	0x00 },
	{ MAX98504_REG_24_PCM_MODE_CONFIG, 			0x00 },
	{ MAX98504_REG_25_PCM_DSP_CONFIG, 			0x00 },
	{ MAX98504_REG_26_PCM_CLOCK_SETUP, 			0x00 },
	{ MAX98504_REG_27_PCM_SAMPLE_RATE_SETUP, 	0x00 },
	{ MAX98504_REG_28_PCM_TO_SPEAKER_MONOMIX, 	0x00 },
	{ MAX98504_REG_30_PDM_TX_ENABLES, 			0x00 },
	{ MAX98504_REG_31_PDM_TX_HIZ_CONTROL, 		0x00 },
	{ MAX98504_REG_32_PDM_TX_CONTROL, 			0x00 },
	{ MAX98504_REG_33_PDM_RX_ENABLE, 			0x00 },
	{ MAX98504_REG_34_SPEAKER_ENABLE, 			0x00 },
	{ MAX98504_REG_35_SPEAKER_SOURCE_SELECT, 	0x00 },
	{ MAX98504_REG_36_MEASUREMENT_ENABLES, 		0x00 },
	{ MAX98504_REG_37_ANALOGUE_INPUT_GAIN, 		0x00 },
	{ MAX98504_REG_38_TEMPERATURE_LIMIT_CONFIG, 0x00 },
	{ MAX98504_REG_39_ANALOGUE_SPARE, 			0x00 },
	{ MAX98504_REG_40_GLOBAL_ENABLE, 			0x00 },
	{ MAX98504_REG_41_SOFTWARE_RESET, 			0x00 },
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

static bool max98504_volatile_register(struct device *dev, unsigned int reg)
{
	if (max98504_reg_access[reg].vol)
		return 1;
	else
		return 0;
}

static bool max98504_readable_register(struct device *dev, unsigned int reg)
{
	if (reg >= MAX98504_REG_CNT)
		return 0;
	return max98504_reg_access[reg].read != 0;
}

static int max98504_reset(struct max98504_priv *max98504)
{
	int ret;

	ret = regmap_write(max98504->regmap, MAX98504_REG_41_SOFTWARE_RESET, M98504_SOFTWARE_RESET_MASK);
	msleep(10);

	return ret;
}

static int max98504_probe(struct max98504_priv *max98504)
{
	struct max98504_pdata *pdata = max98504->pdata;
	struct max98504_cfg_data *cfg_data = &pdata->cfg_data;
		
	u8 regval;
	int ret;
	unsigned int value;
	

	msg_maxim("\n");

	max98504_reset(max98504);

	ret = regmap_read(max98504->regmap, MAX98504_REG_7FFF_REV_ID, &value);
	if (ret < 0) {
		pr_err("Failed to read device revision: %d\n",
			ret);
		goto err_access;
	}
	msg_maxim("REV ID=0x%x\n", value);

	if (!pdata) {
		pr_err("No platform data\n");
		return ret;
	}
	/* Configure Rx Mode */
	if(pdata->rx_mode == MODE_RX_PCM)	{
		regval = 0;
		if(cfg_data->rx_dither_en) regval |= M98504_PCM_DSP_CFG_RX_DITH_EN_MASK;
		if(cfg_data->rx_flt_mode)	regval |= M98504_PCM_DSP_CFG_RX_FLT_MODE_MASK;
		regmap_update_bits(max98504->regmap, MAX98504_REG_25_PCM_DSP_CONFIG, M98504_PCM_DSP_CFG_RX_DITH_EN_MASK|M98504_PCM_DSP_CFG_RX_FLT_MODE_MASK, regval);
		regmap_write(max98504->regmap,  MAX98504_REG_20_PCM_RX_ENABLES, (u8)cfg_data->rx_ch_en);
	}
	else if(pdata->rx_mode==MODE_RX_PDM0 || pdata->rx_mode==MODE_RX_PDM1)	{
		regmap_write(max98504->regmap,  MAX98504_REG_33_PDM_RX_ENABLE, M98504_PDM_RX_EN_MASK);
	}
	regmap_write(max98504->regmap,  MAX98504_REG_35_SPEAKER_SOURCE_SELECT, (u8) (M98504_SPK_SRC_SEL_MASK & pdata->rx_mode));

	/* Configure Tx Mode */
	if(pdata->tx_mode == MODE_TX_PCM)	{
		regval = 0;
		if(cfg_data->tx_dither_en) regval |= M98504_PCM_DSP_CFG_TX_DITH_EN_MASK;
		if(cfg_data->meas_dc_block_en) regval |= M98504_PCM_DSP_CFG_MEAS_DCBLK_EN_MASK;
		regmap_update_bits(max98504->regmap, MAX98504_REG_25_PCM_DSP_CONFIG, M98504_PCM_DSP_CFG_TX_DITH_EN_MASK|M98504_PCM_DSP_CFG_MEAS_DCBLK_EN_MASK, regval);

		regmap_write(max98504->regmap,  MAX98504_REG_21_PCM_TX_ENABLES, (u8)cfg_data->tx_ch_en);
		regmap_write(max98504->regmap,  MAX98504_REG_22_PCM_TX_HIZ_CONTROL, (u8)cfg_data->tx_hiz_ch_en);
		regmap_write(max98504->regmap,  MAX98504_REG_23_PCM_TX_CHANNEL_SOURCES, (u8)cfg_data->tx_ch_src);		
	}
	else {
		regmap_write(max98504->regmap,  MAX98504_REG_30_PDM_TX_ENABLES, (u8)cfg_data->tx_ch_en);
		regmap_write(max98504->regmap,  MAX98504_REG_31_PDM_TX_HIZ_CONTROL, (u8)cfg_data->tx_hiz_ch_en);
		regmap_write(max98504->regmap,  MAX98504_REG_32_PDM_TX_CONTROL, (u8)cfg_data->tx_ch_src);
	}

	regmap_write(max98504->regmap,	MAX98504_REG_36_MEASUREMENT_ENABLES, M98504_MEAS_I_EN_MASK | M98504_MEAS_V_EN_MASK);

	/* Brownout Protection */
	regmap_write(max98504->regmap, MAX98504_REG_16_PVDD_BROWNOUT_ENABLE, 0x1);
	regmap_write(max98504->regmap, MAX98504_REG_17_PVDD_BROWNOUT_CONFIG_1, 0x3);
	regmap_write(max98504->regmap, MAX98504_REG_18_PVDD_BROWNOUT_CONFIG_2, 0x64);
	regmap_write(max98504->regmap, MAX98504_REG_19_PVDD_BROWNOUT_CONFIG_3, 0xff);
	regmap_write(max98504->regmap, MAX98504_REG_1A_PVDD_BROWNOUT_CONFIG_4, 0xff);

	#if defined(CONFIG_SEC_S_PROJECT) //bypass temp code 
		max98504_set_speaker_status(1);
	#endif
	
	return ret;
	
err_access:
	return ret;
}

int max98504_set_speaker_status(int OnOff)
{
	mutex_lock(&max98504_lock);

	if(regmap==NULL)	{
		pr_err("Speaker control is not allowed.\n");
		return -1;
	}
		
	if(OnOff)	{
		regmap_update_bits(regmap, MAX98504_REG_34_SPEAKER_ENABLE,
			M98504_SPK_EN_MASK, M98504_SPK_EN_MASK);
		regmap_update_bits(regmap, MAX98504_REG_40_GLOBAL_ENABLE,
			M98504_GLOBAL_EN_MASK, M98504_GLOBAL_EN_MASK);
	}
	else	{
		regmap_update_bits(regmap, MAX98504_REG_40_GLOBAL_ENABLE,
			M98504_GLOBAL_EN_MASK, 0);
		regmap_update_bits(regmap, MAX98504_REG_34_SPEAKER_ENABLE,
			M98504_SPK_EN_MASK, 0);
	}

	mutex_unlock(&max98504_lock);

	return 0;
}
bool max98504_get_speaker_status(void)
{
	unsigned int ret;
	mutex_lock(&max98504_lock);

	if(regmap==NULL)	{
		pr_err("Speaker control is not allowed.\n");
		return 0;
	}
	regmap_read(regmap, MAX98504_REG_34_SPEAKER_ENABLE, &ret);

	mutex_unlock(&max98504_lock);
	return ret>0?true:false;
}

EXPORT_SYMBOL_GPL(max98504_set_speaker_status);
EXPORT_SYMBOL_GPL(max98504_get_speaker_status);

static const struct regmap_config max98504_regmap = {
	.reg_bits         = 16,
	.val_bits         = 8,
	.max_register     = MAX98504_REG_CNT,
	.reg_defaults     = max98504_reg,
	.num_reg_defaults = ARRAY_SIZE(max98504_reg),
	.volatile_reg     = max98504_volatile_register,
	.readable_reg     = max98504_readable_register,
	.cache_type       = REGCACHE_RBTREE,
};

static int reg_set_optimum_mode_check(struct regulator *reg, int load_uA)
{
	return (regulator_count_voltages(reg) > 0) ?
			regulator_set_optimum_mode(reg, load_uA) : 0;
}

static int max98504_regulator_config(struct i2c_client *i2c, bool pullup, bool on)
{
	struct regulator *max98504_vcc_i2c;
	int rc;
    #define VCC_I2C_MIN_UV	1800000
    #define VCC_I2C_MAX_UV	1800000
	#define I2C_LOAD_UA		300000

	if (pullup) {
		max98504_vcc_i2c = regulator_get(&i2c->dev, "vcc_i2c");

		if (IS_ERR(max98504_vcc_i2c)) {
			rc = PTR_ERR(max98504_vcc_i2c);
			pr_info("Regulator get failed rc=%d\n",	rc);
			goto error_get_vtg_i2c;
		}

		if (regulator_count_voltages(max98504_vcc_i2c) > 0) {
			rc = regulator_set_voltage(max98504_vcc_i2c, VCC_I2C_MIN_UV, VCC_I2C_MAX_UV);
			if (rc) {
				pr_info("regulator set_vtg failed rc=%d\n", rc);
				goto error_set_vtg_i2c;
			}
		}

		rc = reg_set_optimum_mode_check(max98504_vcc_i2c, I2C_LOAD_UA);
		if (rc < 0) {
			pr_info("Regulator vcc_i2c set_opt failed rc=%d\n", rc);
			goto error_reg_opt_i2c;
		}
		
		rc = regulator_enable(max98504_vcc_i2c);
		if (rc) {
			pr_info("Regulator vcc_i2c enable failed rc=%d\n", rc);
			goto error_reg_en_vcc_i2c;
		}
	}

	return 0;

	error_set_vtg_i2c:
		regulator_put(max98504_vcc_i2c);
	error_get_vtg_i2c:
		if (regulator_count_voltages(max98504_vcc_i2c) > 0)
			regulator_set_voltage(max98504_vcc_i2c, 0, VCC_I2C_MAX_UV);
	error_reg_en_vcc_i2c:
		if(pullup) 	reg_set_optimum_mode_check(max98504_vcc_i2c, 0);
	error_reg_opt_i2c:
		regulator_disable(max98504_vcc_i2c);

	return rc;
}

#ifdef USE_MAX98504_IRQ
static irqreturn_t max98504_interrupt(int irq, void *data)
{
	struct max98504_priv *max98504 = (struct max98504_priv * )data;

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
	if (flag & M98504_INT_GENFAIL_EN_MASK) {
		msg_maxim("M98504_INT_GENFAIL_EN_MASK active!");
	}

	if (flag & M98504_INT_AUTHDONE_EN_MASK) {
		msg_maxim("M98504_INT_AUTHDONE_EN_MASK active!");
	}

	if (flag & M98504_INT_VBATBROWN_EN_MASK) {
		msg_maxim("M98504_INT_VBATBROWN_EN_MASK active!");
	}

	if (flag & M98504_INT_WATCHFAIL_EN_MASK) {
		msg_maxim("M98504_INT_WATCHFAIL_EN_MASK active!");
	}

	if (flag & M98504_INT_THERMWARN_END_EN_MASK) {
		msg_maxim("M98504_INT_THERMWARN_END_EN_MASK active!");
	}

	if (flag & M98504_INT_THERMWARN_BGN_EN_MASK) {
		msg_maxim("M98504_INT_THERMWARN_BGN_EN_MASK active!\n");
	}
	if (flag & M98504_INT_THERMSHDN_END_EN_MASK) {
		msg_maxim("M98504_INT_THERMSHDN_END_EN_MASK active!\n");
	}
	if (flag & M98504_INT_THERMSHDN_BGN_FLAG_MASK) {
		msg_maxim("M98504_INT_THERMSHDN_BGN_FLAG_MASK active!\n");
	}
	regmap_write(max98504->regmap, MAX98504_REG_04_INTERRUPT_FLAG_CLEARS, flag&0xff);

	return IRQ_HANDLED;
}
#endif

static int __devinit max98504_i2c_probe(struct i2c_client *i2c,
		const struct i2c_device_id *id)
{
	struct max98504_priv *max98504;
	struct max98504_pdata *pdata;
	int ret;

	msg_maxim("\n");

	max98504 = kzalloc(sizeof(struct max98504_priv), GFP_KERNEL);
	if (max98504 == NULL)
		return -ENOMEM;

	max98504->devtype = id->driver_data;
	i2c_set_clientdata(i2c, max98504);
	max98504->control_data = i2c;

	if (i2c->dev.of_node) {
		max98504->pdata = devm_kzalloc(&i2c->dev,
			sizeof(struct max98504_pdata), GFP_KERNEL);
		if (!max98504->pdata) {
			dev_err(&i2c->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		else pdata = max98504->pdata;

		#ifdef USE_MAX98504_IRQ
		pdata->irq = of_get_named_gpio_flags(i2c->dev.of_node, "max98504,irq-gpio",
					0, NULL);
		#endif

		ret = of_property_read_u32(i2c->dev.of_node, "max98504,rx_mode", &pdata->rx_mode);
		if (ret) {
			dev_err(&i2c->dev, "Failed to read rx_mode.\n");
			return -EINVAL;
		}

		ret = of_property_read_u32(i2c->dev.of_node, "max98504,tx_mode", &pdata->tx_mode);
		if (ret) {
			dev_err(&i2c->dev, "Failed to read tx_mode.\n");
			return -EINVAL;
		}

		ret = of_property_read_u32_array(i2c->dev.of_node, "max98504,cfg_data", 
			(u32*)&pdata->cfg_data, sizeof(struct max98504_cfg_data)/sizeof(u32));
		if (ret) {
			dev_err(&i2c->dev, "Failed to read cfg_data.\n");
			return -EINVAL;
		}

		msg_maxim("rx_mode:%d, tx_mode:%d, tx_dither_en:%d, rx_dither_en:%d, meas_dc_block_en:%d, rx_flt_mode:%d, rx_ch_en:%d\n",
			pdata->rx_mode,
			pdata->tx_mode,
			pdata->cfg_data.tx_dither_en, 
			pdata->cfg_data.rx_dither_en, 
			pdata->cfg_data.meas_dc_block_en, 
			pdata->cfg_data.rx_flt_mode, 
			pdata->cfg_data.rx_ch_en);
		msg_maxim("tx_ch_en:%d, tx_hiz_ch_en:%d, tx_ch_src:%d, auth_en:%d, wdog_time_out:%d\n",
			pdata->cfg_data.tx_ch_en, 
			pdata->cfg_data.tx_hiz_ch_en, 
			pdata->cfg_data.tx_ch_src, 
			pdata->cfg_data.auth_en, 
			pdata->cfg_data.wdog_time_out);
		
	}	else max98504->pdata = i2c->dev.platform_data;

	max98504_regulator_config(i2c, of_property_read_bool(i2c->dev.of_node, "max98504,i2c-pull-up"), 1);

	max98504->regmap = regmap = regmap_init_i2c(i2c, &max98504_regmap);
	
	if (IS_ERR(max98504->regmap)) {
		ret = PTR_ERR(max98504->regmap);
		dev_err(&i2c->dev, "Failed to allocate regmap: %d\n", ret);
		goto err_out;
	}

	max98504_probe(max98504);

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
	if (ret) {
		dev_err(&i2c->dev, "Failed to register interrupt\n");
	}

	err_irq_gpio_req:
		if (gpio_is_valid(pdata->irq))
			gpio_free(pdata->irq);
#endif

	err_out:
	
		if (ret < 0) {
			if (max98504->regmap)
				regmap_exit(max98504->regmap);
			kfree(max98504);
		}

	return 0;
}

static __devexit int max98504_i2c_remove(struct i2c_client *client)
{
	struct max98504_priv *max98504 = dev_get_drvdata(&client->dev);

	if (max98504->regmap)
		regmap_exit(max98504->regmap);
	kfree(i2c_get_clientdata(client));

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
	.probe = max98504_i2c_probe,
	.remove = __devexit_p(max98504_i2c_remove),
	.id_table = max98504_i2c_id,
};

static int __init max98504_init(void)
{
	return i2c_add_driver(&max98504_i2c_driver);
}
module_init(max98504_init);

static void __exit max98504_exit(void)
{
	i2c_del_driver(&max98504_i2c_driver);
}
module_exit(max98504_exit);

MODULE_DESCRIPTION("SoC MAX98504 driver");
MODULE_AUTHOR("Ryan Lee <ryans.lee@maximintegrated.com>");
MODULE_LICENSE("GPL");
