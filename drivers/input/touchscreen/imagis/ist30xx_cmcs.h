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

#ifndef __IST30XX_CMCS_H__
#define __IST30XX_CMCS_H__


#define IST30XX_INTERNAL_CMCS_BIN   (1)

#define CMCS_FLAG_CM                (1)
#define CMCS_FLAG_CM_SPEC           (1 << 1)
#define CMCS_FLAG_CM_SLOPE0         (1 << 2)
#define CMCS_FLAG_CM_SLOPE1         (1 << 3)
#define CMCS_FLAG_CS0               (1 << 4)
#define CMCS_FLAG_CS1               (1 << 5)

#define FLAG_ENABLE_CM              (1)
#define FLAG_ENABLE_CS              (2)
#define FLAG_ENABLE_CR              (4)

#define IST30XX_CMCS_LOAD_END       (0x8FFFFCAB)
#if (IMAGIS_TSP_IC == IMAGIS_IST30XXB)
#define IST30XX_CMCS_BUF_SIZE       (16 * 16)
#elif (IMAGIS_TSP_IC == IMAGIS_IST3038)
#define IST30XX_CMCS_BUF_SIZE       (((19 * 19) / 2 + 1) * 2)
#else
#error "Unknown TSP_IC"
#endif

#define ENABLE_CM_MODE(k)           (k & 1)
#define ENABLE_CS_MODE(k)           ((k >> 1) & 1)

#define IST30XXB_CMCS_NAME          "ist30xxb.cms"

#define IST30XX_CMCS_MAGIC          "CMCS1TAG"
struct CMCS_ADDR_INFO {
	u32	base_screen;
	u32	base_key;
	u32	start_cp;
	u32	vcmp;
	u32	sensor1;
	u32	sensor2;
	u32	sensor3;
};
struct CMCS_CH_INFO {
	u8	tx_num;
	u8	rx_num;
	u8	key_rx;
	u8	key1;
	u8	key2;
	u8	key3;
	u8	key4;
	u8	key5;
};
struct CMCS_SLOPE_INFO {
	s16	x_min;
	s16	x_max;
	s16	y_min;
	s16	y_max;
};
struct CMCS_SPEC_INFO {
	u16	screen_min;
	u16	screen_max;
	u16	key_min;
	u16	key_max;
};
struct CMCS_CMD {
	u16	mode;   // enable bit
	u16	cmcs_size;
	u16	base_screen;
	u16	base_key;
	u8	start_cp_cm;
	u8	start_cp_cs;
	u8	vcmp_cm;
	u8	vcmp_cs;
	u32	reserved; // for checksum of firmware
};
typedef struct _CMCS_INFO {
	u32			timeout;
	struct CMCS_ADDR_INFO	addr;
	struct CMCS_CH_INFO	ch;
	struct CMCS_SPEC_INFO	spec_cr;
	struct CMCS_SPEC_INFO	spec_cm;
	struct CMCS_SPEC_INFO	spec_cs;
	struct CMCS_SLOPE_INFO	slope;
	u16			sensor1_size;
	u16			sensor2_size;
	u16			sensor3_size;
	u16			reserved;
	u32			cmcs_chksum;
	u32			sensor_chksum;
	struct CMCS_CMD		cmd;
} CMCS_INFO;

typedef struct _CMCS_BIN_INFO {
	char		magic1[8];
	CMCS_INFO	cmcs;
	u8 *		buf_cmcs;
	u32 *		buf_sensor;
	u16 *		buf_node;
	char		magic2[8];
} CMCS_BIN_INFO;

typedef struct _CMCS_BUF {
	s16	cm[IST30XX_CMCS_BUF_SIZE];
	s16	spec[IST30XX_CMCS_BUF_SIZE];
	s16	slope0[IST30XX_CMCS_BUF_SIZE];
	s16	slope1[IST30XX_CMCS_BUF_SIZE];
	s16	cs0[IST30XX_CMCS_BUF_SIZE];
	s16	cs1[IST30XX_CMCS_BUF_SIZE];
} CMCS_BUF;


int ist30xx_init_cmcs_sysfs(void);

#endif  // __IST30XX_CMCS_H__
