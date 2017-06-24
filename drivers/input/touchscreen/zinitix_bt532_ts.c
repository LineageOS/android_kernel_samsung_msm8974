/*
 *
 * Zinitix bt532 touchscreen driver
 *
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 */


#define TSP_VERBOSE_DEBUG
#define SEC_FACTORY_TEST
#if defined(CONFIG_SEC_MEGA23G_COMMON) || defined(CONFIG_SEC_MEGA2LTE_COMMON)
#define SUPPORTED_TOUCH_KEY 1
#define ZINITIX_NEW_TA_COVER_REGISTER_0x11F
#define USE_ACTIVE_REPORT_RATE
#else
#define SUPPORTED_TOUCH_KEY 0
#endif

#define TSP_BOOSTER

#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#if defined(CONFIG_PM_RUNTIME)
#include <linux/pm_runtime.h>
#endif
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/regulator/consumer.h>

#include <zinitix_bt532_ts.h>
#include <linux/input/mt.h>
#include <linux/regulator/machine.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <asm/io.h>
#include <linux/power_supply.h>

#if defined(CONFIG_USB_SWITCH_TSU6721) || defined(CONFIG_USB_SWITCH_RT8973) || defined(CONFIG_SM5502_MUIC)
#include <linux/i2c/tsu6721.h>
#endif
#ifdef CONFIG_MACH_PXA_SAMSUNG
#include <linux/sec-common.h>
#endif
/* added header file */

#if defined(CONFIG_SEC_MEGA23G_COMMON) || defined(CONFIG_SEC_MEGA2LTE_COMMON)
#include "zinitix_touch_zxt_firmware_ZI012407.h"
#elif defined(CONFIG_MACH_DEGASLTE_SBM)
#include "zinitix_touch_zxt_firmware_ZI002516.h"
#else
#include "zinitix_touch_zxt_firmware_ZI002517.h"
#endif

#ifdef TSP_BOOSTER
#include <linux/cpufreq.h>
#define DVFS_STAGE_DUAL		2
#define DVFS_STAGE_SINGLE		1
#define DVFS_STAGE_NONE		0
#define TOUCH_BOOSTER_OFF_TIME	300
#define TOUCH_BOOSTER_CHG_TIME	200
#endif

#define NOT_SUPPORTED_TOUCH_DUMMY_KEY
#define SUPPORTED_PALM_TOUCH

extern char *saved_command_line;

#define ZINITIX_DEBUG			0

#ifdef SUPPORTED_PALM_TOUCH
#define TOUCH_POINT_MODE			2
#else
#define TOUCH_POINT_MODE		0
#endif

#define MAX_SUPPORTED_FINGER_NUM	5 /* max 10 */

#ifdef SUPPORTED_TOUCH_KEY
#ifdef NOT_SUPPORTED_TOUCH_DUMMY_KEY
#define MAX_SUPPORTED_BUTTON_NUM	2 /* max 8 */
#define SUPPORTED_BUTTON_NUM		2
#else
#define MAX_SUPPORTED_BUTTON_NUM	2 /* max 8 */
#define SUPPORTED_BUTTON_NUM		2
#endif
#endif
#define TSP_HW_ID_INDEX_NULL	3

/* Upgrade Method*/
#define TOUCH_ONESHOT_UPGRADE		1
/* if you use isp mode, you must add i2c device :
name = "zinitix_isp" , addr 0x50*/
/* resolution offset */
#define ABS_PT_OFFSET			(-1)

#define TOUCH_FORCE_UPGRADE		1
#define USE_CHECKSUM			1
#if defined(CONFIG_SEC_MEGA23G_COMMON) || defined(CONFIG_SEC_MEGA2LTE_COMMON)
#define CHECK_HWID			1
#else
#define CHECK_HWID			0
#endif

#define CHIP_OFF_DELAY			50 /*ms*/
#define CHIP_ON_DELAY			15 /*ms*/
#define FIRMWARE_ON_DELAY		40 /*ms*/

#define DELAY_FOR_SIGNAL_DELAY		30 /*us*/
#define DELAY_FOR_TRANSCATION		50
#define DELAY_FOR_POST_TRANSCATION	10

/* PMIC Regulator based supply to TSP */
#define TSP_REGULATOR_SUPPLY		1
/* gpio controlled LDO based supply to TSP */
#define TSP_LDO_SUPPLY			0

enum power_control {
	POWER_OFF,
	POWER_ON,
	POWER_ON_SEQUENCE,
};

/* Key Enum */
enum key_event {
	ICON_BUTTON_UNCHANGE,
	ICON_BUTTON_DOWN,
	ICON_BUTTON_UP,
};
/* ESD Protection */
/*second : if 0, no use. if you have to use, 3 is recommended*/
#define ESD_TIMER_INTERVAL		1
#define SCAN_RATE_HZ			100
#define CHECK_ESD_TIMER			3

 /*Test Mode (Monitoring Raw Data) */
#if defined(CONFIG_SEC_MEGA23G_COMMON) || defined(CONFIG_SEC_MEGA2LTE_COMMON)
#define SEC_DND_SHIFT_VALUE		2
#define SEC_DND_N_COUNT			10
#define SEC_DND_U_COUNT			2
#define SEC_DND_FREQUENCY		116

#define SEC_PDND_N_COUNT		16
#define SEC_PDND_U_COUNT		20
#define SEC_PDND_FREQUENCY		119
#else
#define SEC_DND_SHIFT_VALUE		3
#define SEC_DND_N_COUNT			10
#define SEC_DND_U_COUNT			2
#define SEC_DND_FREQUENCY		99 /* 200khz */

#define SEC_PDND_N_COUNT		16
#define SEC_PDND_U_COUNT		14
#define SEC_PDND_FREQUENCY		66
#endif

#define MAX_RAW_DATA_SZ			576 /* 32x18 */
#define MAX_TRAW_DATA_SZ	\
	(MAX_RAW_DATA_SZ + 4*MAX_SUPPORTED_FINGER_NUM + 2)
/* preriod raw data interval */

#define RAWDATA_DELAY_FOR_HOST		100

struct raw_ioctl {
	int sz;
	u8 *buf;
};

struct reg_ioctl {
	int addr;
	int *val;
};

#define TOUCH_SEC_MODE			48
#define TOUCH_REF_MODE			10
#define TOUCH_NORMAL_MODE		5
#define TOUCH_DELTA_MODE		3
#define TOUCH_DND_MODE			6
#define TOUCH_PDND_MODE			11

/*  Other Things */
#define INIT_RETRY_CNT			1
#define I2C_SUCCESS			0
#define I2C_FAIL			1

/*---------------------------------------------------------------------*/

/* Register Map*/
#define BT532_SWRESET_CMD		0x0000
#define BT532_WAKEUP_CMD		0x0001

#define BT532_IDLE_CMD			0x0004
#define BT532_SLEEP_CMD			0x0005

#define BT532_CLEAR_INT_STATUS_CMD	0x0003
#define BT532_CALIBRATE_CMD		0x0006
#define BT532_SAVE_STATUS_CMD		0x0007
#define BT532_SAVE_CALIBRATION_CMD	0x0008
#define BT532_RECALL_FACTORY_CMD	0x000f

#define BT532_THRESHOLD			0x0020

#define BT532_DEBUG_REG			0x0115 /* 0~7 */

#define BT532_TOUCH_MODE		0x0010
#define BT532_CHIP_REVISION		0x0011
#define BT532_FIRMWARE_VERSION		0x0012

#define BT532_MINOR_FW_VERSION		0x0121

#define BT532_VENDOR_ID			0x001C
#define BT532_HW_ID			0x0014

#define BT532_DATA_VERSION_REG		0x0013
#define BT532_SUPPORTED_FINGER_NUM	0x0015
#define BT532_EEPROM_INFO		0x0018
#define BT532_INITIAL_TOUCH_MODE	0x0019

#define BT532_TOTAL_NUMBER_OF_X		0x0060
#define BT532_TOTAL_NUMBER_OF_Y		0x0061

#define BT532_DELAY_RAW_FOR_HOST	0x007f

#define BT532_BUTTON_SUPPORTED_NUM	0x00B0
#define BT532_BUTTON_SENSITIVITY	0x00B2
#define BT532_DUMMY_BUTTON_SENSITIVITY	0X00C8

#define BT532_X_RESOLUTION		0x00C0
#define BT532_Y_RESOLUTION		0x00C1

#define BT532_POINT_STATUS_REG		0x0080
#define BT532_ICON_STATUS_REG		0x00AA

#define BT532_DND_SHIFT_VALUE	0x012B
#define BT532_AFE_FREQUENCY		0x0100
#define BT532_DND_N_COUNT		0x0122
#define BT532_DND_U_COUNT		0x0135

#define BT532_RAWDATA_REG		0x0200

#define BT532_EEPROM_INFO_REG		0x0018

#define BT532_INT_ENABLE_FLAG		0x00f0
#define BT532_PERIODICAL_INTERRUPT_INTERVAL	0x00f1

#define BT532_BTN_WIDTH			0x016d

#define BT532_CHECKSUM_RESULT		0x012c

#define BT532_INIT_FLASH		0x01d0
#define BT532_WRITE_FLASH		0x01d1
#define BT532_READ_FLASH		0x01d2

#define ZINITIX_INTERNAL_FLAG_03		0x011f


/* Interrupt & status register flag bit
-------------------------------------------------
*/
#define BIT_PT_CNT_CHANGE	0
#define BIT_DOWN		1
#define BIT_MOVE		2
#define BIT_UP			3
#define BIT_PALM		4
#define BIT_PALM_REJECT		5
#define RESERVED_0		6
#define RESERVED_1		7
#define BIT_WEIGHT_CHANGE	8
#define BIT_PT_NO_CHANGE	9
#define BIT_REJECT		10
#define BIT_PT_EXIST		11
#define RESERVED_2		12
#define BIT_MUST_ZERO		13
#define BIT_DEBUG		14
#define BIT_ICON_EVENT		15

/* button */
#define BIT_O_ICON0_DOWN	0
#define BIT_O_ICON1_DOWN	1
#define BIT_O_ICON2_DOWN	2
#define BIT_O_ICON3_DOWN	3
#define BIT_O_ICON4_DOWN	4
#define BIT_O_ICON5_DOWN	5
#define BIT_O_ICON6_DOWN	6
#define BIT_O_ICON7_DOWN	7

#define BIT_O_ICON0_UP		8
#define BIT_O_ICON1_UP		9
#define BIT_O_ICON2_UP		10
#define BIT_O_ICON3_UP		11
#define BIT_O_ICON4_UP		12
#define BIT_O_ICON5_UP		13
#define BIT_O_ICON6_UP		14
#define BIT_O_ICON7_UP		15


#define SUB_BIT_EXIST		0
#define SUB_BIT_DOWN		1
#define SUB_BIT_MOVE		2
#define SUB_BIT_UP		3
#define SUB_BIT_UPDATE		4
#define SUB_BIT_WAIT		5


#define zinitix_bit_set(val, n)		((val) &= ~(1<<(n)), (val) |= (1<<(n)))
#define zinitix_bit_clr(val, n)		((val) &= ~(1<<(n)))
#define zinitix_bit_test(val, n)	((val) & (1<<(n)))
#define zinitix_swap_v(a, b, t)		((t) = (a), (a) = (b), (b) = (t))
#define zinitix_swap_16(s)		(((((s) & 0xff) << 8) | (((s) >> 8) & 0xff)))

#ifdef CONFIG_SAMSUNG_LPM_MODE
extern int poweroff_charging;
#endif

/* end header file */

#ifdef SEC_FACTORY_TEST
/* Touch Screen */
#define TSP_CMD_STR_LEN			32
#define TSP_CMD_RESULT_STR_LEN		512
#define TSP_CMD_PARAM_NUM		8
#define TSP_CMD_Y_NUM			18
#define TSP_CMD_X_NUM			30
#define TSP_CMD_NODE_NUM		(TSP_CMD_Y_NUM * TSP_CMD_X_NUM)
#define tostring(x) #x

struct tsp_factory_info {
	struct list_head cmd_list_head;
	char cmd[TSP_CMD_STR_LEN];
	char cmd_param[TSP_CMD_PARAM_NUM];
	char cmd_result[TSP_CMD_RESULT_STR_LEN];
	char cmd_buff[TSP_CMD_RESULT_STR_LEN];
	struct mutex cmd_lock;
	bool cmd_is_running;
	u8 cmd_state;
};

struct tsp_raw_data {
	u16 ref_data[TSP_CMD_NODE_NUM];
	u16 pref_data[TSP_CMD_NODE_NUM];
	/*s16 scantime_data[TSP_CMD_NODE_NUM]; */
	s16 delta_data[TSP_CMD_NODE_NUM];
};

enum {
	WAITING = 0,
	RUNNING,
	OK,
	FAIL,
	NOT_APPLICABLE,
};

struct tsp_cmd {
	struct list_head list;
	const char *cmd_name;
	void (*cmd_func)(void *device_data);
};

static void fw_update(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_threshold(void *device_data);
static void module_off_master(void *device_data);
static void module_on_master(void *device_data);
static void module_off_slave(void *device_data);
static void module_on_slave(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void not_support_cmd(void *device_data);

/* Vendor dependant command */
static void run_reference_read(void *device_data);
static void run_reference_PDiff(void *device_data);
static void get_reference(void *device_data);
static void run_preference_read(void *device_data);
static void get_preference(void *device_data);
/*
static void run_scantime_read(void *device_data);
static void get_scantime(void *device_data);
*/
static void run_delta_read(void *device_data);
static void get_delta(void *device_data);
static void clear_cover_mode(void *device_data);
static void get_module_vendor(void *device_data);
static void get_config_ver(void *device_data);
#if defined(USE_ACTIVE_REPORT_RATE)
static void report_rate(void *device_data);
#endif
#define TSP_CMD(name, func) .cmd_name = name, .cmd_func = func

#if !defined(CONFIG_SEC_DEGAS_PROJECT)
u32 config_version;
#else
#define CONFIG_DATE "0603"
#endif

static struct tsp_cmd tsp_cmds[] = {
	{TSP_CMD("fw_update", fw_update),},
	{TSP_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{TSP_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{TSP_CMD("get_threshold", get_threshold),},
	{TSP_CMD("module_off_master", module_off_master),},
	{TSP_CMD("module_on_master", module_on_master),},
	{TSP_CMD("module_off_slave", module_off_slave),},
	{TSP_CMD("module_on_slave", module_on_slave),},
	{TSP_CMD("get_module_vendor", get_module_vendor),},
	{TSP_CMD("get_chip_vendor", get_chip_vendor),},
	{TSP_CMD("get_chip_name", get_chip_name),},
	{TSP_CMD("get_x_num", get_x_num),},
	{TSP_CMD("get_y_num", get_y_num),},

	/* vendor dependant command */
	{TSP_CMD("run_reference_read", run_reference_read),},
	{TSP_CMD("run_reference_diff", run_reference_PDiff),},
	{TSP_CMD("get_reference", get_reference),},
	{TSP_CMD("run_dnd_read", run_preference_read),},
	{TSP_CMD("get_dnd", get_preference),},
/*
	{TSP_CMD("run_scantime_read", run_scantime_read),},
	{TSP_CMD("get_scantime", get_scantime),},
*/
	{TSP_CMD("run_delta_read", run_delta_read),},
	{TSP_CMD("get_delta", get_delta),},
	{TSP_CMD("get_config_ver", get_config_ver),},
	{TSP_CMD("clear_cover_mode", clear_cover_mode),},
#if defined(USE_ACTIVE_REPORT_RATE)	
	{TSP_CMD("report_rate", report_rate),},	
#endif
	{TSP_CMD("not_support_cmd", not_support_cmd),},
};
#if 0
#ifdef SUPPORTED_TOUCH_KEY
/* Touch Key */
static ssize_t touchkey_threshold(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t touch_sensitivity(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t touchkey_back(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t touchkey_menu(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t autocal_stat(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t touchkey_raw_back(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t touchkey_raw_menu(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t touchkey_idac_back(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t touchkey_idac_menu(struct device *dev,
		struct device_attribute *attr, char *buf);
#endif
#endif
#endif

#define TSP_NORMAL_EVENT_MSG 1
static int m_ts_debug_mode = ZINITIX_DEBUG;
static bool ta_connected =0;
//void (*tsp_charger_status_cb)(int);

#define COVER_OPEN 0
#define COVER_CLOSED 3
static int g_cover_state;
static u16 g_internal_flag_03 = 1;
static u16 m_optional_mode = 0;
static u16 m_prev_optional_mode = 0;

#if ESD_TIMER_INTERVAL
static struct workqueue_struct *esd_tmr_workqueue;
#endif

struct coord {
	u16	x;
	u16	y;
	u8	width;
	u8	sub_status;
#if (TOUCH_POINT_MODE == 2)
	u8	minor_width;
	u8	angle;
#endif
};

struct point_info {
	u16	status;
#if (TOUCH_POINT_MODE == 1)
	u16	event_flag;
#else
	u8	finger_cnt;
	u8	time_stamp;
#endif
	struct coord coord[MAX_SUPPORTED_FINGER_NUM];
};

#define TOUCH_V_FLIP	0x01
#define TOUCH_H_FLIP	0x02
#define TOUCH_XY_SWAP	0x04

struct capa_info {
	u16	vendor_id;
	u16	ic_revision;
	u16	fw_version;
	u16	fw_minor_version;
	u16	reg_data_version;
	u16	threshold;
	u16	key_threshold;
	u16	dummy_threshold;
	u32	ic_fw_size;
	u32	MaxX;
	u32	MaxY;
	u32	MinX;
	u32	MinY;
	u8	gesture_support;
	u16	multi_fingers;
	u16	button_num;
	u16	ic_int_mask;
	u16	x_node_num;
	u16	y_node_num;
	u16	total_node_num;
	u16	hw_id;
	u16	afe_frequency;
	u16	shift_value;
	u16	N_cnt;
	u16	u_cnt;
};

enum work_state {
	NOTHING = 0,
	NORMAL,
	ESD_TIMER,
	SUSPEND,
	RESUME,
	UPGRADE,
	REMOVE,
	SET_MODE,
	HW_CALIBRAION,
	RAW_DATA,
	PROBE,
};

enum {
	BUILT_IN = 0,
	UMS,
	REQ_FW,
};

struct bt532_ts_info {
	struct i2c_client		*client;
	struct input_dev		*input_dev;
	struct bt532_ts_platform_data	*pdata;
	char				phys[32];
	/*struct task_struct		*task;*/
	/*wait_queue_head_t		wait;*/
	/*struct semaphore		update_lock;*/
	/*u32				i2c_dev_addr;*/
	struct capa_info		cap_info;
	struct point_info		touch_info;
	struct point_info		reported_touch_info;
	u16				icon_event_reg;
	u16				prev_icon_event;
	/*u16				event_type;*/
	int				irq;
	u8				button[MAX_SUPPORTED_BUTTON_NUM];
	u8				work_state;
	struct semaphore		work_lock;
	/*u16				debug_reg[8];*/
#if ESD_TIMER_INTERVAL
	struct work_struct		tmr_work;
	struct timer_list		esd_timeout_tmr;
	struct timer_list		*p_esd_timeout_tmr;
	spinlock_t			lock;
#endif
	struct semaphore		raw_data_lock;
	u16				touch_mode;
	s16				cur_data[MAX_TRAW_DATA_SZ];
	u8				update;
#ifdef SEC_FACTORY_TEST
	struct tsp_factory_info		*factory_info;
	struct tsp_raw_data		*raw_data;
#endif

	u16 dnd_data[MAX_RAW_DATA_SZ];
	s16 ref_data[MAX_RAW_DATA_SZ];

	s16 ref_scale_factor;
	s16 ref_btn_option;
	s16 hdiff_max_x;
	s16 hdiff_max_y;
	s16 hdiff_max_val;
	s16 hdiff_min_x;
	s16 hdiff_min_y;
	s16 hdiff_min_val;

	s16 vdiff_max_x;
	s16 vdiff_max_y;
	s16 vdiff_max_val;
	s16 vdiff_min_x;
	s16 vdiff_min_y;
	s16 vdiff_min_val;

	struct regulator *vddo_vreg;
	u8 tsp_type_hw;
	struct regulator *vdd_en;
	bool device_enabled;

#ifdef TSP_BOOSTER
	u8 touch_pressed_num;
	bool dvfs_lock_status;
	struct delayed_work work_dvfs_off;
	struct delayed_work work_dvfs_chg;
	struct mutex dvfs_lock;
	int dvfs_old_status;
	unsigned char boost_level;
	int dvfs_freq;
#endif
};
/* Dummy touchkey code */
#define KEY_DUMMY_HOME1		249
#define KEY_DUMMY_HOME2		250
#define KEY_DUMMY_MENU		251
#define KEY_DUMMY_HOME		252
#define KEY_DUMMY_BACK		253
/*<= you must set key button mapping*/
#ifdef NOT_SUPPORTED_TOUCH_DUMMY_KEY
u32 BUTTON_MAPPING_KEY[MAX_SUPPORTED_BUTTON_NUM] = {
	KEY_RECENT, KEY_BACK};
#else
u32 BUTTON_MAPPING_KEY[MAX_SUPPORTED_BUTTON_NUM] = {
	KEY_DUMMY_MENU, KEY_RECENT,// KEY_DUMMY_HOME1,
	/*KEY_DUMMY_HOME2,*/ KEY_BACK, KEY_DUMMY_BACK};
#endif

/* VASTA SPEC */
const u16 dnd_v_diff_00[29][18]={
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500,	3500},
	{	0,	0,	0,	10000,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	10000,	0,	0,	0}
};

const u16 dnd_h_diff_00[29][17]={
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000},
	{3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000,	3000}
};

/* define i2c sub functions*/
static inline s32 read_data(struct i2c_client *client,
	u16 reg, u8 *values, u16 length)
{
	s32 ret;
	int count = 0;
retry:
	/* select register*/
	ret = i2c_master_send(client , (u8 *)&reg , 2);
	if (ret < 0) {
		mdelay(1);

		if (++count < 8)
			goto retry;

		return ret;
	}
	/* for setup tx transaction. */
	udelay(DELAY_FOR_TRANSCATION);
	ret = i2c_master_recv(client , values , length);
	if (ret < 0)
		return ret;

	udelay(DELAY_FOR_POST_TRANSCATION);
	return length;
}

static inline s32 write_data(struct i2c_client *client,
	u16 reg, u8 *values, u16 length)
{
	s32 ret;
	int count = 0;
	u8 pkt[10]; /* max packet */
	pkt[0] = (reg) & 0xff; /* reg addr */
	pkt[1] = (reg >> 8)&0xff;
	memcpy((u8 *)&pkt[2], values, length);

retry:
	ret = i2c_master_send(client , pkt , length + 2);
	if (ret < 0) {
		mdelay(1);

		if (++count < 8)
			goto retry;

		return ret;
	}

	udelay(DELAY_FOR_POST_TRANSCATION);
	return length;
}

static inline s32 write_reg(struct i2c_client *client, u16 reg, u16 value)
{
	if (write_data(client, reg, (u8 *)&value, 2) < 0)
		return I2C_FAIL;

	return I2C_SUCCESS;
}

static inline s32 write_cmd(struct i2c_client *client, u16 reg)
{
	s32 ret;
	int count = 0;

retry:
	ret = i2c_master_send(client , (u8 *)&reg , 2);
	if (ret < 0) {
		mdelay(1);

		if (++count < 8)
			goto retry;

		return ret;
	}

	udelay(DELAY_FOR_POST_TRANSCATION);
	return I2C_SUCCESS;
}

static inline s32 read_raw_data(struct i2c_client *client,
		u16 reg, u8 *values, u16 length)
{
	s32 ret;
	int count = 0;

retry:
	/* select register */
	ret = i2c_master_send(client , (u8 *)&reg , 2);
	if (ret < 0) {
		mdelay(1);

		if (++count < 8)
			goto retry;

		return ret;
	}

	/* for setup tx transaction. */
	udelay(200);

	ret = i2c_master_recv(client , values , length);
	if (ret < 0)
		return ret;

	udelay(DELAY_FOR_POST_TRANSCATION);
	return length;
}

static inline s32 read_firmware_data(struct i2c_client *client,
	u16 addr, u8 *values, u16 length)
{
	s32 ret;
	/* select register*/

	ret = i2c_master_send(client , (u8 *)&addr , 2);
	if (ret < 0)
		return ret;

	/* for setup tx transaction. */
	mdelay(1);

	ret = i2c_master_recv(client , values , length);
	if (ret < 0)
		return ret;

	udelay(DELAY_FOR_POST_TRANSCATION);
	return length;
}

static bool bt532_power_control(struct bt532_ts_info *info, u8 ctl);

static bool init_touch(struct bt532_ts_info *info);
static bool mini_init_touch(struct bt532_ts_info *info);
static void clear_report_data(struct bt532_ts_info *info);
#if ESD_TIMER_INTERVAL
static void esd_timer_start(u16 sec, struct bt532_ts_info *info);
static void esd_timer_stop(struct bt532_ts_info *info);
static void esd_timer_init(struct bt532_ts_info *info);
static void esd_timeout_handler(unsigned long data);
#endif

static long ts_misc_fops_ioctl(struct file *filp, unsigned int cmd,
								unsigned long arg);
static int ts_misc_fops_open(struct inode *inode, struct file *filp);
static int ts_misc_fops_close(struct inode *inode, struct file *filp);

static const struct file_operations ts_misc_fops = {
	.owner = THIS_MODULE,
	.open = ts_misc_fops_open,
	.release = ts_misc_fops_close,
	.unlocked_ioctl = ts_misc_fops_ioctl,
};

static struct miscdevice touch_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "zinitix_touch_misc",
	.fops = &ts_misc_fops,
};

#define TOUCH_IOCTL_BASE	0xbc
#define TOUCH_IOCTL_GET_DEBUGMSG_STATE		_IOW(TOUCH_IOCTL_BASE, 0, int)
#define TOUCH_IOCTL_SET_DEBUGMSG_STATE		_IOW(TOUCH_IOCTL_BASE, 1, int)
#define TOUCH_IOCTL_GET_CHIP_REVISION		_IOW(TOUCH_IOCTL_BASE, 2, int)
#define TOUCH_IOCTL_GET_FW_VERSION			_IOW(TOUCH_IOCTL_BASE, 3, int)
#define TOUCH_IOCTL_GET_REG_DATA_VERSION	_IOW(TOUCH_IOCTL_BASE, 4, int)
#define TOUCH_IOCTL_VARIFY_UPGRADE_SIZE		_IOW(TOUCH_IOCTL_BASE, 5, int)
#define TOUCH_IOCTL_VARIFY_UPGRADE_DATA		_IOW(TOUCH_IOCTL_BASE, 6, int)
#define TOUCH_IOCTL_START_UPGRADE			_IOW(TOUCH_IOCTL_BASE, 7, int)
#define TOUCH_IOCTL_GET_X_NODE_NUM			_IOW(TOUCH_IOCTL_BASE, 8, int)
#define TOUCH_IOCTL_GET_Y_NODE_NUM			_IOW(TOUCH_IOCTL_BASE, 9, int)
#define TOUCH_IOCTL_GET_TOTAL_NODE_NUM		_IOW(TOUCH_IOCTL_BASE, 10, int)
#define TOUCH_IOCTL_SET_RAW_DATA_MODE		_IOW(TOUCH_IOCTL_BASE, 11, int)
#define TOUCH_IOCTL_GET_RAW_DATA			_IOW(TOUCH_IOCTL_BASE, 12, int)
#define TOUCH_IOCTL_GET_X_RESOLUTION		_IOW(TOUCH_IOCTL_BASE, 13, int)
#define TOUCH_IOCTL_GET_Y_RESOLUTION		_IOW(TOUCH_IOCTL_BASE, 14, int)
#define TOUCH_IOCTL_HW_CALIBRAION			_IOW(TOUCH_IOCTL_BASE, 15, int)
#define TOUCH_IOCTL_GET_REG					_IOW(TOUCH_IOCTL_BASE, 16, int)
#define TOUCH_IOCTL_SET_REG					_IOW(TOUCH_IOCTL_BASE, 17, int)
#define TOUCH_IOCTL_SEND_SAVE_STATUS		_IOW(TOUCH_IOCTL_BASE, 18, int)
#define TOUCH_IOCTL_DONOT_TOUCH_EVENT		_IOW(TOUCH_IOCTL_BASE, 19, int)

struct bt532_ts_info *misc_info;

static void bt532_firmware_check(struct bt532_ts_platform_data   *pdata)
{
#if defined(CONFIG_SEC_DEGAS_PROJECT)
	misc_info->tsp_type_hw = TSP_HW_ID_INDEX_NULL;			//HW_ID 3

	gpio_tlmm_config(GPIO_CFG(pdata->tsp_vendor1, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(pdata->tsp_vendor2, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	printk("gpio_get_value(TSP_ID_1:gpio%d)=%d, gpio_get_value(TSP_ID_2:gpio%d)=%d\n",\
		pdata->tsp_vendor1, gpio_get_value(pdata->tsp_vendor1), pdata->tsp_vendor2, gpio_get_value(pdata->tsp_vendor2));

	if(!gpio_get_value(pdata->tsp_vendor1))
		zinitix_bit_clr(misc_info->tsp_type_hw,0);		//HW_ID 2
	if(!gpio_get_value(pdata->tsp_vendor2))
		zinitix_bit_clr(misc_info->tsp_type_hw,1);		//HW_ID 0

	zinitix_printk("%s %d\n",__func__,misc_info->tsp_type_hw);

#else
	return;
#endif
}

static void bt532_set_optional_mode(struct bt532_ts_info *info, bool force)
{
	u16	reg_val;

	if(m_prev_optional_mode == m_optional_mode && !force)
		return;
	reg_val = g_internal_flag_03 | m_optional_mode;
	if(write_reg(info->client, ZINITIX_INTERNAL_FLAG_03, reg_val)==I2C_SUCCESS){
		m_prev_optional_mode = m_optional_mode;
	}
}

static bool get_raw_data(struct bt532_ts_info *info, u8 *buff, int skip_cnt)
{
	struct i2c_client *client = info->client;
	struct bt532_ts_platform_data *pdata = info->pdata;
	u32 total_node = info->cap_info.total_node_num;
	u32 sz;
	int i;

	disable_irq(info->irq);

	down(&info->work_lock);
	if (info->work_state != NOTHING) {
		printk(KERN_INFO "other process occupied.. (%d)\n",
			info->work_state);
		enable_irq(info->irq);
		up(&info->work_lock);
		return false;
		}

	info->work_state = RAW_DATA;

	for (i = 0; i < skip_cnt; i++) {
		while (gpio_get_value(pdata->gpio_int))
			msleep(1);

		write_cmd(client, BT532_CLEAR_INT_STATUS_CMD);
		msleep(1);
	}

	zinitix_debug_msg("read raw data\r\n");
	sz = total_node*2;

	while (gpio_get_value(pdata->gpio_int))
		msleep(1);

	if (read_raw_data(client, BT532_RAWDATA_REG, (char *)buff, sz) < 0) {
		zinitix_printk("error : read zinitix tc raw data\n");
		info->work_state = NOTHING;
		enable_irq(info->irq);
		up(&info->work_lock);
		return false;
	}

	write_cmd(client, BT532_CLEAR_INT_STATUS_CMD);
	info->work_state = NOTHING;
	enable_irq(info->irq);
	up(&info->work_lock);

	return true;
}

static bool ts_get_raw_data(struct bt532_ts_info *info)
{
	struct i2c_client *client = info->client;
	u32 total_node = info->cap_info.total_node_num;
	u32 sz;

	if (down_trylock(&info->raw_data_lock)) {
		dev_err(&client->dev, "Failed to occupy sema\n");
		info->touch_info.status = 0;
		return true;
	}

	sz = total_node * 2 + sizeof(struct point_info);

	if (read_raw_data(info->client, BT532_RAWDATA_REG,
			(char *)info->cur_data, sz) < 0) {
		dev_err(&client->dev, "Failed to read raw data\n");
		up(&info->raw_data_lock);
		return false;
	}

	info->update = 1;
	memcpy((u8 *)(&info->touch_info),
		(u8 *)&info->cur_data[total_node],
		sizeof(struct point_info));
	up(&info->raw_data_lock);

	return true;
}

#if defined(USE_ACTIVE_REPORT_RATE)	
#define BT532_LPM_INTERRUPT_INTERVAL	0x0142
#define BT532_LPM_PERIOD_DELAY			0x00e2
#define BT532_LPM_STATE					0x007e

#define	DEF_TS_LPM_LEVEL_DEFAULT	3

static int g_prev_lpm_level = DEF_TS_LPM_LEVEL_DEFAULT;
static int g_lpm_level = DEF_TS_LPM_LEVEL_DEFAULT;

static void ts_lpm_set(int level)
{
	if(g_prev_lpm_level == level)	return;

	if(level==1)	//25Hz
	{
		if(write_reg(misc_info->client, BT532_LPM_PERIOD_DELAY, 1000)!=I2C_SUCCESS)
			return;
		if(write_reg(misc_info->client, BT532_LPM_INTERRUPT_INTERVAL, 25)!=I2C_SUCCESS)
			return;
		if(write_reg(misc_info->client, BT532_LPM_STATE, 1)!=I2C_SUCCESS)
			return;
	}
	else if(level==2)	//60Hz
	{
		if(write_reg(misc_info->client, BT532_LPM_PERIOD_DELAY, 150)!=I2C_SUCCESS)
			return;
		if(write_reg(misc_info->client, BT532_LPM_INTERRUPT_INTERVAL, 60)!=I2C_SUCCESS)
			return;
		if(write_reg(misc_info->client, BT532_LPM_STATE, 1)!=I2C_SUCCESS)
			return;
	}
	else	// set default
	{
		if(write_reg(misc_info->client, BT532_LPM_PERIOD_DELAY, 1000)!=I2C_SUCCESS)
			return;
		if(write_reg(misc_info->client, BT532_LPM_INTERRUPT_INTERVAL, 25)!=I2C_SUCCESS)
			return;
		if(write_reg(misc_info->client, BT532_LPM_STATE, 3)!=I2C_SUCCESS)
			return;
	}
	g_prev_lpm_level = level;
}

static void ts_lpm_level_set(int level)
{
	g_lpm_level = level;

	if(misc_info == NULL)	return;
	if (misc_info->work_state != NOTHING)
		return;
	ts_lpm_set(g_lpm_level);
}
#endif
static bool ts_read_coord(struct bt532_ts_info *info)
{
	struct i2c_client *client = info->client;
#if (TOUCH_POINT_MODE == 1)
	int i;
#endif

	/* zinitix_debug_msg("ts_read_coord+\r\n"); */

	/* for  Debugging Tool */

	if (info->touch_mode != TOUCH_POINT_MODE) {
		if (ts_get_raw_data(info) == false)
			return false;

		dev_err(&client->dev, "status = 0x%04X\n", info->touch_info.status);

		goto out;
	}

#if (TOUCH_POINT_MODE == 1)
	memset(&info->touch_info,
			0x0, sizeof(struct point_info));

	if (read_data(info->client, BT532_POINT_STATUS_REG,
			(u8 *)(&info->touch_info), 4) < 0) {
		dev_err(&client->dev, "%s: Failed to read point info\n", __func__);

		return false;
	}

	dev_info(&client->dev, "status reg = 0x%x , event_flag = 0x%04x\n",
				info->touch_info.status, info->touch_info.event_flag);

	if (info->touch_info.event_flag == 0)
		goto out;

	for (i = 0; i < info->cap_info.multi_fingers; i++) {
		if (zinitix_bit_test(info->touch_info.event_flag, i)) {
			udelay(20);

			if (read_data(info->client, BT532_POINT_STATUS_REG + 2 + ( i * 4),
					(u8 *)(&info->touch_info.coord[i]),
				sizeof(struct coord)) < 0) {
				dev_err(&client->dev, "Failed to read point info\n");

				return false;
			}
		}
	}

#else
	if (read_data(info->client, BT532_POINT_STATUS_REG,
		(u8 *)(&info->touch_info), sizeof(struct point_info)) < 0) {
		dev_err(&client->dev, "Failed to read point info\n");

		return false;
	}
#endif
#ifdef ZINITIX_NEW_TA_COVER_REGISTER_0x11F
	bt532_set_optional_mode(info, false);
#endif
out:
	/* error */
	if (zinitix_bit_test(info->touch_info.status, BIT_MUST_ZERO)) {
		dev_err(&client->dev, "Invalid must zero bit(%04x)\n",
			info->touch_info.status);
		/*write_cmd(info->client, BT532_CLEAR_INT_STATUS_CMD);
		udelay(DELAY_FOR_SIGNAL_DELAY);*/
		return false;
	}

#if defined(USE_ACTIVE_REPORT_RATE)	
	ts_lpm_set(g_lpm_level);	
#endif

/*
	if (zinitix_bit_test(info->touch_info.status, BIT_ICON_EVENT)) {
		udelay(20);
		if (read_data(info->client,
			BT532_ICON_STATUS_REG,
			(u8 *)(&info->icon_event_reg), 2) < 0) {
			zinitix_printk("error read icon info using i2c.\n");
			return false;
		}
	}
*/
	write_cmd(info->client, BT532_CLEAR_INT_STATUS_CMD);
	/* udelay(DELAY_FOR_SIGNAL_DELAY); */
	/* zinitix_debug_msg("ts_read_coord-\r\n"); */
	return true;
}

#if ESD_TIMER_INTERVAL
static void esd_timeout_handler(unsigned long data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)data;

	info->p_esd_timeout_tmr = NULL;
	queue_work(esd_tmr_workqueue, &info->tmr_work);
}

static void esd_timer_start(u16 sec, struct bt532_ts_info *info)
{
	unsigned long flags;
	spin_lock_irqsave(&info->lock, flags);
	if (info->p_esd_timeout_tmr != NULL)
#ifdef CONFIG_SMP
		del_singleshot_timer_sync(info->p_esd_timeout_tmr);
#else
		del_timer(info->p_esd_timeout_tmr);
#endif
	info->p_esd_timeout_tmr = NULL;
	init_timer(&(info->esd_timeout_tmr));
	info->esd_timeout_tmr.data = (unsigned long)(info);
	info->esd_timeout_tmr.function = esd_timeout_handler;
	info->esd_timeout_tmr.expires = jiffies + (HZ * sec);
	info->p_esd_timeout_tmr = &info->esd_timeout_tmr;
	add_timer(&info->esd_timeout_tmr);
	spin_unlock_irqrestore(&info->lock, flags);
}

static void esd_timer_stop(struct bt532_ts_info *info)
{
	unsigned long flags;
	spin_lock_irqsave(&info->lock, flags);
	if (info->p_esd_timeout_tmr)
#ifdef CONFIG_SMP
		del_singleshot_timer_sync(info->p_esd_timeout_tmr);
#else
		del_timer(info->p_esd_timeout_tmr);
#endif

	info->p_esd_timeout_tmr = NULL;
	spin_unlock_irqrestore(&info->lock, flags);
}

static void esd_timer_init(struct bt532_ts_info *info)
{
	unsigned long flags;
	spin_lock_irqsave(&info->lock, flags);
	init_timer(&(info->esd_timeout_tmr));
	info->esd_timeout_tmr.data = (unsigned long)(info);
	info->esd_timeout_tmr.function = esd_timeout_handler;
	info->p_esd_timeout_tmr = NULL;
	spin_unlock_irqrestore(&info->lock, flags);
}

static void ts_tmr_work(struct work_struct *work)
{
	struct bt532_ts_info *info =
			container_of(work, struct bt532_ts_info, tmr_work);
	struct i2c_client *client = info->client;

#if defined(TSP_VERBOSE_DEBUG)
	dev_info(&client->dev, "tmr queue work ++\n");
#endif

	if (down_trylock(&info->work_lock)) {
		dev_err(&client->dev, "%s: Failed to occupy work lock\n", __func__);
		esd_timer_start(CHECK_ESD_TIMER, info);

		return;
	}

	if (info->work_state != NOTHING) {
		dev_info(&client->dev, "%s: Other process occupied (%d)\n",
			__func__, info->work_state);
		up(&info->work_lock);

		return;
	}
	info->work_state = ESD_TIMER;

	disable_irq(info->irq);
	bt532_power_control(info, POWER_OFF);
	bt532_power_control(info, POWER_ON_SEQUENCE);

	clear_report_data(info);
	if (mini_init_touch(info) == false)
		goto fail_time_out_init;

	info->work_state = NOTHING;
	enable_irq(info->irq);
	up(&info->work_lock);
#if defined(TSP_VERBOSE_DEBUG)
	dev_info(&client->dev, "tmr queue work--\n");
#endif

	return;
fail_time_out_init:
	dev_err(&client->dev, "%s: Failed to restart\n", __func__);
	esd_timer_start(CHECK_ESD_TIMER, info);
	info->work_state = NOTHING;
	enable_irq(info->irq);
	up(&info->work_lock);

	return;
}
#endif

static bool bt532_power_sequence(struct bt532_ts_info *info)
{
	struct i2c_client *client = info->client;
	int retry = 0;
	u16 chip_code;

retry_power_sequence:
	if (write_reg(client, 0xc000, 0x0001) != I2C_SUCCESS) {
		dev_err(&client->dev, "Failed to send power sequence(vendor cmd enable)\n");
		goto fail_power_sequence;
	}
	udelay(10);

	if (read_data(client, 0xcc00, (u8 *)&chip_code, 2) < 0) {
		dev_err(&client->dev, "Failed to read chip code\n");
		goto fail_power_sequence;
	}

	dev_info(&client->dev, "%s: chip code = 0x%x\n", __func__, chip_code);
	udelay(10);

	if (write_cmd(client, 0xc004) != I2C_SUCCESS) {
		dev_err(&client->dev, "Failed to send power sequence(intn clear)\n");
		goto fail_power_sequence;
	}
	udelay(10);

	if (write_reg(client, 0xc002, 0x0001) != I2C_SUCCESS) {
		dev_err(&client->dev, "Failed to send power sequence(nvm init)\n");
		goto fail_power_sequence;
	}
	mdelay(2);

	if (write_reg(client, 0xc001, 0x0001) != I2C_SUCCESS) {
		dev_err(&client->dev, "Failed to send power sequence(program start)\n");
		goto fail_power_sequence;
	}
	msleep(FIRMWARE_ON_DELAY);	/* wait for checksum cal */

	return true;

fail_power_sequence:
	if (retry++ < 3) {
		msleep(CHIP_ON_DELAY);
		dev_info(&client->dev, "retry = %d\n", retry);
		goto retry_power_sequence;
	}

	dev_err(&client->dev, "Failed to send power sequence\n");
	return false;
}

static void bt532_hw_power(struct bt532_ts_info *info, int onoff)
{
	struct bt532_ts_platform_data *pdata = info->pdata;
	if(onoff){
		if (info->vddo_vreg)
			regulator_enable(info->vddo_vreg);
	}else{
		if (info->vddo_vreg)
			regulator_disable(info->vddo_vreg);
	}
	
	if (pdata->tsp_supply_type == TSP_LDO_SUPPLY){
		gpio_direction_output(info->pdata->tsp_en_gpio, onoff);
	}else if (pdata->tsp_supply_type == TSP_REGULATOR_SUPPLY){
		if(onoff){
			if (info->vdd_en)
				regulator_enable(info->vdd_en);
		}else{
			if (info->vdd_en)
				regulator_disable(info->vdd_en);
		}
	}	
	return;
}


static bool bt532_power_control(struct bt532_ts_info *info, u8 ctl)
{
	pr_info("[TSP] %s, %d\n", __func__, ctl);

	if (ctl == POWER_OFF) {
		bt532_hw_power(info, 0);
		mdelay(CHIP_OFF_DELAY);
	} else if (ctl == POWER_ON_SEQUENCE) {
		bt532_hw_power(info, 1);
		mdelay(CHIP_ON_DELAY);
		msleep(CHIP_ON_DELAY);
		return bt532_power_sequence(info); //zxt power on sequence
	} else if (ctl == POWER_ON) {
		bt532_hw_power(info, 1);
		mdelay(CHIP_ON_DELAY);
	}
	return true;
}

static void bt532_set_ta_status(struct bt532_ts_info *info)
{
#ifndef ZINITIX_NEW_TA_COVER_REGISTER_0x11F
	u16	reg_val;
#endif
	printk("Inside bt532_set_ta_status()\n");

	if(info->work_state == SUSPEND || info->work_state == PROBE)
		return;

	if (ta_connected) {
#ifdef ZINITIX_NEW_TA_COVER_REGISTER_0x11F
		zinitix_bit_set(m_optional_mode, 1);
#else
		read_data(info->client, 0x011e,	(u8 *)&reg_val, 2);
		zinitix_bit_clr(reg_val, 1);
		zinitix_bit_set(reg_val, 14);
		write_reg(info->client, 0x011e,	reg_val);
#endif
		}
	else {
#ifdef ZINITIX_NEW_TA_COVER_REGISTER_0x11F
		zinitix_bit_clr(m_optional_mode, 1);
#else
		read_data(info->client, 0x011e,	(u8 *)&reg_val, 2);
		zinitix_bit_set(reg_val, 1);
		zinitix_bit_clr(reg_val, 14);
		write_reg(info->client, 0x011e,	reg_val);
#endif
		}
}

void bt532_charger_status_cb(int status)
{
	int mode = (status == CABLE_TYPE_NONE);
	printk("Inside bt532_charger_status_cb()\n");
	if (mode)
		ta_connected = false;
	else
		ta_connected = true;

	bt532_set_ta_status(misc_info);

	dev_info(&misc_info->client->dev, "TA %s\n",
		mode ? "disconnected" : "connected");
}
#if TOUCH_ONESHOT_UPGRADE
static bool ts_check_need_upgrade(struct bt532_ts_info *info,
	u16 cur_version, u16 cur_minor_version, u16 cur_reg_version, u16 cur_hw_id)
{
	u16	new_version;
	u16	new_minor_version;
	u16	new_reg_version;
/*	u16	new_chip_code;*/
#if CHECK_HWID
	u16	new_hw_id;
#endif
	new_version = (u16) (m_firmware_data[52] | (m_firmware_data[53]<<8));
	new_minor_version = (u16) (m_firmware_data[56] | (m_firmware_data[57]<<8));
	new_reg_version = (u16) (m_firmware_data[60] | (m_firmware_data[61]<<8));
/*	new_chip_code = (u16) (m_firmware_data[64] | (m_firmware_data[65]<<8));*/

#if CHECK_HWID
	new_hw_id = (u16) (m_firmware_data[0x6b12] | (m_firmware_data[0x6b13]<<8));
	zinitix_printk("cur HW_ID = 0x%x, new HW_ID = 0x%x\n",
		cur_hw_id, new_hw_id);
	if (cur_hw_id != new_hw_id)
		return true;
#endif

	zinitix_printk("cur version = 0x%x, new version = 0x%x\n",
		cur_version, new_version);
	if(cur_version > 0xFF)
		return true;
	if (cur_version < new_version)
			return true;
		else if (cur_version > new_version)
			return false;

	zinitix_printk("cur minor version = 0x%x, new minor version = 0x%x\n",
			cur_minor_version, new_minor_version);
	if (cur_minor_version < new_minor_version)
		return true;
	else if (cur_minor_version > new_minor_version)
		return false;

	zinitix_printk("cur reg data version = 0x%x, new reg data version = 0x%x\n",
			cur_reg_version, new_reg_version);
	if (cur_reg_version < new_reg_version)
		return true;

	return false;
}
#endif

#define TC_SECTOR_SZ		8

static bool ts_upgrade_firmware(struct bt532_ts_info *info,
	const u8 *firmware_data, u32 size)
{
	struct i2c_client *client = info->client;
	u16 flash_addr;
	u8 *verify_data;
	int retry_cnt = 0;
	int i;
	int page_sz = 64;
	u16 chip_code;

	verify_data = kzalloc(size, GFP_KERNEL);
	if (verify_data == NULL) {
		zinitix_printk(KERN_ERR "cannot alloc verify buffer\n");
		return false;
	}

retry_upgrade:
	bt532_power_control(info, POWER_OFF);
	bt532_power_control(info, POWER_ON);
	mdelay(10);

	if (write_reg(client, 0xc000, 0x0001) != I2C_SUCCESS) {
		zinitix_printk("power sequence error (vendor cmd enable)\n");
		goto fail_upgrade;
	}

	udelay(10);

	if (read_data(client, 0xcc00, (u8 *)&chip_code, 2) < 0) {
		zinitix_printk("failed to read chip code\n");
		goto fail_upgrade;
	}

	zinitix_printk("chip code = 0x%x\n", chip_code);
	page_sz = 128;
	udelay(10);

	if (write_cmd(client, 0xc004) != I2C_SUCCESS) {
		zinitix_printk("power sequence error (intn clear)\n");
		goto fail_upgrade;
	}

	udelay(10);

	if (write_reg(client, 0xc002, 0x0001) != I2C_SUCCESS) {
		zinitix_printk("power sequence error (nvm init)\n");
		goto fail_upgrade;
	}

	mdelay(5);

	zinitix_printk(KERN_INFO "init flash\n");

	if (write_reg(client, 0xc003, 0x0001) != I2C_SUCCESS) {
		zinitix_printk("failed to write nvm vpp on\n");
		goto fail_upgrade;
	}

	if (write_reg(client, 0xc104, 0x0001) != I2C_SUCCESS) {
		zinitix_printk("failed to write nvm wp disable\n");
		goto fail_upgrade;
	}

	if (write_cmd(client, BT532_INIT_FLASH) != I2C_SUCCESS) {
		zinitix_printk(KERN_INFO "failed to init flash\n");
		goto fail_upgrade;
	}

	zinitix_printk(KERN_INFO "writing firmware data\n");
	for (flash_addr = 0; flash_addr < size; ) {
		for (i = 0; i < page_sz/TC_SECTOR_SZ; i++) {
			//zinitix_debug_msg("write :addr=%04x, len=%d\n",	flash_addr, TC_SECTOR_SZ);
			if (write_data(client,
				BT532_WRITE_FLASH,
				(u8 *)&firmware_data[flash_addr], TC_SECTOR_SZ) < 0) {
				zinitix_printk(KERN_INFO"error : write zinitix tc firmare\n");
				goto fail_upgrade;
			}
			flash_addr += TC_SECTOR_SZ;
			udelay(100);
		}

		mdelay(30);	/*for fuzing delay*/
	}

	if (write_reg(client, 0xc003, 0x0000) != I2C_SUCCESS) {
		zinitix_printk("nvm write vpp off\n");
		goto fail_upgrade;
	}

	if (write_reg(client, 0xc104, 0x0000) != I2C_SUCCESS) {
		zinitix_printk("nvm wp enable\n");
		goto fail_upgrade;
	}

	zinitix_printk(KERN_INFO "init flash\n");

	if (write_cmd(client, BT532_INIT_FLASH) != I2C_SUCCESS) {
		zinitix_printk(KERN_INFO "failed to init flash\n");
		goto fail_upgrade;
	}

	zinitix_printk(KERN_INFO "read firmware data\n");

	for (flash_addr = 0; flash_addr < size; ) {
		for (i = 0; i < page_sz/TC_SECTOR_SZ; i++) {
			/*zinitix_debug_msg("read :addr=%04x, len=%d\n", flash_addr, TC_SECTOR_SZ);*/
			if (read_firmware_data(client,
					BT532_READ_FLASH,
				(u8*)&verify_data[flash_addr], TC_SECTOR_SZ) < 0) {
				dev_err(&client->dev, "Failed to read firmare\n");

				goto fail_upgrade;
			}

			flash_addr += TC_SECTOR_SZ;
		}
	}
	/* verify */
	dev_info(&client->dev, "verify firmware data\n");
	if (memcmp((u8 *)&firmware_data[0], (u8 *)&verify_data[0], size) == 0) {
		dev_info(&client->dev, "upgrade finished\n");
		kfree(verify_data);
		bt532_power_control(info, POWER_OFF);
		bt532_power_control(info, POWER_ON_SEQUENCE);

		return true;
	}

fail_upgrade:
	bt532_power_control(info, POWER_OFF);

	if (retry_cnt++ < INIT_RETRY_CNT) {
		dev_err(&client->dev, "upgrade failed : so retry... (%d)\n", retry_cnt);
		goto retry_upgrade;
	}

	if (verify_data != NULL)
		kfree(verify_data);

	dev_info(&client->dev, "Failed to upgrade\n");

	return false;
}

static bool ts_hw_calibration(struct bt532_ts_info *info)
{
	struct i2c_client *client = info->client;
	u16	chip_eeprom_info;
	int time_out = 0;

#if defined(CONFIG_SEC_DEGAS_PROJECT)
	if (info->tsp_type_hw == TSP_HW_ID_INDEX_NULL){		//HW_ID 3
		zinitix_printk("TSP panel wasn't connected to board\n");
		return true;
	}
#endif
	if (write_reg(client,
		BT532_TOUCH_MODE, 0x07) != I2C_SUCCESS)
		return false;
	mdelay(10);
	write_cmd(client, BT532_CLEAR_INT_STATUS_CMD);
	mdelay(10);
	write_cmd(client, BT532_CLEAR_INT_STATUS_CMD);
	mdelay(50);
	write_cmd(client, BT532_CLEAR_INT_STATUS_CMD);
	mdelay(10);

	if (write_cmd(client,
		BT532_CALIBRATE_CMD) != I2C_SUCCESS)
		return false;

	if (write_cmd(client,
		BT532_CLEAR_INT_STATUS_CMD) != I2C_SUCCESS)
		return false;

	mdelay(10);
	write_cmd(client, BT532_CLEAR_INT_STATUS_CMD);

	/* wait for h/w calibration*/
	do {
		mdelay(500);
		write_cmd(client,
				BT532_CLEAR_INT_STATUS_CMD);

		if (read_data(client,
			BT532_EEPROM_INFO_REG,
			(u8 *)&chip_eeprom_info, 2) < 0)
			return false;

		zinitix_debug_msg("touch eeprom info = 0x%04X\r\n",
			chip_eeprom_info);
		if (!zinitix_bit_test(chip_eeprom_info, 0))
			break;

		if (time_out++ == 4) {
			write_cmd(client, BT532_CALIBRATE_CMD);
			mdelay(10);
			write_cmd(client, BT532_CLEAR_INT_STATUS_CMD);
			dev_err(&client->dev, "h/w calibration retry timeout.\n");
		}

		if (time_out++ > 10) {
			dev_err(&client->dev, "h/w calibration timeout.\n");
			break;
		}

	} while (1);

	if (write_reg(client,
		BT532_TOUCH_MODE, TOUCH_POINT_MODE) != I2C_SUCCESS)
		return false;

	if (info->cap_info.ic_int_mask != 0) {
		if (write_reg(client,
			BT532_INT_ENABLE_FLAG,
			info->cap_info.ic_int_mask)
			!= I2C_SUCCESS)
			return false;
	}

	write_reg(client, 0xc003, 0x0001);
	write_reg(client, 0xc104, 0x0001);
	udelay(100);
	if (write_cmd(client,
		BT532_SAVE_CALIBRATION_CMD) != I2C_SUCCESS)
		return false;

	mdelay(2000);
	write_reg(client, 0xc003, 0x0000);
	write_reg(client, 0xc104, 0x0000);
	return true;
}

static bool init_touch(struct bt532_ts_info *info)
{
	struct bt532_ts_platform_data *pdata = info->pdata;
	struct i2c_client *client = info->client;
	struct capa_info *cap = &(info->cap_info);
	u16 reg_val;
	int i;
	u16 chip_eeprom_info;
#if USE_CHECKSUM
	u16 chip_check_sum;
	u8 checksum_err;
#endif
	int retry_cnt = 0;
	char* productionMode = "androidboot.bsp=2";
	char* checkMode = NULL;

#if defined(USE_ACTIVE_REPORT_RATE)	
	g_prev_lpm_level = DEF_TS_LPM_LEVEL_DEFAULT;
	g_lpm_level = DEF_TS_LPM_LEVEL_DEFAULT;
#endif	

	checkMode = strstr(saved_command_line, productionMode);
retry_init:
	for (i = 0; i < INIT_RETRY_CNT; i++) {
		if (read_data(client, BT532_EEPROM_INFO_REG,
				(u8 *)&chip_eeprom_info, 2) < 0) {
			dev_err(&client->dev, "Failed to read eeprom info(%d)\n", i);
			mdelay(10);
			continue;
		} else
			break;
	}

	if (i == INIT_RETRY_CNT)
		goto fail_init;

#if USE_CHECKSUM
	dev_info(&client->dev, "%s: Check checksum\n", __func__);

	checksum_err = 0;

	for (i = 0; i < INIT_RETRY_CNT; i++) {
		if (read_data(client, BT532_CHECKSUM_RESULT,
				(u8 *)&chip_check_sum, 2) < 0) {
			mdelay(10);
			continue;
		}

#if defined(TSP_VERBOSE_DEBUG)
		dev_info(&client->dev, "0x%04X\n", chip_check_sum);
#endif

		if(chip_check_sum == 0x55aa)
			break;
		else {
			checksum_err = 1;
		break;
	}
	}

	if (i == INIT_RETRY_CNT || checksum_err) {
		dev_err(&client->dev, "Failed to check firmware data\n");
		if (checksum_err == 1 && retry_cnt < INIT_RETRY_CNT)
			retry_cnt = INIT_RETRY_CNT;

		goto fail_init;
	}
#endif

	if (write_cmd(client, BT532_SWRESET_CMD) != I2C_SUCCESS) {
		dev_err(&client->dev, "Failed to write reset command\n");
		goto fail_init;
	}

	cap->button_num = SUPPORTED_BUTTON_NUM;

	reg_val = 0;
	zinitix_bit_set(reg_val, BIT_PT_CNT_CHANGE);
	zinitix_bit_set(reg_val, BIT_DOWN);
	zinitix_bit_set(reg_val, BIT_MOVE);
	zinitix_bit_set(reg_val, BIT_UP);
#if (TOUCH_POINT_MODE == 2)
	zinitix_bit_set(reg_val, BIT_PALM);
	zinitix_bit_set(reg_val, BIT_PALM_REJECT);
#endif

	if (cap->button_num > 0)
		zinitix_bit_set(reg_val, BIT_ICON_EVENT);

	cap->ic_int_mask = reg_val;

	if (write_reg(client, BT532_INT_ENABLE_FLAG, 0x0) != I2C_SUCCESS)
		goto fail_init;

	dev_info(&client->dev, "%s: Send reset command\n", __func__);
	if (write_cmd(client, BT532_SWRESET_CMD) != I2C_SUCCESS)
		goto fail_init;

	/* get chip information */
	if (read_data(client, BT532_VENDOR_ID,
					(u8 *)&cap->vendor_id, 2) < 0) {
		zinitix_printk("failed to read chip revision\n");
		goto fail_init;
	}

	if (read_data(client, BT532_CHIP_REVISION,
					(u8 *)&cap->ic_revision, 2) < 0) {
		zinitix_printk("failed to read chip revision\n");
		goto fail_init;
	}

	cap->ic_fw_size = 32*1024;

	if (read_data(client, BT532_HW_ID, (u8 *)&cap->hw_id, 2) < 0) {
		dev_err(&client->dev, "Failed to read hw id\n");
		goto fail_init;
	}
	if (read_data(client, BT532_THRESHOLD, (u8 *)&cap->threshold, 2) < 0)
		goto fail_init;

	if (read_data(client, BT532_THRESHOLD,
					(u8 *)&cap->threshold, 2) < 0)
			goto fail_init;

	if (read_data(client, BT532_BUTTON_SENSITIVITY,
					(u8 *)&cap->key_threshold, 2) < 0)
		goto fail_init;

	if (read_data(client, BT532_DUMMY_BUTTON_SENSITIVITY,
					(u8 *)&cap->dummy_threshold, 2) < 0)
		goto fail_init;

	if (read_data(client, BT532_TOTAL_NUMBER_OF_X,
					(u8 *)&cap->x_node_num, 2) < 0)
		goto fail_init;

	if (read_data(client, BT532_TOTAL_NUMBER_OF_Y,
					(u8 *)&cap->y_node_num, 2) < 0)
		goto fail_init;

	cap->total_node_num = cap->x_node_num * cap->y_node_num;

	if (read_data(client, BT532_DND_N_COUNT,
					(u8 *)&cap->N_cnt, 2) < 0)
		goto fail_init;

	zinitix_debug_msg("N count = %d\n", cap->N_cnt);

	if (read_data(client, BT532_DND_U_COUNT,
					(u8 *)&cap->u_cnt, 2) < 0)
		goto fail_init;

	zinitix_debug_msg("u count = %d\n", cap->u_cnt);

	if (read_data(client, BT532_AFE_FREQUENCY,
					(u8 *)&cap->afe_frequency, 2) < 0)
		goto fail_init;

	zinitix_debug_msg("afe frequency = %d\n", cap->afe_frequency);


	if (read_data(client, BT532_DND_SHIFT_VALUE,
				(u8 *)&cap->shift_value, 2) < 0)
		goto fail_init;

	/* get chip firmware version */
	if (read_data(client, BT532_FIRMWARE_VERSION,
		(u8 *)&cap->fw_version, 2) < 0)
		goto fail_init;

	if (read_data(client, BT532_MINOR_FW_VERSION,
		(u8 *)&cap->fw_minor_version, 2) < 0)
		goto fail_init;

	if (read_data(client, BT532_DATA_VERSION_REG,
		(u8 *)&cap->reg_data_version, 2) < 0)
		goto fail_init;

#if TOUCH_ONESHOT_UPGRADE
	if ((checkMode == NULL) &&(ts_check_need_upgrade(info, cap->fw_version,
			cap->fw_minor_version, cap->reg_data_version,
								cap->hw_id) == true)) {
		zinitix_printk("start upgrade firmware\n");

		if (ts_upgrade_firmware(info, m_firmware_data,
			cap->ic_fw_size) == false)
			goto fail_init;

		if (ts_hw_calibration(info) == false)
			goto fail_init;

		/* disable chip interrupt */
		if (write_reg(client, BT532_INT_ENABLE_FLAG, 0) != I2C_SUCCESS)
			goto fail_init;

		/* get chip firmware version */
		if (read_data(client, BT532_FIRMWARE_VERSION,
				(u8 *)&cap->fw_version, 2) < 0)
			goto fail_init;

		if (read_data(client, BT532_MINOR_FW_VERSION,
				(u8 *)&cap->fw_minor_version, 2) < 0)
			goto fail_init;

		if (read_data(client, BT532_DATA_VERSION_REG,
				(u8 *)&cap->reg_data_version, 2) < 0)
			goto fail_init;
	}
#endif

	if (read_data(client, BT532_EEPROM_INFO_REG,
			(u8 *)&chip_eeprom_info, 2) < 0)
		goto fail_init;

	if (zinitix_bit_test(chip_eeprom_info, 0)) { /* hw calibration bit*/
		if (ts_hw_calibration(info) == false)
			goto fail_init;

		/* disable chip interrupt */
		if (write_reg(client, BT532_INT_ENABLE_FLAG, 0) != I2C_SUCCESS)
			goto fail_init;
	}

	/* initialize */
	if (write_reg(client, BT532_X_RESOLUTION,
			(u16)pdata->x_resolution) != I2C_SUCCESS)
		goto fail_init;

	if (write_reg(client, BT532_Y_RESOLUTION,
			(u16)pdata->y_resolution) != I2C_SUCCESS)
		goto fail_init;

	cap->MinX = (u32)0;
	cap->MinY = (u32)0;
	cap->MaxX = (u32)pdata->x_resolution;
	cap->MaxY = (u32)pdata->y_resolution;

	if (write_reg(client, BT532_BUTTON_SUPPORTED_NUM,
		(u16)cap->button_num) != I2C_SUCCESS)
		goto fail_init;

	if (write_reg(client, BT532_SUPPORTED_FINGER_NUM,
		(u16)MAX_SUPPORTED_FINGER_NUM) != I2C_SUCCESS)
		goto fail_init;

	cap->multi_fingers = MAX_SUPPORTED_FINGER_NUM;

	zinitix_debug_msg("max supported finger num = %d\r\n",
		cap->multi_fingers);
	cap->gesture_support = 0;
	zinitix_debug_msg("set other configuration\r\n");

	if (write_reg(client, BT532_INITIAL_TOUCH_MODE,
					TOUCH_POINT_MODE) != I2C_SUCCESS)
		goto fail_init;

	if (write_reg(client, BT532_TOUCH_MODE, info->touch_mode) != I2C_SUCCESS)
		goto fail_init;


#ifdef ZINITIX_NEW_TA_COVER_REGISTER_0x11F
	read_data(client, ZINITIX_INTERNAL_FLAG_03,	(u8 *)&g_internal_flag_03, 2);
	zinitix_bit_clr(g_internal_flag_03, 1);		//TA
	zinitix_bit_clr(g_internal_flag_03, 2);		//cover
	bt532_set_optional_mode(info, true);
#else
	bt532_set_ta_status(info);
#endif
	/* soft calibration */
//	if (write_cmd(client, BT532_CALIBRATE_CMD) != I2C_SUCCESS)
//		goto fail_init;

	if (write_reg(client, BT532_INT_ENABLE_FLAG,
		cap->ic_int_mask) != I2C_SUCCESS)
		goto fail_init;

	/* read garbage data */
	for (i = 0; i < 10; i++) {
		write_cmd(client, BT532_CLEAR_INT_STATUS_CMD);
		udelay(10);
	}

	if (info->touch_mode != TOUCH_POINT_MODE) { /* Test Mode */
		if (write_reg(client, BT532_DELAY_RAW_FOR_HOST,
			RAWDATA_DELAY_FOR_HOST) != I2C_SUCCESS) {
			dev_err(&client->dev, "%s: Failed to set DELAY_RAW_FOR_HOST\n",
						__func__);

			goto fail_init;
		}
	}
#if ESD_TIMER_INTERVAL
	if (write_reg(client, BT532_PERIODICAL_INTERRUPT_INTERVAL,
			SCAN_RATE_HZ * ESD_TIMER_INTERVAL) != I2C_SUCCESS)
		goto fail_init;

	read_data(client, BT532_PERIODICAL_INTERRUPT_INTERVAL, (u8 *)&reg_val, 2);
#if defined(TSP_VERBOSE_DEBUG)
	dev_info(&client->dev, "Esd timer register = %d\n", reg_val);
#endif
#endif
	zinitix_debug_msg("successfully initialized\r\n");
	return true;

fail_init:
	if (++retry_cnt <= INIT_RETRY_CNT) {
		bt532_power_control(info, POWER_OFF);
		bt532_power_control(info, POWER_ON_SEQUENCE);

		zinitix_debug_msg("retry to initiallize(retry cnt = %d)\r\n",
				retry_cnt);
		goto	retry_init;

	} else if (retry_cnt == INIT_RETRY_CNT+1) {
		cap->ic_fw_size = 32*1024;

		zinitix_debug_msg("retry to initiallize(retry cnt = %d)\r\n", retry_cnt);
#if TOUCH_FORCE_UPGRADE
		if(checkMode == NULL) {
		if (ts_upgrade_firmware(info, m_firmware_data,
			cap->ic_fw_size) == false) {
			zinitix_printk("upgrade failed\n");
			return false;
		}
		}
		else
			return true;
		mdelay(100);

		/* hw calibration and make checksum */
		if (ts_hw_calibration(info) == false) {
			zinitix_printk("failed to initiallize\r\n");
			return false;
		}
		goto retry_init;
#endif
	}

	dev_err(&client->dev, "Failed to initiallize\n");

	return false;
}

static bool mini_init_touch(struct bt532_ts_info *info)
{
	struct bt532_ts_platform_data *pdata = info->pdata;
	struct i2c_client *client = info->client;
	int i;
#if USE_CHECKSUM
	u16 chip_check_sum;

	dev_info(&client->dev, "check checksum\n");

	if (read_data(client, BT532_CHECKSUM_RESULT,
			(u8 *)&chip_check_sum, 2) < 0)
		goto fail_mini_init;

	if (chip_check_sum != 0x55aa) {
		dev_err(&client->dev, "Failed to check firmware"
			" checksum(0x%04x)\n", chip_check_sum);

		goto fail_mini_init;
	}
#endif

#if defined(USE_ACTIVE_REPORT_RATE)	
	g_prev_lpm_level = DEF_TS_LPM_LEVEL_DEFAULT;
	g_lpm_level = DEF_TS_LPM_LEVEL_DEFAULT;
#endif
	
	if (write_cmd(client, BT532_SWRESET_CMD) != I2C_SUCCESS) {
		dev_info(&client->dev, "Failed to write reset command\n");

		goto fail_mini_init;
	}

	/* initialize */
	if (write_reg(client, BT532_X_RESOLUTION,
			(u16)(pdata->x_resolution)) != I2C_SUCCESS)
		goto fail_mini_init;

	if (write_reg(client, BT532_Y_RESOLUTION,
			(u16)(pdata->y_resolution)) != I2C_SUCCESS)
		goto fail_mini_init;

	dev_info(&client->dev, "touch max x = %d\r\n", pdata->x_resolution);
	dev_info(&client->dev, "touch max y = %d\r\n", pdata->y_resolution);

	if (write_reg(client, BT532_BUTTON_SUPPORTED_NUM,
			(u16)info->cap_info.button_num) != I2C_SUCCESS)
		goto fail_mini_init;

	if (write_reg(client, BT532_SUPPORTED_FINGER_NUM,
			(u16)MAX_SUPPORTED_FINGER_NUM) != I2C_SUCCESS)
		goto fail_mini_init;

	if (write_reg(client, BT532_INITIAL_TOUCH_MODE,
			TOUCH_POINT_MODE) != I2C_SUCCESS)
		goto fail_mini_init;

	if (write_reg(client, BT532_TOUCH_MODE,
			info->touch_mode) != I2C_SUCCESS)
		goto fail_mini_init;

#ifdef ZINITIX_NEW_TA_COVER_REGISTER_0x11F
	bt532_set_optional_mode(info, true);
#else
	bt532_set_ta_status(info);

	/* soft calibration */
	if (write_cmd(client, BT532_CALIBRATE_CMD) != I2C_SUCCESS)
		goto fail_mini_init;
#endif

	if (write_reg(client, BT532_INT_ENABLE_FLAG,
			info->cap_info.ic_int_mask) != I2C_SUCCESS)
		goto fail_mini_init;

	/* read garbage data */
	for (i = 0; i < 10; i++) {
		write_cmd(client, BT532_CLEAR_INT_STATUS_CMD);
		udelay(10);
	}

	if (info->touch_mode != TOUCH_POINT_MODE) {
		if (write_reg(client, BT532_DELAY_RAW_FOR_HOST,
				RAWDATA_DELAY_FOR_HOST) != I2C_SUCCESS){
			dev_err(&client->dev, "Failed to set BT532_DELAY_RAW_FOR_HOST\n");

			goto fail_mini_init;
		}
	}

#if ESD_TIMER_INTERVAL
	if (write_reg(client, BT532_PERIODICAL_INTERRUPT_INTERVAL,
			SCAN_RATE_HZ * ESD_TIMER_INTERVAL) != I2C_SUCCESS)
		goto fail_mini_init;

	esd_timer_start(CHECK_ESD_TIMER, info);
#if defined(TSP_VERBOSE_DEBUG)
	dev_info(&client->dev, "Started esd timer\n");
#endif
#endif

	dev_info(&client->dev, "Successfully mini initialized\r\n");
	return true;

fail_mini_init:
	dev_err(&client->dev, "Failed to initialize mini init\n");
	bt532_power_control(info, POWER_OFF);
	bt532_power_control(info, POWER_ON_SEQUENCE);

	if (init_touch(info) == false) {
		dev_err(&client->dev, "Failed to initialize\n");

		return false;
	}

#if ESD_TIMER_INTERVAL
	esd_timer_start(CHECK_ESD_TIMER, info);
#if defined(TSP_VERBOSE_DEBUG)
	dev_info(&client->dev, "Started esd timer\n");
#endif
#endif
	return true;
}

#ifdef TSP_BOOSTER
static void change_dvfs_lock(struct work_struct *work)
{
	struct bt532_ts_info *info = container_of(work,
		struct bt532_ts_info, work_dvfs_chg.work);
	int ret = 0;

	mutex_lock(&info->dvfs_lock);

	if (info->boost_level == DVFS_STAGE_DUAL) {
		ret = set_freq_limit(DVFS_TOUCH_ID, MIN_TOUCH_LIMIT_SECOND);
		info->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
	} else if (info->boost_level == DVFS_STAGE_SINGLE) {
		ret = set_freq_limit(DVFS_TOUCH_ID, -1);
		info->dvfs_freq = -1;
	}
	if (ret < 0)
		printk(KERN_ERR "bt532_ts_device TSP %s: booster stop failed(%d)\n",\
					__func__, __LINE__);

	mutex_unlock(&info->dvfs_lock);
}
static void set_dvfs_off(struct work_struct *work)
{

	struct bt532_ts_info *info = container_of(work,
				struct bt532_ts_info, work_dvfs_off.work);
	int ret;
	mutex_lock(&info->dvfs_lock);
	ret = set_freq_limit(DVFS_TOUCH_ID, -1);
	if (ret < 0)
		printk(KERN_ERR "bt532_ts_device TSP %s: booster stop failed(%d)\n",\
					__func__, __LINE__);
	info->dvfs_freq = -1;
	info->dvfs_lock_status = false;
	mutex_unlock(&info->dvfs_lock);

}

static void set_dvfs_lock(struct bt532_ts_info *info, uint32_t on, bool mode)
{
	int ret = 0;

	if (DVFS_STAGE_NONE == info->boost_level)
		return;

	mutex_lock(&info->dvfs_lock);
	if (on == 0) {
		if (info->dvfs_lock_status) {
			schedule_delayed_work(&info->work_dvfs_off,
				msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));
		}
	} else if (on == 1) {
		cancel_delayed_work(&info->work_dvfs_off);
		if (!info->dvfs_lock_status || mode) {
			if (info->dvfs_old_status != on) {
				cancel_delayed_work(&info->work_dvfs_chg);
					if (info->dvfs_freq != MIN_TOUCH_LIMIT) {
						ret = set_freq_limit(DVFS_TOUCH_ID,
								MIN_TOUCH_LIMIT);
						info->dvfs_freq = MIN_TOUCH_LIMIT;

						if (ret < 0)
							printk(KERN_ERR
								"%s: cpu first lock failed(%d)\n",
								__func__, ret);

				schedule_delayed_work(&info->work_dvfs_chg,
					msecs_to_jiffies(TOUCH_BOOSTER_CHG_TIME));

					info->dvfs_lock_status = true;
				}
			}
		}
	} else if (on == 2) {
		if (info->dvfs_lock_status) {
			cancel_delayed_work(&info->work_dvfs_off);
			cancel_delayed_work(&info->work_dvfs_chg);
			schedule_work(&info->work_dvfs_off.work);
		}
	}
	info->dvfs_old_status = on;
	mutex_unlock(&info->dvfs_lock);
}
#endif

static void clear_report_data(struct bt532_ts_info *info)
{
	int i;
	u8 reported = 0;
	u8 sub_status;

	for (i = 0; i < info->cap_info.button_num; i++) {
		if (info->button[i] == ICON_BUTTON_DOWN) {
			info->button[i] = ICON_BUTTON_UP;
			input_report_key(info->input_dev, BUTTON_MAPPING_KEY[i], 0);
			reported = true;
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			printk(KERN_INFO "Button up = %d\n", i);
#else
			printk(KERN_INFO "Button up\n");
#endif
		}
	}

	for (i = 0; i < info->cap_info.multi_fingers; i++) {
		sub_status = info->reported_touch_info.coord[i].sub_status;
		if (zinitix_bit_test(sub_status, SUB_BIT_EXIST)) {
			input_mt_slot(info->input_dev, i);
			input_mt_report_slot_state(info->input_dev,	MT_TOOL_FINGER, 0);
			reported = true;
			if (!m_ts_debug_mode && TSP_NORMAL_EVENT_MSG)
				printk(KERN_INFO "[TSP] R %02d\r\n", i);
		}
		info->reported_touch_info.coord[i].sub_status = 0;
	}

	if (reported)
		input_sync(info->input_dev);
#ifdef TSP_BOOSTER
	if (info->touch_pressed_num != 0) {
		printk("bt532_ts_device : %s force dvfs off\n", __func__);
		info->touch_pressed_num = 0;
		set_dvfs_lock(info, 2, false);
	}
#endif

}

#define	PALM_REPORT_WIDTH	200
#define	PALM_REJECT_WIDTH	255

static irqreturn_t bt532_touch_work(int irq, void *data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)data;
	struct bt532_ts_platform_data *pdata = info->pdata;
	struct i2c_client *client = info->client;
	int i;
	u8 reported = false;
	u8 sub_status;
	u8 prev_sub_status;
	u32 x, y, maxX, maxY;
	u32 w;
	u32 tmp;
	u8 palm = 0;
#ifdef TSP_BOOSTER
	bool touch_press = false;
#endif

	if (gpio_get_value(info->pdata->gpio_int)) {
		dev_err(&client->dev, "Invalid interrupt\n");

		return IRQ_HANDLED;
	}

	if (down_trylock(&info->work_lock)) {
		dev_err(&client->dev, "%s: Failed to occupy work lock\n", __func__);
		write_cmd(client, BT532_CLEAR_INT_STATUS_CMD);

		return IRQ_HANDLED;
	}
#if ESD_TIMER_INTERVAL
	esd_timer_stop(info);
#endif

	if (info->work_state != NOTHING) {
		dev_err(&client->dev, "%s: Other process occupied\n", __func__);
		udelay(DELAY_FOR_SIGNAL_DELAY);

		if (!gpio_get_value(info->pdata->gpio_int)) {
			write_cmd(client, BT532_CLEAR_INT_STATUS_CMD);
			udelay(DELAY_FOR_SIGNAL_DELAY);
		}

		goto out;
	}

	info->work_state = NORMAL;

	if (ts_read_coord(info) == false || info->touch_info.status == 0xffff
		|| info->touch_info.status == 0x1) { /* maybe desirable reset */
		dev_err(&client->dev, "Failed to read info coord\n");
		bt532_power_control(info, POWER_OFF);
		bt532_power_control(info, POWER_ON_SEQUENCE);

		clear_report_data(info);
		mini_init_touch(info);

		goto out;
	}

	/* invalid : maybe periodical repeated int. */
	if (info->touch_info.status == 0x0)
		goto out;

	reported = false;

	if (zinitix_bit_test(info->touch_info.status, BIT_ICON_EVENT)) {
		if (read_data(info->client, BT532_ICON_STATUS_REG,
			(u8 *)(&info->icon_event_reg), 2) < 0) {
			dev_err(&client->dev, "Failed to read button info\n");
			write_cmd(client, BT532_CLEAR_INT_STATUS_CMD);

			goto out;
		}

		for (i = 0; i < info->cap_info.button_num; i++) {
			if (zinitix_bit_test(info->icon_event_reg,
						(BIT_O_ICON0_DOWN + i))) {
				info->button[i] = ICON_BUTTON_DOWN;
				input_report_key(info->input_dev, BUTTON_MAPPING_KEY[i], 1);
				reported = true;
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				dev_info(&client->dev, "Button down = %d\n", i);
#else
				dev_info(&client->dev, "Button down\n");
#endif
			}
		}

		for (i = 0; i < info->cap_info.button_num; i++) {
			if (zinitix_bit_test(info->icon_event_reg,
						(BIT_O_ICON0_UP + i))) {
				info->button[i] = ICON_BUTTON_UP;
				input_report_key(info->input_dev, BUTTON_MAPPING_KEY[i], 0);
				reported = true;
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				dev_info(&client->dev, "Button up = %d\n", i);
#else
				dev_info(&client->dev, "Button up\n");
#endif
			}
		}
	}

	/* if button press or up event occured... */
	if (reported == true ||
			!zinitix_bit_test(info->touch_info.status, BIT_PT_EXIST)) {
		for (i = 0; i < info->cap_info.multi_fingers; i++) {
			prev_sub_status = info->reported_touch_info.coord[i].sub_status;
			if (zinitix_bit_test(prev_sub_status, SUB_BIT_EXIST)) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				dev_info(&client->dev, "Finger [%02d] up\n", i);
#else
				dev_info(&client->dev, "Finger up\n");
#endif
				input_mt_slot(info->input_dev, i);
				input_mt_report_slot_state(info->input_dev,
							MT_TOOL_FINGER, 0);
#ifdef TSP_BOOSTER
				info->touch_pressed_num--;
#endif
			}
		}
		memset(&info->reported_touch_info, 0x0, sizeof(struct point_info));
		input_sync(info->input_dev);

		if (reported == true) /* for button event */
			udelay(100);

#ifdef TSP_BOOSTER
		goto touch_booster_out;
#else
		goto out;
#endif
	}
#ifdef SUPPORTED_PALM_TOUCH
	if (zinitix_bit_test(info->touch_info.status, BIT_PALM)) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		dev_info(&client->dev, "Palm report\n");
#endif
		palm = 1;
	}

	if (zinitix_bit_test(info->touch_info.status, BIT_PALM_REJECT)) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		dev_info(&client->dev, "Palm reject\n");
#endif
		palm = 2;
	}
#endif
	for (i = 0; i < info->cap_info.multi_fingers; i++) {
		sub_status = info->touch_info.coord[i].sub_status;
		prev_sub_status = info->reported_touch_info.coord[i].sub_status;

		if (zinitix_bit_test(sub_status, SUB_BIT_EXIST)) {
			x = info->touch_info.coord[i].x;
			y = info->touch_info.coord[i].y;
			w = info->touch_info.coord[i].width;

			 /* transformation from touch to screen orientation */
			if (pdata->orientation & TOUCH_V_FLIP)
				y = info->cap_info.MaxY
					+ info->cap_info.MinY - y;

			if (pdata->orientation & TOUCH_H_FLIP)
				x = info->cap_info.MaxX
					+ info->cap_info.MinX - x;

			maxX = info->cap_info.MaxX;
			maxY = info->cap_info.MaxY;

			if (pdata->orientation & TOUCH_XY_SWAP) {
				zinitix_swap_v(x, y, tmp);
				zinitix_swap_v(maxX, maxY, tmp);
			}

			if (x > maxX || y > maxY) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				dev_err(&client->dev,
							"Invalid coord %d : x=%d, y=%d\n", i, x, y);
#endif
				continue;
			}

			info->touch_info.coord[i].x = x;
			info->touch_info.coord[i].y = y;
			if (zinitix_bit_test(sub_status, SUB_BIT_DOWN)) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				dev_info(&client->dev, "Finger [%02d] x = %d, y = %d,"
						" w = %d\n", i, x, y, w);
#else
				dev_info(&client->dev, "Finger down\n");
#endif
#ifdef TSP_BOOSTER
				info->touch_pressed_num++;
				touch_press = true;
#endif
			}
			if (w == 0)
				w = 1;

			input_mt_slot(info->input_dev, i);
			input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, 1);

#if (TOUCH_POINT_MODE == 2)
			if (palm == 0) {
				if (w >= PALM_REPORT_WIDTH)
					w = PALM_REPORT_WIDTH - 10;
			} else if (palm == 1) {	//palm report
				w = PALM_REPORT_WIDTH;
//				info->touch_info.coord[i].minor_width = PALM_REPORT_WIDTH;
			} else if (palm == 2){	// palm reject
//				x = y = 0;
				w = PALM_REJECT_WIDTH;
//				info->touch_info.coord[i].minor_width = PALM_REJECT_WIDTH;
			}
#endif

			input_report_abs(info->input_dev, ABS_MT_TOUCH_MAJOR, (u32)w);
			input_report_abs(info->input_dev, ABS_MT_PRESSURE, (u32)w);
			input_report_abs(info->input_dev, ABS_MT_WIDTH_MAJOR,
					(u32)((palm == 1) ? w-40 : w));
#if (TOUCH_POINT_MODE == 2)
			input_report_abs(info->input_dev,
				ABS_MT_TOUCH_MINOR, (u32)info->touch_info.coord[i].minor_width);
//			input_report_abs(info->input_dev,
//				ABS_MT_WIDTH_MINOR, (u32)info->touch_info.coord[i].minor_width);
#ifdef SUPPORTED_PALM_TOUCH
			input_report_abs(info->input_dev, ABS_MT_PALM, (palm > 0)?1:0);
#endif
//			input_report_abs(info->input_dev, ABS_MT_PALM, 1);
#endif

			input_report_abs(info->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(info->input_dev, ABS_MT_POSITION_Y, y);
		} else if (zinitix_bit_test(sub_status, SUB_BIT_UP) ||
			zinitix_bit_test(prev_sub_status, SUB_BIT_EXIST)) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			dev_info(&client->dev, "Finger [%02d] up\n", i);
#else
			dev_info(&client->dev, "Finger up\n");
#endif
			memset(&info->touch_info.coord[i], 0x0, sizeof(struct coord));
			input_mt_slot(info->input_dev, i);
			input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, 0);
#ifdef TSP_BOOSTER
			info->touch_pressed_num--;
#endif
		} else {
			memset(&info->touch_info.coord[i], 0x0, sizeof(struct coord));
		}
	}
	memcpy((char *)&info->reported_touch_info, (char *)&info->touch_info,
		sizeof(struct point_info));
	input_sync(info->input_dev);

#ifdef TSP_BOOSTER
touch_booster_out:
	if (!!info->touch_pressed_num){
		if(touch_press)
			set_dvfs_lock(info, 1, true);
	}
	else{
		set_dvfs_lock(info, 0, false);
	}
#endif

out:
	if (info->work_state == NORMAL) {
#if ESD_TIMER_INTERVAL
		esd_timer_start(CHECK_ESD_TIMER, info);
#endif
		info->work_state = NOTHING;
	}

	up(&info->work_lock);

	return IRQ_HANDLED;
}

#if defined(CONFIG_PM)
static int bt532_ts_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bt532_ts_info *info = i2c_get_clientdata(client);

	if(info->device_enabled)
		return 0;
	info->device_enabled = 1;
#if defined(TSP_VERBOSE_DEBUG)
	dev_info(&client->dev, "resume++\n");
#endif
	down(&info->work_lock);
	if (info->work_state != SUSPEND) {
		dev_err(&client->dev, "%s: Invalid work proceedure (%d)\n",
				__func__, info->work_state);
		up(&info->work_lock);

		return 0;
	}

	bt532_power_control(info, POWER_ON_SEQUENCE);

	info->work_state = NOTHING;
	if (mini_init_touch(info) == false)
		dev_err(&client->dev, "Failed to resume\n");
	enable_irq(info->irq);

#if defined(TSP_VERBOSE_DEBUG)
	dev_info(&client->dev, "resume--\n");
#endif
	up(&info->work_lock);

	return 0;
}

static int bt532_ts_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bt532_ts_info *info = i2c_get_clientdata(client);

	if(!info->device_enabled)
		return 0;
	info->device_enabled = 0;
#if defined(TSP_VERBOSE_DEBUG)
	dev_info(&client->dev, "suspend++\n");
#endif
	disable_irq(info->irq);

#if ESD_TIMER_INTERVAL
	flush_work(&info->tmr_work);
#endif

	down(&info->work_lock);
	if (info->work_state != NOTHING
		&& info->work_state != SUSPEND) {
		dev_err(&client->dev, "%s: Invalid work proceedure (%d)\n",
			__func__, info->work_state);
		up(&info->work_lock);

		enable_irq(info->irq);
		return 0;
	}

	clear_report_data(info);

#if ESD_TIMER_INTERVAL
	esd_timer_stop(info);
#if defined(TSP_VERBOSE_DEBUG)
	dev_info(&client->dev, "Stopped esd timer\n");
#endif
#endif
	write_cmd(info->client, BT532_SLEEP_CMD);
	bt532_power_control(info, POWER_OFF);
	info->work_state = SUSPEND;

#if defined(TSP_VERBOSE_DEBUG)
	zinitix_printk("suspend--\n");
#endif
	up(&info->work_lock);

	return 0;
}
#endif

static int bt532_input_open(struct input_dev *dev)
{
	struct bt532_ts_info *info;

	pr_info("[TSP] %s\n", __func__);
	info = input_get_drvdata(dev);
	return bt532_ts_resume(&info->client->dev);
}
static void bt532_input_close(struct input_dev *dev)
{
	struct bt532_ts_info *info;

	pr_info("[TSP] %s\n", __func__);
	info = input_get_drvdata(dev);
	bt532_ts_suspend(&info->client->dev);
}
static bool ts_set_touchmode(u16 value)
{
	int i;
#ifndef ZINITIX_NEW_TA_COVER_REGISTER_0x11F
	u16	reg_val;
#endif

	disable_irq(misc_info->irq);

	down(&misc_info->work_lock);
	if (misc_info->work_state != NOTHING) {
		printk(KERN_INFO "other process occupied.. (%d)\n",
			misc_info->work_state);
		enable_irq(misc_info->irq);
		up(&misc_info->work_lock);
		return -1;
	}

	misc_info->work_state = SET_MODE;

	if (value == TOUCH_DND_MODE) {
#ifndef ZINITIX_NEW_TA_COVER_REGISTER_0x11F
		read_data(misc_info->client, 0x011e,	(u8 *)&reg_val, 2);
		zinitix_bit_clr(reg_val, 1);
		write_reg(misc_info->client, 0x011e,	reg_val);
#endif

		if (write_reg(misc_info->client, BT532_DND_SHIFT_VALUE,
						SEC_DND_SHIFT_VALUE) != I2C_SUCCESS)
			dev_err(&misc_info->client->dev, "Failed to set DND_SHIFT_VALUE\n");
		if (write_reg(misc_info->client, BT532_DND_N_COUNT,
			SEC_DND_N_COUNT) != I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : "
					"Fail to set BT532_DND_N_COUNT %d.\n", SEC_DND_N_COUNT);
		if (write_reg(misc_info->client, BT532_DND_U_COUNT,
			SEC_DND_U_COUNT) != I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : "
					"Fail to set BT532_DND_U_COUNT %d.\n", SEC_DND_U_COUNT);
		if (write_reg(misc_info->client, BT532_AFE_FREQUENCY,
			SEC_DND_FREQUENCY) != I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : "
					"Fail to set BT532_AFE_FREQUENCY %d.\n", SEC_DND_FREQUENCY);
	}
	if (value == TOUCH_PDND_MODE) {
#ifndef ZINITIX_NEW_TA_COVER_REGISTER_0x11F
		read_data(misc_info->client, 0x011e,	(u8 *)&reg_val, 2);
		zinitix_bit_clr(reg_val, 1);
		write_reg(misc_info->client, 0x011e,	reg_val);
#endif
		if (write_reg(misc_info->client, BT532_DND_N_COUNT,
			SEC_PDND_N_COUNT) != I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : "
					"Fail to set BT532_DND_N_COUNT %d.\n", SEC_PDND_N_COUNT);
		if (write_reg(misc_info->client, BT532_DND_U_COUNT,
			SEC_PDND_U_COUNT) != I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : "
					"Fail to set BT532_DND_U_COUNT %d.\n", SEC_PDND_U_COUNT);
		if (write_reg(misc_info->client, BT532_AFE_FREQUENCY,
			SEC_PDND_FREQUENCY) != I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : "
					"Fail to set BT532_AFE_FREQUENCY %d.\n", SEC_PDND_FREQUENCY);
	}
	else if(misc_info->touch_mode == TOUCH_DND_MODE || misc_info->touch_mode == TOUCH_PDND_MODE) {
#ifndef ZINITIX_NEW_TA_COVER_REGISTER_0x11F
		bt532_set_ta_status(misc_info);
#endif

		if (write_reg(misc_info->client, BT532_DND_SHIFT_VALUE,
						misc_info->cap_info.shift_value) != I2C_SUCCESS)
			dev_err(&misc_info->client->dev, "Failed to set DND_SHIFT_VALUE\n");
		if (write_reg(misc_info->client, BT532_DND_N_COUNT,
			misc_info->cap_info.N_cnt) != I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : "
				"Fail to reset BT532_AFE_FREQUENCY %d.\n",
				misc_info->cap_info.N_cnt);
		if (write_reg(misc_info->client, BT532_DND_U_COUNT,
			misc_info->cap_info.u_cnt) != I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : "
				"Fail to reset BT532_DND_U_COUNT %d.\n",
				misc_info->cap_info.u_cnt);
		if (write_reg(misc_info->client, BT532_AFE_FREQUENCY,
			misc_info->cap_info.afe_frequency) != I2C_SUCCESS)
			printk(KERN_INFO "[zinitix_touch] TEST Mode : "
				"Fail to reset BT532_AFE_FREQUENCY %d.\n",
				misc_info->cap_info.afe_frequency);
	}

	if (value == TOUCH_SEC_MODE)
		misc_info->touch_mode = TOUCH_POINT_MODE;
	else
		misc_info->touch_mode = value;

	printk(KERN_INFO "[zinitix_touch] tsp_set_testmode, "
		"touchkey_testmode = %d\r\n", misc_info->touch_mode);

	if (misc_info->touch_mode != TOUCH_POINT_MODE) {
		if (write_reg(misc_info->client, BT532_DELAY_RAW_FOR_HOST,
			RAWDATA_DELAY_FOR_HOST) != I2C_SUCCESS)
			zinitix_printk("Fail to set BT532_DELAY_RAW_FOR_HOST.\r\n");
	}

	if (write_reg(misc_info->client, BT532_TOUCH_MODE,
			misc_info->touch_mode) != I2C_SUCCESS)
		printk(KERN_INFO "[zinitix_touch] TEST Mode : "
				"Fail to set ZINITX_TOUCH_MODE %d.\r\n", misc_info->touch_mode);

	/* clear garbage data */
	for (i = 0; i < 10; i++) {
		mdelay(20);
		write_cmd(misc_info->client, BT532_CLEAR_INT_STATUS_CMD);
	}

	misc_info->work_state = NOTHING;
	enable_irq(misc_info->irq);
	up(&misc_info->work_lock);
	return 1;
}

static int ts_upgrade_sequence(const u8 *firmware_data)
{
	disable_irq(misc_info->irq);
	down(&misc_info->work_lock);
	misc_info->work_state = UPGRADE;

#if ESD_TIMER_INTERVAL
	esd_timer_stop(misc_info);
#endif
	zinitix_debug_msg("clear all reported points\r\n");
	clear_report_data(misc_info);

	printk(KERN_INFO "start upgrade firmware\n");
	if (ts_upgrade_firmware(misc_info,
		firmware_data,
		misc_info->cap_info.ic_fw_size) == false) {
		enable_irq(misc_info->irq);
		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return -1;
	}

	if (init_touch(misc_info) == false) {
		enable_irq(misc_info->irq);
		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return -1;
	}

#if ESD_TIMER_INTERVAL
	esd_timer_start(CHECK_ESD_TIMER, misc_info);
#if defined(TSP_VERBOSE_DEBUG)
	dev_info(&misc_info->client->dev, "Started esd timer\n");
#endif
#endif

	enable_irq(misc_info->irq);
	misc_info->work_state = NOTHING;
	up(&misc_info->work_lock);
	return 0;
}

#ifdef SEC_FACTORY_TEST
static inline void set_cmd_result(struct bt532_ts_info *info, char *buff, int len)
{
	strncat(info->factory_info->cmd_result, buff, len);
}

static inline void set_default_result(struct bt532_ts_info *info)
{
	char delim = ':';
	memset(info->factory_info->cmd_result, 0x00, ARRAY_SIZE(info->factory_info->cmd_result));
	memcpy(info->factory_info->cmd_result, info->factory_info->cmd, strlen(info->factory_info->cmd));
	strncat(info->factory_info->cmd_result, &delim, 1);
}

#define MAX_FW_PATH 255
#define TSP_FW_FILENAME "zinitix_fw.bin"

static void fw_update(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	int ret = 0;
	const u8 *buff = 0;
	mm_segment_t old_fs = {0};
	struct file *fp = NULL;
	long fsize = 0, nread = 0;
	char fw_path[MAX_FW_PATH + 1];
	char result[16] = {0};

	set_default_result(info);

	switch (info->factory_info->cmd_param[0]) {
	case BUILT_IN:
		ret = ts_upgrade_sequence((u8 *)m_firmware_data);
		if(ret<0) {
			info->factory_info->cmd_state = 3;
			return;
		}
		break;

	case UMS:
		old_fs = get_fs();
		set_fs(get_ds());

		snprintf(fw_path, MAX_FW_PATH, "/sdcard/%s", TSP_FW_FILENAME);
		fp = filp_open(fw_path, O_RDONLY, 0);
		if (IS_ERR(fp)) {
			dev_err(&client->dev,
				"file %s open error:%d\n", fw_path, (s32)fp);
			info->factory_info->cmd_state = 3;
			goto err_open;
		}

		fsize = fp->f_path.dentry->d_inode->i_size;

		if (fsize != info->cap_info.ic_fw_size) {
			dev_err(&client->dev, "invalid fw size!!\n");
			info->factory_info->cmd_state = 3;
			goto err_open;
		}

		buff = kzalloc((size_t)fsize, GFP_KERNEL);
		if (!buff) {
			dev_err(&client->dev, "failed to alloc buffer for fw\n");
			info->factory_info->cmd_state = 3;
			goto err_alloc;
		}

		nread = vfs_read(fp, (char __user *)buff, fsize, &fp->f_pos);
		if (nread != fsize) {
			info->factory_info->cmd_state = 3;
			goto err_fw_size;
		}

		filp_close(fp, current->files);
		set_fs(old_fs);
		dev_info(&client->dev, "ums fw is loaded!!\n");

		ret = ts_upgrade_sequence((u8 *)buff);
		if(ret<0) {
			kfree(buff);
			info->factory_info->cmd_state = 3;
			return;
		}
		break;

	default:
		dev_err(&client->dev, "invalid fw file type!!\n");
		goto not_support;
	}

	info->factory_info->cmd_state = 2;
	snprintf(result, sizeof(result) , "%s", "OK");
	set_cmd_result(info, result,
			strnlen(result, sizeof(result)));

if (fp != NULL) {
err_fw_size:
	kfree(buff);
err_alloc:
	filp_close(fp, NULL);
err_open:
	set_fs(old_fs);
}
not_support:
	snprintf(result, sizeof(result) , "%s", "NG");
	set_cmd_result(info, result, strnlen(result, sizeof(result)));
	return;
}

static void get_fw_ver_bin(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;
	u16 fw_version, fw_minor_version, reg_version, hw_id, vendor_id;
	u32 version, length;

	set_default_result(info);

	/* To Do */
	/* modify m_firmware_data */
	fw_version = (u16)(m_firmware_data[52] | (m_firmware_data[53] << 8));
	fw_minor_version = (u16)(m_firmware_data[56] | (m_firmware_data[57] << 8));
	reg_version = (u16)(m_firmware_data[60] | (m_firmware_data[61] << 8));
	hw_id = (u16)(m_firmware_data[0x6b12] | (m_firmware_data[0x6b13] << 8));
	vendor_id = ntohs(*(u16 *)&m_firmware_data[0x6b22]);
	version = (u32)((u32)(hw_id & 0xff) << 16) | ((fw_version & 0xf ) << 12)
				| ((fw_minor_version & 0xf) << 8) | (reg_version & 0xff);
#if !defined(CONFIG_SEC_DEGAS_PROJECT)
        config_version = version;
#endif
	length = sizeof(vendor_id);
	snprintf(finfo->cmd_buff, length + 1, "%s", (u8 *)&vendor_id);
	snprintf(finfo->cmd_buff + length, sizeof(finfo->cmd_buff) - length,
		"%06X", version);
	set_cmd_result(info, finfo->cmd_buff,
			strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void get_fw_ver_ic(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;
	u16 fw_version, fw_minor_version, reg_version, hw_id, vendor_id;
	u32 version, length;

	set_default_result(info);

	fw_version = info->cap_info.fw_version;
	fw_minor_version = info->cap_info.fw_minor_version;
	reg_version = info->cap_info.reg_data_version;
	hw_id = info->cap_info.hw_id;
	vendor_id = ntohs(info->cap_info.vendor_id);
	version = (u32)((u32)(hw_id & 0xff) << 16) | ((fw_version & 0xf) << 12)
		| ((fw_minor_version & 0xf) << 8) | (reg_version & 0xff);

	length = sizeof(vendor_id);
	snprintf(finfo->cmd_buff, length + 1, "%s", (u8 *)&vendor_id);
	snprintf(finfo->cmd_buff + length, sizeof(finfo->cmd_buff) - length,
				"%06X", version);
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void get_threshold(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;

	set_default_result(info);

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff),
		"%d", info->cap_info.threshold);
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void module_off_master(void *device_data)
{
	return;
}

static void module_on_master(void *device_data)
{
	return;
}

static void module_off_slave(void *device_data)
{
	return;
}

static void module_on_slave(void *device_data)
{
	return;
}

static void get_module_vendor(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *fdata = info->factory_info;
	char buff[16] = {0};
	int val,val2;

	set_default_result(info);
	if (!(gpio_get_value(info->pdata->tsp_en_gpio)) ) {
		dev_err(&client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		fdata->cmd_state = NOT_APPLICABLE;
		return;
	}
	if (info->pdata->tsp_vendor1 > 0 && info->pdata->tsp_vendor2 > 0 ) {
		gpio_tlmm_config(GPIO_CFG(info->pdata->tsp_vendor1, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
		gpio_tlmm_config(GPIO_CFG(info->pdata->tsp_vendor2, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
		val = gpio_get_value(info->pdata->tsp_vendor1);
		val2 = gpio_get_value(info->pdata->tsp_vendor2);		
		dev_info(&info->client->dev,
			"%s: TSP_ID: %d[%d]%d[%d]\n", __func__,
			info->pdata->tsp_vendor1, val,info->pdata->tsp_vendor2, val2);
		snprintf(buff, sizeof(buff), "%s,%d%d", tostring(OK), val,val2);
		fdata->cmd_state = OK;
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		return;
	}
	snprintf(buff, sizeof(buff),  "%s", tostring(NG));
	fdata->cmd_state = FAIL;
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
}


#define BT532_VENDOR_NAME "ZINITIX"

static void get_chip_vendor(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;

	set_default_result(info);

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff),
		"%s", BT532_VENDOR_NAME);
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}
static void get_config_ver(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;
        const char *model_name = info->pdata->pname;

	set_default_result(info);
#if !defined(CONFIG_SEC_DEGAS_PROJECT)
	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff),"%s_ZI_%x", model_name,config_version);
#else
	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff),"%s_ZI_%s", model_name,CONFIG_DATE);
#endif
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

#if defined(USE_ACTIVE_REPORT_RATE)	
static void report_rate(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;
	int report_rate_cmd;

	if((info->work_state == SUSPEND) || (info->work_state == PROBE)) {
		dev_err(&client->dev, "%s: probe is not done or suspend mode work_state : %d \n",
				__func__,info->work_state);		
		return;	
	}

	set_default_result(info);	

	if ((finfo->cmd_param[0] < 0) ||
		(finfo->cmd_param[0] > 2)) {
		dev_err(&client->dev, "%s: parameter %d is wrong\n",
			__func__, finfo->cmd_param[0]);
		return;
	}	
	report_rate_cmd = finfo->cmd_param[0];
	ts_lpm_level_set(report_rate_cmd);
	
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));	
}
#endif

#define BT532_CHIP_NAME "BT532"

static void get_chip_name(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;

	set_default_result(info);

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%s", BT532_CHIP_NAME);
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void get_x_num(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;

	set_default_result(info);

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff),
		"%u", info->cap_info.x_node_num);
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void get_y_num(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;

	set_default_result(info);

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff),
		"%u", info->cap_info.y_node_num);
	set_cmd_result(info, finfo->cmd_buff,
			strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void not_support_cmd(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;

	set_default_result(info);

	sprintf(finfo->cmd_buff, "%s", "NA");
	set_cmd_result(info, finfo->cmd_buff,
			strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	info->factory_info->cmd_state = NOT_APPLICABLE;

	dev_info(&client->dev, "%s: \"%s(%d)\"\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void run_reference_read(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;
	struct tsp_raw_data *raw_data = info->raw_data;
	u32 min, max;
	s32 i, j;

	set_default_result(info);

	ts_set_touchmode(TOUCH_DND_MODE);
	get_raw_data(info, (u8 *)raw_data->ref_data, 10);
	ts_set_touchmode(TOUCH_POINT_MODE);

	min = raw_data->ref_data[0];
	max = raw_data->ref_data[0];

	for(i = 0; i < info->cap_info.x_node_num; i++)
	{
		for(j = 0; j < info->cap_info.y_node_num; j++)
		{
			/*pr_info("ref_data : %d ",
			raw_data->ref_data[i * info->cap_info.y_node_num + j]);*/

			if (raw_data->ref_data[i * info->cap_info.y_node_num + j] < min &&
				raw_data->ref_data[i * info->cap_info.y_node_num + j] != 0)
				min = raw_data->ref_data[i * info->cap_info.y_node_num + j];

			if (raw_data->ref_data[i * info->cap_info.y_node_num + j] > max)
				max = raw_data->ref_data[i * info->cap_info.y_node_num + j];

		}
		/*pr_info("\n");*/
	}

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%d,%d\n", min, max);
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: \"%s\"(%d)\n", __func__, finfo->cmd_buff,
		strlen(finfo->cmd_buff));

	return;
}

static bool run_reference_PDiff_read(void *device_data, const u16 h_diff[][17], const u16 v_diff[][18], int bHidden1, int bMenu, int bBack, int bHidden2 ) //DND
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	int i, j, diff_val, pre_val, next_val, x_num, y_num, nButton;
	bool pass = true;
	int buttons[4] = {bHidden1,bMenu, bBack, bHidden2};
#if 1
	char print_diff[29*18]={0,};
#endif

	ts_set_touchmode(TOUCH_PDND_MODE);
	get_raw_data(info, (u8 *)info->dnd_data, 10);
	ts_set_touchmode(TOUCH_POINT_MODE);

	x_num = info->cap_info.x_node_num;
	y_num = info->cap_info.y_node_num;

	printk("%s : ++++++ DND SPEC +++++++++\n",__func__);
	for(i = 0; i < x_num; i++)
	{
		printk("%s : ",__func__);
		for(j = 0; j < y_num; j++)
		{
			printk("%5d ", info->dnd_data[i*y_num+j]);
		}
	printk("\n");
	}
	printk("%s : ------- DND SPEC ----------\n",__func__);
	printk("%s : TSP Diff test scale factor = %d\n", __func__, info->ref_scale_factor);
	printk("%s : H Diff start\n",__func__);
	//H DIff
	info->hdiff_max_x = info->hdiff_max_y = info->hdiff_min_x = info->hdiff_min_y = 0;
	info->hdiff_max_val =-32768;
	info->hdiff_min_val =32767;

	for(i = 0; i < x_num - 1; i++)
	{
		for(j = 0; j <y_num-1; j++)
		{
			//printk("%d ", info->dnd_data[i*info->cap_info.y_node_num+j]);
			next_val = info->dnd_data[(i*y_num)+(j+1)];
			pre_val = info->dnd_data[(i*y_num)+j];
			diff_val = (next_val > pre_val)?(next_val - pre_val):(pre_val - next_val);

			pre_val = (info->ref_scale_factor == 100)?((s16)h_diff[i][j]):((s16)(((s32)h_diff[i][j] * info->ref_scale_factor) /100));
			if(diff_val > pre_val)
				pass = false;
#if 1
			print_diff[i*y_num+j] = diff_val;
#endif
			if(info->hdiff_max_val < diff_val-pre_val) {
				info->hdiff_max_val = diff_val-pre_val;
				info->hdiff_max_x = i;
				info->hdiff_max_y= j+1;
			}
			if(info->hdiff_min_val > diff_val - pre_val) {
				info->hdiff_min_val =  diff_val - pre_val;
				info->hdiff_min_x = i;
				info->hdiff_min_y= j+1;
			}
		}
	//printk("\n");
	}
#if defined(PDIFF_DEBUG)
	printk("%s : ++++++ h_diff SPEC +++++++++\n",__func__);
	for(i = 0; i < x_num-1; i++){
		printk("%s : ",__func__);
		for(j = 0; j < y_num-1; j++)
			printk("%5d ",(u16)h_diff[i][j]);
		printk("\n");
	}
	printk("%s : ------- h_diff SPEC ----------\n",__func__);
	printk("%s : ++++++ calculated h_diff SPEC +++++++++\n",__func__);
	for(i = 0; i < x_num-1; i++){
		printk("%s : ",__func__);
		for(j = 0; j < y_num-1; j++)
			printk("%5d ",print_diff[i*y_num+j]);
		printk("\n");
	}
	printk("%s : ------- calculated h_diff SPEC ----------\n",__func__);
#endif
	printk("%s :",__func__);
	if(pass)
		printk("H Diff pass\n");
	else
		printk("H Diff fail\n");

	printk("%s : V Diff start\n",__func__);
	info->hdiff_max_x = info->hdiff_max_y = info->hdiff_min_x = info->hdiff_min_y = 0;
	info->vdiff_max_val =-32768;
	info->vdiff_min_val =32767;
	//V DIff  View
	for(i=0; i < x_num-2; i++)
	{
		for(j=0; j<y_num; j++)
		{
			//printk("%d ", info->dnd_data[i*info->cap_info.y_node_num+j]);
			next_val = info->dnd_data[(i*y_num)+j];
			pre_val = info->dnd_data[(i*y_num)+j+y_num];
			diff_val = (next_val > pre_val)?(next_val - pre_val):(pre_val - next_val);

			pre_val = (info->ref_scale_factor == 100)?((s16)v_diff[i][j]):((s16)(((s32)v_diff[i][j] * info->ref_scale_factor) /100));
			if(diff_val > pre_val)
				pass = false;
#if 1
			print_diff[i*y_num+j] = diff_val;
#endif
			if(info->vdiff_max_val < diff_val - pre_val) {
				info->vdiff_max_val = diff_val -pre_val;
				info->vdiff_max_x = i;
				info->vdiff_max_y= j+1;
			}
			if(info->vdiff_min_val > diff_val - pre_val) {
				info->vdiff_min_val = diff_val - pre_val;
				info->vdiff_min_x = i;
				info->vdiff_min_y= j+1;
			}
		}
	//printk("\n");
	}
	printk("%s : ",__func__);
	if(pass)
		printk("V Diff view pass\n");
	else
		printk("V Diff view fail\n");

#if 1
	printk(" : ++++++ v_diff SPEC +++++++++\n");
	for(i=0; i < x_num-2; i++){
		printk("%s : ",__func__);
		for(j=0; j < y_num; j++)
			printk("%5d ",(u16)v_diff[i][j]);
		printk("\n");
	}
	printk("%s : ------- v_diff SPEC ----------\n",__func__);
	printk("%s : ++++++ calculated v_diff SPEC +++++++++\n",__func__);
	for(i=0; i < x_num-2; i++){
		printk("%s : ",__func__);
		for(j=0; j < y_num; j++)
			printk("%5d ",print_diff[i*y_num+j]);
		printk("\n");
	}
	printk("%s : ------- calculated v_diff SPEC ----------\n",__func__);
#endif

	//V DIff  button
	if ( info->cap_info.button_num)
	{
		printk("%s : TSP Button scale = %d\n", __func__, info->ref_scale_factor);
		printk("%s : TSP Button Diff Spec. = %d %d %d %d\n", __func__,
			v_diff[x_num-2][buttons[0]], v_diff[x_num-2][buttons[1]],
			v_diff[x_num-2][buttons[2]], v_diff[x_num-2][buttons[3]]);

		for(i = 0; i < 4; i++)
		{
			nButton = buttons[i];
			if(nButton < 0)
				continue;
			next_val = info->dnd_data[(x_num-1)*y_num+nButton];
			pre_val = info->dnd_data[(x_num-2)*y_num+nButton];
			diff_val = (next_val > pre_val)?(next_val - pre_val):(pre_val - next_val);

			pre_val = (info->ref_scale_factor == 100)?( (s16)v_diff[x_num-2][nButton]+info->ref_btn_option): \
												( (s16)(((s32)v_diff[x_num-2][nButton]*info->ref_scale_factor)/100)+info->ref_btn_option);

			if(diff_val > pre_val) {
			pass = false;
				if(info->vdiff_max_val < diff_val - pre_val) {
					info->vdiff_max_val = diff_val - pre_val;
					info->vdiff_max_x = x_num - 1;
					info->vdiff_max_y= nButton;
			}
				if(info->vdiff_min_val  >diff_val - pre_val) {
					info->vdiff_min_val = diff_val-pre_val;
					info->vdiff_min_x = x_num - 1;
					info->vdiff_min_y= nButton;
			}
		}
#if defined(PDIFF_DEBUG)
			printk("%s : ",__func__);
			printk("buttons[%d]'s diff_val is %d\n", i, diff_val);
#endif
			}
			}
	printk("%s : ",__func__);
	if(pass)
		printk("Button Diff pass\n");
	else
		printk("Button Diff fail\n");

	return pass;
}

static void run_reference_PDiff(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct tsp_factory_info *finfo = info->factory_info;
	char buff[16] = {0};
	bool	check;

	finfo->cmd_state = 1;
	set_default_result(info);

	printk("run_ref_diff check hw_id = 0x%04X\n", info->cap_info.hw_id);

#ifdef NOT_SUPPORTED_TOUCH_DUMMY_KEY
	check = run_reference_PDiff_read(device_data, dnd_h_diff_00, dnd_v_diff_00, -1, 4, 13, -1);
#else
	check = run_reference_PDiff_read(device_data, dnd_h_diff_00, dnd_v_diff_00, 2, 4, 13, 15);
#endif
	/*
	if( info->cap_info.hw_id == TSP_HW_ID_INDEX_0) {  // Check TSP FW ID
		printk("HW_ID_INDEX_0\n");
		check = run_reference_PDiff_read(device_data, dnd_h_diff_00, dnd_v_diff_00, 2, 4, 13, 15);
	}
	else if( info->cap_info.hw_id == TSP_HW_ID_INDEX_1){	 // Check TSP FW ID
		printk("HW_ID_INDEX_1\n");
		check = run_reference_PDiff_read(device_data, dnd_h_diff_03, dnd_v_diff_03, 2, 4, 13, 15);
		}
	else{
			printk("Module ETC\n");
		check = run_reference_PDiff_read(device_data, dnd_h_diff_00, dnd_v_diff_00, 2, 4, 13, 15);
		}
	*/
	if(check) {
		printk("diff pass\n");
		sprintf(buff, "%s\n", "pass");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		finfo->cmd_state = 2;
	}
	else {
		printk("diff fail\n");
		sprintf(buff, "%s\n", "fail");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		finfo->cmd_state = 3;
	}

}

static void get_reference(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;
	struct tsp_raw_data *raw_data = info->raw_data;
	unsigned int val;
	int x_node, y_node;
	int node_num;

	set_default_result(info);

	x_node = finfo->cmd_param[0];
	y_node = finfo->cmd_param[1];

	if (x_node < 0 || x_node >= info->cap_info.x_node_num ||
		y_node < 0 || y_node >= info->cap_info.y_node_num) {
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%s", "abnormal");
		set_cmd_result(info, finfo->cmd_buff,
			strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
		info->factory_info->cmd_state = FAIL;
		return;
	}

	node_num = x_node * info->cap_info.y_node_num + y_node;

	val = raw_data->ref_data[node_num];
	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%u", val);
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void run_preference_read(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;
	struct tsp_raw_data *raw_data = info->raw_data;
	u16 min, max;
	s32 i, j;

	set_default_result(info);

	ts_set_touchmode(TOUCH_PDND_MODE);
	get_raw_data(info, (u8 *)raw_data->pref_data, 10);
	ts_set_touchmode(TOUCH_POINT_MODE);

	min = 0xFFFF;
	max = 0x0000;

	for(i = 0; i < info->cap_info.x_node_num; i++)
	{
		for(j = 0; j < info->cap_info.y_node_num; j++)
		{
			/*pr_info("pref_data : %d ",
					raw_data->pref_data[i * info->cap_info.y_node_num + j]);*/

			if (raw_data->pref_data[i * info->cap_info.y_node_num + j] < min &&
				raw_data->pref_data[i * info->cap_info.y_node_num + j] != 0)
				min = raw_data->pref_data[i * info->cap_info.y_node_num + j];

			if (raw_data->pref_data[i * info->cap_info.y_node_num + j] > max)
				max = raw_data->pref_data[i * info->cap_info.y_node_num + j];

		}
		/*pr_info("\n");*/
	}

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%d,%d\n", min, max);
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: \"%s\"(%d)\n", __func__, finfo->cmd_buff,
		strlen(finfo->cmd_buff));

	return;
}

static void get_preference(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;
	struct tsp_raw_data *raw_data = info->raw_data;
	unsigned int val;
	int x_node, y_node;
	int node_num;

	set_default_result(info);

	x_node = finfo->cmd_param[0];
	y_node = finfo->cmd_param[1];

	if (x_node < 0 || x_node >= info->cap_info.x_node_num ||
		y_node < 0 || y_node >= info->cap_info.y_node_num) {
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%s", "abnormal");
		set_cmd_result(info, finfo->cmd_buff,
			strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
		info->factory_info->cmd_state = FAIL;

		return;
	}

	node_num = x_node * info->cap_info.y_node_num + y_node;

	val = raw_data->pref_data[node_num];
	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%u", val);
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void run_delta_read(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;
	struct tsp_raw_data *raw_data = info->raw_data;
	s16 min, max;
	s32 i, j;

	set_default_result(info);

	ts_set_touchmode(TOUCH_DELTA_MODE);
	get_raw_data(info, (u8 *)(u8 *)raw_data->delta_data, 10);
	ts_set_touchmode(TOUCH_POINT_MODE);
	finfo->cmd_state = OK;

	min = (s16)0x7FFF;
	max = (s16)0x8000;

	for(i = 0; i < info->cap_info.x_node_num; i++)
	{
		for(j = 0; j < info->cap_info.y_node_num; j++)
		{
			/*printk("delta_data : %d \n", raw_data->delta_data[j+i]);*/

			if (raw_data->delta_data[i * info->cap_info.y_node_num + j] < min &&
				raw_data->delta_data[i * info->cap_info.y_node_num + j] != 0)
				min = raw_data->delta_data[i * info->cap_info.y_node_num + j];

			if (raw_data->delta_data[i * info->cap_info.y_node_num + j] > max)
				max = raw_data->delta_data[i * info->cap_info.y_node_num + j];

		}
		/*printk("\n");*/
	}

	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%d,%d\n", min, max);
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	finfo->cmd_state = OK;

	dev_info(&client->dev, "%s: \"%s\"(%d)\n", __func__, finfo->cmd_buff,
		strlen(finfo->cmd_buff));

	return;
}

static void get_delta(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;
	struct tsp_raw_data *raw_data = info->raw_data;
	unsigned int val;
	int x_node, y_node;
	int node_num;

	set_default_result(info);

	x_node = finfo->cmd_param[0];
	y_node = finfo->cmd_param[1];

	if (x_node < 0 || x_node >= info->cap_info.x_node_num ||
		y_node < 0 || y_node >= info->cap_info.y_node_num) {
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%s", "abnormal");
		set_cmd_result(info, finfo->cmd_buff,
			strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
		info->factory_info->cmd_state = FAIL;

		return;
	}

	node_num = x_node * info->cap_info.y_node_num + y_node;

	val = raw_data->delta_data[node_num];
	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%u", val);
	set_cmd_result(info, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	info->factory_info->cmd_state = OK;

	dev_info(&client->dev, "%s: %s(%d)\n", __func__, finfo->cmd_buff,
		strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));

	return;
}

static void cover_set(struct bt532_ts_info *info){
	if(g_cover_state == COVER_OPEN){
		zinitix_bit_clr(m_optional_mode, 2);
	} else if(g_cover_state == COVER_CLOSED) {
		zinitix_bit_set(m_optional_mode, 2);
	}
	if(info->work_state == SUSPEND || info->work_state == PROBE)
		return;
	bt532_set_optional_mode(info, true);
}

static void clear_cover_mode(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	struct tsp_factory_info *finfo = info->factory_info;
	int arg = finfo->cmd_param[0];

	set_default_result(info);
	snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "%u",
							(unsigned int) arg);

	g_cover_state = arg;

	cover_set(info);

	dev_info(&info->client->dev, "COVER state = %d\n", g_cover_state);
	set_cmd_result(info, finfo->cmd_buff,
					strnlen(finfo->cmd_buff, sizeof(finfo->cmd_buff)));
	mutex_lock(&finfo->cmd_lock);
	finfo->cmd_is_running = false;
	mutex_unlock(&finfo->cmd_lock);

	info->factory_info->cmd_state = OK;

	return;
}
/*
static void run_intensity_read(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;

	set_default_result(info);

	ts_set_touchmode(TOUCH_DND_MODE);
	get_raw_data(info, (u8 *)info->dnd_data, 10);
	ts_set_touchmode(TOUCH_POINT_MODE);

	//////test////////////////////////////////////////////////////
	int i,j;

	for(i=0; i<30; i++)
	{
		for (j = 0; j < 18; j++)
			printk("[TSP] info->dnd_data : %d ", info->dnd_data[j+i]);

		printk("\n");
	}
	//////test////////////////////////////////////////////////////

	info->factory_info->cmd_state = 2;
}

static void get_normal(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	char buff[16] = {0};
	unsigned int val;
	int x_node, y_node;
	int node_num;

	set_default_result(info);

	x_node = info->factory_info->cmd_param[0];
	y_node = info->factory_info->cmd_param[1];

	if (x_node < 0 || x_node > info->cap_info.x_node_num ||
		y_node < 0 || y_node > info->cap_info.y_node_num) {
		snprintf(buff, sizeof(buff), "%s", "abnormal");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->factory_info->cmd_state = 3;
		return;
	}

	node_num = x_node*info->cap_info.x_node_num + y_node;

	val = info->normal_data[node_num];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->factory_info->cmd_state = 2;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
				buff, strnlen(buff, sizeof(buff)));
}

static void get_tkey_delta(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	char buff[16] = {0};
	u16 val;
	int btn_node;
	int ret;

	set_default_result(info);

	btn_node = info->factory_info->cmd_param[0];

	if (btn_node < 0 || btn_node > MAX_SUPPORTED_BUTTON_NUM)
		goto err_out;

	disable_irq(misc_info->irq);
	down(&misc_info->work_lock);
	if (misc_info->work_state != NOTHING) {
		printk(KERN_INFO "other process occupied.. (%d)\n",
			misc_info->work_state);
		enable_irq(misc_info->irq);
		up(&misc_info->work_lock);
		goto err_out;
	}
	misc_info->work_state = SET_MODE;

	ret = read_data(misc_info->client, BT532_BTN_WIDTH + btn_node, (u8*)&val, 2);

	if (ret < 0) {
		printk(KERN_INFO "read error..\n");
		enable_irq(misc_info->irq);
		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		goto err_out;
	}
	misc_info->work_state = NOTHING;
	enable_irq(misc_info->irq);
	up(&misc_info->work_lock);

	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->factory_info->cmd_state = 2;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
				buff, strnlen(buff, sizeof(buff)));
	return;

err_out:
	snprintf(buff, sizeof(buff), "%s", "abnormal");
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->factory_info->cmd_state = 3;
}

static void get_intensity(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	char buff[16] = {0};
	unsigned int val;
	int x_node, y_node;
	int node_num;

	set_default_result(info);

	x_node = info->factory_info->cmd_param[0];
	y_node = info->factory_info->cmd_param[1];

	if (x_node < 0 || x_node > info->cap_info.x_node_num ||
		y_node < 0 || y_node > info->cap_info.y_node_num) {
		snprintf(buff, sizeof(buff), "%s", "abnormal");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->factory_info->cmd_state = 3;
		return;
	}

	node_num = x_node*info->cap_info.x_node_num + y_node;

	val = info->dnd_data[node_num];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->factory_info->cmd_state = 2;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
				buff, strnlen(buff, sizeof(buff)));
}

static void run_normal_read(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;

	set_default_result(info);

	ts_set_touchmode(TOUCH_NORMAL_MODE);
	get_raw_data(info, (u8 *)info->normal_data, 10);
	ts_set_touchmode(TOUCH_POINT_MODE);

	info->factory_info->cmd_state = 2;
}

static void get_key_threshold(void *device_data)
{
	struct bt532_ts_info *info = (struct bt532_ts_info *)device_data;
	int ret = 0;
	u16 threshold;
	char buff[16] = {0};

	set_default_result(info);

	ret = read_data(misc_info->client, BT532_BUTTON_SENSITIVITY, (u8*)&threshold, 2);

	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "failed");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->factory_info->cmd_state = 3;
		return;
	}

	snprintf(buff, sizeof(buff), "%u", threshold);

	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->factory_info->cmd_state = 2;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}
*/

static ssize_t store_cmd(struct device *dev, struct device_attribute
				  *devattr, const char *buf, size_t count)
{
	struct bt532_ts_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;
	char *cur, *start, *end;
	char buff[TSP_CMD_STR_LEN] = {0};
	int len, i;
	struct tsp_cmd *tsp_cmd_ptr = NULL;
	char delim = ',';
	bool cmd_found = false;
	int param_cnt = 0;

	if (strlen(buf) >= TSP_CMD_STR_LEN) {
		dev_err(&client->dev, "%s: cmd length is over(%s,%d)!!\n", __func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	if (finfo->cmd_is_running == true) {
		dev_err(&client->dev, "%s: other cmd is running\n", __func__);
		goto err_out;
	}

	/* check lock  */
	mutex_lock(&finfo->cmd_lock);
	finfo->cmd_is_running = true;
	mutex_unlock(&finfo->cmd_lock);

	finfo->cmd_state = RUNNING;

	for (i = 0; i < ARRAY_SIZE(finfo->cmd_param); i++)
		finfo->cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;

	memset(finfo->cmd, 0x00, ARRAY_SIZE(finfo->cmd));
	memcpy(finfo->cmd, buf, len);

	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(buff, buf, cur - buf);
	else
		memcpy(buff, buf, len);

	/* find command */
	list_for_each_entry(tsp_cmd_ptr, &finfo->cmd_list_head, list) {
		if (!strcmp(buff, tsp_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(tsp_cmd_ptr, &finfo->cmd_list_head, list) {
			if (!strcmp("not_support_cmd", tsp_cmd_ptr->cmd_name))
				break;
		}
	}

	/* parsing parameters */
	if (cur && cmd_found) {
		cur++;
		start = cur;
		memset(buff, 0x00, ARRAY_SIZE(buff));
		do {
			if (*cur == delim || cur - buf == len) {
				end = cur;
				memcpy(buff, start, end - start);
				*(buff + strlen(buff)) = '\0';
				finfo->cmd_param[param_cnt] =
					(int)simple_strtol(buff, NULL, 10);
				start = cur + 1;
				memset(buff, 0x00, ARRAY_SIZE(buff));
				param_cnt++;
			}
			cur++;
		} while ((cur - buf <= len) && (param_cnt < TSP_CMD_PARAM_NUM));
	}

	dev_info(&client->dev, "cmd = %s\n", tsp_cmd_ptr->cmd_name);
/*	for (i = 0; i < param_cnt; i++)
		dev_info(&client->dev, "cmd param %d= %d\n", i, finfo->cmd_param[i]);*/

	tsp_cmd_ptr->cmd_func(info);

err_out:
	return count;
}

static ssize_t show_cmd_status(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct bt532_ts_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;

	dev_info(&client->dev, "tsp cmd: status:%d\n", finfo->cmd_state);

	if (finfo->cmd_state == WAITING)
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "WAITING");

	else if (finfo->cmd_state == RUNNING)
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "RUNNING");

	else if (finfo->cmd_state == OK)
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "OK");

	else if (finfo->cmd_state == FAIL)
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "FAIL");

	else if (finfo->cmd_state == NOT_APPLICABLE)
		snprintf(finfo->cmd_buff, sizeof(finfo->cmd_buff), "NOT_APPLICABLE");

	return snprintf(buf, sizeof(finfo->cmd_buff),
					"%s\n", finfo->cmd_buff);
}

static ssize_t show_cmd_result(struct device *dev, struct device_attribute
				    *devattr, char *buf)
{
	struct bt532_ts_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	struct tsp_factory_info *finfo = info->factory_info;

	dev_info(&client->dev, "tsp cmd: result: %s\n", finfo->cmd_result);

	mutex_lock(&finfo->cmd_lock);
	finfo->cmd_is_running = false;
	mutex_unlock(&finfo->cmd_lock);

	finfo->cmd_state = WAITING;

	return snprintf(buf, sizeof(finfo->cmd_result),
					"%s\n", finfo->cmd_result);
}

static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, store_cmd);
static DEVICE_ATTR(cmd_status, S_IRUGO, show_cmd_status, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, show_cmd_result, NULL);

static struct attribute *touchscreen_attributes[] = {
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
	NULL,
};

static struct attribute_group touchscreen_attr_group = {
	.attrs = touchscreen_attributes,
};

#ifdef SUPPORTED_TOUCH_KEY
static ssize_t show_touchkey_threshold(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct bt532_ts_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	struct capa_info *cap = &(info->cap_info);

#ifdef NOT_SUPPORTED_TOUCH_DUMMY_KEY
	dev_info(&client->dev, "%s: key threshold = %d\n", __func__,
			cap->key_threshold);

	return snprintf(buf, 41, "%d", cap->key_threshold);
#else
	dev_info(&client->dev, "%s: key threshold = %d %d %d %d\n", __func__,
			cap->dummy_threshold, cap->key_threshold, cap->key_threshold, cap->dummy_threshold);

	return snprintf(buf, 41, "%d %d %d %d", cap->dummy_threshold,
			cap->key_threshold, cap->key_threshold,
			cap->dummy_threshold);
#endif
}
#if 0
static ssize_t enable_dummy_key(struct device *dev,
			struct device_attribute *attr, char *buf, size_t count)
{
	static char enable = '0';
	struct bt532_ts_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;

	if (!strcmp(buf, ""))
		count = sprintf(buf, "%c", enable);
	else {
		if ((buf[0] - '0' <= 1) && count == 2)
			enable = *buf;
		else {
			dev_err(&client->dev, "%s: Invalid parameter\n", __func__);

			goto err_out;
		}
	}

	dev_info(&client->dev, "%s: Extra button event %c\n", __func__, enable);

	return count;

err_out:
	return sprintf(buf, "NG");

	return 0;
}
#endif

static ssize_t show_touchkey_sensitivity(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct bt532_ts_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	u16 val = 0;
	int ret;
	int i;

#ifdef NOT_SUPPORTED_TOUCH_DUMMY_KEY
	if (!strcmp(attr->attr.name, "touchkey_recent"))
		i = 0;
	else if (!strcmp(attr->attr.name, "touchkey_back"))
		i = 1;
	else {
		dev_err(&client->dev, "%s: Invalid attribute\n",__func__);

		goto err_out;
	}

#else
	if (!strcmp(attr->attr.name, "touchkey_dummy_btn1"))
		i = 0;
	else if (!strcmp(attr->attr.name, "touchkey_recent"))
		i = 1;
	else if (!strcmp(attr->attr.name, "touchkey_back"))
		i = 2;
	else if (!strcmp(attr->attr.name, "touchkey_dummy_btn4"))
		i = 3;
	else if (!strcmp(attr->attr.name, "touchkey_dummy_btn5"))
		i = 4;
	else if (!strcmp(attr->attr.name, "touchkey_dummy_btn6"))
		i = 5;
	else {
		dev_err(&client->dev, "%s: Invalid attribute\n", __func__);

		goto err_out;
	}
#endif
	ret = read_data(client, BT532_BTN_WIDTH + i, (u8 *)&val, 2);
	if (ret < 0) {
		dev_err(&client->dev, "%s:Failed to read %d's key sensitivity\n",
					 __func__, i);

		goto err_out;
	}

	dev_info(&client->dev, "%s: %d's key sensitivity = %d\n",
				__func__, i, val);

	return snprintf(buf, 6, "%d", val);

err_out:
	return sprintf(buf, "NG");
}

static ssize_t show_back_key_raw_data(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t show_menu_key_raw_data(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}
/*
static ssize_t show_back_key_idac_data(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t show_menu_key_idac_data(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}
*/
#if SUPPORTED_TOUCH_KEY_LED
static ssize_t touch_led_control(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct bt532_ts_info *info = dev_get_drvdata(dev);
	u8 data;

	sscanf(buf, "%hhu", &data);

	printk("[TKEY] %s : %d _ %d\n",__func__,data,__LINE__);

	if(data == 1)
		gpio_direction_output(info->pdata->gpio_keyled, 1);
	else
		gpio_direction_output(info->pdata->gpio_keyled, 0);

	return size;
}
#endif

static DEVICE_ATTR(touchkey_threshold, S_IRUGO, show_touchkey_threshold, NULL);
/*static DEVICE_ATTR(touch_sensitivity, S_IRUGO, back_key_state_show, NULL);*/
//static DEVICE_ATTR(extra_button_event, S_IWUSR | S_IWGRP | S_IRUGO, NULL, enable_dummy_key );
//static DEVICE_ATTR(touchkey_menu, S_IRUGO, show_touchkey_sensitivity, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, show_touchkey_sensitivity, NULL);
static DEVICE_ATTR(touchkey_recent, S_IRUGO, show_touchkey_sensitivity, NULL);
#ifndef NOT_SUPPORTED_TOUCH_DUMMY_KEY
static DEVICE_ATTR(touchkey_dummy_btn1, S_IRUGO,
					show_touchkey_sensitivity, NULL);
static DEVICE_ATTR(touchkey_dummy_btn3, S_IRUGO,
					show_touchkey_sensitivity, NULL);
static DEVICE_ATTR(touchkey_dummy_btn4, S_IRUGO,
					show_touchkey_sensitivity, NULL);
static DEVICE_ATTR(touchkey_dummy_btn6, S_IRUGO,
					show_touchkey_sensitivity, NULL);
#endif
/*static DEVICE_ATTR(autocal_stat, S_IRUGO, show_autocal_status, NULL);*/
static DEVICE_ATTR(touchkey_raw_back, S_IRUGO, show_back_key_raw_data, NULL);
static DEVICE_ATTR(touchkey_raw_menu, S_IRUGO, show_menu_key_raw_data, NULL);
/*static DEVICE_ATTR(touchkey_idac_back, S_IRUGO, show_back_key_idac_data, NULL);
static DEVICE_ATTR(touchkey_idac_menu, S_IRUGO, show_menu_key_idac_data, NULL);*/
#if SUPPORTED_TOUCH_KEY_LED
static DEVICE_ATTR(brightness, 0664, NULL, touch_led_control);
#endif

static struct attribute *touchkey_attributes[] = {
	&dev_attr_touchkey_threshold.attr,
	/*&dev_attr_touch_sensitivity.attr,*/
	&dev_attr_touchkey_back.attr,
//	&dev_attr_touchkey_menu.attr,
	&dev_attr_touchkey_recent.attr,
	//&dev_attr_autocal_stat.attr,
	//&dev_attr_extra_button_event.attr,
	&dev_attr_touchkey_raw_menu.attr,
	&dev_attr_touchkey_raw_back.attr,
#ifndef NOT_SUPPORTED_TOUCH_DUMMY_KEY
	&dev_attr_touchkey_dummy_btn1.attr,
	&dev_attr_touchkey_dummy_btn3.attr,
	&dev_attr_touchkey_dummy_btn4.attr,
	&dev_attr_touchkey_dummy_btn6.attr,
#endif
	//&dev_attr_touchkey_idac_back.attr,
	//&dev_attr_touchkey_idac_menu.attr,
#if SUPPORTED_TOUCH_KEY_LED	
	&dev_attr_brightness.attr,
#endif	
	NULL,
};
static struct attribute_group touchkey_attr_group = {
	.attrs = touchkey_attributes,
};
#endif

static int init_sec_factory(struct bt532_ts_info *info)
{
	struct device *factory_ts_dev;
#ifdef SUPPORTED_TOUCH_KEY
	struct device *factory_tk_dev;
#endif
	struct tsp_factory_info *factory_info;
	struct tsp_raw_data *raw_data;
	int ret;
	int i;

	factory_info = kzalloc(sizeof(struct tsp_factory_info), GFP_KERNEL);
	if (unlikely(!factory_info)) {
		dev_err(&info->client->dev, "%s: Failed to allocate memory\n",
				__func__);
		ret = -ENOMEM;

		goto err_alloc1;
	}
	raw_data = kzalloc(sizeof(struct tsp_raw_data), GFP_KERNEL);
	if (unlikely(!raw_data)) {
		dev_err(&info->client->dev, "%s: Failed to allocate memory\n",
				__func__);
		ret = -ENOMEM;

		goto err_alloc2;
	}

	INIT_LIST_HEAD(&factory_info->cmd_list_head);
	for (i = 0; i < ARRAY_SIZE(tsp_cmds); i++)
		list_add_tail(&tsp_cmds[i].list, &factory_info->cmd_list_head);

	factory_ts_dev = device_create(sec_class, NULL, 0, info, "tsp");
	if (unlikely(!factory_ts_dev)) {
		dev_err(&info->client->dev, "Failed to create factory dev\n");
		ret = -ENODEV;
		goto err_create_device;
	}

#ifdef SUPPORTED_TOUCH_KEY
	factory_tk_dev = device_create(sec_class, NULL, 0, info, "sec_touchkey");
	if (IS_ERR(factory_tk_dev)) {
		dev_err(&info->client->dev, "Failed to create factory dev\n");
		ret = -ENODEV;
		goto err_create_device;
	}
#endif

	ret = sysfs_create_group(&factory_ts_dev->kobj, &touchscreen_attr_group);
	if (unlikely(ret)) {
		dev_err(&info->client->dev, "Failed to create touchscreen sysfs group\n");
		goto err_create_sysfs;
	}

	ret = sysfs_create_link(&factory_ts_dev->kobj, &info->input_dev->dev.kobj, "input");
	if (ret < 0) {
		dev_err(&info->client->dev, "Failed to create input symbolic link\n");
	}

#ifdef SUPPORTED_TOUCH_KEY
	ret = sysfs_create_group(&factory_tk_dev->kobj, &touchkey_attr_group);
	if (unlikely(ret)) {
		dev_err(&info->client->dev, "Failed to create touchkey sysfs group\n");
		goto err_create_sysfs;
	}
#endif

	mutex_init(&factory_info->cmd_lock);
	factory_info->cmd_is_running = false;

	info->factory_info = factory_info;
	info->raw_data = raw_data;

	return ret;

err_create_sysfs:
err_create_device:
	kfree(raw_data);
err_alloc2:
	kfree(factory_info);
err_alloc1:

	return ret;
}
#endif

static int ts_misc_fops_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int ts_misc_fops_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static long ts_misc_fops_ioctl(struct file *filp,
	unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct raw_ioctl raw_ioctl;
	u8 *u8Data;
	int ret = 0;
	size_t sz = 0;
	u16 version;
	u16 mode;

	struct reg_ioctl reg_ioctl;
	u16 val;
	int nval = 0;

	if (misc_info == NULL)
	{
		zinitix_debug_msg("misc device NULL?\n");
		return -1;
	}

	switch (cmd) {

	case TOUCH_IOCTL_GET_DEBUGMSG_STATE:
		ret = m_ts_debug_mode;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_SET_DEBUGMSG_STATE:
		if (copy_from_user(&nval, argp, 4)) {
			pr_info("[zinitix_touch] error : copy_from_user\n");
			return -1;
		}
		if (nval)
			pr_info("[zinitix_touch] on debug mode (%d)\n",
				nval);
		else
			pr_info("[zinitix_touch] off debug mode (%d)\n",
				nval);
		m_ts_debug_mode = nval;
		break;

	case TOUCH_IOCTL_GET_CHIP_REVISION:
		ret = misc_info->cap_info.ic_revision;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_FW_VERSION:
		ret = misc_info->cap_info.fw_version;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_REG_DATA_VERSION:
		ret = misc_info->cap_info.reg_data_version;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_VARIFY_UPGRADE_SIZE:
		if (copy_from_user(&sz, argp, sizeof(size_t)))
			return -1;

		printk(KERN_INFO "[zinitix_touch]: firmware size = %d\r\n", sz);
		if (misc_info->cap_info.ic_fw_size != sz) {
			pr_info("[zinitix_touch]: firmware size error\r\n");
			return -1;
		}
		break;

	case TOUCH_IOCTL_VARIFY_UPGRADE_DATA:
		if (copy_from_user(m_firmware_data,
			argp, misc_info->cap_info.ic_fw_size))
			return -1;

		version = (u16) (m_firmware_data[52] | (m_firmware_data[53]<<8));

		pr_info("[zinitix_touch]: firmware version = %x\r\n", version);

		if (copy_to_user(argp, &version, sizeof(version)))
			return -1;
		break;

	case TOUCH_IOCTL_START_UPGRADE:
		return ts_upgrade_sequence((u8 *)m_firmware_data);

	case TOUCH_IOCTL_GET_X_RESOLUTION:
		ret = misc_info->pdata->x_resolution;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_Y_RESOLUTION:
		ret = misc_info->pdata->y_resolution;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_X_NODE_NUM:
		ret = misc_info->cap_info.x_node_num;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_Y_NODE_NUM:
		ret = misc_info->cap_info.y_node_num;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_GET_TOTAL_NODE_NUM:
		ret = misc_info->cap_info.total_node_num;
		if (copy_to_user(argp, &ret, sizeof(ret)))
			return -1;
		break;

	case TOUCH_IOCTL_HW_CALIBRAION:
		ret = -1;
		disable_irq(misc_info->irq);
		down(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			pr_info("[zinitix_touch]: other process occupied.. (%d)\r\n",
				misc_info->work_state);
			up(&misc_info->work_lock);
			return -1;
		}
		misc_info->work_state = HW_CALIBRAION;
		mdelay(100);

		/* h/w calibration */
		if (ts_hw_calibration(misc_info) == true)
			ret = 0;

		mode = misc_info->touch_mode;
		if (write_reg(misc_info->client,
			BT532_TOUCH_MODE, mode) != I2C_SUCCESS) {
			pr_err("[zinitix_touch]: failed to set touch mode %d.\n",
				mode);
			goto fail_hw_cal;
		}

		if (write_cmd(misc_info->client,
			BT532_SWRESET_CMD) != I2C_SUCCESS)
			goto fail_hw_cal;

		enable_irq(misc_info->irq);
		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return ret;
fail_hw_cal:
		enable_irq(misc_info->irq);
		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return -1;

	case TOUCH_IOCTL_SET_RAW_DATA_MODE:
		if (misc_info == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		if (copy_from_user(&nval, argp, 4)) {
			pr_info("[zinitix_touch] error : copy_from_user\r\n");
			misc_info->work_state = NOTHING;
			return -1;
		}
		ts_set_touchmode((u16)nval);

		return 0;

	case TOUCH_IOCTL_GET_REG:
		if (misc_info == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		down(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			pr_info("[zinitix_touch]:other process occupied.. (%d)\n",
				misc_info->work_state);
			up(&misc_info->work_lock);
			return -1;
		}

		misc_info->work_state = SET_MODE;

		if (copy_from_user(&reg_ioctl,
			argp, sizeof(struct reg_ioctl))) {
			misc_info->work_state = NOTHING;
			up(&misc_info->work_lock);
			pr_info("[zinitix_touch] error : copy_from_user\n");
			return -1;
		}

		if (read_data(misc_info->client,
			reg_ioctl.addr, (u8 *)&val, 2) < 0)
			ret = -1;

		nval = (int)val;

		if (copy_to_user(reg_ioctl.val, (u8 *)&nval, 4)) {
			misc_info->work_state = NOTHING;
			up(&misc_info->work_lock);
			pr_info("[zinitix_touch] error : copy_to_user\n");
			return -1;
		}

		zinitix_debug_msg("read : reg addr = 0x%x, val = 0x%x\n",
			reg_ioctl.addr, nval);

		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return ret;

	case TOUCH_IOCTL_SET_REG:

		down(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			pr_info("[zinitix_touch]: other process occupied.. (%d)\n",
				misc_info->work_state);
			up(&misc_info->work_lock);
			return -1;
		}

		misc_info->work_state = SET_MODE;
		if (copy_from_user(&reg_ioctl,
				argp, sizeof(struct reg_ioctl))) {
			misc_info->work_state = NOTHING;
			up(&misc_info->work_lock);
			pr_info("[zinitix_touch] error : copy_from_user\n");
			return -1;
		}

		if (copy_from_user(&val, reg_ioctl.val, 4)) {
			misc_info->work_state = NOTHING;
			up(&misc_info->work_lock);
			pr_info("[zinitix_touch] error : copy_from_user\n");
			return -1;
		}

		if (write_reg(misc_info->client,
			reg_ioctl.addr, val) != I2C_SUCCESS)
			ret = -1;

		zinitix_debug_msg("write : reg addr = 0x%x, val = 0x%x\r\n",
			reg_ioctl.addr, val);
		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return ret;

	case TOUCH_IOCTL_DONOT_TOUCH_EVENT:

		if (misc_info == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		down(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			pr_info("[zinitix_touch]: other process occupied.. (%d)\r\n",
				misc_info->work_state);
			up(&misc_info->work_lock);
			return -1;
		}

		misc_info->work_state = SET_MODE;
		if (write_reg(misc_info->client,
			BT532_INT_ENABLE_FLAG, 0) != I2C_SUCCESS)
			ret = -1;
		zinitix_debug_msg("write : reg addr = 0x%x, val = 0x0\r\n",
			BT532_INT_ENABLE_FLAG);

		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return ret;

	case TOUCH_IOCTL_SEND_SAVE_STATUS:
		if (misc_info == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}
		down(&misc_info->work_lock);
		if (misc_info->work_state != NOTHING) {
			pr_info("[zinitix_touch]: other process occupied.." \
				"(%d)\r\n", misc_info->work_state);
			up(&misc_info->work_lock);
			return -1;
		}
		misc_info->work_state = SET_MODE;
		ret = 0;
		write_reg(misc_info->client, 0xc003, 0x0001);
		write_reg(misc_info->client, 0xc104, 0x0001);
		if (write_cmd(misc_info->client,
			BT532_SAVE_STATUS_CMD) != I2C_SUCCESS)
			ret =  -1;

		mdelay(1000);	/* for fusing eeprom */
		write_reg(misc_info->client, 0xc003, 0x0000);
		write_reg(misc_info->client, 0xc104, 0x0000);

		misc_info->work_state = NOTHING;
		up(&misc_info->work_lock);
		return ret;

	case TOUCH_IOCTL_GET_RAW_DATA:
		if (misc_info == NULL) {
			zinitix_debug_msg("misc device NULL?\n");
			return -1;
		}

		if (misc_info->touch_mode == TOUCH_POINT_MODE)
			return -1;

		down(&misc_info->raw_data_lock);
		if (misc_info->update == 0) {
			up(&misc_info->raw_data_lock);
			return -2;
		}

		if (copy_from_user(&raw_ioctl,
			argp, sizeof(struct raw_ioctl))) {
			up(&misc_info->raw_data_lock);
			pr_info("[zinitix_touch] error : copy_from_user\r\n");
			return -1;
		}

		misc_info->update = 0;

		u8Data = (u8 *)&misc_info->cur_data[0];
		if (raw_ioctl.sz > MAX_TRAW_DATA_SZ*2)
			raw_ioctl.sz = MAX_TRAW_DATA_SZ*2;
		if (copy_to_user(raw_ioctl.buf, (u8 *)u8Data,
			raw_ioctl.sz)) {
			up(&misc_info->raw_data_lock);
			return -1;
		}

		up(&misc_info->raw_data_lock);
		return 0;

	default:
		break;
	}
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id zinitix_match_table[] = {
	{ .compatible = "zinitix,bt532_ts_device",},
	{},
};

static int zinitix_init_gpio(struct bt532_ts_platform_data *pdata)
{
	int ret = 0;

	ret = gpio_request(pdata->gpio_int, "zinitix_tsp_irq");
	if(ret) {
		pr_err("[TSP]%s: unable to request zinitix_tsp_irq [%d]\n",
			__func__, pdata->gpio_int);
		return ret;
	}

	gpio_tlmm_config(GPIO_CFG(pdata->gpio_int, 0,
		GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

#if !defined(CONFIG_SEC_MEGA23G_COMMON) && !defined(CONFIG_SEC_MEGA2LTE_COMMON) && !defined(CONFIG_SEC_VASTALTE_CHN_CMMCC_DUOS_PROJECT)
	ret = gpio_request(pdata->tsp_vendor1, "zinitix_tsp_vendor1");
	if (ret) {
		pr_err("[TSP]%s: unable to request zinitix_tsp_vendor1 [%d]\n",
				__func__, pdata->tsp_vendor1);
		return ret;
	}
	ret = gpio_request(pdata->tsp_vendor2, "zinitix_tsp_vendor2");
	if (ret) {
		pr_err("[TSP]%s: unable to request zinitix_tsp_vendor2 [%d]\n",
				__func__, pdata->tsp_vendor2);
		return ret;
	}
#endif

	if (pdata->tsp_supply_type == TSP_LDO_SUPPLY)
	{
		ret = gpio_request(pdata->tsp_en_gpio, "zinitix_tsp_en_gpio");
		if(ret) {
			pr_err("[TSP]%s: unable to request zinitix_tsp_en_gpio [%d]\n",
				__func__, pdata->tsp_en_gpio);
			return ret;
		}
		gpio_tlmm_config(GPIO_CFG(pdata->tsp_en_gpio, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}

#if SUPPORTED_TOUCH_KEY_LED
	if (pdata->gpio_keyled >= 0) {
		ret = gpio_request(pdata->gpio_keyled, "key_led_gpio");
		if (ret < 0) {
			pr_err("%s: gpio_request failed: %d\n", __func__, ret);
			return ret;
		}
	}
#endif
	return ret;
}
#endif
static int bt532_ts_probe_dt(struct device_node *np,
			 struct device *dev,
			 struct bt532_ts_platform_data *pdata)
{
#if SUPPORTED_TOUCH_KEY_LED
        int keyled_n = -1;
	int size_p;
#endif
	int ret = 0;
	u32 temp;

	ret = of_property_read_u32(np, "zinitix,x_resolution", &temp);
	if (ret) {
			dev_err(dev, "Unable to read controller version\n");
			return ret;
	} else
			pdata->x_resolution = (u16) temp;
	ret = of_property_read_u32(np, "zinitix,y_resolution", &temp);
	if (ret) {
			dev_err(dev, "Unable to read controller version\n");
			return ret;
	} else
			pdata->y_resolution = (u16) temp;
	ret = of_property_read_u32(np, "zinitix,page_size", &temp);
	if (ret) {
			dev_err(dev, "Unable to read controller version\n");
			return ret;
	} else
			pdata->page_size = (u16) temp;
	ret = of_property_read_u32(np, "zinitix,orientation", &temp);
	if (ret) {
			dev_err(dev, "Unable to read controller version\n");
			return ret;
	} else
			pdata->orientation = (u8) temp;

	pdata->tsp_vendor1 = of_get_named_gpio(np, "zinitix,vendor1", 0);
	pdata->tsp_vendor2 = of_get_named_gpio(np, "zinitix,vendor2", 0);

#if SUPPORTED_TOUCH_KEY_LED
	if (of_find_property(np, "keyled_gpio", &size_p)) {
		keyled_n = of_get_named_gpio(np, "keyled_gpio", 0);
		if (keyled_n < 0) {
			pr_err("%s: of_get_named_gpio failed: keyled_gpio %d\n", __func__,
				keyled_n);
			return -EINVAL;
		}
	}
	pdata->gpio_keyled = keyled_n;
#endif

	pdata->gpio_int = of_get_named_gpio(np, "zinitix,irq-gpio", 0);
	if (pdata->gpio_int < 0) {
		pr_err("%s: of_get_named_gpio failed: tsp_gpio %d\n", __func__,
			pdata->gpio_int);
		return -EINVAL;
	}
	ret = of_property_read_u32(np, "zinitix,tsp_vdd_supply_type",
				&pdata->tsp_supply_type);
	if (ret < 0) {
		pr_err("%s: failed to read property tsp_vdd_supply_type\n",
			__func__);
		return ret;
	}
	ret = of_property_read_string(np, "zinitix,pname",
				&pdata->pname);
	if (ret < 0) {
		pr_err("%s: failed to read property config_ver\n",
			__func__);
		return ret;
	}
	if (pdata->tsp_supply_type == TSP_LDO_SUPPLY) {
		pdata->tsp_en_gpio = of_get_named_gpio(np, "zinitix,vdd_en-gpio", 0);
		if (pdata->tsp_en_gpio < 0) {
			pr_err("%s: of_get_named_gpio failed: %d\n", __func__,
				pdata->tsp_en_gpio);
			return -EINVAL;
		}
	}
	return 0;

}
static int bt532_ts_probe(struct i2c_client *client,
		const struct i2c_device_id *i2c_id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct bt532_ts_platform_data *pdata = NULL;
	struct bt532_ts_info *info;
	struct input_dev *input_dev;
	int ret = 0;
	int i;

	struct device_node *np = client->dev.of_node;
	zinitix_printk("[TSP]: %s\n", __func__);

	if (client->dev.of_node) {
		if (!pdata) {
			pdata = devm_kzalloc(&client->dev,
					sizeof(*pdata), GFP_KERNEL);
			if (!pdata)
				return -ENOMEM;
		}
		ret = bt532_ts_probe_dt(np, &client->dev, pdata);
		if (ret){
			dev_err(&client->dev, "Error parsing dt %d\n", ret);
			goto err_no_platform_data;
		}
		ret = zinitix_init_gpio(pdata);
		if(ret < 0)
			goto err_gpio_request;

	} else if (!pdata) {
		dev_err(&client->dev, "%s: no platform data defined\n",
			__func__);
		return -EINVAL;
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "Not compatible i2c function\n");
		ret = -EIO;
		goto err_no_platform_data;
	}
#if 0
	if (pdata->tsp_supply_type == TSP_LDO_SUPPLY) {
		ret = gpio_request(pdata->tsp_en_gpio, "tsp_en_gpio");
		if (ret < 0) {
			pr_err("%s: gpio_request failed: %d\n", __func__, ret);
			goto err_gpio_request;
		}
	}
#endif
	info = kzalloc(sizeof(struct bt532_ts_info), GFP_KERNEL);
	if (!info) {
		dev_err(&client->dev, "%s: Failed to allocate memory\n", __func__);
		ret = -ENOMEM;
		goto err_mem_alloc;
	}

	i2c_set_clientdata(client, info);
	info->client = client;
	info->pdata = pdata;
	info->device_enabled = 1;

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "Failed to allocate input device\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	info->input_dev = input_dev;
	info->work_state = PROBE;
	info->vddo_vreg = regulator_get(&info->client->dev,"vddo");
	if (IS_ERR(info->vddo_vreg)){
		info->vddo_vreg = NULL;
		printk(KERN_INFO "info->vddo_vreg error\n");
		ret = -EPERM;
		goto err_power_sequence;
	}
	if (pdata->tsp_supply_type == TSP_REGULATOR_SUPPLY)
	{
		info->vdd_en = regulator_get(&info->client->dev,"vdd_en");
		if (IS_ERR(info->vdd_en)){
			info->vdd_en = NULL;
			printk(KERN_INFO "info->vdd_en error\n");
			ret = -EPERM;
			goto err_power_sequence;
		}
	}
	/* power on */
	if (bt532_power_control(info, POWER_ON_SEQUENCE) == false) {
		ret = -EPERM;
		goto err_power_sequence;
	}

	/* To Do */
	/* FW version read from tsp */

	memset(&info->reported_touch_info,
		0x0, sizeof(struct point_info));

	/* init touch mode */
	info->touch_mode = TOUCH_POINT_MODE;
	misc_info = info;

	bt532_firmware_check(pdata);

	if (init_touch(info) == false) {
		ret = -EPERM;
		goto err_input_register_device;
	}

	for (i = 0; i < MAX_SUPPORTED_BUTTON_NUM; i++)
		info->button[i] = ICON_BUTTON_UNCHANGE;

	snprintf(info->phys, sizeof(info->phys),
		"%s/input0", dev_name(&client->dev));
	input_dev->name = "sec_touchscreen";
	input_dev->id.bustype = BUS_I2C;
/*	input_dev->id.vendor = 0x0001; */
	input_dev->phys = info->phys;
/*	input_dev->id.product = 0x0002; */
/*	input_dev->id.version = 0x0100; */
	input_dev->dev.parent = &client->dev;

	set_bit(EV_SYN, info->input_dev->evbit);
	set_bit(EV_KEY, info->input_dev->evbit);
	set_bit(EV_ABS, info->input_dev->evbit);
#if SUPPORTED_TOUCH_KEY_LED
	set_bit(EV_LED, info->input_dev->evbit);
	set_bit(LED_MISC, info->input_dev->ledbit);
#endif
	set_bit(INPUT_PROP_DIRECT, info->input_dev->propbit);

	for (i = 0; i < MAX_SUPPORTED_BUTTON_NUM; i++)
		set_bit(BUTTON_MAPPING_KEY[i], info->input_dev->keybit);

	if (pdata->orientation & TOUCH_XY_SWAP) {
		input_set_abs_params(info->input_dev, ABS_MT_POSITION_Y,
			info->cap_info.MinX,
			info->cap_info.MaxX + ABS_PT_OFFSET,
			0, 0);
		input_set_abs_params(info->input_dev, ABS_MT_POSITION_X,
			info->cap_info.MinY,
			info->cap_info.MaxY + ABS_PT_OFFSET,
			0, 0);
	} else {
		input_set_abs_params(info->input_dev, ABS_MT_POSITION_X,
			info->cap_info.MinX,
			info->cap_info.MaxX + ABS_PT_OFFSET,
			0, 0);
		input_set_abs_params(info->input_dev, ABS_MT_POSITION_Y,
			info->cap_info.MinY,
			info->cap_info.MaxY + ABS_PT_OFFSET,
			0, 0);
	}

	input_set_abs_params(info->input_dev, ABS_MT_TOUCH_MAJOR,
		0, 255, 0, 0);
	input_set_abs_params(info->input_dev, ABS_MT_WIDTH_MAJOR,
		0, 255, 0, 0);

#if (TOUCH_POINT_MODE == 2)
	input_set_abs_params(info->input_dev, ABS_MT_TOUCH_MINOR,
		0, 255, 0, 0);
/*	input_set_abs_params(info->input_dev, ABS_MT_WIDTH_MINOR,
		0, 255, 0, 0); */
/*	input_set_abs_params(info->input_dev, ABS_MT_ORIENTATION,
		-128, 127, 0, 0);  */
	input_set_abs_params(info->input_dev, ABS_MT_PALM,
		0, 1, 0, 0);
#endif

	info->input_dev->open = bt532_input_open;
	info->input_dev->close = bt532_input_close;
	set_bit(MT_TOOL_FINGER, info->input_dev->keybit);
	input_mt_init_slots(info->input_dev, info->cap_info.multi_fingers);

	zinitix_debug_msg("register %s input device \r\n",
		info->input_dev->name);
	input_set_drvdata(info->input_dev, info);
	ret = input_register_device(info->input_dev);
	if (ret) {
		pr_err("unable to register %s input device\r\n",
			info->input_dev->name);
		goto err_input_register_device;
	}

	info->work_state = NOTHING;
	sema_init(&info->work_lock, 1);

#if ESD_TIMER_INTERVAL
	spin_lock_init(&info->lock);
	INIT_WORK(&info->tmr_work, ts_tmr_work);
	esd_tmr_workqueue =
		create_singlethread_workqueue("esd_tmr_workqueue");

	if (!esd_tmr_workqueue) {
		dev_err(&client->dev, "Failed to create esd tmr work queue\n");
		ret = -EPERM;

		goto err_input_register_device;
	}

	esd_timer_init(info);
	esd_timer_start(CHECK_ESD_TIMER, info);
#if defined(TSP_VERBOSE_DEBUG)
	dev_info(&client->dev, "Started esd timer\n");
#endif
#endif
#if 0
	info->irq = irq_of_parse_and_map(client->dev.of_node, 0);
	if (!info->irq) {
		dev_err(&client->dev, "Failed to retrieve IRQ from device tree.\n");
		ret = -ENODEV;
		goto err_irq_of_parse;
	}
#endif

#ifdef TSP_BOOSTER
	mutex_init(&info->dvfs_lock);
	info->touch_pressed_num = 0;
	info->dvfs_lock_status = false;
	info->boost_level = DVFS_STAGE_DUAL;
	INIT_DELAYED_WORK(&info->work_dvfs_off, set_dvfs_off);
	INIT_DELAYED_WORK(&info->work_dvfs_chg, change_dvfs_lock);
#endif

	/* configure irq */
	info->irq = gpio_to_irq(pdata->gpio_int);
	if (info->irq < 0){
		dev_err(&client->dev, "Invalid GPIO_TOUCH_IRQ\n");
		ret = -ENODEV;
		goto err_irq_of_parse;;
	}
	pdata->tsp_irq = info->irq;
	ret = request_threaded_irq(info->irq, NULL, bt532_touch_work,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT , BT532_TS_DEVICE, info);

	if (ret) {
		printk(KERN_ERR "unable to register irq.(%s)\r\n",
			info->input_dev->name);
		goto err_request_irq;
	}
	dev_info(&client->dev, "zinitix touch probe.\r\n");

#if defined(CONFIG_PM_RUNTIME)
	//pm_runtime_enable(&client->dev);
#endif

	sema_init(&info->raw_data_lock, 1);

	ret = misc_register(&touch_misc_device);
	if (ret) {
		dev_err(&client->dev, "Failed to register touch misc device\n");
		goto err_misc_register;
	}

#ifdef SEC_FACTORY_TEST
	ret = init_sec_factory(info);
	if (ret) {
		dev_err(&client->dev, "Failed to init sec factory device\n");

		goto err_kthread_create_failed;
	}
#endif

	//tsp_charger_status_cb = bt532_charger_status_cb;

	return 0;

#ifdef SEC_FACTORY_TEST
err_kthread_create_failed:
	kfree(info->factory_info);
	kfree(info->raw_data);
#endif
err_misc_register:
	free_irq(info->irq, info);
err_irq_of_parse:
err_request_irq:
	input_unregister_device(info->input_dev);
err_input_register_device:
	input_free_device(info->input_dev);
err_power_sequence:
err_alloc:
	kfree(info);
err_mem_alloc:
	gpio_free(pdata->tsp_en_gpio);
err_gpio_request:
#if SUPPORTED_TOUCH_KEY_LED
	if (pdata->gpio_keyled >= 0)
		gpio_free(pdata->gpio_keyled);
#endif
err_no_platform_data:
	if (IS_ENABLED(CONFIG_OF))
		devm_kfree(&client->dev, (void *)pdata);

	dev_info(&client->dev, "Failed to probe\n");
	return ret;
}

static int bt532_ts_remove(struct i2c_client *client)
{
	struct bt532_ts_info *info = i2c_get_clientdata(client);
	struct bt532_ts_platform_data *pdata = info->pdata;

	disable_irq(info->irq);
	down(&info->work_lock);

	info->work_state = REMOVE;

#ifdef SEC_FACTORY_TEST
	kfree(info->factory_info);
	kfree(info->raw_data);
#endif
#if ESD_TIMER_INTERVAL
	flush_work(&info->tmr_work);
	write_reg(info->client, BT532_PERIODICAL_INTERRUPT_INTERVAL, 0);
	esd_timer_stop(info);
#if defined(TSP_VERBOSE_DEBUG)
	dev_info(&client->dev, "Stopped esd timer\n");
#endif
	destroy_workqueue(esd_tmr_workqueue);
#endif

	if (info->irq)
		free_irq(info->irq, info);

	misc_deregister(&touch_misc_device);


	if (gpio_is_valid(pdata->gpio_int) != 0)
		gpio_free(pdata->gpio_int);
#if SUPPORTED_TOUCH_KEY_LED
	if (gpio_is_valid(pdata->gpio_keyled) != 0)
		gpio_free(pdata->gpio_keyled);
#endif
	if (pdata->tsp_supply_type == TSP_LDO_SUPPLY)
		if (gpio_is_valid(pdata->tsp_en_gpio) != 0)
			gpio_free(pdata->tsp_en_gpio);

	input_unregister_device(info->input_dev);
	input_free_device(info->input_dev);
	up(&info->work_lock);
	kfree(info);

	return 0;
}

void bt532_ts_shutdown(struct i2c_client *client)
{
	struct bt532_ts_info *info = i2c_get_clientdata(client);

	dev_info(&client->dev, "%s++\n", __func__);
	disable_irq(info->irq);
	down(&info->work_lock);
#if ESD_TIMER_INTERVAL
	flush_work(&info->tmr_work);
	esd_timer_stop(info);
#endif
	up(&info->work_lock);
	bt532_power_control(info, POWER_OFF);
	dev_info(&client->dev, "%s--\n", __func__);
}

static struct i2c_device_id bt532_idtable[] = {
	{BT532_TS_DEVICE, 0},
	{ }
};

/*#if defined(CONFIG_PM)
static const struct dev_pm_ops bt532_ts_pm_ops = {
	.suspend = bt532_ts_suspend,
	.resume = bt532_ts_resume,
};
#endif*/

static struct i2c_driver bt532_ts_driver = {
	.probe	= bt532_ts_probe,
	.remove	= bt532_ts_remove,
	.shutdown = bt532_ts_shutdown,
	.id_table	= bt532_idtable,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= BT532_TS_DEVICE,
/*#if defined(CONFIG_PM)
		.pm		= &bt532_ts_pm_ops,
#endif*/
	.of_match_table = zinitix_match_table,
	},
};

static int __init bt532_ts_init(void)
{
	pr_debug("[TSP]: %s\n", __func__);
	zinitix_printk("[TSP]: %s\n", __func__);

#ifdef CONFIG_SAMSUNG_LPM_MODE
	if (poweroff_charging) {
		pr_notice("%s : LPM Charging Mode!!\n", __func__);
		return 0;
	}
#endif

	return i2c_add_driver(&bt532_ts_driver);

}

static void __exit bt532_ts_exit(void)
{
	i2c_del_driver(&bt532_ts_driver);
}

module_init(bt532_ts_init);
module_exit(bt532_ts_exit);

MODULE_DESCRIPTION("touch-screen device driver using i2c interface");
MODULE_AUTHOR("<mika.kim@samsung.com>");
MODULE_LICENSE("GPL");
