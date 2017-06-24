/*
 *  smb358_charger.c
 *  Samsung SMB358 Charger Driver
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

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/of_gpio.h>

#ifndef SLOW_CHARGING_CURRENT_STANDARD

#if defined(CONFIG_MACH_HEAT_AIO)
#define SLOW_CHARGING_CURRENT_STANDARD 400
#else
#define SLOW_CHARGING_CURRENT_STANDARD 1000
#endif

#endif

static int smb358_i2c_write(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_write_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	return ret;
}

static int smb358_i2c_read(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;
	ret = i2c_smbus_read_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);
	return ret;
}

/*static void smb358_i2c_write_array(struct i2c_client *client,
				u8 *buf, int size)
{
	int i;
	for (i = 0; i < size; i += 3)
		smb358_i2c_write(client, (u8) (*(buf + i)), (buf + i) + 1);
}*/

static int smb358_update_reg(struct i2c_client *client, int reg, u8 data)
{
	int ret;
	u8 r_data = 0;
	u8 w_data = 0;
	u8 o_data = data;

	ret = smb358_i2c_read(client, reg, &r_data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - read(%d)\n", __func__, ret);
		goto error;
	}

	w_data  = r_data | data;
	ret = smb358_i2c_write(client, reg, &w_data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - write(%d)\n", __func__, ret);
		goto error;
	}

	ret = smb358_i2c_read(client, reg, &data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - read(%d)\n", __func__, ret);
		goto error;
	}

	dev_dbg(&client->dev,
		"%s: reg(0x%02x) 0x%02x : 0x%02x -> 0x%02x -> 0x%02x\n",
		__func__, reg, o_data, r_data, w_data, data);

error:
	return ret;
}

static int smb358_clear_reg(struct i2c_client *client, int reg, u8 data)
{
	int ret;
	u8 r_data = 0;
	u8 w_data = 0;
	u8 o_data = data;

	ret = smb358_i2c_read(client, reg, &r_data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - read(%d)\n", __func__, ret);
		goto error;
	}

	w_data  = r_data & (~data);
	ret = smb358_i2c_write(client, reg, &w_data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - write(%d)\n", __func__, ret);
		goto error;
	}

	ret = smb358_i2c_read(client, reg, &data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - read(%d)\n", __func__, ret);
		goto error;
	}

	dev_dbg(&client->dev,
		"%s: reg(0x%02x)- 0x%02x : 0x%02x -> 0x%02x -> 0x%02x\n",
		__func__, reg, o_data, r_data, w_data, data);

error:
	return ret;
}

static int smb358_volatile_writes(struct i2c_client *client, u8 value)
{
	int ret = 0;

	if (value == SMB358_ENABLE_WRITE) {
		ret = smb358_update_reg(client, SMB358_COMMAND_A, 0x80);
		if (ret < 0) {
			dev_err(&client->dev, "%s: error(%d)\n", __func__, ret);
			goto error;
		}
		dev_dbg(&client->dev, "%s: ENABLED\n", __func__);
	} else {
		ret = smb358_clear_reg(client, SMB358_COMMAND_A, 0x80);
		if (ret < 0) {
			dev_err(&client->dev, "%s: error(%d)\n", __func__, ret);
			goto error;
		}
		dev_dbg(&client->dev, "%s: DISABLED\n", __func__);
	}

error:
	return ret;
}

static void smb358_set_command(struct i2c_client *client,
				int reg, u8 datum)
{
	int val;
	u8 after_data;

	if (smb358_i2c_write(client, reg, &datum) < 0)
		dev_err(&client->dev,
			"%s : error!\n", __func__);

	val = smb358_i2c_read(client, reg, &after_data);
	if (val >= 0)
		dev_info(&client->dev,
			"%s : reg(0x%02x) 0x%02x => 0x%02x\n",
			__func__, reg, datum, after_data);
	else
		dev_err(&client->dev, "%s : error!\n", __func__);
}

#if 0
static void smb358_test_read(struct i2c_client *client)
{
	u8 data = 0;
	u32 addr = 0;
	for (addr = 0; addr <= 0x0f; addr++) {
		smb358_i2c_read(client, addr, &data);
		dev_dbg(&client->dev,
			"%s : smb358 addr : 0x%02x data : 0x%02x\n",
						__func__, addr, data);
	}
	for (addr = 0x30; addr <= 0x3f; addr++) {
		smb358_i2c_read(client, addr, &data);
		dev_dbg(&client->dev,
			"%s : smb358 addr : 0x%02x data : 0x%02x\n",
						__func__, addr, data);
	}
}
#endif

static void smb358_read_regs(struct i2c_client *client, char *str)
{
	u8 data = 0;
	u32 addr = 0;

	for (addr = 0; addr <= 0x0f; addr++) {
		smb358_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}

	/* "#" considered as new line in application */
	sprintf(str+strlen(str), "#");

	for (addr = 0x30; addr <= 0x3f; addr++) {
		smb358_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}
}

static int smb358_get_aicl_current(u8 aicl_current)
{
	int data;

	if (aicl_current <= 0x10)
		data = 300;
	else if (aicl_current <= 0x11)
		data = 500;
	else if (aicl_current <= 0x12)
		data = 700;
	else if (aicl_current <= 0x13)
		data = 1000;
	else if (aicl_current <= 0x14)
		data = 1200;
	else if (aicl_current <= 0x15)
		data = 1300;
	else if (aicl_current <= 0x16)
		data = 1800;
	else
		data = 2000;

	return data;
}

static int smb358_get_charging_status(struct i2c_client *client)
{
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 data_a = 0;
	u8 data_b = 0;
	u8 data_c = 0;
	u8 data_d = 0;
	u8 data_e = 0;
	u8 therm_control_a = 0;
	u8 other_control_a = 0;


	/*smb358_test_read(client);*/

	smb358_i2c_read(client, SMB358_STATUS_A, &data_a);
	dev_dbg(&client->dev,
		"%s : charger status A(0x%02x)\n", __func__, data_a);
	smb358_i2c_read(client, SMB358_STATUS_B, &data_b);
	dev_dbg(&client->dev,
		"%s : charger status B(0x%02x)\n", __func__, data_b);
	smb358_i2c_read(client, SMB358_STATUS_C, &data_c);
	dev_dbg(&client->dev,
		"%s : charger status C(0x%02x)\n", __func__, data_c);
	smb358_i2c_read(client, SMB358_STATUS_D, &data_d);
	dev_dbg(&client->dev,
		"%s : charger status D(0x%02x)\n", __func__, data_d);
	smb358_i2c_read(client, SMB358_STATUS_E, &data_e);
	dev_dbg(&client->dev,
		"%s : charger status E(0x%02x)\n", __func__, data_e);
	smb358_i2c_read(client, SMB358_THERM_CONTROL_A, &therm_control_a);
	dev_dbg(&client->dev,
		"%s : THERM_CONTROL_A(0x%02x)\n", __func__, therm_control_a);
	smb358_i2c_read(client, SMB358_OTHER_CONTROL_A, &other_control_a);
	dev_dbg(&client->dev,
		"%s : OTHER_CONTROL_A(0x%02x)\n", __func__, other_control_a);
	/* At least one charge cycle terminated,
	 * Charge current < Termination Current
	 */
	if (data_c & 0x20) {
		/* top-off by full charging */
		status = POWER_SUPPLY_STATUS_FULL;
		goto charging_status_end;
	}

	/* Is enabled ? */
	if (data_c & 0x01) {
		/* check for 0x06 : no charging (0b00) */
		/* not charging */
		if (!(data_c & 0x06)) {
			status = POWER_SUPPLY_STATUS_NOT_CHARGING;
			goto charging_status_end;
		} else {
			status = POWER_SUPPLY_STATUS_CHARGING;
			goto charging_status_end;
		}
	} else
		status = POWER_SUPPLY_STATUS_DISCHARGING;

charging_status_end:
	return (int)status;
}

static int smb358_get_charging_health(struct i2c_client *client)
{
	int health = POWER_SUPPLY_HEALTH_GOOD;
	u8 data_a = 0;
	u8 data_b = 0;
	u8 data_c = 0;
	u8 data_d = 0;
	u8 data_e = 0;

	smb358_i2c_read(client, SMB358_STATUS_A, &data_a);
	dev_dbg(&client->dev,
		"%s : charger status A(0x%02x)\n", __func__, data_a);
	smb358_i2c_read(client, SMB358_STATUS_B, &data_b);
	dev_dbg(&client->dev,
		"%s : charger status B(0x%02x)\n", __func__, data_b);
	smb358_i2c_read(client, SMB358_STATUS_C, &data_c);
	dev_dbg(&client->dev,
		"%s : charger status C(0x%02x)\n", __func__, data_c);
	smb358_i2c_read(client, SMB358_STATUS_D, &data_d);
	dev_dbg(&client->dev,
		"%s : charger status D(0x%02x)\n", __func__, data_d);
	smb358_i2c_read(client, SMB358_STATUS_E, &data_e);
	dev_dbg(&client->dev,
		"%s : charger status E(0x%02x)\n", __func__, data_e);
	smb358_i2c_read(client, SMB358_INTERRUPT_STATUS_E, &data_e);
	dev_dbg(&client->dev,
		"%s : charger interrupt status E(0x%02x)\n", __func__, data_e);

	if (data_e & 0x01)
		health = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
	else if (data_e & 0x04)
		health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;

	return (int)health;
}

/*static void smb358_allow_volatile_writes(struct i2c_client *client)
{
	int val, reg;
	u8 data;
	reg = SMB358_COMMAND_A;
	val = smb358_i2c_read(client, reg, &data);
	if ((val >= 0) && !(data & 0x80)) {
		dev_dbg(&client->dev,
			"%s : reg(0x%02x): 0x%02x", __func__, reg, data);
		data |= (0x1 << 7);
		if (smb358_i2c_write(client, reg, &data) < 0)
			dev_err(&client->dev, "%s : error!\n", __func__);
		val = smb358_i2c_read(client, reg, &data);
		if (val >= 0) {
			data = (u8) data;
			dev_dbg(&client->dev, " => 0x%02x\n", data);
		}
	}
}*/

static u8 smb358_get_float_voltage_data(int float_voltage)
{
	u8 data;

	if (float_voltage < 3500)
		data = 0;
	else if(float_voltage <= 4340)
		data = (float_voltage - 3500) / 20;
	else if(float_voltage == 4350)
		data = 0x2B; /* (4340 -3500)/20 + 1 */
	else if(float_voltage <= 4500)
		data = (float_voltage - 3500) / 20 + 1;
	else
		data = 51;

	return data;
}

static u8 smb358_get_input_current_limit_data(
			struct sec_charger_info *charger, int input_current)
{
	u8 data;

	if (input_current <= 300)
		data = 0x00;
	else if (input_current <= 500)
		data = 0x01;
	else if (input_current <= 700)
		data = 0x02;
	else if (input_current <= 1000)
		data = 0x03;
	else if (input_current <= 1200)
		data = 0x04;
	else if (input_current <= 1500)
		data = 0x05;
	else if (input_current <= 1800)
		data = 0x06;
	else
		data = 0x07;	/* set input current to 2000mA */

	return (data << 4);
}

static u8 smb358_get_term_current_limit_data(
			int termination_current)
{
	u8 data;

	if (termination_current <= 30)
		data = 0x00;
	else if (termination_current <= 40)
		data = 0x01;
	else if (termination_current <= 60)
		data = 0x02;
	else if (termination_current <= 80)
		data = 0x03;
	else if (termination_current <= 100)
		data = 0x04;
	else if (termination_current <= 125)
		data = 0x05;
	else if (termination_current <= 150)
		data = 0x06;
	else
		data = 0x07;	/* set termination current limit to 200mA */

	return data;
}

static u8 smb358_get_fast_charging_current_data(
			int fast_charging_current)
{
	u8 data;

	if (fast_charging_current <= 200)
		data = 0x00;
	else if (fast_charging_current <= 450)
		data = 0x01;
	else if (fast_charging_current <= 600)
		data = 0x02;
	else if (fast_charging_current <= 900)
		data = 0x03;
	else if (fast_charging_current <= 1300)
		data = 0x04;
	else if (fast_charging_current <= 1500)
		data = 0x05;
	else if (fast_charging_current <= 1800)
		data = 0x06;
	else
		data = 0x07;	/* set fast charging current to 2000mA */

	return data << 5;
}

static void smb358_enter_suspend(struct i2c_client *client)
{
	u8 data = 0;

	pr_info("%s: ENTER SUSPEND\n", __func__);
	smb358_update_reg(client, SMB358_COMMAND_A, 0x80);
	smb358_set_command(client, SMB358_PIN_ENABLE_CONTROL, 0x18);
	data = (data | 0x4);
	smb358_set_command(client, SMB358_COMMAND_A, data);
}

#if 0
#if (defined(CONFIG_MACH_MILLET3G_EUR) || defined(CONFIG_MACH_MATISSE3G_OPEN) || defined(CONFIG_MACH_BERLUTI3G_EUR))
static void smb358_aicl_calibrate(struct i2c_client *client)
{

	struct sec_charger_info *charger = i2c_get_clientdata(client);
	int ret = 0;
	u8 data = 0;
	u8 count = 0;
	u8 current_initial = 0;
	u8 current_final = 0;

	ret = smb358_i2c_read(client, SMB358_STATUS_E, &data);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - read(%d)\n", __func__, ret);
	}

	/* check If AICL complete */
	if(data & 0x10){
		current_initial = (data & 0x0F);

		if((current_initial >= 0x04) && (current_initial <= 0x05)){
			/* set 1000mA */
			ret = smb358_i2c_read(client, SMB358_INPUT_CURRENTLIMIT, &data);
			data = data & 0x0F;
			data = data | 0x30;
			smb358_set_command(client,SMB358_INPUT_CURRENTLIMIT, data);
		}else{
			/* set 1800mA */
			ret = smb358_i2c_read(client, SMB358_INPUT_CURRENTLIMIT, &data);
			data = data & 0x0F;
			data = data | 0x60;
			smb358_set_command(client,SMB358_INPUT_CURRENTLIMIT, data);
		}

		smb358_i2c_read(client, SMB358_INPUT_CURRENTLIMIT, &data);
		current_final = (data & 0x0F);

		dev_err(&charger->client->dev,
			"%s: AICL calibration success! input current (%dmA) -> (%dmA) ! \n",
			__func__,current_initial,current_final);
		return;

        }else{
        /* Incase of AICL not complete check three times */
		for(count=0 ;count < 3;count++)
		{
			msleep(300);
			ret = smb358_i2c_read(client, SMB358_STATUS_E, &data);
			if (ret < 0) {
				dev_err(&client->dev, "%s: error - read(%d)\n", __func__, ret);
			}

			if(data & 0x10){
				current_initial = (data & 0x0F);

				if((current_initial >= 0x04) && (current_initial <= 0x05)){
				/* set 1000mA */
					ret = smb358_i2c_read(client, SMB358_INPUT_CURRENTLIMIT, &data);
					data = data & 0x0F;
					data = data | 0x30;
					smb358_set_command(client,SMB358_INPUT_CURRENTLIMIT, data);
				}else{
				/* set 1800mA */
					ret = smb358_i2c_read(client, SMB358_INPUT_CURRENTLIMIT, &data);
					data = data & 0x0F;
					data = data | 0x60;
					smb358_set_command(client,SMB358_INPUT_CURRENTLIMIT, data);
				}

				smb358_i2c_read(client, SMB358_INPUT_CURRENTLIMIT, &data);
				current_final = (data & 0x0F);

				dev_err(&charger->client->dev,
					"%s: AICL calibration success! input current (%dmA) -> (%dmA) ! \n",
					__func__,current_initial,current_final);
				return;
			}else{
				dev_err(&charger->client->dev,
                                        "%s: AICL not complete \n",__func__);
			}
		}
	}

	dev_err(&charger->client->dev,
		"%s: AICL calibration Failed! current (%dmA) ! \n",__func__,current_initial);
	return;

}
#endif
#endif

static void smb358_check_slow_charging(struct work_struct *work)
{
	struct sec_charger_info *charger =
		container_of(work, struct sec_charger_info, slow_work.work);

	u8 i, aicl_data;
	int aicl_current = 0;
	union power_supply_propval val;

	if (charger->pdata->chg_functions_setting &
			SEC_CHARGER_NO_GRADUAL_CHARGING_CURRENT) {
		pr_err("%s: aicl is disabled\n", __func__);
		return;
	}
	if (charger->pdata->charging_current
				[charger->cable_type].input_current_limit <= SLOW_CHARGING_CURRENT_STANDARD) {
			charger->is_slow_charging = true;
	} else {
		for(i = 0; i < 20; i++) {
			if (charger->cable_type ==
					POWER_SUPPLY_TYPE_BATTERY) {
				pr_info("%s: cable is removed\n", __func__);
				return;
			}
			msleep(200);
			smb358_i2c_read(charger->client, SMB358_STATUS_E, &aicl_data);
			if (aicl_data & 0x10) { /* check AICL complete */
				break;
			}
			if (i == 10) {
				pr_info("%s: aicl not complete, retry\n", __func__);
				/* disable AICL */
				smb358_set_command(charger->client,
						SMB358_VARIOUS_FUNCTIONS, 0x81);
				/* enable AICL */
				smb358_set_command(charger->client,
						SMB358_VARIOUS_FUNCTIONS, 0x95);
			}
		}

		aicl_data &= 0xF; /* get only AICL result field */
		switch (aicl_data) {
		case 0: /* AICL result 300mA */
			aicl_current = 300;
			break;
		case 1: /* AICL result 500mA */
			aicl_current = 500;
			break;
		case 2: /* AICL result 700mA */
			aicl_current = 700;
			break;
		case 3: /* AICL result 1000mA */
			aicl_current = 1000;
			break;
		case 4: /* AICL result 1200mA */
			aicl_current = 1200;
			break;
		case 5: /* AICL result 1300mA */
			aicl_current = 1300;
			break;
		case 6: /* AICL result 1800mA */
			aicl_current = 1800;
			break;
		case 7: /* AICL result 2000mA */
			aicl_current = 2000;
			break;
		default: /* etc */
			aicl_current = 2000;
			break;
		}
		if (aicl_current <= SLOW_CHARGING_CURRENT_STANDARD)
			charger->is_slow_charging = true;
		else
			charger->is_slow_charging = false;
	}

	pr_info("%s: Slow(%d), aicl_current(%d), input_current(%d)\n",
		__func__, charger->is_slow_charging, aicl_current, charger->pdata->charging_current
			[charger->cable_type].input_current_limit);

	psy_do_property("battery", set,
		POWER_SUPPLY_PROP_CHARGE_TYPE, val);

}
static void smb358_charger_function_control(
				struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	union power_supply_propval val, input_value;
	int status;
	u8 data;

	psy_do_property("battery", get,
		POWER_SUPPLY_PROP_STATUS, input_value);
	status = input_value.intval;

	charger->charging_current_max =
		charger->pdata->charging_current[
		charger->cable_type].input_current_limit;

	charger->charging_current =
		charger->pdata->charging_current[
		charger->cable_type].fast_charging_current;

	if (charger->charging_current < 0) {
		dev_dbg(&client->dev,
			"%s : OTG is activated. Ignore command!\n", __func__);
		return;
	}

	psy_do_property("battery", get,
		POWER_SUPPLY_PROP_HEALTH, input_value);
	if (input_value.intval ==
		POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
		pr_info("[SMB358] Unspec_failure, charger suspend\n");
		smb358_enter_suspend(client);
	}
	else if (charger->cable_type ==
		POWER_SUPPLY_TYPE_BATTERY) {
		/* Charger Disabled */
		smb358_set_command(client, SMB358_COMMAND_A, 0xc0);
		if ((status == POWER_SUPPLY_STATUS_FULL) ||\
			(input_value.intval == POWER_SUPPLY_HEALTH_OVERHEAT) ||\
			(input_value.intval == POWER_SUPPLY_HEALTH_COLD))
			return;
		pr_info("[SMB358] Set the registers to the default configuration\n");
		/* Set the registers to the default configuration */
		smb358_set_command(client, SMB358_CHARGE_CURRENT, 0xFE);
		smb358_set_command(client, SMB358_INPUT_CURRENTLIMIT, 0x74);
		smb358_set_command(client, SMB358_VARIOUS_FUNCTIONS, 0xD7);
		data = 0x00;
		data |= smb358_get_float_voltage_data(charger->pdata->chg_float_voltage);
		smb358_set_command(client, SMB358_FLOAT_VOLTAGE, data);
		/* Disable Automatic Recharge */
		smb358_set_command(client, SMB358_CHARGE_CONTROL, 0x84);
		smb358_set_command(client, SMB358_PIN_ENABLE_CONTROL, 0x09);
		smb358_set_command(client, SMB358_THERM_CONTROL_A, 0xF0);
		smb358_set_command(client, SMB358_SYSOK_USB30_SELECTION, 0x08);
		smb358_set_command(client, SMB358_OTHER_CONTROL_A, 0x01);
		smb358_set_command(client, SMB358_OTG_TLIM_THERM_CONTROL, 0xF6);
		smb358_set_command(client, SMB358_LIMIT_CELL_TEMPERATURE_MONITOR, 0xA5);
		smb358_set_command(client, SMB358_STATUS_INTERRUPT, 0x00);
		smb358_set_command(client, SMB358_COMMAND_B, 0x00);
		if (charger->pdata->chg_irq) {
			smb358_set_command(client, SMB358_STAT_TIMERS_CONTROL, 0x1F);
			smb358_set_command(client, SMB358_FAULT_INTERRUPT, 0x0C);
		} else {
			smb358_set_command(client, SMB358_STAT_TIMERS_CONTROL, 0x0F);
			smb358_set_command(client, SMB358_FAULT_INTERRUPT, 0x00);
		}

	} else {
		int full_check_type;
		psy_do_property("battery", get,
			POWER_SUPPLY_PROP_CHARGE_NOW, val);
		if (val.intval == SEC_BATTERY_CHARGING_1ST)
			full_check_type = charger->pdata->full_check_type;
		else
			full_check_type = charger->pdata->full_check_type_2nd;

		smb358_i2c_read(client, SMB358_COMMAND_A, &data);

		if ((data & 0x10) && charger->pdata->vbus_ctrl_gpio) {
			int level;
			/* disable otg & charging */
			smb358_clear_reg(client, SMB358_COMMAND_A, 0x12);

			/* turn off vbus */
			gpio_set_value(charger->pdata->vbus_ctrl_gpio, 1);
			msleep(30);

			level = gpio_get_value_cansleep(charger->pdata->vbus_ctrl_gpio);
			pr_info("[SMB358] vbus ctrl gpio %d level %d\n", charger->pdata->vbus_ctrl_gpio, level);

			/* turn on vbus */
			gpio_set_value(charger->pdata->vbus_ctrl_gpio, 0);
		}
		/* [STEP - 1] ================================================
                 * Volatile write permission(bit 7) - allow(1)
                 * Charging Enable(bit 1) - Disabled(0, default)
                 * STAT Output(bit 0) - Enabled(0)
                */
		if (data & 0x02) {
			u8 status_c;
			u8 status_e;
			smb358_i2c_read(client, SMB358_STATUS_C, &status_c);
			smb358_i2c_read(client, SMB358_STATUS_E, &status_e);
			pr_info("[SMB358] status_c: 0x%x, status_e: 0x%x\n", status_c, status_e);

			/* no charge or aicl not complete*/
			if (((status_c & 0x06) == 0) || (status_e & 0x10) == 0)
				smb358_set_command(client,
						SMB358_COMMAND_A, 0xC0);
		}

                /* [STEP - 2] ================================================
                 * USB 5/1(9/1.5) Mode(bit 1) - USB1/USB1.5(0), USB5/USB9(1)
                 * USB/HC Mode(bit 0) - USB5/1 or USB9/1.5 Mode(0)
                 *                      High-Current Mode(1)
                */
                switch (charger->cable_type) {
		case POWER_SUPPLY_TYPE_UNKNOWN:
		case POWER_SUPPLY_TYPE_MAINS:
		case POWER_SUPPLY_TYPE_USB_CDP:
		case POWER_SUPPLY_TYPE_MISC:
		case POWER_SUPPLY_TYPE_WIRELESS:
		case POWER_SUPPLY_TYPE_CARDOCK:
		case POWER_SUPPLY_TYPE_UARTOFF:
		case POWER_SUPPLY_TYPE_LAN_HUB:
		case POWER_SUPPLY_TYPE_MHL_900:
		case POWER_SUPPLY_TYPE_MHL_1500:
		case POWER_SUPPLY_TYPE_SMART_OTG:
		case POWER_SUPPLY_TYPE_SMART_NOTG:
#if defined(CONFIG_MUIC_SUPPORT_MULTIMEDIA_DOCK)
		case POWER_SUPPLY_TYPE_MDOCK_TA:
#endif
                    /* High-current mode */
                    data = 0x03;
			break;
		case POWER_SUPPLY_TYPE_UPS:
		case POWER_SUPPLY_TYPE_USB:
		case POWER_SUPPLY_TYPE_USB_DCP:
		case POWER_SUPPLY_TYPE_USB_ACA:
		case POWER_SUPPLY_TYPE_MHL_500:
		case POWER_SUPPLY_TYPE_MHL_USB:
		case POWER_SUPPLY_TYPE_POWER_SHARING:
#if defined(CONFIG_MUIC_SUPPORT_MULTIMEDIA_DOCK)
		case POWER_SUPPLY_TYPE_MDOCK_USB:
#endif
			/* USB5 */
			data = 0x02;
			break;
		default:
			/* USB1 */
			data = 0x00;
			break;
		}
		smb358_set_command(client,
			SMB358_COMMAND_B, data);


		/* [STEP 3] Charge Current(0x00) ===============================
		 * Set pre-charge current(bit 4:3) - 450mA(11)
		 * Set fast charge current(bit 7:5)
		 * Set termination current(bit 2:0)
		*/
		if (charger->siop_level < 100) {
			charger->charging_current =
				charger->charging_current * charger->siop_level / 100;
		}
		dev_info(&client->dev,
			"%s : fast charging current (%dmA)\n",
			__func__, charger->charging_current);

		data = 0x18;
		data |= smb358_get_fast_charging_current_data(
			charger->charging_current);
		switch (full_check_type) {
		case SEC_BATTERY_FULLCHARGED_CHGGPIO:
		case SEC_BATTERY_FULLCHARGED_CHGINT:
		case SEC_BATTERY_FULLCHARGED_CHGPSY:
			if (val.intval == SEC_BATTERY_CHARGING_1ST) {
				dev_info(&client->dev,
					"%s : termination current (%dmA)\n",
					__func__, charger->pdata->charging_current[
					charger->cable_type].full_check_current_1st);
				data |= smb358_get_term_current_limit_data(
					charger->pdata->charging_current[
					charger->cable_type].full_check_current_1st);
			} else {
				dev_info(&client->dev,
					"%s : termination current (%dmA)\n",
					__func__, charger->pdata->charging_current[
					charger->cable_type].full_check_current_2nd);
				data |= smb358_get_term_current_limit_data(
					charger->pdata->charging_current[
					charger->cable_type].full_check_current_2nd);
			}
			break;
		}
		smb358_set_command(client,
			SMB358_CHARGE_CURRENT, data);

		/* [STEP - 4] =================================================
		 * Enable(EN) Pin Control(bit 6) - i2c(0), Pin(1)
		 * Pin control(bit 5) - active high(0), active low(1)
		 * USB5/1/HC input State(bit3) - Dual-state input(1)
		 * USB Input Pre-bias(bit 0) - Enable(1)
		*/
		data = 0x09;
		if (charger->pdata->chg_gpio_en)
			data |= 0x40;
		if (charger->pdata->chg_polarity_en)
			data |= 0x20;
		smb358_set_command(client,
			SMB358_PIN_ENABLE_CONTROL, data);

		/* [STEP - 5] =============================================== */
		dev_info(&client->dev, "%s : input current (%dmA)\n",
			__func__, charger->pdata->charging_current
			[charger->cable_type].input_current_limit);
		/* Input current limit */
		data = 0x00;
		data |= smb358_get_input_current_limit_data(
			charger,
			charger->pdata->charging_current
			[charger->cable_type].input_current_limit);
		smb358_set_command(client,
			SMB358_INPUT_CURRENTLIMIT, data);

		/* [STEP - 6] =================================================
		 * Input to System FET(bit 7) - Controlled by Register(1)
		 * Max System voltage(bit 5) -  Vflt + 0.1v(0)
		 * AICL(bit 4) - Enabled(1)
		 * VCHG Function(bit 0) - Enabled(1)
		 */
		if (charger->pdata->chg_functions_setting &
			SEC_CHARGER_NO_GRADUAL_CHARGING_CURRENT)
			/* disable AICL */
			smb358_set_command(client,
				SMB358_VARIOUS_FUNCTIONS, 0x81);
		else {
			/* disable AICL */
			smb358_set_command(client,
				SMB358_VARIOUS_FUNCTIONS, 0x81);
			/* enable AICL */
			smb358_set_command(client,
				SMB358_VARIOUS_FUNCTIONS, 0x95);
		}

		/* [STEP - 7] =================================================
		 * Pre-charged to Fast-charge Voltage Threshold(Bit 7:6) - 2.3V
		 * Float Voltage(bit 5:0)
		*/
		dev_dbg(&client->dev, "%s : float voltage (%dmV)\n",
				__func__, charger->pdata->chg_float_voltage);
		data = 0x00;
		data |= smb358_get_float_voltage_data(
			charger->pdata->chg_float_voltage);
		smb358_set_command(client,
			SMB358_FLOAT_VOLTAGE, data);

		/* [STEP - 8] =================================================
		 * Charge control
		 * Automatic Recharge disable(bit 7),
		 * Current Termination disable(bit 6),
		 * BMD disable(bit 5:4),
		 * INOK Output Configuration : Push-pull(bit 3)
		 * AICL glitch filter duration : 20msec(bit 0)
		 * APSD disable
		*/
		data = 0xC0;

		switch (full_check_type) {
		case SEC_BATTERY_FULLCHARGED_CHGGPIO:
		case SEC_BATTERY_FULLCHARGED_CHGINT:
		case SEC_BATTERY_FULLCHARGED_CHGPSY:
			/* Enable Current Termination */
			data &= 0xBF;
			break;
		}
		smb358_set_command(client,
			SMB358_CHARGE_CONTROL, data);

		/* [STEP - 9] =================================================
		 *  STAT active low(bit 7),
		 *  Complete charge Timeout(bit 3:2) - Disabled(11)
		 *  Pre-charge Timeout(bit 1:0) - Disable(11)
		*/
		smb358_set_command(client,
			SMB358_STAT_TIMERS_CONTROL, 0x1F);

#if defined(CONFIG_MACH_MATISSELTE_VZW)
		/* [STEP - 10] =================================================
		 * Mininum System Voltage(bit 6) - 3.15v(0)
		 * Therm monitor(bit 4) - Disabled(1)
		 * Soft Cold/Hot Temp Limit Behavior(bit 3:2, bit 1:0) -
		 *   Charger Current + Float voltage Compensation(11)
		*/
		smb358_set_command(client,
			SMB358_THERM_CONTROL_A, 0xB0);

		/* [STEP - 11] ================================================
		 * OTG/ID Pin Control(bit 7:6) - RID Disabled, OTG I2c(00)
		 * Minimum System Voltage(bit 4) - 3.15V(0)
		 * Low-Battery/SYSOK Voltage threshold(bit 3:0) - 2.5V(0001)
		 *    if this bit is disabled,
		 *    input current for system will be disabled
		 */
		smb358_set_command(client,
			SMB358_OTHER_CONTROL_A, 0x11);
#elif defined(CONFIG_MACH_CHAGALL)
		/* [STEP - 10] =================================================
		 * Mininum System Voltage(bit 6) - 3.60v(0)
		 * Therm monitor(bit 4) - Disabled(1)
		 * Soft Cold/Hot Temp Limit Behavior(bit 3:2, bit 1:0) -
		 *   Charger Current + Float voltage Compensation(11)
		*/
		smb358_set_command(client,
			SMB358_THERM_CONTROL_A, 0xF0);

		/* [STEP - 11] ================================================
		 * OTG/ID Pin Control(bit 7:6) - RID Disabled, OTG I2c(00)
		 * Minimum System Voltage(bit 4) - 3.60V(0)
		 * Low-Battery/SYSOK Voltage threshold(bit 3:0) - 2.5V(0001)
		 *    if this bit is disabled,
		 *    input current for system will be disabled
		 */
		smb358_set_command(client,
			SMB358_OTHER_CONTROL_A, 0x01);
#else
		/* [STEP - 10] =================================================
		 * Mininum System Voltage(bit 6) - 3.75v(1)
		* Therm monitor(bit 4) - Disabled(1)
		* Soft Cold/Hot Temp Limit Behavior(bit 3:2, bit 1:0) -
		*   Charger Current + Float voltage Compensation(11)
		*/
		smb358_set_command(client,
			SMB358_THERM_CONTROL_A, 0xF0);

		/* [STEP - 11] ================================================
		 * OTG/ID Pin Control(bit 7:6) - RID Disabled, OTG I2c(00)
		 * Minimum System Voltage(bit 4) - 3.75V(1)
		 * Low-Battery/SYSOK Voltage threshold(bit 3:0) - 2.5V(0001)
		 *   if this bit is disabled,
		 *   input current for system will be disabled
		 */
		smb358_set_command(client,
			SMB358_OTHER_CONTROL_A, 0x11);
#endif

		/* [STEP - 12] ================================================
		 * Charge Current Compensation(bit 7:6) - 200mA(00)
		 * Digital Thermal Regulation Threshold(bit 5:4) - 130c
		 * OTG current Limit at USBIN(Bit 3:2) - 900mA(11)
		 * OTG Battery UVLO Threshold(Bit 1:0) - 3.3V(11)
		*/
		smb358_set_command(client,
			SMB358_OTG_TLIM_THERM_CONTROL, 0x3F);

		/* [STEP - 13] ================================================
		 * Hard/Soft Limit Cell temp monitor
		*/
		smb358_set_command(client,
			SMB358_LIMIT_CELL_TEMPERATURE_MONITOR, 0x01);

		/* [STEP - 14] ================================================
		 * FAULT interrupt - Disabled for non chg_irq, OVP enabled for chg_irq
		*/
		if (charger->pdata->chg_irq) {
			smb358_set_command(client,
				SMB358_FAULT_INTERRUPT, 0x0C);
		} else {
			smb358_set_command(client,
				SMB358_FAULT_INTERRUPT, 0x00);
		}

		/* [STEP - 15] ================================================
		 * STATUS interrupt - Clear
		*/
		smb358_set_command(client,
			SMB358_STATUS_INTERRUPT, 0x00);

		/* [STEP - 16] ================================================
		 * Volatile write permission(bit 7) - allowed(1)
		 * Charging Enable(bit 1) - Enabled(1)
		 * STAT Output(bit 0) - Enabled(0)
		*/
		smb358_set_command(client,
			SMB358_COMMAND_A, 0xC2);

		schedule_delayed_work(&charger->slow_work, 0);
	}
#if 0
#if (defined(CONFIG_MACH_MILLET3G_EUR) || defined(CONFIG_MACH_MATISSE3G_OPEN) || defined(CONFIG_MACH_BERLUTI3G_EUR))
	/* Allow time for AICL to complete */
	msleep(1000);
	smb358_aicl_calibrate(client);
#endif
#endif
}

static void smb358_charger_otg_control(
				struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);

	if (charger->cable_type ==
		POWER_SUPPLY_TYPE_BATTERY) {
		dev_info(&client->dev, "%s : turn off OTG\n", __func__);

		/* disable otg */
		smb358_clear_reg(client, SMB358_COMMAND_A, 0x10);
	} else {
		/* Change "OTG output current limit" to 250mA */
		smb358_clear_reg(client, SMB358_OTG_TLIM_THERM_CONTROL, 0x0C);

		/* OTG Enabled*/
		smb358_update_reg(client, SMB358_COMMAND_A, 0x10);

		dev_info(&client->dev, "%s : turn on OTG\n", __func__);
		smb358_set_command(client, SMB358_COMMAND_B, 0x00);

		/* Change "OTG output current limit" to 500mA */
		smb358_update_reg(client, SMB358_OTG_TLIM_THERM_CONTROL, 0x84);
	}

}

static void smb358_set_charging_current(
		struct i2c_client *client, int charging_current)
{
	u8 data;

	smb358_clear_reg(client, SMB358_COMMAND_A, 0x02);

	if (!charging_current)
		return;

	smb358_i2c_read(client, SMB358_CHARGE_CURRENT, &data);
	data &= 0x1f;
	data |= smb358_get_fast_charging_current_data(charging_current);
	smb358_set_command(client, SMB358_CHARGE_CURRENT, data);

	smb358_update_reg(client, SMB358_COMMAND_A, 0x02);
}

static void smb358_set_charging_input_current_limit(
		struct i2c_client *client, int input_current_limit)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 data;

	/* Input current limit */
	data = 0;
	data = smb358_get_input_current_limit_data(
		charger, input_current_limit);
	smb358_set_command(client, SMB358_INPUT_CURRENTLIMIT, data);
}

void smb358_charger_shutdown(struct i2c_client *client)
{
	pr_info("%s: smb358 Charging Disabled\n", __func__);

	smb358_set_command(client, SMB358_THERM_CONTROL_A, 0xF0);
	smb358_set_command(client, SMB358_COMMAND_A, 0x80);
	smb358_volatile_writes(client, SMB358_DISABLE_WRITE);
}

static int smb358_debugfs_show(struct seq_file *s, void *data)
{
	struct sec_charger_info *charger = s->private;
	u8 reg;
	u8 reg_data;

	seq_printf(s, "SMB CHARGER IC :\n");
	seq_printf(s, "==================\n");
	for (reg = 0x00; reg <= 0x0E; reg++) {
		smb358_i2c_read(charger->client, reg, &reg_data);
		seq_printf(s, "0x%02x:\t0x%02x\n", reg, reg_data);
	}

	for (reg = 0x30; reg <= 0x3F; reg++) {
		smb358_i2c_read(charger->client, reg, &reg_data);
		seq_printf(s, "0x%02x:\t0x%02x\n", reg, reg_data);
	}

	seq_printf(s, "\n");
	return 0;
}

static int smb358_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, smb358_debugfs_show, inode->i_private);
}

static const struct file_operations smb358_debugfs_fops = {
	.open           = smb358_debugfs_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

bool smb358_hal_chg_init(struct i2c_client *client)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);

	dev_info(&client->dev,
		"%s: SMB358 Charger init(Start)!!\n", __func__);

	smb358_volatile_writes(client, SMB358_ENABLE_WRITE);

	/*smb358_test_read(client);*/
	(void) debugfs_create_file("smb358_regs",
		S_IRUGO, NULL, (void *)charger, &smb358_debugfs_fops);

	return true;
}

bool smb358_hal_chg_suspend(struct i2c_client *client)
{
	return true;
}

bool smb358_hal_chg_resume(struct i2c_client *client)
{
	return true;
}

bool smb358_hal_chg_get_property(struct i2c_client *client,
			      enum power_supply_property psp,
			      union power_supply_propval *val)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);
	u8 data;
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = smb358_get_charging_status(client);
		break;

	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (charger->is_charging) {
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
			if (charger->is_slow_charging) {
				val->intval = POWER_SUPPLY_CHARGE_TYPE_SLOW;
				pr_info("%s: slow-charging mode\n", __func__);
			}
		}
		else
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = smb358_get_charging_health(client);
		break;
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CURRENT_AVG:	/* charging current */
		if (charger->charging_current) {
			smb358_i2c_read(client, SMB358_STATUS_B, &data);
			if (data & 0x20)
				switch (data & 0x07) {
				case 0:
					val->intval = 100;
					break;
				case 1:
					val->intval = 200;
					break;
				case 2:
					val->intval = 450;
					break;
				case 3:
					val->intval = 600;
					break;
				case 4:
					val->intval = 900;
					break;
				case 5:
					val->intval = 1300;
					break;
				case 6:
					val->intval = 1500;
					break;
				case 7:
					val->intval = 1800;
					break;
				}
			else
				switch ((data & 0x18) >> 3) {
				case 0:
					val->intval = 100;
					break;
				case 1:
					val->intval = 150;
					break;
				case 2:
					val->intval = 200;
					break;
				case 3:
					val->intval = 250;
					break;
				}
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

bool smb358_hal_chg_set_property(struct i2c_client *client,
			      enum power_supply_property psp,
			      const union power_supply_propval *val)
{
	struct sec_charger_info *charger = i2c_get_clientdata(client);

	switch (psp) {
	/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
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
		if (charger->cable_type == POWER_SUPPLY_TYPE_OTG) {
			smb358_charger_otg_control(client);
		} else if (charger->cable_type == POWER_SUPPLY_TYPE_BATTERY) {
			smb358_charger_function_control(client);
			smb358_charger_otg_control(client);
		} else {
			smb358_charger_function_control(client);
		}
		/* smb358_test_read(client); */
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:	/* input current limit set */
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		smb358_set_charging_input_current_limit(client, val->intval);
		break;
	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		smb358_set_charging_current(client, val->intval);
		break;
	default:
		return false;
	}
	return true;
}

ssize_t smb358_hal_chg_show_attrs(struct device *dev,
				const ptrdiff_t offset, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *chg =
		container_of(psy, struct sec_charger_info, psy_chg);
	int i = 0;
	char *str = NULL;

	switch (offset) {
	case CHG_DATA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
			chg->reg_data);
		break;
	case CHG_REGS:
		str = kzalloc(sizeof(char)*1024, GFP_KERNEL);
		if (!str)
			return -ENOMEM;

		smb358_read_regs(chg->client, str);
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

ssize_t smb358_hal_chg_store_attrs(struct device *dev,
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
			smb358_i2c_read(chg->client,
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
			smb358_i2c_write(chg->client,
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

static struct device_attribute smb358_charger_attrs[] = {
	SMB358_CHARGER_ATTR(reg),
	SMB358_CHARGER_ATTR(data),
	SMB358_CHARGER_ATTR(regs),
};

static enum power_supply_property smb358_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
};

static int smb358_chg_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct sec_charger_info *charger =
		container_of(psy, struct sec_charger_info, psy_chg);
	u8 data = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_MAX:	/* input current limit set */
		smb358_i2c_read(charger->client, SMB358_STATUS_E, &data);
		if (data & 0x10) {
			int aicl_result = smb358_get_aicl_current(data);
			dev_info(&charger->client->dev,
				"%s : AICL completed (%dmA)\n", __func__, aicl_result);
			charger->charging_current_max = aicl_result;
		} else {
			dev_info(&charger->client->dev,
				"%s : AICL is not completed \n", __func__);
			charger->charging_current_max = 300;
		}
		val->intval = charger->charging_current_max;
		break;

	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_HEALTH:
	case POWER_SUPPLY_PROP_CURRENT_AVG:	/* charging current */
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (!smb358_hal_chg_get_property(charger->client, psp, val))
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int smb358_chg_set_property(struct power_supply *psy,
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
			charger->is_slow_charging = false;
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

		if (!smb358_hal_chg_set_property(charger->client, psp, val))
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
		if (!smb358_hal_chg_set_property(charger->client, psp, val))
			return -EINVAL;
		break;

	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		charger->charging_current = val->intval;

		if (!smb358_hal_chg_set_property(charger->client, psp, val))
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
			if (!smb358_hal_chg_set_property(charger->client,
				POWER_SUPPLY_PROP_CURRENT_AVG, &input_value))
				return -EINVAL;
		}
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static void smb358_chg_isr_work(struct work_struct *work)
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
		if (!smb358_hal_chg_get_property(charger->client,
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
		if (!smb358_hal_chg_get_property(charger->client,
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
			/* Do not set POWER_SUPPLY_PROP_HEALTH
			* excute monitor work again.
			* ovp/uvlo is checked by polling
			*/
			psy_do_property("battery", set,
				POWER_SUPPLY_PROP_CHARGE_TYPE, val);
			break;

		case POWER_SUPPLY_HEALTH_UNSPEC_FAILURE:
			dev_err(&charger->client->dev,
				"%s: Interrupted but Unspec\n", __func__);
			break;

		case POWER_SUPPLY_HEALTH_GOOD:
			dev_err(&charger->client->dev,
				"%s: Interrupted but Good\n", __func__);
			/* Do not set POWER_SUPPLY_PROP_HEALTH
			* excute monitor work again.
			* ovp/uvlo is checked by polling
			*/
			psy_do_property("battery", set,
				POWER_SUPPLY_PROP_CHARGE_TYPE, val);
			break;

		case POWER_SUPPLY_HEALTH_UNKNOWN:
		default:
			dev_err(&charger->client->dev,
				"%s: Invalid Charger Health\n", __func__);
			break;
		}
	}

	if (charger->pdata->cable_check_type & SEC_BATTERY_CABLE_CHECK_CHGINT) {
		if (!smb358_hal_chg_get_property(charger->client,
			POWER_SUPPLY_PROP_ONLINE, &val))
			return;

		/* use SEC_BATTERY_CABLE_SOURCE_EXTERNAL for cable_source_type
		 * charger would call battery driver to set ONLINE property
		 * check battery driver loaded or not
		 */
		if (get_power_supply_by_name("battery")) {
			psy_do_property("battery", set,
				POWER_SUPPLY_PROP_ONLINE, val);
		} else {
			if (charger->pdata->check_cable_result_callback)
				charger->pdata->check_cable_result_callback(val.intval);
		}
	}
}

static irqreturn_t smb358_chg_irq_thread(int irq, void *irq_data)
{
	struct sec_charger_info *charger = irq_data;

	schedule_delayed_work(&charger->isr_work, 0);

	return IRQ_HANDLED;
}

static int smb358_chg_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(smb358_charger_attrs); i++) {
		rc = device_create_file(dev, &smb358_charger_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	goto create_attrs_succeed;

create_attrs_failed:
	dev_err(dev, "%s: failed (%d)\n", __func__, rc);
	while (i--)
		device_remove_file(dev, &smb358_charger_attrs[i]);
create_attrs_succeed:
	return rc;
}

ssize_t smb358_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const ptrdiff_t offset = attr - smb358_charger_attrs;
	int i = 0;

	switch (offset) {
	case CHG_REG:
	case CHG_DATA:
	case CHG_REGS:
		i = smb358_hal_chg_show_attrs(dev, offset, buf);
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t smb358_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	const ptrdiff_t offset = attr - smb358_charger_attrs;
	int ret = 0;

	switch (offset) {
	case CHG_REG:
	case CHG_DATA:
		ret = smb358_hal_chg_store_attrs(dev, offset, buf, count);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
#ifdef CONFIG_OF
static int smb358_charger_read_u32_index_dt(const struct device_node *np,
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
static int smb358_charger_parse_dt(struct sec_charger_info *charger)
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
#if defined(CONFIG_MACH_VIENNAVZW) || defined(CONFIG_MACH_VIENNAATT)
	if (system_rev >= 0xC)
		pdata->vbus_ctrl_gpio = 28;
	else
		pdata->vbus_ctrl_gpio = 0;
	pr_info("%s reading vbus_ctrl_gpio = %d\n",
		__func__, pdata->vbus_ctrl_gpio);
#else
		ret = of_get_named_gpio(np, "battery,vbus_ctrl_gpio", 0);
		if (ret > 0) {
			pdata->vbus_ctrl_gpio = ret;
			pr_info("%s reading vbus_ctrl_gpio = %d\n", __func__, ret);
		} else {
			pdata->vbus_ctrl_gpio = 0;
			pr_info("%s vbus_ctrl_gpio read fail\n", __func__);
		}
#endif
		ret = of_property_read_u32(np, "battery,chg_float_voltage",
					&pdata->chg_float_voltage);
		if (ret < 0)
			pr_err("%s: chg_float_voltage read failed (%d)\n", __func__, ret);

		ret = of_property_read_u32(np, "battery,ovp_uvlo_check_type",
					&pdata->ovp_uvlo_check_type);
		if (ret < 0)
			pr_err("%s: ovp_uvlo_check_type read failed (%d)\n", __func__, ret);

		ret = of_get_named_gpio(np, "battery,chg_int", 0);
		if (ret > 0) {
			pdata->chg_irq = gpio_to_irq(ret);
			pr_info("%s reading chg_int_gpio = %d\n", __func__, ret);
		} else {
			pr_info("%s reading chg_int_gpio is empty\n", __func__);
		}

		ret = of_property_read_u32(np, "battery,chg_irq_attr",
					(unsigned int *)&pdata->chg_irq_attr);
		if (ret)
			pr_info("%s: chg_irq_attr is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,full_check_type",
					&pdata->full_check_type);
		if (ret < 0)
			pr_err("%s: full_check_type read failed (%d)\n", __func__, ret);

		p = of_get_property(np, "battery,input_current_limit", &len);
		len = len / sizeof(u32);
		pdata->charging_current = kzalloc(sizeof(sec_charging_current_t) * len,
						  GFP_KERNEL);

		for(i = 0; i < len; i++) {
			ret = smb358_charger_read_u32_index_dt(np,
					 "battery,input_current_limit", i,
					 &pdata->charging_current[i].input_current_limit);
			ret = smb358_charger_read_u32_index_dt(np,
					 "battery,fast_charging_current", i,
					 &pdata->charging_current[i].fast_charging_current);
			ret = smb358_charger_read_u32_index_dt(np,
					 "battery,full_check_current_1st", i,
					 &pdata->charging_current[i].full_check_current_1st);
			ret = smb358_charger_read_u32_index_dt(np,
					 "battery,full_check_current_2nd", i,
					 &pdata->charging_current[i].full_check_current_2nd);
		}
	}
	return ret;
}
#else
static int smb358_charger_parse_dt(struct max77803_charger_data *charger)
{
	return 0;
}
#endif

static int __devinit smb358_charger_probe(
						struct i2c_client *client,
						const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter =
		to_i2c_adapter(client->dev.parent);
	struct sec_charger_info *charger;
	int ret = 0;

	dev_info(&client->dev,
		"%s: SMB358 Charger Driver Loading\n", __func__);

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
		if (smb358_charger_parse_dt(charger))
			dev_err(&client->dev,
				"%s : Failed to get charger dt\n", __func__);
	} else
		charger->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, charger);

	charger->siop_level = 100;
	charger->psy_chg.name		= "smb358";
	charger->psy_chg.type		= POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property	= smb358_chg_get_property;
	charger->psy_chg.set_property	= smb358_chg_set_property;
	charger->psy_chg.properties	= smb358_charger_props;
	charger->psy_chg.num_properties	= ARRAY_SIZE(smb358_charger_props);
	charger->is_slow_charging = false;

	if (charger->pdata->chg_gpio_init) {
		if (!charger->pdata->chg_gpio_init()) {
			dev_err(&client->dev,
					"%s: Failed to Initialize GPIO\n", __func__);
			goto err_free;
		}
	}

	if (!smb358_hal_chg_init(charger->client)) {
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

	INIT_DELAYED_WORK_DEFERRABLE(&charger->slow_work,
		smb358_check_slow_charging);

	if (charger->pdata->chg_irq) {
		INIT_DELAYED_WORK_DEFERRABLE(
			&charger->isr_work, smb358_chg_isr_work);

		ret = request_threaded_irq(charger->pdata->chg_irq,
				NULL, smb358_chg_irq_thread,
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

	ret = smb358_chg_create_attrs(charger->psy_chg.dev);
	if (ret) {
		dev_err(&client->dev,
			"%s : Failed to create_attrs\n", __func__);
		goto err_req_irq;
	}

	dev_dbg(&client->dev,
		"%s: SMB358 Charger Driver Loaded\n", __func__);
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

static int __devexit smb358_charger_remove(
						struct i2c_client *client)
{
	return 0;
}

static int smb358_charger_suspend(struct i2c_client *client,
				pm_message_t state)
{
	if (!smb358_hal_chg_suspend(client))
		dev_err(&client->dev,
			"%s: Failed to Suspend Charger\n", __func__);

	return 0;
}

static int smb358_charger_resume(struct i2c_client *client)
{
	dev_info(&client->dev,"%s: start\n", __func__);

	if (!smb358_hal_chg_resume(client))
		dev_err(&client->dev,
			"%s: Failed to Resume Charger\n", __func__);

	return 0;
}

static const struct i2c_device_id smb358_charger_id[] = {
	{"smb358", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, smb358_charger_id);
static struct of_device_id smb358_i2c_match_table[] = {
	{ .compatible = "smb358,i2c", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, smb358_i2c_match_table);

static struct i2c_driver smb358_charger_driver = {
	.driver = {
		.name	= "smb358",
		.owner = THIS_MODULE,
		.of_match_table = smb358_i2c_match_table,
	},
	.probe	= smb358_charger_probe,
	.remove	= __devexit_p(smb358_charger_remove),
	.suspend	= smb358_charger_suspend,
	.resume		= smb358_charger_resume,
	.shutdown	= smb358_charger_shutdown,
	.id_table	= smb358_charger_id,
};

static int __init smb358_charger_init(void)
{
	return i2c_add_driver(&smb358_charger_driver);
}

static void __exit smb358_charger_exit(void)
{
	i2c_del_driver(&smb358_charger_driver);
}

module_init(smb358_charger_init);
module_exit(smb358_charger_exit);

MODULE_DESCRIPTION("Samsung SMB358 Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");

