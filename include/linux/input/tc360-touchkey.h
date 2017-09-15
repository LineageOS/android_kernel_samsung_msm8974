/*
 * CORERIVER TOUCHCORE 360L touchkey driver
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 * Author: Taeyoon Yoon <tyoony.yoon@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __LINUX_TC360_H
#define __LINUX_TC360_H

#ifdef CONFIG_SAMSUNG_LPM_MODE
extern int poweroff_charging;
#endif

#define TC300K_DEVICE	"sec_touchkey" //"tc300k"
#define TSP_BOOSTER 
#ifdef TSP_BOOSTER
#include <linux/cpufreq.h>

#define DVFS_STAGE_DUAL		2
#define DVFS_STAGE_SINGLE		1
#define DVFS_STAGE_NONE		0
#define TOUCH_BOOSTER_OFF_TIME	500
#define TOUCH_BOOSTER_CHG_TIME	500
#endif

extern struct class *sec_class;
extern int touch_is_pressed;
extern int system_rev;

struct tc300k_platform_data {
	u8	enable;
	u32	gpio_scl;
	u32	gpio_sda;
	u32	gpio_int;
	u32 gpio_en;
	u32 irq_gpio_flags;
	u32 sda_gpio_flags;
	u32 scl_gpio_flags;
	u32 vcc_gpio_flags;
	int	udelay;
	int	num_key;
	int sensing_ch_num;
	int	*keycodes;
	u8	suspend_type;
	u8	exit_flag;
	const char *vcc_en_ldo_name;
	const char *vdd_led_ldo_name;
};
#endif /* __LINUX_TC360_H */
