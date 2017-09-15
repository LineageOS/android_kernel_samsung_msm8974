/*
 * es325.c  --  Audience eS325 ALSA SoC Audio driver
 *
 * Copyright 2011 Audience, Inc.
 *
 * Author: Greg Clemson <gclemson@audience.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *       1.05    Genisim Tsilker <gtsilker@audience.com>
 *                   -  Combine SLIMBus Request - Response (Read - Write) 
 *                      in es325_request_response function.
 *                   - Flexible waiting time for es325 response (up to 20 ms).
 *                   - Flexible waiting time for es325 sleep / wakeup (up to 20 ms).
 *                   - In case of sequential "slim_control_ch" requests only last one
 *                     commits "slim_control_ch" (SLIMBus reconfiguration).
 *                   - Add support for DTS (Device Tree Source)
 *                   - Remove unused comments.
 *                   - Remove code related to i2c / i2s supports.
 */
#define DEBUG

#define BUS_TRANSACTIONS
/* Enable FIXED_CONFIG when eS325 is using for Play Back mode */
/* #define FIXED_CONFIG */
#define ES325_SLEEP /* To support sleep/wakeup for ES325 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/completion.h>
#include <linux/slimbus/slimbus.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/kthread.h>
#include <asm/system_info.h>

#ifdef ES325_SLEEP
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/slimbus/slimbus.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#endif

#include <linux/vmalloc.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <generated/utsrelease.h>
#include <linux/esxxx.h> /* TODO: common location for i2c and slimbus */
#include "es325.h"
#include "es325-export.h"
#include <linux/clk.h>
#include <linux/of_gpio.h>

#define ES325_CMD_ACCESS_WR_MAX 4
#define ES325_CMD_ACCESS_RD_MAX 4
struct es325_cmd_access {
	u8 read_msg[ES325_CMD_ACCESS_RD_MAX];
	unsigned int read_msg_len;
	u8 write_msg[ES325_CMD_ACCESS_WR_MAX];
	unsigned int write_msg_len;
	unsigned int val_shift;
	unsigned int val_max;
};

#include "es325-access.h"

#define ES325_SLIM_RX_PORTS	6
#define ES325_SLIM_TX_PORTS	6

#define ES325_SLIM_1_PB		0
#define ES325_SLIM_1_CAP	1
#define ES325_SLIM_2_PB		2
#define ES325_SLIM_2_CAP	3
#define ES325_SLIM_3_PB		4
#define ES325_SLIM_3_CAP	5

#define ES325_SLIM_1_PB_MAX_CHANS	2
#define ES325_SLIM_1_CAP_MAX_CHANS	2
#define ES325_SLIM_2_PB_MAX_CHANS	2
#define ES325_SLIM_2_CAP_MAX_CHANS	2
#define ES325_SLIM_3_PB_MAX_CHANS	2
#define ES325_SLIM_3_CAP_MAX_CHANS	2

#define ES325_SLIM_1_PB_OFFSET	0
#define ES325_SLIM_2_PB_OFFSET	2
#define ES325_SLIM_3_PB_OFFSET	4
#define ES325_SLIM_1_CAP_OFFSET	0
#define ES325_SLIM_2_CAP_OFFSET	2
#define ES325_SLIM_3_CAP_OFFSET	4

#define NARROW_BAND	0
#define WIDE_BAND	1
#define NETWORK_OFFSET	21

#define ES325_NUM_CODEC_SLIM_DAIS	6

int debug_for_dl_firmware = 0;

enum {
	BOOT_OK = 0,
	BOOT_MSG_ERR,
	BOOT_MSG_NACK,
	SYNC_MSG_NACK
};

struct es325_slim_dai_data {
	unsigned int rate;
	unsigned int *ch_num;
	unsigned int ch_act;
	unsigned int ch_tot;
#ifdef ES325_SLEEP
	unsigned int ch_wakeup;
#endif
};

struct es325_slim_ch {
	u32	sph;
	u32	ch_num;
	u16	ch_h;
	u16	grph;
};

#define		ID(id)		(id)

#define ES325_MAX_INVALID_VEQ 0xFFFF
#define ES325_MAX_INVALID_BWE 0x0000
#define ES325_MAX_INVALID_TX_NS 0xFFFF

static unsigned int es325_VEQ_enable = ES325_MAX_INVALID_VEQ;
static unsigned int es325_VEQ_enable_new = ES325_MAX_INVALID_VEQ;
static unsigned int es325_BWE_enable = ES325_MAX_INVALID_BWE;
static unsigned int es325_BWE_enable_new = ES325_MAX_INVALID_BWE;
static unsigned int es325_Tx_NS = ES325_MAX_INVALID_TX_NS;
static unsigned int es325_Tx_NS_new = ES325_MAX_INVALID_TX_NS;
#ifdef CONFIG_ARCH_MSM8226
static struct regulator* es325_ldo;
#endif

#if defined(CONFIG_SND_SOC_ES325_ATLANTIC)
extern void msm_slim_vote_func(struct slim_device *gen0_client);
#endif

enum es325_power_stage {
	ES325_POWER_BOOT,			/* no firmware loaded */
	ES325_POWER_SLEEP,			/* chip is sleeping */
	ES325_POWER_SLEEP_PENDING,	/* sleep requested */
	ES325_POWER_AWAKE			/* chip is powered */
};

/* codec private data */
struct es325_priv {
	struct snd_soc_codec *codec;
	struct firmware *fw;

	struct esxxx_platform_data *pdata;

	struct slim_device *intf_client;
	struct slim_device *gen0_client;
	struct es325_slim_dai_data dai[ES325_NUM_CODEC_SLIM_DAIS];
	struct es325_slim_ch slim_rx[ES325_SLIM_RX_PORTS];
	struct es325_slim_ch slim_tx[ES325_SLIM_TX_PORTS];
	struct mutex wakeup_mutex;
#ifdef ES325_SLEEP
	struct mutex pm_mutex; /* Mutex for protecting data structure */
	int wakeup_cnt;			/* sleep and wakeup count, when reached to 0, 
					   start sleep timer */
	int clock_on; /* es325 clock status */
	int internal_route_config; /* last configured route config */
	int new_internal_route_config; /* new received route config via KControl,
						it is written when hw param is received */
	int wq_active; /* sleep timer status */
#endif
	enum es325_power_stage power_stage;
}es325_priv = {
	.codec = NULL,
	.fw = NULL,
	.pdata = NULL,
	.intf_client = NULL,
	.gen0_client = NULL,
	.power_stage = ES325_POWER_BOOT
};
static unsigned int es325_ap_tx1_ch_cnt = 1;
unsigned int es325_tx1_route_enable;
unsigned int es325_rx1_route_enable;
unsigned int es325_rx2_route_enable;
unsigned int es325_fw_downloaded = 0;
static unsigned int uart_enable = 0;

#if defined(CONFIG_WCD9306_CODEC)

#define SMB_RX_PORT0 152
#define SMB_RX_PORT1 153
#define SMB_RX_PORT2 154
#define SMB_RX_PORT3 155
#define SMB_RX_PORT4 128
#define SMB_RX_PORT5 129

#define SMB_TX_PORT0 156
#define SMB_TX_PORT1 157
#define SMB_TX_PORT2 144
#define SMB_TX_PORT3 145
#define SMB_TX_PORT4 144
#define SMB_TX_PORT5 145

#elif defined(CONFIG_WCD9310_CODEC)

#define SMB_RX_PORT0 152
#define SMB_RX_PORT1 153
#define SMB_RX_PORT2 154
#define SMB_RX_PORT3 155
#define SMB_RX_PORT4 134
#define SMB_RX_PORT5 135

#define SMB_TX_PORT0 156
#define SMB_TX_PORT1 157
#define SMB_TX_PORT2 138
#define SMB_TX_PORT3 139
#define SMB_TX_PORT4 143
#define SMB_TX_PORT5 144

#elif defined(CONFIG_WCD9320_CODEC)

#define SMB_RX_PORT0 152
#define SMB_RX_PORT1 153
#define SMB_RX_PORT2 154
#define SMB_RX_PORT3 155
#define SMB_RX_PORT4 134
#define SMB_RX_PORT5 135

#define SMB_TX_PORT0 156
#define SMB_TX_PORT1 157
#define SMB_TX_PORT2 144
#define SMB_TX_PORT3 145
#define SMB_TX_PORT4 144
#define SMB_TX_PORT5 145

#else

#define SMB_RX_PORT0 152
#define SMB_RX_PORT1 153
#define SMB_RX_PORT2 154
#define SMB_RX_PORT3 155
#define SMB_RX_PORT4 134
#define SMB_RX_PORT5 135

#define SMB_TX_PORT0 156
#define SMB_TX_PORT1 157
#define SMB_TX_PORT2 144
#define SMB_TX_PORT3 145
#define SMB_TX_PORT4 144
#define SMB_TX_PORT5 145

#endif

static int es325_slim_rx_port_to_ch[ES325_SLIM_RX_PORTS] = {
	SMB_RX_PORT0,
	SMB_RX_PORT1,
	SMB_RX_PORT2,
	SMB_RX_PORT3,
	SMB_RX_PORT4,
	SMB_RX_PORT5
};

static int es325_slim_tx_port_to_ch[ES325_SLIM_TX_PORTS] = {
	SMB_TX_PORT0,
	SMB_TX_PORT1,
	SMB_TX_PORT2,
	SMB_TX_PORT3,
	SMB_TX_PORT4,
	SMB_TX_PORT5
};

static int es325_slim_be_id[ES325_NUM_CODEC_SLIM_DAIS] = {
	ES325_SLIM_2_CAP, /* for ES325_SLIM_1_PB tx from es325 */
	ES325_SLIM_3_PB, /* for ES325_SLIM_1_CAP rx to es325 */
	ES325_SLIM_3_CAP, /* for ES325_SLIM_2_PB tx from es325 */
	-1, /* for ES325_SLIM_2_CAP */
	-1, /* for ES325_SLIM_3_PB */
	-1, /* for ES325_SLIM_3_CAP */
};

#define ES325_INTERNAL_ROUTE_MAX 26
static long es325_internal_route_num;
static int es325_network_type = NARROW_BAND;

/* Audio playback, 1 channel */
static u8 es325_internal_route_audio_playback_1ch[10] = {
	0x90, 0x31, 0x00, 0x07, /* 1 Ch playback 4 Ch pass */
	0x90, 0x31, 0x00, 0x46, /* Algo Preset for 1 Ch playback 4 Ch pass */
	0xff		/* terminate */
};

/* Audio playback, 2 channels */
static u8 es325_internal_route_audio_playback_2ch[10] = {
	0x90, 0x31, 0x00, 0x08, /* 2 Ch playback 4 Ch pass */
	0x90, 0x31, 0x00, 0x50, /* Algo Preset for 2 Ch playback 4 Ch pass */
	0xff		/* terminate */
};

/* 1-mic Headset NB (SW bypss)*/
static u8 es325_internal_route_1mic_headset[10] = {
	0x90, 0x31, 0x00, 0x01, /* 1 Mic 2 FEOUT MD */
	0x90, 0x31, 0x00, 0x84, /* Algo Preset: 1-mic CT NB */
	0xff		/* terminate */
};

/* 1-mic Headset WB (SW bypss)*/
static u8 es325_internal_route_1mic_headset_WB[10] = {
	0x90, 0x31, 0x00, 0x01, /* 1 Mic 2 FEOUT MD */
	0x90, 0x31, 0x00, 0x85, /* Algo Preset: 1-mic CT WB */
	0xff		/* terminate */
};
/* 2-mic Speaker NB (2-mic FT)(NS on) */
static u8 es325_internal_route_2mic_speaker[10] = {
	0x90, 0x31, 0x00, 0x02, /* 2 Mic 1 FEOUT CT */
	0x90, 0x31, 0x00, 0x16, /* Algo Preset for 2 Mic FT NB */
	0xff		/* terminate */
};

/* 2-mic Speaker WB (2-mic FT)(NS off) */
static u8 es325_internal_route_2mic_speaker_WB[10] = {
	0x90, 0x31, 0x00, 0x02, /* 2 Mic 1 FEOUT CT */
	0x90, 0x31, 0x00, 0x17, /* Algo Preset for 2 Mic FT WB */
	0xff		/* terminate */
};

/* 1-mic Handset NB (1-mic CT)(NS off) */
static u8 es325_internal_route_1mic_handset[10] = {
	0x90, 0x31, 0x00, 0x02, /* 2 Mic 1 FEOUT CT */
	0x90, 0x31, 0x00, 0x18, /* Algo Preset for 2 Mic CT NB */
	0xff		/* terminate */
};

/* 1-mic Handset WB (1-mic CT)(NS off) */
static u8 es325_internal_route_1mic_handset_WB[10] = {
	0x90, 0x31, 0x00, 0x02, /* 2 Mic 1 FEOUT CT */
	0x90, 0x31, 0x00, 0x19, /* Algo Preset for 2 Mic CT WB */
	0xff		/* terminate */
};

/* 2-mic Handset NB (2-mic CT)(NS on) */
static u8 es325_internal_route_2mic_handset[10] = {
	0x90, 0x31, 0x00, 0x02, /* 2 Mic 1 FEOUT CT */
	0x90, 0x31, 0x00, 0x14, /* Algo Preset for 2 Mic CT NB */
	0xff		/* terminate */
};

/* 2-mic Handset WB (2-mic CT)(NS on) */
static u8 es325_internal_route_2mic_handset_WB[10] = {
	0x90, 0x31, 0x00, 0x02, /* 2 Mic 1 FEOUT CT */
	0x90, 0x31, 0x00, 0x15, /* Algo Preset for 2 Mic CT WB */
	0xff		/* terminate */
};

/* 1-mic 1-output for Loopback (CT SW bypss) */
static u8 es325_internal_route_rcv_loopback[10] = {
	0x90, 0x31, 0x00, 0x03, /* 1 Mic 1 FEOUT */
	0x90, 0x31, 0x00, 0x83, /* Algo Preset: 1-mic CT WB */
	0xff		/* terminate */
};

/* 1-mic 1-output for Loopback (FT SW bypss) */
static u8 es325_internal_route_spk_loopback[10] = {
	0x90, 0x31, 0x00, 0x03, /* 1 Mic 1 FEOUT */
	0x90, 0x31, 0x00, 0x83, /* Algo Preset: 1-mic FT WB */
	0xff		/* terminate */
};

/* 1-mic 2-output for Loopback (SW bypss) */
static u8 es325_internal_route_headset_loopback[10] = {
	0x90, 0x31, 0x00, 0x01, /* 1 Mic 2 FEOUT MD */
	0x90, 0x31, 0x00, 0x85, /* Algo Preset: 1-mic WB */
	0xff		/* terminate */
};

/* 2-mic ASR */
static u8 es325_internal_route_2mic_ASR[10] = {
	0x90, 0x31, 0x00, 0x6E, /* 2 Mic ASR WB */
	0x90, 0x31, 0x00, 0x70, /* Algo Preset for 2 Mic ASR WB */
	0xff        /* terminate */
};

/* TTY HCO - Rx:Handset(1ch), Tx: Main mic(1ch)(SW bypass)*/
static u8 es325_internal_route_TTY_HCO[10] = {
	0x90, 0x31, 0x00, 0x0d, /* 1 Mic 1 FEOUT */
	0x90, 0x31, 0x00, 0x84, /* Algo Preset: 1-mic CT NB */
	0xff		/* terminate */
};

/* TTY VCO - Rx:Headset(2ch), Tx: Main mic(1ch)(SW bypass)*/
static u8 es325_internal_route_TTY_VCO[10] = {
	0x90, 0x31, 0x00, 0x01, /* 1 Mic 2 FEOUT MD */
	0x90, 0x31, 0x00, 0x84, /* Algo Preset: 1-mic CT NB */
	0xff		/* terminate */
};

/* Audio playback, 1 channel */
static u8 dummy[10] = {
	0x90, 0x31, 0x00, 0x07, /* 1 Ch playback 4 Ch pass */
	0x90, 0x31, 0x00, 0x46, /* Algo Preset for 1 Ch playback 4 Ch pass */
	0xff		/* terminate */
};

static u8 es325_internal_route_interview[10] = {
	0x90, 0x31, 0x00, 0x6E,
	0x90, 0x31, 0x00, 0x41, /* 1 Mic 2 FEOUT MD */
	0xff		/* terminate */
};

static u8 es325_internal_route_conversation[10] = {
	0x90, 0x31, 0x00, 0x6E,
	0x90, 0x31, 0x00, 0x42, /* 1 Mic 2 FEOUT MD */
	0xff		/* terminate */
};

/* 2-mic Speaker NB (2-mic FT)(NS OFF) */
static u8 es325_internal_route_2mic_speaker_nsoff[10] = {
	0x90, 0x31, 0x00, 0x02, /* 2 Mic 1 FEOUT CT */
	0x90, 0x31, 0x00, 0x82, /* Algo Preset for 2 Mic FT NB */
	0xff		/* terminate */
};

/* 2-mic Speaker WB (2-mic FT)(NS OFF) */
static u8 es325_internal_route_2mic_speaker_WB_nsoff[10] = {
	0x90, 0x31, 0x00, 0x02, /* 2 Mic 1 FEOUT CT */
	0x90, 0x31, 0x00, 0x83, /* Algo Preset for 2 Mic FT NB */
	0xff		/* terminate */
};

static u8* es325_internal_route_configs[ES325_INTERNAL_ROUTE_MAX] = {
	es325_internal_route_1mic_headset,			/* [0]: 1-mic Headset NB (SW bypss) */
	es325_internal_route_2mic_speaker,			/* [1]: 2-mic Speaker NB (2-mic FT)(NS on)*/
	es325_internal_route_2mic_handset,			/* [2]: 2-mic Handset NB (2-mic CT)(NS on) */
	es325_internal_route_2mic_speaker_nsoff,	/* [3]: 2-mic Speaker NB (2-mic FT)(NS off) */
	es325_internal_route_1mic_handset,			/* [4]: 1-mic Handset NB (1-mic CT)(NS off) */
	es325_internal_route_audio_playback_1ch,	/* [5]: Audio playback, 1 channel */
	es325_internal_route_audio_playback_2ch,	/* [6]: Audio playback, 2 channels */
	es325_internal_route_interview,				/* [7]: TBD */
	es325_internal_route_conversation,			/* [8]: TBD */
	dummy,										/* [9]: TBD */
	dummy,										/* [10]: TBD */
	es325_internal_route_TTY_VCO,				/* [11]: TTY VCO - Rx:Headset(2ch), Tx: Main mic(1ch)(SW bypass) */
	es325_internal_route_TTY_HCO,				/* [12]: TTY HCO - Rx:Handset(1ch), Tx: Main mic(1ch)(SW bypass) */
	es325_internal_route_2mic_ASR,				/* [13]: 2-mic ASR */
	dummy,										/* [14]: TBD */
	dummy,										/* [15]: TBD */
	dummy,										/* [16]: TBD */
	es325_internal_route_rcv_loopback,			/* [17]: 1-mic 1-output for Loopback (CT SW bypss) */
	es325_internal_route_spk_loopback,			/* [18]: 1-mic 1-output for Loopback (FT SW bypss) */
	es325_internal_route_headset_loopback,		/* [19]: 1-mic 2-output for Loopback (SW bypss) */
	dummy,										/* [20]: TBD */
	es325_internal_route_1mic_headset_WB,		/* [21]: 1-mic Headset WB (SW bypss) */
	es325_internal_route_2mic_speaker_WB,		/* [22]: 2-mic Speaker WB (2-mic FT)(NS on) */
	es325_internal_route_2mic_handset_WB,		/* [23]: 2-mic Handset WB (2-mic CT)(NS on) */
	es325_internal_route_2mic_speaker_WB_nsoff,	/* [24]: 2-mic Speaker WB (2-mic FT)(NS off) */
	es325_internal_route_1mic_handset_WB,		/* [25]: 1-mic Handset WB (1-mic CT)(NS off) */
};

#ifdef NOT_USED_CONFIG
	/* [3]: 1-mic VOIP (CT) */
	{
		0x90, 0x31, 0x00, 0x03, /* 1 Mic 1 FEOUT */
		0x90, 0x31, 0x00, 0x1E, /* Algo Preset for 1 Mic VOIP DV SWB */
		0xff		/* terminate */
	},

	/* [4]: 2-mic VOIP (CT) */
	{
		0x90, 0x31, 0x00, 0x04, /* 2 Mic 1 FEOUT */
		0x90, 0x31, 0x00, 0x29, /* Algo Preset for 2 Mic VOIP CT WB */
		0xff		/* terminate */
	},

	/* [5]: Audio playback, 1 channel */
	{
		0x90, 0x31, 0x00, 0x07, /* 1 Ch playback 4 Ch pass */
		0x90, 0x31, 0x00, 0x46, /* Algo Preset for 1 Ch playback 4 Ch pass */
		0xff		/* terminate */
	},

	/* [7]: Audio record, 1 channel */
	{
		0x90, 0x31, 0x00, 0x09, /* 1 Ch Record 4 Ch pass */
		0x90, 0x31, 0x00, 0x5A, /* Algo Preset for 1 Ch Record 4 Ch pass */
		0xff		/* terminate */
	},

	/* [8]: Audio record, 2 channels */
	{
		0x90, 0x31, 0x00, 0x65, /* 2 Ch Record 4 Ch pass */
		0x90, 0x31, 0x00, 0x64, /* Algo Preset for 2 Ch Record 4 Ch pass */
		0xff		/* terminate */
	},

	/* [9]: 2-mic CT ASR */
	{
		0x90, 0x31, 0x00, 0x06, /* 2 Mic ASR */
		0x90, 0x31, 0x00, 0x3C, /* Algo Preset for 2 Mic ASR NB */
		0xff		/* terminate */
	},

	/* [10]: 1-mic External ASR */
	{
		0x90, 0x31, 0x00, 0x05, /* 1 Mic ASR */
		0x90, 0x31, 0x00, 0x32, /* Algo Preset for 1 Mic ASR NB */
		0xff		/* terminate */
	},

	/* [11]: VOIP Headset(CT) */
	{
		0x90, 0x31, 0x00, 0x01, /* 1 Mic Headset */
		0x90, 0x31, 0x00, 0x0b, /* Algo Preset for WB */
		0xff                    /* terminate */
	},

	/* [14]: 1-mic DV ASR WB */
	{
		0x90, 0x31, 0x00, 0x71, /* 1 Mic DV ASR WB */
		0x90, 0x31, 0x00, 0x73, /* Algo Preset for 1 Mic ASR DV WB */
		0xff                        /* terminate */
	},

	/* [15]: 1-mic FT */
	{
		0x90, 0x31, 0x00, 0x0c, /* 1 Mic FT */
		0x90, 0x31, 0x00, 0x16, /* Algo Preset for 2 Mic FT */
					/* TODO Switch to Algo Preset for 1 Mic FT after testing */
		0xff                        /* terminate */
	},

	/* [16]: 1-mic FT WB */
	{
		0x90, 0x31, 0x00, 0x0c, /* 1 Mic FT WB */
		0x90, 0x31, 0x00, 0x17, /* Algo Preset for 2 Mic FT WB */
					/* TODO Switch to Algo Preset for 1 Mic FT after testing */
		0xff                        /* terminate */
	},
#endif

static struct snd_soc_dai_driver es325_dai[];

#ifdef ES325_SLEEP
void es325_wrapper_sleep_internal(struct work_struct *dummy);
#endif

#ifdef FIXED_CONFIG
static void es325_fixed_config(struct es325_priv *es325);
#endif
static void es325_alloc_slim_rx_chan(struct slim_device *sbdev);
static void es325_alloc_slim_tx_chan(struct slim_device *sbdev);
static int es325_cfg_slim_rx(struct slim_device *sbdev, unsigned int *ch_num,
				unsigned int ch_cnt, unsigned int rate, bool commit);
static int es325_cfg_slim_tx(struct slim_device *sbdev, unsigned int *ch_num,
				unsigned int ch_cnt, unsigned int rate, bool commit);
static int es325_close_slim_rx(struct slim_device *sbdev, unsigned int *ch_num,
				unsigned int ch_cnt);
static int es325_close_slim_tx(struct slim_device *sbdev, unsigned int *ch_num,
				unsigned int ch_cnt);
static int es325_rx_ch_num_to_idx(int ch_num);
static int es325_tx_ch_num_to_idx(int ch_num);
static void es325_update_VEQ_enable(void);

static struct platform_device msm_es325_mclk_dev = {
	.name = "es325_mclk_dev_pdev",
	.id = -1,
	.dev = {
		.init_name = "es325_mclk_dev",
	},
};

static int es325_enable_ext_clk(int enable)
{
	int r = 0;
	static struct clk *es325_codec_clk;
	pr_info("%s():enable=%d\n", __func__, enable);

	if (!es325_codec_clk) {
		es325_codec_clk = clk_get(&msm_es325_mclk_dev.dev, "osr_clk");
	}

	if (enable) {
		clk_prepare_enable(es325_codec_clk);
	} else {
		clk_disable_unprepare(es325_codec_clk);
		clk_put(es325_codec_clk);
		es325_codec_clk = NULL;
	}

	return r;
}

static int es325_rx_ch_num_to_idx(int ch_num)
{
	int i;
	int idx = -1;

	for (i = 0; i < ES325_SLIM_RX_PORTS; i++) {
		if (ch_num == es325_slim_rx_port_to_ch[i]) {
			idx = i;
			break;
		}
	}

	return idx;
}

static int es325_tx_ch_num_to_idx(int ch_num)
{
	int i;
	int idx = -1;

	for (i = 0; i < ES325_SLIM_TX_PORTS; i++) {
		if (ch_num == es325_slim_tx_port_to_ch[i]) {
			idx = i;
			break;
		}
	}
	return idx;
}

/* es325 -> codec - alsa playback function */
static int es325_codec_cfg_slim_tx(struct es325_priv *es325, int dai_id, bool commit)
{
	int rc;
	/* start slim channels associated with id */
	rc = es325_cfg_slim_tx(es325->gen0_client, es325->dai[ID(dai_id)].ch_num,
			es325->dai[ID(dai_id)].ch_tot, es325->dai[ID(dai_id)].rate, commit);
	return rc;
}

/* es325 <- codec - alsa capture function */
static int es325_codec_cfg_slim_rx(struct es325_priv *es325, int dai_id, bool commit)
{
	int rc;
	/* start slim channels associated with id */
	rc = es325_cfg_slim_rx(es325->gen0_client, es325->dai[ID(dai_id)].ch_num,
			es325->dai[ID(dai_id)].ch_tot, es325->dai[ID(dai_id)].rate, commit);
	return rc;
}

/* es325 -> codec - alsa playback function */
static int es325_codec_close_slim_tx(struct es325_priv *es325, int dai_id)
{
	int rc;
	/* close slim channels associated with id */
	rc = es325_close_slim_tx(es325->gen0_client,
			es325->dai[ID(dai_id)].ch_num, es325->dai[ID(dai_id)].ch_tot);
	return rc;
}

/* es325 <- codec - alsa capture function */
static int es325_codec_close_slim_rx(struct es325_priv *es325, int dai_id)
{
	int rc;
	/* close slim channels associated with id */
	rc = es325_close_slim_rx(es325->gen0_client,
			es325->dai[ID(dai_id)].ch_num, es325->dai[ID(dai_id)].ch_tot);
	return rc;
}

static void es325_alloc_slim_rx_chan(struct slim_device *sbdev)
{
	struct es325_priv *es325_priv = slim_get_devicedata(sbdev);
	struct es325_slim_ch *rx = es325_priv->slim_rx;
	int i;
	int port_id;

	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	for (i = 0; i < ES325_SLIM_RX_PORTS; i++) {
		port_id = i;
		rx[i].ch_num = es325_slim_rx_port_to_ch[i];
		slim_get_slaveport(sbdev->laddr, port_id, &rx[i].sph, SLIM_SINK);
		slim_query_ch(sbdev, rx[i].ch_num, &rx[i].ch_h);
		dev_dbg(&sbdev->dev,
			"%s():port_id = %d, ch_num = %d, sph = 0x%08x\n",
			__func__, port_id, rx[i].ch_num, rx[i].sph);
	}
}

static void es325_alloc_slim_tx_chan(struct slim_device *sbdev)
{
	struct es325_priv *es325_priv = slim_get_devicedata(sbdev);
	struct es325_slim_ch *tx = es325_priv->slim_tx;
	int i;
	int port_id;

	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	for (i = 0; i < ES325_SLIM_TX_PORTS; i++) {
		port_id = i + 10; /* ES325_SLIM_RX_PORTS; */
		tx[i].ch_num = es325_slim_tx_port_to_ch[i];
		slim_get_slaveport(sbdev->laddr, port_id, &tx[i].sph, SLIM_SRC);
		slim_query_ch(sbdev, tx[i].ch_num, &tx[i].ch_h);
		dev_dbg(&sbdev->dev,
			"%s():port_id = %d, ch_num = %d, sph = 0x%08x\n",
			__func__, port_id, tx[i].ch_num, tx[i].sph);
	}
}

static int es325_cfg_slim_rx(struct slim_device *sbdev, unsigned int *ch_num,
			     unsigned int ch_cnt, unsigned int rate, bool commit)
{
	struct es325_priv *es325_priv = slim_get_devicedata(sbdev);
	struct es325_slim_ch *rx = es325_priv->slim_rx;
	u16 grph;
	u32 sph[ES325_SLIM_RX_PORTS] = {0};
	u16 ch_h[ES325_SLIM_RX_PORTS] = {0};
	struct slim_ch prop;
	int i;
	int idx;
	int rc;

	dev_dbg(&sbdev->dev, "%s():ch_cnt = %d, rate = %d\n",
			__func__, ch_cnt, rate);

	for (i = 0; i < ch_cnt; i++) {
		idx = es325_rx_ch_num_to_idx(ch_num[i]);
		ch_h[i] = rx[idx].ch_h;
		sph[i] = rx[idx].sph;

		dev_dbg(&sbdev->dev,
			"%s():idx = %d, ch_num[i] = %d, ch_h[i] = %d, sph[i] = 0x%08x\n",
			__func__, idx, ch_num[i], ch_h[i], sph[i]);
	}

	prop.prot = SLIM_AUTO_ISO;
	prop.baser = SLIM_RATE_4000HZ;
	prop.dataf = SLIM_CH_DATAF_NOT_DEFINED;
	prop.auxf = SLIM_CH_AUXF_NOT_APPLICABLE;
	prop.ratem = (rate/4000);
	prop.sampleszbits = 16;

	rc = slim_define_ch(sbdev, &prop, ch_h, ch_cnt, true, &grph);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():slim_define_ch() failed: %d\n", __func__, rc);
		goto slim_define_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		rc = slim_connect_sink(sbdev, &sph[i], 1, ch_h[i]);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s():slim_connect_sink() failed: %d\n", __func__, rc);
			goto slim_connect_sink_error;
		}
	}

	rc = slim_control_ch(sbdev, grph, SLIM_CH_ACTIVATE, true /*commit*/);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():slim_control_ch() failed: %d\n", __func__, rc);
		goto slim_control_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		idx = es325_rx_ch_num_to_idx(ch_num[i]);
		dev_dbg(&sbdev->dev, "%s():idx = %d\n", __func__, idx);
		rx[idx].grph = grph;
	}
	return rc;
slim_control_ch_error:
slim_connect_sink_error:
	es325_close_slim_rx(sbdev, ch_num, ch_cnt);
slim_define_ch_error:
	dev_dbg(&sbdev->dev, "%s():rc = %d\n", __func__, rc);
	return rc;
}

static int es325_cfg_slim_tx(struct slim_device *sbdev, unsigned int *ch_num,
				unsigned int ch_cnt, unsigned int rate, bool commit)
{
	struct es325_priv *es325_priv = slim_get_devicedata(sbdev);
	struct es325_slim_ch *tx = es325_priv->slim_tx;
	u16 grph;
	u32 sph[ES325_SLIM_TX_PORTS] = {0};
	u16 ch_h[ES325_SLIM_TX_PORTS] = {0};
	struct slim_ch prop;
	int i;
	int idx;
	int rc;

	dev_dbg(&sbdev->dev, "%s():ch_cnt = %d, rate = %d\n",	__func__, ch_cnt, rate);

	for (i = 0; i < ch_cnt; i++) {
		idx = es325_tx_ch_num_to_idx(ch_num[i]);
		ch_h[i] = tx[idx].ch_h;
		sph[i] = tx[idx].sph;
		dev_dbg(&sbdev->dev,
			"%s():idx = %d ch_num[i] = %d ch_h[i] = %d sph[i] = 0x%08x\n",
			__func__, idx, ch_num[i], ch_h[i], sph[i]);
	}

	prop.prot = SLIM_AUTO_ISO;
	prop.baser = SLIM_RATE_4000HZ;
	prop.dataf = SLIM_CH_DATAF_NOT_DEFINED;
	prop.auxf = SLIM_CH_AUXF_NOT_APPLICABLE;
	prop.ratem = (rate/4000);
	prop.sampleszbits = 16;

	rc = slim_define_ch(sbdev, &prop, ch_h, ch_cnt, true, &grph);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():slim_define_ch() failed: %d\n", __func__, rc);
		goto slim_define_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		rc = slim_connect_src(sbdev, sph[i], ch_h[i]);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s():slim_connect_src() failed: %d\n", __func__, rc);
			dev_err(&sbdev->dev, "%s():ch_num[0] = %d\n", __func__, ch_num[0]);
			goto slim_connect_src_error;
		}
	}

	rc = slim_control_ch(sbdev, grph, SLIM_CH_ACTIVATE, true /*commit*/);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():slim_control_ch() failed: %d\n", __func__, rc);
		goto slim_control_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		idx = es325_tx_ch_num_to_idx(ch_num[i]);
		dev_dbg(&sbdev->dev, "%s():idx = %d\n", __func__, idx);
		tx[idx].grph = grph;
	}
	return rc;
slim_control_ch_error:
slim_connect_src_error:
	es325_close_slim_tx(sbdev, ch_num, ch_cnt);
slim_define_ch_error:
	dev_dbg(&sbdev->dev, "%s():rc = %d\n", __func__, rc);
	return rc;
}

static int es325_close_slim_rx(struct slim_device *sbdev, unsigned int *ch_num, unsigned int ch_cnt)
{
	struct es325_priv *es325_priv = slim_get_devicedata(sbdev);
	struct es325_slim_ch *rx = es325_priv->slim_rx;
	u16 grph = 0;
	u32 sph[ES325_SLIM_RX_PORTS] = {0};
	int i;
	int idx;
	int rc;

	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	for (i = 0; i < ch_cnt; i++) {
		idx = es325_rx_ch_num_to_idx(ch_num[i]);
		sph[i] = rx[idx].sph;
		grph = rx[idx].grph;
		dev_dbg(&sbdev->dev, "%s():sph[%d] = 0x%08x, grph[%d] = 0x%08x\n", __func__, i, sph[i], i, grph);
	}

	rc = slim_control_ch(sbdev, grph, SLIM_CH_REMOVE, true);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():slim_control_ch() failed: %d\n", __func__, rc);
		goto slim_control_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		idx = es325_rx_ch_num_to_idx(ch_num[i]);
		dev_dbg(&sbdev->dev,"%s():idx = %d\n", __func__, idx);
		rx[idx].grph = 0;
	}
	rc = slim_disconnect_ports(sbdev, sph, ch_cnt);
	if (rc < 0)
		dev_err(&sbdev->dev, "%s():slim_disconnect_ports() failed: %d\n", __func__, rc);

	dev_dbg(&sbdev->dev, "%s():close RX channel",__func__);
	for (i = 0; i < ch_cnt; i++)
		dev_dbg(&sbdev->dev, "[%d]",ch_num[i]);

slim_control_ch_error:
	dev_dbg(&sbdev->dev, "%s()\n", __func__);
	return rc;
}

static int es325_close_slim_tx(struct slim_device *sbdev, unsigned int *ch_num, unsigned int ch_cnt)
{
	struct es325_priv *es325_priv = slim_get_devicedata(sbdev);
	struct es325_slim_ch *tx = es325_priv->slim_tx;
	u16 grph = 0;
	u32 sph[ES325_SLIM_TX_PORTS] = {0};
	int i;
	int idx;
	int rc;

	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	for (i = 0; i < ch_cnt; i++) {
		idx = es325_tx_ch_num_to_idx(ch_num[i]);
		sph[i] = tx[idx].sph;
		grph = tx[idx].grph;
		dev_dbg(&sbdev->dev, "%s():idx %d ch_num[%d] %d sph[%d] = 0x%08x, grph[%d] = 0x%08x\n",
			__func__, idx, i, ch_num[i], i, sph[i], i, grph);
	}

	rc = slim_control_ch(sbdev, grph, SLIM_CH_REMOVE, true);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():slim_connect_sink() failed: %d\n", __func__, rc);
		goto slim_control_ch_error;
	}
	for (i = 0; i < ch_cnt; i++) {
		idx = es325_tx_ch_num_to_idx(ch_num[i]);
		dev_dbg(&sbdev->dev, "%s():ch_num[%d] %d idx = %d\n", __func__, i, ch_num[i], idx);
		tx[idx].grph = 0;
	}
	rc = slim_disconnect_ports(sbdev, sph, ch_cnt);
	if (rc < 0)
		dev_err(&sbdev->dev, "%s():slim_disconnect_ports() failed: %d\n", __func__, rc);

	dev_dbg(&sbdev->dev, "%s():close TX channel",__func__);
	for (i = 0; i < ch_cnt; i++)
		dev_dbg(&sbdev->dev, "[%d]",ch_num[i]);

slim_control_ch_error:
	dev_dbg(&sbdev->dev, "%s()\n", __func__);
	return rc;
}

int es325_remote_cfg_slim_rx(int dai_id)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	int be_id;
	int rc = 0;

	if (es325->power_stage != ES325_POWER_AWAKE)
		return 0;

	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	if(es325_fw_downloaded == 0) {
		dev_info(&sbdev->dev, "%s():eS325 FW not ready, cfg_slim_rx rejected\n", __func__);
		return rc;
	}

	if (dai_id != ES325_SLIM_1_PB && dai_id != ES325_SLIM_2_PB) {
		dev_dbg(&sbdev->dev, "%s():rc = %d\n", __func__, rc);
		return rc;
	}

	/* This is for defending ch_tot is not reset */
	if ((es325_rx1_route_enable == 0) && (es325_rx2_route_enable == 0)) {
		dev_dbg(&sbdev->dev, "%s():rc = %d\n", __func__, rc);
		return rc;
	}

	if (es325->gen0_client == NULL) {
		dev_err(&sbdev->dev, "%s():gen0_client is NULL\n", __func__);
		return rc;
	}

	if (es325->dai[ID(dai_id)].ch_tot != 0) {
		/* start slim channels associated with id */
		rc = es325_cfg_slim_rx(es325->gen0_client, es325->dai[ID(dai_id)].ch_num,
					es325->dai[ID(dai_id)].ch_tot, es325->dai[ID(dai_id)].rate, false);

		be_id = es325_slim_be_id[ID(dai_id)];
		es325->dai[ID(be_id)].ch_tot = es325->dai[ID(dai_id)].ch_tot;
		es325->dai[ID(be_id)].rate = es325->dai[ID(dai_id)].rate;
		if (be_id == ES325_SLIM_2_CAP) {
			es325->dai[ID(be_id)].ch_num[0] = SMB_TX_PORT2;
			es325->dai[ID(be_id)].ch_num[1] = SMB_TX_PORT3;
		} else if (be_id == ES325_SLIM_3_CAP) {
			es325->dai[ID(be_id)].ch_num[0] = SMB_TX_PORT4;
			es325->dai[ID(be_id)].ch_num[1] = SMB_TX_PORT5;
		}
		rc = es325_codec_cfg_slim_tx(es325, be_id, true);
		dev_info(&sbdev->dev, "%s():MDM->>>[%d][%d]ES325[%d][%d]->>>WCD channel mapping\n",
			__func__,
			es325->dai[ID(dai_id)].ch_num[0],
			es325->dai[ID(dai_id)].ch_tot == 1 ? 0 : es325->dai[ID(dai_id)].ch_num[1],
			es325->dai[ID(be_id)].ch_num[0],
			es325->dai[ID(be_id)].ch_tot == 1 ? 0 : es325->dai[ID(be_id)].ch_num[1]);
	}

	dev_dbg(&sbdev->dev, "%s():rc = %d\n", __func__, rc);
	return rc;
}
EXPORT_SYMBOL_GPL(es325_remote_cfg_slim_rx);

int es325_remote_cfg_slim_tx(int dai_id)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	int be_id;
	int ch_cnt;
	int rc = 0;

	if (es325->power_stage != ES325_POWER_AWAKE)
		return 0;

	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	if(es325_fw_downloaded == 0) {
		dev_info(&sbdev->dev, "%s():eS325 FW not ready, cfg_slim_tx rejected\n", __func__);
		return rc;
	}

	if (dai_id != ES325_SLIM_1_CAP) {
		dev_dbg(&sbdev->dev, "%s():rc = %d\n", __func__, rc);
		return rc;
	}

	/* This is for defending ch_tot is not reset */
	if (es325_tx1_route_enable == 0) {
		dev_dbg(&sbdev->dev, "%s():rc = %d\n", __func__, rc);
		return rc;
	}

	if (es325->gen0_client == NULL) {
		dev_err(&sbdev->dev, "%s():gen0_client is NULL\n", __func__);
		return rc;
	}

	if (es325->dai[ID(dai_id)].ch_tot != 0) {
		/* start slim channels associated with id */
		if (dai_id == ES325_SLIM_1_CAP)
			ch_cnt = es325_ap_tx1_ch_cnt;

		rc = es325_cfg_slim_tx(es325->gen0_client, es325->dai[ID(dai_id)].ch_num,
					ch_cnt, es325->dai[ID(dai_id)].rate, false);

		be_id = es325_slim_be_id[ID(dai_id)];
		es325->dai[ID(be_id)].ch_tot = es325->dai[ID(dai_id)].ch_tot;
		es325->dai[ID(dai_id)].ch_tot = ch_cnt;
		es325->dai[ID(be_id)].rate = es325->dai[ID(dai_id)].rate;
		if (be_id == ES325_SLIM_3_PB) {
			es325->dai[ID(be_id)].ch_num[0] = SMB_RX_PORT4;
			es325->dai[ID(be_id)].ch_num[1] = SMB_RX_PORT5;
		}
		rc = es325_codec_cfg_slim_rx(es325, be_id, true);
		dev_info(&sbdev->dev, "%s():MDM<<<-[%d][%d]ES325[%d][%d]<<<-WCD channel mapping\n",
			__func__,
			es325->dai[ID(dai_id)].ch_num[0],
			es325->dai[ID(dai_id)].ch_tot == 1 ? 0 : es325->dai[ID(dai_id)].ch_num[1],
			es325->dai[ID(be_id)].ch_num[0],
			es325->dai[ID(be_id)].ch_tot == 1 ? 0 : es325->dai[ID(be_id)].ch_num[1]);
	}

	dev_dbg(&sbdev->dev, "%s():rc = %d\n", __func__, rc);
	return rc;
}
EXPORT_SYMBOL_GPL(es325_remote_cfg_slim_tx);

int es325_remote_close_slim_rx(int dai_id)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	int be_id;
	int rc = 0;

	if (es325->power_stage != ES325_POWER_AWAKE)
		return 0;

	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	if (dai_id != ES325_SLIM_1_PB && dai_id != ES325_SLIM_2_PB) {
		dev_dbg(&sbdev->dev, "%s():rc = %d\n", __func__, rc);
		return rc;
	}

	if (es325->gen0_client == NULL) {
		dev_err(&sbdev->dev, "%s():gen0_client is NULL\n", __func__);
		return rc;
	}

	if (es325->dai[ID(dai_id)].ch_tot != 0) {
		dev_info(&sbdev->dev, "%s():dai_id = %d, ch_tot =%d\n",
				__func__, dai_id, es325->dai[ID(dai_id)].ch_tot);
		es325_close_slim_rx(es325->gen0_client,
				    es325->dai[ID(dai_id)].ch_num,
				    es325->dai[ID(dai_id)].ch_tot);

		be_id = es325_slim_be_id[ID(dai_id)];
		rc = es325_codec_close_slim_tx(es325, be_id);

		es325->dai[ID(dai_id)].ch_tot = 0;
#ifdef ES325_SLEEP
		es325->dai[ID(be_id)].ch_tot = 0;
#endif
	}

	return rc;
}
EXPORT_SYMBOL_GPL(es325_remote_close_slim_rx);

int es325_remote_close_slim_tx(int dai_id)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	int be_id;
	int rc = 0;

	if (es325->power_stage != ES325_POWER_AWAKE)
		return 0;

	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	if (dai_id != ES325_SLIM_1_CAP) {
		dev_dbg(&sbdev->dev, "%s():rc = %d\n", __func__, rc);
		return rc;
	}

	if (es325->gen0_client == NULL) {
		dev_err(&sbdev->dev, "%s():gen0_client is NULL\n", __func__);
		return rc;
	}

	if (es325->dai[ID(dai_id)].ch_tot != 0) {
		dev_info(&sbdev->dev, "%s():dai_id = %d, ch_tot = %d\n",
				__func__, dai_id, es325->dai[ID(dai_id)].ch_tot);
		es325_close_slim_tx(es325->gen0_client,
				es325->dai[ID(dai_id)].ch_num,
				es325->dai[ID(dai_id)].ch_tot);

		be_id = es325_slim_be_id[ID(dai_id)];
		rc = es325_codec_close_slim_rx(es325, be_id);

		es325->dai[ID(dai_id)].ch_tot = 0;
#ifdef ES325_SLEEP
		es325->dai[ID(be_id)].ch_tot = 0;
#endif
	}

	return rc;
}
EXPORT_SYMBOL_GPL(es325_remote_close_slim_tx);

static void es325_init_slim_slave(struct slim_device *sbdev)
{
	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	es325_alloc_slim_rx_chan(sbdev);
	es325_alloc_slim_tx_chan(sbdev);
}

static void msg_to_bus_order(char *msg, int msg_len)
{
	char tmp;
	for (; msg_len > 0; msg_len -= 4, msg += 4) {
		tmp = *(msg + 3);
		*(msg + 3) = *(msg);
		*(msg) = tmp;
		tmp = *(msg + 2);
		*(msg + 2) = *(msg + 1);
		*(msg + 1) = tmp;
	}
}

#ifdef BUS_TRANSACTIONS
#if defined(CONFIG_SND_SOC_ES325_SLIM)
static int es325_slim_read(struct es325_priv *es325, unsigned int offset,
			   unsigned int width, char *buf, int len, int bus_order);
static int es325_slim_write(struct es325_priv *es325, unsigned int offset,
			    unsigned int width, char *buf, int len, int bus_order);
#define ES325_BUS_READ(x_es325, x_offset, x_width, x_buf, x_len, x_bus_order) \
	es325_slim_read(x_es325, x_offset, x_width, x_buf, x_len, x_bus_order)
#define ES325_BUS_WRITE(x_es325, x_offset, x_width, x_buf, x_len, x_bus_order) \
	es325_slim_write(x_es325, x_offset, x_width, x_buf, x_len, x_bus_order)
#else
#error "es325.c - bus infrastructure not defined"
#endif
#else
/* Pretend all read and write operations on the bus are successful -
	when no bus is available. */
#define ES325_BUS_READ(e, o, w, b, l, r) (0)
#define ES325_BUS_WRITE(e, o, w, b, l, r) (0)
#endif

#ifdef FIXED_CONFIG
static void es325_fixed_config(struct es325_priv *es325)
{
	int rc;
	struct slim_device *sbdev = es325->gen0_client;

	u8 *msg_ptr = es325_internal_route_configs[5];
	while (*msg_ptr != 0xff) {
		u8 msg[4];
		memcpy(msg, msg_ptr, 4);
		rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET,
				ES325_WRITE_VE_WIDTH, msg, 4, 1);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
				__func__, rc);
			break;
		}
		dev_dbg(&sbdev->dev, "%s():msg = %02x%02x%02x%02x\n", __func__,
				msg[3], msg[2], msg[1], msg[0]);
		msg_ptr += 4;
	}
}
#endif

	/*
 * Delay for receiving response can be up to 20 ms.
 * To minimize waiting time, response is checking
 * up to 20 times with 1ms delay.
*/
#define MAX_TRIALS	20
#define SMB_DELAY	1000
const char NOT_READY[4] = {0x00, 0x00, 0x00, 0x00};
static int es325_request_response(struct es325_priv *es325,
				  char *rqst_ptr, int rqst_len, int rqst_order,
				  char *rspn_ptr, int rspn_len, int rspn_order,
				  int match, int return_if_fail)
{
	int rc = 0;
	int i = 0;
	int try_write = 1;
	char msg[4];
	struct slim_device *sbdev = es325->gen0_client;

	do {
		rc = 0;
		if (try_write) {
			memcpy(msg, rqst_ptr, 4);
			rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET,
					ES325_WRITE_VE_WIDTH, msg, 4, rqst_order);
			if (rc < 0) {
				if (return_if_fail)
					break;
				else
					dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
						__func__, rc);
			} else {
				try_write = 0;
			}
		}
		usleep_range(SMB_DELAY, SMB_DELAY);
		if (rc == 0) {
			memset(msg, 0, 4);
			rc = ES325_BUS_READ(es325, ES325_READ_VE_OFFSET,
						ES325_READ_VE_WIDTH, msg, rspn_len, rspn_order);
			if (rc < 0) {
				if (return_if_fail)
					break;
				else
					dev_err(&sbdev->dev, "%s():slim read fail, rc=%d\n",
						__func__, rc);
			} else {
				/* save response */
				memcpy(rspn_ptr, msg, rspn_len);
				if (memcmp(msg, NOT_READY, 4) != 0) {
					rc = 0;
					if (match && (memcmp(rqst_ptr, msg, rspn_len) != 0))
						rc = -1;
					break;
				}
			}
		}
		i++;
	} while (i <= MAX_TRIALS);
	if (i > MAX_TRIALS) {
		dev_err(&sbdev->dev, "%s():Request-Response reach MAX_TRIALS (20), fail\n", __func__);
		return -1;
	}
	return rc;
}

static int es325_slim_read(struct es325_priv *es325, unsigned int offset,
				unsigned int width, char *buf, int len, int bus_order)
{
	struct slim_device *sbdev = es325->gen0_client;
	DECLARE_COMPLETION_ONSTACK(read_done);
	struct slim_ele_access msg = {
		.start_offset = offset,
		.num_bytes = width,
		/* .comp = &read_done, */
		.comp = NULL,
	};
	int rc;
	rc = slim_request_val_element(sbdev, &msg, buf, len);
	if (bus_order)
		msg_to_bus_order(buf, len);

	return rc;
}

static int es325_slim_write(struct es325_priv *es325, unsigned int offset,
				unsigned int width, char *buf, int len, int bus_order)
{
	struct slim_device *sbdev = es325->gen0_client;
	struct slim_ele_access msg = {
		.start_offset = offset,
		.num_bytes = width,
		.comp = NULL,
	};
	int rc;

	if (bus_order)
		msg_to_bus_order(buf, len);
	rc = slim_change_val_element(sbdev, &msg, buf, len);
	if (rc != 0)
		dev_err(&sbdev->dev, "%s():slim_write failed rc=%d\n", __func__, rc);

	return rc;
}

static int es325_build_algo_read_msg(char *msg, int *msg_len, unsigned int reg)
{
	unsigned int index = reg & ES325_ADDR_MASK;
	unsigned int paramid;

	if (index >= ARRAY_SIZE(es325_algo_paramid))
		return -EINVAL;

	paramid = es325_algo_paramid[index];

	/* ES325_GET_ALGO_PARAM */
	*msg++ = (ES325_GET_ALGO_PARAM >> 8) & 0x00ff;
	*msg++ = ES325_GET_ALGO_PARAM & 0x00ff;

	/* PARAM ID */
	*msg++ = (paramid >> 8) & 0x00ff;
	*msg++ = paramid & 0x00ff;
	*msg_len = 4;

	return 0;
}

static int es325_build_algo_write_msg(char *msg, int *msg_len,
					unsigned int reg, unsigned int value)
{
	unsigned int index = reg & ES325_ADDR_MASK;
	unsigned int cmd;
	unsigned int paramid;

	if (index >= ARRAY_SIZE(es325_algo_paramid))
		return -EINVAL;

	paramid = es325_algo_paramid[index];

	/* ES325_SET_ALGO_PARAMID */
	cmd = ES325_SET_ALGO_PARAMID;
	if (reg & ES325_STAGED_CMD)
		cmd |= ES325_STAGED_MSG_BIT;
	*msg++ = (cmd >> 8) & 0x00ff;
	*msg++ = cmd & 0x00ff;

	/* PARAM ID */
	*msg++ = (paramid >> 8) & 0x00ff;
	*msg++ = paramid & 0x00ff;

	/* ES325_SET_ALGO_PARAM */
	cmd = ES325_SET_ALGO_PARAM;
	if (reg & ES325_STAGED_CMD)
		cmd |= ES325_STAGED_MSG_BIT;
	*msg++ = (cmd >> 8) & 0x00ff;
	*msg++ = cmd & 0x00ff;

	/* value */
	*msg++ = (value >> 8) & 0x00ff;
	*msg++ = value & 0x00ff;
	*msg_len = 8;

	return 0;
}

static int es325_build_dev_read_msg(char *msg, int *msg_len, unsigned int reg)
{
	unsigned int index = reg & ES325_ADDR_MASK;
	unsigned int paramid;

	if (index >= ARRAY_SIZE(es325_dev_paramid))
		return -EINVAL;

	paramid = es325_dev_paramid[index];

	/* ES325_GET_DEV_PARAM */
	*msg++ = (ES325_GET_DEV_PARAM >> 8) & 0x00ff;
	*msg++ = ES325_GET_DEV_PARAM & 0x00ff;

	/* PARAM ID */
	*msg++ = (paramid >> 8) & 0x00ff;
	*msg++ = paramid & 0x00ff;
	*msg_len = 4;

	return 0;
}

static int es325_build_dev_write_msg(char *msg, int *msg_len,
					unsigned int reg, unsigned int value)
{
	unsigned int index = reg & ES325_ADDR_MASK;
	unsigned int cmd;
	unsigned int paramid;

	if (index >= ARRAY_SIZE(es325_dev_paramid))
		return -EINVAL;

	paramid = es325_dev_paramid[index];

	/* ES325_SET_DEV_PARAMID */
	cmd = ES325_SET_DEV_PARAMID;
	if (reg & ES325_STAGED_CMD)
		cmd |= ES325_STAGED_MSG_BIT;
	*msg++ = (cmd >> 8) & 0x00ff;
	*msg++ = cmd & 0x00ff;

	/* PARAM ID */
	*msg++ = (paramid >> 8) & 0x00ff;
	*msg++ = paramid & 0x00ff;

	/* ES325_SET_DEV_PARAM */
	cmd = ES325_SET_DEV_PARAM;
	if (reg & ES325_STAGED_CMD)
		cmd |= ES325_STAGED_MSG_BIT;
	*msg++ = (cmd >> 8) & 0x00ff;
	*msg++ = cmd & 0x00ff;

	/* value */
	*msg++ = (value >> 8) & 0x00ff;
	*msg++ = value & 0x00ff;
	*msg_len = 8;

	return 0;
}

static int es325_build_cmd_read_msg(char *msg, int *msg_len, unsigned int reg)
{
	unsigned int index = reg & ES325_ADDR_MASK;
	struct es325_cmd_access *cmd_access;

	if (index >= ARRAY_SIZE(es325_cmd_access))
		return -EINVAL;
	cmd_access = es325_cmd_access + index;

	*msg_len = cmd_access->read_msg_len;
	memcpy(msg, &cmd_access->read_msg, *msg_len);

	return 0;
}

static int es325_build_cmd_write_msg(char *msg, int *msg_len,
					unsigned int reg, unsigned int value)
{
	unsigned int index = reg & ES325_ADDR_MASK;
	struct es325_cmd_access *cmd_access;

	if (index >= ARRAY_SIZE(es325_cmd_access))
		return -EINVAL;
	cmd_access = es325_cmd_access + index;

	*msg_len = cmd_access->write_msg_len;
	memcpy(msg, &cmd_access->write_msg, *msg_len);
	if (reg & ES325_STAGED_CMD)
		*msg |= (1 << 5);

	return 0;
}

static unsigned int es325_read(struct snd_soc_codec *codec, unsigned int reg)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	unsigned int access = reg & ES325_ACCESS_MASK;
	char msg[16];
	unsigned int msg_len;
	unsigned int value;
	int rc;

	switch (access) {
	case ES325_ALGO_ACCESS:
		rc = es325_build_algo_read_msg(msg, &msg_len, reg);
		break;
	case ES325_DEV_ACCESS:
		rc = es325_build_dev_read_msg(msg, &msg_len, reg);
		break;
	case ES325_CMD_ACCESS:
		rc = es325_build_cmd_read_msg(msg, &msg_len, reg);
		break;
	case ES325_OTHER_ACCESS:
		return 0;
	default:
		rc = -EINVAL;
		break;
	}
	if (rc) {
		dev_err(&sbdev->dev, " %s():failed to build read message for address = 0x%04x\n", __func__, reg);
		return rc;
	}

	rc = es325_request_response(es325, msg, msg_len, 1, msg, 4, 1, 0, 0);
	if (rc < 0)
		return rc;
	value = msg[2] << 8 | msg[3];

	return value;
}

static int es325_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	unsigned int access = reg & ES325_ACCESS_MASK;
	char msg[16];
	char *msg_ptr;
	int msg_len = 0;
	int i;
	int rc;

	switch (access) {
	case ES325_ALGO_ACCESS:
		rc = es325_build_algo_write_msg(msg, &msg_len, reg, value);
		break;
	case ES325_DEV_ACCESS:
		rc = es325_build_dev_write_msg(msg, &msg_len, reg, value);
		break;
	case ES325_CMD_ACCESS:
		rc = es325_build_cmd_write_msg(msg, &msg_len, reg, value);
		break;
	case ES325_OTHER_ACCESS:
		return 0;
	default:
		rc = -EINVAL;
		break;
	}
	if (rc) {
		dev_err(&sbdev->dev, " %s():failed to build write message for address = 0x%04x\n", __func__, reg);
		return rc;
	}

	msg_ptr = msg;
	for (i = msg_len; i > 0; i -= 4, msg_ptr += 4) {
		rc = es325_request_response(es325, msg_ptr, 4, 1, msg_ptr, 4, 1, 0, 0);
		if (rc < 0)
			break;
	}
	return rc;
}

static ssize_t es325_route_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;
	int idx;
	ssize_t rc = 0;
	struct es325_priv *es325 = &es325_priv;
	int active;
	char *route_st_names[] = {
		"ACTIVE",
		"MUTING",
		"SWITCHING",
		"UNMUTING",
		"INACTIVE",
		"es325 NO RESPONSE"
	};

	char *status_name[] = {
		"Route Status",
		"Slimbus Ports Raw Status",
		"Slimbus Active Rx Ports",
		"Slimbus Active Tx Ports",
	};

	u8 route_st_req_msg[] = {
		/* Route Status */
		0x80, 0x4f, 0x00, 0x00,
	};

	u8 port_st_req_msg[] = {
		/* SBUS port  State read */
		0x80, 0x0B, 0x09, 0x12,
	};

	u8 ack_msg[4];

	/* Read route status */
	if (es325_request_response(es325, route_st_req_msg, 4, 1, ack_msg, 4, 1, 0, 0) < 0) {
		rc = rc + snprintf(buf+rc, PAGE_SIZE - rc,
			"Read Route Status FAIL. SLIMBus error\n");
		return rc;
	}
	dev_info(dev, "%s():ping ack = %02x%02x%02x%02x\n",
		__func__, ack_msg[0], ack_msg[1], ack_msg[2], ack_msg[3]);

	/* Successful response? */
	if ((ack_msg[0] == 0x80) && (ack_msg[1] == 0x4f)) {
		rc = rc + snprintf(buf+rc, PAGE_SIZE - rc,
			"%s = 0x%02x%02x%02x%02x - %s\n",
			status_name[0],
			ack_msg[0], ack_msg[1],
			ack_msg[2], ack_msg[3],
			route_st_names[ack_msg[3]] );

		/* Read Port status */
		if (es325_request_response(es325, port_st_req_msg, 4, 1, ack_msg, 4, 1, 0, 0) < 0) {
			rc = rc + snprintf(buf+rc, PAGE_SIZE - rc,
				"Read Port Status FAIL. SLIMBus error\n");
			return rc;
		}
		dev_info(dev, " %s():ping ack = %02x%02x%02x%02x\n",
				__func__, ack_msg[0], ack_msg[1], ack_msg[2], ack_msg[3]);

		if ((ack_msg[0] == 0x80) && (ack_msg[1] == 0x0B)) {
			u16 port_status = (ack_msg[2]  << 8) | ack_msg[3];
			rc = rc + snprintf(buf+rc, PAGE_SIZE - rc,
					"%s = 0x%02x%02x%02x%02x \n",
					status_name[1],
					ack_msg[0], ack_msg[1],
					ack_msg[2], ack_msg[3]);

			rc = rc + snprintf(buf+rc, PAGE_SIZE - rc, "%s = ", status_name[2]);
			active = 0;
			for (i = 0; i < 10; i++) {
				idx = (port_status & (1 << i)) >> i;

				if (idx == 1) {
					active = 1;
					rc = rc + snprintf(buf+rc, PAGE_SIZE - rc, "%d ", i);
				}
			}

			if (active == 0)
				rc = rc + snprintf(buf+rc, PAGE_SIZE - rc, "None\n%s = ", status_name[3]);
			else
				rc = rc + snprintf(buf+rc, PAGE_SIZE - rc, "\n%s = ", status_name[3]);

			active = 0;
			for (i = 10; i < 16; i++) {
				idx = (port_status & (1 << i)) >> i;

				if (idx == 1) {
					active = 1;
					rc = rc + snprintf(buf+rc, PAGE_SIZE - rc, "%d ", (i-10));
				}
			}
			if (active == 0)
				rc = rc + snprintf(buf+rc, PAGE_SIZE - rc, "None\n");
			else
				rc = rc + snprintf(buf+rc, PAGE_SIZE - rc, "\n" );
		} else {
			rc = rc + snprintf(buf+rc, PAGE_SIZE - rc,
					"%s = 0x%02x%02x%02x%02x - %s\n",
					status_name[1],
					ack_msg[0], ack_msg[1],
					ack_msg[2], ack_msg[3],
					"Cannot Read!" );
		}
	} else {
		rc = rc + snprintf(buf+rc, PAGE_SIZE - rc,
					"%s = 0x%02x%02x%02x%02x - %s\n",
					status_name[0],
					ack_msg[0], ack_msg[1],
					ack_msg[2], ack_msg[3],
					route_st_names[5] );
	}
	return rc;
}

static ssize_t es325_route_config_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	dev_info(dev, " %s():route=%ld\n", __func__, es325_internal_route_num);
	return snprintf(buf, PAGE_SIZE, "route=%ld\n", es325_internal_route_num);
}

#ifndef ES325_SLEEP
static void es325_switch_route(long route_index)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	u8 msg[4];
	u8 *msg_ptr;
	int rc;

	if (route_index > ES325_INTERNAL_ROUTE_MAX) {
		dev_info(&sbdev->dev, " %s():new es325_internal_route = %ld is out of range\n", __func__, route_index);
		return;
	}

	dev_info(&sbdev->dev, "%s():switch current es325_internal_route = %ld to new route = %ld\n",
		__func__, es325_internal_route_num, route_index);
	es325_internal_route_num = route_index;

	if (es325_network_type != NARROW_BAND) {
		if (es325_internal_route_num >= 0 && es325_internal_route_num < 5)
			es325_internal_route_num += NETWORK_OFFSET;
	}

	msg_ptr = &es325_internal_route_configs[es325_internal_route_num][0];
	while (*msg_ptr != 0xff) {
		memcpy(msg, msg_ptr, 4);
		rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET, ES325_WRITE_VE_WIDTH, msg, 4, 1);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
				__func__, rc);
			break;
		}
		msg_ptr += 4;
	}
}
#else
static void es325_switch_route(void)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	u8 msg[4];
	u8 *msg_ptr;
	int rc;

	if (es325_network_type == WIDE_BAND) {
		if (es325->new_internal_route_config >= 0 &&
			es325->new_internal_route_config < 5) {
			es325->new_internal_route_config += NETWORK_OFFSET;
			dev_info(&sbdev->dev, "%s():adjust wideband offset\n", __func__);
		}
	}

	if (es325_network_type == NARROW_BAND) {
		if (es325->new_internal_route_config >= 0 + NETWORK_OFFSET &&
			es325->new_internal_route_config < 5 + NETWORK_OFFSET) {
			es325->new_internal_route_config -= NETWORK_OFFSET;
			dev_info(&sbdev->dev, "%s():adjust narrowband offset\n", __func__);
		}
	}

	dev_info(&sbdev->dev, "%s():switch current es325_internal_route = %d to new route = %d\n",
		__func__, es325->internal_route_config, es325->new_internal_route_config);

	if (es325->new_internal_route_config > ES325_INTERNAL_ROUTE_MAX) {
		dev_info(&sbdev->dev, "%s():new es325_internal_route = %d is out of range\n",
			__func__, es325->new_internal_route_config);
		return;
	}

	if (es325->internal_route_config != es325->new_internal_route_config) {
		es325_internal_route_num = es325->new_internal_route_config;
		dev_info(&sbdev->dev, "%s():final es325_internal_route_num = %ld\n",
			__func__, es325_internal_route_num);
		msg_ptr = &es325_internal_route_configs[es325_internal_route_num][0];
		while (*msg_ptr != 0xff) {
			memcpy(msg, msg_ptr, 4);
			rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET, ES325_WRITE_VE_WIDTH, msg, 4, 1);
			if (rc < 0)
				dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
					__func__, rc);
			msg_ptr += 4;
		}
		es325->internal_route_config = es325->new_internal_route_config;
	}
}
#endif

static ssize_t es325_route_config_set(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	long route_index;
	int rc;

	dev_info(dev, "%s():buf = %s\n", __func__, buf);
	rc = kstrtol(buf, 10, &route_index);
#ifndef ES325_SLEEP
	es325_switch_route(route_index);
#else
	es325_priv.new_internal_route_config = route_index;
	dev_info(dev, "%s wakeup_cnt=%d\n", __func__, es325_priv.wakeup_cnt);
	mutex_lock(&es325_priv.pm_mutex);
	if (es325_priv.wakeup_cnt)
		es325_switch_route();
	mutex_unlock(&es325_priv.pm_mutex);
#endif
	return count;
}


#define SIZE_OF_VERBUF 256

static ssize_t es325_fw_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char first_char_msg[4] = {0x90, 0x20, 0x00, 0x00};
	char next_char_msg[4] = {0x80, 0x21, 0x00, 0x00};
	struct es325_priv *es325 = &es325_priv;
	int rc = 0, idx = 0;
	unsigned int imsg = 1; /* force first loop */
	unsigned char bmsg[4];
	char versionbuffer[SIZE_OF_VERBUF];
	char *verbuf = versionbuffer;
	char cmd[4];

	if(es325->power_stage != ES325_POWER_AWAKE){
		return sprintf(buf, "Can not get route_status when power_state is %d\n", es325->power_stage);
	}

	if(es325_fw_downloaded) {
		memset(verbuf,0,SIZE_OF_VERBUF);
		memcpy(cmd, first_char_msg, 4);
		rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET, ES325_WRITE_VE_WIDTH, cmd, 4, 1);
		while ((rc >= 0) && (idx < (SIZE_OF_VERBUF-1)) && (imsg & 0xFF)) {
			memcpy(cmd, next_char_msg, 4);
			rc = es325_request_response(es325, cmd, 4, 1, bmsg, 4, 1, 0, 0);
			if (rc < 0) {
				return snprintf(buf, PAGE_SIZE, "Get FW Version FAIL. SLIMBus error\n");
			}
			imsg = bmsg[3];
			if ((bmsg[0] == 0xFF) &&  (bmsg[1] == 0xFF)){
				dev_err(dev, "%s():No version API on Audience\n", __func__);
				rc = -1;
			} else {
				verbuf[idx++] = (char) (bmsg[3]);
			}
		}
		/* Null terminate the string*/
		verbuf[idx] = '\0';
		dev_info(dev, "%s():Audience fw ver %s\n", __func__, verbuf);
		return snprintf(buf, PAGE_SIZE, "FW Version = %s\n",verbuf);
	}else {
		return snprintf(buf, PAGE_SIZE, "Need to FW download\n");
	}
}

static ssize_t es325_txhex_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	dev_info(dev, "%s():called\n", __func__);
	return 0;
}

static ssize_t es325_txhex_set(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct es325_priv *es325 = &es325_priv;
	u8 cmd[128];
	int cmdlen;
	int offset = 0;
	u8 resp[4];
	int rc;

	dev_dbg(dev, "%s()\n", __func__);
	dev_dbg(dev, "%s():count=%i\n", __func__, count);

	/* No command sequences larger than 128 bytes. */
	BUG_ON(count > (128 * 2) + 1);
	/* Expect a even number of hexadecimal digits terminated by a newline. */
	BUG_ON(!(count & 1));

	rc = hex2bin(cmd, buf, count / 2);
	BUG_ON(rc != 0);
	cmdlen = count / 2;
	dev_dbg(dev, "%s rc=%d cmdlen=%d\n", __func__, rc, cmdlen);
	while (offset < cmdlen) {
		/* Commands must be written in 4 byte blocks. */
		int wrsize = (cmdlen - offset > 4) ? 4 : cmdlen - offset;

		rc = es325_request_response(es325, &cmd[offset], wrsize, 1, resp, 4, 1, 0, 0);
		if (rc != 0) {
			dev_err(dev, "%s():FAIL\n", __func__);
			break;
		} else {
			dev_dbg(dev, "%s():%02x%02x%02x%02x\n",
				__func__, resp[0], resp[1], resp[2], resp[3]);
			offset += wrsize;
		}
	}
	dev_dbg(dev, "%s()\n", __func__);
	return count;
}

static ssize_t es325_clock_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char status[4];

	dev_dbg(dev, "%s\n", __func__);
	if(es325_priv.clock_on)
		snprintf(status, sizeof("on"), "%s", "on");
	else
		snprintf(status, sizeof("off"), "%s", "off");

	return snprintf(buf, PAGE_SIZE, "clk_status: %s\n", status);
}

static ssize_t es325_slim_ch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct es325_priv *priv = &es325_priv;
	struct es325_slim_dai_data* dai = priv->dai;
	int length = 0;
	int i, j;

	for(i = 0; i < ES325_NUM_CODEC_SLIM_DAIS; i++) {
		length += sprintf(buf+length,"=dai[%d]=rate[%d]=ch_num=",i, dai[i].rate);
		for(j = 0; j < dai[i].ch_tot; j++)
			length += sprintf(buf+length,"[%d]",dai[i].ch_num[j]);
		length += sprintf(buf+length,"%c\n",'=');
	}

	return length;
}

static ssize_t es325_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int length = 0;
	int i;
	int size = 0;

	length += sprintf(buf+length,"es325_reg : algo\n");
	size = sizeof(es325_algo_paramid)/sizeof(unsigned short); /* 127 items */
	for(i = ES325_MIC_CONFIG; i < size; i++)
		length += sprintf(buf+length,"0x%04x : 0x%04x\n", i, es325_read(NULL, i));

	length += sprintf(buf+length,"\nes325_reg : dev\n");
	size = sizeof(es325_dev_paramid)/sizeof(unsigned short); /* 49 items */
	for(i = ES325_PORTA_WORD_LEN; i < (size + ES325_PORTA_WORD_LEN); i++)
		length += sprintf(buf+length,"0x%04x : 0x%04x\n", i, es325_read(NULL, i));

	return length;
}

static ssize_t es325_reg_write(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t size)
{
	char tempbuf[32];
	char *start = tempbuf;
	unsigned long reg, value;

	memcpy(tempbuf, buf, size);
	tempbuf[size] = 0;

	while (*start == ' ')
		start++;
	reg = simple_strtoul(start, &start, 16);
	while (*start == ' ')
		start++;
	if (strict_strtoul(start, 16, &value))
		return -EINVAL;

	es325_write(NULL, reg, value);

	return size;
}

static ssize_t es325_cmd_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int length = 0;
	int i;
	int size = 0;

	/* removed 0x2001(first), 0x20d4(end) register read, because of error */
	size = sizeof(es325_cmd_access)/sizeof(struct es325_cmd_access); /* 213 items */
	for(i = ES325_POWER_STATE + 1; i < (size + ES325_POWER_STATE -1); i++)
		length += sprintf(buf+length,"0x%04x : 0x%04x\n", i, es325_read(NULL, i));

	return length;
}

#define ES325_FW_LOAD_BUF_SZ 4
#define ES325_BOOTUP_DELAY 100
static int es325_bootup(struct es325_priv *es325)
{
	char sync_cmd[] = {
		0x80, 0x00, 0x00, 0x00 /* expecting response 0x80000000 */
	};
	char resp[4];
	char msg[4];
	unsigned int buf_frames;
	char *buf_ptr;
	int rc;
	struct slim_device *sbdev = es325->gen0_client;
	dev_dbg(&sbdev->dev, "%s()\n", __func__);

	/* Send boot command */
	memset(msg, 0, 4);
	msg[0] = ES325_BOOT_CMD & 0x00ff;
	msg[1] = ES325_BOOT_CMD >> 8;
	dev_dbg(&sbdev->dev, "%s():msg[0] = 0x%02x msg[1] = 0x%02x msg[2] = 0x%02x msg[3] = 0x%02x\n",
		__func__, msg[0], msg[1], msg[2], msg[3]);

	/* Keep SLIMBus CG unchangeable during FW downloading time */
	rc = slim_reservemsg_bw(sbdev, 3072000, true);
	if (rc < 0)
		dev_err(&sbdev->dev, "%s():SLIMBus reserve BW fail", __func__);

	rc = es325_request_response(es325, msg, 4, 0, msg, 4, 0, 0, 0);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():firmware load failed", __func__);
		debug_for_dl_firmware = BOOT_MSG_ERR;
		return rc;
	}
	if ((msg[0] != (ES325_BOOT_ACK >> 8)) || (msg[1] != (ES325_BOOT_ACK & 0x00ff))) {
		dev_err(&sbdev->dev, "%s():firmware load failed boot ack pattern", __func__);
		debug_for_dl_firmware = BOOT_MSG_NACK;
		return -EIO;
	}
	dev_info(&sbdev->dev, "%s():write firmware image\n", __func__);

	/* Send image */
	buf_frames = es325->fw->size / ES325_FW_LOAD_BUF_SZ;
	dev_info(&sbdev->dev, "%s():buf_frames = %d", __func__, buf_frames);
	buf_ptr = (char *)es325->fw->data;
	for ( ; buf_frames; --buf_frames, buf_ptr += ES325_FW_LOAD_BUF_SZ) {
		rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET,
				     ES325_WRITE_VE_WIDTH, buf_ptr,
				     ES325_FW_LOAD_BUF_SZ, 0);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s():firmware load failed, slim write fail, rc=%d\n",
			       __func__, rc);
			debug_for_dl_firmware = buf_frames * ES325_FW_LOAD_BUF_SZ;
			return -EIO;
		}
	}
	if (es325->fw->size % ES325_FW_LOAD_BUF_SZ) {
		rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET,
				     ES325_WRITE_VE_WIDTH, buf_ptr,
				     es325->fw->size % ES325_FW_LOAD_BUF_SZ, 0);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s():firmware load failed, slim write fail, rc=%d\n",
			      __func__, rc);
			debug_for_dl_firmware = es325->fw->size % ES325_FW_LOAD_BUF_SZ;
			return -EIO;
		}
	}

	dev_info(&sbdev->dev, "%s():write ES325_SYNC_CMD and wait %dms\n", __func__, ES325_BOOTUP_DELAY);

	/* the delay for stabilization of downloaded firmware */
	msleep(ES325_BOOTUP_DELAY);

	rc = es325_request_response(es325, sync_cmd, 4, 1, resp, 4, 1, 1, 0);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():firmware load failed sync ack failed=0x%02x%02x%02x%02x\n",
			__func__, resp[0], resp[1], resp[2], resp[3]);
		debug_for_dl_firmware = SYNC_MSG_NACK;
		return EIO;
	} else {
		dev_info(&sbdev->dev, "%s():firmware load success sync ack good=0x%02x%02x%02x%02x\n",
			__func__, resp[0], resp[1], resp[2], resp[3]);
		es325_fw_downloaded = 1;
	}
	dev_info(&sbdev->dev, "%s()\n", __func__);
	slim_reservemsg_bw(sbdev, 0, true);
	return 0;
}

static int register_snd_soc(struct es325_priv *priv);

static int fw_download(void *arg)
{
	struct es325_priv *priv = (struct es325_priv *)arg;
	int rc;

	pr_info("%s():es325 gen0 LA=%d\n", __func__, priv->gen0_client->laddr);
#ifdef BUS_TRANSACTIONS

#if defined(CONFIG_SND_SOC_ES325_ATLANTIC)
	msm_slim_vote_func(priv->gen0_client);
#endif

	usleep_range(10000, 11000);
	rc = es325_bootup(priv);
#endif
	pr_info("%s():bootup rc=%d\n", __func__, rc);
	release_firmware(priv->fw);

	rc = register_snd_soc(priv);
	pr_info("%s():register_snd_soc rc=%d\n", __func__, rc);

#ifdef FIXED_CONFIG
	es325_fixed_config(priv);
#endif

	module_put(THIS_MODULE);
	pr_info("%s()\n", __func__);
	return 0;
}

static const char *fw_path[] = {
       "/data"
};

/* Don't inline this: 'struct kstat' is biggish */
static noinline_for_stack long fw_file_size(struct file *file)
{
	struct kstat st;
	if (vfs_getattr(file->f_path.mnt, file->f_path.dentry, &st))
		return -1;
	if (!S_ISREG(st.mode))
		return -1;
	if (st.size != (long)st.size)
		return -1;
	return st.size;
}

static bool fw_read_file_contents(struct file *file, struct firmware *fw)
{
	long size;
	char *buf;

	size = fw_file_size(file);
	if (size < 0)
		return false;
	buf = vmalloc(size);
	if (!buf)
		return false;
	if (kernel_read(file, 0, buf, size) != size) {
		vfree(buf);
		return false;
	}
	fw->data = buf;
	fw->size = size;
	return true;
}

static bool fw_get_filesystem_firmware(struct firmware *fw, const char *name)
{
	int i;
	bool rc = false;
	char *path = __getname();

	for (i = 0; i < ARRAY_SIZE(fw_path); i++) {
		struct file *file;
		snprintf(path, PATH_MAX, "%s/%s", fw_path[i], name);

		file = filp_open(path, O_RDONLY, 0);
		if (IS_ERR(file))
			continue;
		rc = fw_read_file_contents(file, fw);
		fput(file);
		if (rc)
			break;
	}
	__putname(path);
	return rc;
}

static int es325_sleep(struct es325_priv *es325);
static int es325_wakeup(struct es325_priv *es325);

static ssize_t es325_firmware_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned long val;
	int rc;
	const char *name = CONFIG_EXTRA_FIRMWARE;

	rc = kstrtoul(buf, 10, &val);
	if (rc) {
		dev_err(dev, "%s():err %d\n", __func__, rc);
		return rc;
	}

	if(val == 1) {
		fw_get_filesystem_firmware(es325_priv.fw, name);
		if(es325_priv.fw->data == NULL){
			dev_err(dev, "%s():there is no firmware found\n", __func__);
			return count;
		}
		es325_enable_ext_clk(1);
		usleep_range(1000, 1000);
		es325_wakeup(&es325_priv);
		es325_priv.clock_on = 1;

		/* reset the audience chip */
		gpio_set_value(es325_priv.pdata->reset_gpio, 0);
		usleep_range(1000,1000);
		gpio_set_value(es325_priv.pdata->reset_gpio, 1);
		msleep(50);

		/* Get the control for wakeup GPIO */
		mutex_lock(&es325_priv.wakeup_mutex);

		if (uart_enable) {
			gpio_direction_output(es325_priv.pdata->wakeup_gpio, 1);
			usleep_range(5000, 5100);
		}

		/* Make sure wakeup pin is High before wake up */
		gpio_set_value(es325_priv.pdata->wakeup_gpio, 1);
		msleep(50);
		mutex_unlock(&es325_priv.wakeup_mutex);

		es325_bootup(&es325_priv);
#ifdef FIXED_CONFIG
		es325_fixed_config(&es325_priv);
#endif
		es325_sleep(&es325_priv);
		es325_enable_ext_clk(0);
		es325_priv.clock_on = 0;
	} else {
		return -EINVAL;
	}
	return count;
}

static ssize_t es325_uart_set(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int value = 0;

	sscanf(buf, "%d", &value);
	pr_info("%s : [ES325] uart_set = %d\n", __func__, value);

	if (value == 1)
		uart_enable = 1;
	else
		uart_enable = 0;

	return count;
}

static ssize_t es325_power_stage_show(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct es325_priv *es325 = &es325_priv;

	return sprintf(buf, "es325 power_stage = %d\n", es325->power_stage);
}

static struct device_attribute es325_device_attrs[] = {
	__ATTR(route_status,	0644, es325_route_status_show,	NULL),
	__ATTR(route_config,	0644, es325_route_config_show, es325_route_config_set),
	__ATTR(fw_version,		0644, es325_fw_version_show, NULL),
	__ATTR(txhex,			0644, es325_txhex_show, es325_txhex_set),
	__ATTR(clock_on,		0644, es325_clock_on_show, NULL),
	__ATTR(slim_ch_status,	0644, es325_slim_ch_show, NULL),
	__ATTR(es325_reg,		0644, es325_reg_show, es325_reg_write),
	__ATTR(es325_cmd_reg,	0644, es325_cmd_reg_show, NULL),
	__ATTR(firmware,		0644, NULL, es325_firmware_store),
	__ATTR(uart_set,		0644, NULL, es325_uart_set),
	__ATTR(power_stage,		0644, es325_power_stage_show, NULL),
};


/*
 * es325_sleep algorithm
 *
 *  1. set in 0 possible Smooth Mute time
 *  2. send Power Off command to es325
 *  3. write Sync command to es325
 *  4. if write fail then
 *         es325 is in sleep, return 0
 *  5. else wait 1 ms and read response
 *         if no response from es325 then
 *             es325 is in sleep, return 0
 *         else if Sync was write more then 20 times then
 *             was reach MAX_WAIT_TO_SLEEP time
 *             es325_sleep fail, return -1
 *         else goto point 3
 *
 * Note: After es325 switch to sleep mode and
 *       Sync write or read fails SLIMBus generates error message.
 *         "slim_read failed rc=-5" OR "slim_write failed rc=-5"
 *       SLIMBus error message indicates that es325 is in sleep
 */
static int es325_sleep(struct es325_priv *es325)
{
	char smooth_rate_cmd[] = {
		0x90, 0x4e, 0x00, 0x00 /* response not expected */
	};
	char pwr_cmd[] = {
		0x90, 0x10, 0x00, 0x01 /* response not expected */
	};
	int i = 0;
	int rc;
	struct slim_device *sbdev = es325->gen0_client;

	/*if (es325->power_stage == ES325_POWER_SLEEP) {
		dev_err(&sbdev->dev, "%s():Sleep called while sleeping/trying to sleep. power_state=%d\n", __func__, es325->power_stage);
		return rc =0;
		}*/


	dev_info(&sbdev->dev, "%s()\n", __func__);
	/* Set Smooth Mute period to 0 ms */
	rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET,
				ES325_WRITE_VE_WIDTH, smooth_rate_cmd, 4, 1);
	if (rc < 0)
		dev_err(&sbdev->dev, "%s():Sleep Smooth Mute Set to 0 Fail, rc=%d\n",
			__func__, rc);

	/* write pwr command, es325 to sleep mode */
	do {
		rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET,
			     ES325_WRITE_VE_WIDTH, pwr_cmd, 4, 1);
		if (rc == 0) {
			/* wait 20 ms according to spec end return.
			   eS325 is sleeping */
			msleep(20);
			break;
		}
		dev_dbg(&sbdev->dev, "%s():slim write fail, rc=%d\n", __func__,rc);
		usleep_range(SMB_DELAY, SMB_DELAY);
		i++;
	} while (i <= MAX_TRIALS);

	if (rc == 0)
		es325->power_stage = ES325_POWER_SLEEP;

	dev_info(&sbdev->dev, "%s()\n", __func__);
	return rc;
}

/*
 * es325_wakeup algorithm
 *
 *  1. generate es325 wakeup signal
 *  2. wait 30ms
 *  3. write / read Sync command up to 20 times
 *     with 1ms delay or until write / read success
 *  4. if write / read fail (wakeup fail) return error code
 *     else return 0
 *
 * Note: SLIMBus generates errors for unsucsessfull write / read
 *       commands. These errors indicates that es325 not ready yet
 */
static int es325_wakeup(struct es325_priv *es325)
{
	char sync_cmd[] = {
		0x80, 0x00, 0x00, 0x01 /* expecting response 0x80000001 */
	};
	int rc;
	char resp[4];
	struct slim_device *sbdev = es325->gen0_client;

	dev_info(&sbdev->dev, "%s()\n", __func__);
	if (es325->power_stage == ES325_POWER_AWAKE) {
		dev_err(&sbdev->dev, "Chip is already awake. power_state=%d\n", es325->power_stage);
		return rc = 0;
	}
	/* Get the control for wakeup GPIO */
	mutex_lock(&es325->wakeup_mutex);

	if (uart_enable) {
		gpio_direction_output(es325_priv.pdata->wakeup_gpio, 1);
		usleep_range(5000, 5100);
	}

	/* ASSUMES CLOCK HAS ALREADY BEEN ENABLED AND STABILIZED.
	   Follow recommended steps for es325 wakeup
	   1. Set Wakeup pin low ( H->L)
	   2.    Set Wakeup pin high (L->H)
	   3.    Wait 30 ms <<< delay moves into es325_request_response and
	                        combines with SYNC request / response delay
	                        es325 will be ready in time range 1 to 30 ms
	   4.    Send sync command */

#ifdef CONFIG_ES325_UART_WORKAROUND
#if defined(CONFIG_MACH_HLTESPR) && !defined(CONFIG_ES325_UART_WORKAROUND_ENG_ONLY)
	if (system_rev == 4) {
		pr_info("%s : [ES325] ES325_UART_WORKAROUND system rev = %d\n", __func__, system_rev);
		gpio_tlmm_config(GPIO_CFG(4, 0, GPIO_CFG_INPUT,
					GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
		gpio_tlmm_config(GPIO_CFG(5, 0, GPIO_CFG_INPUT,
					GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
		gpio_set_value(4, 1);
		gpio_set_value(5, 1);
		usleep_range(10000, 10000);
	}
#elif defined(CONFIG_MACH_HLTEUSC) && defined(CONFIG_ES325_UART_WORKAROUND_ENG_ONLY)
	if (system_rev == 4) {
		pr_info("%s : [ES325] ES325_UART_WORKAROUND system rev = %d\n", __func__, system_rev);
		gpio_tlmm_config(GPIO_CFG(4, 0, GPIO_CFG_INPUT,
					GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
		gpio_tlmm_config(GPIO_CFG(5, 0, GPIO_CFG_INPUT,
					GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
		gpio_set_value(4, 1);
		gpio_set_value(5, 1);
		usleep_range(10000, 10000);
	}
#elif (defined(CONFIG_MACH_HLTESKT) || defined(CONFIG_MACH_HLTEKTT) || defined(CONFIG_MACH_FRESCOLTESKT) || defined(CONFIG_MACH_FRESCOLTEKTT)) \
&& defined(CONFIG_ES325_UART_WORKAROUND_ENG_ONLY)
	if (system_rev == 5) {
		pr_info("%s : [ES325] ES325_UART_WORKAROUND system rev = %d\n", __func__, system_rev);
		gpio_tlmm_config(GPIO_CFG(4, 0, GPIO_CFG_INPUT,
					GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
		gpio_tlmm_config(GPIO_CFG(5, 0, GPIO_CFG_INPUT,
					GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
		gpio_set_value(4, 1);
		gpio_set_value(5, 1);
		usleep_range(10000, 10000);
	}
#else
#endif
#endif /* CONFIG_ES325_UART_WORKAROUND */

	/* Assert wakeup signal L->H. */
	gpio_set_value(es325->pdata->wakeup_gpio, 0);
	msleep(30);

	rc = es325_request_response(es325, sync_cmd, 4, 1, resp, 4, 1, 1, 0);
	if (rc < 0) {
		dev_info(&sbdev->dev, "%s():es325 wakeup FAIL\n", __func__);
	}

	/* Deassert wakeup signal L->H. */
	gpio_set_value(es325->pdata->wakeup_gpio, 1);

	if (uart_enable)
		gpio_direction_input(es325_priv.pdata->wakeup_gpio);

	usleep_range(5000, 5100);
	if (rc == 0)
		es325->power_stage = ES325_POWER_AWAKE;
	mutex_unlock(&es325->wakeup_mutex);
	dev_info(&sbdev->dev, "%s()\n", __func__);
	return rc;
}

static int es325_put_control_value(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc = 0;

	value = ucontrol->value.integer.value[0];
	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		return 0;
	}
	rc = es325_write(NULL, reg, value);

	return 0;
}

static int es325_get_control_value(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int value;

	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		ucontrol->value.integer.value[0] = 0;
		return 0;
	}

	value = es325_read(NULL, reg);
	ucontrol->value.integer.value[0] = value;

	return 0;
}

static int es325_put_control_enum(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	unsigned int reg = e->reg;
	unsigned int max = e->max;
	unsigned int value;
	int rc;

	dev_dbg(&sbdev->dev, "%s():reg = %d, max = %d\n", __func__, reg, max);
	value = ucontrol->value.enumerated.item[0];
	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		return 0;
	}

	rc = es325_write(NULL, reg, value);

	return 0;
}

static int es325_get_control_enum(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	unsigned int reg = e->reg;
	unsigned int max = e->max;
	unsigned int value;

	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		ucontrol->value.enumerated.item[0] = 0;
		return 0;
	}

	dev_dbg(&sbdev->dev, "%s():reg = %d, max = %d\n", __func__, reg, max);
	value = es325_read(NULL, reg);
	ucontrol->value.enumerated.item[0] = value;

	return 0;
}

static void es325_update_VEQ_enable(void)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	char msg[8] = {0xb0, 0x17, 0x00, 0x09, 0x90, 0x18, 0x00, 0x00}; /*VEQ Disable */
	int rc;

	if (es325_VEQ_enable_new != 0)
		return;

	if (es325_priv.wakeup_cnt) {
		if (es325_VEQ_enable_new)
			msg[7] = 0x01; /* VEQ Enable */

		dev_dbg(&sbdev->dev, "%s():write=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
				__func__, msg[0], msg[1], msg[2], msg[3],
				msg[4], msg[5], msg[6], msg[7]);
		rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET,
				ES325_WRITE_VE_WIDTH, msg, 4, 1);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
				__func__, rc);
			return;
		}
		rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET,
				ES325_WRITE_VE_WIDTH, msg+4, 4, 1);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
				__func__, rc);
			return;
		}
		es325_VEQ_enable = es325_VEQ_enable_new;
	}
	dev_info(&sbdev->dev, "%s VEQ enable value=%d =\n",__func__,es325_VEQ_enable);
}

static int es325_put_VEQ_enable_control(struct snd_kcontrol *kcontrol,
						struct snd_ctl_elem_value *ucontrol)
{
	es325_VEQ_enable_new = ucontrol->value.integer.value[0];
	return 0;
}

static int es325_get_VEQ_enable_control(struct snd_kcontrol *kcontrol,
						struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es325_VEQ_enable_new;
	return 0;
}

static void es325_update_BWE_enable(void)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	char msg[8] = {0xb0, 0x17, 0x00, 0x4f, 0x90, 0x18, 0x00, 0x00}; /* BWE Off */
	int rc;
	dev_info(&sbdev->dev, "[ES325] %s():curr=%d, new=%d\n", __func__, es325_BWE_enable, es325_BWE_enable_new);

	if (es325_BWE_enable == es325_BWE_enable_new)
		return;

	if (es325_priv.wakeup_cnt) {
		if (es325_BWE_enable_new)
			msg[7] = 0x01; /* BWE On */

		dev_info(&sbdev->dev, "%s():write=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			__func__, msg[0], msg[1], msg[2], msg[3],
			msg[4], msg[5], msg[6], msg[7]);
		rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET,
				ES325_WRITE_VE_WIDTH, msg, 4, 1);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
				__func__, rc);
			return;
		}
		rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET,
				ES325_WRITE_VE_WIDTH, msg+4, 4, 1);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
				__func__, rc);
			return;
		}
		es325_BWE_enable = es325_BWE_enable_new;
	}
	dev_info(&sbdev->dev, "%s BWE enable value=%d =\n", __func__, es325_BWE_enable);
}

static int es325_put_BWE_enable_control(struct snd_kcontrol *kcontrol,
						struct snd_ctl_elem_value *ucontrol)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	dev_info(&sbdev->dev, "%s():value=%d\n", __func__, es325_BWE_enable_new);
	es325_BWE_enable_new = ucontrol->value.integer.value[0];
	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		return 0;
	}

	es325_update_BWE_enable();
	return 0;
}

static int es325_get_BWE_enable_control(struct snd_kcontrol *kcontrol,
						struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es325_BWE_enable_new;
	return 0;
}

static void es325_update_Tx_NS(void)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	char msg[8] = {0xb0, 0x17, 0x00, 0x4b, 0x90, 0x18, 0x00, 0x00}; /* Tx NS level */
	int rc;

	if (es325_Tx_NS == es325_Tx_NS_new)
		return;

	if (es325_priv.wakeup_cnt) {
		msg[7] = es325_Tx_NS_new;
		dev_dbg(&sbdev->dev, "%s():write=0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			__func__, msg[0], msg[1], msg[2], msg[3],
			msg[4], msg[5], msg[6], msg[7]);
		rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET,
				ES325_WRITE_VE_WIDTH, msg, 4, 1);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
				__func__, rc);
			return;
		}
		rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET,
				ES325_WRITE_VE_WIDTH, msg+4, 4, 1);
		if (rc < 0) {
			dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
				__func__, rc);
			return;
		}
		es325_Tx_NS = es325_Tx_NS_new;
	}
	dev_info(&sbdev->dev, "%s Tx Ns value=%d =\n",__func__,es325_Tx_NS);
}

static int es325_put_Tx_NS_control(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	es325_Tx_NS_new = ucontrol->value.integer.value[0];
	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		return 0;
	}

	es325_update_Tx_NS();
	return 0;
}

static int es325_get_Tx_NS_control(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es325_Tx_NS_new;
	return 0;
}

static int es325_get_rx1_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es325_rx1_route_enable;
	pr_debug("%s():es325_rx1_route_enable = %d\n", __func__,
		es325_rx1_route_enable);

	return 0;
}

static int es325_put_rx1_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	es325_rx1_route_enable = ucontrol->value.integer.value[0];
	pr_debug("%s():es325_rx1_route_enable = %d\n", __func__,
		es325_rx1_route_enable);

	return 0;
}

static int es325_get_tx1_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es325_tx1_route_enable;
	pr_debug("%s():es325_tx1_route_enable = %d\n", __func__,
		es325_tx1_route_enable);

	return 0;
}

static int es325_put_tx1_route_enable_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	es325_tx1_route_enable = ucontrol->value.integer.value[0];
	pr_debug("%s():es325_tx1_route_enable = %d\n", __func__,
		es325_tx1_route_enable);

	return 0;
}

static int es325_get_rx2_route_enable_value(struct snd_kcontrol *kcontrol,
						struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = es325_rx2_route_enable;
	return 0;
}

static int es325_put_rx2_route_enable_value(struct snd_kcontrol *kcontrol,
						struct snd_ctl_elem_value *ucontrol)
{
	es325_rx2_route_enable = ucontrol->value.integer.value[0];
	return 0;
}

int es325_remote_route_enable(struct snd_soc_dai *dai)
{
	switch (dai->id) {
	case ES325_SLIM_1_PB:
		return es325_rx1_route_enable;
	case ES325_SLIM_1_CAP:
		return es325_tx1_route_enable;
	case ES325_SLIM_2_PB:
		return es325_rx2_route_enable;
	default:
		return 0;
	}
}
EXPORT_SYMBOL_GPL(es325_remote_route_enable);

static int es325_put_internal_route_config(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
#ifdef ES325_SLEEP
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;

	dev_info(&sbdev->dev, "%s():route = %ld\n", __func__, ucontrol->value.integer.value[0]);
	es325->new_internal_route_config = ucontrol->value.integer.value[0];
	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		return 0;
	}

	if(es325->new_internal_route_config == 5 && es325_priv.wakeup_cnt)
		es325_switch_route();

#else
	es325_switch_route(ucontrol->value.integer.value[0]);
#endif
	return 0;
}

static int es325_get_internal_route_config(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
#ifndef ES325_SLEEP
	ucontrol->value.integer.value[0] = es325_internal_route_num;
	return 0;
#else
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	dev_info(&sbdev->dev, " %s():route = %ld\n", __func__, es325_internal_route_num);
	ucontrol->value.integer.value[0] = es325->new_internal_route_config;
	return 0;
#endif
}

static int es325_put_network_type(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	int network = ucontrol->value.integer.value[0];

	dev_info(&sbdev->dev, "%s():new network type = %d\n", __func__, network);
	if (network != NARROW_BAND)
		es325_network_type = WIDE_BAND;
	else
		es325_network_type = NARROW_BAND;

	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		return 0;
	}


	mutex_lock(&es325_priv.pm_mutex);
	if (es325_priv.wakeup_cnt) {
		es325_switch_route();
	}
	mutex_unlock(&es325_priv.pm_mutex);

	return 0;
}

static int es325_get_network_type(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	ucontrol->value.integer.value[0] = es325_network_type;
	dev_dbg(&sbdev->dev, " %s():es325 network type = %d\n", __func__, es325_network_type);

	return 0;
}

int es325_set_VEQ_max_gain(int volume)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	char msg[4];
	static char cmd_str[8] = {0xb0, 0x17, 0x00, 0x00, 0x90, 0x18, 0x00, 0x00};
#if defined(CONFIG_MACH_HLTEVZW) || defined(CONFIG_MACH_HLTEUSC)
	/* 8 level Voice Rx volume for VZW */
	static char VEQ_max_gain[8] = {3, 3, 3, 5, 7, 9, 7, 4}; /* index 0 means max volume */
	static char VEQ_adj_gain[8] = {30, 2, 2, 2, 2, 2, 2, 2};
#elif defined(CONFIG_MACH_HLTETMO)
	static char VEQ_max_gain[6] = {3, 5, 7, 9, 7, 4}; /* index 0 means max volume */
	static char VEQ_adj_gain[6] = {25, 2, 2, 2, 2, 2};
#elif defined(CONFIG_MACH_HLTEATT) || defined(CONFIG_MACH_HLTEEUR) || defined(CONFIG_MACH_H3GDUOS)
	static char VEQ_max_gain[6] = {3, 5, 7, 9, 7, 4}; /* index 0 means max volume */
	static char VEQ_adj_gain[6] = {25, 2, 2, 2, 2, 2};
#elif defined(CONFIG_MACH_HLTESPR)
	static char VEQ_max_gain[6] = {5, 5, 7, 9, 7, 4}; /* index 0 means max volume */
	static char VEQ_adj_gain[6] = {30, 2, 2, 2, 2, 2};
#elif defined(CONFIG_MACH_ATLANTICLTE_ATT) || defined(CONFIG_MACH_ATLANTICLTE_USC)
	static char VEQ_max_gain[6] = {4, 4, 4, 4, 4, 4}; /* index 0 means max volume */
	static char VEQ_adj_gain[6] = {0x1E, 0x14, 0x14, 0x14, 0x14, 0x14};
#else
	static char VEQ_max_gain[6] = {3, 5, 7, 9, 7, 4}; /* index 0 means max volume */
	static char VEQ_adj_gain[6] = {0, 0, 0, 0, 0, 0};
#endif

	int rc;

	dev_info(&sbdev->dev, "%s():volume=%d, wakeup_cnt =%d\n", __func__,
		volume, es325_priv.wakeup_cnt);

	if (!es325_priv.wakeup_cnt) {
		pr_info("%s():ES325 is sleep. Skip to set the volume.\n", __func__);
		return 0;
	}

	cmd_str[3] = 0x3d; /* VEQ Max Gain ID */
	cmd_str[7] = VEQ_max_gain[volume];

	memcpy(msg, cmd_str, 4);
	dev_dbg(&sbdev->dev, "%s():write=0x%x, 0x%x, 0x%x, 0x%x\n", __func__, msg[0], msg[1], msg[2], msg[3]);
	rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET, ES325_WRITE_VE_WIDTH, msg, 4, 1);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
			__func__, rc);
		return rc;
	}
	memcpy(msg, cmd_str+4, 4);
	dev_dbg(&sbdev->dev, "%s():write=0x%x, 0x%x, 0x%x, 0x%x\n", __func__, msg[0], msg[1], msg[2], msg[3]);
	rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET, ES325_WRITE_VE_WIDTH, msg, 4, 1);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
			__func__, rc);
		return rc;
	}

	cmd_str[3] = 0x25; /* VEQ Noise Estimate Adj */
	cmd_str[7] = VEQ_adj_gain[volume];

	memcpy(msg, cmd_str, 4);
	dev_dbg(&sbdev->dev, "%s():write=0x%x, 0x%x, 0x%x, 0x%x\n", __func__, msg[0], msg[1], msg[2], msg[3]);
	rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET, ES325_WRITE_VE_WIDTH, msg, 4, 1);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
			__func__, rc);
		return rc;
	}
	memcpy(msg, cmd_str+4, 4);
	dev_dbg(&sbdev->dev, "%s():write=0x%x, 0x%x, 0x%x, 0x%x\n", __func__, msg[0], msg[1], msg[2], msg[3]);
	rc = ES325_BUS_WRITE(es325, ES325_WRITE_VE_OFFSET, ES325_WRITE_VE_WIDTH, msg, 4, 1);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():slim write fail, rc=%d\n",
			__func__, rc);
		return rc;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(es325_set_VEQ_max_gain);

static int es325_ap_put_tx1_ch_cnt(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	es325_ap_tx1_ch_cnt = ucontrol->value.enumerated.item[0] + 1;
	dev_dbg(&sbdev->dev, "%s():tx1 ch cnt = %d\n", __func__, es325_ap_tx1_ch_cnt);
	return 0;
}

static int es325_ap_get_tx1_ch_cnt(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	dev_dbg(&sbdev->dev, "%s():tx1 ch cnt = %d\n", __func__, es325_ap_tx1_ch_cnt);
	ucontrol->value.enumerated.item[0] = es325_internal_route_num - 1;
	return 0;
}

static const char * const es325_ap_tx1_ch_cnt_texts[] = {
	"One", "Two"
};

static const struct soc_enum es325_ap_tx1_ch_cnt_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(es325_ap_tx1_ch_cnt_texts), es325_ap_tx1_ch_cnt_texts);

/* generic gain translation */
static int es325_index_to_gain(int min, int step, int index)
{
	return	min + (step * index);
}
static int es325_gain_to_index(int min, int step, int gain)
{
	return	(gain - min) / step;
}

/* dereverb gain */
static int es325_put_dereverb_gain_value(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc;

	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		return 0;
	}

	if (ucontrol->value.integer.value[0] <= 12) {
		dev_info(&sbdev->dev, "%s():ucontrol = %ld\n", __func__, ucontrol->value.integer.value[0]);
		value = es325_index_to_gain(-12, 1, ucontrol->value.integer.value[0]);
		dev_info(&sbdev->dev, "%s():value = %d\n", __func__, value);
		rc = es325_write(NULL, reg, value);
	}

	return 0;
}

static int es325_get_dereverb_gain_value(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	unsigned int reg = mc->reg;
	unsigned int value;

	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		ucontrol->value.integer.value[0] = 0;
		return 0;
	}
	value = es325_read(NULL, reg);
	dev_info(&sbdev->dev, "%s():value = %d\n", __func__, value);
	ucontrol->value.integer.value[0] = es325_gain_to_index(-12, 1, value);
	dev_info(&sbdev->dev, "%s():ucontrol = %ld\n", __func__, ucontrol->value.integer.value[0]);

	return 0;
}

/* bwe high band gain */
static int es325_put_bwe_high_band_gain_value(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc;

	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		return 0;
	}

	if (ucontrol->value.integer.value[0] <= 30) {
		dev_info(&sbdev->dev, "%s():ucontrol = %ld\n", __func__, ucontrol->value.integer.value[0]);
		value = es325_index_to_gain(-10, 1, ucontrol->value.integer.value[0]);
		dev_info(&sbdev->dev, "%s():value = %d\n", __func__, value);
		rc = es325_write(NULL, reg, value);
	}

	return 0;
}

static int es325_get_bwe_high_band_gain_value(struct snd_kcontrol *kcontrol,
					      struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	unsigned int reg = mc->reg;
	unsigned int value;

	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		ucontrol->value.integer.value[0] = 0;
		return 0;
	}
	value = es325_read(NULL, reg);
	dev_info(&sbdev->dev, "%s():value = %d\n", __func__, value);
	ucontrol->value.integer.value[0] = es325_gain_to_index(-10, 1, value);
	dev_info(&sbdev->dev, "%s():ucontrol = %ld\n", __func__, ucontrol->value.integer.value[0]);

	return 0;
}

/* bwe max snr */
static int es325_put_bwe_max_snr_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	unsigned int reg = mc->reg;
	unsigned int value;
	int rc;

	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);			
		return 0;
	}	
	
	if (ucontrol->value.integer.value[0] <= 70) {
		dev_info(&sbdev->dev, "%s():ucontrol = %ld\n", __func__, ucontrol->value.integer.value[0]);
		value = es325_index_to_gain(-20, 1, ucontrol->value.integer.value[0]);
		dev_info(&sbdev->dev, "%s():value = %d\n", __func__, value);
		rc = es325_write(NULL, reg, value);
	}

	return 0;
}

static int es325_get_bwe_max_snr_value(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	unsigned int reg = mc->reg;
	unsigned int value;

	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		ucontrol->value.integer.value[0] = 0;
		return 0;
	}
	value = es325_read(NULL, reg);
	dev_info(&sbdev->dev, "%s():value = %d\n", __func__, value);
	ucontrol->value.integer.value[0] = es325_gain_to_index(-20, 1, value);
	dev_info(&sbdev->dev, "%s():ucontrol = %ld\n", __func__, ucontrol->value.integer.value[0]);

	return 0;
}

static const char * const es325_mic_config_texts[] = {
	"CT 2-mic", "FT 2-mic", "DV 1-mic", "EXT 1-mic", "BT 1-mic",
	"CT ASR 2-mic", "FT ASR 2-mic", "EXT ASR 1-mic", "FT ASR 1-mic",
};

static const struct soc_enum es325_mic_config_enum =
	SOC_ENUM_SINGLE(ES325_MIC_CONFIG, 0, ARRAY_SIZE(es325_mic_config_texts), es325_mic_config_texts);

static const char * const es325_aec_mode_texts[] = {
	"Off", "On", "rsvrd2", "rsvrd3", "rsvrd4", "On half-duplex"
};

static const struct soc_enum es325_aec_mode_enum =
	SOC_ENUM_SINGLE(ES325_AEC_MODE, 0, ARRAY_SIZE(es325_aec_mode_texts), es325_aec_mode_texts);

static const char * const es325_algo_rates_text[] = {
	"fs=8khz", "fs=16khz", "fs=24khz", "fs=48khz", "fs=96khz", "fs=192khz"
};

static const struct soc_enum es325_algo_sample_rate_enum =
	SOC_ENUM_SINGLE(ES325_ALGO_SAMPLE_RATE, 0, ARRAY_SIZE(es325_algo_rates_text), es325_algo_rates_text);

static const struct soc_enum es325_algo_mix_rate_enum =
	SOC_ENUM_SINGLE(ES325_MIX_SAMPLE_RATE, 0, ARRAY_SIZE(es325_algo_rates_text), es325_algo_rates_text);

static const char * const es325_algorithms_text[] = {
	"None", "VP", "Two CHREC", "AUDIO", "Four CHPASS"
};
static const struct soc_enum es325_algorithms_enum =
	SOC_ENUM_SINGLE(ES325_ALGO_SAMPLE_RATE, 0, ARRAY_SIZE(es325_algorithms_text), es325_algorithms_text);

static const char * const es325_off_on_texts[] = {
	"Off", "On"
};

static const struct soc_enum es325_veq_enable_enum =
	SOC_ENUM_SINGLE(ES325_VEQ_ENABLE, 0, ARRAY_SIZE(es325_off_on_texts), es325_off_on_texts);

static const struct soc_enum es325_dereverb_enable_enum =
	SOC_ENUM_SINGLE(ES325_DEREVERB_ENABLE, 0, ARRAY_SIZE(es325_off_on_texts), es325_off_on_texts);

static const struct soc_enum es325_bwe_enable_enum =
	SOC_ENUM_SINGLE(ES325_BWE_ENABLE, 0, ARRAY_SIZE(es325_off_on_texts), es325_off_on_texts);

static const struct soc_enum es325_bwe_post_eq_enable_enum =
	SOC_ENUM_SINGLE(ES325_BWE_POST_EQ_ENABLE, 0, ARRAY_SIZE(es325_off_on_texts), es325_off_on_texts);

static const struct soc_enum es325_algo_processing_enable_enum =
	SOC_ENUM_SINGLE(ES325_ALGO_PROCESSING, 0, ARRAY_SIZE(es325_off_on_texts), es325_off_on_texts);

static unsigned int es325_power_state;
static int es325_put_power_state_enum(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	dev_info(&sbdev->dev, "=ES325=%s():ucontrol = %d\n", __func__, ucontrol->value.enumerated.item[0]);
	dev_info(&sbdev->dev, "=ES325=%s():power state= %d\n", __func__, es325_power_state);

	if (es325_power_state == ucontrol->value.enumerated.item[0]) {
		dev_info(&sbdev->dev, "%s():no power state change\n", __func__);
		return 0;
	}
	es325_power_state = ucontrol->value.enumerated.item[0];

	if((es325_fw_downloaded == 0) || (es325_priv.wakeup_cnt == 0)) {
		pr_info("%s():es325 not ready, return\n", __func__);
		return 0;
	}

	if (es325_power_state)
		es325_wrapper_wakeup(0);
	else
		es325_wrapper_sleep(0);

	return 0;
}

static int es325_get_power_state_enum(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	dev_info(&sbdev->dev, "%s():power state = %d\n", __func__, es325_power_state);
	ucontrol->value.enumerated.item[0] = es325_power_state;
	dev_info(&sbdev->dev, "%s():ucontrol = %d\n", __func__, ucontrol->value.enumerated.item[0]);

	return 0;
}
static const char * const es325_power_state_texts[] = {
	"Sleep", "Active"
};
static const struct soc_enum es325_power_state_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(es325_power_state_texts), es325_power_state_texts);

static struct snd_kcontrol_new es325_digital_ext_snd_controls[] = {
	/* commit controls */
	SOC_SINGLE_EXT("ES325 RX1 Enable", SND_SOC_NOPM, 0, 1, 0,
			es325_get_rx1_route_enable_value, es325_put_rx1_route_enable_value),
	SOC_SINGLE_EXT("ES325 TX1 Enable", SND_SOC_NOPM, 0, 1, 0,
			es325_get_tx1_route_enable_value, es325_put_tx1_route_enable_value),
	SOC_SINGLE_EXT("ES325 RX2 Enable", SND_SOC_NOPM, 0, 1, 0,
			es325_get_rx2_route_enable_value, es325_put_rx2_route_enable_value),
	SOC_ENUM_EXT("Mic Config", es325_mic_config_enum,
			es325_get_control_enum, es325_put_control_enum),
	SOC_ENUM_EXT("AEC Mode", es325_aec_mode_enum,
			es325_get_control_enum, es325_put_control_enum),
	SOC_SINGLE_EXT("BWE Enable", SND_SOC_NOPM,0,1,0,
			es325_get_BWE_enable_control, es325_put_BWE_enable_control),
	SOC_SINGLE_EXT("VEQ Enable", SND_SOC_NOPM,0,1,0,
			es325_get_VEQ_enable_control, es325_put_VEQ_enable_control),
	SOC_SINGLE_EXT("ES325 Tx NS", SND_SOC_NOPM, 0, 100, 0,
			es325_get_Tx_NS_control, es325_put_Tx_NS_control),
	SOC_ENUM_EXT("Dereverb Enable", es325_dereverb_enable_enum,
			es325_get_control_enum, es325_put_control_enum),
	SOC_SINGLE_EXT("Dereverb Gain", ES325_DEREVERB_GAIN, 0, 100, 0,
			es325_get_dereverb_gain_value, es325_put_dereverb_gain_value),
	SOC_SINGLE_EXT("BWE High Band Gain", ES325_BWE_HIGH_BAND_GAIN, 0, 100, 0,
			es325_get_bwe_high_band_gain_value, es325_put_bwe_high_band_gain_value),
	SOC_SINGLE_EXT("BWE Max SNR", ES325_BWE_MAX_SNR, 0, 100, 0,
			es325_get_bwe_max_snr_value, es325_put_bwe_max_snr_value),
	SOC_ENUM_EXT("BWE Post EQ Enable", es325_bwe_post_eq_enable_enum,
			es325_get_control_enum, es325_put_control_enum),
	SOC_SINGLE_EXT("SLIMbus Link Multi Channel", ES325_SLIMBUS_LINK_MULTI_CHANNEL, 0, 65535, 0,
			es325_get_control_value, es325_put_control_value),
	SOC_ENUM_EXT("Set Power State", es325_power_state_enum,
			es325_get_power_state_enum, es325_put_power_state_enum),
	SOC_ENUM_EXT("Algorithm Processing", es325_algo_processing_enable_enum,
			es325_get_control_enum, es325_put_control_enum),
	SOC_ENUM_EXT("Algorithm Sample Rate", es325_algo_sample_rate_enum,
			es325_get_control_enum, es325_put_control_enum),
	SOC_ENUM_EXT("Algorithm", es325_algorithms_enum,
			es325_get_control_enum, es325_put_control_enum),
	SOC_ENUM_EXT("Mix Sample Rate", es325_algo_mix_rate_enum,
			es325_get_control_enum, es325_put_control_enum),
	SOC_SINGLE_EXT("Internal Route Config", SND_SOC_NOPM, 0, 100, 0,
			es325_get_internal_route_config, es325_put_internal_route_config),
	SOC_ENUM_EXT("ES325-AP Tx Channels", es325_ap_tx1_ch_cnt_enum,
			es325_ap_get_tx1_ch_cnt, es325_ap_put_tx1_ch_cnt),
	SOC_SINGLE_EXT("Current Network Type", SND_SOC_NOPM, 0, 1, 0,
			es325_get_network_type, es325_put_network_type)
};

static int es325_set_bias_level(struct snd_soc_codec *codec, enum snd_soc_bias_level level)
{
	int rc = 0;

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		break;

	case SND_SOC_BIAS_STANDBY:
		break;

	case SND_SOC_BIAS_OFF:
		break;
	}
	codec->dapm.bias_level = level;

	return rc;
}

static int es325_slim_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	int rc = 0;
	/* This callback function may be use
	   for the future version of es325 */
	return rc;
}

int es325_slim_set_channel_map(struct snd_soc_dai *dai, unsigned int tx_num,
			       unsigned int *tx_slot, unsigned int rx_num, unsigned int *rx_slot)
{
	struct snd_soc_codec *codec = dai->codec;
	struct es325_priv *es325 = &es325_priv;
	int id = dai->id;
	int i;
	int rc = 0;

	dev_dbg(codec->dev, "%s():dai->id = %d\n", __func__, dai->id);

	if (id == ES325_SLIM_1_PB || id == ES325_SLIM_2_PB || id == ES325_SLIM_3_PB) {
		es325->dai[ID(id)].ch_tot = rx_num;
		es325->dai[ID(id)].ch_act = 0;
		dev_dbg(codec->dev, "%s():id = %d ch_tot = %d\n", __func__, id, rx_num);
		for (i = 0; i < rx_num; i++) {
			es325->dai[ID(id)].ch_num[i] = rx_slot[i];
			dev_dbg(codec->dev, "%s():rx_slot[] = %d\n", __func__, rx_slot[i]);
		}
	} else if (id == ES325_SLIM_1_CAP || id == ES325_SLIM_2_CAP || id == ES325_SLIM_3_CAP) {
		es325->dai[ID(id)].ch_tot = tx_num;
		es325->dai[ID(id)].ch_act = 0;
		dev_dbg(codec->dev, "%s():id = %d ch_tot = %d\n", __func__, id, tx_num);
		for (i = 0; i < tx_num; i++) {
			es325->dai[ID(id)].ch_num[i] = tx_slot[i];
			dev_dbg(codec->dev, "%s():tx_slot[] = %d\n", __func__, tx_slot[i]);
		}
	}

	return rc;
}
EXPORT_SYMBOL_GPL(es325_slim_set_channel_map);

int es325_slim_get_channel_map(struct snd_soc_dai *dai,
			       unsigned int *tx_num, unsigned int *tx_slot,
			       unsigned int *rx_num, unsigned int *rx_slot)
{
	struct snd_soc_codec *codec = dai->codec;
	struct es325_priv *es325 = &es325_priv;
	struct es325_slim_ch *rx = es325->slim_rx;
	struct es325_slim_ch *tx = es325->slim_tx;
	int id = dai->id;
	int i;
	int rc = 0;

	dev_dbg(codec->dev, "%s():dai->id = %d\n", __func__, dai->id);

	if (id == ES325_SLIM_1_PB) {
		*rx_num = es325_dai[ID(id)].playback.channels_max;
		dev_dbg(codec->dev, "%s():*rx_num = %d\n", __func__, *rx_num);
		for (i = 0; i < *rx_num; i++) {
			rx_slot[i] = rx[ES325_SLIM_1_PB_OFFSET + i].ch_num;
			dev_dbg(codec->dev, "%s():rx_slot[%d] = %d\n", __func__, i, rx_slot[i]);
		}
	} else if (id == ES325_SLIM_2_PB) {
		*rx_num = es325_dai[ID(id)].playback.channels_max;
		dev_dbg(codec->dev, "%s():*rx_num = %d\n", __func__, *rx_num);
		for (i = 0; i < *rx_num; i++) {
			rx_slot[i] = rx[ES325_SLIM_2_PB_OFFSET + i].ch_num;
			dev_dbg(codec->dev, "%s():rx_slot[%d] = %d\n", __func__, i, rx_slot[i]);
		}
	} else if (id == ES325_SLIM_3_PB) {
		*rx_num = es325_dai[ID(id)].playback.channels_max;
		dev_dbg(codec->dev, "%s():*rx_num = %d\n", __func__, *rx_num);
		for (i = 0; i < *rx_num; i++) {
			rx_slot[i] = rx[ES325_SLIM_3_PB_OFFSET + i].ch_num;
			dev_dbg(codec->dev, "%s():rx_slot[%d] = %d\n", __func__, i, rx_slot[i]);
		}
	} else if (id == ES325_SLIM_1_CAP) {
		*tx_num = es325_dai[ID(id)].capture.channels_max;
		dev_dbg(codec->dev, "%s():*tx_num = %d\n", __func__, *tx_num);
		for (i = 0; i < *tx_num; i++) {
			tx_slot[i] = tx[ES325_SLIM_1_CAP_OFFSET + i].ch_num;
			dev_dbg(codec->dev, "%s():tx_slot[%d] = %d\n", __func__, i, tx_slot[i]);
		}
	} else if (id == ES325_SLIM_2_CAP) {
		*tx_num = es325_dai[ID(id)].capture.channels_max;
		dev_dbg(codec->dev, "%s():*tx_num = %d\n", __func__, *tx_num);
		for (i = 0; i < *tx_num; i++) {
			tx_slot[i] = tx[ES325_SLIM_2_CAP_OFFSET + i].ch_num;
			dev_dbg(codec->dev, "%s():tx_slot[%d] = %d\n", __func__, i, tx_slot[i]);
		}
	} else if (id == ES325_SLIM_3_CAP) {
		*tx_num = es325_dai[ID(id)].capture.channels_max;
		for (i = 0; i < *tx_num; i++) {
			tx_slot[i] = tx[ES325_SLIM_3_CAP_OFFSET + i].ch_num;
			dev_dbg(codec->dev, "%s():tx_slot[%d] = %d\n", __func__, i, tx_slot[i]);
		}
	}

	return rc;
}
EXPORT_SYMBOL_GPL(es325_slim_get_channel_map);

static int es325_slim_set_tristate(struct snd_soc_dai *dai, int tristate)
{
	int rc = 0;
	/* This callback function may be use
	   for the future version of es325 */
	return rc;
}

static int es325_slim_port_mute(struct snd_soc_dai *dai, int mute)
{
	int rc = 0;
	/* This callback function may be use
	   for the future version of es325 */
	return rc;
}

static int es325_slim_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	int rc = 0;
	/* This callback function may be use
	   for the future version of es325 */
	return rc;
}

static void es325_slim_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	/* This callback function may be use
	   for the future version of es325 */
}

int es325_slim_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct es325_priv *es325 = &es325_priv;
	int id = dai->id;
	int channels;
	int rate;
	int rc = 0;

	dev_dbg(codec->dev, "%s():stream_name = %s id = %d\n", __func__,
		es325_dai[ID(id)].playback.stream_name, es325_dai[ID(id)].id);

	channels = params_channels(params);
	switch (channels) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		es325->dai[ID(id)].ch_tot = channels;
		break;
	default:
		dev_err(codec->dev, "%s():unsupported number of channels, %d\n", __func__, channels);
		return -EINVAL;
	}
	rate = params_rate(params);
	switch (rate) {
	case 8000:
	case 16000:
	case 32000:
	case 48000:
		es325->dai[ID(id)].rate = rate;
		break;
	default:
		dev_err(codec->dev, "%s():unsupported rate, %d\n", __func__, rate);
		return -EINVAL;
	}
#ifdef ES325_SLEEP
	mutex_lock(&es325_priv.pm_mutex);
	if (es325->wakeup_cnt) {
		es325_switch_route();
		es325_update_VEQ_enable();
/*		es325_update_BWE_enable();*/
/*		es325_update_Tx_NS();*/
	}
	mutex_unlock(&es325_priv.pm_mutex);
#endif

	return rc;
}
EXPORT_SYMBOL_GPL(es325_slim_hw_params);

static int es325_slim_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	int rc = 0;

	dev_dbg(codec->dev, "%s()\n", __func__);
	return rc;
}

static int es325_slim_prepare(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	int rc = 0;
	/* This callback function may be use
	   for the future version of es325 */
	return rc;
}

int es325_slim_trigger(struct snd_pcm_substream *substream,
		       int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	int id = dai->id;
	int rc = 0;

	dev_info(codec->dev, "%s():stream_name = %s\n",
				__func__, es325_dai[ID(id)].playback.stream_name);
	dev_info(codec->dev, "%s():id = %d\n", __func__, es325_dai[ID(id)].id);
	dev_info(codec->dev, "%s():cmd = %d\n", __func__, cmd);

	return rc;
}
EXPORT_SYMBOL_GPL(es325_slim_trigger);

#define ES325_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
			SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
			SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000 |\
			SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_192000)
#define ES325_SLIMBUS_RATES (SNDRV_PCM_RATE_48000)

#define ES325_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE |\
			SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE |\
			SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S32_BE)
#define ES325_SLIMBUS_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE)

#if defined(CONFIG_SND_SOC_ES325_SLIM)
static struct snd_soc_dai_ops es325_slim_port_dai_ops = {
	.set_fmt	= es325_slim_set_dai_fmt,
	.set_channel_map	= es325_slim_set_channel_map,
	.get_channel_map	= es325_slim_get_channel_map,
	.set_tristate	= es325_slim_set_tristate,
	.digital_mute	= es325_slim_port_mute,
	.startup	= es325_slim_startup,
	.shutdown	= es325_slim_shutdown,
	.hw_params	= es325_slim_hw_params,
	.hw_free	= es325_slim_hw_free,
	.prepare	= es325_slim_prepare,
	.trigger	= es325_slim_trigger,
};
#endif

static struct snd_soc_dai_driver es325_dai[] = {
#if defined(CONFIG_SND_SOC_ES325_SLIM)
	{
		.name = "es325-slim-rx1",
		.id = ES325_SLIM_1_PB,
		.playback = {
			.stream_name = "SLIM_PORT-1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES325_SLIMBUS_RATES,
			.formats = ES325_SLIMBUS_FORMATS,
		},
		.ops = &es325_slim_port_dai_ops,
	},
	{
		.name = "es325-slim-tx1",
		.id = ES325_SLIM_1_CAP,
		.capture = {
			.stream_name = "SLIM_PORT-1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES325_SLIMBUS_RATES,
			.formats = ES325_SLIMBUS_FORMATS,
		},
		.ops = &es325_slim_port_dai_ops,
	},
	{
		.name = "es325-slim-rx2",
		.id = ES325_SLIM_2_PB,
		.playback = {
			.stream_name = "SLIM_PORT-2 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES325_SLIMBUS_RATES,
			.formats = ES325_SLIMBUS_FORMATS,
		},
		.ops = &es325_slim_port_dai_ops,
	},
	{
		.name = "es325-slim-tx2",
		.id = ES325_SLIM_2_CAP,
		.capture = {
			.stream_name = "SLIM_PORT-2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES325_SLIMBUS_RATES,
			.formats = ES325_SLIMBUS_FORMATS,
		},
		.ops = &es325_slim_port_dai_ops,
	},
	{
		.name = "es325-slim-rx3",
		.id = ES325_SLIM_3_PB,
		.playback = {
			.stream_name = "SLIM_PORT-3 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES325_SLIMBUS_RATES,
			.formats = ES325_SLIMBUS_FORMATS,
		},
		.ops = &es325_slim_port_dai_ops,
	},
	{
		.name = "es325-slim-tx3",
		.id = ES325_SLIM_3_CAP,
		.capture = {
			.stream_name = "SLIM_PORT-3 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = ES325_SLIMBUS_RATES,
			.formats = ES325_SLIMBUS_FORMATS,
		},
		.ops = &es325_slim_port_dai_ops,
	},
#endif
};

#ifdef CONFIG_PM
static int es325_codec_suspend(struct snd_soc_codec *codec)
{
	es325_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static int es325_codec_resume(struct snd_soc_codec *codec)
{
	es325_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	return 0;
}
#else
#define es325_codec_suspend NULL
#define es325_codec_resume NULL
#endif

int es325_remote_add_codec_controls(struct snd_soc_codec *codec)
{
	int rc;

	dev_info(codec->dev, "%s():codec->name = %s\n", __func__, codec->name);

	rc = snd_soc_add_codec_controls(codec, es325_digital_ext_snd_controls,
					ARRAY_SIZE(es325_digital_ext_snd_controls));
	if (rc)
		dev_err(codec->dev, "%s():es325_digital_ext_snd_controls failed\n", __func__);

	return rc;
}

static int es325_codec_probe(struct snd_soc_codec *codec)
{
	struct es325_priv *es325 = snd_soc_codec_get_drvdata(codec);

	dev_info(codec->dev, "%s():codec->name = %s codec = 0x%08x es325 = 0x%08x\n",
		__func__, codec->name, (unsigned int)codec, (unsigned int)es325);
	es325->codec = codec;

	codec->control_data = snd_soc_codec_get_drvdata(codec);
	dev_info(codec->dev, "%s():codec->control_data = 0x%08x\n",
		__func__, (unsigned int)codec->control_data);

	es325_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}

static int  es325_codec_remove(struct snd_soc_codec *codec)
{
	struct es325_priv *es325 = snd_soc_codec_get_drvdata(codec);
	es325_set_bias_level(codec, SND_SOC_BIAS_OFF);
	kfree(es325);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_es325 = {
	.probe =	es325_codec_probe,
	.remove =	es325_codec_remove,
	.suspend =	es325_codec_suspend,
	.resume =	es325_codec_resume,
	.read =		es325_read,
	.write =	es325_write,
	.set_bias_level =	es325_set_bias_level,
};

static int es325_slim_device_up(struct slim_device *sbdev);
static int es325_dt_parse_slim_interface_dev_info(struct device *dev,
						struct slim_device *slim_ifd)
{
	int rc;
	struct property *prop;

	dev_info(dev, "%s \n", __func__);
	rc = of_property_read_string(dev->of_node, "es325-slim-ifd", &slim_ifd->name);
	if (rc) {
		dev_info(dev, "%s():es325-slim-ifd\n : Looking up property in node %s failed",
			__func__, dev->of_node->full_name);
		return -ENODEV;
	}
	prop = of_find_property(dev->of_node, "es325-slim-ifd-elemental-addr", NULL);
	if (!prop) {
		dev_info(dev, " es325-slim-ifd-elemental-addr\n : Looking up %s property in node %s failed",
			__func__, dev->of_node->full_name);
		return -ENODEV;
	} else if (prop->length != 6) {
		dev_err(dev, " invalid codec slim ifd addr. addr length = %d\n", prop->length);
		return -ENODEV;
	}
	memcpy(slim_ifd->e_addr, prop->value, 6);

	return 0;
}

static struct esxxx_platform_data *es325_populate_dt_pdata(struct device *dev)
{
	struct esxxx_platform_data *pdata;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, " %s():could not allocate memory for platform data\n", __func__);
		goto err;
	}

	pdata->reset_gpio = of_get_named_gpio(dev->of_node, "es325-reset-gpio", 0);
	if (pdata->reset_gpio < 0) {
		dev_err(dev, "%s():es325-reset-gpio : Looking up property in node %s failed %d\n",
				__func__, dev->of_node->full_name, pdata->reset_gpio);
		goto alloc_err;
	}
	dev_dbg(dev, "%s():reset gpio %d\n", __func__, pdata->reset_gpio);

	pdata->wakeup_gpio = of_get_named_gpio(dev->of_node, "es325-wakeup-gpio", 0);
	if (pdata->wakeup_gpio < 0) {
		dev_err(dev, "%s():es325-wakeup-gpio : Looking up property in node %s failed %d\n",
				__func__, dev->of_node->full_name, pdata->wakeup_gpio);
		goto alloc_err;
	}
	dev_dbg(dev, "%s():wakeup gpio %d\n", __func__, pdata->wakeup_gpio);
	return pdata;

alloc_err:
	devm_kfree(dev, pdata);
err:
	return NULL;
}
#ifdef CONFIG_ARCH_MSM8226
static int es325_regulator_init(struct device *dev)
{
	int ret =0;
	struct device_node *reg_node = NULL;

	reg_node = of_parse_phandle(dev->of_node, "vdd-2mic-core-supply", 0);
	if(reg_node)
	{
		es325_ldo = regulator_get(dev, "vdd-2mic-core");
		if (IS_ERR(es325_ldo)) {
				pr_err("[%s] could not get earjack_ldo, %ld\n", __func__, PTR_ERR(es325_ldo));
		}
		else
		{
			ret = regulator_enable(es325_ldo);
			if(ret < 0) {
				pr_err("%s():Failed to enable regulator.\n",
					__func__);
				goto err_reg_enable;
			} else
				regulator_set_mode(es325_ldo, REGULATOR_MODE_NORMAL);
		}
	}else
		pr_err("%s Audience LDO node not available\n",__func__);

err_reg_enable:
	if(es325_ldo)
		regulator_put(es325_ldo);

	return ret;
}

static int es325_regulator_deinit(void)
{
	if(es325_ldo)
	{
		int ret;

		ret = regulator_disable(es325_ldo);
		if(ret < 0) {
			pr_err("%s():Failed to disable regulator.\n",__func__);
		}
		regulator_put(es325_ldo);
	}

	return 0;
}
#endif
static int es325_slim_probe(struct slim_device *sbdev)
{
	struct esxxx_platform_data *pdata = NULL;
	int rc;
	int cnt=0;

	dev_dbg(&sbdev->dev, "%s():sbdev->name = %s es325_priv = 0x%08x\n",
		__func__, sbdev->name, (unsigned int)&es325_priv);
#ifdef CONFIG_ARCH_MSM8226
	es325_regulator_init(&sbdev->dev);
#endif
	mutex_lock(&es325_priv.wakeup_mutex);
	if (sbdev->dev.of_node) {
		dev_info(&sbdev->dev, "%s():Platform data from device tree\n", __func__);
		pdata = es325_populate_dt_pdata(&sbdev->dev);
		rc = es325_dt_parse_slim_interface_dev_info(&sbdev->dev, &(pdata->intf_device));

		if (rc) {
			dev_info(&sbdev->dev, "%s():Error, parsing slim interface\n", __func__);
			devm_kfree(&sbdev->dev, pdata);
			rc = -EINVAL;
			return rc;
		}
		sbdev->dev.platform_data = pdata;

		es325_priv.gen0_client = sbdev;
		es325_priv.intf_client = &(pdata->intf_device);
	}

	slim_set_clientdata(sbdev, &es325_priv);
	if (strcmp(sbdev->name, "es325-ifd") == 0) {
		dev_dbg(&sbdev->dev, "%s():interface device probe\n", __func__);
		es325_priv.intf_client = sbdev;
		mutex_unlock(&es325_priv.wakeup_mutex);
		return 0;
	}

	if (strcmp(sbdev->name, "es325-gen") == 0) {
		dev_dbg(&sbdev->dev, "%s():generic device probe\n", __func__);
		es325_priv.gen0_client = sbdev;
	}

	if (es325_priv.intf_client == NULL || es325_priv.gen0_client == NULL) {
		dev_dbg(&sbdev->dev, "%s():incomplete initialization\n", __func__);
		mutex_unlock(&es325_priv.wakeup_mutex);
		return 0;
	}

	if (pdata == NULL) {
		dev_err(&sbdev->dev, "%s():pdata is NULL", __func__);
		rc = -EIO;
		mutex_unlock(&es325_priv.wakeup_mutex);
		goto pdata_error;
	}

	if(strcmp(sbdev->name, "es325-gen") == 0) {
		/* /sys/devices/fe12f000.slim/es325-gen/ */
		for (cnt = 0; cnt < ARRAY_SIZE(es325_device_attrs); cnt++) {
			rc = device_create_file(&sbdev->dev, &es325_device_attrs[cnt]);
			if (rc){
				dev_err(&sbdev->dev, "%s():failed to create debug  [%d]\n", __func__, cnt);
				device_remove_file(&sbdev->dev, &es325_device_attrs[cnt]);
			}
		}
	}

	dev_dbg(&sbdev->dev, "%s():reset_gpio = %d\n", __func__, pdata->reset_gpio);
	rc = gpio_request(pdata->reset_gpio, "es325_reset");
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():es325_reset request failed", __func__);
		goto reset_gpio_request_error;
	}
	rc = gpio_direction_output(pdata->reset_gpio, 0);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():es325_reset direction failed", __func__);
		goto reset_gpio_direction_error;
	}

	dev_dbg(&sbdev->dev, "%s():wakeup_gpio = %d\n", __func__, pdata->wakeup_gpio);
	rc = gpio_request(pdata->wakeup_gpio, "es325_wakeup");
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():es325_wakeup request failed", __func__);
		goto wakeup_gpio_request_error;
	}
	rc = gpio_direction_output(pdata->wakeup_gpio, 1);
	if (rc < 0) {
		dev_err(&sbdev->dev, "%s():es325_wakeup direction failed", __func__);
		goto wakeup_gpio_direction_error;
	}

	es325_enable_ext_clk(1);
	dev_dbg(&sbdev->dev, "%s():GPIO reset\n", __func__);
	gpio_set_value(pdata->reset_gpio, 0);
	/* Wait 2 ms then pull Reset signal in High */
	usleep_range(2000, 2100);
	gpio_set_value(pdata->reset_gpio, 1);
	/* Wait 10 ms then */
	usleep_range(15000, 16000);
	/* eSxxx is READY */

	es325_priv.pdata = pdata;
#ifdef ES325_SLEEP
	mutex_init(&es325_priv.pm_mutex);
	es325_priv.internal_route_config =  ES325_INTERNAL_ROUTE_MAX;
	es325_priv.new_internal_route_config = ES325_INTERNAL_ROUTE_MAX;
#endif
	mutex_unlock(&es325_priv.wakeup_mutex);
	dev_info(&sbdev->dev, "%s()\n", __func__);
	return 0;

wakeup_gpio_direction_error:
	gpio_free(pdata->wakeup_gpio);
wakeup_gpio_request_error:
reset_gpio_direction_error:
	gpio_free(pdata->reset_gpio);
reset_gpio_request_error:
pdata_error:
	es325_enable_ext_clk(0);
	dev_dbg(&sbdev->dev, "%s():rc = %d\n", __func__, rc);
	return rc;
}

static int register_snd_soc(struct es325_priv *priv)
{
	int rc;
	int i;
	int ch_cnt;
	struct slim_device *sbdev = priv->gen0_client;

	es325_init_slim_slave(sbdev);

	dev_dbg(&sbdev->dev, "%s():name = %s\n", __func__, sbdev->name);
	rc = snd_soc_register_codec(&sbdev->dev, &soc_codec_dev_es325,
					es325_dai, ARRAY_SIZE(es325_dai));
	dev_dbg(&sbdev->dev, "%s():rc = snd_soc_regsiter_codec() = %d\n", __func__, rc);

	/* allocate ch_num array for each DAI */
	for (i = 0; i < ARRAY_SIZE(es325_dai); i++) {
		switch (es325_dai[i].id) {
		case ES325_SLIM_1_PB:
		case ES325_SLIM_2_PB:
		case ES325_SLIM_3_PB:
			ch_cnt = es325_dai[i].playback.channels_max;
			break;
		case ES325_SLIM_1_CAP:
		case ES325_SLIM_2_CAP:
		case ES325_SLIM_3_CAP:
			ch_cnt = es325_dai[i].capture.channels_max;
			break;
		default:
			continue;
		}
		es325_priv.dai[i].ch_num = kzalloc((ch_cnt * sizeof(unsigned int)), GFP_KERNEL);
	}

	es325_priv.dai[ES325_SLIM_1_PB].ch_num[0] = SMB_RX_PORT0;
	es325_priv.dai[ES325_SLIM_1_PB].ch_num[1] = SMB_RX_PORT1;
	es325_priv.dai[ES325_SLIM_1_CAP].ch_num[0] = SMB_TX_PORT0;
	es325_priv.dai[ES325_SLIM_1_CAP].ch_num[1] = SMB_TX_PORT1;
	es325_priv.dai[ES325_SLIM_2_PB].ch_num[0] = SMB_RX_PORT2;
	es325_priv.dai[ES325_SLIM_2_PB].ch_num[1] = SMB_RX_PORT3;
	es325_priv.dai[ES325_SLIM_2_CAP].ch_num[0] = SMB_TX_PORT2;
	es325_priv.dai[ES325_SLIM_2_CAP].ch_num[1] = SMB_TX_PORT3;
	es325_priv.dai[ES325_SLIM_3_PB].ch_num[0] = SMB_RX_PORT4;
	es325_priv.dai[ES325_SLIM_3_PB].ch_num[1] = SMB_RX_PORT5;
	es325_priv.dai[ES325_SLIM_3_CAP].ch_num[0] = SMB_TX_PORT4;
	es325_priv.dai[ES325_SLIM_3_CAP].ch_num[1] = SMB_TX_PORT5;

	return rc;
}

static int es325_slim_remove(struct slim_device *sbdev)
{
	struct esxxx_platform_data *pdata = sbdev->dev.platform_data;

	dev_dbg(&sbdev->dev, "%s():sbdev->name = %s\n", __func__, sbdev->name);
#ifdef CONFIG_ARCH_MSM8226
	es325_regulator_deinit();
#endif
	gpio_free(pdata->reset_gpio);
	gpio_free(pdata->wakeup_gpio);
	gpio_free(pdata->gpioa_gpio);

	snd_soc_unregister_codec(&sbdev->dev);


	return 0;
}

#ifdef ES325_SLEEP

#define ES325_SLEEP_TIME	HZ
struct delayed_work es325_work;
struct workqueue_struct *es325_workqueue;
static int es325_schedule_sleep_workqueue(void)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	int rc = queue_delayed_work(es325_workqueue, &es325_work, ES325_SLEEP_TIME);
	if (!rc) {
		dev_err(&sbdev->dev, "%s delayed work queue failed\n", __func__);
		return -1;
	}
	es325->wq_active = 1;
	return 0;
}

static void es325_wrapper_wakeup_internal(void)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	dev_info(&sbdev->dev, "%s()\n", __func__);
	if (!es325->clock_on) {
		dev_info(&sbdev->dev, "%s enable clock\n", __func__);
		es325_enable_ext_clk(1);
		usleep_range(1000, 1000);
		es325_wakeup(es325);
		es325->clock_on = 1;
	}
}
#endif

void es325_wrapper_wakeup(struct snd_soc_dai *dai)
{
#ifdef ES325_SLEEP
	int rc;
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	dev_info(&sbdev->dev, "%s()\n", __func__);
	if(es325_fw_downloaded==0) {
		dev_info(&sbdev->dev, "%s():FW not ready, wakeup suspends, err_msg:%d\n", __func__,debug_for_dl_firmware);
		return;
	}
	dev_dbg(&sbdev->dev, "%s():dai_id=%d ch_wakeup=%d,wakeup_cnt=%d\n",
		__func__, dai->id, es325->dai[ID(dai->id)].ch_wakeup, es325->wakeup_cnt);
	if (!es325_remote_route_enable(dai)) {
		dev_info(&sbdev->dev, "%s():(dai->id = %d) es325 not activated %d %d %d\n",
			__func__, dai->id, es325_rx1_route_enable,
			es325_tx1_route_enable, es325_rx2_route_enable);
		return;
	}
	mutex_lock(&es325_priv.pm_mutex);
	es325->dai[ID(dai->id)].ch_wakeup = 1;
	if (es325->wakeup_cnt) {
		es325->wakeup_cnt++;
	} else {
		es325->wakeup_cnt++;
		if (es325->wq_active) {
			dev_dbg(&sbdev->dev, "%s():delete sleep timer wakeup count=%d\n",
				 __func__, es325->wakeup_cnt);
			if (delayed_work_pending(&es325_work)) {
				rc = cancel_delayed_work_sync(&es325_work);
				dev_dbg(&sbdev->dev, "%s():cancel work queue rc=%d\n", __func__, rc);
			}
			es325->wq_active = 0;
		}
		es325_wrapper_wakeup_internal();
	}
	mutex_unlock(&es325_priv.pm_mutex);
	dev_info(&sbdev->dev, "%s()\n", __func__);
#endif
}
EXPORT_SYMBOL_GPL(es325_wrapper_wakeup);

#ifdef ES325_SLEEP
 void es325_wrapper_sleep_internal(struct work_struct *dummy)
{
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;

	dev_dbg(&sbdev->dev, "%s():clock_on=%d\n", __func__, es325->clock_on);
	mutex_lock(&es325_priv.pm_mutex);
	es325->wq_active = 0;
	if (es325->wakeup_cnt) {
		dev_info(&sbdev->dev, "%s():wakeup needed before turning off clock[clock on==%d]\n",
			__func__, es325->clock_on);
	} else {
		if (es325->clock_on) {
			es325_sleep(es325);
			dev_info(&sbdev->dev, "%s():disable clock\n", __func__);
			es325_enable_ext_clk(0);
			es325->clock_on = 0;
		}
	}
	/* initialize variable to update ES325 settings when wake-up */
	es325->internal_route_config = ES325_INTERNAL_ROUTE_MAX;
	es325_VEQ_enable = ES325_MAX_INVALID_VEQ;
	es325_BWE_enable = ES325_MAX_INVALID_BWE;
	es325_Tx_NS = ES325_MAX_INVALID_TX_NS;

#if !defined(CONFIG_SEC_LOCALE_KOR)
	es325->new_internal_route_config = ES325_INTERNAL_ROUTE_MAX;
#endif

	es325_VEQ_enable_new = ES325_MAX_INVALID_VEQ;
	es325_BWE_enable_new = ES325_MAX_INVALID_BWE;
	es325_Tx_NS_new = ES325_MAX_INVALID_TX_NS;
	mutex_unlock(&es325_priv.pm_mutex);
}
#endif

void es325_wrapper_sleep(int dai_id)
{
#ifdef ES325_SLEEP
	struct es325_priv *es325 = &es325_priv;
	struct slim_device *sbdev = es325->gen0_client;
	dev_dbg(&sbdev->dev, "%s():dai_id=%d ch_wakeup=%d,wakeup_cnt=%d\n",
		__func__, dai_id, es325->dai[ID(dai_id)].ch_wakeup, es325->wakeup_cnt);
	mutex_lock(&es325_priv.pm_mutex);
	/* For dai not using audience */
	if (es325->dai[ID(dai_id)].ch_wakeup == 0) {
		dev_dbg(&sbdev->dev, "%s():dai_id=%d ch_wakeup=%d\n",
			__func__, dai_id, es325->dai[ID(dai_id)].ch_wakeup);
	} else {
		es325->dai[ID(dai_id)].ch_wakeup = 0;

		if (es325->wakeup_cnt)
			es325->wakeup_cnt--;

		if (es325->wakeup_cnt == 0 && !es325->wq_active)
			es325_schedule_sleep_workqueue();
	}
	mutex_unlock(&es325_priv.pm_mutex);
#endif
}
EXPORT_SYMBOL_GPL(es325_wrapper_sleep);

static int es325_slim_device_up(struct slim_device *sbdev)
{
	struct es325_priv *es325;
	int rc;
	const char *filename = CONFIG_EXTRA_FIRMWARE;
	dev_info(&sbdev->dev, "%s():name=%s\n", __func__, sbdev->name);
	dev_info(&sbdev->dev, "%s():laddr=%d\n", __func__, sbdev->laddr);

	mutex_lock(&es325_priv.wakeup_mutex);
	/* Start the firmware download in the workqueue context. */
	es325 = slim_get_devicedata(sbdev);
	rc = request_firmware((const struct firmware **)&es325->fw,
							filename, &sbdev->dev);
	if (rc) {
		dev_err(&sbdev->dev, "%s():request_firmware(%s) failed %d\n", __func__, filename, rc);
		return -1;
	}
	dev_info(&sbdev->dev, "%s():priv=%p\n", __func__, es325);

	if (strcmp(sbdev->name, "es325-ifd") == 0)
		return 0;

	/* device need time to finish send REPORT_PRESENT messages */
	msleep(50);
	rc = fw_download(es325);
	BUG_ON(rc != 0);

#ifdef ES325_SLEEP
	if (strcmp(sbdev->name, "es325-gen") == 0) {
		dev_info(&sbdev->dev, " wrapper %s():es325-gen\n", __func__);
		es325->clock_on = 1;
		es325_wrapper_sleep_internal(NULL);
		dev_info(&sbdev->dev, " wrapper %s():es325 sleep default\n", __func__);
	}
#endif
	mutex_unlock(&es325_priv.wakeup_mutex);
	dev_info(&sbdev->dev, " wrapper %s():device up complete\n", __func__);
	return rc;
}

static const struct slim_device_id es325_slim_id[] = {
	{ "es325", 0 },
	{ "es325-ifd", 0 },
	{ "es325-gen", 0 },
	{  }
};

static struct slim_driver es325_slim_driver = {
	.driver = {
		.name = "es325_gen_slim",
		.owner = THIS_MODULE,
	},
	.probe = es325_slim_probe,
	.remove = es325_slim_remove,
	.device_up = es325_slim_device_up,
	.id_table = es325_slim_id,
};

static __init int es325_init(void)
{
	int rc = 0;
	pr_info("%s():\n", __func__);
	memset(&es325_priv, 0, sizeof(es325_priv));
	mutex_init(&es325_priv.wakeup_mutex);
#ifdef ES325_SLEEP
	es325_workqueue = create_workqueue("ES325");
	if (!es325_workqueue) {
		pr_err("%s():can't create workqueue\n", __func__);
		return -1;
	}
	INIT_DELAYED_WORK(&es325_work, es325_wrapper_sleep_internal);
#endif

	pr_info("%s():slim_driver_register()", __func__);
	rc = slim_driver_register(&es325_slim_driver);
	if (rc)
		pr_err("%s():Error registering Audience eS325 SLIMbus driver: %d\n",
				__func__, rc);

	return rc;
}
module_init(es325_init);

static __exit void es325_exit(void)
{
	pr_info("%s():\n", __func__);
#ifdef ES325_SLEEP
	if (es325_workqueue)
		destroy_workqueue(es325_workqueue);
	es325_workqueue = NULL;
#endif

}
module_exit(es325_exit);


MODULE_DESCRIPTION("ASoC ES325 driver");
MODULE_AUTHOR("Greg Clemson <gclemson@audience.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:es325-codec");
MODULE_FIRMWARE(CONFIG_EXTRA_FIRMWARE);
