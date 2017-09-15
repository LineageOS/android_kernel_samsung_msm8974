/* arch/arm/mach-tegra/sec_vibrator.c
 *
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

#ifndef _LINUX_SEC_VIBRATOR_H
#define _LINUX_SEC_VIBRATOR_H

#define ISA1400_STATUS_SLEEP        0
#define ISA1400_STATUS_STANDBY      1

#define ISA1400_REG_ID              0x00

#define ISA1400_REG_RSTCTRL         0x0f
#define ISA1400_REG_AMPGAIN1        0x10
#define ISA1400_REG_AMPGAIN2        0x11
#define ISA1400_REG_OVDRGAIN1       0x12
#define ISA1400_REG_OVDRGAIN2       0x13
#define ISA1400_REG_OVDRTYP         0x1a
#define ISA1400_REG_AMPSWTCH        0x1b
#define ISA1400_REG_SYSCLK          0x1f
#define ISA1400_REG_GAIN            0x20
#define ISA1400_REG_GAIN1           (ISA1400_REG_GAIN + CH1)
#define ISA1400_REG_GAIN2           (ISA1400_REG_GAIN + CH2)
#define ISA1400_REG_GAIN3           (ISA1400_REG_GAIN + CH3)
#define ISA1400_REG_GAIN4           (ISA1400_REG_GAIN + CH4)
#define ISA1400_REG_FREQ            0x24
#define ISA1400_REG_FREQ1M          (ISA1400_REG_FREQ)
#define ISA1400_REG_FREQ1L          (ISA1400_REG_FREQ + 1)
#define ISA1400_REG_FREQ2M          (ISA1400_REG_FREQ + 2 * CH2)
#define ISA1400_REG_FREQ2L          (ISA1400_REG_FREQ + 2 * CH2 + 1)
#define ISA1400_REG_FREQ3M          (ISA1400_REG_FREQ + 2 * CH3)
#define ISA1400_REG_FREQ3L          (ISA1400_REG_FREQ + 2 * CH3 + 1)
#define ISA1400_REG_FREQ4M          (ISA1400_REG_FREQ + 2 * CH4)
#define ISA1400_REG_FREQ4L          (ISA1400_REG_FREQ + 2 * CH4 + 1)
#define ISA1400_REG_ODCTRL          0x2c
#define ISA1400_REG_HPTEN           0x2d
#define ISA1400_REG_CHMODE          0x2e
#define ISA1400_REG_WAVESEL         0x2f

#define ISA1400_REG_PHESEL          0x35
#define ISA1400_REG_PHESEL1         (ISA1400_REG_PHESEL)
#define ISA1400_REG_PHESEL2         (ISA1400_REG_PHESEL+1)
#define ISA1400_REG_PHEEN           0x37
#define ISA1400_REG_PHE             0x38
#define ISA1400_REG_PHE1            (ISA1400_REG_PHE)
#define ISA1400_REG_PHE2            (ISA1400_REG_PHE + 6 * 1)
#define ISA1400_REG_PHE3            (ISA1400_REG_PHE + 6 * 2)
#define ISA1400_REG_PHE4            (ISA1400_REG_PHE + 6 * 3)
#define ISA1400_REG_PHE5            (ISA1400_REG_PHE + 6 * 4)
#define ISA1400_REG_PHE6            (ISA1400_REG_PHE + 6 * 5)
#define ISA1400_REG_PHE7            (ISA1400_REG_PHE + 6 * 6)
#define ISA1400_REG_PHE8            (ISA1400_REG_PHE + 6 * 7)
#define ISA1400_REG_PHE9            (ISA1400_REG_PHE + 6 * 8)
#define ISA1400_REG_PHE10           (ISA1400_REG_PHE + 6 * 9)
#define ISA1400_REG_PHE11           (ISA1400_REG_PHE + 6 * 10)
#define ISA1400_REG_PHE12           (ISA1400_REG_PHE + 6 * 11)

#define HPTMOD_EACHPIEZO    0
#define HPTMOD_MOT1PIEZO2   1
#define HPTMOD_MOT2         2
#define HPTMOD_PIEZO4       3

#define WAVE_SINE           0
#define WAVE_PWMLIKE        1
#define WAVE_RECT           2
#define WAVE_DC             3

#define ISA1400_REG_PHE_CTRL1       0
#define ISA1400_REG_PHE_CTRL2       1
#define ISA1400_REG_PHE_CTRL3       2
#define ISA1400_REG_PHE_CTRL4       3
#define ISA1400_REG_PHE_CTRL5       4
#define ISA1400_REG_PHE_CTRL6       5
#define ISA1400_REG_PHE_CTRL_CNT    6

#define ISA1400_REG_INIT	0
#define ISA1400_REG_START	1
#define ISA1400_REG_STOP	2
#define ISA1400_REG_MAX	3

#ifdef __KERNEL__
typedef int8_t		VibeInt8;
typedef u_int8_t	VibeUInt8;
typedef int16_t		VibeInt16;
typedef u_int16_t	VibeUInt16;
typedef int32_t		VibeInt32;
typedef u_int32_t	VibeUInt32;
typedef u_int8_t	VibeBool;
typedef VibeInt32	VibeStatus;
#endif 

struct isa1400_vibrator_platform_data {
	int (*gpio_en) (bool) ;
	int (*clk_en) (bool) ;
	const u8 **reg_data;
	int max_timeout;
};

#if defined(CONFIG_MOTOR_DRV_ISA1400)
extern int isa1400_i2c_write(u8 addr, int length, u8 *data);
extern void isa1400_chip_enable(bool en);
extern void isa1400_clk_config(int duty);
#endif


#endif
