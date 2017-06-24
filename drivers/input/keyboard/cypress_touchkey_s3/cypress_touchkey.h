/*
 * cypress_touchkey.h - Platform data for cypress touchkey driver
 *
 * Copyright (C) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __LINUX_CYPRESS_TOUCHKEY_H
#define __LINUX_CYPRESS_TOUCHKEY_H
extern struct class *sec_class;
extern int ISSP_main(void);

/* DVFS feature : TOUCH BOOSTER */
#define TSP_BOOSTER
#ifdef TSP_BOOSTER
#include <linux/cpufreq.h>

#define TOUCH_BOOSTER_OFF_TIME	300
#define TOUCH_BOOSTER_CHG_TIME	200
#endif
#ifdef CONFIG_LEDS_CLASS
#include <linux/leds.h>
#endif
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/mutex.h>

#if defined(CONFIG_GLOVE_TOUCH)
#define TK_BIT_GLOVE 0x40
#endif

//#define AUTOCAL_WORKQUEUE

/*#define TK_HOME_ENABLE*/

/* Flip cover*/
#define TKEY_FLIP_MODE

#ifdef TKEY_FLIP_MODE
#define TK_BIT_FLIP		0x08
#endif

#define TK_INFORM_CHARGER
#define TK_BIT_TA_ON		0x10

#define CYPRESS_55_IC_MASK	0x20
#define CYPRESS_65_IC_MASK	0x04

#define NUM_OF_KEY		4

#ifdef TK_INFORM_CHARGER
struct touchkey_callbacks {
	void (*inform_charger)(struct touchkey_callbacks *, bool);
};
#endif

struct cypress_touchkey_platform_data {
	unsigned	gpio_led_en;
	u32	touchkey_keycode[4];
	int	keycodes_size;
	void	(*power_onoff) (int);
	bool	skip_fw_update;
	bool	touchkey_order;
	void	(*register_cb)(void *);
	bool i2c_pull_up;
	int gpio_int;
	u32 irq_gpio_flags;
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;
	int vdd_led;
	int vdd_en;
};

struct cypress_touchkey_info {
	struct i2c_client			*client;
	struct cypress_touchkey_platform_data	*pdata;
	struct input_dev			*input_dev;
	struct early_suspend			early_suspend;
	char			phys[32];
	unsigned char			keycode[NUM_OF_KEY];
	u8			sensitivity[NUM_OF_KEY];
	int			irq;
	u8			fw_ver;
	void (*power_onoff)(int);
	int			touchkey_update_status;
	u8			ic_vendor;
	struct regulator *vcc_en;
	struct regulator *vdd_led;
#ifdef CONFIG_LEDS_CLASS
	struct led_classdev			leds;
	enum led_brightness			brightness;
	struct mutex			touchkey_mutex;
	struct mutex			fw_lock;
	struct workqueue_struct			*led_wq;
	struct work_struct			led_work;
#endif
#if defined(CONFIG_GLOVE_TOUCH)
	struct workqueue_struct		*glove_wq;
	struct work_struct		glove_work;
#endif
	bool is_powering_on;
	bool enabled;
	bool done_ta_setting;

#ifdef TKEY_FLIP_MODE
	bool enabled_flip;
#endif

#ifdef TSP_BOOSTER
	struct delayed_work	work_dvfs_off;
	struct delayed_work	work_dvfs_chg;
	bool dvfs_lock_status;
	struct mutex		dvfs_lock;
#endif

#ifdef TK_INFORM_CHARGER
	struct touchkey_callbacks callbacks;
	bool charging_mode;
#endif

	u32 fw_id;
#if defined(CONFIG_GLOVE_TOUCH)
	int glove_value;
#endif

};

void touchkey_charger_infom(bool en);


#define PM8921_IRQ_BASE			(NR_MSM_IRQS + NR_GPIO_IRQS)
#define IRQ_TOUCHKEY_INT PM8921_GPIO_IRQ(PMIC8058_IRQ_BASE, (PM8058_GPIO(31)))
#define GPIO_TOUCHKEY_SDA	83
#define GPIO_TOUCHKEY_SCL	84
#define GPIO_TOUCHKEY_SDA_2	95
#define GPIO_TOUCHKEY_SCL_2	96
#define PMIC_GPIO_TKEY_INT	79
#define PMIC_GPIO_TKEY_EN	32

#endif /* __LINUX_CYPRESS_TOUCHKEY_H */
