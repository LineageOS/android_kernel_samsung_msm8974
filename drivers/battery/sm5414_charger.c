/*
 *  SM5414_charger.c
 *  SiliconMitus SM5414 Charger Driver
 *
 *  Copyright (C) 2013 SiliconMitus
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#define DEBUG

#include <linux/battery/sec_charger.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/gpio.h>
#include <linux/gpio-pxa.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

static int charger_health = POWER_SUPPLY_HEALTH_GOOD;

static int SM5414_i2c_write(struct i2c_client *client,
		int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_write_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);

	return ret;
}

static int SM5414_i2c_read(struct i2c_client *client,
		int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_read_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);

	return ret;
}

#if 0
static void SM5414_test_read(struct i2c_client *client)
{
	u8 data = 0;
	u32 addr = 0;

    //0x00~02 are R/C
	for (addr = SM5414_INTMASK1; addr <= SM5414_CHGCTRL5; addr++) {
		SM5414_i2c_read(client, addr, &data);
		dev_info(&client->dev,
			"SM5414 addr : 0x%02x data : 0x%02x\n", addr, data);
	}
}
#endif

static void SM5414_read_regs(struct i2c_client *client, char *str)
{
	u8 data = 0;
	u32 addr = 0;

	//0x00~02 are R/C (read and clear)
	for (addr = SM5414_INTMASK1; addr <= SM5414_CHGCTRL5; addr++) {
		SM5414_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}
}

static int SM5414_get_charging_status(struct i2c_client *client)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	int nCHG;
	u8 int2, chg_en;
	union power_supply_propval value;

	SM5414_i2c_read(client, SM5414_INT2, &int2);
	SM5414_i2c_read(client, SM5414_CTRL, &chg_en);

	if((int2 & SM5414_INT2_DONE) || (int2 & SM5414_INT2_TOPOFF)) {
		psy_do_property(charger->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_CAPACITY, value);
		if ((value.intval > 96) &&
			(charger->cable_type != POWER_SUPPLY_TYPE_BATTERY)) {
				status = POWER_SUPPLY_STATUS_FULL;
				charger->is_fullcharged = true;
				dev_info(&client->dev,
					"%s : Power Supply Full\n", __func__);
		}
	} else if (chg_en & CHARGE_EN) {
		nCHG = gpio_get_value(charger->pdata->chg_gpio_en);
		if ((nCHG) || (charger_health != POWER_SUPPLY_HEALTH_GOOD))
			status = POWER_SUPPLY_STATUS_DISCHARGING;
		else
			status = POWER_SUPPLY_STATUS_CHARGING;
	} else {
		status = POWER_SUPPLY_STATUS_DISCHARGING;
	}

	return (int)status;
}

static int SM5414_get_charging_health(struct i2c_client *client)
{
	static int health = POWER_SUPPLY_HEALTH_GOOD;
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 int1;

	SM5414_i2c_read(client, SM5414_INT1, &int1);

	dev_info(&client->dev,
		"%s : SM5414_INT1 : 0x%02x\n", __func__, int1);

	if (int1 & SM5414_INT1_VBUSOVP) {
		health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		charger_health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	} else if (int1 & SM5414_INT1_VBUSUVLO) {
		msleep(1000);
		if (charger->cable_type != POWER_SUPPLY_TYPE_BATTERY) {
			health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
			charger_health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		}
	} else if (int1 & SM5414_INT1_VBUSINOK) {
		health = POWER_SUPPLY_HEALTH_GOOD;
		charger_health = POWER_SUPPLY_HEALTH_GOOD;
	}

	return (int)health;
}

static u8 SM5414_set_float_voltage_data(
		struct i2c_client *client, int float_voltage)
{
	u8 data, float_reg;

	SM5414_i2c_read(client, SM5414_CHGCTRL3, &data);

	data &= BATREG_MASK;

	if (float_voltage < 4100)
		float_voltage = 4100;
	else if (float_voltage > 4475)
		float_voltage = 4475;

	float_reg = (float_voltage - 4100) / 25;
	data |= (float_reg << 4);

	SM5414_i2c_write(client, SM5414_CHGCTRL3, &data);

	SM5414_i2c_read(client, SM5414_CHGCTRL3, &data);
	dev_dbg(&client->dev,
		"%s : SM5414_CHGCTRL3 (float) : 0x%02x\n", __func__, data);

	return data;
}

static u8 SM5414_set_input_current_limit_data(
		struct i2c_client *client, int input_current)
{
	u8 set_reg, curr_reg = 0;
	u8 chg_en;

	if(input_current < 100)
		input_current = 100;
	else if (input_current >= 2050)
		input_current = 2050;

	set_reg = (input_current - 100) / 50;

	curr_reg = ((input_current % 100) >= 50) ? 1 : 0;

	SM5414_i2c_read(client, SM5414_CTRL, &chg_en);

	if (chg_en & CHARGE_EN) {
		SM5414_i2c_write(client, SM5414_VBUSCTRL, &set_reg);
	} else {
		while (set_reg >= curr_reg) {
			SM5414_i2c_write(client, SM5414_VBUSCTRL, &curr_reg);
			curr_reg += 2;
			msleep(50);
		};
	}

	SM5414_i2c_read(client, SM5414_VBUSCTRL, &curr_reg);
	dev_dbg(&client->dev,
		"%s : SM5414_VBUSCTRL (Input limit) : 0x%02x\n",
		__func__, curr_reg);

	return curr_reg;
}

static u8 SM5414_set_topoff_current_limit_data(
		struct i2c_client *client, int topoff_current)
{
	u8 data;
	u8 topoff_reg;

	SM5414_i2c_read(client, SM5414_CHGCTRL4, &data);

	data &= TOPOFF_MASK;

	if(topoff_current < 100)
		topoff_current = 100;
	else if (topoff_current > 650)
		topoff_current = 650;

	topoff_reg = (topoff_current - 100) / 50;
	data = data | (topoff_reg<<3);

	SM5414_i2c_write(client, SM5414_CHGCTRL4, &data);

	SM5414_i2c_read(client, SM5414_CHGCTRL4, &data);
	dev_dbg(&client->dev,
		"%s : SM5414_CHGCTRL4 (Top-off limit) : 0x%02x\n",
		__func__, data);

	return data;
}

static u8 SM5414_set_fast_charging_current_data(
		struct i2c_client *client, int fast_charging_current)
{
	u8 data = 0;

	if(fast_charging_current < 100)
		fast_charging_current = 100;
	else if (fast_charging_current > 2500)
		fast_charging_current = 2500;

	data = (fast_charging_current - 100) / 50;

	SM5414_i2c_write(client, SM5414_CHGCTRL2, &data);
	SM5414_i2c_read(client, SM5414_CHGCTRL2, &data);

	dev_dbg(&client->dev,
		"%s : SM5414_CHGCTRL2 (fast) : 0x%02x\n", __func__, data);

	return data;
}

static u8 SM5414_set_toggle_charger(struct i2c_client *client, int enable)
{
	u8 chg_en=0;
	u8 data=0;

	SM5414_i2c_read(client, SM5414_CTRL, &chg_en);
	if (enable)
		chg_en |= CHARGE_EN;
	else
		chg_en &= ~CHARGE_EN;

	SM5414_i2c_write(client, SM5414_CTRL, &chg_en);

	dev_info(&client->dev, "%s: SM5414 Charger toggled!! \n", __func__);

	SM5414_i2c_read(client, SM5414_CTRL, &chg_en);
	dev_info(&client->dev,
		"%s : chg_en value(07h register): 0x%02x\n", __func__, chg_en);

	SM5414_i2c_read(client, SM5414_CHGCTRL2, &data);
	dev_info(&client->dev,
		"%s : SM5414_CHGCTRL2 value: 0x%02x", __func__, data);

	return chg_en;
}

static void SM5414_charger_function_control(
				struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	union power_supply_propval value;
	u8 ctrl;
	u8 chg_en;

	charger->charging_current_max =
		charger->pdata->charging_current[
		charger->cable_type].input_current_limit;

	charger->charging_current =
		charger->pdata->charging_current[
		charger->cable_type].fast_charging_current;

	if (charger->charging_current < 0) {
		dev_info(&client->dev,
			"%s : OTG is activated. Ignore command!\n", __func__);
		return;
	}

	if (charger->cable_type == POWER_SUPPLY_TYPE_BATTERY) {
		/* Disable Charger */
		dev_info(&client->dev,
			"%s : Disable Charger, Battery Supply!\n", __func__);
		// nCHG_EN is logic low so set 1 to disable charger
		charger->is_fullcharged = false;
		gpio_direction_output((charger->pdata->chg_gpio_en), 1);
		SM5414_set_toggle_charger(client, 0);
	} else {

		psy_do_property(charger->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_CAPACITY, value);
		if (value.intval > 0) {
			/* Suspend enable for register reset */
			ctrl = 0x44;
			SM5414_i2c_write(client, SM5414_CTRL, &ctrl);
			msleep(20);

			ctrl = 0x40;
			SM5414_i2c_write(client, SM5414_CTRL, &ctrl);
		}

		dev_info(&client->dev, "%s : float voltage (%dmV)\n",
			__func__, charger->pdata->chg_float_voltage);

		/* Set float voltage */
		SM5414_set_float_voltage_data(
			client, charger->pdata->chg_float_voltage);

		dev_info(&client->dev, "%s : topoff current (%dmA)\n",
			__func__, charger->pdata->charging_current[
			charger->cable_type].full_check_current_1st);
		SM5414_set_topoff_current_limit_data(
			client, charger->pdata->charging_current[
				charger->cable_type].full_check_current_1st);

		SM5414_i2c_read(client, SM5414_CTRL, &chg_en);

		if (!(chg_en & CHARGE_EN)) {
			SM5414_set_input_current_limit_data(client, 100);
			// nCHG_EN is logic low so set 0 to enable charger
			gpio_direction_output((charger->pdata->chg_gpio_en), 0);
			SM5414_set_toggle_charger(client, 1);
			msleep(100);

			/* Input current limit */
			dev_info(&client->dev, "%s : input current (%dmA)\n",
				__func__, charger->charging_current_max);

			SM5414_set_input_current_limit_data(
				client, charger->charging_current_max);
		}

		/* Set fast charge current */
		dev_info(&client->dev, "%s : fast charging current (%dmA), siop_level=%d\n",
			__func__, charger->charging_current, charger->siop_level);
		SM5414_set_fast_charging_current_data(
			client, charger->charging_current);

		dev_info(&client->dev,
			"%s : Enable Charger!\n", __func__);
	}
}

static void SM5414_charger_otg_control(
				struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 data;
//turn on/off ENBOOST
	if (charger->cable_type ==
		POWER_SUPPLY_TYPE_BATTERY) {
		dev_info(&client->dev, "%s : turn off OTG\n", __func__);
		/* turn off OTG */
		SM5414_i2c_read(client, SM5414_CTRL, &data);
		data &= 0xfe;
		SM5414_i2c_write(client, SM5414_CTRL, &data);
	} else {
		dev_info(&client->dev, "%s : turn on OTG\n", __func__);
		/* turn on OTG */
		SM5414_i2c_read(client, SM5414_CTRL, &data);
		data |= 0x01;
		SM5414_i2c_write(client, SM5414_CTRL, &data);
	}
}

static int SM5414_debugfs_show(struct seq_file *s, void *data)
{
	struct sec_charger_info *charger = s->private;
	u8 reg;
	u8 reg_data;

	seq_printf(s, "SM CHARGER IC :\n");
	seq_printf(s, "==================\n");
	for (reg = SM5414_INTMASK1; reg <= SM5414_CHGCTRL5; reg++) {
		SM5414_i2c_read(charger->client, reg, &reg_data);
		seq_printf(s, "0x%02x:\t0x%02x\n", reg, reg_data);
	}

	seq_printf(s, "\n");
	return 0;
}

static int SM5414_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, SM5414_debugfs_show, inode->i_private);
}

static const struct file_operations SM5414_debugfs_fops = {
	.open           = SM5414_debugfs_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

bool SM5414_hal_chg_init(struct i2c_client *client)
{
	u8 reg_data;
	u8 int1 = 0;
	struct sec_charger_info *charger = i2c_get_clientdata(client);

	dev_info(&client->dev, "%s: SM5414 Charger init (Starting)!! \n", __func__);
	charger->is_fullcharged = false;

	SM5414_i2c_read(client, SM5414_INT1, &int1);
	dev_info(&client->dev,
		"%s : SM5414_INT1 : 0x%02x\n", __func__, int1);

	reg_data = 0x1F;
	SM5414_i2c_write(client, SM5414_INTMASK1, &reg_data);

	reg_data = 0xFC;
	SM5414_i2c_write(client, SM5414_INTMASK2, &reg_data);

	SM5414_i2c_read(client, SM5414_CHGCTRL1, &reg_data);
	reg_data &= ~SM5414_CHGCTRL1_AUTOSTOP;
	SM5414_i2c_write(client, SM5414_CHGCTRL1, &reg_data);

	(void) debugfs_create_file("SM5414_regs",
		S_IRUGO, NULL, (void *)charger, &SM5414_debugfs_fops);

	return true;
}

bool SM5414_hal_chg_suspend(struct i2c_client *client)
{
	dev_info(&client->dev,
                "%s: CHARGER - SM5414(suspend mode)!!\n", __func__);

	return true;
}

bool SM5414_hal_chg_resume(struct i2c_client *client)
{
	dev_info(&client->dev,
                "%s: CHARGER - SM5414(resume mode)!!\n", __func__);

	return true;
}

bool SM5414_hal_chg_get_property(struct i2c_client *client,
			      enum power_supply_property psp,
			      union power_supply_propval *val)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 data;
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		break;
	case POWER_SUPPLY_PROP_STATUS:
		if (charger->is_fullcharged)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else
			val->intval = SM5414_get_charging_status(client);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = charger_health;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
		break;
	// Have to mention the issue about POWER_SUPPLY_PROP_CHARGE_NOW to Marvel
	case POWER_SUPPLY_PROP_CHARGE_NOW:
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	case POWER_SUPPLY_PROP_CURRENT_AVG:
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (charger->charging_current) {
			SM5414_i2c_read(client, SM5414_VBUSCTRL, &data);
			data &= 0x3f;
			val->intval = (100 + (data * 50));
			if (val->intval < 2050)
				val->intval = (100 + (data * 50));
			/*dev_dbg(&client->dev,
				"%s 1: set-current(%dmA), current now(%dmA)\n",
				__func__, charger->charging_current, val->intval);*/
		} else {
			val->intval = 100;
			/*dev_dbg(&client->dev,
				"%s 2: set-current(%dmA), current now(%dmA)\n",
				__func__, charger->charging_current, val->intval);*/
		}
		break;
	default:
		return false;
	}
	return true;
}

bool SM5414_hal_chg_set_property(struct i2c_client *client,
			      enum power_supply_property psp,
			      const union power_supply_propval *val)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	switch (psp) {
	/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		if (charger->cable_type == POWER_SUPPLY_TYPE_OTG) {
			SM5414_charger_otg_control(client);
		} else if (charger->cable_type == POWER_SUPPLY_TYPE_BATTERY) {
			SM5414_charger_function_control(client);
			SM5414_charger_otg_control(client);
		} else {
			SM5414_charger_function_control(client);
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX: /* input current limit set */
		/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		SM5414_set_input_current_limit_data(client, val->intval);
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		SM5414_set_fast_charging_current_data(client, val->intval);
		break;
	default:
		return false;
	}

	return true;
}

ssize_t SM5414_hal_chg_show_attrs(struct device *dev,
				const ptrdiff_t offset, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *chg =
		container_of(psy, struct sec_charger_info, psy_chg);
	int i = 0;
	char *str = NULL;

	switch (offset) {
/*	case CHG_REG: */
/*		break; */
	case CHG_DATA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
			chg->reg_data);
		break;
	case CHG_REGS:
		str = kzalloc(sizeof(char)*1024, GFP_KERNEL);
		if (!str)
			return -ENOMEM;

		SM5414_read_regs(chg->client, str);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
			str);

		kfree(str);
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t SM5414_hal_chg_store_attrs(struct device *dev,
				const ptrdiff_t offset,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *chg =
		container_of(psy, struct sec_charger_info, psy_chg);
	int ret = 0;
	int x = 0;
	u8 data = 0;

	switch (offset) {
	case CHG_REG:
		if (sscanf(buf, "%x\n", &x) == 1) {
			chg->reg_addr = x;
			SM5414_i2c_read(chg->client,
				chg->reg_addr, &data);
			chg->reg_data = data;
			dev_dbg(dev, "%s: (read) addr = 0x%x, data = 0x%x\n",
				__func__, chg->reg_addr, chg->reg_data);
			ret = count;
		}
		break;
	case CHG_DATA:
		if (sscanf(buf, "%x\n", &x) == 1) {
			data = (u8)x;
			dev_dbg(dev, "%s: (write) addr = 0x%x, data = 0x%x\n",
				__func__, chg->reg_addr, data);
			SM5414_i2c_write(chg->client,
				chg->reg_addr, &data);
			ret = count;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static struct device_attribute SM5414_charger_attrs[] = {
	SM5414_CHARGER_ATTR(reg),
	SM5414_CHARGER_ATTR(data),
	SM5414_CHARGER_ATTR(regs),
};

static enum power_supply_property SM5414_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
};

static int SM5414_chg_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct sec_charger_info *charger =
		container_of(psy, struct sec_charger_info, psy_chg);

	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_MAX:	/* input current limit set */
		val->intval = charger->charging_current_max;
		break;

	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_HEALTH:
	case POWER_SUPPLY_PROP_CURRENT_AVG:	/* charging current */
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (!SM5414_hal_chg_get_property(charger->client, psp, val))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int SM5414_chg_set_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    const union power_supply_propval *val)
{
	struct sec_charger_info *charger =
		container_of(psy, struct sec_charger_info, psy_chg);
	union power_supply_propval input_value;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		charger->status = val->intval;
		break;

	/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		charger->cable_type = val->intval;
		if (val->intval == POWER_SUPPLY_TYPE_BATTERY || \
				val->intval == POWER_SUPPLY_TYPE_OTG) {
			charger->is_charging = false;
			charger->is_slow_charging = false;
		}
		else
			charger->is_charging = true;

		if (!SM5414_hal_chg_set_property(charger->client, psp, val))
			return -EINVAL;
		break;

	/* val->intval : input current limit set */
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		charger->charging_current_max = val->intval;
	/* to control charging current,
	 * use input current limit and set charging current as much as possible
	 * so we only control input current limit to control charge current
	 */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (!SM5414_hal_chg_set_property(charger->client, psp, val))
			return -EINVAL;
		break;

	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		charger->charging_current = val->intval;

		if (!SM5414_hal_chg_set_property(charger->client, psp, val))
			return -EINVAL;
		break;

	/* val->intval : SIOP level (%)
	 * SIOP charging current setting
	 */
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		charger->siop_level = val->intval;
		if (charger->is_charging) {
			/* change val as charging current by SIOP level
			* do NOT change initial charging current setting
			*/
			input_value.intval =
					charger->pdata->charging_current[
					charger->cable_type].fast_charging_current * val->intval / 100;

			/* charging current should be over than USB charging current */
			if (charger->pdata->chg_functions_setting &
				SEC_CHARGER_MINIMUM_SIOP_CHARGING_CURRENT) {
				if (input_value.intval > 0 &&
					input_value.intval <
					charger->pdata->charging_current[
					POWER_SUPPLY_TYPE_USB].fast_charging_current)
					input_value.intval =
					charger->pdata->charging_current[
					POWER_SUPPLY_TYPE_USB].fast_charging_current;
			}

			/* set charging current as new value */
			charger->charging_current = input_value.intval;
			if (!SM5414_hal_chg_set_property(charger->client,
				POWER_SUPPLY_PROP_CURRENT_AVG, &input_value))
				return -EINVAL;
		}
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static void SM5414_chg_isr_work(struct work_struct *work)
{
	struct sec_charger_info *charger =
		container_of(work, struct sec_charger_info, isr_work.work);
	union power_supply_propval val;
	int full_check_type;

	dev_info(&charger->client->dev,
		"%s: Charger Interrupt\n", __func__);

	psy_do_property("battery", get,
		POWER_SUPPLY_PROP_CHARGE_NOW, val);
	if (val.intval == SEC_BATTERY_CHARGING_1ST)
		full_check_type = charger->pdata->full_check_type;
	else
		full_check_type = charger->pdata->full_check_type_2nd;

	if (full_check_type == SEC_BATTERY_FULLCHARGED_CHGINT) {
		if (!SM5414_hal_chg_get_property(charger->client,
			POWER_SUPPLY_PROP_STATUS, &val))
			return;

		switch (val.intval) {
		case POWER_SUPPLY_STATUS_DISCHARGING:
			dev_err(&charger->client->dev,
				"%s: Interrupted but Discharging\n", __func__);
			break;

		case POWER_SUPPLY_STATUS_NOT_CHARGING:
			dev_err(&charger->client->dev,
				"%s: Interrupted but NOT Charging\n", __func__);
			break;

		case POWER_SUPPLY_STATUS_FULL:
			dev_info(&charger->client->dev,
				"%s: Interrupted by Full\n", __func__);
			psy_do_property("battery", set,
				POWER_SUPPLY_PROP_STATUS, val);
			break;

		case POWER_SUPPLY_STATUS_CHARGING:
			dev_err(&charger->client->dev,
				"%s: Interrupted but Charging\n", __func__);
			break;

		case POWER_SUPPLY_STATUS_UNKNOWN:
		default:
			dev_err(&charger->client->dev,
				"%s: Invalid Charger Status\n", __func__);
			break;
		}
	}

	if (charger->pdata->ovp_uvlo_check_type ==
		SEC_BATTERY_OVP_UVLO_CHGINT) {

		val.intval = SM5414_get_charging_health(charger->client);

		switch (val.intval) {
		case POWER_SUPPLY_HEALTH_OVERHEAT:
		case POWER_SUPPLY_HEALTH_COLD:
			dev_err(&charger->client->dev,
				"%s: Interrupted but Hot/Cold\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_DEAD:
			dev_err(&charger->client->dev,
				"%s: Interrupted but Dead\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_OVERVOLTAGE:
		case POWER_SUPPLY_HEALTH_UNDERVOLTAGE:
			dev_info(&charger->client->dev,
				"%s: Interrupted by OVP/UVLO\n", __func__);
			psy_do_property("battery", set,
				POWER_SUPPLY_PROP_HEALTH, val);
			break;

		case POWER_SUPPLY_HEALTH_UNSPEC_FAILURE:
			dev_err(&charger->client->dev,
				"%s: Interrupted but Unspec\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_GOOD:
			dev_err(&charger->client->dev,
				"%s: Interrupted but Good\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_UNKNOWN:
		default:
			dev_err(&charger->client->dev,
				"%s: Invalid Charger Health\n", __func__);
			break;
		}
	}

	if (charger->pdata->cable_check_type & SEC_BATTERY_CABLE_CHECK_CHGINT) {
		if (!SM5414_hal_chg_get_property(charger->client,
			POWER_SUPPLY_PROP_ONLINE, &val))
			return;

		/* use SEC_BATTERY_CABLE_SOURCE_EXTERNAL for cable_source_type
		 * charger would call battery driver to set ONLINE property
		 * check battery driver loaded or not
		 */
		if (get_power_supply_by_name("battery")) {
			psy_do_property("battery", set,
				POWER_SUPPLY_PROP_ONLINE, val);
		} else
			charger->pdata->check_cable_result_callback(val.intval);
	}
}

static irqreturn_t SM5414_chg_irq_thread(int irq, void *irq_data)
{
	struct sec_charger_info *charger = irq_data;

	schedule_delayed_work(&charger->isr_work, 0);

	return IRQ_HANDLED;
}

static int SM5414_chg_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(SM5414_charger_attrs); i++) {
		rc = device_create_file(dev, &SM5414_charger_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	goto create_attrs_succeed;

create_attrs_failed:
	dev_err(dev, "%s: failed (%d)\n", __func__, rc);
	while (i--)
		device_remove_file(dev, &SM5414_charger_attrs[i]);
create_attrs_succeed:
	return rc;
}

ssize_t SM5414_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const ptrdiff_t offset = attr - SM5414_charger_attrs;
	int i = 0;

	switch (offset) {
	case CHG_REG:
	case CHG_DATA:
	case CHG_REGS:
		i = SM5414_hal_chg_show_attrs(dev, offset, buf);
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t SM5414_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	const ptrdiff_t offset = attr - SM5414_charger_attrs;
	int ret = 0;

	switch (offset) {
	case CHG_REG:
	case CHG_DATA:
		ret = SM5414_hal_chg_store_attrs(dev, offset, buf, count);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
#ifdef CONFIG_OF
static int SM5414_charger_read_u32_index_dt(const struct device_node *np,
				       const char *propname,
				       u32 index, u32 *out_value)
{
	struct property *prop = of_find_property(np, propname, NULL);
	u32 len = (index + 1) * sizeof(*out_value);

	if (!prop)
		return (-EINVAL);
	if (!prop->value)
		return (-ENODATA);
	if (len > prop->length)
		return (-EOVERFLOW);

	*out_value = be32_to_cpup(((__be32 *)prop->value) + index);

	return 0;
}
extern unsigned int system_rev;
static int SM5414_charger_parse_dt(struct sec_charger_info *charger)
{
	struct device_node *np = of_find_node_by_name(NULL, "charger");
	sec_battery_platform_data_t *pdata = charger->pdata;
	int ret = 0;
	int i, len;
	const u32 *p;

	if (np == NULL) {
		pr_err("%s np NULL\n", __func__);
		return -1;
	} else {
		ret = of_get_named_gpio(np, "battery,vbus_ctrl_gpio", 0);
		if (ret > 0) {
			pdata->vbus_ctrl_gpio = ret;
			pr_info("%s reading vbus_ctrl_gpio = %d\n", __func__, ret);
		} else {
			pdata->vbus_ctrl_gpio = 0;
			pr_info("%s vbus_ctrl_gpio read fail\n", __func__);
		}
		ret = of_property_read_u32(np, "battery,chg_float_voltage",
					&pdata->chg_float_voltage);
		if (ret < 0)
			pr_err("%s: chg_float_voltage read failed (%d)\n", __func__, ret);

		ret = of_property_read_u32(np, "battery,ovp_uvlo_check_type",
					&pdata->ovp_uvlo_check_type);
		if (ret < 0)
			pr_err("%s: ovp_uvlo_check_type read failed (%d)\n", __func__, ret);

		ret = of_property_read_u32(np, "battery,full_check_type",
					&pdata->full_check_type);
		if (ret < 0)
			pr_err("%s: full_check_type read failed (%d)\n", __func__, ret);

		p = of_get_property(np, "battery,input_current_limit", &len);
		len = len / sizeof(u32);
		pdata->charging_current = kzalloc(sizeof(sec_charging_current_t) * len,
						  GFP_KERNEL);

		for(i = 0; i < len; i++) {
			ret = SM5414_charger_read_u32_index_dt(np,
					 "battery,input_current_limit", i,
					 &pdata->charging_current[i].input_current_limit);
			ret = SM5414_charger_read_u32_index_dt(np,
					 "battery,fast_charging_current", i,
					 &pdata->charging_current[i].fast_charging_current);
			ret = SM5414_charger_read_u32_index_dt(np,
					 "battery,full_check_current_1st", i,
					 &pdata->charging_current[i].full_check_current_1st);
			ret = SM5414_charger_read_u32_index_dt(np,
					 "battery,full_check_current_2nd", i,
					 &pdata->charging_current[i].full_check_current_2nd);
		}

		ret = of_property_read_string(np, "battery,fuelgauge_name",
					(char const **)&pdata->fuelgauge_name);
		if (ret)
			pr_err("%s: fuelgauge_name read failed\n", __func__);
	}
	return ret;
}
#else
static int SM5414_charger_parse_dt(struct max77803_charger_data *charger)
{
	return 0;
}
#endif

static int __devinit SM5414_charger_probe(
						struct i2c_client *client,
						const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter =
		to_i2c_adapter(client->dev.parent);
	struct sec_charger_info *charger;
	int ret = 0;

	dev_info(&client->dev,
		"%s: SM5414 Charger Driver Loading\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	charger->client = client;
	if (client->dev.of_node) {
		void * pdata = kzalloc(sizeof(sec_battery_platform_data_t), GFP_KERNEL);
		if (!pdata)
			goto err_free1;
		charger->pdata = pdata;
		if (SM5414_charger_parse_dt(charger))
			dev_err(&client->dev,
				"%s : Failed to get charger dt\n", __func__);
	} else
		charger->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, charger);

	charger->siop_level = 100;
	charger->psy_chg.name		= "SM5414";
	charger->psy_chg.type		= POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property	= SM5414_chg_get_property;
	charger->psy_chg.set_property	= SM5414_chg_set_property;
	charger->psy_chg.properties	= SM5414_charger_props;
	charger->psy_chg.num_properties	= ARRAY_SIZE(SM5414_charger_props);
	charger->is_slow_charging = false;

	if (charger->pdata->chg_gpio_init) {
		if (!charger->pdata->chg_gpio_init()) {
			dev_err(&client->dev,
					"%s: Failed to Initialize GPIO\n", __func__);
			goto err_free;
		}
	}

	if (!SM5414_hal_chg_init(charger->client)) {
		dev_err(&client->dev,
			"%s: Failed to Initialize Charger\n", __func__);
		goto err_free;
	}

	ret = power_supply_register(&client->dev, &charger->psy_chg);
	if (ret) {
		dev_err(&client->dev,
			"%s: Failed to Register psy_chg\n", __func__);
		goto err_free;
	}

	if (charger->pdata->chg_irq) {
		INIT_DELAYED_WORK_DEFERRABLE(
			&charger->isr_work, SM5414_chg_isr_work);

		ret = request_threaded_irq(charger->pdata->chg_irq,
				NULL, SM5414_chg_irq_thread,
				charger->pdata->chg_irq_attr,
				"charger-irq", charger);
		if (ret) {
			dev_err(&client->dev,
				"%s: Failed to Reqeust IRQ\n", __func__);
			goto err_supply_unreg;
		}

		ret = enable_irq_wake(charger->pdata->chg_irq);
		if (ret < 0)
			dev_err(&client->dev,
				"%s: Failed to Enable Wakeup Source(%d)\n",
				__func__, ret);
	}

	ret = SM5414_chg_create_attrs(charger->psy_chg.dev);
	if (ret) {
		dev_err(&client->dev,
			"%s : Failed to create_attrs\n", __func__);
		goto err_req_irq;
	}

	dev_dbg(&client->dev,
		"%s: SM5414 Charger Driver Loaded\n", __func__);
	return 0;

err_req_irq:
	if (charger->pdata->chg_irq)
		free_irq(charger->pdata->chg_irq, charger);
err_supply_unreg:
	power_supply_unregister(&charger->psy_chg);
err_free:
	kfree(charger->pdata);
err_free1:
	kfree(charger);

	return ret;
}

static int __devexit SM5414_charger_remove(
						struct i2c_client *client)
{
	return 0;
}

static int SM5414_charger_suspend(struct i2c_client *client,
				pm_message_t state)
{
	if (!SM5414_hal_chg_suspend(client))
		dev_err(&client->dev,
			"%s: Failed to Suspend Charger\n", __func__);

	return 0;
}

static int SM5414_charger_resume(struct i2c_client *client)
{
	dev_info(&client->dev,"%s: start\n", __func__);

	if (!SM5414_hal_chg_resume(client))
		dev_err(&client->dev,
			"%s: Failed to Resume Charger\n", __func__);

	return 0;
}

static const struct i2c_device_id SM5414_charger_id[] = {
	{"SM5414", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, SM5414_charger_id);
static struct of_device_id SM5414_i2c_match_table[] = {
	{ .compatible = "SM5414,i2c", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, SM5414_i2c_match_table);

static struct i2c_driver SM5414_charger_driver = {
	.driver = {
		.name	= "SM5414",
		.owner = THIS_MODULE,
		.of_match_table = SM5414_i2c_match_table,
	},
	.probe	= SM5414_charger_probe,
	.remove	= __devexit_p(SM5414_charger_remove),
	.suspend	= SM5414_charger_suspend,
	.resume		= SM5414_charger_resume,
	.id_table	= SM5414_charger_id,
};

static int __init SM5414_charger_init(void)
{
	return i2c_add_driver(&SM5414_charger_driver);
}

static void __exit SM5414_charger_exit(void)
{
	i2c_del_driver(&SM5414_charger_driver);
}

module_init(SM5414_charger_init);
module_exit(SM5414_charger_exit);

MODULE_DESCRIPTION("Samsung SM5414 Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");

