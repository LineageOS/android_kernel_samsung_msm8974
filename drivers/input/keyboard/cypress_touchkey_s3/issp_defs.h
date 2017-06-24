 /* filename: ISSP_Defs.h
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

#ifndef INC_ISSP_DEFS

#define INC_ISSP_DEFS



#include "issp_directives.h"



 /* Block-Verify Uses 64-Bytes of RAM */

/* kevi added + */
#define MAX_TARGET_DATABUFF_LEN		1024
/* Address Numbers for Bytes within a Block */
#define TARGET_DATABUFF_LEN    128/*64*/


/*
 The number of Flash blocks in each part is defined here. This is used in

 main programming loop when programming and verifying the blocks.

*/
/* For example, radon is 2 */
#define NUM_BANKS                     1
/* Block Numbers for Program Data */
#define BLOCKS_PER_BANK              64/*128*/

#define SECURITY_BYTES_PER_BANK      64


#endif /*(INC_ISSP_DEFS)*/
#endif /*(PROJECT_REV_)*/
 /*end of file ISSP_Defs.h*/
