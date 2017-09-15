/******************************************************************************
* (c) COPYRIGHT 2010 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE 	  : RAONTECH TV internal header file. 
*
* FILENAME    : raontv_internal.h
*
* DESCRIPTION : 
*		All the declarations and definitions necessary for the RAONTECH TV driver.
*
******************************************************************************/
/******************************************************************************
* REVISION HISTORY
*
*    DATE	  	  NAME				REMARKS
* ----------  -------------    ------------------------------------------------
* 07/26/2013  Yang, Maverick   Created.
******************************************************************************/

#ifndef __RAONTV_INTERNAL_H__
#define __RAONTV_INTERNAL_H__

#ifdef __cplusplus 
extern "C"{ 
#endif  

#include "raontv.h"


// Do not modify the order!
typedef enum
{	
	RTV_TV_MODE_TDMB   = 0,     // Band III  Korea
	RTV_TV_MODE_DAB_B3 = 1,      // Band III
	RTV_TV_MODE_DAB_L  = 2,      // L-Band		
	RTV_TV_MODE_1SEG   = 3, // UHF
	RTV_TV_MODE_FM     = 4,       // FM
	MAX_NUM_RTV_MODE
} E_RTV_TV_MODE_TYPE;


typedef struct
{
	U8	bReg;
	U8	bVal;
} RTV_REG_INIT_INFO;

struct RTV_ADC_CFG_INFO {
	U32	dwTNCO;
	U32	dwPNCO1;
	U32	dwPNCO2;
	U32	dwCFREQGAIN;
	U16 dwGAIN;
};

typedef struct
{
	U8	bReg;
	U8  bMask;
	U8	bVal;
} RTV_REG_MASK_INFO;

#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
	#if defined(RTV_TSIF_CLK_SPEED_DIV_2) // 12MHz
		#define RTV_COMM_CON47_CLK_SEL	2
	#elif defined(RTV_TSIF_CLK_SPEED_DIV_4) // 6MHz
		#define RTV_COMM_CON47_CLK_SEL	3
	#elif defined(RTV_TSIF_CLK_SPEED_DIV_6) // 3MHz
		#define RTV_COMM_CON47_CLK_SEL	4
	#elif defined(RTV_TSIF_CLK_SPEED_DIV_8) // 1.5MHz
		#define RTV_COMM_CON47_CLK_SEL	5
	#else
		#error "Code not present"
	#endif
#endif

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#if defined(RTV_INTR_POLARITY_LOW_ACTIVE)
		#define SPI_INTR_POL_ACTIVE	0x00
	#elif defined(RTV_INTR_POLARITY_HIGH_ACTIVE)
		#define SPI_INTR_POL_ACTIVE	(1<<7)
	#endif
#else
	#if defined(RTV_INTR_POLARITY_LOW_ACTIVE)
		#define I2C_INTR_POL_ACTIVE 0x08 /* level low */
	#elif defined(RTV_INTR_POLARITY_HIGH_ACTIVE)
		#define I2C_INTR_POL_ACTIVE 0x18 /* level high */
	#endif
#endif

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#if (RTV_SRC_CLK_FREQ_KHz == 4000) /* FPGA */
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
		#define RTV_SPI_INTR_DEACT_PRD_VAL	((6<<4)|3)

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


#define SPI_OVERFLOW_INTR       0x02
#define SPI_UNDERFLOW_INTR      0x20
#define SPI_THRESHOLD_INTR      0x08
#define SPI_INTR_BITS (SPI_THRESHOLD_INTR|SPI_UNDERFLOW_INTR|SPI_OVERFLOW_INTR)

#define MODE1 2 		
#define MODE2 1
#define MODE3 0


#define MAP_SEL_REG 	0x03

#define OFDM_PAGE       0x07
#define FEC_PAGE        0x03
#define HOST_PAGE       0x00

#define SPI_CTRL_PAGE   0x0E
#define RF_PAGE         0x0F
#define OFDM_E_CON      (0x10)

#define SPI_MEM_PAGE	0xFF /* Temp value. > 15 */


#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#define RTV_REG_MAP_SEL(page)	do {g_bRtvPage = page;} while(0)
	#define RTV_REG_GET_MAP_SEL	g_bRtvPage
#else
	#define RTV_REG_MAP_SEL(page)\
		do {\
			RTV_REG_SET(MAP_SEL_REG, page);\
			g_bRtvPage = page;\
		} while (0)

	#define RTV_REG_GET_MAP_SEL\
		(RTV_REG_GET(MAP_SEL_REG))
#endif

#define RTV_TS_STREAM_DISABLE_DELAY		20 // ms

// ISDB-T Channel 
#define ISDBT_CH_NUM_START__JAPAN			13
#define ISDBT_CH_NUM_END__JAPAN				62
#define ISDBT_CH_FREQ_START__JAPAN			473143
#define ISDBT_CH_FREQ_STEP__JAPAN			6000

#define ISDBT_CH_NUM_START__BRAZIL			14
#define ISDBT_CH_NUM_END__BRAZIL			69
#define ISDBT_CH_FREQ_START__BRAZIL			473143
#define ISDBT_CH_FREQ_STEP__BRAZIL			6000

#define ISDBT_CH_NUM_START__ARGENTINA		14
#define ISDBT_CH_NUM_END__ARGENTINA			69
#define ISDBT_CH_FREQ_START__ARGENTINA		473143
#define ISDBT_CH_FREQ_STEP__ARGENTINA		6000

extern volatile E_RTV_ADC_CLK_FREQ_TYPE g_eRtvAdcClkFreqType;
extern E_RTV_COUNTRY_BAND_TYPE g_eRtvCountryBandType;


/* Use SPI/EBI2 interrupt handler to prevent the changing of register map. */
extern volatile BOOL g_fRtvChannelChange;
extern BOOL g_fRtvStreamEnabled;

extern U8 g_bRtvIntrMaskRegL;

extern U8 g_bAdjId;
extern U8 g_bAdjSat;
extern U8 g_bAdjRefL;

extern UINT g_nRtvMscThresholdSize;

#define FM_MIN_FREQ_KHz			75950 // See PLL table in raontv_rf_pll_data_fm.h
#define FM_MAX_FREQ_KHz			108050
#define FM_SCAN_STEP_FREQ_KHz		(RTV_FM_CH_STEP_FREQ_KHz/2)

/*=============================================================================
*
* Common inline functions.
*
*============================================================================*/

static INLINE void rtv_SetupInterruptThreshold(void)
{
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
	RTV_REG_SET(0x23, g_nRtvMscThresholdSize/188);
#endif
}

// Pause straem
static INLINE void rtv_StreamDisable(E_RTV_TV_MODE_TYPE eTvMode)
{
	if (g_fRtvStreamEnabled) {
		RTV_REG_MAP_SEL(FEC_PAGE);
		RTV_REG_SET(0xA8, 0x80);	
		RTV_REG_SET(0xAA, 0x80);

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
		RTV_REG_MAP_SEL(SPI_CTRL_PAGE);

		/* Disable interrupts. */
		g_bRtvIntrMaskRegL |= SPI_INTR_BITS;
		RTV_REG_SET(0x24, g_bRtvIntrMaskRegL);

		/* To clear interrupt and data. */
		RTV_REG_SET(0x2A, 1);
		RTV_REG_SET(0x2A, 0);
#endif
	    g_fRtvStreamEnabled = FALSE; 
	}
}

/* Enable the stream path forcely for ISDB-T and FM only! */
static INLINE void rtv_StreamEnable(void)
{
	if (!g_fRtvStreamEnabled) {
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)				
		rtv_SetupInterruptThreshold();

		/* Enable SPI interrupts */
		g_bRtvIntrMaskRegL &= ~(SPI_INTR_BITS);
		RTV_REG_SET(0x24, g_bRtvIntrMaskRegL);
#endif
		RTV_REG_MAP_SEL(FEC_PAGE);
		RTV_REG_SET(0xA8, 0x81);
		RTV_REG_SET(0xAA, 0x81);

		g_fRtvStreamEnabled = TRUE;
	}
}

//#define DEBUG_UPDATEADJ
static INLINE void rtv_UpdateAdj(void)
{
	U8 bAdjRssId;

	RTV_REG_MAP_SEL(RF_PAGE);
	bAdjRssId = RTV_REG_GET(0x15);
	if ((bAdjRssId > g_bAdjId) && (g_bAdjRefL != 0x4F))
		g_bAdjRefL =g_bAdjRefL - 0x08;
	else if ((bAdjRssId < g_bAdjSat) && (g_bAdjRefL != 0x6F))
		g_bAdjRefL = g_bAdjRefL + 0x08;
	RTV_REG_SET(0x44, g_bAdjRefL);

#if defined(DEBUG_UPDATEADJ)
	RTV_DBGMSG3("[rtv_UpdateAdj] ID(0x%02X), WARN(0x%02X) SAT(0x%02X), ",
			bAdjRssId, g_bAdjId,g_bAdjSat);
	RTV_DBGMSG1("REF_L(0x%02X) \n", g_bAdjRefL);
#endif
}

#ifdef RTV_IF_TSIF
//#define SYNC_POLARITY_ACTIVE_LOW
//#define SYNC_LENGTH_1BIT
#endif

static INLINE void rtv_ConfigureTsifFormat(void)
{
	RTV_REG_MAP_SEL(FEC_PAGE);
	
#if defined(RTV_IF_MPEG2_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE) 
  #if defined(RTV_TSIF_FORMAT_1)
 	RTV_REG_SET(0x9F, 0x00);
	RTV_REG_SET(0xA4, 0x0B);  
	RTV_REG_SET(0xA5, 0x08);   
  #elif defined(RTV_TSIF_FORMAT_2)
	RTV_REG_SET(0x9F, 0x10);
	RTV_REG_SET(0xA4, 0x0B);  
	RTV_REG_SET(0xA5, 0x08);
  #elif defined(RTV_TSIF_FORMAT_3)
	RTV_REG_SET(0x9F, 0x00);
	RTV_REG_SET(0xA4, 0x0B);  
	RTV_REG_SET(0xA5, 0x00);
  #elif defined(RTV_TSIF_FORMAT_4)
	RTV_REG_SET(0x9F, 0x10);
	RTV_REG_SET(0xA4, 0x0B);  
	RTV_REG_SET(0xA5, 0x00);
  #else
	#error "Code not present"
  #endif

#elif defined(RTV_IF_QUALCOMM_TSIF)
  #if defined(RTV_TSIF_FORMAT_1)
	RTV_REG_SET(0x9F, 0x00);
	RTV_REG_SET(0xA4, 0x0B);  
	RTV_REG_SET(0xA5, 0x08);  
  #elif defined(RTV_TSIF_FORMAT_2)
	RTV_REG_SET(0x9F, 0x00);
	RTV_REG_SET(0xA4, 0x0B);  
	RTV_REG_SET(0xA5, 0x00); 
  #elif defined(RTV_TSIF_FORMAT_3)
	RTV_REG_SET(0x9F, 0x00);
	RTV_REG_SET(0xA4, 0x0B);  
	RTV_REG_SET(0xA5, 0x00); 
  #elif defined(RTV_TSIF_FORMAT_4)
	RTV_REG_SET(0x9F, 0x00);
	RTV_REG_SET(0xA4, 0x0B);  
	RTV_REG_SET(0xA5, 0x04);
  #elif defined(RTV_TSIF_FORMAT_5)
	RTV_REG_SET(0x9F, 0x00);
	RTV_REG_SET(0xA4, 0x0B);  
	RTV_REG_SET(0xA5, 0x0C);   
  #else
	#error "Code not present"
  #endif
#endif	

#ifdef SYNC_LENGTH_1BIT
	RTV_REG_MASK_SET(0xA4,0x80,0x80);
#endif

#ifdef SYNC_POLARITY_ACTIVE_LOW
	RTV_REG_MASK_SET(0x9F,0x04,0x04);
#endif
}

/*==============================================================================
 * External functions for RAONTV driver core.
 *============================================================================*/ 
void rtv_ConfigureHostIF(void);
INT  rtv_InitSystem(E_RTV_TV_MODE_TYPE eTvMode, E_RTV_ADC_CLK_FREQ_TYPE eAdcClkFreqType);

#ifdef __cplusplus 
} 
#endif 

#endif /* __RAONTV_INTERNAL_H__ */


