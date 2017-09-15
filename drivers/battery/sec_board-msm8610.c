/*
 *  sec_board-msm8610.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/battery/sec_battery.h>
#include <linux/battery/sec_fuelgauge.h>
#include <linux/battery/sec_charging_common.h>

#include <linux/qpnp/pin.h>
#include <linux/qpnp/qpnp-adc.h>
#if defined(CONFIG_EXTCON)
int current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
#else
extern int current_cable_type;
#endif
extern unsigned int system_rev;

#if defined(CONFIG_FUELGAUGE_MAX17048)
static struct battery_data_t samsung_battery_data[] = {
	/* SDI battery data (High voltage 4.35V) */
	{
#if defined(CONFIG_MACH_HEAT_DYN)
		.RCOMP0 = 0x5C,
		.RCOMP_charging = 0x5C,
		.temp_cohot = -1000,
		.temp_cocold = -4350,
		.is_using_model_data = true,
		.type_str = "SDI",
#else
		.RCOMP0 = 0x73,
		.RCOMP_charging = 0x8D,
		.temp_cohot = -1000,
		.temp_cocold = -4350,
		.is_using_model_data = true,
		.type_str = "SDI",
#endif
	}
};
#elif defined(CONFIG_FUELGAUGE_MAX17050)
static struct battery_data_t samsung_battery_data[] = {
	/* SDI battery data (High voltage 4.35V) */
	{
#if defined(CONFIG_MACH_PICASSO)
		.Capacity = 0x3F76, /* N1/N2: 8123mAh */
#elif defined(CONFIG_MACH_MONDRIAN)
		.Capacity = 0x2456, /* Mondrian : 4651mAh */
#else
		.Capacity = 0x4A38, /* V1/V2: 9500mAh */
#endif
		.low_battery_comp_voltage = 3500,
		.low_battery_table = {
			/* range, slope, offset */
			{-5000,	0,	0},	/* dummy for top limit */
			{-1250, 0,	3320},
			{-750, 97,	3451},
			{-100, 96,	3461},
			{0, 0,	3456},
		},
		.temp_adjust_table = {
			/* range, slope, offset */
			{47000, 122,	8950},
			{60000, 200,	51000},
			{100000, 0,	0},	/* dummy for top limit */
		},
		.type_str = "SDI",
	}
};
#else
static void * samsung_battery_data;
#endif

#define CAPACITY_MAX			1000
#define CAPACITY_MAX_MARGIN	50
#define CAPACITY_MIN			0

//static struct qpnp_vadc_chip *adc_client;
static enum qpnp_vadc_channels temp_channel;
static struct sec_fuelgauge_info *sec_fuelgauge =  NULL;


#if defined(CONFIG_MACH_HEAT_DYN)
static sec_bat_adc_table_data_t temp_table[] = {
	{25898,	900},
	{26106,	850},
	{26351, 800},
	{26641,	750},
	{26982,	700},
	{27382,	650},
	{27839,	600},
	{28376,	550},
	{28999,	500},
	{29709,	450},
	{30849,	400},
	{31813,	350},
	{32821,	300},
	{33881,	250},
	{35007,	200},
	{36117,	150},
	{37207,	100},
	{38228,	50},
	{39170,	0},
	{39670,	-50},
	{40436,	-100},
	{41105,	-150},
	{41653,	-200},
};
#elif defined(CONFIG_SEC_KANAS_PROJECT)
static sec_bat_adc_table_data_t temp_table[] = {
	{29153, 600},
	{29859, 550},
	{30576, 500},
	{31518, 450},
	{32491, 400},
	{33484, 350},
	{34508, 300},
	{35532, 250},
	{36556, 200},
	{37509, 150},
	{38481, 100},
	{39249, 50},
	{40017, 0},
	{40591, -50},
	{41154, -100},
	{41543, -150},
	{41902, -200},
};
#else
static sec_bat_adc_table_data_t temp_table[] = {
    {27281, 700},
    {27669, 650},
    {28178, 600},
    {28724, 550},
    {29342, 500},
    {30101, 450},
    {30912, 400},
    {31807, 350},
    {32823, 300},
    {33858, 250},
    {34950, 200},
    {36049, 150},
    {37054, 100},
    {38025, 50},
    {38219, 40},
    {38448, 30},
    {38626, 20},
    {38795, 10},
    {38989, 0},
    {39229, -10},
    {39540, -30},
    {39687, -40},
    {39822, -50},
    {40523, -100},
    {41123, -150},
    {41619, -200},
};
#endif

#if defined(CONFIG_MACH_HEAT_DYN)
#define TEMP_HIGH_THRESHOLD_EVENT	500
#define TEMP_HIGH_RECOVERY_EVENT		450
#define TEMP_LOW_THRESHOLD_EVENT		-50
#define TEMP_LOW_RECOVERY_EVENT		0
#define TEMP_HIGH_THRESHOLD_NORMAL	500
#define TEMP_HIGH_RECOVERY_NORMAL	450
#define TEMP_LOW_THRESHOLD_NORMAL	-50
#define TEMP_LOW_RECOVERY_NORMAL	0
#define TEMP_HIGH_THRESHOLD_LPM		500
#define TEMP_HIGH_RECOVERY_LPM		450
#define TEMP_LOW_THRESHOLD_LPM		-50
#define TEMP_LOW_RECOVERY_LPM		0
#elif defined(CONFIG_SEC_KANAS_PROJECT)
#define TEMP_HIGH_THRESHOLD_EVENT	600
#define TEMP_HIGH_RECOVERY_EVENT	460
#define TEMP_LOW_THRESHOLD_EVENT	-50
#define TEMP_LOW_RECOVERY_EVENT	0
#define TEMP_HIGH_THRESHOLD_NORMAL	600
#define TEMP_HIGH_RECOVERY_NORMAL	460
#define TEMP_LOW_THRESHOLD_NORMAL	-50
#define TEMP_LOW_RECOVERY_NORMAL	0
#define TEMP_HIGH_THRESHOLD_LPM		600
#define TEMP_HIGH_RECOVERY_LPM		460
#define TEMP_LOW_THRESHOLD_LPM		-50
#define TEMP_LOW_RECOVERY_LPM		0
#else
#define TEMP_HIGH_THRESHOLD_EVENT	600
#define TEMP_HIGH_RECOVERY_EVENT	460
#define TEMP_LOW_THRESHOLD_EVENT	-50
#define TEMP_LOW_RECOVERY_EVENT		0
#define TEMP_HIGH_THRESHOLD_NORMAL	600
#define TEMP_HIGH_RECOVERY_NORMAL	460
#define TEMP_LOW_THRESHOLD_NORMAL	-50
#define TEMP_LOW_RECOVERY_NORMAL	0
#define TEMP_HIGH_THRESHOLD_LPM		600
#define TEMP_HIGH_RECOVERY_LPM		460
#define TEMP_LOW_THRESHOLD_LPM		-50
#define TEMP_LOW_RECOVERY_LPM		0
#endif
void sec_bat_check_batt_id(struct sec_battery_info *battery)
{
#if defined(CONFIG_SENSORS_QPNP_ADC_VOLTAGE)
	int rc, data =  -1;
	struct qpnp_vadc_result results;

	rc = qpnp_vadc_read(NULL, LR_MUX2_BAT_ID, &results);
	if (rc) {
		pr_err("%s: Unable to read batt id rc=%d\n",
				__func__, rc);
		return;
	}
	data = results.adc_code;

	pr_info("%s: batt_id_adc = (%d)\n", __func__, data);
#endif
}

static void sec_bat_adc_ap_init(struct platform_device *pdev,
			struct sec_battery_info *battery)
{
#if defined(CONFIG_SEC_KANAS_PROJECT)
	temp_channel = P_MUX3_1_1;
#else
    temp_channel = LR_MUX1_BATT_THERM;
#endif
	pr_info("%s :  temp_channel = %d\n", __func__,temp_channel);
}

static int sec_bat_adc_ap_read(struct sec_battery_info *battery, int channel)
{
	struct qpnp_vadc_result results;
	int rc = -1;
	int data = -1;

	switch (channel)
	{
	case SEC_BAT_ADC_CHANNEL_TEMP :
		rc = qpnp_vadc_read(NULL, temp_channel, &results);
		if (rc) {
			pr_err("%s: Unable to read batt temperature rc=%d\n",
				__func__, rc);
			return 0;
		}
		data = results.adc_code;
		break;
	case SEC_BAT_ADC_CHANNEL_TEMP_AMBIENT:
		data = 33000;
		break;
	case SEC_BAT_ADC_CHANNEL_BAT_CHECK :
		rc = qpnp_vadc_read(NULL, LR_MUX2_BAT_ID, &results);
        if (rc) {
			pr_err("%s: Unable to read BATT_ID ADC rc=%d\n",
				__func__, rc);
			return 0;
		}
		data = ((int)results.physical) / 1000;
		break;
	default :
		break;
	}

	pr_debug("%s: data(%d)\n", __func__, data);

	return data;
}

static void sec_bat_adc_ap_exit(void)
{
}

static void sec_bat_adc_none_init(struct platform_device *pdev,
			struct sec_battery_info *battery)
{
}

static int sec_bat_adc_none_read(struct sec_battery_info *battery, int channel)
{
	return 0;
}

static void sec_bat_adc_none_exit(void)
{
}

static void sec_bat_adc_ic_init(struct platform_device *pdev,
			struct sec_battery_info *battery)
{
}

static int sec_bat_adc_ic_read(struct sec_battery_info *battery, int channel)
{
	return 0;
}

static void sec_bat_adc_ic_exit(void)
{
}
static int adc_read_type(struct sec_battery_info *battery, int channel)
{
	int adc = 0;

	switch (battery->pdata->temp_adc_type)
	{
	case SEC_BATTERY_ADC_TYPE_NONE :
		adc = sec_bat_adc_none_read(battery, channel);
		break;
	case SEC_BATTERY_ADC_TYPE_AP :
		adc = sec_bat_adc_ap_read(battery, channel);
		break;
	case SEC_BATTERY_ADC_TYPE_IC :
		adc = sec_bat_adc_ic_read(battery, channel);
		break;
	case SEC_BATTERY_ADC_TYPE_NUM :
		break;
	default :
		break;
	}
	pr_debug("[%s] ADC = %d\n", __func__, adc);
	return adc;
}

static void adc_init_type(struct platform_device *pdev,
			struct sec_battery_info *battery)
{
	switch (battery->pdata->temp_adc_type)
	{
	case SEC_BATTERY_ADC_TYPE_NONE :
		sec_bat_adc_none_init(pdev, battery);
		break;
	case SEC_BATTERY_ADC_TYPE_AP :
		sec_bat_adc_ap_init(pdev, battery);
		break;
	case SEC_BATTERY_ADC_TYPE_IC :
		sec_bat_adc_ic_init(pdev, battery);
		break;
	case SEC_BATTERY_ADC_TYPE_NUM :
		break;
	default :
		break;
	}
}

static void adc_exit_type(struct sec_battery_info *battery)
{
	switch (battery->pdata->temp_adc_type)
	{
	case SEC_BATTERY_ADC_TYPE_NONE :
		sec_bat_adc_none_exit();
		break;
	case SEC_BATTERY_ADC_TYPE_AP :
		sec_bat_adc_ap_exit();
		break;
	case SEC_BATTERY_ADC_TYPE_IC :
		sec_bat_adc_ic_exit();
		break;
	case SEC_BATTERY_ADC_TYPE_NUM :
		break;
	default :
		break;
	}
}

int adc_read(struct sec_battery_info *battery, int channel)
{
	int adc = 0;

	adc = adc_read_type(battery, channel);

	pr_info("[%s]adc = %d\n", __func__, adc);

	return adc;
}

void adc_exit(struct sec_battery_info *battery)
{
	adc_exit_type(battery);
}

bool sec_bat_check_jig_status(void)
{
    return (current_cable_type == POWER_SUPPLY_TYPE_UARTOFF);
}

/* callback for battery check
 * return : bool
 * true - battery detected, false battery NOT detected
 */
bool sec_bat_check_callback(struct sec_battery_info *battery)
{
	return true;
}

bool sec_bat_check_cable_result_callback(
		int cable_type)
{
	return true;
}

int sec_bat_check_cable_callback(struct sec_battery_info *battery)
{
	union power_supply_propval value;

	if (battery->pdata->ta_irq_gpio == 0) {
		pr_err("%s: ta_int_gpio is 0 or not assigned yet(cable_type(%d))\n",
			__func__, current_cable_type);
	} else {
		if (current_cable_type == POWER_SUPPLY_TYPE_BATTERY &&
			!gpio_get_value_cansleep(battery->pdata->ta_irq_gpio)) {
			pr_info("%s : VBUS IN\n", __func__);
			psy_do_property("battery", set, POWER_SUPPLY_PROP_ONLINE, value);
			return POWER_SUPPLY_TYPE_UARTOFF;
		}

		if ((current_cable_type == POWER_SUPPLY_TYPE_UARTOFF ||
			current_cable_type == POWER_SUPPLY_TYPE_CARDOCK) &&
			gpio_get_value_cansleep(battery->pdata->ta_irq_gpio)) {
			pr_info("%s : VBUS OUT\n", __func__);
			psy_do_property("battery", set, POWER_SUPPLY_PROP_ONLINE, value);
			return POWER_SUPPLY_TYPE_BATTERY;
		}
	}

	return current_cable_type;
}

void board_battery_init(struct platform_device *pdev, struct sec_battery_info *battery)
{
	if ((!battery->pdata->temp_adc_table) &&
		(battery->pdata->thermal_source == SEC_BATTERY_THERMAL_SOURCE_ADC)) {
		pr_info("%s : assign temp adc table\n", __func__);

		battery->pdata->temp_adc_table = temp_table;
		battery->pdata->temp_amb_adc_table = temp_table;

		battery->pdata->temp_adc_table_size = sizeof(temp_table)/sizeof(sec_bat_adc_table_data_t);
		battery->pdata->temp_amb_adc_table_size = sizeof(temp_table)/sizeof(sec_bat_adc_table_data_t);
	}

	battery->pdata->temp_high_threshold_event = TEMP_HIGH_THRESHOLD_EVENT;
	battery->pdata->temp_high_recovery_event = TEMP_HIGH_RECOVERY_EVENT;
	battery->pdata->temp_low_threshold_event = TEMP_LOW_THRESHOLD_EVENT;
	battery->pdata->temp_low_recovery_event = TEMP_LOW_RECOVERY_EVENT;
	battery->pdata->temp_high_threshold_normal = TEMP_HIGH_THRESHOLD_NORMAL;
	battery->pdata->temp_high_recovery_normal = TEMP_HIGH_RECOVERY_NORMAL;
	battery->pdata->temp_low_threshold_normal = TEMP_LOW_THRESHOLD_NORMAL;
	battery->pdata->temp_low_recovery_normal = TEMP_LOW_RECOVERY_NORMAL;
	battery->pdata->temp_high_threshold_lpm = TEMP_HIGH_THRESHOLD_LPM;
	battery->pdata->temp_high_recovery_lpm = TEMP_HIGH_RECOVERY_LPM;
	battery->pdata->temp_low_threshold_lpm = TEMP_LOW_THRESHOLD_LPM;
	battery->pdata->temp_low_recovery_lpm = TEMP_LOW_RECOVERY_LPM;
	adc_init_type(pdev, battery);
}

void board_fuelgauge_init(struct sec_fuelgauge_info *fuelgauge)
{
	sec_fuelgauge = fuelgauge;

	if (!fuelgauge->pdata->battery_data) {
		pr_info("%s : assign battery data\n", __func__);
			fuelgauge->pdata->battery_data = (void *)samsung_battery_data;
	}
	fuelgauge->pdata->capacity_max = CAPACITY_MAX;
	fuelgauge->pdata->capacity_max_margin = CAPACITY_MAX_MARGIN;
	fuelgauge->pdata->capacity_min = CAPACITY_MIN;

#if defined(CONFIG_FUELGAUGE_MAX17048)
	pr_info("%s: RCOMP0: 0x%x, RCOMP_charging: 0x%x, "
		"temp_cohot: %d, temp_cocold: %d, "
		"is_using_model_data: %d, type_str: %s, "
		"capacity_max: %d, capacity_max_margin: %d, "
		"capacity_min: %d, \n", __func__ ,
		get_battery_data(fuelgauge).RCOMP0,
		get_battery_data(fuelgauge).RCOMP_charging,
		get_battery_data(fuelgauge).temp_cohot,
		get_battery_data(fuelgauge).temp_cocold,
		get_battery_data(fuelgauge).is_using_model_data,
		get_battery_data(fuelgauge).type_str,
		fuelgauge->pdata->capacity_max,
		fuelgauge->pdata->capacity_max_margin,
		fuelgauge->pdata->capacity_min
		);
#endif
}

void cable_initial_check(struct sec_battery_info *battery)
{
	union power_supply_propval value;

	pr_info("%s : current_cable_type : (%d)\n", __func__, current_cable_type);
	if (POWER_SUPPLY_TYPE_BATTERY != current_cable_type) {
		value.intval = current_cable_type;
		psy_do_property("battery", set,
				POWER_SUPPLY_PROP_ONLINE, value);
	} else {
		psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_PROP_ONLINE, value);
		if (value.intval == POWER_SUPPLY_TYPE_WIRELESS) {
			value.intval =
			POWER_SUPPLY_TYPE_WIRELESS;
			psy_do_property("battery", set,
					POWER_SUPPLY_PROP_ONLINE, value);
		}
	}
}
