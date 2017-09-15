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
		.RCOMP0 = 0x5D,
		.RCOMP_charging = 0x5D,
		.temp_cohot = -175,
		.temp_cocold = -5825,
		.is_using_model_data = true,
		.type_str = "SDI",
	}
};

#define CAPACITY_MAX			992
#define CAPACITY_MAX_MARGIN	50
#define CAPACITY_MIN			0

static sec_bat_adc_table_data_t temp_table[] = {
        {1100,  350},
        {668,   350},
        {657,   350},
        {613,   350},
        {556,   325},
        {486,   325},
        {402,   325},
        {306,   300},
        {199,   300},
        {90,    300},
        {-3,    300},
        {-93,   290},
        {-161,  290},
        {-350,  290},
};

static sec_bat_adc_table_data_t chg_temp_table[] = {
	{0, 0},
};
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

#endif /* __SEC_BATTERY_DATA_H */
