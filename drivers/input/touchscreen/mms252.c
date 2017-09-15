/*
 * mms_ts.c - Touchscreen driver for Melfas MMS-series touch controllers
 *
 * Copyright (C) 2011 Google Inc.
 * Author: Dima Zavin <dima@android.com>
 *         Simon Wilson <simonwilson@google.com>
 *
 * ISP reflashing code based on original code from Melfas.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#define DEBUG
/* #define VERBOSE_DEBUG */
#define SEC_TSP_DEBUG
#define USE_OPEN_CLOSE
/* #define FORCE_FW_FLASH */
/* #define FORCE_FW_PASS */
/* #define ESD_DEBUG */
#define DEBUG_PRINT2			1

#define SEC_TSP_FACTORY_TEST
#define TSP_BUF_SIZE 1024
#define RAW_FAIL -1
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mfd/pm8xxx/gpio.h>
#include <linux/uaccess.h>
#include <linux/cpufreq.h>
#include <asm/mach-types.h>
#include <linux/delay.h>

#ifdef CONFIG_SEC_DVFS
#include <linux/cpufreq.h>
#define TOUCH_BOOSTER_DVFS

#define DVFS_STAGE_TRIPLE       3

#define DVFS_STAGE_DUAL         2
#define DVFS_STAGE_SINGLE       1
#define DVFS_STAGE_NONE         0
#endif

/* #include <mach/dev.h> */

#include <linux/regulator/consumer.h>
#include <linux/i2c/mms252.h>
#include <linux/of_gpio.h>

#include <asm/unaligned.h>

#define MAX_FINGERS		10
#define MAX_WIDTH		30
#define MAX_PRESSURE		255

/* Registers */
#define MMS_MODE_CONTROL	0x01
#define MMS_XYRES_HI		0x02
#define MMS_XRES_LO		0x03
#define MMS_YRES_LO		0x04

#define MMS_INPUT_EVENT_PKT_SZ	0x0F
#define MMS_INPUT_EVENT	0x10
#define EVENT_SZ	8

#define MMS_CORE_VERSION	0xE1
#define MMS_TSP_REVISION	0xF0
#define MMS_HW_REVISION		0xF1
#define MMS_COMPAT_GROUP	0xF2
#define MMS_FW_VERSION		0xF4

#ifdef SEC_TSP_FACTORY_TEST
#define TX_NUM		30
#define RX_NUM		20
#define NODE_NUM	620

/* self diagnostic */
#define ADDR_CH_NUM		0x0B
#define ADDR_UNIV_CMD	0xA0
#define CMD_ENTER_TEST	0x40
#define CMD_EXIT_TEST	0x4F
#define CMD_CM_DELTA	0x41
#define CMD_GET_DELTA	0x42
#define CMD_CM_ABS		0X43
#define CMD_GET_ABS		0X44
#define CMD_CM_JITTER	0X45
#define CMD_GET_JITTER	0X46
#define CMD_GET_INTEN	0x70
#define CMD_GET_INTEN_KEY	0x71
#define CMD_RESULT_SZ	0XAE
#define CMD_RESULT		0XAF

/* VSC(Vender Specific Command)  */
#define MMS_VSC_CMD			0xB0	/* vendor specific command */
#define MMS_VSC_MODE			0x1A	/* mode of vendor */

#define MMS_VSC_CMD_ENTER		0X01
#define MMS_VSC_CMD_CM_DELTA		0X02
#define MMS_VSC_CMD_CM_ABS		0X03
#define MMS_VSC_CMD_EXIT		0X05
#define MMS_VSC_CMD_INTENSITY		0X04
#define MMS_VSC_CMD_RAW			0X06
#define MMS_VSC_CMD_REFER		0X07

#define TSP_CMD_STR_LEN 32
#define TSP_CMD_RESULT_STR_LEN 512
#define TSP_CMD_PARAM_NUM 8
#define tostring(x) #x
#endif /* SEC_TSP_FACTORY_TEST */

/* START - Added to support API's for TSP tuning */
#define ESD_DETECT_COUNT		10
#define FINGER_EVENT_SZ			6
#define MAX_LOG_LENGTH			128
#define MMS_EVENT_PKT_SZ		0x0F

/* Universal commands */
#define MMS_CMD_SET_LOG_MODE		0x20

/* Event types */
#define MMS_LOG_EVENT			0xD
#define MMS_NOTIFY_EVENT		0xE
#define MMS_ERROR_EVENT			0xF
#define MMS_TOUCH_KEY_EVENT		0x40

#ifdef TOUCH_BOOSTER_DVFS
#define TOUCH_BOOSTER_OFF_TIME	500
#define TOUCH_BOOSTER_CHG_TIME	130
#endif

enum {
	GET_RX_NUM	= 1,
	GET_TX_NUM,
	GET_EVENT_DATA,
};

enum {
	LOG_TYPE_U08	= 2,
	LOG_TYPE_S08,
	LOG_TYPE_U16,
	LOG_TYPE_S16,
	LOG_TYPE_U32	= 8,
	LOG_TYPE_S32,
};
/* END - Added to support API's for TSP tuning */

extern int poweroff_charging;
struct device *sec_touchscreen;
struct device *sec_touchkey;
/* Touch booster */
int touch_is_pressed;
static int tsp_power_enabled;

/* panel info */
#define ILJIN 0x4
#define EELY 0x0
#define NO_PANEL 0x7
#define YONGFAST 0x8
#define WINTEC 0x9

/* ILJIN panel */
#define BOOT_VERSION_IJ 0x1
#define CORE_VERSION_IJ 0x73
#define FW_VERSION_IJ 0x17

/* EELY panel */
#define BOOT_VERSION_EL 0x1
#define CORE_VERSION_EL 0x78
#if defined(CONFIG_MACH_MILLET3G_CHN_OPEN)
#define FW_VERSION_DATE "140415"
#endif
#define FW_VERSION_EL 0x18

#define MAX_FW_PATH 255
#define TSP_FW_FILENAME "melfas_fw.bin"
#define MMS_COORDS_ARR_SIZE	4
#define SECTION_NUM		3

#define ISC_XFER_LEN		256
#define MMS_FLASH_PAGE_SZ	1024
#define ISC_BLOCK_NUM		(MMS_FLASH_PAGE_SZ / ISC_XFER_LEN)

#define MMS_CMD_ENTER_ISC	0x5F

#define FLASH_VERBOSE_DEBUG	1

enum {
	ISC_NONE = -1,
	ISC_SUCCESS = 0,
	ISC_FILE_OPEN_ERROR,
	ISC_FILE_CLOSE_ERROR,
	ISC_FILE_FORMAT_ERROR,
	ISC_WRITE_BUFFER_ERROR,
	ISC_I2C_ERROR,
	ISC_UPDATE_MODE_ENTER_ERROR,
	ISC_CRC_ERROR,
	ISC_VALIDATION_ERROR,
	ISC_COMPATIVILITY_ERROR,
	ISC_UPDATE_SECTION_ERROR,
	ISC_SLAVE_ERASE_ERROR,
	ISC_SLAVE_DOWNLOAD_ERROR,
	ISC_DOWNLOAD_WHEN_SLAVE_IS_UPDATED_ERROR,
	ISC_INITIAL_PACKET_ERROR,
	ISC_NO_NEED_UPDATE_ERROR,
	ISC_LIMIT
};

enum {
	SEC_NONE = -1,
	SEC_BOOTLOADER = 0,
	SEC_CORE,
	SEC_CONFIG,
	SEC_LIMIT
};

enum {
	ISC_ADDR = 0xD5,
	ISC_CMD_READ_STATUS = 0xD9,
	ISC_CMD_READ = 0x4000,
	ISC_CMD_EXIT = 0x8200,
	ISC_CMD_PAGE_ERASE = 0xC000,
	ISC_PAGE_ERASE_DONE = 0x10000,
	ISC_PAGE_ERASE_ENTER = 0x20000,
};

enum {
	EXT_INFO_ERASE = 0x01,
	EXT_INFO_WRITE = 0x10,
};

enum {
	BUILT_IN = 0,
	UMS,
};

struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *tsp_cb, bool mode);
};

struct mms_ts_info {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct melfas_tsi_platform_data *pdata;
	char phys[32];
	int max_x;
	int max_y;
	bool invert_x;
	bool invert_y;
	u8 palm_flag;
	int irq;
	int (*power) (struct mms_ts_info *info,int on);
	void (*input_event)(void *data);
#if TOUCHKEY
	int (*keyled) (struct mms_ts_info *info,int on);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif

#ifdef TOUCH_BOOSTER_DVFS
	struct delayed_work	work_dvfs_off;
	struct delayed_work	work_dvfs_chg;
	struct mutex		dvfs_lock;
	bool dvfs_lock_status;
	u8								finger_cnt1;
	int dvfs_boost_mode;
	int dvfs_freq;
	int dvfs_old_stauts;
	bool stay_awake;
#endif

#if TOUCHKEY
	bool touchkey[3];
	int keycode[3];
	bool led_cmd;
#if defined(SEC_TSP_FACTORY_TEST)
	int menu_s;
	int back_s;
#endif
#endif

	/* protects the enabled flag */
	struct mutex lock;
	bool enabled;

	void (*register_cb)(void *);
	struct tsp_callbacks callbacks;
	bool ta_status;
	bool noise_mode;
	bool threewave_mode;
	bool sleep_wakeup_ta_check;

#if defined(SEC_TSP_DEBUG)
	unsigned char finger_state[MAX_FINGERS];
#endif

	u8 fw_update_state;
	u8 panel;
	u8 fw_boot_ver;
	u8 fw_core_ver;
	u8 fw_ic_ver;
#if defined(SEC_TSP_FACTORY_TEST)
	struct list_head cmd_list_head;
	u8 cmd_state;
	char cmd[TSP_CMD_STR_LEN];
	int cmd_param[TSP_CMD_PARAM_NUM];
	char cmd_result[TSP_CMD_RESULT_STR_LEN];
	struct mutex cmd_lock;
	bool cmd_is_running;

	unsigned int reference[NODE_NUM];
	unsigned int raw[NODE_NUM]; /* CM_ABS */
	unsigned int inspection[NODE_NUM];/* CM_DELTA */
	unsigned int intensity[NODE_NUM];
	bool ft_flag;
#endif				/* SEC_TSP_FACTORY_TEST */

	struct cdev			cdev;
	dev_t				mms_dev;
	struct class			*class;

	struct mms_log_data {
		u8			*data;
		int			cmd;
	} log;
};

struct mms_bin_hdr {
	char tag[8];
	u16	core_version;
	u16	section_num;
	u16	contains_full_binary;
	u16	reserved0;

	u32	binary_offset;
	u32	binary_length;

	u32	extention_offset;
	u32	reserved1;
} __attribute__ ((packed));

struct mms_ext_hdr {
	u32	data_ID;
	u32	offset;
	u32	length;
	u32	next_item;
	u8	data[0];
} __attribute__ ((packed));

struct mms_fw_img {
	u16	type;
	u16	version;

	u16	start_page;
	u16	end_page;

	u32	offset;
	u32	length;
} __attribute__ ((packed));

struct isc_packet {
	u8	cmd;
	u32	addr;
	u8	data[0];
} __attribute__ ((packed));

struct mms_fw_image {
	__le32 hdr_len;
	__le32 data_len;
	__le32 fw_ver;
	__le32 hdr_ver;
	u8 data[0];
} __packed;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mms_ts_early_suspend(struct early_suspend *h);
static void mms_ts_late_resume(struct early_suspend *h);
#endif
static int mms_ts_resume(struct device *dev);
static int mms_ts_suspend(struct device *dev);

#if defined(SEC_TSP_FACTORY_TEST)
#define TSP_CMD(name, func) .cmd_name = name, .cmd_func = func

enum {
	WAITING = 0,
	RUNNING,
	OK,
	FAIL,
	NOT_APPLICABLE,
	NG,
};

struct tsp_cmd {
	struct list_head	list;
	const char	*cmd_name;
	void	(*cmd_func)(void *device_data);
};

extern unsigned int system_rev;

static void fw_update(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_config_ver(void *device_data);
static void get_threshold(void *device_data);
static void module_off_master(void *device_data);
static void module_on_master(void *device_data);
/*static void module_off_slave(void *device_data);
static void module_on_slave(void *device_data);*/
static void get_module_vendor(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void get_reference(void *device_data);
static void get_cm_abs(void *device_data);
static void get_cm_delta(void *device_data);
static void get_intensity(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void run_reference_read(void *device_data);
static void run_cm_abs_read(void *device_data);
static void run_cm_delta_read(void *device_data);
static void run_intensity_read(void *device_data);
static void not_support_cmd(void *device_data);

#ifdef TOUCH_BOOSTER_DVFS
static void boost_level(void *device_data);
#endif

struct tsp_cmd tsp_cmds[] = {
	{TSP_CMD("fw_update", fw_update),},
	{TSP_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{TSP_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{TSP_CMD("get_config_ver", get_config_ver),},
	{TSP_CMD("get_threshold", get_threshold),},
	{TSP_CMD("module_off_master", module_off_master),},
	{TSP_CMD("module_on_master", module_on_master),},
	{TSP_CMD("module_off_slave", not_support_cmd),},
	{TSP_CMD("module_on_slave", not_support_cmd),},
	{TSP_CMD("get_chip_vendor", get_chip_vendor),},
	{TSP_CMD("get_module_vendor", get_module_vendor),},
	{TSP_CMD("get_chip_name", get_chip_name),},
	{TSP_CMD("get_x_num", get_x_num),},
	{TSP_CMD("get_y_num", get_y_num),},
	{TSP_CMD("get_reference", get_reference),},
	{TSP_CMD("get_cm_abs", get_cm_abs),},
	{TSP_CMD("get_cm_delta", get_cm_delta),},
	{TSP_CMD("get_intensity", get_intensity),},
	{TSP_CMD("run_reference_read", run_reference_read),},
	{TSP_CMD("run_cm_abs_read", run_cm_abs_read),},
	{TSP_CMD("run_cm_delta_read", run_cm_delta_read),},
	{TSP_CMD("run_intensity_read", run_intensity_read),},
	{TSP_CMD("not_support_cmd", not_support_cmd),},
#ifdef TOUCH_BOOSTER_DVFS
        {TSP_CMD("boost_level", boost_level),},
#endif
};
#endif

#ifdef TOUCH_BOOSTER_DVFS
static void samsung_change_dvfs_lock(struct work_struct *work)
{
	struct mms_ts_info *info =
		container_of(work,
			struct mms_ts_info, work_dvfs_chg.work);
	int retval = 0;

	mutex_lock(&info->dvfs_lock);

	if (info->dvfs_boost_mode == DVFS_STAGE_DUAL) {
                if (info->stay_awake) {
                        dev_info(&info->client->dev,
                                        "%s: do fw update, do not change cpu frequency.\n",
                                        __func__);
                } else {
                        retval = set_freq_limit(DVFS_TOUCH_ID,
                                        MIN_TOUCH_LIMIT_SECOND);
                        info->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
                }
        } else if (info->dvfs_boost_mode == DVFS_STAGE_SINGLE ||
                        info->dvfs_boost_mode == DVFS_STAGE_TRIPLE) {
                retval = set_freq_limit(DVFS_TOUCH_ID, -1);
                info->dvfs_freq = -1;
        }

        if (retval < 0)
                dev_err(&info->client->dev,
                                "%s: booster change failed(%d).\n",
                                __func__, retval);

	mutex_unlock(&info->dvfs_lock);
}

static void samsung_set_dvfs_off(struct work_struct *work)
{
	struct mms_ts_info *info =
		container_of(work,
			struct mms_ts_info, work_dvfs_off.work);
	int retval;

	if (info->stay_awake) {
                dev_info(&info->client->dev,
                                "%s: do fw update, do not change cpu frequency.\n",
                                __func__);
        } else {
                mutex_lock(&info->dvfs_lock);

                retval = set_freq_limit(DVFS_TOUCH_ID, -1);
                info->dvfs_freq = -1;

                if (retval < 0)
                        dev_err(&info->client->dev,
                                        "%s: booster stop failed(%d).\n",
                                        __func__, retval);
                info->dvfs_lock_status = false;

                mutex_unlock(&info->dvfs_lock);
        }
}

static void samsung_set_dvfs_lock(struct mms_ts_info *info,
					int32_t on)
{
	int ret = 0;

	if (info->dvfs_boost_mode == DVFS_STAGE_NONE) {
                dev_dbg(&info->client->dev,
                                "%s: DVFS stage is none(%d)\n",
                                __func__, info->dvfs_boost_mode);
                return;
        }

        mutex_lock(&info->dvfs_lock);
        if (on == 0) {
                if (info->dvfs_lock_status)
                        schedule_delayed_work(&info->work_dvfs_off,
                                        msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));
        } else if (on > 0) {
                cancel_delayed_work(&info->work_dvfs_off);

                if ((!info->dvfs_lock_status) || (info->dvfs_old_stauts < on)) {
                        cancel_delayed_work(&info->work_dvfs_chg);

                        if (info->dvfs_freq != MIN_TOUCH_LIMIT) {
                                if (info->dvfs_boost_mode == DVFS_STAGE_TRIPLE)
                                        ret = set_freq_limit(DVFS_TOUCH_ID,
                                                MIN_TOUCH_LIMIT_SECOND);
                                else
                                        ret = set_freq_limit(DVFS_TOUCH_ID,
                                                MIN_TOUCH_LIMIT);
                                info->dvfs_freq = MIN_TOUCH_LIMIT;

                                if (ret < 0)
					dev_err(&info->client->dev,
                                                        "%s: cpu first lock failed(%d)\n",
                                                        __func__, ret);
                        }
			schedule_delayed_work(&info->work_dvfs_chg,
				msecs_to_jiffies(TOUCH_BOOSTER_CHG_TIME));
                        info->dvfs_lock_status = true;
                }
        } else if (on < 0) {
                if (info->dvfs_lock_status) {
                        cancel_delayed_work(&info->work_dvfs_off);
                        cancel_delayed_work(&info->work_dvfs_chg);
                        schedule_work(&info->work_dvfs_off.work);
                }
        }
        info->dvfs_old_stauts = on;
        mutex_unlock(&info->dvfs_lock);
}


static void samsung_init_dvfs(struct mms_ts_info *info)
{
	mutex_init(&info->dvfs_lock);

	info->dvfs_boost_mode = DVFS_STAGE_DUAL;

	INIT_DELAYED_WORK(&info->work_dvfs_off, samsung_set_dvfs_off);
	INIT_DELAYED_WORK(&info->work_dvfs_chg, samsung_change_dvfs_lock);

	info->dvfs_lock_status = false;
}
#endif

#if 0
static inline void mms_pwr_on_reset(struct mms_ts_info *info)
{
	struct i2c_adapter *adapter = to_i2c_adapter(info->client->dev.parent);

/*	if (!info->pdata->mux_fw_flash) {
		dev_info(&info->client->dev,
			 "missing platform data, can't do power-on-reset\n");
		return;
	}
*/
	i2c_lock_adapter(adapter);
//	info->pdata->mux_fw_flash(true);

	//info->pdata->power(0);
	gpio_direction_output(info->pdata->gpio_sda, 0);
	gpio_direction_output(info->pdata->gpio_scl, 0);
	gpio_direction_output(info->pdata->gpio_int, 0);
	msleep(50);
	//info->pdata->power(1);
	msleep(100);

//	info->pdata->mux_fw_flash(false);
	i2c_unlock_adapter(adapter);

	/* TODO: Seems long enough for the firmware to boot.
	 * Find the right value */
	msleep(250);
}
#endif
static void release_all_fingers(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	int i;

	dev_dbg(&client->dev, "[TSP] %s\n", __func__);

#if TOUCHKEY
	for (i = 1; i < 3; i++) {
		if (info->touchkey[i] == 1) {
			info->touchkey[i] = 0;
			input_report_key(info->input_dev,
				info->keycode[i], 0);
		}
	}
#endif

	for (i = 0; i < MAX_FINGERS; i++) {
#ifdef SEC_TSP_DEBUG
		if (info->finger_state[i] == 1)
			dev_notice(&client->dev, "finger %d up(force)\n", i);
#endif
		info->finger_state[i] = 0;
		input_mt_slot(info->input_dev, i);
		input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER,
					   false);
	}
	input_sync(info->input_dev);

#ifdef TOUCH_BOOSTER_DVFS
	info->finger_cnt1=0;
	samsung_set_dvfs_lock(info, -1);
#endif
}

static void mms_set_noise_mode(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;

	if (!(info->noise_mode && info->enabled))
		return;
	dev_notice(&client->dev, "%s\n", __func__);

	if (info->ta_status) {
		dev_notice(&client->dev, "noise_mode & TA connect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x30, 0x1);
	} else {
		dev_notice(&client->dev, "noise_mode & TA disconnect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x30, 0x2);
		info->noise_mode = 0;
	}
}

static void mms_set_threewave_mode(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	dev_notice(&client->dev, "%s\n", __func__);

	i2c_smbus_write_byte_data(info->client, 0x32, 0x1);
}

static int mms_reset(struct mms_ts_info *info)
{
	printk(KERN_ERR" %s excute\n", __func__);
	info->power(info,0);
	msleep(150);
	info->power(info,1);
	msleep(50);

	return ISC_SUCCESS;
}

static void reset_mms_ts(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;

	if (info->enabled == false)
		return;

	dev_notice(&client->dev, "%s++\n", __func__);
	/* Disabling irq as it is not required since we're using oneshot irq
	* and this function is called only from the irq handler
	*/
	//disable_irq_nosync(info->irq);
	info->enabled = false;

	touch_is_pressed = 0;
	release_all_fingers(info);

	mms_reset(info);

	info->enabled = true;
	if (info->ta_status) {
		dev_notice(&client->dev, "TA connect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x33, 0x1);
	} else {
		dev_notice(&client->dev, "TA disconnect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x33, 0x2);
	}
	mms_set_noise_mode(info);

	//enable_irq(info->irq);

	dev_notice(&client->dev, "%s--\n", __func__);
}

static void melfas_ta_cb(struct tsp_callbacks *cb, bool ta_status)
{
	struct mms_ts_info *info =
			container_of(cb, struct mms_ts_info, callbacks);
	struct i2c_client *client = info->client;

	dev_notice(&client->dev, "%s\n", __func__);

	info->ta_status = ta_status;

	if (info->enabled) {
		if (info->ta_status) {
			dev_notice(&client->dev, "TA connect!!!\n");
			i2c_smbus_write_byte_data(info->client, 0x33, 0x1);
		} else {
			dev_notice(&client->dev, "TA disconnect!!!\n");
			i2c_smbus_write_byte_data(info->client, 0x33, 0x2);
		}
		mms_set_noise_mode(info);
	}

/*	if (!ta_status)
		mms_set_noise_mode(info);
*/
}

static irqreturn_t mms_ts_interrupt(int irq, void *dev_id)
{
	struct mms_ts_info *info = dev_id;
	struct i2c_client *client = info->client;
	int ret;
	int i;
	int sz = 0;
	u8 buf[MAX_FINGERS * EVENT_SZ] = { 0 };
	u8 reg = MMS_INPUT_EVENT;

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = &reg,
			.len = 1,
		}, {
			.addr = client->addr,
			.flags = I2C_M_RD,
			.buf = buf,
		},
	};

	mutex_lock(&info->lock);
	if (!info->enabled)
		goto out;

	sz = i2c_smbus_read_byte_data(client, MMS_INPUT_EVENT_PKT_SZ);
	if (sz < 0) {
		dev_err(&client->dev, "%s bytes=%d\n", __func__, sz);
		for (i = 0; i < 50; i++) {
			sz = i2c_smbus_read_byte_data(client,
						      MMS_INPUT_EVENT_PKT_SZ);
			if (sz > 0)
				break;
		}

		if (i == 50) {
			dev_dbg(&client->dev, "i2c failed... reset!!\n");
			reset_mms_ts(info);
			goto out;
		}
	}

	if (sz == 0)
		goto out;

	if (sz > MAX_FINGERS * EVENT_SZ) {
		dev_err(&client->dev, "abnormal data inputed\n");
		goto out;
	}

	msg[1].len = sz;
	ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (ret != ARRAY_SIZE(msg)) {
		dev_err(&client->dev,
			"failed to read %d bytes of touch data (%d)\n",
			sz, ret);
		if (ret < 0) {
			dev_err(&client->dev,
				"%s bytes=%d\n", __func__, sz);
			for (i = 0; i < 5; i++) {
				msleep(20);
				ret = i2c_transfer(client->adapter,
					msg, ARRAY_SIZE(msg));
				if (ret > 0)
					break;
			}
			if (i == 5) {
				dev_dbg(&client->dev,
					"[TSP] i2c failed E2... reset!!\n");
				reset_mms_ts(info);
				goto out;
			}
		}
	}
#if defined(VERBOSE_DEBUG)
	print_hex_dump(KERN_DEBUG, "mms_ts raw: ",
		       DUMP_PREFIX_OFFSET, 32, 1, buf, sz, false);

#endif
	if (buf[0] == 0x0F) {	/* ESD */
		if(buf[1] == 0x00)
		{
			dev_dbg(&client->dev, "ESD DETECT.... reset!!\n");
			reset_mms_ts(info);
		}
		else
			dev_dbg(&client->dev, "Recal Notify %d.... reset!!\n", buf[1]);
		goto out;
	}

	if (buf[0] == 0x0E) { /* NOISE MODE */
		dev_dbg(&client->dev, "[TSP] noise mode enter!!\n");
		info->noise_mode = 1;
		mms_set_noise_mode(info);
		goto out;
	}

	if (buf[0] == 0x0B) { /* THREEWAVE MODE */
		if (buf[1] == 0x01) {
			dev_dbg(&client->dev, "[TSP] three-wave mode enter!!\n");
			info->threewave_mode = 1;
		} else {
			dev_dbg(&client->dev, "[TSP] three-wave mode exit!!\n");
			info->threewave_mode = 0;
		}
		goto out;
	}

	for (i = 0; i < sz; i += EVENT_SZ) {
		u8 *tmp = &buf[i];
		int id, x, y, angle, palm;
#if TOUCHKEY
		u8 keycode;
		u8 key_press;

		if ((tmp[0] & 0x20) == 0x0) { /* touch key */
			keycode = tmp[0] & 0xf;
			key_press = (tmp[0] >> 7);

			if (key_press == 0) {
				input_report_key(info->input_dev,
					info->keycode[keycode], 0);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
				dev_notice(&client->dev,
					"key R(%d)\n", info->panel);
#else
				dev_notice(&client->dev,
					"key R : %d(%d)(%d)\n",
					info->keycode[keycode],
					tmp[0] & 0xf, info->panel);
#endif
			} else {
				input_report_key(info->input_dev,
					info->keycode[keycode], 1);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
				dev_notice(&client->dev,
					"key P(%d)\n", info->panel);
#else
				dev_notice(&client->dev,
					"key P : %d(%d)(%d)\n",
					info->keycode[keycode],
					tmp[0] & 0xf, info->panel);
#endif
			}
			continue;
		}
#else
		if ((tmp[0] & 0x20) == 0x0)
			continue;
#endif
		id = (tmp[0] & 0xf) - 1;
		x = tmp[2] | ((tmp[1] & 0xf) << 8);
		y = tmp[3] | ((tmp[1] >> 4) << 8);
		angle = (tmp[5] >= 127) ? (-(256 - tmp[5])) : tmp[5];
		palm = (tmp[0] & 0x10) >> 4;

		if (info->invert_x) {
			x = info->max_x - x;
			if (x < 0)
				x = 0;
		}
		if (info->invert_y) {
			y = info->max_y - y;
			if (y < 0)
				y = 0;
		}

		if (palm) {
			if (info->palm_flag == 3) {
				info->palm_flag = 1;
			} else {
				info->palm_flag = 3;
				palm = 3;
			}
		} else {
			if (info->palm_flag == 2) {
				info->palm_flag = 0;
			} else {
				info->palm_flag = 2;
				palm = 2;
			}
		}

		if (id >= MAX_FINGERS) {
			dev_notice(&client->dev,
				"finger id error [%d]\n", id);
			reset_mms_ts(info);
			goto out;
		}

		if (x == 0 && y == 0)
			continue;

		if ((tmp[0] & 0x80) == 0) {
			input_mt_slot(info->input_dev, id);
			input_mt_report_slot_state(info->input_dev,
						   MT_TOOL_FINGER, false);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
			if (info->finger_state[id] != 0) {
				info->finger_state[id] = 0;
				touch_is_pressed--;
				dev_notice(&client->dev,
					"R [%2d](%d)", id, info->panel);
			}
#else /*CONFIG_SAMSUNG_PRODUCT_SHIP */
			if (info->finger_state[id] != 0) {
				info->finger_state[id] = 0;
				touch_is_pressed--;
				dev_notice(&client->dev,
					"R [%2d],([%4d],[%3d])(%d)",
					id, x, y, info->panel);
			}

#endif /*CONFIG_SAMSUNG_PRODUCT_SHIP */
			continue;
		}

		input_mt_slot(info->input_dev, id);
		input_mt_report_slot_state(info->input_dev,
					   MT_TOOL_FINGER, true);
		input_report_abs(info->input_dev,
			ABS_MT_POSITION_X, x);
		input_report_abs(info->input_dev,
			ABS_MT_POSITION_Y, y);
#if defined(CONFIG_MACH_MILLETLTE_VZW)
		input_report_abs(info->input_dev,
			ABS_MT_WIDTH_MAJOR, (1229*tmp[4]) >>10); //ABS_MT_WIDTH_MAJOR * 1.2 (only for VZW)
#else
		input_report_abs(info->input_dev,
			ABS_MT_WIDTH_MAJOR, tmp[4]);
#endif
		input_report_abs(info->input_dev,
			ABS_MT_TOUCH_MAJOR, tmp[6]);
		input_report_abs(info->input_dev,
			ABS_MT_TOUCH_MINOR, tmp[7]);
		input_report_abs(info->input_dev,
			ABS_MT_PALM, palm);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
		if (info->finger_state[id] == 0) {
			info->finger_state[id] = 1;
			touch_is_pressed++;
			dev_notice(&client->dev,
				"P [%2d](%d)", id, info->panel);
		}
#else /* CONFIG_SAMSUNG_PRODUCT_SHIP */
		if (info->finger_state[id] == 0) {
			info->finger_state[id] = 1;
			touch_is_pressed++;
#if defined(CONFIG_MACH_MILLETLTE_VZW)
			dev_notice(&client->dev,
				"P [%2d],([%3d],[%4d]) w=%d, major=%d, minor=%d, angle=%d, palm=%d(%d)",
				id, x, y,((1229*tmp[4]) >>10), tmp[6], tmp[7],
				angle, palm, info->panel);
#else
			dev_notice(&client->dev,
				"P [%2d],([%3d],[%4d]) w=%d, major=%d, minor=%d, angle=%d, palm=%d(%d)",
				id, x, y, tmp[4], tmp[6], tmp[7],
				angle, palm, info->panel);
#endif
		}
#endif /* CONFIG_SAMSUNG_PRODUCT_SHIP */
	}
	input_sync(info->input_dev);

#ifdef TOUCH_BOOSTER_DVFS
	samsung_set_dvfs_lock(info, touch_is_pressed);
#endif

out:
	mutex_unlock(&info->lock);
	return IRQ_HANDLED;
}

int get_tsp_status(void)
{
	return touch_is_pressed;
}
EXPORT_SYMBOL(get_tsp_status);

static int get_fw_version(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	u8 reg = MMS_CORE_VERSION;
	int ret;
	unsigned char buf[3] = { 0, };

	msg[0].addr = client->addr;
	msg[0].flags = 0x00;
	msg[0].len = 1;
	msg[0].buf = &reg;
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = SECTION_NUM;
	msg[1].buf = buf;

	ret = i2c_transfer(adapter, msg, ARRAY_SIZE(msg));
	if (ret != ARRAY_SIZE(msg)) {
		pr_err("[TSP] : read error : [%d]", ret);
		return ret;
	}

	info->fw_boot_ver = buf[0];
	info->fw_core_ver = buf[1];
	info->fw_ic_ver = buf[2];

	dev_info(&info->client->dev,
		"boot : 0x%x, core : 0x%x, config : 0x%x\n",
		buf[0], buf[1], buf[2]);
	return 0;
}

/*
static int mms_ts_enable(struct mms_ts_info *info, int wakeupcmd)
{
	mutex_lock(&info->lock);
	if (info->enabled)
		goto out;

	if (wakeupcmd == 1) {
		i2c_smbus_write_byte_data(info->client, 0, 0);
		usleep_range(3000, 5000);
	}
	info->enabled = true;
	enable_irq(info->irq);
out:
	mutex_unlock(&info->lock);
	return 0;
}

static int mms_ts_disable(struct mms_ts_info *info, int sleepcmd)
{
	mutex_lock(&info->lock);
	if (!info->enabled)
		goto out;
	disable_irq_nosync(info->irq);
	if (sleepcmd == 1) {
		i2c_smbus_write_byte_data(info->client, MMS_MODE_CONTROL, 0);
		usleep_range(10000, 12000);
	}
	info->enabled = false;
	touch_is_pressed = 0;
out:
	mutex_unlock(&info->lock);
	return 0;
}
*/

#ifdef SEC_TSP_FACTORY_TEST
static void set_cmd_result(struct mms_ts_info *info, char *buff, int len)
{
	strncat(info->cmd_result, buff, len);
}

static int get_data(struct mms_ts_info *info, u8 addr, u8 size, u8 *array)
{
	struct i2c_client *client = info->client;
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg;
	u8 reg = addr;
	unsigned char buf[size];
	int ret;

	msg.addr = client->addr;
	msg.flags = 0x00;
	msg.len = 1;
	msg.buf = &reg;

	ret = i2c_transfer(adapter, &msg, 1);

	if (ret >= 0) {
		msg.addr = client->addr;
		msg.flags = I2C_M_RD;
		msg.len = size;
		msg.buf = buf;

		ret = i2c_transfer(adapter, &msg, 1);
	}
	if (ret < 0) {
		pr_err("[TSP] : read error : [%d]", ret);
		return ret;
	}

	memcpy(array, &buf, size);
	return size;
}

static void get_intensity_data(struct mms_ts_info *info)
{
	u8 w_buf[4];
	u8 r_buf;
	u8 read_buffer[60] = {0};
	int i, j;
	int ret;
	u16 max_value = 0, min_value = 0;
	u16 raw_data;
	char buff[TSP_CMD_STR_LEN] = {0};

	disable_irq(info->irq);

	w_buf[0] = ADDR_UNIV_CMD;
	w_buf[1] = CMD_GET_INTEN;
	w_buf[2] = 0xFF;
	for (i = 0; i < RX_NUM; i++) {
		w_buf[3] = i;

		ret = i2c_smbus_write_i2c_block_data(info->client,
			w_buf[0], 3, &w_buf[1]);
		if (ret < 0)
			goto err_i2c;
		usleep_range(1, 5);

		ret = i2c_smbus_read_i2c_block_data(info->client,
			CMD_RESULT_SZ, 1, &r_buf);
		if (ret < 0)
			goto err_i2c;

		ret = get_data(info, CMD_RESULT, r_buf, read_buffer);
		if (ret < 0)
			goto err_i2c;

		for (j = 0; j < r_buf/2; j++) {
			raw_data = read_buffer[2*j] | (read_buffer[2*j+1] << 8);
			if (raw_data > 32767)
				raw_data = 0;
			if (i == 0 && j == 0) {
				max_value = min_value = raw_data;
			} else {
				max_value = max(max_value, raw_data);
				min_value = min(min_value, raw_data);
			}
			info->intensity[i * TX_NUM + j] = raw_data;
			dev_dbg(&info->client->dev,
				"[TSP] intensity[%d][%d] = %d\n", j, i,
				info->intensity[i * TX_NUM + j]);
		}
	}

	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	enable_irq(info->irq);

	return;

err_i2c:
	dev_err(&info->client->dev, "%s: fail to i2c (cmd=%d)\n",
		__func__, MMS_VSC_CMD_INTENSITY);
}

static int get_raw_data(struct mms_ts_info *info, u8 cmd)
{
	u8 w_buf[4];
	u8 r_buf = 0;
	u8 read_buffer[60] = {0};
	int ret;
	int i, j;
	int max_value = 0, min_value = 0;
	int raw_data;
	int retry;
	char buff[TSP_CMD_STR_LEN] = {0};
	int gpio = info->pdata->gpio_int;

	disable_irq(info->irq);

	ret = i2c_smbus_write_byte_data(info->client,
		ADDR_UNIV_CMD, CMD_ENTER_TEST);
	if (ret < 0)
		goto err_i2c;

	/* event type check */
	retry = 1;
	while (retry) {
		while (gpio_get_value(gpio))
			udelay(100);

		ret = i2c_smbus_read_i2c_block_data(info->client,
			0x0F, 1, &r_buf);
		if (ret < 0)
			goto err_i2c;

		ret = i2c_smbus_read_i2c_block_data(info->client,
			0x10, 1, &r_buf);
		if (ret < 0)
			goto err_i2c;

		dev_info(&info->client->dev, "event type = 0x%x\n", r_buf);
		if (r_buf == 0x0C)
			retry = 0;
	}

	w_buf[0] = ADDR_UNIV_CMD;
	if (cmd == MMS_VSC_CMD_CM_DELTA)
		w_buf[1] = CMD_CM_DELTA;
	else
		w_buf[1] = CMD_CM_ABS;
	ret = i2c_smbus_write_i2c_block_data(info->client,
		 w_buf[0], 1, &w_buf[1]);
	if (ret < 0)
		goto err_i2c;
	while (gpio_get_value(gpio))
		udelay(100);

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CMD_RESULT_SZ, 1, &r_buf);
	if (ret < 0)
		goto err_i2c;
	ret = i2c_smbus_read_i2c_block_data(info->client,
		CMD_RESULT, 1, &r_buf);
	if (ret < 0)
		goto err_i2c;

	if (r_buf == 1)
		dev_info(&info->client->dev, "PASS\n");
	else
		dev_info(&info->client->dev, "FAIL\n");

	if (cmd == MMS_VSC_CMD_CM_DELTA)
		w_buf[1] = CMD_GET_DELTA;
	else
		w_buf[1] = CMD_GET_ABS;
	w_buf[2] = 0xFF;

	for (i = 0; i < RX_NUM; i++) {
		w_buf[3] = i;

		ret = i2c_smbus_write_i2c_block_data(info->client,
			w_buf[0], 3, &w_buf[1]);
		if (ret < 0)
			goto err_i2c;

		while (gpio_get_value(gpio))
			udelay(100);

		ret = i2c_smbus_read_i2c_block_data(info->client,
			CMD_RESULT_SZ, 1, &r_buf);
		if (ret < 0)
			goto err_i2c;

		ret = get_data(info, CMD_RESULT, r_buf, read_buffer);
		if (ret < 0)
			goto err_i2c;

		for (j = 0; j < TX_NUM; j++) {
			raw_data = read_buffer[2*j] | (read_buffer[2*j+1] << 8);
			if (i == 0 && j == 0) {
				max_value = min_value = raw_data;
			} else {
				max_value = max(max_value, raw_data);
				min_value = min(min_value, raw_data);
			}

			if (cmd == MMS_VSC_CMD_CM_DELTA) {
				info->inspection[i * TX_NUM + j] =
					raw_data;
				dev_dbg(&info->client->dev,
					"[TSP] delta[%d][%d] = %d\n", j, i,
					info->inspection[i * TX_NUM + j]);
			} else if (cmd == MMS_VSC_CMD_CM_ABS) {
				info->raw[i * TX_NUM + j] =
					raw_data;
				dev_dbg(&info->client->dev,
					"[TSP] raw[%d][%d] = %d\n", j, i,
					info->raw[i * TX_NUM + j]);
			} else if (cmd == MMS_VSC_CMD_REFER) {
				info->reference[i * TX_NUM + j] =
					raw_data;
				dev_dbg(&info->client->dev,
					"[TSP] reference[%d][%d] = %d\n", j, i,
					info->reference[i * TX_NUM + j]);
			}
		}
	}

	ret = i2c_smbus_write_byte_data(info->client,
		ADDR_UNIV_CMD, CMD_EXIT_TEST);

	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	touch_is_pressed = 0;
	release_all_fingers(info);

	mms_reset(info);
	info->enabled = true;

	if (info->ta_status) {
		dev_notice(&info->client->dev, "TA connect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x33, 0x1);
	} else {
		dev_notice(&info->client->dev, "TA disconnect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x33, 0x2);
	}
	mms_set_noise_mode(info);

	enable_irq(info->irq);

	if((min_value < 2 && max_value < 2)||(min_value > 1680 && max_value < 1730))
		return -ECHRNG;

	return 0;

err_i2c:
	dev_err(&info->client->dev, "%s: fail to i2c (cmd=%d)\n",
		__func__, cmd);
	enable_irq(info->irq);
	return -EIO;
}

static void get_raw_data_all(struct mms_ts_info *info, u8 cmd)
{
	u8 w_buf[6];
	u8 read_buffer[2];	/* 52 */
	int ret;
	int i, j;
	u32 max_value = 0, min_value = 0;
	u32 raw_data;
	char buff[TSP_CMD_STR_LEN] = {0};
	int gpio = info->pdata->gpio_int;

/*      gpio = msm_irq_to_gpio(info->irq); */
	disable_irq(info->irq);

	w_buf[0] = MMS_VSC_CMD;	/* vendor specific command id */
	w_buf[1] = MMS_VSC_MODE;	/* mode of vendor */
	w_buf[2] = 0;		/* tx line */
	w_buf[3] = 0;		/* rx line */
	w_buf[4] = 0;		/* reserved */
	w_buf[5] = 0;		/* sub command */

	if (cmd == MMS_VSC_CMD_EXIT) {
		w_buf[5] = MMS_VSC_CMD_EXIT;	/* exit test mode */

		ret = i2c_smbus_write_i2c_block_data(info->client,
						     w_buf[0], 5, &w_buf[1]);
		if (ret < 0)
			goto err_i2c;
		enable_irq(info->irq);
		msleep(200);
		return;
	}

	/* MMS_VSC_CMD_CM_DELTA or MMS_VSC_CMD_CM_ABS
	 * this two mode need to enter the test mode
	 * exit command must be followed by testing.
	 */
	if (cmd == MMS_VSC_CMD_CM_DELTA || cmd == MMS_VSC_CMD_CM_ABS) {
		/* enter the debug mode */
		w_buf[2] = 0x0;	/* tx */
		w_buf[3] = 0x0;	/* rx */
		w_buf[5] = MMS_VSC_CMD_ENTER;

		ret = i2c_smbus_write_i2c_block_data(info->client,
						     w_buf[0], 5, &w_buf[1]);
		if (ret < 0)
			goto err_i2c;

		/* wating for the interrupt */
		while (gpio_get_value(gpio))
			udelay(100);
	}

	for (i = 0; i < RX_NUM; i++) {
		for (j = 0; j < TX_NUM; j++) {

			w_buf[2] = j;	/* tx */
			w_buf[3] = i;	/* rx */
			w_buf[5] = cmd;

			ret = i2c_smbus_write_i2c_block_data(info->client,
					w_buf[0], 5, &w_buf[1]);
			if (ret < 0)
				goto err_i2c;

			usleep_range(1, 5);

			ret = i2c_smbus_read_i2c_block_data(info->client, 0xBF,
							    2, read_buffer);
			if (ret < 0)
				goto err_i2c;

			raw_data = ((u16) read_buffer[1] << 8) | read_buffer[0];
			if (i == 0 && j == 0) {
				max_value = min_value = raw_data;
			} else {
				max_value = max(max_value, raw_data);
				min_value = min(min_value, raw_data);
			}

			if (cmd == MMS_VSC_CMD_INTENSITY) {
				info->intensity[i * TX_NUM + j] =
					raw_data;
				dev_dbg(&info->client->dev,
					"[TSP] intensity[%d][%d] = %d\n", j, i,
					info->intensity[i * TX_NUM + j]);
			} else if (cmd == MMS_VSC_CMD_CM_DELTA) {
				info->inspection[i * TX_NUM + j] =
					raw_data;
				dev_dbg(&info->client->dev,
					"[TSP] delta[%d][%d] = %d\n", j, i,
					info->inspection[i * TX_NUM + j]);
			} else if (cmd == MMS_VSC_CMD_CM_ABS) {
				info->raw[i * TX_NUM + j] =
					raw_data;
				dev_dbg(&info->client->dev,
					"[TSP] raw[%d][%d] = %d\n", j, i,
					info->raw[i * TX_NUM + j]);
			} else if (cmd == MMS_VSC_CMD_REFER) {
				info->reference[i * TX_NUM + j] =
					raw_data;
				dev_dbg(&info->client->dev,
					"[TSP] reference[%d][%d] = %d\n", j, i,
					info->reference[i * TX_NUM + j]);
			}
		}

	}

	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	enable_irq(info->irq);

	return;

err_i2c:
	dev_err(&info->client->dev, "%s: fail to i2c (cmd=%d)\n",
		__func__, cmd);
}

static ssize_t show_close_tsp_test(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);

	get_raw_data_all(info, MMS_VSC_CMD_EXIT);
	info->ft_flag = 0;

	return snprintf(buf, TSP_BUF_SIZE, "%u\n", 0);
}

static void set_default_result(struct mms_ts_info *info)
{
	char delim = ':';

	memset(info->cmd_result, 0x00, ARRAY_SIZE(info->cmd_result));
	memcpy(info->cmd_result, info->cmd, strlen(info->cmd));
	strncat(info->cmd_result, &delim, 1);
}

static int check_rx_tx_num(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[TSP_CMD_STR_LEN] = {0};
	int node;

	if (info->cmd_param[0] < 0 ||
			info->cmd_param[0] >= TX_NUM  ||
			info->cmd_param[1] < 0 ||
			info->cmd_param[1] >= RX_NUM) {
		snprintf(buff, sizeof(buff) , "%s", "NG");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = FAIL;

		dev_info(&info->client->dev, "%s: parameter error: %u,%u\n",
				__func__, info->cmd_param[0],
				info->cmd_param[1]);
		node = -1;
		return node;
}
	node = info->cmd_param[1] * TX_NUM + info->cmd_param[0];
	dev_info(&info->client->dev, "%s: node = %d\n", __func__,
			node);
	return node;

}

static void not_support_cmd(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};

	set_default_result(info);
	sprintf(buff, "%s", "NA");
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	mutex_lock(&info->cmd_lock);
	info->cmd_is_running = false;
	mutex_unlock(&info->cmd_lock);

	info->cmd_state = WAITING;
	dev_info(&info->client->dev, "%s: \"%s(%d)\"\n", __func__,
				buff, strnlen(buff, sizeof(buff)));
	return;
}

#ifdef TOUCH_BOOSTER_DVFS
static void boost_level(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	struct i2c_client *client = info->client;
	char buff[16] = {0};

	int retval = 0;

	dev_info(&client->dev, "%s\n", __func__);

	set_default_result(info);

	info->dvfs_boost_mode = info->cmd_param[0];

	dev_info(&client->dev,
			"%s: dvfs_boost_mode = %d\n",
			__func__, info->dvfs_boost_mode);

	snprintf(buff, sizeof(buff), "OK");
	info->cmd_state = RUNNING;

	if (info->dvfs_boost_mode == DVFS_STAGE_NONE) {
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		if (retval < 0) {
			dev_err(&info->client->dev,
					"%s: booster stop failed(%d).\n",
					__func__, retval);
			snprintf(buff, sizeof(buff), "NG");
			info->cmd_state = FAIL;

			info->dvfs_lock_status = false;
		}
	}

	set_cmd_result(info, buff,
			strnlen(buff, sizeof(buff)));

	mutex_lock(&info->cmd_lock);
	info->cmd_is_running = false;
	mutex_unlock(&info->cmd_lock);

	info->cmd_state = WAITING;

	return;
}
#endif

static int mms_isc_read_status(struct mms_ts_info *info, u32 val)
{
	struct i2c_client *client = info->client;
	u8 cmd = ISC_CMD_READ_STATUS;
	u32 result = 0;
	int cnt = 100;
	int ret = 0;

	do {
		ret = i2c_smbus_read_i2c_block_data(client,
			cmd, 4, (u8 *)&result);
		if (ret < 0)
			usleep_range(1000, 1000);

		if (result == val)
			break;
		usleep_range(1000, 1000);
	} while (--cnt);

	if (!cnt) {
		dev_err(&client->dev,
			"status read fail. cnt : %d, val : 0x%x != 0x%x\n",
			cnt, result, val);
	}

	return ret;
}

static int mms_isc_transfer_cmd(struct mms_ts_info *info, int cmd)
{
	struct i2c_client *client = info->client;
	struct isc_packet pkt = { ISC_ADDR, cmd };
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.len = sizeof(struct isc_packet),
		.buf = (u8 *)&pkt,
	};

	return (i2c_transfer(client->adapter, &msg, 1) != 1);
}

static int mms_isc_erase_page(struct mms_ts_info *info, int page)
{
	int ret = 0;

	ret = mms_isc_transfer_cmd(info, ISC_CMD_PAGE_ERASE | page);
	ret |= mms_isc_read_status(info, ISC_PAGE_ERASE_DONE | ISC_PAGE_ERASE_ENTER | page);

	return ret;
}
#if 0
static int mms_isc_enter(struct mms_ts_info *info)
{
	return i2c_smbus_write_byte_data(info->client, MMS_CMD_ENTER_ISC, true);
}
#endif
static int mms_isc_exit(struct mms_ts_info *info)
{
	return mms_isc_transfer_cmd(info, ISC_CMD_EXIT);
}

static int mms_flash_section(struct mms_ts_info *info,
		struct mms_fw_img *img, const u8 *data,
		struct mms_ext_hdr *ext)
{
	struct i2c_client *client = info->client;
	struct isc_packet *isc_packet;
	int ret;
	int page, i;
	struct i2c_msg msg[2] = {
		{
			.addr = client->addr,
			.flags = 0,
		}, {
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = ISC_XFER_LEN,
		},
	};
	int ptr = img->offset;

	isc_packet = kzalloc(sizeof(*isc_packet) + ISC_XFER_LEN, GFP_KERNEL);
	isc_packet->cmd = ISC_ADDR;

	msg[0].buf = (u8 *)isc_packet;
	msg[1].buf = kzalloc(ISC_XFER_LEN, GFP_KERNEL);

	for (page = img->start_page; page <= img->end_page; page++) {
		if (ext->data[page] & EXT_INFO_ERASE)
			mms_isc_erase_page(info, page);

		if (!(ext->data[page] & EXT_INFO_WRITE)) {
			ptr += MMS_FLASH_PAGE_SZ;
			continue;
		}

		for (i = 0; i < ISC_BLOCK_NUM; i++, ptr += ISC_XFER_LEN) {
			/* flash firmware */
			u32 tmp = page * 256 + i * (ISC_XFER_LEN / 4);
			put_unaligned_le32(tmp, &isc_packet->addr);
			msg[0].len = sizeof(struct isc_packet) + ISC_XFER_LEN;

			memcpy(isc_packet->data, data + ptr, ISC_XFER_LEN);
			if (i2c_transfer(client->adapter, msg, 1) != 1)
				goto i2c_err;

			/* verify firmware */
			tmp |= ISC_CMD_READ;
			put_unaligned_le32(tmp, &isc_packet->addr);
			msg[0].len = sizeof(struct isc_packet);

			if (i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg)) != ARRAY_SIZE(msg))
				goto i2c_err;

			if (memcmp(isc_packet->data, msg[1].buf, ISC_XFER_LEN)) {
#if FLASH_VERBOSE_DEBUG
				print_hex_dump(KERN_ERR, "mms fw wr : ",
						DUMP_PREFIX_OFFSET, 16, 1,
						isc_packet->data, ISC_XFER_LEN, false);

				print_hex_dump(KERN_ERR, "mms fw rd : ",
						DUMP_PREFIX_OFFSET, 16, 1,
						msg[1].buf, ISC_XFER_LEN, false);
#endif
				dev_err(&client->dev, "flash verify failed\n");
				ret = -1;
				goto out;
			}

		}
	}

	dev_info(&client->dev, "section [%d] update succeeded\n", img->type);

	ret = 0;
	goto out;

i2c_err:
	dev_err(&client->dev, "i2c failed @ %s\n", __func__);
	ret = -1;

out:
	kfree(isc_packet);
	kfree(msg[1].buf);

	return ret;
}

static int mms_flash_fw(struct mms_ts_info *info, bool force_update, int cmd)
{
	struct i2c_client *client = info->client;
	struct mms_bin_hdr *fw_hdr;
	struct mms_fw_img **img;
	struct mms_ext_hdr *ext;
	struct file *fp = NULL;
	const struct firmware *fw;
	char fw_path[MAX_FW_PATH + 1];
	const u8 *buff = 0;
	long fsize = 0, nread = 0;
	mm_segment_t old_fs = {0};
	u8 ver[SECTION_NUM];
	u8 target[SECTION_NUM];
	int offset = sizeof(struct mms_bin_hdr);
	int ret;
	int i;
	bool update_flag = false;

	ret = get_fw_version(info);
	if (ret == 0) {
		ver[0] = info->fw_boot_ver;
		ver[1] = info->fw_core_ver;
		ver[2] = info->fw_ic_ver;
	} else {
		ver[0] = 0;
		ver[1] = 0;
		ver[2] = 0;
	}

	if (cmd == UMS) {
		old_fs = get_fs();
		set_fs(get_ds());

		snprintf(fw_path, MAX_FW_PATH,
			"/sdcard/%s", TSP_FW_FILENAME);
		fp = filp_open(fw_path, O_RDONLY, 0);
		if (IS_ERR(fp)) {
			dev_err(&client->dev,
				"file %s open error:%d\n", fw_path, (s32)fp);
			set_fs(old_fs);
			ret = -1;
			goto fw_read_fail;
		}

		fsize = fp->f_path.dentry->d_inode->i_size;
		buff = kzalloc((size_t)fsize, GFP_KERNEL);
		if (!buff) {
			dev_err(&client->dev, "fail to alloc buffer for fw\n");
			set_fs(old_fs);
			filp_close(fp, current->files);
			ret = -1;
			goto fw_read_fail;
		}

		nread = vfs_read(fp, (char __user *)buff, fsize, &fp->f_pos);
		if (nread != fsize) {
			dev_err(&client->dev, "fail to vfs_read file\n");
			kfree(buff);
			set_fs(old_fs);
			filp_close(fp, current->files);
			ret = -1;
			goto fw_read_fail;
		}

		fw_hdr = (struct mms_bin_hdr *)buff;
		ext = (struct mms_ext_hdr *)(buff + fw_hdr->extention_offset);
		img = kzalloc(sizeof(*img) * fw_hdr->section_num, GFP_KERNEL);

		mms_reset(info);
		msleep(50);

		for (i = 0; i < fw_hdr->section_num; i++,
			offset += sizeof(struct mms_fw_img)) {
			img[i] = (struct mms_fw_img *)(buff + offset);
			target[i] = img[i]->version;

			if ((ver[img[i]->type] != target[i]) ||
					(force_update == true)) {
				dev_info(&client->dev,
					"section [%d] is need to be updated. ver : 0x%x, bin : 0x%x\n",
					i, ver[i], target[i]);

				update_flag = true;
				ret = mms_flash_section(info, img[i],
					buff + fw_hdr->binary_offset, ext);
				if (ret != 0) {
					mms_reset(info);
					goto out;
				}
			} else {
				dev_info(&client->dev, "section [%d] is up to date\n", i);
			}
		}
	} else {
		if(info->panel == EELY) {
			dev_err(&client->dev, "requesting tsp_melfas/lt/mms252_el.fw\n");
			ret = request_firmware(&fw,
					"tsp_melfas/lt/mms252_el.fw",
					&client->dev);
		} else if (info->panel == ILJIN) {
			ret = request_firmware(&fw,
					"tsp_melfas/lt/melfas_ij.fw",
					&client->dev);
			dev_err(&client->dev, "requesting tsp_melfas/lt/melfas_ij.fw\n");
		}
		else {
			ret = -1;
			dev_err(&client->dev, "[TSP] wrong panel - no firmware requested\n");	
		}
		if (ret) {
			dev_err(&client->dev, "failed to read firmware\n");
			goto fw_read_fail;
		}

		fw_hdr = (struct mms_bin_hdr *)fw->data;
		ext = (struct mms_ext_hdr *)(fw->data + fw_hdr->extention_offset);
		img = kzalloc(sizeof(*img) * fw_hdr->section_num, GFP_KERNEL);

		mms_reset(info);
		msleep(50);

		for (i = 0; i < fw_hdr->section_num; i++,
			offset += sizeof(struct mms_fw_img)) {
			img[i] = (struct mms_fw_img *)(fw->data + offset);
			target[i] = img[i]->version;

			if ((ver[img[i]->type] != target[i]) ||
				(force_update == true)) {
				dev_info(&client->dev,
					"section [%d] is need to be updated. ver : 0x%x, bin : 0x%x\n",
					i, ver[i], target[i]);

				update_flag = true;
				ret = mms_flash_section(info, img[i],
					fw->data + fw_hdr->binary_offset, ext);
				if (ret != 0) {
					mms_reset(info);
					goto out;
				}
			} else {
				dev_info(&client->dev, "section [%d] is up to date\n", i);
			}
		}
	}

	if (update_flag)
		mms_isc_exit(info);

	msleep(5);
	mms_reset(info);

	ret = get_fw_version(info);
	if (ret != 0) {
		dev_err(&client->dev, "failed to obtain version after flash\n");
		ret = -1;
		goto out;
	} else {
		ver[0] = info->fw_boot_ver;
		ver[1] = info->fw_core_ver;
		ver[2] = info->fw_ic_ver;
		for (i = 0; i < fw_hdr->section_num; i++) {
			if (ver[img[i]->type] != target[i]) {
				dev_info(&client->dev,
					"version mismatch after flash. [%d] 0x%x != 0x%x\n",
					i, ver[img[i]->type], target[i]);

				ret = -1;
				goto out;
			}
		}
	}
	ret = 0;
printk(KERN_INFO "mms_flash_fw end");
out:
	if (cmd == UMS) {
		filp_close(fp, current->files);
		set_fs(old_fs);
		kfree(buff);
	} else {
		release_firmware(fw);
	}
	kfree(img);
fw_read_fail:
	if (ret != 0) {
		info->fw_boot_ver = 0;
		info->fw_core_ver = 0;
		info->fw_ic_ver = 0;
	}
	return ret;
}

static void fw_update(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	struct i2c_client *client = info->client;
	int ret = 0;
	int fw_bin_ver = 0;
	int fw_core_ver = 0;
	int retries = 4;
	char result[16] = {0};

	if(info->panel == EELY) {
		fw_bin_ver = FW_VERSION_EL;
		fw_core_ver = CORE_VERSION_EL;
	}
	else if (info->panel == ILJIN) {
		fw_bin_ver = FW_VERSION_IJ;
		fw_core_ver = CORE_VERSION_IJ;
	}
	else {
		pr_err("[TSP] fw_update Wrong Panel");
	}

	set_default_result(info);
	dev_info(&client->dev,
		"fw_ic_ver = 0x%02x, fw_bin_ver = 0x%02x\n",
		info->fw_ic_ver, fw_bin_ver);

	switch (info->cmd_param[0]) {
	case BUILT_IN:
		dev_info(&client->dev, "built in fw update\n");
		if (info->fw_core_ver > fw_core_ver) {
			dev_info(&client->dev,
				"fw update does not need\n");
			goto do_not_need_update;
		} else if (info->fw_core_ver == fw_core_ver) {
			if (info->fw_ic_ver >= fw_bin_ver) {
				dev_info(&client->dev,
					"fw update does not need\n");
				goto do_not_need_update;
			}
		} else { /* core < fw_core_ver */
			dev_info(&client->dev,
				"fw update excute(core:0x%x)\n",
				info->fw_core_ver);
		}

		disable_irq(info->irq);
		while (retries--) {
			ret = mms_flash_fw(info, false, BUILT_IN);
			if (ret == 0) {
				pr_err("[TSP] fw update success");
				break;
			} else {
				pr_err("[TSP] fw update fail[%d], retry = %d",
					ret, retries);
			}
		}
		enable_irq(info->irq);
		break;

	case UMS:
		dev_info(&client->dev, "UMS fw update!!\n");

		disable_irq(info->irq);
		while (retries--) {
			ret = mms_flash_fw(info, true, UMS);
			if (ret == 0) {
				pr_err("[TSP] fw update success");
				break;
			} else {
				pr_err("[TSP] fw update fail[%d], retry = %d",
					ret, retries);
			}
		}
		enable_irq(info->irq);
		break;

	default:
		dev_err(&client->dev, "invalid fw update type\n");
		ret = -1;
		break;
	}

do_not_need_update:

	dev_info(&client->dev, "boot version : %d\n",
		info->fw_boot_ver);
	dev_info(&client->dev, "core version : 0x%02x\n",
		info->fw_core_ver);
	dev_info(&client->dev, "config version : 0x%02x\n",
		info->fw_ic_ver);

	if (ret == 0) {
		snprintf(result, sizeof(result) , "%s", "OK");
		set_cmd_result(info, result,
			strnlen(result, sizeof(result)));
		info->cmd_state = OK;
	} else {
		snprintf(result, sizeof(result) , "%s", "NG");
		set_cmd_result(info, result,
			strnlen(result, sizeof(result)));
		info->cmd_state = FAIL;
	}
}

static void get_fw_ver_bin(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};

	set_default_result(info);

	if (info->panel == ILJIN) {
		snprintf(buff, sizeof(buff), "ME%02x%02x%02x",
			info->panel, CORE_VERSION_IJ, FW_VERSION_IJ);	
	}
	else { /* EELY*/
		snprintf(buff, sizeof(buff), "ME%02x%02x%02x",
			info->panel, CORE_VERSION_EL, FW_VERSION_EL);
	}

	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_fw_ver_ic(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};

	set_default_result(info);

	if (info->enabled){
		if(get_fw_version(info)<0)
			pr_err("[TSP] : get_fw_version");
	}

	snprintf(buff, sizeof(buff), "ME%02x%02x%02x",
		info->panel, info->fw_core_ver, info->fw_ic_ver);

	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_config_ver(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[20] = {0};

	set_default_result(info);

	if (info->panel == ILJIN)
		snprintf(buff, sizeof(buff), "T311_Me_0608_IJ");
	else if (info->panel == EELY)
		#if defined(CONFIG_MACH_MILLET3G_CHN_OPEN)
		snprintf(buff, sizeof(buff), "SM-T331C_ME_%s", FW_VERSION_DATE);
		#else
		snprintf(buff, sizeof(buff), "T335_ME_0x%02x", FW_VERSION_EL);
		#endif
	else
		snprintf(buff, sizeof(buff), "T311_Me_0608_UN");

	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_threshold(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	int threshold;

	set_default_result(info);

	threshold = i2c_smbus_read_byte_data(info->client, 0x05);
	if (threshold < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = FAIL;
		return;
	}
	snprintf(buff, sizeof(buff), "%d", threshold);

	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}
static int is_melfas_vdd_on(struct mms_ts_info *info)
{
	int ret;

	ret = gpio_get_value(info->pdata->vdd_en);
	pr_info("[TSP] %s = %d\n", __func__, ret);

	if (ret)
		return 1;
	return 0;
}
static void module_off_master(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[3] = {0};

	mutex_lock(&info->lock);
	if (info->enabled) {
		disable_irq_nosync(info->irq);
		info->enabled = false;
		touch_is_pressed = 0;
	}
	mutex_unlock(&info->lock);

	info->power(info, 0);

	if (is_melfas_vdd_on(info) == 0)
		snprintf(buff, sizeof(buff), "%s", "OK");
	else
		snprintf(buff, sizeof(buff), "%s", "NG");

	set_default_result(info);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	if (strncmp(buff, "OK", 2) == 0)
		info->cmd_state = OK;
	else
		info->cmd_state = FAIL;

	dev_info(&info->client->dev, "%s: %s\n", __func__, buff);
}

static void module_on_master(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[3] = {0};

	mms_reset(info);

	mutex_lock(&info->lock);
	if (!info->enabled) {
		enable_irq(info->irq);
		info->enabled = true;
	}
	mutex_unlock(&info->lock);

	if (is_melfas_vdd_on(info) == 1)
		snprintf(buff, sizeof(buff), "%s", "OK");
	else
		snprintf(buff, sizeof(buff), "%s", "NG");

	set_default_result(info);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	if (strncmp(buff, "OK", 2) == 0)
		info->cmd_state = OK;
	else
		info->cmd_state = FAIL;

	dev_info(&info->client->dev, "%s: %s\n", __func__, buff);

}
/*
static void module_off_slave(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	not_support_cmd(info);
}

static void module_on_slave(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	not_support_cmd(info);
}
*/
static void get_module_vendor(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};
	int val,val2;

	set_default_result(info);
	if (!(gpio_get_value(info->pdata->vdd_en) && 
				gpio_get_value(info->pdata->vdd_en2))) {
		dev_err(&info->client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(buff, sizeof(buff), "%s", "TSP turned off");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = NOT_APPLICABLE;
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
		info->cmd_state = OK;
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

		return;
	}
	snprintf(buff, sizeof(buff),  "%s", tostring(NG));
	info->cmd_state = FAIL;
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
}
static void get_chip_vendor(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};

	set_default_result(info);

	snprintf(buff, sizeof(buff), "%s", "MELFAS");
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_chip_name(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};

	set_default_result(info);

	snprintf(buff, sizeof(buff), "%s", "MMS252");
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_reference(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	unsigned int val;
	int node;

	set_default_result(info);
	node = check_rx_tx_num(info);

	if (node < 0)
		return;

	val = info->reference[node];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = OK;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));

}

static void get_cm_abs(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	unsigned int val;
	int node;

	set_default_result(info);
	node = check_rx_tx_num(info);

	if (node < 0)
		return;

	val = info->raw[node];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_cm_delta(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	unsigned int val;
	int node;

	set_default_result(info);
	node = check_rx_tx_num(info);

	if (node < 0)
		return;

	val = info->inspection[node];
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_intensity(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	unsigned int val;
	int node;

	set_default_result(info);
	node = check_rx_tx_num(info);

	if (node < 0)
		return;

	val = info->intensity[node];

	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

#if DEBUG_PRINT2
#define BUF_SIZE PAGE_SIZE

static char get_debug_data[BUF_SIZE];

static int get_intensity1(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	int r, t;
	int ret = 0;
	u8 sz = 0;
	u8 buf[100]={0, };
	u8 reg[4]={ 0, };
	u8 tx_num;
	u8 rx_num;
	u8 key_num;
	s16 cmdata;
	char data[6];
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = reg,
		},{
			.addr = client->addr,
			.flags = I2C_M_RD,
		},
	};
	tx_num = i2c_smbus_read_byte_data(client, 0x0B);
	rx_num = i2c_smbus_read_byte_data(client, 0x0C);
	key_num = i2c_smbus_read_byte_data(client, 0x0D);
	disable_irq_nosync(info->irq);
	memset(get_debug_data,0,BUF_SIZE);
	sprintf(get_debug_data,"%s", "start intensity\n");
	for(r = 0 ; r < rx_num ; r++)
	{
		printk("[%2d]",r);
		sprintf(data,"[%2d]",r);
		strcat(get_debug_data,data);
		memset(data,0,5);

		reg[0] = 0xA0;
		reg[1] = 0x70;
		reg[2] = 0xFF;
		reg[3] = r;
		msg[0].len = 4;

		msleep(1);
		if(i2c_transfer(client->adapter, &msg[0],1)!=1){
			dev_err(&client->dev, "intensity i2c transfer failed\n");
			ret = -1;
			enable_irq(info->irq);
			return ret;
		}

		sz = i2c_smbus_read_byte_data(client, 0xAE);

		reg[0] =0xAF;
		msg[0].len = 1;
		msg[1].len = sz;
		msg[1].buf = buf;


		if(i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg))!=ARRAY_SIZE(msg)){
			ret = -1;
			enable_irq(info->irq);
			return ret;
		}

		sz >>=1;
		for(t = 0 ; t <sz ; t++){
			cmdata = (s16)(buf[2*t] | (buf[2*t+1] << 8));
			printk("%3d",cmdata);
			sprintf(data,"%3d",cmdata);
			strcat(get_debug_data,data);
			memset(data,0,5);
		}
		printk("\n");
		sprintf(data,"\n");
		strcat(get_debug_data,data);
		memset(data,0,5);

	}

	if (key_num)
	{
		printk("---key intensity---\n");
		strcat(get_debug_data,"key intensity\n");
		memset(data,0,5);

		reg[0] = 0xA0;
		reg[1] = 0x71;
		reg[2] = 0xFF;
		reg[3] = 0x00;
		msg[0].len = 4;

		if(i2c_transfer(client->adapter, &msg[0],1)!=1){
			dev_err(&client->dev, "Cm delta i2c transfer failed\n");
			ret = -1;
			enable_irq(info->irq);
			return ret;
		}

		while (gpio_get_value(info->pdata->gpio_int)){
		}

		sz = i2c_smbus_read_byte_data(info->client, 0xAE);

		reg[0] =0xAF;
		msg[0].len = 1;
		msg[1].len = sz;
		msg[1].buf = buf;
		if(i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg))!=ARRAY_SIZE(msg)){
			ret = -1;
			enable_irq(info->irq);
			return ret;
		}
		for(t = 0; t< key_num; t++){
			cmdata = (s16)(buf[2*t] | (buf[2*t+1] << 8));
			printk("%5d",cmdata);
			sprintf(data,"%5d",cmdata);
			strcat(get_debug_data,data);
			memset(data,0,5);
		}
		printk("\n");
		sprintf(data,"\n");
		strcat(get_debug_data,data);
		memset(data,0,5);

	}
	enable_irq(info->irq);

	return 0;
}

static ssize_t mms_intensity(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	int ret;
	dev_info(&info->client->dev, "Intensity Test\n");
	if(get_intensity1(info)!=0){
		dev_err(&info->client->dev, "Intensity Test failed\n");
		return -1;
	}
	ret = snprintf(buf,BUF_SIZE,"%s\n",get_debug_data);
	return ret;
}

static DEVICE_ATTR(intensity, 0664, mms_intensity, NULL);

static struct attribute *mms_attrs[] = {
	&dev_attr_intensity.attr,
	NULL,
};

static const struct attribute_group mms_attr_group = {
	.attrs = mms_attrs,
};
#endif

static int using = 0;
static struct mms_ts_info *ts = NULL;

void dump_tsp_log(void){
#if DEBUG_PRINT2
	if(using ==0){
		using = 1;
		dev_info(&ts->client->dev, "demp_tsp_log, Intensity value\n");
		if(get_intensity1(ts)!=0){
			dev_err(&ts->client->dev, "Intensity Test failed\n");
		}
		using = 0;
	}else{
		dev_err(&ts->client->dev, "Intensity Testing. wait.\n");
	}
#else
	dev_info(&ts->client->dev, "demp_tsp_log, not debug2 mode\n");
#endif
}
EXPORT_SYMBOL(dump_tsp_log);

static void get_x_num(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	u32 val;
	u8 r_buf[2];
	int ret;

	set_default_result(info);

	ret = i2c_smbus_read_i2c_block_data(info->client,
		ADDR_CH_NUM, 2, r_buf);
	val = r_buf[0];
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = FAIL;

		dev_info(&info->client->dev,
			"%s: fail to read num of x (%d).\n",
			__func__, val);
		return;
	}

	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_y_num(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	u32 val;
	u8 r_buf[2];
	int ret;

	set_default_result(info);

	ret = i2c_smbus_read_i2c_block_data(info->client,
		ADDR_CH_NUM, 2, r_buf);
	val = r_buf[1];
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = FAIL;

		dev_info(&info->client->dev,
			"%s: fail to read num of x (%d).\n",
			__func__, val);
		return;
	}

	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = OK;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void run_reference_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);
	if(!get_raw_data(info, MMS_VSC_CMD_REFER))
		info->cmd_state = OK;
	else
		info->cmd_state = NG;

/*	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__); */
}

static void run_cm_abs_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);
	if(!get_raw_data(info, MMS_VSC_CMD_CM_ABS))
		info->cmd_state = OK;
	else
		info->cmd_state = NG;

/*	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__); */
}

static void run_cm_delta_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);
	if(!get_raw_data(info, MMS_VSC_CMD_CM_DELTA))
		info->cmd_state = OK;
	else
		info->cmd_state = NG;

/*	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__); */
}

static void run_intensity_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);
	get_intensity_data(info);
	info->cmd_state = OK;

/*	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__); */
}

static ssize_t store_cmd(struct device *dev, struct device_attribute
				  *devattr, const char *buf, size_t count)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;

	char *cur, *start, *end;
	char buff[TSP_CMD_STR_LEN] = {0};
	int len, i;
	struct tsp_cmd *tsp_cmd_ptr = NULL;
	char delim = ',';
	bool cmd_found = false;
	int param_cnt = 0;
	int ret;

	if (strlen(buf) >= TSP_CMD_STR_LEN) {
		dev_err(&info->client->dev, "%s: cmd length is over(%s,%d)!!\n", __func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	if (info->cmd_is_running == true) {
		dev_err(&info->client->dev, "tsp_cmd: other cmd is running.\n");
		goto err_out;
	}


	/* check lock  */
	mutex_lock(&info->cmd_lock);
	info->cmd_is_running = true;
	mutex_unlock(&info->cmd_lock);

	info->cmd_state = RUNNING;

	for (i = 0; i < ARRAY_SIZE(info->cmd_param); i++)
		info->cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;
	memset(info->cmd, 0x00, ARRAY_SIZE(info->cmd));
	memcpy(info->cmd, buf, len);

	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(buff, buf, cur - buf);
	else
		memcpy(buff, buf, len);

	/* find command */
	list_for_each_entry(tsp_cmd_ptr, &info->cmd_list_head, list) {
		if (!strcmp(buff, tsp_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(tsp_cmd_ptr, &info->cmd_list_head, list) {
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
				ret = kstrtoint(buff, 10,\
						info->cmd_param + param_cnt);
				start = cur + 1;
				memset(buff, 0x00, ARRAY_SIZE(buff));
				param_cnt++;
			}
			cur++;
		} while ((cur - buf <= len) && (param_cnt < TSP_CMD_PARAM_NUM));
	}

	dev_info(&client->dev, "cmd = %s\n", tsp_cmd_ptr->cmd_name);
	for (i = 0; i < param_cnt; i++)
		dev_info(&client->dev, "cmd param %d= %d\n", i,
							info->cmd_param[i]);

	tsp_cmd_ptr->cmd_func(info);


err_out:
	return count;
}

static ssize_t show_cmd_status(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	char buff[16] = {0};

	dev_info(&info->client->dev, "tsp cmd: status:%d\n",
			info->cmd_state);

	if (info->cmd_state == WAITING)
		snprintf(buff, sizeof(buff), "WAITING");

	else if (info->cmd_state == RUNNING)
		snprintf(buff, sizeof(buff), "RUNNING");

	else if (info->cmd_state == OK)
		snprintf(buff, sizeof(buff), "OK");

	else if (info->cmd_state == FAIL)
		snprintf(buff, sizeof(buff), "FAIL");

	else if (info->cmd_state == NOT_APPLICABLE)
		snprintf(buff, sizeof(buff), "NOT_APPLICABLE");

	else if (info->cmd_state == NG)
		snprintf(buff, sizeof(buff), "NG");

	return snprintf(buf, TSP_BUF_SIZE, "%s\n", buff);
}

static ssize_t show_cmd_result(struct device *dev, struct device_attribute
				    *devattr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);

	dev_info(&info->client->dev, "tsp cmd: result: %s\n", info->cmd_result);

	mutex_lock(&info->cmd_lock);
	info->cmd_is_running = false;
	mutex_unlock(&info->cmd_lock);

	info->cmd_state = WAITING;

	return snprintf(buf, TSP_BUF_SIZE, "%s\n", info->cmd_result);
}

#if TOUCHKEY
static ssize_t touch_led_control(struct device *dev,
		 struct device_attribute *attr, const char *buf,
		 size_t count)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	u32 data;
	int ret;

	ret = sscanf(buf, "%d", &data);
	//ret = kstrtol(buf,12, &data);
	if (ret != 1) {
		dev_err(&client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(data == 0 || data == 1)) {
		dev_err(&client->dev, "%s: wrong command(%d)\n",
			__func__, data);
		return count;
	}

	if (data == 1) {
		dev_notice(&client->dev, "led on\n");
		info->keyled(info, 1);
		info->led_cmd = true;
	} else {
		dev_notice(&client->dev, "led off\n");
		info->keyled(info, 0);
		info->led_cmd = false;
	}
	return count;
}

static ssize_t touchkey_threshold_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	u8 r_buf[2];
	int i;

	if (!info->enabled)
		return sprintf(buf, "0 0\n");

	for (i = 0; i < 2; i++) {
		i2c_smbus_write_byte_data(info->client, 0x20, i+1);
		r_buf[i] = i2c_smbus_read_byte_data(info->client, 0x20);
	}

	//return sprintf(buf, "%d %d\n", r_buf[0], r_buf[1]);
	return sprintf(buf,"%d\n", r_buf[0]);
}

static void get_intensity_key(struct mms_ts_info *info)
{
	u8 w_buf[4];
	u8 r_buf;
	u8 read_buffer[10] = {0};
	int i;
	int ret;
	u16 raw_data;
	int data;

	if (!info->enabled) {
		info->menu_s = 0;
		info->back_s = 0;
		return;
	}

	disable_irq(info->irq);

	w_buf[0] = ADDR_UNIV_CMD;
	w_buf[1] = CMD_GET_INTEN_KEY;
	w_buf[2] = 0xFF;
	w_buf[3] = 0;

	ret = i2c_smbus_write_i2c_block_data(info->client,
		w_buf[0], 3, &w_buf[1]);
	if (ret < 0)
		goto err_i2c;
	usleep_range(1, 5);

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CMD_RESULT_SZ, 1, &r_buf);
	if (ret < 0)
		goto err_i2c;

	ret = get_data(info, CMD_RESULT, r_buf, read_buffer);
	if (ret < 0)
		goto err_i2c;

	for (i = 0; i < r_buf/2; i++) {
		raw_data = read_buffer[2*i] | (read_buffer[2*i+1] << 8);

		if (raw_data < 32767)
			data = (int)raw_data;
		else
			data = -(65536 - raw_data);

		if (i == 0)
			info->menu_s = data;
		if (i == 1)
			info->back_s = data;
	}

	enable_irq(info->irq);

	return;

err_i2c:
	dev_err(&info->client->dev, "%s: fail to i2c (cmd=%d)\n",
		__func__, MMS_VSC_CMD_INTENSITY);

	info->menu_s = 0;
	info->back_s = 0;

	release_all_fingers(info);
	mms_reset(info);

	enable_irq(info->irq);
}


static ssize_t touchkey_recent_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);

	get_intensity_key(info);

	return sprintf(buf, "%d\n", info->menu_s);
}

static ssize_t touchkey_back_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);

	get_intensity_key(info);

	return sprintf(buf, "%d\n", info->back_s);
}

static ssize_t touchkey_fw_read(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	#if 0
	struct mms_ts_info *info = dev_get_drvdata(dev);

	if(info->panel == ILJIN) {
		return snprintf(buf, 10, "ME%02x%02x%02x",
			info->panel, CORE_VERSION_IJ, FW_VERSION_IJ);
	} else { /* EELY */
		return snprintf(buf, 10, "ME%02x%02x%02x",
			info->panel, CORE_VERSION_EL, FW_VERSION_EL);
	}
	#endif
	return snprintf(buf, 4, "%s", "N");
}

#endif

#ifdef ESD_DEBUG

static bool intensity_log_flag;

static u32 get_raw_data_one(struct mms_ts_info *info, u16 rx_idx, u16 tx_idx,
			    u8 cmd)
{
	u8 w_buf[6];
	u8 read_buffer[2];
	int ret;
	u32 raw_data;

	w_buf[0] = MMS_VSC_CMD;	/* vendor specific command id */
	w_buf[1] = MMS_VSC_MODE;	/* mode of vendor */
	w_buf[2] = 0;		/* tx line */
	w_buf[3] = 0;		/* rx line */
	w_buf[4] = 0;		/* reserved */
	w_buf[5] = 0;		/* sub command */

	if (cmd != MMS_VSC_CMD_INTENSITY && cmd != MMS_VSC_CMD_RAW &&
	    cmd != MMS_VSC_CMD_REFER) {
		dev_err(&info->client->dev, "%s: not profer command(cmd=%d)\n",
			__func__, cmd);
		return RAW_FAIL;
	}

	w_buf[2] = tx_idx;	/* tx */
	w_buf[3] = rx_idx;	/* rx */
	w_buf[5] = cmd;		/* sub command */

	ret = i2c_smbus_write_i2c_block_data(info->client, w_buf[0], 5,
					     &w_buf[1]);
	if (ret < 0)
		goto err_i2c;

	ret = i2c_smbus_read_i2c_block_data(info->client, 0xBF, 2, read_buffer);
	if (ret < 0)
		goto err_i2c;

	raw_data = ((u16) read_buffer[1] << 8) | read_buffer[0];
	if (cmd == MMS_VSC_CMD_REFER)
		raw_data = raw_data >> 4;

	return raw_data;

err_i2c:
	dev_err(&info->client->dev, "%s: fail to i2c (cmd=%d)\n",
		__func__, cmd);
	return RAW_FAIL;
}

static ssize_t show_intensity_logging_on(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;
	struct file *fp;
	char log_data[160] = { 0, };
	char buff[16] = { 0, };
	mm_segment_t old_fs;
	long nwrite;
	u32 val;
	int i, c;
	u32 y;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

#define MELFAS_DEBUG_LOG_PATH "/sdcard/melfas_log"

	dev_info(&client->dev, "%s: start.\n", __func__);
	fp = filp_open(MELFAS_DEBUG_LOG_PATH, O_RDWR | O_CREAT,
		       S_IRWXU | S_IRWXG | S_IRWXO);
	if (IS_ERR(fp)) {
		dev_err(&client->dev, "%s: fail to open log file\n", __func__);
		goto open_err;
	}

	intensity_log_flag = 1;
	do {
		for (y = 0; y < 3; y++) {
			/* for tx chanel 0~2 */
			memset(log_data, 0x00, 160);

			snprintf(buff, 16, "%1u: ", y);
			strncat(log_data, buff, strnlen(buff, 16));

			for (i = 0; i < RX_NUM; i++) {
				val = get_raw_data_one(info, i, y,
						       MMS_VSC_CMD_INTENSITY);
				snprintf(buff, 16, "%5u, ", val);
				strncat(log_data, buff, strnlen(buff, 16));
			}
			memset(buff, '\n', 2);
			c = (y == 2) ? 2 : 1;
			strncat(log_data, buff, c);
			nwrite = vfs_write(fp, (const char __user *)log_data,
					   strnlen(log_data, 160), &fp->f_pos);
		}
		usleep_range(3000, 5000);
	} while (intensity_log_flag);

	filp_close(fp, current->files);
	set_fs(old_fs);

	return 0;

open_err:
	set_fs(old_fs);
	return RAW_FAIL;
}

static ssize_t show_intensity_logging_off(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct mms_ts_info *info = dev_get_drvdata(dev);
	intensity_log_flag = 0;
	usleep_range(10000, 12000);
	get_raw_data_all(info, MMS_VSC_CMD_EXIT);
	return 0;
}

#endif

static DEVICE_ATTR(close_tsp_test, S_IRUGO, show_close_tsp_test, NULL);
static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, store_cmd);
static DEVICE_ATTR(cmd_status, S_IRUGO, show_cmd_status, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, show_cmd_result, NULL);
#if TOUCHKEY
static DEVICE_ATTR(touchkey_threshold, S_IRUGO, touchkey_threshold_show, NULL);
static DEVICE_ATTR(touchkey_recent, S_IRUGO, touchkey_recent_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, touchkey_back_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO,
                                touchkey_fw_read, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO,
                                touchkey_fw_read, NULL);

#endif
#ifdef ESD_DEBUG
static DEVICE_ATTR(intensity_logging_on, S_IRUGO, show_intensity_logging_on,
			NULL);
static DEVICE_ATTR(intensity_logging_off, S_IRUGO, show_intensity_logging_off,
			NULL);
#endif

static struct attribute *sec_touch_facotry_attributes[] = {
	&dev_attr_close_tsp_test.attr,
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
#ifdef ESD_DEBUG
	&dev_attr_intensity_logging_on.attr,
	&dev_attr_intensity_logging_off.attr,
#endif
	NULL,
};

static struct attribute_group sec_touch_factory_attr_group = {
	.attrs = sec_touch_facotry_attributes,
};
#endif /* SEC_TSP_FACTORY_TEST */

#if TOUCHKEY
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
			touch_led_control);

static struct attribute *sec_touchkeyled_attributes[] = {
	&dev_attr_brightness.attr,
	NULL,
};

static struct attribute *sec_touchkey_attributes[] = {
#ifdef SEC_TSP_FACTORY_TEST
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
#endif
	NULL,
};

static struct attribute_group sec_touchkeyled_attr_group = {
	.attrs = sec_touchkeyled_attributes,
};

static struct attribute_group sec_touchkey_attr_group = {
	.attrs = sec_touchkey_attributes,
};
#endif

static int mms_ts_fw_check(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	int ret, retry;
	char buf[4] = { 0, };
	bool update = false;

	ret = i2c_master_recv(client, buf, 1);
	if (ret < 0) {		/* tsp connect check */
		pr_err("%s: i2c fail...[%d], addr[%d]\n",
			   __func__, ret, info->client->addr);
#if defined(CONFIG_MACH_MILLET3G_CHN_OPEN)
		retry = 4;
		while (retry)
		{
		    ret = i2c_master_recv(client, buf, 1);
		    if(ret > 0 && ret == 0)
		    {
		       break;
		    }
		    msleep(10);
		    retry--;		  
		}
		if (retry == 0 && ret < 0)
		{
		    pr_info("[TSP] force update firmware \n");
		    update = true;
		}
#else
		pr_err("%s: tsp driver unload\n", __func__);
		return ret;
#endif	
	}
	if (info->panel == EELY)
		dev_info(&client->dev,
			"EELY panel\n");
	else if (info->panel == ILJIN)
		dev_info(&client->dev,
			"IJLIN panel\n");
	else
		dev_info(&client->dev,
			"unknown panel\n");

	if (info->panel == ILJIN)
	{
#if defined(CONFIG_MACH_MILLET3G_CHN_OPEN)	
		if (!update)	{
		    ret = get_fw_version(info);
		}
		else {
		    ret = 0;
		}
		
#else
		ret = get_fw_version(info);
#endif
		if (ret != 0) {
			dev_err(&client->dev, "fw_version read fail\n");
			update = true;
		}
		if (!update) {
			if (info->fw_boot_ver != BOOT_VERSION_IJ) {
				dev_err(&client->dev,
					"boot version must be 0x%x\n",
					BOOT_VERSION_IJ);
				update = true;
			}
		}
		if (!update) {
			if ((info->fw_core_ver < CORE_VERSION_IJ) ||
				(info->fw_core_ver == 0xff)) {
				dev_err(&client->dev,
					"core version must be 0x%x\n",
					CORE_VERSION_IJ);
				update = true;
			}
		}
		if (!update) {
			if ((info->fw_ic_ver < FW_VERSION_IJ) ||
				(info->fw_ic_ver == 0xff)) {
				dev_err(&client->dev,
					"config version must be over 0x%x\n",
					FW_VERSION_IJ);
				update = true;
			}
		}
	}
	else if (info->panel == EELY)
	{
#if defined(CONFIG_MACH_MILLET3G_CHN_OPEN)
		if (!update){
		    ret = get_fw_version(info);
		}
		else {
		    ret = 0;
		}
#else
		ret = get_fw_version(info);
#endif
		if (ret != 0) {
			dev_err(&client->dev, "fw_version read fail\n");
			update = true;
		}
		if (!update) {
			if (info->fw_boot_ver != BOOT_VERSION_EL) {
				dev_err(&client->dev,
					"boot version must be 0x%x\n",
					BOOT_VERSION_EL);
				update = true;
			}
		}
		if (!update) {
			if ((info->fw_core_ver < CORE_VERSION_EL) ||
				(info->fw_core_ver == 0xff)) {
				dev_err(&client->dev,
					"core version must be 0x%x\n",
					CORE_VERSION_EL);
				update = true;
			}
		}
		if (!update) {
			if ((info->fw_ic_ver < FW_VERSION_EL) ||
				(info->fw_ic_ver == 0xff)) {
				dev_err(&client->dev,
					"config version must be over 0x%x\n",
					FW_VERSION_EL);
				update = true;
			}
		}
	}

	if (update) {
		dev_err(&client->dev, "excute mms_flash_fw\n");
		retry = 4;
		while (retry--) {
			ret = mms_flash_fw(info, true, BUILT_IN);
			if (ret) {
				dev_err(&client->dev,
					"failed to mms_flash_fw (%d)\n", ret);
				dev_err(&client->dev,
					"retry flash (%d)\n", retry);
			} else {
				dev_info(&client->dev,
					"fw update success\n");
				break;
			}
		}
	}

	if (update && !!ret)
		return -1;

	return 0;
}
static void melfas_request_gpio(struct melfas_tsi_platform_data *pdata)
{
	int ret;
	pr_info("[TSP] %s called \n", __func__);

	ret = gpio_request(pdata->gpio_int, "melfas_tsp_irq");
	if (ret) {
		pr_err("[TSP]%s: unable to request melfas_tsp_irq [%d]\n",
				__func__, pdata->gpio_int);
		return;
	}
	ret = gpio_request(pdata->vdd_en, "melfas_vdd_en");
	if (ret) {
		pr_err("[TSP]%s: unable to request melfas_vdd_en [%d]\n",
				__func__, pdata->vdd_en);
		return;
	}
	ret = gpio_request(pdata->vdd_en2, "melfas_vdd_en2");
	if (ret) {
		pr_err("[TSP]%s: unable to request melfas_vdd_en2 [%d]\n",
				__func__, pdata->vdd_en2);
		return;
	}
	ret = gpio_request(pdata->tsp_vendor1, "melfas_tsp_vendor1");
	if (ret) {
		pr_err("[TSP]%s: unable to request melfas_vdd_en2 [%d]\n",
				__func__, pdata->tsp_vendor1);
		return;
	}
	ret = gpio_request(pdata->tsp_vendor2, "melfas_tsp_vendor2");
	if (ret) {
		pr_err("[TSP]%s: unable to request melfas_vdd_en2 [%d]\n",
				__func__, pdata->tsp_vendor2);
		return;
	}
	if(pdata->tkey_led_en >= 0){
		ret = gpio_request(pdata->tkey_led_en, "tkey_enable");
		if (ret) {
			pr_err("[TSP]%s: unable to request tkey_enable [%d]\n",
					__func__, pdata->tkey_led_en);
			return;
		}
	}
}

#ifdef CONFIG_OF

static int mms_get_dt_coords(struct device *dev, char *name,
				struct melfas_tsi_platform_data *pdata)
{
	u32 coords[MMS_COORDS_ARR_SIZE];
	struct property *prop;
	struct device_node *np = dev->of_node;
	int coords_size, rc;

	prop = of_find_property(np, name, NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;

	coords_size = prop->length / sizeof(u32);
	if (coords_size != MMS_COORDS_ARR_SIZE) {
		dev_err(dev, "invalid %s\n", name);
		return -EINVAL;
	}

	rc = of_property_read_u32_array(np, name, coords, coords_size);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read %s\n", name);
		return rc;
	}

	if (strncmp(name, "melfas,panel-coords",
			sizeof("melfas,panel-coords")) == 0) {
		pdata->invert_x = coords[0];
		pdata->invert_y = coords[1];
		pdata->max_x = coords[2];
		pdata->max_y = coords[3];
	} else {
		dev_err(dev, "unsupported property %s\n", name);
		return -EINVAL;
	}

	return 0;
}

static int  get_panel_version(struct mms_ts_info *info){

	struct regulator *regulator_pullup;
	int rc = 0;

	regulator_pullup = regulator_get(NULL, "8226_l6");
	if (IS_ERR(regulator_pullup))
		return PTR_ERR(regulator_pullup);
	regulator_set_voltage(regulator_pullup, 1800000, 1800000);
	regulator_enable(regulator_pullup);
	gpio_tlmm_config(GPIO_CFG(info->pdata->tsp_vendor1,0,GPIO_CFG_INPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(info->pdata->tsp_vendor2,0,GPIO_CFG_INPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (gpio_get_value(info->pdata->tsp_vendor1))
		info->pdata->panel = 1;
	else
		info->pdata->panel = 0;

	if (gpio_get_value(info->pdata->tsp_vendor2))
		info->pdata->panel = info->pdata->panel | (1 << 1);

	regulator_disable(regulator_pullup);
	return rc;
}

int melfas_power(struct mms_ts_info *info, int on){

	if (tsp_power_enabled == on)
		return 0;
	printk(KERN_DEBUG "[TSP] %s %s\n",
				__func__, on ? "on" : "off");
	gpio_tlmm_config(GPIO_CFG(info->pdata->vdd_en,0,GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(info->pdata->vdd_en2,0,GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(info->pdata->vdd_en,on);
	gpio_set_value(info->pdata->vdd_en2,on);
	msleep(20);
	tsp_power_enabled = on;
	return 0;
}

#if TOUCHKEY
int key_led_control(struct mms_ts_info *info, int on)
{

	printk(KERN_DEBUG "[TSP] %s %s\n",
		__func__, on ? "on" : "off");
	if(info->pdata->tkey_led_en >= 0){
		gpio_set_value(info->pdata->tkey_led_en,on);
	}
	return 0;
}

#endif
static int mms_parse_dt(struct device *dev,
			struct melfas_tsi_platform_data *pdata)
{
	int rc;
	struct device_node *np = dev->of_node;

	rc = mms_get_dt_coords(dev, "melfas,panel-coords", pdata);
	if (rc)
		return rc;

	/* regulator info */
	pdata->i2c_pull_up = of_property_read_bool(np, "melfas,i2c-pull-up");
	pdata->vdd_en = of_get_named_gpio(np, "vdd_en-gpio", 0);
	pdata->vdd_en2 = of_get_named_gpio(np, "vdd_en2-gpio", 0);
	pdata->tkey_led_en = of_get_named_gpio(np, "tkey_en-gpio", 0);
	if(pdata->tkey_led_en < 0){
		pr_info("[TSP] error %d requesting ledgpio, ignoring\n",
							pdata->tkey_led_en);
	}
	pdata->tsp_vendor1 = of_get_named_gpio(np, "tsp_vendor1-gpio", 0);
	pdata->tsp_vendor2 = of_get_named_gpio(np, "tsp_vendor2-gpio", 0);

	if(of_property_read_u32(np, "melfas,key1", &pdata->key1))
		pdata->key1 = KEY_MENU;

	/* reset, irq gpio info */
	//pdata->gpio_scl = of_get_named_gpio_flags(np, "melfas,scl-gpio",
	//			0, &pdata->scl_gpio_flags);
	//pdata->gpio_sda = of_get_named_gpio_flags(np, "melfas,sda-gpio",
	//			0, &pdata->sda_gpio_flags);
	pdata->gpio_int = of_get_named_gpio_flags(np, "melfas,irq-gpio",
				0, &pdata->irq_gpio_flags);
	pdata->config_fw_version = of_get_property(np,
				"melfas,config_fw_version", NULL);

	return 0;
}
#else
static int mms_parse_dt(struct device *dev,
			struct melfas_tsi_platform_data *pdata)
{
	return -ENODEV;
}
#endif

#if 0
static ssize_t melfas_enabled_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	long input;
	int ret = 0;

	ret = kstrtol(buf, 10, &input);
	if(ret || ret < 0){
            printk(KERN_INFO "Error value melfas_enabled_store \n");
            return ret;
        }
	pr_info("TSP enabled: %d\n",(int)input);

	if(input == 1)
		mms_ts_resume(dev);
	else if(input == 0)
		mms_ts_suspend(dev);
	else
		return -EINVAL;

	return count;
}

static struct device_attribute attrs[] = {
	__ATTR(enabled, (S_IRUGO | S_IWUSR | S_IWGRP),
			NULL,
			melfas_enabled_store),
};
#endif

/* START - Added to support API's for TSP tuning */
static int mms_fs_open(struct inode *node, struct file *fp)
{
	struct mms_ts_info *info;
	struct i2c_client *client;
	struct i2c_msg msg;
	u8 buf[3] = {
		ADDR_UNIV_CMD,//MMS_UNIVERSAL_CMD,
		MMS_CMD_SET_LOG_MODE,
		true,
	};

	info = container_of(node->i_cdev, struct mms_ts_info, cdev);
	client = info->client;

	disable_irq(info->irq);
	fp->private_data = info;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.buf = buf;
	msg.len = sizeof(buf);

	i2c_transfer(client->adapter, &msg, 1);

	info->log.data = kzalloc(MAX_LOG_LENGTH * 20 + 5, GFP_KERNEL);

	//mms_clear_input_data(info);
	touch_is_pressed = 0;
	release_all_fingers(info);

	return 0;
}

static int mms_fs_release(struct inode *node, struct file *fp)
{
	struct mms_ts_info *info = fp->private_data;

//	mms_clear_input_data(info);
	touch_is_pressed = 0;
	release_all_fingers(info);

	//mms_reboot(info);
	mms_reset(info);

	kfree(info->log.data);
	enable_irq(info->irq);

	return 0;
}

static int esd_cnt;
static void mms_report_input_data(struct mms_ts_info *info, u8 sz, u8 *buf)
{
	int i;
	struct i2c_client *client = info->client;
	int id;
	int x;
	int y;
	int touch_major;
	int pressure;
	int key_code;
	int key_state;
	u8 *tmp;

	if (buf[0] == MMS_NOTIFY_EVENT) {
		dev_info(&client->dev, "TSP mode changed (%d)\n", buf[1]);
		goto out;
	} else if (buf[0] == MMS_ERROR_EVENT) {
		dev_info(&client->dev, "Error detected, restarting TSP\n");
		//mms_clear_input_data(info);
		touch_is_pressed = 0;
		release_all_fingers(info);

		//mms_reboot(info);
		mms_reset(info);
		esd_cnt++;
		if (esd_cnt>= ESD_DETECT_COUNT)
		{
			i2c_smbus_write_byte_data(info->client, MMS_MODE_CONTROL, 0x04);
			esd_cnt = 0;
		}
		goto out;
	}

	for (i = 0; i < sz; i += FINGER_EVENT_SZ) {
		tmp = buf + i;
		esd_cnt = 0;
		if (tmp[0] & MMS_TOUCH_KEY_EVENT) {
			switch (tmp[0] & 0xf) {
			case 1:
				key_code = info->pdata->key1;
				break;
			case 2:
				key_code = KEY_BACK;
				break;
			default:
				dev_err(&client->dev, "unknown key type\n");
				goto out;
				break;
			}

			key_state = (tmp[0] & 0x80) ? 1 : 0;
			input_report_key(info->input_dev, key_code, key_state);

		} else {
			id = (tmp[0] & 0xf) -1;
			x = tmp[2] | ((tmp[1] & 0xf) << 8);
			y = tmp[3] | (((tmp[1] >> 4 ) & 0xf) << 8);
			touch_major = tmp[4];
			pressure = tmp[5];

			input_mt_slot(info->input_dev, id);

			if (!(tmp[0] & 0x80)) {
				input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, false);
				continue;
			}

			input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, true);
			input_report_abs(info->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(info->input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(info->input_dev, ABS_MT_TOUCH_MAJOR, touch_major);
			input_report_abs(info->input_dev, ABS_MT_PRESSURE, pressure);
		}
	}

	input_sync(info->input_dev);

out:
	return;

}

static void mms_event_handler(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	int sz;
	int ret;
	int row_num;
	u8 reg = MMS_INPUT_EVENT;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = &reg,
			.len = 1,
		}, {
			.addr = client->addr,
			.flags = I2C_M_RD,
			.buf = info->log.data,
		},

	};
	struct mms_log_pkt {
		u8	marker;
		u8	log_info;
		u8	code;
		u8	element_sz;
		u8	row_sz;
	} __attribute__ ((packed)) *pkt = (struct mms_log_pkt *)info->log.data;

	memset(pkt, 0, sizeof(*pkt));

	//if (gpio_get_value(info->pdata->gpio_resetb))
	//	return;

	sz = i2c_smbus_read_byte_data(client, MMS_EVENT_PKT_SZ);
	msg[1].len = sz;

	ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (ret != ARRAY_SIZE(msg)) {
		dev_err(&client->dev,
			"failed to read %d bytes of data\n",
			sz);
		return;
	}

	if ((pkt->marker & 0xf) == MMS_LOG_EVENT) {
		if ((pkt->log_info & 0x7) == 0x1) {
			pkt->element_sz = 0;
			pkt->row_sz = 0;

			return;
		}

		switch (pkt->log_info >> 4) {
		case LOG_TYPE_U08:
		case LOG_TYPE_S08:
			msg[1].len = pkt->element_sz;
			break;
		case LOG_TYPE_U16:
		case LOG_TYPE_S16:
			msg[1].len = pkt->element_sz * 2;
			break;
		case LOG_TYPE_U32:
		case LOG_TYPE_S32:
			msg[1].len = pkt->element_sz * 4;
			break;
		default:
			dev_err(&client->dev, "invalied log type\n");
			return;
		}

		msg[1].buf = info->log.data + sizeof(struct mms_log_pkt);
		reg = CMD_RESULT;//MMS_UNIVERSAL_RESULT;
		row_num = pkt->row_sz ? pkt->row_sz : 1;

		while (row_num--) {
			//while (gpio_get_value(info->pdata->gpio_resetb))
			//	;
			ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
			msg[1].buf += msg[1].len;
		};
	} else {
		mms_report_input_data(info, sz, info->log.data);
		memset(pkt, 0, sizeof(*pkt));
	}

	return;
}

static ssize_t mms_fs_read(struct file *fp, char *rbuf, size_t cnt, loff_t *fpos)
{
	struct mms_ts_info *info = fp->private_data;
	struct i2c_client *client = info->client;
	int ret = 0;
	char rx_num, tx_num;

	switch (info->log.cmd) {
	case GET_RX_NUM:
		rx_num = RX_NUM;
		ret = copy_to_user(rbuf, &rx_num, 1);
		break;
	case GET_TX_NUM:
		tx_num = TX_NUM;
		ret = copy_to_user(rbuf, &tx_num, 1);
		break;
	case GET_EVENT_DATA:
		mms_event_handler(info);
		/* copy data without log marker */
		ret = copy_to_user(rbuf, info->log.data + 1, cnt);
		break;
	default:
		dev_err(&client->dev, "unknown command\n");
		ret = -EFAULT;
		break;
	}

	return ret;

}

static ssize_t mms_fs_write(struct file *fp, const char *wbuf, size_t cnt, loff_t *fpos)
{
	struct mms_ts_info *info = fp->private_data;
	struct i2c_client *client = info->client;
	u8 *buf;
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.len = cnt,
	};
	int ret = 0;

	mutex_lock(&info->lock);

	if (!info->enabled)
		goto tsp_disabled;

	msg.buf = buf = kzalloc(cnt + 1, GFP_KERNEL);

	if ((buf == NULL) || copy_from_user(buf, wbuf, cnt)) {
		dev_err(&client->dev, "failed to read data from user\n");
		ret = -EIO;
		goto out;
	}

	if (cnt == 1) {
		info->log.cmd = *buf;
	} else {
		if (i2c_transfer(client->adapter, &msg, 1) != 1) {
			dev_err(&client->dev, "failed to transfer data\n");
			ret = -EIO;
			goto out;
		}
	}

	ret = 0;

out:
	kfree(buf);
tsp_disabled:
	mutex_unlock(&info->lock);

	return ret;
}

static struct file_operations mms_fops = {
	.owner		= THIS_MODULE,
	.open		= mms_fs_open,
	.release	= mms_fs_release,
	.read		= mms_fs_read,
	.write		= mms_fs_write,
};
/* END - Added to support API's for TSP tuning */

#ifdef USE_OPEN_CLOSE
static int mms_ts_input_open(struct input_dev *dev)
{
	struct mms_ts_info *info;

	pr_info("[TSP] %s\n", __func__);
	info = input_get_drvdata(dev);
	return mms_ts_resume(&info->client->dev);
}
static void mms_ts_input_close(struct input_dev *dev)
{
	struct mms_ts_info *info;

	pr_info("[TSP] %s\n", __func__);
	info = input_get_drvdata(dev);
	mms_ts_suspend(&info->client->dev);
}
#endif

void melfas_register_callback(void *);

#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
extern int get_lcd_attached(void);
#endif
static int __devinit mms_ts_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct melfas_tsi_platform_data *pdata;
	struct mms_ts_info *info;
	struct input_dev *input_dev;
	int ret = 0;
	int error;

#ifdef SEC_TSP_FACTORY_TEST
	int i;
	struct device *fac_dev_ts;
#endif
#if TOUCHKEY
	struct device *touchkey_dev;
#endif
	touch_is_pressed = 0;
#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
	if (get_lcd_attached() == 0) {
		dev_err(&client->dev, "%s : get_lcd_attached()=0 \n", __func__);
		return -EIO;
	}
#endif

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct melfas_tsi_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		error = mms_parse_dt(&client->dev, pdata);
		if (error)
			return error;

		pdata->register_cb = melfas_register_callback;
	} else
		pdata = client->dev.platform_data;

	if (!pdata)
		return -EINVAL;
	melfas_request_gpio(pdata);

	info = kzalloc(sizeof(struct mms_ts_info), GFP_KERNEL);
	if (!info) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	ts = info;

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "Failed to allocate memory for input device\n");
		ret = -ENOMEM;
		goto err_input_alloc;
	}
	info->client = client;
	info->input_dev = input_dev;
	info->pdata = pdata;
	if(pdata->tkey_led_en >= 0)
		info->keyled = key_led_control;
	ret = get_panel_version(info);
	if(ret < 0)
		printk(KERN_INFO "get_panel_version error" );
	info->irq = -1;
	mutex_init(&info->lock);

	info->max_x = info->pdata->max_x;
	info->max_y = info->pdata->max_y;
	info->invert_x = info->pdata->invert_x;
	info->invert_y = info->pdata->invert_y;
	info->input_event = info->pdata->input_event;
	info->register_cb = info->pdata->register_cb;
	info->threewave_mode = false;
	info->power = melfas_power;
#if TOUCHKEY
	info->keycode[0] = 0;
	info->keycode[1] = pdata->key1;
	info->keycode[2] = KEY_BACK;

	for (i = 0; i < 3; i++)
		info->touchkey[i] = 0;

	info->led_cmd = false;
	if(info->keyled)
		info->keyled(info, 0);
	info->menu_s = 0;
	info->back_s = 0;
#endif
	i2c_set_clientdata(client, info);
	if (info->power == NULL) {
		dev_err(&client->dev,
			"missing power control\n");
		goto err_config;
	} else {
		info->power(info, 1);
		msleep(100);
	}

	info->panel = info->pdata->panel;

	#if defined(CONFIG_MACH_MILLET3G_EUR)
		if ((info->panel == 0) && (system_rev < 2))
			info->panel = ILJIN;
	#endif

	printk(KERN_INFO "%s: [TSP] panel = %d!!\n", __func__, info->panel);
	printk("%s: [TSP] system_rev = %d\n", __func__, system_rev);
	dev_info(&client->dev, "%d panel\n", info->panel);

	//mms_reset(info); //need to check

	ret = mms_ts_fw_check(info);
	if (ret) {
		dev_err(&client->dev,
			"failed to initialize (%d)\n", ret);
		goto err_reg_input_dev;
	}

	info->callbacks.inform_charger = melfas_ta_cb;
	if (info->register_cb)
		info->register_cb(&info->callbacks);

	snprintf(info->phys, sizeof(info->phys),
		 "%s/input0", dev_name(&client->dev));
	input_dev->name = "sec_touchscreen";
	input_dev->phys = info->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
#if TOUCHKEY
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(pdata->key1, input_dev->keybit);
	set_bit(KEY_BACK, input_dev->keybit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);
#endif
	input_mt_init_slots(input_dev, MAX_FINGERS);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
				0, MAX_PRESSURE, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR,
				0, MAX_PRESSURE, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
				0, (info->max_x)-1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
				0, (info->max_y)-1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR,
				0, MAX_WIDTH, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PALM,
				0, 1, 0, 0);
#ifdef USE_OPEN_CLOSE
	input_dev->open = mms_ts_input_open;
	input_dev->close = mms_ts_input_close;
#endif
	input_set_drvdata(input_dev, info);

	ret = input_register_device(input_dev);
	if (ret) {
		dev_err(&client->dev, "failed to register input dev (%d)\n",
			ret);
		goto err_reg_input_dev;
	}

#ifdef TOUCH_BOOSTER_DVFS
	samsung_init_dvfs(info);
#endif

	info->enabled = true;

	client->irq = gpio_to_irq(pdata->gpio_int);
	ret = request_threaded_irq(client->irq, NULL, mms_ts_interrupt,
				   IRQF_TRIGGER_LOW  | IRQF_ONESHOT,
				   MELFAS_TS_NAME, info);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to register interrupt\n");
		goto err_req_irq;
	}
	info->irq = client->irq;

#ifdef CONFIG_HAS_EARLYSUSPEND
	info->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING;
	info->early_suspend.suspend = mms_ts_early_suspend;
	info->early_suspend.resume = mms_ts_late_resume;
	register_early_suspend(&info->early_suspend);
#endif

/* START - Added to support API's for TSP tuning */
	if (alloc_chrdev_region(&info->mms_dev, 0, 1, "mms_ts")) {
		dev_err(&client->dev, "failed to allocate device region\n");
		return -ENOMEM;
	}

	cdev_init(&info->cdev, &mms_fops);
	info->cdev.owner = THIS_MODULE;

	if (cdev_add(&info->cdev, info->mms_dev, 1)) {
		dev_err(&client->dev, "failed to add ch dev\n");
		return -EIO;
	}

	info->class = class_create(THIS_MODULE, "mms_ts");
	device_create(info->class, NULL, info->mms_dev, NULL, "mms_ts");
/* END - Added to support API's for TSP tuning */

	sec_touchscreen = device_create(sec_class,
					NULL, 0, info, "sec_touchscreen");
	if (IS_ERR(sec_touchscreen)) {
		dev_err(&client->dev,
			"Failed to create device for the sysfs1\n");
		ret = -ENODEV;
	}

#if DEBUG_PRINT2
	if (sysfs_create_group(&sec_touchscreen->kobj, &mms_attr_group)) {
		dev_err(&client->dev, "failed to create sysfs group, debug2 \n");
		return -EAGAIN;
	}
/*	if (sysfs_create_link(NULL, &sec_touchscreen->kobj, "sec_touchscreen")) {
		dev_err(&client->dev, "failed to create sysfs symlink, debug2 \n");
		return -EAGAIN;
	}*/

#endif

	#if 0
	ret = sysfs_create_file(&info->input_dev->dev.kobj,
			&attrs[0].attr);
	if (ret < 0) {
		dev_err(&client->dev,
				"%s: Failed to create sysfs attributes\n",
				__func__);
	}
	#endif
#if TOUCHKEY
	touchkey_dev = device_create(sec_class,
		NULL, 0, info, "sec_touchkey");

	if (IS_ERR(touchkey_dev))
		dev_err(&client->dev,
		"Failed to create device for the touchkey sysfs\n");

	ret = sysfs_create_group(&touchkey_dev->kobj,
		&sec_touchkey_attr_group);
	if (ret)
		dev_err(&client->dev, "Failed to create sysfs group\n");

	if(info->keyled){
		ret = sysfs_create_group(&touchkey_dev->kobj,
			&sec_touchkeyled_attr_group);
		if (ret)
			dev_err(&client->dev, "Failed to create sysfs group\n");
	}

#endif

#ifdef SEC_TSP_FACTORY_TEST
	INIT_LIST_HEAD(&info->cmd_list_head);
	for (i = 0; i < ARRAY_SIZE(tsp_cmds); i++)
		list_add_tail(&tsp_cmds[i].list, &info->cmd_list_head);

	mutex_init(&info->cmd_lock);
	info->cmd_is_running = false;

	fac_dev_ts = device_create(sec_class,
		NULL, 0, info, "tsp");
	if (IS_ERR(fac_dev_ts))
		dev_err(&client->dev,
		"Failed to create device for the tsp sysfs\n");

	ret = sysfs_create_group(&fac_dev_ts->kobj,
		 &sec_touch_factory_attr_group);
	if (ret)
		dev_err(&client->dev, "Failed to create sysfs group\n");

	ret = sysfs_create_link(&fac_dev_ts->kobj, &info->input_dev->dev.kobj, "input");
	if (ret < 0) {
		dev_err(&client->dev,
				"%s: Failed to create input symbolic link\n",
				__func__);
	}
#endif
	return 0;

err_req_irq:
	input_unregister_device(input_dev);
err_reg_input_dev:
	info->power(info,0);
#if TOUCHKEY
	if(info->keyled)
		info->keyled(info, 0);
#endif
err_config:
	input_free_device(input_dev);
	/*input_dev = NULL;*/
err_input_alloc:
	kfree(info);
err_alloc:
	return ret;

}

static int __devexit mms_ts_remove(struct i2c_client *client)
{
	struct mms_ts_info *info = i2c_get_clientdata(client);

	if (info->enabled)
		info->power(info,0);

#if DEBUG_PRINT2
	sysfs_remove_link(NULL, "sec_touchscreen");
	sysfs_remove_group(&client->dev.kobj, &mms_attr_group);
#endif

#if TOUCHKEY
	if (info->led_cmd)
		if(info->keyled)
			info->keyled(info, 0);
#endif
	unregister_early_suspend(&info->early_suspend);
	if (info->irq >= 0)
		free_irq(info->irq, info);
	input_unregister_device(info->input_dev);
	kfree(info);

	return 0;
}

static void mms_ts_shutdown(struct i2c_client *client)
{
	struct mms_ts_info *info = i2c_get_clientdata(client);

	if (info->enabled)
		info->power(info,0);
#if TOUCHKEY
	if (info->led_cmd)
		if(info->keyled)
			info->keyled(info, 0);
#endif
}

#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
static int mms_ts_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mms_ts_info *info = i2c_get_clientdata(client);

	mutex_lock(&info->lock);
	if (!info->enabled)
		goto out;
	info->enabled = false;
	disable_irq_nosync(info->irq);

	dev_notice(&info->client->dev, "%s: users=%d\n", __func__,
		   info->input_dev->users);

	touch_is_pressed = 0;
	release_all_fingers(info);
	info->power(info,0);
	info->sleep_wakeup_ta_check = info->ta_status;
#if TOUCHKEY
	if (info->led_cmd == true) {
		if(info->keyled)
			info->keyled(info, 0);
		info->led_cmd = false;
	}
#endif
	/* This delay needs to prevent unstable POR by
	rapid frequently pressing of PWR key. */
	msleep(50);

#ifdef TOUCH_BOOSTER_DVFS
	samsung_set_dvfs_lock(info, -1);
	dev_info(&info->client->dev,
			"%s: dvfs_lock free.\n", __func__);
#endif
out:
	mutex_unlock(&info->lock);
	return 0;
}

static int mms_ts_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mms_ts_info *info = i2c_get_clientdata(client);

	if (info->enabled)
		return 0;

	dev_notice(&info->client->dev, "%s: users=%d\n", __func__,
		   info->input_dev->users);
	info->power(info, 1);

	if (info->threewave_mode)
		mms_set_threewave_mode(info);

	if (info->ta_status) {
		dev_notice(&client->dev, "TA connect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x33, 0x1);
	} else {
		dev_notice(&client->dev, "TA disconnect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x33, 0x2);
	}
	info->enabled = true;
	mms_set_noise_mode(info);

	/* Because irq_type by EXT_INTxCON register is changed to low_level
	 *  after wakeup, irq_type set to falling edge interrupt again.
	 */
	enable_irq(info->irq);

	return 0;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mms_ts_early_suspend(struct early_suspend *h)
{
	struct mms_ts_info *info;
	info = container_of(h, struct mms_ts_info, early_suspend);
	mms_ts_suspend(&info->client->dev);

}

static void mms_ts_late_resume(struct early_suspend *h)
{
	struct mms_ts_info *info;
	info = container_of(h, struct mms_ts_info, early_suspend);
	mms_ts_resume(&info->client->dev);
}
#endif

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static const struct dev_pm_ops mms_ts_pm_ops = {
	.suspend = mms_ts_suspend,
	.resume = mms_ts_resume,
#ifdef CONFIG_HIBERNATION
	.freeze = mms_ts_suspend,
	.thaw = mms_ts_resume,
	.restore = mms_ts_resume,
#endif
};
#endif

static const struct i2c_device_id mms_ts_id[] = {
	{MELFAS_TS_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, mms_ts_id);

#ifdef CONFIG_OF
static struct of_device_id mms_match_table[] = {
	{ .compatible = "melfas,mms252-ts",},
	{ },
};
#else
#define mms_match_table NULL
#endif
static struct i2c_driver mms_ts_driver = {
	.probe = mms_ts_probe,
	.remove = __devexit_p(mms_ts_remove),
	.shutdown = mms_ts_shutdown,
	.driver = {
		   .name = MELFAS_TS_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = mms_match_table,
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
		   .pm = &mms_ts_pm_ops,
#endif
	},
	.id_table = mms_ts_id,
};

static int __devinit mms_ts_init(void)
{
	if (poweroff_charging) {
		printk("%s : LPM Charging Mode!!\n", __func__);
		return 0;
	}
	else	{
		return i2c_add_driver(&mms_ts_driver);
	}
}

static void __exit mms_ts_exit(void)
{
	i2c_del_driver(&mms_ts_driver);
}

module_init(mms_ts_init);
module_exit(mms_ts_exit);

/* Module information */
MODULE_DESCRIPTION("Touchscreen driver for Melfas MMS-series controllers");
MODULE_LICENSE("GPL");
