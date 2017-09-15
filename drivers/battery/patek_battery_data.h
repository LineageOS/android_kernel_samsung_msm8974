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
	{
		.RCOMP0 = 0x6B,
		.RCOMP_charging = 0x6B,
		.temp_cohot = -1175,
		.temp_cocold = -5675,
		.is_using_model_data = true,
		.type_str = "SDI",
	}
};

#define CAPACITY_MAX			990
#define CAPACITY_MAX_MARGIN	50
#define CAPACITY_MIN			-7

static sec_bat_adc_table_data_t temp_table[] = {
	{25970, 900},
	{26193, 850},
	{26454, 800},
	{26757, 750},
	{27098,	700},
	{27512,	650},
	{27984,	600},
	{28557,	550},
	{29213,	500},
	{29792,	460},
	{29950,	450},
	{30805,	400},
	{31676,	350},
	{32685,	300},
	{33727,	250},
	{34793,	200},
	{35848,	150},
	{36950,	100},
	{37977,	50},
	{38830,	0},
	{38970,	-10},
	{39173,	-20},
	{39322,	-30},
	{39637,	-50},
	{40366,	-100},
	{40964,	-150},
	{41461,	-200},
	{41830,	-250},
	{42074,	-300},
};

static sec_bat_adc_table_data_t chg_temp_table[] = {
	{0, 0},
};

#define TEMP_HIGHLIMIT_THRESHOLD_EVENT		800
#define TEMP_HIGHLIMIT_RECOVERY_EVENT		750
#define TEMP_HIGHLIMIT_THRESHOLD_NORMAL		800
#define TEMP_HIGHLIMIT_RECOVERY_NORMAL		750
#define TEMP_HIGHLIMIT_THRESHOLD_LPM		800
#define TEMP_HIGHLIMIT_RECOVERY_LPM			750

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

#endif /* __SEC_BATTERY_DATA_H */
