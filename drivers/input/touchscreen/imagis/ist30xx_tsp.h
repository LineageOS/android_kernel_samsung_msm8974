/*
 *  Copyright (C) 2010, Imagis Technology Co. Ltd. All Rights Reserved.
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

#ifndef __IST30XX_TSP_H__
#include "ist30xx.h"

#define __IST30XX_TSP_H__

#define IST30XX_DD_VERSION  (3)

#define IST30XX_MULTIPLE_TSP    (0)

#define FLAG_NODE_Y         (0)
#define FLAG_NODE_X         (1)

#if defined(CONFIG_SEC_KANAS_PROJECT)
#define NODE_TX_NUM         (21)
#define NODE_RX_NUM         (14)
#define NODE_TOTAL_NUM      (TSP_TX_NUM * TSP_RX_NUM)

#define TSP_TX_NUM          (21)
#define TSP_RX_NUM          (13)
#define TSP_TOTAL_NUM       (TSP_TX_NUM * TSP_RX_NUM)
#else
#define NODE_TX_NUM         (18)
#define NODE_RX_NUM         (13)
#define NODE_TOTAL_NUM      (TSP_TX_NUM * TSP_RX_NUM)

#define TSP_TX_NUM          (18)
#define TSP_RX_NUM          (12)
#define TSP_TOTAL_NUM       (TSP_TX_NUM * TSP_RX_NUM)
#endif

#define TSP_THRESHOLD       (30)
#define TKEY_THRESHOLD      (700)

#define IST30XXB_PARSE_TSPTYPE(k)   ((k >> 1) & 0xF)

#define TSP_TYPE_UNKNOWN    (0xF0)
#define TSP_TYPE_ALPS       (0xF)
#define TSP_TYPE_EELY       (0xE)
#define TSP_TYPE_TOP        (0xD)
#define TSP_TYPE_MELFAS     (0xC)
#define TSP_TYPE_ILJIN      (0xB)
#define TSP_TYPE_SYNOPEX    (0xA)
#define TSP_TYPE_SMAC       (0x9)
#define TSP_TYPE_OTHERS     (0x8)

#endif  // __IST30XX_TSP_H__
