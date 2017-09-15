/******************************************************************************
* (c) COPYRIGHT 2013 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE 	  : RAONTECH TV RF services source file.
*
* FILENAME    : raontv_rf.c
*
* DESCRIPTION : 
*		Library of routines to initialize, and operate on, the RAONTECH RF chip.
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
#include "raontv_rf_adc_data.h"

/* Down conversion Signal Monitoring. */
//#define DEBUG_A_TEST_ZERO

#ifdef RTV_ISDBT_ENABLE

 static const RTV_REG_INIT_INFO t_ISDBT_INIT[] =   
 {
	{  0x28,	 0x83  },
	{  0x2A,	 0x00  },
	{  0x2B,	 0x42  },
	{  0x2C,	 0xF8  },
	{  0x2D,	 0x00  },
	{  0x2E,	 0x18  },
	{  0x32,	 0x03  },
	{  0x33,	 0x1B  },
	{  0x34,	 0x60  },
	{  0x37,	 0xA0  },
	{  0x38,	 0x08  },
	{  0x39,	 0x53  },
	{  0x3A,	 0xDC  },
	{  0x3B,	 0xD0  },
	{  0x3C,	 0xEB  }, // 0x0B => 0xEB
	{  0x3D,	 0x76  },
	{  0x3E,	 0x4F  },
	{  0x3F,	 0x26  },
	{  0x40,	 0xDA  },
	{  0x41,	 0x06  },	
	{  0x42,	 0x02  },
	{  0x43,	 0x50  },
	{  0x44,	 0x4F  },
	{  0x45,	 0x88  },
	{  0x46,	 0xDF  },
	{  0x49,	 0x34  },
	{  0x4A,	 0x23  },
	{  0x4E,	 0xFF  },
	{  0x4F,	 0xFD  },
	{  0x50,	 0x7F  },
#if defined(RAONTV_CHIP_PKG_WLCSP_HRM_ON)
	{  0x51,	 0x00  },
#else
	{  0x51,	 0x07  },
#endif
	{  0x52,	 0x49  },
	{  0x56,	 0x14  },
	{  0x58,	 0x13  },
	{  0x5A,	 0x3F  },
	{  0x63,	 0x0C  },
	{  0x64,	 0x0C  },
	{  0x65,	 0x4C  },
	{  0x66,	 0x0C  },
	{  0x67,	 0x0C  },
	{  0x68,	 0x4C  },
	{  0x69,	 0x0C  },
	{  0x6A,	 0x0C  },
	{  0x6B,	 0x4C  },
	{  0x6D,	 0x1E  },
	{  0x6E,	 0x1E  },
	{  0x6F,	 0x1E  },
	{  0x70,	 0x1E  },
	{  0x71,	 0x1E  },
	{  0x72,	 0x1E  },
	{  0x73,	 0x1F  },
	{  0x74,	 0x1F  },
	{  0x78,	 0x08  },
	{  0x7A,	 0x07  },
	{  0x7B,	 0x03  },
	{  0x7D,	 0x08  },
	{  0x86,	 0x82  },
	{  0x88,	 0x8C  },
	{  0x89,	 0x0A  },
	{  0x8E,	 0x69  },
	{  0x8F,	 0x00  },
	{  0x92,	 0x80  },
#ifdef DEBUG_A_TEST_ZERO
	{  0xA8,	 0xA6  },
#endif
	{  0xA9,	 0xD6  },
	{  0xAA,	 0xB5  },
	{  0xAB,	 0x51  }, 
	{  0xAC,	 0xCA  },
	{  0xAD,	 0x52  },
	{  0xAE,	 0x0C  },
	{  0xAF,	 0x62  },
	{  0xB1,	 0x75  }
	
 };	
#endif /* RTV_ISDBT_ENABLE */

volatile U8 g_nLnaTuneVal;

/*===============================================================================
 * rtvRF_ConfigurePowerType
 *
 * DESCRIPTION : 
 *		This function returns 
 *		
 *
 * ARGUMENTS : none.
 * RETURN VALUE : none.
 *============================================================================*/
void rtvRF_ConfigurePowerType(E_RTV_TV_MODE_TYPE eTvMode)
{

   	RTV_REG_MAP_SEL(RF_PAGE);
#if defined(RTV_PWR_LDO)             
	RTV_REG_SET(0x5D,0x00);
#elif defined(RTV_PWR_EXTERNAL)          
	RTV_REG_SET(0x5D,0x01);
#else
	#error "Code not present"
#endif    

}

static INT rtvRF_Lna_Tuning( U32 dwLoFreq)
{
	U8 nidx=0;

#if defined(RAONTV_CHIP_PKG_QFN)
	if (470000 < dwLoFreq && 500000 >= dwLoFreq)		nidx = 0;
	else if (500000 < dwLoFreq && 530000 >= dwLoFreq)	nidx = 1;
	else if (530000 < dwLoFreq && 630000 >= dwLoFreq)	nidx = 2;
	else if (630000 < dwLoFreq && 710000 >= dwLoFreq)	nidx = 3;
	else if (710000 < dwLoFreq && 810000 >= dwLoFreq)	nidx = 4;
	else
		return RTV_INVAILD_FREQ;
#else
	if (470000 < dwLoFreq && 510000 >= dwLoFreq)		nidx = 0;
	else if (510000 < dwLoFreq && 550000 >= dwLoFreq)	nidx = 1;
	else if (550000 < dwLoFreq && 590000 >= dwLoFreq)	nidx = 2;
	else if (590000 < dwLoFreq && 670000 >= dwLoFreq)	nidx = 3;
	else if (670000 < dwLoFreq && 709000 >= dwLoFreq)	nidx = 4;
	else if (709000 < dwLoFreq && 810000 >= dwLoFreq)	nidx = 5;
	else
		return RTV_INVAILD_FREQ;
#endif
   	if(g_nLnaTuneVal == nidx)
   	  return RTV_SUCCESS;

	RTV_REG_MAP_SEL(RF_PAGE);

	RTV_REG_MASK_SET(0x3E,0x3F,g_atLNAtbl[nidx][0]);
	RTV_REG_MASK_SET(0x42,0x03,g_atLNAtbl[nidx][1]);
	RTV_REG_MASK_SET(0x3C,0x3F,g_atLNAtbl[nidx][2]);
	RTV_REG_MASK_SET(0x41,0xF1,g_atLNAtbl[nidx][3]<<3);
	RTV_REG_MASK_SET(0xA9,0xF0,g_atLNAtbl[nidx][4]<<4);

	RTV_REG_MASK_SET(0xA9,0x0F,g_atLNAtbl[nidx][5]);
	RTV_REG_SET(0xAA,(g_atLNAtbl[nidx][6]<<4) | g_atLNAtbl[nidx][7]);
	RTV_REG_SET(0xAB,(g_atLNAtbl[nidx][8]<<3) | ((g_atLNAtbl[nidx][10] & 0x1C)>>2 ));
	RTV_REG_SET(0xAD,((g_atLNAtbl[nidx][14] & 0x0F) <<4)
	                | (g_atLNAtbl[nidx][9] & 0x1E)>>1);
	RTV_REG_SET(0xAE,((g_atLNAtbl[nidx][9] & 0x01)<<7)
					| (g_atLNAtbl[nidx][11]<<2)
					| ((g_atLNAtbl[nidx][13] & 0x18)>>3 ));
	RTV_REG_SET(0xAC,(g_atLNAtbl[nidx][10] & 0x03)<<6
	                | (g_atLNAtbl[nidx][12] <<1 )
	                |((g_atLNAtbl[nidx][14] & 0x10)>>4));

	RTV_REG_SET(0xAF,((g_atLNAtbl[nidx][13] & 0x07)<<5)
					|  g_atLNAtbl[nidx][15]);

	RTV_REG_MASK_SET(0x42,0x10,g_atLNAtbl[nidx][16]<<4);
	RTV_REG_MASK_SET(0x42,0x0C,g_atLNAtbl[nidx][17]<<2);

	g_nLnaTuneVal = nidx;
	
	return RTV_SUCCESS;
}


INT rtvRF_ChangeAdcClock(E_RTV_TV_MODE_TYPE eTvMode,
			E_RTV_ADC_CLK_FREQ_TYPE eAdcClkFreqType, S16 dwIFFreq)
{	
	U8 RD12;
	UINT nRetryCnt = 10;
	const U8 *pbAdcClkSynTbl = (const U8 *)&g_abAdcClkSynTbl[eAdcClkFreqType];
	const struct RTV_ADC_CFG_INFO *ptOfdmCfgTbl = &g_atOfdmCfgTbl_ISDBT[eAdcClkFreqType];

	if (pbAdcClkSynTbl[0] == 0xFF) {
		RTV_DBGMSG1("[rtvRF_ChangeAdcClock] Unsupport ADC clock type: %d\n", eAdcClkFreqType);
		return RTV_UNSUPPORT_ADC_CLK;
   	 }

	RTV_REG_MAP_SEL(OFDM_PAGE);

	if (dwIFFreq == 857) {
		RTV_REG_SET(0x18, ( ptOfdmCfgTbl->dwPNCO2  >> 0 ));  
		RTV_REG_SET(0x19, ( ptOfdmCfgTbl->dwPNCO2  >> 8 ));
		RTV_REG_SET(0x1A, ( ptOfdmCfgTbl->dwPNCO2  >> 16));
		RTV_REG_SET(0x1B, ( ptOfdmCfgTbl->dwPNCO2  >> 24));
	}
	else {
		RTV_REG_SET(0x18, ( ptOfdmCfgTbl->dwPNCO1  >> 0 ));  
		RTV_REG_SET(0x19, ( ptOfdmCfgTbl->dwPNCO1  >> 8 ));
		RTV_REG_SET(0x1A, ( ptOfdmCfgTbl->dwPNCO1  >> 16));
		RTV_REG_SET(0x1B, ( ptOfdmCfgTbl->dwPNCO1  >> 24));
	}

	if (eAdcClkFreqType == g_eRtvAdcClkFreqType)
		return RTV_SUCCESS;

	RTV_REG_SET(0x14, (ptOfdmCfgTbl->dwTNCO  >>  0) & 0xFF);   
	RTV_REG_SET(0x15, (ptOfdmCfgTbl->dwTNCO  >>  8) & 0xFF);
	RTV_REG_SET(0x16, (ptOfdmCfgTbl->dwTNCO  >> 16) & 0xFF);
	RTV_REG_SET(0x17, (ptOfdmCfgTbl->dwTNCO  >> 24) & 0xFF);

	RTV_REG_SET (0x1C,(ptOfdmCfgTbl->dwGAIN)&0xFF);

	RTV_REG_SET (0x1D, (ptOfdmCfgTbl->dwCFREQGAIN >>  0) & 0xFF);    
	RTV_REG_SET (0x1E, (ptOfdmCfgTbl->dwCFREQGAIN >>  8) & 0xFF);
	RTV_REG_SET (0x1F, (ptOfdmCfgTbl->dwCFREQGAIN >> 16) & 0xFF);

	RTV_REG_MAP_SEL(RF_PAGE);

	RTV_REG_SET(0x25,pbAdcClkSynTbl[0]);
	RTV_REG_SET(0x26,(pbAdcClkSynTbl[1] & 0xFF));
	RTV_REG_MASK_SET(0x87,0xFC,pbAdcClkSynTbl[2]<<2);
	RTV_REG_MASK_SET(0x85,0xFC,pbAdcClkSynTbl[3]<<2);
	RTV_REG_MASK_SET(0x5C,0xFC,pbAdcClkSynTbl[4]<<2);
	RTV_REG_MASK_SET(0x5B,0x3F,pbAdcClkSynTbl[5]);

	while (1) {
		RTV_DELAY_MS(1);
		RD12 = RTV_REG_GET(0x12);
		if (RD12 & 0x80)
			break;

		if (--nRetryCnt == 0) {
			RTV_DBGMSG0("[rtvRF_ChangeAdcClock] Syn Unlocked!\n");
			return RTV_ADC_CLK_UNLOCKED;
		}
	}

	g_eRtvAdcClkFreqType = eAdcClkFreqType;

#if 0
	RTV_DBGMSG1("[rtvRF_ChangeAdcClock] ADC clk Type: %d\n",
				g_eRtvAdcClkFreqType[RaonTvChipIdx]);
#endif

	return RTV_SUCCESS;	
}


INT rtvRF_SetFrequency(E_RTV_TV_MODE_TYPE eTvMode, UINT nChNum, U32 dwChFreqKHz)
{

	INT nRet = RTV_SUCCESS;
	E_RTV_ADC_CLK_FREQ_TYPE eAdcClkFreqType = RTV_ADC_CLK_FREQ_8_MHz;
	U32 dwPLLN = 0, dwPLLF = 0, dwPLLNF = 0; 
	U32 dwPllFreq = 0, dwLoFreq = 0;
	S16 dwIFfREQ = 0;
	U8 WR2A,RD15;
	U32 PLL_Verify_cnt = 10;
#if (RTV_SRC_CLK_FREQ_KHz == 19200)
	U8 nPllr=4;
#else
	U8 nPllr=1;
#endif

#if (RTV_SRC_CLK_FREQ_KHz == 13000) || (RTV_SRC_CLK_FREQ_KHz == 27000)
	#define pllf_mul	1
	#define r_div		3
#else
	#define pllf_mul	0
	#define r_div		4
#endif

	g_fRtvChannelChange = TRUE;
	g_bAdjRefL = 0x4F;

    /* Get the PLLNF and ADC clock type. */
	switch (nChNum) {
	case 14: case 22: case 38: case 46: case 54: case 57: case 62:
	case 17: case 25: case 26: case 33: case 49: case 65:
		eAdcClkFreqType = g_aeAdcClkTypeTbl_ISDBT[2];
		break;
	case 30: case 41: case 61:
		eAdcClkFreqType = g_aeAdcClkTypeTbl_ISDBT[1];
		break;
	default: 
		eAdcClkFreqType = g_aeAdcClkTypeTbl_ISDBT[0];
		break;
	}

#if (RTV_SRC_CLK_FREQ_KHz == 19200)
	if (nChNum & 0x01)
		dwIFfREQ = -343;
	else
		dwIFfREQ = 857;
#else
	dwIFfREQ = 500;
#endif

	dwLoFreq = dwChFreqKHz + dwIFfREQ;
    dwPllFreq = dwLoFreq << 1;
     
	if (rtvRF_Lna_Tuning(dwLoFreq) != RTV_SUCCESS) {
		nRet = RTV_INVAILD_FREQ;
		goto RF_SET_FREQ_EXIT;
	}

	nRet = rtvRF_ChangeAdcClock(eTvMode, eAdcClkFreqType,dwIFfREQ);
	if (nRet != RTV_SUCCESS)
		goto RF_SET_FREQ_EXIT;
	
	RTV_REG_MAP_SEL(RF_PAGE);

	if (dwLoFreq < 550000) {
		RTV_REG_MASK_SET(0x52,0x0F,0x0C);
		RTV_REG_MASK_SET(0x56,0x0C,0x08);
	}
	else {
		RTV_REG_MASK_SET(0x52,0x0F,0x09);
		RTV_REG_MASK_SET(0x56,0x0C,0x04);
	}


    /* Set the PLLNF and channel. */
	dwPLLN = dwPllFreq / RTV_SRC_CLK_FREQ_KHz;
	dwPLLF = dwPllFreq - (dwPLLN* RTV_SRC_CLK_FREQ_KHz);
	dwPLLNF = ((dwPLLN<<20 )
	     	+ (((dwPLLF<<16) / (RTV_SRC_CLK_FREQ_KHz>>r_div)) << pllf_mul))
	     	* nPllr ; 

	RTV_REG_MAP_SEL(RF_PAGE);
	RTV_REG_SET(0x21, ((dwPLLNF>>22)&0xFF));

#if (RTV_SRC_CLK_FREQ_KHz == 19200)
	RTV_REG_SET(0x22, ((dwPLLNF>>14)&0xC0));
	RTV_REG_SET(0x2A, 0x00);
	RTV_REG_SET(0x23, 0x00);

	WR2A = RTV_REG_GET(0x2A) & 0x3F;
	RTV_REG_SET(0x2A, (WR2A | 0x80));
	RTV_REG_SET(0x2A, (WR2A | 0xC0));
	RTV_DELAY_MS(1);
	RTV_REG_SET(0x2A, (WR2A | 0x80));
	RTV_REG_SET(0x2A, (WR2A | 0x00));
	
	RTV_REG_MASK_SET(0x2E,0x40,0x40);
    RTV_REG_MASK_SET(0x2E,0x40,0x00);
#else
	RTV_REG_SET(0x22, ((dwPLLNF>>14)&0xFF));
	RTV_REG_SET(0x2A, (dwPLLNF&0x3F));
	RTV_REG_SET(0x23, ((dwPLLNF>>6)&0xFF));

	WR2A = RTV_REG_GET(0x2A) & 0x3F;
	RTV_REG_SET(0x2A, (WR2A | 0x80));
	RTV_REG_SET(0x2A, (WR2A | 0xC0));
	RTV_DELAY_MS(1);
	RTV_REG_SET(0x2A, (WR2A | 0x80));
	RTV_REG_SET(0x2A, (WR2A | 0x00));

#endif

	do {
	RTV_DELAY_MS(1);

		RD15 = RTV_REG_GET(0x10);
		if ((RD15 & 0x20) == 0x20)
			break;
		else {
#if (RTV_SRC_CLK_FREQ_KHz == 19200)
			RTV_REG_SET(0x2A, (WR2A | 0x80)); 
			RTV_REG_SET(0x2A, (WR2A | 0xC0)); 
			RTV_DELAY_MS(1);
			RTV_REG_SET(0x2A, (WR2A | 0x80)); 
			RTV_REG_SET(0x2A, (WR2A | 0x00)); 
			
			RTV_REG_MASK_SET(0x2E,0x40,0x40);
			RTV_REG_MASK_SET(0x2E,0x40,0x00);

#else
			RTV_REG_SET(0x2A, (WR2A | 0x80)); 
			RTV_REG_SET(0x2A, (WR2A | 0xC0)); 
			RTV_DELAY_MS(1);
			RTV_REG_SET(0x2A, (WR2A | 0x80)); 
			RTV_REG_SET(0x2A, (WR2A | 0x00)); 
#endif
		}
	} while (--PLL_Verify_cnt);

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x10, 0x01, 0x01);
	RTV_REG_MASK_SET(0x10, 0x01, 0x00);

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_MASK_SET(0xFB, 0x01, 0x01); 
	RTV_REG_MASK_SET(0xFB, 0x01, 0x00);

	if (PLL_Verify_cnt == 0) {
		RTV_DBGMSG2("[rtvRF_SetFrequency] (%u/%u) PLL unlocked!\n",
					nChNum, dwChFreqKHz);

		nRet = RTV_PLL_UNLOCKED;
		goto RF_SET_FREQ_EXIT;
	}

	rtv_UpdateAdj();

RF_SET_FREQ_EXIT:
	g_fRtvChannelChange = FALSE; 
		 
	return nRet;
}


INT rtvRF_Initilize(E_RTV_TV_MODE_TYPE eTvMode)
{
	UINT nNumTblEntry = 0;
	const RTV_REG_INIT_INFO *ptInitTbl = NULL;
			
	g_fRtvChannelChange = FALSE;
	g_eRtvAdcClkFreqType = MAX_NUM_RTV_ADC_CLK_FREQ_TYPE;
	g_nLnaTuneVal = 0xFF;
	
	ptInitTbl = t_ISDBT_INIT;
	nNumTblEntry = sizeof(t_ISDBT_INIT) / sizeof(RTV_REG_INIT_INFO);
	
	RTV_REG_MAP_SEL(RF_PAGE);
	do {
		RTV_REG_SET(ptInitTbl->bReg, ptInitTbl->bVal);
		ptInitTbl++;
	} while (--nNumTblEntry);

#if (RTV_SRC_CLK_FREQ_KHz == 19200)
	RTV_REG_MASK_SET(0x2B, 0xF0, 4<<4); //PLLR
	RTV_REG_MASK_SET(0x2E, 0x80, 0x00);//DTHEN =0;

	RTV_REG_MASK_SET(0x52, 0x20, 0x00); //EN_DIG_VCOTEMPCON	0
	RTV_REG_MASK_SET(0x2B, 0x03, 0x02); //SEL_VP  2
	RTV_REG_MASK_SET(0x2C, 0xF0, 0xF0); //BWCALI  F
	RTV_REG_MASK_SET(0x52, 0x0F, 0x09); //ICON_VCO_I2C  9
	RTV_REG_MASK_SET(0xB1, 0xE0, 0x60); //ICONDIVFM2  3
	RTV_REG_MASK_SET(0x2D, 0xF8, 0x00); //CP_COMP  0
	RTV_REG_MASK_SET(0x56, 0x0C, 0x04); //BM_VCOCORE_I2C  1
	RTV_REG_MASK_SET(0x88, 0xE0, 0x80); //ICONDIV1  4
	RTV_REG_MASK_SET(0x5D, 0xE0, 0x00); //LDO_OUTSEL  0
	RTV_REG_SET(0x94, 0x43);
	RTV_REG_SET(0x95, 0x72);
	RTV_REG_SET(0x96, 0x43);
	RTV_REG_SET(0x97, 0x40);
	RTV_REG_SET(0x98, 0xBF);
	RTV_REG_SET(0x99, 0xDE);
	RTV_REG_SET(0x9A, 0x3D);
	RTV_REG_SET(0x9B, 0x24);
	RTV_REG_SET(0x9C, 0x43);
	RTV_REG_SET(0x9D, 0x2A);
	RTV_REG_SET(0x9E, 0xC1);
	RTV_REG_SET(0x9F, 0x97);
	RTV_REG_SET(0xA6, 0x37);
	RTV_REG_SET(0xA7, 0x1E);
#else
	RTV_REG_MASK_SET(0x2B, 0xF0, 1<<4); //PLLR
	RTV_REG_MASK_SET(0x2E, 0x80,0x80);//DTHEN =1;

	RTV_REG_MASK_SET(0x52, 0x20, 0x00); //EN_DIG_VCOTEMPCON	0
	RTV_REG_MASK_SET(0x2B, 0x03, 0x02); //SEL_VP  2
	RTV_REG_MASK_SET(0x2C, 0xF0, 0x80); //BWCALI  8
	RTV_REG_MASK_SET(0x52, 0x0F, 0x09); //ICON_VCO_I2C  9
	RTV_REG_MASK_SET(0xB1, 0xE0, 0x60); //ICONDIVFM2  3
	RTV_REG_MASK_SET(0x2D, 0xF8, 0xF8); //CP_COMP  1F
	RTV_REG_MASK_SET(0x56, 0x0C, 0x04); //BM_VCOCORE_I2C  1
	RTV_REG_MASK_SET(0x88, 0xE0, 0x80); //ICONDIV1  4
	RTV_REG_MASK_SET(0x5D, 0xE0, 0xC0); //LDO_OUTSEL  6
	RTV_REG_SET(0x94,0xC3); 	
	RTV_REG_SET(0x95,0x70); 	
	RTV_REG_SET(0x96,0x43); 		
	RTV_REG_SET(0x97,0x7F); 	
	RTV_REG_SET(0x98,0xC1); 		
	RTV_REG_SET(0x99,0x5C); 	
	RTV_REG_SET(0x9A,0xBF); 			
	RTV_REG_SET(0x9B,0xC6); 	
	RTV_REG_SET(0x9C,0x43); 			
	RTV_REG_SET(0x9D,0x96); 	
	RTV_REG_SET(0x9E,0xC1); 				
	RTV_REG_SET(0x9F,0xD7); 	
	RTV_REG_SET(0xA6,0x3B); 					
	RTV_REG_SET(0xA7,0x11); 
#endif

	return RTV_SUCCESS;
}

