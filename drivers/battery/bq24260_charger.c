/*
 *  bq24260_charger.c
 *  Samsung bq24260 Charger Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
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

extern unsigned int system_rev;

static int bq24260_i2c_write(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_write_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	return ret;
}

static int bq24260_i2c_read(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_read_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	return ret;
}

#if 0
static void bq24260_i2c_write_array(struct i2c_client *client,
				u8 *buf, int size)
{
	int i;
	for (i = 0; i < size; i += 3)
		bq24260_i2c_write(client, (u8) (*(buf + i)), (buf + i) + 1);
}
#endif

static void bq24260_set_command(struct i2c_client *client,
				int reg, int datum)
{
	int val;
	u8 data = 0;
	val = bq24260_i2c_read(client, reg, &data);
	if (val >= 0) {
		dev_dbg(&client->dev, "%s : reg(0x%02x): 0x%02x(0x%02x)",
			__func__, reg, data, datum);
		if (data != datum) {
			data = datum;
			if (bq24260_i2c_write(client, reg, &data) < 0)
				dev_err(&client->dev,
					"%s : error!\n", __func__);
			val = bq24260_i2c_read(client, reg, &data);
			if (val >= 0)
				dev_dbg(&client->dev, " => 0x%02x\n", data);
		}
	}
}

static void bq24260_test_read(struct i2c_client *client)
{
	u8 data = 0;
	u32 addr = 0;
	for (addr = 0; addr <= 0x06; addr++) {
		bq24260_i2c_read(client, addr, &data);
		dev_dbg(&client->dev,
			"bq24260 addr : 0x%02x data : 0x%02x\n", addr, data);
	}
}

static void bq24260_read_regs(struct i2c_client *client, char *str)
{
	u8 data = 0;
	u32 addr = 0;

	for (addr = 0; addr <= 0x06; addr++) {
		bq24260_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}
}


static int bq24260_get_charging_status(struct i2c_client *client)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 data = 0;

	bq24260_i2c_read(client, BQ24260_STATUS, &data);
	dev_info(&client->dev,
		"%s : charger status(0x%02x)\n", __func__, data);

	data = (data & 0x30);

	switch (data) {
	case 0x00:
		status = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	case 0x10:
		status = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case 0x20:
		status = POWER_SUPPLY_STATUS_FULL;
		break;
	case 0x30:
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	}

	return (int)status;
}

static int bq24260_get_charging_health(struct i2c_client *client)
{
	int health = POWER_SUPPLY_HEALTH_GOOD;
	u8 data = 0;

	bq24260_i2c_read(client, BQ24260_STATUS, &data);
	dev_info(&client->dev,
		"%s : charger status(0x%02x)\n", __func__, data);

	if ((data & 0x30) == 0x30) {	/* check for fault */
		data = (data & 0x07);

		switch (data) {
		case 0x01:
			health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
			break;
		case 0x02:
			health = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
			break;
		}
	}

	return (int)health;
}

static u8 bq24260_get_float_voltage_data(
			int float_voltage)
{
	u8 data;

	if (float_voltage < 3500)
		float_voltage = 3500;

	data = (float_voltage - 3500) / 20;

	return data << 2;
}

static u8 bq24260_get_input_current_limit_data(
			int input_current)
{
	u8 data = 0x00;

	if (system_rev >= 0x01)
		if (input_current <= 100)
			data = 0x00;
		else if (input_current <= 150)
			data = 0x01;
		else if (input_current <= 500)
			data = 0x02;
		else if (input_current <= 900)
			data = 0x03;/*v2: 1000mA*/
		else if (input_current <= 1500)
			data = 0x04;/*v2: 1300mA*/
		else if (input_current <= 2000)
			data = 0x05;/*1950mA, v2: 1800mA*/
		else if (input_current <= 2500)
			data = 0x06;/*v2: 2200mA*/
		else/*1950mA, v2: 1800mA*/
			data = 0x07;
	else {
		if (input_current <= 100)
			data = 0x00;
		else if (input_current <= 150)
			data = 0x01;
		else if (input_current <= 500)
			data = 0x02;
		else if (input_current <= 900)
			data = 0x03;
		else if (input_current <= 1000)
			data = 0x04;
		else if (input_current <= 2000)
			data = 0x06;/*1950mA*/
		else
			data = 0x07;
	}

	return data << 4;
}

static u8 bq24260_get_termination_current_limit_data(
			int termination_current)
{
	u8 data;

	/* default offset 50mA, max 300mA */
	data = (termination_current - 50) / 50;

	return data;
}

static u8 bq24260_get_fast_charging_current_data(
			int fast_charging_current)
{
	u8 data;

	/* default offset 500mA */
	if (fast_charging_current < 500)
		fast_charging_current = 500;

	data = (fast_charging_current - 500) / 100;

	return data << 3;
}

static void bq24260_charger_function_conrol(
				struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	union power_supply_propval val;
	int full_check_type;
	u8 data;
	if (charger->charging_current < 0) {
		dev_dbg(&client->dev,
			"%s : OTG is activated. Ignore command!\n", __func__);
		return;
	}

	if (charger->cable_type ==
		POWER_SUPPLY_TYPE_BATTERY) {
		data = 0x00;
		bq24260_i2c_read(client, BQ24260_CONTROL, &data);
		data |= 0x2;
		data &= 0x7f; /* Prevent register reset */
		bq24260_set_command(client,
			BQ24260_CONTROL, data);
	} else {
		data = 0x00;
		bq24260_i2c_read(client, BQ24260_CONTROL, &data);
		/* Enable charging */
		data &= 0x7d; /*default enabled*/
		psy_do_property("battery", get,
			POWER_SUPPLY_PROP_CHARGE_NOW, val);
		if (val.intval == SEC_BATTERY_CHARGING_1ST)
			full_check_type = charger->pdata->full_check_type;
		else
			full_check_type = charger->pdata->full_check_type_2nd;
		/* Termination setting */
		switch (full_check_type) {
		case SEC_BATTERY_FULLCHARGED_CHGGPIO:
		case SEC_BATTERY_FULLCHARGED_CHGINT:
		case SEC_BATTERY_FULLCHARGED_CHGPSY:
			/* Enable Current Termination */
			data |= 0x04;
			break;
		default:
			data &= 0x7b;
			break;
		}

		/* Input current limit */
		if (charger->pdata->cable_source_type &
			SEC_BATTERY_CABLE_SOURCE_EXTENDED) {
			dev_dbg(&client->dev, "%s : chg max (%dmA)\n",
				__func__, charger->charging_current_max);
			data &= 0x0F;
			data |= bq24260_get_input_current_limit_data(
				charger->charging_current_max);
		} else {
			dev_dbg(&client->dev, "%s : input current (%dmA)\n",
				__func__, charger->pdata->charging_current
				[charger->cable_type].input_current_limit);
			data &= 0x0F;
			data |= bq24260_get_input_current_limit_data(
				charger->pdata->charging_current
				[charger->cable_type].input_current_limit);
		}
		bq24260_set_command(client,
			BQ24260_CONTROL, data);

		data = 0x00;
		/* Float voltage */
		dev_dbg(&client->dev, "%s : float voltage (%dmV)\n",
			__func__, charger->pdata->chg_float_voltage);
		data |= bq24260_get_float_voltage_data(
			charger->pdata->chg_float_voltage);
		bq24260_set_command(client,
			BQ24260_VOLTAGE, data);

		data = 0x00;
		/* Fast charge and Termination current */
		dev_dbg(&client->dev, "%s : fast charging current (%dmA)\n",
				__func__, charger->charging_current);
		data |= bq24260_get_fast_charging_current_data(
			charger->charging_current);
		dev_dbg(&client->dev, "%s : termination current (%dmA)\n",
			__func__, charger->pdata->charging_current[
			charger->cable_type].full_check_current_1st >= 300 ?
			300 : charger->pdata->charging_current[
			charger->cable_type].full_check_current_1st);
		data |= bq24260_get_termination_current_limit_data(
			charger->pdata->charging_current[
			charger->cable_type].full_check_current_1st);
		bq24260_set_command(client,
			BQ24260_CURRENT, data);

		/* Special Charger Voltage
		 * Normal charge current
		 */
		bq24260_i2c_read(client, BQ24260_SPECIAL, &data);
		data &= 0xd8;
		data |= 0x4;
		bq24260_set_command(client,
			BQ24260_SPECIAL, data);
	}
}

static void bq24260_charger_otg_conrol(
				struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 data;

	bq24260_i2c_read(client, BQ24260_SAFETY, &data);
	data &= ~(0x1 << 4);
	bq24260_set_command(client,	BQ24260_SAFETY, data);
	data = 0x00;

	if (charger->cable_type !=
		POWER_SUPPLY_TYPE_OTG) {
		dev_info(&client->dev, "%s : turn off OTG\n", __func__);
		/* turn off OTG */
		bq24260_i2c_read(client, BQ24260_STATUS, &data);
		data &= 0xbf;
		bq24260_set_command(client,
			BQ24260_STATUS, data);
	} else {
		dev_info(&client->dev, "%s : turn on OTG\n", __func__);
		/* turn on OTG */
		bq24260_i2c_read(client, BQ24260_STATUS, &data);
		data |= 0x40;
		bq24260_set_command(client,
			BQ24260_STATUS, data);
	}
}

static void bq24260_set_input_current(
				struct i2c_client *client, int input_current)
{
	u8 data = 0x00;

	bq24260_i2c_read(client, BQ24260_CONTROL, &data);
	data &= 0x0F;
	data |= bq24260_get_input_current_limit_data(input_current);
	bq24260_set_command(client,	BQ24260_CONTROL, data);
}

static void bq24260_set_charging_current(
				struct i2c_client *client, int charging_current)
{
	u8 data = 0x00;

	bq24260_i2c_read(client, BQ24260_CURRENT, &data);
	data |= bq24260_get_fast_charging_current_data(charging_current);
	bq24260_set_command(client,	BQ24260_CURRENT, data);
}

#if 0
static int bq24260_get_charge_type(struct i2c_client *client)
{
	int ret;
	u8 data;

	bq24260_i2c_read(client, BQ24260_STATUS, &data);
	data = (data & 0x30)>>4;

	switch (data) {
	case 0x01:
		ret = POWER_SUPPLY_CHARGE_TYPE_FAST;
		break;
	default:
		ret = POWER_SUPPLY_CHARGE_TYPE_NONE;
		break;
	}

	return ret;
}
#endif

bool bq24260_hal_chg_init(struct i2c_client *client)
{
	bq24260_test_read(client);
	return true;
}

bool bq24260_hal_chg_suspend(struct i2c_client *client)
{
	return true;
}

bool bq24260_hal_chg_resume(struct i2c_client *client)
{
	return true;
}

bool bq24260_hal_chg_shutdown(struct i2c_client *client)
{
	u8 data = 1;

	bq24260_i2c_write(client, BQ24260_CONTROL, &data);
	return true;
}
bool bq24260_hal_chg_get_property(struct i2c_client *client,
			      enum power_supply_property psp,
			      union power_supply_propval *val)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 data;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = bq24260_get_charging_status(client);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		/*val->intval = bq24260_get_charge_type(client);*/
		if (charger->is_charging)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		else
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = bq24260_get_charging_health(client);
		break;
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CURRENT_AVG:	/* charging current */
		if (charger->charging_current) {
			/* Rsns 0.068 Ohm */
			bq24260_i2c_read(client, BQ24260_CURRENT, &data);
			val->intval = (data >> 3) * 100 + 500;
		} else
			val->intval = 0;
		dev_dbg(&client->dev,
			"%s : set-current(%dmA), current now(%dmA)\n",
			__func__, charger->charging_current, val->intval);
		break;
	default:
		return false;
	}
	return true;
}

bool bq24260_hal_chg_set_property(struct i2c_client *client,
			      enum power_supply_property psp,
			      const union power_supply_propval *val)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);

	switch (psp) {
	/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		if (charger->pdata->chg_gpio_en) {
			if (gpio_request(charger->pdata->chg_gpio_en,
				"CHG_EN") < 0) {
				dev_err(&client->dev,
					"failed to request vbus_in gpio\n");
				break;
			}
			if (charger->cable_type ==
				POWER_SUPPLY_TYPE_BATTERY)
				gpio_set_value_cansleep(
					charger->pdata->chg_gpio_en,
					charger->pdata->chg_polarity_en ?
					0 : 1);
			else
				gpio_set_value_cansleep(
					charger->pdata->chg_gpio_en,
					charger->pdata->chg_polarity_en ?
					1 : 0);
			gpio_free(charger->pdata->chg_gpio_en);
		}

		if (val->intval == POWER_SUPPLY_TYPE_POWER_SHARING) {
			union power_supply_propval ps_status;
			psy_do_property("ps", get,
				POWER_SUPPLY_PROP_STATUS, ps_status);
			if (ps_status.intval) {
				charger->cable_type = POWER_SUPPLY_TYPE_OTG;
				pr_info("%s: ps enable\n", __func__);
			} else {
				charger->cable_type = POWER_SUPPLY_TYPE_BATTERY;
				pr_info("%s: ps disable\n", __func__);
			}
		}

#if defined(CONFIG_MUIC_SUPPORT_MULTIMEDIA_DOCK)
		if(charger->is_mdock) {
			if(charger->is_smartotg)
				charger->cable_type = POWER_SUPPLY_TYPE_SMART_OTG;
			else
				charger->cable_type = POWER_SUPPLY_TYPE_MDOCK_TA;

			pr_debug("%s: cable type %d\n", __func__, charger->cable_type);
		}
#endif

		if (charger->charging_current >= 0)
			bq24260_charger_function_conrol(client);

		bq24260_charger_otg_conrol(client);
		bq24260_test_read(client);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX: /* input current limit set */
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		bq24260_set_input_current(client, val->intval);
		break;
	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		bq24260_set_charging_current(client, val->intval);
		break;
	default:
		return false;
	}
	return true;
}

ssize_t bq24260_hal_chg_show_attrs(struct device *dev,
				const ptrdiff_t offset, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *chg =
		container_of(psy, struct sec_charger_info, psy_chg);
	int i = 0;
	char *str = NULL;

	switch (offset) {
	case CHG_REG:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
			chg->reg_addr);
		break;
	case CHG_DATA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
			chg->reg_data);
		break;
	case CHG_REGS:
		str = kzalloc(sizeof(char)*1024, GFP_KERNEL);
		if (!str)
			return -ENOMEM;

		bq24260_read_regs(chg->client, str);
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

ssize_t bq24260_hal_chg_store_attrs(struct device *dev,
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
			bq24260_i2c_read(chg->client,
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
			bq24260_i2c_write(chg->client,
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

static struct device_attribute bq24260_charger_attrs[] = {
	BQ24260_CHARGER_ATTR(reg),
	BQ24260_CHARGER_ATTR(data),
	BQ24260_CHARGER_ATTR(regs),
};

static enum power_supply_property bq24260_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
};

static int bq24260_chg_get_property(struct power_supply *psy,
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
		if (!bq24260_hal_chg_get_property(charger->client, psp, val))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int bq24260_chg_set_property(struct power_supply *psy,
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
			val->intval == POWER_SUPPLY_TYPE_OTG || \
			val->intval == POWER_SUPPLY_TYPE_POWER_SHARING) {
			charger->is_charging = false;
#if defined(CONFIG_MUIC_SUPPORT_MULTIMEDIA_DOCK)
			charger->is_mdock = false;
			charger->is_smartotg = false;
#endif
		}
		else {
			charger->is_charging = true;
#if defined(CONFIG_MUIC_SUPPORT_MULTIMEDIA_DOCK)
			if(val->intval == POWER_SUPPLY_TYPE_SMART_NOTG)
				charger->is_smartotg = false;
			else if (val->intval == POWER_SUPPLY_TYPE_SMART_OTG)
				charger->is_smartotg = true;
			if (val->intval == POWER_SUPPLY_TYPE_MDOCK_TA)
				charger->is_mdock = true;
#endif
		}

		/* current setting */
		if (!(charger->pdata->cable_source_type &
			SEC_BATTERY_CABLE_SOURCE_EXTENDED)) {
			charger->charging_current_max =
				charger->pdata->charging_current[
				val->intval].input_current_limit;

			charger->charging_current =
				charger->pdata->charging_current[
				val->intval].fast_charging_current;
		}

		if (!bq24260_hal_chg_set_property(charger->client, psp, val))
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
		if (!bq24260_hal_chg_set_property(charger->client, psp, val))
			return -EINVAL;
		break;

	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		charger->charging_current = val->intval;

		if (!bq24260_hal_chg_set_property(charger->client, psp, val))
			return -EINVAL;
		break;

	/* val->intval : SIOP level (%)
	 * SIOP charging current setting
	 */
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		/* change val as charging current by SIOP level
		 * do NOT change initial charging current setting
		 */
		input_value.intval =
			charger->charging_current * val->intval / 100;

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
		if (!bq24260_hal_chg_set_property(charger->client,
			POWER_SUPPLY_PROP_CURRENT_AVG, &input_value))
			return -EINVAL;
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static void bq24260_chg_isr_work(struct work_struct *work)
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
		if (!bq24260_hal_chg_get_property(charger->client,
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
		if (!bq24260_hal_chg_get_property(charger->client,
			POWER_SUPPLY_PROP_HEALTH, &val))
			return;

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
		if (!bq24260_hal_chg_get_property(charger->client,
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

static irqreturn_t bq24260_chg_irq_thread(int irq, void *irq_data)
{
	struct sec_charger_info *charger = irq_data;

	schedule_delayed_work(&charger->isr_work, 0);

	return IRQ_HANDLED;
}

static int bq24260_chg_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(bq24260_charger_attrs); i++) {
		rc = device_create_file(dev, &bq24260_charger_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	goto create_attrs_succeed;

create_attrs_failed:
	dev_err(dev, "%s: failed (%d)\n", __func__, rc);
	while (i--)
		device_remove_file(dev, &bq24260_charger_attrs[i]);
create_attrs_succeed:
	return rc;
}

ssize_t bq24260_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const ptrdiff_t offset = attr - bq24260_charger_attrs;
	int i = 0;

	switch (offset) {
	case CHG_REG:
	case CHG_DATA:
	case CHG_REGS:
		i = bq24260_hal_chg_show_attrs(dev, offset, buf);
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t bq24260_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	const ptrdiff_t offset = attr - bq24260_charger_attrs;
	int ret = 0;

	switch (offset) {
	case CHG_REG:
	case CHG_DATA:
		ret = bq24260_hal_chg_store_attrs(dev, offset, buf, count);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
#ifdef CONFIG_OF
static int bq24260_charger_read_u32_index_dt(const struct device_node *np,
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

static int bq24260_charger_parse_dt(struct sec_charger_info *charger)
{
	struct device_node *np = of_find_node_by_name(NULL, "charger");
	sec_battery_platform_data_t *pdata = charger->pdata;
	int ret = 0;
	int i, len;
	const u32 *p;

	if (np == NULL) {
		pr_err("%s np NULL\n", __func__);
	} else {
		ret = of_property_read_u32(np, "battery,chg_float_voltage",
					&pdata->chg_float_voltage);
		ret = of_property_read_u32(np, "battery,ovp_uvlo_check_type",
					&pdata->ovp_uvlo_check_type);
		ret = of_property_read_u32(np, "battery,full_check_type",
					&pdata->full_check_type);

		p = of_get_property(np, "battery,input_current_limit", &len);
		len = len / sizeof(u32);
		pdata->charging_current = kzalloc(sizeof(sec_charging_current_t) * len,
						  GFP_KERNEL);

		for(i = 0; i < len; i++) {
			ret = bq24260_charger_read_u32_index_dt(np,
					 "battery,input_current_limit", i,
					 &pdata->charging_current[i].input_current_limit);
			ret = bq24260_charger_read_u32_index_dt(np,
					 "battery,fast_charging_current", i,
					 &pdata->charging_current[i].fast_charging_current);
			ret = bq24260_charger_read_u32_index_dt(np,
					 "battery,full_check_current_1st", i,
					 &pdata->charging_current[i].full_check_current_1st);
			ret = bq24260_charger_read_u32_index_dt(np,
					 "battery,full_check_current_2nd", i,
					 &pdata->charging_current[i].full_check_current_2nd);
		}
	}

	return ret;
}
#endif

static int __devinit bq24260_charger_probe(
						struct i2c_client *client,
						const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter =
		to_i2c_adapter(client->dev.parent);
	struct sec_charger_info *charger;
	int ret = 0;

	dev_info(&client->dev,
		"%s: BQ24260 Charger Driver Loading\n", __func__);

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
		if (bq24260_charger_parse_dt(charger))
			dev_err(&client->dev,
				"%s : Failed to get charger dt\n", __func__);
	} else
		charger->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, charger);

	charger->psy_chg.name		= "bq24260";
	charger->psy_chg.type		= POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property	= bq24260_chg_get_property;
	charger->psy_chg.set_property	= bq24260_chg_set_property;
	charger->psy_chg.properties	= bq24260_charger_props;
	charger->psy_chg.num_properties	= ARRAY_SIZE(bq24260_charger_props);

	if (!bq24260_hal_chg_init(charger->client)) {
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
			&charger->isr_work, bq24260_chg_isr_work);

		ret = request_threaded_irq(charger->pdata->chg_irq,
				NULL, bq24260_chg_irq_thread,
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

	ret = bq24260_chg_create_attrs(charger->psy_chg.dev);
	if (ret) {
		dev_err(&client->dev,
			"%s : Failed to create_attrs\n", __func__);
		goto err_req_irq;
	}

	dev_dbg(&client->dev,
		"%s: BQ24260 Charger Driver Loaded\n", __func__);
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

static int __devexit bq24260_charger_remove(
						struct i2c_client *client)
{
	return 0;
}

static int bq24260_charger_suspend(struct i2c_client *client,
				pm_message_t state)
{
	if (!bq24260_hal_chg_suspend(client))
		dev_err(&client->dev,
			"%s: Failed to Suspend Charger\n", __func__);

	return 0;
}

static int bq24260_charger_resume(struct i2c_client *client)
{
	if (!bq24260_hal_chg_resume(client))
		dev_err(&client->dev,
			"%s: Failed to Resume Charger\n", __func__);

	return 0;
}

static void bq24260_charger_shutdown(struct i2c_client *client)
{
#if defined(CONFIG_CHARGER_BQ24260)
		bq24260_hal_chg_shutdown(client);
#endif

}

static const struct i2c_device_id bq24260_charger_id[] = {
	{"bq24260", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, bq24260_charger_id);
static struct of_device_id bq24260_i2c_match_table[] = {
	{ .compatible = "bq24260,i2c", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, bq24260_i2c_match_table);

static struct i2c_driver bq24260_charger_driver = {
	.driver = {
		.name	= "bq24260",
		.owner = THIS_MODULE,
		.of_match_table = bq24260_i2c_match_table,
	},
	.probe	= bq24260_charger_probe,
	.remove	= __devexit_p(bq24260_charger_remove),
	.suspend	= bq24260_charger_suspend,
	.resume		= bq24260_charger_resume,
	.shutdown	= bq24260_charger_shutdown,
	.id_table	= bq24260_charger_id,
};

static int __init bq24260_charger_init(void)
{
	return i2c_add_driver(&bq24260_charger_driver);
}
static void __exit bq24260_charger_exit(void)
{
	i2c_del_driver(&bq24260_charger_driver);
}

module_init(bq24260_charger_init);
module_exit(bq24260_charger_exit);

MODULE_DESCRIPTION("Samsung bq24260 Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");

