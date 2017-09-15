/*
 * LED driver for Maxim MAX77828 - leds-MAX77828.c
 *
 * Copyright (C) 2013 Samsung co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/mfd/max77828.h>
#include <linux/mfd/max77828-private.h>
#include <linux/leds-max77828.h>
#include <linux/ctype.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>

#ifdef CONFIG_LEDS_SWITCH
#include <linux/gpio.h>
#define FLASH_SWITCH_REMOVED_REVISION	0x05
#endif

#undef DEBUG_FLASH
#define DEBUG_FLASH
#if defined(DEBUG_FLASH)
#define DEBUG_MAX77828(fmt, args...) pr_err(fmt, ##args)
#else
#define DEBUG_MAX77828(fmt, args...) do{}while(0)
#endif

struct max77828_led_data {
	struct led_classdev led;
	struct max77828_dev *max77828;
	struct max77828_led *data;
	struct i2c_client *i2c;
	struct work_struct work;
	struct mutex lock;
	spinlock_t value_lock;
	int brightness;
	int test_brightness;
#ifdef CONFIG_LEDS_SEPERATE_MOVIE_FLASH
	int movie_brightness;
#endif 
};


struct max77828_flash_platform_data
{
	bool flash_en_control;
	bool torch_stb_control;
	int flash_ramp_up;		/* One of 384, 640, 1152, 2176, 4224, 8320, 16512, 32896 usec */
	int flash_ramp_down;		/* One of 384, 640, 1152, 2176, 4224, 8320, 16512, 32896 usec */
	int torch_ramp_up;		/* One of 16392, 32776, 65544, 131080, 262152, 524296, 1048000, 2097000 usec */
	int torch_ramp_down;		/* One of 16392, 32776, 65544, 131080, 262152, 524296, 1048000, 2097000 usec */
	bool enable_pulldown_resistor;	/* On/Off Control for Pulldown Resistor */
	bool enable_maxflash;		/* enable MAXFLASH */
	int maxflash_threshold;		/* 2400000uV to 3400000uV in 33333uV steps */
	int maxflash_hysteresis;	/* One of 50000uV, 100000uV, 300000uV, 350000uV or -1. -1 means only allowed to decrease */
	int maxflash_falling_timer;	/* 256us to 4096us in 256us steps */
	int maxflash_rising_timer;	/* 256us to 4096us in 256us steps */
};

#define SEC_LED_SPECIFIC

#ifdef SEC_LED_SPECIFIC
struct device *led_dev;
#endif

struct max77828_flash_platform_data *fdata;

extern struct class *camera_class; /*sys/class/camera*/
struct device *flash_dev;
static int flash_torch_en;

extern int led_torch_en;
extern int led_flash_en;

const int flash_ramp[] = {384, 640, 1152, 2176, 4224, 8320, 16512, 32896};
const int torch_ramp[] = {16392, 32776, 65544, 131080, 262152, 524296, 1048000, 2097000};

#ifdef CONFIG_LEDS_SEPERATE_MOVIE_FLASH
static ssize_t max77828_show_movie_brightness(struct device *dev,
				     struct device_attribute *devattr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct max77828_led_data *led_data = container_of(led_cdev, struct max77828_led_data, led);

	return sprintf(buf, "%d\n", led_data->movie_brightness);
}

static ssize_t max77828_store_movie_brightness(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	return size;
}

static DEVICE_ATTR(movie_brightness, S_IWUSR|S_IWGRP|S_IRUGO,
	max77828_show_movie_brightness, max77828_store_movie_brightness);

#endif

#if defined(DEBUG_FLASH)
static void print_all_reg_value(struct i2c_client *client)
{
	u8 value;
	u8 i;

	for (i = 2; i != 0x36; ++i) {
		max77828_read_reg(client, i, &value);
		printk(KERN_ERR "LEDS_MAX77828 REG(%x) = %x\n", i, value);
		if(i==0x12)
			i=0x2F;
		value = 0;
	}
}

static int max77828_debugfs_show(struct seq_file *s, void *data)
{
        struct max77828_dev *ldata = s->private;
        u8 reg;
        u8 reg_data;

        seq_printf(s, "MAX77828 IF PMIC :\n");
        seq_printf(s, "=============\n");
        for (reg = 0x02; reg <= 0x36; reg++) {
                max77828_read_reg(ldata->led, reg, &reg_data);
                seq_printf(s, "0x%02x:\t0x%02x\n", reg, reg_data);
		if(reg==0x12)
				reg=0x2F;
                        }
	print_all_reg_value(ldata->led);
        seq_printf(s, "\n");
        return 0;
                    }

static int max77828_debugfs_open(struct inode *inode, struct file *file)
{
        return single_open(file, max77828_debugfs_show, inode->i_private);
        }

static const struct file_operations max77828_debugfs_fops = {
        .open           = max77828_debugfs_open,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = single_release,
};

#endif

int max77828_led_en(int onoff, int mode)
{
	int ret = 0;
	if (flash_torch_en) {
	    if (onoff) { 	/* enable */
			if (mode) { 	/* flash */
				DEBUG_MAX77828("[LED] %s: max77828_flash_en set 1\n", __func__);
				gpio_direction_output(led_flash_en, 1);
			}
			else { 	/* torch */
				DEBUG_MAX77828("[LED] %s: max77828_torch_en set 1\n", __func__);
				gpio_direction_output(led_torch_en, 1);
			}
	    }
	    else { 		/* disable */
			if (mode) { 	/* flash */
				DEBUG_MAX77828("[LED] %s: max77828_flash_en set 0\n", __func__);
				gpio_direction_output(led_flash_en, 0);
			}
			else { 	/* torch */
				DEBUG_MAX77828("[LED] %s: max77828_torch_en set 0\n", __func__);
				gpio_direction_output(led_torch_en, 0);
			}
	    }
	}
	else {
	    pr_err("%s : Error!!, find gpio", __func__);
	    ret = -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL(max77828_led_en);

static void max77828_led_set(struct led_classdev *led_cdev,
					enum led_brightness value)
{

	unsigned long flags;
	struct max77828_led_data *led_data = container_of(led_cdev, struct max77828_led_data, led);

	DEBUG_MAX77828("[LED] %s - brightness value written is %d\n", __func__,value);

	spin_lock_irqsave(&led_data->value_lock, flags);
	led_data->test_brightness = value;
	spin_unlock_irqrestore(&led_data->value_lock, flags);

	schedule_work(&led_data->work);

}

static void max77828_flash_set(struct max77828_led_data *led_data)
{
	int ret;
	int brightness = led_data->test_brightness;

	if(brightness == LED_OFF)
	{
		DEBUG_MAX77828("(%s) LED_OFF\n",__func__);
		/* mode select */
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_MODE_SEL, 0xCA);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  MAX77828_LED_REG_MODE_SEL update failed\n",__func__);

		/* Flash OFF */
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_I_FLASH1,
						MAX77828_FLASH_FLED1_EN);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  MAX77828_LED_REG_I_FLASH1 update failed\n",__func__ );
	}
	else
	{
		if(brightness > 0x3F)
			brightness = 0x3F;
		DEBUG_MAX77828("(%s) LED_ON\n",__func__);
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_DCDC_CNTL1,
				MAX77828_DCDC_MODE_PAM|0xC0);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  MAX77828_LED_REG_DCDC_CNTL1 write failed\n",__func__);
		/* mode select */
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_MODE_SEL,0x47);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  MAX77828_LED_REG_MODE_SEL update failed\n",__func__);

		/* Flash ON, Set Current */
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_I_FLASH1,
				MAX77828_FLASH_FLED1_EN |
				((unsigned int)brightness << max77828_set_bit(MAX77828_FLASH_IOUT)));
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  MAX77828_LED_REG_I_FLASH1 write failed\n",__func__);
	}
	return;
}

static void max77828_torch_set(struct max77828_led_data *led_data)
{
	int ret;
	int brightness = led_data->test_brightness;

	if(brightness == LED_OFF)
	{
		/* mode select */
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_MODE_SEL, 0xCA);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  MAX77828_LED_REG_MODE_SEL update failed\n",__func__);

		/* Torch OFF */
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_I_TORCH1, 
						MAX77828_TORCH_FLED1_EN);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  MAX77828_LED_REG_I_TORCH1 update failed\n",__func__ );
	}
	else
	{
		if(brightness > 0x7E)
			brightness = 0x7E;
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_DCDC_CNTL1,
                                MAX77828_DCDC_MODE_PAM|0xC0);
		if(IS_ERR_VALUE(ret))
                        pr_err("(%s) :  MAX77828_LED_REG_DCDC_CNTL1 write failed\n",__func__);

		/* mode select */
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_MODE_SEL,0xB1);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  MAX77828_LED_REG_MODE_SEL update failed\n",__func__);

		/* Torch ON, Set Current */
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_I_TORCH1,
				MAX77828_TORCH_FLED1_EN |
				((unsigned int)brightness << max77828_set_bit(MAX77828_TORCH_IOUT)));
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  MAX77828_LED_REG_I_TORCH1 write failed\n",__func__ );
	}
	return;
}

static int max77828_rgb_blink(struct led_classdev *led_cdev, unsigned long
							*delay_on, unsigned long *delay_off)
{
	u8 val;
	int ret;
	struct max77828_led_data *led_data = container_of(led_cdev, struct max77828_led_data, led);

	if(*delay_on==0 && *delay_off==0)
	{
		ret = max77828_read_reg(led_data->i2c, MAX77828_LED_REG_LEDBLNK, &val);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  LED Enable Bit read failed\n",__func__ );
		printk("(%s) LED Blink reg (0x%02x)\n",__func__,val);

		*delay_on = (val & MAX77828_LEDBLINKD) * 200;
		val = (val & MAX77828_LEDBLINKP);
		printk("(%s) LED Blink reg (blink pd. : 0x%02x)(delay: 0x%02lx)\n",__func__,val, *delay_on);
		if(val <= 0x0A)
			val = val * 500 + 1000;
		else if (val <= 0x0C)
			val = (val - 0x0A) * 1000 + 6000;
		else if (val <= 0x0E)
			val = (val - 0x0C) * 2000 + 8000;
		else
			val = *delay_on;
		*delay_off = val - *delay_on;
	}
	else
	{
		if (unlikely(*delay_on > 3000))
			return -EINVAL;

		val = *delay_on + *delay_off;
		if (val <= 6000)
			val = (val - 1000) / 500;
		else if (val < 8000)
			val = (val - 6000) / 1000;
		else if (val < 12000)
			val = (val - 8000) / 2000;
		else
			return -EINVAL;

		val |= (*delay_on / 200);
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_LEDBLNK, val);
		if(IS_ERR_VALUE(ret))
		{
			pr_err("(%s) :  LED write failed\n",__func__ );
			return ret;
		}
	}
	return 0;
}

static ssize_t store_max77828_rgb_blink(struct device *dev, struct device_attribute *devattr, 
													const char *buf, size_t count)
{
	int retval;
	enum led_brightness ledb = LED_OFF;
	unsigned int led_brightness = 0;
	unsigned long delay_on_time = 0;
	unsigned long delay_off_time = 0;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct max77828_led_data *led_data = container_of(led_cdev, struct max77828_led_data, led);

	retval = sscanf(buf, "0x%x %ld %ld", &led_brightness, &delay_on_time, &delay_off_time);
    if (retval == 0) {
            printk("fail to get led_blink value.\n");
            return count;
    }
    if(led_brightness > 0 && led_brightness <=127)
    	ledb = LED_HALF;
    else if (led_brightness > 127)
    	ledb = LED_FULL;

	led_data->led.brightness_set(led_cdev, ledb);
	led_data->led.blink_set (led_cdev,  &delay_on_time, &delay_off_time);

	return count;
}

static void max77828_rgb_set(struct max77828_led_data *led_data)
{
	int ret;
	int id = led_data->data->id;
	u8 reg;
	u8 bit = 0;

	switch(id)
	{
		case MAX77828_RGB_R:
			reg = MAX77828_LED_REG_LED1BRT;
			bit = 1<<1;
			break;
		case MAX77828_RGB_G:
			reg = MAX77828_LED_REG_LED2BRT;
			bit = 1<<2;
			break;
		case MAX77828_RGB_B:
			reg = MAX77828_LED_REG_LED3BRT;
			bit = 1<<3;
			break;
		default :
			printk("(%s) : Not valid LED ID\n",__func__);
			break;
	}

	if((enum led_brightness)led_data->test_brightness  == LED_OFF)
	{
		printk("(%s) LED_OFF\n",__func__);
		/* LED OFF */
		ret = max77828_update_reg(led_data->i2c, MAX77828_LED_REG_LEDEN,	0, bit);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  LED Bit 0x%02x update failed\n",__func__, bit );
	}
	else
	{
		/* Set Current*/
		ret = max77828_write_reg(led_data->i2c, reg, (unsigned int)led_data->test_brightness);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  write failed\n",__func__);

		/* LEDBLNK ON */
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_LEDBLNK, 0x50);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  LED Bit %x update failed\n",__func__, bit );

		/* LEDRMP ON */
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_LEDRMP, 0x00);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  LED Bit %x update failed\n",__func__, bit );

		ret = max77828_read_reg(led_data->i2c, MAX77828_LED_REG_LEDEN, &reg);
		bit |= reg;
		/* LED ON */
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_LEDEN, bit);
		if(IS_ERR_VALUE(ret))
			pr_err("(%s) :  LED Bit %x update failed\n",__func__, bit );
	}
	return;
}

#ifdef SEC_LED_SPECIFIC
static ssize_t max77828_led_r(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct max77828_led_data *led_data = container_of(led_cdev, struct max77828_led_data, led);

	int ret;
	int brightness;

	ret = kstrtoint(buf, 0, &brightness);
	if (ret != 0){
			printk("fail to get brightness.\n");
			goto out;
	}
	led_data->test_brightness = brightness;
	led_data->data->id = MAX77828_RGB_R;
	max77828_rgb_set(led_data);
out:
	return count;
}

static ssize_t max77828_led_g(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct max77828_led_data *led_data = container_of(led_cdev, struct max77828_led_data, led);

	int ret;
	int brightness;

	ret = kstrtoint(buf, 0, &brightness);
	if (ret != 0){
			printk("fail to get brightness.\n");
			goto out;
	}
	led_data->test_brightness = brightness;
	led_data->data->id = MAX77828_RGB_G;
	max77828_rgb_set(led_data);
out:
	return count;
}

static ssize_t max77828_led_b(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct max77828_led_data *led_data = container_of(led_cdev, struct max77828_led_data, led);

	int ret;
	int brightness;

	ret = kstrtoint(buf, 0, &brightness);
	if (ret != 0){
			printk("fail to get brightness.\n");
			goto out;
	}
	led_data->test_brightness = brightness;
	led_data->data->id = MAX77828_RGB_B;
	max77828_rgb_set(led_data);
out:
	return count;
}
#endif

static void led_set(struct max77828_led_data *led_data)
{
	struct max77828_led *data = led_data->data;
	int id = data->id;
	print_all_reg_value(led_data->i2c);
	switch(id)
	{
		case MAX77828_FLASH:
			max77828_flash_set(led_data);
			break;
		case MAX77828_TORCH:
			max77828_torch_set(led_data);
			break;
		case MAX77828_RGB_R :
		case MAX77828_RGB_G :
		case MAX77828_RGB_B :
			max77828_rgb_set(led_data);
			break;
		default:
			pr_err("(%s) : LED id is not valid\n",__func__);
			break;
	}
	return;
}

static void max77828_led_work(struct work_struct *work)
{
	struct max77828_led_data *led_data
		= container_of(work, struct max77828_led_data, work);

	DEBUG_MAX77828("[LED] %s\n", __func__);

	mutex_lock(&led_data->lock);
	led_set(led_data);
	mutex_unlock(&led_data->lock);
}
static int max77828_led_hw_setup(struct max77828_led_data *led_data)
{
	int value = 0;
	int ret;
	int mode;

	if (led_data->data->id == MAX77828_FLASH)
		mode = 0x01; /*Flash triggered via FLASHEN*/
	else
		mode = 0x02; /*Torch triggered via TORCHEN*/
	if (flash_torch_en) {
		if (mode == 0x01) {
			ret = gpio_request(led_flash_en, "max77828_flash_en");
			if (ret)
				pr_err("can't get max77828_flash_en");
			else {
				DEBUG_MAX77828("[LED] %s: max77828_flash_en set 0\n", __func__);
				gpio_direction_output(led_flash_en, 0);
				gpio_free(led_flash_en);
			}
		}
		if (mode == 0x02) {
			ret = gpio_request(led_torch_en, "max77828_torch_en");
			if (ret)
				pr_err("can't get max77828_torch_en");
			else {
				DEBUG_MAX77828("[LED] %s: max77828_torch_en set 0\n", __func__);
				gpio_direction_output(led_torch_en, 0);
				gpio_free(led_torch_en);
			}
		}
	} else
		pr_err("%s : can't find gpio", __func__);
	
	/*  pin control setting */
	value = MAX77828_TORCH_MD_TORCHEN|MAX77828_TORCHEN_PD|
			MAX77828_FLASH_MD_FLASHSTB|MAX77828_FLASHSTB_PD;
	ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_MODE_SEL, value);
	if (IS_ERR_VALUE(ret))
		return ret;

	ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_DCDC_CNTL1,
			MAX77828_DCDC_MODE_FAM|0xC0);
	if(IS_ERR_VALUE(ret))
		pr_err("(%s) :  MAX77828_LED_REG_DCDC_CNTL1 write failed\n",__func__);

	ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_DCDC_CNTL2, 0x14);                        
        if(IS_ERR_VALUE(ret))
                pr_err("(%s) :  MAX77828_LED_REG_DCDC_CNTL2 write failed\n",__func__);

	ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_DCDC_ILIM, 0xC0);
        if(IS_ERR_VALUE(ret))
                pr_err("(%s) :  MAX77828_LED_REG_DCDC_ILIM write failed\n",__func__);
	if (fdata->enable_maxflash)
	{
		if (fdata->maxflash_rising_timer > 4096 || fdata->maxflash_rising_timer > 4096)
			return -EINVAL;
		value = ((fdata->maxflash_rising_timer / 256 - 1) << max77828_set_bit(MAX77828_LB_TMR_R)) |
				((fdata->maxflash_falling_timer / 256 - 1) << max77828_set_bit(MAX77828_LB_TMR_F));
		ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_MAXFLASH1, value);
		if (IS_ERR_VALUE(ret))
			return ret;

		switch(fdata->maxflash_hysteresis)
		{
		case 0:
			value = 0;
			break;
		case 50000:
			value = 1;
			break;
		case 100000:
			value = 2;
			break;
		case 300000:
			value = 6;
			break;
		case 350000:
			value = 7;
			break;
		default:
			return -EINVAL;
		}

		if (likely(fdata->maxflash_threshold >= 2400000 && fdata->maxflash_threshold <= 3400000))
			value |= (((fdata->maxflash_threshold - 2400000) / 33000 + 1) << max77828_set_bit(MAX77828_MAXFLASH_HYS));
		else
			return -EINVAL;
	}
	else
		value = 0;

	return ret;
}

static int max77828_led_setup(struct max77828_led_data *led_data)
{
	int ret = 0;

	struct max77828_led *data = led_data->data;
	int id = data->id;
	int value;
	int i;

	DEBUG_MAX77828("[LED] %s : id - %d\n", __func__,id);
	switch(id)
	{
		case MAX77828_FLASH:
			max77828_led_hw_setup(led_data);
			/* ramp up/down setting */
			for (i = 0; i < ARRAY_SIZE(flash_ramp); i++)
				if (fdata->flash_ramp_up <= flash_ramp[i])
					break;
			if (unlikely(i == ARRAY_SIZE(flash_ramp)))
				return -EINVAL;
			value = (unsigned int)i << max77828_set_bit(MAX77828_FLASH_RU);
			for (i = 0; i < ARRAY_SIZE(flash_ramp); i++)
				if (fdata->flash_ramp_down <= flash_ramp[i])
					break;
			if (unlikely(i == ARRAY_SIZE(flash_ramp)))
				return -EINVAL;
			value |= (unsigned int)i << max77828_set_bit(MAX77828_FLASH_RD);

			ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_FLASH_RAMP_SEL, value);
			if (IS_ERR_VALUE(ret))
				return ret;
			/* flash timer control */
			ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_FLASH_TMR_CNTL, 0x7F);
			if (IS_ERR_VALUE(ret))
				return ret;

			/* MAXFLASH setting */
			value = 0x09;
			ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_MAXFLASH1, value);
			if (IS_ERR_VALUE(ret))
				return ret;

			ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_I_FLASH1, 0x80);
			if (IS_ERR_VALUE(ret))
				pr_err("(%s) :  MAX77828_LED_REG_I_FLASH1 write failed\n",__func__);

			break;
		case MAX77828_TORCH:
			max77828_led_hw_setup(led_data);
			for (i = 0; i < ARRAY_SIZE(torch_ramp); i++)
					if (fdata->flash_ramp_up <= torch_ramp[i])
						break;
			if (unlikely(i == ARRAY_SIZE(torch_ramp)))
					return -EINVAL;
				value = (unsigned int)i << max77828_set_bit(MAX77828_TORCH_RU);

			for (i = 0; i < ARRAY_SIZE(torch_ramp); i++)
					if (fdata->flash_ramp_down <= torch_ramp[i])
						break;
			if (unlikely(i == ARRAY_SIZE(torch_ramp)))
					return -EINVAL;
			value |= (unsigned int)i << max77828_set_bit(MAX77828_TORCH_RD);
			ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_FLASH_RAMP_SEL, value);
			if (IS_ERR_VALUE(ret))
				return ret;
		/* torch timer control */
			ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_TORCH_TMR_CNTL, 0x80);
			if (IS_ERR_VALUE(ret))
				return ret;
			/* MAXFLASH setting */
                        value = 0x09;
                        ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_MAXFLASH1, value);
                        if (IS_ERR_VALUE(ret))
                                return ret;

			ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_I_TORCH1, 0x80);
			if(IS_ERR_VALUE(ret))
				pr_err("(%s) :  MAX77828_LED_REG_I_TORCH1 write failed\n",__func__);

			break;
		case MAX77828_RGB_R:
			ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_LEDEN, 0x02);
			if (IS_ERR_VALUE(ret))
				return ret;
			ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_LED0BRT, 0xFF);
			if (IS_ERR_VALUE(ret))
				return ret;
			break;
		case MAX77828_RGB_G:
			ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_LED1BRT, 0x00);
			if (IS_ERR_VALUE(ret))
				return ret;
			break;
		case MAX77828_RGB_B:
			ret = max77828_write_reg(led_data->i2c, MAX77828_LED_REG_LED2BRT, 0x00);
			if (IS_ERR_VALUE(ret))
				return ret;
			break;
		default :
			printk("Error : No such LED present\n");
			break;
	}
	return ret;
}

static ssize_t max77828_flash(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct max77828_led_data *led_data = container_of(led_cdev, struct max77828_led_data, led);
	ssize_t ret = -EINVAL;

	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	DEBUG_MAX77828("[LED] %s\n", __func__);

	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		led_data->data->id = MAX77828_FLASH;
		if (state > led_cdev->max_brightness)
			state = led_cdev->max_brightness;
		led_cdev->brightness = state;
		DEBUG_MAX77828("[LED] %s : led_cdev->brightness %d\n",
		       __func__, led_cdev->brightness);
		led_data->test_brightness = state;
		if (!(led_cdev->flags & LED_SUSPENDED))
			max77828_torch_set(led_data);
	}

	return ret;
}

static DEVICE_ATTR(rear_flash, S_IWUSR|S_IWGRP|S_IROTH,
	NULL, max77828_flash);

#ifdef SEC_LED_SPECIFIC
static DEVICE_ATTR(led_blink, 0664, NULL, store_max77828_rgb_blink);

static DEVICE_ATTR(led_r, 0664, NULL, max77828_led_r);
static DEVICE_ATTR(led_g, 0664, NULL, max77828_led_g);
static DEVICE_ATTR(led_b, 0664, NULL, max77828_led_b);


static struct attribute *sec_led_attributes[] = {
	&dev_attr_led_blink.attr,
	&dev_attr_led_r.attr,
	&dev_attr_led_g.attr,
	&dev_attr_led_b.attr,
	NULL,
};

static struct attribute_group sec_led_attr_group = {
	.attrs = sec_led_attributes,
};
#endif

static int max77828_led_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i,j;
	struct max77828_dev *max77828 = dev_get_drvdata(pdev->dev.parent);
	struct max77828_platform_data *max77828_pdata
		= dev_get_platdata(max77828->dev);
	struct max77828_led_platform_data *pdata = max77828_pdata->led_data;
	struct max77828_led_data *led_data;
	struct max77828_led *data;
	struct max77828_led_data **led_datas;

	pr_err("max77828_led_probe\n");

	if (pdata == NULL) {
		pr_err("[LED] no platform data for this led is found\n");
		return -EFAULT;
	}

	led_datas = kzalloc(sizeof(struct max77828_led_data *)
			    * MAX77828_LED_MAX, GFP_KERNEL);
	if (unlikely(!led_datas)) {
		pr_err("[LED] memory allocation error %s", __func__);
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, led_datas);

	fdata = kzalloc(sizeof(struct max77828_flash_platform_data), GFP_KERNEL);
	if (unlikely(fdata == NULL))
		return (-ENOMEM);
	fdata->flash_ramp_up = 16512;
	fdata->flash_ramp_down = 16512;
	fdata->torch_ramp_up = 32776;
	fdata->torch_ramp_up = 32776;
	fdata->enable_maxflash = 1;
	fdata->maxflash_rising_timer = 4096;
	fdata->maxflash_falling_timer = 256;
	fdata->maxflash_hysteresis = 50000;
	fdata->maxflash_threshold = 2400000;

	if (led_torch_en && led_flash_en) {
	    DEBUG_MAX77828("[LED] %s, led_torch %d, led_flash %d\n", __func__,
			   led_torch_en, led_flash_en);
	    flash_torch_en = 1;

	} else {
	    pr_err("%s : can't find gpio", __func__);
	    flash_torch_en = 0;
	}

	for (i = 0; i != pdata->num_leds; ++i)
	{
		data = &(pdata->leds[i]);

		led_data = kzalloc(sizeof(struct max77828_led_data),
				   GFP_KERNEL);
		led_datas[i] = led_data;
		if (unlikely(!led_data)) {
			pr_err("[LED] memory allocation error %s\n", __func__);
			ret = -ENOMEM;
			continue;
		}

		led_data->max77828 = max77828;
		led_data->i2c = max77828->led;
		led_data->data = data;
		led_data->led.name = data->name;
		led_data->led.brightness_set = max77828_led_set;
		led_data->led.brightness = LED_OFF;
		led_data->brightness = data->brightness;
		led_data->led.flags = 0;
		led_data->led.max_brightness = 0xFF;
		for(j = 2; j != pdata->num_leds; ++j)
			led_data->led.blink_set = max77828_rgb_blink;

		mutex_init(&led_data->lock);
		spin_lock_init(&led_data->value_lock);
		INIT_WORK(&led_data->work, max77828_led_work);

		ret = led_classdev_register(&pdev->dev, &led_data->led);
		if (unlikely(ret)) {
			pr_err("unable to register LED\n");
			kfree(led_data);
			ret = -EFAULT;
			continue;
		}

		ret = max77828_led_setup(led_data);
		if (unlikely(ret)) {
			pr_err("unable to register LED\n");
			mutex_destroy(&led_data->lock);
			led_classdev_unregister(&led_data->led);
			kfree(led_data);
			ret = -EFAULT;
		}
	}
//	ret = max77828_led_hw_setup(led_data);
#if defined(DEBUG_FLASH)
	print_all_reg_value(max77828->led);
	(void) debugfs_create_file("max77828-led-regs", S_IRUGO, NULL,
			(void *)max77828, &max77828_debugfs_fops);
#endif
	if (!IS_ERR(camera_class)) {
	    flash_dev = device_create(camera_class, NULL, 0, led_datas[0], "flash");
	    if (flash_dev < 0)
		pr_err("Failed to create device(flash)!\n");

	    if (device_create_file(flash_dev, &dev_attr_rear_flash) < 0) {
		pr_err("failed to create device file, %s\n",
		       dev_attr_rear_flash.attr.name);
	    }
#ifdef CONFIG_LEDS_SEPERATE_MOVIE_FLASH
	    if (device_create_file(flash_dev, &dev_attr_movie_brightness) < 0) {
		    pr_err("failed to create device file, %s\n",
			   dev_attr_movie_brightness.attr.name);
	    }
#endif
	} else
	    pr_err("Failed to create device(flash) because of nothing camera class!\n");

#ifdef CONFIG_LEDS_SWITCH
	if (system_rev < FLASH_SWITCH_REMOVED_REVISION) {
		if (gpio_request(GPIO_CAM_SW_EN, "CAM_SW_EN"))
			pr_err("failed to request CAM_SW_EN\n");
		else
			gpio_direction_output(GPIO_CAM_SW_EN, 1);
	}
#endif

#ifdef SEC_LED_SPECIFIC
        led_dev = device_create(sec_class, NULL, 0, led_data, "led");
        if (IS_ERR(led_dev)) {
                printk("Failed to create device for samsung specific led\n");
                ret = -ENODEV;
                goto exit;
        }
        ret = sysfs_create_group(&led_dev->kobj, &sec_led_attr_group);
        if (ret) {
                printk("Failed to create sysfs group for samsung specific led\n");
                goto exit;
        }
        return ret;

 exit:
   device_destroy(sec_class, 0);
#endif


	return ret;
}

static int __devexit max77828_led_remove(struct platform_device *pdev)
{

	struct max77828_led_data **led_datas = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i != MAX77828_LED_MAX; ++i) {
		if (led_datas[i] == NULL)
			continue;

		cancel_work_sync(&led_datas[i]->work);
		mutex_destroy(&led_datas[i]->lock);
		led_classdev_unregister(&led_datas[i]->led);
		kfree(led_datas[i]);
	}
	kfree(led_datas);
	kfree(fdata);

	device_remove_file(flash_dev, &dev_attr_rear_flash);
#ifdef CONFIG_LEDS_SEPERATE_MOVIE_FLASH
	device_remove_file(flash_dev, &dev_attr_movie_brightness);
#endif	
	device_destroy(camera_class, 0);
	class_destroy(camera_class);
#ifdef SEC_LED_SPECIFIC
        sysfs_remove_group(&led_dev->kobj, &sec_led_attr_group);
#endif
	return 0;
}

void max77828_led_shutdown(struct device *dev)
{
}

static struct platform_driver max77828_led_driver = {
	.probe		= max77828_led_probe,
	.remove		= __devexit_p(max77828_led_remove),
	.driver		= {
		.name	= "max77828-led",
		.owner	= THIS_MODULE,
		.shutdown = max77828_led_shutdown,
	},
};

static int __init max77828_led_init(void)
{
	return platform_driver_register(&max77828_led_driver);
}
module_init(max77828_led_init);

static void __exit max77828_led_exit(void)
{
	platform_driver_unregister(&max77828_led_driver);
}
module_exit(max77828_led_exit);

MODULE_AUTHOR("Ravi Shekhar Singh <shekhar.sr@samsung.com>");
MODULE_DESCRIPTION("MAX77828 LED driver");
MODULE_LICENSE("GPL");
