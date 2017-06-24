/******************************************************************************
* (c) COPYRIGHT 2013 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE		: MTV23x RF services source file.
*
* FILENAME	: mtv23x_rf.c
*
* DESCRIPTION	:
*		Library of routines to initialize, and operate on the RAONTECH T-DMB demod.
*
******************************************************************************/
/******************************************************************************
* REVISION HISTORY
*
*    DATE         NAME          REMARKS
* ----------  -------------    ------------------------------------------------
* 08/01/2013  Yang, Maverick       Revised for REV3
* 03/03/2013  Yang, Maverick       Created.
******************************************************************************/

#include "mtv23x_rf.h"
#include "mtv23x_rf_adc_data.h"
#include "mtv23x_internal.h"

/*
Down conversion Signal Monitoring.
*/
/* #define DEBUG_A_TEST_ZERO */

static const struct RTV_REG_INIT_INFO t_mtv23x_INIT[] = {
	{0x25, 0xF8},
	{0x26, 0x00},
	{0x28, 0xDD},
	{0x29, 0xC4},
	{0x2C, 0x1D},
	{0x2D, 0x90},
	{0x2F, 0x06},
	{0x30, 0xDF},
	{0x33, 0x11},
	{0x36, 0x09},
	{0x38, 0xF0},
	{0x39, 0x00},
	{0x3A, 0xAA},
	{0x3E, 0x2D},
	{0x47, 0x59},
	{0x48, 0x28},
	{0x49, 0x41},
	{0x4A, 0x70},
	{0x4B, 0x65},
	{0x4E, 0x4B},
	{0x50, 0x6F},
	{0x51, 0x3C},
	{0x53, 0x65},
	{0x54, 0xC0},
	{0x5D, 0x01},
	{0x5E, 0x70},
	{0x5F, 0x75},
	{0x60, 0x62},
	{0x61, 0x80},
	{0x69, 0x0E},
	{0x6A, 0x14},
	{0x6B, 0x18},
	{0x6C, 0xFF},
	{0x6D, 0xFD},
	{0x6E, 0x19},
	{0x70, 0x80},
	{0x71, 0x6E},
	{0x74, 0x15},
	{0x75, 0xA4},
	{0x77, 0x69},
	{0x78, 0x3D},
	{0x7D, 0x28},
	{0x81, 0x9C},
	{0x83, 0x9F},
	{0x85, 0x40},
	{0x86, 0x87},
	{0x87, 0x84},
	{0x88, 0x22},
	{0x89, 0x20},
	{0x8A, 0xF6},
	{0x8B, 0xB5},
	{0x8C, 0xFC},
	{0x8D, 0xFF},
	{0x8E, 0xFE},
	{0x8F, 0xFD},
	{0x90, 0xFD},
	{0x91, 0xFC},
	{0x92, 0x0E},
	{0x93, 0x0D},
	{0x94, 0x09},
	{0x95, 0xA3},
	{0x96, 0xF0},
	{0x97, 0x19},
	{0x99, 0x42},
	{0x9A, 0x6C},
	{0x9B, 0x10},
	{0x9C, 0x8E},
	{0x9D, 0x3C},
	{0x9E, 0x30},
	{0x9F, 0x63},
	{0xA1, 0x40},
	{0xA2, 0x5C},
	{0xA3, 0x1C},
	{0xA4, 0x85},
	{0xA5, 0xB4},
	{0xA6, 0x30},
	{0xA7, 0x00},
	{0xA9, 0x00},
	{0xAA, 0x04},
	{0xAB, 0x30},
	{0xAC, 0x00},
	{0xAD, 0x14},
	{0xAE, 0x30},
	{0xAF, 0x00},
	{0xB1, 0x00},
	{0xB2, 0x04},
	{0xB3, 0x30},
	{0xB4, 0x00},
	{0xB5, 0xB1},
	{0xB7, 0x05},
	{0xBC, 0x1F},
	{0xBD, 0x1F},
	{0xBE, 0x5F},
	{0xBF, 0x1F},
	{0xC0, 0x1F},
	{0xC1, 0x5F},
	{0xC2, 0x1F},
	{0xC3, 0x1F},
	{0xC4, 0x5F},
	{0xC6, 0x4A},
	{0xC7, 0x4A},
	{0xCA, 0xCA},
	{0xCB, 0x4A},
	{0xCC, 0x4F},
	{0xCF, 0x80},
	{0xD0, 0x20},
	{0xD4, 0x1F},
	{0xD7, 0x80},
	{0xD8, 0x00},
	{0xDA, 0xA4},
	{0xDF, 0x01},
	{0xE2, 0x24},
	{0xE5, 0xA8},
	{0xE6, 0xA6},
	{0xE7, 0x64}
};

static enum E_RTV_BANDWIDTH_TYPE g_aeBwType;
UINT g_dwRtvPrevChFreqKHz;
static U32 g_nLnaTuneVal;

#ifdef RTV_DUAL_DIVERISTY_ENABLE
enum E_RTV_BANDWIDTH_TYPE g_aeBwType_slave;
U32 g_nLnaTuneVal_slave;
#endif

static int rtvRF_ConfigureClkCKSYN(enum E_RTV_BANDWIDTH_TYPE eBwType)
{
	U8 WR6D = 0, WR6E = 0, WR70 = 0, WR71 = 0;
	U8 WR2A = 0, WR72 = 0, WR73 = 0, WR74 = 0, WR75 = 0;

	WR6D = RTV_REG_GET(0x6d) & 0xFC;
	WR6E = RTV_REG_GET(0x6e) & 0x00;
	WR70 = RTV_REG_GET(0x70) & 0x00;
	WR2A = RTV_REG_GET(0x2A) & 0xF7;
	WR71 = RTV_REG_GET(0x71) & 0x00;
	WR72 = RTV_REG_GET(0x72) & 0x03;
	WR73 = RTV_REG_GET(0x73) & 0x03;
	WR74 = RTV_REG_GET(0x74) & 0x0F;
	WR75 = RTV_REG_GET(0x75) & 0x03;

	RTV_REG_SET(0x6D, WR6D | (U8)g_atBW_TABLE_CKSYN[eBwType][2]);
	RTV_REG_SET(0x6E, WR6E | (U8)g_atBW_TABLE_CKSYN[eBwType][0]);
	RTV_REG_SET(0x70, WR70 | (U8)(g_atBW_TABLE_CKSYN[eBwType][1] & 0xFF));
	RTV_REG_SET(0x2A, WR2A | (U8)(g_atBW_TABLE_CKSYN[eBwType][3] << 3));
	RTV_REG_SET(0x71, WR71 | (U8)(((g_atBW_TABLE_CKSYN[eBwType][1] & 0x300) >> 2)
		| g_atBW_TABLE_CKSYN[eBwType][4]));
	RTV_REG_SET(0x72, WR72 | (U8)(g_atBW_TABLE_CKSYN[eBwType][6] << 2));
	RTV_REG_SET(0x73, WR73 | (U8)(g_atBW_TABLE_CKSYN[eBwType][5] << 2));
	RTV_REG_SET(0x74, WR74 | (U8)(g_atBW_TABLE_CKSYN[eBwType][7] << 4));
	RTV_REG_SET(0x75, WR75 | (U8)(g_atBW_TABLE_CKSYN[eBwType][8] << 2));

	if (rtvRF_LockCheck(1) != RTV_SUCCESS)
		return RTV_ADC_CLK_UNLOCKED;

	return RTV_SUCCESS;
}

static int rtvRF_ConfigureIIRFilter(enum E_RTV_BANDWIDTH_TYPE eBwType)
{
	U8 WR95 = 0;

	WR95 = RTV_REG_GET(0x95) & 0xC0;

	RTV_REG_SET(0x95, (WR95 | (U8)((g_atBW_TABLE_IIR[eBwType][0]<<4) | ((g_atBW_TABLE_IIR[eBwType][1]&0xF0000)>>16))));
	RTV_REG_SET(0x96, ((g_atBW_TABLE_IIR[eBwType][1] & 0x0FF00)>>8));
	RTV_REG_SET(0x97, ((g_atBW_TABLE_IIR[eBwType][1] & 0x000FF)>>0));
	RTV_REG_SET(0x98, ((g_atBW_TABLE_IIR[eBwType][2] & 0xFF000)>>12));
	RTV_REG_SET(0x99, ((g_atBW_TABLE_IIR[eBwType][2] & 0x00FF0)>>4));
	RTV_REG_SET(0x9A, (U8)((((g_atBW_TABLE_IIR[eBwType][2] & 0x0000F)>>0) << 4) |
			((g_atBW_TABLE_IIR[eBwType][3] & 0xF0000)>>16)));
	RTV_REG_SET(0x9B, (U8)((g_atBW_TABLE_IIR[eBwType][3] & 0x0FF00)>>8));
	RTV_REG_SET(0x9C, (U8)((g_atBW_TABLE_IIR[eBwType][3] & 0x000FF)>>0));
	RTV_REG_SET(0x9D, (U8)((((g_atBW_TABLE_IIR[eBwType][13] & 0xF0000)>>16) << 4) |
			(U8)((g_atBW_TABLE_IIR[eBwType][4] & 0xF0000)>>16)));
	RTV_REG_SET(0x9E, (U8)((g_atBW_TABLE_IIR[eBwType][4] & 0x0FF00)>>8));
	RTV_REG_SET(0x9F, (U8)((g_atBW_TABLE_IIR[eBwType][4] & 0x000FF)>>0));
	RTV_REG_SET(0xA0, (U8)((g_atBW_TABLE_IIR[eBwType][5] & 0xFF000)>>12));
	RTV_REG_SET(0xA1, (U8)((g_atBW_TABLE_IIR[eBwType][5] & 0x00FF0)>>4));
	RTV_REG_SET(0xA2, (U8)((((g_atBW_TABLE_IIR[eBwType][5] & 0x0000F)>>0) << 4) |
			((g_atBW_TABLE_IIR[eBwType][6] & 0xF0000)>>16)));
	RTV_REG_SET(0xA3, (U8)((g_atBW_TABLE_IIR[eBwType][6] & 0x0FF00)>>8));
	RTV_REG_SET(0xA4, (U8)((g_atBW_TABLE_IIR[eBwType][6] & 0x000FF)>>0));
	RTV_REG_SET(0xA5, (U8)((((g_atBW_TABLE_IIR[eBwType][13] & 0x0F000)>>12) << 4) |
			(U8)((g_atBW_TABLE_IIR[eBwType][7] & 0xF0000)>>16)));
	RTV_REG_SET(0xA6, (U8)((g_atBW_TABLE_IIR[eBwType][7] & 0x0FF00)>>8));
	RTV_REG_SET(0xA7, (U8)((g_atBW_TABLE_IIR[eBwType][7] & 0x000FF)>>0));
	RTV_REG_SET(0xA8, (U8)((g_atBW_TABLE_IIR[eBwType][8] & 0xFF000)>>12));
	RTV_REG_SET(0xA9, (U8)((g_atBW_TABLE_IIR[eBwType][8] & 0x00FF0)>>4));
	RTV_REG_SET(0xAA, (U8)((((g_atBW_TABLE_IIR[eBwType][8] & 0x0000F)>>0) << 4) |
			((g_atBW_TABLE_IIR[eBwType][9] & 0xF0000)>>16)));
	RTV_REG_SET(0xAB, (U8)((g_atBW_TABLE_IIR[eBwType][9] & 0x0FF00)>>8));
	RTV_REG_SET(0xAC, (U8)((g_atBW_TABLE_IIR[eBwType][9] & 0x000FF)>>0));
	RTV_REG_SET(0xAD, (U8)((((g_atBW_TABLE_IIR[eBwType][13] & 0x00F00)>>8) << 4) |
			(U8)((g_atBW_TABLE_IIR[eBwType][10] & 0xF0000)>>16)));
	RTV_REG_SET(0xAE, (U8)((g_atBW_TABLE_IIR[eBwType][10] & 0x0FF00)>>8));
	RTV_REG_SET(0xAF, (U8)((g_atBW_TABLE_IIR[eBwType][10] & 0x000FF)>>0));
	RTV_REG_SET(0xB0, (U8)((g_atBW_TABLE_IIR[eBwType][11] & 0xFF000)>>12));
	RTV_REG_SET(0xB1, (U8)((g_atBW_TABLE_IIR[eBwType][11] & 0x00FF0)>>4));
	RTV_REG_SET(0xB2, (U8)((((g_atBW_TABLE_IIR[eBwType][11] & 0x0000F)>>0) << 4) |
			((g_atBW_TABLE_IIR[eBwType][12] & 0xF0000)>>16)));
	RTV_REG_SET(0xB3, (U8)((g_atBW_TABLE_IIR[eBwType][12] & 0x0FF00)>>8));
	RTV_REG_SET(0xB4, (U8)((g_atBW_TABLE_IIR[eBwType][12] & 0x000FF)>>0));
	RTV_REG_SET(0xB5, (U8)((g_atBW_TABLE_IIR[eBwType][13] & 0x000FF)>>0));

	return RTV_SUCCESS;
}

static int rtvRF_ConfigureBBA(enum E_RTV_BANDWIDTH_TYPE eBwType)
{
	U8 WR3E = 0, WR3F = 0, WR50 = 0, WR51 = 0, WR4F = 0, WR4E = 0, WR77 = 0;

	WR3E = RTV_REG_GET(0x3E) & 0x00;
	WR3F = RTV_REG_GET(0x3F) & 0x03;
	WR50 = RTV_REG_GET(0x50) & 0x1F;
	WR51 = RTV_REG_GET(0x51) & 0x8F;
	WR4F = RTV_REG_GET(0x4F) & 0x1F;
	WR4E = RTV_REG_GET(0x4E) & 0x1F;
	WR77 = RTV_REG_GET(0x77) & 0xFC;

	RTV_REG_SET(0x3E, WR3E | g_atBW_TABLE_BBA[eBwType][0]);
	RTV_REG_SET(0x3F, WR3F | (g_atBW_TABLE_BBA[eBwType][1] << 2));
	RTV_REG_SET(0x50, WR50 | (g_atBW_TABLE_BBA[eBwType][2] << 5));
	RTV_REG_SET(0x51, WR51 | (g_atBW_TABLE_BBA[eBwType][3] << 4));
	RTV_REG_SET(0x4F, WR4F | (g_atBW_TABLE_BBA[eBwType][4] << 5));
	RTV_REG_SET(0x4E, WR4E | (g_atBW_TABLE_BBA[eBwType][5] << 5));
	RTV_REG_SET(0x77, WR77 | (g_atBW_TABLE_BBA[eBwType][6] << 0));

	return RTV_SUCCESS;
}

static int rtvRF_ConfigureADC(enum E_RTV_BANDWIDTH_TYPE eBwType)
{
	U8 WRB7 = 0, WRC8 = 0, WRC9 = 0, WRCA = 0, WRCB = 0, WRCC = 0;
	U8 WRCD = 0, WRCE = 0;
	U8 WRD1 = 0, WRD2 = 0, WRD3 = 0, WRD5 = 0, WRD6 = 0, WRD7 = 0;
	U8 WRD8 = 0, WRD9 = 0, WRDA = 0;

	WRB7 = RTV_REG_GET(0xB7) & 0x07;
	WRC8 = RTV_REG_GET(0xC8) & 0x80;
	WRC9 = RTV_REG_GET(0xC9) & 0x80;
	WRCA = RTV_REG_GET(0xCA) & 0x80;
	WRCB = RTV_REG_GET(0xCB) & 0x80;
	WRCC = RTV_REG_GET(0xCC) & 0x80;
	WRCD = RTV_REG_GET(0xCD) & 0x80;
	WRCE = RTV_REG_GET(0xCE) & 0x80;
	WRD1 = RTV_REG_GET(0xD1) & 0x80;
	WRD2 = RTV_REG_GET(0xD2) & 0x80;
	WRD3 = RTV_REG_GET(0xD3) & 0x80;
	WRD5 = RTV_REG_GET(0xD5) & 0x80;
	WRD6 = RTV_REG_GET(0xD6) & 0x80;
	WRD7 = RTV_REG_GET(0xD7) & 0x80;
	WRD8 = RTV_REG_GET(0xD8) & 0x80;
	WRD9 = RTV_REG_GET(0xD9) & 0xC0;
	WRDA = RTV_REG_GET(0xDA) & 0xC0;

	RTV_REG_SET(0xB7, (WRB7 | (g_atBW_TABLE_ADC[eBwType][45] << 6) |
				(g_atBW_TABLE_ADC[eBwType][46] << 5)));
	RTV_REG_SET(0xB8, (g_atBW_TABLE_ADC[eBwType][44]));
	RTV_REG_SET(0xB9, (g_atBW_TABLE_ADC[eBwType][43]));
	RTV_REG_SET(0xBA, (g_atBW_TABLE_ADC[eBwType][42]));
	RTV_REG_SET(0xBB, (g_atBW_TABLE_ADC[eBwType][41]));
	RTV_REG_SET(0xBC, ((g_atBW_TABLE_ADC[eBwType][37] & 0x03) << 6) |
				g_atBW_TABLE_ADC[eBwType][40]);
	RTV_REG_SET(0xBD, (((g_atBW_TABLE_ADC[eBwType][37] & 0x0C) >> 2) << 6) |
				g_atBW_TABLE_ADC[eBwType][39]);
	RTV_REG_SET(0xBE, (((g_atBW_TABLE_ADC[eBwType][37] & 0x30) >> 4) << 6) |
				g_atBW_TABLE_ADC[eBwType][38]);
	RTV_REG_SET(0xBF, ((g_atBW_TABLE_ADC[eBwType][33] & 0x03) << 6) |
				(g_atBW_TABLE_ADC[eBwType][36]));
	RTV_REG_SET(0xC0, (((g_atBW_TABLE_ADC[eBwType][33] & 0x0C) >> 2) << 6) |
				(g_atBW_TABLE_ADC[eBwType][35]));
	RTV_REG_SET(0xC1, (((g_atBW_TABLE_ADC[eBwType][33] & 0x30) >> 4) << 6) |
				(g_atBW_TABLE_ADC[eBwType][34]));
	RTV_REG_SET(0xC2, ((g_atBW_TABLE_ADC[eBwType][29] & 0x03) << 6) |
				(g_atBW_TABLE_ADC[eBwType][32]));
	RTV_REG_SET(0xC3, (((g_atBW_TABLE_ADC[eBwType][29] & 0x0C) >> 2) << 6) |
				(g_atBW_TABLE_ADC[eBwType][31]));
	RTV_REG_SET(0xC4, (((g_atBW_TABLE_ADC[eBwType][29] & 0x30) >> 4) << 6) |
				(g_atBW_TABLE_ADC[eBwType][30]));
	RTV_REG_SET(0xC5, (g_atBW_TABLE_ADC[eBwType][28]));
	RTV_REG_SET(0xC6, (g_atBW_TABLE_ADC[eBwType][27]));
	RTV_REG_SET(0xC7, (g_atBW_TABLE_ADC[eBwType][26]));
	RTV_REG_SET(0xC8, (WRC8 | (g_atBW_TABLE_ADC[eBwType][25])));
	RTV_REG_SET(0xC9, (WRC9 | (g_atBW_TABLE_ADC[eBwType][24])));
	RTV_REG_SET(0xCA, (WRCA | (g_atBW_TABLE_ADC[eBwType][23])));
	RTV_REG_SET(0xCB, (WRCB | (g_atBW_TABLE_ADC[eBwType][22])));
	RTV_REG_SET(0xCC, (WRCC | (g_atBW_TABLE_ADC[eBwType][21])));
	RTV_REG_SET(0xCD, (WRCD | (g_atBW_TABLE_ADC[eBwType][20])));
	RTV_REG_SET(0xCE, (WRCE | (g_atBW_TABLE_ADC[eBwType][19])));
	RTV_REG_SET(0xCF, (g_atBW_TABLE_ADC[eBwType][18] << 6) |
				(g_atBW_TABLE_ADC[eBwType][17] << 4) |
				(g_atBW_TABLE_ADC[eBwType][16] << 2) |
				(g_atBW_TABLE_ADC[eBwType][15] << 0));
	RTV_REG_SET(0xD0, (g_atBW_TABLE_ADC[eBwType][14]));
	RTV_REG_SET(0xD1, (WRD1 | (g_atBW_TABLE_ADC[eBwType][13])));
	RTV_REG_SET(0xD2, (WRD2 | (g_atBW_TABLE_ADC[eBwType][12])));
	RTV_REG_SET(0xD3, (WRD3 | (g_atBW_TABLE_ADC[eBwType][11])));
	RTV_REG_SET(0xD4, (g_atBW_TABLE_ADC[eBwType][10]));
	RTV_REG_SET(0xD5, (WRD5 | (g_atBW_TABLE_ADC[eBwType][9])));
	RTV_REG_SET(0xD6, (WRD6 | (g_atBW_TABLE_ADC[eBwType][8])));
	RTV_REG_SET(0xD7, (WRD7 | (g_atBW_TABLE_ADC[eBwType][7])));
	RTV_REG_SET(0xD8, (WRD8 | (g_atBW_TABLE_ADC[eBwType][6])));
	RTV_REG_SET(0xD9, (WRD9 | (g_atBW_TABLE_ADC[eBwType][5])));
	RTV_REG_SET(0xDA, (WRDA | (g_atBW_TABLE_ADC[eBwType][4])));
	RTV_REG_SET(0xDB, ((g_atBW_TABLE_ADC[eBwType][3]) << 4) |
				(g_atBW_TABLE_ADC[eBwType][2]));
	RTV_REG_SET(0xDC, ((g_atBW_TABLE_ADC[eBwType][1]) << 4) |
				(g_atBW_TABLE_ADC[eBwType][0]));

	return RTV_SUCCESS;
}

static INT rtvRF_ChangeLpfBwType(enum E_RTV_BANDWIDTH_TYPE eBwType)
{
#ifdef RTV_DUAL_DIVERISTY_ENABLE
	if (rtvMTV23x_Get_Diversity_Current_path() == DIVERSITY_MASTER) {
		if (g_aeBwType == eBwType)
			return RTV_SUCCESS;
	} else {
		if (g_aeBwType_slave == eBwType)
			return RTV_SUCCESS;
	}
#else
	if (g_aeBwType == eBwType)
		return RTV_SUCCESS;
#endif

	RTV_REG_MAP_SEL(RF_PAGE);

	if (rtvRF_ConfigureClkCKSYN(eBwType) != RTV_SUCCESS)
		return RTV_ADC_CLK_UNLOCKED;

	rtvRF_ConfigureIIRFilter(eBwType);
	rtvRF_ConfigureBBA(eBwType);
	rtvRF_ConfigureADC(eBwType);

#ifdef RTV_DUAL_DIVERISTY_ENABLE
	if (rtvMTV23x_Get_Diversity_Current_path() == DIVERSITY_MASTER)
		g_aeBwType = eBwType;
	else
		g_aeBwType_slave = eBwType;
#else
	g_aeBwType = eBwType;
#endif

	return RTV_SUCCESS;
}

static INT rtvRF_Lna_Tuning(U32 dwLoFreq)
{
	U8 nidx = 0;
	U8 WR50 = 0, WR73 = 0, WR4E = 0, WR69 = 0, WR88 = 0, WR89 = 0;
	U8 WR8A = 0, WR8B = 0;
	U8 WR6C = 0, WR6D = 0, WR6A = 0, WR6B = 0, WR8C = 0, WR8D = 0;
	U8 WR8E = 0, WR8F = 0;
	U8 WR90 = 0, WR91 = 0, WR92 = 0, WR87 = 0, WR93 = 0, WR94 = 0;

	if (75000 < dwLoFreq && 90000 >= dwLoFreq)
		nidx = 0;
	else if (90000 < dwLoFreq && 100000 >= dwLoFreq)
		nidx = 1;
	else if (100000 < dwLoFreq && 115000 >= dwLoFreq)
		nidx = 2;
	else if (115000 < dwLoFreq && 180000 >= dwLoFreq)
		nidx = 3;
	else if (180000 < dwLoFreq && 190000 >= dwLoFreq)
		nidx = 4;
	else if (190000 < dwLoFreq && 200000 >= dwLoFreq)
		nidx = 5;
	else if (200000 < dwLoFreq && 210000 >= dwLoFreq)
		nidx = 6;
	else if (210000 < dwLoFreq && 220000 >= dwLoFreq)
		nidx = 7;
	else if (220000 < dwLoFreq && 230000 >= dwLoFreq)
		nidx = 8;
	else if (230000 < dwLoFreq && 240000 >= dwLoFreq)
		nidx = 9;
	else if (240000 < dwLoFreq && 250000 >= dwLoFreq)
		nidx = 10;
	else if (250000 < dwLoFreq && 320000 >= dwLoFreq)
		nidx = 11;
	else if (320000 < dwLoFreq && 510000 >= dwLoFreq)
		nidx = 12;
	else if (510000 < dwLoFreq && 540000 >= dwLoFreq)
		nidx = 13;
	else if (540000 < dwLoFreq && 560000 >= dwLoFreq)
		nidx = 14;
	else if (560000 < dwLoFreq && 600000 >= dwLoFreq)
		nidx = 15;
	else if (600000 < dwLoFreq && 630000 >= dwLoFreq)
		nidx = 16;
	else if (630000 < dwLoFreq && 710000 >= dwLoFreq)
		nidx = 17;
	else if (710000 < dwLoFreq && 810000 >= dwLoFreq)
		nidx = 18;
	else if (810000 < dwLoFreq && 880000 >= dwLoFreq)
		nidx = 19;
	else
		return RTV_INVAILD_FREQUENCY_RANGE;

#ifdef RTV_DUAL_DIVERISTY_ENABLE
	if (rtvMTV23x_Get_Diversity_Current_path() == DIVERSITY_MASTER) {
		if (g_nLnaTuneVal == nidx)
			return RTV_SUCCESS;
	} else {
		if (g_nLnaTuneVal_slave == nidx)
			return RTV_SUCCESS;
	}
#else
	if (g_nLnaTuneVal == nidx)
		return RTV_SUCCESS;
#endif

	RTV_REG_MAP_SEL(RF_PAGE);

	WR50 = RTV_REG_GET(0x50) & 0xE0;
	WR73 = RTV_REG_GET(0x73) & 0xFC;
	WR4E = RTV_REG_GET(0x4E) & 0xE0;
	WR69 = RTV_REG_GET(0x69) & 0x80;
	WR88 = RTV_REG_GET(0x88) & 0x03;
	WR89 = RTV_REG_GET(0x89) & 0x03;
	WR8A = RTV_REG_GET(0x8A) & 0x00;
	WR8B = RTV_REG_GET(0x8B) & 0x00;
	WR6C = RTV_REG_GET(0x6C) & 0x03;
	WR6D = RTV_REG_GET(0x6D) & 0x03;
	WR6A = RTV_REG_GET(0x6A) & 0xC1;
	WR6B = RTV_REG_GET(0x6B) & 0x03;
	WR8C = RTV_REG_GET(0x8C) & 0x00;
	WR8D = RTV_REG_GET(0x8D) & 0x00;
	WR8E = RTV_REG_GET(0x8E) & 0x00;
	WR8F = RTV_REG_GET(0x8F) & 0x00;
	WR90 = RTV_REG_GET(0x90) & 0x00;
	WR91 = RTV_REG_GET(0x91) & 0x00;
	WR92 = RTV_REG_GET(0x92) & 0x00;
	WR87 = RTV_REG_GET(0x87) & 0xFB;
	WR93 = RTV_REG_GET(0x93) & 0x03;
	WR94 = RTV_REG_GET(0x94) & 0x03;

	RTV_REG_SET(0x50, WR50 |  g_atLNA_TABLE[nidx][0]);
	RTV_REG_SET(0x73, WR73 |  g_atLNA_TABLE[nidx][1]);
	RTV_REG_SET(0x4E, WR4E |  g_atLNA_TABLE[nidx][2]);
	RTV_REG_SET(0x69, WR69 | (g_atLNA_TABLE[nidx][3] << 2)
				| g_atLNA_TABLE[nidx][28]);

	RTV_REG_SET(0x88, WR88 | (g_atLNA_TABLE[nidx][4] << 5)
				| (g_atLNA_TABLE[nidx][5] << 2));

	RTV_REG_SET(0x89, WR89 | (g_atLNA_TABLE[nidx][6] << 5)
				| (g_atLNA_TABLE[nidx][7] << 2));

	RTV_REG_SET(0x8A, WR8A | (g_atLNA_TABLE[nidx][8] << 4)
				| g_atLNA_TABLE[nidx][9]);

	RTV_REG_SET(0x8B, WR8B | (g_atLNA_TABLE[nidx][10] << 4)
				| g_atLNA_TABLE[nidx][11]);

	RTV_REG_SET(0x6C, WR6C | (g_atLNA_TABLE[nidx][12] << 2));
	RTV_REG_SET(0x6D, WR6D | (g_atLNA_TABLE[nidx][13] << 2));
	RTV_REG_SET(0x6A, WR6A | (g_atLNA_TABLE[nidx][14] << 1));
	RTV_REG_SET(0x6B, WR6B | (g_atLNA_TABLE[nidx][15] << 2));
	RTV_REG_SET(0x8C, WR8C | (g_atLNA_TABLE[nidx][16] << 2)
				| ((g_atLNA_TABLE[nidx][18] & 0x18) >> 3));

	RTV_REG_SET(0x8D, WR8D | (g_atLNA_TABLE[nidx][20] << 2)
				| ((g_atLNA_TABLE[nidx][18] & 0x06) >> 1));

	RTV_REG_SET(0x8E, WR8E | (g_atLNA_TABLE[nidx][24] << 2)
				| ((g_atLNA_TABLE[nidx][18] & 0x01) << 1)
				| ((g_atLNA_TABLE[nidx][22] & 0x10) >> 4));

	RTV_REG_SET(0x8F, WR8F | (g_atLNA_TABLE[nidx][17] << 2)
				| ((g_atLNA_TABLE[nidx][22] & 0x0C) >> 2));

	RTV_REG_SET(0x90, WR90 | (g_atLNA_TABLE[nidx][21] << 2)
				| ((g_atLNA_TABLE[nidx][22] & 0x03) >> 0));

	RTV_REG_SET(0x91, WR91 | (g_atLNA_TABLE[nidx][25] << 2)
				| ((g_atLNA_TABLE[nidx][26] & 0x18) >> 3));

	RTV_REG_SET(0x92, WR92 | (g_atLNA_TABLE[nidx][19] << 2)
				| ((g_atLNA_TABLE[nidx][26] & 0x06) >> 1));

	RTV_REG_SET(0x87, WR87 | ((g_atLNA_TABLE[nidx][26] & 0x01) << 2));
	RTV_REG_SET(0x93, WR93 | (g_atLNA_TABLE[nidx][23] << 2));
	RTV_REG_SET(0x94, WR94 | (g_atLNA_TABLE[nidx][27] << 2));

#ifdef RTV_DUAL_DIVERISTY_ENABLE
	if (rtvMTV23x_Get_Diversity_Current_path() == DIVERSITY_MASTER)
		g_nLnaTuneVal = nidx;
	else
		g_nLnaTuneVal_slave = nidx;
#else
	g_nLnaTuneVal = nidx;
#endif

	return RTV_SUCCESS;
}

static INT rtvRF_SetUpVCO(U32 dwLoFreq, U32 *dwPllfreq)
{
	INT nRet = RTV_SUCCESS;
	UINT nVcoDivRate = 0;

	RTV_REG_MAP_SEL(RF_PAGE);

	if (dwLoFreq < 107500) {
		nVcoDivRate = 5;
		RTV_REG_MASK_SET(0x78, 0x0F, 0x0F);
	} else if (dwLoFreq >= 107500 && dwLoFreq < 215000) {
		nVcoDivRate = 4;
		RTV_REG_MASK_SET(0x78, 0x0F, 0x0E);
	} else if (dwLoFreq >= 215000 && dwLoFreq < 430000) {
		nVcoDivRate = 3;
		RTV_REG_MASK_SET(0x78, 0x0F, 0x0E);
	} else {
		nVcoDivRate = 2;
		RTV_REG_MASK_SET(0x78, 0x0F, 0x0D);
	}

	*dwPllfreq = dwLoFreq * (1<<nVcoDivRate);

	RTV_REG_MASK_SET(0x28, 0x03, ((nVcoDivRate & 0x06)>>1));
	RTV_REG_MASK_SET(0x29, 0x08, (nVcoDivRate & 0x01)<<3);

	if (*dwPllfreq >= 1720000 && *dwPllfreq < 1892000) {
		RTV_REG_MASK_SET(0x94, 0x02, 0x00);
		RTV_REG_MASK_SET(0x78, 0x70, 0x40);
		RTV_REG_MASK_SET(0xEA, 0x60, 0x00);
	} else if (*dwPllfreq >= 1892000 && *dwPllfreq < 3440000) {
		RTV_REG_MASK_SET(0x94, 0x02, 0x00);
		RTV_REG_MASK_SET(0x78, 0x70, 0x30);
		RTV_REG_MASK_SET(0xEA, 0x60, 0x00);
	} else
		nRet =   RTV_INVAILD_FREQUENCY_RANGE;

	return nRet;
}

static INT rtvRF_SetOfdmPara(enum E_RTV_SERVICE_TYPE eServiceType,
			enum E_RTV_BANDWIDTH_TYPE eLpfBwType, U32 dwChFreqKHz)
{
	INT nRet = RTV_SUCCESS;
	INT nNumAdcType = 0;
	const struct RTV_ADC_CFG_INFO *ptOfdmCfgTbl = NULL;

	switch (eServiceType) {
#if defined(RTV_ISDBT_ENABLE)
	case RTV_SERVICE_UHF_ISDBT_1seg:
		nNumAdcType = 1; /* ADC 9MHz */
		switch (eLpfBwType) {
		case RTV_BW_MODE_6MHZ:
		case RTV_BW_MODE_430KHZ:
			if ((dwChFreqKHz == 485143) || (dwChFreqKHz == 503143)
			|| (dwChFreqKHz == 539143) || (dwChFreqKHz == 647143)
			|| (dwChFreqKHz == 665143) || (dwChFreqKHz == 683143)
			|| (dwChFreqKHz == 755143))
				nNumAdcType = 0; /* ADC 8MHz */

			ptOfdmCfgTbl = &g_atAdcCfgTbl_ISDBT_6MHz[nNumAdcType];
			break;

		case RTV_BW_MODE_7MHZ:
		case RTV_BW_MODE_500KHZ:
			ptOfdmCfgTbl = &g_atAdcCfgTbl_ISDBT_7MHz[nNumAdcType];
			break;

		case RTV_BW_MODE_8MHZ:
		case RTV_BW_MODE_571KHZ:
			ptOfdmCfgTbl = &g_atAdcCfgTbl_ISDBT_8MHz[nNumAdcType];
			break;
		default:
			RTV_DBGMSG0("[rtvRF_SetOfdmPara] Unsupport 1seg BW\n");
			return RTV_INVAILD_SERVICE_TYPE;
		}
		break;

	case RTV_SERVICE_VHF_ISDBTmm_1seg:
		nNumAdcType = 0; /* ADC 8MHz */
		switch (eLpfBwType) {
		case RTV_BW_MODE_6MHZ:
		case RTV_BW_MODE_430KHZ:
			ptOfdmCfgTbl = &g_atAdcCfgTbl_ISDBT_6MHz[nNumAdcType];
			break;
		default:
			 RTV_DBGMSG0("[rtvRF_SetOfdmPara] Unsupport Tmm1seg\n");
			 return RTV_INVAILD_SERVICE_TYPE;
		}
		break;

	case RTV_SERVICE_VHF_ISDBTsb_1seg:
		nNumAdcType = 1; /* ADC 9MHz */
		switch (eLpfBwType) {
		case RTV_BW_MODE_6MHZ:
		case RTV_BW_MODE_430KHZ:
			ptOfdmCfgTbl = &g_atAdcCfgTbl_ISDBT_6MHz[nNumAdcType];
			break;
		default:
			RTV_DBGMSG0("[rtvRF_SetOfdmPara] Unsupport Tsb1seg\n");
			return RTV_INVAILD_SERVICE_TYPE;
		}
		break;

	case RTV_SERVICE_VHF_ISDBTsb_3seg:
		 RTV_DBGMSG0("[rtvRF_SetOfdmPara] Unsupport Tsb3seg\n");
		 return RTV_INVAILD_SERVICE_TYPE;

	case RTV_SERVICE_VHF_ISDBTmm_13seg:
		nNumAdcType = 4; /* ADC 20.48MHz */
		ptOfdmCfgTbl = &g_atAdcCfgTbl_ISDBT_6MHz[nNumAdcType];
		break;

	case RTV_SERVICE_UHF_ISDBT_13seg:
		if ((dwChFreqKHz == 551143) || (dwChFreqKHz == 581143) ||
		(dwChFreqKHz == 611143) || (dwChFreqKHz == 617143) ||
		(dwChFreqKHz == 647143) || (dwChFreqKHz == 677143) ||
		(dwChFreqKHz == 707143) || (dwChFreqKHz == 737143) ||
		(dwChFreqKHz == 767143) || (dwChFreqKHz == 797143))
			nNumAdcType = 2; /* ADC 19.2MHz */
		else if ((dwChFreqKHz == 491143) || (dwChFreqKHz == 521143))
			nNumAdcType = 3; /* ADC 20.0MHz */
		else
			nNumAdcType = 4; /* ADC 20.48MHz */

		switch (eLpfBwType) {
		case RTV_BW_MODE_6MHZ:
			ptOfdmCfgTbl = &g_atAdcCfgTbl_ISDBT_6MHz[nNumAdcType];
			break;
		case RTV_BW_MODE_7MHZ:
			ptOfdmCfgTbl = &g_atAdcCfgTbl_ISDBT_7MHz[nNumAdcType];
			break;
		case RTV_BW_MODE_8MHZ:
			ptOfdmCfgTbl = &g_atAdcCfgTbl_ISDBT_8MHz[nNumAdcType];
			break;
		default:
			RTV_DBGMSG0("[rtvRF_SetOfdmPara] Unsupport 13seg\n");
			return RTV_INVAILD_SERVICE_TYPE;
		}
		break;
#endif
#if defined(RTV_DVBT_ENABLE)
	case RTV_SERVICE_DVBT:
		if ((dwChFreqKHz == 184500) || (dwChFreqKHz == 490000) ||
		(dwChFreqKHz == 554000) || (dwChFreqKHz == 618000) ||
		(dwChFreqKHz == 674000) || (dwChFreqKHz == 738000) ||
		(dwChFreqKHz == 858000))
			nNumAdcType = 2; /* ADC 19.2MHz */
		else
			nNumAdcType = 4; /* ADC 20.48MHz */

		switch (eLpfBwType) {
		case RTV_BW_MODE_5MHZ:
			ptOfdmCfgTbl = &g_atAdcCfgTbl_DVBT_5MHz[nNumAdcType];
			break;
		case RTV_BW_MODE_6MHZ:
			ptOfdmCfgTbl = &g_atAdcCfgTbl_DVBT_6MHz[nNumAdcType];
			break;
		case RTV_BW_MODE_7MHZ:
			ptOfdmCfgTbl = &g_atAdcCfgTbl_DVBT_7MHz[nNumAdcType];
			break;
		case RTV_BW_MODE_8MHZ:
			ptOfdmCfgTbl = &g_atAdcCfgTbl_DVBT_8MHz[nNumAdcType];
			break;
		default:
			RTV_DBGMSG0("[rtvRF_SetOfdmPara] Unsupport DVB-T\n");
			return RTV_INVAILD_SERVICE_TYPE;
		}
		break;
#endif
	default:
		RTV_DBGMSG0("[rtvRF_SetOfdmPara] Invaild Service Type\n");
		return RTV_INVAILD_SERVICE_TYPE;
	}

	RTV_REG_MAP_SEL(RF_PAGE);
	RTV_REG_SET(0x2A, ptOfdmCfgTbl->bData2A);
	RTV_REG_SET(0x6E, ptOfdmCfgTbl->bData6E);
	RTV_REG_SET(0x70, ptOfdmCfgTbl->bData70);
	RTV_REG_SET(0x71, ptOfdmCfgTbl->bData71);
	RTV_REG_MASK_SET(0x75, 0xFC, (ptOfdmCfgTbl->bData75 << 2));

	if (rtvRF_LockCheck(1) != RTV_SUCCESS)
		return RTV_ADC_CLK_UNLOCKED;

	if ((eServiceType == RTV_SERVICE_UHF_ISDBT_1seg) ||
	(eServiceType == RTV_SERVICE_VHF_ISDBTmm_1seg) ||
	(eServiceType == RTV_SERVICE_VHF_ISDBTsb_1seg) ||
	(eServiceType == RTV_SERVICE_VHF_ISDBTsb_3seg)) {
		RTV_REG_MAP_SEL(LPOFDM_PAGE);
		RTV_REG_SET(0x14, (U8)((ptOfdmCfgTbl->dwTNCO >> 0) & 0xFF));
		RTV_REG_SET(0x15, (U8)((ptOfdmCfgTbl->dwTNCO >>  8) & 0xFF));
		RTV_REG_SET(0x16, (U8)((ptOfdmCfgTbl->dwTNCO >> 16) & 0xFF));
		RTV_REG_SET(0x17, (U8)((ptOfdmCfgTbl->dwTNCO >> 24) & 0xFF));

		RTV_REG_SET(0x18, (U8)((ptOfdmCfgTbl->dwPNCO >> 0) & 0xFF));
		RTV_REG_SET(0x19, (U8)((ptOfdmCfgTbl->dwPNCO >> 8) & 0xFF));
		RTV_REG_SET(0x1A, (U8)((ptOfdmCfgTbl->dwPNCO >> 16) & 0xFF));
		RTV_REG_SET(0x1B, (U8)((ptOfdmCfgTbl->dwPNCO >> 24) & 0xFF));

		RTV_REG_SET(0x1C, (U8)((ptOfdmCfgTbl->dwGAIN) & 0xFF));

		RTV_REG_SET(0x1D, (U8)((ptOfdmCfgTbl->dwCFREQGAIN>>0) & 0xFF));
		RTV_REG_SET(0x1E, (U8)((ptOfdmCfgTbl->dwCFREQGAIN>>8) & 0xFF));
		RTV_REG_SET(0x1F, (U8)((ptOfdmCfgTbl->dwCFREQGAIN>>16) & 0xFF));
	} else {
		RTV_REG_MAP_SEL(OFDM_PAGE);
		switch (eLpfBwType) {
		case RTV_BW_MODE_5MHZ:
			RTV_REG_MASK_SET(0x11, 0x01, 0x00);
			RTV_REG_MASK_SET(0x10, 0xE0, 0xB0);
			RTV_REG_MASK_SET(0x1F, 0xC0, 0xC0);
			break;
		case RTV_BW_MODE_6MHZ:
			RTV_REG_MASK_SET(0x11, 0x01, 0x00);
			RTV_REG_MASK_SET(0x10, 0xE0, 0xC0);
			RTV_REG_MASK_SET(0x1F, 0xC0, 0x80);
			break;
		case RTV_BW_MODE_7MHZ:
			RTV_REG_MASK_SET(0x11, 0x01, 0x00);
			RTV_REG_MASK_SET(0x10, 0xE0, 0xE0);
			RTV_REG_MASK_SET(0x1F, 0xC0, 0x40);
			break;
		case RTV_BW_MODE_8MHZ:
			RTV_REG_MASK_SET(0x11, 0x01, 0x01);
			RTV_REG_MASK_SET(0x10, 0xE0, 0x00);
			RTV_REG_MASK_SET(0x1F, 0xC0, 0x00);
			break;
		default:
			RTV_DBGMSG0("[rtvRF_SetFrequency] Unsupported BW\n");
			return RTV_INVAILD_SERVICE_TYPE;
			break;
		}

		RTV_REG_SET(0x34, (U8)((ptOfdmCfgTbl->dwTNCO>>0) & 0xFF));
		RTV_REG_SET(0x35, (U8)((ptOfdmCfgTbl->dwTNCO>>8) & 0xFF));
		RTV_REG_SET(0x36, (U8)((ptOfdmCfgTbl->dwTNCO>>16) & 0xFF));
		RTV_REG_SET(0x37, (U8)((ptOfdmCfgTbl->dwTNCO>>24) & 0xFF));

		RTV_REG_SET(0x38, (U8) (ptOfdmCfgTbl->dwPNCO>>0));
		RTV_REG_SET(0x39, (U8) (ptOfdmCfgTbl->dwPNCO>>8));
		RTV_REG_SET(0x3A, (U8) (ptOfdmCfgTbl->dwPNCO>>16));
		RTV_REG_SET(0x3B, (U8) (ptOfdmCfgTbl->dwPNCO>>24));

		RTV_REG_SET(0x3D, (U8)((ptOfdmCfgTbl->dwCFREQGAIN>>0) & 0xFF));
		RTV_REG_SET(0x3E, (U8)((ptOfdmCfgTbl->dwCFREQGAIN>>8) & 0xFF));
		RTV_REG_SET(0x3F, (U8)((ptOfdmCfgTbl->dwCFREQGAIN>>16) & 0xFF));

		RTV_REG_MASK_SET(0x55, 0xC0, (ptOfdmCfgTbl->dwGAIN & 0x03));
		RTV_REG_SET(0x56, (ptOfdmCfgTbl->dwGAIN>>2) & 0xFF);
	}

	return nRet;
}

static INT rtvRF_SelectService(enum E_RTV_SERVICE_TYPE eServiceType)
{
	UINT nRet = RTV_SUCCESS;

#ifdef RTV_DUAL_DIVERISTY_ENABLE
	if (rtvMTV23x_Get_Diversity_Current_path() == DIVERSITY_MASTER) {
		if (g_eRtvServiceType == eServiceType)
				return RTV_SUCCESS;
	} else {
		if (g_eRtvServiceType_slave == eServiceType)
				return RTV_SUCCESS;
	}
#else
	if (g_eRtvServiceType == eServiceType)
			return RTV_SUCCESS;
#endif

	rtv_ServiceTypeSelect(eServiceType);

	RTV_REG_MAP_SEL(RF_PAGE);

	switch (eServiceType) {
	case RTV_SERVICE_UHF_ISDBT_1seg:
		RTV_REG_SET(0x49, 0x21);
		RTV_REG_SET(0x4A, 0x60);
		RTV_REG_SET(0x4B, 0x50);
		RTV_REG_SET(0x5E, 0x70);
		RTV_REG_SET(0x5F, 0x75);
		break;
	case RTV_SERVICE_VHF_ISDBTmm_1seg:
		RTV_REG_SET(0x49, 0x21);
		RTV_REG_SET(0x4A, 0x30);
		RTV_REG_SET(0x4B, 0x20);
		RTV_REG_SET(0x5E, 0x70);
		RTV_REG_SET(0x5F, 0x75);
		break;
	case RTV_SERVICE_VHF_ISDBTsb_1seg:
		RTV_REG_SET(0x49, 0x21);
		RTV_REG_SET(0x4A, 0x60);
		RTV_REG_SET(0x4B, 0x50);
		RTV_REG_SET(0x5E, 0x70);
		RTV_REG_SET(0x5F, 0x75);
		break;
	case RTV_SERVICE_VHF_ISDBTsb_3seg:
		RTV_DBGMSG0("[rtvRF_SelectService] Unsupported 3seg\n");
		break;
	case RTV_SERVICE_UHF_ISDBT_13seg:
		RTV_REG_SET(0x49, 0x41);
		RTV_REG_SET(0x4A, 0x70);
		RTV_REG_SET(0x4B, 0x65);
		RTV_REG_SET(0x5E, 0x70);
		RTV_REG_SET(0x5F, 0x75);
		break;
	case RTV_SERVICE_VHF_ISDBTmm_13seg:
		RTV_REG_SET(0x49, 0x41);
		RTV_REG_SET(0x4A, 0x70);
		RTV_REG_SET(0x4B, 0x65);
		RTV_REG_SET(0x5E, 0x70);
		RTV_REG_SET(0x5F, 0x75);
		break;

#if defined(RTV_DVBT_ENABLE)
	case RTV_SERVICE_DVBT:
		RTV_REG_SET(0x49, 0x41);
		RTV_REG_SET(0x4A, 0x70);
		RTV_REG_SET(0x4B, 0x65);
		RTV_REG_SET(0x5E, 0x70);
		RTV_REG_SET(0x5F, 0x75);
		break;
#endif

	default:
		nRet = RTV_INVAILD_SERVICE_TYPE;
	}

#ifdef RTV_DUAL_DIVERISTY_ENABLE
	if (rtvMTV23x_Get_Diversity_Current_path() == DIVERSITY_MASTER)
		g_eRtvServiceType = eServiceType;
	else
		g_eRtvServiceType_slave = eServiceType;
#else
		g_eRtvServiceType = eServiceType;
#endif

	return nRet;
}

INT rtvRF_SetFrequency(enum E_RTV_SERVICE_TYPE eServiceType,
		enum E_RTV_BANDWIDTH_TYPE eLpfBwType, U32 dwChFreqKHz)
{
	INT nRet;
	U8 pllf_mul = 0, r_div = 4;
	U32 dwPLLN = 0, dwPLLF = 0, dwPLLNF = 0;
	U32 dwPllFreq = 0, dwLoFreq = 0;

	if (rtvRF_ChangeLpfBwType(eLpfBwType) != RTV_SUCCESS)
		return RTV_ADC_CLK_UNLOCKED;

	nRet = rtvRF_SetOfdmPara(eServiceType, eLpfBwType, dwChFreqKHz);
	if (nRet != RTV_SUCCESS)
		return nRet;

	nRet = rtvRF_SelectService(eServiceType);
	if (nRet != RTV_SUCCESS)
		return nRet;

	if (g_fRtv1segLpMode) {
		RTV_REG_MAP_SEL(LPOFDM_PAGE);

		if (eServiceType == RTV_SERVICE_VHF_ISDBTmm_1seg) {
			RTV_REG_SET(0x10, 0xFA);
			dwLoFreq = dwChFreqKHz - 857;
		} else {
			RTV_REG_SET(0x10, 0xF8);
			dwLoFreq = dwChFreqKHz + 857;
		}
	} else
		dwLoFreq = dwChFreqKHz;

	if (rtvRF_Lna_Tuning(dwLoFreq) != RTV_SUCCESS)
		return RTV_ERROR_LNATUNE;

	if (rtvRF_SetUpVCO(dwLoFreq, &dwPllFreq) != RTV_SUCCESS)
		return RTV_INVAILD_FREQUENCY_RANGE;

	dwPLLN = dwPllFreq / RTV_SRC_CLK_FREQ_KHz;
	dwPLLF = dwPllFreq - (dwPLLN * RTV_SRC_CLK_FREQ_KHz);
	if (RTV_SRC_CLK_FREQ_KHz == 13000 || RTV_SRC_CLK_FREQ_KHz == 27000) {
		pllf_mul = 1;
		r_div = 3;
	}

	dwPLLNF = (dwPLLN<<20)
		+ (((dwPLLF<<16) / (RTV_SRC_CLK_FREQ_KHz>>r_div)) << pllf_mul);

	RTV_REG_MAP_SEL(RF_PAGE);
	RTV_REG_SET(0x20, ((dwPLLNF>>22)&0xFF));
	RTV_REG_SET(0x21, ((dwPLLNF>>14)&0xFF));
	RTV_REG_MASK_SET(0x28, 0xFC, ((dwPLLNF&0x3F)<<2));
	RTV_REG_SET(0x22, ((dwPLLNF>>6)&0xFF));

	RTV_DELAY_MS(1);

	if (rtvRF_LockCheck(0) != RTV_SUCCESS)
		return RTV_PLL_UNLOCKED;

	if (dwPllFreq >= 2140000 && dwPllFreq < 2950000) {
		   RTV_REG_MASK_SET(0x94, 0x02, 0x02);
		   RTV_REG_MASK_SET(0x78, 0x70, 0x50);
		   RTV_REG_MASK_SET(0xEA, 0x60, 0x40);
	} else if (dwPllFreq >= 2950000 && dwPllFreq < 3440000) {
		   RTV_REG_MASK_SET(0x94, 0x02, 0x02);
		   RTV_REG_MASK_SET(0x78, 0x70, 0x40);
		   RTV_REG_MASK_SET(0xEA, 0x60, 0x00);
	}
	   

	g_dwRtvPrevChFreqKHz = dwChFreqKHz;

	return RTV_SUCCESS;
}

INT rtvRF_Initilize(enum E_RTV_BANDWIDTH_TYPE eBandwidthType)
{
	UINT nNumTblEntry = 0;
	const struct RTV_REG_INIT_INFO *ptInitTbl = NULL;

	ptInitTbl = t_mtv23x_INIT;
	nNumTblEntry = sizeof(t_mtv23x_INIT) / sizeof(struct RTV_REG_INIT_INFO);
	g_aeBwType = MAX_NUM_RTV_BW_MODE_TYPE;
	g_nLnaTuneVal = 0xFF;
	g_dwRtvPrevChFreqKHz = 0;
#ifdef RTV_DUAL_DIVERISTY_ENABLE
	g_aeBwType_slave = MAX_NUM_RTV_BW_MODE_TYPE;
	g_nLnaTuneVal_slave = 0xFF;
#endif
	RTV_REG_MAP_SEL(RF_PAGE);

	do {
		RTV_REG_SET(ptInitTbl->bReg, ptInitTbl->bVal);
		ptInitTbl++;
	} while (--nNumTblEntry);

#ifdef DEBUG_A_TEST_ZERO
	RTV_REG_MASK_SET(0x85, 0x02, 0x02);
#endif

	if (rtvRF_ChangeLpfBwType(eBandwidthType) != RTV_SUCCESS)
		return RTV_ADC_CLK_UNLOCKED;

	return RTV_SUCCESS;
}

