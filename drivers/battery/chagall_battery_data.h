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
		.Capacity = 0x3CD8, /* CHAGALL : 7788mAh */
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
	{668,	900},
	{657,	800},
	{613,   700},
	{556,   600},
	{486,   500},
	{402,   400},
	{306,   300},
	{199,	200},
	{90,	100},
	{-3,	0},
	{-93,  -100},
	{-161,  -200},
	{-350,  -400},
};

static sec_bat_adc_table_data_t chg_temp_table[] = {
	{0, 0},
};

#if defined(CONFIG_MACH_CHAGALL_SPR) || defined(CONFIG_MACH_CHAGALL_USC)
#define TEMP_HIGH_THRESHOLD_EVENT	560
#define TEMP_HIGH_RECOVERY_EVENT	480
#define TEMP_LOW_THRESHOLD_EVENT	-50
#define TEMP_LOW_RECOVERY_EVENT		0
#define TEMP_HIGH_THRESHOLD_NORMAL	560
#define TEMP_HIGH_RECOVERY_NORMAL	480
#define TEMP_LOW_THRESHOLD_NORMAL	-50
#define TEMP_LOW_RECOVERY_NORMAL	0
#define TEMP_HIGH_THRESHOLD_LPM		520
#define TEMP_HIGH_RECOVERY_LPM		472
#define TEMP_LOW_THRESHOLD_LPM		10
#define TEMP_LOW_RECOVERY_LPM		25
#else //CONFIG_MACH_CHAGALL_VZW
#define TEMP_HIGH_THRESHOLD_EVENT	567
#define TEMP_HIGH_RECOVERY_EVENT	480
#define TEMP_LOW_THRESHOLD_EVENT	-50
#define TEMP_LOW_RECOVERY_EVENT		0
#define TEMP_HIGH_THRESHOLD_NORMAL	567
#define TEMP_HIGH_RECOVERY_NORMAL	480
#define TEMP_LOW_THRESHOLD_NORMAL	-50
#define TEMP_LOW_RECOVERY_NORMAL	0
#define TEMP_HIGH_THRESHOLD_LPM		520
#define TEMP_HIGH_RECOVERY_LPM		472
#define TEMP_LOW_THRESHOLD_LPM		-20
#define TEMP_LOW_RECOVERY_LPM		0
#endif

#endif /* __SEC_BATTERY_DATA_H */
