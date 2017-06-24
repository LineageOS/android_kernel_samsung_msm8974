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
#include <linux/fs.h>
#include <mach/sec_debug.h>



#define SSP_DEBUG_TIMER_SEC		(10 * HZ)

#define LIMIT_RESET_CNT		3000000
#define LIMIT_SSD_FAIL_CNT		3
#define LIMIT_INSTRUCTION_FAIL_CNT	1
#define LIMIT_IRQ_FAIL_CNT		2
#define LIMIT_TIMEOUT_CNT		2

#define DUMP_FILE_PATH "/data/log/MCU_DUMP"

void ssp_dump_task(struct work_struct *work) {
	struct ssp_big *big;
	struct file *dump_file;
	struct ssp_msg *msg;
	char *buffer;
	char strFilePath[60];
	struct timeval cur_time;
	int iTimeTemp;
	mm_segment_t fs;
	int buf_len, packet_len, residue, iRet = 0, index = 0 ,iRetTrans=0 ,iRetWrite=0;
	

	big = container_of(work, struct ssp_big, work);
	pr_err("[SSP]: %s - start ssp dumping (%d)(%d)\n", __func__,big->data->bMcuDumpMode,big->data->uDumpCnt);
	big->data->uDumpCnt++;
	wake_lock(&big->data->ssp_wake_lock);


	fs = get_fs();
	set_fs(get_ds());

	if(big->data->bMcuDumpMode == true)
	{
		do_gettimeofday(&cur_time);
		iTimeTemp = (int) cur_time.tv_sec;

		sprintf(strFilePath, "%s%d.txt", DUMP_FILE_PATH, iTimeTemp);

		dump_file = filp_open(strFilePath, O_RDWR | O_CREAT | O_APPEND, 0666);
		if (IS_ERR(dump_file)) {
			pr_err("[SSP]: %s - Can't open dump file\n", __func__);
			set_fs(fs);
			iRet = PTR_ERR(dump_file);
			wake_unlock(&big->data->ssp_wake_lock);
			return;
		}
	}
	else
		dump_file = NULL;


	buf_len = big->length > DATA_PACKET_SIZE ? DATA_PACKET_SIZE : big->length;
	buffer = kzalloc(buf_len, GFP_KERNEL);
	residue = big->length;

	while (residue > 0) {
		packet_len = residue > DATA_PACKET_SIZE ? DATA_PACKET_SIZE : residue;

		msg = kzalloc(sizeof(*msg), GFP_KERNEL);
		msg->cmd = MSG2SSP_AP_GET_BIG_DATA;
		msg->length = packet_len;
		msg->options = AP2HUB_READ | (index++ << SSP_INDEX);
		msg->data = big->addr;
		msg->buffer = buffer;
		msg->free_buffer = 0;

		iRetTrans = ssp_spi_sync(big->data, msg, 1000);
		if (iRetTrans != SUCCESS) {
			pr_err("[SSP]: %s - Fail to receive data %d (%d)\n", __func__, iRetTrans,residue);
			break;
		}
			if(big->data->bMcuDumpMode == true)
			{
				iRetWrite = vfs_write(dump_file, (char __user *) buffer, packet_len,
					&dump_file->f_pos);
				if (iRetWrite < 0) {
					pr_err("[SSP]: %s - Can't write dump to file\n", __func__);
					break;
				}
		}
		
		residue -= packet_len;
	}


	if(big->data->bMcuDumpMode == true && (iRetTrans != SUCCESS || iRetWrite < 0) )
	{
		char FAILSTRING[100];
		sprintf(FAILSTRING,"FAIL OCCURED(%d)(%d)(%d)",iRetTrans,iRetWrite,big->length);
		vfs_write(dump_file, (char __user *) FAILSTRING, strlen(FAILSTRING),&dump_file->f_pos);
	}

	ssp_send_cmd(big->data, MSG2SSP_AP_MCU_DUMP_FINISH, SUCCESS);

	big->data->bDumping = false;
	if(big->data->bMcuDumpMode == true)
	{
		filp_close(dump_file, current->files);
	}
	set_fs(fs);
	sanity_check(big->data);
	
	wake_unlock(&big->data->ssp_wake_lock);
	kfree(buffer);
	kfree(big);

	pr_err("[SSP]: %s done\n", __func__);
}

void ssp_temp_task(struct work_struct *work) {
	struct ssp_big *big;
	struct ssp_msg *msg;
	char *buffer;
	int buf_len, packet_len, residue, iRet = 0, index = 0, i = 0, buffindex = 0;

	big = container_of(work, struct ssp_big, work);
	buf_len = big->length > DATA_PACKET_SIZE ? DATA_PACKET_SIZE : big->length;
	buffer = kzalloc(buf_len, GFP_KERNEL);
	residue = big->length;
	mutex_lock(&big->data->bulk_temp_read_lock);
	if (big->data->bulk_buffer == NULL)
		big->data->bulk_buffer = kzalloc(sizeof(struct shtc1_buffer),
				GFP_KERNEL);
	big->data->bulk_buffer->len = big->length / 12;

	while (residue > 0) {
		packet_len = residue > DATA_PACKET_SIZE ? DATA_PACKET_SIZE : residue;

		msg = kzalloc(sizeof(*msg), GFP_KERNEL);
		msg->cmd = MSG2SSP_AP_GET_BIG_DATA;
		msg->length = packet_len;
		msg->options = AP2HUB_READ | (index++ << SSP_INDEX);
		msg->data = big->addr;
		msg->buffer = buffer;
		msg->free_buffer = 0;

		iRet = ssp_spi_sync(big->data, msg, 1000);
		if (iRet != SUCCESS) {
			pr_err("[SSP]: %s - Fail to receive data %d\n", __func__, iRet);
			break;
		}
		// 12 = 1 chunk size for ks79.shin
		// order is thermistor Bat, thermistor PA, Temp, Humidity, Baro, Gyro
		// each data consist of 2bytes
		i = 0;
		while (packet_len - i >= 12) {
			ssp_dbg("[SSP]: %s %d %d %d %d %d %d", __func__,
					*((s16 *) (buffer + i + 0)), *((s16 *) (buffer + i + 2)),
					*((s16 *) (buffer + i + 4)), *((s16 *) (buffer + i + 6)),
					*((s16 *) (buffer + i + 8)), *((s16 *) (buffer +i + 10)));
			big->data->bulk_buffer->batt[buffindex] = *((u16 *) (buffer + i + 0));
			big->data->bulk_buffer->chg[buffindex] = *((u16 *) (buffer + i + 2));
			big->data->bulk_buffer->temp[buffindex] = *((s16 *) (buffer + i + 4));
			big->data->bulk_buffer->humidity[buffindex] = *((u16 *) (buffer + i + 6));
			big->data->bulk_buffer->baro[buffindex] = *((s16 *) (buffer + i + 8));
			big->data->bulk_buffer->gyro[buffindex] = *((s16 *) (buffer + i + 10));
			buffindex++;
			i += 12;
		}

		residue -= packet_len;
	}
	if (iRet == SUCCESS)
		report_bulk_comp_data(big->data);
	mutex_unlock(&big->data->bulk_temp_read_lock);
	kfree(buffer);
	kfree(big);
	ssp_dbg("[SSP]: %s done\n", __func__);
}

/*************************************************************************/
/* SSP Debug timer function                                              */
/*************************************************************************/

int print_mcu_debug(char *pchRcvDataFrame, int *pDataIdx,
		int iRcvDataFrameLength)
{
	int iLength = pchRcvDataFrame[(*pDataIdx)++];
	int cur = *pDataIdx;

	if (iLength > iRcvDataFrameLength - *pDataIdx || iLength <= 0) {
		ssp_dbg("[SSP]: MSG From MCU - invalid debug length(%d/%d/%d)\n",
			iLength, iRcvDataFrameLength, cur);
		return iLength ? iLength : ERROR;
	}

	ssp_dbg("[SSP]: MSG From MCU - %s\n", &pchRcvDataFrame[*pDataIdx]);
	*pDataIdx += iLength;
	return 0;
}

void reset_mcu(struct ssp_data *data)
{
	func_dbg();
	ssp_enable(data, false);

	clean_pending_list(data);
	toggle_mcu_reset(data);
	msleep(SSP_SW_RESET_TIME);
	ssp_enable(data, true);

	if (initialize_mcu(data) < 0)
		return;

	sync_sensor_state(data);

#ifdef CONFIG_SENSORS_SSP_SENSORHUB
	ssp_sensorhub_report_notice(data, MSG2SSP_AP_STATUS_RESET);
#endif
	if(data->uLastAPState!=0)
		ssp_send_cmd(data, data->uLastAPState, 0);
	if(data->uLastResumeState != 0)
		ssp_send_cmd(data, data->uLastResumeState, 0);
}

void sync_sensor_state(struct ssp_data *data)
{
	unsigned char uBuf[9] = {0,};
	unsigned int uSensorCnt;
	int iRet = 0;
	iRet = set_hw_offset(data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - set_hw_offset failed\n", __func__);
	}
	iRet = set_gyro_cal(data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - set_gyro_cal failed\n", __func__);
	}
	iRet = set_accel_cal(data);
	if (iRet < 0) {
		pr_err("[SSP]: %s - set_accel_cal failed\n", __func__);
	}
	udelay(10);

	for (uSensorCnt = 0; uSensorCnt < SENSOR_MAX; uSensorCnt++) {
		if (atomic_read(&data->aSensorEnable) & (1 << uSensorCnt)) {
			s32 dMsDelay = get_msdelay(data->adDelayBuf[uSensorCnt]);
			memcpy(&uBuf[0], &dMsDelay, 4);
			memcpy(&uBuf[4], &data->batchLatencyBuf[uSensorCnt], 4);
			uBuf[8] = data->batchOptBuf[uSensorCnt];
			send_instruction(data, ADD_SENSOR, uSensorCnt, uBuf, 9);
			udelay(10);
		}
	}

	if (data->bProximityRawEnabled == true) {
		s32 dMsDelay = 20;
		memcpy(&uBuf[0], &dMsDelay, 4);
		send_instruction(data, ADD_SENSOR, PROXIMITY_RAW, uBuf, 4);
	}

	set_proximity_threshold(data, data->uProxHiThresh,data->uProxLoThresh);


	data->bMcuDumpMode = sec_debug_is_enabled();
	iRet = ssp_send_cmd(data, MSG2SSP_AP_MCU_SET_DUMPMODE,data->bMcuDumpMode);
	if (iRet < 0) {
		pr_err("[SSP]: %s - MSG2SSP_AP_MCU_SET_DUMPMODE failed\n", __func__);
	}


}

static void print_sensordata(struct ssp_data *data, unsigned int uSensor)
{
	switch (uSensor) {
	case ACCELEROMETER_SENSOR:
	case GYROSCOPE_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].x, data->buf[uSensor].y,
			data->buf[uSensor].z,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case GEOMAGNETIC_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].cal_x, data->buf[uSensor].cal_y,
			data->buf[uSensor].cal_y, data->buf[uSensor].accuracy,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case GEOMAGNETIC_UNCALIB_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].uncal_x, data->buf[uSensor].uncal_y,
			data->buf[uSensor].uncal_z, data->buf[uSensor].offset_x,
			data->buf[uSensor].offset_y, data->buf[uSensor].offset_z,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case PRESSURE_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].pressure[0],
			data->buf[uSensor].pressure[1],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case GESTURE_SENSOR:
		ssp_dbg("[SSP] %u : %d %d %d %d (%ums)\n", uSensor,
			data->buf[uSensor].data[0], data->buf[uSensor].data[1],
			data->buf[uSensor].data[2], data->buf[uSensor].data[3],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case TEMPERATURE_HUMIDITY_SENSOR:
		ssp_dbg("[SSP] %u : %d %d %d(%ums)\n", uSensor,
			data->buf[uSensor].data[0], data->buf[uSensor].data[1],
			data->buf[uSensor].data[2], get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case LIGHT_SENSOR:
#if defined (CONFIG_SENSORS_SSP_MAX88921)
		ssp_dbg("[SSP] %u : %u, %u, %u, %u, %u, %u (%ums)\n", uSensor,
			data->buf[uSensor].r, data->buf[uSensor].g,
			data->buf[uSensor].b, data->buf[uSensor].w,
			data->buf[uSensor].ir_cmp, data->buf[uSensor].amb_pga,
			get_msdelay(data->adDelayBuf[uSensor]));
#else
		ssp_dbg("[SSP] %u : %u, %u, %u, %u (%ums)\n", uSensor,
			data->buf[uSensor].r, data->buf[uSensor].g,
			data->buf[uSensor].b, data->buf[uSensor].w,
			get_msdelay(data->adDelayBuf[uSensor]));
#endif
		break;
	case PROXIMITY_SENSOR:
		ssp_dbg("[SSP] %u : %d %d(%ums)\n", uSensor,
			data->buf[uSensor].prox[0], data->buf[uSensor].prox[1],
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case STEP_DETECTOR:
		ssp_dbg("[SSP] %u : %u(%ums)\n", uSensor,
			data->buf[uSensor].step_det,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case GAME_ROTATION_VECTOR:
	case ROTATION_VECTOR:
		ssp_dbg(" %u : %d, %d, %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].quat_a, data->buf[uSensor].quat_b,
			data->buf[uSensor].quat_c, data->buf[uSensor].quat_d,
			data->buf[uSensor].acc_rot,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case SIG_MOTION_SENSOR:
		ssp_dbg("[SSP] %u : %u(%ums)\n", uSensor,
			data->buf[uSensor].sig_motion,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case GYRO_UNCALIB_SENSOR:
		ssp_dbg("[SSP] %u : %d, %d, %d, %d, %d, %d (%ums)\n", uSensor,
			data->buf[uSensor].uncal_x, data->buf[uSensor].uncal_y,
			data->buf[uSensor].uncal_z, data->buf[uSensor].offset_x,
			data->buf[uSensor].offset_y, data->buf[uSensor].offset_z,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;
	case STEP_COUNTER:
		ssp_dbg("[SSP] %u : %u(%ums)\n", uSensor,
			data->buf[uSensor].step_diff,
			get_msdelay(data->adDelayBuf[uSensor]));
		break;

	default:
		ssp_dbg("[SSP] Wrong sensorCnt: %u\n", uSensor);
		break;
	}
}

static void debug_work_func(struct work_struct *work)
{
	unsigned int uSensorCnt;
	struct ssp_data *data = container_of(work, struct ssp_data, work_debug);

	ssp_dbg("[SSP]: %s(%u) - Sensor state: 0x%x, RC: %u, MS: %u Santi: %u Dump: %u\n",
		__func__, data->uIrqCnt, data->uSensorState, data->uResetCnt,
		data->uMissSensorCnt,data->uSanityCnt,data->uDumpCnt);

	if (data->fw_dl_state >= FW_DL_STATE_DOWNLOADING &&
		data->fw_dl_state < FW_DL_STATE_DONE) {
		pr_info("[SSP] : %s firmware downloading state = %d\n",
			__func__, data->fw_dl_state);
		return;
	} else if (data->fw_dl_state == FW_DL_STATE_FAIL) {
		pr_err("[SSP] : %s firmware download failed = %d\n",
			__func__, data->fw_dl_state);
		return;
	}
	if ((data->uSensorState & 0xff) == 0x0){
		pr_err("[SSP] : %s MCU sensor probe fail\n", __func__);
		return;
	}
	wake_lock(&data->ssp_wake_lock);
	if  (data->uLastResumeState != MSG2SSP_AP_STATUS_SUSPEND) {
		if(sanity_check(data)>0) data->uSanityCnt++;

		if (set_sensor_position(data) < 0) {
			pr_err("[SSP]: %s :set_sensor_position delayed \n", __func__);
		}
	}

	for (uSensorCnt = 0; uSensorCnt < SENSOR_MAX; uSensorCnt++)
		if ((atomic_read(&data->aSensorEnable) & (1 << uSensorCnt))
			|| data->batchLatencyBuf[uSensorCnt])
			print_sensordata(data, uSensorCnt);

	if (((data->uSsdFailCnt >= LIMIT_SSD_FAIL_CNT)
		|| (data->uInstFailCnt >= LIMIT_INSTRUCTION_FAIL_CNT)
		|| (data->uIrqFailCnt >= LIMIT_IRQ_FAIL_CNT)
		|| ((data->uTimeOutCnt + data->uBusyCnt) > LIMIT_TIMEOUT_CNT))
		&& (data->bSspShutdown == false)
		&& (data->uLastResumeState != MSG2SSP_AP_STATUS_SUSPEND)) {

		if (data->uResetCnt < LIMIT_RESET_CNT) {
			pr_info("[SSP] : %s - uSsdFailCnt(%u), uInstFailCnt(%u),"\
				"uIrqFailCnt(%u), uTimeOutCnt(%u), uBusyCnt(%u), pending(%u)\n",
				__func__, data->uSsdFailCnt, data->uInstFailCnt, data->uIrqFailCnt,
				data->uTimeOutCnt, data->uBusyCnt, !list_empty(&data->pending_list));
			reset_mcu(data);
			data->uResetCnt++;
		} else {
			pr_info("[SSP] : %s - data->uResetCnt == LIMIT_RESET_CNT\n", __func__);
			ssp_enable(data, false);
		}

		data->uSsdFailCnt = 0;
		data->uInstFailCnt = 0;
		data->uTimeOutCnt = 0;
		data->uBusyCnt = 0;
		data->uIrqFailCnt = 0;
	}

	data->uIrqCnt = 0;
	wake_unlock(&data->ssp_wake_lock);
}

static void debug_timer_func(unsigned long ptr)
{
	struct ssp_data *data = (struct ssp_data *)ptr;

	queue_work(data->debug_wq, &data->work_debug);
	mod_timer(&data->debug_timer,
		round_jiffies_up(jiffies + SSP_DEBUG_TIMER_SEC));
}

void enable_debug_timer(struct ssp_data *data)
{
	mod_timer(&data->debug_timer,
		round_jiffies_up(jiffies + SSP_DEBUG_TIMER_SEC));
}

void disable_debug_timer(struct ssp_data *data)
{
	del_timer_sync(&data->debug_timer);
	cancel_work_sync(&data->work_debug);
}

int initialize_debug_timer(struct ssp_data *data)
{
	setup_timer(&data->debug_timer, debug_timer_func, (unsigned long)data);

	data->debug_wq = create_singlethread_workqueue("ssp_debug_wq");
	if (!data->debug_wq)
		return ERROR;

	INIT_WORK(&data->work_debug, debug_work_func);
	return SUCCESS;
}
