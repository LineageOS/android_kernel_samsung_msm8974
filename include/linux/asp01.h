/*
 *  AD Semiconductor grip sensor driver
 *
 *  Copyright (C) 2012 Samsung Electronics Co.Ltd
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

#ifndef __ASP01_GRIP_HEADER__
#define __ASP01_GRIP_HEADER__

#define EN_INIT_CAL	(0x1 << 3)

enum {
	IIO_ATTR_ENABLE = 0,
	IIO_ATTR_DELAY,
};

enum asp01_I2C_IF {
	REG = 0,
	CMD,
};

/* register address */
enum asp01_REG {
	REG_PROM_EN1 = 0x01,
	REG_PROM_EN2,	/* 0x02 */
	REG_SETMFM,	/* 0x03 */
	REG_CR_CNT,	/* 0x04 */
	REG_CR_CNT_H,	/* 0x05 */
	REG_CR_REF0,	/* 0x06 */
	REG_CR_REF1,	/* 0x07 */
	REG_CR_PER_L,	/* 0x08 */
	REG_CR_PER_H,	/* 0x09 */
	REG_CS_REF0 = 0x0c,
	REG_CS_REF1,	/* 0x0d */
	REG_CS_PERL,	/* 0x0e */
	REG_CS_PERH,	/* 0x0f */
	REG_UNLOCK = 0x11,
	REG_RST_ERR,	/* 0x12 */
	REG_PROX_PER,	/* 0x13 */
	REG_PAR_PER,	/* 0x14 */
	REG_TOUCH_PER,	/* 0x15 */
	REG_HI_CAL_PER = 0x19,
	REG_BSMFM_SET,	/*0x1A */
	REG_ERR_MFM_CYC,	/* 0x1B */
	REG_TOUCH_MFM_CYC,	/* 0x1C */
	REG_HI_CAL_SPD,	/* 0x1D */
	REG_CAL_SPD,	/* 0x1E */
	REG_INIT_REF,	/* 0x1F */
	REG_BFT_MOT,	/* 0x20 */
	REG_TOU_RF_EXT = 0x22,
	REG_SYS_FUNC,	/* 0x23 */
	REG_MFM_INIT_REF0,	/* 0x24 */
	REG_EEP_ST_CON = 0x34,
	REG_EEP_ADDR0,	/* 0x35 */
	REG_EEP_ADDR1,	/* 0x36 */
	REG_EEP_DATA,	/* 0x37 */
	REG_ID_CHECK,	/* 0x38 */
	REG_OFF_TIME,	/* 0x39 */
	REG_SENSE_TIME,	/* 0x3A */
	REG_DUTY_TIME,	/* 0x3B */
	REG_HW_CON1,	/* 0x3C */
	REG_HW_CON2,	/* 0x3D */
	REG_HW_CON3,	/* 0x3E */
	REG_HW_CON4,	/* 0x3F */
	REG_HW_CON5,	/* 0x40 */
	REG_HW_CON6,	/* 0x41 */
	REG_HW_CON7,	/* 0x42 */
	REG_HW_CON8,	/* 0x43 */
	REG_HW_CON9,	/* 0x44 */
	REG_HW_CON10,	/* 0x45 */
	REG_HW_CON11,	/* 0x46 */
};

enum asp01_CMD {
	CMD_CLK_OFF = 0,
	CMD_CLK_ON,
	CMD_RESET,
	CMD_INIT_TOUCH_OFF,
	CMD_INIT_TOUCH_ON,
	CMD_NUM,
};

static const u8 control_reg[CMD_NUM][2] = {
	{REG_SYS_FUNC, 0x16}, /* clock off */
	{REG_SYS_FUNC, 0x10}, /* clock on */
	{REG_SYS_FUNC, 0x01}, /* sw reset */
	{REG_INIT_REF, 0x00}, /* disable initial touch */
	{REG_INIT_REF, EN_INIT_CAL}, /* enable initial touch */
};

/* order of init code */
enum ASP01_INIT_CODE {
	SET_UNLOCK = 0,
	SET_RST_ERR,
	SET_PROX_PER,
	SET_PAR_PER,
	SET_TOUCH_PER,
	SET_HI_CAL_PER,
	SET_BSMFM_SET,
	SET_ERR_MFM_CYC,
	SET_TOUCH_MFM_CYC,
	SET_HI_CAL_SPD,
	SET_CAL_SPD,
	SET_BFT_MOT,
	SET_TOU_RF_EXT,
	SET_SYS_FUNC,
	SET_OFF_TIME,
	SET_SENSE_TIME,
	SET_DUTY_TIME,
	SET_HW_CON1,
	SET_HW_CON2,
	SET_HW_CON3,
	SET_HW_CON4,
	SET_HW_CON5,
	SET_HW_CON6,
	SET_HW_CON7,
	SET_HW_CON8,
	SET_HW_CON9,
	SET_HW_CON10,
	SET_HW_CON11,
	SET_REG_NUM,
};

struct asp01_platform_data {
	int cr_divsr;
	int cr_divnd;
	int cs_divsr;
	int cs_divnd;
	int irq;
	int gpio;
	u8 init_code[SET_REG_NUM];
};

#endif
