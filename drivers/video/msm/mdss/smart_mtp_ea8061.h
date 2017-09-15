/*
 * =================================================================
 *
 *       Filename:  smart_mtp_se6e8fa.h
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
#ifndef _SMART_MTP_SE6E8FA_H_
#define _SMART_MTP_SE6E8FA_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <asm/div64.h>

#define VT232_ID 0x02
#define CCG6_ID 0x03
#define EVT1_ID 0x23
#define EVT1_SECOND_ID 0x44

/* EVT1_THIRD_ID & EVT1_FOUTRH_ID has same smart-dimming algo */
#define EVT1_THIRD_ID 0x25
#define EVT1_FOUTRH_ID 0x45

/* for REV H LDI */
#define EVT1_REV_H_ID3_1 0x46
#define EVT1_REV_H_ID3_2 0x26

/* for REV I LDI */
#define EVT1_REV_I_ID3_1 0x47
#define EVT1_REV_I_ID3_2 0x27

/* octa ldi id3 */
#define EVT0_ID 0x00
#define EVT0_SECOND_ID 0x01
#define EVT1_H_REV_I 0x23
#define EVT1_H_REV_J 0x24

/* youm ldi id3 */
#define EVT0_F_REV_A 0x10
#define EVT0_F_REV_E 0x11
#define EVT0_F_REV_F 0x12
#define EVT2_F_REV_G 0x32

#define EVT2_FRESCO_REV_G 0x43

/* EA8061V ldi id3 */
#define EVT0_EA8061V_REV_A 0x82
#define EVT2_EA8061V_REV_C 0x95
#define EVT2_EA8061V_REV_D 0x96
#define EVT2_EA8061V_REV_E 0x97
#define EVT0_EA8061V_KMINI_REV_A 0x84

/* EA8061 ldi id3 */
#define EVT2_EA8061_HESTIA_REV_I 0x47
#define EVT2_EA8061_HESTIA_REV_J 0x48
#define EVT2_EA8061_HESTIA_REV_A 0x40

/*
*	From 4.8 inch model use AID function
*	CASE#1 is used for now.
*/
#define AID_OPERATION

#define GAMMA_CURVE_2P25 1
#define GAMMA_CURVE_2P2 2
#define GAMMA_CURVE_2P15 3
#define GAMMA_CURVE_2P1 4
#define GAMMA_CURVE_2P0 5
#define GAMMA_CURVE_1P9 6

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
#define LUMINANCE_MAX 75
#define MTP_START_ADDR 0xFE
#else
#define LUMINANCE_MAX 72
#define MTP_START_ADDR 0xC8
#endif
#define GAMMA_SET_MAX 33
/*
 * Each of R, G, B have 1 count so the offset of VT is
 * GAMMA_SET_MAX -3
 */
#define VT_GAMMA_SET_MAX 3
#define VT_GAMMA_OFFSET	GAMMA_SET_MAX - VT_GAMMA_SET_MAX
enum {
	VT_GAMMA_OFFSET_R = 0,
	VT_GAMMA_OFFSET_G = 0,
	VT_GAMMA_OFFSET_B = 1,
};
enum {
	VT_GAMMA_BIT_SHIFT_R = 0,
	VT_GAMMA_BIT_SHIFT_G = 4,
	VT_GAMMA_BIT_SHIFT_B = 0,
};
enum {
	VT_GAMMA_BIT_MASK_R = 0xF,
	VT_GAMMA_BIT_MASK_G = 0xF,
	VT_GAMMA_BIT_MASK_B = 0xFF,
};
#define BIT_SHIFT 22
/*
	it means BIT_SHIFT is 22.  pow(2,BIT_SHIFT) is 4194304.
	BIT_SHIFT is used for right bit shfit
*/
#define BIT_SHFIT_MUL 4194304

#define S6E8FA_GRAY_SCALE_MAX 256

/*6.3*4194304 */
#define S6E8FA_VREG0_REF 26424115

/*6.1*4194304 */
#define EA8061_VREG0_REF_6P1 25585284
/*V0,V1,V3,V11,V23,V35,V51,V87,V151,V203,V255*/
#define S6E8FA_MAX 11

/* PANEL DEPENDENT THINGS */
#define MAX_CANDELA 350
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_HD_PANEL)
#define MIN_CANDELA	2
#else
#define MIN_CANDELA	5
#endif

/*
*	ID 0x20
*/
#define V255_300CD_R_MSB_20 0x01
#define V255_300CD_R_LSB_20 0x00

#define V255_300CD_G_MSB_20 0x01
#define V255_300CD_G_LSB_20 0x00

#define V255_300CD_B_MSB_20 0x01
#define V255_300CD_B_LSB_20 0x00

#define V203_300CD_R_20 0x80
#define V203_300CD_G_20 0x80
#define V203_300CD_B_20 0x80

#define V151_300CD_R_20 0x80
#define V151_300CD_G_20 0x80
#define V151_300CD_B_20 0x80

#define V87_300CD_R_20 0x80
#define V87_300CD_G_20 0x80
#define V87_300CD_B_20 0x80

#define V51_300CD_R_20 0x80
#define V51_300CD_G_20 0x80
#define V51_300CD_B_20 0x80

#define V35_300CD_R_20 0x80
#define V35_300CD_G_20 0x80
#define V35_300CD_B_20 0x80

#define V23_300CD_R_20 0x80
#define V23_300CD_G_20 0x80
#define V23_300CD_B_20 0x80

#define V11_300CD_R_20 0x80
#define V11_300CD_G_20 0x80
#define V11_300CD_B_20 0x80

#define V3_300CD_R_20 0x80
#define V3_300CD_G_20 0x80
#define V3_300CD_B_20 0x80

#define VT_300CD_R_20 0x00
#define VT_300CD_G_20 0x00
#define VT_300CD_B_20 0x00


/* PANEL DEPENDENT THINGS END*/

enum {
	V1_INDEX = 0,
	V3_INDEX = 1,
	V11_INDEX = 2,
	V23_INDEX = 3,
	V35_INDEX = 4,
	V51_INDEX = 5,
	V87_INDEX = 6,
	V151_INDEX = 7,
	V203_INDEX = 8,
	V255_INDEX = 9,
};

struct GAMMA_LEVEL {
	int level_0;
	int level_1;
	int level_3;
	int level_11;
	int level_23;
	int level_35;
	int level_51;
	int level_87;
	int level_151;
	int level_203;
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
	struct GRAY_VOLTAGE TABLE[S6E8FA_GRAY_SCALE_MAX];
	struct GRAY_VOLTAGE VT_TABLE;
} __packed;

/*V0,V1,V3,V11,V23,V35,V51,V87,V151,V203,V255*/

struct MTP_SET {
	char OFFSET_255_MSB;
	char OFFSET_255_LSB;
	char OFFSET_203;
	char OFFSET_151;
	char OFFSET_87;
	char OFFSET_51;
	char OFFSET_35;
	char OFFSET_23;
	char OFFSET_11;
	char OFFSET_3;
	char OFFSET_1;
} __packed;

#ifdef CONFIG_HBM_PSRE
struct MTP_OFFSET_400CD {
	struct MTP_SET R_OFFSET;
	struct MTP_SET G_OFFSET;
	struct MTP_SET B_OFFSET;
	char mtp_400cd[6];  /*gamma for 400cd*/
	char elvss_400cd;	/*elvss for 400cd*/
} __packed;
#endif

struct MTP_OFFSET {
	struct MTP_SET R_OFFSET;
	struct MTP_SET G_OFFSET;
	struct MTP_SET B_OFFSET;
} __packed;

struct illuminance_table {
	int lux;
	char gamma_setting[GAMMA_SET_MAX];
} __packed;

struct SMART_DIM {
#ifdef CONFIG_HBM_PSRE
	struct MTP_OFFSET_400CD MTP_ORIGN;
#else
	struct MTP_OFFSET MTP_ORIGN;
#endif
	struct MTP_OFFSET MTP;
	struct RGB_OUTPUT_VOLTARE RGB_OUTPUT;
	struct GRAY_SCALE GRAY;

	/* Because of AID funtion, below members are added*/
	int lux_table_max;
	int *plux_table;
	struct illuminance_table gen_table[LUMINANCE_MAX];

	int brightness_level;
	int ldi_revision;
	int vregout_voltage;
} __packed;

#endif
