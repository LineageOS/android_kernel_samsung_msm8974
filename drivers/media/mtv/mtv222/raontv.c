/******************************************************************************
* (c) COPYRIGHT 2013 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE      : RAONTECH TV device driver API header file.
*
* FILENAME   : raontv.c
*
* DESCRIPTION:
*  Configuration for RAONTECH TV Services.
*
******************************************************************************/
/******************************************************************************
* REVISION HISTORY
*
*    DATE	  	  NAME				REMARKS
* ----------  -------------    ------------------------------------------------
* 07/26/2013  Yang, Maverick   Created.
******************************************************************************/

#include "raontv_rf.h"

volatile BOOL g_fRtvChannelChange;


U8 g_bRtvPage;

volatile E_RTV_ADC_CLK_FREQ_TYPE g_eRtvAdcClkFreqType;
BOOL g_fRtvStreamEnabled;

#if defined(RTV_TDMB_ENABLE) || defined(RTV_ISDBT_ENABLE)
	E_RTV_COUNTRY_BAND_TYPE g_eRtvCountryBandType;
#endif

UINT g_nRtvMscThresholdSize;
U8 g_bRtvIntrMaskRegL;

U8 g_bAdjId;
U8 g_bAdjSat;
U8 g_bAdjRefL;

INT rtv_InitSystem(E_RTV_TV_MODE_TYPE eTvMode,
		E_RTV_ADC_CLK_FREQ_TYPE eAdcClkFreqType)
{
	INT nRet;
	int i;
	U8 read0, read1;

	g_fRtvChannelChange = FALSE;
	g_fRtvStreamEnabled = FALSE;

	g_bRtvIntrMaskRegL = 0xFF;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#ifdef RTV_IF_SPI
	#define WR27_VAL	0x11
	#else
	#define WR27_VAL	0x13
	#endif

	#define WR29_VAL	0x31

	RTV_REG_MAP_SEL(SPI_CTRL_PAGE);

	for (i = 0; i < 100; i++) {
		RTV_REG_SET(0x29, WR29_VAL); /* BUFSEL first! */
		RTV_REG_SET(0x27, WR27_VAL);

		read0 = RTV_REG_GET(0x27);
		read1 = RTV_REG_GET(0x29);
		RTV_DBGMSG2("read27(0x%02X), read29(0x%02X)\n", read0, read1);

		if ((read0 == WR27_VAL) && (read1 == WR29_VAL))
			goto RTV_POWER_ON_SUCCESS;

		RTV_DBGMSG1("[rtv_InitSystem] Power On wait: %d\n", i);
		RTV_DELAY_MS(5);
	}
#else
	RTV_REG_MAP_SEL(HOST_PAGE);
	for (i = 0; i < 100; i++) {
		read0 = RTV_REG_GET(0x00);
		read1 = RTV_REG_GET(0x01);
		RTV_DBGMSG2("read27(0x%02X), read29(0x%02X)\n", read0, read1);

		if ((read0 == 0xC6) && (read1 == 0x43))
			goto RTV_POWER_ON_SUCCESS;

		RTV_DBGMSG1("[rtv_InitSystem] Power On wait: %d\n", i);
		RTV_DELAY_MS(5);
	}
#endif

	RTV_DBGMSG1("rtv_InitSystem: Power On Check error: %d\n", i);
	return RTV_POWER_ON_CHECK_ERROR;

RTV_POWER_ON_SUCCESS:

	rtvRF_ConfigurePowerType(eTvMode);

	nRet = rtvRF_ChangeAdcClock(eTvMode, eAdcClkFreqType, 500);
	if (nRet != RTV_SUCCESS)
		return nRet;

	return RTV_SUCCESS;
}

