/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _SYNAPTICS_RMI4_H_
#define _SYNAPTICS_RMI4_H_

#define DISABLE_IRQ_WHEN_ENTER_DEEPSLEEP

#define SYNAPTICS_RMI4_DRIVER_VERSION "DS5 1.0"
#include <linux/device.h>
#include <linux/i2c/synaptics_rmi.h>
#include <linux/regulator/consumer.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if defined(CONFIG_INPUT_BOOSTER)
//to enabled common touch booster. This must be included.
#include <linux/input/input_booster.h>
#endif
/* To support suface touch, firmware should support data
 * which is required related app ex) MT_PALM ...
 * Synpatics IC report those data through F51's edge swipe
 * fucntionality.
 */

/* feature default mode */
#define DEFAULT_ENABLE	1
#define DEFAULT_DISABLE	0

/* feature define */
#define TSP_BOOSTER	/* DVFS feature : TOUCH BOOSTER */
#define USE_OPEN_CLOSE	/* Use when CONFIG_HAS_EARLYSUSPEND is disabled */
#define REPORT_2D_W
#define REDUCE_I2C_DATA_LENGTH
#define USE_SENSOR_SLEEP

#define	TSP_IRQ_TYPE_LEVEL	IRQF_TRIGGER_LOW | IRQF_ONESHOT
#define	TSP_IRQ_TYPE_EDGE	IRQF_TRIGGER_FALLING

#if defined(CONFIG_SEC_MONDRIAN_PROJECT)
#define TOUCHKEY_ENABLE
#define USE_RECENT_TOUCHKEY
#define PROXIMITY
#define EDGE_SWIPE
#define TKEY_BOOSTER
#define SYNAPTICS_DEVICE_NAME	"T320"
#define USE_PALM_REJECTION_KERNEL

#elif defined(CONFIG_SEC_CHAGALL_PROJECT)
#define PROXIMITY
#define EDGE_SWIPE
#define GLOVE_MODE
#define SYNAPTICS_DEVICE_NAME	"T807"
#define USE_PALM_REJECTION_KERNEL
#define ENABLE_F12_OBJTYPE

#elif defined(CONFIG_SEC_KLIMT_PROJECT)
#define PROXIMITY
#define EDGE_SWIPE
#define GLOVE_MODE
#define SYNAPTICS_DEVICE_NAME	"T707"
#define USE_PALM_REJECTION_KERNEL
#define ENABLE_F12_OBJTYPE

#elif defined(CONFIG_SEC_K_PROJECT)
#define PROXIMITY
#define EDGE_SWIPE
#define SIDE_TOUCH
#define USE_HOVER_REZERO
#define GLOVE_MODE
#define USE_SHUTDOWN_CB
#define CHECK_BASE_FIRMWARE
#define USE_ACTIVE_REPORT_RATE
#define USE_F51_OFFSET_CALCULATE
#define SYNAPTICS_DEVICE_NAME	"G900F"
#define USE_STYLUS
#define USE_DETECTION_FLAG2
#define USE_EDGE_EXCLUSION

#elif defined(CONFIG_SEC_KACTIVE_PROJECT)
#define PROXIMITY
#define EDGE_SWIPE
#define SIDE_TOUCH
#define USE_HOVER_REZERO
#define GLOVE_MODE
#define USE_SHUTDOWN_CB
#define CHECK_BASE_FIRMWARE
#define USE_ACTIVE_REPORT_RATE
#define USE_F51_OFFSET_CALCULATE
#define SYNAPTICS_DEVICE_NAME	"G870"
#define USE_STYLUS
#define USE_DETECTION_FLAG2
#define USE_EDGE_EXCLUSION

#elif defined(CONFIG_SEC_KSPORTS_PROJECT)
#define PROXIMITY
#define EDGE_SWIPE
#define SIDE_TOUCH
#define USE_HOVER_REZERO
#define GLOVE_MODE
#define USE_SHUTDOWN_CB
#define CHECK_BASE_FIRMWARE
#define USE_ACTIVE_REPORT_RATE
#define USE_F51_OFFSET_CALCULATE
#define SYNAPTICS_DEVICE_NAME	"G860"
#define USE_STYLUS
#define USE_DETECTION_FLAG2
#define USE_EDGE_EXCLUSION

#elif defined(CONFIG_SEC_H_PROJECT)
#define PROXIMITY
#define EDGE_SWIPE
#define USE_HOVER_REZERO
#define GLOVE_MODE
#define READ_LCD_ID
#define SYNAPTICS_DEVICE_NAME	"N9005"
#define USE_PALM_REJECTION_KERNEL
#define USE_EDGE_EXCLUSION
#define USE_EDGE_SWIPE_WIDTH_MAJOR

#elif defined(CONFIG_SEC_F_PROJECT)
#define PROXIMITY
#define EDGE_SWIPE
#define USE_HOVER_REZERO
#define GLOVE_MODE
#define TOUCHKEY_ENABLE
#define EDGE_SWIPE_SCALE
#define TOUCHKEY_LED_GPIO
#define USE_PALM_REJECTION_KERNEL
#define USE_EDGE_EXCLUSION
#define USE_EDGE_SWIPE_WIDTH_MAJOR

#elif defined(CONFIG_SEC_GNOTE_PROJECT)
#define SYNAPTICS_DEVICE_NAME	"S5006"
#define USE_PALM_REJECTION_KERNEL
#define USE_EDGE_EXCLUSION
#define USE_EDGE_SWIPE_WIDTH_MAJOR
#undef TSP_BOOSTER     ///// temp code for new model setup

#elif defined(CONFIG_SEC_HESTIA_PROJECT)
#define PROXIMITY
#define EDGE_SWIPE
#define GLOVE_MODE
#define USE_EDGE_SWIPE_WIDTH_MAJOR
#define EDGE_SWIPE_SCALE
#define USE_PALM_REJECTION_KERNEL
#undef CONFIG_HAS_EARLYSUSPEND

#elif defined(CONFIG_SEC_RUBENS_PROJECT)
#if defined(CONFIG_SEC_RUBENSLTE_COMMON)
#define SYNAPTICS_DEVICE_NAME	"T365"
#else
#define SYNAPTICS_DEVICE_NAME	"T360"
#endif
#undef CONFIG_HAS_EARLYSUSPEND
#undef TSP_BOOSTER
/* changes to fix PLM P140707-06422(PALM TOUCH) issue in RUBEN */
#define PROXIMITY
#define EDGE_SWIPE

#else /* default undefine all */
#undef PROXIMITY			/* Use F51 - edge_swipe, hover, side_touch, stylus, hand_grip */
#undef EDGE_SWIPE			/* Screen Caputure, and Palm pause */
#undef EDGE_SWIPE_SCALE			/* Recalculate edge_swipe data */
#undef USE_PALM_REJECTION_KERNEL	/* Fix Firmware bug.(Finger_status, PALM flag) */
#undef SIDE_TOUCH			/* Side Touch */
#undef USE_HOVER_REZERO			/* Use hover rezero */
#undef USE_EDGE_EXCLUSION		/* Disable edge hover when grip the phone */
#undef GLOVE_MODE			/* Glove Mode */
#undef HAND_GRIP_MODE			/* Hand Grip mode */
#undef USE_EDGE_SWIPE_WIDTH_MAJOR	/* Upgrade model use it. KK new model use SUMSIZE */
#undef USE_STYLUS			/* Use Stylus */
#undef USE_DETECTION_FLAG2		/* Use Detection Flag2 Register: Edge_swipe, Side_touch */

#undef TSP_TURNOFF_AFTER_PROBE		/* Turn off touch IC after probe success, will be turned by InputRedaer */
#undef USE_SHUTDOWN_CB			/* Use shutdown callback function */
#undef READ_LCD_ID			/* Need to separate f/w according to LCD ID */
#undef CHECK_BASE_FIRMWARE		/* Check base fw version. base fw version is PR number */

#undef TOUCHKEY_ENABLE			/* TSP/Tkey in one chip */
#undef TKEY_BOOSTER			/* Tkey booster for performance */
#undef TOUCHKEY_LED_GPIO		/* Tkey led use gpio pin rather than pmic regulator */
#endif

#ifndef SYNAPTICS_DEVICE_NAME
#define SYNAPTICS_DEVICE_NAME	"SEC_XX_XX"
#endif

#if defined(CONFIG_LEDS_CLASS) && defined(TOUCHKEY_ENABLE)
#include <linux/leds.h>
#define TOUCHKEY_BACKLIGHT "button-backlight"
#endif

#if defined(TSP_BOOSTER) || defined(TKEY_BOOSTER)
#define DVFS_STAGE_NINTH	9
#define DVFS_STAGE_PENTA	5
#define DVFS_STAGE_TRIPLE	3
#define DVFS_STAGE_DUAL		2
#define DVFS_STAGE_SINGLE	1
#define DVFS_STAGE_NONE		0
#include <linux/cpufreq.h>

#define TOUCH_BOOSTER_OFF_TIME		500
#define TOUCH_BOOSTER_CHG_TIME		130
#define TOUCH_BOOSTER_HIGH_OFF_TIME	1000
#define TOUCH_BOOSTER_HIGH_CHG_TIME	500
#endif

/* TA_CON mode @ H mode */
#define TA_CON_REVISION		0xFF

#ifdef GLOVE_MODE
#define GLOVE_MODE_EN (1 << 0)
#define CLOSED_COVER_EN (1 << 1)
#define FAST_DETECT_EN (1 << 2)
#endif

#define SYNAPTICS_HW_RESET_TIME	100
#define SYNAPTICS_POWER_MARGIN_TIME	150

#define SYNAPTICS_PRODUCT_ID_NONE	0
#define SYNAPTICS_PRODUCT_ID_S5000	1
#define SYNAPTICS_PRODUCT_ID_S5050	2
#define SYNAPTICS_PRODUCT_ID_S5100	3
#define SYNAPTICS_PRODUCT_ID_S5700	4
#define SYNAPTICS_PRODUCT_ID_S5707	5
#define SYNAPTICS_PRODUCT_ID_S5708	6
#define SYNAPTICS_PRODUCT_ID_S5006	7
#define SYNAPTICS_PRODUCT_ID_S5710	8

#define SYNAPTICS_IC_REVISION_NONE	0x00
#define SYNAPTICS_IC_REVISION_A0	0xA0
#define SYNAPTICS_IC_REVISION_A1	0xA1
#define SYNAPTICS_IC_REVISION_A2	0xA2
#define SYNAPTICS_IC_REVISION_A3	0xA3
#define SYNAPTICS_IC_REVISION_B0	0xB0
#define SYNAPTICS_IC_REVISION_B1	0xB1
#define SYNAPTICS_IC_REVISION_B2	0xB2
#define SYNAPTICS_IC_REVISION_AF	0xAF
#define SYNAPTICS_IC_REVISION_BF	0xBF

/* old bootloader(V606) do not supply Guest Thread image.
 * new bootloader(V646) supply Guest Thread image.
 */
#define SYNAPTICS_IC_OLD_BOOTLOADER	"06"
#define SYNAPTICS_IC_NEW_BOOTLOADER	"46"

#define FW_IMAGE_NAME_NONE		NULL
#define FW_IMAGE_NAME_S5050_H		"tsp_synaptics/synaptics_s5050_h.fw"
#define FW_IMAGE_NAME_S5100_K_A2_FHD	"tsp_synaptics/synaptics_s5100_k_a2_fhd.fw"
#define FW_IMAGE_NAME_S5100_K_A3	"tsp_synaptics/synaptics_s5100_k_a3.fw"
#define FW_IMAGE_NAME_S5100_K_A3_KOR	"tsp_synaptics/synaptics_s5100_kkor_a3.fw"
#define FW_IMAGE_NAME_S5100_K_ACTIVE	"tsp_synaptics/synaptics_s5100_k_active.fw"
#define FW_IMAGE_NAME_S5100_K_SPORTS	"tsp_synaptics/synaptics_s5100_k_sports.fw"
#define FW_IMAGE_NAME_S5100_HESTIA	"tsp_synaptics/synaptics_s5100_hestia.fw"
#define FW_IMAGE_NAME_S5100_PSLTE	"tsp_synaptics/synaptics_s5100_pslte.fw"
#define FW_IMAGE_NAME_S5707		"tsp_synaptics/synaptics_s5707.fw"
#define FW_IMAGE_NAME_S5707_KLIMT	"tsp_synaptics/synaptics_s5707_klimt.fw"
#define FW_IMAGE_NAME_S5707_RUBENS	"tsp_synaptics/synaptics_s5707_rubens.fw"
#define FW_IMAGE_NAME_S5708		"tsp_synaptics/synaptics_s5708.fw"
#define FW_IMAGE_NAME_S5050		"tsp_synaptics/synaptics_s5050.fw"
#define FW_IMAGE_NAME_S5050_F		"tsp_synaptics/synaptics_s5050_f.fw"
#define FW_IMAGE_NAME_S5006		"tsp_synaptics/synaptics_s5006.fw"
#define FW_IMAGE_NAME_S5710		"tsp_synaptics/synaptics_chagall_5710.fw"


#define SYNAPTICS_FACTORY_TEST_PASS	2
#define SYNAPTICS_FACTORY_TEST_FAIL	1
#define SYNAPTICS_FACTORY_TEST_NONE	0

#define SYNAPTICS_MAX_FW_PATH		64

#define SYNAPTICS_DEFAULT_UMS_FW	"/sdcard/synaptics.fw"

#define DATE_OF_FIRMWARE_BIN_OFFSET	0xEF00
#define IC_REVISION_BIN_OFFSET		0xEF02
#define FW_VERSION_BIN_OFFSET		0xEF03

#define DATE_OF_FIRMWARE_BIN_OFFSET_S5050	0x016D00
#define IC_REVISION_BIN_OFFSET_S5050		0x016D02
#define FW_VERSION_BIN_OFFSET_S5050		0x016D03

#define DATE_OF_FIRMWARE_BIN_OFFSET_S5100_A2	0x00B0
#define IC_REVISION_BIN_OFFSET_S5100_A2		0x00B2
#define FW_VERSION_BIN_OFFSET_S5100_A2		0x00B3

#define DATE_OF_FIRMWARE_BIN_OFFSET_S5100_PS	0x015D00
#define IC_REVISION_BIN_OFFSET_S5100_PS		0x015D02
#define FW_VERSION_BIN_OFFSET_S5100_PS		0x015D03

#define PDT_PROPS (0X00EF)
#define PDT_START (0x00E9)
#define PDT_END (0x000A)
#define PDT_ENTRY_SIZE (0x0006)
#define PAGES_TO_SERVICE (10)
#define PAGE_SELECT_LEN (2)

#define SYNAPTICS_RMI4_F01 (0x01)
#define SYNAPTICS_RMI4_F11 (0x11)
#define SYNAPTICS_RMI4_F12 (0x12)
#define SYNAPTICS_RMI4_F1A (0x1a)
#define SYNAPTICS_RMI4_F34 (0x34)
#define SYNAPTICS_RMI4_F51 (0x51)
#define SYNAPTICS_RMI4_F54 (0x54)
#define SYNAPTICS_RMI4_F55 (0x55)
#define SYNAPTICS_RMI4_F60 (0x60)
#define SYNAPTICS_RMI4_FDB (0xdb)

#define SYNAPTICS_RMI4_PRODUCT_INFO_SIZE	2
#define SYNAPTICS_RMI4_DATE_CODE_SIZE		3
#define SYNAPTICS_RMI4_PRODUCT_ID_SIZE		10
#define SYNAPTICS_RMI4_BUILD_ID_SIZE		3
#define SYNAPTICS_RMI4_PRODUCT_ID_LENGTH	10
#define SYNAPTICS_RMI4_PACKAGE_ID_SIZE		4

#define MAX_NUMBER_OF_BUTTONS	4
#define MAX_INTR_REGISTERS	4
#define MAX_NUMBER_OF_FINGERS	12
#define F12_FINGERS_TO_SUPPORT	10

#define MASK_16BIT 0xFFFF
#define MASK_8BIT 0xFF
#define MASK_7BIT 0x7F
#define MASK_6BIT 0x3F
#define MASK_5BIT 0x1F
#define MASK_4BIT 0x0F
#define MASK_3BIT 0x07
#define MASK_2BIT 0x03
#define MASK_1BIT 0x01

#define F12_FINGERS_TO_SUPPORT 10

#define INVALID_X 65535
#define INVALID_Y 65535

#define RPT_TYPE (1 << 0)
#define RPT_X_LSB (1 << 1)
#define RPT_X_MSB (1 << 2)
#define RPT_Y_LSB (1 << 3)
#define RPT_Y_MSB (1 << 4)
#define RPT_Z (1 << 5)
#define RPT_WX (1 << 6)
#define RPT_WY (1 << 7)
#define RPT_DEFAULT (RPT_TYPE | RPT_X_LSB | RPT_X_MSB | RPT_Y_LSB | RPT_Y_MSB)

#ifdef PROXIMITY
#define F51_FINGER_TIMEOUT 50 /* ms */
#define HOVER_Z_MAX (255)

#define EDGE_SWIPE_DEGREES_MAX (90)
#define EDGE_SWIPE_DEGREES_MIN (-89)
#define EDGE_SWIPE_WIDTH_SCALING_FACTOR (9)

#define EDGE_SWIPE_SUMSIZE_OFFSET	5

#define F51_PROXIMITY_ENABLES_OFFSET (0)
#define F51_SIDE_BUTTON_THRESHOLD_OFFSET	(47)
#define F51_SIDE_BUTTON_PARTIAL_ENABLE_OFFSET	(44)
#define F51_GPIP_EDGE_EXCLUSION_RX_OFFSET	(32)


#define FINGER_HOVER_DIS (0 << 0)
#define FINGER_HOVER_EN (1 << 0)
#define AIR_SWIPE_EN (1 << 1)
#define LARGE_OBJ_EN (1 << 2)
#define HOVER_PINCH_EN (1 << 3)
/* This command is reserved..
#define NO_PROXIMITY_ON_TOUCH_EN (1 << 5)
#define CONTINUOUS_LOAD_REPORT_EN (1 << 6)
*/
#define ENABLE_HANDGRIP_RECOG (1 << 6)
#define SLEEP_PROXIMITY (1 << 7)

#define F51_GENERAL_CONTROL_OFFSET (1)
#define F51_GENERAL_CONTROL2_OFFSET (2)
#define JIG_TEST_EN	(1 << 0)
#define JIG_COMMAND_EN	(1 << 1)
#define NO_PROXIMITY_ON_TOUCH (1 << 2)
#define CONTINUOUS_LOAD_REPORT (1 << 3)
#define HOST_REZERO_COMMAND (1 << 4)
#define EDGE_SWIPE_EN (1 << 5)
#define HSYNC_STATUS	(1 << 6)
#define F51_GENERAL_CONTROL (NO_PROXIMITY_ON_TOUCH | HOST_REZERO_COMMAND | EDGE_SWIPE_EN)
#define F51_GENERAL_CONTROL_NO_HOST_REZERO (NO_PROXIMITY_ON_TOUCH | EDGE_SWIPE_EN)

/* f51_query feature(proximity_controls) */
#define HAS_FINGER_HOVER (1 << 0)
#define HAS_AIR_SWIPE (1 << 1)
#define HAS_LARGE_OBJ (1 << 2)
#define HAS_HOVER_PINCH (1 << 3)
#define HAS_EDGE_SWIPE (1 << 4)
#define HAS_SINGLE_FINGER (1 << 5)
#define HAS_GRIP_SUPPRESSION (1 << 6)
#define HAS_PALM_REJECTION (1 << 7)
/* f51_query feature-2(proximity_controls_2) */
#define HAS_PROFILE_HANDEDNESS (1 << 0)
#define HAS_LOWG (1 << 1)
#define HAS_FACE_DETECTION (1 << 2)
#define HAS_SIDE_BUTTONS (1 << 3)
#define HAS_CAMERA_GRIP_DETECTION (1 << 4)
/* Reserved 5~7 */

/* Use Detection Flag for F51 feature */
#define HOVER_UN_DETECTED		(0 << 0)
#define HOVER_DETECTED			(1 << 0)
#define AIR_SWIPE_DETECTED		(1 << 1)
#define LARGE_OBJECT_DETECTED	(1 << 2)
#define HOVER_PINCH_DETECTED	(1 << 3)
#define LOWG_DETECTED			(1 << 4)
#define PROFILE_HANDEDNESS_DETECTED	((1 << 5) | (1 << 6))
#define FACE_DETECTED			(1 << 7)
#define CAMERA_GRIP_DETECTED	(1 << 0)
#define EDGE_SWIPE_DETECTED		(1 << 0)
#define SIDE_BUTTON_DETECTED	(1 << 1)


/* F51 General Control enable */

/* F51 General Control2 enable */
#define FACE_DETECTED_ENABLE	(1 << 0)
#define SIDE_BUTTONS_ENABLE		(1 << 1)
#define SIDE_BUTTONS_PRODUCTION_TEST	(1 << 2)
#define SIDE_TOUCH_ONLY_ACTIVE	(1 << 3)

#ifdef EDGE_SWIPE

#if defined(CONFIG_SEC_MONDRIAN_PROJECT) || defined(CONFIG_SEC_CHAGALL_PROJECT)\
	|| defined(CONFIG_SEC_KLIMT_PROJECT) || defined(CONFIG_SEC_RUBENS_PROJECT)
#define EDGE_SWIPE_DATA_OFFSET	3
#else
#define EDGE_SWIPE_DATA_OFFSET	9
#endif

#define EDGE_SWIPE_WIDTH_MAX	255
#define EDGE_SWIPE_PALM_MAX		1
#endif

#define F51_DATA_RESERVED_SIZE	(1)
#define F51_DATA_1_SIZE (4)	/* FINGER HOVER */
#define F51_DATA_2_SIZE (1)	/* HOVER PINCH */
#define F51_DATA_3_SIZE (1)	/* AIR_SWIPE  | LARGE_OBJ */
#define F51_DATA_4_SIZE (2)	/* SIDE_BUTTON */
#define F51_DATA_5_SIZE (1)	/* CAMERA GRIP DETECTION */
#define F51_DATA_6_SIZE (2)	/* DETECTION FLAG2 */
/*
 * DATA_5, DATA_6, RESERVED
 * 5: 1 byte RESERVED + CAM GRIP DETECT
 * 6: 1 byte RESERVED + SIDE BUTTON DETECT + HAND EDGE SWIPE DETECT
 * R: 1 byte RESERVED
 */
#endif

#define SYN_I2C_RETRY_TIMES 10
#define MAX_F11_TOUCH_WIDTH 15

#define CHECK_STATUS_TIMEOUT_MS 100

#define F01_STD_QUERY_LEN 21
#define F01_BUID_ID_OFFSET 18
#define F01_PACKAGE_ID_LEN	4
#define F11_STD_QUERY_LEN 9
#define F11_STD_CTRL_LEN 10
#define F11_STD_DATA_LEN 12

#define STATUS_NO_ERROR 0x00
#define STATUS_RESET_OCCURRED 0x01
#define STATUS_INVALID_CONFIG 0x02
#define STATUS_DEVICE_FAILURE 0x03
#define STATUS_CONFIG_CRC_FAILURE 0x04
#define STATUS_FIRMWARE_CRC_FAILURE 0x05
#define STATUS_CRC_IN_PROGRESS 0x06

#define NORMAL_OPERATION (0 << 0)
#define SENSOR_SLEEP (1 << 0)
#define NO_SLEEP_OFF (0 << 2)
#define NO_SLEEP_ON (1 << 2)
#define CHARGER_CONNECTED (1 << 5)
#define CHARGER_DISCONNECTED	0xDF

#define CONFIGURED (1 << 7)

#define TSP_NEEDTO_REBOOT	(-ECONNREFUSED)
#define MAX_TSP_REBOOT		3

/*
 * Each 3-bit finger status field represents the following:
 * 000 = finger not present
 * 001 = finger present and data accurate
 * 010 = stylus pen (passive pen)
 * 011 = palm touch
 * 100 = not used
 * 101 = hover
 * 110 = glove touch
 */
#define FINGER_NOT_PRESENT	0x0
#define FINGER_PRESSED		0x1
#define STYLUS_PRESSED		0x2
#define PALM_PRESSED		0x3
#define HOVER_PRESSED		0x5
#define GLOVE_PRESSED		0x6

#ifdef ENABLE_F12_OBJTYPE 
/* Define for object type report enable Mask(F12_2D_CTRL23) */
#define OBJ_TYPE_FINGER			(1 << 0)
#define OBJ_TYPE_PASSIVE_STYLUS	(1 << 1)
#define OBJ_TYPE_PALM			(1 << 2)
#define OBJ_TYPE_UNCLASSIFIED	(1 << 3)
#define OBJ_TYPE_HOVER			(1 << 4)
#define OBJ_TYPE_GLOVE			(1 << 5)
#define OBJ_TYPE_NARROW_SWIPE	(1 << 6)
#define OBJ_TYPE_HANDEDGE		(1 << 7)
#define OBJ_TYPE_DEFAUT			(0x85)
/*OBJ_TYPE_FINGER, OBJ_TYPE_UNCLASSIFIED, OBJ_TYPE_HANDEDGE*/
#endif
/*
 * synaptics_rmi4_set_custom_ctrl_register()
 * mode TRUE : read, mode FALSE : write
 */
#define REGISTER_READ	1
#define REGISTER_WRITE	0

/*
 * synaptics absolute register address
 * if changed register address, fix below address value
 */
#ifdef USE_ACTIVE_REPORT_RATE
#define REGISTER_ADDR_CHANGE_REPORT_RATE	0x012A
#define REGISTER_ADDR_FORCE_CALIBRATION		0x0138

#define REPORT_RATE_90HZ	0x04
#define REPORT_RATE_60HZ	0x16
#define REPORT_RATE_30HZ	0x50
#define REPORT_RATE_FORCE_UPDATE	0x04
#endif
#ifdef SIDE_TOUCH
#define F51_CUSTOM_CTRL78_OFFSET	47
#endif
#ifdef USE_STYLUS
#define F51_CUSTOM_CTRL87_OFFSET	61
#endif


extern unsigned int system_rev;

struct synaptics_rmi4_f01_device_status {
	union {
		struct {
			unsigned char status_code:4;
			unsigned char reserved:2;
			unsigned char flash_prog:1;
			unsigned char unconfigured:1;
		} __packed;
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f12_query_5 {
	union {
		struct {
			unsigned char size_of_query6;
			struct {
				unsigned char ctrl0_is_present:1;
				unsigned char ctrl1_is_present:1;
				unsigned char ctrl2_is_present:1;
				unsigned char ctrl3_is_present:1;
				unsigned char ctrl4_is_present:1;
				unsigned char ctrl5_is_present:1;
				unsigned char ctrl6_is_present:1;
				unsigned char ctrl7_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl8_is_present:1;
				unsigned char ctrl9_is_present:1;
				unsigned char ctrl10_is_present:1;
				unsigned char ctrl11_is_present:1;
				unsigned char ctrl12_is_present:1;
				unsigned char ctrl13_is_present:1;
				unsigned char ctrl14_is_present:1;
				unsigned char ctrl15_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl16_is_present:1;
				unsigned char ctrl17_is_present:1;
				unsigned char ctrl18_is_present:1;
				unsigned char ctrl19_is_present:1;
				unsigned char ctrl20_is_present:1;
				unsigned char ctrl21_is_present:1;
				unsigned char ctrl22_is_present:1;
				unsigned char ctrl23_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl24_is_present:1;
				unsigned char ctrl25_is_present:1;
				unsigned char ctrl26_is_present:1;
				unsigned char ctrl27_is_present:1;
				unsigned char ctrl28_is_present:1;
				unsigned char ctrl29_is_present:1;
				unsigned char ctrl30_is_present:1;
				unsigned char ctrl31_is_present:1;
			} __packed;
		};
		unsigned char data[5];
	};
};

struct synaptics_rmi4_f12_query_8 {
	union {
		struct {
			unsigned char size_of_query9;
			struct {
				unsigned char data0_is_present:1;
				unsigned char data1_is_present:1;
				unsigned char data2_is_present:1;
				unsigned char data3_is_present:1;
				unsigned char data4_is_present:1;
				unsigned char data5_is_present:1;
				unsigned char data6_is_present:1;
				unsigned char data7_is_present:1;
			} __packed;
			struct {
				unsigned char data8_is_present:1;
				unsigned char data9_is_present:1;
				unsigned char data10_is_present:1;
				unsigned char data11_is_present:1;
				unsigned char data12_is_present:1;
				unsigned char data13_is_present:1;
				unsigned char data14_is_present:1;
				unsigned char data15_is_present:1;
			} __packed;
		};
		unsigned char data[3];
	};
};

struct synaptics_rmi4_f12_query_10 {
	union {
		struct {
			unsigned char f12_query10_b0__4:5;
			unsigned char glove_mode_feature:1;
			unsigned char f12_query10_b6__7:2;
		} __packed;
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f12_ctrl_8 {
	union {
		struct {
			unsigned char max_x_coord_lsb;
			unsigned char max_x_coord_msb;
			unsigned char max_y_coord_lsb;
			unsigned char max_y_coord_msb;
			unsigned char rx_pitch_lsb;
			unsigned char rx_pitch_msb;
			unsigned char tx_pitch_lsb;
			unsigned char tx_pitch_msb;
			unsigned char low_rx_clip;
			unsigned char high_rx_clip;
			unsigned char low_tx_clip;
			unsigned char high_tx_clip;
			unsigned char num_of_rx;
			unsigned char num_of_tx;
		};
		unsigned char data[14];
	};
};

struct synaptics_rmi4_f12_ctrl_9 {
	union {
		struct {
			unsigned char touch_threshold;
			unsigned char lift_hysteresis;
			unsigned char small_z_scale_factor_lsb;
			unsigned char small_z_scale_factor_msb;
			unsigned char large_z_scale_factor_lsb;
			unsigned char large_z_scale_factor_msb;
			unsigned char small_large_boundary;
			unsigned char wx_scale;
			unsigned char wx_offset;
			unsigned char wy_scale;
			unsigned char wy_offset;
			unsigned char x_size_lsb;
			unsigned char x_size_msb;
			unsigned char y_size_lsb;
			unsigned char y_size_msb;
			unsigned char gloved_finger;
		};
		unsigned char data[16];
	};
};

struct synaptics_rmi4_f12_ctrl_11 {
	union {
		struct {
			unsigned char small_corner;
			unsigned char large_corner;
			unsigned char jitter_filter_strength;
			unsigned char x_minimum_z;
			unsigned char y_minimum_z;
			unsigned char x_maximum_z;
			unsigned char y_maximum_z;
			unsigned char x_amplitude;
			unsigned char y_amplitude;
			unsigned char gloved_finger_jitter_filter_strength;
		};
		unsigned char data[10];
	};
};

struct synaptics_rmi4_f12_ctrl_23 {
	union {
		struct {
			unsigned char obj_type_enable;
			unsigned char max_reported_objects;
		};
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f12_finger_data {
	unsigned char object_type_and_status;
	unsigned char x_lsb;
	unsigned char x_msb;
	unsigned char y_lsb;
	unsigned char y_msb;
#ifdef REPORT_2D_Z
	unsigned char z;
#endif
#ifdef REPORT_2D_W
	unsigned char wx;
	unsigned char wy;
#endif
};

struct synaptics_rmi4_f1a_query {
	union {
		struct {
			unsigned char max_button_count:3;
			unsigned char reserved:5;
			unsigned char has_general_control:1;
			unsigned char has_interrupt_enable:1;
			unsigned char has_multibutton_select:1;
			unsigned char has_tx_rx_map:1;
			unsigned char has_perbutton_threshold:1;
			unsigned char has_release_threshold:1;
			unsigned char has_strongestbtn_hysteresis:1;
			unsigned char has_filter_strength:1;
		} __packed;
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f1a_control_0 {
	union {
		struct {
			unsigned char multibutton_report:2;
			unsigned char filter_mode:2;
			unsigned char reserved:4;
		} __packed;
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f1a_control {
	struct synaptics_rmi4_f1a_control_0 general_control;
	unsigned char button_int_enable;
	unsigned char multi_button;
	unsigned char *txrx_map;
	unsigned char *button_threshold;
	unsigned char button_release_threshold;
	unsigned char strongest_button_hysteresis;
	unsigned char filter_strength;
};

struct synaptics_rmi4_f1a_handle {
	int button_bitmask_size;
	unsigned char max_count;
	unsigned char valid_button_count;
	unsigned char *button_data_buffer;
	unsigned char *button_map;
	struct synaptics_rmi4_f1a_query button_query;
	struct synaptics_rmi4_f1a_control button_control;
};

struct synaptics_rmi4_f34_ctrl_3 {
	union {
		struct {
			unsigned char fw_release_month;
			unsigned char fw_release_date;
			unsigned char fw_release_revision;
			unsigned char fw_release_version;
		};
		unsigned char data[4];
	};
};


struct synaptics_rmi4_f34_handle {
	unsigned char status;
	unsigned char cmd;
	unsigned short bootloaderid;
	unsigned short blocksize;
	unsigned short imageblockcount;
	unsigned short configblockcount;
	unsigned short blocknum;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	bool inflashprogmode;
	unsigned char intr_mask;
	struct mutex attn_mutex;
	struct synaptics_rmi4_f34_fn_ptr *fn_ptr;
};

#ifdef PROXIMITY
struct synaptics_rmi4_f51_query {
	union {
		struct {
			unsigned char query_register_count;
			unsigned char data_register_count;
			unsigned char control_register_count;
			unsigned char command_register_count;
			unsigned char features;
			unsigned char side_touch_feature;
		};
		unsigned char data[6];
	};
};

struct synaptics_rmi4_f51_data {
	union {
		struct {
			unsigned char finger_hover_det:1;
			unsigned char air_swipe_det:1;
			unsigned char large_obj_det:1;
			unsigned char f1a_data0_b3:1;
			unsigned char hover_pinch_det:1;
			unsigned char profile_handedness_status:2;
			unsigned char f1a_data0_b5__7:1;
			unsigned char hover_finger_x_4__11;
			unsigned char hover_finger_y_4__11;
			unsigned char hover_finger_xy_0__3;
			unsigned char hover_finger_z;
			unsigned char f1a_data2_b0__6:7;
			unsigned char hover_pinch_dir:1;
			unsigned char air_swipe_dir_0:1;
			unsigned char air_swipe_dir_1:1;
			unsigned char f1a_data3_b2__4:3;
			unsigned char object_present:1;
			unsigned char large_obj_act:2;
		} __packed;
		unsigned char proximity_data[7];
	};
#ifdef EDGE_SWIPE
	union {
		struct {
			unsigned char edge_swipe_x_lsb;
			unsigned char edge_swipe_x_msb;
			unsigned char edge_swipe_y_lsb;
			unsigned char edge_swipe_y_msb;
			unsigned char edge_swipe_z;
			unsigned char edge_swipe_wx;
			unsigned char edge_swipe_wy;
			unsigned char edge_swipe_mm;
			signed char edge_swipe_dg;
		} __packed;
		unsigned char edge_swipe_data[9];
	};
#endif
#ifdef SIDE_TOUCH
	union {
		struct {
			unsigned char side_button_leading;
			unsigned char side_button_trailing;
			unsigned char side_button_cam_det;
		} __packed;
		unsigned char side_button_data[3];
	};
#endif
};

#ifdef EDGE_SWIPE
struct synaptics_rmi4_surface {
	int sumsize;
	int palm;
	int angle;
	int wx;
	int wy;
};
#endif

struct synaptics_rmi4_f51_handle {
	unsigned char edge_swipe_data_offset;
	unsigned char side_button_data_offset;
	unsigned char proximity_enables;
	unsigned char general_control;
	unsigned short proximity_enables_addr;
	unsigned short general_control_addr;
	unsigned short general_control2_addr;
	unsigned short edge_swipe_data_addr;
	unsigned short side_button_data_addr;
	unsigned short detection_flag2_addr;
	unsigned short sidekey_threshold_addr;
	unsigned short stylus_enable_addr;
	struct synaptics_rmi4_surface surface_data;
};
#endif

/*
 * struct synaptics_rmi4_fn_desc - function descriptor fields in PDT
 * @query_base_addr: base address for query registers
 * @cmd_base_addr: base address for command registers
 * @ctrl_base_addr: base address for control registers
 * @data_base_addr: base address for data registers
 * @intr_src_count: number of interrupt sources
 * @fn_number: function number
 */
struct synaptics_rmi4_fn_desc {
	unsigned char query_base_addr;
	unsigned char cmd_base_addr;
	unsigned char ctrl_base_addr;
	unsigned char data_base_addr;
	unsigned char intr_src_count;
	unsigned char fn_number;
};

/*
 * synaptics_rmi4_fn_full_addr - full 16-bit base addresses
 * @query_base: 16-bit base address for query registers
 * @cmd_base: 16-bit base address for data registers
 * @ctrl_base: 16-bit base address for command registers
 * @data_base: 16-bit base address for control registers
 */
struct synaptics_rmi4_fn_full_addr {
	unsigned short query_base;
	unsigned short cmd_base;
	unsigned short ctrl_base;
	unsigned short data_base;
};

struct synaptics_rmi4_f12_extra_data {
	unsigned char data1_offset;
	unsigned char data15_offset;
	unsigned char data15_size;
	unsigned char data15_data[(F12_FINGERS_TO_SUPPORT + 7) / 8];
};

/*
 * struct synaptics_rmi4_fn - function handler data structure
 * @fn_number: function number
 * @num_of_data_sources: number of data sources
 * @num_of_data_points: maximum number of fingers supported
 * @size_of_data_register_block: data register block size
 * @data1_offset: offset to data1 register from data base address
 * @intr_reg_num: index to associated interrupt register
 * @intr_mask: interrupt mask
 * @full_addr: full 16-bit base addresses of function registers
 * @link: linked list for function handlers
 * @data_size: size of private data
 * @data: pointer to private data
 */
struct synaptics_rmi4_fn {
	unsigned char fn_number;
	unsigned char num_of_data_sources;
	unsigned char num_of_data_points;
	unsigned char size_of_data_register_block;
	unsigned char intr_reg_num;
	unsigned char intr_mask;
	struct synaptics_rmi4_fn_full_addr full_addr;
	struct list_head link;
	int data_size;
	void *data;
	void *extra;
};

/*
 * struct synaptics_rmi4_device_info - device information
 * @version_major: rmi protocol major version number
 * @version_minor: rmi protocol minor version number
 * @manufacturer_id: manufacturer id
 * @product_props: product properties information
 * @product_info: product info array
 * @date_code: device manufacture date
 * @tester_id: tester id array
 * @serial_number: device serial number
 * @product_id_string: device product id
 * @support_fn_list: linked list for function handlers
 */
struct synaptics_rmi4_device_info {
	unsigned int version_major;
	unsigned int version_minor;
	unsigned char manufacturer_id;
	unsigned char product_props;
	unsigned char product_info[SYNAPTICS_RMI4_PRODUCT_INFO_SIZE];
	unsigned char date_code[SYNAPTICS_RMI4_DATE_CODE_SIZE];
	unsigned short tester_id;
	unsigned short serial_number;
	unsigned char product_id_string[SYNAPTICS_RMI4_PRODUCT_ID_SIZE + 1];
	unsigned char build_id[SYNAPTICS_RMI4_BUILD_ID_SIZE];
	unsigned int package_id;
	unsigned int package_rev;
	unsigned int pr_number;
	struct list_head support_fn_list;
};

/**
 * struct synaptics_finger - Represents fingers.
 * @ state: finger status.
 * @ mcount: moving counter for debug.
 */
struct synaptics_finger {
	unsigned char state;
	unsigned short mcount;
};

/**
 * synaptics_rmi_f1a_button_map
 * @ nbuttons : number of buttons
 * @ map : key map
 */
struct synaptics_rmi_f1a_button_map {
	unsigned char nbuttons;
	u32 map[4];
};

/**
 * struct synaptics_device_tree_data - power supply information
 * @ coords : horizontal, vertical max width and max hight
 * @ external_ldo : sensor power supply : 3.3V, enbled by GPIO
 * @ sub_pmic : sensor power supply : 3.3V, enabled by subp_mic MAX77826
 * @ irq_gpio : interrupt GPIO PIN defined device tree files(dtsi)
 * @ project : project name string for Firmware name
 * @ sub-project : project name string for Firmware name by sub project
 */

struct synaptics_rmi4_device_tree_data {
	int coords[2];
	int extra_config[4];
	int external_ldo;
#if defined(CONFIG_SEC_RUBENS_PROJECT)
	int external_ldo2;
#endif
	int tkey_led_en;
	int scl_gpio;
	int sda_gpio;
	int irq_gpio;
	int reset_gpio;
	int id_gpio;
	bool tablet;
	char swap_axes;
	char x_flip;
	char y_flip;

	struct synaptics_rmi_f1a_button_map *f1a_button_map;

	const char *project;
	const char *sub_project;

	int num_of_supply;
	const char **name_of_supply;

	bool surface_only;
};

/*
 * struct synaptics_rmi4_data - rmi4 device instance data
 * @i2c_client: pointer to associated i2c client
 * @input_dev: pointer to associated input device
 * @board: constant pointer to platform data
 * @rmi4_mod_info: device information
 * @regulator: pointer to associated regulator
 * @rmi4_io_ctrl_mutex: mutex for i2c i/o control
 * @early_suspend: instance to support early suspend power management
 * @current_page: current page in sensor to acess
 * @button_0d_enabled: flag for 0d button support
 * @full_pm_cycle: flag for full power management cycle in early suspend stage
 * @num_of_intr_regs: number of interrupt registers
 * @f01_query_base_addr: query base address for f01
 * @f01_cmd_base_addr: command base address for f01
 * @f01_ctrl_base_addr: control base address for f01
 * @f01_data_base_addr: data base address for f01
 * @irq: attention interrupt
 * @sensor_max_x: sensor maximum x value
 * @sensor_max_y: sensor maximum y value
 * @irq_enabled: flag for indicating interrupt enable status
 * @touch_stopped: flag to stop interrupt thread processing
 * @fingers_on_2d: flag to indicate presence of fingers in 2d area
 * @sensor_sleep: flag to indicate sleep state of sensor
 * @wait: wait queue for touch data polling in interrupt thread
 * @i2c_read: pointer to i2c read function
 * @i2c_write: pointer to i2c write function
 * @irq_enable: pointer to irq enable function
 */
struct synaptics_rmi4_data {
	struct i2c_client *i2c_client;
	struct input_dev *input_dev;
	struct regulator *vcc_en;
	struct regulator *sub_pmic;
	struct synaptics_rmi4_device_tree_data *dt_data;
	struct synaptics_rmi4_device_info rmi4_mod_info;
	struct regulator_bulk_data *supplies;
#ifdef PROXIMITY
	struct synaptics_rmi4_f51_handle *f51_handle;
#endif

	struct mutex rmi4_device_mutex;
	struct mutex rmi4_reset_mutex;
	struct mutex rmi4_io_ctrl_mutex;
	struct mutex rmi4_reflash_mutex;
	struct timer_list f51_finger_timer;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	const char *firmware_name;

	struct completion init_done;
	struct synaptics_finger finger[MAX_NUMBER_OF_FINGERS];

	unsigned char current_page;
	unsigned char button_0d_enabled;
	unsigned char full_pm_cycle;
	unsigned char num_of_rx;
	unsigned char num_of_tx;
	unsigned int num_of_node;
	unsigned char num_of_fingers;
	unsigned char max_touch_width;
	unsigned char feature_enable;
	unsigned char report_enable;
	unsigned char no_sleep_setting;
	unsigned char intr_mask[MAX_INTR_REGISTERS];
	unsigned char *button_txrx_mapping;
	unsigned char bootloader_id[4];
#ifdef ENABLE_F12_OBJTYPE
	unsigned char obj_type_enable;	/* F12_2D_CTRL23 */
	unsigned short f12_ctrl23_addr;		/* F12_2D_CTRL23 : object report enable */
#endif
	unsigned short num_of_intr_regs;
	unsigned short f01_query_base_addr;
	unsigned short f01_cmd_base_addr;
	unsigned short f01_ctrl_base_addr;
	unsigned short f01_data_base_addr;
	unsigned short f12_ctrl11_addr;		/* for setting jitter level*/
	unsigned short f12_ctrl15_addr;		/* for getting finger amplitude threshold */
	unsigned short f12_ctrl26_addr;
	unsigned short f12_ctrl28_addr;
	unsigned short f34_ctrl_base_addr;
	unsigned short f51_ctrl_base_addr;

	int irq;
	int sensor_max_x;
	int sensor_max_y;
	int touch_threshold;
	int gloved_sensitivity;
	int ta_status;
	bool flash_prog_mode;
	bool irq_enabled;
	bool touch_stopped;
	bool fingers_on_2d;
	bool f51_finger;
	bool f51_finger_is_hover;
	bool hand_edge_down;
	bool has_edge_swipe;
	bool has_glove_mode;
	bool has_side_buttons;
	bool sensor_sleep;
	bool stay_awake;
	bool staying_awake;
	bool tsp_probe;
	bool firmware_cracked;
	bool ta_con_mode;	/* ta_con_mode ? I2C(RMI) : INT(GPIO) */
	bool hover_status_in_normal_mode;
	bool fast_glove_state;
	bool touchkey_glove_mode_status;
	bool created_sec_class;
	bool use_deepsleep;

#ifdef USE_STYLUS
	bool use_stylus;
#endif

	int ic_version;			/* define S5000, S5050, S5700, S5707, S5708 */
	int ic_revision_of_ic;
	int ic_revision_of_bin;		/* revision of reading from binary */
	int fw_version_of_ic;		/* firmware version of IC */
	int fw_version_of_bin;		/* firmware version of binary */
	int fw_release_date_of_ic;	/* Config release data from IC */
	int lcd_id;
	int debug_log;			/* Test log : 1[F51/edge_swipe] 2[F12/abs_report] */
	unsigned int fw_pr_number;

	bool doing_reflash;
	int rebootcount;

#if defined(CONFIG_LEDS_CLASS) && defined(TOUCHKEY_ENABLE)
	struct led_classdev	leds;
#endif

#ifdef TOUCHKEY_ENABLE
	int touchkey_menu;
	int touchkey_back;
	bool touchkey_led;
	bool created_sec_tkey_class;
#endif

#ifdef CONFIG_SEC_TSP_FACTORY
	int bootmode;
#endif

#ifdef TSP_BOOSTER
	struct delayed_work	work_dvfs_off;
	struct delayed_work	work_dvfs_chg;
	struct mutex		dvfs_lock;
	bool dvfs_lock_status;
	int dvfs_old_stauts;
	int dvfs_boost_mode;
	int dvfs_freq;
#endif
#ifdef TKEY_BOOSTER
	struct delayed_work	work_tkey_dvfs_off;
	struct delayed_work	work_tkey_dvfs_chg;
	struct mutex		tkey_dvfs_lock;
	bool tkey_dvfs_lock_status;
	int tkey_dvfs_old_stauts;
	int tkey_dvfs_boost_mode;
	int tkey_dvfs_freq;
#endif

#ifdef COMMON_INPUT_BOOSTER
	struct input_booster *tsp_booster;
#endif
#ifdef USE_HOVER_REZERO
	struct delayed_work rezero_work;
#endif

#ifdef HAND_GRIP_MODE
	unsigned int hand_grip_mode;
	unsigned int hand_grip;
	unsigned int old_hand_grip;
	unsigned int old_code;
#endif

#ifdef SIDE_TOUCH
	bool sidekey_enables;
	unsigned char sidekey_data;
	bool sidekey_test;
	bool sidekey_only_enable;
#endif

#ifdef SYNAPTICS_RMI_INFORM_CHARGER
	void (*register_cb)(struct synaptics_rmi_callbacks *);
	struct synaptics_rmi_callbacks callbacks;
#endif

	int (*i2c_read)(struct synaptics_rmi4_data *pdata, unsigned short addr,
			unsigned char *data, unsigned short length);
	int (*i2c_write)(struct synaptics_rmi4_data *pdata, unsigned short addr,
			unsigned char *data, unsigned short length);
	int (*irq_enable)(struct synaptics_rmi4_data *rmi4_data, bool enable);
	int (*reset_device)(struct synaptics_rmi4_data *rmi4_data);
	int (*stop_device)(struct synaptics_rmi4_data *rmi4_data);
	int (*start_device)(struct synaptics_rmi4_data *rmi4_data);
};

enum exp_fn {
	RMI_DEV = 0,
	RMI_F54,
	RMI_FW_UPDATER,
	RMI_DB,
	RMI_GUEST,
	RMI_LAST,
};

struct synaptics_rmi4_exp_fn {
	enum exp_fn fn_type;
	bool initialized;
	int (*func_init)(struct synaptics_rmi4_data *rmi4_data);
	void (*func_remove)(struct synaptics_rmi4_data *rmi4_data);
	void (*func_attn)(struct synaptics_rmi4_data *rmi4_data,
			unsigned char intr_mask);
	struct list_head link;
};

struct synaptics_rmi4_exp_fn_ptr {
	int (*read)(struct synaptics_rmi4_data *rmi4_data, unsigned short addr,
			unsigned char *data, unsigned short length);
	int (*write)(struct synaptics_rmi4_data *rmi4_data, unsigned short addr,
			unsigned char *data, unsigned short length);
	int (*enable)(struct synaptics_rmi4_data *rmi4_data, bool enable);
};

int synaptics_rmi4_new_function(enum exp_fn fn_type,
		int (*func_init)(struct synaptics_rmi4_data *rmi4_data),
		void (*func_remove)(struct synaptics_rmi4_data *rmi4_data),
		void (*func_attn)(struct synaptics_rmi4_data *rmi4_data,
			unsigned char intr_mask));

int rmidev_module_register(void);
int rmi4_f54_module_register(void);
int rmi4_fw_update_module_register(void);
int rmidb_module_register(void);
int rmi_guest_module_register(void);

int synaptics_rmi4_f54_set_control(struct synaptics_rmi4_data *rmi4_data);

int synaptics_fw_updater(unsigned char *fw_data);
int synaptics_rmi4_fw_update_on_probe(struct synaptics_rmi4_data *rmi4_data);
int synaptics_rmi4_proximity_enables(struct synaptics_rmi4_data *rmi4_data, unsigned char enables);
int synaptics_proximity_no_sleep_set(struct synaptics_rmi4_data *rmi4_data, bool enables);
int synaptics_rmi4_f12_set_feature(struct synaptics_rmi4_data *rmi4_data);
int synaptics_rmi4_set_tsp_test_result_in_config(int pass_fail);
int synaptics_rmi4_tsp_read_test_result(struct synaptics_rmi4_data *rmi4_data);
#ifdef USE_EDGE_EXCLUSION
int synaptics_rmi4_f51_grip_edge_exclusion_rx(struct synaptics_rmi4_data *rmi4_data, bool enables);
#endif
int synaptics_rmi4_f12_ctrl11_set (struct synaptics_rmi4_data *rmi4_data,
		unsigned char data);

int synaptics_rmi4_set_custom_ctrl_register(struct synaptics_rmi4_data *rmi4_data,
					bool mode, unsigned short address,
					int length, unsigned char *value);

void synaptics_rmi4_free_sidekeys(struct synaptics_rmi4_data *rmi4_data);
int synaptics_rmi4_free_fingers(struct synaptics_rmi4_data *rmi4_data);
int synaptics_rmi4_irq_enable(struct synaptics_rmi4_data *rmi4_data, bool enable);

#ifdef TOUCHKEY_ENABLE
int synaptics_tkey_led_vdd_on(struct synaptics_rmi4_data *rmi4_data, bool onoff);
#endif

extern struct class *sec_class;
#define SEC_CLASS_DEVT_TSP	10
#define SEC_CLASS_DEVT_TKEY	11

#ifdef CONFIG_SAMSUNG_LPM_MODE
extern int poweroff_charging;
#endif

static inline ssize_t synaptics_rmi4_show_error(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	dev_warn(dev, "%s Attempted to read from write-only attribute %s\n",
			__func__, attr->attr.name);
	return -EPERM;
}

static inline ssize_t synaptics_rmi4_store_error(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	dev_warn(dev, "%s Attempted to write to read-only attribute %s\n",
			__func__, attr->attr.name);
	return -EPERM;
}

static inline void batohs(unsigned short *dest, unsigned char *src)
{
	*dest = src[1] * 0x100 + src[0];
}

static inline void hstoba(unsigned char *dest, unsigned short src)
{
	dest[0] = src % 0x100;
	dest[1] = src / 0x100;
}
#endif
