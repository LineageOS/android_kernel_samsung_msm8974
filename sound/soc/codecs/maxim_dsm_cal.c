/*
 * maxim_dsm_cal.c -- Module for Rdc calibration
 *
 * Copyright 2014 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/power_supply.h>
#include <sound/maxim_dsm.h>
#include <sound/maxim_dsm_cal.h>

#define DEBUG_MAXIM_DSM_CAL
#ifdef DEBUG_MAXIM_DSM_CAL
#define dbg_maxdsm(format, args...)	\
pr_info("[MAXIM_DSM_CAL] %s: " format "\n", __func__, ## args)
#else
#define dbg_maxdsm(format, args...)
#endif /* DEBUG_MAXIM_DSM_CAL */

struct class *g_class;

struct maxim_dsm_cal *g_mdc;
static int maxdsm_cal_read_file(char *filename, char *data, size_t size)
{
	struct file *cal_filp;
	mm_segment_t old_fs = get_fs();
	int ret;

	set_fs(KERNEL_DS);
	cal_filp = filp_open(filename, O_RDONLY, 0660);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: there is no dsm_cal file\n", __func__);
		set_fs(old_fs);
		ret = 0;
		return ret;
	}
	ret = cal_filp->f_op->read(cal_filp, data, size, &cal_filp->f_pos);
	if (ret != size) {
		pr_err("%s: can't read dsm calibration value to file\n",
				__func__);
		ret = -EIO;
	}
	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int maxdsm_cal_write_file(char *filename, char *data, size_t size)
{
	struct file *cal_filp;
	mm_segment_t old_fs = get_fs();
	int ret;

	set_fs(KERNEL_DS);
	cal_filp = filp_open(filename,
			O_CREAT | O_TRUNC | O_WRONLY, 0660);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);
		return ret;
	}
	ret = cal_filp->f_op->write(cal_filp, data, size, &cal_filp->f_pos);
	if (ret != size) {
		pr_err("%s: can't write dsm calibration value to file\n",
				__func__);
		ret = -EIO;
	}
	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

struct regmap *maxdsm_cal_set_regmap(
		struct regmap *regmap)
{
	g_mdc->regmap = regmap;
#if defined(CONFIG_SND_SOC_MAXIM_DSM) && defined(USE_DSM_LOG)
	maxdsm_set_regmap(g_mdc->regmap);
#endif /* CONFIG_SND_SOC_MAXIM_DSM && USE_DSM_LOG */
	return g_mdc->regmap;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_set_regmap);

int maxdsm_cal_set_temp(uint32_t value)
{
	char data[12];
	int ret;

	memset(data, 0x00, sizeof(data));
	sprintf(data, "%x", value);
	ret = maxdsm_cal_write_file(FILEPATH_TEMP_CAL,
			data, sizeof(data));
	g_mdc->values.temp = ret < 0 ? ret : value;

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_set_temp);

int maxdsm_cal_get_temp(uint32_t *value)
{
	char data[12];
	int ret;

	memset(data, 0x00, sizeof(data));
	ret = maxdsm_cal_read_file(FILEPATH_TEMP_CAL,
			data, sizeof(data));
	if (ret < 0)
		g_mdc->values.temp = ret;
	else
		ret = kstrtos32(data, 16, value);

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_get_temp);

int maxdsm_cal_set_rdc(uint32_t value)
{
	char data[12];
	int ret;

	memset(data, 0x00, sizeof(data));
	sprintf(data, "%x", value);
	ret = maxdsm_cal_write_file(FILEPATH_RDC_CAL,
			data, sizeof(data));
	g_mdc->values.rdc = ret < 0 ? ret : value;

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_set_rdc);

int maxdsm_cal_get_rdc(uint32_t *value)
{
	char data[12];
	int ret;

	memset(data, 0x00, sizeof(data));
	ret = maxdsm_cal_read_file(FILEPATH_RDC_CAL,
			data, sizeof(data));
	if (ret < 0)
		g_mdc->values.rdc = ret;
	else
		ret = kstrtos32(data, 16, value);

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_get_rdc);

static int maxdsm_cal_regmap_write(struct regmap *regmap,
		unsigned int reg,
		unsigned int val)
{
	return regmap ?
		regmap_write(g_mdc->regmap, reg, val) : -ENXIO;
}

static int maxdsm_cal_regmap_read(struct regmap *regmap,
		unsigned int reg,
		unsigned int *val)
{
	return regmap ?
		regmap_read(g_mdc->regmap, reg, val) : -ENXIO;
}

static void maxdsm_cal_start_calibration(
		struct maxim_dsm_cal *mdc)
{
#ifdef CONFIG_SND_SOC_MAXIM_DSM
	mdc->platform_type = maxdsm_get_platform_type();
#else
	mdc->platform_type = 0;
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
	dbg_maxdsm("platform_type=%d", mdc->platform_type);

	switch (mdc->platform_type) {
	case PLATFORM_TYPE_A:
		maxdsm_cal_regmap_read(mdc->regmap,
				ADDR_FEATURE_ENABLE,
				&mdc->info.feature_en);
		maxdsm_cal_regmap_write(mdc->regmap,
				ADDR_FEATURE_ENABLE,
				0x3F);
		break;
	case PLATFORM_TYPE_B:
#ifdef CONFIG_SND_SOC_MAXIM_DSM
		maxdsm_set_feature_en(1);
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
		break;
	default:
		break;
	}

	mdc->info.previous_jiffies = jiffies;
}

static uint32_t maxdsm_cal_read_dcresistance(
		struct maxim_dsm_cal *mdc)
{
	uint32_t dcresistance = 0;

	switch (mdc->platform_type) {
	case PLATFORM_TYPE_A:
		maxdsm_cal_regmap_read(mdc->regmap,
				ADDR_RDC,
				&dcresistance);
		break;
	case PLATFORM_TYPE_B:
#ifdef CONFIG_SND_SOC_MAXIM_DSM
		dcresistance = maxdsm_get_dcresistance();
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
		break;
	default:
		break;
	}

	return dcresistance;
}

static void maxdsm_cal_end_calibration(
		struct maxim_dsm_cal *mdc)
{
	switch (mdc->platform_type) {
	case PLATFORM_TYPE_A:
		maxdsm_cal_regmap_write(mdc->regmap,
				ADDR_FEATURE_ENABLE,
				mdc->info.feature_en);
		break;
	case PLATFORM_TYPE_B:
#ifdef CONFIG_SND_SOC_MAXIM_DSM
		maxdsm_set_rdc_temp(mdc->values.rdc,
				(uint32_t)mdc->values.temp / 10);
		maxdsm_set_feature_en(0);
#endif /* CONFIG_SND_SOC_MAXIM_DSM */
		break;
	default:
		break;
	}
}

static int maxdsm_cal_get_temp_from_power_supply(void)
{
	union power_supply_propval value = {0,};
	struct power_supply *psy;
	int temperature;

	psy = power_supply_get_by_name("battery");
	if (!psy) {
		pr_err("%s: Failed to get psy\n", __func__);
		temperature = 23 * 10;
	} else {
		psy->get_property(psy, POWER_SUPPLY_PROP_TEMP, &value);
		temperature = value.intval;
	}
	dbg_maxdsm("temperature=%d", temperature);

	return temperature;
}

static void maxdsm_cal_completed(struct maxim_dsm_cal *mdc)
{
	char rdc[12] = {0,};
	char temp[12] = {0,};
	int ret;

	/* We try to get ambient temp by using power supply core */
	mdc->values.temp = maxdsm_cal_get_temp_from_power_supply();

	sprintf(rdc, "%x", mdc->values.rdc);
	sprintf(temp, "%x",
			mdc->values.temp < 0 ? 23 * 10 : mdc->values.temp);

	ret = maxdsm_cal_write_file(
			FILEPATH_TEMP_CAL, temp, sizeof(temp));
	if (ret < 0)
		mdc->values.temp = ret;

	ret = maxdsm_cal_write_file(
			FILEPATH_RDC_CAL, rdc, sizeof(rdc));
	if (ret < 0)
		mdc->values.rdc = ret;

	maxdsm_cal_end_calibration(mdc);

	mdc->values.status = 0;

	dbg_maxdsm("temp=%d rdc=%d", mdc->values.temp, mdc->values.rdc);
}

static void maxdsm_cal_work(struct work_struct *work)
{
	struct maxim_dsm_cal *mdc;
	unsigned int dcresistance = 0;
	unsigned long diff;

	mdc = container_of(work, struct maxim_dsm_cal, work.work);

	mutex_lock(&mdc->mutex);

	dcresistance = maxdsm_cal_read_dcresistance(mdc);
	if (!(mdc->info.min > dcresistance
				|| mdc->info.max < dcresistance) &&
			((mdc->info.duration - mdc->info.remaining)
			> mdc->info.ignored_t)) {
		mdc->values.avg += dcresistance;
		mdc->values.count++;
	}

	diff = jiffies - mdc->info.previous_jiffies;
	mdc->info.remaining
		-= jiffies_to_msecs(diff);

	dbg_maxdsm("dcresistance=%d remaining=%d duration=%d",
			dcresistance,
			mdc->info.remaining,
			mdc->info.duration);

	if (mdc->info.remaining > 0
			&& mdc->values.status) {
		mdc->info.previous_jiffies = jiffies;
		queue_delayed_work(mdc->wq,
				&mdc->work,
				msecs_to_jiffies(mdc->info.interval));
	} else {
		mdc->values.count > 0 ?
			do_div(mdc->values.avg, mdc->values.count) : 0;
		mdc->values.rdc = mdc->values.avg;
		maxdsm_cal_completed(mdc);
	}

	mutex_unlock(&mdc->mutex);
}

static void maxdsm_cal_check(
		struct maxim_dsm_cal *mdc, int action)
{
	if (delayed_work_pending(&mdc->work))
		cancel_delayed_work(&mdc->work);

	if (action) {
		mdc->info.remaining = mdc->info.duration;
		mdc->values.count = mdc->values.avg = 0;
		maxdsm_cal_start_calibration(mdc);
		queue_delayed_work(mdc->wq,
				&mdc->work,
				1);
	}
}

static ssize_t maxdsm_cal_min_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", g_mdc->info.min);
}

static ssize_t maxdsm_cal_min_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info.min))
		dev_err(dev,
			"%s: Failed converting from str to u32.\n", __func__);
	return size;
}
static DEVICE_ATTR(min, S_IRUGO | S_IWUSR | S_IWGRP,
		maxdsm_cal_min_show, maxdsm_cal_min_store);

static ssize_t maxdsm_cal_max_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", g_mdc->info.max);
}

static ssize_t maxdsm_cal_max_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info.max))
		dev_err(dev,
			"%s: Failed converting from str to u32.\n", __func__);
	return size;
}
static DEVICE_ATTR(max, S_IRUGO | S_IWUSR | S_IWGRP,
		maxdsm_cal_max_show, maxdsm_cal_max_store);

static ssize_t maxdsm_cal_duration_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", g_mdc->info.duration);
}

static ssize_t maxdsm_cal_duration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info.duration))
		dev_err(dev,
			"%s: Failed converting from str to u32.\n", __func__);
	return size;
}
static DEVICE_ATTR(duration, S_IRUGO | S_IWUSR | S_IWGRP,
		maxdsm_cal_duration_show, maxdsm_cal_duration_store);

static ssize_t maxdsm_cal_rdc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char rdc[12] = {0,};
	int ret;

	if (g_mdc->values.rdc == 0xFFFFFFFF) {
		ret = maxdsm_cal_read_file(
				FILEPATH_RDC_CAL, rdc, sizeof(rdc));
		if (ret < 0)
			g_mdc->values.rdc = ret;
		else
			ret = kstrtos32(rdc, 16, &g_mdc->values.rdc);
	}

	return sprintf(buf, "%x", g_mdc->values.rdc);
}

static ssize_t maxdsm_cal_rdc_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	char rdc[12] = {0,};
	int ret;

	if (kstrtos32(buf, 0, &g_mdc->values.rdc))
		dev_err(dev,
			"%s: Failed converting from str to u32.\n", __func__);
	else {
		sprintf(rdc, "%x", g_mdc->values.rdc);
		ret = maxdsm_cal_write_file(
				FILEPATH_RDC_CAL, rdc, sizeof(rdc));
		if (ret < 0)
			g_mdc->values.rdc = ret;
	}

	return size;
}
static DEVICE_ATTR(rdc, S_IRUGO | S_IWUSR | S_IWGRP,
		maxdsm_cal_rdc_show, maxdsm_cal_rdc_store);

static ssize_t maxdsm_cal_temp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[12] = {0,};
	int ret;

	if (g_mdc->values.temp == 0xFFFFFFFF) {
		ret = maxdsm_cal_read_file(
				FILEPATH_TEMP_CAL, temp, sizeof(temp));
		if (ret < 0)
			g_mdc->values.temp = ret;
		else
			ret = kstrtos32(temp, 16, &g_mdc->values.temp);
	}

	return sprintf(buf, "%x", g_mdc->values.temp);
}

static ssize_t maxdsm_cal_temp_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	char temp[12] = {0,};
	int ret;

	if (kstrtos32(buf, 0, &g_mdc->values.temp))
		dev_err(dev,
			"%s: Failed converting from str to u32.\n", __func__);
	else {
		sprintf(temp, "%x", g_mdc->values.temp);
		ret = maxdsm_cal_write_file(
				FILEPATH_TEMP_CAL, temp, sizeof(temp));
		if (ret < 0)
			g_mdc->values.temp = ret;
	}

	return size;
}
static DEVICE_ATTR(temp, S_IRUGO | S_IWUSR | S_IWGRP,
		maxdsm_cal_temp_show, maxdsm_cal_temp_store);

static ssize_t maxdsm_cal_interval_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", g_mdc->info.interval);
}

static ssize_t maxdsm_cal_interval_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info.interval))
		dev_err(dev,
			"%s: Failed converting from str to u32.\n", __func__);
	return size;
}
static DEVICE_ATTR(interval, S_IRUGO | S_IWUSR | S_IWGRP,
		maxdsm_cal_interval_show, maxdsm_cal_interval_store);

static ssize_t maxdsm_cal_ignored_t_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", g_mdc->info.ignored_t);
}

static ssize_t maxdsm_cal_ignored_t_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (kstrtou32(buf, 0, &g_mdc->info.ignored_t))
		dev_err(dev,
			"%s: Failed converting from str to u32.\n", __func__);
	return size;
}
static DEVICE_ATTR(ignored_t, S_IRUGO | S_IWUSR | S_IWGRP,
		maxdsm_cal_ignored_t_show, maxdsm_cal_ignored_t_store);

static ssize_t maxdsm_cal_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n",
			g_mdc->values.status ? "Enabled" : "Disabled");
}

static ssize_t maxdsm_cal_status_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int status = 0;

	if (!kstrtou32(buf, 0, &status)) {
		if (status == g_mdc->values.status) {
			dbg_maxdsm("Already run. It will be ignored.");
		} else {
			mutex_lock(&g_mdc->mutex);
			g_mdc->values.status = status;
			mutex_unlock(&g_mdc->mutex);
			if (g_mdc->values.status)
				maxdsm_cal_check(g_mdc, 1);
			else
				maxdsm_cal_check(g_mdc, 0);
		}
	}

	return size;
}
static DEVICE_ATTR(status, S_IRUGO | S_IWUSR | S_IWGRP,
	maxdsm_cal_status_show, maxdsm_cal_status_store);

static struct attribute *maxdsm_cal_attr[] = {
	&dev_attr_min.attr,
	&dev_attr_max.attr,
	&dev_attr_duration.attr,
	&dev_attr_rdc.attr,
	&dev_attr_temp.attr,
	&dev_attr_interval.attr,
	&dev_attr_ignored_t.attr,
	&dev_attr_status.attr,
	NULL,
};

static struct attribute_group maxdsm_cal_attr_grp = {
	.attrs = maxdsm_cal_attr,
};

static int __init maxdsm_cal_init(void)
{
	struct maxim_dsm_cal *mdc;
	int ret = 0;

	g_mdc = kzalloc(sizeof(struct maxim_dsm_cal), GFP_KERNEL);
	if (g_mdc == NULL)
		return -ENOMEM;
	mdc = g_mdc;

	mdc->wq = create_singlethread_workqueue(WQ_NAME);
	if (mdc->wq == NULL) {
		kfree(g_mdc);
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&g_mdc->work, maxdsm_cal_work);
	mutex_init(&g_mdc->mutex);

	mdc->info.min = 0;
	mdc->info.max = 0xFFFFFFFF;
	mdc->info.duration = 1500; /* 1.5 secs */
	mdc->info.remaining = mdc->info.duration;
	mdc->info.interval = 100;
	mdc->info.ignored_t = 1000;
	mdc->values.rdc = 0xFFFFFFFF;
	mdc->values.temp = 0xFFFFFFFF;
	mdc->platform_type = 0xFFFFFFFF;

	if (!g_class)
		g_class = class_create(THIS_MODULE, DSM_NAME);
	mdc->class = g_class;
	if (mdc->class) {
		mdc->dev = device_create(mdc->class, NULL, 1, NULL, CLASS_NAME);
		if (!IS_ERR(mdc->dev)) {
			if (sysfs_create_group(&mdc->dev->kobj,
						&maxdsm_cal_attr_grp))
				dbg_maxdsm(
						"Failed to create sysfs group. ret=%d",
						ret);
		}
	}
	dbg_maxdsm("g_class=%p %p", g_class, mdc->class);

	dbg_maxdsm("Completed initialization");

	return ret;
}
module_init(maxdsm_cal_init);

static void __exit maxdsm_cal_exit(void)
{
	kfree(g_mdc);
}
module_exit(maxdsm_cal_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_SUPPORTED_DEVICE(DRIVER_SUPPORTED);
MODULE_LICENSE("GPL");
