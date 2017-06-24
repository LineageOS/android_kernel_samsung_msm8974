/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) "%s:%d " fmt, __func__, __LINE__

#include <linux/module.h>
// Implementation KTD2692 flashIC
#if defined(CONFIG_MACH_VIENNA_LTE)
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#endif

#if defined(CONFIG_LEDS_MAX77804K)
#include <linux/leds-max77804k.h>
#include <linux/gpio.h>
#endif
#if defined(CONFIG_LEDS_MAX77828)
#include <linux/leds-max77828.h>
#endif
#include "msm_led_flash.h"

#define FLASH_NAME "camera-led-flash"

/*#define CONFIG_MSMB_CAMERA_DEBUG*/
#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

static struct msm_led_flash_ctrl_t fctrl;

#if defined(CONFIG_LEDS_MAX77804K)
extern int led_flash_en;
extern int led_torch_en;
#endif

// Implementation KTD2692 flashIC
#if defined(CONFIG_MACH_VIENNA_LTE)
extern unsigned int system_rev;
extern int led_flash_en;
extern int led_torch_en;
bool is_torch_enabled;

extern struct class *camera_class; /*sys/class/camera*/
struct device *flash_dev;

/* KTD2692 : command time delay(us) */
#define T_DS		15	//	12
#define T_EOD_H		1000 //	350
#define T_EOD_L		10
#define T_H_LB		10
#define T_L_LB		3*T_H_LB
#define T_L_HB		10
#define T_H_HB		7*T_L_HB
#define T_RESET		800	//	700
#define T_RESET2	1000
/* KTD2692 : command address(A2:A0) */
#define LVP_SETTING		0x0 << 5
#define FLASH_TIMEOUT	0x1 << 5
#define MIN_CURRENT		0x2 << 5
#define MOVIE_CURRENT	0x3 << 5
#define FLASH_CURRENT	0x4 << 5
#define MODE_CONTROL	0x5 << 5

static DEFINE_SPINLOCK(flash_ctrl_lock);
void KTD2692_set_flash(unsigned int ctl_cmd)
{
	int i = 0;
	int j = 0;
	int k = 0;
	unsigned long flags;
	unsigned int ctl_cmd_buf;

	spin_lock_irqsave(&flash_ctrl_lock, flags);
	if ( MODE_CONTROL == (MODE_CONTROL & ctl_cmd) )
		k = 8;
	else
		k = 1;
	for (j = 0; j < k; j++) {
		CDBG("[cmd::0x%2X][MODE_CONTROL&cmd::0x%2X][k::%d]\n", ctl_cmd, (MODE_CONTROL & ctl_cmd), k);
		gpio_set_value(led_torch_en, 1);
		udelay(T_DS);

		ctl_cmd_buf = ctl_cmd;
		for (i = 0; i < 8; i++) {
			if (ctl_cmd_buf & 0x80) { /* set bit to 1 */
				gpio_set_value(led_torch_en, 0);
				gpio_set_value(led_torch_en, 1);
				udelay(T_H_HB);
			} else { /* set bit to 0 */
				gpio_set_value(led_torch_en, 0);
				udelay(T_L_LB);
				gpio_set_value(led_torch_en, 1);
			}
			ctl_cmd_buf = ctl_cmd_buf << 1;
		}

		gpio_set_value(led_torch_en, 0);
		udelay(T_EOD_L);
		gpio_set_value(led_torch_en, 1);
		udelay(T_EOD_H);
	}
	spin_unlock_irqrestore(&flash_ctrl_lock, flags);
}

static ssize_t ktd2692_flash(struct device *dev,
			     struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (isspace(*after))
		count++;
	if (count == size) {
		ret = count;
#if defined(CONFIG_MACH_LT03EUR) || defined(CONFIG_MACH_LT03SKT) \
		|| defined(CONFIG_MACH_LT03KTT) || defined(CONFIG_MACH_LT03LGT)
		if (state == 0) {
			KTD2692_set_flash(MODE_CONTROL | 0x00);
			gpio_set_value(led_torch_en, 0);
			is_torch_enabled = false;
		} else {
			KTD2692_set_flash(LVP_SETTING | 0x00);
			KTD2692_set_flash(MODE_CONTROL | 0x01); /* Movie mode */
			is_torch_enabled = true;
		}
#else
		if (state == 0) {
			CDBG("%s:%d rear_flash off\n", __func__, __LINE__);
			if (system_rev < 0x02) { // For KTD267 flashIC
				gpio_set_value(led_flash_en, 0);
				gpio_set_value(led_torch_en, 0);
			} else { // For KTD2692 flashIC
				KTD2692_set_flash(MODE_CONTROL | 0x00);
				gpio_set_value(led_torch_en, 0);
				is_torch_enabled = false;;
			}

		} else {
			CDBG("%s:%d rear_flash on[%ld]\n", __func__, __LINE__, state);
			if (system_rev < 0x02) { // For KTD267 flashIC
				gpio_set_value(led_flash_en, 1);
				gpio_set_value(led_torch_en, 1);
			} else {                                        // For KTD2692 flashIC
				KTD2692_set_flash(LVP_SETTING | 0x00);
				KTD2692_set_flash(MODE_CONTROL | 0x01); /* Movie mode */
				is_torch_enabled = true;
			}
		}
#endif
	}

	return ret;
}

static DEVICE_ATTR(rear_flash, S_IWUSR | S_IWGRP | S_IROTH,
		   NULL, ktd2692_flash);

#endif

static int32_t msm_led_trigger_get_subdev_id(struct msm_led_flash_ctrl_t *fctrl,
					     void *arg)
{
	uint32_t *subdev_id = (uint32_t*)arg;

	if (!subdev_id) {
		pr_err("%s:%d failed\n", __func__, __LINE__);
		return -EINVAL;
	}
	*subdev_id = fctrl->pdev->id;
	CDBG("%s:%d subdev_id %d\n", __func__, __LINE__, *subdev_id);
	return 0;
}

static int32_t msm_led_trigger_config(struct msm_led_flash_ctrl_t *fctrl,
				      void *data)
{
	int rc = 0;

#if defined(CONFIG_LEDS_MAX77804K)
	int ret;
#endif
	struct msm_camera_led_cfg_t *cfg = (struct msm_camera_led_cfg_t *)data;
	CDBG("called led_state %d\n", cfg->cfgtype);
#if defined(CONFIG_MACH_VIENNA_LTE)
	if (is_torch_enabled == true) {
		return rc;
	}
#endif
	if (!fctrl) {
		pr_err("failed\n");
		return -EINVAL;
	}
#if defined(CONFIG_LEDS_MAX77804K)
	switch (cfg->cfgtype) {
	case MSM_CAMERA_LED_OFF:
		pr_err("CAM Flash OFF");
		max77804k_led_en(0, 0);
		max77804k_led_en(0, 1);
		break;

	case MSM_CAMERA_LED_LOW:
		pr_err("CAM Pre Flash ON");
		max77804k_led_en(1, 0);
		break;

	case MSM_CAMERA_LED_HIGH:
		pr_err("CAM Flash ON");
		max77804k_led_en(1, 1);
		break;

	case MSM_CAMERA_LED_INIT:
		break;
	case MSM_CAMERA_LED_RELEASE:
		pr_err("CAM Flash OFF & release");
		ret = gpio_request(led_flash_en, "max77804k_flash_en");
		if (ret)
			pr_err("can't get max77804k_flash_en");
		else {
			gpio_direction_output(led_flash_en, 0);
			gpio_free(led_flash_en);
		}
		ret = gpio_request(led_torch_en, "max77804k_torch_en");
		if (ret)
			pr_err("can't get max77804k_torch_en");
		else {
			gpio_direction_output(led_torch_en, 0);
			gpio_free(led_torch_en);
		}
		break;

	default:
		rc = -EFAULT;
		break;
	}
#elif defined(CONFIG_LEDS_MAX77828)
	switch (cfg->cfgtype) {
	case MSM_CAMERA_LED_OFF:
		pr_err("CAM Flash OFF");
		max77828_led_en(0, 0);
		max77828_led_en(0, 1);
		break;

	case MSM_CAMERA_LED_LOW:
		pr_err("CAM Pre Flash ON");
		max77828_led_en(1, 0);
		break;

	case MSM_CAMERA_LED_HIGH:
		pr_err("CAM Flash ON");
		max77828_led_en(1, 1);
		break;

	case MSM_CAMERA_LED_INIT:
	case MSM_CAMERA_LED_RELEASE:
		break;

	default:
		rc = -EFAULT;
		break;
	}
// Implementation KTD2692 flashIC
#elif defined(CONFIG_MACH_VIENNA_LTE)
	switch (cfg->cfgtype) {
#if defined(CONFIG_MACH_LT03EUR) || defined(CONFIG_MACH_LT03SKT) \
		|| defined(CONFIG_MACH_LT03KTT) || defined(CONFIG_MACH_LT03LGT)
	case MSM_CAMERA_LED_OFF:
		KTD2692_set_flash(MODE_CONTROL | 0x00);
		break;
	case MSM_CAMERA_LED_LOW:
		gpio_set_value(led_torch_en, 0);
		udelay(T_RESET);
		gpio_set_value(led_torch_en, 1);
		udelay(T_RESET2);
		KTD2692_set_flash(LVP_SETTING | 0x00);
		KTD2692_set_flash(MOVIE_CURRENT | 0x04);
		KTD2692_set_flash(MODE_CONTROL | 0x01);
		break;
	case MSM_CAMERA_LED_HIGH:
		gpio_set_value(led_torch_en, 0);
		udelay(T_RESET);
		gpio_set_value(led_torch_en, 1);
		udelay(T_RESET2);
		KTD2692_set_flash(LVP_SETTING | 0x00);
		KTD2692_set_flash(FLASH_CURRENT | 0x0F);
		KTD2692_set_flash(MODE_CONTROL | 0x02);
		break;
	case MSM_CAMERA_LED_INIT:
		break;
	case MSM_CAMERA_LED_RELEASE:
		gpio_set_value(led_torch_en, 0);
		break;
	default:
		rc = -EFAULT;
		break;
#else
	case MSM_CAMERA_LED_OFF:
		if (system_rev < 0x02) { // For KTD267 flashIC
			gpio_set_value(led_flash_en, 0);
			gpio_set_value(led_torch_en, 0);
		} else { // For KTD2692 flashIC
			KTD2692_set_flash(MODE_CONTROL | 0x00);
		}
		break;

	case MSM_CAMERA_LED_LOW:
		if (system_rev < 0x02) { // For KTD267 flashIC
			gpio_set_value(led_flash_en, 1);
			gpio_set_value(led_torch_en, 1);
		} else { // For KTD2692 flashIC
			KTD2692_set_flash(LVP_SETTING | 0x00);
			KTD2692_set_flash(MOVIE_CURRENT | 0x04);
			KTD2692_set_flash(MODE_CONTROL | 0x01);
		}
		break;

	case MSM_CAMERA_LED_HIGH:
		if (system_rev < 0x02) { // For KTD267 flashIC
			gpio_set_value(led_flash_en, 1);
			gpio_set_value(led_torch_en, 1);
		} else { // For KTD2692 flashIC
			KTD2692_set_flash(LVP_SETTING | 0x00);
			KTD2692_set_flash(FLASH_CURRENT | 0x0F);
			KTD2692_set_flash(MODE_CONTROL | 0x02);
		}
		break;

	case MSM_CAMERA_LED_INIT:
		break;

	case MSM_CAMERA_LED_RELEASE:
		if (system_rev < 0x02) {        // For KTD267 flashIC
		} else {                        // For KTD2692 flashIC
			gpio_set_value(led_torch_en, 0);
		}
		break;

	default:
		rc = -EFAULT;
		break;
#endif
	}
#else
	switch (cfg->cfgtype) {
	case MSM_CAMERA_LED_OFF:
		led_trigger_event(fctrl->led_trigger[0], 0);
		break;

	case MSM_CAMERA_LED_LOW:
		led_trigger_event(fctrl->led_trigger[0],
				  fctrl->max_current[0] / 2);
		break;

	case MSM_CAMERA_LED_HIGH:
		led_trigger_event(fctrl->led_trigger[0], fctrl->max_current[0]);
		break;

	case MSM_CAMERA_LED_INIT:
	case MSM_CAMERA_LED_RELEASE:
		led_trigger_event(fctrl->led_trigger[0], 0);
		break;

	default:
		rc = -EFAULT;
		break;
	}
#endif
	CDBG("flash_set_led_state: return %d\n", rc);
	return rc;
}

static const struct of_device_id msm_led_trigger_dt_match[] = {
	{ .compatible = "qcom,camera-led-flash" },
	{}
};

MODULE_DEVICE_TABLE(of, msm_led_trigger_dt_match);

static struct platform_driver msm_led_trigger_driver = {
	.driver			= {
		.name		= FLASH_NAME,
		.owner		= THIS_MODULE,
		.of_match_table = msm_led_trigger_dt_match,
	},
};

static int32_t msm_led_trigger_probe(struct platform_device *pdev)
{
	int32_t rc = 0, i = 0;
	struct device_node *of_node = pdev->dev.of_node;
	struct device_node *flash_src_node = NULL;
	uint32_t count = 0;

	CDBG("called\n");

	if (!of_node) {
		pr_err("of_node NULL\n");
		return -EINVAL;
	}

	fctrl.pdev = pdev;

	rc = of_property_read_u32(of_node, "cell-index", &pdev->id);
	if (rc < 0) {
		pr_err("failed\n");
		return -EINVAL;
	}
	CDBG("pdev id %d\n", pdev->id);

	if (of_get_property(of_node, "qcom,flash-source", &count)) {
		count /= sizeof(uint32_t);
		CDBG("count %d\n", count);
		if (count > MAX_LED_TRIGGERS) {
			pr_err("failed\n");
			return -EINVAL;
		}
		for (i = 0; i < count; i++) {
			flash_src_node = of_parse_phandle(of_node,
							  "qcom,flash-source", i);
			if (!flash_src_node) {
				pr_err("flash_src_node NULL\n");
				continue;
			}

			rc = of_property_read_string(flash_src_node,
						     "linux,default-trigger",
						     &fctrl.led_trigger_name[i]);
			if (rc < 0) {
				pr_err("failed\n");
				of_node_put(flash_src_node);
				continue;
			}

			CDBG("default trigger %s\n", fctrl.led_trigger_name[i]);

			rc = of_property_read_u32(flash_src_node,
						  "qcom,max-current", &fctrl.max_current[i]);
			if (rc < 0) {
				pr_err("failed rc %d\n", rc);
				of_node_put(flash_src_node);
				continue;
			}

			of_node_put(flash_src_node);

			CDBG("max_current[%d] %d\n", i, fctrl.max_current[i]);

			led_trigger_register_simple(fctrl.led_trigger_name[i],
						    &fctrl.led_trigger[i]);
		}
	}
	rc = msm_led_flash_create_v4lsubdev(pdev, &fctrl);
// Implementation KTD2692 flashIC
#if defined(CONFIG_MACH_VIENNA_LTE)
	if (!IS_ERR(camera_class)) {
		flash_dev = device_create(camera_class, NULL, 0, NULL, "flash");
		if (flash_dev < 0)
			pr_err("Failed to create device(flash)!\n");

		if (device_create_file(flash_dev, &dev_attr_rear_flash) < 0) {
			pr_err("failed to create device file, %s\n",
			       dev_attr_rear_flash.attr.name);
		}

	} else
		pr_err("Failed to create device(flash) because of nothing camera class!\n");
#endif
	return rc;
}

static int __init msm_led_trigger_add_driver(void)
{
	CDBG("called\n");
	return platform_driver_probe(&msm_led_trigger_driver,
				     msm_led_trigger_probe);
}

static struct msm_flash_fn_t msm_led_trigger_func_tbl = {
	.flash_get_subdev_id	= msm_led_trigger_get_subdev_id,
	.flash_led_config	= msm_led_trigger_config,
};

static struct msm_led_flash_ctrl_t fctrl = {
	.func_tbl	= &msm_led_trigger_func_tbl,
};

module_init(msm_led_trigger_add_driver);
MODULE_DESCRIPTION("LED TRIGGER FLASH");
MODULE_LICENSE("GPL v2");
