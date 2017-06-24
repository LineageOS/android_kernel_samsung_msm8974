/*
 * SM5414_charger.h
 * SiliconMitus SM5414 Charger Header
 *
 * Copyright (C) 2013 SiliconMitus
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SM5414_CHARGER_H
#define __SM5414_CHARGER_H __FILE__

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */
#define SEC_CHARGER_I2C_SLAVEADDR	(0x92>>1)

/* SM5414 Registers. */

#define SM5414_INT1		    0x00
#define SM5414_INT2		    0x01
#define SM5414_INT3		    0x02
#define SM5414_INTMASK1		0x03
#define SM5414_INTMASK2		0x04
#define SM5414_INTMASK3		0x05
#define SM5414_STATUS		0x06
#define SM5414_CTRL		    0x07
#define SM5414_VBUSCTRL		0x08
#define SM5414_CHGCTRL1		0x09
#define SM5414_CHGCTRL2		0x0A
#define SM5414_CHGCTRL3		0x0B
#define SM5414_CHGCTRL4		0x0C
#define SM5414_CHGCTRL5		0x0D

#define SM5414_INT1_THEMREG		0x01
#define SM5414_INT1_THEMSHDN	0x02
#define SM5414_INT1_BATOVP		0x04
#define SM5414_INT1_VBUSLIMIT	0x08
#define SM5414_INT1_AICL		0x10
#define SM5414_INT1_VBUSINOK	0x20
#define SM5414_INT1_VBUSUVLO	0x40
#define SM5414_INT1_VBUSOVP		0x80

#define SM5414_INT2_TOPOFF		0x01
#define SM5414_INT2_DONE		0x02
#define SM5414_INT2_CHGRSTF		0x04
#define SM5414_INT2_PRETMROFF	0x08
#define SM5414_INT2_OTGFAIL		0x10
#define SM5414_INT2_WEAKBAT		0x20
#define SM5414_INT2_NOBAT		0x40
#define SM5414_INT2_FASTTMROFF	0x80

#define SM5414_INT3_DISLIMIT	0x01
#define SM5414_INT3_VSYSOLP		0x02
#define SM5414_INT3_VSYSNG		0x04
#define SM5414_INT3_VSYSOK		0x08
#define SM5414_CHGCTRL1_AUTOSTOP        (1 << 3)

/* FAST Charge current */
#define FASTCHG_100mA       0
#define FASTCHG_150mA       1
#define FASTCHG_200mA       2
#define FASTCHG_250mA       3
#define FASTCHG_300mA       4
#define FASTCHG_350mA       5
#define FASTCHG_400mA       6
#define FASTCHG_450mA       7
#define FASTCHG_500mA       8
#define FASTCHG_550mA       9
#define FASTCHG_600mA      10
#define FASTCHG_650mA      11
#define FASTCHG_700mA      12
#define FASTCHG_750mA      13
#define FASTCHG_800mA      14
#define FASTCHG_850mA      15
#define FASTCHG_900mA      16
#define FASTCHG_950mA      17
#define FASTCHG_1000mA     18
#define FASTCHG_1050mA     19
#define FASTCHG_1100mA     20
#define FASTCHG_1150mA     21
#define FASTCHG_1200mA     22
#define FASTCHG_1250mA     23
#define FASTCHG_1300mA     24
#define FASTCHG_1350mA     25
#define FASTCHG_1400mA     26
#define FASTCHG_1450mA     27
#define FASTCHG_1500mA     28
#define FASTCHG_1550mA     29
#define FASTCHG_1600mA     30
#define FASTCHG_1650mA     31
#define FASTCHG_1700mA     32
#define FASTCHG_1750mA     33
#define FASTCHG_1800mA     34
#define FASTCHG_1850mA     35
#define FASTCHG_1900mA     36
#define FASTCHG_1950mA     37
#define FASTCHG_2000mA     38
#define FASTCHG_2050mA     39
#define FASTCHG_2100mA     40
#define FASTCHG_2150mA     41
#define FASTCHG_2200mA     42
#define FASTCHG_2250mA     43
#define FASTCHG_2300mA     44
#define FASTCHG_2350mA     45
#define FASTCHG_2400mA     46
#define FASTCHG_2450mA     47
#define FASTCHG_2500mA     48

/* Input current Limit */
#define VBUSLIMIT_100mA       0
#define VBUSLIMIT_150mA       1
#define VBUSLIMIT_200mA       2
#define VBUSLIMIT_250mA       3
#define VBUSLIMIT_300mA       4
#define VBUSLIMIT_350mA       5
#define VBUSLIMIT_400mA       6
#define VBUSLIMIT_450mA       7
#define VBUSLIMIT_500mA       8
#define VBUSLIMIT_550mA       9
#define VBUSLIMIT_600mA      10
#define VBUSLIMIT_650mA      11
#define VBUSLIMIT_700mA      12
#define VBUSLIMIT_750mA      13
#define VBUSLIMIT_800mA      14
#define VBUSLIMIT_850mA      15
#define VBUSLIMIT_900mA      16
#define VBUSLIMIT_950mA      17
#define VBUSLIMIT_1000mA     18
#define VBUSLIMIT_1050mA     19
#define VBUSLIMIT_1100mA     20
#define VBUSLIMIT_1150mA     21
#define VBUSLIMIT_1200mA     22
#define VBUSLIMIT_1250mA     23
#define VBUSLIMIT_1300mA     24
#define VBUSLIMIT_1350mA     25
#define VBUSLIMIT_1400mA     26
#define VBUSLIMIT_1450mA     27
#define VBUSLIMIT_1500mA     28
#define VBUSLIMIT_1550mA     29
#define VBUSLIMIT_1600mA     30
#define VBUSLIMIT_1650mA     31
#define VBUSLIMIT_1700mA     32
#define VBUSLIMIT_1750mA     33
#define VBUSLIMIT_1800mA     34
#define VBUSLIMIT_1850mA     35
#define VBUSLIMIT_1900mA     36
#define VBUSLIMIT_1950mA     37
#define VBUSLIMIT_2000mA     38
#define VBUSLIMIT_2050mA     39
#define VBUSLIMIT_2100mA     40
#define VBUSLIMIT_2150mA     41
#define VBUSLIMIT_2200mA     42
#define VBUSLIMIT_2250mA     43
#define VBUSLIMIT_2300mA     44
#define VBUSLIMIT_2350mA     45
#define VBUSLIMIT_2400mA     46
#define VBUSLIMIT_2450mA     47
#define VBUSLIMIT_2500mA     48

/* AICL TH */
#define AICL_THRESHOLD_4_3_V         0
#define AICL_THRESHOLD_4_4_V         1
#define AICL_THRESHOLD_4_5_V         2
#define AICL_THRESHOLD_4_6_V         3
#define AICL_THRESHOLD_4_7_V         4
#define AICL_THRESHOLD_4_8_V         5
#define AICL_THRESHOLD_4_9_V         6
#define AICL_THRESHOLD_MASK       0x0F

/* AUTOSTOP */
#define AUTOSTOP_EN     (1<<3)

/* AICLEN */
#define AICL_EN         (1<<2)

/* PRECHG */
#define PRECHG_150mA         0
#define PRECHG_250mA         1
#define PRECHG_350mA         2
#define PRECHG_450mA         3
#define PRECHG_MASK       0xFC

/* Battery Regulation Voltage */
#define BATREG_4_1_0_0_V     0
#define BATREG_4_1_2_5_V     1
#define BATREG_4_1_5_0_V     2
#define BATREG_4_1_7_5_V     3
#define BATREG_4_2_0_0_V     4
#define BATREG_4_2_2_5_V     5
#define BATREG_4_2_5_0_V     6
#define BATREG_4_2_7_5_V     7
#define BATREG_4_3_0_0_V     8
#define BATREG_4_3_2_5_V     9
#define BATREG_4_3_5_0_V    10
#define BATREG_4_3_7_5_V    11
#define BATREG_4_4_0_0_V    12
#define BATREG_4_4_2_5_V    13
#define BATREG_4_4_5_0_V    14
#define BATREG_4_4_7_5_V    15
#define BATREG_MASK       0x0F

/* Weak Battery Voltage */
#define WEAKBAT_3_0_0_V     0
#define WEAKBAT_3_0_5_V     1
#define WEAKBAT_3_1_0_V     2
#define WEAKBAT_3_1_5_V     3
#define WEAKBAT_3_2_0_V     4
#define WEAKBAT_3_2_5_V     5
#define WEAKBAT_3_3_0_V     6
#define WEAKBAT_3_3_5_V     7
#define WEAKBAT_3_4_0_V     8
#define WEAKBAT_3_4_5_V     9
#define WEAKBAT_3_5_0_V    10
#define WEAKBAT_3_5_5_V    11
#define WEAKBAT_3_6_0_V    12
#define WEAKBAT_3_6_5_V    13
#define WEAKBAT_3_7_0_V    14
#define WEAKBAT_3_7_5_V    15
#define WEAKBAT_MASK     0xF0

/* top-off charge current */
#define TOPOFF_100mA       0
#define TOPOFF_150mA       1
#define TOPOFF_200mA       2
#define TOPOFF_250mA       3
#define TOPOFF_300mA       4
#define TOPOFF_350mA       5
#define TOPOFF_400mA       6
#define TOPOFF_450mA       7
#define TOPOFF_500mA       8
#define TOPOFF_550mA       9
#define TOPOFF_600mA      10
#define TOPOFF_650mA      11
#define TOPOFF_MASK     0x07

/* discharge current */
#define DISCHARGELIMIT_DISABLED    0
#define DISCHARGELIMIT_2_0_A       1
#define DISCHARGELIMIT_2_5_A       2
#define DISCHARGELIMIT_3_0_A       3
#define DISCHARGELIMIT_3_5_A       4
#define DISCHARGELIMIT_4_0_A       5
#define DISCHARGELIMIT_4_5_A       6
#define DISCHARGELIMIT_5_0_A       7
#define DISCHARGELIMIT_MASK     0xF8

/* OTG voltage */
#define VOTG_5_0_V      0
#define VOTG_5_1_V      1
#define VOTG_5_2_V      2
#define VOTG_MASK    0x0F

/* Fast timer */
#define FASTTIMER_3_5_HOUR      0
#define FASTTIMER_4_5_HOUR      1
#define FASTTIMER_5_5_HOUR      2
#define FASTTIMER_DISABLED      3
#define FASTTIMER_MASK       0xF3

/* Topoff timer */
#define TOPOFFTIMER_10MIN       0
#define TOPOFFTIMER_20MIN       1
#define TOPOFFTIMER_30MIN       2
#define TOPOFFTIMER_45MIN       3
#define TOPOFFTIMER_MASK     0xFC

/* Enable charger */
#define CHARGE_EN 0x02

void SM5414_i2c_int_read(struct i2c_client *client);

#define BATREG_MASK        0x0F

struct sec_chg_info {
	bool dummy;
	u8 int_1_s;
	u8 int_2_s;
	u8 int_3_s;
	u8 int_1_h;
	u8 int_2_h;
	u8 int_3_h;
};

ssize_t SM5414_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t SM5414_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define SM5414_CHARGER_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = SM5414_chg_show_attrs,			\
	.store = SM5414_chg_store_attrs,			\
}

#endif /* __SM5414_CHARGER_H */
