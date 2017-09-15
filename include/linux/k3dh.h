/*
 *  STMicroelectronics k3dh acceleration sensor driver
 *
 *  Copyright (C) 2010 Samsung Electronics Co.Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#ifndef __K3DH_ACC_HEADER__
#define __K3DH__ACC_HEADER__

#include <linux/types.h>
#include <linux/ioctl.h>

extern struct class *sec_class;

struct k3dh_acceldata {
	__s16 x;
	__s16 y;
	__s16 z;
};

struct accel_platform_data_k2dh {
	int (*accel_get_position) (void);
	bool i2c_pull_up;
	int gpio_int;
	u32 irq_gpio_flags;
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;

	const u8 *str_l19;
	const u8 *str_lvs1;
	/* Change axis or not for user-level
	 * If it is true, driver reports adjusted axis-raw-data
	 * to user-space based on accel_get_position() value,
	 * or if it is false, driver reports original axis-raw-data */
	bool axis_adjust;
};


/* dev info */
#define ACC_DEV_NAME "accelerometer"

/* k3dh ioctl command label */
#define K3DH_IOCTL_BASE 'a'
#define K3DH_IOCTL_SET_DELAY		_IOW(K3DH_IOCTL_BASE, 0, int64_t)
#define K3DH_IOCTL_GET_DELAY		_IOR(K3DH_IOCTL_BASE, 1, int64_t)
#define K3DH_IOCTL_READ_ACCEL_XYZ	_IOR(K3DH_IOCTL_BASE, 8, \
						struct k3dh_acceldata)
#define K3DH_IOCTL_SET_ENABLE		_IOW(K3DH_IOCTL_BASE, 9, int)
#endif
