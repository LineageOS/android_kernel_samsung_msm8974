/*
 * cyttsp4_samsung_sysfs.c
 *
 */

#include <linux/slab.h>
#include <linux/err.h>

#include "cyttsp4_regs.h"


/************************************************************************
 * Macros, Structures
 ************************************************************************/
#define SEC_DEV_TOUCH_MAJOR 		0
#define SEC_DEV_TSP_MINOR 			1
#define SEC_DEV_TOUCHSCREEN_MINOR 	2
#define SEC_DEV_TOUCHKEY_MINOR 		3

#define MAX_NODE_NUM 900 /* 30 * 30 */
#define MAX_INPUT_HEADER_SIZE 12
#define MAX_GIDAC_NODES 32
#define MAX_LIDAC_NODES (MAX_GIDAC_NODES * 30)
#define MAX_BTN_NUM 4

extern struct class *sec_class;
extern const char *model_name;
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
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void get_x_num(void *device_data);
static void get_y_num(void *device_data);
static void run_raw_count_read(void *device_data);
static void get_raw_count(void *device_data);
static void get_raw_count_btn(void *device_data);
static void run_difference_read(void *device_data);
static void get_difference(void *device_data);
static void get_difference_btn(void *device_data);
static void run_idac_read(void *device_data);
static void get_global_idac(void *device_data);
static void get_local_idac(void *device_data);
static void get_global_idac_btn(void *device_data);
static void get_local_idac_btn(void *device_data);
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
	{FACTORY_CMD("get_chip_vendor", get_chip_vendor),},
	{FACTORY_CMD("get_chip_name", get_chip_name),},
	{FACTORY_CMD("get_x_num", get_x_num),},
	{FACTORY_CMD("get_y_num", get_y_num),},
	{FACTORY_CMD("run_raw_count_read", run_raw_count_read),},
	{FACTORY_CMD("get_raw_count", get_raw_count),},
	{FACTORY_CMD("get_raw_count_btn", get_raw_count_btn),},
	{FACTORY_CMD("run_difference_read", run_difference_read),},
	{FACTORY_CMD("get_difference", get_difference),},
	{FACTORY_CMD("get_difference_btn", get_difference_btn),},
	{FACTORY_CMD("run_local_idac_read", run_idac_read),},
	{FACTORY_CMD("get_global_idac", get_global_idac),},
	{FACTORY_CMD("get_local_idac", get_local_idac),},
	{FACTORY_CMD("get_global_idac_btn", get_global_idac_btn),},
	{FACTORY_CMD("get_local_idac_btn", get_local_idac_btn),},
	{FACTORY_CMD("not_support_cmd", not_support_cmd),},
};

/************************************************************************
 * helpers
 ************************************************************************/
static void set_cmd_result(struct cyttsp4_samsung_sysfs_data* ssd,
		char *strbuff, int len)
{
	strncat(ssd->factory_cmd_result, strbuff, len);
}

static void set_default_result(struct cyttsp4_samsung_sysfs_data* ssd)
{
	char delim = ':';

	memset(ssd->factory_cmd_result, 0x00, ARRAY_SIZE(ssd->factory_cmd_result));
	memcpy(ssd->factory_cmd_result, ssd->factory_cmd, strlen(ssd->factory_cmd));
	strncat(ssd->factory_cmd_result, &delim, 1);
}

/************************************************************************
 * commands
 ************************************************************************/
static void fw_update(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	char strbuff[16] = {0};
	int rc = 0;

	set_default_result(ssd);

	if (ssd->factory_cmd_param[0] == 0)
		rc = upgrade_firmware_from_platform(ssd->dev, false);
	else
	if (ssd->factory_cmd_param[0] == 1)
		rc = upgrade_firmware_from_sdcard(ssd->dev);
	else
		rc = -EINVAL;

	if(rc == 0) {
		snprintf(strbuff, sizeof(strbuff), "%s", "OK");
		ssd->factory_cmd_state = FACTORYCMD_OK;
	} else {
		dev_err(ssd->dev, "%s: rc=%d\n", __func__, rc);

		snprintf(strbuff, sizeof(strbuff), "%s", "NG");
		ssd->factory_cmd_state = FACTORYCMD_FAIL;
	}

	set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));

	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
			strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_fw_ver_bin(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	struct cyttsp4_platform_data *pdata = dev_get_platdata(ssd->dev);
	struct cyttsp4_touch_firmware *fw = pdata->loader_pdata->fw;
	char strbuff[16] = {0};

	set_default_result(ssd);

	if (fw) {
		snprintf(strbuff, sizeof(strbuff), "CY%02x%04x",
			fw->hw_version, fw->fw_version);
		ssd->factory_cmd_state = FACTORYCMD_OK;
	} else {
		snprintf(strbuff, sizeof(strbuff), "%s", "NG");
		ssd->factory_cmd_state = FACTORYCMD_FAIL;
	}

	set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

//extern int cyttsp4_get_sysinfo_regs(struct cyttsp4_core_data *cd);
static void get_fw_ver_ic(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	struct cyttsp4_sysinfo *sysinfo;
	char strbuff[16] = {0};
	
	set_default_result(ssd);

	sysinfo = ssd->corecmd->update_sysinfo(ssd->dev);
	if (sysinfo) {
		//ssd->si = sysinfo;
		snprintf(strbuff, sizeof(strbuff), "CY%02x%02x%02x",
			sysinfo->sti->hw_version, 
			sysinfo->sti->fw_versionh, 
			sysinfo->sti->fw_versionl);
		ssd->factory_cmd_state = FACTORYCMD_OK;
	} else {
		sprintf(strbuff, "%s", "NG");
		ssd->factory_cmd_state = FACTORYCMD_FAIL;
	}

	set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	
	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_config_ver(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	char strbuff[16] = {0};

	set_default_result(ssd);

	if (ssd->si) {
		snprintf(strbuff, sizeof(strbuff), "%s_%02x",
			model_name,ssd->si->sti->config_version);
		ssd->factory_cmd_state = FACTORYCMD_OK;
	} else {
		sprintf(strbuff, "%s", "NG");
		ssd->factory_cmd_state = FACTORYCMD_FAIL;
	}

	set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	
	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

extern int cyttsp4_request_get_parameter_(struct device *dev,
		u8 param_id, u32 *param_value);
static void get_threshold(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	u32 value;
	int rc;
	char strbuff[16] = {0};

	set_default_result(ssd);

	rc = cyttsp4_request_get_parameter_(ssd->dev, 
		CY_RAM_ID_FINGER_THRESHOLH, &value);

	if (rc == 0) {
		snprintf(strbuff, sizeof(strbuff), "%d",
			value);
		ssd->factory_cmd_state = FACTORYCMD_OK;
	} else {
		sprintf(strbuff, "%s", "NG");
		ssd->factory_cmd_state = FACTORYCMD_FAIL;
	}

	set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	
	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_chip_vendor(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	char strbuff[16] = {0};

	set_default_result(ssd);
	snprintf(strbuff, sizeof(strbuff), "%s", "Cypress");
	set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	ssd->factory_cmd_state = FACTORYCMD_OK;
	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_chip_name(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	char strbuff[16] = {0};

	set_default_result(ssd);
	snprintf(strbuff, sizeof(strbuff), "%s", "CYTMA445");
	set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	ssd->factory_cmd_state = FACTORYCMD_OK;
	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_x_num(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	char strbuff[16] = {0};

	set_default_result(ssd);

	sprintf(strbuff, "%u", ssd->si->si_ptrs.pcfg->electrodes_x);
	set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));

	ssd->factory_cmd_state = FACTORYCMD_OK;
	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_y_num(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	char strbuff[16] = {0};

	set_default_result(ssd);

	sprintf(strbuff, "%u", ssd->si->si_ptrs.pcfg->electrodes_y);

	set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	ssd->factory_cmd_state = FACTORYCMD_OK;
	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}


/************************************************************************
 * commands - raw, diff
 ************************************************************************/
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

static s16 btn_value_s16(u8* buf, u8 btn, enum cyttsp4_scan_data_type data_type,
	u8 element_size)
{
	buf += CY_CMD_RET_PNL_OUT_DATA_OFFS;
	buf += (btn * 4 * element_size);
	if (data_type == CY_SDT_MUT_DIFF)
		buf += (2 * element_size);
	return (s16)get_unaligned_le16(buf);
}

static void run_raw_diff_read(struct cyttsp4_samsung_sysfs_data* ssd, 
	enum cyttsp4_scan_data_type data_type)
{
	s16 screen_min;
	s16 screen_max;
	char strbuff[8*4] = {0};
	int rc;

	set_default_result(ssd);

	rc = ssd->corecmd->scan_and_retrieve(ssd->dev, true, true, 
		0, ssd->num_all_nodes, data_type, ssd->screen_buf, NULL,
		&ssd->raw_diff_element_size);

	if (rc < 0) {
		dev_err(ssd->dev, "%s: scan_and_retrieve_ failed r=%d\n",
			__func__, rc);
		goto exit;
	}

	if (ssd->num_btns) {
		rc = ssd->corecmd->scan_and_retrieve(ssd->dev, true, true, 
			0, ssd->num_btns*4, CY_SDT_BTN, ssd->btn_buf, NULL,
			&ssd->raw_diff_element_size);

		if (rc < 0) {
			dev_err(ssd->dev, "%s: scan_and_retrieve_ btn failed r=%d\n",
				__func__, rc);
			goto exit;
		}
	}
	
exit:
	if (rc == 0) {
		if (ssd->raw_diff_element_size == 2)
			find_max_min_s16(ssd->screen_buf + CY_CMD_RET_PNL_OUT_DATA_OFFS,
				ssd->num_all_nodes, &screen_max, &screen_min);
		else
			find_max_min_s8(ssd->screen_buf + CY_CMD_RET_PNL_OUT_DATA_OFFS,
				ssd->num_all_nodes, &screen_max, &screen_min);
		snprintf(strbuff, sizeof(strbuff), "%d,%d", screen_min, screen_max);

		if (ssd->num_btns) {
			snprintf(strbuff         + strnlen(strbuff, sizeof(strbuff)), 
					 sizeof(strbuff) - strnlen(strbuff, sizeof(strbuff)), 
					 ",%d,%d", // btn0, btn1
					 btn_value_s16(ssd->btn_buf, 0, data_type, 
					 	ssd->raw_diff_element_size),
					 btn_value_s16(ssd->btn_buf, 1, data_type, 
					 	ssd->raw_diff_element_size) );
		}

		ssd->factory_cmd_state = FACTORYCMD_OK;
	} else {
		sprintf(strbuff, "%s", "NG");
		ssd->factory_cmd_state = FACTORYCMD_FAIL;
	}

	set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));

	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void run_raw_count_read(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	run_raw_diff_read(ssd, CY_SDT_MUT_RAW);
}

static void run_difference_read(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	run_raw_diff_read(ssd, CY_SDT_MUT_DIFF);

}

static inline s16 node_value_s16(u8* buf, u16 node)
{
	return (s16)get_unaligned_le16(buf + node * 2);
}

static void get_raw_diff(struct cyttsp4_samsung_sysfs_data* ssd)
{
	char strbuff[16] = {0};
	s16 value = 0;

	set_default_result(ssd);

	if ((ssd->factory_cmd_param[0] < 0) ||
		(ssd->factory_cmd_param[0] >= ssd->num_all_nodes)) {
		dev_err(ssd->dev, "%s: parameter %d is wrong\n",
					__func__, ssd->factory_cmd_param[0]);

		sprintf(strbuff, "%s", "NG");
		set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));

		ssd->factory_cmd_state = FACTORYCMD_FAIL;
	} else {
		if (ssd->raw_diff_element_size == 2)
			value = node_value_s16(ssd->screen_buf + CY_CMD_RET_PNL_OUT_DATA_OFFS,
				ssd->factory_cmd_param[0]);
		else
			value = ssd->screen_buf[CY_CMD_RET_PNL_OUT_DATA_OFFS + (u8)ssd->factory_cmd_param[0]];

		snprintf(strbuff, sizeof(strbuff), "%d", value);
		set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));

		ssd->factory_cmd_state = FACTORYCMD_OK;

		dev_info(ssd->dev, "%s: node %d = %d\n",
					__func__, ssd->factory_cmd_param[0], value);
	}

	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));

}

static void get_raw_count(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;

	get_raw_diff(ssd);
}

static void get_difference(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;

	get_raw_diff(ssd);
}

static void get_btn_raw_diff(struct cyttsp4_samsung_sysfs_data* ssd,
	enum cyttsp4_scan_data_type data_type)
{
	char strbuff[16] = {0};
	s16 value = 0;

	set_default_result(ssd);

	if ((ssd->factory_cmd_param[0] < 0) ||
		(ssd->factory_cmd_param[0] >= ssd->num_btns)) {
		dev_err(ssd->dev, "%s: parameter %d is wrong\n",
					__func__, ssd->factory_cmd_param[0]);

		sprintf(strbuff, "%s", "NG");
		set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));

		ssd->factory_cmd_state = FACTORYCMD_FAIL;
	} else {
		value = btn_value_s16(ssd->btn_buf, ssd->factory_cmd_param[0], 
			data_type, ssd->raw_diff_element_size),

		snprintf(strbuff, sizeof(strbuff), "%d", value);
		set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));

		ssd->factory_cmd_state = FACTORYCMD_OK;

		dev_info(ssd->dev, "%s: node %d = %d\n",
					__func__, ssd->factory_cmd_param[0], value);
	}

	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_raw_count_btn(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;

	get_btn_raw_diff(ssd, CY_SDT_MUT_RAW);
}

static void get_difference_btn(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;

	get_btn_raw_diff(ssd, CY_SDT_MUT_DIFF);
}


/************************************************************************
 * commands - IDAC
 ************************************************************************/
static u8 gidac_node_num(struct cyttsp4_samsung_sysfs_data* ssd)
{
	return 1;
}
static u16 lidac_node_num(struct cyttsp4_samsung_sysfs_data* ssd)
{
	return ssd->num_all_nodes;
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

static void run_idac_read(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	u8 screen_lidac_min;
	u8 screen_lidac_max;
	char strbuff[4*6] = {0};
	int rc;

	set_default_result(ssd);

	rc = ssd->corecmd->retrieve_data_structure(ssd->dev, 
		0, gidac_node_num(ssd)+lidac_node_num(ssd), 
		0/*mutual*/, ssd->screen_buf);
	if (rc < 0) {
		dev_err(ssd->dev, "%s: retrieve_data_structure failed r=%d\n",
			__func__, rc);
		goto exit;
	}

	if (ssd->num_btns) {
		rc = ssd->corecmd->retrieve_data_structure(ssd->dev, 
			0, 1 + ssd->num_btns, 
			3/*but mutual*/, ssd->btn_buf);
		if (rc < 0) {
			dev_err(ssd->dev, "%s: retrieve_data_structure failed r=%d\n",
				__func__, rc);
			goto exit;
		}
	}
	
exit:
	if (rc == 0) {	
		find_max_min_u8(ssd->screen_buf + CY_CMD_RET_PNL_OUT_DATA_OFFS + gidac_node_num(ssd),
			lidac_node_num(ssd), &screen_lidac_max, &screen_lidac_min);

		snprintf(strbuff, sizeof(strbuff), "%d,%d,%d", // global, local min, local max
			ssd->screen_buf[CY_CMD_RET_PNL_OUT_DATA_OFFS],
			screen_lidac_min, screen_lidac_max);
		ssd->factory_cmd_state = FACTORYCMD_OK;

		if (ssd->num_btns) {
			snprintf(strbuff         + strnlen(strbuff, sizeof(strbuff)), 
					 sizeof(strbuff) - strnlen(strbuff, sizeof(strbuff)), 
					 ",%d,%d,%d", // global, local 0, local 1
					 ssd->btn_buf[CY_CMD_RET_PNL_OUT_DATA_OFFS],
					 ssd->btn_buf[CY_CMD_RET_PNL_OUT_DATA_OFFS + 1],
					 ssd->btn_buf[CY_CMD_RET_PNL_OUT_DATA_OFFS + 2] );
		}
	} else {
		sprintf(strbuff, "%s", "NG");
		ssd->factory_cmd_state = FACTORYCMD_FAIL;
	}

	set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));
	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_idac(struct cyttsp4_samsung_sysfs_data* ssd,
	int max_node_num, u8 *buf)
{
	char strbuff[16] = {0};
	u8 value = 0;

	set_default_result(ssd);

	if ((ssd->factory_cmd_param[0] < 0) ||
		(ssd->factory_cmd_param[0] >= max_node_num)) {
		dev_err(ssd->dev, "%s: parameter %d is wrong\n",
					__func__, ssd->factory_cmd_param[0]);

		sprintf(strbuff, "%s", "NG");
		set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));

		ssd->factory_cmd_state = FACTORYCMD_FAIL;
	} else {
		value = buf[(u8)ssd->factory_cmd_param[0]];

		snprintf(strbuff, sizeof(strbuff), "%d", value);
		set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));

		ssd->factory_cmd_state = FACTORYCMD_OK;

		dev_info(ssd->dev, "%s: node %d = %d\n",
					__func__, ssd->factory_cmd_param[0], value);
	}
	dev_info(ssd->dev, "%s: %s(%d)\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
}

static void get_global_idac(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;

	get_idac(ssd, gidac_node_num(ssd), ssd->screen_buf + CY_CMD_RET_PNL_OUT_DATA_OFFS);
}

static void get_local_idac(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;

	get_idac(ssd, lidac_node_num(ssd), ssd->screen_buf + CY_CMD_RET_PNL_OUT_DATA_OFFS + gidac_node_num(ssd));
}

static void get_global_idac_btn(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;

	get_idac(ssd, gidac_node_num(ssd), ssd->btn_buf + CY_CMD_RET_PNL_OUT_DATA_OFFS);
}

static void get_local_idac_btn(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;

	get_idac(ssd, lidac_node_num(ssd), ssd->btn_buf + CY_CMD_RET_PNL_OUT_DATA_OFFS + 1);
}


/************************************************************************
 * 
 ************************************************************************/
static void not_support_cmd(void *device_data)
{
	struct cyttsp4_samsung_sysfs_data* ssd =
		(struct cyttsp4_samsung_sysfs_data *) device_data;
	char strbuff[16] = {0};

	set_default_result(ssd);
	sprintf(strbuff, "%s", "NA");
	set_cmd_result(ssd, strbuff, strnlen(strbuff, sizeof(strbuff)));

	mutex_lock(&ssd->factory_cmd_lock);
	ssd->factory_cmd_is_running = false;
	mutex_unlock(&ssd->factory_cmd_lock);

	ssd->factory_cmd_state = FACTORYCMD_WAITING;
	dev_info(ssd->dev, "%s: \"%s(%d)\"\n", __func__,
		strbuff, strnlen(strbuff, sizeof(strbuff)));
	return;
}

static ssize_t store_cmd(struct device *dev, struct device_attribute
		*devattr, const char *buf, size_t count)
{
	struct cyttsp4_samsung_sysfs_data *ssd = dev_get_drvdata(dev);
	struct factory_cmd *factory_cmd_ptr = NULL;
	int param_cnt = 0;
	int ret, len, i;
	char *cur, *start, *end;
	char strbuff[FACTORY_CMD_STR_LEN] = {0};
	char delim = ',';
	bool cmd_found = false;

	if (strlen(buf) >= FACTORY_CMD_STR_LEN) {
		dev_err(ssd->dev, "%s: cmd length is over(%s,%d)!!\n", __func__, buf, (int)strlen(buf));
		return -EINVAL;
	}

	if (ssd->factory_cmd_is_running == true) {
		dev_err(ssd->dev, "factory_cmd: other cmd is running.\n");
		goto err_out;
	}

	/* check lock  */
	mutex_lock(&ssd->factory_cmd_lock);
	ssd->factory_cmd_is_running = true;
	mutex_unlock(&ssd->factory_cmd_lock);

	ssd->factory_cmd_state = FACTORYCMD_RUNNING;

	for (i = 0; i < ARRAY_SIZE(ssd->factory_cmd_param); i++)
		ssd->factory_cmd_param[i] = 0;

	len = (int)count;
	if (*(buf + len - 1) == '\n')
		len--;
	memset(ssd->factory_cmd, 0x00, ARRAY_SIZE(ssd->factory_cmd));
	memcpy(ssd->factory_cmd, buf, len);

	cur = strchr(buf, (int)delim);
	if (cur)
		memcpy(strbuff, buf, cur - buf);
	else
		memcpy(strbuff, buf, len);

	/* find command */
	list_for_each_entry(factory_cmd_ptr,
			&ssd->factory_cmd_list_head, list) {
		if (!strcmp(strbuff, factory_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(factory_cmd_ptr,
				&ssd->factory_cmd_list_head, list) {
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
						ssd->factory_cmd_param + param_cnt);
				start = cur + 1;
				memset(strbuff, 0x00, ARRAY_SIZE(strbuff));
				param_cnt++;
			}
			cur++;
		} while ((cur - buf <= len) && (param_cnt < FACTORY_CMD_PARAM_NUM));
	}

	dev_info(ssd->dev, "cmd = %s\n", factory_cmd_ptr->cmd_name);
	for (i = 0; i < param_cnt; i++)
		dev_info(ssd->dev, "cmd param %d= %d\n", i,
			ssd->factory_cmd_param[i]);

	factory_cmd_ptr->cmd_func(ssd);

err_out:
	return count;
}

static ssize_t show_cmd_status(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct cyttsp4_samsung_sysfs_data *ssd = dev_get_drvdata(dev);
	char strbuff[16] = {0};

	dev_info(ssd->dev, "tsp cmd: status:%d, PAGE_SIZE=%ld\n",
			ssd->factory_cmd_state, PAGE_SIZE);

	if (ssd->factory_cmd_state == FACTORYCMD_WAITING)
		snprintf(strbuff, sizeof(strbuff), "WAITING");

	else if (ssd->factory_cmd_state == FACTORYCMD_RUNNING)
		snprintf(strbuff, sizeof(strbuff), "RUNNING");

	else if (ssd->factory_cmd_state == FACTORYCMD_OK)
		snprintf(strbuff, sizeof(strbuff), "OK");

	else if (ssd->factory_cmd_state == FACTORYCMD_FAIL)
		snprintf(strbuff, sizeof(strbuff), "FAIL");

	else if (ssd->factory_cmd_state == FACTORYCMD_NOT_APPLICABLE)
		snprintf(strbuff, sizeof(strbuff), "NOT_APPLICABLE");

	return snprintf(buf, PAGE_SIZE, "%s\n", strbuff);
}

static ssize_t show_cmd_result(struct device *dev, struct device_attribute
		*devattr, char *buf)
{
	struct cyttsp4_samsung_sysfs_data *ssd = dev_get_drvdata(dev);

	dev_info(ssd->dev, "tsp cmd: result: %s\n", ssd->factory_cmd_result);

	mutex_lock(&ssd->factory_cmd_lock);
	ssd->factory_cmd_is_running = false;
	mutex_unlock(&ssd->factory_cmd_lock);

	ssd->factory_cmd_state = FACTORYCMD_WAITING;

	return snprintf(buf, PAGE_SIZE, "%s\n", ssd->factory_cmd_result);
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
 * sysfs screen
 ************************************************************************/
#define SEC_TOUCHSCREEN
#ifdef SEC_TOUCHSCREEN
extern int cyttsp4_fw_calibrate(struct device *dev);
static ssize_t tsp_calibration_run(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cyttsp4_samsung_sysfs_data *ssd = dev_get_drvdata(dev);
	int rc;

	rc = cyttsp4_fw_calibrate(ssd->dev);

	if(rc == 0) {
		return sprintf(buf, "idac calibration has done.\n");
	} else {
		return sprintf(buf, "idac calibration fail.\n");
	}
}

static DEVICE_ATTR(tsp_calibration, S_IRUGO | S_IWUSR | S_IWGRP, tsp_calibration_run, NULL);

static struct attribute *sec_touch_screen_attributes[] = {
	&dev_attr_tsp_calibration.attr,		
	NULL,
};
static struct attribute_group sec_touch_screen_attr_group = {
	.attrs	= sec_touch_screen_attributes,
};
#endif//SEC_TOUCHSCREEN


/************************************************************************
 * sysfs key
 ************************************************************************/
#define SEC_TOUCHKEY
#ifdef SEC_TOUCHKEY

static ssize_t key_sensitivity_show(struct device *dev, 
	struct device_attribute *attr, char *buf, int key)
{
	struct cyttsp4_samsung_sysfs_data *ssd = dev_get_drvdata(dev);
	s16 value;
	int rc;

	if (!ssd->num_btns)
		return 0;

	if (key >= ssd->num_btns)
		return 0;
	
	pm_runtime_get_sync(ssd->dev);

	rc = ssd->corecmd->scan_and_retrieve(ssd->dev, true, true, 
		0, ssd->num_btns*4, CY_SDT_BTN, ssd->btn_buf, NULL,
		&ssd->raw_diff_element_size);

	if (rc < 0) {
		dev_err(ssd->dev, "%s: scan_and_retrieve_ btn failed r=%d\n",
			__func__, rc);
		return 0;
	}

	pm_runtime_put(ssd->dev);

	value = btn_value_s16(ssd->btn_buf, key, CY_SDT_MUT_DIFF, 
					 ssd->raw_diff_element_size);

	return sprintf(buf, "%d\n", value);
}


static ssize_t recent_sensitivity_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return key_sensitivity_show(dev, attr, buf, 0);
}

static ssize_t back_sensitivity_show(struct device *dev, 
	struct device_attribute *attr, char *buf)
{
	return key_sensitivity_show(dev, attr, buf, 1);
}

static ssize_t touchkey_threshold_show(struct device *dev, 
	struct device_attribute *attr, char *buf)
{
	struct cyttsp4_samsung_sysfs_data *ssd = dev_get_drvdata(dev);
	u32 value;
	int rc;
	
	rc = cyttsp4_request_get_parameter_(ssd->dev, 
		CY_RAM_ID_BTN_THRSH_MUT, &value);
	if (rc < 0) {
		dev_err(ssd->dev, "%s: failed request get param r=%d\n",
			__func__, rc);
		return 0;
	}
	return sprintf(buf, "%d\n", value);
}

static ssize_t touch_version_read(struct device *dev,
	struct device_attribute *attr, char *buf){
	return snprintf(buf, 4, "%s", "N");
}

static DEVICE_ATTR(touchkey_recent, S_IRUGO, recent_sensitivity_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, back_sensitivity_show, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO, touchkey_threshold_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO,
							touch_version_read, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO,
							touch_version_read, NULL);

static struct attribute *sec_touch_key_attributes[] = {
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	NULL,
};
static struct attribute_group sec_touch_key_attr_group = {
	.attrs	= sec_touch_key_attributes,
};
#endif//SEC_TOUCHKEY

/************************************************************************
 * init
 ************************************************************************/
int cyttsp4_samsung_sysfs_probe(struct device *dev)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp4_samsung_sysfs_data *ssd = &cd->ssd;
	int rc = 0;
	int i;

	dev_info(dev, "%s: \n",	__func__);
	
	ssd->dev = dev;

	ssd->corecmd = cyttsp4_get_commands();
	if (!ssd->corecmd) {
		dev_err(dev, "%s: core cmd not available\n", __func__);
		rc = -EINVAL;
		goto error_return;
	}

	ssd->si = cyttsp4_request_sysinfo_(dev);
	if (!ssd->si) {
		dev_err(dev, "%s: Fail get sysinfo pointer from core\n", __func__);
		rc = -EINVAL;
		goto error_return;
	}

	dev_dbg(dev, "%s: electrodes_x=%d\n", __func__,
		ssd->si->si_ptrs.pcfg->electrodes_x);
	dev_dbg(dev, "%s: electrodes_y=%d\n", __func__,
		ssd->si->si_ptrs.pcfg->electrodes_y);
	ssd->num_all_nodes = ssd->si->si_ptrs.pcfg->electrodes_x *
		ssd->si->si_ptrs.pcfg->electrodes_y;
	if (ssd->num_all_nodes > MAX_NODE_NUM) {
		dev_err(dev, "%s: sensor node num(%d) exceeds limits\n", __func__,
			ssd->num_all_nodes);
		rc = -EINVAL;
		goto error_return;
	}

	dev_dbg(dev, "%s: btn num=%d\n", __func__,
		ssd->si->si_ofs.num_btns);
	ssd->num_btns = ssd->si->si_ofs.num_btns;
	if (ssd->num_btns > MAX_BTN_NUM) {
		dev_err(dev, "%s: btn num(%d) exceeds limits\n", __func__,
			ssd->num_btns);
		rc = -EINVAL;
		goto error_return;
	}

	ssd->screen_buf = kzalloc((MAX_INPUT_HEADER_SIZE +
		ssd->num_all_nodes * 2), GFP_KERNEL);
	if (ssd->screen_buf == NULL) {
		dev_err(dev, "%s: Error, kzalloc screen_buf\n", __func__);
		rc = -ENOMEM;
		goto error_return;
	}

	ssd->btn_buf = kzalloc((MAX_INPUT_HEADER_SIZE +
		ssd->num_btns * 8), GFP_KERNEL);
	if (ssd->btn_buf == NULL) {
		dev_err(dev, "%s: Error, kzalloc btn_buf\n", __func__);
		rc = -ENOMEM;
		goto error_alloc_btn_buf;
	}

	INIT_LIST_HEAD(&ssd->factory_cmd_list_head);
	for (i = 0; i < ARRAY_SIZE(factory_cmds); i++)
		list_add_tail(&factory_cmds[i].list, &ssd->factory_cmd_list_head);

	mutex_init(&ssd->factory_cmd_lock);
	ssd->factory_cmd_is_running = false;

//-- sysfs factory
	ssd->dev_factory = device_create(sec_class, NULL, 
		MKDEV(SEC_DEV_TOUCH_MAJOR, SEC_DEV_TSP_MINOR), ssd, "tsp");
	if (IS_ERR(ssd->dev_factory)) {
		dev_err(ssd->dev, "Failed device_create tsp\n");
		goto error_device_create_factory;
	}
	dev_dbg(dev, "%s ssd->dev_factory->devt=%d\n",__func__, ssd->dev_factory->devt);

	rc = sysfs_create_group(&ssd->dev_factory->kobj,
		&sec_touch_factory_attr_group);
	if (rc) {
		dev_err(ssd->dev, "Failed sysfs_create_group factory\n");
		goto error_sysfs_create_group_factory;
	}

#ifdef SEC_TOUCHSCREEN
//-- sysfs screen
	ssd->dev_screen = device_create(sec_class, NULL, 
		MKDEV(SEC_DEV_TOUCH_MAJOR,SEC_DEV_TOUCHSCREEN_MINOR), ssd, "sec_touchscreen");
	if (IS_ERR(ssd->dev_screen)) {
		dev_err(ssd->dev, "Failed device_create sec_touchscreen\n");
		goto error_device_create_screen;
	}
	dev_dbg(dev, "%s ssd->dev_screen->devt=%d\n",__func__, ssd->dev_screen->devt);

	rc = sysfs_create_group(&ssd->dev_screen->kobj,
		&sec_touch_screen_attr_group);
	if (rc) {
		dev_err(ssd->dev, "Failed sysfs_create_group screen\n");
		goto error_sysfs_create_group_screen;
	}
#endif//SEC_TOUCHSCREEN

#ifdef SEC_TOUCHKEY
//-- sysfs key
	ssd->dev_key = device_create(sec_class, NULL, 
		MKDEV(SEC_DEV_TOUCH_MAJOR, SEC_DEV_TOUCHKEY_MINOR), ssd, "sec_touchkey");
	if (IS_ERR(ssd->dev_key)) {
		dev_err(ssd->dev, "Failed device_create sec_touchkey\n");
		goto error_device_create_key;
	}
	dev_dbg(dev, "%s ssd->dev_key->devt=%d\n",__func__, ssd->dev_key->devt);
	
	rc = sysfs_create_group(&ssd->dev_key->kobj,
		&sec_touch_key_attr_group);
	if (rc) {
		dev_err(ssd->dev, "Failed sysfs_create_group key\n");
		goto error_sysfs_create_group_key;
	}
#endif//SEC_TOUCHKEY
	ssd->sysfs_nodes_created = true;

	dev_dbg(ssd->dev, "%s success. rc=%d\n", __func__, rc);
	return 0;

#ifdef SEC_TOUCHKEY
error_sysfs_create_group_key:
	device_destroy(sec_class, ssd->dev_key->devt);
error_device_create_key:
#ifdef SEC_TOUCHSCREEN
	sysfs_remove_group(&ssd->dev_screen->kobj,
			&sec_touch_screen_attr_group);
#endif//SEC_TOUCHSCREEN
#endif//SEC_TOUCHKEY

#ifdef SEC_TOUCHSCREEN
error_sysfs_create_group_screen:
	device_destroy(sec_class, ssd->dev_screen->devt);
error_device_create_screen:
#endif//SEC_TOUCHSCREEN

#if defined(SEC_TOUCHKEY) || defined(SEC_TOUCHSCREEN)
	sysfs_remove_group(&ssd->dev_factory->kobj,
			&sec_touch_factory_attr_group);
#endif

error_sysfs_create_group_factory:
	device_destroy(sec_class, ssd->dev_factory->devt);
error_device_create_factory:
	kfree(ssd->btn_buf);	
error_alloc_btn_buf:
	kfree(ssd->screen_buf);
error_return:
	dev_err(dev, "%s failed. rc=%d\n", __func__, rc);
	return rc;
}

int cyttsp4_samsung_sysfs_release(struct device *dev)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp4_samsung_sysfs_data *ssd = &cd->ssd;

	dev_info(dev, "%s\n",__func__);

	if (ssd->sysfs_nodes_created) {
#ifdef SEC_TOUCHKEY
		sysfs_remove_group(&ssd->dev_key->kobj,
			&sec_touch_key_attr_group);
		dev_dbg(dev, "%s ssd->dev_key->devt=%d\n",__func__, ssd->dev_key->devt);
		device_destroy(sec_class, ssd->dev_key->devt);
#endif

#ifdef SEC_TOUCHSCREEN
		sysfs_remove_group(&ssd->dev_screen->kobj,
			&sec_touch_screen_attr_group);
		dev_dbg(dev, "%s ssd->dev_screen->devt=%d\n",__func__, ssd->dev_screen->devt);
		device_destroy(sec_class, ssd->dev_screen->devt);
#endif

		sysfs_remove_group(&ssd->dev_factory->kobj,
			&sec_touch_factory_attr_group);
		dev_dbg(dev, "%s ssd->dev_factory->devt=%d\n",__func__, ssd->dev_factory->devt);
		device_destroy(sec_class, ssd->dev_factory->devt);

		ssd->sysfs_nodes_created = false;
	}

	kfree(ssd->btn_buf);
	kfree(ssd->screen_buf);

	return 0;
}

