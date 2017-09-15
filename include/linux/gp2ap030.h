/*
 * Copyright (c) 2012 SAMSUNG
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#ifndef __GP2A_H__
#define __GP2A_H__

#define I2C_M_WR 0		/* for i2c Write */
#define I2c_M_RD 1		/* for i2c Read */

#define I2C_DF_NOTIFY			0x01	/* for i2c */

/* Registers */
#define COMMAND1    0x00
#define COMMAND2    0x01	/* Read&Write */
#define COMMAND3    0x02	/* Read&Write */
#define COMMAND4    0x03	/* Read&Write */
#define INT_LT_LSB  0x04	/* Read&Write */
#define INT_LT_MSB  0x05	/* Read&Write */
#define INT_HT_LSB  0x06	/* Read&Write */
#define INT_HT_MSB  0x07	/* Read&Write */
#define PS_LT_LSB   0x08	/* Read&Write */
#define PS_LT_MSB   0x09	/* Read&Write */
#define PS_HT_LSB   0x0A	/* Read&Write */
#define PS_HT_MSB   0x0B	/* Read&Write */
#define DATA0_LSB   0x0C	/* Read Only */
#define DATA0_MSB   0x0D	/* Read Only */
#define DATA1_LSB   0x0E	/* Read Only */
#define DATA1_MSB   0x0F	/* Read Only */
#define DATA2_LSB   0x10	/* Read Only */
#define DATA2_MSB   0x11	/* Read Only */

#define ADC_BUFFER_NUM	6

#define DEFAULT_LO_THR	0x07 /* sharp recommand Loff */
#define DEFAULT_HI_THR	0x08 /* sharp recommand Lon */

#define OFFSET_ARRAY_LENGTH		10

/* 16 level for premium model*/
enum {
	LIGHT_DIM   = 0,
	LIGHT_LEVEL1,
	LIGHT_LEVEL2,
	LIGHT_LEVEL3,
	LIGHT_LEVEL4,
	LIGHT_LEVEL5,
	LIGHT_LEVEL6,
	LIGHT_LEVEL7,
	LIGHT_LEVEL8,
	LIGHT_LEVEL9,
	LIGHT_LEVEL10,
	LIGHT_LEVEL11,
	LIGHT_LEVEL12,
	LIGHT_LEVEL13,
	LIGHT_LEVEL14,
	LIGHT_LEVEL15,
	LIGHT_LEVEL16,
	LIGHT_INIT,
};
enum {
	D0_BND = 0,
	D0_COND1,
	D0_COND1_A,
	D0_COND1_B,
	D0_COND2,
	D0_COND2_A,
	D0_COND2_B,
	D0_COND3_A,
	D0_COND3_B,
};
struct gp2ap020_pdata {
	/*void (*power_on) (bool);
	void (*led_on) (bool);*/
	int p_out; /* proximity-sensor-output gpio (proximity interrupt) */
	int irq;
	u32 p_out_flags;
	u32 vdd_2p85;
	/*u32 vdd_en;*/
	int version; /* choice chip version 020 or 030 */
	u32 version_flags;
	const char *prox_cal_path;
	int d0_value[9];
	u8 thresh[2];
};

#endif
