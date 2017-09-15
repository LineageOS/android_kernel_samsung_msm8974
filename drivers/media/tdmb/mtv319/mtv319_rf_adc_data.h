/*
*
* File name: mtv319_rf_adc_data.h
*
* Description : MTV319 T-DMB RF ADC data header file.
*
* Copyright (C) (2013, RAONTECH)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#if (RTV_SRC_CLK_FREQ_KHz == 19200)
	static const struct RTV_ADC_CFG_INFO g_atAdcCfgTbl_TDMB[] = {
		/* FEBD REFD	TNCO */
		{0x05A, 0x04, 0x3A41148E} /* 175280: 7A */,
		{0x05A, 0x04, 0x3A41148E} /* 177008: 7B */,
		{0x050, 0x04, 0x41893720} /* 178736: 7C */,
		{0x050, 0x04, 0x41893720} /* 181280: 8A */,
		{0x050, 0x04, 0x41893720} /* 183008: 8B */,
		{0x05A, 0x04, 0x3A41148E} /* 184736: 8C */,
		{0x05A, 0x04, 0x3A41148E} /* 187280: 9A */,
		{0x060, 0x04, 0x369D0345} /* 189008: 9B */,
		{0x050, 0x04, 0x41893720} /* 190736: 9C */,
		{0x05A, 0x04, 0x3A41148E} /* 193280: 10A */,
		{0x05A, 0x04, 0x3A41148E} /* 195008: 10B */,
		{0x060, 0x04, 0x369D0345} /* 196736: 10C */,
		{0x05A, 0x04, 0x3A41148E} /* 199280: 11A */,
		{0x050, 0x04, 0x41893720} /* 201008: 11B */,
		{0x050, 0x04, 0x41893720} /* 202736: 11C */,
		{0x050, 0x04, 0x41893720} /* 205280: 12A */,
		{0x060, 0x04, 0x369D0345} /* 207008: 12B */,
		{0x05A, 0x04, 0x3A41148E} /* 208736: 12C */,
		{0x050, 0x04, 0x41893720} /* 211280: 13A */,
		{0x060, 0x04, 0x369D0345} /* 213008: 13B */,
		{0x050, 0x04, 0x41893720} /* 214736: 13C */
	};

#elif (RTV_SRC_CLK_FREQ_KHz == 24576)
	static const struct RTV_ADC_CFG_INFO g_atAdcCfgTbl_TDMB[] = {
		/* FEBD REFD    TNCO */
		{0x0E8, 0x0F, 0x4234F722} /* 175280: 7A */,
		{0x0AA, 0x0B, 0x42424238} /* 177008: 7B */,
		{0x060, 0x06, 0x3FFFFFF6} /* 178736: 7C */,
		{0x07D, 0x08, 0x41893742} /* 181280: 8A */,
		{0x0E5, 0x0E, 0x3E9A4546} /* 183008: 8B */,
		{0x063, 0x06, 0x3E0F83D7} /* 184736: 8C */,
		{0x0F8, 0x0F, 0x3DEF7BD5} /* 187280: 9A */,
		{0x063, 0x06, 0x3E0F83D7} /* 189008: 9B */,
		{0x0AC, 0x0A, 0x3B88EE1B} /* 190736: 9C */,
		{0x0C6, 0x0C, 0x3E0F83D7} /* 193280: 10A */,
		{0x0C8, 0x0C, 0x3D70A3CE} /* 195008: 10B */,
		{0x07D, 0x08, 0x41893742} /* 196736: 10C */,
		{0x07C, 0x08, 0x42108417} /* 199280: 11A */,
		{0x07D, 0x08, 0x41893742} /* 201008: 11B */,
		{0x0E1, 0x0D, 0x3B2A18FF} /* 202736: 11C */,
		{0x0CB, 0x0B, 0x377CD8DF} /* 205280: 12A */,
		{0x0B3, 0x0B, 0x3EED685D} /* 207008: 12B */,
		{0x0AF, 0x0B, 0x405D9F6A} /* 208736: 12C */,
		{0x0B3, 0x0B, 0x3EED685D} /* 211280: 13A */,
		{0x0B6, 0x0B, 0x3DE3DE34} /* 213008: 13B */,
		{0x0B6, 0x0B, 0x3DE3DE34} /* 214736: 13C */
	};
#else
	#error "Unsupport clock freqency!"
#endif