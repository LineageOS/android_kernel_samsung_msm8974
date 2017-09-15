/*
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
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
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/gp2a002.h>
#include <linux/module.h>
#include <linux/sensors_core.h>

#include <linux/regulator/consumer.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#define REGS_PROX		0x0 /* Read  Only */
#define REGS_GAIN		0x1 /* Write Only */
#define REGS_HYS		0x2 /* Write Only */
#define REGS_CYCLE		0x3 /* Write Only */
#define REGS_OPMOD		0x4 /* Write Only */
#define REGS_CON		0x6 /* Write Only */

#if defined(CONFIG_MACH_AFYONLTE_TMO) || defined(CONFIG_MACH_AFYONLTE_MTR)
#define PROX_NONDETECT			0x40

#define PROX_DETECT				0x20
#else
#define PROX_NONDETECT			0x2F
#define PROX_DETECT				0x0F
#endif
#define PROX_NONDETECT_MODE1	0x43
#define PROX_DETECT_MODE1		0x28
#define PROX_NONDETECT_MODE2	0x48
#define PROX_DETECT_MODE2		0x42
#define OFFSET_FILE_PATH		"/efs/prox_cal"

#define PROXIMITY	1
#define CHIP_DEV_NAME	"GP2AP002"
#define CHIP_DEV_VENDOR	"SHARP"

struct gp2a_data {
	struct input_dev *input;
	struct device *dev;
	struct gp2a_platform_data *pdata;
	struct i2c_client *i2c_client;
	struct mutex power_lock;
	struct wake_lock prx_wake_lock;
	struct workqueue_struct *wq;
	struct work_struct work_prox;

	int irq;
	int power_state;
	char val_state;
	char cal_mode;

	u8 detect;
	u8 nondetect;
};

int gp2a_i2c_read(struct gp2a_data *gp2a, u8 reg, u8 *val)
{
	int err = 0;
	unsigned char data[2] = {reg, 0};
	int retry = 10;
	struct i2c_msg msg[2] = {};
	struct i2c_client *client = gp2a->i2c_client;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = data;

	msg[1].addr = client->addr;
	msg[1].flags = 1;
	msg[1].len = 2;
	msg[1].buf = data;

	while (retry--) {
		data[0] = reg;

		err = i2c_transfer(client->adapter, msg, 2);

		if (err >= 0) {
			*val = data[1];
			return 0;
		}
	}

	pr_err("%s : i2c transfer error ret = %d\n", __func__, err);

	return err;
}

int gp2a_i2c_write(struct gp2a_data *gp2a, u8 reg, u8 val)
{
	int err = -1;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int retry = 10;
	struct i2c_client *client = gp2a->i2c_client;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		data[0] = reg;
		data[1] = val;

		msg->addr = client->addr;
		msg->flags = 0;
		msg->len = 2;
		msg->buf = data;

		err = i2c_transfer(client->adapter, msg, 1);

		if (err >= 0)
			return 0;
	}

	pr_err("%s : i2c transfer error ret= %d\n", __func__, err);

	return err;
}
#ifdef CONFIG_SENSORS_POWER_EN
static int gp2a_leda_onoff(struct gp2a_data *gp2a, int power)
{
	int ret;
	if (power)
		ret = gpio_direction_output(gp2a->pdata->power_en, 1);
	else
		ret = gpio_direction_output(gp2a->pdata->power_en, 0);

	if (ret < 0)
		pr_err("%s, error for direction\n", __func__);

	return 0;
}
#endif
static int gp2a_power_onoff(struct gp2a_data *gp2a, int power)
{
	u8 value;
	pr_info("%s,status(%d)\n", __func__, power);

	if (power) {
#ifdef CONFIG_SENSORS_POWER_EN
		gp2a_leda_onoff(gp2a, power);
#endif
		value = 0x18;
		gp2a_i2c_write(gp2a, REGS_CON, value);
		value = 0x08;
		gp2a_i2c_write(gp2a, REGS_GAIN, value);
		value = gp2a->nondetect;
		gp2a_i2c_write(gp2a, REGS_HYS, value);
		value = 0x04;
		gp2a_i2c_write(gp2a, REGS_CYCLE, value);
		value = 0x03;
		gp2a_i2c_write(gp2a, REGS_OPMOD, value);

		enable_irq_wake(gp2a->irq);
		enable_irq(gp2a->irq);

		value = 0x00;
		gp2a_i2c_write(gp2a, REGS_CON, value);
	} else {
#ifdef CONFIG_SENSORS_POWER_EN
		gp2a_leda_onoff(gp2a, power);
#endif
		disable_irq_wake(gp2a->irq);
		disable_irq(gp2a->irq);

		value = 0x02;
		gp2a_i2c_write(gp2a, REGS_OPMOD, value);
	}
	return 0;
}

static ssize_t adc_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", gp2a->val_state);
}

static ssize_t state_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", gp2a->val_state);
}

static ssize_t name_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_DEV_NAME);
}

static ssize_t vendor_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_DEV_VENDOR);
}

static int gp2a_cal_mode_read_file(struct gp2a_data *gp2a)
{
	int err = 0;
	mm_segment_t old_fs;
	struct file *cal_mode_filp = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_mode_filp = filp_open(OFFSET_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(cal_mode_filp)) {
		err = PTR_ERR(cal_mode_filp);
		if (err != -ENOENT)
			pr_err("%s,Can't open cal_mode file\n", __func__);
		set_fs(old_fs);
		return err;
	}
	err = cal_mode_filp->f_op->read(cal_mode_filp,
		(char *)&gp2a->cal_mode,
		sizeof(u8), &cal_mode_filp->f_pos);

	if (err != sizeof(u8)) {
		pr_err("%s,Can't read the cal_mode from file\n",
			__func__);
		filp_close(cal_mode_filp, current->files);
		set_fs(old_fs);
		return -EIO;
	}

	filp_close(cal_mode_filp, current->files);
	set_fs(old_fs);

	return err;
}

static int gp2a_cal_mode_save_file(char mode)
{
	struct file *cal_mode_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_mode_filp = filp_open(OFFSET_FILE_PATH,
		O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (IS_ERR(cal_mode_filp)) {
		pr_err("%s,Can't open cal_mode file\n",
			__func__);
		set_fs(old_fs);
		err = PTR_ERR(cal_mode_filp);
		pr_err("%s,err = %d\n",
			__func__, err);
		return err;
	}

	err = cal_mode_filp->f_op->write(cal_mode_filp,
		(char *)&mode, sizeof(u8), &cal_mode_filp->f_pos);
	if (err != sizeof(u8)) {
		pr_err("%s,Can't read the cal_mode from file\n", __func__);
		err = -EIO;
	}

	filp_close(cal_mode_filp, current->files);
	set_fs(old_fs);

	return err;
}

static ssize_t prox_cal_read(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", gp2a->cal_mode);
}

static ssize_t prox_cal_write(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	int err;

	if (sysfs_streq(buf, "1")) {
		gp2a->cal_mode = 1;
		gp2a->nondetect = PROX_NONDETECT_MODE1;
		gp2a->detect = PROX_DETECT_MODE1;
	} else if (sysfs_streq(buf, "2")) {
		gp2a->cal_mode = 2;
		gp2a->nondetect = PROX_NONDETECT_MODE2;
		gp2a->detect = PROX_DETECT_MODE2;
	} else if (sysfs_streq(buf, "0")) {
		gp2a->cal_mode = 0;
		gp2a->nondetect = PROX_NONDETECT;
		gp2a->detect = PROX_DETECT;
	} else {
		pr_err("%s,invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if (gp2a->power_state == 1) {
		gp2a_power_onoff(gp2a, 0);
		msleep(5);
		gp2a_power_onoff(gp2a, 1);
	}

	err = gp2a_cal_mode_save_file(gp2a->cal_mode);
	if (err < 0) {
		pr_err("%s,prox_cal_write() failed\n", __func__);
		return err;
	}

	return size;
}

static DEVICE_ATTR(adc, 0440, adc_read, NULL);
static DEVICE_ATTR(state, 0440, state_read, NULL);
static DEVICE_ATTR(name, 0440, name_read, NULL);
static DEVICE_ATTR(vendor, 0440, vendor_read, NULL);
static DEVICE_ATTR(prox_cal, 0664, prox_cal_read, prox_cal_write);

static struct device_attribute *proxi_attrs[] = {
	&dev_attr_adc,
	&dev_attr_state,
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_prox_cal,
	NULL,
};
#ifdef CONFIG_SENSORS_POWERCONTROL
static int gp2a_regulator_onoff(struct device *dev, bool onoff)
{
	struct regulator *gp2a_vio;
#ifdef CONFIG_SENSORS_GP2A_VDDCONTROL
	struct regulator *gp2a_vdd;
	struct regulator *gp2a_vdd2;
	int ret = -1;
#endif

	pr_info("%s %s\n", __func__, (onoff) ? "on" : "off");
#ifdef CONFIG_SENSORS_GP2A_VDDCONTROL
	gp2a_vdd = devm_regulator_get(dev, "gp2a-vdd");
	if (IS_ERR(gp2a_vdd)) {
		pr_err("[SENSOR]: %s - cannot get gp2a_vdd\n", __func__);
		return -ENOMEM;
	}
	ret = regulator_set_voltage(gp2a_vdd, 3000000, 3000000);
	if (ret) {
		pr_err("%s: set voltage failed on gp2a_vdd, rc=%d\n",
			__func__, ret);
		return ret;
	}
	gp2a_vdd2 = devm_regulator_get(dev, "gp2a-vdd2");
	if (IS_ERR(gp2a_vdd2)) {
		pr_err("[SENSOR]: %s - cannot get gp2a_vdd2\n", __func__);
		return -ENOMEM;
	}
	ret = regulator_set_voltage(gp2a_vdd2, 3000000, 3000000);
	if (ret) {
		pr_err("%s: set voltage failed on gp2a_vdd2, rc=%d\n",
			__func__, ret);
		return ret;
	}
#endif
	gp2a_vio = devm_regulator_get(dev, "gp2a-vio");
	if (IS_ERR(gp2a_vio)) {
		pr_err("%s: cannot get gp2a-vio\n", __func__);
		return -ENOMEM;
	}

	if (onoff) {
#ifdef CONFIG_SENSORS_GP2A_VDDCONTROL
		regulator_enable(gp2a_vdd);
		regulator_enable(gp2a_vdd2);
#endif
		regulator_enable(gp2a_vio);
	} else {
#ifdef CONFIG_SENSORS_GP2A_VDDCONTROL
		regulator_disable(gp2a_vdd);
		regulator_disable(gp2a_vdd2);
#endif
		regulator_disable(gp2a_vio);
	}
#ifdef CONFIG_SENSORS_GP2A_VDDCONTROL
	devm_regulator_put(gp2a_vdd);
	devm_regulator_put(gp2a_vdd2);
#endif
	devm_regulator_put(gp2a_vio);
	msleep(10);

	return 0;
}
#endif
static ssize_t proximity_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", gp2a->power_state);
}

static ssize_t proximity_enable_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	int value = 0;
	int err = 0;

	err = kstrtoint(buf, 10, &value);
	if (err) {
		pr_err("%s,kstrtoint failed.", __func__);
		goto done;
	}
	if (value != 0 && value != 1) {
		pr_err("%s,wrong value(%d)\n", __func__, value);
		goto done;
	}

	mutex_lock(&gp2a->power_lock);

	if (gp2a->power_state != value) {
		pr_info("%s,enable(%d)\n", __func__, value);

		if (value) {
			err = gp2a_cal_mode_read_file(gp2a);
			if (err < 0 && err != -ENOENT)
				pr_err("%s,cal_mode file read fail\n", __func__);

			pr_info("%s,mode(%d)\n", __func__, gp2a->cal_mode);
			if (gp2a->cal_mode == 2) {
				gp2a->nondetect = PROX_NONDETECT_MODE2;
				gp2a->detect = PROX_DETECT_MODE2;
			} else if (gp2a->cal_mode == 1) {
				gp2a->nondetect = PROX_NONDETECT_MODE1;
				gp2a->detect = PROX_DETECT_MODE1;
			} else {
				gp2a->nondetect = PROX_NONDETECT;
				gp2a->detect = PROX_DETECT;
			}
#ifdef CONFIG_SENSORS_POWERCONTROL
			gp2a_regulator_onoff(&gp2a->i2c_client->dev, true);
#endif
			gp2a_power_onoff(gp2a, 1);
			gp2a->power_state = value;

			gp2a->val_state = value;
			input_report_abs(gp2a->input, ABS_DISTANCE, gp2a->val_state);
			input_sync(gp2a->input);
		} else {
#ifdef CONFIG_SENSORS_POWERCONTROL
			gp2a_regulator_onoff(&gp2a->i2c_client->dev, false);
#endif
			gp2a_power_onoff(gp2a, 0);
			gp2a->power_state = value;
		}

	} else {
		pr_err("%s,wrong cmd for enable\n", __func__);
	}

	mutex_unlock(&gp2a->power_lock);
done:
	return size;
}

static struct device_attribute dev_attr_proximity_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	       proximity_enable_show, proximity_enable_store);

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_proximity_enable.attr,
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};

static void gp2a_prox_work_func(struct work_struct *work)
{
	struct gp2a_data *gp2a = container_of(work,
		struct gp2a_data, work_prox);
	u8 vo, value;

	gp2a_i2c_read(gp2a, REGS_PROX, &vo);
	vo = 0x01 & vo;

	value = 0x18;
	gp2a_i2c_write(gp2a, REGS_CON, value);

	if (!vo) {
		gp2a->val_state = 0x01;
		value = gp2a->nondetect;
	} else {
		gp2a->val_state = 0x00;
		value = gp2a->detect;
	}
	gp2a_i2c_write(gp2a, REGS_HYS, value);

	pr_info("%s,%d\n", __func__, gp2a->val_state);

	input_report_abs(gp2a->input, ABS_DISTANCE, gp2a->val_state);
	input_sync(gp2a->input);
	msleep(5);

	value = 0x00;
	gp2a_i2c_write(gp2a, REGS_CON, value);
}

irqreturn_t gp2a_irq_handler(int irq, void *data)
{
	struct gp2a_data *gp2a = data;
	pr_info("%s,%d\n", __func__, irq);

	schedule_work((struct work_struct *)&gp2a->work_prox);
	wake_lock_timeout(&gp2a->prx_wake_lock, 3*HZ);

	return IRQ_HANDLED;
}

static int gp2a_setup_irq(struct gp2a_data *gp2a)
{
	int rc;
	struct gp2a_platform_data *pdata = gp2a->pdata;
	int irq = -1;
	u8 value;

	rc = gpio_request(pdata->p_out, "gpio_proximity_out");
	if (rc < 0) {
		pr_err("%s,gpio %d request failed (%d)\n",
			__func__, pdata->p_out, rc);
		return rc;
	}

	rc = gpio_direction_input(pdata->p_out);
	if (rc < 0) {
		pr_err("%s,failed gpio %d as input (%d)\n",
			__func__, pdata->p_out, rc);
		goto err_gpio_direction_input;
	}

	value = 0x18;
	gp2a_i2c_write(gp2a, REGS_CON, value);
	irq = gpio_to_irq(pdata->p_out);
	rc = request_irq(irq, gp2a_irq_handler, IRQF_TRIGGER_FALLING,
					"proximity_int", gp2a);

	if (rc < 0) {
		pr_err("%s,request_irq(%d) failed for gpio %d (%d)\n",
			__func__, irq,
			pdata->p_out, rc);
		goto err_request_irq;
	} else{
		pr_info("%s,request_irq(%d) success for gpio %d\n",
			__func__, irq, pdata->p_out);
	}
	disable_irq(irq);
	gp2a->irq = irq;

	value = 0x02;
	gp2a_i2c_write(gp2a, REGS_OPMOD, value);
	goto done;

err_request_irq:
err_gpio_direction_input:
	gpio_free(pdata->p_out);
done:
	return rc;
}
#ifdef CONFIG_SENSORS_POWER_EN
static int gp2a_request_gpio(struct gp2a_platform_data *pdata)
{
	int ret;
	ret = gpio_request(pdata->power_en, "prox_en");
	if(ret) {
		pr_err("%s: gpio request fail\n",__func__);
		return ret;
	}

	ret = gpio_direction_output(pdata->power_en, 0);
	if (ret) {
		pr_err("%s: unable to set_direction [%d]\n",__func__, pdata->power_en);
		return ret;
	}
	return 0;
}
#endif


static int gp2a_parse_dt(struct device *dev, struct gp2a_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;

	pdata->p_out = of_get_named_gpio_flags(np, "gp2a-i2c,irq-gpio",
				0, &flags);
	if (pdata->p_out < 0) {
		pr_err("%s : get irq_gpio(%d) error\n", __func__, pdata->p_out);
		return -ENODEV;
	}
#ifdef CONFIG_SENSORS_POWER_EN
	pdata->power_en = of_get_named_gpio_flags(np, "gp2a-i2c,en-gpio",
				0, &flags);
	if (pdata->power_en < 0) {
		pr_err("%s : get power_en(%d) error\n", __func__, pdata->power_en);
		return -ENODEV;
	}
#endif
	return 0;
}


static int gp2a_i2c_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	int ret = 0;
	struct input_dev *input_dev;
	struct gp2a_data *gp2a;
	struct gp2a_platform_data *pdata = client->dev.platform_data;

	pr_info("%s, start\n", __func__);

#ifdef CONFIG_SENSORS_POWERCONTROL
	ret = gp2a_regulator_onoff(&client->dev, true);
	if (ret) {
		pr_err("%s, Power Up Failed\n", __func__);
		return ret;
	}
#endif

	if(client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
		sizeof(struct gp2a_platform_data), GFP_KERNEL);
		if (!pdata) {
			pr_err("%s,Failed to allocate memory\n", __func__);
			return -ENOMEM;
		}
		ret = gp2a_parse_dt(&client->dev, pdata);
		if (ret < 0)
			return ret;
#ifdef CONFIG_SENSORS_POWER_EN
		ret = gp2a_request_gpio(pdata);
		if (ret < 0)
			return ret;
#endif
	}

	if (!pdata) {
		pr_err("%s,missing pdata\n", __func__);
		return -ENOMEM;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s,i2c functionality failed\n", __func__);
		return -ENOMEM;
	}

	gp2a = kzalloc(sizeof(struct gp2a_data), GFP_KERNEL);
	if (!gp2a) {
		pr_err("%s,failed memory alloc\n",
		       __func__);
		return -ENOMEM;
	}

	gp2a->pdata = pdata;
	gp2a->i2c_client = client;
	i2c_set_clientdata(client, gp2a);

	wake_lock_init(&gp2a->prx_wake_lock, WAKE_LOCK_SUSPEND,
		       "prx_wake_lock");
	mutex_init(&gp2a->power_lock);

	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s,could not allocate input device\n", __func__);
		goto err_input_allocate_device_proximity;
	}

	gp2a->input = input_dev;
	input_dev->name = "proximity_sensor";
	input_set_capability(input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);
	input_set_drvdata(input_dev, gp2a);

	ret = input_register_device(input_dev);
	if (ret < 0) {
		pr_err("%s,could not register input device\n",
			__func__);
		goto err_input_register_device_proximity;
	}
	ret = sensors_create_symlink(&input_dev->dev.kobj, input_dev->name);
	if (ret < 0) {
		input_unregister_device(input_dev);
		return ret;
	}
	ret = sysfs_create_group(&input_dev->dev.kobj,
				 &proximity_attribute_group);

	if (ret) {
		pr_err("%s,create sysfs group error\n", __func__);
		goto err_sysfs_create_group_proximity;
	}

	INIT_WORK(&gp2a->work_prox, gp2a_prox_work_func);

#ifdef CONFIG_SENSORS_POWER_EN
	gp2a_leda_onoff(gp2a, 1);
#endif

	ret = gp2a_setup_irq(gp2a);

	if (ret) {
		pr_err("%s,could not setup irq\n", __func__);
		goto err_setup_irq;
	}

#ifdef CONFIG_SENSORS_POWER_EN
	gp2a_leda_onoff(gp2a, 0);
#endif

	ret = sensors_register(gp2a->dev, gp2a,
		proxi_attrs, "proximity_sensor");
	if (ret < 0) {
		pr_info("%s,could not sensors_register\n", __func__);
		goto exit_gp2a_sensors_register;
	}

	pr_info("%s,%d\n", __func__, __LINE__);

	goto done;

exit_gp2a_sensors_register:
	free_irq(gp2a->irq, gp2a);
	gpio_free(gp2a->pdata->p_out);
err_setup_irq:
#ifdef CONFIG_SENSORS_POWER_EN
	gp2a_leda_onoff(gp2a, 0);
#endif
	sysfs_remove_group(&gp2a->input->dev.kobj,
			   &proximity_attribute_group);
err_sysfs_create_group_proximity:
	input_unregister_device(gp2a->input);
err_input_register_device_proximity:
	input_free_device(input_dev);
err_input_allocate_device_proximity:
	mutex_destroy(&gp2a->power_lock);
	wake_lock_destroy(&gp2a->prx_wake_lock);
	kfree(gp2a);
done:
#ifdef CONFIG_SENSORS_POWERCONTROL
	gp2a_regulator_onoff(&client->dev, false);
#endif
	return ret;
}

static void gp2a_i2c_shutdown(struct i2c_client *client)
{
	struct gp2a_data *gp2a = i2c_get_clientdata(client);
	if (gp2a != NULL) {
		if (gp2a->power_state) {
			disable_irq_wake(gp2a->irq);
			disable_irq(gp2a->irq);
			msleep(5);
		}
		sysfs_remove_group(&gp2a->input->dev.kobj,
				   &proximity_attribute_group);
		input_unregister_device(gp2a->input);

		free_irq(gp2a->irq, gp2a);
		gpio_free(gp2a->pdata->p_out);
		mutex_destroy(&gp2a->power_lock);
		wake_lock_destroy(&gp2a->prx_wake_lock);
		kfree(gp2a);
	}
}

static const struct i2c_device_id gp2a_device_id[] = {
	{"gp2a", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, gp2a_device_id);

static struct of_device_id gp2a_i2c_match_table[] = {
	{ .compatible = "gp2a-i2c",},
	{},
};

MODULE_DEVICE_TABLE(of, gp2a_i2c_match_table);

static struct i2c_driver gp2a_i2c_driver = {
	.driver = {
		.name = "gp2a",
		.owner = THIS_MODULE,
		.of_match_table = gp2a_i2c_match_table,

	},
	.probe		= gp2a_i2c_probe,
	.shutdown	= gp2a_i2c_shutdown,
	.id_table	= gp2a_device_id,
};

module_i2c_driver(gp2a_i2c_driver);

MODULE_AUTHOR("mjchen@sta.samsung.com");
MODULE_DESCRIPTION("Optical Sensor driver for gp2ap002");
MODULE_LICENSE("GPL");
