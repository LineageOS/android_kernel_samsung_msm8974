/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Author: Rajucm <rajkumar.m@samsung.com>
 *	   kmini.park <kmini.park@samsung.com>
 *	   Daniel(Philju) Lee <daniel.lee@siliconimage.com>
 *
 * Date: 00:00 AM, 6th September, 2013
 *
 * Based on  Silicon Image MHL SII8246 Transmitter Driver
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

 #ifndef __SII8246_MHL_TX_H__
#define __SII8246_MHL_TX_H__
#include "sii8246_platform.h"
#include <linux/spinlock.h>
#include <linux/input.h>
#include <linux/device.h>
#define SFEATURE_MHL_TEST_CB

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define BIT0                    0x01
#define BIT1                    0x02
#define BIT2                    0x04
#define BIT3                    0x08
#define BIT4                    0x10
#define BIT5                    0x20
#define BIT6                    0x40
#define BIT7                    0x80


#define	PAGE_0			0x72
#define	PAGE_1			0x7A
#define	PAGE_2			0x92
#define	PAGE_CBUS		0xC8

#define	MHL_VER_MAJOR		(0x02 << 4)	/* bits 4..7 */
#define	MHL_VER_MINOR		0x00		/* bits 0..3 */
#define MHL_VERSION		(MHL_VER_MAJOR | MHL_VER_MINOR)

#define	MHL_DEV_CAT_SINK			0x01
#define	MHL_DEV_CAT_SOURCE			0x02
#define	MHL_DEV_CAT_DONGLE			0x03
#define	MHL_DEV_CAT_SELF_POWERED_DONGLE		0x13

#define MHL_SINK_W_POW  0x11
#define MHL_SINK_WO_POW 0x01
#define MHL_DONGLE_W_POW 0x13
#define MHL_DONGLE_WO_POW 0x03


#define SILICON_IMAGE_ADOPTER_ID 321

enum DevCapOffset_e {
	DEVCAP_OFFSET_DEV_STATE		= 0x00,
	DEVCAP_OFFSET_MHL_VERSION	= 0x01,
	DEVCAP_OFFSET_DEV_CAT		= 0x02,
	DEVCAP_OFFSET_ADOPTER_ID_H	= 0x03,
	DEVCAP_OFFSET_ADOPTER_ID_L	= 0x04,
	DEVCAP_OFFSET_VID_LINK_MODE	= 0x05,
	DEVCAP_OFFSET_AUD_LINK_MODE	= 0x06,
	DEVCAP_OFFSET_VIDEO_TYPE	= 0x07,
	DEVCAP_OFFSET_LOG_DEV_MAP	= 0x08,
	DEVCAP_OFFSET_BANDWIDTH		= 0x09,
	DEVCAP_OFFSET_FEATURE_FLAG	= 0x0A,
	DEVCAP_OFFSET_DEVICE_ID_H	= 0x0B,
	DEVCAP_OFFSET_DEVICE_ID_L	= 0x0C,
	DEVCAP_OFFSET_SCRATCHPAD_SIZE	= 0x0D,
	DEVCAP_OFFSET_INT_STAT_SIZE	= 0x0E,
	DEVCAP_OFFSET_RESERVED		= 0x0F,
	/* this one must be last */
	DEVCAP_SIZE,
};


#define	MHL_DEV_FEATURE_FLAG_OFFSET	DEVCAP_OFFSET_FEATURE_FLAG

/* Dongles have freedom to not support RCP, RAP and SCRATCHPAD */
#define	MHL_FEATURE_RCP_SUPPORT		BIT0
#define	MHL_FEATURE_RAP_SUPPORT		BIT1
#define	MHL_FEATURE_SP_SUPPORT		BIT2

#define TRANSCODER_DEVICE_ID 0x0000

/* This contains one nibble each - max offset */
#define	MHL_INT_AND_STATUS_SIZE		0x33
#define	MHL_SCRATCHPAD_SIZE		16
/* manually define highest number */
#define	MHL_MAX_BUFFER_SIZE	MHL_SCRATCHPAD_SIZE

#define DEVCAP_VAL_DEV_STATE       0
#define DEVCAP_VAL_MHL_VERSION     MHL_VERSION
#define DEVCAP_VAL_DEV_CAT         (MHL_DEV_CAT_SOURCE)
#define DEVCAP_VAL_ADOPTER_ID_H    (uint8_t)(SILICON_IMAGE_ADOPTER_ID >> 8)
#define DEVCAP_VAL_ADOPTER_ID_L    (uint8_t)(SILICON_IMAGE_ADOPTER_ID & 0xFF)
#define DEVCAP_VAL_VID_LINK_MODE   MHL_DEV_VID_LINK_SUPPRGB444
#define DEVCAP_VAL_AUD_LINK_MODE   0x01 /*DEV_AUDIO_LINK_2CH*/
#define DEVCAP_VAL_VIDEO_TYPE      0
#define DEVCAP_VAL_LOG_DEV_MAP     MHL_DEV_LD_GUI
#define DEVCAP_VAL_BANDWIDTH       0x0F
#define DEVCAP_VAL_FEATURE_FLAG	(MHL_FEATURE_RCP_SUPPORT | \
				MHL_FEATURE_RAP_SUPPORT |  \
				MHL_FEATURE_SP_SUPPORT)
#define DEVCAP_VAL_DEVICE_ID_H     (uint8_t)(TRANSCODER_DEVICE_ID >> 8)
#define DEVCAP_VAL_DEVICE_ID_L     (uint8_t)(TRANSCODER_DEVICE_ID & 0xFF)
#define DEVCAP_VAL_SCRATCHPAD_SIZE MHL_SCRATCHPAD_SIZE
#define DEVCAP_VAL_INT_STAT_SIZE   MHL_INT_AND_STATUS_SIZE
#define DEVCAP_VAL_RESERVED        0

/*============================================= */
#define INTR1_STATUS_ADD   0x71
#define INTR1_ENABLE_ADD   0x75
#define BIT_INTR1_RSEN_CHG  0x20
#define BIT_INTR1_HPD_CHG   0x40

#define INTR2_STATUS_ADD   0x72
#define INTR2_ENABLE_ADD   0x76
#define BIT_INTR2_TCLK_STBL 0x01

#define INTR3_STATUS_ADD    0x73
#define INTR3_ENABLE_ADD    0x77
#define BIT_INTR3_DDC_EMPTY 0x01
#define BIT_INTR3_DDC_FIFO_FULL 0x02
#define BIT_INTR3_DDD_FIFO_HALF_FULL 0x04
#define BIT_INTR3_DDC_CMD_DONE 0x08

#define INTR4_STATUS_ADD   0x74
#define INTR4_ENABLE_ADD   0x78
#define BIT_INTR4_SCDT    0x01
#define BIT_INTR4_RPWR5V  0x02
#define BIT_INTR4_MHL_EST 0x04
#define BIT_INTR4_NON_MHL_EST 0x08
#define BIT_INTR4_CBUS_LKOUT 0x10
#define BIT_INTR4_CBUS_DISCONNECT 0x20
#define BIT_INTR4_RGND	0x40

#define INTR5_STATUS_ADD   0x60
#define INTR5_ENABLE_ADD   0x63
#define BIT_INTR5_VBUS_CHG  0x01
#define BIT_INTR5_CKDT_CHANGE 0x02
#define BIT_INTR5_MHL_FIFO_OVERFLOW   0x04
#define BIT_INTR5_MHL_FIFO_UNDERFLOW  0x08

#define INTR7_STATUS_ADD   0x61
#define INTR7_ENABLE_ADD   0x64

#define INTR8_STATUS_ADD   0x62
#define INTR8_ENABLE_ADD   0x65

#define CBUS_INTR_STATUS_ADD   0x08
#define CBUS_INTR_ENABLE_ADD   0x09
#define BIT_CBUS_INTR_CONNECTION_CHG 0x01
#define BIT_CBUS_INTR_CEC_ABORT 0x02
#define BIT_CBUS_INTR_DDC_ABORT 0x04
#define BIT_CBUS_INTR_MSC_MSG_CMD_RCV 0x08
#define BIT_CBUS_INTR_MSC_CMD_DONE  0x10
#define BIT_CBUS_INTR_MSC_MT_ABORT 0x20  /* as requester */
#define BIT_CBUS_INTR_MSC_MR_ABORT  0x40 /* as responser */

#define CBUS_INTR0_STATUS_ADD   0x51
#define CBUS_INTR0_ENABLE_ADD   0x52
#define BIT_CBUS_INTR0_HPD_RCV   0x04
#define BIT_CBUS_INTR0_MSC_DONE_NACK   0x80


#define CBUS_INTR1_STATUS_ADD   0x53
#define CBUS_INTR1_ENABLE_ADD   0x54
#define BIT_CBUS_INTR1_MSC_ABORT  0x08
#define BIT_CBUS_INTR1_MSC_SET_CAP_ID 0x10
#define BIT_CBUS_INTR1_CBUS_PACKET_RCV   0x20
#define BIT_CBUS_INTR1_MSC_CMD_ABORT  0x40
#define BIT_CBUS_INTR1_MHL_CABLE_CONNECT  0x80

#define CBUS_INTR2_STATUS_ADD   0x1E
#define CBUS_INTR2_ENABLE_ADD   0x1F
#define BIT_CBUS_INTR2_MSC_WRITE_BURST_RCV  0x01
#define BIT_CBUS_INTR2_MSC_HEARTBEAT_MAX_FAIL 0x02
#define BIT_CBUS_INTR2_MSC_SET_INT   0x04
#define BIT_CBUS_INTR2_MSC_WRITE_STAT_RCV  0x08

/*============================================= */

#define	REG_CBUS_LINK_CONTROL_1				0x30
#define	REG_CBUS_LINK_CONTROL_2				0x31
#define	REG_CBUS_LINK_CONTROL_3				0x32
#define	REG_CBUS_LINK_CONTROL_4				0x33
#define	REG_CBUS_LINK_CONTROL_5				0x34
#define	REG_CBUS_LINK_CONTROL_6				0x35
#define	REG_CBUS_LINK_CONTROL_7				0x36
#define REG_CBUS_LINK_STATUS_1        0x37
#define REG_CBUS_LINK_STATUS_2        0x38
#define	REG_CBUS_LINK_CONTROL_8				0x39
#define	REG_CBUS_LINK_CONTROL_9				0x3A
#define	REG_CBUS_LINK_CONTROL_10			0x3B
#define	REG_CBUS_LINK_CONTROL_11			0x3C
#define	REG_CBUS_LINK_CONTROL_12			0x3D

#define REG_MSC_TIMEOUT_LIMIT           0x22
#define	MSC_TIMEOUT_LIMIT_MSB_MASK	    0x0F
#define	MSC_LEGACY_BIT					    (0x01 << 7)


#define BIT_DC9_VBUS_OUTPUT_CAPABILITY_SRC  0x01
#define BIT_DC9_WAKE_PULSE_BYPASS           0x02
#define BIT_DC9_DISC_PULSE_PROCEED          0x04
#define BIT_DC9_USB_EST                     0x08
#define BIT_DC9_WAKE_DRVFLT                 0x10
#define BIT_DC9_CBUS_LOW_TO_DISCONNECT      0x20
#define BIT_DC9_VBUS_EN_OVERRIDE            0x40
#define BIT_DC9_VBUS_EN_OVERRIDE_VAL        0x80


/* Device Category */

#define MHL_CAP_DEV_STATE 0x00
#define MHL_CAP_MHL_VERSION 0x01
#define	MHL_DEV_CATEGORY_OFFSET				0x02
#define MHL_CAP_ADOPTER_ID_H  0x03
#define MHL_CAP_ADOPTER_ID_L  0x04
#define MHL_CAP_VID_LINK_MODE 0x05
#define MHL_CAP_AUD_LINK_MODE 0x06
#define MHL_CAP_VIDEO_TYPE 0x07
#define MHL_CAP_LOG_DEV_MAP 0x08
#define MHL_CAP_BANDWIDTH 0x09
#define MHL_CAP_FEATURE_FLAG 0x0A
#define MHL_CAP_DEVICE_ID_H 0x0B
#define MHL_CAP_DEVICE_ID_L 0x0C
#define MHL_CAP_SCRATCHPAD_SIZE 0x0D
#define MHL_CAP_INT_STAT_SIZE 0x0E
#define MHL_CAP_RESERVED  0x0F

/* Video Link Mode */

#define	MHL_DEV_VID_LINK_SUPPRGB444			0x01
#define	MHL_DEV_VID_LINK_SUPPYCBCR444		0x02
#define	MHL_DEV_VID_LINK_SUPPYCBCR422		0x04
#define	MHL_DEV_VID_LINK_SUPP_PPIXEL		0x08
#define	MHL_DEV_VID_LINK_SUPP_ISLANDS		0x10


#define MHL_RCHANGE_INT                     0x20
#define MHL_DCHANGE_INT                     0x21

#define	MHL_INT_EDID_CHG					BIT1

#define	MHL_INT_DCAP_CHG					          BIT0
#define MHL_INT_DSCR_CHG                    BIT1
#define MHL_INT_REQ_WRT                     BIT2
#define MHL_INT_GRT_WRT                     BIT3
#define MHL2_INT_3D_REQ                     BIT4

#define MHL_3D_VIC_CODE 0x0010
#define MHL_3D_DTD_CODE 0x0011

#define	MHL_BANDWIDTH_LIMIT		22	/* 225 MHz*/


#define MHL_STATUS_REG_CONNECTED_RDY        0x30
#define MHL_STATUS_REG_LINK_MODE            0x31

#define	MHL_STATUS_DCAP_RDY					BIT0

#define MHL_STATUS_CLK_MODE_MASK            0x07
#define MHL_STATUS_CLK_MODE_PACKED_PIXEL    0x02
#define MHL_STATUS_CLK_MODE_NORMAL          0x03
#define MHL_STATUS_PATH_EN_MASK             0x08
#define MHL_STATUS_PATH_ENABLED             0x08
#define MHL_STATUS_PATH_DISABLED            0x00
#define MHL_STATUS_MUTED_MASK               0x10

#define MSC_SEND  0x00
#define MSC_DONE_ACK  0x01
#define MSC_DONE_NACK 0x02

#define BIT_CBUS_MSC_PEER_CMD              0x01
#define BIT_CBUS_MSC_MSG                   0x02
#define BIT_CBUS_MSC_READ_DEVCAP           0x04
#define BIT_CBUS_MSC_WRITE_STAT_OR_SET_INT 0x08
#define BIT_CBUS_MSC_WRITE_BURST           0x10
/* Responder aborted DDC command at translation layer */
#define BIT_DDC_ABORT		BIT2
/* Responder sent a VS_MSG packet (response data or command.) */
#define BIT_MSC_MSG_RCV		BIT3
/* Responder sent ACK packet (not VS_MSG) */
#define BIT_MSC_XFR_DONE	BIT4
/* Command send aborted on TX side */
#define BIT_MSC_XFR_ABORT	BIT5
/* Responder aborted MSC command at translation layer */
#define BIT_MSC_ABORT		BIT6

#define BIT_CBUS_MSC_MT_ABORT_INT_MAX_FAIL             0x01
#define BIT_CBUS_MSC_MT_ABORT_INT_PROTO_ERR            0x02
#define BIT_CBUS_MSC_MT_ABORT_INT_TIMEOUT              0x04
#define BIT_CBUS_MSC_MT_ABORT_INT_UNDEF_CMD            0x08
#define BIT_CBUS_MSC_MT_ABORT_INT_MSC_MT_PEER_ABORT    0x80

/*Turn content streaming ON/OFF*/
#define	MHL_RAP_CONTENT_ON		0x10
#define	MHL_RAP_CONTENT_OFF		0x11

#define	RCPE_NO_ERROR			0x00
#define	RCPE_INEEFECTIVE_KEY_CODE	0x01
#define	RCPE_BUSY			0x02


#define	MHL_DEV_LD_DISPLAY		(0x01 << 0)
#define	MHL_DEV_LD_VIDEO		(0x01 << 1)
#define	MHL_DEV_LD_AUDIO		(0x01 << 2)
#define	MHL_DEV_LD_MEDIA		(0x01 << 3)
#define	MHL_DEV_LD_TUNER		(0x01 << 4)
#define	MHL_DEV_LD_RECORD		(0x01 << 5)
#define	MHL_DEV_LD_SPEAKER		(0x01 << 6)
#define	MHL_DEV_LD_GUI			(0x01 << 7)

#define	MHL_LOGICAL_DEVICE_MAP	(MHL_DEV_LD_AUDIO | \
				MHL_DEV_LD_VIDEO | MHL_DEV_LD_MEDIA | \
				MHL_DEV_LD_GUI)

enum {
	MHL_MSC_MSG_RCP             = 0x10, /* RCP sub-command */
	MHL_MSC_MSG_RCPK            = 0x11, /* RCP Acknowledge sub-command */
	MHL_MSC_MSG_RCPE            = 0x12, /* RCP Error sub-command */
	MHL_MSC_MSG_RAP             = 0x20, /* Mode Change Warning sub-command*/
	MHL_MSC_MSG_RAPK            = 0x21, /* MCW Acknowledge sub-command */
	MHL_MSC_MSG_UCP             = 0x30, /* UCP sub-command */
	MHL_MSC_MSG_UCPK            = 0x31, /* UCP Acknowledge sub-command */
	MHL_MSC_MSG_UCPE            = 0x32, /* UCP Error sub-command */
};


enum {
	MHL_ACK			= 0x33,	/* Command or Data byte acknowledge */
	MHL_NACK		= 0x34,	/*Command or Data byte not acknowledge*/
	MHL_ABORT		= 0x35,	/* Transaction abort */
	MHL_WRITE_STAT		= 0x60 | 0x80,
	/* 0xE0 - Write one status register strip top bit */
	MHL_SET_INT		= 0x60,	/* Write one interrupt register */
	MHL_READ_DEVCAP		= 0x61,	/* Read one register */
	MHL_GET_STATE		= 0x62,
	/* Read CBUS revision level from follower */
	MHL_GET_VENDOR_ID	= 0x63,	/* Read vendor ID value from follower.*/
	MHL_SET_HPD		= 0x64,	/* Set Hot Plug Detect in follower */
	MHL_CLR_HPD		= 0x65,	/* Clear Hot Plug Detect in follower */
	MHL_SET_CAP_ID		= 0x66,
	/*Set Capture ID for downstream device.*/
	MHL_GET_CAP_ID		= 0x67,
	/*Get Capture ID from downstream device.*/
	MHL_MSC_MSG		= 0x68,	/*VS command to send RCP sub-commands */
	MHL_GET_SC1_ERRORCODE	= 0x69,
	/* Get Vendor-Specific command error code.*/
	MHL_GET_DDC_ERRORCODE	= 0x6A,	/* Get DDC channel command error code.*/
	MHL_GET_MSC_ERRORCODE	= 0x6B,	/* Get MSC command error code.*/
	MHL_WRITE_BURST		= 0x6C,
	/* Write 1-16 bytes to responder's scratchpad.*/
	MHL_GET_SC3_ERRORCODE	= 0x6D,	/* Get channel 3 command error code.*/
};


struct tx_page0 {
	struct mhl_platform_data	*pdata;
};

struct tx_page1 {
	struct mhl_platform_data	*pdata;
};

struct tx_page2 {
	struct mhl_platform_data	*pdata;
};

struct cbus_msg {
	u8 command;
	u8 offset;
	u8 buff[3]; /*currently we use 3 byte as max len*/
	struct list_head list;
};

struct mhl_tx {
	u8 msc_cmd_done_intr;
	bool msc_cmd_abort;

	struct mutex  cbus_cmd_lock;

	struct mhl_platform_data	*pdata;
	struct work_struct cbus_cmd_work;
	struct work_struct mhl_power_on;
	struct workqueue_struct *cbus_cmd_wqs;

	wait_queue_head_t cbus_cmd_wq;
	struct list_head cbus_msg_list;
	bool power_state;
	bool hpd_state;
	struct wake_lock mhl_wake_lock;
	struct notifier_block mhl_nb;
	struct input_dev *input_dev;
};

#ifdef CONFIG_SAMSUNG_LPM_MODE
extern int poweroff_charging;
#endif

#if defined(FEATURE_SS_MHL_TEST_APP_SUPPORT)
extern int hdmi_forced_resolution;
#endif

#if defined(FEATURE_SS_MHL_RCP_SUPPORT)
extern bool is_key_supported(int keyindex);
extern struct input_dev *register_mhl_input_device(void);
extern void rcp_key_report(struct input_dev *input_dev, u8 key);
#endif
#endif/*__SII8246_MHL_TX_H__*/
