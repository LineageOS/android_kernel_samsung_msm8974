/*
*  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
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
#ifndef __ADSP_SENSOR_H__
#define __ADSP_SENSOR_H__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define PID 20000
#define NETLINK_ADSP_FAC 22

#define PROX_READ_NUM	10
#define SNS_REG_DATA_FILENAME "/persist/sensors/sns.reg"

/* ENUMS for Selecting the current sensor being used */
enum{
	ADSP_FACTORY_MODE_ACCEL,
	ADSP_FACTORY_MODE_GYRO,
	ADSP_FACTORY_MODE_MAG,
	ADSP_FACTORY_MODE_LIGHT,
	ADSP_FACTORY_MODE_PROX,
	ADSP_FACTORY_MODE_MAG_POWERNOISE_ON,
	ADSP_FACTORY_MODE_MAG_POWERNOISE_OFF,
	ADSP_FACTORY_SENSOR_MAX
};

enum {
  ADSP_RAW_ACCEL              = (1 << 0), /* 0b0000000000000001 */
  ADSP_RAW_GYRO               = (1 << 1), /* 0b0000000000000010 */
  ADSP_RAW_MAG                 = (1 << 2), /* 0b0000000000000100 */
  ADSP_RAW_LIGHT               = (1 << 3), /* 0b0000000000001000 */
  ADSP_RAW_PROXY              = (1 << 4), /* 0b0000000000010000 */
  ADSP_RAW_PRESSURE         = (1 << 5), /* 0b0000000000100000 */
  ADSP_RAW_MAX                  = (1 << 8), /* 0b0000000100000000 */
};

enum {
  ADSP_DATA_NONE              = (1 << 0), /* 0b0000000000000001 */
  ADSP_DATA_GYRO_TEMP    = (1 << 1), /* 0b0000000000000010 */
};

//Call back command
enum{
	CALLBACK_REGISTER_SUCCESS,
	CALLBACK_TIMER_EXPIRED
};
enum {
	NETLINK_ATTR_SENSOR_TYPE,
	NETLINK_ATTR_MAX
};

/* Netlink ENUMS Message Protocols */
enum {
	NETLINK_MESSAGE_GET_RAW_DATA,
	NETLINK_MESSAGE_RAW_DATA_RCVD,
	NETLINK_MESSAGE_GET_CALIB_DATA,
	NETLINK_MESSAGE_CALIB_DATA_RCVD,
	NETLINK_MESSAGE_CALIB_STORE_DATA,
	NETLINK_MESSAGE_CALIB_STORE_RCVD,
	NETLINK_MESSAGE_SELFTEST_SHOW_DATA,
	NETLINK_MESSAGE_SELFTEST_SHOW_RCVD,
	NETLINK_MESSAGE_REACTIVE_ALERT_DATA,
	NETLINK_MESSAGE_REACTIVE_ALERT_RCVD,
	NETLINK_MESSAGE_STOP_RAW_DATA,
	NETLINK_MESSAGE_GYRO_TEMP,
	NETLINK_MESSAGE_GYRO_TEMP_RCVD,
	NETLINK_MESSAGE_MAG_POWERNOISE_ON,
	NETLINK_MESSAGE_MAG_POWERNOISE_OFF,
	NETLINK_MESSAGE_MAX
};

struct msg_data{
	int sensor_type;
	int param1;//For passing extra parameter
	int param2;//For passing extra parameter
	int param3;//For passing extra parameter
};

struct sensor_value {
	unsigned int sensor_type;
	union{
		struct {
			s16 x;
			s16 y;
			s16 z;
		};
		s16 reactive_alert;
		s16 temperature;
	};
};

struct sensor_stop_value {
  unsigned int sensor_type;
  int result;
};

/* Structs used in calibration show and store */
struct sensor_calib_value {
	unsigned int sensor_type;
	union {
		struct {
			int x;
			int y;
			int z;
		};
	};
	int result;
};

struct sensor_calib_store_result{
	unsigned int sensor_type;
	int nCalibstoreresult;
};

struct trans_value{
	unsigned int sensor_type;
};

/* Struct used for selftest */
struct sensor_selftest_show_result{
	unsigned int sensor_type;
	int nSelftestresult1;
	int nSelftestresult2;
	//Noice bias
	int bias_x;
	int bias_y;
	int bias_z;
	//Noice power
	int rms_x;
	int rms_y;
	int rms_z;

	//H/W selftest %
	int ratio_x;
	int ratio_x_dec;
	int ratio_y;
	int ratio_y_dec;
	int ratio_z;
	int ratio_z_dec;
	//H/W packet count  %
	int st_x;
	int st_y;
	int st_z;
	/* FOR YAS532 */
	/* DEV_ID */
	int id;
	/* DIRECTION */
	int dir;
	/* OFFSET */
	int offset_x;
	int offset_y;
	int offset_z;
	/* SENSITIVITY */
	int sx;
	int sy;
	/* OFFSET H */
	int ohx;
	int ohy;
	int ohz;
};

/* Main struct containing all the data */
struct adsp_data {
	struct device *adsp;
	struct device *sensor_device[ADSP_FACTORY_SENSOR_MAX];
	struct sensor_value sensor_data[ADSP_FACTORY_SENSOR_MAX];
	struct sensor_calib_value sensor_calib_data[ADSP_FACTORY_SENSOR_MAX];
	struct sensor_calib_store_result sensor_calib_result[ADSP_FACTORY_SENSOR_MAX];
	struct sensor_selftest_show_result sensor_selftest_result[ADSP_FACTORY_SENSOR_MAX];
    struct sensor_stop_value sensor_stop_data[ADSP_FACTORY_SENSOR_MAX];
	struct adsp_fac_ctl *ctl[ADSP_FACTORY_SENSOR_MAX];
	uint8_t reactive_alert;
	uint8_t gyro_temp;
	struct sock		*adsp_skt;
	unsigned int alert_ready_flag;
	unsigned int data_ready_flag;
	unsigned int calib_ready_flag;
	unsigned int calib_store_ready_flag;
	unsigned int selftest_ready_flag;
	unsigned int data_ready;
	struct timer_list	 command_timer;
	int prox_average[PROX_READ_NUM];
};

struct adsp_fac_ctl {
	int (*init_fnc) (struct adsp_data *data);
	int (*exit_fnc) (struct adsp_data *data);
	int (*receive_data_fnc) (struct adsp_data *data, int cmd );
};

int adsp_factory_register( char *dev_name,int type, struct device_attribute *attributes[],struct adsp_fac_ctl *fact_ctl );
int adsp_unicast( struct msg_data param , int message, int flags, u32 pid);
int adsp_factory_start_timer( const unsigned int ms );
extern struct mutex factory_mutex;
#endif
