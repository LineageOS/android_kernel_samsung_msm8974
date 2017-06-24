/*
*
* File name: raontv_port.c
*
* Description : User-supplied Routines for RAONTECH TV Services.
*
* Copyright (C) (2014, RAONTECH)
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
#include "isdbt.h"

#include "isdbt_port_mtv222.h"

/* Declares a variable of gurad object if neccessry. */
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#if defined(__KERNEL__)	
	struct mutex raontv_guard;
	#elif defined(WINCE)
        CRITICAL_SECTION raontv_guard;
	#else
	/* non-OS and RTOS. */
	#endif
#endif


void rtvOEM_ConfigureInterrupt(void) 
{
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_MAP_SEL(SPI_CTRL_PAGE);
	RTV_REG_SET(0x21, SPI_INTR_POL_ACTIVE|0x02);

#ifdef RTV_IF_SPI
	RTV_REG_SET(0x27, 0x01); /* AUTO_INTR: 0, Recovery mode ON */
#endif

	RTV_REG_SET(0x2B, RTV_SPI_INTR_DEACT_PRD_VAL);

	RTV_REG_SET(0x2A, 1); /* SRAM init */
	RTV_REG_SET(0x2A, 0);
#endif
}

#if defined(RTV_IF_SPI)
#define SPI_CMD_SIZE	MTV222_SPI_CMD_SIZE

static struct spi_device *mtv_spi;
void mtv222_set_port_if(unsigned long interface)
{
	mtv_spi = (struct spi_device *)interface;
}

unsigned char mtv222_spi_read(unsigned char page, unsigned char reg)
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
		DMBERR("error: %d\n", ret);
		return 0xFF;
	}

#if 0
	DMBMSG("0x%02X 0x%02X 0x%02X 0x%02X\n",
			in_buf[0], in_buf[1], in_buf[2], in_buf[3]);
#endif

	return in_buf[SPI_CMD_SIZE];
}

void mtv222_spi_read_burst(unsigned char page, unsigned char reg,
			unsigned char *buf, int size)
{
	int ret;
	u8 out_buf[SPI_CMD_SIZE];
	struct spi_message msg;
	struct spi_transfer msg_xfer0 = {
		.tx_buf = out_buf,
		.rx_buf = buf,
		.len = SPI_CMD_SIZE,
		.cs_change = 0,
		.delay_usecs = 0
	};

	struct spi_transfer msg_xfer1 = {
		.tx_buf = buf,
		.rx_buf = buf,
		.len = size,
		.cs_change = 0,
		.delay_usecs = 0
	};

	if (page > 15) { /* 0 ~ 15: not SPI memory */
		out_buf[0] = 0xA0; /* Memory read */
		out_buf[1] = 0x00;
		out_buf[2] = 188; /* Fix */
	} else {
		out_buf[0] = 0x90 | page; /* Register read */
		out_buf[1] = reg;
		out_buf[2] = size;
	}

	spi_message_init(&msg);
	spi_message_add_tail(&msg_xfer0, &msg);
	spi_message_add_tail(&msg_xfer1, &msg);

	ret = spi_sync(mtv_spi, &msg);
	if (ret)
		DMBERR("1 error: %d\n", ret);	
}

void mtv222_spi_write(unsigned char page, unsigned char reg, unsigned char val)
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
		DMBERR("error: %d\n", ret);
}

void mtv222_spi_recover(unsigned char *buf, unsigned int size)
{
	int ret;
	struct spi_message msg;
	struct spi_transfer msg_xfer = {
		.tx_buf = buf,
		.rx_buf = buf,
		.len = size,
		.cs_change = 0,
		.delay_usecs = 0,
	};

	memset(buf, 0xFF, size);

	spi_message_init(&msg);
	spi_message_add_tail(&msg_xfer, &msg);

	ret = spi_sync(mtv_spi, &msg);
	if (ret)
		DMBERR("error: %d\n", ret);
}
#endif /* #if defined(RTV_IF_SPI) */



