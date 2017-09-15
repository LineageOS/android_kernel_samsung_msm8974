/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/bma254.h>

#include <linux/regulator/consumer.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#include <linux/sensors_core.h>

#ifdef CONFIG_BMA254_SMART_ALERT
#include <linux/wakelock.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#endif

#define CHIP_DEV_NAME	"BMA254"
#define CHIP_DEV_VENDOR	"BOSCH"
#define CAL_PATH		"/efs/calibration_data"
#define CALIBRATION_DATA_AMOUNT         20
#define MAX_ACCEL_1G			1024

#ifdef CONFIG_BMA254_SMART_ALERT
extern unsigned int system_rev;

struct bma254_platform_data {
	int p_out;				/* acc-sensor-irq gpio */
};
#endif

struct bma254_data {
	struct i2c_client *client;
	struct input_dev *input;
	struct device *dev;
	struct delayed_work work;
#ifdef CONFIG_BMA254_SMART_ALERT
	struct bma254_platform_data *pdata;
	struct mutex data_mutex;
	struct work_struct alert_work;
	struct wake_lock reactive_wake_lock;
	atomic_t reactive_state;
	atomic_t reactive_enable;
	bool factory_mode;
	int accsns_activate_flag;
	int pin_check_fail;
	int IRQ;
#endif
	int cal_data[3];
	int position;
	int delay;
	int enable;
};

static const int position_map[][3][3] = {
	{ {-1,  0,  0}, { 0, -1,  0}, { 0,  0,  1} },
	{ { 0, -1,  0}, { 1,  0,  0}, { 0,  0,  1} },
	{ { 1,  0,  0}, { 0,  1,  0}, { 0,  0,  1} },
	{ { 0,  1,  0}, {-1,  0,  0}, { 0,  0,  1} },
	{ { 1,  0,  0}, { 0, -1,  0}, { 0,  0, -1} },
	{ { 0,  1,  0}, { 1,  0,  0}, { 0,  0, -1} },
	{ {-1,  0,  0}, { 0,  1,  0}, { 0,  0, -1} },
	{ { 0, -1,  0}, {-1,  0,  0}, { 0,  0, -1} },
};

static int bma254_i2c_read(struct bma254_data *bma254, u8 reg, unsigned char *rbuf, int len)
{
	int ret = -1;
	struct i2c_msg msg[2];
	struct i2c_client *client = bma254->client;

	if (unlikely((client == NULL) || (!client->adapter)))
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &reg;

	msg[1].addr = client->addr;
	msg[1].flags = 1;
	msg[1].len = len;
	msg[1].buf = rbuf;

	ret = i2c_transfer(client->adapter, &msg[0], 2);

	if (unlikely(ret < 0))
		pr_err("%s,i2c transfer error ret=%d\n", __func__, ret);

	return ret;
}

static int bma254_i2c_write(struct bma254_data *bma254, u8 reg, u8 val)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int retry = 2;
	struct i2c_client *client = bma254->client;

	if (unlikely((client == NULL) || (!client->adapter)))
		return -ENODEV;

	do {
		data[0] = reg;
		data[1] = val;

		msg->addr = client->addr;
		msg->flags = 0;
		msg->len = 2;
		msg->buf = data;

		err = i2c_transfer(client->adapter, msg, 1);

		if (err >= 0)
			return 0;
	} while (--retry > 0);

	pr_err("%s,i2c transfer error(%d)\n", __func__, err);
	return err;
}

static void bma254_activate(struct bma254_data *bma254, bool enable)
{
	pr_err("%s,acitve(%s)\n", __func__, enable ? "true" : "false");
	if (enable == true) {
#ifdef CONFIG_BMA254_SMART_ALERT
		if (!bma254->accsns_activate_flag)
		{
			bma254_i2c_write(bma254, BMA254_REG14, SOFT_RESEET);
			msleep(5);
		}
#else
		bma254_i2c_write(bma254, BMA254_REG14, SOFT_RESEET);
		msleep(5);
#endif

		bma254_i2c_write(bma254, BMA254_REG0F, BMA2X2_RANGE_SET);
		bma254_i2c_write(bma254, BMA254_REG10, BANDWIDTH_31_25);
		bma254_i2c_write(bma254, BMA254_REG11, BMA254_MODE_NORMAL);
	}
#ifndef CONFIG_BMA254_SMART_ALERT
	else {
		bma254_i2c_write(bma254, BMA254_REG11, BMA254_MODE_SUSPEND);
	}
#else
	if(bma254->accsns_activate_flag && enable != true){
		pr_info("%s: low power mode\n", __func__);
		bma254_i2c_write(bma254, BMA254_REG0F, BMA2X2_RANGE_SET);
		bma254_i2c_write(bma254, BMA254_REG10, BANDWIDTH_07_81);
		bma254_i2c_write(bma254, BMA254_REG11, BMA254_MODE_LOWPOWER1);
	}else if(enable != true){
		bma254_i2c_write(bma254, BMA254_REG11, BMA254_MODE_SUSPEND);
	}
#endif
}

#ifdef CONFIG_BMA254_SMART_ALERT
static int bma254_get_motion_interrupt(struct bma254_data *bma254)
{
	int result = 0;
	unsigned char data;
	u8 buf[2];
	data = bma254_i2c_read(bma254, BMA254_SLOPE_INT_S__REG, buf, 2);
	pr_info("%s: call : return %d\n", __func__, data);
	result = 1;
	return result;
}

static void bma254_set_motion_interrupt(struct bma254_data *bma254,  bool enable,
	bool factorytest)
{
	if (enable) {
		bma254_activate(bma254, false);
		usleep_range(5000, 6000);
		pr_info("%s : enable\n", __func__);
		bma254_i2c_write(bma254, BMA254_EN_INT1_PAD_SLOPE__REG, 0x04);
		if (factorytest) {
			bma254_i2c_write(bma254, BMA254_SLOPE_DUR__REG, 0x00);
			bma254_i2c_write(bma254, BMA254_SLOPE_THRES__REG, 0x00);
		} else {
			bma254_i2c_write(bma254, BMA254_SLOPE_DUR__REG, 0x02);
			bma254_i2c_write(bma254, BMA254_SLOPE_THRES__REG, 0x10);
		}

		bma254_i2c_write(bma254, BMA254_INT_ENABLE1_REG, 0x07);
	} else {
		pr_info("%s : disable\n", __func__);
		bma254_i2c_write(bma254, BMA254_EN_INT1_PAD_SLOPE__REG, 0x00);
		bma254_i2c_write(bma254, BMA254_SLOPE_DUR__REG, 0x03);
		bma254_i2c_write(bma254, BMA254_INT_ENABLE1_REG, 0x00);
		bma254_i2c_write(bma254, BMA254_SLOPE_THRES__REG, 0xff);
		usleep_range(5000, 6000);
	}
}
#endif

static int bma254_get_data(struct bma254_data *bma254, int *xyz, bool cal)
{

	int ret;
	int i, j;
	int data[3] = {0,};
	u8 sx[6] = {0,};

	ret = bma254_i2c_read(bma254, BMA254_XOUT, sx, 6);
	if (ret < 0)
		return ret;
	for (i = 0; i < 3; i++) {
		data[i] = ((int)((s8)sx[2 * i + 1] << 8 |
			((s8)sx[2 * i] & 0xfe))) >> 4;
	}

	for (i = 0; i < 3; i++) {
		xyz[i] = 0;
		for (j = 0; j < 3; j++)
			xyz[i] += data[j] * position_map[bma254->position][i][j];

		if (cal)
			xyz[i] = (xyz[i] - bma254->cal_data[i]);
	}
	return ret;
}

static void bma254_work_func(struct work_struct *work)
{
	struct bma254_data *bma254 = container_of((struct delayed_work *)work,
			struct bma254_data, work);
	int xyz[3] = {0,};
	int ret;

	if (bma254 == NULL) {
		pr_err("%s, NULL point\n", __func__);
		return;
	}

	ret = bma254_get_data(bma254, xyz, true);
	if (ret < 0) {
		pr_err("%s, data error(%d)\n", __func__, ret);
	} else {
#ifdef REPORT_ABS
		input_report_abs(bma254->input, ABS_X, xyz[0]);
		input_report_abs(bma254->input, ABS_Y, xyz[1]);
		input_report_abs(bma254->input, ABS_Z, xyz[2]);
#else
		input_report_rel(bma254->input, REL_X, xyz[0] < 0 ? xyz[0] : xyz[0] + 1);
		input_report_rel(bma254->input, REL_Y, xyz[1] < 0 ? xyz[1] : xyz[1] + 1);
		input_report_rel(bma254->input, REL_Z, xyz[2] < 0 ? xyz[2] : xyz[2] + 1);
#endif
		input_sync(bma254->input);
	}

	if (bma254->enable)
		schedule_delayed_work(&bma254->work, msecs_to_jiffies(bma254->delay));
}

static int bma254_open_calibration(struct bma254_data *bma254)
{
	struct file *cal_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CAL_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		return err;
	}

	err = cal_filp->f_op->read(cal_filp,
		(char *)&bma254->cal_data, 3 * sizeof(s32), &cal_filp->f_pos);
	if (err != 3 * sizeof(s32)) {
		pr_err("%s: Can't read the cal data from file\n", __func__);
		err = -EIO;
	}

	pr_info("%s: (%d,%d,%d)\n", __func__,
		bma254->cal_data[0], bma254->cal_data[1], bma254->cal_data[2]);

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return err;
}

static ssize_t bma254_enable_store(struct device *dev,
			   struct device_attribute *attr,
		       const char *buf, size_t size)
{
	struct bma254_data *bma254 = dev_get_drvdata(dev);

	int value = 0;
	int err = 0;

	err = kstrtoint(buf, 10, &value);
	if (err) {
		pr_err("%s, kstrtoint failed.", __func__);
		goto done;
	}
	if (value != 0 && value != 1) {
		pr_err("%s,wrong value(%d)\n", __func__, value);
		goto done;
	}

	if (bma254->enable != value) {
		pr_info("%s, enable(%d)\n", __func__, value);
		if (value) {
			bma254_activate(bma254, true);
			bma254_open_calibration(bma254);
			schedule_delayed_work(&bma254->work, msecs_to_jiffies(bma254->delay));
		} else {
			cancel_delayed_work_sync(&bma254->work);
			bma254_activate(bma254, false);
		}
		bma254->enable = value;
	} else {
		pr_err("%s, wrong cmd for enable\n", __func__);
	}
done:
	return size;
}

static ssize_t bma254_enable_show(struct device *dev,
			  struct device_attribute *attr,
		      char *buf)
{
	struct bma254_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->enable);
}

static ssize_t bma254_delay_store(struct device *dev,
			   struct device_attribute *attr,
		       const char *buf, size_t size)
{
	struct bma254_data *bma254 = dev_get_drvdata(dev);

	int value = 0;
	int err = 0;

	err = kstrtoint(buf, 10, &value);
	if (err) {
		pr_err("%s, kstrtoint failed\n", __func__);
		goto done;
	}
	if (value < 0 || 200 < value) {
		pr_err("%s,wrong value(%d)\n", __func__, value);
		goto done;
	}

	if (bma254->delay != value) {
		if (bma254->enable)
			cancel_delayed_work_sync(&bma254->work);

		bma254->delay = value;

		if (bma254->enable)
			schedule_delayed_work(&bma254->work, msecs_to_jiffies(bma254->delay));
	} else {
		pr_err("%s, same delay\n", __func__);
	}
	pr_info("%s,delay %d\n", __func__, bma254->delay);
done:
	return size;
}

static ssize_t bma254_delay_show(struct device *dev,
			  struct device_attribute *attr,
		      char *buf)
{
	struct bma254_data *bma254 = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", bma254->delay);
}

static DEVICE_ATTR(enable,
		   S_IRUGO|S_IWUSR|S_IWGRP,
		   bma254_enable_show,
		   bma254_enable_store
		   );
static DEVICE_ATTR(delay,
		   S_IRUGO|S_IWUSR|S_IWGRP,
		   bma254_delay_show,
		   bma254_delay_store
		   );

static struct attribute *bma254_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_delay.attr,
	NULL
};

static struct attribute_group bma254_attribute_group = {
	.attrs = bma254_attributes
};

static int bma254_do_calibrate(struct bma254_data *bma254, int enable)
{
	struct file *cal_filp;
	int sum[3] = { 0, };
	int err;
	mm_segment_t old_fs;

	if (enable) {
                int data[3] = { 0, };
                int i;
		for (i = 0; i < CALIBRATION_DATA_AMOUNT; i++) {
			err = bma254_get_data(bma254, data, false);
			if (err < 0) {
				pr_err("%s : failed in the %dth loop\n", __func__, i);
				return err;
			}

			sum[0] += data[0];
			sum[1] += data[1];
			sum[2] += data[2];
		}

		bma254->cal_data[0] = (sum[0] / CALIBRATION_DATA_AMOUNT);
		bma254->cal_data[1] = (sum[1] / CALIBRATION_DATA_AMOUNT);
		bma254->cal_data[2] = (sum[2] / CALIBRATION_DATA_AMOUNT);

		if(bma254->cal_data[2] > 0)
			bma254->cal_data[2] -= MAX_ACCEL_1G;
		else if(bma254->cal_data[2] < 0)
			bma254->cal_data[2] += MAX_ACCEL_1G;

	} else {
		bma254->cal_data[0] = 0;
		bma254->cal_data[1] = 0;
		bma254->cal_data[2] = 0;
	}

	pr_info("%s: cal data (%d,%d,%d)\n", __func__,
			bma254->cal_data[0], bma254->cal_data[1],
				bma254->cal_data[2]);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CAL_PATH,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0666);
	if (IS_ERR(cal_filp)) {
		pr_err("%s: Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		return err;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&bma254->cal_data, 3 * sizeof(s32), &cal_filp->f_pos);
	if (err != 3 * sizeof(s32)) {
		pr_err("%s: Can't write the cal data to file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return err;
}


static ssize_t bma254_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_DEV_NAME);
}

static ssize_t bma254_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_DEV_VENDOR);
}

static ssize_t bma254_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct bma254_data *bma254 = dev_get_drvdata(dev);
	int xyz[3] = {0, };
	int ret;

	if (!bma254->enable) {
		bma254_activate(bma254, true);
#if defined(CONFIG_MACH_KANAS3G_CTC)||defined(CONFIG_MACH_KANAS3G_CMCC)
		msleep(300);
#elif defined (CONFIG_MACH_VICTORLTE_CTC)
		msleep(50);
#endif
		schedule_delayed_work(&bma254->work, msecs_to_jiffies(bma254->delay));
	}

	ret = bma254_get_data(bma254, xyz, true);
	if (ret < 0) {
		pr_err("%s, data error(%d)\n", __func__, ret);
	}
	if (!bma254->enable) {
		cancel_delayed_work_sync(&bma254->work);
		bma254_activate(bma254, false);
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		xyz[0], xyz[1], xyz[2]);
}

static ssize_t bma254_calibartion_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct bma254_data *bma254 = dev_get_drvdata(dev);
	int ret;

	ret = bma254_open_calibration(bma254);
	if (ret < 0) {
		pr_err("%s, bma254_open_calibration(%d)\n", __func__, ret);
	}
	if (bma254->cal_data[0] == 0 &&
		bma254->cal_data[1] == 0 &&
		bma254->cal_data[2] == 0)
		ret = 0;
	else
		ret = 1;
	return snprintf(buf, PAGE_SIZE, "%d %d %d %d\n", ret,
		bma254->cal_data[0], bma254->cal_data[1], bma254->cal_data[2]);
}

static ssize_t bma254_calibartion_store(struct device *dev,
	struct device_attribute *attr,const char *buf, size_t count)
{
	struct bma254_data *bma254 = dev_get_drvdata(dev);
	int value;
	int err;
	char tmp[64];

	err = kstrtoint(buf, 10, &value);
	if (err) {
		pr_err("%s, kstrtoint failed.", __func__);
		return -EINVAL;
	}

	err = bma254_do_calibrate(bma254, value);
	if (err < 0)
		pr_err("%s, bma254_do_calibrate(%d)\n", __func__, err);
	else
		err = 0;
	count = sprintf(tmp, "%d\n", err);
	return count;
}

#ifdef CONFIG_BMA254_SMART_ALERT
static ssize_t bma254_reactive_enable_show(struct device *dev,
					struct device_attribute
						*attr, char *buf)
{
	struct bma254_data *bma254 = dev_get_drvdata(dev);
	pr_info("%s: %d\n", __func__, atomic_read(&bma254->reactive_state));
	return snprintf(buf, PAGE_SIZE, "%d\n",
		atomic_read(&bma254->reactive_state));
}

static ssize_t bma254_reactive_enable_store(struct device *dev,
					struct device_attribute
						*attr, const char *buf,
							size_t count)
{
	struct bma254_data *bma254 = dev_get_drvdata(dev);
	bool onoff = false;
	bool factory_test = false;
	unsigned long value = 0;
	int err = count;

	if (strict_strtoul(buf, 10, &value)) {
		err = -EINVAL;
		return err;
	}

	pr_info("%s: %lu\n", __func__, value);

	if(bma254->accsns_activate_flag == value)
	{
		pr_err("%s: duplicate value. Discard\n", __func__);
	       return count;
	}

	switch (value) {
	case 0:
		bma254->accsns_activate_flag = 0;
		break;
	case 1:
		bma254->accsns_activate_flag = 1;
		onoff = true;
		break;
	case 2:
		bma254->accsns_activate_flag = 1;
		onoff = true;
		factory_test = true;
		break;
	default:
		//err = -EINVAL;
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return count;
	}

	if (!bma254->pin_check_fail) {
		if (bma254->IRQ) {
			if (!value) {
				disable_irq_wake(bma254->IRQ);
				disable_irq(bma254->IRQ);
			} else {
				enable_irq(bma254->IRQ);
				enable_irq_wake(bma254->IRQ);
			}
		}
		mutex_lock(&bma254->data_mutex);
		atomic_set(&bma254->reactive_enable, onoff);

		if (bma254->IRQ) {
			bma254_set_motion_interrupt(bma254,
				onoff, factory_test);
		}
		atomic_set(&bma254->reactive_state, false);
		mutex_unlock(&bma254->data_mutex);
		pr_info("%s: value = %lu, onoff = %d, state =%d\n",
			__func__, value,
			atomic_read(&bma254->reactive_enable),
			atomic_read(&bma254->reactive_state));
	}
	return count;
}
#endif

static DEVICE_ATTR(name, 0440, bma254_name_show, NULL);
static DEVICE_ATTR(vendor, 0440, bma254_vendor_show, NULL);
static DEVICE_ATTR(raw_data, 0440, bma254_raw_data_show, NULL);
static DEVICE_ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
	bma254_calibartion_show, bma254_calibartion_store);
#ifdef CONFIG_BMA254_SMART_ALERT
static DEVICE_ATTR(reactive_alert, S_IRUGO|S_IWUSR|S_IWGRP,
		bma254_reactive_enable_show, bma254_reactive_enable_store);
#endif

static struct device_attribute *bma254_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_raw_data,
	&dev_attr_calibration,
#ifdef CONFIG_BMA254_SMART_ALERT
	&dev_attr_reactive_alert,
#endif
	NULL,
};

#ifdef CONFIG_BMA254_SMART_ALERT
static void bma254_work_func_alert(struct work_struct *work)
{
	int result;

	struct bma254_data *bma254 = container_of(work,
		struct bma254_data, alert_work);
	result = bma254_get_motion_interrupt(bma254);
	if (result || bma254->factory_mode) {
		/*handle motion recognition*/
		atomic_set(&bma254->reactive_state, true);
		bma254->factory_mode = false;

		pr_info("%s: motion interrupt happened\n",
			__func__);
		wake_lock_timeout(&bma254->reactive_wake_lock,
			msecs_to_jiffies(2000));
		bma254_i2c_write(bma254, BMA254_INT_ENABLE1_REG, 0x00);
	}
}

irqreturn_t bma254_acc_irq_thread(int irq, void *dev)
{
	struct bma254_data *data = dev;
	pr_info("%s\n", __func__);
	schedule_work(&data->alert_work);
	return IRQ_HANDLED;
}

static int bma254_setup_irq(struct bma254_data *bma254)
{
	int irq = -1;
	struct bma254_platform_data *pdata = bma254->pdata;
	int rc;
	pr_info("%s\n", __func__);

	rc = gpio_request(pdata->p_out, "gpio_bma254_int");
	if (rc < 0) {
		pr_err("%s: gpio %d request failed (%d)\n",
			__func__, pdata->p_out, rc);
		return rc;
	}

	rc = gpio_direction_input(pdata->p_out);
	if (rc < 0) {
		pr_err("%s: failed to set gpio %d as input (%d)\n",
			__func__, pdata->p_out, rc);
		goto err_gpio_direction_input;
	}

	irq = gpio_to_irq(pdata->p_out);

	if (irq > 0) {
		rc = request_threaded_irq(irq,
			NULL, bma254_acc_irq_thread,
			IRQF_TRIGGER_RISING,
			"accelerometer", bma254);

		pr_info("%s: irq = %d\n", __func__, irq);

		if (rc < 0) {
			pr_err("%s request_threaded_irq fail err=%d\n",
				__func__, rc);
			return rc;
		}
		/* start with interrupts disabled */
		disable_irq(irq);
	}
	bma254->IRQ = irq;
	goto done;

err_gpio_direction_input:
	gpio_free(pdata->p_out);
done:
	return rc;
}
#endif

#ifdef CONFIG_SENSORS_POWERCONTROL
static int bma254_regulator_onoff(struct device *dev, bool onoff)
{
	struct regulator *bma_vdd;

	pr_info("%s %s\n", __func__, (onoff) ? "on" : "off");

	bma_vdd = devm_regulator_get(dev, "bma254-vdd");
	if (IS_ERR(bma_vdd)) {
		pr_err("%s: cannot get bma_vdd\n", __func__);
		return -ENOMEM;
	}

	if (onoff) {
		regulator_enable(bma_vdd);
	} else {
		regulator_disable(bma_vdd);
	}

	devm_regulator_put(bma_vdd);
	msleep(10);

	return 0;
}
#endif
static int bma254_parse_dt(struct bma254_data *data, struct device *dev)
{
	struct device_node *this_node= dev->of_node;
#ifdef CONFIG_BMA254_SMART_ALERT
	struct bma254_platform_data *pdata = data->pdata;
	enum of_gpio_flags flags;
#endif
	u32 temp;

	if (this_node == NULL) {
		pr_err("%s,this_node is empty\n", __func__);
		return -ENODEV;
	}

#ifdef CONFIG_BMA254_SMART_ALERT
	pdata->p_out = of_get_named_gpio_flags(this_node, "bma254,irq-gpio",
				0, &flags);
	if (pdata->p_out < 0) {
		pr_err("%s : get irq_gpio(%d) error\n", __func__, pdata->p_out);
		return -ENODEV;
	}else {
		pr_info("%s, get irq_gpio(%d)\n", __func__, pdata->p_out);
	}
#endif

	if (of_property_read_u32(this_node, "bma254,position", &temp) < 0) {
		pr_err("%s,get position(%d) error\n", __func__, temp);
		return -ENODEV;
	} else {
		data->position = (u8)temp;
		pr_info("%s, position(%d)\n", __func__, data->position);
		return 0;
	}
}

static int bma254_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct bma254_data *bma254;
	struct input_dev *dev;
#ifdef CONFIG_BMA254_SMART_ALERT
	int err = 0;
	struct bma254_platform_data *pdata;
#endif

	pr_info("%s, is called\n", __func__);

	if (client == NULL) {
		pr_err("%s, client doesn't exist\n", __func__);
		ret = -ENOMEM;
		return ret;
	}
#ifdef CONFIG_SENSORS_POWERCONTROL
	ret = bma254_regulator_onoff(&client->dev, true);
	if (ret) {
		pr_err("%s, Power Up Failed\n", __func__);
		return ret;
	}
#endif
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s,client not i2c capable\n", __func__);
		return -ENOMEM;
	}

	ret = i2c_smbus_read_word_data(client, BMA254_CHIP_ID);
	if ((ret & 0x00ff) != 0xfa) {
		pr_err("%s,i2c failed(%x)\n", __func__, ret & 0x00ff);
		ret = -ENOMEM;
		return ret;
	} else {
		pr_err("%s,chip id %x\n", __func__, ret & 0x00ff);
	}

	bma254 = kzalloc(sizeof(struct bma254_data), GFP_KERNEL);
	if (!bma254) {
		pr_err("%s, kzalloc error\n", __func__);
		ret = -ENOMEM;
		return ret;
	}

#ifdef CONFIG_BMA254_SMART_ALERT
	if(client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct bma254_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory \n");
			kfree(bma254);

			return -ENOMEM;
		}
	}else{
		pr_err("%s,this_node is empty\n", __func__);
		kfree(bma254);
		return -ENODEV;
	}

	bma254->pdata = pdata;
#endif
	ret = bma254_parse_dt(bma254, &client->dev);
	if (ret) {
		pr_err("%s, get gpio is failed\n", __func__);
		goto parse_dt_err;
	}

	i2c_set_clientdata(client, bma254);
	bma254->client = client;

	bma254_activate(bma254, false);

	dev = input_allocate_device();
	if (!dev){
		goto input_allocate_device_err;
	}
	dev->name = "accelerometer";
	dev->id.bustype = BUS_I2C;
#if !defined(CONFIG_MACH_VICTORLTE) && !defined(CONFIG_MACH_VICTOR3GDSDTV_LTN)
	dev->dev.parent = &client->dev;
#endif
#ifdef REPORT_ABS
	input_set_capability(dev, EV_ABS, ABS_MISC);
	input_set_abs_params(dev, ABS_X, ABSMIN, ABSMAX, 0, 0);
	input_set_abs_params(dev, ABS_Y, ABSMIN, ABSMAX, 0, 0);
	input_set_abs_params(dev, ABS_Z, ABSMIN, ABSMAX, 0, 0);
#else
	input_set_capability(dev, EV_REL, REL_X);
	input_set_capability(dev, EV_REL, REL_Y);
	input_set_capability(dev, EV_REL, REL_Z);
#endif
	input_set_drvdata(dev, bma254);

	ret = input_register_device(dev);
	if (ret < 0) {
		pr_err("%s,sysfs_create_group failed\n", __func__);
		goto input_register_device_err;
	}
	bma254->input = dev;

#ifdef CONFIG_BMA254_SMART_ALERT
	pr_info("%s:  HW_rev success %d\n", __func__, system_rev);
	INIT_WORK(&bma254->alert_work, bma254_work_func_alert);
	wake_lock_init(&bma254->reactive_wake_lock, WAKE_LOCK_SUSPEND,
		"reactive_wake_lock");
	err = bma254_setup_irq(bma254);
	if (err) {
		bma254->pin_check_fail = true;
		pr_err("%s: could not setup irq\n", __func__);
		goto err_setup_irq;
	}
	mutex_init(&bma254->data_mutex);
#endif

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0) {
		input_unregister_device(dev);
		return ret;
	}
	ret = sysfs_create_group(&bma254->input->dev.kobj,
			&bma254_attribute_group);
	if (ret < 0) {
		pr_err("%s,sysfs_create_group failed\n", __func__);
		sensors_remove_symlink(&bma254->input->dev.kobj,
			bma254->input->name);
		goto sysfs_create_group_err;
	}

	INIT_DELAYED_WORK(&bma254->work, bma254_work_func);
	bma254->delay = MAX_DELAY;
	bma254->enable = 0;

	ret = sensors_register(bma254->dev, bma254,
		bma254_attrs, "accelerometer_sensor");
	if (ret < 0) {
		pr_info("%s: could not sensors_register\n", __func__);
		goto sensors_register_err;
	}

#ifdef CONFIG_SENSORS_POWERCONTROL
	bma254_regulator_onoff(&client->dev, false);
#endif
	pr_info("[SENSOR]: %s - Probe done!(chip pos : %d)\n",
		__func__, bma254->position);

	return 0;

sensors_register_err:
	sysfs_remove_group(&bma254->input->dev.kobj,
			&bma254_attribute_group);
	sensors_remove_symlink(&bma254->input->dev.kobj, bma254->input->name);
sysfs_create_group_err:
	input_unregister_device(bma254->input);
#ifdef CONFIG_BMA254_SMART_ALERT
err_setup_irq:
	wake_lock_destroy(&bma254->reactive_wake_lock);
#endif
input_register_device_err:
	input_free_device(dev);
input_allocate_device_err:
parse_dt_err:
	kfree(bma254);
	pr_err("[SENSOR]: %s - Probe fail!\n", __func__);
#ifdef CONFIG_SENSORS_POWERCONTROL
	bma254_regulator_onoff(&client->dev, false);
#endif
	return ret;
}

static int bma254_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	pr_info("%s, enable(%d)\n", __func__, bma254->enable);
	if (bma254->enable) {
		cancel_delayed_work_sync(&bma254->work);
		bma254_activate(bma254, false);
	}

	return 0;
}

static int bma254_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma254_data *bma254 = i2c_get_clientdata(client);

	pr_info("%s, enable(%d)\n", __func__, bma254->enable);
	if (bma254->enable) {
		bma254_activate(bma254, true);
		schedule_delayed_work(&bma254->work, msecs_to_jiffies(bma254->delay));
	}

	return 0;
}

static const struct dev_pm_ops bma254_dev_pm_ops = {
	.suspend = bma254_suspend,
	.resume = bma254_resume,
};

static const u16 normal_i2c[] = { I2C_CLIENT_END };

static const struct i2c_device_id bma254_device_id[] = {
	{"bma254", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, bma254_device_id);

static struct of_device_id bma254_match_table[] = {
	{ .compatible = "bma254",},
	{},
};

MODULE_DEVICE_TABLE(of, bma254_match_table);

static struct i2c_driver bma254_i2c_driver = {
	.driver = {
		.name = "bma254",
		.owner = THIS_MODULE,
		.of_match_table = bma254_match_table,
		.pm = &bma254_dev_pm_ops,
	},
	.probe		= bma254_probe,
	.id_table	= bma254_device_id,
	.address_list = normal_i2c,
};

module_i2c_driver(bma254_i2c_driver);

MODULE_AUTHOR("daehan.wi@samsung.com");
MODULE_DESCRIPTION("G-Sensor Driver for BMA254");
MODULE_LICENSE("GPL");
