/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/ctype.h>
#include <linux/hrtimer.h>
#include <linux/firmware.h>
#include <linux/gpio.h>

#include "synaptics_i2c_rmi.h"

#define FACTORY_MODE

#ifdef TOUCHKEY_ENABLE
#define NUM_OF_BUTTONS 2
#endif
#ifdef SIDE_TOUCH
#define NUM_OF_SIDE_BUTTONS	6
#endif

#define CMD_REPORT_TYPE_DELTA	2
#define CMD_REPORT_TYPE_RAWCAP	20
#define CMD_GET_REPORT	1

#define TSP_RAWCAP_MAX	6000
#define TSP_RAWCAP_MIN	300
#define TSP_DELTA_MAX	10
#define TSP_DELTA_MIN	-10

#define WATCHDOG_HRTIMER
#define WATCHDOG_TIMEOUT_S 2
#define FORCE_TIMEOUT_100MS 10
#define STATUS_WORK_INTERVAL 20 /* ms */

/*
#define RAW_HEX
#define HUMAN_READABLE
 */

#define STATUS_IDLE 0
#define STATUS_BUSY 1
#define STATUS_ERROR 2

#define DATA_REPORT_INDEX_OFFSET 1
#define DATA_REPORT_DATA_OFFSET 3

#define SENSOR_RX_MAPPING_OFFSET 1
#define SENSOR_TX_MAPPING_OFFSET 2

#define COMMAND_GET_REPORT 1
#define COMMAND_FORCE_CAL 2
#define COMMAND_FORCE_UPDATE 4

#define CONTROL_42_SIZE 2
#define CONTROL_43_54_SIZE 13
#define CONTROL_55_56_SIZE 2
#define CONTROL_58_SIZE 1
#define CONTROL_59_SIZE 2
#define CONTROL_60_62_SIZE 3
#define CONTROL_63_SIZE 1
#define CONTROL_64_67_SIZE 4
#define CONTROL_68_73_SIZE 8
#define CONTROL_74_SIZE 2
#define CONTROL_76_SIZE 1
#define CONTROL_77_78_SIZE 2
#define CONTROL_79_83_SIZE 5
#define CONTROL_84_85_SIZE 2
#define CONTROL_86_SIZE 1
#define CONTROL_87_SIZE 1

#define HIGH_RESISTANCE_DATA_SIZE 6
#define FULL_RAW_CAP_MIN_MAX_DATA_SIZE 4
#define TREX_DATA_SIZE 7

#define NO_AUTO_CAL_MASK 0x01

#define concat(a, b) a##b
#define tostring(x) #x

#define GROUP(_attrs) {\
	.attrs = _attrs,\
}

#define attrify(propname) (&dev_attr_##propname.attr)

#define show_prototype(propname)\
	static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
			struct device *dev,\
			struct device_attribute *attr,\
			char *buf);\
\
struct device_attribute dev_attr_##propname =\
__ATTR(propname, S_IRUGO,\
		concat(synaptics_rmi4_f54, _##propname##_show),\
		synaptics_rmi4_store_error);

#define store_prototype(propname)\
	static ssize_t concat(synaptics_rmi4_f54, _##propname##_store)(\
			struct device *dev,\
			struct device_attribute *attr,\
			const char *buf, size_t count);\
\
struct device_attribute dev_attr_##propname =\
__ATTR(propname, S_IWUSR | S_IWGRP,\
		synaptics_rmi4_show_error,\
		concat(synaptics_rmi4_f54, _##propname##_store));

#define show_store_prototype(propname)\
	static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
			struct device *dev,\
			struct device_attribute *attr,\
			char *buf);\
\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count);\
\
struct device_attribute dev_attr_##propname =\
__ATTR(propname, (S_IRUGO | S_IWUSR | S_IWGRP),\
		concat(synaptics_rmi4_f54, _##propname##_show),\
		concat(synaptics_rmi4_f54, _##propname##_store));

#define simple_show_func(rtype, propname, fmt)\
	static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
			struct device *dev,\
			struct device_attribute *attr,\
			char *buf)\
{\
	return snprintf(buf, PAGE_SIZE, fmt, f54->rtype.propname);\
} \

#define simple_show_func_unsigned(rtype, propname)\
	simple_show_func(rtype, propname, "%u\n")

#define show_func(rtype, rgrp, propname, fmt)\
	static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
			struct device *dev,\
			struct device_attribute *attr,\
			char *buf)\
{\
	int retval;\
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;\
	\
	mutex_lock(&f54->rtype##_mutex);\
	\
	retval = f54->fn_ptr->read(rmi4_data,\
			f54->rtype.rgrp->address,\
			f54->rtype.rgrp->data,\
			sizeof(f54->rtype.rgrp->data));\
	mutex_unlock(&f54->rtype##_mutex);\
	if (retval < 0) {\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Failed to read " #rtype\
				" " #rgrp "\n",\
				__func__);\
		return retval;\
	} \
	\
	return snprintf(buf, PAGE_SIZE, fmt,\
			f54->rtype.rgrp->propname);\
} \

#define show_store_func(rtype, rgrp, propname, fmt)\
	show_func(rtype, rgrp, propname, fmt)\
\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count)\
{\
	int retval;\
	unsigned long setting;\
	unsigned long o_setting;\
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;\
	\
	retval = kstrtoul(buf, 10, &setting);\
	if (retval)\
	return retval;\
	\
	mutex_lock(&f54->rtype##_mutex);\
	retval = f54->fn_ptr->read(rmi4_data,\
			f54->rtype.rgrp->address,\
			f54->rtype.rgrp->data,\
			sizeof(f54->rtype.rgrp->data));\
	if (retval < 0) {\
		mutex_unlock(&f54->rtype##_mutex);\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Failed to read " #rtype\
				" " #rgrp "\n",\
				__func__);\
		return retval;\
	} \
	\
	if (f54->rtype.rgrp->propname == setting) {\
		mutex_unlock(&f54->rtype##_mutex);\
		return count;\
	} \
	\
	o_setting = f54->rtype.rgrp->propname;\
	f54->rtype.rgrp->propname = setting;\
	\
	retval = f54->fn_ptr->write(rmi4_data,\
			f54->rtype.rgrp->address,\
			f54->rtype.rgrp->data,\
			sizeof(f54->rtype.rgrp->data));\
	if (retval < 0) {\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Failed to write " #rtype\
				" " #rgrp "\n",\
				__func__);\
		f54->rtype.rgrp->propname = o_setting;\
		mutex_unlock(&f54->rtype##_mutex);\
		return retval;\
	} \
	\
	mutex_unlock(&f54->rtype##_mutex);\
	return count;\
} \

#define show_store_func_unsigned(rtype, rgrp, propname)\
	show_store_func(rtype, rgrp, propname, "%u\n")

#define show_replicated_func(rtype, rgrp, propname, fmt)\
	static ssize_t concat(synaptics_rmi4_f54, _##propname##_show)(\
			struct device *dev,\
			struct device_attribute *attr,\
			char *buf)\
{\
	int retval;\
	int size = 0;\
	unsigned char ii;\
	unsigned char length;\
	unsigned char *temp;\
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;\
	\
	mutex_lock(&f54->rtype##_mutex);\
	\
	length = f54->rtype.rgrp->length;\
	\
	retval = f54->fn_ptr->read(rmi4_data,\
			f54->rtype.rgrp->address,\
			(unsigned char *)f54->rtype.rgrp->data,\
			length);\
	mutex_unlock(&f54->rtype##_mutex);\
	if (retval < 0) {\
		dev_dbg(&rmi4_data->i2c_client->dev,\
				"%s: Failed to read " #rtype\
				" " #rgrp "\n",\
				__func__);\
	} \
	\
	temp = buf;\
	\
	for (ii = 0; ii < length; ii++) {\
		retval = snprintf(temp, PAGE_SIZE - size, fmt " ",\
				f54->rtype.rgrp->data[ii].propname);\
		if (retval < 0) {\
			dev_err(&rmi4_data->i2c_client->dev,\
					"%s: Faild to write output\n",\
					__func__);\
			return retval;\
		} \
		size += retval;\
		temp += retval;\
	} \
	\
	retval = snprintf(temp, PAGE_SIZE - size, "\n");\
	if (retval < 0) {\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Faild to write null terminator\n",\
				__func__);\
		return retval;\
	} \
	\
	return size + retval;\
} \

#define show_replicated_func_unsigned(rtype, rgrp, propname)\
	show_replicated_func(rtype, rgrp, propname, "%u")

#define show_store_replicated_func(rtype, rgrp, propname, fmt)\
	show_replicated_func(rtype, rgrp, propname, fmt)\
\
static ssize_t concat(synaptics_rmi4_f54, _##propname##_store)(\
		struct device *dev,\
		struct device_attribute *attr,\
		const char *buf, size_t count)\
{\
	int retval;\
	unsigned int setting;\
	unsigned char ii;\
	unsigned char length;\
	const unsigned char *temp;\
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;\
	\
	mutex_lock(&f54->rtype##_mutex);\
	\
	length = f54->rtype.rgrp->length;\
	\
	retval = f54->fn_ptr->read(rmi4_data,\
			f54->rtype.rgrp->address,\
			(unsigned char *)f54->rtype.rgrp->data,\
			length);\
	if (retval < 0) {\
		dev_dbg(&rmi4_data->i2c_client->dev,\
				"%s: Failed to read " #rtype\
				" " #rgrp "\n",\
				__func__);\
	} \
	\
	temp = buf;\
	\
	for (ii = 0; ii < length; ii++) {\
		if (sscanf(temp, fmt, &setting) == 1) {\
			f54->rtype.rgrp->data[ii].propname = setting;\
		} else {\
			retval = f54->fn_ptr->read(rmi4_data,\
					f54->rtype.rgrp->address,\
					(unsigned char *)f54->rtype.rgrp->data,\
					length);\
			mutex_unlock(&f54->rtype##_mutex);\
			return -EINVAL;\
		} \
		\
		while (*temp != 0) {\
			temp++;\
			if (isspace(*(temp - 1)) && !isspace(*temp))\
			break;\
		} \
	} \
	\
	retval = f54->fn_ptr->write(rmi4_data,\
			f54->rtype.rgrp->address,\
			(unsigned char *)f54->rtype.rgrp->data,\
			length);\
	mutex_unlock(&f54->rtype##_mutex);\
	if (retval < 0) {\
		dev_err(&rmi4_data->i2c_client->dev,\
				"%s: Failed to write " #rtype\
				" " #rgrp "\n",\
				__func__);\
		return retval;\
	} \
	\
	return count;\
} \

#define show_store_replicated_func_unsigned(rtype, rgrp, propname)\
	show_store_replicated_func(rtype, rgrp, propname, "%u")

enum f54_report_types {
	F54_8BIT_IMAGE = 1,
	F54_16BIT_IMAGE = 2,
	F54_RAW_16BIT_IMAGE = 3,
	F54_HIGH_RESISTANCE = 4,
	F54_TX_TO_TX_SHORT = 5,
	F54_RX_TO_RX1 = 7,
	F54_TRUE_BASELINE = 9,
	F54_FULL_RAW_CAP_MIN_MAX = 13,
	F54_RX_OPENS1 = 14,
	F54_TX_OPEN = 15,
	F54_TX_TO_GROUND = 16,
	F54_RX_TO_RX2 = 17,
	F54_RX_OPENS2 = 18,
	F54_FULL_RAW_CAP = 19,
	F54_FULL_RAW_CAP_RX_COUPLING_COMP = 20,
	F54_SENSOR_SPEED = 22,
	F54_ADC_RANGE = 23,
	F54_TREX_OPENS = 24,
	F54_TREX_TO_GND = 25,
	F54_TREX_SHORTS = 26,
	F54_ABS_CAP = 38,
	F54_ABS_DELTA = 40,
	F54_ABS_ADC = 42,
	INVALID_REPORT_TYPE = -1,
};

struct f54_query {
	union {
		struct {
			/* query 0 */
			unsigned char num_of_rx_electrodes;

			/* query 1 */
			unsigned char num_of_tx_electrodes;

			/* query 2 */
			unsigned char f54_query2_b0__1:2;
			unsigned char has_baseline:1;
			unsigned char has_image8:1;
			unsigned char f54_query2_b4__5:2;
			unsigned char has_image16:1;
			unsigned char f54_query2_b7:1;

			/* queries 3.0 and 3.1 */
			unsigned short clock_rate;

			/* query 4 */
			unsigned char touch_controller_family;

			/* query 5 */
			unsigned char has_pixel_touch_threshold_adjustment:1;
			unsigned char f54_query5_b1__7:7;

			/* query 6 */
			unsigned char has_sensor_assignment:1;
			unsigned char has_interference_metric:1;
			unsigned char has_sense_frequency_control:1;
			unsigned char has_firmware_noise_mitigation:1;
			unsigned char has_ctrl11:1;
			unsigned char has_two_byte_report_rate:1;
			unsigned char has_one_byte_report_rate:1;
			unsigned char has_relaxation_control:1;

			/* query 7 */
			unsigned char curve_compensation_mode:2;
			unsigned char f54_query7_b2__7:6;

			/* query 8 */
			unsigned char f54_query8_b0:1;
			unsigned char has_iir_filter:1;
			unsigned char has_cmn_removal:1;
			unsigned char has_cmn_maximum:1;
			unsigned char has_touch_hysteresis:1;
			unsigned char has_edge_compensation:1;
			unsigned char has_per_frequency_noise_control:1;
			unsigned char has_enhanced_stretch:1;

			/* query 9 */
			unsigned char has_force_fast_relaxation:1;
			unsigned char has_multi_metric_state_machine:1;
			unsigned char has_signal_clarity:1;
			unsigned char has_variance_metric:1;
			unsigned char has_0d_relaxation_control:1;
			unsigned char has_0d_acquisition_control:1;
			unsigned char has_status:1;
			unsigned char has_slew_metric:1;

			/* query 10 */
			unsigned char has_h_blank:1;
			unsigned char has_v_blank:1;
			unsigned char has_long_h_blank:1;
			unsigned char has_startup_fast_relaxation:1;
			unsigned char has_esd_control:1;
			unsigned char has_noise_mitigation2:1;
			unsigned char has_noise_state:1;
			unsigned char has_energy_ratio_relaxation:1;

			/* query 11 */
			unsigned char has_excessive_noise_reporting:1;
			unsigned char has_slew_option:1;
			unsigned char has_two_overhead_bursts:1;
			unsigned char has_query13:1;
			unsigned char has_one_overhead_burst:1;
			unsigned char f54_query11_b5:1;
			unsigned char has_ctrl88:1;
			unsigned char has_query15:1;

			/* query 12 */
			unsigned char number_of_sensing_frequencies:4;
			unsigned char f54_query12_b4__7:4;

			/* query 13 */
			unsigned char has_ctrl86:1;
			unsigned char has_ctrl87:1;
			unsigned char has_ctrl87_sub0:1;
			unsigned char has_ctrl87_sub1:1;
			unsigned char has_ctrl87_sub2:1;
			unsigned char has_cidim:1;
			unsigned char has_noise_mitigation_enhancement:1;
			unsigned char has_rail_im:1;
		} __packed;
		unsigned char data[15];
	};
};

struct f54_control_0 {
	union {
		struct {
			unsigned char no_relax:1;
			unsigned char no_scan:1;
			unsigned char force_fast_relaxation:1;
			unsigned char startup_fast_relaxation:1;
			unsigned char gesture_cancels_sfr:1;
			unsigned char enable_energy_ratio_relaxation:1;
			unsigned char excessive_noise_attn_enable:1;
			unsigned char f54_control0_b7:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_1 {
	union {
		struct {
			unsigned char bursts_per_cluster:4;
			unsigned char f54_ctrl1_b4__7:4;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_2 {
	union {
		struct {
			unsigned short saturation_cap;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_3 {
	union {
		struct {
			unsigned char pixel_touch_threshold;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_4__6 {
	union {
		struct {
			/* control 4 */
			unsigned char rx_feedback_cap:2;
			unsigned char bias_current:2;
			unsigned char f54_ctrl4_b4__7:4;

			/* control 5 */
			unsigned char low_ref_cap:2;
			unsigned char low_ref_feedback_cap:2;
			unsigned char low_ref_polarity:1;
			unsigned char f54_ctrl5_b5__7:3;

			/* control 6 */
			unsigned char high_ref_cap:2;
			unsigned char high_ref_feedback_cap:2;
			unsigned char high_ref_polarity:1;
			unsigned char f54_ctrl6_b5__7:3;
		} __packed;
		struct {
			unsigned char data[3];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_7 {
	union {
		struct {
			unsigned char cbc_cap:3;
			unsigned char cbc_polarity:1;
			unsigned char cbc_tx_carrier_selection:1;
			unsigned char f54_ctrl7_b5__7:3;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_8__9 {
	union {
		struct {
			/* control 8 */
			unsigned short integration_duration:10;
			unsigned short f54_ctrl8_b10__15:6;

			/* control 9 */
			unsigned char reset_duration;
		} __packed;
		struct {
			unsigned char data[3];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_10 {
	union {
		struct {
			unsigned char noise_sensing_bursts_per_image:4;
			unsigned char f54_ctrl10_b4__7:4;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_11 {
	union {
		struct {
			unsigned short f54_ctrl11;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_12__13 {
	union {
		struct {
			/* control 12 */
			unsigned char slow_relaxation_rate;

			/* control 13 */
			unsigned char fast_relaxation_rate;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_14 {
	union {
		struct {
			unsigned char rxs_on_xaxis:1;
			unsigned char curve_comp_on_txs:1;
			unsigned char f54_ctrl14_b2__7:6;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_15n {
	unsigned char sensor_rx_assignment;
};

struct f54_control_15 {
	struct f54_control_15n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_16n {
	unsigned char sensor_tx_assignment;
};

struct f54_control_16 {
	struct f54_control_16n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_17n {
	unsigned char burst_count_b8__10:3;
	unsigned char disable:1;
	unsigned char f54_ctrl17_b4:1;
	unsigned char filter_bandwidth:3;
};

struct f54_control_17 {
	struct f54_control_17n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_18n {
	unsigned char burst_count_b0__7;
};

struct f54_control_18 {
	struct f54_control_18n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_19n {
	unsigned char stretch_duration;
};

struct f54_control_19 {
	struct f54_control_19n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_20 {
	union {
		struct {
			unsigned char disable_noise_mitigation:1;
			unsigned char f54_ctrl20_b1__7:7;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_21 {
	union {
		struct {
			unsigned short freq_shift_noise_threshold;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_22__26 {
	union {
		struct {
			/* control 22 */
			unsigned char f54_ctrl22;

			/* control 23 */
			unsigned short medium_noise_threshold;

			/* control 24 */
			unsigned short high_noise_threshold;

			/* control 25 */
			unsigned char noise_density;

			/* control 26 */
			unsigned char frame_count;
		} __packed;
		struct {
			unsigned char data[7];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_27 {
	union {
		struct {
			unsigned char iir_filter_coef;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_28 {
	union {
		struct {
			unsigned short quiet_threshold;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_29 {
	union {
		struct {
			/* control 29 */
			unsigned char f54_ctrl29_b0__6:7;
			unsigned char cmn_filter_disable:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_30 {
	union {
		struct {
			unsigned char cmn_filter_max;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_31 {
	union {
		struct {
			unsigned char touch_hysteresis;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_32__35 {
	union {
		struct {
			/* control 32 */
			unsigned short rx_low_edge_comp;

			/* control 33 */
			unsigned short rx_high_edge_comp;

			/* control 34 */
			unsigned short tx_low_edge_comp;

			/* control 35 */
			unsigned short tx_high_edge_comp;
		} __packed;
		struct {
			unsigned char data[8];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_36n {
	unsigned char axis1_comp;
};

struct f54_control_36 {
	struct f54_control_36n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_37n {
	unsigned char axis2_comp;
};

struct f54_control_37 {
	struct f54_control_37n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_38n {
	unsigned char noise_control_1;
};

struct f54_control_38 {
	struct f54_control_38n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_39n {
	unsigned char noise_control_2;
};

struct f54_control_39 {
	struct f54_control_39n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_40n {
	unsigned char noise_control_3;
};

struct f54_control_40 {
	struct f54_control_40n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_41 {
	union {
		struct {
			unsigned char no_signal_clarity:1;
			unsigned char f54_ctrl41_b1__7:7;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_57 {
	union {
		struct {
			unsigned char cbc_cap_0d:3;
			unsigned char cbc_polarity_0d:1;
			unsigned char cbc_tx_carrier_selection_0d:1;
			unsigned char f54_ctrl57_b5__7:3;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_88 {
	union {
		struct {
			unsigned char tx_low_reference_polarity:1;
			unsigned char tx_high_reference_polarity:1;
			unsigned char abs_low_reference_polarity:1;
			unsigned char abs_polarity:1;
			unsigned char cbc_polarity:1;
			unsigned char cbc_tx_carrier_selection:1;
			unsigned char charge_pump_enable:1;
			unsigned char cbc_abs_auto_servo:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control {
	struct f54_control_0 *reg_0;
	struct f54_control_1 *reg_1;
	struct f54_control_2 *reg_2;
	struct f54_control_3 *reg_3;
	struct f54_control_4__6 *reg_4__6;
	struct f54_control_7 *reg_7;
	struct f54_control_8__9 *reg_8__9;
	struct f54_control_10 *reg_10;
	struct f54_control_11 *reg_11;
	struct f54_control_12__13 *reg_12__13;
	struct f54_control_14 *reg_14;
	struct f54_control_15 *reg_15;
	struct f54_control_16 *reg_16;
	struct f54_control_17 *reg_17;
	struct f54_control_18 *reg_18;
	struct f54_control_19 *reg_19;
	struct f54_control_20 *reg_20;
	struct f54_control_21 *reg_21;
	struct f54_control_22__26 *reg_22__26;
	struct f54_control_27 *reg_27;
	struct f54_control_28 *reg_28;
	struct f54_control_29 *reg_29;
	struct f54_control_30 *reg_30;
	struct f54_control_31 *reg_31;
	struct f54_control_32__35 *reg_32__35;
	struct f54_control_36 *reg_36;
	struct f54_control_37 *reg_37;
	struct f54_control_38 *reg_38;
	struct f54_control_39 *reg_39;
	struct f54_control_40 *reg_40;
	struct f54_control_41 *reg_41;
	struct f54_control_57 *reg_57;
	struct f54_control_88 *reg_88;
};

#ifdef FACTORY_MODE
#include <linux/uaccess.h>

#define CMD_STR_LEN 32
#define CMD_PARAM_NUM 8
#define CMD_RESULT_STR_LEN 896
#define FT_CMD(name, func) .cmd_name = name, .cmd_func = func

enum CMD_STATUS {
	CMD_STATUS_WAITING = 0,
	CMD_STATUS_RUNNING,
	CMD_STATUS_OK,
	CMD_STATUS_FAIL,
	CMD_STATUS_NOT_APPLICABLE,
};

struct ft_cmd {
	const char *cmd_name;
	void (*cmd_func)(void);
	struct list_head list;
};

struct factory_data {
	struct device *fac_dev_ts;
	short *rawcap_data;
	short *test_data;
	short *delta_data;
	int *abscap_data;
	int *absdelta_data;
	char *trx_short;
	bool cmd_is_running;
	unsigned char cmd_state;
	char cmd[CMD_STR_LEN];
	int cmd_param[CMD_PARAM_NUM];
	char cmd_buff[CMD_RESULT_STR_LEN];
	char cmd_result[CMD_RESULT_STR_LEN];
	struct mutex cmd_lock;
	struct list_head cmd_list_head;
#ifdef TOUCHKEY_ENABLE
	struct device *fac_dev_tskey;
	int tkey_delta_offset[NUM_OF_BUTTONS];
	unsigned char dummy_rx;
	unsigned char dummy_tx;
#endif
};


extern void synaptics_power_ctrl(struct synaptics_rmi4_data *rmi4_data, bool enable);

static int synaptics_rmi4_f54_get_report_type(int type);

static ssize_t cmd_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count);

static ssize_t cmd_status_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t cmd_result_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t cmd_list_show(struct device *dev,
		struct device_attribute *attr, char *buf);

#ifdef TOUCHKEY_ENABLE
static ssize_t sec_touchkey_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t sec_touchkey_menu_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t sec_touchkey_back_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t touchkey_led_control(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t touchkey_led_state_show(struct device *dev,
		struct device_attribute *attr, char *buf);

#ifdef TKEY_BOOSTER
static ssize_t boost_level_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
#endif

static DEVICE_ATTR(touchkey_threshold, S_IRUGO | S_IWUSR, sec_touchkey_threshold_show, NULL);
#ifdef USE_RECENT_TOUCHKEY
static DEVICE_ATTR(touchkey_recent, S_IRUGO | S_IWUSR, sec_touchkey_menu_show, NULL);
#else
static DEVICE_ATTR(touchkey_menu, S_IRUGO | S_IWUSR, sec_touchkey_menu_show, NULL);
#endif
static DEVICE_ATTR(touchkey_back, S_IRUGO | S_IWUSR, sec_touchkey_back_show, NULL);
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, touchkey_led_state_show, touchkey_led_control);
#ifdef TKEY_BOOSTER
static DEVICE_ATTR(boost_level, S_IWUSR | S_IWGRP, NULL, boost_level_store);
#endif

#endif

static DEVICE_ATTR(cmd, S_IWUSR | S_IWGRP, NULL, cmd_store);
static DEVICE_ATTR(cmd_status, S_IRUGO, cmd_status_show, NULL);
static DEVICE_ATTR(cmd_result, S_IRUGO, cmd_result_show, NULL);
static DEVICE_ATTR(cmd_list, S_IRUGO, cmd_list_show, NULL);


static struct attribute *cmd_attributes[] = {
	&dev_attr_cmd.attr,
	&dev_attr_cmd_status.attr,
	&dev_attr_cmd_result.attr,
	&dev_attr_cmd_list.attr,
	NULL,
};

#ifdef TOUCHKEY_ENABLE
static struct attribute *tskey_attributes[] = {
	&dev_attr_touchkey_threshold.attr,
#ifdef USE_RECENT_TOUCHKEY
	&dev_attr_touchkey_recent.attr,
#else
	&dev_attr_touchkey_menu.attr,
#endif
	&dev_attr_touchkey_back.attr,
	&dev_attr_brightness.attr,
#ifdef TKEY_BOOSTER
	&dev_attr_boost_level.attr,
#endif
	NULL,
};

static struct attribute_group tskey_attr_group = {
	.attrs = tskey_attributes,
};
#endif

static struct attribute_group cmd_attr_group = {
	.attrs = cmd_attributes,
};

static void fw_update(void);
static void get_fw_ver_bin(void);
static void get_fw_ver_ic(void);
static void get_fac_fw_ver_bin(void);
static void get_config_ver(void);
static void get_threshold(void);
static void module_off_master(void);
static void module_on_master(void);
static void get_module_vendor(void);
static void get_chip_vendor(void);
static void get_chip_name(void);
static void get_x_num(void);
static void get_y_num(void);
static void get_rawcap(void);
static void run_rawcap_read(void);
static void run_no_interrupt_test(void);
static void get_delta(void);
static void run_delta_read(void);
static void run_abscap_read(void);
static void run_absdelta_read(void);
static void run_trx_short_test(void);
#ifdef PROXIMITY
static void hover_enable(void);
static void hover_no_sleep_enable(void);
#ifdef USE_EDGE_EXCLUSION
static void hover_set_edge_rx(void);
#endif
#endif
static void set_jitter_level(void);
#ifdef HAND_GRIP_MODE
static void handgrip_enable(void);
#endif
#ifdef GLOVE_MODE
static void glove_mode(void);
static void clear_cover_mode(void);
static void get_glove_sensitivity(void);
static void fast_glove_mode(void);
int synaptics_rmi4_glove_mode_enables(struct synaptics_rmi4_data *rmi4_data);
#endif
#ifdef TOUCHKEY_ENABLE
static void run_deltacap_read(void);
#endif
#ifdef TSP_BOOSTER
static void boost_level(void);
#endif
#ifdef SIDE_TOUCH
static void sidekey_enable(void);
static void get_sidekey_threshold(void);
static void sidekey_partial_enable(void);
static void set_sidekey_delta(void);
static void run_sidekey_delta_read(void);
static void run_sidekey_abscap_read(void);
static void set_deepsleep_mode(void);
static void set_sidekey_only_enable(void);
static void lozemode_enable(void);
#endif
static void set_tsp_test_result(void);
static void get_tsp_test_result(void);
#ifdef USE_STYLUS
static void stylus_enable(void);
#endif
#ifdef USE_ACTIVE_REPORT_RATE
static void report_rate(void);
#endif
static void debug_log(void);
static void not_support_cmd(void);

struct ft_cmd ft_cmds[] = {
	{FT_CMD("fw_update", fw_update),},
	{FT_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{FT_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{FT_CMD("get_fac_fw_ver_bin", get_fac_fw_ver_bin),},
	{FT_CMD("get_config_ver", get_config_ver),},
	{FT_CMD("get_threshold", get_threshold),},
	{FT_CMD("module_off_master", module_off_master),},
	{FT_CMD("module_on_master", module_on_master),},
	{FT_CMD("module_off_slave", not_support_cmd),},
	{FT_CMD("module_on_slave", not_support_cmd),},
	{FT_CMD("get_module_vendor", get_module_vendor),},
	{FT_CMD("get_chip_vendor", get_chip_vendor),},
	{FT_CMD("get_chip_name", get_chip_name),},
	{FT_CMD("get_x_num", get_x_num),},
	{FT_CMD("get_y_num", get_y_num),},
	{FT_CMD("get_rawcap", get_rawcap),},
	{FT_CMD("run_rawcap_read", run_rawcap_read),},
	{FT_CMD("run_no_interrupt_test", run_no_interrupt_test),},
	{FT_CMD("get_delta", get_delta),},
	{FT_CMD("run_delta_read", run_delta_read),},
	{FT_CMD("run_abscap_read", run_abscap_read),},
	{FT_CMD("run_absdelta_read", run_absdelta_read),},
	{FT_CMD("run_trx_short_test", run_trx_short_test),},
#ifdef PROXIMITY
	{FT_CMD("hover_enable", hover_enable),},
	{FT_CMD("hover_no_sleep_enable", hover_no_sleep_enable),},
#ifdef USE_EDGE_EXCLUSION
	{FT_CMD("hover_set_edge_rx", hover_set_edge_rx),},
#endif
#endif
	{FT_CMD("set_jitter_level", set_jitter_level),},
#ifdef HAND_GRIP_MODE
	{FT_CMD("handgrip_enable", handgrip_enable),},
#endif
#ifdef TOUCHKEY_ENABLE
	{FT_CMD("run_deltacap_read", run_deltacap_read),},
#endif
#ifdef GLOVE_MODE
	{FT_CMD("glove_mode", glove_mode),},
	{FT_CMD("clear_cover_mode", clear_cover_mode),},
	{FT_CMD("get_glove_sensitivity", get_glove_sensitivity),},
	{FT_CMD("fast_glove_mode", fast_glove_mode),},
#endif
#ifdef TSP_BOOSTER
	{FT_CMD("boost_level", boost_level),},
#endif
#ifdef SIDE_TOUCH
	{FT_CMD("sidekey_enable", sidekey_enable),},
	{FT_CMD("get_sidekey_threshold", get_sidekey_threshold),},
	{FT_CMD("sidekey_partial_enable", sidekey_partial_enable),},
	{FT_CMD("set_sidekey_delta", set_sidekey_delta),},
	{FT_CMD("run_sidekey_delta_read", run_sidekey_delta_read),},
	{FT_CMD("run_sidekey_abscap_read", run_sidekey_abscap_read),},
	{FT_CMD("set_deepsleep_mode", set_deepsleep_mode),},
	{FT_CMD("set_sidekey_only_enable", set_sidekey_only_enable),},
	{FT_CMD("lozemode_enable", lozemode_enable),},
#endif
	{FT_CMD("set_tsp_test_result", set_tsp_test_result),},
	{FT_CMD("get_tsp_test_result", get_tsp_test_result),},
#ifdef USE_STYLUS
	{FT_CMD("stylus_enable", stylus_enable),},
#endif
#ifdef USE_ACTIVE_REPORT_RATE
	{FT_CMD("report_rate", report_rate),},
#endif
	{FT_CMD("debug_log", debug_log),},
	{FT_CMD("not_support_cmd", not_support_cmd),},
};
#endif

struct synaptics_rmi4_f54_handle {
	bool no_auto_cal;
	unsigned char status;
	unsigned char intr_mask;
	unsigned char intr_reg_num;
	unsigned char rx_assigned;
	unsigned char tx_assigned;
	unsigned char *report_data;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	unsigned short fifoindex;
	unsigned int report_size;
	unsigned int data_buffer_size;
	enum f54_report_types report_type;
	struct mutex status_mutex;
	struct mutex data_mutex;
	struct mutex control_mutex;
	struct f54_query query;
	struct f54_control control;

#ifdef FACTORY_MODE
	struct factory_data *factory_data;
#endif
	struct kobject *attr_dir;
	struct hrtimer watchdog;
	struct work_struct timeout_work;
	struct delayed_work status_work;
	struct workqueue_struct *status_workqueue;
	struct synaptics_rmi4_exp_fn_ptr *fn_ptr;
	struct synaptics_rmi4_data *rmi4_data;
};

struct f55_query {
	union {
		struct {
			/* query 0 */
			unsigned char num_of_rx_electrodes;

			/* query 1 */
			unsigned char num_of_tx_electrodes;

			/* query 2 */
			unsigned char has_sensor_assignment:1;
			unsigned char has_edge_compensation:1;
			unsigned char curve_compensation_mode:2;
			unsigned char has_ctrl6:1;
			unsigned char has_alternate_transmitter_assignment:1;
			unsigned char has_single_layer_multi_touch:1;
			unsigned char has_query5:1;
		} __packed;
		unsigned char data[3];
	};
};

struct synaptics_rmi4_f55_handle {
	unsigned char *rx_assignment;
	unsigned char *tx_assignment;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	struct f55_query query;
};

	show_prototype(status)
	show_prototype(report_size)
	show_store_prototype(no_auto_cal)
	show_store_prototype(report_type)
	show_store_prototype(fifoindex)
	store_prototype(do_preparation)
	store_prototype(get_report)
	store_prototype(force_cal)
	show_prototype(num_of_mapped_rx)
	show_prototype(num_of_mapped_tx)
	show_prototype(num_of_rx_electrodes)
	show_prototype(num_of_tx_electrodes)
	show_prototype(has_image16)
	show_prototype(has_image8)
	show_prototype(has_baseline)
	show_prototype(clock_rate)
	show_prototype(touch_controller_family)
	show_prototype(has_pixel_touch_threshold_adjustment)
	show_prototype(has_sensor_assignment)
	show_prototype(has_interference_metric)
	show_prototype(has_sense_frequency_control)
	show_prototype(has_firmware_noise_mitigation)
	show_prototype(has_two_byte_report_rate)
	show_prototype(has_one_byte_report_rate)
	show_prototype(has_relaxation_control)
	show_prototype(curve_compensation_mode)
	show_prototype(has_iir_filter)
	show_prototype(has_cmn_removal)
	show_prototype(has_cmn_maximum)
	show_prototype(has_touch_hysteresis)
	show_prototype(has_edge_compensation)
	show_prototype(has_per_frequency_noise_control)
	show_prototype(has_signal_clarity)
	show_prototype(number_of_sensing_frequencies)

	show_store_prototype(no_relax)
	show_store_prototype(no_scan)
	show_store_prototype(bursts_per_cluster)
	show_store_prototype(saturation_cap)
	show_store_prototype(pixel_touch_threshold)
	show_store_prototype(rx_feedback_cap)
	show_store_prototype(low_ref_cap)
	show_store_prototype(low_ref_feedback_cap)
	show_store_prototype(low_ref_polarity)
	show_store_prototype(high_ref_cap)
	show_store_prototype(high_ref_feedback_cap)
	show_store_prototype(high_ref_polarity)
	show_store_prototype(cbc_cap)
	show_store_prototype(cbc_polarity)
	show_store_prototype(cbc_tx_carrier_selection)
	show_store_prototype(integration_duration)
	show_store_prototype(reset_duration)
	show_store_prototype(noise_sensing_bursts_per_image)
	show_store_prototype(slow_relaxation_rate)
	show_store_prototype(fast_relaxation_rate)
	show_store_prototype(rxs_on_xaxis)
	show_store_prototype(curve_comp_on_txs)
	show_prototype(sensor_rx_assignment)
	show_prototype(sensor_tx_assignment)
	show_prototype(burst_count)
	show_prototype(disable)
	show_prototype(filter_bandwidth)
	show_prototype(stretch_duration)
	show_store_prototype(disable_noise_mitigation)
	show_store_prototype(freq_shift_noise_threshold)
	show_store_prototype(medium_noise_threshold)
	show_store_prototype(high_noise_threshold)
	show_store_prototype(noise_density)
	show_store_prototype(frame_count)
	show_store_prototype(iir_filter_coef)
	show_store_prototype(quiet_threshold)
	show_store_prototype(cmn_filter_disable)
	show_store_prototype(cmn_filter_max)
	show_store_prototype(touch_hysteresis)
	show_store_prototype(rx_low_edge_comp)
	show_store_prototype(rx_high_edge_comp)
	show_store_prototype(tx_low_edge_comp)
	show_store_prototype(tx_high_edge_comp)
	show_store_prototype(axis1_comp)
	show_store_prototype(axis2_comp)
	show_prototype(noise_control_1)
	show_prototype(noise_control_2)
	show_prototype(noise_control_3)
	show_store_prototype(no_signal_clarity)
	show_store_prototype(cbc_cap_0d)
	show_store_prototype(cbc_polarity_0d)
	show_store_prototype(cbc_tx_carrier_selection_0d)

static ssize_t synaptics_rmi4_f54_data_read(struct file *data_file,
			struct kobject *kobj, struct bin_attribute *attributes,
			char *buf, loff_t pos, size_t count);

static struct attribute *attrs[] = {
	attrify(status),
	attrify(report_size),
	attrify(no_auto_cal),
	attrify(report_type),
	attrify(fifoindex),
	attrify(do_preparation),
	attrify(get_report),
	attrify(force_cal),
	attrify(num_of_mapped_rx),
	attrify(num_of_mapped_tx),
	attrify(num_of_rx_electrodes),
	attrify(num_of_tx_electrodes),
	attrify(has_image16),
	attrify(has_image8),
	attrify(has_baseline),
	attrify(clock_rate),
	attrify(touch_controller_family),
	attrify(has_pixel_touch_threshold_adjustment),
	attrify(has_sensor_assignment),
	attrify(has_interference_metric),
	attrify(has_sense_frequency_control),
	attrify(has_firmware_noise_mitigation),
	attrify(has_two_byte_report_rate),
	attrify(has_one_byte_report_rate),
	attrify(has_relaxation_control),
	attrify(curve_compensation_mode),
	attrify(has_iir_filter),
	attrify(has_cmn_removal),
	attrify(has_cmn_maximum),
	attrify(has_touch_hysteresis),
	attrify(has_edge_compensation),
	attrify(has_per_frequency_noise_control),
	attrify(has_signal_clarity),
	attrify(number_of_sensing_frequencies),
	NULL,
};

static struct attribute_group attr_group = GROUP(attrs);

static struct attribute *attrs_reg_0[] = {
	attrify(no_relax),
	attrify(no_scan),
	NULL,
};

static struct attribute *attrs_reg_1[] = {
	attrify(bursts_per_cluster),
	NULL,
};

static struct attribute *attrs_reg_2[] = {
	attrify(saturation_cap),
	NULL,
};

static struct attribute *attrs_reg_3[] = {
	attrify(pixel_touch_threshold),
	NULL,
};

static struct attribute *attrs_reg_4__6[] = {
	attrify(rx_feedback_cap),
	attrify(low_ref_cap),
	attrify(low_ref_feedback_cap),
	attrify(low_ref_polarity),
	attrify(high_ref_cap),
	attrify(high_ref_feedback_cap),
	attrify(high_ref_polarity),
	NULL,
};

static struct attribute *attrs_reg_7[] = {
	attrify(cbc_cap),
	attrify(cbc_polarity),
	attrify(cbc_tx_carrier_selection),
	NULL,
};

static struct attribute *attrs_reg_8__9[] = {
	attrify(integration_duration),
	attrify(reset_duration),
	NULL,
};

static struct attribute *attrs_reg_10[] = {
	attrify(noise_sensing_bursts_per_image),
	NULL,
};

static struct attribute *attrs_reg_11[] = {
	NULL,
};

static struct attribute *attrs_reg_12__13[] = {
	attrify(slow_relaxation_rate),
	attrify(fast_relaxation_rate),
	NULL,
};

static struct attribute *attrs_reg_14__16[] = {
	attrify(rxs_on_xaxis),
	attrify(curve_comp_on_txs),
	attrify(sensor_rx_assignment),
	attrify(sensor_tx_assignment),
	NULL,
};

static struct attribute *attrs_reg_17__19[] = {
	attrify(burst_count),
	attrify(disable),
	attrify(filter_bandwidth),
	attrify(stretch_duration),
	NULL,
};

static struct attribute *attrs_reg_20[] = {
	attrify(disable_noise_mitigation),
	NULL,
};

static struct attribute *attrs_reg_21[] = {
	attrify(freq_shift_noise_threshold),
	NULL,
};

static struct attribute *attrs_reg_22__26[] = {
	attrify(medium_noise_threshold),
	attrify(high_noise_threshold),
	attrify(noise_density),
	attrify(frame_count),
	NULL,
};

static struct attribute *attrs_reg_27[] = {
	attrify(iir_filter_coef),
	NULL,
};

static struct attribute *attrs_reg_28[] = {
	attrify(quiet_threshold),
	NULL,
};

static struct attribute *attrs_reg_29[] = {
	attrify(cmn_filter_disable),
	NULL,
};

static struct attribute *attrs_reg_30[] = {
	attrify(cmn_filter_max),
	NULL,
};

static struct attribute *attrs_reg_31[] = {
	attrify(touch_hysteresis),
	NULL,
};

static struct attribute *attrs_reg_32__35[] = {
	attrify(rx_low_edge_comp),
	attrify(rx_high_edge_comp),
	attrify(tx_low_edge_comp),
	attrify(tx_high_edge_comp),
	NULL,
};

static struct attribute *attrs_reg_36[] = {
	attrify(axis1_comp),
	NULL,
};

static struct attribute *attrs_reg_37[] = {
	attrify(axis2_comp),
	NULL,
};

static struct attribute *attrs_reg_38__40[] = {
	attrify(noise_control_1),
	attrify(noise_control_2),
	attrify(noise_control_3),
	NULL,
};

static struct attribute *attrs_reg_41[] = {
	attrify(no_signal_clarity),
	NULL,
};

static struct attribute *attrs_reg_57[] = {
	attrify(cbc_cap_0d),
	attrify(cbc_polarity_0d),
	attrify(cbc_tx_carrier_selection_0d),
	NULL,
};

static struct attribute_group attrs_ctrl_regs[] = {
	GROUP(attrs_reg_0),
	GROUP(attrs_reg_1),
	GROUP(attrs_reg_2),
	GROUP(attrs_reg_3),
	GROUP(attrs_reg_4__6),
	GROUP(attrs_reg_7),
	GROUP(attrs_reg_8__9),
	GROUP(attrs_reg_10),
	GROUP(attrs_reg_11),
	GROUP(attrs_reg_12__13),
	GROUP(attrs_reg_14__16),
	GROUP(attrs_reg_17__19),
	GROUP(attrs_reg_20),
	GROUP(attrs_reg_21),
	GROUP(attrs_reg_22__26),
	GROUP(attrs_reg_27),
	GROUP(attrs_reg_28),
	GROUP(attrs_reg_29),
	GROUP(attrs_reg_30),
	GROUP(attrs_reg_31),
	GROUP(attrs_reg_32__35),
	GROUP(attrs_reg_36),
	GROUP(attrs_reg_37),
	GROUP(attrs_reg_38__40),
	GROUP(attrs_reg_41),
	GROUP(attrs_reg_57),
};

static bool attrs_ctrl_regs_exist[ARRAY_SIZE(attrs_ctrl_regs)];

static struct bin_attribute dev_report_data = {
	.attr = {
		.name = "report_data",
		.mode = S_IRUGO,
	},
	.size = 0,
	.read = synaptics_rmi4_f54_data_read,
};

static struct synaptics_rmi4_f54_handle *f54;
static struct synaptics_rmi4_f55_handle *f55;

static bool is_report_type_valid(enum f54_report_types report_type)
{
	switch (report_type) {
		case F54_8BIT_IMAGE:
		case F54_16BIT_IMAGE:
		case F54_RAW_16BIT_IMAGE:
		case F54_HIGH_RESISTANCE:
		case F54_TX_TO_TX_SHORT:
		case F54_RX_TO_RX1:
		case F54_TRUE_BASELINE:
		case F54_FULL_RAW_CAP_MIN_MAX:
		case F54_RX_OPENS1:
		case F54_TX_OPEN:
		case F54_TX_TO_GROUND:
		case F54_RX_TO_RX2:
		case F54_RX_OPENS2:
		case F54_FULL_RAW_CAP:
		case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
		case F54_SENSOR_SPEED:
		case F54_ADC_RANGE:
		case F54_TREX_OPENS:
		case F54_TREX_TO_GND:
		case F54_TREX_SHORTS:
		case F54_ABS_CAP:
		case F54_ABS_DELTA:
		case F54_ABS_ADC:
			return true;
			break;
		default:
			f54->report_type = INVALID_REPORT_TYPE;
			f54->report_size = 0;
			return false;
	}
}

static void set_report_size(void)
{
	int retval;
	unsigned char rx = f54->rx_assigned;
	unsigned char tx = f54->tx_assigned;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	switch (f54->report_type) {
		case F54_8BIT_IMAGE:
			f54->report_size = rx * tx;
			break;
		case F54_16BIT_IMAGE:
		case F54_RAW_16BIT_IMAGE:
		case F54_TRUE_BASELINE:
		case F54_FULL_RAW_CAP:
		case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
		case F54_SENSOR_SPEED:
			f54->report_size = 2 * rx * tx;
			break;
		case F54_HIGH_RESISTANCE:
			f54->report_size = HIGH_RESISTANCE_DATA_SIZE;
			break;
		case F54_TX_TO_TX_SHORT:
		case F54_TX_OPEN:
		case F54_TX_TO_GROUND:
			f54->report_size = (tx + 7) / 8;
			break;
		case F54_RX_TO_RX1:
		case F54_RX_OPENS1:
			if (rx < tx)
				f54->report_size = 2 * rx * rx;
			else
				f54->report_size = 2 * rx * tx;
			break;
		case F54_FULL_RAW_CAP_MIN_MAX:
			f54->report_size = FULL_RAW_CAP_MIN_MAX_DATA_SIZE;
			break;
		case F54_RX_TO_RX2:
		case F54_RX_OPENS2:
			if (rx <= tx)
				f54->report_size = 0;
			else
				f54->report_size = 2 * rx * (rx - tx);
			break;
		case F54_ADC_RANGE:
			if (f54->query.has_signal_clarity) {
				mutex_lock(&f54->control_mutex);
				retval = f54->fn_ptr->read(rmi4_data,
						f54->control.reg_41->address,
						f54->control.reg_41->data,
						sizeof(f54->control.reg_41->data));
				mutex_unlock(&f54->control_mutex);
				if (retval < 0) {
					dev_dbg(&rmi4_data->i2c_client->dev,
							"%s: Failed to read control reg_41\n",
							__func__);
					f54->report_size = 0;
					break;
				}
				if (!f54->control.reg_41->no_signal_clarity) {
					if (tx % 4)
						tx += 4 - (tx % 4);
				}
			}
			f54->report_size = 2 * rx * tx;
			break;
		case F54_TREX_OPENS:
		case F54_TREX_TO_GND:
		case F54_TREX_SHORTS:
			f54->report_size = TREX_DATA_SIZE;
			break;
		case F54_ABS_CAP:
		case F54_ABS_DELTA:
#ifdef SIDE_TOUCH
			if (rmi4_data->sidekey_test)
				f54->report_size = 4 * (rx + tx + 6);
			else
				f54->report_size = 4 * (rx + tx);
#else
			f54->report_size = 4 * (rx + tx);
#endif
			break;
		case F54_ABS_ADC:
			f54->report_size = 2 * (rx + tx);
			break;
		default:
			f54->report_size = 0;
	}

	return;
}

static int set_interrupt(bool set)
{
	int retval;
	unsigned char ii;
	unsigned char zero = 0x00;
	unsigned char *intr_mask;
	unsigned short f01_ctrl_reg;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	intr_mask = rmi4_data->intr_mask;
	f01_ctrl_reg = rmi4_data->f01_ctrl_base_addr + 1 + f54->intr_reg_num;

	if (!set) {
		retval = f54->fn_ptr->write(rmi4_data,
				f01_ctrl_reg,
				&zero,
				sizeof(zero));
		if (retval < 0)
			return retval;
	}

	for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
		if (intr_mask[ii] != 0x00) {
			f01_ctrl_reg = rmi4_data->f01_ctrl_base_addr + 1 + ii;
			if (set) {
				retval = f54->fn_ptr->write(rmi4_data,
						f01_ctrl_reg,
						&zero,
						sizeof(zero));
				if (retval < 0)
					return retval;
			} else {
				retval = f54->fn_ptr->write(rmi4_data,
						f01_ctrl_reg,
						&(intr_mask[ii]),
						sizeof(intr_mask[ii]));
				if (retval < 0)
					return retval;
			}
		}
	}

	f01_ctrl_reg = rmi4_data->f01_ctrl_base_addr + 1 + f54->intr_reg_num;

	if (set) {
		retval = f54->fn_ptr->write(rmi4_data,
				f01_ctrl_reg,
				&f54->intr_mask,
				1);
		if (retval < 0)
			return retval;
	}

	return 0;
}

static int do_preparation(void)
{
	int retval;
	unsigned char value;
	unsigned char command;
	unsigned char timeout_count;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	mutex_lock(&f54->control_mutex);

	if (f54->query.touch_controller_family == 1) {
		value = 0;
		retval = f54->fn_ptr->write(rmi4_data,
				f54->control.reg_7->address,
				&value,
				sizeof(f54->control.reg_7->data));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to disable CBC\n",
					__func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
	} else if (f54->query.has_ctrl88 == 1) {
		retval = f54->fn_ptr->read(rmi4_data,
				f54->control.reg_88->address,
				f54->control.reg_88->data,
				sizeof(f54->control.reg_88->data));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to disable CBC (read ctrl88)\n",
					__func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
		f54->control.reg_88->cbc_polarity = 0;
		f54->control.reg_88->cbc_tx_carrier_selection = 0;
		retval = f54->fn_ptr->write(rmi4_data,
				f54->control.reg_88->address,
				f54->control.reg_88->data,
				sizeof(f54->control.reg_88->data));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to disable CBC (write ctrl88)\n",
					__func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
	}
	/* check this code to using S5000 and S5050 */
	if (f54->query.has_0d_acquisition_control) {
		value = 0;
		retval = f54->fn_ptr->write(rmi4_data,
				f54->control.reg_57->address,
				&value,
				sizeof(f54->control.reg_57->data));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to disable 0D CBC\n",
					__func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
	}

	if (f54->query.has_signal_clarity) {
		value = 1;
		retval = f54->fn_ptr->write(rmi4_data,
				f54->control.reg_41->address,
				&value,
				sizeof(f54->control.reg_41->data));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to disable signal clarity\n",
					__func__);
			mutex_unlock(&f54->control_mutex);
			return retval;
		}
	}

	mutex_unlock(&f54->control_mutex);

	command = (unsigned char)COMMAND_FORCE_UPDATE;

	retval = f54->fn_ptr->write(rmi4_data,
			f54->command_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write force update command\n",
				__func__);
		return retval;
	}

	timeout_count = 0;
	do {
		retval = f54->fn_ptr->read(rmi4_data,
				f54->command_base_addr,
				&value,
				sizeof(value));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to read command register\n",
					__func__);
			return retval;
		}

		if (value == 0x00)
			break;

		msleep(100);
		timeout_count++;
	} while (timeout_count < FORCE_TIMEOUT_100MS);

	if (timeout_count == FORCE_TIMEOUT_100MS) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Timed out waiting for force update\n",
				__func__);
		return -ETIMEDOUT;
	}

	command = (unsigned char)COMMAND_FORCE_CAL;

	retval = f54->fn_ptr->write(rmi4_data,
			f54->command_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write force cal command\n",
				__func__);
		return retval;
	}

	timeout_count = 0;
	do {
		retval = f54->fn_ptr->read(rmi4_data,
				f54->command_base_addr,
				&value,
				sizeof(value));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to read command register\n",
					__func__);
			return retval;
		}

		if (value == 0x00)
			break;

		msleep(100);
		timeout_count++;
	} while (timeout_count < FORCE_TIMEOUT_100MS);

	if (timeout_count == FORCE_TIMEOUT_100MS) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Timed out waiting for force cal\n",
				__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

#ifdef WATCHDOG_HRTIMER
static void timeout_set_status(struct work_struct *work)
{
	int retval;
	unsigned char command;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	pr_info("[synaptics] %s\n", __func__);
	mutex_lock(&f54->status_mutex);
	if (f54->status == STATUS_BUSY) {
		retval = f54->fn_ptr->read(rmi4_data,
				f54->command_base_addr,
				&command,
				sizeof(command));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to read command register\n",
					__func__);
			f54->status = STATUS_ERROR;
		} else if (command & COMMAND_GET_REPORT) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Report type not supported by FW\n",
					__func__);
			f54->status = STATUS_ERROR;
		} else {
			queue_delayed_work(f54->status_workqueue,
					&f54->status_work,
					0);
			mutex_unlock(&f54->status_mutex);
			return;
		}
		f54->report_type = INVALID_REPORT_TYPE;
		f54->report_size = 0;
	}
	mutex_unlock(&f54->status_mutex);

	/* read fail : need ic reset */
	if (f54->status == STATUS_ERROR) {
		if (rmi4_data->touch_stopped) {
			dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
					__func__);
			f54->status = STATUS_IDLE;
			return;
		}
		dev_err(&rmi4_data->i2c_client->dev, "%s: reset device\n",
				__func__);

		retval = rmi4_data->reset_device(rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to issue reset command, error = %d\n",
					__func__, retval);
		}

		mutex_lock(&f54->status_mutex);
		f54->status = STATUS_IDLE;
		mutex_unlock(&f54->status_mutex);
	}

	if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5000) {
		retval = rmi4_data->reset_device(rmi4_data);
		if (retval < 0)
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to issue reset command, error = %d\n",
					__func__, retval);
	}

	return;
}

static enum hrtimer_restart get_report_timeout(struct hrtimer *timer)
{
	schedule_work(&(f54->timeout_work));

	return HRTIMER_NORESTART;
}
#endif

#ifdef RAW_HEX
static void print_raw_hex_report(void)
{
	unsigned int ii;

	pr_info("%s: Report data (raw hex)\n", __func__);

	switch (f54->report_type) {
		case F54_16BIT_IMAGE:
		case F54_RAW_16BIT_IMAGE:
		case F54_HIGH_RESISTANCE:
		case F54_TRUE_BASELINE:
		case F54_FULL_RAW_CAP_MIN_MAX:
		case F54_FULL_RAW_CAP:
		case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
		case F54_SENSOR_SPEED:
		case F54_ADC_RANGE:
		case F54_ABS_ADC:
			for (ii = 0; ii < f54->report_size; ii += 2) {
				pr_info("%03d: 0x%02x%02x\n",
						ii / 2,
						f54->report_data[ii + 1],
						f54->report_data[ii]);
			}
			break;
		case F54_ABS_CAP:
		case F54_ABS_DELTA:
			for (ii = 0; ii < f54->report_size; ii += 4) {
				pr_info("%03d: 0x%02x%02x%02x%02x\n",
						ii / 4,
						f54->report_data[ii + 3],
						f54->report_data[ii + 2],
						f54->report_data[ii + 1],
						f54->report_data[ii]);
			}
			break;
		default:
			for (ii = 0; ii < f54->report_size; ii++)
				pr_info("%03d: 0x%02x\n", ii, f54->report_data[ii]);
			break;
	}

	return;
}
#endif

#ifdef HUMAN_READABLE
static void print_image_report(void)
{
	unsigned int ii;
	unsigned int jj;
	short *report_data;

	switch (f54->report_type) {
		case F54_16BIT_IMAGE:
		case F54_RAW_16BIT_IMAGE:
		case F54_TRUE_BASELINE:
		case F54_FULL_RAW_CAP:
		case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
			pr_info("%s: Report data (image)\n", __func__);
			report_data = (short *)f54->report_data;
			/* rmi4_data->num_tx -> tx_assinged */
			for (ii = 0; ii < f54->tx_assigned; ii++) {
				for (jj = 0; jj < f54->rx_assigned; jj++) {

					if (*report_data < -64)
						pr_cont(".");
					else if (*report_data < 0)
						pr_cont("-");
					else if (*report_data > 64)
						pr_cont("*");
					else if (*report_data > 0)
						pr_cont("+");
					else
						pr_cont("0");

					report_data++;
				}
				pr_info("");
			}
			pr_info("%s: End of report\n", __func__);
			break;
		default:
			pr_info("%s: Image not supported for report type %d\n",
					__func__, f54->report_type);
	}

	return;
}
#endif

static void free_control_mem(void)
{
	struct f54_control control = f54->control;

	kfree(control.reg_0);
	kfree(control.reg_1);
	kfree(control.reg_2);
	kfree(control.reg_3);
	kfree(control.reg_4__6);
	kfree(control.reg_7);
	kfree(control.reg_8__9);
	kfree(control.reg_10);
	kfree(control.reg_11);
	kfree(control.reg_12__13);
	kfree(control.reg_14);
	kfree(control.reg_15);
	kfree(control.reg_16);
	kfree(control.reg_17);
	kfree(control.reg_18);
	kfree(control.reg_19);
	kfree(control.reg_20);
	kfree(control.reg_21);
	kfree(control.reg_22__26);
	kfree(control.reg_27);
	kfree(control.reg_28);
	kfree(control.reg_29);
	kfree(control.reg_30);
	kfree(control.reg_31);
	kfree(control.reg_32__35);
	kfree(control.reg_36);
	kfree(control.reg_37);
	kfree(control.reg_38);
	kfree(control.reg_39);
	kfree(control.reg_40);
	kfree(control.reg_41);
	kfree(control.reg_57);

	return;
}

static void remove_sysfs(void)
{
	int reg_num;

	sysfs_remove_bin_file(f54->attr_dir, &dev_report_data);

	sysfs_remove_group(f54->attr_dir, &attr_group);

	for (reg_num = 0; reg_num < ARRAY_SIZE(attrs_ctrl_regs); reg_num++)
		sysfs_remove_group(f54->attr_dir, &attrs_ctrl_regs[reg_num]);

#ifdef FACTORY_MODE
	sysfs_remove_group(f54->attr_dir, &cmd_attr_group);
#endif

	kobject_put(f54->attr_dir);

	return;
}

#ifdef FACTORY_MODE
static void set_default_result(struct factory_data *data)
{
	char delim = ':';

	memset(data->cmd_buff, 0x00, sizeof(data->cmd_buff));
	memset(data->cmd_result, 0x00, sizeof(data->cmd_result));
	memcpy(data->cmd_result, data->cmd, strlen(data->cmd));
	strncat(data->cmd_result, &delim, 1);

	return;
}

static void set_cmd_result(struct factory_data *data, char *buf, int length)
{
	strncat(data->cmd_result, buf, length);

	return;
}

static ssize_t cmd_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned char param_cnt = 0;
	char *start;
	char *end;
	char *pos;
	char delim = ',';
	char buffer[CMD_STR_LEN];
	bool cmd_found = false;
	int *param;
	int length;
	struct ft_cmd *ft_cmd_ptr;
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	if (data->cmd_is_running == true) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: Still servicing previous command. Skip cmd :%s\n",
				__func__, buf);
		return count;
	}

	if ((int)count >= CMD_STR_LEN) {
		dev_info(&rmi4_data->i2c_client->dev, "%s: cmd size overflow![%d]\n",
				__func__, (int)count);
		return count;
	}

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = true;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_RUNNING;

	length = (int)count;
	if (*(buf + length - 1) == '\n')
		length--;

	memset(data->cmd, 0x00, sizeof(data->cmd));
	memcpy(data->cmd, buf, length);
	memset(data->cmd_param, 0, sizeof(data->cmd_param));

	memset(buffer, 0x00, sizeof(buffer));
	pos = strchr(buf, (int)delim);
	if (pos)
		memcpy(buffer, buf, pos - buf);
	else
		memcpy(buffer, buf, length);

	/* find command */
	list_for_each_entry(ft_cmd_ptr, &data->cmd_list_head, list) {
		if (!strcmp(buffer, ft_cmd_ptr->cmd_name)) {
			cmd_found = true;
			break;
		}
	}

	/* set not_support_cmd */
	if (!cmd_found) {
		list_for_each_entry(ft_cmd_ptr,
				&data->cmd_list_head, list) {
			if (!strcmp("not_support_cmd", ft_cmd_ptr->cmd_name))
				break;
		}
	}

	/* parsing parameters */
	if (cmd_found && pos) {
		pos++;
		start = pos;
		do {
			if ((*pos == delim) || (pos - buf == length)) {
				end = pos;
				memset(buffer, 0x00, sizeof(buffer));
				memcpy(buffer, start, end - start);
				*(buffer + strlen(buffer)) = '\0';
				param = data->cmd_param + param_cnt;
				if (kstrtoint(buffer, 10, param) < 0)
					break;
				param_cnt++;
				start = pos + 1;
			}
			pos++;
		} while ((pos - buf <= length) && (param_cnt < CMD_PARAM_NUM));
	}

	dev_info(&rmi4_data->i2c_client->dev, "%s: Command = %s\n",
			__func__, buf);

	ft_cmd_ptr->cmd_func();

	return count;
}

static ssize_t cmd_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char buffer[CMD_RESULT_STR_LEN];
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	dev_info(&rmi4_data->i2c_client->dev, "%s: Command status = %d\n",
			__func__, data->cmd_state);

	switch (data->cmd_state) {
		case CMD_STATUS_WAITING:
		snprintf(buffer, CMD_RESULT_STR_LEN, "%s", tostring(WAITING));
			break;
		case CMD_STATUS_RUNNING:
		snprintf(buffer, CMD_RESULT_STR_LEN, "%s", tostring(RUNNING));
			break;
		case CMD_STATUS_OK:
		snprintf(buffer, CMD_RESULT_STR_LEN, "%s", tostring(OK));
			break;
		case CMD_STATUS_FAIL:
		snprintf(buffer, CMD_RESULT_STR_LEN, "%s", tostring(FAIL));
			break;
		case CMD_STATUS_NOT_APPLICABLE:
		snprintf(buffer, CMD_RESULT_STR_LEN, "%s", tostring(NOT_APPLICABLE));
			break;
		default:
		snprintf(buffer, CMD_RESULT_STR_LEN, "%s", tostring(NOT_APPLICABLE));
			break;
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", buffer);
}

static ssize_t cmd_result_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	dev_info(&rmi4_data->i2c_client->dev, "%s: Command result = %s\n",
			__func__, data->cmd_result);

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;

	return snprintf(buf, PAGE_SIZE, "%s\n", data->cmd_result);
}

static ssize_t cmd_list_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ii = 0;
	char buffer[CMD_RESULT_STR_LEN];
	char buffer_name[CMD_STR_LEN];

	snprintf(buffer, 30, "++factory command list++\n");
	while (strncmp(ft_cmds[ii].cmd_name, "not_support_cmd", 16) != 0) {
		snprintf(buffer_name, CMD_STR_LEN, "%s\n", ft_cmds[ii].cmd_name);
		strncat(buffer, buffer_name, strlen(buffer_name));
		ii++;
		if (strlen(buffer) > (CMD_RESULT_STR_LEN - CMD_STR_LEN)) {
			dev_info(&f54->rmi4_data->i2c_client->dev,
				"%s: command lengh overflow\n", __func__);
			break;
		}
	}

	dev_info(&f54->rmi4_data->i2c_client->dev,
		"%s: length : %u / %d\n", __func__,
		strlen(buffer), CMD_RESULT_STR_LEN);
	return snprintf(buf, PAGE_SIZE, "%s\n", buffer);
}

/* synaptics_skip_firmware_update() return value
 * TRUE : skip update, false : update firmware
 */
static bool synaptics_skip_firmware_update(struct synaptics_rmi4_data *rmi4_data,
		const struct firmware *fw_entry)
{
	/* Read revision and firmware info from binary */
	if ((rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5100) &&
		(rmi4_data->ic_revision_of_ic >= SYNAPTICS_IC_REVISION_A2)) {
#if !defined(CONFIG_SEC_HESTIA_PROJECT)
		if (strncmp(rmi4_data->dt_data->project, "PSLTE", 5) == 0) {
			rmi4_data->ic_revision_of_bin = (int)fw_entry->data[IC_REVISION_BIN_OFFSET_S5100_PS];
			rmi4_data->fw_version_of_bin = (int)fw_entry->data[FW_VERSION_BIN_OFFSET_S5100_PS];
		} else {
			rmi4_data->ic_revision_of_bin = (int)fw_entry->data[IC_REVISION_BIN_OFFSET_S5100_A2];
			rmi4_data->fw_version_of_bin = (int)fw_entry->data[FW_VERSION_BIN_OFFSET_S5100_A2];
		}
#else
		rmi4_data->ic_revision_of_bin = (int)fw_entry->data[IC_REVISION_BIN_OFFSET_S5050];
		rmi4_data->fw_version_of_bin = (int)fw_entry->data[FW_VERSION_BIN_OFFSET_S5050];
#endif
	} else if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5006) {
		rmi4_data->ic_revision_of_bin = (int)fw_entry->data[IC_REVISION_BIN_OFFSET];
		rmi4_data->fw_version_of_bin = (int)fw_entry->data[FW_VERSION_BIN_OFFSET];		
	} else if (rmi4_data->ic_version >= SYNAPTICS_PRODUCT_ID_S5050) {
		rmi4_data->ic_revision_of_bin = (int)fw_entry->data[IC_REVISION_BIN_OFFSET_S5050];
		rmi4_data->fw_version_of_bin = (int)fw_entry->data[FW_VERSION_BIN_OFFSET_S5050];
	} else {
		rmi4_data->ic_revision_of_bin = (int)fw_entry->data[IC_REVISION_BIN_OFFSET];
		rmi4_data->fw_version_of_bin = (int)fw_entry->data[FW_VERSION_BIN_OFFSET];
	}

	dev_info(&rmi4_data->i2c_client->dev,
			"%s: [FW size, version] [%d, 0x%02X%02X/0x%02X%02X(BIN/IC), prog bit[%x]\n",
			__func__, fw_entry->size, rmi4_data->ic_revision_of_bin,
			rmi4_data->fw_version_of_bin, rmi4_data->ic_revision_of_ic,
			rmi4_data->fw_version_of_ic,rmi4_data->flash_prog_mode);

	if (rmi4_data->flash_prog_mode) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Force firmware update : Flash prog bit is setted fw\n",
				__func__);
		return false;
	}

#ifdef READ_LCD_ID
	if ((rmi4_data->lcd_id == 0x03) && \
		(rmi4_data->fw_version_of_bin != rmi4_data->fw_version_of_ic)) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: LCD id is %x, fw version [%x, %x][BIN,IC]\n",
				__func__, rmi4_data->lcd_id,
				rmi4_data->fw_version_of_bin, rmi4_data->fw_version_of_ic);
		return false;
	}
#endif

#ifdef CHECK_BASE_FIRMWARE
	/* check base fw version. base fw version is PR number. ex)PR1566790_...img */
	if (rmi4_data->fw_version_of_bin == rmi4_data->fw_version_of_ic) {
		if ((strncmp(&rmi4_data->bootloader_id[2], SYNAPTICS_IC_NEW_BOOTLOADER, 2) == 0) &&
			((int)(fw_entry->data[0x07] >= 0x10))){
			rmi4_data->fw_pr_number = ((int)(fw_entry->data[0x77] & 0xFF) << 24) | \
							((int)(fw_entry->data[0x76] & 0xFF) << 16) | \
							((int)(fw_entry->data[0x75] & 0xFF) << 8) | \
							(int)(fw_entry->data[0x74] & 0xFF);

			if (rmi4_data->fw_pr_number != rmi4_data->rmi4_mod_info.pr_number) {
				dev_info(&rmi4_data->i2c_client->dev,
					"%s: 06:%X, IC/FW pr_number : %d / %d, run update\n",
					__func__, (int)fw_entry->data[0x06],
					rmi4_data->rmi4_mod_info.pr_number, rmi4_data->fw_pr_number);

				return false;
			}
		} else {
			dev_info(&rmi4_data->i2c_client->dev,
				"%s: IC do not have new bootloader / or flag is smaller than 0x10\n", __func__);

		}
	} else {
		dev_info(&rmi4_data->i2c_client->dev,
			"%s: firmware version is not equal. not check PR number. check config version.\n",
				__func__);
	}
#endif

	/* temp chagall sdc firmware */
	if ((rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5710) && (rmi4_data->fw_version_of_ic == 0x34)) {
		dev_info(&rmi4_data->i2c_client->dev,
			"%s: chagall's sdc firmware is 0x34, it should be updated 0x24 firmware\n",
				__func__);
		return false;
	}
	
	if (rmi4_data->fw_version_of_bin <= rmi4_data->fw_version_of_ic) {
		dev_info(&rmi4_data->i2c_client->dev,
				"%s: Do not need to update\n",
				__func__);
		return true;
	}

	return false;
}

int synaptics_rmi4_fw_update_on_probe(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	const struct firmware *fw_entry = NULL;
	unsigned char *fw_data = NULL;
	char fw_path[SYNAPTICS_MAX_FW_PATH];

	snprintf(fw_path, SYNAPTICS_MAX_FW_PATH,
			"%s", rmi4_data->firmware_name);

	if (rmi4_data->firmware_name == NULL) {
		dev_info(&rmi4_data->i2c_client->dev, "%s: firmware name is NULL!, return\n",
				__func__);
		return 0;
	} else {
		dev_info(&rmi4_data->i2c_client->dev, "%s: Load firmware : %s\n",
			__func__, fw_path);
	}

	retval = request_firmware(&fw_entry, fw_path, &rmi4_data->i2c_client->dev);
	if (retval) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: Firmware image %s not available\n",
				__func__, fw_path);
		goto done;
	}

	if (synaptics_skip_firmware_update(rmi4_data, fw_entry))
		goto done;

	fw_data = (unsigned char *)fw_entry->data;

	retval = synaptics_fw_updater(fw_data);
	if (retval)
		dev_err(&rmi4_data->i2c_client->dev, "%s: failed update firmware\n",
				__func__);
done:
	if (fw_entry)
		release_firmware(fw_entry);

	return retval;
}
EXPORT_SYMBOL(synaptics_rmi4_fw_update_on_probe);

static int synaptics_load_fw_from_kernel(struct synaptics_rmi4_data *rmi4_data, const char *fw_path)
{
	int retval;
	const struct firmware *fw_entry = NULL;
	unsigned char *fw_data = NULL;

	if (!fw_path) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: Firmware name is not defined\n",
				__func__);
		return -EINVAL;
	}

	dev_info(&rmi4_data->i2c_client->dev, "%s: Load firmware : %s\n",
			__func__, fw_path);

	retval = request_firmware(&fw_entry, fw_path,
			&rmi4_data->i2c_client->dev);

	if (retval) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: Firmware image %s not available\n",
				__func__, fw_path);
		goto done;
	}

	if ((rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5100) &&
		(rmi4_data->ic_revision_of_ic >= SYNAPTICS_IC_REVISION_A2)) {
#if !defined(CONFIG_SEC_HESTIA_PROJECT)
		if (strncmp(rmi4_data->dt_data->project, "PSLTE", 5) == 0) {
			rmi4_data->ic_revision_of_bin = (int)fw_entry->data[IC_REVISION_BIN_OFFSET_S5100_PS];
			rmi4_data->fw_version_of_bin = (int)fw_entry->data[FW_VERSION_BIN_OFFSET_S5100_PS];
		} else {
			rmi4_data->ic_revision_of_bin = (int)fw_entry->data[IC_REVISION_BIN_OFFSET_S5100_A2];
			rmi4_data->fw_version_of_bin = (int)fw_entry->data[FW_VERSION_BIN_OFFSET_S5100_A2];
		}
#else
		rmi4_data->ic_revision_of_bin = (int)fw_entry->data[IC_REVISION_BIN_OFFSET_S5050];
		rmi4_data->fw_version_of_bin = (int)fw_entry->data[FW_VERSION_BIN_OFFSET_S5050];
#endif
	} else if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5006) {
		rmi4_data->ic_revision_of_bin = (int)fw_entry->data[IC_REVISION_BIN_OFFSET];
		rmi4_data->fw_version_of_bin = (int)fw_entry->data[FW_VERSION_BIN_OFFSET];
	} else if (rmi4_data->ic_version >= SYNAPTICS_PRODUCT_ID_S5050) {
		rmi4_data->ic_revision_of_bin = (int)fw_entry->data[IC_REVISION_BIN_OFFSET_S5050];
		rmi4_data->fw_version_of_bin = (int)fw_entry->data[FW_VERSION_BIN_OFFSET_S5050];
	} else {
		rmi4_data->ic_revision_of_bin = (int)fw_entry->data[IC_REVISION_BIN_OFFSET];
		rmi4_data->fw_version_of_bin = (int)fw_entry->data[FW_VERSION_BIN_OFFSET];
	}
	dev_info(&rmi4_data->i2c_client->dev,
			"%s: [FW size, version] [%d, 0x%02X/0x%02X(BIN/IC), prog bit[%x]\n",
			__func__, fw_entry->size, rmi4_data->fw_version_of_bin,
			rmi4_data->fw_version_of_ic, rmi4_data->flash_prog_mode);

	fw_data = (unsigned char *)fw_entry->data;

	retval = synaptics_fw_updater(fw_data);
	if (retval)
		dev_err(&rmi4_data->i2c_client->dev, "%s: failed update firmware\n",
				__func__);
done:
	if (fw_entry)
		release_firmware(fw_entry);

	return retval;
}

static int synaptics_load_fw_from_ums(struct synaptics_rmi4_data *rmi4_data)
{
	struct file *fp;
	mm_segment_t old_fs;
	int fw_size, nread;
	int error = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(SYNAPTICS_DEFAULT_UMS_FW, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: failed to open %s.\n", __func__, SYNAPTICS_DEFAULT_UMS_FW);
		error = -ENOENT;
		goto open_err;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;

	if (0 < fw_size) {
		unsigned char *fw_data;
		fw_data = kzalloc(fw_size, GFP_KERNEL);
		nread = vfs_read(fp, (char __user *)fw_data,
				fw_size, &fp->f_pos);

		dev_info(&rmi4_data->i2c_client->dev,
				"%s: start, file path %s, size %u Bytes\n", __func__,
				SYNAPTICS_DEFAULT_UMS_FW, fw_size);

		if (nread != fw_size) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: failed to read firmware file, nread %u Bytes\n",
					__func__,
					nread);
			error = -EIO;
		} else {
			/* UMS case */
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			int ic_revision_of_bin;
			int fw_version_of_bin;
			int fw_release_date_of_bin;

			if ((rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5100) &&
				(rmi4_data->ic_revision_of_ic >= SYNAPTICS_IC_REVISION_A2)) {
#if !defined(CONFIG_SEC_HESTIA_PROJECT)
				if (strncmp(rmi4_data->dt_data->project, "PSLTE", 5) == 0) {
					ic_revision_of_bin = (int)fw_data[IC_REVISION_BIN_OFFSET_S5100_PS];
					fw_version_of_bin = (int)fw_data[FW_VERSION_BIN_OFFSET_S5100_PS];
					fw_release_date_of_bin =
						(int)(fw_data[DATE_OF_FIRMWARE_BIN_OFFSET_S5100_PS] << 8
								| fw_data[DATE_OF_FIRMWARE_BIN_OFFSET_S5100_PS + 1]);
				} else {
					ic_revision_of_bin = (int)fw_data[IC_REVISION_BIN_OFFSET_S5100_A2];
					fw_version_of_bin = (int)fw_data[FW_VERSION_BIN_OFFSET_S5100_A2];
					fw_release_date_of_bin =
						(int)(fw_data[DATE_OF_FIRMWARE_BIN_OFFSET_S5100_A2] << 8
								| fw_data[DATE_OF_FIRMWARE_BIN_OFFSET_S5100_A2 + 1]);
				}
#else
				ic_revision_of_bin = (int)fw_data[IC_REVISION_BIN_OFFSET_S5050];
				fw_version_of_bin = (int)fw_data[FW_VERSION_BIN_OFFSET_S5050];
				fw_release_date_of_bin =
					(int)(fw_data[DATE_OF_FIRMWARE_BIN_OFFSET_S5050] << 8
							| fw_data[DATE_OF_FIRMWARE_BIN_OFFSET_S5050 + 1]);
#endif
			} else if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5006) {
				ic_revision_of_bin = (int)fw_data[IC_REVISION_BIN_OFFSET];
				fw_version_of_bin = (int)fw_data[FW_VERSION_BIN_OFFSET];
				fw_release_date_of_bin =
					(int)(fw_data[DATE_OF_FIRMWARE_BIN_OFFSET] << 8
							| fw_data[DATE_OF_FIRMWARE_BIN_OFFSET + 1]);
			} else if (rmi4_data->ic_version >= SYNAPTICS_PRODUCT_ID_S5050) {
				ic_revision_of_bin = (int)fw_data[IC_REVISION_BIN_OFFSET_S5050];
				fw_version_of_bin = (int)fw_data[FW_VERSION_BIN_OFFSET_S5050];
				fw_release_date_of_bin =
					(int)(fw_data[DATE_OF_FIRMWARE_BIN_OFFSET_S5050] << 8
							| fw_data[DATE_OF_FIRMWARE_BIN_OFFSET_S5050 + 1]);
			} else {
				ic_revision_of_bin = (int)fw_data[IC_REVISION_BIN_OFFSET];
				fw_version_of_bin = (int)fw_data[FW_VERSION_BIN_OFFSET];
				fw_release_date_of_bin =
					(int)(fw_data[DATE_OF_FIRMWARE_BIN_OFFSET] << 8
							| fw_data[DATE_OF_FIRMWARE_BIN_OFFSET + 1]);
			}

			/* Test firmware file does not have version infomation */
			if (!ic_revision_of_bin && !fw_version_of_bin
					&& !fw_release_date_of_bin) {
				dev_info(&rmi4_data->i2c_client->dev, "%s [UMS] : Firmware is Test firmware\n",
						__func__);
			}
#endif
			error = synaptics_fw_updater(fw_data);
		}

		if (error < 0)
			dev_err(&rmi4_data->i2c_client->dev, "%s: failed update firmware\n",
					__func__);

		kfree(fw_data);
	}

	filp_close(fp, current->files);

open_err:
	set_fs(old_fs);
	return error;
}

static int synaptics_rmi4_fw_update_on_hidden_menu(struct synaptics_rmi4_data *rmi4_data,
		int update_type)
{
	int retval = 0;

	/* Factory cmd for firmware update
	 * argument represent what is source of firmware like below.
	 *
	 * 0, 2 : Getting firmware which is for user.
	 * 1 : Getting firmware from sd card.
	 */
	switch (update_type) {
		case 2:
		case 0:
			retval = synaptics_load_fw_from_kernel(rmi4_data, rmi4_data->firmware_name);
			break;
		case 1:
			retval = synaptics_load_fw_from_ums(rmi4_data);
			break;
		default:
			dev_err(&rmi4_data->i2c_client->dev, "%s: Not support command[%d]\n",
					__func__, update_type);
			break;
	}

	return retval;
}

static void fw_update(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	int retval = 0;

	set_default_result(data);

	retval = synaptics_rmi4_fw_update_on_hidden_menu(rmi4_data,
			data->cmd_param[0]);
	msleep(1000);

	if (retval < 0) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NA));
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_FAIL;
		dev_err(&rmi4_data->i2c_client->dev, "%s: failed [%d]\n",
				__func__, retval);
	} else {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(OK));
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_OK;
		dev_info(&rmi4_data->i2c_client->dev, "%s: success [%d]\n",
				__func__, retval);
	}

	return;
}

static void get_fw_ver_bin(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "SY%02X%02X%02X",
			rmi4_data->ic_revision_of_bin,
			rmi4_data->lcd_id,
			rmi4_data->fw_version_of_bin);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = CMD_STATUS_OK;

	return;
}

static void get_fw_ver_ic(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "SY%02X%02X%02X",
			rmi4_data->ic_revision_of_ic,
			rmi4_data->lcd_id,
			rmi4_data->fw_version_of_ic);

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = CMD_STATUS_OK;

	return;
}

static void get_fac_fw_ver_bin(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	int retval;
	const struct firmware *fw_entry = NULL;

	set_default_result(data);

	if (rmi4_data->firmware_name == NULL)
		retval = -EINVAL;
	else
		retval = request_firmware(&fw_entry, rmi4_data->firmware_name,
					&rmi4_data->i2c_client->dev);

	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: factory firmware request failed\n",
				__func__);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

		data->cmd_state = CMD_STATUS_FAIL;

	} else {
		if ((rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5100) &&
			(rmi4_data->ic_revision_of_ic >= SYNAPTICS_IC_REVISION_A2))
#if !defined(CONFIG_SEC_HESTIA_PROJECT)
			if (strncmp(rmi4_data->dt_data->project, "PSLTE", 5) == 0) {
				snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "SY00%02X%02X",
						(int)fw_entry->data[IC_REVISION_BIN_OFFSET_S5100_PS],
						(int)fw_entry->data[FW_VERSION_BIN_OFFSET_S5100_PS]);
			} else {
				snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "SY00%02X%02X",
						(int)fw_entry->data[IC_REVISION_BIN_OFFSET_S5100_A2],
						(int)fw_entry->data[FW_VERSION_BIN_OFFSET_S5100_A2]);
			}
#else
			snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "SY00%02X%02X",
					(int)fw_entry->data[IC_REVISION_BIN_OFFSET_S5050],
					(int)fw_entry->data[FW_VERSION_BIN_OFFSET_S5050]);
#endif
		else if (rmi4_data->ic_version >= SYNAPTICS_PRODUCT_ID_S5006)
			snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "SY00%02X%02X",
					(int)fw_entry->data[IC_REVISION_BIN_OFFSET],
					(int)fw_entry->data[FW_VERSION_BIN_OFFSET]);
		else if (rmi4_data->ic_version >= SYNAPTICS_PRODUCT_ID_S5050)
			snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "SY00%02X%02X",
					(int)fw_entry->data[IC_REVISION_BIN_OFFSET_S5050],
					(int)fw_entry->data[FW_VERSION_BIN_OFFSET_S5050]);
		else
			snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "SY00%02X%02X",
					(int)fw_entry->data[IC_REVISION_BIN_OFFSET],
					(int)fw_entry->data[FW_VERSION_BIN_OFFSET]);

		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

		data->cmd_state = CMD_STATUS_OK;
	}

	release_firmware(fw_entry);

	return;
}

static void get_config_ver(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s_SY_%02d%02d",
			SYNAPTICS_DEVICE_NAME, (rmi4_data->fw_release_date_of_ic >> 8) & 0xF,
			rmi4_data->fw_release_date_of_ic & 0x00FF);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = CMD_STATUS_OK;

	return;
}

static void get_threshold(void)
{
	unsigned char SaturationCap_LSB;
	unsigned char SaturationCap_MSB;
	unsigned char FingerAmplitudeThreshold;
	unsigned int SaturationCap;
	unsigned int threshold_integer;
	unsigned int threshold_fraction;
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	f54->fn_ptr->read(rmi4_data,
			rmi4_data->f12_ctrl15_addr,
			&FingerAmplitudeThreshold,
			sizeof(FingerAmplitudeThreshold));
	f54->fn_ptr->read(rmi4_data,
			f54->control.reg_2->address,
			&SaturationCap_LSB,
			sizeof(SaturationCap_LSB));
	f54->fn_ptr->read(rmi4_data,
			f54->control.reg_2->address + 1,
			&SaturationCap_MSB,
			sizeof(SaturationCap_MSB));

	SaturationCap = (SaturationCap_LSB & 0xFF) | ((SaturationCap_MSB & 0xFF) <<8);
	threshold_integer = (FingerAmplitudeThreshold * SaturationCap)/256;
	threshold_fraction = ((FingerAmplitudeThreshold * SaturationCap * 1000)/256)%1000;

	dev_info(&rmi4_data->i2c_client->dev,
		"%s: FingerAmp : %d, Satruration cap : %d\n",
		__func__, FingerAmplitudeThreshold, SaturationCap);

	set_default_result(data);
	sprintf(data->cmd_buff, "%u.%u",
		threshold_integer, threshold_fraction);

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
	data->cmd_state = CMD_STATUS_OK;
	return;
}

static void module_off_master(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);

	mutex_lock(&rmi4_data->input_dev->mutex);

	rmi4_data->stop_device(rmi4_data);

	mutex_unlock(&rmi4_data->input_dev->mutex);

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(OK));
	data->cmd_state = CMD_STATUS_OK;

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void module_on_master(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	int retval;

	set_default_result(data);

	mutex_lock(&rmi4_data->input_dev->mutex);

	retval = rmi4_data->start_device(rmi4_data);
	if (retval < 0)
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to start device\n", __func__);

	mutex_unlock(&rmi4_data->input_dev->mutex);

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(OK));
	data->cmd_state = CMD_STATUS_OK;

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void get_module_vendor(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	int val;

	set_default_result(data);

	if (rmi4_data->touch_stopped) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP turned off");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (rmi4_data->dt_data->id_gpio > 0) {
		gpio_tlmm_config(GPIO_CFG(rmi4_data->dt_data->id_gpio, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
		val = gpio_get_value(rmi4_data->dt_data->id_gpio);
		dev_info(&rmi4_data->i2c_client->dev,
			"%s: TSP_ID: %d[%d]\n", __func__, rmi4_data->dt_data->id_gpio, val);

		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s,%d", tostring(OK), val);
		data->cmd_state = CMD_STATUS_OK;
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

		return;
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NG));
	data->cmd_state = CMD_STATUS_FAIL;
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void get_chip_vendor(void)
{
	struct factory_data *data = f54->factory_data;

	set_default_result(data);

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(SYNAPTICS));
	data->cmd_state = CMD_STATUS_OK;
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void get_chip_name(void)
{
	struct factory_data *data = f54->factory_data;

	set_default_result(data);

	if (f54->rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5050)
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(S5050));
	else if (f54->rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5000)
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(S5000));
	else if (f54->rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5050)
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(S5050));
	else if (f54->rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5100)
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(S5100));
	else if (f54->rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5700)
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(S5700));
	else if (f54->rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5707)
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(S5707));
	else if (f54->rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5708)
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(S5708));
	else if (f54->rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5006)
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(S5006));
	else
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NA));

	data->cmd_state = CMD_STATUS_OK;
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
}

static void get_x_num(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%d", rmi4_data->num_of_tx);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = CMD_STATUS_OK;

	return;
}

static void get_y_num(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%d", rmi4_data->num_of_rx);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = CMD_STATUS_OK;

	return;
}

static int check_rx_tx_num(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	unsigned char num_of_tx, num_of_rx;

	int node;

	dev_info(&rmi4_data->i2c_client->dev, "%s: param[0] = %d, param[1] = %d\n",
			__func__, data->cmd_param[0], data->cmd_param[1]);

#ifdef TOUCHKEY_ENABLE
	num_of_tx = f54->tx_assigned;
	num_of_rx = f54->rx_assigned;
#else
	num_of_tx = rmi4_data->num_of_tx;
	num_of_rx = rmi4_data->num_of_rx;
#endif

	if (data->cmd_param[0] < 0 ||
			data->cmd_param[0] >= num_of_tx ||
			data->cmd_param[1] < 0 ||
			data->cmd_param[1] >= num_of_rx) {

		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NA));
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_FAIL;

		dev_info(&rmi4_data->i2c_client->dev, "%s: parameter error: %u,%u\n",
				__func__, data->cmd_param[0], data->cmd_param[1]);
		node = -1;
	} else {
		node = data->cmd_param[0] * num_of_rx +
			data->cmd_param[1];
		dev_info(&rmi4_data->i2c_client->dev, "%s: node = %d\n",
				__func__, node);
	}
	return node;
}

static void get_rawcap(void)
{
	int node;
	short report_data;
	struct factory_data *data = f54->factory_data;

	set_default_result(data);

	node = check_rx_tx_num();

	if (node < 0) {
		data->cmd_state = CMD_STATUS_FAIL;
		return;
	} else {
		report_data = f54->factory_data->rawcap_data[node];

		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%d", report_data);
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_OK;
	}
	return;
}

static void run_rawcap_read(void)
{
	int retval;
	int kk = 0;
	unsigned char ii;
	unsigned char jj;
	unsigned char num_of_tx;
	unsigned char num_of_rx;
	short *report_data;
	short max_value;
	short min_value;
	short cur_value;
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	unsigned char command = 0x01;
	int retry = 2;
	unsigned char cmd_state = CMD_STATUS_RUNNING;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP enter suspend");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	while (retry--) {
		retval = do_preparation();
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to do preparation, do soft reset, and retry\n",
				__func__);
			/* soft reset */
			retval = f54->fn_ptr->write(rmi4_data,
					rmi4_data->f01_cmd_base_addr,
					&command,
					sizeof(command));
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to issue reset1 command 1, error = %d\n",
					__func__, retval);
			}
			msleep(100);
		} else {
			break;
		}
	}
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to do preparation\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "Error preparation");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (!synaptics_rmi4_f54_get_report_type(CMD_REPORT_TYPE_RAWCAP)) {
		cmd_state = CMD_STATUS_FAIL;
		goto exit;
	}

	report_data = f54->factory_data->rawcap_data;
	memcpy(report_data, f54->report_data, f54->report_size);

	num_of_tx = rmi4_data->num_of_tx;
	num_of_rx = rmi4_data->num_of_rx;
	max_value = min_value = report_data[0];

	for (ii = 0; ii < num_of_tx; ii++) {
#ifdef TOUCHKEY_ENABLE
		for (jj = 0; jj < num_of_rx + 1; jj++) {
			if (jj == num_of_rx) {
				report_data++;
				continue;
			}
#else
		for (jj = 0; jj < num_of_rx; jj++) {
#endif
			cur_value = *report_data;
			max_value = max(max_value, cur_value);
			min_value = min(min_value, cur_value);
			report_data++;

			dev_info(&rmi4_data->i2c_client->dev,
					"tx = %02d, rx = %02d, data[%d] = %d\n",
					ii, jj, kk, cur_value);
			kk++;
		}
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%d,%d", min_value, max_value);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	cmd_state = CMD_STATUS_OK;

exit:
	retval = rmi4_data->reset_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
	}

	data->cmd_state = cmd_state;

	return;
}

static void run_no_interrupt_test(void)
{
	int retval;
	int kk = 0;
	unsigned char ii;
	unsigned char jj;
	unsigned char num_of_tx;
	unsigned char num_of_rx;
	short *report_data;
	short max_value;
	short min_value;
	short cur_value;
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	unsigned char cmd_state = CMD_STATUS_RUNNING;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s TSP enter suspend", __func__);
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (!synaptics_rmi4_f54_get_report_type(F54_RAW_16BIT_IMAGE)) {
		cmd_state = CMD_STATUS_FAIL;
		goto exit;
	}

	report_data = f54->factory_data->test_data;
	memcpy(report_data, f54->report_data, f54->report_size);

	num_of_tx = rmi4_data->num_of_tx;
	num_of_rx = rmi4_data->num_of_rx;
	max_value = min_value = report_data[0];

	for (ii = 0; ii < num_of_tx; ii++) {
		for (jj = 0; jj < num_of_rx; jj++) {
			cur_value = *report_data;
			max_value = max(max_value, cur_value);
			min_value = min(min_value, cur_value);
			report_data++;

			dev_info(&rmi4_data->i2c_client->dev,
					"tx = %02d, rx = %02d, data[%d] = %d\n",
					ii, jj, kk, cur_value);
			kk++;
		}
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%d,%d", min_value, max_value);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	cmd_state = CMD_STATUS_OK;

exit:
	retval = rmi4_data->reset_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
	}

	data->cmd_state = cmd_state;

	return;
}

static void get_delta(void)
{
	int node;
	short report_data;
	struct factory_data *data = f54->factory_data;

	set_default_result(data);

	node = check_rx_tx_num();

	if (node < 0) {
		data->cmd_state = CMD_STATUS_FAIL;
		return;
	} else {
		report_data = f54->factory_data->delta_data[node];

		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%d", report_data);
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_OK;
	}
}

static void run_delta_read(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	short *report_data;
	short cur_value;
	unsigned char ii;
	unsigned char jj;
	unsigned char num_of_tx;
	unsigned char num_of_rx;
	int kk = 0;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP enter suspend");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (!synaptics_rmi4_f54_get_report_type(CMD_REPORT_TYPE_DELTA)) {
		data->cmd_state = CMD_STATUS_FAIL;
		return;
	}

	report_data = f54->factory_data->delta_data;
	memcpy(report_data, f54->report_data, f54->report_size);

	num_of_tx = rmi4_data->num_of_tx;
	num_of_rx = rmi4_data->num_of_rx;

	for (ii = 0; ii < num_of_tx; ii++) {
		for (jj = 0; jj < num_of_rx; jj++) {
			cur_value = *report_data;
			report_data++;
			dev_info(&rmi4_data->i2c_client->dev,
					"tx = %02d, rx = %02d, data[%d] = %d\n",
					ii, jj, kk, cur_value);
			kk++;
		}
	}

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = CMD_STATUS_OK;

	return;

}

static void run_abscap_read(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	unsigned int *report_data;
	char temp[CMD_STR_LEN];
	char temp2[CMD_RESULT_STR_LEN];
	unsigned char ii;
	unsigned short num_of_tx;
	unsigned short num_of_rx;
	int retval;
	unsigned char cmd_state = CMD_STATUS_RUNNING;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP enter suspend");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	retval = synaptics_rmi4_f54_get_report_type(F54_ABS_CAP);
	if (!retval) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s: Failed to get report type\n",
			__func__);
		cmd_state = CMD_STATUS_FAIL;
		goto exit;
	}

	report_data = f54->factory_data->abscap_data;
	memcpy(report_data, f54->report_data, f54->report_size);
	memset(temp, 0, CMD_STR_LEN);
	memset(temp2, 0, CMD_RESULT_STR_LEN);

	num_of_tx = rmi4_data->num_of_tx;
	num_of_rx = rmi4_data->num_of_rx;

	for (ii = 0; ii < num_of_rx + num_of_tx; ii++) {
		if (f54->rmi4_data->ic_version != SYNAPTICS_PRODUCT_ID_S5100)
			*report_data &= 0x0FFFF;

		dev_info(&rmi4_data->i2c_client->dev,
				"%s: %s [%d] = %d\n", __func__,
				ii >= num_of_rx ? "Tx" : "Rx",
				ii < num_of_rx ? ii : ii - num_of_rx,
				*report_data);
		snprintf(temp, CMD_RESULT_STR_LEN, "%d,", *report_data);
		strncat(temp2, temp, 9);
		report_data ++;
	}
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", temp2);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	cmd_state = CMD_STATUS_OK;

exit:
	retval = rmi4_data->reset_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
	}

	data->cmd_state = cmd_state;

	return;
}

static void run_absdelta_read(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	int *report_data;
	char temp[CMD_STR_LEN];
	char temp2[CMD_RESULT_STR_LEN];
	unsigned char ii;
	unsigned short num_of_tx;
	unsigned short num_of_rx;
	int retval;
	unsigned char cmd_state = CMD_STATUS_RUNNING;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP enter suspend");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

#ifdef PROXIMITY
	retval = synaptics_rmi4_proximity_enables(rmi4_data, 1);
	if (retval < 0)
		dev_err(&rmi4_data->i2c_client->dev,
			"%s: proximity_enable failed, retval = %d\n",
			__func__, retval);
	msleep(150);
#endif

	if (!synaptics_rmi4_f54_get_report_type(F54_ABS_DELTA)) {
		cmd_state = CMD_STATUS_FAIL;
		goto exit;
	}

	report_data = f54->factory_data->absdelta_data;
	memcpy(report_data, f54->report_data, f54->report_size);
	memset(temp, 0, CMD_STR_LEN);
	memset(temp2, 0, CMD_RESULT_STR_LEN);

	num_of_tx = rmi4_data->num_of_tx;
	num_of_rx = rmi4_data->num_of_rx;

	for (ii = 0; ii < num_of_rx + num_of_tx; ii++) {
		dev_info(&rmi4_data->i2c_client->dev,
				"%s: %s [%d] = %d\n", __func__,
				ii >= num_of_rx ? "Tx" : "Rx",
				ii < num_of_rx ? ii : ii - num_of_rx,
				*report_data);
		snprintf(temp, CMD_RESULT_STR_LEN, "%d,", *report_data);
		strncat(temp2, temp, 9);
		report_data++;
	}
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", temp2);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	cmd_state = CMD_STATUS_OK;

exit:
	retval = rmi4_data->reset_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
	}

	data->cmd_state = cmd_state;

	return;
}

/* trx_short_test register mapping
 * 0 : not used ( using 5.2 inch)
 * 1 ~ 28 : Rx
 * 29 ~ 31 : Side Button 0, 1, 2
 * 32 ~ 33 : Guard
 * 34 : Charge Substraction
 * 35 ~ 50 : Tx
 * 51 ~ 53 : Side Button 3, 4, 5
 */

static void run_trx_short_test(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	char *report_data;
	unsigned char ii, jj;
	int retval = 0;
	char temp[CMD_STR_LEN];
	char temp2[CMD_RESULT_STR_LEN];
	unsigned char cmd_state = CMD_STATUS_RUNNING;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP enter suspend");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	disable_irq(rmi4_data->i2c_client->irq);
	if (!synaptics_rmi4_f54_get_report_type(F54_TREX_SHORTS)) {
		cmd_state = CMD_STATUS_FAIL;
		goto exit;
	}

	report_data = f54->factory_data->trx_short;
	memcpy(report_data, f54->report_data, f54->report_size);
	memset(temp, 0, CMD_STR_LEN);
	memset(temp2, 0, CMD_RESULT_STR_LEN);

	for (ii = 0; ii < f54->report_size; ii++) {
		dev_info(&rmi4_data->i2c_client->dev,
				"%s: [%d]: [%x][%x][%x][%x][%x][%x][%x][%x]\n",
				__func__, ii, *report_data & 0x1, (*report_data & 0x2) >> 1,
				(*report_data & 0x4) >> 2, (*report_data & 0x8) >> 3,
				(*report_data & 0x10) >> 4, (*report_data & 0x20) >> 5,
				(*report_data & 0x40) >> 6, (*report_data & 0x80) >> 7);
		for (jj = 0; jj < 8; jj++) {
			snprintf(temp, CMD_RESULT_STR_LEN, "%d,", (*report_data >> jj) & 0x01);
			strncat(temp2, temp, 9);

		}
		report_data++;
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", temp2);

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	cmd_state = CMD_STATUS_OK;

exit:
	enable_irq(rmi4_data->i2c_client->irq);

	retval = rmi4_data->reset_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
	}

	data->cmd_state = cmd_state;

	return;
}

#ifdef PROXIMITY
static void hover_enable(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		int retval, enables;

		retval = 0;
		enables = data->cmd_param[0];
		rmi4_data->hover_status_in_normal_mode = data->cmd_param[0];

		if ((rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5100) &&
			(rmi4_data->ic_revision_of_ic < SYNAPTICS_IC_REVISION_A2))
			retval = -1;
		else
			retval = synaptics_rmi4_proximity_enables(rmi4_data, enables);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s failed, retval = %d\n",
					__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;
		} else {
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
			data->cmd_state = CMD_STATUS_OK;
		}
	}

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;

	return;
}

static void hover_no_sleep_enable(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		int retval = 0;

		if (data->cmd_param[0])
			retval = synaptics_proximity_no_sleep_set(rmi4_data, true);
		else
			retval = synaptics_proximity_no_sleep_set(rmi4_data, false);

		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s failed, retval = %d\n",
					__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;
		} else {
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
			data->cmd_state = CMD_STATUS_OK;
		}

	}

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	return;
}

#ifdef USE_EDGE_EXCLUSION
static void hover_set_edge_rx(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s failed, command is only 0 or 1.\n",
				__func__);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		int retval;

		retval = synaptics_rmi4_f51_grip_edge_exclusion_rx(rmi4_data, data->cmd_param[0]);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
				"%s failed, retval = %d\n",
				__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;
		} else {
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
			data->cmd_state = CMD_STATUS_OK;
		}
	}

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	return;
}
#endif
#endif

static void set_jitter_level(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 255) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s failed, the range of jitter level is 0~255\n",
				__func__);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		int retval, level;

		level = data->cmd_param[0];

		retval = synaptics_rmi4_f12_ctrl11_set(rmi4_data, level);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
				"%s failed, retval = %d\n",
				__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;
		} else {
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
			data->cmd_state = CMD_STATUS_OK;
		}
	}

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;

	return;
}

#ifdef HAND_GRIP_MODE
static void handgrip_enable(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		int retval, enables;

		retval = 0;
		rmi4_data->hand_grip_mode = enables = data->cmd_param[0];
/*
		rmi4_data->hover_status_in_normal_mode = data->cmd_param[0];
*/
#ifdef PROXIMITY
		retval = synaptics_rmi4_proximity_enables(rmi4_data, enables);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s failed, retval = %d\n",
					__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;
		} else {
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
			data->cmd_state = CMD_STATUS_OK;
		}
#endif
	}

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;

	return;
}
#endif

#ifdef GLOVE_MODE
static void glove_mode(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);

	if (rmi4_data->feature_enable & CLOSED_COVER_EN) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
		data->cmd_state = CMD_STATUS_OK;
		dev_info(&rmi4_data->i2c_client->dev,
				"%s Skip glove mode set (cover bit enabled)\n",
				__func__);
		goto skip_glove_mode_set;
	}

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		int retval;

		if (data->cmd_param[0]){
			rmi4_data->feature_enable |= GLOVE_MODE_EN;
#ifdef ENABLE_F12_OBJTYPE
			rmi4_data->obj_type_enable |= OBJ_TYPE_GLOVE;
#endif
		} else {
			rmi4_data->feature_enable &= ~(GLOVE_MODE_EN);
#ifdef ENABLE_F12_OBJTYPE
			rmi4_data->obj_type_enable &= ~(OBJ_TYPE_GLOVE);
#endif
		}
		retval = synaptics_rmi4_glove_mode_enables(rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s failed, retval = %d\n",
					__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;
		} else {
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
			data->cmd_state = CMD_STATUS_OK;
		}
	}

skip_glove_mode_set:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;

	return;
}

static void fast_glove_mode(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		int retval;

		if (data->cmd_param[0]) {
			rmi4_data->feature_enable |= FAST_DETECT_EN | GLOVE_MODE_EN;
#ifdef ENABLE_F12_OBJTYPE
			rmi4_data->obj_type_enable |= OBJ_TYPE_GLOVE;
#endif
			rmi4_data->fast_glove_state = true;
		} else {
			rmi4_data->feature_enable &= ~(FAST_DETECT_EN);
#ifdef ENABLE_F12_OBJTYPE
			rmi4_data->obj_type_enable &= ~(OBJ_TYPE_GLOVE);
#endif
			rmi4_data->fast_glove_state = false;
		}

		retval = synaptics_rmi4_glove_mode_enables(rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s failed, retval = %d\n",
					__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;
		} else {
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
			data->cmd_state = CMD_STATUS_OK;
		}
	}

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;

	return;
}

static void clear_cover_mode(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 3) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		int retval;

		rmi4_data->feature_enable = data->cmd_param[0];

		if (data->cmd_param[0] && rmi4_data->fast_glove_state)
			rmi4_data->feature_enable |= FAST_DETECT_EN;

		retval = synaptics_rmi4_f12_set_feature(rmi4_data);

		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s failed, retval = %d\n",
					__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;
		} else {
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
			data->cmd_state = CMD_STATUS_OK;
		}

		/* Sync user setting value when wakeup with flip cover opened */
		if ((0x02 == rmi4_data->feature_enable) ||
				(0x06 == rmi4_data->feature_enable)) {
			rmi4_data->feature_enable &= ~(CLOSED_COVER_EN);

			if (rmi4_data->fast_glove_state)
				rmi4_data->feature_enable |= GLOVE_MODE_EN;
		}

	}
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;

	return;
}

static void get_glove_sensitivity(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);

	dev_info(&rmi4_data->i2c_client->dev,
			"%s : %x\n", __func__, rmi4_data->gloved_sensitivity & 0x0F);

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%x", rmi4_data->gloved_sensitivity & 0x0F);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = CMD_STATUS_OK;
	return;
}
#endif

#ifdef TSP_BOOSTER
static void boost_level(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	int retval;

	set_default_result(data);

	if ((data->cmd_param[0] != DVFS_STAGE_NONE) &&
		(data->cmd_param[0] != DVFS_STAGE_SINGLE) &&
		(data->cmd_param[0] != DVFS_STAGE_DUAL) &&
		(data->cmd_param[0] != DVFS_STAGE_TRIPLE) &&
		(data->cmd_param[0] != DVFS_STAGE_PENTA) &&
		(data->cmd_param[0] != DVFS_STAGE_NINTH)) {

		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;

		goto boost_out;
	}

	rmi4_data->dvfs_boost_mode = data->cmd_param[0];

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

	if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_NONE) {
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: booster stop failed(%d).\n",
					__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;

			rmi4_data->dvfs_lock_status = false;
		}
	}

boost_out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;

	return;
}
#endif

#ifdef SIDE_TOUCH
static void sidekey_enable(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	unsigned char value;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 1) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		int retval;

		rmi4_data->sidekey_enables = data->cmd_param[0];

		retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_READ,
						rmi4_data->f51_handle->general_control2_addr,
						1, &value);
		if (retval < 0)
			goto i2c_err;

		if (rmi4_data->sidekey_enables)
			value |= SIDE_BUTTONS_ENABLE;
		else
			value &= ~(SIDE_BUTTONS_ENABLE);

		retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_WRITE,
						rmi4_data->f51_handle->general_control2_addr,
						1, &value);
		if (retval < 0)
			goto i2c_err;

	}

	if (!rmi4_data->sidekey_enables)
		synaptics_rmi4_free_sidekeys(rmi4_data);

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;

	return;

i2c_err:
	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
	data->cmd_state = CMD_STATUS_FAIL;

	return;
}

static void get_sidekey_threshold(void)
{
	int retval = 0, ii = 0, length = 0;
	unsigned char sidekey_threshold[10];
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	char temp[70] = {0, };
	char t_temp[7] = {0, };

	length = NUM_OF_SIDE_BUTTONS;
	retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data,
					REGISTER_READ, rmi4_data->f51_handle->sidekey_threshold_addr,
					length, sidekey_threshold);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: read failed.\n", __func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NG));
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_FAIL;
		return;
	}

	while (ii < NUM_OF_SIDE_BUTTONS) {
		snprintf(t_temp, 7, "%u, ", sidekey_threshold[ii]);
		strcat(temp, t_temp);
		ii++;
	}

	set_default_result(data);
	sprintf(data->cmd_buff, "%s", temp);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
	data->cmd_state = CMD_STATUS_OK;

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;

	return;
}

static void sidekey_partial_enable(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	int ii;
	unsigned char key_enable = 0;
	int retval;
	unsigned short data_addr = 0;

	set_default_result(data);

	data_addr = rmi4_data->f51_ctrl_base_addr + F51_SIDE_BUTTON_PARTIAL_ENABLE_OFFSET;


	/* check previous key_enable register value.(if power off, setting default value : 0x3F : all key enable) */
	retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data,
					REGISTER_READ, data_addr, 1, &key_enable);
	key_enable = 0;


	for (ii = 0; ii < CMD_PARAM_NUM; ii++) {
		if ((ii > 0) && (data->cmd_param[ii] == '\0'))
			break;

		key_enable |= (1 << data->cmd_param[ii]);
	}

	data_addr = rmi4_data->f51_ctrl_base_addr + F51_SIDE_BUTTON_PARTIAL_ENABLE_OFFSET;

	retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data,
					REGISTER_WRITE, data_addr, 1, &key_enable);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s failed, retval = %d\n",
				__func__, retval);
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
		data->cmd_state = CMD_STATUS_OK;
	}

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;

	return;

}

static void set_sidekey_delta(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	int retval;
	int length = 1;
	unsigned char sidekey_production_test[1];
	unsigned short data_addr = 0x41A;

	set_default_result(data);

	if (rmi4_data->touch_stopped) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP turned off");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data,
					REGISTER_READ, data_addr, length, sidekey_production_test);

	sidekey_production_test[0] |= SIDE_BUTTONS_PRODUCTION_TEST;

	retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data,
					REGISTER_WRITE, data_addr, length, sidekey_production_test);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
	}
	data->cmd_state = CMD_STATUS_OK;

	return;

}

static void run_sidekey_delta_read(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	int *report_data;
	char temp[CMD_STR_LEN];
	char temp2[CMD_RESULT_STR_LEN];
	unsigned char ii, jj = 0;
	unsigned short num_of_tx;
	unsigned short num_of_rx;
	unsigned char command = 0x01;
	int retval;
	int length = 1;
	unsigned char sidekey_production_test[1];

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP enter suspend");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data,
					REGISTER_READ, rmi4_data->f51_handle->general_control2_addr,
					length, sidekey_production_test);

	sidekey_production_test[0] |= SIDE_BUTTONS_PRODUCTION_TEST;

	retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data,
					REGISTER_WRITE, rmi4_data->f51_handle->general_control2_addr,
					length, sidekey_production_test);

	rmi4_data->sidekey_test = true;
	msleep(100);

	if (!synaptics_rmi4_f54_get_report_type(F54_ABS_DELTA)) {
		data->cmd_state = CMD_STATUS_FAIL;
		return;
	}

	report_data = f54->factory_data->absdelta_data;
	memcpy(report_data, f54->report_data, f54->report_size);
	memset(temp, 0, CMD_STR_LEN);
	memset(temp2, 0, CMD_RESULT_STR_LEN);

	num_of_tx = rmi4_data->num_of_tx;
	num_of_rx = rmi4_data->num_of_rx;

	for (ii = 0; ii < num_of_rx + num_of_tx + NUM_OF_SIDE_BUTTONS; ii++) {
		if (f54->rmi4_data->ic_version != SYNAPTICS_PRODUCT_ID_S5100)
			*report_data &= 0x0FFFF;
		if (ii < (num_of_rx + num_of_tx)) {
			report_data++;
			continue;
		} else {
			jj = ii - (num_of_rx + num_of_tx);
		}

		dev_info(&rmi4_data->i2c_client->dev,
				"%s: %s [%d] = %d\n", __func__,
				"SIDE", jj,
				*report_data);
		snprintf(temp, CMD_RESULT_STR_LEN, "%d,", *report_data);
		strncat(temp2, temp, 9);
		report_data++;
	}
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", temp2);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	goto out;

	retval = f54->fn_ptr->write(rmi4_data,
				rmi4_data->f01_cmd_base_addr,
				&command,
				sizeof(command));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
	}

out:

	rmi4_data->sidekey_test = false;

	data->cmd_state = CMD_STATUS_OK;

	return;
}

static void run_sidekey_abscap_read(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	unsigned int *report_data;
	char temp[CMD_STR_LEN];
	char temp2[CMD_RESULT_STR_LEN];
	unsigned char ii, jj = 0;
	unsigned short num_of_tx;
	unsigned short num_of_rx;
	int retval;
	unsigned char sidekey_production_test[1];
	int length = 1;
	unsigned char cmd_state = CMD_STATUS_RUNNING;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP enter suspend");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data,
					REGISTER_READ, rmi4_data->f51_handle->general_control2_addr,
					length, sidekey_production_test);

	sidekey_production_test[0] |= SIDE_BUTTONS_PRODUCTION_TEST;

	retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data,
					REGISTER_WRITE, rmi4_data->f51_handle->general_control2_addr,
					length, sidekey_production_test);

	rmi4_data->sidekey_test = true;
	msleep(100);

	rmi4_data->sidekey_test = true;

	if (!synaptics_rmi4_f54_get_report_type(F54_ABS_CAP)) {
		cmd_state = CMD_STATUS_FAIL;
		goto exit;
	}

	report_data = f54->factory_data->abscap_data;
	memcpy(report_data, f54->report_data, f54->report_size);
	memset(temp, 0, CMD_STR_LEN);
	memset(temp2, 0, CMD_RESULT_STR_LEN);

	num_of_tx = rmi4_data->num_of_tx;
	num_of_rx = rmi4_data->num_of_rx;

	for (ii = 0; ii < num_of_rx + num_of_tx + NUM_OF_SIDE_BUTTONS; ii++) {
		if (f54->rmi4_data->ic_version != SYNAPTICS_PRODUCT_ID_S5100)
			*report_data &= 0x0FFFF;

		if (ii < (num_of_rx + num_of_tx)) {
			report_data++;
			continue;
		} else {
			jj = ii - (num_of_rx + num_of_tx);
		}

		dev_info(&rmi4_data->i2c_client->dev,
				"%s: %s [%d] = %d\n", __func__,
				"SIDE", jj,
				*report_data);
		snprintf(temp, CMD_RESULT_STR_LEN, "%d,", *report_data);
		strncat(temp2, temp, 9);
		report_data++;
	}

	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", temp2);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	cmd_state = CMD_STATUS_OK;
exit:
	retval = rmi4_data->reset_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
	}

	rmi4_data->sidekey_test = false;

	data->cmd_state = cmd_state;

	return;
}

static void set_deepsleep_mode(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);

	rmi4_data->use_deepsleep = data->cmd_param[0];

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;


	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	return;
}

static void set_sidekey_only_enable(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	unsigned char value = 0;

	set_default_result(data);

	if (!rmi4_data->sensor_sleep && !data->cmd_param[0]) {
		dev_info(&rmi4_data->i2c_client->dev,
				"%s: sensor wakeup, no sidekey only enable\n",
				__func__);

		synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_READ,
					rmi4_data->f51_handle->general_control2_addr, 1, &value);
		if (!((value >> 5) & 0x1)) {
			value |= 0x20;
			synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_WRITE,
						rmi4_data->f51_handle->general_control2_addr, 1, &value);
		}

		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
		data->cmd_state = CMD_STATUS_OK;

		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_OK;

		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

		return;
	}

#ifdef DISABLE_IRQ_WHEN_ENTER_DEEPSLEEP
	synaptics_rmi4_irq_enable(rmi4_data, data->cmd_param[0]);
#endif

	if (data->cmd_param[0]) {
		synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_READ,
					rmi4_data->f01_ctrl_base_addr, 1, &value);

		value &= ~(SENSOR_SLEEP);

		synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_WRITE,
					rmi4_data->f01_ctrl_base_addr, 1, &value);

	} else {
		synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_READ,
					rmi4_data->f01_ctrl_base_addr, 1, &value);

		value |= SENSOR_SLEEP;

		synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_WRITE,
					rmi4_data->f01_ctrl_base_addr, 1, &value);

	}
	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	return;
}

static void lozemode_enable(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	unsigned char value = 0;

	set_default_result(data);

	synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_READ,
				rmi4_data->f51_handle->general_control2_addr, 1, &value);

	if (data->cmd_param[0])
		value |= 0x20;
	else
		value &= ~0x20;

	synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_WRITE,
				rmi4_data->f51_handle->general_control2_addr, 1, &value);

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	return;

}
#endif

static void set_tsp_test_result(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	int retval = 0;
	unsigned char device_status = 0;
	int retry = 2;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP enter suspend");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (data->cmd_param[0] < SYNAPTICS_FACTORY_TEST_NONE ||\
		data->cmd_param[0] > SYNAPTICS_FACTORY_TEST_PASS) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		retval = rmi4_data->i2c_read(rmi4_data,
				rmi4_data->f01_data_base_addr,
				&device_status,
				sizeof(device_status));
		if (device_status != 0) {
			snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NR));
			set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
			data->cmd_state = CMD_STATUS_FAIL;
			dev_err(&rmi4_data->i2c_client->dev, "%s: IC not ready[%d]\n",
					__func__, device_status);

			return;
		} else {
			dev_err(&rmi4_data->i2c_client->dev, "%s: check status register[%d]\n",
					__func__, device_status);
		}

		while (retry--) {
			retval = synaptics_rmi4_set_tsp_test_result_in_config(data->cmd_param[0]);
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev, "%s: failed [%d], retry%d\n",
					__func__, retval, retry);
				synaptics_power_ctrl(rmi4_data, false);
				msleep(SYNAPTICS_POWER_MARGIN_TIME);
				synaptics_power_ctrl(rmi4_data, true);
				msleep(SYNAPTICS_POWER_MARGIN_TIME);
			} else {
				break;
			}
		}
		msleep(200);

		if (retval < 0) {
			snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NA));
			set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
			data->cmd_state = CMD_STATUS_FAIL;
			dev_err(&rmi4_data->i2c_client->dev, "%s: failed [%d]\n",
					__func__, retval);
		} else {
			snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(OK));
			set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
			data->cmd_state = CMD_STATUS_OK;
			dev_info(&rmi4_data->i2c_client->dev, "%s: success [%d]\n",
					__func__, retval);
		}
	}

	return;
}

static void get_tsp_test_result(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	int retval = 0;

	set_default_result(data);

	retval = synaptics_rmi4_tsp_read_test_result(rmi4_data);

	if (retval == 0) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NONE));
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_OK;
		dev_err(&rmi4_data->i2c_client->dev, "%s: success [%d]\n",
				__func__, retval);
	} else if (retval == 1){
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(FAIL));
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_OK;
		dev_info(&rmi4_data->i2c_client->dev, "%s: success [%d]\n",
				__func__, retval);
	} else if (retval == 2) {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(PASS));
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_OK;
		dev_info(&rmi4_data->i2c_client->dev, "%s: success [%d]\n",
				__func__, retval);
	} else {
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NG));
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_FAIL;
		dev_err(&rmi4_data->i2c_client->dev, "%s: failed [%d]\n",
				__func__, retval);
	}

	return;
}

#ifdef USE_STYLUS
static void stylus_enable(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	int retval;
	unsigned char value;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 2) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		rmi4_data->use_stylus = data->cmd_param[0];

		if (data->cmd_param[0])
			value = 0x1;
		else
			value = 0x0;

		retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data,
						REGISTER_WRITE, rmi4_data->f51_handle->stylus_enable_addr,
						1, &value);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to send edgeoffset, error = %d\n",
					__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;
		}
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
		data->cmd_state = CMD_STATUS_OK;
	}

	synaptics_rmi4_free_fingers(rmi4_data);

	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	return;
}
#endif

#ifdef USE_ACTIVE_REPORT_RATE
static void report_rate(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	unsigned char reg_data[4] = {0, 0, 0, 0};
	unsigned short r_addr = 0;

	set_default_result(data);

	if (data->cmd_param[0] < 0 || data->cmd_param[0] > 2) {
		snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
		data->cmd_state = CMD_STATUS_FAIL;
	} else {
		int retval;

		r_addr = REGISTER_ADDR_CHANGE_REPORT_RATE; /* f54 analog command : set report rate */

		/* cmd_param == 1 ? (set 60 Hz) : == 0 ? (set 90 Hz) */
		retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_READ, r_addr, 4, reg_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s failed read f54 analog ctrl94, retval = %d\n",
				__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;
			goto out;
		}

		/* cmd_param value
		 * 0: report rate 90 Hz
		 * 1: report rate 64 Hz
		 * 2: report rate 33 Hz
		 */
		if (data->cmd_param[0] == 0)
			reg_data[3] = REPORT_RATE_90HZ;
		else if (data->cmd_param[0] == 1)
			reg_data[3] = REPORT_RATE_60HZ;
		else if (data->cmd_param[0] == 2)
			reg_data[3] = REPORT_RATE_30HZ;

		retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_WRITE, r_addr, 4, reg_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s set report_rate failed, retval = %d\n",
				__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;
			goto out;
		}

		r_addr = f54->command_base_addr; /* f54 analog command : set force update */

		retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_READ, r_addr, 1, reg_data);
		reg_data[0] |= REPORT_RATE_FORCE_UPDATE;

		retval = synaptics_rmi4_set_custom_ctrl_register(rmi4_data, REGISTER_WRITE, r_addr, 1, reg_data);

		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s set force update failed, retval = %d\n",
				__func__, retval);
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "NG");
			data->cmd_state = CMD_STATUS_FAIL;
		} else {
			snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
			data->cmd_state = CMD_STATUS_OK;
		}

	}
out:
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	return;
}
#endif

static void debug_log(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	set_default_result(data);

	rmi4_data->debug_log = data->cmd_param[0];

	snprintf(data->cmd_buff, sizeof(data->cmd_buff), "OK");
	data->cmd_state = CMD_STATUS_OK;


	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	return;
}

/*
 * No need to read 'cmd_result' to complete cmd.
 * Changed to prevent unsupported cmd is written to 'cmd' and not read 'cmd_result', so later cmd is rejected.
 * ex) clear_cover_mode
 */
static void not_support_cmd(void)
{
	struct factory_data *data = f54->factory_data;

	set_default_result(data);
	snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", tostring(NA));
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	mutex_lock(&data->cmd_lock);
	data->cmd_is_running = false;
	mutex_unlock(&data->cmd_lock);

	data->cmd_state = CMD_STATUS_WAITING;
	dev_err(&f54->rmi4_data->i2c_client->dev, "%s\n", __func__);
}
#endif

static ssize_t synaptics_rmi4_f54_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->status);
}

static ssize_t synaptics_rmi4_f54_report_size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->report_size);
}

static ssize_t synaptics_rmi4_f54_no_auto_cal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->no_auto_cal);
}

static ssize_t synaptics_rmi4_f54_no_auto_cal_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char data;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = kstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting > 1)
		return -EINVAL;

	retval = f54->fn_ptr->read(rmi4_data,
			f54->control_base_addr,
			&data,
			sizeof(data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read control register\n",
				__func__);
		return retval;
	}

	if ((data & NO_AUTO_CAL_MASK) == setting)
		return count;

	data = (data & ~NO_AUTO_CAL_MASK) | (data & NO_AUTO_CAL_MASK);

	retval = f54->fn_ptr->write(rmi4_data,
			f54->control_base_addr,
			&data,
			sizeof(data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write control register\n",
				__func__);
		return retval;
	}

	f54->no_auto_cal = (setting == 1);

	return count;
}

static ssize_t synaptics_rmi4_f54_report_type_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->report_type);
}

static ssize_t synaptics_rmi4_f54_report_type_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char data;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = kstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (!is_report_type_valid((enum f54_report_types)setting)) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Report type not supported by driver\n",
				__func__);
		return -EINVAL;
	}

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_BUSY) {
		f54->report_type = (enum f54_report_types)setting;
		data = (unsigned char)setting;
		retval = f54->fn_ptr->write(rmi4_data,
				f54->data_base_addr,
				&data,
				sizeof(data));
		mutex_unlock(&f54->status_mutex);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to write data register\n",
					__func__);
			return retval;
		}
		return count;
	} else {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Previous get report still ongoing\n",
				__func__);
		mutex_unlock(&f54->status_mutex);
		return -EINVAL;
	}
}

static ssize_t synaptics_rmi4_f54_fifoindex_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int retval;
	unsigned char data[2];
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = f54->fn_ptr->read(rmi4_data,
			f54->data_base_addr + DATA_REPORT_INDEX_OFFSET,
			data,
			sizeof(data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read data registers\n",
				__func__);
		return retval;
	}

	batohs(&f54->fifoindex, data);

	return snprintf(buf, PAGE_SIZE, "%u\n", f54->fifoindex);
}
static ssize_t synaptics_rmi4_f54_fifoindex_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char data[2];
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = kstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	f54->fifoindex = setting;

	hstoba(data, (unsigned short)setting);

	retval = f54->fn_ptr->write(rmi4_data,
			f54->data_base_addr + DATA_REPORT_INDEX_OFFSET,
			data,
			sizeof(data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write data registers\n",
				__func__);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_f54_do_preparation_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = kstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_IDLE) {
		if (f54->status != STATUS_BUSY) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Invalid status (%d)\n",
					__func__, f54->status);
		} else {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Previous get report still ongoing\n",
					__func__);
		}
		mutex_unlock(&f54->status_mutex);
		return -EBUSY;
	}

	mutex_unlock(&f54->status_mutex);

	retval = do_preparation();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to do preparation\n",
				__func__);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_f54_get_report_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char command;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = kstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return -EINVAL;

	command = (unsigned char)COMMAND_GET_REPORT;

	if (!is_report_type_valid(f54->report_type)) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Invalid report type\n",
				__func__);
		return -EINVAL;
	}

	mutex_lock(&f54->status_mutex);

	if (f54->status != STATUS_IDLE) {
		if (f54->status != STATUS_BUSY) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Invalid status (%d)\n",
					__func__, f54->status);
		} else {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Previous get report still ongoing\n",
					__func__);
		}
		mutex_unlock(&f54->status_mutex);
		return -EBUSY;
	}

	set_interrupt(true);

	f54->status = STATUS_BUSY;

	retval = f54->fn_ptr->write(rmi4_data,
			f54->command_base_addr,
			&command,
			sizeof(command));
	mutex_unlock(&f54->status_mutex);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write get report command\n",
				__func__);
		return retval;
	}

#ifdef WATCHDOG_HRTIMER
	hrtimer_start(&f54->watchdog,
			ktime_set(WATCHDOG_TIMEOUT_S, 0),
			HRTIMER_MODE_REL);
#endif

	return count;
}

static ssize_t synaptics_rmi4_f54_force_cal_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned char command;
	unsigned long setting;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = kstrtoul(buf, 10, &setting);
	if (retval)
		return retval;

	if (setting != 1)
		return count;

	command = (unsigned char)COMMAND_FORCE_CAL;

	if (f54->status == STATUS_BUSY)
		return -EBUSY;

	retval = f54->fn_ptr->write(rmi4_data,
			f54->command_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write force cal command\n",
				__func__);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_f54_num_of_mapped_rx_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->rx_assigned);
}

static ssize_t synaptics_rmi4_f54_num_of_mapped_tx_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", f54->tx_assigned);
}

	simple_show_func_unsigned(query, num_of_rx_electrodes)
	simple_show_func_unsigned(query, num_of_tx_electrodes)
	simple_show_func_unsigned(query, has_image16)
	simple_show_func_unsigned(query, has_image8)
	simple_show_func_unsigned(query, has_baseline)
	simple_show_func_unsigned(query, clock_rate)
	simple_show_func_unsigned(query, touch_controller_family)
	simple_show_func_unsigned(query, has_pixel_touch_threshold_adjustment)
	simple_show_func_unsigned(query, has_sensor_assignment)
	simple_show_func_unsigned(query, has_interference_metric)
	simple_show_func_unsigned(query, has_sense_frequency_control)
	simple_show_func_unsigned(query, has_firmware_noise_mitigation)
	simple_show_func_unsigned(query, has_two_byte_report_rate)
	simple_show_func_unsigned(query, has_one_byte_report_rate)
	simple_show_func_unsigned(query, has_relaxation_control)
	simple_show_func_unsigned(query, curve_compensation_mode)
	simple_show_func_unsigned(query, has_iir_filter)
	simple_show_func_unsigned(query, has_cmn_removal)
	simple_show_func_unsigned(query, has_cmn_maximum)
	simple_show_func_unsigned(query, has_touch_hysteresis)
	simple_show_func_unsigned(query, has_edge_compensation)
	simple_show_func_unsigned(query, has_per_frequency_noise_control)
	simple_show_func_unsigned(query, has_signal_clarity)
simple_show_func_unsigned(query, number_of_sensing_frequencies)

	show_store_func_unsigned(control, reg_0, no_relax)
	show_store_func_unsigned(control, reg_0, no_scan)
	show_store_func_unsigned(control, reg_1, bursts_per_cluster)
	show_store_func_unsigned(control, reg_2, saturation_cap)
	show_store_func_unsigned(control, reg_3, pixel_touch_threshold)
	show_store_func_unsigned(control, reg_4__6, rx_feedback_cap)
	show_store_func_unsigned(control, reg_4__6, low_ref_cap)
	show_store_func_unsigned(control, reg_4__6, low_ref_feedback_cap)
	show_store_func_unsigned(control, reg_4__6, low_ref_polarity)
	show_store_func_unsigned(control, reg_4__6, high_ref_cap)
	show_store_func_unsigned(control, reg_4__6, high_ref_feedback_cap)
	show_store_func_unsigned(control, reg_4__6, high_ref_polarity)
	show_store_func_unsigned(control, reg_7, cbc_cap)
	show_store_func_unsigned(control, reg_7, cbc_polarity)
	show_store_func_unsigned(control, reg_7, cbc_tx_carrier_selection)
	show_store_func_unsigned(control, reg_8__9, integration_duration)
	show_store_func_unsigned(control, reg_8__9, reset_duration)
	show_store_func_unsigned(control, reg_10, noise_sensing_bursts_per_image)
	show_store_func_unsigned(control, reg_12__13, slow_relaxation_rate)
	show_store_func_unsigned(control, reg_12__13, fast_relaxation_rate)
	show_store_func_unsigned(control, reg_14, rxs_on_xaxis)
	show_store_func_unsigned(control, reg_14, curve_comp_on_txs)
	show_store_func_unsigned(control, reg_20, disable_noise_mitigation)
	show_store_func_unsigned(control, reg_21, freq_shift_noise_threshold)
	show_store_func_unsigned(control, reg_22__26, medium_noise_threshold)
	show_store_func_unsigned(control, reg_22__26, high_noise_threshold)
	show_store_func_unsigned(control, reg_22__26, noise_density)
	show_store_func_unsigned(control, reg_22__26, frame_count)
	show_store_func_unsigned(control, reg_27, iir_filter_coef)
	show_store_func_unsigned(control, reg_28, quiet_threshold)
	show_store_func_unsigned(control, reg_29, cmn_filter_disable)
	show_store_func_unsigned(control, reg_30, cmn_filter_max)
	show_store_func_unsigned(control, reg_31, touch_hysteresis)
	show_store_func_unsigned(control, reg_32__35, rx_low_edge_comp)
	show_store_func_unsigned(control, reg_32__35, rx_high_edge_comp)
	show_store_func_unsigned(control, reg_32__35, tx_low_edge_comp)
	show_store_func_unsigned(control, reg_32__35, tx_high_edge_comp)
	show_store_func_unsigned(control, reg_41, no_signal_clarity)
	show_store_func_unsigned(control, reg_57, cbc_cap_0d)
	show_store_func_unsigned(control, reg_57, cbc_polarity_0d)
show_store_func_unsigned(control, reg_57, cbc_tx_carrier_selection_0d)

	show_replicated_func_unsigned(control, reg_15, sensor_rx_assignment)
	show_replicated_func_unsigned(control, reg_16, sensor_tx_assignment)
	show_replicated_func_unsigned(control, reg_17, disable)
	show_replicated_func_unsigned(control, reg_17, filter_bandwidth)
	show_replicated_func_unsigned(control, reg_19, stretch_duration)
	show_replicated_func_unsigned(control, reg_38, noise_control_1)
	show_replicated_func_unsigned(control, reg_39, noise_control_2)
show_replicated_func_unsigned(control, reg_40, noise_control_3)

	show_store_replicated_func_unsigned(control, reg_36, axis1_comp)
show_store_replicated_func_unsigned(control, reg_37, axis2_comp)

static ssize_t synaptics_rmi4_f54_burst_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int retval;
	int size = 0;
	unsigned char ii;
	unsigned char *temp;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	mutex_lock(&f54->control_mutex);

	retval = f54->fn_ptr->read(rmi4_data,
			f54->control.reg_17->address,
			(unsigned char *)f54->control.reg_17->data,
			f54->control.reg_17->length);
	if (retval < 0) {
		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: Failed to read control reg_17\n",
				__func__);
	}

	retval = f54->fn_ptr->read(rmi4_data,
			f54->control.reg_18->address,
			(unsigned char *)f54->control.reg_18->data,
			f54->control.reg_18->length);
	if (retval < 0) {
		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: Failed to read control reg_18\n",
				__func__);
	}

	mutex_unlock(&f54->control_mutex);

	temp = buf;

	for (ii = 0; ii < f54->control.reg_17->length; ii++) {
		retval = snprintf(temp, PAGE_SIZE - size, "%u ", (1 << 8) *
				f54->control.reg_17->data[ii].burst_count_b8__10 +
				f54->control.reg_18->data[ii].burst_count_b0__7);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Faild to write output\n",
					__func__);
			return retval;
		}
		size += retval;
		temp += retval;
	}

	retval = snprintf(temp, PAGE_SIZE - size, "\n");
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Faild to write null terminator\n",
				__func__);
		return retval;
	}

	return size + retval;
}

static ssize_t synaptics_rmi4_f54_data_read(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	mutex_lock(&f54->data_mutex);

	if (count < f54->report_size) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Report type %d data size (%d) too large\n",
				__func__, f54->report_type, f54->report_size);
		mutex_unlock(&f54->data_mutex);
		return -EINVAL;
	}

	if (f54->report_data) {
		memcpy(buf, f54->report_data, f54->report_size);
		mutex_unlock(&f54->data_mutex);
		return f54->report_size;
	} else {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Report type %d data not available\n",
				__func__, f54->report_type);
		mutex_unlock(&f54->data_mutex);
		return -EINVAL;
	}
}

static int synaptics_rmi4_f54_set_sysfs(void)
{
	int retval;
	int reg_num;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	f54->attr_dir = kobject_create_and_add("f54",
			&rmi4_data->input_dev->dev.kobj);
	if (!f54->attr_dir) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs directory\n",
				__func__);
		goto exit_1;
	}

	retval = sysfs_create_bin_file(f54->attr_dir, &dev_report_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs bin file\n",
				__func__);
		goto exit_2;
	}

	retval = sysfs_create_group(f54->attr_dir, &attr_group);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs attributes\n",
				__func__);
		goto exit_3;
	}

	for (reg_num = 0; reg_num < ARRAY_SIZE(attrs_ctrl_regs); reg_num++) {
		if (attrs_ctrl_regs_exist[reg_num]) {
			retval = sysfs_create_group(f54->attr_dir,
					&attrs_ctrl_regs[reg_num]);
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev,
						"%s: Failed to create sysfs attributes\n",
						__func__);
				goto exit_4;
			}
		}
	}

#ifdef FACTORY_MODE
	retval = sysfs_create_group(f54->attr_dir, &cmd_attr_group);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs attributes\n",
				__func__);
		goto exit_4;
	}
#endif

	return 0;

exit_4:
	for (reg_num--; reg_num >= 0; reg_num--)
		sysfs_remove_group(f54->attr_dir, &attrs_ctrl_regs[reg_num]);

	sysfs_remove_group(f54->attr_dir, &attr_group);

exit_3:
	sysfs_remove_bin_file(f54->attr_dir, &dev_report_data);

exit_2:
	kobject_put(f54->attr_dir);

exit_1:
	return -ENODEV;
}

static int synaptics_rmi4_f54_set_ctrl(void)
{
	unsigned char length;
	unsigned char reg_num = 0;
	unsigned char num_of_sensing_freqs;
	unsigned short reg_addr = f54->control_base_addr;
	struct f54_control *control = &f54->control;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	num_of_sensing_freqs = f54->query.number_of_sensing_frequencies;

	/* control 0 */
	attrs_ctrl_regs_exist[reg_num] = true;
	control->reg_0 = kzalloc(sizeof(*(control->reg_0)),
			GFP_KERNEL);
	if (!control->reg_0)
		goto exit_no_mem;
	control->reg_0->address = reg_addr;
	reg_addr += sizeof(control->reg_0->data);
	reg_num++;

	/* control 1 */
	if ((f54->query.touch_controller_family == 0) ||
			(f54->query.touch_controller_family == 1)) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_1 = kzalloc(sizeof(*(control->reg_1)),
				GFP_KERNEL);
		if (!control->reg_1)
			goto exit_no_mem;
		control->reg_1->address = reg_addr;
		reg_addr += sizeof(control->reg_1->data);
	}
	reg_num++;

	/* control 2 */
	attrs_ctrl_regs_exist[reg_num] = true;
	control->reg_2 = kzalloc(sizeof(*(control->reg_2)),
			GFP_KERNEL);
	if (!control->reg_2)
		goto exit_no_mem;
	control->reg_2->address = reg_addr;
	reg_addr += sizeof(control->reg_2->data);
	reg_num++;

	/* control 3 */
	if (f54->query.has_pixel_touch_threshold_adjustment == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_3 = kzalloc(sizeof(*(control->reg_3)),
				GFP_KERNEL);
		if (!control->reg_3)
			goto exit_no_mem;
		control->reg_3->address = reg_addr;
		reg_addr += sizeof(control->reg_3->data);
	}
	reg_num++;

	/* controls 4 5 6 */
	if ((f54->query.touch_controller_family == 0) ||
			(f54->query.touch_controller_family == 1)) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_4__6 = kzalloc(sizeof(*(control->reg_4__6)),
				GFP_KERNEL);
		if (!control->reg_4__6)
			goto exit_no_mem;
		control->reg_4__6->address = reg_addr;
		reg_addr += sizeof(control->reg_4__6->data);
	}
	reg_num++;

	/* control 7 */
	if (f54->query.touch_controller_family == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_7 = kzalloc(sizeof(*(control->reg_7)),
				GFP_KERNEL);
		if (!control->reg_7)
			goto exit_no_mem;
		control->reg_7->address = reg_addr;
		reg_addr += sizeof(control->reg_7->data);
	}
	reg_num++;

	/* controls 8 9 */
	if ((f54->query.touch_controller_family == 0) ||
			(f54->query.touch_controller_family == 1)) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_8__9 = kzalloc(sizeof(*(control->reg_8__9)),
				GFP_KERNEL);
		if (!control->reg_8__9)
			goto exit_no_mem;
		control->reg_8__9->address = reg_addr;
		reg_addr += sizeof(control->reg_8__9->data);
	}
	reg_num++;

	/* control 10 */
	if (f54->query.has_interference_metric == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_10 = kzalloc(sizeof(*(control->reg_10)),
				GFP_KERNEL);
		if (!control->reg_10)
			goto exit_no_mem;
		control->reg_10->address = reg_addr;
		reg_addr += sizeof(control->reg_10->data);
	}
	reg_num++;

	/* control 11 */
	if (f54->query.has_ctrl11 == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_11 = kzalloc(sizeof(*(control->reg_11)),
				GFP_KERNEL);
		if (!control->reg_11)
			goto exit_no_mem;
		control->reg_11->address = reg_addr;
		reg_addr += sizeof(control->reg_11->data);
	}
	reg_num++;

	/* controls 12 13 */
	if (f54->query.has_relaxation_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_12__13 = kzalloc(sizeof(*(control->reg_12__13)),
				GFP_KERNEL);
		if (!control->reg_12__13)
			goto exit_no_mem;
		control->reg_12__13->address = reg_addr;
		reg_addr += sizeof(control->reg_12__13->data);
	}
	reg_num++;

	/* controls 14 15 16 */
	if (f54->query.has_sensor_assignment == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;

		control->reg_14 = kzalloc(sizeof(*(control->reg_14)),
				GFP_KERNEL);
		if (!control->reg_14)
			goto exit_no_mem;
		control->reg_14->address = reg_addr;
		reg_addr += sizeof(control->reg_14->data);

		control->reg_15 = kzalloc(sizeof(*(control->reg_15)),
				GFP_KERNEL);
		if (!control->reg_15)
			goto exit_no_mem;
		control->reg_15->length = f54->query.num_of_rx_electrodes;
		control->reg_15->data = kzalloc(control->reg_15->length *
				sizeof(*(control->reg_15->data)), GFP_KERNEL);
		if (!control->reg_15->data)
			goto exit_no_mem;
		control->reg_15->address = reg_addr;
		reg_addr += control->reg_15->length;

		control->reg_16 = kzalloc(sizeof(*(control->reg_16)),
				GFP_KERNEL);
		if (!control->reg_16)
			goto exit_no_mem;
		control->reg_16->length = f54->query.num_of_tx_electrodes;
		control->reg_16->data = kzalloc(control->reg_16->length *
				sizeof(*(control->reg_16->data)), GFP_KERNEL);
		if (!control->reg_16->data)
			goto exit_no_mem;
		control->reg_16->address = reg_addr;
		reg_addr += control->reg_16->length;
	}
	reg_num++;

	/* controls 17 18 19 */
	if (f54->query.has_sense_frequency_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;

		length = num_of_sensing_freqs;

		control->reg_17 = kzalloc(sizeof(*(control->reg_17)),
				GFP_KERNEL);
		if (!control->reg_17)
			goto exit_no_mem;
		control->reg_17->length = length;
		control->reg_17->data = kzalloc(length *
				sizeof(*(control->reg_17->data)), GFP_KERNEL);
		if (!control->reg_17->data)
			goto exit_no_mem;
		control->reg_17->address = reg_addr;
		reg_addr += length;

		control->reg_18 = kzalloc(sizeof(*(control->reg_18)),
				GFP_KERNEL);
		if (!control->reg_18)
			goto exit_no_mem;
		control->reg_18->length = length;
		control->reg_18->data = kzalloc(length *
				sizeof(*(control->reg_18->data)), GFP_KERNEL);
		if (!control->reg_18->data)
			goto exit_no_mem;
		control->reg_18->address = reg_addr;
		reg_addr += length;

		control->reg_19 = kzalloc(sizeof(*(control->reg_19)),
				GFP_KERNEL);
		if (!control->reg_19)
			goto exit_no_mem;
		control->reg_19->length = length;
		control->reg_19->data = kzalloc(length *
				sizeof(*(control->reg_19->data)), GFP_KERNEL);
		if (!control->reg_19->data)
			goto exit_no_mem;
		control->reg_19->address = reg_addr;
		reg_addr += length;
	}
	reg_num++;

	/* control 20 */
	attrs_ctrl_regs_exist[reg_num] = true;
	control->reg_20 = kzalloc(sizeof(*(control->reg_20)),
			GFP_KERNEL);
	if (!control->reg_20)
		goto exit_no_mem;
	control->reg_20->address = reg_addr;
	reg_addr += sizeof(control->reg_20->data);
	reg_num++;

	/* control 21 */
	if (f54->query.has_sense_frequency_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_21 = kzalloc(sizeof(*(control->reg_21)),
				GFP_KERNEL);
		if (!control->reg_21)
			goto exit_no_mem;
		control->reg_21->address = reg_addr;
		reg_addr += sizeof(control->reg_21->data);
	}
	reg_num++;

	/* controls 22 23 24 25 26 */
	if (f54->query.has_firmware_noise_mitigation == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_22__26 = kzalloc(sizeof(*(control->reg_22__26)),
				GFP_KERNEL);
		if (!control->reg_22__26)
			goto exit_no_mem;
		control->reg_22__26->address = reg_addr;
		reg_addr += sizeof(control->reg_22__26->data);
	}
	reg_num++;

	/* control 27 */
	if (f54->query.has_iir_filter == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_27 = kzalloc(sizeof(*(control->reg_27)),
				GFP_KERNEL);
		if (!control->reg_27)
			goto exit_no_mem;
		control->reg_27->address = reg_addr;
		reg_addr += sizeof(control->reg_27->data);
	}
	reg_num++;

	/* control 28 */
	if (f54->query.has_firmware_noise_mitigation == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_28 = kzalloc(sizeof(*(control->reg_28)),
				GFP_KERNEL);
		if (!control->reg_28)
			goto exit_no_mem;
		control->reg_28->address = reg_addr;
		reg_addr += sizeof(control->reg_28->data);
	}
	reg_num++;

	/* control 29 */
	if (f54->query.has_cmn_removal == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_29 = kzalloc(sizeof(*(control->reg_29)),
				GFP_KERNEL);
		if (!control->reg_29)
			goto exit_no_mem;
		control->reg_29->address = reg_addr;
		reg_addr += sizeof(control->reg_29->data);
	}
	reg_num++;

	/* control 30 */
	if (f54->query.has_cmn_maximum == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_30 = kzalloc(sizeof(*(control->reg_30)),
				GFP_KERNEL);
		if (!control->reg_30)
			goto exit_no_mem;
		control->reg_30->address = reg_addr;
		reg_addr += sizeof(control->reg_30->data);
	}
	reg_num++;

	/* control 31 */
	if (f54->query.has_touch_hysteresis == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_31 = kzalloc(sizeof(*(control->reg_31)),
				GFP_KERNEL);
		if (!control->reg_31)
			goto exit_no_mem;
		control->reg_31->address = reg_addr;
		reg_addr += sizeof(control->reg_31->data);
	}
	reg_num++;

	/* controls 32 33 34 35 */
	if (f54->query.has_edge_compensation == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_32__35 = kzalloc(sizeof(*(control->reg_32__35)),
				GFP_KERNEL);
		if (!control->reg_32__35)
			goto exit_no_mem;
		control->reg_32__35->address = reg_addr;
		reg_addr += sizeof(control->reg_32__35->data);
	}
	reg_num++;

	/* control 36 */
	if ((f54->query.curve_compensation_mode == 1) ||
			(f54->query.curve_compensation_mode == 2)) {
		attrs_ctrl_regs_exist[reg_num] = true;

		if (f54->query.curve_compensation_mode == 1) {
			length = max(f54->query.num_of_rx_electrodes,
					f54->query.num_of_tx_electrodes);
		} else if (f54->query.curve_compensation_mode == 2) {
			length = f54->query.num_of_rx_electrodes;
		}

		control->reg_36 = kzalloc(sizeof(*(control->reg_36)),
				GFP_KERNEL);
		if (!control->reg_36)
			goto exit_no_mem;
		control->reg_36->length = length;
		control->reg_36->data = kzalloc(length *
				sizeof(*(control->reg_36->data)), GFP_KERNEL);
		if (!control->reg_36->data)
			goto exit_no_mem;
		control->reg_36->address = reg_addr;
		reg_addr += length;
	}
	reg_num++;

	/* control 37 */
	if (f54->query.curve_compensation_mode == 2) {
		attrs_ctrl_regs_exist[reg_num] = true;

		control->reg_37 = kzalloc(sizeof(*(control->reg_37)),
				GFP_KERNEL);
		if (!control->reg_37)
			goto exit_no_mem;
		control->reg_37->length = f54->query.num_of_tx_electrodes;
		control->reg_37->data = kzalloc(control->reg_37->length *
				sizeof(*(control->reg_37->data)), GFP_KERNEL);
		if (!control->reg_37->data)
			goto exit_no_mem;

		control->reg_37->address = reg_addr;
		reg_addr += control->reg_37->length;
	}
	reg_num++;

	/* controls 38 39 40 */
	if (f54->query.has_per_frequency_noise_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;

		control->reg_38 = kzalloc(sizeof(*(control->reg_38)),
				GFP_KERNEL);
		if (!control->reg_38)
			goto exit_no_mem;
		control->reg_38->length = num_of_sensing_freqs;
		control->reg_38->data = kzalloc(control->reg_38->length *
				sizeof(*(control->reg_38->data)), GFP_KERNEL);
		if (!control->reg_38->data)
			goto exit_no_mem;
		control->reg_38->address = reg_addr;
		reg_addr += control->reg_38->length;

		control->reg_39 = kzalloc(sizeof(*(control->reg_39)),
				GFP_KERNEL);
		if (!control->reg_39)
			goto exit_no_mem;
		control->reg_39->length = num_of_sensing_freqs;
		control->reg_39->data = kzalloc(control->reg_39->length *
				sizeof(*(control->reg_39->data)), GFP_KERNEL);
		if (!control->reg_39->data)
			goto exit_no_mem;
		control->reg_39->address = reg_addr;
		reg_addr += control->reg_39->length;

		control->reg_40 = kzalloc(sizeof(*(control->reg_40)),
				GFP_KERNEL);
		if (!control->reg_40)
			goto exit_no_mem;
		control->reg_40->length = num_of_sensing_freqs;
		control->reg_40->data = kzalloc(control->reg_40->length *
				sizeof(*(control->reg_40->data)), GFP_KERNEL);
		if (!control->reg_40->data)
			goto exit_no_mem;
		control->reg_40->address = reg_addr;
		reg_addr += control->reg_40->length;
	}
	reg_num++;

	/* control 41 */
	if (f54->query.has_signal_clarity == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_41 = kzalloc(sizeof(*(control->reg_41)),
				GFP_KERNEL);
		if (!control->reg_41)
			goto exit_no_mem;
		control->reg_41->address = reg_addr;
		reg_addr += sizeof(control->reg_41->data);
	}
	reg_num++;

	/* control 42 */
	if (f54->query.has_variance_metric == 1)
		reg_addr += CONTROL_42_SIZE;

	/* controls 43 44 45 46 47 48 49 50 51 52 53 54 */
	if (f54->query.has_multi_metric_state_machine == 1)
		reg_addr += CONTROL_43_54_SIZE;

	/* controls 55 56 */
	if (f54->query.has_0d_relaxation_control == 1)
		reg_addr += CONTROL_55_56_SIZE;

	/* control 57 */
	if (f54->query.has_0d_acquisition_control == 1) {
		attrs_ctrl_regs_exist[reg_num] = true;
		control->reg_57 = kzalloc(sizeof(*(control->reg_57)),
				GFP_KERNEL);
		if (!control->reg_57)
			goto exit_no_mem;
		control->reg_57->address = reg_addr;
		reg_addr += sizeof(control->reg_57->data);
	}
	reg_num++;

	/* control 58 */
	if (f54->query.has_0d_acquisition_control == 1)
		reg_addr += CONTROL_58_SIZE;

	/* control 59 */
	if (f54->query.has_h_blank == 1)
		reg_addr += CONTROL_59_SIZE;

	/* controls 60 61 62 */
	if ((f54->query.has_h_blank == 1) ||
			(f54->query.has_v_blank == 1) ||
			(f54->query.has_long_h_blank == 1))
		reg_addr += CONTROL_60_62_SIZE;

	/* control 63 */
	if ((f54->query.has_h_blank == 1) ||
			(f54->query.has_v_blank == 1) ||
			(f54->query.has_long_h_blank == 1) ||
			(f54->query.has_slew_metric == 1) ||
			(f54->query.has_slew_option == 1) ||
			(f54->query.has_noise_mitigation2 == 1))
		reg_addr += CONTROL_63_SIZE;

	/* controls 64 65 66 67 */
	if (f54->query.has_h_blank == 1)
		reg_addr += CONTROL_64_67_SIZE * 7;
	else if ((f54->query.has_v_blank == 1) ||
			(f54->query.has_long_h_blank == 1))
		reg_addr += CONTROL_64_67_SIZE;

	/* controls 68 69 70 71 72 73 */
	if ((f54->query.has_h_blank == 1) ||
			(f54->query.has_v_blank == 1) ||
			(f54->query.has_long_h_blank == 1))
		reg_addr += CONTROL_68_73_SIZE;

	/* control 74 */
	if (f54->query.has_slew_metric == 1)
		reg_addr += CONTROL_74_SIZE;

	/* control 75 */
	if (f54->query.has_enhanced_stretch == 1)
		reg_addr += num_of_sensing_freqs;

	/* control 76 */
	if (f54->query.has_startup_fast_relaxation == 1)
		reg_addr += CONTROL_76_SIZE;

	/* controls 77 78 */
	if (f54->query.has_esd_control == 1)
		reg_addr += CONTROL_77_78_SIZE;

	/* controls 79 80 81 82 83 */
	if (f54->query.has_noise_mitigation2 == 1)
		reg_addr += CONTROL_79_83_SIZE;

	/* controls 84 85 */
	if (f54->query.has_energy_ratio_relaxation == 1)
		reg_addr += CONTROL_84_85_SIZE;

	/* control 86 */
	if ((f54->query.has_query13 == 1) && (f54->query.has_ctrl86 == 1))
		reg_addr += CONTROL_86_SIZE;

	/* control 87 */
	if ((f54->query.has_query13 == 1) && (f54->query.has_ctrl87 == 1))
		reg_addr += CONTROL_87_SIZE;

	/* control 88 */
	if (f54->query.has_ctrl88 == 1) {
		control->reg_88 = kzalloc(sizeof(*(control->reg_88)),
				GFP_KERNEL);
		if (!control->reg_88)
			goto exit_no_mem;
		control->reg_88->address = reg_addr;
		reg_addr += sizeof(control->reg_88->data);
	}

	return 0;

exit_no_mem:
	dev_err(&rmi4_data->i2c_client->dev,
			"%s: Failed to alloc mem for control registers\n",
			__func__);
	return -ENOMEM;
}


int synaptics_rmi4_f54_set_control(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned short ii;
	unsigned char page;
	unsigned char intr_count = 0;
	unsigned char intr_offset;
	struct synaptics_rmi4_fn_desc rmi_fd;

	f54->rmi4_data = rmi4_data;
	f54->fn_ptr->read = rmi4_data->i2c_read;
	f54->fn_ptr->write = rmi4_data->i2c_write;
	f54->fn_ptr->enable = rmi4_data->irq_enable;

	for (page = 0; page < PAGES_TO_SERVICE; page++) {
		for (ii = PDT_START; ii > PDT_END; ii -= PDT_ENTRY_SIZE) {
			ii |= (page << 8);

			retval = f54->fn_ptr->read(rmi4_data,
					ii,
					(unsigned char *)&rmi_fd,
					sizeof(rmi_fd));
			if (retval < 0)
				goto err_out;

			if (!rmi_fd.fn_number)
				break;

			if (rmi_fd.fn_number == SYNAPTICS_RMI4_F54)
				goto f54_found;

			intr_count += (rmi_fd.intr_src_count & MASK_3BIT);
		}
	}

f54_found:
	f54->query_base_addr = rmi_fd.query_base_addr | (page << 8);
	f54->control_base_addr = rmi_fd.ctrl_base_addr | (page << 8);
	f54->data_base_addr = rmi_fd.data_base_addr | (page << 8);
	f54->command_base_addr = rmi_fd.cmd_base_addr | (page << 8);

	dev_info(&rmi4_data->i2c_client->dev,
			"%s: query_base_addr[0x%x] control_base_addr[0x%x] data_base_addr[0x%x] command_base_addr[0x%x]\n",
			__func__, f54->query_base_addr, f54->control_base_addr, f54->data_base_addr, f54->command_base_addr);

	f54->intr_reg_num = (intr_count + 7) / 8;
	if (f54->intr_reg_num != 0)
		f54->intr_reg_num -= 1;

	f54->intr_mask = 0;
	intr_offset = intr_count % 8;
	for (ii = intr_offset;
			ii < ((rmi_fd.intr_src_count & MASK_3BIT) +
				intr_offset);
			ii++) {
		f54->intr_mask |= 1 << ii;
	}

	retval = f54->fn_ptr->read(rmi4_data,
			f54->query_base_addr,
			f54->query.data,
			sizeof(f54->query.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read query registers\n",
				__func__);
		goto err_out;
	}

	retval = synaptics_rmi4_f54_set_ctrl();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to set up control registers\n",
				__func__);
		goto err_out;
	}

	return 0;

err_out:
	return retval;
}

#ifdef FACTORY_MODE
static int synaptics_rmi4_f54_get_report_type(int type)
{
	int retval;
	char buf[3];
	unsigned int patience = 90;

	memset(buf, 0x00, sizeof(buf));
	snprintf(buf, 3, "%u\n", type);
	retval = synaptics_rmi4_f54_report_type_store(NULL, NULL, buf, 2);
	if (retval != 2)
		return 0;

	memset(buf, 0x00, sizeof(buf));
	snprintf(buf, 3, "%u\n", CMD_GET_REPORT);
	retval = synaptics_rmi4_f54_get_report_store(NULL, NULL, buf, 2);
	if (retval != 2)
		return 0;

	do {
		msleep(20);
		if (f54->status == STATUS_IDLE)
			break;
	} while (--patience > 0);

	if ((f54->report_size == 0) || (f54->status != STATUS_IDLE))
		return 0;
	else
		return 1;
}
#endif

static void synaptics_rmi4_f54_status_work(struct work_struct *work)
{
	int retval;
	unsigned char report_index[2];
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	if (f54->status != STATUS_BUSY)
		return;
#if defined(CONFIG_SEC_RUBENS_PROJECT)
	retval = f54->fn_ptr->read(rmi4_data,
			f54->query_base_addr,
			f54->query.data,
			sizeof(f54->query.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read f54 query registers\n",
				__func__);
		retval = -EINVAL;
		goto error_exit;
	}

	f54->rx_assigned = f54->query.num_of_rx_electrodes;
	f54->tx_assigned = f54->query.num_of_tx_electrodes;
#endif
	set_report_size();
	if (f54->report_size == 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Report data size = 0\n",
				__func__);
		retval = -EINVAL;
		goto error_exit;
	}

	if (f54->data_buffer_size < f54->report_size) {
		mutex_lock(&f54->data_mutex);
		if (f54->data_buffer_size)
			kfree(f54->report_data);
		f54->report_data = kzalloc(f54->report_size, GFP_KERNEL);
		if (!f54->report_data) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to alloc mem for data buffer\n",
					__func__);
			f54->data_buffer_size = 0;
			mutex_unlock(&f54->data_mutex);
			retval = -ENOMEM;
			goto error_exit;
		}
		f54->data_buffer_size = f54->report_size;
		mutex_unlock(&f54->data_mutex);
	}

	report_index[0] = 0;
	report_index[1] = 0;

	retval = f54->fn_ptr->write(rmi4_data,
			f54->data_base_addr + DATA_REPORT_INDEX_OFFSET,
			report_index,
			sizeof(report_index));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write report data index\n",
				__func__);
		retval = -EINVAL;
		goto error_exit;
	}

	retval = f54->fn_ptr->read(rmi4_data,
			f54->data_base_addr + DATA_REPORT_DATA_OFFSET,
			f54->report_data,
			f54->report_size);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read report data\n",
				__func__);
		retval = -EINVAL;
		goto error_exit;
	}

	retval = STATUS_IDLE;

#ifdef RAW_HEX
	print_raw_hex_report();
#endif

#ifdef HUMAN_READABLE
	print_image_report();
#endif

error_exit:
	mutex_lock(&f54->status_mutex);
	set_interrupt(false);
	f54->status = retval;
	mutex_unlock(&f54->status_mutex);

	return;
}

static void synaptics_rmi4_f54_set_regs(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count,
		unsigned char page)
{
	unsigned char ii;
	unsigned char intr_offset;

	f54->query_base_addr = fd->query_base_addr | (page << 8);
	f54->control_base_addr = fd->ctrl_base_addr | (page << 8);
	f54->data_base_addr = fd->data_base_addr | (page << 8);
	f54->command_base_addr = fd->cmd_base_addr | (page << 8);

	f54->intr_reg_num = (intr_count + 7) / 8;
	if (f54->intr_reg_num != 0)
		f54->intr_reg_num -= 1;

	f54->intr_mask = 0;
	intr_offset = intr_count % 8;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) +
				intr_offset);
			ii++) {
		f54->intr_mask |= 1 << ii;
	}

	return;
}

#ifdef TOUCHKEY_ENABLE
static ssize_t sec_touchkey_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char threshold;

	threshold = 150;

	return snprintf(buf, PAGE_SIZE, "%03d\n", threshold);
}

static void run_deltacap_read(void)
{
	struct factory_data *data = f54->factory_data;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	short *report_data;

	set_default_result(data);

	if (rmi4_data->touch_stopped || rmi4_data->sensor_sleep) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: [ERROR] Touch is stopped\n",
				__func__);
		snprintf(data->cmd_buff, CMD_RESULT_STR_LEN, "%s", "TSP enter suspend");
		set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));
		data->cmd_state = CMD_STATUS_NOT_APPLICABLE;
		return;
	}

	if (!synaptics_rmi4_f54_get_report_type(CMD_REPORT_TYPE_DELTA)) {
		data->cmd_state = CMD_STATUS_FAIL;
		return;
	}

	report_data = f54->factory_data->delta_data;
	memcpy(report_data, f54->report_data, f54->report_size);

	/* Tablet(default Horizontal) normally designed dummy_tx: 2, dummy_rx: 1
	 * Phone(default Vertical) normally designed dummy_tx: 1, dummy_rx: 2
	 * BUT need to ask Synaptics for right offset value */
	if (data->dummy_tx > data->dummy_rx) {
		data->tkey_delta_offset[1] = (rmi4_data->num_of_tx + data->dummy_tx)
						* (rmi4_data->num_of_rx + data->dummy_rx) - 1;
		data->tkey_delta_offset[0] = data->tkey_delta_offset[1]
						- (rmi4_data->num_of_rx + data->dummy_rx);
	} else {
		data->tkey_delta_offset[1] = (rmi4_data->num_of_tx + data->dummy_tx)
						* (rmi4_data->num_of_rx + data->dummy_rx) - 1;
		data->tkey_delta_offset[0] = data->tkey_delta_offset[1] - 1;
	}

	rmi4_data->touchkey_menu = *(report_data + data->tkey_delta_offset[0]);
	rmi4_data->touchkey_back = *(report_data + data->tkey_delta_offset[1]);

	snprintf(data->cmd_buff, PAGE_SIZE, "%d,%d", rmi4_data->touchkey_menu, rmi4_data->touchkey_back);
	set_cmd_result(data, data->cmd_buff, strlen(data->cmd_buff));

	data->cmd_state = CMD_STATUS_OK;

	return;

}

static ssize_t sec_touchkey_menu_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	run_deltacap_read();

	if ((rmi4_data->touchkey_menu) < 0)
		rmi4_data->touchkey_menu = 0;

	dev_info(&rmi4_data->i2c_client->dev, "%s: %d\n",
			__func__, rmi4_data->touchkey_menu);
	return snprintf(buf, PAGE_SIZE, "%03d\n", rmi4_data->touchkey_menu);
}

static ssize_t sec_touchkey_back_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	run_deltacap_read();

	if ((rmi4_data->touchkey_back) < 0)
		rmi4_data->touchkey_back = 0;

	dev_info(&rmi4_data->i2c_client->dev, "%s: %d\n",
			__func__, rmi4_data->touchkey_back);
	return snprintf(buf, PAGE_SIZE, "%03d\n", rmi4_data->touchkey_back);
}

static ssize_t touchkey_led_control(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	int input;
	int ret;

	ret = sscanf(buf, "%d\n", &input);

	if (ret != 1) {
		printk(KERN_DEBUG "[TouchKey] %s, %d err\n",
			__func__, __LINE__);
		return count;
	}

#ifndef TOUCHKEY_LED_GPIO
	if (input == 1)
		rmi4_data->touchkey_led = true;
	else
		rmi4_data->touchkey_led = false;

	synaptics_tkey_led_vdd_on(rmi4_data, rmi4_data->touchkey_led);
	return count;
#else
	if (input == 1) {
		gpio_direction_output(rmi4_data->dt_data->tkey_led_en , 1);
		rmi4_data->touchkey_led = true;
		dev_err(&rmi4_data->i2c_client->dev,
			"%s:[TouchKey gpio value] %d %d %d on\n", __func__, input,
			gpio_get_value(rmi4_data->dt_data->tkey_led_en),
			rmi4_data->touchkey_led);
		return count;
	} else {
		gpio_direction_output(rmi4_data->dt_data->tkey_led_en, 0);
		rmi4_data->touchkey_led = false;
		dev_err(&rmi4_data->i2c_client->dev,
			"%s:[TouchKey gpio value] %d %d %d off\n", __func__, input,
			gpio_get_value(rmi4_data->dt_data->tkey_led_en),
			rmi4_data->touchkey_led);
		return count;
	}
#endif
}


static ssize_t touchkey_led_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	if(rmi4_data->touchkey_led == 1)
		return snprintf(buf, PAGE_SIZE, "%d\n", rmi4_data->touchkey_led);

	else
		return snprintf(buf, PAGE_SIZE, "%d\n", rmi4_data->touchkey_led);
}

#ifdef TKEY_BOOSTER
static ssize_t boost_level_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;
	int val, retval;

	dev_info(&rmi4_data->i2c_client->dev, "%s\n", __func__);
	sscanf(buf, "%d", &val);

	if (val != 1 && val != 2 && val != 0) {
		dev_info(&rmi4_data->i2c_client->dev,
			"%s: wrong cmd %d\n", __func__, val);
		return count;
	}
	rmi4_data->tkey_dvfs_boost_mode = val;
	dev_info(&rmi4_data->i2c_client->dev,
			"%s: tkey_dvfs_boost_mode = %d\n",
			__func__, rmi4_data->tkey_dvfs_boost_mode);

	if (rmi4_data->tkey_dvfs_boost_mode == DVFS_STAGE_DUAL) {
		rmi4_data->tkey_dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
		dev_info(&rmi4_data->i2c_client->dev,
			"%s: boost_mode DUAL, tkey_dvfs_freq = %d\n",
			__func__, rmi4_data->tkey_dvfs_freq);
	} else if (rmi4_data->tkey_dvfs_boost_mode == DVFS_STAGE_SINGLE) {
		rmi4_data->tkey_dvfs_freq = MIN_TOUCH_LIMIT;
		dev_info(&rmi4_data->i2c_client->dev,
			"%s: boost_mode SINGLE, tkey_dvfs_freq = %d\n",
			__func__, rmi4_data->tkey_dvfs_freq);
	} else if (rmi4_data->tkey_dvfs_boost_mode == DVFS_STAGE_NONE) {
		rmi4_data->tkey_dvfs_freq = -1;
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: booster stop failed(%d).\n",
					__func__, retval);
			rmi4_data->tkey_dvfs_lock_status = false;
		}
	}
	return count;
}
#endif
#endif /* TOUCHKEY_ENABLE */

static void synaptics_rmi5_f55_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii;
	unsigned char rx_electrodes = f54->query.num_of_rx_electrodes;
	unsigned char tx_electrodes = f54->query.num_of_tx_electrodes;

	retval = f54->fn_ptr->read(rmi4_data,
			f55->query_base_addr,
			f55->query.data,
			sizeof(f55->query.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read f55 query registers\n",
				__func__);
		return;
	}

	if (!f55->query.has_sensor_assignment)
		return;

	f55->rx_assignment = kzalloc(rx_electrodes, GFP_KERNEL);
	f55->tx_assignment = kzalloc(tx_electrodes, GFP_KERNEL);

	retval = f54->fn_ptr->read(rmi4_data,
			f55->control_base_addr + SENSOR_RX_MAPPING_OFFSET,
			f55->rx_assignment,
			rx_electrodes);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read f55 rx assignment\n",
				__func__);
		return;
	}

	retval = f54->fn_ptr->read(rmi4_data,
			f55->control_base_addr + SENSOR_TX_MAPPING_OFFSET,
			f55->tx_assignment,
			tx_electrodes);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read f55 tx assignment\n",
				__func__);
		return;
	}

	f54->rx_assigned = 0;
	for (ii = 0; ii < rx_electrodes; ii++) {
		if (f55->rx_assignment[ii] != 0xff)
			f54->rx_assigned++;
	}

	f54->tx_assigned = 0;
	for (ii = 0; ii < tx_electrodes; ii++) {
		if (f55->tx_assignment[ii] != 0xff)
			f54->tx_assigned++;
	}

	return;
}

static void synaptics_rmi4_f55_set_regs(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned char page)
{
	f55 = kzalloc(sizeof(*f55), GFP_KERNEL);
	if (!f55) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for f55\n",
				__func__);
		return;
	}

	f55->query_base_addr = fd->query_base_addr | (page << 8);
	f55->control_base_addr = fd->ctrl_base_addr | (page << 8);
	f55->data_base_addr = fd->data_base_addr | (page << 8);
	f55->command_base_addr = fd->cmd_base_addr | (page << 8);

	return;
}

static void synaptics_rmi4_f54_attn(struct synaptics_rmi4_data *rmi4_data,
		unsigned char intr_mask)
{
	if (!f54)
		return;

	if (f54->intr_mask & intr_mask) {
		queue_delayed_work(f54->status_workqueue,
				&f54->status_work,
				msecs_to_jiffies(STATUS_WORK_INTERVAL));
	}

	return;
}

static int synaptics_rmi4_f54_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned short ii;
	unsigned char page;
	unsigned char intr_count = 0;
	bool f54found = false;
	bool f55found = false;
#ifdef FACTORY_MODE
	unsigned char rx;
	unsigned char tx;
	struct factory_data *factory_data;
#endif
	struct synaptics_rmi4_fn_desc rmi_fd;

	f54 = kzalloc(sizeof(*f54), GFP_KERNEL);
	if (!f54) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for f54\n",
				__func__);
		retval = -ENOMEM;
		goto exit;
	}

	f54->fn_ptr = kzalloc(sizeof(*(f54->fn_ptr)), GFP_KERNEL);
	if (!f54->fn_ptr) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for fn_ptr\n",
				__func__);
		retval = -ENOMEM;
		goto exit_free_f54;
	}

	f54->rmi4_data = rmi4_data;
	f54->fn_ptr->read = rmi4_data->i2c_read;
	f54->fn_ptr->write = rmi4_data->i2c_write;
	f54->fn_ptr->enable = rmi4_data->irq_enable;

	for (page = 0; page < PAGES_TO_SERVICE; page++) {
		for (ii = PDT_START; ii > PDT_END; ii -= PDT_ENTRY_SIZE) {
			ii |= (page << 8);

			retval = f54->fn_ptr->read(rmi4_data,
					ii,
					(unsigned char *)&rmi_fd,
					sizeof(rmi_fd));
			if (retval < 0)
				goto exit_free_mem;

			if (!rmi_fd.fn_number)
				break;

			switch (rmi_fd.fn_number) {
				case SYNAPTICS_RMI4_F54:
					synaptics_rmi4_f54_set_regs(rmi4_data,
							&rmi_fd, intr_count, page);
					f54found = true;
					break;
				case SYNAPTICS_RMI4_F55:
					synaptics_rmi4_f55_set_regs(rmi4_data,
							&rmi_fd, page);
					f55found = true;
					break;
				default:
					break;
			}

			if (f54found && f55found)
				goto pdt_done;

			intr_count += (rmi_fd.intr_src_count & MASK_3BIT);
		}
	}

	if (!f54found) {
		retval = -ENODEV;
		goto exit_free_mem;
	}

pdt_done:
	retval = f54->fn_ptr->read(rmi4_data,
			f54->query_base_addr,
			f54->query.data,
			sizeof(f54->query.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read f54 query registers\n",
				__func__);
		goto exit_free_mem;
	}

	f54->rx_assigned = f54->query.num_of_rx_electrodes;
	f54->tx_assigned = f54->query.num_of_tx_electrodes;

	retval = synaptics_rmi4_f54_set_ctrl();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to set up f54 control registers\n",
				__func__);
		goto exit_free_control;
	}

	if (f55found)
		synaptics_rmi5_f55_init(rmi4_data);

	mutex_init(&f54->status_mutex);
	mutex_init(&f54->data_mutex);
	mutex_init(&f54->control_mutex);

	retval = synaptics_rmi4_f54_set_sysfs();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs entries\n",
				__func__);
		goto exit_sysfs;
	}

#ifdef FACTORY_MODE
	rx = f54->rx_assigned;
	tx = f54->tx_assigned;

	factory_data = kzalloc(sizeof(*factory_data), GFP_KERNEL);
	if (!factory_data) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for factory_data\n",
				__func__);
		retval = -ENOMEM;
		goto exit_factory_data;
	}

	factory_data->rawcap_data = kzalloc(2 * rx * tx, GFP_KERNEL);
	if (!factory_data->rawcap_data) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for rawcap_data\n",
				__func__);
		retval = -ENOMEM;
		goto exit_rawcap_data;
	}

	factory_data->test_data = kzalloc(2 * rx * tx, GFP_KERNEL);
	if (!factory_data->test_data) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for test_data\n",
				__func__);
		retval = -ENOMEM;
		goto exit_test_data;
	}

	factory_data->delta_data = kzalloc(2 * rx * tx, GFP_KERNEL);
	if (!factory_data->delta_data) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for delta_data\n",
				__func__);
		retval = -ENOMEM;
		goto exit_delta_data;
	}

	factory_data->abscap_data = kzalloc(4 * rx * tx, GFP_KERNEL);
	if (!factory_data->abscap_data) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for abscap_data\n",
				__func__);
		retval = -ENOMEM;
		goto exit_abscap_data;
	}
	factory_data->absdelta_data = kzalloc(4 * rx * tx, GFP_KERNEL);
	if (!factory_data->abscap_data) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for abscap_data\n",
				__func__);
		retval = -ENOMEM;
		goto exit_absdelta_data;
	}

	factory_data->trx_short = kzalloc(TREX_DATA_SIZE, GFP_KERNEL);
	if (!factory_data->trx_short) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for trx_short\n",
				__func__);
		retval = -ENOMEM;
		goto exit_trx_short;
	}

	INIT_LIST_HEAD(&factory_data->cmd_list_head);
	for (ii = 0; ii < ARRAY_SIZE(ft_cmds); ii++)
		list_add_tail(&ft_cmds[ii].list, &factory_data->cmd_list_head);

	mutex_init(&factory_data->cmd_lock);
	factory_data->cmd_is_running = false;

	if (!rmi4_data->created_sec_class) {
		factory_data->fac_dev_ts = device_create(sec_class,
				NULL, SEC_CLASS_DEVT_TSP, f54, "tsp");

		retval = IS_ERR(factory_data->fac_dev_ts);
		if (retval) {
			dev_err(&rmi4_data->i2c_client->dev, "%s: Failed to create device for the sysfs\n",
					__func__);
			retval = IS_ERR(factory_data->fac_dev_ts);
			goto exit_cmd_attr;
		}

		rmi4_data->created_sec_class = true;
		retval = sysfs_create_link(&factory_data->fac_dev_ts->kobj,
						&rmi4_data->input_dev->dev.kobj, "input");
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to create input symbolic link\n",
					__func__);
		}
		retval = sysfs_create_group(&factory_data->fac_dev_ts->kobj,
				&cmd_attr_group);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to create sysfs attributes\n",
					__func__);
			goto exit_cmd_sysfs_group;
		}
	}
#ifdef TOUCHKEY_ENABLE
	factory_data->dummy_rx = rx - rmi4_data->num_of_rx;
	factory_data->dummy_tx = tx - rmi4_data->num_of_tx;

	if (!rmi4_data->created_sec_tkey_class) {
		factory_data->fac_dev_tskey = device_create(sec_class,
				NULL, SEC_CLASS_DEVT_TKEY, f54, "sec_touchkey");

		retval = IS_ERR(factory_data->fac_dev_tskey);
		if (retval) {
			dev_err(&rmi4_data->i2c_client->dev, "%s: Failed to create device for the sysfs\n",
					__func__);
			retval = IS_ERR(factory_data->fac_dev_tskey);
			goto exit_tskey_attr;
		}
		rmi4_data->created_sec_tkey_class = true;

		retval = sysfs_create_group(&factory_data->fac_dev_tskey->kobj,
		    &tskey_attr_group);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to create sysfs attributes\n",
					__func__);
			goto exit_tskey_attr_group;
		}
	}
#endif /* TOUCHKEY_ENABLE */

	f54->factory_data = factory_data;
#endif /* FACTORY_MODE */

	f54->status_workqueue =
		create_singlethread_workqueue("f54_status_workqueue");
	INIT_DELAYED_WORK(&f54->status_work,
			synaptics_rmi4_f54_status_work);

#ifdef WATCHDOG_HRTIMER
	/* Watchdog timer to catch unanswered get report commands */
	hrtimer_init(&f54->watchdog, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	f54->watchdog.function = get_report_timeout;

	/* Work function to do actual cleaning up */
	INIT_WORK(&f54->timeout_work, timeout_set_status);
#endif

	f54->status = STATUS_IDLE;

	return 0;

#ifdef FACTORY_MODE

#ifdef TOUCHKEY_ENABLE
exit_tskey_attr_group:
	if (rmi4_data->created_sec_tkey_class) {
		device_destroy(sec_class, SEC_CLASS_DEVT_TKEY);
		rmi4_data->created_sec_tkey_class = false;
	}
exit_tskey_attr:
	sysfs_remove_group(&factory_data->fac_dev_ts->kobj, &cmd_attr_group);
#endif
exit_cmd_sysfs_group:
	if (rmi4_data->created_sec_class) {
		device_destroy(sec_class, SEC_CLASS_DEVT_TSP);
		rmi4_data->created_sec_class = false;
	}
exit_cmd_attr:
	kfree(factory_data->trx_short);
exit_trx_short:
	kfree(factory_data->abscap_data);
exit_absdelta_data:
	kfree(factory_data->absdelta_data);
exit_abscap_data:
	kfree(factory_data->delta_data);
exit_delta_data:
	kfree(factory_data->test_data);
exit_test_data:
	kfree(factory_data->rawcap_data);
exit_rawcap_data:
	kfree(factory_data);
exit_factory_data:
	remove_sysfs();
#endif

exit_sysfs:
	mutex_destroy(&f54->status_mutex);
	mutex_destroy(&f54->data_mutex);
	mutex_destroy(&f54->control_mutex);

	if (f55) {
		kfree(f55->rx_assignment);
		kfree(f55->tx_assignment);
	}

exit_free_control:
	free_control_mem();

exit_free_mem:
	if (f55) {
		kfree(f55);
		f55 = NULL;
	}
	kfree(f54->fn_ptr);

exit_free_f54:
	kfree(f54);
	f54 = NULL;

exit:
	return retval;
}

static void synaptics_rmi4_f54_remove(struct synaptics_rmi4_data *rmi4_data)
{
	if (!f54)
		goto exit;

#ifdef WATCHDOG_HRTIMER
	hrtimer_cancel(&f54->watchdog);
#endif

	cancel_delayed_work_sync(&f54->status_work);
	flush_workqueue(f54->status_workqueue);
	destroy_workqueue(f54->status_workqueue);

#ifdef FACTORY_MODE
#ifdef TOUCHKEY_ENABLE
	sysfs_remove_group(&f54->factory_data->fac_dev_tskey->kobj, &tskey_attr_group);
	if (rmi4_data->created_sec_tkey_class) {
		device_destroy(sec_class, SEC_CLASS_DEVT_TKEY);
		rmi4_data->created_sec_tkey_class = false;
	}
#endif
	sysfs_remove_group(&f54->factory_data->fac_dev_ts->kobj, &cmd_attr_group);
	if (rmi4_data->created_sec_class) {
		device_destroy(sec_class, SEC_CLASS_DEVT_TSP);
		rmi4_data->created_sec_class = false;
	}

	kfree(f54->factory_data->trx_short);
	kfree(f54->factory_data->abscap_data);
	kfree(f54->factory_data->absdelta_data);
	kfree(f54->factory_data->delta_data);
	kfree(f54->factory_data->test_data);
	kfree(f54->factory_data->rawcap_data);
	kfree(f54->factory_data);
#endif

	remove_sysfs();

	free_control_mem();

	mutex_destroy(&f54->status_mutex);
	mutex_destroy(&f54->data_mutex);
	mutex_destroy(&f54->control_mutex);

	if (f55) {
		if (f55->rx_assignment)
			kfree(f55->rx_assignment);
		if (f55->tx_assignment)
			kfree(f55->tx_assignment);

		kfree(f55);
		f55 = NULL;
	}

	if (f54->data_buffer_size)
		kfree(f54->report_data);

	kfree(f54->fn_ptr);
	kfree(f54);
	f54 = NULL;

exit:
	return;
}

int rmi4_f54_module_register(void)
{
	int retval;

	retval = synaptics_rmi4_new_function(RMI_F54,
			synaptics_rmi4_f54_init,
			synaptics_rmi4_f54_remove,
			synaptics_rmi4_f54_attn);

	return retval;
}
