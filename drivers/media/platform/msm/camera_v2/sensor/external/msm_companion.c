/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "msm_companion.h"
#include "msm_cci.h"
#include "msm_camera_io_util.h"
#include "msm_camera_dt_util.h"
#include "msm_camera_i2c_mux.h"
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/msm_cam_sensor.h>
#include <media/msmb_pproc.h>
#include <linux/crc32.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/module.h>

#ifdef BYPASS_COMPANION
#include "isp073Cfw_spi.h"
#include "concordmaster_spi.h"
#include "mode1_spi.h"  // for stats2
//#include "3_Mode2_8.h"  // for stats1
#else
#ifndef MSM_CC_FW
#include "isp073Cfw2_spi.h"
#include "concordmaster2_spi.h"
#include "3_Mode2_1.h"  // for stats2
//#include "3_Mode2_8.h"  // for stats1
#else
#include <linux/vmalloc.h>
#endif
#endif
//#define CONFIG_MSMB_CAMERA_DEBUG
//#define CONFIG_SPI_TRANS_DEBUG
#define BURST_WRITE_ENABLE
#define USE_STATIC_BUF
#ifdef USE_STATIC_BUF
#define COMPANION_FW_MAX_SIZE 131072   // 128k
#endif

#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args ...) pr_err(fmt, ## args)
#else
#define CDBG(fmt, args ...) do { } while (0)
#endif

#undef CDBG_SPI
#ifdef CONFIG_SPI_TRANS_DEBUG
#define CDBG_SPI(fmt, args ...) pr_err(fmt, ## args)
#else
#define CDBG_SPI(fmt, args ...) do { } while (0)
#endif

#undef CDBG_FW
#ifdef MSM_CC_DEBUG
#define CDBG_FW(fmt, args ...) pr_err("[syscamera-fw][%s::%d]"fmt, __FUNCTION__, __LINE__, ## args)
#else
#define CDBG_FW(fmt, args ...) do { } while (0)
#endif

#define MSM_COMP_DRV_NAME                    "msm_companion"
#define MSM_COMP_POLL_RETRIES           20

#if 0
static const char* ISP_COMPANION_BINARY_PATH = "/data/log/CamFW_Companion.bin";
#else
static const char* ISP_COMPANION_BINARY_PATH = "/data/media/0/CamFW_Companion.bin";
#endif


extern void ncp6335b_set_voltage(int version);
#if defined(CONFIG_SEC_S_PROJECT)
extern void fan53555_set_voltage(int version);
#endif
extern int poweroff_charging;

uint8_t fw_name[2][2][64];
#ifdef USE_STATIC_BUF
static u8 spi_isp_buf[COMPANION_FW_MAX_SIZE] = { 0, };
#endif

static int msm_companion_get_crc(struct companion_device *companion_dev, struct companion_crc_check_param crc_param, int callByKernel);

static atomic_t comp_streamon_set;

static int msm_companion_dump_register(struct companion_device *companion_dev, uint8_t *buffer)
{
	struct msm_camera_i2c_client *client = NULL;


	CDBG("[syscamera][%s::%d] Enter\n", __FUNCTION__, __LINE__);

	if (companion_dev) {
		client = &companion_dev->companion_i2c_client;
	} else {
		pr_err("[syscamera][%s::%d]companion_dev is null\n", __FUNCTION__, __LINE__);
		return 0;
	}

	client->i2c_func_tbl->i2c_write(
		client, 0x642c, 0x4000, MSM_CAMERA_I2C_WORD_DATA);
	client->i2c_func_tbl->i2c_write(
		client, 0x642e, 0x6000, MSM_CAMERA_I2C_WORD_DATA);
	client->i2c_func_tbl->i2c_read_multi(
		client, 0x8AFA, buffer);

	return 0;
}

static int msm_companion_get_string_length(uint8_t *str)
{
	int size = 0;

	while (size < 64 &&  str[size] != 0) {
		size++;
	}
	return size;
}

static int msm_companion_append_string(uint8_t *str, int offset, uint8_t *substr, int size)
{
	int i;

	if (str == NULL || substr == NULL) {
		pr_err("[%s::%d] NULL buffer error ! (str = %p, substr = %p)", __FUNCTION__, __LINE__, str, substr);
		return 0;
	}

	if (offset + size > 64 || str == NULL || substr == NULL) {
		pr_err("[%s::%d] string overflow ! (offset = %d, size = %d)", __FUNCTION__, __LINE__, offset, size);
		return 0;
	}

	for (i = 0; i < size; i++)
		str[offset + i] = substr[i];

	return 1;
}

static int msm_companion_fw_binary_set(struct companion_device *companion_dev, struct companion_fw_binary_param fw_bin)
{
	long ret = 0;
	int i;

	// Setting fw binary
	if (fw_bin.size != 0 && fw_bin.buffer != NULL)
	{
	if(companion_dev->eeprom_fw_bin != NULL && companion_dev->eeprom_fw_bin_size > 0)
		{
			pr_info("[syscamera][%s::%d] eeprom_fw_bin is already set (bin_size = %d). return!\n",
				__FUNCTION__, __LINE__, companion_dev->eeprom_fw_bin_size);
		}
		else
		{
			if(companion_dev->eeprom_fw_bin != NULL)
				kfree(companion_dev->eeprom_fw_bin);

			companion_dev->eeprom_fw_bin_size = 0;
			companion_dev->eeprom_fw_bin = NULL;
		// Copy fw binary buffer from user-space area
		companion_dev->eeprom_fw_bin = (uint8_t*)kmalloc(fw_bin.size, GFP_KERNEL);
		if (!companion_dev->eeprom_fw_bin) {
			pr_err("[syscamera][%s::%d][Error] Memory allocation fail\n", __FUNCTION__, __LINE__);
			return -ENOMEM;
		}

		if (copy_from_user(companion_dev->eeprom_fw_bin, fw_bin.buffer, fw_bin.size)) {
			pr_err("[syscamera][%s::%d][Error] buffer from user space\n", __FUNCTION__, __LINE__);
			kfree(companion_dev->eeprom_fw_bin);
			return -EFAULT;
		}
		companion_dev->eeprom_fw_bin_size = fw_bin.size;
		}
	}

	if (copy_from_user(companion_dev->eeprom_fw_ver, fw_bin.version, 12)) {
		pr_err("[syscamera][%s::%d][Error] Failed to copy version info from user-space\n", __FUNCTION__, __LINE__);
		return -EFAULT;
	}
	pr_err("[%s:%d] Version string from EEPROM = %s, bin size = %d", __FUNCTION__, __LINE__, companion_dev->eeprom_fw_ver, companion_dev->eeprom_fw_bin_size);

	//Updating path for fw binary (companion_fw_path + sensor_version + companion_fw_name + sensor_name + extension)
	{
		char *sensor_version, *sensor_name, *extension;
		char version[6];
		int fw_p, fw_n, offset, size;

		// Get sensor version
		if (companion_dev->eeprom_fw_ver[1] == '1' &&
		    companion_dev->eeprom_fw_ver[2] == '6' &&
		    companion_dev->eeprom_fw_ver[3] == 'Q') {
			for (i = 0; i < 5; i++) {
				version[i] = companion_dev->eeprom_fw_ver[i];
			}
			version[5] = 0;
			sensor_version = version;
			pr_err("[%s:%d] Valid hw version info : %s", __FUNCTION__, __LINE__, sensor_version);
		} else {
			sensor_version = FW_DEFAULT_SENSOR_VER;
			pr_err("[%s:%d] Invalid hw version info, using default : %s", __FUNCTION__, __LINE__, sensor_version);
		}

		// Get sensor name
		if (companion_dev->eeprom_fw_ver[4] == 'L') {
			sensor_name = "s5k2p2xx";
		} else if (companion_dev->eeprom_fw_ver[4] == 'S') {
			sensor_name = "imx240";
		} else {
			sensor_name = FW_DEFAULT_SENSOR_NAME;
		}

		//Get extension
		extension = FW_EXTENSION;

		// Creating path table
		for (fw_p = 0; fw_p < FW_PATH_MAX; fw_p++) {
			for (fw_n = 0; fw_n < FW_NAME_MAX; fw_n++) {
				offset = 0;
				size = 0;

				// Add path to stringfw_p
				size = msm_companion_get_string_length(companion_fw_path[fw_p]);
				//pr_err("[syscamera][%s::%d] offset = %d, size = %d\n", __FUNCTION__, __LINE__, offset, size);
				if (msm_companion_append_string(fw_name[fw_p][fw_n], offset, companion_fw_path[fw_p], size) == 0) {
					pr_err("[syscamera][%s::%d][Error] fail to appending path string\n", __FUNCTION__, __LINE__);
					return -EFAULT;
				}
				offset += size;

				if (fw_n != FW_NAME_MASTER) {
					// Add sensor version to string
					size = msm_companion_get_string_length(sensor_version);
					//pr_err("[syscamera][%s::%d] offset = %d, size = %d\n", __FUNCTION__, __LINE__, offset, size);

					if (msm_companion_append_string(fw_name[fw_p][fw_n], offset, sensor_version, size) == 0) {
						pr_err("[syscamera][%s::%d][Error] fail to appending sensor version string\n", __FUNCTION__, __LINE__);
						return -EFAULT;
					}
					offset += size;
				}

				// Add name to string
				size = msm_companion_get_string_length(companion_fw_name[fw_n]);
				//pr_err("[syscamera][%s::%d] offset = %d, size = %d\n", __FUNCTION__, __LINE__, offset, size);
				if (msm_companion_append_string(fw_name[fw_p][fw_n], offset, companion_fw_name[fw_n], size) == 0) {
					pr_err("[syscamera][%s::%d][Error] fail to appending fw name string\n", __FUNCTION__, __LINE__);
					return -EFAULT;
				}
				offset += size;

				if (fw_n != FW_NAME_MASTER) {
					// Add sensor name to string
					size = msm_companion_get_string_length(sensor_name);
					//pr_err("[syscamera][%s::%d] offset = %d, size = %d\n", __FUNCTION__, __LINE__, offset, size);

					if (msm_companion_append_string(fw_name[fw_p][fw_n], offset, sensor_name, size) == 0) {
						pr_err("[syscamera][%s::%d][Error] fail to appending sensor name string\n", __FUNCTION__, __LINE__);
						return -EFAULT;
					}
					offset += size;
				}

				// Add extension to string
				size = msm_companion_get_string_length(extension);
				//pr_err("[syscamera][%s::%d] offset = %d, size = %d\n", __FUNCTION__, __LINE__, offset, size);
				if (msm_companion_append_string(fw_name[fw_p][fw_n], offset, extension, size) == 0) {
					pr_err("[syscamera][%s::%d][Error] fail to appending extension string\n", __FUNCTION__, __LINE__);
					return -EFAULT;
				}
				offset += size;

				if (fw_p == FW_PATH_SD && fw_n == FW_NAME_ISP)
					snprintf(fw_name[fw_p][fw_n], 64, "%s", ISP_COMPANION_BINARY_PATH);

				// print debug message
				pr_err("[syscamera][%s::%d] PathIDX = %d, NameIDX = %d, path = %s\n", __FUNCTION__, __LINE__, fw_p, fw_n, fw_name[fw_p][fw_n]);
			}
		}
	}

	return ret;
}

static int msm_companion_set_cal_tbl(struct companion_device *companion_dev, struct msm_camera_i2c_reg_setting cal_tbl)
{
	pr_err("[syscamera][%s::%d][E]\n", __FUNCTION__, __LINE__);
#if 0
	if (companion_dev->companion_cal_tbl != NULL) {
		kfree(companion_dev->companion_cal_tbl);
		companion_dev->companion_cal_tbl = NULL;
		companion_dev->companion_cal_tbl_size = 0;
		//pr_err("[syscamera][%s::%d] companion_cal_tbl is not NULL. return!\n", __FUNCTION__, __LINE__);
		//return -1;
	}
#else
	if(companion_dev->companion_cal_tbl != NULL) {
		pr_err("[syscamera][%s::%d] companion_cal_tbl is already set. return!\n", __FUNCTION__, __LINE__);
		return 0;
	}
#endif
	// Allocate memory for the calibration table
	companion_dev->companion_cal_tbl = (struct msm_camera_i2c_reg_array *)kmalloc(sizeof(struct msm_camera_i2c_reg_array) * cal_tbl.size, GFP_KERNEL);
	if (!companion_dev->companion_cal_tbl) {
		pr_err("[syscamera][%s::%d] Memory allocation fail\n", __FUNCTION__, __LINE__);
		companion_dev->companion_cal_tbl_size = 0;
		return -ENOMEM;
	}

	// Copy table from user-space area
	if (copy_from_user(companion_dev->companion_cal_tbl, (void*)cal_tbl.reg_setting, sizeof(struct msm_camera_i2c_reg_array) * cal_tbl.size)) {
		pr_err("[syscamera][%s::%d] failed to copy mode table from user-space\n", __FUNCTION__, __LINE__);
		kfree(companion_dev->companion_cal_tbl);
		companion_dev->companion_cal_tbl = NULL;
		companion_dev->companion_cal_tbl_size = 0;
		return -EFAULT;
	}
	companion_dev->companion_cal_tbl_size = cal_tbl.size;

	return 0;
}

static int msm_companion_read_cal_tbl(struct companion_device *companion_dev, uint32_t offset, uint32_t read_size)
{
	struct msm_camera_i2c_client *client = &companion_dev->companion_i2c_client;
	int rc = 0;

	rc = client->i2c_func_tbl->i2c_write(
		client, 0x642C, (offset & 0xFFFF0000) >> 16, MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0)
		pr_err("[syscamera][%s::%d] i2c_write failed.\n", __FUNCTION__, __LINE__);

	rc = client->i2c_func_tbl->i2c_write(
		client, 0x642E, (offset & 0xFFFF), MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0)
		pr_err("[syscamera][%s::%d] i2c_write failed.\n", __FUNCTION__, __LINE__);

	rc = client->i2c_func_tbl->i2c_read_multi(
		client, read_size, cal_data);
	if (rc < 0)
		pr_err("[syscamera][%s::%d] i2c_read_burst failed.\n", __FUNCTION__, __LINE__);

	return 0;
}

static int msm_companion_compare_FW_crc(struct companion_device *companion_dev)
{
	int ret = 0;
	uint32_t crc32;
	struct companion_crc_check_param crc_param;

	//  check crc inside companion FW after upload FW to companion.
	crc_param.addr = 0x0000;
	crc_param.count = companion_dev->loading_fw_bin_size-4;
	crc_param.CRC = &crc32;

	ret = msm_companion_get_crc(companion_dev, crc_param, 1);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d] msm_companion_get_crc failed\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	pr_info("[syscamera][%s::%d] companion : 0x%08X vs AP : 0x%08X\n", __FUNCTION__, __LINE__, *crc_param.CRC, companion_dev->crc_by_ap);

	if(*crc_param.CRC != companion_dev->crc_by_ap)
	{
		msm_camera_fw_check('F', 1); //F: Fail
		pr_err("[syscamera][%s::%d] msm_companion_get_crc failed.\n", __FUNCTION__, __LINE__);
		return -EFAULT;
	}

	return ret;
}

static int msm_companion_cal_data_write(struct companion_device *companion_dev)
{
	int ret = 0; //, idx;
	struct msm_camera_i2c_client *client = &companion_dev->companion_i2c_client;
	struct msm_camera_i2c_reg_array * cal_tbl = companion_dev->companion_cal_tbl;

	pr_err("[syscamera][%s::%d] writing cal table to companion chip hw (cal_tbl_size = %d)\n",
	       __FUNCTION__, __LINE__, companion_dev->companion_cal_tbl_size);

	if (cal_tbl == NULL) {
		pr_err("[syscamera][%s::%d] cal table is empty. returning function.\n", __FUNCTION__, __LINE__);
		return 0;
	}

	// Writing cal data to companion chip hw in burst mode
	ret = client->i2c_func_tbl->i2c_write_burst(client,
						    cal_tbl, companion_dev->companion_cal_tbl_size, MAX_SPI_SIZE);

	return ret;
}

static int msm_companion_pll_init(struct companion_device *companion_dev)
{
	long ret = 0;
	u16 read_value = 0xFFFF;
	void (*set_voltage_function)(int);
	struct msm_camera_i2c_client *client = NULL;
	char isp_core[10];

	pr_err("[syscamera][%s::%d][E]\n", __FUNCTION__, __LINE__);

	if (companion_dev)
		client = &companion_dev->companion_i2c_client;
	else {
		pr_err("[syscamera][%s::%d][ERROR][companion_dev is NULL]\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	// Read Device ID
	ret = client->i2c_func_tbl->i2c_read(client, 0x0000, &read_value, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d][PID::0x%4x][ret::%ld] I2C read fail \n", __FUNCTION__, __LINE__, read_value, ret);
		return -EIO;
	}
	if (read_value != 0x73C1) {
		pr_err("[syscamera][%s::%d][PID::0x%4x] Device ID failed\n", __FUNCTION__, __LINE__, read_value);
		return -EIO;
	}

	//Read BIN_INFO(0x5000 500C)
	read_value = 0x0000;
	ret = client->i2c_func_tbl->i2c_write(client, 0x602C, 0x5000, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d] I2C[0x602C] write fail [rc::%ld]\n", __FUNCTION__, __LINE__, ret);
		return -EIO;
	}
	ret = client->i2c_func_tbl->i2c_write(client, 0x602E, 0x500C, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d] I2C[0x602E] write fail [rc::%ld]\n", __FUNCTION__, __LINE__, ret);
		return -EIO;
	}
	ret = client->i2c_func_tbl->i2c_read(client, 0x6F12, &read_value, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d][0x6F12::0x%04x][ret::%ld] I2C read fail \n", __FUNCTION__, __LINE__, read_value, ret);
		return -EIO;
	}
	pr_err("[syscamera][%s::%d][BIN_INFO::0x%04x]\n", __FUNCTION__, __LINE__, read_value);

#if defined(CONFIG_SEC_S_PROJECT)
	set_voltage_function = &fan53555_set_voltage;
	pr_err("[syscamera][%s::%d] FAN53555\n", __FUNCTION__, __LINE__);
	if (read_value & 0x3F) {
		if (read_value & (1<<CC_BIN1)) {
			(*set_voltage_function)(CC_BIN1);
			strncpy(isp_core, "0.8800", sizeof(isp_core));
		} else if (read_value & (1<<CC_BIN2)) {
			(*set_voltage_function)(CC_BIN2);
			strncpy(isp_core, "0.9000", sizeof(isp_core));
		} else if (read_value & (1<<CC_BIN3)) {
			(*set_voltage_function)(CC_BIN3);
			strncpy(isp_core, "0.9300", sizeof(isp_core));
		} else if (read_value & (1<<CC_BIN4)) {
			(*set_voltage_function)(CC_BIN4);
			strncpy(isp_core, "0.9500", sizeof(isp_core));
		} else if (read_value & (1<<CC_BIN5)) {
			(*set_voltage_function)(CC_BIN5);
			strncpy(isp_core, "0.9800", sizeof(isp_core));
		} else {
			(*set_voltage_function)(CC_BIN6);
			strncpy(isp_core, "1.0000", sizeof(isp_core));
		}
	} else {
		(*set_voltage_function)(CC_BIN6);
		strncpy(isp_core, "1.0000", sizeof(isp_core));
		pr_warn("[syscamera][%s::%d][BIN_INFO::0x%4x]Old hw version \n", __FUNCTION__, __LINE__, read_value);
	}
#else
	set_voltage_function = &ncp6335b_set_voltage;
	pr_err("[syscamera][%s::%d] NCP6335B\n", __FUNCTION__, __LINE__);
	if (read_value & 0x3F) {
		if (read_value & (1<<CC_BIN1)) {
			(*set_voltage_function)(CC_BIN1);
			strncpy(isp_core, "0.8750", sizeof(isp_core));
		} else if (read_value & (1<<CC_BIN2)) {
			(*set_voltage_function)(CC_BIN2);
			strncpy(isp_core, "0.9000", sizeof(isp_core));
		} else if (read_value & (1<<CC_BIN3)) {
			(*set_voltage_function)(CC_BIN3);
			strncpy(isp_core, "0.9250", sizeof(isp_core));
		} else if (read_value & (1<<CC_BIN4)) {
			(*set_voltage_function)(CC_BIN4);
			strncpy(isp_core, "0.9500", sizeof(isp_core));
		} else if (read_value & (1<<CC_BIN5)) {
			(*set_voltage_function)(CC_BIN5);
			strncpy(isp_core, "0.9750", sizeof(isp_core));
		} else {
			(*set_voltage_function)(CC_BIN6);
			strncpy(isp_core, "1.0000", sizeof(isp_core));
		}
	} else {
		(*set_voltage_function)(CC_BIN6);
		strncpy(isp_core, "0.1000", sizeof(isp_core));
		pr_warn("[syscamera][%s::%d][BIN_INFO::0x%4x]Old hw version \n", __FUNCTION__, __LINE__, read_value);
	}
#endif
	pr_info("[syscamera][%s::%d][BIN_INFO:: voltage %s]\n", __FUNCTION__, __LINE__, isp_core);
	msm_camera_write_sysfs(SYSFS_ISP_CORE_PATH, isp_core, sizeof(isp_core));

	//Read Ver
	read_value = 0xFFFF;
	ret = client->i2c_func_tbl->i2c_read(client, 0x0002, &read_value, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d][PID::0x%4x][ret::%ld] I2C read fail \n", __FUNCTION__, __LINE__, read_value, ret);
		return -EIO;
	}

	//PLL Initialize
	if (read_value == 0x00A0) {
		pr_err("[syscamera][%s::%d][Companion EVT 0]\n", __FUNCTION__, __LINE__);
		ret = client->i2c_func_tbl->i2c_write(
			client, 0x0256, 0x0001, MSM_CAMERA_I2C_WORD_DATA);
		companion_version_info = COMP_EVT0;
	} else if (read_value == 0x00B0) {
		pr_err("[syscamera][%s::%d][Companion EVT 1]\n", __FUNCTION__, __LINE__);
		ret = client->i2c_func_tbl->i2c_write(
			client, 0x0122, 0x0001, MSM_CAMERA_I2C_WORD_DATA);
		companion_version_info = COMP_EVT1;
	} else {
		pr_err("[syscamera][%s::%d][Invalid Companion Version : 0x%4x]\n", __FUNCTION__, __LINE__, read_value);
		return -EIO;
	}

	msleep(1);

	//ARM Reset & Memory Remap
	ret = client->i2c_func_tbl->i2c_write(
		client, 0x6042, 0x0001, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d]ARM Reset & Memory Remap failed\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	//  Signature Clear
	ret = client->i2c_func_tbl->i2c_write(
		client, 0x0000, 0xBEEF, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d]Signature Clear failed\n", __FUNCTION__, __LINE__);
		return -EIO;
	}
	return 0;
}

static int msm_companion_release_arm_reset(struct companion_device *companion_dev)
{
	long ret = 0;
	uint16_t read_value = 0xFFFF;
	struct msm_camera_i2c_client *client = NULL;

	pr_err("[syscamera][%s::%d][E]\n", __FUNCTION__, __LINE__);

	if (companion_dev)
		client = &companion_dev->companion_i2c_client;
	else {
		pr_err("[syscamera][%s::%d][ERROR][companion_dev is NULL]\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	//ARM Reset & Memory Remap
	ret = client->i2c_func_tbl->i2c_write(
		client, 0x6014, 0x0001, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d]Release ARM reset failed\n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	ret = client->i2c_func_tbl->i2c_write(
		client, 0x7106, 0x1555, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d] CRC CMD write Fail \n", __FUNCTION__, __LINE__);
		return -EIO;
	}

	usleep(500);

	//  Check Device ID again
	read_value = 0xFFFF;
	ret = client->i2c_func_tbl->i2c_read(client, 0x0000, &read_value, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d][PID::0x%4x][ret::%ld] I2C read fail\n", __FUNCTION__, __LINE__, read_value, ret);
		return -EIO;
	}

	if(read_value != 0x73C1)
	{
		pr_err("[syscamera][%s::%d][Device ID::0x%04x] Device ID is not 0x73C1.\n", __FUNCTION__, __LINE__, read_value);
		return -EFAULT;
	}
	return 0;
}

int msm_companion_sysfs_fw_version_write(const char* eeprom_ver, const char* phone_ver, const char* load_ver)
{
	int ret = 0;
	char fw_ver[37] = "NULL NULL NULL\n";

	if(strcmp(phone_ver, "") == 0)
		snprintf(fw_ver, sizeof(fw_ver), "%s NULL %s\n", eeprom_ver, load_ver);
	else
		snprintf(fw_ver, sizeof(fw_ver), "%s %s %s\n", eeprom_ver, phone_ver, load_ver);

	pr_err("%s:[FW_DBG][EEPROM/PHONE/LOAD] %s", __func__, fw_ver);

	ret = msm_camera_write_sysfs(SYSFS_COMP_FW_PATH, fw_ver, sizeof(fw_ver));
	if (ret < 0) {
		pr_err("%s: msm_camera_write_sysfs failed.", __func__);
		ret = -1;
	}

	return ret;
}

static int msm_companion_fw_write(struct companion_device *companion_dev)
{
#ifdef MSM_CC_DEBUG
	uint16_t data = 0;
#endif
	long ret = 0;
	int rc = 0;
	struct file *isp_filp = NULL;
	u8 *isp_vbuf = NULL, *isp_fbuf = NULL, *buf_backup = NULL;
#ifndef USE_STATIC_BUF
	u8 *spi_isp_buf = NULL;
#endif
	u32 spi_isp_buf_size = 0;
	u32 isp_size = 0, isp_vsize = 0, isp_fsize = 0, size_backup = 0;
	struct msm_camera_i2c_client *client = NULL;
	mm_segment_t old_fs;

	u8 fs_fw_version[12] = "NULL";
	int i, isEepromFwUsed = 0;

	struct spi_message m;
	struct spi_transfer tx;
	char *sd_fw_isp_path = NULL;
	char *cc_fw_isp_path = NULL;
	uint8_t iter = 0, crc_pass = 0;
	uint32_t crc_cal = ~0UL;

	if (companion_dev == NULL) {
		pr_err("[syscamera][%s::%d][companion_dev is NULL]\n", __FUNCTION__, __LINE__);
		return -EIO;
	}
	client = &companion_dev->companion_i2c_client;
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (companion_version_info == COMP_EVT0) {
		pr_err("[syscamera][%s::%d][Companion EVT 0]\n", __FUNCTION__, __LINE__);
//		fw_name[FW_PATH_SD][FW_NAME_ISP][REV_OFFSET_ISP_SD] = '0';
		fw_name[FW_PATH_CC][FW_NAME_ISP][REV_OFFSET_ISP_CC] = '0';

		fw_name[FW_PATH_SD][FW_NAME_MASTER][REV_OFFSET_MASTER_SD] = '0';
		fw_name[FW_PATH_CC][FW_NAME_MASTER][REV_OFFSET_MASTER_CC] = '0';
	} else if (companion_version_info == COMP_EVT1) {
		pr_err("[syscamera][%s::%d][Companion EVT 1]\n", __FUNCTION__, __LINE__);
//		fw_name[FW_PATH_SD][FW_NAME_ISP][REV_OFFSET_ISP_SD] = '1';
		fw_name[FW_PATH_CC][FW_NAME_ISP][REV_OFFSET_ISP_CC] = '1';

		fw_name[FW_PATH_SD][FW_NAME_MASTER][REV_OFFSET_MASTER_SD] = '1';
		fw_name[FW_PATH_CC][FW_NAME_MASTER][REV_OFFSET_MASTER_CC] = '1';
	} else {
		pr_err("[syscamera][%s::%d][Invalid Companion Version : %d]\n", __FUNCTION__, __LINE__, companion_version_info);
		return -EIO;
	}
	sd_fw_isp_path = fw_name[FW_PATH_SD][FW_NAME_ISP];      // SD_FW_EVT0_ISP_PATH;
	cc_fw_isp_path = fw_name[FW_PATH_CC][FW_NAME_ISP];      // CC_FW_EVT0_ISP_PATH;
	pr_err("[syscamera][%s::%d] sd path = %s, cc path = %s\n", __FUNCTION__, __LINE__, sd_fw_isp_path, cc_fw_isp_path);

	for (iter = 0; iter < 3; iter++) {
		isp_filp = filp_open(sd_fw_isp_path, O_RDONLY, 0);
		if (IS_ERR(isp_filp)) {
			pr_err("[syscamera]%s does not exist, err %ld, search next path.\n",
			       sd_fw_isp_path, PTR_ERR(isp_filp));
			isp_filp = filp_open(cc_fw_isp_path, O_RDONLY, 0);
			if (IS_ERR(isp_filp)) {
				pr_err("[syscamera]failed to open %s, err %ld\n",
				       cc_fw_isp_path, PTR_ERR(isp_filp));
				isp_filp = NULL;
				goto isp_check_multimodule;
			} else {
				pr_err("[syscamera]open success : %s\n", cc_fw_isp_path);
			}
		} else {
			pr_err("[syscamera]open success : %s\n", sd_fw_isp_path);
		}

		isp_size = isp_filp->f_path.dentry->d_inode->i_size;
		isp_fsize = isp_size - isp_vsize;
		CDBG_FW("[syscamera]ISP size %d, fsize %d Bytes\n", isp_size, isp_fsize);

		/* version info is located at the end of 16byte of the buffer. */
		isp_vbuf = vmalloc(isp_size);
		isp_filp->f_pos = 0;
		ret = vfs_read(isp_filp, (char __user*)isp_vbuf, isp_size, &isp_filp->f_pos);
		if (ret != isp_size) {
			err("failed to read Concord info, %ld Bytes\n", ret);
			ret = -EIO;
			goto isp_filp_verr_iter;
		}

		/* Isp set */
		isp_fbuf = vmalloc(isp_fsize);
		isp_filp->f_pos = 0;

		ret = vfs_read(isp_filp, (char __user*)isp_fbuf, isp_fsize, &isp_filp->f_pos);
		if (ret != isp_fsize) {
			err("failed to read Isp, %ld Bytes\n", ret);
			ret = -EIO;
			goto isp_filp_ferr_iter;
		}
#ifdef MSM_CC_DEBUG
		for (arr_idx = 0; arr_idx < isp_fsize; arr_idx++) {
			printk("%02x", isp_fbuf[arr_idx]);
			if (((arr_idx % 8) == 0) && (arr_idx != 0)) {
				printk("\n");
			}
		}
		printk("\n");
#endif

		// Version from file-system
		for (i = 0; i < 11; i++)
			fs_fw_version[i] = isp_vbuf[isp_size - 16 + i];
		fs_fw_version[11] = 0;

		crc_cal = ~0UL;
		crc_cal = crc32_le(crc_cal, isp_vbuf, isp_size - 4);
		crc_cal = ~crc_cal;
		companion_dev->crc_by_ap = crc_cal;
		companion_dev->loading_fw_bin_size = isp_size;
		pr_err("[syscamera][%s::%d][companion_dev = %p size = %d crc_by_ap = 0x%08X]\n", __FUNCTION__, __LINE__, companion_dev, companion_dev->loading_fw_bin_size, companion_dev->crc_by_ap);
		pr_err("[syscamera][%s::%d][crc_cal = 0x%08X, expected = 0x%08X]\n", __FUNCTION__, __LINE__, crc_cal, *(uint32_t*)(&isp_vbuf[isp_size - 4]));
		if (crc_cal != *(uint32_t*)(&isp_vbuf[isp_size - 4])) {
			pr_err("[syscamera][%s::%d][Err::CRC32 is not correct. iter = %d(max=3)]\n", __FUNCTION__, __LINE__, iter);
			msm_camera_fw_check('F', 1); //F: Fail
 isp_filp_ferr_iter:
			if (isp_fbuf) {
				vfree(isp_fbuf);
				isp_fbuf = NULL;
			} else {
				pr_err("[syscamera][%s::%d][Err::isp_fbuf is NULL]\n", __FUNCTION__, __LINE__);
			}
 isp_filp_verr_iter:
			if (isp_vbuf) {
				vfree(isp_vbuf);
				isp_vbuf = NULL;
			} else {
				pr_err("[syscamera][%s::%d][Err::isp_vbuf is NULL]\n", __FUNCTION__, __LINE__);
			}
		} else {
			crc_pass = 1;
			pr_err("[syscamera][%s::%d][CRC32 is correct. iter = %d(max=3)]\n", __FUNCTION__, __LINE__, iter);
			break;
		}
	}

	if (crc_pass == 0)
		goto isp_filp_ferr;

	// Multi module support
isp_check_multimodule:
	pr_err("[syscamera][%s::%d][fs version = %s, eeprom version = %s]\n", __FUNCTION__, __LINE__, fs_fw_version, companion_dev->eeprom_fw_ver);
	if (companion_dev->eeprom_fw_bin != NULL && companion_dev->eeprom_fw_bin_size != 0) {
		// HW version check
		for (i = 0; i < 5; i++)
			if (fs_fw_version[i] != companion_dev->eeprom_fw_ver[i])
				isEepromFwUsed = 1;

		// SW version check
		if (isEepromFwUsed != 1) {
			for (i = 5; i < 9; i++) {
				if (fs_fw_version[i] != companion_dev->eeprom_fw_ver[i]) {
					if (fs_fw_version[i] < companion_dev->eeprom_fw_ver[i]) {
						isEepromFwUsed = 1;
						break;
					} else {
						isEepromFwUsed = 0;
						break;
					}
				}
			}
		}
	} else {
		isEepromFwUsed = 0;
	}

	pr_err("[syscamera][%s::%d][fs version = %s, eeprom version = %s, isEepromUsed = %d]\n", __FUNCTION__, __LINE__,
	       fs_fw_version, companion_dev->eeprom_fw_ver, isEepromFwUsed);

	if (isEepromFwUsed) {
		buf_backup = isp_fbuf;
		size_backup = isp_fsize;

		crc_cal = ~0UL;
		crc_cal = crc32_le(crc_cal, companion_dev->eeprom_fw_bin, companion_dev->eeprom_fw_bin_size - 4);
		crc_cal = ~crc_cal;
		companion_dev->crc_by_ap = crc_cal;
		companion_dev->loading_fw_bin_size = companion_dev->eeprom_fw_bin_size;
		pr_info("[syscamera][%s::%d][EEPROM companion_dev = %p size = %d crc_by_ap = 0x%08X]\n", __FUNCTION__, __LINE__, companion_dev, companion_dev->loading_fw_bin_size, companion_dev->crc_by_ap);
		pr_err("[syscamera][%s::%d][EEPROM Companion FW crc_cal = 0x%08X, expected = 0x%08X]\n",
		       __FUNCTION__, __LINE__, crc_cal, *(uint32_t*)(&(companion_dev->eeprom_fw_bin[companion_dev->eeprom_fw_bin_size - 4])));

		if (crc_cal != *(uint32_t*)(&companion_dev->eeprom_fw_bin[companion_dev->eeprom_fw_bin_size - 4])) {
			pr_err("[syscamera][%s::%d][Err::EEPROM Companion FW CRC32 is not correct. \n", __FUNCTION__, __LINE__);
			msm_camera_fw_check('F', 1); //F: Fail
			goto isp_filp_ferr;
		}

		isp_fbuf = companion_dev->eeprom_fw_bin;
		isp_fsize = companion_dev->eeprom_fw_bin_size;
		pr_err("[syscamera][%s::%d][fw from eeprom will be used !]\n", __FUNCTION__, __LINE__);
		msm_companion_sysfs_fw_version_write(companion_dev->eeprom_fw_ver, fs_fw_version, companion_dev->eeprom_fw_ver);
	} else {
		pr_err("[syscamera][%s::%d][fw from phone will be used !]\n", __FUNCTION__, __LINE__);
		msm_companion_sysfs_fw_version_write(companion_dev->eeprom_fw_ver, fs_fw_version, fs_fw_version);
	}

	ret = client->i2c_func_tbl->i2c_write(
		client, 0x6428, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera]%s: Isp1 init failed\n", __func__);
		ret = -EIO;
		goto isp_filp_ferr;
	}
	ret = client->i2c_func_tbl->i2c_write(
		client, 0x642A, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera]%s: Isp2 init failed\n", __func__);
		ret = -EIO;
		goto isp_filp_ferr;
	}

	if (isp_fsize > COMPANION_FW_MAX_SIZE - 3) {
		pr_err("[syscamera]%s: FW file size is bigger than buffer size\n", __func__);
		ret = -EIO;
		goto isp_filp_ferr;
	}
	spi_isp_buf_size = isp_fsize + 3;
#ifndef USE_STATIC_BUF
	spi_isp_buf = kmalloc(spi_isp_buf_size, GFP_KERNEL);

	if (!spi_isp_buf) {
		pr_err("[syscamera][%s::%d][Err::kmalloc spi_isp_buf is NULL]\n", __FUNCTION__, __LINE__);
		ret = -EIO;
		goto isp_filp_ferr;
	}
#endif

	spi_isp_buf[0] = 0x02;
	spi_isp_buf[1] = 0x6F;
	spi_isp_buf[2] = 0x12;
	memcpy(spi_isp_buf + 3, isp_fbuf, isp_fsize);
	memset(&tx, 0, sizeof(struct spi_transfer));
	tx.tx_buf = spi_isp_buf;
	tx.len = spi_isp_buf_size;

	spi_message_init(&m);
	spi_message_add_tail(&tx, &m);
	rc = spi_sync(client->spi_client->spi_master, &m);

#ifndef USE_STATIC_BUF
	kfree(spi_isp_buf);
	spi_isp_buf = NULL;
#endif

	if (isEepromFwUsed) {
		isp_fbuf = buf_backup;
		isp_fsize = size_backup;
	}

	if (isp_filp) {
		filp_close(isp_filp, NULL);
		isp_filp = NULL;
	} else {
		pr_err("[syscamera][%s::%d][Err::isp_filp is NULL]\n", __FUNCTION__, __LINE__);
	}

	if (isp_vbuf) {
		vfree(isp_vbuf);
		isp_vbuf = NULL;
	} else {
		pr_err("[syscamera][%s::%d][Err::isp_vbuf is NULL]\n", __FUNCTION__, __LINE__);
	}

	if (isp_fbuf) {
		vfree(isp_fbuf);
		isp_fbuf = NULL;
	} else {
		pr_err("[syscamera][%s::%d][Err::isp_fbuf is NULL]\n", __FUNCTION__, __LINE__);
	}

	/* restore kernel memory setting */
	set_fs(old_fs);
	return 0;
 isp_filp_ferr:
//isp_filp_verr:
#ifndef USE_STATIC_BUF
	if (spi_isp_buf) {
		kfree(spi_isp_buf);
		spi_isp_buf = NULL;
	}
#endif

	if (isEepromFwUsed) {
		isp_fbuf = buf_backup;
		isp_fsize = size_backup;
	}

	if (isp_filp) {
		filp_close(isp_filp, NULL);
		isp_filp = NULL;
	} else {
		pr_err("[syscamera][%s::%d][Err::isp_filp is NULL]\n", __FUNCTION__, __LINE__);
	}

	if (isp_vbuf) {
		vfree(isp_vbuf);
		isp_vbuf = NULL;
	} else {
		pr_err("[syscamera][%s::%d][Err::isp_vbuf is NULL]\n", __FUNCTION__, __LINE__);
	}

	if (isp_fbuf) {
		vfree(isp_fbuf);
		isp_fbuf = NULL;
	} else {
		pr_err("[syscamera][%s::%d][Err::isp_fbuf is NULL]\n", __FUNCTION__, __LINE__);
	}

	/* restore kernel memory setting */
	set_fs(old_fs);
	return -EFAULT;
}

static int msm_companion_master_write(struct companion_device *companion_dev)
{
	long ret = 0;
	struct file *cc_filp = NULL;
	u8 *cc_vbuf = NULL, *cc_fbuf = NULL;
	u32 cc_size = 0, cc_vsize = 16, cc_fsize = 0;
	uint16_t addr = 0, data = 0;
	struct msm_camera_i2c_client *client = NULL;
	mm_segment_t old_fs;

	u32 arr_idx = 0;

	client = &companion_dev->companion_i2c_client;
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cc_filp = filp_open(fw_name[FW_PATH_SD][FW_NAME_MASTER], O_RDONLY, 0);
	if (IS_ERR(cc_filp)) {
		pr_err("[syscamera]%s does not exist, err %ld, search next path.\n",
		       fw_name[FW_PATH_SD][FW_NAME_MASTER], PTR_ERR(cc_filp));
		cc_filp = filp_open(fw_name[FW_PATH_CC][FW_NAME_MASTER], O_RDONLY, 0);
		if (IS_ERR(cc_filp)) {
			pr_err("[syscamera]failed to open %s, err %ld\n",
			       fw_name[FW_PATH_CC][FW_NAME_MASTER], PTR_ERR(cc_filp));
			goto cc_filp_ferr;
		} else {
			pr_err("[syscamera]open success : %s\n", fw_name[FW_PATH_CC][FW_NAME_MASTER]);
		}
	} else {
		pr_err("[syscamera]open success : %s\n", fw_name[FW_PATH_SD][FW_NAME_MASTER]);
	}

	if (!cc_filp) {
		pr_err("cc_flip is NULL\n");
		goto cc_filp_ferr;
	}

	cc_size = cc_filp->f_path.dentry->d_inode->i_size;
	cc_fsize = cc_size - cc_vsize;
	CDBG_FW("[syscamera]concord size %d, fsize %d Bytes\n", cc_size, cc_fsize);

	/* version & setfile info */
	cc_vbuf = vmalloc(cc_vsize + 1);
	memset(cc_vbuf, 0x00, cc_vsize + 1);
	cc_filp->f_pos = cc_fsize;
	ret = vfs_read(cc_filp, (char __user*)cc_vbuf, cc_vsize, &cc_filp->f_pos);
	if (ret != cc_vsize) {
		err("failed to read Concord info, %ld Bytes\n", ret);
		ret = -EIO;
		goto cc_filp_verr;
	}
	CDBG_FW("[master-version]%s\n", cc_vbuf);
	/* Concord set */
	cc_fbuf = vmalloc(cc_fsize);
	cc_filp->f_pos = 0;     //swap
	ret = vfs_read(cc_filp, (char __user*)cc_fbuf, cc_fsize, &cc_filp->f_pos);
	if (ret != cc_fsize) {
		err("failed to read Concord, %ld Bytes\n", ret);
		ret = -EIO;
		goto cc_filp_ferr;
	}
#ifdef MSM_CC_DEBUG
	for (arr_idx = 0; arr_idx < cc_fsize; arr_idx++) {
		printk("%02x", cc_fbuf[arr_idx]);
		if (((arr_idx % 15) == 0) && (arr_idx != 0)) {
			printk("\n");
		}
	}
	printk("\n");
#endif
	if (cc_fsize % 4 == 0) {
		for (arr_idx = 0; arr_idx < cc_fsize; arr_idx += 4) {
			addr = (((cc_fbuf[arr_idx] << 8) & 0xFF00) | ((cc_fbuf[arr_idx + 1] << 0) & 0x00FF));
			data = (((cc_fbuf[arr_idx + 2] << 8) & 0xFF00) | ((cc_fbuf[arr_idx + 3] << 0) & 0x00FF));
			CDBG_FW("[syscamera]addr : %04x\n", addr);
			CDBG_FW(" data : %04x\n", data);

			ret = client->i2c_func_tbl->i2c_write(
				client, addr, data, MSM_CAMERA_I2C_WORD_DATA);
			if (ret < 0) {
				pr_err("[syscamera]%s: Concord failed\n", __func__);
				ret = -EIO;
				goto cc_filp_ferr;
			}
		}
	} else {
		pr_err("[syscamera]error : The size of Master set file should be multiple of 4. (size = %d byte)", cc_fsize);
		ret = -EIO;
		goto cc_filp_ferr;
	}

	if (cc_filp) {
		filp_close(cc_filp, NULL);
		cc_filp = NULL;
	} else {
		pr_err("[syscamera][%s::%d][Err::cc_filp is NULL]\n", __FUNCTION__, __LINE__);
	}

	if (cc_vbuf) {
		vfree(cc_vbuf);
		cc_vbuf = NULL;
	} else {
		pr_err("[syscamera][%s::%d][Err::cc_vbuf is NULL]\n", __FUNCTION__, __LINE__);
	}

	if (cc_fbuf) {
		vfree(cc_fbuf);
		cc_fbuf = NULL;
	} else {
		pr_err("[syscamera][%s::%d][Err::cc_fbuf is NULL]\n", __FUNCTION__, __LINE__);
	}

	/* restore kernel memory setting */
	set_fs(old_fs);
	return 0;

 cc_filp_ferr:
 cc_filp_verr:
	if (cc_filp) {
		filp_close(cc_filp, NULL);
		cc_filp = NULL;
	} else {
		pr_err("[syscamera][%s::%d][Err::cc_filp is NULL]\n", __FUNCTION__, __LINE__);
	}

	if (cc_vbuf) {
		vfree(cc_vbuf);
		cc_vbuf = NULL;
	} else {
		pr_err("[syscamera][%s::%d][Err::cc_vbuf is NULL]\n", __FUNCTION__, __LINE__);
	}

	if (cc_fbuf) {
		vfree(cc_fbuf);
		cc_fbuf = NULL;
	} else {
		pr_err("[syscamera][%s::%d][Err::cc_fbuf is NULL]\n", __FUNCTION__, __LINE__);
	}

	/* restore kernel memory setting */
	set_fs(old_fs);
	return -EFAULT;
}

static int msm_companion_get_crc(struct companion_device *companion_dev, struct companion_crc_check_param crc_param, int callByKernel)
{
	int ret = 0;
	struct msm_camera_i2c_client *client = &companion_dev->companion_i2c_client;
	int i;
	uint16_t crc_high, crc_low;
	uint32_t crc;

	//pr_err("[syscamera][%s::%d][E]\n", __FUNCTION__, __LINE__);

	// 1. Reset CRC32
	ret += client->i2c_func_tbl->i2c_write(
		client, 0x0024, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	ret += client->i2c_func_tbl->i2c_write(
		client, 0x0026, 0x0000, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d][Error on creating crc]\n", __FUNCTION__, __LINE__);
		return ret;
	}

	// 2. Set address
	ret += client->i2c_func_tbl->i2c_write(
		client, 0x0014, (crc_param.addr & 0xFFFF0000) >> 16, MSM_CAMERA_I2C_WORD_DATA);
	ret += client->i2c_func_tbl->i2c_write(
		client, 0x0016, (crc_param.addr & 0x0000FFFF), MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d][Error on creating crc]\n", __FUNCTION__, __LINE__);
		return ret;
	}

	// 3. Set count
	ret += client->i2c_func_tbl->i2c_write(
		client, 0x0018, (crc_param.count & 0xFFFF0000) >> 16, MSM_CAMERA_I2C_WORD_DATA);
	ret += client->i2c_func_tbl->i2c_write(
		client, 0x001A, (crc_param.count & 0x0000FFFF), MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d][Error on creating crc]\n", __FUNCTION__, __LINE__);
		return ret;
	}

	// 4. Set sflash cmd
	ret += client->i2c_func_tbl->i2c_write(
		client, 0x000C, 0x000C, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d][Error on creating crc]\n", __FUNCTION__, __LINE__);
		return ret;
	}

	// 5. Set host intrp flash cmd
	ret += client->i2c_func_tbl->i2c_write(
		client, 0x6806, 0x0001, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d][Error on creating crc]\n", __FUNCTION__, __LINE__);
		return ret;
	}

	// 6. Polling sflash
	for (i = 0; i < 50; i++) {
		uint16_t polling = 0;
		ret += client->i2c_func_tbl->i2c_read(
			client, 0x000c, &polling, MSM_CAMERA_I2C_WORD_DATA);
		if (polling == 0) {
			//pr_err("[syscamera][%s::%d] break the loop after %d tries.\n", __FUNCTION__, __LINE__, i);
			break;
		}
		usleep(200);
	}
	if (ret < 0) {
		pr_err("[syscamera][%s::%d][Error on creating crc]\n", __FUNCTION__, __LINE__);
		return ret;
	}

	// 7. Read CRC32
	ret += client->i2c_func_tbl->i2c_read(
		client, 0x0024, &crc_high, MSM_CAMERA_I2C_WORD_DATA);
	ret += client->i2c_func_tbl->i2c_read(
		client, 0x0026, &crc_low, MSM_CAMERA_I2C_WORD_DATA);
	if (ret < 0) {
		pr_err("[syscamera][%s::%d][Error on creating crc]\n", __FUNCTION__, __LINE__);
		return ret;
	}

	// 8. Return CRC32
	crc = (((uint32_t)crc_high) << 16) + crc_low;
	if(callByKernel == 1)
	{
		memcpy(crc_param.CRC, &crc, sizeof(crc));
	}
	else
	{
		ret = copy_to_user(crc_param.CRC, &crc, sizeof(crc));
		if(ret < 0) {
			pr_err("[syscamera][%s::%d][Error on copy to user]\n", __FUNCTION__, __LINE__);
			return ret;
		}
	}

	//pr_info("[syscamera][%s::%d] CRC = 0x%08X[X]\n", __FUNCTION__, __LINE__, crc);

	return ret;
}

static int msm_companion_stream_on(struct msm_camera_i2c_client *companion_i2c_dev, uint16_t enable)
{
	int ret = 0;

	pr_err("[syscamera][%s::%d][E][enable::%d]\n", __FUNCTION__, __LINE__, enable);

#if 1   // a clk control needs to be enabled 03.Dec.2013
	ret = companion_i2c_dev->i2c_func_tbl->i2c_write(
		companion_i2c_dev, 0x6800, enable, MSM_CAMERA_I2C_WORD_DATA);
#endif

	if (ret < 0) {
		pr_err("[syscamera][%s::%d][Error on streaming control][enable::%d]\n", __FUNCTION__, __LINE__, enable);
	}

	return ret;
}

static int msm_companion_set_mode(struct companion_device *companion_dev, struct msm_camera_i2c_reg_setting mode_setting)
{
	int ret = 0, idx;
	struct msm_camera_i2c_client *client = &companion_dev->companion_i2c_client;
	struct msm_camera_i2c_reg_array * mode_tbl;

	// Allocate memory for the mode table
	mode_tbl = (struct msm_camera_i2c_reg_array *)kmalloc(sizeof(struct msm_camera_i2c_reg_array) * mode_setting.size, GFP_KERNEL);
	if (!mode_tbl) {
		pr_err("[syscamera][%s::%d][Error] Memory allocation fail\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	// Copy table from user-space area
	if (copy_from_user(mode_tbl, (void*)mode_setting.reg_setting, sizeof(struct msm_camera_i2c_reg_array) * mode_setting.size)) {
		pr_err("[syscamera][%s::%d][Error] Failed to copy mode table from user-space\n", __FUNCTION__, __LINE__);
		kfree(mode_tbl);
		return -EFAULT;
	}

	// Mode setting
	for (idx = 0; idx < mode_setting.size; idx++) {

		if (mode_tbl[idx].delay) {
			msleep(mode_tbl[idx].delay);
			pr_err("[syscamera][%s::%d] delay : %d ms\n", __FUNCTION__, __LINE__, mode_tbl[idx].delay);
		}

		ret = client->i2c_func_tbl->i2c_write(
			client, mode_tbl[idx].reg_addr, mode_tbl[idx].reg_data, MSM_CAMERA_I2C_WORD_DATA);
		if (ret < 0) {
			pr_err("[syscamera][%s::%d] mode setting failed\n", __FUNCTION__, __LINE__);
			ret = 0;
		}
		pr_err("[syscamera][%s::%d][idx::%d][reg_addr::0x%4x][reg_data::0x%4x]\n", __FUNCTION__, __LINE__, idx,
		       mode_tbl[idx].reg_addr, mode_tbl[idx].reg_data);
	}

	// Releasing memory for the mode table
	kfree(mode_tbl);

	return ret;
}

static int msm_companion_aec_update(struct companion_device *companion_dev, struct msm_camera_i2c_reg_setting mode_setting)
{
	int ret = 0, idx;
	struct msm_camera_i2c_client *client = &companion_dev->companion_i2c_client;
	struct msm_camera_i2c_reg_array * mode_tbl;

	// Allocate memory for the mode table
	mode_tbl = (struct msm_camera_i2c_reg_array *)kmalloc(sizeof(struct msm_camera_i2c_reg_array) * mode_setting.size, GFP_KERNEL);
	if (!mode_tbl) {
		pr_err("[syscamera][%s::%d][Error] Memory allocation fail\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	// Copy table from user-space area
	if (copy_from_user(mode_tbl, (void*)mode_setting.reg_setting, sizeof(struct msm_camera_i2c_reg_array) * mode_setting.size)) {
		pr_err("[syscamera][%s::%d][Error] Failed to copy aec update table from user-space\n", __FUNCTION__, __LINE__);
		kfree(mode_tbl);
		return -EFAULT;
	}

	// Mode setting
	for (idx = 0; idx < mode_setting.size; idx++) {
		ret = client->i2c_func_tbl->i2c_write(
			client, mode_tbl[idx].reg_addr, mode_tbl[idx].reg_data, MSM_CAMERA_I2C_WORD_DATA);
		if (ret < 0) {
			pr_err("[syscamera][%s::%d] aec update failed\n", __FUNCTION__, __LINE__);
			ret = 0;
		}
		CDBG("[syscamera][%s::%d][idx::%d][reg_addr::0x%4x][reg_data::0x%4x]\n", __FUNCTION__, __LINE__, idx,
		     mode_tbl[idx].reg_addr, mode_tbl[idx].reg_data);
	}

#if 0
	// Delay for 2 ms
	msleep(2);

	// read data
	for (idx = 0; idx < mode_setting.size; idx++) {
		uint16_t readval;
		ret = client->i2c_func_tbl->i2c_read(
			client, mode_tbl[idx].reg_addr, &readval, MSM_CAMERA_I2C_WORD_DATA);
		if (ret < 0) {
			pr_err("[syscamera][%s::%d] i2c read failed\n", __FUNCTION__, __LINE__);
			ret = 0;
		}
		pr_err("[syscamera][%s::%d][idx::%d][reg_addr::0x%4x][read_data::0x%4x]\n", __FUNCTION__, __LINE__, idx,
		       mode_tbl[idx].reg_addr, readval);
	}
#endif

	// Releasing memory for the mode table
	kfree(mode_tbl);

	return ret;
}

static int msm_companion_awb_update(struct companion_device *companion_dev, struct msm_camera_i2c_reg_setting mode_setting)
{
	int ret = 0, idx;
	struct msm_camera_i2c_client *client = &companion_dev->companion_i2c_client;
	struct msm_camera_i2c_reg_array * mode_tbl;

	// Allocate memory for the mode table
	mode_tbl = (struct msm_camera_i2c_reg_array *)kmalloc(sizeof(struct msm_camera_i2c_reg_array) * mode_setting.size, GFP_KERNEL);
	if (!mode_tbl) {
		pr_err("%s,%d Memory allocation fail\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	// Copy table from user-space area
	if (copy_from_user(mode_tbl, (void*)mode_setting.reg_setting, sizeof(struct msm_camera_i2c_reg_array) * mode_setting.size)) {
		pr_err("%s,%d failed to copy awb update table from user-space\n", __FUNCTION__, __LINE__);
		kfree(mode_tbl);
		return -EFAULT;
	}

	// Mode setting
	for (idx = 0; idx < mode_setting.size; idx++) {
		ret = client->i2c_func_tbl->i2c_write(
			client, mode_tbl[idx].reg_addr, mode_tbl[idx].reg_data, MSM_CAMERA_I2C_WORD_DATA);
		if (ret < 0) {
			pr_err("[syscamera][%s::%d] awb update failed\n", __FUNCTION__, __LINE__);
			ret = 0;
		}
		CDBG("[syscamera][%s::%d][idx::%d][reg_addr::0x%4x][reg_data::0x%4x]\n", __FUNCTION__, __LINE__, idx,
		     mode_tbl[idx].reg_addr, mode_tbl[idx].reg_data);
	}

	/* for test */
#if 0
	// Delay for 2 ms
	msleep(2);

	// read data
	for (idx = 0; idx < mode_setting.size; idx++) {
		uint16_t readval;
		ret = client->i2c_func_tbl->i2c_read(
			client, mode_tbl[idx].reg_addr, &readval, MSM_CAMERA_I2C_WORD_DATA);
		if (ret < 0) {
			pr_err("[syscamera][%s::%d] i2c read failed\n", __FUNCTION__, __LINE__);
			ret = 0;
		}
		pr_err("[syscamera][%s::%d][idx::%d][reg_addr::0x%4x][read_data::0x%4x]\n", __FUNCTION__, __LINE__, idx,
		       mode_tbl[idx].reg_addr, readval);
	}
#endif

	// Releasing memory for the mode table
	kfree(mode_tbl);

	return ret;
}

static int msm_companion_af_update(struct companion_device *companion_dev, struct msm_camera_i2c_reg_setting mode_setting)
{
	int ret = 0, idx;
	struct msm_camera_i2c_client *client = &companion_dev->companion_i2c_client;
	struct msm_camera_i2c_reg_array * mode_tbl;

	// Allocate memory for the mode table
	mode_tbl = (struct msm_camera_i2c_reg_array *)kmalloc(sizeof(struct msm_camera_i2c_reg_array) * mode_setting.size, GFP_KERNEL);
	if (!mode_tbl) {
		pr_err("%s,%d Memory allocation fail\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	// Copy table from user-space area
	if (copy_from_user(mode_tbl, (void*)mode_setting.reg_setting, sizeof(struct msm_camera_i2c_reg_array) * mode_setting.size)) {
		pr_err("%s,%d failed to copy af update table from user-space\n", __FUNCTION__, __LINE__);
		kfree(mode_tbl);
		return -EFAULT;
	}

	// Mode setting
	for (idx = 0; idx < mode_setting.size; idx++) {
		ret = client->i2c_func_tbl->i2c_write(
			client, mode_tbl[idx].reg_addr, mode_tbl[idx].reg_data, MSM_CAMERA_I2C_WORD_DATA);
		if (ret < 0) {
			pr_err("[syscamera][%s::%d] af update failed\n", __FUNCTION__, __LINE__);
			ret = 0;
		}
		CDBG("[syscamera][%s::%d][idx::%d][reg_addr::0x%4x][reg_data::0x%4x]\n", __FUNCTION__, __LINE__, idx,
		     mode_tbl[idx].reg_addr, mode_tbl[idx].reg_data);
	}
#if 0
	// Delay for 2 ms
	msleep(2);

	// read data
	for (idx = 0; idx < mode_setting.size; idx++) {
		uint16_t readval;
		ret = client->i2c_func_tbl->i2c_read(
			client, mode_tbl[idx].reg_addr, &readval, MSM_CAMERA_I2C_WORD_DATA);
		if (ret < 0) {
			pr_err("[syscamera][%s::%d] i2c read failed\n", __FUNCTION__, __LINE__);
			ret = 0;
		}
		pr_err("[syscamera][%s::%d][idx::%d][reg_addr::0x%4x][read_data::0x%4x]\n", __FUNCTION__, __LINE__, idx,
		       mode_tbl[idx].reg_addr, readval);
	}
#endif
	// Releasing memory for the mode table
	kfree(mode_tbl);

	return ret;
}

static int msm_companion_get_dt_data(struct device_node *of_node,
				     struct companion_device *device)
{
	int32_t rc = 0;
	uint32_t id_info[3];

	rc = of_property_read_u32(of_node, "qcom,cci-master",
				  &device->cci_i2c_master);
	pr_err("[syscamera][%s::%d][qcom,cci-master::%d][rc::%d]\n", __FUNCTION__, __LINE__, device->cci_i2c_master, rc);
	if (rc < 0) {
		/* Set default master 0 */
		device->cci_i2c_master = MASTER_0;
		rc = 0;
	}

	device->slave_info = kzalloc(sizeof(struct msm_camera_slave_info),
				     GFP_KERNEL);
	if (!device->slave_info) {
		pr_err("[syscamera][%s::%d] Memory allocation fail\n", __FUNCTION__, __LINE__);
		rc = -ENOMEM;
		return rc;;
	}

	rc = of_property_read_u32_array(of_node, "qcom,slave-id",
					id_info, 3);
	if (rc < 0) {
		pr_err("[syscamera][%s::%d] Failed to of_property_read_u32_array\n", __FUNCTION__, __LINE__);
		goto FREE_COMPANION_INFO;
	}

	device->slave_info->sensor_slave_addr = id_info[0];
	device->slave_info->sensor_id_reg_addr = id_info[1];
	device->slave_info->sensor_id = id_info[2];
	pr_info("[syscamera][%s::%d]slave addr = %x, sensor id = %x\n", __FUNCTION__, __LINE__, id_info[0], id_info[2]);
	return rc;
 FREE_COMPANION_INFO:
	kfree(device->slave_info);
	return rc;
}

#if 0
static void msm_companion_poll(uint32_t val)
{
	uint32_t tmp = 0, retry = 0;

	do {
		usleep_range(1000, 2000);
		/*Add read register here for status*/
	} while ((tmp != val) && (retry++ < MSM_COMP_POLL_RETRIES));
	if (retry < MSM_COMP_POLL_RETRIES)
		CDBG("Poll finished\n");
	else
		pr_err("[syscamera][%s::%d] Poll failed: expect: 0x%x\n", __FUNCTION__, __LINE__, val);
}
#endif

static int msm_companion_read_stats2(struct companion_device *companion_dev, unsigned char * buffer)
{
	struct msm_camera_i2c_client *client = &companion_dev->companion_i2c_client;
	int rc = 0;

	CDBG("[syscamera][%s::%d] Enter\n", __FUNCTION__, __LINE__);

	rc = client->i2c_func_tbl->i2c_write(
		client, 0x642c, STAT2_READREG_ADDR_MSB, MSM_CAMERA_I2C_WORD_DATA);
	rc = client->i2c_func_tbl->i2c_write(
		client, 0x642e, STAT2_READREG_ADDR_LSB, MSM_CAMERA_I2C_WORD_DATA);
	rc = client->i2c_func_tbl->i2c_read_multi(
		client, stats2_len, buffer);

	CDBG("[syscamera][%s::%d] Exit\n", __FUNCTION__, __LINE__);
	return rc;
}

#ifdef STATS2_WORKQUEUE
static void msm_companion_stat2_read(struct work_struct *work)
{
#define STAT2_DATA_SIZE 432

	struct companion_device *companion_dev = NULL;
	struct companion_isr_resource *isr_resource = NULL;
	struct msm_camera_i2c_client *client = NULL;
	int i;
	uint8_t *buffer = NULL;
	uint32_t len = STAT2_DATA_SIZE;
	int32_t rc = 0;
	static int s_cnt = 0;

	CDBG("[syscamera][%s::%d][E]\n", __FUNCTION__, __LINE__);

	companion_dev = container_of(work, struct companion_device, companion_read_work);
	CDBG("[syscamera][%s::%d]companion_dev=0x%p\n", __FUNCTION__, __LINE__, companion_dev);

	isr_resource = &companion_dev->isr_resource;
	client = &companion_dev->companion_i2c_client;
	buffer = (uint8_t*)kmalloc(STAT2_DATA_READ_SIZE, GFP_KERNEL);
	if (!buffer) {
		pr_err("[syscamera][%s::%d]memory allocation fail\n", __FUNCTION__, __LINE__);
		return;
	}

	// ToDo do word read from SPI
	if ( !(s_cnt++ % 200) ) {
		rc = client->i2c_func_tbl->i2c_write(
			client, 0x642c, STAT2_READREG_ADDR_MSB, MSM_CAMERA_I2C_WORD_DATA);
		rc = client->i2c_func_tbl->i2c_write(
			client, 0x642e, STAT2_READREG_ADDR_LSB, MSM_CAMERA_I2C_WORD_DATA);
		rc = client->i2c_func_tbl->i2c_read_burst(
			client, MAX_SPI_SIZE, len, buffer);

		for (i = 0; i < MAX_SPI_SIZE; i++) {
			pr_err("[syscamera][%s] stat2_data[%d] = 0x%x\n", __func__, i, buffer[i]);
		}
	}

	if (buffer) {
		kzfree(buffer);
	}

	CDBG("[syscamera][%s::%d][X]\n", __FUNCTION__, __LINE__);
}
#endif

static void msm_companion_do_tasklet(unsigned long data)
{
	unsigned long flags;
	struct companion_isr_queue_cmd *qcmd = NULL;
	struct companion_device *companion_dev = NULL;
	struct companion_isr_resource *isr_resource = NULL;

	companion_dev = (struct companion_device *)data;
	isr_resource = &companion_dev->isr_resource;
	while (atomic_read(&isr_resource->comp_irq_cnt)) {
		if (atomic_read(&comp_streamon_set)) {
			struct v4l2_event v4l2_evt;
			v4l2_evt.id = 0;
			v4l2_evt.type = V4L2_EVENT_COMPANION_IRQ_IN;
			v4l2_event_queue(companion_dev->msm_sd.sd.devnode, &v4l2_evt);
		}

		spin_lock_irqsave(&isr_resource->comp_tasklet_lock, flags);
		qcmd = list_first_entry(&isr_resource->comp_tasklet_q,
					struct companion_isr_queue_cmd, list);

		atomic_sub(1, &isr_resource->comp_irq_cnt);
		CDBG("[syscamera][%s:%d] cnt = %d\n", __FUNCTION__, __LINE__, atomic_read(&isr_resource->comp_irq_cnt));

		if (!qcmd) {
			atomic_set(&isr_resource->comp_irq_cnt, 0);
			spin_unlock_irqrestore(&isr_resource->comp_tasklet_lock,
					       flags);
			return;
		}
		list_del(&qcmd->list);
		spin_unlock_irqrestore(&isr_resource->comp_tasklet_lock,
				       flags);
		kfree(qcmd);
	}
}

irqreturn_t msm_companion_process_irq(int irq_num, void *data)
{
	unsigned long flags;
	struct companion_isr_queue_cmd *qcmd;
	struct companion_device *companion_dev =
		(struct companion_device *)data;
	struct companion_isr_resource *isr_resource = NULL;

	if (NULL == data)
		return IRQ_HANDLED;

	isr_resource = &companion_dev->isr_resource;
	qcmd = kzalloc(sizeof(struct companion_isr_queue_cmd),
		       GFP_ATOMIC);
	if (!qcmd) {
		pr_err("[syscamera][%s::%d]qcmd malloc failed!\n", __FUNCTION__, __LINE__);
		return IRQ_HANDLED;
	}
	/*This irq wil fire whenever the gpio toggles - rising or falling edge*/
	qcmd->compIrqStatus = 0;

	spin_lock_irqsave(&isr_resource->comp_tasklet_lock, flags);
	list_add_tail(&qcmd->list, &isr_resource->comp_tasklet_q);

	atomic_add(1, &isr_resource->comp_irq_cnt);
	CDBG("[syscamera][%s::%d] Companion IRQ, cnt = %d\n", __FUNCTION__, __LINE__, atomic_read(&isr_resource->comp_irq_cnt));
	spin_unlock_irqrestore(&isr_resource->comp_tasklet_lock, flags);
#ifdef STATS2_WORKQUEUE
	queue_work(companion_dev->companion_queue, &companion_dev->companion_read_work);
#else
	tasklet_schedule(&isr_resource->comp_tasklet);
#endif
	return IRQ_HANDLED;
}


static int msm_companion_init(struct companion_device *companion_dev, uint32_t size, uint16_t companion_dump)
{
	int rc = 0;

	pr_err("[syscamera][%s::%d][E] : (dump : %d)\n", __FUNCTION__, __LINE__, companion_dump);
	if (companion_dev->companion_state == COMP_POWER_UP) {
		pr_err("[syscamera][%s::%d]companion invalid state %d\n", __FUNCTION__, __LINE__,
		       companion_dev->companion_state);
		rc = -EINVAL;
		return rc;
	}

	stats2 = kmalloc(size, GFP_KERNEL);
	if (stats2 == NULL) {
		pr_err("[syscamera][%s::%d] stats2 memory alloc fail\n", __FUNCTION__, __LINE__);
		rc = -ENOMEM;
		return rc;
	}

	if (companion_dump == 1) {
		pr_err("[syscamera][%s::%d] companion dump enable\n", __FUNCTION__, __LINE__);
		dump_buf = (uint8_t*)kmalloc(0x8AFA, GFP_KERNEL);
		if (dump_buf == NULL) {
			pr_err("[syscamera][%s::%d] dump_buf memory alloc fail\n", __FUNCTION__, __LINE__);
			kfree(stats2);
			rc = -ENOMEM;
			return rc;
		}
	}
	stats2_len = size;

	init_completion(&companion_dev->wait_complete);
	spin_lock_init(&companion_dev->isr_resource.comp_tasklet_lock);
	INIT_LIST_HEAD(&companion_dev->isr_resource.comp_tasklet_q);
	tasklet_init(&companion_dev->isr_resource.comp_tasklet,
		     msm_companion_do_tasklet, (unsigned long)companion_dev);
	atomic_set(&comp_streamon_set, 0);
	enable_irq(companion_dev->isr_resource.comp_irq_num);
	companion_dev->companion_state = COMP_POWER_UP;
	return rc;
}

static int msm_companion_release(struct companion_device *companion_dev)
{
	pr_err("[syscamera][%s::%d][E]\n", __FUNCTION__, __LINE__);

	if (companion_dev->companion_state != COMP_POWER_UP) {
		pr_err("[syscamera][%s::%d]companion invalid state %d\n", __FUNCTION__, __LINE__,
		       companion_dev->companion_state);
		return -EINVAL;
	}

	tasklet_kill(&companion_dev->isr_resource.comp_tasklet);
	disable_irq(companion_dev->isr_resource.comp_irq_num);

	if (stats2) {
		kfree(stats2);
		stats2 = NULL;
		pr_err("[syscamera][%s::%d][stats2 free success]\n", __FUNCTION__, __LINE__);
	}

	if (dump_buf) {
		kfree(dump_buf);
		dump_buf = NULL;
		pr_err("[syscamera][%s::%d][dump_buf free success]\n", __FUNCTION__, __LINE__);
	}

	companion_dev->companion_state = COMP_POWER_DOWN;
	return 0;
}

static long msm_companion_cmd(struct companion_device *companion_dev, void *arg)
{
	int rc = 0;
	struct companion_cfg_data *cdata = (struct companion_cfg_data *)arg;
	struct msm_camera_i2c_client *client = &companion_dev->companion_i2c_client;

	if (!companion_dev || !cdata) {
		pr_err("[syscamera][%s::%d]companion_dev %p, cdata %p\n", __FUNCTION__, __LINE__,
		       companion_dev, cdata);
		return -EINVAL;
	}

	CDBG("[syscamera][%s::%d]cfgtype = %d\n", __FUNCTION__, __LINE__, cdata->cfgtype);
	switch (cdata->cfgtype) {
	case COMPANION_CMD_INIT: {
		uint32_t stats2_size = *(uint32_t*)cdata->cfg.setting;
		uint16_t companion_dump = cdata->isDump;

		pr_err("[syscamera][%s::%d] Companion init\n", __FUNCTION__, __LINE__);
		rc = msm_companion_init(companion_dev, stats2_size, companion_dump);
	}
	break;
	case COMPANION_CMD_AEC_UPDATE: {
		CDBG("[syscamera][%s::%d] Companion mode setting Array size = %d, Data type = %d\n", __FUNCTION__, __LINE__, cdata->cfg.mode_setting.size, cdata->cfg.mode_setting.data_type);
		rc = msm_companion_aec_update(companion_dev, cdata->cfg.mode_setting);
	}

	break;
	case COMPANION_CMD_AWB_UPDATE: {
		CDBG("[syscamera][%s::%d] Companion mode setting Array size = %d, Data type = %d\n", __FUNCTION__, __LINE__, cdata->cfg.mode_setting.size, cdata->cfg.mode_setting.data_type);
		rc = msm_companion_awb_update(companion_dev, cdata->cfg.mode_setting);
	}
	break;

	case COMPANION_CMD_AF_UPDATE: {
		CDBG("[syscamera][%s::%d] Companion mode setting Array size = %d, Data type = %d\n", __FUNCTION__, __LINE__, cdata->cfg.mode_setting.size, cdata->cfg.mode_setting.data_type);
		rc = msm_companion_af_update(companion_dev, cdata->cfg.mode_setting);
	}
	break;

	case COMPANION_CMD_GET_INFO: {
		uint16_t read_value = 0;
		struct msm_camera_i2c_client *client = &companion_dev->companion_i2c_client;
		pr_err("[syscamera][%s::%d][Get info on i2c::%x][sid::%d]\n", __FUNCTION__, __LINE__, (unsigned int)client, client->cci_client->sid);
		rc = client->i2c_func_tbl->i2c_read(client, 0x0000, &read_value, MSM_CAMERA_I2C_WORD_DATA);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d][PID::0x%4x] read failed\n", __FUNCTION__, __LINE__, read_value);
			return -EFAULT;
		}
		if (copy_to_user(cdata->cfg.read_id, &read_value, sizeof(read_value))) {
			pr_err("[syscamera][%s::%d] copy_to_user failed\n", __FUNCTION__, __LINE__);
			rc = -EFAULT;
			break;
		}
	}
	break;

	case COMPANION_CMD_GET_REV: {
		uint16_t read_value = 0;
		struct msm_camera_i2c_client *client = &companion_dev->companion_i2c_client;
		pr_err("[syscamera][%s::%d][Get EVT rev on i2c::%x][sid::%d]\n", __FUNCTION__, __LINE__, (unsigned int)client, client->cci_client->sid);
		rc = client->i2c_func_tbl->i2c_read(client, 0x0002, &read_value, MSM_CAMERA_I2C_WORD_DATA);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d][PID::0x%4x] read failed\n", __FUNCTION__, __LINE__, read_value);
			return -EFAULT;
		}
		if (copy_to_user(cdata->cfg.rev, &read_value, sizeof(read_value))) {
			pr_err("[syscamera][%s::%d] copy_to_user failed\n", __FUNCTION__, __LINE__);
			rc = -EFAULT;
			break;
		}
	}
	break;

	case COMPANION_CMD_I2C_READ: {
			struct msm_camera_i2c_client *client = &companion_dev->companion_i2c_client;
			uint16_t local_data = 0;
			uint16_t local_addr =cdata->isDump;

			pr_err("[syscamera][%s::%d]CC_[I2C_READ::%x][sid::%d]\n", __FUNCTION__, __LINE__, (unsigned int)client, client->cci_client->sid);

			rc = client->i2c_func_tbl->i2c_read(client, local_addr, &local_data, MSM_CAMERA_I2C_WORD_DATA);
			if (rc < 0) {
				pr_err("[syscamera][%s::%d][PID::0x%4x] read failed\n", __FUNCTION__, __LINE__, local_addr);
				return -EFAULT;
			}
			pr_err("[syscamera][%s::%d][local_addr::0x%4x][local_data::0x%4x]\n", __FUNCTION__, __LINE__, local_addr, local_data);
			if (copy_to_user(cdata->cfg.read_id, (void *)&local_data, sizeof(uint16_t))) {
				pr_err("[syscamera][%s::%d] copy_to_user failed\n", __FUNCTION__, __LINE__);
				rc = -EFAULT;
				break;
			}
		}
		break;

	case COMPANION_CMD_FW_BINARY_SET:
		pr_err("[syscamera][%s::%d] Setting fw binary\n", __FUNCTION__, __LINE__);

		rc = msm_companion_fw_binary_set(companion_dev, cdata->cfg.fw_bin);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d] error on writing cal data\n", __FUNCTION__, __LINE__);
			break;
		}
		break;

	case COMPANION_CMD_SET_CAL_TBL:
		pr_err("[syscamera][%s::%d] Setting calibration table (size = %d)\n", __FUNCTION__, __LINE__, cdata->cfg.mode_setting.size);
		rc = msm_companion_set_cal_tbl(companion_dev, cdata->cfg.mode_setting);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d] error on writing cal data\n", __FUNCTION__, __LINE__);
			break;
		}
		break;

	case COMPANION_CMD_READ_CAL_TBL:
		pr_err("[syscamera][%s::%d] Read calibration table (cal_size = %d)\n", __FUNCTION__, __LINE__, cdata->cfg.read_cal.size);
		pr_err("[syscamera][%s::%d] Read calibration table (write size = %d)\n", __FUNCTION__, __LINE__, cdata->cfg.read_cal.offset);
		pr_err("[syscamera][%s::%d] Read calibration table (buffer = %p)\n", __FUNCTION__, __LINE__, cdata->cfg.read_cal.cal_data);

		cal_data = kmalloc(sizeof(uint8_t) * cdata->cfg.read_cal.size, GFP_KERNEL);
		if (!cal_data) {
			pr_err("[syscamera][%s::%d][Error] Memory allocation fail\n", __FUNCTION__, __LINE__);
			return -ENOMEM;
		}

		rc = msm_companion_read_cal_tbl(companion_dev, cdata->cfg.read_cal.offset, cdata->cfg.read_cal.size);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d] error on reading cal data\n", __FUNCTION__, __LINE__);
			kfree(cal_data);
			break;
		}
		if (copy_to_user(cdata->cfg.read_cal.cal_data, cal_data, sizeof(uint8_t) * cdata->cfg.read_cal.size)) {
			pr_err("[syscamera][%s::%d] copy_to_user failed\n", __func__, __LINE__);
			rc = -EFAULT;
			kfree(cal_data);
			break;
		}
		kfree(cal_data);

		break;

	case COMPANION_CMD_LOAD_FIRMWARE_STEP_A:
		pr_info("[syscamera][%s::%d] COMPANION_CMD_LOAD_FIRMWARE_STEP_A\n", __FUNCTION__, __LINE__);
		rc = msm_companion_pll_init(companion_dev);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d] error on loading firmware\n", __FUNCTION__, __LINE__);
			break;
		}
		break;
	case COMPANION_CMD_LOAD_FIRMWARE_STEP_B:
		pr_info("[syscamera][%s::%d] COMPANION_CMD_LOAD_FIRMWARE_STEP_B\n", __FUNCTION__, __LINE__);
		rc = msm_companion_fw_write(companion_dev);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d] error on loading firmware\n", __FUNCTION__, __LINE__);
			break;
		}
		break;
	case COMPANION_CMD_LOAD_FIRMWARE_STEP_C:
		pr_info("[syscamera][%s::%d] COMPANION_CMD_LOAD_FIRMWARE_STEP_C\n", __FUNCTION__, __LINE__);
		rc = msm_companion_release_arm_reset(companion_dev);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d] error on loading firmware\n", __FUNCTION__, __LINE__);
			break;
		}
		break;

	case COMPANION_CMD_LOAD_MASTER:
		pr_err("[syscamera][%s::%d] Loading master\n", __FUNCTION__, __LINE__);
		rc = msm_companion_master_write(companion_dev);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d] error on loading master\n", __FUNCTION__, __LINE__);
			break;
		}
		break;

	case COMPANION_CMD_CAL_DATA_WRITE:
		pr_err("[syscamera][%s::%d] compare CRC calculated inside\n", __FUNCTION__, __LINE__);
		rc = msm_companion_compare_FW_crc(companion_dev);
		if(rc < 0) {
			pr_err("[syscamera][%s::%d] error on compare crc data\n", __FUNCTION__, __LINE__);
			break;
		}
		pr_err("[syscamera][%s::%d] Writing cal data\n", __FUNCTION__, __LINE__);
		rc = msm_companion_cal_data_write(companion_dev);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d] error on writing cal data\n", __FUNCTION__, __LINE__);
			break;
		}
		break;

	case COMPANION_CMD_GET_CRC:
		//pr_err("[syscamera][%s::%d] CRC check routine [address::0x%08x]\n", __FUNCTION__, __LINE__, cdata->cfg.crc_check.addr);
		rc = msm_companion_get_crc(companion_dev, cdata->cfg.crc_check, 0);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d] msm_companion_get_crc failed\n", __FUNCTION__, __LINE__);
			return -EFAULT;
		}
		break;

	case COMPANION_CMD_STREAM_ON:
		pr_err("[syscamera][%s::%d] Companion stream on[enable::%d]\n", __FUNCTION__, __LINE__, cdata->cfg.stream_on);
		if (cdata->cfg.stream_on == 1)
			atomic_set(&comp_streamon_set, 1);
		else
			atomic_set(&comp_streamon_set, 0);
		rc = msm_companion_stream_on(client, cdata->cfg.stream_on);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d] msm_companion_stream_on failed\n", __FUNCTION__, __LINE__);
			return -EFAULT;
		}
//		usleep(10000);
		break;

	case COMPANION_CMD_DUMP_REGISTER:
		rc = msm_companion_dump_register(companion_dev, dump_buf);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d] msm_companion_stream_on failed\n", __FUNCTION__, __LINE__);
			return -EFAULT;
		}
		if (copy_to_user(cdata->cfg.dump_buf, dump_buf, 0x8AFA)) {
			pr_err("[syscamera][%s::%d] copy_to_user failed\n", __FUNCTION__, __LINE__);
			rc = -EFAULT;
			break;
		}
		break;

	case COMPANION_CMD_SET_MODE:
		pr_err("[syscamera][%s::%d] Companion mode setting Array size = %d, Data type = %d\n", __FUNCTION__, __LINE__, cdata->cfg.mode_setting.size, cdata->cfg.mode_setting.data_type);
		rc = msm_companion_set_mode(companion_dev, cdata->cfg.mode_setting);
//		usleep(10000);
		break;

	case COMPANION_CMD_GET_STATS2:
		if (NULL == stats2 || NULL == cdata->cfg.stats2) {
			pr_err("[syscamera][%s::%d] source or destination is null[stats2::%p][cfg.stats2::%p]\n", __FUNCTION__, __LINE__, stats2, cdata->cfg.stats2);
			return -EFAULT;
		}
		rc = msm_companion_read_stats2(companion_dev, stats2);
		if (copy_to_user(cdata->cfg.stats2, stats2, stats2_len)) {
			pr_err("[syscamera][%s::%d] copy_to_user failed\n", __FUNCTION__, __LINE__);
			rc = -EFAULT;
			break;
		}
		break;

	case COMPANION_CMD_RELEASE:
		pr_err("[syscamera][%s::%d]COMPANION_CMD_RELEASE\n", __FUNCTION__, __LINE__);
		rc = msm_companion_release(companion_dev);
		break;

	default:
		pr_err("[syscamera][%s::%d]failed\n", __FUNCTION__, __LINE__);
		rc = -ENOIOCTLCMD;
		break;
	}
	return rc;
}

static int32_t msm_companion_get_subdev_id(struct companion_device *companion_dev, void *arg)
{
	uint32_t *subdev_id = (uint32_t*)arg;

	if (!subdev_id) {
		pr_err("[syscamera][%s::%d]failed\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	*subdev_id = companion_dev->subdev_id;
	pr_debug("[syscamera][%s::%d]subdev_id %d\n", __FUNCTION__, __LINE__, *subdev_id);
	return 0;
}


static long msm_companion_subdev_ioctl(struct v4l2_subdev *sd,
				       unsigned int cmd, void *arg)
{
	int rc = -ENOIOCTLCMD;
	struct companion_device *companion_dev = v4l2_get_subdevdata(sd);

	mutex_lock(&companion_dev->comp_mutex);
	CDBG("[syscamera][%s::%d]id %d\n", __FUNCTION__, __LINE__, companion_dev->pdev->id);
	switch (cmd) {
	case VIDIOC_MSM_SENSOR_GET_SUBDEV_ID:
		rc = msm_companion_get_subdev_id(companion_dev, arg);
		break;
	case VIDIOC_MSM_COMPANION_IO_CFG:
		rc = msm_companion_cmd(companion_dev, arg);
		break;
	case MSM_SD_SHUTDOWN:
		pr_err("[syscamera][%s::%d]MSM_SD_SHUTDOWN\n", __FUNCTION__, __LINE__);
		rc = msm_companion_release(companion_dev);
		break;
	default:
		pr_err("[syscamera][%s::%d]command not found\n", __FUNCTION__, __LINE__);
	}
	CDBG("[syscamera][%s::%d]\n", __FUNCTION__, __LINE__);
	mutex_unlock(&companion_dev->comp_mutex);
	return rc;
}

int msm_companion_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub)
{
	pr_err("[syscamera][%s::%d][E]-[sd::%s][navailable::%d]\n", __FUNCTION__, __LINE__, sd->name, fh->navailable);
	return v4l2_event_subscribe(fh, sub, 30);
}

int msm_companion_unsubscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				    struct v4l2_event_subscription *sub)
{
	pr_err("[syscamera][%s::%d][E]-[sd::%s][navailable::%d]\n", __FUNCTION__, __LINE__, sd->name, fh->navailable);
	return v4l2_event_unsubscribe(fh, sub);
}

static const struct v4l2_subdev_internal_ops msm_companion_internal_ops;

static struct v4l2_subdev_core_ops msm_companion_subdev_core_ops = {
	.ioctl			= msm_companion_subdev_ioctl,
	.subscribe_event	= msm_companion_subscribe_event,
	.unsubscribe_event	= msm_companion_unsubscribe_event,
};

static const struct v4l2_subdev_ops msm_companion_subdev_ops = {
	.core	= &msm_companion_subdev_core_ops,
};

static struct msm_camera_i2c_fn_t msm_companion_cci_func_tbl = {
	.i2c_read			= msm_camera_cci_i2c_read,
	.i2c_read_seq			= msm_camera_cci_i2c_read_seq,
	.i2c_write			= msm_camera_cci_i2c_write,
	.i2c_write_table		= msm_camera_cci_i2c_write_table,
	.i2c_write_seq_table		= msm_camera_cci_i2c_write_seq_table,
	.i2c_write_table_w_microdelay	=
		msm_camera_cci_i2c_write_table_w_microdelay,
	.i2c_util			= msm_sensor_cci_i2c_util,
	.i2c_write_conf_tbl		= msm_camera_cci_i2c_write_conf_tbl,
};

static struct msm_camera_i2c_fn_t msm_companion_spi_func_tbl = {
	.i2c_read		= msm_camera_spi_read,
	.i2c_read_seq		= msm_camera_spi_read_seq,
	.i2c_read_multi		= msm_camera_spi_read_multi,
	.i2c_write_seq		= msm_camera_spi_write_seq,
	.i2c_write		= msm_camera_spi_write,
	.i2c_write_table	= msm_camera_spi_write_table,
	.i2c_write_burst	= msm_camera_spi_write_burst,
};

#define msm_companion_spi_parse_cmd(spic, str, name, out, size)         \
	{                                                               \
		if (of_property_read_u32_array(                         \
			    spic->spi_master->dev.of_node,                  \
			    str, out, size)) {                              \
			return -EFAULT;                                 \
		} else {                                                \
			spic->cmd_tbl.name.opcode = out[0];             \
			spic->cmd_tbl.name.addr_len = out[1];           \
			spic->cmd_tbl.name.dummy_len = out[2];          \
			spic->cmd_tbl.name.delay_intv = out[3];         \
			spic->cmd_tbl.name.delay_count = out[4];                \
		}                                                       \
	}

static int msm_companion_spi_parse_of(struct msm_camera_spi_client *spic)
{
	int rc = -EFAULT;
	uint32_t tmp[5];
	struct device_node *of = spic->spi_master->dev.of_node;

	memset(&spic->cmd_tbl, 0x00, sizeof(struct msm_camera_spi_inst_tbl));
	msm_companion_spi_parse_cmd(spic, "qcom,spiop-read", read, tmp, 5);
	msm_companion_spi_parse_cmd(spic, "qcom,spiop-readseq", read_seq, tmp, 5);
	msm_companion_spi_parse_cmd(spic, "qcom,spiop-queryid", query_id, tmp, 5);
	msm_companion_spi_parse_cmd(spic, "qcom,spiop-pprog", page_program, tmp, 5);
	msm_companion_spi_parse_cmd(spic, "qcom,spiop-readst", read_status, tmp, 5);
	msm_companion_spi_parse_cmd(spic, "qcom,spiop-erase", erase, tmp, 5);

	rc = of_property_read_u32(of, "qcom,spi-busy-mask", tmp);
	if (rc < 0) {
		pr_err("[syscamera][%s::%d]Failed to get busy mask\n", __FUNCTION__, __LINE__);
		return rc;
	}
	spic->busy_mask = tmp[0];
	rc = of_property_read_u32(of, "qcom,spi-page-size", tmp);
	if (rc < 0) {
		pr_err("[syscamera][%s::%d]Failed to get page size\n", __FUNCTION__, __LINE__);
		return rc;
	}
	spic->page_size = tmp[0];
	rc = of_property_read_u32(of, "qcom,spi-erase-size", tmp);
	if (rc < 0) {
		pr_err("[syscamera][%s::%d]Failed to get erase size\n", __FUNCTION__, __LINE__);
		return rc;
	}
	spic->erase_size = tmp[0];
	return 0;
}

static int msm_companion_spi_setup(struct spi_device *spi)
{
	struct msm_camera_i2c_client *client = NULL;
	struct msm_camera_spi_client *spi_client = NULL;
	struct companion_device *companion_dev = NULL;
	struct companion_isr_resource *isr_resource = NULL;
	int32_t rc = 0;
	uint8_t tmp[2];

	companion_dev = kzalloc(sizeof(struct companion_device), GFP_KERNEL);
	if (!companion_dev) {
		pr_err("[syscamera][%s::%d]no enough memory\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	client = &companion_dev->companion_i2c_client;
	spi_client = kzalloc(sizeof(struct msm_camera_spi_client), GFP_KERNEL);
	if (!spi_client) {
		pr_err("[syscamera][%s::%d]kzalloc failed\n", __FUNCTION__, __LINE__);
		kfree(companion_dev);
		return -ENOMEM;
	}
	rc = of_property_read_u32(spi->dev.of_node, "cell-index",
				  &companion_dev->subdev_id);
	CDBG("[syscamera][%s::%d] cell-index %d, rc %d\n", __FUNCTION__, __LINE__, companion_dev->subdev_id, rc);
	if (rc) {
		pr_err("[syscamera][%s::%d]failed rc %d\n", __FUNCTION__, __LINE__, rc);
		goto device_free;
	}
	isr_resource = &companion_dev->isr_resource;
	rc = of_property_read_u32(spi->dev.of_node, "qcom,gpio-irq",
				  &isr_resource->comp_gpio_irq_pin);
	pr_err("[syscamera][%s::%d]gpio-irq %d, rc %d\n", __FUNCTION__, __LINE__,
	       isr_resource->comp_gpio_irq_pin, rc);
	if (rc) {
		pr_err("[syscamera][%s::%d]failed rc %d\n", __FUNCTION__, __LINE__, rc);
		goto device_free;
	}

	rc = of_property_read_u32(spi->dev.of_node,
				  "qcom,companion-id", &companion_dev->companion_device_id);
	CDBG("[syscamera][%s::%d] companion-device-id %d, rc %d\n", __FUNCTION__, __LINE__,
	     companion_dev->companion_device_id, rc);
	if (rc) {
		pr_err("[syscamera][%s::%d]Failed to get companion id\n", __FUNCTION__, __LINE__);
		goto device_free;
	}

	client->spi_client = spi_client;
	spi_client->spi_master = spi;
	client->i2c_func_tbl = &msm_companion_spi_func_tbl;
	client->addr_type = MSM_CAMERA_I2C_WORD_ADDR;

	/* set spi instruction info */
	spi_client->retry_delay = 1;
	spi_client->retries = 0;
	if (msm_companion_spi_parse_of(spi_client)) {
		dev_err(&spi->dev,
			"%s: Error parsing device properties\n", __func__);
		goto device_free;
	}

	rc = msm_camera_spi_query_id(client, 0, &tmp[0], 2);
	if (!rc) {
		spi_client->mfr_id0 = tmp[0];
		spi_client->device_id0 = tmp[1];
	}

	/*query spi device may be added here*/
	rc = gpio_request(isr_resource->comp_gpio_irq_pin, "comp-gpio-irq");
	if (rc) {
		pr_err("[syscamera][%s::%d]err gpio request\n", __FUNCTION__, __LINE__);
		goto device_free;
	}

	isr_resource->comp_irq_num =
		gpio_to_irq(isr_resource->comp_gpio_irq_pin);
	if (isr_resource->comp_irq_num < 0) {
		pr_err("[syscamera][%s::%d]irq request failed\n", __FUNCTION__, __LINE__);
		goto free_gpio;
	}

	rc = request_irq(isr_resource->comp_irq_num, msm_companion_process_irq,
			 IRQF_TRIGGER_FALLING, "companion", companion_dev);
	if (rc < 0) {
		pr_err("[syscamera][%s::%d]irq request failed\n", __FUNCTION__, __LINE__);
		goto device_free;
	}
	disable_irq(isr_resource->comp_irq_num);
#ifdef STATS2_WORKQUEUE
	/* Initialize workqueue */
	companion_dev->companion_queue = alloc_workqueue("companion_queue", WQ_HIGHPRI | WQ_CPU_INTENSIVE, 0);
	if (!companion_dev->companion_queue) {
		pr_err("[syscamera][%s::%d]could not create companion_queue for companion dev id %d\n",
		       __FUNCTION__, __LINE__, companion_dev->companion_device_id);
		goto device_free;
	}

	INIT_WORK(&companion_dev->companion_read_work, msm_companion_stat2_read);
#endif
	mutex_init(&companion_dev->comp_mutex);
	/* initialize subdev */
	v4l2_spi_subdev_init(&companion_dev->msm_sd.sd,
			     companion_dev->companion_i2c_client.spi_client->spi_master,
			     &msm_companion_subdev_ops);
	v4l2_set_subdevdata(&companion_dev->msm_sd.sd, companion_dev);
	companion_dev->msm_sd.sd.internal_ops = &msm_companion_internal_ops;
	companion_dev->msm_sd.sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	companion_dev->msm_sd.sd.flags |= V4L2_SUBDEV_FL_HAS_EVENTS;
	media_entity_init(&companion_dev->msm_sd.sd.entity, 0, NULL, 0);
	companion_dev->msm_sd.sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	companion_dev->msm_sd.sd.entity.group_id = MSM_CAMERA_SUBDEV_COMPANION;
	companion_dev->msm_sd.close_seq = MSM_SD_CLOSE_2ND_CATEGORY | 0x4;
	msm_sd_register(&companion_dev->msm_sd);

	// Initialize resources
	companion_dev->companion_cal_tbl = NULL;
	companion_dev->companion_cal_tbl_size = 0;
	companion_dev->eeprom_fw_bin = NULL;
	companion_dev->eeprom_fw_bin_size = 0;

	pr_err("[syscamera][%s::%d]spi probe success =%d X\n", __FUNCTION__, __LINE__, rc);
	return 0;
 free_gpio:
	gpio_free(isr_resource->comp_gpio_irq_pin);
 device_free:
	kfree(companion_dev);
	kfree(spi_client);
	return rc;
}

static int msm_companion_spi_probe(struct spi_device *spi)
{
	int irq, cs, cpha, cpol, cs_high;

	pr_err("[syscamera][%s::%d][E]\n", __FUNCTION__, __LINE__);
	if (poweroff_charging == 1) {
		int comp_gpio_en;
		comp_gpio_en = of_get_named_gpio(spi->dev.of_node, "qcom,gpio-comp-en", 0);
		gpio_request(comp_gpio_en, "gpio-comp-en");
		gpio_direction_output(comp_gpio_en, 0);
		gpio_free(comp_gpio_en);
		pr_err("forced return companion_spi_probe at lpm mode\n");
		return 0;
	}

	spi->bits_per_word = 8;
	spi->mode = SPI_MODE_0;
	spi_setup(spi);

	irq = spi->irq;
	cs = spi->chip_select;
	cpha = (spi->mode & SPI_CPHA) ? 1 : 0;
	cpol = (spi->mode & SPI_CPOL) ? 1 : 0;
	cs_high = (spi->mode & SPI_CS_HIGH) ? 1 : 0;
	dev_info(&spi->dev, "irq[%d] cs[%x] CPHA[%x] CPOL[%x] CS_HIGH[%x]\n",
		 irq, cs, cpha, cpol, cs_high);
	dev_info(&spi->dev, "max_speed[%u]\n", spi->max_speed_hz);

	return msm_companion_spi_setup(spi);
}

static int msm_companion_spi_remove(struct spi_device *sdev)
{
	struct v4l2_subdev *sd = spi_get_drvdata(sdev);
	struct companion_device *companion_dev = NULL;

	if (!sd) {
		pr_err("[syscamera][%s::%d]Subdevice is NULL\n", __FUNCTION__, __LINE__);
		return 0;
	}

	companion_dev = (struct companion_device *)v4l2_get_subdevdata(sd);
	if (!companion_dev) {
		pr_err("[syscamera][%s::%d]companion device is NULL\n", __FUNCTION__, __LINE__);
		return 0;
	}

#ifdef STATS2_WORKQUEUE
	if (companion_dev->companion_queue) {
		destroy_workqueue(companion_dev->companion_queue);
		companion_dev->companion_queue = NULL;
	}
#endif

	kfree(companion_dev->companion_i2c_client.spi_client);
	kfree(companion_dev);
	return 0;
}
static int msm_companion_probe(struct platform_device *pdev)
{
	struct companion_device *companion_dev = NULL;
	struct msm_camera_cci_client *cci_client = NULL;
	struct msm_camera_i2c_client *client = NULL;
	int32_t rc = 0;

	pr_err("[syscamera][%s::%d][E]\n", __FUNCTION__, __LINE__);

	if (poweroff_charging == 1) {
		pr_err("forced return companion_probe at lpm mode\n");
		return rc;
	}

	if (pdev->dev.of_node) {
		rc = of_property_read_u32((&pdev->dev)->of_node,
					  "cell-index", &pdev->id);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d]failed to read cell-index\n", __FUNCTION__, __LINE__);
			goto companion_no_resource;
		}
		CDBG("[syscamera][%s::%d] device id %d\n", __FUNCTION__, __LINE__, pdev->id);
	}

	companion_dev = kzalloc(sizeof(struct companion_device), GFP_KERNEL);
	if (!companion_dev) {
		pr_err("[syscamera][%s::%d]no enough memory\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	companion_dev->subdev_id = pdev->id;
	if (pdev->dev.of_node) {
		rc = msm_companion_get_dt_data(pdev->dev.of_node, companion_dev);
		if (rc < 0) {
			pr_err("[syscamera][%s::%d]failed to msm_companion_get_dt_data\n", __FUNCTION__, __LINE__);
			kfree(companion_dev);
			return rc;
		}
	}

	companion_dev->companion_i2c_client.cci_client = kzalloc(sizeof(
									 struct msm_camera_cci_client), GFP_KERNEL);
	if (!companion_dev->companion_i2c_client.cci_client) {
		pr_err("[syscamera][%s::%d]memory allocation fail\n", __FUNCTION__, __LINE__);
		kfree(companion_dev);
		return rc;
	}

	client = &companion_dev->companion_i2c_client;
	client->addr_type = MSM_CAMERA_I2C_WORD_ADDR;
	cci_client = companion_dev->companion_i2c_client.cci_client;
	cci_client->cci_subdev = msm_cci_get_subdev();
	cci_client->cci_i2c_master = companion_dev->cci_i2c_master;
	cci_client->sid =
		companion_dev->slave_info->sensor_slave_addr >> 1;
	cci_client->retries = 3;
	cci_client->id_map = 0;
	if (!companion_dev->companion_i2c_client.i2c_func_tbl) {
		pr_err("[syscamera][%s::%d] i2c_func_tbl=cci \n", __FUNCTION__, __LINE__);
		companion_dev->companion_i2c_client.i2c_func_tbl =
			&msm_companion_cci_func_tbl;
	} else {
		pr_err("[syscamera][%s::%d] i2c_func_tbl=cci skipped \n", __FUNCTION__, __LINE__);
	}
	mutex_init(&companion_dev->comp_mutex);
	v4l2_subdev_init(&companion_dev->msm_sd.sd, &msm_companion_subdev_ops);
	v4l2_set_subdevdata(&companion_dev->msm_sd.sd, companion_dev);
	platform_set_drvdata(pdev, &companion_dev->msm_sd.sd);

	companion_dev->pdev = pdev;
	companion_dev->msm_sd.sd.internal_ops = &msm_companion_internal_ops;
	companion_dev->msm_sd.sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	companion_dev->msm_sd.sd.flags |= V4L2_SUBDEV_FL_HAS_EVENTS;
	snprintf(companion_dev->msm_sd.sd.name,
		 ARRAY_SIZE(companion_dev->msm_sd.sd.name), "msm_companion");
	media_entity_init(&companion_dev->msm_sd.sd.entity, 0, NULL, 0);
	companion_dev->msm_sd.sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	companion_dev->msm_sd.sd.entity.group_id = MSM_CAMERA_SUBDEV_COMPANION;
	companion_dev->msm_sd.close_seq = MSM_SD_CLOSE_2ND_CATEGORY | 0x4;
	msm_sd_register(&companion_dev->msm_sd);

	// Initialize resources
	companion_dev->companion_cal_tbl = NULL;
	companion_dev->companion_cal_tbl_size = 0;
	companion_dev->eeprom_fw_bin = NULL;
	companion_dev->eeprom_fw_bin_size = 0;
	companion_dev->companion_state = COMP_POWER_DOWN;
	pr_err("[syscamera][%s::%d] probe success\n", __FUNCTION__, __LINE__);
	return 0;

 companion_no_resource:
	pr_err("[syscamera][%s::%d] probe failed\n", __FUNCTION__, __LINE__);
	kfree(companion_dev);
	return 0;
}

static const struct of_device_id msm_companion_dt_match[] = {
	{ .compatible = "qcom,companion" },
	{}
};

MODULE_DEVICE_TABLE(of, msm_companion_dt_match);

static struct platform_driver companion_driver = {
	.probe			= msm_companion_probe,
	.driver			= {
		.name		= MSM_COMP_DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = msm_companion_dt_match,
	},
};

static struct spi_driver companion_spi_driver = {
	.driver			= {
		.name		= "qcom_companion",
		.owner		= THIS_MODULE,
		.of_match_table = msm_companion_dt_match,
	},
	.probe			= msm_companion_spi_probe,
	.remove			= msm_companion_spi_remove,
};

static int __init msm_companion_init_module(void)
{
	int32_t rc = 0;

	pr_err("[syscamera][%s::%d]init companion module\n", __FUNCTION__, __LINE__);
	rc = platform_driver_register(&companion_driver);
	rc = spi_register_driver(&companion_spi_driver);
	return rc;
}

static void __exit msm_companion_exit_module(void)
{
	platform_driver_unregister(&companion_driver);
	spi_unregister_driver(&companion_spi_driver);
}

late_initcall(msm_companion_init_module);
module_exit(msm_companion_exit_module);
MODULE_DESCRIPTION("MSM Companion driver");
MODULE_LICENSE("GPL v2");
