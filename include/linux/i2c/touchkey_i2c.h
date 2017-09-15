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

#ifdef CONFIG_SAMSUNG_LPM_MODE
extern int poweroff_charging;
#endif

/* DVFS feature : TOUCH BOOSTER */
#define TSP_BOOSTER
#ifdef TSP_BOOSTER
#include <linux/cpufreq.h>

#define DVFS_STAGE_DUAL		2
#define DVFS_STAGE_SINGLE		1
#define DVFS_STAGE_NONE		0
#define TOUCH_BOOSTER_OFF_TIME	500
#define TOUCH_BOOSTER_CHG_TIME	500
#endif

#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/mutex.h>
#include <linux/wakelock.h>

#if defined(CONFIG_SEC_K_PROJECT)
#define CYPRESS_SUPPORT_DUAL_INT_MODE
#define CYPRESS_RECENT_BACK_REPORT_FW_VER	0x1D
#define TK_CMD_INTERRUPT_SET_REG		0x00
#define CYPRESS_DETECTION_FLAG			0x08
#define TK_CMD_DUAL_DETECTION			0x01
#define TK_BIT_DETECTION_CONFIRM		0xEE
#elif defined(CONFIG_SEC_H_PROJECT)
#define CYPRESS_SUPPORT_DUAL_INT_MODE
#define CYPRESS_RECENT_BACK_REPORT_FW_VER	0x12
#define TK_CMD_INTERRUPT_SET_REG		0x18
#define CYPRESS_DETECTION_FLAG			0x1B
#define TK_CMD_DUAL_DETECTION			0x01
#define TK_BIT_DETECTION_CONFIRM		0xEE
#elif defined(CONFIG_SEC_KLIMT_PROJECT)
#define CYPRESS_SUPPORT_DUAL_INT_MODE
#define CYPRESS_RECENT_BACK_REPORT_FW_VER	0x09
#define TK_CMD_INTERRUPT_SET_REG		0x18
#define CYPRESS_DETECTION_FLAG			0x1B
#define TK_CMD_DUAL_DETECTION			0x01
#define TK_BIT_DETECTION_CONFIRM		0xEE
#define CRC_CHECK_DELAY
#elif defined(CONFIG_MACH_JS01LTEDCM)
#define CYPRESS_SUPPORT_DUAL_INT_MODE
#define CYPRESS_RECENT_BACK_REPORT_FW_VER	0x09
#define TK_CMD_INTERRUPT_SET_REG		0x18
#define CYPRESS_DETECTION_FLAG			0x1B
#define TK_CMD_DUAL_DETECTION			0x01
#define TK_BIT_DETECTION_CONFIRM		0xEE
#else
#undef CYPRESS_SUPPORT_DUAL_INT_MODE
#define CYPRESS_RECENT_BACK_REPORT_FW_VER	0xFF
#endif

#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY)
/* Touchkey Register */
#define CYPRESS_GEN			0X00
#define CYPRESS_OP_MODE			0X01
#define CYPRESS_SP_REG			0X02
#define CYPRESS_BUTTON_STATUS		0X03
#define CYPRESS_FW_VER			0X04
#define CYPRESS_MODULE_VER		0X05
#define CYPRESS_DEVICE_VER		0X06
#define CYPRESS_STATUS_FLAG		0X07
#define CYPRESS_THRESHOLD		0X09
#define CYPRESS_THRESHOLD2		0X0A
#define CYPRESS_THRESHOLD3		0X0B
#define CYPRESS_THRESHOLD4		0X0C
#define CYPRESS_IDAC_MENU		0x0D
#define CYPRESS_IDAC_MENU_INNER		0x0E
#define CYPRESS_IDAC_BACK		0x0F
#define CYPRESS_IDAC_BACK_INNER		0x10
#define CYPRESS_COMPIDAC_MENU		0x11
#define CYPRESS_COMPIDAC_MENU_INNER	0x12
#define CYPRESS_COMPIDAC_BACK		0x13
#define CYPRESS_COMPIDAC_BACK_INNER	0x14
#define CYPRESS_DIFF_MENU		0x16
#define CYPRESS_DIFF_MENU_INNER		0x18
#define CYPRESS_DIFF_BACK		0x1A
#define CYPRESS_DIFF_BACK_INNER		0x1C
#define CYPRESS_RAW_DATA_MENU		0x1E
#define CYPRESS_RAW_DATA_MENU_INNER	0x20
#define CYPRESS_RAW_DATA_BACK		0x22
#define CYPRESS_RAW_DATA_BACK_INNER	0x24
#define CYPRESS_BASE_DATA_MENU		0x26
#define CYPRESS_BASE_DATA_MENU_INNER	0x28
#define CYPRESS_BASE_DATA_BACK		0x2A
#define CYPRESS_BASE_DATA_BACK_INNER	0x2C
#define CYPRESS_DATA_UPDATE		0X40

#elif defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY_H) || defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY_C) || defined (CONFIG_KEYBOARD_CYPRESS_TOUCHKEY_HE)
#define CYPRESS_GEN		0X00
#define CYPRESS_FW_VER		0X01
#define CYPRESS_MODULE_VER	0X02
#define CYPRESS_2ND_HOST	0X03
#define CYPRESS_THRESHOLD	0X04
#define CYPRESS_AUTO_CAL_FLG	0X05
#define CYPRESS_IDAC_MENU	0X07
#define CYPRESS_IDAC_BACK	0X06
#define CYPRESS_IDAC_HOME	0X08
#define CYPRESS_DIFF_MENU	0x0C
#define CYPRESS_DIFF_BACK	0x0A
#define CYPRESS_DIFF_HOME	0x0E
#define CYPRESS_RAW_DATA_MENU	0x10
#define CYPRESS_RAW_DATA_BACK	0x0E
#define CYPRESS_RAW_DATA_HOME	0x12
#define CYPRESS_RAW_DATA_BACK_GOGH	0x14
#define CYPRESS_DATA_UPDATE	0X40
#define CYPRESS_AUTO_CAL	0X50
#define CYPRESS_SLEEP		0X80
#define CYPRESS_FW_ID_REG	0X05
#define USE_OPEN_CLOSE
#undef DO_NOT_USE_FUNC_PARAM
#define KEYCODE_REG		0x00
#endif

/* OP MODE CMD */
#define TK_BIT_CMD_LEDCONTROL	0x40    /* Owner for LED on/off control (0:host / 1:TouchIC) */
#define TK_BIT_CMD_INSPECTION	0x20    /* Inspection mode */
#define TK_BIT_CMD_1MM		0x10    /* 1mm stylus mode */
#define TK_BIT_CMD_FLIP		0x08    /* flip mode */
#define TK_BIT_CMD_GLOVE	0x04    /* glove mode */
#define TK_BIT_CMD_TA_ON	0x02    /* Ta mode */
#define TK_BIT_CMD_REGULAR	0x01    /* regular mode = normal mode */

#define TK_BIT_WRITE_CONFIRM	0xAA

/* STATUS FLAG */
#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY_H) || defined (CONFIG_KEYBOARD_CYPRESS_TOUCHKEY_C) || defined (CONFIG_KEYBOARD_CYPRESS_TOUCHKEY_HE)
#define TK_BIT_AUTOCAL		0x80
#define TK_BIT_GLOVE		0x40
#define TK_BIT_TA_ON		0x10
#define TK_BIT_FLIP		0x08
#else
#define TK_BIT_LEDCONTROL	0x40    /* Owner for LED on/off control (0:host / 1:TouchIC) */
#define TK_BIT_1MM		0x20    /* 1mm stylus mode */
#define TK_BIT_FLIP		0x10    /* flip mode */
#define TK_BIT_GLOVE		0x08    /* glove mode */
#define TK_BIT_TA_ON		0x04    /* Ta mode */
#define TK_BIT_REGULAR		0x02    /* regular mode = normal mode */
#define TK_BIT_LED_STATUS	0x01    /* LED status */
#endif

/* bit masks*/
#define PRESS_BIT_MASK		0X08
#define KEYCODE_BIT_MASK	0X07

#define TK_CMD_LED_ON		0x10
#define TK_CMD_LED_OFF		0x20

/*
#define TOUCHKEY_LOG(k, v) dev_notice(&info->client->dev, "key[%d] %d\n", k, v);
#define FUNC_CALLED dev_notice(&info->client->dev, "%s: called.\n", __func__);
*/
#define NUM_OF_RETRY_UPDATE	5
/*#define NUM_OF_KEY		4*/

#define USE_OPEN_CLOSE
#undef DO_NOT_USE_FUNC_PARAM


#define CONFIG_GLOVE_TOUCH
#ifdef CONFIG_GLOVE_TOUCH
#define	TKEY_GLOVE_DWORK_TIME	300
#endif

/* Flip cover*/
#define TKEY_FLIP_MODE
/* 1MM stylus */
#define TKEY_1MM_MODE

//#define TK_INFORM_CHARGER
#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY) && \
	!defined(CONFIG_EXTCON)
#define TK_INFORM_CHARGER
#elif defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY_H) || defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY_C) || defined (CONFIG_KEYBOARD_CYPRESS_TOUCHKEY_HE)
#undef TK_INFORM_CHARGER
#endif

#define CYPRESS_55_IC_MASK	0x20
#define CYPRESS_65_IC_MASK	0x04

#define NUM_OF_KEY		4

enum {
	CORERIVER_TOUCHKEY,
	CYPRESS_TOUCHKEY,
};

#ifdef TK_INFORM_CHARGER
struct touchkey_callbacks {
	void (*inform_charger)(struct touchkey_callbacks *, bool);
};
#endif

enum {
	UPDATE_NONE,
	DOWNLOADING,
	UPDATE_FAIL,
	UPDATE_PASS,
};

#define USE_TKEY_UPDATE_WORK
#define TKEY_REQUEST_FW_UPDATE

#ifdef TKEY_REQUEST_FW_UPDATE
#define TKEY_FW_BUILTIN_PATH	"tkey"
#define TKEY_FW_IN_SDCARD_PATH	"/sdcard/"

#define TKEY_SEMCO_CYPRESS_FW_NAME	"semco_cypress_tkey"
#define TKEY_SEMCO02_CYPRESS_FW_NAME	"semco02_cypress_tkey"
#define TKEY_SEMCO04_CYPRESS_FW_NAME	"semco04_cypress_tkey"
#define TKEY_DTECH_CYPRESS_FW_NAME	"dtech_cypress_tkey"

#ifdef CONFIG_MACH_JS01LTEDCM
#define TKEY_CORERIVER_FW_NAME      "hltejs01_coreriver_tkey"
#define TKEY_CYPRESS_FW_NAME        "hltejs01_cypress_tkey"
#else
#define TKEY_CORERIVER_FW_NAME		"hlte_coreriver_tkey"
#if defined(CONFIG_SEC_BERLUTI_PROJECT)
#define TKEY_CYPRESS_FW_NAME		"berluti_cypress_tkey"
#elif defined(CONFIG_SEC_KLIMT_PROJECT)
#define TKEY_CYPRESS_FW_NAME		"klimt_cypress_tkey"
#else
#define TKEY_CYPRESS_FW_NAME		"hlte_cypress_tkey"
#endif
#endif

enum {
	FW_BUILT_IN = 0,
	FW_IN_SDCARD,
};

struct fw_image {
	u8 hdr_ver;
	u8 hdr_len;
	u16 first_fw_ver;
	u16 second_fw_ver;
	u16 third_ver;
	u32 fw_len;
#if defined(CONFIG_SEC_K_PROJECT) || defined(CONFIG_SEC_S_PROJECT) || defined(CONFIG_SEC_PATEK_PROJECT) || defined(CONFIG_SEC_KLIMT_PROJECT)
	u16 checksum;
	u16 alignment_dummy;
#endif
	u8 data[0];
} __attribute__ ((packed));
#else

#define BIN_FW_VERSION	0
#endif

/* Divide model */
#if defined(CONFIG_SEC_KLIMT_PROJECT)
#define USE_SW_I2C // only use sw i2c line
#define ENABLE_FW_UPDATE //fw update after dtsi 02
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
	bool vcc_flag;
	bool fw_update_flag;
	int gpio_int;
	u32 irq_gpio_flags;
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;
	int gpio_touchkey_id;
	u32	gpio_touchkey_id_flags;
	int vdd_led;
	int vcc_en;
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
	void (*power_onoff)(int);
	u8			touchkey_update_status;
	u8			ic_vendor;
	struct regulator *vcc_en;
	struct regulator *vdd_led;
#ifdef CONFIG_GLOVE_TOUCH
#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY)
	struct delayed_work	glove_work;
	struct mutex	tkey_glove_lock;
#else
	struct workqueue_struct		*glove_wq;
	struct work_struct		glove_work;
#endif
	int glove_value;
#endif
#ifdef USE_TKEY_UPDATE_WORK
	struct workqueue_struct	*fw_wq;
	struct work_struct		update_work;
#endif
	bool is_powering_on;
	bool enabled;
	bool done_ta_setting;

#ifdef TKEY_FLIP_MODE
	bool enabled_flip;
#endif
#ifdef TKEY_1MM_MODE
	bool enabled_1mm;
#endif

#ifdef TSP_BOOSTER
	struct delayed_work	work_dvfs_off;
	struct delayed_work	work_dvfs_chg;
	struct mutex		dvfs_lock;
	bool dvfs_lock_status;
	int dvfs_old_stauts;
	int dvfs_boost_mode;
	int dvfs_freq;
#endif

#ifdef TK_INFORM_CHARGER
	struct touchkey_callbacks callbacks;
	bool charging_mode;
#endif
#ifdef TKEY_REQUEST_FW_UPDATE
	const struct firmware		*fw;	
	struct fw_image	*fw_img;
	u8	cur_fw_path;
#endif
	int	src_fw_ver;
	int	ic_fw_ver;
	int	module_ver;
	int	device_ver;	
	u32 fw_id;

	u8	touchkeyid;
	bool	support_fw_update;
	bool	do_checksum;
	struct wake_lock fw_wakelock;
};

#ifdef TK_INFORM_CHARGER
void touchkey_charger_infom(bool en);
#endif

/* TKEY MODULE 0x6 */
#if defined(CONFIG_MACH_KLTE_EUR) || defined(CONFIG_MACH_KWIFI_LDU)
#define TKEY_MODULE_CHECK_REV		0xA
#elif defined(CONFIG_MACH_KLTE_ATT) || defined(CONFIG_MACH_KLTE_AUS_TEL)
#define TKEY_MODULE_CHECK_REV		0xE
#elif defined(CONFIG_MACH_KLTE_CHN) || defined(CONFIG_MACH_KLTE_KOR) || defined(CONFIG_MACH_KLTE_JPN) \
	|| defined(CONFIG_MACH_KLTE_SPR) || defined(CONFIG_MACH_KLTE_VZW) || defined(CONFIG_MACH_KLTE_LRA) \
	|| defined(CONFIG_MACH_KLTE_TMO) || defined(CONFIG_MACH_KLTE_CAN) || defined(CONFIG_MACH_KLTE_MTR) \
	|| defined(CONFIG_MACH_KLTE_USC) || defined(CONFIG_MACH_KLTE_ACG) || defined(CONFIG_MACH_K3GDUOS_CTC)
#define TKEY_MODULE_CHECK_REV		0x9
#else
#define TKEY_MODULE_CHECK_REV		0xFF
#endif

extern int coreriver_fw_update(struct cypress_touchkey_info *info, bool force);

#define PM8921_IRQ_BASE			(NR_MSM_IRQS + NR_GPIO_IRQS)
#if defined(CONFIG_KEYBOARD_CYPRESS_TOUCHKEY_C)
#define GPIO_TOUCHKEY_SDA	22
#define GPIO_TOUCHKEY_SCL	23
#define PMIC_GPIO_TKEY_INT	49
#elif defined(CONFIG_SEC_KLIMT_PROJECT)
#define GPIO_TOUCHKEY_SDA	10
#define GPIO_TOUCHKEY_SCL	11
#define GPIO_TKEY_INT	144
#else
#define GPIO_TOUCHKEY_SDA	95
#define GPIO_TOUCHKEY_SCL	96
#define PMIC_GPIO_TKEY_INT	79
#endif
extern void cypress_power_onoff(struct cypress_touchkey_info *info, int onoff);
#endif /* __LINUX_CYPRESS_TOUCHKEY_H */
