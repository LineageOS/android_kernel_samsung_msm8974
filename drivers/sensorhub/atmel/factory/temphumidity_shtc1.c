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
#include <linux/qpnp/qpnp-adc.h>

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define VENDOR		"SENSIRION"
#define CHIP_ID		"SHTC1"
#define DONE_CAL	3

#if defined(CONFIG_MACH_KS01EUR)
#define MODEL_NAME	"GT-I9506"
#elif defined(CONFIG_MACH_KS01SKT)
#define MODEL_NAME	"SHV-E330S"
#elif defined(CONFIG_MACH_KS01KTT)
#define MODEL_NAME	"SHV-E330K"
#elif defined(CONFIG_MACH_KS01LGT)
#define MODEL_NAME	"SHV-E330L"
#elif defined(CONFIG_MACH_JACTIVESKT)
#define MODEL_NAME	"SHV-E470S"
#else
#define MODEL_NAME	"SGH-I337"
#endif

#define CP_THM_ADC_SAMPLING_CNT 7

#if defined(CONFIG_MACH_KS01SKT) || defined(CONFIG_MACH_KS01KTT)\
	|| defined(CONFIG_MACH_KS01LGT) || defined(CONFIG_MACH_JACTIVESKT)
/* {adc, temp*10}, -20 to +70 */
static struct cp_thm_adc_table temp_table_cp[] = {
	{27279, 700}, {27367, 690}, {27455, 680}, {27543, 670}, {27631, 660},
	{27718, 650}, {27819, 640}, {27920, 630}, {28021, 620}, {28122, 610},
	{28221, 600}, {28333, 590}, {28445, 580}, {28557, 570}, {28669, 560},
	{28781, 550}, {28917, 540}, {29053, 530}, {29189, 520}, {29325, 510},
	{29463, 500}, {29621, 490}, {29779, 480}, {29937, 470}, {30095, 460},
	{30252, 450}, {30416, 440}, {30580, 430}, {30744, 420}, {30908, 410},
	{31070, 400}, {31261, 390}, {31452, 380}, {31643, 370}, {31834, 360},
	{32025, 350}, {32223, 340}, {32421, 330}, {32619, 320}, {32817, 310},
	{33016, 300}, {33221, 290}, {33426, 280}, {33631, 270}, {33836, 260},
	{34043, 250}, {34242, 240}, {34441, 230}, {34640, 220}, {34839, 210},
	{35036, 200}, {35252, 190}, {35468, 180}, {35684, 170}, {35900, 160},
	{36115, 150}, {36320, 140}, {36525, 130}, {36730, 120}, {36935, 110},
	{37139, 100}, {37336, 90}, {37533, 80}, {37730, 70}, {37927, 60},
	{38122, 50}, {38327, 40}, {38532, 30}, {38737, 20}, {38942, 10},
	{39149, 0},
	{39310, -10}, {39471, -20}, {39632, -30}, {39793, -40}, {39954, -50},
	{40092, -60}, {40230, -70}, {40368, -80}, {40506, -90}, {40643, -100},
	{40755, -110}, {40867, -120}, {40979, -130}, {41091, -140},{41203, -150},
	{41296, -160}, {41389, -170}, {41482, -180}, {41575, -190}, {41669, -200},
 };
#endif
static int get_cp_thm_value(struct ssp_data *data)
{
	int err = 0;
	struct qpnp_vadc_result results;
		mutex_lock(&data->cp_temp_adc_lock);
	err = qpnp_vadc_read(NULL, LR_MUX6_PU2_AMUX_THM3, &results);
		mutex_unlock(&data->cp_temp_adc_lock);
		if (err) {
		pr_err("%s : error reading chn %d, rc = %d\n",
			__func__, LR_MUX6_PU2_AMUX_THM3, err);
			return err;
		}

	return results.adc_code;
}

static int convert_adc_to_temp(struct ssp_data *data, unsigned int adc)
{
#if defined(CONFIG_MACH_KS01SKT) || defined(CONFIG_MACH_KS01KTT)\
	|| defined(CONFIG_MACH_KS01LGT) || defined(CONFIG_MACH_JACTIVESKT)
	int low = 0;
	int high = 0;
	int mid = 0;
	u8 array_size = ARRAY_SIZE(temp_table_cp);

	if (temp_table_cp == NULL) {
		/* No voltage vs temperature table, using fake temp */
		return -990;
	}

	high = array_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (temp_table_cp[mid].adc > adc)
			high = mid - 1;
		else if (temp_table_cp[mid].adc < adc)
			low = mid + 1;
		else
			break;
	}
	return temp_table_cp[mid].temperature;
#else
{
	int err = 0;
	struct qpnp_vadc_result results;
	mutex_lock(&data->cp_temp_adc_lock);
	err = qpnp_vadc_read(NULL, LR_MUX6_PU2_AMUX_THM3, &results);
	mutex_unlock(&data->cp_temp_adc_lock);
	if (err) {
		pr_err("%s : error reading chn %d, rc = %d\n",
			__func__, LR_MUX6_PU2_AMUX_THM3, err);
		return err;
	}

	return results.physical;
}
#endif
}

static ssize_t temphumidity_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t temphumidity_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static ssize_t engine_version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s - engine_ver = %s_%s\n",
		__func__, MODEL_NAME, data->comp_engine_ver);

	return sprintf(buf, "%s_%s\n",
		MODEL_NAME, data->comp_engine_ver);
}

static ssize_t engine_version_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	kfree(data->comp_engine_ver);
	data->comp_engine_ver =
		    kzalloc(((strlen(buf)+1) * sizeof(char)), GFP_KERNEL);
	strncpy(data->comp_engine_ver, buf, strlen(buf)+1);
	pr_info("[SSP] %s - engine_ver = %s, %s\n",
		__func__, data->comp_engine_ver, buf);

	return size;
}

static ssize_t engine_version2_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	pr_info("[SSP] %s - engine_ver2 = %s_%s\n",
		__func__, MODEL_NAME, data->comp_engine_ver2);

	return sprintf(buf, "%s_%s\n",
		MODEL_NAME, data->comp_engine_ver2);
}

static ssize_t engine_version2_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);

	kfree(data->comp_engine_ver2);
	data->comp_engine_ver2 =
		    kzalloc(((strlen(buf)+1) * sizeof(char)), GFP_KERNEL);
	strncpy(data->comp_engine_ver2, buf, strlen(buf)+1);
	pr_info("[SSP] %s - engine_ver2 = %s, %s\n",
		__func__, data->comp_engine_ver2, buf);

	return size;
}

static ssize_t pam_adc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int adc = 0;
	if (data->bSspShutdown == true) {
		adc = 0;
		goto exit;
	}
		adc = get_cp_thm_value(data);
	/* pr_info("[SSP] %s cp_thm = %dmV\n", __func__, adc); */
exit:
	return sprintf(buf, "%d\n", adc);
}

static ssize_t pam_temp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int adc, temp;

	adc = get_cp_thm_value(data);
	if (adc < 0) {
		pr_err("[SSP] %s - reading adc failed.(%d)\n", __func__, adc);
		temp = adc;
	} else
		temp = convert_adc_to_temp(data, adc);
	pr_info("[SSP] %s cp_temperature(Celsius * 10) = %d\n",
		__func__, temp);
	return sprintf(buf, "%d\n", temp);
}

static ssize_t temphumidity_crc_check(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char chTempBuf[2] = {0, 10};
	int iDelayCnt = 0, iRet;
	struct ssp_data *data = dev_get_drvdata(dev);

	data->uFactorydata[0] = 0xff;
	data->uFactorydataReady = 0;
	iRet = send_instruction(data, FACTORY_MODE,
		TEMPHUMIDITY_CRC_FACTORY, chTempBuf, 2);

	while (!(data->uFactorydataReady &
		(1 << TEMPHUMIDITY_CRC_FACTORY))
		&& (iDelayCnt++ < 50)
		&& (iRet == SUCCESS))
		msleep(20);

	if ((iDelayCnt >= 50) || (iRet != SUCCESS)) {
		pr_err("[SSP]: %s - Temphumidity check crc Timeout!! %d\n",
			__func__, iRet);
			goto exit;
	}

	mdelay(5);

	pr_info("[SSP] : %s -Check_CRC : %d\n", __func__,
			data->uFactorydata[0]);

exit:
	if (data->uFactorydata[0] == 1)
		return sprintf(buf, "%s\n", "OK");
	else if (data->uFactorydata[0] == 2)
		return sprintf(buf, "%s\n","NG_NC");
	else
		return sprintf(buf, "%s\n","NG");
}

ssize_t temphumidity_send_accuracy(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	u8 accuracy;

	if (kstrtou8(buf, 10, &accuracy) < 0) {
		pr_err("[SSP] %s - read buf is fail(%s)\n", __func__, buf);
		return size;
	}

	if (accuracy == DONE_CAL)
		ssp_send_cmd(data, MSG2SSP_AP_TEMPHUMIDITY_CAL_DONE);
	pr_info("[SSP] %s - accuracy = %d\n", __func__, accuracy);

	return size;
}

static DEVICE_ATTR(name, S_IRUGO, temphumidity_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, temphumidity_vendor_show, NULL);
static DEVICE_ATTR(engine_ver, S_IRUGO | S_IWUSR | S_IWGRP,
	engine_version_show, engine_version_store);
static DEVICE_ATTR(engine_ver2, S_IRUGO | S_IWUSR | S_IWGRP,
	engine_version2_show, engine_version2_store);
static DEVICE_ATTR(cp_thm, S_IRUGO, pam_adc_show, NULL);
static DEVICE_ATTR(cp_temperature, S_IRUGO, pam_temp_show, NULL);
static DEVICE_ATTR(crc_check, S_IRUGO,
	temphumidity_crc_check, NULL);
static DEVICE_ATTR(send_accuracy,  S_IWUSR | S_IWGRP,
	NULL, temphumidity_send_accuracy);

static struct device_attribute *temphumidity_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_engine_ver,
	&dev_attr_engine_ver2,
	&dev_attr_cp_thm,
	&dev_attr_cp_temperature,
	&dev_attr_crc_check,
	&dev_attr_send_accuracy,
	NULL,
};

void initialize_temphumidity_factorytest(struct ssp_data *data)
{
	sensors_register(data->temphumidity_device,
		data, temphumidity_attrs, "temphumidity_sensor");
}

void remove_temphumidity_factorytest(struct ssp_data *data)
{
	if (data->comp_engine_ver != NULL)
		kfree(data->comp_engine_ver);
	if (data->comp_engine_ver2 != NULL)
		kfree(data->comp_engine_ver2);
	sensors_unregister(data->temphumidity_device, temphumidity_attrs);
}
