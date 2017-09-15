/*
* Copyright (C) 2014 Invensense, Inc.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

/**
 * @file inv_sensors_control.c
 * @author IKD
 */

#include "inv_sensors_common.h"
#include "inv_sensors_sm.h"
#include "inv_sensors_control.h"

#define CCMD_ACCEL       "A"
#define CCMD_GYRO        "G"
#define CCMD_MAG          "M"
#define CCMD_PRESSURE   "P"
#define CCMD_ROTATION_VECTOR   "RV"
#define CCMD_GAME_ROTATION_VECTOR   "GRV"
#define CCMD_SIGNIFICANT_MOTION   "SMD"
#define CCMD_STEP_DETECTOR   "SD"
#define CCMD_STEP_COUNTER   "SC"
#define CCMD_GEOMAG_ROTATION_VECTOR   "GMRV"
#define CCMD_BATCH   "BATCH"
#define CCMD_FLUSH   "FLUSH"


#define CCMD_SYNC "SYNC"
#define CCFG_HEADER "CCFG"

#define CCONFIG_WRITE_DGB  "WDGB"
#define CCONFIG_WRITE_DAB  "WDAB"
#define CCONFIG_READ_DBG  "RDGB"
#define CCONFIG_READ_DAB  "RDAB"


struct control_item_t {
	const char *cmd_str;
	int control_type;
	u32 func_type;
};

struct control_ccfg_item_t {
	const char *ccfg_str;
	u32 ccfg_id;
};

static const struct control_item_t control_cmd_table[] = {
	{CCMD_ACCEL, INV_CONTROL_ENABLE_SENSOR, INV_FUNC_ACCEL},
	{CCMD_GYRO, INV_CONTROL_ENABLE_SENSOR, INV_FUNC_GYRO},
	{CCMD_MAG, INV_CONTROL_ENABLE_SENSOR, INV_FUNC_COMPASS},
	{CCMD_ROTATION_VECTOR, INV_CONTROL_ENABLE_SENSOR,
	 INV_FUNC_ROTATION_VECTOR},
	{CCMD_GAME_ROTATION_VECTOR, INV_CONTROL_ENABLE_SENSOR,
	 INV_FUNC_GAMING_ROTATION_VECTOR},
	{CCMD_SIGNIFICANT_MOTION, INV_CONTROL_ENABLE_SENSOR,
	 INV_FUNC_SIGNIFICANT_MOTION_DETECT},
	{CCMD_STEP_DETECTOR, INV_CONTROL_ENABLE_SENSOR,
	 INV_FUNC_STEP_DETECT},
	{CCMD_STEP_COUNTER, INV_CONTROL_ENABLE_SENSOR,
	 INV_FUNC_STEP_COUNT},
	{CCMD_GEOMAG_ROTATION_VECTOR, INV_CONTROL_ENABLE_SENSOR,
	 INV_FUNC_GEOMAG_ROTATION_VECTOR},
	{CCMD_BATCH, INV_CONTROL_BATCH, 0},
	{CCMD_FLUSH, INV_CONTROL_FLUSH, 0},
	{CCMD_SYNC, INV_CONTROL_SYNC, 0},
	{CCFG_HEADER, INV_CONTROL_CONFIG, 0},
};


static const struct control_ccfg_item_t control_ccfg_table[] = {
	{CCONFIG_WRITE_DGB, INV_CCFG_WDGB},
	{CCONFIG_WRITE_DAB, INV_CCFG_WDAB},
};


#define CONTROL_CMD_ITEM_NUM \
	(sizeof(control_cmd_table) / sizeof(control_cmd_table[0]))

#define CONTROL_CCFG_ITEM_NUM \
	(sizeof(control_ccfg_table) / sizeof(control_ccfg_table[0]))


static const struct control_item_t *control_find_cmd(const char *cmd_str)
{
	int i = 0, len = CONTROL_CMD_ITEM_NUM;
	int cmd_str_len = 0;

	if (cmd_str == NULL)
		goto error_case;

	cmd_str_len = strlen(cmd_str);

	for (i = 0; i < len; i++) {
		const struct control_item_t *p = &control_cmd_table[i];

		if ((cmd_str_len == strlen(p->cmd_str)) &&
		    !memcmp(cmd_str, p->cmd_str, strlen(p->cmd_str)))
			return p;
	}

error_case:
	return NULL;
}

static int control_parse_enable_sensor(const char *control_string,
				   struct invsens_control_data_t *control_data)
{
	char *param = NULL;

	INV_DBG_FUNC_NAME;

	param = strsep((char **) &control_string, ":");
	if (param)
		control_data->delay = simple_strtoul(param, NULL, 0);

	INVSENS_LOGD("%s control_data.func %d control_data.delay %ld", __func__,
		     control_data->func, control_data->delay);

	return 0;
}

static int control_parse_batch(const char *control_string,
			   struct invsens_control_data_t *control_data)
{
	char *param = NULL;

	INV_DBG_FUNC_NAME;

	param = strsep((char **) &control_string, ":");
	if (param) {
		const struct control_item_t *cmd = control_find_cmd(param);
		if (cmd) {
			control_data->func = cmd->func_type;
			param = strsep((char **) &control_string, ":");
			if (param)
				control_data->flags =
					simple_strtoul(param, NULL, 0);

			param = strsep((char **) &control_string, ":");
			if (param)
				control_data->delay =
					simple_strtoul(param, NULL, 0);

			param = strsep((char **) &control_string, ":");
			if (param)
				control_data->timeout =
					simple_strtoul(param, NULL, 0);
		}
	}

	return 0;
}

static int control_parse_sync(const char *control_string,
			  struct invsens_control_data_t *control_data)
{
	INV_DBG_FUNC_NAME;

	control_data->key = strsep((char **) &control_string, ":");

	INVSENS_LOGD("sync : %s\n", control_data->key);

	return 0;
}

static int control_parse_ccfg(const char *control_string,
			  struct invsens_control_data_t *control_data)
{
	int i = 0, len = CONTROL_CCFG_ITEM_NUM;
	int str_len, res = SM_SUCCESS;
	char *ccfg_name = NULL;

	INV_DBG_FUNC_NAME;

	ccfg_name = strsep((char **) &control_string, ":");

	for (; i < len; i++) {
		const struct control_ccfg_item_t *ccfg = &control_ccfg_table[i];
		str_len = strlen(ccfg_name);
		if ((str_len == strlen(ccfg->ccfg_str)) &&
		    !memcmp(ccfg_name, ccfg->ccfg_str, strlen(ccfg->ccfg_str))) {
			int cnt = 0;
			control_data->param_count = 0;
			do {
				char *p = strsep((char **)&control_string,
					",");
				if (!p || strlen(p) == 0)
					break;

				res = kstrtol(p, 0,
					&control_data->params[cnt++]);
				if (res)
					return -SM_EFAIL;
			} while (cnt < INVSENS_CCFG_PARAM_MAX);
			control_data->config = ccfg->ccfg_id;
			control_data->param_count = cnt;

			return res;
		}
	}

	return -SM_ENOTEXIST;
}


int invsens_parse_control(const char *control_string, void *user_data,
		      int (*callback) (void *, struct invsens_control_data_t *))
{
	int result = 0;
	const struct control_item_t *p = NULL;
	char *str = NULL;

	struct invsens_control_data_t control_data = {
		.control_type = 0, .func = 0,
		.flags = 0, .delay = -1, .timeout = 0
	};

	INV_DBG_FUNC_NAME;

	str = strsep((char **) &control_string, ":");

	p = control_find_cmd(str);
	if (p == NULL)
		return -SM_EINVAL;

	control_data.control_type = p->control_type;
	control_data.func = p->func_type;

	switch (control_data.control_type) {
	case INV_CONTROL_FLUSH:
		break;

	case INV_CONTROL_SYNC:
		result = control_parse_sync(control_string, &control_data);
		break;

	case INV_CONTROL_ENABLE_SENSOR:
		result = control_parse_enable_sensor(control_string,
			&control_data);
		break;

	case INV_CONTROL_BATCH:
		result = control_parse_batch(control_string, &control_data);
		break;

	case INV_CONTROL_CONFIG:
		result = control_parse_ccfg(control_string, &control_data);
		break;

	default:
		break;
	};

	callback(user_data, &control_data);

	return result;
}
