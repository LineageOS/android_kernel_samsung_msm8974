/* include/linux/wlan_plat.h
 *
 * Copyright (C) 2010 Google, Inc.
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
#ifndef _LINUX_WLAN_PLAT_H_
#define _LINUX_WLAN_PLAT_H_

struct wifi_platform_data {
#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE)
	int (*set_power)(int val,bool b0rev);
#else
	int (*set_power)(int val);
#endif
	int (*set_reset)(int val);
	int (*set_carddetect)(int val);
	void *(*mem_prealloc)(int section, unsigned long size);
	int (*get_mac_addr)(unsigned char *buf);
	void *(*get_country_code)(char *ccode);
};

#endif
