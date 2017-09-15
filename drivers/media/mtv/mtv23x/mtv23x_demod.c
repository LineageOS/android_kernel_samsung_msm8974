/******************************************************************************
* (c) COPYRIGHT 2013 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE		: MTV23x ISDB-T/DVB-T services source file.
*
* FILENAME	: mtv23x_demod.c
*
* DESCRIPTION	:
*		Library of routines to initialize, and operate on,
*		the RAONTECH ISDB-T demod.
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
#include "mtv23x_internal.h"

#define MTV23X_MAX_NUM_ANTENNA_LEVEL	7

BOOL g_fRtv1segLpMode;
static UINT g_nIsdbtPrevAntennaLevel;
static UINT g_nIsdbtPrevAntennaLevel_1seg;

enum E_RTV_SERVICE_TYPE g_eRtvServiceType;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
BOOL g_bRtvSpiHighSpeed;
#endif

#if !defined(RTV_IF_SPI) && !defined(RTV_IF_EBI2)
static U8 g_SelectedLayer;
#endif

#ifdef RTV_DUAL_DIVERISTY_ENABLE
U8 g_div_i2c_chip_id = RTV_CHIP_ADDR; /* 7bits ID */
enum E_RTV_SERVICE_TYPE g_eRtvServiceType_slave;
#endif

static const U8 g_atSubChNum[] = {
	0x00, 0x00, 0x10, 0x10, 0x10, 0x20, /*0  ~ 5  */
	0x20, 0x20, 0x30, 0x30, 0x30, 0x40, /*6  ~ 11 */
	0x40, 0x40, 0x50, 0x50, 0x50, 0x60, /*12 ~ 17 */
	0x60, 0x60, 0x70, 0x70, 0x70, 0x80, /*18 ~ 23 */
	0x80, 0x80, 0x90, 0x90, 0x90, 0xA0, /*24 ~ 29 */
	0xA0, 0xA0, 0xB0, 0xB0, 0xB0, 0xC0, /*30 ~ 35 */
	0xC0, 0xC0, 0xD0, 0xD0, 0xD0, 0x00  /*31 ~ 41 */
};

void rtvMTV23x_StandbyMode(int on)
{
	U8 revnum = 0;

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(RF_PAGE);
	revnum = (RTV_REG_GET(0x10) & 0xF0)>>4;

	if (on) { /* Sleep Mode */
		if (revnum >= 4) {
			RTV_REG_MASK_SET(0x3B, 0x01, 0x00);
			RTV_REG_MASK_SET(0x32, 0x01, 0x00);
		} else {
			RTV_REG_MASK_SET(0x3B, 0x01, 0x01);
			RTV_REG_MASK_SET(0x32, 0x01, 0x01);
		}

		RTV_REG_MASK_SET(0x31, 0x07, 0x07);
		RTV_REG_MASK_SET(0x32, 0xFE, 0xFE);
		RTV_REG_MASK_SET(0x33, 0xA0, 0xA0);
		RTV_REG_MASK_SET(0x3F, 0x01, 0x01);
	} else {
		if (revnum >= 4) {
			RTV_REG_MASK_SET(0x3B, 0x01, 0x01);
			RTV_REG_MASK_SET(0x32, 0x01, 0x01);
		} else {
			RTV_REG_MASK_SET(0x3B, 0x01, 0x00);
			RTV_REG_MASK_SET(0x32, 0x01, 0x00);
		}

		RTV_REG_MASK_SET(0x31, 0x07, 0x00);
		RTV_REG_MASK_SET(0x32, 0xFE, 0x00);
		RTV_REG_MASK_SET(0x33, 0xA0, 0x00);
		RTV_REG_MASK_SET(0x3F, 0x01, 0x00);
	}

	RTV_GUARD_FREE;
}

/* #define OFDM_TRACE_ENABLE */
UINT rtvMTV23x_GetLockStatus(void)
{
	U8  OFDMREG = 0, TMCCL = 0, OFDML = 0;
	UINT lock_st = 0;

	RTV_GUARD_LOCK;

	rtv_UpdateMon();

	if (g_fRtv1segLpMode) {
		RTV_REG_MAP_SEL(LPOFDM_PAGE);
		OFDMREG = RTV_REG_GET(0xC0);
		OFDML = OFDMREG & 0x07;
	} else {
		RTV_REG_MAP_SEL(SHAD_PAGE);
		OFDMREG = RTV_REG_GET(0x81);
		OFDML = (OFDMREG & 0x04) >> 2;
	}

	if (OFDML & 0x01)
		lock_st = RTV_ISDBT_OFDM_LOCK_MASK;

	RTV_REG_MAP_SEL(FEC_PAGE);
	TMCCL = RTV_REG_GET(0x10);

	if (TMCCL & 0x01)
		lock_st |= RTV_ISDBT_TMCC_LOCK_MASK;

#ifdef OFDM_TRACE_ENABLE
{
	INT acc;
	if (lock_st == RTV_ISDBT_CHANNEL_LOCK_OK) {
		if (g_fRtv1segLpMode) {
			RTV_REG_MAP_SEL(LPOFDM_PAGE);
			RTV_REG_MASK_SET(0x25, 0x70, 0x00);
			RTV_REG_MASK_SET(0x13, 0x80, 0x80);
			RTV_REG_MASK_SET(0x13, 0x80, 0x00);

			acc = (RTV_REG_GET(0xCB)<<24) |
			      (RTV_REG_GET(0xCA)<<16) |
			      (RTV_REG_GET(0xC9)<<8) |
			      RTV_REG_GET(0xC8);
			RTV_DBGMSG2("freq(%d) LP_Mode Trace = %d\n",
						g_dwRtvPrevChFreqKHz, acc);
		} else {
			RTV_REG_MAP_SEL(OFDM_PAGE);
			RTV_REG_MASK_SET(0x85, 0x30, 0x00);
			RTV_REG_MASK_SET(0x1B, 0x80, 0x80);
			RTV_REG_MASK_SET(0x1B, 0x80, 0x00);
			RTV_REG_MAP_SEL(SHAD_PAGE);
			acc = (RTV_REG_GET(0x8b)<<24) |
				  (RTV_REG_GET(0x8a)<<16) |
				  (RTV_REG_GET(0x89)<<8) |
				  RTV_REG_GET(0x88);
			RTV_DBGMSG2("freq(%d) FULL Mode Trace = %d\n",
						g_dwRtvPrevChFreqKHz, acc);
		}
	}
}
#endif

	RTV_GUARD_FREE;

	return lock_st;
}

U8 rtvMTV23x_GetAGC(void)
{
	U8 agc = 0;
	/* TBD */
	return agc;
}

/*
RSSI debugging log enable
*/
/* #define DEBUG_LOG_FOR_RSSI_FITTING */

#define RSSI_UINT(val)	(S32)((val)*RTV_ISDBT_RSSI_DIVIDER)

#define RSSI_RFAGC_VAL(rfagc, coeffi)\
	((rfagc) * RSSI_UINT(coeffi))

#define RSSI_GVBB_VAL(gvbb, coeffi)\
	((gvbb) * RSSI_UINT(coeffi))


S32 rtvMTV23x_GetRSSI(void)
{
	U8 RD11 = 0, GVBB = 0, LNAGAIN = 0, RFAGC = 0, CH_FLAG = 0;
	S32 nRssi = 0;
	S32 nRssiAppDelta = 4*RTV_ISDBT_RSSI_DIVIDER;

	if (g_fRtv1segLpMode)
		nRssiAppDelta = 0;

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(RF_PAGE);
	RD11 = RTV_REG_GET(0x11);
	GVBB = RTV_REG_GET(0x14);

	RTV_GUARD_FREE;

	CH_FLAG = ((RD11 & 0xC0) >> 6);
	LNAGAIN = ((RD11 & 0x18) >> 3);
	RFAGC = (RD11 & 0x07);

	switch (LNAGAIN) {
	case 0:
		nRssi = -(RSSI_RFAGC_VAL(RFAGC, 2.8)
			+ RSSI_GVBB_VAL(GVBB, 0.44) + 0)
			+ 5*RTV_ISDBT_RSSI_DIVIDER;
		break;

	case 1:
		nRssi = -(RSSI_RFAGC_VAL(RFAGC, 3)
			+ RSSI_GVBB_VAL(GVBB, 0.3)
			+ (19*RTV_ISDBT_RSSI_DIVIDER))
			+ 0*RTV_ISDBT_RSSI_DIVIDER;
		break;

	case 2:
		nRssi = -(RSSI_RFAGC_VAL(RFAGC, 3)
			+ RSSI_GVBB_VAL(GVBB, 0.3)
			+ (16*2*RTV_ISDBT_RSSI_DIVIDER))
			+ 0*RTV_ISDBT_RSSI_DIVIDER;
		break;

	case 3:
		nRssi = -(RSSI_RFAGC_VAL(RFAGC, 2.6)
			+ RSSI_GVBB_VAL(GVBB, 0.4)
			+ (11*3*RTV_ISDBT_RSSI_DIVIDER))
			+ 0*RTV_ISDBT_RSSI_DIVIDER;
		break;

	default:
		break;
	}

	if (g_fRtv1segLpMode)
		nRssiAppDelta = 0;
	else if (CH_FLAG == 0)
		nRssiAppDelta += (7 * RTV_ISDBT_RSSI_DIVIDER);

#ifdef DEBUG_LOG_FOR_RSSI_FITTING
	RTV_DBGMSG3("[rtvMTV23x_GetRSSI] CH_FLAG=%d 0x11=0x%02x, 0x14=0x%02x\n",
			CH_FLAG, RD11, GVBB);
	RTV_DBGMSG3("LNAGAIN = %d, RFAGC = %d GVBB : %d\n\n",
			LNAGAIN, RFAGC, GVBB);
#endif

	return nRssi + nRssiAppDelta;
}

static UINT GetSNR_LP_Mode(U8 mod, UINT val)
{
	UINT cn_a = 0;
	UINT cn_b = 0;

	if (mod == 1) {
		/* QPSK */
		if (val > 270000) {
			cn_a = 0;
			cn_b = 0;
			return 0;
		} else if (val > 258000) { /* 0~ */
			cn_a = 0;
			cn_b = (270000 - val)/1300;
		} else if (val > 246000) { /* 1~ */
			cn_a = 1;
			cn_b = (258000 - val)/1300;
		} else if (val > 226000) { /* 2~ */
			cn_a = 2;
			cn_b = (246000 - val)/2100;
		} else if (val > 206500) { /* 3~ */
			cn_a = 3;
			cn_b = (226000 - val)/2100;
		} else if (val > 186500) { /* 4~ */
			cn_a = 4;
			cn_b = (206500 - val)/2200;
		} else if (val > 163500) { /* 5~ */
			cn_a = 5;
			cn_b = (186500 - val)/2400;
		} else if (val > 142000) { /* 6~ */
			cn_a = 6;
			cn_b = (163500 - val)/2300;
		} else if (val > 121000) { /* 7~ */
			cn_a = 7;
			cn_b = (142000 - val)/2300;
		} else if (val > 100500) { /* 8~ */
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
		} else if (val > 11900) {
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
	} else if (mod == 2) {
		/* 16 QAM */
		if (val > 353500) {
			cn_a = 0;
			cn_b = 0;
		} else if (val > 353500) { /* 0~ */
			cn_a = 0;
			cn_b = (365000 - val)/124;
		} else if (val > 344200) { /* 1~ */
			cn_a = 1;
			cn_b = (353500 - val)/101;
		} else if (val > 333200) { /* 2~ */
			cn_a = 2;
			cn_b = (344200 - val)/120;
		} else if (val > 325000) { /* 3~ */
			cn_a = 3;
			cn_b = (333200 - val)/90;
		} else if (val > 316700) { /* 4~ */
			cn_a = 4;
			cn_b = (325000 - val)/91;
		} else if (val > 308200) { /* 5~ */
			cn_a = 5;
			cn_b = (316700 - val)/93;
		} else if (val > 299000) { /* 6~ */
			cn_a = 6;
			cn_b = (308200 - val)/98;
		} else if (val > 295000) { /* 7~ */
			cn_a = 7;
			cn_b = (299000 - val)/1050;
		} else if (val > 280500) { /* 8~ */
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
		return (cn_a*(U32)RTV_ISDBT_CNR_DIVIDER) + cn_b;
	else if (cn_b > 100)
		return (cn_a*(U32)RTV_ISDBT_CNR_DIVIDER) + (cn_b*10);
	else
		return (cn_a*(U32)RTV_ISDBT_CNR_DIVIDER) + (cn_b*100);
}

static UINT GetSNR_FULL_Mode(U8 mod, UINT val)
{
	UINT cn_a = 0;
	UINT cn_b = 0;

	if (mod == 1) {
		/* QPSK */
		if (val > 32500) {
			cn_a = 0;
			cn_b = 0;
			return 0;
		} else if (val > 31400) { /* 0~ */
			cn_a = 0;
			cn_b = (32500 - val)/118;
		} else if (val > 29800) { /* 1~ */
			cn_a = 1;
			cn_b = (31400 - val)/170;
		} else if (val > 27900) { /* 2~ */
			cn_a = 2;
			cn_b = (29800 - val)/205;
		} else if (val > 25500) { /* 3~ */
			cn_a = 3;
			cn_b = (27900 - val)/258;
		} else if (val > 23000) { /* 4~ */
			cn_a = 4;
			cn_b = (25500 - val)/268;
		} else if (val > 20300) { /* 5~ */
			cn_a = 5;
			cn_b = (23000 - val)/290;
		} else if (val > 17500) { /* 6~ */
			cn_a = 6;
			cn_b = (20300 - val)/300;
		} else if (val > 14600) { /* 7~ */
			cn_a = 7;
			cn_b = (17500 - val)/310;
		} else if (val > 12000) { /* 8~ */
			cn_a = 8;
			cn_b = (14600 - val)/280;
		} else if (val > 9750) {
			cn_a = 9;
			cn_b = (12000 - val)/240;
		} else if (val > 7600) {
			cn_a = 10;
			cn_b = (9750 - val)/230;
		} else if (val > 6100) {
			cn_a = 11;
			cn_b = (7600 - val)/160;
		} else if (val > 5000) {
			cn_a = 12;
			cn_b = (6100 - val)/118;
		} else if (val > 3950) {
			cn_a = 13;
			cn_b = (5000 - val)/112;
		} else if (val > 3200) {
			cn_a = 14;
			cn_b = (3950 - val)/80;
		} else if (val > 2580) {
			cn_a = 15;
			cn_b = (3200 - val)/65;
		} else if (val > 2100) {
			cn_a = 16;
			cn_b = (2580 - val)/51;
		} else if (val > 1720) {
			cn_a = 17;
			cn_b = (2100 - val)/40;
		} else if (val > 1390) {
			cn_a = 18;
			cn_b = (1720 - val)/35;
		} else if (val > 1160) {
			cn_a = 19;
			cn_b = (1390 - val)/24;
		} else if (val > 980) {
			cn_a = 20;
			cn_b = (1160 - val)/19;
		} else if (val > 820) {
			cn_a = 21;
			cn_b = (980 - val)/17;
		} else if (val > 700) {
			cn_a = 22;
			cn_b = (820 - val)/13;
		} else if (val > 600) {
			cn_a = 23;
			cn_b = (700 - val)/11;
		} else if (val > 520) {
			cn_a = 24;
			cn_b = (600 - val)/8;
		} else if (val > 450) {
			cn_a = 25;
			cn_b = (520 - val)/7;
		} else if (val > 410) {
			cn_a = 26;
			cn_b = (450 - val)/4;
		} else if (val > 380) {
			cn_a = 27;
			cn_b = (410 - val)/3;
		} else if (val > 350) {
			cn_a = 28;
			cn_b = (380 - val)/3;
		} else if (val > 330) {
			cn_a = 29;
			cn_b = (350 - val)/2;
		} else if (val > 0) {
			cn_a = 30;
			cn_b = 0;
		}
	} else if (mod == 2) {
		/* 16 QAM */
		if (val > 42000) {
			cn_a = 0;
			cn_b = 0;
		} else if (val > 40500) { /* 0~ */
			cn_a = 0;
			cn_b = (42000 - val)/160;
		} else if (val > 39000) { /* 1~ */
			cn_a = 1;
			cn_b = (40500 - val)/160;
		} else if (val > 38000) { /* 2~ */
			cn_a = 2;
			cn_b = (39000 - val)/108;
		} else if (val > 37000) {
			cn_a = 3;
			cn_b = (38000 - val)/108;
		} else if (val > 36000) {
			cn_a = 4;
			cn_b = (37000 - val)/108;
		} else if (val > 35000) {
			cn_a = 5;
			cn_b = (36000 - val)/108;
		} else if (val > 34000) {
			cn_a = 6;
			cn_b = (35000 - val)/108;
		} else if (val > 32900) {
			cn_a = 7;
			cn_b = (34000 - val)/118;
		} else if (val > 31800) {
			cn_a = 8;
			cn_b = (32900 - val)/118;
		} else if (val > 29500) {
			cn_a = 9;
			cn_b = (31800 - val)/248;
		} else if (val > 27000) {
			cn_a = 10;
			cn_b = (29500 - val)/270;
		} else if (val > 24300) {
			cn_a = 11;
			cn_b = (27000 - val)/290;
		} else if (val > 21400) {
			cn_a = 12;
			cn_b = (24300 - val)/315;
		} else if (val > 18500) {
			cn_a = 13;
			cn_b = (21400 - val)/315;
		} else if (val > 15600) {
			cn_a = 14;
			cn_b = (18500 - val)/315;
		} else if (val > 13000) {
			cn_a = 15;
			cn_b = (15600 - val)/280;
		} else if (val > 10600) {
			cn_a = 16;
			cn_b = (13000 - val)/260;
		} else if (val > 8700) {
			cn_a = 17;
			cn_b = (10600 - val)/206;
		} else if (val > 7200) {
			cn_a = 18;
			cn_b = (8700 - val)/160;
		} else if (val > 6100) {
			cn_a = 19;
			cn_b = (7200 - val)/118;
		} else if (val > 5050) {
			cn_a = 20;
			cn_b = (6100 - val)/112;
		} else if (val > 4100) {
			cn_a = 21;
			cn_b = (5050 - val)/102;
		} else if (val > 3600) {
			cn_a = 22;
			cn_b = (4100 - val)/53;
		} else if (val > 3100) {
			cn_a = 23;
			cn_b = (3600 - val)/53;
		} else if (val > 2650) {
			cn_a = 24;
			cn_b = (3100 - val)/48;
		} else if (val > 2400) {
			cn_a = 25;
			cn_b = (2650 - val)/27;
		} else if (val > 2200) {
			cn_a = 26;
			cn_b = (2400 - val)/22;
		} else if (val > 2000) {
			cn_a = 27;
			cn_b = (2200 - val)/22;
		} else if (val > 1820) {
			cn_a = 28;
			cn_b = (2000 - val)/19;
		} else if (val > 1750) {
			cn_a = 29;
			cn_b = (1820 - val)/7;
		} else if (val > 0) {
			cn_a = 30;
			cn_b = 0;
		}
	} else if (mod == 3) {
		/* 64 QAM */
		if (val > 43000) {
			cn_a = 0;
			cn_b = 0;
		} else if (val > 40700) { /* 0~ */
			cn_a = 0;
			cn_b = (43000 - val)/250;
		} else if (val > 40000) { /* 1~ */
			cn_a = 1;
			cn_b = (40700 - val)/75;
		} else if (val > 38600) {
			cn_a = 2;
			cn_b = (40000 - val)/150;
		} else if (val > 37500) {
			cn_a = 3;
			cn_b = (38600 - val)/118;
		} else if (val > 36800) {
			cn_a = 4;
			cn_b = (37500 - val)/74;
		} else if (val > 36100) {
			cn_a = 5;
			cn_b = (36800 - val)/74;
		} else if (val > 35500) {
			cn_a = 6;
			cn_b = (36100 - val)/64;
		} else if (val > 35000) {
			cn_a = 7;
			cn_b = (35500 - val)/53;
		} else if (val > 34600) {
			cn_a = 8;
			cn_b = (35000 - val)/43;
		} else if (val > 34050) {
			cn_a = 9;
			cn_b = (34600 - val)/59;
		} else if (val > 33500) {
			cn_a = 10;
			cn_b = (34050 - val)/59;
		} else if (val > 32900) {
			cn_a = 11;
			cn_b = (33500 - val)/64;
		} else if (val > 32100) {
			cn_a = 12;
			cn_b = (32900 - val)/85;
		} else if (val > 31200) {
			cn_a = 13;
			cn_b = (32100 - val)/96;
		} else if (val > 30400) {
			cn_a = 14;
			cn_b = (31200 - val)/85;
		} else if (val > 29200) {
			cn_a = 15;
			cn_b = (30400 - val)/128;
		} else if (val > 27800) {
			cn_a = 16;
			cn_b = (29200 - val)/150;
		} else if (val > 25900) {
			cn_a = 17;
			cn_b = (27800 - val)/205;
		} else if (val > 23800) {
			cn_a = 18;
			cn_b = (25900 - val)/228;
		} else if (val > 21500) {
			cn_a = 19;
			cn_b = (23800 - val)/248;
		} else if (val > 19300) {
			cn_a = 20;
			cn_b = (21500 - val)/235;
		} else if (val > 17300) {
			cn_a = 21;
			cn_b = (19300 - val)/215;
		} else if (val > 15300) {
			cn_a = 22;
			cn_b = (17300 - val)/215;
		} else if (val > 13500) {
			cn_a = 23;
			cn_b = (15300 - val)/190;
		} else if (val > 11800) {
			cn_a = 24;
			cn_b = (13500 - val)/182;
		} else if (val > 10500) {
			cn_a = 25;
			cn_b = (11800 - val)/140;
		} else if (val > 9300) {
			cn_a = 26;
			cn_b = (10500 - val)/130;
		} else if (val > 8500) {
			cn_a = 27;
			cn_b = (9300 - val)/86;
		} else if (val > 8000) {
			cn_a = 28;
			cn_b = (8500 - val)/53;
		} else if (val > 7500) {
			cn_a = 29;
			cn_b = (8000 - val)/53;
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
		return (cn_a*(U32)RTV_ISDBT_CNR_DIVIDER) + cn_b;
	else if (cn_b > 100)
		return (cn_a*(U32)RTV_ISDBT_CNR_DIVIDER) + (cn_b*10);
	else
		return (cn_a*(U32)RTV_ISDBT_CNR_DIVIDER) + (cn_b*100);
}

#if defined(RTV_DVBT_ENABLE)
static UINT GetSNR_DVBT_Mode(U8 mod, UINT val)
{
	UINT cn_a = 0;
	UINT cn_b = 0;

	if (mod == 1) {
	/* QPSK */
		if (val > 43000) { 
			cn_a = 0;
			cn_b = 0;
			return 0;
		} else if (val > 41000) {	// 0~
			cn_a = 0;
			cn_b = (43000 - val)/222;
		} else if (val > 39000) {	// 1~
			cn_a = 1;
			cn_b = (41000 - val)/222;
		} else if (val > 36400) {	// 2~
			cn_a = 2;
			cn_b = (39000 - val)/289;
		} else if (val > 33500) {	// 3~
			cn_a = 3;
			cn_b = (36400 - val)/322;
		} else if (val > 30000) {	// 4~
			cn_a = 4;
			cn_b = (33500 - val)/389;
		} else if (val > 26450) {	// 5~
			cn_a = 5;
			cn_b = (30000 - val)/394;
		} else if (val > 22900) {	// 6~
			cn_a = 6;
			cn_b = (26450 - val)/394;
		} else if (val > 19200) {	// 7~
			cn_a = 7;
			cn_b = (22900 - val)/411;
		} else if (val > 15900) {	// 8~
			cn_a = 8;
			cn_b = (19200 - val)/367;
		} else if (val > 13000) {
			cn_a = 9;
			cn_b = (15900 - val)/322;
		} else if (val > 10400) {
			cn_a = 10;
			cn_b = (13000 - val)/289;
		} else if (val > 8400) {
			cn_a = 11;
			cn_b = (10400 - val)/222;
		} else if (val > 6720) {
			cn_a = 12;
			cn_b = (8400 - val)/187;
		} else if (val > 5450) {
			cn_a = 13;
			cn_b = (6720 - val)/141;
		} else if (val > 4380) {
			cn_a = 14;
			cn_b = (5450 - val)/119;
		} else if (val > 3520) {
			cn_a = 15;
			cn_b = (4380 - val)/95.5;
		} else if (val > 2850) {
			cn_a = 16;
			cn_b = (3520 - val)/74.4;
		} else if (val > 2340) {
			cn_a = 17;
			cn_b = (2850 - val)/56.7;
		} else if (val > 1940) {
			cn_a = 18;
			cn_b = (2340 - val)/44.4;
		} else if (val > 1600) {
			cn_a = 19;
			cn_b = (1940 - val)/37.8;
		} else if (val > 1370) {
			cn_a = 20;
			cn_b = (1600 - val)/25.6;
		} else if (val > 1160) {
			cn_a = 21;
			cn_b = (1370 - val)/23.3;
		} else if (val > 980) {
			cn_a = 22;
			cn_b = (1160 - val)/20;
		} else if (val > 830) {
			cn_a = 23;
			cn_b = (980 - val)/16.7;
		} else if (val > 720) {
			cn_a = 24;
			cn_b = (830 - val)/12.2;
		} else if (val > 640) {
			cn_a = 25;
			cn_b = (720 - val)/8.9;
		} else if (val > 570) {
			cn_a = 26;
			cn_b = (640 - val)/7.8;
		} else if (val > 520) {
			cn_a = 27;
			cn_b = (570 - val)/5.5;
		} else if (val > 480) {
			cn_a = 28;
			cn_b = (520 - val)/4.4;
		} else if (val > 450) {
			cn_a = 29;
			cn_b = (480 - val)/3.3;
		} else if (val > 0) {
			cn_a = 30;
			cn_b = 0;
		}
	}else if (mod == 2) {
		/* 16 QAM */
		if (val > 53000) { 
			cn_a = 0;
			cn_b = 0;
		} else if (val > 51200) {	// 0~
			cn_a = 0;
			cn_b = (53000 - val)/200;
		} else if (val > 49600) {	// 1~
			cn_a = 1;
			cn_b = (51200 - val)/178;
		} else if (val > 48300) {	// 2~
			cn_a = 2;
			cn_b = (49600 - val)/144;
		} else if (val > 46850) {	// 3~
			cn_a = 3;
			cn_b = (48300 - val)/161;
		} else if (val > 45500) {	// 4~
			cn_a = 4;
			cn_b = (46850 - val)/150;
		} else if (val > 44300) {	// 5~
			cn_a = 5;
			cn_b = (45500 - val)/133;
		} else if (val > 43100) {	// 6~
			cn_a = 6;
			cn_b = (44300 - val)/133;
		} else if (val > 41800) {	// 7~
			cn_a = 7;
			cn_b = (43100 - val)/144;
		} else if (val > 40000) {	// 8~
			cn_a = 8;
			cn_b = (41800 - val)/200;
		} else if (val > 38000) {
			cn_a = 9;
			cn_b = (40000 - val)/222;
		} else if (val > 34800) {
			cn_a = 10;
			cn_b = (38000 - val)/356;
		} else if (val > 31600) {
			cn_a = 11;
			cn_b = (34800 - val)/356;
		} else if (val > 28100) {
			cn_a = 12;
			cn_b = (31600 - val)/389;
		} else if (val > 24300) {
			cn_a = 13;
			cn_b = (28100 - val)/422;
		} else if (val > 20800) {
			cn_a = 14;
			cn_b = (24300 - val)/389;
		} else if (val > 17500) {
			cn_a = 15;
			cn_b = (20800 - val)/367;
		} else if (val > 14500) {
			cn_a = 16;
			cn_b = (17500 - val)/333;
		} else if (val > 12100) {
			cn_a = 17;
			cn_b = (14500 - val)/267;
		} else if (val > 10000) {
			cn_a = 18;
			cn_b = (12100 - val)/233;
		} else if (val > 8300) {
			cn_a = 19;
			cn_b = (10000 - val)/189;
		} else if (val > 7050) {
			cn_a = 20;
			cn_b = (8300 - val)/139;
		} else if (val > 6000) {
			cn_a = 21;
			cn_b = (7050 - val)/117;
		} else if (val > 5200) {
			cn_a = 22;
			cn_b = (6000 - val)/89;
		} else if (val > 4450) {
			cn_a = 23;
			cn_b = (5200 - val)/83;
		} else if (val > 3950) {
			cn_a = 24;
			cn_b = (4450 - val)/55.5;
		} else if (val > 3600) {
			cn_a = 25;
			cn_b = (3950 - val)/39;
		} else if (val > 3100) {
			cn_a = 26;
			cn_b = (3600 - val)/55.5;
		} else if (val > 2900) {
			cn_a = 27;
			cn_b = (3100 - val)/22;
		} else if (val > 2650) {
			cn_a = 28;
			cn_b = (2900 - val)/28;
		} else if (val > 2500) {
			cn_a = 29;
			cn_b = (2650 - val)/16.5;
		} else if (val > 0) {
			cn_a = 30;
			cn_b = 0;
		}
	}else if (mod == 3) { 
	/* 64 QAM */
		if (val > 53300) { 
			cn_a = 0;
			cn_b = 0;
		} else if (val > 52000) {	// 0~
			cn_a = 0;
			cn_b = (53300 - val)/144;
		} else if (val > 50500) {	// 1~
			cn_a = 1;
			cn_b = (52000 - val)/166;
		} else if (val > 49200) {	// 2~
			cn_a = 2;
			cn_b = (50500 - val)/144;
		} else if (val > 47900) {	// 3~
			cn_a = 3;
			cn_b = (49200 - val)/144;
		} else if (val > 46700) {	// 4~
			cn_a = 4;
			cn_b = (47900 - val)/133;
		} else if (val > 45800) {	// 5~
			cn_a = 5;
			cn_b = (46700 - val)/100;
		} else if (val > 44700) {	// 6~
			cn_a = 6;
			cn_b = (45800 - val)/122;
		}else if (val > 43800) {	// 7~
			cn_a = 7;
			cn_b = (44700 - val)/100;
		} else if (val > 43000) {	// 8~
			cn_a = 8;
			cn_b = (43800 - val)/89;
		} else if (val > 42100) {
			cn_a = 9;
			cn_b = (43000 - val)/100;
		} else if (val > 41500) {
			cn_a = 10;
			cn_b = (42100 - val)/66;
		} else if (val > 40700) {
			cn_a = 11;
			cn_b = (41500 - val)/89;
		} else if (val > 40200) {
			cn_a = 12;
			cn_b = (40700 - val)/55;
		} else if (val > 39300) {
			cn_a = 13;
			cn_b = (40200 - val)/100;
		} else if (val > 38400) {
			cn_a = 14;
			cn_b = (39300 - val)/100;
		} else if (val > 37100) {
			cn_a = 15;
			cn_b = (38400 - val)/144;
		} else if (val > 35400) {
			cn_a = 16;
			cn_b = (37100 - val)/189;
		} else if (val > 33200) {
			cn_a = 17;
			cn_b = (35400 - val)/244;
		} else if (val > 30500) {
			cn_a = 18;
			cn_b = (33200 - val)/300;
		} else if (val > 27200) {
			cn_a = 19;
			cn_b = (30500 - val)/366;
		} else if (val > 24000) {
			cn_a = 20;
			cn_b = (27200 - val)/355;
		} else if (val > 21000) {
			cn_a = 21;
			cn_b = (24000 - val)/333;
		} else if (val > 19000) {
			cn_a = 22;
			cn_b = (21000 - val)/222;
		} else if (val > 16500) {
			cn_a = 23;
			cn_b = (19000 - val)/278;
		} else if (val > 14800) {
			cn_a = 24;
			cn_b = (16500 - val)/189;
		}else if (val > 13000) {
			cn_a = 25;
			cn_b = (14800 - val)/200;
		} else if (val > 11700) {
			cn_a = 26;
			cn_b = (13000 - val)/144;
		} else if (val > 10800) {
			cn_a = 27;
			cn_b = (11700 - val)/100;
		}else if (val > 10000) {
			cn_a = 28;
			cn_b = (10800 - val)/89;
		} else if (val > 9200) {
			cn_a = 29;
			cn_b = (10000 - val)/89;
		} else if (val > 0) {
			cn_a = 30;
			cn_b = 0;
		}
	}	else {	
		cn_a = 0;
		cn_b = 0;
		return 0;
	}

	if (cn_b > 1000)
		return (cn_a*(U32)RTV_ISDBT_CNR_DIVIDER) + cn_b;
	else if (cn_b > 100)
		return (cn_a*(U32)RTV_ISDBT_CNR_DIVIDER) + (cn_b*10);
	else
		return (cn_a*(U32)RTV_ISDBT_CNR_DIVIDER) + (cn_b*100);
}
#endif /* #if defined(RTV_DVBT_ENABLE) */

U32 rtvMTV23x_GetCNR(void)
{
	U32 data = 0, cnr = 0;
	U8 Mod = 0xFF, Cd = 0xFF;

	RTV_GUARD_LOCK;

	rtv_UpdateMon();

	RTV_REG_MASK_SET(0x76, 0x18, 0x00);

	if (g_fRtv1segLpMode) {
		Mod = RTV_REG_GET(0x7B) & 0x07;

		RTV_REG_MAP_SEL(LPOFDM_PAGE);
		RTV_REG_MASK_SET(0x25, 0x70, 0x10);
		RTV_REG_MASK_SET(0x13, 0x80, 0x80);
		RTV_REG_MASK_SET(0x13, 0x80, 0x00);

		data = ((RTV_REG_GET(0xCA)&0xff)<<16)
				| ((RTV_REG_GET(0xC9)&0xff)<<8)
				| (RTV_REG_GET(0xC8)&0xff);

		RTV_GUARD_FREE;
		cnr = GetSNR_LP_Mode(Mod, data);
	} else {

		Cd = (RTV_REG_GET(0x7C) >> 3) & 0x0F;

		if (Cd < 2)
			RTV_REG_MASK_SET(0x76, 0x18, 0x08);

#if defined(RTV_DVBT_ENABLE)
		if (g_eRtvServiceType == RTV_SERVICE_DVBT)
			Mod = ((RTV_REG_GET(0x6F) >> 2) & 0x03) + 1;
		else
#endif
			Mod = (RTV_REG_GET(0x7B) & 0x07);

		RTV_REG_MAP_SEL(OFDM_PAGE);
		RTV_REG_MASK_SET(0x1B, 0x80, 0x80);
		RTV_REG_MASK_SET(0x1B, 0x80, 0x00);

		RTV_REG_MAP_SEL(SHAD_PAGE);
		data  = ((RTV_REG_GET(0xde) << 16)
			     | (RTV_REG_GET(0xdd) << 8)
			     | (RTV_REG_GET(0xdc) << 0));

		RTV_GUARD_FREE;

#if defined(RTV_DVBT_ENABLE)
		if (g_eRtvServiceType == RTV_SERVICE_DVBT)
		cnr = GetSNR_DVBT_Mode(Mod, data);
		else
#endif
		cnr = GetSNR_FULL_Mode(Mod, data);
	}

	return cnr;
}

U32 rtvMTV23x_GetCNR_LayerA(void)
{
	U32 data = 0, cnr = 0;
	U8 Mod = 0xFF, Cd = 0xFF;

#if defined(RTV_DVBT_ENABLE)
	if (g_eRtvServiceType == RTV_SERVICE_DVBT) {
		RTV_DBGMSG0("[rtvMTV23x_GetCNR_LayerA] Not Support at DVB-T\n");
		return 0;
	}
#endif

	RTV_GUARD_LOCK;
	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x7E, 0x60, 0x00);
	RTV_GUARD_FREE;

	RTV_DELAY_MS(20);

	RTV_GUARD_LOCK;
	RTV_REG_MAP_SEL(OFDM_PAGE);
	rtv_UpdateMon();

	RTV_REG_MASK_SET(0x76, 0x18, 0x00);

	Cd = ((RTV_REG_GET(0x7C) >> 3) & 0x0F);

	Mod = (RTV_REG_GET(0x7B) & 0x07);

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x1B, 0x80, 0x80);
	RTV_REG_MASK_SET(0x1B, 0x80, 0x00);

	RTV_REG_MAP_SEL(SHAD_PAGE);
	data  = ((RTV_REG_GET(0xde) << 16)
		   | (RTV_REG_GET(0xdd) << 8)
		   | (RTV_REG_GET(0xdc) << 0));

	RTV_GUARD_FREE;

	data = data * ((13 - Cd) + 1);
	cnr = GetSNR_FULL_Mode(Mod, data);

	return cnr;
}

U32 rtvMTV23x_GetCNR_LayerB(void)
{
	U32 data = 0, cnr = 0;
	U8 Mod = 0xFF, Cd = 0xFF;
	
#if defined(RTV_DVBT_ENABLE)
		if (g_eRtvServiceType == RTV_SERVICE_DVBT) {
			RTV_DBGMSG0("[rtvMTV23x_GetCNR_LayerB] Not Support at DVB-T\n");
			return 0;
		}
#endif

	RTV_GUARD_LOCK;
	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x7E, 0x60, 0x20);
	RTV_GUARD_FREE;

	RTV_GUARD_LOCK;
	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_DELAY_MS(20);
	rtv_UpdateMon();

	RTV_REG_MASK_SET(0x76, 0x18, 0x08);

	Cd = ((RTV_REG_GET(0x7C) >> 3) & 0x0F);

	Mod = (RTV_REG_GET(0x7B) & 0x07);

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x1B, 0x80, 0x80);
	RTV_REG_MASK_SET(0x1B, 0x80, 0x00);

	RTV_REG_MAP_SEL(SHAD_PAGE);
	data  = ((RTV_REG_GET(0xde) << 16)
		   | (RTV_REG_GET(0xdd) << 8)
		   | (RTV_REG_GET(0xdc) << 0));

	RTV_GUARD_FREE;

	data = data * (13 - Cd);
	cnr = GetSNR_FULL_Mode(Mod, data);

	return cnr;
}

U32 rtvMTV23x_GetCNR_LayerC(void)
{
	U32 data = 0, cnr = 0;
	U8 Mod = 0xFF, Cd = 0xFF;

#if defined(RTV_DVBT_ENABLE)
		if (g_eRtvServiceType == RTV_SERVICE_DVBT) {
			RTV_DBGMSG0("[rtvMTV23x_GetCNR_LayerC] Not Support at DVB-T\n");
			return 0;
		}
#endif

	RTV_GUARD_LOCK;
	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x7E, 0x60, 0x40);
	RTV_GUARD_FREE;

	RTV_DELAY_MS(20);

	RTV_GUARD_LOCK;
	RTV_REG_MAP_SEL(OFDM_PAGE);
	rtv_UpdateMon();

	RTV_REG_MASK_SET(0x76, 0x18, 0x10);

	Cd = ((RTV_REG_GET(0x7C) >> 3) & 0x0F);

	Mod = (RTV_REG_GET(0x7B) & 0x07);

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x1B, 0x80, 0x80);
	RTV_REG_MASK_SET(0x1B, 0x80, 0x00);

	RTV_REG_MAP_SEL(SHAD_PAGE);
	data  = ((RTV_REG_GET(0xde) << 16)
		   | (RTV_REG_GET(0xdd) << 8)
		   | (RTV_REG_GET(0xdc) << 0));

	RTV_REG_MAP_SEL(OFDM_PAGE);
	RTV_REG_MASK_SET(0x7E, 0x60, 0x60);

	RTV_GUARD_FREE;

	data = data * (13 - Cd);
	cnr = GetSNR_FULL_Mode(Mod, data);

	return cnr;
}


UINT rtvMTV23x_GetAntennaLevel(U32 dwCNR)
{
	UINT nCurLevel = MTV23X_MAX_NUM_ANTENNA_LEVEL-1;
	UINT nPrevLevel = g_nIsdbtPrevAntennaLevel;
	static const UINT aAntLvlTbl[MTV23X_MAX_NUM_ANTENNA_LEVEL-1]
		= {27*RTV_ISDBT_CNR_DIVIDER, /* 6 */
			25*RTV_ISDBT_CNR_DIVIDER, /* 5 */
			23*RTV_ISDBT_CNR_DIVIDER, /* 4 */
			22*RTV_ISDBT_CNR_DIVIDER, /* 5 */
			20*RTV_ISDBT_CNR_DIVIDER, /* 2 */
			18*RTV_ISDBT_CNR_DIVIDER /* 1 */
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

UINT rtvMTV23x_GetAntennaLevel_1seg(U32 dwCNR)
{
	UINT nCurLevel = MTV23X_MAX_NUM_ANTENNA_LEVEL-1;
	UINT nPrevLevel = g_nIsdbtPrevAntennaLevel_1seg;
	static const UINT aAntLvlTbl[MTV23X_MAX_NUM_ANTENNA_LEVEL-1]
		= {15*RTV_ISDBT_CNR_DIVIDER, /* 6 */
			12*RTV_ISDBT_CNR_DIVIDER, /* 5 */
			11*RTV_ISDBT_CNR_DIVIDER, /* 4 */
			9*RTV_ISDBT_CNR_DIVIDER, /* 5 */
			7*RTV_ISDBT_CNR_DIVIDER, /* 2 */
			4*RTV_ISDBT_CNR_DIVIDER /* 1 */
			};

#if defined(RTV_DVBT_ENABLE)
		if (g_eRtvServiceType == RTV_SERVICE_DVBT) {
			RTV_DBGMSG0("[rtvMTV23x_GetAntennaLevel_1seg] Not Support at DVB-T\n");
			return 0;
		}
#endif

	do {
		if (dwCNR >= aAntLvlTbl[6-nCurLevel])
			break;
	} while (--nCurLevel != 0);

	if (nCurLevel != nPrevLevel) {
		if (nCurLevel < nPrevLevel)
			nPrevLevel--;
		else
			nPrevLevel++;

		g_nIsdbtPrevAntennaLevel_1seg = nPrevLevel;
	}

	return nPrevLevel;
}

/* #define CHECK_TSIF_OVERRUN */
U32 rtvMTV23x_GetPER(void)
{
	U8 rdata0 = 0, rdata1 = 0, FECL = 0;
	U32 per = 700;

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(FEC_PAGE);
	FECL = RTV_REG_GET(0x10);

	if (FECL & 0x20) {
		rdata1 = RTV_REG_GET(0x37);
		rdata0 = RTV_REG_GET(0x38);
		per = (rdata1 << 8) | rdata0;
	}

#ifdef CHECK_TSIF_OVERRUN
{
	U8 rOverRun = 0;
	rOverRun = (RTV_REG_GET(0x1A) >> 3) & 0x01;

	RTV_DBGMSG1("[rtvMTV23x_GetPER] TSIF Overrun Check(%d)\n", rOverRun);
}
#endif

	RTV_GUARD_FREE;

	return per;
}

U32 rtvMTV23x_GetPER2(void)
{
	U8 rdata0 = 0, rdata1 = 0, FECL = 0;
	U32 per = 700;

#if defined(RTV_DVBT_ENABLE)
		if (g_eRtvServiceType == RTV_SERVICE_DVBT) {
			RTV_DBGMSG0("[rtvMTV23x_GetPER2] Not Support at DVB-T\n");
			return 0;
		}
#endif

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(FEC_PAGE);
	FECL = RTV_REG_GET(0x10);

	if (FECL & 0x20) {
		RTV_REG_MAP_SEL(FEC2_PAGE);

		rdata1 = RTV_REG_GET(0x37);
		rdata0 = RTV_REG_GET(0x38);
		per = (rdata1 << 8) | rdata0;
	}

	RTV_GUARD_FREE;

	return  per;
}

U32 rtvMTV23x_GetPER3(void)
{
	U8 rdata0 = 0, rdata1 = 0, FECL = 0;
	U32 per = 700;

#if defined(RTV_DVBT_ENABLE)
		if (g_eRtvServiceType == RTV_SERVICE_DVBT) {
			RTV_DBGMSG0("[rtvMTV23x_GetPER3] Not Support at DVB-T\n");
			return 0;
		}
#endif

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(FEC_PAGE);
	FECL = RTV_REG_GET(0x10);

	if (FECL & 0x20) {
		RTV_REG_MAP_SEL(FEC2_PAGE);

		rdata1 = RTV_REG_GET(0x4F);
		rdata0 = RTV_REG_GET(0x50);
		per = (rdata1 << 8) | rdata0;
	}

	RTV_GUARD_FREE;

	return  per;
}

U32 rtvMTV23x_GetBER(void)
{
	U8 FECL = 0, prd0 = 0, prd1 = 0, cnt0 = 0, cnt1 = 0, cnt2 = 0;
	U32 count, period, ber = RTV_ISDBT_BER_DIVIDER;

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(FEC_PAGE);
	FECL = RTV_REG_GET(0x10);

	if (FECL & 0x20) {
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


U32 rtvMTV23x_GetBER2(void)
{
	U8 FECL = 0, prd0 = 0, prd1 = 0, cnt0 = 0, cnt1 = 0, cnt2 = 0;
	U32 count, period, ber = RTV_ISDBT_BER_DIVIDER;

#if defined(RTV_DVBT_ENABLE)
		if (g_eRtvServiceType == RTV_SERVICE_DVBT) {
			RTV_DBGMSG0("[rtvMTV23x_GetBER2] Not Support at DVB-T\n");
			return 0;
		}
#endif

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(FEC_PAGE);
	FECL = RTV_REG_GET(0x10);

	if (FECL & 0x20) {
		RTV_REG_MAP_SEL(FEC2_PAGE);

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

U32 rtvMTV23x_GetBER3(void)
{
	U8 FECL = 0, prd0 = 0, prd1 = 0, cnt0 = 0, cnt1 = 0, cnt2 = 0;
	U32 count, period, ber = RTV_ISDBT_BER_DIVIDER;

#if defined(RTV_DVBT_ENABLE)
		if (g_eRtvServiceType == RTV_SERVICE_DVBT) {
			RTV_DBGMSG0("[rtvMTV23x_GetBER3] Not Support at DVB-T\n");
			return 0;
		}
#endif

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(FEC_PAGE);
	FECL = RTV_REG_GET(0x10);

	if (FECL & 0x20) {
		RTV_REG_MAP_SEL(FEC2_PAGE);
		prd0 = RTV_REG_GET(0x40);
		prd1 = RTV_REG_GET(0x41);
		period = (prd0<<8) | prd1;

		cnt0 = RTV_REG_GET(0x49);
		cnt1 = RTV_REG_GET(0x4A);
		cnt2 = RTV_REG_GET(0x4B);
		count = ((cnt0&0x7f)<<16) | (cnt1<<8) | cnt2;
	} else
		period = 0;

	RTV_GUARD_FREE;

	if (period)
		ber = (count * (U32)RTV_ISDBT_BER_DIVIDER) / (period*8*204);

	return ber;
}

void rtvMTV23x_GetSignalStatistics(struct RTV_Statistics *ptSigInfo)
{
	ptSigInfo->lock = rtvMTV23x_GetLockStatus();
	ptSigInfo->rssi = rtvMTV23x_GetRSSI();
	ptSigInfo->cnr = rtvMTV23x_GetCNR();
	ptSigInfo->antenna_level = rtvMTV23x_GetAntennaLevel(ptSigInfo->cnr);
	ptSigInfo->layerA.ber = rtvMTV23x_GetBER();
	ptSigInfo->layerA.per = rtvMTV23x_GetPER();

	if ((g_eRtvServiceType == RTV_SERVICE_UHF_ISDBT_13seg)
	|| (g_eRtvServiceType == RTV_SERVICE_VHF_ISDBTmm_13seg)) {
		ptSigInfo->layerA.cnr = rtvMTV23x_GetCNR_LayerA();
		ptSigInfo->antenna_level_1seg
			= rtvMTV23x_GetAntennaLevel(ptSigInfo->layerA.cnr);
		ptSigInfo->layerB.ber = rtvMTV23x_GetBER2();
		ptSigInfo->layerB.per = rtvMTV23x_GetPER2();
		ptSigInfo->layerB.cnr = rtvMTV23x_GetCNR_LayerB();
		ptSigInfo->layerC.ber = rtvMTV23x_GetBER3();
		ptSigInfo->layerC.per = rtvMTV23x_GetPER3();
		ptSigInfo->layerC.cnr = rtvMTV23x_GetCNR_LayerC();

#if !defined(RTV_IF_SPI) && !defined(RTV_IF_EBI2)
		if (g_SelectedLayer == 2) {
			ptSigInfo->layerB.ber = ptSigInfo->layerA.ber;
			ptSigInfo->layerB.per = ptSigInfo->layerA.per;
			ptSigInfo->layerA.ber = 0;
			ptSigInfo->layerA.per = 0;
			ptSigInfo->layerC.ber = 0;
			ptSigInfo->layerC.per = 0;
		}
#endif
	} else {
		ptSigInfo->layerB.ber = 0;
		ptSigInfo->layerB.per = 0;
		ptSigInfo->layerC.ber = 0;
		ptSigInfo->layerC.per = 0;
	}

}

U32 rtvMTV23x_GetPreviousFrequency(void)
{
	return g_dwRtvPrevChFreqKHz;
}


void rtvMTV23x_GetTMCC(struct RTV_ISDBT_TMCC_INFO *ptTmccInfo)
{
	U8 R_Data = 0, tempData = 0;
	U8 tempSeg = 0, tempModule = 0, tempCoderate = 0, tempInterl = 0;

	if (ptTmccInfo == NULL) {
		RTV_DBGMSG0("[rtvMTV23x_GetTMCC] Invalid buffer pointer!\n");
		return;
	}

	RTV_GUARD_LOCK;

	if (g_fRtv1segLpMode) {
		RTV_REG_MAP_SEL(LPOFDM_PAGE);
		tempData = RTV_REG_GET(0xC6);
		R_Data = ((tempData & 0x30) >> 2) | ((tempData & 0xC0) >> 2);
	} else {
		RTV_REG_MAP_SEL(SHAD_PAGE);
		R_Data = RTV_REG_GET(0x80);
	}

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
	}

	RTV_REG_MAP_SEL(FEC_PAGE);
	ptTmccInfo->fEWS = (RTV_REG_GET(0x7D)>>4) & 0x01; /* EWS */

	RTV_REG_MASK_SET(0x76, 0x18, 0x00); /* LAYER A */

#if defined(RTV_DVBT_ENABLE)
	if (g_eRtvServiceType == RTV_SERVICE_DVBT) {
		R_Data = RTV_REG_GET(0x6f) & 0x7f;
		/* HP CodeRate */
		ptTmccInfo->eLayerA.eInterlv
			= (enum E_RTV_ISDBT_INTERLV_TYPE)((R_Data>>4) & 0x07);
		ptTmccInfo->eLayerA.eModulation
			= (enum E_RTV_MODULATION_TYPE)((R_Data>>2) & 0x03);
		R_Data = RTV_REG_GET(0x70) & 0x3F;

		/* LP CodeRate */
		ptTmccInfo->eLayerA.eSeg =
			(enum E_RTV_ISDBT_SEG_TYPE)(R_Data & 0x07);

		/* Hierarchy Mode */
		ptTmccInfo->eLayerA.eCodeRate =
			(enum E_RTV_CODE_RATE_TYPE)((R_Data>>3) & 0x07);
		RTV_GUARD_FREE;
		return ;
	}
#endif

#if defined(RTV_ISDBT_ENABLE)
	R_Data = RTV_REG_GET(0x7B) & 0x3f;
	tempCoderate = ((R_Data>>3) & 0x07);
	tempModule = (R_Data & 0x07);
	R_Data = RTV_REG_GET(0x7C) & 0x7f;
	tempInterl = R_Data & 0x07;
	tempSeg = (R_Data>>3) & 0x0f;

	switch (tempCoderate) {
	case 0:
		ptTmccInfo->eLayerA.eCodeRate = RTV_CODE_RATE_1_2;
		break;
	case 1:
		ptTmccInfo->eLayerA.eCodeRate = RTV_CODE_RATE_2_3;
		break;
	case 2:
		ptTmccInfo->eLayerA.eCodeRate = RTV_CODE_RATE_3_4;
		break;
	case 3:
		ptTmccInfo->eLayerA.eCodeRate = RTV_CODE_RATE_5_6;
		break;
	case 4:
		ptTmccInfo->eLayerA.eCodeRate = RTV_CODE_RATE_7_8;
		break;
	}

	switch (tempModule) {
	case 0:
		ptTmccInfo->eLayerA.eModulation = RTV_MOD_DQPSK;
		break;
	case 1:
		ptTmccInfo->eLayerA.eModulation = RTV_MOD_QPSK;
		break;
	case 2:
		ptTmccInfo->eLayerA.eModulation = RTV_MOD_16QAM;
		break;
	case 3:
		ptTmccInfo->eLayerA.eModulation = RTV_MOD_64QAM;
		break;
	}

	switch (ptTmccInfo->eTvMode) {
	case RTV_ISDBT_MODE_3:
		switch (tempInterl) {
		case 0:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_0;
			break;
		case 1:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_1;
			break;
		case 2:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_2;
			break;
		case 3:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_4;
			break;
		case 4:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_8;
			break;
		}
		break;

	case RTV_ISDBT_MODE_2:
		switch (tempInterl) {
		case 0:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_0;
			break;
		case 1:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_2;
			break;
		case 2:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_4;
			break;
		case 3:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_8;
			break;
		case 4:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_16;
			break;
		}
		break;

	case RTV_ISDBT_MODE_1:
		switch (tempInterl) {
		case 0:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_0;
			break;
		case 1:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_4;
			break;
		case 2:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_8;
			break;
		case 3:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_16;
			break;
		case 4:
			ptTmccInfo->eLayerA.eInterlv = RTV_ISDBT_INTERLV_32;
			break;
		}
		break;
	}

	switch (tempSeg) {
	case 1:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_1;
		break;
	case 2:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_2;
		break;
	case 3:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_3;
		break;
	case 4:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_4;
		break;
	case 5:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_5;
		break;
	case 6:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_6;
		break;
	case 7:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_7;
		break;
	case 8:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_8;
		break;
	case 9:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_9;
		break;
	case 10:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_10;
		break;
	case 11:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_11;
		break;
	case 12:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_12;
		break;
	case 13:
		ptTmccInfo->eLayerA.eSeg = RTV_ISDBT_SEG_13;
		break;
	}

	RTV_REG_MASK_SET(0x76, 0x18, 0x08); /* LAYER B */

	R_Data = RTV_REG_GET(0x7B) & 0x3f;
	tempCoderate = ((R_Data>>3) & 0x07);
	tempModule = (R_Data & 0x07);
	R_Data = RTV_REG_GET(0x7C) & 0x7f;
	tempInterl = R_Data & 0x07;
	tempSeg = (R_Data>>3) & 0x0f;

	switch (tempCoderate) {
	case 0:
		ptTmccInfo->eLayerB.eCodeRate = RTV_CODE_RATE_1_2;
		break;
	case 1:
		ptTmccInfo->eLayerB.eCodeRate = RTV_CODE_RATE_2_3;
		break;
	case 2:
		ptTmccInfo->eLayerB.eCodeRate = RTV_CODE_RATE_3_4;
		break;
	case 3:
		ptTmccInfo->eLayerB.eCodeRate = RTV_CODE_RATE_5_6;
		break;
	case 4:
		ptTmccInfo->eLayerB.eCodeRate = RTV_CODE_RATE_7_8;
		break;
	}

	switch (tempModule) {
	case 0:
		ptTmccInfo->eLayerB.eModulation = RTV_MOD_DQPSK;
		break;
	case 1:
		ptTmccInfo->eLayerB.eModulation = RTV_MOD_QPSK;
		break;
	case 2:
		ptTmccInfo->eLayerB.eModulation = RTV_MOD_16QAM;
		break;
	case 3:
		ptTmccInfo->eLayerB.eModulation = RTV_MOD_64QAM;
		break;
	}

	switch (ptTmccInfo->eTvMode) {
	case RTV_ISDBT_MODE_3:
		switch (tempInterl) {
		case 0:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_0;
			break;
		case 1:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_1;
			break;
		case 2:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_2;
			break;
		case 3:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_4;
			break;
		case 4:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_8;
			break;
		}
		break;

	case RTV_ISDBT_MODE_2:
		switch (tempInterl) {
		case 0:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_0;
			break;
		case 1:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_2;
			break;
		case 2:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_4;
			break;
		case 3:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_8;
			break;
		case 4:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_16;
			break;
		}
		break;

	case RTV_ISDBT_MODE_1:
		switch (tempInterl) {
		case 0:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_0;
			break;
		case 1:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_4;
			break;
		case 2:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_8;
			break;
		case 3:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_16;
			break;
		case 4:
			ptTmccInfo->eLayerB.eInterlv = RTV_ISDBT_INTERLV_32;
			break;
		}
		break;
	}

	switch (tempSeg) {
	case 1:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_1;
		break;
	case 2:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_2;
		break;
	case 3:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_3;
		break;
	case 4:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_4;
		break;
	case 5:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_5;
		break;
	case 6:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_6;
		break;
	case 7:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_7;
		break;
	case 8:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_8;
		break;
	case 9:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_9;
		break;
	case 10:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_10;
		break;
	case 11:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_11;
		break;
	case 12:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_12;
		break;
	case 13:
		ptTmccInfo->eLayerB.eSeg = RTV_ISDBT_SEG_13;
		break;
	}

	RTV_REG_MASK_SET(0x76, 0x18, 0x10); /* LAYER C */

	R_Data = RTV_REG_GET(0x7B) & 0x3f;
	tempCoderate = ((R_Data>>3) & 0x07);
	tempModule = (R_Data & 0x07);
	R_Data = RTV_REG_GET(0x7C) & 0x7f;
	tempInterl = R_Data & 0x07;
	tempSeg = (R_Data>>3) & 0x0f;

	switch (tempCoderate) {
	case 0:
		ptTmccInfo->eLayerC.eCodeRate = RTV_CODE_RATE_1_2;
		break;
	case 1:
		ptTmccInfo->eLayerC.eCodeRate = RTV_CODE_RATE_2_3;
		break;
	case 2:
		ptTmccInfo->eLayerC.eCodeRate = RTV_CODE_RATE_3_4;
		break;
	case 3:
		ptTmccInfo->eLayerC.eCodeRate = RTV_CODE_RATE_5_6;
		break;
	case 4:
		ptTmccInfo->eLayerC.eCodeRate = RTV_CODE_RATE_7_8;
		break;
	}

	switch (tempModule) {
	case 0:
		ptTmccInfo->eLayerC.eModulation = RTV_MOD_DQPSK;
		break;
	case 1:
		ptTmccInfo->eLayerC.eModulation = RTV_MOD_QPSK;
		break;
	case 2:
		ptTmccInfo->eLayerC.eModulation = RTV_MOD_16QAM;
		break;
	case 3:
		ptTmccInfo->eLayerC.eModulation = RTV_MOD_64QAM;
		break;
	}

	switch (ptTmccInfo->eTvMode) {
	case RTV_ISDBT_MODE_3:
		switch (tempInterl) {
		case 0:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_0;
			break;
		case 1:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_1;
			break;
		case 2:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_2;
			break;
		case 3:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_4;
			break;
		case 4:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_8;
			break;
		}
		break;

	case RTV_ISDBT_MODE_2:
		switch (tempInterl) {
		case 0:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_0;
			break;
		case 1:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_2;
			break;
		case 2:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_4;
			break;
		case 3:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_8;
			break;
		case 4:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_16;
			break;
		}
		break;

	case RTV_ISDBT_MODE_1:
		switch (tempInterl) {
		case 0:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_0;
			break;
		case 1:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_4;
			break;
		case 2:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_8;
			break;
		case 3:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_16;
			break;
		case 4:
			ptTmccInfo->eLayerC.eInterlv = RTV_ISDBT_INTERLV_32;
			break;
		}
		break;
	}

	switch (tempSeg) {
	case 1:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_1;
		break;
	case 2:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_2;
		break;
	case 3:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_3;
		break;
	case 4:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_4;
		break;
	case 5:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_5;
		break;
	case 6:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_6;
		break;
	case 7:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_7;
		break;
	case 8:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_8;
		break;
	case 9:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_9;
		break;
	case 10:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_10;
		break;
	case 11:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_11;
		break;
	case 12:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_12;
		break;
	case 13:
		ptTmccInfo->eLayerC.eSeg = RTV_ISDBT_SEG_13;
		break;
	}
#endif
	RTV_GUARD_FREE;
}

void rtvMTV23x_DisableStreamOut(void)
{
	RTV_GUARD_LOCK;

	rtv_DisableTSIF();

	RTV_GUARD_FREE;
}

void rtvMTV23x_EnableStreamOut(void)
{
	RTV_GUARD_LOCK;

	rtv_EnableTSIF();

	RTV_GUARD_FREE;
}

#if !defined(RTV_IF_SPI) && !defined(RTV_IF_EBI2)
void rtvMTV23X_ISDBT_LayerSel(U8 layer)
{
	U8 cd = 0;

	if ((g_eRtvServiceType == RTV_SERVICE_UHF_ISDBT_1seg) ||
	(g_eRtvServiceType == RTV_SERVICE_VHF_ISDBTmm_1seg) ||
	(g_eRtvServiceType == RTV_SERVICE_VHF_ISDBTsb_1seg) ||
#if defined(RTV_DVBT_ENABLE)
	(g_eRtvServiceType == RTV_SERVICE_DVBT) ||
#endif
	(g_eRtvServiceType == RTV_SERVICE_VHF_ISDBTsb_3seg)) {
		RTV_DBGMSG1("[rtvMTV23X_ISDBT_LayerSel]UnSupported svc(%d)\n",
				g_eRtvServiceType);
		return;
	}

	RTV_GUARD_LOCK;
	RTV_REG_MASK_SET(0x76, 0x18, 0x00);
	cd = ((RTV_REG_GET(0x7C) >> 3) & 0x0F);

	if ((cd > 2) && (layer == 2)) {
		RTV_DBGMSG2("[rtvMTV23X_ISDBT_LayerSel]ERR cd(%d) layer(%d)\n",
					cd, layer);
		RTV_GUARD_FREE;
		return;
	}

	RTV_REG_MAP_SEL(FEC_PAGE);

	switch (layer) {
	case 0: /* 0: A,B,C  */
		RTV_REG_SET(0x23, 0x90);
		RTV_REG_SET(0x24, 0x01);
		RTV_REG_SET(0x4F, 0x00);
		break;
	case 1: /* 1: A */
		RTV_REG_SET(0x23, 0xF0);
		RTV_REG_SET(0x24, 0x31);
		RTV_REG_SET(0x4F, 0x1F);
		break;
	case 2: /* 2: B */
		RTV_REG_SET(0x23, 0xF4);
		RTV_REG_SET(0x24, 0x31);
		RTV_REG_SET(0x4F, 0x1F);
		break;
	default:
		RTV_DBGMSG1("[rtvMTV23X_ISDBT_LayerSel] Layer Error(%d)\n",
					layer);
		RTV_GUARD_FREE;
		return;
	}

	RTV_REG_MASK_SET(0xFB, 0x01, 0x01);
	RTV_REG_MASK_SET(0xFB, 0x01, 0x00);

	RTV_GUARD_FREE;

	g_SelectedLayer = layer;
}
#endif

/* #define VERIFY_INPUT_PARA */
INT rtvMTV23x_SetFrequency(U32 dwChFreqKHz, UINT nSubchID,
		enum E_RTV_SERVICE_TYPE eServiceType,
		enum E_RTV_BANDWIDTH_TYPE eBandwidthType, UINT nThresholdSize)
{
	INT nRet = RTV_SUCCESS;

#ifdef VERIFY_INPUT_PARA
	RTV_DBGMSG3("[rtvMTV23x_SetFrequency] freq(%u), subch(%u), svc(%d)",
		dwChFreqKHz, nSubchID, eServiceType);
	RTV_DBGMSG2("\tbw(%d), th(%u)\n",
		eBandwidthType, nThresholdSize);
#endif

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	if (!nThresholdSize || (nThresholdSize > (435 * RTV_TSP_XFER_SIZE))) {
		RTV_DBGMSG1("[rtvMTV23x_SetFrequency] Invalid size(%d)\n",
					nThresholdSize);
		return RTV_INVAILD_THRESHOLD_SIZE;
	}

	if (nThresholdSize % RTV_TSP_XFER_SIZE) {
		RTV_DBGMSG0("[rtvMTV23x_SetFrequency] Should 188 align!\n");
		return RTV_INVAILD_THRESHOLD_SIZE;
	}

	if ((nThresholdSize/RTV_TSP_XFER_SIZE) & 0x3) {
		RTV_DBGMSG1("[rtvMTV23x_SetFrequency] Should 4 align(%u)\n",
					nThresholdSize);
		return RTV_INVAILD_THRESHOLD_SIZE;
	}

	g_nRtvThresholdSize = nThresholdSize;
#endif

	RTV_GUARD_LOCK;

	/* Must place in the last */
	nRet = rtvRF_SetFrequency(eServiceType, eBandwidthType, dwChFreqKHz);
	if (nRet != RTV_SUCCESS)
		goto demod_SetChannel_exit;

	if ((eServiceType == RTV_SERVICE_VHF_ISDBTmm_1seg) ||
	(eServiceType == RTV_SERVICE_VHF_ISDBTsb_1seg) ||
	(eServiceType == RTV_SERVICE_VHF_ISDBTsb_3seg)) {
		RTV_REG_MAP_SEL(LPOFDM_PAGE);
		RTV_REG_SET(0x31, g_atSubChNum[nSubchID]);
		RTV_REG_SET(0x34, 0xD1);
		RTV_REG_SET(0x36, 0x00);
	} else if (eServiceType == RTV_SERVICE_UHF_ISDBT_1seg) {
		RTV_REG_MAP_SEL(LPOFDM_PAGE);
		RTV_REG_SET(0x31, 0x70);
		RTV_REG_SET(0x34, 0x9F);
		RTV_REG_SET(0x36, 0x01);
	}

	RTV_DELAY_MS(20);
	rtv_SoftReset();

#if !defined(RTV_IF_SPI) && !defined(RTV_IF_EBI2)
	g_SelectedLayer = 0;
#endif

demod_SetChannel_exit:
	RTV_GUARD_FREE;

	return nRet;
}


#define MAX_MON_FSM_MS		200
#define MAX_OFDM_RETRY_MS	1500
#define MAX_TMCC_RETRY_MS	3000

#define MON_FSM_MS_CNT		(MAX_MON_FSM_MS / 10)
#define OFDM_RETRY_MS_CNT	(MAX_OFDM_RETRY_MS / 10)
#define TMCC_RETRY_MS_CNT	(MAX_TMCC_RETRY_MS / 10)

/*
SCAN debuging log enable
*/
#define DEBUG_LOG_FOR_SCAN

INT rtvMTV23x_ScanFrequency(U32 dwChFreqKHz, UINT nSubchID,
	enum E_RTV_SERVICE_TYPE eServiceType,
	 enum E_RTV_BANDWIDTH_TYPE eBandwidthType, UINT nThresholdSize)
{
	INT nRet = RTV_SUCCESS;
	U8 Mon_FSM = 0;
	UINT peak_pwr = 0, dwLoop_Peak_pwr = 0;
	U8 OFDM_L = 0;
	INT sucess_flag = RTV_CHANNEL_NOT_DETECTED;
	INT scan_stage = RTV_CHANNEL_NOT_DETECTED;
	UINT pwr_threshold = 0;
	U8 TMCC_L = 0;
	UINT i = 0;
	UINT nOFDM_LockCnt = 0, nTMCC_LockCnt = 0;
	UINT nRealSubChid = 0, nSlave_pwr=0;
#if defined(__KERNEL__) /* Linux kernel */
	unsigned long start_jiffies, end_jiffies;
	unsigned long start_jiffies_TMCC, end_jiffies_TMCC;
	UINT diff_time = 0;
#endif

#ifdef DEBUG_LOG_FOR_SCAN
	RTV_DBGMSG1("[rtvISDBT_ScanFrequency: %u] Enter...\n", dwChFreqKHz);
#endif

	nRealSubChid = nSubchID & 0xFF;
	nSlave_pwr = nSubchID >> 8;

	nRet = rtvMTV23x_SetFrequency(dwChFreqKHz, nRealSubChid, eServiceType,
					eBandwidthType, nThresholdSize);
	if (nRet != RTV_SUCCESS)
		goto DEMOD_SCAN_EXIT;

	switch (eServiceType) {
	case RTV_SERVICE_UHF_ISDBT_1seg:
	case RTV_SERVICE_VHF_ISDBTmm_1seg:
	case RTV_SERVICE_VHF_ISDBTsb_1seg:
		pwr_threshold = 35000;
		break;

	case RTV_SERVICE_VHF_ISDBTsb_3seg:
		pwr_threshold = 35000;
		break;

	case RTV_SERVICE_UHF_ISDBT_13seg:
	case RTV_SERVICE_VHF_ISDBTmm_13seg:
		if ((dwChFreqKHz == 551143) || (dwChFreqKHz == 581143) ||
		(dwChFreqKHz == 611143) || (dwChFreqKHz == 617143) ||
		(dwChFreqKHz == 647143) || (dwChFreqKHz == 677143) ||
		(dwChFreqKHz == 707143) || (dwChFreqKHz == 737143) ||
		(dwChFreqKHz == 767143) || (dwChFreqKHz == 797143))
			pwr_threshold = 80000;
		else  if ((dwChFreqKHz == 491143) || (dwChFreqKHz == 521143))
			pwr_threshold = 90000;
		else
			pwr_threshold = 100000;
		break;

#if defined(RTV_DVBT_ENABLE)		
	case RTV_SERVICE_DVBT:
		if ((dwChFreqKHz == 184500) || (dwChFreqKHz == 490000) ||
		(dwChFreqKHz == 554000) || (dwChFreqKHz == 618000) ||
		(dwChFreqKHz == 674000) || (dwChFreqKHz == 738000) ||
		(dwChFreqKHz == 858000))
			pwr_threshold = 22000;
		else
			pwr_threshold = 25000;
		break;
#endif

	default:
		goto DEMOD_SCAN_EXIT;
	}

	RTV_GUARD_LOCK;

	i = MON_FSM_MS_CNT;
#if defined(__KERNEL__) /* Linux kernel */
	start_jiffies = get_jiffies_64();
#endif

	do {
		if (g_fRtv1segLpMode) {
			RTV_REG_MAP_SEL(LPOFDM_PAGE);
			RTV_REG_MASK_SET(0x25, 0x70, 0x20);
			RTV_REG_MASK_SET(0x13, 0x80, 0x80);
			RTV_REG_MASK_SET(0x13, 0x80, 0x00);

			Mon_FSM = (RTV_REG_GET(0xC0)>>4) & 0x07;
			peak_pwr = ((RTV_REG_GET(0xCA)&0x3f)<<16)
					|  ((RTV_REG_GET(0xC9)&0xff)<<8)
					|  (RTV_REG_GET(0xC8)&0xff);
		} else {
			RTV_REG_MAP_SEL(OFDM_PAGE);
			RTV_REG_MASK_SET(0x1B, 0x80, 0x80);
			RTV_REG_MASK_SET(0x1B, 0x80, 0x00);

			RTV_REG_MAP_SEL(SHAD_PAGE);
			Mon_FSM = RTV_REG_GET(0x84)&0x07;
			peak_pwr = (RTV_REG_GET(0xA2)&0x3f)<<16 |
					   (RTV_REG_GET(0xA1)&0xff)<<8 |
					   (RTV_REG_GET(0xA0)&0xff);
		}

		if (peak_pwr > pwr_threshold)
			break;

#ifdef RTV_DUAL_DIVERISTY_ENABLE
		if (rtvMTV23x_Get_Diversity_Current_path() == DIVERSITY_MASTER) {
		    if ( nSlave_pwr > pwr_threshold)
		          break;
		}
#endif

#if defined(__KERNEL__) /* Linux kernel */
		end_jiffies = get_jiffies_64();
		diff_time = jiffies_to_msecs(end_jiffies - start_jiffies);
		if (diff_time >= MAX_MON_FSM_MS)
			break;
#endif

		RTV_DELAY_MS(10);
	} while (--i);

#ifdef RTV_DUAL_DIVERISTY_ENABLE
	if (rtvMTV23x_Get_Diversity_Current_path() == DIVERSITY_MASTER) {
	    if (peak_pwr < nSlave_pwr)
	           peak_pwr = nSlave_pwr;
	}
	else {
		nRet = peak_pwr;
		RTV_GUARD_FREE;
		return nRet;
	}
#endif

	if ((peak_pwr > pwr_threshold) || (Mon_FSM >= 3)) {
		nOFDM_LockCnt = OFDM_RETRY_MS_CNT;
#if defined(__KERNEL__) /* Linux kernel */
		start_jiffies = get_jiffies_64();
#endif
		do {
			if (g_fRtv1segLpMode) {
				RTV_REG_MAP_SEL(LPOFDM_PAGE);
				RTV_REG_MASK_SET(0x13, 0x80, 0x80);
				RTV_REG_MASK_SET(0x13, 0x80, 0x00);

				OFDM_L = (RTV_REG_GET(0xC0)&0x01);
			} else {
				RTV_REG_MAP_SEL(OFDM_PAGE);
				RTV_REG_MASK_SET(0x1B, 0x80, 0x80);
				RTV_REG_MASK_SET(0x1B, 0x80, 0x00);
				RTV_REG_MAP_SEL(SHAD_PAGE);
				OFDM_L = ((RTV_REG_GET(0x81)&0x04)>>2);
#if 0

				Mon_FSM = RTV_REG_GET(0x84)&0x07;
				dwLoop_Peak_pwr = (RTV_REG_GET(0xA2)&0x3f)<<16 |
					   (RTV_REG_GET(0xA1)&0xff)<<8  |
					   (RTV_REG_GET(0xA0)&0xff);
			   if ((Mon_FSM >= 1)
			   && (dwLoop_Peak_pwr  < (pwr_threshold - 10000))) {
#ifdef DEBUG_LOG_FOR_SCAN
				RTV_DBGMSG2("OFDM Pwr_Peak(%d), Mon_FSM(%d)\n",
						dwLoop_Peak_pwr, Mon_FSM);
#endif
				   sucess_flag = RTV_CHANNEL_NOT_DETECTED;
				   scan_stage = -31;

				   RTV_GUARD_FREE;
				   goto DEMOD_SCAN_EXIT;
				}
#endif

#ifdef RTV_DUAL_DIVERISTY_ENABLE
		       if (OFDM_L == 0 ) {
					rtvMTV23x_Diversity_Path_Select(DIVERSITY_SLAVE);

					RTV_REG_MAP_SEL(OFDM_PAGE);
					RTV_REG_MASK_SET(0x1B, 0x80, 0x80);
					RTV_REG_MASK_SET(0x1B, 0x80, 0x00);
					RTV_REG_MAP_SEL(SHAD_PAGE);
					OFDM_L = ((RTV_REG_GET(0x81)&0x04)>>2);
					rtvMTV23x_Diversity_Path_Select(DIVERSITY_MASTER);		
				}
#endif
			}

			if (OFDM_L) {
				nTMCC_LockCnt = TMCC_RETRY_MS_CNT;
#if defined(__KERNEL__) /* Linux kernel */
				start_jiffies_TMCC = get_jiffies_64();
#endif
				do {
					RTV_REG_MAP_SEL(FEC_PAGE);
					RTV_REG_MASK_SET(0x11, 0x04, 0x04);
					RTV_REG_MASK_SET(0x11, 0x04, 0x00);

					TMCC_L = RTV_REG_GET(0x10) & 0x01;
					
#ifndef RTV_DUAL_DIVERISTY_ENABLE
					if (!g_fRtv1segLpMode) {
						RTV_REG_MAP_SEL(OFDM_PAGE);
						RTV_REG_MASK_SET(0x1B, 0x80,
									0x80);
						RTV_REG_MASK_SET(0x1B, 0x80,
									0x00);

						RTV_REG_MAP_SEL(SHAD_PAGE);
						Mon_FSM
						= RTV_REG_GET(0x84) & 0x07;

						dwLoop_Peak_pwr
						= (RTV_REG_GET(0xA2)&0x3f)<<16
						| (RTV_REG_GET(0xA1)&0xff)<<8
						| (RTV_REG_GET(0xA0)&0xff);

						if ((Mon_FSM >= 1) && (dwLoop_Peak_pwr < (pwr_threshold - 10000))) {
#ifdef DEBUG_LOG_FOR_SCAN
							RTV_DBGMSG2("\n TMCC Power_Peak(%d), Mon_FSM(%d)\n",
							   dwLoop_Peak_pwr, Mon_FSM);
#endif
							sucess_flag = RTV_CHANNEL_NOT_DETECTED;
							scan_stage = -32;

							RTV_GUARD_FREE;
							goto DEMOD_SCAN_EXIT;
						}
					}
#endif

					if (TMCC_L) {
						sucess_flag = RTV_SUCCESS;
						scan_stage = 0;

						RTV_GUARD_FREE;
						goto DEMOD_SCAN_EXIT;
					}

		#if defined(__KERNEL__) /* Linux kernel */
					 end_jiffies_TMCC = get_jiffies_64();
					 diff_time
						= jiffies_to_msecs(end_jiffies_TMCC-start_jiffies_TMCC);
					 if (diff_time >= MAX_TMCC_RETRY_MS) {
						 RTV_GUARD_FREE;

						//RTV_DBGMSG2("\t@@ OFDM: nTMCC_LockCnt(%u), diff_time(%u)\n", TMCC_RETRY_MS_CNT-nTMCC_LockCnt, diff_time);
						sucess_flag = RTV_CHANNEL_NOT_DETECTED;
						scan_stage= -1;
						goto DEMOD_SCAN_EXIT;
					}
		#else
					if (nTMCC_LockCnt == 1) {
						RTV_GUARD_FREE;

						sucess_flag = RTV_CHANNEL_NOT_DETECTED;
						scan_stage= -1;
						goto DEMOD_SCAN_EXIT;
					}
		#endif

					RTV_DELAY_MS(10);
				} while (--nTMCC_LockCnt);

				scan_stage = -7;
				goto DEMOD_SCAN_EXIT;
			}

#if defined(__KERNEL__) /* Linux kernel */
			 end_jiffies = get_jiffies_64();
			 diff_time = jiffies_to_msecs(end_jiffies - start_jiffies);
			 if (diff_time >= MAX_OFDM_RETRY_MS) {
			 	RTV_GUARD_FREE;

				//RTV_DBGMSG2("\t@@ OFDM: nOFDM_LockCnt(%u), diff_time(%u)\n", OFDM_RETRY_MS_CNT-nOFDM_LockCnt, diff_time);
				sucess_flag = RTV_CHANNEL_NOT_DETECTED;
				scan_stage= -2;
				goto DEMOD_SCAN_EXIT;
			}
#else
			if (nOFDM_LockCnt == 1) {
				RTV_GUARD_FREE;
				sucess_flag = RTV_CHANNEL_NOT_DETECTED;
				scan_stage= -2;
				goto DEMOD_SCAN_EXIT;
			}
#endif

			RTV_DELAY_MS(10);
		} while (--nOFDM_LockCnt);
	} else
		scan_stage = -3;

	RTV_GUARD_FREE;

DEMOD_SCAN_EXIT:

#ifdef DEBUG_LOG_FOR_SCAN
	RTV_DBGMSG3("[rtvMTV23x_ScanFrequency: %u] Pwr_Peak(%d), Mon_FSM(%d)\n",
			dwChFreqKHz, peak_pwr, Mon_FSM);
	RTV_DBGMSG3("\tOFDML = %d, OFDM_L_Cnt = %d SCAN Stage : %d\n",
			OFDM_L, nOFDM_LockCnt, scan_stage);
	RTV_DBGMSG3("\tTMCCL = %d, TMCC_L_Cnt = %d Lock Result : %d\n\n",
			TMCC_L, nTMCC_LockCnt, sucess_flag);
#endif

	return sucess_flag;
}

void rtvOEM_ConfigureInterrupt(void)
{
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
	RTV_REG_SET(0x20, 0x24); /* NEW_NESTING and period */
	RTV_REG_SET(0x2B, RTV_SPI_INTR_DEACT_PRD_VAL);

	RTV_REG_SET(0x2A, 1); /* SRAM init */
	RTV_REG_SET(0x2A, 0);
#else
	/* RTV_REG_MAP_SEL(FEC_PAGE); */
#endif
}

static void rtv_InitDemod(void)
{
	RTV_REG_MAP_SEL(TOP_PAGE);

#if defined(RTV_CHIP_PKG_CSP)
	RTV_REG_SET(0x04, 0x02);
#endif
	RTV_REG_SET(0x09, 0x00);

	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x28, 0x70);

	RTV_REG_MAP_SEL(LPOFDM_PAGE);
	RTV_REG_SET(0x34, 0x9F);
	RTV_REG_SET(0x35, 0xFF);
	RTV_REG_SET(0x36, 0x01);
	
	RTV_REG_SET(0x71, 0xAA);
	RTV_REG_SET(0x8E, 0x15);

	RTV_REG_MAP_SEL(OFDM_PAGE);

	RTV_REG_SET(0xB3, 0x31);
	RTV_REG_SET(0xCA, 0x10);
	RTV_REG_MASK_SET(0x7E, 0x60, 0x60);

	RTV_REG_SET(0x81, 0xFF);
	RTV_REG_SET(0x82, 0xFF);

	RTV_REG_SET(0x6D, 0x4A);
	RTV_REG_SET(0xB8, 0xA8);
	RTV_REG_SET(0xC6, 0x78);

	RTV_REG_SET(0x6F, 0x21);
	RTV_REG_SET(0xC9, 0x80);
	RTV_REG_SET(0x5F, 0x10);

	RTV_REG_SET(0x58, 0x5A);
	RTV_REG_SET(0x5E, 0x10);
	RTV_REG_SET(0xCB, 0x02);

	RTV_REG_MAP_SEL(DATA_PAGE);
	RTV_REG_SET(0x8C, 0x80);
	RTV_REG_SET(0x8F, 0x40);
	RTV_REG_SET(0xDB, 0x01);
	RTV_REG_SET(0xD8, 0x10);

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0x44, 0x68);
	RTV_REG_SET(0x47, 0x40);

	RTV_REG_SET(0x16, 0xFF);
	RTV_REG_SET(0x17, 0xFF);
	RTV_REG_SET(0x18, 0xFF);
	RTV_REG_SET(0x19, 0xFF);
	RTV_REG_SET(0xA7, 0x40);
	RTV_REG_SET(0xA8, 0x80);
	RTV_REG_SET(0xA9, 0xB9);
	RTV_REG_SET(0xAA, 0x80);
	RTV_REG_SET(0xAB, 0x80);

	RTV_REG_SET(0x5C, 0x10);
	RTV_REG_SET(0x5F, 0x10);

	RTV_REG_SET(0xFC, 0x83);
	RTV_REG_SET(0xFF, 0x03);

	rtvOEM_ConfigureInterrupt();

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x20, 0x18);

	RTV_REG_MAP_SEL(FEC_PAGE);
	RTV_REG_SET(0x24, 0x01);
	RTV_REG_SET(0x4F, 0x00);

	RTV_REG_SET(0x9F, 0x50);
	RTV_REG_SET(0xA4, 0x81);
	RTV_REG_SET(0xA5, 0x08);
	RTV_REG_SET(0xA8, 0x87);

	#if defined(RTV_ERROR_TSP_OUTPUT_DISABLE)
	RTV_REG_MASK_SET(0xA5, 0x40, 0x40);
	#endif

	#if defined(RTV_NULL_PID_TSP_OUTPUT_DISABLE)
	RTV_REG_MASK_SET(0xA5, 0x20, 0x20);
	#endif

	#if defined(RTV_NULL_PID_GENERATE)
	RTV_REG_MASK_SET(0xA4, 0x02, 0x02);
	#endif
#else
	#ifdef RTV_DUAL_DIVERISTY_ENABLE
	if (rtvMTV23x_Get_Diversity_Current_path() == DIVERSITY_MASTER) {
	#endif
		#if defined(RTV_IF_TSIF_0)
		rtv_ConfigureTsif0Format();
		#endif

		#if defined(RTV_IF_TSIF_1) || defined(RTV_IF_SPI_SLAVE)
		rtv_ConfigureTsif1Format();
		#endif
	#ifdef RTV_DUAL_DIVERISTY_ENABLE
	}
	#endif
#endif
}


#ifdef RTV_DUAL_DIVERISTY_ENABLE
INT rtvMTV23x_Diversity_Path_Select(BOOL bMS)
{
	switch (bMS) {
	case DIVERSITY_MASTER:
		g_div_i2c_chip_id = RTV_CHIP_ADDR;
		break;

	case DIVERSITY_SLAVE:
		g_div_i2c_chip_id = RTV_CHIP_ADDR_SLAVE;
		break;

	default:
		RTV_DBGMSG1("Invalied diversity path select(bMS = %d)", bMS);
		return RTV_INVALID_DIVER_TYPE;
		break;
	}

	return RTV_SUCCESS;
}

INT rtvMTV23x_Get_Diversity_Current_path(void)
{
	INT nRet;

	if (g_div_i2c_chip_id == RTV_CHIP_ADDR)
		nRet = DIVERSITY_MASTER;
	else
		nRet = DIVERSITY_SLAVE;

	return nRet;
}

void rtvMTV23x_Diver_Manual_Set(U8 sel)
{

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(FEC_PAGE);

	switch (sel) {
	case 0: /* AUTO */
		RTV_REG_MASK_SET(0xD1, 0x03, 0x00);
		break;
	case 1: /* AUTO_VAR_NOTUSED */
		RTV_REG_MASK_SET(0xD1, 0x03, 0x01);
		break;
	case 2: /* MASTER */
		RTV_REG_MASK_SET(0xD1, 0x03, 0x02);
		break;
	case 3: /* SLAVE */
		RTV_REG_MASK_SET(0xD1, 0x03, 0x03);
		break;
	default:
		RTV_DBGMSG1("Diversity Path set error(sel = %d)", sel);
		break;
	}

	RTV_REG_MASK_SET(0xFB, 0x01, 0x01);
	RTV_REG_MASK_SET(0xFB, 0x01, 0x00);

	RTV_GUARD_FREE;
}

INT rtvMTV23x_ConfigureDualDiversity(INT bMS)
{
	INT nRet = RTV_SUCCESS;

	RTV_GUARD_LOCK;

	switch (bMS) {
	case DIVERSITY_MASTER:
#ifndef RTV_DIVER_TWO_XTAL_USED
		RTV_REG_MAP_SEL(RF_PAGE);
		RTV_REG_MASK_SET(0xDD, 0x60, 0x60);
#endif
		RTV_REG_MAP_SEL(TOP_PAGE);
		RTV_REG_SET(0x06, 0x50);
		RTV_REG_SET(0x08, 0x07);
		RTV_REG_SET(0x09, 0x00);
		RTV_REG_SET(0x0C, 0xC3);
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x13, 0x3C);
		RTV_REG_MAP_SEL(OFDM_PAGE);
		RTV_REG_SET(0x19, 0x80);
		RTV_REG_SET(0xDB, 0xC4);
		RTV_REG_SET(0xDE, 0x09);

		RTV_REG_MAP_SEL(FEC_PAGE);
		RTV_REG_SET(0x21, 0x22);
		RTV_REG_SET(0x22, 0x22);
		RTV_REG_SET(0xD0, 0x47);
		RTV_REG_SET(0xD1, 0xD4);
		RTV_REG_SET(0xD2, 0x12);
		RTV_REG_SET(0xD3, 0x5F);
		break;

	case DIVERSITY_SLAVE:
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x13, 0x3E);
		RTV_DELAY_MS(10);
		RTV_REG_SET(0x0C, 0xC0);
		RTV_REG_SET(0x06, 0x10);
		RTV_REG_SET(0x08, 0x07);
		RTV_REG_SET(0x09, 0x80);
		RTV_REG_MAP_SEL(OFDM_PAGE);
		RTV_REG_SET(0x19, 0xC0);
		RTV_REG_SET(0xDB, 0xC4);
		RTV_REG_SET(0xDE, 0x09);

		RTV_REG_MAP_SEL(FEC_PAGE);
		RTV_REG_SET(0x21, 0x22);
		RTV_REG_SET(0x22, 0x22);
		RTV_REG_SET(0xD0, 0x43);
		RTV_REG_SET(0xD1, 0xD4);
		RTV_REG_SET(0xD2, 0x12);
		RTV_REG_SET(0xD3, 0x5F);
		RTV_REG_SET(0xFC, 0x80);
		break;
	default:
		RTV_DBGMSG1("Diversity setting is not correct(bMS = %d)", bMS);
		nRet = RTV_INVALID_DIVER_TYPE;
		break;
	}

	RTV_GUARD_FREE;

	return nRet;
}

INT rtvMTV23x_MonDualDiversity(void)
{
	U8 nDivStatusMon = 0; /* 0 :MIX, 1: Master 2: Slave */

	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(FEC_PAGE);

	nDivStatusMon = RTV_REG_GET(0xD4) & 0x03;

	RTV_GUARD_FREE;

	return nDivStatusMon;
}

void rtvMTV23x_ONOFF_DualDiversity(BOOL onoff)
{
	RTV_GUARD_LOCK;

	RTV_REG_MAP_SEL(FEC_PAGE);

	if (onoff)
		RTV_REG_SET(0xD0, 0x47);
	else
		RTV_REG_SET(0xD0, 0x46);

	RTV_GUARD_FREE;
}

void rtvMTV23X_Diver_Update(void)
{
	U8 mobile,div_mode;
	U8 oper_div,s_gau;
	U8 m_ofdm_lock,s_ofdm_lock;
	INT cPath;

	cPath = rtvMTV23x_Get_Diversity_Current_path();

	RTV_GUARD_LOCK;

	rtvMTV23x_Diversity_Path_Select(DIVERSITY_MASTER);
	
	rtv_UpdateMon();
	oper_div = rtvMTV23x_MonDualDiversity();

	RTV_REG_MAP_SEL(SHAD_PAGE);
	m_ofdm_lock = RTV_REG_GET(0x81) & 0x04;
	
	mobile = (RTV_REG_GET(0x93)>>2) & 0x01;

	RTV_REG_MAP_SEL(FEC_PAGE);
	div_mode = RTV_REG_GET(0xD1);
	
	rtvMTV23x_Diversity_Path_Select(DIVERSITY_SLAVE);
	rtv_UpdateMon();

	RTV_REG_MAP_SEL(SHAD_PAGE);
	s_ofdm_lock = RTV_REG_GET(0x81) & 0x04;
	s_gau = (RTV_REG_GET(0x93)>>4) & 0x01;

	rtvMTV23x_Diversity_Path_Select(DIVERSITY_MASTER);
	RTV_REG_MAP_SEL(FEC_PAGE);
	////////////////////////STEP 1/////////////////////////////////
	{
		if ((div_mode & 0x03) <= 1) {
			if (mobile)
				RTV_REG_SET(0xD1, 0xD5); 
			else
				RTV_REG_SET(0xD1, 0xD4); 
		}
	}
	////////////////////////STEP 2/////////////////////////////////
	{
		if (oper_div == 2) {
			if ( s_gau == 1 ) {
				RTV_REG_SET(0x3E, 0x1A); 
				RTV_REG_SET(0x3F, 0x92); 
			}
			else {
				RTV_REG_SET(0x3E, 0x9A); 
				RTV_REG_SET(0x3F, 0x92); 
			}
		} else {
			RTV_REG_SET(0x3F, 0x12); 
		}
	}
	////////////////////////STEP 3/////////////////////////////////
	{
		 if ((m_ofdm_lock == 0) && (s_ofdm_lock == 0)) {
			RTV_REG_MAP_SEL(OFDM_PAGE);
			RTV_REG_SET(0x19, 0xC0); 
			RTV_REG_SET(0x19, 0x80); 

		 } else if (oper_div == 2 && (s_ofdm_lock == 0)) {
			RTV_REG_MAP_SEL(FEC_PAGE);
			RTV_REG_SET(0xFB, 0xDB); 
			RTV_REG_SET(0xFB, 0xDA);
		 }
	}

	RTV_GUARD_FREE;

	if (cPath == DIVERSITY_SLAVE) 
		rtvMTV23x_Diversity_Path_Select(DIVERSITY_SLAVE);

}

#endif

INT rtvMTV23x_Initialize(enum E_RTV_BANDWIDTH_TYPE eBandwidthType)
{
	INT nRet = RTV_SUCCESS;
#if defined(RTV_SPI_HIGH_SPEED_ENABLE)
	U8 nReg = 0;
	INT i = 0;
#endif

	g_nIsdbtPrevAntennaLevel = 0;
	g_nIsdbtPrevAntennaLevel_1seg = 0;
	g_eRtvServiceType = MAX_NUM_RTV_SERVICE;
#ifdef RTV_DUAL_DIVERISTY_ENABLE
	g_eRtvServiceType_slave = MAX_NUM_RTV_SERVICE;
#endif
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	g_bRtvSpiHighSpeed = FALSE;
#endif

	nRet = rtv_InitSystem();
	if (nRet != RTV_SUCCESS)
		return nRet;

	nRet = rtvRF_Initilize(eBandwidthType);
	if (nRet != RTV_SUCCESS)
		goto mtv23x_init_exit;

	/* Must after rtv_InitSystem(). */
	rtv_InitDemod();

	rtv_SoftReset();

#if defined(RTV_SPI_HIGH_SPEED_ENABLE)
	RTV_REG_MAP_SEL(RF_PAGE);
	RTV_REG_SET(0xB6, 0x05);
	RTV_REG_SET(0xB6, 0x25); /* DIV8 */

	for (i = 0; i < 10; i++) {
		nReg = RTV_REG_GET(0xB6);

		if (nReg == 0x25) {
			g_bRtvSpiHighSpeed = TRUE;
			break;
		} else
			RTV_DELAY_MS(1);
	}

	if (i == 10) {
		RTV_DBGMSG0("[rtvMTV23x_Initialize] SPI_IF is unstable.\n");
		nRet = RTV_POWER_ON_CHECK_ERROR;
		goto mtv23x_init_exit;
	}
#endif

mtv23x_init_exit:

	return nRet;
}

