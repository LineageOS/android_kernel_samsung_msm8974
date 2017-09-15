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

#ifndef _IRDA_ICE40_H_
#define _IRDA_ICE40_H_

struct irda_ice40_platform_data {
	int fw_ver;
	int spi_clk;
	int spi_si;
	int cresetb;
	int rst_n;
	int irda_irq;
#ifdef CONFIG_MACH_KLTE_VZW
	int tunable_support;
	int tunable_crstb;
#endif
};

#define IR_DRIVER_NAME		"ice4_dev"
#define IR_IOCTL_BASE		'I'
#define IR_IOCTL_SET_FREQ	_IOW(IR_IOCTL_BASE, 1, int)
#define IR_IOCTL_SET_SIZE	_IOW(IR_IOCTL_BASE, 2, int)
#define IR_IOCTL_SET_DATA	_IOW(IR_IOCTL_BASE, 3, int*)
#define IR_IOCTL_START		_IO(IR_IOCTL_BASE, 4)
#define IR_IOCTL_STOP		_IO(IR_IOCTL_BASE, 5)
#ifdef IRDA_RX_ENABLE
#define IR_IOCTL_GET_LEARN	_IOW(IR_IOCTL_BASE, 6, char*)
#define IR_IOCTL_OPERATION	_IOW(IR_IOCTL_BASE, 7, int)
#endif

#define SEC_FPGA_MAX_FW_PATH    255
#define SEC_FPGA_FW_FILENAME    "i2c_top_bitmap.bin"

#define SNPRINT_BUF_SIZE	255
#define FW_VER_ADDR		0x02
#define FIRMWARE_MAX_RETRY	2
#define GPIO_FPGA_MAIN_CLK	58
#define GPIO_FPGA_MAIN_CLK_CTC_REV02	18

#define IRDA_I2C_ADDR		0x50
#define IRDA_I2C_RX_ADDR	0x6C
#define IRDA_TEST_CODE_SIZE	144
#define IRDA_TEST_CODE_ADDR	0x00
#define MAX_SIZE		4096
#define READ_LENGTH		8

#define POWER_ON		1
#define POWER_OFF		0
#define SEND_SUCCESS		0
#define SEND_FAIL		-1

enum irda_tx_register_map {
	IRDA_REG_LENGTH_MSB,
	IRDA_REG_LENGTH_LSB,
	IRDA_REG_OPERATION,
	IRDA_REG_FREQ_MSB,
	IRDA_REG_FREQ_LSB,
	IRDA_REG_DATA,
};

enum irda_tx_operation_type {
	IRDA_SINGLE,
	IRDA_REPEAT,
	IRDA_STOP,
	IRDA_LEARN,
};

extern struct class *sec_class;

#define GPIO_LEVEL_LOW        0
#define GPIO_LEVEL_HIGH       1

#endif /* _IRDA_ICE40_H_ */
