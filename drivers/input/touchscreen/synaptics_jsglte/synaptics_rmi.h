/*
 * Copyright (c) 2011, 2012 Synaptics Incorporated
 * Copyright (c) 2011 Unixphere
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifndef _SYNAPTICS_RMI4_GENERIC_H_
#define _SYNAPTICS_RMI4_GENERIC_H_


#define SYNAPTICS_HW_RESET_TIME_B0	100

#define OCTA_PANEL_REVISION_51	0x08
#define OCTA_PANEL_REVISION_43	0x02
#define OCTA_PANEL_REVISION_40	0x01
#define OCTA_PANEL_REVISION_34	0x00

struct synaptics_rmi_f1a_button_map {
	unsigned char nbuttons;
	u32 map[4];
};

#undef SYNAPTICS_RMI_INFORM_CHARGER 
//#define SYNAPTICS_RMI_INFORM_CHARGER 

#ifdef SYNAPTICS_RMI_INFORM_CHARGER
struct synaptics_rmi_callbacks {
	void (*inform_charger)(struct synaptics_rmi_callbacks *, int);
};
#endif


struct synaptics_rmi4_power_data {
	int vdd_io_1p8;
	int tsp_int;
};


/**
 * struct synaptics_rmi4_platform_data - rmi4 platform data
 * @x_flip: x flip flag
 * @y_flip: y flip flag
 * @regulator_en: regulator enable flag
 * @gpio: attention interrupt gpio
 * @irq_type: irq type
 * @gpio_config: pointer to gpio configuration function
 * @f1a_button_map: pointer to 0d button map
 */
struct synaptics_rmi4_platform_data {
	bool x_flip;
	bool y_flip;
	unsigned int sensor_max_x;
	unsigned int sensor_max_y;
	unsigned char max_touch_width;
	unsigned char panel_revision;	/* to identify panel info */
	bool regulator_en;
	unsigned gpio;
	int irq_type;
	int (*gpio_config)(unsigned interrupt_gpio, bool configure);
	int tsppwr_1p8_en;
	void (*enable_sync)(bool on);
	const char *firmware_name;
	const char *fac_firmware_name;
	int num_of_rx;
	int num_of_tx;

	int vdd_io_1p8;
	int tsp_int;
	int tkey_led_vdd_on;

/* use H project, S5050 driver */
	bool swap_axes;
	int reset_gpio;
	unsigned long irq_flags;
	unsigned int panel_x;
	unsigned int panel_y;
	unsigned int reset_delay_ms;
	unsigned char model_name[32];
	const char *project_name;
#ifdef SYNAPTICS_RMI_INFORM_CHARGER	
	void (*register_cb)(struct synaptics_rmi_callbacks *);
#endif
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_PREVENT_HSYNC_LEAKAGE)
	void (*hsync_onoff)(bool onoff);
#endif
	struct synaptics_rmi_f1a_button_map *f1a_button_map;
};

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_PREVENT_HSYNC_LEAKAGE)
extern void mdss_dsi_panel_hsync_onoff(bool onoff);
#endif

#endif
