/*
 * battery_data.h
 * Samsung Mobile Battery data Header
 *
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#ifndef __SEC_BATTERY_DATA_H
#define __SEC_BATTERY_DATA_H __FILE__
static struct battery_data_t samsung_battery_data[] = {
	/* SDI battery data (High voltage 4.35V) */
	{
		.Capacity = 0x2710, /* KLIMT : 5000mAh */
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

#define CAPACITY_MAX			992
#define CAPACITY_MAX_MARGIN	50
#define CAPACITY_MIN			0

static sec_bat_adc_table_data_t temp_table[] = {
	{1100,	1100},
	{695,	900},
	{678,	850},
	{661,	800},
	{641,   750},
	{619,   700},
	{593,	650},
	{562,   600},
	{529,   550},
	{495,   500},
	{454,   450},
	{412,   400},
	{367,   350},
	{318,   300},
	{267,   250},
	{213,	200},
	{158,	150},
	{105,	100},
	{53,	50},
	{6,	0},
	{-40,  -50},
	{-71,  -100},
	{-120, -150},
	{-155,  -200},
	{-350,  -400},
};

static sec_bat_adc_table_data_t chg_temp_table[] = {
	{0, 0},
};

#define TEMP_HIGH_THRESHOLD_EVENT	540
#define TEMP_HIGH_RECOVERY_EVENT	470
#define TEMP_LOW_THRESHOLD_EVENT	-50
#define TEMP_LOW_RECOVERY_EVENT		0
#define TEMP_HIGH_THRESHOLD_NORMAL	540
#define TEMP_HIGH_RECOVERY_NORMAL	470
#define TEMP_LOW_THRESHOLD_NORMAL	-50
#define TEMP_LOW_RECOVERY_NORMAL	0
#define TEMP_HIGH_THRESHOLD_LPM		540
#define TEMP_HIGH_RECOVERY_LPM		480
#define TEMP_LOW_THRESHOLD_LPM		-40
#define TEMP_LOW_RECOVERY_LPM		0

#if defined(CONFIG_BATTERY_SWELLING)
#define BATT_SWELLING_HIGH_TEMP_BLOCK			450
#define BATT_SWELLING_HIGH_TEMP_RECOV			400
#define BATT_SWELLING_LOW_TEMP_BLOCK			100
#define BATT_SWELLING_LOW_TEMP_RECOV			150
#define BATT_SWELLING_HIGH_CHG_CURRENT			0
#define BATT_SWELLING_LOW_CHG_CURRENT			0
#define BATT_SWELLING_FULL_CHECK_CURRENT_2ND	0
#define BATT_SWELLING_DROP_FLOAT_VOLTAGE		4200
#define BATT_SWELLING_HIGH_RECHG_VOLTAGE		4150
#define BATT_SWELLING_LOW_RECHG_VOLTAGE			4050
#endif

#endif /* __SEC_BATTERY_DATA_H */
