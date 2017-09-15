
#define DEBUG
#define SEC_TSP_DEBUG

#define SEC_TSP_FACTORY_TEST
#define TSP_BUF_SIZE 1024

#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/platform_data/mms_ts.h>
#include <linux/completion.h>
#include <linux/init.h>
#include <asm/uaccess.h>

#include <linux/mutex.h>
#include <mach/gpio.h>
#include <linux/cpufreq.h>
#include <asm/unaligned.h>

#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>

#include "mms100_cfg_update.h"

/* Flag to enable touch key */
#define MMS_HAS_TOUCH_KEY		1

#if MMS_HAS_TOUCH_KEY
#define SEC_TOUCHKEY_INFO
#endif

#define ESD_DETECT_COUNT		10

#define MAX_SECTION_NUM			3
#define MAX_FINGER_NUM			5
#define MAX_WIDTH			30
#define MAX_PRESSURE			255
#define MAX_LOG_LENGTH			128

#define FINGER_EVENT_SZ			6
#define MAX_FINGER_SIZE		MAX_FINGER_NUM * (FINGER_EVENT_SZ+1)

/* Registers */
#define MMS_MODE_CONTROL		0x01
#define MMS_TX_NUM			0x0B
#define MMS_RX_NUM			0x0C
#define MMS_KEY_NUM			0x0D

#define MMS_EVENT_PKT_SZ		0x0F
#define MMS_INPUT_EVENT			0x10

#define MMS_UNIVERSAL_CMD		0xA0
#define MMS_UNIVERSAL_RESULT_LENGTH	0xAE
#define MMS_UNIVERSAL_RESULT		0xAF
//#define MMS_FW_VERSION			0xE1

/* Universal commands */
#define MMS_CMD_SET_LOG_MODE		0x20

/* Event types */
#define MMS_LOG_EVENT			0xD
#define MMS_NOTIFY_EVENT		0xE
#define MMS_ERROR_EVENT			0xF
#define MMS_TOUCH_KEY_EVENT		0x40

#define MMS_COORDS_ARR_SIZE	4

#define MMS_I2C_VTG_MIN_UV	1800000
#define MMS_I2C_VTG_MAX_UV	1800000
#define MMS_I2C_LOAD_UA	10000

struct device *sec_touchscreen;
int touch_is_pressed;

#define MMS_CORE_VERSION	0xE1
#define MMS_TSP_REVISION	0xF0
#define MMS_HW_REVISION		0xF1
#define MMS_COMPAT_GROUP	0xF2
#define MMS_FW_VERSION		0xE1 //0xF4
#define MMS_MODULE_REVISION 0xC2

#ifdef SEC_TSP_FACTORY_TEST
#define TX_NUM		21 //25
#define RX_NUM		14 //18
#define NODE_NUM	294 //450	/* 25x18 */

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
#endif /* SEC_TSP_FACTORY_TEST */

#define FLIP_COVER_TEST 0

#define FW_VERSION 0x18 /*Config Version*/
#define BOOT_VERSION 0x06
#define CORE_VERSION 0x58
#define MODU_VERSION 0x02
#define MAX_FW_PATH 255

/* Touch booster */
#ifdef CONFIG_SEC_DVFS
#define TOUCH_BOOSTER				1
#define TOUCH_BOOSTER_OFF_TIME		100
#define TOUCH_BOOSTER_CHG_TIME		200
#else
#define TOUCH_BOOSTER			0
#endif 

/* touchkey info */
#ifdef SEC_TOUCHKEY_INFO
#define RMI_ADDR_UNIV_CMD						0xA0
#define UNIVCMD_GET_INTENSITY_KEY				0x71
#define RMI_ADDR_UNIV_CMD_RESULT_LENGTH			0xAE
#define RMI_ADDR_UNIV_CMD_RESULT				0xAF
#define RMI_ADDR_KEY_NUM						0x0D

#define WRITEARRAY_LEN		10
#define READARRAY_LEN       10

struct device *sec_touchkey;
u8 master_write_buf[WRITEARRAY_LEN];
u8 master_read_buf_array[READARRAY_LEN];
#endif

enum{
	SYS_TXNUM = 3,
	SYS_RXNUM,
	SYS_CLEAR,
	SYS_ENABLE,
	SYS_DISABLE,
	SYS_INTERRUPT,
	SYS_RESET,
};

enum {
	GET_RX_NUM = 1,
	GET_TX_NUM,
	GET_EVENT_DATA,
};

enum {
	LOG_TYPE_U08 = 2,
	LOG_TYPE_S08,
	LOG_TYPE_U16,
	LOG_TYPE_S16,
	LOG_TYPE_U32 = 8,
	LOG_TYPE_S32,
};

enum {
	SEC_NONE = -1,
	SEC_BOOTLOADER = 0,
	SEC_CORE,
	SEC_CONFIG,
	SEC_MODULE,
	SEC_LIMIT
};

struct mms_log_data {
	__u8				*data;
	int				cmd;
};

struct tsp_callbacks {
	void (*inform_charger)(struct tsp_callbacks *tsp_cb, bool mode);
};

struct mms_ts_info {
	struct i2c_client 		*client;
	struct input_dev 		*input_dev;
	char 				phys[32];

	int 				max_x;
	int 				max_y;

	u8				tx_num;
	u8				rx_num;
	u8				key_num;

	int 				irq;
	int				data_cmd;
	struct regulator *vcc_i2c;	
	struct mms_ts_platform_data 	*pdata;

	char 				*fw_name;
	//struct completion 		init_done;
	struct early_suspend		early_suspend;
	
#if TOUCH_BOOSTER
		struct delayed_work work_dvfs_off;
		struct delayed_work work_dvfs_chg;
		bool dvfs_lock_status;
		//int cpufreq_level;
		struct mutex dvfs_lock;
#endif

	struct mutex 			lock;
	bool				enabled;

	struct class			*class;
	dev_t				mms_dev;
	struct cdev			cdev;

	struct mms_log_data		*log;

#if defined(SEC_TSP_DEBUG)
	unsigned char finger_state[MAX_FINGER_NUM];
#endif

#if 1
	void (*register_cb)(void *);
	struct tsp_callbacks callbacks;
	bool ta_status;
	bool noise_mode;
	//bool sleep_wakeup_ta_check;
	u8 fw_ic_ver;
	//u8 panel;
	u8 fw_core_ver;
	u8 fw_module_ver;
#endif

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

};

static int esd_cnt;

#define USE_OPEN_CLOSE

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mms_ts_early_suspend(struct early_suspend *h);
static void mms_ts_late_resume(struct early_suspend *h);
#endif


static void mms_reboot(struct mms_ts_info *info);
static void mms_report_input_data(struct mms_ts_info *info, u8 sz, u8 *buf);


extern struct class *sec_class;
//EXPORT_SYMBOL(sec_class);

#if defined(SEC_TSP_FACTORY_TEST)
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
/*static void module_off_slave(void *device_data);
static void module_on_slave(void *device_data);*/
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
static int get_data(struct mms_ts_info *info, u8 addr, u8 size, u8 *array);


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
#if FLIP_COVER_TEST
	{TSP_CMD("flip_cover_enable", flip_cover_enable),},
#endif	
	{TSP_CMD("not_support_cmd", not_support_cmd),},
};
#endif

#if FLIP_COVER_TEST
static int set_conifg_flip_cover(struct mms_ts_info *info,int enables);
static int note_flip_open(struct mms_ts_info *info);//void);
static int note_flip_close(struct mms_ts_info *info);//(void);
#endif

#if TOUCH_BOOSTER
static void change_dvfs_lock(struct work_struct *work)
{
	struct mms_ts_info *info = container_of(work,
				struct mms_ts_info, work_dvfs_chg.work);
	int ret;

	mutex_lock(&info->dvfs_lock);
	ret = set_freq_limit(DVFS_TOUCH_ID, MIN_TOUCH_LIMIT_SECOND);
	mutex_unlock(&info->dvfs_lock);

	if (ret < 0)
		pr_err("%s: 1booster stop failed(%d)\n", __func__, __LINE__);
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
			cancel_delayed_work(&info->work_dvfs_chg);
			schedule_delayed_work(&info->work_dvfs_off,
				msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));
		}
	} else if (on == 1) {
		cancel_delayed_work(&info->work_dvfs_off);
		if (!info->dvfs_lock_status) {
			ret = set_freq_limit(DVFS_TOUCH_ID, MIN_TOUCH_LIMIT);
			if (ret < 0 )
				pr_err("%s: cpu lock failed(%d)\n",	__func__, ret);

			schedule_delayed_work(&info->work_dvfs_chg,	msecs_to_jiffies(TOUCH_BOOSTER_CHG_TIME));
			info->dvfs_lock_status = true;
		}
	} else if (on == 2) {
		cancel_delayed_work(&info->work_dvfs_off);
		cancel_delayed_work(&info->work_dvfs_chg);
		schedule_work(&info->work_dvfs_off.work);
	}
	mutex_unlock(&info->dvfs_lock);
}
#endif	// TOUCH_BOOSTER

static void mms_set_noise_mode(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;

	if (!(info->noise_mode && info->enabled))
		return;
	dev_info(&client->dev, "%s\n", __func__);
	i2c_smbus_write_byte_data(info->client, 0x30, 0x1);

/*
	if (info->ta_status) {
		dev_info(&client->dev, "noise_mode & TA connect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x30, 0x1);
	} else {
		dev_info(&client->dev, "noise_mode & TA disconnect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x30, 0x2);
		info->noise_mode = 0;
	}
*/
}

struct tsp_callbacks *melfas_charger_callbacks;
void tsp_charger_infom(bool en)
{
	pr_err("[TSP]%s: ta:%d\n",	__func__, en);

	if (melfas_charger_callbacks && melfas_charger_callbacks->inform_charger)
		melfas_charger_callbacks->inform_charger(melfas_charger_callbacks, en);	
}

static void melfas_tsp_register_callback(void *cb)
{
	melfas_charger_callbacks = cb;
}

static void melfas_ta_cb(struct tsp_callbacks *cb, bool ta_status)
{
	struct mms_ts_info *info =
			container_of(cb, struct mms_ts_info, callbacks);
	struct i2c_client *client = info->client;

	dev_info(&client->dev, "%s\n", __func__);


	if(info->ta_status == ta_status){
		dev_info(&client->dev, "%s ignored same value:%d\n", __func__, ta_status);
	}else{
		info->ta_status = ta_status;
	
		if (info->enabled) {
			if (info->ta_status) {
				dev_info(&client->dev, "TA connect!!!\n");
				i2c_smbus_write_byte_data(info->client, 0x33, 0x1);
			} else {
				dev_info(&client->dev, "TA disconnect!!!\n");
				i2c_smbus_write_byte_data(info->client, 0x33, 0x2);
			}
			//mms_set_noise_mode(info);
		}
	}

}

/*
 * mms_clear_input_data - all finger point release
 */
static void mms_clear_input_data(struct mms_ts_info *info)
{
	int i;

	for (i = 0; i< MAX_FINGER_NUM; i++) {
		input_mt_slot(info->input_dev, i);
		input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, false);
#if defined(SEC_TSP_DEBUG)
		if (info->finger_state[i] == 1){
			info->finger_state[i]=0;
			dev_info(&info->client->dev,"[R] fing[%d] off (force)\n", i);
		}
#endif
	}
	touch_is_pressed = 0;
	input_sync(info->input_dev);

#if TOUCH_BOOSTER
	set_dvfs_lock(info, 2);
	pr_info("[TSP] dvfs_lock free.\n ");
#endif

	return;
}

/*
 * mms_fs_open, mms_fs_release, mms_event_handler, mms_fs_read, mms_fs_write
 * melfas debugging function
 */
static int mms_fs_open(struct inode *node, struct file *fp)
{
	struct mms_ts_info *info;
	struct i2c_client *client;
	struct i2c_msg msg;
	u8 buf[3] = { 
		MMS_UNIVERSAL_CMD, 
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

	info->log->data = kzalloc(MAX_LOG_LENGTH * 20 + 5, GFP_KERNEL);

	mms_clear_input_data(info);

	return 0;
}

static int mms_fs_release(struct inode *node, struct file *fp)
{
	struct mms_ts_info *info = fp->private_data;

	mms_clear_input_data(info);
	mms_reboot(info);

	mms_set_noise_mode(info);

	kfree(info->log->data);
	enable_irq(info->irq);

	return 0;
}

static void mms_event_handler(struct mms_ts_info *info)
{
	struct i2c_client *client = info->client;
	int sz, i;
	int ret;
	int row_num;
	u8 reg = MMS_INPUT_EVENT;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = &reg,
			.len = 1,
		},
		{
			.addr = client->addr,
			.flags = 1,
			.buf = info->log->data,
		},
	};
	struct mms_log_pkt {
		u8 marker;
		u8 log_info;
		u8 code;
		u8 element_sz;
		u8 row_sz;
	} __attribute__((packed)) *pkt = (struct mms_log_pkt *)info->log->data;;

	memset(pkt, 0, sizeof(struct mms_log_pkt));

	if (gpio_get_value(info->pdata->gpio_resetb)) {
		return;
	}

	sz = i2c_smbus_read_byte_data(client, MMS_EVENT_PKT_SZ);
	if (sz < 0) {
		dev_err(&client->dev, "%s bytes=%d\n", __func__, sz);
		for (i = 0; i < 3; i++) {
			sz = i2c_smbus_read_byte_data(client,
						MMS_EVENT_PKT_SZ);
			if (sz > 0)
				break;
		}

		if (i == 3) {
			dev_dbg(&client->dev, "i2c failed... reset!!\n");
			mms_clear_input_data(info);
			mms_reboot(info);
			mms_set_noise_mode(info);
			return;
		}
	}

	if (sz == 0)
		return;

	if (sz > MAX_FINGER_SIZE) {
		dev_err(&client->dev, "[TSP] abnormal data inputed.\n");
		return;
	}
	msg[1].len = sz;
	ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (ret != ARRAY_SIZE(msg)) {
		dev_err(&client->dev,
			"%s : failed to read %d bytes of touch data (%d)\n",
			__func__, sz, ret);
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
			dev_err(&client->dev, "invalid log type\n");
			break;
		}

		msg[1].buf = info->log->data + sizeof(struct mms_log_pkt);
		reg = MMS_UNIVERSAL_RESULT;
		row_num = pkt->row_sz ? pkt->row_sz : 1;

		while (row_num--) {
			while (gpio_get_value(info->pdata->gpio_resetb))
				;

			ret = i2c_transfer(client->adapter, msg, 2);
			msg[1].buf += msg[1].len;
		}; 

	} else {
		mms_report_input_data(info, sz, info->log->data);
		memset(pkt, 0, sizeof(struct mms_log_pkt));
	}


	return;

}

static ssize_t mms_fs_read(struct file *fp, char *rbuf, size_t cnt, loff_t *fpos)
{
	struct mms_ts_info *info = fp->private_data;
	struct i2c_client *client = info->client;
	int ret = 0;

	switch (info->log->cmd) {
	case GET_RX_NUM:
		ret = copy_to_user(rbuf, &info->rx_num, 1);
		break;

	case GET_TX_NUM:
		ret = copy_to_user(rbuf, &info->tx_num, 1);
		break;

	case GET_EVENT_DATA:
		mms_event_handler(info);
		/* send event without log marker */
		ret = copy_to_user(rbuf, info->log->data + 1, cnt);
		break;

	default:
		dev_err(&client->dev, "unknown command\n");
		ret = -EFAULT;
		goto out;
	}

out:
	return ret; 
}

static ssize_t mms_fs_write(struct file *fp, const char *wbuf, size_t cnt, loff_t *fpos)
{
	struct mms_ts_info *info = fp->private_data;
	struct i2c_client *client = info->client;
	struct i2c_msg msg;
	int ret = 0;
	u8 *buf;

	mutex_lock(&info->lock);

	buf = kzalloc(cnt + 1, GFP_KERNEL); 

	if ((buf == NULL) || copy_from_user(buf, wbuf, cnt)) {
		dev_err(&client->dev, "failed to read data from user\n");
		ret = -EIO;
		goto out;
	}

	if (cnt == 1) {
		info->log->cmd = *buf;
	} else {
		msg.addr = client->addr;
		msg.flags = 0;
		msg.buf = buf;
		msg.len = cnt;

		if (i2c_transfer(client->adapter, &msg, 1) != 1) {
			dev_err(&client->dev, "failt to transfer command\n");
			ret = -EIO;
			goto out;
		}
	}

	ret = 0;

out:
	kfree(buf);
	mutex_unlock(&info->lock);

	return ret;
}

static struct file_operations mms_fops = {
	.owner 				= THIS_MODULE,
	.open 				= mms_fs_open,
	.release 			= mms_fs_release,
	.read 				= mms_fs_read,
	.write 				= mms_fs_write,
};

static int reg_set_optimum_mode_check(struct regulator *reg, int load_uA)
{
	return (regulator_count_voltages(reg) > 0) ?
		regulator_set_optimum_mode(reg, load_uA) : 0;
}

void  melfas_vdd_on(struct mms_ts_info *info, bool onoff)
{
	int ret = 0, rc = 0;
	pr_info("[TSP] power %s\n", onoff ? "on" : "off");

	if (!info->vcc_i2c) {
		if (info->pdata->i2c_pull_up) {
			info->vcc_i2c = regulator_get(&info->client->dev,
				"vcc_i2c");
			if (IS_ERR(info->vcc_i2c)) {
				rc = PTR_ERR(info->vcc_i2c);
				dev_err(&info->client->dev,
					"Regulator get failed rc=%d\n", rc);
				goto error_get_vtg_i2c;
			}
			if (regulator_count_voltages(info->vcc_i2c) > 0) {
				rc = regulator_set_voltage(info->vcc_i2c,
				MMS_I2C_VTG_MIN_UV, MMS_I2C_VTG_MAX_UV);
				if (rc) {
					dev_err(&info->client->dev,
					"regulator set_vtg failed rc=%d\n",
					rc);
					goto error_set_vtg_i2c;
				}
			}
		}
	}

	if (onoff) {
		if (info->pdata->i2c_pull_up) {
			rc = reg_set_optimum_mode_check(info->vcc_i2c,
						MMS_I2C_LOAD_UA);
			if (rc < 0) {
				dev_err(&info->client->dev,
				"Regulator vcc_i2c set_opt failed rc=%d\n",
				rc);
				goto error_reg_opt_i2c;
			}

			rc = regulator_enable(info->vcc_i2c);
			if (rc) {
				dev_err(&info->client->dev,
				"Regulator vcc_i2c enable failed rc=%d\n",
				rc);
				goto error_reg_en_vcc_i2c;
			}
		}
	} else {
		if (info->pdata->i2c_pull_up) {
			reg_set_optimum_mode_check(info->vcc_i2c, 0);
			regulator_disable(info->vcc_i2c);
		}
	}
	//msleep(50);

	ret = gpio_direction_output(info->pdata->vdd_en, onoff);
	if (ret) {
		pr_err("[TSP]%s: unable to set_direction for mms_vdd_en [%d]\n",
				__func__, info->pdata->vdd_en);
	}
	//msleep(30);
	return;

error_reg_en_vcc_i2c:
	if (info->pdata->i2c_pull_up)
		reg_set_optimum_mode_check(info->vcc_i2c, 0);
error_reg_opt_i2c:
error_set_vtg_i2c:
	regulator_put(info->vcc_i2c);
	if(info->vcc_i2c)
		info->vcc_i2c = NULL;
error_get_vtg_i2c:
	return;

}

int melfas_power(struct i2c_client *client,bool onoff)
{
	struct mms_ts_info *info = i2c_get_clientdata(client);	
	melfas_vdd_on(info,onoff);
	return 0;
}

EXPORT_SYMBOL(melfas_power);

/* mms_reboot - IC reset */ 
static void mms_reboot(struct mms_ts_info *info)
{
	struct i2c_adapter *adapter = to_i2c_adapter(info->client->dev.parent);

	i2c_lock_adapter(adapter);
	melfas_vdd_on(info, 0);
	msleep(50);
	melfas_vdd_on(info, 1);
	msleep(50);
	i2c_unlock_adapter(adapter);
}

/* mms_report_input_data - The position of a touch send to platfrom  */
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
		if(buf[1]==1){
			info->noise_mode = 1;	// noise
		}else if(buf[1]==2){
			info->noise_mode = 0;	// normal
		}
		dev_info(&client->dev, "TSP mode changed (%d) noise(%d)\n", buf[1], info->noise_mode);
		goto out;
	} else if (buf[0] == MMS_ERROR_EVENT) {
		dev_info(&client->dev, "Error detected, restarting TSP\n");
		mms_clear_input_data(info);
		mms_reboot(info);
		mms_set_noise_mode(info);
		esd_cnt++;
		if (esd_cnt>= ESD_DETECT_COUNT)
		{
			i2c_smbus_write_byte_data(info->client, MMS_MODE_CONTROL, 0x04);
			esd_cnt =0;
		}
		goto out;
	}

	touch_is_pressed = 0;

	for (i = 0; i < sz; i += FINGER_EVENT_SZ) {
		tmp = buf + i;
		esd_cnt =0;
		if (tmp[0] & MMS_TOUCH_KEY_EVENT) {
			switch (tmp[0] & 0xf) {
			case 1:
				key_code = KEY_RECENT;
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
			printk("[KEY] key_code=%d key_state=%d\n",key_code, key_state);

		} else {
			id = (tmp[0] & 0xf) -1;
			x = tmp[2] | ((tmp[1] & 0xf) << 8);
			y = tmp[3] | (((tmp[1] >> 4 ) & 0xf) << 8);
			touch_major = tmp[4];
			pressure = tmp[5];

			/* coordinate */
			if (x < 0)
				x = 0;
			if (y < 0)
				y = 0;
			if(x >= info->max_x)
				x = info->max_x -1;
			if(y >= info->max_y)
				y = info->max_y -1;
			
			if (id >= MAX_FINGER_NUM || id < 0) {
				dev_notice(&client->dev, \
					"finger id error [%d]\n", id);
				goto out;
			}

			input_mt_slot(info->input_dev, id);
			
			if (!(tmp[0] & 0x80)) {
#if defined(SEC_TSP_DEBUG)
				info->finger_state[id] = 0;
				dev_info(&client->dev,"[R] fing[%d]: (%d, %d) off\n", id, x, y);
#endif
				input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, false);
				continue;
			}

#if defined(SEC_TSP_DEBUG)
			if(!info->finger_state[id]){
				dev_info(&client->dev,"[P] fing[%d]: (%d, %d)\n", id, x, y);
				info->finger_state[id] = 1;
			}
#endif
			input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, true);
			input_report_abs(info->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(info->input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(info->input_dev, ABS_MT_TOUCH_MAJOR, touch_major);
			input_report_abs(info->input_dev, ABS_MT_PRESSURE, pressure);

			touch_is_pressed++;
		}
	}

	input_sync(info->input_dev);
	
#if TOUCH_BOOSTER
	set_dvfs_lock(info, !!touch_is_pressed);
#endif
 out:
	return;
}

/* mms_ts_interrupt - interrupt thread */
static irqreturn_t mms_ts_interrupt(int irq, void *dev_id)
{
	struct mms_ts_info *info = dev_id;
	struct i2c_client *client = info->client;
	u8 buf[MAX_FINGER_SIZE] = { 0 };
	int ret, i;
	int sz;
	u8 reg = MMS_INPUT_EVENT;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.buf = &reg,
			.len = 1,
		},
		{
			.addr = client->addr,
			.flags = 1,
			.buf = buf,
		},
	};

	sz = i2c_smbus_read_byte_data(client, MMS_EVENT_PKT_SZ);
	if (sz < 0) {
		dev_err(&client->dev, "%s bytes=%d\n", __func__, sz);
		for (i = 0; i < 3; i++) {
			sz = i2c_smbus_read_byte_data(client,
						MMS_EVENT_PKT_SZ);
			if (sz > 0)
				break;
		}

		if (i == 3) {
			dev_dbg(&client->dev, "i2c failed... reset!!\n");
			mms_clear_input_data(info);
			mms_reboot(info);
			mms_set_noise_mode(info);
			goto out;
		}
	}

	if (sz == 0)
		goto out;

	if (sz > MAX_FINGER_SIZE) {
		dev_err(&client->dev, "[TSP] abnormal data inputed.\n");
		goto out;
	}

	msg[1].len = sz;
	ret = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	if (ret != ARRAY_SIZE(msg)) {
		dev_err(&client->dev,
			"%s : failed to read %d bytes of touch data (%d)\n",
			__func__, sz, ret);
	}

	mms_report_input_data(info, sz, buf);
out:
	return IRQ_HANDLED;
}

/* mms_ts_enable - wake-up func (VDD on)  */
static int mms_ts_enable(struct mms_ts_info *info)
{
//        mutex_lock(&info->lock);
	if (info->enabled)
		goto out;
	gpio_direction_output(info->pdata->vdd_en, 1);
	msleep(50);
	info->enabled = true;

	if (info->ta_status) {
		dev_info(&info->client->dev,"TA connect!!!, noise:%d\n", info->noise_mode);
		i2c_smbus_write_byte_data(info->client, 0x33, 0x1);
		mms_set_noise_mode(info);
	} else {
		dev_info(&info->client->dev,"TA disconnect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x33, 0x2);
	}
	//mms_set_noise_mode(info);

	enable_irq(info->irq);
out:
//	mutex_unlock(&info->lock);
	return 0;
}

/* mms_ts_disable - sleep func (VDD off) */
static int mms_ts_disable(struct mms_ts_info *info)
{
//        mutex_lock(&info->lock);

	if (!info->enabled)
		goto out;
	disable_irq(info->irq);
	gpio_direction_output(info->pdata->vdd_en, 0);
	info->enabled = false;
out:
//	mutex_unlock(&info->lock);
	return 0;
}
/*
 * mms_ts_input_open - Register input device after call this function 
 * this function is wait firmware flash wait
 */ 
static int mms_ts_input_open(struct input_dev *dev)
{
	struct mms_ts_info *info = input_get_drvdata(dev);
	printk("%s %d\n",__func__,__LINE__);
	mms_ts_enable(info);
	return 0;
}

/*
 * mms_ts_input_close -If device power off state call this function
 */
static void mms_ts_input_close(struct input_dev *dev)
{
	struct mms_ts_info *info = input_get_drvdata(dev);
	printk("%s %d\n",__func__,__LINE__);	
	mms_ts_disable(info);
	mms_clear_input_data(info);
	return;
}

/*
 * get_fw_version - get firmware vertion
 */

static unsigned char get_fw_version(struct mms_ts_info *info, u8 area)
{
	struct i2c_client *client = info->client;
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg;
	u8 reg = MMS_CORE_VERSION;
	u8 reg2 = MMS_MODULE_REVISION;
	int ret;
	unsigned char buf[4];

	msg.addr = client->addr;
	msg.flags = 0x00;
	msg.len = 1;
	if(area == SEC_MODULE)
		msg.buf = &reg2;
	else 
	msg.buf = &reg;

	ret = i2c_transfer(adapter, &msg, 1);

	if (ret >= 0) {
		msg.addr = client->addr;
		msg.flags = I2C_M_RD;
		
		if(area == SEC_MODULE)
			msg.len = 2;
		else
		msg.len = 4;
		msg.buf = buf;

		ret = i2c_transfer(adapter, &msg, 1);
	}
	if (ret < 0) {
		pr_err("[TSP] : read error : [%d]", ret);
		return ret;
	}

	if ((area == SEC_BOOTLOADER)||(area == SEC_MODULE))
		return buf[0];
	else if (area == SEC_CORE)
		return buf[1];
	else if (area == SEC_CONFIG)
		return buf[2];
	else
		return 0;
}

static int get_fw_version2(struct i2c_client *client, u8 *buf)
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

/*
 * mms_ts_config - f/w check download & irq thread register
 */
static int mms_ts_config(struct mms_ts_info *info, int fw_location)
{
	struct i2c_client *client = info->client;
	int ret;
	int module_v;
	bool flag = false;
	int retries = 3;
	u8 ver[MAX_SECTION_NUM];

	while (retries--){
		if (!get_fw_version2(client, ver)){
			print_hex_dump(KERN_INFO, "mms_ts fw ver : ", DUMP_PREFIX_NONE, 16, 1,
					ver, MAX_SECTION_NUM, false);
			if(!(ver[0] == BOOT_VERSION && ver[1] == CORE_VERSION) //Victor TSP 06 58 xx
				&& ver[0] != 0xff && ver[1] != 0xff){ 
				printk("%s TSP firmware update skip !!boot=%x, core=%x(%d)\n"
					,__func__,ver[0],ver[1], __LINE__);
			ret = ISC_NO_NEED_UPDATE_ERROR;
				break;
			}
			module_v = get_fw_version(info, SEC_MODULE);
			if(module_v != MODU_VERSION){
				printk("%s TSP module is not applicable(%x), update skip !!(%d)\n",__func__,module_v, __LINE__);
				ret = ISC_NO_NEED_UPDATE_ERROR;
				break;
			}
			if((ver[2] >= FW_VERSION) && (ver[2] < FW_VERSION + 0x10) && !fw_location){
				printk("%s TSP version(%x) is latest, update skip !!(%d)\n",__func__,ver[2], __LINE__);
				ret = ISC_NO_NEED_UPDATE_ERROR;
				break;
			}
			if(fw_location)
				flag = true;
			ret = mms100_ISC_download_mbinary(client, flag, fw_location);
			if(ISC_NO_NEED_UPDATE_ERROR || ISC_SUCCESS)
				break;
		}else{
			mms_reboot(info);
		}
	} 

	if (retries < 0) {
		dev_err(&client->dev, "failed to update firmware\n");
		ret = ISC_NONE;
	}
	dev_info(&client->dev,
		"Melfas MMS-series touch controller initialized\n");

	return ret;
}

/*
 * bin_report_read, bin_report_write, bin_sysfs_read, bin_sysfs_write
 * melfas debugging function
 */
static ssize_t bin_report_read(struct file *fp, struct kobject *kobj, struct bin_attribute *attr,
                                char *buf, loff_t off, size_t count)
{
        struct device *dev = container_of(kobj, struct device, kobj);
        struct i2c_client *client = to_i2c_client(dev);
        struct mms_ts_info *info = i2c_get_clientdata(client);
	count = 0;
	switch(info->data_cmd){
	case SYS_TXNUM:
		dev_info(&info->client->dev, "tx send %d \n",info->tx_num);
		buf[0]=info->tx_num;
		count =1;
		info->data_cmd = 0;
		break;
	case SYS_RXNUM:
		dev_info(&info->client->dev, "rx send%d\n", info->rx_num);
		buf[0]=info->rx_num;
		count =1;
		info->data_cmd = 0;
		break;
	case SYS_CLEAR:
		dev_info(&info->client->dev, "Input clear\n");
		mms_clear_input_data(info);
		count = 1;
		info->data_cmd = 0;
		break;
	case SYS_ENABLE:
		dev_info(&info->client->dev, "enable_irq  \n");
		enable_irq(info->irq);
		count = 1;
		info->data_cmd = 0;
		break;
	case SYS_DISABLE:
		dev_info(&info->client->dev, "disable_irq  \n");
		disable_irq(info->irq);
		count = 1;
		info->data_cmd = 0;
		break;
	case SYS_INTERRUPT:
		count = gpio_get_value(info->pdata->gpio_resetb);
		info->data_cmd = 0;
		break;
	case SYS_RESET:
		mms_reboot(info);
		dev_info(&info->client->dev, "read mms_reboot\n");
		count = 1;
		info->data_cmd = 0;
		break;
	}
	return count;
}

static ssize_t bin_report_write(struct file *fp, struct kobject *kobj, struct bin_attribute *attr,
                                char *buf, loff_t off, size_t count)
{
        struct device *dev = container_of(kobj, struct device, kobj);
        struct i2c_client *client = to_i2c_client(dev);
        struct mms_ts_info *info = i2c_get_clientdata(client);
	if(buf[0]==100){
		mms_report_input_data(info, buf[1], &buf[2]);
	}else{
		info->data_cmd=(int)buf[0];
	}
	return count;
        
}
static struct bin_attribute bin_attr_data = {
        .attr = {
                .name = "report_data",
                .mode = S_IWUSR | S_IWGRP | S_IRUGO, // S_IRWXUGO,
        },
        .size = PAGE_SIZE,
        .read = bin_report_read,
        .write = bin_report_write,
};

static ssize_t bin_sysfs_read(struct file *fp, struct kobject *kobj , struct bin_attribute *attr,
                          char *buf, loff_t off,size_t count)
{
        struct device *dev = container_of(kobj, struct device, kobj);
        struct i2c_client *client = to_i2c_client(dev);
        struct mms_ts_info *info = i2c_get_clientdata(client);
        struct i2c_msg msg;
        info->client = client;

        msg.addr = client->addr;
        msg.flags = I2C_M_RD ;
        msg.buf = (u8 *)buf;
        msg.len = count;

	switch (count)
	{
		case 65535:
			mms_reboot(info);
			dev_info(&client->dev, "read mms_reboot\n");
			return 0;

		default :
			if(i2c_transfer(client->adapter, &msg, 1) != 1){
	                	dev_err(&client->dev, "failed to transfer data\n");
        	        	mms_reboot(info);
        	        	return 0;
        		}
			break;

	}

        return count;
}

static ssize_t bin_sysfs_write(struct file *fp, struct kobject *kobj, struct bin_attribute *attr,
                                char *buf, loff_t off, size_t count)
{
        struct device *dev = container_of(kobj, struct device, kobj);
        struct i2c_client *client = to_i2c_client(dev);
        struct mms_ts_info *info = i2c_get_clientdata(client);
        struct i2c_msg msg;

        msg.addr =client->addr;
        msg.flags = 0;
        msg.buf = (u8 *)buf;
        msg.len = count;


        if(i2c_transfer(client->adapter, &msg, 1) != 1){
                dev_err(&client->dev, "failed to transfer data\n");
                mms_reboot(info);
                return 0;
        }

        return count;
}

static struct bin_attribute bin_attr = {
        .attr = {
                .name = "mms_bin",
                .mode = S_IWUSR | S_IWGRP | S_IRUGO, // S_IRWXUGO,
        },
        .size = PAGE_SIZE,
        .read = bin_sysfs_read,
        .write = bin_sysfs_write,
};

#ifdef SEC_TOUCHKEY_INFO
static ssize_t menu_sensitivity_show(struct device *dev, struct device_attribute *attr, char *buf)
{

   	struct i2c_client *client = to_i2c_client(dev);
	struct mms_ts_info *info = i2c_get_clientdata(client);

	s16 local_menu_sensitivity = 0;

	int ret;

	master_write_buf[0] = RMI_ADDR_UNIV_CMD;
	master_write_buf[1] = UNIVCMD_GET_INTENSITY_KEY;
	master_write_buf[2] = 0xFF; //Exciting CH.
	master_write_buf[3] = 0; //Sensing CH.


	ret = i2c_smbus_write_i2c_block_data(info->client,master_write_buf[0], 4, &master_write_buf[1]);
	if(ret < 0)
	{
		printk( "menu_sensitivity_show 1 error\n");
		goto ERROR_HANDLE;
	}

	master_write_buf[0] = RMI_ADDR_UNIV_CMD_RESULT_LENGTH;
	
	ret = i2c_smbus_write_i2c_block_data(info->client,master_write_buf[0], 4, &master_write_buf[1]);
	
	if(ret < 0)
	{
		printk( "menu_sensitivity_show 2 error\n");
		goto ERROR_HANDLE;
	}

	ret = i2c_smbus_read_i2c_block_data(info->client,master_write_buf[0], 1, &master_read_buf_array[0]);

	if(ret < 0)
	{
		printk( "menu_sensitivity_show 3 error\n");
		goto ERROR_HANDLE;
	}

	master_write_buf[0] = RMI_ADDR_UNIV_CMD_RESULT;
	ret = i2c_smbus_write_i2c_block_data(info->client,master_write_buf[0], 4, &master_write_buf[1]);

	if(ret < 0)
	{
		printk( "menu_sensitivity_show 4 error\n");
		goto ERROR_HANDLE;
	}

	ret = i2c_smbus_read_i2c_block_data(info->client,master_write_buf[0], 4, &master_read_buf_array[0]);

	if(ret < 0)
	{
		printk( "menu_sensitivity_show 5 error\n");
		goto ERROR_HANDLE;
	}

	local_menu_sensitivity = master_read_buf_array[0] | (master_read_buf_array[1] << 8);


	printk( "%5d\n", local_menu_sensitivity);

	return sprintf(buf, "%d\n",  local_menu_sensitivity);


ERROR_HANDLE:
	printk( "menu sensitivity read Error!!\n");
	return 0;

}

static ssize_t back_sensitivity_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
   	struct i2c_client *client = to_i2c_client(dev);
	struct mms_ts_info *info = i2c_get_clientdata(client);

	s16 local_back_sensitivity = 0;

	int ret;

	master_write_buf[0] = RMI_ADDR_UNIV_CMD;
	master_write_buf[1] = UNIVCMD_GET_INTENSITY_KEY;
	master_write_buf[2] = 0xFF; //Exciting CH.
	master_write_buf[3] = 0; //Sensing CH.


	ret = i2c_smbus_write_i2c_block_data(info->client,master_write_buf[0], 4, &master_write_buf[1]);
	if(ret < 0)
	{
		printk( "menu_sensitivity_show 1 error\n");
		goto ERROR_HANDLE;
	}

	master_write_buf[0] = RMI_ADDR_UNIV_CMD_RESULT_LENGTH;
	
	ret = i2c_smbus_write_i2c_block_data(info->client,master_write_buf[0], 4, &master_write_buf[1]);
	
	if(ret < 0)
	{
		printk( "menu_sensitivity_show 2 error\n");
		goto ERROR_HANDLE;
	}

	ret = i2c_smbus_read_i2c_block_data(info->client,master_write_buf[0], 1, &master_read_buf_array[0]);

	if(ret < 0)
	{
		printk( "menu_sensitivity_show 3 error\n");
		goto ERROR_HANDLE;
	}

	master_write_buf[0] = RMI_ADDR_UNIV_CMD_RESULT;
	ret = i2c_smbus_write_i2c_block_data(info->client,master_write_buf[0], 4, &master_write_buf[1]);

	if(ret < 0)
	{
		printk( "menu_sensitivity_show 4 error\n");
		goto ERROR_HANDLE;
	}

	ret = i2c_smbus_read_i2c_block_data(info->client,master_write_buf[0], 4, &master_read_buf_array[0]);

	if(ret < 0)
	{
		printk( "menu_sensitivity_show 5 error\n");
		goto ERROR_HANDLE;
	}

	local_back_sensitivity = master_read_buf_array[2] | (master_read_buf_array[3] << 8);


	printk( "%5d\n", local_back_sensitivity);

	return sprintf(buf, "%d\n",  local_back_sensitivity);


ERROR_HANDLE:
	printk( "back sensitivity read Error!!\n");
	return 0;

}

static ssize_t touchkey_threshold_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	int ret;
	ret = sprintf(buf, "%d\n", 15);
	return ret;
}

static DEVICE_ATTR(touchkey_recent, S_IRUGO, menu_sensitivity_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, back_sensitivity_show, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO, touchkey_threshold_show, NULL);
#endif				


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

	pr_cont("\n[  tx] ");
	for (i = 0; i < TX_NUM ; i++) {
		pr_cont("[%2d] ", i);
	}
	pr_cont("\n");
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

		pr_cont("[rx%2d]", i);
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
			pr_cont("%4d,", info->intensity[i * TX_NUM + j]);
		}
		pr_cont("\n");
	}

	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	enable_irq(info->irq);

	return;

err_i2c:
	dev_err(&info->client->dev, "%s: fail to i2c (cmd=%d)\n",
		__func__, MMS_VSC_CMD_INTENSITY);
}

static void get_raw_data(struct mms_ts_info *info, u8 cmd)
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

	pr_cont("\n[  tx] ");
	for (i = 0; i < TX_NUM ; i++) {
		pr_cont("[%2d] ", i);
	}
	pr_cont("\n");

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

		pr_cont("[rx%2d]", i);

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
				pr_cont("%4d,",info->inspection[i * TX_NUM + j]);
			} else if (cmd == MMS_VSC_CMD_CM_ABS) {
				info->raw[i * TX_NUM + j] =
					raw_data;
				pr_cont("%4d,",info->raw[i * TX_NUM + j]);
			} else if (cmd == MMS_VSC_CMD_REFER) {
				info->reference[i * TX_NUM + j] =
					raw_data;
				pr_cont("%4d,",info->reference[i * TX_NUM + j]);
			}
		}
		pr_cont("\n");
	}

	ret = i2c_smbus_write_byte_data(info->client,
		ADDR_UNIV_CMD, CMD_EXIT_TEST);

	snprintf(buff, sizeof(buff), "%d,%d", min_value, max_value);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	touch_is_pressed = 0;
	mms_clear_input_data(info);

	mms_reboot(info);
	info->enabled = true;


      printk("%s  ,   %d \n",__func__, __LINE__);
	if (info->ta_status) {
		dev_notice(&info->client->dev, "TA connect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x30, 0x1);
	} else {
		dev_notice(&info->client->dev, "TA disconnect!!!\n");
		i2c_smbus_write_byte_data(info->client, 0x30, 0x2);
	}
	mms_set_noise_mode(info);

	enable_irq(info->irq);

	return;

err_i2c:
	dev_err(&info->client->dev, "%s: fail to i2c (cmd=%d)\n",
		__func__, cmd);
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
		info->cmd_state = 3;

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
	info->cmd_state = 4;
	dev_info(&info->client->dev, "%s: \"%s(%d)\"\n", __func__,
				buff, strnlen(buff, sizeof(buff)));
	return;
}

static void fw_update(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;
	struct i2c_client *client = info->client;
	int ret = 0;
	int fw_bin_ver=FW_VERSION, fw_core_ver=CORE_VERSION;
	char result[16] = {0};

	set_default_result(info);
	dev_info(&client->dev,
		"fw_ic_ver = 0x%02x, fw_bin_ver = 0x%02x\n",info->fw_ic_ver, fw_bin_ver);
	dev_info(&client->dev,
		"core_ic_ver = 0x%02x, core_bin_ver = 0x%02x\n",info->fw_core_ver, fw_core_ver);

	switch (info->cmd_param[0]) {
	case BUILT_IN:
		disable_irq(info->irq);
		ret=mms_ts_config(info,BUILT_IN);
		enable_irq(info->irq);
		if (ret){
			goto update_fail;
		}
		else {
			info->cmd_state = 2;
			snprintf(result, sizeof(result), "OK");
			set_cmd_result(info, result, strnlen(result, sizeof(result)));
			return;
		}
		break;

	case UMS:
		disable_irq(info->irq);
		ret=mms_ts_config(info,UMS);	
		enable_irq(info->irq);
		if (ret){
			goto update_fail;
		}
		else {
			info->cmd_state = 2;
			snprintf(result, sizeof(result), "OK");
			set_cmd_result(info, result, strnlen(result, sizeof(result)));
			return;
		}
		break;
	default:
		dev_err(&client->dev, "invalid fw file type!!\n");
	}

update_fail:
	info->cmd_state = 3;
	snprintf(result, sizeof(result) , "%s", "NG");
	set_cmd_result(info, result, strnlen(result, sizeof(result)));
	return;
}


#if FLIP_COVER_TEST
static void flip_cover_enable(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	int status = 0;
	char buff[16] = {0};

	set_default_result(info);

	status = set_conifg_flip_cover(info, info->cmd_param[0]);

	dev_info(&info->client->dev, "%s: flip_cover %s %s.\n",
			__func__, info->cmd_param ? "enable" : "disable", status == 0 ? "successed" : "failed");

	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	info->cmd_state = 2;

	dev_info(&info->client->dev, "%s\n", __func__);
}

static int set_conifg_flip_cover(struct mms_ts_info *info,int enables)
{
    int retval = 0;

	if (enables) {

        retval = note_flip_open(info);

	} else {

        retval = note_flip_close(info);
	}

	return retval;
}
#define RMI_ADDR_UNIV_CMD 			0xA0
#define UNIVCMD_SET_CUSTOM_VALUE 0x80 //Use Custom value for flip cover defence
static int note_flip_open(struct mms_ts_info *info )
{
    int retval = 0;

	master_write_buf[0] = RMI_ADDR_UNIV_CMD;
	master_write_buf[1] = UNIVCMD_SET_CUSTOM_VALUE;
	master_write_buf[2] = 0; //Exciting CH.
	master_write_buf[3] = 0; //Sensing CH.

	
	printk("%s Enter\n", __func__);
	retval = i2c_smbus_write_i2c_block_data(info->client,master_write_buf[0], 4, &master_write_buf[1]);
	if(retval < 0)
	{
		printk( "open error\n", __func__);
		return -1;
	}
	return retval;
}

static int note_flip_close(struct mms_ts_info *info )
{
    int retval = 0;
	
	master_write_buf[0] = RMI_ADDR_UNIV_CMD;
	master_write_buf[1] = UNIVCMD_SET_CUSTOM_VALUE;
	master_write_buf[2] = 0; //Exciting CH.
	master_write_buf[3] = 1; //Sensing CH.

	
	printk("%s Enter\n", __func__);
	retval = i2c_smbus_write_i2c_block_data(info->client,master_write_buf[0], 4, &master_write_buf[1]);
	if(retval < 0)
	{
		printk( "close error\n", __func__);
		return -1;
	}
	return retval;
}
#endif


static void get_fw_ver_bin(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};

	set_default_result(info);
	snprintf(buff, sizeof(buff), "ME%02x%02x%02x", MODU_VERSION, CORE_VERSION, FW_VERSION);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = 2;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_fw_ver_ic(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};

	set_default_result(info);

	if (info->enabled) {
		info->fw_core_ver = get_fw_version(info, SEC_CORE);
		info->fw_ic_ver = get_fw_version(info, SEC_CONFIG);
		info->fw_module_ver = get_fw_version(info, SEC_MODULE);
	}
	snprintf(buff, sizeof(buff), "ME%02x%02x%02x", info->fw_module_ver, info->fw_core_ver, info->fw_ic_ver);

	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = 2;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_config_ver(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[20] = {0};

	set_default_result(info);

	snprintf(buff, sizeof(buff), "NA");//G3588_ME_0124");

	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = 2;
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
		info->cmd_state = 3;
		return;
}
	snprintf(buff, sizeof(buff), "%d", threshold);

	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = 2;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void module_off_master(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[3] = {0};

	//mms_ts_suspend(&info->client->dev);
	mms_ts_disable(info);

//	if (info->pdata->is_vdd_on() == 0)
		snprintf(buff, sizeof(buff), "%s", "OK");
//	else
//		snprintf(buff, sizeof(buff), "%s", "NG");

	set_default_result(info);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	if (strncmp(buff, "OK", 2) == 0)
		info->cmd_state = 2;
	else
		info->cmd_state = 3;

	info->cmd_is_running = false;

	dev_info(&info->client->dev, "%s: %s\n", __func__, buff);
}

static void module_on_master(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[3] = {0};

	//mms_ts_resume(&info->client->dev);
	mms_ts_enable(info);

//	if (info->pdata->is_vdd_on() == 1)
		snprintf(buff, sizeof(buff), "%s", "OK");
//	else
//		snprintf(buff, sizeof(buff), "%s", "NG");

	set_default_result(info);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));

	if (strncmp(buff, "OK", 2) == 0)
		info->cmd_state = 2;
	else
		info->cmd_state = 3;

	info->cmd_is_running = false;

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
static void get_chip_vendor(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};

	set_default_result(info);

	snprintf(buff, sizeof(buff), "%s", "MELFAS");
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = 2;
	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__,
			buff, strnlen(buff, sizeof(buff)));
}

static void get_chip_name(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};

	set_default_result(info);

	snprintf(buff, sizeof(buff), "%s", "MMS244");
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = 2;
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

	info->cmd_state = 2;

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
	info->cmd_state = 2;

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
	info->cmd_state = 2;

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
	info->cmd_state = 2;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_x_num(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	int val;
	u8 r_buf[2];
	int ret;

	set_default_result(info);

	ret = i2c_smbus_read_i2c_block_data(info->client,
		ADDR_CH_NUM, 2, r_buf);
	val = r_buf[0];
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = 3;

			dev_info(&info->client->dev,
			"%s: fail to read num of x (%d).\n",
			__func__, val);
		return;
	}

	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = 2;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void get_y_num(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	char buff[16] = {0};
	int val;
	u8 r_buf[2];
	int ret;

	set_default_result(info);

	ret = i2c_smbus_read_i2c_block_data(info->client,
		ADDR_CH_NUM, 2, r_buf);
	val = r_buf[1];
	if (ret < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
		info->cmd_state = 3;
			
		dev_info(&info->client->dev,
			"%s: fail to read num of x (%d).\n",
			__func__, val);
		return;
	}

	snprintf(buff, sizeof(buff), "%u", val);
	set_cmd_result(info, buff, strnlen(buff, sizeof(buff)));
	info->cmd_state = 2;

	dev_info(&info->client->dev, "%s: %s(%d)\n", __func__, buff,
			strnlen(buff, sizeof(buff)));
}

static void run_reference_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);
	get_raw_data(info, MMS_VSC_CMD_REFER);
	info->cmd_state = 2;
}

static void run_cm_abs_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);
	get_raw_data(info, MMS_VSC_CMD_CM_ABS);
	info->cmd_state = 2;
}

static void run_cm_delta_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);
	get_raw_data(info, MMS_VSC_CMD_CM_DELTA);
	info->cmd_state = 2;
}

static void run_intensity_read(void *device_data)
{
	struct mms_ts_info *info = (struct mms_ts_info *)device_data;

	set_default_result(info);
	get_intensity_data(info);
	info->cmd_state = 2;
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

	info->cmd_state = 1;

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

	if (info->cmd_state == 0)
		snprintf(buff, sizeof(buff), "WAITING");

	else if (info->cmd_state == 1)
		snprintf(buff, sizeof(buff), "RUNNING");

	else if (info->cmd_state == 2)
		snprintf(buff, sizeof(buff), "OK");

	else if (info->cmd_state == 3)
		snprintf(buff, sizeof(buff), "FAIL");

	else if (info->cmd_state == 4)
		snprintf(buff, sizeof(buff), "NOT_APPLICABLE");

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

	info->cmd_state = 0;

	return snprintf(buf, TSP_BUF_SIZE, "%s\n", info->cmd_result);
}

static ssize_t cmd_list_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ii = 0;
	char buffer[TSP_CMD_RESULT_STR_LEN];
	char buffer_name[TSP_CMD_STR_LEN];
	struct mms_ts_info *info = dev_get_drvdata(dev);

	snprintf(buffer, 30, "++factory command list++\n");
	while (strncmp(tsp_cmds[ii].cmd_name, "not_support_cmd", 16) != 0) {
		snprintf(buffer_name, TSP_CMD_STR_LEN, "%s\n", tsp_cmds[ii].cmd_name);
		strncat(buffer, buffer_name, strlen(buffer_name));
		ii++;
	}

	dev_info(&info->client->dev,
		"%s: length : %u / %d\n", __func__,
		strlen(buffer), TSP_CMD_RESULT_STR_LEN);
	return snprintf(buf, PAGE_SIZE, "%s\n", buffer);
}

static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, store_cmd);
static DEVICE_ATTR(cmd_status, S_IRUGO, show_cmd_status, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, show_cmd_result, NULL);
static DEVICE_ATTR(cmd_list, S_IRUGO, cmd_list_show, NULL);

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
#endif /* SEC_TSP_FACTORY_TEST */


#ifdef CONFIG_OF
static void melfas_request_gpio(struct mms_ts_platform_data *pdata)
{
	int ret;
	pr_info("[TSP] request gpio\n");

	ret = gpio_request(pdata->gpio_scl, "melfas_tsp_scl");
	if (ret) {
		pr_err("[TSP]%s: unable to request melfas_tsp_scl [%d]\n",
				__func__, pdata->gpio_scl);
		return;
	}

	ret = gpio_request(pdata->gpio_sda, "melfas_tsp_sda");
	if (ret) {
		pr_err("[TSP]%s: unable to request melfas_tsp_sda [%d]\n",
				__func__, pdata->gpio_sda);
		return;
	}

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
}



static int mms_get_dt_coords(struct device *dev, char *name,
				struct mms_ts_platform_data *pdata)
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

static int mms_parse_dt(struct device *dev,
			struct mms_ts_platform_data *pdata)
{
	int rc;
	struct device_node *np = dev->of_node;

	rc = mms_get_dt_coords(dev, "melfas,panel-coords", pdata);
	if (rc)
		return rc;

	/* regulator info */
	pdata->i2c_pull_up = of_property_read_bool(np, "melfas,i2c-pull-up");
	pdata->vdd_en = of_get_named_gpio(np, "vdd_en-gpio", 0);

	/* reset, irq gpio info */
	pdata->gpio_scl = of_get_named_gpio_flags(np, "melfas,scl-gpio",
				0, &pdata->scl_gpio_flags);
	pdata->gpio_sda = of_get_named_gpio_flags(np, "melfas,sda-gpio",
				0, &pdata->sda_gpio_flags);
	pdata->gpio_int = of_get_named_gpio_flags(np, "melfas,irq-gpio",
				0, &pdata->irq_gpio_flags);
//data->config_fw_version = of_get_property(np,
//		"melfas,config_fw_version", NULL);
	return 0;
}
#else
static int mms_parse_dt(struct device *dev,
			struct mms_ts_platform_data *pdata)
{
	return -ENODEV;
}
#endif


int __devinit mms_ts_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct mms_ts_platform_data *pdata;
	struct mms_ts_info *info;
	struct input_dev *input_dev;
	int ret = 0;
	int error;

	#ifdef SEC_TSP_FACTORY_TEST
	int i;
	struct device *fac_dev_ts;
	#endif

	printk("%s +++ %d\n",__func__,__LINE__);
	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;
#ifdef CONFIG_OF
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct mms_ts_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		error = mms_parse_dt(&client->dev, pdata);
		if (error)
			return error;
	} else
		pdata = client->dev.platform_data;

	if (!pdata)
		return -EINVAL;

	melfas_request_gpio(pdata);
#endif
	info = kzalloc(sizeof(struct mms_ts_info), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!info || !input_dev) {
		dev_err(&client->dev, " Failed to allocate memory\n");
		input_free_device(input_dev);
		kfree(info);
		return -ENOMEM;
	}

	info->client = client;
	info->input_dev = input_dev;
	//info->pdata = client->dev.platform_data;
	//init_completion(&info->init_done);
	info->irq = -1;
	//mutex_init(&info->lock);
	info->pdata = pdata;

	info->max_x = info->pdata->max_x;
	info->max_y = info->pdata->max_y;

	melfas_vdd_on(info, 1);
	msleep(100);

	info->register_cb = melfas_tsp_register_callback;
	info->ta_status = 0;	// init value

	input_mt_init_slots(input_dev, MAX_FINGER_NUM);

//	snprintf(info->phys, sizeof(info->phys),
//		"%s/input0", dev_name(&client->dev));
	
	input_dev->name = "sec_touchscreen";//"Melfas MMSxxx Touchscreen";
	input_dev->phys = info->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->open = mms_ts_input_open;
	input_dev->close = mms_ts_input_close;

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, MAX_WIDTH, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, MAX_PRESSURE, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, info->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, info->max_y, 0, 0);

	input_set_drvdata(input_dev, info);

#if MMS_HAS_TOUCH_KEY
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(KEY_RECENT, input_dev->keybit);
	__set_bit(KEY_BACK, input_dev->keybit);
#endif

#if TOUCH_BOOSTER
	mutex_init(&info->dvfs_lock);
	INIT_DELAYED_WORK(&info->work_dvfs_off, set_dvfs_off);
	INIT_DELAYED_WORK(&info->work_dvfs_chg, change_dvfs_lock);
	info->dvfs_lock_status = false;
#endif

	ret = input_register_device(input_dev);
	if (ret) {
		dev_err(&client->dev, "failed to register input dev\n");
		return -EIO;
	}

	i2c_set_clientdata(client, info);

	ret = request_threaded_irq(client->irq, NULL, mms_ts_interrupt,
				   IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				   "mms_ts", info);
	disable_irq(client->irq);
	mms_ts_config(info,BUILT_IN);
	info->irq = client->irq;

	info->tx_num = i2c_smbus_read_byte_data(client, MMS_TX_NUM);
	info->rx_num = i2c_smbus_read_byte_data(client, MMS_RX_NUM);
	info->key_num = i2c_smbus_read_byte_data(client, MMS_KEY_NUM);

	esd_cnt = 0;


	info->callbacks.inform_charger = melfas_ta_cb;
	if (info->register_cb) {
		info->register_cb(&info->callbacks);;
	}
	

	enable_irq(info->irq);

#ifdef CONFIG_HAS_EARLYSUSPEND
	info->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +1;
	info->early_suspend.suspend = mms_ts_early_suspend;
	info->early_suspend.resume = mms_ts_late_resume;
	register_early_suspend(&info->early_suspend);
#endif

	if (alloc_chrdev_region(&info->mms_dev, 0, 1, "mms_ts")) {
		dev_err(&client->dev, "failed to allocated device region\n");
		return -ENOMEM;
	}

	info->class = class_create(THIS_MODULE, "mms_ts");

	cdev_init(&info->cdev, &mms_fops);
	info->cdev.owner = THIS_MODULE;

	if (cdev_add(&info->cdev, info->mms_dev, 1)) {
		dev_err(&client->dev, "failed to add ch dev\n");
		return -EIO;
	}

	sec_touchscreen = device_create(sec_class,
					NULL, 0, info, "sec_touchscreen");
	if (IS_ERR(sec_touchscreen)) {
		dev_err(&client->dev,
			"Failed to create device for the sysfs1\n");
		ret = -ENODEV;
	}
	

#ifdef SEC_TOUCHKEY_INFO
	sec_touchkey =
	device_create(sec_class, NULL, 0, info, "sec_touchkey");

	  if (device_create_file(sec_touchkey, &dev_attr_touchkey_recent) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_recent.attr.name);
	if (device_create_file(sec_touchkey, &dev_attr_touchkey_back) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_back.attr.name);
	  if (device_create_file(sec_touchkey, &dev_attr_touchkey_threshold) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_touchkey_threshold.attr.name);
#endif

	device_create(info->class, NULL, info->mms_dev, NULL, "mms_ts");

	if(sysfs_create_bin_file(&client->dev.kobj ,&bin_attr)){
		dev_err(&client->dev, "failed to create sysfs symlink\n");
		return -EAGAIN;
	}

	if(sysfs_create_bin_file(&client->dev.kobj ,&bin_attr_data)){
		dev_err(&client->dev, "failed to create sysfs symlink\n");
		return -EAGAIN;
	}

	if (sysfs_create_link(NULL, &client->dev.kobj, "mms_ts")) {
		dev_err(&client->dev, "failed to create sysfs symlink\n");
		return -EAGAIN;
	}

#ifdef SEC_TSP_FACTORY_TEST
			INIT_LIST_HEAD(&info->cmd_list_head);
			for (i = 0; i < ARRAY_SIZE(tsp_cmds); i++)
				list_add_tail(&tsp_cmds[i].list, &info->cmd_list_head);
	
			mutex_init(&info->cmd_lock);
			info->cmd_is_running = false;
	
		fac_dev_ts = device_create(sec_class,
				NULL, 0, info, "tsp");
		if (IS_ERR(fac_dev_ts))
			dev_err(&client->dev, "Failed to create device for the sysfs\n");
	
		ret = sysfs_create_group(&fac_dev_ts->kobj,
					 &sec_touch_factory_attr_group);
		if (ret)
			dev_err(&client->dev, "Failed to create sysfs group\n");

		ret = sysfs_create_link(&fac_dev_ts->kobj, &info->input_dev->dev.kobj, "input");
		if (ret < 0)
			dev_err(&info->client->dev, "%s: Failed to create input symbolic link[%d]\n",
					__func__, ret);
#endif

	info->log = kzalloc(sizeof(struct mms_log_data), GFP_KERNEL);
	info->fw_name = kstrdup("mms_fw", GFP_KERNEL);

	dev_notice(&client->dev, "mms dev initialized\n");
	printk("%s --- %d\n",__func__,__LINE__);
	return 0;
}

static int __devexit mms_ts_remove(struct i2c_client *client)
{
	struct mms_ts_info *info = i2c_get_clientdata(client);

	if (info->irq >= 0)
		free_irq(info->irq, info);

	sysfs_remove_link(NULL, "mms_ts");
	sysfs_remove_bin_file(&client->dev.kobj, &bin_attr);
	sysfs_remove_bin_file(&client->dev.kobj, &bin_attr_data);
	input_unregister_device(info->input_dev);
	unregister_early_suspend(&info->early_suspend);
	
	device_destroy(info->class, info->mms_dev);
	class_destroy(info->class);
	
	kfree(info->log);
	kfree(info->fw_name);
	kfree(info);

	return 0;
}

#if (defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)) && !defined(USE_OPEN_CLOSE)
static int mms_ts_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mms_ts_info *info = i2c_get_clientdata(client);

	mutex_lock(&info->input_dev->mutex);
	if (!info->input_dev->users)
		goto out;

	mms_ts_disable(info);
	mms_clear_input_data(info);

out:
	mutex_unlock(&info->input_dev->mutex);
	return 0;

}

static int mms_ts_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mms_ts_info *info = i2c_get_clientdata(client);
	int ret = 0;

	mutex_lock(&info->input_dev->mutex);
	if (info->input_dev->users)
		ret = mms_ts_enable(info);
	mutex_unlock(&info->input_dev->mutex);

	return ret;
}
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)  && !defined(USE_OPEN_CLOSE)
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

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE)
static const struct dev_pm_ops mms_ts_pm_ops = {
	.suspend	= mms_ts_suspend,
	.resume		= mms_ts_resume,
};
#endif

static const struct i2c_device_id mms_ts_id[] = {
	{MELFAS_TS_NAME, 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, mms_ts_id);

#ifdef CONFIG_OF
static struct of_device_id mms_match_table[] = {
	{ .compatible = "melfas,mms-ts",},
	{ },
};
#else
#define mms_match_table NULL
#endif

static struct i2c_driver mms_ts_driver = {
	.probe		= mms_ts_probe,
	.remove		= __devexit_p(mms_ts_remove),
	.driver		= {
				.name 	= MELFAS_TS_NAME,
#ifdef CONFIG_OF
		   .of_match_table = mms_match_table,
#endif
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE)
				.pm	= &mms_ts_pm_ops,
#endif
	},
	.id_table	= mms_ts_id,
};

static int __init mms_ts_init(void)
{
	return i2c_add_driver(&mms_ts_driver);
}

static void __exit mms_ts_exit(void)
{
	i2c_del_driver(&mms_ts_driver);
}

module_init(mms_ts_init);
module_exit(mms_ts_exit);

MODULE_VERSION("1.1.1");
MODULE_DESCRIPTION("Touchscreen driver for MELFAS MMS-series");
MODULE_LICENSE("GPL");

