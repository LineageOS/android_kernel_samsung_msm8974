/******************************************************************************
* (c) COPYRIGHT 2013 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE		: MTV23x services source file.
*
* FILENAME	: mtv23x_new_high_speed.c
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
* 03/03/2013  Yang, Maverick       Created.
******************************************************************************/

#include "mtv23x_rf.h"
#include "mtv23x_internal.h"

U8 g_bRtvIntrMaskReg;
U8 g_bRtvPage;
UINT g_nRtvThresholdSize;

/* #define INTERFACE_TEST */

/* #define CHECK_REV_NUM */

INT rtv_InitSystem(void)
{
	int i;
	U8 read0, read1;
	U8 rev_num;
	U8 ALDO_OUT = 0, DLDO_OUT = 0;
		
#ifdef RTV_DUAL_DIVERISTY_ENABLE
	U8 S_ALDO_OUT = 0, S_DLDO_OUT = 0;
#endif
	g_bRtvIntrMaskReg = 0x3F;

#if defined(RTV_IF_SPI) || defined(RTV_IF_SPI_TSIFx) || defined(RTV_IF_EBI2)
	#define WR27_VAL	(0x50|SPI_INTR_POL_ACTIVE)
	#define WR29_VAL	0x10

	for (i = 0; i < 100; i++) {
		RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
		RTV_REG_SET(0x29, WR29_VAL); /* BUFSEL first! */
		RTV_REG_SET(0x27, WR27_VAL);

		read0 = RTV_REG_GET(0x27);
		read1 = RTV_REG_GET(0x29);

#if defined(RTV_SPI_HIGH_SPEED_ENABLE)
		RTV_REG_MAP_SEL(RF_PAGE);
#if (RTV_SRC_CLK_FREQ_KHz == 19200)
		RTV_REG_SET(0xB6, 0x04);
		RTV_REG_SET(0xB6, 0x24); /* DIV8 */
#else
		RTV_REG_SET(0xB6, 0x05);
		RTV_REG_SET(0xB6, 0x25); /* DIV8 */
#endif

#endif

		RTV_REG_MAP_SEL(TOP_PAGE);
		RTV_REG_SET(0x0C, 0xC3);
		RTV_DBGMSG2("read0(0x%02X), read1(0x%02X)\n", read0, read1);

		if ((read0 == WR27_VAL) && (read1 == WR29_VAL)) {
			RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
			RTV_REG_SET(0x21, 0x87);
			RTV_REG_SET(0x22, 0x00);
			goto RTV_POWER_ON_SUCCESS;
		}

		RTV_DBGMSG1("[rtv_InitSystem] Power On wait: %d\n", i);
		RTV_DELAY_MS(5);
	}
#else
	RTV_REG_MAP_SEL(TOP_PAGE);
	RTV_REG_SET(0x0C, 0xC3);
	for (i = 0; i < 100; i++) {
		read0 = RTV_REG_GET(0x00);
		read1 = RTV_REG_GET(0x01);
		RTV_DBGMSG2("read0(0x%02X), read1(0x%02X)\n", read0, read1);

		if ((read0 == 0xC6))
			goto RTV_POWER_ON_SUCCESS;

		RTV_DBGMSG1("[rtv_InitSystem] Power On wait: %d\n", i);
		RTV_DELAY_MS(5);
	}
#endif

	RTV_DBGMSG1("rtv_InitSystem: Power On Check error: %d\n", i);
	return RTV_POWER_ON_CHECK_ERROR;

RTV_POWER_ON_SUCCESS:


#if 1
	RTV_REG_MAP_SEL(RF_PAGE);
	rev_num = (RTV_REG_GET(0x10) & 0xF0) >> 4 ;

#ifdef CHECK_REV_NUM
	RTV_DBGMSG1("[rtv_InitSystem] REV number (%d)\n", rev_num);
#endif
	if (rev_num >= 0x04) {
		RTV_REG_MASK_SET(0x3B, 0x01, 0x01);
		RTV_REG_MASK_SET(0x32, 0x01, 0x01);
	}
#endif

#ifdef INTERFACE_TEST
#if defined(RTV_IF_SPI) || defined(RTV_IF_SPI_TSIFx) || defined(RTV_IF_EBI2)
	
		RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
		for (i = 0; i < 100; i++) {
			RTV_REG_SET(0x22, 0x55);
			read0 = RTV_REG_GET(0x22);
			RTV_REG_SET(0x22, 0xAA);
			read1 = RTV_REG_GET(0x22);
	
			RTV_DBGMSG2("Before Power Setup :readSPI22_55(0x%02X), readSPI22_AA(0x%02X)\n",
						read0, read1);
		}
	
		RTV_REG_MAP_SEL(RF_PAGE);
		for (i = 0; i < 100; i++) {
			RTV_REG_SET(0x20, 0x55);
			read0 = RTV_REG_GET(0x20);
			RTV_REG_SET(0x20, 0xAA);
			read1 = RTV_REG_GET(0x20);
	
			RTV_DBGMSG2("Before Power Setup :readRF20_55(0x%02X), readRF20_AA(0x%02X)\n",
					read0, read1);
		}
#else
		RTV_REG_MAP_SEL(RF_PAGE);
	
		for (i = 0; i < 100; i++) {
			RTV_REG_SET(0x20, 0x55);
			read0 = RTV_REG_GET(0x20);
			RTV_REG_SET(0x20, 0xAA);
			read1 = RTV_REG_GET(0x20);
	
			RTV_DBGMSG2("Before Power Setup :readRF20_55(0x%02X), readRF20_AA(0x%02X)\n",
				read0, read1);
		}
#endif
#endif /* INTERFACE_TEST */


	ALDO_OUT = 6;
	DLDO_OUT = 1;

#ifdef RTV_DUAL_DIVERISTY_ENABLE
	S_ALDO_OUT = 6;
	S_DLDO_OUT = 1;
#endif

	RTV_REG_MAP_SEL(RF_PAGE);

#ifdef RTV_DUAL_DIVERISTY_ENABLE
	if (rtvMTV23x_Get_Diversity_Current_path() == DIVERSITY_MASTER) {
#endif
		RTV_REG_MASK_SET(0xC8, 0x80, ((ALDO_OUT & 0x04) << 5));
		RTV_REG_MASK_SET(0xD1, 0x80, ((ALDO_OUT & 0x02) << 6));
		RTV_REG_MASK_SET(0xD2, 0x80, ((ALDO_OUT & 0x01) << 7));

		RTV_REG_MASK_SET(0xD3, 0x80, ((DLDO_OUT & 0x04) << 5));
		RTV_REG_MASK_SET(0xD5, 0x80, ((DLDO_OUT & 0x02) << 6));
		RTV_REG_MASK_SET(0xD6, 0x80, ((DLDO_OUT & 0x01) << 7));
#ifdef RTV_DUAL_DIVERISTY_ENABLE
	} else {
		RTV_REG_MASK_SET(0xC8, 0x80, ((S_ALDO_OUT & 0x04) << 5));
		RTV_REG_MASK_SET(0xD1, 0x80, ((S_ALDO_OUT & 0x02) << 6));
		RTV_REG_MASK_SET(0xD2, 0x80, ((S_ALDO_OUT & 0x01) << 7));

		RTV_REG_MASK_SET(0xD3, 0x80, ((S_DLDO_OUT & 0x04) << 5));
		RTV_REG_MASK_SET(0xD5, 0x80, ((S_DLDO_OUT & 0x02) << 6));
		RTV_REG_MASK_SET(0xD6, 0x80, ((S_DLDO_OUT & 0x01) << 7));
	}
#endif

	RTV_DELAY_MS(10);

	RTV_REG_MASK_SET(0xC9, 0x80, 0x80);

#if defined(RTV_EXT_POWER_MODE)
	RTV_REG_SET(0xCD, 0xCF);
	#ifdef RTV_DUAL_DIVERISTY_ENABLE
	RTV_REG_SET(0xCE, 0x35);
	#else
	RTV_REG_SET(0xCE, 0xB5);
	#endif
#else /* Internal LDO Mode */
	RTV_REG_SET(0xCD, 0x4F);
	RTV_REG_SET(0xCE, 0x35);
#endif

#ifdef INTERFACE_TEST
#if defined(RTV_IF_SPI) || defined(RTV_IF_SPI_TSIFx) || defined(RTV_IF_EBI2)

	RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
	for (i = 0; i < 100; i++) {
		RTV_REG_SET(0x22, 0x55);
		read0 = RTV_REG_GET(0x22);
		RTV_REG_SET(0x22, 0xAA);
		read1 = RTV_REG_GET(0x22);

		RTV_DBGMSG2("After Power Setup :readSPI22_55(0x%02X), readSPI22_AA(0x%02X)\n",
					read0, read1);
	}

	RTV_REG_MAP_SEL(RF_PAGE);
	for (i = 0; i < 100; i++) {
		RTV_REG_SET(0x20, 0x55);
		read0 = RTV_REG_GET(0x20);
		RTV_REG_SET(0x20, 0xAA);
		read1 = RTV_REG_GET(0x20);

		RTV_DBGMSG2("After Power Setup :readRF20_55(0x%02X), readRF20_AA(0x%02X)\n",
				read0, read1);
	}
#else
	RTV_REG_MAP_SEL(RF_PAGE);

	for (i = 0; i < 100; i++) {
		RTV_REG_SET(0x20, 0x55);
		read0 = RTV_REG_GET(0x20);
		RTV_REG_SET(0x20, 0xAA);
		read1 = RTV_REG_GET(0x20);

		RTV_DBGMSG2("After Power Setup :readRF20_55(0x%02X), readRF20_AA(0x%02X)\n",
			read0, read1);
	}
#endif
#endif /* INTERFACE_TEST */

	return RTV_SUCCESS;
}