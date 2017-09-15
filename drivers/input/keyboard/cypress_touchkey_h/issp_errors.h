/* filename: ISSP_Errors.h */
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
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
* MA  02110-1301, USA.
*/

#ifndef INC_ISSP_ERRORS
#define INC_ISSP_ERRORS

/* The following are defines for error messages from the ISSP program. */
#define PASS           0
/* PASS is used to indicate that a function completed successfully. */
#define ERROR         -1
/*
 ERROR is a generic failure used within lower level functions before the
 error is reported.  This should not be seen as an error that is reported
 from main.
*/
#define INIT_ERROR     1
/* INIT_ERROR means a step in chip initialization failed.
*/
#define SiID_ERROR     2
/*
 SiID_ERROR means that the Silicon ID check failed. This happens if the
 target part does not match the device type that the ISSP program is
 configured for.

*/
#define ERASE_ERROR    3
/*/ ERASE_ERROR means that the bulk erase step failed.
*/
#define BLOCK_ERROR    4
/*
 BLOCK_ERROR means that a step in programming a Flash block or the verify
 of the block failed.

*/
#define VERIFY_ERROR   5
/* VERIFY_ERROR means that the checksum verification failed.*/
#define SECURITY_ERROR 6
/* SECURITY_ERROR means that the write of the security information failed.*/
#define STATUS_ERROR 7


#define CHECKSUM_ERROR 8


#endif  /*(INC_ISSP_ERRORS)*/
#endif  /*(PROJECT_REV_)*/
/*end of file ISSP_Errors.h*/
