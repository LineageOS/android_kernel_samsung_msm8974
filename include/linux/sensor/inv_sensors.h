#ifndef _INV_SENSORS_H_
#define _INV_SENSORS_H_

/*
	secondary compass ids;
*/

enum {
	INVSENS_AID_NONE = 0,
	INVSENS_AID_AKM8975,
	INVSENS_AID_AKM8963,
	INVSENS_AID_AKM09911,
	INVSENS_AID_NUM
};


struct invsens_auxiliary_data_t {
	u8 aux_id;
	u8 i2c_addr;
	signed char orientation[9];
};

struct invsens_platform_data_t {
	unsigned char int_config;

	signed char orientation[9];
	unsigned char level_shifter;

	struct invsens_auxiliary_data_t compass;
	struct invsens_auxiliary_data_t accel;

#ifdef CONFIG_INV_SENSORS_DTS_SUPPORT
	int (*power_on)(struct invsens_platform_data_t *);
	int (*power_off)(struct invsens_platform_data_t *);
	struct regulator *vdd;
#endif

};


#endif //_INV_SENSORS_H_

