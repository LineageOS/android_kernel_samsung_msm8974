/*
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef __LINUX_TAOS_H
#define __LINUX_TAOS_H

#include <linux/types.h>

#ifdef __KERNEL__
#define TAOS_OPT "taos-opt"
#define MIN 1

/* TMD3782 register offsets */
#define CNTRL				0x00
#define ALS_TIME			0X01
#define PRX_TIME			0x02
#define WAIT_TIME			0x03
#define ALS_MINTHRESHLO			0X04
#define ALS_MINTHRESHHI			0X05
#define ALS_MAXTHRESHLO			0X06
#define ALS_MAXTHRESHHI			0X07
#define PRX_MINTHRESHLO			0X08
#define PRX_MINTHRESHHI			0X09
#define PRX_MAXTHRESHLO			0X0A
#define PRX_MAXTHRESHHI			0X0B
#define INTERRUPT			0x0C
#define PRX_CFG				0x0D
#define PRX_COUNT			0x0E
#define GAIN				0x0F
#define REVID				0x11
#define CHIPID				0x12
#define STATUS				0x13
#define CLR_CHAN0LO			0x14
#define CLR_CHAN0HI			0x15
#define RED_CHAN1LO			0x16
#define RED_CHAN1HI			0x17
#define GRN_CHAN1LO			0x18
#define GRN_CHAN1HI			0x19
#define BLU_CHAN1LO			0x1A
#define BLU_CHAN1HI			0x1B
#define PRX_LO				0x1C
#define PRX_HI				0x1D
#define PRX_OFFSET			0x1E
#define TEST_STATUS			0x1F

/*TMD3782 cmd reg masks*/
#define CMD_REG					0X80
#define CMD_BYTE_RW				0x00
#define CMD_WORD_BLK_RW			0x20
#define CMD_SPL_FN				0x60
#define CMD_PROX_INTCLR			0X05
#define CMD_ALS_INTCLR			0X06
#define CMD_PROXALS_INTCLR		0X07
#define CMD_TST_REG				0X08
#define CMD_USER_REG			0X09

/* TMD3782 cntrl reg masks */
#define CNTL_REG_CLEAR			0x00
#define CNTL_PROX_INT_ENBL		0X20
#define CNTL_ALS_INT_ENBL		0X10
#define CNTL_WAIT_TMR_ENBL		0X08
#define CNTL_PROX_DET_ENBL		0X04
#define CNTL_ADC_ENBL			0x02
#define CNTL_PWRON				0x01
#define CNTL_ALSPON_ENBL		0x03
#define CNTL_INTALSPON_ENBL		0x13
#define CNTL_PROXPON_ENBL		0x0F
#define CNTL_INTPROXPON_ENBL		0x2F

/* TMD3782 status reg masks */
#define STA_ADCVALID			0x01
#define STA_PRXVALID			0x02
#define STA_ADC_PRX_VALID		0x03
#define STA_ADCINTR				0x10
#define STA_PRXINTR				0x20

#ifdef CONFIG_PROX_WINDOW_TYPE
#define WINTYPE_OTHERS	'0'
#define WINTYPE_WHITE		'2'
#define WHITEWINDOW_HI_THRESHOLD		720
#define WHITEWINDOW_LOW_THRESHOLD		590
#if defined(CONFIG_MACH_MELIUS_USC) || defined(CONFIG_MACH_MELIUS_SPR)
#define BLACKWINDOW_HI_THRESHOLD		750
#define BLACKWINDOW_LOW_THRESHOLD		520
#else
#define BLACKWINDOW_HI_THRESHOLD		650
#define BLACKWINDOW_LOW_THRESHOLD		520
#endif
#endif

struct taos_platform_data {
	int als_int;
	u32 als_int_flags;
#if defined(CONFIG_SEC_S_PROJECT)
	int enable;
#endif
	void (*power)(bool);
	int (*light_adc_value)(void);
	void (*led_on)(bool);
	int prox_thresh_hi;
	int prox_thresh_low;
	int prox_th_hi_cal;
	int prox_th_low_cal;
	int als_time;
	int intr_filter;
	int prox_pulsecnt;
	int als_gain;
	int coef_atime;
	int ga;
	int coef_a;
	int coef_b;
	int coef_c;
	int coef_d;
	int min_max;
	int prox_rawdata_trim;
	int crosstalk_max_offset;
	int thresholed_max_offset;
};
#endif /*__KERNEL__*/
#endif
