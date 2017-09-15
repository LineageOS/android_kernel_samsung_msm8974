#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/semaphore.h>
#include <linux/suspend.h>
#include <linux/wakelock.h>
#include <linux/sensor/inv_sensors.h>

#include "../../iio.h"
#include "../../buffer.h"
#include "../../trigger.h"
#include "../../kfifo_buf.h"
#include "../../sysfs.h"

#include "inv_sensors_common.h"
#include "inv_sensors_control.h"
#include "inv_sensors_sm.h"
#include "inv_sensors_dts.h"
#include "inv_sensors_buffer.h"

#if defined(CONFIG_SENSORS)
#include <linux/sensor/sensors_core.h>

#define INVSENS_ACCEL_CAL_FILENAME	"/efs/calibration_data"
#define MODEL_NAME	"MPU6515"
#define VENDOR_NAME	"INVENSENSE"
#endif

#define INVSENS_SELFTEST_BUF_LEN 256
#define INVSENS_IIO_BUFFER_DATUM 240

struct invsens_iio_data {
	struct iio_dev *indio_dev;

	int irq;
	const char *device_name;
	/*log level*/
	u32 log_level;
	struct invsens_i2c_t i2c_handler;
	struct semaphore sema;

	struct invsens_platform_data_t *platform_data;
	struct invsens_sm_data_t sm_data;

	/*variables for factory mode*/
	struct delayed_work fso_work;
	bool fso_enabled;

	short accel_cache[INVSENS_AXIS_NUM];
	short gyro_cache[INVSENS_AXIS_NUM];

	bool accel_is_calibrated;
	short accel_bias[INVSENS_AXIS_NUM];

	bool motion_alert_is_occured;
	bool motion_alert_factory_mode;
	unsigned long motion_alert_start_time;
	struct wake_lock motion_alert_wake_lock;

	bool irq_is_disabled;

#ifdef CONFIG_SENSORS
	struct device *gyro_sensor_device;
	struct device *accel_sensor_device;
#endif
};

static struct invsens_data_list_t data_list;


static int invsens_store_accel_cal(const short accel_bias[])
{
	int res = SM_SUCCESS;
	struct file *fp;
	mm_segment_t old_fs;
	loff_t pos = 0;

	INV_DBG_FUNC_NAME;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	/* open file to write */
	fp = filp_open(INVSENS_ACCEL_CAL_FILENAME, O_WRONLY|O_CREAT,
		S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(fp)) {
		INVSENS_LOGE("%s: open file error\n", __func__);
		res = -1;
		goto exit;
	}

	/* Write buf to file */
	vfs_write(fp,
		(void *)accel_bias, sizeof(short) * INVSENS_AXIS_NUM, &pos);

exit:

	/* close file before return */
	if (fp)
		filp_close(fp, NULL);
	/* restore previous address limit */
	set_fs(old_fs);

	return res;
}

static int invsens_load_accel_cal(short accel_bias[])
{
	int res = SM_SUCCESS;
	struct file *fp = NULL;
	mm_segment_t old_fs;

	INV_DBG_FUNC_NAME;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	/* open file to read */
	fp = filp_open(INVSENS_ACCEL_CAL_FILENAME, O_RDONLY,
		S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(fp)) {
		INVSENS_LOGE("invsens_load_accel_cal : fp = %d\n",
			(int) -PTR_ERR(fp));
		res = -1;
		fp = NULL;
		goto exit;
	}

	/* Write buf to file */
	vfs_read(fp, (void *)accel_bias,
			sizeof(short) * INVSENS_AXIS_NUM, &fp->f_pos);

	INVSENS_LOGD("accel bias : %d %d %d res=%d\n", accel_bias[0],
		accel_bias[1], accel_bias[2], res);

exit:
	/* close file before return */
	if (fp)
		filp_close(fp, NULL);

	/* restore previous address limit */
	set_fs(old_fs);

	return res;
}

static int invsens_execute_ccfg(struct invsens_iio_data *iio_data,
	struct invsens_control_data_t *cmd_data)
{
	int res = SM_SUCCESS;
	struct invsens_ioctl_param_t ioctl_data;

	INV_DBG_FUNC_NAME;

	if (cmd_data->config == INV_CCFG_WDGB) {
		ioctl_data.wdgb.bias[0] = cmd_data->params[0];
		ioctl_data.wdgb.bias[1] = cmd_data->params[1];
		ioctl_data.wdgb.bias[2] = cmd_data->params[2];
		res = invsens_sm_ioctl(&iio_data->sm_data,
			INV_IOCTL_SET_GYRO_DMP_BIAS, 0, &ioctl_data);
	} else if (cmd_data->config == INV_CCFG_WDAB) {
		ioctl_data.wdab.bias[0] = cmd_data->params[0];
		ioctl_data.wdab.bias[1] = cmd_data->params[1];
		ioctl_data.wdab.bias[2] = cmd_data->params[2];
		res = invsens_sm_ioctl(&iio_data->sm_data,
			INV_IOCTL_SET_ACCEL_DMP_BIAS, 0, &ioctl_data);
	}

	return res;
}

/**
	command calback
*/
static int invsens_cmd_callback(void *user_data,
	struct invsens_control_data_t *cmd_data)
{
	int result = 0;
	struct invsens_iio_data *iio_data =
	    (struct invsens_iio_data *) user_data;

	INV_DBG_FUNC_NAME;

	switch (cmd_data->control_type) {
	case INV_CONTROL_FLUSH:
		break;

	case INV_CONTROL_SYNC:
		down(&iio_data->sema);
		result = invsens_sm_sync(&iio_data->sm_data, cmd_data->key);
		up(&iio_data->sema);

		/*load bias from calibration file*/
		invsens_load_accel_cal(iio_data->accel_bias);
		if (!iio_data->accel_bias[0]
			&& !iio_data->accel_bias[1]
			&& !iio_data->accel_bias[2])
			iio_data->accel_is_calibrated = false;
		else
			iio_data->accel_is_calibrated = true;

		break;

	case INV_CONTROL_ENABLE_SENSOR:
		down(&iio_data->sema);
		result =
		    invsens_sm_enable_sensor(&iio_data->sm_data,
			     cmd_data->func,
			     (cmd_data->delay == 0) ? false : true,
			     cmd_data->delay);
		up(&iio_data->sema);
		break;

	case INV_CONTROL_BATCH:
		down(&iio_data->sema);
		result =
		    invsens_sm_batch(&iio_data->sm_data, cmd_data->func,
				     cmd_data->delay, cmd_data->timeout);
		up(&iio_data->sema);
		break;
	case INV_CONTROL_CONFIG:
		down(&iio_data->sema);
		result = invsens_execute_ccfg(iio_data, cmd_data);
		up(&iio_data->sema);
		break;
	default:
		break;

	}
	return 0;
}

static ssize_t invsens_control_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int result = 0;
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct invsens_iio_data *iio_data = iio_priv(indio_dev);

	pr_info("[INV]%s: %s\n", __func__, buf);

	result = invsens_parse_control(buf, iio_data, invsens_cmd_callback);

	return strlen(buf);
}

static ssize_t invsens_log_level_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	return sprintf(buf, "%d\n", invsens_get_log_mask());
}

static ssize_t invsens_log_level_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int result = 0;
	long log_mask;


	result = kstrtol(buf, 0, &log_mask);

	invsens_set_log_mask((u32)log_mask);

	return strlen(buf);
}

static ssize_t invsens_reg_dump_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct invsens_iio_data *iio_data = iio_priv(indio_dev);
	u16 i;

	char tmp[5] = { 0 };
	u8 d;
	down(&iio_data->sema);
	for (i = 0; i < 0x80; i++) {
		if (i == 0x74)
			d = 0;
		else
			inv_i2c_read(&iio_data->i2c_handler,
				iio_data->sm_data.board_cfg.i2c_addr,
				i, 1, &d);

		sprintf(tmp, "%02X,", d);
		strcat(buf, tmp);
		if (!((i + 1) % 16))
			strcat(buf, "\n");
	}
	up(&iio_data->sema);
	return strlen(buf);
}

static ssize_t invsens_swst_gyro_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct invsens_iio_data *iio_data = iio_priv(indio_dev);
	struct invsens_ioctl_param_t ioctl_data;
	int res = 0;

	down(&iio_data->sema);
	res = invsens_sm_ioctl(&iio_data->sm_data,
		INV_IOCTL_GYRO_SW_SELFTEST, 0, (void *) &ioctl_data);
	up(&iio_data->sema);

	if (res == SM_IOCTL_HANDLED)
		sprintf(buf, "%d,%ld,%ld,%ld,%ld,%ld,%ld,%d\n",
			ioctl_data.swst_gyro.result,
			ioctl_data.swst_gyro.bias[0],
			ioctl_data.swst_gyro.bias[1],
			ioctl_data.swst_gyro.bias[2],
			ioctl_data.swst_gyro.rms[0],
			ioctl_data.swst_gyro.rms[1],
			ioctl_data.swst_gyro.rms[2],
			ioctl_data.swst_gyro.packet_cnt);
	else
		sprintf(buf, "0");

	return strlen(buf);
}

static ssize_t invsens_hwst_accel_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct invsens_iio_data *iio_data = iio_priv(indio_dev);
	struct invsens_ioctl_param_t ioctl_data;
	int res = 0;

	down(&iio_data->sema);
	res = invsens_sm_ioctl(&iio_data->sm_data,
		INV_IOCTL_ACCEL_HW_SELFTEST, 0, (void *) &ioctl_data);

	up(&iio_data->sema);

	if (res == SM_IOCTL_HANDLED)
		sprintf(buf, "%d,%d,%d,%d\n",
			ioctl_data.hwst_accel.result,
			ioctl_data.hwst_accel.ratio[0],
			ioctl_data.hwst_accel.ratio[1],
			ioctl_data.hwst_accel.ratio[2]);
	else
		sprintf(buf, "0");

	return strlen(buf);
}

static ssize_t invsens_hwst_gyro_show(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct invsens_iio_data *iio_data = iio_priv(indio_dev);
	struct invsens_ioctl_param_t ioctl_data;
	int res = 0;

	down(&iio_data->sema);
	res = invsens_sm_ioctl(&iio_data->sm_data,
		INV_IOCTL_GYRO_HW_SELFTEST, 0, (void *) &ioctl_data);
	up(&iio_data->sema);


	if (res == SM_IOCTL_HANDLED)
		sprintf(buf, "%d,%d,%d,%d\n",
			ioctl_data.hwst_gyro.result,
			ioctl_data.hwst_gyro.ratio[0],
			ioctl_data.hwst_gyro.ratio[1],
			ioctl_data.hwst_gyro.ratio[2]);
	else
		sprintf(buf, "0");

	return strlen(buf);
}

static ssize_t invsens_swst_compass_show(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct invsens_iio_data *iio_data = iio_priv(indio_dev);
	struct invsens_ioctl_param_t ioctl_data;
	int res = 0, value = 0;

	down(&iio_data->sema);

	res = invsens_sm_ioctl(&iio_data->sm_data,
		INV_IOCTL_COMPASS_SELFTEST, 0, (void *) &ioctl_data);
	if (res == SM_IOCTL_HANDLED)
		value = 1;

	up(&iio_data->sema);

	return sprintf(buf, "%d\n", value);
}

static ssize_t invsens_temperature_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct invsens_iio_data *iio_data = dev_get_drvdata(dev);
	int res;
	struct invsens_ioctl_param_t ioctl_data;

	down(&iio_data->sema);
	res = invsens_sm_ioctl(&iio_data->sm_data,
		INV_IOCTL_GET_TEMP, 0, (void *) &ioctl_data);

	up(&iio_data->sema);

	if (res == SM_IOCTL_HANDLED)
		return snprintf(buf, PAGE_SIZE, "%d\n",
			ioctl_data.temperature.temp);
	else
		return snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t invsens_gyro_is_on_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct invsens_iio_data *iio_data = dev_get_drvdata(dev);
	bool is_on;

	down(&iio_data->sema);
	is_on = invsens_sm_is_func_enabled(&iio_data->sm_data, INV_FUNC_GYRO);

	up(&iio_data->sema);

	return snprintf(buf, PAGE_SIZE, "%d\n", is_on);
}

static ssize_t invsens_s_gyro_selftest_show(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct invsens_iio_data *iio_data = dev_get_drvdata(dev);
	int res;
	struct invsens_ioctl_param_t ioctl_data;

	down(&iio_data->sema);

	res = invsens_sm_ioctl(&iio_data->sm_data,
		INV_IOCTL_S_GYRO_SELFTEST, 0, (void *) &ioctl_data);
	up(&iio_data->sema);

	if (res == SM_IOCTL_HANDLED)
		return snprintf(buf, PAGE_SIZE, "%d,"\
			"%d.%03d,%d.%03d,%d.%03d,"\
			"%d.%03d,%d.%03d,%d.%03d,"\
			"%d.%01d,%d.%01d,%d.%01d,"\
			"%d.%03d,%d.%03d,%d.%03d\n",
			ioctl_data.s_gyro_selftest.result,
			(int)abs(ioctl_data.s_gyro_selftest.gyro_bias[0] / 1000),
			(int)abs(ioctl_data.s_gyro_selftest.gyro_bias[0]) % 1000,
			(int)abs(ioctl_data.s_gyro_selftest.gyro_bias[1] / 1000),
			(int)abs(ioctl_data.s_gyro_selftest.gyro_bias[1]) % 1000,
			(int)abs(ioctl_data.s_gyro_selftest.gyro_bias[1] / 1000),
			(int)abs(ioctl_data.s_gyro_selftest.gyro_bias[1]) % 1000,
			(int)ioctl_data.s_gyro_selftest.gyro_rms[0] /1000,
			(int)ioctl_data.s_gyro_selftest.gyro_rms[0] % 1000,
			(int)ioctl_data.s_gyro_selftest.gyro_rms[1] / 1000,
			(int)ioctl_data.s_gyro_selftest.gyro_rms[1] % 1000,
			(int)ioctl_data.s_gyro_selftest.gyro_rms[2] / 1000,
			(int)ioctl_data.s_gyro_selftest.gyro_rms[2] % 1000,
			ioctl_data.s_gyro_selftest.gyro_ratio[0] / 10,
			ioctl_data.s_gyro_selftest.gyro_ratio[0] % 10,
			ioctl_data.s_gyro_selftest.gyro_ratio[1] / 10,
			ioctl_data.s_gyro_selftest.gyro_ratio[1] % 10,
			ioctl_data.s_gyro_selftest.gyro_ratio[2] / 10,
			ioctl_data.s_gyro_selftest.gyro_ratio[2] % 10,
			ioctl_data.s_gyro_selftest.packet_cnt, 0,
			ioctl_data.s_gyro_selftest.packet_cnt, 0,
			ioctl_data.s_gyro_selftest.packet_cnt, 0);
	else
		return snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t invsens_s_accel_selftest_show(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct invsens_iio_data *iio_data = dev_get_drvdata(dev);
	int res;
	struct invsens_ioctl_param_t ioctl_data;

	down(&iio_data->sema);

	res = invsens_sm_ioctl(&iio_data->sm_data,
		INV_IOCTL_S_ACCEL_SELFTEST, 0, (void *) &ioctl_data);

	up(&iio_data->sema);

	if (res == SM_IOCTL_HANDLED)
		return snprintf(buf, PAGE_SIZE, "%d,%d.%01d,%d.%01d,%d.%01d\n",
			ioctl_data.s_accel_selftest.result,
			ioctl_data.s_accel_selftest.accel_ratio[0] / 10,
			ioctl_data.s_accel_selftest.accel_ratio[0] % 10,
			ioctl_data.s_accel_selftest.accel_ratio[1] / 10,
			ioctl_data.s_accel_selftest.accel_ratio[1] % 10,
			ioctl_data.s_accel_selftest.accel_ratio[2] / 10,
			ioctl_data.s_accel_selftest.accel_ratio[2] % 10);
	else
		return snprintf(buf, PAGE_SIZE, "0\n");
}

static ssize_t invsens_s_accel_cal_start_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int res, i;
	struct invsens_iio_data *iio_data = dev_get_drvdata(dev);
	struct invsens_ioctl_param_t ioctl_data;
	long value;

	res = kstrtol(buf, 0, &value);

	if (value == 1) {
		down(&iio_data->sema);
		res = invsens_sm_ioctl(&iio_data->sm_data,
			INV_IOCTL_DO_ACCEL_CAL, 0, (void *) &ioctl_data);
		up(&iio_data->sema);

		memcpy(iio_data->accel_bias, ioctl_data.accel_bias.bias,
			sizeof(ioctl_data.accel_bias.bias));
		iio_data->accel_is_calibrated = true;
	} else if (value == 0) {
		for (i = 0; i < 3 ; i++)
			iio_data->accel_bias[i] = 0;

		iio_data->accel_is_calibrated = false;
	}

	invsens_store_accel_cal(iio_data->accel_bias);

	return count;
}

static ssize_t invsens_s_accel_cal_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct invsens_iio_data *iio_data = dev_get_drvdata(dev);
	int err;

	INV_DBG_FUNC_NAME;

	if (iio_data->accel_is_calibrated)
		err = 1;
	else
		err = -1;

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d, %d\n", err,
			iio_data->accel_bias[0], iio_data->accel_bias[1],
			iio_data->accel_bias[2]);
}

static ssize_t invsens_motion_interrupt_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct invsens_iio_data *iio_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n",
		iio_data->motion_alert_is_occured);
}

static ssize_t invsens_motion_interrupt_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	int res = 0, enable;
	struct invsens_iio_data *iio_data = dev_get_drvdata(dev);

	down(&iio_data->sema);
	res = kstrtoint(buf, 0, &enable);
	/*
		mode of motion interrupt
			0. disable motion interrupt
			1. normal motion interrupt
			2. probe interrupt pin
	*/

	if (enable > 0) {
		struct invsens_ioctl_param_t ioctl_data;

		if (enable == 1) {
			ioctl_data.data[0] = 0xc;
		} else if (enable == 2) {
			ioctl_data.data[0] = 0x0;
			iio_data->motion_alert_factory_mode = true;
		}

		res = invsens_sm_ioctl(&iio_data->sm_data,
			INV_IOCTL_SET_WOM_THRESH, 0, (void *) &ioctl_data);
		iio_data->motion_alert_start_time = jiffies;
	} else {
		iio_data->motion_alert_factory_mode = false;
	}

	res = invsens_sm_enable_sensor(&iio_data->sm_data,
		INV_FUNC_MOTION_INTERRUPT, (bool)enable, SM_DELAY_DEFAULT);

	up(&iio_data->sema);

	iio_data->motion_alert_is_occured = false;

	pr_info("[INV]%s: enable = %d\n", __func__, enable);

	return count;
}

static void invsens_delayed_work_fso(struct work_struct *work)
{
	struct invsens_iio_data *iio_data = container_of(work,
		struct invsens_iio_data, fso_work.work);

	int res;
	down(&iio_data->sema);

	res = invsens_sm_enable_sensor(&iio_data->sm_data,
		INV_FUNC_FT_SENSOR_ON, false, SM_DELAY_DEFAULT);
	iio_data->fso_enabled = false;
	up(&iio_data->sema);
}

static ssize_t invsens_accel_raw_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	int res = SM_SUCCESS;

	struct invsens_iio_data *iio_data = dev_get_drvdata(dev);

	/*factory mode on*/
	if (!iio_data->fso_enabled) {
		down(&iio_data->sema);
		res = invsens_sm_enable_sensor(&iio_data->sm_data,
			INV_FUNC_FT_SENSOR_ON, true, SM_DELAY_FTMODE);
		iio_data->fso_enabled = true;
		up(&iio_data->sema);
	}

	if (!res) {
		cancel_delayed_work(&iio_data->fso_work);
		schedule_delayed_work(&iio_data->fso_work,
			msecs_to_jiffies(2000));
	}
	/*read data from cache*/
	return snprintf(buf, PAGE_SIZE, "%d, %d, %d\n",
		iio_data->accel_cache[0], iio_data->accel_cache[1],
		iio_data->accel_cache[2]);
}


static IIO_DEVICE_ATTR(control, S_IRUGO, NULL, invsens_control_store, 0);
static IIO_DEVICE_ATTR(reg_dump, S_IRUGO, invsens_reg_dump_show, NULL, 0);
static IIO_DEVICE_ATTR(log_level, S_IRUGO | S_IWUSR,
		       invsens_log_level_show, invsens_log_level_store, 0);
static IIO_DEVICE_ATTR(swst_gyro, S_IRUGO | S_IWUSR,
		       invsens_swst_gyro_show, NULL, 0);
static IIO_DEVICE_ATTR(hwst_accel, S_IRUGO | S_IWUSR,
		       invsens_hwst_accel_show, NULL, 0);
static IIO_DEVICE_ATTR(hwst_gyro, S_IRUGO | S_IWUSR,
		       invsens_hwst_gyro_show, NULL, 0);
static IIO_DEVICE_ATTR(swst_compass, S_IRUGO | S_IWUSR,
		       invsens_swst_compass_show, NULL, 0);


static struct attribute *invsens_attributes[] = {
	&iio_dev_attr_control.dev_attr.attr,
	&iio_dev_attr_reg_dump.dev_attr.attr,
	&iio_dev_attr_log_level.dev_attr.attr,
	&iio_dev_attr_swst_gyro.dev_attr.attr,
	&iio_dev_attr_hwst_accel.dev_attr.attr,
	&iio_dev_attr_hwst_gyro.dev_attr.attr,
	&iio_dev_attr_swst_compass.dev_attr.attr,
	NULL,
};

static const struct attribute_group invsens_attr_group = {
	.attrs = invsens_attributes,
};

static const struct iio_info invsens_i2c_info = {
	.attrs = &invsens_attr_group,
	.driver_module = THIS_MODULE,
};

#if defined(CONFIG_SENSORS)
static ssize_t invens_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t invens_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", MODEL_NAME);
}


static struct device_attribute dev_attr_acc_reactive_alert =
	__ATTR(reactive_alert, S_IRUGO | S_IWUSR | S_IWGRP,
		invsens_motion_interrupt_show, invsens_motion_interrupt_store);
static struct device_attribute dev_attr_acc_calibration =
	__ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
		invsens_s_accel_cal_show, invsens_s_accel_cal_start_store);
static struct device_attribute dev_attr_acc_selftest =
	__ATTR(selftest, S_IRUSR | S_IRGRP,
	invsens_s_accel_selftest_show, NULL);
static struct device_attribute dev_attr_acc_raw_data =
	__ATTR(raw_data, S_IRUSR | S_IRGRP,
	invsens_accel_raw_show, NULL);
static struct device_attribute dev_attr_acc_vendor =
	__ATTR(vendor, S_IRUSR | S_IRGRP,
	invens_vendor_show, NULL);
static struct device_attribute dev_attr_acc_name =
	__ATTR(name, S_IRUSR | S_IRGRP,
	invens_name_show, NULL);

static struct device_attribute *accel_sensor_attrs[] = {
	&dev_attr_acc_reactive_alert,
	&dev_attr_acc_calibration,
	&dev_attr_acc_selftest,
	&dev_attr_acc_raw_data,
	&dev_attr_acc_vendor,
	&dev_attr_acc_name,
	NULL,
};

static struct device_attribute dev_attr_gyro_temperature =
	__ATTR(temperature, S_IRUSR | S_IRGRP,
	invsens_temperature_show, NULL);
static struct device_attribute dev_attr_gyro_selftest =
	__ATTR(selftest, S_IRUSR | S_IRGRP,
	invsens_s_gyro_selftest_show, NULL);
static struct device_attribute dev_attr_gyro_power_on =
	__ATTR(power_on, S_IRUSR | S_IRGRP,
	invsens_gyro_is_on_show, NULL);
static struct device_attribute dev_attr_gyro_vendor =
	__ATTR(vendor, S_IRUSR | S_IRGRP,
	invens_vendor_show, NULL);
static struct device_attribute dev_attr_gyro_name =
	__ATTR(name, S_IRUSR | S_IRGRP,
	invens_name_show, NULL);

static struct device_attribute *gyro_sensor_attrs[] = {
	&dev_attr_gyro_temperature,
	&dev_attr_gyro_selftest,
	&dev_attr_gyro_power_on,
	&dev_attr_gyro_vendor,
	&dev_attr_gyro_name,
	NULL,
};
#endif

static const struct iio_chan_spec invsens_channels[] = {
	IIO_CHAN_SOFT_TIMESTAMP(8),
};

static int invsens_preenable(struct iio_dev *indio_dev)
{
	int result = 0;
	struct invsens_iio_data *iio_data = iio_priv(indio_dev);
	struct iio_buffer *iio_buffer = NULL;

	INV_DBG_FUNC_NAME;

	data_list.copy_buffer =
	    kzalloc(INVSENS_COPY_BUFFER_SIZE, GFP_KERNEL);

	if (!data_list.copy_buffer)
		return -ENOMEM;
	iio_buffer = iio_data->indio_dev->buffer;
	iio_buffer->access->set_bytes_per_datum(iio_buffer,
				INVSENS_IIO_BUFFER_DATUM);

	return result;
}

static int invsens_predisable(struct iio_dev *indio_dev)
{
	int result = 0;

	INV_DBG_FUNC_NAME;

	if (data_list.copy_buffer) {
		kzfree(data_list.copy_buffer);
		data_list.copy_buffer = NULL;
	}

	return result;
}

static const struct iio_buffer_setup_ops invsens_ring_setup_ops = {
	.preenable = &invsens_preenable,
	.predisable = &invsens_predisable,
};

static irqreturn_t invsens_irq_handler(int irq, void *user_data)
{

	INV_DBG_FUNC_NAME;

	data_list.timestamp = iio_get_time_ns();

	return IRQ_WAKE_THREAD;
}

static void invsens_prefetch_data(struct invsens_iio_data *iio_data,
	bool *need_to_store)
{
	int i;
	struct invsens_data_t *p = NULL;
	unsigned long timediff = 0;

	if (!iio_data)
		return;

	/*copy data to cache*/
	for (i = 0; i < data_list.count; i++) {
		p = &data_list.items[i];

		if (p->hdr == INV_DATA_HDR_ACCEL) {
			memcpy(iio_data->accel_cache, p->data, p->length);
			if (iio_data->accel_is_calibrated) {
				iio_data->accel_cache[0] -=
					iio_data->accel_bias[0];
				iio_data->accel_cache[1] -=
					iio_data->accel_bias[1];
				iio_data->accel_cache[2] -=
					iio_data->accel_bias[2];
				memcpy(p->data,
					iio_data->accel_cache, p->length);
			}
		}

		if (p->hdr == INV_DATA_HDR_GYRO)
			memcpy(iio_data->gyro_cache, p->data, p->length);
	}

	if (unlikely(data_list.request_fifo_reset)) {
		invsens_sm_ioctl(&iio_data->sm_data,
			INV_IOCTL_RESET_FIFO, 0, NULL);
		INVSENS_LOGE("reset fifo\n");
		//return; /*remove to deliver normally stored data to HAL*/
	}

	*need_to_store = true;

	/*if only factory mode is on,
	*it isn't neccessary to post buffer to hal*/
	if (!(data_list.enable_mask & ~(1 << INV_FUNC_FT_SENSOR_ON)))
		*need_to_store = false;

	if (data_list.event_motion_interrupt_notified) {
		timediff = jiffies_to_msecs(jiffies - iio_data->motion_alert_start_time);
		/* ignore motion interrupt happened
		*in 100ms to skip intial erronous interrupt */
		if (timediff < 1000 && !(iio_data->motion_alert_factory_mode)) {
			pr_info("[INV]%s: timediff = %ld msec\n", __func__, timediff);
			return;
		}

		pr_info("[INV]%s: motion interrupt is delivered !!!!!\n", __func__);
		iio_data->motion_alert_is_occured = true;
		wake_lock_timeout(&iio_data->motion_alert_wake_lock, msecs_to_jiffies(2000));
	}
}

static void invsens_store_to_buffer(struct invsens_iio_data *iio_data)
{
	s64 timestamp;
	uint8_t *copy_buffer = NULL;
	struct iio_buffer *iio_buffer = NULL;
	struct invsens_data_t *p = NULL;
	int i = 0, ind = 0;

	timestamp = data_list.timestamp;
	copy_buffer = data_list.copy_buffer;

	if (copy_buffer && (data_list.count > 0)) {
		int timestamp_size = sizeof(timestamp);
		u16 packet_header = INVSENS_PACKET_HEADER;
		u16 packet_size;
		u16 packet_size_offset;

		iio_buffer = iio_data->indio_dev->buffer;

		/*fill packet header */
		memcpy(&copy_buffer[ind], &packet_header,
			sizeof(packet_header));
		ind += sizeof(packet_header);
		packet_size_offset = ind;
		/*leave packet size empty,
		 *it will be filled out at the bottom*/
		ind += sizeof(packet_size);

		/*copy timestamp*/
		memcpy(&copy_buffer[ind], (void *) &timestamp,
		       timestamp_size);
		ind += timestamp_size;

		/*fill sensor data into copy buffer*/
		for (i = 0; i < data_list.count; i++) {
			p = &data_list.items[i];

			/*put header into buffer first*/
			memcpy(&copy_buffer[ind], &p->hdr, sizeof(p->hdr));
			ind += sizeof(p->hdr);

			/*put data into buffer*/
			memcpy(&copy_buffer[ind], p->data, p->length);
			ind += p->length;
		}

		/*fill packet size*/
		packet_size = (u16)ind;
		memcpy(&copy_buffer[packet_size_offset],
			&packet_size, sizeof(packet_size));

		iio_buffer->access->store_to(iio_buffer, copy_buffer,
					     timestamp);
	}

}

static irqreturn_t invsens_thread_handler(int irq, void *user_data)
{
	int result = 0;
	struct invsens_iio_data *iio_data = user_data;
	bool need_to_store = false;

	INV_DBG_FUNC_NAME;

	if (iio_data->irq_is_disabled == true)
		goto error_trylock;

	if (down_trylock(&iio_data->sema))
		goto error_trylock;

	result = invsens_sm_read(&iio_data->sm_data, &data_list);
	up(&iio_data->sema);

	invsens_prefetch_data(iio_data, &need_to_store);

	if ((result == SM_SUCCESS) && need_to_store)
		invsens_store_to_buffer(iio_data);

	/*clear data list */
	data_list.count = 0;
	data_list.is_fifo_data_copied = false;
	data_list.event_motion_interrupt_notified = false;
	data_list.request_fifo_reset = false;

error_trylock:
	return IRQ_HANDLED;
}

static int invsens_configure_buffer(struct invsens_iio_data *iio_data)
{
	int result = 0;
	struct iio_buffer *buffer;

	buffer = invsens_kfifo_allocate(iio_data->indio_dev);
	if (buffer == NULL)
		goto exit_destruct_kfifo;

	iio_data->indio_dev->buffer = buffer;

	/* setup ring buffer */
	buffer->scan_timestamp = true;
	iio_data->indio_dev->setup_ops = &invsens_ring_setup_ops;
	iio_data->indio_dev->modes |= INDIO_BUFFER_TRIGGERED;

	result = request_threaded_irq(iio_data->irq,
				      invsens_irq_handler,
				      invsens_thread_handler,
				      IRQF_TRIGGER_RISING | IRQF_SHARED | IRQF_ONESHOT,
				      "inv_sensors_irq", iio_data);
	if (result)
		goto exit_unreg_irq;
	return 0;

exit_destruct_kfifo:
exit_unreg_irq:
	invsens_kfifo_free(buffer);
	return result;
}

static int invsens_set_trigger_state(struct iio_trigger *trig, bool state)
{
	return 0;
}

static const struct iio_trigger_ops invsens_trriger_ops = {
	.owner = THIS_MODULE,
	.set_trigger_state = &invsens_set_trigger_state,
};

static int invsens_set_trigger(struct invsens_iio_data *iio_data)
{
	struct iio_trigger *trig;
	int result = 0;
	trig = iio_allocate_trigger("%s-dev%d",
				    iio_data->indio_dev->name,
				    iio_data->indio_dev->id);
	trig->private_data = iio_data;
	trig->ops = &invsens_trriger_ops;

	result = iio_trigger_register(trig);
	if (result) {
		iio_free_trigger(trig);
		return result;
	}

	iio_data->indio_dev->trig = trig;

	return result;
}

static int invsens_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct invsens_iio_data *data = NULL;
	struct iio_dev *indio_dev;
	int result = 0;
#ifdef CONFIG_INV_SENSORS_DTS_SUPPORT
	static struct invsens_platform_data_t platform_data;
#endif

	pr_info("[INV] invensense sensors probe : %s\n", client->name);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		result = -ENOSYS;
		pr_err("[INV] I2c function error\n");
		goto exit_free_data;
	}

	indio_dev = iio_allocate_device(sizeof(struct invsens_iio_data));
	if (!indio_dev) {
		result = -ENOMEM;
		goto exit_free_data;
	}

	/* change the name of iio node from iio:devicex to invensense*/
	dev_set_name(&indio_dev->dev, "invensense");

	i2c_set_clientdata(client, indio_dev);

	data = iio_priv(indio_dev);

	invsens_set_log_mask(INVSENS_LOG_ERR);
	data->indio_dev = indio_dev;

	/*setup iio device nodes */
	data->indio_dev->info = &invsens_i2c_info;
	data->indio_dev->dev.parent = &client->dev;
	data->indio_dev->modes = 0x0;
	data->indio_dev->channels = invsens_channels;
	data->indio_dev->num_channels = ARRAY_SIZE(invsens_channels);
	data->indio_dev->name = client->name;

	sema_init(&data->sema, 1);
	/*set inital values for internal drivers */
	data->i2c_handler.client = client;

	data->irq = client->irq;
	data->device_name = client->name;

	/*set sm data */
	data->sm_data.board_cfg.i2c_handle = &data->i2c_handler;
	data->sm_data.board_cfg.i2c_addr = client->addr;
	data->sm_data.board_cfg.platform_data = data->platform_data;

	INIT_DELAYED_WORK(&data->fso_work, invsens_delayed_work_fso);
	data->fso_enabled = false;

	/*cal info*/
	memset(data->accel_bias, 0x0, sizeof(data->accel_bias));
	data->accel_is_calibrated = false;

	/*motion alert*/
	data->motion_alert_is_occured = false;

	data->irq_is_disabled = false;

#ifdef CONFIG_INV_SENSORS_DTS_SUPPORT
	inv_sensors_parse_dt(&client->dev, &platform_data);
	data->sm_data.board_cfg.platform_data = &platform_data;
	/*Power on device. */
	if (platform_data.power_on) {
		result = platform_data.power_on(&platform_data);
		if (result < 0) {
			dev_err(&client->dev,
				"power_on failed: %d\n", result);
			return result;
		}
	}

	msleep(100);
#else
	data->platform_data = (struct invsens_platform_data_t *)
	    dev_get_platdata(&client->dev);
	data->sm_data.board_cfg.platform_data = data->platform_data;
#endif

	/*configure iio fifo buffer */
	result = invsens_configure_buffer(data);
	if (result)
		goto exit_free_ring;

	/*register iio buffer */
	result = iio_buffer_register(data->indio_dev,
			data->indio_dev->channels,
			data->indio_dev->num_channels);
	if (result)
		goto exit_register_buffer_failed;

	result = invsens_set_trigger(data);
	if (result)
		goto exit_free_buffer;

	result = invsens_sm_init(&data->sm_data);
	if (result)
		goto exit_sm_init_failed;

	/*register iio device */
	result = iio_device_register(data->indio_dev);
	if (result)
		goto exit_register_device_failed;

#ifdef CONFIG_SENSORS
	wake_lock_init(&data->motion_alert_wake_lock, WAKE_LOCK_SUSPEND,
			"motion_alert_wake_lock");
	result = sensors_register(data->gyro_sensor_device,
		data, gyro_sensor_attrs, "gyro_sensor");
	if (result) {
		pr_err("%s: cound not register gyro sensor device(%d).\n",
		__func__, result);
		goto err_gyro_sensor_register_failed;
	}

	result = sensors_register(data->accel_sensor_device,
		data, accel_sensor_attrs, "accelerometer_sensor");
	if (result) {
		pr_err("%s: cound not register accel sensor device(%d).\n",
		__func__, result);
		goto err_accel_sensor_register_failed;
	}
#endif

	pr_info("[INV] probe done\n");
	return 0;

#ifdef CONFIG_SENSORS
err_accel_sensor_register_failed:
	sensors_unregister(data->gyro_sensor_device, gyro_sensor_attrs);
err_gyro_sensor_register_failed:
	iio_device_unregister(data->indio_dev);
	wake_lock_destroy(&data->motion_alert_wake_lock);
#endif
exit_register_device_failed:
	invsens_sm_term(&data->sm_data);
exit_sm_init_failed:
	iio_free_trigger(data->indio_dev->trig);
exit_free_buffer:
	iio_buffer_unregister(data->indio_dev);
exit_register_buffer_failed:
	invsens_kfifo_free(data->indio_dev->buffer);
exit_free_ring:
	iio_free_device(data->indio_dev);
exit_free_data:
	if (data != NULL)
		kfree(data);

	pr_info("[INV] probe FAILED(errno : %d)\n", result);
	return -EIO;
}

static int invsens_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);
	iio_device_unregister(indio_dev);
	return 0;
}

static int invsens_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);
	struct invsens_iio_data *iio_data = NULL;
	u32 enabled_mask;
	bool is_irq_disable = false;
	iio_data = iio_priv(indio_dev);

	pr_info("[INV] %s: start\n", __func__);
	if (down_trylock(&iio_data->sema))
		INVSENS_LOGD("TRYLOCK Fail, count=%d\n", iio_data->sema.count);

	up(&iio_data->sema);

	INV_DBG_FUNC_NAME;

	invsens_sm_get_enabled_mask(&iio_data->sm_data, &enabled_mask);

	if((enabled_mask & (1<< INV_FUNC_STEP_DETECT))
		||(enabled_mask & (1<< INV_FUNC_STEP_COUNT))
		||(enabled_mask & (1<< INV_FUNC_MOTION_INTERRUPT))
		||(enabled_mask & (1<< INV_FUNC_FT_SENSOR_ON))) {
		is_irq_disable = false;
	} else
		is_irq_disable = true;

	if(is_irq_disable) {
		disable_irq(iio_data->irq);
		iio_data->irq_is_disabled = true;
	} else
		enable_irq_wake(iio_data->irq);

	data_list.request_fifo_reset = false;

	pr_info("[INV] %s: finish\n", __func__);
	return 0;
}

static int invsens_resume(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);
	struct invsens_iio_data *iio_data = NULL;
	iio_data = iio_priv(indio_dev);

	pr_info("[INV] %s: start\n", __func__);
	if (down_trylock(&iio_data->sema))
		INVSENS_LOGD("TRYLOCK Fail, count=%d\n", iio_data->sema.count);

	up(&iio_data->sema);

	INV_DBG_FUNC_NAME;

	if(iio_data->irq_is_disabled) {
		enable_irq(iio_data->irq);
		iio_data->irq_is_disabled = false;
	} else
		disable_irq_wake(iio_data->irq);

	/*fifo reset*/
	data_list.request_fifo_reset = true;

	pr_info("[INV] %s: finish\n", __func__);
	return 0;
}

static const u16 normal_i2c[] = { I2C_CLIENT_END };

static struct of_device_id mpu6515_match_table[] = {
	{ .compatible = "mpu9250",},
	{ .compatible = "mpu6515",},
	{},
};

static const struct i2c_device_id invsens_ids[] = {
	{"ITG3500", 119},
	{"mpu3050", 63},
	{"mpu6050", 117},
	{"mpu9150", 118},
	{"mpu6500", 128},
	{"mpu9250", 128},
	{"mpu9350", 128},
	{"mpu6515", 128},
	{}
};

static struct i2c_driver invsens_driver = {
	.driver = {
		.name = "inv_sensors",
		.of_match_table = mpu6515_match_table,
	},
	.id_table = invsens_ids,
	.probe = invsens_probe,
	.remove = invsens_remove,
	.address_list = normal_i2c,
	.suspend = invsens_suspend,
	.resume = invsens_resume,
};

static int __init invsens_init(void)
{
	INV_DBG_FUNC_NAME;

	return i2c_add_driver(&invsens_driver);
}

static void __exit invsens_exit(void)
{
	INV_DBG_FUNC_NAME;

	i2c_del_driver(&invsens_driver);
}

MODULE_AUTHOR("Invensense Korea Design Center");
MODULE_DESCRIPTION("Invensense Linux Driver");
MODULE_LICENSE("GPL");

module_init(invsens_init);
module_exit(invsens_exit);
