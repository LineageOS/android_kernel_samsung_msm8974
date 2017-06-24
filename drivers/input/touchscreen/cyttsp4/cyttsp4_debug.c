/*
 * cyttsp4_debug.c
 * Cypress TrueTouch(TM) Standard Product V4 Core driver module.
 * For use with Cypress Txx4xx parts.
 * Supported parts include:
 * TMA4XX
 * TMA1036
 *
 * Copyright (C) 2012 Cypress Semiconductor
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * Author: Aleksej Makarov <aleksej.makarov@sonyericsson.com>
 * Modified by: Cypress Semiconductor to add device functions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contact Cypress Semiconductor at www.cypress.com <ttdrivers@cypress.com>
 *
 */

#include "cyttsp4_regs.h"

#define CYTTSP4_DEBUG_NAME "cyttsp4_debug"
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEBUG
enum cyttsp4_monitor_status {
	CY_MNTR_DISABLED,
	CY_MNTR_ENABLED,
};

struct cyttsp4_sensor_monitor {
	enum cyttsp4_monitor_status mntr_status;
	u8 sensor_data[150];		/* operational sensor data */
};

struct cyttsp4_debug_data {
	struct device *dev;
	struct cyttsp4_sysinfo *si;
	uint32_t interrupt_count;
	uint32_t formated_output;
	struct mutex sysfs_lock;
	struct cyttsp4_sensor_monitor monitor;
	u8 pr_buf[CY_MAX_PRBUF_SIZE];
};

static struct cyttsp4_core_commands *cmd;

static inline struct cyttsp4_debug_data *cyttsp4_get_debug_data(
		struct device *dev)
{
	return cyttsp4_get_dynamic_data(dev, CY_MODULE_DEBUG);
}

/*
 * This function provide output of combined xy_mode and xy_data.
 * Required by TTHE.
 */
static void cyttsp4_pr_buf_op_mode(struct device *dev, u8 *pr_buf,
		struct cyttsp4_sysinfo *si, u8 cur_touch)
{
	int i, k;
	const char fmt[] = "%02X ";
	int max = (CY_MAX_PRBUF_SIZE - 1) - sizeof(CY_PR_TRUNCATED);
	int total_size = si->si_ofs.mode_size
			+ (cur_touch * si->si_ofs.tch_rec_size);
	u8 num_btns = si->si_ofs.num_btns;

	pr_buf[0] = 0;
	for (i = k = 0; i < si->si_ofs.mode_size && i < max; i++, k += 3)
		scnprintf(pr_buf + k, CY_MAX_PRBUF_SIZE, fmt, si->xy_mode[i]);

	for (i = 0; i < (cur_touch * si->si_ofs.tch_rec_size) && i < max;
			i++, k += 3)
		scnprintf(pr_buf + k, CY_MAX_PRBUF_SIZE, fmt, si->xy_data[i]);

	if (num_btns) {
		/* print btn diff data for TTHE */
		scnprintf(pr_buf + k, CY_MAX_PRBUF_SIZE, "%s", "=");
		k++;
		for (i = 0; i < (num_btns * si->si_ofs.btn_rec_size) && i < max;
				i++, k += 3)
			scnprintf(pr_buf + k, CY_MAX_PRBUF_SIZE, fmt,
					si->btn_rec_data[i]);
		total_size += num_btns * si->si_ofs.btn_rec_size + 1;
	}
	pr_info("%s=%s%s\n", "cyttsp4_OpModeData", pr_buf,
			total_size <= max ? "" : CY_PR_TRUNCATED);
}

static void cyttsp4_debug_print(struct device *dev, u8 *pr_buf, u8 *sptr,
		int size, const char *data_name)
{
	int i, j;
	int elem_size = sizeof("XX ") - 1;
	int max = (CY_MAX_PRBUF_SIZE - 1) / elem_size;
	int limit = size < max ? size : max;

	if (limit < 0)
		limit = 0;

	pr_buf[0] = 0;
	for (i = j = 0; i < limit; i++, j += elem_size)
		scnprintf(pr_buf + j, CY_MAX_PRBUF_SIZE - j, "%02X ", sptr[i]);

	pr_info("%s[0..%d]=%s%s\n", data_name, size ? size - 1 : 0, pr_buf,
			size <= max ? "" : CY_PR_TRUNCATED);
}

static void cyttsp4_debug_formated(struct device *dev, u8 *pr_buf,
		struct cyttsp4_sysinfo *si, u8 num_cur_rec)
{
	u8 mode_size = si->si_ofs.mode_size;
	u8 rep_len = si->xy_mode[si->si_ofs.rep_ofs];
	u8 tch_rec_size = si->si_ofs.tch_rec_size;
	u8 num_btns = si->si_ofs.num_btns;
	u8 num_btn_regs = (num_btns + CY_NUM_BTN_PER_REG - 1)
			/ CY_NUM_BTN_PER_REG;
	u8 num_btn_tch;
	u8 data_name[] = "touch[99]";
	int max_print_length = 18;
	int i;

	/* xy_mode */
	cyttsp4_debug_print(dev, pr_buf, si->xy_mode, mode_size, "xy_mode");

	/* xy_data */
	if (rep_len > max_print_length) {
		pr_info("xy_data[0..%d]:\n", rep_len);
		for (i = 0; i < rep_len - max_print_length;
				i += max_print_length) {
			cyttsp4_debug_print(dev, pr_buf, si->xy_data + i,
					max_print_length, " ");
		}
		if (rep_len - i)
			cyttsp4_debug_print(dev, pr_buf, si->xy_data + i,
					rep_len - i, " ");
	} else {
		cyttsp4_debug_print(dev, pr_buf, si->xy_data,
				rep_len - si->si_ofs.rep_hdr_size, "xy_data");
	}

	/* touches */
	for (i = 0; i < num_cur_rec; i++) {
		scnprintf(data_name, sizeof(data_name) - 1, "touch[%u]", i);
		cyttsp4_debug_print(dev, pr_buf,
				si->xy_data + (i * tch_rec_size),
				tch_rec_size, data_name);
	}

	/* buttons */
	if (num_btns) {
		num_btn_tch = 0;
		for (i = 0; i < num_btn_regs; i++) {
			if (si->xy_mode[si->si_ofs.rep_ofs + 2 + i]) {
				num_btn_tch++;
				break;
			}
		}
		if (num_btn_tch)
			cyttsp4_debug_print(dev, pr_buf,
					&si->xy_mode[si->si_ofs.rep_ofs + 2],
					num_btn_regs, "button");
	}
}

/* read xy_data for all touches for debug */
static int cyttsp4_xy_worker(struct cyttsp4_debug_data *dd)
{
	struct device *dev = dd->dev;
	struct cyttsp4_sysinfo *si = dd->si;
	u8 tt_stat = si->xy_mode[si->si_ofs.tt_stat_ofs];
	u8 num_cur_rec = GET_NUM_TOUCH_RECORDS(tt_stat);
	uint32_t formated_output;
	int rc;

	mutex_lock(&dd->sysfs_lock);
	dd->interrupt_count++;
	formated_output = dd->formated_output;
	mutex_unlock(&dd->sysfs_lock);

	/* Read command parameters */
	rc = cmd->read(dev, CY_MODE_OPERATIONAL,
			si->si_ofs.cmd_ofs + 1,
			&si->xy_mode[si->si_ofs.cmd_ofs + 1],
			si->si_ofs.rep_ofs - si->si_ofs.cmd_ofs - 1);
	if (rc < 0) {
		dev_err(dev, "%s: read fail on command parameter regs r=%d\n",
				__func__, rc);
	}

	if (si->si_ofs.num_btns > 0) {
		/* read button diff data */
		rc = cmd->read(dev, CY_MODE_OPERATIONAL,
				/*  replace with btn_diff_ofs when that field
				 *  becomes supported in the firmware */
				si->si_ofs.tt_stat_ofs + 1 +
				si->si_ofs.max_tchs * si->si_ofs.tch_rec_size,
				si->btn_rec_data,
				si->si_ofs.num_btns * si->si_ofs.btn_rec_size);
		if (rc < 0) {
			dev_err(dev, "%s: read fail on button regs r=%d\n",
					__func__, rc);
		}
	}

	/* Interrupt */
	pr_info("Interrupt(%u)\n", dd->interrupt_count);

	if (formated_output)
		cyttsp4_debug_formated(dev, dd->pr_buf, si, num_cur_rec);
	else
		/* print data for TTHE */
		cyttsp4_pr_buf_op_mode(dev, dd->pr_buf, si, num_cur_rec);

	if (dd->monitor.mntr_status == CY_MNTR_ENABLED) {
		int offset = (si->si_ofs.max_tchs * si->si_ofs.tch_rec_size)
				+ (si->si_ofs.num_btns
					* si->si_ofs.btn_rec_size)
				+ (si->si_ofs.tt_stat_ofs + 1);
		rc = cmd->read(dev, CY_MODE_OPERATIONAL,
				offset, &(dd->monitor.sensor_data[0]), 150);
		if (rc < 0)
			dev_err(dev, "%s: read fail on sensor monitor regs r=%d\n",
					__func__, rc);
		/* print data for the sensor monitor */
		cyttsp4_debug_print(dev, dd->pr_buf, dd->monitor.sensor_data,
				150, "cyttsp4_sensor_monitor");
	}

	pr_info("\n");

	return 0;
}


static int cyttsp4_debug_op_attention(struct device *dev)
{
	struct cyttsp4_debug_data *dd = cyttsp4_get_debug_data(dev);
	int rc = 0;

	/* core handles handshake */
	rc = cyttsp4_xy_worker(dd);
	if (rc < 0)
		dev_err(dev, "%s: xy_worker error r=%d\n", __func__, rc);

	return rc;
}

static int cyttsp4_debug_cat_attention(struct device *dev)
{
	struct cyttsp4_debug_data *dd = cyttsp4_get_debug_data(dev);
	struct cyttsp4_sysinfo *si = dd->si;
	u8 cat_masked_cmd;

	/* Check for CaT command executed */
	cat_masked_cmd = si->xy_mode[CY_REG_CAT_CMD] & CY_CMD_MASK;
	if (cat_masked_cmd == CY_CMD_CAT_START_SENSOR_DATA_MODE) {
		dev_vdbg(dev, "%s: Sensor data mode enabled\n", __func__);
		dd->monitor.mntr_status = CY_MNTR_ENABLED;
	} else if (cat_masked_cmd == CY_CMD_CAT_STOP_SENSOR_DATA_MODE) {
		dev_vdbg(dev, "%s: Sensor data mode disabled\n", __func__);
		dd->monitor.mntr_status = CY_MNTR_DISABLED;
	}

	return 0;
}

static int cyttsp4_debug_startup_attention(struct device *dev)
{
	struct cyttsp4_debug_data *dd = cyttsp4_get_debug_data(dev);

	dd->monitor.mntr_status = CY_MNTR_DISABLED;
	return 0;
}

static ssize_t cyttsp4_interrupt_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cyttsp4_debug_data *dd = cyttsp4_get_debug_data(dev);
	int val;

	mutex_lock(&dd->sysfs_lock);
	val = dd->interrupt_count;
	mutex_unlock(&dd->sysfs_lock);

	return scnprintf(buf, CY_MAX_PRBUF_SIZE, "Interrupt Count: %d\n", val);
}

static ssize_t cyttsp4_interrupt_count_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4_debug_data *dd = cyttsp4_get_debug_data(dev);
	mutex_lock(&dd->sysfs_lock);
	dd->interrupt_count = 0;
	mutex_unlock(&dd->sysfs_lock);
	return size;
}

static DEVICE_ATTR(int_count, S_IRUSR | S_IWUSR,
	cyttsp4_interrupt_count_show, cyttsp4_interrupt_count_store);

static ssize_t cyttsp4_formated_output_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cyttsp4_debug_data *dd = cyttsp4_get_debug_data(dev);
	int val;

	mutex_lock(&dd->sysfs_lock);
	val = dd->formated_output;
	mutex_unlock(&dd->sysfs_lock);

	return scnprintf(buf, CY_MAX_PRBUF_SIZE,
			"Formated debug output: %x\n", val);
}

static ssize_t cyttsp4_formated_output_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4_debug_data *dd = cyttsp4_get_debug_data(dev);
	unsigned long value;
	int rc;

	rc = kstrtoul(buf, 10, &value);
	if (rc < 0) {
		dev_err(dev, "%s: Invalid value\n", __func__);
		return size;
	}

	/* Expecting only 0 or 1 */
	if (value != 0 && value != 1) {
		dev_err(dev, "%s: Invalid value %lu\n", __func__, value);
		return size;
	}

	mutex_lock(&dd->sysfs_lock);
	dd->formated_output = value;
	mutex_unlock(&dd->sysfs_lock);
	return size;
}

static DEVICE_ATTR(formated_output, S_IRUSR | S_IWUSR,
	cyttsp4_formated_output_show, cyttsp4_formated_output_store);

int cyttsp4_debug_probe(struct device *dev)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp4_debug_data *dd;
	int rc;

	dev_info(dev, "%s: \n",	__func__);

	cmd = cyttsp4_get_commands();
	if (!cmd)
	{
		dev_err(dev, "%s: cmd invalid\n", __func__);
		return -EINVAL;
	}
	
	/* get context and debug print buffers */
	dd = kzalloc(sizeof(*dd), GFP_KERNEL);
	if (!dd) {
		dev_err(dev, "%s: Error, kzalloc\n", __func__);
		rc = -ENOMEM;
		goto cyttsp4_debug_probe_alloc_failed;
	}

	rc = device_create_file(dev, &dev_attr_int_count);
	if (rc) {
		dev_err(dev, "%s: Error, could not create int_count\n",
				__func__);
		goto cyttsp4_debug_probe_create_int_count_failed;
	}

	rc = device_create_file(dev, &dev_attr_formated_output);
	if (rc) {
		dev_err(dev, "%s: Error, could not create formated_output\n",
				__func__);
		goto cyttsp4_debug_probe_create_formated_failed;
	}

	mutex_init(&dd->sysfs_lock);
	dd->dev = dev;
	cd->cyttsp4_dynamic_data[CY_MODULE_DEBUG] = dd;

	dd->si = cmd->request_sysinfo(dev);
	if (!dd->si) {
		dev_err(dev, "%s: Fail get sysinfo pointer from core\n",
				__func__);
		rc = -ENODEV;
		goto cyttsp4_debug_probe_sysinfo_failed;
	}

	rc = cmd->subscribe_attention(dev, CY_ATTEN_IRQ, CY_MODULE_DEBUG,
		cyttsp4_debug_op_attention, CY_MODE_OPERATIONAL);
	if (rc < 0) {
		dev_err(dev, "%s: Error, could not subscribe Operating mode attention cb\n",
				__func__);
		goto cyttsp4_debug_probe_subscribe_op_failed;
	}

	rc = cmd->subscribe_attention(dev, CY_ATTEN_IRQ, CY_MODULE_DEBUG,
		cyttsp4_debug_cat_attention, CY_MODE_CAT);
	if (rc < 0) {
		dev_err(dev, "%s: Error, could not subscribe CaT mode attention cb\n",
				__func__);
		goto cyttsp4_debug_probe_subscribe_cat_failed;
	}

	rc = cmd->subscribe_attention(dev, CY_ATTEN_STARTUP, CY_MODULE_DEBUG,
		cyttsp4_debug_startup_attention, 0);
	if (rc < 0) {
		dev_err(dev, "%s: Error, could not subscribe startup attention cb\n",
				__func__);
		goto cyttsp4_debug_probe_subscribe_startup_failed;
	}
	return 0;

cyttsp4_debug_probe_subscribe_startup_failed:
	cmd->unsubscribe_attention(dev, CY_ATTEN_IRQ, CY_MODULE_DEBUG,
		cyttsp4_debug_cat_attention, CY_MODE_CAT);
cyttsp4_debug_probe_subscribe_cat_failed:
	cmd->unsubscribe_attention(dev, CY_ATTEN_IRQ, CY_MODULE_DEBUG,
		cyttsp4_debug_op_attention, CY_MODE_OPERATIONAL);
cyttsp4_debug_probe_subscribe_op_failed:
cyttsp4_debug_probe_sysinfo_failed:
	cd->cyttsp4_dynamic_data[CY_MODULE_DEBUG] = NULL;
	device_remove_file(dev, &dev_attr_formated_output);
cyttsp4_debug_probe_create_formated_failed:
	device_remove_file(dev, &dev_attr_int_count);
cyttsp4_debug_probe_create_int_count_failed:
	kfree(dd);
cyttsp4_debug_probe_alloc_failed:
	dev_err(dev, "%s failed.\n", __func__);
	return rc;
}

int cyttsp4_debug_release(struct device *dev)
{
	struct cyttsp4_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp4_debug_data *dd = cyttsp4_get_debug_data(dev);

	/* Unsubscribe from attentions */
	cmd->unsubscribe_attention(dev, CY_ATTEN_IRQ, CY_MODULE_DEBUG,
		cyttsp4_debug_op_attention, CY_MODE_OPERATIONAL);
	cmd->unsubscribe_attention(dev, CY_ATTEN_IRQ, CY_MODULE_DEBUG,
		cyttsp4_debug_cat_attention, CY_MODE_CAT);
	cmd->unsubscribe_attention(dev, CY_ATTEN_STARTUP, CY_MODULE_DEBUG,
		cyttsp4_debug_startup_attention, 0);

	device_remove_file(dev, &dev_attr_int_count);
	device_remove_file(dev, &dev_attr_formated_output);
	cd->cyttsp4_dynamic_data[CY_MODULE_DEBUG] = NULL;
	kfree(dd);

	return 0;
}
#endif