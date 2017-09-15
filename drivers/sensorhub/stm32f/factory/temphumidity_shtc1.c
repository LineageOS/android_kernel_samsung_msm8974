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

#define SHTC1_IOCTL_MAGIC		0xFB
#define IOCTL_READ_COMPLETE	_IOR(SHTC1_IOCTL_MAGIC, 0x01, unsigned short *)
#define IOCTL_READ_ADC_BATT_DATA	_IOR(SHTC1_IOCTL_MAGIC, 0x02, unsigned short *)
#define IOCTL_READ_ADC_CHG_DATA	_IOR(SHTC1_IOCTL_MAGIC, 0x03, unsigned short *)
#define IOCTL_READ_THM_SHTC1_DATA	_IOR(SHTC1_IOCTL_MAGIC, 0x04, short *)
#define IOCTL_READ_HUM_SHTC1_DATA	_IOR(SHTC1_IOCTL_MAGIC, 0x05, unsigned short *)
#define IOCTL_READ_THM_BARO_DATA	_IOR(SHTC1_IOCTL_MAGIC, 0x06, unsigned short *)
#define IOCTL_READ_THM_GYRO_DATA	_IOR(SHTC1_IOCTL_MAGIC, 0x07, unsigned short *)

#if defined (CONFIG_MACH_JSGLTE_CHN_CMCC)
#define MODEL_NAME	"GT-I9508V"
#elif defined (CONFIG_MACH_HLTEATT)
#define MODEL_NAME	"SM-N900A"
#elif defined (CONFIG_MACH_HLTE_CHN_TDOPEN)
#define MODEL_NAME	"SM-N9008S"
#elif defined (CONFIG_MACH_HLTE_CHN_CMCC)
#define MODEL_NAME	"SM-N9008V"
#elif defined (CONFIG_MACH_H3GDUOS_CTC)
#define MODEL_NAME	"SM-N9009"
#elif defined (CONFIG_MACH_H3GDUOS_CU)
#define MODEL_NAME	"SM-N9002"
#elif defined (CONFIG_MACH_H3G_CHN_OPEN)
#define MODEL_NAME	"SM-N9006"
#elif defined (CONFIG_MACH_H3G_CHN_CMCC)
#define MODEL_NAME	"SM-N9008"
#elif defined (CONFIG_MACH_HLTECHNTWU)
#define MODEL_NAME	"SM-N900U"
#elif defined (CONFIG_MACH_HLTEEUR)
#define MODEL_NAME	"SM-N9005"
#elif defined (CONFIG_MACH_HLTEAUS)
#define MODEL_NAME	"SM-N9007"
#elif defined (CONFIG_MACH_HLTETMO)
#define MODEL_NAME	"SM-N900T"
#elif defined (CONFIG_MACH_HLTESPR)
#define MODEL_NAME	"SM-N900P"
#elif defined (CONFIG_MACH_HLTEVZW)
#define MODEL_NAME	"SM-N900V"
#elif defined (CONFIG_MACH_HLTEUSC)
#define MODEL_NAME	"SM-N900R4"
#elif defined (CONFIG_MACH_HLTESKT)
#define MODEL_NAME	"SM-N900S"
#elif defined (CONFIG_MACH_HLTEKTT)
#define MODEL_NAME	"SM-N900K"
#elif defined (CONFIG_MACH_HLTELGT)
#define MODEL_NAME	"SM-N900L"
#elif defined (CONFIG_MACH_FRESCOLTESKT)
#define MODEL_NAME	"SM-N750S"
#elif defined (CONFIG_MACH_FRESCOLTEKTT)
#define MODEL_NAME	"SM-N750K"
#elif defined (CONFIG_MACH_FRESCOLTELGT)
#define MODEL_NAME	"SM-N750L"
#elif defined (CONFIG_MACH_HLTEDCM)
#define MODEL_NAME	"SM-N900D"
#elif defined (CONFIG_MACH_HLTEKDI)
#define MODEL_NAME	"SM-N900J"
#elif defined (CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_JS01LTESBM)
#define MODEL_NAME	"SGH-N075"
#elif defined (CONFIG_MACH_FLTEEUR)
#define MODEL_NAME	"SM-G7905"
#elif defined (CONFIG_MACH_FLTESKT)
#define MODEL_NAME	"SM-G910S"
#else
#define MODEL_NAME	"SM-N900A"
#endif

#if defined(CONFIG_MACH_JF_ATT) || defined(CONFIG_MACH_JF_TMO) || \
	defined(CONFIG_MACH_JF_EUR) || defined(CONFIG_MACH_JF_SPR) || \
	defined(CONFIG_MACH_JF_USC) || defined(CONFIG_MACH_JF_VZW)
/* {adc, temp*10}, -20 to +70 */
static struct cp_thm_adc_table temp_table_cp[] = {
	{200, 700}, {207, 690}, {214, 680}, {221, 670}, {248, 660},
	{235, 650}, {243, 640}, {251, 630}, {259, 620}, {267, 610},
	{276, 600}, {286, 590}, {295, 580}, {304, 570}, {314, 560},
	{324, 550}, {336, 540}, {348, 530}, {360, 520}, {372, 510},
	{383, 500}, {402, 490}, {422, 480}, {441, 470}, {461, 460},
	{480, 450}, {495, 440}, {510, 430}, {526, 420}, {542, 410},
	{558, 400}, {574, 390}, {591, 380}, {607, 370}, {624, 360},
	{641, 350}, {659, 340}, {677, 330}, {696, 320}, {705, 310},
	{733, 300}, {752, 290}, {771, 280}, {790, 270}, {808, 260},
	{828, 250}, {848, 240}, {869, 230}, {890, 220}, {910, 210},
	{931, 200}, {951, 190}, {971, 180}, {992, 170}, {1012, 160},
	{1032, 150}, {1053, 140}, {1074, 130}, {1095, 120}, {1116, 110},
	{1137, 100}, {1155, 90}, {1173, 80}, {1191, 70}, {1210, 60},
	{1228, 50}, {1245, 40}, {1262, 30}, {1279, 20}, {1296, 10},
	{1313, 0},
	{1322, -10}, {1331, -20}, {1340, -30}, {1350, -40}, {1359, -50},
	{1374, -60}, {1389, -70}, {1404, -80}, {1419, -90}, {1434, -100},
	{1446, -110}, {1458, -120}, {1470, -130}, {1483, -140},{1495, -150},
	{1504, -160}, {1513, -170}, {1522, -180}, {1532, -190}, {1542, -200},
 };
#endif

static struct cp_thm_adc_table temp_table_batt[] = {
	{636, 600}, {659, 590}, {683, 580}, {707, 570}, {730, 560},
	{754, 550}, {782, 540}, {810, 530}, {838, 520}, {866, 510},
	{894, 500}, {924, 490}, {953, 480}, {982, 470}, {1011, 460},
	{1040, 450}, {1077, 440}, {1114, 430}, {1152, 420}, {1189, 410},
	{1227, 400}, {1259, 390}, {1293, 380}, {1326, 370}, {1360, 360},
	{1394, 350}, {1435, 340}, {1476, 330}, {1516, 320}, {1557, 310},
	{1598, 300}, {1642, 290}, {1687, 280}, {1731, 270}, {1776, 260},
	{1820, 250}, {1866, 240}, {1913, 230}, {1961, 220}, {2008, 210},
	{2055, 200}, {2111, 190}, {2166, 180}, {2222, 170}, {2277, 160},
	{2333, 150}, {2382, 140}, {2431, 130}, {2480, 120}, {2529, 110},
	{2578, 100}, {2624, 90}, {2670, 80}, {2717, 70}, {2763, 60},
	{2810, 50}, {2851, 40}, {2892, 30}, {2933, 20}, {2973, 10},
	{3014, 0}, {3056, -10}, {3099, -20}, {3142, -30}, {3185, -40},
	{3229, -50}, {3320, -60}, {3336, -70}, {3352, -80}, {3368, -90},
	{3385, -100}, {3494, -110}, {3509, -120}, {3524, -130}, {3539, -140},
	{3554, -150}, {3614, -160}, {3629, -170}, {3644, -180},{3659, -190},
	{3674, -200}
 };

static struct cp_thm_adc_table temp_table_chg[] = {
	{636, 600}, {659, 590}, {682, 580}, {705, 570}, {728, 560},
	{751, 550}, {779, 540}, {808, 530}, {837, 520}, {866, 510},
	{895, 500}, {924, 490}, {953, 480}, {982, 470}, {1011, 460},
	{1040, 450}, {1077, 440}, {1115, 430}, {1152, 420}, {1189, 410},
	{1227, 400}, {1260, 390}, {1293, 380}, {1326, 370}, {1360, 360},
	{1393, 350}, {1435, 340}, {1477, 330}, {1520, 320}, {1562, 310},
	{1604, 300}, {1648, 290}, {1691, 280}, {1735, 270}, {1778, 260},
	{1822, 250}, {1869, 240}, {1915, 230}, {1962, 220}, {2009, 210},
	{2056, 200}, {2110, 190}, {2164, 180}, {2219, 170}, {2273, 160},
	{2328, 150}, {2379, 140}, {2430, 130}, {2481, 120}, {2532, 110},
	{2584, 100}, {2629, 90}, {2674, 80}, {2719, 70}, {2764, 60},
	{2810, 50}, {2852, 40}, {2895, 30}, {2937, 20}, {2980, 10},
	{3022, 0}, {3063, -10}, {3103, -20}, {3144, -30}, {3184, -40},
	{3225, -50}, {3257, -60}, {3290, -70}, {3322, -80}, {3355, -90},
	{3387, -100}, {3523, -110}, {3532, -120}, {3540, -130}, {3549, -140},
	{3558, -150}, {3654, -160}, {3658, -170}, {3661, -180}, {3664, -190},
	{3667, -200}
 };

struct qpnp_vadc_chip *ssp_vadc;

static long ssp_temphumidity_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	struct ssp_data *data
		= container_of(file->private_data,
			struct ssp_data, shtc1_device);

	void __user *argp = (void __user *)arg;
	int retries = 2;
	int length = 0;
	int ret = 0;
	if (data->bulk_buffer == NULL)
		return -EINVAL;

	length = data->bulk_buffer->len;
	mutex_lock(&data->bulk_temp_read_lock);
	switch (cmd) {
		case IOCTL_READ_COMPLETE: /* free */
			if(data->bulk_buffer) {
				kfree(data->bulk_buffer);
				data->bulk_buffer = NULL;
			}
			length = 1;
			break;

		case IOCTL_READ_ADC_BATT_DATA:
			while (retries--) {
				ret = copy_to_user(argp,
					data->bulk_buffer->batt,
					data->bulk_buffer->len*2);
				if (likely(!ret))
					break;
			}
			if (unlikely(ret)) {
				pr_err("[SSP] read bluk adc1 data err(%d)", ret);
				goto ioctl_error;
			}
			break;

		case IOCTL_READ_ADC_CHG_DATA:
			while (retries--) {
				ret = copy_to_user(argp,
					data->bulk_buffer->chg,
					data->bulk_buffer->len*2);
				if (likely(!ret))
					break;
			}
			if (unlikely(ret)) {
				pr_err("[SSP] read bluk adc1 data err(%d)", ret);
				goto ioctl_error;
			}
			break;

		case IOCTL_READ_THM_SHTC1_DATA:
			while (retries--) {
				ret = copy_to_user(argp,
					data->bulk_buffer->temp,
					data->bulk_buffer->len*2);
				if (likely(!ret))
					break;
			}
			if (unlikely(ret)) {
				pr_err("[SSP] read bluk adc1 data err(%d)", ret);
				goto ioctl_error;
			}
			break;

		case IOCTL_READ_HUM_SHTC1_DATA:
			while (retries--) {
				ret = copy_to_user(argp,
					data->bulk_buffer->humidity,
					data->bulk_buffer->len*2);
				if (likely(!ret))
					break;
			}
			if (unlikely(ret)) {
				pr_err("[SSP] read bluk adc1 data err(%d)", ret);
				goto ioctl_error;
			}
			break;

		case IOCTL_READ_THM_BARO_DATA:
			while (retries--) {
				ret = copy_to_user(argp,
					data->bulk_buffer->baro,
					data->bulk_buffer->len*2);
				if (likely(!ret))
					break;
			}
			if (unlikely(ret)) {
				pr_err("[SSP] read bluk adc1 data err(%d)", ret);
				goto ioctl_error;
			}
			break;

		case IOCTL_READ_THM_GYRO_DATA:
			while (retries--) {
				ret = copy_to_user(argp,
					data->bulk_buffer->gyro,
					data->bulk_buffer->len*2);
				if (likely(!ret))
					break;
			}
			if (unlikely(ret)) {
				pr_err("[SSP] read bluk adc1 data err(%d)", ret);
				goto ioctl_error;
			}
			break;

		default:
			pr_err("[SSP] temp ioctl cmd err(%d)", cmd);
			ret = EINVAL;
			goto ioctl_error;
	}
	mutex_unlock(&data->bulk_temp_read_lock);
	return length;

ioctl_error:
		mutex_unlock(&data->bulk_temp_read_lock);
		return -ret;
}

static struct file_operations ssp_temphumidity_fops = {
	.owner = THIS_MODULE,
	.open = nonseekable_open,
	.unlocked_ioctl = ssp_temphumidity_ioctl,
};

static int get_cp_thm_value(struct ssp_data *data)
{
	int err = 0;
	struct qpnp_vadc_result results;
		mutex_lock(&data->cp_temp_adc_lock);
	err = qpnp_vadc_read(ssp_vadc,LR_MUX6_PU2_AMUX_THM3, &results);
		mutex_unlock(&data->cp_temp_adc_lock);
		if (err) {
		pr_err("%s : error reading chn %d, rc = %d\n",
			__func__, LR_MUX6_PU2_AMUX_THM3, err);
			return err;
		}
	return results.adc_code;

}

static int get_cp_thm2_value(struct ssp_data *data)
{
	int err = 0;
	struct qpnp_vadc_result results;
	mutex_lock(&data->cp_temp_adc_lock);
	err = qpnp_vadc_read(ssp_vadc, LR_MUX8_PU2_AMUX_THM4, &results);
	mutex_unlock(&data->cp_temp_adc_lock);
	if (err) {
		pr_err("%s : error reading chn %d, rc = %d\n",
			__func__, LR_MUX8_PU2_AMUX_THM4, err);
		return err;
	}
	return results.adc_code;
}

static int convert_adc_to_temp(struct ssp_data *data, unsigned int adc)
{
#if defined(CONFIG_MACH_JF_ATT) || defined(CONFIG_MACH_JF_TMO) || \
	defined(CONFIG_MACH_JF_EUR) || defined(CONFIG_MACH_JF_SPR) || \
	defined(CONFIG_MACH_JF_USC) || defined(CONFIG_MACH_JF_VZW)
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
	err = qpnp_vadc_read(ssp_vadc, LR_MUX6_PU2_AMUX_THM3, &results);
	mutex_unlock(&data->cp_temp_adc_lock);
	if (err) {
		pr_err("%s : error reading chn %d, rc = %d\n",
			__func__, LR_MUX6_PU2_AMUX_THM3, err);
		return err;
	}

	return results.physical * 10;
}
#endif
}

static int convert_adc_to_temp2(struct ssp_data *data, unsigned int adc)
{
	int err = 0;
	struct qpnp_vadc_result results;
	mutex_lock(&data->cp_temp_adc_lock);
	err = qpnp_vadc_read(ssp_vadc, LR_MUX8_PU2_AMUX_THM4, &results);
	mutex_unlock(&data->cp_temp_adc_lock);
	if (err) {
		pr_err("%s : error reading chn %d, rc = %d\n",
			__func__, LR_MUX8_PU2_AMUX_THM4, err);
		return err;
	}

	return results.physical * 10;
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

static ssize_t pam_adc2_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int adc;
	if (data->bSspShutdown == true) {
		adc = 0;
		goto exit;
	}
	adc = get_cp_thm2_value(data);
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
	} else {
		temp = convert_adc_to_temp(data, adc);
	}

	pr_info("[SSP] %s cp_temperature(Celsius * 10) = %d\n",
		__func__, temp);
	return sprintf(buf, "%d\n", temp);
}

static ssize_t pam_temp2_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int adc, temp;

	adc = get_cp_thm_value(data);
	if (adc < 0) {
		pr_err("[SSP] %s - reading adc failed.(%d)\n", __func__, adc);
		temp = adc;
	} else {
		temp = convert_adc_to_temp2(data, adc);
	}

	pr_info("[SSP] %s cp_temperature(Celsius * 10) = %d\n",
		__func__, temp);
	return sprintf(buf, "%d\n", temp);
}

s16 get_hub_adc(struct ssp_data *data, u32 chan) {

	s16 adc = -1;
	int iRet = 0;

	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_GET_THERM;
	msg->length = 2;
	msg->options = AP2HUB_READ;
	msg->data = chan;
	msg->buffer = (char *) &adc;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	return adc;
}

static ssize_t hub_batt_adc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	static s16 prev_adc = 1865;
	s16 adc;

	if (data->bSspShutdown == true){
		adc = 0;
		goto exit;
	}

	adc = get_hub_adc(data, ADC_BATT);

	if (adc > 0)
		prev_adc = adc;
	else
		adc = prev_adc;

	pr_info("[SSP]: %s: adc %d\n", __func__, adc);

exit:
	return sprintf(buf, "%d\n", adc);
}

static ssize_t hub_chg_adc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	static s16 prev_adc = 1630;
	s16 adc;

	if (data->bSspShutdown == true){
		adc = 0;
		goto exit;
	}

	adc = get_hub_adc(data, ADC_CHG);

	if (adc > 0)
		prev_adc = adc;
	else
		adc = prev_adc;

	pr_info("[SSP]: %s: adc %d\n", __func__, adc);

exit:
	return sprintf(buf, "%d\n", adc);
}

static ssize_t hub_batt_temp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int low = 0;
	int high = 0;
	int mid = 0;
	u8 array_size = ARRAY_SIZE(temp_table_batt);

	s16 adc = get_hub_adc(data, ADC_BATT);
	high = array_size - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		if (temp_table_batt[mid].adc > adc)
			high = mid - 1;
		else if (temp_table_batt[mid].adc < adc)
			low = mid + 1;
		else
			break;
	}

	pr_info("[SSP]: %s: adc %d\n", __func__, temp_table_batt[mid].temperature);

	return sprintf(buf, "%d\n", temp_table_batt[mid].temperature);

}

static ssize_t hub_chg_temp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int low = 0;
	int high = 0;
	int mid = 0;
	u8 array_size = ARRAY_SIZE(temp_table_chg);

	s16 adc = get_hub_adc(data, ADC_CHG);
	high = array_size - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		if (temp_table_chg[mid].adc > adc)
			high = mid - 1;
		else if (temp_table_chg[mid].adc < adc)
			low = mid + 1;
		else
			break;
	}

	pr_info("[SSP]: %s: adc %d\n", __func__, temp_table_chg[mid].temperature);
	return sprintf(buf, "%d\n", temp_table_chg[mid].temperature);
}

static ssize_t temphumidity_crc_check(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char chTempBuf = 0xff;
	int iRet = 0;
	struct ssp_data *data = dev_get_drvdata(dev);

	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = TEMPHUMIDITY_CRC_FACTORY;
	msg->length = 1;
	msg->options = AP2HUB_READ;
	msg->buffer = &chTempBuf;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Temphumidity check crc Timeout!! %d\n", __func__,
				iRet);
		goto exit;
	}

	pr_info("[SSP] : %s -Check_CRC : %d\n", __func__,
			chTempBuf);

	exit:
	if (chTempBuf == 1)
		return sprintf(buf, "%s\n", "OK");
	else if (chTempBuf == 2)
		return sprintf(buf, "%s\n", "NG_NC");
	else
		return sprintf(buf, "%s\n", "NG");
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
		ssp_send_cmd(data, MSG2SSP_AP_TEMPHUMIDITY_CAL_DONE, 0);
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
static DEVICE_ATTR(cp_thm2, S_IRUGO, pam_adc2_show, NULL);
static DEVICE_ATTR(cp_temperature, S_IRUGO, pam_temp_show, NULL);
static DEVICE_ATTR(cp_temperature2, S_IRUGO, pam_temp2_show, NULL);
static DEVICE_ATTR(mcu_batt_adc, S_IRUGO, hub_batt_adc_show, NULL);
static DEVICE_ATTR(mcu_chg_adc, S_IRUGO, hub_chg_adc_show, NULL);
static DEVICE_ATTR(batt_temperature, S_IRUGO, hub_batt_temp_show, NULL);
static DEVICE_ATTR(chg_temperature, S_IRUGO, hub_chg_temp_show, NULL);
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
	&dev_attr_cp_thm2,
	&dev_attr_cp_temperature,
	&dev_attr_cp_temperature2,
	&dev_attr_mcu_batt_adc,
	&dev_attr_mcu_chg_adc,
	&dev_attr_batt_temperature,
	&dev_attr_chg_temperature,
	&dev_attr_crc_check,
	&dev_attr_send_accuracy,
	NULL,
};

void initialize_temphumidity_factorytest(struct ssp_data *data)
{
	int ret;
	sensors_register(data->temphumidity_device,
		data, temphumidity_attrs, "temphumidity_sensor");

	data->shtc1_device.minor = MISC_DYNAMIC_MINOR;
	data->shtc1_device.name = "shtc1_sensor";
	data->shtc1_device.fops = &ssp_temphumidity_fops;

	ret = misc_register(&data->shtc1_device);
	if (ret < 0) {
		pr_err("register temphumidity misc device err(%d)", ret);
	}

	ssp_vadc = qpnp_get_vadc(&data->spi->dev, "temphumidity_sensor");

	if (IS_ERR(ssp_vadc)) {
		ret = PTR_ERR(ssp_vadc);
		if (ret != -EPROBE_DEFER)
			pr_err("%s: Fail to get vadc %d\n", __func__, ret);
	}

}

void remove_temphumidity_factorytest(struct ssp_data *data)
{
	if (data->comp_engine_ver != NULL)
		kfree(data->comp_engine_ver);
	if (data->comp_engine_ver2 != NULL)
		kfree(data->comp_engine_ver2);
	sensors_unregister(data->temphumidity_device, temphumidity_attrs);
	ssp_temphumidity_fops.unlocked_ioctl = NULL;
	misc_deregister(&data->shtc1_device);

}
