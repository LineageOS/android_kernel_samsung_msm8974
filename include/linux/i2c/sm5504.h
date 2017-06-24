/*
 * Copyright (C) 2013 Samsung Electronics
 * Nitin Chaudhary <nc.chaudhary@samsung.com>
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

#ifndef _SM5504_H_
#define _SM5504_H_
#include <linux/i2c/muic.h>

enum {
	SM5504_NONE = -1,
	SM5504_DETACHED = 0,
	SM5504_ATTACHED = 1
};

struct sm5504_platform_data {
	void (*callback)(enum cable_type_t cable_type, int attached);
#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
	void (*lanhub_cb)(enum cable_type_t cable_type, int attached, bool lanhub_ta);
#endif
	void (*oxp_callback)(int state);
	void (*mhl_sel) (bool onoff);
	int	(*dock_init) (void);
	int gpio_int;
	u32 irq_gpio_flags;
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;
	int gpio_uart_on;
	u32 uarton_gpio_flags;
};

/*SM5504 Callback functions in sec-switch.c*/
extern int check_sm5504_jig_state(void);
extern struct sm5504_platform_data sm5504_pdata;
extern void sm5504_callback(enum cable_type_t cable_type, int attached);
extern void sm5504_oxp_callback(int state);
extern int sm5504_dock_init(void);
#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
extern void sm5504_lanhub_callback(enum cable_type_t cable_type, int attached, bool lanhub_ta);
#endif
#endif /* _SM5504_H_ */

