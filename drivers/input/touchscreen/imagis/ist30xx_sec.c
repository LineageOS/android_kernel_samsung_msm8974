/*
 *  Copyright (C) 2010,Imagis Technology Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */


#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <linux/ioctl.h>
#include <linux/platform_device.h>
#include <linux/module.h>

#include <linux/input.h>
#include <linux/uaccess.h>

#include "ist30xx.h"
#include "ist30xx_update.h"
#include "ist30xx_misc.h"

#define FW_DOWNLOADING "Downloading"
#define FW_DOWNLOAD_COMPLETE "Complete"
#define FW_DOWNLOAD_FAIL "FAIL"
#if defined(CONFIG_MACH_KANAS3G_CTC)
#define FW_RELEASE_DATE "0528"
#elif defined(CONFIG_SEC_KANAS_PROJECT)
#define FW_RELEASE_DATE "0526"
#endif

#define FACTORY_BUF_SIZE    (1024)
#define BUILT_IN            (0)
#define SDCARD_IN         (1)

#define CMD_STATE_WAITING   (0)
#define CMD_STATE_RUNNING   (1)
#define CMD_STATE_OK        (2)
#define CMD_STATE_FAIL      (3)
#define CMD_STATE_NA        (4)

#define TSP_NODE_DEBUG      (1)

static u16 node_value[TSP_TOTAL_NUM];

extern struct ist30xx_data *ts_data;
extern TSP_INFO ist30xx_tsp_info;
extern TKEY_INFO ist30xx_tkey_info;


#define TSP_CMD(name, func) .cmd_name = name, .cmd_func = func
struct tsp_cmd {
	struct list_head	list;
	const char *		cmd_name;
	void			(*cmd_func)(void *dev_data);
};

u32 ist30xxb_get_fw_ver(struct ist30xx_data *data)
{
	u32 addr = data->tags.cfg_addr + 4;
	u32 len = 1;
	u32 ver = 0;
	int ret = -EPERM;
	int cnt = 2;

get_fw_ver_start:
	cnt--;

	ist30xx_disable_irq(data);
	ret = ist30xx_cmd_reg(data->client, CMD_ENTER_REG_ACCESS);
	if (unlikely(ret)) goto get_fw_ver_fail;

	ret = ist30xx_write_cmd(data->client, IST30XX_RX_CNT_ADDR, len);
	if (unlikely(ret)) goto get_fw_ver_fail;

	ret = ist30xx_read_cmd(data->client, addr, &ver);
	if (unlikely(ret)) goto get_fw_ver_fail;

	tsp_debug("%s: Reg addr: %x, ver: %x\n", __func__, addr, ver);

	ret = ist30xx_cmd_reg(data->client, CMD_EXIT_REG_ACCESS);
	if (likely(ret == 0))
		goto get_fw_ver_end;

get_fw_ver_fail:
	ist30xx_cmd_run_device(data->client, true);
	if(cnt == 1) {
		tsp_info("%s(), try to get fw version again\n", __func__);
		goto get_fw_ver_start;
	}

get_fw_ver_end:
	ist30xx_cmd_start_scan(data->client);
	ist30xx_enable_irq(data);

	return ver;
}

int ist30xxb_get_key_sensitivity(struct ist30xx_data *data, int id)
{
	u32 addr = IST30XXB_MEM_COUNT + 4 * sizeof(u32);
	u32 val = 0;

	if (unlikely(id >= ist30xx_tkey_info.key_num))
		return 0;

	if (ist30xx_intr_wait(30) < 0) return 0;

	ist30xx_disable_irq(data);
	ist30xx_read_cmd(data->client, addr, &val);
	ist30xx_enable_irq(data);

	tsp_debug("%s: Reg addr: %x, val: %8x\n", __func__, addr, val);

	val >>= (16 * id);

	return (int)(val & 0xFFFF);
}


/* Factory CMD function */
static void set_default_result(struct sec_factory *sec)
{
	char delim = ':';

	memset(sec->cmd_result, 0, ARRAY_SIZE(sec->cmd_result));
	memcpy(sec->cmd_result, sec->cmd, strlen(sec->cmd));
	strncat(sec->cmd_result, &delim, CMD_STATE_RUNNING);
}

static void set_cmd_result(struct sec_factory *sec, char *buf, int len)
{
	strncat(sec->cmd_result, buf, len);
}

static void not_support_cmd(void *dev_data)
{
	char buf[16] = { 0 };

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	set_default_result(sec);
	snprintf(buf, sizeof(buf), "%s", "NA");

	set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	sec->cmd_state = CMD_STATE_NA;
	tsp_info("%s(): %s(%d)\n", __func__, buf,
		 strnlen(buf, sizeof(buf)));

	mutex_lock(&sec->cmd_lock);
	sec->cmd_is_running = false;
	mutex_unlock(&sec->cmd_lock);

	sec->cmd_state = CMD_STATE_WAITING;
	return;
}

static void get_chip_vendor(void *dev_data)
{
	char buf[16] = { 0 };

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	set_default_result(sec);
	snprintf(buf, sizeof(buf), "%s", TSP_CHIP_VENDOR);

	set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	sec->cmd_state = CMD_STATE_OK;
	tsp_info("%s(): %s(%d)\n", __func__, buf,
		 strnlen(buf, sizeof(buf)));
}

static void get_chip_name(void *dev_data)
{
	char buf[16] = { 0 };

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	set_default_result(sec);
	snprintf(buf, sizeof(buf), "%s", TSP_CHIP_NAME);

	set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	sec->cmd_state = CMD_STATE_OK;
	tsp_info("%s(): %s(%d)\n", __func__, buf,
		 strnlen(buf, sizeof(buf)));
}

static void get_chip_id(void *dev_data)
{
	char buf[16] = { 0 };

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	set_default_result(sec);
	snprintf(buf, sizeof(buf), "%#02x", data->chip_id);

	set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	sec->cmd_state = CMD_STATE_OK;
	tsp_info("%s(): %s(%d)\n", __func__, buf,
		 strnlen(buf, sizeof(buf)));
}

extern const u8 *ts_fw;
extern u32 ts_fw_size;
static inline unsigned char str2int(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	tsp_info("%s: character : %c\n", __func__,c);
	return 0;
}

static void fw_string_to_arr(uint8_t *src,int start,uint8_t *arr)
{
	int i,j = 0;
	int flag_start = 0;
	for(i = start;src[i] != ';';i++)
	{
		if(src[i] == '{')
			flag_start = 1;

		while(src[i] == ',' && flag_start == 1)
		{
			arr[j] = (unsigned char)(str2int(src[i-2]) * 16 + str2int(src[i-1]));
			j++;
			break;
		}
	}
	arr[j] = (unsigned char)(str2int(src[i-3]) * 16 + str2int(src[i-2]));
	j++;
	tsp_info("%s: size of the file : %d, ts_fw_size=%d, 0x%x\n", __func__, j, ts_fw_size, arr[j-1]);
}

static int read_data_from_file(unsigned char *fw_data)
{
	//read the fw data from sdcard
	int ret = 0;
	long file_size = 0;
	uint8_t *file_data;
	struct file *filp;
	loff_t pos;
	mm_segment_t oldfs;
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open("/sdcard/ist30xx_fw.h", O_RDONLY, 0);
	if (IS_ERR(filp)) {
		tsp_err("%s: file open error:%d\n", __func__, (s32)filp);
		return -1;
	}

	file_size = filp->f_path.dentry->d_inode->i_size;
	tsp_info("%s: size of the file : %ld(bytes)\n", __func__, file_size);
	file_data = kzalloc(file_size, GFP_KERNEL);
	pos = 0;
	ret = vfs_read(filp, (char __user *)file_data, file_size, &pos);
	if (ret != file_size) {
		tsp_err("%s: tsp fw. : failed to read file (ret = %d)\n",
			__func__, ret);
		kfree(file_data);
		filp_close(filp, current->files);
		return -1;
	}

	filp_close(filp, current->files);
	set_fs(oldfs);

	fw_string_to_arr(file_data,0,fw_data);

	kfree(file_data);

	return 0;
}
static void fw_update(void *dev_data)
{
	int ret;
	char buf[16] = { 0 };
	unsigned char *fw_data;

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	set_default_result(sec);

	tsp_info("%s(), %d\n", __func__, sec->cmd_param[0]);

	if(data->status.power == 0) {
		tsp_warn("%s: power off, fw_update stop, line %d\n", __func__, __LINE__);
		sec->cmd_state = CMD_STATE_FAIL;
		goto set_fw_update_result;
	}

	switch (sec->cmd_param[0]) {
	case BUILT_IN:
		sec->cmd_state = CMD_STATE_OK;
		ret = ist30xx_fw_recovery(data);
		if (ret < 0) {
			sec->cmd_state = CMD_STATE_FAIL;
			goto set_fw_update_result;
		}
		break;
	case SDCARD_IN:
		sec->cmd_state = CMD_STATE_OK;
		fw_data = kzalloc(ts_fw_size, GFP_KERNEL);

		ret = read_data_from_file(fw_data);
		if(ret < 0){
			tsp_warn("%s: read fw file error!", __func__);
			sec->cmd_state = CMD_STATE_FAIL;
			kfree(fw_data);
			goto set_fw_update_result;
		}
		ret = ist30xx_fw_force_sdcard(data, fw_data, ts_fw_size);

		if(ret<0) {
			sec->cmd_state = CMD_STATE_FAIL;
			kfree(fw_data);
			goto set_fw_update_result;
		}
		kfree(fw_data);
		break;
	default:
		tsp_warn("%s(), Invalid fw file type!\n", __func__);
		sec->cmd_state = CMD_STATE_FAIL;
		break;
	}

set_fw_update_result:

	if (sec->cmd_state == CMD_STATE_OK)
		snprintf(buf, sizeof(buf), "%s", "OK");
	else
		snprintf(buf, sizeof(buf), "%s", "NG");

	set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
}


static void get_fw_ver_bin(void *dev_data)
{
	u32 ver = 0;
	char buf[16] = { 0 };

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	set_default_result(sec);

	ver = ist30xx_parse_ver(FLAG_PARAM, data->fw.buf);
	snprintf(buf, sizeof(buf), "IM00%04x", ver);

	set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	sec->cmd_state = CMD_STATE_OK;
	tsp_info("%s(): %s(%d)\n", __func__, buf,
		 strnlen(buf, sizeof(buf)));
}

static void get_fw_ver_ic(void *dev_data)
{
	u32 ver = 0;
	char buf[16] = { 0 };

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	set_default_result(sec);

	if (data->status.power == 1)
		ver = ist30xxb_get_fw_ver(ts_data);

	snprintf(buf, sizeof(buf), "IM00%04x", ver & 0xFFFF);
	tsp_info("%s(), %s\n", __func__, buf);

	set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	sec->cmd_state = CMD_STATE_OK;
	tsp_info("%s(): %s(%d)\n", __func__, buf,
		 strnlen(buf, sizeof(buf)));
}

static void get_config_ver(void *dev_data)
{
	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	char buff[255] = {0};

	set_default_result(sec);

#if defined(CONFIG_MACH_KANAS3G_CTC)
	snprintf(buff, sizeof(buff), "%s_%s", "SM-G3559_IM", FW_RELEASE_DATE);
#elif defined(CONFIG_MACH_KANAS3G_CMCC)
	snprintf(buff, sizeof(buff), "%s_%s", "SM-G3558_IM", FW_RELEASE_DATE);
#elif defined(CONFIG_MACH_KANAS3G_CU)
	snprintf(buff, sizeof(buff), "%s_%s", "SM-G3556D_IM", FW_RELEASE_DATE);
#else
	snprintf(buff, sizeof(buff), "%s_%s", TSP_CHIP_VENDOR, TSP_CHIP_NAME);
#endif

	set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = CMD_STATE_OK;
	tsp_info("%s(): %s(%d)\n", __func__, buff,
		strnlen(buff, sizeof(buff)));
}

static void get_threshold(void *dev_data)
{
	char buf[16] = { 0 };
	int val = TSP_THRESHOLD;
#if defined(CONFIG_SEC_KANAS_PROJECT)
	u32 *cfg_buf;
#endif

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	set_default_result(sec);

#if defined(CONFIG_SEC_KANAS_PROJECT)
	ist30xx_get_update_info(data, data->fw.buf, data->fw.buf_size);
	cfg_buf = (u32 *)&data->fw.buf[data->tags.cfg_addr];

	val = (int)(cfg_buf[0x108 / IST30XX_DATA_LEN] >> 16);
#endif

	snprintf(buf, sizeof(buf), "%d", val);

	set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	sec->cmd_state = CMD_STATE_OK;
	tsp_info("%s(): %s(%d)\n", __func__, buf,
		 strnlen(buf, sizeof(buf)));
}

static void get_x_num(void *dev_data)
{
	int val = ist30xx_tsp_info.width;
	char buf[16] = { 0 };

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	set_default_result(sec);

	if (val >= 0) {
		snprintf(buf, sizeof(buf), "%u", val);
		sec->cmd_state = CMD_STATE_OK;
	} else {
		snprintf(buf, sizeof(buf), "%s", "NG");
		sec->cmd_state = CMD_STATE_FAIL;
		tsp_err("%s: fail to read num of x (%d).\n", __func__, val);
	}
	set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	tsp_info("%s(): %s(%d)\n", __func__, buf,
		 strnlen(buf, sizeof(buf)));
}

static void get_y_num(void *dev_data)
{
	int val = ist30xx_tsp_info.height;
	char buf[16] = { 0 };

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	set_default_result(sec);

	if (val >= 0) {
		snprintf(buf, sizeof(buf), "%u", val);
		sec->cmd_state = CMD_STATE_OK;
	} else {
		snprintf(buf, sizeof(buf), "%s", "NG");
		sec->cmd_state = CMD_STATE_FAIL;
		tsp_err("%s: fail to read num of x (%d).\n", __func__, val);
	}
	set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	tsp_info("%s(): %s(%d)\n", __func__, buf,
		 strnlen(buf, sizeof(buf)));
}

int check_tsp_channel(void *dev_data)
{
	int node = -EPERM;

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;
	TSP_INFO *tsp = &ist30xx_tsp_info;

	if ((sec->cmd_param[0] < 0) || (sec->cmd_param[0] >= tsp->width) ||
	    (sec->cmd_param[1] < 0) || (sec->cmd_param[1] >= tsp->height)) {
		tsp_info("%s: parameter error: %u,%u\n",
			 __func__, sec->cmd_param[0], sec->cmd_param[1]);
	} else {
		node = sec->cmd_param[0] + sec->cmd_param[1] * tsp->width;
		tsp_info("%s: node = %d\n", __func__, node);
	}

	return node;
}


extern int parse_tsp_node(u8 flag, struct TSP_NODE_BUF *node, s16 *buf16);
void run_raw_read(void *dev_data)
{
	int i, ret;
	int min_val, max_val;
	char buf[16] = { 0 };
	u8 flag = NODE_FLAG_RAW;

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;
	TSP_INFO *tsp = &ist30xx_tsp_info;

	set_default_result(sec);

	ret = ist30xx_read_touch_node(flag, &tsp->node);
	if (ret) {
		sec->cmd_state = CMD_STATE_FAIL;
		tsp_warn("%s(), tsp node read fail!\n", __func__);
		return;
	}
	ist30xx_parse_touch_node(flag, &tsp->node);

	ret = parse_tsp_node(flag, &tsp->node, node_value);
	if (ret) {
		sec->cmd_state = CMD_STATE_FAIL;
		tsp_warn("%s(), tsp node parse fail - flag: %d\n", __func__, flag);
		return;
	}

	min_val = max_val = node_value[0];
	for (i = 0; i < tsp->width * tsp->height; i++) {
#if TSP_NODE_DEBUG
		if ((i % tsp->width) == 0)
			printk("\n%s %4d: ", IST30XX_DEBUG_TAG, i);

		printk("%4d ", node_value[i]);
#endif

		max_val = max(max_val, (int)node_value[i]);
		min_val = min(min_val, (int)node_value[i]);
	}
	printk("\n");

	snprintf(buf, sizeof(buf), "%d,%d", min_val, max_val);
	sec->cmd_state = CMD_STATE_OK;
	set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	tsp_info("%s(): %s(%d)\n", __func__, buf,
		 strnlen(buf, sizeof(buf)));
}

void get_raw_value(void *dev_data)
{
	int idx = 0;
	char buf[16] = { 0 };

	struct ist30xx_data *data = (struct ist30xx_data *)dev_data;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	set_default_result(sec);

	idx = check_tsp_channel(data);
	if (idx < 0) { // Parameter parsing fail
		snprintf(buf, sizeof(buf), "%s", "NG");
		sec->cmd_state = CMD_STATE_FAIL;
	} else {
		snprintf(buf, sizeof(buf), "%d", node_value[idx]);
		sec->cmd_state = CMD_STATE_OK;
	}

	set_cmd_result(sec, buf, strnlen(buf, sizeof(buf)));
	tsp_info("%s(): %s(%d)\n", __func__, buf,
		 strnlen(buf, sizeof(buf)));
}



/* sysfs: /sys/class/sec/tsp/close_tsp_test */
static ssize_t show_close_tsp_test(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return snprintf(buf, FACTORY_BUF_SIZE, "%u\n", 0);
}

/* sysfs: /sys/class/sec/tsp/cmd */
static ssize_t store_cmd(struct device *dev, struct device_attribute
			 *devattr, const char *buf, size_t count)
{
	struct ist30xx_data *data = dev_get_drvdata(dev);
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	char *cur, *start, *end;
	char msg[SEC_CMD_STR_LEN] = { 0 };
	int len, i;
	struct tsp_cmd *tsp_cmd_ptr = NULL;
	char delim = ',';
	bool cmd_found = false;
	int param_cnt = 0;
	int ret;

	if (strlen(buf) >= SEC_CMD_STR_LEN) {
		tsp_err("%s: cmd length is over(%s,%d)!!\n", __func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	if (sec->cmd_is_running == true) {
		tsp_err("%s: tsp_cmd: other cmd is running.\n", __func__);
		goto err_out;
	}

	/* check lock  */
	mutex_lock(&sec->cmd_lock);
	sec->cmd_is_running = true;
	mutex_unlock(&sec->cmd_lock);

	sec->cmd_state = CMD_STATE_RUNNING;

	for (i = 0; i < ARRAY_SIZE(sec->cmd_param); i++)
		sec->cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;
	memset(sec->cmd, 0, ARRAY_SIZE(sec->cmd));
	memcpy(sec->cmd, buf, len);

	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(msg, buf, cur - buf);
	else
		memcpy(msg, buf, len);
	/* find command */
	list_for_each_entry(tsp_cmd_ptr, &sec->cmd_list_head, list) {
		if (!strcmp(msg, tsp_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(tsp_cmd_ptr, &sec->cmd_list_head, list) {
			if (!strcmp("not_support_cmd", tsp_cmd_ptr->cmd_name))
				break;
		}
	}

	/* parsing parameters */
	if (cur && cmd_found) {
		cur++;
		start = cur;
		memset(msg, 0, ARRAY_SIZE(msg));
		do {
			if (*cur == delim || cur - buf == len) {
				end = cur;
				memcpy(msg, start, end - start);
				*(msg + strlen(msg)) = '\0';
				ret = kstrtoint(msg, 10, \
						sec->cmd_param + param_cnt);
				start = cur + 1;
				memset(msg, 0, ARRAY_SIZE(msg));
				param_cnt++;
			}
			cur++;
		} while ((cur - buf <= len) && (param_cnt < SEC_CMD_PARAM_NUM));
	}
	tsp_info("tsp cmd: %s\n", buf);

	for (i = 0; i < param_cnt; i++)
		tsp_info("tsp cmd param %d= %d\n", i, sec->cmd_param[i]);

	tsp_cmd_ptr->cmd_func(data);

err_out:
	return count;
}

/* sysfs: /sys/class/sec/tsp/cmd_status */
static ssize_t show_cmd_status(struct device *dev,
			       struct device_attribute *devattr, char *buf)
{
	struct ist30xx_data *data = dev_get_drvdata(dev);
	struct sec_factory *sec = (struct sec_factory *)&data->sec;
	char msg[16] = { 0 };

	tsp_info("tsp cmd: status:%d\n", sec->cmd_state);

	if (sec->cmd_state == CMD_STATE_WAITING)
		snprintf(msg, sizeof(msg), "WAITING");

	else if (sec->cmd_state == CMD_STATE_RUNNING)
		snprintf(msg, sizeof(msg), "RUNNING");

	else if (sec->cmd_state == CMD_STATE_OK)
		snprintf(msg, sizeof(msg), "OK");

	else if (sec->cmd_state == CMD_STATE_FAIL)
		snprintf(msg, sizeof(msg), "FAIL");

	else if (sec->cmd_state == CMD_STATE_NA)
		snprintf(msg, sizeof(msg), "NOT_APPLICABLE");

	return snprintf(buf, FACTORY_BUF_SIZE, "%s\n", msg);
}

/* sysfs: /sys/class/sec/tsp/result */
static ssize_t show_cmd_result(struct device *dev, struct device_attribute
			       *devattr, char *buf)
{
	struct ist30xx_data *data = dev_get_drvdata(dev);
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	tsp_info("tsp cmd: result: %s\n", sec->cmd_result);

	mutex_lock(&sec->cmd_lock);
	sec->cmd_is_running = false;
	mutex_unlock(&sec->cmd_lock);

	sec->cmd_state = CMD_STATE_WAITING;

	return snprintf(buf, FACTORY_BUF_SIZE, "%s\n", sec->cmd_result);
}


/* sysfs: /sys/class/sec/sec_touchkey/touchkey_recent */
static ssize_t recent_sensitivity_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int sensitivity = ist30xxb_get_key_sensitivity(ts_data, 0);

	tsp_info("%s(), %d\n", __func__, sensitivity);

	return sprintf(buf, "%d\n", sensitivity);
}

/* sysfs: /sys/class/sec/tsp/sec_touchkey/touchkey_back */
static ssize_t back_sensitivity_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int sensitivity = ist30xxb_get_key_sensitivity(ts_data, 1);

	tsp_info("%s(), %d\n", __func__, sensitivity);

	return sprintf(buf, "%d\n", sensitivity);
}

/* sysfs: /sys/class/sec/tsp/sec_touchkey/touchkey_threshold */
static ssize_t touchkey_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val = TKEY_THRESHOLD;
#if defined(CONFIG_SEC_KANAS_PROJECT)
	u32 *cfg_buf;

	ist30xx_get_update_info(ts_data, ts_data->fw.buf, ts_data->fw.buf_size);
	cfg_buf = (u32 *)&ts_data->fw.buf[ts_data->tags.cfg_addr];

	val = (int)(cfg_buf[0x134 / IST30XX_DATA_LEN] >> 16);
#endif

	tsp_info("%s(), %d\n", __func__, val);

	return snprintf(buf, sizeof(int), "%d\n", val);
}



struct tsp_cmd tsp_cmds[] = {
	{ TSP_CMD("fw_update",	fw_update),       },
	{ TSP_CMD("get_fw_ver_bin",  get_fw_ver_bin),  },
	{ TSP_CMD("get_fw_ver_ic",   get_fw_ver_ic),   },
	{ TSP_CMD("get_config_ver",  get_config_ver),  },
	{ TSP_CMD("get_threshold",   get_threshold),   },
	{ TSP_CMD("get_chip_vendor", get_chip_vendor), },
	{ TSP_CMD("get_chip_name",   get_chip_name),   },
	{ TSP_CMD("get_chip_id",     get_chip_id),     },
	{ TSP_CMD("get_x_num",	     get_x_num),       },
	{ TSP_CMD("get_y_num",	     get_y_num),       },
	{ TSP_CMD("run_reference_read",  run_raw_read),},
	{ TSP_CMD("run_raw_read",    run_raw_read),    },
	{ TSP_CMD("get_reference",   get_raw_value),   },
	{ TSP_CMD("get_raw_value",   get_raw_value),   },
	{ TSP_CMD("not_support_cmd", not_support_cmd), },
};


#define SEC_DEFAULT_ATTR    (S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH)

/* sysfs - touchkey */
static DEVICE_ATTR(touchkey_recent, S_IRUGO,
		   recent_sensitivity_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO,
		   back_sensitivity_show, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO,
		   touchkey_threshold_show, NULL);

/* sysfs - tsp */
static DEVICE_ATTR(close_tsp_test, S_IRUGO, show_close_tsp_test, NULL);
static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, store_cmd);
static DEVICE_ATTR(cmd_status, S_IRUGO, show_cmd_status, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, show_cmd_result, NULL);


static struct attribute *sec_tkey_attributes[] = {
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_threshold.attr,
	NULL,
};

static struct attribute *sec_touch_facotry_attributes[] = {
	&dev_attr_close_tsp_test.attr,
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
	NULL,
};

static struct attribute_group sec_tkey_attr_group = {
	.attrs	= sec_tkey_attributes,
};

static struct attribute_group sec_touch_factory_attr_group = {
	.attrs	= sec_touch_facotry_attributes,
};

struct device *sec_touchkey;
EXPORT_SYMBOL(sec_touchkey);
struct device *sec_fac_dev;
extern struct class *sec_class;

int sec_touch_sysfs(struct ist30xx_data *data)
{

	tsp_info("%s\n", __func__);

	/* /sys/class/sec/sec_touchkey */
	sec_touchkey = device_create(sec_class, NULL, 0, NULL, "sec_touchkey");
	if (IS_ERR(sec_touchkey)) {
		tsp_err("%s: Failed to create device (%s)!\n", __func__, "sec_touchkey");
		return -ENODEV;
	}
	/* /sys/class/sec/sec_touchkey/... */
	if (sysfs_create_group(&sec_touchkey->kobj, &sec_tkey_attr_group))
		tsp_err("%s: Failed to create sysfs group(%s)!\n", __func__, "sec_touchkey");

	/* /sys/class/sec/tsp */
	sec_fac_dev = device_create(sec_class, NULL, 0, data, "tsp");
	if (IS_ERR(sec_fac_dev)) {
		tsp_err("%s: Failed to create device (%s)!\n", __func__, "tsp");
		return -ENODEV;
	}
	/* /sys/class/sec/tsp/... */
	if (sysfs_create_group(&sec_fac_dev->kobj, &sec_touch_factory_attr_group))
		tsp_err("%s: Failed to create sysfs group(%s)!\n", __func__, "tsp");

	return 0;
}
EXPORT_SYMBOL(sec_touch_sysfs);
int sec_fac_cmd_init(struct ist30xx_data *data)
{
	int i;
	struct sec_factory *sec = (struct sec_factory *)&data->sec;

	INIT_LIST_HEAD(&sec->cmd_list_head);
	for (i = 0; i < ARRAY_SIZE(tsp_cmds); i++)
		list_add_tail(&tsp_cmds[i].list, &sec->cmd_list_head);

	mutex_init(&sec->cmd_lock);
	sec->cmd_is_running = false;

	return 0;
}
EXPORT_SYMBOL(sec_fac_cmd_init);
