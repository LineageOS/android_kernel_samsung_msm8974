/*
 * vibrator.h
 *
 * header file describing vibrator platform data for Samsung device
 *
 * COPYRIGHT(C) Samsung Electronics Co., Ltd. 2006-2011 All Right Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __VIBRATOR_H__
#define __VIBRATOR_H__

enum vibrator_model {
	NO_VIBRATOR,
	HAPTIC_PWM,
	HAPTIC_MOTOR,
};

struct vibrator_platform_data {
	unsigned int vib_pwm_gpio;/* gpio number for vibrator pwm */
	unsigned int haptic_pwr_en_gpio;/* gpio number of haptic power enable */
	unsigned int vib_en_gpio;/* gpio number of vibrator enable */
	unsigned int is_pmic_haptic_pwr_en;	/* 1 -> pmic gpio used,\
						   0 -> msm gpio used */
	unsigned int is_pmic_vib_en;		/* 1 -> pmic gpio used,\
						   0 -> msm gpio used */
	unsigned int is_pmic_vib_pwm ;
	enum vibrator_model vib_model;
	struct pwm_device	*pwm_dev;
	unsigned int pwm_period_us;
	unsigned int duty_us;
	void (*power_onoff)(int onoff);
	struct clk *gp2_clk;

	unsigned int changed_chip;
	unsigned int changed_en_gpio;
#if defined(CONFIG_MOTOR_DRV_DRV2603)
	unsigned int drv2603_en_gpio;
#endif
#if defined(CONFIG_MOTOR_DRV_MAX77888)
	unsigned int max77888_en_gpio;
#endif
	unsigned int intensity;
	unsigned int state;
};

struct vibrator_platform_data_motor {
	void (*power_onoff)(int onoff);
};

#if defined(CONFIG_HAPTIC_ISA1200)

struct vibrator_platform_data_isa1200 {
	unsigned int motor_en;
	unsigned int vib_clk;
	struct i2c_client *client;
#if defined(CONFIG_MACH_MATISSE3G_OPEN) || defined (CONFIG_SEC_MATISSELTE_COMMON) || defined (CONFIG_MACH_T10_3G_OPEN)
	void (*power_onoff)(int onoff);
#endif
	unsigned int intensity;
};
#endif

#if defined(CONFIG_DRV2604_VIBRATOR)
struct vibrator_platform_data_drv2604 {
	unsigned int motor_en;
	struct i2c_client *client;
};

#endif

#endif
