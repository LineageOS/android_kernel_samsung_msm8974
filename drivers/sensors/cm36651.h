/*
 * driver/sensor/cm36651.c
 * Copyright (c) 2011 SAMSUNG ELECTRONICS
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#ifndef __LINUX_CM36651_H
#define __CM36651_H__

struct cm36651_platform_data {
	//void (*cm36651_led_on) (int);
	//void (*cm36651_power_on) (int);
	int irq;		/* proximity-sensor irq gpio */
	unsigned char threshold;

	int p_out; /* proximity-sensor-output gpio (proximity interrupt) */
	u32 p_out_flags;
	u32 vdd_2p85;
	u32 vdd_en;
	const char *prox_cal_path;
	int d0_value[9];
	u8 thresh[2];
};
extern struct class *sensors_class;

#endif
