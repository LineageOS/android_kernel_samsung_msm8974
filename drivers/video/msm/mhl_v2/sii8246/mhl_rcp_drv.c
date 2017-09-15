/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Author: Rajucm <rajkumar.m@samsung.com>
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

#include <linux/input.h>
#include <linux/types.h>

#define RCP_INPUTDEV_NAME "sii8246"

#define	MHL_DEV_LD_DISPLAY	(0x01 << 0)
#define	LOG_DEV_VIDEO		(0x01 << 1)
#define	LOG_DEV_AUDIO		(0x01 << 2)
#define	LOG_DEV_MEDIA		(0x01 << 3)
#define	LOG_DEV_TUNER		(0x01 << 4)
#define	LOG_DEV_RECORD		(0x01 << 5)
#define	LOG_DEV_SPEAKER	(0x01 << 6)
#define	LOG_DEV_GUI	(0x01 << 7)

#define LOG_DEV_NONE 0x00
#define LOG_DEV_ALL 0xFF

#define	MHL_MAX_RCP_KEY_CODE	(0x7F + 1)	/* inclusive */
/* @log_dev_type: type of logical device which should support/handle this
 * key;each MHL device will have its logical device type stored in one of the
 * capability registers */
struct rcp_keymap {
	u16 key_code;
	u8 log_dev_type;	/* Logical device type */
};

static const struct rcp_keymap mhl_rcp_keymap[] = {
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

#define MHL_RCP_NUM_KEYS ARRAY_SIZE(mhl_rcp_keymap)

static u16 keycode[MHL_RCP_NUM_KEYS];

bool is_key_supported(int keyindex)
{
	u8 log_dev = LOG_DEV_GUI;

	if (keyindex > MHL_RCP_NUM_KEYS) {
		pr_err("[ERROR] rcp: keyindex[%d] > MHL_RCP_NUM_KEYS[%d]\n",
				keyindex, MHL_RCP_NUM_KEYS);
		return false;
	}

	if (mhl_rcp_keymap[keyindex].key_code != KEY_UNKNOWN &&
		mhl_rcp_keymap[keyindex].key_code != KEY_RESERVED &&
		(mhl_rcp_keymap[keyindex].log_dev_type & log_dev))
		return true;
	 else
		return false;
}

struct input_dev *register_mhl_input_device(void)
{
	struct input_dev *input;
	int ret;
	u8 i;

	input = input_allocate_device();
	if (!input) {
		pr_err("rcp: failed to allocate input device\n");
		return NULL;
	}
	set_bit(EV_KEY, input->evbit);
	for (i = 0; i < MHL_RCP_NUM_KEYS; i++)
		keycode[i] = mhl_rcp_keymap[i].key_code;

	input->keycode = keycode;
	input->keycodemax = MHL_RCP_NUM_KEYS;
	input->keycodesize = sizeof(keycode[0]);
	for (i = 0; i < MHL_RCP_NUM_KEYS; i++) {
		if (is_key_supported(i))
			set_bit(keycode[i], input->keybit);
	}

	input->name = RCP_INPUTDEV_NAME;
	input->id.bustype = BUS_I2C;

	pr_info("RCP: Registering input device\n");
	ret = input_register_device(input);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] : failed to register input device\n");
		input_free_device(input);
		return NULL;
	}

	return input;
}

void rcp_key_report(struct input_dev *input_dev, u8 key)
{
	pr_info("sii8246: report rcp key: %d\n", key);
	if (input_dev) {
		input_report_key(input_dev, key, 1);
		input_report_key(input_dev, key, 0);
		input_sync(input_dev);
	}
}
