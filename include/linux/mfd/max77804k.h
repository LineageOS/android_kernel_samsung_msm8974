/*
 * max77804k.h - Driver for the Maxim 77804k
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
 * MAX77804K has Charger, Flash LED, Haptic, MUIC devices.
 * The devices share the same I2C bus and included in
 * this mfd driver.
 */

#ifndef __LINUX_MFD_MAX77804K_H
#define __LINUX_MFD_MAX77804K_H

#include <linux/regulator/consumer.h>
#include <linux/battery/sec_charger.h>

enum {
#if defined (CONFIG_MUIC_MAX77804K_SUPPORT_LANHUB)
	MAX77804K_MUIC_NONE = -1,
#endif
	MAX77804K_MUIC_DETACHED = 0,
	MAX77804K_MUIC_ATTACHED
};

enum {
	PATH_OPEN = 0,
	PATH_USB_AP,
	PATH_AUDIO,
	PATH_UART_AP,
	PATH_USB_CP,
	PATH_UART_CP,
};

/* ATTACHED Dock Type */
enum {
	MAX77804K_MUIC_DOCK_DETACHED,
	MAX77804K_MUIC_DOCK_DESKDOCK,
	MAX77804K_MUIC_DOCK_CARDOCK,
	MAX77804K_MUIC_DOCK_AUDIODOCK = 7,
	MAX77804K_MUIC_DOCK_SMARTDOCK = 8,
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
	MAX77804K_MUIC_DOCK_HMT = 11,
#endif
};

/* MAX77686 regulator IDs */
enum max77804k_regulators {
	MAX77804K_ESAFEOUT1 = 0,
	MAX77804K_ESAFEOUT2,

	MAX77804K_CHARGER,

	MAX77804K_REG_MAX,
};

struct max77804k_charger_reg_data {
	u8 addr;
	u8 data;
};

struct max77804k_charger_platform_data {
	struct max77804k_charger_reg_data *init_data;
	int num_init_data;
	sec_battery_platform_data_t *sec_battery;
#if defined(CONFIG_WIRELESS_CHARGING) || defined(CONFIG_CHARGER_MAX77804K)
	int wpc_irq_gpio;
	int vbus_irq_gpio;
	bool wc_pwr_det;
#endif
};

#ifdef CONFIG_SS_VIBRATOR
#define MAX8997_MOTOR_REG_CONFIG2	0x2
#define MOTOR_LRA			(1<<7)
#define MOTOR_EN			(1<<6)
#define EXT_PWM				(0<<5)
#define DIVIDER_128			(1<<1)
#define DIVIDER_256			0x3

struct max77804k_haptic_platform_data {
	u32 max_timeout;
	u32 duty;
	u32 period;
	u32 reg2;
	u32 pwm_id;
};
#endif

#ifdef CONFIG_LEDS_MAX77804K
struct max77804k_led_platform_data;
#endif

struct max77804k_regulator_data {
	int id;
	struct regulator_init_data *initdata;
};

struct max77804k_platform_data {
	/* IRQ */
	u32 irq_base;
	u32 irq_base_flags;
	int irq_gpio;
	u32 irq_gpio_flags;
	unsigned int wc_irq_gpio;
	bool wakeup;
	struct max77804k_muic_data *muic_data;
	struct max77804k_regulator_data *regulators;
	int num_regulators;
#ifdef CONFIG_SS_VIBRATOR
	/* haptic motor data */
	struct max77804k_haptic_platform_data *haptic_data;
#endif
#ifdef CONFIG_LEDS_MAX77804K
	/* led (flash/torch) data */
	struct max77804k_led_platform_data *led_data;
#endif
#if defined(CONFIG_CHARGER_MAX77804K)
	sec_battery_platform_data_t *charger_data;
#endif
};

enum cable_type_muic;
struct max77804k_muic_data {
#if !defined(CONFIG_EXTCON)
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
	int (*host_notify_cb) (int enable);
	int gpio_usb_sel;
	int sw_path;
	int uart_path;
	void (*jig_state) (int jig_state);
#else
	int usb_sel;
	int uart_sel;
#endif
};

#ifdef CONFIG_MFD_MAX77804K
extern struct max77804k_muic_data max77804k_muic;
extern struct max77804k_regulator_data max77804k_regulators[];
extern int muic_otg_control(int enable);
extern struct max77804k_haptic_platform_data max77804k_haptic_pdata;
extern struct max77804k_led_platform_data max77804k_led_pdata;
extern int max77804k_muic_set_safeout(int path);
#ifndef CONFIG_USB_DWC3
extern void sec_otg_set_vbus_state(int online);
extern int sec_otg_notify(int event);
#endif
#endif
#if defined (CONFIG_VIDEO_MHL_V2) || defined (CONFIG_VIDEO_MHL_SII8246)
int acc_register_notifier(struct notifier_block *nb);
#endif
#endif				/* __LINUX_MFD_MAX77804K_H */
