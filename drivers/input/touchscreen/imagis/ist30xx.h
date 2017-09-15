/*
 *  Copyright (C) 2010, Imagis Technology Co. Ltd. All Rights Reserved.
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

#ifndef __IST30XX_H__
#define __IST30XX_H__

/*
 * Support F/W ver : IST30xxB v3.3 (included tag)
 * Support IC : IST3026B, IST3032B, IST3038
 * Release : 2014.01.07 by Ian Bae
 */
#include "ist30xx_sec.h"

#define IMAGIS_IST30XX          (1)
#define IMAGIS_IST30XXB         (2)
#define IMAGIS_IST3038          (3)
#define IMAGIS_IST3044          (4)

#if defined(CONFIG_SEC_KANAS_PROJECT)
#define IMAGIS_TSP_IC           IMAGIS_IST3038
#else
#define IMAGIS_TSP_IC           IMAGIS_IST30XXB
#endif

#if ((IMAGIS_TSP_IC == IMAGIS_IST30XX) || (IMAGIS_TSP_IC == IMAGIS_IST30XXB))
#define IST30XX_EXTEND_COORD    (0)     /* IST3026, IST3032, IST3038 */
#elif ((IMAGIS_TSP_IC == IMAGIS_IST3038) || (IMAGIS_TSP_IC == IMAGIS_IST3044))
#define IST30XX_EXTEND_COORD    (1)     /* IST3038, IST3044 */
#endif

#define TSP_CHIP_VENDOR     ("IMAGIS")

#if ((IMAGIS_TSP_IC == IMAGIS_IST30XX) || (IMAGIS_TSP_IC == IMAGIS_IST30XXB))
#define TSP_CHIP_NAME       ("IST3032B")
#elif (IMAGIS_TSP_IC == IMAGIS_IST3038)
#define TSP_CHIP_NAME       ("IST3038")
#elif (IMAGIS_TSP_IC== IMAGIS_IST3044)
#define TSP_CHIP_NAME       ("IST3044")
#endif

#define I2C_BURST_MODE          (1)
#define I2C_MONOPOLY_MODE       (0)

#define IST30XX_EVENT_MODE      (1)
#if IST30XX_EVENT_MODE
# define IST30XX_NOISE_MODE     (1)
# define IST30XX_TRACKING_MODE  (1)
# define IST30XX_ALGORITHM_MODE (1)
#else
# define IST30XX_NOISE_MODE     (0)
# define IST30XX_TRACKING_MODE  (0)
# define IST30XX_ALGORITHM_MODE (0)
#endif

#define IST30XX_USE_KEY         (1)
#define IST30XX_DEBUG           (1)

#define SEC_FACTORY_MODE        (1)
#define IST30XX_CMCS_TEST       (1)

#define IST30XX_DEV_NAME        "sec_touch"
#define IST30XX_CHIP_ID         (0x30003000)
#define IST30XXA_CHIP_ID        (0x300a300a)
#define IST30XXB_CHIP_ID        (0x300b300b)
#define IST3038_CHIP_ID         (0x30383038)

#define IST30XX_DEV_ID          (0xA0 >> 1)
#define IST30XX_FW_DEV_ID       (0xA4 >> 1)

#define IST30XX_ADDR_LEN        (4)
#define IST30XX_DATA_LEN        (4)

#define IST30XX_ISP_CMD_LEN     (3)

#define IST30XX_MAX_MT_FINGERS  (10)
#define IST30XX_MAX_KEYS        (2)

#define IST30XX_MAX_X           (480)
#define IST30XX_MAX_Y           (800)
#define IST30XX_MAX_W           (15)

#define IST30XX                 (1)
#define IST30XXB                (2)

/* I2C Transfer msg number */
#define WRITE_CMD_MSG_LEN       (1)
#define READ_CMD_MSG_LEN        (2)

#define NORMAL_TEMPERATURE      (0)
#define LOW_TEMPERATURE         (1)
#define HIGH_TEMPERATURE        (2)

/* TSP Local Code */
#define TSP_LOCAL_EU               (0)
#define TSP_LOCAL_EEU             (1)
#define TSP_LOCAL_TD               (11)
#define TSP_LOCAL_CMCC           (12)
#define TSP_LOCAL_CU               (13)
#define TSP_LOCAL_SPRD            (14)
#define TSP_LOCAL_CTC               (15)
#define TSP_LOCAL_INDIA           (21)
#define TSP_LOCAL_SWASIA         (22)
#define TSP_LOCAL_NA              (31)
#define TSP_LOCAL_LA               (32)
#if defined(CONFIG_MACH_KANAS3G_CTC)
#define TSP_LOCAL_CODE           TSP_LOCAL_CTC
#elif defined(CONFIG_MACH_KANAS3G_CMCC)
#define TSP_LOCAL_CODE           TSP_LOCAL_CMCC
#elif defined(CONFIG_MACH_KANAS3G_CU)
#define TSP_LOCAL_CODE           TSP_LOCAL_CU
#else
#define TSP_LOCAL_CODE           TSP_LOCAL_EU
#endif

/* Debug message */
#define DEV_ERR     (1)
#define DEV_WARN    (2)
#define DEV_INFO    (3)
#define DEV_DEBUG   (4)
#define DEV_VERB    (5)

#define IST30XX_DEBUG_TAG       "[ TSP ]"
#define IST30XX_DEBUG_LEVEL     DEV_INFO
//#define IST30XX_DEBUG_LEVEL     DEV_VERB

#define tsp_err(fmt, ...)   tsp_printk(DEV_ERR, fmt, ## __VA_ARGS__)
#define tsp_warn(fmt, ...)  tsp_printk(DEV_WARN, fmt, ## __VA_ARGS__)
#define tsp_info(fmt, ...)  tsp_printk(DEV_INFO, fmt, ## __VA_ARGS__)
#define tsp_debug(fmt, ...) tsp_printk(DEV_DEBUG, fmt, ## __VA_ARGS__)
#define tsp_verb(fmt, ...)  tsp_printk(DEV_VERB, fmt, ## __VA_ARGS__)

#if defined(CONFIG_SEC_DVFS) || defined (CONFIG_CPU_FREQ_LIMIT_USERSPACE)
#define TOUCH_BOOSTER			1
#define TOUCH_BOOSTER_OFF_TIME	100
#define TOUCH_BOOSTER_CHG_TIME	200
#endif

enum ist30xx_commands {
	CMD_ENTER_UPDATE            = 0x02,
	CMD_EXIT_UPDATE             = 0x03,
	CMD_UPDATE_SENSOR           = 0x04,
	CMD_UPDATE_CONFIG           = 0x05,
	CMD_ENTER_REG_ACCESS        = 0x07,
	CMD_EXIT_REG_ACCESS         = 0x08,
	CMD_SET_NOISE_MODE          = 0x0A,
	CMD_START_SCAN              = 0x0B,
	CMD_ENTER_FW_UPDATE         = 0x0C,
	CMD_RUN_DEVICE              = 0x0D,
	CMD_EXEC_MEM_CODE           = 0x0E,
	CMD_SET_TEST_MODE           = 0x0F,

	CMD_CALIBRATE               = 0x11,
	CMD_USE_IDLE                = 0x12,
	CMD_USE_DEBUG               = 0x13,
	CMD_ZVALUE_MODE             = 0x15,
	CMD_SAME_POSITION           = 0x16,
	CMD_CHECK_CALIB             = 0x1A,
	CMD_SET_TEMPER_MODE         = 0x1B,
	CMD_USE_CORRECT_CP          = 0x1C,
	CMD_SET_REPORT_RATE         = 0x1D,
	CMD_SET_IDLE_TIME           = 0x1E,

	CMD_GET_COORD               = 0x20,

	CMD_GET_CHIP_ID             = 0x30,
	CMD_GET_FW_VER              = 0x31,
	CMD_GET_CHECKSUM            = 0x32,
	CMD_GET_LCD_RESOLUTION      = 0x33,
	CMD_GET_TSP_CHNUM1          = 0x34,
	CMD_GET_PARAM_VER           = 0x35,
	CMD_GET_SUB_VER             = 0x36,
	CMD_GET_CALIB_RESULT        = 0x37,
	CMD_GET_TSP_SWAP_INFO       = 0x38,
	CMD_GET_KEY_INFO1           = 0x39,
	CMD_GET_KEY_INFO2           = 0x3A,
	CMD_GET_KEY_INFO3           = 0x3B,
	CMD_GET_TSP_CHNUM2          = 0x3C,
	CMD_GET_TSP_DIRECTION       = 0x3D,

	CMD_GET_TSP_VENDOR          = 0x3E,
	CMD_GET_TSP_PANNEL_TYPE     = 0x40,

	CMD_GET_CHECKSUM_ALL        = 0x41,
	CMD_DEFAULT                 = 0xFF,
};

#define CMD_FW_UPDATE_MAGIC     (0x85FDAE8A)


typedef struct _ALGR_INFO {
	u32	scan_status;
	u8	touch_cnt;
	u8	intl_touch_cnt;
	u16	status_flag;

	u16	raw_peak_min;
	u16	raw_peak_max;
	u16	flt_peak_max;
	u16	adpt_threshold;

	u16	key_raw_data[6];
} ALGR_INFO;

#if IST30XX_EXTEND_COORD
#define EXTEND_COORD_CHECKSUM   (0)
#define IST30XX_INTR_STATUS1    (0x71000000)
#define IST30XX_INTR_STATUS2    (0x00000C00)
#define CHECK_INTR_STATUS1(n)   (((n & IST30XX_INTR_STATUS1) == IST30XX_INTR_STATUS1) ? 1 : 0)
#define CHECK_INTR_STATUS2(n)   (((n & IST30XX_INTR_STATUS2) > 0) ? 0 : 1)

#define PARSE_FINGER_CNT(n)     ((n >> 12) & 0xF)
#define PARSE_KEY_CNT(n)        ((n >> 21) & 0x7)
#define PARSE_FINGER_STATUS(n)  (n & 0x3FF)         /* Finger status: [9:0] */
#define PARSE_KEY_STATUS(n)     ((n >> 16) & 0x1F)  /* Key status: [20:16] */

#define PRESSED_FINGER(s, id)    ((s & (1 << (id - 1))) ? true : false)
#define PRESSED_KEY(s, id)       ((s & (1 << (16 + id - 1))) ? true : false)
typedef union {
	struct {
		u32	y       : 12;
		u32	x       : 12;
		u32	area    : 4;
		u32	id      : 4;
	} bit_field;
	u32 full_field;
} finger_info;
#else  // IST30XX_EXTEND_COORD
#define EXTEND_COORD_CHECKSUM   (0)
typedef union {
	struct {
		u32	y       : 10;
		u32	w       : 6;
		u32	x       : 10;
		u32	id      : 4;
		u32	udmg    : 2;
	} bit_field;
	u32 full_field;
} finger_info;
#endif  // IST30XX_EXTEND_COORD


struct ist30xx_status {
	int	power;
	int	update;
	int	calib;
	int	calib_msg;
	bool	event_mode;
	bool	noise_mode;
};

struct ist30xx_fw {
	u32	prev_core_ver;
	u32	prev_param_ver;
	u32	core_ver;
	u32	param_ver;
	u32	sub_ver;
	u32	index;
	u32	size;
	u32	chksum;
	u32	buf_size;
	u8 *	buf;
};

struct ist30xx_os {
	u32	bin_size;
	u32	curve_size;
	u8 *	bin_buf;
	u8 *	curve_buf;
	u16	open_buf[16 * 16];      // 32channel
	u16	short1_buf[16 * 16];    // 32channel
	u16	short2_buf[16 * 16];    // 32channel
};

#define IST30XX_TAG_MAGIC       "ISTV1TAG"
struct ist30xx_tags {
	char	magic1[8];
	u32	fw_addr;
	u32	fw_size;
	u32	flag_addr;
	u32	flag_size;
	u32	cfg_addr;
	u32	cfg_size;
	u32	sensor1_addr;
	u32	sensor1_size;
	u32	sensor2_addr;
	u32	sensor2_size;
	u32	sensor3_addr;
	u32	sensor3_size;
	u32	chksum;
	u32	reserved2;
	char	magic2[8];
};

struct ist30xx_dt_data {
	int irq_gpio;
	int scl_gpio;
	int sda_gpio;
	int touch_en_gpio;
};

#include <linux/earlysuspend.h>
struct ist30xx_data {
	struct i2c_client *	client;
	struct input_dev *	input_dev;
	struct ist30xx_dt_data	*dt_data;
	struct early_suspend	early_suspend;
	struct ist30xx_status	status;
	struct ist30xx_fw	fw;
	struct ist30xx_tags	tags;
#if SEC_FACTORY_MODE
	struct sec_factory	sec;
#endif
#if defined(TOUCH_BOOSTER)
	struct delayed_work work_dvfs_off;
	struct mutex dvfs_lock;
	bool	                         dvfs_lock_status;
#endif
	u32			chip_id;
	u32			tsp_type;
	u32			num_fingers;
	u32			num_keys;
	u32			max_fingers;
	u32			max_keys;
	u32			irq_enabled;
#if IST30XX_EXTEND_COORD
	u32			t_status;
#else
	finger_info		prev_fingers[IST30XX_MAX_MT_FINGERS];
	finger_info		prev_keys[IST30XX_MAX_MT_FINGERS];
#endif
	finger_info		fingers[IST30XX_MAX_MT_FINGERS];
	bool i2cPower_flag;
};


extern struct mutex ist30xx_mutex;
extern int ist30xx_dbg_level;

void tsp_printk(int level, const char *fmt, ...);
int ist30xx_intr_wait(long ms);

void ist30xx_enable_irq(struct ist30xx_data *data);
void ist30xx_disable_irq(struct ist30xx_data *data);
void ist30xx_set_ta_mode(bool charging);
void ist30xx_set_cover_mode(int mode);
void ist30xx_start(struct ist30xx_data *data);
int ist30xx_get_ver_info(struct ist30xx_data *data);
int ist30xx_init_touch_driver(struct ist30xx_data *data);

int ist30xx_get_position(struct i2c_client *client, u32 *buf, u16 len);

int ist30xx_read_cmd(struct i2c_client *client, u32 cmd, u32 *buf);
int ist30xx_write_cmd(struct i2c_client *client, u32 cmd, u32 val);

int ist30xx_cmd_run_device(struct i2c_client *client, bool is_reset);
int ist30xx_cmd_start_scan(struct i2c_client *client);
int ist30xx_cmd_calibrate(struct i2c_client *client);
int ist30xx_cmd_check_calib(struct i2c_client *client);
int ist30xx_cmd_update(struct i2c_client *client, int cmd);
int ist30xx_cmd_reg(struct i2c_client *client, int cmd);

int ist30xx_power_on(void);
int ist30xx_power_off(void);
int ist30xx_reset(void);

int ist30xx_internal_suspend(struct ist30xx_data *data);
int ist30xx_internal_resume(struct ist30xx_data *data);

int ist30xx_init_system(void);

#endif  // __IST30XX_H__
