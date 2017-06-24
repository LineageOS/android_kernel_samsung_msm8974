/*
 * Copyright (C) 2010 Melfas, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/i2c/mms134S_ts.h>
#include <linux/mfd/pmic8058.h>
#include <linux/cpufreq.h>
#include <asm-generic/uaccess.h>
#include <asm/unaligned.h>

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>

#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/string.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/firmware.h>
#include <linux/input/mt.h>
#include <asm/io.h>

#include <../mach-msm/smd_private.h>
#include <../mach-msm/smd_rpcrouter.h>


#define USE_OPEN_CLOSE
#define TOUCH_BOOSTER
#undef TSP_PATTERN_TRACTKING

#define SEC_TSP_FACTORY_TEST
#define SEC_TKEY_FACTORY_TEST

#define TSP_BUF_SIZE 1024
#undef ENABLE_NOISE_TEST_MODE

#define FW_IMAGE_NAME_UMS		"/sdcard/melfas.fw"
#define FW_IMAGE_NAME_OLD		"tsp_melfas/kanas_mms134s_old.fw"
#define FW_IMAGE_NAME_NEW		"tsp_melfas/kanas_mms134s_new.fw"
#define FW_IMAGE_NAME_NULL		NULL

#define TS_WRITE_REGS_LEN		16
#define TS_READ_REGS_LEN		66

#ifdef TOUCH_BOOSTER
#define TOUCH_BOOSTER_OFF_TIME		100
#define TOUCH_BOOSTER_CHG_TIME		200
#endif

#define TS_MAX_Z_TOUCH			255
#define TS_MAX_W_TOUCH			100

#define FW_IMAGE_NAME_UMS		"/sdcard/melfas.fw"
#define FW_IMAGE_NAME_OLD		"tsp_melfas/kanas_mms134s_old.fw"
#define FW_IMAGE_NAME_NEW		"tsp_melfas/kanas_mms134s_new.fw"
#define FW_IMAGE_NAME_NULL		NULL

#define NUM_OF_TOUCHKEY			2
#define PRESS_KEY			1
#define RELEASE_KEY			0
#define MELFAS_MAX_TOUCH		5
#define DOWNLOAD_RETRY_CNT		1

/* Melfas RMI Map */
#define MMS_MIP_CONTACT_ON_EVENT_THRES	0x05
#define MMS_MIP_MOVING_EVENT_THRES	0x06
#define MMS_MIP_ACTIVE_REPORT_RATE	0x07
#define MMS_MIP_POSITION_FILTER_LEVEL	0x08
#define MMS_MIP_EVENT_PACKET_LENGTH	0x0F
#define MMS_MIP_EVENT_PACKET		0x10

#define MMS_CMD_ENTER_ISC		0x5F
#define MMS_FW_VERSION			0xE1

#define MMS_UNIVERSAL_CMD		0xA0
#define MMS_UNIVERSAL_RESULT		0xAF
#define MMS_UNIVERSAL_RESULT_LENGTH	0xAE

#define MMS_UNIV_ENTER_TEST		0x40
#define MMS_UNIV_TEST_CM_DELTA		0x41
#define MMS_UNIV_GET_CM_DELTA		0x42
#define MMS_UNIV_TEST_CM_ABS		0x43
#define MMS_UNIV_GET_CM_ABS		0x44
#define MMS_UNIV_TEST_CM_JITTER		0x45
#define MMS_UNIV_GET_CM_JITTER		0x46
#define MMS_UNIV_KEY_GET_CM_DELTA	0x4A
#define MMS_UNIV_KEY_GET_CM_ABS		0x4B
#define MMS_UNIV_KEY_GET_CM_JITTER	0x4C
#define MMS_UNIV_EXIT_TEST		0x4F

#define MMS_UNIV_INTENSITY		0x70
#define MMS_UNIV_KEY_INTENSITY		0x71
#define MMS_UNIV_REFERENCE		0x72
#define MMS_UNIV_KEY_REFERENCE		0x73

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
#define MELFAS_DEBUG_PRINT		0
#define FLASH_VERBOSE_DEBUG		0
#else
#define MELFAS_DEBUG_PRINT		1
#define FLASH_VERBOSE_DEBUG		1
#endif

#ifdef SEC_TSP_FACTORY_TEST
#define TEST_CODE 0

#define TSP_CMD_STR_LEN			32
#define TSP_CMD_RESULT_STR_LEN		512
#define TSP_CMD_PARAM_NUM		8
#endif /* SEC_TSP_FACTORY_TEST */


/* FW_UPDATE define START*/

enum {
	FW_UPDATE_BUILT_IN = 0,
	FW_UPDATE_UMS,
};

/* ISC_XFER_LEN - ISC unit transfer length. Give number of 2 power n, where  n is between 2 and 10 */
#define ISC_XFER_LEN		256
#define MMS_FLASH_PAGE_SZ	1024
#define ISC_BLOCK_NUM		(MMS_FLASH_PAGE_SZ / ISC_XFER_LEN)

enum {
	BOOT_SECTION		= 0,
	CORE_SECTION		= 1,
	CONFIG_SECTION		= 2,
	MAX_SECTION_NUM		= 3,
};

enum {
	ISC_ADDR		= 0xD5,
	ISC_CMD_READ_STATUS	= 0xD9,
	ISC_CMD_READ		= 0x4000,
	ISC_CMD_EXIT		= 0x8200,
	ISC_CMD_PAGE_ERASE	= 0xC000,
	ISC_CMD_ERASE		= 0xC100,
	ISC_PAGE_ERASE_DONE	= 0x10000,
	ISC_PAGE_ERASE_ENTER	= 0x20000,
};

struct mms_bin_hdr {
	char	tag[8];
	u16	core_version;
	u16	section_num;
	u16	contains_full_binary;
	u16	reserved0;

	u32	binary_offset;
	u32	binary_length;

	u32	extention_offset;
	u32	reserved1;

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

/* FW_UPDATE define END*/


int touch_is_pressed;
bool mms134s_initialized = 0;

#define TSP_DEVICE_NAME "sec_touchscreen"
#define TSP_DEVICE_NAME2 "mms_ts"

enum {
	None = 0,
	TOUCH_SCREEN,
	TOUCH_KEY
};

struct mms_ts_touch_info {
	int strength;
	int width;
	int posX;
	int posY;
	unsigned char state;
	unsigned short mcount;
};

struct mms_btn_map {
	unsigned char nbuttons;
	u32 map[NUM_OF_TOUCHKEY];
	s16 intensity[NUM_OF_TOUCHKEY];
	bool pressed[NUM_OF_TOUCHKEY];
};

struct mms_ts_dt_data {
	int coords[2];

	int irq_gpio;
	int scl_gpio;
	int sda_gpio;
	int vdd_gpio;

	struct mms_btn_map *btn;
};

struct mms_ts_info {
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct mms_ts_dt_data *dt_data;
	struct mms_ts_touch_info finger[MELFAS_MAX_TOUCH];

	int		irq;
	bool		tsp_enabled;

	struct mutex	lock;
	bool		noise_mode;
	bool		ta_status;
	u8		fw_ic_ver;
	const u8	*config_fw_version;

	int	tx_num;
	int	rx_num;

	u8	fw_ver_boot_bin;
	u8	fw_ver_core_bin;
	u8	fw_ver_config_bin;
	u8	fw_ver_boot_ic;
	u8	fw_ver_core_ic;
	u8	fw_ver_config_ic;

	const char		*fw_path;
	const struct firmware	*fw_img;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif

#ifdef TOUCH_BOOSTER
	struct delayed_work work_dvfs_off;
	struct delayed_work work_dvfs_chg;
	bool	dvfs_lock_status;
	struct mutex dvfs_lock;
#endif

#ifdef SEC_TSP_FACTORY_TEST
	struct list_head	cmd_list_head;
	u8			cmd_state;
	char			cmd[TSP_CMD_STR_LEN];
	int			cmd_param[TSP_CMD_PARAM_NUM];
	char			cmd_result[TSP_CMD_RESULT_STR_LEN];
	struct mutex		cmd_lock;
	bool			cmd_is_running;
	s16 *reference;
	s16 *cm_abs;
	s16 *cm_delta;
	s16 *intensity;

	struct device *tsp_dev;
#endif /* SEC_TSP_FACTORY_TEST */

#ifdef SEC_TKEY_FACTORY_TEST
	struct device *tkey_dev;
#endif
};

static struct mms_ts_info *g_info = NULL;

extern struct class *sec_class;
#define SEC_CLASS_DEVT_TSP	10
#define SEC_CLASS_DEVT_TKEY	11

static int melfas_ts_start(struct mms_ts_info *info);
static int melfas_ts_stop(struct mms_ts_info *info);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_ts_early_suspend(struct early_suspend *h);
static void melfas_ts_late_resume(struct early_suspend *h);
#endif

#ifdef USE_OPEN_CLOSE
static void  melfas_ts_input_close(struct input_dev *dev);
static int  melfas_ts_input_open(struct input_dev *dev);
#endif


#ifdef SEC_TSP_FACTORY_TEST
enum CMD_STATUS {
	CMD_STATUS_WAITING = 0,
	CMD_STATUS_RUNNING,
	CMD_STATUS_OK,
	CMD_STATUS_FAIL,
	CMD_STATUS_NOT_APPLICABLE,
};

#define TSP_CMD(name, func) .cmd_name = name, .cmd_func = func
struct tsp_cmd {
	struct list_head	list;
	const char	*cmd_name;
	void	(*cmd_func)(void *device_data);
};

static void fw_update(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_config_ver(void *device_data);
static void get_threshold(void *device_data);
static void module_off_master(void *device_data);
static void module_on_master(void *device_data);
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

static struct tsp_cmd tsp_cmds[] = {
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
};
#endif

#ifdef TOUCH_BOOSTER
static void change_dvfs_lock(struct work_struct *work)
{
	struct mms_ts_info *info = container_of(work,
				struct mms_ts_info, work_dvfs_chg.work);
	int ret;
	mutex_lock(&info->dvfs_lock);
	ret = set_freq_limit(DVFS_TOUCH_ID, 998400);
	mutex_unlock(&info->dvfs_lock);

	if (ret < 0)
		dev_err(&info->client->dev, "%s: 1booster stop failed(%d)\n",
					__func__, __LINE__);
	else
		dev_info(&info->client->dev, "%s\n", __func__);
}

static void set_dvfs_off(struct work_struct *work)
{
	struct mms_ts_info *info = container_of(work,
				struct mms_ts_info, work_dvfs_off.work);
	mutex_lock(&info->dvfs_lock);
	set_freq_limit(DVFS_TOUCH_ID, -1);
	info->dvfs_lock_status = false;
	mutex_unlock(&info->dvfs_lock);

}

static void set_dvfs_lock(struct mms_ts_info *info, uint32_t on)
{
	int ret = 0;

	mutex_lock(&info->dvfs_lock);
	if (on == 0) {
		if (info->dvfs_lock_status) {
			schedule_delayed_work(&info->work_dvfs_off,
				msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));
		}
	} else if (on == 1) {
		cancel_delayed_work(&info->work_dvfs_off);
		if (!info->dvfs_lock_status) {
			ret = set_freq_limit(DVFS_TOUCH_ID, 998400);
			if (ret < 0)
				dev_err(&info->client->dev, "%s: cpu lock failed(%d)\n",
							__func__, ret);

			info->dvfs_lock_status = true;
		}
	} else if (on == 2) {
		cancel_delayed_work(&info->work_dvfs_off);
		schedule_work(&info->work_dvfs_off.work);
	}
	mutex_unlock(&info->dvfs_lock);
}
#endif

static int melfas_i2c_write(struct i2c_client *client, char *buf, int length);
static void melfas_ts_reboot(void);
static void melfas_ts_reboot_after_update(void);

static void melfas_ts_power_enable(int en)
{
#if defined(CONFIG_MACH_KANAS3G_CTC)
	static struct regulator* ldo22;
	int rc = 0;

	printk(KERN_ERR "%s: (%d)\n", __func__, en);

	if(!ldo22){
		ldo22 = regulator_get(NULL,"vdd_l22");
		rc = regulator_set_voltage(ldo22,3000000,3000000);
		if (rc){
			printk(KERN_ERR "%s: TSP set_level failed (%d)\n",__func__, rc);
		}
	}

	if(en){
		if(regulator_is_enabled(ldo22))
		{
			printk(KERN_ERR "%s: TSP power already enable\n", __func__);
			return;
		}
		rc = regulator_enable(ldo22);
		if(rc)
			printk(KERN_ERR "%s: TSP power enable failed (%d)\n", __func__, rc);
	} else {
		if(!regulator_is_enabled(ldo22))
		{
			printk(KERN_ERR "%s: TSP power already disable\n", __func__);
			return;
		}
		rc = regulator_disable(ldo22);
		if(rc)
			printk(KERN_ERR "%s: TSP power disable failed (%d)\n", __func__, rc);
	}
#else
	dev_info(&g_info->client->dev, "%s: (%d)\n", __func__, en);
	gpio_direction_output(g_info->dt_data->vdd_gpio, en);
#endif
}

#ifdef TSP_PATTERN_TRACTKING
/* To do forced calibration when ghost touch occured at the same point
    for several second. */
#define MAX_GHOSTCHECK_FINGER			10
#define MAX_GHOSTTOUCH_COUNT			300
#define MAX_GHOSTTOUCH_BY_PATTERNTRACKING	5
static int tcount_finger[MAX_GHOSTCHECK_FINGER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int touchbx[MAX_GHOSTCHECK_FINGER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int touchby[MAX_GHOSTCHECK_FINGER] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int ghosttouchcount;
static int cFailbyPattenTracking;

static void clear_tcount(void)
{
	int i;
	for (i = 0; i < MAX_GHOSTCHECK_FINGER; i++) {
		tcount_finger[i] = 0;
		touchbx[i] = 0;
		touchby[i] = 0;
	}
}

static int diff_two_point(int x, int y, int oldx, int oldy)
{
	int diffx, diffy;
	int distance;

	diffx = x-oldx;
	diffy = y-oldy;
	distance = abs(diffx) + abs(diffy);

	if (distance < 3)
		return 1;
	else
		return 0;
}

static int tsp_pattern_tracking(struct mms_ts_info *info, int fingerindex, int x, int y)
{
	int i;
	int ghosttouch = 0;

	if (i == fingerindex) {
		if (diff_two_point (x, y, touchbx[i], touchby[i])) {
			tcount_finger[i] = tcount_finger[i]+1;
		} else {
			tcount_finger[i] = 0;
		}
		touchbx[i] = x;
		touchby[i] = y;

		if (tcount_finger[i] > MAX_GHOSTTOUCH_COUNT) {
			ghosttouch = 1;
			ghosttouchcount++;
			dev_dbg(&info->client->dev, "SUNFLOWER (PATTERN TRACKING) %d\n", ghosttouchcount);
			clear_tcount();
			cFailbyPattenTracking++;

			if (cFailbyPattenTracking > MAX_GHOSTTOUCH_BY_PATTERNTRACKING) {
				cFailbyPattenTracking = 0;
				dev_info(&info->client->dev, "Reboot.\n");
				melfas_ts_reboot();
			} else {
				/* Do something for calibration */
			}
		}
	}

	return ghosttouch;
}
#endif

static int mms_isc_read_status(struct i2c_client *client, u32 val)
{
	u8 cmd = ISC_CMD_READ_STATUS;
	u32 result = 0;
	int cnt = 100;
	int ret = 0;

	do {
		i2c_smbus_read_i2c_block_data(client, cmd, 4, (u8 *)&result);
		if (result == val)
			break;
		msleep(1);
	} while (--cnt);

	if (!cnt) {
		dev_err(&client->dev,
			"status read fail. cnt : %d, val : 0x%x != 0x%x\n",
			cnt, result, val);
	}
	return ret;
}

static int mms_isc_transfer_cmd(struct i2c_client *client, int cmd)
{
	struct isc_packet pkt = { ISC_ADDR, cmd };
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.len = sizeof(struct isc_packet),
		.buf = (u8 *)&pkt,
	};

	return (i2c_transfer(client->adapter, &msg, 1) != 1);
}

static int mms_isc_erase_page(struct i2c_client *client, int page)
{
	return mms_isc_transfer_cmd(client, ISC_CMD_PAGE_ERASE | page) ||
		mms_isc_read_status(client, ISC_PAGE_ERASE_DONE | ISC_PAGE_ERASE_ENTER | page);
}

static int mms_isc_enter(struct i2c_client *client)
{
	dev_info(&client->dev, "%s\n", __func__);
	return i2c_smbus_write_byte_data(client, MMS_CMD_ENTER_ISC, true);
}

static int mms_isc_exit(struct i2c_client *client)
{
	dev_info(&client->dev, "%s\n", __func__);
	return mms_isc_transfer_cmd(client, ISC_CMD_EXIT);
}

static int mms_flash_section(struct i2c_client *client, struct mms_fw_img *img, const u8 *data)
{
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
		mms_isc_erase_page(client, page);

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

	dev_info(&client->dev, "%s: section[%d] update succeeded\n", __func__, img->type);

	ret = 0;
	goto out;

i2c_err:
	dev_err(&client->dev, "%s: i2c failed\n", __func__);
	ret = -1;

out:
	kfree(isc_packet);
	kfree(msg[1].buf);

	return ret;
}

static int get_fw_version(struct i2c_client *client, u8 *buf)
{
	u8 cmd = MMS_FW_VERSION;
	struct i2c_msg msg[2] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = &cmd,
			.len = 1,
		}, {
			.addr = client->addr,
			.flags = I2C_M_RD,
			.buf = buf,
			.len = MAX_SECTION_NUM,
		},
	};
	return (i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg)) != ARRAY_SIZE(msg));
}

static int mms_flash_fw(struct mms_ts_info *info, bool force)
{
	struct i2c_client *client = info->client;
	int ret =0;
	int i;
	struct mms_bin_hdr *fw_hdr;
	struct mms_fw_img **img;
	u8 ver[MAX_SECTION_NUM];
	u8 target[MAX_SECTION_NUM];
	int offset = sizeof(struct mms_bin_hdr);
	int retires = 3;
	bool update_flag = false;
	bool isc_flag = true;

	fw_hdr = (struct mms_bin_hdr *)info->fw_img->data;
	img = kzalloc(sizeof(*img) * fw_hdr->section_num, GFP_KERNEL);

	dev_info(&client->dev, "%s[%d]: FW:(%d,%d,%d,%d) \n", __func__, force,
		(u8)fw_hdr->core_version, (u8)fw_hdr->contains_full_binary,
		(u8)fw_hdr->reserved0, (u8)fw_hdr->section_num);

	while (retires--) {
		if (!get_fw_version(client, ver))
			break;
		else
			melfas_ts_reboot_after_update();
	}

	if (retires < 0) {
		dev_warn(&client->dev, "failed to obtain ver. info\n");
		isc_flag = false;
		memset(ver, 0xff, sizeof(ver));
	} else {
		dev_err(&client->dev, "%s: IC Ver %02x,%02x,%02x \n", __func__, ver[0], ver[1], ver[2]);
	}

	info->fw_ver_boot_ic = ver[BOOT_SECTION];
	info->fw_ver_core_ic = ver[CORE_SECTION];
	info->fw_ver_config_ic = ver[CONFIG_SECTION];

	info->fw_ver_core_bin = fw_hdr->core_version;

	for (i = 0; i < fw_hdr->section_num; i++, offset += sizeof(struct mms_fw_img)) {
		img[i] = (struct mms_fw_img *)(info->fw_img->data + offset);
		target[i] = img[i]->version;

		dev_info(&client->dev, "%s: section[%d] Check IC:0x%02x, BIN:0x%02x\n",
			__func__, img[i]->type, (ver[img[i]->type]), target[i]);

		if (!force) {
			if (i == 0)
				info->fw_ver_boot_bin = target[i];
			else if (i == 1)
				info->fw_ver_config_bin = target[i];
		}

		/* Update condition
		 * - IC ver: 0xff: fw crack, cannot read ver
		 * - IC ver < BIN ver
		 * - force: true */
		if ((ver[img[i]->type] == 0xff) || (ver[img[i]->type] < target[i]) || force) {
			if (isc_flag) {
				mms_isc_enter(client);
				isc_flag = false;
			}
			update_flag = true;

			ret = mms_flash_section(client, img[i], info->fw_img->data + fw_hdr->binary_offset);
			if (ret < 0) {
				dev_err(&client->dev,
					"%s: section[%d] fw update is failed.\n",
					__func__, img[i]->type);
				melfas_ts_reboot_after_update();
				goto out;
			}

		} else {
			dev_info(&client->dev, "%s: section[%d] do not update\n",
				__func__, img[i]->type);
		}
	}

	if (update_flag){
		mms_isc_exit(client);
		msleep(5);
		melfas_ts_reboot_after_update();
	} else {
		ret = 0;
		goto out;
	}

	if (get_fw_version(client, ver)) {
		dev_err(&client->dev, "%s: failed to obtain version after flash\n", __func__);
		ret = -1;
		goto out;
	} else {
		info->fw_ver_boot_ic = ver[BOOT_SECTION];
		info->fw_ver_core_ic = ver[CORE_SECTION];
		info->fw_ver_config_ic = ver[CONFIG_SECTION];

		for (i = 0; i < fw_hdr->section_num; i++) {
			if (ver[img[i]->type] != target[i]) {
				dev_err(&client->dev,
					"%s: version mismatch after flash. [%d] 0x%02x != 0x%02x\n",
					__func__, img[i]->type, ver[img[i]->type], target[i]);
				ret = -1;
				goto out;
			}
		}
	}
	ret = 0;

out:
	dev_info(&info->client->dev, "%s: succeeded. IC[%02x%02x%02x], BIN[%02x%02x%02x]\n", __func__,
		info->fw_ver_boot_ic, info->fw_ver_core_ic, info->fw_ver_config_ic,
		info->fw_ver_boot_bin,info->fw_ver_core_bin, info->fw_ver_config_bin);
	kfree(img);

	return ret;
}

static int melfas_ts_fw_update(struct mms_ts_info *info,
		const struct firmware *fw_data, bool force)
{
	int retires = 3;
	int ret;

	if (!fw_data) {
		dev_err(&info->client->dev, "%s: Firmware data is NULL\n", __func__);
		return -ENODEV;
	}

	info->fw_img = fw_data;
	do {
		ret = mms_flash_fw(info, force);
	} while (ret && --retires);

	if (!retires) {
		dev_err(&info->client->dev,
			"%s: failed to flash firmware after retries\n", __func__);
		ret = -1;
	}
	info->fw_img = NULL;

	return ret;
}

static int melfas_ts_set_fw_name(struct mms_ts_info *info)
{
	u8 ver[MAX_SECTION_NUM];
	int ret;
#if defined(CONFIG_MACH_KANAS3G_CTC)
	int retries = 3;
#endif

	/* Need 150ms to configure old/new firmware version in IC after power-on */
	msleep(150);

#if defined(CONFIG_MACH_KANAS3G_CTC)
	while(retries--) {
		ret = get_fw_version(info->client, ver);
		if(!ret)
			break;
		else if(ret == -5 || ret == -6) {
			dev_err(&info->client->dev, "%s: it is not melfas IC\n",
			__func__);
			mdelay(5);
			continue;
		} else {
			dev_err(&info->client->dev, "%s: Failed to get config_fw_ver from IC\n",
				__func__);
			info->fw_path = FW_IMAGE_NAME_NULL;
			return -1;
		}
	}
	if(retries == 0)
		return ret;
#else
	ret = get_fw_version(info->client, ver);
	if (ret) {
		dev_err(&info->client->dev, "%s: Failed to get config_fw_ver from IC\n",
			__func__);
		info->fw_path = FW_IMAGE_NAME_NULL;
		return -1;
	}
#endif

	if (ver[CONFIG_SECTION] >= 0x50) {
		info->fw_path = FW_IMAGE_NAME_NEW;
	} else if (ver[CONFIG_SECTION] > 0x00) {
		info->fw_path = FW_IMAGE_NAME_OLD;
	} else {
		info->fw_path = FW_IMAGE_NAME_NULL;
	}

	info->fw_ver_boot_ic = ver[BOOT_SECTION];
	info->fw_ver_config_ic = ver[CONFIG_SECTION];
	info->config_fw_version = "N/A";

	dev_info(&info->client->dev, "%s: FW_NAME: %s [%02x%02x]\n", __func__,
		info->fw_path, info->fw_ver_boot_ic, info->fw_ver_config_ic);

	return 0;
}

static int melfas_ts_fw_update_probe(struct mms_ts_info *info)
{
	int ret, i;
	const struct firmware *fw_entry = NULL;

#if defined(CONFIG_MACH_KANAS3G_CTC)
	ret = melfas_ts_set_fw_name(info);
	if(ret == -EIO || ret == -ENXIO) {
		return -ENXIO;
	}
#else
	melfas_ts_set_fw_name(info);
#endif

	if (!info->fw_path) {
		dev_err(&info->client->dev, "%s: Firmware name is not defined\n",
				__func__);
		return -EINVAL;
	}

	dev_info(&info->client->dev, "%s: Load firmware : %s\n", __func__, info->fw_path);

	ret = request_firmware(&fw_entry, info->fw_path, &info->client->dev);
	if (ret) {
		dev_err(&info->client->dev, "%s: Firmware image %s not available\n",
				__func__, info->fw_path);
		ret = -EINVAL;
		goto done;
	}

	for (i = 0; i < DOWNLOAD_RETRY_CNT; i++) {
		ret =  melfas_ts_fw_update(info, fw_entry, false);
		if (ret < 0)
			dev_err(&info->client->dev, "%s: Failed to fw update[%d]\n", __func__, ret);
		else
			break;
	}
done:
	if (fw_entry)
		release_firmware(fw_entry);

	return ret;
}

static int mms_load_fw_from_kernel(struct mms_ts_info *info, const char *fw_path)
{
	int retval;
	const struct firmware *fw_entry = NULL;

	if (!fw_path) {
		dev_err(&info->client->dev, "%s: Firmware name is not defined\n",
				__func__);
		return -EINVAL;
	}

	dev_info(&info->client->dev, "%s: Load firmware : %s\n", __func__, fw_path);

	retval = request_firmware(&fw_entry, fw_path, &info->client->dev);

	if (retval) {
		dev_err(&info->client->dev, "%s: Firmware image %s not available\n",
				__func__, fw_path);
		goto done;
	}

	retval = melfas_ts_fw_update(info, fw_entry, true);
	if (retval < 0)
		dev_err(&info->client->dev, "%s: failed update firmware\n", __func__);
done:
	if (fw_entry)
		release_firmware(fw_entry);

	return retval;
}

static int mms_load_fw_from_ums(struct mms_ts_info *info)
{
	struct file *fp;
	mm_segment_t old_fs;
	int nread;
	int error = 0;
	struct firmware fw;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(FW_IMAGE_NAME_UMS, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		dev_err(&info->client->dev,
				"%s: failed to open %s.\n", __func__, FW_IMAGE_NAME_UMS);
		error = -ENOENT;
		goto open_err;
	}

	fw.size = fp->f_path.dentry->d_inode->i_size;
	if (fw.size > 0) {
		unsigned char *fw_data;
		fw_data = kzalloc(fw.size, GFP_KERNEL);
		nread = vfs_read(fp, (char __user *)fw_data,
				fw.size, &fp->f_pos);

		dev_info(&info->client->dev,
				"%s: start, file path %s, size %u Bytes\n", __func__,
				FW_IMAGE_NAME_UMS, fw.size);

		if (nread != fw.size) {
			dev_err(&info->client->dev,
					"%s: failed to read firmware file, nread %u Bytes\n",
					__func__,
					nread);
			error = -EIO;
		} else {
			/* UMS case */
			fw.data = fw_data;
			error = melfas_ts_fw_update(info, &fw, true);
		}
		if (error < 0)
			dev_err(&info->client->dev, "%s: failed update firmware\n",
					__func__);

		kfree(fw_data);
	}

	filp_close(fp, current->files);

open_err:
	set_fs(old_fs);
	return error;
}

static void mms_set_noise_mode(struct mms_ts_info *info)
{
	int ret;
	u8 setLowLevelData[2];

	int bit1 = 0;
	int bit2 = 0;

	dev_info(&info->client->dev, "%s: Noise mode:%d, TA:%d \n",
		__func__, info->noise_mode, info->ta_status );
	if (!info->noise_mode) {
		bit1 = 0x04;
	}
	if (info->ta_status) {
		bit2 = 0x01;
	} else {
		bit2 = 0x02;
		info->noise_mode = 0;
	}
	// 1xx , noise mode is 0, 1 is not
	// x01 , insert TA
	// x10 , pull out TA

	setLowLevelData[0] = 0x30;
	setLowLevelData[1] = (bit1 | bit2);
	ret = melfas_i2c_write(info->client, setLowLevelData, 2);

	dev_info(&info->client->dev, "%s, Reg:%d, ret:%d\n", __func__, (bit1 | bit2), ret);

}

static void melfas_ts_get_data(struct mms_ts_info *info)
{
	int ret = 0, i, j;
	uint8_t buf[TS_READ_REGS_LEN];
	int read_num, FingerID;
	int _touch_is_pressed, line;
	int keyID = 0, touchType = 0, touchState = 0;
	u8 setLowLevelData[2];

	if (info == NULL) {
		pr_err("%s : TS NULL\n", __func__);
		return;
	}

	if (!info->tsp_enabled) {
		dev_err(&info->client->dev, "[TSP ]%s. tsp_disabled.\n", __func__);
		msleep(500);
		return;
	}

	for (j = 0; j < 3; j++) {
		buf[0] = MMS_MIP_EVENT_PACKET_LENGTH;
		ret = i2c_master_send(info->client, buf, 1);
		if (ret < 0) {
			line = __LINE__;
			goto tsp_error;
		}
		ret = i2c_master_recv(info->client, buf, 1);
		if (ret < 0) {
			line = __LINE__;
			goto tsp_error;
		}
		read_num = buf[0];
		if (read_num < 60)
			break;
	}
	if (read_num > TS_READ_REGS_LEN)
		read_num = TS_READ_REGS_LEN;

	if (read_num > 0) {
		buf[0] = MMS_MIP_EVENT_PACKET;
		ret = i2c_master_send(info->client, buf, 1);
		if (ret < 0) {
			line = __LINE__;
			goto tsp_error;
		}
		ret = i2c_master_recv(info->client, buf, read_num);
		if (ret < 0) {
			line = __LINE__;
			goto tsp_error;
		}

		if (buf[0] == 0x0E) {
			dev_info(&info->client->dev, "%s: noise mode enter!!\n", __func__);
			info->noise_mode = 1 ;
			setLowLevelData[0] = 0x10;
			setLowLevelData[1] = 0x00;
			ret = melfas_i2c_write(info->client, setLowLevelData, 2);
			mms_set_noise_mode(info);
		}

		for (i = 0; i < read_num; i = i + 6) {
			if (buf[i] == 0x0F) {
				dev_err(&info->client->dev, "%s : ESD-DETECTED!!!\n", __func__);
				line = __LINE__;
				goto tsp_error;
			}

			touchType = (buf[i] >> 5) & 0x03;
			if (touchType == TOUCH_SCREEN) {
				FingerID = (buf[i] & 0x0F) - 1;
				info->finger[FingerID].posX = (uint16_t)(buf[i + 1] & 0x0F) << 8 | buf[i + 2];
				info->finger[FingerID].posY = (uint16_t)(buf[i + 1] & 0xF0) << 4 | buf[i + 3];

				if ((buf[i] & 0x80) == 0)
					info->finger[FingerID].strength = 0;
				else
					info->finger[FingerID].strength = buf[i + 4];
				info->finger[FingerID].width = buf[i + 5];

			} else if (touchType == TOUCH_KEY) {
				keyID = (buf[i] & 0x0F) - 1;
				touchState = !!(buf[i] & 0x80);

				info->dt_data->btn->pressed[keyID] = touchState ? true : false;

				input_report_key(info->input_dev, info->dt_data->btn->map[keyID],
						touchState ? PRESS_KEY : RELEASE_KEY);

				dev_err(&info->client->dev, "Button %d[%d] is %s\n",
						keyID, info->dt_data->btn->map[keyID],
						touchState ? "pressed" : "released");

				// When happend tkey and tsp event same time, input fw can't get, so added sync and 1ms delay time.
				input_sync(info->input_dev);
				msleep(1);
			}
		}
	}

	_touch_is_pressed = 0;
	for (i = 0; i < MELFAS_MAX_TOUCH; i++) {
		if (info->finger[i].strength == -1)
			continue;
#ifdef TSP_PATTERN_TRACTKING
		tsp_pattern_tracking(info, i,  info->finger[i].posX,  info->finger[i].posY);
#endif
		if (info->finger[i].strength) {
			input_mt_slot(info->input_dev, i);
			input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, true);
			input_report_abs(info->input_dev, ABS_MT_PRESSURE, info->finger[i].strength);
			input_report_abs(info->input_dev, ABS_MT_POSITION_X, info->finger[i].posX);
			input_report_abs(info->input_dev, ABS_MT_POSITION_Y, info->finger[i].posY);
			input_report_key(info->input_dev, BTN_TOUCH, info->finger[i].strength);

			if (info->finger[i].state == 0){
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				dev_info(&info->client->dev, "P[%d] (%d,%d), z:%d w:%d\n",
						i, info->finger[i].posX, info->finger[i].posY,
						info->finger[i].strength, info->finger[i].width);
#else
				dev_info(&info->client->dev, "P[%d]\n", i);
#endif
			} else
				info->finger[i].mcount++;
			info->finger[i].state = 1;

		} else {
			input_mt_slot(info->input_dev, i);
			input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, false);
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			dev_info(&info->client->dev, "R[%d] (%d,%d), M[%d], Ver[%02x]\n",
					i, info->finger[i].posX, info->finger[i].posY,
					info->finger[i].mcount, info->fw_ver_config_ic);
#else
			dev_info(&info->client->dev, "R[%d] M[%d], Ver[%02x]\n",
					i, info->finger[i].mcount, info->fw_ver_config_ic);
#endif
			info->finger[i].mcount = 0;
			info->finger[i].state = 0;
		}

		if (info->finger[i].strength == 0)
			info->finger[i].strength = -1;
		if (info->finger[i].strength > 0)
			_touch_is_pressed = 1;
	}

	input_sync(info->input_dev);
	touch_is_pressed = _touch_is_pressed;

#ifdef TOUCH_BOOSTER
	set_dvfs_lock(info, !!touch_is_pressed);
#endif

	return;

tsp_error:
	dev_err(&info->client->dev, "[TSP] %s: i2c failed(%d)\n", __func__, line);
	melfas_ts_reboot();
}

static irqreturn_t melfas_ts_irq_handler(int irq, void *handle)
{
	struct mms_ts_info *info = (struct mms_ts_info *)handle;

	melfas_ts_get_data(info);

	return IRQ_HANDLED;
}

#if TEST_CODE
static int melfas_i2c_read(struct i2c_client *client, u16 addr, u16 length, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 1;
	msg[0].buf   = (u8 *) &addr;
	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = length;
	msg[1].buf   = (u8 *) value;

	if  (i2c_transfer(adapter, msg, 2) == 2)
		return 0;
	else
		return -EIO;
}
#endif

static int melfas_i2c_write(struct i2c_client *client, char *buf, int length)
{
	int i;
	char data[TS_WRITE_REGS_LEN];
	if (length > TS_WRITE_REGS_LEN) {
		dev_err(&client->dev, "[TSP] size error - %s\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < length; i++)
		data[i] = *buf++;
	i = i2c_master_send(client, (char *)data, length);
	if (i == length)
		return length;
	else
		return -EIO;
}

static void release_all_fingers(struct mms_ts_info *info)
{
	int i;

	dev_err(&info->client->dev, "%s start.\n", __func__);
	for (i = 0; i < MELFAS_MAX_TOUCH; i++) {

		info->finger[i].state = 0;

		info->finger[i].strength = 0;
		input_mt_slot(info->input_dev, i);
		input_mt_report_slot_state(info->input_dev,
					MT_TOOL_FINGER, false);
		info->finger[i].posX = 0;
		info->finger[i].posY = 0;

		if (info->finger[i].strength == 0)
			info->finger[i].strength = -1;
	}
	input_sync(info->input_dev);
#ifdef TOUCH_BOOSTER
	set_dvfs_lock(info, 2);
	dev_info(&info->client->dev, "[TSP] dvfs_lock free.\n ");
#endif
}

static void melfas_ts_reboot(void)
{
	dev_info(&g_info->client->dev, "%s\n", __func__);
	disable_irq_nosync(g_info->irq);

	g_info->tsp_enabled = false;

	melfas_ts_power_enable(0);
	msleep(60);

	release_all_fingers(g_info);

	msleep(60);
	melfas_ts_power_enable(1);
	msleep(60);

	enable_irq(g_info->irq);
	g_info->tsp_enabled = true;

};

static void melfas_ts_reboot_after_update(void)
{
	dev_info(&g_info->client->dev, "%s\n", __func__);

	msleep(50);
	melfas_ts_power_enable(0);
	msleep(1000);
	melfas_ts_power_enable(1);
	msleep(300);
}

#ifdef SEC_TSP_FACTORY_TEST
static void set_default_result(struct mms_ts_info *info)
{
	char delim = ':';

	memset(info->cmd_result, 0x00, ARRAY_SIZE(info->cmd_result));
	memcpy(info->cmd_result, info->cmd, strlen(info->cmd));
	strncat(info->cmd_result, &delim, 1);
}

static void set_cmd_result(struct mms_ts_info *info, char *buff, int len)
{
	strncat(info->cmd_result, buff, len);
}

static int check_rx_tx_num(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[TSP_CMD_STR_LEN] = {0};
	int node;

	if (info->cmd_param[0] < 0 ||
			info->cmd_param[0] >= info->tx_num ||
			info->cmd_param[1] < 0 ||
			info->cmd_param[1] >= info->rx_num) {
		snprintf(buff, sizeof(buff) , "%s", "NG");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = CMD_STATUS_FAIL;

		dev_info(&info->client->dev, "%s: parameter error: %u,%u\n",
				__func__, info->cmd_param[0],
				info->cmd_param[1]);
		node = -1;
		return node;
	}
	node = info->cmd_param[1] * info->tx_num+ info->cmd_param[0];

	dev_info(&info->client->dev, "%s: node = %d\n", __func__, node);
	return node;
}

static void not_support_cmd(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};

	set_default_result(info);
	snprintf(buff, sizeof(buff), "%s", "NA");
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	mutex_lock(&info->cmd_lock);
	info->cmd_is_running = false;
	mutex_unlock(&info->cmd_lock);

	info->cmd_state = CMD_STATUS_WAITING;
	dev_info(&info->client->dev, "%s: \"%s(%d)\"\n", __func__,
				buff, strnlen(buff, sizeof(buff)));
	return;
}

static void fw_update(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	struct i2c_client *client = info->client;
	int ret, i;
	char buff[16] = {0};

	set_default_result(info);

	for (i = 0; i < DOWNLOAD_RETRY_CNT; i++) {

		switch (info->cmd_param[0]) {
		case FW_UPDATE_BUILT_IN:
			ret = mms_load_fw_from_kernel(info, info->fw_path);
			break;
		case FW_UPDATE_UMS:
			ret = mms_load_fw_from_ums(info);
			break;
		default:
			dev_err(&client->dev,"%s: not support fw_update mode:%d\n", __func__, info->cmd_param[0]);
			snprintf(buff, sizeof(buff), "%s", "NA");
			set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
			info->cmd_state = CMD_STATUS_NOT_APPLICABLE;
			return;
		}

		if (ret != 0) {
			dev_err(&info->client->dev, "%s: failed to fw update[%d]\n", __func__, ret);

			snprintf(buff, sizeof(buff), "%s", "NG");
			set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
			info->cmd_state = CMD_STATUS_FAIL;

		} else {
			snprintf(buff, sizeof(buff), "%s", "OK");
			set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
			info->cmd_state = CMD_STATUS_OK;

			break;
		}
	}
	return;
}

static void get_fw_ver_bin(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};

	set_default_result(info);
	snprintf(buff, sizeof(buff), "ME%02x%02x%02x",
		info->fw_ver_boot_bin, info->fw_ver_core_bin, info->fw_ver_config_bin);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = CMD_STATUS_OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_fw_ver_ic(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};

	set_default_result(info);
	snprintf(buff, sizeof(buff), "ME%02x%02x%02x",
		info->fw_ver_boot_ic, info->fw_ver_core_ic, info->fw_ver_config_ic);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = CMD_STATUS_OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_config_ver(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[20] = {0};

	set_default_result(info);
	snprintf(buff, sizeof(buff), "%s", info->config_fw_version);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = CMD_STATUS_OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_threshold(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};
	int threshold;

	threshold = 25;	//TSP

	set_default_result(info);
	/*
	melfas_i2c_read(info->client, MMS_MIP_CONTACT_ON_EVENT_THRES, 1, &threshold);
	*/
	if (threshold < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = CMD_STATUS_FAIL;
		return;
	}
	snprintf(buff, sizeof(buff), "%d", threshold);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = CMD_STATUS_OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void module_off_master(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[3] = {0};
	int ret;

	set_default_result(info);

	ret = melfas_ts_stop(info);
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		info->cmd_state = CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		info->cmd_state = CMD_STATUS_OK;
	}
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	dev_info(&info->client->dev, "%s: %s\n", __func__, buff);
}

static void module_on_master(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[3] = {0};
	int ret;

	set_default_result(info);

	ret = melfas_ts_start(info);
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		info->cmd_state = CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		info->cmd_state = CMD_STATUS_OK;
	}
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	dev_info(&info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_vendor(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};

	set_default_result(info);
	snprintf(buff, sizeof(buff), "%s", "MELFAS");
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = CMD_STATUS_OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_chip_name(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};

	set_default_result(info);
	snprintf(buff, sizeof(buff), "%s", "MMS134S");
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = CMD_STATUS_OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_reference(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};
	s16 val;
	int node;

	set_default_result(info);

	node = check_rx_tx_num(info);
	if (node < 0)
		return ;

	val = info->reference[node];
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = CMD_STATUS_OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_cm_abs(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};
	s16 val;
	int node;

	set_default_result(info);

	node = check_rx_tx_num(info);
	if (node < 0)
		return;

	val = info->cm_abs[node];
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = CMD_STATUS_OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_cm_delta(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};
	s16 val;
	int node;

	set_default_result(info);

	node = check_rx_tx_num(info);
	if (node < 0)
		return;

	val = info->cm_delta[node];
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = CMD_STATUS_OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_intensity(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};
	s16 val;
	int node;

	set_default_result(info);

	node = check_rx_tx_num(info);
	if (node < 0)
		return;

	val = info->intensity[node];
	snprintf(buff, sizeof(buff), "%d", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = CMD_STATUS_OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_x_num(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};
	int val;

	set_default_result(info);

	val = i2c_smbus_read_byte_data(info->client, 0x0B);
	info->tx_num = val;

	if (val < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = CMD_STATUS_FAIL;
		dev_err(&info->client->dev,
			"%s: fail to read num of x (%d).\n", __func__, val);
		return ;
	}
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = CMD_STATUS_OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_y_num(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	char buff[16] = {0};
	int val;

	set_default_result(info);

	val = i2c_smbus_read_byte_data(info->client, 0x0C);
	info->rx_num = val;

	if (val < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = CMD_STATUS_FAIL;
		dev_err(&info->client->dev,
			"%s: fail to read num of y (%d).\n", __func__, val);
		return ;
	}
	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = CMD_STATUS_OK;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

int get_cm_test_init(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	int ret = 0;
	u8 sz = 0;

	disable_irq(info->irq);

	if (i2c_smbus_write_byte_data(client, MMS_UNIVERSAL_CMD, MMS_UNIV_ENTER_TEST)) {
		dev_err(&client->dev, "%s: i2c failed\n", __func__);
	}

	do {
		udelay(100);
	} while (gpio_get_value(info->dt_data->irq_gpio));

	sz = i2c_smbus_read_byte_data(client, MMS_MIP_EVENT_PACKET_LENGTH);
	sz = i2c_smbus_read_byte_data(client, MMS_MIP_EVENT_PACKET);

	if (sz != 0x0C) {
		dev_err(&client->dev, "%s: maker\n", __func__);
		return -1;
	}
	sz = i2c_smbus_read_byte_data(client, MMS_UNIVERSAL_RESULT_LENGTH);
	sz = i2c_smbus_read_byte_data(client, MMS_UNIVERSAL_RESULT);

	return ret;
}

int get_cm_test_exit(struct mms_ts_info *info)
{

	struct i2c_client *client = info->client;
	int ret = 0;

	if(i2c_smbus_write_byte_data(client, MMS_UNIVERSAL_CMD, MMS_UNIV_EXIT_TEST)){
		dev_err(&client->dev, "%s: i2c failed\n", __func__);
		return -1;
	}

	enable_irq(info->irq);
	return ret;

}

static int test_to_get_raw_data_all(struct mms_ts_info *info, u8 cmd)
{
	int sz, result;

	if (i2c_smbus_write_byte_data(info->client, MMS_UNIVERSAL_CMD, cmd)) {
		dev_err(&info->client->dev, "%s: i2c failed\n", __func__);
	}

	do {
		udelay(100);
	} while (gpio_get_value(info->dt_data->irq_gpio));

	sz = i2c_smbus_read_byte_data(info->client, MMS_UNIVERSAL_RESULT_LENGTH);
	result = i2c_smbus_read_byte_data(info->client, MMS_UNIVERSAL_RESULT);

	dev_info(&info->client->dev, "%s: result: %s\n", __func__, result ? "pass" : "fail");

	return result;
}

static int get_raw_data_all(struct mms_ts_info *info, u8 cmd)
{
	struct i2c_client *client = info->client;
	int r, t, ii;
	int ret = 0;
	u8 sz = 0;
	u8 buf[256] = {0, };
	u8 reg[4] = { 0, };
	s16 cmdata, max_value = 0, min_value = 0;
	char buff[TSP_CMD_STR_LEN] = {0};
	s16 *raw_data;
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

	switch (cmd) {
	case MMS_UNIV_GET_CM_DELTA:
		dev_info(&info->client->dev, "%s: CM_DELTA: 0x%02x\n", __func__, cmd);
		raw_data = info->cm_delta;
		goto test_raw_data;

	case MMS_UNIV_GET_CM_ABS:
		dev_info(&info->client->dev, "%s: CM_ABS: 0x%02x\n", __func__, cmd);
		raw_data = info->cm_abs;
		goto test_raw_data;

	case MMS_UNIV_INTENSITY:
		disable_irq(info->irq);
		dev_info(&info->client->dev, "%s: INTENSITY: 0x%02x\n", __func__, cmd);
		raw_data = info->intensity;
		goto get_raw_data;

	case MMS_UNIV_REFERENCE:
		disable_irq(info->irq);
		dev_info(&info->client->dev, "%s: REFERENCE: 0x%02x\n", __func__, cmd);
		raw_data = info->reference;
		goto get_raw_data;

	default:
		dev_err(&info->client->dev, "%s: unsupported cmd: 0x%02x\n", __func__, cmd);
		ret = -EINVAL;
		goto err;
	}

test_raw_data:
	ret = test_to_get_raw_data_all(info, cmd - 1);
	if (ret != 1) {
		dev_err(&client->dev, "%s: test failed, %d\n", __func__, ret);
		ret = -1;
		goto err;
	}
	msleep(1);

get_raw_data:
	dev_info(&info->client->dev, "\t");
	for (t = 0; t < info->tx_num ; t++) {
		pr_cont("[%2d] ", t);
	}
	pr_cont("\n");
	for (r = 0, ii = 0; r < info->rx_num; r++) {
		dev_info(&info->client->dev, "[%2d]", r);

		reg[0] = MMS_UNIVERSAL_CMD;
		reg[1] = cmd;
		reg[2] = 0xFF;
		reg[3] = r;
		msg[0].len = 4;
		if (i2c_transfer(client->adapter, &msg[0], 1) != 1) {
			dev_err(&client->dev, "%s: i2c transfer failed, cmd\n", __func__);
			ret = -1;
			goto err;
		}

		while (gpio_get_value(info->dt_data->irq_gpio));

		sz = i2c_smbus_read_byte_data(client, MMS_UNIVERSAL_RESULT_LENGTH);

		reg[0] = MMS_UNIVERSAL_RESULT;
		msg[0].len = 1;
		msg[1].len = sz;
		msg[1].buf = buf;
		if (i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg)) != ARRAY_SIZE(msg)) {
			dev_err(&client->dev, "%s: i2c transfer failed, result\n", __func__);
			ret = -1;
			goto err;
		}

		for (t = 0; t < info->tx_num; t++, ii++) {
			cmdata = (s16)(buf[2 * t] | (buf[2 * t + 1] << 8));
			pr_cont("%5d", cmdata);
			raw_data[ii] = cmdata;

			if (r == 0 && t == 0) {
				max_value = min_value = cmdata;
			} else {
				max_value = max(max_value, cmdata);
				min_value = min(min_value, cmdata);
			}
		}
		pr_cont("\n");

	}

	ret = 0;
err:
	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	dev_err(&info->client->dev, "%s: %d,%d\n", __func__, min_value, max_value);

	if (cmd == MMS_UNIV_INTENSITY || cmd == MMS_UNIV_REFERENCE)
		enable_irq(info->irq);
	return ret;
}

static void run_reference_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);

	get_raw_data_all(info, MMS_UNIV_REFERENCE);

	info->cmd_state = CMD_STATUS_OK;
}

static void run_cm_abs_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);

	get_cm_test_init(info);
	get_raw_data_all(info, MMS_UNIV_GET_CM_ABS);
	get_cm_test_exit(info);

	info->cmd_state = CMD_STATUS_OK;
}

static void run_cm_delta_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);

	get_cm_test_init(info);
	get_raw_data_all(info, MMS_UNIV_GET_CM_DELTA);
	get_cm_test_exit(info);

	info->cmd_state = CMD_STATUS_OK;
}

static void run_intensity_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);

	get_raw_data_all(info, MMS_UNIV_INTENSITY);

	info->cmd_state = CMD_STATUS_OK;
}

static ssize_t store_cmd(struct device *dev, struct device_attribute *devattr,
		const char *buf, size_t count)
{
	struct mms_ts_info *info = g_info;
	struct i2c_client *client = info->client;
	char *cur, *start, *end;
	char buff[TSP_CMD_STR_LEN] = {0};
	int len, i;
	struct tsp_cmd *tsp_cmd_ptr = NULL;
	char delim = ',';
	bool cmd_found = false;
	int param_cnt = 0;

	if (strlen(buf) >= TSP_CMD_STR_LEN) {
		dev_err(&info->client->dev, "%s: cmd length is over(%s,%d)!!\n", __func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	if (info->cmd_is_running == true) {
		dev_err(&client->dev, "tsp_cmd: other cmd is running.\n");
		goto err_out;
	}

	/* check lock  */
	mutex_lock(&info->cmd_lock);
	info->cmd_is_running = true;
	mutex_unlock(&info->cmd_lock);
	info->cmd_state = CMD_STATUS_RUNNING;

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
				if (kstrtoint(buff, 10,
					info->cmd_param + param_cnt) < 0)
					goto err_out;
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
	/*for*/
	tsp_cmd_ptr->cmd_func(info);
err_out:

	return count;
}

static ssize_t show_cmd_status(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct mms_ts_info *info = g_info;
	char buff[16] = {0};

	dev_info(&info->client->dev, "tsp cmd: status:%d\n",
			info->cmd_state);

	switch (info->cmd_state) {
	case CMD_STATUS_WAITING:
		snprintf(buff, sizeof(buff), "WAITING");
		break;
	case CMD_STATUS_RUNNING:
		snprintf(buff, sizeof(buff), "RUNNING");
		break;
	case CMD_STATUS_OK:
		snprintf(buff, sizeof(buff), "OK");
		break;
	case CMD_STATUS_FAIL:
		snprintf(buff, sizeof(buff), "FAIL");
		break;
	case CMD_STATUS_NOT_APPLICABLE:
	default:
		snprintf(buff, sizeof(buff), "NOT_APPLICABLE");
		break;
	}

	return snprintf(buf, TSP_BUF_SIZE, "%s\n", buff);
}

static ssize_t show_cmd_result(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct mms_ts_info *info = g_info;

	dev_info(&info->client->dev, "tsp cmd: result: %s\n", info->cmd_result);

	mutex_lock(&info->cmd_lock);
	info->cmd_is_running = false;
	mutex_unlock(&info->cmd_lock);
	info->cmd_state = CMD_STATUS_WAITING;

	return snprintf(buf, TSP_BUF_SIZE, "%s\n", info->cmd_result);
}

static ssize_t show_cmd_list(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct mms_ts_info *info = g_info;
	char buffer[TSP_CMD_RESULT_STR_LEN];
	char buffer_name[TSP_CMD_STR_LEN];
	struct tsp_cmd *tsp_cmd_ptr = NULL;

	snprintf(buffer, 30, "++ factory command list ++\n");
	list_for_each_entry(tsp_cmd_ptr, &info->cmd_list_head, list) {
		if (strncmp(tsp_cmd_ptr->cmd_name, "not_support_cmd", 16)) {
			snprintf(buffer_name, TSP_CMD_STR_LEN, "%s\n", tsp_cmd_ptr->cmd_name);
			strncat(buffer, buffer_name, strlen(buffer_name));
		}
	}

	dev_info(&info->client->dev,
			"%s: length : %u / %d\n", __func__,
			strlen(buffer), TSP_CMD_RESULT_STR_LEN);

	return snprintf(buf, PAGE_SIZE, "%s\n", buffer);
}

static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, store_cmd);
static DEVICE_ATTR(cmd_status, S_IRUGO, show_cmd_status, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, show_cmd_result, NULL);
static DEVICE_ATTR(cmd_list, S_IRUGO, show_cmd_list, NULL);

static struct attribute *sec_touch_facotry_attributes[] = {
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
	&dev_attr_cmd_list.attr,
	NULL,
};

static struct attribute_group sec_touch_factory_attr_group = {
	.attrs = sec_touch_facotry_attributes,
};

static int factory_init_tsp(struct mms_ts_info *info)
{
	int ret, i;

	INIT_LIST_HEAD(&info->cmd_list_head);
	for (i = 0; i < ARRAY_SIZE(tsp_cmds); i++)
		list_add_tail(&tsp_cmds[i].list, &info->cmd_list_head);

	mutex_init(&info->cmd_lock);

	info->cmd_is_running = false;
	info->cmd_state = CMD_STATUS_WAITING;
	info->noise_mode = 0;

	info->tsp_dev = device_create(sec_class,
			NULL, SEC_CLASS_DEVT_TSP, info, "tsp");
	if (IS_ERR(info->tsp_dev)) {
		dev_err(&info->client->dev, "Failed to create device(tsp)\n");
		ret = -ENODEV;
		goto err_create_device;
	}

	ret = sysfs_create_group(&info->tsp_dev->kobj,
			       &sec_touch_factory_attr_group);
	if (ret) {
		dev_err(&info->client->dev, "Failed to create sysfs group\n");
		ret = (ret > 0) ? -ret : ret;
		goto err_create_group;
	}

	ret = sysfs_create_link(&info->tsp_dev->kobj, &info->input_dev->dev.kobj, "input");
	if (ret < 0)
		dev_err(&info->client->dev, "%s: Failed to create input symbolic link[%d]\n",
				__func__, ret);

	info->tx_num = i2c_smbus_read_byte_data(info->client, 0x0B);
	info->rx_num = i2c_smbus_read_byte_data(info->client, 0x0C);

	info->reference = kzalloc(2 * info->rx_num * info->tx_num, GFP_KERNEL);
	if (!info->reference) {
		dev_err(&info->client->dev,
				"%s: Failed to alloc mem for reference\n",
				__func__);
		ret = -ENOMEM;
		goto err_mem_reference;
	}

	info->cm_abs = kzalloc(2 * info->rx_num * info->tx_num, GFP_KERNEL);
	if (!info->cm_abs) {
		dev_err(&info->client->dev,
				"%s: Failed to alloc mem for cm_abs\n",
				__func__);
		ret = -ENOMEM;
		goto err_mem_cm_abs;
	}

	info->cm_delta = kzalloc(2 * info->rx_num * info->tx_num, GFP_KERNEL);
	if (!info->cm_delta) {
		dev_err(&info->client->dev,
				"%s: Failed to alloc mem for cm_delta\n",
				__func__);
		ret = -ENOMEM;
		goto err_mem_cm_delta;
	}

	info->intensity = kzalloc(2 * info->rx_num * info->tx_num, GFP_KERNEL);
	if (!info->intensity) {
		dev_err(&info->client->dev,
				"%s: Failed to alloc mem for intensity\n",
				__func__);
		ret = -ENOMEM;
		goto err_mem_intensity;
	}

	return ret;

err_mem_intensity:
	kfree(info->cm_delta);
err_mem_cm_delta:
	kfree(info->cm_abs);
err_mem_cm_abs:
	kfree(info->reference);
err_mem_reference:
	sysfs_remove_group(&info->tsp_dev->kobj,
			       &sec_touch_factory_attr_group);
err_create_group:
	device_destroy(sec_class, SEC_CLASS_DEVT_TSP);
err_create_device:
	info->tsp_dev = NULL;

	return ret;
}
#endif /* SEC_TSP_FACTORY_TEST */

#ifdef SEC_TKEY_FACTORY_TEST
static int get_raw_data_one(struct mms_ts_info *info, u8 cmd)
{
	struct i2c_client *client = info->client;
	int t;
	int ret = 0;
	u8 sz = 0;
	u8 buf[256] = {0, };
	u8 reg[4] = { 0, };
	s16 cmdata;
	u8 key_num;
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

	disable_irq(info->irq);
	key_num = i2c_smbus_read_byte_data(client, 0x0D);

	if (key_num) {
		reg[0] = MMS_UNIVERSAL_CMD;
		reg[1] = cmd;
		reg[2] = 0xFF;
		reg[3] = 0x00;
		msg[0].len = 4;

		if (i2c_transfer(client->adapter, &msg[0], 1) != 1) {
			dev_err(&client->dev, "%s: i2c transfer failed, cmd\n", __func__);
			ret = -1;
			goto out;
		}

		while (gpio_get_value(info->dt_data->irq_gpio));

		sz = i2c_smbus_read_byte_data(client, MMS_UNIVERSAL_RESULT_LENGTH);

		reg[0] = MMS_UNIVERSAL_RESULT;
		msg[0].len = 1;
		msg[1].len = sz;
		msg[1].buf = buf;
		if (i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg)) != ARRAY_SIZE(msg)) {
			dev_err(&client->dev, "%s: i2c transfer failed, result\n", __func__);
			ret = -1;
			goto out;
		}
		dev_info(&client->dev, "%s: ", __func__);
		for (t = 0; t < key_num; t++) {
			cmdata = (s16)(buf[2 * t] | (buf[2 * t + 1] << 8));
			info->dt_data->btn->intensity[t] = cmdata;
			pr_cont("%-5d", cmdata);
		}
		pr_cont("\n");
	}

out:
	enable_irq(info->irq);

	return ret;
}

static ssize_t touchkey_threshold_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int threshold;

	threshold = 22;	//TSK

	return snprintf(buf, sizeof(buf), "%d\n", threshold);
}

static ssize_t touchkey_back_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u16 intensity;

	get_raw_data_one(g_info, MMS_UNIV_KEY_INTENSITY);
	intensity = (g_info->dt_data->btn->intensity[1] < 0) ?
				0 : g_info->dt_data->btn->intensity[1];

	return snprintf(buf, 10, "%d\n", intensity);
}

static ssize_t touchkey_recent_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u16 intensity;

	get_raw_data_one(g_info, MMS_UNIV_KEY_INTENSITY);
	intensity = (g_info->dt_data->btn->intensity[0] < 0) ?
				0 : g_info->dt_data->btn->intensity[0];

	return snprintf(buf, 10, "%d\n", intensity);
}

static DEVICE_ATTR(touchkey_back, S_IRUGO | S_IWUSR | S_IWGRP,
			touchkey_back_show, NULL);
static DEVICE_ATTR(touchkey_recent, S_IRUGO | S_IWUSR | S_IWGRP,
			touchkey_recent_show, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO | S_IWUSR | S_IWGRP,
			touchkey_threshold_show, NULL);

static struct attribute *touchkey_attributes[] = {
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_recent.attr,
	NULL,
};

static struct attribute_group touchkey_attr_group = {
	.attrs = touchkey_attributes,
};

static int factory_init_tk(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	int ret;

	info->tkey_dev= device_create(sec_class, NULL, SEC_CLASS_DEVT_TKEY,
					info, "sec_touchkey");
	if (IS_ERR(info->tkey_dev)) {
		dev_err(&client->dev, "Failed to create fac touchkey dev\n");
		ret = -ENODEV;
		goto err_create_device_tk;
	}
	ret = sysfs_create_group(&info->tkey_dev->kobj, &touchkey_attr_group);
	if (ret) {
		dev_err(&client->dev,
			"Failed to create sysfs (touchkey_attr_group).\n");
		ret = (ret > 0) ? -ret : ret;
		goto err_create_group_tk;
	}

	return 0;

err_create_group_tk:
	device_destroy(sec_class, SEC_CLASS_DEVT_TKEY);
err_create_device_tk:
	info->tkey_dev = NULL;

	return ret;
}
#endif /* SEC_TKEY_FACTORY_TEST */

#ifdef CONFIG_OF
static int melfas_ts_parse_dt(struct device *dev,
			struct mms_ts_dt_data *dt_data)
{
	struct device_node *np = dev->of_node;
	struct property *prop;
	int rc;
	int i;
	u32 coords[2];
	struct mms_btn_map *btn;

	/* vdd, irq gpio info */
	dt_data->irq_gpio = of_get_named_gpio(np, "melfas,irq-gpio", 0);
	dt_data->scl_gpio = of_get_named_gpio(np, "melfas,scl-gpio", 0);
	dt_data->sda_gpio = of_get_named_gpio(np, "melfas,sda-gpio", 0);
	dt_data->vdd_gpio = of_get_named_gpio(np, "melfas,vdd-gpio", 0);

	rc = of_property_read_u32_array(np, "melfas,tsp-coords", coords, 2);
	if (rc < 0) {
		dev_info(dev, "%s: Unable to read synaptics,tsp-coords\n", __func__);
		return rc;
	}

	dt_data->coords[0] = coords[0];
	dt_data->coords[1] = coords[1];

	prop = of_find_property(np, "melfas,tkey-keycodes", NULL);
	if (prop && prop->value) {
		btn = kzalloc(sizeof(*btn), GFP_KERNEL);
		if (!btn) {
			dev_err(dev, "Failed to allocate f1a memory\n");
			return -ENOMEM;
		}

		btn->nbuttons = prop->length / sizeof(u32);

		rc = of_property_read_u32_array(np, "melfas,tkey-keycodes",
			btn->map, btn->nbuttons);
		if (rc && (rc != -EINVAL)) {
			dev_info(dev, "%s: Unable to read %s, free button map memory\n", __func__,
					"melfas,tkey-keycodes");
			kfree(btn);
			return rc;
		}

		dt_data->btn = btn;

		pr_err("%s tkey enabled! ", __func__);
		for (i = 0; i < dt_data->btn->nbuttons; i++)
			pr_cont("keycode[%d] = %d ", i, dt_data->btn->map[i]);
		pr_cont("\n");
	}

	pr_err("%s: tsp_int= %d, vdd= %d, x= %d, y= %d\n",
		__func__, dt_data->irq_gpio, dt_data->vdd_gpio, dt_data->coords[0], dt_data->coords[1]);

	return 0;
}
#else
static int melfas_ts_parse_dt(struct device *dev,
			struct mms_ts_dt_data *dt_data)
{
	return -ENODEV;
}
#endif

static void melfas_ts_request_gpio(struct mms_ts_dt_data *dt_data)
{
	int error;

	if (dt_data->irq_gpio > 0) {
		error = gpio_request(dt_data->irq_gpio, "tsp_irq_gpio");
		if (error) {
			pr_err("[TSP] %s: unable to request irq-gpio[%d]\n", __func__, dt_data->irq_gpio);
		}
	}

	if (dt_data->vdd_gpio > 0) {
		error = gpio_request(dt_data->vdd_gpio, "tsp_en_gpio");
		if (error) {
			pr_err("[TSP] %s: unable to request vdd-gpio[%d]\n", __func__, dt_data->vdd_gpio);
		}
	}
}

#if defined(CONFIG_GET_LCD_ATTACHED)
extern int get_lcd_attached(void);
#endif

#if defined(CONFIG_TOUCHSCREEN_IST30XX) && defined(CONFIG_MACH_KANAS3G_CTC)
extern bool ist30xx_initialized;
#endif
static int melfas_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct mms_ts_info *info;
	struct mms_ts_dt_data *dt_data;
	int ret = 0, i;

	dev_err(&client->dev, "%s\n", __func__);

#if defined(CONFIG_GET_LCD_ATTACHED)
	if (get_lcd_attached() == 0) {
		dev_err(&client->dev, "%s : get_lcd_attached()=0 \n", __func__);
		return -EIO;
	}
#endif

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "%s: need I2C_FUNC_I2C\n", __func__);
		return -ENODEV;
	}

	if (client->dev.of_node) {
		dt_data = devm_kzalloc(&client->dev,
				sizeof(struct mms_ts_dt_data), GFP_KERNEL);
		if (!dt_data) {
			dev_err(&client->dev, "%s: Failed to allocate memory\n", __func__);
			return -ENOMEM;
		}
		ret = melfas_ts_parse_dt(&client->dev, dt_data);
		if (ret < 0) {
			devm_kfree(&client->dev, (void *)dt_data);
			return ret;
		}
	} else	{
		dt_data = client->dev.platform_data;
		dev_err(&client->dev, "TSP failed to align dtsi %s", __func__);
	}

	if (!dt_data) {
		dev_err(&client->dev,
			"%s: device tree data is not found\n", __func__);
		return -EINVAL;
	}
	melfas_ts_request_gpio(dt_data);

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_err(&client->dev,
			"%s: Failed to alloc mem for melfas-ts\n", __func__);
		return -ENOMEM;
		goto err_alloc_data_failed;
	}
	g_info = info;
	info->tsp_enabled = false;
	info->irq = client->irq = gpio_to_irq(dt_data->irq_gpio);
	info->dt_data = dt_data;
	mutex_init(&info->lock);

	info->client = client;
	i2c_set_clientdata(client, info);

	melfas_ts_power_enable(0);
	msleep(60);
	melfas_ts_power_enable(1);
	msleep(60);

	ret = melfas_ts_fw_update_probe(info);
	if (ret < 0) {
		dev_err(&info->client->dev,
			"%s: Failed to Firmware update\n", __func__);
		goto err_fw_update_failed;
	};

	info->input_dev = input_allocate_device();
	if (!info->input_dev) {
		dev_err(&info->client->dev,
			"%s: Failed to alloc mem for input_dev\n", __func__);
		ret = -ENOMEM;
		goto err_input_dev_alloc_failed;
	}
	info->input_dev->name = TSP_DEVICE_NAME;
	info->input_dev->evbit[0] = BIT_MASK(EV_ABS) | BIT_MASK(EV_KEY);
	info->input_dev->dev.parent = &client->dev;
#ifdef USE_OPEN_CLOSE
	info->input_dev->open = melfas_ts_input_open;
	info->input_dev->close = melfas_ts_input_close;
#endif

	set_bit(INPUT_PROP_DIRECT, info->input_dev->propbit);	//JB touch mode setting

	for (i = 0; i < dt_data->btn->nbuttons; i++)
		info->input_dev->keybit[BIT_WORD(dt_data->btn->map[i])] |= BIT_MASK(dt_data->btn->map[i]);

	input_mt_init_slots(info->input_dev, MELFAS_MAX_TOUCH);
	input_set_abs_params(info->input_dev, ABS_MT_POSITION_X,
						0, info->dt_data->coords[0], 0, 0);
	input_set_abs_params(info->input_dev, ABS_MT_POSITION_Y,
						0, info->dt_data->coords[1], 0, 0);
	input_set_abs_params(info->input_dev, ABS_MT_TOUCH_MAJOR,
						0, TS_MAX_Z_TOUCH, 0, 0);
	input_set_abs_params(info->input_dev, ABS_MT_TRACKING_ID,
						0, MELFAS_MAX_TOUCH - 1, 0, 0);
	input_set_abs_params(info->input_dev, ABS_MT_WIDTH_MAJOR,
						0, TS_MAX_W_TOUCH, 0, 0);

	//__set_bit(EV_LED, info->input_dev->evbit);
	//__set_bit(LED_MISC, info->input_dev->ledbit);
	set_bit(EV_SYN, info->input_dev->evbit);
	set_bit(EV_KEY, info->input_dev->evbit);
	set_bit(EV_LED, info->input_dev->evbit);
	set_bit(LED_MISC, info->input_dev->ledbit);
	set_bit(EV_ABS, info->input_dev->evbit);
	set_bit(INPUT_PROP_DIRECT, info->input_dev->propbit);

	input_set_drvdata(info->input_dev, info);

	ret = input_register_device(info->input_dev);
	if (ret) {
		dev_err(&info->client->dev, "%s: Failed to register input device\n", __func__);
		ret = -ENODEV;
		goto err_input_register_device_failed;
	}

#ifdef TOUCH_BOOSTER
	mutex_init(&info->dvfs_lock);
	INIT_DELAYED_WORK(&info->work_dvfs_off, set_dvfs_off);
	INIT_DELAYED_WORK(&info->work_dvfs_chg, change_dvfs_lock);
	info->dvfs_lock_status = false;
#endif

	if (info->client->irq) {
#if MELFAS_DEBUG_PRINT
		dev_info(&info->client->dev, "%s: trying to request irq: %s-%d\n", __func__,
			info->client->name, info->client->irq);
#endif
		ret = request_threaded_irq(client->irq, NULL, melfas_ts_irq_handler,
						IRQF_TRIGGER_LOW | IRQF_ONESHOT,
						info->client->name, info);
		if (ret > 0) {
			dev_err(&info->client->dev, "%s: failed to request irq %d, ret %d\n",
				__func__, info->client->irq, ret);
			ret = -EBUSY;
			goto err_request_irq;
		}
	}

	for (i = 0; i < MELFAS_MAX_TOUCH ; i++)
		info->finger[i].strength = -1;

#ifdef CONFIG_HAS_EARLYSUSPEND
	dev_info(&info->client->dev, "%s: register earlysuspend.\n", __func__);
	info->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	info->early_suspend.suspend = melfas_ts_early_suspend;
	info->early_suspend.resume = melfas_ts_late_resume;
	register_early_suspend(&info->early_suspend);
#endif

#ifdef SEC_TSP_FACTORY_TEST
	ret = factory_init_tsp(info);
	if (ret < 0) {
		dev_err(&info->client->dev, "%s: Failed to factory_init_tsp, %d\n",
				__func__, ret);
		goto err_init_factory_tsp;
	}
#endif

#ifdef SEC_TKEY_FACTORY_TEST
	ret = factory_init_tk(info);
	if (ret < 0) {
		dev_err(&info->client->dev, "%s: Failed to factory_init_tk, %d\n",
				__func__, ret);
		goto err_init_factory_tk;
	}
#endif

	info->tsp_enabled = true;
	mms134s_initialized = 1;
#if MELFAS_DEBUG_PRINT
	dev_err(&info->client->dev, "%s: Start touchscreen. name: %s, irq: %d\n",
		__func__, info->client->name, info->client->irq);
#endif
	return 0;

#ifdef SEC_TKEY_FACTORY_TEST
	sysfs_remove_group(&info->tkey_dev->kobj, &touchkey_attr_group);
	device_destroy(sec_class, SEC_CLASS_DEVT_TKEY);
err_init_factory_tk:
#endif
#ifdef SEC_TSP_FACTORY_TEST
	kfree(info->intensity);
	kfree(info->cm_delta);
	kfree(info->cm_abs);
	kfree(info->reference);
	sysfs_remove_group(&info->tsp_dev->kobj,
			       &sec_touch_factory_attr_group);
	device_destroy(sec_class, SEC_CLASS_DEVT_TSP);
err_init_factory_tsp:
#endif
err_request_irq:
	free_irq(client->irq, info);
	input_unregister_device(info->input_dev);
err_input_register_device_failed:
	input_free_device(info->input_dev);
err_input_dev_alloc_failed:
err_fw_update_failed:
#if defined(CONFIG_TOUCHSCREEN_IST30XX) && defined(CONFIG_MACH_KANAS3G_CTC)
	if(!ist30xx_initialized)
#endif
		melfas_ts_power_enable(0);
	kfree(info);
err_alloc_data_failed:
	devm_kfree(&client->dev, (void *)dt_data);

	return ret;
}

static int melfas_ts_remove(struct i2c_client *client)
{
	struct mms_ts_info *info = i2c_get_clientdata(client);

	unregister_early_suspend(&info->early_suspend);
	free_irq(client->irq, info);
	melfas_ts_power_enable(0);
	input_unregister_device(info->input_dev);
	kfree(info);

	return 0;
}

static int melfas_ts_start(struct mms_ts_info *info)
{
	int ret = 0;

	mutex_lock(&info->lock);

	if (info->tsp_enabled) {
		dev_err(&info->client->dev, "%s: already powered on\n", __func__);
		ret = -1;
		goto out;
	}

	melfas_ts_power_enable(1);

	msleep(50);
	mms_set_noise_mode(info);
	info->tsp_enabled = true;
	enable_irq(info->irq);

out:
	mutex_unlock(&info->lock);
	return ret;
}

static int melfas_ts_stop(struct mms_ts_info *info)
{
	int ret;
	u8 setLowLevelData[2];

	mutex_lock(&info->lock);

	if (!info->tsp_enabled) {
		dev_err(&info->client->dev, "%s: already powered off\n", __func__);
		ret = -1;
		goto out;
	}

	info->tsp_enabled = false;
	disable_irq(info->irq);
	release_all_fingers(info);
	touch_is_pressed = 0;

	setLowLevelData[0] = 0xB0;
	setLowLevelData[1] = 0x01;
	ret = melfas_i2c_write(info->client, setLowLevelData, 2);

	msleep(100);
	melfas_ts_power_enable(0);

out:
	mutex_unlock(&info->lock);
	return ret;
}

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE)
static int melfas_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct mms_ts_info *info = i2c_get_clientdata(client);

	return melfas_ts_stop(info);
}

static int melfas_ts_resume(struct i2c_client *client)
{
	struct mms_ts_info *info = i2c_get_clientdata(client);

	return melfas_ts_start(info);
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_ts_early_suspend(struct early_suspend *h)
{
	struct mms_ts_info *info = container_of(h, struct mms_ts_info, early_suspend);
	melfas_ts_stop(info);
}

static void melfas_ts_late_resume(struct early_suspend *h)
{
	struct mms_ts_info *info = container_of(h, struct mms_ts_info, early_suspend);
	melfas_ts_start(info);
}
#endif

#ifdef USE_OPEN_CLOSE
static void  melfas_ts_input_close(struct input_dev *dev)
{
	struct mms_ts_info *info = input_get_drvdata(dev);

	dev_err(&info->client->dev, "%s\n",__func__);
	melfas_ts_stop(info);

}

static int  melfas_ts_input_open(struct input_dev *dev)
{
	struct mms_ts_info *info = input_get_drvdata(dev);

	dev_err(&info->client->dev, "%s\n",__func__);
	melfas_ts_start(info);

	return 0;
}
#endif


static const struct i2c_device_id melfas_ts_id[] = {
	{ TSP_DEVICE_NAME2, 0 },
	{ }
};

#ifdef CONFIG_OF
static struct of_device_id mms_match_table[] = {
	{ .compatible = "melfas,mms-ts",},
	{ },
};
#else
#define mms_match_table NULL
#endif

static struct i2c_driver melfas_ts_driver = {
	.driver = {
		.name = TSP_DEVICE_NAME2,
		.owner = THIS_MODULE,
		.of_match_table = mms_match_table,
	},
	.id_table = melfas_ts_id,
	.probe	= melfas_ts_probe,
	.remove	= __devexit_p(melfas_ts_remove),
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE)
	.suspend	= melfas_ts_suspend,
	.resume		= melfas_ts_resume,
#endif
};

#ifdef CONFIG_SAMSUNG_LPM_MODE
extern bool poweroff_charging;
#endif
static int __devinit melfas_ts_init(void)
{
#ifdef CONFIG_SAMSUNG_LPM_MODE
	if (poweroff_charging){
		pr_notice("%s : LPM Charging Mode!!\n", __func__);
		return 0;
	}
#endif

	return i2c_add_driver(&melfas_ts_driver);
}

static void __exit melfas_ts_exit(void)
{
	i2c_del_driver(&melfas_ts_driver);
}

module_init(melfas_ts_init);
module_exit(melfas_ts_exit);

MODULE_DESCRIPTION("Driver for Melfas MTSI Touchscreen Controller");
MODULE_AUTHOR("MinSang, Kim <kimms@melfas.com>");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");

