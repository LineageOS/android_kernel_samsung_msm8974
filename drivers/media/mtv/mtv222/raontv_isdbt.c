/******************************************************************************
* (c) COPYRIGHT 2010 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE 	  : RAONTECH TV ISDB-T services source file. 
*
* FILENAME    : raontv_isdbt.c
*
* DESCRIPTION :
*		Library of routines to initialize, and operate on, the RAONTECH ISDB-T demod.
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

UINT g_dwRtvPrevChNum;


static const RTV_REG_INIT_INFO g_atTopHostInitData[] = {
	{0x04, 0x02},
	{0x08, 0x17},
	{0x09, 0x00},
	{0x0A, 0x10},
	{0x0B, 0x36},
	{0x0D, 0x05},
	{0x0E, 0xCB},
	{0x10, 0x40},
	{0x11, 0x0A},
	{0x12, 0x08},
	{0x13, 0x3C},
	{0x14, 0x50},
	{0x15, 0xA0},
	{0x16, 0xE0},
	{0x17, 0x02},
	{0x18, 0xC0},
	{0x21, 0x01}
};

static const RTV_REG_INIT_INFO g_atOfdmInitData[] = {
	{0x21, 0xFF},
	{0x22, 0xFF},
	{0x30, 0x28}, //2014-03-14 raontech
	{0x34, 0x0F},
	{0x35, 0xFF},
	{0x36, 0x00},
	{0x37, 0x86}, //Locking
	{0x39, 0x6A}, //2014-03-14 raontech
	{0x3A, 0x5F}, //2014-03-14 raontech
	{0x3B, 0x74},
	{0x4B, 0x2C},
	{0x6D, 0x52},
	{0x71, 0xAC},
	{0x75, 0x8B},
	{0x8E, 0x28} //AGC
};

static const RTV_REG_INIT_INFO g_atFecInitData[] = {
	{0x16, 0xFF},	
	{0x17, 0xFF}, 	
	{0x18, 0xFF}, 	
	{0x19, 0xFF}, 	
	{0x24, 0x01}, 	
	{0x4F, 0x00}, 	
	{0x53, 0x3F}, 	
	{0x83, 0x10}, 
	{0x97, 0xFF},	
	{0x98, 0xFF},	
	{0x99, 0xFF},	
	{0xA7, 0x40},	
	{0xA8, 0x80},	
	{0xA9, 0xB9},	
	{0xAA, 0x80},	
	{0xAB, 0x80},	
	{0xFC, 0x00}
}; 

static void isdbt_UpdateMonitoring(void)
{	
	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x25, 0x70, 0x20);
	RTV_REG_MASK_SET(0x13, 0x80, 0x80);
	RTV_REG_MASK_SET(0x13, 0x80, 0x00);

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_MASK_SET(0x11, 0x04, 0x04);
	RTV_REG_MASK_SET(0x11, 0x04, 0x00);
}

static void isdbt_InitTOP_HOST(void)
{
	UINT nNumTblEntry;
	const RTV_REG_INIT_INFO *ptTopHostInitData;
	
	RTV_REG_MAP_SEL(HOST_PAGE);
	
	/* Set the remained initial values. */
	nNumTblEntry = sizeof(g_atTopHostInitData) / sizeof(RTV_REG_INIT_INFO);
	ptTopHostInitData = g_atTopHostInitData;
		
	do {
		RTV_REG_SET(ptTopHostInitData->bReg, ptTopHostInitData->bVal);
		ptTopHostInitData++;
	} while (--nNumTblEntry);
}

static void isdbt_InitOFDM(void)
{
	UINT nNumTblEntry;
	const RTV_REG_INIT_INFO *ptOfdmInitData;
	
	RTV_REG_MAP_SEL(OFDM_PAGE);
	
	// Set the remained initial values.
	nNumTblEntry = sizeof(g_atOfdmInitData) / sizeof(RTV_REG_INIT_INFO);
	ptOfdmInitData = g_atOfdmInitData;
		
	do {
		RTV_REG_SET(ptOfdmInitData->bReg, ptOfdmInitData->bVal);
		ptOfdmInitData++;
	} while (--nNumTblEntry);
}

static void isdbt_InitFEC(void)
{
	UINT nNumTblEntry;
	const RTV_REG_INIT_INFO *ptFecInitData;
	
	RTV_REG_MAP_SEL(FEC_PAGE);
	
	nNumTblEntry = sizeof(g_atFecInitData) / sizeof(RTV_REG_INIT_INFO);
	ptFecInitData = g_atFecInitData;

	do {
		RTV_REG_SET(ptFecInitData->bReg, ptFecInitData->bVal);
		ptFecInitData++;
	} while (--nNumTblEntry);
}

static void isdbt_SoftResetFEC(void)
{
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_MASK_SET(0xFB,0x01,0x01); 
	RTV_REG_MASK_SET(0xFB,0x01,0x00);
}

static void isdbt_InitDemod(void)
{

	RTV_REG_MAP_SEL(HOST_PAGE);
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_SET(0x05,0xBE); 
#else
	RTV_REG_SET(0x05,0x3E); 
#endif

	isdbt_InitTOP_HOST();

	isdbt_InitOFDM();
		
	isdbt_InitFEC();

	rtvOEM_ConfigureInterrupt();
	
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0x9F, 0x00);
	RTV_REG_SET(0xA4, 0x03);
	RTV_REG_SET(0xA5, 0x08);

	RTV_REG_MASK_SET(0xA9, 0x07, 0x03);
#elif defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
	rtv_ConfigureTsifFormat();
	RTV_REG_MASK_SET(0xA9, 0x07, RTV_COMM_CON47_CLK_SEL);
#else
	#error "Code not present"  	 	 
#endif

#ifdef RTV_ERROR_TSP_OUTPUT_DISABLE
	RTV_REG_MASK_SET(0xA5, 0x40, 0x40);
#endif

#ifdef RTV_NULL_PID_GENERATE
	RTV_REG_MASK_SET(0xA4, 0x02, 0x02);
#endif

	isdbt_SoftResetFEC();
}

void rtvISDBT_StandbyMode(int on)
{
	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(RF_PAGE);
	
	if (on)
		RTV_REG_MASK_SET(0x27, 0x01, 0x01);
	else
		RTV_REG_MASK_SET(0x27, 0x01, 0x00);

	RTV_GUARD_FREE;
}

UINT rtvISDBT_GetLockStatus(void)
{
	UINT OFDM_Lock;
	UINT TMCC_lock;
	UINT lock_st = 0;

	RTV_GUARD_LOCK;

   	if (g_fRtvChannelChange) {
   		RTV_GUARD_FREE;
   		RTV_DBGMSG0("[rtvISDBT_GetLockStatus] RTV Freqency change state! \n");
		return 0x0;
	}

	isdbt_UpdateMonitoring();

	RTV_REG_MAP_SEL(OFDM_PAGE);
	OFDM_Lock = RTV_REG_GET(0xC0);
	if (OFDM_Lock & 0x01)
		lock_st = RTV_ISDBT_OFDM_LOCK_MASK;

	RTV_REG_MAP_SEL(FEC_PAGE);
	TMCC_lock = RTV_REG_GET(0x10) & 0x01;

	RTV_GUARD_FREE;

	if (TMCC_lock)
		lock_st |= RTV_ISDBT_TMCC_LOCK_MASK;

	return lock_st;
}


U8 rtvISDBT_GetAGC(void)
{
	U8 bAgc;
	
	RTV_GUARD_LOCK;

	if (g_fRtvChannelChange) {
		RTV_GUARD_FREE;
		RTV_DBGMSG0("[rtvISDBT_GetAGC] RTV Freqency change state! \n");
		return 0;	 
	}

	RTV_REG_MAP_SEL(RF_PAGE);
	bAgc = RTV_REG_GET(0x11);

	RTV_GUARD_FREE;

	return bAgc;
}

//RSSI debugging log enable
//#define DEBUG_LOG_FOR_RSSI_FITTING

#define RSSI_UINT(val)	(S32)((val)*RTV_ISDBT_RSSI_DIVIDER)

#define RSSI_RFAGC_VAL(rfagc, coeffi)\
	((rfagc) * RSSI_UINT(coeffi))

#define RSSI_GVBB_VAL(gvbb, coeffi)\
	((gvbb) * RSSI_UINT(coeffi))

S32 rtvISDBT_GetRSSI(void)
{
	U8 RD10 = 0, GVBB = 0, LNAGAIN = 0, RFAGC = 0;
	S32 nRssi;

	RTV_GUARD_LOCK;

	if (g_fRtvChannelChange) {
		RTV_GUARD_FREE;
		RTV_DBGMSG0("[rtvISDBT_GetRSSI] RTV Freqency change state! \n");
		return 0;	 
	}

	RTV_REG_MAP_SEL(RF_PAGE);
	RD10 = RTV_REG_GET(0x10);
	GVBB = RTV_REG_GET(0x11);

	RTV_GUARD_FREE;

	LNAGAIN = ((RD10 & 0x18) >> 3);
	RFAGC = (RD10 & 0x07);

	switch (LNAGAIN) {
	case 0:
		nRssi = -(RSSI_RFAGC_VAL(RFAGC, 3.2)
			+ RSSI_GVBB_VAL(GVBB, 0.55)
			+ RSSI_UINT(8.6))
			+ RSSI_UINT(15);

		if (RFAGC !=0)
			nRssi += RSSI_UINT(7.5);
		break;

	case 1:
		nRssi = -(RSSI_RFAGC_VAL(RFAGC, 3.2)
			+ RSSI_GVBB_VAL(GVBB, 0.55)
			+ RSSI_UINT(-16.4))
			- RSSI_UINT(7);
		break;

	case 2:
		nRssi = -(RSSI_RFAGC_VAL(RFAGC, 3.2)
			+ RSSI_GVBB_VAL(GVBB, 0.55)
			+ RSSI_UINT(-0.6))
			- RSSI_UINT(4);
		break;

	case 3:
		nRssi = -(RSSI_RFAGC_VAL(RFAGC, 3.2)
			+ RSSI_GVBB_VAL(GVBB, 0.55)
			+ RSSI_UINT(9.6))
			- RSSI_UINT(10);
		break;
	default:
		break;
	}

#ifdef DEBUG_LOG_FOR_RSSI_FITTING
	RTV_DBGMSG2("\n[rtvISDBT_GetRSSI] 0x11(0x%02X), 0x14(0x%02X)\n",
			RD10, GVBB);
	RTV_DBGMSG3("LNAGAIN(%d), RFAGC(%d) GVBB(%d)\n", LNAGAIN, RFAGC,GVBB);
#endif

	return nRssi;
}


U32 rtvISDBT_GetPER(void)
{
	U8 rdata0 = 0, rdata1 = 0, TMCCL = 0;
	U32 per = 700;

	RTV_GUARD_LOCK;

	if (g_fRtvChannelChange) {
		RTV_GUARD_FREE;
		RTV_DBGMSG0("[rtvISDBT_GetPER] RTV Freqency change state! \n");
		return 0;	 
	}

	isdbt_UpdateMonitoring();

	RTV_REG_MAP_SEL(FEC_PAGE);
	TMCCL = RTV_REG_GET(0x10);

	if (TMCCL & 0x01) {
		rdata1 = RTV_REG_GET(0x37);
		rdata0 = RTV_REG_GET(0x38);
		per = (rdata1 << 8) | rdata0;
	}

	RTV_GUARD_FREE;

	return per;
}

U32 rtvISDBT_GetBER(void)
{
	U8 TMCCL = 0, prd0 = 0, prd1 = 0, cnt0 = 0, cnt1 = 0, cnt2 = 0;
	U32 count = 0, period, ber = RTV_ISDBT_BER_DIVIDER;

	RTV_GUARD_LOCK;

	if (g_fRtvChannelChange) {
		RTV_GUARD_FREE;
		RTV_DBGMSG0("[rtvISDBT_GetBER] RTV Freqency change state! \n"); 
		return 0;	 
	}

	isdbt_UpdateMonitoring();

	RTV_REG_MAP_SEL(FEC_PAGE);
	TMCCL = RTV_REG_GET(0x10);
	if (TMCCL & 0x01) {
		prd0 = RTV_REG_GET(0x28);
		prd1 = RTV_REG_GET(0x29);
		period = (prd0<<8) | prd1;

		cnt0 = RTV_REG_GET(0x31);
		cnt1 = RTV_REG_GET(0x32);
		cnt2 = RTV_REG_GET(0x33);
		count = ((cnt0&0x7f)<<16) | (cnt1<<8) | cnt2;
	} else
		period = 0;

	RTV_GUARD_FREE;

	if (period)
		ber = (count * (U32)RTV_ISDBT_BER_DIVIDER) / (period*8*204);

	return ber;
}

U32 rtvISDBT_GetCNR(void)
{
	U32 Mode;
	U32 val;
	U32 cn_a = 0;
	U32 cn_b = 0;

	RTV_GUARD_LOCK;

	if (g_fRtvChannelChange) {
		RTV_GUARD_FREE;
		RTV_DBGMSG0("[rtvISDBT_GetCNR] RTV Freqency change state! \n"); 
		return 0;	
	}

	rtv_UpdateAdj();

	isdbt_UpdateMonitoring();

	Mode = (RTV_REG_GET(0x7B) & 0x07);

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x25, 0x70, 0x10);
	RTV_REG_MASK_SET(0x13, 0x80, 0x80);
	RTV_REG_MASK_SET(0x13, 0x80, 0x00);
		
	val = ((RTV_REG_GET(0xCA)&0xff)<<16) 
		| ((RTV_REG_GET(0xC9)&0xff)<<8) 
		| (RTV_REG_GET(0xC8)&0xff);

	RTV_GUARD_FREE;
	
 	if (Mode == 1) {
		/* QPSK */
		if (val > 270000) { 
			cn_a = 0;
			cn_b = 0;
			return 0;
		} else if (val > 258000) {
			cn_a = 0;
			cn_b = (270000 - val)/1300;
		} else if (val > 246000) {
			cn_a = 1;
			cn_b = (258000 - val)/1300;
		} else if (val > 226000) {
			cn_a = 2;
			cn_b = (246000 - val)/2100;
		} else if (val > 206500) {
			cn_a = 3;
			cn_b = (226000 - val)/2100;
		} else if (val > 186500) {
			cn_a = 4;
			cn_b = (206500 - val)/2200;
		} else if (val > 163500) {
			cn_a = 5;
			cn_b = (186500 - val)/2400;
		} else if (val > 142000) {
			cn_a = 6;
			cn_b = (163500 - val)/2300;
		} else if (val > 121000) {
			cn_a = 7;
			cn_b = (142000 - val)/2300;
		} else if (val > 100500) {
			cn_a = 8;
			cn_b = (121000 - val)/2200;
		} else if (val > 83500) {
			cn_a = 9;
			cn_b = (100500 - val)/1800;
		} else if (val > 69000) {
			cn_a = 10;
			cn_b = (83500 - val)/1550;
		} else if (val > 57200) {
			cn_a = 11;
			cn_b = (69000 - val)/1250;
		} else if (val > 47900) {
			cn_a = 12;
			cn_b = (57200 - val)/1000;
		} else if (val > 40100) {
			cn_a = 13;
			cn_b = (47900 - val)/830;
		} else if (val > 33700) {
			cn_a = 14;
			cn_b = (40100 - val)/680;
		} else if (val > 29000) {
			cn_a = 15;
			cn_b = (33700 - val)/500;
		} else if (val > 25600) {
			cn_a = 16;
			cn_b = (29000 - val)/360;
		} else if (val > 22200) {
			cn_a = 17;
			cn_b = (25600 - val)/360;
		} else if (val > 19700) {
			cn_a = 18;
			cn_b = (22200 - val)/265;
		} else if (val > 18000) {
			cn_a = 19;
			cn_b = (19700 - val)/180;
		} else if (val > 16500) {
			cn_a = 20;
			cn_b = (18000 - val)/160;
		} else if (val > 15200) {
			cn_a = 21;
			cn_b = (16500 - val)/140;
		} else if (val > 14100) {
			cn_a = 22;
			cn_b = (15200 - val)/120;
		} else if (val > 13550) {
			cn_a = 23;
			cn_b = (14100 - val)/60;
		} else if (val > 12800) {
			cn_a = 24;
			cn_b = (13550 - val)/80;
		} else if (val > 12300) {
			cn_a = 25;
			cn_b = (12800 - val)/53;
		}
		else if (val > 11900) {
			cn_a = 26;
			cn_b = (12300 - val)/42;
		} else if (val > 11600) {
			cn_a = 27;
			cn_b = (11900 - val)/31;
		} else if (val > 11300) {
			cn_a = 28;
			cn_b = (11600 - val)/31;
		} else if (val > 11000) {
			cn_a = 29;
			cn_b = (11300 - val)/31;
		} else if (val > 0) {
			cn_a = 30;
			cn_b = 0;
		}
	} 	else if (Mode == 2) {
		/* 16 QAM  */
		if (val > 353500) { 
			cn_a = 0;
			cn_b = 0;
		} else if (val > 353500) {
			cn_a = 0;
			cn_b = (365000 - val)/124;
		} else if (val > 344200) {
			cn_a = 1;
			cn_b = (353500 - val)/101;
		} else if (val > 333200) {
			cn_a = 2;
			cn_b = (344200 - val)/120;
		} else if (val > 325000) {
			cn_a = 3;
			cn_b = (333200 - val)/90;
		} else if (val > 316700) {
			cn_a = 4;
			cn_b = (325000 - val) / 91;
		} else if (val > 308200) {
			cn_a = 5;
			cn_b = (316700 - val)/93;
		} else if (val > 299000) {
			cn_a = 6;
			cn_b = (308200 - val)/98;
		} else if (val > 295000) {
			cn_a = 7;
			cn_b = (299000 - val)/1050;
		} else if (val > 280500) {
			cn_a = 8;
			cn_b = (295000 - val)/1550;
		} else if (val > 264000) {
			cn_a = 9;
			cn_b = (280500 - val)/1750;
		} else if (val > 245000) {
			cn_a = 10;
			cn_b = (264000 - val)/2050;
		} else if (val > 222000) {
			cn_a = 11;
			cn_b = (245000 - val)/2450;
		} else if (val > 197000) {
			cn_a = 12;
			cn_b = (222000 - val)/2650;
		} else if (val > 172000) {
			cn_a = 13;
			cn_b = (197000 - val)/2650;
		} else if (val > 147000) {
			cn_a = 14;
			cn_b = (172000 - val)/2650;
		} else if (val > 125000) {
			cn_a = 15;
			cn_b = (147000 - val)/2350;
		} else if (val > 105000) {
			cn_a = 16;
			cn_b = (125000 - val)/2150;
		} else if (val > 88000) {
			cn_a = 17;
			cn_b = (105000 - val)/1800;
		} else if (val > 75000) {
			cn_a = 18;
			cn_b = (88000 - val)/1400;
		} else if (val > 64000) {
			cn_a = 19;
			cn_b = (75000 - val)/1180;
		} else if (val > 55000) {
			cn_a = 20;
			cn_b = (64000 - val)/980;
		} else if (val > 48000) {
			cn_a = 21;
			cn_b = (55000 - val)/750;
		} else if (val > 42000) {
			cn_a = 22;
			cn_b = (48000 - val)/640;
		} else if (val > 38000) {
			cn_a = 23;
			cn_b = (42000 - val)/420;
		} else if (val > 34900) {
			cn_a = 24;
			cn_b = (38000 - val)/330;
		} else if (val > 32000) {
			cn_a = 25;
			cn_b = (34900 - val)/310;
		} else if (val > 29500) {
			cn_a = 26;
			cn_b = (32000 - val)/265;
		} else if (val > 27100) {
			cn_a = 27;
			cn_b = (29500 - val)/250;
		} else if (val > 26000) {
			cn_a = 28;
			cn_b = (27100 - val)/118;
		} else if (val > 25200) {
			cn_a = 29;
			cn_b = (26000 - val)/85;
		} else if (val > 0) {
			cn_a = 30;
			cn_b = 0;
		}
	} else {	
		cn_a = 0;
		cn_b = 0;
		return 0;
	}

	if (cn_b > 1000)
		return ((cn_a*(U32)RTV_ISDBT_CNR_DIVIDER) + cn_b);
	else if (cn_b > 100)
		return ((cn_a*(U32)RTV_ISDBT_CNR_DIVIDER) + (cn_b*10));
	else
		return ((cn_a*(U32)RTV_ISDBT_CNR_DIVIDER) + (cn_b*100));
}

static UINT g_nIsdbtPrevAntennaLevel;
#define ISDBT_MAX_NUM_ANTENNA_LEVEL	7

UINT rtvISDBT_GetAntennaLevel(U32 dwCNR)
{
	UINT nCurLevel = ISDBT_MAX_NUM_ANTENNA_LEVEL-1;
	UINT nPrevLevel = g_nIsdbtPrevAntennaLevel;
	static const UINT aAntLvlTbl[ISDBT_MAX_NUM_ANTENNA_LEVEL-1]
		= {15*RTV_ISDBT_CNR_DIVIDER, /* 6 */
			12*RTV_ISDBT_CNR_DIVIDER, /* 5 */
			11*RTV_ISDBT_CNR_DIVIDER, /* 4 */
			9*RTV_ISDBT_CNR_DIVIDER, /* 5 */
			7*RTV_ISDBT_CNR_DIVIDER, /* 2 */
			4*RTV_ISDBT_CNR_DIVIDER /* 1 */
			};

	do {
		if (dwCNR >= aAntLvlTbl[6-nCurLevel])
			break;
	} while (--nCurLevel != 0);

	if (nCurLevel != nPrevLevel) {
		if (nCurLevel < nPrevLevel)
			nPrevLevel--;
		else
			nPrevLevel++;

		g_nIsdbtPrevAntennaLevel = nPrevLevel;
	}

	return nPrevLevel;
}

void rtvISDBT_GetTMCC(RTV_ISDBT_TMCC_INFO *ptTmccInfo)
{
	U8 R_Data,tempData;
	U8 tempSeg = 0, tempModule = 0, tempCoderate = 0, tempInterl = 0;

	if (ptTmccInfo == NULL)	{
		RTV_DBGMSG0("[rtvISDBT_GetTMCC] RTV Invalid buffer pointer!\n"); 
		return;
	}
		
	RTV_GUARD_LOCK;

	if (g_fRtvChannelChange) {
		RTV_GUARD_FREE;
		RTV_DBGMSG0("[rtvISDBT_GetTMCC] RTV Freqency change state! \n");
		return;	
	}

	RTV_REG_MAP_SEL(OFDM_PAGE);
	tempData = RTV_REG_GET(0xC6);
	R_Data = ((tempData & 0x30) >> 2) | ((tempData & 0xC0) >> 2);
	
	switch (R_Data & 0x03) {
	case 0:
		ptTmccInfo->eGuard = RTV_ISDBT_GUARD_1_32;
		break;
	case 1:
		ptTmccInfo->eGuard = RTV_ISDBT_GUARD_1_16;
		break;
	case 2:
		ptTmccInfo->eGuard = RTV_ISDBT_GUARD_1_8;
		break;
	case 3:
		ptTmccInfo->eGuard = RTV_ISDBT_GUARD_1_4;
		break;
	default:
		break;
	}

	switch ((R_Data>>2) & 0x03) {
	case 0:
		ptTmccInfo->eTvMode = RTV_ISDBT_MODE_1;
		break;
	case 1:
		ptTmccInfo->eTvMode = RTV_ISDBT_MODE_2;
		break;
	case 2:
		ptTmccInfo->eTvMode = RTV_ISDBT_MODE_3;
		break;
	default:
		break;
	}	

	RTV_REG_MAP_SEL(FEC_PAGE);
	ptTmccInfo->fEWS = (RTV_REG_GET(0x7D)>>4)&0x01; //EWS

	R_Data = RTV_REG_GET(0x7B) & 0x3f;
	tempCoderate = ((R_Data>>3) & 0x07);
	tempModule = (R_Data & 0x07);
	R_Data = RTV_REG_GET(0x7C) & 0x7f;
	tempInterl = R_Data & 0x07;		
	tempSeg = (R_Data>>3) & 0x0f;	

	switch (tempCoderate) {
	case 0:
		ptTmccInfo->eCodeRate = RTV_CODE_RATE_1_2;
		break;
	case 1:
		ptTmccInfo->eCodeRate = RTV_CODE_RATE_2_3;
		break;
	case 2:
		ptTmccInfo->eCodeRate = RTV_CODE_RATE_3_4;
		break;
	case 3:
		ptTmccInfo->eCodeRate = RTV_CODE_RATE_5_6;
		break;
	case 4:
		ptTmccInfo->eCodeRate = RTV_CODE_RATE_7_8;
		break;
	default:
		break;
	}

	switch (tempModule) {
	case 0:
		ptTmccInfo->eModulation = RTV_MOD_DQPSK;
		break;
	case 1:
		ptTmccInfo->eModulation = RTV_MOD_QPSK;
		break;
	case 2:
		ptTmccInfo->eModulation = RTV_MOD_16QAM;
		break;
	case 3:
		ptTmccInfo->eModulation = RTV_MOD_64QAM;
		break;
	default:
		break;
	}

	switch( ptTmccInfo->eTvMode ) {
	case RTV_ISDBT_MODE_3:
		switch (tempInterl) {
		case 0:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_0;
			break;
		case 1:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_1;
			break;
		case 2:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_2;
			break;
		case 3:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_4;
			break;
		case 4:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_8;
			break;
		default:
			break;
		}
		break;

	case RTV_ISDBT_MODE_2:
		switch (tempInterl) {
		case 0:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_0;
			break;
		case 1:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_2;
			break;
		case 2:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_4;
			break;
		case 3:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_8;
			break;
		case 4:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_16;
			break;
		default:
			break;
		}
		break;
		
	case RTV_ISDBT_MODE_1:
		switch (tempInterl) {
		case 0:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_0;
			break;
		case 1:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_4;
			break;
		case 2:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_8;
			break;
		case 3:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_16;
			break;
		case 4:
			ptTmccInfo->eInterlv = RTV_ISDBT_INTERLV_32;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}	

	switch (tempSeg) {
	case 1:
		ptTmccInfo->eSeg = RTV_ISDBT_SEG_1;
		break;
	case 3:
		ptTmccInfo->eSeg = RTV_ISDBT_SEG_3;
		break;
	default:
		ptTmccInfo->eSeg = RTV_ISDBT_SEG_1;
		break;
	}

	RTV_GUARD_FREE;
}

//SCAN debuging log enable
//#define DEBUG_LOG_FOR_SCAN

#define MAX_MON_FSM_MS		100
#define MAX_COARSE_MS		600
#define MAX_OFDM_RETRY_MS	600
#define MAX_TMCC_RETRY_MS	3000

#define MON_FSM_MS_CNT		(MAX_MON_FSM_MS / 10)
#define COARSE_MS_CNT		(MAX_COARSE_MS / 10)
#define OFDM_RETRY_MS_CNT	(MAX_OFDM_RETRY_MS / 10)
#define TMCC_RETRY_MS_CNT	(MAX_TMCC_RETRY_MS / 10)

INT rtvISDBT_ScanFrequency(UINT nChNum) 
{
	INT nRet = RTV_SUCCESS;	
	UINT dwChannelFreq;
	int pwr_threshold = 0;
	int peak_pwr = 0;
	int peak_pwr2 = 0;
	INT sucess_flag = RTV_CHANNEL_NOT_DETECTED;
	INT scan_stage = RTV_CHANNEL_NOT_DETECTED;
	U8 OFDM_L = 0, Mon_FSM = 0, TMCC_L = 0;
	UINT i = MON_FSM_MS_CNT, j = COARSE_MS_CNT;
	UINT nOFDM_LockCnt = OFDM_RETRY_MS_CNT, nTMCC_LockCnt = TMCC_RETRY_MS_CNT;
	U8 CoarseCheck = 0;
#if defined(__KERNEL__) /* Linux kernel */
	unsigned long start_jiffies, end_jiffies;
	unsigned long start_jiffies_TMCC, end_jiffies_TMCC;
	UINT diff_time = 0;
#endif

//	RTV_DBGMSG1("[rtvISDBT_ScanFrequency: %u] Enter...\n", nChNum);

	RTV_GUARD_LOCK;

	g_fRtvChannelChange = TRUE;

	if (g_eRtvCountryBandType != RTV_COUNTRY_BAND_JAPAN)
	   nChNum -= 1;

	if (g_dwRtvPrevChNum != nChNum) {
		g_dwRtvPrevChNum = nChNum;
	} else {
		g_fRtvChannelChange = FALSE;
		RTV_GUARD_FREE;
		return RTV_SUCCESS;
	}

	dwChannelFreq = (nChNum * 6000) + 395143;

	RTV_REG_MAP_SEL(OFDM_PAGE);

	RTV_REG_SET(0x27, 0x6B);
	RTV_REG_SET(0x37, 0x87);  //87
	RTV_REG_SET(0x75, 0x4B);
	RTV_REG_SET(0x8C, 0xAC);

	RTV_REG_SET(0x30, 0x20);
	RTV_REG_SET(0x39, 0x5A);
	RTV_REG_SET(0x3A, 0x45);
	
#if (RTV_SRC_CLK_FREQ_KHz == 19200)
	if ((nChNum & 0x01) == 0x00)
		RTV_REG_SET(0x8E, 0x58);
#endif

	nRet = rtvRF_SetFrequency(RTV_TV_MODE_1SEG, nChNum, dwChannelFreq);
	if (nRet != RTV_SUCCESS)
		goto ISDBT_SCAN_FREQ_EXIT;

	pwr_threshold = 10000;

#if defined(__KERNEL__) /* Linux kernel */
	start_jiffies = get_jiffies_64();
#endif
	do {
		rtv_UpdateAdj();

		RTV_REG_MAP_SEL(OFDM_PAGE);
		RTV_REG_MASK_SET(0x25, 0x70, 0x20);
		RTV_REG_MASK_SET(0x13, 0x80, 0x80);
		RTV_REG_MASK_SET(0x13, 0x80, 0x00);

		Mon_FSM = (RTV_REG_GET(0xC0)>>4) & 0x07;
		peak_pwr = ((RTV_REG_GET(0xCA)&0x3f)<<16)
			 | ((RTV_REG_GET(0xC9)&0xff)<<8)
			 | (RTV_REG_GET(0xC8)&0xff);

        if ((Mon_FSM >=0x01) && (peak_pwr >= pwr_threshold))
			break;

#if defined(__KERNEL__) /* Linux kernel */
		end_jiffies = get_jiffies_64();
		diff_time = jiffies_to_msecs(end_jiffies - start_jiffies);
		if (diff_time >= MAX_MON_FSM_MS)
			break;
#endif
		RTV_DELAY_MS(10);
	} while (--i);

#if (RTV_SRC_CLK_FREQ_KHz == 19200)
	if ((nChNum & 0x01) == 0x00)
		RTV_REG_SET(0x8E, 0x28);
#endif

	RTV_REG_SET(0x27, 0x65);   //2014-03-14 raontech
	RTV_REG_SET(0x24, 0x0A);   //2014-03-14 raontech
	RTV_REG_SET(0x29, 0x29);   //2014-03-14 raontech
	RTV_REG_SET(0x2B, 0x21);   //2014-03-14 raontech

	RTV_REG_SET(0x37, 0x86); 
	RTV_REG_SET(0x30, 0x28);
	RTV_REG_SET(0x39, 0x6A);
	RTV_REG_SET(0x3A, 0x5F);
	
	RTV_REG_MASK_SET(0x10, 0x01, 0x01);
	RTV_REG_MASK_SET(0x10, 0x01, 0x00);

	if ((peak_pwr >= pwr_threshold) ) {
#if defined(__KERNEL__) /* Linux kernel */
		start_jiffies = get_jiffies_64();
#endif
		do {
			rtv_UpdateAdj();
 
	RTV_REG_MAP_SEL(OFDM_PAGE);
			RTV_REG_MASK_SET(0x25, 0x70, 0x20);
			RTV_REG_MASK_SET(0x13, 0x80, 0x80);
			RTV_REG_MASK_SET(0x13, 0x80, 0x00);
	
			Mon_FSM = (RTV_REG_GET(0xC0)>>4) & 0x07;
			peak_pwr2 = ((RTV_REG_GET(0xCA)&0x3f)<<16)
				 | ((RTV_REG_GET(0xC9)&0xff)<<8)
				 | (RTV_REG_GET(0xC8)&0xff);
	
			CoarseCheck = (RTV_REG_GET(0xC2) & 0x40) >> 6;
			if (Mon_FSM == 0x04)
				CoarseCheck = 1;
			if ((CoarseCheck == 1) && (peak_pwr2 >= pwr_threshold))
				break;
				
#if defined(__KERNEL__) /* Linux kernel */
			end_jiffies = get_jiffies_64();
			diff_time = jiffies_to_msecs(end_jiffies - start_jiffies);
			if (diff_time >= MAX_COARSE_MS)
				break;
#endif

			RTV_DELAY_MS(10);
		} while (--j);
	}

#if defined(__KERNEL__) /* Linux kernel */
//	RTV_DBGMSG2("\t@@ MON: i(%u), diff_time(%u)\n", MON_FSM_MS_CNT-i, diff_time);
#endif
 
	if (CoarseCheck == 1){
#if defined(__KERNEL__) /* Linux kernel */
		start_jiffies = get_jiffies_64();
#endif
		do {
			rtv_UpdateAdj();
			RTV_REG_MAP_SEL(OFDM_PAGE);
			RTV_REG_MASK_SET(0x13, 0x80, 0x80);
			RTV_REG_MASK_SET(0x13, 0x80, 0x00);

			OFDM_L = RTV_REG_GET(0xC0) & 0x07;

			if (OFDM_L == 0x07) {
		#if defined(__KERNEL__) /* Linux kernel */
				start_jiffies_TMCC = get_jiffies_64();
		#endif
				do {
					rtv_UpdateAdj();

					RTV_REG_MAP_SEL(FEC_PAGE);
					RTV_REG_MASK_SET(0x11, 0x04, 0x04);
					RTV_REG_MASK_SET(0x11, 0x04, 0x00);

					TMCC_L = RTV_REG_GET(0x10) & 0x01;
					if (TMCC_L)  {
						U8 part_flag = RTV_REG_GET(0x7C);

						if (part_flag & 0x80)
							sucess_flag = RTV_SUCCESS;
						else
							RTV_DBGMSG1("[rtvISDBT_ScanFrequency] (%u) Part Rx OFFed\n",
											nChNum);
						scan_stage = 0;
						goto ISDBT_SCAN_FREQ_EXIT;
					}

		#if defined(__KERNEL__) /* Linux kernel */
					 end_jiffies_TMCC = get_jiffies_64();
					 diff_time
					 	= jiffies_to_msecs(end_jiffies_TMCC-start_jiffies_TMCC);
					 if (diff_time >= MAX_TMCC_RETRY_MS) {
						RTV_DBGMSG3("\t@@ ch(%u), TMCC: nTMCC_LockCnt(%u), diff_time(%u)\n", nChNum, TMCC_RETRY_MS_CNT-nTMCC_LockCnt, diff_time);
						scan_stage= -3;
						goto ISDBT_SCAN_FREQ_EXIT;
					}
		#else
					if (nTMCC_LockCnt == 1) {
						scan_stage= -3;
						goto ISDBT_SCAN_FREQ_EXIT;
					}
		#endif
					RTV_DELAY_MS(10);
				} while (--nTMCC_LockCnt);

				scan_stage = -7;
				goto ISDBT_SCAN_FREQ_EXIT;
			 }

#if defined(__KERNEL__) /* Linux kernel */
			 end_jiffies = get_jiffies_64();
			 diff_time = jiffies_to_msecs(end_jiffies - start_jiffies);
			 if (diff_time >= MAX_OFDM_RETRY_MS) {
				//RTV_DBGMSG2("\t@@ OFDM: nOFDM_LockCnt(%u), diff_time(%u)\n", OFDM_RETRY_MS_CNT-nOFDM_LockCnt, diff_time);
				scan_stage= -2;
				goto ISDBT_SCAN_FREQ_EXIT;
			}
#else
			if (nOFDM_LockCnt == 1) {
				scan_stage= -2;
				goto ISDBT_SCAN_FREQ_EXIT;
			}
#endif

			RTV_DELAY_MS(10);
		 } while (--nOFDM_LockCnt);
	 }
	 else
	 	scan_stage= -1;

ISDBT_SCAN_FREQ_EXIT:
	g_fRtvChannelChange = FALSE;
 
 	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_SET(0x27, 0x5B);   //2014-03-14 raontech
	RTV_REG_SET(0x24, 0x08);   //2014-03-14 raontech
	RTV_REG_SET(0x29, 0x69);   //2014-03-14 raontech
	RTV_REG_SET(0x2B, 0x61);   //2014-03-14 raontech

	RTV_REG_SET(0x75, 0x8B);
	RTV_REG_SET(0x8C, 0x5C);  //2014-03-14 raontech

	RTV_GUARD_FREE;

#ifdef DEBUG_LOG_FOR_SCAN
	RTV_DBGMSG3("[new rtvISDBT_ScanFrequency: %u #(%u)] Power_Peak(%d)\n",
		 dwChannelFreq, nChNum, peak_pwr);
	RTV_DBGMSG3("\t CNT(%d), COARSE = %d, CNT = %d\n", MON_FSM_MS_CNT - i, CoarseCheck, COARSE_MS_CNT - j);
	RTV_DBGMSG3("\tOFDML = %d, OFDM_L_Cnt = %d SCAN Stage : %d\n", OFDM_L, OFDM_RETRY_MS_CNT - nOFDM_LockCnt,scan_stage);
	RTV_DBGMSG3("\tTMCCL = %d, TMCC_L_Cnt = %d Lock Result : %d\n\n", TMCC_L, TMCC_RETRY_MS_CNT - nTMCC_LockCnt,sucess_flag);
#endif
 
	return sucess_flag;
}


void rtvISDBT_DisableStreamOut(void)
{
	RTV_GUARD_LOCK;

	rtv_StreamDisable(RTV_TV_MODE_1SEG);

	RTV_GUARD_FREE;
}

void rtvISDBT_EnableStreamOut(void)
{
	RTV_GUARD_LOCK;
	
	rtv_StreamEnable();

	RTV_GUARD_FREE;
}

INT rtvISDBT_SetFrequency(UINT nChNum)
{
	INT nRet;
	UINT dwChannelFreq;

	RTV_GUARD_LOCK;

	if (g_eRtvCountryBandType != RTV_COUNTRY_BAND_JAPAN)
		nChNum -= 1;

	dwChannelFreq = (nChNum * 6000) + 395143;

	nRet = rtvRF_SetFrequency(RTV_TV_MODE_1SEG, nChNum, dwChannelFreq);

	RTV_GUARD_FREE;

	return nRet;
}

void rtvISDBT_SwReset(void)
{
	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x10,0x01,0x01);
	RTV_REG_MASK_SET(0x10,0x01,0x00);
	
	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_MASK_SET(0xFB,0x01,0x01); 
	RTV_REG_MASK_SET(0xFB,0x01,0x00);
	
	RTV_GUARD_FREE;
}

INT rtvISDBT_Initialize(E_RTV_COUNTRY_BAND_TYPE eRtvCountryBandType,
			UINT nThresholdSize)
{
	INT nRet;

	switch (eRtvCountryBandType) {
	case RTV_COUNTRY_BAND_JAPAN:
	case RTV_COUNTRY_BAND_BRAZIL:
	case RTV_COUNTRY_BAND_ARGENTINA:
		break;
	default:
		return RTV_INVAILD_COUNTRY_BAND;
	}
	g_eRtvCountryBandType = eRtvCountryBandType;

	if ((nThresholdSize % RTV_TS_PACKET_SIZE) != 0) {
		RTV_DBGMSG0("[rtvISDBT_Initialize] The size of TS data should 188 align!\n");
		return RTV_INVAILD_THRESHOLD_SIZE;
	}

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	if (nThresholdSize > (RTV_TS_PACKET_SIZE * 64)) {
		RTV_DBGMSG0("[rtvISDBT_Initialize] Maximum TSP size must be less than (188 * 64) bytes\n");
		return RTV_INVAILD_THRESHOLD_SIZE;
	}
#else
	/* TSIF */
	if (nThresholdSize > (RTV_TS_PACKET_SIZE * 16))	{
		RTV_DBGMSG0("[rtvISDBT_Initialize] Maximum TSP size must be less than (188 * 16) bytes\n");
		return RTV_INVAILD_THRESHOLD_SIZE;
	}
#endif

	g_dwRtvPrevChNum = 0;

	g_nRtvMscThresholdSize = nThresholdSize;

	g_nIsdbtPrevAntennaLevel = 0;

	g_bAdjId = 0x8C;
	g_bAdjSat = 0x7C;
	g_bAdjRefL = 0x4F;

	nRet = rtv_InitSystem(RTV_TV_MODE_1SEG, RTV_ADC_CLK_FREQ_8_MHz);
	if(nRet != RTV_SUCCESS)
		return nRet;

	/* Must after rtv_InitSystem() */
	isdbt_InitDemod();

	nRet = rtvRF_Initilize(RTV_TV_MODE_1SEG);
	if(nRet != RTV_SUCCESS)
		return nRet;

	RTV_DELAY_MS(100);

	RTV_REG_MAP_SEL(RF_PAGE);
	RTV_REG_SET(0x8F, 0x1C); 

	return RTV_SUCCESS;
}


