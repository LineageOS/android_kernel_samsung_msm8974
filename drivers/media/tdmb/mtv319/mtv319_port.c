/*
*
* File name: mtv319_port.c
*
* Description : User-supplied Routines for RAONTECH TV Services.
*
* Copyright (C) (2013, RAONTECH)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include <linux/spi/spi.h>

#include "mtv319.h"
#include "mtv319_internal.h"
#include "tdmb.h"


/* Declares a variable of gurad object if neccessry. */
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)\
|| defined(RTV_FIC_I2C_INTR_ENABLED)
	#if defined(__KERNEL__)
	struct mutex raontv_guard;
	#elif defined(WINCE) || defined(WINDOWS) || defined(WIN32)
	CRITICAL_SECTION raontv_guard;
	#else
	/* non-OS and RTOS. */
	#endif
#endif

#if defined(RTV_IF_SPI)
static struct spi_device *mtv_spi;
void mtv319_set_port_if(unsigned long interface)
{
	mtv_spi = (struct spi_device *)interface;
}
unsigned char mtv319_spi_read(unsigned char page, unsigned char reg)
{
	int ret;
	u8 out_buf[4], in_buf[4];
	struct spi_message msg;
	struct spi_transfer msg_xfer = {
		.tx_buf = out_buf,
		.rx_buf = in_buf,
		.len = 4,
		.cs_change = 0,
		.delay_usecs = 0
	};

	out_buf[0] = 0x90 | page;
	out_buf[1] = reg;
	out_buf[2] = 1; /* Read size */

	spi_message_init(&msg);
	spi_message_add_tail(&msg_xfer, &msg);

	ret = spi_sync(mtv_spi, &msg);
	if (ret) {
		DPRINTK("error: %d\n", ret);
		return 0xFF;
	}

#if 0
	DPRINTK("0x%02X 0x%02X 0x%02X 0x%02X\n",
			in_buf[0], in_buf[1], in_buf[2], in_buf[3]);
#endif

	return in_buf[MTV319_SPI_CMD_SIZE];
}

void mtv319_spi_read_burst(unsigned char page, unsigned char reg,
			unsigned char *buf, int size)
{
	int ret;
	u8 out_buf[MTV319_SPI_CMD_SIZE];
	struct spi_message msg;
	struct spi_transfer xfer0 = {
		.tx_buf = out_buf,
		.rx_buf = buf,
		.len = MTV319_SPI_CMD_SIZE,
		.cs_change = 0,
		.delay_usecs = 0
	};

	struct spi_transfer xfer1 = {
		.tx_buf = buf,
		.rx_buf = buf,
		.len = size,
		.cs_change = 0,
		.delay_usecs = 0,
	};

	out_buf[0] = 0xA0; /* Memory read */
	out_buf[1] = 0x00;
	out_buf[2] = 188; /* Fix */

	spi_message_init(&msg);
	spi_message_add_tail(&xfer0, &msg);
	spi_message_add_tail(&xfer1, &msg);

	ret = spi_sync(mtv_spi, &msg);
	if (ret) {
		DPRINTK("error: %d\n", ret);
		return;
	}
}

void mtv319_spi_write(unsigned char page, unsigned char reg, unsigned char val)
{
	u8 out_buf[4];
	u8 in_buf[4];
	struct spi_message msg;
	struct spi_transfer msg_xfer = {
		.tx_buf = out_buf,
		.rx_buf = in_buf,
		.len = 4,
		.cs_change = 0,
		.delay_usecs = 0
	};
	int ret;

	out_buf[0] = 0x80 | page;
	out_buf[1] = reg;
	out_buf[2] = 1; /* size */
	out_buf[3] = val;

	spi_message_init(&msg);
	spi_message_add_tail(&msg_xfer, &msg);

	ret = spi_sync(mtv_spi, &msg);
	if (ret)
		DPRINTK("error: %d\n", ret);
}

void mtv319_spi_recover(unsigned char *buf, unsigned int intr_size)
{
	int ret;
	struct spi_message msg;
	struct spi_transfer msg_xfer = {
		.tx_buf = buf,
		.rx_buf = buf,
		.len = MTV319_SPI_CMD_SIZE + intr_size,
		.cs_change = 0,
		.delay_usecs = 0,
	};

	memset(buf, 0xFF, MTV319_SPI_CMD_SIZE+intr_size);

	spi_message_init(&msg);
	spi_message_add_tail(&msg_xfer, &msg);

	ret = spi_sync(mtv_spi, &msg);
	if (ret)
		DPRINTK("error: %d\n", ret);
}
#endif
