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

#ifndef _BARCODE_EMUL_H_
#define _BARCODE_EMUL_H_

#define FPGA_GPIO_BASE	300
#define NR_FPGA_GPIO	16
#define ICE_FPGA_GPIO_TO_SYS(fpga_gpio)  (fpga_gpio + FPGA_GPIO_BASE)

enum { ICE_12M = 0, ICE_19M, ICE_GPIOX, ICE_I2C, ICE_24M, ICE_I2C_2, ICE_I2C_R2, ICE_I2C_R3 };

extern int ice_gpiox_set(int num, int val);
extern int ice_gpiox_get(int num);

struct barcode_emul_platform_data {
	int spi_clk;
	u32 spi_clk_flag;
	int spi_si;
	u32 spi_si_flag;
	int spi_en;	
	int cresetb;
	u32 cresetb_flag;
	int rst_n;
	u32 rst_n_flag;
	int fw_type;
	int cdone;
	u32 cdone_flag;	
#if defined CONFIG_IR_REMOCON_FPGA
	int irda_wake;
	u32 irda_wake_flag;
	int irda_irq;
	u32 irda_irq_flag;
	int ir_led_en;
	u32 ir_led_en_flag;
#endif
};

#define GPIO_LEVEL_LOW        0
#define GPIO_LEVEL_HIGH       1

#define BLANK_GPIOS           2

#define BEAMING_OFF           0
#define BEAMING_ON            1

#define STOP_BEAMING       0x00

#endif /* _BARCODE_EMUL_H_ */
