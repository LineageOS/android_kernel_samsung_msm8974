/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Author: Kyungmin Park <kmini.park@samsung.com>
 *
 * Date: 3:00 PM, 31st May,2012
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

#ifndef __SII8240_H__
#define __SII8240_H__

#include <linux/notifier.h>
#include <linux/timer.h>
#include <linux/switch.h>
#include <linux/wakelock.h>

#ifdef CONFIG_EXTCON
#include <linux/extcon.h>
#endif

#include "sii8240_rcp.h"
#include "../../mdss/mdss_hdmi_util.h"

#undef MHL_2X_3D
/* for factory test process */
#define CONFIG_SS_FACTORY
#define SFEATURE_UNSTABLE_SOURCE_WA
#define SFEATURE_HDCP_SUPPORT
#define SII8240_CHECK_MONITOR

#define  MHL_MAJOR                271

/* I2C Register pages mentioned in SII8240 PRM */

/* Page-0:TMDS & MISC control */
#define SII8240_MHL_TMDS_TX		0x72

/* Page-1:Reserved */
#define SII8240_MHL_RESERVED		0x7A

/* Page-2:HDMI, Analog core control */
#define SII8240_MHL_HDMI_RX		0x92

/* Page-3:Discover control & interrupt */
#define SII8240_MHL_DISC_TX		0x9A

/* TPI:TMDS,HDCP,EDID,InfoFrame Control */
#define SII8240_MHL_TPI_TX		0xB2

/* VS:Reserved */
#define SII8240_MHL_VS_RESERVED		0xC0

/* CBUS:Cbus config,control etc */
#define SII8240_MHL_CBUS_TX		0xC8

/* DDC:EDID Segment Pointer */
#define SII8240_MHL_EDID_SEG_TX		0x60

/* DDC:HDCP Protocol */
#define SII8240_MHL_HDCP_TX		0x74

/* DDC:EDID Transfer */
#define SII8240_MHL_EDID_XFER_TX	0xA0


/* Page-0[0x72]:TMDS & MISC control Registers & Bit-defintions */

#define MHL_TX_DEV_IDLOW_REG		0x02	/* default value: 0x40 */
#define MHL_TX_DEV_IDHIGH_REG		0x03	/* default value: 0x82 */
#define MHL_TX_DEV_IDREV_REG		0x04	/* default value: 0x00 */

#define UPSTRM_HPD_CTRL_REG		0x79
#define HPD_OVERRIDE_EN			(1<<4)
#define HPD_OUT_OVERRIDE_VAL		(1<<5)
#define HPD_OUTPUT_OD_EN		(1<<6)
#define TPI_DISABLE_REG			0xC7	/* Not documented */
#define SW_TPI_EN_MASK			0x80
#define SW_TPI_EN_HW_TPI		0x00
#define SW_TPI_EN_NON_HW_TPI		0x80

#define DCTL_REG			0x0D	/* Not documented */
#define TCLK3X_PHASE			0x01
#define TCLK_PHASE_MASK			0x02
#define TCLK_PHASE_NORMAL		0x00
#define TCLK_PHASE_INVERTED		0x02
#define TRANSCODE_MASK			0x08
#define TRANSCODE_OFF			0x00
#define TRANSCODE_ON			0x08
#define EXT_DDC_SEL			0x10

/* Page-2[0x92]: Analog Core Registers */

#define TMDS_CLK_EN_REG			0x11	/* Not documented */
#define TMDS_CH_EN_REG			0x12	/* Not documented */

#define MHLTX_TERM_CTRL_REG		0x80
#define MHLTX_TERM_OFF			((1<<6) | (1<<7))

#define MHLTX_CTL4_REG			0x83	/* Not documented */
#define DATA_SWING_CTL_MASK		0x07
#define CLK_SWING_CTL_MASK		0x38

#define MHLTX_CTL7_REG			0x86	/* Not documented */

#define TPI_PACKET_FILTER_REG		0x90	/* Not documented */
#define DROP_GCP_PKT			(1<<0)
#define DROP_CTS_PKT			(1<<1)
#define DROP_AVI_PKT			(1<<2)
#define DROP_AIF_PKT			(1<<3)
#define DROP_SPIF_PKT			(1<<4)
#define DROP_MPEG_PKT			(1<<5)
#define DROP_CEA_CP_PKT			(1<<6)
#define DROP_CEA_GAMUT_PKT		(1<<7)

#define EDID_CTRL_REG			0xE3	/* Not documented */

/* PAge-3[0x9A]:Power & Interrupt Registers */
#define POWER_CTRL_REG			0x01
#define MASTER_POWER_CTRL_D0		(1<<0)
#define MASTER_POWER_CTRL_D3		(~(1<<0))
#define PCLK_EN				(1<<2)
#define OSC_EN				(1<<4)
#define TMDS_RX_EN			(1<<5)
#define TMDS_TX_EN			(1<<6)
#define POWER_TO_D0		(MASTER_POWER_CTRL_D0 | PCLK_EN | OSC_EN | \
				TMDS_RX_EN | TMDS_TX_EN)
#define POWER_DOWN			0x00

#define INT_CTRL_REG			0x20
#define	INTR_OPEN_DRAIN			(1<<2)
#define INTR_POLARITY			(1<<1)

/* PAge-3[0x9A]: Discovery Registers */
#define DISC_INTR_REG			0x21
#define RGND_RDY_INT			(1<<6)
#define CBUS_DISCONNECT_INT		(1<<5)
#define CBUS_UNSTABLE_INT		(1<<4)
#define NON_MHL_DETECTION_INT		(1<<3)
#define MHL_EST_INT			(1<<2)
#define MHL_VUBS_CHG          (1<<1)

#define DISC_INTR_ENABLE_REG		0x22

#define DISC_CTRL1_REG			0x10
#define CBUS_DISCOVERY_ENABLED		(1<<0)
#define PDN_REGS_ACCESS_STROBE		(~(1<<1))

#define DISC_CTRL3_REG			0x12
#define MHL_MODE_FORCED			(1<<6)
#define MHL_MODE_NORMAL			(~(1<<6))
#define USB_ID_SWITCH_SW_ENABLED	(1<<3)
#define USB_ID_SWITCH_SW_DISABLED	(~(1<<3))

#define DISC_CTRL6_REG			0x15
#define USB_ID_SWITCH_SW		(1<<6)

#define DISC_CTRL9_REG			0x18	/* Not Documented */
#define VBUS_OUTPUT_CAPABILITY_SRC	0x01
#define WAKE_PULSE_BYPASS		0x02
#define DISC_PULSE_PROCEED		0x04
#define USB_EST				0x08
#define WAKE_DRVFLT			0x10
#define CBUS_LOW_TO_DISCONNECT		0x20
#define VBUS_EN_OVERRIDE		0x40
#define VBUS_EN_OVERRIDE_VAL		0x80
#define MHLTX_CTRL_AON_REG		0x32

#define DISC_RGND_REG			0x1C
#define RGND_INTP_MASK			0x03
#define	RGND_INTP_OPEN			0
#define RGND_INTP_2K			1
#define RGND_INTP_1K			2
#define RGND_INTP_SHORT			3

#define	US_TMDS_STATUS	0x74
/* MHL TX SYS STAT Registers */
#define MHL_TX_SYSSTAT_REG		0x09

#define	BIT_INTR5_SCDT_CHANGE		0x01
#define	BIT_INTR5_CKDT_CHANGE		0x02
#define	BIT_INTR5_MHL_FIFO_OVERFLOW	0x04
#define	BIT_INTR5_MHL_FIFO_UNDERFLOW 0x08
#define	BIT_INTR5_RPWR5V_CHG		0x10
#define	BIT_INTR5_PQ_OVERFLOW		0x80

/* TPI[0xB2]:HDCP Registers */
#define TPI_AV_MUTE_REG			0x1A	/* Not Documented */
#define AV_MUTE_NORMAL			0x00
#define AV_MUTE_MUTED			0x08
#define AV_MUTE_MASK			0x08

#define TPI_HDCP_CTRL_REG		0x2A
#define	INTERM_RI_CHECK_EN		(1<<3)
#define	DOUBLE_RI_CHECK			(1<<2)
#define DDC_SHORT_RI_RD			(1<<1)
#define HDCP_START			(1<<0)

#define BIT_DPD_PDIDCK_MASK 0x04
#define BIT_DPD_PDIDCK_POWER_DOWN        0x00

/* Device Capbaility Register address and offsets have same values now.
 * For ex- while receving SET_INT, use these Registers [0x21...0x23],
 * while sending SET_INT, use these Offset[0x21...0x23]. */

/* 0xC8: CBUS Device Capability Registers */
#define MHL_VER_MAJOR			(CONFIG_MHL_VERSION >> 4) /* Most Significant 4-bits */
#define MHL_VER_MINOR			(CONFIG_MHL_VERSION & 0x0F) /* Least significant 4-bits */

#define DEV_TYPE_SOURCE			0x02
#define DEV_TYPE_SINK			0x01
#define DEV_TYPE_DONGLE			0x03
/* set when Device can supply power across VBUS,else clear this bit */
#define DEV_POW_SUPPLY			(1<<4)

#define VID_LINK_SUPP_RGB444		(1<<0)
#define VID_LINK_SUPP_YCBCR444		(1<<1)
#define VID_LINK_SUPP_YCBCR422		(1<<2)
#define VID_LINK_SUPP_PPIXEL		(1<<3)
#define VID_LINK_SUPP_ISLANDS		(1<<4)
#define VID_LINK_SUPP_VGA		(1<<5)

#define CBUS_AUD_LINK_MODE_REG		0x06
#define DEV_AUDIO_LINK_2CH		(1<<0)
#define DEV_AUDIO_LINK_8CH		(1<<1)

#define VIDEO_TYPE_GRAPHICS		(1<<0)
#define VIDEO_TYPE_PHOTO		(1<<1)
#define VIDEO_TYPE_CINEMA		(1<<2)
#define VIDEO_TYPE_GAME			(1<<3)
/* When cleared,ignore the video type */
#define SUPPORT_VT			(1<<7)

#define LOG_DEV_DISPLAY			(1<<0)
#define LOG_DEV_VIDEO			(1<<1)
#define LOG_DEV_AUDIO			(1<<2)
#define LOG_DEV_MEDIA			(1<<3)
#define LOG_DEV_TUNER			(1<<4)
#define LOG_DEV_RECORD			(1<<5)
#define LOG_DEV_SPEAKER			(1<<6)
#define LOG_DEV_GUI			(1<<7)

#define CBUS_FEATURE_FLAG_REG		0x0A
#define RCP_SUPPORT			(1<<0)
#define RAP_SUPPORT			(1<<1)
#define	SP_SUPPORT			(1<<2)

/* Bits [4...7] */
#define STATUS_SIZE			0x3
/* Bits [0...3] */
#define INTR_SIZE			0x3

#define DEVCAP_COUNT_MAX		16

#define MHL_DEVCAP_DEVSTATE		0x0
#define MHL_DEVCAP_MHL_VERSION		0x1
#define MHL_DEVCAP_DEV_CAT		0x2
#define MHL_DEVCAP_ADOPTER_ID_H		0x3
#define MHL_DEVCAP_ADOPTER_ID_L		0x4
#define MHL_DEVCAP_VID_LINK_MODE	0x5
#define MHL_DEVCAP_AUD_LINK_MODE	0x6
#define MHL_DEVCAP_VIDEO_TYPE		0x7
#define MHL_DEVCAP_LOG_DEV_MAP		0x8
#define MHL_DEVCAP_BANDWIDTH		0x9
#define MHL_DEVCAP_FEATURE_FLAG		0xa
#define MHL_DEVCAP_DEVICE_ID_H		0xb
#define MHL_DEVCAP_DEVICE_ID_L		0xc
#define MHL_DEVCAP_SCRATHPAD_SIZE	0xd
#define MHL_DEVCAP_INT_STAT_SIZE	0xe
#define MHL_DEVCAP_RESERVED		0xf

/* Device interrupt register offset of connected device */
#define CBUS_MHL_INTR_REG_0		0x20
#define MHL_INT_DCAP_CHG		(1<<0)
#define	MHL_INT_DSCR_CHG		(1<<1)
#define MHL_INT_REQ_WRT			(1<<2)
#define MHL_INT_GRT_WRT			(1<<3)
#define MHL_INT_3D_REQ			(1<<4)

#define CBUS_MHL_INTR_REG_1		0x21
#define MHL_INT_EDID_CHG		(1<<1)

#define CBUS_MHL_INTR_REG_2		0x22
#define CBUS_MHL_INTR_REG_3		0x23

/* CBUS: Mask Registers for Interrupt offset registers(0x21...0x23) */
#define CBUS_MHL_INTR_REG_0_MASK	0x80
#define CBUS_MHL_INTR_REG_1_MASK	0x81
#define CBUS_MHL_INTR_REG_2_MASK	0x82
#define CBUS_MHL_INTR_REG_3_MASK	0x83

/* CBUS: Status Register offset for connected device */
#define CBUS_MHL_STATUS_OFFSET_0	0x30 /* CONNECTED_RDY */
#define MHL_STATUS_DCAP_READY		(1<<0)
#define CBUS_MHL_STATUS_OFFSET_1	0x31 /* LINK_MODE */
#define MHL_STATUS_CLK_PACKED_PIXEL	(1<<1)
#define MHL_STATUS_CLK_NORMAL		((1<<0) | (1<<1))
#define MHL_STATUS_PATH_ENABLED		(1<<3)
#define MHL_STATUS_MUTED		(1<<4)
#define MHL_STATUS_PATH_DISABLED	0x00

#define CBUS_MHL_STATUS_OFFSET_2	0x32
#define CBUS_MHL_STATUS_OFFSET_3	0x33

/* CBUS: Scratchpad Registers */
#define CBUS_MHL_SCRPAD_REG_0		0x40
#define CBUS_MHL_SCRPAD_REG_1		0x41
#define CBUS_MHL_SCRPAD_REG_2		0x42
#define CBUS_MHL_SCRPAD_REG_3		0x43
#define CBUS_MHL_SCRPAD_REG_4		0x44
#define CBUS_MHL_SCRPAD_REG_5		0x45
#define CBUS_MHL_SCRPAD_REG_6		0x46
#define CBUS_MHL_SCRPAD_REG_7		0x47
#define CBUS_MHL_SCRPAD_REG_8		0x48
#define CBUS_MHL_SCRPAD_REG_9		0x49
#define CBUS_MHL_SCRPAD_REG_A		0x4A
#define CBUS_MHL_SCRPAD_REG_B		0x4B
#define CBUS_MHL_SCRPAD_REG_C		0x4C
#define CBUS_MHL_SCRPAD_REG_D		0x4D
#define CBUS_MHL_SCRPAD_REG_E		0x4E
#define CBUS_MHL_SCRPAD_REG_F		0x4F


#define CBUS_CONN_STATUS_REG		0x91
#define DOWNSTREAM_HPD_MASK		0x04
#define DOWNSTREAM_CLR_HPD_RECD		0
#define DOWNSTREAM_SET_HPD_RECD		1

#define CBUS_MSC_INTR_REG		0x92
#define CBUS_MSC_INTR_ENABLE_REG	0x93
#define MSC_MSG_NACK_RCVD		(1<<7)
#define MSC_SET_INT_RCVD		(1<<6)
#define MSC_WRITE_BURST_RCVD		(1<<5)
#define MSC_MSG_RCVD			(1<<4)
#define MSC_WRITE_STAT_RCVD		(1<<3)
#define MSC_HPD_RCVD			(1<<2)
#define MSC_CMD_DONE			(1<<1)


#define CBUS_MSC_CMD_START_REG		0xB8
#define START_WRITE_BURST		(1<<4)
/*start WRITE_STAT or SET_INT */
#define START_WRITE_STAT_SET_INT	(1<<3)
#define START_READ_DEVCAP		(1<<2)
#define START_MSC_MSG			(1<<1)
#define	START_MISC_CMD			(1<<0)

/* sii8240 MSC error interrupt mask register 0xC8:0x95*/
/* Responder aborted DDC command at translation layer */
#define BIT_CBUS_CEC_ABRT              0x02
/* Responder sent a VS_MSG packet (response data or command.) */
#define BIT_CBUS_DDC_ABRT              0x04
#define BIT_CBUS_MSC_ABORT_RCVD        0x08
#define BIT_CBUS_MSC_SET_CAP_ID_RCVD   0x10
#define BIT_CBUS_RCV_VALID             0x20
#define BIT_CBUS_CMD_ABORT             0x40
#define BIT_CBUS_MHL_CABLE_CNX_CHG     0x80

/* MSC Command Opcode or offset for MSC commands.
 * if START_MISC_CMD, used as command Opcode,else used as offset[except
 * in case of MSC_MSG]
 */
#define MSC_CMD_OR_OFFSET_REG		0xB9

#define MSC_SEND_DATA1_REG		0xBA
#define MSC_SEND_DATA2_REG		0xBB
#define MSC_RCVD_DATA1_REG		0xBC
#define MSC_MSG_RCVD_DATA1_REG		0xBF
#define MSC_MSG_RCVD_DATA2_REG		0xC0

#define MSC_RETRY_INTERVAL_REG		0xD5 /* default:0x14 */

#define CBUS_MSC_ERROR_INTR_REG		0x94
#define CBUS_MSC_ERROR_INTR_ENABLE_REG	0x95

/* sii8240 initiated an MSC command,but an error occured;exact error
 * available in CBUS:0x9A */
#define SEND_ERROR_INT			(1<<6)

/* sii8240 was receiving an MSC command,but an error occured;exact error
 * available in CBUS:0x9C */
#define RECD_ERROR_INT			(1<<3)
#define RECD_DDC_ABORT_INT		(1<<2)


#define MSC_SEND_ERROR_REG		0x9A
#define MSC_RECVD_ERROR_REG		0x9C
/* [Sender]:Last command sent was ABORT-ed;[Receiver]:Received an ABORT */
#define CMD_ABORT			(1<<7)

/* [Sender]:Last command rejected because of undefined opcode,never sent over
 * CBUS;[Receiver]:first packet contained an undefined opcode */
#define CMD_UNDEF_OPCODE		(1<<3)

/* [Sender]:Failed to receive a response from Receiver[Timeout];
 * [Receiver]:Failed to receieve sender's response(Sender Timeout) */
#define CMD_TIMEOUT			(1<<2)

/*[Sender]: Last packet of MSC command received Protocol error
 *[Receiver]: Last packet of MSC command response received protocol error
 */
#define CMD_RCVD_PROT_ERR		(1<<1)
/* Retry Threshold exceeded */
#define CMD_MAX_FAIL		(1<<0)

/* events to be used by notifier;issued by board file or connector driver
   to notify MHL driver of various events */
#define DONGLE_DETACHED			0x00
#define DONGLE_ATTACHED			0x01
#define DONGLE_POWER_ATTACHED		0x02
#define DONGLE_POWER_DETACHED		0x03

#define MHL_CON_UNHANDLED		0
#define MHL_CON_HANDLED			1
#define MHL_CON_RETRY			2

/* Some Time limits as per MHL Specifications */
#define T_WAIT_TIMEOUT_RGND_INT		3000
#define T_WAIT_TIMEOUT_DISC_INT		2000

/* Host Device capabilities values */
#define DEV_STATE			0
/* MHL Version 1.2 */
#define DEV_MHL_VERSION			CONFIG_MHL_VERSION
/* A source kind of device which will not provide power across VBUS */
#define DEV_CAT_SOURCE_NO_PWR		(DEV_POW_SUPPLY | DEV_TYPE_SOURCE)
#define DEV_ADOPTER_ID_H		0x01	/* Samsung's Adopter ID? */
#define DEV_ADOPTER_ID_L		0x41
#define DEV_VID_LINK_MODE		(VID_LINK_SUPP_RGB444 | \
					VID_LINK_SUPP_PPIXEL)
#define DEV_AUDIO_LINK_MODE		DEV_AUDIO_LINK_2CH
#define DEV_VIDEO_TYPE			0
#define DEV_LOGICAL_DEV			LOG_DEV_GUI
/*TODO: need to check bandwidth value whether 75MHz or 150 MHz */
#define DEV_BANDWIDTH			0xF
#define DEV_FEATURE_FLAG		(RCP_SUPPORT | RAP_SUPPORT | SP_SUPPORT)
#define DEV_DEVICE_ID_H			0x00 /* Samsung Device Specific values */
#define DEV_DEVICE_ID_L			0x00
#define DEV_SCRATCHPAD_SIZE		16
#define DEV_INT_STATUS_SIZE		((STATUS_SIZE<<4) | INTR_SIZE)
#define DEV_RESERVED			0

#define INFO_BUFFER		31
#define SIZE_AVI_INFOFRAME	14
#define EDID_SIZE	128

#define MHL_STATUS_CLK_MODE_PACKED_PIXEL	0x02
#define MHL_STATUS_CLK_MODE_NORMAL	0x03

#define BIT_INTR4_VBUS_CHG			0x01
#define BIT_INTR4_MHL_EST			0x04
#define BIT_INTR4_NON_MHL_EST		0x08
#define BIT_INTR4_CBUS_LKOUT			0x10
#define BIT_INTR4_CBUS_DISCONNECT	0x20
#define BIT_INTR4_RGND_DETECTION		0x40

#define BIT_INTR1_RSEN_CHG 0x20
#define BIT_INTR1_HPD_CHG 0x40

#define BIT_MHLTX_CTL1_VCO_FMAX_CTL_MASK   0x0F
#define BIT_MHLTX_CTL1_DISC_OVRIDE_MASK    0x10
#define BIT_MHLTX_CTL1_DISC_OVRIDE_OFF     0x00
#define BIT_MHLTX_CTL1_DISC_OVRIDE_ON      0x10
#define BIT_MHLTX_CTL1_TX_TERM_MODE_MASK   0xC0

/*MSC interrupt mask register 0xC8:0x93*/
#define	BIT_CBUS_CNX_CHG (1<<0)
#define	BIT_CBUS_MSC_MT_DONE (1<<1)
#define	BIT_CBUS_HPD_RCVD (1<<2)
#define	BIT_CBUS_MSC_MR_WRITE_STAT (1<<3)
#define	BIT_CBUS_MSC_MR_MSC_MSG (1<<4)
#define	BIT_CBUS_MSC_MR_WRITE_BURST (1<<5)
#define	BIT_CBUS_MSC_MR_SET_INT (1<<6)
#define	BIT_CBUS_MSC_MT_DONE_NACK (1<<7)

/*TPI control*/
#define TMDS_OUTPUT_MODE_MASK 0x01
#define TMDS_OUTPUT_MODE_DVI 0x00
#define TMDS_OUTPUT_MODE_HDMI 0x01

/*TMDS control*/
#define TMDS_OUTPUT_CONTROL_MASK 0x10
#define TMDS_OUTPUT_CONTROL_ACTIVE 0x00
#define TMDS_OUTPUT_CONTROL_POWER_DOWN 0x10

#define BIT_DC6_USB_OVERRIDE_MASK 0x40
#define BIT_DC6_USB_OVERRIDE_OFF   0x00
#define BIT_DC6_USB_OVERRIDE_ON    0x40

#define BIT_DC3_DLYTRG_SEL_MASK  0x07
#define BIT_DC3_DLYTRG_SEL_133us 0x00
#define BIT_DC3_DLYTRG_SEL_534us 0x01
#define BIT_DC3_DLYTRG_SEL_002ms 0x02
#define BIT_DC3_DLYTRG_SEL_008ms 0x03
#define BIT_DC3_DLYTRG_SEL_016ms 0x04
#define BIT_DC3_DLYTRG_SEL_032ms 0x05
#define BIT_DC3_DLYTRG_SEL_064ms 0x06
#define BIT_DC3_DLYTRG_SEL_128ms 0x07
#define BIT_DC3_USB_EN_MASK      0x08
#define BIT_DC3_USB_EN_OFF       0x00
#define BIT_DC3_USB_EN_ON        0x08
#define BIT_DC3_FORCE_USB_MASK   0x10
#define BIT_DC3_FORCE_USB_OFF    0x00
#define BIT_DC3_FORCE_USB_ON     0x10
#define BIT_DC3_DISC_SIMODE_MASK 0x20
#define BIT_DC3_DISC_SIMODE_OFF  0x00
#define BIT_DC3_DISC_SIMODE_ON   0x20
#define BIT_DC3_FORCE_MHL_MASK   0x40
#define BIT_DC3_FORCE_MHL_OFF    0x00
#define BIT_DC3_FORCE_MHL_ON     0x40
#define BIT_DC3_COMM_IMME_MASK   0x80
#define BIT_DC3_COMM_IMME_OFF    0x00
#define BIT_DC3_COMM_IMME_ON     0x80

#define BIT_HPD_CTRL_HPD_OUT_OVR_EN_MASK  0x10
#define BIT_HPD_CTRL_HPD_OUT_OVR_EN_OFF   0x00
#define BIT_HPD_CTRL_HPD_OUT_OVR_EN_ON    0x10
#define BIT_HPD_CTRL_HPD_OUT_OVR_VAL_MASK 0x20
#define BIT_HPD_CTRL_HPD_OUT_OVR_VAL_OFF  0x00
#define BIT_HPD_CTRL_HPD_OUT_OVR_VAL_ON   0x20

/*Video link mode*/
#define	MHL_DEV_VID_LINK_SUPPRGB444			0x01
#define	MHL_DEV_VID_LINK_SUPPYCBCR444		0x02
#define	MHL_DEV_VID_LINK_SUPPYCBCR422		0x04
#define	MHL_DEV_VID_LINK_SUPP_PPIXEL		0x08
#define	MHL_DEV_VID_LINK_SUPP_ISLANDS		0x10

#define	MHL_DEV_AUD_LINK_2CH				0x01
#define	MHL_DEV_AUD_LINK_8CH				0x02

/*Edid*/
#define EDID_MAX_LENGTH	512
#define EDID_STATUS_HW_ASSIST_REG	0xE0
#define HW_EDID_DONE (1<<5)
#define HW_EDID_ERROR (1<<6)
#define EDID_CTRL_REG			0xE3
#define EDID_MODE_EN			(1<<0)
#define INVALID_BKSV			(1<<1)
#define EDID_FIFO_ADDR_AUTO		(1<<4)
#define SELECT_DEVCAP			(1<<5)
#define EDID_PRIME_VALID		(1<<7)
#define EDID_PAGE_HW_ASSIST_REG		0xE2
#define HW_READ_EDID_BLOCK_0		(1<<1)
/* none of the below 4 registers are documented in PRM */
#define EDID_FIFO_ADDR_REG 0xE9
#define EDID_FIFO_WR_DATA_REG 0xEA
#define EDID_FIFO_RD_DATA_REG 0xEC
#define EDID_BLOCK_ADDR_HW_ASSIST_REG 0xED

/* Upstream register which means that MHL source */
#define US_HDMI_INFO_PKT_CTRL 0xA3
#define US_HPD_CTRL 0x79
#define US_INTR 0x7B
#define US_INTR_MASK 0x7D
#define REG_US_TMDS_STATUS 0x74
#define REG_US_TMDS_STATUS_MASK 0x78

#define BIT_INTR7_MUTE_ON       0x01
#define BIT_INTR7_CP_NEW_CP     0x02
#define BIT_INTR7_CP_SET_MUTE   0x04
#define BIT_INTR7_CP_CLR_MUTE   0x08
#define BIT_INTR7_CEA_NO_AVI    0x40
#define BIT_INTR7_CEA_NO_VSI    0x80

#define BIT_INTR8_CEA_DET_AIF      0x01
#define BIT_INTR8_CEA_NEW_AVI      0x02
#define BIT_INTR8_CEA_NEW_VSI      0x04
#define BIT_INTR8_CEA_DET_ACP      0x08
#define BIT_INTR8_CEA_DET_SPD      0x10
#define BIT_INTR8_CEA_DET_IS_RC1   0x20
#define BIT_INTR8_CEA_DET_IS_RC2   0x40

/*AVI information data*/
enum {
	INFO_DATA_CHECK = 0x00,
	INFO_DATA1 = 0x01,
	INFO_DATA2,
	INFO_DATA3,
	INFO_DATA4,
	INFO_DATA5,
	INFO_DATA6,
	INFO_DATA7,
	INFO_DATA8,
	INFO_DATA9,
	INFO_DATA10,
	INFO_DATA11,
	INFO_DATA12,
	INFO_DATA13
};

#define INFO_BUFFER 31
#define INFO_TYPE   0x00
#define INFO_VER    0x01
#define INFO_LENTH  0x02
#define INFO_CHECKSUM 0x03
#define INFO_VIC    (INFO_CHECKSUM + INFO_DATA4)

/*Color Space*/
#define BIT_DATA_SWING_CTL_MASK 0x07
#define BIT_CLK_SWING_CTL_MASK 0x38
#define BIT_MHLTX_CTL4_MHL_CLK_RATIO_MASK 0x40
#define BIT_MHLTX_CTL4_MHL_CLK_RATIO_2X 0x00
#define BIT_MHLTX_CTL4_MHL_CLK_RATIO_3X 0x40

#define BIT_TPI_INPUT_QUAN_RANGE_MASK       0x0C
#define BIT_TPI_INPUT_QUAN_RANGE_AUTO       0x00
#define BIT_TPI_INPUT_QUAN_RANGE_LIMITED    0x04
#define BIT_TPI_INPUT_QUAN_RANGE_FULL       0x08
#define BIT_TPI_INPUT_QUAN_RANGE_RSVD       0x0C

#define BIT_TPI_OUTPUT_QUAN_RANGE_MASK      0x0C
#define BIT_TPI_OUTPUT_QUAN_RANGE_AUTO      0x00
#define BIT_TPI_OUTPUT_QUAN_RANGE_LIMITED   0x04
#define BIT_TPI_OUTPUT_QUAN_RANGE_FULL      0x08
#define BIT_TPI_OUTPUT_QUAN_RANGE_RSVD      0x0C

#define BIT_TPI_INPUT_FORMAT_MASK           0x03
#define BIT_TPI_INPUT_FORMAT_RGB            0x00
#define BIT_TPI_INPUT_FORMAT_YCbCr444       0x01
#define BIT_TPI_INPUT_FORMAT_YCbCr422       0x02
#define BIT_TPI_INPUT_FORMAT_INTERNAL_RGB   0x03

#define BIT_TPI_OUTPUT_FORMAT_MASK          0x03
#define BIT_TPI_OUTPUT_FORMAT_HDMI_TO_RGB   0x00
#define BIT_TPI_OUTPUT_FORMAT_YCbCr444      0x01
#define BIT_TPI_OUTPUT_FORMAT_YCbCr422      0x02
#define BIT_TPI_OUTPUT_FORMAT_DVI_TO_RGB    0x03

#define BIT_MHLTX_CTL3_DAMPING_SEL_MASK     0x30
#define BIT_MHLTX_CTL3_DAMPING_SEL_OFF      0x30
#define BIT_MHLTX_CTL3_DAMPING_SEL_200_OHM  0x10
#define BIT_MHLTX_CTL3_DAMPING_SEL_300_OHM  0x20
#define BIT_MHLTX_CTL3_DAMPING_SEL_150_OHM   0x00

#define BIT_MHLTX_CTL1_TX_TERM_MODE_MASK   0xC0
#define BIT_MHLTX_CTL1_TX_TERM_MODE_100DIFF 0x00
#define BIT_MHLTX_CTL1_TX_TERM_MODE_150DIFF 0x40
#define BIT_MHLTX_CTL1_TX_TERM_MODE_300DIFF 0x80
#define BIT_MHLTX_CTL1_TX_TERM_MODE_OFF     0xC0

/*HDCP*/
#define AKSV_SIZE              5
#define NUM_OF_ONES_IN_KSV    20
#define TPI_HDCP_QUERY_DATA_REG 0x29
#define HDCP_CTRL 0x2A
#define HDCP_INTR 0x3D
#define HDCP_INTR_MASK 0x3C
#define HDCP_KEY 0x36

#define PROTECTION_TYPE_MASK 0x02
#define PROTECTION_TYPE_NONE 0x00
#define PROTECTION_TYPE_HDCP 0x02
#define HDCP_REPEATER_MASK 0x08
#define HDCP_REPEATER_NO 0x00
#define HDCP_REPEATER_YES 0x08


/*HDCP interrupt status reg*/
#define BIT_TPI_INTR_ST0_BKSV_DONE                     0x04
#define BIT_TPI_INTR_ST0_BKSV_ERR                      0x02
#define BIT_TPI_INTR_ST0_KSV_FIFO_FIRST                0x08
#define BIT_TPI_INTR_ST0_AUDIO_ERROR_EVENT             0x10
#define BIT_TPI_INTR_ST0_HDCP_SECURITY_CHANGE_EVENT    0x20
#define BIT_TPI_INTR_ST0_HDCP_VPRIME_VALUE_READY_EVENT 0x40
#define BIT_TPI_INTR_ST0_HDCP_AUTH_STATUS_CHANGE_EVENT 0x80

/*Link status*/
#define LINK_STATUS_MASK ((1<<5)|(1<<4))
#define LINK_STATUS_NORMAL (0x00)
#define LINK_STATUS_LINK_LOST (0x10)
#define LINK_STATUS_RENEGOTIATION_REQ (0x20)
#define LINK_STATUS_LINK_SUSPENDED (0x30)
#define EXTENDED_LINK_PROTECTION_MASK		(1<<7)
#define EXTENDED_LINK_PROTECTION_NONE		(0x00)
#define EXTENDED_LINK_PROTECTION_SECURE		(0x80)
#define LOCAL_LINK_PROTECTION_MASK			(1<<6)
#define LOCAL_LINK_PROTECTION_NONE			(0x00)
#define LOCAL_LINK_PROTECTION_SECURE		(0x40)

#define BIT_TMDS_CCTRL_BGRCTL_MASK  0x07
#define BIT_TMDS_CCTRL_SEL_BGR      0x08
#define BIT_TMDS_CCTRL_TMDS_OE      0x10
#define BIT_TMDS_CCTRL_CKDT_EN      0x20
/*Clock*/
#define BIT_TMDS_CSTAT_P3_PDO_MASK             0x01
#define BIT_TMDS_CSTAT_P3_PDO_CLOCK_DETECTED   0x00
#define BIT_TMDS_CSTAT_P3_PDO_CLOCK_STOPPED    0x01
#define BIT_TMDS_CSTAT_P3_SCDT                 0x02
#define BIT_TMDS_CSTAT_RPWR5V_STATUS		0x04
/* MSC_MSG Sub-Command data */
/*Turn content streaming ON.*/
#define	MHL_RAP_CONTENT_ON		0x10
/*Turn content streaming OFF.*/
#define	MHL_RAP_CONTENT_OFF		0x11
#ifdef SII8240_CHECK_MONITOR
/*check HDCP enable for DRM contents*/
#define _SCM_SVC_OEM    249
#define _SCM_OEM_CMD    1
__packed struct hdcp_auth_status {
	uint32_t a;
	void *b;
	void *c;
	uint32_t d;
};
#endif

/* need these to keep track of chip power state when we are using unpowered MHL
 * dongles/bridges and hence need to do some power-saving during suspend
 * (D0-->D2-->D3) or resume(D3-->D2-->D0).Also,some boards/devices are
 * designed to keep the chip dormant in D3 Mode after probe succeeds.
 */

enum mhl_attached_type {
	MHL_DETACHED,
	MHL_ATTACHED,
};


enum rgnd_state {
	RGND_UNKNOWN = 0,
	RGND_OPEN,
	RGND_1K,
	RGND_2K,
	RGND_SHORT,
};

enum mhl_state {
	STATE_DISCONNECTED = 0,
	STATE_MHL_READY_RGND_DETECT,
	STATE_CBUS_UNSTABLE,
	STATE_MHL_CONNECTED,
	STATE_NON_MHL_DETECTED,
	STATE_MHL_USB_CONNECTED,
	STATE_MHL_DISCOVERY_SUCCESS,
	STATE_MHL_DISCOVERY_FAIL,
	STATE_MHL_DISCOVERY_ON,
	STATE_ESTABLISHED,
	STATE_WAIT_FOR_DISCONNECTION,
};

enum {
	BIT_TPI_INFO_SEL_MASK		 = 0x07
	, BIT_TPI_INFO_SEL_AVI            = 0x00
	, BIT_TPI_INFO_SEL_SPD            = 0x01
	, BIT_TPI_INFO_SEL_Audio          = 0x02
	, BIT_TPI_INFO_SEL_MPEG           = 0x03
	, BIT_TPI_INFO_SEL_GENERIC        = 0x04
	, BIT_TPI_INFO_SEL_GENERIC2       = 0x05
	, BIT_TPI_INFO_READ_FLAG_MASK     = 0x20
	, BIT_TPI_INFO_READ_FLAG_NO_READ  = 0x00
	, BIT_TPI_INFO_READ_FLAG_READ     = 0x20
	, BIT_TPI_INFO_RPT                = 0x40
	, BIT_TPI_INFO_EN                 = 0x80
};

enum cbus_command {
	IDLE =			0x00,
	END_OF_FRAME =		0x32,
	ACK =			0x33,
	NACK =			0x34,
	ABORT =			0x35,
	WRITE_STAT =		0x60 | 0x80,
	SET_INT =		0x60,
	READ_DEVCAP =		0x61,
	GET_STATE =		0x62,
	GET_VENDOR_ID =		0x63,
	SET_HPD =		0x64,
	CLR_HPD =		0x65,
	SET_CAP_ID =		0x66,
	GET_CAP_ID =		0x67,
	MSC_MSG =		0x68,
	GET_SC1_ERR_CODE =	0x69,
	GET_DDC_ERR_CODE =	0x6A,
	GET_MSC_ERR_CODE =	0x6B,
	WRITE_BURST =		0x6C,
	GET_SC3_ERR_CODE =	0x6D,
};
enum msc_subcommand {
	/* MSC_MSG Sub-Command codes */
	MSG_RCP =	0x10,
	MSG_RCPK =	0x11,
	MSG_RCPE =	0x12,
	MSG_RAP =	0x20,
	MSG_RAPK =	0x21,
};

enum avi_cmd_type {
	AVI_CMD_NONE = 0x00,
	HPD_HIGH_EVENT = 0x01,
	HPD_LOW_EVENT,
	CEA_NO_AVI,
	CEA_NEW_AVI,
	AVI_CMD_MAX,
};

enum {
	BIT_RX_HDMI_CTRL2_VSI_MON_SEL_MASK             = 0x01
	, BIT_RX_HDMI_CTRL2_VSI_MON_SEL_AVI_INFOFRAME    = 0x00
	, BIT_RX_HDMI_CTRL2_VSI_MON_SEL_VS_INFOFRAME     = 0x01
	, BIT_RX_HDMI_CTRL2_USE_AV_MUTE_SUPPORT_MASK = 0x08
	, BIT_RX_HDMI_CTRL2_USE_AV_MUTE_SUPPORT_DISABLE = 0x00
	, BIT_RX_HDMI_CTRL2_USE_AV_MUTE_SUPPORT_ENABLE = 0x08
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_MASK       = 0xF0
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_0      = 0x00
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_1      = 0x10
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_2      = 0x20
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_3      = 0x30
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_4      = 0x40
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_5      = 0x50
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_6      = 0x60
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_7      = 0x70
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_8      = 0x80
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_9      = 0x90
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_A      = 0xA0
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_B      = 0xB0
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_C      = 0xC0
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_D      = 0xD0
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_E      = 0xE0
	, BIT_RX_HDMI_CTRL2_IDLE_CNT_3_0_VAL_F      = 0xF0
};

/*Pack pixel*/
enum {
	BIT_VID_OVRRD_3DCONV_EN_MASK = 0x10
	, BIT_VID_OVRRD_3DCONV_EN_NORMAL = 0x00
	, BIT_VID_OVRRD_3DCONV_EN_FRAME_PACK = 0x10
	, BIT_VID_MODE_m1080p_MASK = 0x40
	, BIT_VID_MODE_m1080p_DISABLE    = 0x00
	, BIT_VID_MODE_m1080p_ENABLE    = 0x40
};

/*HDCP*/
enum {
	BIT_TPI_HDCP_CONTROL_DATA_COPP_PROTLEVEL_MASK    = 0x01
	, BIT_TPI_HDCP_CONTROL_DATA_COPP_PROTLEVEL_MIN     = 0x00
	, BIT_TPI_HDCP_CONTROL_DATA_COPP_PROTLEVEL_MAX     = 0x01
	, BIT_TPI_HDCP_CONTROL_DATA_DOUBLE_RI_CHECK_MASK   = 0x04
	, BIT_TPI_HDCP_CONTROL_DATA_DOUBLE_RI_CHECK_DISABLE = 0x00
	, BIT_TPI_HDCP_CONTROL_DATA_DOUBLE_RI_CHECK_ENABLE = 0x04

};
enum {
	BIT_REG_RX_HDMI_CTRL0_rx_hdmi_hdmi_mode_MASK           = 0x01
	, BIT_REG_RX_HDMI_CTRL0_rx_hdmi_hdmi_mode_en_MASK        = 0x02
	, BIT_REG_RX_HDMI_CTRL0_hdmi_mode_overwrite_MASK         = 0x04
	, BIT_REG_RX_HDMI_CTRL0_hdmi_mode_overwrite_HW_CTRL      = 0x00
	, BIT_REG_RX_HDMI_CTRL0_hdmi_mode_overwrite_SW_CTRL      = 0x04
	, BIT_REG_RX_HDMI_CTRL0_hdmi_mode_sw_value_MASK          = 0x08
	, BIT_REG_RX_HDMI_CTRL0_hdmi_mode_sw_value_DVI           = 0x00
	, BIT_REG_RX_HDMI_CTRL0_hdmi_mode_sw_value_HDMI          = 0x08
	, BIT_REG_RX_HDMI_CTRL0_ri_hdmi_mode_en_itself_clr_MASK  = 0x10
	, BIT_REG_RX_HDMI_CTRL0_byp_dvifilt_sync_MASK            = 0x20
};
enum {
	BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_EN_MASK    = 0x01
	, BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_EN_STALE   = 0x00
	, BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_EN_CLEAR   = 0x01
	, BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_W_AVI_EN_MASK    = 0x10
	, BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_W_AVI_EN_STALE   = 0x00
	, BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_W_AVI_EN_CLEAR   = 0x10
};
#define VIDEO_CAPABILITY_D_BLOCK 0x00
#define MAX_V_DESCRIPTORS 20
#define MAX_A_DESCRIPTORS 10
#define MAX_SPEAKER_CONFIGURATIONS 4
#define AUDIO_DESCR_SIZE 3
#define HDMI_PATTERN_GEN	0
#define MHL_3D_VIC_CODE 0x0010
#define MHL_3D_DTD_CODE 0x0011

enum {
	NON_3D_VIDEO = 0x00
	, NON_FRAME_PACKING_3D = 0x01
	, FRAME_PACKING_3D = 0x02
};
enum {
	B_ID_H = 0x00,
	B_ID_L = 0x01,
	CHECK_SUM = 0x02,
	TOT_ENT = 0x03,
	SEQ = 0x04,
	NUM_ENT = 0x05,
	VDI_0_H = 0x06,
	VDI_0_L = 0x07,
	VDI_1_H = 0x08,
	VDI_1_L = 0x09,
	VDI_2_H = 0x0A,
	VDI_2_L = 0x0B,
	VDI_3_H = 0x0C,
	VDI_3_L = 0x0D,
	VDI_4_H = 0x0E,
	VDI_4_L = 0x0F
};
struct cbus_data {
	enum cbus_command cmd;		/* cbus command type */
	u8 offset;			/* for MSC_MSG,stores msc_subcommand */
	u8 data;
	struct completion complete;
	bool use_completion;
	int *ret;			/* optional return value */
	struct list_head list;
};

struct sii8240_intr_regs {
	u8 disc_intr;           /* Discovery & Connection interrupts */
	u8 msc_intr;            /* CBUS: MSC transaction interrupts */
	u8 spad_intr[2];        /* CBUS: scratchpad interrupts */
	u8 msc_error_intr;      /* CBUS: MSC transaction error interrupts */
	u8 hdcp_intr;           /* TPI: HDCP interrupts */
	u8 upstrm_intr[2];      /* Miscellanous: Upstream Interrupts */
};

struct sii8240_intr_regs_mask {
	u8 intr1_mask_value;
	u8 intr2_mask_value;
	u8 intr3_mask_value;
	u8 intr4_mask_value;
	u8 intr5_mask_value;
	u8 intr7_mask_value;
	u8 intr8_mask_value;
	u8 intr_cbus0_mask_value;
	u8 intr_cbus1_mask_value;
	u8 intr_tpi_mask_value;
};

struct sii8240_regs_cache {
	u8                                      link_mode;
	u8                                      host_devcap[DEVCAP_COUNT_MAX];
	u8                                      peer_devcap[DEVCAP_COUNT_MAX];
	struct sii8240_intr_regs                intrs;
	struct sii8240_intr_regs_mask           intr_masks;
};
struct mhl_3D_VDI {
	u8 reservedHigh;
	u8 FrameSequential:1;
	u8 TopBottom:1;
	u8 LeftRight:1;
	u8 reservedLow:5;
};
struct mhl_3D_data {
	struct mhl_3D_VDI vdi[50];
	u8 tot_ent;
	u8 num_ent;
};

struct mhl_timing {
	u8 avi_infoframe[HDMI_VFRMT_MAX];
	u8 d_format[HDMI_VFRMT_MAX];
};

struct sii8240_data;
#ifdef CONFIG_EXTCON
struct sec_mhl_cable {
	struct work_struct work;
	struct notifier_block nb;
	struct extcon_specific_cable_nb extcon_nb;
	struct extcon_dev *edev;
	enum extcon_cable_name cable_type;
	unsigned long cable_state;
};
#endif

struct sii8240_data {
	struct sii8240_platform_data            *pdata;
	/* single device entry-point,e.g. /dev/mhl; all the factory-related
	* sysfs entries,debugfs and class-creation etc should be
	* done on this device */
	struct device                           mhl_dev;
	/* Notifier block for registering MHL callback function.The callback
	* function will serve as entry-point into the MHL driver from Board
	* file or connector drivers(USB-type, 30-pin or 11-pin) */
#ifndef CONFIG_EXTCON
	struct notifier_block                   mhl_nb;
#endif
	struct mutex                            lock;
	struct mutex                            msc_lock;
	struct mutex				restart_lock;
	struct sii8240_regs_cache               regs;
	wait_queue_head_t                       wq;
	int                                     irq;

	enum mhl_state                          state;
	enum rgnd_state                         rgnd;
	enum mhl_attached_type                  muic_state;

	bool                                    cbus_ready;
	struct mutex                            cbus_lock;
	struct completion                       cbus_complete;

	struct work_struct                      cbus_work;
	struct work_struct            		mhl_power_on;
	struct work_struct				cbus_cmd_work;
	struct work_struct				avi_control_work;
	struct work_struct				redetect_work;
#ifdef SII8240_CHECK_MONITOR
	struct work_struct				mhl_link_monitor_work;
	bool ckdt_stable;
#endif
#ifdef SFEATURE_UNSTABLE_SOURCE_WA
	struct work_struct				avi_check_work;
#endif

	struct workqueue_struct		*avi_cmd_wqs;
	struct workqueue_struct			*cbus_cmd_wqs;
	struct workqueue_struct		*mhl_link_monitor_wq;
	struct workqueue_struct		*mhl_detection_workqueue;
	struct list_head                        cbus_data_list;

	/*mhl tx configuration*/
	u8 aviInfoFrame[INFO_BUFFER];
	u8 current_aviInfoFrame[INFO_BUFFER];
	u8 output_avi_data[SIZE_AVI_INFOFRAME];
	u8 vendorSpecificInfoFrame[INFO_BUFFER];
	u8 input_3d_format;
	u8 edid[EDID_MAX_LENGTH];

	bool hdmi_sink;
	bool hdmi_mode;
	bool avi_work;
	enum avi_cmd_type avi_cmd;
	struct mhl_3D_data vic_data;
	struct mhl_3D_data dtd_data;
	struct mhl_timing support_mhl_timing;

	bool rcp_support;
	bool rap_support;
	bool sp_support;
	bool hpd_status;
	bool cbus_abort;

	bool mhl_rgnd;
	bool cbus_connected;
	bool irq_enabled;
	bool mhl_connected;
	u8 connected_ready;

	bool hdcp_support;
	struct input_dev                        *input_dev;
	struct mutex                            input_lock;
	u16                                     keycode[SII8240_RCP_NUM_KEYS];

#ifdef SFEATURE_UNSTABLE_SOURCE_WA
	u8				r281;
	struct timer_list		avi_check_timer;
#endif

	struct switch_dev		mhl_event_switch;
	struct timer_list		mhl_timer;
        struct wake_lock                mhl_wake_lock;

	bool				tmds_enable;
	bool				ap_hdcp_success;
	void (*mhl_ddc_bypass)(bool bypass_on);
};

uint8_t *sii8240_get_mhl_edid(void);
u8 sii8240_mhl_get_version(void);
u8 sii8240_support_packedpixel(void);
#endif /* __SII8240_H__ */
