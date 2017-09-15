/*
 * Copyright (C) 2012 Invensense, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include "inv_mpu_iio.h"
#include "dmpKey.h"
#include "dmpmap.h"

#define CFG_OUT_PRESS           (2214)
#define CFG_FB_GAIN_GYRO_ON     (1674)
#define CFG_OUT_GYRO            (2146)
#define CFG_PEDSTEP_DET         (3363)
#define OUT_GYRO_DAT            (2155)
#define CFG_FIFO_INT            (2325)
#define OUT_CPASS_DAT           (2189)
#define CFG_AUTH                (1355)
#define CFG_QSHOT_FINISH_INT    (1918)
#define OUT_ACCL_DAT            (2121)
#define FCFG_1                  (1383)
#define FCFG_3                  (1413)
#define FCFG_2                  (1387)
#define CFG_OUT_CPASS           (2180)
#define FCFG_7                  (1394)
#define CFG_OUT_3QUAT           (2008)
#define OUT_PRESS_DAT           (2223)
#define OUT_3QUAT_DAT           (2018)
#define CFG_QSHOT_START_INT     (1895)
#define CFG_7                   (1610)
#define OUT_PQUAT_DAT           (2087)
#define CFG_OUT_6QUAT           (2043)
#define CFG_PED_INT             (3352)
#define SMD_TP2                 (1575)
#define SMD_TP1                 (1554)
#define CFG_S_HEALTH_INT        (2614)
#define CFG_PED_ENABLE          (2327)
#define CFG_MOTION_BIAS         (1612)
#define CFG_OUT_ACCL            (2112)
#define CFG_OUT_STEPDET         (1978)
#define OUT_6QUAT_DAT           (2053)
#define CFG_OUT_PQUAT           (2078)

#define D_0_22                  (22+512)
#define D_0_24                  (24+512)

#define D_0_36                  (36)
#define D_0_52                  (52)
#define D_0_96                  (96)
#define D_0_104                 (104)
#define D_0_108                 (108)
#define D_0_163                 (163)
#define D_0_188                 (188)
#define D_0_192                 (192)
#define D_0_224                 (224)
#define D_0_228                 (228)
#define D_0_232                 (232)
#define D_0_236                 (236)

#define D_1_2                   (256 + 2)
#define D_1_4                   (256 + 4)
#define D_1_8                   (256 + 8)
#define D_1_10                  (256 + 10)
#define D_1_24                  (256 + 24)
#define D_1_28                  (256 + 28)
#define D_1_36                  (256 + 36)
#define D_1_40                  (256 + 40)
#define D_1_44                  (256 + 44)
#define D_1_72                  (256 + 72)
#define D_1_74                  (256 + 74)
#define D_1_79                  (256 + 79)
#define D_1_88                  (256 + 88)
#define D_1_90                  (256 + 90)
#define D_1_92                  (256 + 92)
#define D_1_96                  (256 + 96)
#define D_1_98                  (256 + 98)
#define D_1_106                 (256 + 106)
#define D_1_108                 (256 + 108)
#define D_1_112                 (256 + 112)
#define D_1_128                 (256 + 144)
#define D_1_152                 (256 + 12)
#define D_1_160                 (256 + 160)
#define D_1_176                 (256 + 176)
#define D_1_178                 (256 + 178)
#define D_1_218                 (256 + 218)
#define D_1_232                 (256 + 232)
#define D_1_236                 (256 + 236)
#define D_1_240                 (256 + 240)
#define D_1_244                 (256 + 244)
#define D_1_250                 (256 + 250)
#define D_1_252                 (256 + 252)
#define D_2_12                  (512 + 12)
#define D_2_96                  (512 + 96)
#define D_2_108                 (512 + 108)
#define D_2_208                 (512 + 208)
#define D_2_224                 (512 + 224)
#define D_2_236                 (512 + 236)
#define D_2_244                 (512 + 244)
#define D_2_248                 (512 + 248)
#define D_2_252                 (512 + 252)
#define CPASS_BIAS_X            (30 * 16 + 4)
#define CPASS_BIAS_Y            (30 * 16 + 8)
#define CPASS_BIAS_Z            (30 * 16 + 12)
#define CPASS_MTX_00            (32 * 16 + 4)
#define CPASS_MTX_01            (32 * 16 + 8)
#define CPASS_MTX_02            (36 * 16 + 12)
#define CPASS_MTX_10            (33 * 16)
#define CPASS_MTX_11            (33 * 16 + 4)
#define CPASS_MTX_12            (33 * 16 + 8)
#define CPASS_MTX_20            (33 * 16 + 12)
#define CPASS_MTX_21            (34 * 16 + 4)
#define CPASS_MTX_22            (34 * 16 + 8)
#define D_EXT_GYRO_BIAS_X       (61 * 16)
#define D_EXT_GYRO_BIAS_Y       (61 * 16) + 4
#define D_EXT_GYRO_BIAS_Z       (61 * 16) + 8
#define D_ACT0                  (40 * 16)
#define D_ACSX                  (40 * 16 + 4)
#define D_ACSY                  (40 * 16 + 8)
#define D_ACSZ                  (40 * 16 + 12)

#define FLICK_MSG               (45 * 16 + 4)
#define FLICK_COUNTER           (45 * 16 + 8)
#define FLICK_LOWER             (45 * 16 + 12)
#define FLICK_UPPER             (46 * 16 + 12)

#define D_SMD_ENABLE            (18 * 16)
#define D_SMD_MOT_THLD          (21 * 16 + 8)
#define D_SMD_DELAY_THLD        (21 * 16 + 4)
#define D_SMD_DELAY2_THLD       (21 * 16 + 12)
#define D_SMD_EXE_STATE         (22 * 16)
#define D_SMD_DELAY_CNTR        (21 * 16)

#define D_WF_GESTURE_TIME_THRSH	(25 * 16 + 8)
#define D_WF_GESTURE_TILT_ERROR	(25 * 16 + 12)
#define D_WF_GESTURE_TILT_THRSH	(26 * 16 + 8)
#define D_WF_GESTURE_TILT_REJECT_THRSH	(26 * 16 + 12)

#define D_LCD_UP_DOWN_ENABLE	(29 * 16 + 2)
#define D_LCD_TIME_THRESH		(29 * 16 + 8)
#define D_LCD_UP_X_THRESH		(31 * 16 + 4)
#define D_LCD_UP_Y_THRESH		(31 * 16 + 8)
#define D_LCD_UP_Z_THRESH		(31 * 16 + 12)
#define D_LCD_DOWN_X_THRESH		(30 * 16 + 4)
#define D_LCD_DOWN_Y_THRESH		(30 * 16 + 8)
#define D_LCD_DOWN_Z_THRESH		(30 * 16 + 12)
#define D_LCD_STATUS			(29 * 16)

#define D_QSHOT_START_ANGLE		(13 * 16 + 8)
#define D_QSHOT_FINISH_ANGLE	(13 * 16 + 12)

#define D_AUTH_OUT              (992)
#define D_AUTH_IN               (996)
#define D_AUTH_A                (1000)
#define D_AUTH_B                (1004)

#define D_PEDSTD_BP_B           (768 + 0x1C)
#define D_PEDSTD_BP_A4          (768 + 0x40)
#define D_PEDSTD_BP_A3          (768 + 0x44)
#define D_PEDSTD_BP_A2          (768 + 0x48)
#define D_PEDSTD_BP_A1          (768 + 0x4C)
#define D_PEDSTD_SB             (768 + 0x28)
#define D_PEDSTD_SB_TIME        (768 + 0x2C)
#define D_PEDSTD_PEAKTHRSH      (768 + 0x98)
#define D_PEDSTD_TIML           (768 + 0x2A)
#define D_PEDSTD_TIMH           (768 + 0x2E)
#define D_PEDSTD_PEAK           (768 + 0X94)
#define D_PEDSTD_STEPCTR        (768 + 0x60)
#define D_PEDSTD_STEPCTR2       (58 * 16 + 8)
#define D_PEDSTD_TIMECTR        (964)
#define D_PEDSTD_DECI           (768 + 0xA0)
#define D_PEDSTD_SB2			(60 * 16 + 14)
#define D_STPDET_TIMESTAMP      (28 * 16 + 8)
#define D_PEDSTD_DRIVE_STATE    (58)

/* S-Health keys */
#define D_S_HEALTH_WALK_RUN_1   (16*69 + 12)
#define D_S_HEALTH_WALK_RUN_2   (16*69 +  8)
#define D_S_HEALTH_WALK_RUN_3   (16*69 +  4)
#define D_S_HEALTH_WALK_RUN_4   (16*69 +  0)
#define D_S_HEALTH_WALK_RUN_5   (16*68 + 12)
#define D_S_HEALTH_WALK_RUN_6   (16*68 +  8)
#define D_S_HEALTH_WALK_RUN_7   (16*68 +  4)
#define D_S_HEALTH_WALK_RUN_8   (16*68 +  0)
#define D_S_HEALTH_WALK_RUN_9   (16*67 + 12)
#define D_S_HEALTH_WALK_RUN_10  (16*67 +  8)
#define D_S_HEALTH_WALK_RUN_11  (16*67 +  4)
#define D_S_HEALTH_WALK_RUN_12  (16*67 +  0)
#define D_S_HEALTH_WALK_RUN_13  (16*66 + 12)
#define D_S_HEALTH_WALK_RUN_14  (16*66 +  8)
#define D_S_HEALTH_WALK_RUN_15  (16*66 +  4)
#define D_S_HEALTH_WALK_RUN_16  (16*66 +  0)
#define D_S_HEALTH_WALK_RUN_17  (16*65 + 12)
#define D_S_HEALTH_WALK_RUN_18  (16*65 +  8)
#define D_S_HEALTH_WALK_RUN_19  (16*65 +  4)
#define D_S_HEALTH_WALK_RUN_20  (16*65 +  0)
#define D_S_HEALTH_WALK_RUN_21   (16*70 +  8)
#define D_S_HEALTH_CADENCE1     (16*69 + 14)
#define D_S_HEALTH_CADENCE2     (16*69 + 10)
#define D_S_HEALTH_CADENCE3     (16*69 +  6)
#define D_S_HEALTH_CADENCE4     (16*69 +  2)
#define D_S_HEALTH_CADENCE5     (16*68 + 14)
#define D_S_HEALTH_CADENCE6     (16*68 + 10)
#define D_S_HEALTH_CADENCE7     (16*68 +  6)
#define D_S_HEALTH_CADENCE8     (16*68 +  2)
#define D_S_HEALTH_CADENCE9     (16*67 + 14)
#define D_S_HEALTH_CADENCE10    (16*67 + 10)
#define D_S_HEALTH_CADENCE11    (16*67 +  6)
#define D_S_HEALTH_CADENCE12    (16*67 +  2)
#define D_S_HEALTH_CADENCE13    (16*66 + 14)
#define D_S_HEALTH_CADENCE14    (16*66 + 10)
#define D_S_HEALTH_CADENCE15    (16*66 +  6)
#define D_S_HEALTH_CADENCE16    (16*66 +  2)
#define D_S_HEALTH_CADENCE17    (16*65 + 14)
#define D_S_HEALTH_CADENCE18    (16*65 + 10)
#define D_S_HEALTH_CADENCE19    (16*65 +  6)
#define D_S_HEALTH_CADENCE20    (16*65 +  2)
#define D_S_HEALTH_CADENCE21     (16*70 + 10)

#define D_S_HEALTH_INT_PERIOD   (16*70 +  2)
#define D_S_HEALTH_INT_PERIOD2  (16*70 +  6)
#define D_S_HEALTH_FREQ_TH      (16*25 +  0)
#define D_S_HEALTH_ALPHA1       (16*35 +  4)
#define D_S_HEALTH_ALPHA2       (16*35 +  8)

#define D_S_HEALTH_MIN_CNTR	(16*64 + 2)
#define D_S_HEALTH_MIN_CONST	(16*64 + 6)
#define D_S_HEALTH_STEP_CNT	(16*64 + 8)
#define D_S_HEALTH_STEP_CNT_P	(16*64 + 12)


#define D_HOST_NO_MOT           (976)
#define D_ACCEL_BIAS            (660)

#define D_BM_BATCH_CNTR         (27 * 16 + 4)
#define D_BM_BATCH_THLD         (27 * 16 + 12)
#define D_BM_ENABLE             (28 * 16 + 6)
#define D_BM_NUMWORD_TOFILL     (28 * 16 + 4)

#define D_SO_DATA               (4 * 16 + 2)

#define D_P_HW_ID               (22 * 16 + 10)
#define D_P_INIT                (22 * 16 + 2)

#define D_DMP_RUN_CNTR          (24*16)

#define D_ODR_S0                (45*16+8)
#define D_ODR_S1                (45*16+12)
#define D_ODR_S2                (45*16+10)
#define D_ODR_S3                (45*16+14)
#define D_ODR_S4                (46*16+8)
#define D_ODR_S5                (46*16+12)
#define D_ODR_S6                (46*16+10)
#define D_ODR_S7                (46*16+14)
#define D_ODR_S8                (42*16+8)
#define D_ODR_S9                (42*16+12)

#define D_ODR_CNTR_S0           (45*16)
#define D_ODR_CNTR_S1           (45*16+4)
#define D_ODR_CNTR_S2           (45*16+2)
#define D_ODR_CNTR_S3           (45*16+6)
#define D_ODR_CNTR_S4           (46*16)
#define D_ODR_CNTR_S5           (46*16+4)
#define D_ODR_CNTR_S6           (46*16+2)
#define D_ODR_CNTR_S7           (46*16+6)
#define D_ODR_CNTR_S8           (42*16)
#define D_ODR_CNTR_S9           (42*16+4)

#define D_FS_LPQ0               (59*16)
#define D_FS_LPQ1               (59*16 + 4)
#define D_FS_LPQ2               (59*16 + 8)
#define D_FS_LPQ3               (59*16 + 12)

#define D_FS_Q0                 (12*16)
#define D_FS_Q1                 (12*16 + 4)
#define D_FS_Q2                 (12*16 + 8)
#define D_FS_Q3                 (12*16 + 12)

#define D_FS_9Q0                (2*16)
#define D_FS_9Q1                (2*16 + 4)
#define D_FS_9Q2                (2*16 + 8)
#define D_FS_9Q3                (2*16 + 12)

#define D_CPASS_STATUS_CHK		(22*16 + 8)

static const struct tKeyLabel dmpTConfig[] = {
	{KEY_CFG_OUT_ACCL,              CFG_OUT_ACCL},
	{KEY_CFG_OUT_GYRO,              CFG_OUT_GYRO},
	{KEY_CFG_OUT_3QUAT,             CFG_OUT_3QUAT},
	{KEY_CFG_OUT_6QUAT,             CFG_OUT_6QUAT},
	{KEY_CFG_OUT_PQUAT,             CFG_OUT_PQUAT},
	{KEY_CFG_PED_ENABLE,            CFG_PED_ENABLE},
	{KEY_CFG_FIFO_INT,              CFG_FIFO_INT},
	{KEY_CFG_AUTH,                  CFG_AUTH},
	{KEY_FCFG_1,                    FCFG_1},
	{KEY_FCFG_3,                    FCFG_3},
	{KEY_FCFG_2,                    FCFG_2},
	{KEY_FCFG_7,                    FCFG_7},
	{KEY_CFG_7,                     CFG_7},
	{KEY_CFG_MOTION_BIAS,           CFG_MOTION_BIAS},
	{KEY_CFG_PEDSTEP_DET,           CFG_PEDSTEP_DET},
	{KEY_D_0_22,                    D_0_22},
	{KEY_D_0_104,                   D_0_104},
	{KEY_D_1_74,                    D_1_74},
	{KEY_D_1_232,                   D_1_232},
	{KEY_D_1_250,                   D_1_250},
	{KEY_D_AUTH_OUT,                D_AUTH_OUT},
	{KEY_D_AUTH_IN,                 D_AUTH_IN},
	{KEY_D_AUTH_A,                  D_AUTH_A},
	{KEY_D_AUTH_B,                  D_AUTH_B},
	{KEY_CPASS_BIAS_X,          CPASS_BIAS_X},
	{KEY_CPASS_BIAS_Y,          CPASS_BIAS_Y},
	{KEY_CPASS_BIAS_Z,          CPASS_BIAS_Z},
	{KEY_CPASS_MTX_00,          CPASS_MTX_00},
	{KEY_CPASS_MTX_01,          CPASS_MTX_01},
	{KEY_CPASS_MTX_02,          CPASS_MTX_02},
	{KEY_CPASS_MTX_10,          CPASS_MTX_10},
	{KEY_CPASS_MTX_11,          CPASS_MTX_11},
	{KEY_CPASS_MTX_12,          CPASS_MTX_12},
	{KEY_CPASS_MTX_20,          CPASS_MTX_20},
	{KEY_CPASS_MTX_21,          CPASS_MTX_21},
	{KEY_CPASS_MTX_22,          CPASS_MTX_22},
	{KEY_D_PEDSTD_BP_B,             D_PEDSTD_BP_B},
	{KEY_D_PEDSTD_BP_A4,            D_PEDSTD_BP_A4},
	{KEY_D_PEDSTD_BP_A3,            D_PEDSTD_BP_A3},
	{KEY_D_PEDSTD_BP_A2,            D_PEDSTD_BP_A2},
	{KEY_D_PEDSTD_BP_A1,            D_PEDSTD_BP_A1},
	{KEY_D_PEDSTD_SB,               D_PEDSTD_SB},
	{KEY_D_PEDSTD_SB_TIME,          D_PEDSTD_SB_TIME},
	{KEY_D_PEDSTD_PEAKTHRSH,        D_PEDSTD_PEAKTHRSH},
	{KEY_D_PEDSTD_TIML,             D_PEDSTD_TIML},
	{KEY_D_PEDSTD_TIMH,             D_PEDSTD_TIMH},
	{KEY_D_PEDSTD_PEAK,             D_PEDSTD_PEAK},
	{KEY_D_PEDSTD_STEPCTR,          D_PEDSTD_STEPCTR},
	{KEY_D_PEDSTD_STEPCTR2,         D_PEDSTD_STEPCTR2},
	{KEY_D_PEDSTD_TIMECTR,          D_PEDSTD_TIMECTR},
	{KEY_D_PEDSTD_DECI,             D_PEDSTD_DECI},
	{KEY_D_PEDSTD_SB2,				D_PEDSTD_SB2},
	{KEY_D_PEDSTD_DRIVE_STATE,      D_PEDSTD_DRIVE_STATE},
	{KEY_D_STPDET_TIMESTAMP,		D_STPDET_TIMESTAMP},
	{KEY_D_S_HEALTH_WALK_RUN_1,     D_S_HEALTH_WALK_RUN_1},
	{KEY_D_S_HEALTH_WALK_RUN_2,     D_S_HEALTH_WALK_RUN_2},
	{KEY_D_S_HEALTH_WALK_RUN_3,     D_S_HEALTH_WALK_RUN_3},
	{KEY_D_S_HEALTH_WALK_RUN_4,     D_S_HEALTH_WALK_RUN_4},
	{KEY_D_S_HEALTH_WALK_RUN_5,     D_S_HEALTH_WALK_RUN_5},
	{KEY_D_S_HEALTH_WALK_RUN_6,     D_S_HEALTH_WALK_RUN_6},
	{KEY_D_S_HEALTH_WALK_RUN_7,     D_S_HEALTH_WALK_RUN_7},
	{KEY_D_S_HEALTH_WALK_RUN_8,     D_S_HEALTH_WALK_RUN_8},
	{KEY_D_S_HEALTH_WALK_RUN_9,     D_S_HEALTH_WALK_RUN_9},
	{KEY_D_S_HEALTH_WALK_RUN_10,    D_S_HEALTH_WALK_RUN_10},
	{KEY_D_S_HEALTH_WALK_RUN_11,    D_S_HEALTH_WALK_RUN_11},
	{KEY_D_S_HEALTH_WALK_RUN_12,    D_S_HEALTH_WALK_RUN_12},
	{KEY_D_S_HEALTH_WALK_RUN_13,    D_S_HEALTH_WALK_RUN_13},
	{KEY_D_S_HEALTH_WALK_RUN_14,    D_S_HEALTH_WALK_RUN_14},
	{KEY_D_S_HEALTH_WALK_RUN_15,    D_S_HEALTH_WALK_RUN_15},
	{KEY_D_S_HEALTH_WALK_RUN_16,    D_S_HEALTH_WALK_RUN_16},
	{KEY_D_S_HEALTH_WALK_RUN_17,    D_S_HEALTH_WALK_RUN_17},
	{KEY_D_S_HEALTH_WALK_RUN_18,    D_S_HEALTH_WALK_RUN_18},
	{KEY_D_S_HEALTH_WALK_RUN_19,    D_S_HEALTH_WALK_RUN_19},
	{KEY_D_S_HEALTH_WALK_RUN_20,    D_S_HEALTH_WALK_RUN_20},
	{KEY_D_S_HEALTH_WALK_RUN_21,    D_S_HEALTH_WALK_RUN_21},
	{KEY_D_S_HEALTH_CADENCE1,       D_S_HEALTH_CADENCE1},
	{KEY_D_S_HEALTH_CADENCE2,       D_S_HEALTH_CADENCE2},
	{KEY_D_S_HEALTH_CADENCE3,       D_S_HEALTH_CADENCE3},
	{KEY_D_S_HEALTH_CADENCE4,       D_S_HEALTH_CADENCE4},
	{KEY_D_S_HEALTH_CADENCE5,       D_S_HEALTH_CADENCE5},
	{KEY_D_S_HEALTH_CADENCE6,       D_S_HEALTH_CADENCE6},
	{KEY_D_S_HEALTH_CADENCE7,       D_S_HEALTH_CADENCE7},
	{KEY_D_S_HEALTH_CADENCE8,       D_S_HEALTH_CADENCE8},
	{KEY_D_S_HEALTH_CADENCE9,       D_S_HEALTH_CADENCE9},
	{KEY_D_S_HEALTH_CADENCE10,      D_S_HEALTH_CADENCE10},
	{KEY_D_S_HEALTH_CADENCE11,      D_S_HEALTH_CADENCE11},
	{KEY_D_S_HEALTH_CADENCE12,      D_S_HEALTH_CADENCE12},
	{KEY_D_S_HEALTH_CADENCE13,      D_S_HEALTH_CADENCE13},
	{KEY_D_S_HEALTH_CADENCE14,      D_S_HEALTH_CADENCE14},
	{KEY_D_S_HEALTH_CADENCE15,      D_S_HEALTH_CADENCE15},
	{KEY_D_S_HEALTH_CADENCE16,      D_S_HEALTH_CADENCE16},
	{KEY_D_S_HEALTH_CADENCE17,      D_S_HEALTH_CADENCE17},
	{KEY_D_S_HEALTH_CADENCE18,      D_S_HEALTH_CADENCE18},
	{KEY_D_S_HEALTH_CADENCE19,      D_S_HEALTH_CADENCE19},
	{KEY_D_S_HEALTH_CADENCE20,      D_S_HEALTH_CADENCE20},
	{KEY_D_S_HEALTH_CADENCE21,      D_S_HEALTH_CADENCE21},
	{KEY_S_HEALTH_INT_PERIOD,       D_S_HEALTH_INT_PERIOD},
	{KEY_S_HEALTH_INT_PERIOD2,      D_S_HEALTH_INT_PERIOD2},
	{KEY_S_HEALTH_MIN_CNTR,         D_S_HEALTH_MIN_CNTR},
	{KEY_S_HEALTH_MIN_CONST,        D_S_HEALTH_MIN_CONST},
	{KEY_S_HEALTH_STEP_CNT,         D_S_HEALTH_STEP_CNT},
	{KEY_S_HEALTH_STEP_CNT_P,       D_S_HEALTH_STEP_CNT_P},
	{KEY_S_HEALTH_FREQ_TH,          D_S_HEALTH_FREQ_TH},
	{KEY_S_HEALTH_ALPHA1,           D_S_HEALTH_ALPHA1},
	{KEY_S_HEALTH_ALPHA2,           D_S_HEALTH_ALPHA2},
	{KEY_D_HOST_NO_MOT,             D_HOST_NO_MOT},
	{KEY_D_ACCEL_BIAS,              D_ACCEL_BIAS},
	{KEY_CFG_EXT_GYRO_BIAS_X,       D_EXT_GYRO_BIAS_X},
	{KEY_CFG_EXT_GYRO_BIAS_Y,       D_EXT_GYRO_BIAS_Y},
	{KEY_CFG_EXT_GYRO_BIAS_Z,       D_EXT_GYRO_BIAS_Z},
	{KEY_CFG_PED_INT,               CFG_PED_INT},
	{KEY_CFG_S_HEALTH_INT,          CFG_S_HEALTH_INT},
	{KEY_SMD_ENABLE,                D_SMD_ENABLE},
	{KEY_SMD_ACCEL_THLD,            D_SMD_MOT_THLD},
	{KEY_SMD_DELAY_THLD,            D_SMD_DELAY_THLD},
	{KEY_SMD_DELAY2_THLD,           D_SMD_DELAY2_THLD},
	{KEY_SMD_ENABLE_TESTPT1,        SMD_TP1},
	{KEY_SMD_ENABLE_TESTPT2,        SMD_TP2},
	{KEY_SMD_EXE_STATE,             D_SMD_EXE_STATE},
	{KEY_SMD_DELAY_CNTR,            D_SMD_DELAY_CNTR},
	{KEY_BM_ENABLE,                 D_BM_ENABLE},
	{KEY_BM_BATCH_CNTR,             D_BM_BATCH_CNTR},
	{KEY_BM_BATCH_THLD,             D_BM_BATCH_THLD},
	{KEY_BM_NUMWORD_TOFILL,         D_BM_NUMWORD_TOFILL},
	{KEY_SO_DATA,                   D_SO_DATA},
	{KEY_P_INIT,                    D_P_INIT},
	{KEY_CFG_OUT_CPASS,             CFG_OUT_CPASS},
	{KEY_CFG_OUT_PRESS,             CFG_OUT_PRESS},
	{KEY_CFG_OUT_STEPDET,           CFG_OUT_STEPDET},
	{KEY_CFG_3QUAT_ODR,             D_ODR_S1},
	{KEY_CFG_6QUAT_ODR,             D_ODR_S2},
	{KEY_CFG_PQUAT6_ODR,            D_ODR_S3},
	{KEY_CFG_ACCL_ODR,              D_ODR_S4},
	{KEY_CFG_GYRO_ODR,              D_ODR_S5},
	{KEY_CFG_CPASS_ODR,             D_ODR_S6},
	{KEY_CFG_PRESS_ODR,             D_ODR_S7},
	{KEY_CFG_9QUAT_ODR,             D_ODR_S8},
	{KEY_CFG_PQUAT9_ODR,            D_ODR_S9},
	{KEY_ODR_CNTR_3QUAT,            D_ODR_CNTR_S1},
	{KEY_ODR_CNTR_6QUAT,            D_ODR_CNTR_S2},
	{KEY_ODR_CNTR_PQUAT,            D_ODR_CNTR_S3},
	{KEY_ODR_CNTR_ACCL,             D_ODR_CNTR_S4},
	{KEY_ODR_CNTR_GYRO,             D_ODR_CNTR_S5},
	{KEY_ODR_CNTR_CPASS,            D_ODR_CNTR_S6},
	{KEY_ODR_CNTR_PRESS,            D_ODR_CNTR_S7},
	{KEY_ODR_CNTR_9QUAT,			D_ODR_CNTR_S8},
	{KEY_ODR_CNTR_PQUAT9,			D_ODR_CNTR_S9},
	{KEY_DMP_RUN_CNTR,              D_DMP_RUN_CNTR},
	{KEY_DMP_LPQ0,                  D_FS_LPQ0},
	{KEY_DMP_LPQ1,					D_FS_LPQ1},
	{KEY_DMP_LPQ2,					D_FS_LPQ2},
	{KEY_DMP_LPQ3,					D_FS_LPQ3},
	{KEY_DMP_Q0,					D_FS_Q0},
	{KEY_DMP_Q1,					D_FS_Q1},
	{KEY_DMP_Q2,					D_FS_Q2},
	{KEY_DMP_Q3,					D_FS_Q3},
	{KEY_DMP_9Q0,					D_FS_9Q0},
	{KEY_DMP_9Q1,					D_FS_9Q1},
	{KEY_DMP_9Q2,					D_FS_9Q2},
	{KEY_DMP_9Q3,					D_FS_9Q3},
	{KEY_CPASS_STATUS_CHK,          D_CPASS_STATUS_CHK},
	{KEY_CFG_FB_GAIN_GYRO_ON,		CFG_FB_GAIN_GYRO_ON},
	{KEY_LCD_UP_DOWN_ENABLE,		D_LCD_UP_DOWN_ENABLE},
	{KEY_LCD_TIME_THRESH,			D_LCD_TIME_THRESH},
	{KEY_LCD_UP_X_THRESH,			D_LCD_UP_X_THRESH},
	{KEY_LCD_UP_Y_THRESH,			D_LCD_UP_Y_THRESH},
	{KEY_LCD_UP_Z_THRESH,			D_LCD_UP_Z_THRESH},
	{KEY_LCD_DOWN_X_THRESH,			D_LCD_DOWN_X_THRESH},
	{KEY_LCD_DOWN_Y_THRESH,			D_LCD_DOWN_Y_THRESH},
	{KEY_LCD_DOWN_Z_THRESH,			D_LCD_DOWN_Z_THRESH},
	{KEY_LCD_STATUS,				D_LCD_STATUS},
	{KEY_CFG_QSHOT_START_INT,		CFG_QSHOT_START_INT},
	{KEY_CFG_QSHOT_FINISH_INT,		CFG_QSHOT_FINISH_INT},
	{KEY_D_QSHOT_START_ANGLE,		D_QSHOT_START_ANGLE},
	{KEY_D_QSHOT_FINISH_ANGLE,		D_QSHOT_FINISH_ANGLE},
};
#define NUM_LOCAL_KEYS (sizeof(dmpTConfig)/sizeof(dmpTConfig[0]))

static struct tKeyLabel keys[NUM_KEYS];

unsigned short inv_dmp_get_address(unsigned short key)
{
	static int isSorted;

	if (!isSorted) {
		int kk;
		for (kk = 0; kk < NUM_KEYS; ++kk) {
			keys[kk].addr = 0xffff;
			keys[kk].key = kk;
		}
		for (kk = 0; kk < NUM_LOCAL_KEYS; ++kk)
			keys[dmpTConfig[kk].key].addr = dmpTConfig[kk].addr;
		isSorted = 1;
	}
	if (key >= NUM_KEYS) {
		pr_err("ERROR!! key not exist=%d!\n", key);
		return 0xffff;
	}
	if (0xffff == keys[key].addr)
		pr_err("ERROR!!key not local=%d!\n", key);
	return keys[key].addr;
}
