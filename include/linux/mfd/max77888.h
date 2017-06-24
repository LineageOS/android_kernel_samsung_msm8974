/*
 * max77888.h - Driver for the Maxim 77803
 *
 *  Copyright (C) 2011 Samsung Electrnoics
 *  SangYoung Son <hello.son@samsung.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This driver is based on max8997.h
 *
 * MAX77888 has Charger, Flash LED, Haptic, MUIC devices.
 * The devices share the same I2C bus and included in
 * this mfd driver.
 */

#ifndef __LINUX_MFD_MAX77888_H
#define __LINUX_MFD_MAX77888_H

#include <linux/regulator/consumer.h>
#include <linux/battery/sec_charger.h>

enum {
	MAX77888_MUIC_DETACHED = 0,
	MAX77888_MUIC_ATTACHED
};

enum {
	MAX77888_MUIC_DOCK_DETACHED = 0,
	MAX77888_MUIC_DOCK_DESKDOCK,
	MAX77888_MUIC_DOCK_CARDOCK,
	MAX77888_MUIC_DOCK_AUDIODOCK = 7,
	MAX77888_MUIC_DOCK_SMARTDOCK = 8
};

/* MAX77686 regulator IDs */
enum max77888_regulators {
	MAX77888_ESAFEOUT1 = 0,
	MAX77888_ESAFEOUT2,

	MAX77888_CHARGER,

	MAX77888_REG_MAX,
};

struct max77888_charger_reg_data {
	u8 addr;
	u8 data;
};

#if defined(CONFIG_CHARGER_MAX77888)
struct max77888_charger_platform_data {
	struct max77888_charger_reg_data *init_data;
	int num_init_data;
	sec_battery_platform_data_t *sec_battery;
#if defined(CONFIG_WIRELESS_CHARGING) || defined(CONFIG_CHARGER_MAX77888)
	int wpc_irq_gpio;
	int vbus_irq_gpio;
	bool wc_pwr_det;
#endif
};
#endif

#ifdef CONFIG_SS_VIBRATOR
#define MAX8997_MOTOR_REG_CONFIG2	0x2
#define MOTOR_LRA			(1<<7)
#define MOTOR_EN			(1<<6)
#define EXT_PWM				(0<<5)
#define DIVIDER_128			(1<<1)
#define DIVIDER_256			0x3

struct max77888_haptic_platform_data {
	u32 max_timeout;
	u32 duty;
	u32 period;
	u32 reg2;
	char *regulator_name;
	u32 pwm_id;

	void (*init_hw) (void);
	void (*motor_en) (bool);
};
#endif

#ifdef CONFIG_LEDS_MAX77888
struct max77888_led_platform_data;
#endif

struct max77888_regulator_data {
	int id;
	struct regulator_init_data *initdata;
};

struct max77888_platform_data {
	/* IRQ */
	u32 irq_base;
	u32 irq_base_flags;
	int irq_gpio;
#ifdef CONFIG_MUIC_RESET_PIN_ENABLE
	int irq_reset_gpio;
#endif
	u32 irq_gpio_flags;
	unsigned int wc_irq_gpio;
	bool wakeup;
	struct max77888_muic_data *muic;
//	bool (*is_default_uart_path_cp) (void);
	struct max77888_regulator_data *regulators;
	int num_regulators;
#ifdef CONFIG_SS_VIBRATOR
	/* haptic motor data */
	struct max77888_haptic_platform_data *haptic_data;
#endif
#ifdef CONFIG_LEDS_MAX77888
	/* led (flash/torch) data */
	struct max77888_led_platform_data *led_data;
#endif
	/* charger data */
#ifdef CONFIG_BATTERY_MAX77888_CHARGER
	struct max77888_charger_platform_data *charger_data;
#endif
#if defined(CONFIG_CHARGER_MAX77888)
	sec_battery_platform_data_t *charger_data;
#endif
};

enum cable_type_muic;
struct max77888_muic_data {
	void (*usb_cb) (u8 attached);
	void (*uart_cb) (u8 attached);
	int (*charger_cb) (enum cable_type_muic);
	void (*dock_cb) (int type);
	void (*mhl_cb) (int attached);
	void (*init_cb) (void);
	int (*set_safeout) (int path);
	 bool(*is_mhl_attached) (void);
	int (*cfg_uart_gpio) (void);
	void (*jig_uart_cb) (int path);
#if defined(CONFIG_MUIC_DET_JACK)
	void (*earjack_cb) (int attached);
	void (*sendend_cb) (int pressed);
#endif
	int (*host_notify_cb) (int enable);
	int gpio_usb_sel;
	int sw_path;
	int uart_path;

	void (*jig_state) (int jig_state);
	void (*check_id_state)(int state);

};

extern struct class *sec_class;
extern struct max77888_muic_data max77888_muic;
extern int muic_otg_control(int enable);
extern struct max77888_regulator_data max77888_regulators[];
extern struct max77888_haptic_platform_data max77888_haptic_pdata;
#ifdef CONFIG_LEDS_MAX77888
extern struct max77888_led_platform_data max77888_led_pdata;
#endif
#ifdef CONFIG_VIDEO_MHL_V2
int acc_register_notifier(struct notifier_block *nb);
#endif
#endif				/* __LINUX_MFD_MAX77888_H */
