/*
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#ifndef __ASM_ARCH_SEC_HEADSET_H
#define __ASM_ARCH_SEC_HEADSET_H

#ifdef __KERNEL__

enum {
	SEC_JACK_NO_DEVICE				= 0x0,
	SEC_HEADSET_4POLE				= 0x01 << 0,
	SEC_HEADSET_3POLE				= 0x01 << 1,
#if defined(CONFIG_MACH_KLTE_JPN)
	SEC_EXTERNAL_ANTENNA			= 0x01 << 2,
#endif
};

struct sec_jack_zone {
	unsigned int adc_high;
	unsigned int delay_us;
	unsigned int check_count;
	unsigned int jack_type;
};

struct sec_jack_buttons_zone {
	unsigned int code;
	unsigned int adc_low;
	unsigned int adc_high;
};

struct sec_jack_platform_data {
	int	det_gpio;
	int	send_end_gpio;
	int	ear_micbias_gpio;
	const char *ear_micbias_ldo;
	int	fsa_en_gpio;
	bool	det_active_high;
	bool	send_end_active_high;
	struct qpnp_vadc_chip		*vadc_dev;
	struct sec_jack_zone jack_zones[4];
#if defined(CONFIG_SAMSUNG_JACK_VOICE_BTN)
	struct sec_jack_buttons_zone jack_buttons_zones[4];
#else
	struct sec_jack_buttons_zone jack_buttons_zones[3];
#endif
	int mpp_ch_scale[3];
};

#endif

#endif
