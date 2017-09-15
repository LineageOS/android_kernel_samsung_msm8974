
#ifndef _LINUX_WACOM_I2C_H
#define _LINUX_WACOM_I2C_H

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <linux/cpufreq.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

/* WACOM NOISE from LCD OSC.
 * read Vsync frequency value at wacom firmware.
 */
#if defined(CONFIG_SEC_H_PROJECT)
#define USE_WACOM_LCD_WORKAROUND
#else
#undef USE_WACOM_LCD_WORKAROUND
#endif

/* wacom region : touch region, touchkey region,
 * HW team request : block touchkey event between touch release ~ 100msec.
 */
#define USE_WACOM_BLOCK_KEYEVENT
/* FW UPDATE @ SYSTEM REVISION -- DIGITIGER CHANGED */
#define WACOM_FW_UPDATE_REVISION	0x05

/* FW UPDATE @ SYSTEM REVISION -- BOOT VERSION CHANGED */
#define WACOM_BOOT_REVISION	0x7

/* FW UPDATE FEATURE */
#define WACOM_UMS_UPDATE
#define WACOM_MAX_FW_PATH		64
#define WACOM_FW_NAME_W9001		"epen/W9001_B911.bin"
#if defined(CONFIG_MACH_HLTESKT) || defined(CONFIG_MACH_HLTEKTT) || defined(CONFIG_MACH_HLTELGT) ||\
	defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI)
#define WACOM_FW_NAME_W9010		"epen/W9010_0208.bin"
#else
#define WACOM_FW_NAME_W9010		"epen/W9010_0174.bin"
#endif

#ifdef CONFIG_SEC_LT03_PROJECT 
#define WACOM_FW_NAME_W9007_BL92		"epen/W9007A_0267.bin"
#define WACOM_FW_NAME_W9007_BL91		"epen/W9007A_0267.bin"
#else
#define WACOM_FW_NAME_W9007_BL92		"epen/W9007A_0450.bin"
#define WACOM_FW_NAME_W9007_BL91		"epen/W9007_0200.bin"
#endif

#define WACOM_FW_PATH		"/sdcard/firmware/wacom_firm.bin"
#define WACOM_FW_NAME_NONE	NULL

#define NAMEBUF 12
#define WACNAME "WAC_I2C_EMR"
#define WACFLASH "WAC_I2C_FLASH"

extern unsigned int system_rev;

/*Wacom Command*/
#define COM_COORD_NUM	8
#define COM_COORD_NUM_W9010		12
#define COM_QUERY_NUM	9

#define COM_SAMPLERATE_STOP 0x30
#define COM_SAMPLERATE_40  0x33
#define COM_SAMPLERATE_80  0x32
#define COM_SAMPLERATE_133 0x31
#define COM_SURVEYSCAN     0x2B
#define COM_QUERY          0x2A
#define COM_FLASH          0xff
#define COM_CHECKSUM       0x63

#if defined(CONFIG_SEC_LT03_PROJECT) || defined(CONFIG_SEC_VIENNA_PROJECT)
#define WACOM_RESETPIN_DELAY
#endif

/*I2C address for digitizer and its boot loader*/
#define WACOM_I2C_ADDR 0x56
#define WACOM_I2C_BOOT 0x09
#define WACOM_I2C_9001_BOOT 0x57
/*Information for input_dev*/
#define EMR 0
#define WACOM_PKGLEN_I2C_EMR 0

/*Enable/disable irq*/
#define ENABLE_IRQ 1
#define DISABLE_IRQ 0

/*Special keys*/
#define EPEN_TOOL_PEN		0x220
#define EPEN_TOOL_RUBBER	0x221
#define EPEN_STYLUS			0x22b
#define EPEN_STYLUS2		0x22c

#define WACOM_DELAY_FOR_RST_RISING 200
/* #define INIT_FIRMWARE_FLASH */

#define WACOM_PDCT_WORK_AROUND

/*****************/
#define WACOM_USE_QUERY_DATA

/*PDCT Signal*/
#define PDCT_NOSIGNAL 1
#define PDCT_DETECT_PEN 0

/*Digitizer Type*/
#define EPEN_DTYPE_B660	1
#define EPEN_DTYPE_B713 2
#define EPEN_DTYPE_B746 3
#define EPEN_DTYPE_B887 4
#define EPEN_DTYPE_B911 5
#define EPEN_DTYPE_B930	6
#define EPEN_DTYPE_B934	7
#define EPEN_DTYPE_B968	8

#define WACOM_HAVE_FWE_PIN

#define WACOM_USE_SOFTKEY

#define COOR_WORK_AROUND

#define WACOM_IMPORT_FW_ALGO
/*#define WACOM_USE_OFFSET_TABLE*/
#undef WACOM_USE_AVERAGING
#if 0
#define WACOM_USE_AVE_TRANSITION
#define WACOM_USE_BOX_FILTER
#define WACOM_USE_TILT_OFFSET
#define WACOM_USE_HEIGHT
#endif

#define WACOM_USE_GAIN

#define MAX_ROTATION	4
#define MAX_HAND		2

#define WACOM_PEN_DETECT
/*****************/

/* origin offset */
#define EPEN_B660_ORG_X 456
#define EPEN_B660_ORG_Y 504

#define EPEN_B713_ORG_X 676
#define EPEN_B713_ORG_Y 724

#define WACOM_BOOSTER
#ifdef WACOM_BOOSTER
#define DVFS_STAGE_TRIPLE	3
#define DVFS_STAGE_DUAL		2
#define DVFS_STAGE_SINGLE		1
#define DVFS_STAGE_NONE		0
#define TOUCH_BOOSTER_OFF_TIME	500
#define TOUCH_BOOSTER_CHG_TIME	130
#endif

#define BATTERY_SAVING_MODE
#define WACOM_CONNECTION_CHECK
/**************/

/*HWID to distinguish Detect Switch*/
#define WACOM_DETECT_SWITCH_HWID 0xFFFF

/*HWID to distinguish FWE1*/
#define WACOM_FWE1_HWID 0xFFFF

/*HWID to distinguish B911 Digitizer*/
#define WACOM_DTYPE_B911_HWID 1

/*End of Model config*/

#ifdef BATTERY_SAVING_MODE
#ifndef WACOM_PEN_DETECT
#define WACOM_PEN_DETECT
#endif
#endif

#ifdef WACOM_USE_PDATA
#undef WACOM_USE_QUERY_DATA
#endif

#define WACOM_COORDS_ARR_SIZE	9

/*Parameters for wacom own features*/
struct wacom_features {
	int x_max;
	int y_max;
	int pressure_max;
	char comstat;
	u8 data[COM_COORD_NUM_W9010];
	unsigned int fw_ic_version;
	unsigned int fw_version;
	int firm_update_status;
};

/*sec_class sysfs*/
extern struct class *sec_class;

#ifdef CONFIG_SAMSUNG_LPM_MODE
extern int poweroff_charging;
#endif

struct wacom_g5_callbacks {
	int (*check_prox)(struct wacom_g5_callbacks *);
};

/*Parameters for i2c driver*/
struct wacom_i2c {
	struct i2c_client *client;
	struct i2c_client *client_boot;
	struct input_dev *input_dev;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	struct mutex lock;
#if defined(CONFIG_SEC_LT03_PROJECT) || defined(CONFIG_SEC_VIENNA_PROJECT)
	struct mutex irq_lock;
#endif
#ifdef WACOM_BOOSTER
	struct delayed_work	work_dvfs_off;
	struct delayed_work	work_dvfs_chg;
	struct mutex		dvfs_lock;
	bool dvfs_lock_status;
	int dvfs_old_stauts;
	int dvfs_boost_mode;
	int dvfs_freq;
#endif

	struct device	*dev;
	int irq;
#ifdef WACOM_PDCT_WORK_AROUND
	int irq_pdct;
	bool rdy_pdct;
#endif
	int pen_pdct;
	int gpio;
	int irq_flag;
	int pen_prox;
	int pen_pressed;
	int side_pressed;
	int tool;
	s16 last_x;
	s16 last_y;
#ifdef WACOM_PEN_DETECT
	int irq_pen_insert;
	struct delayed_work pen_insert_dwork;
	bool pen_insert;
	int gpio_pen_insert;
#endif
#ifdef WACOM_RESETPIN_DELAY
	struct delayed_work work_wacom_reset;
#endif
	int invert_pen_insert;
#ifdef WACOM_HAVE_FWE_PIN
	int gpio_fwe;
	bool have_fwe_pin;
#endif
#ifdef WACOM_IMPORT_FW_ALGO
	bool use_offset_table;
	bool use_aveTransition;
#endif
	bool checksum_result;
	const char name[NAMEBUF];
	struct wacom_features *wac_feature;
	struct wacom_g5_platform_data *wac_pdata;
	struct wacom_g5_callbacks callbacks;
	int (*power)(int on);
	struct delayed_work resume_work;

#ifdef BATTERY_SAVING_MODE
	bool battery_saving_mode;
#endif
	bool power_enable;
	bool boot_mode;
	bool query_status;
	int ic_mpu_ver;
	int boot_ver;
	bool init_fail;
#ifdef USE_WACOM_LCD_WORKAROUND
	unsigned int vsync;
	struct delayed_work read_vsync_work;
	struct delayed_work boot_done_work;
	bool wait_done;
	bool boot_done;
	unsigned int delay_time;
#endif
#ifdef USE_WACOM_BLOCK_KEYEVENT
	struct delayed_work	touch_pressed_work;
	unsigned int key_delay_time;
	bool touchkey_skipped;
	bool touch_pressed;
#endif
	bool enabled;
};

struct wacom_g5_platform_data {
	char *name;
/* using dts feature */
	const char *basic_model;
	bool i2c_pull_up;
	int gpio_int;
	u32 irq_gpio_flags;
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;
	int gpio_pdct;
	u32 pdct_gpio_flags;
	int vdd_en;
	int gpio_pen_reset_n;
	u32 pen_reset_n_gpio_flags;
	int gpio_pen_fwe1;
	u32 pen_fwe1_gpio_flags;
	int gpio_pen_pdct;
	u32 pen_pdct_gpio_flags;

	int x_invert;
	int y_invert;
	int xy_switch;
	int min_x;
	int max_x;
	int min_y;
	int max_y;
	int max_pressure;
	int min_pressure;
	int gpio_pendct;
	u32 ic_mpu_ver;
	u32 irq_flags;
/* using dts feature */
#ifdef WACOM_PEN_DETECT
	int gpio_pen_insert;
#endif
#ifdef WACOM_HAVE_FWE_PIN
	void (*compulsory_flash_mode)(struct wacom_i2c *, bool);
#endif
	int (*reset_platform_hw)(struct wacom_i2c *);
	int (*wacom_start)(struct wacom_i2c *);
	int (*wacom_stop)(struct wacom_i2c *);

	void (*register_cb)(struct wacom_g5_callbacks *);
};

#endif /* _LINUX_WACOM_I2C_H */
