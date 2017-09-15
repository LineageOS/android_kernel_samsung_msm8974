/*
 * Copyright (c) 2012 SAMSUNG
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <mach/hardware.h>
#include <linux/wakelock.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/platform_data/gp2ap030.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/fs.h>

#include "../iio.h"
#include "../sysfs.h"
#include "../events.h"

#define MIN_HZ	5
#define MAX_HZ 100
#define SENSOR_ENABLE	1
#define SENSOR_DISABLE	0

#define THR_REG_LSB(data, reg) \
	{ \
		reg = (u8)data & 0xff; \
	}
#define THR_REG_MSB(data, reg) \
	{ \
		reg = (u8)data >> 8; \
	}

#define CAL_PATH 		"/efs/prox_cal_data"
#ifdef CONFIG_SENSORS
#include "../../../sensors/sensors_core.h"
#define GP2A_VENDOR		"SHARP"
#define GP2A_CHIP_ID	"GP2AP030"
#define PROX_READ_NUM	10
#define GP2A_PROX_MAX 1023
#define GP2A_PROX_MIN 0
#endif

struct gp2a_light_data {
	struct gp2a_data *data;
	struct iio_dev *indio_light;

	u8 light_data;
};

#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
struct gp2a_prox_data {
	struct gp2a_data *data;
	struct iio_dev *indio_prox;

	u8 prox_data;
};
#endif

struct gp2a_data {
	struct i2c_client *client;
	struct gp2a_light_data *light_data;

	struct delayed_work light_work;
	struct mutex data_mutex;
	struct mutex light_mutex;

	u8 light_enable;
	u8 prox_enable;
	u8 proximity_detection;
	u8 lightsensor_mode;


	int light_delay;
	int lux;

#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
	int irq;
	int gpio;
	int vled_gpio;
	struct gp2a_prox_data *prox_data;
	struct wake_lock prx_wake_lock;

	int offset_value;
	int cal_result;
	uint16_t threshold_high;
	bool offset_cal_high;
#endif
#ifdef CONFIG_SENSORS
	struct device *light_sensor_device;
#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
	struct device *prox_sensor_device;
	struct delayed_work prox_avg_work;

	int prox_delay;
	int avg[3];
#endif
#endif
};

/* initial value for sensor register */
#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
#define COL 8
static u8 gp2a_reg[COL][2] = {
	/*  {Regster, Value} */
	{0x01, 0x63},	/*PRST :01(4 cycle at Detection/Non-detection), ALSresolution :16bit, range *128   //0x1F -> 5F by sharp */
	/*{0x02, 0x1A},	*//*ALC : 0, INTTYPE : 1, PS mode resolution : 12bit, range*1 */
	{0x02, 0x5A},	/*ALC : 0, INTTYPE : 1, PS mode resolution : 12bit, range*1 */
	{0x03, 0x3C},	/*LED drive current 110mA, Detection/Non-detection judgment output */
	{0x08, 0x08},	/*PS mode LTH(Loff):  (??mm) */
	{0x09, 0x00},	/*PS mode LTH(Loff) : */
	{0x0A, 0x0A},	/*PS mode HTH(Lon) : (??mm) */
	{0x0B, 0x00},	/*PS mode HTH(Lon) : */
	{0x00, 0xC0}	/*alternating mode (PS+ALS), TYPE=1 (0:externel 1:auto calculated mode)*/
};
#endif

static int gp2a_i2c_read(struct gp2a_data *gp2a,
	u8 reg, unsigned char *rbuf, int len)
{
	int ret = -1;
	struct i2c_msg msg[2];
	struct i2c_client *client = gp2a->client;

	if (unlikely((client == NULL) || (!client->adapter)))
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = I2C_M_WR;
	msg[0].len = 1;
	msg[0].buf = &reg;

	msg[1].addr = client->addr;
	msg[1].flags = I2c_M_RD;
	msg[1].len = len;
	msg[1].buf = rbuf;

	ret = i2c_transfer(client->adapter, &msg[0], 2);

	if (unlikely(ret < 0))
		pr_err("i2c transfer error ret=%d\n", ret);

	return ret;
}

static int gp2a_i2c_write(struct gp2a_data *gp2a,
	u8 reg, u8 *val)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int retry = 2;
	struct i2c_client *client = gp2a->client;

	if (unlikely((client == NULL) || (!client->adapter)))
		return -ENODEV;

	do {
		data[0] = reg;
		data[1] = *val;

		msg->addr = client->addr;
		msg->flags = I2C_M_WR;
		msg->len = 2;
		msg->buf = data;

		err = i2c_transfer(client->adapter, msg, 1);

		if (err >= 0)
			return 0;
	} while (--retry > 0);
	pr_err(" i2c transfer error(%d)\n", err);
	return err;
}

static int gp2a_regulator_onoff(struct device *dev, bool onoff)
{
	struct regulator *gp2a_vcc, *gp2a_lvs1;

	gp2a_vcc = devm_regulator_get(dev, "gp2a030a-vcc");
	if (IS_ERR(gp2a_vcc)) {
		pr_err("%s: cannot get gp2a_vcc\n", __func__);
		return -ENOMEM;
	}

	gp2a_lvs1 = devm_regulator_get(dev, "gp2a030a-lvs1");
	if (IS_ERR(gp2a_lvs1)) {
		pr_err("%s: cannot get gp2a_vcc\n", __func__);
		devm_regulator_put(gp2a_vcc);
		return -ENOMEM;
	}

	if (onoff) {
		regulator_enable(gp2a_vcc);
		msleep(5);
		regulator_enable(gp2a_lvs1);
	} else {
		regulator_disable(gp2a_lvs1);
		msleep(5);
		regulator_disable(gp2a_vcc);
	}

	devm_regulator_put(gp2a_vcc);
	devm_regulator_put(gp2a_lvs1);
	msleep(10);

	return 0;
}

static int lightsensor_onoff(u8 onoff, struct gp2a_data *data)
{
	u8 value;

	pr_debug("%s : light_sensor onoff = %d\n", __func__, onoff);

	if (onoff) {
		/*in calling, must turn on proximity sensor */
		if (data->prox_enable == 0) {
			value = 0x01;
			gp2a_i2c_write(data, COMMAND4, &value);
			value = 0x63;
			gp2a_i2c_write(data, COMMAND2, &value);
			/*OP3 : 1(operating mode)
			  OP2 : 1(coutinuous operating mode)
			  OP1 : 01(ALS mode) TYPE=0(auto)*/
			value = 0xD0;
			gp2a_i2c_write(data, COMMAND1, &value);
			/* other setting have defualt value. */
		}
		msleep(3);
	} else {
		/*in calling, must turn on proximity sensor */
		if (data->prox_enable == 0) {
			value = 0x00;	/*shutdown mode */
			gp2a_i2c_write(data, COMMAND1, &value);
		}
	}

	return 0;
}

int gp2a_get_lux(struct gp2a_data *data)
{
	unsigned char get_data[4] = { 0, };
	int d0_raw_data;
	int d1_raw_data;
	int d0_data;
	int d1_data;
	int lx = 0;
	u8 value;
	int light_alpha = 0;
	int light_beta = 0;
	static int lx_prev;
	int ret ;
	int d0_boundary = 92;

	mutex_lock(&data->data_mutex);
	ret = gp2a_i2c_read(data, DATA0_LSB, get_data, sizeof(get_data));
	mutex_unlock(&data->data_mutex);
	if (ret < 0)
		return lx_prev;
	d0_raw_data = (get_data[1] << 8) | get_data[0]; /* clear */
	d1_raw_data = (get_data[3] << 8) | get_data[2]; /* IR */

	if (100 * d1_raw_data <= 40 * d0_raw_data) {
		light_alpha = 935;
		light_beta = 0;
	} else if (100 * d1_raw_data <= 54 * d0_raw_data) {
		light_alpha = 3039;
		light_beta = 5176;
	} else if (100 * d1_raw_data <= d0_boundary * d0_raw_data) {
		light_alpha = 494;
		light_beta = 533;
	} else {
		light_alpha = 0;
		light_beta = 0;
	}

	if (data->lightsensor_mode) {	/* HIGH_MODE */
		d0_data = d0_raw_data * 16;
		d1_data = d1_raw_data * 16;
	} else {		/* LOW_MODE */
		d0_data = d0_raw_data;
		d1_data = d1_raw_data;
	}

	if (d0_data < 3) {
		lx = 0;
	} else if (data->lightsensor_mode == 0
		&& (d0_raw_data >= 16000 || d1_raw_data >= 16000)
		&& (d0_raw_data <= 16383 && d1_raw_data <= 16383)) {
		lx = lx_prev;
	} else if (100 * d1_data > d0_boundary * d0_data) {
		lx = lx_prev;
		return lx;
	} else {
		lx = (int)((light_alpha / 10 * d0_data * 33)
			- (light_beta / 10 * d1_data * 33)) / 1000;
	}

	lx_prev = lx;

	if (data->lightsensor_mode) {	/* HIGH MODE */
		if (d0_raw_data < 1000) {
			pr_info("%s: change to LOW_MODE detection=%d\n",
				__func__, data->proximity_detection);
			data->lightsensor_mode = 0;	/* change to LOW MODE */

			value = 0x0C;
			gp2a_i2c_write(data, COMMAND1, &value);

			if (data->proximity_detection)
				value = 0x23;
			else
				value = 0x63;
			gp2a_i2c_write(data, COMMAND2, &value);

			if (data->prox_enable)
				value = 0xCC;
			else
				value = 0xDC;
			gp2a_i2c_write(data, COMMAND1, &value);
		}
	} else {		/* LOW MODE */
		if (d0_raw_data > 16000 || d1_raw_data > 16000) {
			pr_info("%s: change to HIGH_MODE detection=%d\n",
				__func__, data->proximity_detection);
			/* change to HIGH MODE */
			data->lightsensor_mode = 1;

			value = 0x0C;
			gp2a_i2c_write(data, COMMAND1, &value);

			if (data->proximity_detection)
				value = 0x27;
			else
				value = 0x67;
			gp2a_i2c_write(data, COMMAND2, &value);

			if (data->prox_enable)
				value = 0xCC;
			else
				value = 0xDC;
			gp2a_i2c_write(data, COMMAND1, &value);
		}
	}

	return lx;
}

static void gp2a_work_func_light(struct work_struct *work)
{
	int err;
	struct gp2a_data *data = container_of((struct delayed_work *)work,
						struct gp2a_data, light_work);
	static int count;

	data->lux = gp2a_get_lux(data);

	/* detecting 0 after 3.6sec, set the register again. */
	if (data->lux == 0) {
		count++;
		if (count * data->light_delay > 3600) {
			lightsensor_onoff(1, data);
			count = 0;
			pr_info("%s: register reset\n", __func__);
		}
	} else
		count = 0;

	err = iio_push_event(iio_priv_to_dev(data->light_data),
			IIO_UNMOD_EVENT_CODE(IIO_LIGHT,
				0,
				IIO_EV_TYPE_THRESH,
				IIO_EV_DIR_EITHER),
			iio_get_time_ns());

	if (err)
		pr_err("%s, Could not push IIO_LIGHT event\n", __func__);

	if (data->light_enable)
		schedule_delayed_work(&data->light_work,
				msecs_to_jiffies(data->light_delay));
}

static ssize_t gp2a_light_attr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int val, err;
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct gp2a_light_data *light_data = iio_priv(indio_dev);
	struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);

	err = kstrtoint(buf, 10, &val);

	if (err < 0) {
		pr_err("%s, kstrtoint failed\n", __func__);
		return -EINVAL;
	} else {
		pr_info("%s, %d\n", __func__, val);
	}

	switch (this_attr->address) {
		case IIO_ATTR_ENABLE:
			if (val == SENSOR_DISABLE || val == SENSOR_ENABLE) {
				if (light_data->data->light_enable != (u8)val) {
					light_data->data->light_enable = (u8)val;

					if (light_data->data->light_enable == SENSOR_ENABLE) {
						pr_info("%s, light sensor enable\n", __func__);
						lightsensor_onoff(1, light_data->data);
						schedule_delayed_work(&light_data->data->light_work,
							msecs_to_jiffies(light_data->data->light_delay));
					} else {
						pr_info("%s, light sensor disable\n", __func__);
						cancel_delayed_work_sync(&light_data->data->light_work);
						lightsensor_onoff(0, light_data->data);
					}
				}
			} else {
				pr_err("%s, wrong cmd for enable\n", __func__);
			}
			break;

		case IIO_ATTR_DELAY:
			mutex_lock(&light_data->data->light_mutex);
			if (MIN_HZ <= val && val <= MAX_HZ) {
				if (light_data->data->light_enable)
					cancel_delayed_work_sync(&light_data->data->light_work);

				light_data->data->light_delay = MSEC_PER_SEC / val;
				pr_info("%s, set delay(%d)\n", __func__, light_data->data->light_delay);
				if (light_data->data->light_enable)
					schedule_delayed_work(&light_data->data->light_work,
							msecs_to_jiffies(light_data->data->light_delay));
			} else {
				pr_err("%s, wrong cmd for HZ(%d)\n", __func__, val);
			}
			mutex_unlock(&light_data->data->light_mutex);
			break;

		default:
			pr_err("%s, wrong address for sysfs\n", __func__);
			return -EINVAL;
	}
	return count;
}

static ssize_t gp2a_light_attr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct gp2a_light_data *light_data = iio_priv(indio_dev);
	struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);

	switch (this_attr->address) {
		case IIO_ATTR_ENABLE:
			pr_info("%s, sensor enable : %d\n",
				__func__, light_data->data->light_enable);
			return snprintf(buf, PAGE_SIZE, "%d\n",
				light_data->data->light_enable);

		case IIO_ATTR_DELAY:
			pr_info("%s, sensor delay : %d\n",
				__func__, light_data->data->light_delay);
			return snprintf(buf, PAGE_SIZE, "%d\n",
				light_data->data->light_delay);

		default:
			return -EINVAL;
	}
}

static IIO_DEVICE_ATTR(enable, S_IRUGO | S_IWUSR,
	gp2a_light_attr_show, gp2a_light_attr_store, IIO_ATTR_ENABLE);
static IIO_DEVICE_ATTR(delay, S_IRUGO | S_IWUSR,
	gp2a_light_attr_show, gp2a_light_attr_store, IIO_ATTR_DELAY);

static struct attribute *gp2a_light_attributes[] = {
	&iio_dev_attr_enable.dev_attr.attr,
	&iio_dev_attr_delay.dev_attr.attr,
	NULL,
};

static struct attribute_group gp2a_light_attribute_group = {
	.name = "light",
	.attrs = gp2a_light_attributes,
};

static int gp2a_light_raw(struct iio_dev *indio_dev,
			      struct iio_chan_spec const *chan,
			      int *val, int *val2, long m)
{
	struct gp2a_light_data *light_data = iio_priv(indio_dev);

	pr_info("%s, Reporting lux(%d) to user\n", __func__, light_data->data->lux);
	*val = light_data->data->lux;

	return IIO_VAL_INT;
}


static struct iio_info light_info = {
	.driver_module = THIS_MODULE,
	.attrs = &gp2a_light_attribute_group,
	.read_raw = gp2a_light_raw,
};

static const struct iio_chan_spec gp2a_light_channels[] = {
	{
		.type = IIO_LIGHT,
		.indexed = 1,
		.channel = 0,
		.processed_val = IIO_PROCESSED,
		.event_mask = (IIO_EV_BIT(IIO_EV_TYPE_THRESH,
				IIO_EV_DIR_EITHER)),
	}
};
#ifdef CONFIG_SENSORS
static ssize_t gp2a_light_raw_data_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);

	unsigned char get_data[4] = { 0, };
	int d0_raw_data;
	int d1_raw_data;
	int ret = 0;

	mutex_lock(&data->data_mutex);
	ret = gp2a_i2c_read(data, DATA0_LSB, get_data, sizeof(get_data));
	mutex_unlock(&data->data_mutex);

	if (ret < 0)
		pr_err("%s i2c err: %d\n", __func__, ret) ;

	d0_raw_data = (get_data[1] << 8) | get_data[0];	/* clear */
	d1_raw_data = (get_data[3] << 8) | get_data[2];	/* IR */

	return snprintf(buf, PAGE_SIZE, "%d,%d\n", d0_raw_data, d1_raw_data);
}

static ssize_t gp2a_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", GP2A_VENDOR);
}

static ssize_t gp2a_name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", GP2A_CHIP_ID);
}

static struct device_attribute dev_attr_light_raw_data =
	__ATTR(raw_data, S_IRUSR | S_IRGRP,
	gp2a_light_raw_data_show, NULL);
static struct device_attribute dev_attr_light_lux =
	__ATTR(lux, S_IRUSR | S_IRGRP,
	gp2a_light_raw_data_show, NULL);
static struct device_attribute dev_attr_light_vendor =
	__ATTR(vendor, S_IRUSR | S_IRGRP,
	gp2a_vendor_show, NULL);
static struct device_attribute dev_attr_light_name =
	__ATTR(name, S_IRUSR | S_IRGRP,
	gp2a_name_show, NULL);

static struct device_attribute *light_sensor_attrs[] = {
	&dev_attr_light_raw_data,
	&dev_attr_light_lux,
	&dev_attr_light_vendor,
	&dev_attr_light_name,
	NULL,
};
#endif

#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
static int gp2a_prox_open_calibration(struct gp2a_data  *data)
{
	struct file *cal_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CAL_PATH,
		O_RDONLY, S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("%s Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->read(cal_filp,
		(char *)&data->offset_value,
			sizeof(int), &cal_filp->f_pos);
	if (err != sizeof(int)) {
		pr_err("%s Can't read the cal data from file\n",
			__func__);
		err = -EIO;
	}

	pr_info("%s (%d)\n", __func__, data->offset_value);

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);

	return err;
}

static int gp2a_prox_onoff(u8 onoff, struct gp2a_data  *data)
{
	u8 value;
	int i;
	int err = 0;

	pr_info("%s : proximity turn on/off = %d\n", __func__, onoff);

	/* already on light sensor, so must simultaneously
		turn on light sensor and proximity sensor */
	if (onoff) {
		gpio_set_value(data->vled_gpio, 1);
		for (i = 0; i < COL; i++) {
			err = gp2a_i2c_write(data, gp2a_reg[i][0],
				&gp2a_reg[i][1]);
			if (err < 0)
				pr_err("%s : turnning on error i = %d, err=%d\n",
					__func__, i, err);
			data->lightsensor_mode = 0;
		}
	} else {
		if (data->light_enable) {
			if (data->lightsensor_mode)
				value = 0x67; /*resolution :16bit, range: *8(HIGH) */
			else
				value = 0x63; /* resolution :16bit, range: *128(LOW) */
			gp2a_i2c_write(data, COMMAND2, &value);
			/* OP3 : 1(operating mode)
			   OP2 : 1(coutinuous operating mode) OP1 : 01(ALS mode) */
			value = 0xD0;
			gp2a_i2c_write(data, COMMAND1, &value);
		} else {
			value = 0x00;	/*shutdown mode */
			gp2a_i2c_write(data, (u8) (COMMAND1), &value);
		}
		gpio_set_value(data->vled_gpio, 0);
	}

	return 0;
}

static ssize_t gp2a_prox_attr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int val, err;
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct gp2a_prox_data *prox_data = iio_priv(indio_dev);
	struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);
	uint16_t thrd = 0;
	u8 reg;
	err = kstrtoint(buf, 10, &val);

	if (err < 0) {
		pr_err("%s, kstrtoint failed\n", __func__);
		return -EINVAL;
	} else {
		pr_info("%s, %d\n", __func__, val);
	}

	switch (this_attr->address) {
		case IIO_ATTR_ENABLE:
			if (val == SENSOR_DISABLE || val == SENSOR_ENABLE) {
				if (prox_data->data->prox_enable!= (u8)val) {
					prox_data->data->prox_enable = (u8)val;

					if (prox_data->data->prox_enable == SENSOR_ENABLE) {
						pr_info("%s, prox sensor enable\n", __func__);
						gp2a_prox_onoff(1, prox_data->data);

						err = gp2a_prox_open_calibration(prox_data->data);
						if (err < 0 && err != -ENOENT)
							pr_err("%s gp2a_prox_open_offset() failed\n",
								__func__);
						else {
							thrd = gp2a_reg[3][1]+(prox_data->data->offset_value);
							THR_REG_LSB(thrd, reg);
							gp2a_i2c_write(prox_data->data, gp2a_reg[3][0], &reg);
							THR_REG_MSB(thrd, reg);
							gp2a_i2c_write(prox_data->data, gp2a_reg[4][0], &reg);

							thrd = gp2a_reg[5][1]+(prox_data->data->offset_value);
							THR_REG_LSB(thrd, reg);
							gp2a_i2c_write(prox_data->data, gp2a_reg[5][0], &reg);
							THR_REG_MSB(thrd, reg);
							gp2a_i2c_write(prox_data->data, gp2a_reg[6][0], &reg);
						}
						
						err = iio_push_event(iio_priv_to_dev(prox_data),
								IIO_UNMOD_EVENT_CODE(IIO_PROXIMITY,
									0,
									IIO_EV_TYPE_THRESH,
									IIO_EV_DIR_EITHER),
								iio_get_time_ns());

						if (err)
							pr_err("%s, Could not push IIO_LIGHT event\n", __func__);

						enable_irq_wake(prox_data->data->irq);
						enable_irq(prox_data->data->irq);
					} else {
						pr_info("%s, prox sensor disable\n", __func__);
						gp2a_prox_onoff(0, prox_data->data);
						disable_irq(prox_data->data->irq);
						disable_irq_wake(prox_data->data->irq);
					}
				}
			} else {
				pr_err("%s, wrong cmd for enable\n", __func__);
			}
			break;
		case IIO_ATTR_DELAY:
			break;
		default:
			pr_err("%s, wrong address for sysfs\n", __func__);
			return -EINVAL;
	}
	return count;
}

static ssize_t gp2a_prox_attr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct gp2a_prox_data *prox_data = iio_priv(indio_dev);
	struct iio_dev_attr *this_attr = to_iio_dev_attr(attr);

	switch (this_attr->address) {
		case IIO_ATTR_ENABLE:
				pr_info("%s, sensor enable : %d\n",
				__func__, prox_data->data->prox_enable);
			return snprintf(buf, PAGE_SIZE, "%d\n",
				prox_data->data->prox_enable);
		case IIO_ATTR_DELAY:
			return snprintf(buf, PAGE_SIZE, "%d\n",
				prox_data->data->prox_enable);
		default:
			return -EINVAL;
	}
}

static IIO_DEVICE_ATTR(prox_enable, S_IRUGO | S_IWUSR,
	gp2a_prox_attr_show, gp2a_prox_attr_store, IIO_ATTR_ENABLE);
static IIO_DEVICE_ATTR(prox_delay, S_IRUGO | S_IWUSR,
	gp2a_prox_attr_show, gp2a_prox_attr_store, IIO_ATTR_DELAY);

static struct attribute *gp2a_prox_attributes[] = {
	&iio_dev_attr_prox_enable.dev_attr.attr,
	&iio_dev_attr_prox_delay.dev_attr.attr,
	NULL,
};

static struct attribute_group gp2a_prox_attribute_group = {
	.name = "prox",
	.attrs = gp2a_prox_attributes,
};

static int gp2a_prox_raw(struct iio_dev *indio_dev,
			      struct iio_chan_spec const *chan,
			      int *val, int *val2, long m)
{
	struct gp2a_prox_data *prox_data = iio_priv(indio_dev);
	unsigned char value;
	int ret;

	ret = gp2a_i2c_read(prox_data->data, COMMAND1, &value, sizeof(value));
	if (ret < 0) {
		pr_info("%s, read data error\n", __func__);
	} else {
		pr_info("%s, read data %d, %d\n", __func__, value & 0x08, !(value & 0x08));
		prox_data->data->proximity_detection = !(value & 0x08);
	}

	if (!(value & 0x08)) {
		if (prox_data->data->lightsensor_mode == 0)
			value = 0x63;
		else
			value = 0x67;
		ret = gp2a_i2c_write(prox_data->data, COMMAND2, &value);
	} else {
		if (prox_data->data->lightsensor_mode == 0)
			value = 0x23;
		else
			value = 0x27;
		ret = gp2a_i2c_write(prox_data->data, COMMAND2, &value);
	}

	value = 0xCC;
	ret = gp2a_i2c_write(prox_data->data, COMMAND1, &value);

	ret = gp2a_i2c_read(prox_data->data, COMMAND1, &value, sizeof(value));
	if (ret < 0)
		pr_info("%s, read data error\n", __func__);

	pr_info("%s, detection=%d, mode=%d\n", __func__,
		prox_data->data->proximity_detection, prox_data->data->lightsensor_mode);

	*val = prox_data->data->proximity_detection;

	return IIO_VAL_INT;
}

static struct iio_info prox_info = {
	.driver_module = THIS_MODULE,
	.attrs = &gp2a_prox_attribute_group,
	.read_raw = gp2a_prox_raw,
};

static const struct iio_chan_spec gp2a_prox_channels[] = {
	{
		.type = IIO_PROXIMITY,
		.indexed = 1,
		.channel = 0,
		.processed_val = IIO_PROCESSED,
		.event_mask = (IIO_EV_BIT(IIO_EV_TYPE_THRESH,
				IIO_EV_DIR_EITHER)),
	}
};
static irqreturn_t gp2a_irq_handler(int irq, void *dev_id)
{
	struct gp2a_data *data = dev_id;
	int err;
	wake_lock_timeout(&data->prx_wake_lock, 3 * HZ);

	if (irq == data->irq) {
		#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
		err = iio_push_event(iio_priv_to_dev(data->prox_data),
				IIO_UNMOD_EVENT_CODE(IIO_PROXIMITY,
					0,
					IIO_EV_TYPE_THRESH,
					IIO_EV_DIR_EITHER),
				iio_get_time_ns());
		
		if (err)
			pr_err("%s, Could not push IIO_LIGHT event\n", __func__);
		else
		#endif
			pr_info("%s, interrupt occured\n", __func__);
		
	} else {
		pr_err("%s, wrong irq occured(%d)\n", __func__, irq);
	}
	return IRQ_HANDLED;
}

static int gp2a_setup_irq(struct gp2a_data *data)
{
	int rc = -EIO;

	pr_err("%s, start\n", __func__);

	rc = gpio_request(data->vled_gpio, "gpio_vled_en");
	if (unlikely(rc < 0)) {
		pr_err("%s: gpio %d request failed (%d)\n",
				__func__, data->vled_gpio, rc);
		goto done;
	}

	rc = gpio_direction_output(data->vled_gpio, 1);
	if (unlikely(rc < 0)) {
		pr_err("%s: failed to set gpio %d as input (%d)\n",
				__func__, data->vled_gpio, rc);
		goto err_gpio_direction_output;
	}

	rc = gpio_request(data->gpio, "gpio_gp2a_prox_out");
	if (unlikely(rc < 0)) {
		pr_err("%s: gpio %d request failed (%d)\n",
				__func__, data->gpio, rc);
		goto done;
	}

	rc = gpio_direction_input(data->gpio);
	if (unlikely(rc < 0)) {
		pr_err("%s: failed to set gpio %d as input (%d)\n",
				__func__, data->gpio, rc);
		goto err_gpio_direction_input;
	}

	data->irq = gpio_to_irq(data->gpio);
	rc = request_threaded_irq(data->irq, NULL,
			gp2a_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"gp2a_prox_int", data);
	if (unlikely(rc < 0)) {
		pr_err("%s: request_irq(%d) failed for gpio %d (%d)\n",
				__func__, data->irq, data->gpio, rc);
		goto err_request_irq;
	}

	disable_irq(data->irq);

	pr_info("%s, success\n", __func__);

	goto done;

err_request_irq:
err_gpio_direction_input:
	gpio_free(data->gpio);
err_gpio_direction_output:
	gpio_free(data->vled_gpio);
done:
	return rc;
}

static int gp2a_parse_dt(struct gp2a_data *data, struct device *dev)
{
	struct device_node *this_node= dev->of_node;
	enum of_gpio_flags flags;

	if (this_node == NULL)
		return -ENODEV;

	data->gpio = of_get_named_gpio_flags(this_node,
						"gp2a030a,irq_gpio", 0, &flags);
	if (data->gpio < 0) {
		pr_err("%s : get irq_gpio(%d) error\n", __func__, data->gpio);
		return -ENODEV;
	}

	data->vled_gpio = of_get_named_gpio_flags(this_node,
						"gp2a030a,vled_gpio", 0, &flags);
	if (data->vled_gpio < 0) {
		pr_err("%s : get irq_gpio(%d) error\n", __func__, data->vled_gpio);
		return -ENODEV;
	}

	return 0;
}

#ifdef CONFIG_SENSORS
static int gp2a_prox_adc_read(struct gp2a_data *data)
{
	int sum[OFFSET_ARRAY_LENGTH];
	int i = OFFSET_ARRAY_LENGTH-1;
	int avg;
	int min = 0;
	int max = 0;
	int total = 0;
	int D2_data;
	unsigned char get_D2_data[2];

	mutex_lock(&data->data_mutex);
	do {
		msleep(50);
		gp2a_i2c_read(data, DATA2_LSB, get_D2_data,
			sizeof(get_D2_data));
		D2_data = (get_D2_data[1] << 8) | get_D2_data[0];
		sum[i] = D2_data;
		if (i == OFFSET_ARRAY_LENGTH - 1) {
			min = sum[i];
			max = sum[i];
		} else {
			if (sum[i] < min)
				min = sum[i];
			else if (sum[i] > max)
				max = sum[i];
		}
		total += sum[i];
	} while (i--);
	mutex_unlock(&data->data_mutex);

	total -= (min + max);
	avg = (int)(total / (OFFSET_ARRAY_LENGTH - 2));
	pr_info("%s offset = %d\n", __func__, avg);

	return avg;
}

static int gp2a_prox_do_calibrate(struct gp2a_data  *data,
			bool do_calib, bool thresh_set)
{
	struct file *cal_filp;
	int err;
	int xtalk_avg = 0;
	int offset_change = 0;
	uint16_t thrd = 0;
	u8 reg;
	mm_segment_t old_fs;

	if (do_calib) {
		if (thresh_set) {
			/* for gp2a_prox_thresh_store */
			data->offset_value =
				data->threshold_high -
				(gp2a_reg[6][1] << 8 | gp2a_reg[5][1]);
		} else {
			/* tap offset button */
			/* get offset value */
			xtalk_avg = gp2a_prox_adc_read(data);
			offset_change =
				(gp2a_reg[6][1] << 8 | gp2a_reg[5][1])
				- DEFAULT_HI_THR;
			if (xtalk_avg < offset_change) {
				/* do not need calibration */
				data->cal_result = 0;
				err = 0;
				goto no_cal;
			}
			data->offset_value = xtalk_avg - offset_change;
		}
		/* update threshold */
		thrd = (gp2a_reg[4][1] << 8 | gp2a_reg[3][1])
			+ (data->offset_value);
		THR_REG_LSB(thrd, reg);
		gp2a_i2c_write(data, gp2a_reg[3][0], &reg);
		THR_REG_MSB(thrd, reg);
		gp2a_i2c_write(data, gp2a_reg[4][0], &reg);

		thrd = (gp2a_reg[4][1] << 8 | gp2a_reg[5][1])
			+(data->offset_value);
		THR_REG_LSB(thrd, reg);
		gp2a_i2c_write(data, gp2a_reg[5][0], &reg);
		THR_REG_MSB(thrd, reg);
		gp2a_i2c_write(data, gp2a_reg[6][0], &reg);

		/* calibration result */
		if (!thresh_set)
			data->cal_result = 1;
	} else {
		/* tap reset button */
		data->offset_value = 0;
		/* update threshold */
		gp2a_i2c_write(data, gp2a_reg[3][0], &gp2a_reg[3][1]);
		gp2a_i2c_write(data, gp2a_reg[4][0], &gp2a_reg[4][1]);
		gp2a_i2c_write(data, gp2a_reg[5][0], &gp2a_reg[5][1]);
		gp2a_i2c_write(data, gp2a_reg[6][0], &gp2a_reg[6][1]);
		/* calibration result */
		data->cal_result = 2;
	}
    
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CAL_PATH,
			O_CREAT | O_TRUNC | O_WRONLY,
			S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("%s Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&data->offset_value, sizeof(int),
			&cal_filp->f_pos);
	if (err != sizeof(int)) {
		pr_err("%s Can't write the cal data to file\n",
			__func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
no_cal:
	return err;
}

static ssize_t gp2a_prox_cal_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int thresh_hi;
	unsigned char get_D2_data[2];
	
    msleep(20);
	gp2a_i2c_read(data, PS_HT_LSB, get_D2_data,
		sizeof(get_D2_data));
	thresh_hi = (get_D2_data[1] << 8) | get_D2_data[0];
	data->threshold_high = thresh_hi;
	return sprintf(buf, "%d,%d\n",
			data->offset_value, data->threshold_high);
}

static ssize_t gp2a_prox_cal_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	bool do_calib;
	int err;

	if (sysfs_streq(buf, "1")) { /* calibrate cancelation value */
		do_calib = true;
	} else if (sysfs_streq(buf, "0")) { /* reset cancelation value */
		do_calib = false;
	} else {
		pr_err("%s invalid value %d\n", __func__, *buf);
		err = -EINVAL;
		goto done;
	}
	err = gp2a_prox_do_calibrate(data, do_calib, false);
	if (err < 0) {
		pr_err("%s  gp2a_prox_store_offset() failed\n",
			__func__);
		goto done;
	} else
		err = size;
done:
	return err;
}

static void gp2a_work_func_prox(struct work_struct *work)
{
	struct gp2a_data *data = container_of((struct delayed_work *)work,
						struct gp2a_data, prox_avg_work);

	int gp2a_prox_value = 0;
	int min = 0, max = 0, avg = 0;
	int i = 0;
	unsigned char raw_data[2] = { 0, };

	for (i = 0; i < PROX_READ_NUM; i++) {
	    mutex_lock(&data->data_mutex);
		gp2a_i2c_read(data, 0x10, raw_data, sizeof(raw_data));
		mutex_unlock(&data->data_mutex);
		gp2a_prox_value = (raw_data[1] << 8) | raw_data[0];

		if (gp2a_prox_value > GP2A_PROX_MAX)
			gp2a_prox_value = GP2A_PROX_MAX;
		if (gp2a_prox_value > GP2A_PROX_MIN) {
			avg += gp2a_prox_value;
			if (!i)
				min = gp2a_prox_value;
			else if (gp2a_prox_value < min)
				min = gp2a_prox_value;
			if (gp2a_prox_value > max)
				max = gp2a_prox_value;
		} else {
			gp2a_prox_value = GP2A_PROX_MIN;
		}
		msleep(40);
	}
	avg /= i;
	data->avg[0] = min;
	data->avg[1] = avg;
	data->avg[2] = max;

	if (data->prox_enable)
		schedule_delayed_work(&data->prox_avg_work,
				msecs_to_jiffies(data->prox_delay));
}

static ssize_t gp2a_pxor_prox_avg_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", data->avg[0],
		data->avg[1], data->avg[2]);
}
static ssize_t gp2a_pxor_prox_avg_store(struct device *dev,
			struct device_attribute *attr,
		    const char *buf, size_t size)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int enable = 0;
	int err = 0;

	err = kstrtoint(buf, 10, &enable);
	if (err < 0) {
		pr_err("%s, kstrtoint failed.", __func__);
	} else {
		pr_info("%s, %d\n", __func__, enable);
		if (enable)
			schedule_delayed_work(&data->prox_avg_work,
				msecs_to_jiffies(data->prox_delay));
		else
			cancel_delayed_work_sync(&data->prox_avg_work);
	}
	return size;
}

static ssize_t gp2a_prox_raw_data_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int d2_data = 0;
	unsigned char raw_data[2] = { 0, };

	mutex_lock(&data->data_mutex);
	gp2a_i2c_read(data, 0x10, raw_data, sizeof(raw_data));
	mutex_unlock(&data->data_mutex);
	d2_data = (raw_data[1] << 8) | raw_data[0];

	return snprintf(buf, PAGE_SIZE, "%d\n", d2_data);
}

static int gp2a_prox_manual_offset(struct gp2a_data  *data, u8 change_on)
{
	struct file *cal_filp;
	int err;
	int16_t thrd;
	u8 reg;
	mm_segment_t old_fs;

	data->offset_value = change_on;
	/* update threshold */
	thrd = gp2a_reg[3][1]+(data->offset_value);
	THR_REG_LSB(thrd, reg);
	gp2a_i2c_write(data, gp2a_reg[3][0], &reg);
	THR_REG_MSB(thrd, reg);
	gp2a_i2c_write(data, gp2a_reg[4][0], &reg);

	thrd = gp2a_reg[5][1]+(data->offset_value);
	THR_REG_LSB(thrd, reg);
	gp2a_i2c_write(data, gp2a_reg[5][0], &reg);
	THR_REG_MSB(thrd, reg);
	gp2a_i2c_write(data, gp2a_reg[6][0], &reg);

	/* calibration result */
	data->cal_result = 1;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CAL_PATH,
			O_CREAT | O_TRUNC | O_WRONLY,
			S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("%s Can't open calibration file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_filp);
		goto done;
	}

	err = cal_filp->f_op->write(cal_filp,
		(char *)&data->offset_value, sizeof(int),
			&cal_filp->f_pos);
	if (err != sizeof(int)) {
		pr_err("%s Can't write the cal data to file\n",
			__func__);
		err = -EIO;
	}

	filp_close(cal_filp, current->files);
done:
	set_fs(old_fs);
	return err;
}

static ssize_t gp2a_prox_cal2_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	u8 change_on;
	int err;

	if (sysfs_streq(buf, "1")) /* change hi threshold by -2 */
		change_on = -2;
	else if (sysfs_streq(buf, "2")) /*change hi threshold by +4 */
		change_on = 4;
	else if (sysfs_streq(buf, "3")) /*change hi threshold by +8 */
		change_on = 8;
	else {
		pr_err("%s invalid value %d\n", __func__, *buf);
		err = -EINVAL;
		goto done;
	}
	err = gp2a_prox_manual_offset(data, change_on);
	if (err < 0) {
		pr_err("%s gp2a_prox_store_offset() failed\n",
			__func__);
		goto done;
	}
done:
	return size;
}

static ssize_t gp2a_prox_thresh_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	int thresh_hi = 0;
	unsigned char get_D2_data[2];

	msleep(20);
	gp2a_i2c_read(data, PS_HT_LSB, get_D2_data,
		sizeof(get_D2_data));
	thresh_hi = (get_D2_data[1] << 8) | get_D2_data[0];
	pr_info("%s THRESHOLD = %d\n", __func__, thresh_hi);

	return sprintf(buf, "prox_threshold = %d\n", thresh_hi);
}

static ssize_t gp2a_prox_thresh_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
	long thresh_value = 0;
	int err = 0;

	err = strict_strtol(buf, 10, &thresh_value);
	if (unlikely(err < 0)) {
		pr_err("%s kstrtoint failed.", __func__);
		goto done;
	}
	data->threshold_high = (uint16_t)thresh_value;
	err = gp2a_prox_do_calibrate(data, true, true);
	if (err < 0) {
		pr_err("%s thresh_store failed\n", __func__);
		goto done;
	}
	msleep(20);
done:
	return size;
}


static ssize_t prox_offset_pass_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *data = dev_get_drvdata(dev);
  
	return sprintf(buf, "%d\n", data->cal_result);
}

static struct device_attribute dev_attr_gp2a_prox_sensor_prox_cal2 =
	__ATTR(prox_cal2, S_IRUGO | S_IWUSR | S_IWGRP,
				NULL, gp2a_prox_cal2_store);
static struct device_attribute dev_attr_gp2a_prox_thresh =
	__ATTR(prox_thresh, S_IRUGO | S_IWUSR | S_IWGRP,
				gp2a_prox_thresh_show, gp2a_prox_thresh_store);
static struct device_attribute dev_attr_gp2a_prox_offset_pass =
	__ATTR(prox_offset_pass, S_IRUGO | S_IWUSR | S_IWGRP,
				prox_offset_pass_show, NULL);
static struct device_attribute dev_attr_prox_cal =
	__ATTR(prox_cal, S_IRUGO | S_IWUSR | S_IWGRP,
				gp2a_prox_cal_show, gp2a_prox_cal_store);
static struct device_attribute dev_attr_prox_prox_avg =
	__ATTR(prox_avg, S_IRUGO | S_IWUSR | S_IWGRP,
	gp2a_pxor_prox_avg_show, gp2a_pxor_prox_avg_store);
static struct device_attribute dev_attr_prox_raw_data =
	__ATTR(raw_data, S_IRUSR | S_IRGRP,
	gp2a_prox_raw_data_show, NULL);
static struct device_attribute dev_attr_prox_state =
	__ATTR(state, S_IRUSR | S_IRGRP,
	gp2a_prox_raw_data_show, NULL);
static struct device_attribute dev_attr_prox_vendor =
	__ATTR(vendor, S_IRUSR | S_IRGRP,
	gp2a_vendor_show, NULL);
static struct device_attribute dev_attr_prox_name =
	__ATTR(name, S_IRUSR | S_IRGRP,
	gp2a_name_show, NULL);

static struct device_attribute *prox_sensor_attrs[] = {
	&dev_attr_prox_state,
	&dev_attr_prox_prox_avg,
	&dev_attr_prox_raw_data,
	&dev_attr_prox_vendor,
	&dev_attr_prox_name,
	&dev_attr_prox_cal,
	&dev_attr_gp2a_prox_thresh,
	&dev_attr_gp2a_prox_offset_pass,
	&dev_attr_gp2a_prox_sensor_prox_cal2,
	NULL,
};
#endif

#endif

static int gp2a_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int err = 0;
	struct gp2a_data *data;
	struct gp2a_light_data *light_data;
	struct iio_dev *light_dev;
#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
	struct gp2a_prox_data *prox_data;
	struct iio_dev *prox_dev;
#endif
	u8 value;

	pr_info("%s, is called\n", __func__);

	if (client == NULL) {
		pr_err("%s, client doesn't exist\n", __func__);
		err = -ENOMEM;
		return err;
	}

	err = gp2a_regulator_onoff(&client->dev, true);
	if (err) {
		pr_err("%s, Power Up Failed\n", __func__);
		return err;
	}

	data = kzalloc(sizeof(struct gp2a_data), GFP_KERNEL);
	if (!data) {
		pr_err("%s, kzalloc error\n", __func__);
		err = -ENOMEM;
		return err;
	}
#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
	err = gp2a_parse_dt(data, &client->dev);
	if (err) {
		pr_err("%s, get gpio is failed\n", __func__);
		goto gp2a_parse_dt_err;
	}
#endif
	light_dev = iio_allocate_device(sizeof(struct gp2a_light_data));
	if (!light_dev) {
		pr_err("%s, iio_allocate_device error\n", __func__);
		err = -ENOMEM;
		goto iio_allocate_light_err;
	}
#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
	prox_dev = iio_allocate_device(sizeof(struct gp2a_prox_data));
	if (!prox_dev) {
		pr_err("%s, iio_allocate_device error\n", __func__);
		err = -ENOMEM;
		goto iio_allocate_prox_err;
	}
#endif

	light_data = iio_priv(light_dev);
	light_data->data = data;
	data->light_data = light_data;

#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
	prox_data = iio_priv(prox_dev);
	prox_data->data = data;
	data->prox_data = prox_data;
#endif
	i2c_set_clientdata(client, data);
	data->client = client;

	/* GP2A Regs INIT SETTINGS  and Check I2C communication */
	/* shutdown mode op[3]=0 */
	value = 0x00;
	err = gp2a_i2c_write(data, COMMAND1, &value);
	if (err < 0) {
		pr_err("%s failed : threre is no such device.\n", __func__);
		goto i2c_fail_err;
	}

	light_dev->name = "lightsensor-level";
	light_dev->channels = gp2a_light_channels;
	light_dev->num_channels = ARRAY_SIZE(gp2a_light_channels);
	light_dev->dev.parent = &client->dev;
	light_dev->modes = INDIO_DIRECT_MODE;
	light_dev->info = &light_info;

	err = iio_device_register(light_dev);
	if (err) {
		pr_err("%s: iio_device_register fail\n", __func__);
		goto err_iio_register_device_light_err;
	}
#ifdef CONFIG_SENSORS
	err = sensors_register(data->light_sensor_device,
		data, light_sensor_attrs, "light_sensor");
	if (err) {
		pr_err("%s: cound not register light sensor device(%d).\n",
		__func__, err);
		goto err_light_sensor_register_failed;
	}
#endif
#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
	prox_dev->name = "proximity-level";
	prox_dev->channels = gp2a_prox_channels;
	prox_dev->num_channels = ARRAY_SIZE(gp2a_prox_channels);
	prox_dev->dev.parent = &client->dev;
	prox_dev->modes = INDIO_DIRECT_MODE;
	prox_dev->info = &prox_info;

	err = iio_device_register(prox_dev);
	if (err) {
		pr_err("%s: iio_device_register fail\n", __func__);
		goto err_iio_register_device_prox_err;
	}

	err = gp2a_setup_irq(data);
	if (err) {
		pr_err("%s: could not setup irq\n", __func__);
		goto err_setup_irq;
	}

#ifdef CONFIG_SENSORS
	err = sensors_register(data->prox_sensor_device,
		data, prox_sensor_attrs, "proximity_sensor");
	if (err) {
		pr_err("%s: cound not register prox sensor device(%d).\n",
		__func__, err);
		goto err_prox_sensor_register_failed;
	}
#endif

	wake_lock_init(&data->prx_wake_lock, WAKE_LOCK_SUSPEND,
		"prx_wake_lock");

	INIT_DELAYED_WORK(&data->prox_avg_work, gp2a_work_func_prox);
	data->prox_delay = MSEC_PER_SEC / 20;
#endif
	INIT_DELAYED_WORK(&data->light_work, gp2a_work_func_light);
	mutex_init(&data->light_mutex);
	mutex_init(&data->data_mutex);

	data->light_delay = MSEC_PER_SEC / MIN_HZ;

	goto done;

#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
#ifdef CONFIG_SENSORS
err_prox_sensor_register_failed:
	sensors_unregister(data->prox_sensor_device, prox_sensor_attrs);
	gpio_free(data->gpio);
	gpio_free(data->vled_gpio);
#endif
err_setup_irq:
	iio_device_unregister(prox_dev);
err_iio_register_device_prox_err:
#endif
#ifdef CONFIG_SENSORS
err_light_sensor_register_failed:
	sensors_unregister(data->light_sensor_device, light_sensor_attrs);
#endif
	iio_device_unregister(light_dev);
err_iio_register_device_light_err:
i2c_fail_err:
#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
	iio_free_device(prox_dev);
iio_allocate_prox_err:
#endif
	iio_free_device(light_dev);
iio_allocate_light_err:
#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
gp2a_parse_dt_err:
#endif
	kfree(data);
	gp2a_regulator_onoff(&client->dev, false);
done:
	return err;
}

static void gp2a_shutdown(struct i2c_client *client)
{
	struct gp2a_data *data = i2c_get_clientdata(client);

	pr_info("%s, is called\n", __func__);
	if (data->light_enable) {
		cancel_delayed_work_sync(&data->light_work);
		lightsensor_onoff(0, data);
	}
#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
	if (data->prox_enable) {
		gp2a_prox_onoff(0, data);
		disable_irq(data->irq);
		disable_irq_wake(data->irq);
	}
	gpio_free(data->gpio);
	gpio_free(data->vled_gpio);
#endif
#ifdef CONFIG_SENSORS_GP2A_IIO_PROX
	iio_free_device(data->prox_data->indio_prox);
#endif
	iio_free_device(data->light_data->indio_light);
	kfree(data);
	gp2a_regulator_onoff(&client->dev, false);
}

static int gp2a_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct gp2a_data *data = i2c_get_clientdata(client);

	if (data->light_enable) {
		cancel_delayed_work_sync(&data->light_work);
		lightsensor_onoff(0, data);
	}
	pr_info("%s, is called\n", __func__);
	return 0;
}

static int gp2a_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct gp2a_data *data = i2c_get_clientdata(client);

	if (data->light_enable) {
		lightsensor_onoff(1, data);
		schedule_delayed_work(&data->light_work,
				msecs_to_jiffies(data->light_delay));
	}
	pr_info("%s, is called\n", __func__);
	return 0;
}

static const u16 normal_i2c[] = { I2C_CLIENT_END };

static struct of_device_id gp2a_match_table[] = {
	{ .compatible = "gp2a030a",},
	{},
};

static const struct i2c_device_id gp2a_device_id[] = {
	{"gp2a030a", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, gp2a_device_id);

static const struct dev_pm_ops gp2a_pm_ops = {
	.suspend = gp2a_suspend,
	.resume = gp2a_resume,
};

static struct i2c_driver gp2a_driver = {
	.driver = {
		   .name = "gp2a030a",
		   .owner = THIS_MODULE,
		   .pm = &gp2a_pm_ops,
		   .of_match_table = gp2a_match_table,
	},
	.probe = gp2a_probe,
	.shutdown = gp2a_shutdown,
	.id_table = gp2a_device_id,
	.address_list = normal_i2c,
};

module_i2c_driver(gp2a_driver);

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("Optical Sensor driver for GP2AP030A00F");
MODULE_LICENSE("GPL");
