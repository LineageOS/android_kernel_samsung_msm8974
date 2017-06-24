/*
 * Copyright (C) 2013 Samsung Electronics
 * Jeongrae Kim <jryu.kim@samsung.com>
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

#ifndef _TSU6721_H_
#define _TSU6721_H_
#include <linux/i2c/muic.h>

enum {
	TSU6721_DETACHED,
	TSU6721_ATTACHED
};

extern struct switch_dev switch_dock;

struct tsu6721_platform_data {
	void	(*callback)(enum cable_type_t cable_type, int attached);
	void	(*oxp_callback)(int state);
	void	(*mhl_sel) (bool onoff);
	int	(*dock_init) (void);
	int	gpio_int;
	u32	irq_gpio_flags;
	int	gpio_sda;
	u32	sda_gpio_flags;
	int	gpio_scl;
	u32	scl_gpio_flags;
	int	gpio_uart_on;
	u32	uarton_gpio_flags;
};

extern int poweroff_charging;

extern int check_jig_state(void);
extern struct tsu6721_platform_data tsu6721_pdata;
extern void tsu6721_callback(enum cable_type_t cable_type, int attached);
extern void tsu6721_oxp_callback(int state);
extern int tsu6721_dock_init(void);

#if defined(CONFIG_VIDEO_MHL_V2)
extern int dock_det(void);
#endif

extern struct class *sec_class;

#endif /* _TSU6721_H_ */

