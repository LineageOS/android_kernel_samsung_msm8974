/*
 * max77803.c - mfd core driver for the Maxim 77803
 *
 * Copyright (C) 2011 Samsung Electronics
 * SangYoung Son <hello.son@smasung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This driver is based on max8997.c
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/mfd/core.h>
#include <linux/mfd/max77803.h>
#include <linux/mfd/max77803-private.h>
#include <linux/regulator/machine.h>

//#include <mach/sec_debug.h>
#include <linux/mfd/pm8xxx/misc.h>
#if defined (CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif
#define I2C_ADDR_PMIC	(0xCC >> 1)	/* Charger, Flash LED */
#define I2C_ADDR_MUIC	(0x4A >> 1)
#define I2C_ADDR_HAPTIC	(0x90 >> 1)

static struct mfd_cell max77803_devs[] = {
	{ .name = "max77803-charger", },
	{ .name = "max77803-led", },
	{ .name = "max77803-muic", },
	{ .name = "max77803-safeout", },
	{ .name = "max77803-haptic", },
};

int max77803_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct max77803_dev *max77803 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77803->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&max77803->iolock);
	if (ret < 0) {
		dev_err(max77803->dev,
			"%s, reg(0x%x), ret(%d)\n", __func__, reg, ret);
		return ret;
	}

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL_GPL(max77803_read_reg);

int max77803_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77803_dev *max77803 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77803->iolock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max77803->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(max77803_bulk_read);

int max77803_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct max77803_dev *max77803 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77803->iolock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	if (ret < 0)
		dev_err(max77803->dev,
			"%s, reg(0x%x), ret(%d)\n", __func__, reg, ret);
	mutex_unlock(&max77803->iolock);
	return ret;
}
EXPORT_SYMBOL_GPL(max77803_write_reg);

int max77803_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77803_dev *max77803 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77803->iolock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max77803->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(max77803_bulk_write);

int max77803_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct max77803_dev *max77803 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77803->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		u8 old_val = ret & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&max77803->iolock);
	return ret;
}
EXPORT_SYMBOL_GPL(max77803_update_reg);

static int of_max77803_dt(struct device *dev, struct max77803_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
#ifdef CONFIG_SS_VIBRATOR
	struct max77803_haptic_platform_data *haptic_data;
#endif
	int retval = 0;
	
#ifdef CONFIG_SS_VIBRATOR
	haptic_data = kzalloc(sizeof(struct max77803_haptic_platform_data), GFP_KERNEL);
	if (haptic_data == NULL)
		return -ENOMEM;
#endif
	if(!np)
		return -EINVAL;

	pdata->irq_gpio = of_get_named_gpio_flags(np, "max77803,irq-gpio", 
				0, &pdata->irq_gpio_flags);
	of_property_read_u32(np, "max77803,irq-base", &pdata->irq_base);
	pdata->wakeup = of_property_read_bool(np, "max77803,wakeup");
	retval = of_get_named_gpio(np, "max77803,wc-irq-gpio", 0);
	if (retval < 0)
		pdata->wc_irq_gpio = 0;
	else
		pdata->wc_irq_gpio = retval;
	
	pr_info("%s: irq-gpio: %u \n", __func__, pdata->irq_gpio);
	pr_info("%s: irq-base: %u \n", __func__, pdata->irq_base);
	pr_info("%s: wc-irq-gpio: %u \n", __func__, pdata->wc_irq_gpio);
#ifdef CONFIG_SS_VIBRATOR
	of_property_read_u32(np, "haptic,max_timeout", &haptic_data->max_timeout);
	of_property_read_u32(np, "haptic,duty", &haptic_data->duty);
	of_property_read_u32(np, "haptic,period", &haptic_data->period);
	of_property_read_u32(np, "haptic,pwm_id", &haptic_data->pwm_id);
	pr_info("%s: timeout: %u \n", __func__, haptic_data->max_timeout);
	pr_info("%s: duty: %u \n", __func__, haptic_data->duty);
	pr_info("%s: period: %u \n", __func__, haptic_data->period);
	pr_info("%s: pwm_id: %u \n", __func__, haptic_data->pwm_id);
	pdata->haptic_data = haptic_data;
#endif
	return 0;
}

static int max77803_i2c_probe(struct i2c_client *i2c,
			      const struct i2c_device_id *id)
{
	struct max77803_dev *max77803;
	struct max77803_platform_data *pdata;
	u8 reg_data;
	int ret = 0;
	dev_info(&i2c->dev, "%s\n", __func__);

	max77803 = kzalloc(sizeof(struct max77803_dev), GFP_KERNEL);
	if (max77803 == NULL)
		return -ENOMEM;

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev,
				sizeof(struct max77803_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&i2c->dev, "Failed to allocate memory \n");
			ret = -ENOMEM;
			goto err;
		}

		ret = of_max77803_dt(&i2c->dev, pdata);
		if (ret < 0){
			dev_err(&i2c->dev, "Failed to get device of_node \n");
			return ret;
		}
		/*Filling the platform data*/
		pdata->muic_data = &max77803_muic;
#ifdef CONFIG_REGULATOR_MAX77803
		pdata->num_regulators = MAX77803_REG_MAX;
		pdata->regulators = max77803_regulators;
#endif
#ifdef CONFIG_LEDS_MAX77803
		pdata->led_data = &max77803_led_pdata;
#endif
		/*pdata update to other modules*/
		i2c->dev.platform_data = pdata;
	} else
		pdata = i2c->dev.platform_data;

	i2c_set_clientdata(i2c, max77803);
	max77803->dev = &i2c->dev;

	max77803->i2c = i2c;
	max77803->irq = i2c->irq;
//	max77803->type = id->driver_data;
	if (pdata) {
		max77803->irq_base = pdata->irq_base;
		max77803->irq_gpio = pdata->irq_gpio;
		max77803->wakeup = pdata->wakeup;
		gpio_tlmm_config(GPIO_CFG(max77803->irq_gpio,  0, GPIO_CFG_INPUT,
                GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
	} else {
		ret = -EINVAL;
		goto err;
	}

	mutex_init(&max77803->iolock);

	if (max77803_read_reg(i2c, MAX77803_PMIC_REG_PMIC_ID2, &reg_data) < 0) {
		dev_err(max77803->dev,
			"device not found on this channel (this is not an error)\n");
		ret = -ENODEV;
		goto err;
	} else {
		/* print rev */
		max77803->pmic_rev = (reg_data & 0x7);
		max77803->pmic_ver = ((reg_data & 0xF8) >> 0x3);
		pr_info("%s: device found: rev.0x%x, ver.0x%x\n", __func__,
				max77803->pmic_rev, max77803->pmic_ver);
	}
#if 0
#if defined(CONFIG_MACH_JF_VZW) || defined(CONFIG_MACH_JF_LGT)
	if (kernel_sec_get_debug_level() == KERNEL_SEC_DEBUG_LEVEL_LOW) {
		pm8xxx_hard_reset_config(PM8XXX_DISABLE_HARD_RESET);
		max77803_write_reg(i2c, MAX77803_PMIC_REG_MAINCTRL1, 0x04);
	} else {
		pm8xxx_hard_reset_config(PM8XXX_DISABLE_HARD_RESET);
		max77803_write_reg(i2c, MAX77803_PMIC_REG_MAINCTRL1, 0x0c);
	}
#else
	if (kernel_sec_get_debug_level() == KERNEL_SEC_DEBUG_LEVEL_LOW) {
		max77803_write_reg(i2c, MAX77803_PMIC_REG_MAINCTRL1, 0x04);
	} else {
		pm8xxx_hard_reset_config(PM8XXX_DISABLE_HARD_RESET);
		max77803_write_reg(i2c, MAX77803_PMIC_REG_MAINCTRL1, 0x0c);
	}
#endif
#endif
	max77803_update_reg(i2c, MAX77803_CHG_REG_SAFEOUT_CTRL, 0x00, 0x30);

	max77803->muic = i2c_new_dummy(i2c->adapter, I2C_ADDR_MUIC);
	i2c_set_clientdata(max77803->muic, max77803);

	max77803->haptic = i2c_new_dummy(i2c->adapter, I2C_ADDR_HAPTIC);
	i2c_set_clientdata(max77803->haptic, max77803);

	ret = max77803_irq_init(max77803);
	if (ret < 0)
		goto err_irq_init;

	ret = mfd_add_devices(max77803->dev, -1, max77803_devs,
			ARRAY_SIZE(max77803_devs), NULL, 0);
	if (ret < 0)
		goto err_mfd;

	device_init_wakeup(max77803->dev, pdata->wakeup);

	return ret;

err_mfd:
	mfd_remove_devices(max77803->dev);
	max77803_irq_exit(max77803);
err_irq_init:
	i2c_unregister_device(max77803->muic);
	i2c_unregister_device(max77803->haptic);
err:
	kfree(max77803);
	return ret;
}

static int max77803_i2c_remove(struct i2c_client *i2c)
{
	struct max77803_dev *max77803 = i2c_get_clientdata(i2c);

	mfd_remove_devices(max77803->dev);
	i2c_unregister_device(max77803->muic);
	i2c_unregister_device(max77803->haptic);
	kfree(max77803);

	return 0;
}

static const struct i2c_device_id max77803_i2c_id[] = {
	{ "max77803", TYPE_MAX77803 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max77803_i2c_id);
static struct of_device_id max77803_i2c_match_table[] = {
	{ .compatible = "max77803,i2c", },
	{ },
};
MODULE_DEVICE_TABLE(of, max77803_i2c_match_table);

#ifdef CONFIG_PM
static int max77803_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77803_dev *max77803 = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		enable_irq_wake(max77803->irq);

	return 0;
}

static int max77803_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77803_dev *max77803 = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		disable_irq_wake(max77803->irq);

	return max77803_irq_resume(max77803);
}
#else
#define max77803_suspend	NULL
#define max77803_resume		NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_HIBERNATION

u8 max77803_dumpaddr_pmic[] = {
	MAX77803_LED_REG_IFLASH1,
	MAX77803_LED_REG_IFLASH2,
	MAX77803_LED_REG_ITORCH,
	MAX77803_LED_REG_ITORCHTORCHTIMER,
	MAX77803_LED_REG_FLASH_TIMER,
	MAX77803_LED_REG_FLASH_EN,
	MAX77803_LED_REG_MAX_FLASH1,
	MAX77803_LED_REG_MAX_FLASH2,
	MAX77803_LED_REG_VOUT_CNTL,
	MAX77803_LED_REG_VOUT_FLASH1,
	MAX77803_LED_REG_FLASH_INT_STATUS,

	MAX77803_PMIC_REG_TOPSYS_INT_MASK,
	MAX77803_PMIC_REG_MAINCTRL1,
	MAX77803_PMIC_REG_LSCNFG,
	MAX77803_CHG_REG_CHG_INT_MASK,
	MAX77803_CHG_REG_CHG_CNFG_00,
	MAX77803_CHG_REG_CHG_CNFG_01,
	MAX77803_CHG_REG_CHG_CNFG_02,
	MAX77803_CHG_REG_CHG_CNFG_03,
	MAX77803_CHG_REG_CHG_CNFG_04,
	MAX77803_CHG_REG_CHG_CNFG_05,
	MAX77803_CHG_REG_CHG_CNFG_06,
	MAX77803_CHG_REG_CHG_CNFG_07,
	MAX77803_CHG_REG_CHG_CNFG_08,
	MAX77803_CHG_REG_CHG_CNFG_09,
	MAX77803_CHG_REG_CHG_CNFG_10,
	MAX77803_CHG_REG_CHG_CNFG_11,
	MAX77803_CHG_REG_CHG_CNFG_12,
	MAX77803_CHG_REG_CHG_CNFG_13,
	MAX77803_CHG_REG_CHG_CNFG_14,
	MAX77803_CHG_REG_SAFEOUT_CTRL,
};

u8 max77803_dumpaddr_muic[] = {
	MAX77803_MUIC_REG_INTMASK1,
	MAX77803_MUIC_REG_INTMASK2,
	MAX77803_MUIC_REG_INTMASK3,
	MAX77803_MUIC_REG_CDETCTRL1,
	MAX77803_MUIC_REG_CDETCTRL2,
	MAX77803_MUIC_REG_CTRL1,
	MAX77803_MUIC_REG_CTRL2,
	MAX77803_MUIC_REG_CTRL3,
};


u8 max77803_dumpaddr_haptic[] = {
	MAX77803_HAPTIC_REG_CONFIG1,
	MAX77803_HAPTIC_REG_CONFIG2,
	MAX77803_HAPTIC_REG_CONFIG_CHNL,
	MAX77803_HAPTIC_REG_CONFG_CYC1,
	MAX77803_HAPTIC_REG_CONFG_CYC2,
	MAX77803_HAPTIC_REG_CONFIG_PER1,
	MAX77803_HAPTIC_REG_CONFIG_PER2,
	MAX77803_HAPTIC_REG_CONFIG_PER3,
	MAX77803_HAPTIC_REG_CONFIG_PER4,
	MAX77803_HAPTIC_REG_CONFIG_DUTY1,
	MAX77803_HAPTIC_REG_CONFIG_DUTY2,
	MAX77803_HAPTIC_REG_CONFIG_PWM1,
	MAX77803_HAPTIC_REG_CONFIG_PWM2,
	MAX77803_HAPTIC_REG_CONFIG_PWM3,
	MAX77803_HAPTIC_REG_CONFIG_PWM4,
};


static int max77803_freeze(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77803_dev *max77803 = i2c_get_clientdata(i2c);
	int i;

	for (i = 0; i < ARRAY_SIZE(max77803_dumpaddr_pmic); i++)
		max77803_read_reg(i2c, max77803_dumpaddr_pmic[i],
				&max77803->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77803_dumpaddr_muic); i++)
		max77803_read_reg(i2c, max77803_dumpaddr_muic[i],
				&max77803->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77803_dumpaddr_haptic); i++)
		max77803_read_reg(i2c, max77803_dumpaddr_haptic[i],
				&max77803->reg_haptic_dump[i]);

	disable_irq(max77803->irq);

	return 0;
}

static int max77803_restore(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77803_dev *max77803 = i2c_get_clientdata(i2c);
	int i;

	enable_irq(max77803->irq);

	for (i = 0; i < ARRAY_SIZE(max77803_dumpaddr_pmic); i++)
		max77803_write_reg(i2c, max77803_dumpaddr_pmic[i],
				max77803->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77803_dumpaddr_muic); i++)
		max77803_write_reg(i2c, max77803_dumpaddr_muic[i],
				max77803->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77803_dumpaddr_haptic); i++)
		max77803_write_reg(i2c, max77803_dumpaddr_haptic[i],
				max77803->reg_haptic_dump[i]);


	return 0;
}
#endif


const struct dev_pm_ops max77803_pm = {
	.suspend = max77803_suspend,
	.resume = max77803_resume,
#ifdef CONFIG_HIBERNATION
	.freeze =  max77803_freeze,
	.thaw = max77803_restore,
	.restore = max77803_restore,
#endif
};

static struct i2c_driver max77803_i2c_driver = {
	.driver = {
		.name = "max77803",
		.owner = THIS_MODULE,
		.pm = &max77803_pm,
		.of_match_table = max77803_i2c_match_table,
	},
	.probe = max77803_i2c_probe,
	.remove = max77803_i2c_remove,
	.id_table = max77803_i2c_id,
};

//module_i2c_driver(max77803_i2c_driver);

static int __init max77803_i2c_init(void)
{
	return i2c_add_driver(&max77803_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(max77803_i2c_init);

static void __exit max77803_i2c_exit(void)
{
	i2c_del_driver(&max77803_i2c_driver);
}
module_exit(max77803_i2c_exit);

MODULE_DESCRIPTION("MAXIM 77803 multi-function core driver");
MODULE_AUTHOR("SangYoung, Son <hello.son@samsung.com>");
MODULE_LICENSE("GPL");
