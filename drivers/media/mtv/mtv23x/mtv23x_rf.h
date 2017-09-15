/*
*
* File name: mtv23x_rf.h
*
* Description : MTV23x RF services header file.
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

#ifndef __MTV23X_RF_H__
#define __MTV23X_RF_H__

#include "mtv23x_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

extern UINT g_dwRtvPrevChFreqKHz;

INT rtvRF_SetFrequency(enum E_RTV_SERVICE_TYPE eServiceType,
	enum E_RTV_BANDWIDTH_TYPE eLpfBwType, U32 dwChFreqKHz);
INT rtvRF_Initilize(enum E_RTV_BANDWIDTH_TYPE eBandwidthType);

#ifdef __cplusplus
}
#endif

#endif /* __MTV23X_RF_H__ */

