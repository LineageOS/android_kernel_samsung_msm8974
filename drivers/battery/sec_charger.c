/*
 *  sec_charger.c
 *  Samsung Mobile Charger Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG

#include <linux/battery/sec_charger.h>
#include <linux/of_gpio.h>

static struct device_attribute sec_charger_attrs[] = {
	SEC_CHARGER_ATTR(reg),
	SEC_CHARGER_ATTR(data),
	SEC_CHARGER_ATTR(regs),
};

static enum power_supply_property sec_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
};

static int sec_chg_get_property(struct power_supply *psy,
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
		if (!sec_hal_chg_get_property(charger->client, psp, val))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sec_chg_set_property(struct power_supply *psy,
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
		if (val->intval == POWER_SUPPLY_TYPE_BATTERY)
			charger->is_charging = false;
		else
			charger->is_charging = true;

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

		if (!sec_hal_chg_set_property(charger->client, psp, val))
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
		if (!sec_hal_chg_set_property(charger->client, psp, val))
			return -EINVAL;
		break;

	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		charger->charging_current = val->intval;

		if (!sec_hal_chg_set_property(charger->client, psp, val))
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
		if (!sec_hal_chg_set_property(charger->client,
			POWER_SUPPLY_PROP_CURRENT_AVG, &input_value))
			return -EINVAL;
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static void sec_chg_isr_work(struct work_struct *work)
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
		if (!sec_hal_chg_get_property(charger->client,
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
		if (!sec_hal_chg_get_property(charger->client,
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
		if (!sec_hal_chg_get_property(charger->client,
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

static irqreturn_t sec_chg_irq_thread(int irq, void *irq_data)
{
	struct sec_charger_info *charger = irq_data;

	schedule_delayed_work(&charger->isr_work, 0);

	return IRQ_HANDLED;
}

static int sec_chg_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(sec_charger_attrs); i++) {
		rc = device_create_file(dev, &sec_charger_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	goto create_attrs_succeed;

create_attrs_failed:
	dev_err(dev, "%s: failed (%d)\n", __func__, rc);
	while (i--)
		device_remove_file(dev, &sec_charger_attrs[i]);
create_attrs_succeed:
	return rc;
}

ssize_t sec_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const ptrdiff_t offset = attr - sec_charger_attrs;
	int i = 0;

	switch (offset) {
	case CHG_REG:
	case CHG_DATA:
	case CHG_REGS:
		i = sec_hal_chg_show_attrs(dev, offset, buf);
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t sec_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	const ptrdiff_t offset = attr - sec_charger_attrs;
	int ret = 0;

	switch (offset) {
	case CHG_REG:
	case CHG_DATA:
		ret = sec_hal_chg_store_attrs(dev, offset, buf, count);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

#ifdef CONFIG_OF
static int smb358_parse_dt(struct sec_charger_info *charger)
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

		for(i = 0; i < len; i++)
			pdata->charging_current[i].input_current_limit = be32_to_cpup(((__be32 *)p) + i);

		p = of_get_property(np, "battery,fast_charging_current", &len);
		len = len / sizeof(u32);
		for(i = 0; i < len; i++)
			pdata->charging_current[i].fast_charging_current = be32_to_cpup(((__be32 *)p) + i);

		p = of_get_property(np, "battery,full_check_current_1st", &len);
		len = len / sizeof(u32);
		for(i = 0; i < len; i++)
			pdata->charging_current[i].full_check_current_1st = be32_to_cpup(((__be32 *)p) + i);

		p = of_get_property(np, "battery,full_check_current_2nd", &len);
		len = len / sizeof(u32);
		for(i = 0; i < len; i++)
			pdata->charging_current[i].full_check_current_2nd = be32_to_cpup(((__be32 *)p) + i);
	}
	return ret;
}
#else
static int smb358_parse_dt(struct max77803_charger_data *charger)
{
	return 0;
}
#endif

static int __devinit sec_charger_probe(
						struct i2c_client *client,
						const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter =
		to_i2c_adapter(client->dev.parent);
	struct sec_charger_info *charger;
	int ret = 0;

	dev_info(&client->dev,
		"%s: SEC Charger Driver Loading\n", __func__);

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
		if (smb358_parse_dt(charger))
			dev_err(&client->dev,
				"%s : Failed to get charger int\n", __func__);
	} else
		charger->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, charger);

	charger->psy_chg.name		= "sec-charger";
	charger->psy_chg.type		= POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property	= sec_chg_get_property;
	charger->psy_chg.set_property	= sec_chg_set_property;
	charger->psy_chg.properties	= sec_charger_props;
	charger->psy_chg.num_properties	= ARRAY_SIZE(sec_charger_props);

	if (!charger->pdata->chg_gpio_init()) {
		dev_err(&client->dev,
			"%s: Failed to Initialize GPIO\n", __func__);
		goto err_free;
	}

	if (!sec_hal_chg_init(charger->client)) {
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
			&charger->isr_work, sec_chg_isr_work);

		ret = request_threaded_irq(charger->pdata->chg_irq,
				NULL, sec_chg_irq_thread,
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

	ret = sec_chg_create_attrs(charger->psy_chg.dev);
	if (ret) {
		dev_err(&client->dev,
			"%s : Failed to create_attrs\n", __func__);
		goto err_req_irq;
	}

	dev_dbg(&client->dev,
		"%s: SEC Charger Driver Loaded\n", __func__);
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

static int __devexit sec_charger_remove(
						struct i2c_client *client)
{
	return 0;
}

static int sec_charger_suspend(struct i2c_client *client,
				pm_message_t state)
{
	if (!sec_hal_chg_suspend(client))
		dev_err(&client->dev,
			"%s: Failed to Suspend Charger\n", __func__);

	return 0;
}

static int sec_charger_resume(struct i2c_client *client)
{
	if (!sec_hal_chg_resume(client))
		dev_err(&client->dev,
			"%s: Failed to Resume Charger\n", __func__);

	return 0;
}

static void sec_charger_shutdown(struct i2c_client *client)
{
#if defined(CONFIG_CHARGER_BQ24260)
	sec_hal_chg_shutdown(client);
#endif
}

static const struct i2c_device_id sec_charger_id[] = {
	{"sec-charger", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sec_charger_id);
static struct of_device_id charger_i2c_match_table[] = {
	{ .compatible = "sec-charger,i2c", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, charger_i2c_match_table);

static struct i2c_driver sec_charger_driver = {
	.driver = {
		.name	= "sec-charger",
		.owner = THIS_MODULE,
		.of_match_table = charger_i2c_match_table,
	},
	.probe	= sec_charger_probe,
	.remove	= __devexit_p(sec_charger_remove),
	.suspend	= sec_charger_suspend,
	.resume		= sec_charger_resume,
	.shutdown	= sec_charger_shutdown,
	.id_table	= sec_charger_id,
};

static int __init sec_charger_init(void)
{
	return i2c_add_driver(&sec_charger_driver);
}

static void __exit sec_charger_exit(void)
{
	i2c_del_driver(&sec_charger_driver);
}

module_init(sec_charger_init);
module_exit(sec_charger_exit);

MODULE_DESCRIPTION("Samsung Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
