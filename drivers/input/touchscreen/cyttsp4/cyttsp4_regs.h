/*
 * cyttsp4_regs.h
 * Cypress TrueTouch(TM) Standard Product V4 registers.
 * For use with Cypress Txx4xx parts.
 * Supported parts include:
 * TMA4XX
 * TMA1036
 *
 * Copyright (C) 2012 Cypress Semiconductor
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * Author: Aleksej Makarov <aleksej.makarov@sonyericsson.com>
 * Modified by: Cypress Semiconductor to add test modes and commands
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contact Cypress Semiconductor at www.cypress.com <ttdrivers@cypress.com>
 *
 */

#ifndef _CYTTSP4_REGS_H
#define _CYTTSP4_REGS_H

#include <asm/unaligned.h>
#include <linux/delay.h>
#include <linux/device.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#define SAMSUNG_TSP_INFO
#define SAMSUNG_SYSFS
#include <linux/cyttsp4_core.h>


/* Timeout in ms. */
#define CY_COMMAND_COMPLETE_TIMEOUT		500
#define CY_CALIBRATE_COMPLETE_TIMEOUT		5000
#define CY_WATCHDOG_TIMEOUT			1000
#define CY_CORE_REQUEST_EXCLUSIVE_TIMEOUT	500
#define CY_PROXIMITY_REQUEST_EXCLUSIVE_TIMEOUT	1000
#define CY_DA_REQUEST_EXCLUSIVE_TIMEOUT		500
#define CY_LDR_REQUEST_EXCLUSIVE_TIMEOUT	5000
#define CY_CORE_SLEEP_REQUEST_EXCLUSIVE_TIMEOUT	5000
#define CY_CORE_WAIT_SYSINFO_MODE_TIMEOUT	2000
#define CY_CORE_MODE_CHANGE_TIMEOUT		1000
#define CY_CORE_RESET_AND_WAIT_TIMEOUT		500
#define CY_CORE_WAKEUP_TIMEOUT			50
#define CY_LDR_CMD_TIMEOUT			500
#define CY_LDR_CMD_INIT_TIMEOUT			10000

/* helpers */
#define GET_NUM_TOUCH_RECORDS(x)	((x) & 0x1F)
#define IS_LARGE_AREA(x)		((x) & 0x20)
#define IS_BAD_PKT(x)			((x) & 0x20)
#define IS_TTSP_VER_GE(p, maj, min) \
		((p)->si_ptrs.cydata == NULL ? \
		0 : \
		((p)->si_ptrs.cydata->ttsp_ver_major < (maj) ? \
			0 : \
			((p)->si_ptrs.cydata->ttsp_ver_minor < (min) ? \
				0 : \
				1)))

#define IS_BOOTLOADER(hst_mode, reset_detect) \
		((hst_mode) & 0x01 || (reset_detect) != 0)
#define IS_BOOTLOADER_IDLE(hst_mode, reset_detect) \
		((hst_mode) & 0x01 && (reset_detect) & 0x01)

#define GET_HSTMODE(reg)		((reg & 0x70) >> 4)
#define GET_TOGGLE(reg)			((reg & 0x80) >> 7)

#define IS_LITTLEENDIAN(reg)		((reg & 0x01) == 1)
#define GET_PANELID(reg)		(reg & 0x07)

#define HI_BYTE(x)			(u8)(((x) >> 8) & 0xFF)
#define LO_BYTE(x)			(u8)((x) & 0xFF)

#define IS_DEEP_SLEEP_CONFIGURED(x)	((x) == 0 || (x) == 0xFF)

#define IS_TMO(t)			((t) == 0)

#define PUT_FIELD16(si, val, addr) \
do { \
	if (IS_LITTLEENDIAN((si)->si_ptrs.cydata->device_info)) \
		put_unaligned_le16(val, addr); \
	else \
		put_unaligned_be16(val, addr); \
} while (0)

#define GET_FIELD16(si, addr) \
({ \
	u16 __val; \
	if (IS_LITTLEENDIAN((si)->si_ptrs.cydata->device_info)) \
		__val = get_unaligned_le16(addr); \
	else \
		__val = get_unaligned_be16(addr); \
	__val; \
})

#define RETRY_OR_EXIT(retry_cnt, retry_label, exit_label) \
do { \
	if (retry_cnt) \
		goto retry_label; \
	goto exit_label; \
} while (0)

/* DEVICE REGISTERS */
/* OP MODE REGISTERS */
#define CY_REG_BASE			0x00

enum cyttsp4_hst_mode_bits {
	CY_HST_TOGGLE      = (1 << 7),
	CY_HST_MODE_CHANGE = (1 << 3),
	CY_HST_DEVICE_MODE = (7 << 4),
	CY_HST_OPERATE     = (0 << 4),
	CY_HST_SYSINFO     = (1 << 4),
	CY_HST_CAT         = (2 << 4),
	CY_HST_LOWPOW      = (1 << 2),
	CY_HST_SLEEP       = (1 << 1),
	CY_HST_RESET       = (1 << 0),
};

/* Touch record registers */
enum cyttsp4_object_id {
	CY_OBJ_STANDARD_FINGER = 0,
	CY_OBJ_PROXIMITY       = (1 << 0),
	CY_OBJ_STYLUS          = (1 << 1),
	CY_OBJ_GLOVE           = (1 << 2),
};

enum cyttsp4_event_id {
	CY_EV_NO_EVENT,
	CY_EV_TOUCHDOWN,
	CY_EV_MOVE,		/* significant displacement (> act dist) */
	CY_EV_LIFTOFF,		/* record reports last position */
};

/* Touch id */
#define CY_TMA1036_MAX_TCH		0x0E
#define CY_TMA4XX_MAX_TCH		0x1E

/* CAT MODE REGISTERS */
#define CY_REG_CAT_CMD			2
#define CY_CMD_COMPLETE_MASK		(1 << 6)
#define CY_CMD_MASK			0x3F

enum cyttsp_cmd_bits {
	CY_CMD_COMPLETE    = (1 << 6),
};

/* SYSINFO REGISTERS */
#define CY_NUM_REVCTRL			8

#define CY_POST_CODEL_WDG_RST           0x01
#define CY_POST_CODEL_CFG_DATA_CRC_FAIL 0x02
#define CY_POST_CODEL_PANEL_TEST_FAIL   0x04

/* touch record system information offset masks and shifts */
#define CY_BYTE_OFS_MASK		0x1F
#define CY_BOFS_MASK			0xE0
#define CY_BOFS_SHIFT			5

/* x-axis resolution of panel in pixels */
#define CY_PCFG_RESOLUTION_X_MASK 0x7F

/* y-axis resolution of panel in pixels */
#define CY_PCFG_RESOLUTION_Y_MASK 0x7F

/* x-axis, 0:origin is on left side of panel, 1: right */
#define CY_PCFG_ORIGIN_X_MASK 0x80

/* y-axis, 0:origin is on top side of panel, 1: bottom */
#define CY_PCFG_ORIGIN_Y_MASK 0x80

#define CY_NORMAL_ORIGIN		0	/* upper, left corner */
#define CY_INVERT_ORIGIN		1	/* lower, right corner */

/* TTSP System Information interface definitions */
struct cyttsp4_cydata {
	u8 ttpidh;
	u8 ttpidl;
	u8 fw_ver_major;
	u8 fw_ver_minor;
	u8 revctrl[CY_NUM_REVCTRL];
	u8 blver_major;
	u8 blver_minor;
	u8 jtag_si_id3;
	u8 jtag_si_id2;
	u8 jtag_si_id1;
	u8 jtag_si_id0;
	u8 mfgid_sz;
	u8 cyito_idh;
	u8 cyito_idl;
	u8 cyito_verh;
	u8 cyito_verl;
	u8 ttsp_ver_major;
	u8 ttsp_ver_minor;
	u8 device_info;
	u8 mfg_id[];
} __packed;

struct cyttsp4_test {
	u8 post_codeh;
	u8 post_codel;
} __packed;

struct cyttsp4_pcfg {
	u8 electrodes_x;
	u8 electrodes_y;
	u8 len_xh;
	u8 len_xl;
	u8 len_yh;
	u8 len_yl;
	u8 res_xh;
	u8 res_xl;
	u8 res_yh;
	u8 res_yl;
	u8 max_zh;
	u8 max_zl;
	u8 panel_info0;
} __packed;

#define CY_NUM_TCH_FIELDS		7
#define CY_NUM_EXT_TCH_FIELDS		3

struct cyttsp4_tch_rec_params {
	u8 loc;
	u8 size;
} __packed;

struct cyttsp4_opcfg {
	u8 cmd_ofs;
	u8 rep_ofs;
	u8 rep_szh;
	u8 rep_szl;
	u8 num_btns;
	u8 tt_stat_ofs;
	u8 obj_cfg0;
	u8 max_tchs;
	u8 tch_rec_size;
	struct cyttsp4_tch_rec_params tch_rec_old[CY_NUM_TCH_FIELDS];
	u8 btn_rec_size;/* btn record size (in bytes) */
	u8 btn_diff_ofs;/* btn data loc ,diff counts, (Op-Mode byte ofs) */
	u8 btn_diff_size;/* btn size of diff counts (in bits) */
	struct cyttsp4_tch_rec_params tch_rec_new[CY_NUM_EXT_TCH_FIELDS];
	u8 noise_data_ofs;
	u8 noise_data_sz;
} __packed;

struct cyttsp4_sysinfo_data {
	u8 hst_mode;
	u8 reserved;
	u8 map_szh;
	u8 map_szl;
	u8 cydata_ofsh;
	u8 cydata_ofsl;
	u8 test_ofsh;
	u8 test_ofsl;
	u8 pcfg_ofsh;
	u8 pcfg_ofsl;
	u8 opcfg_ofsh;
	u8 opcfg_ofsl;
	u8 ddata_ofsh;
	u8 ddata_ofsl;
	u8 mdata_ofsh;
	u8 mdata_ofsl;
} __packed;

/* FLASH BLOCKS */
enum cyttsp4_ic_ebid {
	CY_TCH_PARM_EBID,
	CY_MDATA_EBID,
	CY_DDATA_EBID,
	CY_EBID_NUM,
};

/* ttconfig block */
#define CY_TTCONFIG_VERSION_OFFSET	8
#define CY_TTCONFIG_VERSION_SIZE	2
#define CY_TTCONFIG_VERSION_ROW		0

#define CY_CONFIG_LENGTH_INFO_OFFSET	0
#define CY_CONFIG_LENGTH_INFO_SIZE	4
#define CY_CONFIG_LENGTH_OFFSET		0
#define CY_CONFIG_LENGTH_SIZE		2
#define CY_CONFIG_MAXLENGTH_OFFSET	2
#define CY_CONFIG_MAXLENGTH_SIZE	2

/* DEBUG */
/* drv_debug commands */
#define CY_DBG_SUSPEND			4
#define CY_DBG_RESUME			5
#define CY_DBG_SOFT_RESET		97
#define CY_DBG_RESET			98

/* Debug buffer */
#define CY_MAX_PRBUF_SIZE		PIPE_BUF
#define CY_PR_TRUNCATED			" truncated..."

/* CMD */
enum cyttsp4_cmd_cat {
	CY_CMD_CAT_NULL,
	CY_CMD_CAT_RESERVED_1,
	CY_CMD_CAT_GET_CFG_ROW_SZ,
	CY_CMD_CAT_READ_CFG_BLK,
	CY_CMD_CAT_WRITE_CFG_BLK,
	CY_CMD_CAT_RESERVED_2,
	CY_CMD_CAT_LOAD_SELF_TEST_DATA,
	CY_CMD_CAT_RUN_SELF_TEST,
	CY_CMD_CAT_GET_SELF_TEST_RESULT,
	CY_CMD_CAT_CALIBRATE_IDACS,
	CY_CMD_CAT_INIT_BASELINES,
	CY_CMD_CAT_EXEC_PANEL_SCAN,
	CY_CMD_CAT_RETRIEVE_PANEL_SCAN,
	CY_CMD_CAT_START_SENSOR_DATA_MODE,
	CY_CMD_CAT_STOP_SENSOR_DATA_MODE,
	CY_CMD_CAT_INT_PIN_MODE,
	CY_CMD_CAT_RETRIEVE_DATA_STRUCTURE,
	CY_CMD_CAT_VERIFY_CFG_BLK_CRC,
	CY_CMD_CAT_RESERVED_N,
};

enum cyttsp4_cmd_op {
	CY_CMD_OP_NULL,
	CY_CMD_OP_RESERVED_1,
	CY_CMD_OP_GET_PARAM,
	CY_CMD_OP_SET_PARAM,
	CY_CMD_OP_RESERVED_2,
	CY_CMD_OP_GET_CRC,
	CY_CMD_OP_WAIT_FOR_EVENT,
};

enum cyttsp4_cmd_status {
	CY_CMD_STATUS_SUCCESS,
	CY_CMD_STATUS_FAILURE,
};

/* Operational Mode Command Sizes */
/* NULL Command */
#define CY_CMD_OP_NULL_CMD_SZ			1
#define CY_CMD_OP_NULL_RET_SZ			0
/* Get Parameter */
#define CY_CMD_OP_GET_PARAM_CMD_SZ		2
#define CY_CMD_OP_GET_PARAM_RET_SZ		6
/* Set Parameter */
#define CY_CMD_OP_SET_PARAM_CMD_SZ		7
#define CY_CMD_OP_SET_PARAM_RET_SZ		2
/* Get Config Block CRC */
#define CY_CMD_OP_GET_CFG_BLK_CRC_CMD_SZ	2
#define CY_CMD_OP_GET_CFG_BLK_CRC_RET_SZ	3
/* Wait For Event */
#define CY_CMD_OP_WAIT_FOR_EVENT_CMD_SZ		2

/* CaT Mode Command Sizes */
/* NULL Command */
#define CY_CMD_CAT_NULL_CMD_SZ			1
#define CY_CMD_CAT_NULL_RET_SZ			0
/* Get Config Row Size */
#define CY_CMD_CAT_GET_CFG_ROW_SIZE_CMD_SZ	1
#define CY_CMD_CAT_GET_CFG_ROW_SIZE_RET_SZ	2
/* Read Config Block */
#define CY_CMD_CAT_READ_CFG_BLK_CMD_SZ		6
#define CY_CMD_CAT_READ_CFG_BLK_RET_SZ		7 /* + Data */
#define CY_CMD_CAT_READ_CFG_BLK_RET_HDR_SZ	5
/* Write Config Block */
#define CY_CMD_CAT_WRITE_CFG_BLK_CMD_SZ		8 /* + Data + Security Key */
#define CY_CMD_CAT_WRITE_CFG_BLK_RET_SZ		5
#define CY_CMD_CAT_WRITE_CFG_BLK_CMD_HDR_SZ	6
/* Load Self-Test Data */
#define CY_CMD_CAT_LOAD_SELFTEST_DATA_CMD_SZ	6
#define CY_CMD_CAT_LOAD_SELFTEST_DATA_RET_SZ	5 /* + Data */
/* Run Self-Test */
#define CY_CMD_CAT_RUN_SELFTEST_CMD_SZ		2
#define CY_CMD_CAT_RUN_SELFTEST_RET_SZ		3
/* Calibrate IDACs */
#define CY_CMD_CAT_CALIBRATE_IDAC_CMD_SZ	2
#define CY_CMD_CAT_CALIBRATE_IDAC_RET_SZ	1
/* Get Self-Test Results */
#define CY_CMD_CAT_GET_SELFTEST_RES_CMD_SZ	6
#define CY_CMD_CAT_GET_SELFTEST_RES_RET_SZ	5 /* + Data */
/* Initialize Baselines */
#define CY_CMD_CAT_INIT_BASELINE_CMD_SZ		2
#define CY_CMD_CAT_INIT_BASELINE_RET_SZ		1
/* Execute Panel Scan */
#define CY_CMD_CAT_EXECUTE_PANEL_SCAN_CMD_SZ	1
#define CY_CMD_CAT_EXECUTE_PANEL_SCAN_RET_SZ	1
/* Retrieve Panel Scan */
#define CY_CMD_CAT_RETRIEVE_PANEL_SCAN_CMD_SZ	6
#define CY_CMD_CAT_RETRIEVE_PANEL_SCAN_RET_SZ	5 /* + Data */
/* Start Sensor Data Mode */
#define CY_CMD_CAT_START_SENSOR_MODE_CMD_SZ	1 /* + Data */
#define CY_CMD_CAT_START_SENSOR_MODE_RET_SZ	0 /* + Data */
/* Stop Sensor Data Mode */
#define CY_CMD_CAT_STOP_SENSOR_MODE_CMD_SZ	1
#define CY_CMD_CAT_STOP_SENSOR_MODE_RET_SZ	0
/* Interrupt Pin Override */
#define CY_CMD_CAT_INT_PIN_OVERRIDE_CMD_SZ	2
#define CY_CMD_CAT_INT_PIN_OVERRIDE_RET_SZ	1
/* Retrieve Data Structure */
#define CY_CMD_CAT_RETRIEVE_DATA_STRUCT_CMD_SZ	6
#define CY_CMD_CAT_RETRIEVE_DATA_STRUCT_RET_SZ	5 /* + Data */
/* Verify Config Block CRC */
#define CY_CMD_CAT_VERIFY_CFG_BLK_CRC_CMD_SZ	2
#define CY_CMD_CAT_VERIFY_CFG_BLK_CRC_RET_SZ	5

/* RAM ID */
#define CY_RAM_ID_ACTIVE_DISTANCE		0x4A
#define CY_RAM_ID_SCAN_TYPE			0x4B
#define CY_RAM_ID_LOW_POWER_INTERVAL		0x4C
#define CY_RAM_ID_REFRESH_INTERVAL		0x4D
#define CY_RAM_ID_ACTIVE_MODE_TIMEOUT		0x4E
#define CY_RAM_ID_ACTIVE_LFT_INTERVAL		0x4F
#define CY_RAM_ID_ACTIVE_DISTANCE2		0x50
#define CY_RAM_ID_CHARGER_STATUS		0x51
#define CY_RAM_ID_IMO_TRIM_VALUE		0x52
#define CY_RAM_ID_FINGER_THRESHOLH		0x93
#define CY_RAM_ID_DETECT_AREA_MARGIN_X		0x58
#define CY_RAM_ID_DETECT_AREA_MARGIN_Y		0x59
#define CY_RAM_ID_GRIP_XEDGE_A			0x70
#define CY_RAM_ID_GRIP_XEDGE_B			0x71
#define CY_RAM_ID_GRIP_XEXC_A			0x72
#define CY_RAM_ID_GRIP_XEXC_B			0x73
#define CY_RAM_ID_GRIP_YEDGE_A			0x74
#define CY_RAM_ID_GRIP_YEDGE_B			0x75
#define CY_RAM_ID_GRIP_YEXC_A			0x76
#define CY_RAM_ID_GRIP_YEXC_B			0x77
#define CY_RAM_ID_GRIP_FIRST_EXC		0x78
#define CY_RAM_ID_GRIP_EXC_EDGE_ORIGIN		0x79
#define CY_RAM_ID_PROX_ACTIVE_DIST_Z_VALUE	0x9B
#define CY_RAM_ID_BTN_THRSH_MUT			0xA0
#define CY_RAM_ID_TOUCHMODE_ENABLED		0xD0 /* Enable proximity */

#ifdef CONFIG_SEC_DVFS
#define TSP_BOOSTER
#else
#undef TSP_BOOSTER
#endif
/* TOUCH PARSE */
/* abs signal capabilities offsets in the frameworks array */
enum cyttsp4_sig_caps {
	CY_SIGNAL_OST,
	CY_MIN_OST,
	CY_MAX_OST,
	CY_FUZZ_OST,
	CY_FLAT_OST,
	CY_NUM_ABS_SET	/* number of signal capability fields */
};

/* abs axis signal offsets in the framworks array  */
enum cyttsp4_sig_ost {
	CY_ABS_X_OST,
	CY_ABS_Y_OST,
	CY_ABS_P_OST,
	CY_ABS_W_OST,
	CY_ABS_ID_OST,
	CY_ABS_MAJ_OST,
	CY_ABS_MIN_OST,
	CY_ABS_OR_OST,
	CY_NUM_ABS_OST	/* number of abs signals */
};

enum cyttsp4_tch_abs {	/* for ordering within the extracted touch data array */
	CY_TCH_X,	/* X */
	CY_TCH_Y,	/* Y */
	CY_TCH_P,	/* P (Z) */
	CY_TCH_T,	/* TOUCH ID */
	CY_TCH_E,	/* EVENT ID */
	CY_TCH_O,	/* OBJECT ID */
	CY_TCH_W,	/* SIZE */
	CY_TCH_MAJ,	/* TOUCH_MAJOR */
	CY_TCH_MIN,	/* TOUCH_MINOR */
	CY_TCH_OR,	/* ORIENTATION */
	CY_TCH_NUM_ABS
};

static const char * const cyttsp4_tch_abs_string[] = {
	[CY_TCH_X]	= "X",
	[CY_TCH_Y]	= "Y",
	[CY_TCH_P]	= "P",
	[CY_TCH_T]	= "T",
	[CY_TCH_E]	= "E",
	[CY_TCH_O]	= "O",
	[CY_TCH_W]	= "W",
	[CY_TCH_MAJ]	= "MAJ",
	[CY_TCH_MIN]	= "MIN",
	[CY_TCH_OR]	= "OR",
	[CY_TCH_NUM_ABS] = "INVALID"
};

/* scan_type ram id, scan values */
#define CY_SCAN_TYPE_GLOVE		0x8
#define CY_SCAN_TYPE_STYLUS		0x10
#define CY_SCAN_TYPE_PROXIMITY		0x40
#define CY_SCAN_TYPE_APA_MC		0x80

enum cyttsp4_scan_type {
	CY_ST_APA_MC,
	CY_ST_GLOVE,
	CY_ST_STYLUS,
	CY_ST_PROXIMITY,
};


/* DRIVER STATES */
enum cyttsp4_mode {
	CY_MODE_UNKNOWN      = 0,
	CY_MODE_BOOTLOADER   = (1 << 0),
	CY_MODE_OPERATIONAL  = (1 << 1),
	CY_MODE_SYSINFO      = (1 << 2),
	CY_MODE_CAT          = (1 << 3),
	CY_MODE_STARTUP      = (1 << 4),
	CY_MODE_LOADER       = (1 << 5),
	CY_MODE_CHANGE_MODE  = (1 << 6),
	CY_MODE_CHANGED      = (1 << 7),
	CY_MODE_CMD_COMPLETE = (1 << 8),
};

enum cyttsp4_int_state {
	CY_INT_NONE,
	CY_INT_IGNORE      = (1 << 0),
	CY_INT_MODE_CHANGE = (1 << 1),
	CY_INT_EXEC_CMD    = (1 << 2),
	CY_INT_AWAKE       = (1 << 3),
};

enum cyttsp4_sleep_state {
	SS_SLEEP_OFF,
	SS_SLEEP_ON,
	SS_SLEEPING,
	SS_WAKING,
};

enum cyttsp4_startup_state {
	STARTUP_NONE,
	STARTUP_QUEUED,
	STARTUP_RUNNING,
};

enum cyttsp4_atten_type {
	CY_ATTEN_IRQ,
	CY_ATTEN_STARTUP,
	CY_ATTEN_EXCLUSIVE,
	CY_ATTEN_WAKE,
	CY_ATTEN_LOADER,
	CY_ATTEN_NUM_ATTEN,
};

enum cyttsp4_module_id {
	CY_MODULE_MT,
	CY_MODULE_BTN,
	CY_MODULE_PROX,
	CY_MODULE_DEBUG,
	CY_MODULE_LOADER,
	CY_MODULE_DEVICE_ACCESS,
	CY_MODULE_LAST,
};

struct cyttsp4_sysinfo_ptr {
	struct cyttsp4_cydata *cydata;
	struct cyttsp4_test *test;
	struct cyttsp4_pcfg *pcfg;
	struct cyttsp4_opcfg *opcfg;
	struct cyttsp4_ddata *ddata;
	struct cyttsp4_mdata *mdata;
};

struct cyttsp4_touch {
	int abs[CY_TCH_NUM_ABS];
};

struct cyttsp4_tch_abs_params {
	size_t ofs;	/* abs byte offset */
	size_t size;	/* size in bits */
	size_t max;	/* max value */
	size_t bofs;	/* bit offset */
};

struct cyttsp4_sysinfo_ofs {
	size_t chip_type;
	size_t cmd_ofs;
	size_t rep_ofs;
	size_t rep_sz;
	size_t num_btns;
	size_t num_btn_regs;	/* ceil(num_btns/4) */
	size_t tt_stat_ofs;
	size_t tch_rec_size;
	size_t obj_cfg0;
	size_t max_tchs;
	size_t mode_size;
	size_t data_size;
	size_t rep_hdr_size;
	size_t map_sz;
	size_t max_x;
	size_t x_origin;	/* left or right corner */
	size_t max_y;
	size_t y_origin;	/* upper or lower corner */
	size_t max_p;
	size_t cydata_ofs;
	size_t test_ofs;
	size_t pcfg_ofs;
	size_t opcfg_ofs;
	size_t ddata_ofs;
	size_t mdata_ofs;
	size_t cydata_size;
	size_t test_size;
	size_t pcfg_size;
	size_t opcfg_size;
	size_t ddata_size;
	size_t mdata_size;
	size_t btn_keys_size;
	struct cyttsp4_tch_abs_params tch_abs[CY_TCH_NUM_ABS];
	size_t btn_rec_size; /* btn record size (in bytes) */
	size_t btn_diff_ofs;/* btn data loc ,diff counts, (Op-Mode byte ofs) */
	size_t btn_diff_size;/* btn size of diff counts (in bits) */
	size_t noise_data_ofs;
	size_t noise_data_sz;
};

/* button to keycode support */
#define CY_NUM_BTN_PER_REG		4
#define CY_BITS_PER_BTN			2

enum cyttsp4_btn_state {
	CY_BTN_RELEASED,
	CY_BTN_PRESSED,
	CY_BTN_NUM_STATE
};

struct cyttsp4_btn {
	bool enabled;
	int state;	/* CY_BTN_PRESSED, CY_BTN_RELEASED */
	int key_code;
};

struct cyttsp4_ttconfig {
	u16 version;
	u16 length;
	u16 max_length;
	u16 crc;
};


#ifdef SAMSUNG_TSP_INFO
struct cyttsp4_samsung_tsp_info {
	u8 ic_vendorh;
	u8 ic_vendorl;
	u8 module_vendorh;
	u8 module_vendorl;
	u8 hw_version;
	u8 fw_versionh;
	u8 fw_versionl;
	u8 config_version;
	u8 ic_series;
	u8 num_sensor_x;
	u8 num_sensor_y;
	u8 resolution_xh;
	u8 resolution_xl;
	u8 resolution_yh;
	u8 resolution_yl;
	u8 button_info;
	u8 set_comb_info;
} __packed;
#endif//SAMSUNG_TSP_INFO

struct cyttsp4_sysinfo {
	bool ready;
	struct cyttsp4_sysinfo_data si_data;
	struct cyttsp4_sysinfo_ptr si_ptrs;
	struct cyttsp4_sysinfo_ofs si_ofs;
	struct cyttsp4_ttconfig ttconfig;
	struct cyttsp4_btn *btn;	/* button states */
	u8 *btn_rec_data;		/* button diff count data */
	u8 *xy_mode;			/* operational mode and status regs */
	u8 *xy_data;			/* operational touch regs */
#ifdef SAMSUNG_TSP_INFO
	struct cyttsp4_samsung_tsp_info *sti;
#endif
};

/* device_access */
enum cyttsp4_ic_grpnum {
	CY_IC_GRPNUM_RESERVED,
	CY_IC_GRPNUM_CMD_REGS,
	CY_IC_GRPNUM_TCH_REP,
	CY_IC_GRPNUM_DATA_REC,
	CY_IC_GRPNUM_TEST_REC,
	CY_IC_GRPNUM_PCFG_REC,
	CY_IC_GRPNUM_TCH_PARM_VAL,
	CY_IC_GRPNUM_TCH_PARM_SIZE,
	CY_IC_GRPNUM_RESERVED1,
	CY_IC_GRPNUM_RESERVED2,
	CY_IC_GRPNUM_OPCFG_REC,
	CY_IC_GRPNUM_DDATA_REC,
	CY_IC_GRPNUM_MDATA_REC,
	CY_IC_GRPNUM_TEST_REGS,
	CY_IC_GRPNUM_BTN_KEYS,
	CY_IC_GRPNUM_TTHE_REGS,
	CY_IC_GRPNUM_NUM
};

/* test mode NULL command driver codes */
enum cyttsp4_null_test_cmd_code {
	CY_NULL_CMD_NULL,
	CY_NULL_CMD_MODE,
	CY_NULL_CMD_STATUS_SIZE,
	CY_NULL_CMD_HANDSHAKE,
	CY_NULL_CMD_LOW_POWER,
};

enum cyttsp4_test_mode {
	CY_TEST_MODE_NORMAL_OP,		/* Send touch data to OS; normal op */
	CY_TEST_MODE_CAT,		/* Configuration and Test */
	CY_TEST_MODE_SYSINFO,		/* System information mode */
	CY_TEST_MODE_CLOSED_UNIT,	/* Send scan data to sysfs */
};

struct cyttsp4_test_mode_params {
	int cur_mode;
	int cur_cmd;
	size_t cur_status_size;
};

/* FW file name */
#define CY_FW_FILE_NAME			"cyttsp4_fw.bin"

/* Communication bus values */
#define CY_DEFAULT_ADAP_MAX_XFER	256
#define CY_ADAP_MIN_XFER		140

/* Core module */
#define CY_DEFAULT_CORE_ID		"cyttsp4_core0"
#define CY_MAX_NUM_CORE_DEVS		5

struct cyttsp4_mt_data;
struct cyttsp4_mt_function {
	int (*mt_release)(struct device *dev);
	int (*mt_probe)(struct device *dev, struct cyttsp4_mt_data *md);
	void (*report_slot_liftoff)(struct cyttsp4_mt_data *md, int max_slots);
	void (*input_sync)(struct input_dev *input);
	void (*input_report)(struct input_dev *input, int sig, int t, int type);
	void (*final_sync)(struct input_dev *input, int max_slots,
			int mt_sync_count, unsigned long *ids);
	int (*input_register_device)(struct input_dev *input, int max_slots);
};

struct cyttsp4_mt_data {
	struct device *dev;
	struct cyttsp4_mt_platform_data *pdata;
	struct cyttsp4_sysinfo *si;
	struct input_dev *input;
	struct cyttsp4_mt_function mt_function;
	struct mutex mt_lock;
#if defined(TSP_BOOSTER)
		u8 touch_pressed_num;
		struct delayed_work work_dvfs_off;
		struct delayed_work work_dvfs_chg;
		bool	dvfs_lock_status;
		struct mutex dvfs_lock;
		int dvfs_old_status;
		unsigned char boost_level;
		int dvfs_freq;
#endif
	bool input_device_registered;
	char phys[NAME_MAX];
	int num_prv_rec; /* Number of previous touch records */
	int prv_tch_type;
};

struct cyttsp4_btn_data {
	struct device *dev;
	struct cyttsp4_btn_platform_data *pdata;
	struct cyttsp4_sysinfo *si;
	struct input_dev *input;
	struct mutex btn_lock;
	bool is_suspended;
	bool input_device_registered;
	char phys[NAME_MAX];
};

struct cyttsp4_proximity_data {
	struct device *dev;
	struct cyttsp4_proximity_platform_data *pdata;
	struct cyttsp4_sysinfo *si;
	struct input_dev *input;
	struct mutex prox_lock;
	struct mutex sysfs_lock;
	int enable_count;
	bool input_device_registered;
	char phys[NAME_MAX];
};

typedef int (*cyttsp4_atten_func) (struct device *);

struct cyttsp4_core_commands {
	int (*subscribe_attention)(struct device *dev,
		enum cyttsp4_atten_type type, char id, cyttsp4_atten_func func,
		int flags);
	int (*unsubscribe_attention)(struct device *dev,
		enum cyttsp4_atten_type type, char id, cyttsp4_atten_func func,
		int flags);
	int (*request_exclusive)(struct device *dev, int timeout_ms);
	int (*release_exclusive)(struct device *dev);
	int (*request_reset)(struct device *dev);
	int (*request_restart)(struct device *dev, bool wait);
	int (*request_set_mode)(struct device *dev, int mode);
	struct cyttsp4_sysinfo * (*request_sysinfo)(struct device *dev);
	struct cyttsp4_loader_platform_data
		*(*request_loader_pdata)(struct device *dev);
	int (*request_handshake)(struct device *dev, u8 mode);
	int (*request_exec_cmd)(struct device *dev, u8 mode, u8 *cmd_buf,
		size_t cmd_size, u8 *return_buf, size_t return_buf_size,
		int timeout_ms);
	int (*request_stop_wd)(struct device *dev);
	int (*request_toggle_lowpower)(struct device *dev, u8 mode);
	int (*request_config_row_size)(struct device *dev,
		u16 *config_row_size);
	int (*request_write_config)(struct device *dev, u8 ebid, u16 offset,
		u8 *data, u16 length);
	int (*request_enable_scan_type)(struct device *dev, u8 scan_type);
	int (*request_disable_scan_type)(struct device *dev, u8 scan_type);
	const u8 * (*get_security_key)(struct device *dev, int *size);
	void (*get_touch_record)(struct device *dev, int rec_no, int *rec_abs);
	int (*write)(struct device *dev, int mode, u16 addr, const void *buf,
			int size);
	int (*read)(struct device *dev, int mode, u16 addr, void *buf,
			int size);

	struct cyttsp4_sysinfo * (*update_sysinfo)(struct device *dev);
	int (*exec_panel_scan)(struct device *dev);
	int (*retrieve_panel_scan)(struct device *dev, int read_offset,
		int num_element, u8 data_type, u8 *return_buf);
	int (*scan_and_retrieve)(struct device *dev, bool switch_to_cat, bool scan_start, int read_offset,
		int num_element, u8 data_type, u8 *big_buf, int *r_read_element_offset, u8 *element_size);
	int (*retrieve_data_structure)(struct device *dev, int read_offset,
		int num_element, u8 data_id, u8 *big_buf);
};

#define CY_CMD_IN_DATA_OFFSET_VALUE 0

#define CY_CMD_OUT_STATUS_OFFSET 0
#define CY_CMD_RET_PNL_OUT_ELMNT_SZ_OFFS_H 2
#define CY_CMD_RET_PNL_OUT_ELMNT_SZ_OFFS_L 3
#define CY_CMD_RET_PNL_OUT_DATA_FORMAT_OFFS 4
#define CY_CMD_RET_PNL_OUT_DATA_OFFS 8

#define CY_CMD_RET_PANEL_ELMNT_SZ_MASK 0x07

enum cyttsp4_scan_data_type {
	CY_SDT_MUT_RAW = 0x00,
	CY_SDT_MUT_BASE,
	CY_SDT_MUT_DIFF,
	CY_SDT_SELF_RAW,
	CY_SDT_SELF_BASE,
	CY_SDT_SELF_DIFF,
	CY_SDT_BTN = 0x09
};

#ifdef SAMSUNG_SYSFS
#define FACTORY_CMD_STR_LEN 32
#define FACTORY_CMD_RESULT_STR_LEN 128
#define FACTORY_CMD_PARAM_NUM 8

struct cyttsp4_samsung_sysfs_data {
	struct device *dev;
	struct device *dev_factory;
	struct device *dev_screen;
	struct device *dev_key;
	
	struct cyttsp4_core_commands* corecmd;
	struct cyttsp4_sysinfo *si;
	struct list_head factory_cmd_list_head;

	struct mutex factory_cmd_lock;
	int factory_cmd_state;
	int factory_cmd_param[FACTORY_CMD_PARAM_NUM];
	char factory_cmd[FACTORY_CMD_STR_LEN];
	char factory_cmd_result[FACTORY_CMD_RESULT_STR_LEN];
	bool factory_cmd_is_running;
	bool sysfs_nodes_created;

	u8 *screen_buf;// raw, diff, global, local
	u8 *btn_buf;// raw, diff, global, local
	u8 raw_diff_element_size;

	int num_all_nodes;
	int num_btns;
};
#endif//SAMSUNG_SYSFS


struct cyttsp4_core_data {
	struct list_head node;
	char core_id[20];
	struct device *dev;
	struct list_head atten_list[CY_ATTEN_NUM_ATTEN];
	struct mutex system_lock;
	struct mutex adap_lock;
	enum cyttsp4_mode mode;
	enum cyttsp4_sleep_state sleep_state;
	enum cyttsp4_startup_state startup_state;
	int int_status;
	int cmd_toggle;
	spinlock_t spinlock;
	struct cyttsp4_mt_data md;
	struct cyttsp4_btn_data bd;
	struct cyttsp4_proximity_data pd;
#ifdef SAMSUNG_SYSFS
	struct cyttsp4_samsung_sysfs_data ssd;
#endif
	int phys_num;
	int number_of_open_input_device;
	int pm_runtime_usage_count;
	void *cyttsp4_dynamic_data[CY_MODULE_LAST];
	struct cyttsp4_platform_data *pdata;
	struct cyttsp4_core_platform_data *cpdata;
	const struct cyttsp4_bus_ops *bus_ops;
	wait_queue_head_t wait_q;
	int irq;
	struct work_struct startup_work;
	struct cyttsp4_sysinfo sysinfo;
	void *exclusive_dev;
	int exclusive_waits;
	atomic_t ignore_irq;
	bool irq_enabled;
	bool irq_wake;
	bool irq_disabled;
	bool wake_initiated_by_device;
	bool bl_fast_exit;
	bool invalid_touch_app;
	int max_xfer;
	int apa_mc_en;
	int glove_en;
	int stylus_en;
	int proximity_en;
	u8 default_scantype;
	u8 easy_wakeup_gesture;
	unsigned int active_refresh_cycle_ms;
	u8 heartbeat_count;
	struct work_struct watchdog_work;
	struct timer_list watchdog_timer;
	u8 wr_buf[CY_DEFAULT_ADAP_MAX_XFER];
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend es;
#endif
#ifdef VERBOSE_DEBUG
	u8 pr_buf[CY_MAX_PRBUF_SIZE];
#endif
};

struct cyttsp4_bus_ops {
	int (*write)(struct device *dev, u16 addr, u8 *wr_buf,
		const void *buf, int size, int max_xfer);
	int (*read)(struct device *dev, u16 addr, void *buf,
		int size, int max_xfer);
};

static inline void *cyttsp4_get_dynamic_data(struct device *dev, int id)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	return cd->cyttsp4_dynamic_data[id];
}

void cyttsp4_get_touch_record_(struct device *dev, int rec_no, int *rec_abs);
int cyttsp4_read_(struct device *dev, int mode, u16 addr, void *buf, int size);
int cyttsp4_write_(struct device *dev, int mode, u16 addr, const void *buf,
	int size);
int request_exclusive(struct cyttsp4_core_data *cd, void *ownptr,
		int timeout_ms);
int release_exclusive(struct cyttsp4_core_data *cd, void *ownptr);

static inline void cyttsp4_get_touch_record(struct device *dev, int rec_no,
		int *rec_abs)
{
	cyttsp4_get_touch_record_(dev, rec_no, rec_abs);
}

static inline int cyttsp4_read(struct device *dev, int mode, u16 addr,
	void *buf, int size)
{
	return cyttsp4_read_(dev, mode, addr, buf, size);
}

static inline int cyttsp4_write(struct device *dev, int mode, u16 addr,
	const void *buf, int size)
{
	return cyttsp4_write_(dev, mode, addr, buf, size);
}

static inline int cyttsp4_request_exclusive(struct device *dev, int timeout_ms)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	return request_exclusive(cd, dev, timeout_ms);
}

static inline int cyttsp4_release_exclusive(struct device *dev)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	return release_exclusive(cd, dev);
}

#ifdef VERBOSE_DEBUG
extern void cyttsp4_pr_buf(struct device *dev, u8 *pr_buf, u8 *dptr, int size,
			   const char *data_name);
#else
#define cyttsp4_pr_buf(a, b, c, d, e) do { } while (0)
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICETREE_SUPPORT
int cyttsp4_devtree_create_and_get_pdata(struct device *adap_dev);
int cyttsp4_devtree_clean_pdata(struct device *adap_dev);
#else
static inline int cyttsp4_devtree_create_and_get_pdata(struct device *adap_dev)
{
	return 0;
}

static inline int cyttsp4_devtree_clean_pdata(struct device *adap_dev)
{
	return 0;
}
#endif
int cyttsp4_probe(const struct cyttsp4_bus_ops *ops, struct device *dev,
		u16 irq, size_t xfer_buf_size);
int cyttsp4_release(struct cyttsp4_core_data *cd);

struct cyttsp4_core_commands *cyttsp4_get_commands(void);
struct cyttsp4_core_data *cyttsp4_get_core_data(char *id);

int cyttsp4_mt_probe(struct device *dev);
int cyttsp4_mt_release(struct device *dev);

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_BUTTON
int cyttsp4_btn_probe(struct device *dev);
int cyttsp4_btn_release(struct device *dev);
#else
static inline int cyttsp4_btn_probe(struct device *dev) { return 0; }
static inline int cyttsp4_btn_release(struct device *dev) { return 0; }
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_PROXIMITY
int cyttsp4_proximity_probe(struct device *dev);
int cyttsp4_proximity_release(struct device *dev);
#else
static inline int cyttsp4_proximity_probe(struct device *dev) { return 0; }
static inline int cyttsp4_proximity_release(struct device *dev) { return 0; }
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEBUG
int cyttsp4_debug_probe(struct device *dev);
int cyttsp4_debug_release(struct device *dev);
#else
static inline int cyttsp4_debug_probe(struct device *dev) { return 0; }
static inline int cyttsp4_debug_release(struct device *dev) { return 0; }
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICE_ACCESS
int cyttsp4_device_access_probe(struct device *dev);
int cyttsp4_device_access_release(struct device *dev);
#else
static inline int cyttsp4_device_access_probe(struct device *dev) { return 0; }
static inline int cyttsp4_device_access_release(struct device *dev) { return 0; }
#endif

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_LOADER
int cyttsp4_loader_probe(struct device *dev);
int cyttsp4_loader_release(struct device *dev);
#else
static inline int cyttsp4_loader_probe(struct device *dev) { return 0; }
static inline int cyttsp4_loader_release(struct device *dev) { return 0; }
#endif

#ifdef SAMSUNG_SYSFS
int cyttsp4_samsung_sysfs_probe(struct device *dev);
int cyttsp4_samsung_sysfs_release(struct device *dev);
#else
static inline int cyttsp4_samsung_sysfs_probe(struct device *dev) { return 0; }
static inline int cyttsp4_samsung_sysfs_release(struct device *dev) { return 0; }
#endif

void cyttsp4_init_function_ptrs(struct cyttsp4_mt_data *md);
int _cyttsp4_subscribe_attention(struct device *dev,
	enum cyttsp4_atten_type type, char id, int (*func)(struct device *),
	int mode);
int _cyttsp4_unsubscribe_attention(struct device *dev,
	enum cyttsp4_atten_type type, char id, int (*func)(struct device *),
	int mode);
struct cyttsp4_sysinfo *cyttsp4_request_sysinfo_(struct device *dev);
int cyttsp4_request_disable_scan_type_(struct device *dev, u8 scan_type);
int cyttsp4_request_enable_scan_type_(struct device *dev, u8 scan_type);

extern const struct dev_pm_ops cyttsp4_pm_ops;

int cyttsp4_core_suspend(struct device *dev);
int cyttsp4_core_resume(struct device *dev);

int upgrade_firmware_from_platform(struct device *dev, bool forced);
int upgrade_firmware_from_sdcard(struct device *dev);

#endif /* _CYTTSP4_REGS_H */
