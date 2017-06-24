/*
 * escore-uart.h  --  Audience eS705 UART interface
 *
 * Copyright 2013 Audience, Inc.
 *
 * Author: Matt Lupfer <mlupfer@cardinalpeak.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ESCORE_UART_H
#define _ESCORE_UART_H

struct escore_priv;

struct escore_uart_device {
	struct tty_struct *tty;
	struct file *file;
	struct tty_ldisc *ld;
};

extern struct platform_driver escore_uart_driver;
extern struct platform_device escore_uart_device;

int escore_uart_bus_init(struct escore_priv *escore);

#endif
