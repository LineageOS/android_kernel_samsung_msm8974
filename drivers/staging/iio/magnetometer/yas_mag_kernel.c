/*
 * Copyright (c) 2013-2014 Yamaha Corporation
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

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/of_gpio.h>

#include "buffer.h"
#include "iio.h"
#include "ring_sw.h"
#include "sysfs.h"
#include "trigger.h"
#include "trigger_consumer.h"
#include "yas.h"

#if defined(CONFIG_SENSORS)
#include <linux/sensor/sensors_core.h>

#define VENDOR_NAME			"YAMAHA"
#define CHIP_NAME			"YAS532"
#endif

#if defined(CONFIG_MACH_SLTE_CMCC) || defined(CONFIG_MACH_SLTE_CU) || defined(CONFIG_MACH_SLTE_TD) \
|| defined(CONFIG_MACH_SLTE_ATT) || defined(CONFIG_MACH_SLTE_TMO) \
|| defined(CONFIG_MACH_SLTE_DCM) || defined(CONFIG_MACH_SLTE_KDI)
extern unsigned int system_rev;
#endif

static struct i2c_client *this_client;

enum {
	YAS_SCAN_MAGN_X,
	YAS_SCAN_MAGN_Y,
	YAS_SCAN_MAGN_Z,
	YAS_SCAN_TIMESTAMP,
};
struct yas_state {
	struct mutex lock;
	spinlock_t spin_lock;
	struct yas_mag_driver mag;
	struct i2c_client *client;
	struct iio_trigger  *trig;
	struct delayed_work work;
	struct device *yas_device;
	int16_t sampling_frequency;
	atomic_t pseudo_irq_enable;
	int32_t compass_data[3];
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend sus;
#endif
	int position;
};

static int yas_device_open(int32_t type)
{
	return 0;
}

static int yas_device_close(int32_t type)
{
	return 0;
}

static int yas_device_write(int32_t type, uint8_t addr, const uint8_t *buf,
		int len)
{
	uint8_t tmp[2];
	if (sizeof(tmp) - 1 < len)
		return -1;
	tmp[0] = addr;
	memcpy(&tmp[1], buf, len);
	if (i2c_master_send(this_client, tmp, len + 1) < 0)
		return -1;
	return 0;
}

static int yas_device_read(int32_t type, uint8_t addr, uint8_t *buf, int len)
{
	struct i2c_msg msg[2];
	int err;
	msg[0].addr = this_client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &addr;
	msg[1].addr = this_client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = buf;
	err = i2c_transfer(this_client->adapter, msg, 2);
	if (err != 2) {
		dev_err(&this_client->dev,
				"i2c_transfer() read error: "
				"slave_addr=%02x, reg_addr=%02x, err=%d\n",
				this_client->addr, addr, err);
		return err;
	}
	return 0;
}

static void yas_usleep(int us)
{
	usleep_range(us, us + 1000);
}

static uint32_t yas_current_time(void)
{
	return jiffies_to_msecs(jiffies);
}

static int yas_pseudo_irq_enable(struct iio_dev *indio_dev)
{
	struct yas_state *st = iio_priv(indio_dev);
	if (!atomic_cmpxchg(&st->pseudo_irq_enable, 0, 1)) {
		mutex_lock(&st->lock);
		st->mag.set_enable(1);
		mutex_unlock(&st->lock);
		schedule_delayed_work(&st->work, 0);
	}
	return 0;
}

static int yas_pseudo_irq_disable(struct iio_dev *indio_dev)
{
	struct yas_state *st = iio_priv(indio_dev);
	if (atomic_cmpxchg(&st->pseudo_irq_enable, 1, 0)) {
		cancel_delayed_work_sync(&st->work);
		mutex_lock(&st->lock);
		st->mag.set_enable(0);
		mutex_unlock(&st->lock);
	}
	return 0;
}

static int yas_set_pseudo_irq(struct iio_dev *indio_dev, int enable)
{
	if (enable)
		yas_pseudo_irq_enable(indio_dev);
	else
		yas_pseudo_irq_disable(indio_dev);
	return 0;
}

static int yas_data_rdy_trig_poll(struct iio_dev *indio_dev)
{
	struct yas_state *st = iio_priv(indio_dev);
	unsigned long flags;
	spin_lock_irqsave(&st->spin_lock, flags);
	iio_trigger_poll(st->trig, iio_get_time_ns());
	spin_unlock_irqrestore(&st->spin_lock, flags);
	return 0;
}

static irqreturn_t yas_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct iio_buffer *buffer = indio_dev->buffer;
	struct yas_state *st = iio_priv(indio_dev);
	int len = 0, i, j;
	size_t datasize = buffer->access->get_bytes_per_datum(buffer);
	int32_t *mag;
	struct timespec ts;
	s64 timestamp;

	mag = (int32_t *) kmalloc(datasize, GFP_KERNEL);
	if (mag == NULL)
		goto done;
	if (!bitmap_empty(indio_dev->active_scan_mask, indio_dev->masklength)) {
		j = 0;
		for (i = 0; i < 3; i++) {
			if (test_bit(i, indio_dev->active_scan_mask)) {
				mag[j] = st->compass_data[i];
				j++;
			}
		}
		len = j * 4;
	}

	/* Guaranteed to be aligned with 8 byte boundary */
	//if (indio_dev->scan_timestamp)
	// *(s64 *)((u8 *)mag + ALIGN(len, sizeof(s64))) = pf->timestamp;

	ts = ktime_to_timespec(ktime_get_boottime());
	timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	*(s64 *)((u8 *)mag + ALIGN(len, sizeof(s64))) = timestamp;
	if (timestamp <= 0)
		pr_err("[%s] invalid time = %lld\n", __func__, timestamp);
	iio_push_to_buffer(indio_dev->buffer, (u8 *)mag, 0);
	kfree(mag);
done:
	iio_trigger_notify_done(indio_dev->trig);
	return IRQ_HANDLED;
}

static int yas_data_rdy_trigger_set_state(struct iio_trigger *trig,
		bool state)
{
	struct iio_dev *indio_dev = trig->private_data;
	yas_set_pseudo_irq(indio_dev, state);
	return 0;
}

static const struct iio_trigger_ops yas_trigger_ops = {
	.owner = THIS_MODULE,
	.set_trigger_state = &yas_data_rdy_trigger_set_state,
};

static int yas_probe_trigger(struct iio_dev *indio_dev)
{
	int ret;
	struct yas_state *st = iio_priv(indio_dev);
	indio_dev->pollfunc = iio_alloc_pollfunc(&iio_pollfunc_store_time,
			&yas_trigger_handler, IRQF_ONESHOT, indio_dev,
			"%s_consumer%d", indio_dev->name, indio_dev->id);
	if (indio_dev->pollfunc == NULL) {
		ret = -ENOMEM;
		goto error_ret;
	}
	st->trig = iio_allocate_trigger("%s-dev%d",
			indio_dev->name,
			indio_dev->id);
	if (!st->trig) {
		ret = -ENOMEM;
		goto error_dealloc_pollfunc;
	}
	st->trig->dev.parent = &st->client->dev;
	st->trig->ops = &yas_trigger_ops;
	st->trig->private_data = indio_dev;
	ret = iio_trigger_register(st->trig);
	if (ret)
		goto error_free_trig;
	return 0;

error_free_trig:
	iio_free_trigger(st->trig);
error_dealloc_pollfunc:
	iio_dealloc_pollfunc(indio_dev->pollfunc);
error_ret:
	return ret;
}

static void yas_remove_trigger(struct iio_dev *indio_dev)
{
	struct yas_state *st = iio_priv(indio_dev);
	iio_trigger_unregister(st->trig);
	iio_free_trigger(st->trig);
	iio_dealloc_pollfunc(indio_dev->pollfunc);
}

static const struct iio_buffer_setup_ops yas_buffer_setup_ops = {
	.preenable = &iio_sw_buffer_preenable,
	.postenable = &iio_triggered_buffer_postenable,
	.predisable = &iio_triggered_buffer_predisable,
};

static void yas_remove_buffer(struct iio_dev *indio_dev)
{
	iio_buffer_unregister(indio_dev);
	iio_sw_rb_free(indio_dev->buffer);
};

static int yas_probe_buffer(struct iio_dev *indio_dev)
{
	int ret;
	struct iio_buffer *buffer;

	buffer = iio_sw_rb_allocate(indio_dev);
	if (!buffer) {
		ret = -ENOMEM;
		goto error_ret;
	}
	buffer->scan_timestamp = true;
	indio_dev->buffer = buffer;
	indio_dev->setup_ops = &yas_buffer_setup_ops;
	indio_dev->modes |= INDIO_BUFFER_TRIGGERED;
	ret = iio_buffer_register(indio_dev, indio_dev->channels,
			indio_dev->num_channels);
	if (ret)
		goto error_free_buf;
	iio_scan_mask_set(indio_dev, indio_dev->buffer, YAS_SCAN_MAGN_X);
	iio_scan_mask_set(indio_dev, indio_dev->buffer, YAS_SCAN_MAGN_Y);
	iio_scan_mask_set(indio_dev, indio_dev->buffer, YAS_SCAN_MAGN_Z);
	return 0;

error_free_buf:
	iio_sw_rb_free(indio_dev->buffer);
error_ret:
	return ret;
}

static ssize_t yas_position_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct yas_state *st = iio_priv(indio_dev);
	int ret;
	mutex_lock(&st->lock);
	ret = st->mag.get_position();
	mutex_unlock(&st->lock);
	if (ret < 0)
		return -EFAULT;
	return sprintf(buf, "%d\n", ret);
}

static ssize_t yas_position_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct yas_state *st = iio_priv(indio_dev);
	int ret, position;
	sscanf(buf, "%d\n", &position);
	mutex_lock(&st->lock);
	ret = st->mag.set_position(position);
	mutex_unlock(&st->lock);
	if (ret < 0)
		return -EFAULT;
	return count;
}

#if YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS532 \
	|| YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS533
static ssize_t yas_hard_offset_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct yas_state *st = iio_priv(indio_dev);
	int8_t hard_offset[3];
	int ret;
	mutex_lock(&st->lock);
	ret = st->mag.ext(YAS532_GET_HW_OFFSET, hard_offset);
	mutex_unlock(&st->lock);
	if (ret < 0)
		return -EFAULT;
	return sprintf(buf, "%d %d %d\n", hard_offset[0], hard_offset[1],
			hard_offset[2]);
}

static ssize_t yas_hard_offset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct yas_state *st = iio_priv(indio_dev);
	int32_t tmp[3];
	int8_t hard_offset[3];
	int ret, i;
	sscanf(buf, "%d %d %d\n", &tmp[0], &tmp[1], &tmp[2]);
	for (i = 0; i < 3; i++)
		hard_offset[i] = (int8_t)tmp[i];
	mutex_lock(&st->lock);
	ret = st->mag.ext(YAS532_SET_HW_OFFSET, hard_offset);
	mutex_unlock(&st->lock);
	if (ret < 0)
		return -EFAULT;
	return count;
}
#endif

static ssize_t yas_sampling_frequency_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct yas_state *st = iio_priv(indio_dev);
	return sprintf(buf, "%d\n", st->sampling_frequency);
}

static ssize_t yas_sampling_frequency_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct yas_state *st = iio_priv(indio_dev);
	int ret, data;
	ret = kstrtoint(buf, 10, &data);
	if (ret)
		return ret;
	if (data <= 0)
		return -EINVAL;
	mutex_lock(&st->lock);
	st->sampling_frequency = data;
	mutex_unlock(&st->lock);
	return count;
}

#if YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS532 \
	|| YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS533
static ssize_t yas_self_test_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct yas_state *st = iio_priv(indio_dev);
	struct yas532_self_test_result r;
	int ret;
	s8 err[7] = { 0, };
	mutex_lock(&st->lock);
	ret = st->mag.ext(YAS532_SELF_TEST, &r);
	mutex_unlock(&st->lock);

	if (unlikely(r.id != 0x2))
		err[0] = -1;
	if (unlikely(r.xy1y2[0] < -30 || r.xy1y2[0] > 30))
		err[3] = -1;
	if (unlikely(r.xy1y2[1] < -30 || r.xy1y2[1] > 30))
		err[3] = -1;
	if (unlikely(r.xy1y2[2] < -30 || r.xy1y2[2] > 30))
		err[3] = -1;
	if (unlikely(r.sx < 17 || r.sy < 22))
		err[5] = -1;
    if (unlikely(r.xyz[0] < -600 || r.xyz[0] > 600))
        err[6] = -1;
    if (unlikely(r.xyz[1] < -600 || r.xyz[1] > 600))
        err[6] = -1;
    if (unlikely(r.xyz[2] < -600 || r.xyz[2] > 600))
        err[6] = -1;

	pr_info("[SENSOR] %s\n"
		"[SENSOR] Test1 - err = %d, id = %d\n"
		"[SENSOR] Test3 - err = %d\n"
		"[SENSOR] Test4 - err = %d, offset = %d,%d,%d\n"
		"[SENSOR] Test5 - err = %d, direction = %d\n"
		"[SENSOR] Test6 - err = %d, sensitivity = %d,%d\n"
		"[SENSOR] Test7 - err = %d, offset = %d,%d,%d\n"
		"[SENSOR] Test2 - err = %d\n", __func__,
		err[0], r.id, err[2], err[3], r.xy1y2[0], r.xy1y2[1],
		r.xy1y2[2], err[4], r.dir, err[5], r.sx, r.sy,
		err[6], r.xyz[0], r.xyz[1], r.xyz[2], err[1]);

	return sprintf(buf,
			"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			err[0], r.id, err[2], err[3], r.xy1y2[0],
			r.xy1y2[1], r.xy1y2[2], err[4], r.dir,
			err[5], r.sx, r.sy, err[6],
			r.xyz[0], r.xyz[1], r.xyz[2], err[1]);
}

static ssize_t yas_self_test_noise_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct yas_state *st = iio_priv(indio_dev);
	int32_t xyz_raw[3];
	int ret;
	mutex_lock(&st->lock);
	ret = st->mag.ext(YAS532_SELF_TEST_NOISE, xyz_raw);
	mutex_unlock(&st->lock);
	if (ret < 0)
		return -EFAULT;
	return sprintf(buf, "%d,%d,%d\n", xyz_raw[0], xyz_raw[1], xyz_raw[2]);
}

static ssize_t yas_self_test_noise_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
    pr_info("%s: \n", __func__);

	return count;
}
#endif

static int yas_read_raw(struct iio_dev *indio_dev,
		struct iio_chan_spec const *chan,
		int *val,
		int *val2,
		long mask) {
	struct yas_state  *st = iio_priv(indio_dev);
	int ret = -EINVAL;

	if (chan->type != IIO_MAGN)
		return -EINVAL;

	mutex_lock(&st->lock);

	switch (mask) {
	case 0:
		*val = st->compass_data[chan->channel2 - IIO_MOD_X];
		ret = IIO_VAL_INT;
		break;
	case IIO_CHAN_INFO_SCALE:
		/* Gain : counts / uT = 1000 [nT] */
		/* Scaling factor : 1000000 / Gain = 1000 */
		*val = 0;
		*val2 = 1000;
		ret = IIO_VAL_INT_PLUS_MICRO;
		break;
	}

	mutex_unlock(&st->lock);

	return ret;
}

static void yas_work_func(struct work_struct *work)
{
	struct yas_data mag[1];
	struct yas_state *st =
		container_of((struct delayed_work *)work,
				struct yas_state, work);
	struct iio_dev *indio_dev = iio_priv_to_dev(st);
	uint32_t time_before, time_after;
	int32_t delay;
	int ret, i;

	time_before = jiffies_to_msecs(jiffies);
	mutex_lock(&st->lock);
	ret = st->mag.measure(mag, 1);
	if (ret == 1) {
		for (i = 0; i < 3; i++)
			st->compass_data[i] = mag[0].xyz.v[i];
	}
	mutex_unlock(&st->lock);
	if (ret == 1)
		yas_data_rdy_trig_poll(indio_dev);
	time_after = jiffies_to_msecs(jiffies);
	delay = MSEC_PER_SEC / st->sampling_frequency
		- (time_after - time_before);
	if (delay <= 0)
		delay = 1;
	schedule_delayed_work(&st->work, msecs_to_jiffies(delay));
}

#if defined(CONFIG_SENSORS)
/* sensors sysfs interface */
static ssize_t yas53x_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t yas53x_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_NAME);
}

static ssize_t yas_selftest_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct yas_state *st = dev_get_drvdata(dev);
	struct yas532_self_test_result r;
	int ret;
	s8 err[7] = { 0, };
	mutex_lock(&st->lock);
	ret = st->mag.ext(YAS532_SELF_TEST, &r);
	mutex_unlock(&st->lock);

	if (unlikely(r.id != 0x2))
		err[0] = -1;
	if (unlikely(r.xy1y2[0] < -30 || r.xy1y2[0] > 30))
		err[3] = -1;
	if (unlikely(r.xy1y2[1] < -30 || r.xy1y2[1] > 30))
		err[3] = -1;
	if (unlikely(r.xy1y2[2] < -30 || r.xy1y2[2] > 30))
		err[3] = -1;
	if (unlikely(r.sx < 17 || r.sy < 22))
		err[5] = -1;
	if (unlikely(r.xyz[0] < -600 || r.xyz[0] > 600))
		err[6] = -1;
	if (unlikely(r.xyz[1] < -600 || r.xyz[1] > 600))
		err[6] = -1;
	if (unlikely(r.xyz[2] < -600 || r.xyz[2] > 600))
		err[6] = -1;

	pr_info("[SENSOR] %s\n"
		"[SENSOR] Test1 - err = %d, id = %d\n"
		"[SENSOR] Test3 - err = %d\n"
		"[SENSOR] Test4 - err = %d, offset = %d,%d,%d\n"
		"[SENSOR] Test5 - err = %d, direction = %d\n"
		"[SENSOR] Test6 - err = %d, sensitivity = %d,%d\n"
		"[SENSOR] Test7 - err = %d, offset = %d,%d,%d\n"
		"[SENSOR] Test2 - err = %d\n", __func__,
		err[0], r.id, err[2], err[3], r.xy1y2[0], r.xy1y2[1],
		r.xy1y2[2], err[4], r.dir, err[5], r.sx, r.sy,
		err[6], r.xyz[0], r.xyz[1], r.xyz[2], err[1]);

	return sprintf(buf,
			"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			err[0], r.id, err[2], err[3], r.xy1y2[0],
			r.xy1y2[1], r.xy1y2[2], err[4], r.dir,
			err[5], r.sx, r.sy, err[6],
			r.xyz[0], r.xyz[1], r.xyz[2], err[1]);
}

static ssize_t yas_raw_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct yas_state *st = dev_get_drvdata(dev);
	int32_t xyz_raw[3];
	int ret;
	mutex_lock(&st->lock);
	ret = st->mag.ext(YAS532_SELF_TEST_NOISE, xyz_raw);
	mutex_unlock(&st->lock);
	if (ret < 0)
		return -EFAULT;
	return sprintf(buf, "%d,%d,%d\n", xyz_raw[0], xyz_raw[1], xyz_raw[2]);
}

static ssize_t yas_raw_data_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	pr_info("%s: \n", __func__);
	return count;
}

static DEVICE_ATTR(name, S_IRUGO, yas53x_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, yas53x_vendor_show, NULL);
static DEVICE_ATTR(selftest, S_IRUSR, yas_selftest_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUSR|S_IWUSR, yas_raw_data_show, yas_raw_data_store);

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_selftest,
	&dev_attr_raw_data,
	NULL,
};
#endif

#define YAS_MAGNETOMETER_CHANNEL(axis)		\
{							\
	.type = IIO_MAGN,				\
	.modified = 1,					\
	.channel2 = IIO_MOD_##axis,			\
	.info_mask = IIO_CHAN_INFO_SCALE_SHARED_BIT,	\
	.scan_index = YAS_SCAN_MAGN_##axis,		\
	.scan_type = IIO_ST('s', 32, 32, 0)		\
}

static const struct iio_chan_spec yas_channels[] = {
	YAS_MAGNETOMETER_CHANNEL(X),
	YAS_MAGNETOMETER_CHANNEL(Y),
	YAS_MAGNETOMETER_CHANNEL(Z),
	IIO_CHAN_SOFT_TIMESTAMP(YAS_SCAN_TIMESTAMP)
};

static IIO_DEVICE_ATTR(sampling_frequency, S_IRUSR|S_IWUSR,
		yas_sampling_frequency_show,
		yas_sampling_frequency_store, 0);
static IIO_DEVICE_ATTR(position, S_IRUSR|S_IWUSR,
		yas_position_show, yas_position_store, 0);
#if YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS532 \
	|| YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS533
static IIO_DEVICE_ATTR(hard_offset, S_IRUSR|S_IWUSR,
		yas_hard_offset_show, yas_hard_offset_store, 0);
static IIO_DEVICE_ATTR(self_test, S_IRUSR, yas_self_test_show, NULL, 0);
static IIO_DEVICE_ATTR(raw_data, S_IRUSR, yas_self_test_noise_show,
		yas_self_test_noise_store, 0);
#endif

static struct attribute *yas_attributes[] = {
	&iio_dev_attr_sampling_frequency.dev_attr.attr,
	&iio_dev_attr_position.dev_attr.attr,
#if YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS532 \
	|| YAS_MAG_DRIVER == YAS_MAG_DRIVER_YAS533
	&iio_dev_attr_hard_offset.dev_attr.attr,
	&iio_dev_attr_self_test.dev_attr.attr,
	&iio_dev_attr_raw_data.dev_attr.attr,
#endif
	NULL
};
static const struct attribute_group yas_attribute_group = {
	.attrs = yas_attributes,
};

static const struct iio_info yas_info = {
	.read_raw = &yas_read_raw,
	.attrs = &yas_attribute_group,
	.driver_module = THIS_MODULE,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void yas_early_suspend(struct early_suspend *h)
{
	struct yas_state *st = container_of(h,
			struct yas_state, sus);
	if (atomic_read(&st->pseudo_irq_enable)) {
		cancel_delayed_work_sync(&st->work);
		st->mag.set_enable(0);
	}
}


static void yas_late_resume(struct early_suspend *h)
{
	struct yas_state *st = container_of(h,
			struct yas_state, sus);
	if (atomic_read(&st->pseudo_irq_enable)) {
		st->mag.set_enable(1);
		schedule_delayed_work(&st->work, 0);
	}
}
#endif

static int yas_parse_dt(struct device *dev,
			struct  yas_state *st)
{
	struct device_node *np = dev->of_node;

	if (!of_property_read_u32(np, "yas,orientation",
				&st->position))
		pr_info("[SENSOR] yas,orientation = %d\n", st->position);
	else
		return -EINVAL;
#if defined(CONFIG_MACH_SLTE_CMCC) || defined(CONFIG_MACH_SLTE_CU) || defined(CONFIG_MACH_SLTE_TD)
	if (system_rev < 2)
		st->position = 2;
#elif defined(CONFIG_MACH_SLTE_ATT) || defined(CONFIG_MACH_SLTE_TMO) \
|| defined(CONFIG_MACH_SLTE_DCM) || defined(CONFIG_MACH_SLTE_KDI)
	if (system_rev < 1)
		st->position = 4;
#endif
	return 0;
}

static int yas_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	struct yas_state *st;
	struct iio_dev *indio_dev;
	int position, i;
	int ret;

	pr_info("[SENSOR] %s is called!!\n", __func__);

	this_client = i2c;
	indio_dev = iio_allocate_device(sizeof(*st));
	if (!indio_dev) {
		ret = -ENOMEM;
		goto error_ret;
	}
	i2c_set_clientdata(i2c, indio_dev);

	indio_dev->name = id->name;
	indio_dev->dev.parent = &i2c->dev;
	indio_dev->info = &yas_info;
	indio_dev->channels = yas_channels;
	indio_dev->num_channels = ARRAY_SIZE(yas_channels);
	indio_dev->modes = INDIO_DIRECT_MODE;

	st = iio_priv(indio_dev);
	st->client = i2c;
	st->sampling_frequency = 20;
	st->mag.callback.device_open = yas_device_open;
	st->mag.callback.device_close = yas_device_close;
	st->mag.callback.device_read = yas_device_read;
	st->mag.callback.device_write = yas_device_write;
	st->mag.callback.usleep = yas_usleep;
	st->mag.callback.current_time = yas_current_time;
	INIT_DELAYED_WORK(&st->work, yas_work_func);
	mutex_init(&st->lock);
#ifdef CONFIG_HAS_EARLYSUSPEND
	st->sus.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	st->sus.suspend = yas_early_suspend;
	st->sus.resume = yas_late_resume;
	register_early_suspend(&st->sus);
#endif

#ifdef CONFIG_SENSORS
    ret = sensors_register(st->yas_device, st, sensor_attrs, "magnetic_sensor");
    if (ret) {
        pr_err("%s: cound not register gyro sensor device(%d).\n",
        __func__, ret);
        goto err_yas_sensor_register_failed;
    }
#endif
	for (i = 0; i < 3; i++)
		st->compass_data[i] = 0;

	ret = yas_probe_buffer(indio_dev);
	if (ret)
		goto error_free_dev;
	ret = yas_probe_trigger(indio_dev);
	if (ret)
		goto error_remove_buffer;
	ret = iio_device_register(indio_dev);
	if (ret)
		goto error_remove_trigger;
	ret = yas_mag_driver_init(&st->mag);
	if (ret < 0) {
		ret = -EFAULT;
		goto error_unregister_iio;
	}
	ret = st->mag.init();
	if (ret < 0) {
		ret = -EFAULT;
		goto error_unregister_iio;
	}
	ret = yas_parse_dt(&i2c->dev, st);
	if(!ret){
	    position = st->position;
	    ret = st->mag.set_position(position);
	    pr_info("[SENSOR] set_position (%d)\n", position);
	}

	spin_lock_init(&st->spin_lock);
	pr_info("[SENSOR] %s is finished!!\n", __func__);

	return 0;

error_unregister_iio:
	iio_device_unregister(indio_dev);
error_remove_trigger:
	yas_remove_trigger(indio_dev);
error_remove_buffer:
	yas_remove_buffer(indio_dev);
error_free_dev:
#ifdef CONFIG_SENSORS
        sensors_unregister(st->yas_device, sensor_attrs);
err_yas_sensor_register_failed:
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&st->sus);
#endif
	iio_free_device(indio_dev);
error_ret:
	i2c_set_clientdata(i2c, NULL);
	this_client = NULL;
	return ret;
}

static int yas_remove(struct i2c_client *i2c)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(i2c);
	struct yas_state *st;
	if (indio_dev) {
		st = iio_priv(indio_dev);
#ifdef CONFIG_HAS_EARLYSUSPEND
		unregister_early_suspend(&st->sus);
#endif
		yas_pseudo_irq_disable(indio_dev);
		st->mag.term();
		iio_device_unregister(indio_dev);
		yas_remove_trigger(indio_dev);
		yas_remove_buffer(indio_dev);
		iio_free_device(indio_dev);
		this_client = NULL;
	}
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int yas_suspend(struct device *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct yas_state *st = iio_priv(indio_dev);
	if (atomic_read(&st->pseudo_irq_enable)) {
		cancel_delayed_work_sync(&st->work);
		st->mag.set_enable(0);
	}
	return 0;
}

static int yas_resume(struct device *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct yas_state *st = iio_priv(indio_dev);
	if (atomic_read(&st->pseudo_irq_enable)) {
		st->mag.set_enable(1);
		schedule_delayed_work(&st->work, 0);
	}
	return 0;
}

static SIMPLE_DEV_PM_OPS(yas_pm_ops, yas_suspend, yas_resume);
#define YAS_PM_OPS (&yas_pm_ops)
#else
#define YAS_PM_OPS NULL
#endif

#ifdef CONFIG_OF
static struct of_device_id yas_match_table[] = {
	{ .compatible = "yas_magnetometer",},
	{},
};
#else
#define yas_match_table NULL
#endif

static const struct i2c_device_id yas_id[] = {
	{YAS_MAG_NAME, 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, yas_id);

static struct i2c_driver yas_driver = {
	.driver = {
		.name	= YAS_MAG_NAME,
		.owner	= THIS_MODULE,
		.pm	= YAS_PM_OPS,
		.of_match_table = yas_match_table,
	},
	.probe		= yas_probe,
	.remove		= __devexit_p(yas_remove),
	.id_table	= yas_id,
};
static int __init yas_init(void)
{
	return i2c_add_driver(&yas_driver);
}

static void __exit yas_exit(void)
{
	i2c_del_driver(&yas_driver);
}

module_init(yas_init);
module_exit(yas_exit);

MODULE_DESCRIPTION("Yamaha Magnetometer I2C driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("5.1.1000e");
