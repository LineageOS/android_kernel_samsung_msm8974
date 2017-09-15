/*
 * es705-routes.h  --  Audience eS705 ALSA SoC Audio driver
 *
 * Copyright 2013 Audience, Inc.
 *
 * Author: Greg Clemson <gclemson@audience.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ES705_ROUTES_H
#define _ES705_ROUTES_H

struct esxxx_route_config {
	const u32 *route;
	const u32 *nb;
	const u32 *wb;
	const u32 *swb;
	const u32 *fb;
};

enum {
	ROUTE_OFF,
	ROUTE_CS_VOICE_1MIC_CT,
	ROUTE_CS_VOICE_2MIC_CT,
	ROUTE_CS_VOICE_3MIC_CT,
	ROUTE_CS_VOICE_1MIC_FT,
	ROUTE_CS_VOICE_2MIC_FT,
	ROUTE_CS_VOICE_3MIC_FT,
	ROUTE_CS_VOICE_HEADSET,
	ROUTE_CS_VOICE_1MIC_HEADPHONE,
	ROUTE_VOIP_1MIC_CT,
	ROUTE_VOIP_2MIC_CT,
	ROUTE_VOIP_3MIC_CT,
	ROUTE_VOIP_1MIC_FT,
	ROUTE_VOIP_2MIC_FT,
	ROUTE_VOIP_3MIC_FT,
	ROUTE_VOIP_HEADSET,
	ROUTE_VOIP_1MIC_HEADPHONE,
	ROUTE_VOICE_ASR_1MIC,
	ROUTE_VOICE_ASR_2MIC,
	ROUTE_VOICE_ASR_3MIC,
	ROUTE_VOICESENSE_SBUSRX4,
	ROUTE_VOICESENSE_SBUSRX0,
	ROUTE_VOICESENSE_PDM,
	ROUTE_1CHAN_PLAYBACK,
	ROUTE_2CHAN_PLAYBACK,
	ROUTE_1CHAN_CAPTURE,
	ROUTE_2CHAN_CAPTURE,
	ROUTE_AUDIOZOOM_2MIC,
	ROUTE_AUDIOZOOM_3MIC,
	ROUTE_MAX
};

enum {
	RATE_NB,
	RATE_WB,
	RATE_SWB,
	RATE_FB,
	RATE_MAX
};

static const u32 route_off[] = {
	0xffffffff,
};

static const u32 route_cs_voice_1mic_ct[] = {
	0x903103e9,
	0xffffffff	/* terminate */
};
static const u32 nb_cs_voice_1mic_ct[] = {
	0x90310227,
	0xffffffff	/* terminate */
};
static const u32 wb_cs_voice_1mic_ct[] = {
	0x9031024f,
	0xffffffff	/* terminate */
};
static const u32 swb_cs_voice_1mic_ct[] = {
	0x90310250,
	0xffffffff	/* terminate */
};

static const u32 route_cs_voice_2mic_ct[] = {
	0x9031041b,	/* 1051, without PDM */
/*	0x90310577,	 1399 with PDM*/
	0xffffffff	/* terminate */
};
static const u32 nb_cs_voice_2mic_ct[] = {
	0x9031022a,
	0xffffffff	/* terminate */
};
static const u32 wb_cs_voice_2mic_ct[] = {
	0x9031022b,
	0xffffffff	/* terminate */
};
static const u32 swb_cs_voice_2mic_ct[] = {
	0x9031022c,
	0xffffffff	/* terminate */
};

static const u32 route_cs_voice_3mic_ct[] = {
	0x9031044d,	/* 1101, 3 mic, without PDM */
/*	0x903157d,	 1405, 3 mic, 1PDM-2Analog */
	0xffffffff	/* terminate */
};
static const u32 nb_cs_voice_3mic_ct[] = {
	0x9031022d,
	0xffffffff	/* terminate */
};
static const u32 wb_cs_voice_3mic_ct[] = {
	0x9031022e,
	0xffffffff	/* terminate */
};
static const u32 swb_cs_voice_3mic_ct[] = {
	0xffffffff	/* terminate */
};

static const u32 route_cs_voice_1mic_ft[] = {
	0x903103e9,
	0xffffffff	/* terminate */
};
static const u32 nb_cs_voice_1mic_ft[] = {
	0x90310230,
	0xffffffff	/* terminate */
};
static const u32 wb_cs_voice_1mic_ft[] = {
	0x9031024f,
	0xffffffff	/* terminate */
};
static const u32 swb_cs_voice_1mic_ft[] = {
	0x90310250,
	0xffffffff	/* terminate */
};

static const u32 route_cs_voice_2mic_ft[] = {
	0x9031041b,	/* 1051, without PDM  */
/*	0x90310577, 	1399, 2mic, PDM mic */
	0xffffffff	/* terminate */
};
static const u32 nb_cs_voice_2mic_ft[] = {
	0x9031023c,
	0xffffffff	/* terminate */
};
static const u32 wb_cs_voice_2mic_ft[] = {
	0x9031023a,
	0xffffffff	/* terminate */
};
static const u32 swb_cs_voice_2mic_ft[] = {
	0x9031023b,
	0xffffffff	/* terminate */
};

static const u32 route_cs_voice_3mic_ft[] = {
	0x9031044d,	/* 1101, without PDM */
	0xffffffff	/* terminate */
};
static const u32 nb_cs_voice_3mic_ft[] = {
	0x90310242,
	0xffffffff	/* terminate */
};
static const u32 wb_cs_voice_3mic_ft[] = {
	0x90310243,
	0xffffffff	/* terminate */
};
static const u32 swb_cs_voice_3mic_ft[] = {
	0xffffffff	/* terminate */
};

static const u32 route_cs_voice_headset[] = {
	0x903103ed,
	0xffffffff	/* terminate */
};
static const u32 nb_cs_voice_headset[] = {
	0x9031024e,
	0xffffffff	/* terminate */
};
static const u32 wb_cs_voice_headset[] = {
	0x9031024f,
	0xffffffff	/* terminate */
};
static const u32 swb_cs_voice_headset[] = {
	0xffffffff	/* terminate */
};

static const u32 route_cs_voice_1mic_headphone[] = {
	0x903103ed,
	0xffffffff	/* terminate */
};
static const u32 nb_cs_voice_1mic_headphone[] = {
	0xffffffff	/* terminate */
};
static const u32 wb_cs_voice_1mic_headphone[] = {
	0xffffffff	/* terminate */
};
static const u32 swb_cs_voice_1mic_headphone[] = {
	0xffffffff	/* terminate */
};

static const u32 route_voip_1mic_ct[] = {
	0x903103e9,
	0xffffffff	/* terminate */
};
static const u32 nb_voip_1mic_ct[] = {
	0x9031024e,
	0xffffffff	/* terminate */
};
static const u32 wb_voip_1mic_ct[] = {
	0x9031024f,
	0xffffffff	/* terminate */
};
static const u32 swb_voip_1mic_ct[] = {
	0x90310250,
	0xffffffff	/* terminate */
};

static const u32 route_voip_2mic_ct[] = {
	/*	0x9031041b,     without PDM  */
	0x90310577,	/* 1399 with PDM*/
	0xffffffff	/* terminate */
};
static const u32 nb_voip_2mic_ct[] = {
	0x9031022a,
	0xffffffff	/* terminate */
};
static const u32 wb_voip_2mic_ct[] = {
	0x9031022b,
	0xffffffff	/* terminate */
};
static const u32 swb_voip_2mic_ct[] = {
	0x9031022c,
	0xffffffff	/* terminate */
};
static const u32 fb_voip_2mic_ct[] = {
	0x90310272, /* 626 */
	0xffffffff	/* terminate */
};

static const u32 route_voip_3mic_ct[] = {
	0x9031057d,	/* 1405, 3 mic, 1PDM-2Analog */
	0xffffffff	/* terminate */
};
static const u32 nb_voip_3mic_ct[] = {
	0x9031022d, /* 557 */
	0xffffffff	/* terminate */
};
static const u32 wb_voip_3mic_ct[] = {
	0x9031022e, /* 558 */
	0xffffffff	/* terminate */
};
static const u32 swb_voip_3mic_ct[] = {
	0x9031022f, /* 559 */
	0xffffffff	/* terminate */
};
static const u32 fb_voip_3mic_ct[] = {
	0x90310273, /* 627 */
	0xffffffff	/* terminate */
};

static const u32 route_voip_1mic_ft[] = {
	0x903103e9,
	0xffffffff	/* terminate */
};
static const u32 nb_voip_1mic_ft[] = {
	0x9031024e,
	0xffffffff	/* terminate */
};
static const u32 wb_voip_1mic_ft[] = {
	0x9031024f,
	0xffffffff	/* terminate */
};
static const u32 swb_voip_1mic_ft[] = {
	0x90310250,
	0xffffffff	/* terminate */
};

static const u32 route_voip_2mic_ft[] = {
/*	0x9031041b, Analog Mic */
	0x90310577, /* 1399, 2mic, PDM mic */
	0xffffffff	/* terminate */
};
static const u32 nb_voip_2mic_ft[] = {
	0x90310239,
	0xffffffff	/* terminate */
};
static const u32 wb_voip_2mic_ft[] = {
	0x9031023a,
	0xffffffff	/* terminate */
};
static const u32 swb_voip_2mic_ft[] = {
	0x9031023b,
	0xffffffff	/* terminate */
};
static const u32 fb_voip_2mic_ft[] = {
	0x90310277, /* 631 */
	0xffffffff	/* terminate */
};

static const u32 route_voip_3mic_ft[] = {
	0x90310595,	/* 1429, 3 mic, 1PDM-2Analog, Ter is PDM Mic */
	0xffffffff	/* terminate */
};
static const u32 nb_voip_3mic_ft[] = {
	0x90310242, /* 578 */
	0xffffffff	/* terminate */
};
static const u32 wb_voip_3mic_ft[] = {
	0x90310243, /* 579 */
	0xffffffff	/* terminate */
};
static const u32 swb_voip_3mic_ft[] = {
	0x90310244, /* 580 */
	0xffffffff	/* terminate */
};
static const u32 fb_voip_3mic_ft[] = {
	0x9031027a, /* 634 */
	0xffffffff	/* terminate */
};

static const u32 route_voip_headset[] = {
	0x903103ed,
	0xffffffff	/* terminate */
};
static const u32 nb_voip_headset[] = {
	0x9031024e,
	0xffffffff	/* terminate */
};
static const u32 wb_voip_headset[] = {
	0x9031024f,
	0xffffffff	/* terminate */
};
static const u32 swb_voip_headset[] = {
	0xffffffff	/* terminate */
};

static const u32 route_voip_1mic_headphone[] = {
	0x903103ed,
	0xffffffff	/* terminate */
};
static const u32 nb_voip_1mic_headphone[] = {
	0xffffffff	/* terminate */
};
static const u32 wb_voip_1mic_headphone[] = {
	0xffffffff	/* terminate */
};
static const u32 swb_voip_1mic_headphone[] = {
	0xffffffff	/* terminate */
};

static const u32 route_voice_asr_1mic[] = {
	0x903104b1,
	0xffffffff	/* terminate */
};
static const u32 nb_voice_asr_1mic[] = {
	0xffffffff	/* terminate */
};
static const u32 wb_voice_asr_1mic[] = {
	0xffffffff	/* terminate */
};
static const u32 swb_voice_asr_1mic[] = {
	0xffffffff	/* terminate */
};

static const u32 route_voice_asr_2mic[] = {
	0x903104ca,
	0xffffffff	/* terminate */
};
static const u32 nb_voice_asr_2mic[] = {
	0xffffffff	/* terminate */
};
static const u32 wb_voice_asr_2mic[] = {
	0xffffffff	/* terminate */
};
static const u32 swb_voice_asr_2mic[] = {
	0xffffffff	/* terminate */
};

static const u32 route_voice_asr_3mic[] = {
	0xffffffff	/* terminate */
};
static const u32 nb_voice_asr_3mic[] = {
	0xffffffff	/* terminate */
};
static const u32 wb_voice_asr_3mic[] = {
	0xffffffff	/* terminate */
};
static const u32 swb_voice_asr_3mic[] = {
	0xffffffff	/* terminate */
};

static const u32 route_voicesense_sbusrx4[] = {
	0xb05a0a40,	/* SBUS.Rx4 -> RxChMgr0*/
	0xb05b0000,	/* Set PathId  RxChMgr0 PATH_ID_PRI */
	0xb0640038,	/* connect RxChMgr0.o0 */
	0xb0640190,	/* connect senseVoice.i0 */
	0xb0630003,	/* set rate RxChMgr0 8k*/
	0xb0630019,	/* set rate senseVoice 8k*/
	0xb0680300,	/* set group RxChMgr0 0*/
	0xb0681900,	/* set group sensevoice 0*/
	0xb00c0900,	/* setDeviceParamID set Multichannel Link Bits*/
	0x900d0000,	/* setDeviceParam Slimbus Linked*/
	0xffffffff	/* terminate */
};

static const u32 route_voicesense_pdm[] = {
	0x90310566,
	0xffffffff      /* terminate */
};

static const u32 route_sensory_pdm[] = {
	0xb05a0c00,
	0xb05b0000,
	0xb0640038,
	0xb0640190,
	0xb0630103,
	0xb0630119,
	0xb0680300,
	0xb0681900,
	0x9017e02b, /* Sensory input gain reg = 0xe02b */
	0x90181000, /* Sensory input gain value = 0x1000 */
	0xffffffff,

};

static const u32 route_voicesense_sbusrx0[] = {
	0xb05a0a00,	/* SBUS.Rx0 -> RxChMgr0*/
	0xb05b0000,	/* Set PathId  RxChMgr0 PATH_ID_PRI */
	0xb0640038,	/* connect RxChMgr0.o0 */
	0xb0640190,	/* connect senseVoice.i0 */
	0xb0630003,	/* set rate RxChMgr0 8k*/
	0xb0630019,	/* set rate senseVoice 8k*/
	0xb0680300,	/* set group RxChMgr0 0*/
	0xb0681900,	/* set group sensevoice 0*/
	0xb00c0900,	/* setDeviceParamID set Multichannel Link Bits*/
	0x900d0000,	/* setDeviceParam Slimbus Linked*/
	0xffffffff	/* terminate */
};

static const u32 route_1chan_playback[] = {
	0x9031047f,
	0xffffffff	/* terminate */
};

static const u32 route_2chan_playback[] = {
	0x90310483,
	0xffffffff	/* terminate */
};

static const u32 route_1chan_capture[] = {
	0x90310480,
	0xffffffff	/* terminate */
};

static const u32 route_2chan_capture[] = {
	0x90310484,
	0xffffffff	/* terminate */
};

static const u32 route_audiozoom_2mic[] = {
	0x9031054b,
	0xffffffff	/* terminate */
};

static const u32 route_audiozoom_3mic[] = {
	0x9031054d,
	0xffffffff	/* terminate */
};

static const struct esxxx_route_config es705_route_config[ROUTE_MAX] = {
	[ROUTE_OFF] = {
		.route = route_off,
	},
	[ROUTE_CS_VOICE_1MIC_CT] = {
		.route = route_cs_voice_1mic_ct,
		.nb = nb_cs_voice_1mic_ct,
		.wb = wb_cs_voice_1mic_ct,
		.swb = swb_cs_voice_1mic_ct,
	},
	[ROUTE_CS_VOICE_2MIC_CT] = {
		.route = route_cs_voice_2mic_ct,
		.nb = nb_cs_voice_2mic_ct,
		.wb = wb_cs_voice_2mic_ct,
		.swb = swb_cs_voice_2mic_ct,
	},
	[ROUTE_CS_VOICE_3MIC_CT] = {
		.route = route_cs_voice_3mic_ct,
		.nb = nb_cs_voice_3mic_ct,
		.wb = wb_cs_voice_3mic_ct,
		.swb = swb_cs_voice_3mic_ct,
	},
	[ROUTE_CS_VOICE_1MIC_FT] = {
		.route = route_cs_voice_1mic_ft,
		.nb = nb_cs_voice_1mic_ft,
		.wb = wb_cs_voice_1mic_ft,
		.swb = swb_cs_voice_1mic_ft,
	},
	[ROUTE_CS_VOICE_2MIC_FT] = {
		.route = route_cs_voice_2mic_ft,
		.nb = nb_cs_voice_2mic_ft,
		.wb = wb_cs_voice_2mic_ft,
		.swb = swb_cs_voice_2mic_ft,
	},
	[ROUTE_CS_VOICE_3MIC_FT] = {
		.route = route_cs_voice_3mic_ft,
		.nb = nb_cs_voice_3mic_ft,
		.wb = wb_cs_voice_3mic_ft,
		.swb = swb_cs_voice_3mic_ft,
	},
	[ROUTE_CS_VOICE_HEADSET] = {
		.route = route_cs_voice_headset,
		.nb = nb_cs_voice_headset,
		.wb = wb_cs_voice_headset,
		.swb = swb_cs_voice_headset,
	},
	[ROUTE_CS_VOICE_1MIC_HEADPHONE] = {
		.route = route_cs_voice_1mic_headphone,
		.nb = nb_cs_voice_1mic_headphone,
		.wb = wb_cs_voice_1mic_headphone,
		.swb = swb_cs_voice_1mic_headphone,
	},
	[ROUTE_VOIP_1MIC_CT] = {
		.route = route_voip_1mic_ct,
		.nb = nb_voip_1mic_ct,
		.wb = wb_voip_1mic_ct,
		.swb = swb_voip_1mic_ct,
	},
	[ROUTE_VOIP_2MIC_CT] = {
		.route = route_voip_2mic_ct,
		.nb = nb_voip_2mic_ct,
		.wb = wb_voip_2mic_ct,
		.swb = swb_voip_2mic_ct,
		.fb = fb_voip_2mic_ct,
	},
	[ROUTE_VOIP_3MIC_CT] = {
		.route = route_voip_3mic_ct,
		.nb = nb_voip_3mic_ct,
		.wb = wb_voip_3mic_ct,
		.swb = swb_voip_3mic_ct,
		.fb = fb_voip_3mic_ct,
	},
	[ROUTE_VOIP_1MIC_FT] = {
		.route = route_voip_1mic_ft,
		.nb = nb_voip_1mic_ft,
		.wb = wb_voip_1mic_ft,
		.swb = swb_voip_1mic_ft,
	},
	[ROUTE_VOIP_2MIC_FT] = {
		.route = route_voip_2mic_ft,
		.nb = nb_voip_2mic_ft,
		.wb = wb_voip_2mic_ft,
		.swb = swb_voip_2mic_ft,
		.fb = fb_voip_2mic_ft,
	},
	[ROUTE_VOIP_3MIC_FT] = {
		.route = route_voip_3mic_ft,
		.nb = nb_voip_3mic_ft,
		.wb = wb_voip_3mic_ft,
		.swb = swb_voip_3mic_ft,
		.fb = fb_voip_3mic_ft,
	},
	[ROUTE_VOIP_HEADSET] = {
		.route = route_voip_headset,
		.nb = nb_voip_headset,
		.wb = wb_voip_headset,
		.swb = swb_voip_headset,
	},
	[ROUTE_VOIP_1MIC_HEADPHONE] = {
		.route = route_voip_1mic_headphone,
		.nb = nb_voip_1mic_headphone,
		.wb = wb_voip_1mic_headphone,
		.swb = swb_voip_1mic_headphone,
	},
	[ROUTE_VOICE_ASR_1MIC] = {
		.route = route_voice_asr_1mic,
		.nb = nb_voice_asr_1mic,
		.wb = wb_voice_asr_1mic,
		.swb = swb_voice_asr_1mic,
	},
	[ROUTE_VOICE_ASR_2MIC] = {
		.route = route_voice_asr_2mic,
		.nb = nb_voice_asr_2mic,
		.wb = wb_voice_asr_2mic,
		.swb = swb_voice_asr_2mic,
	},
	[ROUTE_VOICE_ASR_3MIC] = {
		.route = route_voice_asr_3mic,
		.nb = nb_voice_asr_3mic,
		.wb = wb_voice_asr_3mic,
		.swb = swb_voice_asr_3mic,
	},
	[ROUTE_VOICESENSE_SBUSRX4] = {
		.route = route_voicesense_sbusrx4,
	},
	[ROUTE_VOICESENSE_SBUSRX0] = {
		.route = route_voicesense_sbusrx0,
	},
	[ROUTE_VOICESENSE_PDM] = {
		.route = route_voicesense_pdm,
	},
	[ROUTE_1CHAN_PLAYBACK] = {
		.route = route_1chan_playback,
	},
	[ROUTE_2CHAN_PLAYBACK] = {
		.route = route_2chan_playback,
	},
	[ROUTE_1CHAN_CAPTURE] = {
		.route = route_1chan_capture,
	},
	[ROUTE_2CHAN_CAPTURE] = {
		.route = route_2chan_capture,
	},
	[ROUTE_AUDIOZOOM_2MIC] = {
		.route = route_audiozoom_2mic,
	},
	[ROUTE_AUDIOZOOM_3MIC] = {
		.route = route_audiozoom_3mic,
	},
};

static const u32 route_1Mic_WHS_NB_SWBypass[] = {
	0x903103ed,	/* Route, #1005 */
	0x9031024e,	/* Algo, #590  */
	0xffffffff	/* terminate */
};

#if  defined(CONFIG_MACH_KACTIVELTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_ATT) || defined(CONFIG_SEC_S_PROJECT) || defined(CONFIG_MACH_KACTIVELTE_CAN) \
|| defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_HESTIALTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_KOR)
static const u32 route_2Mic_SPK_NB_FO_NSOn[] = {
	0x9031041b,	/* Route, #1051 */
	0x90310395,	/* Algo, #917 */
	0xffffffff	/* terminate */
};
#endif

static const u32 route_2Mic_SPK_NB_NSOn[] = {
	0x9031041b,	/* Route, #1051 */
	0x9031023c,	/* Algo, #572 */
	0xffffffff	/* terminate */
};

static const u32 route_2Mic_HS_NB_NSOn[] = {
	0x9031041b,	/* Route, #1051 */
	0x9031022a,	/* Algo, #554 */
	0xffffffff	/* terminate */
};

#if  defined(CONFIG_MACH_KACTIVELTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_ATT) || defined(CONFIG_SEC_S_PROJECT) || defined(CONFIG_MACH_KACTIVELTE_CAN) \
|| defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_HESTIALTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_KOR)
static const u32 route_2Mic_SPK_NB_FO_NSOff[] = {
	0x9031041b,	/* Route, #1051 */
	0x90310395,	/* Algo, #917 */
	0x903103e6,	/* Algo, #998 */	
	0xffffffff	/* terminate */
};
#endif

static const u32 route_2Mic_SPK_NB_NSOff[] = {
	0x9031041b,	/* Route, #1051 */
	0x9031026d,	/* Algo, #621 */
	0xffffffff	/* terminate */
};

static const u32 route_2Mic_HS_NB_NSOff[] = {
	0x9031041b,	/* Route, #1051 */
	0x9031022a,	/* Algo, #554 */
	0x903103bd,	/* Algo, #957 */
	0xffffffff	/* terminate */
};

static const u32 route_1Ch_AudioPB[] = {
	0x9031047f,	/* Route, #1151 */
	0x90310353,	/* Algo, #851 */
	0xffffffff	/* terminate */
};

static const u32 route_2Ch_AudioPB[] = {
	0x90310484,	/* Route, #1156 */
	0x90310358,	/* Algo, #856 */
	0xffffffff	/* terminate */
};

static const u32 route_2Mic_AudioZoomInterview[] = {
	0x9031054b,	/* Route, #1355 */
	0x903102f0,	/* Algo, #752 */
	0xffffffff	/* terminate */
};

static const u32 route_2Mic_AudioZoomConversation[] = {
	0x9031054b,	/* Route, #1355 */
	0x903102f1,	/* Algo, #753 */
	0xffffffff	/* terminate */
};

static const u32 route_dummy[] = {
	0xffffffff	/* terminate */
};

static const u32 route_TTYVCO[] = {
	0x903103ed,	/* Route, #1005 */
	0x9031024e,	/* Algo, #590  */
	0x9031026d,	/* VP Off, #621 */
	0xffffffff	/* terminate */
};

#if  defined(CONFIG_MACH_KACTIVELTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_ATT) || defined(CONFIG_SEC_S_PROJECT) || defined(CONFIG_MACH_KACTIVELTE_CAN) \
|| defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_HESTIALTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_KOR)
static const u32 route_TTYHCO[] = {
	0x903103e9,	/* Route, #1001 */
	0x9031024e,	/* Algo, #590  */
	0x9031026d,	/* VP Off, #621 */
	0xffffffff	/* terminate */
};

static const u32 route_2Mic_ASR_AO[] = {
	0x903104ca,	/* Route, #1226 */
	0x903103b2,	/* Algo, #946  */
	0xffffffff	/* terminate */
};
#else
static const u32 route_TTYHCO[] = {
	0x903103e9,	/* Route, #1001 */
	0x9031024e,	/* Algo, #590  */
	0x9031026d,	/* VP Off, #621 */
	0xffffffff	/* terminate */
};

static const u32 route_2Mic_ASR[] = {
	0x903104ca,	/* Route, #1226 */
	0x90310260,	/* Algo, #608  */
	0xffffffff	/* terminate */
};
#endif
static const u32 route_1Mic_HS_LB_SWBypass[] = {
	0x903103e9,	/* Route, #1001 */
	0x9031026d,	/* VP Off, #621 */
	0xffffffff	/* terminate */
};

static const u32 route_1Mic_SPK_LB_SWBypass[] = {
	0x903103e9,	/* Route, #1001 */
	0x9031026d,	/* VP Off, #621 */
	0xffffffff	/* terminate */
};

static const u32 route_1Mic_WHS_LB_SWBypass[] = {
	0x903103ed,	/* Route, #1005 */
	0x9031026d,	/* VP Off, #621 */
	0xffffffff	/* terminate */
};

static const u32 route_1Mic_WHS_WB_SWBypass[] = {
	0x903103ed,	/* Route, #1005 */
	0x9031024f,	/* Algo, #591  */
	0x9031026d,	/* VP Off, #621 */
	0xffffffff	/* terminate */
};

#if  defined(CONFIG_MACH_KACTIVELTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_ATT) || defined(CONFIG_SEC_S_PROJECT) || defined(CONFIG_MACH_KACTIVELTE_CAN) \
|| defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_HESTIALTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_KOR)
static const u32 route_2Mic_SPK_WB_FO_NSOn[] = {
	0x9031041b,	/* Route, #1051 */
	0x90310396,	/* Algo, #918 */
	0xffffffff	/* terminate */
};
#endif

static const u32 route_2Mic_SPK_WB_NSOn[] = {
	0x9031041b,	/* Route, #1051 */
	0x9031023a,	/* Algo, #570 */
	0xffffffff	/* terminate */
};

static const u32 route_2Mic_HS_WB_NSOn[] = {
	0x9031041b,	/* Route, #1051 */
	0x9031022b,	/* Algo, #555 */
	0xffffffff	/* terminate */
};

#if  defined(CONFIG_MACH_KACTIVELTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_ATT) || defined(CONFIG_SEC_S_PROJECT) || defined(CONFIG_MACH_KACTIVELTE_CAN) \
|| defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_HESTIALTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_KOR)
static const u32 route_2Mic_SPK_WB_FO_NSOff[] = {
	0x9031041b,	/* Route, #1051 */
	0x90310396,	/* Algo, #918 */
	0x903103e7,	/* Algo, #999 */	
	0xffffffff	/* terminate */
};
#endif

static const u32 route_2Mic_SPK_WB_NSOff[] = {
	0x9031041b,	/* Route, #1051 */
	0x9031024f,	/* Algo, #591 */
	0xffffffff	/* terminate */
};

static const u32 route_2Mic_HS_WB_NSOff[] = {
	0x9031041b,	/* Route, #1051 */
	0x9031022b,	/* Algo, #555 */
	0x903103be,	/* Algo, #958 */
	0xffffffff	/* terminate */
};

static const u32 route_3Mic_SPK_NB_FO_NSOn[] = {
	0x9031044d,	/* Route, #1101 */
	0x9031039d,	/* Algo, #925 */
	0xffffffff	/* terminate */
};

static const u32 route_3Mic_SPK_NB_FO_NSOff[] = {
	0x9031044d,	/* Route, #1101 */
	0x9031039d,	/* Algo, #925 */
	0x903103e6,	/* Algo, #998 */
	0xffffffff	/* terminate */
};

static const u32 route_3Mic_AudioZoomInterview[] = {
	0x9031054d,	/* Route, #1357 */
	0x903102f0,	/* Algo, #752 */
	0xffffffff	/* terminate */
};

static const u32 route_3Mic_AudioZoomConversation[] = {
	0x9031054d,	/* Route, #1357 */
	0x903102f1,	/* Algo, #753 */
	0xffffffff	/* terminate */
};

static const u32 route_3Mic_ASR_AO[] = {
	0x903104e3,	/* Route, #1251 */
	0x903103ba,	/* Algo, #954  */
	0xffffffff	/* terminate */
};

static const u32 route_3Mic_SPK_WB_FO_NSOn[] = {
	0x9031044d,	/* Route, #1101 */
	0x9031039e,	/* Algo, #926 */
	0xffffffff	/* terminate */
};

static const u32 route_3Mic_SPK_WB_FO_NSOff[] = {
	0x9031044d,	/* Route, #1101 */
	0x9031039e,	/* Algo, #926 */
	0x903103e7,	/* Algo, #999 */
	0xffffffff	/* terminate */
};

#if  defined(CONFIG_MACH_KACTIVELTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_ATT) || defined(CONFIG_SEC_S_PROJECT) || defined(CONFIG_MACH_KACTIVELTE_CAN) \
|| defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_HESTIALTE_EUR) || defined(CONFIG_MACH_KACTIVELTE_KOR)
static const u32 *es705_route_configs[] = {
	route_1Mic_WHS_NB_SWBypass,			/*  0 */
	route_2Mic_SPK_NB_FO_NSOn,			/*  1 */
	route_2Mic_HS_NB_NSOn,				/*  2*/
	route_2Mic_SPK_NB_FO_NSOff,			/*  3 */
	route_2Mic_HS_NB_NSOff,				/*  4 */
	route_1Ch_AudioPB,					/*  5 */
	route_2Ch_AudioPB,					/*  6 */
	route_2Mic_AudioZoomInterview,		/*  7 */
	route_2Mic_AudioZoomConversation,	/*  8 */
	route_dummy,						/*  9 */
	route_dummy,						/* 10 */
	route_TTYVCO,						/* 11 */
	route_TTYHCO,						/* 12 */
	route_2Mic_ASR_AO,					/* 13 */
	route_dummy,						/* 14 */
	route_dummy,						/* 15 */
	route_dummy,						/* 16 */
	route_1Mic_HS_LB_SWBypass,			/* 17 */
	route_1Mic_SPK_LB_SWBypass,			/* 18 */
	route_1Mic_WHS_LB_SWBypass,			/* 19 */
	route_dummy,						/* 20 */
	route_1Mic_WHS_WB_SWBypass,			/* 21 */
	route_2Mic_SPK_WB_FO_NSOn,			/* 22 */
	route_2Mic_HS_WB_NSOn,				/* 23 */
	route_2Mic_SPK_WB_FO_NSOff,			/* 24 */
	route_2Mic_HS_WB_NSOff,				/* 25 */
	route_voicesense_pdm,				/* 26 */
	route_sensory_pdm,				/* 27 */
};
#else
static const u32 *es705_route_configs[] = {
	route_1Mic_WHS_NB_SWBypass,			/*  0 */
	route_3Mic_SPK_NB_FO_NSOn,			/*  1 */
	route_2Mic_HS_NB_NSOn,				/*  2*/
	route_3Mic_SPK_NB_FO_NSOff,			/*  3 */
	route_2Mic_HS_NB_NSOff,				/*  4 */
	route_1Ch_AudioPB,					/*  5 */
	route_2Ch_AudioPB,					/*  6 */
	route_2Mic_AudioZoomInterview,		/*  7 */
	route_2Mic_AudioZoomConversation,		/*  8 */
	route_dummy,						/*  9 */
	route_dummy,						/* 10 */
	route_TTYVCO,						/* 11 */
	route_TTYHCO,						/* 12 */
	route_3Mic_ASR_AO,					/* 13 */
	route_dummy,						/* 14 */
	route_dummy,						/* 15 */
	route_dummy,						/* 16 */
	route_1Mic_HS_LB_SWBypass,			/* 17 */
	route_1Mic_SPK_LB_SWBypass,			/* 18 */
	route_1Mic_WHS_LB_SWBypass,			/* 19 */
	route_dummy,						/* 20 */
	route_1Mic_WHS_WB_SWBypass,			/* 21 */
	route_3Mic_SPK_WB_FO_NSOn,			/* 22 */
	route_2Mic_HS_WB_NSOn,				/* 23 */
	route_3Mic_SPK_WB_FO_NSOff,			/* 24 */
	route_2Mic_HS_WB_NSOff,				/* 25 */
	route_voicesense_pdm,				/* 26 */
	route_sensory_pdm,				/* 27 */
};
#endif
#endif
