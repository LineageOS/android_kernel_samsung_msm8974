/*
 *  wacom_i2c_coord_table.h - Wacom G5 Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _LINUX_WACOM_I2C_COORD_TABLE_H
#define _LINUX_WACOM_I2C_COORD_TABLE_H

/*Tables*/
#if 0
#if defined(CONFIG_MACH_T0) || defined(CONFIG_HA) || \
	defined(CONFIG_MACH_HLLTE) || \
	defined(CONFIG_MACH_HL3G)
/*CONFIG_MACH_T0_EUR_OPEN*/
#include "table-t03g.h"
#endif
#endif
char* tuning_version = "0000";
char *tuning_model = "N750";

/* Origin Shift */
#if defined(CONFIG_MACH_T0)
short origin_offset[] = {676, 724};

short tilt_offsetX_B713[MAX_HAND][MAX_ROTATION] = \
{{85, 100, -50, -85, }, {-85, 85, 100, -50, } };
short tilt_offsetY_B713[MAX_HAND][MAX_ROTATION] = \
{{-90, 120, 100, -80, }, {-80, -90, 120, 100, } };

char *tuning_version_B713 = "0730";
#elif defined(CONFIG_HA)
short origin_offset[] = {752, 643};
#elif defined(CONFIG_MACH_HLLTE) || defined(CONFIG_MACH_HL3G) || \
	defined(CONFIG_MACH_FRESCONEOLTE_CTC) || defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
short origin_offset[] = {740, 760};
#endif
#if 0
/* Distance Offset Table */
short *tableX[MAX_HAND][MAX_ROTATION] = \
	{{TblX_PLeft_44, TblX_CCW_LLeft_44, TblX_CW_LRight_44, TblX_PRight_44},
	{TblX_PRight_44, TblX_PLeft_44, TblX_CCW_LLeft_44, TblX_CW_LRight_44} };

short *tableY[MAX_HAND][MAX_ROTATION] = \
	{{TblY_PLeft_44, TblY_CCW_LLeft_44, TblY_CW_LRight_44, TblY_PRight_44},
	{TblY_PRight_44, TblY_PLeft_44, TblY_CCW_LLeft_44, TblY_CW_LRight_44} };
#endif
#endif /* _LINUX_WACOM_I2C_COORD_TABLE_H */

