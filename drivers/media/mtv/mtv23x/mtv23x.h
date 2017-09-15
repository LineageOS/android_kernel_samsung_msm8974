/******************************************************************************
* (c) COPYRIGHT 2013 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE		: MTV23x device driver API header file.
*
* FILENAME	: mtv23x.h
*
* DESCRIPTION	:
*		This file contains types and declarations associated
*		with the RAONTECH TV Services.
*
******************************************************************************/
/******************************************************************************
* REVISION HISTORY
*
*    DATE         NAME          REMARKS
* ----------  -------------    ------------------------------------------------
* 03/03/2013  Yang, Maverick       Created.
******************************************************************************/

#ifndef __MTV23X_H__
#define __MTV23X_H__

#include "mtv23x_port.h"

#ifdef __cplusplus
extern "C"{
#endif

/*=============================================================================
*
* Common definitions and types.
*
*===========================================================================*/
#define MTV23X_SPI_CMD_SIZE		3

#ifndef NULL
	#define NULL		0
#endif

#ifndef FALSE
	#define FALSE		0
#endif

#ifndef TRUE
	#define TRUE		1
#endif

#ifndef MAX
	#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
	#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef ABS
	#define ABS(x)		(((x) < 0) ? -(x) : (x))
#endif


/* Error codes. */
#define RTV_SUCCESS						 0
#define RTV_POWER_ON_CHECK_ERROR		-1
#define RTV_ADC_CLK_UNLOCKED			-2
#define RTV_PLL_UNLOCKED			    -3
#define RTV_CHANNEL_NOT_DETECTED		-4
#define RTV_INVAILD_FREQUENCY_RANGE		-5
#define RTV_INVAILD_RF_BAND             -6
#define RTV_ERROR_LNATUNE               -7
#define RTV_INVAILD_THRESHOLD_SIZE		-8
#define RTV_INVAILD_SERVICE_TYPE		-9
#define RTV_INVALID_DIVER_TYPE          -10

/* Do not modify the order and value! */
enum E_RTV_SERVICE_TYPE {
	RTV_SERVICE_INVALID = -1,
	RTV_SERVICE_UHF_ISDBT_1seg = 0, /* ISDB-T 1seg */
	RTV_SERVICE_UHF_ISDBT_13seg = 1, /* ISDB-T fullseg */
	RTV_SERVICE_VHF_ISDBTmm_1seg = 2, /* ISDB-Tmm 1seg */
	RTV_SERVICE_VHF_ISDBTmm_13seg = 3, /* ISDB-Tmm 13seg */
	RTV_SERVICE_VHF_ISDBTsb_1seg = 4, /* ISDB-Tsb 1seg */
	RTV_SERVICE_VHF_ISDBTsb_3seg = 5, /* ISDB-Tsb 3seg */
#if defined(RTV_DVBT_ENABLE)
	RTV_SERVICE_DVBT = 6, /* DVB-T */
#endif
	MAX_NUM_RTV_SERVICE
};

enum E_RTV_BANDWIDTH_TYPE {
	RTV_BW_MODE_5MHZ = 0, /* DVB_T */
	RTV_BW_MODE_6MHZ, /* DVB_T, FULLSEG, ISDB-Tmm */
	RTV_BW_MODE_7MHZ, /* DVB_T, FULLSEG */
	RTV_BW_MODE_8MHZ, /* DVB_T, FULLSEG */
	RTV_BW_MODE_430KHZ, /* 1SEG at 6MHz BW */
	RTV_BW_MODE_500KHZ, /* 1SEG at 7MHz BW */
	RTV_BW_MODE_571KHZ, /* 1SEG at 8MHz BW */
	RTV_BW_MODE_857KHZ, /* DAB */
	RTV_BW_MODE_1290KHZ, /* 3SEG */
	MAX_NUM_RTV_BW_MODE_TYPE
};

/*=============================================================================
*
* ISDB-T definitions, types and APIs.
*
*===========================================================================*/
#define RTV_ISDBT_OFDM_LOCK_MASK	0x1
#define RTV_ISDBT_TMCC_LOCK_MASK		0x2
#define RTV_ISDBT_CHANNEL_LOCK_OK	\
	(RTV_ISDBT_OFDM_LOCK_MASK|RTV_ISDBT_TMCC_LOCK_MASK)

struct RTV_LAYER_SIGNAL_INFO {
	U32 cnr;
	U32 ber;
	U32 per;
};

struct RTV_Statistics {
	UINT lock;
	S32 rssi;
	U32 cnr;
	UINT antenna_level;
	UINT antenna_level_1seg;
	struct RTV_LAYER_SIGNAL_INFO layerA; /*LP 1SEG, DVB-T, ISDBT Layer A*/
	struct RTV_LAYER_SIGNAL_INFO layerB; /*ISDBT Layer B*/
	struct RTV_LAYER_SIGNAL_INFO layerC; /*ISDBT Layer C*/
};

#define RTV_ISDBT_BER_DIVIDER		100000
#define RTV_ISDBT_CNR_DIVIDER		1000
#define RTV_ISDBT_RSSI_DIVIDER		10

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
extern UINT g_nRtvThresholdSize;
static INLINE UINT rtvMTV23x_GetInterruptSize(void)
{
	return g_nRtvThresholdSize;
}
#endif

enum E_RTV_ISDBT_SEG_TYPE {
	RTV_ISDBT_SEG_1 = 0,
	RTV_ISDBT_SEG_2,
	RTV_ISDBT_SEG_3,
	RTV_ISDBT_SEG_4,
	RTV_ISDBT_SEG_5,
	RTV_ISDBT_SEG_6,
	RTV_ISDBT_SEG_7,
	RTV_ISDBT_SEG_8,
	RTV_ISDBT_SEG_9,
	RTV_ISDBT_SEG_10,
	RTV_ISDBT_SEG_11,
	RTV_ISDBT_SEG_12,
	RTV_ISDBT_SEG_13
};

enum E_RTV_ISDBT_MODE_TYPE {
	RTV_ISDBT_MODE_1 = 0,
	RTV_ISDBT_MODE_2,
	RTV_ISDBT_MODE_3
};

enum E_RTV_ISDBT_GUARD_TYPE {
	RTV_ISDBT_GUARD_1_32 = 0,
	RTV_ISDBT_GUARD_1_16,
	RTV_ISDBT_GUARD_1_8,
	RTV_ISDBT_GUARD_1_4
};

enum E_RTV_ISDBT_INTERLV_TYPE {
	RTV_ISDBT_INTERLV_0 = 0,
	RTV_ISDBT_INTERLV_1,
	RTV_ISDBT_INTERLV_2,
	RTV_ISDBT_INTERLV_4,
	RTV_ISDBT_INTERLV_8,
	RTV_ISDBT_INTERLV_16,
	RTV_ISDBT_INTERLV_32
};

enum E_RTV_MODULATION_TYPE {
	RTV_MOD_DQPSK = 0,
	RTV_MOD_QPSK,
	RTV_MOD_16QAM,
	RTV_MOD_64QAM
};

enum E_RTV_CODE_RATE_TYPE {
	RTV_CODE_RATE_1_2 = 0,
	RTV_CODE_RATE_2_3,
	RTV_CODE_RATE_3_4,
	RTV_CODE_RATE_5_6,
	RTV_CODE_RATE_7_8
};

enum E_RTV_LAYER_TYPE {
	RTV_LAYER_A = 0,
	RTV_LAYER_B,
	RTV_LAYER_C
};

struct RTV_ISDBT_LAYER_TMCC_INFO {
	enum E_RTV_ISDBT_SEG_TYPE	eSeg; /* LP CodeRate @DVB-T */
	enum E_RTV_MODULATION_TYPE	eModulation; /* Modulation @DVB-T */
	enum E_RTV_CODE_RATE_TYPE	eCodeRate; /* Hierarchy Mode@DVB-T */
	enum E_RTV_ISDBT_INTERLV_TYPE	eInterlv; /* HP CodeRate@DVB-T */
};

struct RTV_ISDBT_TMCC_INFO {
	BOOL					fEWS;
	enum E_RTV_ISDBT_MODE_TYPE		eTvMode;
	enum E_RTV_ISDBT_GUARD_TYPE		eGuard;
	struct RTV_ISDBT_LAYER_TMCC_INFO	eLayerA;
	struct RTV_ISDBT_LAYER_TMCC_INFO	eLayerB;
	struct RTV_ISDBT_LAYER_TMCC_INFO	eLayerC;
};

void rtvMTV23x_StandbyMode(int on);
void rtvMTV23x_GetSignalStatistics(struct RTV_Statistics *ptSigInfo);
UINT rtvMTV23x_GetLockStatus(void);
UINT rtvMTV23x_GetAntennaLevel(U32 dwCNR);
UINT rtvMTV23x_GetAntennaLevel_1seg(U32 dwCNR);
U8   rtvMTV23x_GetAGC(void);
U32  rtvMTV23x_GetPER(void);
U32  rtvMTV23x_GetPER2(void);
U32  rtvMTV23x_GetPER3(void);

S32  rtvMTV23x_GetRSSI(void);

U32  rtvMTV23x_GetCNR(void);
U32  rtvMTV23x_GetCNR_LayerA(void);
U32  rtvMTV23x_GetCNR_LayerB(void);
U32  rtvMTV23x_GetCNR_LayerC(void);

U32  rtvMTV23x_GetBER(void);
U32  rtvMTV23x_GetBER2(void);
U32  rtvMTV23x_GetBER3(void);

U32  rtvMTV23x_GetPreviousFrequency(void);

void rtvMTV23x_GetTMCC(struct RTV_ISDBT_TMCC_INFO *ptTmccInfo);

void rtvMTV23x_DisableStreamOut(void);
void rtvMTV23x_EnableStreamOut(void);

INT  rtvMTV23x_SetFrequency(U32 dwChFreqKHz, UINT nSubchID,
		enum E_RTV_SERVICE_TYPE eServiceType,
		enum E_RTV_BANDWIDTH_TYPE eBandwidthType, UINT nThresholdSize);

INT  rtvMTV23x_ScanFrequency(U32 dwChFreqKHz, UINT nSubchID,
		enum E_RTV_SERVICE_TYPE eServiceType,
		enum E_RTV_BANDWIDTH_TYPE eBandwidthType, UINT nThresholdSize);

INT  rtvMTV23x_Initialize(enum E_RTV_BANDWIDTH_TYPE eBandwidthType);

#if !defined(RTV_IF_SPI) && !defined(RTV_IF_EBI2)
/* 0: A,B,C 1: A 2: B */
void rtvMTV23X_ISDBT_LayerSel(U8 layer);
#endif

#ifdef RTV_DUAL_DIVERISTY_ENABLE
#define DIVERSITY_MASTER 0
#define DIVERSITY_SLAVE  1

INT rtvMTV23x_Diversity_Path_Select(BOOL bMS);

INT rtvMTV23x_Get_Diversity_Current_path(void);

INT rtvMTV23x_ConfigureDualDiversity(INT bMS);

void rtvMTV23x_ONOFF_DualDiversity(BOOL onoff);

/* return value : 0 :MIX, 1: Master 2: Slave */
INT rtvMTV23x_MonDualDiversity(void);

/* 0 : Auto 1 : Auto_NOT_Var_used 2: Force Master 3: Force Slave */
void rtvMTV23x_Diver_Manual_Set(U8 sel);

void rtvMTV23X_Diver_Update(void);
#endif /* #ifdef RTV_DUAL_DIVERISTY_ENABLE */


#ifdef __cplusplus
}
#endif

#endif /* __MTV23X_H__ */

