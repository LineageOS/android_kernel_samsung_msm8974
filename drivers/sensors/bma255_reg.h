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

#ifndef _BMA255_REG_H_
#define _BMA255_REG_H_

/* For Mode Settings    */
#define BMA255_MODE_NORMAL              0
#define BMA255_MODE_LOWPOWER1           1
#define BMA255_MODE_SUSPEND             2
#define BMA255_MODE_DEEP_SUSPEND        3
#define BMA255_MODE_LOWPOWER2           4
#define BMA255_MODE_STANDBY             5
#define BMA255_MODE_MAX			6

#define BMA255_RANGE_2G                 3
#define BMA255_RANGE_4G                 5
#define BMA255_RANGE_8G                 8
#define BMA255_RANGE_16G                12

#define BMA255_BW_7_81HZ                0x08
#define BMA255_BW_15_63HZ               0x09
#define BMA255_BW_31_25HZ               0x0A
#define BMA255_BW_62_50HZ               0x0B
#define BMA255_BW_125HZ                 0x0C
#define BMA255_BW_250HZ                 0x0D
#define BMA255_BW_500HZ                 0x0E
#define BMA255_BW_1000HZ                0x0F

/* Data Register */
#define BMA255_CHIP_ID_REG              0x00
#define BMA255_VERSION_REG              0x01
#define BMA255_X_AXIS_LSB_REG           0x02
#define BMA255_X_AXIS_MSB_REG           0x03
#define BMA255_Y_AXIS_LSB_REG           0x04
#define BMA255_Y_AXIS_MSB_REG           0x05
#define BMA255_Z_AXIS_LSB_REG           0x06
#define BMA255_Z_AXIS_MSB_REG           0x07

/* Control Register */
#define BMA255_STATUS1_REG              0x09
#define BMA255_RANGE_SEL_REG            0x0F
#define BMA255_BW_SEL_REG               0x10
#define BMA255_MODE_CTRL_REG            0x11
#define BMA255_LOW_NOISE_CTRL_REG       0x12
#define BMA255_INT_ENABLE1_REG          0x16
#define BMA255_INT1_PAD_SEL_REG         0x19

#define BMA255_STATUS_TAP_SLOPE_REG     0x0B
#define BMA255_INT_CTRL_REG             0x21
#define BMA255_SLOPE_DURN_REG           0x27
#define BMA255_SLOPE_THRES_REG          0x28

#define BMA255_LOW_POWER_MODE__POS      6
#define BMA255_LOW_POWER_MODE__LEN      1
#define BMA255_LOW_POWER_MODE__MSK      0x40
#define BMA255_LOW_POWER_MODE__REG      BMA255_LOW_NOISE_CTRL_REG

#define BMA255_MODE_CTRL__POS           5
#define BMA255_MODE_CTRL__LEN           3
#define BMA255_MODE_CTRL__MSK           0xE0
#define BMA255_MODE_CTRL__REG           BMA255_MODE_CTRL_REG

#define BMA255_RANGE_SEL__POS           0
#define BMA255_RANGE_SEL__LEN           4
#define BMA255_RANGE_SEL__MSK           0x0F
#define BMA255_RANGE_SEL__REG           BMA255_RANGE_SEL_REG

#define BMA255_BANDWIDTH__POS           0
#define BMA255_BANDWIDTH__LEN           5
#define BMA255_BANDWIDTH__MSK           0x1F
#define BMA255_BANDWIDTH__REG           BMA255_BW_SEL_REG

#define BMA255_ACC_X_MSB__POS           0
#define BMA255_ACC_X_MSB__LEN           8
#define BMA255_ACC_X_MSB__MSK           0xFF
#define BMA255_ACC_X_MSB__REG           BMA255_X_AXIS_MSB_REG

#define BMA255_ACC_Y_MSB__POS           0
#define BMA255_ACC_Y_MSB__LEN           8
#define BMA255_ACC_Y_MSB__MSK           0xFF
#define BMA255_ACC_Y_MSB__REG           BMA255_Y_AXIS_MSB_REG

#define BMA255_ACC_Z_MSB__POS           0
#define BMA255_ACC_Z_MSB__LEN           8
#define BMA255_ACC_Z_MSB__MSK           0xFF
#define BMA255_ACC_Z_MSB__REG           BMA255_Z_AXIS_MSB_REG

#define BMA255_ACC_X12_LSB__POS         4
#define BMA255_ACC_X12_LSB__LEN         4
#define BMA255_ACC_X12_LSB__MSK         0xF0
#define BMA255_ACC_X12_LSB__REG         BMA255_X_AXIS_LSB_REG

#define BMA255_ACC_Y12_LSB__POS         4
#define BMA255_ACC_Y12_LSB__LEN         4
#define BMA255_ACC_Y12_LSB__MSK         0xF0
#define BMA255_ACC_Y12_LSB__REG         BMA255_Y_AXIS_LSB_REG

#define BMA255_ACC_Z12_LSB__POS         4
#define BMA255_ACC_Z12_LSB__LEN         4
#define BMA255_ACC_Z12_LSB__MSK         0xF0
#define BMA255_ACC_Z12_LSB__REG         BMA255_Z_AXIS_LSB_REG

#define BMA255_SLOPE_INT_S__POS         2
#define BMA255_SLOPE_INT_S__LEN         1
#define BMA255_SLOPE_INT_S__MSK         0x04
#define BMA255_SLOPE_INT_S__REG         BMA255_STATUS1_REG

#define BMA255_EN_SLOPE_X_INT__POS      0
#define BMA255_EN_SLOPE_X_INT__LEN      1
#define BMA255_EN_SLOPE_X_INT__MSK      0x01
#define BMA255_EN_SLOPE_X_INT__REG      BMA255_INT_ENABLE1_REG

#define BMA255_EN_SLOPE_Y_INT__POS      1
#define BMA255_EN_SLOPE_Y_INT__LEN      1
#define BMA255_EN_SLOPE_Y_INT__MSK      0x02
#define BMA255_EN_SLOPE_Y_INT__REG      BMA255_INT_ENABLE1_REG

#define BMA255_EN_SLOPE_Z_INT__POS      2
#define BMA255_EN_SLOPE_Z_INT__LEN      1
#define BMA255_EN_SLOPE_Z_INT__MSK      0x04
#define BMA255_EN_SLOPE_Z_INT__REG      BMA255_INT_ENABLE1_REG

#define BMA255_EN_INT1_PAD_SLOPE__POS   2
#define BMA255_EN_INT1_PAD_SLOPE__LEN   1
#define BMA255_EN_INT1_PAD_SLOPE__MSK   0x04
#define BMA255_EN_INT1_PAD_SLOPE__REG   BMA255_INT1_PAD_SEL_REG

#define BMA255_SLOPE_DUR__POS           0
#define BMA255_SLOPE_DUR__LEN           2
#define BMA255_SLOPE_DUR__MSK           0x03
#define BMA255_SLOPE_DUR__REG           BMA255_SLOPE_DURN_REG

#define BMA255_SLOPE_THRES__POS         0
#define BMA255_SLOPE_THRES__LEN         8
#define BMA255_SLOPE_THRES__MSK         0xFF
#define BMA255_SLOPE_THRES__REG         BMA255_SLOPE_THRES_REG

#define BMA255_INT_MODE_SEL__POS        0
#define BMA255_INT_MODE_SEL__LEN        4
#define BMA255_INT_MODE_SEL__MSK        0x0F
#define BMA255_INT_MODE_SEL__REG        BMA255_INT_CTRL_REG

#define BMA255_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)

#define BMA255_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))
#endif
