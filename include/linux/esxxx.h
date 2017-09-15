/*
 * esxxx.h - header for esxxx I2C interface
 *
 * Copyright (C) 2011-2012 Audience, Inc.
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

#ifndef __ESXXX_H__
#define __ESXXX_H__

#include <linux/types.h>

#if defined(CONFIG_SND_SOC_ES705)
struct esxxx_accdet_config {
	int	btn_serial_cfg;
	int	btn_parallel_cfg;
	int	btn_detection_rate;
	int	btn_press_settling_time;
	int	btn_bounce_time;
	int	btn_long_press_time;
};

struct esxxx_platform_data {
	int	irq_base, irq_end;
	int	reset_gpio;
	int	wakeup_gpio;
	int	uart_gpio;	/* uart switch */
	int	uart_tx_gpio;
	int	uart_rx_gpio;
	int	gpioa_gpio;
	int	gpiob_gpio;
	int	accdet_gpio;
	int	int_gpio;
	struct esxxx_accdet_config accdet_cfg;
	int (*esxxx_clk_cb) (int);
};
#else
struct esxxx_platform_data {
	unsigned int	irq_base, irq_end;
	unsigned int	reset_gpio;
	unsigned int	wakeup_gpio;
	unsigned int	gpioa_gpio;
	unsigned int	gpiob_gpio;
	unsigned int	accdet_gpio;
	unsigned int	int_gpio;
	struct slim_device intf_device;	

};
#endif
#endif /* __ESXXX_H__ */
