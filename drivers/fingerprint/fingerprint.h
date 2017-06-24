/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef FINGERPRINT_H_
#define FINGERPRINT_H_

#include <linux/clk.h>

/* fingerprint debug timer */
#define FPSENSOR_DEBUG_TIMER_SEC (10 * HZ)

/* For Sensor Type Check */
enum {
	SENSOR_UNKNOWN = -1,
	SENSOR_FAILED,
	SENSOR_VIPER,
	SENSOR_RAPTOR,
	SENSOR_EGIS,
};

#define SENSOR_STATUS_SIZE 5
static char sensor_status[SENSOR_STATUS_SIZE][8] ={"unknown", "failed",
	"viper", "raptor", "egis"};

/* Fingerprint vendor check */
#undef ENABLE_VENDOR_CHECK

#ifdef ENABLE_VENDOR_CHECK
extern int FP_CHECK; /* extern variable */
#endif

#endif
