#ifndef _CYPRESS_TOUCHKEY_H
#define _CYPRESS_TOUCHKEY_H

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

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include <linux/i2c/touchkey_hl.h>

/* Touchkey Register */
#define CYPRESS_REG_STATUS	0x00
#define CYPRESS_REG_FW_VER	0X01
#define CYPRESS_REG_MD_VER	0X02
#define CYPRESS_REG_COMMAND	0X03
#define CYPRESS_REG_THRESHOLD	0X04
#define CYPRESS_REG_AUTOCAL	0X05
#define CYPRESS_REG_IDAC	0X06
#define CYPRESS_REG_DIFF	0X0A
#define CYPRESS_REG_RAW		0X0E
#define CYPRESS_REG_BASE	0X12
#define CYPRESS_REG_CRC		0X16
#define CYPRESS_REG_DETECTION		0x18
#define CYPRESS_REG_DETECTION_FLAG	0x1B

#define KEYCODE_REG			0x00

#define TK_BIT_PRESS_EV		0x08
#define TK_BIT_KEYCODE		0x07

#define TK_BIT_AUTOCAL		0x80
#define TK_BIT_GLOVE		0x40
#define TK_BIT_TA_ON		0x10
#define TK_BIT_FW_ID_55		0x20
#define TK_BIT_FW_ID_65		0x04

#define TK_BIT_DETECTION_CONFIRM	0xEE

#define TK_CMD_LED_ON		0x10
#define TK_CMD_LED_OFF		0x20

#define TK_CMD_DUAL_DETECTION	0x01

#define I2C_M_WR 0		/* for i2c */

#define TK_UPDATE_DOWN		1
#define TK_UPDATE_FAIL		-1
#define TK_UPDATE_PASS		0

/* Flip cover*/
#define TKEY_FLIP_MODE 

#ifdef TKEY_FLIP_MODE
#define TK_BIT_FLIP	0x08
#endif

#ifdef TK_USE_LDO_CONTROL
/* LDO Regulator */
#define	TKEY_I2C_REGULATOR	"8941_lvs3"
	
/* LDO Regulator */
#define	TKEY_LED_REGULATOR	"8941_l13"

/* LED LDO Type*/
#define LED_LDO_WITH_REGULATOR
#endif

/* Autocalibration */
#define TK_HAS_AUTOCAL

/* Generalized SMBus access */
#define TK_USE_GENERAL_SMBUS

/* Boot-up Firmware Update */
#define TK_HAS_FIRMWARE_UPDATE
#define TK_UPDATABLE_BD_ID	0

#if defined(CONFIG_SEC_S_PROJECT)
#define CYPRESS_CRC_CHECK
#define TK_USE_RECENT
#define FW_PATH "tkey/s_cypress_tkey.fw"
#define TOUCHKEY_BOOSTER
#else
#define FW_PATH "tkey/fresco_n_cypress_tkey.fw"
#endif

#define TKEY_MODULE07_HWID 8
#define TKEY_FW_PATH "/sdcard/cypress/fw.bin"

#define  TOUCHKEY_FW_UPDATEABLE_HW_REV  11

#if defined(CONFIG_SEC_S_PROJECT)
#define CYPRESS_RECENT_BACK_REPORT_FW_VER	0x0D
#elif defined(CONFIG_SEC_FRESCO_PROJECT)
#define CYPRESS_RECENT_BACK_REPORT_FW_VER	0x0B
#else
#define CYPRESS_RECENT_BACK_REPORT_FW_VER	0xFF
#endif

#ifdef TOUCHKEY_BOOSTER
#include <linux/cpufreq.h>

#define TKEY_BOOSTER_ON_TIME	500
#define TKEY_BOOSTER_OFF_TIME	500
#define TKEY_BOOSTER_CHG_TIME	130

enum BOOST_LEVEL {
	TKEY_BOOSTER_DISABLE = 0,
	TKEY_BOOSTER_LEVEL1,
	TKEY_BOOSTER_LEVEL2,
};
#endif

/* #define TK_USE_OPEN_DWORK */
#ifdef TK_USE_OPEN_DWORK
#define	TK_OPEN_DWORK_TIME	10
#endif
#ifdef CONFIG_GLOVE_TOUCH
#define	TK_GLOVE_DWORK_TIME	300
#endif

//#define TKEY_GRIP_MODE

enum {
	FW_NONE = 0,
	FW_BUILT_IN,
	FW_HEADER,
	FW_IN_SDCARD,
	FW_EX_SDCARD,
};

/* header ver 1 */
struct fw_image {
	u8 hdr_ver;
	u8 hdr_len;
	u16 first_fw_ver;
	u16 second_fw_ver;
	u16 third_ver;
	u32 fw_len;
	u16 checksum;
	u16 alignment_dummy;
	u8 data[0];
} __attribute__ ((packed));

enum {
	MODE_NORMAL = 0,
	MODE_GRIP,
};

#define TKEY_CMD_GRIP 0x80

/*Parameters for i2c driver*/
struct touchkey_i2c {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct completion init_done;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	struct mutex lock;
	struct mutex i2c_lock;
	struct mutex irq_lock;
	struct wake_lock fw_wakelock;
	struct device	*dev;
	int irq;
	int md_ver_ic; /*module ver*/
	int fw_ver_ic;
	int firmware_id;
	struct touchkey_platform_data *pdata;
	char *name;
	int (*power)(int on);
	int update_status;
	bool enabled;
#ifdef TOUCHKEY_BOOSTER
	bool dvfs_lock_status;
	struct delayed_work work_dvfs_off;
	struct delayed_work work_dvfs_chg;
	struct mutex dvfs_lock;
	int dvfs_old_stauts;
	int dvfs_boost_mode;
	int dvfs_freq;
#endif
#ifdef TK_USE_OPEN_DWORK
	struct delayed_work open_work;
#endif
#ifdef CONFIG_GLOVE_TOUCH
	struct work_struct glove_change_work;
	int ic_mode;
	bool tsk_cmd_glove;
	bool tsk_enable_glove_mode;
	struct mutex tsk_glove_lock;
#endif
#ifdef TK_INFORM_CHARGER
	struct touchkey_callbacks callbacks;
	bool charging_mode;
#endif
#ifdef TKEY_FLIP_MODE
	bool enabled_flip;
#endif
#ifdef TKEY_GRIP_MODE
	bool grip_mode;
	u8 ic_mode;
	bool pwr_flag;
	struct mutex grip_mode_lock;
#endif
	bool status_update;
	struct work_struct update_work;
	struct workqueue_struct *fw_wq;
	u8 fw_path;
	const struct firmware *firm_data;
	struct fw_image *fw_img;
	bool do_checksum;
};

extern struct class *sec_class;

extern unsigned int system_rev;
#endif /* _CYPRESS_TOUCHKEY_H */
