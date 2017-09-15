/*
 * cyttsp5_samsung_factory.c
 * Cypress TrueTouch(TM) Standard Product V5 Device Access module.
 * Configuration and Test command/status user interface.
 * For use with Cypress Txx5xx parts.
 * Supported parts include:
 * TMA5XX
 *
 * Copyright (C) 2012-2013 Cypress Semiconductor
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contact Cypress Semiconductor at www.cypress.com <ttdrivers@cypress.com>
 *
 */

#include <linux/slab.h>
#include <linux/err.h>

#include "cyttsp5_regs.h"


/************************************************************************
 * Macros, Structures
 ************************************************************************/
#define SEC_DEV_TOUCH_MAJOR			0
#define SEC_DEV_TSP_MINOR			1

#define MAX_NODE_NUM 900 /* 30 * 30 */
#define MAX_INPUT_HEADER_SIZE 12
#define MAX_GIDAC_NODES 32
#define MAX_LIDAC_NODES (MAX_GIDAC_NODES * 30)

#define RECAL_FALSELY_CALED_PANEL
#define SPEC_MUT_RAW_MIN (-19500)
#define SPEC_MUT_RAW_MAX (5008)
#define SPEC_SELF_RAW_MIN (-20000)
#define SPEC_SELF_RAW_MAX (20000)

extern struct class *sec_class;

enum {
	FACTORYCMD_WAITING,
	FACTORYCMD_RUNNING,
	FACTORYCMD_OK,
	FACTORYCMD_FAIL,
	FACTORYCMD_NOT_APPLICABLE
};

enum {
	IDAC_GLOBAL,
	IDAC_LOCAL,
};
#define FACTORY_CMD(name, func) .cmd_name = name, .cmd_func = func

struct factory_cmd {
	struct list_head list;
	const char *cmd_name;
	void (*cmd_func)(void *device_data);
};

/************************************************************************
 * function def
 ************************************************************************/
static void fw_update(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_config_ver(void *device_data);
static void get_threshold(void *device_data);
static void module_off_master(void *device_data);
static void module_on_master(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void get_raw_count(void *device_data);
static void get_difference(void *device_data);
static void get_local_idac(void *device_data);
static void get_global_idac(void *device_data);
static void run_raw_count_read(void *device_data);
static void run_difference_read(void *device_data);
static void run_local_idac_read(void *device_data);
static void run_global_idac_read(void *device_data);
static void hover_enable(void *device_data);
static void glove_mode(void *device_data);
static void clear_cover_mode(void *device_data);
static void fast_glove_mode(void *device_data);
static void baseline_reset(void *device_data);
static void report_rate(void *device_data);
static void stylus_enable(void *device_data);
static void not_support_cmd(void *device_data);

/************************************************************************
 * cmd table
 ************************************************************************/
struct factory_cmd factory_cmds[] = {
	{FACTORY_CMD("fw_update", fw_update),},
	{FACTORY_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{FACTORY_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{FACTORY_CMD("get_config_ver", get_config_ver),},
	{FACTORY_CMD("get_threshold", get_threshold),},
	{FACTORY_CMD("module_off_master", module_off_master),},
	{FACTORY_CMD("module_on_master", module_on_master),},
	{FACTORY_CMD("module_off_slave", not_support_cmd),},
	{FACTORY_CMD("module_on_slave", not_support_cmd),},
	{FACTORY_CMD("get_chip_vendor", get_chip_vendor),},
	{FACTORY_CMD("get_chip_name", get_chip_name),},
	{FACTORY_CMD("get_x_num", get_x_num),},
	{FACTORY_CMD("get_y_num", get_y_num),},
	{FACTORY_CMD("get_raw_count", get_raw_count),},
	{FACTORY_CMD("get_difference", get_difference),},
	{FACTORY_CMD("get_local_idac", get_local_idac),},
	{FACTORY_CMD("get_global_idac", get_global_idac),},
	{FACTORY_CMD("run_raw_count_read", run_raw_count_read),},
	{FACTORY_CMD("run_difference_read", run_difference_read),},
	{FACTORY_CMD("run_local_idac_read", run_local_idac_read),},
	{FACTORY_CMD("run_global_idac_read", run_global_idac_read),},
	{FACTORY_CMD("get_module_vendor", not_support_cmd),},
	{FACTORY_CMD("hover_enable", hover_enable),},
	{FACTORY_CMD("glove_mode", glove_mode),},
	{FACTORY_CMD("clear_cover_mode", clear_cover_mode),},
	{FACTORY_CMD("fast_glove_mode", fast_glove_mode),},
	{FACTORY_CMD("baseline_reset", baseline_reset),},
	{FACTORY_CMD("report_rate", report_rate),},
	{FACTORY_CMD("stylus_enable", stylus_enable),},
	{FACTORY_CMD("clear_cover_mode", not_support_cmd),},
	{FACTORY_CMD("not_support_cmd", not_support_cmd),},
};

/************************************************************************
 * helpers
 ************************************************************************/
static void set_cmd_result(struct cyttsp5_samsung_factory_data* sfd,
		char *strbuff, int len)
{
	strncat(sfd->factory_cmd_result, strbuff, len);
}

static void set_default_result(struct cyttsp5_samsung_factory_data* sfd)
{
	char delim = ':';

	memset(sfd->factory_cmd_result, 0x00, ARRAY_SIZE(sfd->factory_cmd_result));
	memcpy(sfd->factory_cmd_result, sfd->factory_cmd, strlen(sfd->factory_cmd));
	strncat(sfd->factory_cmd_result, &delim, 1);
}

/************************************************************************
 * commands
 ************************************************************************/
extern int upgrade_firmware_from_sdcard(struct device *dev,
	const u8 *fw_data, int fw_size);

static int fw_update_ums(struct cyttsp5_samsung_factory_data* sfd)
{
	struct file *fp;
	mm_segment_t old_fs;
	int fw_size, nread;
	int error = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(CY_FW_FILE_PATH, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		dev_err(sfd->dev, "%s: failed to open firmware. %s\n",
			__func__, CY_FW_FILE_PATH);
		error = -ENOENT;
		goto open_err;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;

	if (0 < fw_size) {
		unsigned char *fw_data;
		fw_data = kzalloc(fw_size, GFP_KERNEL);
		nread = vfs_read(fp, (char __user *)fw_data,
			fw_size, &fp->f_pos);

		dev_info(sfd->dev, "%s: start, file path %s, size %u Bytes\n",
			__func__, CY_FW_FILE_PATH, fw_size);

		if (nread != fw_size) {
			dev_err(sfd->dev, "%s: failed to read firmware file, nread %u Bytes\n",
				__func__, nread);
			error = -EIO;
		} else {
			error = upgrade_firmware_from_sdcard(sfd->dev,
				(const u8 *)fw_data, fw_size);
		}

		if (error < 0)
			dev_err(sfd->dev, "%s: failed update firmware\n", __func__);

		kfree(fw_data);
	}

	filp_close(fp, current->files);

 open_err:
	set_fs(old_fs);
	return error;
}

static void fw_update(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	cyttsp5_upgrade_firmware_from_platform upgrade_firmware_from_platform;
	char strbuff[16] = {0};
	int rc = 0;

	set_default_result(sfd);

	if (sfd->factory_cmd_param[0] == 0) {
		upgrade_firmware_from_platform =
			_cyttsp5_request_upgrade_firmware_from_platform(sfd->dev);
		if (upgrade_firmware_from_platform)
			rc = upgrade_firmware_from_platform(sfd->dev, 1);
		else
			rc = -1;
	} else if (sfd->factory_cmd_param[0] == 1) {
		rc = fw_update_ums(sfd);
	} else {
		dev_err(sfd->dev, "%s: parameter %d is wrong\n",
			__func__, sfd->factory_cmd_param[0]);
		rc = -1;
	}

	if(rc == 0) {
		snprintf(strbuff, sizeof(strbuff), "%s", "OK");
		sfd->factory_cmd_state = FACTORYCMD_OK;
	} else {
		dev_err(sfd->dev, "%s: rc=%d\n", __func__, rc);

		snprintf(strbuff, sizeof(strbuff), "%s", "NG");
		sfd->factory_cmd_state = FACTORYCMD_FAIL;
	}

	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));

	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
			strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_fw_ver_bin(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	struct cyttsp5_platform_data *pdata = dev_get_platdata(sfd->dev);
	struct cyttsp5_touch_firmware *fw = pdata->loader_pdata->fw;
		char strbuff[16] = {0};

	set_default_result(sfd);

	if (fw) {
		snprintf(strbuff, sizeof(strbuff), "CY%02x%04x",
			fw->hw_version, fw->fw_version);
		sfd->factory_cmd_state = FACTORYCMD_OK;
	} else {
		snprintf(strbuff, sizeof(strbuff), "%s", "NG");
		sfd->factory_cmd_state = FACTORYCMD_FAIL;
	}

	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));

	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_fw_ver_ic(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	struct cyttsp5_samsung_tsp_info_dev* sti =
		cyttsp5_get_samsung_tsp_info(sfd->dev);
	char strbuff[16] = {0};

	set_default_result(sfd);
	snprintf(strbuff, sizeof(strbuff), "CY%02x%02x%02x",
		sti->hw_version, sti->fw_versionh, sti->fw_versionl);
	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	sfd->factory_cmd_state = FACTORYCMD_OK;
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_config_ver(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	struct cyttsp5_samsung_tsp_info_dev* sti =
		cyttsp5_get_samsung_tsp_info(sfd->dev);
	char strbuff[16] = {0};

	set_default_result(sfd);
#ifdef SAMSUNG_TSP_INFO
	snprintf(strbuff, sizeof(strbuff), "%u", sti->config_version);

	sfd->factory_cmd_state = FACTORYCMD_OK;
#else
	sprintf(strbuff, "%s", "NG");

	sfd->factory_cmd_state = FACTORYCMD_FAIL;
#endif
	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));

	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_threshold(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	struct cyttsp5_samsung_tsp_info_dev* sti =
		cyttsp5_get_samsung_tsp_info(sfd->dev);
	char strbuff[16] = {0};
	int rc = 0;
#if 0
	u8 parm_id = 0x1a;
	u32 test;
#endif
	set_default_result(sfd);

	snprintf(strbuff, sizeof(strbuff), "%u", get_unaligned_be16(&sti->thresholdh));
	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	if (rc < 0)
		sfd->factory_cmd_state = FACTORYCMD_FAIL;
	else
		sfd->factory_cmd_state = FACTORYCMD_OK;
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void module_off_master(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	char strbuff[3] = {0};

	set_default_result(sfd);
#if 0
	_cyttsp5_unsubscribe_attention(sfd->dev, CY_ATTEN_IRQ, CY_MODULE_MT,
		cyttsp5_mt_attention, CY_MODE_OPERATIONAL);

	_cyttsp5_unsubscribe_attention(sfd->dev, CY_ATTEN_STARTUP, CY_MODULE_MT,
		cyttsp5_startup_attention, 0);
#endif
	cyttsp5_core_suspend(sfd->dev);

	snprintf(strbuff, sizeof(strbuff), "%s", "OK");
	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));
		sfd->factory_cmd_state = FACTORYCMD_OK;
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void module_on_master(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	char strbuff[3] = {0};

	set_default_result(sfd);
#if 0
	_cyttsp5_subscribe_attention(sfd->dev, CY_ATTEN_IRQ, CY_MODULE_MT,
		cyttsp5_mt_attention, CY_MODE_OPERATIONAL);

	_cyttsp5_subscribe_attention(sfd->dev, CY_ATTEN_STARTUP, CY_MODULE_MT,
		cyttsp5_startup_attention, 0);

	_cyttsp5_subscribe_attention(sfd->dev, CY_ATTEN_WAKE, CY_MODULE_MT,
		cyttsp5_mt_wake_attention, 0);
#endif
	cyttsp5_core_resume(sfd->dev);

	snprintf(strbuff, sizeof(strbuff), "%s", "OK");
	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));
		sfd->factory_cmd_state = FACTORYCMD_OK;
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_chip_vendor(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	char strbuff[16] = {0};

	set_default_result(sfd);
	snprintf(strbuff, sizeof(strbuff), "%s", "Cypress");
	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	sfd->factory_cmd_state = FACTORYCMD_OK;
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_chip_name(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	char strbuff[16] = {0};

	set_default_result(sfd);
	snprintf(strbuff, sizeof(strbuff), "%s", "CYTMA545");
	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	sfd->factory_cmd_state = FACTORYCMD_OK;
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_x_num(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	char strbuff[16] = {0};

	set_default_result(sfd);

	sprintf(strbuff, "%u", sfd->si->sensing_conf_data.electrodes_x);
	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));

	sfd->factory_cmd_state = FACTORYCMD_OK;
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_y_num(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	char strbuff[16] = {0};

	set_default_result(sfd);

	sprintf(strbuff, "%u", sfd->si->sensing_conf_data.electrodes_y);

	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	sfd->factory_cmd_state = FACTORYCMD_OK;
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static inline s16 node_value_s16(u8* buf, u16 node)
{
	return (s16)get_unaligned_le16(buf + node * 2);
}

static void get_raw_diff(struct cyttsp5_samsung_factory_data* sfd,
	struct cyttsp5_sfd_panel_scan_data *panel_scan_data)
{
	struct cyttsp5_samsung_tsp_info_dev* sti =
		cyttsp5_get_samsung_tsp_info(sfd->dev);
	s16 value = 0;
	char strbuff[16] = {0};

	set_default_result(sfd);

	if (sfd->factory_cmd_param[0] < 0 ||
		sfd->factory_cmd_param[0] >= sfd->si->sensing_conf_data.electrodes_x ||
		sfd->factory_cmd_param[1] < 0 ||
		sfd->factory_cmd_param[1] >= sfd->si->sensing_conf_data.electrodes_y) {
		dev_err(sfd->dev,
			"%s: parameter wrong param[0]=%d param[1]=%d\n", __func__,
			sfd->factory_cmd_param[0], sfd->factory_cmd_param[1]);

		sprintf(strbuff, "%s", "NG");
		set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));

		sfd->factory_cmd_state = FACTORYCMD_FAIL;
	} else {
		u16 node =
			(sti->rx_nodes == sfd->si->sensing_conf_data.electrodes_x) ? /*x is rx*/
			sfd->factory_cmd_param[0] +
			sfd->si->sensing_conf_data.electrodes_x *
			sfd->factory_cmd_param[1]
			:
			sfd->factory_cmd_param[1] +
			sfd->si->sensing_conf_data.electrodes_y *
			sfd->factory_cmd_param[0];

		if (panel_scan_data->element_size == 2)
			value = node_value_s16(panel_scan_data->buf + CY_CMD_RET_PANEL_HDR,
				node);
		else
			value = panel_scan_data->buf[CY_CMD_RET_PANEL_HDR + node];

		snprintf(strbuff, sizeof(strbuff), "%d", value);
		set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));

		sfd->factory_cmd_state = FACTORYCMD_OK;

		dev_info(sfd->dev, "%s: node [%d,%d] = %d\n", __func__,
			sfd->factory_cmd_param[0], sfd->factory_cmd_param[1], value);
	}
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_raw_count(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;

	get_raw_diff(sfd, &sfd->raw);
}

static void get_difference(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;

	get_raw_diff(sfd, &sfd->diff);
}

static void find_max_min_s16(u8* buf, int num_nodes, s16 *max_value, s16 *min_value)
{
	int i;
	*max_value = 0x8000;
	*min_value = 0x7FFF;

	for (i = 0 ; i < num_nodes ; i++) {
		*max_value = max((s16)*max_value, (s16)get_unaligned_le16(buf));
		*min_value = min((s16)*min_value, (s16)get_unaligned_le16(buf));
		buf += 2;
	}
}
static void find_max_min_s8(u8* buf, int num_nodes, s16 *max_value, s16 *min_value)
{
	int i;
	*max_value = 0x8000;
	*min_value = 0x7FFF;

	for (i = 0 ; i < num_nodes ; i++) {
		*max_value = max((s8)*max_value, (s8)(*buf));
		*min_value = min((s8)*min_value, (s8)(*buf));
		buf += 1;
	}
}

static int retrieve_panel_scan(struct cyttsp5_samsung_factory_data* sfd,
		u8* buf, u8 data_id, int num_nodes, u8* r_element_size/*in bytes*/)
{
	int rc = 0;
	int elem = num_nodes;
	int elem_offset = 0;
	u16 actual_read_len;
	u8 config;
	u16 length;
	u8 *buf_offset;
	u8 element_size = 0;

	/* fill buf with header and data */
	rc = sfd->corecmd->cmd->retrieve_panel_scan(sfd->dev, 0, elem_offset, elem,
		data_id, buf, &config, &actual_read_len, NULL);
	if (rc < 0)
		goto end;

	length = get_unaligned_le16(buf);
	buf_offset = buf + length;

	element_size = config & CY_CMD_RET_PANEL_ELMNT_SZ_MASK;
	*r_element_size = element_size;

	elem -= actual_read_len;
	elem_offset = actual_read_len;
	while (elem > 0) {
		/* append data to the buf */
		rc = sfd->corecmd->cmd->retrieve_panel_scan(sfd->dev, 0, elem_offset, elem,
				data_id, NULL, &config, &actual_read_len, buf_offset);
		if (rc < 0)
			goto end;

		if (!actual_read_len)
			break;

		length += actual_read_len * element_size;
		buf_offset = buf + length;
		elem -= actual_read_len;
		elem_offset += actual_read_len;
	}
end:
	return rc;
}

static int panel_scan_and_retrieve(struct cyttsp5_samsung_factory_data* sfd,
		u8 data_id, struct cyttsp5_sfd_panel_scan_data *panel_scan_data)
{
	struct device *dev = sfd->dev;
	int node_count;
	int buf_offset;
	int rc = 0;

	dev_dbg(sfd->dev, "%s\n", __func__);

	rc = cyttsp5_request_exclusive(dev, CY_REQUEST_EXCLUSIVE_TIMEOUT);
	if (rc < 0) {
		dev_err(dev, "%s: request exclusive failed(%d)\n",
			__func__, rc);
		return rc;
	}

	rc = sfd->corecmd->cmd->suspend_scanning(dev, 0);
	if (rc < 0) {
		dev_err(dev, "%s: suspend scanning failed r=%d\n",
			__func__, rc);
		goto release_exclusive;
	}

	rc = sfd->corecmd->cmd->exec_panel_scan(dev, 0);
	if (rc < 0) {
		dev_err(dev, "%s: exec panel scan failed r=%d\n",
			__func__, rc);
		goto release_exclusive;
	}

	if (data_id == CY_MUT_RAW || data_id == CY_MUT_DIFF) {
		node_count = sfd->num_all_nodes;
	} else {
		node_count = sfd->si->sensing_conf_data.electrodes_x +
			sfd->si->sensing_conf_data.electrodes_y;
	}
	rc = retrieve_panel_scan(sfd, panel_scan_data->buf, data_id,
		node_count, &panel_scan_data->element_size);
	if (rc < 0) {
		dev_err(dev,
			"%s: retrieve_panel_scan raw count failed r=%d\n",
			__func__, rc);
		goto release_exclusive;
	}

	if (data_id == CY_MUT_RAW || data_id == CY_MUT_DIFF) {
		buf_offset = 0;
	} else {
		buf_offset = sfd->si->sensing_conf_data.electrodes_x *
			panel_scan_data->element_size;
	}

	if (panel_scan_data->element_size == 2)
		find_max_min_s16(panel_scan_data->buf + CY_CMD_RET_PANEL_HDR + buf_offset,
			node_count, &panel_scan_data->max, &panel_scan_data->min);
	else
		find_max_min_s8(panel_scan_data->buf + CY_CMD_RET_PANEL_HDR + buf_offset,
			node_count, &panel_scan_data->max, &panel_scan_data->min);

	rc = sfd->corecmd->cmd->resume_scanning(dev, 0);
	if (rc < 0) {
		dev_err(dev, "%s: resume_scanning failed r=%d\n",
			__func__, rc);
	}

release_exclusive:
	rc = cyttsp5_release_exclusive(dev);
	if (rc < 0)
		dev_err(dev, "%s: release_exclusive failed r=%d\n",
			__func__, rc);

	return rc;
}

int cyttsp5_fw_calibrate(struct device *dev);
static void run_raw_count_read(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	char strbuff[16] = {0};
	int rc;

	set_default_result(sfd);

#ifdef RECAL_FALSELY_CALED_PANEL
	rc = panel_scan_and_retrieve(sfd, CY_SELF_RAW, &sfd->raw);
	if (rc == 0) {
		dev_info(sfd->dev, "%s: self min:%d,max:%d\n", 
			__func__, sfd->raw.min, sfd->raw.max);
		if (sfd->raw.min < SPEC_SELF_RAW_MIN || SPEC_SELF_RAW_MAX < sfd->raw.max) {
			dev_info(sfd->dev, "%s: selfcap wrong\n",
				__func__);
			rc = cyttsp5_fw_calibrate(sfd->dev);
			if (rc < 0)
				dev_err(sfd->dev, "%s: calibration fail, rc=%d\n", 
				__func__, rc);
		}
	}
	rc = panel_scan_and_retrieve(sfd, CY_MUT_RAW, &sfd->raw);
	if (rc == 0) {
		dev_info(sfd->dev, "%s: mutual min:%d,max:%d\n",
			__func__, sfd->raw.min, sfd->raw.max);
		if (sfd->raw.min < SPEC_MUT_RAW_MIN || SPEC_MUT_RAW_MAX < sfd->raw.max) {
			dev_info(sfd->dev, "%s: mutualcap wrong\n", 
				__func__);
			rc = cyttsp5_fw_calibrate(sfd->dev);
			if (rc < 0)
				dev_err(sfd->dev, "%s: calibration fail, rc=%d\n",
				__func__, rc);
			rc = panel_scan_and_retrieve(sfd, CY_MUT_RAW, &sfd->raw);
		}
	}
#else
	rc = panel_scan_and_retrieve(sfd, CY_MUT_RAW, &sfd->raw);
#endif
	if (rc == 0) {
		snprintf(strbuff, sizeof(strbuff), "%d,%d", sfd->raw.min, sfd->raw.max);
		sfd->factory_cmd_state = FACTORYCMD_OK;
	} else {
		sprintf(strbuff, "%s", "NG");
		sfd->factory_cmd_state = FACTORYCMD_FAIL;
	}

#if 0
{
	int i;
	dev_info(sfd->dev, "%s: raw : \n", __func__);
	for (i = 0; i < (sfd->num_all_nodes * 2); i++) {
		dev_info(sfd->dev, "%s: 0x%02x\n",
			__func__, sfd->raw.buf[CY_CMD_RET_PANEL_HDR + i]);
	}
}
#endif

	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void run_difference_read(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	char strbuff[16] = {0};
	int rc;

	set_default_result(sfd);

	rc = panel_scan_and_retrieve(sfd, CY_MUT_DIFF, &sfd->diff);
	if (rc == 0) {
		snprintf(strbuff, sizeof(strbuff), "%d,%d", sfd->diff.min, sfd->diff.max);
		sfd->factory_cmd_state = FACTORYCMD_OK;
	} else {
		sprintf(strbuff, "%s", "NG");
		sfd->factory_cmd_state = FACTORYCMD_FAIL;
	}

	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

/************************************************************************
 * commands - IDAC
 ************************************************************************/
static u16 gidac_node_num(struct cyttsp5_samsung_factory_data* sfd)
{
	struct cyttsp5_samsung_tsp_info_dev* sti =
		cyttsp5_get_samsung_tsp_info(sfd->dev);

	return sti->gidac_nodes;
}
static u16 lidac_node_num(struct cyttsp5_samsung_factory_data* sfd)
{
	struct cyttsp5_samsung_tsp_info_dev* sti =
		cyttsp5_get_samsung_tsp_info(sfd->dev);

	return sti->gidac_nodes * sti->rx_nodes;
}

static void find_max_min_u8(u8* buf, int num_nodes, u8 *max_value, u8 *min_value)
{
	int i;
	*max_value = 0x00;
	*min_value = 0xff;

	for (i = 0 ; i < num_nodes ; i++) {
		*max_value = max((u8)*max_value, (u8)(*buf));
		*min_value = min((u8)*min_value, (u8)(*buf));
		buf += 1;
	}
}

static int retrieve_data_structure(struct cyttsp5_samsung_factory_data* sfd,
		u8 data_id, u8* buf, int num_nodes)
{
	int rc = 0;
	int elem = num_nodes;
	int elem_offset = 0;
	u16 actual_read_len;
	u8 config;
	u16 length;
	u8 *buf_offset;

	/* fill buf with header and data */
	rc = sfd->corecmd->cmd->retrieve_data_structure(sfd->dev, 0, elem_offset, elem,
		data_id, buf, &config, &actual_read_len, NULL);
	if (rc < 0)
		goto end;

	length = get_unaligned_le16(buf);
	buf_offset = buf + length;

	elem -= actual_read_len;
	elem_offset = actual_read_len;
	while (elem > 0) {
		/* append data to the buf */
		rc = sfd->corecmd->cmd->retrieve_data_structure(sfd->dev, 0, elem_offset, elem,
				data_id, NULL, &config, &actual_read_len, buf_offset);
		if (rc < 0)
			goto end;

		if (!actual_read_len)
			break;

		length += actual_read_len;
		buf_offset = buf + length;
		elem -= actual_read_len;
		elem_offset += actual_read_len;
	}
end:
	return rc;
}

static void get_global_idac(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	u8 *buf = sfd->mutual_idac.buf + CY_CMD_RET_PANEL_HDR;
	u8 value = 0;
	char strbuff[16] = {0};

	set_default_result(sfd);

	if (sfd->factory_cmd_param[0] < 0 ||
		sfd->factory_cmd_param[0] >= gidac_node_num(sfd)) {
		dev_err(sfd->dev, "%s: parameter %d is wrong\n",
					__func__, sfd->factory_cmd_param[0]);

		sprintf(strbuff, "%s", "NG");
		set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));

		sfd->factory_cmd_state = FACTORYCMD_FAIL;
	} else {
		value = buf[(u8)sfd->factory_cmd_param[0]];

		snprintf(strbuff, sizeof(strbuff), "%d", value);
		set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));

		sfd->factory_cmd_state = FACTORYCMD_OK;

		dev_info(sfd->dev, "%s: node %d = %d\n",
					__func__, sfd->factory_cmd_param[0], value);
	}
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_local_idac(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	struct cyttsp5_samsung_tsp_info_dev* sti =
		cyttsp5_get_samsung_tsp_info(sfd->dev);
	u8 *buf = sfd->mutual_idac.buf + CY_CMD_RET_PANEL_HDR + gidac_node_num(sfd);
	u8 value = 0;

	char strbuff[16] = {0};

	set_default_result(sfd);

	if (sfd->factory_cmd_param[1] < 0 ||
		sfd->factory_cmd_param[1] >= sti->gidac_nodes ||
		sfd->factory_cmd_param[0] < 0 ||
		sfd->factory_cmd_param[0] >= sti->rx_nodes) {
		dev_err(sfd->dev,
			"%s: parameter wrong param[0]=%d param[1]=%d\n", __func__,
			sfd->factory_cmd_param[0], sfd->factory_cmd_param[1]);

		sprintf(strbuff, "%s", "NG");
		set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));

		sfd->factory_cmd_state = FACTORYCMD_FAIL;
	} else {
		u16 node =
			sfd->factory_cmd_param[0] +
			sti->rx_nodes *
			sfd->factory_cmd_param[1];

		value = buf[node];

		snprintf(strbuff, sizeof(strbuff), "%d", value);
		set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));

		sfd->factory_cmd_state = FACTORYCMD_OK;

		dev_info(sfd->dev, "%s: node [%d,%d] = %d\n", __func__,
			sfd->factory_cmd_param[0], sfd->factory_cmd_param[1], value);
	}
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static int retrieve_mutual_idac(struct cyttsp5_samsung_factory_data* sfd,
		u8 type)
{
	struct device *dev = sfd->dev;
	int rc = 0;

	rc = cyttsp5_request_exclusive(dev, CY_REQUEST_EXCLUSIVE_TIMEOUT);
	if (rc < 0) {
		dev_err(dev, "%s: request exclusive failed(%d)\n",
			__func__, rc);
		return rc;
	}

	rc = sfd->corecmd->cmd->suspend_scanning(dev, 0);
	if (rc < 0) {
		dev_err(dev, "%s: suspend scanning failed r=%d\n",
			__func__, rc);
		goto release_exclusive;
	}

	rc = retrieve_data_structure(sfd, CY_PWC_MUT, sfd->mutual_idac.buf,
			gidac_node_num(sfd)+lidac_node_num(sfd));
	if (rc < 0) {
		dev_err(dev, "%s: retrieve_data_structure failed r=%d\n",
			__func__, rc);
		goto release_exclusive;
	}
	if (type == IDAC_GLOBAL)
		find_max_min_u8(sfd->mutual_idac.buf + CY_CMD_RET_PANEL_HDR,
			gidac_node_num(sfd), &sfd->mutual_idac.gidac_max, &sfd->mutual_idac.gidac_min);
	else
		find_max_min_u8(sfd->mutual_idac.buf + CY_CMD_RET_PANEL_HDR + gidac_node_num(sfd),
			lidac_node_num(sfd), &sfd->mutual_idac.lidac_max, &sfd->mutual_idac.lidac_min);

	rc = sfd->corecmd->cmd->resume_scanning(dev, 0);
	if (rc < 0) {
		dev_err(dev, "%s: resume_scanning failed r=%d\n",
			__func__, rc);
		goto release_exclusive;
	}

release_exclusive:
	rc = cyttsp5_release_exclusive(dev);
	if (rc < 0)
		dev_err(dev, "%s: release_exclusive failed r=%d\n",
			__func__, rc);

	return rc;
}

static void run_idac_read(struct cyttsp5_samsung_factory_data* sfd,
	u8 type)
{
	char strbuff[16] = {0};
	int rc;

	set_default_result(sfd);

	rc = retrieve_mutual_idac(sfd, type);
	if (rc == 0) {
		if (type == IDAC_GLOBAL)
			snprintf(strbuff, sizeof(strbuff), "%d,%d",
				sfd->mutual_idac.gidac_min, sfd->mutual_idac.gidac_max);
		else
			snprintf(strbuff, sizeof(strbuff), "%d,%d",
				sfd->mutual_idac.lidac_min, sfd->mutual_idac.lidac_max);
		sfd->factory_cmd_state = FACTORYCMD_OK;
	} else {
		sprintf(strbuff, "%s", "NG");
		sfd->factory_cmd_state = FACTORYCMD_FAIL;
	}

	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void run_global_idac_read(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;

	run_idac_read(sfd, IDAC_GLOBAL);
}

static void run_local_idac_read(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;

	run_idac_read(sfd, IDAC_LOCAL);
}

/************************************************************************
 * check rc
 ************************************************************************/
static inline void _check_rc(struct cyttsp5_samsung_factory_data* sfd,
	int rc)
{
	char strbuff[16] = {0};
	if (rc == 0) {
		snprintf(strbuff, sizeof(strbuff), "OK");
		sfd->factory_cmd_state = FACTORYCMD_OK;
	} else {
		dev_err(sfd->dev, "%s failed(%d)\n", __func__, rc);
		sprintf(strbuff, "%s", "NG");
		sfd->factory_cmd_state = FACTORYCMD_FAIL;
	}

	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	dev_info(sfd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));

	mutex_lock(&sfd->factory_cmd_lock);
	sfd->factory_cmd_is_running = false;
	mutex_unlock(&sfd->factory_cmd_lock);

	sfd->factory_cmd_state = FACTORYCMD_WAITING;
}

/************************************************************************
 * init baseline
 ************************************************************************/
static int initialize_baselines(struct cyttsp5_samsung_factory_data* sfd)
{
	struct device *dev = sfd->dev;
	int rc = 0, rc1 = 0;

	rc = cyttsp5_request_exclusive(dev, CY_REQUEST_EXCLUSIVE_TIMEOUT);
	if (rc < 0) {
		dev_err(dev, "%s: request exclusive failed(%d)\n",
			__func__, rc);
		return rc;
	}

	rc = sfd->corecmd->cmd->suspend_scanning(sfd->dev, 0);
	if (rc) {
		dev_err(sfd->dev, "%s: error on suspend_scanning\n", __func__);
		goto release_exclusive;
	}

	rc = sfd->corecmd->cmd->initialize_baselines(sfd->dev, 0, 0x07);
	if (rc) {
		dev_err(sfd->dev, "%s: error on initialize_baselines\n", __func__);
		goto release_exclusive;
	}

	rc = sfd->corecmd->cmd->resume_scanning(sfd->dev, 0);
	if (rc) {
		dev_err(sfd->dev, "%s: error on resume_scanning\n", __func__);
		goto release_exclusive;
	}

release_exclusive:
	rc1 = cyttsp5_release_exclusive(dev);
	if (rc1 < 0)
		dev_err(dev, "%s: release_exclusive failed r=%d\n",
			__func__, rc);

	return rc;
}


static void baseline_reset(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	int rc = 0;

	pr_debug("%s: \n", __func__);

	if (!sfd->probe_done) {
		pr_debug("%s: probe is not done\n", __func__);
		return;
	}

	set_default_result(sfd);

	if (sfd->suspended) {
		dev_err(sfd->dev, "%s: device is suspended\n", __func__);
		rc = -EIO;
		goto check_rc;
	}

	rc = initialize_baselines(sfd);
	if (rc) {
		dev_err(sfd->dev, "%s: error on initialize_baselines\n",
			__func__);
		goto check_rc;
	}

check_rc:
	_check_rc(sfd, rc);
}

/************************************************************************
  * commands - hover/glove/stylus
 ************************************************************************/
#define PARAM_ID_TOUCH_MODE	0xD0
#define PARAM_ID_VIEW_COVER_MODE 0xD3
#define SCAN_TYPE_FINGER	(1 << 0)  // means nothing but set it always
#define SCAN_TYPE_GLOVE		(1 << 1)  // high sensitivity mode
#define SCAN_TYPE_STYLUS	(1 << 2)  // means nothing but set it always
#define SCAN_TYPE_HOVER		(1 << 3)
#define SCAN_TYPE_FINGER_PRIORITY (1 << 4)

#define SCAN_TYPE_FINGER_ONLY 1
#define SCAN_TYPE_STYLUS_ONLY 4
#define SCAN_TYPE_FINGER_AND_STYLUS (5 | 16) // 21
#define SCAN_TYPE_FINGER_AND_GLOVE 3
#define SCAN_TYPE_FINGER_AND_HOVER 9
#define SCAN_TYPE_FINGER_GLOVE_AND_STYLUS 7
#define SCAN_TYPE_FINGER_STYLUS_AND_HOVER 13
#define SCAN_TYPE_FULL_AUTO 15

#ifdef SAMSUNG_TOUCH_MODE
extern void cyttsp5_mt_glove_enable(struct device *dev, bool enable);
extern void cyttsp5_mt_prevent_touch(struct device *dev, bool prevent);
extern void cyttsp5_mt_stylus_enable(struct device *dev, bool enable);
#endif
static inline u32 get_touch_mode_value(bool hover, bool glove)
{
	u32 value;

	if (hover && glove)
		value = SCAN_TYPE_FULL_AUTO;
	else if (hover && !glove)
		value = SCAN_TYPE_FINGER_STYLUS_AND_HOVER;
	else if (!hover && glove)
		value = SCAN_TYPE_FINGER_GLOVE_AND_STYLUS;
	else
		value = SCAN_TYPE_FINGER_AND_STYLUS;

	return value;
}

static const char* get_touch_mode_str(u32 touch_mode)
{
	switch (touch_mode) {
	case SCAN_TYPE_FINGER_ONLY:
	    return "F";
	case SCAN_TYPE_STYLUS_ONLY:
	    return "S";
	case SCAN_TYPE_FINGER_AND_STYLUS:
	    return "FS";
	case SCAN_TYPE_FINGER_AND_GLOVE:
	    return "FG";
	case SCAN_TYPE_FINGER_AND_HOVER:
	    return "FH";
	case SCAN_TYPE_FINGER_GLOVE_AND_STYLUS:
	    return "FGS";
	case SCAN_TYPE_FINGER_STYLUS_AND_HOVER:
	    return "FSH";
	case SCAN_TYPE_FULL_AUTO:
	    return "AUTO";
	}
	return "";
}

//-- TOUCH_MODE_SWITCH Commands
#define CMD_HOVER_ENABLE	0
#define CMD_GLOVE_MODE		1
#define CMD_CLEAR_COVER_MODE 2
#define CMD_FAST_GLOVE_MODE  3

static void touch_mode_switch(struct cyttsp5_samsung_factory_data* sfd,
	int command, int param_max)
{
        bool glove, hover;
	int rc = 0;

	if (!sfd->probe_done) {
		pr_debug("%s: probe is not done\n", __func__);
		return;
	}

	set_default_result(sfd);

	if (sfd->factory_cmd_param[0] < 0 ||
	    sfd->factory_cmd_param[0] > param_max) {
		rc = -EINVAL;
		dev_err(sfd->dev, "%s: parameter %d is wrong\n",
			__func__, sfd->factory_cmd_param[0]);
		goto check_rc;
	}

	dev_info(sfd->dev,
		"%s: old touch mode=%s\n", __func__,
		get_touch_mode_str(sfd->touch_mode));

	hover = (sfd->touch_mode & SCAN_TYPE_HOVER) ? 1 : 0;
	glove = (sfd->touch_mode & SCAN_TYPE_GLOVE) ? 1 : 0;
	switch (command) {
	case CMD_HOVER_ENABLE:
		hover = sfd->factory_cmd_param[0];
		break;
	case CMD_GLOVE_MODE:
	case CMD_FAST_GLOVE_MODE:
		glove = sfd->factory_cmd_param[0];
		break;
	case CMD_CLEAR_COVER_MODE:
		if (sfd->factory_cmd_param[0] != 2) {// not flip cover closed
			glove = sfd->factory_cmd_param[0] == 0 ? 0 : 1;
			sfd->view_cover_closed = (sfd->factory_cmd_param[0] == 3);
		}
		break;
	}
        sfd->touch_mode = get_touch_mode_value(hover, glove);

	if (sfd->suspended) {
		dev_err(sfd->dev, "%s: device is suspended\n", __func__);
		goto check_rc;
	}
	rc = cyttsp5_request_exclusive(sfd->dev, CY_REQUEST_EXCLUSIVE_TIMEOUT_SET_PARAM);
	if (rc < 0) {
		dev_err(sfd->dev, "%s: request exclusive failed(%d)\n",
			__func__, rc);
		goto check_rc;
	}	
	if (command == CMD_CLEAR_COVER_MODE &&
		sfd->factory_cmd_param[0] == 2) {// flip cover closed
		cyttsp5_mt_prevent_touch(sfd->dev, 1);
		goto skip_touch_mode_glove_setting;
	}

//-- touch mode
	rc = sfd->corecmd->cmd->set_param(sfd->dev, 0,
		PARAM_ID_TOUCH_MODE, sfd->touch_mode);
	if (rc) {
		dev_err(sfd->dev,
			"%s: error on set_param touch mode\n", __func__);
		goto release_exclusive;
	}
	dev_info(sfd->dev,
			"%s: new touch mode=%s\n", __func__,
			get_touch_mode_str(sfd->touch_mode));
	cyttsp5_mt_glove_enable(sfd->dev, glove);
skip_touch_mode_glove_setting:

//-- view cover mode
	if (command == CMD_CLEAR_COVER_MODE &&
		sfd->factory_cmd_param[0] != 2) { // !flip cover closed
		rc = sfd->corecmd->cmd->set_param(sfd->dev, 0,
			PARAM_ID_VIEW_COVER_MODE,
			sfd->view_cover_closed);
		if (rc) {
			dev_err(sfd->dev,
				"%s: error on set_param view cover mode\n", __func__);
			goto release_exclusive;
		}
		dev_info(sfd->dev, "%s: view_cover_closed=%d\n",
			__func__, sfd->view_cover_closed);
		cyttsp5_mt_prevent_touch(sfd->dev, 0);
	}

release_exclusive:
	if (cyttsp5_release_exclusive(sfd->dev) < 0)
		dev_err(sfd->dev, "%s: release_exclusive failed\n",
			__func__);
check_rc:
	_check_rc(sfd, rc);

}

static void hover_enable(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	touch_mode_switch(sfd, CMD_HOVER_ENABLE, 1);
}

static void glove_mode(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	touch_mode_switch(sfd, CMD_GLOVE_MODE, 1);
}

static void clear_cover_mode(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	touch_mode_switch(sfd, CMD_CLEAR_COVER_MODE, 3);
}

static void fast_glove_mode(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	touch_mode_switch(sfd, CMD_FAST_GLOVE_MODE, 1);

}

static void stylus_enable(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	bool glove, hover;
	u32 touch_mode;
	int rc = 0;

	if (!sfd->probe_done) {
		pr_debug("%s: probe is not done\n", __func__);
		return;
	}

	set_default_result(sfd);

	if ((sfd->factory_cmd_param[0] < 0) ||
		(sfd->factory_cmd_param[0] > 2)) {
		dev_err(sfd->dev, "%s: parameter %d is wrong\n",
			__func__, sfd->factory_cmd_param[0]);

		rc = -EINVAL;
		goto check_rc;
	}

	sfd->stylus_enable = sfd->factory_cmd_param[0] ? true : false;

	rc = cyttsp5_request_exclusive(sfd->dev, CY_REQUEST_EXCLUSIVE_TIMEOUT_SET_PARAM);
	if (rc < 0) {
		dev_err(sfd->dev, "%s: request exclusive failed(%d)\n",
			__func__, rc);
		goto check_rc;
	}

	hover = (sfd->touch_mode & SCAN_TYPE_HOVER) ? 1 : 0;
	glove = (sfd->touch_mode & SCAN_TYPE_GLOVE) ? 1 : 0;
	// glove off -> stylus on -> stylus off : glove needs to be off.
	// glove on -> stylus on -> stylus off : glove needs to be on.
	if (!glove) {
		touch_mode = get_touch_mode_value(hover,
			sfd->stylus_enable);

		rc = sfd->corecmd->cmd->set_param(sfd->dev, 0,
			PARAM_ID_TOUCH_MODE, touch_mode);
		if (rc)
			dev_err(sfd->dev,
				"%s: error on set_param touch mode\n", __func__);
		else
			dev_info(sfd->dev,
				"%s: new touch mode=%s\n", __func__,
				get_touch_mode_str(touch_mode));
	}

	cyttsp5_mt_stylus_enable(sfd->dev, sfd->stylus_enable);

	if (cyttsp5_release_exclusive(sfd->dev) < 0)
		dev_err(sfd->dev, "%s: release_exclusive failed\n",
			__func__);
check_rc:
	_check_rc(sfd, rc);
}

/************************************************************************
 * report_rate
 ************************************************************************/
#define PARAM_ID_ACT_INTRVL0 0x4D
#define PARAM_ID_LP_INTRVL0	 0x4C
static int report_rate_set_param(struct cyttsp5_samsung_factory_data* sfd)
{
	u32 interval;
	int rc;

	switch (sfd->report_rate) {
	case 1:
		interval = 17/*ms, 60Hz*/;
		break;
	case 2:
		interval = 33/*ms, 30Hz*/;
		break;
	default:
		interval = 10/*ms, 100Hz*/;
		break;
	}

	dev_dbg(sfd->dev, "%s: interval=%d\n", __func__,
		interval);
	rc = sfd->corecmd->cmd->set_param(sfd->dev, 0,
		PARAM_ID_ACT_INTRVL0, interval);
	if (rc)
		dev_err(sfd->dev, "%s: error on set_param\n", __func__);

	return rc;
}

static void report_rate(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	int rc = 0;

	if (!sfd->probe_done) {
		pr_debug("%s: probe is not done\n", __func__);
		return;
	}

	set_default_result(sfd);

	if ((sfd->factory_cmd_param[0] < 0) ||
		(sfd->factory_cmd_param[0] > 2)) {
		dev_err(sfd->dev, "%s: parameter %d is wrong\n",
			__func__, sfd->factory_cmd_param[0]);

		rc = -EINVAL;
		goto check_rc;
	}
	sfd->report_rate = sfd->factory_cmd_param[0];

	if (sfd->suspended) {
		dev_info(sfd->dev, "%s: device is suspended\n", __func__);
		goto check_rc;
	}
	rc = cyttsp5_request_exclusive(sfd->dev, CY_REQUEST_EXCLUSIVE_TIMEOUT_SET_PARAM);
	if (rc < 0) {
		dev_info(sfd->dev, "%s: request exclusive failed(%d)\n",
			__func__, rc);
		goto check_rc;
	}

	rc = report_rate_set_param(sfd);

	if (cyttsp5_release_exclusive(sfd->dev) < 0)
		dev_info(sfd->dev, "%s: release_exclusive failed\n",
			__func__);
check_rc:
	_check_rc(sfd, rc);
}


/************************************************************************
 * commands -
 ************************************************************************/
static void not_support_cmd(void *device_data)
{
	struct cyttsp5_samsung_factory_data* sfd =
		(struct cyttsp5_samsung_factory_data *) device_data;
	char strbuff[16] = {0};

	set_default_result(sfd);
	sprintf(strbuff, "%s", "NA");
	set_cmd_result(sfd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	sfd->factory_cmd_state = FACTORYCMD_NOT_APPLICABLE;
	dev_info(sfd->dev, "%s: \"%s(%d)\"\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));

	/* Some cmds are supported in specific IC and they are clear the cmd_is running flag
	 * itself(without show_cmd_result_) in their function such as hover_enable, glove_mode.
	 * So we need to clear cmd_is runnint flag if that command is replaced with
	 * not_support_cmd */
	mutex_lock(&sfd->factory_cmd_lock);
	sfd->factory_cmd_is_running = false;
	mutex_unlock(&sfd->factory_cmd_lock);

	return;
}

static ssize_t store_cmd(struct device *dev, struct device_attribute
		*devattr, const char *buf, size_t count)
{
	struct cyttsp5_samsung_factory_data *sfd = dev_get_drvdata(dev);
	struct factory_cmd *factory_cmd_ptr = NULL;
	int param_cnt = 0;
	int ret, len, i;
	char *cur, *start, *end;
	char strbuff[FACTORY_CMD_STR_LEN] = {0};
	char delim = ',';
	bool cmd_found = false;

	if (strlen(buf) >= FACTORY_CMD_STR_LEN) {
		dev_err(sfd->dev, "%s: cmd length is over(%s,%d)!!\n", __func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	if (sfd->factory_cmd_is_running == true) {
		dev_err(sfd->dev, "factory_cmd: other cmd is running.\n");
		goto err_out;
	}

	/* check lock  */
	mutex_lock(&sfd->factory_cmd_lock);
	sfd->factory_cmd_is_running = true;
	mutex_unlock(&sfd->factory_cmd_lock);

	sfd->factory_cmd_state = FACTORYCMD_RUNNING;

	for (i = 0; i < ARRAY_SIZE(sfd->factory_cmd_param); i++)
		sfd->factory_cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;
	memset(sfd->factory_cmd, 0x00, ARRAY_SIZE(sfd->factory_cmd));
	memcpy(sfd->factory_cmd, buf, len);

	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(strbuff, buf, cur - buf);
	else
		memcpy(strbuff, buf, len);

	/* find command */
	list_for_each_entry(factory_cmd_ptr,
			&sfd->factory_cmd_list_head, list) {
		if (!strcmp(strbuff, factory_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(factory_cmd_ptr,
				&sfd->factory_cmd_list_head, list) {
			if (!strcmp("not_support_cmd", factory_cmd_ptr->cmd_name))
				break;
		}
	}

	/* parsing parameters */
	if (cur && cmd_found) {
		cur++;
		start = cur;
		memset(strbuff, 0x00, ARRAY_SIZE(strbuff));
		do {
			if (*cur == delim || cur - buf == len) {
				end = cur;
				memcpy(strbuff, start, end - start);
				*(strbuff + strlen(strbuff)) = '\0';
				ret = kstrtoint(strbuff, 10,\
						sfd->factory_cmd_param + param_cnt);
				start = cur + 1;
				memset(strbuff, 0x00, ARRAY_SIZE(strbuff));
				param_cnt++;
			}
			cur++;
		} while ((cur - buf <= len) && (param_cnt < FACTORY_CMD_PARAM_NUM));
	}

	dev_info(sfd->dev, "cmd = %s\n", factory_cmd_ptr->cmd_name);
	for (i = 0; i < param_cnt; i++)
		dev_info(sfd->dev, "cmd param %d= %d\n", i,
			sfd->factory_cmd_param[i]);

	factory_cmd_ptr->cmd_func(sfd);

err_out:
	return count;
}

static ssize_t show_cmd_status(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct cyttsp5_samsung_factory_data *sfd = dev_get_drvdata(dev);
	char strbuff[16] = {0};

	dev_info(sfd->dev, "tsp cmd: status:%d, PAGE_SIZE=%ld\n",
			sfd->factory_cmd_state, PAGE_SIZE);

	if (sfd->factory_cmd_state == FACTORYCMD_WAITING)
		snprintf(strbuff, sizeof(strbuff), "WAITING");

	else if (sfd->factory_cmd_state == FACTORYCMD_RUNNING)
		snprintf(strbuff, sizeof(strbuff), "RUNNING");

	else if (sfd->factory_cmd_state == FACTORYCMD_OK)
		snprintf(strbuff, sizeof(strbuff), "OK");

	else if (sfd->factory_cmd_state == FACTORYCMD_FAIL)
		snprintf(strbuff, sizeof(strbuff), "FAIL");

	else if (sfd->factory_cmd_state == FACTORYCMD_NOT_APPLICABLE)
		snprintf(strbuff, sizeof(strbuff), "NOT_APPLICABLE");

	return snprintf(buf, PAGE_SIZE, "%s\n", strbuff);
}

static ssize_t show_cmd_result(struct device *dev, struct device_attribute
		*devattr, char *buf)
{
	struct cyttsp5_samsung_factory_data *sfd = dev_get_drvdata(dev);

	dev_info(sfd->dev, "tsp cmd: result: %s\n", sfd->factory_cmd_result);

	mutex_lock(&sfd->factory_cmd_lock);
	sfd->factory_cmd_is_running = false;
	mutex_unlock(&sfd->factory_cmd_lock);

	sfd->factory_cmd_state = FACTORYCMD_WAITING;

	return snprintf(buf, PAGE_SIZE, "%s\n", sfd->factory_cmd_result);
}

static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, store_cmd);
static DEVICE_ATTR(cmd_status, S_IRUGO, show_cmd_status, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, show_cmd_result, NULL);

static struct attribute *sec_touch_factory_attributes[] = {
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
	NULL,
};

static struct attribute_group sec_touch_factory_attr_group = {
	.attrs = sec_touch_factory_attributes,
};

/************************************************************************
 * suspend/resume
 ************************************************************************/
/************************************************************************
 * This function called with startup() finished startup procedure
 * and having exclusive_access
 ************************************************************************/
#define PARAM_ID_TOUCH_MODE_DEFAULT SCAN_TYPE_FINGER_AND_STYLUS
#define STYLUS_ENABLE_DEFAULT 0
#define PARAM_ID_VIEW_COVER_MODE_DEFAULT 0
#define PARAM_ID_REPORT_RATE_DEFAULT 0
#define PARAM_ID_SEPERATE_VELOCITY_MODE_DEFAULT 0

int cyttsp5_samsung_factory_startup_attention(struct device *dev)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp5_samsung_factory_data *sfd = &cd->sfd;
	u32 touch_mode;
	bool hover, glove;
	int rc;

	if (!sfd->probe_done) {
		pr_debug("%s: probe is not done\n", __func__);
		return 0;
	}

	dev_dbg(dev, "%s: \n", __func__);

//-- glove enable
	cyttsp5_mt_glove_enable(sfd->dev,
		(sfd->touch_mode & SCAN_TYPE_GLOVE) ? 1 : 0);

//-- stylus enable
	if (sfd->stylus_enable == STYLUS_ENABLE_DEFAULT)
		dev_dbg(sfd->dev,
			"%s: stylus_enable %x is default value, bypass setting the mode\n",
			__func__, sfd->stylus_enable);
	else
		cyttsp5_mt_stylus_enable(sfd->dev, sfd->stylus_enable);

//-- touch mode
	hover = (sfd->touch_mode & SCAN_TYPE_HOVER) ? 1 : 0;
	glove = (sfd->touch_mode & SCAN_TYPE_GLOVE) ? 1 : 0;
	if (sfd->stylus_enable)
		glove = 1;
	touch_mode = get_touch_mode_value(hover, glove);
	if (touch_mode == PARAM_ID_TOUCH_MODE_DEFAULT)
		dev_dbg(sfd->dev,
			"%s: touch mode %s is default value, bypass set param\n",
			__func__, get_touch_mode_str(touch_mode));
	else {
		rc = sfd->corecmd->cmd->set_param(sfd->dev, 0,
			PARAM_ID_TOUCH_MODE, touch_mode);
		if (rc) {
			dev_err(sfd->dev, "%s: error on set touch mode rc=%d\n",
				__func__, rc);
			goto exit;
		}
		dev_dbg(sfd->dev, "%s: touch_mode=%s\n", __func__,
			get_touch_mode_str(touch_mode));
	}

//-- view cover mode
	if (sfd->view_cover_closed == PARAM_ID_VIEW_COVER_MODE_DEFAULT)
		dev_dbg(sfd->dev,
			"%s: view cover mode %x is default value, bypass set param\n",
			__func__, sfd->view_cover_closed);
	else {
		rc = sfd->corecmd->cmd->set_param(sfd->dev, 0,
			PARAM_ID_VIEW_COVER_MODE,
			sfd->view_cover_closed);
		if (rc) {
			dev_err(sfd->dev,
				"%s: error on set_param view cover mode\n", __func__);
			goto exit;
		}
		dev_info(sfd->dev, "%s: view_cover_closed=%d\n",
			__func__, sfd->view_cover_closed);
	}

//-- report rate
	if (sfd->report_rate == PARAM_ID_REPORT_RATE_DEFAULT)
		dev_dbg(sfd->dev,
			"%s: report_rate %x is default value, bypass set param\n",
			__func__, sfd->report_rate);
	else {
		rc = report_rate_set_param(sfd);
		if (rc) {
			dev_err(sfd->dev, "%s: error on set report rate rc=%d\n",
				__func__, rc);
		}
		dev_info(sfd->dev, "%s: report rate=%d\n",
			__func__, sfd->report_rate);
	}

exit:
	sfd->suspended = 0;
	return rc;

}

/************************************************************************
 * This function is called with sleep() having exclusive access
 ************************************************************************/
int cyttsp5_samsung_factory_suspend_attention(struct device *dev)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp5_samsung_factory_data *sfd = &cd->sfd;

	if (!sfd->probe_done) {
		pr_debug("%s: probe is not done\n", __func__);
		return 0;
	}

	dev_dbg(dev, "%s: \n", __func__);

	sfd->suspended = 1;
	return 0;
}


/************************************************************************
 * init
 ************************************************************************/
int cyttsp5_samsung_factory_probe(struct device *dev)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp5_samsung_factory_data *sfd = &cd->sfd;
	int rc = 0;
	int i;

	sfd->dev = dev;

	sfd->corecmd = cyttsp5_get_commands();
	if (!sfd->corecmd) {
		dev_err(dev, "%s: core cmd not available\n", __func__);
		rc = -EINVAL;
		goto error_return;
	}

	sfd->si = _cyttsp5_request_sysinfo(dev);
	if (!sfd->si) {
		dev_err(dev, "%s: Fail get sysinfo pointer from core\n", __func__);
		rc = -EINVAL;
		goto error_return;
	}

	dev_dbg(dev, "%s: electrodes_x=%d\n", __func__,
		sfd->si->sensing_conf_data.electrodes_x);
	dev_dbg(dev, "%s: electrodes_y=%d\n", __func__,
		sfd->si->sensing_conf_data.electrodes_y);

	sfd->num_all_nodes = sfd->si->sensing_conf_data.electrodes_x *
		sfd->si->sensing_conf_data.electrodes_y;
	if (sfd->num_all_nodes > MAX_NODE_NUM) {
		dev_err(dev, "%s: sensor node num(%d) exceeds limits\n", __func__,
			sfd->num_all_nodes);
		rc = -EINVAL;
		goto error_return;
	}

	sfd->raw.buf = kzalloc((MAX_INPUT_HEADER_SIZE +
		sfd->num_all_nodes * 2), GFP_KERNEL);
	if (sfd->raw.buf == NULL) {
		dev_err(dev, "%s: Error, kzalloc sfd->raw.buf\n", __func__);
		rc = -ENOMEM;
		goto error_return;
	}

	sfd->diff.buf = kzalloc((MAX_INPUT_HEADER_SIZE +
		sfd->num_all_nodes * 2), GFP_KERNEL);
	if (sfd->diff.buf == NULL) {
		dev_err(dev, "%s: Error, kzalloc sfd->diff.buf\n", __func__);
		rc = -ENOMEM;
		goto error_alloc_difference_buf;
	}

	sfd->mutual_idac.buf = kzalloc((MAX_INPUT_HEADER_SIZE +
		gidac_node_num(sfd) + lidac_node_num(sfd)), GFP_KERNEL);
	if (sfd->mutual_idac.buf == NULL) {
		dev_err(dev, "%s: Error, kzalloc sfd->mutual_idac.buf\n", __func__);
		rc = -ENOMEM;
		goto error_alloc_idac_buf;
	}

	INIT_LIST_HEAD(&sfd->factory_cmd_list_head);
	for (i = 0; i < ARRAY_SIZE(factory_cmds); i++)
		list_add_tail(&factory_cmds[i].list, &sfd->factory_cmd_list_head);

	mutex_init(&sfd->factory_cmd_lock);
	sfd->factory_cmd_is_running = false;

	sfd->factory_dev = device_create(sec_class, NULL,
		MKDEV(SEC_DEV_TOUCH_MAJOR, SEC_DEV_TSP_MINOR), sfd, "tsp");
	if (IS_ERR(sfd->factory_dev)) {
		dev_err(sfd->dev, "Failed to create device for the sysfs\n");
		goto error_device_create_factory_dev;
	}
	dev_dbg(dev, "%s sfd->factory_dev->devt=%d\n",
		__func__, sfd->factory_dev->devt);

	rc = sysfs_create_link(&sfd->factory_dev->kobj,
		&cd->md.input->dev.kobj, "input");
	if (rc < 0) {
		dev_err(sfd->dev,
			"%s: Failed to create input symbolic link\n",
			__func__);
	}

	rc = sysfs_create_group(&sfd->factory_dev->kobj,
		&sec_touch_factory_attr_group);
	if (rc) {
		dev_err(sfd->dev, "Failed to create sysfs group\n");
		goto error_sysfs_create_group;
	}

	sfd->sysfs_nodes_created = true;

	rc = sfd->corecmd->cmd->get_param(sfd->dev, 1,
		PARAM_ID_TOUCH_MODE, &sfd->touch_mode);
	if (rc) {
		dev_err(dev, "%s: get_param failed r=%d\n",
			__func__, rc);
		sfd->touch_mode = PARAM_ID_TOUCH_MODE_DEFAULT;
	}
	dev_dbg(sfd->dev, "%s: touch_mode=%s\n", __func__,
		get_touch_mode_str(sfd->touch_mode));

	sfd->stylus_enable = STYLUS_ENABLE_DEFAULT;
	sfd->view_cover_closed = PARAM_ID_VIEW_COVER_MODE_DEFAULT;
	sfd->report_rate = PARAM_ID_REPORT_RATE_DEFAULT;
	sfd->is_inputmethod = PARAM_ID_SEPERATE_VELOCITY_MODE_DEFAULT;

	sfd->probe_done = 1;
	dev_dbg(sfd->dev, "%s: success. rc=%d\n", __func__, rc);
	return 0;

error_sysfs_create_group:
	device_destroy(sec_class, sfd->factory_dev->devt);
error_device_create_factory_dev:
	kfree(sfd->mutual_idac.buf);
error_alloc_idac_buf:
	kfree(sfd->diff.buf);
error_alloc_difference_buf:
	kfree(sfd->raw.buf);
error_return:
	dev_err(dev, "%s failed. rc=%d\n", __func__, rc);
	return rc;
}

int cyttsp5_samsung_factory_release(struct device *dev)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp5_samsung_factory_data *sfd = &cd->sfd;

	if (sfd->sysfs_nodes_created) {
		sysfs_remove_group(&sfd->factory_dev->kobj,
			&sec_touch_factory_attr_group);
		dev_dbg(dev, "%s sfd->factory_dev->devt=%d\n",
			__func__, sfd->factory_dev->devt);
		device_destroy(sec_class, sfd->factory_dev->devt);

		kfree(sfd->mutual_idac.buf);
		kfree(sfd->diff.buf);
		kfree(sfd->raw.buf);
		sfd->sysfs_nodes_created = false;
	}
	dev_dbg(dev, "%s\n",__func__);

	return 0;
}

