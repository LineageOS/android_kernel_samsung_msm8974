/******************************************************************************
* (c) COPYRIGHT 2013 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE		: MTV23x internal header file.
*
* FILENAME	: mtv23x_internal.h
*
* DESCRIPTION	:
*		All the declarations and definitions necessary for
*		the MTV23x TV driver.
*
******************************************************************************/
/******************************************************************************
* REVISION HISTORY
*
*    DATE         NAME          REMARKS
* ----------  -------------    ------------------------------------------------
* 03/03/2013  Yang, Maverick       Created.
******************************************************************************/

#ifndef __MTV23X_INTERNAL_H__
#define __MTV23X_INTERNAL_H__

#include "mtv23x.h"

#ifdef __cplusplus
extern "C"{
#endif

#if defined(RTV_IF_SPI) || defined(RTV_IF_SPI_TSIFx) || defined(RTV_IF_EBI2)
	#if defined(RTV_INTR_POLARITY_LOW_ACTIVE)
		#define SPI_INTR_POL_ACTIVE	0x00
	#elif defined(RTV_INTR_POLARITY_HIGH_ACTIVE)
		#define SPI_INTR_POL_ACTIVE	(1<<3)
	#endif
#endif

struct RTV_REG_INIT_INFO {
	U8	bReg;
	U8	bVal;
};

struct RTV_REG_MASK_INFO {
	U8	bReg;
	U8  bMask;
	U8	bVal;
};

struct RTV_ADC_CFG_INFO {
	U8 bData2A;
	U8 bData6E;
	U8 bData70;
	U8 bData71;
	U8 bData75;
	U32 dwTNCO;
	U32 dwPNCO;
	U32 dwCFREQGAIN;
	U16 dwGAIN;
};

#if defined(RTV_IF_TSIF_0) || defined(RTV_IF_TSIF_1)\
|| defined(RTV_IF_SPI_SLAVE)
	#if defined(RTV_TSIF_SPEED_500_kbps)
		#define RTV_FEC_TSIF_OUT_SPEED	7
	#elif defined(RTV_TSIF_SPEED_1_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	6
	#elif defined(RTV_TSIF_SPEED_2_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	5
	#elif defined(RTV_TSIF_SPEED_4_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	4
	#elif defined(RTV_TSIF_SPEED_7_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	3
	#elif defined(RTV_TSIF_SPEED_15_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	2
	#elif defined(RTV_TSIF_SPEED_30_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	1
	#elif defined(RTV_TSIF_SPEED_60_Mbps)
		#define RTV_FEC_TSIF_OUT_SPEED	0
	#else
		#error "Code not present"
	#endif
#endif

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#if (RTV_SRC_CLK_FREQ_KHz == 4000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	0x51

	#elif (RTV_SRC_CLK_FREQ_KHz == 13000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 16000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 16384)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 18000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 19200) /* 1 clk: 52.08ns */
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 24000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 24576) /* 1 clk: 40.7ns */
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((7<<4)|2)/*about 10us*/

	#elif (RTV_SRC_CLK_FREQ_KHz == 26000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 27000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 32000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((7<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 32768)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 36000) /* 1clk: 27.7 ns */
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((7<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 38400)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 40000)
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

	#elif (RTV_SRC_CLK_FREQ_KHz == 48000) /* 1clk: 20.8 ns */
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((9<<4)|0)
	#else
		#error "Code not present"
	#endif
#endif /* #if defined(RTV_IF_SPI) */

#if (RTV_TSP_XFER_SIZE == 188)
	#define N_DATA_LEN_BITVAL	0x02
	#define ONE_DATA_LEN_BITVAL	0x00
#elif (RTV_TSP_XFER_SIZE == 204)
	#define N_DATA_LEN_BITVAL	0x03
	#define ONE_DATA_LEN_BITVAL	(1<<5)
#endif

#define SPI_OVERFLOW_INTR       0x02
#define SPI_UNDERFLOW_INTR      0x20
#define SPI_THRESHOLD_INTR      0x08
#define SPI_INTR_BITS (SPI_THRESHOLD_INTR|SPI_UNDERFLOW_INTR|SPI_OVERFLOW_INTR)

#define TOP_PAGE	0x00
#define HOST_PAGE	0x00
#define OFDM_PAGE	0x01
#define SHAD_PAGE	0x02
#define FEC_PAGE	0x03
#define DATA_PAGE	0x04
#define FEC2_PAGE	0x06
#define LPOFDM_PAGE	0x07
#define SPI_CTRL_PAGE	0x0E
#define RF_PAGE		0x0F
#define SPI_MEM_PAGE	0xFF /* Temp value. > 15 */

#define MAP_SEL_REG	0x03
#define MAP_SEL_VAL(page)		(page)

#if defined(RTV_IF_SPI) ||  defined(RTV_IF_SPI_TSIFx) || defined(RTV_IF_EBI2)
	#define RTV_REG_MAP_SEL(page)	g_bRtvPage = page
	#define RTV_REG_GET_MAP_SEL	g_bRtvPage
#else
	#define RTV_REG_MAP_SEL(page) \
		do  {\
			RTV_REG_SET(MAP_SEL_REG, MAP_SEL_VAL(page));\
		} while (0)

	#define RTV_REG_GET_MAP_SEL \
		(RTV_REG_GET(MAP_SEL_REG))
#endif

extern U8 g_bRtvIntrMaskReg;
extern UINT g_nRtvThresholdSize;

/*==============================================================================
 *
 * Common inline functions.
 *
 *============================================================================*/

static INLINE void rtv_SetupInterruptThreshold(void)
{
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
	RTV_REG_SET(0x23, (g_nRtvThresholdSize/188)/4);
#endif
}

/* Forward prototype. */
static INLINE void rtv_DisableTSIF(void)
{
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xA8, 0x80);

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_SET(0xAA, 0x80);

	RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
	/* Disable interrupts. */
	g_bRtvIntrMaskReg |= SPI_INTR_BITS;
	RTV_REG_SET(0x24, g_bRtvIntrMaskReg);

	/* To clear interrupt and data. */
	RTV_REG_SET(0x2A, 1);
	RTV_REG_SET(0x2A, 0);
#else
	#if defined(RTV_IF_TSIF_0)
	RTV_REG_SET(0xAA, 0x80);
	#endif

	#if defined(RTV_IF_TSIF_1)
	RTV_REG_SET(0xAB, 0x80);
	#endif
#endif

}

static INLINE void rtv_EnableTSIF(void)
{
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	rtv_SetupInterruptThreshold();

	/* To clear interrupt and data. */
	RTV_REG_SET(0x2A, 1);
	RTV_REG_SET(0x2A, 0);

	/* Enable SPI interrupts */
	g_bRtvIntrMaskReg &= ~(SPI_INTR_BITS);
	RTV_REG_SET(0x24, g_bRtvIntrMaskReg);

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xA8, 0x87);
	RTV_REG_SET(0xAA, 0x87);
#else
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0xA8, 0x87);

	#if defined(DUAL_PORT_TSOUT_ENABLE)
		RTV_REG_SET(0xAA, 0x82); /* TS0 Layer A only */
		RTV_REG_SET(0xAB, 0x85); /* TS1 Layer B,C only */
	#else
		#if defined(RTV_IF_TSIF_0)
		RTV_REG_SET(0xAA, 0x87);
		#endif

		#if defined(RTV_IF_TSIF_1)
		RTV_REG_SET(0xAB, 0x87);
		#endif
	#endif
#endif

}

/* #define PRE_EXTEND_VALID_SIGNAL */

#if defined(RTV_IF_TSIF_0) || defined(RTV_IF_SPI_SLAVE)
static INLINE void rtv_ConfigureTsif0Format(void)
{
	U8 REG9F;
	RTV_REG_MAP_SEL(FEC_PAGE);
	REG9F = RTV_REG_GET(0x9F) & 0xAA;

#if defined(RTV_TSIF_FORMAT_0) /* EN_high, CLK_rising */
	RTV_REG_SET(0x9F, (REG9F | 0x00));
	RTV_REG_SET(0xA5, 0x08);
#elif defined(RTV_TSIF_FORMAT_1) /* EN_high, CLK_falling */
	RTV_REG_SET(0x9F, (REG9F | 0x00));
	RTV_REG_SET(0xA5, 0x00);
#elif defined(RTV_TSIF_FORMAT_2) /* EN_low, CLK_rising */
	RTV_REG_SET(0x9F, (REG9F | 0x10));
	RTV_REG_SET(0xA5, 0x08);
#elif defined(RTV_TSIF_FORMAT_3) /* EN_low, CLK_falling */
	RTV_REG_SET(0x9F, (REG9F | 0x10));
	RTV_REG_SET(0xA5, 0x00);
#elif defined(RTV_TSIF_FORMAT_4) /* EN_high, CLK_rising + 1CLK add */
	RTV_REG_SET(0x9F, (REG9F | 0x00));
	RTV_REG_SET(0xA5, 0x0C);
#elif defined(RTV_TSIF_FORMAT_5) /* EN_high, CLK_falling + 1CLK add */
	RTV_REG_SET(0x9F, (REG9F | 0x00));
	RTV_REG_SET(0xA5, 0x04);
#elif defined(RTV_TSIF_FORMAT_6) || defined(RTV_TSIF_FORMAT_7)
	#error "RTV_TSIF_FORMAT_6/7 is not suported at RTV_IF_TSIF_0 Mode"
#else
	#error "Code not present"
#endif
	RTV_REG_SET(0xA4, 0x89);
	RTV_REG_SET(0xA8, 0x87);
	RTV_REG_SET(0xA9, (0xB8|RTV_FEC_TSIF_OUT_SPEED));

#if defined(RTV_ERROR_TSP_OUTPUT_DISABLE)
	RTV_REG_MASK_SET(0xA5, 0x40, 0x40);
#endif
#if defined(RTV_NULL_PID_TSP_OUTPUT_DISABLE)
	RTV_REG_MASK_SET(0xA5, 0x20, 0x20);
#endif
#if defined(RTV_NULL_PID_GENERATE)
	RTV_REG_MASK_SET(0xA4, 0x02, 0x02);
#endif

#if defined(RTV_IF_CSI656_RAW_8BIT_ENABLE)
	RTV_REG_MASK_SET(0x9F, 0x0F, 0x05); /* One clock pre-add. */
	RTV_REG_SET(0x9D, 0x01); /* Sync signal One pre-move. */
#endif

#if defined(PRE_EXTEND_VALID_SIGNAL)
	RTV_REG_MASK_SET(0xA5, 0x10, 0x10);
#endif

#if defined(DUAL_PORT_TSOUT_ENABLE)
	RTV_REG_SET(0xAA, 0x82); /* TS0 Layer A only */
#else
	RTV_REG_SET(0xAA, 0x87);
#endif
}
#endif /* #elif defined(RTV_IF_TSIF_0) || defined(RTV_IF_SPI_SLAVE) */

#if defined(RTV_IF_TSIF_1) || defined(RTV_IF_SPI_SLAVE)
static INLINE void rtv_ConfigureTsif1Format(void)
{
	 U8 REG9F;
	RTV_REG_MAP_SEL(FEC_PAGE);
	REG9F = RTV_REG_GET(0x9F) & 0x55;
#if defined(RTV_TSIF_FORMAT_0) /* EN_high, CLK_rising */
	RTV_REG_SET(0x9F, (REG9F | 0x00));
	RTV_REG_SET(0xA6, 0x88);
	RTV_REG_SET(0xA7, 0x48);

#elif defined(RTV_TSIF_FORMAT_1) /* EN_high, CLK_falling */
	RTV_REG_SET(0x9F, (REG9F | 0x00));
	RTV_REG_SET(0xA6, 0x88);
	RTV_REG_SET(0xA7, 0x40);

#elif defined(RTV_TSIF_FORMAT_2) /* EN_low, CLK_rising */
	RTV_REG_SET(0x9F, (REG9F | 0x20));
	RTV_REG_SET(0xA6, 0x88);
	RTV_REG_SET(0xA7, 0x48);

#elif defined(RTV_TSIF_FORMAT_3) /* EN_low, CLK_falling */
	RTV_REG_SET(0x9F, (REG9F | 0x20));
	RTV_REG_SET(0xA6, 0x88);
	RTV_REG_SET(0xA7, 0x40);

#elif defined(RTV_TSIF_FORMAT_4) /* EN_high, CLK_rising + 1CLK add */
	RTV_REG_SET(0x9F, (REG9F | 0x00));
	RTV_REG_SET(0xA6, 0x88);
	RTV_REG_SET(0xA7, 0x4C);

#elif defined(RTV_TSIF_FORMAT_5) /* EN_high, CLK_falling + 1CLK add */
	RTV_REG_SET(0x9F, (REG9F | 0x00));
	RTV_REG_SET(0xA6, 0x88);
	RTV_REG_SET(0xA7, 0x44);

#elif defined(RTV_TSIF_FORMAT_6) /* Parallel: EN_high, CLK_rising*/
	#if defined(RTV_IF_SPI_SLAVE)
	#error "RTV_TSIF_FORMAT_6 is not suported at RTV_IF_SPI_SLAVE Mode"
	#else
	RTV_REG_SET(0x9F, (REG9F | 0x00));
	RTV_REG_SET(0xA6, 0x80);
	RTV_REG_SET(0xA7, 0x48);
	#endif
#elif defined(RTV_TSIF_FORMAT_7) /* Parallel: EN_high, CLK_falling */
	#if defined(RTV_IF_SPI_SLAVE)
	#error "RTV_TSIF_FORMAT_7 is not suported at RTV_IF_SPI_SLAVE Mode"
	#else
	RTV_REG_SET(0x9F, (REG9F | 0x00));
	RTV_REG_SET(0xA6, 0x80);
	RTV_REG_SET(0xA7, 0x40);
	#endif
#else
	#error "Code not present"
#endif

	RTV_REG_MASK_SET(0xA4, 0x01, 0x01); /* TEI Enable */

#if defined(RTV_ERROR_TSP_OUTPUT_DISABLE)
	RTV_REG_MASK_SET(0xA5, 0x40, 0x40);
#endif

#if defined(RTV_NULL_PID_TSP_OUTPUT_DISABLE)
	RTV_REG_MASK_SET(0xA5, 0x20, 0x20);
#endif

#if defined(RTV_NULL_PID_GENERATE)
	RTV_REG_MASK_SET(0xA4, 0x02, 0x02);
#endif
	RTV_REG_SET(0xA8, 0x87);
	RTV_REG_SET(0xA9, (0xB8|RTV_FEC_TSIF_OUT_SPEED));

#if defined(RTV_IF_CSI656_RAW_8BIT_ENABLE)
	RTV_REG_MASK_SET(0x9F, 0x0F, 0x0A); /* One clock pre-add. */
	#if defined(RTV_TSIF_FORMAT_6)  || defined(RTV_TSIF_FORMAT_7)
	RTV_REG_SET(0x9E, 0x04); /* 4bit clock pre-move. */
	#else
	RTV_REG_SET(0x9E, 0x01); /* 8bit clock pre-move. */
	#endif
#endif

#if defined(PRE_EXTEND_VALID_SIGNAL)
	RTV_REG_MASK_SET(0xA7, 0x10, 0x10);
#endif

#if defined(DUAL_PORT_TSOUT_ENABLE)
	RTV_REG_SET(0xAB, 0x85); /* TS1 Layer B,C only */
#else
	RTV_REG_SET(0xAB, 0x87);
#endif
}
#endif /* #elif defined(RTV_IF_TSIF_1) || defined(RTV_IF_SPI_SLAVE) */

static INLINE int rtvRF_LockCheck(U8 bCheckBlock)
{
	INT i = 0;
	INT nRet = RTV_SUCCESS;
	U8 nLockCheck = 0;

	RTV_REG_MAP_SEL(RF_PAGE);

	switch (bCheckBlock) {
	case 0: /* O == RF Lock Check */
		for (i = 0; i < 10; i++) {
			nLockCheck = RTV_REG_GET(0x1B) & 0x02;
			if (nLockCheck)
				break;
			else
				RTV_DBGMSG1("[rtvRF_LockCheck]VCheck(%d)\n", i);

			RTV_DELAY_MS(1);
		}

		if (i == 10) {
			RTV_DBGMSG0("[rtvRF_LockCheck] VCO Pll unlocked!\n");
			nRet =  RTV_PLL_UNLOCKED;
		}
		break;

	case 1: /* CLK Synth Lock Check */
		for (i = 0; i < 10; i++) {
			nLockCheck = RTV_REG_GET(0x1B) & 0x01;
			if (nLockCheck)
				break;
			else
				RTV_DBGMSG1("[rtvRF_LockCheck]SCheck(%d)\n", i);

			RTV_DELAY_MS(1);
		}

		if (i == 10) {
			RTV_DBGMSG0("[rtvRF_LockCheck] ADC clock unlocked!\n");
			nRet =  RTV_ADC_CLK_UNLOCKED;
		}
		break;
	}

	return nRet;
}

extern BOOL g_fRtv1segLpMode;
extern enum E_RTV_SERVICE_TYPE g_eRtvServiceType;

#ifdef RTV_DUAL_DIVERISTY_ENABLE
extern enum E_RTV_SERVICE_TYPE g_eRtvServiceType_slave;
#endif

static INLINE void rtv_UpdateMon(void)
{
	if (g_fRtv1segLpMode) {
		RTV_REG_MAP_SEL(LPOFDM_PAGE);
		RTV_REG_MASK_SET(0x13, 0x80, 0x80);
		RTV_REG_MASK_SET(0x13, 0x80, 0x00);
	} else {
		RTV_REG_MAP_SEL(OFDM_PAGE);
		RTV_REG_MASK_SET(0x1B, 0x80, 0x80);
		RTV_REG_MASK_SET(0x1B, 0x80, 0x00);
	}

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_MASK_SET(0x11, 0x04, 0x04);
	RTV_REG_MASK_SET(0x11, 0x04, 0x00);
}

static INLINE void rtv_SoftReset(void)
{
	if (g_fRtv1segLpMode)
		RTV_REG_MAP_SEL(LPOFDM_PAGE);
	else
		RTV_REG_MAP_SEL(OFDM_PAGE);

	RTV_REG_MASK_SET(0x10, 0x01, 0x01);
	RTV_REG_MASK_SET(0x10, 0x01, 0x00);

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_MASK_SET(0xFB, 0x01, 0x01);
	RTV_REG_MASK_SET(0xFB, 0x01, 0x00);
}

static INLINE INT rtv_ServiceTypeSelect(enum E_RTV_SERVICE_TYPE eServiceType)
{
	INT nRet = RTV_SUCCESS;

	switch (eServiceType) {
#if defined(RTV_ISDBT_ENABLE)
	case RTV_SERVICE_UHF_ISDBT_1seg:
	case RTV_SERVICE_VHF_ISDBTmm_1seg:
	case RTV_SERVICE_VHF_ISDBTsb_1seg:
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x0B, 0x36);

		RTV_REG_SET(0x12, 0x08);
		RTV_REG_SET(0x21, 0x01);
		RTV_REG_SET(0x26, 0x00);

		RTV_REG_MAP_SEL(FEC_PAGE);
		RTV_REG_SET(0x20, 0x0C);

		RTV_REG_SET(0x23, 0xF0); /* Layer A */
#if !defined(RTV_IF_SPI) && !defined(RTV_IF_EBI2)
		RTV_REG_SET(0x24, 0x31);
		RTV_REG_SET(0x4F, 0x1F);
#endif
		RTV_REG_SET(0x44, 0x68);
		RTV_REG_SET(0x47, 0x40);

		RTV_REG_SET(0x53, 0x3E);
		RTV_REG_SET(0x21, 0x00);
		RTV_REG_SET(0x22, 0x00);
		RTV_REG_SET(0x5C, 0x10);
		RTV_REG_SET(0x5F, 0x10);
		RTV_REG_SET(0x77, 0x40);
		RTV_REG_SET(0x7A, 0x20);
		RTV_REG_SET(0x83, 0x10);
		RTV_REG_SET(0x96, 0x00);
		RTV_REG_SET(0xAE, 0x00);

		RTV_REG_SET(0xFC, 0x83);
		RTV_REG_SET(0xFF, 0x03);

#if 0
		RTV_REG_SET(0x44, 0x48);
		RTV_REG_SET(0x47, 0x00);
#endif
		g_fRtv1segLpMode = 1;
		break;
	case RTV_SERVICE_VHF_ISDBTsb_3seg:
		RTV_DBGMSG0("[rtvRF_SelectService] 3seg is not implemented\n");
		break;

	case RTV_SERVICE_UHF_ISDBT_13seg:
	case RTV_SERVICE_VHF_ISDBTmm_13seg:
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x0B, 0x96);

		RTV_REG_SET(0x12, 0x00);
		RTV_REG_SET(0x21, 0x00);
		RTV_REG_SET(0x26, 0xB8);

		RTV_REG_MAP_SEL(OFDM_PAGE);
		RTV_REG_SET(0x10, 0xD4);

		RTV_REG_MAP_SEL(FEC_PAGE);
		RTV_REG_SET(0x20, 0x00);
#ifdef RTV_DUAL_DIVERISTY_ENABLE
		RTV_REG_SET(0x21, 0x22);
		RTV_REG_SET(0x22, 0x22);
#else
		RTV_REG_SET(0x21, 0x21);
		RTV_REG_SET(0x22, 0x21);
#endif

#if 0
		RTV_REG_SET(0x23, 0x84);
		RTV_REG_SET(0x24, 0x31);
		RTV_REG_SET(0x4F, 0x1F);
#endif

		RTV_REG_SET(0x23, 0x90);
#if !defined(RTV_IF_SPI) && !defined(RTV_IF_EBI2)
		RTV_REG_SET(0x24, 0x01);
		RTV_REG_SET(0x4F, 0x00);
#endif
		RTV_REG_SET(0x44, 0x68);
		RTV_REG_SET(0x47, 0x40);

		RTV_REG_SET(0x53, 0x1E);
		RTV_REG_SET(0x5C, 0x11);
		RTV_REG_SET(0x5F, 0x11);
		RTV_REG_SET(0x77, 0x00);
		RTV_REG_SET(0x7A, 0x00);
		RTV_REG_SET(0x83, 0x00);
		RTV_REG_SET(0x96, 0x20);
		RTV_REG_SET(0xAE, 0x02);

		RTV_REG_SET(0xFC, 0x83);
		RTV_REG_SET(0xFF, 0x03);

#if 0
		RTV_REG_SET(0x44, 0xE8);
		RTV_REG_SET(0x47, 0x40);
#endif
		g_fRtv1segLpMode = 0;
		break;
#endif
#if defined(RTV_DVBT_ENABLE)
	case RTV_SERVICE_DVBT:
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x0B, 0x96);
		RTV_REG_SET(0x12, 0x00);
		RTV_REG_SET(0x21, 0x00);
		RTV_REG_SET(0x26, 0xB8);

		RTV_REG_MAP_SEL(DATA_PAGE);
		RTV_REG_SET(0xA2, 0x0E);
		RTV_REG_SET(0xA3, 0x0E);
		RTV_REG_SET(0xA7, 0x0D);
		RTV_REG_SET(0xA6, 0x0D);

		RTV_REG_MAP_SEL(OFDM_PAGE);
		RTV_REG_SET(0x10, 0xD6);

		RTV_REG_MAP_SEL(FEC_PAGE);
		RTV_REG_SET(0x20, 0x00);
#ifdef RTV_DUAL_DIVERISTY_ENABLE
		RTV_REG_SET(0x21, 0x22);
		RTV_REG_SET(0x22, 0x22);
		RTV_REG_SET(0x53, 0x03); //
#else
		RTV_REG_SET(0x21, 0x21);
		RTV_REG_SET(0x22, 0x21);
		RTV_REG_SET(0x53, 0x1E);
#endif

		RTV_REG_SET(0x23, 0xF0); /* Layer A */
#if !defined(RTV_IF_SPI) && !defined(RTV_IF_EBI2)
		RTV_REG_SET(0x24, 0x11);
		RTV_REG_SET(0x4F, 0x07);
#endif
		RTV_REG_SET(0x44, 0xE8);
		RTV_REG_SET(0x47, 0x40);

		RTV_REG_SET(0x5C, 0x10);
		RTV_REG_SET(0x5F, 0x10);
		RTV_REG_SET(0x77, 0x00);
		RTV_REG_SET(0x7A, 0x00);
		RTV_REG_SET(0x83, 0x00);
		RTV_REG_SET(0x96, 0x20);
		RTV_REG_SET(0xAE, 0x02);

		RTV_REG_SET(0xFC, 0x83);
		RTV_REG_SET(0xFF, 0xFF);
		
#if 0
		RTV_REG_SET(0x44, 0xE8);
		RTV_REG_SET(0x47, 0x40);
#endif
		g_fRtv1segLpMode = 0;
		break;
#endif
	default:
		nRet = RTV_INVAILD_SERVICE_TYPE;
	}

	return nRet;
}

/*=============================================================================
* External functions for RAONTV driver core.
*============================================================================*/
INT rtv_InitSystem(void);

#ifdef __cplusplus
}
#endif

#endif /* __MTV23X_INTERNAL_H__ */


