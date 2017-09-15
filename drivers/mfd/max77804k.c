/*
 * max77804k.c - mfd core driver for the Maxim 77804k
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
#include <linux/mfd/max77804k.h>
#include <linux/mfd/max77804k-private.h>
#include <linux/regulator/machine.h>

#include <linux/mfd/pm8xxx/misc.h>
#if defined (CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif
#define I2C_ADDR_PMIC	(0xCC >> 1)	/* Charger, Flash LED */
#define I2C_ADDR_MUIC	(0x4A >> 1)
#define I2C_ADDR_HAPTIC	(0x90 >> 1)

static struct mfd_cell max77804k_devs[] = {
	{ .name = "max77804k-charger", },
	{ .name = "max77804k-led", },
	{ .name = "max77804k-muic", },
	{ .name = "max77804k-safeout", },
	{ .name = "max77804k-haptic", },
};

#if defined(CONFIG_EXTCON)
struct max77804k_muic_data max77804k_muic = {
	.usb_sel = 0,
	.uart_sel = 0,
};
#endif

int max77804k_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct max77804k_dev *max77804k = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77804k->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&max77804k->iolock);
	if (ret < 0) {
		dev_err(max77804k->dev,
			"%s, reg(0x%x), ret(%d)\n", __func__, reg, ret);
		return ret;
	}

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL_GPL(max77804k_read_reg);

int max77804k_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77804k_dev *max77804k = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77804k->iolock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max77804k->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(max77804k_bulk_read);

int max77804k_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct max77804k_dev *max77804k = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77804k->iolock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	if (ret < 0)
		dev_err(max77804k->dev,
			"%s, reg(0x%x), ret(%d)\n", __func__, reg, ret);
	mutex_unlock(&max77804k->iolock);
	return ret;
}
EXPORT_SYMBOL_GPL(max77804k_write_reg);

int max77804k_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77804k_dev *max77804k = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77804k->iolock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max77804k->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(max77804k_bulk_write);

int max77804k_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct max77804k_dev *max77804k = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77804k->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		u8 old_val = ret & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&max77804k->iolock);
	return ret;
}
EXPORT_SYMBOL_GPL(max77804k_update_reg);

static int of_max77804k_dt(struct device *dev, struct max77804k_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int retval = 0;

#ifdef CONFIG_SS_VIBRATOR
	struct max77804k_haptic_platform_data *haptic_data;

	haptic_data = kzalloc(sizeof(struct max77804k_haptic_platform_data), GFP_KERNEL);
	if (haptic_data == NULL)
		return -ENOMEM;
	if(!np) {
		kfree(haptic_data);
		return -EINVAL;
	}
#endif
	pdata->irq_gpio = of_get_named_gpio_flags(np, "max77804k,irq-gpio",
				0, &pdata->irq_gpio_flags);
	pdata->irq_base = irq_alloc_descs(-1, 0, MAX77804K_IRQ_NR, -1);
	if (pdata->irq_base < 0) {
		pr_info("%s irq_alloc_descs is failed! irq_base:%d\n", __func__, pdata->irq_base);
		/* getting a predefined irq_base on dt file	*/
		of_property_read_u32(np, "max77804k,irq-base", &pdata->irq_base);
	}
	pdata->wakeup = of_property_read_bool(np, "max77804k,wakeup");
	retval = of_get_named_gpio(np, "max77804k,wc-irq-gpio", 0);
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
	kfree(haptic_data);
#endif
	return 0;
}

static int max77804k_i2c_probe(struct i2c_client *i2c,
			      const struct i2c_device_id *id)
{
	struct max77804k_dev *max77804k;
	struct max77804k_platform_data *pdata;
	u8 reg_data;
	int ret = 0;
	dev_info(&i2c->dev, "%s\n", __func__);

	max77804k = kzalloc(sizeof(struct max77804k_dev), GFP_KERNEL);
	if (max77804k == NULL)
		return -ENOMEM;

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev,
				sizeof(struct max77804k_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&i2c->dev, "Failed to allocate memory \n");
			ret = -ENOMEM;
			goto err;
		}

		ret = of_max77804k_dt(&i2c->dev, pdata);
		if (ret < 0){
			dev_err(&i2c->dev, "Failed to get device of_node \n");
			ret = -ENOMEM;
			goto err;
		}
		/*Filling the platform data*/
		pdata->muic_data = &max77804k_muic;
#ifdef CONFIG_REGULATOR_MAX77804K
		pdata->num_regulators = MAX77804K_REG_MAX;
		pdata->regulators = max77804k_regulators;
#endif
#ifdef CONFIG_LEDS_MAX77804K
		pdata->led_data = &max77804k_led_pdata;
#endif
		/*pdata update to other modules*/
		i2c->dev.platform_data = pdata;
	} else
		pdata = i2c->dev.platform_data;

	i2c_set_clientdata(i2c, max77804k);
	max77804k->dev = &i2c->dev;

	max77804k->i2c = i2c;
	max77804k->irq = i2c->irq;
	if (pdata) {
		max77804k->irq_base = pdata->irq_base;
		max77804k->irq_gpio = pdata->irq_gpio;
		max77804k->wakeup = pdata->wakeup;
		gpio_tlmm_config(GPIO_CFG(max77804k->irq_gpio,  0, GPIO_CFG_INPUT,
                GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
	} else {
		ret = -EINVAL;
		goto err;
	}

	mutex_init(&max77804k->iolock);

	if (max77804k_read_reg(i2c, MAX77804K_PMIC_REG_PMIC_ID2, &reg_data) < 0) {
		dev_err(max77804k->dev,
			"device not found on this channel (this is not an error)\n");
		ret = -ENODEV;
		goto err;
	} else {
		/* print rev */
		max77804k->pmic_rev = (reg_data & 0x7);
		max77804k->pmic_ver = ((reg_data & 0xF8) >> 0x3);
		pr_info("%s: device found: rev.0x%x, ver.0x%x\n", __func__,
				max77804k->pmic_rev, max77804k->pmic_ver);
	}
	max77804k_update_reg(i2c, MAX77804K_CHG_REG_SAFEOUT_CTRL, 0x00, 0x30);

	max77804k->muic = i2c_new_dummy(i2c->adapter, I2C_ADDR_MUIC);
	i2c_set_clientdata(max77804k->muic, max77804k);

	max77804k->haptic = i2c_new_dummy(i2c->adapter, I2C_ADDR_HAPTIC);
	i2c_set_clientdata(max77804k->haptic, max77804k);

	ret = max77804k_irq_init(max77804k);
	if (ret < 0)
		goto err_irq_init;

	/* disable manual reset */
	max77804k_read_reg(max77804k->i2c,
		MAX77804K_PMIC_REG_MAINCTRL1, &reg_data);
	reg_data &= ~(PMIC_MAINCTRL1_MREN_MASK);
	max77804k_write_reg(max77804k->i2c,
		MAX77804K_PMIC_REG_MAINCTRL1, reg_data);

	ret = mfd_add_devices(max77804k->dev, -1, max77804k_devs,
			ARRAY_SIZE(max77804k_devs), NULL, 0);
	if (ret < 0)
		goto err_mfd;

	device_init_wakeup(max77804k->dev, pdata->wakeup);
#if defined(CONFIG_ADC_ONESHOT)
	/* Set oneshot mode */
	max77804k_update_reg(max77804k->muic, MAX77804K_MUIC_REG_CTRL4,
			ADC_ONESHOT<<CTRL4_ADCMODE_SHIFT, CTRL4_ADCMODE_MASK);
#else
	/* Set continuous mode */
	max77804k_update_reg(max77804k->muic, MAX77804K_MUIC_REG_CTRL4,
			ADC_ALWAYS<<CTRL4_ADCMODE_SHIFT, CTRL4_ADCMODE_MASK);
#endif
	return ret;

err_mfd:
	mfd_remove_devices(max77804k->dev);
	max77804k_irq_exit(max77804k);
err_irq_init:
	i2c_unregister_device(max77804k->muic);
	i2c_unregister_device(max77804k->haptic);
err:
	kfree(max77804k);
	return ret;
}

static int max77804k_i2c_remove(struct i2c_client *i2c)
{
	struct max77804k_dev *max77804k = i2c_get_clientdata(i2c);

	mfd_remove_devices(max77804k->dev);
	i2c_unregister_device(max77804k->muic);
	i2c_unregister_device(max77804k->haptic);
	kfree(max77804k);

	return 0;
}

static const struct i2c_device_id max77804k_i2c_id[] = {
	{ "max77804k", TYPE_MAX77804K },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max77804k_i2c_id);
static struct of_device_id max77804k_i2c_match_table[] = {
	{ .compatible = "max77804k,i2c", },
	{ },
};
MODULE_DEVICE_TABLE(of, max77804k_i2c_match_table);

#ifdef CONFIG_PM
static int max77804k_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77804k_dev *max77804k = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		enable_irq_wake(max77804k->irq);

	return 0;
}

static int max77804k_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77804k_dev *max77804k = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		disable_irq_wake(max77804k->irq);

	return max77804k_irq_resume(max77804k);
}
#else
#define max77804k_suspend	NULL
#define max77804k_resume		NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_HIBERNATION

u8 max77804k_dumpaddr_pmic[] = {
	MAX77804K_LED_REG_IFLASH1,
	MAX77804K_LED_REG_IFLASH2,
	MAX77804K_LED_REG_ITORCH,
	MAX77804K_LED_REG_ITORCHTORCHTIMER,
	MAX77804K_LED_REG_FLASH_TIMER,
	MAX77804K_LED_REG_FLASH_EN,
	MAX77804K_LED_REG_MAX_FLASH1,
	MAX77804K_LED_REG_MAX_FLASH2,
	MAX77804K_LED_REG_VOUT_CNTL,
	MAX77804K_LED_REG_VOUT_FLASH1,
	MAX77804K_LED_REG_FLASH_INT_STATUS,

	MAX77804K_PMIC_REG_TOPSYS_INT_MASK,
	MAX77804K_PMIC_REG_MAINCTRL1,
	MAX77804K_PMIC_REG_LSCNFG,
	MAX77804K_CHG_REG_CHG_INT_MASK,
	MAX77804K_CHG_REG_CHG_CNFG_00,
	MAX77804K_CHG_REG_CHG_CNFG_01,
	MAX77804K_CHG_REG_CHG_CNFG_02,
	MAX77804K_CHG_REG_CHG_CNFG_03,
	MAX77804K_CHG_REG_CHG_CNFG_04,
	MAX77804K_CHG_REG_CHG_CNFG_05,
	MAX77804K_CHG_REG_CHG_CNFG_06,
	MAX77804K_CHG_REG_CHG_CNFG_07,
	MAX77804K_CHG_REG_CHG_CNFG_08,
	MAX77804K_CHG_REG_CHG_CNFG_09,
	MAX77804K_CHG_REG_CHG_CNFG_10,
	MAX77804K_CHG_REG_CHG_CNFG_11,
	MAX77804K_CHG_REG_CHG_CNFG_12,
	MAX77804K_CHG_REG_CHG_CNFG_13,
	MAX77804K_CHG_REG_CHG_CNFG_14,
	MAX77804K_CHG_REG_SAFEOUT_CTRL,
};

u8 max77804k_dumpaddr_muic[] = {
	MAX77804K_MUIC_REG_INTMASK1,
	MAX77804K_MUIC_REG_INTMASK2,
	MAX77804K_MUIC_REG_INTMASK3,
	MAX77804K_MUIC_REG_CDETCTRL1,
	MAX77804K_MUIC_REG_CDETCTRL2,
	MAX77804K_MUIC_REG_CTRL1,
	MAX77804K_MUIC_REG_CTRL2,
	MAX77804K_MUIC_REG_CTRL3,
};


u8 max77804k_dumpaddr_haptic[] = {
	MAX77804K_HAPTIC_REG_CONFIG1,
	MAX77804K_HAPTIC_REG_CONFIG2,
	MAX77804K_HAPTIC_REG_CONFIG_CHNL,
	MAX77804K_HAPTIC_REG_CONFG_CYC1,
	MAX77804K_HAPTIC_REG_CONFG_CYC2,
	MAX77804K_HAPTIC_REG_CONFIG_PER1,
	MAX77804K_HAPTIC_REG_CONFIG_PER2,
	MAX77804K_HAPTIC_REG_CONFIG_PER3,
	MAX77804K_HAPTIC_REG_CONFIG_PER4,
	MAX77804K_HAPTIC_REG_CONFIG_DUTY1,
	MAX77804K_HAPTIC_REG_CONFIG_DUTY2,
	MAX77804K_HAPTIC_REG_CONFIG_PWM1,
	MAX77804K_HAPTIC_REG_CONFIG_PWM2,
	MAX77804K_HAPTIC_REG_CONFIG_PWM3,
	MAX77804K_HAPTIC_REG_CONFIG_PWM4,
};


static int max77804k_freeze(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77804k_dev *max77804k = i2c_get_clientdata(i2c);
	int i;

	for (i = 0; i < ARRAY_SIZE(max77804k_dumpaddr_pmic); i++)
		max77804k_read_reg(i2c, max77804k_dumpaddr_pmic[i],
				&max77804k->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77804k_dumpaddr_muic); i++)
		max77804k_read_reg(i2c, max77804k_dumpaddr_muic[i],
				&max77804k->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77804k_dumpaddr_haptic); i++)
		max77804k_read_reg(i2c, max77804k_dumpaddr_haptic[i],
				&max77804k->reg_haptic_dump[i]);

	disable_irq(max77804k->irq);

	return 0;
}

static int max77804k_restore(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77804k_dev *max77804k = i2c_get_clientdata(i2c);
	int i;

	enable_irq(max77804k->irq);

	for (i = 0; i < ARRAY_SIZE(max77804k_dumpaddr_pmic); i++)
		max77804k_write_reg(i2c, max77804k_dumpaddr_pmic[i],
				max77804k->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77804k_dumpaddr_muic); i++)
		max77804k_write_reg(i2c, max77804k_dumpaddr_muic[i],
				max77804k->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77804k_dumpaddr_haptic); i++)
		max77804k_write_reg(i2c, max77804k_dumpaddr_haptic[i],
				max77804k->reg_haptic_dump[i]);


	return 0;
}
#endif


const struct dev_pm_ops max77804k_pm = {
	.suspend = max77804k_suspend,
	.resume = max77804k_resume,
#ifdef CONFIG_HIBERNATION
	.freeze =  max77804k_freeze,
	.thaw = max77804k_restore,
	.restore = max77804k_restore,
#endif
};

static struct i2c_driver max77804k_i2c_driver = {
	.driver = {
		.name = "max77804k",
		.owner = THIS_MODULE,
		.pm = &max77804k_pm,
		.of_match_table = max77804k_i2c_match_table,
	},
	.probe = max77804k_i2c_probe,
	.remove = max77804k_i2c_remove,
	.id_table = max77804k_i2c_id,
};


static int __init max77804k_i2c_init(void)
{
	return i2c_add_driver(&max77804k_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(max77804k_i2c_init);

static void __exit max77804k_i2c_exit(void)
{
	i2c_del_driver(&max77804k_i2c_driver);
}
module_exit(max77804k_i2c_exit);

MODULE_DESCRIPTION("MAXIM 77804k multi-function core driver");
MODULE_AUTHOR("SangYoung, Son <hello.son@samsung.com>");
MODULE_LICENSE("GPL");
