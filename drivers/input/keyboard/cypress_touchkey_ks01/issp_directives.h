/* filename: ISSP_Directives.h */
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

/*--------------------- Compiler Directives ----------------------------------
*/
#ifndef INC_ISSP_DIRECTIVES
#define INC_ISSP_DIRECTIVES

 /*
 This directive will enable a Genral Purpose test-point on P1.7
 It can be toggled as needed to measure timing, execution, etc...
 A "Test Point" sets a GPIO pin of the host processor high or low. This GPIO
 pin can be observed with an oscilloscope to verify the timing of key
 programming steps. TPs have been added in main() that set Port 0, pin 1
 high during bulk erase, during each block write and during security write.
 The timing of these programming steps should be verified as correct as part
 of the validation process of the final program.

 ****************************************************************************
 ************* USER ATTENTION REQUIRED: TARGET SUPPLY VOLTAGE ***************
 ****************************************************************************
 This directive causes the proper Initialization vector #3 to be sent
 to the Target, based on what the Target Vdd programming voltage will
 be. Either 5V (if #define enabled) or 3.3V (if #define disabled).
 */

/*
#define TARGET_VOLTAGE_IS_5V
*/

/*
 ****************************************************************************
 **************** USER ATTENTION REQUIRED: PROGRAMMING MODE *****************
 ****************************************************************************
 This directive selects whether code that uses reset programming mode or code
 that uses power cycle programming is use. Reset programming mode uses the
 external reset pin (XRES) to enter programming mode. Power cycle programming
 mode uses the power-on reset to enter programming mode.
 Applying signals to various pins on the target device must be done in a
 deliberate order when using power cycle mode. Otherwise, high signals to GPIO
 pins on the target will power the PSoC through the protection diodes.
*/

#define CY8C20236
/*
#define RESET_MODE

*/

/*
#define TX_ON

#define CY8C20236
*/

/*
#define LCD_ON
*/

 /*****************************************************************************
 ****************** USER ATTENTION REQUIRED: TARGET PSOC ********************
 ****************************************************************************
 The directives below enable support for various PSoC devices. The root part
 number to be programmed should be un-commented so that its value becomes
 defined.  All other devices should be commented out.
 Select one device to be supported below:

 **** CY821x34 devices ****
*/


/*
#define CY8C21234
#define CY8C21334
#define CY8C21434
#define CY8C21534

*/

#define CY8C20x66
			/* Quark */


/*-----------------------------------------------------------------------------
   The directives below are used for Krypton.
 See the Krypton programming spec 001-15870 rev *A for more details. (The
 spec uses "mnemonics" instead of "directives"
-----------------------------------------------------------------------------*/
#ifdef CY8C20x66
    #define TSYNC

	#define ID_SETUP_1/*PTJ: ID_SETUP_1 is similar to init1_v*/
	#define ID_SETUP_2/*PTJ: ID_SETUP_2 is similar to init2_v*/
	#define SET_BLOCK_NUM
	#define CHECKSUM_SETUP/*PTJ: CHECKSUM_SETUP_20x66 is
				the same as CHECKSUM-SETUP in 001-15870*/
	#define READ_CHECKSUM
	#define PROGRAM_AND_VERIFY/*PTJ: PROGRAM_BLOCK_20x66 is the
				same as PROGRAM-AND-VERIFY in 001-15870*/
	#define ERASE
	#define	SECURE
	#define READ_SECURITY
	#define READ_WRITE_SETUP
	#define WRITE_BYTE
	#define VERIFY_SETUP
	#define READ_STATUS
	#define READ_BYTE
	/*READ_ID_WORD	PTJ: 3rd Party Progrmmer will have to write
	code to handle this directive, we do it out own way in this
	code, see read_id_v*/
#endif
/*-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// The directives below are used to define various sets of vectors that differ
// for more than one set of PSoC parts.
//-----------------------------------------------------------------------------
// **** Select a Checksum Setup Vector ****/
#ifdef CY8C21x23
    #define CHECKSUM_SETUP_21_27
#endif
#ifdef CY8C21x34
    #define CHECKSUM_SETUP_21_27
#endif
#ifdef CY8C24x23A
    #define CHECKSUM_SETUP_24_24A
#endif
#ifdef CY8C24x94
    #define CHECKSUM_SETUP_24_29
#endif
#ifdef CY8C27x43
    #define CHECKSUM_SETUP_21_27
#endif
#ifdef CY8C29x66
    #define CHECKSUM_SETUP_24_29
#endif

/*** Select a Program Block Vector ****/

#ifdef CY8C21x23
    #define PROGRAM_BLOCK_21_24_29
#endif
#ifdef CY8C21x34
    #define PROGRAM_BLOCK_21_24_29
#endif
#ifdef CY8C24x23A
    #define PROGRAM_BLOCK_21_24_29
#endif
#ifdef CY8C24x94
    #define PROGRAM_BLOCK_21_24_29
#endif
#ifdef CY8C27x43
    #define PROGRAM_BLOCK_27
#endif
#ifdef CY8C29x66
    #define PROGRAM_BLOCK_21_24_29
#endif

/*-----------------------------------------------------------------------------
// The directives below are used to control switching banks if the device is
// has multiple banks of Flash.
//-----------------------------------------------------------------------------
// **** Select a Checksum Setup Vector ****/
#ifdef CY8C24x94
    #define MULTI_BANK
#endif
#ifdef CY8C29x66
    #define MULTI_BANK
#endif

#endif  /*(INC_ISSP_DIRECTIVES)*/
#endif  /*(PROJECT_REV_) */
/* end of file ISSP_Directives.h */

