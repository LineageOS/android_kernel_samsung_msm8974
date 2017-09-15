/*
 * mms_ts.h - Platform data for Melfas MMS-series touch driver
 *
 * Copyright (C) 2011 Google Inc.
 * Author: Dima Zavin <dima@android.com>
 *
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef _LINUX_MMS_TOUCH_H
#define _LINUX_MMS_TOUCH_H
#define MELFAS_TS_NAME			"mms300-ts"
#undef TOUCHKEY	

struct melfas_tsi_platform_data {
	int max_x;
	int max_y;

	bool invert_x;
	bool invert_y;
	bool i2c_pull_up;
	int gpio_int;
	u32 irq_gpio_flags;
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;
	int vdd_en;
	int vdd_en2;
	int tsp_vendor1;
	int tsp_vendor2;
	int tkey_led_en;
	int key1;

	/*int (*mux_fw_flash) (bool to_gpios);
	int (*is_vdd_on) (void);
	int (*power) (bool on);*/
	const u8 *config_fw_version;
	const char *fw_name;
	bool use_touchkey;
	const u8 *touchkey_keycode;
	void (*input_event) (void *data);
	void (*register_cb) (void *);
	u8 panel;
};
extern struct class *sec_class;
void tsp_charger_infom(bool en);
#endif /* _LINUX_MMS_TOUCH_H */
