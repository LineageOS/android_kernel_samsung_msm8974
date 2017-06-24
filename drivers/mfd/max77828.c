/*
 * max77828.c - mfd core driver for the Maxim 77828
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
#include <linux/mfd/max77828.h>
#include <linux/mfd/max77828-private.h>
#include <linux/regulator/machine.h>

#include <linux/mfd/pm8xxx/misc.h>
#if defined (CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif
#define I2C_ADDR_PMIC	(0x92 >> 1)	/* Top sys, Haptic */
#define I2C_ADDR_MUIC	(0x4A >> 1)
#define I2C_ADDR_LED	(0x94 >> 1)

static struct mfd_cell max77828_devs[] = {
	{ .name = "max77828-muic", },
	{ .name = "max77828-haptic", },
	{ .name = "max77828-led",},
};

int max77828_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct max77828_dev *max77828 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77828->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&max77828->iolock);
	if (ret < 0) {
		dev_err(max77828->dev, "%s, reg(0x%x), ret(%d)\n", __func__, reg, ret);
		return ret;
	}

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL_GPL(max77828_read_reg);

int max77828_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77828_dev *max77828 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77828->iolock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max77828->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(max77828_bulk_read);

int max77828_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct max77828_dev *max77828 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77828->iolock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&max77828->iolock);
	if (ret < 0)
		dev_err(max77828->dev,
			"%s, reg(0x%x), ret(%d)\n", __func__, reg, ret);
	return ret;
}
EXPORT_SYMBOL_GPL(max77828_write_reg);

int max77828_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77828_dev *max77828 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77828->iolock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max77828->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(max77828_bulk_write);

int max77828_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct max77828_dev *max77828 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77828->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		u8 old_val = ret & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&max77828->iolock);
	return ret;
}
EXPORT_SYMBOL_GPL(max77828_update_reg);

static int of_max77828_dt(struct device *dev, struct max77828_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int retval = 0;
	struct max77828_haptic_platform_data  *haptic_data;
	
	haptic_data = kzalloc(sizeof(struct max77828_haptic_platform_data), GFP_KERNEL);
	if (haptic_data == NULL)
		return -ENOMEM;
	if(!np)
		return -EINVAL;

	pdata->irq_gpio = of_get_named_gpio_flags(np, "max77828,irq-gpio", 
				0, &pdata->irq_gpio_flags);
	of_property_read_u32(np, "max77828,irq-base", &pdata->irq_base);
	pdata->wakeup = of_property_read_bool(np, "max77828,wakeup");
	retval = of_get_named_gpio(np, "max77828,wc-irq-gpio", 0);
	if (retval < 0)
		pdata->wc_irq_gpio = 0;
	else
		pdata->wc_irq_gpio = retval;
	
	pr_info("%s: irq-gpio: %u \n", __func__, pdata->irq_gpio);
	pr_info("%s: irq-base: %u \n", __func__, pdata->irq_base);
	pr_info("%s: wc-irq-gpio: %u \n", __func__, pdata->wc_irq_gpio);

#ifdef CONFIG_SS_VIBRATOR
	if (!of_property_read_u32(np, "haptic,mode", &haptic_data->mode))
		haptic_data->mode = 0;

	if (!of_property_read_u32(np, "haptic,divisor", &haptic_data->divisor))
		haptic_data->divisor = 128;

	pr_info("%s: mode: %u \n", __func__, haptic_data->mode);
	pr_info("%s: divisor: %u \n", __func__, haptic_data->divisor);

	pdata->haptic_data = haptic_data;
#endif

	return 0;
}

static int max77828_i2c_probe(struct i2c_client *i2c,
			      const struct i2c_device_id *id)
{
	struct max77828_dev *max77828;
	struct max77828_platform_data *pdata = i2c->dev.platform_data;

	u8 reg_data;
	int ret = 0;

	dev_info(&i2c->dev, "%s\n", __func__);

	max77828 = kzalloc(sizeof(struct max77828_dev), GFP_KERNEL);
	if (!max77828) {
		dev_err(&i2c->dev, "%s: Failed to alloc mem for max77828\n", __func__);
		return -ENOMEM;
	}

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev,	sizeof(struct max77828_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&i2c->dev, "Failed to allocate memory \n");
			ret = -ENOMEM;
			goto err;
		}

		ret = of_max77828_dt(&i2c->dev, pdata);
		if (ret < 0){
			dev_err(&i2c->dev, "Failed to get device of_node \n");
			return ret;
		}

		/*pdata update to other modules*/
		pdata->muic_data = &max77828_muic;
#ifdef CONFIG_LEDS_MAX77828
        pdata->led_data = &max77828_led_pdata;
#endif
		i2c->dev.platform_data = pdata;
	} else
		pdata = i2c->dev.platform_data;

	max77828->dev = &i2c->dev;
	max77828->i2c = i2c;
	max77828->irq = i2c->irq;
	if (pdata) {
		max77828->pdata = pdata;
		max77828->irq_base = pdata->irq_base;
		max77828->irq_gpio = pdata->irq_gpio;
		max77828->wakeup = pdata->wakeup;

		gpio_tlmm_config(GPIO_CFG(max77828->irq_gpio,  0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
	} else {
		ret = -EINVAL;
		goto err;
	}

	mutex_init(&max77828->iolock);

	i2c_set_clientdata(i2c, max77828);

	if (max77828_read_reg(i2c, MAX77828_PMIC_REG_PMICREV, &reg_data) < 0) {
		dev_err(max77828->dev,
			"device not found on this channel (this is not an error)\n");
		ret = -ENODEV;
		goto err;
	} else {
		/* print rev */
		max77828->pmic_rev = (reg_data & 0x7);
		max77828->pmic_ver = ((reg_data & 0xF8) >> 0x3);
		pr_info("%s: device found: rev.0x%x, ver.0x%x\n", __func__,
				max77828->pmic_rev, max77828->pmic_ver);
	}

	max77828->muic = i2c_new_dummy(i2c->adapter, I2C_ADDR_MUIC);
	i2c_set_clientdata(max77828->muic, max77828);

	max77828->led = i2c_new_dummy(i2c->adapter, I2C_ADDR_LED);
	i2c_set_clientdata(max77828->led, max77828);

	ret = max77828_irq_init(max77828);
	if (ret < 0)
		goto err_irq_init;

	ret = mfd_add_devices(max77828->dev, -1, max77828_devs,
			ARRAY_SIZE(max77828_devs), NULL, 0);
	if (ret < 0)
		goto err_mfd;

	device_init_wakeup(max77828->dev, pdata->wakeup);
	return ret;

err_mfd:
	mfd_remove_devices(max77828->dev);
	max77828_irq_exit(max77828);
err_irq_init:
	i2c_unregister_device(max77828->muic);
	i2c_unregister_device(max77828->led);
err:
	kfree(max77828);
	return ret;
}

static int max77828_i2c_remove(struct i2c_client *i2c)
{
	struct max77828_dev *max77828 = i2c_get_clientdata(i2c);

	mfd_remove_devices(max77828->dev);
	i2c_unregister_device(max77828->muic);
	i2c_unregister_device(max77828->led);
	kfree(max77828);

	return 0;
}

static const struct i2c_device_id max77828_i2c_id[] = {
	{ "max77828", TYPE_MAX77828 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max77828_i2c_id);
static struct of_device_id max77828_i2c_match_table[] = {
	{ .compatible = "max77828,i2c", },
	{ },
};
MODULE_DEVICE_TABLE(of, max77828_i2c_match_table);

#ifdef CONFIG_PM
static int max77828_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77828_dev *max77828 = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		enable_irq_wake(max77828->irq);

	return 0;
}

static int max77828_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77828_dev *max77828 = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		disable_irq_wake(max77828->irq);

	return max77828_irq_resume(max77828);
}
#else
#define max77828_suspend	NULL
#define max77828_resume		NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_HIBERNATION
#if 0
u8 max77828_dumpaddr_pmic[] = {
	MAX77828_LED_REG_IFLASH1,
	MAX77828_LED_REG_IFLASH2,
	MAX77828_LED_REG_ITORCH,
	MAX77828_LED_REG_ITORCHTORCHTIMER,
	MAX77828_LED_REG_FLASH_TIMER,
	MAX77828_LED_REG_FLASH_EN,
	MAX77828_LED_REG_MAX_FLASH1,
	MAX77828_LED_REG_MAX_FLASH2,
	MAX77828_LED_REG_VOUT_CNTL,
	MAX77828_LED_REG_VOUT_FLASH1,
	MAX77828_LED_REG_FLASH_INT_STATUS,

	MAX77828_PMIC_REG_TOPSYS_INT_MASK,
	MAX77828_PMIC_REG_MAINCTRL1,
	MAX77828_PMIC_REG_LSCNFG,
};
#endif
u8 max77828_dumpaddr_muic[] = {
	MAX77828_MUIC_REG_INTMASK1,
	MAX77828_MUIC_REG_INTMASK2,
	MAX77828_MUIC_REG_INTMASK3,
	MAX77828_MUIC_REG_CDETCTRL1,
	MAX77828_MUIC_REG_CDETCTRL2,
	MAX77828_MUIC_REG_CTRL1,
	MAX77828_MUIC_REG_CTRL2,
	MAX77828_MUIC_REG_CTRL3,
	MAX77828_MUIC_REG_CTRL4,
};

#if 0
u8 max77828_dumpaddr_haptic[] = {
	MAX77828_HAPTIC_REG_CONFIG1,
	MAX77828_HAPTIC_REG_CONFIG2,
	MAX77828_HAPTIC_REG_CONFIG_CHNL,
	MAX77828_HAPTIC_REG_CONFG_CYC1,
	MAX77828_HAPTIC_REG_CONFG_CYC2,
	MAX77828_HAPTIC_REG_CONFIG_PER1,
	MAX77828_HAPTIC_REG_CONFIG_PER2,
	MAX77828_HAPTIC_REG_CONFIG_PER3,
	MAX77828_HAPTIC_REG_CONFIG_PER4,
	MAX77828_HAPTIC_REG_CONFIG_DUTY1,
	MAX77828_HAPTIC_REG_CONFIG_DUTY2,
	MAX77828_HAPTIC_REG_CONFIG_PWM1,
	MAX77828_HAPTIC_REG_CONFIG_PWM2,
	MAX77828_HAPTIC_REG_CONFIG_PWM3,
	MAX77828_HAPTIC_REG_CONFIG_PWM4,
};
#endif

u8 max77828_dumpaddr_led[] = {
	MAX77828_RGBLED_REG_LEDEN,
	MAX77828_RGBLED_REG_LED0BRT,
	MAX77828_RGBLED_REG_LED1BRT,
	MAX77828_RGBLED_REG_LED2BRT,
	MAX77828_RGBLED_REG_LED3BRT,
	MAX77828_RGBLED_REG_LEDBLNK,
	MAX77828_RGBLED_REG_LEDRMP,
};

static int max77828_freeze(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77828_dev *max77828 = i2c_get_clientdata(i2c);
	int i;

	for (i = 0; i < ARRAY_SIZE(max77828_dumpaddr_pmic); i++)
		max77828_read_reg(i2c, max77828_dumpaddr_pmic[i],
				&max77828->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77828_dumpaddr_muic); i++)
		max77828_read_reg(i2c, max77828_dumpaddr_muic[i],
				&max77828->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77828_dumpaddr_led); i++)
		max77828_read_reg(i2c, max77828_dumpaddr_led[i],
				&max77828->reg_led_dump[i]);

	disable_irq(max77828->irq);

	return 0;
}

static int max77828_restore(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77828_dev *max77828 = i2c_get_clientdata(i2c);
	int i;

	enable_irq(max77828->irq);

	for (i = 0; i < ARRAY_SIZE(max77828_dumpaddr_pmic); i++)
		max77828_write_reg(i2c, max77828_dumpaddr_pmic[i],
				max77828->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77828_dumpaddr_muic); i++)
		max77828_write_reg(i2c, max77828_dumpaddr_muic[i],
				max77828->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77828_dumpaddr_led); i++)
		max77828_write_reg(i2c, max77828_dumpaddr_led[i],
				max77828->reg_led_dump[i]);


	return 0;
}
#endif


const struct dev_pm_ops max77828_pm = {
	.suspend = max77828_suspend,
	.resume = max77828_resume,
#ifdef CONFIG_HIBERNATION
	.freeze =  max77828_freeze,
	.thaw = max77828_restore,
	.restore = max77828_restore,
#endif
};

static struct i2c_driver max77828_i2c_driver = {
	.driver = {
		.name = "max77828",
		.owner = THIS_MODULE,
		.pm = &max77828_pm,
		.of_match_table = max77828_i2c_match_table,
	},
	.probe = max77828_i2c_probe,
	.remove = max77828_i2c_remove,
	.id_table = max77828_i2c_id,
};

static int __init max77828_i2c_init(void)
{
	return i2c_add_driver(&max77828_i2c_driver);
}
/* init early so consumer devices can complete system boot */
subsys_initcall(max77828_i2c_init);

static void __exit max77828_i2c_exit(void)
{
	i2c_del_driver(&max77828_i2c_driver);
}
module_exit(max77828_i2c_exit);

MODULE_DESCRIPTION("MAXIM 77828 multi-function core driver");
MODULE_AUTHOR("SangYoung, Son <hello.son@samsung.com>");
MODULE_LICENSE("GPL");
