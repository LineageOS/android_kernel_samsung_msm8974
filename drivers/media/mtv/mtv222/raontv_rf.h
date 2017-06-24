/******************************************************************************
* (c) COPYRIGHT 2010 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE 	  : RAONTECH TV RF services header file. 
*
* FILENAME    : raontv_rf.h
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

#ifndef __RAONTV_RF_H__
#define __RAONTV_RF_H__

#ifdef __cplusplus 
extern "C"{ 
#endif  

#include "raontv_internal.h"

INT  rtvRF_SetFrequency(E_RTV_TV_MODE_TYPE eTvMode, UINT nChNum, U32 dwFreqKHz);
INT  rtvRF_ChangeAdcClock(E_RTV_TV_MODE_TYPE eTvMode, E_RTV_ADC_CLK_FREQ_TYPE eAdcClkFreqType, S16 dwIFFreq);
void rtvRF_ConfigurePowerType(E_RTV_TV_MODE_TYPE eTvMode);
INT  rtvRF_Initilize(E_RTV_TV_MODE_TYPE eTvMode);

#ifdef __cplusplus 
} 
#endif 

#endif /* __RAONTV_RF_H__ */

