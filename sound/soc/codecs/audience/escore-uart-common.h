/*
 * escore-uart-common.h  --  UART interface for Audience earSmart chips
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

#ifndef _ESCORE_UART_COMMON_H
#define _ESCORE_UART_COMMON_H

#define UART_TTY_DEVICE_NODE		"/dev/ttyHS1"
#define UART_TTY_BAUD_RATE_BOOTLOADER	460800
#define UART_TTY_BAUD_RATE_FIRMWARE	3000000
#define UART_TTY_STOP_BITS		2
#define UART_TTY_WRITE_SZ		512

#define ESCORE_SBL_SYNC_CMD		0x00
#define ESCORE_SBL_SYNC_ACK		ESCORE_SBL_SYNC_CMD
#define ESCORE_SBL_BOOT_CMD		0x01
#define ESCORE_SBL_BOOT_ACK		ESCORE_SBL_BOOT_CMD
#define ESCORE_SBL_FW_ACK		0x02

int escore_uart_read(struct escore_priv *escore, void *buf, int len);
int escore_uart_write(struct escore_priv *escore, const void *buf, int len);
int escore_uart_cmd(struct escore_priv *escore, u32 cmd, int sr, u32 *resp);
int escore_configure_tty(struct tty_struct *tty, u32 bps, int stop);
int escore_uart_open(struct escore_priv *escore);
int escore_uart_close(struct escore_priv *escore);
int escore_uart_wait(struct escore_priv *escore);

extern struct es_stream_device es_uart_streamdev;

#endif
