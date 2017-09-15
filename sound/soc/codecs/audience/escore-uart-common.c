/*
 * escore-uart-common.c  --  Audience eS705 UART interface
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

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/kthread.h>
#include <linux/esxxx.h>
#include <linux/serial_core.h>
#include <linux/tty.h>

#include "escore.h"
#include "escore-uart-common.h"

int escore_uart_read(struct escore_priv *escore, void *buf, int len)
{
	int rc;
	mm_segment_t oldfs;

	dev_dbg(escore->dev, "%s() size %d\n", __func__, len);

	/*
	 * we may call from user context via char dev, so allow
	 * read buffer in kernel address space
	 */
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	rc = escore->uart_dev.ld->ops->read(escore->uart_dev.tty,
		escore->uart_dev.file, (char __user *)buf, len);

	/* restore old fs context */
	set_fs(oldfs);

	dev_dbg(escore->dev, "%s() returning %d\n", __func__, rc);

	return rc;
}

int escore_uart_write(struct escore_priv *escore, const void *buf, int len)
{
	int rc = 0;
	int count_remain = len;
	int bytes_wr = 0;
	mm_segment_t oldfs;

	dev_dbg(escore->dev, "%s() size %d\n", __func__, len);

	/*
	 * we may call from user context via char dev, so allow
	 * read buffer in kernel address space
	 */
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	while (count_remain > 0) {
		/* block until tx buffer space is available */
		while (tty_write_room(escore->uart_dev.tty) < UART_TTY_WRITE_SZ)
			usleep_range(5000, 5000);

		rc = escore->uart_dev.ld->ops->write(escore->uart_dev.tty,
			escore->uart_dev.file, buf + bytes_wr,
				min(UART_TTY_WRITE_SZ, count_remain));

		if (rc < 0) {
			bytes_wr = rc;
			goto err_out;
		}

		bytes_wr += rc;
		count_remain -= rc;
	}

err_out:
	/* restore old fs context */
	set_fs(oldfs);

	dev_dbg(escore->dev, "%s() returning %d\n", __func__, rc);

	return bytes_wr;
}

int escore_uart_cmd(struct escore_priv *escore, u32 cmd, int sr, u32 *resp)
{
	int err;
	u32 rv;

	pr_debug("escore: cmd=0x%08x  sr=%i\n", cmd, sr);

	cmd = cpu_to_be32(cmd);
	err = escore_uart_write(escore, &cmd, sizeof(cmd));
	if (err < 0 || sr)
		return min(err, 0);
	else if (err > 0)
		err = 0;

	/* Maximum response time is 10ms */
	usleep_range(10000, 10500);

	err = escore_uart_read(escore, &rv, sizeof(rv));
	if (err > 0)
		*resp = be32_to_cpu(rv);
	err = err > 0 ? 0 : err;

	return err;
}

int escore_configure_tty(struct tty_struct *tty, u32 bps, int stop)
{
	int rc = 0;

	struct ktermios termios;
	termios = *tty->termios;

	dev_dbg(escore_priv.dev, "%s(): Requesting baud %u\n", __func__, bps);

	termios.c_cflag &= ~(CBAUD | CSIZE | PARENB);   /* clear csize, baud */
	termios.c_cflag |= BOTHER;	      /* allow arbitrary baud */
	termios.c_cflag |= CS8;

	if (stop == 2)
		termios.c_cflag |= CSTOPB;

	/* set uart port to raw mode (see termios man page for flags) */
	termios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
		| INLCR | IGNCR | ICRNL | IXON);
	termios.c_oflag &= ~(OPOST);
	termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

	/* set baud rate */
	termios.c_ospeed = bps;
	termios.c_ispeed = bps;

	rc = tty_set_termios(tty, &termios);

	dev_dbg(escore_priv.dev,
		"%s(): New baud %u\n", __func__, tty->termios->c_ospeed);

	return rc;
}

int escore_uart_open(struct escore_priv *escore)
{
	long err = 0;
	struct file *fp = NULL;
	unsigned long timeout = jiffies + msecs_to_jiffies(60000);
	int attempt = 0;

	/* try to probe tty node every 1 second for 60 seconds */
	do {
		ssleep(1);
		dev_dbg(escore->dev,
			"%s(): probing for tty on %s (attempt %d)\n",
			 __func__, UART_TTY_DEVICE_NODE, ++attempt);

		fp = filp_open(UART_TTY_DEVICE_NODE,
			       O_RDWR | O_NONBLOCK | O_NOCTTY,
			       0);
		err = PTR_ERR(fp);
	} while (time_before(jiffies, timeout) && err == -ENOENT);

	if (IS_ERR_OR_NULL(fp)) {
		dev_err(escore->dev,
			"%s(): UART device node open failed\n", __func__);
		return -ENODEV;
	}

	/* device node found */
	dev_dbg(escore->dev, "%s(): successfully opened tty\n",
		  __func__);

	/* set uart_dev members */
	escore_priv.uart_dev.file = fp;
	escore_priv.uart_dev.tty =
		((struct tty_file_private *)fp->private_data)->tty;
	escore_priv.uart_dev.ld = tty_ldisc_ref(
		escore_priv.uart_dev.tty);

	/* set baudrate to FW baud (common case) */
	escore_configure_tty(escore_priv.uart_dev.tty,
		UART_TTY_BAUD_RATE_FIRMWARE, UART_TTY_STOP_BITS);

	return 0;
}

int escore_uart_close(struct escore_priv *escore)
{
	tty_ldisc_deref(escore->uart_dev.ld);
	filp_close(escore->uart_dev.file, 0);

	return 0;
}

int escore_uart_wait(struct escore_priv *escore)
{
	/* wait on tty read queue until awoken or for 50ms */
	wait_event_interruptible_timeout(
		escore->uart_dev.tty->read_wait,
		escore->uart_dev.tty->read_cnt,
		msecs_to_jiffies(50));

	return 0;
}

struct es_stream_device es_uart_streamdev = {
	.open = escore_uart_open,
	.read = escore_uart_read,
	.close = escore_uart_close,
	.wait = escore_uart_wait,
	.intf = ES_UART_INTF,
};

MODULE_DESCRIPTION("ASoC ESCORE driver");
MODULE_AUTHOR("Greg Clemson <gclemson@audience.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:escore-codec");
