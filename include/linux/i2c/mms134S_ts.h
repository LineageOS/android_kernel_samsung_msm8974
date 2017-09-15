/*
 * include/linux/melfas_ts.h - platform data structure for MMS Series sensor
 *
 * Copyright (C) 2010 Melfas, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_MELFAS_TS_H
#define _LINUX_MELFAS_TS_H
#undef TA_DETECTION

#define MELFAS_TS_NAME "melfas-ts"

struct melfas_version {
	uint8_t tsp_revision;
	uint8_t hardware;
	uint8_t compatibility;
	uint8_t core;
	uint8_t private;
	uint8_t public;
	uint8_t product_code;
};

struct melfas_tsi_platform_data {
	int x_size;
	int y_size;
	struct melfas_version *version;
	int (*power)(int on);
	void (*gpio)(void);

	bool i2c_pull_up;
	int gpio_int;
	u32 irq_gpio_flags;
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;
	int	gpio_resetb;
	int vdd_en;

#ifdef TA_DETECTION
	void (*register_cb)(void*);
	void (*read_ta_status)(void*);
#endif
};

#endif /* _LINUX_MELFAS_TS_H */
