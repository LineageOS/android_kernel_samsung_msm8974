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
#include <linux/sensors_core.h>

#define VENDOR		"SAMSUNG"
#define CHIP_ID		"COMMON"

#define EVENT_TYPE_STATUS	REL_HWHEEL
#define EVENT_TYPE_RESULT	REL_DIAL

struct bgm_data {
	struct platform_device *pdev;
	struct miscdevice bgm_device;
	struct wake_lock bgm_wake_lock;
	
	struct work_struct strip_work;
	struct workqueue_struct *report_queue;
	struct workqueue_struct *indication_queue;
	struct delayed_work report_work;
	struct delayed_work indication_work;
	struct input_dev *input;
	struct device *bgm_dev;
	u8 enabled;
	int strip_irq;
	int notify_cmd;
	int state_cmd;
	u32 indication;
	u32 strip_status;
	int bgm_state;
	u32 ecg_enable;
	int check_loop;
};

static void report_state(struct bgm_data *data)
{
	input_report_rel(data->input, EVENT_TYPE_STATUS,
			data->bgm_state + 1);
	input_sync(data->input);
}

static void start_check(struct bgm_data *data)
{
	data->check_loop = 1;
	queue_delayed_work(data->indication_queue,
			&data->indication_work, HZ/2);
}

static void stop_check(struct bgm_data *data)
{
	data->check_loop = 0;
	//cancel_delayed_work(&data->indication_work);
}

static void indication_func(struct work_struct *work)
{
	struct bgm_data *data = container_of(work,
		struct bgm_data, indication_work.work);
	int strip_value, indi_value;

	indi_value = gpio_get_value(data->indication);
	strip_value = gpio_get_value(data->strip_status);
	pr_info("[BGM] %s : strip=%d, indication=%d\n", __func__, strip_value, indi_value);

	if (indi_value == 1 && data->bgm_state == 0 && strip_value == 0) {
		pr_info("[BGM] %s : Start-up check OK\n", __func__);
		data->bgm_state = 1;
		report_state(data);
	} else 	if (indi_value == 0 && data->bgm_state == 1) {
		pr_info("[BGM] %s : Waiting sample\n", __func__);
		data->bgm_state = 2;
		report_state(data);
	} else 	if (indi_value == 1 && data->bgm_state == 2) {
		pr_info("[BGM] %s : Sample applied\n", __func__);
		data->bgm_state = 3;
		report_state(data);
	} else 	if (indi_value == 1 && data->bgm_state == 3
			&& data->notify_cmd == 1) {
		pr_info("[BGM] %s : Send sleep\n", __func__);
		data->bgm_state = 4;
		report_state(data);
	}

	if (data->check_loop)
		queue_delayed_work(data->indication_queue,
			&data->indication_work, HZ/2);
}

static void report_func(struct work_struct *work)
{
	struct bgm_data *data = container_of(work,
		struct bgm_data, report_work.work);
	pr_info("[BGM] %s : %d\n", __func__, data->notify_cmd + 1);
	input_report_rel(data->input, EVENT_TYPE_STATUS,
						data->notify_cmd + 1);
	input_sync(data->input);
}

static irqreturn_t bgm_strip_irq_handler(int irq, void *dev_id)
{
	struct bgm_data *data = dev_id;
	wake_lock_timeout(&data->bgm_wake_lock, 3 * HZ);

	//schedule_work(&data->strip_work);

	pr_info("[BGM] %s : IRQ_HANDLED. %d\n", __func__,
		gpio_get_value(data->strip_status));
	return IRQ_HANDLED;
}



static void strip_func(struct work_struct *work)
{
	struct bgm_data *data = container_of((struct work_struct *)work,
					struct bgm_data, strip_work);
	int ret;

	ret = gpio_get_value(data->strip_status);
	pr_info("[BGM] %s : strip gpio value is %d\n", __func__, ret);
}

static int bgm_parse_dt(struct device *dev, struct bgm_data *data)
{
	struct device_node *np = dev->of_node;
	enum of_gpio_flags flags;
	int errorno = 0;
	int irq;
	int rc;

	pr_info("[BGM] %s : bgm,indication\n", __func__);
	data->indication = of_get_named_gpio_flags(np, "bgm,indication",
		0, &flags);
	if (data->indication < 0) {
		errorno = data->indication;
		pr_err("[BGM] %s : bgm,indication error\n", __func__);
	} else {
		
		errorno = gpio_request(data->indication, "BGMINDI");
		if (errorno) {
			pr_err("[BGM] %s : failed to request indication PIN\n",
				__func__);
		} else {
			errorno = gpio_direction_input(data->indication);
			if (errorno) {
			pr_err("[BGM] %s : failed to set indication PIN as output\n",
				__func__);
			}
		}
	}

	pr_info("[BGM] %s : bgm,strip_status\n", __func__);
	data->strip_status = of_get_named_gpio_flags(np, "bgm,strip_status",
		0, &flags);
	if (data->strip_status < 0) {
		errorno = data->strip_status;
		pr_err("[BGM] %s : bgm,strip_status\n", __func__);
	} else {
		
		errorno = gpio_request(data->strip_status, "BGMSTRIP");
		if (errorno) {
			pr_err("[BGM] %s : failed to request strip_status PIN\n",
				__func__);
		} else	{
			errorno = gpio_direction_input(data->strip_status);
			if (errorno) {
				pr_err("[BGM] %s : failed to set strip_status PIN as output\n",
					__func__);
			}
		}
	}

	if (data->strip_status >= 0) {
		irq = gpio_to_irq(data->strip_status);
	
		rc = request_threaded_irq(irq, NULL,
				bgm_strip_irq_handler,
				IRQF_TRIGGER_FALLING |
				IRQF_TRIGGER_RISING |
				IRQF_ONESHOT,
				"bgm_ind_int",
				data);
		if (rc < 0) {
			pr_err("[BGM] %s : request_irq(%d) failed for gpio %d (%d)\n",
				__func__, irq,
				data->strip_status, rc);
			//goto dt_exit;
		}
	
		disable_irq(irq);
		data->strip_irq = irq;
	}


#if 1
	pr_info("[BGM] %s : bgm,ecg_enable\n", __func__);
	data->ecg_enable = of_get_named_gpio_flags(np, "bgm,ecg_enable",
		0, &flags);
	if (data->ecg_enable < 0) {
		errorno = data->ecg_enable;
		pr_err("[BGM] %s: mlx90615,ecg_enable\n", __func__);
	} else {
		errorno = gpio_request(data->ecg_enable, "BGM_ECG_ENABLE");
		if (errorno) {
			pr_err("[BGM] %s: failed to request ecg_enable PIN \n",
				__func__);
		} else {
			errorno = gpio_direction_output(data->ecg_enable, 1);
			if (errorno) {
				pr_err("[BGM] %s: failed to set ecg_enable PIN as output\n",
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
static ssize_t bgm_io_read(struct file *filp, char __user *buf,
			      size_t count, loff_t *offset)
{
	//struct bgm_data *data = filp->private_data;
	//char tmp[MAX_BUFFER_SIZE] = {0, };
	int ret = 0;
	pr_info("[BGM] %s :\n", __func__);
//ret = wait_event_interruptible(pn547_dev->read_wq,
	return ret;
}
static ssize_t bgm_io_write(struct file *filp, const char __user *buf,
			       size_t count, loff_t *offset)
{
	int ret = 0;
	struct bgm_data *data =filp->private_data;
	pr_info("[BGM] %s :\n", __func__);




	return ret;
}

static long bgm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
//	void __user *argp = (void __user *)arg;
//	struct bgm_data *data = container_of(filp->private_data,
//					     struct bgm_data, bgm_device);

	switch (cmd) {
	default:
		return -ENOTTY;
	}
	return 0;
}

*/
static int bgm_io_open(struct inode *inode, struct file *filp)
{
	pr_info("[BGM] %s :\n", __func__);
	return 0;
}

static int bgm_io_release(struct inode *inode, struct file *filp)
{
	pr_info("[BGM] %s :\n", __func__);
	return 0;
}

static const struct file_operations bgm_fops = {
	.owner   = THIS_MODULE,
	.open    = bgm_io_open,
	.release = bgm_io_release,
	//.unlocked_ioctl = bgm_ioctl,
	.llseek = no_llseek,
	//.read = bgm_io_read,
	//.write = bgm_io_write,
};

static ssize_t bgm_enable_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct bgm_data *data = dev_get_drvdata(dev);
	int new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("[BGM] %s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if (new_value && !data->enabled) {
		pr_err("[BGM] %s : bgm enable, polling start, sensor on\n", __func__);
		data->bgm_state = 0;
		data->notify_cmd = 0;
		data->enabled = 1;
		gpio_direction_output(data->ecg_enable, 0);
		msleep(300);
		gpio_direction_output(data->ecg_enable, 1);
		start_check(data);
	} else if (!new_value && data->enabled) {
		data->enabled = 0;
		pr_err("[BGM] %s : bgm enable, polling stop, sensor off\n", __func__);
		gpio_direction_output(data->ecg_enable, 1);
		stop_check(data);
	}

	return size;
}

static ssize_t bgm_notify_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int err;
	long noti_cmd;
	struct bgm_data *data = dev_get_drvdata(dev);

	err = strict_strtol(buf, 10, &noti_cmd);

	pr_info("[BGM] %s : %s = 0x%02x\n", __func__, buf, (int)noti_cmd);

	if (err < 0)
		return -EINVAL;

	if (noti_cmd == 1)
		data->notify_cmd = 1;
	else { /* error or test state */
		input_report_rel(data->input, EVENT_TYPE_STATUS, noti_cmd + 1);
		input_sync(data->input);
		data->bgm_state = 0;
		data->notify_cmd = 0;
	}
	//queue_delayed_work(data->report_queue, &data->report_work, HZ*2);

	//TODO : input report
	
	return size;
}

static ssize_t bgm_notify_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct bgm_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->notify_cmd);
}


static ssize_t bgm_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct bgm_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->enabled);
}

static struct device_attribute dev_attr_enable =
__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	bgm_enable_show, bgm_enable_store);

static struct attribute *bgm_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group bgm_attribute_group = {
	.attrs = bgm_sysfs_attrs,
};

/* sysfs for vendor & name */
static ssize_t bgm_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR);
}

static ssize_t bgm_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_ID);
}

static DEVICE_ATTR(vendor, 0644, bgm_vendor_show, NULL);
static DEVICE_ATTR(name, 0644, bgm_name_show, NULL);
static DEVICE_ATTR(notify, 0666, bgm_notify_show, bgm_notify_store);


static struct device_attribute *bgm_sensor_attrs[] = {
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_notify,
	NULL,
};

static int bgm_probe(struct platform_device *dev)
{
	int ret;
	struct bgm_data *data;

	pr_info("[BGM] %s : bgm_probe\n", __func__);

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL) {
		pr_err("%s, failed to alloc memory for module data\n",
			__func__);
		ret = -ENOMEM;
		goto exit;
	}

	data->pdev = platform_device_register_simple("bgm", -1, NULL, 0);
	if (IS_ERR(data->pdev)) {
		ret = PTR_ERR(data->pdev);
		goto out_driver;
	}

	if (dev->dev.of_node)
		pr_info("[BGM] %s : of node\n", __func__);
	else {
		pr_err("%s : no of node\n", __func__);
		goto out_device;
	}

	ret = bgm_parse_dt(&dev->dev, data);
	if (ret) {
		pr_err("%s : parse dt error\n", __func__);
		//goto err_parse_dt;
	}

	data->bgm_state = 0;

	data->bgm_device.minor = MISC_DYNAMIC_MINOR;
	data->bgm_device.name = "bgm";
	data->bgm_device.fops = &bgm_fops;
	ret = misc_register(&data->bgm_device);
	if (ret)
		goto exit_misc_device_register_failed;

	wake_lock_init(&data->bgm_wake_lock,
			WAKE_LOCK_SUSPEND, "bgm_wake_lock");
	INIT_WORK(&data->strip_work, strip_func);
	INIT_DELAYED_WORK(&data->report_work, report_func);
	INIT_DELAYED_WORK(&data->indication_work, indication_func);
	data->report_queue =
		create_singlethread_workqueue(dev_name(&data->pdev->dev));
	data->indication_queue =
		create_singlethread_workqueue(dev_name(&data->pdev->dev));


	data->input = input_allocate_device();
	if (!data->input) {
		pr_err("[BGM] %s : could not allocate mlx90615 input device\n", __func__);
		goto err_input_allocate_device;
	}

	data->input->name = "bgm_sensor";
	input_set_capability(data->input, EV_REL, EVENT_TYPE_STATUS);
	//input_set_capability(data->input, EV_REL, EVENT_TYPE_OBJECT_TEMP);
	input_set_drvdata(data->input, data);

	ret = input_register_device(data->input);
	if (ret < 0) {
		input_free_device(data->input);
		pr_err("[BGM] %s : could not register input device\n", __func__);
		//goto err_input_register_device;
	}

	ret = sensors_create_symlink(&data->input->dev.kobj,
					data->input->name);
	if (ret < 0) {
		input_unregister_device(data->input);
		goto err_sysfs_create_symlink;
	}

	ret = sysfs_create_group(&data->input->dev.kobj,
				&bgm_attribute_group);
	if (ret) {
		pr_err("[BGM] %s : could not create sysfs group\n", __func__);
		goto err_sysfs_create_group;
	}
	ret = sensors_register(data->bgm_dev,
			data, bgm_sensor_attrs, "bgm_sensor");
	if (ret) {
		pr_err("[BGM] %s : cound not register prox sensor device(%d).\n",
			__func__, ret);
		goto err_sysfs_create_symlink;
	}
	platform_set_drvdata(dev,data);
	pr_info("[BGM] %s : success.\n", __func__);

	return 0;
err_input_allocate_device:
err_sysfs_create_symlink:
	input_free_device(data->input);
err_sysfs_create_group:
	input_unregister_device(data->input);

exit_misc_device_register_failed:
	misc_deregister(&data->bgm_device);

out_device:
	platform_device_unregister(data->pdev);
out_driver:
	//mutex_destroy(&data->bgm_lock);
	kfree(data);
exit:
	pr_err("[BGM] %s : failed!\n", __func__);
	return ret;
}

static int bgm_remove(struct platform_device *dev)
{
	struct bgm_data *data = platform_get_drvdata(dev);

	misc_deregister(&data->bgm_device);
	platform_device_unregister(data->pdev);

	kfree(data);

	return 0;
}

static int bgm_suspend(struct device *dev)
{
	struct bgm_data *data = dev_get_drvdata(dev);
	gpio_direction_output(data->ecg_enable, 0);
	return 0;
}

static int bgm_resume(struct device *dev)
{
	struct bgm_data *data = dev_get_drvdata(dev);
	gpio_direction_output(data->ecg_enable, 1);
	cancel_delayed_work(&data->report_work);
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id bgm_match_table[] = {
	{.compatible = "blood_glucose",},
	{},
};
#endif

static const struct dev_pm_ops bgm_pm_ops = {
	.suspend = bgm_suspend,
	.resume = bgm_resume
};

static struct platform_driver bgm_driver = {
	.driver	= {
#ifndef CONFIG_HAS_EARLYSUSPEND
		.pm = &bgm_pm_ops,
#endif
		.name = "blood_glucose",
		.owner = THIS_MODULE,
		.of_match_table = bgm_match_table,
	},
	.probe = bgm_probe,
	.remove = bgm_remove,
};

static int __init bgm_init(void)
{
	return platform_driver_register(&bgm_driver);
}

static void __exit bgm_exit(void)
{
	platform_driver_unregister(&bgm_driver);
}

module_init(bgm_init);
module_exit(bgm_exit);

MODULE_DESCRIPTION("BGM sensor control");
MODULE_AUTHOR("Samsung Electronic");
MODULE_LICENSE("GPL v2");
