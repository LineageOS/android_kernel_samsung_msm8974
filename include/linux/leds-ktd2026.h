/*
 * Copyright (C) 2011 Samsung Electronics Co. Ltd. All Rights Reserved.
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

#ifndef _LEDS_KTD2026_H
#define _LEDS_KTD2026_H

#include <linux/ioctl.h>
#include <linux/types.h>

struct ktd2026_led_conf {
	const char      *name;
	int          brightness;
	int          max_brightness;
	int          flags;
};
enum ktd2026_pattern {
	PATTERN_OFF,
	CHARGING,
	CHARGING_ERR,
	MISSED_NOTI,
	LOW_BATTERY,
	FULLY_CHARGED,
	POWERING,
};
extern void ktd2026_start_led_pattern(enum ktd2026_pattern mode);
extern void ktd2026_led_blink(int rgb, int on, int off);

#endif						/* _LEDS_KTD2026_H */
