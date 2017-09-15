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
	#if defined(CONFIG_MACH_RUBENSWIFI_OPEN)
    .RCOMP0 = 0x75,
    .RCOMP_charging = 0x75,
    .temp_cohot = -100,
    .temp_cocold = -3600,
    #else
	.RCOMP0 = 0x91,
    .RCOMP_charging = 0x91,
    .temp_cohot = -100,
    .temp_cocold = -3600,
    #endif
    .is_using_model_data = true,
    .type_str = "SDI",
	}
};

#define CAPACITY_MAX      1000
#define CAPACITY_MAX_MARGIN 50
#define CAPACITY_MIN      0

#define TEMP_HIGH_THRESHOLD_EVENT  550
#define TEMP_HIGH_RECOVERY_EVENT   460
#define TEMP_LOW_THRESHOLD_EVENT   (-50)
#define TEMP_LOW_RECOVERY_EVENT    0
#define TEMP_HIGH_THRESHOLD_NORMAL 550
#define TEMP_HIGH_RECOVERY_NORMAL  460
#define TEMP_LOW_THRESHOLD_NORMAL  (-50)
#define TEMP_LOW_RECOVERY_NORMAL   0
#define TEMP_HIGH_THRESHOLD_LPM    550
#define TEMP_HIGH_RECOVERY_LPM     460
#define TEMP_LOW_THRESHOLD_LPM     (-50)
#define TEMP_LOW_RECOVERY_LPM      0

static sec_bat_adc_table_data_t temp_table[] = {
  {25348, 900},
  {25804, 850},
  {26261, 800},
  {26717, 750},
  {27174, 700},
  {27630, 650},
  {28119, 600},
  {28716, 550},
  {29379, 500},
  {30051, 450},
  {30884, 400},
  {31955, 350},
  {32915, 300},
  {34013, 250},
  {35124, 200},
  {36159, 150},
  {37233, 100},
  {38286, 50},
  {39075, 0},
  {39866, -50},
  {40669, -100},
  {41184, -150},
  {41686, -200},
};

static sec_bat_adc_table_data_t chg_temp_table[] = {
	{0, 0},
};

#endif /* __SEC_BATTERY_DATA_H */