/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Author: Dharam Kumar <dharam.kr@samsung.com>
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
 #ifndef __SII8240_RCP_H__
#define __SII8240_RCP_H__

#include <linux/input.h>

/* RCPE sub-commands */
enum rcpe_key_status_code {
	/* Dont use it as the key will be ACKed using RCPK command */
	RCPE_KEY_NO_ERROR =		0x00,
	/* Key-code was either invalid,not-supoorted or not-recognized by
	 * receiving device */
	RCPE_KEY_INVALID =		0x01,
	/* Receiving device busy,received key-code discarded,sender may try
	 * again */
	RCPE_KEY_RESPONDER_BUSY =	0x02,
};

/* RAP sub-commands */
enum rap_action_code {
	RAP_POLL =		0x00,
	RAP_CONTENT_ON =	0x10,
	RAP_CONTENT_OFF =	0x11,
};

enum rapk_status_code {
	RAPK_NO_ERROR =		0x00,
	RAPK_UNRECOGNIZED =	0x01,
	RAPK_UNSUPPORTED =	0x02,
	RAPK_RESPONDER_BUSY =	0x03,
};

#define LOG_DEV_DISPLAY			(1<<0)
#define LOG_DEV_VIDEO			(1<<1)
#define LOG_DEV_AUDIO			(1<<2)
#define LOG_DEV_MEDIA			(1<<3)
#define LOG_DEV_TUNER			(1<<4)
#define LOG_DEV_RECORD			(1<<5)
#define LOG_DEV_SPEAKER			(1<<6)
#define LOG_DEV_GUI			(1<<7)
#define LOG_DEV_NONE			0
#define LOG_DEV_ALL			0xFF
/* @log_dev_type: type of logical device which should support/handle this
 * key;each MHL device will have its logical device type stored in one of the
 * capability registers */
struct rcp_keymap {
	u16 key_code;
	u8 log_dev_type;	/* Logical device type */
};

static const struct rcp_keymap sii8240_rcp_keymap[] = {
	{ KEY_ENTER,		LOG_DEV_GUI },
	{ KEY_UP,		LOG_DEV_GUI },
	{ KEY_DOWN,		LOG_DEV_GUI },
	{ KEY_LEFT,		LOG_DEV_GUI },
	{ KEY_RIGHT,		LOG_DEV_GUI },
	/* TODO:Could not find Keys like RIGHTUP, RIGHTDOWN, LEFTUP,LEFTDOWN
	 * in <linux/input.h>,should not be these keys added to the input header
	 * file ? */
	{ KEY_UNKNOWN,		LOG_DEV_NONE },		/* right-up */
	{ KEY_UNKNOWN,		LOG_DEV_NONE },		/* right-down */
	{ KEY_UNKNOWN,		LOG_DEV_NONE },		/* left-up */
	{ KEY_UNKNOWN,		LOG_DEV_NONE },		/* left-down */
	{ KEY_MENU,		LOG_DEV_GUI },
	{ KEY_SETUP,		LOG_DEV_NONE },		/* setup */
	{ KEY_UNKNOWN,		LOG_DEV_NONE },		/* contents */
	{ KEY_FAVORITES,	LOG_DEV_NONE },
	{ KEY_BACK,		LOG_DEV_GUI },

	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x0e */
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x1F */

	{ KEY_NUMERIC_0,	LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_TUNER | LOG_DEV_ALL},

	{ KEY_NUMERIC_1,	LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_TUNER | LOG_DEV_ALL},

	{ KEY_NUMERIC_2,	LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_TUNER | LOG_DEV_ALL},

	{ KEY_NUMERIC_3,	LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_TUNER | LOG_DEV_ALL},

	{ KEY_NUMERIC_4,	LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_TUNER | LOG_DEV_ALL},

	{ KEY_NUMERIC_5,	LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_TUNER | LOG_DEV_ALL},

	{ KEY_NUMERIC_6,	LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_TUNER | LOG_DEV_ALL},

	{ KEY_NUMERIC_7,	LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_TUNER | LOG_DEV_ALL},

	{ KEY_NUMERIC_8,	LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_TUNER | LOG_DEV_ALL},

	{ KEY_NUMERIC_9,	LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_TUNER | LOG_DEV_ALL},

	{ KEY_DOT,		LOG_DEV_NONE },

	{ KEY_ENTER,		LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_TUNER | LOG_DEV_ALL},

	{ KEY_CLEAR,		LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_TUNER | LOG_DEV_ALL},

	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x2D */
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x2F */

	{ KEY_CHANNELUP,	LOG_DEV_TUNER },	/* channel up */
	{ KEY_CHANNELDOWN,	LOG_DEV_TUNER },	/* channel down */
	/* TODO:Not sure, what is the correct key code for 'previous channel' */
	{ KEY_PREVIOUS,		LOG_DEV_TUNER },	/* previous channel */
	{ KEY_UNKNOWN,		LOG_DEV_AUDIO },	/* sound select */
	{ KEY_UNKNOWN,		LOG_DEV_NONE },		/* input select */
	{ KEY_INFO,		LOG_DEV_NONE },		/* show information */
	{ KEY_HELP,		LOG_DEV_NONE },		/* help */
	{ KEY_PAGEUP,		LOG_DEV_NONE },		/* page up */
	{ KEY_PAGEDOWN,		LOG_DEV_NONE },		/* page down */

	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x39 */
	{ KEY_RESERVED,		LOG_DEV_NONE},
	{ KEY_RESERVED,		LOG_DEV_NONE},
	{ KEY_RESERVED,		LOG_DEV_NONE},
	{ KEY_RESERVED,		LOG_DEV_NONE},
	{ KEY_RESERVED,		LOG_DEV_NONE},
	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x3F */
	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x40 */

	{ KEY_VOLUMEUP,		LOG_DEV_RECORD },	/* volume up */
	{ KEY_VOLUMEDOWN,	LOG_DEV_RECORD },	/* volume down */
	{ KEY_MUTE,		LOG_DEV_RECORD },	/* mute */

	{ KEY_PLAY,		LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_ALL},

	{ KEY_STOP,		LOG_DEV_VIDEO | LOG_DEV_AUDIO |
				LOG_DEV_RECORD | LOG_DEV_ALL},

	{ KEY_PAUSECD,		LOG_DEV_VIDEO | LOG_DEV_AUDIO |
				LOG_DEV_RECORD | LOG_DEV_ALL},

	{ KEY_RECORD,		LOG_DEV_RECORD },	/* record */
	{ KEY_REWIND,		LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_ALL},
	{ KEY_FASTFORWARD,	LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_ALL},
	{ KEY_EJECTCD,		LOG_DEV_MEDIA },	/* eject */
	{ KEY_NEXTSONG,		LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_ALL},
	{ KEY_PREVIOUSSONG,	LOG_DEV_VIDEO | LOG_DEV_AUDIO | LOG_DEV_MEDIA |
				LOG_DEV_ALL},

	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x4D */
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x4F */

	{ KEY_ANGLE,		LOG_DEV_NONE },		/* angle */
	/* TODO: there is no key in <linux/input.h>,corresponding to SUBPICTURE
	 * should we add such keys to <linux/input.h> ?? */
	{ KEY_UNKNOWN,	LOG_DEV_NONE },		/* subpicture */

	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x52 */
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x5F */

	{ KEY_PLAY,		LOG_DEV_VIDEO | LOG_DEV_AUDIO },
	{ KEY_PAUSE,		LOG_DEV_VIDEO | LOG_DEV_AUDIO },
	{ KEY_RECORD,		LOG_DEV_RECORD },	/* record_function */
	{ KEY_UNKNOWN,		LOG_DEV_RECORD },/* pause_record_function */
	{ KEY_STOP,		LOG_DEV_VIDEO | LOG_DEV_AUDIO |
				LOG_DEV_RECORD },

	{ KEY_UNKNOWN,		LOG_DEV_SPEAKER },	/* mute_function */
	{ KEY_UNKNOWN,		LOG_DEV_SPEAKER },/* restore_volume_function */
	{ KEY_UNKNOWN,		LOG_DEV_NONE },		/* tune_function */
	{ KEY_UNKNOWN,		LOG_DEV_NONE },/* select_media_function */

	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x69 */
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x70 */

	{ KEY_F1,		LOG_DEV_NONE },		/* F1 */
	{ KEY_F2,		LOG_DEV_NONE },		/* F2 */
	{ KEY_F3,		LOG_DEV_NONE },		/* F3 */
	{ KEY_F4,		LOG_DEV_NONE },		/* F4 */
	{ KEY_F5,		LOG_DEV_NONE },		/* F5 */

	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x76 */
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },
	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x7D */

	{ KEY_VENDOR,		LOG_DEV_NONE },

	{ KEY_RESERVED,		LOG_DEV_NONE },		/* 0x7F */
};
#define SII8240_RCP_NUM_KEYS ARRAY_SIZE(sii8240_rcp_keymap)

#endif
