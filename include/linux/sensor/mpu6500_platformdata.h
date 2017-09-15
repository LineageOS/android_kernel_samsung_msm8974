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

#ifndef _MPU6500_PLATFORMDATA_H_
#define _MPU6500_PLATFORMDATA_H_

#define MPU6500_TOP_LEFT_UPPER		0
#define MPU6500_TOP_RIGHT_UPPER		1
#define MPU6500_TOP_RIGHT_LOWER		2
#define MPU6500_TOP_LEFT_LOWER		3
#define MPU6500_BOTTOM_LEFT_UPPER	4
#define MPU6500_BOTTOM_RIGHT_UPPER	5
#define MPU6500_BOTTOM_RIGHT_LOWER	6
#define MPU6500_BOTTOM_LEFT_LOWER	7

struct mpu6500_platform_data {
	void (*get_pos)(int *);
	bool i2c_pull_up;
	int gpio_int;
	u32 irq_gpio_flags;
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;

	const u8 *str_l6;
	const u8 *str_lvs1;

	const char *acc_cal_path;
	const char *gyro_cal_path;
};
#endif
