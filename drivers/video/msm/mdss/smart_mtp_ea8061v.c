/*
 * =================================================================
 *
 *       Filename:  smart_mtp_ea8061v.c
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

#include "smart_mtp_ea8061v.h"
#include "smart_mtp_2p2_gamma.h"
#include "smart_dimming.h"

static struct SMART_DIM smart_EA8061V;
static struct smartdim_conf __EA8061V__ ;

//#define SMART_DIMMING_DEBUG

static char max_lux_table[GAMMA_SET_MAX];
static char min_lux_table[GAMMA_SET_MAX];

static int *temp_candela_coeff;

/*
*	To support different center cell gamma setting
*/
static char V255_300CD_R_MSB;
static char V255_300CD_R_LSB;

static char V255_300CD_G_MSB;
static char V255_300CD_G_LSB;

static char V255_300CD_B_MSB;
static char V255_300CD_B_LSB;

static char V203_300CD_R;
static char V203_300CD_G;
static char V203_300CD_B;

static char V151_300CD_R;
static char V151_300CD_G;
static char V151_300CD_B;

static char V87_300CD_R;
static char V87_300CD_G;
static char V87_300CD_B;

static char V51_300CD_R;
static char V51_300CD_G;
static char V51_300CD_B;

static char V35_300CD_R;
static char V35_300CD_G;
static char V35_300CD_B;

static char V23_300CD_R;
static char V23_300CD_G;
static char V23_300CD_B;

static char V11_300CD_R;
static char V11_300CD_G;
static char V11_300CD_B;

static char V3_300CD_R;
static char V3_300CD_G;
static char V3_300CD_B;

static char VT_300CD_R;
static char VT_300CD_G;
static char VT_300CD_B;

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

void print_aid_log(void)
{
	print_RGB_offset(&smart_EA8061V);
	print_lux_table(&smart_EA8061V);
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


/*V0,V1,V3,V11,V23,V35,V51,V87,V151,V203,V255*/
static int EA8061V_ARRAY[EA8061V_MAX] = {0, 1, 3, 11, 23, 35, 51, 87, 151, 203, 255};

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

	for (cnt = 0; cnt < EA8061V_MAX; cnt++) {
		pSmart->GRAY.TABLE[EA8061V_ARRAY[cnt]].R_Gray =
			((int *)&(pSmart->RGB_OUTPUT.R_VOLTAGE))[cnt];

		pSmart->GRAY.TABLE[EA8061V_ARRAY[cnt]].G_Gray =
			((int *)&(pSmart->RGB_OUTPUT.G_VOLTAGE))[cnt];

		pSmart->GRAY.TABLE[EA8061V_ARRAY[cnt]].B_Gray =
			((int *)&(pSmart->RGB_OUTPUT.B_VOLTAGE))[cnt];
	}

	/*
		below codes use hard coded value.
		So it is possible to modify on each model.
		V0,V1,V3,V11,V23,V35,V51,V87,V151,V203,V255
	*/
	for (cnt = 0; cnt < EA8061V_GRAY_SCALE_MAX; cnt++) {

		if (cnt == EA8061V_ARRAY[0]) {
			/* 0 */
			array_index = 0;
			cal_cnt = 0;
		} else if ((cnt > EA8061V_ARRAY[0]) &&
			(cnt < EA8061V_ARRAY[2])) {
			/* 1 ~ 2 */
			array_index = 2;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-2]].R_Gray,
			ptable[EA8061V_ARRAY[array_index]].R_Gray,
			V0toV3_Coefficient, V0toV3_Multiple,
			V0toV3_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-2]].G_Gray,
			ptable[EA8061V_ARRAY[array_index]].G_Gray,
			V0toV3_Coefficient, V0toV3_Multiple,
			V0toV3_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-2]].B_Gray,
			ptable[EA8061V_ARRAY[array_index]].B_Gray,
			V0toV3_Coefficient, V0toV3_Multiple,
			V0toV3_denominator , cal_cnt);

			cal_cnt++;
		} else if (cnt == EA8061V_ARRAY[2]) {
			/* 3 */
			cal_cnt = 0;
		} else if ((cnt > EA8061V_ARRAY[2]) &&
			(cnt < EA8061V_ARRAY[3])) {
			/* 4 ~ 10 */
			array_index = 3;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].R_Gray,
			ptable[EA8061V_ARRAY[array_index]].R_Gray,
			V3toV11_Coefficient, V3toV11_Multiple,
			V3toV11_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].G_Gray,
			ptable[EA8061V_ARRAY[array_index]].G_Gray,
			V3toV11_Coefficient, V3toV11_Multiple,
			V3toV11_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].B_Gray,
			ptable[EA8061V_ARRAY[array_index]].B_Gray,
			V3toV11_Coefficient, V3toV11_Multiple,
			V3toV11_denominator , cal_cnt);

			cal_cnt++;
		} else if (cnt == EA8061V_ARRAY[3]) {
			/* 11 */
			cal_cnt = 0;
		} else if ((cnt > EA8061V_ARRAY[3]) &&
			(cnt < EA8061V_ARRAY[4])) {
			/* 12 ~ 22 */
			array_index = 4;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].R_Gray,
			ptable[EA8061V_ARRAY[array_index]].R_Gray,
			V11toV23_Coefficient, V11toV23_Multiple,
			V11toV23_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].G_Gray,
			ptable[EA8061V_ARRAY[array_index]].G_Gray,
			V11toV23_Coefficient, V11toV23_Multiple,
			V11toV23_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].B_Gray,
			ptable[EA8061V_ARRAY[array_index]].B_Gray,
			V11toV23_Coefficient, V11toV23_Multiple,
			V11toV23_denominator , cal_cnt);

			cal_cnt++;
		}  else if (cnt == EA8061V_ARRAY[4]) {
			/* 23 */
			cal_cnt = 0;
		} else if ((cnt > EA8061V_ARRAY[4]) &&
			(cnt < EA8061V_ARRAY[5])) {
			/* 24 ~ 34 */
			array_index = 5;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].R_Gray,
			ptable[EA8061V_ARRAY[array_index]].R_Gray,
			V23toV35_Coefficient, V23toV35_Multiple,
			V23toV35_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].G_Gray,
			ptable[EA8061V_ARRAY[array_index]].G_Gray,
			V23toV35_Coefficient, V23toV35_Multiple,
			V23toV35_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].B_Gray,
			ptable[EA8061V_ARRAY[array_index]].B_Gray,
			V23toV35_Coefficient, V23toV35_Multiple,
			V23toV35_denominator , cal_cnt);

			cal_cnt++;
		} else if (cnt == EA8061V_ARRAY[5]) {
			/* 35 */
			cal_cnt = 0;
		} else if ((cnt > EA8061V_ARRAY[5]) &&
			(cnt < EA8061V_ARRAY[6])) {
			/* 36 ~ 50 */
			array_index = 6;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].R_Gray,
			ptable[EA8061V_ARRAY[array_index]].R_Gray,
			V35toV51_Coefficient, V35toV51_Multiple,
			V35toV51_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].G_Gray,
			ptable[EA8061V_ARRAY[array_index]].G_Gray,
			V35toV51_Coefficient, V35toV51_Multiple,
			V35toV51_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].B_Gray,
			ptable[EA8061V_ARRAY[array_index]].B_Gray,
			V35toV51_Coefficient, V35toV51_Multiple,
			V35toV51_denominator, cal_cnt);
			cal_cnt++;

		} else if (cnt == EA8061V_ARRAY[6]) {
			/* 51 */
			cal_cnt = 0;
		} else if ((cnt > EA8061V_ARRAY[6]) &&
			(cnt < EA8061V_ARRAY[7])) {
			/* 52 ~ 86 */
			array_index = 7;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].R_Gray,
			ptable[EA8061V_ARRAY[array_index]].R_Gray,
			V51toV87_Coefficient, V51toV87_Multiple,
			V51toV87_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].G_Gray,
			ptable[EA8061V_ARRAY[array_index]].G_Gray,
			V51toV87_Coefficient, V51toV87_Multiple,
			V51toV87_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].B_Gray,
			ptable[EA8061V_ARRAY[array_index]].B_Gray,
			V51toV87_Coefficient, V51toV87_Multiple,
			V51toV87_denominator, cal_cnt);
			cal_cnt++;

		} else if (cnt == EA8061V_ARRAY[7]) {
			/* 87 */
			cal_cnt = 0;
		} else if ((cnt > EA8061V_ARRAY[7]) &&
			(cnt < EA8061V_ARRAY[8])) {
			/* 88 ~ 150 */
			array_index = 8;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].R_Gray,
			ptable[EA8061V_ARRAY[array_index]].R_Gray,
			V87toV151_Coefficient, V87toV151_Multiple,
			V87toV151_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].G_Gray,
			ptable[EA8061V_ARRAY[array_index]].G_Gray,
			V87toV151_Coefficient, V87toV151_Multiple,
			V87toV151_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].B_Gray,
			ptable[EA8061V_ARRAY[array_index]].B_Gray,
			V87toV151_Coefficient, V87toV151_Multiple,
			V87toV151_denominator, cal_cnt);

			cal_cnt++;
		} else if (cnt == EA8061V_ARRAY[8]) {
			/* 151 */
			cal_cnt = 0;
		} else if ((cnt > EA8061V_ARRAY[8]) &&
			(cnt < EA8061V_ARRAY[9])) {
			/* 152 ~ 202 */
			array_index = 9;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].R_Gray,
			ptable[EA8061V_ARRAY[array_index]].R_Gray,
			V151toV203_Coefficient, V151toV203_Multiple,
			V151toV203_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].G_Gray,
			ptable[EA8061V_ARRAY[array_index]].G_Gray,
			V151toV203_Coefficient, V151toV203_Multiple,
			V151toV203_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].B_Gray,
			ptable[EA8061V_ARRAY[array_index]].B_Gray,
			V151toV203_Coefficient, V151toV203_Multiple,
			V151toV203_denominator, cal_cnt);

			cal_cnt++;
		} else if (cnt == EA8061V_ARRAY[9]) {
			/* 203 */
			cal_cnt = 0;
		} else if ((cnt > EA8061V_ARRAY[9]) &&
			(cnt < EA8061V_ARRAY[10])) {
			/* 204 ~ 254 */
			array_index = 10;

			pSmart->GRAY.TABLE[cnt].R_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].R_Gray,
			ptable[EA8061V_ARRAY[array_index]].R_Gray,
			V203toV255_Coefficient, V203toV255_Multiple,
			V203toV255_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].G_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].G_Gray,
			ptable[EA8061V_ARRAY[array_index]].G_Gray,
			V203toV255_Coefficient, V203toV255_Multiple,
			V203toV255_denominator, cal_cnt);

			pSmart->GRAY.TABLE[cnt].B_Gray = cal_gray_scale_linear(
			ptable[EA8061V_ARRAY[array_index-1]].B_Gray,
			ptable[EA8061V_ARRAY[array_index]].B_Gray,
			V203toV255_Coefficient, V203toV255_Multiple,
			V203toV255_denominator, cal_cnt);

			cal_cnt++;
		 } else {
			if (cnt == EA8061V_ARRAY[10]) {
				pr_debug("%s end\n", __func__);
			} else {
				pr_err("%s fail cnt:%d\n", __func__, cnt);
				return -EINVAL;
			}
		}

	}

#ifdef SMART_DIMMING_DEBUG
		for (cnt = 0; cnt < EA8061V_GRAY_SCALE_MAX; cnt++) {
			pr_info("%s %8d %8d %8d %d\n", __func__,
				pSmart->GRAY.TABLE[cnt].R_Gray,
				pSmart->GRAY.TABLE[cnt].G_Gray,
				pSmart->GRAY.TABLE[cnt].B_Gray, cnt);
		}
#endif
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

	for (cnt = 0; cnt < (EA8061V_GRAY_SCALE_MAX-1); cnt++) {
		if (gamma_curve == GAMMA_CURVE_1P9) {
			delta_1 = candela - curve_1p9_350[cnt];
			delta_2 = candela - curve_1p9_350[cnt+1];
		} else if (gamma_curve == GAMMA_CURVE_2P15) {
			delta_1 = candela - curve_2p15_350[cnt];
			delta_2 = candela - curve_2p15_350[cnt+1];
		} else if (gamma_curve == GAMMA_CURVE_2P2) {
			delta_1 = candela - curve_2p2_350[cnt];
			delta_2 = candela - curve_2p2_350[cnt+1];
		} else
			delta_1 = candela - curve_2p2_350[cnt];
			delta_2 = candela - curve_2p2_350[cnt+1];

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


/* -1 means V1 */
#define EA8061V_TABLE_MAX  (EA8061V_MAX-1)
static void(*Make_hexa[EA8061V_TABLE_MAX])(int*, struct SMART_DIM*, char*) = {
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

#define CCG6_MAX_TABLE 64
static int ccg6_candela_table[][2] = {
	{2, 0,},
	{3, 1,},
	{4, 2,},
	{5, 3,},
	{6, 4,},
	{7, 5,},
	{8, 6,},
	{9, 7,},
	{10, 8,},
	{11, 9,},
	{12, 10,},
	{13, 11,},
	{14, 12,},
	{15, 13,},
	{16, 14,},
	{17, 15,},
	{19, 16,},
	{20, 17,},
	{21, 18,},
	{22, 19,},
	{24, 20,},
	{25, 21,},
	{27, 22,},
	{29, 23,},
	{30, 24,},
	{32, 25,},
	{34, 26,},
	{37, 27,},
	{39, 28,},
	{41, 29,},
	{44, 30,},
	{47, 31,},
	{50, 32,},
	{53, 33,},
	{56, 34,},
	{60, 35,},
	{64, 36,},
	{68, 37,},
	{72, 38,},
	{77, 39,},
	{82, 40,},
	{87, 41,},
	{93, 42,},
	{98, 43,},
	{105, 44,},
	{111, 45,},
	{119, 46,},
	{126, 47,},
	{134, 48,},
	{143, 49,},
	{152, 50,},
	{162, 51,},
	{172, 52,},
	{183, 53,},
	{195, 54,},
	{207, 55,},
	{220, 56,},
	{234, 57,},
	{249, 58,},
	{265, 59,},
	{282, 60,},
	{300, 61,},
	{316, 62,},
	{333, 63,},
	{350, 64,},
};

static int find_cadela_table(int brightness)
{
	int loop;
	int err = -1;

	for(loop = 0; loop <= CCG6_MAX_TABLE; loop++)
		if (ccg6_candela_table[loop][0] == brightness)
			return ccg6_candela_table[loop][1];

	return err;
}

#define RGB_COMPENSATION 24

static int gradation_offset_EA8061V_revA[][9] = {
/*	V255 V203 V151 V87 V51 V35 V23 V11 V3 */
	{0, 8, 15, 26, 36, 40, 46, 49, 49},
	{0, 7, 13, 22, 28, 33, 39, 42, 42},
	{0, 6, 10, 18, 24, 29, 33, 36, 38},
	{0, 5, 9,  17, 22, 26, 30, 33, 33},
	{0, 4, 8,  14, 19, 23, 27, 30, 31},
	{0, 4, 8,  14, 17, 20, 25, 28, 29},
	{0, 4, 7,  13, 17, 19, 24, 26, 27},
	{0, 4, 6,  12, 15, 17, 22, 24, 25},
	{0, 3, 5,  10, 13, 15, 20, 22, 23},
	{0, 3, 5,  9,  12, 14, 19, 21, 21},
	{0, 3, 4,  8,  12, 15, 18, 20, 20},
	{0, 3, 4,  7,  9, 12, 17, 19, 19},
	{0, 2, 4,  7,  10, 13, 16, 18, 18},
	{0, 2, 3,  7,  10, 11, 16, 17, 18},
	{0, 2, 2,  7,  10, 10, 15, 16, 17},
	{0, 2, 2,  6,  9, 10, 15, 15, 15},
	{0, 2, 2,  6,  8, 9, 13, 14, 15},
	{0, 2, 2,  6,  7, 8, 12, 14, 15},
	{0, 2, 2,  6,  6, 7, 12, 13, 13},
	{0, 2, 2,  6,  6, 7, 11, 13, 13},
	{0, 2, 2,  5,  5, 6, 10, 12, 12},
	{0, 2, 2,  5,  5, 6, 10, 11, 11},
	{0, 2, 2,  4,  4, 5, 9, 10, 10},
	{0, 2, 2,  4,  4, 4, 9, 10, 10},
	{0, 2, 2,  4,  4, 4, 8, 9, 9},
	{0, 2, 2,  4,  6, 5, 9, 10, 10},
	{0, 2, 2,  4,  6, 5, 8, 9, 9},
	{0, 1, 1,  3,  5, 5, 8, 9, 9},
	{0, 1, 1,  3,  4, 5, 8, 9, 9},
	{0, 1, 1,  3,  4, 5, 8, 8, 8},
	{0, 1, 1,  3,  4, 4, 7, 8, 8},
	{0, 1, 0,  2,  4, 4, 6, 8, 8},
	{0, 1, 0,  2,  3, 4, 6, 7, 7},
	{0, 1, 0,  2,  3, 3, 5, 6, 6},
	{0, 1, 1,  2,  2, 2, 5, 6, 6},
	{0, 1, 1,  2,  2, 1, 4, 5, 5},
	{0, 1, 2,  2,  2, 1, 4, 4, 4},
	{0, 2, 2,  5,  4, 4, 4, 4, 5},
	{0, 2, 2,  5,  4, 4, 4, 5, 5},
	{0, 2, 2,  4,  3, 3, 4, 5, 5},
	{0, 2, 2,  3,  3, 3, 3, 4, 5},
	{0, 2, 2,  3,  3, 3, 3, 4, 5},
	{0, 1, 1,  2,  3, 3, 4, 5, 5},
	{0, 1, 1,  2,  3, 3, 4, 5, 5},
	{0, 1, 1,  1,  2, 2, 3, 4, 4},
	{0, 1, 1,  1,  2, 2, 3, 4, 4},
	{0, 1, 1,  1,  1, 2, 3, 4, 4},
	{0, 1, 1,  1,  1, 2, 3, 4, 4},
	{0, 1, 1,  1,  1, 2, 2, 3, 3},
	{0, 0, 0,  1,  1, 2, 2, 3, 4},
	{0, 0, 0,  1,  1, 2, 2, 3, 5},
	{0, 0, 0,  1,  1, 1, 2, 3, 3},
	{0, 0, 0,  1,  1, 1, 2, 3, 3},
	{0, 0, 0,  1,  1, 1, 1, 2, 2},
	{0, 0, 0,  0,  0, 0, 0, 2, 2},
	{0, 0, 0,  0,  0, 0, 0, 1, 1},
	{0, 0, 0,  0,  0, 0, 0, 1, 1},
	{0, 0, 0,  0,  0, 0, 0, 0, 0},
	{2, 0, 0,  0,  0, 0, 0, 0, 0},
	{2, 0, 0,  0,  0, 0, 0, 0, 0},
	{1, 0, 0,  0,  0, 0, 0, 0, 0},
	{1, 0, 0,  0,  0, 0, 0, 0, 0},
	{1, 0, 0,  0,  0, 0, 0, 0, 0},
	{0, 0, 0,  0,  0, 0, 0, 0, 0},
	{0, 0, 0,  0,  0, 0, 0, 0, 0},
};
static int rgb_offset_EA8061V_revA[][RGB_COMPENSATION] = {
/*	R255 G255 B255 R203 G203 B203 R151 G151 B151
	R87 G87 B87 R51 G51 B51 R35 G35 B35
	R23 G23 B23 R11 G11 B11
*/
	{-6,2,-7,-3,1,-5,-4,1,-5,-10,4,-13,-5,3,-11,-3,1,-7,0,0,-3,0,0,-1},
	{-4,2,-4,-2,1,-4,-4,1,-5,-9,3,-12,-6,3,-9,-3,2,-6,-2,0,-4,0,0,-2},
	{-4,2,-3,-2,0,-4,-3,1,-4,-9,3,-10,-6,3,-8,-3,2,-7,-1,1,-5,0,0,-3},
	{-3,1,-2,-1,0,-3,-3,1,-4,-8,3,-9,-6,3,-8,-3,2,-7,-1,3,-6,0,0,-4},
	{-2,1,-2,-1,0,-2,-2,0,-3,-8,3,-9,-6,2,-8,-4,2,-7,-2,3,-6,0,0,-5},
	{-2,1,-1,-1,0,-2,-2,0,-3,-8,3,-9,-6,2,-8,-4,2,-7,-3,1,-6,1,1,-7},
	{-2,1,-1,-1,0,-2,-2,0,-3,-8,2,-9,-6,2,-8,-5,2,-7,-4,2,-6,3,2,-7},
	{-2,1,0,-1,0,-2,-2,0,-3,-8,2,-8,-6,2,-7,-4,1,-4,-2,2,-6,6,3,-7},
	{-2,1,0,-1,0,-1,-2,0,-3,-6,2,-8,-6,2,-7,-5,0,-5,-3,2,-9,7,2,-6},
	{-2,1,0,-1,0,-1,-2,0,-3,-6,2,-8,-6,2,-6,-4,0,-5,-3,2,-9,6,2,-7},
	{-2,1,0,-1,0,-1,-2,0,-3,-6,2,-8,-6,2,-6,-5,1,-6,-3,2,-9,6,2,-8},
	{-2,1,0,-1,0,-1,-2,0,-3,-6,2,-8,-5,1,-3,-2,1,-3,-4,1,-9,4,2,-10},
	{-2,0,0,-1,0,-1,-2,0,-2,-6,2,-8,-5,1,-4,-3,1,-4,-2,1,-8,4,3,-10},
	{-2,0,0,-1,0,-1,-1,0,-2,-6,2,-7,-5,1,-4,-2,1,-5,-2,1,-8,4,4,-10},
	{-2,0,0,-1,0,-1,0,0,-2,-6,2,-6,-5,1,-4,-3,1,-7,-2,1,-9,4,4,-10},
	{-1,0,0,-1,0,-1,0,0,-1,-5,2,-6,-5,1,-3,-3,1,-6,-3,1,-8,4,2,-11},
	{-1,0,0,-1,0,-1,0,0,-1,-5,2,-5,-5,1,-3,-3,1,-6,-3,1,-9,3,2,-12},
	{-1,0,0,-1,0,-1,0,0,-1,-6,2,-6,-5,1,-2,2,1,-4,-3,2,-9,-2,2,-14},
	{-1,0,0,-1,0,-1,0,0,-1,-6,2,-6,-4,1,-2,2,1,-5,-3,1,-8,-1,2,-14},
	{-1,0,0,-1,0,-1,0,0,-1,-6,2,-6,-4,1,-2,2,1,-3,-3,1,-9,0,2,-14},
	{-1,0,0,-1,0,0,0,0,0,-5,2,-5,-3,1,-1,1,1,-4,-3,0,-8,1,3,-14},
	{-1,0,0,-1,0,0,0,0,0,-5,2,-5,-3,1,-1,1,1,-5,-3,0,-8,2,3,-14},
	{-1,0,0,-1,0,0,0,0,0,-5,2,-5,-2,1,-1,0,1,-5,-4,0,-8,4,4,-14},
	{-1,0,0,-1,0,0,0,0,0,-5,2,-5,-2,1,-1,0,1,-5,-4,0,-8,5,4,-13},
	{-1,0,0,-1,0,0,0,0,0,-5,2,-5,-2,1,-1,0,1,-5,-4,0,-8,6,4,-13},
	{-1,0,0,-1,0,0,0,0,0,-4,1,-5,-4,0,-3,-2,1,-5,-4,1,-8,3,3,-10},
	{-1,0,0,-1,0,0,0,0,0,-4,1,-5,-4,0,-3,-3,1,-3,-4,1,-5,5,3,-9},
	{-1,0,0,-1,0,0,0,0,0,-4,0,-5,-5,0,-2,-1,0,-4,-4,0,-8,3,3,-10},
	{-1,0,0,-1,0,0,0,0,0,-4,0,-5,-5,0,-1,0,0,-4,-6,0,-8,1,3,-10},
	{-1,0,0,-1,0,0,0,0,0,-4,0,-5,-5,0,-2,0,-1,-4,-3,0,-5,6,3,-11},
	{-1,0,0,-1,0,0,0,0,0,-4,0,-6,-4,0,-2,-2,-1,-3,-5,0,-7,4,1,-8},
	{-1,0,0,0,0,0,0,0,0,-4,0,-6,-4,0,-2,-3,0,-4,-5,1,-8,3,1,-8},
	{-1,0,0,0,0,0,0,0,0,-5,0,-6,-2,0,0,-1,0,-4,-5,1,-7,2,1,-7},
	{-1,0,0,0,0,0,0,0,0,-5,0,-6,-2,0,0,-1,0,-3,-3,1,-6,3,0,-11},
	{-1,0,0,0,0,0,0,0,0,-5,0,-6,0,0,1,-1,0,-2,-5,0,-7,4,1,-10},
	{-1,0,0,0,0,-1,0,0,1,-4,1,-5,0,0,2,-1,1,-3,-2,0,-3,4,1,-10},
	{-1,0,0,0,0,0,0,0,0,-4,1,-4,0,0,2,-1,1,-2,-3,1,-5,3,3,-13},
	{-1,0,0,0,0,-1,0,0,0,-4,0,-5,-2,0,0,0,0,-1,-3,1,-5,4,2,-11},
	{-1,0,0,0,0,-1,0,0,0,-4,0,-5,-2,0,0,0,0,-1,-3,1,-5,4,2,-11},
	{-1,0,0,0,0,-1,0,0,0,-4,0,-5,-1,0,0,0,0,-2,-3,1,-4,3,2,-11},
	{-1,0,0,0,0,-1,0,0,0,-4,0,-5,-1,0,0,0,0,-1,-3,1,-3,2,2,-10},
	{-1,0,0,0,0,-1,0,0,0,-4,0,-5,-1,0,0,0,0,-1,-3,1,-3,2,2,-10},
	{-1,0,0,0,0,0,0,0,0,-3,0,-4,-2,0,-1,-1,0,0,-3,0,-2,1,2,-10},
	{-1,0,0,0,0,0,0,0,0,-3,0,-4,-2,0,-1,-1,0,0,-3,0,-2,1,2,-9},
	{-1,0,0,0,0,0,0,0,0,-2,0,-3,-2,0,-1,-1,0,0,-2,0,-3,0,1,-8},
	{-1,0,0,0,0,0,0,0,0,-2,0,-3,-2,0,-1,-1,0,0,-2,0,-2,0,1,-8},
	{-1,0,0,0,0,0,0,0,0,-1,0,-2,-1,0,-1,0,0,0,-1,1,-1,0,1,-8},
	{-1,0,0,0,0,0,0,0,0,-1,0,-2,-1,0,-1,0,0,0,-1,1,-1,0,1,-8},
	{-1,0,0,0,0,0,0,0,0,-1,0,-2,-1,0,-1,-1,0,0,-2,1,-1,-1,1,-8},
	{-1,0,0,0,0,0,0,0,0,-2,0,-2,0,0,-2,-2,0,0,-2,1,-2,-2,1,-8},
	{-1,0,0,0,0,0,0,0,-1,-2,0,-2,0,0,-2,-3,0,-1,-3,1,-2,-2,1,-8},
	{-1,0,0,0,0,0,0,0,0,0,0,-1,-1,0,-2,0,0,0,-2,0,-2,-2,1,-5},
	{-1,0,0,0,0,0,0,0,0,0,0,-1,-1,0,-2,0,0,0,-2,0,-2,-3,1,-5},
	{-1,0,0,0,0,0,0,0,0,0,0,-1,-1,0,-2,0,0,0,-2,0,-1,-1,1,-3},
	{-1,0,0,0,0,0,0,0,0,0,0,0,-1,0,-2,0,0,0,-2,0,0,0,0,-3},
	{-1,0,0,0,0,0,0,0,0,0,0,0,-1,0,-1,-1,0,0,-3,0,0,0,0,-2},
	{-1,0,0,0,0,0,0,0,0,0,0,0,-1,0,-1,-1,0,0,-3,0,1,0,0,-1},
	{-1,0,0,0,0,0,0,0,0,0,0,0,-1,0,-1,-1,0,0,-3,0,2,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static void gamma_init_EA8061V_revA(struct SMART_DIM *pSmart, char *str, int size)
{
	long long candela_level[EA8061V_TABLE_MAX] = {-1, };
	int bl_index[EA8061V_TABLE_MAX] = {-1, };

	long long temp_cal_data = 0;
	int bl_level;

	int level_255_temp_MSB = 0;
	int level_V255 = 0;

	int point_index;
	int cnt;
	int table_index;

	pr_info_once("%s : start !!\n",__func__);
	/*calculate candela level */
	if (pSmart->brightness_level > 249) {
		/* 265CD ~ 350CD */
		bl_level = pSmart->brightness_level;

	}else if ((pSmart->brightness_level <= 249) &&
				(pSmart->brightness_level > 162)) {
		/* 172CD ~ 249CD */
		bl_level = 249;
	} else if ((pSmart->brightness_level <= 162) &&
				(pSmart->brightness_level >= 68)) {

		/* 72CD ~ 162CD */
		if (pSmart->brightness_level == 68)
				bl_level = 116;
		else if (pSmart->brightness_level == 72)
			bl_level = 123;
		else if (pSmart->brightness_level == 77)
			bl_level = 131;
		else if (pSmart->brightness_level == 82)
			bl_level = 139;
		else if (pSmart->brightness_level == 87)
			bl_level = 147;
		else if (pSmart->brightness_level == 93)
			bl_level = 157;
		else if (pSmart->brightness_level == 98)
			bl_level = 165;
		else if (pSmart->brightness_level == 105)
			bl_level = 176;
		else if (pSmart->brightness_level == 111)
			bl_level = 185;
		else if (pSmart->brightness_level == 119)
			bl_level = 197;
		else if (pSmart->brightness_level == 126)
			bl_level = 207;
		else if (pSmart->brightness_level == 134)
			bl_level = 219;
		else if (pSmart->brightness_level == 143)
			bl_level = 232;
		else if (pSmart->brightness_level == 152)
			bl_level = 245;
		else if (pSmart->brightness_level == 162)
			bl_level = 249;
		else
			bl_level = 249;

	} else {
		/* 60CD ~ 2CD */
		bl_level = 110;
	}

	if (pSmart->brightness_level < 350) {
		for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
			point_index = EA8061V_ARRAY[cnt+1];
			temp_cal_data =
			((long long)(candela_coeff_2p15[point_index])) *
			((long long)(bl_level));
			candela_level[cnt] = temp_cal_data;
		}
	} else {
		for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
			point_index = EA8061V_ARRAY[cnt+1];
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

	for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
		if (searching_function(candela_level[cnt],
			&(bl_index[cnt]), GAMMA_CURVE_2P2)) {
			pr_info("%s searching functioin error cnt:%d\n",
			__func__, cnt);
		}
	}

	/*
	*	Candela compensation
	*/
	for (cnt = 1; cnt < EA8061V_TABLE_MAX; cnt++) {
		table_index = find_cadela_table(pSmart->brightness_level);

		if (table_index == -1) {
			table_index = CCG6_MAX_TABLE;
			pr_info("%s fail candela table_index cnt : %d brightness %d",
				__func__, cnt, pSmart->brightness_level);
		}

		bl_index[EA8061V_TABLE_MAX - cnt] +=
			gradation_offset_EA8061V_revA[table_index][cnt - 1];

		/* THERE IS M-GRAY0 target */
		if (bl_index[EA8061V_TABLE_MAX - cnt] == 0)
			bl_index[EA8061V_TABLE_MAX - cnt] = 1;
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
	for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++)
		(void)Make_hexa[cnt](bl_index , pSmart, str);

	/*
	*	RGB compensation
	*/
	for (cnt = 0; cnt < RGB_COMPENSATION; cnt++) {
		table_index = find_cadela_table(pSmart->brightness_level);

		if (table_index == -1) {
			table_index = CCG6_MAX_TABLE;
			pr_info("%s fail RGB table_index cnt : %d brightness %d",
				__func__, cnt, pSmart->brightness_level);
		}

		if (cnt < 3) {
			level_V255 = str[cnt * 2] << 8 | str[(cnt * 2) + 1];
			level_V255 +=
				rgb_offset_EA8061V_revA[table_index][cnt];
			level_255_temp_MSB = level_V255 / 256;

			str[cnt * 2] = level_255_temp_MSB & 0xff;
			str[(cnt * 2) + 1] = level_V255 & 0xff;
		} else {
			str[cnt+3] += rgb_offset_EA8061V_revA[table_index][cnt];
		}
	}
	/*subtration MTP_OFFSET value from generated gamma table*/
	mtp_offset_substraction(pSmart, str);
}

static int gradation_offset_EA8061V_revC[][9] = {
/*	V255 V203 V151 V87 V51 V35 V23 V11 V3 */
/*2  */{0, 9, 19, 31, 42, 47, 52, 56, 56},
/*3  */{0, 9, 15, 26, 34, 38, 42, 46, 46},
/*4  */{0, 7, 14, 22, 30, 34, 39, 42, 42},
/*5  */{0, 6, 11, 18, 25, 29, 34, 38, 38},
/*6  */{0, 6, 10, 16, 23, 26, 31, 35, 35},
/*7  */{0, 6, 10, 14, 20, 24, 28, 32, 32},
/*8  */{0, 5, 9,  13, 19, 22, 26, 30, 30},
/*9  */{0, 5, 8,  12, 18, 22, 25, 28, 28},
/*10 */{0, 5, 7,  12, 16, 19, 23, 27, 27},
/*11 */{0, 4, 6,  12,  16, 18, 22, 26, 26},
/*12 */{0, 4, 6,  11,  15, 18, 22, 25, 25},
/*13 */{0, 4, 6,  11,  14, 17, 21, 24, 24},
/*14 */{0, 4, 6,  10,  13, 16, 20, 23, 23},
/*15 */{0, 4, 6,  9,  12, 15, 19, 22, 22},
/*16 */{0, 4, 6,  9,  12, 15, 18, 21, 21},
/*17 */{0, 4, 5,  9,  11, 13, 17, 20, 20},
/*19 */{0, 3, 4,  8,  10, 13, 16, 19, 19},
/*20 */{0, 3, 4,  7,  10, 12, 16, 18, 18},
/*21 */{0, 3, 4,  7,  9, 11, 15, 18, 18},
/*22 */{0, 3, 4,  7,  9, 11, 15, 17, 17},
/*24 */{0, 3, 4,  6,  8, 11, 14, 16, 16},
/*25 */{0, 3, 4,  6,  8, 10, 14, 16, 16},
/*27 */{0, 3, 4,  6,  7, 9, 12, 14, 14},
/*29 */{0, 3, 3,  5,  6, 8, 12, 14, 14},
/*30 */{0, 3, 3,  5,  6, 8, 11, 14, 14},
/*32 */{0, 3, 3,  5,  6, 7, 10, 13, 13},
/*34 */{0, 3, 3,  5,  6, 7, 9, 12, 12},
/*37 */{0, 3, 2,  4,  5, 6, 8, 11, 11},
/*39 */{0, 3, 2,  4,  4, 6, 8, 10, 10},
/*41 */{0, 3, 2,  3,  4, 5, 7, 10, 10},
/*44 */{0, 3, 2,  3,  4, 5, 7, 10, 10},
/*47 */{0, 3, 2,  3,  3, 4, 6, 9, 9},
/*50 */{0, 3, 2,  3,  3, 4, 6, 8, 8},
/*53 */{0, 3, 2,  3,  3, 3, 5, 7, 7},
/*56 */{0, 3, 2,  3,  3, 3, 5, 7, 7},
/*60 */{0, 2, 2,  2,  2, 3, 5, 6, 6},
/*64 */{0, 2, 2,  2,  2, 3, 5, 6, 6},
/*68 */{0, 2, 2,  3,  2, 3, 4, 6, 6},
/*72 */{0, 3, 3,  4,  3, 3, 5, 6, 6},
/*77 */{0, 4, 4,  4,  3, 4, 5, 6, 6},
/*82 */{0, 4, 4,  5,  3, 4, 5, 7, 7},
/*87 */{0, 4, 4,  5,  3, 4, 4, 7, 7},
/*93 */{0, 4, 4,  5,  3, 4, 4, 7, 7},
/*98 */{0, 4, 4,  5,  3, 3, 4, 7, 7},
/*105*/{0, 4, 5,  5,  4, 4, 4, 6, 6},
/*111*/{0, 4, 5,  5,  4, 4, 4, 6, 6},
/*119*/{0, 3, 4,  4,  3, 3, 4, 6, 6},
/*126*/{0, 3, 3,  4,  3, 3, 4, 6, 6},
/*134*/{0, 2, 2,  3,  2, 3, 3, 6, 6},
/*143*/{0, 1, 1,  2,  2, 3, 4, 5, 5},
/*152*/{0, 0, 0,  1,  1, 3, 3, 5, 4},
/*162*/{0, 0, 0,  3,  2, 3, 3, 5, 5},
/*172*/{0, 0, 0,  3,  2, 3, 2, 4, 4},
/*183*/{0, 0, 0,  2,  2, 2, 2, 3, 3},
/*195*/{0, 0, 0,  2,  2, 2, 2, 2, 2},
/*207*/{0, 0, 0,  2,  2, 2, 2, 2, 2},
/*220*/{0, 0, 0,  2,  2, 2, 1, 2, 2},
/*234*/{0, 0, 0,  1,  1, 1, 1, 1, 1},
/*249*/{3, 3, 4,  3,  2, 2, 1, 1, 1},
/*265*/{3, 3, 4,  3,  1, 1, 1, 1, 1},
/*282*/{2, 2, 3,  2,  1, 1, 0, 0, 0},
/*300*/{1, 1, 2,  2,  1, 1, 0, 0, 0},
/*316*/{1, 1, 2,  1,  0, 0, 0, 0, 0},
/*333*/{0, 0, 2,  1,  0, 0, 0, 0, 0},
/*350*/{0, 0, 0,  0,  0, 0, 0, 0, 0},
};
static int rgb_offset_EA8061V_revC[][RGB_COMPENSATION] = {
/*	R255 G255 B255 R203 G203 B203 R151 G151 B151
	R87 G87 B87 R51 G51 B51 R35 G35 B35
	R23 G23 B23 R11 G11 B11
*/
/*2  */	{-7,3,-12,-5,2,-7,-6,2,-7,-11,5,-16,-5,6,-13,-2,2,-10,-3,3,-11,0,1,-3},
/*3  */	{-6,2,-10,-4,2,-5,-5,2,-6,-11,4,-13,-5,6,-15,-2,2,-10,-3,3,-10,0,2,-5},
/*4  */	{-4,2,-7,-4,1,-4,-4,1,-6,-11,4,-12,-8,6,-17,-4,3,-10,-3,3,-10,0,2,-7},
/*5  */	{-2,2,-4,-4,1,-4,-4,1,-6,-10,4,-11,-9,5,-17,-4,4,-10,-3,2,-9,0,2,-9},
/*6  */	{-2,2,-4,-4,0,-3,-3,1,-4,-9,4,-10,-9,5,-15,-4,3,-10,-3,2,-9,0,2,-9},
/*7  */	{-2,2,-4,-4,0,-3,-3,1,-4,-8,4,-10,-9,5,-14,-4,3,-10,-3,3,-9,0,2,-10},
/*8  */	{-2,1,-3,-3,0,-3,-3,1,-4,-8,3,-9,-9,5,-14,-4,3,-10,-3,3,-9,0,3,-10},
/*9  */	{-2,0,-3,-2,0,-2,-3,1,-4,-8,3,-8,-9,5,-14,-4,3,-10,-3,3,-9,0,3,-10},
/*10 */	{-2,0,-3,-2,0,-2,-3,1,-4,-7,3,-8,-9,4,-14,-4,3,-10,-3,3,-9,0,3,-11},
/*11 */	{-2,0,-2,-2,0,-2,-2,0,-4,-6,3,-8,-9,4,-14,-4,3,-10,-3,3,-9,0,4,-12},
/*12 */	{-2,0,-2,-2,0,-2,-2,0,-4,-6,3,-8,-9,4,-13,-5,3,-9,-3,3,-9,0,4,-12},
/*13 */	{-2,0,-2,-2,0,-2,-2,0,-4,-6,2,-8,-9,4,-13,-5,3,-9,-3,3,-9,0,4,-13},
/*14 */	{-2,0,-2,-2,0,-2,-2,0,-4,-6,2,-8,-8,4,-11,-5,2,-9,-3,3,-8,0,4,-14},
/*15 */	{-2,0,-2,-2,0,-2,-2,0,-3,-6,2,-7,-8,4,-11,-5,2,-9,-3,3,-7,0,4,-14},
/*16 */	{-2,0,-2,-2,0,-2,-2,0,-3,-6,2,-7,-8,4,-9,-5,2,-9,-3,3,-7,0,4,-14},
/*17 */	{-2,0,-2,-2,0,-2,-2,0,-3,-5,1,-7,-8,4,-9,-5,2,-9,-3,3,-7,0,5,-15},
/*19 */	{-1,0,-2,-1,0,-2,-2,0,-2,-5,1,-6,-8,3,-8,-5,2,-9,-3,3,-7,-1,6,-16},
/*20 */	{-1,0,-2,-1,0,-2,-2,0,-2,-5,1,-6,-8,3,-8,-5,2,-9,-3,3,-6,-1,6,-16},
/*21 */	{-1,0,-2,-1,0,-2,-2,0,-1,-5,1,-6,-8,3,-8,-5,2,-9,-3,3,-6,-1,6,-16},
/*22 */	{-1,0,-2,-1,0,-2,-2,0,-1,-5,1,-5,-8,3,-8,-5,2,-9,-3,4,-6,-1,6,-16},
/*24 */	{-1,0,-2,-1,0,-2,-2,0,-1,-4,1,-4,-7,2,-8,-5,2,-9,-3,3,-6,-1,6,-16},
/*25 */	{-1,0,-2,-1,0,-2,-2,0,-1,-4,1,-4,-7,2,-8,-5,2,-9,-3,3,-6,-1,6,-16},
/*27 */	{-1,0,-2,-1,0,-2,-2,0,-1,-4,1,-4,-7,2,-8,-5,2,-7,-3,3,-6,-1,6,-16},
/*29 */	{-1,0,-1,-1,0,-2,-1,0,-1,-3,1,-3,-6,1,-8,-5,1,-7,-3,3,-6,-1,5,-17},
/*30 */	{-1,0,-1,-1,0,-2,-1,0,-1,-3,1,-3,-6,1,-8,-5,1,-7,-3,3,-6,-1,5,-17},
/*32 */	{-1,0,-1,-1,0,-2,-1,0,0,-3,1,-3,-6,1,-8,-5,1,-7,-3,3,-6,-1,5,-17},
/*34 */	{-1,0,-1,-1,0,-2,-1,0,0,-3,1,-3,-6,1,-8,-5,1,-7,-3,3,-6,-1,5,-17},
/*37 */	{0,0,0,-1,0,-2,0,0,0,-3,1,-2,-6,1,-8,-5,1,-6,-3,3,-6,0,4,-18},
/*39 */	{0,0,0,-1,0,-2,0,0,0,-2,1,-2,-6,1,-7,-5,1,-6,-3,3,-6,0,4,-18},
/*41 */	{0,0,0,-1,0,-2,0,0,0,-2,1,-1,-5,1,-6,-5,1,-5,-3,3,-6,0,4,-18},
/*44 */	{0,0,0,-1,0,-2,0,0,0,-2,0,-1,-5,1,-6,-5,1,-4,-3,2,-6,0,4,-18},
/*47 */	{0,0,0,-1,0,-2,0,0,0,-2,0,-1,-4,3,-5,-4,1,-3,-3,2,-5,0,4,-19},
/*50 */	{0,0,0,-1,0,-2,0,0,0,-2,0,-1,-3,1,-4,-4,1,-2,-3,2,-5,0,4,-19},
/*53 */	{0,0,0,-1,0,-2,0,0,0,-2,0,-1,-2,1,-4,-4,1,-2,-3,3,-5,0,4,-19},
/*56 */	{0,0,0,-1,0,-2,0,0,0,-2,0,-1,-2,1,-4,-3,1,-1,-3,2,-5,0,4,-19},
/*60 */	{0,0,0,0,0,-2,0,0,0,-2,0,-1,-2,1,-4,-3,1,-1,-4,2,-4,0,4,-20},
/*64 */	{0,0,0,0,0,-1,-1,0,0,-2,0,-1,-1,1,-4,-3,1,-1,-5,1,-4,0,5,-20},
/*68 */	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-4,-3,2,-1,-5,1,-4,0,5,-20},
/*72 */	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-4,-3,2,-2,-5,1,-4,0,5,-19},
/*77 */	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-4,-3,2,-2,-5,0,-4,0,5,-18},
/*82 */	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-3,-3,2,-3,-5,0,-4,-1,5,-18},
/*87 */	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-3,-3,2,-3,-5,0,-3,-1,5,-17},
/*93 */	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-3,-3,2,-4,-5,0,-3,-1,5,-16},
/*98 */	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-3,-3,2,-4,-5,0,-3,-2,5,-16},
/*105*/	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-2,-2,2,-5,-5,0,-3,-3,5,-16},
/*111*/	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-2,-2,1,-5,-5,0,-3,-4,5,-16},
/*119*/	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-2,-3,0,-5,-5,0,-3,-5,5,-15},
/*126*/	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-2,-3,0,-5,-5,0,-3,-5,5,-15},
/*134*/	{-1,0,0,0,0,-1,0,0,-1,0,0,0,-2,0,-2,-3,0,-4,-5,0,-3,-5,4,-14},
/*143*/	{-1,0,0,0,0,0,0,0,-1,0,0,0,-2,0,-2,-3,0,-4,-4,0,-3,-6,4,-14},
/*152*/	{-1,0,0,0,0,0,0,0,-1,0,0,0,-2,0,-2,-3,0,-3,-3,0,-4,-6,4,-14},
/*162*/	{0,0,-1,0,0,0,0,0,0,0,0,0,-2,0,-2,-3,0,-3,-3,0,-2,-6,3,-12},
/*172*/	{0,0,-1,0,0,0,0,0,0,0,0,0,-2,0,-2,-3,0,-3,-3,0,-1,-6,3,-12},
/*183*/	{0,0,-1,0,0,0,0,0,0,0,0,0,-2,0,-1,-3,0,-3,-3,0,0,-6,3,-11},
/*195*/	{0,0,-1,0,0,0,0,0,0,0,0,0,-1,0,-1,-3,0,-2,-3,0,0,-4,2,-9},
/*207*/	{0,0,-1,0,0,0,0,0,0,0,0,0,-1,0,-1,-3,0,-2,-3,0,0,-3,2,-7},
/*220*/	{0,0,-1,0,0,0,0,0,0,0,0,0,-1,0,-1,-2,0,-1,-3,0,0,-2,2-5},
/*234*/	{0,0,-1,0,0,0,0,0,0,0,0,0,-1,0,-1,0,0,0,-2,0,0,0,0,0},
/*249*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*265*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*282*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*300*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*316*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*333*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*350*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static void gamma_init_EA8061V_revC(struct SMART_DIM *pSmart, char *str, int size)
{
	long long candela_level[EA8061V_TABLE_MAX] = {-1, };
	int bl_index[EA8061V_TABLE_MAX] = {-1, };

	long long temp_cal_data = 0;
	int bl_level;

	int level_255_temp_MSB = 0;
	int level_V255 = 0;

	int point_index;
	int cnt;
	int table_index;

	pr_debug("%s : start !!\n",__func__);
	/*calculate candela level */
	if (pSmart->brightness_level == 350) {
		bl_level = pSmart->brightness_level;

	}else if (pSmart->brightness_level == 333)
		bl_level = 333;
	else if (pSmart->brightness_level == 316)
		bl_level = 316;
	else if (pSmart->brightness_level == 300)
		bl_level = 300;
	else if (pSmart->brightness_level == 282)
		bl_level = 282;
	else if (pSmart->brightness_level == 265)
		bl_level = 265;


	else if ((pSmart->brightness_level <= 249) &&
				(pSmart->brightness_level >= 162)) {
		bl_level = 249;
	} else if ((pSmart->brightness_level <= 152) &&
				(pSmart->brightness_level >= 68)) {

		if (pSmart->brightness_level == 68)
			bl_level = 116;
		else if (pSmart->brightness_level == 72)
			bl_level = 123;
		else if (pSmart->brightness_level == 77)
			bl_level = 131;
		else if (pSmart->brightness_level == 82)
			bl_level = 139;
		else if (pSmart->brightness_level == 87)
			bl_level = 147;
		else if (pSmart->brightness_level == 93)
			bl_level = 157;
		else if (pSmart->brightness_level == 98)
			bl_level = 165;
		else if (pSmart->brightness_level == 105)
			bl_level = 176;
		else if (pSmart->brightness_level == 111)
			bl_level = 185;
		else if (pSmart->brightness_level == 119)
			bl_level = 197;
		else if (pSmart->brightness_level == 126)
			bl_level = 207;
		else if (pSmart->brightness_level == 134)
			bl_level = 219;
		else if (pSmart->brightness_level == 143)
			bl_level = 232;
		else
			bl_level = 245;

	} else {
		bl_level = 110;
	}

	if (pSmart->brightness_level < 350) {
		for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
			point_index = EA8061V_ARRAY[cnt+1];
			temp_cal_data =
			((long long)(candela_coeff_2p15[point_index])) *
			((long long)(bl_level));
			candela_level[cnt] = temp_cal_data;
		}
	} else {
		for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
			point_index = EA8061V_ARRAY[cnt+1];
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

	for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
		if (searching_function(candela_level[cnt],
			&(bl_index[cnt]), GAMMA_CURVE_2P2)) {
			pr_info("%s searching functioin error cnt:%d\n",
			__func__, cnt);
		}
	}

	/*
	*	Candela compensation
	*/
	for (cnt = 1; cnt < EA8061V_TABLE_MAX; cnt++) {
		table_index = find_cadela_table(pSmart->brightness_level);

		if (table_index == -1) {
			table_index = CCG6_MAX_TABLE;
			pr_info("%s fail candela table_index cnt : %d brightness %d",
				__func__, cnt, pSmart->brightness_level);
		}

		bl_index[EA8061V_TABLE_MAX - cnt] +=
			gradation_offset_EA8061V_revC[table_index][cnt - 1];

		/* THERE IS M-GRAY0 target */
		if (bl_index[EA8061V_TABLE_MAX - cnt] == 0)
			bl_index[EA8061V_TABLE_MAX - cnt] = 1;
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
	for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++)
		(void)Make_hexa[cnt](bl_index , pSmart, str);

	/*
	*	RGB compensation
	*/
	for (cnt = 0; cnt < RGB_COMPENSATION; cnt++) {
		table_index = find_cadela_table(pSmart->brightness_level);

		if (table_index == -1) {
			table_index = CCG6_MAX_TABLE;
			pr_info("%s fail RGB table_index cnt : %d brightness %d",
				__func__, cnt, pSmart->brightness_level);
		}

		if (cnt < 3) {
			level_V255 = str[cnt * 2] << 8 | str[(cnt * 2) + 1];
			level_V255 +=
				rgb_offset_EA8061V_revC[table_index][cnt];
			level_255_temp_MSB = level_V255 / 256;

			str[cnt * 2] = level_255_temp_MSB & 0xff;
			str[(cnt * 2) + 1] = level_V255 & 0xff;
		} else {
			str[cnt+3] += rgb_offset_EA8061V_revC[table_index][cnt];
		}
	}
	/*subtration MTP_OFFSET value from generated gamma table*/
	mtp_offset_substraction(pSmart, str);
}

static int gradation_offset_EA8061V_revD[][9] = {
/*	V255 V203 V151 V87 V51 V35 V23 V11 V3 */
/*2  */{0, 8, 17, 28, 39, 43, 48, 48, 48},
/*3  */{0, 6, 12, 23, 32, 34, 39, 40, 40},
/*4  */{0, 6, 12, 21, 28, 30, 35, 37, 37},
/*5  */{0, 5, 11, 19, 25, 28, 32, 32, 32},
/*6  */{0, 4, 10, 17, 22, 24, 29, 32, 32},
/*7  */{0, 4, 9,  15, 20, 23, 27, 29, 29},
/*8  */{0, 4, 9,  14, 19, 22, 25, 27, 27},
/*9  */{0, 4, 8,  12, 18, 20, 24, 26, 26},
/*10 */{0, 4, 7,  11, 16, 19, 22, 24, 24},
/*11 */{0, 4, 6,  10,  16, 17, 20, 23, 23},
/*12 */{0, 4, 6,  9,  15, 17, 19, 22, 22},
/*13 */{0, 4, 6,  8,  13, 15, 18, 20, 20},
/*14 */{0, 4, 6,  8,  12, 14, 17, 20, 20},
/*15 */{0, 4, 6,  8,  11, 13, 16, 19, 19},
/*16 */{0, 4, 6,  8,  10, 12, 15, 17, 17},
/*17 */{0, 3, 5,  7,  10, 12, 15, 17, 17},
/*19 */{0, 3, 4,  6,  10, 11, 14, 16, 16},
/*20 */{0, 3, 4,  6,  9, 11, 13, 15, 15},
/*21 */{0, 3, 4,  6,  8, 10, 12, 14, 14},
/*22 */{0, 3, 4,  6,  8, 10, 12, 14, 14},
/*24 */{0, 3, 4,  5,  8, 9, 12, 14, 14},
/*25 */{0, 3, 4,  5,  8, 9, 12, 14, 14},
/*27 */{0, 3, 4,  4,  7, 8, 11, 13, 13},
/*29 */{0, 3, 3,  4,  6, 7, 10, 12, 12},
/*30 */{0, 3, 3,  4,  6, 7, 10, 12, 12},
/*32 */{0, 3, 2,  4,  5, 6, 10, 11, 11},
/*34 */{0, 3, 2,  3,  6, 6, 9, 11, 11},
/*37 */{0, 3, 2,  3,  5, 6, 8, 10, 10},
/*39 */{0, 3, 2,  3,  4, 5, 8, 9, 9},
/*41 */{0, 3, 2,  3,  4, 5, 7, 9, 9},
/*44 */{0, 3, 2,  3,  4, 5, 7, 8, 8},
/*47 */{0, 3, 2,  2,  4, 4, 6, 8, 8},
/*50 */{0, 3, 2,  2,  3, 4, 6, 7, 7},
/*53 */{0, 3, 2,  2,  3, 3, 6, 7, 7},
/*56 */{0, 3, 2,  2,  3, 3, 5, 6, 6},
/*60 */{0, 2, 2,  2,  2, 3, 4, 6, 6},
/*64 */{0, 2, 2,  1,  2, 3, 4, 5, 5},
/*68 */{0, 2, 2,  1,  2, 2, 4, 5, 5},
/*72 */{0, 2, 2,  1,  2, 2, 4, 5, 5},
/*77 */{0, 2, 2,  1,  2, 2, 4, 5, 5},
/*82 */{0, 2, 2,  1,  2, 2, 4, 5, 5},
/*87 */{0, 0, 0,  1,  2, 2, 3, 5, 5},
/*93 */{0, 0, 0,  1,  2, 2, 3, 5, 5},
/*98 */{0, 0, 0,  1,  2, 2, 3, 5, 5},
/*105*/{0, 0, 0,  1,  2, 2, 3, 5, 5},
/*111*/{0, 0, 0,  1,  3, 2, 3, 5, 5},
/*119*/{0, 0, 0,  1,  2, 2, 3, 5, 5},
/*126*/{0, 0, 0,  1,  2, 2, 3, 5, 5},
/*134*/{0, 0, 0,  1,  2, 2, 3, 5, 5},
/*143*/{0, 0, 0,  1,  2, 2, 3, 4, 4},
/*152*/{0, 0, 0,  1,  1, 2, 2, 4, 4},
/*162*/{0, 0, 0,  3,  2, 2, 3, 4, 4},
/*172*/{0, 0, 0,  2,  2, 2, 3, 4, 4},
/*183*/{0, 0, 0,  1,  1, 1, 2, 3, 3},
/*195*/{0, 0, 0,  0,  1, 1, 2, 2, 2},
/*207*/{0, 0, 0,  0,  1, 1, 1, 2, 2},
/*220*/{0, 0, 0,  0,  1, 1, 1, 1, 1},
/*234*/{0, 0, 0,  0,  0, 0, 0, 0, 0},
/*249*/{2, 1, 1,  1,  1, 1, 1, 1, 1},
/*265*/{2, 1, 1,  1,  1, 1, 1, 1, 1},
/*282*/{1, 0, 0,  0,  0, 0, 0, 0, 0},
/*300*/{0, 0, 0,  0,  0, 0, 0, 0, 0},
/*316*/{0, 0, 0,  0,  0, 0, 0, 0, 0},
/*333*/{0, 0, 0,  0,  0, 0, 0, 0, 0},
/*350*/{0, 0, 0,  0,  0, 0, 0, 0, 0},
};
static int rgb_offset_EA8061V_revD[][RGB_COMPENSATION] = {
/*	R255 G255 B255 R203 G203 B203 R151 G151 B151
	R87 G87 B87 R51 G51 B51 R35 G35 B35
	R23 G23 B23 R11 G11 B11
*/
/*2  */	{-7,3,-11,-5,2,-5,-6,2,-6,-11,4,-13,-9,4,-13,-3,3,-7,0,1,-6,0,0,0},
/*3  */	{-5,2,-9,-5,1,-4,-6,1,-5,-10,4,-12,-9,4,-13,-4,4,-8,-3,1,-9,0,0,0},
/*4  */	{-4,2,-6,-4,1,-4,-6,1,-5,-10,2,-12,-9,4,-13,-4,4,-8,-3,2,-9,0,0,0},
/*5  */	{-2,1,-5,-4,1,-4,-6,1,-4,-10,2,-12,-9,4,-13,-4,4,-8,-5,2,-10,0,0,0},
/*6  */	{-2,0,-5,-4,0,-4,-6,1,-4,-9,2,-10,-9,4,-12,-4,3,-8,-5,2,-10,0,0,-2},
/*7  */	{-1,0,-4,-3,0,-3,-5,0,-4,-9,2,-10,-9,4,-12,-4,3,-8,-5,3,-11,-1,1,-3},
/*8  */	{-1,0,-4,-3,0,-3,-5,0,-4,-8,2,-10,-9,3,-12,-4,3,-7,-5,3,-12,-2,2,-5},
/*9  */	{-1,0,-3,-2,0,-3,-4,0,-4,-8,2,-8,-9,2,-12,-4,3,-7,-6,3,-12,-2,2,-7},
/*10 */	{-1,0,-2,-2,0,-3,-4,0,-4,-8,2,-8,-9,2,-11,-4,3,-7,-6,3,-12,-3,3,-9},
/*11 */	{0,0,-2,-2,0,-2,-4,0,-4,-8,2,-8,-8,2,-10,-4,3,-7,-6,3,-12,-4,3,-10},
/*12 */	{0,0,-2,-2,0,-2,-4,0,-3,-7,2,-7,-8,2,-10,-5,3,-7,-6,3,-12,-4,3,-10},
/*13 */	{0,0,-2,-2,0,-2,-4,0,-3,-6,2,-7,-8,2,-9,-5,3,-7,-6,3,-12,-4,3,-10},
/*14 */	{0,0,-2,-2,0,-2,-4,0,-3,-6,2,-6,-8,2,-9,-5,2,-7,-6,4,-12,-4,3,-10},
/*15 */	{0,0,-2,-2,0,-2,-4,0,-3,-5,1,-5,-7,2,-9,-5,2,-7,-6,4,-12,-4,3,-11},
/*16 */	{0,0,-2,-2,0,-2,-4,0,-2,-5,1,-5,-7,2,-10,-5,2,-7,-6,4,-12,-4,4,-11},
/*17 */	{0,0,-2,-2,0,-2,-3,0,-2,-5,1,-5,-7,2,-10,-5,2,-7,-6,4,-12,-4,4,-11},
/*19 */	{0,0,-1,-1,0,-2,-3,0,-2,-5,0,-5,-7,2,-9,-5,2,-7,-6,4,-13,-4,4,-11},
/*20 */	{0,0,-1,-1,0,-2,-3,0,-2,-5,0,-5,-7,2,-8,-5,2,-7,-6,4,-13,-4,4,-12},
/*21 */	{0,0,-1,-1,0,-2,-3,0,-2,-5,0,-5,-7,2,-8,-5,2,-7,-6,4,-12,-4,4,-12},
/*22 */	{0,0,-1,-1,0,-2,-3,0,-2,-5,0,-5,-7,2,-8,-5,2,-7,-6,4,-12,-4,4,-12},
/*24 */	{0,0,-1,-1,0,-2,-3,0,-2,-5,0,-5,-7,2,-8,-5,2,-7,-6,3,-12,-4,4,-12},
/*25 */	{0,0,-1,-1,0,-2,-3,0,-2,-5,0,-5,-7,2,-8,-5,2,-7,-6,3,-12,-4,4,-13},
/*27 */	{0,0,-1,-1,0,-2,-3,0,-2,-4,0,-4,-6,2,-8,-5,2,-6,-6,2,-11,-4,4,-13},
/*29 */	{0,0,-1,0,0,-1,-3,0,-1,-4,0,-4,-6,1,-7,-5,1,-6,-6,2,-10,-4,4,-13},
/*30 */	{0,0,-1,0,0,-1,-3,0,-1,-4,0,-4,-6,1,-7,-5,1,-6,-6,2,-10,-4,4,-13},
/*32 */	{0,0,-1,0,0,-1,-2,0,0,-3,0,-4,-5,1,-6,-5,1,-6,-6,2,-10,-4,4,-13},
/*34 */	{0,0,-1,0,0,-1,-2,0,0,-3,0,-3,-4,1,-6,-5,1,-6,-6,2,-9,-4,4,-13},
/*37 */	{0,0,-1,0,0,-1,-1,0,0,-3,0,-3,-4,1,-6,-5,1,-6,-6,2,-9,-4,4,-13},
/*39 */	{0,0,-1,0,0,-1,-1,0,0,-3,0,-3,-4,1,-6,-5,1,-6,-6,2,-8,-4,4,-13},
/*41 */	{0,0,-1,0,0,-1,-1,0,0,-3,0,-3,-4,1,-6,-5,1,-5,-5,2,-8,-4,3,-13},
/*44 */	{0,0,-1,0,0,-1,-1,0,0,-3,0,-3,-4,1,-5,-5,1,-5,-4,2,-7,-4,3,-13},
/*47 */	{0,0,-1,0,0,-1,-1,0,0,-3,0,-2,-4,1,-4,-4,1,-4,-4,2,-6,-4,3,-13},
/*50 */	{0,0,-1,0,0,-1,-1,0,0,-3,0,-2,-3,1,-4,-4,1,-4,-3,2,-6,-4,3,-13},
/*53 */	{0,0,-1,0,0,-1,-1,0,0,-3,0,-2,-3,1,-4,-4,1,-4,-3,1,-6,-4,3,-13},
/*56 */	{0,0,-1,0,0,-1,-1,0,0,-3,0,-2,-3,1,-3,-3,1,-4,-3,1,-6,-4,3,-13},
/*60 */	{0,0,-1,0,0,-1,-1,0,0,-2,0,-2,-2,1,-2,-3,1,-4,-2,1,-6,-4,3,-13},
/*64 */	{0,0,-1,0,0,-1,-1,0,0,-2,0,-1,-2,1,-2,-3,1,-3,-2,1,-5,-4,3,-13},
/*68 */	{0,0,-1,0,0,-1,-1,0,0,-2,0,-1,-2,0,-4,-3,1,-1,-3,1,-3,-4,3,-11},
/*72 */	{0,0,-1,0,0,-1,-1,0,0,-2,0,-1,-2,0,-4,-3,1,-1,-3,1,-3,-4,3,-11},
/*77 */	{0,0,-1,0,0,-1,-1,0,0,-2,0,-1,-2,0,-4,-3,1,-1,-3,1,-3,-2,3,-11},
/*82 */	{0,0,0,0,0,0,0,0,0,-2,0,-1,-2,0,-4,-2,0,-2,-2,1,-4,-4,2,-11},
/*87 */	{0,0,0,0,0,0,0,0,0,-2,0,-1,-1,0,-3,-2,0,-2,-2,1,-4,-4,2,-11},
/*93 */	{0,0,0,0,0,0,0,0,0,-2,0,-1,-1,0,-2,-2,0,-2,-2,1,-4,-4,2,-12},
/*98 */	{0,0,0,0,0,0,0,0,0,-2,0,-1,-1,0,-2,-2,0,-2,-2,1,-4,-4,2,-12},
/*105*/	{0,0,0,0,0,0,0,0,0,-1,0,0,-1,0,-2,-2,0,-2,-3,0,-4,-4,2,-11},
/*111*/	{0,0,0,0,0,0,0,0,0,-1,0,0,-1,0,-2,-2,0,-2,-3,0,-4,-4,2,-11},
/*119*/	{0,0,0,0,0,0,0,0,0,-1,0,0,-1,0,-2,-2,0,-2,-3,0,-4,-4,2,-11},
/*126*/	{0,0,0,0,0,0,0,0,0,-1,0,0,-1,0,-2,-2,0,-2,-3,0,-4,-4,2,-11},
/*134*/	{0,0,0,0,0,0,0,0,0,0,0,0,-1,0,-2,-2,0,-2,-3,0,-4,-4,2,-10},
/*143*/	{0,0,0,0,0,0,0,0,0,0,0,0,-1,0,-2,-2,0,-2,-3,0,-4,-4,2,-8},
/*152*/	{0,0,0,0,0,0,0,0,0,0,0,0,-1,0,-2,-2,0,-3,-3,0,-4,-5,2,-6},
/*162*/	{0,0,0,0,0,0,0,0,0,0,0,0,-2,0,-2,-3,0,-2,-2,0,-2,-4,2,-6},
/*172*/	{0,0,0,0,0,0,0,0,0,0,0,0,-2,0,-2,-3,0,-2,-2,0,-2,-3,2,-6},
/*183*/	{0,0,0,0,0,0,0,0,0,0,0,0,-1,0,-2,-2,0,-2,-1,0,-1,-2,1,-5},
/*195*/	{0,0,0,0,0,0,0,0,0,0,0,0,-1,0,-1,-2,0,-2,-1,0,-1,-2,1,-4},
/*207*/	{0,0,0,0,0,0,0,0,0,0,0,0,-1,0,-1,-1,0,-2,-1,0,-1,-1,1,-4},
/*220*/	{0,0,0,0,0,0,0,0,0,0,0,0,-1,0,-1,-1,0,-2,-1,0,-1,-1,1,-3},
/*234*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*249*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*265*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*282*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*300*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*316*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*333*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*350*/	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static void gamma_init_EA8061V_revD(struct SMART_DIM *pSmart, char *str, int size)
{
	long long candela_level[EA8061V_TABLE_MAX] = {-1, };
	int bl_index[EA8061V_TABLE_MAX] = {-1, };

	long long temp_cal_data = 0;
	int bl_level;

	int level_255_temp_MSB = 0;
	int level_V255 = 0;

	int point_index;
	int cnt;
	int table_index;

	pr_debug("%s : start !!\n",__func__);
	/*calculate candela level */
	if (pSmart->brightness_level == 350) {
		bl_level = pSmart->brightness_level;

	}else if (pSmart->brightness_level == 333)
		bl_level = 333;
	else if (pSmart->brightness_level == 316)
		bl_level = 316;
	else if (pSmart->brightness_level == 300)
		bl_level = 300;
	else if (pSmart->brightness_level == 282)
		bl_level = 282;
	else if (pSmart->brightness_level == 265)
		bl_level = 265;


	else if ((pSmart->brightness_level <= 249) &&
				(pSmart->brightness_level >= 162)) {
		bl_level = 249;
	} else if ((pSmart->brightness_level <= 152) &&
				(pSmart->brightness_level >= 68)) {

		if (pSmart->brightness_level == 68)
			bl_level = 116;
		else if (pSmart->brightness_level == 72)
			bl_level = 123;
		else if (pSmart->brightness_level == 77)
			bl_level = 131;
		else if (pSmart->brightness_level == 82)
			bl_level = 139;
		else if (pSmart->brightness_level == 87)
			bl_level = 147;
		else if (pSmart->brightness_level == 93)
			bl_level = 157;
		else if (pSmart->brightness_level == 98)
			bl_level = 165;
		else if (pSmart->brightness_level == 105)
			bl_level = 176;
		else if (pSmart->brightness_level == 111)
			bl_level = 185;
		else if (pSmart->brightness_level == 119)
			bl_level = 197;
		else if (pSmart->brightness_level == 126)
			bl_level = 207;
		else if (pSmart->brightness_level == 134)
			bl_level = 219;
		else if (pSmart->brightness_level == 143)
			bl_level = 232;
		else
			bl_level = 245;

	} else {
		bl_level = 110;
	}

	if (pSmart->brightness_level < 350) {
		for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
			point_index = EA8061V_ARRAY[cnt+1];
			temp_cal_data =
			((long long)(candela_coeff_2p15[point_index])) *
			((long long)(bl_level));
			candela_level[cnt] = temp_cal_data;
		}
	} else {
		for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
			point_index = EA8061V_ARRAY[cnt+1];
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

	for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
		if (searching_function(candela_level[cnt],
			&(bl_index[cnt]), GAMMA_CURVE_2P2)) {
			pr_info("%s searching functioin error cnt:%d\n",
			__func__, cnt);
		}
	}

	/*
	*	Candela compensation
	*/
	for (cnt = 1; cnt < EA8061V_TABLE_MAX; cnt++) {
		table_index = find_cadela_table(pSmart->brightness_level);

		if (table_index == -1) {
			table_index = CCG6_MAX_TABLE;
			pr_info("%s fail candela table_index cnt : %d brightness %d",
				__func__, cnt, pSmart->brightness_level);
		}

		bl_index[EA8061V_TABLE_MAX - cnt] +=
			gradation_offset_EA8061V_revD[table_index][cnt - 1];

		/* THERE IS M-GRAY0 target */
		if (bl_index[EA8061V_TABLE_MAX - cnt] == 0)
			bl_index[EA8061V_TABLE_MAX - cnt] = 1;
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
	for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++)
		(void)Make_hexa[cnt](bl_index , pSmart, str);

	/*
	*	RGB compensation
	*/
	for (cnt = 0; cnt < RGB_COMPENSATION; cnt++) {
		table_index = find_cadela_table(pSmart->brightness_level);

		if (table_index == -1) {
			table_index = CCG6_MAX_TABLE;
			pr_info("%s fail RGB table_index cnt : %d brightness %d",
				__func__, cnt, pSmart->brightness_level);
		}

		if (cnt < 3) {
			level_V255 = str[cnt * 2] << 8 | str[(cnt * 2) + 1];
			level_V255 +=
				rgb_offset_EA8061V_revD[table_index][cnt];
			level_255_temp_MSB = level_V255 / 256;

			str[cnt * 2] = level_255_temp_MSB & 0xff;
			str[(cnt * 2) + 1] = level_V255 & 0xff;
		} else {
			str[cnt+3] += rgb_offset_EA8061V_revD[table_index][cnt];
		}
	}
	/*subtration MTP_OFFSET value from generated gamma table*/
	mtp_offset_substraction(pSmart, str);
}

static int gradation_offset_EA8061V_revE[][9] = {
/*	V255 V203 V151 V87 V51 V35 V23 V11 V3 */
	{0, 7, 14, 25, 35, 39, 39, 39, 39},
	{0, 5, 12, 19, 29, 32, 35, 35, 35},
	{0, 5, 10, 17, 25, 28, 31, 31, 31},
	{0, 3, 7,  14, 21, 24, 28, 28, 28},
	{0, 3, 6,  12, 20, 22, 26, 26, 26},
	{0, 3, 6,  11, 18, 20, 23, 25, 25},
	{0, 3, 5,  10, 16, 18, 21, 23, 23},
	{0, 3, 4,  8, 14, 16, 20, 22, 22},
	{0, 3, 4,  7, 13, 15, 18, 21, 21},
	{0, 3, 4,  7,  12, 14, 17, 20, 20},
	{0, 3, 4,  6,  12, 14, 16, 20, 20},
	{0, 3, 3,  6,  11, 13, 16, 19, 19},
	{0, 3, 2,  6,  10, 12, 16, 18, 18},
	{0, 3, 2,  5,  10, 12, 15, 17, 17},
	{0, 3, 2,  4,  10, 12, 14, 16, 16},
	{0, 3, 2,  4,  9, 11, 14, 16, 16},
	{0, 3, 2,  3,  8, 9, 12, 15, 15},
	{0, 3, 2,  3,  7, 9, 12, 15, 15},
	{0, 3, 2,  3,  6, 8, 12, 14, 14},
	{0, 3, 2,  3,  6, 8, 11, 14, 14},
	{0, 3, 2,  3,  5, 7, 10, 13, 13},
	{0, 3, 2,  3,  5, 7, 10, 13, 13},
	{0, 3, 2,  3,  4, 6, 9, 12, 12},
	{0, 3, 2,  3,  4, 6, 9, 11, 11},
	{1, 3, 2,  3,  4, 6, 9, 11, 11},
	{1, 3, 2,  3,  4, 5, 8, 10, 10},
	{1, 3, 2,  3,  4, 5, 8, 10, 10},
	{0, 3, 2,  3,  4, 5, 7, 9, 9},
	{0, 3, 2,  3,  4, 5, 6, 8, 8},
	{0, 3, 2,  3,  4, 5, 6, 8, 8},
	{0, 3, 2,  3,  4, 4, 6, 8, 8},
	{1, 3, 2,  3,  3, 3, 6, 7, 7},
	{0, 3, 2,  3,  3, 3, 5, 7, 7},
	{0, 3, 2,  3,  3, 3, 5, 7, 7},
	{0, 3, 2,  3,  3, 3, 5, 6, 6},
	{0, 2, 2,  2,  2, 3, 5, 6, 6},
	{0, 2, 2,  2,  2, 3, 5, 5, 5},
	{0, 2, 2,  3,  2, 2, 4, 6, 6},
	{0, 2, 2,  3,  2, 2, 4, 6, 6},
	{0, 3, 3,  4,  2, 2, 4, 6, 6},
	{0, 3, 3,  4,  2, 2, 4, 6, 6},
	{0, 3, 3,  4,  2, 2, 4, 6, 6},
	{0, 4, 4,  5,  3, 2, 4, 6, 6},
	{0, 4, 4,  5,  3, 2, 4, 6, 6},
	{0, 4, 4,  4,  3, 2, 4, 6, 6},
	{0, 4, 4,  4,  3, 2, 4, 6, 6},
	{0, 3, 3,  3,  2, 2, 3, 5, 5},
	{0, 3, 3,  3,  2, 2, 3, 5, 5},
	{0, 3, 3,  3,  2, 2, 3, 5, 5},
	{0, 2, 2,  2,  2, 2, 2, 4, 4},
	{0, 2, 2,  2,  2, 2, 2, 4, 4},
	{0, 2, 2,  3,  2, 2, 2, 4, 4},
	{0, 2, 2,  3,  2, 2, 2, 3, 3},
	{0, 2, 2,  2,  2, 2, 1, 3, 3},
	{0, 2, 2,  2,  2, 2, 1, 2, 2},
	{0, 1, 2,  2,  2, 2, 1, 1, 1},
	{0, 1, 1,  2,  2, 2, 1, 1, 1},
	{0, 1, 1,  1,  1, 1, 1, 1, 1},
	{3, 3, 4,  3,  2, 2, 1, 1, 1},
	{3, 3, 4,  3,  1, 1, 1, 1, 1},
	{2, 2, 3,  2,  1, 1, 0, 0, 0},
	{1, 1, 2,  2,  1, 1, 0, 0, 0},
	{1, 1, 2,  1,  0, 0, 0, 0, 0},
	{0, 0, 2,  1,  0, 0, 0, 0, 0},
	{0, 0, 0,  0,  0, 0, 0, 0, 0},
};
static int rgb_offset_EA8061V_revE[][RGB_COMPENSATION] = {
/*	R255 G255 B255 R203 G203 B203 R151 G151 B151
	R87 G87 B87 R51 G51 B51 R35 G35 B35
	R23 G23 B23 R11 G11 B11
*/
	{-5,3,-10,-4,2,-5,-5,2,-5,-11,5,-12,-8,4,-12,0,4,-7,0,2,-3,0,0,0},
	{-4,2,-8,-4,2,-4,-4,2,-4,-10,4,-10,-8,4,-12,0,4,-7,0,2,-4,0,0,0},
	{-4,2,-7,-3,1,-4,-4,1,-4,-9,3,-9,-7,4,-12,-1,4,-7,0,3,-4,0,0,0},
	{-3,0,-5,-2,1,-4,-3,1,-3,-9,3,-9,-8,4,-12,-2,4,-7,0,4,-5,0,0,0},
	{-2,0,-4,-2,1,-4,-2,1,-2,-8,2,-8,-8,4,-12,-3,4,-7,0,4,-5,0,0,0},
	{-2,0,-4,-1,1,-3,-2,1,-2,-8,2,-8,-7,3,-11,-3,4,-7,-1,3,-5,0,1,-1},
	{-2,0,-3,-1,1,-3,-2,1,-2,-8,2,-8,-7,3,-11,-3,4,-7,-1,3,-5,0,1,-2},
	{-2,0,-2,-1,1,-2,-2,1,-2,-8,2,-8,-7,3,-10,-3,4,-7,-1,3,-5,0,1,-2},
	{-1,0,-2,-1,1,-1,-2,1,-2,-7,2,-7,-7,3,-10,-3,4,-7,-1,3,-5,0,1,-3},
	{-1,0,-2,-1,1,-2,-2,1,-2,-7,2,-7,-7,3,-10,-3,4,-7,-1,3,-5,0,1,-4},
	{0,0,-2,-1,0,-2,-2,0,-2,-7,2,-6,-6,2,-10,-4,4,-7,-3,2,-6,0,2,-4},
	{0,0,-1,-1,0,-2,-1,0,-1,-7,2,-6,-6,2,-9,-4,4,-7,-3,2,-6,0,2,-5},
	{0,0,-1,-1,0,-2,-1,0,-1,-7,2,-6,-6,2,-8,-4,4,-7,-3,2,-6,0,2,-6},
	{0,0,-1,-1,0,-2,-1,0,-1,-6,2,-5,-6,2,-8,-4,4,-7,-3,2,-6,0,2,-6},
	{0,0,-1,-1,0,-2,-1,0,-1,-5,2,-5,-6,2,-8,-4,4,-7,-3,2,-6,0,2,-7},
	{0,0,-1,-1,0,-2,-1,0,-1,-4,2,-5,-6,2,-7,-5,3,-7,-3,2,-6,0,2,-8},
	{0,0,0,-1,0,-2,0,0,0,-3,2,-7,-6,2,-7,-6,3,-7,-3,2,-6,0,2,-9},
	{0,0,0,-1,0,-2,0,0,0,-3,2,-7,-6,2,-7,-6,3,-7,-3,2,-6,0,2,-10},
	{0,0,0,-1,0,-2,0,0,0,-3,2,-7,-6,2,-7,-6,3,-7,-3,2,-6,0,2,-10},
	{0,0,0,-1,0,-2,0,0,0,-3,2,-7,-6,2,-7,-6,3,-7,-3,2,-6,0,2,-10},
	{0,0,1,-1,0,-2,0,0,0,-3,2,-7,-6,2,-7,-6,3,-7,-3,2,-6,0,2,-10},
	{0,0,0,-1,0,-2,0,0,0,-2,1,-7,-6,2,-7,-6,3,-7,-3,2,-6,0,2,-10},
	{0,0,0,-1,0,-2,0,0,0,-2,1,-7,-5,1,-7,-6,3,-6,-3,2,-6,0,2,-10},
	{0,0,0,-1,0,-2,0,0,0,-2,1,-7,-5,1,-7,-6,3,-6,-3,2,-6,0,2,-10},
	{0,-1,0,-1,0,-2,0,0,0,-2,1,-7,-5,1,-7,-6,3,-6,-3,2,-6,0,2,-10},
	{0,0,0,-1,0,-2,0,0,0,-2,1,-7,-5,1,-7,-6,2,-6,-3,2,-6,0,2,-10},
	{0,0,0,-1,0,-2,0,0,0,-2,1,-7,-5,1,-7,-6,2,-6,-3,2,-6,0,2,-10},
	{0,0,0,-1,0,-2,0,0,0,-2,1,-7,-5,1,-7,-6,2,-6,-3,2,-6,0,2,-10},
	{0,0,0,-1,0,-2,0,0,0,-2,1,-1,-5,1,-6,-6,2,-6,-3,2,-6,0,2,-10},
	{0,0,0,-1,0,-2,0,0,0,-2,1,-1,-5,1,-6,-6,2,-5,-3,2,-6,0,2,-11},
	{0,0,1,-1,0,-2,0,0,0,-2,1,-1,-4,1,-6,-6,2,-4,-3,2,-6,0,2,-11},
	{0,0,0,-1,0,-2,0,0,0,-2,1,-1,-4,1,-5,-6,2,-4,-4,2,-5,0,2,-11},
	{0,0,0,-1,0,-2,0,0,0,-2,1,-1,-3,1,-5,-6,2,-3,-4,2,-5,0,2,-11},
	{0,0,0,-1,0,-2,0,0,0,-2,1,-1,-2,1,-5,-6,2,-2,-4,2,-5,0,2,-11},
	{0,0,0,-1,0,-2,0,0,0,-2,1,-1,-2,1,-5,-6,2,-2,-4,2,-5,0,2,-12},
	{0,-2,0,0,0,-2,0,0,1,-2,0,-1,-2,1,-4,-6,2,-2,-4,2,-4,0,2,-12},
	{0,0,0,0,0,-1,-1,0,0,-2,0,-1,-1,1,-4,-5,1,-1,-5,1,-4,0,3,-12},
	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-4,-3,2,-1,-3,1,-3,-1,3,-9},
	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-4,-3,2,-1,-3,1,-3,-1,3,-9},
	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-4,-3,1,-2,-4,1,-3,-2,2,-8},
	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-4,-3,1,-2,-4,1,-3,-2,2,-8},
	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-4,-3,1,-3,-4,1,-3,-2,2,-8},
	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-3,-3,0,-4,-5,0,-3,-3,2,-8},
	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-3,-3,0,-4,-5,0,-3,-3,2,-8},
	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-3,-3,0,-4,-2,0,-3,-3,2,-8},
	{-1,0,0,0,0,-1,0,0,0,0,0,0,-2,0,-3,-3,0,-4,-5,0,-3,-3,2,-8},
	{-1,0,0,0,0,0,0,0,-1,0,0,0,-2,0,-2,-3,0,-3,-4,0,-4,-4,1,-8},
	{-1,0,0,0,0,0,0,0,-1,0,0,0,-2,0,-2,-3,0,-3,-4,0,-4,-4,1,-8},
	{-1,0,0,0,0,0,0,0,-1,0,0,0,-2,0,-2,-3,0,-3,-4,0,-4,-4,1,-8},
	{-1,0,0,0,0,0,0,0,-1,0,0,0,-2,0,-2,-3,0,-3,-4,0,-4,-4,0,-8},
	{-1,0,0,0,0,0,0,0,-1,0,0,0,-2,0,-2,-3,0,-3,-3,0,-4,-5,0,-8},
	{0,1,-1,0,0,0,0,0,0,0,0,0,-2,0,-2,-3,0,-3,-3,0,-2,-5,0,-7},
	{0,1,-1,0,0,0,0,0,0,0,0,0,-2,0,-2,-3,0,-3,-3,0,-1,-4,0,-6},
	{0,0,-1,0,0,0,0,0,0,0,0,0,-2,0,-1,-3,0,-3,-3,0,0,-4,0,-4},
	{0,0,-1,0,0,0,0,0,0,0,0,0,-1,0,-1,-3,0,-2,-3,0,0,-4,0,-3},
	{0,0,-1,0,0,0,0,0,0,0,0,0,-1,0,-1,-3,0,-2,-3,0,0,-3,0,-3},
	{0,0,-1,0,0,0,0,0,0,0,0,0,-1,0,-1,-2,0,-1,-3,0,0,-2,0,-2},
	{0,0,-1,0,0,0,0,0,0,0,0,0,-1,0,-1,0,0,0,-2,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};
static void gamma_init_EA8061V_revE(struct SMART_DIM *pSmart, char *str, int size)
{
	long long candela_level[EA8061V_TABLE_MAX] = {-1, };
	int bl_index[EA8061V_TABLE_MAX] = {-1, };

	long long temp_cal_data = 0;
	int bl_level;

	int level_255_temp_MSB = 0;
	int level_V255 = 0;

	int point_index;
	int cnt;
	int table_index;

	pr_debug("%s : start !!\n",__func__);
	/*calculate candela level */
	if (pSmart->brightness_level == 350) {
		bl_level = pSmart->brightness_level;

	}else if (pSmart->brightness_level == 333)
		bl_level = 333;
	else if (pSmart->brightness_level == 316)
		bl_level = 316;
	else if (pSmart->brightness_level == 300)
		bl_level = 300;
	else if (pSmart->brightness_level == 282)
		bl_level = 282;
	else if (pSmart->brightness_level == 265)
		bl_level = 265;


	else if ((pSmart->brightness_level <= 249) &&
				(pSmart->brightness_level >= 162)) {
		bl_level = 249;
	} else if ((pSmart->brightness_level <= 152) &&
				(pSmart->brightness_level >= 68)) {

		if (pSmart->brightness_level == 68)
			bl_level = 116;
		else if (pSmart->brightness_level == 72)
			bl_level = 123;
		else if (pSmart->brightness_level == 77)
			bl_level = 131;
		else if (pSmart->brightness_level == 82)
			bl_level = 139;
		else if (pSmart->brightness_level == 87)
			bl_level = 147;
		else if (pSmart->brightness_level == 93)
			bl_level = 157;
		else if (pSmart->brightness_level == 98)
			bl_level = 165;
		else if (pSmart->brightness_level == 105)
			bl_level = 176;
		else if (pSmart->brightness_level == 111)
			bl_level = 185;
		else if (pSmart->brightness_level == 119)
			bl_level = 197;
		else if (pSmart->brightness_level == 126)
			bl_level = 207;
		else if (pSmart->brightness_level == 134)
			bl_level = 219;
		else if (pSmart->brightness_level == 143)
			bl_level = 232;
		else
			bl_level = 245;

	} else {
		bl_level = 110;
	}

	if (pSmart->brightness_level < 350) {
		for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
			point_index = EA8061V_ARRAY[cnt+1];
			temp_cal_data =
			((long long)(candela_coeff_2p15[point_index])) *
			((long long)(bl_level));
			candela_level[cnt] = temp_cal_data;
		}
	} else {
		for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
			point_index = EA8061V_ARRAY[cnt+1];
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

	for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
		if (searching_function(candela_level[cnt],
			&(bl_index[cnt]), GAMMA_CURVE_2P2)) {
			pr_info("%s searching functioin error cnt:%d\n",
			__func__, cnt);
		}
	}

	/*
	*	Candela compensation
	*/
	for (cnt = 1; cnt < EA8061V_TABLE_MAX; cnt++) {
		table_index = find_cadela_table(pSmart->brightness_level);

		if (table_index == -1) {
			table_index = CCG6_MAX_TABLE;
			pr_info("%s fail candela table_index cnt : %d brightness %d",
				__func__, cnt, pSmart->brightness_level);
		}

		bl_index[EA8061V_TABLE_MAX - cnt] +=
			gradation_offset_EA8061V_revE[table_index][cnt - 1];

		/* THERE IS M-GRAY0 target */
		if (bl_index[EA8061V_TABLE_MAX - cnt] == 0)
			bl_index[EA8061V_TABLE_MAX - cnt] = 1;
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
	for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++)
		(void)Make_hexa[cnt](bl_index , pSmart, str);

	/*
	*	RGB compensation
	*/
	for (cnt = 0; cnt < RGB_COMPENSATION; cnt++) {
		table_index = find_cadela_table(pSmart->brightness_level);

		if (table_index == -1) {
			table_index = CCG6_MAX_TABLE;
			pr_info("%s fail RGB table_index cnt : %d brightness %d",
				__func__, cnt, pSmart->brightness_level);
		}

		if (cnt < 3) {
			level_V255 = str[cnt * 2] << 8 | str[(cnt * 2) + 1];
			level_V255 +=
				rgb_offset_EA8061V_revE[table_index][cnt];
			level_255_temp_MSB = level_V255 / 256;

			str[cnt * 2] = level_255_temp_MSB & 0xff;
			str[(cnt * 2) + 1] = level_V255 & 0xff;
		} else {
			str[cnt+3] += rgb_offset_EA8061V_revE[table_index][cnt];
		}
	}
	/*subtration MTP_OFFSET value from generated gamma table*/
	mtp_offset_substraction(pSmart, str);
}
#endif

static void pure_gamma_init(struct SMART_DIM *pSmart, char *str, int size)
{
	long long candela_level[EA8061V_TABLE_MAX] = {-1, };
	int bl_index[EA8061V_TABLE_MAX] = {-1, };

	long long temp_cal_data = 0;
	int bl_level, cnt;
	int point_index;

	bl_level = pSmart->brightness_level;

	for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
			point_index = EA8061V_ARRAY[cnt+1];
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
	for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++) {
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
	for (cnt = 0; cnt < EA8061V_TABLE_MAX; cnt++)
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
static void gamma_cell_determine(int ldi_revision)
{
	pr_info("%s ldi_revision: 0x%x", __func__, ldi_revision);

	V255_300CD_R_MSB = V255_300CD_R_MSB_20;
	V255_300CD_R_LSB = V255_300CD_R_LSB_20;

	V255_300CD_G_MSB = V255_300CD_G_MSB_20;
	V255_300CD_G_LSB = V255_300CD_G_LSB_20;

	V255_300CD_B_MSB = V255_300CD_B_MSB_20;
	V255_300CD_B_LSB = V255_300CD_B_LSB_20;

	V203_300CD_R = V203_300CD_R_20;
	V203_300CD_G = V203_300CD_G_20;
	V203_300CD_B = V203_300CD_B_20;

	V151_300CD_R = V151_300CD_R_20;
	V151_300CD_G = V151_300CD_G_20;
	V151_300CD_B = V151_300CD_B_20;

	V87_300CD_R = V87_300CD_R_20;
	V87_300CD_G = V87_300CD_G_20;
	V87_300CD_B = V87_300CD_B_20;

	V51_300CD_R = V51_300CD_R_20;
	V51_300CD_G = V51_300CD_G_20;
	V51_300CD_B = V51_300CD_B_20;

	V35_300CD_R = V35_300CD_R_20;
	V35_300CD_G = V35_300CD_G_20;
	V35_300CD_B = V35_300CD_B_20;

	V23_300CD_R = V23_300CD_R_20;
	V23_300CD_G = V23_300CD_G_20;
	V23_300CD_B = V23_300CD_B_20;

	V11_300CD_R = V11_300CD_R_20;
	V11_300CD_G = V11_300CD_G_20;
	V11_300CD_B = V11_300CD_B_20;

	V3_300CD_R = V3_300CD_R_20;
	V3_300CD_G = V3_300CD_G_20;
	V3_300CD_B = V3_300CD_B_20;

	VT_300CD_R = VT_300CD_R_20;
	VT_300CD_G = VT_300CD_G_20;
	VT_300CD_B = VT_300CD_B_20;
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

	int id3;//id1,id2;

#ifdef SMART_DIMMING_DEBUG
	int cnt;
	char pBuffer[256];
	memset(pBuffer, 0x00, 256);
#endif

	/*id1 = (psmart->ldi_revision & 0x00FF0000) >> 16;
	id2 = (psmart->ldi_revision & 0x0000FF00) >> 8;*/
	id3 = psmart->ldi_revision & 0xFF;

	pr_debug("%s : ++\n",__func__);

	mtp_sorting(psmart);
	gamma_cell_determine(psmart->ldi_revision);
	set_max_lux_table();

#ifdef SMART_DIMMING_DEBUG
	print_RGB_offset(psmart);
#endif

	if(id3 == EVT2_EA8061V_REV_E || id3 == EVT2_EA8061V_REV_D || EVT2_EA8061V_REV_C)
		psmart->vregout_voltage = EA8061V_VREG0_REF_6P0;
	else
		psmart->vregout_voltage = EA8061V_VREG0_REF_6P3;

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
	if(id3 == EVT2_EA8061V_REV_E)
		gamma_init_EA8061V_revE(psmart,
			(char *)(&(psmart->gen_table[lux_loop].gamma_setting)),
			GAMMA_SET_MAX);
	else if(id3 == EVT2_EA8061V_REV_D)
		gamma_init_EA8061V_revD(psmart,
			(char *)(&(psmart->gen_table[lux_loop].gamma_setting)),
			GAMMA_SET_MAX);
	else if(id3 == EVT2_EA8061V_REV_C)
		gamma_init_EA8061V_revC(psmart,
			(char *)(&(psmart->gen_table[lux_loop].gamma_setting)),
			GAMMA_SET_MAX);
	else
		gamma_init_EA8061V_revA(psmart,
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
static void wrap_generate_gamma(int cd, char *cmd_str) {
	smart_EA8061V.brightness_level = cd;
	generate_gamma(&smart_EA8061V, cmd_str, GAMMA_SET_MAX);
}

static void wrap_smart_dimming_init(void) {
	smart_EA8061V.plux_table = __EA8061V__.lux_tab;
	smart_EA8061V.lux_table_max = __EA8061V__.lux_tabsize;
	smart_EA8061V.ldi_revision = __EA8061V__.man_id;
	smart_dimming_init(&smart_EA8061V);
}

struct smartdim_conf *smart_S6E8FA0_get_conf(void) {
	__EA8061V__.generate_gamma = wrap_generate_gamma;
	__EA8061V__.init = wrap_smart_dimming_init;
	__EA8061V__.get_min_lux_table = get_min_lux_table;
	__EA8061V__.mtp_buffer = (char *)(&smart_EA8061V.MTP_ORIGN);
	__EA8061V__.print_aid_log = print_aid_log;
	return (struct smartdim_conf *)&__EA8061V__;
}

/* TEMP for build error */
struct smartdim_conf *smart_S6E3FA0_get_conf(void) {
	__EA8061V__.generate_gamma = wrap_generate_gamma;
	__EA8061V__.init = wrap_smart_dimming_init;
	__EA8061V__.get_min_lux_table = get_min_lux_table;
	__EA8061V__.mtp_buffer = (char *)(&smart_EA8061V.MTP_ORIGN);
	__EA8061V__.print_aid_log = print_aid_log;
	return (struct smartdim_conf *)&__EA8061V__;
}

/* ----------------------------------------------------------------------------
 * END - Wrapper
 * ----------------------------------------------------------------------------
 */
