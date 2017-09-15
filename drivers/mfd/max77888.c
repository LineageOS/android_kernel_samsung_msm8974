/*
 * max77888.c - mfd core driver for the Maxim 77888
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

#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/mfd/core.h>
#include <linux/mfd/max77888.h>
#include <linux/mfd/max77888-private.h>
#include <linux/regulator/machine.h>
#include <linux/delay.h>
#if defined (CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif

#define I2C_ADDR_PMIC	(0xCC >> 1)	/* Charger, Flash LED */
#define I2C_ADDR_MUIC	(0x4A >> 1)
#define I2C_ADDR_HAPTIC	(0x90 >> 1)
#define I2C_ADDR_TEST	(0xCE >> 1)	/* TEST register */

#ifdef CONFIG_MUIC_RESET_PIN_ENABLE
int muic_reset_pin = 0;
EXPORT_SYMBOL_GPL(muic_reset_pin);
#endif

static struct mfd_cell max77888_devs[] = {
	{ .name = "max77888-charger", },
	{ .name = "max77888-led", },
	{ .name = "max77888-muic", },
	{ .name = "max77888-safeout", },
	{ .name = "max77888-haptic", },
};

int max77888_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest)
{
	struct max77888_dev *max77888 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77888->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	mutex_unlock(&max77888->iolock);
	if (ret < 0)
		return ret;

	ret &= 0xff;
	*dest = ret;
	return 0;
}
EXPORT_SYMBOL_GPL(max77888_read_reg);

int max77888_bulk_read(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77888_dev *max77888 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77888->iolock);
	ret = i2c_smbus_read_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max77888->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(max77888_bulk_read);

int max77888_write_reg(struct i2c_client *i2c, u8 reg, u8 value)
{
	struct max77888_dev *max77888 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77888->iolock);
	ret = i2c_smbus_write_byte_data(i2c, reg, value);
	mutex_unlock(&max77888->iolock);
	return ret;
}
EXPORT_SYMBOL_GPL(max77888_write_reg);

int max77888_bulk_write(struct i2c_client *i2c, u8 reg, int count, u8 *buf)
{
	struct max77888_dev *max77888 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77888->iolock);
	ret = i2c_smbus_write_i2c_block_data(i2c, reg, count, buf);
	mutex_unlock(&max77888->iolock);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(max77888_bulk_write);

static int max77888_read_word(struct i2c_client *i2c, u8 reg)
{
	struct max77888_dev *max77888 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77888->iolock);
	ret = i2c_smbus_read_word_data(i2c, reg);
	mutex_unlock(&max77888->iolock);

	return ret;
}

int max77888_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask)
{
	struct max77888_dev *max77888 = i2c_get_clientdata(i2c);
	int ret;

	mutex_lock(&max77888->iolock);
	ret = i2c_smbus_read_byte_data(i2c, reg);
	if (ret >= 0) {
		u8 old_val = ret & 0xff;
		u8 new_val = (val & mask) | (old_val & (~mask));
		ret = i2c_smbus_write_byte_data(i2c, reg, new_val);
	}
	mutex_unlock(&max77888->iolock);
	return ret;
}
EXPORT_SYMBOL_GPL(max77888_update_reg);

static int of_max77888_dt(struct device *dev, struct max77888_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int retval = 0;
#ifdef CONFIG_SS_VIBRATOR
	struct max77888_haptic_platform_data  *haptic_data;
	haptic_data = kzalloc(sizeof(struct max77888_haptic_platform_data), GFP_KERNEL);
	if (haptic_data == NULL)
		return -ENOMEM;
#endif
	if(!np) {
#ifdef CONFIG_SS_VIBRATOR
	kfree(haptic_data);
#endif
		return -EINVAL;
	}

	pdata->irq_gpio = of_get_named_gpio(np, "max77888,irq-gpio", 0);
	if (pdata->irq_gpio < 0) {
		pr_err("%s: failed get max77888 irq-gpio : %d\n",
			__func__, pdata->irq_gpio);
		pdata->irq_gpio = 0;
	}

#ifdef CONFIG_MUIC_RESET_PIN_ENABLE
	pdata->irq_reset_gpio = of_get_named_gpio(np, "max77888,irq-reset-gpio", 0);
	if (pdata->irq_reset_gpio < 0) {
		pr_err("%s: failed get max77888 irq-reset-gpio : %d\n",
			__func__, pdata->irq_gpio);
		pdata->irq_reset_gpio = -1;
		muic_reset_pin = 0;
	}
	else
		muic_reset_pin = 1;
	
#endif

	retval = of_property_read_u32(np, "max77888,irq-base", &pdata->irq_base);
	pdata->wakeup = of_property_read_bool(np, "max77888,wakeup");

	pr_info("%s: irq-gpio: %u \n", __func__, pdata->irq_gpio);
#ifdef CONFIG_MUIC_RESET_PIN_ENABLE
	pr_info("%s: irq-reset-gpio: %u \n", __func__, pdata->irq_reset_gpio);
#endif
	pr_info("%s: irq-gpio_flags: %u \n", __func__, pdata->irq_gpio_flags);
	pr_info("%s: irq-base: %u \n", __func__, pdata->irq_base);

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

static int max77888_i2c_probe(struct i2c_client *i2c,
			      const struct i2c_device_id *id)
{
	struct max77888_dev *max77888;
	struct max77888_platform_data *pdata; 
	u8 reg_data;
	u16 reg_data16;
	u8 str_data[10] = {0,};
	int i;
	int ret = 0;

	msleep(500);

	max77888 = kzalloc(sizeof(struct max77888_dev), GFP_KERNEL);
	if (max77888 == NULL)
		return -ENOMEM;

	if (i2c->dev.of_node) {
		pdata = devm_kzalloc(&i2c->dev,
				sizeof(struct max77888_platform_data),
				GFP_KERNEL);

		if (!pdata) {
			dev_err(&i2c->dev, "Failed to allocate memory \n");
			ret = -ENOMEM;
			goto err;
		}

		ret = of_max77888_dt(&i2c->dev, pdata);
		if (ret < 0){
			dev_err(&i2c->dev, "Failed to get device of_node \n");
			return ret;
		}

		/*Filling the platform data*/
		pdata->muic = &max77888_muic;
#if defined(CONFIG_REGULATOR_MAX77888)
		pdata->num_regulators = MAX77888_REG_MAX;
		pdata->regulators = max77888_regulators,
#endif
#ifdef CONFIG_LEDS_MAX77888
		pdata->led_data = &max77888_led_pdata;
#endif
		/*pdata update to other modules*/
		i2c->dev.platform_data = pdata;
	} else {
		pdata = i2c->dev.platform_data;
	}

	i2c_set_clientdata(i2c, max77888);
	max77888->dev = &i2c->dev;
	max77888->i2c = i2c;
	max77888->irq = i2c->irq;
	if (pdata) {
		max77888->irq_base = pdata->irq_base;
		max77888->irq_gpio = pdata->irq_gpio;
#ifdef CONFIG_MUIC_RESET_PIN_ENABLE
		if (muic_reset_pin)
		{
			max77888->irq_reset_gpio = pdata->irq_reset_gpio;
			gpio_tlmm_config(GPIO_CFG(max77888->irq_reset_gpio,  0, GPIO_CFG_INPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
		}
#endif
		max77888->wakeup = pdata->wakeup;

		gpio_tlmm_config(GPIO_CFG(max77888->irq_gpio,  0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);

	} else {
		goto err;
	}

	mutex_init(&max77888->iolock);

	if (max77888_read_reg(i2c, MAX77888_PMIC_REG_PMIC_ID2, &reg_data) < 0) {
		dev_err(max77888->dev,
			"device not found on this channel (this is not an error)\n");
		ret = -ENODEV;
		goto err;
	} else {
		/* print rev */
		max77888->pmic_rev = (reg_data & 0x7);
		max77888->pmic_ver = ((reg_data & 0xF8) >> 0x3);
		pr_info("%s: device found: rev.0x%x, ver.0x%x\n", __func__,
				max77888->pmic_rev, max77888->pmic_ver);
	}

	/* No active discharge on safeout ldo 1,2 */
	max77888_update_reg(i2c, MAX77888_CHG_REG_SAFEOUT_CTRL, 0x00, 0x30);
	pr_info("%s: i2c->name=%s irq=%d   !!!\n",__func__, i2c->name, i2c->irq);

	max77888->muic = i2c_new_dummy(i2c->adapter, I2C_ADDR_MUIC);
	i2c_set_clientdata(max77888->muic, max77888);

	max77888->haptic = i2c_new_dummy(i2c->adapter, I2C_ADDR_HAPTIC);
	i2c_set_clientdata(max77888->haptic, max77888);

	// Set TEST Reigster Slave address
	max77888->test = i2c_new_dummy(i2c->adapter, I2C_ADDR_TEST);
	i2c_set_clientdata(max77888->test, max77888);

	// Start Over-write wrong-Trimmed bit //

	// 1. Test Register Access Enabled
	max77888_write_reg(max77888->i2c, 0xFE, 0xC5);
	// 2. Enable TST_KEY
	max77888_write_reg(max77888->test, 0xB3, 0x0C);
	// 3. Read 0x2E with word unit.
	reg_data16 = max77888_read_word(max77888->test, 0x2E);
	// 4. Check Bit5 of First bit(Bit13)
	if ((reg_data16 & 0x2000) == 0) {
		// Wrong Trimmed
		// 5. Read and Store
		// 5-1. Read and Store from 0x21 to 0x2A
		for (i = 0x21; i <= 0x2A; i++) {
			if (i == 0x25) {
				continue;
			}
			reg_data16 = max77888_read_word(max77888->test, i);
			str_data[i-0x21] = (reg_data16 >> 8);
		}
		// 5-2. Read and Store 0x2E
		reg_data16 = max77888_read_word(max77888->test, i);
		reg_data = (reg_data16 >> 8);

		// 6. Write Stored data from 0x21 to 0x2A.
		for (i = 0x21; i <= 0x2A; i++) {
			if ( i == 0x25) {
				continue;
			}
			max77888_write_reg(max77888->test, i, str_data[i-0x21]);
		}
		// 7. Write 0x2E
		max77888_write_reg(max77888->test, 0x2E, (reg_data | 0x20));

		// 8. Write 0x20 to 0x40.
		max77888_write_reg(max77888->test, 0x20, 0x40);
	}

	// 9. Disable TST_KEY, Write 0xB3 to 0x00
	max77888_write_reg(max77888->test, 0xB3, 0x00);

	// 10. Test Register Access Disabled, Write 0xFE to 0x00
	max77888_write_reg(max77888->i2c, 0xFE, 0x00);

	ret = max77888_irq_init(max77888);
	if (ret < 0)
		goto err_irq_init;

	ret = mfd_add_devices(max77888->dev, -1, max77888_devs,
			ARRAY_SIZE(max77888_devs), NULL, 0);
	if (ret < 0)
		goto err_mfd;

	device_init_wakeup(max77888->dev, pdata->wakeup);

	pr_info("%s:%d:",  __func__, __LINE__ );
	return ret;

err_mfd:
	mfd_remove_devices(max77888->dev);
	max77888_irq_exit(max77888);
err_irq_init:
	i2c_unregister_device(max77888->muic);
	i2c_unregister_device(max77888->haptic);
err:
	pr_info("%s:%d:",  __func__, __LINE__ );
	kfree(max77888);
	return ret;
}

static int max77888_i2c_remove(struct i2c_client *i2c)
{
	struct max77888_dev *max77888 = i2c_get_clientdata(i2c);

	mfd_remove_devices(max77888->dev);
	i2c_unregister_device(max77888->muic);
	i2c_unregister_device(max77888->haptic);
	kfree(max77888);

	return 0;
}

static const struct i2c_device_id max77888_i2c_id[] = {
	{ "max77888", TYPE_MAX77888 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max77888_i2c_id);
static struct of_device_id max77888_i2c_match_table[] = {
    { .compatible = "max77888,i2c", },
    { },
};
MODULE_DEVICE_TABLE(of, max77888_i2c_match_table);


#ifdef CONFIG_PM
static int max77888_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77888_dev *max77888 = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		enable_irq_wake(max77888->irq);

	return 0;
}

static int max77888_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77888_dev *max77888 = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		disable_irq_wake(max77888->irq);

	return max77888_irq_resume(max77888);
}
#else
#define max77888_suspend	NULL
#define max77888_resume		NULL
#endif /* CONFIG_PM */

#ifdef CONFIG_HIBERNATION

u8 max77888_dumpaddr_pmic[] = {
#ifdef CONFIG_MFD_MAX77888
	MAX77888_LED_REG_IFLASH,
#else
	MAX77888_LED_REG_IFLASH1,
	MAX77888_LED_REG_IFLASH2,
#endif
	MAX77888_LED_REG_ITORCH,
	MAX77888_LED_REG_ITORCHTORCHTIMER,
	MAX77888_LED_REG_FLASH_TIMER,
	MAX77888_LED_REG_FLASH_EN,
	MAX77888_LED_REG_MAX_FLASH1,
	MAX77888_LED_REG_MAX_FLASH2,
	MAX77888_LED_REG_VOUT_CNTL,
#ifdef CONFIG_MFD_MAX77888
	MAX77888_LED_REG_VOUT_FLASH,
#else
	MAX77888_LED_REG_VOUT_FLASH1,
#endif
	MAX77888_LED_REG_FLASH_INT_STATUS,

	MAX77888_PMIC_REG_TOPSYS_INT_MASK,
	MAX77888_PMIC_REG_MAINCTRL1,
	MAX77888_PMIC_REG_LSCNFG,
	MAX77888_CHG_REG_CHG_INT_MASK,
	MAX77888_CHG_REG_CHG_CNFG_00,
	MAX77888_CHG_REG_CHG_CNFG_01,
	MAX77888_CHG_REG_CHG_CNFG_02,
	MAX77888_CHG_REG_CHG_CNFG_03,
	MAX77888_CHG_REG_CHG_CNFG_04,
	MAX77888_CHG_REG_CHG_CNFG_05,
	MAX77888_CHG_REG_CHG_CNFG_06,
	MAX77888_CHG_REG_CHG_CNFG_07,
	MAX77888_CHG_REG_CHG_CNFG_08,
	MAX77888_CHG_REG_CHG_CNFG_09,
	MAX77888_CHG_REG_CHG_CNFG_10,
	MAX77888_CHG_REG_CHG_CNFG_11,
	MAX77888_CHG_REG_CHG_CNFG_12,
	MAX77888_CHG_REG_CHG_CNFG_13,
	MAX77888_CHG_REG_CHG_CNFG_14,
	MAX77888_CHG_REG_SAFEOUT_CTRL,
};

u8 max77888_dumpaddr_muic[] = {
	MAX77888_MUIC_REG_INTMASK1,
	MAX77888_MUIC_REG_INTMASK2,
	MAX77888_MUIC_REG_INTMASK3,
	MAX77888_MUIC_REG_CDETCTRL1,
	MAX77888_MUIC_REG_CDETCTRL2,
	MAX77888_MUIC_REG_CTRL1,
	MAX77888_MUIC_REG_CTRL2,
	MAX77888_MUIC_REG_CTRL3,
};


u8 max77888_dumpaddr_haptic[] = {
	MAX77888_HAPTIC_REG_CONFIG1,
	MAX77888_HAPTIC_REG_CONFIG2,
	MAX77888_HAPTIC_REG_CONFIG_CHNL,
	MAX77888_HAPTIC_REG_CONFG_CYC1,
	MAX77888_HAPTIC_REG_CONFG_CYC2,
	MAX77888_HAPTIC_REG_CONFIG_PER1,
	MAX77888_HAPTIC_REG_CONFIG_PER2,
	MAX77888_HAPTIC_REG_CONFIG_PER3,
	MAX77888_HAPTIC_REG_CONFIG_PER4,
	MAX77888_HAPTIC_REG_CONFIG_DUTY1,
	MAX77888_HAPTIC_REG_CONFIG_DUTY2,
	MAX77888_HAPTIC_REG_CONFIG_PWM1,
	MAX77888_HAPTIC_REG_CONFIG_PWM2,
	MAX77888_HAPTIC_REG_CONFIG_PWM3,
	MAX77888_HAPTIC_REG_CONFIG_PWM4,
};


static int max77888_freeze(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77888_dev *max77888 = i2c_get_clientdata(i2c);
	int i;

	for (i = 0; i < ARRAY_SIZE(max77888_dumpaddr_pmic); i++)
		max77888_read_reg(i2c, max77888_dumpaddr_pmic[i],
				&max77888->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77888_dumpaddr_muic); i++)
		max77888_read_reg(i2c, max77888_dumpaddr_muic[i],
				&max77888->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77888_dumpaddr_haptic); i++)
		max77888_read_reg(i2c, max77888_dumpaddr_haptic[i],
				&max77888->reg_haptic_dump[i]);

	disable_irq(max77888->irq);
#ifdef CONFIG_MUIC_RESET_PIN_ENABLE
	if (muic_reset_pin)
		disable_irq(max77888->irq_reset);
#endif
	return 0;
}

static int max77888_restore(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct max77888_dev *max77888 = i2c_get_clientdata(i2c);
	int i;

	enable_irq(max77888->irq);
#ifdef CONFIG_MUIC_RESET_PIN_ENABLE
	if (muic_reset_pin)
		enable_irq(max77888->irq_reset);
#endif
	for (i = 0; i < ARRAY_SIZE(max77888_dumpaddr_pmic); i++)
		max77888_write_reg(i2c, max77888_dumpaddr_pmic[i],
				max77888->reg_pmic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77888_dumpaddr_muic); i++)
		max77888_write_reg(i2c, max77888_dumpaddr_muic[i],
				max77888->reg_muic_dump[i]);

	for (i = 0; i < ARRAY_SIZE(max77888_dumpaddr_haptic); i++)
		max77888_write_reg(i2c, max77888_dumpaddr_haptic[i],
				max77888->reg_haptic_dump[i]);


	return 0;
}
#endif


const struct dev_pm_ops max77888_pm = {
	.suspend = max77888_suspend,
	.resume = max77888_resume,
#ifdef CONFIG_HIBERNATION
	.freeze =  max77888_freeze,
	.thaw = max77888_restore,
	.restore = max77888_restore,
#endif
};

static struct i2c_driver max77888_i2c_driver = {
	.driver = {
		.name = "max77888",
		.owner = THIS_MODULE,
		.pm = &max77888_pm,
	        .of_match_table = max77888_i2c_match_table,
	},
	.probe = max77888_i2c_probe,
	.remove = max77888_i2c_remove,
	.id_table = max77888_i2c_id,
};

static int __init max77888_i2c_init(void)
{
	pr_info("%s: START\n", __func__);
	return i2c_add_driver(&max77888_i2c_driver);
}
/* init early so consumer devices can complete system boot */
//subsys_initcall(max77888_i2c_init);
module_init(max77888_i2c_init);

static void __exit max77888_i2c_exit(void)
{
	i2c_del_driver(&max77888_i2c_driver);
}
module_exit(max77888_i2c_exit);

MODULE_DESCRIPTION("MAXIM 77888 multi-function core driver");
MODULE_AUTHOR("SangYoung, Son <hello.son@samsung.com>");
MODULE_LICENSE("GPL");
