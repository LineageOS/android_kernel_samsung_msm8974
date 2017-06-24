#ifndef _INV_SENSORS_CMD_H_
#define _INV_SENSORS_CMD_H_

#include "inv_sensors_sm.h"

#define INVSENS_CCFG_PARAM_MAX 8

enum invsens_control_type {
	INV_CONTROL_NONE = 0,

	INV_CONTROL_ENABLE_SENSOR,
	INV_CONTROL_BATCH,
	INV_CONTROL_FLUSH,
	INV_CONTROL_SYNC,

	INV_CONTROL_CONFIG,

	INV_CONTROL_NUM
};


enum invsens_ccfg_type {
	INV_CCFG_NONE = 0,

	INV_CCFG_WDGB,
	INV_CCFG_WDAB,

	INV_CCFG_NUM
};

struct invsens_control_data_t {
	int control_type;

	union {
		char *key;
		u32 func;
		u32 config;
	};

	long flags;
	long delay;
	long timeout;

	int param_count;
	long params[INVSENS_CCFG_PARAM_MAX];
};


int invsens_parse_control(const char *control_string, void *user_data,
	      int (*callback) (void *, struct invsens_control_data_t *));

#endif		/*_INV_SENSORS_CMD_H_*/
