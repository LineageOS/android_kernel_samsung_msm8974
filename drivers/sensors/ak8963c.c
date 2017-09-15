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
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include "sensors_core.h"
#include "ak8963c_reg.h"

/* Rx buffer size. i.e ST,TMPS,H1X,H1Y,H1Z*/
#define SENSOR_DATA_SIZE		8
#define AK8963C_DEFAULT_DELAY		200000000LL

#define I2C_M_WR                        0 /* for i2c Write */
#define I2c_M_RD                        1 /* for i2c Read */

#define VENDOR_NAME                     "AKM"
#define MODEL_NAME                      "AK8963C"
#define MODULE_NAME                     "magnetic_sensor"

#define AK8963C_TOP_LOWER_RIGHT         0
#define AK8963C_TOP_LOWER_LEFT          1
#define AK8963C_TOP_UPPER_LEFT          2
#define AK8963C_TOP_UPPER_RIGHT         3
#define AK8963C_BOTTOM_LOWER_RIGHT      4
#define AK8963C_BOTTOM_LOWER_LEFT       5
#define AK8963C_BOTTOM_UPPER_LEFT       6
#define AK8963C_BOTTOM_UPPER_RIGHT      7

struct ak8963c_v {
	union {
		s16 v[3];
		struct {
			s16 x;
			s16 y;
			s16 z;
		};
	};
};

struct ak8963c_p {
	struct i2c_client *client;
	struct input_dev *input;
	struct device *factory_device;
	struct ak8963c_v magdata;
	struct mutex lock;
	struct completion data_ready;
	struct delayed_work work;

	atomic_t delay;
	atomic_t enable;

	wait_queue_head_t state_wq;
	u8 asa[3];
	u32 chip_pos;

	int irq;
	int m_rst_n;
	int m_sensor_int;
};

static int ak8963c_i2c_read(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *buf)
{
	int ret;
	struct i2c_msg msg[2];

	msg[0].addr = client->addr;
	msg[0].flags = I2C_M_WR;
	msg[0].len = 1;
	msg[0].buf = &reg_addr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - i2c bus read error %d\n",
			__func__, ret);
		return ret;
	}

	return 0;
}

static int ak8963c_i2c_write(struct i2c_client *client,
		unsigned char reg_addr, unsigned char buf)
{
	int ret;
	struct i2c_msg msg;
	unsigned char w_buf[2];

	w_buf[0] = reg_addr;
	w_buf[1] = buf;

	msg.addr = client->addr;
	msg.flags = I2C_M_WR;
	msg.len = 2;
	msg.buf = (char *)w_buf;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - i2c bus write error %d\n",
			__func__, ret);
		return ret;
	}

	return 0;
}

static int ak8963c_i2c_read_block(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *buf, unsigned char len)
{
	int i, ret = 0;

	for (i = 0; i < len; i++)
		ret += ak8963c_i2c_read(client, reg_addr + i, &buf[i]);

	return ret;
}

static void ak8963c_disable_irq(struct ak8963c_p *data)
{
	disable_irq(data->irq);
	if (try_wait_for_completion(&data->data_ready)) {
		/* we actually got the interrupt before we could disable it
		 * so we need to enable again to undo our disable since the
		 * irq_handler already disabled it
		 */
		enable_irq(data->irq);
	}
}

static int ak8963c_ecs_set_mode_power_down(struct ak8963c_p *data)
{
	unsigned char reg;
	int ret;

	reg = AK8963C_CNTL1_POWER_DOWN;
	ret = ak8963c_i2c_write(data->client, AK8963C_REG_CNTL1, reg);

	return ret;
}

static int ak8963c_ecs_set_mode(struct ak8963c_p *data, char mode)
{
	u8 reg;
	int ret;

	switch (mode) {
	case AK8963C_CNTL1_SNG_MEASURE:
		reg = AK8963C_CNTL1_SNG_MEASURE;
		ret = ak8963c_i2c_write(data->client, AK8963C_REG_CNTL1, reg);
		break;
	case AK8963C_CNTL1_FUSE_ACCESS:
		reg = AK8963C_CNTL1_FUSE_ACCESS;
		ret = ak8963c_i2c_write(data->client, AK8963C_REG_CNTL1, reg);
		break;
	case AK8963C_CNTL1_POWER_DOWN:
		reg = AK8963C_CNTL1_SNG_MEASURE;
		ret = ak8963c_ecs_set_mode_power_down(data);
		break;
	case AK8963C_CNTL1_SELF_TEST:
		reg = AK8963C_CNTL1_SELF_TEST;
		ret = ak8963c_i2c_write(data->client, AK8963C_REG_CNTL1, reg);
		break;
	default:
		return -EINVAL;
	}

	if (ret < 0)
		return ret;

	/* Wait at least 300us after changing mode. */
	udelay(300);

	return 0;
}

static irqreturn_t ak8963c_irq_handler(int irq, void *pdata)
{
	struct ak8963c_p *data = pdata;

	disable_irq_nosync(irq);
	complete(&data->data_ready);
	return IRQ_HANDLED;
}

static int ak8963c_wait_for_data_ready(struct ak8963c_p *data)
{
	int ret;
	int data_ready = gpio_get_value(data->m_sensor_int);

	if (data_ready)
		return 0;

	enable_irq(data->irq);

	ret = wait_for_completion_timeout(&data->data_ready, 2 * HZ);
	if (ret > 0)
		return 0;

	ak8963c_disable_irq(data);

	if (ret == 0) {
		pr_err("[SENSOR]: %s - wait timed out\n", __func__);
		return -ETIMEDOUT;
	}

	pr_err("[SENSOR]: %s - wait restart\n", __func__);
	return ret;
}

static int ak8963c_read_mag_xyz(struct ak8963c_p *data, struct ak8963c_v *mag)
{
	u8 temp[SENSOR_DATA_SIZE];
	int ret;

	mutex_lock(&data->lock);
	ret = ak8963c_ecs_set_mode(data, AK8963C_CNTL1_SNG_MEASURE);
	if (ret) {
		mutex_unlock(&data->lock);
		return ret;
	}

	ret = ak8963c_wait_for_data_ready(data);
	if (ret) {
		mutex_unlock(&data->lock);
		return ret;
	}

	ret = ak8963c_i2c_read_block(data->client, AK8963C_REG_ST1,
			temp, SENSOR_DATA_SIZE);
	mutex_unlock(&data->lock);

	mag->x = temp[1] | (temp[2] << 8);
	mag->y = temp[3] | (temp[4] << 8);
	mag->z = temp[5] | (temp[6] << 8);

	remap_sensor_data(mag->v, data->chip_pos);

	return ret;
}

static void ak8963c_work_func(struct work_struct *work)
{
	struct ak8963c_v mag;
	struct ak8963c_p *data = container_of((struct delayed_work *)work,
			struct ak8963c_p, work);
	unsigned long delay = nsecs_to_jiffies(atomic_read(&data->delay));

	ak8963c_read_mag_xyz(data, &mag);
	input_report_rel(data->input, REL_X, mag.x);
	input_report_rel(data->input, REL_Y, mag.y);
	input_report_rel(data->input, REL_Z, mag.z);
	input_sync(data->input);
	data->magdata = mag;

	schedule_delayed_work(&data->work, delay);
}

static void ak8963c_set_enable(struct ak8963c_p *data, int enable)
{
	int pre_enable = atomic_read(&data->enable);

	if (enable) {
		if (pre_enable == 0) {
			ak8963c_ecs_set_mode(data, AK8963C_CNTL1_SNG_MEASURE);
			schedule_delayed_work(&data->work,
				nsecs_to_jiffies(atomic_read(&data->delay)));
			atomic_set(&data->enable, 1);
		}
	} else {
		if (pre_enable == 1) {
			ak8963c_ecs_set_mode(data, AK8963C_CNTL1_POWER_DOWN);
			cancel_delayed_work_sync(&data->work);
			atomic_set(&data->enable, 0);
		}
	}
}

static ssize_t ak8963c_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ak8963c_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&data->enable));
}

static ssize_t ak8963c_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u8 enable;
	int ret;
	struct ak8963c_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &enable);
	if (ret) {
		pr_err("[SENSOR]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	pr_info("[SENSOR]: %s - new_value = %u\n", __func__, enable);
	if ((enable == 0) || (enable == 1))
		ak8963c_set_enable(data, (int)enable);

	return size;
}

static ssize_t ak8963c_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ak8963c_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&data->delay));
}

static ssize_t ak8963c_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	int ret;
	int64_t delay;
	struct ak8963c_p *data = dev_get_drvdata(dev);

	ret = kstrtoll(buf, 10, &delay);
	if (ret) {
		pr_err("[SENSOR]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	atomic_set(&data->delay, (int64_t)delay);
	pr_info("[SENSOR]: %s - poll_delay = %lld\n", __func__, delay);

	return size;
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		ak8963c_delay_show, ak8963c_delay_store);
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		ak8963c_enable_show, ak8963c_enable_store);

static struct attribute *ak8963c_attributes[] = {
	&dev_attr_poll_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group ak8963c_attribute_group = {
	.attrs = ak8963c_attributes
};

static int ak8963c_selftest(struct ak8963c_p *data, int *sf)
{
	u8 temp[6], reg;
	s16 x, y, z;
	int retry_count = 0;
	int ready_count = 0;
	int ret;
retry:
	mutex_lock(&data->lock);
	/* power down */
	reg = AK8963C_CNTL1_POWER_DOWN;
	ak8963c_i2c_write(data->client, AK8963C_REG_CNTL1, reg);

	/* read device info */
	ak8963c_i2c_read_block(data->client, AK8963C_REG_WIA, temp, 2);
	pr_info("[SENSOR]: %s - device id = 0x%x, info = 0x%x\n",
		__func__, temp[0], temp[1]);

	/* set ATSC self test bit to 1 */
	reg = 0x40;
	ak8963c_i2c_write(data->client, AK8963C_REG_ASTC, reg);

	/* start self test */
	reg = AK8963C_CNTL1_SELF_TEST;
	ak8963c_i2c_write(data->client, AK8963C_REG_CNTL1, reg);

	/* wait for data ready */
	while (ready_count < 10) {
		msleep(20);
		ret = ak8963c_i2c_read(data->client, AK8963C_REG_ST1, &reg);
		if ((reg == 1) && (ret == 0))
			break;
		ready_count++;
	}

	ak8963c_i2c_read_block(data->client, AK8963C_REG_HXL,
			temp, sizeof(temp));

	/* set ATSC self test bit to 0 */
	reg = 0x00;
	ak8963c_i2c_write(data->client, AK8963C_REG_ASTC, reg);
	mutex_unlock(&data->lock);

	x = temp[0] | (temp[1] << 8);
	y = temp[2] | (temp[3] << 8);
	z = temp[4] | (temp[5] << 8);

	/* Hadj = (H*(Asa+128))/256 */
	x = (x * (data->asa[0] + 128)) >> 8;
	y = (y * (data->asa[1] + 128)) >> 8;
	z = (z * (data->asa[2] + 128)) >> 8;

	pr_info("[SENSOR]: %s - self test x = %d, y = %d, z = %d\n",
		__func__, x, y, z);
	if ((x >= -200) && (x <= 200))
		pr_info("[SENSOR]: %s - x passed self test, -200<=x<=200\n",
			__func__);
	else
		pr_info("[SENSOR]: %s - x failed self test, -200<=x<=200\n",
			__func__);
	if ((y >= -200) && (y <= 200))
		pr_info("[SENSOR]: %s - y passed self test, -200<=y<=200\n",
			__func__);
	else
		pr_info("[SENSOR]: %s - y failed self test, -200<=y<=200\n",
			__func__);
	if ((z >= -3200) && (z <= -800))
		pr_info("[SENSOR]: %s - z passed self test, -3200<=z<=-800\n",
			__func__);
	else
		pr_info("[SENSOR]: %s - z failed self test, -3200<=z<=-800\n",
			__func__);

	sf[0] = x;
	sf[1] = y;
	sf[2] = z;

	if (((x >= -200) && (x <= 200)) &&
		((y >= -200) && (y <= 200)) &&
		((z >= -3200) && (z <= -800))) {
		pr_info("%s, Selftest is successful.\n", __func__);
		return 1;
	} else {
		if (retry_count < 5) {
			retry_count++;
			pr_warn("############################################");
			pr_warn("%s, retry_count=%d\n", __func__, retry_count);
			pr_warn("############################################");
			goto retry;
		} else {
			pr_err("[SENSOR]: %s - Selftest is failed.\n",
				__func__);
			return 0;
		}
	}
}

static ssize_t ak8963c_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t ak8963c_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", MODEL_NAME);
}

static ssize_t ak8963c_get_asa(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ak8963c_p *data  = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u,%u,%u\n",
			data->asa[0], data->asa[1], data->asa[2]);
}

static ssize_t ak8963c_get_selftest(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int sf[3] = {0,};

	ret = ak8963c_selftest(dev_get_drvdata(dev), sf);
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n",
			ret, sf[0], sf[1], sf[2]);
}

static ssize_t ak8963c_check_registers(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 temp[13], reg;
	struct ak8963c_p *data = dev_get_drvdata(dev);

	mutex_lock(&data->lock);
	/* power down */
	reg = AK8963C_CNTL1_POWER_DOWN;
	ak8963c_i2c_write(data->client, AK8963C_REG_CNTL1, reg);

	/* get the value */
	ak8963c_i2c_read_block(data->client, AK8963C_REG_WIA, temp, 11);
	ak8963c_i2c_read(data->client, AK8963C_REG_ASTC, &temp[11]);
	ak8963c_i2c_read(data->client, AK8963C_REG_I2CDIS, &temp[12]);
	mutex_unlock(&data->lock);

	return snprintf(buf, PAGE_SIZE,
			"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			temp[0], temp[1], temp[2], temp[3], temp[4], temp[5],
			temp[6], temp[7], temp[8], temp[9], temp[10], temp[11],
			temp[12]);
}

static ssize_t ak8963c_check_cntl(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 reg;
	int ret = 0;
	struct ak8963c_p *data = dev_get_drvdata(dev);

	mutex_lock(&data->lock);
	/* power down */
	reg = AK8963C_CNTL1_POWER_DOWN;

	ret = ak8963c_i2c_write(data->client, AK8963C_REG_CNTL1, reg);
	ret += ak8963c_i2c_read(data->client, AK8963C_REG_CNTL1, &reg);
	mutex_unlock(&data->lock);

	return snprintf(buf, PAGE_SIZE, "%s\n",
			(((reg == AK8963C_CNTL1_POWER_DOWN) &&
			(ret == 0)) ? "OK" : "NG"));
}

static ssize_t ak8963c_get_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	bool success;
	struct ak8963c_p *data = dev_get_drvdata(dev);

	if ((data->asa[0] == 0) | (data->asa[0] == 0xff)
		| (data->asa[1] == 0) | (data->asa[1] == 0xff)
		| (data->asa[2] == 0) | (data->asa[2] == 0xff))
		success = false;
	else
		success = true;

	return snprintf(buf, PAGE_SIZE, "%s\n", (success ? "OK" : "NG"));
}

static ssize_t ak8963c_adc(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	bool success = false;
	u8 temp[SENSOR_DATA_SIZE] = {0,};
	int ret, retry = 0;
	struct ak8963c_p *data = dev_get_drvdata(dev);
	struct ak8963c_v mag = data->magdata;

	if (atomic_read(&data->enable) == 1) {
		success = true;
		goto exit;
	}

	msleep(20);

retry_adc:
	mutex_lock(&data->lock);
	ret = ak8963c_ecs_set_mode(data, AK8963C_CNTL1_SNG_MEASURE);
	if (ret) {
		mutex_unlock(&data->lock);
		goto exit;
	}
	ret = ak8963c_wait_for_data_ready(data);
	if (ret) {
		mutex_unlock(&data->lock);
		goto exit;
	}
	ret = ak8963c_i2c_read_block(data->client, AK8963C_REG_ST1,
			temp, SENSOR_DATA_SIZE);
	if (ret < 0) {
		pr_err("[SENSOR] %s: failed to read mag data\n", __func__);
		mutex_unlock(&data->lock);
		goto exit;
	}

	ak8963c_ecs_set_mode(data, AK8963C_CNTL1_POWER_DOWN);
	mutex_unlock(&data->lock);

	/* buf[0] is status1, buf[7] is status2 */
	if ((temp[0] == 0) | (temp[7] == 1)) {
		if (retry++ < 3)
			goto retry_adc;

		success = false;
	}
	else
		success = true;

	mag.x = (temp[2] << 8) + temp[1];
	mag.y = (temp[4] << 8) + temp[3];
	mag.z = (temp[6] << 8) + temp[5];

	pr_info("[SENSOR]: %s - ST1=%d, x=%d, y=%d, z=%d, ST2=%d\n",
		__func__, temp[0], mag.x, mag.y, mag.z, temp[7]);

exit:
	return snprintf(buf, PAGE_SIZE, "%s,%d,%d,%d\n",
			(success ? "OK" : "NG"), mag.x, mag.y, mag.z);
}

static ssize_t ak8963c_raw_data_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ak8963c_p *data = dev_get_drvdata(dev);
	struct ak8963c_v mag = data->magdata;
	int ret, retry = 0;
	u8 temp[SENSOR_DATA_SIZE] = {0,};

	if (atomic_read(&data->enable) == 1)
		goto exit;

	msleep(20);

retry_rawdata:
	mutex_lock(&data->lock);
	ret = ak8963c_ecs_set_mode(data, AK8963C_CNTL1_SNG_MEASURE);
	if (ret) {
		mutex_unlock(&data->lock);
		goto exit;
	}
	ret = ak8963c_wait_for_data_ready(data);
	if (ret) {
		mutex_unlock(&data->lock);
		goto exit;
	}
	ret = ak8963c_i2c_read_block(data->client, AK8963C_REG_ST1,
			temp, SENSOR_DATA_SIZE);
	if (ret < 0) {
		pr_err("[SENSOR] %s: failed to read mag data\n", __func__);
		mutex_unlock(&data->lock);
		goto exit;
	}

	ak8963c_ecs_set_mode(data, AK8963C_CNTL1_POWER_DOWN);
	mutex_unlock(&data->lock);

	if (temp[0] & 0x01) {
		mag.x = (temp[2] << 8) + temp[1];
		mag.y = (temp[4] << 8) + temp[3];
		mag.z = (temp[6] << 8) + temp[5];
	} else {
		pr_err("[SENSOR]: %s - invalid raw data(st1 = %d)\n",
					__func__, temp[0] & 0x01);
		pr_info("[SENSOR]: %s - ST1=%d, x=%d, y=%d, z=%d, ST2=%d\n",
			__func__, temp[0], mag.x, mag.y, mag.z, temp[7]);

		if (retry++ < 3)
			goto retry_rawdata;
	}

exit:
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", mag.x, mag.y, mag.z);
}

static DEVICE_ATTR(name, S_IRUGO, ak8963c_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, ak8963c_vendor_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO, ak8963c_raw_data_read, NULL);
static DEVICE_ATTR(adc, S_IRUGO, ak8963c_adc, NULL);
static DEVICE_ATTR(dac, S_IRUGO, ak8963c_check_cntl, NULL);
static DEVICE_ATTR(chk_registers, S_IRUGO, ak8963c_check_registers, NULL);
static DEVICE_ATTR(selftest, S_IRUGO, ak8963c_get_selftest, NULL);
static DEVICE_ATTR(asa, S_IRUGO, ak8963c_get_asa, NULL);
static DEVICE_ATTR(status, S_IRUGO, ak8963c_get_status, NULL);

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_raw_data,
	&dev_attr_adc,
	&dev_attr_dac,
	&dev_attr_chk_registers,
	&dev_attr_selftest,
	&dev_attr_asa,
	&dev_attr_status,
	NULL,
};

static int ak8963c_read_fuserom(struct ak8963c_p *data)
{
	unsigned char reg;
	int ret;

	/* put into fuse access mode to read asa data */
	reg = AK8963C_CNTL1_FUSE_ACCESS;
	ret = ak8963c_i2c_write(data->client, AK8963C_REG_CNTL1, reg);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - unable to enter fuse rom mode\n",
			__func__);
		goto exit_default_value;
	}

	ret = ak8963c_i2c_read_block(data->client, AK8963C_FUSE_ASAX,
			data->asa, sizeof(data->asa));
	if (ret < 0) {
		pr_err("[SENSOR]: %s - unable to load factory sensitivity "\
			"adjust values\n", __func__);
		goto exit_default_value;
	} else
		pr_info("[SENSOR]: %s - asa_x = %u, asa_y = %u, asa_z = %u\n",
			__func__, data->asa[0], data->asa[1], data->asa[2]);

	reg = AK8963C_CNTL1_POWER_DOWN;
	ret = ak8963c_i2c_write(data->client, AK8963C_REG_CNTL1, reg);
	if (ret < 0)
		pr_err("[SENSOR] Error in setting power down mode\n");

	return 0;

exit_default_value:
	data->asa[0] = 0;
	data->asa[1] = 0;
	data->asa[2] = 0;

	return ret;
}

static int ak8963c_setup_pin(struct ak8963c_p *data)
{
	int ret, irq;

	ret = gpio_request(data->m_rst_n, "M_RST_N");
	if (ret < 0) {
		pr_err("[SENSOR] %s - gpio %d request failed (%d)\n",
			__func__, data->m_rst_n, ret);
		goto exit;
	}

	ret = gpio_direction_output(data->m_rst_n, 1);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - failed to set gpio %d as input (%d)\n",
			__func__, data->m_rst_n, ret);
		goto exit_reset_gpio;
	}

	gpio_set_value(data->m_rst_n, 0);
	udelay(10);
	gpio_set_value(data->m_rst_n, 1);
	udelay(100);

	ret = gpio_request(data->m_sensor_int, "M_SENSOR_INT");
	if (ret < 0) {
		pr_err("[SENSOR]: %s - gpio %d request failed (%d)\n",
			__func__, data->m_sensor_int, ret);
		goto exit_reset_gpio;
	}

	ret = gpio_direction_input(data->m_sensor_int);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - failed to set gpio %d as input (%d)\n",
			__func__, data->m_sensor_int, ret);
		goto exit_int_gpio;
	}

	irq = gpio_to_irq(data->m_sensor_int);

	/* trigger high so we don't miss initial interrupt if it
	 * is already pending
	 */
	ret = request_irq(irq, ak8963c_irq_handler,
		IRQF_TRIGGER_RISING | IRQF_ONESHOT, "ak8963c_int", data);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - request_irq(%d) fail for gpio %d (%d)\n",
			__func__, irq,
			data->m_sensor_int, ret);
		goto exit_int_gpio;
	}

	/* start with interrupt disabled until the driver is enabled */
	data->irq = irq;
	ak8963c_disable_irq(data);

	goto exit;

exit_int_gpio:
	gpio_free(data->m_sensor_int);
exit_reset_gpio:
	gpio_free(data->m_rst_n);
exit:
	return ret;
}

static int ak8963c_input_init(struct ak8963c_p *data)
{
	int ret = 0;
	struct input_dev *dev;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_X);
	input_set_capability(dev, EV_REL, REL_Y);
	input_set_capability(dev, EV_REL, REL_Z);
	input_set_drvdata(dev, data);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		return ret;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0) {
		input_unregister_device(dev);
		return ret;
	}

	/* sysfs node creation */
	ret = sysfs_create_group(&dev->dev.kobj, &ak8963c_attribute_group);
	if (ret < 0) {
		sensors_remove_symlink(&data->input->dev.kobj,
			data->input->name);
		input_unregister_device(dev);
		return ret;
	}

	data->input = dev;
	return 0;
}

static int ak8963c_parse_dt(struct ak8963c_p *data, struct device *dev)
{
	struct device_node *dNode = dev->of_node;
	enum of_gpio_flags flags;

	if (dNode == NULL)
		return -ENODEV;

	data->m_sensor_int = of_get_named_gpio_flags(dNode,
		"ak8963c-i2c,m_sensor_int-gpio", 0, &flags);
	if (data->m_sensor_int < 0) {
		pr_err("[SENSOR]: %s - get m_sensor_int error\n", __func__);
		return -ENODEV;
	}

	data->m_rst_n = of_get_named_gpio_flags(dNode,
		"ak8963c-i2c,m_rst_n-gpio", 0, &flags);
	if (data->m_rst_n < 0) {
		pr_err("[SENSOR]: %s - m_rst_n error\n", __func__);
		return -ENODEV;
	}

	if (of_property_read_u32(dNode,
			"ak8963c-i2c,chip_pos", &data->chip_pos) < 0)
		data->chip_pos = AK8963C_TOP_LOWER_RIGHT;

	return 0;
}

static int ak8963c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct ak8963c_p *data = NULL;

	pr_info("[SENSOR]: %s - Probe Start!\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[SENSOR]: %s - i2c_check_functionality error\n",
			__func__);
		goto exit;
	}

	data = kzalloc(sizeof(struct ak8963c_p), GFP_KERNEL);
	if (data == NULL) {
		pr_err("[SENSOR]: %s - kzalloc error\n", __func__);
		ret = -ENOMEM;
		goto exit_kzalloc;
	}

	ret = ak8963c_parse_dt(data, &client->dev);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - of_node error\n", __func__);
		ret = -ENODEV;
		goto exit_of_node;
	}

	init_completion(&data->data_ready);
	ret = ak8963c_setup_pin(data);
	if (ret) {
		pr_err("[SENSOR]: %s - could not setup pin\n", __func__);
		goto exit_setup_pin;
	}

	i2c_set_clientdata(client, data);
	data->client = client;

	ret = ak8963c_ecs_set_mode_power_down(data);
	if (ret < 0) {
		pr_err("[SENSOR]: %s - ak8963c_ecs_set_mode_power_down fail"\
			"(err=%d)\n", __func__, ret);
		goto exit_set_mode_power_down;
	}

	/* input device init */
	ret = ak8963c_input_init(data);
	if (ret < 0)
		goto exit_input_init;

	sensors_register(data->factory_device, data, sensor_attrs, MODULE_NAME);

	/* workqueue init */
	INIT_DELAYED_WORK(&data->work, ak8963c_work_func);
	mutex_init(&data->lock);
	init_waitqueue_head(&data->state_wq);

	atomic_set(&data->delay, AK8963C_DEFAULT_DELAY);
	atomic_set(&data->enable, 0);

	ak8963c_read_fuserom(data);

	pr_info("[SENSOR]: %s - Probe done!(chip pos : %d)\n",
		__func__, data->chip_pos);

	return 0;

exit_input_init:
exit_set_mode_power_down:
	free_irq(data->irq, data);
	gpio_free(data->m_rst_n);
	gpio_free(data->m_sensor_int);
exit_setup_pin:
exit_of_node:
	kfree(data);
exit_kzalloc:
exit:
	pr_err("[SENSOR]: %s - Probe fail!\n", __func__);
	return ret;
}

static void ak8963c_shutdown(struct i2c_client *client)
{
	struct ak8963c_p *data = (struct ak8963c_p *)i2c_get_clientdata(client);

	pr_info("[SENSOR]: %s\n", __func__);
	if (atomic_read(&data->enable) == 1) {
		ak8963c_ecs_set_mode(data, AK8963C_CNTL1_POWER_DOWN);
		cancel_delayed_work_sync(&data->work);
	}
}

static int __devexit ak8963c_remove(struct i2c_client *client)
{
	struct ak8963c_p *data = (struct ak8963c_p *)i2c_get_clientdata(client);

	if (atomic_read(&data->enable) == 1)
		ak8963c_set_enable(data, 0);

	free_irq(data->irq, data);
	gpio_free(data->m_rst_n);
	gpio_free(data->m_sensor_int);
	mutex_destroy(&data->lock);

	sensors_unregister(data->factory_device, sensor_attrs);
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);

	sysfs_remove_group(&data->input->dev.kobj, &ak8963c_attribute_group);
	input_unregister_device(data->input);
	kfree(data);

	return 0;
}

static int ak8963c_suspend(struct device *dev)
{
	struct ak8963c_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == 1) {
		ak8963c_ecs_set_mode(data, AK8963C_CNTL1_POWER_DOWN);
		cancel_delayed_work_sync(&data->work);
	}

	return 0;
}

static int ak8963c_resume(struct device *dev)
{
	struct ak8963c_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == 1) {
		ak8963c_ecs_set_mode(data, AK8963C_CNTL1_SNG_MEASURE);
		schedule_delayed_work(&data->work,
		nsecs_to_jiffies(atomic_read(&data->delay)));
	}

	return 0;
}

static struct of_device_id ak8963c_match_table[] = {
	{ .compatible = "ak8963c-i2c",},
	{},
};

static const struct i2c_device_id ak8963c_id[] = {
	{ "ak8963c_match_table", 0 },
	{ }
};

static const struct dev_pm_ops ak8963c_pm_ops = {
	.suspend = ak8963c_suspend,
	.resume = ak8963c_resume
};

static struct i2c_driver ak8963c_driver = {
	.driver = {
		.name	= MODEL_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = ak8963c_match_table,
		.pm = &ak8963c_pm_ops
	},
	.probe		= ak8963c_probe,
	.shutdown	= ak8963c_shutdown,
	.remove		= __devexit_p(ak8963c_remove),
	.id_table	= ak8963c_id,
};

static int __init ak8963c_init(void)
{
	return i2c_add_driver(&ak8963c_driver);
}

static void __exit ak8963c_exit(void)
{
	i2c_del_driver(&ak8963c_driver);
}

module_init(ak8963c_init);
module_exit(ak8963c_exit);

MODULE_DESCRIPTION("AKM8963 compass driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
