/*
 * Copyright (c) 2011 SAMSUNG ELECTRONICS
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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/wakelock.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/moduleparam.h>
#include <linux/sensors_core.h>

#define VENDOR		"SAMSUNG"
#define CHIP_ID		"COMMON"

#define ECG_VERSION_FILE_PATH	"/efs/ecg_ver"

#define EVENT_TYPE_STATUS	REL_HWHEEL
#define EVENT_TYPE_RESULT	REL_DIAL

struct bio_data {
	struct platform_device *pdev;
	struct miscdevice bio_device;
	struct wake_lock bio_wake_lock;

	struct work_struct strip_work;
	struct workqueue_struct *report_queue;
	struct workqueue_struct *indication_queue;
	struct delayed_work report_work;
	struct delayed_work indication_work;
	struct input_dev *input;
	struct device *bio_dev;
	u8 enabled;
	int strip_irq;
	int notify_cmd;
	int state_cmd;
	u32 indication;
	u32 strip_status;
	int bgm_state;
	int bgm_timeout;
	u32 ecg_enable;
	u32 bgm_enable;
	int check_loop;
};

static void report_state(struct bio_data *data)
{
	input_report_rel(data->input, EVENT_TYPE_STATUS,
			data->bgm_state + 1);
	input_sync(data->input);
}

static void start_check(struct bio_data *data)
{
	pr_info("[bgm] %s\n", __func__);
	data->check_loop = 1;
	queue_delayed_work(data->indication_queue,
			&data->indication_work, HZ/2);
}

static void stop_check(struct bio_data *data)
{
	pr_info("[bgm] %s\n", __func__);
	data->check_loop = 0;
	//cancel_delayed_work(&data->indication_work);
}

static void indication_func(struct work_struct *work)
{
	struct bio_data *data = container_of(work,
		struct bio_data, indication_work.work);
	int strip_value, indi_value, bgm_state;

	indi_value = gpio_get_value(data->indication);
	strip_value = gpio_get_value(data->strip_status);
	bgm_state = data->bgm_state;
	pr_info("[bgm] %s : strip=%d, indication=%d, bgm_state=%d\n",
		__func__, strip_value, indi_value, data->bgm_state);
#if 0
	if (bgm_state == 0 && indi_value == 0 && strip_value == 1) {
	/* initial state */
		pr_info("[bgm] %s : Initial state check", __func__);

		if (data->bgm_timeout == 0) {
			pr_info("\n[bgm] %s : Initial state OK", __func__);
			report_state(data);
		} else if (data->bgm_timeout == 4) {
			pr_info("\n[bgm] %s : Initial state T_ind timeout", __func__);
			bgm_state = 100;
			report_state(data);
			bgm_state = 0;
		}
		//data->bgm_timeout++;
		pr_info(",[bgm] time out count is %d\n", data->bgm_timeout);
	} else 
#endif
	if (bgm_state == 0 && indi_value == 1 && strip_value == 0) {
	/* strip inserted m Startup check OK */
		pr_info("[bgm] %s : Start-up check OK\n", __func__);
		bgm_state = 1;
		report_state(data);
	} else if (bgm_state == 1 && indi_value == 0) {
	/* Waiting sample */
		pr_info("[bgm] %s : Waiting sample\n", __func__);
		bgm_state = 2;
		report_state(data);
	} else if (bgm_state == 2 && indi_value == 1) {
	/* Sample applied */
		pr_info("[bgm] %s : Sample applied\n", __func__);
		bgm_state = 3;
		report_state(data);
	} else if (bgm_state == 3 && indi_value == 1
			&& data->notify_cmd == 1) {
	/* Check complete, send sleep packet */
		pr_info("[bgm] %s : Send sleep\n", __func__);
		bgm_state = 4;
		report_state(data);
	}

	data->bgm_state = bgm_state;
	if (data->check_loop)
		queue_delayed_work(data->indication_queue,
			&data->indication_work, HZ/2);
}

static void report_func(struct work_struct *work)
{
	struct bio_data *data = container_of(work,
		struct bio_data, report_work.work);
	pr_info("[bgm] %s : %d\n", __func__, data->notify_cmd + 1);
	input_report_rel(data->input, EVENT_TYPE_STATUS,
						data->notify_cmd + 1);
	input_sync(data->input);
}

static irqreturn_t bio_strip_irq_handler(int irq, void *dev_id)
{
	struct bio_data *data = dev_id;
	wake_lock_timeout(&data->bio_wake_lock, 3 * HZ);

	//schedule_work(&data->strip_work);

	pr_info("[bgm] %s : IRQ_HANDLED. strip %s\n", __func__,
		(gpio_get_value(data->strip_status))?"removed":"inserted");
	return IRQ_HANDLED;
}

static void strip_func(struct work_struct *work)
{
	struct bio_data *data = container_of((struct work_struct *)work,
					struct bio_data, strip_work);
	int ret;

	ret = gpio_get_value(data->strip_status);
	pr_info("[bgm] %s : strip gpio value is %d\n", __func__, ret);
}

static int bio_parse_dt(struct device *dev, struct bio_data *data)
{
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int errorno = 0;
	int irq;
	int rc;

	pr_info("[bgm] %s : bio,bgm_indication\n", __func__);
	data->indication = of_get_named_gpio_flags(np, "bio,bgm_indication",
		0, &flags);
	if (data->indication < 0) {
		errorno = data->indication;
		pr_err("[bgm] %s : bio,bgm_indication error\n", __func__);
	} else {
		errorno = gpio_request(data->indication, "BGMINDI");
		if (errorno) {
			pr_err("[bgm] %s : failed to request indication PIN\n",
				__func__);
		} else {
			errorno = gpio_direction_input(data->indication);
			if (errorno) {
			pr_err("[bgm] %s : failed to set indication PIN as output\n",
				__func__);
			}
		}
	}

	pr_info("[bgm] %s : bio,bgm_strip_status\n", __func__);
	data->strip_status = of_get_named_gpio_flags(np, "bio,bgm_strip_status",
		0, &flags);
	if (data->strip_status < 0) {
		errorno = data->strip_status;
		pr_err("[bgm] %s : bio,bgm_strip_status\n", __func__);
	} else {
		errorno = gpio_request(data->strip_status, "BGMSTRIP");
		if (errorno) {
			pr_err("[bgm] %s : failed to request bgm_strip_status PIN\n",
				__func__);
		} else	{
			errorno = gpio_direction_input(data->strip_status);
			if (errorno) {
				pr_err("[bgm] %s : failed to set strip_status PIN as output\n",
					__func__);
			}
		}
	}

	if (data->strip_status >= 0) {
		irq = gpio_to_irq(data->strip_status);
	
		rc = request_threaded_irq(irq, NULL,
				bio_strip_irq_handler,
				IRQF_TRIGGER_FALLING |
				IRQF_TRIGGER_RISING |
				IRQF_ONESHOT,
				"bio_strip_int",
				data);
		if (rc < 0) {
			pr_err("[bgm] %s : request_irq(%d) failed for gpio %d (%d)\n",
				__func__, irq,
				data->strip_status, rc);
			//goto dt_exit;
		}

		disable_irq(irq);
		data->strip_irq = irq;
	}


#if 1
	pr_info("[ecg] %s : bio,ecg_enable\n", __func__);
	data->ecg_enable = of_get_named_gpio_flags(np, "bio,ecg_enable",
		0, &flags);
	if (data->ecg_enable < 0) {
		errorno = data->ecg_enable;
		pr_err("[ecg] %s: bio,ecg_enable\n", __func__);
	} else {
		errorno = gpio_request(data->ecg_enable, "bio_ECG_ENABLE");
		if (errorno) {
			pr_err("[ecg] %s: failed to request ecg_enable PIN \n",
				__func__);
		} else {
			errorno = gpio_direction_output(data->ecg_enable, 0);
			if (errorno) {
				pr_err("[ecg] %s: failed to set ecg_enable PIN as output\n",
					__func__);
			}
		}
	}

	pr_info("[bgm] %s : bio,bgm_enable\n", __func__);
	data->bgm_enable = of_get_named_gpio_flags(np, "bio,bgm_enable",
		0, &flags);
	if (data->bgm_enable < 0) {
		errorno = data->bgm_enable;
		pr_err("[bgm] %s: bio,bgm_enable\n", __func__);
	} else {
		errorno = gpio_request(data->bgm_enable, "bio_BGM_ENABLE");
		if (errorno) {
			pr_err("[bgm] %s: failed to request bgm_enable PIN \n",
				__func__);
		} else {
			errorno = gpio_direction_output(data->bgm_enable, 1);
			if (errorno) {
				pr_err("[bgm] %s: failed to set bgm_enable PIN as output\n",
					__func__);
			}
		}
	}
#endif

	return 0;

//dt_exit:
	//return errorno;
}
/*
static ssize_t bio_io_read(struct file *filp, char __user *buf,
			      size_t count, loff_t *offset)
{
	//struct bio_data *data = filp->private_data;
	//char tmp[MAX_BUFFER_SIZE] = {0, };
	int ret = 0;
	pr_info("[bgm] %s :\n", __func__);
//ret = wait_event_interruptible(pn547_dev->read_wq,
	return ret;
}
static ssize_t bio_io_write(struct file *filp, const char __user *buf,
			       size_t count, loff_t *offset)
{
	int ret = 0;
	struct bio_data *data =filp->private_data;
	pr_info("[bgm] %s :\n", __func__);




	return ret;
}

static long bio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
//	void __user *argp = (void __user *)arg;
//	struct bio_data *data = container_of(filp->private_data,
//					     struct bio_data, bio_device);

	switch (cmd) {
	default:
		return -ENOTTY;
	}
	return 0;
}

*/
static int bio_io_open(struct inode *inode, struct file *filp)
{
	pr_info("[bgm] %s :\n", __func__);
	return 0;
}

static int bio_io_release(struct inode *inode, struct file *filp)
{
	pr_info("[bgm] %s :\n", __func__);
	return 0;
}

static const struct file_operations bio_fops = {
	.owner   = THIS_MODULE,
	.open    = bio_io_open,
	.release = bio_io_release,
	//.unlocked_ioctl = bio_ioctl,
	.llseek = no_llseek,
	//.read = bio_io_read,
	//.write = bio_io_write,
};

static ssize_t bio_enable_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct bio_data *data = dev_get_drvdata(dev);
	int new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("[bgm] %s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if (new_value && !data->enabled) {
		pr_err("[bgm] %s : bgm enable, polling start, sensor on\n", __func__);
		data->bgm_state = 0;
		data->bgm_timeout = 0;
		data->notify_cmd = 0;
		data->enabled = 1;
		//gpio_direction_output(data->bgm_enable, 1);
		msleep(200);
		start_check(data);
	} else if (!new_value && data->enabled) {
		pr_err("[bgm] %s : bgm enable, polling stop, sensor off\n", __func__);
		data->enabled = 0;
		stop_check(data);
		//gpio_direction_output(data->bgm_enable, 0);
	}

	return size;
}

static ssize_t bio_notify_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct bio_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->notify_cmd);
}

static ssize_t bio_notify_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int err;
	long noti_cmd;
	struct bio_data *data = dev_get_drvdata(dev);

	err = strict_strtol(buf, 10, &noti_cmd);

	pr_info("[bgm] %s : %s = 0x%02x\n", __func__, buf, (int)noti_cmd);

	if (err < 0)
		return -EINVAL;

	if (noti_cmd == 1)
		data->notify_cmd = 1;
	else { /* error or test state */
		input_report_rel(data->input, EVENT_TYPE_STATUS, noti_cmd + 1);
		input_sync(data->input);
		data->bgm_state = 0;
		data->bgm_timeout = 0;
		data->notify_cmd = 0;
	}
	//queue_delayed_work(data->report_queue, &data->report_work, HZ*2);

	//TODO : input report

	return size;
}

static ssize_t ecg_onoff_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct bio_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", gpio_get_value(data->ecg_enable));
}

static ssize_t ecg_onoff_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int err;
	long power_gpio;
	struct bio_data *data = dev_get_drvdata(dev);

	err = strict_strtol(buf, 10, &power_gpio);

	pr_info("[ecg] %s : %s = %d\n", __func__, buf, (int)power_gpio);

	if (err < 0)
		return -EINVAL;

	if (power_gpio == 1)
		gpio_direction_output(data->ecg_enable, 1);
	else
		gpio_direction_output(data->ecg_enable, 0);

	return size;
}

static ssize_t ecg_fw_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	//struct bio_data *data = dev_get_drvdata(dev);
	struct file *version_filp = NULL;
	mm_segment_t old_fs;
	int err;
	int version;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	version_filp = filp_open(ECG_VERSION_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(version_filp)) {
		err = PTR_ERR(version_filp);
		if (err != -ENOENT)
			pr_err("[bgm] %s: Can't open version file\n", __func__);
		set_fs(old_fs);
		version = 0;
		return sprintf(buf, "%d\n", version);
	}

	err = version_filp->f_op->read(version_filp,
		(char *)&version, sizeof(int), &version_filp->f_pos);

	if (err != sizeof(int)) {
		filp_close(version_filp, NULL);
		set_fs(old_fs);
		version = 0;
		return sprintf(buf, "%d\n", version);
	}

	filp_close(version_filp, NULL);
	set_fs(old_fs);

	return sprintf(buf, "%d\n", version);
}

static ssize_t ecg_fw_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int err;
	long temp;
	int version;
	//struct bio_data *data = dev_get_drvdata(dev);
	struct file *version_filp = NULL;
	mm_segment_t old_fs;

	err = strict_strtol(buf, 10, &temp);
	version = (int)temp;
	pr_info("[bgm] %s : %s = %d\n", __func__, buf, version);

	if (err < 0)
		return -EINVAL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	version_filp = filp_open(ECG_VERSION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0666);

	if (IS_ERR(version_filp)) {
		pr_err("[bgm] %s: Can't open version file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(version_filp);
		return size;
	}

	err = version_filp->f_op->write(version_filp,
		(char *)&version, sizeof(int), &version_filp->f_pos);
	if (err != sizeof(int)) {
		pr_err("[bgm] %s: Can't write the version data to file\n", __func__);
		filp_close(version_filp, NULL);
		set_fs(old_fs);
		return size;
	}

	filp_close(version_filp, NULL);
	set_fs(old_fs);

	return size;
}


static ssize_t bio_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct bio_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->enabled);
}

static struct device_attribute dev_attr_enable =
__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	bio_enable_show, bio_enable_store);

static struct attribute *bio_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group bio_attribute_group = {
	.attrs = bio_sysfs_attrs,
};

/* sysfs for vendor & name */
static ssize_t bio_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t bio_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static DEVICE_ATTR(vendor, 0644, bio_vendor_show, NULL);
static DEVICE_ATTR(name, 0644, bio_name_show, NULL);
static DEVICE_ATTR(notify, 0666, bio_notify_show, bio_notify_store);
static DEVICE_ATTR(onoff, 0666, ecg_onoff_show, ecg_onoff_store);
static DEVICE_ATTR(fw, 0666, ecg_fw_show, ecg_fw_store);

static struct device_attribute *bio_sensor_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_notify,
	&dev_attr_onoff,
	&dev_attr_fw,
	NULL,
};

static int bio_probe(struct platform_device *dev)
{
	int ret;
	struct bio_data *data;

	pr_info("[bgm] %s : bio_probe\n", __func__);

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL) {
		pr_err("%s, failed to alloc memory for module data\n",
			__func__);
		ret = -ENOMEM;
		goto exit;
	}

	data->pdev = platform_device_register_simple("bio", -1, NULL, 0);
	if (IS_ERR(data->pdev)) {
		ret = PTR_ERR(data->pdev);
		goto out_driver;
	}

	if (dev->dev.of_node)
		pr_info("[bgm] %s : of node\n", __func__);
	else {
		pr_err("%s : no of node\n", __func__);
		goto out_device;
	}

	ret = bio_parse_dt(&dev->dev, data);
	if (ret) {
		pr_err("%s : parse dt error\n", __func__);
		//goto err_parse_dt;
	}

	data->bgm_state = 0;
	data->bgm_timeout = 0;
	data->strip_irq = 0;
	data->bio_device.minor = MISC_DYNAMIC_MINOR;
	data->bio_device.name = "bio";
	data->bio_device.fops = &bio_fops;
	ret = misc_register(&data->bio_device);
	if (ret)
		goto exit_misc_device_register_failed;

	wake_lock_init(&data->bio_wake_lock,
			WAKE_LOCK_SUSPEND, "bio_wake_lock");
	INIT_WORK(&data->strip_work, strip_func);
	INIT_DELAYED_WORK(&data->report_work, report_func);
	INIT_DELAYED_WORK(&data->indication_work, indication_func);
	data->report_queue =
		create_singlethread_workqueue(dev_name(&data->pdev->dev));
	data->indication_queue =
		create_singlethread_workqueue(dev_name(&data->pdev->dev));


	data->input = input_allocate_device();
	if (!data->input) {
		pr_err("[bgm] %s : could not allocate mlx90615 input device\n", __func__);
		goto err_input_allocate_device;
	}

	data->input->name = "bio_sensor";
	input_set_capability(data->input, EV_REL, EVENT_TYPE_STATUS);
	//input_set_capability(data->input, EV_REL, EVENT_TYPE_OBJECT_TEMP);
	input_set_drvdata(data->input, data);

	ret = input_register_device(data->input);
	if (ret < 0) {
		input_free_device(data->input);
		pr_err("[bgm] %s : could not register input device\n", __func__);
		//goto err_input_register_device;
	}

	ret = sensors_create_symlink(&data->input->dev.kobj,
					data->input->name);
	if (ret < 0) {
		input_unregister_device(data->input);
		goto err_sysfs_create_symlink;
	}

	ret = sysfs_create_group(&data->input->dev.kobj,
				&bio_attribute_group);
	if (ret) {
		pr_err("[bgm] %s : could not create sysfs group\n", __func__);
		goto err_sysfs_create_group;
	}
	ret = sensors_register(data->bio_dev,
			data, bio_sensor_attrs, "bio_sensor");
	if (ret) {
		pr_err("[bgm] %s : cound not register prox sensor device(%d).\n",
			__func__, ret);
		goto err_sysfs_create_symlink;
	}
	platform_set_drvdata(dev,data);
	pr_info("[bgm] %s : success.\n", __func__);
	//if (data->strip_irq)
		//enable_irq(data->strip_irq);
	return 0;
err_input_allocate_device:
err_sysfs_create_symlink:
	input_free_device(data->input);
err_sysfs_create_group:
	input_unregister_device(data->input);

exit_misc_device_register_failed:
	misc_deregister(&data->bio_device);

out_device:
	platform_device_unregister(data->pdev);
out_driver:
	//mutex_destroy(&data->bio_lock);
	kfree(data);
exit:
	pr_err("[BIO] %s : failed!\n", __func__);
	return ret;
}

static int bio_remove(struct platform_device *dev)
{
	struct bio_data *data = platform_get_drvdata(dev);

	misc_deregister(&data->bio_device);
	platform_device_unregister(data->pdev);

	kfree(data);

	return 0;
}

static int bio_suspend(struct device *dev)
{
	//struct bio_data *data = dev_get_drvdata(dev);
	//gpio_direction_output(data->ecg_enable, 0);
	return 0;
}

static int bio_resume(struct device *dev)
{
	struct bio_data *data = dev_get_drvdata(dev);
	//gpio_direction_output(data->ecg_enable, 1);
	cancel_delayed_work(&data->report_work);
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id bio_match_table[] = {
	{.compatible = "bio_sensor",},
	{},
};
#endif

static const struct dev_pm_ops bio_pm_ops = {
	.suspend = bio_suspend,
	.resume = bio_resume
};

static struct platform_driver bio_driver = {
	.driver	= {
#ifndef CONFIG_HAS_EARLYSUSPEND
		.pm = &bio_pm_ops,
#endif
		.name = "bio_sensor",
		.owner = THIS_MODULE,
		.of_match_table = bio_match_table,
	},
	.probe = bio_probe,
	.remove = bio_remove,
};

static int __init bio_init(void)
{
	return platform_driver_register(&bio_driver);
}

static void __exit bio_exit(void)
{
	platform_driver_unregister(&bio_driver);
}

module_init(bio_init);
module_exit(bio_exit);

MODULE_DESCRIPTION("BIO sensor control");
MODULE_AUTHOR("Samsung Electronic");
MODULE_LICENSE("GPL v2");
