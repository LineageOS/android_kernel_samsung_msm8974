/* //filename: ISSP_Delays.h
*/
#include "issp_revision.h"
#ifdef PROJECT_REV_304
/*
* Copyright 2006-2007, Cypress Semiconductor Corporation.

* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
* MA  02110-1301, USA.
*/

#ifndef INC_ISSP_DELAYS
#define INC_ISSP_DELAYS

/*
 The Delay() routine, in ISSP_Driver_Routines.c, has a delay of n+3 usec,
 where n is the value passed to the routine.  This is true for the m8c micro-
 processor in the PSoC when it is running at a CPU clock of 24MHz.

 PROCESSOR_SPECIFIC
 If another processor is used, or if the m8c is running at a slower clock
 speed, then the delay parameters will be different. This file makes changing
 the delays simpiler when porting the program to other processors.
*/

/* DELAY_M is the slope of the Delay = Mx + B equation
*/
#define DELAY_M    1
/* DELAY_B is the offset of the delay in Delay = Mx + B.
*/
#define DELAY_B    3
/*

 CAUTION:
 For the above parameters the minimum delay value is 3 (this would result in
 0 being passed for a minimum delay. A value less than 3 would actually
 create a negative number, causing a very long delay


 TRANSITION_TIMEOUT is a loop counter for a 100msec timeout when waiting for
 a high-to-low transition. This is used in the polling loop of
 fDetectHiLoTransition(). Each pass through the loop takes approximately 15
 usec. 100 msec is about 6740 loops. 13480
*/
#define TRANSITION_TIMEOUT     (65535*10)
/*
 XRES_DELAY is the time duration for which XRES is asserted. This defines
 a 63 usec delay.
 The minimum Xres time (from the device datasheet) is 10 usec.
*/
#define XRES_CLK_DELAY    ((63 - DELAY_B) / DELAY_M)
/*
 POWER_CYCLE_DELAY is the time required when power is cycled to the target
 device to create a power reset after programming has been completed. The
 actual time of this delay will vary from system to system depending on the
 bypass capacitor size.  A delay of 150 usec is used here.
*/
#define POWER_CYCLE_DELAY ((150 - DELAY_B) / DELAY_M)
/*
 DELAY_100us delays 100 usec. This is used in fXRESInitializeTargetForISSP to
 time the wait for Vdd to become stable after a power up.  A loop runs 10 of
 these for a total delay of 1 msec.
*/
#define DELAY100us        ((100 - DELAY_B) / DELAY_M)

#endif /* (INC_ISSP_DELAYS)
*/
#endif /* (PROJECT_REV_)
*/
/* end of file ISSP_Delays.h */
