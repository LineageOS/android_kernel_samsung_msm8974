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

#ifndef _BMG160_REG_H_
#define _BMG160_REG_H_

/* For Settings */
#define BMG160_MODE_NORMAL                 0
#define BMG160_MODE_DEEPSUSPEND            1
#define BMG160_MODE_SUSPEND                2
#define BMG160_MODE_FASTPOWERUP            3
#define BMG160_MODE_ADVANCEDPOWERSAVING    4

#define BMG160_RANGE_2000DPS               0
#define BMG160_RANGE_1000DPS               1
#define BMG160_RANGE_500DPS                2
#define BMG160_RANGE_250DPS                3
#define BMG160_RANGE_125DPS                4

#define BMG160_No_Filter                   0
#define	BMG160_BW_230Hz                    1
#define	BMG160_BW_116Hz                    2
#define	BMG160_BW_47Hz                     3
#define	BMG160_BW_23Hz                     4
#define	BMG160_BW_12Hz                     5
#define	BMG160_BW_64Hz                     6
#define	BMG160_BW_32Hz                     7

#define BMG160_No_AutoSleepDur	           0
#define	BMG160_4ms_AutoSleepDur	           1
#define	BMG160_5ms_AutoSleepDur            2
#define	BMG160_8ms_AutoSleepDur            3
#define	BMG160_10ms_AutoSleepDur           4
#define	BMG160_15ms_AutoSleepDur           5
#define	BMG160_20ms_AutoSleepDur           6
#define	BMG160_40ms_AutoSleepDur           7

/* Data Register */
#define BMG160_CHIP_ID_REG                 0x00

#ifdef CONFIG_SENSORS_BMI058
#define BMG160_RATE_X_LSB_ADDR             0x04
#define BMG160_RATE_X_MSB_ADDR             0x05
#define BMG160_RATE_Y_LSB_ADDR             0x02
#define BMG160_RATE_Y_MSB_ADDR             0x03
#else
#define BMG160_RATE_X_LSB_ADDR             0x02
#define BMG160_RATE_X_MSB_ADDR             0x03
#define BMG160_RATE_Y_LSB_ADDR             0x04
#define BMG160_RATE_Y_MSB_ADDR             0x05
#endif
#define BMG160_RATE_Z_LSB_ADDR             0x06
#define BMG160_RATE_Z_MSB_ADDR             0x07
#define BMG160_TEMP_ADDR                   0x08

/* Control Register */
#define BMG160_RANGE_ADDR                  0x0F
#define BMG160_BW_ADDR                     0x10
#define BMG160_MODE_LPM1_ADDR              0x11
#define BMG160_MODE_LPM2_ADDR              0x12
#define BMG160_RATED_HBW_ADDR              0x13
#define BMG160_BGW_SOFTRESET_ADDR          0x14
#define BMG160_SELF_TEST_ADDR              0x3C

#define BMG160_RATE_X_LSB_VALUEX__POS        0

#define BMG160_RATE_X_LSB_VALUEX__LEN                   8
#define BMG160_RATE_X_LSB_VALUEX__MSK                   0xFF
#define BMG160_RATE_X_LSB_VALUEX__REG                   BMG160_RATE_X_LSB_ADDR

#define BMG160_RATE_Y_LSB_VALUEY__POS                   0
#define BMG160_RATE_Y_LSB_VALUEY__LEN                   8
#define BMG160_RATE_Y_LSB_VALUEY__MSK                   0xFF
#define BMG160_RATE_Y_LSB_VALUEY__REG                   BMG160_RATE_Y_LSB_ADDR

#define BMG160_RATE_Z_LSB_VALUEZ__POS                   0
#define BMG160_RATE_Z_LSB_VALUEZ__LEN                   8
#define BMG160_RATE_Z_LSB_VALUEZ__MSK                   0xFF
#define BMG160_RATE_Z_LSB_VALUEZ__REG                   BMG160_RATE_Z_LSB_ADDR

#define BMG160_MODE_LPM1__POS                           5
#define BMG160_MODE_LPM1__LEN                           3
#define BMG160_MODE_LPM1__MSK                           0xA0
#define BMG160_MODE_LPM1__REG                           BMG160_MODE_LPM1_ADDR

#define BMG160_MODELPM1_ADDR_SLEEPDUR__POS              1
#define BMG160_MODELPM1_ADDR_SLEEPDUR__LEN              3
#define BMG160_MODELPM1_ADDR_SLEEPDUR__MSK              0x0E
#define BMG160_MODELPM1_ADDR_SLEEPDUR__REG              BMG160_MODE_LPM1_ADDR

#define BMG160_MODE_LPM2_ADDR_FAST_POWERUP__POS         7
#define BMG160_MODE_LPM2_ADDR_FAST_POWERUP__LEN         1
#define BMG160_MODE_LPM2_ADDR_FAST_POWERUP__MSK         0x80
#define BMG160_MODE_LPM2_ADDR_FAST_POWERUP__REG         BMG160_MODE_LPM2_ADDR

#define BMG160_MODE_LPM2_ADDR_ADV_POWERSAVING__POS      6
#define BMG160_MODE_LPM2_ADDR_ADV_POWERSAVING__LEN      1
#define BMG160_MODE_LPM2_ADDR_ADV_POWERSAVING__MSK      0x40
#define BMG160_MODE_LPM2_ADDR_ADV_POWERSAVING__REG      BMG160_MODE_LPM2_ADDR

#define BMG160_MODE_LPM2_ADDR_EXT_TRI_SEL__POS          4
#define BMG160_MODE_LPM2_ADDR_EXT_TRI_SEL__LEN          2
#define BMG160_MODE_LPM2_ADDR_EXT_TRI_SEL__MSK          0x30
#define BMG160_MODE_LPM2_ADDR_EXT_TRI_SEL__REG          BMG160_MODE_LPM2_ADDR

#define BMG160_MODE_LPM2_ADDR_AUTOSLEEPDUR__POS         0
#define BMG160_MODE_LPM2_ADDR_AUTOSLEEPDUR__LEN         3
#define BMG160_MODE_LPM2_ADDR_AUTOSLEEPDUR__MSK         0x07
#define BMG160_MODE_LPM2_ADDR_AUTOSLEEPDUR__REG         BMG160_MODE_LPM2_ADDR

#define BMG160_RANGE_ADDR_RANGE__POS                    0
#define BMG160_RANGE_ADDR_RANGE__LEN                    3
#define BMG160_RANGE_ADDR_RANGE__MSK                    0x07
#define BMG160_RANGE_ADDR_RANGE__REG                    BMG160_RANGE_ADDR

#define BMG160_BW_ADDR__POS                             0
#define BMG160_BW_ADDR__LEN                             3
#define BMG160_BW_ADDR__MSK                             0x07
#define BMG160_BW_ADDR__REG                             BMG160_BW_ADDR

#define BMG160_SELF_TEST_ADDR_BISTFAIL__POS             2
#define BMG160_SELF_TEST_ADDR_BISTFAIL__LEN             1
#define BMG160_SELF_TEST_ADDR_BISTFAIL__MSK             0x04
#define BMG160_SELF_TEST_ADDR_BISTFAIL__REG             BMG160_SELF_TEST_ADDR

#define BMG160_SELF_TEST_ADDR_TRIGBIST__POS             0
#define BMG160_SELF_TEST_ADDR_TRIGBIST__LEN             1
#define BMG160_SELF_TEST_ADDR_TRIGBIST__MSK             0x01
#define BMG160_SELF_TEST_ADDR_TRIGBIST__REG             BMG160_SELF_TEST_ADDR

#define BMG160_SELF_TEST_ADDR_RATEOK__POS               4
#define BMG160_SELF_TEST_ADDR_RATEOK__LEN               1
#define BMG160_SELF_TEST_ADDR_RATEOK__MSK               0x10
#define BMG160_SELF_TEST_ADDR_RATEOK__REG               BMG160_SELF_TEST_ADDR

/* get bit slice  */
#define BMG160_GET_BITSLICE(regvar, bitname)\
		((regvar & bitname##__MSK) >> bitname##__POS)

/* Set bit slice */
#define BMG160_SET_BITSLICE(regvar, bitname, val)\
		((regvar & ~bitname##__MSK) |\
		((val << bitname##__POS) & bitname##__MSK))

enum {
	OFF = 0,
	ON = 1
};

#endif
