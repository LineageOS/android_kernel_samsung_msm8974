/*
 *  max17048_fuelgauge.c
 *  Samsung MAX17048 Fuel Gauge Driver
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

#include <linux/battery/sec_fuelgauge.h>
extern int poweroff_charging;

#if defined(CONFIG_SEC_K_PROJECT)
u16 model_data_active_1[8] = {
		0xAB40, 0xB640, 0xBAF0, 0xBC90,
		0xBD00, 0xBE10,	0xBF40, 0xC230
};
u16 model_data_active_2[8] = {
		0xC570, 0xC850, 0xCAB0, 0xCD20,
		0xD110, 0xD510, 0xD6C0, 0xDA00
};
u16 model_data_active_3[8] = {
		0x01C0, 0x19C0, 0x22E0, 0x7700,
		0x3320, 0x3CC0, 0x1900, 0x0E20
};
u16 model_data_active_4[8] = {
		0x1380, 0x1240, 0x1200, 0x0D40,
		0x0D20, 0x0440,	0x10E0, 0x10E0
};

u16 model_rcomp_seg[8] = {0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080};
#elif defined(CONFIG_SEC_S_PROJECT)
u16 model_data_active_1[8] = {
		0xA4F0, 0xB7A0, 0xB810, 0xB870,
		0xBB20, 0xBB90, 0xBCA0, 0xBDF0
};
u16 model_data_active_2[8] = {
		0xC010, 0xC330, 0xC610, 0xC930,
		0xCCE0, 0xCFE0, 0xD320, 0xD9D0
};
u16 model_data_active_3[8] = {
		0x0170, 0x44F0, 0x5940, 0x1200,
		0x2320, 0x5180, 0x3DF0, 0x26D0
};
u16 model_data_active_4[8] = {
		0x0FF0, 0x17E0, 0x10F0, 0x0F10,
		0x0E00, 0x0E20, 0x0C00, 0x0C00
};

u16 model_rcomp_seg[8] = {0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080};
#endif


#if 0
static int max17048_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int max17048_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}
#endif
static int max17048_write_word(struct i2c_client *client, int reg, u16 buf)
{
	int ret;

	ret = i2c_smbus_write_word_data(client, reg, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int max17048_read_word(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_word_data(client, reg);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static void max17048_reset(struct i2c_client *client)
{
	u16 reset_cmd;

	reset_cmd = swab16(0x4000);

	i2c_smbus_write_word_data(client, MAX17048_MODE_MSB, reset_cmd);

	msleep(300);
}

#if defined(CONFIG_SEC_K_PROJECT) || defined(CONFIG_SEC_S_PROJECT)
static bool max17048_set_modeldata(struct i2c_client *client)
{
	int temp, i = 0;
	u16 data;
	u16 read_ocv = 0;
	u8 read_soc[2] = {0, 0};
	u8 config[2];
	u32 addr;
	union power_supply_propval value;
	struct sec_fuelgauge_info *fuelgauge =
		i2c_get_clientdata(client);

	do {
		/* 1. Unlock Model Access */
		addr = 0x3E;
		data = 0x4A57; /* Send 0x4A, and then send 0x57*/
		max17048_write_word(client, addr, swab16(data));

		/* 2. Read Config and OCV */
		addr = 0x0C;
		temp = max17048_read_word(client, addr);
		config[1] = temp & 0xff;
		config[0] = (temp & 0xff00) >> 8;

		pr_info("%s : fuelguage : OriginalRCOMP = 0x%x, OriginalAlert = 0x%x\n",
			__func__, config[1], config[0]);

		addr = 0x0E;
		read_ocv = max17048_read_word(client, addr);
		pr_info("%s : fuelguage : OCV_DATA = 0x%4x\n", __func__,  read_ocv);
		/* 2-5. Verify Model Access unlocked */
	} while((read_ocv == 0xFFFF) && (i++ < 10));

	/* 3. Write OCV */
	/* 4. Write RCOMP to a Maximum value of 0xFF00 */
	/* 5. Write the Model */
	/* Once the model is unlocked, the host s/w must write 64 bytes model to the max17048.
	*		The model is located between locations 0x40 ~ 0x7F. */
	for (i=0; i<8; i++) {
		addr = 0x40 + i*2;
		max17048_write_word(client, addr, swab16(model_data_active_1[i]));
	}
	for (i=0; i<8; i++) {
		addr = 0x50 + i*2;
		max17048_write_word(client, addr, swab16(model_data_active_2[i]));
	}
	for (i=0; i<8; i++) {
		addr = 0x60 + i*2;
		max17048_write_word(client, addr, swab16(model_data_active_3[i]));
	}
	for (i=0; i<8; i++) {
		addr = 0x70 + i*2;
		max17048_write_word(client, addr, swab16(model_data_active_4[i]));
	}

	/*overwrite RCOMP Seg to default values*/
	for (i=0; i<8; i++) {
		addr = 0x80 + i*2;
		max17048_write_word(client, addr, swab16(model_rcomp_seg[i]));
	}
	for (i=0; i<8; i++) {
		addr = 0x90 + i*2;
		max17048_write_word(client, addr, swab16(model_rcomp_seg[i]));
	}

	/* 6. Delay at least 150msec */
	/* 7. Write OCV */
	addr = 0x0E;
#if defined(CONFIG_SEC_K_PROJECT)
	data = 0xE400;
#elif defined(CONFIG_SEC_S_PROJECT)
	data = 0xE3D0;
#endif
	max17048_write_word(client, addr, swab16(data));

	/* 7-1. Disable Hibernation */
	addr = 0x0A;
	data = 0x0000;
	max17048_write_word(client, addr, swab16(data));

	/* 7-2. Lock Model Acess */
	addr = 0x3E;
	data = 0x0000;
	max17048_write_word(client, addr, swab16(data));

	/* 8. Delay between 150msec and 600msec */
	/* This delay must be between 150msec and 600msec.
	*		Delaying beyond 600msec could cause the verification to fail. */
	mdelay(200);

	/* 9. Read SOC Register and Compare to expected result */
	addr = 0x04;
	temp = max17048_read_word(client, addr);
	read_soc[1] = temp & 0xff;
	read_soc[0] = (temp & 0xff00) >> 8;
	pr_info("%s : reg = 0x%4x, SOC_DATA1 = 0x%x, SOC_DATA2 = 0x%x\n",
		__func__, temp, read_soc[1], read_soc[0]);

#if defined(CONFIG_SEC_K_PROJECT)
	if ((read_soc[1] >= 0xF2) && (read_soc[1] <= 0xF4)) {
#elif defined(CONFIG_SEC_S_PROJECT)
	if ((read_soc[1] >= 0xE4) && (read_soc[1] <= 0xE6)) {
#endif
		pr_info("%s : model was loaded successful\n", __func__);
	} else {
		pr_err("%s : error! model was not loaded successful\n", __func__);
		return false;
	}

	/* 9-1.  Unlock Model Access */
	addr = 0x3E;
	data = 0x4A57;
	max17048_write_word(client, addr, swab16(data));

	/* 10. Restore CONFIG and OCV */
	addr = 0x0C;
	psy_do_property("battery", get,
		POWER_SUPPLY_PROP_STATUS, value);
	if (value.intval == POWER_SUPPLY_STATUS_CHARGING) /* in charging */
		config[1] = get_battery_data(fuelgauge).RCOMP_charging;
	else
		config[1] = get_battery_data(fuelgauge).RCOMP0;

	data = (0x1C | (config[1] & 0xff) << 8);
	pr_info("%s : RCOMP(0x%4x) is applied\n", __func__, data);
	max17048_write_word(client, addr, swab16(data));

	addr = 0x0E;
	max17048_write_word(client, addr, swab16(read_ocv));

	read_ocv = 0;
	read_ocv = max17048_read_word(client, addr);
	pr_info("%s : OCV_DATA = 0x%4x\n", __func__, read_ocv);

	/* 11. Lock Model Acess */
	addr = 0x3E;
	data = 0x0000;
	max17048_write_word(client, addr, swab16(data));

	/* 11-1. Delay at least 150msec */
	mdelay(200);

	/* 12. Set Voltage Detector threshold */
	addr = 0x18;
	data = 0x7D7D;
	max17048_write_word(client, addr, swab16(data));

	/* 13. Set hibernation*/
	addr = 0x0A;
	data = 0x0000;
	max17048_write_word(client, addr, swab16(data));

	/* 14. Set vart */
	addr = 0x14;
	data = 0x00FF;
	max17048_write_word(client, addr, swab16(data));

	/* 15. Set Vreset */
	addr = 0x18;
	data = 0x7D7D;
	max17048_write_word(client, addr, swab16(data));

	pr_info("%s : load model data complete!\n", __func__);

	return true;
}
#endif

static int max17048_get_vcell(struct i2c_client *client)
{
	u32 vcell;
	u16 w_data;
	u32 temp;

	temp = max17048_read_word(client, MAX17048_VCELL_MSB);

	w_data = swab16(temp);

	temp = ((w_data & 0xFFF0) >> 4) * 1250;
	vcell = temp / 1000;

	dev_dbg(&client->dev,
		"%s : vcell (%d)\n", __func__, vcell);

	return vcell;
}

static int max17048_get_avg_vcell(struct i2c_client *client)
{
	u32 vcell_data = 0;
	u32 vcell_max = 0;
	u32 vcell_min = 0;
	u32 vcell_total = 0;
	u32 i;

	for (i = 0; i < AVER_SAMPLE_CNT; i++) {
		vcell_data = max17048_get_vcell(client);

		if (i != 0) {
			if (vcell_data > vcell_max)
				vcell_max = vcell_data;
			else if (vcell_data < vcell_min)
				vcell_min = vcell_data;
		} else {
			vcell_max = vcell_data;
			vcell_min = vcell_data;
		}
		vcell_total += vcell_data;
	}

	return (vcell_total - vcell_max - vcell_min) / (AVER_SAMPLE_CNT-2);
}

static int max17048_get_ocv(struct i2c_client *client)
{
/*
	u32 ocv;
	u16 w_data;
	u32 temp;
	u16 cmd;

	cmd = swab16(0x4A57);
	max17048_write_word(client, 0x3E, cmd);

	temp = max17048_read_word(client, MAX17048_OCV_MSB);

	w_data = swab16(temp);

	temp = ((w_data & 0xFFF0) >> 4) * 1250;
	ocv = temp / 1000;

	cmd = swab16(0x0000);
	max17048_write_word(client, 0x3E, cmd);

	dev_dbg(&client->dev,
			"%s : ocv (%d)\n", __func__, ocv);

	return ocv;
*/
	return 1;
}

/* soc should be 0.01% unit */
static int max17048_get_soc(struct i2c_client *client)
{
	struct sec_fuelgauge_info *fuelgauge =
				i2c_get_clientdata(client);
	u8 data[2] = {0, 0};
	int temp, soc;
	u64 psoc64 = 0;
	u64 temp64;
	u32 divisor = 10000000;

	temp = max17048_read_word(client, MAX17048_SOC_MSB);

	if (get_battery_data(fuelgauge).is_using_model_data) {
		/* [ TempSOC = ((SOC1 * 256) + SOC2) * 0.001953125 ] */
		temp64 = swab16(temp);
		psoc64 = temp64 * 1953125;
		psoc64 = div_u64(psoc64, divisor);
		soc = psoc64 & 0xffff;
	} else {
		data[0] = temp & 0xff;
		data[1] = (temp & 0xff00) >> 8;

		soc = (data[0] * 100) + (data[1] * 100 / 256);
	}

	dev_info(&client->dev,
		"%s : raw capacity (%d), data(0x%04x)\n",
		__func__, soc, (data[0]<<8) | data[1]);

	return soc;
}

static int max17048_get_current(struct i2c_client *client)
{
	struct sec_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);
	union power_supply_propval value;

	psy_do_property(fuelgauge->pdata->charger_name, get,
		POWER_SUPPLY_PROP_CURRENT_NOW, value);

	return value.intval;
}

#define DISCHARGE_SAMPLE_CNT 20
static int discharge_cnt=0;
static int all_vcell[20] = {0,};

/* if ret < 0, discharge */
static int sec_bat_check_discharge(int vcell)
{
	int i, cnt, ret = 0;

	all_vcell[discharge_cnt++] = vcell;
	if (discharge_cnt >= DISCHARGE_SAMPLE_CNT)
		discharge_cnt = 0;

	cnt = discharge_cnt;

	/* check after last value is set */
	if (all_vcell[cnt] == 0)
		return 0;

	for (i = 0; i < DISCHARGE_SAMPLE_CNT; i++) {
		if (cnt == i)
			continue;
		if (all_vcell[cnt] > all_vcell[i])
			ret--;
		else
			ret++;
	}
	return ret;
}

/* judge power off or not by current_avg */
static int max17048_get_current_average(struct i2c_client *client)
{
	struct sec_fuelgauge_info *fuelgauge = i2c_get_clientdata(client);
	union power_supply_propval value_bat;
	union power_supply_propval value_chg;
	int vcell, soc, curr_avg;
	int check_discharge;

	psy_do_property(fuelgauge->pdata->charger_name, get,
		POWER_SUPPLY_PROP_CURRENT_NOW, value_chg);
	psy_do_property("battery", get,
		POWER_SUPPLY_PROP_HEALTH, value_bat);
	vcell = max17048_get_vcell(client);
	soc = max17048_get_soc(client) / 100;
	check_discharge = sec_bat_check_discharge(vcell);

	/* if 0% && under 3.4v && low power charging(1000mA), power off */
	if (!poweroff_charging && (soc <= 0) && (vcell < 3400) &&
			(check_discharge < 0) &&
			((value_chg.intval < 1000) ||
			((value_bat.intval == POWER_SUPPLY_HEALTH_OVERHEAT) ||
			(value_bat.intval == POWER_SUPPLY_HEALTH_COLD)))) {
		pr_info("%s: SOC(%d), Vnow(%d), Inow(%d)\n",
			__func__, soc, vcell, value_chg.intval);
		curr_avg = -1;
	} else {
		curr_avg = value_chg.intval;
	}

	return curr_avg;
}

void sec_bat_reset_discharge(struct i2c_client *client)
{
	int i;

	for (i = 0; i < DISCHARGE_SAMPLE_CNT ; i++)
		all_vcell[i] = 0;
	discharge_cnt = 0;
}

static void max17048_get_version(struct i2c_client *client)
{
	u16 w_data;
	int temp;

	temp = max17048_read_word(client, MAX17048_VER_MSB);

	w_data = swab16(temp);

	dev_info(&client->dev,
		"MAX17048 Fuel-Gauge Ver 0x%04x\n", w_data);
}

#if defined(CONFIG_SEC_K_PROJECT) || defined(CONFIG_SEC_S_PROJECT)
static bool max17048_check_fg_validity(struct i2c_client *client)
{
	int temp;
	u8 data;
	u16 w_data = 0;

	/* check status register Reset indicator bit*/
	temp = max17048_read_word(client, 0x1A);
	data = temp & 0xff;

	if (0/* data & 0x01 */) {
		dev_err(&client->dev,
			"%s : IC is not configured. init modeldata reg(0x%x)\n", __func__, data);
		if (!max17048_set_modeldata(client)) {
			dev_err(&client->dev,
				"%s : fail to set modeldata\n", __func__);
			return false;
		}
		w_data = data & 0xfe;
		max17048_write_word(client, 0x1A, swab16(w_data));
	}

	return true;
}
#endif

static u16 max17048_get_rcomp(struct i2c_client *client)
{
	u16 w_data;
	int temp;

	temp = max17048_read_word(client, MAX17048_RCOMP_MSB);

	w_data = swab16(temp);

	dev_dbg(&client->dev,
		"%s : current rcomp = 0x%04x\n",
		__func__, w_data);

	return w_data;
}

static void max17048_set_rcomp(struct i2c_client *client, u16 new_rcomp)
{
	i2c_smbus_write_word_data(client,
		MAX17048_RCOMP_MSB, swab16(new_rcomp));
}

static void max17048_rcomp_update(struct i2c_client *client, int temp)
{
	struct sec_fuelgauge_info *fuelgauge =
				i2c_get_clientdata(client);
	union power_supply_propval value;

	int starting_rcomp = 0;
	int new_rcomp = 0;
	int rcomp_current = 0;

	rcomp_current = max17048_get_rcomp(client);

	psy_do_property("battery", get,
		POWER_SUPPLY_PROP_STATUS, value);

	if (value.intval == POWER_SUPPLY_STATUS_CHARGING) /* in charging */
		starting_rcomp = get_battery_data(fuelgauge).RCOMP_charging;
	else
		starting_rcomp = get_battery_data(fuelgauge).RCOMP0;

	if (temp > RCOMP0_TEMP)
		new_rcomp = starting_rcomp + ((temp - RCOMP0_TEMP) *
			get_battery_data(fuelgauge).temp_cohot / 1000);
	else if (temp < RCOMP0_TEMP)
		new_rcomp = starting_rcomp + ((temp - RCOMP0_TEMP) *
			get_battery_data(fuelgauge).temp_cocold / 1000);
	else
		new_rcomp = starting_rcomp;

	if (new_rcomp > 255)
		new_rcomp = 255;
	else if (new_rcomp < 0)
		new_rcomp = 0;

	new_rcomp <<= 8;
	new_rcomp &= 0xff00;
	/* not related to RCOMP */
	new_rcomp |= (rcomp_current & 0xff);

	if (rcomp_current != new_rcomp) {
		dev_dbg(&client->dev,
			"%s : RCOMP 0x%04x -> 0x%04x (0x%02x)\n",
			__func__, rcomp_current, new_rcomp,
			new_rcomp >> 8);
		max17048_set_rcomp(client, new_rcomp);
	}
}

#ifdef CONFIG_OF
#if 0
static int max17048_parse_dt(struct device *dev,
			     struct sec_fuelgauge_info *fuelgauge)
{
	struct device_node *np = dev->of_node;
	int ret;
	int value;

	if (np == NULL) {
		pr_err("%s np NULL\n", __func__);
	} else {
		ret = of_property_read_u32(np, "fuelgauge,rcomp0",
					   &value);
		pr_err("%s value %d\n",
		       __func__, value);
		get_battery_data(fuelgauge).RCOMP0 = (u8)value;
		if (ret < 0)
			pr_err("%s error reading rcomp0 %d\n",
			       __func__, ret);
		ret = of_property_read_u32(np, "fuelgauge,rcomp_charging",
					   &value);
		pr_err("%s value %d\n",
		       __func__, value);
		get_battery_data(fuelgauge).RCOMP_charging = (u8)value;
		if (ret < 0)
			pr_err("%s error reading rcomp_charging %d\n",
			       __func__, ret);
		ret = of_property_read_u32(np, "fuelgauge,temp_cohot",
				   &get_battery_data(fuelgauge).temp_cohot);
		if (ret < 0)
			pr_err("%s error reading temp_cohot %d\n",
			       __func__, ret);
		ret = of_property_read_u32(np, "fuelgauge,temp_cocold",
				   &get_battery_data(fuelgauge).temp_cocold);
		if (ret < 0)
			pr_err("%s error reading temp_cocold %d\n",
			       __func__, ret);
		get_battery_data(fuelgauge).is_using_model_data = of_property_read_bool(np,
				"fuelgauge,is_using_model_data");
		ret = of_property_read_string(np, "fuelgauge,type_str",
				(const char **)&get_battery_data(fuelgauge).type_str);
		if (ret < 0)
			pr_err("%s error reading temp_cocold %d\n",
			       __func__, ret);

		pr_info("%s RCOMP0: 0x%x, RCOMP_charging: 0x%x, temp_cohot: %d,"
			"temp_cocold: %d, is_using_model_data: %d, "
			"type_str: %s,\n", __func__,
			get_battery_data(fuelgauge).RCOMP0,
			get_battery_data(fuelgauge).RCOMP_charging,
			get_battery_data(fuelgauge).temp_cohot,
			get_battery_data(fuelgauge).temp_cocold,
			get_battery_data(fuelgauge).is_using_model_data,
			get_battery_data(fuelgauge).type_str
			);
	}

	return 0;
}
#endif
#endif

static void fg_read_regs(struct i2c_client *client, char *str)
{
	int data = 0;
	u32 addr = 0;

	for (addr = 0x02; addr <= 0x04; addr += 2) {
		data = max17048_read_word(client, addr);
		sprintf(str + strlen(str), "0x%04x, ", data);
	}

	/* "#" considered as new line in application */
	sprintf(str+strlen(str), "#");

	for (addr = 0x08; addr <= 0x1a; addr += 2) {
		data = max17048_read_word(client, addr);
		sprintf(str + strlen(str), "0x%04x, ", data);
	}
}

/* read all the register */
static void fg_read_all_regs(struct i2c_client *client)
{
	int data = 0;
	u32 addr = 0;
	u8 addr1;
	u16 data1;
	u8 data2[2] = {0, 0};
	int i = 0;
	char *str = kzalloc(sizeof(char)*2048, GFP_KERNEL);
	int count = 0;

	addr1 = 0x3E;
	data1 = 0x4A57;
	max17048_write_word(client, addr1, swab16(data1));
	for (addr = 0x02; addr <= 0x1A; addr += 2) {
		count++;
		data = max17048_read_word(client, addr);
		data2[0] = data & 0xff;
		data2[1] = (data & 0xff00) >> 8;

		for (i = 0; i < 2; i++)	{
			sprintf(str + strlen(str), "0x%02x(0x%02x), ", addr+i, data2[i]);
		}

		if((count % 7) == 0)
			sprintf(str+strlen(str), "\n");
	}

	dev_info(&client->dev, "%s\n", str);
	str[0] = '\0';
	count = 0;
	for (addr = 0x40; addr <= 0x7F; addr += 2) {
		count++;
		data = max17048_read_word(client, addr);
		data2[0] = data & 0xff;
		data2[1] = (data & 0xff00) >> 8;

		for (i = 0; i < 2; i++)	{
			sprintf(str + strlen(str), "0x%02x(0x%02x), ", addr+i, data2[i]);
		}

		if((count % 7) == 0)
			sprintf(str+strlen(str), "\n");
	}
	dev_info(&client->dev, "%s\n", str);
	str[0] = '\0';
	count = 0;
	for (addr = 0x80; addr <= 0x9F; addr += 2) {
		count++;
		data = max17048_read_word(client, addr);
		data2[0] = data & 0xff;
		data2[1] = (data & 0xff00) >> 8;

		for (i = 0; i < 2; i++)	{
			sprintf(str + strlen(str), "0x%02x(0x%02x), ", addr+i, data2[i]);
		}

		if((count % 7) == 0)
			sprintf(str+strlen(str), "\n");
	}
	str[strlen(str)] = '\0';
	dev_info(&client->dev, "%s\n", str);

	addr1 = 0x3E;
	data1 = 0x0000;
	max17048_write_word(client, addr1, swab16(data1));
	kfree(str);
}

/* read the specific address */
#if 0
static void fg_read_address(struct i2c_client *client)
{
	int data = 0;
	u32 addr = 0;
	u8 data1[2] = {0, 0};
	int i = 0;
	char str[512] = {0,};
	int count = 0;

	for (addr = 0x02; addr <= 0x0d; addr += 2) {
		count++;
		data = max17048_read_word(client, addr);
		data1[0] = data & 0xff;
		data1[1] = (data & 0xff00) >> 8;

		for (i = 0; i < 2; i++)	{
			sprintf(str + strlen(str), "0x%02x(0x%02x), ", addr+i, data1[i]);
		}

		if((count % 7) == 0)
			sprintf(str+strlen(str), "\n");
	}

	dev_info(&client->dev, "%s\n", str);
	str[0] = '\0';

	count = 0;
	for (addr = 0x14; addr <= 0x1b; addr += 2) {
		count++;
		data = max17048_read_word(client, addr);
		data1[0] = data & 0xff;
		data1[1] = (data & 0xff00) >> 8;

		for (i = 0; i < 2; i++)	{
			sprintf(str + strlen(str), "0x%02x(0x%02x), ", addr+i, data1[i]);
		}

		if((count % 7) == 0)
			sprintf(str+strlen(str), "\n");
	}
	str[strlen(str)] = '\0';
	dev_info(&client->dev, "%s\n", str);
}
#endif

bool sec_hal_fg_init(struct i2c_client *client)
{
#ifdef CONFIG_OF
#if 1
	struct sec_fuelgauge_info *fuelgauge =
		i2c_get_clientdata(client);

	board_fuelgauge_init(fuelgauge);
#else
	struct sec_fuelgauge_info *fuelgauge =
		i2c_get_clientdata(client);
	int error;

	error = max17048_parse_dt(&client->dev, fuelgauge);

	if (error) {
		dev_err(&client->dev,
			"%s : Failed to get max17048 fuel_init\n", __func__);
		return false;
	}
#endif
#endif
	max17048_get_version(client);

#if defined(CONFIG_SEC_K_PROJECT) || defined(CONFIG_SEC_S_PROJECT)
	if (!max17048_check_fg_validity(client))
		return false;
#endif
	return true;
}

bool sec_hal_fg_suspend(struct i2c_client *client)
{
	return true;
}

bool sec_hal_fg_resume(struct i2c_client *client)
{
	return true;
}

bool sec_hal_fg_fuelalert_init(struct i2c_client *client, int soc)
{
	u16 temp;
	u8 data;

	temp = max17048_get_rcomp(client);
	data = 32 - soc; /* set soc for fuel alert */
	temp &= 0xff00;
	temp += data;

	dev_dbg(&client->dev,
		"%s : new rcomp = 0x%04x\n",
		__func__, temp);

	max17048_set_rcomp(client, temp);

	return true;
}

bool sec_hal_fg_is_fuelalerted(struct i2c_client *client)
{
	u16 temp;

	temp = max17048_get_rcomp(client);

	if (temp & 0x20)	/* ALRT is asserted */
		return true;

	return false;
}

bool sec_hal_fg_fuelalert_process(void *irq_data, bool is_fuel_alerted)
{
	return true;
}

bool sec_hal_fg_full_charged(struct i2c_client *client)
{
	return true;
}

bool sec_hal_fg_reset(struct i2c_client *client)
{
	max17048_reset(client);
	return true;
}

bool sec_hal_fg_get_property(struct i2c_client *client,
			     enum power_supply_property psp,
			     union power_supply_propval *val)
{
	union power_supply_propval value_bat;

	switch (psp) {
		/* Cell voltage (VCELL, mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = max17048_get_vcell(client);
		break;
		/* Additional Voltage Information (mV) */
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
		switch (val->intval) {
		case SEC_BATTEY_VOLTAGE_AVERAGE:
			val->intval = max17048_get_avg_vcell(client);
			break;
		case SEC_BATTEY_VOLTAGE_OCV:
			val->intval = max17048_get_ocv(client);
			break;
		}
		break;
		/* Current (mA) */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		psy_do_property("battery", get,
			POWER_SUPPLY_PROP_STATUS, value_bat);
		if(value_bat.intval == POWER_SUPPLY_STATUS_DISCHARGING)
			val->intval = -max17048_get_current(client);
		else
			val->intval = max17048_get_current(client);
		break;
		/* Average Current (mA) */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = max17048_get_current_average(client);
		break;
		/* SOC (%) */
	case POWER_SUPPLY_PROP_CAPACITY:
		if (val->intval == SEC_FUELGAUGE_CAPACITY_TYPE_RAW)
			val->intval = max17048_get_soc(client);
		else
			val->intval = max17048_get_soc(client) / 10;
		/* fg_read_address(client); */
		break;
		/* Battery Temperature */
	case POWER_SUPPLY_PROP_TEMP:
		/* Target Temperature */
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		fg_read_all_regs(client);
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		break;

	default:
		return false;
	}
	return true;
}

bool sec_hal_fg_set_property(struct i2c_client *client,
			     enum power_supply_property psp,
			     const union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		sec_bat_reset_discharge(client);
		break;
		/* Battery Temperature */
	case POWER_SUPPLY_PROP_TEMP:
		/* temperature is 0.1 degree, should be divide by 10 */
		max17048_rcomp_update(client, val->intval / 10);
		break;
		/* Target Temperature */
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		break;
	default:
		return false;
	}
	return true;
}

ssize_t sec_hal_fg_show_attrs(struct device *dev,
				const ptrdiff_t offset, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_fuelgauge_info *fg =
		container_of(psy, struct sec_fuelgauge_info, psy_fg);
	int i = 0;
	char *str = NULL;

	switch (offset) {
	case FG_DATA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%02x%02x\n",
			fg->reg_data[1], fg->reg_data[0]);
		break;
	case FG_REGS:
		str = kzalloc(sizeof(char)*1024, GFP_KERNEL);
		if (!str)
			return -ENOMEM;

		fg_read_regs(fg->client, str);
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

ssize_t sec_hal_fg_store_attrs(struct device *dev,
				const ptrdiff_t offset,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_fuelgauge_info *fg =
		container_of(psy, struct sec_fuelgauge_info, psy_fg);
	int ret = 0;
	int x = 0;
	u16 data;

	switch (offset) {
	case FG_REG:
		if (sscanf(buf, "%x\n", &x) == 1) {
			fg->reg_addr = x;
			data = max17048_read_word(
				fg->client, fg->reg_addr);
			fg->reg_data[0] = (data & 0xff00) >> 8;
			fg->reg_data[1] = (data & 0x00ff);

			dev_dbg(&fg->client->dev,
				"%s: (read) addr = 0x%x, data = 0x%02x%02x\n",
				 __func__, fg->reg_addr,
				 fg->reg_data[1], fg->reg_data[0]);
			ret = count;
		}
		break;
	case FG_DATA:
		if (sscanf(buf, "%x\n", &x) == 1) {
			dev_dbg(&fg->client->dev,
				"%s: (write) addr = 0x%x, data = 0x%04x\n",
				__func__, fg->reg_addr, x);
			i2c_smbus_write_word_data(fg->client,
				fg->reg_addr, swab16(x));
			ret = count;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
