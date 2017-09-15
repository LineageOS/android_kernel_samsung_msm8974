/* filename: ISSP_Extern.h */
#include "issp_revision.h"
#include <linux/i2c/touchkey_i2c.h>

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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
* MA  02110-1301, USA.
*/

#ifndef INC_ISSP_EXTERN
#define INC_ISSP_EXTERN

#include "issp_directives.h"
#include "issp_defs.h"

extern int ISSP_main(struct cypress_touchkey_info *info);

extern signed char fXRESInitializeTargetForISSP(void);
extern signed char fPowerCycleInitializeTargetForISSP(struct cypress_touchkey_info *info);
extern signed char fEraseTarget(void);
extern unsigned int iLoadTarget(void);
extern void ReStartTarget(struct cypress_touchkey_info *info);
extern signed char fVerifySiliconID(void);
extern signed char fAccTargetBankChecksum(unsigned int *);
extern void SetBankNumber(unsigned char);
extern signed char fProgramTargetBlock(unsigned char, unsigned char);
extern signed char fVerifyTargetBlock(unsigned char, unsigned char);
/* PTJ: VERIFY-SETUP */
extern signed char fVerifySetup(unsigned char, unsigned char);
/* PTJ: read bytes after VERIFY-SETUP */
extern signed char fReadByteLoop(void);
extern signed char fSecureTargetFlash(void);
extern signed char fReadStatus(void);/*PTJ:R-STATUS*/
extern signed char fReadCalRegisters(void);
extern signed char fReadWriteSetup(void);/*PTJ:RW-SETUP*/
extern signed char fReadSecurity(void);/*PTJ: READ-SECURITY*/
extern signed char fSyncDisable(void);/*PTJ: SYNC-DISABLE rev 307*/
extern signed char fSyncEnable(void);/*PTJ: SYNC-ENABLE rev 307 */

extern void InitTargetTestData(void);
extern void LoadArrayWithSecurityData(unsigned char,
						unsigned char, unsigned char);

extern void LoadProgramData(struct cypress_touchkey_info *info, unsigned char, unsigned char);
extern signed char fLoadSecurityData(unsigned char);
extern void Delay(unsigned int);
extern unsigned char fSDATACheck(void);
extern void SCLKHigh(void);
extern void SCLKLow(void);
#ifndef RESET_MODE  /*only needed when power cycle mode*/
extern void SetSCLKHiZ(void);
#endif
extern void SetSCLKStrong(void);

extern void SetSDATAHigh(void);
extern void SetSDATALow(void);

extern void SetSDATAHiZ(void);
extern void SetSDATAOpenDrain(void);
extern void SetSDATAStrong(void);

extern void AssertXRES(void);
extern void DeassertXRES(void);
extern void SetXRESStrong(void);
extern void ApplyTargetVDD(struct cypress_touchkey_info *info);
extern void RemoveTargetVDD(struct cypress_touchkey_info *info);
extern void SetTargetVDDStrong(void);


#ifdef TX_ON
extern void TX8SW_PutHexHalf(char ch);
extern void TX8SW_PutHexByte(unsigned char ch);
extern void TX8SW_PutHexWord(unsigned int ch);
#endif

extern unsigned char   fIsError;
/* Address Numbers for Bytes within a Block */
extern unsigned char TargetDatabufLen;
/* For example, radon is 2 */
extern unsigned char NumBanks;
/* Block Numbers for Program Data */
extern unsigned char BlocksPerBank;
extern unsigned char SecurityBytesPerBank;
extern unsigned char    bTargetDataPtr;
extern unsigned char    abTargetDataOUT[MAX_TARGET_DATABUFF_LEN];
extern unsigned char    TargetDatabufLen;

extern unsigned int system_rev;
#endif  /* (INC_ISSP_EXTERN) */
#endif  /* (PROJECT_REV_) */
/* end of file ISSP_Extern.h */
