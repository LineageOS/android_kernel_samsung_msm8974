#ifndef _LINUX_WACOM_H
#define _LINUX_WACOM_H

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
#include <linux/firmware.h>
#include <linux/pm_qos.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "wacom_i2c.h"

#ifdef CONFIG_INPUT_BOOSTER
#include <linux/input/input_booster.h>
#endif

#if defined(CONFIG_MACH_FRESCONEOLTE_CTC)
#define GPIO_PEN_LDO_EN			6	// pmic gpio
#define GPIO_PEN_RESET_N_18V	13
#define GPIO_PEN_SDA_18V		14
#define GPIO_PEN_SCL_N_18V		15
#define GPIO_WACOM_SENSE		54
#define GPIO_PEN_PDCT_18V		110 
#define GPIO_PEN_FWE1_18V		119
#define GPIO_PEN_IRQ_18V		120
#elif defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
#define GPIO_PEN_SDA_18V		29
#define GPIO_PEN_SCL_N_18V		30
#endif

#define NAMEBUF 12
#define WACNAME "WAC_I2C_EMR"
#define WACFLASH "WAC_I2C_FLASH"

#ifdef CONFIG_EPEN_WACOM_G9PM
#define WACOM_FW_SIZE 61440
#elif defined(CONFIG_EPEN_WACOM_G9PLL) \
	|| defined(CONFIG_EPEN_WACOM_G10PM)
#define WACOM_FW_SIZE 65535
#else
#define WACOM_FW_SIZE 32768
#endif

/*Wacom Command*/
#if defined(CONFIG_HA) || defined(CONFIG_V1A) || \
	defined(CONFIG_MACH_HLLTE) || defined(CONFIG_MACH_HL3G) || \
	defined(CONFIG_MACH_FRESCONEOLTE_CTC) || defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
#define COM_COORD_NUM	12
#elif defined(CONFIG_N1A)
#define COM_COORD_NUM	8
#else
#define COM_COORD_NUM	7
#endif

#if defined(CONFIG_MACH_HLLTE) || defined(CONFIG_MACH_HL3G) || \
	defined(CONFIG_MACH_FRESCONEOLTE_CTC) || defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
#define COM_QUERY_NUM 13
#elif defined(CONFIG_HA)
#define COM_QUERY_NUM	11
#else
#define COM_QUERY_NUM	9
#endif

#define COM_SAMPLERATE_STOP 0x30
#define COM_SAMPLERATE_40  0x33
#define COM_SAMPLERATE_80  0x32
#define COM_SAMPLERATE_133 0x31
#define COM_SURVEYSCAN     0x2B
#define COM_QUERY          0x2A
#define COM_FLASH          0xff
#define COM_CHECKSUM       0x63

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
#define WACOM_USE_QUERY_DATA

/*PDCT Signal*/
#define PDCT_NOSIGNAL 1
#define PDCT_DETECT_PEN 0

#define WACOM_PRESSURE_MAX 1023

/*Digitizer Type*/
#define EPEN_DTYPE_B660	1
#define EPEN_DTYPE_B713 2
#define EPEN_DTYPE_B746 3
#define EPEN_DTYPE_B804 4
#define EPEN_DTYPE_B878 5

#define EPEN_DTYPE_B887 6
#define EPEN_DTYPE_B911 7
#define EPEN_DTYPE_B934 8
#define EPEN_DTYPE_B968 9

#define WACOM_I2C_MODE_BOOT 1
#define WACOM_I2C_MODE_NORMAL 0

#define EPEN_RESUME_DELAY 180

/* Wacom Booster */
#if !defined(CONFIG_INPUT_BOOSTER)
//#define WACOM_BOOSTER
#endif

#if defined(CONFIG_MACH_HLLTE) || defined(CONFIG_MACH_HL3G) || \
	defined(CONFIG_MACH_FRESCONEOLTE_CTC) || defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
/* softkey block workaround */
#define WACOM_USE_SOFTKEY_BLOCK
#define SOFTKEY_BLOCK_DURATION (HZ / 10)

#define WACOM_BOOSTER_CPU_FREQ1 1600000
#define WACOM_BOOSTER_MIF_FREQ1 667000
#define WACOM_BOOSTER_INT_FREQ1 333000

#define WACOM_BOOSTER_CPU_FREQ2 650000
#define WACOM_BOOSTER_MIF_FREQ2 400000
#define WACOM_BOOSTER_INT_FREQ2 111000

#define WACOM_BOOSTER_CPU_FREQ3 650000
#define WACOM_BOOSTER_MIF_FREQ3 667000
#define WACOM_BOOSTER_INT_FREQ3 333000

/* LCD freq sync */
//#define LCD_FREQ_SYNC

#ifdef LCD_FREQ_SYNC
#define LCD_FREQ_BOTTOM 60100
#define LCD_FREQ_TOP 60500
#endif

#define LCD_FREQ_SUPPORT_HWID 8

/*IRQ TRIGGER TYPE*/
#ifdef CONFIG_EPEN_WACOM_G9PM
#define EPEN_IRQF_TRIGGER_TYPE IRQF_TRIGGER_RISING
#else
#define EPEN_IRQF_TRIGGER_TYPE IRQF_TRIGGER_FALLING
#endif

#define WACOM_USE_PDATA

#define WACOM_USE_SOFTKEY

/* For Android origin */
#define WACOM_POSX_MAX WACOM_MAX_COORD_Y
#define WACOM_POSY_MAX WACOM_MAX_COORD_X

#define COOR_WORK_AROUND

#define WACOM_IMPORT_FW_ALGO
/*#define WACOM_USE_OFFSET_TABLE*/
#if 0
#define WACOM_USE_AVERAGING
#define WACOM_USE_AVE_TRANSITION
#define WACOM_USE_BOX_FILTER
#define WACOM_USE_TILT_OFFSET
#endif
#define WACOM_USE_GAIN

#define MAX_ROTATION	4
#define MAX_HAND		2

#define WACOM_PEN_DETECT

/* origin offset */
#define EPEN_B660_ORG_X 456
#define EPEN_B660_ORG_Y 504

#define EPEN_B713_ORG_X 676
#define EPEN_B713_ORG_Y 724

/*Box Filter Parameters*/
#define  X_INC_S1  1500
#define  X_INC_E1  (WACOM_MAX_COORD_X - 1500)
#define  Y_INC_S1  1500
#define  Y_INC_E1  (WACOM_MAX_COORD_Y - 1500)

#define  Y_INC_S2  500
#define  Y_INC_E2  (WACOM_MAX_COORD_Y - 500)
#define  Y_INC_S3  1100
#define  Y_INC_E3  (WACOM_MAX_COORD_Y - 1100)

#define BATTERY_SAVING_MODE

/*HWID to distinguish Detect Switch*/
#define WACOM_DETECT_SWITCH_HWID 0xFFFF

/*HWID to distinguish FWE1*/
#define WACOM_FWE1_HWID 0xFFFF

/*HWID to distinguish Digitizer*/
#define WACOM_DTYPE_B934_HWID 4
#define WACOM_DTYPE_B968_HWID 6

#elif defined(CONFIG_V1A)
#define WACOM_CONNECTION_CHECK

/* softkey block workaround */
#define WACOM_USE_SOFTKEY_BLOCK
#define SOFTKEY_BLOCK_DURATION (HZ / 10)

/*IRQ TRIGGER TYPE*/
#define EPEN_IRQF_TRIGGER_TYPE IRQF_TRIGGER_RISING

#define WACOM_USE_SOFTKEY
#define WACOM_USE_GAIN

#define WACOM_X_INVERT 0
#define WACOM_XY_SWITCH 0

#define BATTERY_SAVING_MODE

#define WACOM_PEN_DETECT

/* For Android origin */
/*v1, both origins are same */
#define WACOM_POSX_MAX WACOM_MAX_COORD_X
#define WACOM_POSY_MAX WACOM_MAX_COORD_Y

#define COOR_WORK_AROUND

#define WACOM_BOOSTER_CPU_FREQ1 1600000
#define WACOM_BOOSTER_MIF_FREQ1 800000
#define WACOM_BOOSTER_INT_FREQ1 400000

#define WACOM_BOOSTER_CPU_FREQ2 650000
#define WACOM_BOOSTER_MIF_FREQ2 400000
#define WACOM_BOOSTER_INT_FREQ2 222000

#define WACOM_BOOSTER_CPU_FREQ3 650000
#define WACOM_BOOSTER_MIF_FREQ3 400000
#define WACOM_BOOSTER_INT_FREQ3 222000

#elif defined(CONFIG_N1A)

#define WACOM_CONNECTION_CHECK

#define EPEN_IRQF_TRIGGER_TYPE IRQF_TRIGGER_RISING

#define WACOM_HAVE_FWE_PIN

#define WACOM_USE_SOFTKEY
#define WACOM_USE_GAIN

#define WACOM_X_INVERT 0
#define WACOM_XY_SWITCH 0

#define BATTERY_SAVING_MODE

#define WACOM_PEN_DETECT

/* For Android origin */
/*v1, both origins are same */
#define WACOM_POSX_MAX WACOM_MAX_COORD_X
#define WACOM_POSY_MAX WACOM_MAX_COORD_Y

#define COOR_WORK_AROUND

/* softkey block workaround */
#define WACOM_USE_SOFTKEY_BLOCK
#define SOFTKEY_BLOCK_DURATION (HZ / 10)

#define WACOM_BOOSTER_CPU_FREQ1 1600000
#define WACOM_BOOSTER_MIF_FREQ1 800000
#define WACOM_BOOSTER_INT_FREQ1 400000

#define WACOM_BOOSTER_CPU_FREQ2 650000
#define WACOM_BOOSTER_MIF_FREQ2 400000
#define WACOM_BOOSTER_INT_FREQ2 222000

#define WACOM_BOOSTER_CPU_FREQ3 650000
#define WACOM_BOOSTER_MIF_FREQ3 400000
#define WACOM_BOOSTER_INT_FREQ3 222000

#elif defined(CONFIG_HA)
/* softkey block workaround */
#define WACOM_USE_SOFTKEY_BLOCK
#define SOFTKEY_BLOCK_DURATION (HZ / 10)

#define WACOM_BOOSTER_CPU_FREQ1 1600000
#define WACOM_BOOSTER_MIF_FREQ1 667000
#define WACOM_BOOSTER_INT_FREQ1 333000

#define WACOM_BOOSTER_CPU_FREQ2 650000
#define WACOM_BOOSTER_MIF_FREQ2 400000
#define WACOM_BOOSTER_INT_FREQ2 111000

#define WACOM_BOOSTER_CPU_FREQ3 650000
#define WACOM_BOOSTER_MIF_FREQ3 667000
#define WACOM_BOOSTER_INT_FREQ3 333000

/* LCD freq sync */
#define LCD_FREQ_SYNC

#ifdef LCD_FREQ_SYNC
#define LCD_FREQ_BOTTOM 60100
#define LCD_FREQ_TOP 60500
#endif

#define LCD_FREQ_SUPPORT_HWID 8

/*IRQ TRIGGER TYPE*/
#define EPEN_IRQF_TRIGGER_TYPE IRQF_TRIGGER_FALLING

#define WACOM_USE_PDATA

#define WACOM_USE_SOFTKEY

/* For Android origin */
#define WACOM_POSX_MAX WACOM_MAX_COORD_Y
#define WACOM_POSY_MAX WACOM_MAX_COORD_X

#define COOR_WORK_AROUND

#define WACOM_IMPORT_FW_ALGO
/*#define WACOM_USE_OFFSET_TABLE*/
#if 0
#define WACOM_USE_AVERAGING
#define WACOM_USE_AVE_TRANSITION
#define WACOM_USE_BOX_FILTER
#define WACOM_USE_TILT_OFFSET
#endif
#define WACOM_USE_GAIN

#define MAX_ROTATION	4
#define MAX_HAND		2

#define WACOM_PEN_DETECT

/* origin offset */
#define EPEN_B660_ORG_X 456
#define EPEN_B660_ORG_Y 504

#define EPEN_B713_ORG_X 676
#define EPEN_B713_ORG_Y 724

/*Box Filter Parameters*/
#define  X_INC_S1  1500
#define  X_INC_E1  (WACOM_MAX_COORD_X - 1500)
#define  Y_INC_S1  1500
#define  Y_INC_E1  (WACOM_MAX_COORD_Y - 1500)

#define  Y_INC_S2  500
#define  Y_INC_E2  (WACOM_MAX_COORD_Y - 500)
#define  Y_INC_S3  1100
#define  Y_INC_E3  (WACOM_MAX_COORD_Y - 1100)

#define BATTERY_SAVING_MODE

/*HWID to distinguish Detect Switch*/
#define WACOM_DETECT_SWITCH_HWID 0xFFFF

/*HWID to distinguish FWE1*/
#define WACOM_FWE1_HWID 0xFFFF

/*HWID to distinguish Digitizer*/
#define WACOM_DTYPE_B934_HWID 4
#define WACOM_DTYPE_B968_HWID 6

#endif /*End of Model config*/

#define WACOM_BOOSTER_OFF_TIME	500
#define WACOM_BOOSTER_CHG_TIME	130

enum BOOST_LEVEL {
	WACOM_BOOSTER_DISABLE = 0,
	WACOM_BOOSTER_LEVEL1,
	WACOM_BOOSTER_LEVEL2,
	WACOM_BOOSTER_LEVEL3,
};

#if !defined(WACOM_SLEEP_WITH_PEN_SLP)
#define WACOM_SLEEP_WITH_PEN_LDO_EN
#endif

#ifdef BATTERY_SAVING_MODE
#ifndef WACOM_PEN_DETECT
#define WACOM_PEN_DETECT
#endif
#endif

#ifdef WACOM_USE_PDATA
#undef WACOM_USE_QUERY_DATA
#endif


//#ifdef CONFIG_SEC_DVFS
#include <linux/cpufreq.h>
#define WACOM_BOOSTER_DVFS
#define DVFS_STAGE_TRIPLE       3
#define DVFS_STAGE_DUAL         2
#define DVFS_STAGE_SINGLE       1
#define DVFS_STAGE_NONE         0
//#endif


/*Parameters for wacom own features*/
struct wacom_features {
	int x_max;
	int y_max;
	int pressure_max;
	char comstat;
	u8 data[COM_COORD_NUM];
	unsigned int fw_version;
	int firm_update_status;
};

enum {
	FW_BUILT_IN = 0,
	FW_HEADER,
	FW_IN_SDCARD,
	FW_EX_SDCARD,
};

struct fw_update_info {
	u8 fw_path;
	bool forced;
	const struct firmware *firm_data;
	u8 *fw_data;
};

/*Parameters for i2c driver*/
struct wacom_i2c {
	struct i2c_client *client;
	struct i2c_client *client_boot;
	struct completion init_done;
	struct input_dev *input_dev;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	struct mutex lock;
	struct mutex update_lock;
	struct mutex irq_lock;
	struct wake_lock fw_wakelock;
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
	struct delayed_work pen_insert_dwork;
	bool pen_insert;
	int gpio_pen_insert;
#ifdef CONFIG_MACH_T0
	int invert_pen_insert;
#endif
#endif
	int gpio_fwe;
#ifdef WACOM_IMPORT_FW_ALGO
	bool use_offset_table;
	bool use_aveTransition;
#endif
	bool checksum_result;
	struct wacom_features *wac_feature;
	struct wacom_g5_platform_data *wac_pdata;
	struct wacom_g5_callbacks callbacks;
	int (*power)(int on);
	struct delayed_work resume_work;
#ifdef WACOM_CONNECTION_CHECK
	bool connection_check;
#endif
#ifdef BATTERY_SAVING_MODE
	bool battery_saving_mode;
#endif
#if defined(WACOM_BOOSTER_DVFS)
	struct delayed_work	work_dvfs_off;
	struct delayed_work	work_dvfs_chg;
	struct mutex		dvfs_lock;
	bool dvfs_lock_status;
	int dvfs_boost_mode;
	int dvfs_freq;
	int dvfs_old_stauts;
	bool stay_awake;

#elif defined(WACOM_BOOSTER)
	bool dvfs_lock_status;
	struct delayed_work dvfs_off_work;
	struct delayed_work dvfs_chg_work;
	struct mutex dvfs_lock;
	struct pm_qos_request cpu_qos;
	struct pm_qos_request mif_qos;
	struct pm_qos_request int_qos;
	unsigned char boost_level;
#endif
	bool pwr_flag;
	bool power_enable;
	bool boot_mode;
	bool query_status;
#ifdef LCD_FREQ_SYNC
	int lcd_freq;
	bool lcd_freq_wait;
	bool use_lcd_freq_sync;
	struct work_struct lcd_freq_work;
	struct delayed_work lcd_freq_done_work;
	struct mutex freq_write_lock;
#endif
#ifdef WACOM_USE_SOFTKEY_BLOCK
	bool block_softkey;
	struct delayed_work softkey_block_work;
#endif
	struct work_struct update_work;
	struct fw_update_info update_info;
	bool enabled;
};

#endif /* _LINUX_WACOM_H */
