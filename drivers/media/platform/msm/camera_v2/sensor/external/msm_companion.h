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

#ifndef MSM_COMPANION_H
#define MSM_COMPANION_H

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/irqreturn.h>
#include "msm_sd.h"
#include <mach/camera2.h>
#include <media/msm_cam_sensor.h>
#include "msm_camera_i2c.h"
#include "msm_camera_spi.h"
#include <linux/regulator/consumer.h>

#ifdef STATS2_WORKQUEUE
#include <linux/workqueue.h>
#endif

#ifndef BYPASS_COMPANION
#define MSM_CC_FW
#endif
#ifdef MSM_CC_FW
#define INFO_SWAP
//#define MSM_CC_DEBUG
#if 0
#define CC_FW_EVT0_ISP_PATH		"/system/cameradata/Isp.bin"
#define CC_FW_EVT1_ISP_PATH		"/system/cameradata/Isp1.bin"
#define CC_FW_MASTER_PATH	"/system/cameradata/Master.bin"
#define CC_FW_MODE_PATH		"/system/cameradata/Mode.bin"
#define SD_FW_EVT0_ISP_PATH		"/data/media/0/Isp.bin"
#define SD_FW_EVT1_ISP_PATH		"/data/media/0/Isp1.bin"
#define SD_FW_MASTER_PATH	"/data/media/0/Master.bin"
#define SD_FW_MODE_PATH		"/data/media/0/Mode.bin"
#else
enum {
	CC_BIN1 = 0,
	CC_BIN2,
	CC_BIN3,
	CC_BIN4,
	CC_BIN5,
	CC_BIN6,
	CC_BIN_MAX,
};

enum {
	FW_PATH_CC = 0,
	FW_PATH_SD,
	FW_PATH_MAX,
};
char *companion_fw_path[] = {
	"/system/cameradata/",
	"/data/media/0/",
};

enum {
	FW_NAME_ISP = 0,
	FW_NAME_MASTER,
	FW_NAME_MAX,
};
char *companion_fw_name[] = {
	"_IspX_",
	"MasterX",
//	"_Mode_",
};

#define FW_DEFAULT_SENSOR_VER	"F16QL"
#define FW_DEFAULT_SENSOR_NAME	"s5k2p2xx"
#define FW_EXTENSION			".bin"

#define REV_OFFSET_ISP_CC			28
#define REV_OFFSET_ISP_SD			23

#define REV_OFFSET_MASTER_CC			25
#define REV_OFFSET_MASTER_SD			20

#endif

#endif

#define STAT2_READREG_ADDR_MSB		0x2002
#define STAT2_READREG_ADDR_LSB		0x6000

enum companion_state_t {
	COMP_POWER_DOWN,
	COMP_POWER_UP,
};

enum companion_version_t {
	COMP_EVT0,
	COMP_EVT1,
	COMP_EVT_MAX,
} companion_version_info;

struct companion_isr_queue_cmd {
	struct list_head list;
	uint32_t compIrqStatus;
};

struct companion_isr_resource {
	atomic_t comp_irq_cnt;
	spinlock_t comp_tasklet_lock;
	struct list_head comp_tasklet_q;
	struct tasklet_struct comp_tasklet;
	uint32_t comp_gpio_irq_pin;
	int32_t comp_irq_num;
};

struct companion_device {
	struct platform_device *pdev;
	struct msm_sd_subdev msm_sd;
	struct msm_camera_i2c_client companion_i2c_client;
	enum cci_i2c_master_t cci_i2c_master;
	struct msm_camera_slave_info *slave_info;
	struct mutex comp_mutex;
	struct regulator *companion_power;
	struct completion wait_complete;
	uint32_t hw_version;
	uint32_t subdev_id;
	uint32_t companion_device_id;
	enum companion_state_t companion_state;
	struct companion_isr_resource isr_resource;
	struct msm_camera_i2c_reg_array * companion_cal_tbl;
	uint32_t companion_cal_tbl_size;
	uint8_t eeprom_fw_ver[12];
	uint8_t *eeprom_fw_bin;
	uint32_t eeprom_fw_bin_size;
	uint32_t loading_fw_bin_size;
	uint32_t crc_by_ap;
#ifdef STATS2_WORKQUEUE
	struct workqueue_struct *companion_queue;
	struct work_struct companion_read_work;
#endif
};
// unsigned char stats2[COMPANION_STATS2_LENGTH];
uint8_t *stats2;
uint32_t stats2_len;
uint8_t *dump_buf;

// unsigned char for cal data
uint8_t *cal_data;
#endif

