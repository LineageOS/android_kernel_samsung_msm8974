/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
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
 */

#if defined (CONFIG_MACH_H3GDUOS_CTC)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10286, 423, 412, 731, 9062, -100, -301, 1065, 10704}
#elif defined (CONFIG_MACH_JSGLTE_CHN_CMCC)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10138, 635, -346, 763, 9952, 158, -599, 554, 9944}
#elif defined (CONFIG_MACH_HLTE_CHN_CMCC)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10289, 169, 104, 763, 8994, -241, 90, 1146, 10767}
#elif defined (CONFIG_MACH_H3GDUOS_CU)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10279, 367, 440, 763, 9028, -94, 204, 1048, 10769}
#elif defined (CONFIG_MACH_HLTEEUR)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10324, 453, 415, 622, 9019, -65, -564, 1323, 10708}
#elif defined (CONFIG_MACH_HLTEVZW)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10326, 149, 136, 840, 9061, -73, 177, 1074, 10670}
#elif defined (CONFIG_MACH_HLTESPR)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10323, 180, 180, 809, 9080, 3, -24, 1174, 10658}
#elif defined (CONFIG_MACH_HLTEATT)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10292, 517, 147, 599, 9069, -136, -238, 649, 10705}
#elif defined (CONFIG_MACH_HLTETMO)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10301, 354, 170, 800, 9111, -41, 213, 1113, 10648}
#elif defined (CONFIG_MACH_HLTEKDI)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10613, 480, 164, 795, 9171, -15, -470, 1323, 10613}
#elif defined (CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_JS01LTESBM)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{9871, -47, 13, 416, 9559, 48, -307, -240, 10590}
#elif defined (CONFIG_MACH_HLTEUSC)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10263, 286, 186, 867, 9108, -52, 309, 1017, 10683}
#elif defined (CONFIG_MACH_HLTEDCM)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10298, 293, 292, 723, 9031, -17, 126, 1025, 10749}
#elif defined (CONFIG_MACH_HLTESKT) || defined(CONFIG_MACH_FRESCOLTESKT)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10340, 324, 225, 953, 8996, -7, -65, 508, 10743}
#elif defined (CONFIG_MACH_HLTEKTT) || defined(CONFIG_MACH_FRESCOLTEKTT)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10340, 324, 225, 953, 8996, -7, -65, 508, 10743}
#elif defined (CONFIG_MACH_HLTELGT) || defined(CONFIG_MACH_FRESCOLTELGT)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10340, 324, 225, 953, 8996, -7, -65, 508, 10743}
#elif defined (CONFIG_MACH_FLTEEUR)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10067, 194, -598, 213, 10553, -73, 778, -440, 9370}
#elif defined (CONFIG_MACH_FLTESKT)
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10182, -330, -729, 134, 10606, -181, 942, -491, 9197}
#else
#define YAS_STATIC_ELLIPSOID_MATRIX \
{10000, 0, 0, 0, 10000, 0, 0, 0, 10000}
#endif
