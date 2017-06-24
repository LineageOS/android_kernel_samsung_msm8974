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
#include "ssp.h"
#include <linux/math64.h>
#include <linux/sched.h>

/* SSP -> AP Instruction */
#define MSG2AP_INST_BYPASS_DATA			0x37
#define MSG2AP_INST_LIBRARY_DATA		0x01
#define MSG2AP_INST_DEBUG_DATA			0x03
#define MSG2AP_INST_BIG_DATA			0x04
#define MSG2AP_INST_META_DATA			0x05
#define MSG2AP_INST_TIME_SYNC			0x06

/*************************************************************************/
/* SSP parsing the dataframe                                             */
/*************************************************************************/

static void generate_data(struct ssp_data *data, struct sensor_value *sensorsdata,
						int iSensorData, u64 timestamp)
{
	u64 move_timestamp = data->lastTimestamp[iSensorData];
	if ((iSensorData != STEP_DETECTOR) && (iSensorData != STEP_COUNTER)) {
		while ((move_timestamp * 10 + data->adDelayBuf[iSensorData] * 15) < (timestamp * 10)) {
			move_timestamp += data->adDelayBuf[iSensorData];
			sensorsdata->timestamp = move_timestamp;
			data->report_sensor_data[iSensorData](data, sensorsdata);
		}
	}
}

static void get_timestamp(struct ssp_data *data, char *pchRcvDataFrame,
		int *iDataIdx, struct sensor_value *sensorsdata,
		struct ssp_time_diff *sensortime, int iSensorData)
{
	if (sensortime->batch_mode == BATCH_MODE_RUN) {
		if (sensortime->batch_count == sensortime->batch_count_fixed) {
			if (sensortime->time_diff == data->adDelayBuf[iSensorData]) {
				generate_data(data, sensorsdata, iSensorData,
						(data->timestamp - data->adDelayBuf[iSensorData] * (sensortime->batch_count_fixed - 1)));
			}
			sensorsdata->timestamp = data->timestamp - ((sensortime->batch_count - 1) * sensortime->time_diff);
		} else {
			if (sensortime->batch_count > 1)
				sensorsdata->timestamp = data->timestamp - ((sensortime->batch_count - 1) * sensortime->time_diff);
			else
				sensorsdata->timestamp = data->timestamp;
		}
	} else {
		if (((sensortime->irq_diff * 10) > (data->adDelayBuf[iSensorData] * 18))
			&& ((sensortime->irq_diff * 10) < (data->adDelayBuf[iSensorData] * 100))) {
			generate_data(data, sensorsdata, iSensorData, data->timestamp);
		}
		sensorsdata->timestamp = data->timestamp;
	}
	*iDataIdx += 4;
}

static void get_3axis_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 6);
	*iDataIdx += 6;
}

static void get_geomagnetic_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 7);
	*iDataIdx += 7;
}

static void get_uncalib_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 12);
	*iDataIdx += 12;
}

static void get_rot_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 17);
	*iDataIdx += 17;
}

static void get_step_det_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}

static void get_light_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{

#if defined (CONFIG_SENSORS_SSP_MAX88921)
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 12);
	*iDataIdx += 12;
#else
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 8);
	*iDataIdx += 8;
#endif

}

static void get_pressure_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	s16 temperature = 0;
	memcpy(&sensorsdata->pressure[0], pchRcvDataFrame + *iDataIdx, 4);
	memcpy(&temperature, pchRcvDataFrame + *iDataIdx + 4, 2);
	sensorsdata->pressure[1] = temperature;
	*iDataIdx += 6;
}

static void get_gesture_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
#if defined (CONFIG_SENSORS_SSP_MAX88920)
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 18);
	*iDataIdx += 18;
#else
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 38);
	*iDataIdx += 38;
#endif
}

static void get_proximity_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memset(&sensorsdata->prox[0], 0, 2);
#if defined (CONFIG_SENSORS_SSP_MAX88920)
	memcpy(&sensorsdata->prox[0], pchRcvDataFrame + *iDataIdx, 2);
	//memcpy(&sensorsdata->prox[1], pchRcvDataFrame + *iDataIdx + 1, 1);
	*iDataIdx += 2;
#else
	memcpy(&sensorsdata->prox[0], pchRcvDataFrame + *iDataIdx, 1);
	memcpy(&sensorsdata->prox[1], pchRcvDataFrame + *iDataIdx + 1, 2);
	*iDataIdx += 3;
#endif
}

static void get_proximity_rawdata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
#if defined (CONFIG_SENSORS_SSP_MAX88920)
	memcpy(&sensorsdata->prox[0], pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
#else
	memcpy(&sensorsdata->prox[0], pchRcvDataFrame + *iDataIdx, 2);
	*iDataIdx += 2;
#endif
}

static void get_geomagnetic_rawdata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 6);
	*iDataIdx += 6;
}

static void get_temp_humidity_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memset(&sensorsdata->data[2], 0, 2);
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 5);
	*iDataIdx += 5;
}

static void get_sig_motion_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(sensorsdata, pchRcvDataFrame + *iDataIdx, 1);
	*iDataIdx += 1;
}

static void get_step_cnt_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	memcpy(&sensorsdata->step_diff, pchRcvDataFrame + *iDataIdx, 4);
	*iDataIdx += 4;
}

int handle_big_data(struct ssp_data *data, char *pchRcvDataFrame, int *pDataIdx) {
	u8 bigType = 0;
	struct ssp_big *big = kzalloc(sizeof(*big), GFP_KERNEL);
	big->data = data;
	bigType = pchRcvDataFrame[(*pDataIdx)++];
	memcpy(&big->length, pchRcvDataFrame + *pDataIdx, 4);
	*pDataIdx += 4;
	memcpy(&big->addr, pchRcvDataFrame + *pDataIdx, 4);
	*pDataIdx += 4;

	if (bigType >= BIG_TYPE_MAX) {
		kfree(big);
		return FAIL;
	}

	INIT_WORK(&big->work, data->ssp_big_task[bigType] );
	queue_work(data->debug_wq, &big->work);
	return SUCCESS;
}

int parse_dataframe(struct ssp_data *data, char *pchRcvDataFrame, int iLength) {
	int iDataIdx, iSensorData;
	u16 length = 0;
	struct sensor_value sensorsdata;
	struct ssp_time_diff sensortime;

	for (iDataIdx = 0; iDataIdx < iLength;) {
		switch (pchRcvDataFrame[iDataIdx++]) {
		case MSG2AP_INST_BYPASS_DATA:
			iSensorData = pchRcvDataFrame[iDataIdx++];
			if ((iSensorData < 0) || (iSensorData >= SENSOR_MAX)) {
				pr_err("[SSP]: %s - Mcu data frame1 error %d\n", __func__,
						iSensorData);
				return ERROR;
			}
			memcpy(&length, pchRcvDataFrame + iDataIdx, 2);
			iDataIdx += 2;
			sensortime.batch_count = sensortime.batch_count_fixed = length;
			sensortime.batch_mode = length > 1 ? BATCH_MODE_RUN : BATCH_MODE_NONE;
			sensortime.irq_diff = data->timestamp - data->lastTimestamp[iSensorData];

			if (sensortime.batch_mode == BATCH_MODE_RUN) {
				if (data->reportedData[iSensorData] == true) {
					u64 time;
					sensortime.time_diff = div64_long((s64)(data->timestamp - data->lastTimestamp[iSensorData]), (s64)length);
					if (length > 8)
						time = data->adDelayBuf[iSensorData] * 18;
					else if (length > 4)
						time = data->adDelayBuf[iSensorData] * 25;
					else if (length > 2)
						time = data->adDelayBuf[iSensorData] * 50;
					else
						time = data->adDelayBuf[iSensorData] * 100;
					if ((sensortime.time_diff * 10) > time) {
						data->lastTimestamp[iSensorData] = data->timestamp - (data->adDelayBuf[iSensorData] * length);
						sensortime.time_diff = data->adDelayBuf[iSensorData];
					} else {
						time = data->adDelayBuf[iSensorData] * 18;
						if ((sensortime.time_diff * 10) > time)
							sensortime.time_diff = data->adDelayBuf[iSensorData];
					}
				} else {
					if (data->lastTimestamp[iSensorData] < (data->timestamp - (data->adDelayBuf[iSensorData] * length))) {
						data->lastTimestamp[iSensorData] = data->timestamp - (data->adDelayBuf[iSensorData] * length);
						sensortime.time_diff = data->adDelayBuf[iSensorData];
					} else
						sensortime.time_diff = div64_long((s64)(data->timestamp - data->lastTimestamp[iSensorData]), (s64)length);
				}
			} else {
				if (data->reportedData[iSensorData] == false)
					sensortime.irq_diff = data->adDelayBuf[iSensorData];
			}

			do {
				data->get_sensor_data[iSensorData](pchRcvDataFrame, &iDataIdx,
						&sensorsdata);

				get_timestamp(data, pchRcvDataFrame, &iDataIdx, &sensorsdata, &sensortime, iSensorData);
				if (sensortime.irq_diff > 1000000)
					data->report_sensor_data[iSensorData](data, &sensorsdata);
				else if ((iSensorData == PROXIMITY_SENSOR) || (iSensorData == PROXIMITY_RAW)
						|| (iSensorData == GESTURE_SENSOR) || (iSensorData == SIG_MOTION_SENSOR)
						|| (iSensorData == STEP_DETECTOR) || (iSensorData == STEP_COUNTER))
					data->report_sensor_data[iSensorData](data, &sensorsdata);
				else
					pr_err("[SSP]: %s irq_diff is under 1msec (%d)\n", __func__, iSensorData);
				sensortime.batch_count--;
			} while ((sensortime.batch_count > 0) && (iDataIdx < iLength));

			if (sensortime.batch_count > 0)
				pr_err("[SSP]: %s batch count error (%d)\n", __func__, sensortime.batch_count);

			data->lastTimestamp[iSensorData] = data->timestamp;
			data->reportedData[iSensorData] = true;
			break;
		case MSG2AP_INST_DEBUG_DATA:
			iSensorData = print_mcu_debug(pchRcvDataFrame, &iDataIdx, iLength);
			if (iSensorData) {
				pr_err("[SSP]: %s - Mcu data frame3 error %d\n", __func__,
						iSensorData);
				return ERROR;
			}
			break;
		case MSG2AP_INST_LIBRARY_DATA:
			memcpy(&length, pchRcvDataFrame + iDataIdx, 2);
			iDataIdx += 2;
			ssp_sensorhub_handle_data(data, pchRcvDataFrame, iDataIdx,
					iDataIdx + length);
			iDataIdx += length;
			break;
		case MSG2AP_INST_BIG_DATA:
			handle_big_data(data, pchRcvDataFrame, &iDataIdx);
			break;
		case MSG2AP_INST_META_DATA:
			sensorsdata.meta_data.what = pchRcvDataFrame[iDataIdx++];
			sensorsdata.meta_data.sensor = pchRcvDataFrame[iDataIdx++];
			report_meta_data(data, &sensorsdata);
			break;
		case MSG2AP_INST_TIME_SYNC:
			data->bTimeSyncing = true;
			break;
		}
	}

	return SUCCESS;
}

void initialize_function_pointer(struct ssp_data *data)
{
	data->get_sensor_data[ACCELEROMETER_SENSOR] = get_3axis_sensordata;
	data->get_sensor_data[GYROSCOPE_SENSOR] = get_3axis_sensordata;
	data->get_sensor_data[GEOMAGNETIC_SENSOR] = get_geomagnetic_sensordata;
	data->get_sensor_data[GEOMAGNETIC_UNCALIB_SENSOR] = get_uncalib_sensordata;
	data->get_sensor_data[PRESSURE_SENSOR] = get_pressure_sensordata;
	data->get_sensor_data[GESTURE_SENSOR] = get_gesture_sensordata;
	data->get_sensor_data[PROXIMITY_SENSOR] = get_proximity_sensordata;
	data->get_sensor_data[PROXIMITY_RAW] = get_proximity_rawdata;
	data->get_sensor_data[LIGHT_SENSOR] = get_light_sensordata;
	data->get_sensor_data[TEMPERATURE_HUMIDITY_SENSOR] =
		get_temp_humidity_sensordata;
	data->get_sensor_data[GEOMAGNETIC_RAW] = get_geomagnetic_rawdata;
	data->get_sensor_data[ROTATION_VECTOR] = get_rot_sensordata;
	data->get_sensor_data[GAME_ROTATION_VECTOR] = get_rot_sensordata;
	data->get_sensor_data[STEP_DETECTOR] = get_step_det_sensordata;
	data->get_sensor_data[SIG_MOTION_SENSOR] = get_sig_motion_sensordata;
	data->get_sensor_data[GYRO_UNCALIB_SENSOR] = get_uncalib_sensordata;
	data->get_sensor_data[STEP_COUNTER] = get_step_cnt_sensordata;

	data->report_sensor_data[ACCELEROMETER_SENSOR] = report_acc_data;
	data->report_sensor_data[GYROSCOPE_SENSOR] = report_gyro_data;
	data->report_sensor_data[GEOMAGNETIC_SENSOR] = report_mag_data;
	data->report_sensor_data[GEOMAGNETIC_UNCALIB_SENSOR] = report_uncalib_mag_data;
	data->report_sensor_data[PRESSURE_SENSOR] = report_pressure_data;
	data->report_sensor_data[GESTURE_SENSOR] = report_gesture_data;
	data->report_sensor_data[PROXIMITY_SENSOR] = report_prox_data;
	data->report_sensor_data[PROXIMITY_RAW] = report_prox_raw_data;
	data->report_sensor_data[LIGHT_SENSOR] = report_light_data;
	data->report_sensor_data[TEMPERATURE_HUMIDITY_SENSOR] =
		report_temp_humidity_data;
	data->report_sensor_data[GEOMAGNETIC_RAW] = report_geomagnetic_raw_data;
	data->report_sensor_data[ROTATION_VECTOR] = report_rot_data;
	data->report_sensor_data[GAME_ROTATION_VECTOR] = report_game_rot_data;
	data->report_sensor_data[STEP_DETECTOR] = report_step_det_data;
	data->report_sensor_data[SIG_MOTION_SENSOR] = report_sig_motion_data;
	data->report_sensor_data[GYRO_UNCALIB_SENSOR] = report_uncalib_gyro_data;
	data->report_sensor_data[STEP_COUNTER] = report_step_cnt_data;

	data->ssp_big_task[BIG_TYPE_DUMP] = ssp_dump_task;
	data->ssp_big_task[BIG_TYPE_READ_LIB] = ssp_read_big_library_task;
	data->ssp_big_task[BIG_TYPE_VOICE_NET] = ssp_send_big_library_task;
	data->ssp_big_task[BIG_TYPE_VOICE_GRAM] = ssp_send_big_library_task;
	data->ssp_big_task[BIG_TYPE_VOICE_PCM] = ssp_pcm_dump_task;
	data->ssp_big_task[BIG_TYPE_TEMP] = ssp_temp_task;
}
