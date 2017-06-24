/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <mach/pmic.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>

#include "../staging/android/timed_output.h"

/* default timeout */
#define VIB_DEFAULT_TIMEOUT 10000

struct max77826_vib {
	struct hrtimer vib_timer;
	struct timed_output_dev timed_dev;
	struct work_struct work;
	struct workqueue_struct *queue;

	struct regulator *vib_power;
	const char *power_name;
	int power_volt;
	int state;
	int timeout;
	struct mutex lock;
};

static void set_vibrator(struct regulator *reg_vib, int on)
{
	int ret = 0;

	pr_info("[VIB] %s, on=%d\n",__func__, on);

	if (on) {
		if (regulator_is_enabled(reg_vib))
			pr_err("%s: power regulator(3.0V) is enabled\n", __func__);
		else
			ret = regulator_enable(reg_vib);
		if (ret) {
			pr_err("%s: power regulator enable failed, rc=%d\n",
					__func__, ret);
			return;
		}
		pr_info("%s: max77826 VIB 3.0V ON\n", __func__);
	} else {
		if (regulator_is_enabled(reg_vib))
			ret = regulator_disable(reg_vib);
		else
			pr_err("%s: power regulator(3.0V) is disabled\n", __func__);
		if (ret) {
			pr_err("%s: disable power regulator failed, rc=%d\n",
					__func__, ret);
			return;
		}
		pr_info("%s: max77826 VIB 3.0V OFF\n", __func__);
	}
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	struct max77826_vib *vib = container_of(dev, struct max77826_vib,
					 timed_dev);

	mutex_lock(&vib->lock);
	hrtimer_cancel(&vib->vib_timer);

	if (value == 0) {
		pr_info("[VIB] OFF\n");
		vib->state = 0;
	}
	else {
		pr_info("[VIB] ON, Duration : %d msec\n" , value);
		vib->state = 1;

		if (value == 0x7fffffff){
			pr_info("[VIB] No Use Timer %d \n", value);
		}
		else	{
			value = (value > vib->timeout ?
					vib->timeout : value);
				hrtimer_start(&vib->vib_timer,
					ktime_set(value / 1000, (value % 1000) * 1000000),
					HRTIMER_MODE_REL);
                }
	}
	mutex_unlock(&vib->lock);
	queue_work(vib->queue, &vib->work);
}

static void max77826_vibrator_update(struct work_struct *work)
{
	struct max77826_vib *vib = container_of(work, struct max77826_vib,
					 work);

	set_vibrator(vib->vib_power, vib->state);
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	struct max77826_vib *vib = container_of(dev, struct max77826_vib,
							 timed_dev);

	if (hrtimer_active(&vib->vib_timer)) {
		ktime_t r = hrtimer_get_remaining(&vib->vib_timer);
		return (int)ktime_to_us(r);
	}
	else
		return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	struct max77826_vib *vib = container_of(timer, struct max77826_vib,
							 vib_timer);

	vib->state = 0;
	queue_work(vib->queue, &vib->work);

	return HRTIMER_NORESTART;
}

#ifdef CONFIG_PM
static int max77826_vibrator_suspend(struct device *dev)
{
	struct max77826_vib *vib = dev_get_drvdata(dev);

	pr_info("[VIB] %s\n",__func__);

	hrtimer_cancel(&vib->vib_timer);
	cancel_work_sync(&vib->work);
	/* turn-off vibrator */
	set_vibrator(vib->vib_power, 0);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(vibrator_pm_ops, max77826_vibrator_suspend, NULL);
static int max77826_vibrator_probe(struct platform_device *pdev)
{
	struct max77826_vib *vib;
	int rc = 0;

	pr_info("[VIB] %s\n",__func__);

	vib = devm_kzalloc(&pdev->dev, sizeof(*vib), GFP_KERNEL);
	if (!vib)	{
		pr_err("%s : Failed to allocate memory\n", __func__);
		return -ENOMEM;
	}

	rc = of_property_read_string(pdev->dev.of_node,
			"vibrator,ldo_name", &vib->power_name);
	if (rc < 0) {
		pr_err("[%s]: Unable to read ldo_name\n", __func__);
	}
	rc = of_property_read_u32(pdev->dev.of_node,
			"vibrator,ldo_volt", &vib->power_volt);
	if (rc < 0) {
		pr_err("[%s] Unable to read ldo_volt\n", __func__);
	}

	vib->vib_power = devm_regulator_get(&pdev->dev, vib->power_name);
	if (IS_ERR(vib->vib_power)) {
		pr_err("%s: could not get vib_power, rc=%ld\n",
				__func__, PTR_ERR(vib->vib_power));
		return PTR_ERR(vib->vib_power);
	} else {
		pr_info("%s: vib_power init.. \n", __func__);
	}

	pr_info("[%s] ldo_name[%s] ldo_volt[%d]\n", __func__,
			vib->power_name, vib->power_volt);

	regulator_set_voltage(vib->vib_power, vib->power_volt, vib->power_volt);
	vib->timeout = VIB_DEFAULT_TIMEOUT;

	INIT_WORK(&vib->work, max77826_vibrator_update);
	mutex_init(&vib->lock);

	vib->queue = create_singlethread_workqueue("max77826_vibrator");
	if (!vib->queue) {
		pr_err("%s(): can't create workqueue\n", __func__);
		return -ENOMEM;
	}

	hrtimer_init(&vib->vib_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vib->vib_timer.function = vibrator_timer_func;

	vib->timed_dev.name = "vibrator";
	vib->timed_dev.get_time = vibrator_get_time;
	vib->timed_dev.enable = vibrator_enable;

	dev_set_drvdata(&pdev->dev, vib);

	rc = timed_output_dev_register(&vib->timed_dev);
	if (rc < 0) {
		pr_err("[VIB] timed_output_dev_register fail (rc=%d)\n", rc);
		goto err_read_vib;
	}

	return 0;

err_read_vib:
	destroy_workqueue(vib->queue);
	return rc;
}

static int max77826_vibrator_remove(struct platform_device *pdev)
{
	struct max77826_vib *vib = dev_get_drvdata(&pdev->dev);

	destroy_workqueue(vib->queue);
	mutex_destroy(&vib->lock);

	return 0;
}

static const struct of_device_id vib_motor_match[] = {
	{.compatible = "max77826,vibrator"},
	{}
};

static struct platform_driver max77826_vibrator_drv =
{
	.driver = {
		.name = "max77826_vibrator",
		.owner = THIS_MODULE,
		.of_match_table = vib_motor_match,
		.pm	= &vibrator_pm_ops,
	},
	.probe = max77826_vibrator_probe,
	.remove = __devexit_p(max77826_vibrator_remove),
};

static int __init max77826_vibrator_init(void)
{
	pr_info("[VIB] %s\n",__func__);

	return platform_driver_register(&max77826_vibrator_drv);
}

void __exit max77826_vibrator_exit(void)
{
	platform_driver_unregister(&max77826_vibrator_drv);
}
module_init(max77826_vibrator_init);
module_exit(max77826_vibrator_exit);

MODULE_DESCRIPTION("timed output vibrator device");
MODULE_LICENSE("GPL v2");
