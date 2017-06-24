/*
*
* isdbtmm driver
*
* Copyright (C) (2011, Samsung Electronics)
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

#ifndef _ISDBTMM_PDATA_H_
#define _ISDBTMM_PDATA_H_


struct isdbtmm_platform_data {
	int	irq;
	int gpio_en;
	int gpio_rst;
	int gpio_int;
	int gpio_spi_do;
	int gpio_spi_di;
	int gpio_spi_cs;
	int gpio_spi_clk;
	int gpio_i2c_sda;
	int gpio_i2c_scl;
#ifdef CONFIG_TMM_ANT_DET
	int gpio_ant_det;
	int irq_ant_det;
#endif
};

#endif
