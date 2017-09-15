/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/of_gpio.h>
#include <linux/mfd/pm8xxx/pm8921.h>

#include "issp_extern.h"
#include "cypress_touchkey.h"

#ifdef TK_HAS_FIRMWARE_UPDATE
u8 *tk_fw_name = FW_PATH;

/*For HA-3G*/
static u8 fw_ver_file = 0x0;
static u8 md_ver_file = 0x0;

#if defined(CONFIG_KEYBOARD_CYPRESS_TKEY_HL)
u8 module_divider[] = {0, 0xfe, 0xff};
#else
u8 module_divider[] = {0, 0xff};
#endif

u8 *firmware_data;
#endif

#define MODE_NORMAL 0
#define MODE_GLOVE 1
#define MODE_FLIP 2

static int touchkey_keycode[] = { 0, KEY_MENU, KEY_BACK,};
static const int touchkey_count = ARRAY_SIZE(touchkey_keycode);

#if defined(TK_HAS_AUTOCAL)
static int touchkey_autocalibration(struct touchkey_i2c *tkey_i2c);
#endif

static int touchkey_i2c_check(struct touchkey_i2c *tkey_i2c);
static bool is_same_module_class(struct touchkey_i2c *tkey_i2c);

#ifdef TK_INFORM_CHARGER
extern void touchkey_register_callback(void *cb);
#endif

#if defined(TK_USE_4KEY)
static u8 home_sensitivity;
static u8 search_sensitivity;
#endif

static bool touchkey_probe;

static const struct i2c_device_id sec_touchkey_id[] = {
	{"cypress_touchkey", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sec_touchkey_id);


int tkey_gpio_sda;
int tkey_gpio_scl;
int tkey_gpio_int;
int tkey_gpio_rst;
int tkey_gpio_id;
int tkey_gpio_grip;
#ifndef TK_USE_LDO_CONTROL
int tkey_gpio_i2cldo;
int tkey_gpio_vddldo;	
#else
static int cypress_ldo_onoff(bool on);

#if defined(TK_USE_MAX_SUBPM_CONTROL)
struct regulator_bulk_data *cyp_supplies;
enum {
	CYP_IC = 0,		
	CYP_LED,
};

#endif
#endif

extern int get_touchkey_firmware(char *version);
static int touchkey_led_status;
static int touchled_cmd_reversed;

static void touchkey_enable_irq(struct touchkey_i2c *tkey_i2c, int enable)
{
	static int depth = 0;

	mutex_lock(&tkey_i2c->irq_lock);
	if (enable == 1) {
		if (depth == 1)
			enable_irq(tkey_i2c->irq);
		if (depth)
			--depth;
	} else if (enable == 0){
		if (depth == 0)
			disable_irq(tkey_i2c->irq);
		++depth;

		/* forced enable */
	} else {
		if (depth) {
			depth = 0;
			enable_irq(tkey_i2c->irq);
		}
	}
	mutex_unlock(&tkey_i2c->irq_lock);

#ifdef WACOM_IRQ_DEBUG
	printk(KERN_DEBUG"touchkey:Enable %d, depth %d\n", (int)enable, depth);
#endif
}

#ifdef LED_LDO_WITH_REGULATOR
static void change_touch_key_led_voltage(int vol_mv)
{
	struct regulator *tled_regulator;

	tled_regulator = regulator_get(NULL, TKEY_LED_REGULATOR);
	if (IS_ERR(tled_regulator)) {
		pr_err("%s: failed to get resource %s\n", __func__,
		       "touchkey_led");
		return;
	}
	regulator_set_voltage(tled_regulator, vol_mv * 1000, vol_mv * 1000);
	regulator_put(tled_regulator);
}

static ssize_t brightness_control(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	int data;

	if (sscanf(buf, "%d\n", &data) == 1) {
		dev_err(dev, "%s: %d\n", __func__, data);
		change_touch_key_led_voltage(data);
	} else {
		dev_err(dev, "%s Error\n", __func__);
	}

	return size;
}
#endif

static int i2c_touchkey_read(struct touchkey_i2c *tkey_i2c,
		u8 reg, u8 *val, unsigned int len)
{
	struct i2c_client *client = tkey_i2c->client;
	int ret = 0;
	int retry = 3;
#if !defined(TK_USE_GENERAL_SMBUS)
	struct i2c_msg msg[1];
#endif

	mutex_lock(&tkey_i2c->i2c_lock);

	while (retry--) {
		if ((client == NULL) || !(tkey_i2c->enabled)) {
			dev_err(&client->dev, "Touchkey is not enabled. %d\n",
				   __LINE__);
			ret = -ENODEV;
			goto out_i2c_read;
		}

#if defined(TK_USE_GENERAL_SMBUS)
		ret = i2c_smbus_read_i2c_block_data(client,
				reg, len, val);
#else
		msg->addr = client->addr;
		msg->flags = I2C_M_RD;
		msg->len = len;
		msg->buf = val;
		ret = i2c_transfer(client->adapter, msg, 1);
#endif
		if (ret < 0) {
			dev_err(&client->dev, "%s:error(%d)\n", __func__, ret);

			if (!(tkey_i2c->enabled)) {
				dev_err(&client->dev, "Touchkey is not enabled.. %d\n",
					   __LINE__);
				ret = -ENODEV;
				goto out_i2c_read;
			}
			usleep_range(10000, 10000);
			continue;
	}
		break;
	}
out_i2c_read:
	mutex_unlock(&tkey_i2c->i2c_lock);
	return ret;
}

static int i2c_touchkey_write(struct touchkey_i2c *tkey_i2c,
		u8 reg, u8 *val, unsigned int len)
{
	struct i2c_client *client = tkey_i2c->client;
	int ret = 0;
	int retry = 3;
#if !defined(TK_USE_GENERAL_SMBUS)
	struct i2c_msg msg[1];
#endif

	mutex_lock(&tkey_i2c->i2c_lock);

	while (retry--) {
		if ((client == NULL) || !(tkey_i2c->enabled)) {
			dev_err(&client->dev, "Touchkey is not enabled. %d\n",
			       __LINE__);
			ret = -ENODEV;
			goto out_i2c_write;
		}

#if defined(TK_USE_GENERAL_SMBUS)
		ret = i2c_smbus_write_i2c_block_data(client,
				reg, len, val);
#else
		msg->addr = client->addr;
		msg->flags = I2C_M_WR;
		msg->len = len;
		msg->buf = val;
		ret = i2c_transfer(client->adapter, msg, 1);
#endif

		if (ret < 0) {
			dev_err(&client->dev, "%s:error(%d)\n", __func__, ret);
			
			if (!(tkey_i2c->enabled)) {
				dev_err(&client->dev, "Touchkey is not enabled.. %d\n",
					   __LINE__);
				ret = -ENODEV;
				goto out_i2c_write;
			}

			usleep_range(10000, 10000);
			continue;
		}
		break;
	}

out_i2c_write:
	mutex_unlock(&tkey_i2c->i2c_lock);
	return ret;
}

static void cypress_touchkey_interrupt_set_dual(struct touchkey_i2c *tkey_i2c)
{
	struct i2c_client *client = tkey_i2c->client;
	int ret = 0;
	int retry = 5;
	u8 data[3] = {0, };

	ret = is_same_module_class(tkey_i2c);
	if (!ret) {
		dev_err(&client->dev,
			"%s: not support this module 0x%02x\n",
			__func__, tkey_i2c->md_ver_ic);
		return;
	}

	if (tkey_i2c->fw_ver_ic <= CYPRESS_RECENT_BACK_REPORT_FW_VER) {
		dev_err(&client->dev,
			"%s: not support this version 0x%02x\n",
			__func__, tkey_i2c->fw_ver_ic);
		return;
	}
	
	while (retry--) {
		data[0] = TK_CMD_DUAL_DETECTION;
		data[1] = 0x00;
		data[2] = TK_BIT_DETECTION_CONFIRM;

		//ret = i2c_touchkey_write(tkey_i2c, CYPRESS_REG_DETECTION, &data[0], 3);
		ret = i2c_smbus_write_i2c_block_data(client, 0x18, 3, &data[0]);
		if (ret < 0) {
			dev_err(&client->dev,
				"%s: i2c write error. (%d)\n", __func__, ret);
			msleep(30);
			continue;
		}
		msleep(30);

		data[0] = CYPRESS_REG_DETECTION_FLAG;
		
		//ret = i2c_touchkey_read(tkey_i2c, data[0], &data[1], 1);
		ret = i2c_smbus_read_i2c_block_data(client, data[0], 1, &data[1]);
		if (ret < 0) {
			dev_err(&client->dev,
				"%s: i2c read error. (%d)\n", __func__, ret);
			msleep(30);
			continue;
		}

		if (data[1] != 1) {
			dev_err(&client->dev,
				"%s: interrupt set: 0x%X, failed.\n",
				__func__, data[1]);
			continue;
		}

		dev_info(&client->dev,
			"%s: interrupt set: 0x%X\n", __func__, data[1]);
		break;
	}

}


#if defined(TK_HAS_AUTOCAL)
static int touchkey_autocalibration(struct touchkey_i2c *tkey_i2c)
{
	u8 data[6] = { 0, };
	int count = 0;
	int ret = 0;
	unsigned short retry = 0;

	if (wake_lock_active(&tkey_i2c->fw_wakelock)) {
		printk(KERN_DEBUG"touchkey:wakelock active\n");
		return 0;
	}

	while (retry < 3) {
		ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 4);
		if (ret < 0) {
			dev_err(&tkey_i2c->client->dev, "%s: Failed to read Keycode_reg %d times\n",
				__func__, retry);
			return ret;
		}
		dev_dbg(&tkey_i2c->client->dev,
				"data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
				data[0], data[1], data[2], data[3]);

		/* Send autocal Command */
		data[0] = 0x50;
		data[3] = 0x01;

		count = i2c_touchkey_write(tkey_i2c, KEYCODE_REG, data, 4);

		msleep(130);

		/* Check autocal status */
		ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 6);

		if (data[5] & TK_BIT_AUTOCAL) {
			dev_info(&tkey_i2c->client->dev, "%s: Run Autocal\n", __func__);
			break;
		} else {
			dev_err(&tkey_i2c->client->dev,	"%s: Error to set Autocal, retry %d\n",
			       __func__, retry);
		}
		retry = retry + 1;
	}

	if (retry == 3)
		dev_err(&tkey_i2c->client->dev, "%s: Failed to Set the Autocalibration\n", __func__);

	return count;
}
#endif

#if defined(TK_INFORM_CHARGER)
static int touchkey_ta_setting(struct touchkey_i2c *tkey_i2c)
{
	u8 data[6] = { 0, };
	int count = 0;
	int ret = 0;
	unsigned short retry = 0;

	if (wake_lock_active(&tkey_i2c->fw_wakelock)) {
		printk(KERN_DEBUG"touchkey:wakelock active\n");
		return 0;
	}

	while (retry < 3) {
		ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 4);
		if (ret < 0) {
			dev_err(&tkey_i2c->client->dev, "%s: Failed to read Keycode_reg %d times\n",
				__func__, retry);
			return ret;
		}
		dev_dbg(&tkey_i2c->client->dev,
				"data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
				data[0], data[1], data[2], data[3]);

		/* Send autocal Command */

		if (tkey_i2c->charging_mode) {
			dev_info(&tkey_i2c->client->dev, "%s: TA connected!!!\n", __func__);
			data[0] = 0x90;
			data[3] = 0x10;
		} else {
			dev_info(&tkey_i2c->client->dev, "%s: TA disconnected!!!\n", __func__);
			data[0] = 0x90;
			data[3] = 0x20;
		}

		count = i2c_touchkey_write(tkey_i2c, KEYCODE_REG, data, 4);

		msleep(100);
		dev_dbg(&tkey_i2c->client->dev,
				"write data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
				data[0], data[1], data[2], data[3]);

		/* Check autocal status */
		ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 6);

		if (tkey_i2c->charging_mode) {
			if (data[5] & TK_BIT_TA_ON) {
				dev_dbg(&tkey_i2c->client->dev, "%s: TA mode is Enabled\n", __func__);
				break;
			} else {
				dev_err(&tkey_i2c->client->dev, "%s: Error to enable TA mode, retry %d\n",
					__func__, retry);
			}
		} else {
			if (!(data[5] & TK_BIT_TA_ON)) {
				dev_dbg(&tkey_i2c->client->dev, "%s: TA mode is Disabled\n", __func__);
				break;
			} else {
				dev_err(&tkey_i2c->client->dev, "%s: Error to disable TA mode, retry %d\n",
					__func__, retry);
			}
		}
		retry = retry + 1;
	}

	if (retry == 3)
		dev_err(&tkey_i2c->client->dev, "%s: Failed to set the TA mode\n", __func__);

	return count;

}

static void touchkey_ta_cb(struct touchkey_callbacks *cb, bool ta_status)
{
	struct touchkey_i2c *tkey_i2c =
			container_of(cb, struct touchkey_i2c, callbacks);

	tkey_i2c->charging_mode = ta_status;

	if (tkey_i2c->enabled)
		touchkey_ta_setting(tkey_i2c);
}
#endif
#if defined(CONFIG_GLOVE_TOUCH)
static void touchkey_glove_change_work(struct work_struct *work)
{
	u8 data[6] = { 0, };
	int ret = 0;
	unsigned short retry = 0;
	bool value;
	u8 glove_bit;
	struct touchkey_i2c *tkey_i2c =
			container_of(work, struct touchkey_i2c,
			glove_change_work);

#ifdef TKEY_FLIP_MODE
	if (tkey_i2c->ic_mode == MODE_FLIP) {
		dev_info(&tkey_i2c->client->dev,"As flip cover mode enabled, skip glove mode set\n");
		return;
	}
#endif

	mutex_lock(&tkey_i2c->tsk_glove_lock);
	value = tkey_i2c->tsk_cmd_glove;

	while (retry < 3) {
		ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 4);
		if (ret < 0) {
			dev_err(&tkey_i2c->client->dev, "%s: Failed to read Keycode_reg %d times\n",
				__func__, retry);
			goto out_glove_change_work;
		}

		dev_dbg(&tkey_i2c->client->dev,
				"data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
				data[0], data[1], data[2], data[3]);

		if (value) {
			/* Send glove Command */
				data[0] = 0xA0;
				data[3] = 0x30;
		} else {
				data[0] = 0xA0;
				data[3] = 0x40;
		}

		i2c_touchkey_write(tkey_i2c, KEYCODE_REG, data, 4);
		msleep(130);

		dev_dbg(&tkey_i2c->client->dev,
				"data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
				data[0], data[1], data[2], data[3]);

		/* Check autocal status */
		ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 6);

		glove_bit = !!(data[5] & TK_BIT_GLOVE);

		if (value == glove_bit) {
			dev_info(&tkey_i2c->client->dev, "%s:Glove mode is %s\n",
				__func__, value ? "enabled" : "disabled");
			tkey_i2c->ic_mode = value ? MODE_GLOVE : MODE_NORMAL;
			break;
		} else
			dev_err(&tkey_i2c->client->dev, "%s:Error to set glove_mode val %d, bit %d, retry %d\n",
				__func__, value, glove_bit, retry);

		retry = retry + 1;
	}
	if (retry == 3)
		dev_err(&tkey_i2c->client->dev, "%s: Failed to set the glove mode\n", __func__);

out_glove_change_work:
	mutex_unlock(&tkey_i2c->tsk_glove_lock);
}

static struct touchkey_i2c *tkey_i2c_global;

void touchkey_glovemode(int on)
{
	struct touchkey_i2c *tkey_i2c = tkey_i2c_global;

	if (!touchkey_probe) {
		dev_err(&tkey_i2c->client->dev, "%s: Touchkey is not probed\n", __func__);
		return;
	}
	if (!tkey_i2c->enabled) {
		dev_err(&tkey_i2c->client->dev, "%s: Touchkey is not enabled\n", __func__);
		return ;
	}
	if (wake_lock_active(&tkey_i2c->fw_wakelock)) {
		printk(KERN_DEBUG"touchkey:%s, wakelock active\n", __func__);
		return ;
	}

	cancel_work_sync(&tkey_i2c->glove_change_work);

	/* protect duplicated execution */
	if (tkey_i2c->ic_mode == MODE_FLIP) {
		dev_info(&tkey_i2c->client->dev, "%s pass. flip enabled\n",
			__func__);
		goto end_glovemode;
	}
	if (on == (tkey_i2c->ic_mode == MODE_GLOVE)) {
		dev_info(&tkey_i2c->client->dev, "%s pass. cmd %d, cur mode %d\n",
			__func__, on, tkey_i2c->ic_mode);

		if(!on) dump_stack();
		goto end_glovemode;
	}

	tkey_i2c->tsk_cmd_glove = on;
	schedule_work(&tkey_i2c->glove_change_work);

	dev_info(&tkey_i2c->client->dev, "Touchkey glove %s\n", on ? "On" : "Off");

 end_glovemode:
	return ;
}
#endif

#ifdef TKEY_FLIP_MODE
void touchkey_flip_cover(int value)
{
	struct touchkey_i2c *tkey_i2c = tkey_i2c_global;
	u8 data[6] = { 0, };
	int ret = 0;
	unsigned short retry = 0;
	u8 flip_status;

	tkey_i2c->enabled_flip = value;

	if (!touchkey_probe) {
		dev_err(&tkey_i2c->client->dev, "%s: Touchkey is not probed\n", __func__);
		return;
	}
	if (!tkey_i2c->enabled) {
		dev_err(&tkey_i2c->client->dev, "%s: Touchkey is not enabled\n", __func__);
		if (value)
			tkey_i2c->ic_mode = MODE_FLIP;
		else
			tkey_i2c->ic_mode = MODE_NORMAL;
		return;
	}
	if (wake_lock_active(&tkey_i2c->fw_wakelock)) {
		printk(KERN_DEBUG"touchkey:%s, wakelock active\n", __func__);
		return ;
	}

#if 0
	if (tkey_i2c->firmware_id != TK_MODULE_20065) {
		dev_err(&tkey_i2c->client->dev,
				"%s: Do not support old module\n",
				__func__);
		return;
	}
#endif
	mutex_lock(&tkey_i2c->tsk_glove_lock);
	while (retry < 3) {
		ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 4);
		if (ret < 0) {
			dev_err(&tkey_i2c->client->dev, "%s: Failed to read Keycode_reg %d times\n",
				__func__, retry);
			goto out_flip_cover;
		}

		dev_dbg(&tkey_i2c->client->dev,
				"data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
				data[0], data[1], data[2], data[3]);

		if (value == 1) {
			/* Send filp mode Command */
				data[0] = 0xA0;
				data[3] = 0x50;
		} else {
				data[0] = 0xA0;
				data[3] = 0x40;
		}

		i2c_touchkey_write(tkey_i2c, KEYCODE_REG, data, 4);
		msleep(130);

		dev_dbg(&tkey_i2c->client->dev,
				"data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
				data[0], data[1], data[2], data[3]);

		/* Check status */
		ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 6);
		flip_status = !!(data[5] & TK_BIT_FLIP);

		dev_dbg(&tkey_i2c->client->dev,
				"data[5]=%x",data[5] & TK_BIT_FLIP);

		if (value == flip_status) {
			dev_info(&tkey_i2c->client->dev, "%s: Flip mode is %s\n", __func__, flip_status ? "enabled" : "disabled");
			tkey_i2c->ic_mode = flip_status ? MODE_FLIP : MODE_NORMAL;
			break;
		} else
			dev_err(&tkey_i2c->client->dev, "%s: Error to set Flip mode, val %d, flip bit %d, retry %d\n",
				__func__, value, flip_status, retry);

		retry = retry + 1;
	}

	if (retry == 3) {
		dev_err(&tkey_i2c->client->dev, "%s: Failed to set the Flip mode\n", __func__);
		if (value == 0) {
			tkey_i2c->pdata->power_on(0);
			usleep_range(1000, 1000);
			tkey_i2c->pdata->power_on(1);
			msleep(300);
			printk(KERN_DEBUG"touchkey:%s, reset ic\n", __func__);

			/* CYPRESS Firmware setting interrupt type : dual or single interrupt */
			cypress_touchkey_interrupt_set_dual(tkey_i2c);

		}
	}

out_flip_cover:
	mutex_unlock(&tkey_i2c->tsk_glove_lock);
}
#endif

#ifdef TKEY_GRIP_MODE
void touchkey_grip_mode_ctr(struct touchkey_i2c *tkey_i2c)
{
	u8 buf = TKEY_CMD_GRIP;
	int ret;
	int retry = 3;

	mutex_lock(&tkey_i2c->grip_mode_lock);	

	ret = wake_lock_active(&tkey_i2c->fw_wakelock);
	if (unlikely(ret)) {
		printk(KERN_DEBUG"touchkey:%s, wakelock active\n", __func__);
		goto out_grip_mode_ctr;
	}
	if (tkey_i2c->ic_mode == tkey_i2c->grip_mode) {
		printk(KERN_DEBUG"touchkey:pass grip cmd. ic mode %d, cmd %d\n",
				tkey_i2c->ic_mode, tkey_i2c->grip_mode);
		goto out_grip_mode_ctr;
	}

	if (tkey_i2c->grip_mode) {
		tkey_i2c->pdata->power_on(1);
		msleep(300); //50);
		
		/* set as grip mode */
		/* to avoid i2c write lock */
		while (retry--) {
			ret = i2c_smbus_write_i2c_block_data(tkey_i2c->client, KEYCODE_REG, 1, &buf);
			if (ret < 0) {
				printk(KERN_DEBUG"touchkey:failed to set grip mode ret(%d),retry(%d)\n",
							ret, retry);
				tkey_i2c->pdata->power_on(0);
				msleep(30);
				tkey_i2c->pdata->power_on(1);
				msleep(300); //50);
				continue;
			}
			break;
		}
		if (ret < 0) {
			tkey_i2c->pdata->power_on(0);
			goto out_grip_mode_ctr;
		}
		tkey_i2c->ic_mode = MODE_GRIP;

		/* CYPRESS Firmware setting interrupt type : dual or single interrupt */
		cypress_touchkey_interrupt_set_dual(tkey_i2c);

	} else {
		tkey_i2c->pdata->power_on(0);
		tkey_i2c->ic_mode = MODE_NORMAL;
	}
	printk(KERN_DEBUG"touchkey:set as %s mode\n",
		(tkey_i2c->ic_mode == MODE_GRIP) ? "grip" : "normal");

 out_grip_mode_ctr:
	mutex_unlock(&tkey_i2c->grip_mode_lock);
}
#endif

static int touchkey_enable_status_update(struct touchkey_i2c *tkey_i2c)
{
	unsigned char data = 0x40;
	int ret;

	ret = i2c_touchkey_write(tkey_i2c, KEYCODE_REG, &data, 1);
	if (ret < 0) {
		dev_err(&tkey_i2c->client->dev, "%s, err(%d)\n", __func__, ret);
		tkey_i2c->status_update = false;
		return ret;
	}

	tkey_i2c->status_update = true;
	dev_dbg(&tkey_i2c->client->dev, "%s\n", __func__);

	msleep(20);

	return 0;
}

#if defined(TK_HAS_AUTOCAL)
enum {
	TK_CMD_READ_THRESHOLD = 0,
	TK_CMD_READ_IDAC,
	TK_CMD_READ_SENSITIVITY,
	TK_CMD_READ_RAW,
};

const u8 fac_reg_index[] = {
	CYPRESS_REG_THRESHOLD,
	CYPRESS_REG_IDAC,
	CYPRESS_REG_DIFF,
	CYPRESS_REG_RAW,
};

struct FAC_CMD {
	u8 cmd;
	u8 opt1; // 0, 1, 2, 3
	u16 result;
};

static u8 touchkey_get_read_size(u8 cmd)
{
	switch (cmd) {
	case TK_CMD_READ_RAW:
	case TK_CMD_READ_SENSITIVITY:
		return 2;
	case TK_CMD_READ_IDAC:
	case TK_CMD_READ_THRESHOLD:
		return 1;
		break;
	default:
		break;
	}
	return 0;
}

static int touchkey_fac_read_data(struct device *dev,
		char *buf, struct FAC_CMD *cmd)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int ret;
	u8 size;
	u8 base_index;
	u8 data[26] = { 0, };
	int i;
	u16 max_val = 0;

	if (unlikely(!tkey_i2c->status_update)) {
		ret = touchkey_enable_status_update(tkey_i2c);
		if (ret < 0)
			goto out_fac_read_data;
	}

	size = touchkey_get_read_size(cmd->cmd);
	if (size == 0) {
		printk(KERN_DEBUG"touchkey:wrong size %d\n", size);
		goto out_fac_read_data;
	}

	if (cmd->opt1 > 4) {
		printk(KERN_DEBUG"touchkey:wrong opt1 %d\n", cmd->opt1);
		goto out_fac_read_data;
	}

	base_index = fac_reg_index[cmd->cmd] + size * cmd->opt1;
	if (base_index > 25) {
		printk(KERN_DEBUG"touchkey:wrong index %d, cmd %d, size %d, opt1 %d\n",
			base_index, cmd->cmd, size, cmd->opt1);
		goto out_fac_read_data;
	}
	
	ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, base_index + size);
	if (ret <  0) {
		printk(KERN_DEBUG"touchkey:i2c read failed\n");
		goto out_fac_read_data;
	}

	/* make value */
	cmd->result = 0;
	for (i = size - 1; i >= 0; --i) {
		cmd->result = cmd->result | (data[base_index++] << (8 * i));
		max_val |= 0xff << (8 * i);
	}

	/* garbage check */
	if (unlikely(cmd->result == max_val)) {
		printk(KERN_DEBUG"touchkey:cmd %d opt1 %d, max value\n", cmd->cmd, cmd->opt1);
		cmd->result = 0;
	}

 out_fac_read_data:
	return sprintf(buf, "%d\n", cmd->result);
}

static ssize_t touchkey_raw_data0_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_RAW, 0, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_raw_data1_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_RAW, 1, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}
#if !defined(TK_USE_RECENT)
static ssize_t touchkey_raw_data2_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_RAW, 2, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_raw_data3_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_RAW, 3, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}
#endif
static ssize_t touchkey_idac0_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_IDAC, 0, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_idac1_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_IDAC, 1, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}
#if !defined(TK_USE_RECENT)
static ssize_t touchkey_idac2_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_IDAC, 2, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_idac3_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_IDAC, 3, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}
#endif
static ssize_t touchkey_threshold_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_THRESHOLD, 0, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}
#endif

#ifdef TOUCHKEY_BOOSTER
static void touchkey_change_dvfs_lock(struct work_struct *work)
{
	struct touchkey_i2c *tkey_i2c =
			container_of(work,
				struct touchkey_i2c, work_dvfs_chg.work);
	int retval = 0;

	mutex_lock(&tkey_i2c->dvfs_lock);

	retval = set_freq_limit(DVFS_TOUCH_ID, tkey_i2c->dvfs_freq);
	if (retval < 0)
		pr_info("%s: booster change failed(%d).\n",
			__func__, retval);

	tkey_i2c->dvfs_lock_status = false;
	mutex_unlock(&tkey_i2c->dvfs_lock);
}

static void touchkey_set_dvfs_off(struct touchkey_i2c *tkey_i2c)
{
	int retval;

	mutex_lock(&tkey_i2c->dvfs_lock);

	retval = set_freq_limit(DVFS_TOUCH_ID, -1);
	if (retval < 0){
		pr_err("%s: booster stop failed(%d).\n",
					__func__, retval);
		tkey_i2c->dvfs_lock_status = false;
	}
	else
		tkey_i2c->dvfs_lock_status = true;

	mutex_unlock(&tkey_i2c->dvfs_lock);
}

static void touchkey_set_dvfs_off_work(struct work_struct *work)
{
	struct touchkey_i2c *tkey_i2c =
				container_of(work,
					struct touchkey_i2c, work_dvfs_off.work);

	touchkey_set_dvfs_off(tkey_i2c);
}

static void touchkey_set_dvfs_lock(struct touchkey_i2c *tkey_i2c,
					uint32_t on)
{
	int retval;
	if (TKEY_BOOSTER_DISABLE == tkey_i2c->dvfs_boost_mode)
		return;

	mutex_lock(&tkey_i2c->dvfs_lock);
	if (on == 0) {
		cancel_delayed_work(&tkey_i2c->work_dvfs_chg);

		if (tkey_i2c->dvfs_lock_status) {
			retval = set_freq_limit(DVFS_TOUCH_ID, tkey_i2c->dvfs_freq);
			if (retval < 0)
				pr_info("%s: cpu first lock failed(%d)\n", __func__, retval);
			tkey_i2c->dvfs_lock_status = false;
		}

		schedule_delayed_work(&tkey_i2c->work_dvfs_off,
			msecs_to_jiffies(TKEY_BOOSTER_OFF_TIME));

	} else if (on == 1) {
		cancel_delayed_work(&tkey_i2c->work_dvfs_off);
		schedule_delayed_work(&tkey_i2c->work_dvfs_chg,
			msecs_to_jiffies(TKEY_BOOSTER_CHG_TIME));

	} else if (on == 2) {
		if (tkey_i2c->dvfs_lock_status) {
			cancel_delayed_work(&tkey_i2c->work_dvfs_off);
			cancel_delayed_work(&tkey_i2c->work_dvfs_chg);
			schedule_work(&tkey_i2c->work_dvfs_off.work);
		}
	}
	mutex_unlock(&tkey_i2c->dvfs_lock);
}

static int touchkey_init_dvfs(struct touchkey_i2c *tkey_i2c)
{
	mutex_init(&tkey_i2c->dvfs_lock);

	INIT_DELAYED_WORK(&tkey_i2c->work_dvfs_off, touchkey_set_dvfs_off_work);
	INIT_DELAYED_WORK(&tkey_i2c->work_dvfs_chg, touchkey_change_dvfs_lock);

	tkey_i2c->dvfs_boost_mode = TKEY_BOOSTER_LEVEL2;
	tkey_i2c->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
	tkey_i2c->dvfs_lock_status = true;
	return 0;
}
#endif

#if defined(TK_HAS_FIRMWARE_UPDATE)
/* To check firmware compatibility */
int get_module_class(u8 ver)
{
	static int size = ARRAY_SIZE(module_divider);
	int i;

	if (size == 2)
		return 0;

	for (i = size - 1; i > 0; --i) {
		if (ver < module_divider[i] &&
			ver >= module_divider[i-1])
			return i;
	}

	return 0;
}

bool is_same_module_class(struct touchkey_i2c *tkey_i2c)
{
	int class_ic, class_file;

	if (md_ver_file == tkey_i2c->md_ver_ic)
		return true;
	
	class_file = get_module_class(md_ver_file);
	class_ic = get_module_class(tkey_i2c->md_ver_ic);

	printk(KERN_DEBUG"touchkey:module class, IC %d, File %d\n", class_ic, class_file);

	if (class_file == class_ic)
		return true;

	return false;
}

int tkey_load_fw_built_in(struct touchkey_i2c *tkey_i2c)
{
	int retry = 3;
	int ret;

	while (retry--) {
		ret =
			request_firmware(&tkey_i2c->firm_data, tk_fw_name,
			&tkey_i2c->client->dev);
		if (ret < 0) {
			printk(KERN_ERR
				"touchkey:Unable to open firmware. ret %d retry %d\n",
				ret, retry);
			continue;
		}
		break;
	}

	if (ret < 0)
		return ret;

	tkey_i2c->fw_img = (struct fw_image *)tkey_i2c->firm_data->data;

	return ret;
}

int tkey_load_fw_sdcard(struct touchkey_i2c *tkey_i2c)
{
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int ret = 0;
	u8 *ums_data = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(TKEY_FW_PATH, O_RDONLY, S_IRUSR);

	if (IS_ERR(fp)) {
		printk(KERN_ERR "touchkey:failed to open %s.\n", TKEY_FW_PATH);
		ret = -ENOENT;
		set_fs(old_fs);
		return ret;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	printk(KERN_NOTICE
		"touchkey:start, file path %s, size %ld Bytes\n",
		TKEY_FW_PATH, fsize);

	ums_data = kmalloc(fsize, GFP_KERNEL);
	if (IS_ERR(ums_data)) {
		printk(KERN_ERR
			"touchkey:%s, kmalloc failed\n", __func__);
		ret = -EFAULT;
		goto malloc_error;
	}

	nread = vfs_read(fp, (char __user *)ums_data,
		fsize, &fp->f_pos);
	printk(KERN_NOTICE "touchkey:nread %ld Bytes\n", nread);
	if (nread != fsize) {
		printk(KERN_ERR
			"touchkey:failed to read firmware file, nread %ld Bytes\n",
			nread);
		ret = -EIO;
		kfree(ums_data);
		goto read_err;
	}

	filp_close(fp, current->files);
	set_fs(old_fs);

	tkey_i2c->fw_img = (struct fw_image *)ums_data;

	return 0;

read_err:
malloc_error:
	filp_close(fp, current->files);
	set_fs(old_fs);
	return ret;
}

void touchkey_unload_fw(struct touchkey_i2c *tkey_i2c)
{
	switch (tkey_i2c->fw_path) {
	case FW_BUILT_IN:
		release_firmware(tkey_i2c->firm_data);
		break;
	case FW_IN_SDCARD:
		kfree(tkey_i2c->fw_img);
		break;
	default:
		break;
	}

	tkey_i2c->fw_path = FW_NONE;

	tkey_i2c->fw_img = NULL;
	tkey_i2c->firm_data = NULL;
	firmware_data = NULL;
}

int touchkey_load_fw(struct touchkey_i2c *tkey_i2c, u8 fw_path)
{
	int ret = 0;
	struct fw_image *fw_img;

	switch (fw_path) {
	case FW_BUILT_IN:
		ret = tkey_load_fw_built_in(tkey_i2c);
		break;
	case FW_IN_SDCARD:
		ret = tkey_load_fw_sdcard(tkey_i2c);
		break;
	default:
		printk(KERN_DEBUG"touchkey:unknown path(%d)\n", fw_path);
		goto err_load_fw;
	}

	if (ret < 0)
		goto err_load_fw;

	fw_img = tkey_i2c->fw_img;

	/* header check  */
	if (fw_img->hdr_ver == 1 && fw_img->hdr_len == 16) {
		printk(KERN_DEBUG"touchkey:detect hdr\n");
		firmware_data = (u8 *)fw_img->data;
		fw_ver_file = fw_img->first_fw_ver;
		md_ver_file = fw_img->second_fw_ver;
	} else {
		printk(KERN_DEBUG"touchkey:no hdr\n");
		firmware_data = (u8 *)fw_img;
	}

	return ret;
 err_load_fw:
	firmware_data = NULL;
	return ret;
}
#ifdef CYPRESS_CRC_CHECK
int tkey_crc_check(struct touchkey_i2c *tkey_i2c)
{
	char data[3] = {0, };
	int retry = 3;
	int ret;
	unsigned short chk_ic;

	while (retry--) {
		ret = i2c_touchkey_read(tkey_i2c, CYPRESS_REG_CRC, data, 2);
		if (ret < 0) {
			dev_err(&tkey_i2c->client->dev, "retry crc read(%d)\n", retry);
			msleep(10);
			continue;
		}
		break;
	}
	if (ret < 0) {
		dev_err(&tkey_i2c->client->dev, "Failed to read CRC\n");
		return 0;
	}

	chk_ic = (data[0] << 8) | data[1];
	dev_info(&tkey_i2c->client->dev, "checksum, ic:%#x, bin:%#x\n",
				chk_ic, tkey_i2c->fw_img->checksum);

	return (tkey_i2c->fw_img->checksum == chk_ic);
}
#endif

int touchkey_fw_update(struct touchkey_i2c *tkey_i2c, u8 fw_path, bool bforced)
{
	int ret;

	ret = touchkey_load_fw(tkey_i2c, fw_path);
	if (ret < 0) {
		printk(KERN_DEBUG"touchkey:failed to load fw data\n");
		return ret;
	}
	tkey_i2c->fw_path = fw_path;

	/* f/w info */
	dev_info(&tkey_i2c->client->dev, "fw ver %#x, new fw ver %#x\n",
		tkey_i2c->fw_ver_ic, fw_ver_file);
	dev_info(&tkey_i2c->client->dev, "module ver %#x, new module ver %#x\n",
		tkey_i2c->md_ver_ic, md_ver_file);
	dev_info(&tkey_i2c->client->dev, "new checksum %#x\n",
		tkey_i2c->fw_img->checksum);

	if (unlikely(bforced)) {
		printk(KERN_DEBUG"touchkey:forced update\n");
		goto run_fw_update;
	}

	/* check update condition */
	ret = is_same_module_class(tkey_i2c);
	if (ret == false)
		goto out_fw_update;

#if defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	if (tkey_i2c->fw_ver_ic != fw_ver_file)
		goto run_fw_update;
#else
	if (tkey_i2c->fw_ver_ic < fw_ver_file)
		goto run_fw_update;
	if (tkey_i2c->fw_ver_ic > fw_ver_file)
		goto out_fw_update;
#endif

	/* do not need to update f/w */
	/* if bin = ic version, check checksum */
#ifdef CYPRESS_CRC_CHECK
	ret = tkey_crc_check(tkey_i2c);
	if (ret == false) {
		dev_info(&tkey_i2c->client->dev, "crc error, run fw update\n");
		goto run_fw_update;
	}else{
		dev_info(&tkey_i2c->client->dev, "crc ok, pass fw update\n");
		goto out_fw_update;
	}
#endif
	tkey_i2c->do_checksum = true;
	//	goto run_fw_update;

out_fw_update:
	/* pass update */
	dev_info(&tkey_i2c->client->dev, "pass fw update\n");
	touchkey_unload_fw(tkey_i2c);

 run_fw_update:
	queue_work(tkey_i2c->fw_wq, &tkey_i2c->update_work);
	return 0;
}

static void touchkey_i2c_update_work(struct work_struct *work)
{
	struct touchkey_i2c *tkey_i2c =
		container_of(work, struct touchkey_i2c, update_work);
	int ret = 0;
	int retry = 3;

	touchkey_enable_irq(tkey_i2c, false);
	wake_lock(&tkey_i2c->fw_wakelock);

	if (tkey_i2c->fw_path == FW_NONE)
		goto end_fw_update;

	printk(KERN_DEBUG"touchkey:%s\n", __func__);
	tkey_i2c->update_status = TK_UPDATE_DOWN;

	while (retry--) {
		ret = ISSP_main(tkey_i2c);
		if (ret != 0) {
			msleep(50);
			dev_err(&tkey_i2c->client->dev, "failed to update f/w. retry\n");
			continue;
		}

		dev_info(&tkey_i2c->client->dev, "finish f/w update\n");
		tkey_i2c->update_status = TK_UPDATE_PASS;
		break;
	}
	if (retry <= 0 && ret != 0) {
		tkey_i2c->pdata->power_on(0);
		tkey_i2c->update_status = TK_UPDATE_FAIL;
		dev_err(&tkey_i2c->client->dev, "failed to update f/w\n");
		goto err_fw_update;
	}else{
		msleep(50);
		tkey_i2c->pdata->power_on(0);
		msleep(10);
		tkey_i2c->pdata->power_on(1);
		msleep(300);
	}

	ret = touchkey_i2c_check(tkey_i2c);
	if (ret < 0)
		goto err_fw_update;

	dev_info(&tkey_i2c->client->dev, "f/w ver = %#X, module ver = %#X\n",
		tkey_i2c->fw_ver_ic, tkey_i2c->md_ver_ic);

 err_fw_update:
	touchkey_unload_fw(tkey_i2c);
 end_fw_update: 
	wake_unlock(&tkey_i2c->fw_wakelock);
	if (ret < 0) {
		printk(KERN_DEBUG"touchkey:do not enable irq\n");
		return ;
	}
#if defined(TK_HAS_AUTOCAL)
	//touchkey_autocalibration(tkey_i2c);
#endif
	touchkey_enable_irq(tkey_i2c, true);
}
#endif

int PrevMenuValue = 0xFF, PrevBackValue = 0xFF; /*Dummy value*/

static irqreturn_t touchkey_interrupt(int irq, void *dev_id)
{
	struct touchkey_i2c *tkey_i2c = dev_id;
	u8 data[3];
	int ret;
	int keycode_type = 0;
	int pressed = 0;
	bool ic_mode;
	int keycode_data[3];
	int i;

#if !defined(CONFIG_SEC_S_PROJECT)
	int CurrMenuValue, CurrBackValue;
#endif
	if (unlikely(!touchkey_probe)) {
		dev_err(&tkey_i2c->client->dev, "%s: Touchkey is not probed\n", __func__);
		return IRQ_HANDLED;
	}

	#ifdef CONFIG_GLOVE_TOUCH
	ic_mode = tkey_i2c->ic_mode;
	#else
	ic_mode = 0;
	#endif

	/* L OS support Screen Pinning feature.
	  * "recent + back key" event is CombinationKey for exit Screen Pinning mode.
	  * If touchkey firmware version is higher than CYPRESS_RECENT_BACK_REPORT_FW_VER,
	  * touchkey can make interrupt "back key" and "recent key" in same time.
	  * lower version firmware can make interrupt only one key event.
	  */

	if (tkey_i2c->fw_ver_ic >= CYPRESS_RECENT_BACK_REPORT_FW_VER) {
		ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 1);
		if (ret < 0){
			dev_err(&tkey_i2c->client->dev, "%s: i2c read fail, ret:%d\n", __func__, ret);
			return IRQ_HANDLED;
		}

		keycode_data[1] = data[0] & 0x3;
		keycode_data[2] = (data[0] >> 2) & 0x3;

		for (i = 1; i < 3; i++) {
			if (keycode_data[i]) {
				input_report_key(tkey_i2c->input_dev, touchkey_keycode[i], (keycode_data[i] % 2));
				#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				dev_info(&tkey_i2c->client->dev, "keycode: %d %s %d\n", touchkey_keycode[i],
					(keycode_data[i] % 2) ? "P" : "R", ic_mode);
				#else
				dev_info(&tkey_i2c->client->dev, " %s %d\n",
					(keycode_data[i] % 2) ? "P" : "R", ic_mode);
				#endif
				if (keycode_data[i] % 2) pressed++;
			}
		}
		input_sync(tkey_i2c->input_dev);

		#ifdef TOUCHKEY_BOOSTER
		touchkey_set_dvfs_lock(tkey_i2c, !!pressed);
		#endif

	} else { /* old version. */
		ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 3);
		if (ret < 0){
			dev_err(&tkey_i2c->client->dev, "%s: i2c read fail, ret:%d\n", __func__, ret);
			return IRQ_HANDLED;
		}

		#if !defined(CONFIG_SEC_S_PROJECT)
		if (tkey_i2c->fw_ver_ic >= 0x07) {
			keycode_type = (data[0] & TK_BIT_KEYCODE);
			if (keycode_type < 0 || keycode_type > touchkey_count) {
				dev_dbg(&tkey_i2c->client->dev, "keycode_type err\n");
				return IRQ_HANDLED;
			}

		CurrMenuValue = (data[0] & 0x1);
		CurrBackValue = (data[0] & 0x2) >> 1;

		if (CurrMenuValue != PrevMenuValue) {
			input_report_key(tkey_i2c->input_dev, KEY_MENU, CurrMenuValue);
			keycode_type = 1;
				pressed = CurrMenuValue;
		}
		if (CurrBackValue != PrevBackValue) {
			input_report_key(tkey_i2c->input_dev, KEY_BACK, CurrBackValue);
			keycode_type = 2;
				pressed = CurrBackValue;
		}

		input_sync(tkey_i2c->input_dev);

		/* Backup data*/
		PrevMenuValue = CurrMenuValue;
		PrevBackValue = CurrBackValue;
		}
		else
		#endif
		{
			keycode_type = (data[0] & TK_BIT_KEYCODE);
			pressed = !(data[0] & TK_BIT_PRESS_EV);

			if (keycode_type <= 0 || keycode_type >= touchkey_count) {
				dev_dbg(&tkey_i2c->client->dev, "keycode_type err\n");
				return IRQ_HANDLED;
			}

			input_report_key(tkey_i2c->input_dev,
					 touchkey_keycode[keycode_type], pressed);
			input_sync(tkey_i2c->input_dev);
		}

		#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		dev_info(&tkey_i2c->client->dev, "keycode:%d pressed:%d %d\n",
			touchkey_keycode[keycode_type], pressed, ic_mode);
		#else
		dev_info(&tkey_i2c->client->dev, "pressed:%d %d\n",
			pressed, ic_mode);
		#endif
		#ifdef TOUCHKEY_BOOSTER
		touchkey_set_dvfs_lock(tkey_i2c, !!pressed);
		#endif
	}
	return IRQ_HANDLED;
}

static int touchkey_stop(struct touchkey_i2c *tkey_i2c)
{
	int i;

	mutex_lock(&tkey_i2c->lock);

	if (!tkey_i2c->enabled) {
		dev_err(&tkey_i2c->client->dev, "Touch key already disabled\n");
		goto err_stop_out;
	}
	if (wake_lock_active(&tkey_i2c->fw_wakelock)) {
		printk(KERN_DEBUG"touchkey:wake_lock active\n");
		goto err_stop_out;
	}

	touchkey_enable_irq(tkey_i2c, false);

	/* release keys */
	for (i = 1; i < touchkey_count; ++i) {
		input_report_key(tkey_i2c->input_dev,
				 touchkey_keycode[i], 0);
	}
	input_sync(tkey_i2c->input_dev);

#if defined(CONFIG_GLOVE_TOUCH)
	/*cancel or waiting before pwr off*/
	cancel_work_sync(&tkey_i2c->glove_change_work);
	if (tkey_i2c->ic_mode != MODE_FLIP)
		tkey_i2c->ic_mode = MODE_NORMAL;
#endif
#ifdef TKEY_GRIP_MODE
	if (tkey_i2c->grip_mode)
		touchkey_grip_mode_ctr(tkey_i2c);
	else {
		tkey_i2c->pdata->led_power_on(0);
		tkey_i2c->pdata->power_on(0);
	}
#else
	tkey_i2c->pdata->led_power_on(0);
	tkey_i2c->pdata->power_on(0);
#endif
	
	tkey_i2c->enabled = false;
	tkey_i2c->status_update = false;
#ifdef TOUCHKEY_BOOSTER
	touchkey_set_dvfs_lock(tkey_i2c, 2);
#endif

 err_stop_out:
	mutex_unlock(&tkey_i2c->lock);

	return 0;
}

static int touchkey_start(struct touchkey_i2c *tkey_i2c)
{
#ifdef TEST_JIG_MODE
	unsigned char get_touch = 0x40;
#endif

	mutex_lock(&tkey_i2c->lock);

	if (tkey_i2c->enabled) {
		dev_err(&tkey_i2c->client->dev, "Touch key already enabled\n");
		goto err_start_out;
	}
	if (wake_lock_active(&tkey_i2c->fw_wakelock)) {
		printk(KERN_DEBUG"touchkey:wake_lock active\n");
		goto err_start_out;
	}

#ifdef TKEY_GRIP_MODE
	tkey_i2c->ic_mode = MODE_NORMAL;
	tkey_i2c->pdata->power_on(0);
	msleep(10);
#endif

    // Initialize variable
    PrevMenuValue = 0xFF;    // Dummy value
    PrevBackValue = 0xFF;    // Dummy value

	/* enable ldo11 */
	tkey_i2c->pdata->power_on(1);
	msleep(300); //50
	
	/* CYPRESS Firmware setting interrupt type : dual or single interrupt */
	cypress_touchkey_interrupt_set_dual(tkey_i2c);

#if !defined(TK_LED_DIRECT_CONTORL)
	tkey_i2c->pdata->led_power_on(1);
	tkey_i2c->enabled = true;

#if defined(TK_HAS_AUTOCAL)
	//touchkey_autocalibration(tkey_i2c);
#endif

	if (touchled_cmd_reversed) {
		touchled_cmd_reversed = 0;
		i2c_touchkey_write(tkey_i2c, KEYCODE_REG, (u8 *) &touchkey_led_status, 1);
		dev_err(&tkey_i2c->client->dev, "%s: Turning LED is reserved\n", __func__);
		msleep(30);
	}
#else
	tkey_i2c->enabled = true;
	if (touchled_cmd_reversed) {
		touchled_cmd_reversed = 0;
		tkey_i2c->pdata->led_power_on(touchkey_led_status);
		dev_err(&tkey_i2c->client->dev, "%s: Turning LED is reserved\n", __func__);
	}
	#if defined(TK_HAS_AUTOCAL)
	//touchkey_autocalibration(tkey_i2c);
	#endif

#endif


#ifdef TEST_JIG_MODE
	i2c_touchkey_write(tkey_i2c, KEYCODE_REG, &get_touch, 1);
#endif

#if defined(TK_INFORM_CHARGER)
	touchkey_ta_setting(tkey_i2c);
#endif

	if (tkey_i2c->ic_mode == MODE_FLIP) {
		tkey_i2c->ic_mode = MODE_NORMAL;
		touchkey_flip_cover(1);
	}
#if defined(CONFIG_GLOVE_TOUCH)
	if (tkey_i2c->tsk_enable_glove_mode)
		touchkey_glovemode(1);
#endif
	touchkey_enable_irq(tkey_i2c, true);
 err_start_out:
	mutex_unlock(&tkey_i2c->lock);

	return 0;
}

#ifdef TK_USE_OPEN_DWORK
static void touchkey_open_work(struct work_struct *work)
{
	int retval;
	struct touchkey_i2c *tkey_i2c =
			container_of(work, struct touchkey_i2c,
			open_work.work);

	if (tkey_i2c->enabled) {
		dev_err(&tkey_i2c->client->dev, "Touch key already enabled\n");
		return;
	}

	retval = touchkey_start(tkey_i2c);
	if (retval < 0)
		dev_err(&tkey_i2c->client->dev,
				"%s: Failed to start device\n", __func__);
}
#endif

#define USE_OPEN_CLOSE
static int touchkey_input_open(struct input_dev *dev)
{
	struct touchkey_i2c *data = input_get_drvdata(dev);
	int ret;

	if (touchkey_probe != true) {
		printk(KERN_ERR "%s, not yet init. \n", __func__);
		return 0;
	}

#ifdef TK_USE_OPEN_DWORK
	schedule_delayed_work(&data->open_work,
					msecs_to_jiffies(TK_OPEN_DWORK_TIME));
#else
#ifdef TKEY_GRIP_MODE
	data->pwr_flag = true;
#endif
	gpio_tlmm_config(GPIO_CFG(tkey_gpio_scl, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(tkey_gpio_sda, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	ret = touchkey_start(data);
	if (ret)
		goto err_open;
#endif

	dev_info(&data->client->dev, "%s\n", __func__);

	return 0;

err_open:
	return ret;
}

static void touchkey_input_close(struct input_dev *dev)
{
	struct touchkey_i2c *data = input_get_drvdata(dev);

#ifdef TK_USE_OPEN_DWORK
	cancel_delayed_work_sync(&data->open_work);
#endif
#ifdef TKEY_GRIP_MODE
	data->pwr_flag = false;
#endif
	touchkey_stop(data);
	gpio_tlmm_config(GPIO_CFG(tkey_gpio_scl, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(tkey_gpio_sda, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);

	dev_info(&data->client->dev, "%s\n", __func__);
}

#if defined(CONFIG_PM) && !defined(USE_OPEN_CLOSE)
#ifdef CONFIG_HAS_EARLYSUSPEND
#define touchkey_suspend	NULL
#define touchkey_resume	NULL

static int sec_touchkey_early_suspend(struct early_suspend *h)
{
	struct touchkey_i2c *tkey_i2c =
		container_of(h, struct touchkey_i2c, early_suspend);

	touchkey_stop(tkey_i2c);

	dev_dbg(&tkey_i2c->client->dev, "%s\n", __func__);

	return 0;
}

static int sec_touchkey_late_resume(struct early_suspend *h)
{
	struct touchkey_i2c *tkey_i2c =
		container_of(h, struct touchkey_i2c, early_suspend);

	dev_dbg(&tkey_i2c->client->dev, "%s\n", __func__);

	touchkey_start(tkey_i2c);

	return 0;
}
#else
static int touchkey_suspend(struct device *dev)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);

	if (touchkey_probe != true) {
		printk(KERN_ERR "%s Touchkey is not enabled. \n",
		       __func__);
		return 0;
	}
	mutex_lock(&tkey_i2c->input_dev->mutex);

	if (tkey_i2c->input_dev->users)
		touchkey_stop(tkey_i2c);

	mutex_unlock(&tkey_i2c->input_dev->mutex);

	dev_dbg(&tkey_i2c->client->dev, "%s\n", __func__);

	return 0;
}

static int touchkey_resume(struct device *dev)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);

	if (touchkey_probe != true) {
		printk(KERN_ERR "%s Touchkey is not enabled. \n",
		       __func__);
		return 0;
	}
	mutex_lock(&tkey_i2c->input_dev->mutex);

	if (tkey_i2c->input_dev->users)
		touchkey_start(tkey_i2c);

	mutex_unlock(&tkey_i2c->input_dev->mutex);

	dev_dbg(&tkey_i2c->client->dev, "%s\n", __func__);

	return 0;
}
#endif
static SIMPLE_DEV_PM_OPS(touchkey_pm_ops, touchkey_suspend, touchkey_resume);

#endif

static int touchkey_i2c_check(struct touchkey_i2c *tkey_i2c)
{
	char data[3] = { 0, };
	int ret = 0;
	int retry = 3;

	while (retry--) {
		ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 3);
		if (ret < 0) {
			dev_err(&tkey_i2c->client->dev, "retry i2c check(%d)\n", retry);
			msleep(30);
			continue;
		}
		break;
	}

	if (ret < 0) {
		dev_err(&tkey_i2c->client->dev, "Failed to read Module version\n");
		tkey_i2c->fw_ver_ic = 0;
		tkey_i2c->md_ver_ic = 0;
		return ret;
	}

	tkey_i2c->fw_ver_ic = data[1];
	tkey_i2c->md_ver_ic = data[2];
	dev_info(&tkey_i2c->client->dev, "%s: fw ver = 0x%02x, md ver = 0x%02x\n",
		__func__, tkey_i2c->fw_ver_ic, tkey_i2c->md_ver_ic);

	/* CYPRESS Firmware setting interrupt type : dual or single interrupt */
	cypress_touchkey_interrupt_set_dual(tkey_i2c);

	return ret;
}

static ssize_t touchkey_led_control(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int data;
	int ret;

#if !defined(TK_LED_DIRECT_CONTORL)
	static const int ledCmd[] = {TK_CMD_LED_OFF, TK_CMD_LED_ON};
#endif
	if (wake_lock_active(&tkey_i2c->fw_wakelock)) {
		printk(KERN_DEBUG"touchkey:%s, wakelock active\n", __func__);
		return size;
	}

	ret = sscanf(buf, "%d", &data);

	if (ret != 1) {
		dev_err(&tkey_i2c->client->dev, "%s, %d err\n",
			__func__, __LINE__);
		return size;
	}

	if (data != 0 && data != 1) {
		dev_err(&tkey_i2c->client->dev, "%s wrong cmd %x\n",
			__func__, data);
		return size;
	}

#if !defined(TK_LED_DIRECT_CONTORL)
	data = ledCmd[data];
	if (!tkey_i2c->enabled) {
		touchled_cmd_reversed = 1;
		goto out;
	}

	ret = i2c_touchkey_write(tkey_i2c, KEYCODE_REG, (u8 *) &data, 1);
	if (ret < 0) {
		dev_err(&tkey_i2c->client->dev, "%s: Error turn on led %d\n",
			__func__, ret);
		touchled_cmd_reversed = 1;
		goto out;
	}
	msleep(30);
#else
	if (!tkey_i2c->enabled) {
		touchled_cmd_reversed = 1;
		goto out;
	}

	tkey_i2c->pdata->led_power_on(data);
#endif


out:
	touchkey_led_status = data;

	return size;
}

#if defined(TK_USE_4KEY)
static ssize_t touchkey_menu_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[18] = { 0, };
	int ret;

	dev_dbg(&tkey_i2c->client->dev, "called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 18);

	dev_dbg(&tkey_i2c->client->dev, "called %s data[10] =%d,data[11] = %d\n", __func__,
	       data[10], data[11]);
	menu_sensitivity = ((0x00FF & data[10]) << 8) | data[11];

	return sprintf(buf, "%d\n", menu_sensitivity);
}

static ssize_t touchkey_home_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[18] = { 0, };
	int ret;

	dev_dbg(&tkey_i2c->client->dev, "called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 18);

	dev_dbg(&tkey_i2c->client->dev, "called %s data[12] =%d,data[13] = %d\n", __func__,
	       data[12], data[13]);
	home_sensitivity = ((0x00FF & data[12]) << 8) | data[13];

	return sprintf(buf, "%d\n", home_sensitivity);
}

static ssize_t touchkey_back_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[18] = { 0, };
	int ret;

	dev_dbg(&tkey_i2c->client->dev, "called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 18);

	dev_dbg(&tkey_i2c->client->dev, "called %s data[14] =%d,data[15] = %d\n", __func__,
	       data[14], data[15]);
	back_sensitivity = ((0x00FF & data[14]) << 8) | data[15];

	return sprintf(buf, "%d\n", back_sensitivity);
}

static ssize_t touchkey_search_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	u8 data[18] = { 0, };
	int ret;

	dev_dbg(&tkey_i2c->client->dev, "called %s\n", __func__);
	ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 18);

	dev_dbg(&tkey_i2c->client->dev, "called %s data[16] =%d,data[17] = %d\n", __func__,
	       data[16], data[17]);
	search_sensitivity = ((0x00FF & data[16]) << 8) | data[17];

	return sprintf(buf, "%d\n", search_sensitivity);
}
#else
static ssize_t touchkey_menu_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_SENSITIVITY, 0, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}

static ssize_t touchkey_back_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct FAC_CMD cmd = {TK_CMD_READ_SENSITIVITY, 1, 0};
	return touchkey_fac_read_data(dev, buf, &cmd);
}
#endif

#if defined(TK_HAS_AUTOCAL)
static ssize_t autocalibration_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int data;

	sscanf(buf, "%d\n", &data);
	dev_dbg(&tkey_i2c->client->dev, "%s %d\n", __func__, data);

	if (data == 1)
		touchkey_autocalibration(tkey_i2c);

	return size;
}

static ssize_t autocalibration_status(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	u8 data[6];
	int ret;
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);

	dev_dbg(&tkey_i2c->client->dev, "%s\n", __func__);

	ret = i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 6);
	if ((data[5] & TK_BIT_AUTOCAL))
		return sprintf(buf, "Enabled\n");
	else
		return sprintf(buf, "Disabled\n");

}
#endif

#if defined(CONFIG_GLOVE_TOUCH)
static ssize_t glove_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int data;

	sscanf(buf, "%d\n", &data);
	dev_dbg(&tkey_i2c->client->dev, "%s %d\n", __func__, data);

	tkey_i2c->tsk_enable_glove_mode = data;
	touchkey_glovemode(data);

	return size;
}
#endif

#ifdef TKEY_FLIP_MODE
static ssize_t flip_cover_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int flip_mode_on;

	sscanf(buf, "%d\n", &flip_mode_on);
	dev_info(&tkey_i2c->client->dev, "%s %d\n", __func__, flip_mode_on);
	/* glove mode control */
	if (flip_mode_on) {
		touchkey_flip_cover(flip_mode_on);
	} else {
#if defined(CONFIG_GLOVE_TOUCH)
		if (tkey_i2c->tsk_enable_glove_mode) {
			tkey_i2c->ic_mode = MODE_NORMAL;
			touchkey_glovemode(1);
		}
		else
#endif
			touchkey_flip_cover(0);
	}
	return size;
}
#endif

static ssize_t touch_sensitivity_control(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	unsigned char data = 0x40;
	i2c_touchkey_write(tkey_i2c, KEYCODE_REG, &data, 1);
	dev_dbg(&tkey_i2c->client->dev, "%s\n", __func__);
	msleep(20);
	return size;
}

static ssize_t set_touchkey_update_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t size)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);

	u8 fw_path;

	switch(*buf) {
	case 's':
	case 'S':
		fw_path = FW_BUILT_IN;
		break;
	case 'i':
	case 'I':
		fw_path = FW_IN_SDCARD;
		break;
	default:
		return size;
	}

	touchkey_fw_update(tkey_i2c, fw_path, true);

	msleep(3000);
	cancel_work_sync(&tkey_i2c->update_work);

	return size;
}

static ssize_t set_touchkey_firm_version_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	printk(KERN_DEBUG"touchkey:firm_ver_bin %0#4x\n", fw_ver_file);
	return sprintf(buf, "%0#4x\n", fw_ver_file);
}

static ssize_t set_touchkey_firm_version_read_show(struct device *dev,
						   struct device_attribute
						   *attr, char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	char data[3] = { 0, };
	int count;

	i2c_touchkey_read(tkey_i2c, KEYCODE_REG, data, 3);
	count = sprintf(buf, "0x%02x\n", data[1]);

	dev_info(&tkey_i2c->client->dev, "Touch_version_read 0x%02x\n", data[1]);
	dev_info(&tkey_i2c->client->dev, "Module_version_read 0x%02x\n", data[2]);
	return count;
}

static ssize_t set_touchkey_firm_status_show(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int count = 0;

	dev_info(&tkey_i2c->client->dev, "Touch_update_read: update_status %d\n",
	       tkey_i2c->update_status);

	if (tkey_i2c->update_status == TK_UPDATE_PASS)
		count = sprintf(buf, "PASS\n");
	else if (tkey_i2c->update_status == TK_UPDATE_DOWN)
		count = sprintf(buf, "Downloading\n");
	else if (tkey_i2c->update_status == TK_UPDATE_FAIL)
		count = sprintf(buf, "Fail\n");

	return count;
}
#if defined(TOUCHKEY_BOOSTER)
static ssize_t touchkey_boost_level(struct device *dev,
						struct device_attribute *attr, const char *buf,
						size_t count)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	unsigned int level;

	sscanf(buf, "%d", &level);

	if (level > 2) {
		dev_err(dev, "err to set boost_level %d\n", level);
		return count;
	}

#ifdef TOUCHKEY_BOOSTER
	tkey_i2c->dvfs_boost_mode = level;
#endif
	dev_info(dev, "%s %d\n", __func__, level);

	if (tkey_i2c->dvfs_boost_mode == TKEY_BOOSTER_LEVEL2) {
		tkey_i2c->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
		dev_info(dev,
			"%s: boost_mode DUAL, dvfs_freq = %d\n",
			__func__, tkey_i2c->dvfs_freq);
	} else if (tkey_i2c->dvfs_boost_mode == TKEY_BOOSTER_LEVEL1) {
		tkey_i2c->dvfs_freq = MIN_TOUCH_LIMIT;
		dev_info(dev,
			"%s: boost_mode SINGLE, dvfs_freq = %d\n",
			__func__, tkey_i2c->dvfs_freq);
	} else if (tkey_i2c->dvfs_boost_mode == TKEY_BOOSTER_DISABLE) {
		touchkey_set_dvfs_off(tkey_i2c);
	}

	return count;
}
#endif

#if defined(TKEY_GRIP_MODE)
static ssize_t touchkey_grip_mode_show(struct device *dev,
		struct device_attribute *attr,
			char *buf)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", tkey_i2c->grip_mode ? "ON" : "OFF");
}

static ssize_t touchkey_grip_mode(struct device *dev,
struct device_attribute *attr, const char *buf,
	size_t count)
{
	struct touchkey_i2c *tkey_i2c = dev_get_drvdata(dev);
	int mode = 0;

	mutex_lock(&tkey_i2c->lock);

	sscanf(buf, "%d", &mode);

	tkey_i2c->grip_mode = (mode == 1) ? true : false;
	printk(KERN_DEBUG"touchkey:%s, grip_mode %d, enabled %d\n",
		__func__, tkey_i2c->grip_mode, tkey_i2c->ic_mode);

	if (tkey_i2c->pwr_flag == false)
		touchkey_grip_mode_ctr(tkey_i2c);
	else
		printk(KERN_DEBUG"touchkey:pwr enabled, skip grip mode\n");

	mutex_unlock(&tkey_i2c->lock);

	return count;
}
#endif

static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   touchkey_led_control);
#ifdef TK_USE_RECENT
static DEVICE_ATTR(touchkey_recent, S_IRUGO | S_IWUSR | S_IWGRP,
		   touchkey_menu_show, NULL);
#else
static DEVICE_ATTR(touchkey_menu, S_IRUGO | S_IWUSR | S_IWGRP,
		   touchkey_menu_show, NULL);
#endif
static DEVICE_ATTR(touchkey_back, S_IRUGO | S_IWUSR | S_IWGRP,
		   touchkey_back_show, NULL);

#if defined(TK_USE_4KEY)
static DEVICE_ATTR(touchkey_home, S_IRUGO, touchkey_home_show, NULL);
static DEVICE_ATTR(touchkey_search, S_IRUGO, touchkey_search_show, NULL);
#endif

static DEVICE_ATTR(touch_sensitivity, S_IRUGO | S_IWUSR | S_IWGRP,
		   NULL, touch_sensitivity_control);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO | S_IWUSR | S_IWGRP,
		   NULL, set_touchkey_update_store);
static DEVICE_ATTR(touchkey_firm_update_status, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_touchkey_firm_status_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_touchkey_firm_version_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO | S_IWUSR | S_IWGRP,
		   set_touchkey_firm_version_read_show, NULL);
#ifdef LED_LDO_WITH_REGULATOR
static DEVICE_ATTR(touchkey_brightness, S_IRUGO | S_IWUSR | S_IWGRP,
		   NULL, brightness_control);
#endif

#if defined(TK_HAS_AUTOCAL)
#ifdef TK_USE_RECENT
static DEVICE_ATTR(touchkey_recent_raw, S_IRUGO, touchkey_raw_data0_show, NULL);
static DEVICE_ATTR(touchkey_back_raw, S_IRUGO, touchkey_raw_data1_show, NULL);
static DEVICE_ATTR(touchkey_recent_idac, S_IRUGO, touchkey_idac0_show, NULL);
static DEVICE_ATTR(touchkey_back_idac, S_IRUGO, touchkey_idac1_show, NULL);
#else
static DEVICE_ATTR(touchkey_raw_data0, S_IRUGO, touchkey_raw_data0_show, NULL);
static DEVICE_ATTR(touchkey_raw_data1, S_IRUGO, touchkey_raw_data1_show, NULL);
static DEVICE_ATTR(touchkey_raw_data2, S_IRUGO, touchkey_raw_data2_show, NULL);
static DEVICE_ATTR(touchkey_raw_data3, S_IRUGO, touchkey_raw_data3_show, NULL);
static DEVICE_ATTR(touchkey_idac0, S_IRUGO, touchkey_idac0_show, NULL);
static DEVICE_ATTR(touchkey_idac1, S_IRUGO, touchkey_idac1_show, NULL);
static DEVICE_ATTR(touchkey_idac2, S_IRUGO, touchkey_idac2_show, NULL);
static DEVICE_ATTR(touchkey_idac3, S_IRUGO, touchkey_idac3_show, NULL);
#endif
static DEVICE_ATTR(touchkey_threshold, S_IRUGO, touchkey_threshold_show, NULL);
static DEVICE_ATTR(autocal_enable, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   autocalibration_enable);
static DEVICE_ATTR(autocal_stat, S_IRUGO | S_IWUSR | S_IWGRP,
		   autocalibration_status, NULL);
#endif
#if defined(CONFIG_GLOVE_TOUCH)
static DEVICE_ATTR(glove_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   glove_mode_enable);
#endif
#ifdef TKEY_FLIP_MODE
static DEVICE_ATTR(flip_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   flip_cover_mode_enable);
#endif
#if defined(TOUCHKEY_BOOSTER)
static DEVICE_ATTR(boost_level, S_IWUSR | S_IWGRP, NULL, touchkey_boost_level);
#endif

#ifdef TKEY_GRIP_MODE
static DEVICE_ATTR(grip_mode, S_IRUGO | S_IWUSR | S_IWGRP, touchkey_grip_mode_show,
	touchkey_grip_mode);
#endif

static struct attribute *touchkey_attributes[] = {
	&dev_attr_brightness.attr,
#ifdef TK_USE_RECENT
	&dev_attr_touchkey_recent.attr,
#else
	&dev_attr_touchkey_menu.attr,
#endif
	&dev_attr_touchkey_back.attr,
#if defined(TK_USE_4KEY)
	&dev_attr_touchkey_home.attr,
	&dev_attr_touchkey_search.attr,
#endif
	&dev_attr_touch_sensitivity.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
#ifdef LED_LDO_WITH_REGULATOR
	&dev_attr_touchkey_brightness.attr,
#endif
#if defined(TK_HAS_AUTOCAL)
#ifdef TK_USE_RECENT
	&dev_attr_touchkey_recent_raw.attr,
	&dev_attr_touchkey_back_raw.attr,
	&dev_attr_touchkey_recent_idac.attr,
	&dev_attr_touchkey_back_idac.attr,
#else
	&dev_attr_touchkey_raw_data0.attr,
	&dev_attr_touchkey_raw_data1.attr,
	&dev_attr_touchkey_raw_data2.attr,
	&dev_attr_touchkey_raw_data3.attr,
	&dev_attr_touchkey_idac0.attr,
	&dev_attr_touchkey_idac1.attr,
	&dev_attr_touchkey_idac2.attr,
	&dev_attr_touchkey_idac3.attr,
#endif
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_autocal_enable.attr,
	&dev_attr_autocal_stat.attr,
#endif
#if defined(CONFIG_GLOVE_TOUCH)
	&dev_attr_glove_mode.attr,
#endif
#ifdef TKEY_FLIP_MODE
	&dev_attr_flip_mode.attr,
#endif
#if defined(TOUCHKEY_BOOSTER)
	&dev_attr_boost_level.attr,
#endif
#ifdef TKEY_GRIP_MODE
	&dev_attr_grip_mode.attr,
#endif
	NULL,
};

static struct attribute_group touchkey_attr_group = {
	.attrs = touchkey_attributes,
};

static void touchkey_init_hw(void)
{

	printk(KERN_ERR "%s, %d\n", __func__, __LINE__);

#ifndef TK_USE_LDO_CONTROL
	gpio_direction_output(tkey_gpio_i2cldo, 0);
#else
	cypress_ldo_onoff(0);
#endif
	
	gpio_tlmm_config(GPIO_CFG(tkey_gpio_int, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

#ifdef GPIO_2TOUCH_RST
	gpio_direction_output(tkey_gpio_rst, 0);
#endif
#ifdef GPIO_2_TOUCH_ID
	gpio_tlmm_config(GPIO_CFG(tkey_gpio_id, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
#endif
	gpio_tlmm_config(GPIO_CFG(tkey_gpio_scl, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(tkey_gpio_sda, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);

}

static int touchkey_suspend(void)
{
#ifndef TK_USE_LDO_CONTROL
	gpio_direction_output(tkey_gpio_i2cldo, 0);
#else
	cypress_ldo_onoff(0);
#endif

	return 1;
}

static int touchkey_resume(void)
{
#ifndef TK_USE_LDO_CONTROL
	gpio_direction_output(tkey_gpio_i2cldo, 1);
#else
	cypress_ldo_onoff(1);
#endif

	return 1;
}

static int touchkey_power_on(bool on)
{
	int ret;
	
	printk(KERN_ERR "%s, on=%d %d\n", __func__,on, __LINE__);

	if (on) {
		gpio_direction_input(tkey_gpio_int);
		irq_set_irq_type(gpio_to_irq(tkey_gpio_int), IRQF_TRIGGER_FALLING);
		gpio_tlmm_config(GPIO_CFG(tkey_gpio_int, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	} else {
		gpio_direction_input(tkey_gpio_int);
#ifdef GPIO_2TOUCH_RST
		gpio_direction_output(tkey_gpio_rst, 0);
#endif
	}

	if (on)
		ret = touchkey_resume();
	else
		ret = touchkey_suspend();

	return ret;
}

#if defined(TK_USE_MAX_SUBPM_CONTROL)
static int cypress_vdd_onoff(bool on, int led)	// led 3.3V is 1.
{
	int retval;	
	if(on){
		if (regulator_is_enabled(cyp_supplies[led].consumer)) {
			pr_err("%s: %s is already enabled\n", __func__, cyp_supplies[led].supply);
		}else {
			retval = regulator_enable(cyp_supplies[led].consumer);
			if (retval) {
				pr_err("%s: Fail to enable regulator %s (%d)\n", __func__, cyp_supplies[led].supply, retval);
			}
			pr_err("%s: %s is enabled[OK]\n", __func__, cyp_supplies[led].supply);
		}
	}else{
		if (regulator_is_enabled(cyp_supplies[led].consumer)) {
			retval = regulator_disable(cyp_supplies[led].consumer);
			if (retval) {
				pr_err("%s: Fail to disable regulator %s (%d)\n",	__func__, cyp_supplies[led].supply, retval);
			}
			pr_err("%s: %s is disabled[OK]\n", __func__, cyp_supplies[led].supply);
		}else {
			pr_err("%s: %s is already disabled\n", __func__, cyp_supplies[led].supply);
		}
	}
	return 1;
}
static int cypress_ldo_onoff(bool on)
{
	pr_err("%s: on:%d\n", __func__, on);
	cypress_vdd_onoff(on, CYP_IC);
	return 1;
}
static int touchkey_led_power_on(bool on)
{
	pr_err("%s: on:%d\n", __func__, on);
	cypress_vdd_onoff(on, CYP_LED);
	return 1;
}

#elif !defined(TK_USE_LDO_CONTROL)
static int touchkey_led_power_on(bool on)
{
	printk(KERN_ERR "%s, on:%d %d\n", __func__,on, __LINE__);

	if (on)
		gpio_direction_output(tkey_gpio_vddldo, 1);
	else
		gpio_direction_output(tkey_gpio_vddldo, 0);

	return 1;
}
#else
static int cypress_ldo_onoff(bool on)
{
	int rc = 0;
	static struct regulator *tkey_regulator;

	printk(KERN_ERR "%s, on:%d %d\n", __func__,on, __LINE__);

	if(!tkey_regulator){
		tkey_regulator = regulator_get(NULL, TKEY_I2C_REGULATOR);
		if (IS_ERR(tkey_regulator)) {
			pr_err("%s: failed to get resource %s\n", __func__,
			       "touchkey_ldo");
			return 1;
		}
	}

	if (on) {
		if(tkey_regulator){		
			if (!regulator_is_enabled(tkey_regulator)) {
				rc = regulator_enable(tkey_regulator);
				if (rc) {
					pr_err("Regulator vcc_en enable failed rc=%d\n",rc);
					return 1;
				}
			}
		}
	} else {
		if(tkey_regulator){		
			if (regulator_is_enabled(tkey_regulator)) {
				rc = regulator_disable(tkey_regulator);
				if (rc) {
					pr_err("Regulator vcc_en disable failed rc=%d\n",rc);
					return 1;
				}
			}
		}
	}
	return 1;	
}

static int touchkey_led_power_on(bool on)
{
	int rc = 0;
	static struct regulator *tkey_led_regulator;

	printk(KERN_ERR "%s, on:%d %d\n", __func__,on, __LINE__);

	if(!tkey_led_regulator){
		tkey_led_regulator = regulator_get(NULL, TKEY_LED_REGULATOR);
		if (IS_ERR(tkey_led_regulator)) {
			pr_err("%s: failed to get resource %s\n", __func__,
			       "touchkey_led_ldo");
			return 1;
		}

		rc = regulator_set_voltage(tkey_led_regulator, 3300000, 3300000);
		if (rc) {
			pr_err("%s: regulator(vdd_led) set_vtg failed %s\n", __func__,
			       "touchkey_led_ldo");
			return 1;
		}
	}

	if (on) {
		if(tkey_led_regulator){
			if (!regulator_is_enabled(tkey_led_regulator)) {
				rc = regulator_enable(tkey_led_regulator);
				if (rc) {
					pr_err("Regulator Tkey_LED enable failed rc=%d\n",rc);
					return 1;
				}
			}
		}
	} else {
		if(tkey_led_regulator){	
			if (regulator_is_enabled(tkey_led_regulator)) {
				rc = regulator_disable(tkey_led_regulator);
				if (rc) {
					pr_err("Regulator Tkey_LED disable failed rc=%d\n",rc);
					return 1;
				}
			}
		}
	}
	return 1;
}
#endif

#ifdef GPIO_2TOUCH_RST
static int touchkey_rst_reset(void)
{
	printk(KERN_ERR "%s, %d\n", __func__, __LINE__);

	gpio_direction_output(tkey_gpio_rst, 1);
	usleep(1000);
	gpio_direction_output(tkey_gpio_rst, 0);
	
	return 0;
}
#endif



static void cypress_request_gpio(struct touchkey_platform_data *pdata)
{
	int ret;

	printk(KERN_ERR "%s, %d\n", __func__, __LINE__);


	ret = gpio_request(pdata->gpio_int, "touchkey_irq");
	if (ret) {
		printk(KERN_ERR "%s: unable to request touchkey_irq [%d]\n", __func__, pdata->gpio_int);
	}
#ifdef GPIO_2_TOUCH_ID
	if (pdata->gpio_id > 0) {
		ret = gpio_request(pdata->gpio_id, "gpio_id");
		if (ret) {
			printk(KERN_ERR "%s: unable to request gpio_id[%d]\n", __func__, pdata->gpio_id);
		}
	}
#endif

#ifndef TK_USE_LDO_CONTROL

	if (tkey_gpio_i2cldo > 0) {
		ret = gpio_request(tkey_gpio_i2cldo, "tkey_gpio_i2cldo");
		if (ret) {
			printk(KERN_ERR "%s: unable to request tkey_gpio_i2cldo [%d]\n", __func__, tkey_gpio_i2cldo);
		}
	}

	if (tkey_gpio_vddldo > 0) {
		ret = gpio_request(tkey_gpio_vddldo, "tkey_gpio_vddldo");
		if (ret) {
			printk(KERN_ERR "%s: unable to request tkey_gpio_vddldo [%d]\n", __func__, tkey_gpio_vddldo);
		}
	}
#endif
#ifdef GPIO_2TOUCH_RST
	if (pdata->gpio_rst > 0) {
		ret = gpio_request(pdata->gpio_rst, "gpio_rst");
		if (ret) {
			printk(KERN_ERR "%s: unable to request gpio_rst [%d]\n", __func__, pdata->gpio_rst);
		}
	}
#endif
#ifdef TKEY_GRIP_MODE
	if (pdata->gpio_grip > 0) {
		ret = gpio_request(pdata->gpio_grip, "gpio_grip");
		if (ret) {
			printk(KERN_ERR "%s: unable to request gpio_grip [%d]\n", __func__, pdata->gpio_grip);
		}
	}
#endif

	
}

#ifdef CONFIG_OF
static int cypress_parse_dt(struct device *dev,
			struct touchkey_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	
	printk(KERN_ERR "%s, %d\n", __func__, __LINE__);

#ifndef TK_USE_LDO_CONTROL
	tkey_gpio_i2cldo = of_get_named_gpio(np, "cypress,touchkey_i2cldo-gpio", 0);
	tkey_gpio_vddldo = of_get_named_gpio(np, "cypress,touchkey_vddldo-gpio", 0);
#endif

	/* reset, irq gpio info */
	pdata->gpio_scl = of_get_named_gpio(np, "cypress,scl-gpio", 0);
	pdata->gpio_sda = of_get_named_gpio(np, "cypress,sda-gpio", 0);
	pdata->gpio_int = of_get_named_gpio(np, "cypress,irq-gpio", 0);

	tkey_gpio_sda = pdata->gpio_sda;
	tkey_gpio_scl = pdata->gpio_scl;
	tkey_gpio_int = pdata->gpio_int;

#ifdef GPIO_2_TOUCH_ID
	pdata->gpio_id = of_get_named_gpio(np, "cypress,twotouch_id-gpio", 0);
	tkey_gpio_id = pdata->gpio_id;
#endif
#ifdef GPIO_2TOUCH_RST
	pdata->gpio_rst = of_get_named_gpio(np, "cypress,twotouch_rst-gpio", 0);
	tkey_gpio_rst = pdata->gpio_rst;
#endif
#ifdef TKEY_GRIP_MODE
	pdata->gpio_grip = of_get_named_gpio(np, "cypress,twotouch_grip-gpio", 0);
	tkey_gpio_grip = pdata->gpio_grip;
#endif

#if defined(TK_USE_MAX_SUBPM_CONTROL)
	{
		int rc;
		pdata->name_of_supply = kzalloc(sizeof(char *) * 2, GFP_KERNEL);
		rc = of_property_read_string_index(np, "cypress,supply-name", 0, &pdata->name_of_supply[0]);
		if (rc && (rc != -EINVAL)) {
			printk(KERN_INFO "%s: Unable to read %s\n", __func__,"cypress,supply-name 0");
			return rc;
		}
		rc = of_property_read_string_index(np, "cypress,supply-name", 1, &pdata->name_of_supply[1]);
		if (rc && (rc != -EINVAL)) {
			printk(KERN_INFO "%s: Unable to read %s\n", __func__,"cypress,supply-name 1");
			return rc;
		}
		dev_info(dev, "%s: supply: %s, %s\n", __func__, pdata->name_of_supply[0],pdata->name_of_supply[1]);
	}
	
#endif

#if defined(CONFIG_SEC_S_PROJECT)
	touchkey_keycode[1] = KEY_RECENT;
#endif

	return 0;
}
#endif

static int i2c_touchkey_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct touchkey_platform_data *pdata; 
	struct touchkey_i2c *tkey_i2c;
	#if defined(CONFIG_KEYBOARD_CYPRESS_TKEY_HL)
	bool bforced = false;
	#endif
	struct input_dev *input_dev;
	int i;
	int ret = 0;
	int error = 0;

	/*Check I2C functionality */
	ret = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if (ret == 0) {
		dev_err(&client->dev, "No I2C functionality found\n");
		return -ENODEV;
	}
	
#ifdef CONFIG_OF
	if (client->dev.of_node) {
		
		dev_err(&client->dev, "%s, config of, %d\n", __func__, __LINE__);
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct touchkey_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_info(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		error = cypress_parse_dt(&client->dev, pdata);
		if (error)
			return error;
	} else{
		pdata = client->dev.platform_data;
		dev_err(&client->dev, "undefined config of\n");
	}
	cypress_request_gpio(pdata);


	pdata->init_platform_hw = touchkey_init_hw;
	pdata->suspend = touchkey_suspend;
	pdata->resume = touchkey_resume;
	pdata->power_on = touchkey_power_on;
	pdata->led_power_on = touchkey_led_power_on;
#ifdef GPIO_2TOUCH_RST
	pdata->rst_reset = touchkey_rst_reset;
#endif
	
#if defined(TK_USE_MAX_SUBPM_CONTROL)
	cyp_supplies = kzalloc(sizeof(struct regulator_bulk_data) * 2, GFP_KERNEL);
	if (!cyp_supplies) {
		pr_err("%s: Failed to alloc mem for supplies\n", __func__);
		return -ENOMEM;
	}
	cyp_supplies[0].supply = pdata->name_of_supply[0];	// 1.8V, IC
	cyp_supplies[1].supply = pdata->name_of_supply[1];	// 3.3V, LED
	
	ret = regulator_bulk_get(&client->dev, 2, cyp_supplies);
	if (ret)
		goto err_get_regulator;


	ret = regulator_set_voltage(cyp_supplies[CYP_IC].consumer,1950000, 1950000);
	if (ret) pr_err("%s, 1.8 set_vtg failed rc=%d\n", __func__, ret);
	
	ret = regulator_set_voltage(cyp_supplies[CYP_LED].consumer,3300000, 3300000);
	if (ret) pr_err("%s, 3.3 set_vtg failed rc=%d\n", __func__, ret);


	pr_err("%s: Max77826 supplies was seted \n", __func__);
#endif


	touchkey_init_hw();
	client->irq = gpio_to_irq(tkey_gpio_int);
	irq_set_irq_type(gpio_to_irq(tkey_gpio_int), IRQF_TRIGGER_FALLING);

#endif	

	/*Obtain kernel memory space for touchkey i2c */
	tkey_i2c = kzalloc(sizeof(struct touchkey_i2c), GFP_KERNEL);
	if (NULL == tkey_i2c) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

#ifdef CONFIG_GLOVE_TOUCH
	tkey_i2c_global = tkey_i2c;
#endif

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "Failed to allocate input device\n");
		ret = -ENOMEM;
		goto err_allocate_input_device;
	}

	input_dev->name = "sec_touchkey";
	input_dev->phys = "sec_touchkey/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &client->dev;
	input_dev->open = touchkey_input_open;
	input_dev->close = touchkey_input_close;

	/*tkey_i2c*/
	tkey_i2c->pdata = pdata;
	tkey_i2c->input_dev = input_dev;
	tkey_i2c->client = client;
	tkey_i2c->irq = client->irq;
	tkey_i2c->name = "sec_touchkey";
	tkey_i2c->status_update = false;
#ifdef TKEY_GRIP_MODE
	tkey_i2c->grip_mode = false;
	tkey_i2c->ic_mode = MODE_NORMAL;
	mutex_init(&tkey_i2c->grip_mode_lock);
#endif
	// init_completion(&tkey_i2c->init_done);
	mutex_init(&tkey_i2c->lock);
	mutex_init(&tkey_i2c->i2c_lock);
	mutex_init(&tkey_i2c->irq_lock);
#ifdef TK_USE_OPEN_DWORK
	INIT_DELAYED_WORK(&tkey_i2c->open_work, touchkey_open_work);
#endif
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);
	set_bit(EV_KEY, input_dev->evbit);
#ifdef CONFIG_VT_TKEY_SKIP_MATCH
	set_bit(EV_TOUCHKEY, input_dev->evbit);
#endif
	INIT_WORK(&tkey_i2c->update_work, touchkey_i2c_update_work);
	wake_lock_init(&tkey_i2c->fw_wakelock, WAKE_LOCK_SUSPEND, "touchkey");

	for (i = 1; i < touchkey_count; i++)
		set_bit(touchkey_keycode[i], input_dev->keybit);

	input_set_drvdata(input_dev, tkey_i2c);
	i2c_set_clientdata(client, tkey_i2c);

	ret = input_register_device(input_dev);
	if (ret) {
		dev_err(&client->dev, "Failed to register input device\n");
		goto err_register_device;
	}

	tkey_i2c->pdata->power_on(1);
#ifdef TKEY_GRIP_MODE
	tkey_i2c->pwr_flag = true;
#endif
	msleep(300);//50
	tkey_i2c->enabled = true;

	ret = touchkey_i2c_check(tkey_i2c);
	if (ret < 0) {
		dev_err(&client->dev, "i2c_check failed\n");
#if defined(CONFIG_KEYBOARD_CYPRESS_TKEY_HL)
		bforced = true;
#else
		goto err_i2c_check;
#endif
	}

	/*sysfs*/
	tkey_i2c->dev = device_create(sec_class, NULL, 0, NULL, "sec_touchkey");

	ret = IS_ERR(tkey_i2c->dev);
	if (ret) {
		dev_err(&client->dev, "Failed to create device(tkey_i2c->dev)!\n");
		goto err_device_create;
	}
	dev_set_drvdata(tkey_i2c->dev, tkey_i2c);

	ret = sysfs_create_group(&tkey_i2c->dev->kobj,
				&touchkey_attr_group);
	if (ret) {
		dev_err(&client->dev, "Failed to create sysfs group\n");
		goto err_sysfs_init;
	}

	ret = sysfs_create_link(&tkey_i2c->dev->kobj,
					&tkey_i2c->input_dev->dev.kobj, "input");
	if (ret) {
		dev_err(&client->dev, "Failed to connect link\n");
		goto err_create_symlink;
	}

	tkey_i2c->fw_wq = create_singlethread_workqueue(client->name);
	if (!tkey_i2c->fw_wq) {
		dev_err(&client->dev, "fail to create workqueue for fw_wq\n");
		ret = -ENOMEM;
		goto err_create_fw_wq;
	}

#ifdef TOUCHKEY_BOOSTER
	touchkey_init_dvfs(tkey_i2c);
#endif

#if defined(CONFIG_GLOVE_TOUCH)
		mutex_init(&tkey_i2c->tsk_glove_lock);
	INIT_WORK(&tkey_i2c->glove_change_work, touchkey_glove_change_work);
#endif

	ret =
		request_threaded_irq(tkey_i2c->irq, NULL, touchkey_interrupt,
				IRQF_DISABLED | IRQF_TRIGGER_FALLING |
				IRQF_ONESHOT, tkey_i2c->name, tkey_i2c);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to request irq(%d) - %d\n",
			tkey_i2c->irq, ret);
		goto err_request_threaded_irq;
	}

	tkey_i2c->pdata->led_power_on(1);

#if defined(TK_HAS_FIRMWARE_UPDATE)
/*tkey_firmupdate_retry_byreboot:*/
	if (system_rev >= TK_UPDATABLE_BD_ID) {
		touchkey_fw_update(tkey_i2c, FW_BUILT_IN, bforced);
	}
#endif

#if defined(TK_INFORM_CHARGER)
	tkey_i2c->pdata->register_cb = touchkey_register_callback;

	tkey_i2c->callbacks.inform_charger = touchkey_ta_cb;
	if (tkey_i2c->pdata->register_cb) {
		dev_info(&client->dev, "Touchkey TA information\n");
		tkey_i2c->pdata->register_cb(&tkey_i2c->callbacks);
	}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	tkey_i2c->early_suspend.suspend =
		(void *)sec_touchkey_early_suspend;
	tkey_i2c->early_suspend.resume =
		(void *)sec_touchkey_late_resume;
	register_early_suspend(&tkey_i2c->early_suspend);
#endif

/*	touchkey_stop(tkey_i2c); */
	// complete_all(&tkey_i2c->init_done);
	touchkey_probe = true;

	return 0;

#if defined(TK_HAS_FIRMWARE_UPDATE)
/*err_firmware_update:*/
	tkey_i2c->pdata->led_power_on(0);
	touchkey_enable_irq(tkey_i2c, false);
	free_irq(tkey_i2c->irq, tkey_i2c);
#endif
err_request_threaded_irq:
#ifdef CONFIG_GLOVE_TOUCH
	mutex_destroy(&tkey_i2c->tsk_glove_lock);
#endif
#ifdef TOUCHKEY_BOOSTER
	mutex_destroy(&tkey_i2c->dvfs_lock);
#endif
	destroy_workqueue(tkey_i2c->fw_wq);
err_create_fw_wq:
	sysfs_delete_link(&tkey_i2c->dev->kobj,
		&tkey_i2c->input_dev->dev.kobj, "input");
err_create_symlink:
	sysfs_remove_group(&tkey_i2c->dev->kobj, &touchkey_attr_group);
err_sysfs_init:
	device_destroy(sec_class, (dev_t)NULL);
err_device_create:
#if !defined(CONFIG_KEYBOARD_CYPRESS_TKEY_HL)
err_i2c_check:
#endif	
	tkey_i2c->pdata->power_on(0);
	input_unregister_device(input_dev);
	input_dev = NULL;
err_register_device:
	wake_lock_destroy(&tkey_i2c->fw_wakelock);
	mutex_destroy(&tkey_i2c->irq_lock);
	mutex_destroy(&tkey_i2c->i2c_lock);
	mutex_destroy(&tkey_i2c->lock);
#ifdef TKEY_GRIP_MODE	
	mutex_destroy(&tkey_i2c->grip_mode_lock);	
#endif
	if(input_dev)
	input_free_device(input_dev);
err_allocate_input_device:
	kfree(tkey_i2c);
#if defined(TK_USE_MAX_SUBPM_CONTROL)
err_get_regulator:
	kfree(cyp_supplies);
#endif		
	return ret;
}

void touchkey_shutdown(struct i2c_client *client)
{
	struct touchkey_i2c *tkey_i2c = i2c_get_clientdata(client);

	touchkey_stop(tkey_i2c);
	printk(KERN_DEBUG"touchkey:%s\n", __func__);
}

#ifdef CONFIG_OF
static struct of_device_id cypress_match_table[] = {
	{ .compatible = "cypress,cypress-tkey",},
	{ },
};
#else
#define cypress_match_table	NULL
#endif


struct i2c_driver touchkey_i2c_driver = {
	.driver = {
		.name = "cypress_touchkey", //"sec_touchkey",
		.owner = THIS_MODULE,

		#ifdef CONFIG_OF
		.of_match_table = cypress_match_table,
		#endif
		
		#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE)
		.pm = &touchkey_pm_ops,
		#endif
	},
	.id_table = sec_touchkey_id,
	.probe = i2c_touchkey_probe,
	.shutdown = &touchkey_shutdown,
};

static int __init touchkey_init(void)
{
#ifdef TEST_JIG_MODE
	unsigned char get_touch = 0x40;
#endif

#ifdef CONFIG_SAMSUNG_LPM_MODE
		extern int poweroff_charging;

		if (poweroff_charging) {
			printk(KERN_ERR "%s : LPM Charging Mode!!\n", __func__);
			return 0;
		}
#endif

	i2c_add_driver(&touchkey_i2c_driver);

#ifdef TEST_JIG_MODE
	i2c_touchkey_write(tkey_i2c, KEYCODE_REG, &get_touch, 1);
#endif
	printk(KERN_ERR "%s: init done %d\n", __func__, __LINE__);

	return 0;
}

static void __exit touchkey_exit(void)
{
	i2c_del_driver(&touchkey_i2c_driver);
	touchkey_probe = false;
}

module_init(touchkey_init);
module_exit(touchkey_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("@@@");
MODULE_DESCRIPTION("touch keypad");
