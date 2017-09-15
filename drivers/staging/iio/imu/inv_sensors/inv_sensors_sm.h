#ifndef _INV_SENSORS_SM_H_
#define _INV_SENSORS_SM_H_

#include "inv_sensors_common.h"

#define SM_SUCCESS (0)

#define SM_IOCTL_NOTHANDLED (0x60)
#define SM_IOCTL_HANDLED (0x61)

/*
	internal error codes;
*/
#define SM_EFAIL (0x70)
#define SM_ENOMEM (0x71)
#define SM_EINVAL (0x72)
#define SM_EUNSUPPORT (0x74)
#define SM_ENOTEXIST	(0x75)
#define SM_EINCOMPLETE	(0x76)


/*
	protocol definition between HAL and kernel
*/

#define INV_DATA_HDR_MAGIC 0xac00

#define INV_SET_HDR_MAGIC(hdr) \
	(INV_DATA_HDR_MAGIC | hdr)

#define INV_DATA_HDR_ACCEL	INV_SET_HDR_MAGIC(0x1)
#define INV_DATA_HDR_GYRO	INV_SET_HDR_MAGIC(0x2)
#define INV_DATA_HDR_COMPASS	INV_SET_HDR_MAGIC(0x3)
#define INV_DATA_HDR_STEPDET	INV_SET_HDR_MAGIC(0x4)
#define INV_DATA_HDR_LPQUAT	INV_SET_HDR_MAGIC(0x5)
#define INV_DATA_HDR_PRESS	INV_SET_HDR_MAGIC(0x6)
#define INV_DATA_HDR_PEDO	INV_SET_HDR_MAGIC(0x7)
#define INV_DATA_HDR_SMD	INV_SET_HDR_MAGIC(0x8)
#define INV_DATA_HDR_6AXIS_QUAT	INV_SET_HDR_MAGIC(0x9)



#define INVSENS_AXIS_NUM 3
#define INV_DMP_RATE_DIV (4) /*smplrt_div for dmp*/


#define INVSENS_DATA_BUFFER_MAX (16)
#define INVSENS_DATA_ITEM_MAX (64)
#define INVSENS_PACKET_HEADER (0x4956) /*IV*/
#define INVSENS_COPY_BUFFER_SIZE (2048)
#define INVSENS_DMP_DATUM (8)


enum invsens_chip_type {
	INV_CHIP_TYPE_NOME = 0,
	INV_CHIP_TYPE_GYRO_ONLY,
	INV_CHIP_TYPE_ACCEL_GYRO,
	INV_CHIP_TYPE_ACCEL_GYRO_COMPASS,
};


enum invsens_driver_layer {
	INV_DRIVER_LAYER_NATIVE = 0,
	INV_DRIVER_LAYER_PLATFORM,
};

enum invsens_driver_id {
	INV_DRIVER_NONE = 0,
	INV_DRIVER_MPU65XX,
	INV_DRIVER_AUX,
	INV_DRIVER_DMP,
};

enum invsens_func_type {
	/**
		functions matched with android sensor type
	*/
	INV_FUNC_ACCEL = 0,
	INV_FUNC_GYRO,
	INV_FUNC_COMPASS,
	INV_FUNC_GAMING_ROTATION_VECTOR,
	INV_FUNC_SIGNIFICANT_MOTION_DETECT,
	INV_FUNC_STEP_DETECT,
	INV_FUNC_STEP_COUNT,
	INV_FUNC_SCREEN_ORIENTATION,
	INV_FUNC_ROTATION_VECTOR,
	INV_FUNC_GEOMAG_ROTATION_VECTOR,
	INV_FUNC_BATCH,
	INV_FUNC_FLUSH,

	/**
		additional functions
	*/
	INV_FUNC_MOTION_INTERRUPT,
	INV_FUNC_FT_SENSOR_ON,
	INV_FUNC_NUM
};


enum invsens_ioctl_cmd {
	INV_IOCTL_NONE = 0,
	INV_IOCTL_ACCEL_SW_SELFTEST,
	INV_IOCTL_GYRO_SW_SELFTEST,
	INV_IOCTL_ACCEL_HW_SELFTEST,
	INV_IOCTL_GYRO_HW_SELFTEST,
	INV_IOCTL_COMPASS_SELFTEST,
	INV_IOCTL_S_GYRO_SELFTEST,
	INV_IOCTL_S_ACCEL_SELFTEST,
	INV_IOCTL_GET_ACCEL_RAW,
	INV_IOCTL_GET_GYRO_RAW,
	INV_IOCTL_SET_GYRO_DMP_BIAS,
	INV_IOCTL_SET_ACCEL_DMP_BIAS,
	INV_IOCTL_GET_ACCEL_BIAS,
	INV_IOCTL_SET_ACCEL_BIAS,
	INV_IOCTL_DO_ACCEL_CAL,
	INV_IOCTL_SET_WOM_THRESH,
	INV_IOCTL_GET_TEMP,
	INV_IOCTL_RESET_FIFO,
	INV_IOCTL_NUM
};



struct invsense_ioctl_swst_gyro_t {
	int result;
	long bias[INVSENS_AXIS_NUM];	
	long rms[INVSENS_AXIS_NUM];
	int packet_cnt;
};

struct invsense_ioctl_hwst_accel_t {
	int result;
	int ratio[INVSENS_AXIS_NUM];
};

struct invsense_ioctl_hwst_gyro_t {
	int result;
	int ratio[INVSENS_AXIS_NUM];	
};

struct invsens_ioctl_s_accel_selftest_t {
	int result;
	int accel_ratio[INVSENS_AXIS_NUM];
};

struct invsens_ioctl_s_gyro_selftest_t {
	int result;
	long gyro_bias[INVSENS_AXIS_NUM];
	unsigned long gyro_rms[INVSENS_AXIS_NUM];
	int gyro_ratio[INVSENS_AXIS_NUM];
	int packet_cnt;
};

struct invsens_ioctl_accel_raw_t {
	short raw[INVSENS_AXIS_NUM];
};

struct invsens_ioctl_accel_bias_t {
	short bias[INVSENS_AXIS_NUM];
};

struct invsens_ioctl_gyro_raw_t {
	short raw[INVSENS_AXIS_NUM];
};

struct invsens_ioctl_wdgb_t {
	long bias[INVSENS_AXIS_NUM];
};

struct invsens_ioctl_wdab_t {
	long bias[INVSENS_AXIS_NUM];
};

struct invsens_ioctl_temperature_t {
	int temp;
};


struct invsens_ioctl_param_t {
	union  {
		struct invsense_ioctl_swst_gyro_t swst_gyro;
		struct invsense_ioctl_hwst_accel_t hwst_accel;
		struct invsense_ioctl_hwst_gyro_t hwst_gyro;
		struct invsens_ioctl_s_gyro_selftest_t s_gyro_selftest;
		struct invsens_ioctl_s_accel_selftest_t s_accel_selftest;

		struct invsens_ioctl_gyro_raw_t gyro_raw;
		struct invsens_ioctl_accel_raw_t accel_raw;
		struct invsens_ioctl_accel_bias_t accel_bias;
		struct invsens_ioctl_temperature_t temperature;

		struct invsens_ioctl_wdgb_t wdgb;
		struct invsens_ioctl_wdab_t wdab;

		u8 data[16];
	};
};


#define SM_DELAY_DEFAULT (200000000L)	/*200ms*/
#define SM_DELAY_FTMODE (66700000L)	/*667ms*/

struct invsens_board_cfg_t {
	u16 i2c_addr;
	struct invsens_i2c_t *i2c_handle;
	struct invsens_platform_data_t *platform_data;
};

struct invsens_data_t {
	u16 hdr;	/* func_type of data */
	u8 length;
	u8 data[INVSENS_DATA_BUFFER_MAX];
};

struct invsens_data_list_t {
	u32 enable_mask;
	int count;
	s64 timestamp;
	u8 *copy_buffer;
	u16 fifo_data_length;
	bool is_fifo_data_copied;
	bool request_fifo_reset;
	bool event_motion_interrupt_notified;
	struct invsens_data_t items[INVSENS_DATA_ITEM_MAX];
};

struct invsens_sm_data_t {
	struct invsens_board_cfg_t board_cfg;
	void *user_data;
};

struct invsens_sm_ctrl_t {
	u32 enable_mask;
	u32 pysical_mask;
	long delays[INV_FUNC_NUM];

	bool is_dmp_on;
	bool is_bypass_on;

	int start_addr;
	int dmp_packet_size;

	bool has_compass;
};

struct invsens_driver_t {
	u8 driver_id;
	u8 driver_layer;

	bool is_activated;

	int (*init) (struct invsens_driver_t *drv);
	int (*sync) (struct invsens_driver_t *drv, const char *key);
	int (*enable) (struct invsens_driver_t *drv, u32 func_mask,
			   bool enabled, struct invsens_sm_ctrl_t *sm_ctrl);
	int (*set_delay) (struct invsens_driver_t *drv, u32 func_mask,
			  long delay);
	int (*read) (struct invsens_driver_t *drv,
			 struct invsens_data_list_t *buffer);
	int (*batch) (struct invsens_driver_t *drv, u32 func_mask,
			  long delay, int timeout);
	int (*flush) (struct invsens_driver_t *drv, u32 func_mask);
	int (*suspend) (struct invsens_driver_t * drv);
	int (*resume) (struct invsens_driver_t * drv);
	int (*terminate) (struct invsens_driver_t *drv);
	int (*ioctl) (struct invsens_driver_t *drv, u32 cmd, long lparam,
			  void *vparam);
	int func_mask;

	void *user_data;
};



#define INVSENS_DRV_USER_DATA(_handle_) \
		(_handle_->user_data)


int invsens_sm_init(struct invsens_sm_data_t *sm);
int invsens_sm_term(struct invsens_sm_data_t *sm);
int invsens_sm_suspend(struct invsens_sm_data_t *sm);
int invsens_sm_resume(struct invsens_sm_data_t *sm);
int invsens_sm_enable_sensor(struct invsens_sm_data_t *sm, u32 func,
	bool enable, long delay);
int invsens_sm_set_delay(struct invsens_sm_data_t *sm, u32 func,
	long delay);
int invsens_sm_batch(struct invsens_sm_data_t *sm, u32 func, long period,
	long timeout);
int invsens_sm_flush(struct invsens_sm_data_t *sm, u32 func);
int invsens_sm_sync(struct invsens_sm_data_t *sm, const char *key);
int invsens_sm_read(struct invsens_sm_data_t *sm,

struct invsens_data_list_t *data_list);

int invsens_sm_ioctl(struct invsens_sm_data_t *sm, u32 cmd,
	long lparam, void *vparam);
int invsens_sm_get_enabled_mask(struct invsens_sm_data_t *sm,
	u32 *enabled_mask);
bool invsens_sm_is_func_enabled(struct invsens_sm_data_t *sm, int func);
#endif	/* _INV_SENSORS_SM_H_ */
