/******************************************************************************
* (c) COPYRIGHT 2013 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE      : RAONTECH TV device driver API header file.
*
* FILENAME   : raontv.h
*
* DESCRIPTION:
*  This file contains types and declarations associated with the RAONTECH
*  TV Services.
*
******************************************************************************/
/******************************************************************************
* REVISION HISTORY
*
*    DATE	  	  NAME				REMARKS
* ----------  -------------    ------------------------------------------------
* 07/26/2013  Yang, Maverick   Created.
******************************************************************************/

#ifndef __RAONTV_H__
#define __RAONTV_H__

#ifdef __cplusplus 
extern "C"{ 
#endif  

#include "raontv_port.h"

#define RAONTV_CHIP_ID		0x8A
#define MTV222_SPI_CMD_SIZE		3

/*==============================================================================
 *
 * Common definitions and types.
 *
 *============================================================================*/
#ifndef NULL
	#define NULL    	0
#endif

#ifndef FALSE
	#define FALSE		0
#endif

#ifndef TRUE
	#define TRUE		1
#endif

#ifndef MAX
	#define MAX(a, b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
	#define MIN(a, b)    (((a) < (b)) ? (a) : (b))
#endif

#ifndef ABS
	#define ABS(x) 		 (((x) < 0) ? -(x) : (x))
#endif


#define	RTV_TS_PACKET_SIZE		188


/* Error codes. */
#define RTV_SUCCESS				0
#define RTV_INVAILD_COUNTRY_BAND		-1
#define RTV_UNSUPPORT_ADC_CLK			-2
#define RTV_INVAILD_TV_MODE			-3
#define RTV_CHANNEL_NOT_DETECTED		-4
#define RTV_INSUFFICIENT_CHANNEL_BUF		-5
#define RTV_INVAILD_FREQ			-6
#define RTV_INVAILD_SUB_CHANNEL_ID		-7 // for T-DMB and DAB
#define RTV_NO_MORE_SUB_CHANNEL			-8 // for T-DMB and DAB
#define RTV_ALREADY_OPENED_SUB_CHANNEL_ID	-9 // for T-DMB and DAB
#define RTV_NOT_OPENED_SUB_CHANNEL_ID		-10 // for T-DMB and DAB
#define RTV_INVAILD_THRESHOLD_SIZE		-11 
#define RTV_POWER_ON_CHECK_ERROR		-12 
#define RTV_PLL_UNLOCKED			-13 
#define RTV_ADC_CLK_UNLOCKED			-14 


typedef enum
{
	RTV_COUNTRY_BAND_JAPAN = 0,
	RTV_COUNTRY_BAND_KOREA,		
	RTV_COUNTRY_BAND_BRAZIL,
	RTV_COUNTRY_BAND_ARGENTINA 
} E_RTV_COUNTRY_BAND_TYPE;


// Do not modify the order!
typedef enum
{
	RTV_ADC_CLK_FREQ_8_MHz = 0,
	RTV_ADC_CLK_FREQ_8_192_MHz,
	RTV_ADC_CLK_FREQ_9_MHz,
	RTV_ADC_CLK_FREQ_9_6_MHz,
	MAX_NUM_RTV_ADC_CLK_FREQ_TYPE
} E_RTV_ADC_CLK_FREQ_TYPE;


// Modulation
typedef enum
{
	RTV_MOD_DQPSK = 0,
	RTV_MOD_QPSK,
	RTV_MOD_16QAM,
	RTV_MOD_64QAM
} E_RTV_MODULATION_TYPE;

typedef enum
{
	RTV_CODE_RATE_1_2 = 0,
	RTV_CODE_RATE_2_3,
	RTV_CODE_RATE_3_4,
	RTV_CODE_RATE_5_6,
	RTV_CODE_RATE_7_8
} E_RTV_CODE_RATE_TYPE;


enum E_RTV_SERVICE_TYPE {
	RTV_SERVICE_INVALID = -1,
	RTV_SERVICE_UHF_ISDBT_1seg = 0, /* ISDB-T 1seg */
	MAX_NUM_RTV_SERVICE
};

enum E_RTV_BANDWIDTH_TYPE {
     RTV_BW_MODE_430KHZ = 0, //1SEG at 6MHz BW
     RTV_BW_MODE_500KHZ,	//1SEG at 7MHz BW
     RTV_BW_MODE_571KHZ,	//1SEG at 8MHz BW
	 MAX_NUM_RTV_BW_MODE_TYPE
};

/*==============================================================================
 *
 * ISDB-T definitions, types and APIs.
 *
 *============================================================================*/
static INLINE UINT RTV_ISDBT_FREQ2CHNUM(E_RTV_COUNTRY_BAND_TYPE eRtvCountryBandType, U32 dwFreqKHz)
{
	switch (eRtvCountryBandType) {
		case RTV_COUNTRY_BAND_JAPAN:
			return ((dwFreqKHz - 395143) / 6000);
			
		case RTV_COUNTRY_BAND_BRAZIL:
		case RTV_COUNTRY_BAND_ARGENTINA: 
			return (((dwFreqKHz - 395143) / 6000) + 1);
			
		default:
			return 0xFFFF;
	}
}


#define RTV_ISDBT_OFDM_LOCK_MASK	0x1
#define RTV_ISDBT_TMCC_LOCK_MASK	0x2
#define RTV_ISDBT_CHANNEL_LOCK_OK	(RTV_ISDBT_OFDM_LOCK_MASK|RTV_ISDBT_TMCC_LOCK_MASK)

#define RTV_ISDBT_BER_DIVIDER		100000
#define RTV_ISDBT_CNR_DIVIDER		1000
#define RTV_ISDBT_RSSI_DIVIDER		10


typedef enum
{
	RTV_ISDBT_SEG_1 = 0,
	RTV_ISDBT_SEG_3
} E_RTV_ISDBT_SEG_TYPE;

typedef enum
{
	RTV_ISDBT_MODE_1 = 0, // 2048
	RTV_ISDBT_MODE_2,	  // 4096
	RTV_ISDBT_MODE_3      // 8192 fft
} E_RTV_ISDBT_MODE_TYPE;

typedef enum
{
	RTV_ISDBT_GUARD_1_32 = 0, /* 1/32 */
	RTV_ISDBT_GUARD_1_16,     /* 1/16 */
	RTV_ISDBT_GUARD_1_8,      /* 1/8 */
	RTV_ISDBT_GUARD_1_4       /* 1/4 */
} E_RTV_ISDBT_GUARD_TYPE;


typedef enum
{
	RTV_ISDBT_INTERLV_0 = 0,
	RTV_ISDBT_INTERLV_1,
	RTV_ISDBT_INTERLV_2,
	RTV_ISDBT_INTERLV_4,
	RTV_ISDBT_INTERLV_8,
	RTV_ISDBT_INTERLV_16,
	RTV_ISDBT_INTERLV_32
} E_RTV_ISDBT_INTERLV_TYPE;


// for Layer A.
typedef struct
{
	E_RTV_ISDBT_SEG_TYPE		eSeg;
	E_RTV_ISDBT_MODE_TYPE		eTvMode;
	E_RTV_ISDBT_GUARD_TYPE		eGuard;
	E_RTV_MODULATION_TYPE		eModulation;
	E_RTV_CODE_RATE_TYPE		eCodeRate;
	E_RTV_ISDBT_INTERLV_TYPE	eInterlv;
	int						fEWS;	
} RTV_ISDBT_TMCC_INFO;

void rtvISDBT_StandbyMode(int on);
UINT rtvISDBT_GetLockStatus(void); 
U8   rtvISDBT_GetAGC(void);
S32  rtvISDBT_GetRSSI(void);
U32  rtvISDBT_GetPER(void);
U32  rtvISDBT_GetCNR(void);
U32  rtvISDBT_GetBER(void);
UINT rtvISDBT_GetAntennaLevel(U32 dwCNR);
void rtvISDBT_GetTMCC(RTV_ISDBT_TMCC_INFO *ptTmccInfo);
void rtvISDBT_DisableStreamOut(void);
void rtvISDBT_EnableStreamOut(void);
INT  rtvISDBT_SetFrequency(UINT nChNum);
INT  rtvISDBT_ScanFrequency(UINT nChNum);
void rtvISDBT_SwReset(void);
INT  rtvISDBT_Initialize(E_RTV_COUNTRY_BAND_TYPE eRtvCountryBandType, UINT nThresholdSize);

 
#ifdef __cplusplus 
} 
#endif 

#endif /* __RAONTV_H__ */

