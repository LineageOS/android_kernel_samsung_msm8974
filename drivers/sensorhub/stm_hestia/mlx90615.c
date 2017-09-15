/* driver/sensor/mlx90615.c
 * Copyright (c) 2011 SAMSUNG
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

#include <linux/i2c.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sensors_core.h>

#define VENDOR		"MELEXIS"
#define CHIP_ID		"MLX90615"

#define I2C_M_WR	0 /* for i2c Write */
#define I2c_M_RD	1 /* for i2c Read */

#define RAM_REG_RAW_IR		0x25
#define RAM_REG_T_AMBIENT	0x26
#define RAM_REG_T_OUT		0x27

#define ROM_REG_I2C_ADDR	0x10
#define ROM_REG_CONFIG		0x12
#define ROM_REG_EMISSIVITY	0x13
#define ROM_REG_ID_NUMBER	0x1E

#define REG_SLEEP		0xff

#define DEFAULT_DELAY		210

#define I2C_READ		0x01
#define I2C_WRITE		0x00

#define EVENT_TYPE_AMBIENT_TEMP	REL_HWHEEL
#define EVENT_TYPE_OBJECT_TEMP	REL_DIAL

#define MLX90615_CMD_LENGTH	2

#define TEMPERATURE_OFFSET	0

#define ARRAY_DEPTH		7

struct mlx90615_data {
	struct i2c_client *i2c_client;
	struct input_dev *input;
	struct mutex power_lock;
	struct mutex read_lock;
	struct hrtimer timer;
	struct workqueue_struct *mlx90615_wq;
	struct work_struct work_mlx90615;
	struct device *mlx90615_dev;
	ktime_t poll_delay;
	u8 enabled;
	int ambient_temp;
	int object_temp;
	u32 scl;
	u32 sda;
	u32 power;
	int always_on;
	int amb[ARRAY_DEPTH], obj[ARRAY_DEPTH];
	int index;
};

static inline void setsda(struct mlx90615_data *data, int state)
{
	if (state)
		gpio_direction_input(data->sda);
	else
		gpio_direction_output(data->sda, 0);
}

static inline void setscl(struct mlx90615_data *data, int state)
{
	if (state)
		gpio_direction_output(data->scl, 1);
	else
		gpio_direction_output(data->scl, 0);
}

static int mlx90615_power_parse_dt(struct device *dev,
	struct mlx90615_data *data)
{
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int errorno = 0;

	data->power = of_get_named_gpio_flags(np, "mlx90615,power",
		0, &flags);
	if (data->power < 0) {
		errorno = data->power;
		pr_err("[BODYTEMP] %s: mlx90615,power\n", __func__);
		goto dt_exit;
	}

	errorno = gpio_request(data->power, "MLX9061_POWER");
	if (errorno) {
		pr_err("[BODYTEMP] %s: failed to request EN PIN for MLX90615\n",
			__func__);
		goto dt_exit;
	}

	errorno = gpio_direction_output(data->power, 0);
	if (errorno) {
		pr_err("[BODYTEMP] %s: failed to set ENABLE PIN as input\n",
			__func__);
		goto dt_exit;
	}

	return 0;

dt_exit:
	return errorno;
}

static int mlx90615_i2c_read_word(struct mlx90615_data *mlx90615, u8 command,
	s32 *val)
{
	int err = 0;
	struct i2c_client *client = mlx90615->i2c_client;
	struct i2c_msg msg[2];
	unsigned char data[3] = {0,};
	s32 value = 0;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	/* send slave address & command */
	msg[0].addr = client->addr;
	msg[0].flags = I2C_M_WR;
	msg[0].len = 1;
	msg[0].buf = &command;

	/* read word data */
	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 3;
	msg[1].buf = data;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err >= 0) {
		if(data[2] == 0xff) {
			pr_err("[BODYTEMP] %s : crc error detect", __func__);
			return -EINVAL;
		} else {
			value = (s32)data[1];
			*val= (value << 8) | (s32)data[0];
		}
	} else
		pr_err("[BODYTEMP] %s, i2c transfer error ret=%d\n",
		__func__, err);

	return err;
}

static void mlx90615_set_power(struct mlx90615_data *mlx90615, int onoff)
{
	pr_info("[BODYTEMP] %s : %s\n", __func__, (onoff)?"ON":"OFF");

	if (onoff) {
		setscl(mlx90615, 1);

		if (mlx90615->power > 0)
			gpio_set_value(mlx90615->power, 1);
		else {
			mlx90615_power_parse_dt(&mlx90615->i2c_client->dev,
				mlx90615);
		}
		udelay(100);
		setscl(mlx90615, 0);
		msleep(50);
		setscl(mlx90615, 1);
		msleep(20);
	} else 
		if (mlx90615->power > 0)
			gpio_set_value(mlx90615->power, 0);
}


static void mlx90615_convert(s32 kalvin, int *real, int* frac)
{
	int temperature;

	temperature = (kalvin * 2) - 27315;
	*real = temperature / 100;
	*frac = temperature % 100;
}

static int mlx90615_read_data(struct mlx90615_data *mlx90615,
	int *tar, int *taf, int *tor, int *tof)
{
	static s32 object;
	static s32 ambient;
	s32 ret;
	u8 cmd;

	if (mlx90615->always_on == 0)
		mlx90615_set_power(mlx90615, 1);

	cmd = RAM_REG_T_AMBIENT;
	ret = mlx90615_i2c_read_word(mlx90615, cmd, &ambient);
//	ret = i2c_smbus_read_word_data(mlx90615->i2c_client, cmd);

	if (ret < 0)
		pr_info("[BODYTEMP] %s : ambient temp read fail %d\n",
			__func__, ret);

	mlx90615_convert(ambient, tar, taf);

	cmd = RAM_REG_T_OUT;
	ret = mlx90615_i2c_read_word(mlx90615, cmd, &object);
//	ret = i2c_smbus_read_word_data(mlx90615->i2c_client, cmd);

	if (ret < 0)
		pr_info("[BODYTEMP] %s : object temp read fail %d\n",
			__func__, ret);

	mlx90615_convert(object, tor, tof);

	if (mlx90615->always_on == 0)
		mlx90615_set_power(mlx90615, 0);

	return 0;
}

static void mlx90615_enable(struct mlx90615_data *mlx90615)
{
	int i;
	/* enable setting */
	pr_info("[BODYTEMP] %s: starting poll timer, delay %lldns\n",
				__func__, ktime_to_ns(mlx90615->poll_delay));

/*
	if (ktime_to_ms(mlx90615->poll_delay) < 200) {
		pr_info("%s : power always on mode\n", __func__);
		mlx90615->always_on = 1;
		mlx90615_set_power(mlx90615, 1);
	} else {
		pr_info("%s : power on/off mode\n", __func__);
		mlx90615->always_on = 0;
		mlx90615_set_power(mlx90615, 0);
	}
*/
	mlx90615->always_on = 1;
	mlx90615_set_power(mlx90615, 1);

	mlx90615->index = 0;
	for (i = 0 ; i < 5 ; i++) {
		mlx90615->amb[i] = 0;
		mlx90615->obj[i] = 0;
	}

	hrtimer_start(&mlx90615->timer, mlx90615->poll_delay,
					HRTIMER_MODE_REL);
}

static void mlx90615_disable(struct mlx90615_data *mlx90615)
{
	/* disable setting */
	pr_info("[BODYTEMP] %s: cancelling poll timer\n", __func__);
	mlx90615->always_on = 0;
	mlx90615_set_power(mlx90615, 0);
	hrtimer_cancel(&mlx90615->timer);
	cancel_work_sync(&mlx90615->work_mlx90615);
}

/* sysfs */
static ssize_t poll_delay_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct mlx90615_data *mlx90615 = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(mlx90615->poll_delay));
}

static ssize_t poll_delay_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct mlx90615_data *mlx90615 = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;
	pr_info("[BODYTEMP] %s : %s\n", __func__, buf);
	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	mutex_lock(&mlx90615->power_lock);
	if (new_delay != ktime_to_ns(mlx90615->poll_delay)) {
#if 0
		if (new_delay < ktime_to_ns(mlx90615->poll_delay))
			mlx90615->poll_delay =
				ns_to_ktime(DEFAULT_DELAY * NSEC_PER_MSEC);
		else
#endif
			mlx90615->poll_delay = ns_to_ktime(new_delay);
		if (mlx90615->enabled) {
			mlx90615_disable(mlx90615);
			mlx90615_enable(mlx90615);
		}
		pr_info("[BODYTEMP] %s, poll_delay = %lld\n",
			__func__, new_delay);
	}
	mutex_unlock(&mlx90615->power_lock);

	return size;
}

static ssize_t mlx90615_enable_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct mlx90615_data *mlx90615 = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("[BODYTEMP] %s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&mlx90615->power_lock);
	pr_info("[BODYTEMP] %s: new_value = %d\n", __func__, new_value);
	if (new_value && !mlx90615->enabled) {
		mlx90615->enabled = 1;
		mlx90615_enable(mlx90615);
	} else if (!new_value && mlx90615->enabled) {
		mlx90615_disable(mlx90615);
		mlx90615->enabled = 0;
	}
	mutex_unlock(&mlx90615->power_lock);

	return size;
}

static ssize_t mlx90615_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct mlx90615_data *mlx90615 = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", mlx90615->enabled);
}

static ssize_t mlx90615_raw_data_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct mlx90615_data *mlx90615 = dev_get_drvdata(dev);
	int tar = 0, taf = 0, tor = 0, tof = 0;

	mutex_lock(&mlx90615->read_lock);
	mlx90615_read_data(mlx90615, &tar, &taf, &tor, &tof);
	mutex_unlock(&mlx90615->read_lock);

	return sprintf(buf, "%d.%02d,%d.%02d\n", tar,taf,tor,tof);
}

static DEVICE_ATTR(raw_data, S_IRUGO, mlx90615_raw_data_show, NULL);
static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
			poll_delay_show, poll_delay_store);

/* sysfs for vendor & name */
static ssize_t mlx90615_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t mlx90615_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static DEVICE_ATTR(vendor, 0644, mlx90615_vendor_show, NULL);
static DEVICE_ATTR(name, 0644, mlx90615_name_show, NULL);

static struct device_attribute dev_attr_enable =
__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	mlx90615_enable_show, mlx90615_enable_store);

static struct attribute *mlx90615_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group mlx90615_attribute_group = {
	.attrs = mlx90615_sysfs_attrs,
};

static struct device_attribute *bodytemp_sensor_attrs[] = {
	&dev_attr_raw_data,
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_enable,
	&dev_attr_poll_delay,
	NULL,
};

/* This function is for mlx90615 sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart timer_func(struct hrtimer *timer)
{
	struct mlx90615_data *mlx90615
		= container_of(timer, struct mlx90615_data, timer);

	queue_work(mlx90615->mlx90615_wq, &mlx90615->work_mlx90615);
	hrtimer_forward_now(&mlx90615->timer, mlx90615->poll_delay);

	return HRTIMER_RESTART;
}

static void mlx90615_work_func(struct work_struct *work)
{
	struct mlx90615_data *mlx90615 = container_of(work, struct mlx90615_data,
						work_mlx90615);
	int tar = 0, taf = 0, tor = 0, tof = 0;
	int ambient, object;
	int amb_min = 80000, obj_min = 80000;
	int amb_max = -1, obj_max = -1;
	int index;
	int i, total_a = 0, total_o = 0;

	mutex_lock(&mlx90615->read_lock);
	mlx90615_read_data(mlx90615, &tar, &taf, &tor, &tof);
	mutex_unlock(&mlx90615->read_lock);

	ambient = tar * 100 + taf + TEMPERATURE_OFFSET;
	object = tor * 100 + tof + TEMPERATURE_OFFSET;

	index = mlx90615->index;
	mlx90615->amb[index] = ambient;
	mlx90615->obj[index] = object;

	index++;

	mlx90615->index = index % ARRAY_DEPTH;

	for ( i = 0 ; i < ARRAY_DEPTH ; i++) {
		total_a += mlx90615->amb[i];
		total_o += mlx90615->obj[i];

		if (mlx90615->amb[i] <= amb_min)
			amb_min = mlx90615->amb[i];
		if (mlx90615->amb[i] >= amb_max)
			amb_max = mlx90615->amb[i];

		if (mlx90615->obj[i] <= obj_min)
			obj_min = mlx90615->obj[i];
		if (mlx90615->obj[i] >= obj_max)
			obj_max = mlx90615->obj[i];
	}

	total_a = total_a - amb_min - amb_max;
	total_o = total_o - obj_min - obj_max;

	mlx90615->ambient_temp = total_a / (ARRAY_DEPTH - 2);
	mlx90615->object_temp = total_o / (ARRAY_DEPTH - 2);

	input_report_rel(mlx90615->input, EVENT_TYPE_AMBIENT_TEMP,
						mlx90615->ambient_temp + 27315);
	input_report_rel(mlx90615->input, EVENT_TYPE_OBJECT_TEMP,
						mlx90615->object_temp + 27315);

	input_sync(mlx90615->input);

	pr_info("[BODYTEMP] %s: ambient = %d.%02d C object = %d.%02d C\n",
		__func__, tar, taf, tor, tof);
}

static int mlx90615_parse_dt(struct device *dev, struct mlx90615_data *data)
{
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int errorno = 0;

	pr_info("[BODYTEMP] %s : mlx90615,scl\n", __func__);
	data->scl = of_get_named_gpio_flags(np, "mlx90615,scl",
		0, &flags);
	if (data->scl < 0) {
		errorno = data->scl;
		pr_err("[BODYTEMP] %s: mlx90615,scl\n", __func__);
		goto dt_exit;
	}

	pr_info("[BODYTEMP] %s : mlx90615,sda\n", __func__);
	data->sda = of_get_named_gpio_flags(np, "mlx90615,sda",
		0, &flags);
	if (data->sda < 0) {
		errorno = data->sda;
		pr_err("[BODYTEMP] %s: mlx90615,sda\n", __func__);
		goto dt_exit;
	}

	pr_info("[BODYTEMP] %s : mlx90615,power\n", __func__);
	data->power = of_get_named_gpio_flags(np, "mlx90615,power",
		0, &flags);
	if (data->power < 0) {
		errorno = data->power;
		pr_err("[BODYTEMP] %s: mlx90615,power\n", __func__);
		goto dt_exit;
	}

	errorno = gpio_request(data->power, "MLX9061_POWER");
	if (errorno) {
		pr_err("[BODYTEMP] %s: failed to request ENABLE PIN\n",
			__func__);
		goto dt_exit;
	}

	errorno = gpio_direction_output(data->power, 0);
	if (errorno) {
		pr_err("[BODYTEMP] %s: failed to set ENABLE PIN as input\n",
			__func__);
		goto dt_exit;
	}

	return 0;

dt_exit:
	return errorno;
}

static int mlx90615_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct mlx90615_data *mlx90615 = NULL;

	pr_info("[BODYTEMP] %s is called.\n", __func__);

	/* Check i2c functionality */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[BODYTEMP] %s: i2c functionality check failed!\n",
			__func__);
		return ret;
	}

	mlx90615 = kzalloc(sizeof(struct mlx90615_data), GFP_KERNEL);
	if (!mlx90615) {
		pr_err("[BODYTEMP] %s: failed to alloc memory mlx90615_data\n",
			__func__);
		return -ENOMEM;
	}

	if (client-> dev.of_node)
		pr_info("[BODYTEMP] %s : of node\n", __func__);
	else {
		pr_err("[BODYTEMP] %s : no of node\n", __func__);
		goto err_setup_reg;
	}

	ret = mlx90615_parse_dt(&client->dev, mlx90615);

	if (ret) {
		pr_err("[BODYTEMP] %s : parse dt error\n", __func__);
		//goto err_parse_dt;
	}

	mlx90615->i2c_client = client;
	mlx90615->always_on = 0;

	i2c_set_clientdata(client, mlx90615);

	mutex_init(&mlx90615->power_lock);
	mutex_init(&mlx90615->read_lock);
#if 0
	/* Check if the device is there or not. */
	ret = i2c_master_send(mlx90615->i2c_client,
				CMD_READ_ID_REG, MLX90615_CMD_LENGTH);
	if (ret < 0) {
		pr_err("[BODYTEMP] %s: failed i2c_master_send (err = %d)\n",
			__func__, ret);
		/* goto err_i2c_master_send;*/
	}
#endif
	/* allocate mlx90615 input_device */
	mlx90615->input = input_allocate_device();
	if (!mlx90615->input) {
		pr_err("[BODYTEMP] %s: could not allocate input device\n",
			__func__);
		goto err_input_allocate_device;
	}

	mlx90615->input->name = "bodytemp_sensor";
	input_set_capability(mlx90615->input, EV_REL, EVENT_TYPE_AMBIENT_TEMP);
	input_set_capability(mlx90615->input, EV_REL, EVENT_TYPE_OBJECT_TEMP);
	input_set_drvdata(mlx90615->input, mlx90615);

	ret = input_register_device(mlx90615->input);
	if (ret < 0) {
		input_free_device(mlx90615->input);
		pr_err("[BODYTEMP] %s: could not register input device\n",
			__func__);
		goto err_input_register_device;
	}

	ret = sensors_create_symlink(&mlx90615->input->dev.kobj,
					mlx90615->input->name);
	if (ret < 0) {
		input_unregister_device(mlx90615->input);
		goto err_sysfs_create_symlink;
	}

	ret = sysfs_create_group(&mlx90615->input->dev.kobj,
				&mlx90615_attribute_group);
	if (ret) {
		pr_err("[BODYTEMP] %s: could not create sysfs group\n",
			__func__);
		goto err_sysfs_create_group;
	}
	ret = sensors_register(mlx90615->mlx90615_dev,
			mlx90615, bodytemp_sensor_attrs, "bodytemp_sensor");
	if (ret) {
		pr_err("[BODYTEMP] %s: cound not register sensor device(%d).\n",
			__func__, ret);
		goto err_sysfs_create_symlink;
	}
	/* Timer settings. We poll for mlx90615 values using a timer. */
	hrtimer_init(&mlx90615->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mlx90615->poll_delay = ns_to_ktime(DEFAULT_DELAY * NSEC_PER_MSEC);
	mlx90615->timer.function = timer_func;

	/* Timer just fires off a work queue request.  We need a thread
	   to read the i2c (can be slow and blocking). */
	mlx90615->mlx90615_wq =
		create_singlethread_workqueue("mlx90615_bidytemp_wq");
	if (!mlx90615->mlx90615_wq) {
		ret = -ENOMEM;
		pr_err("[BODYTEMP] %s: could not create mlx90615 workqueue\n", __func__);
		goto err_create_workqueue;
	}

	/* This is the thread function we run on the work queue */
	INIT_WORK(&mlx90615->work_mlx90615, mlx90615_work_func);

	pr_info("[BODYTEMP] %s is success.\n", __func__);

	goto done;


err_create_workqueue:
	sysfs_remove_group(&mlx90615->input->dev.kobj,
				&mlx90615_attribute_group);
	/* sensors_classdev_unregister(mlx90615->mlx90615_dev);*/
	destroy_workqueue(mlx90615->mlx90615_wq);
err_sysfs_create_group:
	input_unregister_device(mlx90615->input);
err_sysfs_create_symlink:
	sensors_remove_symlink(&mlx90615->input->dev.kobj,
			mlx90615->input->name);
err_input_register_device:
err_input_allocate_device:
	mutex_destroy(&mlx90615->read_lock);
	mutex_destroy(&mlx90615->power_lock);
err_setup_reg:
//err_parse_dt:
	kfree(mlx90615);
done:
	return ret;
}

static int mlx90615_i2c_remove(struct i2c_client *client)
{
	struct mlx90615_data *mlx90615 = i2c_get_clientdata(client);
	/* device off */
	if (mlx90615->enabled)
		mlx90615_disable(mlx90615);

	/* destroy workqueue */
	destroy_workqueue(mlx90615->mlx90615_wq);
#if 0
	/* sysfs destroy */
	device_remove_file(mlx90615->mlx90615_dev, &dev_attr_name);
	device_remove_file(mlx90615->mlx90615_dev, &dev_attr_vendor);
	device_remove_file(mlx90615->mlx90615_dev, &dev_attr_raw_data);
#endif
	//sensors_classdev_unregister(mlx90615->mlx90615_dev);

	/* input device destroy */
	sysfs_remove_group(&mlx90615->input->dev.kobj,
				&mlx90615_attribute_group);
	input_unregister_device(mlx90615->input);

	/* lock destroy */
	mutex_destroy(&mlx90615->read_lock);
	mutex_destroy(&mlx90615->power_lock);
	kfree(mlx90615);

	return 0;
}

static int mlx90615_suspend(struct device *dev)
{
	struct mlx90615_data *mlx90615 = dev_get_drvdata(dev);

	if (mlx90615->enabled)
		mlx90615_disable(mlx90615);

	return 0;
}

static int mlx90615_resume(struct device *dev)
{
	struct mlx90615_data *mlx90615 = dev_get_drvdata(dev);

	if (mlx90615->enabled)
		mlx90615_enable(mlx90615);

	return 0;
}


//MODULE_DEVICE_TABLE(i2c, mlx90615_device_id);


static struct of_device_id mlx90615_table[] = {
	{ .compatible = "mlx90615",},
	{},
};

static const struct i2c_device_id mlx90615_device_id[] = {
	{"mlx90615_table", 0},
	{}
};

static const struct dev_pm_ops mlx90615_pm_ops = {
	.suspend = mlx90615_suspend,
	.resume = mlx90615_resume
};

static struct i2c_driver mlx90615_i2c_driver = {
	.driver = {
		.name = "mlx90615",
		.owner = THIS_MODULE,
		.pm = &mlx90615_pm_ops,
		.of_match_table = mlx90615_table,
	},
	.probe = mlx90615_i2c_probe,
	.remove = mlx90615_i2c_remove,
	.id_table = mlx90615_device_id,
};

static int __init mlx90615_init(void)
{
	return i2c_add_driver(&mlx90615_i2c_driver);
}

static void __exit mlx90615_exit(void)
{
	i2c_del_driver(&mlx90615_i2c_driver);
}

module_init(mlx90615_init);
module_exit(mlx90615_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("mlx90615 Temperature Sensor & mlx90615 sensor device driver");
MODULE_LICENSE("GPL");
