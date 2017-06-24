/* Copyright (c) 2009-2011, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef LINUX_BMA254_MODULE_H
#define LINUX_BMA254_MODULE_H


#define BMA254_CHIP_ID		0x00
#define BMA254_XOUT		0x02
#define BMA254_YOUT		0x04
#define BMA254_ZOUT		0x06
#define BMA254_TEMP		0x08
#define BMA254_REG0B		0x0B
#define BMA254_REG0A		0x0A
#define BMA254_REG0F		0X0F
#define BMA254_REG10		0x10
#define BMA254_REG11		0x11
#define BMA254_REG14		0x14
#define SOFT_RESEET			0xB6

#define BMA2X2_RANGE_SET		3
#define MAX_DELAY				200
#define ABSMIN				-128
#define ABSMAX				128

/*Mode for BMA254 only*/
#define BMA254_MODE_NORMAL			0x00
#define BMA254_MODE_LOWPOWER1		0x40
#define BMA254_MODE_SUSPEND			0x80
#define BMA254_MODE_DEEP_SUSPEND	0x20

/* Bandwidth */
#define BANDWIDTH_07_81		0x08
#define BANDWIDTH_15_63		0x09
#define BANDWIDTH_31_25		0x0A
#define BANDWIDTH_62_50		0x0B
#define BANDWIDTH_125		0x0C
#define BANDWIDTH_250		0x0D

/*add register for calibartion*/
#define BMA254_EEPROM_CTRL_REG                  0x33
#define BMA254_OFFSET_CTRL_REG                  0x36
#define BMA254_OFFSET_PARAMS_REG                0x37
#define BMA254_OFFSET_FILT_X_REG                0x38
#define BMA254_OFFSET_FILT_Y_REG                0x39
#define BMA254_OFFSET_FILT_Z_REG                0x3A

#define BMA254_EN_FAST_COMP__POS                5
#define BMA254_EN_FAST_COMP__LEN                2
#define BMA254_EN_FAST_COMP__MSK                0x60
#define BMA254_EN_FAST_COMP__REG                BMA254_OFFSET_CTRL_REG

#define BMA254_COMP_TARGET_OFFSET_X__POS        1
#define BMA254_COMP_TARGET_OFFSET_X__LEN        2
#define BMA254_COMP_TARGET_OFFSET_X__MSK        0x06
#define BMA254_COMP_TARGET_OFFSET_X__REG        BMA254_OFFSET_PARAMS_REG

#define BMA254_COMP_TARGET_OFFSET_Y__POS        3
#define BMA254_COMP_TARGET_OFFSET_Y__LEN        2
#define BMA254_COMP_TARGET_OFFSET_Y__MSK        0x18
#define BMA254_COMP_TARGET_OFFSET_Y__REG        BMA254_OFFSET_PARAMS_REG

#define BMA254_COMP_TARGET_OFFSET_Z__POS        5
#define BMA254_COMP_TARGET_OFFSET_Z__LEN        2
#define BMA254_COMP_TARGET_OFFSET_Z__MSK        0x60
#define BMA254_COMP_TARGET_OFFSET_Z__REG        BMA254_OFFSET_PARAMS_REG

#define BMA254_FAST_COMP_RDY_S__POS             4
#define BMA254_FAST_COMP_RDY_S__LEN             1
#define BMA254_FAST_COMP_RDY_S__MSK             0x10
#define BMA254_FAST_COMP_RDY_S__REG             BMA254_OFFSET_CTRL_REG

#define BMA254_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)
#define BMA254_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))
/** \endcond */

/* SETTING THIS BIT  UNLOCK'S WRITING SETTING REGISTERS TO EEPROM */

#define BMA254_UNLOCK_EE_WRITE_SETTING__POS     0
#define BMA254_UNLOCK_EE_WRITE_SETTING__LEN     1
#define BMA254_UNLOCK_EE_WRITE_SETTING__MSK     0x01
#define BMA254_UNLOCK_EE_WRITE_SETTING__REG     BMA254_EEPROM_CTRL_REG


/* SETTING THIS BIT STARTS WRITING SETTING REGISTERS TO EEPROM */

#define BMA254_START_EE_WRITE_SETTING__POS      1
#define BMA254_START_EE_WRITE_SETTING__LEN      1
#define BMA254_START_EE_WRITE_SETTING__MSK      0x02
#define BMA254_START_EE_WRITE_SETTING__REG      BMA254_EEPROM_CTRL_REG

/* STATUS OF WRITING TO EEPROM */

#define BMA254_EE_WRITE_SETTING_S__POS          2
#define BMA254_EE_WRITE_SETTING_S__LEN          1
#define BMA254_EE_WRITE_SETTING_S__MSK          0x04
#define BMA254_EE_WRITE_SETTING_S__REG          BMA254_EEPROM_CTRL_REG

#ifdef CONFIG_BMA254_SMART_ALERT
#define BMA254_STATUS1_REG						0x09
#define BMA254_INT1_PAD_SEL_REG					0x19
#define BMA254_SLOPE_DURN_REG					0x27
#define BMA254_INT_ENABLE1_REG					0x16
#define BMA254_SLOPE_THRES_REG					0x28
#define BMA254_SLOPE_INT_S__REG				BMA254_STATUS1_REG
#define BMA254_EN_INT1_PAD_SLOPE__REG			BMA254_INT1_PAD_SEL_REG
#define BMA254_SLOPE_DUR__REG				BMA254_SLOPE_DURN_REG
#define BMA254_EN_SLOPE_X_INT__REG			BMA254_INT_ENABLE1_REG
#define BMA254_SLOPE_THRES__REG				BMA254_SLOPE_THRES_REG
#endif
#endif
