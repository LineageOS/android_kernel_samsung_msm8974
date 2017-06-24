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

/* SSP -> AP Instruction */
#define MSG2AP_INST_BYPASS_DATA			0x00
#define MSG2AP_INST_LIBRARY_DATA		0x01
#define MSG2AP_INST_SELFTEST_DATA		0x02
#define MSG2AP_INST_DEBUG_DATA			0x03

/* Factory data length */
#define ACCEL_FACTORY_DATA_LENGTH		1
#define GYRO_FACTORY_DATA_LENGTH		36
#define MAGNETIC_FACTORY_DATA_LENGTH		26
#define PRESSURE_FACTORY_DATA_LENGTH		1
#define MCU_FACTORY_DATA_LENGTH			5
#define	GYRO_TEMP_FACTORY_DATA_LENGTH		2
#define	GYRO_DPS_FACTORY_DATA_LENGTH		1
#define TEMPHUMIDITY_FACTORY_DATA_LENGTH	1
#define MCU_SLEEP_FACTORY_DATA_LENGTH		FACTORY_DATA_MAX
#define GESTURE_FACTORY_DATA_LENGTH     4

static void get_timestamp(struct ssp_data *data, int iSensorData,
	struct sensor_value *sensorsdata, struct ssp_time_diff *sensortime)
{
	if ((iSensorData == PROXIMITY_SENSOR) || (iSensorData == GESTURE_SENSOR)
		|| (iSensorData == STEP_DETECTOR) || (iSensorData == SIG_MOTION_SENSOR)
		|| (iSensorData == STEP_COUNTER)) {
		sensorsdata->timestamp = data->timestamp;
		return;
	}

	if (((sensortime->irq_diff * 10) >
		(data->adDelayBuf[iSensorData] * 18))
		&& ((sensortime->irq_diff * 10) <
		(data->adDelayBuf[iSensorData] * 100))) {
		u64 move_timestamp;
		u64 shift_timestamp =
			div64_long(data->adDelayBuf[iSensorData], 2);
		for (move_timestamp = data->lastTimestamp[iSensorData] +
			data->adDelayBuf[iSensorData];
			move_timestamp < (data->timestamp - shift_timestamp);
			move_timestamp += data->adDelayBuf[iSensorData]) {
			sensorsdata->timestamp = move_timestamp;
			data->report_sensor_data[iSensorData](data,
				sensorsdata);
		}
	}
	sensorsdata->timestamp = data->timestamp;
}


/*************************************************************************/
/* SSP parsing the dataframe                                             */
/*************************************************************************/

static void get_3axis_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	int iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->x = (s16)iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->y = (s16)iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->z = (s16)iTemp;
}

static void get_light_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	int iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->r = (u16)iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->g = (u16)iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->b = (u16)iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->w = (u16)iTemp;
}

static void get_pressure_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	int iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 16;
	sensorsdata->pressure[0] = iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	sensorsdata->pressure[0] += iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->pressure[0] += iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += (int)pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->pressure[1] = (s16)iTemp;
}

static void get_gesture_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	int iTemp;
	int i = 0;
	for (i=0; i<9; i++) {
		iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
		iTemp <<= 8;
		iTemp += pchRcvDataFrame[(*iDataIdx)++];
		sensorsdata->data[i] = (s16)iTemp;
	}
}

static void get_proximity_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	sensorsdata->prox[0] = (u8)pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->prox[1] = (u8)pchRcvDataFrame[(*iDataIdx)++];
}

static void get_proximity_rawdata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	sensorsdata->prox[0] = (u8)pchRcvDataFrame[(*iDataIdx)++];
}

static void get_geomagnetic_rawdata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	int iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->x = (s16)iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->y = (s16)iTemp;

	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->z = (s16)iTemp;

}

static void get_factory_data(struct ssp_data *data, int iSensorData,
	char *pchRcvDataFrame, int *iDataIdx)
{
	int iIdx, iTotalLenth = 0;
	unsigned int uTemp = 0;

	switch (iSensorData) {
	case ACCELEROMETER_FACTORY:
		uTemp = (1 << ACCELEROMETER_FACTORY);
		iTotalLenth = ACCEL_FACTORY_DATA_LENGTH;
		break;
	case GYROSCOPE_FACTORY:
		uTemp = (1 << GYROSCOPE_FACTORY);
		iTotalLenth = GYRO_FACTORY_DATA_LENGTH;
		break;
	case GEOMAGNETIC_FACTORY:
		uTemp = (1 << GEOMAGNETIC_FACTORY);
		iTotalLenth = MAGNETIC_FACTORY_DATA_LENGTH;
		break;
	case PRESSURE_FACTORY:
		uTemp = (1 << PRESSURE_FACTORY);
		iTotalLenth = PRESSURE_FACTORY_DATA_LENGTH;
		break;
	case MCU_FACTORY:
		uTemp = (1 << MCU_FACTORY);
		iTotalLenth = MCU_FACTORY_DATA_LENGTH;
		break;
	case GYROSCOPE_TEMP_FACTORY:
		uTemp = (1 << GYROSCOPE_TEMP_FACTORY);
		iTotalLenth = GYRO_TEMP_FACTORY_DATA_LENGTH;
		break;
	case GYROSCOPE_DPS_FACTORY:
		uTemp = (1 << GYROSCOPE_DPS_FACTORY);
		iTotalLenth = GYRO_DPS_FACTORY_DATA_LENGTH;
		break;
	case MCU_SLEEP_FACTORY:
		uTemp = (1 << MCU_SLEEP_FACTORY);
		iTotalLenth = MCU_SLEEP_FACTORY_DATA_LENGTH;
		break;
	case GESTURE_FACTORY:
		uTemp = (1 << GESTURE_FACTORY);
		iTotalLenth = GESTURE_FACTORY_DATA_LENGTH;
		break;
	case TEMPHUMIDITY_CRC_FACTORY:
		uTemp = (1 << TEMPHUMIDITY_CRC_FACTORY);
		iTotalLenth = TEMPHUMIDITY_FACTORY_DATA_LENGTH;
		break;
	}

	ssp_dbg("[SSP]: %s - Factory test data %d\n", __func__, iSensorData);
	for (iIdx = 0; iIdx < iTotalLenth; iIdx++)
		data->uFactorydata[iIdx] = (u8)pchRcvDataFrame[(*iDataIdx)++];

	data->uFactorydataReady = uTemp;
}

static void get_temp_humidity_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	int iTemp;

	/* Temperature */
	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->data[0] = (s16)iTemp;
	/* Humidity */
	iTemp = (int)pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;
	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->data[1] = (s16)iTemp;
	/* wakelock */
	sensorsdata->data[2] = (s16)pchRcvDataFrame[(*iDataIdx)++];


}

static void get_sig_motion_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	sensorsdata->sig_motion = (u8)pchRcvDataFrame[(*iDataIdx)++];
}

static void get_step_det_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	sensorsdata->step_det = (u8)pchRcvDataFrame[(*iDataIdx)++];
}

static void get_step_cnt_sensordata(char *pchRcvDataFrame, int *iDataIdx,
	struct sensor_value *sensorsdata)
{
	u32 iTemp = 0;

	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 24;

	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 16;

	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	iTemp <<= 8;

	iTemp += pchRcvDataFrame[(*iDataIdx)++];
	sensorsdata->step_diff = iTemp;
}

int parse_dataframe(struct ssp_data *data, char *pchRcvDataFrame, int iLength)
{
	int iDataIdx, iSensorData;
	struct sensor_value *sensorsdata;
	struct ssp_time_diff sensortime;

	sensorsdata = kzalloc(sizeof(*sensorsdata), GFP_KERNEL);
	if (sensorsdata == NULL)
		return ERROR;

	for (iDataIdx = 0; iDataIdx < iLength;) {
		if (pchRcvDataFrame[iDataIdx] == MSG2AP_INST_BYPASS_DATA) {
			iDataIdx++;
			iSensorData = pchRcvDataFrame[iDataIdx++];
			if ((iSensorData < 0) ||
				(iSensorData >= SENSOR_MAX)) {
				pr_err("[SSP]: %s - Mcu data frame1 error %d\n",
					__func__, iSensorData);
				kfree(sensorsdata);
				return ERROR;
			}
			sensortime.irq_diff = data->timestamp -
				data->lastTimestamp[iSensorData];
			data->get_sensor_data[iSensorData](pchRcvDataFrame,
				&iDataIdx, sensorsdata);
			get_timestamp(data, iSensorData, sensorsdata,
				&sensortime);

			if (sensortime.irq_diff > 1000000)
				data->report_sensor_data[iSensorData](data,
					sensorsdata);
			else if ((iSensorData == PROXIMITY_SENSOR) ||
				(iSensorData == PROXIMITY_RAW) ||
				(iSensorData == GESTURE_SENSOR) ||
				(iSensorData == SIG_MOTION_SENSOR) ||
				(iSensorData == STEP_DETECTOR) ||
				(iSensorData == STEP_COUNTER))
				data->report_sensor_data[iSensorData](data,
					sensorsdata);
			else
				pr_info("[SSP]: %s irq_diff is under 1ms(%d)\n",
					__func__, iSensorData);

			data->lastTimestamp[iSensorData] = data->timestamp;
		} else if (pchRcvDataFrame[iDataIdx] ==
			MSG2AP_INST_SELFTEST_DATA) {
			iDataIdx++;
			iSensorData = pchRcvDataFrame[iDataIdx++];
			if ((iSensorData < 0) ||
				(iSensorData >= SENSOR_FACTORY_MAX)) {
				pr_err("[SSP]: %s - Mcu data frame2 error %d\n",
					__func__, iSensorData);
				kfree(sensorsdata);
				return ERROR;
			}
			get_factory_data(data, iSensorData, pchRcvDataFrame,
				&iDataIdx);
		} else if (pchRcvDataFrame[iDataIdx] ==
			MSG2AP_INST_DEBUG_DATA) {
			iSensorData
				= print_mcu_debug(pchRcvDataFrame + iDataIdx+1,
						&iDataIdx, iLength);
			if (iSensorData) {
				pr_err("[SSP]: %s - Mcu data frame3 error %d\n",
					__func__, iSensorData);
				kfree(sensorsdata);
				return ERROR;
			}
#ifdef CONFIG_SENSORS_SSP_SENSORHUB
		} else if (pchRcvDataFrame[iDataIdx] ==
			MSG2AP_INST_LIBRARY_DATA) {
			ssp_sensorhub_handle_data(data,
					pchRcvDataFrame, iDataIdx, iLength);
			break;
#endif
		} else
			iDataIdx++;
	}
	kfree(sensorsdata);
	return SUCCESS;
}

void initialize_function_pointer(struct ssp_data *data)
{
	data->get_sensor_data[ACCELEROMETER_SENSOR] = get_3axis_sensordata;
	data->get_sensor_data[GYROSCOPE_SENSOR] = get_3axis_sensordata;
	data->get_sensor_data[GEOMAGNETIC_SENSOR] = get_3axis_sensordata;
	data->get_sensor_data[PRESSURE_SENSOR] = get_pressure_sensordata;
	data->get_sensor_data[GESTURE_SENSOR] = get_gesture_sensordata;
	data->get_sensor_data[PROXIMITY_SENSOR] = get_proximity_sensordata;
	data->get_sensor_data[PROXIMITY_RAW] = get_proximity_rawdata;
	data->get_sensor_data[LIGHT_SENSOR] = get_light_sensordata;
	data->get_sensor_data[TEMPERATURE_HUMIDITY_SENSOR] =
		get_temp_humidity_sensordata;
	data->get_sensor_data[GEOMAGNETIC_RAW] = get_geomagnetic_rawdata;
	data->get_sensor_data[SIG_MOTION_SENSOR] = get_sig_motion_sensordata;
	data->get_sensor_data[STEP_DETECTOR] = get_step_det_sensordata;
	data->get_sensor_data[STEP_COUNTER] = get_step_cnt_sensordata;

	data->report_sensor_data[ACCELEROMETER_SENSOR] = report_acc_data;
	data->report_sensor_data[GYROSCOPE_SENSOR] = report_gyro_data;
	data->report_sensor_data[GEOMAGNETIC_SENSOR] = report_mag_data;
	data->report_sensor_data[PRESSURE_SENSOR] = report_pressure_data;
	data->report_sensor_data[GESTURE_SENSOR] = report_gesture_data;
	data->report_sensor_data[PROXIMITY_SENSOR] = report_prox_data;
	data->report_sensor_data[PROXIMITY_RAW] = report_prox_raw_data;
	data->report_sensor_data[LIGHT_SENSOR] = report_light_data;
	data->report_sensor_data[TEMPERATURE_HUMIDITY_SENSOR] =
		report_temp_humidity_data;
	data->report_sensor_data[GEOMAGNETIC_RAW] = report_geomagnetic_raw_data;
	data->report_sensor_data[SIG_MOTION_SENSOR] = report_sig_motion_data;
	data->report_sensor_data[STEP_DETECTOR] = report_step_det_data;
	data->report_sensor_data[STEP_COUNTER] = report_step_cnt_data;
}
