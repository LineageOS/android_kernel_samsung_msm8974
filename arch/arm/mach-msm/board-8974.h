/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
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

#ifndef __MACH_MSM8974_BOARD_MONDRIAN_H
#define __MACH_MSM8974_BOARD_MONDRIAN_H

#include <mach/irqs.h>
#include <linux/i2c.h>

#if defined(CONFIG_BATTERY_SAMSUNG)
void samsung_init_battery(void);
#endif

#endif
