/*
 * leds_ktd2026.c - driver for KTD2026 led driver chip
 *
 * Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 * Contact: Hyoyoung Kim <hyway.kim@samsung.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/leds.h>
#include <linux/leds-ktd2026.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#undef AN3029_COMPATIBLE

#if defined(CONFIG_SEC_FACTORY)
#if defined (CONFIG_SEC_ATLANTIC_PROJECT)
static int batt_id_value;
static int jig_power_on_value;
#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
extern int get_lcd_attached(void);
#endif

static int __init get_batt_id_value_cmdline(char *mode)
{
	batt_id_value = mode[0]-48;		// Converting Char to Int
	return 0;
}

static int __init jig_power_on_value_cmdline(char *mode)
{
	jig_power_on_value = mode[0]-48;
	return 0;
}

__setup( "jig_power_on=0x", jig_power_on_value_cmdline );
__setup( "batt_id_value=0x", get_batt_id_value_cmdline );
#endif
#endif

/* KTD2026 register map */
#define KTD2026_REG_EN_RST		0x00
#define KTD2026_REG_FLASH_PERIOD	0x01
#define KTD2026_REG_PWM1_TIMER		0x02
#define KTD2026_REG_PWM2_TIMER		0x03
#define KTD2026_REG_LED_EN		0x04
#define KTD2026_REG_TRISE_TFALL		0x05
#define KTD2026_REG_LED1		0x06
#define KTD2026_REG_LED2		0x07
#define KTD2026_REG_LED3		0x08
#define KTD2026_REG_MAX			0x09
#define KTD2026_TIME_UNIT		500
/* MASK */
#define CNT_TIMER_SLOT_MASK		0x07
#define CNT_ENABLE_MASK			0x18
#define CNT_RISEFALL_TSCALE_MASK	0x60

#define CNT_TIMER_SLOT_SHIFT		0x00
#define CNT_ENABLE_SHIFT		0x03
#define CNT_RISEFALL_TSCALE_SHIFT	0x05

#define LED_R_MASK		0x00ff0000
#define LED_G_MASK		0x0000ff00
#define LED_B_MASK		0x000000ff
#define LED_R_SHIFT		16
#define LED_G_SHIFT		8

#define KTD2026_RESET		0x07

#define LED_MAX_CURRENT		0x30
#define LED_DEFAULT_CURRENT	0x20
#define LED_LOW_CURRENT		0x07
#define LED_OFF			0x00

#define	MAX_NUM_LEDS		3

static u8 led_dynamic_current = LED_DEFAULT_CURRENT;
static u8 led_lowpower_mode = 0x0;

enum ktd2026_led_mode {
	LED_EN_OFF	= 0,
	LED_EN_ON	= 1,
	LED_EN_PWM1	= 2,
	LED_EN_PWM2	= 3,
};
enum ktd2026_pwm{
	PWM1 = 0,
	PWM2 = 1,
};

static struct ktd2026_led_conf led_conf[] = {
	{
		.name = "led_r",
		.brightness = LED_OFF,
		.max_brightness = LED_MAX_CURRENT,
		.flags = 0,
	}, {
		.name = "led_g",
		.brightness = LED_OFF,
		.max_brightness = LED_MAX_CURRENT,
		.flags = 0,
	}, {
		.name = "led_b",
		.brightness = LED_OFF,
		.max_brightness = LED_MAX_CURRENT,
		.flags = 0,
	}
};

enum ktd2026_led_enum {
	LED_R = 0,
	LED_G = 2,
	LED_B = 4,
};

struct ktd2026_led {
	u8	channel;
	u8	brightness;
	struct led_classdev	cdev;
	struct work_struct	brightness_work;
	unsigned long delay_on_time_ms;
	unsigned long delay_off_time_ms;
};

struct ktd2026_data {
	struct	i2c_client	*client;
	struct	mutex	mutex;
	struct	ktd2026_led	leds[MAX_NUM_LEDS];
	u8		shadow_reg[KTD2026_REG_MAX];
};

struct i2c_client *b_client;

#define SEC_LED_SPECIFIC

#ifdef SEC_LED_SPECIFIC
extern struct class *sec_class;
static struct device *led_dev;
/*path : /sys/class/sec/led/led_pattern*/
/*path : /sys/class/sec/led/led_blink*/
/*path : /sys/class/sec/led/led_brightness*/
/*path : /sys/class/leds/led_r/brightness*/
/*path : /sys/class/leds/led_g/brightness*/
/*path : /sys/class/leds/led_b/brightness*/
#endif

static void ktd2026_leds_on(enum ktd2026_led_enum led,
		enum ktd2026_led_mode mode, u8 bright);


static inline struct ktd2026_led *cdev_to_led(struct led_classdev *cdev)
{
	return container_of(cdev, struct ktd2026_led, cdev);
}

static int leds_i2c_write_all(struct i2c_client *client)
{
	struct ktd2026_data *data = i2c_get_clientdata(client);
	int ret = 0;
	int retry = 0;

	mutex_lock(&data->mutex);
	do
	{
		ret = i2c_smbus_write_i2c_block_data(client,
				KTD2026_REG_EN_RST, KTD2026_REG_MAX,
				&data->shadow_reg[KTD2026_REG_EN_RST]);
		if (ret < 0) {
			dev_err(&client->adapter->dev,
				"%s: failure on i2c block write\n",
				__func__);
			retry++;
			if(retry > 10) 
			{
				dev_err(&client->adapter->dev,
					"%s: retry > 10 : Fatal i2c error\n",
					__func__);
				break;
			}
			msleep(100);
		}
	} while(ret < 0);

	mutex_unlock(&data->mutex);
	return ret;
}

void ktd2026_set_brightness(struct led_classdev *cdev,
			enum led_brightness brightness)
{
		struct ktd2026_led *led = cdev_to_led(cdev);
		led->brightness = (u8)brightness;
		schedule_work(&led->brightness_work);
}

static void ktd2026_led_brightness_work(struct work_struct *work)
{
		struct i2c_client *client = b_client;
		struct ktd2026_led *led = container_of(work,
				struct ktd2026_led, brightness_work);
		if (led->brightness==0)
			ktd2026_leds_on(led->channel, LED_EN_OFF, 0);
		else ktd2026_leds_on(led->channel, LED_EN_ON, led->brightness);

		leds_i2c_write_all(client);
}

/* leds_set_slope_mode function and leds_on are for compatiblity
 * with AN3029A LED Driver , thus depricated.
 *
 * leds_set_slope_mode() sets correct values to corresponding shadow registers.
 * led: stands for LED_R or LED_G or LED_B.
 * delay: represents for starting delay time in multiple of .5 second.
 * dutymax: led at slope lighting maximum PWM Duty setting.
 * dutymid: led at slope lighting middle PWM Duty setting.
 * dutymin: led at slope lighting minimum PWM Duty Setting.
 * slptt1: total time of slope operation 1 and 2, in multiple of .5 second.
 * slptt2: total time of slope operation 3 and 4, in multiple of .5 second.
 * dt1: detention time at each step in slope operation 1, in multiple of 4ms.
 * dt2: detention time at each step in slope operation 2, in multiple of 4ms.
 * dt3: detention time at each step in slope operation 3, in multiple of 4ms.
 * dt4: detention time at each step in slope operation 4, in multiple of 4ms.
 */
#ifdef AN3029_COMPATIBLE
static void leds_set_slope_mode(struct i2c_client *client,
				enum ktd2026_led_enum led, u8 delay,
				u8 dutymax, u8 dutymid, u8 dutymin,
				u8 slptt1, u8 slptt2,
				u8 dt1, u8 dt2, u8 dt3, u8 dt4)
{
	struct ktd2026_data *data = i2c_get_clientdata(client);

	data->shadow_reg[KTD2026_REG_FLASH_PERIOD]
			= (slptt1 + slptt2) * 4 + 2;
	data->shadow_reg[KTD2026_REG_PWM1_TIMER]
			= slptt1*255 / (slptt1 + slptt2);
	data->shadow_reg[KTD2026_REG_TRISE_TFALL]
			= ((dt3+dt4)*5 << 4) + (dt1+dt2)*5;
}

static void leds_on(enum ktd2026_led_enum led, bool on, bool slopemode,
			u8 ledcc)
{
	struct ktd2026_data *data = i2c_get_clientdata(b_client);

	if(led == LED_R)data->shadow_reg[KTD2026_REG_LED1] = ledcc;
	if(led == LED_G)data->shadow_reg[KTD2026_REG_LED2] = ledcc;
	if(led == LED_B)data->shadow_reg[KTD2026_REG_LED3] = ledcc;

	if (on)
	{
		if (slopemode)
			data->shadow_reg[KTD2026_REG_LED_EN]
				|= LED_EN_PWM1 << led;
		else data->shadow_reg[KTD2026_REG_LED_EN] |= LED_EN_ON << led;
	}
	else
	/*off*/
		data->shadow_reg[KTD2026_REG_LED_EN] &= ~(LED_EN_PWM2 << led);
}
#endif
static void ktd2026_leds_on(enum ktd2026_led_enum led,
		enum ktd2026_led_mode mode, u8 bright)
{
	struct ktd2026_data *data = i2c_get_clientdata(b_client);

	data->shadow_reg[KTD2026_REG_LED1 + led/2] = bright;

	if(mode == LED_EN_OFF)
		data->shadow_reg[KTD2026_REG_LED_EN] &= ~(LED_EN_PWM2 << led);
	else
		data->shadow_reg[KTD2026_REG_LED_EN] |= mode << led;
}
void ktd2026_set_timerslot_control(int timer_slot)
{
	struct ktd2026_data *data = i2c_get_clientdata(b_client);

	data->shadow_reg[KTD2026_REG_EN_RST] &= ~(CNT_TIMER_SLOT_MASK);
	data->shadow_reg[KTD2026_REG_EN_RST]
		|= timer_slot << CNT_TIMER_SLOT_SHIFT;
}

/*  Flash period = period * 0.128 + 0.256
	exception  0 = 0.128s
	please refer to data sheet for detail */
void ktd2026_set_period(int period)
{
	struct ktd2026_data *data = i2c_get_clientdata(b_client);

	data->shadow_reg[KTD2026_REG_FLASH_PERIOD] = period;
}

/* MAX duty = 0xFF (99.6%) , min duty = 0x0 (0%) , 0.4% scale */
void ktd2026_set_pwm_duty(enum ktd2026_pwm pwm, int duty)
{
	struct ktd2026_data *data = i2c_get_clientdata(b_client);
	data->shadow_reg[KTD2026_REG_PWM1_TIMER + pwm] = duty;
}

/* Rise Ramp Time = trise * 96 (ms) */
/* minimum rise ramp time = 1.5ms when traise is set to 0 */
/* Tscale */
/* 0 = 1x      1 = 2x slower      2 = 4x slower    3 = 8x slower */

void ktd2026_set_trise_tfall(int trise, int tfall, int tscale)
{
	struct ktd2026_data *data = i2c_get_clientdata(b_client);

	data->shadow_reg[KTD2026_REG_TRISE_TFALL] = (tfall << 4) + trise;

	data->shadow_reg[KTD2026_REG_EN_RST] &= ~(CNT_RISEFALL_TSCALE_MASK);
	data->shadow_reg[KTD2026_REG_EN_RST]
			|= tscale << CNT_RISEFALL_TSCALE_SHIFT;
}

void ktd2026_set_sleep(int sleep)
{
	struct ktd2026_data *data = i2c_get_clientdata(b_client);

	if (sleep)
	{
		int val;
		val = 0x1 << CNT_ENABLE_SHIFT;
		data->shadow_reg[KTD2026_REG_EN_RST] |= val;
	}
	else
		data->shadow_reg[KTD2026_REG_EN_RST] &= ~CNT_ENABLE_MASK;
}

#ifdef SEC_LED_SPECIFIC
static void ktd2026_reset(void)
{
	int retval;
	struct i2c_client *client;
	client = b_client;

	ktd2026_leds_on(LED_R, LED_EN_OFF, 0);
	ktd2026_leds_on(LED_G, LED_EN_OFF, 0);
	ktd2026_leds_on(LED_B, LED_EN_OFF, 0);
	retval = leds_i2c_write_all(client);
	if (retval)
		pr_err("%s:leds_i2c_write_all failed\n", __func__);

	ktd2026_set_timerslot_control(0); /* Tslot1 */
	ktd2026_set_period(0);
	ktd2026_set_pwm_duty(PWM1, 0);
	ktd2026_set_pwm_duty(PWM2, 0);
	ktd2026_set_trise_tfall(0, 0, 0);

	/* get into sleep mode after turing off all the leds */
	ktd2026_set_sleep(1);

	retval = leds_i2c_write_all(client);
	if (retval)
		pr_err("%s:leds_i2c_write_all failed\n", __func__);

	/* reset sleep mode, so that next i2c command
		would not make the driver IC go into sleep mode */
	ktd2026_set_sleep(0);
}

void ktd2026_start_led_pattern(enum ktd2026_pattern mode)
{
	int retval;

	struct i2c_client *client;
	client = b_client;

	if (mode > POWERING)
		return;
	/* Set all LEDs Off */
	ktd2026_reset();
	if (mode == LED_OFF)
		return;

	/* Set to low power consumption mode */
	if (led_lowpower_mode == 1)
		led_dynamic_current = LED_LOW_CURRENT;
	else
		led_dynamic_current = LED_DEFAULT_CURRENT;

	switch (mode) {
	case CHARGING:
		ktd2026_leds_on(LED_R, LED_EN_ON, led_dynamic_current);
		break;

	case CHARGING_ERR:
		ktd2026_set_timerslot_control(1); /* Tslot2 */
		ktd2026_set_period(6);
		ktd2026_set_pwm_duty(PWM1, 127);
		ktd2026_set_pwm_duty(PWM2, 127);
		ktd2026_set_trise_tfall(0, 0, 0);
		ktd2026_leds_on(LED_R, LED_EN_PWM2, led_dynamic_current);
		break;

	case MISSED_NOTI:
		ktd2026_set_timerslot_control(1); /* Tslot2 */
		ktd2026_set_period(41);
		ktd2026_set_pwm_duty(PWM1, 232);
		ktd2026_set_pwm_duty(PWM2, 23);
		ktd2026_set_trise_tfall(0, 0, 0);
		ktd2026_leds_on(LED_B, LED_EN_PWM2, led_dynamic_current);
		break;

	case LOW_BATTERY:
		ktd2026_set_timerslot_control(1); /* Tslot2 */
		ktd2026_set_period(41);
		ktd2026_set_pwm_duty(PWM1, 232);
		ktd2026_set_pwm_duty(PWM2, 23);
		ktd2026_set_trise_tfall(0, 0, 0);
		ktd2026_leds_on(LED_R, LED_EN_PWM2, led_dynamic_current);
		break;

	case FULLY_CHARGED:
		ktd2026_leds_on(LED_G, LED_EN_ON, led_dynamic_current);
		break;

	case POWERING:
		ktd2026_set_timerslot_control(0); /* Tslot1 */
		ktd2026_set_period(14);
		ktd2026_set_pwm_duty(PWM1, 128);
		ktd2026_set_trise_tfall(7, 7, 0);
		ktd2026_leds_on(LED_B, LED_EN_ON, led_dynamic_current/2);
		ktd2026_leds_on(LED_G, LED_EN_PWM1, led_dynamic_current/3);
		break;

	default:
		return;
		break;
	}
	retval = leds_i2c_write_all(client);
	if (retval)
		pr_err("%s:leds_i2c_write_all failed\n", __func__);
}
EXPORT_SYMBOL(ktd2026_start_led_pattern);

static void ktd2026_set_led_blink(enum ktd2026_led_enum led,
					unsigned int delay_on_time,
					unsigned int delay_off_time,
					u8 brightness)
{
	int on_time, off_time;

	if (brightness == LED_OFF) {
		ktd2026_leds_on(led, LED_EN_OFF, brightness);
		return;
	}

	if (brightness > LED_MAX_CURRENT)
		brightness = LED_MAX_CURRENT;

	if (delay_off_time == LED_OFF) {
		ktd2026_leds_on(led, LED_EN_ON, brightness);
		return;
	} else
		ktd2026_leds_on(led, LED_EN_PWM1, brightness);

	on_time = (delay_on_time + KTD2026_TIME_UNIT - 1) / KTD2026_TIME_UNIT;
	off_time = (delay_off_time + KTD2026_TIME_UNIT - 1) / KTD2026_TIME_UNIT;
	ktd2026_set_timerslot_control(0); /* Tslot1 */
	ktd2026_set_period((on_time+off_time) * 4 + 2);
	ktd2026_set_pwm_duty(PWM1, on_time*255 / (on_time + off_time));
	ktd2026_set_trise_tfall(0, 0, 0);
	pr_info("%s:on_time=%d, off_time=%d, period=%d, duty=%d\n" ,
		__func__, on_time, off_time, (on_time+off_time) * 4 + 2,
		on_time * 255 / (on_time + off_time) );
}

static ssize_t store_ktd2026_led_lowpower(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	int retval;
	u8 led_lowpower;
	struct ktd2026_data *data = dev_get_drvdata(dev);

	retval = kstrtou8(buf, 0, &led_lowpower);
	if (retval != 0) {
		dev_err(&data->client->dev, "fail to get led_lowpower.\n");
		return count;
	}

	led_lowpower_mode = led_lowpower;

	pr_info("%s:led_lowpower mode set to %i\n", __func__, led_lowpower);

	return count;
}

static ssize_t store_ktd2026_led_brightness(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	int retval;
	u8 brightness;
	struct ktd2026_data *data = dev_get_drvdata(dev);

	retval = kstrtou8(buf, 0, &brightness);
	if (retval != 0) {
		dev_err(&data->client->dev, "fail to get led_brightness.\n");
		return count;
	}

	if (brightness > LED_MAX_CURRENT)
		brightness = LED_MAX_CURRENT;

	led_dynamic_current = brightness;

	pr_info("%s:led brightness set to %i\n", __func__, brightness);

	return count;
}

static ssize_t store_ktd2026_led_br_lev(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	int retval;
	unsigned long brightness_lev;
	struct ktd2026_data *data = dev_get_drvdata(dev);

	retval = kstrtoul(buf, 16, &brightness_lev);
	if (retval != 0) {
		dev_err(&data->client->dev, "fail to get led_br_lev.\n");
		return count;
	}

	return count;
}

static ssize_t store_ktd2026_led_pattern(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	int retval;
	unsigned int mode = 0;
	struct ktd2026_data *data = dev_get_drvdata(dev);

	retval = sscanf(buf, "%d", &mode);

	if (retval == 0) {
		dev_err(&data->client->dev, "fail to get led_pattern mode.\n");
		return count;
	}

	ktd2026_start_led_pattern(mode);
	pr_info("%s:led pattern : %d is activated(Type:%d)\n",
		__func__, mode, led_lowpower_mode);
	return count;
}

static ssize_t store_ktd2026_led_blink(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	/* ex) echo 0x201000 2000 500 > led_blink */
	/* brightness r=20 g=10 b=00, ontime=2000ms, offtime=500ms */
	/* minimum timeunit  of 500ms */

	int retval;
	unsigned int led_brightness = 0;
	unsigned int delay_on_time = 0;
	unsigned int delay_off_time = 0;
	struct ktd2026_data *data = dev_get_drvdata(dev);
	u8 led_r_brightness = 0;
	u8 led_g_brightness = 0;
	u8 led_b_brightness = 0;

	retval = sscanf(buf, "0x%x %d %d", &led_brightness,
		&delay_on_time, &delay_off_time);

	if (retval == 0) {
		dev_err(&data->client->dev, "fail to get led_blink value.\n");
		return count;
	}
	/*Reset ktd2026*/
	ktd2026_start_led_pattern(LED_OFF);

	/*Set LED blink mode*/
	led_r_brightness = ((u32)led_brightness & LED_R_MASK) >> LED_R_SHIFT;
	led_g_brightness = ((u32)led_brightness & LED_G_MASK) >> LED_G_SHIFT;
	led_b_brightness = ((u32)led_brightness & LED_B_MASK);

	ktd2026_set_led_blink(LED_R, delay_on_time,
			delay_off_time, led_r_brightness);
	ktd2026_set_led_blink(LED_G, delay_on_time,
			delay_off_time, led_g_brightness);
	ktd2026_set_led_blink(LED_B, delay_on_time,
			delay_off_time, led_b_brightness);

	leds_i2c_write_all(data->client);

	pr_info("%s:led_blink is called, Color:0x%X Brightness:%i\n",
			__func__, led_brightness, led_dynamic_current);

	return count;
}

void ktd2026_led_blink(int rgb, int on, int off)
{
	unsigned int led_brightness = rgb;
	unsigned int delay_on_time = on;
	unsigned int delay_off_time = off;

	u8 led_r_brightness = 0;
	u8 led_g_brightness = 0;
	u8 led_b_brightness = 0;

	/*Reset ktd2026*/
	ktd2026_start_led_pattern(LED_OFF);

	/*Set LED blink mode*/
	led_r_brightness = ((u32)led_brightness & LED_R_MASK) >> LED_R_SHIFT;
	led_g_brightness = ((u32)led_brightness & LED_G_MASK) >> LED_G_SHIFT;
	led_b_brightness = ((u32)led_brightness & LED_B_MASK);

	ktd2026_set_led_blink(LED_R, delay_on_time,
			delay_off_time, led_r_brightness);
	ktd2026_set_led_blink(LED_G, delay_on_time,
			delay_off_time, led_g_brightness);
	ktd2026_set_led_blink(LED_B, delay_on_time,
			delay_off_time, led_b_brightness);

	leds_i2c_write_all(b_client);

	pr_info("%s:led_blink is called, Color:0x%X Brightness:%i\n",
		__func__, led_brightness, led_dynamic_current);
}
EXPORT_SYMBOL(ktd2026_led_blink);

static ssize_t store_led_r(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct ktd2026_data *data = dev_get_drvdata(dev);
	int ret;
	u8 brightness;

	ret = kstrtou8(buf, 0, &brightness);
	if (ret != 0) {
		dev_err(&data->client->dev, "fail to get brightness.\n");
		goto out;
	}

	if (brightness == 0)
		ktd2026_leds_on(LED_R, LED_EN_OFF, 0);
	else
		ktd2026_leds_on(LED_R, LED_EN_ON, brightness);

	leds_i2c_write_all(data->client);
out:
	return count;
}

static ssize_t store_led_g(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct ktd2026_data *data = dev_get_drvdata(dev);
	int ret;
	u8 brightness;

	ret = kstrtou8(buf, 0, &brightness);
	if (ret != 0) {
		dev_err(&data->client->dev, "fail to get brightness.\n");
		goto out;
	}

	if (brightness == 0)
		ktd2026_leds_on(LED_G, LED_EN_OFF, 0);
	else
		ktd2026_leds_on(LED_G, LED_EN_ON, brightness);

	leds_i2c_write_all(data->client);
out:
	return count;
}

static ssize_t store_led_b(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct ktd2026_data *data = dev_get_drvdata(dev);
	int ret;
	u8 brightness;

	ret = kstrtou8(buf, 0, &brightness);
	if (ret != 0) {
		dev_err(&data->client->dev, "fail to get brightness.\n");
		goto out;
	}

	if (brightness == 0)
		ktd2026_leds_on(LED_B, LED_EN_OFF, 0);
	else
		ktd2026_leds_on(LED_B, LED_EN_ON, brightness);

	leds_i2c_write_all(data->client);
out:
	return count;

}
#endif

/* Added for led common class */
static ssize_t led_delay_on_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ktd2026_led *led = cdev_to_led(led_cdev);

	return sprintf(buf, "%lu\n", led->delay_on_time_ms);
}

static ssize_t led_delay_on_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t len)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ktd2026_led *led = cdev_to_led(led_cdev);
	unsigned long time;

	if (kstrtoul(buf, 0, &time))
		return -EINVAL;

	led->delay_on_time_ms = (int)time;
	return len;
}

static ssize_t led_delay_off_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ktd2026_led *led = cdev_to_led(led_cdev);

	return sprintf(buf, "%lu\n", led->delay_off_time_ms);
}

static ssize_t led_delay_off_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t len)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ktd2026_led *led = cdev_to_led(led_cdev);
	unsigned long time;

	if (kstrtoul(buf, 0, &time))
		return -EINVAL;

	led->delay_off_time_ms = (int)time;

	return len;
}

static ssize_t led_blink_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t len)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct ktd2026_led *led = cdev_to_led(led_cdev);
	unsigned long blink_set;

	if (kstrtoul(buf, 0, &blink_set))
		return -EINVAL;

	if (!blink_set) {
		led->delay_on_time_ms = LED_OFF;
		ktd2026_set_brightness(led_cdev, LED_OFF);
	}

	led_blink_set(led_cdev,
		&led->delay_on_time_ms, &led->delay_off_time_ms);

	return len;
}

/* permission for sysfs node */
static DEVICE_ATTR(delay_on, 0644, led_delay_on_show, led_delay_on_store);
static DEVICE_ATTR(delay_off, 0644, led_delay_off_show, led_delay_off_store);
static DEVICE_ATTR(blink, 0644, NULL, led_blink_store);

#ifdef SEC_LED_SPECIFIC
/* below nodes is SAMSUNG specific nodes */
static DEVICE_ATTR(led_r, 0664, NULL, store_led_r);
static DEVICE_ATTR(led_g, 0664, NULL, store_led_g);
static DEVICE_ATTR(led_b, 0664, NULL, store_led_b);
/* led_pattern node permission is 664 */
/* To access sysfs node from other groups */
static DEVICE_ATTR(led_pattern, 0664, NULL, \
					store_ktd2026_led_pattern);
static DEVICE_ATTR(led_blink, 0664, NULL, \
					store_ktd2026_led_blink);
static DEVICE_ATTR(led_br_lev, 0664, NULL, \
					store_ktd2026_led_br_lev);
static DEVICE_ATTR(led_brightness, 0664, NULL, \
					store_ktd2026_led_brightness);
static DEVICE_ATTR(led_lowpower, 0664, NULL, \
					store_ktd2026_led_lowpower);

#endif
static struct attribute *led_class_attrs[] = {
	&dev_attr_delay_on.attr,
	&dev_attr_delay_off.attr,
	&dev_attr_blink.attr,
	NULL,
};

static struct attribute_group common_led_attr_group = {
	.attrs = led_class_attrs,
};

#ifdef SEC_LED_SPECIFIC
static struct attribute *sec_led_attributes[] = {
	&dev_attr_led_r.attr,
	&dev_attr_led_g.attr,
	&dev_attr_led_b.attr,
	&dev_attr_led_pattern.attr,
	&dev_attr_led_blink.attr,
	&dev_attr_led_br_lev.attr,
	&dev_attr_led_brightness.attr,
	&dev_attr_led_lowpower.attr,
	NULL,
};

static struct attribute_group sec_led_attr_group = {
	.attrs = sec_led_attributes,
};
#endif

static int __devinit initialize_channel(struct i2c_client *client,
					struct ktd2026_led *led, int channel)
{
	struct device *dev = &client->dev;
	int ret;

	led->channel = channel * 2;
	led->cdev.brightness_set = ktd2026_set_brightness;
	led->cdev.name = led_conf[channel].name;
	led->cdev.brightness = led_conf[channel].brightness;
	led->cdev.max_brightness = led_conf[channel].max_brightness;
	led->cdev.flags = led_conf[channel].flags;

	ret = led_classdev_register(dev, &led->cdev);

	if (ret < 0) {
		dev_err(dev, "can not register led channel : %d\n", channel);
		return ret;
	}

	ret = sysfs_create_group(&led->cdev.dev->kobj,
			&common_led_attr_group);

	if (ret < 0) {
		dev_err(dev, "can not register sysfs attribute\n");
		return ret;
	}

	return 0;
}


static int __devinit ktd2026_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct ktd2026_data *data;
	int ret, i;

	pr_info("%s\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "need I2C_FUNC_I2C.\n");
		return -ENODEV;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "need I2C_FUNC_SMBUS_BYTE_DATA.\n");
		return -EIO;
	}

	if (client->dev.of_node) {
		data = kzalloc(sizeof(*data), GFP_KERNEL);
		if (!data) {
			dev_err(&client->adapter->dev,
					"failed to allocate driver data.\n");
			return -ENOMEM;
		}
	} else
		data = client->dev.platform_data;

	i2c_set_clientdata(client, data);
	data->client = client;
	b_client = client;

	mutex_init(&data->mutex);

	/* initialize LED */
	/* turn off all leds */
	ktd2026_leds_on(LED_R, LED_EN_OFF, 0);
#if defined(CONFIG_SEC_FACTORY)
#if defined (CONFIG_SEC_ATLANTIC_PROJECT)
	if(batt_id_value == 0)
		ret = 1;
	else
		ret = 0;
#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
	if(get_lcd_attached() == 0)     // When LCD Not connected turning RED LED on
		ret++;;
#endif

	if(jig_power_on_value == 0)
		ret++;
	if(ret == 3)	// case when LCD not connected,battery power on and jig off
		ktd2026_leds_on(LED_R, LED_EN_ON, 30);
#endif		//ATLANTIC FLAG
#endif
	ktd2026_leds_on(LED_G, LED_EN_OFF, 0);
	ktd2026_leds_on(LED_B, LED_EN_OFF, 0);

	ktd2026_set_timerslot_control(0); /* Tslot1 */
	ktd2026_set_period(0);
	ktd2026_set_pwm_duty(PWM1, 0);
	ktd2026_set_pwm_duty(PWM2, 0);
	ktd2026_set_trise_tfall(0, 0, 0);

	for (i = 0; i < MAX_NUM_LEDS; i++) {

		ret = initialize_channel(client, &data->leds[i], i);

		if (ret < 0) {
			dev_err(&client->adapter->dev, "failure on initialization\n");
			goto exit;
		}
		INIT_WORK(&(data->leds[i].brightness_work),
				 ktd2026_led_brightness_work);
	}

	leds_i2c_write_all(client);

#ifdef SEC_LED_SPECIFIC
	led_dev = device_create(sec_class, NULL, 0, data, "led");
	if (IS_ERR(led_dev)) {
		dev_err(&client->dev,
			"Failed to create device for samsung specific led\n");
		ret = -ENODEV;
		goto exit;
	}
	ret = sysfs_create_group(&led_dev->kobj, &sec_led_attr_group);
	if (ret) {
		dev_err(&client->dev,
			"Failed to create sysfs group for samsung specific led\n");
		goto exit;
	}
#endif
	return ret;
exit:
	mutex_destroy(&data->mutex);
	kfree(data);
	return ret;
}

static int __devexit ktd2026_remove(struct i2c_client *client)
{
	struct ktd2026_data *data = i2c_get_clientdata(client);
	int i;
	dev_dbg(&client->adapter->dev, "%s\n", __func__);
	pr_info("%s\n", __func__);
	ktd2026_reset();
#ifdef SEC_LED_SPECIFIC
	sysfs_remove_group(&led_dev->kobj, &sec_led_attr_group);
#endif
	for (i = 0; i < MAX_NUM_LEDS; i++) {
		sysfs_remove_group(&data->leds[i].cdev.dev->kobj,
						&common_led_attr_group);
		led_classdev_unregister(&data->leds[i].cdev);
		cancel_work_sync(&data->leds[i].brightness_work);
	}
	mutex_destroy(&data->mutex);
	kfree(data);
	return 0;
}

static void ktd2026_shutdown(struct i2c_client *i2c)
{
	pr_info("%s: turn off leds\n", __func__);
	ktd2026_reset();
}

static struct i2c_device_id ktd2026_id[] = {
	{"ktd2026", 0},
	{},
};

#ifdef CONFIG_OF
static struct of_device_id ktd2026_match_table[] = {
	{ .compatible = "leds,ktd2026",},
	{ },
};
#else
#define ktd2026_match_table NULL
#endif


MODULE_DEVICE_TABLE(i2c, ktd2026_id);

static struct i2c_driver ktd2026_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ktd2026",
		.of_match_table = ktd2026_match_table,
	},
	.id_table = ktd2026_id,
	.probe = ktd2026_probe,
	.remove = __devexit_p(ktd2026_remove),
	.shutdown = ktd2026_shutdown,
};

static int __init ktd2026_init(void)
{
	return i2c_add_driver(&ktd2026_i2c_driver);
}

static void __exit ktd2026_exit(void)
{
	i2c_del_driver(&ktd2026_i2c_driver);
}

late_initcall(ktd2026_init);
module_exit(ktd2026_exit);

MODULE_DESCRIPTION("KTD2026 LED driver");
MODULE_AUTHOR("Hyoyoung Kim <hyway.kim@samsung.com");
MODULE_LICENSE("GPL");
