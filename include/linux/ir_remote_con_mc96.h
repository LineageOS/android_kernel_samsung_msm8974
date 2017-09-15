/*
 * Copyright (C) 2012 Samsung Electronics
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
 */

#ifndef _IR_REMOTE_CON_H_
#define _IR_REMOTE_CON_H_

struct mc96_platform_data {
	int     str1;
	int     str2;
	char    char_name;
	int irda_irq_gpio;
	u32 irq_gpio_flags;
	int irda_led_en;
	u32 led_en_flags;
	int irda_wake_en;
	u32 wake_en_flags;
	int irda_scl_gpio;
	u32 irda_scl_flags;
	int irda_sda_gpio;
	u32 irda_sda_flags;
	int irda_poweron;
	u32 poweron_flags;
	void (*ir_wake_en)(struct mc96_platform_data *pdata, bool onoff);
	void (*ir_remote_init) (void);
	void(*ir_vdd_onoff)(struct device *dev,bool onoff);
};

extern struct class *sec_class;

#endif /* _IR_REMOTE_CON_H_ */
