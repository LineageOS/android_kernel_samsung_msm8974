/*
 * =================================================================
 *
 *       Filename:  smart_mtp_s6e8aa0x01.h
 *
 *    Description:  Smart dimming algorithm implementation
 *
 *        Author: jb09.kim
 *        Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2012, Samsung Electronics. All rights reserved.

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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
*/
#ifndef _SMART_MTP_S6E8AA0X01_H_
#define _SMART_MTP_S6E8AA0X01_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <asm/div64.h>

#if defined (CONFIG_FB_MSM_MDSS_S6E8AA0A_HD_PANEL)

enum {
	CI_RED = 0,
	CI_GREEN = 1,
	CI_BLUE = 2,
	CI_MAX = 3,
};

enum {
	IV_1 = 0,
	IV_15 = 1,
	IV_35 = 2,
	IV_59 = 3,
	IV_87 = 4,
	IV_171 = 5,
	IV_255 = 6,
	IV_MAX = 7,
	IV_TABLE_MAX = 8,
};

enum {
	AD_IV0 = 0,
	AD_IV1 = 1,
	AD_IV15 = 2,
	AD_IV35 = 3,
	AD_IV59 = 4,
	AD_IV87 = 5,
	AD_IV171 = 6,
	AD_IV255 = 7,
	AD_IVMAX = 8,
};

#define MAX_GRADATION   300

struct str_voltage_entry {
	u32 v[CI_MAX];
};

struct str_table_info {
	u8 st;
	u8 et;
	u8 count;
	u8 *offset_table;
	u32 rv;
};

struct str_flookup_table {
	u16 entry;
	u16 count;
};

struct str_smart_dim {
	s16 mtp[CI_MAX][IV_MAX];
	struct str_voltage_entry ve[256];
	u8 *default_gamma;
	struct str_table_info t_info[IV_TABLE_MAX];
	struct str_flookup_table *flooktbl;
	u32 *g22_tbl;
	u32 *g300_gra_tbl;
	u32 adjust_volt[CI_MAX][AD_IVMAX];
	s16 adjust_mtp[CI_MAX][IV_MAX];

};

int init_table_info(struct str_smart_dim *smart, unsigned char *srcGammaTable);
u8 calc_voltage_table(struct str_smart_dim *smart, const u8 * mtp);
u32 calc_gamma_table(struct str_smart_dim *smart, u32 gv, u8 result[]);
#endif

#if defined(CONFIG_MACH_STRETTO) || defined(CONFIG_MACH_SUPERIORLTE_SKT)
/*
*	STRETTO/SUPERIORLTE_SKT: 4.65 inch / no AID
*/
#else
/*
*	4.8 inch model use AID function
*	CASE#1 is used for now.
*/
#define AID_OPERATION_4_8_INCH
#define AID_CASE_1


/*
*	To set default AID algorithm
*/
#if !defined(AID_CASE_1) && !defined(AID_CASE_2) && !defined(AID_CASE_3)
#define AID_CASE_1
#endif
#endif

#define LUMINANCE_MAX 35
#define GAMMA_SET_MAX 24
#define BIT_SHIFT 14
/*
	it means BIT_SHIFT is 14.  pow(2,BIT_SHIFT) is 16384.
	BIT_SHIFT is used for right bit shfit
*/
#define BIT_SHFIT_MUL 16384

#define S6E8AA0X01_GRAY_SCALE_MAX 256

/*4.713*16384 */
#define S6E8AA0X01_VREG0_REF 77218

/*V0,V1,V15,V35,V59,V87,V177,V255*/
#define S6E8AA0X01_MAX 8

/* PANEL DEPENDENT THINGS */
#define MAX_CANDELA 300
#define MIN_CANDELA	20

/*
*	ID 0x20
*/
#define V1_300CD_R_20 0x43
#define V1_300CD_G_20 0x14
#define V1_300CD_B_20 0x45

#define V15_300CD_R_20 0xAD
#define V15_300CD_G_20 0xBE
#define V15_300CD_B_20 0xA9

#define V35_300CD_R_20 0xB0
#define V35_300CD_G_20 0xC3
#define V35_300CD_B_20 0xAF

#define V59_300CD_R_20 0xC1
#define V59_300CD_G_20 0xCD
#define V59_300CD_B_20 0xC0

#define V87_300CD_R_20 0x95
#define V87_300CD_G_20 0xA2
#define V87_300CD_B_20 0x91

#define V171_300CD_R_20 0xAC
#define V171_300CD_G_20 0xB5
#define V171_300CD_B_20 0xAA

#define V255_300CD_R_MSB_20 0x00
#define V255_300CD_R_LSB_20 0xB0

#define V255_300CD_G_MSB_20 0x00
#define V255_300CD_G_LSB_20 0xA0

#define V255_300CD_B_MSB_20 0x00
#define V255_300CD_B_LSB_20 0xCC

/*
*	ID 0x40
*/
#define V1_300CD_R_40 0x44
#define V1_300CD_G_40 0x0F
#define V1_300CD_B_40 0x42

#define V15_300CD_R_40 0xAB
#define V15_300CD_G_40 0xC1
#define V15_300CD_B_40 0xAC

#define V35_300CD_R_40 0xAF
#define V35_300CD_G_40 0xC8
#define V35_300CD_B_40 0xB3

#define V59_300CD_R_40 0xC2
#define V59_300CD_G_40 0xD0
#define V59_300CD_B_40 0xC1

#define V87_300CD_R_40 0x97
#define V87_300CD_G_40 0xA9
#define V87_300CD_B_40 0x95

#define V171_300CD_R_40 0xB3
#define V171_300CD_G_40 0xBE
#define V171_300CD_B_40 0xB3

#define V255_300CD_R_MSB_40 0x00
#define V255_300CD_R_LSB_40 0x9C

#define V255_300CD_G_MSB_40 0x00
#define V255_300CD_G_LSB_40 0x85

#define V255_300CD_B_MSB_40 0x00
#define V255_300CD_B_LSB_40 0xB2

/*
*	ID 0x60
*/
#define V1_300CD_R_60 0x3F
#define V1_300CD_G_60 0x12
#define V1_300CD_B_60 0x41

#define V15_300CD_R_60 0xB4
#define V15_300CD_G_60 0xCB
#define V15_300CD_B_60 0xB0

#define V35_300CD_R_60 0xB2
#define V35_300CD_G_60 0xC4
#define V35_300CD_B_60 0xB2

#define V59_300CD_R_60 0xC4
#define V59_300CD_G_60 0xCF
#define V59_300CD_B_60 0xC2

#define V87_300CD_R_60 0x9A
#define V87_300CD_G_60 0xA7
#define V87_300CD_B_60 0x97

#define V171_300CD_R_60 0xB2
#define V171_300CD_G_60 0xBA
#define V171_300CD_B_60 0xB0

#define V255_300CD_R_MSB_60 0x00
#define V255_300CD_R_LSB_60 0xA0

#define V255_300CD_G_MSB_60 0x00
#define V255_300CD_G_LSB_60 0x98

#define V255_300CD_B_MSB_60 0x00
#define V255_300CD_B_LSB_60 0xB7

#if defined(CONFIG_MACH_STRETTO) || defined(CONFIG_MACH_SUPERIORLTE_SKT)
/*
*	ID 0xAE
*/
#define V1_300CD_R_AE 0x5F
#define V1_300CD_G_AE 0x2E
#define V1_300CD_B_AE 0x67

#define V15_300CD_R_AE 0xAA
#define V15_300CD_G_AE 0xC6
#define V15_300CD_B_AE 0xAC

#define V35_300CD_R_AE 0xB0
#define V35_300CD_G_AE 0xC8
#define V35_300CD_B_AE 0xBB

#define V59_300CD_R_AE 0xBE
#define V59_300CD_G_AE 0xCB
#define V59_300CD_B_AE 0xBD

#define V87_300CD_R_AE 0x97
#define V87_300CD_G_AE 0xA5
#define V87_300CD_B_AE 0x91

#define V171_300CD_R_AE 0xAF
#define V171_300CD_G_AE 0xB8
#define V171_300CD_B_AE 0xAB

#define V255_300CD_R_MSB_AE 0x00
#define V255_300CD_R_LSB_AE 0xC2

#define V255_300CD_G_MSB_AE 0x00
#define V255_300CD_G_LSB_AE 0xBA

#define V255_300CD_B_MSB_AE 0x00
#define V255_300CD_B_LSB_AE 0xE2
#endif

/* PANEL DEPENDENT THINGS END*/


enum {
	V1_INDEX = 0,
	V15_INDEX = 1,
	V35_INDEX = 2,
	V59_INDEX = 3,
	V87_INDEX = 4,
	V171_INDEX = 5,
	V255_INDEX = 6,
};

struct GAMMA_LEVEL {
	int level_0;
	int level_1;
	int level_15;
	int level_35;
	int level_59;
	int level_87;
	int level_171;
	int level_255;
} __packed;

struct RGB_OUTPUT_VOLTARE {
	struct GAMMA_LEVEL R_VOLTAGE;
	struct GAMMA_LEVEL G_VOLTAGE;
	struct GAMMA_LEVEL B_VOLTAGE;
} __packed;

struct GRAY_VOLTAGE {
	/*
		This voltage value use 14bit right shit
		it means voltage is divied by 16384.
	*/
	int R_Gray;
	int G_Gray;
	int B_Gray;
} __packed;

struct GRAY_SCALE {
	struct GRAY_VOLTAGE TABLE[S6E8AA0X01_GRAY_SCALE_MAX];
} __packed;

struct MTP_SET {
	char OFFSET_1;
	char OFFSET_15;
	char OFFSET_35;
	char OFFSET_59;
	char OFFSET_87;
	char OFFSET_171;
	char OFFSET_255_MSB;
	char OFFSET_255_LSB;
} __packed;

struct MTP_OFFSET {
	/*
		MTP_OFFSET is consist of 22 byte.
		First byte is dummy and 21 byte is useful.
	*/
	struct MTP_SET R_OFFSET;
	struct MTP_SET G_OFFSET;
	struct MTP_SET B_OFFSET;
} __packed;

struct illuminance_table {
	int lux;
	char gamma_setting[GAMMA_SET_MAX];
} __packed;

struct SMART_DIM {
	struct MTP_OFFSET MTP;
	struct RGB_OUTPUT_VOLTARE RGB_OUTPUT;
	struct GRAY_SCALE GRAY;

	/* Because of AID funtion, below members are added*/
	int lux_table_max;
	int *plux_table;
	struct illuminance_table gen_table[LUMINANCE_MAX];

	int brightness_level;
	int ldi_revision;
} __packed;


int smart_dimming_init(struct SMART_DIM *psmart);
void get_min_lux_table(char *str, int size);
void generate_gamma(struct SMART_DIM *psmart, char *str, int size);


#endif
