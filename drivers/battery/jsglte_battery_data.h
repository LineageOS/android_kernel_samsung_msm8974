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
		.RCOMP0 = 0x6A,
		.RCOMP_charging = 0x7C,
		.temp_cohot = -700,
		.temp_cocold = -4875,
		.is_using_model_data = true,
		.type_str = "SDI",
	}
};

#define CAPACITY_MAX			990
#define CAPACITY_MAX_MARGIN	50
#define CAPACITY_MIN			-7

static sec_bat_adc_table_data_t temp_table[] = {
	{25400,	900},
	{25525,	850},
	{25660,	800},
	{25838,	750},
	{25991,	700},
	{26242,	650},
	{26549,	600},
	{26878,	550},
	{27278,	500},
	{27763,	450},
	{28358,	400},
	{29026,	350},
	{29807,	300},
	{30684,	250},
	{31671,	200},
	{32851,	150},
	{33976,	100},
	{35128,	50},
	{35341,	40},
	{35602,	30},
	{35823,	20},
	{36059,	10},
	{36302,	0},
	{36488,	-10},
	{36909,	-30},
	{37117,	-40},
	{37311,	-50},
	{38380,	-100},
	{39335,	-150},
	{40179,	-200},
};

static sec_bat_adc_table_data_t chg_temp_table[] = {
	{0, 0},
};

#define TEMP_HIGH_THRESHOLD_EVENT	600
#define TEMP_HIGH_RECOVERY_EVENT	400
#define TEMP_LOW_THRESHOLD_EVENT	-45
#define TEMP_LOW_RECOVERY_EVENT	0
#define TEMP_HIGH_THRESHOLD_NORMAL	600
#define TEMP_HIGH_RECOVERY_NORMAL	400
#define TEMP_LOW_THRESHOLD_NORMAL	-45
#define TEMP_LOW_RECOVERY_NORMAL	0
#define TEMP_HIGH_THRESHOLD_LPM		600
#define TEMP_HIGH_RECOVERY_LPM		400
#define TEMP_LOW_THRESHOLD_LPM		-45
#define TEMP_LOW_RECOVERY_LPM		0
#endif /* __SEC_BATTERY_DATA_H */
