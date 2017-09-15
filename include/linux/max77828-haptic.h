/*
 * Vibrator driver for Maxim MAX77828
 *
 * Copyright (C) 2013 Maxim Integrated Product
 * Gyungoh Yoo <jack.yoo@maximintegrated.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __MAX77828_HAPTIC_H__
#define __MAX77828_HAPTIC_H__

enum max77828_haptic_motor
{
	MAX77828_VIB_MOTOR_ERM,
	MAX77828_VIB_MOTOR_LRA,
};

struct max77828_haptic_platform_data
{
	enum max77828_haptic_motor mode;
	int divisor;	/* PWM Frequency Divisor. 32, 64, 128 or 256 */
};

#endif /* __MAX77828_HAPTIC_H__ */
