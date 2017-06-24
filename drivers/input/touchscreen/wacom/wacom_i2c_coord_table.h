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

#include "table_msm8974.h"

/* Origin Shift */
#if defined(CONFIG_SEC_VIENNA_PROJECT) || defined(CONFIG_SEC_LT03_PROJECT)
short origin_offset[] = {0, 0};
#else
short origin_offset[] = {752, 643};
#endif

/* Tilt offset */
/* 0: Left, 1: Right */
/* 0: Portrait 0, 1: Landscape 90, 2: Portrait 180 3: Landscape 270*/
short tilt_offsetX_B887[MAX_HAND][MAX_ROTATION] = \
{{0, 0, 0, 0, }, {-180, 85, 100, -50, } };
/*{{85, 100, -50, -85, }, {-180, 0, 100, 0, } };*/
short tilt_offsetY_B887[MAX_HAND][MAX_ROTATION] = \
{{0, 0, 0, 0, }, {-140, -90, 120, 100, } };
/*{{-90, 120, 100, -80, }, {-50, -180, 120, 0, } };*/

char *tuning_version_B887 = "1320";


short tilt_offsetX_B713[MAX_HAND][MAX_ROTATION] = \
{{85, 100, -50, -85, }, {-85, 85, 100, -50, } };
short tilt_offsetY_B713[MAX_HAND][MAX_ROTATION] = \
{{-90, 120, 100, -80, }, {-80, -90, 120, 100, } };

char *tuning_version_B713 = "0730";

/* Distance Offset Table */
short *tableX[MAX_HAND][MAX_ROTATION] = \
	{{TblX_PLeft_44, TblX_CCW_LLeft_44, TblX_CW_LRight_44, TblX_PRight_44},
	{TblX_PRight_44, TblX_PLeft_44, TblX_CCW_LLeft_44, TblX_CW_LRight_44} };

short *tableY[MAX_HAND][MAX_ROTATION] = \
	{{TblY_PLeft_44, TblY_CCW_LLeft_44, TblY_CW_LRight_44, TblY_PRight_44},
	{TblY_PRight_44, TblY_PLeft_44, TblY_CCW_LLeft_44, TblY_CW_LRight_44} };
