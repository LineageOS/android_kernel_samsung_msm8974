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
#include "../ssp.h"

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define VENDOR_AK		"AKM"
#define CHIP_ID_AK		"AK09911C"

#define GM_DATA_SPEC_MIN	-1600
#define GM_DATA_SPEC_MAX	1600

#define GM_SELFTEST_X_SPEC_MIN	-30
#define GM_SELFTEST_X_SPEC_MAX	30
#define GM_SELFTEST_Y_SPEC_MIN	-30
#define GM_SELFTEST_Y_SPEC_MAX	30
#define GM_SELFTEST_Z_SPEC_MIN	-400
#define GM_SELFTEST_Z_SPEC_MAX	-50

static ssize_t magnetic_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR_AK);
}

static ssize_t magnetic_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID_AK);
}

static int check_data_spec(struct ssp_data *data, u8 uSensorType)
{
	if ((data->buf[uSensorType].x == 0) &&
		(data->buf[uSensorType].y == 0) &&
		(data->buf[uSensorType].z == 0))
		return FAIL;
	else if ((data->buf[uSensorType].x > GM_DATA_SPEC_MAX)
		|| (data->buf[uSensorType].x < GM_DATA_SPEC_MIN)
		|| (data->buf[uSensorType].y > GM_DATA_SPEC_MAX)
		|| (data->buf[uSensorType].y < GM_DATA_SPEC_MIN)
		|| (data->buf[uSensorType].z > GM_DATA_SPEC_MAX)
		|| (data->buf[uSensorType].z < GM_DATA_SPEC_MIN))
		return FAIL;
	else
		return SUCCESS;
}

static ssize_t raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s - %d,%d,%d\n", __func__,
		data->buf[GEOMAGNETIC_RAW].x,
		data->buf[GEOMAGNETIC_RAW].y,
		data->buf[GEOMAGNETIC_RAW].z);

	if (data->bGeomagneticRawEnabled == false) {
		data->buf[GEOMAGNETIC_RAW].x = -1;
		data->buf[GEOMAGNETIC_RAW].y = -1;
		data->buf[GEOMAGNETIC_RAW].z = -1;
	} else {
		if (data->buf[GEOMAGNETIC_RAW].x > 18000)
			data->buf[GEOMAGNETIC_RAW].x = 18000;
		else if (data->buf[GEOMAGNETIC_RAW].x < -18000)
			data->buf[GEOMAGNETIC_RAW].x = -18000;
		if (data->buf[GEOMAGNETIC_RAW].y > 18000)
			data->buf[GEOMAGNETIC_RAW].y = 18000;
		else if (data->buf[GEOMAGNETIC_RAW].y < -18000)
			data->buf[GEOMAGNETIC_RAW].y = -18000;
		if (data->buf[GEOMAGNETIC_RAW].z > 18000)
			data->buf[GEOMAGNETIC_RAW].z = 18000;
		else if (data->buf[GEOMAGNETIC_RAW].z < -18000)
			data->buf[GEOMAGNETIC_RAW].z = -18000;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->buf[GEOMAGNETIC_RAW].x,
		data->buf[GEOMAGNETIC_RAW].y,
		data->buf[GEOMAGNETIC_RAW].z);
}

static ssize_t raw_data_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	char chTempbuf[9] = { 0, };
	int iRet;
	int64_t dEnable;
	int iRetries = 50;
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 dMsDelay = 20;
	memcpy(&chTempbuf[0], &dMsDelay, 4);

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	if (dEnable) {
		data->buf[GEOMAGNETIC_RAW].x = 0;
		data->buf[GEOMAGNETIC_RAW].y = 0;
		data->buf[GEOMAGNETIC_RAW].z = 0;

		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_RAW,
			chTempbuf, 9);

		do {
			msleep(20);
			if (check_data_spec(data, GEOMAGNETIC_RAW) == SUCCESS)
				break;
		} while (--iRetries);

		if (iRetries > 0) {
			pr_info("[SSP] %s - success, %d\n", __func__, iRetries);
			data->bGeomagneticRawEnabled = true;
		} else {
			pr_err("[SSP] %s - wait timeout, %d\n", __func__,
				iRetries);
			data->bGeomagneticRawEnabled = false;
		}
	} else {
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_RAW,
			chTempbuf, 4);
		data->bGeomagneticRawEnabled = false;
	}

	return size;
}

static ssize_t adc_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	bool bSuccess = false;
	u8 chTempbuf[9] = { 0, };
	s16 iSensorBuf[3] = {0, };
	int iRetries = 10;
	bool add_mag = false;
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 dMsDelay = 20;
	memcpy(&chTempbuf[0], &dMsDelay, 4);

	data->buf[GEOMAGNETIC_SENSOR].x = 0;
	data->buf[GEOMAGNETIC_SENSOR].y = 0;
	data->buf[GEOMAGNETIC_SENSOR].z = 0;

	if (!(atomic_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_SENSOR))) {
		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_SENSOR,
			chTempbuf, 9);
		add_mag = true;
	}

	do {
		msleep(60);
		if (check_data_spec(data, GEOMAGNETIC_SENSOR) == SUCCESS)
			break;
	} while (--iRetries);

	if (iRetries > 0)
		bSuccess = true;

	iSensorBuf[0] = data->buf[GEOMAGNETIC_SENSOR].x;
	iSensorBuf[1] = data->buf[GEOMAGNETIC_SENSOR].y;
	iSensorBuf[2] = data->buf[GEOMAGNETIC_SENSOR].z;

	if (add_mag == true)
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_SENSOR,
			chTempbuf, 4);

	pr_info("[SSP]: %s - x = %d, y = %d, z = %d\n", __func__,
		iSensorBuf[0], iSensorBuf[1], iSensorBuf[2]);

	return sprintf(buf, "%s,%d,%d,%d\n", (bSuccess ? "OK" : "NG"),
		iSensorBuf[0], iSensorBuf[1], iSensorBuf[2]);
}

static ssize_t magnetic_get_asa(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d,%d,%d\n", (s16)data->uFuseRomData[0],
		(s16)data->uFuseRomData[1], (s16)data->uFuseRomData[2]);
}

static ssize_t magnetic_get_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	bool bSuccess;
	struct ssp_data *data = dev_get_drvdata(dev);

	if ((data->uFuseRomData[0] == 0) ||
		(data->uFuseRomData[0] == 0xff) ||
		(data->uFuseRomData[1] == 0) ||
		(data->uFuseRomData[1] == 0xff) ||
		(data->uFuseRomData[2] == 0) ||
		(data->uFuseRomData[2] == 0xff))
		bSuccess = false;
	else
		bSuccess = true;

	return sprintf(buf, "%s,%u\n", (bSuccess ? "OK" : "NG"), bSuccess);
}

static ssize_t magnetic_check_cntl(struct device *dev,
		struct device_attribute *attr, char *strbuf)
{
	bool bSuccess = false;
	int iRet;
	char chTempBuf[22] = { 0,  };
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg;

	if (!data->uMagCntlRegData) {
		bSuccess = true;
	} else {
		pr_info("[SSP] %s - check cntl register before selftest", __func__);
		msg = kzalloc(sizeof(*msg), GFP_KERNEL);
		if (msg == NULL) {
			pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n", __func__);
			return -ENOMEM;
		}
		msg->cmd = GEOMAGNETIC_FACTORY;
		msg->length = 22;
		msg->options = AP2HUB_READ;
		msg->buffer = chTempBuf;
		msg->free_buffer = 0;

		iRet = ssp_spi_sync(data, msg, 1000);

		if (iRet != SUCCESS) {
			pr_err("[SSP] %s - spi sync failed due to Timeout!! %d\n",
					__func__, iRet);
		}

		data->uMagCntlRegData = chTempBuf[21];
		bSuccess = !data->uMagCntlRegData;
	}

	pr_info("[SSP] %s - CTRL : 0x%x\n", __func__,
				data->uMagCntlRegData);

	data->uMagCntlRegData = 1;	/* reset the value */

	return sprintf(strbuf, "%s,%d,%d,%d\n",
		(bSuccess ? "OK" : "NG"), (bSuccess ? 1 : 0), 0, 0);
}

static ssize_t magnetic_get_selftest(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	s8 iResult[4] = {-1, -1, -1, -1};
	char bufSelftset[22] = {0, };
	char bufAdc[9] = {0, };
	s16 iSF_X = 0, iSF_Y = 0, iSF_Z = 0;
	s16 iADC_X = 0, iADC_Y = 0, iADC_Z = 0;
	s32 dMsDelay = 20;
	int iRet = 0, iSpecOutRetries = 0;
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg;

	pr_info("[SSP] %s in\n", __func__);

	/* STATUS */
	if ((data->uFuseRomData[0] == 0) ||
		(data->uFuseRomData[0] == 0xff) ||
		(data->uFuseRomData[1] == 0) ||
		(data->uFuseRomData[1] == 0xff) ||
		(data->uFuseRomData[2] == 0) ||
		(data->uFuseRomData[2] == 0xff))
		iResult[0] = -1;
	else
		iResult[0] = 0;

Retry_selftest:
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL) {
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n", __func__);
		goto exit;
	}
	msg->cmd = GEOMAGNETIC_FACTORY;
	msg->length = 22;
	msg->options = AP2HUB_READ;
	msg->buffer = bufSelftset;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Magnetic Selftest Timeout!! %d\n", __func__, iRet);
		goto exit;
	}

	/* read 6bytes data registers */
	iSF_X = (s16)((bufSelftset[13] << 8) + bufSelftset[14]);
	iSF_Y = (s16)((bufSelftset[15] << 8) + bufSelftset[16]);
	iSF_Z = (s16)((bufSelftset[17] << 8) + bufSelftset[18]);

	/* DAC (store Cntl Register value to check power down) */
	iResult[2] = bufSelftset[21];

	iSF_X = (s16)(((iSF_X * data->uFuseRomData[0]) >> 7) + iSF_X);
	iSF_Y = (s16)(((iSF_Y * data->uFuseRomData[1]) >> 7) + iSF_Y);
	iSF_Z = (s16)(((iSF_Z * data->uFuseRomData[2]) >> 7) + iSF_Z);

	pr_info("[SSP] %s: self test x = %d, y = %d, z = %d\n",
		__func__, iSF_X, iSF_Y, iSF_Z);

	if ((iSF_X >= GM_SELFTEST_X_SPEC_MIN)
		&& (iSF_X <= GM_SELFTEST_X_SPEC_MAX))
		pr_info("[SSP] x passed self test, expect -30<=x<=30\n");
	else
		pr_info("[SSP] x failed self test, expect -30<=x<=30\n");
	if ((iSF_Y >= GM_SELFTEST_Y_SPEC_MIN)
		&& (iSF_Y <= GM_SELFTEST_Y_SPEC_MAX))
		pr_info("[SSP] y passed self test, expect -30<=y<=30\n");
	else
		pr_info("[SSP] y failed self test, expect -30<=y<=30\n");
	if ((iSF_Z >= GM_SELFTEST_Z_SPEC_MIN)
		&& (iSF_Z <= GM_SELFTEST_Z_SPEC_MAX))
		pr_info("[SSP] z passed self test, expect -400<=z<=-50\n");
	else
		pr_info("[SSP] z failed self test, expect -400<=z<=-50\n");

	/* SELFTEST */
	if ((iSF_X >= GM_SELFTEST_X_SPEC_MIN)
		&& (iSF_X <= GM_SELFTEST_X_SPEC_MAX)
		&& (iSF_Y >= GM_SELFTEST_Y_SPEC_MIN)
		&& (iSF_Y <= GM_SELFTEST_Y_SPEC_MAX)
		&& (iSF_Z >= GM_SELFTEST_Z_SPEC_MIN)
		&& (iSF_Z <= GM_SELFTEST_Z_SPEC_MAX))
		iResult[1] = 0;

	if ((iResult[1] == -1) && (iSpecOutRetries++ < 5)) {
		pr_err("[SSP] %s, selftest spec out. Retry = %d", __func__, iSpecOutRetries);
		goto Retry_selftest;
	}

	iSpecOutRetries = 10;

	/* ADC */
	memcpy(&bufAdc[0], &dMsDelay, 4);

	data->buf[GEOMAGNETIC_RAW].x = 0;
	data->buf[GEOMAGNETIC_RAW].y = 0;
	data->buf[GEOMAGNETIC_RAW].z = 0;

	if (!(atomic_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_RAW)))
		send_instruction(data, ADD_SENSOR, GEOMAGNETIC_RAW,
			bufAdc, 9);

	do {
		msleep(60);
		if (check_data_spec(data, GEOMAGNETIC_RAW) == SUCCESS)
			break;
	} while (--iSpecOutRetries);

	if (iSpecOutRetries > 0)
		iResult[3] = 0;

	iADC_X = data->buf[GEOMAGNETIC_RAW].x;
	iADC_Y = data->buf[GEOMAGNETIC_RAW].y;
	iADC_Z = data->buf[GEOMAGNETIC_RAW].z;

	if (!(atomic_read(&data->aSensorEnable) & (1 << GEOMAGNETIC_RAW)))
		send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_RAW,
			bufAdc, 4);

	pr_info("[SSP]: %s -adc, x = %d, y = %d, z = %d, retry = %d\n", __func__,
		iADC_X, iADC_Y, iADC_Z, iSpecOutRetries);

exit:
	pr_info("[SSP] %s out. Result = %d %d %d %d\n",
		__func__, iResult[0], iResult[1], iResult[2], iResult[3]);

	return sprintf(buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		iResult[0], iResult[1], iSF_X, iSF_Y, iSF_Z,
		iResult[2], iResult[3], iADC_X, iADC_Y, iADC_Z);
}

#ifdef SAVE_MAG_LOG
static ssize_t raw_data_logging_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	if (data->bGeomagneticLogged == false) {
		data->buf[GEOMAGNETIC_SENSOR].x = -1;
		data->buf[GEOMAGNETIC_SENSOR].y = -1;
		data->buf[GEOMAGNETIC_SENSOR].z = -1;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		data->buf[GEOMAGNETIC_SENSOR].x,
		data->buf[GEOMAGNETIC_SENSOR].y,
		data->buf[GEOMAGNETIC_SENSOR].z);
}

static ssize_t raw_data_logging_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	u8 uBuf[4] = {0, };
	int iRet;
	int64_t dEnable;
	struct ssp_data *data = dev_get_drvdata(dev);
	s32 dMsDelay = 10;
	memcpy(&uBuf[0], &dMsDelay, 4);

	iRet = kstrtoll(buf, 10, &dEnable);
	if (iRet < 0)
		return iRet;

	if (dEnable) {
		ssp_dbg("[SSP]: %s - add %u, New = %dns\n",
			 __func__, 1 << GEOMAGNETIC_SENSOR, dMsDelay);

		iRet = send_instruction(data, GET_LOGGING, GEOMAGNETIC_SENSOR, uBuf, 4);
		if (iRet == SUCCESS) {
			pr_info("[SSP] %s - success\n", __func__);
			data->bGeomagneticLogged = true;
		} else {
			pr_err("[SSP] %s - failed, %d\n", __func__, iRet);
			data->bGeomagneticLogged = false;
		}
	} else {
		iRet = send_instruction(data, REMOVE_SENSOR, GEOMAGNETIC_SENSOR, uBuf, 4);
		if (iRet == SUCCESS) {
			pr_info("[SSP] %s - success\n", __func__);
			data->bGeomagneticLogged = false;
		}
		ssp_dbg("[SSP]: %s - remove sensor = %d\n",
			__func__, (1 << GEOMAGNETIC_SENSOR));
	}

	return size;
}
#endif

static DEVICE_ATTR(name, S_IRUGO, magnetic_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, magnetic_vendor_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO | S_IWUSR | S_IWGRP,
		raw_data_show, raw_data_store);
static DEVICE_ATTR(adc, S_IRUGO, adc_data_read, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, magnetic_get_selftest, NULL);
static DEVICE_ATTR(status, S_IRUGO,  magnetic_get_status, NULL);
static DEVICE_ATTR(dac, S_IRUGO, magnetic_check_cntl, NULL);
static DEVICE_ATTR(ak09911_asa, S_IRUGO, magnetic_get_asa, NULL);
#ifdef SAVE_MAG_LOG
static DEVICE_ATTR(logging_data, S_IRUGO | S_IWUSR | S_IWGRP,
		raw_data_logging_show, raw_data_logging_store);
#endif

static struct device_attribute *mag_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_adc,
	&dev_attr_dac,
	&dev_attr_raw_data,
	&dev_attr_selftest,
	&dev_attr_status,
	&dev_attr_ak09911_asa,
#ifdef SAVE_MAG_LOG
	&dev_attr_logging_data,
#endif
	NULL,
};

void initialize_magnetic_factorytest(struct ssp_data *data)
{
	sensors_register(data->mag_device, data,
				mag_attrs, "magnetic_sensor");
}

void remove_magnetic_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->mag_device, mag_attrs);
}
