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


#if defined(CONFIG_TOUCHSCREEN_MMS136)
#define MELFAS_TS_NAME			"mms136_ts"
#else
#define MELFAS_TS_NAME			"mms144_ts"
#endif

#if defined(CONFIG_TOUCHSCREEN_MMS136)
struct mms_ts_platform_data {
	int	max_x;
	int	max_y;

	bool	invert_x;
	bool	invert_y;

	bool i2c_pull_up;
	int gpio_int;
	u32 irq_gpio_flags;
	u32 sda_gpio_flags;
	u32 scl_gpio_flags;
	int vdd_en;

	int	gpio_sda;
	int	gpio_scl;
	int	gpio_resetb;
	int	gpio_vdd_en;

	int	(*mux_fw_flash)(bool to_gpios);
	const char	*fw_name;
};
#else
struct mms_ts_platform_data {
	int	max_x;
	int	max_y;

	bool	invert_x;
	bool	invert_y;
	bool	flip_xy;

//	int	gpio_sda;
//	int	gpio_scl;

	bool i2c_pull_up;
	int gpio_int;
	u32 irq_gpio_flags;
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;
	int	gpio_resetb;
	int vdd_en;
	
//	int	gpio_resetb;
	int	gpio_lcd_type;
	int	(*mux_fw_flash)(bool to_gpios);
	void (*vdd_on)(bool);
	int (*is_vdd_on)(void);
//	void	(*register_cb)(struct tsp_callbacks *);
	const char	*fw_name;
	bool	use_touchkey;
	const u8	*touchkey_keycode;
	const u8	*config_fw_version;
	int	check_module_type;
};
extern struct class *sec_class;
extern int poweroff_charging;
extern unsigned char LCD_Get_Value(void);

//extern struct tsp_callbacks {
//	void (*inform_charger)(struct tsp_callbacks *tsp_cb, bool mode);
//};
extern struct tsp_callbacks *charger_callbacks;
#endif

#endif /* _LINUX_MMS_TOUCH_H */
