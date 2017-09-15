 /* filename: main.c
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

/* ############################################################################
   ###################  CRITICAL PROJECT CONSTRAINTS   ########################
   ############################################################################

   ISSP programming can only occur within a temperature range of 5C to 50C.
   - This project is written without temperature compensation and using
     programming pulse-widths that match those used by programmers such as the
     Mini-Prog and the ISSP Programmer.
     This means that the die temperature of the PSoC device cannot be outside
     of the above temperature range.
     If a wider temperature range is required, contact your Cypress Semi-
     conductor FAE or sales person for assistance.

   The project can be configured to program devices at 5V or at 3.3V.
   - Initialization of the device is different for different voltages. The
     initialization is hardcoded and can only be set for one voltage range.
     The supported voltages ranges are 3.3V (3.0V to 3.6V) and 5V (4.75V to
     5.25V). See the device datasheet for more details. If varying voltage
     ranges must be supported, contact your Cypress Semiconductor FAE or sales
     person for assistance.
   - ISSP programming for the 2.7V range (2.7V to 3.0V) is not supported.

   This program does not support programming all PSoC Devices
   - It does not support obsoleted PSoC devices. A list of devices that are
	 not supported is shown here:
		 CY8C22x13 - not supported
		 CY8C24x23 - not supported (CY8C24x23A is supported)
		 CY8C25x43 - not supported
		 CY8C26x43 - not supported
   - It does not suport devices that have not been released for sale at the
     time this version was created. If you need to ISSP program a newly released
     device, please contact Cypress Semiconductor Applications, your FAE or
     sales person for assistance.
     The CY8C20x23 devices are not supported at the time of this release.
*/


/*
 PSoC In-System Serial Programming (ISSP) Template
 This PSoC Project is designed to be used as a template for designs that
 require PSoC ISSP Functions.

 This project is based on the AN2026 series of Application Notes. That app
 note should be referenced before any modifications to this project are made.

 The subroutines and files were created in such a way as to allow easy cut &
 paste as needed. There are no customer-specific functions in this project.
 This demo of the code utilizes a PSoC as the Host.

 Some of the subroutines could be merged, or otherwise reduced, but they have
 been written as independently as possible so that the specific steps involved
 within each function can easily be seen. By merging things, some code-space
 savings could be realized.

 As is, and with all features enabled, the project consumes approximately 3500
 bytes of code space, and 19-Bytes of RAM (not including stack usage). The
 Block-Verify requires a 64-Byte buffer for read-back verification. This same
 buffer could be used to hold the (actual) incoming program data.

 Please refer to the compiler-directives file "directives.h" to see the various
 features.

 The pin used in this project are assigned as shown below. The HOST pins are
 arbitrary and any 3 pins could be used (the masks used to control the pins
 must be changed). The TARGET pins cannot be changed, these are fixed function
 pins on the PSoC.
 The PWR pin is used to provide power to the target device
 if power cycle programming mode is used.
 The compiler directive RESET_MODE in ISSP_directives.h
 is used to select the programming mode. This pin could control the enable on
 a voltage regulator, or could control the gate of a FET that is used to turn
 the power to the PSoC on.
 The TP pin is a Test Point pin that can be used signal from the host processor
 that the program has completed certain tasks. Predefined test points are
 included that can be used to observe the timing for bulk erasing, block
 programming and security programming.

      SIGNAL  HOST  TARGET
      ---------------------
      SDATA   P1.0   P1.0
      SCLK    P1.1   P1.1
      XRES    P2.0   XRES
      PWR     P2.1   Vdd
      TP      P0.7   n/a

 For test & demonstration, this project generates the program data internally.
 It does not take-in the data from an external source such as I2C, UART, SPI,
 etc. However, the program was written in such a way to be portable into such
 designs. The spirit of this project was to keep it stripped to the minimum
 functions required to do the ISSP functions only, thereby making a portable
 framework for integration with other projects.

 The high-level functions have been written in C in order to be portable to
 other processors. The low-level functions that are processor dependent, such
 as toggling pins and implementing specific delays, are all found in the file
 ISSP_Drive_Routines.c. These functions must be converted to equivalent
 functions for the HOST processor.  Care must be taken to meet the timing
 requirements when converting to a new processor. ISSP timing information can
 be found in Application Note AN2026.  All of the sections of this program
 that need to be modified for the host processor have "PROCESSOR_SPECIFIC" in
 the comments. By performing a "Find in files" using "PROCESSOR_SPECIFIC" these
 sections can easily be identified.

 The variables in this project use Hungarian notation. Hungarian prepends a
 lower case letter to each variable that identifies the variable type. The
 prefixes used in this program are defined below:
  b = byte length variable, signed char and unsigned char
  i = 2-byte length variable, signed int and unsigned int
  f = byte length variable used as a flag (TRUE = 0, FALSE != 0)
  ab = an array of byte length variables


 After this program has been ported to the desired host processor the timing
 of the signals must be confirmed.  The maximum SCLK frequency must be checked
 as well as the timing of the bulk erase, block write and security write
 pulses.

 The maximum SCLK frequency for the target device can be found in the device
 datasheet under AC Programming Specifications with a Symbol of "Fsclk".
 An oscilloscope should be used to make sure that no half-cycles (the high
 time or the low time) are shorter than the half-period of the maximum
 freqency. In other words, if the maximum SCLK frequency is 8MHz, there can be
 no high or low pulses shorter than 1/(2*8MHz), or 62.5 nsec.

 The test point (TP) functions, enabled by the define USE_TP, provide an output
 from the host processor that brackets the timing of the internal bulk erase,
 block write and security write programming pulses. An oscilloscope, along with
 break points in the PSoC ICE Debugger should be used to verify the timing of
 the programming.  The Application Note, "Host-Sourced Serial Programming"
 explains how to do these measurements and should be consulted for the expected
 timing of the erase and program pulses.

*/



/*
----------------------------------------------------------------------------
							C main line
----------------------------------------------------------------------------
*/

/*
#include <m8c.h>
#include "PSoCAPI.h"
*/

/*
 ------ Declarations Associated with ISSP Files & Routines -------
     Add these to your project as needed.

*/
#include "issp_extern.h"
#include "issp_directives.h"
#include "issp_defs.h"
#include "issp_errors.h"

#include <linux/module.h>

/* ------------------------------------------------------------------------- */

unsigned char bBankCounter;
unsigned int  iBlockCounter;
unsigned int  iChecksumData;
unsigned int  iChecksumTarget;
/* Address Numbers for Bytes within a Block */
unsigned char TargetDatabufLen = TARGET_DATABUFF_LEN;
/* For example, radon is 2 */
unsigned char NumBanks = NUM_BANKS;
/* Block Numbers for Program Data */
unsigned char BlocksPerBank = BLOCKS_PER_BANK;
unsigned char SecurityBytesPerBank = SECURITY_BYTES_PER_BANK;
/* received data counter */
unsigned int  RxCounter;

unsigned char TopMenu = 0xFF;
char MenuCountdown = 10;

#ifdef TX_ON
void TX8SW_PutHexHalf(char ch)
{
	if (ch >= 10)
		TX8SW_PutChar(ch + 'A'-10);
	else
		TX8SW_PutChar(ch + '0');
}

void TX8SW_PutHexWord(unsigned int ch)
{
	TX8SW_PutSHexByte(ch>>8);
	TX8SW_PutSHexByte(ch&0xff);
}
#endif

/*
 =========================================================================
 ErrorTrap()
 Return is not valid from main for PSOC, so this ErrorTrap routine is used.
 For some systems returning an error code will work best. For those, the
 calls to ErrorTrap() should be replaced with a return(bErrorNumber). For
 other systems another method of reporting an error could be added to this
 function -- such as reporting over a communcations port.
 =========================================================================
*/
void ErrorTrap(struct cypress_touchkey_info *info, unsigned char bErrorNumber)
{
    #ifndef RESET_MODE
	/*
	 Set all pins to highZ to avoid back powering the PSoC through the GPIO
	 protection diodes.
	*/
	SetSCLKHiZ();
	SetSDATAHiZ();
	 /* If Power Cycle programming, turn off the target */
	RemoveTargetVDD(info);
    #endif

	#ifdef TX_ON
		TX8SW_PutCRLF();
		TX8SW_CPutString("ErrorTrap");
		TX8SW_PutSHexByte(bErrorNumber);
	#endif
	pr_err("[TKEY] ErrorTrap..%d\n", bErrorNumber);

	/*	while (1)
		;
	*/

}

/* ========================================================================= */
/* MAIN LOOP                                                                 */
/* Based on the diagram in the AN2026                                        */
/* ========================================================================= */

int ISSP_main(struct cypress_touchkey_info *info)
{
    /*
     -- This example section of commands show the high-level calls to -------
     -- perform Target Initialization, SilcionID Test, Bulk-Erase, Target ---
     -- RAM Load, FLASH-Block Program, and Target Checksum Verification. ----
    */
	pr_err("[TKEY] start..\n");

	SetSCLKHiZ();
	SetSDATAHiZ();
	/* Cycle power on the target to cause a reset */
	RemoveTargetVDD(info);

	#ifdef TX_ON
		TX8SW_Start();
		TX8SW_CPutString("\r\nSee HSSP Spec. 40-95004");
		TX8SW_CPutString("\r\nStart HSSP....");
	#endif

/*
>>>> ISSP Programming Starts Here <<<<

Acquire the device through reset or power cycle
*/

#ifdef RESET_MODE
	 /* Initialize the Host & Target for ISSP operations */
	fIsError = fXRESInitializeTargetForISSP();
	if (fIsError) {
		ErrorTrap(info, fIsError);
		return fIsError;
	}
#else
	 /* Initialize the Host & Target for ISSP operations */
	fIsError = fPowerCycleInitializeTargetForISSP(info);

	if (fIsError){
		ErrorTrap(info, fIsError);
		return fIsError;
	}
#endif
/*#if 0
Run the SiliconID Verification, and proceed according to result.
	fIsError = fVerifySiliconID();
	if (fIsError != 0)
		ErrorTrap(fIsError);
#endif*/
#ifdef TX_ON
	TX8SW_PutCRLF();
	TX8SW_CPutString("End VerifySiliconID");
#endif

	if (info->do_checksum) {
		/*data checksum*/
		pr_err("[TKEY] re run check.\n");
		iChecksumData = info->fw_img->checksum;

		iChecksumTarget = 0;
		for (bBankCounter = 0; bBankCounter < NUM_BANKS; bBankCounter++) {
			fIsError = fAccTargetBankChecksum(&iChecksumTarget);
			if (fIsError) {
				ErrorTrap(info, fIsError);
				return fIsError;
			}
		}
		info->do_checksum = false;
		if ((unsigned short)(iChecksumTarget & 0xffff) ==
				    (unsigned short)(iChecksumData & 0xffff)) {
			    pr_err("[TKEY] do not need fw update.\n");
			    goto out;
		}
		pr_err("[TKEY] checksum fail/////need fw update.!!\n");
	}

	/* Bulk-Erase the Device. */
	fIsError = fEraseTarget();
	if (fIsError) {
		ErrorTrap(info, fIsError);
		return fIsError;		
	}

	#ifdef TX_ON
		TX8SW_PutCRLF();
		TX8SW_CPutString("End EraseTarget");
		TX8SW_PutCRLF();
		TX8SW_CPutString("Program Flash Blocks Start");
		TX8SW_PutCRLF();
	#endif

/* program flash block */

/*
  Program Flash blocks with predetermined data. In the final application
 this data should come from the HEX output of PSoC Designer.
*/
	iChecksumData = 0;     /* Calculte the device checksum as you go*/
	for (bBankCounter = 0; bBankCounter < NUM_BANKS; bBankCounter++) {
		/*PTJ: NUM_BANKS should be 1 for Krypton*/
		for (iBlockCounter = 0; iBlockCounter < BLOCKS_PER_BANK;
			iBlockCounter++) {
			/*printk("Program Loop : iBlockCounter %d\n"
			,iBlockCounter);
			INTLOCK();
			local_irq_save(flags);
			PTJ: READ-WRITE-SETUP used here to select SRAM
			Bank 1, and TSYNC Enable*/
			#ifdef CY8C20x66
			fIsError = fSyncEnable();
			if (fIsError) {
				ErrorTrap(info, fIsError);
				return fIsError;
			}
			fIsError = fReadWriteSetup();
			if (fIsError) {
				/*send write command - swanhan*/
				ErrorTrap(info, fIsError);
				return fIsError;
			}
			#endif
		/*firmware read.
		//LoadProgramData(bBankCounter, (unsigned char)iBlockCounter);
		//PTJ: this loads the Hydra with test data, not the krypton
		//PTJ: this loads the Hydra with test data, not the krypton*/
		LoadProgramData(info, (unsigned char)iBlockCounter, bBankCounter);
		iChecksumData += iLoadTarget();/*PTJ: this loads the Krypton*/
		/*dog_kick();*/
	fIsError = fProgramTargetBlock(bBankCounter,
			(unsigned char)iBlockCounter);
	if (fIsError) {
		ErrorTrap(info, fIsError);
		return fIsError;
		}
#ifdef CY8C20x66 /*PTJ: READ-STATUS after PROGRAM-AND-VERIFY*/
#if defined(CONFIG_KOR_MODEL_SHV_E160S) || defined(CONFIG_KOR_MODEL_SHV_E160K)
		|| defined(CONFIG_KOR_MODEL_SHV_E160L)
		msleep(50);
#endif
		fIsError = fReadStatus();
		if (fIsError) {
			ErrorTrap(info, fIsError);
			return fIsError;
		}
#endif
	}
	}
/*
PTJ: Doing Verify
PTJ: this code isnt needed in the program flow
because we use PROGRAM-AND-VERIFY (ProgramAndVerify SROM Func)
PTJ: which has Verify built into it.
Verify included for completeness in case host desires to do
a stand-alone verify at a later date.
*/
	for (bBankCounter = 0; bBankCounter < NUM_BANKS; bBankCounter++) {
		/*PTJ: READ-WRITE-SETUP used here to select SRAM Bank 1*/
#ifdef CY8C20x66
	fIsError = fSyncEnable();
	if (fIsError) {/*PTJ: 307, added for tsync enable testing.*/
		ErrorTrap(info, fIsError);
		return fIsError;
	}
	fIsError = fReadWriteSetup();
	if (fIsError) {
		ErrorTrap(info, fIsError);
		return fIsError;
	}
#endif
	/*Load one bank of security data from hex file into buffer*/
	fIsError = fLoadSecurityData(bBankCounter);
	if (fIsError) {
		ErrorTrap(info, fIsError);
		return fIsError;
	}
	/*Secure one bank of the target flash*/
	fIsError = fSecureTargetFlash();
	if (fIsError) {
		ErrorTrap(info, fIsError);
		return fIsError;
	}
	}
    /*printk("Program security data END\n");*/
/*
  Program security data into target PSoC. In the final application this
 data should come from the HEX output of PSoC Designer.
*/
	fIsError = fLoadSecurityData(bBankCounter);
	if (fIsError) {
		ErrorTrap(info, fIsError);
		return fIsError;
	}

    #ifdef CY8C20x66
	fIsError = fReadSecurity();
	if (fIsError) {
		ErrorTrap(info, fIsError);
		return fIsError;
	}
    #endif

/* checksum */
    /*=======================================================//
    //PTJ: Doing Checksum after READ-SECURITY*/
	iChecksumTarget = 0;
for (bBankCounter = 0; bBankCounter < NUM_BANKS; bBankCounter++) {
	fIsError = fAccTargetBankChecksum(&iChecksumTarget);
	if (fIsError) {
		ErrorTrap(info, fIsError);
		return fIsError;
	}
}
pr_err("Checksum : iChecksumTarget (0x%X)\n", (unsigned char)iChecksumTarget);
pr_err("Checksum : iChecksumData (0x%X)\n", (unsigned char)iChecksumData);

if ((unsigned short)(iChecksumTarget&0xffff) !=
	(unsigned short) (iChecksumData & 0xffff)) {
	ErrorTrap(info, VERIFY_ERROR);
	return fIsError;
}

    /* *** SUCCESS ***
    // At this point, the Target has been successfully Initialize, ID-Checked,
    // Bulk-Erased, Block-Loaded, Block-Programmed, Block-Verified, and Device-
    // Checksum Verified.
    // You may want to restart Your Target PSoC Here.*/
out:
	ReStartTarget(info);
	return 0;
}
#endif  /* (PROJECT_REV_) */

