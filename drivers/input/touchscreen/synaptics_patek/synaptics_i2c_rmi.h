/* Synaptics Register Mapped Interface (RMI4) I2C Physical Layer Driver.
 * Copyright (c) 2007-2012, Synaptics Incorporated
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _SYNAPTICS_RMI4_H_
#define _SYNAPTICS_RMI4_H_

#define SYNAPTICS_RMI4_DRIVER_VERSION "DS5 1.0"
#define DRIVER_NAME "synaptics_rmi4_i2c"
#define SYNAPTICS_DEVICE_NAME	"SM-W2015"

#include <linux/device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/firmware.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/extcon.h>

#define PROXIMITY
#define TYPE_B_PROTOCOL
#define TSP_BOOSTER
#define CONFIG_GLOVE_TOUCH
#define	USE_OPEN_CLOSE
#define TSP_PATTERN_TRACKING_METHOD
#define PATTERN_TRACKING_FOR_FULLSCREEN
#define REPORT_2D_W
#define CHARGER_NOTIFIER

#ifdef CHARGER_NOTIFIER
#define GPIO_TSP_TA	63
#endif

enum tsp_target{
	TSP_MAIN = 0,
	TSP_SUB,
};

/* DVFS feature : TOUCH BOOSTER */
#define DVFS_STAGE_TRIPLE			3
#ifdef TSP_BOOSTER
#define DVFS_STAGE_DUAL				2
#define DVFS_STAGE_SINGLE			1
#define DVFS_STAGE_NONE				0
#include <linux/cpufreq.h>
#define TOUCH_BOOSTER_OFF_TIME			300
#define TOUCH_BOOSTER_CHG_TIME			200
#endif

#ifdef USE_OPEN_CLOSE
/*#define USE_OPEN_DWORK*/
#endif

#ifdef USE_OPEN_DWORK
#define TOUCH_OPEN_DWORK_TIME 			10
#endif

/* TSP_PATTERN_TRACKING : This is protection code for ghost touch
 * such as stucked single postion and frequently pressed and released
 * touch into specific area.
 * Almost these kind of ghost touch can be treated into firmware level.
 * But some hardware or some regional project might be needed this feature.
 * Basically this define is not recommeded.
 */
/* To recovery the device from ghost touch  */
#ifdef TSP_PATTERN_TRACKING_METHOD
#define TSP_PT_MAX_GHOSTCHECK_FINGER		10
#define MAX_GHOSTCHECK_FINGER 			10
#define MAX_GHOSTTOUCH_COUNT			5
#define MAX_COUNT_TOUCHSYSREBOOT		4
#define MAX_GHOSTTOUCH_BY_PATTERNTRACKING	3
#define PATTERN_TRACKING_DISTANCE 		4
#define TSP_REBOOT_PENDING_TIME 		50
#define MOVE_COUNT_TH				100

#define MIN_X_EDGE				17
#define MAX_X_EDGE				1060   //1062
#define MIN_Y_EDGE				17
#define MAX_Y_EDGE				1900   // 1902
#endif

#define F12_FINGERS_TO_SUPPORT 			10
#define INVALID_X 65535
#define INVALID_Y 65535

#define RPT_TYPE 	(1 << 0)
#define RPT_X_LSB 	(1 << 1)
#define RPT_X_MSB 	(1 << 2)
#define RPT_Y_LSB 	(1 << 3)
#define RPT_Y_MSB 	(1 << 4)
#define RPT_Z 		(1 << 5)
#define RPT_WX 		(1 << 6)
#define RPT_WY 		(1 << 7)
#define RPT_DEFAULT 	(RPT_TYPE | RPT_X_LSB | RPT_X_MSB |\
			RPT_Y_LSB | RPT_Y_MSB)

#ifdef CONFIG_GLOVE_TOUCH
#define GLOVE_FEATURE_EN		0x20
#define GLOVE_CLEAR_DEFAULT		0x00
#endif

#ifdef PROXIMITY
#define USE_CUSTOM_REZERO

#define HOVER_Z_MAX			(255)
#define FINGER_HOVER_EN			(1 << 0)
#define FINGER_HOVER_DIS		(0 << 0)
#define AIR_SWIPE_EN			(1 << 1)
#define LARGE_OBJ_EN			(1 << 2)
#define HOVER_PINCH_EN			(1 << 3)
#define NO_PROXIMITY_ON_TOUCH_EN	(1 << 5)
#define CONTINUOUS_LOAD_REPORT_EN	(1 << 6)
#define SLEEP_PROXIMITY			(1 << 7)

#define PROXIMITY_DEFAULT	(NO_PROXIMITY_ON_TOUCH_EN)
#define PROXIMITY_ENABLE	(PROXIMITY_DEFAULT | FINGER_HOVER_EN)

#define HAS_FINGER_HOVER		(1 << 0)
#define HAS_AIR_SWIPE			(1 << 1)
#define HAS_LARGE_OBJ			(1 << 2)
#define HAS_HOVER_PINCH			(1 << 3)
#define HAS_EDGE_SWIPE			(1 << 4)
#define HAS_SINGLE_FINGER		(1 << 5) //removable
#define F51_VERSION			0x41
#define F51_PROXIMITY_ENABLES_OFFSET	0
#define F51_CTRL54_OFFSET		99
#ifdef USE_CUSTOM_REZERO
#define F51_GENERAL_CONTROL_OFFSET	1
#define F51_CTRL78_OFFSET		115
#endif

#define F51_FINGER_TIMEOUT		50 /* ms */
#endif

#define POLLING_PERIOD			1 /* ms */
#define SYN_I2C_RETRY_TIMES		3

#define CHECK_STATUS_TIMEOUT_MS		200
#define STATUS_NO_ERROR			0x00
#define STATUS_RESET_OCCURRED		0x01
#define STATUS_INVALID_CONFIG		0x02
#define STATUS_DEVICE_FAILURE		0x03
#define STATUS_CONFIG_CRC_FAILURE	0x04
#define STATUS_FIRMWARE_CRC_FAILURE	0x05
#define STATUS_CRC_IN_PROGRESS		0x06

#define F01_STD_QUERY_LEN		21
#define F01_BUID_ID_OFFSET		18

#define NORMAL_OPERATION		(0 << 0)
#define SENSOR_SLEEP			(1 << 0)
#define NO_SLEEP_OFF			(0 << 2)
#define NO_SLEEP_ON			(1 << 2)
#define CHARGER_CONNECTED		(1 << 5)
#define CHARGER_DISCONNECTED		0xDF

#define CONFIGURED			(1 << 7)

#define TSP_NEEDTO_REBOOT		(-ECONNREFUSED)
#define MAX_TSP_REBOOT			3

#define SYNAPTICS_MAX_X_SIZE		1079
#define SYNAPTICS_MAX_Y_SIZE		1919
#define SYNAPTICS_MAX_WIDTH		SYNAPTICS_MAX_Y_SIZE

#if defined(CONFIG_MACH_JACTIVESKT)
#define NUM_RX	16
#define NUM_TX	28
#else
#define NUM_RX	28
#define NUM_TX	16
#endif


#define SYNAPTICS_HW_RESET_TIME		80
#define SYNAPTICS_REZERO_TIME		100
#define SYNAPTICS_POWER_MARGIN_TIME	150

#define SYNAPTICS_PRODUCT_ID_B0		"SY 01"
#define SYNAPTICS_PRODUCT_ID_B0_SPAIR	"S5000B"

#define FPGA_FW_PATH			"ice40xx/fpga_sdio_patek.fw"

/* after B'd rev 06 */
/* User firmware */
#define FW_IMAGE_NAME_PATEK		"tsp_synaptics/synaptics_patek.fw"
/* Factory firmware */
#define FAC_FWIMAGE_NAME_PATEK		"tsp_synaptics/synaptics_patek_fac.fw"

/* until B'd rev 05 */
/* User firmware */
#define FW_IMAGE_NAME_PATEK_OLD		"tsp_synaptics/synaptics_patek_old.fw"
/* Factory firmware */
#define FAC_FWIMAGE_NAME_PATEK_OLD	"tsp_synaptics/synaptics_patek_fac_old.fw"

#define SYNAPTICS_MAX_FW_PATH		64

#define SYNAPTICS_DEFAULT_UMS_FW	"/sdcard/synaptics.fw"

#define DATE_OF_FIRMWARE_BIN_OFFSET	0xEF00
#define IC_REVISION_BIN_OFFSET		0xEF02
#define FW_VERSION_BIN_OFFSET		0xEF03

#define PDT_PROPS			(0X00EF)
#define PDT_START			(0x00E9)
#define PDT_END				(0x000A)
#define PDT_ENTRY_SIZE			(0x0006)
#define PAGES_TO_SERVICE		(10)
#define PAGE_SELECT_LEN			(2)

#define SYNAPTICS_RMI4_F01		(0x01)
#define SYNAPTICS_RMI4_F12		(0x12)
#define SYNAPTICS_RMI4_F34		(0x34)
#define SYNAPTICS_RMI4_F51		(0x51)
#define SYNAPTICS_RMI4_F54		(0x54)

#define SYNAPTICS_RMI4_PRODUCT_INFO_SIZE	2
#define SYNAPTICS_RMI4_DATE_CODE_SIZE		3
#define SYNAPTICS_RMI4_PRODUCT_ID_SIZE		10
#define SYNAPTICS_RMI4_BUILD_ID_SIZE		3
#define SYNAPTICS_RMI4_PRODUCT_ID_LENGTH	10

#define MAX_INTR_REGISTERS		4
#define MAX_NUMBER_OF_FINGERS		10

#define MASK_16BIT			0xFFFF
#define MASK_8BIT			0xFF
#define MASK_7BIT			0x7F
#define MASK_6BIT			0x3F
#define MASK_5BIT			0x1F
#define MASK_4BIT			0x0F
#define MASK_3BIT			0x07
#define MASK_2BIT			0x03
#define MASK_1BIT			0x01

/*
 * struct synaptics_rmi4_fn_desc - function descriptor fields in PDT
 * @query_base_addr: base address for query registers
 * @cmd_base_addr: base address for command registers
 * @ctrl_base_addr: base address for control registers
 * @data_base_addr: base address for data registers
 * @intr_src_count: number of interrupt sources
 * @fn_number: function number
 */
struct synaptics_rmi4_fn_desc {
	unsigned char query_base_addr;
	unsigned char cmd_base_addr;
	unsigned char ctrl_base_addr;
	unsigned char data_base_addr;
	unsigned char intr_src_count;
	unsigned char fn_number;
};

/*
 * synaptics_rmi4_fn_full_addr - full 16-bit base addresses
 * @query_base: 16-bit base address for query registers
 * @cmd_base: 16-bit base address for data registers
 * @ctrl_base: 16-bit base address for command registers
 * @data_base: 16-bit base address for control registers
 */
struct synaptics_rmi4_fn_full_addr {
	unsigned short query_base;
	unsigned short cmd_base;
	unsigned short ctrl_base;
	unsigned short data_base;
};

/*
 * struct synaptics_rmi4_fn - function handler data structure
 * @fn_number: function number
 * @num_of_data_sources: number of data sources
 * @num_of_data_points: maximum number of fingers supported
 * @size_of_data_register_block: data register block size
 * @data1_offset: offset to data1 register from data base address
 * @intr_reg_num: index to associated interrupt register
 * @intr_mask: interrupt mask
 * @full_addr: full 16-bit base addresses of function registers
 * @link: linked list for function handlers
 * @data_size: size of private data
 * @data: pointer to private data
 */
struct synaptics_rmi4_fn {
	unsigned char fn_number;
	unsigned char num_of_data_sources;
	unsigned char num_of_data_points;
	unsigned char size_of_data_register_block;
	unsigned char data1_offset;
	unsigned char intr_reg_num;
	unsigned char intr_mask;
	struct synaptics_rmi4_fn_full_addr full_addr;
	struct list_head link;
	int data_size;
	void *data;
};

struct synaptics_rmi4_device_tree_data {
	int tsp_int;
	int num_of_supply;
	const char **name_of_supply;
	struct regulator_bulk_data *supplies;
	int tsp_sel;
	int tsp_scl;
	int tsp_sda;
	int hall_ic;
	int fpga_mainclk;
	int cresetb;
	int cdone;

	char x_flip;
	char y_flip;
};

/*
 * struct synaptics_rmi4_device_info - device information
 * @version_major: rmi protocol major version number
 * @version_minor: rmi protocol minor version number
 * @manufacturer_id: manufacturer id
 * @product_props: product properties information
 * @product_info: product info array
 * @date_code: device manufacture date
 * @tester_id: tester id array
 * @serial_number: device serial number
 * @product_id_string: device product id
 * @support_fn_list: linked list for function handlers
 */
struct synaptics_rmi4_device_info {
	unsigned int version_major;
	unsigned int version_minor;
	unsigned char manufacturer_id;
	unsigned char product_props;
	unsigned char product_info[SYNAPTICS_RMI4_PRODUCT_INFO_SIZE];
	unsigned char date_code[SYNAPTICS_RMI4_DATE_CODE_SIZE];
	unsigned short tester_id;
	unsigned short serial_number;
	unsigned char product_id_string[SYNAPTICS_RMI4_PRODUCT_ID_SIZE + 1];
	unsigned char build_id[SYNAPTICS_RMI4_BUILD_ID_SIZE];
	struct list_head support_fn_list;
};

/**
 * struct synaptics_finger - Represents fingers.
 * @ state: finger status.
 * @ mcount: moving counter for debug.
 */
struct synaptics_finger {
	unsigned char state;
	unsigned short mcount;
};

#if defined(TSP_PATTERN_TRACKING_METHOD)
struct pattern_tracking {
	int tcount_finger[TSP_PT_MAX_GHOSTCHECK_FINGER];
	int touchbx[TSP_PT_MAX_GHOSTCHECK_FINGER];
	int touchby[TSP_PT_MAX_GHOSTCHECK_FINGER];
	int ghosttouchcount;
	bool is_working;
};
#endif

#ifdef CHARGER_NOTIFIER
struct synaptics_cable {
	    struct work_struct work;
	    struct notifier_block nb;
	    struct extcon_specific_cable_nb extcon_nb;
	    struct extcon_dev *edev;
	    enum extcon_cable_name cable_type;
	    unsigned long cable_state;
};
#endif

/*
 * struct synaptics_rmi4_data - rmi4 device instance data
 * @i2c_client: pointer to associated i2c client
 * @input_dev: pointer to associated input device
 * @board: constant pointer to platform data
 * @rmi4_mod_info: device information
 * @regulator: pointer to associated regulator
 * @rmi4_io_ctrl_mutex: mutex for i2c i/o control
 * @early_suspend: instance to support early suspend power management
 * @current_page: current page in sensor to acess
 * @full_pm_cycle: flag for full power management cycle in early suspend stage
 * @num_of_intr_regs: number of interrupt registers
 * @f01_query_base_addr: query base address for f01
 * @f01_cmd_base_addr: command base address for f01
 * @f01_ctrl_base_addr: control base address for f01
 * @f01_data_base_addr: data base address for f01
 * @irq: attention interrupt
 * @sensor_max_x: sensor maximum x value
 * @sensor_max_y: sensor maximum y value
 * @irq_enabled: flag for indicating interrupt enable status
 * @touch_stopped: flag to stop interrupt thread processing
 * @fingers_on_2d: flag to indicate presence of fingers in 2d area
 * @sensor_sleep: flag to indicate sleep state of sensor
 * @wait: wait queue for touch data polling in interrupt thread
 * @i2c_read: pointer to i2c read function
 * @i2c_write: pointer to i2c write function
 * @irq_enable: pointer to irq enable function
 */
struct synaptics_rmi4_data {
	struct i2c_client *i2c_client;
	struct input_dev *input_dev;
	struct synaptics_rmi4_device_tree_data *dt_data;
	struct synaptics_rmi4_device_info rmi4_mod_info;
	struct regulator *regulator;
	struct mutex rmi4_reset_mutex;
	struct mutex rmi4_io_ctrl_mutex;
	struct mutex rmi4_reflash_mutex;
	struct timer_list f51_finger_timer;

	struct completion init_done;
	struct synaptics_finger finger[MAX_NUMBER_OF_FINGERS];

	unsigned char current_page;
	unsigned char full_pm_cycle;
	unsigned char num_of_rx;
	unsigned char num_of_tx;
	unsigned char num_of_node;
	unsigned char num_of_fingers;
	unsigned char max_touch_width;
	unsigned char intr_mask[MAX_INTR_REGISTERS];
	unsigned short num_of_intr_regs;
	unsigned short f01_query_base_addr;
	unsigned short f01_cmd_base_addr;
	unsigned short f01_ctrl_base_addr;
	unsigned short f01_data_base_addr;
	int irq;
	int sensor_max_x;
	int sensor_max_y;
	int touch_threshold;
	int gloved_sensitivity;
	int ta_status;
	bool flash_prog_mode;
	bool irq_enabled;
	bool touch_stopped;
	bool fingers_on_2d;
	bool f51_finger;
	bool f51_finger_is_hover;
	bool sensor_sleep;
	bool stay_awake;
	bool staying_awake;
	bool tsp_probe;
#ifdef CHARGER_NOTIFIER
	bool cable_state;
#endif

	const char *firmware_name;
	const char *fac_firmware_name;

	int ic_revision_of_ic;		/* revision of reading from IC */
	int fw_version_of_ic;		/* firmware version of IC */
	int ic_revision_of_bin;		/* revision of reading from binary */
	int fw_version_of_bin;		/* firmware version of binary */
	int fw_release_date_of_ic;	/* Config release data from IC */
	bool doing_reflash;
	int rebootcount;
#ifdef CONFIG_DUAL_LCD
	int flip_status;
#endif
#ifdef TSP_PATTERN_TRACKING_METHOD
	struct pattern_tracking pattern_data;
#endif

	struct regulator *vreg_2p5;
	struct regulator *vreg_2p95;
	const struct firmware *fpga_fw;
	int Is_clk_enabled;
	int enable_counte;

#ifdef TSP_BOOSTER
	struct delayed_work	work_dvfs_off;
	struct delayed_work	work_dvfs_chg;
	struct mutex		dvfs_lock;
	bool dvfs_lock_status;
	int dvfs_old_stauts;
	int dvfs_boost_mode;
	int dvfs_freq;
#endif

	bool hover_status_in_normal_mode;
	bool hover_called;
	bool hover_ic;

#ifdef CONFIG_GLOVE_TOUCH
	unsigned char glove_mode_feature;
	unsigned char glove_mode_enables;
	unsigned short glove_mode_enables_addr;
	bool fast_glove_state;
	bool touchkey_glove_mode_status;
#endif
#ifdef USE_OPEN_DWORK
	struct delayed_work open_work;
#endif
	struct delayed_work rezero_work;
#ifdef TSP_PATTERN_TRACKING_METHOD
	struct delayed_work reboot_work;
#endif
	struct mutex rmi4_device_mutex;
	int (*i2c_read)(struct synaptics_rmi4_data *pdata, unsigned short addr,
			unsigned char *data, unsigned short length);
	int (*i2c_write)(struct synaptics_rmi4_data *pdata, unsigned short addr,
			unsigned char *data, unsigned short length);
	int (*irq_enable)(struct synaptics_rmi4_data *rmi4_data, bool enable);
	int (*reset_device)(struct synaptics_rmi4_data *rmi4_data);
	int (*stop_device)(struct synaptics_rmi4_data *rmi4_data);
	int (*start_device)(struct synaptics_rmi4_data *rmi4_data);
};

enum exp_fn {
	RMI_DEV = 0,
	RMI_F54,
	RMI_FW_UPDATER,
	RMI_LAST,
};

struct synaptics_rmi4_exp_fn_ptr {
	int (*read)(struct synaptics_rmi4_data *rmi4_data, unsigned short addr,
			unsigned char *data, unsigned short length);
	int (*write)(struct synaptics_rmi4_data *rmi4_data, unsigned short addr,
			unsigned char *data, unsigned short length);
	int (*enable)(struct synaptics_rmi4_data *rmi4_data, bool enable);
};

struct synaptics_rmi4_f01_device_status {
	union {
		struct {
			unsigned char status_code:4;
			unsigned char reserved:2;
			unsigned char flash_prog:1;
			unsigned char unconfigured:1;
		} __packed;
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f12_query_5 {
	union {
		struct {
			unsigned char size_of_query6;
			struct {
				unsigned char ctrl0_is_present:1;
				unsigned char ctrl1_is_present:1;
				unsigned char ctrl2_is_present:1;
				unsigned char ctrl3_is_present:1;
				unsigned char ctrl4_is_present:1;
				unsigned char ctrl5_is_present:1;
				unsigned char ctrl6_is_present:1;
				unsigned char ctrl7_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl8_is_present:1;
				unsigned char ctrl9_is_present:1;
				unsigned char ctrl10_is_present:1;
				unsigned char ctrl11_is_present:1;
				unsigned char ctrl12_is_present:1;
				unsigned char ctrl13_is_present:1;
				unsigned char ctrl14_is_present:1;
				unsigned char ctrl15_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl16_is_present:1;
				unsigned char ctrl17_is_present:1;
				unsigned char ctrl18_is_present:1;
				unsigned char ctrl19_is_present:1;
				unsigned char ctrl20_is_present:1;
				unsigned char ctrl21_is_present:1;
				unsigned char ctrl22_is_present:1;
				unsigned char ctrl23_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl24_is_present:1;
				unsigned char ctrl25_is_present:1;
				unsigned char ctrl26_is_present:1;
				unsigned char ctrl27_is_present:1;
				unsigned char ctrl28_is_present:1;
				unsigned char ctrl29_is_present:1;
				unsigned char ctrl30_is_present:1;
				unsigned char ctrl31_is_present:1;
			} __packed;
		};
		unsigned char data[5];
	};
};

struct synaptics_rmi4_f12_query_8 {
	union {
		struct {
			unsigned char size_of_query9;
			struct {
				unsigned char data0_is_present:1;
				unsigned char data1_is_present:1;
				unsigned char data2_is_present:1;
				unsigned char data3_is_present:1;
				unsigned char data4_is_present:1;
				unsigned char data5_is_present:1;
				unsigned char data6_is_present:1;
				unsigned char data7_is_present:1;
			} __packed;
		};
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f12_ctrl_8 {
	union {
		struct {
			unsigned char max_x_coord_lsb;
			unsigned char max_x_coord_msb;
			unsigned char max_y_coord_lsb;
			unsigned char max_y_coord_msb;
			unsigned char rx_pitch_lsb;
			unsigned char rx_pitch_msb;
			unsigned char tx_pitch_lsb;
			unsigned char tx_pitch_msb;
			unsigned char low_rx_clip;
			unsigned char high_rx_clip;
			unsigned char low_tx_clip;
			unsigned char high_tx_clip;
			unsigned char num_of_rx;
			unsigned char num_of_tx;
		};
		unsigned char data[14];
	};
};

struct synaptics_rmi4_f12_ctrl_9 {
	union {
		struct {
			unsigned char touch_threshold;
			unsigned char lift_hysteresis;
			unsigned char small_z_scale_factor_lsb;
			unsigned char small_z_scale_factor_msb;
			unsigned char large_z_scale_factor_lsb;
			unsigned char large_z_scale_factor_msb;
			unsigned char small_large_boundary;
			unsigned char wx_scale;
			unsigned char wx_offset;
			unsigned char wy_scale;
			unsigned char wy_offset;
			unsigned char x_size_lsb;
			unsigned char x_size_msb;
			unsigned char y_size_lsb;
			unsigned char y_size_msb;
			unsigned char gloved_finger;
		};
		unsigned char data[16];
	};
};

struct synaptics_rmi4_f12_ctrl_23 {
	union {
		struct {
			unsigned char obj_type_enable;
			unsigned char max_reported_objects;
		};
		unsigned char data[2];
	};
};

#ifdef CONFIG_GLOVE_TOUCH
struct synaptics_rmi4_f12_ctrl_26 {
	union {
		struct {
			unsigned char glove_feature_enable;
		};
		unsigned char data[1];
	};
};
#endif

struct synaptics_rmi4_f12_finger_data {
	unsigned char object_type_and_status;
	unsigned char x_lsb;
	unsigned char x_msb;
	unsigned char y_lsb;
	unsigned char y_msb;
#ifdef REPORT_2D_Z
	unsigned char z;
#endif
#ifdef REPORT_2D_W
	unsigned char wx;
	unsigned char wy;
#endif
};

struct synaptics_rmi4_f34_ctrl_3 {
	union {
		struct {
			unsigned char fw_release_month;
			unsigned char fw_release_date;
			unsigned char fw_release_revision;
			unsigned char fw_release_version;
		};
		unsigned char data[4];
	};
};

struct synaptics_rmi4_f34_fn_ptr {
	int (*read)(struct synaptics_rmi4_data *rmi4_data, unsigned short addr,
			unsigned char *data, unsigned short length);
	int (*write)(struct synaptics_rmi4_data *rmi4_data, unsigned short addr,
			unsigned char *data, unsigned short length);
	int (*enable)(struct synaptics_rmi4_data *rmi4_data, bool enable);
};

struct synaptics_rmi4_f34_handle {
	unsigned char status;
	unsigned char cmd;
	unsigned short bootloaderid;
	unsigned short blocksize;
	unsigned short imageblockcount;
	unsigned short configblockcount;
	unsigned short blocknum;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	bool inflashprogmode;
	unsigned char intr_mask;
	struct mutex attn_mutex;
	struct synaptics_rmi4_f34_fn_ptr *fn_ptr;
};

#ifdef PROXIMITY
struct synaptics_rmi4_f51_query {
	union {
		struct {
			unsigned char query_register_count;
			unsigned char data_register_count;
			unsigned char control_register_count;
			unsigned char command_register_count;
			unsigned char proximity_controls;
		};
		unsigned char data[5];
	};
};

struct synaptics_rmi4_f51_data {
	union {
		struct {
			unsigned char finger_hover_det:1;
			unsigned char air_swipe_det:1;
			unsigned char large_obj_det:1;
			unsigned char hover_pinch_det:1;
			unsigned char hover_finger_x_4__11;
			unsigned char hover_finger_y_4__11;
			unsigned char hover_finger_xy_0__3;
			unsigned char hover_finger_z;
			unsigned char air_swipe_dir_0:1;
			unsigned char air_swipe_dir_1:1;
			unsigned char object_present:1;
			unsigned char large_obj_act:2;
		} __packed;
		unsigned char proximity_data[6];
	};
};

struct synaptics_rmi4_f51_handle {
	unsigned char proximity_enables;
	unsigned short proximity_enables_addr;
	unsigned char num_of_data_sources;
#ifdef USE_CUSTOM_REZERO
	unsigned short proximity_custom_rezero_addr;
#endif
	unsigned char proximity_controls;
	struct synaptics_rmi4_data *rmi4_data;
};
#endif

struct synaptics_rmi4_exp_fn {
	enum exp_fn fn_type;
	bool initialized;
	int (*func_init)(struct synaptics_rmi4_data *rmi4_data);
	void (*func_remove)(struct synaptics_rmi4_data *rmi4_data);
	void (*func_attn)(struct synaptics_rmi4_data *rmi4_data,
			unsigned char intr_mask);
	struct list_head link;
};

int synaptics_rmi4_new_function(enum exp_fn fn_type,
		int (*func_init)(struct synaptics_rmi4_data *rmi4_data),
		void (*func_remove)(struct synaptics_rmi4_data *rmi4_data),
		void (*func_attn)(struct synaptics_rmi4_data *rmi4_data,
				unsigned char intr_mask));
int rmidev_module_register(void);
int rmi4_f54_module_register(void);
int synaptics_rmi4_f54_set_control(struct synaptics_rmi4_data *rmi4_data);
int rmi4_fw_update_module_register(void);
int synaptics_fw_updater(unsigned char *fw_data);
int synaptics_rmi4_fw_update_on_probe(struct synaptics_rmi4_data *rmi4_data);
int synaptics_rmi4_proximity_enables(unsigned char enables);
int synaptics_rmi4_glove_mode_enables(struct synaptics_rmi4_data *rmi4_data);
int synaptics_proximity_no_sleep_set(bool enables);
int synaptics_rmi4_force_calibration(void);
void ice40_fpga_firmware_update(struct synaptics_rmi4_data *rmi4_data);
void fpga_enable(struct synaptics_rmi4_data *rmi4_data, int enable_clk);

extern struct class *sec_class;
extern int get_lcd_attached(void);

static inline ssize_t synaptics_rmi4_show_error(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	dev_warn(dev, "%s Attempted to read from write-only attribute %s\n",
			__func__, attr->attr.name);
	return -EPERM;
}

static inline ssize_t synaptics_rmi4_store_error(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	dev_warn(dev, "%s Attempted to write to read-only attribute %s\n",
			__func__, attr->attr.name);
	return -EPERM;
}

static inline void batohs(unsigned short *dest, unsigned char *src)
{
	*dest = src[1] * 0x100 + src[0];
}

static inline void hstoba(unsigned char *dest, unsigned short src)
{
	dest[0] = src % 0x100;
	dest[1] = src / 0x100;
}
#endif
