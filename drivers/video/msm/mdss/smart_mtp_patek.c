/*
 * =================================================================
 *
 *       Filename:  smart_mtp_patek.c
 *
 *    Description:  Smart dimming algorithm implementation
 *
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

#include "smart_mtp_patek.h"
#include "smart_mtp_2p2_gamma.h"
#include "smart_dimming.h"

static struct SMART_DIM smart_EA8061V_main;
static struct SMART_DIM smart_EA8061V_sub;

static struct smartdim_conf __EA8061V_MAIN__;
static struct smartdim_conf __EA8061V_SUB__;


//#define SMART_DIMMING_DEBUG

static char max_lux_table[GAMMA_SET_MAX];
static char min_lux_table[GAMMA_SET_MAX];

static int *temp_candela_coeff;

static int id3;

static int char_to_int(char data1)
{
	int cal_data;

	if (data1 & 0x80) {
		cal_data = data1 & 0x7F;
		cal_data *= -1;
	} else
		cal_data = data1;

	return cal_data;
}

static int char_to_int_v255(char data1, char data2)
{
	int cal_data;

	if (data1)
		cal_data = data2 * -1;
	else
		cal_data = data2;

	return cal_data;
}

void print_RGB_offset(struct SMART_DIM *pSmart)
{
	pr_info("%s MTP Offset VT R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_1),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_1),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_1));
	pr_info("%s MTP Offset V3 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_3),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_3),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_3));
	pr_info("%s MTP Offset V11 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_11),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_11),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_11));
	pr_info("%s MTP Offset V23 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_23),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_23),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_23));
	pr_info("%s MTP Offset V35 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_35),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_35),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_35));
	pr_info("%s MTP Offset V51 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_51),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_51),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_51));
	pr_info("%s MTP Offset V87 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_87),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_87),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_87));
	pr_info("%s MTP Offset V151 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_151),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_151),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_151));
	pr_info("%s MTP Offset V203 R:%d G:%d B:%d\n", __func__,
			char_to_int(pSmart->MTP.R_OFFSET.OFFSET_203),
			char_to_int(pSmart->MTP.G_OFFSET.OFFSET_203),
			char_to_int(pSmart->MTP.B_OFFSET.OFFSET_203));
	pr_info("%s MTP Offset V255 R:%d G:%d B:%d\n", __func__,
			char_to_int_v255(pSmart->MTP.R_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.R_OFFSET.OFFSET_255_LSB),
			char_to_int_v255(pSmart->MTP.G_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.G_OFFSET.OFFSET_255_LSB),
			char_to_int_v255(pSmart->MTP.B_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.B_OFFSET.OFFSET_255_LSB));
}

void print_lux_table(struct SMART_DIM *psmart)
{
	int lux_loop;
	int cnt;
	char pBuffer[256];
	memset(pBuffer, 0x00, 256);

	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
			snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %d",
				psmart->gen_table[lux_loop].gamma_setting[cnt]);

		pr_info("lux : %3d  %s", psmart->plux_table[lux_loop], pBuffer);
		memset(pBuffer, 0x00, 256);
	}
}

void print_aid_log_main(void)
{
	print_RGB_offset(&smart_EA8061V_main);
	print_lux_table(&smart_EA8061V_main);
}

void print_aid_log_sub(void)
{
	print_RGB_offset(&smart_EA8061V_sub);
	print_lux_table(&smart_EA8061V_sub);
}

#define v255_coefficient 72
#define v255_denominator 860
static int v255_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;
	int v255_value;

	v255_value = (V255_300CD_R_MSB << 8) | (V255_300CD_R_LSB);
	LSB = char_to_int_v255(pSmart->MTP.R_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.R_OFFSET.OFFSET_255_LSB);
	add_mtp = LSB + v255_value;
	result_1 = result_2 = (v255_coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, v255_denominator);
	result_3 = (pSmart->vregout_voltage * result_2) >> BIT_SHIFT;
	result_4 = pSmart->vregout_voltage - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_255 = result_4;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_0 = pSmart->vregout_voltage;

	v255_value = (V255_300CD_G_MSB << 8) | (V255_300CD_G_LSB);
	LSB = char_to_int_v255(pSmart->MTP.G_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.G_OFFSET.OFFSET_255_LSB);
	add_mtp = LSB + v255_value;
	result_1 = result_2 = (v255_coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, v255_denominator);
	result_3 = (pSmart->vregout_voltage * result_2) >> BIT_SHIFT;
	result_4 = pSmart->vregout_voltage - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_255 = result_4;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_0 = pSmart->vregout_voltage;

	v255_value = (V255_300CD_B_MSB << 8) | (V255_300CD_B_LSB);
	LSB = char_to_int_v255(pSmart->MTP.B_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.B_OFFSET.OFFSET_255_LSB);
	add_mtp = LSB + v255_value;
	result_1 = result_2 = (v255_coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, v255_denominator);
	result_3 = (pSmart->vregout_voltage * result_2) >> BIT_SHIFT;
	result_4 = pSmart->vregout_voltage - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_255 = result_4;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_0 = pSmart->vregout_voltage;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V255 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_255,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_255,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_255);
#endif

	return 0;
}

static void v255_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = pSmart->vregout_voltage -
		(pSmart->GRAY.TABLE[index[V255_INDEX]].R_Gray);
	result_2 = result_1 * v255_denominator;
	do_div(result_2, pSmart->vregout_voltage);
	result_3 = result_2  - v255_coefficient;
	str[0] = (result_3 & 0xff00) >> 8;
	str[1] = result_3 & 0xff;

	result_1 = pSmart->vregout_voltage -
		(pSmart->GRAY.TABLE[index[V255_INDEX]].G_Gray);
	result_2 = result_1 * v255_denominator;
	do_div(result_2, pSmart->vregout_voltage);
	result_3 = result_2  - v255_coefficient;
	str[2] = (result_3 & 0xff00) >> 8;
	str[3] = result_3 & 0xff;

	result_1 = pSmart->vregout_voltage -
			(pSmart->GRAY.TABLE[index[V255_INDEX]].B_Gray);
	result_2 = result_1 * v255_denominator;
	do_div(result_2, pSmart->vregout_voltage);
	result_3 = result_2  - v255_coefficient;
	str[4] = (result_3 & 0xff00) >> 8;
	str[5] = result_3 & 0xff;

}

static int vt_coefficient[] = {
	0, 12, 24, 36, 48,
	60, 72, 84, 96, 108,
	138, 148, 158, 168,
	178, 186,
};
#define vt_denominator 860
static int vt_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_1);
	add_mtp = LSB + VT_300CD_R;
	result_1 = result_2 = vt_coefficient[LSB] << BIT_SHIFT;
	do_div(result_2, vt_denominator);
	result_3 = (pSmart->vregout_voltage * result_2) >> BIT_SHIFT;
	result_4 = pSmart->vregout_voltage - result_3;
	pSmart->GRAY.VT_TABLE.R_Gray = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_1);
	add_mtp = LSB + VT_300CD_G;
	result_1 = result_2 = vt_coefficient[LSB] << BIT_SHIFT;
	do_div(result_2, vt_denominator);
	result_3 = (pSmart->vregout_voltage * result_2) >> BIT_SHIFT;
	result_4 = pSmart->vregout_voltage - result_3;
	pSmart->GRAY.VT_TABLE.G_Gray = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_1);
	add_mtp = LSB + VT_300CD_B;
	result_1 = result_2 = vt_coefficient[LSB] << BIT_SHIFT;
	do_div(result_2, vt_denominator);
	result_3 = (pSmart->vregout_voltage * result_2) >> BIT_SHIFT;
	result_4 = pSmart->vregout_voltage - result_3;
	pSmart->GRAY.VT_TABLE.B_Gray = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s VT RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->GRAY.VT_TABLE.R_Gray,
			pSmart->GRAY.VT_TABLE.G_Gray,
			pSmart->GRAY.VT_TABLE.B_Gray);
#endif

	return 0;

}

static void vt_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	str[30] = VT_300CD_R;
	str[31] = VT_300CD_G;
	str[32] = VT_300CD_B;
}

#define v203_coefficient 64
#define v203_denominator 320
static int v203_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_203);
	add_mtp = LSB + V203_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
				- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_255);
	result_2 = (v203_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v203_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_203 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_203);
	add_mtp = LSB + V203_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
				- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_255);
	result_2 = (v203_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v203_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_203 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_203);
	add_mtp = LSB + V203_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
				- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_255);
	result_2 = (v203_coefficient+add_mtp) << BIT_SHIFT;
	do_div(result_2, v203_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_203 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V203 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_203,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_203,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_203);
#endif

	return 0;

}

static void v203_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V203_INDEX]].R_Gray);
	result_2 = result_1 * v203_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V255_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[6] = (result_2  - v203_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V203_INDEX]].G_Gray);
	result_2 = result_1 * v203_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V255_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[7] = (result_2  - v203_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V203_INDEX]].B_Gray);
	result_2 = result_1 * v203_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V255_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[8] = (result_2  - v203_coefficient) & 0xff;

}

#define v151_coefficient 64
#define v151_denominator 320
static int v151_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_151);
	add_mtp = LSB + V151_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_203);
	result_2 = (v151_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v151_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_151 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_151);
	add_mtp = LSB + V151_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_203);
	result_2 = (v151_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v151_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_151 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_151);
	add_mtp = LSB + V151_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_203);
	result_2 = (v151_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v151_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_151 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V151 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_151,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_151,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_151);
#endif

	return 0;

}

static void v151_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V151_INDEX]].R_Gray);
	result_2 = result_1 * v151_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V203_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[9] = (result_2  - v151_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V151_INDEX]].G_Gray);
	result_2 = result_1 * v151_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V203_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[10] = (result_2  - v151_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V151_INDEX]].B_Gray);
	result_2 = result_1 * v151_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V203_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[11] = (result_2  - v151_coefficient) & 0xff;
}

#define v87_coefficient 64
#define v87_denominator 320
static int v87_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_87);
	add_mtp = LSB + V87_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_151);
	result_2 = (v87_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v87_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_87 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_87);
	add_mtp = LSB + V87_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_151);
	result_2 = (v87_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v87_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_87 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_87);
	add_mtp = LSB + V87_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_151);
	result_2 = (v87_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v87_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_87 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V87 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_87,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_87,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_87);
#endif

	return 0;

}

static void v87_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].R_Gray);
	result_2 = result_1 * v87_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V151_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[12] = (result_2  - v87_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].G_Gray);
	result_2 = result_1 * v87_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V151_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[13] = (result_2  - v87_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].B_Gray);
	result_2 = result_1 * v87_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V151_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[14] = (result_2  - v87_coefficient) & 0xff;
}

#define v51_coefficient 64
#define v51_denominator 320
static int v51_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_51);
	add_mtp = LSB + V51_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_87);
	result_2 = (v51_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v51_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_51 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_51);
	add_mtp = LSB + V51_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_87);
	result_2 = (v51_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v51_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_51 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_51);
	add_mtp = LSB + V51_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_87);
	result_2 = (v51_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v51_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_51 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V51 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_51,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_51,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_51);
#endif

	return 0;

}

static void v51_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V51_INDEX]].R_Gray);
	result_2 = result_1 * v51_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[15] = (result_2  - v51_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V51_INDEX]].G_Gray);
	result_2 = result_1 * v51_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[16] = (result_2  - v51_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V51_INDEX]].B_Gray);
	result_2 = result_1 * v51_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V87_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[17] = (result_2  - v51_coefficient) & 0xff;

}

#define v35_coefficient 64
#define v35_denominator 320
static int v35_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_35);
	add_mtp = LSB + V35_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_51);
	result_2 = (v35_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v35_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_35 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_35);
	add_mtp = LSB + V35_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_51);
	result_2 = (v35_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v35_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_35 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_35);
	add_mtp = LSB + V35_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_51);
	result_2 = (v35_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v35_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_35 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V35 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_35,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_35,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_35);
#endif

	return 0;

}

static void v35_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].R_Gray);
	result_2 = result_1 * v35_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V51_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[18] = (result_2  - v35_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].G_Gray);
	result_2 = result_1 * v35_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V51_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[19] = (result_2  - v35_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].B_Gray);
	result_2 = result_1 * v35_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V51_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[20] = (result_2  - v35_coefficient) & 0xff;

}

#define v23_coefficient 64
#define v23_denominator 320
static int v23_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_23);
	add_mtp = LSB + V23_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_35);
	result_2 = (v23_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v23_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_23 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_23);
	add_mtp = LSB + V23_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_35);
	result_2 = (v23_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v23_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_23 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_23);
	add_mtp = LSB + V23_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_35);
	result_2 = (v23_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v23_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_23 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V23 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_23,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_23,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_23);
#endif

	return 0;

}

static void v23_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V23_INDEX]].R_Gray);
	result_2 = result_1 * v23_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[21] = (result_2  - v23_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V23_INDEX]].G_Gray);
	result_2 = result_1 * v23_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[22] = (result_2  - v23_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V23_INDEX]].B_Gray);
	result_2 = result_1 * v23_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V35_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[23] = (result_2  - v23_coefficient) & 0xff;

}

#define v11_coefficient 64
#define v11_denominator 320
static int v11_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_11);
	add_mtp = LSB + V11_300CD_R;
	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_23);
	result_2 = (v11_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v11_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.R_Gray) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_11 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_11);
	add_mtp = LSB + V11_300CD_G;
	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_23);
	result_2 = (v11_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v11_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.G_Gray) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_11 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_11);
	add_mtp = LSB + V11_300CD_B;
	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_23);
	result_2 = (v11_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v11_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->GRAY.VT_TABLE.B_Gray) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_11 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V11 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_11,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_11,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_11);
#endif

	return 0;

}

static void v11_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V11_INDEX]].R_Gray);
	result_2 = result_1 * v11_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.R_Gray)
			- (pSmart->GRAY.TABLE[index[V23_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[24] = (result_2  - v11_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V11_INDEX]].G_Gray);
	result_2 = result_1 * v11_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.G_Gray)
			- (pSmart->GRAY.TABLE[index[V23_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[25] = (result_2  - v11_coefficient) & 0xff;

	result_1 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V11_INDEX]].B_Gray);
	result_2 = result_1 * v11_denominator;
	result_3 = (pSmart->GRAY.VT_TABLE.B_Gray)
			- (pSmart->GRAY.TABLE[index[V23_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[26] = (result_2  - v11_coefficient) & 0xff;

}

#define v3_coefficient 64
#define v3_denominator 320
static int v3_adjustment(struct SMART_DIM *pSmart)
{
	unsigned long long result_1, result_2, result_3, result_4;
	int add_mtp;
	int LSB;

	LSB = char_to_int(pSmart->MTP.R_OFFSET.OFFSET_3);
	add_mtp = LSB + V3_300CD_R;
	result_1 = (pSmart->vregout_voltage)
			- (pSmart->RGB_OUTPUT.R_VOLTAGE.level_11);
	result_2 = (v3_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v3_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->vregout_voltage) - result_3;
	pSmart->RGB_OUTPUT.R_VOLTAGE.level_3 = result_4;

	LSB = char_to_int(pSmart->MTP.G_OFFSET.OFFSET_3);
	add_mtp = LSB + V3_300CD_G;
	result_1 = (pSmart->vregout_voltage)
			- (pSmart->RGB_OUTPUT.G_VOLTAGE.level_11);
	result_2 = (v3_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v3_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->vregout_voltage) - result_3;
	pSmart->RGB_OUTPUT.G_VOLTAGE.level_3 = result_4;

	LSB = char_to_int(pSmart->MTP.B_OFFSET.OFFSET_3);
	add_mtp = LSB + V3_300CD_B;
	result_1 = (pSmart->vregout_voltage)
			- (pSmart->RGB_OUTPUT.B_VOLTAGE.level_11);
	result_2 = (v3_coefficient + add_mtp) << BIT_SHIFT;
	do_div(result_2, v3_denominator);
	result_3 = (result_1 * result_2) >> BIT_SHIFT;
	result_4 = (pSmart->vregout_voltage) - result_3;
	pSmart->RGB_OUTPUT.B_VOLTAGE.level_3 = result_4;

#ifdef SMART_DIMMING_DEBUG
	pr_info("%s V3 RED:%d GREEN:%d BLUE:%d\n", __func__,
			pSmart->RGB_OUTPUT.R_VOLTAGE.level_3,
			pSmart->RGB_OUTPUT.G_VOLTAGE.level_3,
			pSmart->RGB_OUTPUT.B_VOLTAGE.level_3);
#endif

	return 0;

}

static void v3_hexa(int *index, struct SMART_DIM *pSmart, char *str)
{
	unsigned long long result_1, result_2, result_3;

	result_1 = (pSmart->vregout_voltage)
			- (pSmart->GRAY.TABLE[index[V3_INDEX]].R_Gray);
	result_2 = result_1 * v3_denominator;
	result_3 = (pSmart->vregout_voltage)
			- (pSmart->GRAY.TABLE[index[V11_INDEX]].R_Gray);
	do_div(result_2, result_3);
	str[27] = (result_2  - v3_coefficient) & 0xff;

	result_1 = (pSmart->vregout_voltage)
			- (pSmart->GRAY.TABLE[index[V3_INDEX]].G_Gray);
	result_2 = result_1 * v3_denominator;
	result_3 = (pSmart->vregout_voltage)
			- (pSmart->GRAY.TABLE[index[V11_INDEX]].G_Gray);
	do_div(result_2, result_3);
	str[28] = (result_2  - v3_coefficient) & 0xff;

	result_1 = (pSmart->vregout_voltage)
			- (pSmart->GRAY.TABLE[index[V3_INDEX]].B_Gray);
	result_2 = result_1 * v3_denominator;
	result_3 = (pSmart->vregout_voltage)
			- (pSmart->GRAY.TABLE[index[V11_INDEX]].B_Gray);
	do_div(result_2, result_3);
	str[29] = (result_2  - v3_coefficient) & 0xff;

}


/*V0,V3,V11,V23,V35,V51,V87,V151,V203,V255*/
static int TP_ARRAY[TP_MAX] = {0, 3, 11, 23, 35, 51, 87, 151, 203, 255};

#define V0toV3_Coefficient 2
#define V0toV3_Multiple 1
#define V0toV3_denominator 3

#define V3toV11_Coefficient 7
#define V3toV11_Multiple 1
#define V3toV11_denominator 8

#define V11toV23_Coefficient 11
#define V11toV23_Multiple 1
#define V11toV23_denominator 12

#define V23toV35_Coefficient 11
#define V23toV35_Multiple 1
#define V23toV35_denominator 12

#define V35toV51_Coefficient 15
#define V35toV51_Multiple 1
#define V35toV51_denominator 16

#define V51toV87_Coefficient 35
#define V51toV87_Multiple 1
#define V51toV87_denominator 36

#define V87toV151_Coefficient 63
#define V87toV151_Multiple 1
#define V87toV151_denominator 64

#define V151toV203_Coefficient 51
#define V151toV203_Multiple 1
#define V151toV203_denominator 52

#define V203toV255_Coefficient 51
#define V203toV255_Multiple 1
#define V203toV255_denominator 52

static int cal_gray_scale_linear(int up, int low, int coeff,
int mul, int deno, int cnt)
{
	unsigned long long result_1, result_2, result_3, result_4;

	result_1 = up - low;
	result_2 = (result_1 * (coeff - (cnt * mul))) << BIT_SHIFT;
	do_div(result_2, deno);
	result_3 = result_2 >> BIT_SHIFT;
	result_4 = low + result_3;

	return (int)result_4;
}

static int generate_gray_scale(struct SMART_DIM *pSmart)
{
	int cnt = 0, cal_cnt = 0;
	int array_index = 0;
	struct GRAY_VOLTAGE *ptable = (struct GRAY_VOLTAGE *)
						(&(pSmart->GRAY.TABLE));

	pr_info("%s ++ \n", __func__);
	for (cnt = 0; cnt < TP_MAX; cnt++) {
		pSmart->GRAY.TABLE[TP_ARRAY[cnt]].R_Gray =
			((int *)&(pSmart->RGB_OUTPUT.R_VOLTAGE))[cnt];

		pSmart->GRAY.TABLE[TP_ARRAY[cnt]].G_Gray =
			((int *)&(pSmart->RGB_OUTPUT.G_VOLTAGE))[cnt];

		pSmart->GRAY.TABLE[TP_ARRAY[cnt]].B_Gray =
			((int *)&(pSmart->RGB_OUTPUT.B_VOLTAGE))[cnt];
	}


#ifdef SMART_DIMMING_DEBUG
		for (cnt = 0; cnt < TP_MAX; cnt++) {
			pr_info("1 %s %8d %8d %8d %d\n", __func__,
				ptable[TP_ARRAY[cnt]].R_Gray,
				ptable[TP_ARRAY[cnt]].G_Gray,
				ptable[TP_ARRAY[cnt]].B_Gray, cnt);
		}
#endif

	/*
		below codes use hard coded value.
		So it is possible to modify on each model.
		V0,V1,V3,V11,V23,V35,V51,V87,V151,V203,V255
	*/
	for (cnt = 0; cnt < GRAY_SCALE_MAX; cnt++) {

		if (cnt == TP_ARRAY[0]) {
			/* 0 */
			array_index = 0;
			cal_cnt = 0;
		} else if ((cnt > TP_ARRAY[0]) &&
			(cnt < TP_ARRAY[1])) {
			/* 1 ~ 2 */
			array_index = 1;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].R_Gray,
			ptable[TP_ARRAY[array_index]].R_Gray,
			V0toV3_Coefficient, V0toV3_Multiple,
			V0toV3_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].G_Gray,
			ptable[TP_ARRAY[array_index]].G_Gray,
			V0toV3_Coefficient, V0toV3_Multiple,
			V0toV3_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].B_Gray,
			ptable[TP_ARRAY[array_index]].B_Gray,
			V0toV3_Coefficient, V0toV3_Multiple,
			V0toV3_denominator , cal_cnt);

			cal_cnt++;
		} else if (cnt == TP_ARRAY[1]) {
			/* 3 */
			cal_cnt = 0;
		} else if ((cnt > TP_ARRAY[1]) &&
			(cnt < TP_ARRAY[2])) {
			/* 4 ~ 10 */
			array_index = 2;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].R_Gray,
			ptable[TP_ARRAY[array_index]].R_Gray,
			V3toV11_Coefficient, V3toV11_Multiple,
			V3toV11_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].G_Gray,
			ptable[TP_ARRAY[array_index]].G_Gray,
			V3toV11_Coefficient, V3toV11_Multiple,
			V3toV11_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].B_Gray,
			ptable[TP_ARRAY[array_index]].B_Gray,
			V3toV11_Coefficient, V3toV11_Multiple,
			V3toV11_denominator , cal_cnt);

			cal_cnt++;
		} else if (cnt == TP_ARRAY[2]) {
			/* 11 */
			cal_cnt = 0;
		} else if ((cnt > TP_ARRAY[2]) &&
			(cnt < TP_ARRAY[3])) {
			/* 12 ~ 22 */
			array_index = 3;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].R_Gray,
			ptable[TP_ARRAY[array_index]].R_Gray,
			V11toV23_Coefficient, V11toV23_Multiple,
			V11toV23_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].G_Gray,
			ptable[TP_ARRAY[array_index]].G_Gray,
			V11toV23_Coefficient, V11toV23_Multiple,
			V11toV23_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].B_Gray,
			ptable[TP_ARRAY[array_index]].B_Gray,
			V11toV23_Coefficient, V11toV23_Multiple,
			V11toV23_denominator , cal_cnt);

			cal_cnt++;
		}  else if (cnt == TP_ARRAY[3]) {
			/* 23 */
			cal_cnt = 0;
		} else if ((cnt > TP_ARRAY[3]) &&
			(cnt < TP_ARRAY[4])) {
			/* 24 ~ 34 */
			array_index = 4;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].R_Gray,
			ptable[TP_ARRAY[array_index]].R_Gray,
			V23toV35_Coefficient, V23toV35_Multiple,
			V23toV35_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].G_Gray,
			ptable[TP_ARRAY[array_index]].G_Gray,
			V23toV35_Coefficient, V23toV35_Multiple,
			V23toV35_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].B_Gray,
			ptable[TP_ARRAY[array_index]].B_Gray,
			V23toV35_Coefficient, V23toV35_Multiple,
			V23toV35_denominator , cal_cnt);

			cal_cnt++;
		} else if (cnt == TP_ARRAY[4]) {
			/* 35 */
			cal_cnt = 0;
		} else if ((cnt > TP_ARRAY[4]) &&
			(cnt < TP_ARRAY[5])) {
			/* 36 ~ 50 */
			array_index = 5;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].R_Gray,
			ptable[TP_ARRAY[array_index]].R_Gray,
			V35toV51_Coefficient, V35toV51_Multiple,
			V35toV51_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].G_Gray,
			ptable[TP_ARRAY[array_index]].G_Gray,
			V35toV51_Coefficient, V35toV51_Multiple,
			V35toV51_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].B_Gray,
			ptable[TP_ARRAY[array_index]].B_Gray,
			V35toV51_Coefficient, V35toV51_Multiple,
			V35toV51_denominator, cal_cnt);
			cal_cnt++;

		} else if (cnt == TP_ARRAY[5]) {
			/* 51 */
			cal_cnt = 0;
		} else if ((cnt > TP_ARRAY[5]) &&
			(cnt < TP_ARRAY[6])) {
			/* 52 ~ 86 */
			array_index = 6;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].R_Gray,
			ptable[TP_ARRAY[array_index]].R_Gray,
			V51toV87_Coefficient, V51toV87_Multiple,
			V51toV87_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].G_Gray,
			ptable[TP_ARRAY[array_index]].G_Gray,
			V51toV87_Coefficient, V51toV87_Multiple,
			V51toV87_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].B_Gray,
			ptable[TP_ARRAY[array_index]].B_Gray,
			V51toV87_Coefficient, V51toV87_Multiple,
			V51toV87_denominator, cal_cnt);
			cal_cnt++;

		} else if (cnt == TP_ARRAY[6]) {
			/* 87 */
			cal_cnt = 0;
		} else if ((cnt > TP_ARRAY[6]) &&
			(cnt < TP_ARRAY[7])) {
			/* 88 ~ 150 */
			array_index = 7;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].R_Gray,
			ptable[TP_ARRAY[array_index]].R_Gray,
			V87toV151_Coefficient, V87toV151_Multiple,
			V87toV151_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].G_Gray,
			ptable[TP_ARRAY[array_index]].G_Gray,
			V87toV151_Coefficient, V87toV151_Multiple,
			V87toV151_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].B_Gray,
			ptable[TP_ARRAY[array_index]].B_Gray,
			V87toV151_Coefficient, V87toV151_Multiple,
			V87toV151_denominator, cal_cnt);

			cal_cnt++;
		} else if (cnt == TP_ARRAY[7]) {
			/* 151 */
			cal_cnt = 0;
		} else if ((cnt > TP_ARRAY[7]) &&
			(cnt < TP_ARRAY[8])) {
			/* 152 ~ 202 */
			array_index = 8;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].R_Gray,
			ptable[TP_ARRAY[array_index]].R_Gray,
			V151toV203_Coefficient, V151toV203_Multiple,
			V151toV203_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].G_Gray,
			ptable[TP_ARRAY[array_index]].G_Gray,
			V151toV203_Coefficient, V151toV203_Multiple,
			V151toV203_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].B_Gray,
			ptable[TP_ARRAY[array_index]].B_Gray,
			V151toV203_Coefficient, V151toV203_Multiple,
			V151toV203_denominator, cal_cnt);

			cal_cnt++;
		} else if (cnt == TP_ARRAY[8]) {
			/* 203 */
			cal_cnt = 0;
		} else if ((cnt > TP_ARRAY[8]) &&
			(cnt < TP_ARRAY[9])) {
			/* 204 ~ 254 */
			array_index = 9;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].R_Gray,
			ptable[TP_ARRAY[array_index]].R_Gray,
			V203toV255_Coefficient, V203toV255_Multiple,
			V203toV255_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].G_Gray,
			ptable[TP_ARRAY[array_index]].G_Gray,
			V203toV255_Coefficient, V203toV255_Multiple,
			V203toV255_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[TP_ARRAY[array_index-1]].B_Gray,
			ptable[TP_ARRAY[array_index]].B_Gray,
			V203toV255_Coefficient, V203toV255_Multiple,
			V203toV255_denominator, cal_cnt);

			cal_cnt++;
		 } else {
			if (cnt == TP_ARRAY[9]) {
				pr_debug("%s end\n", __func__);
			} else {
				pr_err("%s fail cnt:%d\n", __func__, cnt);
				return -EINVAL;
			}
		}

	}

#ifdef SMART_DIMMING_DEBUG
		for (cnt = 0; cnt < GRAY_SCALE_MAX; cnt++) {
			pr_info("%s %8d %8d %8d %d\n", __func__,
				pSmart->GRAY.TABLE[cnt].R_Gray,
				pSmart->GRAY.TABLE[cnt].G_Gray,
				pSmart->GRAY.TABLE[cnt].B_Gray, cnt);
		}
#endif

	pr_info("%s -- \n", __func__);

	return 0;
}

char offset_cal(int offset,  char value)
{
	unsigned char real_value;

	if (value < 0 )
		real_value = value * -1;
	else
		real_value = value;

	if (real_value - offset < 0)
		return 0;
	else if (real_value - offset > 255)
		return 0xFF;
	else
		return real_value - offset;
}

static void mtp_offset_substraction(struct SMART_DIM *pSmart, char *str)
{
	int level_255_temp = 0;
	int level_255_temp_MSB = 0;
	int MTP_V255;

	/*subtration MTP_OFFSET value from generated gamma table*/
	level_255_temp = (str[0] << 8) | str[1] ;
	MTP_V255 = char_to_int_v255(pSmart->MTP.R_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.R_OFFSET.OFFSET_255_LSB);
	level_255_temp -=  MTP_V255;
	level_255_temp_MSB = level_255_temp / 256;
	str[0] = level_255_temp_MSB & 0xff;
	str[1] = level_255_temp & 0xff;

	level_255_temp = (str[2] << 8) | str[3] ;
	MTP_V255 = char_to_int_v255(pSmart->MTP.G_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.G_OFFSET.OFFSET_255_LSB);
	level_255_temp -=  MTP_V255;
	level_255_temp_MSB = level_255_temp / 256;
	str[2] = level_255_temp_MSB & 0xff;
	str[3] = level_255_temp & 0xff;

	level_255_temp = (str[4] << 8) | str[5] ;
	MTP_V255 = char_to_int_v255(pSmart->MTP.B_OFFSET.OFFSET_255_MSB,
				pSmart->MTP.B_OFFSET.OFFSET_255_LSB);
	level_255_temp -=  MTP_V255;
	level_255_temp_MSB = level_255_temp / 256;
	str[4] = level_255_temp_MSB & 0xff;
	str[5] = level_255_temp & 0xff;

	str[6] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_203), str[6]);
	str[7] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_203), str[7]);
	str[8] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_203), str[8]);

	str[9] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_151), str[9]);
	str[10] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_151), str[10]);
	str[11] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_151), str[11]);

	str[12] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_87), str[12]);
	str[13] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_87), str[13]);
	str[14] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_87), str[14]);

	str[15] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_51), str[15]);
	str[16] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_51), str[16]);
	str[17] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_51), str[17]);

	str[18] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_35), str[18]);
	str[19] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_35), str[19]);
	str[20] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_35), str[20]);

	str[21] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_23), str[21]);
	str[22] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_23), str[22]);
	str[23] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_23), str[23]);

	str[24] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_11), str[24]);
	str[25] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_11), str[25]);
	str[26] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_11), str[26]);

	str[27] = offset_cal(char_to_int(pSmart->MTP.R_OFFSET.OFFSET_3), str[27]);
	str[28] = offset_cal(char_to_int(pSmart->MTP.G_OFFSET.OFFSET_3), str[28]);
	str[29] = offset_cal(char_to_int(pSmart->MTP.B_OFFSET.OFFSET_3), str[29]);
}

static int searching_function(long long candela, int *index, int gamma_curve)
{
	long long delta_1 = 0, delta_2 = 0;
	int cnt;

	/*
	*	This searching_functin should be changed with improved
		searcing algorithm to reduce searching time.
	*/
	*index = -1;

	for (cnt = 0; cnt < (GRAY_SCALE_MAX-1); cnt++) {
		if (gamma_curve == GAMMA_CURVE_1P9) {
			delta_1 = candela - curve_1p9_360[cnt];
			delta_2 = candela - curve_1p9_360[cnt+1];
		} else if (gamma_curve == GAMMA_CURVE_2P15) {
			delta_1 = candela - curve_2p15_360[cnt];
			delta_2 = candela - curve_2p15_360[cnt+1];
		} else if (gamma_curve == GAMMA_CURVE_2P2) {
			delta_1 = candela - curve_2p2_360[cnt];
			delta_2 = candela - curve_2p2_360[cnt+1];
		} else {
			delta_1 = candela - curve_2p2_360[cnt];
			delta_2 = candela - curve_2p2_360[cnt+1];
		}

		if (delta_2 < 0) {
			*index = (delta_1 + delta_2) <= 0 ? cnt : cnt+1;
			break;
		}

		if (delta_1 == 0) {
			*index = cnt;
			break;
		}

		if (delta_2 == 0) {
			*index = cnt+1;
			break;
		}
	}

	if (*index == -1)
		return -EINVAL;
	else
		return 0;
}

static void(*Make_hexa[TP_MAX])(int*, struct SMART_DIM*, char*) = {
	v255_hexa,
	v203_hexa,
	v151_hexa,
	v87_hexa,
	v51_hexa,
	v35_hexa,
	v23_hexa,
	v11_hexa,
	v3_hexa,
	vt_hexa,
};

#if defined(AID_OPERATION)

#define CCG6_MAX_TABLE 62
static int ccg6_candela_table[][2] = {
	{5, 0},
	{6, 1},
	{7, 2},
	{8, 3},
	{9, 4},
	{10, 5},
	{11, 6},
	{12, 7},
	{13, 8},
	{14, 9},
	{15, 10},
	{16, 11},
	{17, 12},
	{19, 13},
	{20, 14},
	{21, 15},
	{22, 16},
	{24, 17},
	{25, 18},
	{27, 19},
	{29, 20},
	{30, 21},
	{32, 22},
	{34, 23},
	{37, 24},
	{39, 25},
	{41, 26},
	{44, 27},
	{47, 28},
	{50, 29},
	{53, 30},
	{56, 31},
	{60, 32},
	{64, 33},
	{68, 34},
	{72, 35},
	{77, 36},
	{82, 37},
	{87, 38},
	{93, 39},
	{98, 40},
	{105, 41},
	{111, 42},
	{119, 43},
	{126, 44},
	{134, 45},
	{143, 46},
	{152, 47},
	{162, 48},
	{172, 49},
	{183, 50},
	{195, 51},
	{207, 52},
	{220, 53},
	{234, 54},
	{249, 55},
	{265, 56},
	{282, 57},
	{300, 58},
	{316, 59},
	{333, 60},
	{360, 61},
};

static int find_cadela_table(int brightness)
{
	int loop;
	int err = -1;

	for(loop = 0; loop < CCG6_MAX_TABLE; loop++)
		if (ccg6_candela_table[loop][0] == brightness)
			return ccg6_candela_table[loop][1];

	return err;
}

#define RGB_COMPENSATION 24

static int gradation_offset_EA8061V_revF[][9] = {
	{0,	6,	9,	17, 24, 26, 30, 32, 30},
	{0,	6,	8,	16, 20, 23, 26, 28, 31},
	{0,	5,	7,	14, 19, 21, 24, 26, 24},
	{0,	5,	6,	13, 17, 20, 23, 25, 19},
	{0,	5,	5,	12, 16, 18, 20, 23, 18},
	{0,	4,	5,	10, 14, 16, 19, 21, 17},
	{0,	4,	4,	10, 13, 15, 18, 20, 13},
	{0,	3,	4,	9,	13, 14, 17, 19, 13},
	{0,	3,	4,	9,	12, 13, 16, 18, 12},
	{0,	3,	3,	8,	11, 12, 15, 16, 12},
	{0,	3,	3,	8,	10, 11, 14, 16, 9},
	{0,	3,	3,	7,	10, 11, 14, 15, 12},
	{0,	3,	3,	7,	10, 11, 13, 15, 8},
	{0,	3,	2,	6,	8,	9,	11, 13, 9},
	{0,	3,	2,	6,	8,	9,	11, 13, 10},
	{0,	3,	2,	6,	8,	9,	10, 13, 6},
	{0,	3,	2,	6,	7,	8,	10, 12, 8},
	{0,	3,	2,	5,	7,	8,	9,	11, 10},
	{0,	3,	1,	5,	7,	7,	9,	11, 8},
	{0,	3,	1,	5,	6,	7,	8,	11, 5},
	{0,	2,	2,	5,	6,	7,	8,	10, 8},
	{0,	2,	2,	5,	6,	6,	8,	10, 6},
	{0,	2,	2,	4,	6,	6,	8,	9,	8},
	{0,	2,	2,	4,	5,	5,	7,	9,	4},
	{0,	2,	1,	4,	5,	5,	7,	8,	6},
	{0,	2,	1,	4,	4,	4,	6,	8,	4},
	{0,	2,	1,	4,	4,	4,	6,	8,	3},
	{0,	2,	1,	4,	4,	4,	6,	7,	4},
	{0,	2,	1,	3,	4,	4,	5,	6,	5},
	{0,	2,	1,	3,	4,	3,	5,	6,	5},
	{0,	2,	1,	3,	3,	3,	4,	5,	5},
	{0,	2,	1,	3,	3,	3,	4,	5,	4},
	{0,	2,	1,	3,	3,	3,	4,	5,	2},
	{0,	2,	1,	3,	3,	2,	4,	5,	1},
	{0,	2,	0,	2,	2,	2,	3,	4,	3},
	{0,	2,	0,	3,	2,	2,	2,	4,	2},
	{0,	1,	1,	2,	2,	2,	3,	4,	2},
	{0,	1,	2,	3,	2,	1,	2,	3,	3},
	{0,	2,	2,	3,	2,	2,	3,	3,	3},
	{0,	1,	1,	2,	1,	1,	2,	3,	3},
	{0,	1,	2,	3,	2,	2,	3,	4,	1},
	{0,	1,	2,	2,	2,	2,	3,	4,	1},
	{0,	1,	1,	3,	2,	2,	2,	3,	1},
	{0,	2,	2,	3,	2,	1,	3,	3,	2},
	{0,	1,	2,	2,	2,	2,	2,	3,	2},
	{0,	2,	3,	3,	2,	1,	2,	3,	2},
	{0,	1,	2,	2,	2,	1,	1,	2,	2},
	{0,	1,	2,	2,	1,	1,	2,	2,	2},
	{0,	2,	3,	2,	2,	2,	1,	2,	3},
	{0,	2,	3,	2,	2,	1,	1,	2,	3},
	{0,	2,	3,	2,	2,	1,	1,	2,	2},
	{0,	1,	3,	1,	1,	1,	1,	1,	3},
	{0,	1,	2,	1,	1,	1,	0,	1,	1},
	{0,	1,	2,	1,	1,	1,	0,	1,	0},
	{0,	1,	2,	1,	1,	1,	0,	1,	0},
	{0,	1,	2,	1,	1,	1,	0,	1,	1},
	{0,	1,	1,	0,	1,	0,	1,	1,	2},
	{0,	0,	1,	1,	0,	0,	0,	0,	2},
	{0,	0,	1,	0,	1,	0,	1,	0,	3},
	{0,	-1, 1,	0,	1,	0,	0,	1,	0},
	{0,	-1, 0,	0,	0,	0,	-1, 0,	3},
	{0,	0,	0,	0,	0,	0,	0,	0,	0},
};

static int rgb_offset_EA8061V_revF[][RGB_COMPENSATION] = {
/*	R255 G255 B255 R203 G203 B203 R151 G151 B151
	R87 G87 B87 R51 G51 B51 R35 G35 B35
	R23 G23 B23 R11 G11 B11
*/
	{-3,	0,	-2, -2, 0,	-3, -4, 1,	-4, -11,	3,	-8, -11,	4,	-9, -9, 4,	-9, -6, 3,	-8, -5, 3,	-6},
	{-3,	0,	-2, -1, 0,	-2, -4, 0,	-4, -8, 3,	-6, -11,	4,	-8, -8, 3,	-8, -5, 4,	-9, -9, 3,	-8},
	{-2,	0,	-1, -1, 0,	-2, -3, 0,	-4, -9, 2,	-6, -12,	3,	-8, -9, 4,	-9, -4, 4,	-8, -4, 4,	-9},
	{-2,	0,	-1, -1, 0,	-2, -2, 0,	-3, -9, 2,	-6, -11,	3,	-8, -8, 3,	-8, -5, 3,	-7, -3, 5,	-10},
	{-2,	0,	-1, 0,	0,	-1, -2, 0,	-3, -7, 2,	-5, -12,	3,	-8, -7, 3,	-7, -5, 4,	-10,	-3, 4,	-10},
	{-2,	0,	-1, 0,	0,	-1, -1, 0,	-2, -8, 2,	-5, -9, 3,	-6, -8, 3,	-8, -5, 4,	-9, -1, 5,	-12},
	{0, 0,	0,	-2, 0,	-2, -1, 0,	-2, -7, 1,	-4, -10,	3,	-7, -8, 3,	-8, -4, 4,	-8, -1, 6,	-12},
	{0, 0,	0,	-2, 0,	-2, -1, 0,	-2, -6, 2,	-4, -10,	2,	-6, -8, 3,	-8, -3, 5,	-10,	-1, 6,	-12},
	{0, 0,	0,	-2, 0,	-2, 0,	0,	-1, -6, 2,	-4, -10,	2,	-6, -8, 3,	-7, -3, 4,	-10,	-1, 5,	-12},
	{0, 0,	0,	-2, 0,	-2, 0,	0,	-1, -6, 1,	-3, -9, 2,	-6, -7, 3,	-7, -1, 4,	-9, -1, 7,	-14},
	{0, 0,	0,	-1, 0,	-1, -1, 0,	-2, -6, 1,	-3, -9, 2,	-6, -6, 3,	-7, -1, 4,	-9, -1, 5,	-12},
	{0, 0,	0,	-1, 0,	-1, 0,	0,	-1, -6, 1,	-3, -10,	2,	-6, -5, 3,	-6, -2, 4,	-9, -1, 6,	-14},
	{0, 0,	0,	-1, 0,	-1, 0,	0,	-1, -6, 1,	-3, -8, 2,	-4, -6, 2,	-6, -3, 5,	-11,	-1, 5,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	-2, -4, 1,	-2, -7, 2,	-4, -4, 3,	-6, -2, 5,	-11,	-2, 6,	-13},
	{0, 0,	0,	0,	0,	0,	0,	0,	-2, -4, 0,	-2, -8, 2,	-5, -5, 2,	-6, -2, 5,	-12,	-2, 6,	-13},
	{0, 0,	0,	0,	0,	0,	0,	0,	-2, -4, 0,	-2, -7, 2,	-4, -4, 2,	-6, -2, 5,	-12,	-1, 5,	-10},
	{0, 0,	0,	0,	0,	0,	0,	0,	-1, -3, 1,	-2, -8, 1,	-4, -4, 3,	-7, -1, 5,	-10,	-3, 6,	-13},
	{0, 0,	0,	0,	0,	0,	0,	0,	-1, -4, 1,	-2, -6, 1,	-3, -3, 2,	-6, -2, 5,	-12,	-3, 6,	-14},
	{0, 0,	0,	0,	0,	0,	0,	0,	-2, -4, 0,	-1, -6, 1,	-4, -4, 2,	-6, -1, 5,	-11,	-2, 6,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	-1, -4, 0,	-2, -6, 1,	-2, -2, 2,	-6, -2, 5,	-12,	-2, 4,	-10},
	{0, 0,	0,	0,	0,	0,	0,	0,	-2, -4, 0,	-2, -7, 0,	-2, -2, 2,	-6, -1, 5,	-11,	-4, 6,	-13},
	{0, 0,	0,	0,	0,	0,	0,	0,	-2, -4, 0,	-1, -6, 1,	-2, -3, 2,	-6, -2, 4,	-10,	-3, 6,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	-2, -3, 0,	-1, -6, 1,	-2, -3, 2,	-6, 0,	4,	-8, -3, 6,	-14},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-2, 1,	-2, -5, 1,	-2, -1, 2,	-6, -1, 4,	-8, -2, 5,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-2, 0,	-2, -5, 1,	-2, -1, 2,	-6, -1, 3,	-8, -2, 6,	-14},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-2, 0,	-2, -5, 1,	-2, 0,	2,	-6, -2, 3,	-8, -3, 5,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-2, 0,	-2, -5, 0,	-2, 0,	2,	-6, -2, 3,	-8, -3, 5,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	-2, -6, 0,	-2, 0,	2,	-4, -1, 3,	-8, -3, 6,	-14},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-2, 0,	-2, -4, 0,	-1, 0,	2,	-4, 0,	4,	-8, -2, 6,	-13},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	-2, -5, 0,	-1, 0,	2,	-6, -1, 2,	-6, -3, 6,	-14},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	-2, -4, 0,	0,	0,	2,	-4, -1, 3,	-8, -1, 6,	-13},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	-1, -4, 0,	-1, 0,	1,	-4, -1, 3,	-8, -1, 6,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	-1, -4, 0,	-1, 0,	1,	-4, 0,	3,	-6, -1, 5,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1, -4, 0,	0,	0,	1,	-4, 0,	2,	-6, -1, 5,	-11},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	0,	-2, 0,	0,	0,	1,	-4, -1, 2,	-6, -2, 5,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-2, 0,	-1, 1,	1,	-3, 0,	2,	-5, -1, 5,	-11},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	0,	-2, 0,	0,	1,	1,	-2, 0,	2,	-6, -1, 6,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	0,	-1, 0,	0,	1,	1,	-2, 0,	2,	-6, -2, 5,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-2, 0,	2,	-5, 0,	6,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	1,	0,	2,	-2, 0,	0,	1,	0,	-1, 0,	2,	-5, 0,	6,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	-1, -2, 0,	0,	0,	0,	-2, 0,	1,	-4, -2, 5,	-12},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-3, 0,	0,	0,	0,	-1, 0,	2,	-4, 0,	5,	-11},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-2, 0,	0,	0,	0,	0,	0,	2,	-6, 0,	4,	-10},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	1,	0,	0,	0,	0,	2,	-4, 0,	4,	-10},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-2, 0,	-1, 0,	1,	-4, 0,	5,	-10},
	{0, 0,	1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	0,	0,	0,	0,	0,	1,	-4, 0,	4,	-10},
	{0, 0,	1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	1,	-4, -1, 4,	-9},
	{0, 0,	1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	0,	-1, 0,	0,	0,	1,	-4, 0,	4,	-9},
	{0, 0,	1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1, -2, 0,	0,	1,	1,	-3, -1, 4,	-10},
	{0, 0,	1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1, -1, 0,	0,	1,	1,	-4, -1, 4,	-9},
	{0, 0,	1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1, -1, 0,	0,	1,	1,	-4, 0,	3,	-8},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	1,	-1, 0,	0,	2,	1,	-2, 0,	3,	-8},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	1,	0,	0,	1,	1,	1,	-2, 0,	3,	-7},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	1,	0,	0,	1,	1,	1,	-2, 1,	2,	-6},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	-1, 0,	1,	0,	0,	1,	1,	0,	-2, 1,	2,	-6},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0},
	{0, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0},
};

static int base_luminance_revF[][2] = {
	{5, 115},
	{6, 115},
	{7, 115},
	{8, 115},
	{9, 115},
	{10,	115},
	{11,	115},
	{12,	115},
	{13,	115},
	{14,	115},
	{15,	115},
	{16,	115},
	{17,	115},
	{19,	115},
	{20,	115},
	{21,	115},
	{22,	115},
	{24,	115},
	{25,	115},
	{27,	115},
	{29,	115},
	{30,	115},
	{32,	115},
	{34,	115},
	{37,	115},
	{39,	115},
	{41,	115},
	{44,	115},
	{47,	115},
	{50,	115},
	{53,	115},
	{56,	115},
	{60,	115},
	{64,	115},
	{68,	115},
	{72,	121},
	{77,	129},
	{82,	137},
	{87,	145},
	{93,	154},
	{98,	162},
	{105,	172},
	{111,	180},
	{119,	192},
	{126,	202},
	{134,	213},
	{143,	228},
	{152,	241},
	{162,	254},
	{172,	254},
	{183,	254},
	{195,	254},
	{207,	254},
	{220,	254},
	{234,	254},
	{249,	254},
	{265,	268},
	{282,	286},
	{300,	303},
	{316,	318},
	{333,	336},
	{360,	360},
};

static int get_base_luminance(struct SMART_DIM *pSmart)
{
	int cnt;
	int base_luminance[CCG6_MAX_TABLE][2];

	if (id3 <= EA8061V_REV_C)
		memcpy(base_luminance, base_luminance_revF, sizeof(base_luminance_revF));
	else
		memcpy(base_luminance, base_luminance_revF, sizeof(base_luminance_revF));

	for (cnt = 0; cnt < CCG6_MAX_TABLE; cnt++)
		if (base_luminance[cnt][0] == pSmart->brightness_level)
			return base_luminance[cnt][1];

	return -1;
}

static int get_gamma_curve(struct SMART_DIM *pSmart)
{
	return GAMMA_CURVE_2P2;
}

static int get_gradation_offset(int table_index, int index)
{
	if(id3 <= EA8061V_REV_C)
		return gradation_offset_EA8061V_revF[table_index][index];
	else
		return gradation_offset_EA8061V_revF[table_index][index];
}

static int get_rgb_offset(int table_index, int index)
{
	if(id3 <= EA8061V_REV_C)
		return rgb_offset_EA8061V_revF[table_index][index];
	else
		return rgb_offset_EA8061V_revF[table_index][index];
}

void gamma_init_EA8061V(struct SMART_DIM *pSmart, char *str, int size)
{
	long long candela_level[TP_MAX] = {-1, };
	int bl_index[TP_MAX] = {-1, };

	long long temp_cal_data = 0;
	int bl_level;

	int level_255_temp_MSB = 0;
	int level_V255 = 0;

	int point_index;
	int cnt;
	int table_index;

	pr_info_once("%s : start !!\n",__func__);

	/* get base luminance */
	bl_level = get_base_luminance(pSmart);
	if (bl_level < 0)
		pr_err("%s : can not find base luminance!!\n", __func__);

	if (pSmart->brightness_level < 350) {
		for (cnt = 0; cnt < TP_MAX; cnt++) {
			point_index = TP_ARRAY[cnt];
			temp_cal_data =
			((long long)(candela_coeff_2p15[point_index])) *
			((long long)(bl_level));
			candela_level[cnt] = temp_cal_data;
		}
	} else {
		for (cnt = 0; cnt < TP_MAX; cnt++) {
			point_index = TP_ARRAY[cnt];
			temp_cal_data =
			((long long)(candela_coeff_2p2[point_index])) *
			((long long)(bl_level));
			candela_level[cnt] = temp_cal_data;
		}
	}

#ifdef SMART_DIMMING_DEBUG
	printk(KERN_INFO "\n candela_1:%llu  candela_3:%llu  candela_11:%llu ",
		candela_level[0], candela_level[1], candela_level[2]);
	printk(KERN_INFO "candela_23:%llu  candela_35:%llu  candela_51:%llu ",
		candela_level[3], candela_level[4], candela_level[5]);
	printk(KERN_INFO "candela_87:%llu  candela_151:%llu  candela_203:%llu ",
		candela_level[6], candela_level[7], candela_level[8]);
	printk(KERN_INFO "candela_255:%llu brightness_level %d\n", candela_level[9], pSmart->brightness_level);
#endif

	for (cnt = 0; cnt < TP_MAX; cnt++) {
		if (searching_function(candela_level[cnt],
			&(bl_index[cnt]), get_gamma_curve(pSmart))) {
			pr_info("%s searching functioin error cnt:%d\n",
			__func__, cnt);
		}
	}

	/*
	*	Candela compensation
	*/
	for (cnt = 1; cnt < TP_MAX; cnt++) {
		table_index = find_cadela_table(pSmart->brightness_level);

		if (table_index == -1) {
			table_index = CCG6_MAX_TABLE-1;
			pr_info("%s fail candela table_index cnt : %d brightness %d",
				__func__, cnt, pSmart->brightness_level);
		}

		bl_index[TP_MAX - cnt] +=
			get_gradation_offset(table_index, cnt-1);

		/* THERE IS M-GRAY0 target */
		if (bl_index[TP_MAX - cnt] == 0)
			bl_index[TP_MAX - cnt] = 1;
	}

#ifdef SMART_DIMMING_DEBUG
	printk(KERN_INFO "\n bl_index_1:%d  bl_index_3:%d  bl_index_11:%d",
		bl_index[0], bl_index[1], bl_index[2]);
	printk(KERN_INFO "bl_index_23:%d bl_index_35:%d  bl_index_51:%d",
		bl_index[3], bl_index[4], bl_index[5]);
	printk(KERN_INFO "bl_index_87:%d  bl_index_151:%d bl_index_203:%d",
		bl_index[6], bl_index[7], bl_index[8]);
	printk(KERN_INFO "bl_index_255:%d\n", bl_index[9]);
#endif
	/*Generate Gamma table*/
	for (cnt = 0; cnt < TP_MAX; cnt++)
		(void)Make_hexa[cnt](bl_index , pSmart, str);

	/*
	*	RGB compensation
	*/
	for (cnt = 0; cnt < RGB_COMPENSATION; cnt++) {
		table_index = find_cadela_table(pSmart->brightness_level);

		if (table_index == -1) {
			table_index = CCG6_MAX_TABLE-1;
			pr_info("%s fail RGB table_index cnt : %d brightness %d",
				__func__, cnt, pSmart->brightness_level);
		}

		if (cnt < 3) {
			level_V255 = str[cnt * 2] << 8 | str[(cnt * 2) + 1];
			level_V255 +=
				get_rgb_offset(table_index, cnt);
			level_255_temp_MSB = level_V255 / 256;

			str[cnt * 2] = level_255_temp_MSB & 0xff;
			str[(cnt * 2) + 1] = level_V255 & 0xff;
		} else {
			str[cnt+3] += get_rgb_offset(table_index, cnt);
		}
	}
	/*subtration MTP_OFFSET value from generated gamma table*/
	mtp_offset_substraction(pSmart, str);
}

#endif

static void pure_gamma_init(struct SMART_DIM *pSmart, char *str, int size)
{
	long long candela_level[TP_MAX] = {-1, };
	int bl_index[TP_MAX] = {-1, };

	long long temp_cal_data = 0;
	int bl_level, cnt;
	int point_index;

	bl_level = pSmart->brightness_level;

	for (cnt = 0; cnt < TP_MAX; cnt++) {
			point_index = TP_ARRAY[cnt];
			temp_cal_data =
			((long long)(candela_coeff_2p2[point_index])) *
			((long long)(bl_level));
			candela_level[cnt] = temp_cal_data;
	}

#ifdef SMART_DIMMING_DEBUG
	printk(KERN_INFO "\n candela_1:%llu  candela_3:%llu  candela_11:%llu ",
		candela_level[0], candela_level[1], candela_level[2]);
	printk(KERN_INFO "candela_23:%llu  candela_35:%llu  candela_51:%llu ",
		candela_level[3], candela_level[4], candela_level[5]);
	printk(KERN_INFO "candela_87:%llu  candela_151:%llu  candela_203:%llu ",
		candela_level[6], candela_level[7], candela_level[8]);
	printk(KERN_INFO "candela_255:%llu\n", candela_level[9]);
#endif

	/*calculate brightness level*/
	for (cnt = 0; cnt < TP_MAX; cnt++) {
			if (searching_function(candela_level[cnt],
				&(bl_index[cnt]), GAMMA_CURVE_2P2)) {
				pr_info("%s searching functioin error cnt:%d\n",
					__func__, cnt);
			}
	}

	/*
	*	210CD ~ 190CD compensation
	*	V3 level + 1
	*/
	if ((bl_level >= 190) && (bl_level <= 210))
		bl_index[1] += 1;

#ifdef SMART_DIMMING_DEBUG
	printk(KERN_INFO "\n bl_index_1:%d  bl_index_3:%d  bl_index_11:%d",
	bl_index[0], bl_index[1], bl_index[2]);
	printk(KERN_INFO "bl_index_23:%d bl_index_35:%d  bl_index_51:%d",
		bl_index[3], bl_index[4], bl_index[5]);
	printk(KERN_INFO "bl_index_87:%d  bl_index_151:%d bl_index_203:%d",
		bl_index[5], bl_index[7], bl_index[8]);
	printk(KERN_INFO "bl_index_255:%d\n", bl_index[9]);
#endif

	/*Generate Gamma table*/
	for (cnt = 0; cnt < TP_MAX; cnt++)
		(void)Make_hexa[cnt](bl_index , pSmart, str);

	mtp_offset_substraction(pSmart, str);

	temp_candela_coeff = candela_coeff_2p25;
	temp_candela_coeff = candela_coeff_2p1;
	temp_candela_coeff = candela_coeff_2p05;
	temp_candela_coeff = candela_coeff_2p0;
	temp_candela_coeff = candela_coeff_1p95;
	temp_candela_coeff = candela_coeff_1p9;
	temp_candela_coeff = candela_coeff_1p85;
	temp_candela_coeff = candela_coeff_1p8;
	temp_candela_coeff = candela_coeff_1p75;
	temp_candela_coeff = candela_coeff_1p7;
	temp_candela_coeff = candela_coeff_1p65;
	temp_candela_coeff = candela_coeff_1p6;
	temp_candela_coeff = s6e63m0_candela_coeff;
	temp_candela_coeff = s6e63m0_curve_2p2;
	temp_candela_coeff = curve_2p2;
	temp_candela_coeff = curve_1p9;
	temp_candela_coeff = candela_coeff_2p2;
	temp_candela_coeff = candela_coeff_2p15;
	temp_candela_coeff = curve_1p9_350;
	temp_candela_coeff = curve_2p15_350;
	temp_candela_coeff = curve_2p2_350;
	temp_candela_coeff = curve_1p9_360;
	temp_candela_coeff = curve_2p15_360;
	temp_candela_coeff = curve_2p2_360;
}

static void set_max_lux_table(void)
{
	max_lux_table[0] = V255_300CD_R_MSB;
	max_lux_table[1] = V255_300CD_R_LSB;

	max_lux_table[2] = V255_300CD_G_MSB;
	max_lux_table[3] = V255_300CD_G_LSB;

	max_lux_table[4] = V255_300CD_B_MSB;
	max_lux_table[5] = V255_300CD_B_LSB;

	max_lux_table[6] = V203_300CD_R;
	max_lux_table[7] = V203_300CD_G;
	max_lux_table[8] = V203_300CD_B;

	max_lux_table[9] = V151_300CD_R;
	max_lux_table[10] = V151_300CD_G;
	max_lux_table[11] = V151_300CD_B;

	max_lux_table[12] = V87_300CD_R;
	max_lux_table[13] = V87_300CD_G;
	max_lux_table[14] = V87_300CD_B;

	max_lux_table[15] = V51_300CD_R;
	max_lux_table[16] = V51_300CD_G;
	max_lux_table[17] = V51_300CD_B;

	max_lux_table[18] = V35_300CD_R;
	max_lux_table[19] = V35_300CD_G;
	max_lux_table[20] = V35_300CD_B;

	max_lux_table[21] = V23_300CD_R;
	max_lux_table[22] = V23_300CD_G;
	max_lux_table[23] = V23_300CD_B;

	max_lux_table[24] = V11_300CD_R;
	max_lux_table[25] = V11_300CD_G;
	max_lux_table[26] = V11_300CD_B;

	max_lux_table[27] = V3_300CD_R;
	max_lux_table[28] = V3_300CD_G;
	max_lux_table[29] = V3_300CD_B;

	max_lux_table[30] = VT_300CD_R;
	max_lux_table[31] = VT_300CD_G;
	max_lux_table[32] = VT_300CD_B;

}


static void set_min_lux_table(struct SMART_DIM *psmart)
{
	psmart->brightness_level = MIN_CANDELA;
	pure_gamma_init(psmart, min_lux_table, GAMMA_SET_MAX);
}

static void get_min_lux_table(char *str, int size)
{
	memcpy(str, min_lux_table, size);
}

static void generate_gamma(struct SMART_DIM *psmart, char *str, int size)
{
	int lux_loop;
	struct illuminance_table *ptable = (struct illuminance_table *)
						(&(psmart->gen_table));

	/* searching already generated gamma table */
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		if (ptable[lux_loop].lux == psmart->brightness_level) {
			memcpy(str, &(ptable[lux_loop].gamma_setting), size);
			break;
		}
	}

	/* searching fail... Setting 300CD value on gamma table */
	if (lux_loop == psmart->lux_table_max) {
		pr_info("%s searching fail lux : %d\n", __func__,
				psmart->brightness_level);
		memcpy(str, max_lux_table, size);
	}

#ifdef SMART_DIMMING_DEBUG
	if (lux_loop != psmart->lux_table_max)
		pr_info("%s searching ok index : %d lux : %d", __func__,
			lux_loop, ptable[lux_loop].lux);
#endif
}

static void mtp_sorting(struct SMART_DIM *psmart)
{
	int sorting[GAMMA_SET_MAX] = {
		0, 1, 6, 9, 12, 15, 18, 21, 24, 27, 30, /* R*/
		2, 3, 7, 10, 13, 16, 19, 22, 25, 28, 31, /* G */
		4, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, /* B */
	};
	int loop;
	char *pfrom, *pdest;

	pfrom = (char *)&(psmart->MTP_ORIGN);
	pdest = (char *)&(psmart->MTP);

	for (loop = 0; loop < GAMMA_SET_MAX; loop++)
		pdest[loop] = pfrom[sorting[loop]];
}

static int smart_dimming_init(struct SMART_DIM *psmart)
{
	int lux_loop;

#ifdef SMART_DIMMING_DEBUG
	int cnt;
	char pBuffer[256];
	memset(pBuffer, 0x00, 256);
#endif

	id3 = psmart->ldi_revision & 0xFF;

	pr_info("%s : ++\n",__func__);

	mtp_sorting(psmart);
	set_max_lux_table();

#ifdef SMART_DIMMING_DEBUG
	print_RGB_offset(psmart);
#endif

	psmart->vregout_voltage = VREG0_REF_6P0;

	v255_adjustment(psmart);
	vt_adjustment(psmart);
	v203_adjustment(psmart);
	v151_adjustment(psmart);
	v87_adjustment(psmart);
	v51_adjustment(psmart);
	v35_adjustment(psmart);
	v23_adjustment(psmart);
	v11_adjustment(psmart);
	v3_adjustment(psmart);


	if (generate_gray_scale(psmart)) {
		pr_info(KERN_ERR "lcd smart dimming fail generate_gray_scale\n");
		return -EINVAL;
	}

	/*Generating lux_table*/
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		/* To set brightness value */
		psmart->brightness_level = psmart->plux_table[lux_loop];
		/* To make lux table index*/
		psmart->gen_table[lux_loop].lux = psmart->plux_table[lux_loop];

#if defined(AID_OPERATION)
		gamma_init_EA8061V(psmart,
			(char *)(&(psmart->gen_table[lux_loop].gamma_setting)),
			GAMMA_SET_MAX);
#else
		pure_gamma_init(psmart,
			(char *)(&(psmart->gen_table[lux_loop].gamma_setting)),
			GAMMA_SET_MAX);
#endif
	}

/* set 300CD max gamma table */
	memcpy(&(psmart->gen_table[lux_loop-1].gamma_setting),
			max_lux_table, GAMMA_SET_MAX);

	set_min_lux_table(psmart);

#ifdef SMART_DIMMING_DEBUG
	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
			snprintf(pBuffer + strnlen(pBuffer, 256), 256, " %d",
				psmart->gen_table[lux_loop].gamma_setting[cnt]);

		pr_info("lux : %3d  %s", psmart->plux_table[lux_loop], pBuffer);
		memset(pBuffer, 0x00, 256);
	}

	for (lux_loop = 0; lux_loop < psmart->lux_table_max; lux_loop++) {
		for (cnt = 0; cnt < GAMMA_SET_MAX; cnt++)
			snprintf(pBuffer + strnlen(pBuffer, 256), 256,
				" %02X",
				psmart->gen_table[lux_loop].gamma_setting[cnt]);

		pr_info("lux : %3d  %s", psmart->plux_table[lux_loop], pBuffer);
		memset(pBuffer, 0x00, 256);
	}
#endif

	pr_info("%s done\n",__func__);

	return 0;
}

/* ----------------------------------------------------------------------------
 * Wrapper functions for smart dimming to work with 8974 generic code
 * ----------------------------------------------------------------------------
 */
static void wrap_generate_gamma_main(int cd, char *cmd_str) {
	smart_EA8061V_main.brightness_level = cd;
	generate_gamma(&smart_EA8061V_main, cmd_str, GAMMA_SET_MAX);
}

static void wrap_smart_dimming_init_main(void) {
	smart_EA8061V_main.plux_table = __EA8061V_MAIN__.lux_tab;
	smart_EA8061V_main.lux_table_max = __EA8061V_MAIN__.lux_tabsize;
	smart_EA8061V_main.ldi_revision = __EA8061V_MAIN__.man_id;
	smart_dimming_init(&smart_EA8061V_main);
}

struct smartdim_conf *smart_S6E8FA0_get_conf_main(void) {
	__EA8061V_MAIN__.generate_gamma = wrap_generate_gamma_main;
	__EA8061V_MAIN__.init = wrap_smart_dimming_init_main;
	__EA8061V_MAIN__.get_min_lux_table = get_min_lux_table;
	__EA8061V_MAIN__.mtp_buffer = (char *)(&smart_EA8061V_main.MTP_ORIGN);
	__EA8061V_MAIN__.print_aid_log = print_aid_log_main;
	return (struct smartdim_conf *)&__EA8061V_MAIN__;
}


static void wrap_generate_gamma_sub(int cd, char *cmd_str) {
	smart_EA8061V_sub.brightness_level = cd;
	generate_gamma(&smart_EA8061V_sub, cmd_str, GAMMA_SET_MAX);
}

static void wrap_smart_dimming_init_sub(void) {
	smart_EA8061V_sub.plux_table = __EA8061V_SUB__.lux_tab;
	smart_EA8061V_sub.lux_table_max = __EA8061V_SUB__.lux_tabsize;
	smart_EA8061V_sub.ldi_revision = __EA8061V_SUB__.man_id;
	smart_dimming_init(&smart_EA8061V_sub);
}

struct smartdim_conf *smart_S6E8FA0_get_conf_sub(void) {
	__EA8061V_SUB__.generate_gamma = wrap_generate_gamma_sub;
	__EA8061V_SUB__.init = wrap_smart_dimming_init_sub;
	__EA8061V_SUB__.get_min_lux_table = get_min_lux_table;
	__EA8061V_SUB__.mtp_buffer = (char *)(&smart_EA8061V_sub.MTP_ORIGN);
	__EA8061V_SUB__.print_aid_log = print_aid_log_sub;
	return (struct smartdim_conf *)&__EA8061V_SUB__;
}

