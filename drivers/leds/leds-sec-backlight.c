
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/slab.h>

#define LED_GPIO_BACKLIGHT_DRIVER_NAME	"sec,leds-gpio-bkl"
#define LED_TRIGGER_DEFAULT		"none"

struct led_brt_value {
	int os_value;			/* Platform setting values*/
	int ic_value;			/* Chip Setting values*/
};

struct led_gpio_bkl_data {
	int led_ctrl;
	int brightness;
	int max_brt_value, min_brt_value, dimming_value, max_brt_in_blu;
	int brt_table_len;
	int easyscale_addr;
	const char* control_type;
	struct led_brt_value *brt_table;
	struct led_classdev cdev;
#ifdef CONFIG_BACKLIGHT_KTD2801
	int express_wire;
#endif
};

static struct of_device_id led_gpio_bkl_of_match[] = {
	{.compatible = LED_GPIO_BACKLIGHT_DRIVER_NAME,},
	{},
};

static DEFINE_SPINLOCK(bl_ctrl_lock);

#if defined(CONFIG_GET_LCD_ATTACHED)
extern int get_lcd_attached(void);
#endif

#ifdef CONFIG_BACKLIGHT_KTD2801
#define MAX_BRIGHTNESS	255
#define DEFAULT_BRIGHTNESS 122
#define MAX_BRIGHTNESS_IN_BLU	32
#define KTD2801_DATA_BITS 8
#define KTD2801_T_OFF 5
#define KTD2801_T_EW_DET		300
#define KTD2801_T_EW_DELAY	200
#define KTD2801_T_EW_WIN	1
#define KTD2801_T_DS			5
#define KTD2801_T_EOD_H		400
#define KTD2801_T_EOD_L		30
#define KTD2801_T_H_LB		30
#define KTD2801_T_L_LB		70
#define KTD2801_T_H_HB		70
#define KTD2801_T_L_HB		30

#define MAX_BRIGHTNESS_IN_BLU 32

static int easy_scale_send_bit (unsigned gpio, bool bit)
{
	if (!bit) { /* Send bit 0 */
		gpio_direction_output(gpio, 0);
		udelay(KTD2801_T_L_LB);
		gpio_direction_output(gpio, 1);
		udelay(KTD2801_T_H_LB);
		return 0;
	} else { /* Send bit 1 */
		gpio_direction_output(gpio, 0);
		udelay(KTD2801_T_L_HB);
		gpio_direction_output(gpio, 1);
		udelay(KTD2801_T_H_HB);
		return 0;
	}
}

static void easy_scale_type_set_brightness(struct led_classdev *led_cdev,
				    enum led_brightness value)
{
	struct led_gpio_bkl_data *bkl_led =
	    container_of(led_cdev, struct led_gpio_bkl_data, cdev);
	int brightness_idx = 0;

	if (value > 255 || value < 0 ) {
		pr_info("BL : Error level = %d\n", value);
		return; /* Add Exception over 255 */
	}

	if (value > 0) {
		for (brightness_idx = 0; brightness_idx <  bkl_led->brt_table_len; brightness_idx++) {
			if (value <=  bkl_led->brt_table[brightness_idx].os_value)
				break;
		}
	} else {
		gpio_direction_output(bkl_led->led_ctrl, 0);
		mdelay(KTD2801_T_OFF);
		bkl_led->express_wire = 0;
		pr_info("KTD2801 BL : level = %d, index = %d\n", value, brightness_idx);
		bkl_led->brightness = brightness_idx;
		return;
	}

	pr_info("KTD2801 BL : level = %d, index = %d\n", value, brightness_idx);
	if (bkl_led->brightness != brightness_idx) {
		int i = 0;
		if(!bkl_led->express_wire) /* express wire dimming */
		{
			gpio_direction_output(bkl_led->led_ctrl, 0);
			mdelay(KTD2801_T_OFF);
			gpio_direction_output(bkl_led->led_ctrl, 1);
			udelay(KTD2801_T_EW_DELAY);
			gpio_direction_output(bkl_led->led_ctrl, 0);
			udelay(KTD2801_T_EW_DET);
			gpio_direction_output(bkl_led->led_ctrl, 1);
			mdelay(KTD2801_T_EW_WIN);
			bkl_led->express_wire = 1;
		}
		gpio_direction_output(bkl_led->led_ctrl, 1); /* Data start */
		udelay(KTD2801_T_DS); 

		for (i = 0; i < KTD2801_DATA_BITS ; i++) /* Send DATA */
			easy_scale_send_bit(bkl_led->led_ctrl, bkl_led->brt_table[brightness_idx].ic_value & (0x1 << (7-i)));

		gpio_direction_output(bkl_led->led_ctrl, 0); /* EOD */
		udelay(KTD2801_T_EOD_L);
		gpio_direction_output(bkl_led->led_ctrl, 1);
		udelay(KTD2801_T_EOD_H);
	}
	bkl_led->brightness = brightness_idx;
	return ;
}

#else
#define MAX_BRIGHTNESS	255
#define DEFAULT_BRIGHTNESS 122
#define MAX_BRIGHTNESS_IN_BLU	32
#define TPS61158_DATA_BITS 8
#define TPS61158_ADDRESS_BITS 8
#define TPS61158_TIME_T_EOS 300
#define TPS61158_TIME_ES_DET_DELAY 150
#define TPS61158_TIME_ES_DET_TIME 500
#define TPS61158_TIME_T_START 10
#define TPS61158_TIME_DATA_DELAY_SHORT 180
#define TPS61158_TIME_DATA_DELAY_LONG 400

#define MAX_BRIGHTNESS_IN_BLU 32

static int easy_scale_send_bit (unsigned gpio, bool bit)
{
	if (bit) { /* Send bit 1 */
		gpio_direction_output(gpio, 0);
		udelay(TPS61158_TIME_DATA_DELAY_SHORT);
		gpio_direction_output(gpio, 1);
		udelay(TPS61158_TIME_DATA_DELAY_LONG);
		return 0;
	} else { /* Send bit 0 */
		gpio_direction_output(gpio, 0);
		udelay(TPS61158_TIME_DATA_DELAY_LONG);
		gpio_direction_output(gpio, 1);
		udelay(TPS61158_TIME_DATA_DELAY_SHORT);
		return 0;
	}
}

static void easy_scale_type_set_brightness(struct led_classdev *led_cdev,
				    enum led_brightness value)
{
	struct led_gpio_bkl_data *bkl_led =
	    container_of(led_cdev, struct led_gpio_bkl_data, cdev);
	int brightness_idx = 0;

	if (value > 255 || value < 0 ) {
		pr_info("BL : Error level = %d\n", value);
		return; /* Add Exception over 255 */
	}

	if (value > 0) {
		for (brightness_idx = 0; brightness_idx <  bkl_led->brt_table_len; brightness_idx++) {
			if (value <=  bkl_led->brt_table[brightness_idx].os_value)
				break;
		}
	} else {
		gpio_direction_output(bkl_led->led_ctrl, 0);
		mdelay(3);
		pr_info("BL : level = %d, index = %d\n", value, brightness_idx);
		bkl_led->brightness = brightness_idx;
		return;
	}

	pr_info("BL : level = %d, index = %d\n", value, brightness_idx);
	if (bkl_led->brightness != brightness_idx) {
		int i = 0;
		gpio_direction_output(bkl_led->led_ctrl, 0);
		udelay(TPS61158_TIME_T_EOS);
		gpio_direction_output(bkl_led->led_ctrl, 1);
		udelay(TPS61158_TIME_ES_DET_DELAY);
		gpio_direction_output(bkl_led->led_ctrl, 0);
		udelay(TPS61158_TIME_ES_DET_TIME);
		gpio_direction_output(bkl_led->led_ctrl, 1);
		mdelay(3);
		udelay(500); /* ES window time */

		for (i = 0; i < TPS61158_DATA_BITS ; i++) /* Send DATA */
			easy_scale_send_bit(bkl_led->led_ctrl, bkl_led->easyscale_addr & (0x1 << i));

		gpio_direction_output(bkl_led->led_ctrl, 0); /* 2nd FLAG */
		udelay(TPS61158_TIME_T_EOS);
		gpio_direction_output(bkl_led->led_ctrl, 1);
		udelay(TPS61158_TIME_T_START);

		for (i = 0; i < TPS61158_DATA_BITS ; i++) /* Send DATA */
			easy_scale_send_bit(bkl_led->led_ctrl, bkl_led->brt_table[brightness_idx].ic_value & (0x1 << i));

		gpio_direction_output(bkl_led->led_ctrl, 0); /* 3rd FLAG */
		udelay(TPS61158_TIME_T_EOS);
		gpio_direction_output(bkl_led->led_ctrl, 1); /* BL brightness enable */
	}
	bkl_led->brightness = brightness_idx;
	return ;
}
#endif

static void gpio_swing_type_set_brightness(struct led_classdev *led_cdev,
				    enum led_brightness value)
{
	struct led_gpio_bkl_data *bkl_led =
	    container_of(led_cdev, struct led_gpio_bkl_data, cdev);
	int brightness_idx = 0;

#if defined(CONFIG_GET_LCD_ATTACHED)
	if(get_lcd_attached() == 0)
	{
		pr_info("%s: get_lcd_attached(0)!  Backlight IC off\n",__func__);
		gpio_set_value(bkl_led->led_ctrl, 0);
		mdelay(50);
		return;
	}
#endif
	if (value > 255 || value < 0 ) {
		pr_info("BL : Error level = %d\n", value);
		return; /* Add Exception over 255 */
	}

	if (value > 0) {
		if(value < bkl_led->min_brt_value) {
			brightness_idx = bkl_led->dimming_value;
		} else if(value >= bkl_led->max_brt_value) {
			brightness_idx = bkl_led->brt_table[bkl_led->brt_table_len-1].ic_value;
		} else {
			int i;
			
			for (i = 0; i < bkl_led->brt_table_len; i++) {
				if (value <= bkl_led->brt_table[i].os_value) {
					brightness_idx = bkl_led->brt_table[i].ic_value;
					break;
				}
			}
		}
	} else {
		gpio_set_value(bkl_led->led_ctrl, 0);
		mdelay(3);
		pr_info("BL : level = %d, index = %d\n", value, brightness_idx);
		bkl_led->brightness = brightness_idx;
		return;
	}

	pr_info("BL : level = %d, index = %d\n", value, brightness_idx);

	if(bkl_led->brightness != brightness_idx)
	{
		if (!brightness_idx) {
			gpio_set_value(bkl_led->led_ctrl, 0);
			mdelay(50);
		} else {
			int pulse;

			if (unlikely(bkl_led->brightness < 0)) {
				int val = gpio_get_value(bkl_led->led_ctrl);
				if (val) {
					bkl_led->brightness = 0;
					gpio_set_value(bkl_led->led_ctrl, 0);
					mdelay(50);
					pr_info("BL : LCD Baklight init in boot time on kernel\n");
				}
			}
			if (!bkl_led->brightness) {
				gpio_set_value(bkl_led->led_ctrl, 1);
				udelay(100);
				bkl_led->brightness = bkl_led->max_brt_in_blu;
			}

			pulse = (brightness_idx - bkl_led->brightness + bkl_led->max_brt_in_blu) % bkl_led->max_brt_in_blu;

			pr_debug("BL : ctrl_pls = %d, cur_pls = %d, bef_pls = %d\n", pulse, brightness_idx, bkl_led->brightness);

			for (; pulse > 0; pulse--) {
				gpio_set_value(bkl_led->led_ctrl, 0);
				udelay(2);
				gpio_set_value(bkl_led->led_ctrl, 1);
				udelay(2);
			}
		}
		bkl_led->brightness  = brightness_idx;
	}
	
	mdelay(1);
}


static void led_gpio_brightness_set(struct led_classdev *led_cdev,
				    enum led_brightness value)
{
	struct led_gpio_bkl_data *bkl_led =
		container_of(led_cdev, struct led_gpio_bkl_data, cdev);

	spin_lock(&bl_ctrl_lock);
	if(!strcmp("easy_scale", bkl_led->control_type)) {
		easy_scale_type_set_brightness(led_cdev, value);
	} else if(!strcmp("gpio_swing", bkl_led->control_type)) {
		gpio_swing_type_set_brightness(led_cdev, value);
	} else {
		dev_err(bkl_led->cdev.dev, "%s:%d control type is not supported\n",
			__func__, __LINE__);
	}
	spin_unlock(&bl_ctrl_lock);
}

static enum led_brightness led_gpio_brightness_get(struct led_classdev
						   *led_cdev)
{
	struct led_gpio_bkl_data *bkl_led =
		container_of(led_cdev, struct led_gpio_bkl_data, cdev);
	return bkl_led->brightness;
}

int __devinit led_gpio_bkl_probe(struct platform_device *pdev)
{
	int rc = 0, i;
	u32 tmp;
	const char *temp_str;
	struct led_gpio_bkl_data *bkl_led = NULL;
	struct device_node *node = pdev->dev.of_node;

	bkl_led = devm_kzalloc(&pdev->dev, sizeof(struct led_gpio_bkl_data),
				 GFP_KERNEL);
	if (bkl_led == NULL) {
		dev_err(&pdev->dev, "%s:%d Unable to allocate memory\n",
			__func__, __LINE__);
		return -ENOMEM;
	}

	bkl_led->cdev.default_trigger = LED_TRIGGER_DEFAULT;
	rc = of_property_read_string(node, "linux,default-trigger", &temp_str);
	if (!rc)
		bkl_led->cdev.default_trigger = temp_str;

	bkl_led->led_ctrl = of_get_named_gpio(node, "sec,led-ctrl", 0);

	if (bkl_led->led_ctrl < 0) {
		dev_err(&pdev->dev,
			"Looking up %s property in node %s failed. rc =  %d\n",
			"flash-en", node->full_name, bkl_led->led_ctrl);
		goto error;
	} else {
		rc = gpio_request(bkl_led->led_ctrl, "led_ctrl");
		if (rc) {
			dev_err(&pdev->dev,
				"%s: Failed to request gpio %d,rc = %d\n",
				__func__, bkl_led->led_ctrl, rc);

			goto error;
		}
	}

	bkl_led->control_type = of_get_property(node,
				"sec,control-type", NULL);

	rc = of_property_read_u32(node,
		"sec,brightness-table-len", &tmp);
	bkl_led->brt_table_len =  (!rc ? tmp : false);
	bkl_led->brightness = -1;

	bkl_led->brt_table = (struct led_brt_value *)kzalloc(sizeof(struct led_brt_value) * bkl_led->brt_table_len, GFP_KERNEL);
	rc = of_property_read_u32_array(node, "sec,brightness-table", (u32 *)bkl_led->brt_table, bkl_led->brt_table_len*2);

	for (i = 0 ; i < bkl_led->brt_table_len; i++ ) {
		pr_debug("BL : [%d] %d %d\n",i, bkl_led->brt_table[i].os_value, bkl_led->brt_table[i].ic_value);
	}

	rc = of_property_read_u32(node,
		"sec,max-brightness-value", &tmp);
	bkl_led->max_brt_value = (!rc ? tmp : false);
	rc = of_property_read_u32(node,
		"sec,min-brightness-value", &tmp);
	bkl_led->min_brt_value = (!rc ? tmp : false);
	rc = of_property_read_u32(node,
		"sec,dimming-value", &tmp);
	bkl_led->dimming_value = (!rc ? tmp : false);
	rc = of_property_read_u32(node,
		"sec,max-brightness-in-blu-value", &tmp);
	bkl_led->max_brt_in_blu = (!rc ? tmp : false);
	rc = of_property_read_u32(node,
		"sec,easyscale-address", &tmp);
	bkl_led->easyscale_addr = (!rc ? tmp : false);

	gpio_tlmm_config(GPIO_CFG(bkl_led->led_ctrl, 0,
				  GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				  GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	platform_set_drvdata(pdev, bkl_led);
	bkl_led->cdev.max_brightness = LED_FULL;
	bkl_led->cdev.brightness_set = led_gpio_brightness_set;
	bkl_led->cdev.brightness_get = led_gpio_brightness_get;

#ifdef CONFIG_BACKLIGHT_KTD2801
	bkl_led->express_wire = 0;
#endif
	rc = led_classdev_register(&pdev->dev, &bkl_led->cdev);
	if (rc) {
		dev_err(&pdev->dev, "%s: Failed to register led dev. rc = %d\n",
			__func__, rc);
		goto error;
	}
	return 0;

error:
	devm_kfree(&pdev->dev, bkl_led);
	return rc;
}

int __devexit led_gpio_bkl_remove(struct platform_device *pdev)
{
	struct led_gpio_bkl_data *bkl_led =
		(struct led_gpio_bkl_data *)platform_get_drvdata(pdev);

	led_classdev_unregister(&bkl_led->cdev);
	devm_kfree(&pdev->dev, bkl_led);
	return 0;
}

static struct platform_driver led_gpio_bkl_driver = {
	.probe = led_gpio_bkl_probe,
	.remove = __devexit_p(led_gpio_bkl_remove),
	.driver = {
		   .name = LED_GPIO_BACKLIGHT_DRIVER_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = led_gpio_bkl_of_match,
		   }
};

static int __init led_gpio_bkl_init(void)
{
	return platform_driver_register(&led_gpio_bkl_driver);
}

static void __exit led_gpio_bkl_exit(void)
{
	return platform_driver_unregister(&led_gpio_bkl_driver);
}

late_initcall(led_gpio_bkl_init);
module_exit(led_gpio_bkl_exit);

MODULE_DESCRIPTION("QCOM GPIO LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("leds:leds-msm-gpio-flash");

