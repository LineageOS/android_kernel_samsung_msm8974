#ifndef _INV_DMP_BASE_H_
#define _INV_DMP_BASE_H_


#define INVDMP_DEFAULT_FREQ     200
#define INVDMP_VERSION_LEN (64)


enum invdmp_feat_type {
	INV_DFEAT_ACCEL = 0,
	INV_DFEAT_GYRO,
	INV_DFEAT_COMPASS,
	INV_DFEAT_PRESSURE,
	INV_DFEAT_3AXIS_QUAT,
	INV_DFEAT_6AXIS_QUAT,
	INV_DFEAT_PED_QUAT,
	INV_DFEAT_STEP_IND,
	INV_DFEAT_STEP_DETECTOR,
	INV_DFEAT_SCREEN_ORIENTATION,
	INV_DFEAT_SMD,
	INV_DFEAT_PEDO,
	INV_DFEAT_MAX
};



struct invdmp_key_t {
	u16 key;
	u16 addr;
};


struct invdmp_fw_data_t {
	struct invdmp_key_t *key_map;
	u16 key_size;
};



struct invdmp_driver_t {

	char version[INVDMP_VERSION_LEN];

	int start_address;

	int (*init) (struct invdmp_driver_t *drv,
		     const signed char *orientation);
	int (*enable_func) (struct invdmp_driver_t *drv, u32 func_mask,
			    bool enabled, void *user_cfg);
	int (*read) (struct invdmp_driver_t *drv,
		     struct invsens_data_list_t *buffer);
	int (*suspend) (struct invdmp_driver_t * drv);
	int (*resume) (struct invdmp_driver_t * drv);
	int (*terminate) (struct invdmp_driver_t *drv);
	int (*ioctl)(struct invdmp_driver_t *drv, u32 cmd,
			     long lparam, void *vparam);
	void *user_data;
};


/**
	fundamental apis for dmp dirvers
*/

int invdmp_write_memory(u16 addr, u16 length,
	const uint8_t *data, void *dbase_data);
int invdmp_read_memory(u16 addr, u16 length, uint8_t *data,
	void *dbase_data);
int invdmp_load_firmware(const char *filename, int offset, int start_addr,
	void *dbase_data);


/**
	load dmp module
*/
int invsens_load_dmp(struct invsens_driver_t **driver,
	struct invsens_board_cfg_t *board_cfg);


#endif	/* _INV_DMP_BASE_H_ */
