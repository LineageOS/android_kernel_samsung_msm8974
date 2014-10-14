/*
 * cypress_touchkey.c - Platform data for cypress touchkey driver
 *
 * Copyright (C) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define SEC_TOUCHKEY_DEBUG
/* #define SEC_TOUCHKEY_VERBOSE_DEBUG */

#include <linux/kernel.h>
#include <asm/unaligned.h>
//#include <mach/cpufreq.h>
#include <linux/input/mt.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/earlysuspend.h>
#include <linux/i2c/touchkey_i2c.h>
#include <linux/regulator/consumer.h>
#include <asm/mach-types.h>
#include <linux/device.h>
/* #include <mach/msm8974-gpio.h> */
#include <linux/of_gpio.h>
#include <linux/firmware.h>
#include <linux/uaccess.h>

#include "issp_extern.h"
#include "coreriver_extern.h"
#include <linux/mfd/pm8xxx/pm8921.h>
/*
#define CYPRESS_GEN		0X00
#define CYPRESS_FW_VER		0X01
#define CYPRESS_MODULE_VER	0X02
#define CYPRESS_2ND_HOST	0X03
#define CYPRESS_THRESHOLD	0X04
#define CYPRESS_AUTO_CAL_FLG	0X05

#define CYPRESS_IDAC_MENU	0X07
#define CYPRESS_IDAC_BACK	0X06
#define CYPRESS_IDAC_HOME	0X08

#define CYPRESS_DIFF_MENU	0x0C
#define CYPRESS_DIFF_BACK	0x0A
#define CYPRESS_DIFF_HOME	0x0E

#define CYPRESS_RAW_DATA_MENU	0x10
#define CYPRESS_RAW_DATA_BACK	0x0E
#define CYPRESS_RAW_DATA_HOME	0x12
#define CYPRESS_RAW_DATA_BACK_GOGH	0x14

#define CYPRESS_DATA_UPDATE	0X40
#define CYPRESS_AUTO_CAL	0X50
#define CYPRESS_SLEEP		0X80

#define CYPRESS_FW_ID_REG	0X05

#define USE_OPEN_CLOSE
#undef DO_NOT_USE_FUNC_PARAM

#define KEYCODE_REG		0x00
*/
/* bit masks*/
#define PRESS_BIT_MASK		0X08
#define KEYCODE_BIT_MASK	0X07

/*
#define TOUCHKEY_LOG(k, v) dev_notice(&info->client->dev, "key[%d] %d\n", k, v);
#define FUNC_CALLED dev_notice(&info->client->dev, "%s: called.\n", __func__);
*/

#ifdef USE_OPEN_CLOSE
static int cypress_input_open(struct input_dev *dev);
static void cypress_input_close(struct input_dev *dev);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cypress_touchkey_early_suspend(struct early_suspend *h);
static void cypress_touchkey_late_resume(struct early_suspend *h);
#endif

#ifdef TK_INFORM_CHARGER
extern void touchkey_register_callback(void *cb);
#endif

#ifndef USE_SW_I2C
static void cypress_config_gpio_i2c(struct cypress_touchkey_platform_data *pdata, int onoff);
#endif

#ifdef TKEY_REQUEST_FW_UPDATE
static int tkey_load_fw(struct cypress_touchkey_info *info, u8 fw_path);
static int tkey_unload_fw(struct cypress_touchkey_info *info, u8 fw_path);
#endif
static int touchkey_led_status;
static int touchled_cmd_reversed;

#ifdef DO_NOT_USE_FUNC_PARAM
static struct pm_gpio tkey_int = {
	.direction	= PM_GPIO_DIR_IN,
	.pull		= PM_GPIO_PULL_NO,
	.vin_sel	= 2,
	.function	= PM_GPIO_FUNC_NORMAL,
	.inv_int_pol	= 0,
};

static struct pm_gpio tkey_sleep_int = {
	.direction	= PM_GPIO_DIR_IN,
	.pull		= PM_GPIO_PULL_UP_30,
	.vin_sel	= 2,
	.function	= PM_GPIO_FUNC_NORMAL,
	.inv_int_pol	= 0,
};
#endif

#ifdef TSP_BOOSTER
static void cypress_change_dvfs_lock(struct work_struct *work)
{
	struct cypress_touchkey_info *info =
		container_of(work,
			struct cypress_touchkey_info, work_dvfs_chg.work);
	int retval = 0;
	mutex_lock(&info->dvfs_lock);

	retval = set_freq_limit(DVFS_TOUCH_ID, info->dvfs_freq);
	if (retval < 0)
		dev_info(&info->client->dev,
			"%s: booster change failed(%d).\n",
			__func__, retval);
	info->dvfs_lock_status = false;
	mutex_unlock(&info->dvfs_lock);
}

static void cypress_set_dvfs_off(struct work_struct *work)
{
	struct cypress_touchkey_info *info =
		container_of(work,
			struct cypress_touchkey_info, work_dvfs_off.work);
	int retval;

	mutex_lock(&info->dvfs_lock);
	retval = set_freq_limit(DVFS_TOUCH_ID, -1);
	if (retval < 0)
		dev_info(&info->client->dev,
			"%s: booster stop failed(%d).\n",
			__func__, retval);

	info->dvfs_lock_status = true;
	mutex_unlock(&info->dvfs_lock);
}

static void cypress_set_dvfs_lock(struct cypress_touchkey_info *info,
					uint32_t on)
{
	int ret = 0;
	if (info->is_powering_on) {/*0603 - SMD issue*/
		dev_info(&info->client->dev,
				"%s: ignoring dvfs set.\n", __func__);
		return;
	}

	if (info->dvfs_boost_mode == DVFS_STAGE_NONE) {
		dev_dbg(&info->client->dev,
				"%s: DVFS stage is none(%d)\n",
				__func__, info->dvfs_boost_mode);
		return;
	}

	mutex_lock(&info->dvfs_lock);
	if (on == 0) {
			cancel_delayed_work(&info->work_dvfs_chg);

		if (info->dvfs_lock_status) {
			ret = set_freq_limit(DVFS_TOUCH_ID, info->dvfs_freq);
					if (ret < 0)
						dev_info(&info->client->dev,
					"%s: cpu first lock failed(%d)\n", __func__, ret);
			info->dvfs_lock_status = false;
		}

		schedule_delayed_work(&info->work_dvfs_off,
			msecs_to_jiffies(TOUCH_BOOSTER_CHG_TIME));

	} else if (on == 1) {
		cancel_delayed_work(&info->work_dvfs_off);
				schedule_delayed_work(&info->work_dvfs_chg,
				msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));

	} else if (on == 2) {
		if (info->dvfs_lock_status) {
			cancel_delayed_work(&info->work_dvfs_off);
			cancel_delayed_work(&info->work_dvfs_chg);
			schedule_work(&info->work_dvfs_off.work);
		}
	}
	mutex_unlock(&info->dvfs_lock);
}

static void cypress_init_dvfs(struct cypress_touchkey_info *info)
{
	mutex_init(&info->dvfs_lock);
	info->dvfs_boost_mode = DVFS_STAGE_DUAL;
	info->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;

	INIT_DELAYED_WORK(&info->work_dvfs_off, cypress_set_dvfs_off);
	INIT_DELAYED_WORK(&info->work_dvfs_chg, cypress_change_dvfs_lock);

	info->dvfs_lock_status = true;
}
#endif

static int cypress_touchkey_i2c_read(struct i2c_client *client,
		u8 reg, u8 *val, unsigned int len)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(&client->dev);
	int err = 0;
	int retry = 3;

	if (!info->enabled) {
		dev_info(&client->dev, "%s: touchkey is not enabled\n", __func__);
		return 0;
	}

	while (retry--) {
		err = i2c_smbus_read_i2c_block_data(client,
				KEYCODE_REG, len, val);
		if (err >= 0)
			return err;

		dev_info(&client->dev, "%s: i2c transfer error.\n", __func__);
		msleep(20);
	}
	return err;

}

static int cypress_touchkey_i2c_write(struct i2c_client *client,
		u8 *val, unsigned int len)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(&client->dev);
	int err = 0;
	int retry = 3;

	if (!info->enabled) {
		dev_info(&client->dev, "%s: touchkey is not enabled\n", __func__);
		return 0;
	}

	while (retry--) {
		err = i2c_smbus_write_i2c_block_data(client,
				KEYCODE_REG, len, val);
		if (err >= 0)
			return err;

		dev_info(&client->dev, "%s: i2c transfer error.\n", __func__);
		msleep(20);
	}
	return err;
}

#ifdef CYPRESS_SUPPORT_DUAL_INT_MODE
static void cypress_touchkey_interrupt_set_dual(struct i2c_client *client)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(&client->dev);
	int ret = 0;
	int retry = 5;
	u8 data[3] = {0, };

	if (info->touchkeyid != CYPRESS_TOUCHKEY) { /* support CYPRESS only */
		dev_err(&client->dev, "%s: not support this Touchkey IC\n", __func__);
		return;
	}

	if (info->ic_fw_ver < CYPRESS_RECENT_BACK_REPORT_FW_VER) {
		dev_err(&client->dev, "%s: not support this version\n", __func__);
		return;
	}

	while (retry--) {
		data[0] = TK_CMD_DUAL_DETECTION;
		data[1] = 0x00;
		data[2] = TK_BIT_DETECTION_CONFIRM;

		ret = i2c_smbus_write_i2c_block_data(client, TK_CMD_INTERRUPT_SET_REG, 3, &data[0]);
		if (ret < 0) {
			dev_err(&client->dev, "%s: i2c write error. (%d)\n", __func__, ret);
			msleep(30);
			continue;
		}
		msleep(30);

		data[0] = CYPRESS_DETECTION_FLAG;

		ret = i2c_smbus_read_i2c_block_data(client, data[0], 1, &data[1]);
		if (ret < 0) {
			dev_err(&client->dev, "%s: i2c read error. (%d)\n", __func__, ret);
			msleep(30);
			continue;
		}

		if (data[1] != 1) {
			dev_err(&client->dev,
				"%s: interrupt set: 0x%X, failed.\n", __func__, data[1]);
			continue;
		}

		dev_info(&client->dev, "%s: interrupt set: 0x%X\n", __func__, data[1]);
		break;
	}

}
#endif

static int tkey_i2c_check(struct cypress_touchkey_info *info)
{
	struct i2c_client *client = info->client;

	int ret = 0;
	u8 data[3] = { 0, };

	ret = cypress_touchkey_i2c_read(info->client, KEYCODE_REG, data, 4);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to read Module version\n");
		return ret;
	}

	info->ic_fw_ver = data[1];
	info->module_ver = data[2];
	dev_info(&client->dev, "%s: ic_fw_ver = %x, module_ver = %x \n",
		__func__, info->ic_fw_ver, info->module_ver);

#ifdef CYPRESS_SUPPORT_DUAL_INT_MODE
	/* CYPRESS Firmware setting interrupt type : dual or single interrupt */
	cypress_touchkey_interrupt_set_dual(info->client);
#endif

	return ret;
}

void cypress_power_onoff(struct cypress_touchkey_info *info, int onoff)
{
	int ret = 0, rc = 0;

	dev_info(&info->client->dev, "%s: power %s\n",
			__func__, onoff ? "on" : "off");

	if (info->pdata->vcc_en < 0) {
		if (!info->vcc_en) {
			info->vcc_en = regulator_get(&info->client->dev,
				"vcc_en");
			if (IS_ERR(info->vcc_en)) {
				dev_err(&info->client->dev,
					"Regulator(vcc_en) get failed rc = %ld\n", PTR_ERR(info->vcc_en));
				return;
			}
		}
	}

	if(!info->pdata->fw_update_flag) {
		if (info->pdata->vdd_led < 0) {
			if (!info->vdd_led) {
				info->vdd_led = regulator_get(&info->client->dev,
					"vdd_led");
				if (IS_ERR(info->vdd_led)) {
					dev_err(&info->client->dev,
						"Regulator(vdd_led) get failed rc = %ld\n", PTR_ERR(info->vdd_led));
					return;
				}

				rc = regulator_set_voltage(info->vdd_led,
					3300000, 3300000);
				if (rc) {
					dev_err(&info->client->dev,
						"regulator(vdd_led) set_vtg failed rc=%d\n", rc);
					return;
				}
			}
		}
	}

	if (onoff) {
		if (info->pdata->vcc_en < 0) {
			if (!regulator_is_enabled(info->vcc_en)) {
				rc = regulator_enable(info->vcc_en);
				if (rc) {
					dev_err(&info->client->dev,
						"Regulator vcc_en enable failed rc=%d\n", rc);
					return;
				}
			}
		}
		if(!info->pdata->fw_update_flag) {
			if (info->pdata->vdd_led < 0) {
				if (!regulator_is_enabled(info->vdd_led)) {
					rc = regulator_enable(info->vdd_led);
					if (rc) {
						dev_err(&info->client->dev,
							"Regulator vdd_led enable failed rc=%d\n", rc);
						return;
					}
				}
			}
		}
	} else {
		if (info->pdata->vcc_en < 0) {
			if (regulator_is_enabled(info->vcc_en)) {
				rc = regulator_disable(info->vcc_en);
				if (rc) {
					dev_err(&info->client->dev,
						"Regulator vcc_en disable failed rc=%d\n", rc);
					return;
				}
			}
		}
		if(!info->pdata->fw_update_flag) {
			if (info->pdata->vdd_led < 0) {
				if (regulator_is_enabled(info->vdd_led)) {
					rc = regulator_disable(info->vdd_led); 
					if (rc) {
						dev_err(&info->client->dev,
							"Regulator vdd_led disable failed rc=%d\n", rc);
						return;
					}
				}
			}
		}
	}

	if (info->pdata->vdd_led > 0) {
		ret = gpio_direction_output(info->pdata->vdd_led, onoff);
		if (ret) {
			dev_err(&info->client->dev,
					"%s: unable to set_direction for vdd_led [%d]\n",
					__func__, info->pdata->vdd_led);
		}
	}

	if (info->pdata->vcc_en > 0) {
		ret = gpio_direction_output(info->pdata->vcc_en, onoff);
		if (ret) {
			dev_err(&info->client->dev,
					"%s: unable to set_direction for vcc_en [%d]\n",
					__func__, info->pdata->vcc_en);
		}
	}
	
}

#ifdef LED_LDO_WITH_REGULATOR
static void change_touch_key_led_voltage(struct cypress_touchkey_info *info, int vol_mv)
{
	struct regulator *tled_regulator;
	int ret;
	vol_mv_level = vol_mv;

	tled_regulator = regulator_get(NULL, "8921_l10");
	if (IS_ERR(tled_regulator)) {
		dev_info(&info->client->dev,
				"%s: failed to get resource touch_led\n", __func__);
		return;
	}
	ret = regulator_set_voltage(tled_regulator,
		vol_mv * 100000, vol_mv * 100000);
	if (ret)
		dev_info(&info->client->dev,
				"%s: error setting voltage\n", __func__);

	regulator_put(tled_regulator);
}

static ssize_t cypress_touchkey_brightness_control(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);

	int data;

	if (sscanf(buf, "%d\n", &data) == 1)
		change_touch_key_led_voltage(info, data);
	else
		dev_info(&info->client->dev, "%s: Error[%d]\n",
				__func__, data);

	return size;
}
#endif

#ifdef TK_INFORM_CHARGER
static int touchkey_ta_setting(struct cypress_touchkey_info *info)
{
	u8 data[6] = { 0, };
	int count = 0;
	int ret = 0;
	u8 retry = 0;

	while (retry < 3) {
		ret = cypress_touchkey_i2c_read(info->client, KEYCODE_REG, data, 4);
		if (ret < 0) {
			dev_err(&info->client->dev, "Failed to read Keycode_reg.\n");
			return ret;
		}

		/* Send autocal Command */
		if (info->charging_mode) {
			data[0] = 0x90;
			data[3] = 0x10;
		} else {
			data[0] = 0x90;
			data[3] = 0x20;
		}

		count = cypress_touchkey_i2c_write(info->client, data, 4);
		msleep(100);

		/* Check autocal status */
		ret = cypress_touchkey_i2c_read(info->client, KEYCODE_REG, data, 6);

		if (info->charging_mode) {
			if (data[5] & TK_BIT_TA_ON) {
				dev_dbg(&info->client->dev, "%s: TA mode is Enabled\n", __func__);
				break;
			} else {
				dev_err(&info->client->dev, "%s: TA Enabled Error! retry=%d\n",
					__func__, retry);
			}
		} else {
			if (!(data[5] & TK_BIT_TA_ON)) {
				dev_dbg(&info->client->dev, "%s: TA mode is Disabled\n", __func__);				
				break;
			} else {
				dev_err(&info->client->dev, "%s: TA Disabled Error! retry=%d\n",
					__func__, retry);
			}
		}
		retry = retry + 1;
	}

	if (retry == 3)
		dev_err(&info->client->dev, "%s: Failed to set the TA mode\n", __func__);

	return count;
}

static void cypress_touchkey_ta_cb(struct touchkey_callbacks *cb, bool ta_status)
{
	struct cypress_touchkey_info *info =
			container_of(cb, struct cypress_touchkey_info, callbacks);

	info->charging_mode = ta_status;

	if (!(info->enabled)) {
		dev_info(&info->client->dev, "%s: Touchkey is not enabled.\n",
				__func__);
		info->done_ta_setting = false;
		return;
	}

	dev_info(&info->client->dev, "%s: TA %s\n",
			__func__, info->charging_mode ? "connected" : "disconnected");
	touchkey_ta_setting(info);
}
#endif

#ifdef CONFIG_GLOVE_TOUCH
static void cypress_touchkey_glove_work(struct work_struct *work)
{
	struct cypress_touchkey_info *info =
		container_of(work, struct cypress_touchkey_info, glove_work);

	u8 data[6] = { 0, };
	int count = 0;
	int ret = 0;
	u8 retry = 0;

	while (retry < 3) {
		ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 4);
		if (ret < 0) {
			dev_err(&info->client->dev, "%s: Failed to read Keycode_reg.\n",
				__func__);
			return;
		}

		if (info->glove_value == 1) {
			/* Send glove Command */
			data[0] = 0xA0;
			data[3] = 0x30;
		} else {
			data[0] = 0xA0;
			data[3] = 0x40;
		}

		count = cypress_touchkey_i2c_write(info->client, data, 4);

		msleep(50);

		/* Check autocal status */
		ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 6);

		if (info->glove_value == 1) {
			if (data[5] & TK_BIT_GLOVE) {
				dev_dbg(&info->client->dev, "%s: Glove mode is enabled\n", __func__);
				break;
			} else {
				dev_err(&info->client->dev, "%s: glove_mode Error! retry=%d\n",
					__func__, retry);
			}
		} else {
			if (!(data[5] & TK_BIT_GLOVE)) {
				dev_dbg(&info->client->dev, "%s: Normal mode from Glove mode\n", __func__);				
				break;
			} else {
				dev_err(&info->client->dev, "%s: normal_mode Error! retry=%d\n",
					__func__, retry);
			}
		}
		retry = retry + 1;
	}

	if (retry == 3)
		dev_err(&info->client->dev, "%s: Failed to set the glove mode\n", __func__);

	return;
}

int cypress_touchkey_glovemode(struct cypress_touchkey_info *info, int value)
{
	if (!(info->enabled)) {
		dev_info(&info->client->dev, "%s Touchkey is not enabled.\n",
				__func__);
		return 0;
		}

	if (info->glove_value == value) {
		dev_info(&info->client->dev, "glove mode is already %s\n", value ? "enabled" : "disabled");
		return 0;
	}

	info->glove_value = value;

	queue_work(info->glove_wq, &info->glove_work);

	return 0;
}
#endif

#ifdef TKEY_FLIP_MODE
void cypress_touchkey_flip_cover(struct cypress_touchkey_info *info, int value)
{
	u8 data[6] = { 0, };
	int ret = 0;
	u8 retry = 0;

	if (!(info->enabled)) {
		dev_info(&info->client->dev, "%s : Touchkey is not enabled.\n",
				__func__);
		return ;
	}

	while (retry < 3) {
		ret = cypress_touchkey_i2c_read(info->client, KEYCODE_REG, data, 4);
			if (ret <= 0) {
			dev_info(&info->client->dev, "%s: Failed to read Keycode_reg.\n",
					__func__);
				return;
			}

			if (value == 1) {
				/* Send filp mode Command */
					data[0] = 0xA0;
					data[3] = 0x50;
			} else {
					data[0] = 0xA0;
					data[3] = 0x40;
			}

		ret = cypress_touchkey_i2c_write(info->client, data, 4);
		if (ret < 0) {
			dev_info(&info->client->dev, "%s: Failed to write flip mode command.\n",
					__func__);
			return;
		}

		msleep(100);

			/* Check autocal status */
		ret = cypress_touchkey_i2c_read(info->client, KEYCODE_REG, data, 6);
		if (ret <= 0) {
			dev_info(&info->client->dev, "%s: Failed to check autocal status.\n",
					__func__);
			return;
		}

		dev_dbg(&info->client->dev,
				"data[5]=%x",data[5] & TK_BIT_FLIP);

		if (value == 1) {
			if (data[5] & TK_BIT_FLIP) {					
				dev_dbg(&info->client->dev, "%s: Flip mode is enabled\n", __func__);
				info->enabled_flip = true;
				break;
			} else {
				dev_err(&info->client->dev, "%s: flip_mode Enable failed. retry=%d\n",
					__func__, retry);
			}
		} else {
			if (!(data[5] & TK_BIT_FLIP)) {
				dev_dbg(&info->client->dev, "%s: Normal mode form Flip mode\n", __func__);					
				info->enabled_flip = false;
				break;
			} else {
				dev_info(&info->client->dev, "%s: normal_mode Enable failed. retry=%d \n",
					__func__, retry);
			}
		}
		retry = retry + 1;
	}

	if (retry == 3)
		dev_err(&info->client->dev, "[Touchkey] flip cover failed\n");

	return;
}
#endif

static irqreturn_t cypress_touchkey_interrupt(int irq, void *dev_id)
{
	struct cypress_touchkey_info *info = dev_id;
	u8 buf[10] = {0,};
	int code;
	int press;
	int ret;
	int i;

	if (!atomic_read(&info->keypad_enable)) {
		goto out;
	}

	ret = gpio_get_value(info->pdata->gpio_int);
	if (ret) {
		dev_info(&info->client->dev,
				"%s: not real interrupt (%d).\n", __func__, ret);
		goto out;
	}

	if (info->is_powering_on) {
		dev_info(&info->client->dev,
				"%s: ignoring spurious boot interrupt\n", __func__);
		goto out;
	}

	buf[0] = i2c_smbus_read_byte_data(info->client, CYPRESS_GEN);
	if (buf[0] < 0) {
		dev_info(&info->client->dev, "%s: interrupt failed with %d.\n",
				__func__, ret);
		goto out;
	}

	if (info->ic_fw_ver >= CYPRESS_RECENT_BACK_REPORT_FW_VER) {
		int menu_data = buf[0] & 0x3;
		int back_data = (buf[0] >> 2) & 0x3;
		u8 menu_press = menu_data % 2;
		u8 back_press = back_data % 2;

		if (menu_data)
			input_report_key(info->input_dev, info->keycode[0], menu_press);
		if (back_data)
			input_report_key(info->input_dev, info->keycode[1], back_press);

		press = menu_press | back_press;

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
		dev_info(&info->client->dev,
				"%s: %s%s%X, fw_ver: 0x%x, modue_ver: 0x%x\n", __func__,
				menu_data ? (menu_press ? "menu P " : "menu R ") : "",
				back_data ? (back_press ? "back P " : "back R ") : "",
				buf[0], info->ic_fw_ver, info->module_ver);
#else
		dev_info(&info->client->dev, "%s: key %s%s fw_ver: 0x%x, modue_ver: 0x%x\n", __func__,
				menu_data ? (menu_press ? "P" : "R") : "",
				back_data ? (back_press ? "P" : "R") : "",
				info->ic_fw_ver, info->module_ver);
#endif
	} else {
		press = !(buf[0] & PRESS_BIT_MASK);
		code = (int)(buf[0] & KEYCODE_BIT_MASK) - 1;

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
		dev_info(&info->client->dev,
				"%s: code=%d %s. fw_ver=0x%x, module_ver=0x%x \n", __func__,
				code, press ? "pressed" : "released", info->ic_fw_ver, info->module_ver);
#else
		dev_info(&info->client->dev,
				"%s: %s. fw_ver=0x%x, module_ver=0x%x \n", __func__,
				press ? "pressed" : "released", info->ic_fw_ver, info->module_ver);
#endif
		if (code < 0) {
			dev_info(&info->client->dev,
					"%s, not profer interrupt 0x%2X.(release all finger)\n",
					__func__, buf[0]);
			/* need release all finger function. */
			for (i = 0; i < info->pdata->keycodes_size; i++) {
				input_report_key(info->input_dev, info->keycode[i], 0);
				input_sync(info->input_dev);
			}
			goto out;
		}

		input_report_key(info->input_dev, info->keycode[code], press);
	}

	input_sync(info->input_dev);
#ifdef TSP_BOOSTER
	cypress_set_dvfs_lock(info, !!press);
#endif

out:
	return IRQ_HANDLED;
}

static void cypress_touchkey_con_hw(struct cypress_touchkey_info *dev_info,
								bool flag)
{
	struct cypress_touchkey_info *info =  dev_info;

	if (info->pdata->vdd_led > 0)
		gpio_set_value(info->pdata->vdd_led, flag ? 1 : 0);

#if defined(SEC_TOUCHKEY_DEBUG)
	dev_notice(&info->client->dev,
			"%s : called with flag %d.\n", __func__, flag);
#endif
}

static int cypress_touchkey_auto_cal(struct cypress_touchkey_info *info, bool booting)
{
	u8 data[6] = { 0, };
	int count = 0;
	int ret = 0;
	u8 retry = 0;

	while (retry < 3) {
		ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 4);
			if (ret < 0) {
				dev_info(&info->client->dev, "%s: autocal read fail.\n", __func__);
				return ret;
			}

		data[0] = 0x50;
		data[3] = 0x01;

		count = cypress_touchkey_i2c_write(info->client, data, 4);
		dev_info(&info->client->dev,
				"%s: data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
				__func__, data[0], data[1], data[2], data[3]);

		if (booting)
			msleep(100);
		else
			msleep(130);

		ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 6);

		if (data[5] & TK_BIT_AUTOCAL) {
			dev_dbg(&info->client->dev, "%s: Run Autocal\n", __func__);
			break;
		} else {
			dev_info(&info->client->dev, "%s: autocal failed[%x][%d]\n",
					__func__, data[5], ret);
		}
		retry = retry + 1;
	}

	if (retry == 3)
		dev_err(&info->client->dev, "%s: Failed to Set the Autocalibration\n", __func__);

	return count;
}

static int tkey_fw_update(struct cypress_touchkey_info *info, bool force)
{
	struct i2c_client *client = info->client;
	int retry;
	int ret = 0;

	dev_info(&client->dev, "%s : touchkey_update Start!!\n", __func__);

	if (force == true)
		retry = 2;
	else
		retry = NUM_OF_RETRY_UPDATE;
	
#ifndef USE_SW_I2C
	cypress_config_gpio_i2c(info->pdata, 0);
#endif

	while (retry--) {
		if (ISSP_main(info) == 0) {
			dev_info(&client->dev, "%s : touchkey_update pass!!\n", __func__);
			msleep(50);
			cypress_touchkey_auto_cal(info, false);	
			break;
		}
		msleep(50);
		dev_err(&client->dev,
			"%s : touchkey_update failed... retry...\n", __func__);
	}
	
#ifndef USE_SW_I2C
	cypress_config_gpio_i2c(info->pdata, 1);
#endif

	if (retry <= 0) {
		dev_err(&client->dev, "%s : touchkey_update fail\n", __func__);
		return -1;
	}

	msleep(500);

	info->ic_fw_ver = i2c_smbus_read_byte_data(info->client,
			CYPRESS_FW_VER);
	dev_info(&client->dev,
		"%s : FW Ver 0x%02x\n", __func__, info->ic_fw_ver);

#ifdef CYPRESS_SUPPORT_DUAL_INT_MODE
	/* CYPRESS Firmware setting interrupt type : dual or single interrupt */
	cypress_touchkey_interrupt_set_dual(info->client);
#endif
	return ret;
}

static ssize_t cypress_touchkey_ic_version_read(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int count;

	dev_info(&info->client->dev,
			"%s : FW IC Ver 0x%02x\n", __func__, info->ic_fw_ver);

	count = snprintf(buf, 20, "0x%02x\n", info->ic_fw_ver);
	return count;
}

static ssize_t cypress_touchkey_src_version_read(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int count;
	dev_info(&info->client->dev,
			"%s : FW src ver 0x%02x\n", __func__, info->src_fw_ver);

	count = snprintf(buf, 20, "0x%02x\n", info->src_fw_ver);
	return count;
}

static ssize_t cypress_touchkey_firm_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int count = 0;
	char buff[16] = {0};
	dev_info(&info->client->dev, "%s touchkey_update_status: %d\n",
			__func__, info->touchkey_update_status);
	if (info->touchkey_update_status == DOWNLOADING)
		count = snprintf(buff, sizeof(buff), "Downloading\n");
	else if (info->touchkey_update_status == UPDATE_FAIL)
		count = snprintf(buff, sizeof(buff), "FAIL\n");
	else if (info->touchkey_update_status == UPDATE_PASS)
		count = snprintf(buff, sizeof(buff), "PASS\n");
	return count;
}

static ssize_t cypress_touchkey_update_write(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf, size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->client;	
	int count = 0;
#ifdef TKEY_REQUEST_FW_UPDATE
	u8 fw_path;
#endif
	dev_info(&info->client->dev, "%s \n", __func__);

#ifdef TKEY_REQUEST_FW_UPDATE
	switch (*buf) {
		case 's':
		case 'S':
			fw_path = FW_BUILT_IN;
#if !defined(CONFIG_MACH_JS01LTEDCM) && !defined(CONFIG_MACH_JS01LTESBM)
			if(info->touchkeyid == CYPRESS_TOUCHKEY && info->support_fw_update == false) {
				dev_err(&client->dev, "%s: module %x does not support fw update\n.", __func__, info->module_ver);
				return size;
			}
#endif
			break;

		case 'i':
		case 'I':
			fw_path = FW_IN_SDCARD;
			break;

		default:
			dev_err(&client->dev, "%s: invalid parameter %c\n.", __func__,
				*buf);
			return -EINVAL;
	}
	count = tkey_load_fw(info, fw_path);
	if (count < 0) {
		dev_err(&client->dev, "fail to load fw in %d (%d)\n",
			fw_path, count);
		return count;
	}
#endif

	info->touchkey_update_status = DOWNLOADING;
	disable_irq(info->irq);

	if (info->touchkeyid == CYPRESS_TOUCHKEY)
		count = tkey_fw_update(info, false);
	else
		count = coreriver_fw_update(info, false);
	if (count < 0) {
		if (info->pdata->gpio_led_en)
			cypress_touchkey_con_hw(info, false);		
		cypress_power_onoff(info, 0);
		info->touchkey_update_status = UPDATE_FAIL;
		dev_err(&client->dev, "%s: fail to flash fw (%d)\n.", __func__, count);
		return count;
	}

	info->touchkey_update_status = UPDATE_PASS;
#ifdef TKEY_REQUEST_FW_UPDATE
	tkey_unload_fw(info, fw_path);
#endif
	enable_irq(info->irq);

	return size;
}

#if defined(CONFIG_SEC_KLIMT_PROJECT)
static ssize_t cypress_touchkey_led(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int data;
	int ret;

	dev_info(&info->client->dev, "called %s\n", __func__);
	ret = sscanf(buf, "%d", &data);

	if (ret != 1) {
		dev_err(&info->client->dev, "%s, %d err\n",
			__func__, __LINE__);
		return size;
	}

	if (data != 0 && data != 1) {
		dev_err(&info->client->dev, "%s wrong cmd %x\n",
			__func__, data);
		return size;
	}

	if (info->pdata->vdd_led > 0) {
		if (!info->enabled) {
			touchled_cmd_reversed = 1;
			goto out;
		}
	}

	if (!info->vdd_led) {
		info->vdd_led = regulator_get(&info->client->dev, "vdd_led");
		if (IS_ERR(info->vdd_led)) {
			dev_err(&info->client->dev,
				"Regulator(vdd_led) get failed rc = %ld\n", PTR_ERR(info->vdd_led));
			goto out;
		}

		ret = regulator_set_voltage(info->vdd_led, 3300000, 3300000);
		if (ret) {
			dev_err(&info->client->dev,
				"regulator(vdd_led) set_vtg failed %d\n", ret);
			goto out;
		}
	}

	if (data) {
		if (!regulator_is_enabled(info->vdd_led)) {
			ret = regulator_enable(info->vdd_led);
			if (ret) {
				dev_err(&info->client->dev,
					"Regulator vdd_led enable failed %d\n", ret);
				goto out;
			}
		}
	} else {
		if (regulator_is_enabled(info->vdd_led)) {
			ret = regulator_disable(info->vdd_led); 
			if (ret) {
				dev_err(&info->client->dev,
					"Regulator vdd_led disable failed %d\n", ret);
				goto out;
			}
		}
	}

#if defined(SEC_TOUCHKEY_DEBUG)
	dev_info(&info->client->dev,
		"touch_led_control : %d\n", data);
#endif

	msleep(30);
out:
	return size;
}
#else
static ssize_t cypress_touchkey_led_control(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int data;
	int ret;	
	static const int ledCmd[] = {TK_CMD_LED_OFF, TK_CMD_LED_ON};

	dev_info(&info->client->dev, "called %s\n", __func__);
	ret = sscanf(buf, "%d", &data);

	if (ret != 1) {
		dev_err(&info->client->dev, "%s, %d err\n",
			__func__, __LINE__);
		return size;
	}

	if (data != 0 && data != 1) {
		dev_err(&info->client->dev, "%s wrong cmd %x\n",
			__func__, data);
		return size;
	}

	if (!info->enabled) {
		touchled_cmd_reversed = 1;
		goto out;
	}

	ret = i2c_smbus_write_byte_data(info->client, CYPRESS_GEN, ledCmd[data]);
	if (ret < 0) {
		dev_info(&info->client->dev,
				"%s: i2c write error [%d]\n", __func__, ret);
		touchled_cmd_reversed = 1;
		goto out;
	}

#if defined(SEC_TOUCHKEY_DEBUG)
	dev_info(&info->client->dev,
		"touch_led_control : %d\n", data);
#endif

	msleep(30);

out:
	touchkey_led_status =  ledCmd[data];

	return size;
}
#endif

static ssize_t cypress_touchkey_sensitivity_control(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int ret;

	ret = i2c_smbus_write_byte_data(info->client,
			CYPRESS_GEN, CYPRESS_DATA_UPDATE);
	if (ret < 0) {
		dev_info(&info->client->dev,
			"[Touchkey] fail to CYPRESS_DATA_UPDATE.\n");
		return ret;
	}
	msleep(20);
	return size;
}

static ssize_t cypress_touchkey_menu_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 menu_sensitivity;
	u8 data[14] = { 0, };
	int ret;

	unsigned char sensitivity_control = 0x40;
	cypress_touchkey_i2c_write(info->client, &sensitivity_control, 1);

	ret = cypress_touchkey_i2c_read(info->client, KEYCODE_REG, data, 14);

	dev_dbg(&info->client->dev, "called %s data[10] =%d,data[11] = %d\n", __func__,
	       data[10], data[11]);

	menu_sensitivity = ((0x00FF & data[10]) << 8) | data[11];

	return snprintf(buf, 20, "%d\n", menu_sensitivity);
}

static ssize_t cypress_touchkey_back_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 back_sensitivity;
	u8 data[14] = { 0, };
	int ret;

	unsigned char sensitivity_control = 0x40;
	cypress_touchkey_i2c_write(info->client, &sensitivity_control, 1);

	ret = cypress_touchkey_i2c_read(info->client, KEYCODE_REG, data, 14);

	dev_dbg(&info->client->dev, "called %s data[12] =%d,data[13] = %d\n", __func__,
	       data[12], data[13]);

	back_sensitivity = ((0x00FF & data[12]) << 8) | data[13];

	return snprintf(buf, 20, "%d\n", back_sensitivity);
}

static ssize_t cypress_touchkey_raw_data0_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 raw_data0;
	u8 data[2] = {0,};
	int ret;
	bool touchkey;

	touchkey = info->pdata->touchkey_order;
	if (0) {
		ret = i2c_smbus_read_i2c_block_data(info->client,
			CYPRESS_RAW_DATA_MENU, ARRAY_SIZE(data), data);

	} else {
		ret = i2c_smbus_read_i2c_block_data(info->client,
		touchkey ? CYPRESS_RAW_DATA_MENU : CYPRESS_RAW_DATA_BACK,
		ARRAY_SIZE(data), data);
	}
	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"[TouchKey] fail to read MENU raw data.\n");
		return ret;
	}

	raw_data0 = ((0x00FF & data[0])<<8) | data[1];

	dev_dbg(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", raw_data0);

}

static ssize_t cypress_touchkey_raw_data1_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 raw_data1;
	u8 data[2] = {0,};
	int ret;
	bool touchkey;

	touchkey = info->pdata->touchkey_order;
	if (0) {
		ret = i2c_smbus_read_i2c_block_data(info->client,
			CYPRESS_RAW_DATA_BACK_GOGH, ARRAY_SIZE(data), data);
	} else {
		ret = i2c_smbus_read_i2c_block_data(info->client,
		touchkey ? CYPRESS_RAW_DATA_BACK : CYPRESS_RAW_DATA_MENU,
		ARRAY_SIZE(data), data);
	}
	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"[TouchKey] fail to read HOME raw data.\n");
		return ret;
	}

	raw_data1 = ((0x00FF & data[0])<<8) | data[1];

	dev_info(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", raw_data1);

}

static ssize_t cypress_touchkey_idac0_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 idac0;
	u8 data = 0;
	bool touchkey;

	touchkey = info->pdata->touchkey_order;
	if (0) {
		data = i2c_smbus_read_byte_data(info->client,
					CYPRESS_IDAC_MENU);
	} else {
		data = i2c_smbus_read_byte_data(info->client,
			touchkey ? CYPRESS_IDAC_MENU : CYPRESS_IDAC_BACK);
	}
	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	idac0 = data;
	return snprintf(buf, 20, "%d\n", idac0);

}

static ssize_t cypress_touchkey_idac1_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 idac1;
	u8 data = 0;
	bool touchkey;

	touchkey = info->pdata->touchkey_order;
	if (0) {
		data = i2c_smbus_read_byte_data(info->client,
					CYPRESS_IDAC_BACK);
	} else {
		data = i2c_smbus_read_byte_data(info->client,
			touchkey ? CYPRESS_IDAC_BACK : CYPRESS_IDAC_MENU);
	}
	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	idac1 = data;
	return snprintf(buf, 20, "%d\n", idac1);

}

#if defined(TK_HOME_ENABLE)
static ssize_t cypress_touchkey_home_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 home_sensitivity;
	u8 data[2] = {0,};
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CYPRESS_DIFF_MENU, ARRAY_SIZE(data), data);
	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"[TouchKey] fail to read home sensitivity.\n");
		return ret;
	}

	home_sensitivity = ((0x00FF & data[0])<<8) | data[1];

	dev_info(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", home_sensitivity);

}
static ssize_t cypress_touchkey_raw_data2_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 raw_data1;
	u8 data[2] = {0,};
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CYPRESS_RAW_DATA_HOME, ARRAY_SIZE(data), data);

	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"[TouchKey] fail to read HOME raw data.\n");
		return ret;
	}

	raw_data1 = ((0x00FF & data[0])<<8) | data[1];

	dev_info(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", raw_data1);

}
static ssize_t cypress_touchkey_idac2_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 idac1;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_IDAC_HOME);

	dev_dbg(&info->client->dev, "called %s , data : %d\n", __func__, data);
	idac1 = data;
	return snprintf(buf, 20, "%d\n", idac1);

}
#endif
static ssize_t cypress_touchkey_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 touchkey_threshold;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_THRESHOLD);

	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	touchkey_threshold = data;
	return snprintf(buf, 20, "%d\n", touchkey_threshold);
}

static ssize_t cypress_touchkey_autocal_testmode(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int count = 0;
	int on_off;
	if (sscanf(buf, "%d\n", &on_off) == 1) {
		dev_info(&info->client->dev, "[TouchKey] Test Mode : %d\n", on_off);
		if (on_off == 1) {
		count = i2c_smbus_write_byte_data(info->client,
			CYPRESS_GEN, CYPRESS_DATA_UPDATE);
		}
	} else {
		if (info->pdata->gpio_led_en) {
			cypress_touchkey_con_hw(info, false);
			msleep(50);
			cypress_touchkey_con_hw(info, true);
			msleep(50);
		}
		cypress_touchkey_auto_cal(info, false);
	}

	return count;
}

static ssize_t cypress_touchkey_autocal_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int data;

	sscanf(buf, "%d\n", &data);
	dev_dbg(&info->client->dev, "%s %d\n", __func__, data);

	if (data == 1)
		cypress_touchkey_auto_cal(info, false);
	return 0;
}

static ssize_t cypress_touchkey_autocal_status(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	u8 data[6];
	int ret;
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);

	dev_dbg(&info->client->dev, "%s\n", __func__);

	ret = i2c_smbus_read_i2c_block_data(info->client,
				CYPRESS_GEN, 6, data);
	if ((data[5] & 0x80))
		return snprintf(buf, 20, "Enabled\n");
	else
		return snprintf(buf, 20, "Disabled\n");
}

#if defined(CONFIG_GLOVE_TOUCH)
static ssize_t cypress_touchkey_glove_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int data;

	sscanf(buf, "%d\n", &data);
	dev_info(&info->client->dev, "%s %d\n", __func__, data);

	cypress_touchkey_glovemode(info, data);

	return size;
}
#endif

#ifdef TKEY_FLIP_MODE
static ssize_t cypress_touchkey_flip_cover_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int data;

	sscanf(buf, "%d\n", &data);
	dev_info(&info->client->dev, "%s %d\n", __func__, data);

	cypress_touchkey_flip_cover(info, data);

	return size;
}
#endif


#ifdef TSP_BOOSTER
static ssize_t boost_level_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int val, retval;

	dev_info(&info->client->dev, "%s\n", __func__);
	sscanf(buf, "%d", &val);

	if (val != 1 && val != 2 && val != 0) {
		dev_info(&info->client->dev,
			"%s: wrong cmd %d\n", __func__, val);
		return count;
	}
	info->dvfs_boost_mode = val;
	dev_info(&info->client->dev,
			"%s: dvfs_boost_mode = %d\n",
			__func__, info->dvfs_boost_mode);

	if (info->dvfs_boost_mode == DVFS_STAGE_DUAL) {
		info->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
		dev_info(&info->client->dev,
			"%s: boost_mode DUAL, dvfs_freq = %d\n",
			__func__, info->dvfs_freq);
	} else if (info->dvfs_boost_mode == DVFS_STAGE_SINGLE) {
		info->dvfs_freq = MIN_TOUCH_LIMIT;
		dev_info(&info->client->dev,
			"%s: boost_mode SINGLE, dvfs_freq = %d\n",
			__func__, info->dvfs_freq);
	} else if (info->dvfs_boost_mode == DVFS_STAGE_NONE) {
		info->dvfs_freq = -1;
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		if (retval < 0) {
			dev_err(&info->client->dev,
					"%s: booster stop failed(%d).\n",
					__func__, retval);
			info->dvfs_lock_status = false;
		}
	}
	return count;
}
#endif

static ssize_t sec_keypad_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", atomic_read(&info->keypad_enable));
}

static ssize_t sec_keypad_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int i;

	unsigned int val = 0;
	sscanf(buf, "%d", &val);
	val = (val == 0 ? 0 : 1);
	atomic_set(&info->keypad_enable, val);
	if (val) {
		for (i = 0; i < ARRAY_SIZE(info->keycode); i++)
			set_bit(info->keycode[i], info->input_dev->keybit);
	} else {
		for (i = 0; i < ARRAY_SIZE(info->keycode); i++)
			clear_bit(info->keycode[i], info->input_dev->keybit);
	}
	input_sync(info->input_dev);

	return count;
}

static DEVICE_ATTR(keypad_enable, S_IRUGO|S_IWUSR, sec_keypad_enable_show,
	      sec_keypad_enable_store);
static DEVICE_ATTR(touchkey_firm_update_status, S_IRUGO | S_IWUSR | S_IWGRP,
		cypress_touchkey_firm_status_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO,
		cypress_touchkey_ic_version_read, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO,
		cypress_touchkey_src_version_read, NULL);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, cypress_touchkey_update_write);
static DEVICE_ATTR(touch_sensitivity, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, cypress_touchkey_sensitivity_control);
#if defined(CONFIG_SEC_KLIMT_PROJECT)
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, cypress_touchkey_led);
static DEVICE_ATTR(touchkey_recent, S_IRUGO,
		cypress_touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_recent_raw, S_IRUGO,
		cypress_touchkey_raw_data0_show, NULL);
static DEVICE_ATTR(touchkey_idac3, S_IRUGO,
		cypress_touchkey_idac0_show, NULL);
static DEVICE_ATTR(touchkey_back_raw, S_IRUGO,
		cypress_touchkey_raw_data1_show, NULL);
#else
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, cypress_touchkey_led_control);
static DEVICE_ATTR(touchkey_menu, S_IRUGO,
		cypress_touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_raw_data0, S_IRUGO,
		cypress_touchkey_raw_data0_show, NULL);
static DEVICE_ATTR(touchkey_idac0, S_IRUGO,
		cypress_touchkey_idac0_show, NULL);
static DEVICE_ATTR(touchkey_raw_data1, S_IRUGO,
		cypress_touchkey_raw_data1_show, NULL);
#endif
static DEVICE_ATTR(touchkey_back, S_IRUGO,
		cypress_touchkey_back_show, NULL);
static DEVICE_ATTR(touchkey_idac1, S_IRUGO,
		cypress_touchkey_idac1_show, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO,
		cypress_touchkey_threshold_show, NULL);
static DEVICE_ATTR(touchkey_autocal_start, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, cypress_touchkey_autocal_testmode);
static DEVICE_ATTR(autocal_enable, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		cypress_touchkey_autocal_enable);
static DEVICE_ATTR(autocal_stat, S_IRUGO | S_IWUSR | S_IWGRP,
		cypress_touchkey_autocal_status, NULL);
#ifdef LED_LDO_WITH_REGULATOR
static DEVICE_ATTR(touchkey_brightness_level, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, cypress_touchkey_brightness_control);
#endif
#if defined(CONFIG_GLOVE_TOUCH)
static DEVICE_ATTR(glove_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		cypress_touchkey_glove_mode_enable);
#endif

#ifdef TKEY_FLIP_MODE
static DEVICE_ATTR(flip_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		cypress_touchkey_flip_cover_mode_enable);
#endif
#ifdef TSP_BOOSTER
static DEVICE_ATTR(boost_level,
		   S_IWUSR | S_IWGRP, NULL, boost_level_store);
#endif

static struct attribute *touchkey_attributes[] = {
	&dev_attr_keypad_enable.attr,
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_brightness.attr,
	&dev_attr_touch_sensitivity.attr,
#if defined(CONFIG_SEC_KLIMT_PROJECT)
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_recent_raw.attr,
	&dev_attr_touchkey_idac3.attr,
	&dev_attr_touchkey_back_raw.attr,
#else
	&dev_attr_touchkey_menu.attr,
	&dev_attr_touchkey_raw_data0.attr,
	&dev_attr_touchkey_idac0.attr,
	&dev_attr_touchkey_raw_data1.attr,
#endif
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_idac1.attr,
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_touchkey_autocal_start.attr,
	&dev_attr_autocal_enable.attr,
	&dev_attr_autocal_stat.attr,
#ifdef LED_LDO_WITH_REGULATOR
	&dev_attr_touchkey_brightness_level.attr,
#endif	
#ifdef CONFIG_GLOVE_TOUCH
	&dev_attr_glove_mode.attr,
#endif
#ifdef TKEY_FLIP_MODE
	&dev_attr_flip_mode.attr,
#endif
#ifdef TSP_BOOSTER
	&dev_attr_boost_level.attr,
#endif
	NULL,
};

static struct attribute_group touchkey_attr_group = {
	.attrs = touchkey_attributes,
};

#ifdef TKEY_REQUEST_FW_UPDATE
static int load_fw_built_in(struct cypress_touchkey_info *info)
{
	struct i2c_client *client = info->client;
	int ret;
	char *fw_name;

	if (info->touchkeyid == CYPRESS_TOUCHKEY)
		fw_name = kasprintf(GFP_KERNEL, "%s/%s.fw",
			TKEY_FW_BUILTIN_PATH, TKEY_CYPRESS_FW_NAME);
	else
		fw_name = kasprintf(GFP_KERNEL, "%s/%s.fw",
			TKEY_FW_BUILTIN_PATH, TKEY_CORERIVER_FW_NAME);

	pr_info("%s!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",fw_name);
	ret = request_firmware(&info->fw, fw_name, &client->dev);
	if (ret) {
		dev_err(&client->dev, "error requesting built-in firmware (%d)"
			"\n", ret);
		goto out;
	}

	info->fw_img = (struct fw_image *)info->fw->data;
	info->src_fw_ver = info->fw_img->first_fw_ver;
	dev_info(&client->dev, "the fw 0x%x is loaded (size=%d)\n",
		 info->fw_img->first_fw_ver, info->fw_img->fw_len);

out:
	kfree(fw_name);
	return ret;
}

static int load_fw_in_sdcard(struct cypress_touchkey_info *info)
{
	struct i2c_client *client = info->client;
	int ret;
	mm_segment_t old_fs;
	struct file *fp;
	long nread;
	int len;
	char *fw_name;

	if (info->touchkeyid == CYPRESS_TOUCHKEY)
		fw_name = kasprintf(GFP_KERNEL, "%s/%s.bin",
			TKEY_FW_IN_SDCARD_PATH, TKEY_CYPRESS_FW_NAME);
	else
		fw_name = kasprintf(GFP_KERNEL, "%s/%s.bin",
			TKEY_FW_IN_SDCARD_PATH, TKEY_CORERIVER_FW_NAME);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(fw_name, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		dev_err(&client->dev, "%s: fail to open fw in %s\n",
			__func__, fw_name);
		ret = -ENOENT;
		goto err_open_fw;
	}
	len = fp->f_path.dentry->d_inode->i_size;

	info->fw_img = kzalloc(len, GFP_KERNEL);
	if (!info->fw_img) {
		dev_err(&client->dev, "%s: fail to alloc mem for fw\n",
			__func__);
		ret = -ENOMEM;
		goto err_alloc;
	}
	nread = vfs_read(fp, (char __user *)info->fw_img, len, &fp->f_pos);

	dev_info(&client->dev, "%s: load fw in internal sd (%ld)\n",
		 __func__, nread);

	ret = 0;

err_alloc:
	filp_close(fp, NULL);
err_open_fw:
	set_fs(old_fs);
	kfree(fw_name);
	return ret;
}


static int tkey_load_fw(struct cypress_touchkey_info *info, u8 fw_path)
{
	struct i2c_client *client = info->client;
	int ret;

	switch (fw_path) {
	case FW_BUILT_IN:
		ret = load_fw_built_in(info);
		break;

	case FW_IN_SDCARD:
		ret = load_fw_in_sdcard(info);
		break;

	default:
		dev_err(&client->dev, "%s: invalid fw path (%d)\n",
			__func__, fw_path);
		return -ENOENT;
	}
	if (ret < 0) {
		dev_err(&client->dev, "fail to load fw in %d (%d)\n",
			fw_path, ret);
		return ret;
	}
	return 0;
}

static int tkey_unload_fw(struct cypress_touchkey_info *info, u8 fw_path)
{
	struct i2c_client *client = info->client;

	switch (fw_path) {
	case FW_BUILT_IN:
		release_firmware(info->fw);
		break;

	case FW_IN_SDCARD:
		kfree(info->fw_img);
		break;

	default:
		dev_err(&client->dev, "%s: invalid fw path (%d)\n",
			__func__, fw_path);
		return -ENOENT;
	}

	return 0;
}

static int tkey_flash_fw(struct cypress_touchkey_info *info, u8 fw_path, bool force)
{
	struct i2c_client *client = info->client;
	int ret;

	/* firmware load */
	ret = tkey_load_fw(info, fw_path);
	if (ret < 0) {
		dev_err(&client->dev, "%s : fail to load fw (%d)\n", __func__, ret);
		return ret;
	}
	info->cur_fw_path = fw_path;

	if (force == true || info->touchkeyid == CORERIVER_TOUCHKEY ||
		(info->touchkeyid == CYPRESS_TOUCHKEY && (info->module_ver ==0x8 ||info->module_ver == 0x9)))
		info->support_fw_update = true;
	else
		info->support_fw_update = false;


#ifdef ENABLE_FW_UPDATE
	if(info->pdata->fw_update_flag)
		info->support_fw_update = true;
	else
		info->support_fw_update = false;
#endif

	/* firmware version compare */
#if defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_JS01LTESBM)
	if (info->ic_fw_ver >= info->src_fw_ver && !force) {
#else
	if ((info->ic_fw_ver >= info->src_fw_ver && !force) || info->support_fw_update == false) {
#endif
		dev_info(&client->dev, "%s : IC aleady have latest firmware (fw:%#x module:%#x)\n",
			 __func__ , info->ic_fw_ver, info->module_ver);
		goto out;
	}

	/* firmware update */
	disable_irq(client->irq);

	if (info->touchkeyid == CYPRESS_TOUCHKEY)
		ret = tkey_fw_update(info, force);
	else
		ret = coreriver_fw_update(info, force);
	if (ret < 0) {
		goto err_fw_update;
	}
	enable_irq(client->irq);

err_fw_update:
out:
	tkey_unload_fw(info, fw_path);
	return ret;
}
#endif

#ifndef USE_SW_I2C
static void cypress_config_gpio_i2c(struct cypress_touchkey_platform_data *pdata, int onoff)
{
	if (onoff) {
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_scl, 3, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_sda, 3, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	} else {
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_scl, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_sda, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	}
}
#endif

static void cypress_request_gpio(struct cypress_touchkey_platform_data *pdata)
{
	int ret;

	ret = gpio_request(pdata->gpio_int, "touchkey_irq");
	if (ret) {
		printk(KERN_ERR "%s: unable to request touchkey_irq [%d]\n",
				__func__, pdata->gpio_int);
	}

	if (pdata->gpio_touchkey_id > 0) {
		ret = gpio_request(pdata->gpio_touchkey_id, "touchkey_id");
		if (ret) {
			printk(KERN_ERR "%s: unable to request touchkey_id[%d]\n",
					__func__, pdata->gpio_touchkey_id);
		}
	}

	if (pdata->vdd_led > 0) {
		ret = gpio_request(pdata->vdd_led, "touchkey_vdd_led");
		if (ret) {
			printk(KERN_ERR "%s: unable to request touchkey_vdd_led [%d]\n",
					__func__, pdata->vdd_led);
		}
	}
}

#ifdef CONFIG_OF
static int cypress_get_keycodes(struct device *dev, char *name,
				struct cypress_touchkey_platform_data *pdata)
{
	struct property *prop;
	struct device_node *np = dev->of_node;
	int rc;

	prop = of_find_property(np, name, NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;

	pdata->keycodes_size = prop->length / sizeof(u32);
	rc = of_property_read_u32_array(np, name, pdata->touchkey_keycode,
				pdata->keycodes_size);
	if (rc && (rc != -EINVAL)) {
		dev_info(dev, "%s: Unable to read %s\n", __func__, name);
		return rc;
	}

	return 0;
}

static int cypress_parse_dt(struct device *dev,
			struct cypress_touchkey_platform_data *pdata)
{
	int rc;
	struct device_node *np = dev->of_node;

	rc = cypress_get_keycodes(dev, "cypress,tkey-keycodes", pdata);
	if (rc)
		return rc;

	/* regulator info */
	pdata->i2c_pull_up = of_property_read_bool(np, "cypress,i2c-pull-up");
	pdata->fw_update_flag = of_property_read_bool(np, "cypress,fw_update_flag");
	pdata->vdd_led = of_get_named_gpio(np, "vdd_led-gpio", 0);
	pdata->vcc_en = of_get_named_gpio(np, "vcc_en-gpio", 0);

	/* reset, irq gpio info */
	pdata->gpio_scl = of_get_named_gpio_flags(np, "cypress,scl-gpio",
				0, &pdata->scl_gpio_flags);
	pdata->gpio_sda = of_get_named_gpio_flags(np, "cypress,sda-gpio",
				0, &pdata->sda_gpio_flags);
	pdata->gpio_int = of_get_named_gpio_flags(np, "cypress,irq-gpio",
				0, &pdata->irq_gpio_flags);
	pdata->gpio_touchkey_id = of_get_named_gpio_flags(np, "cypress,touchkey_id-gpio",
				0, &pdata->gpio_touchkey_id_flags);

	pr_err("%s: SCL:%d, SDA:%d, INT:%d, ID:%d, VDD_GPIO:%d ,VCC_EN:%d, FW_UPDATE:%d\n",
			__func__, pdata->gpio_scl, pdata->gpio_sda, pdata->gpio_int,
			pdata->gpio_touchkey_id, pdata->vdd_led, pdata->vcc_en, pdata->fw_update_flag);
	return 0;
}
#else
static int cypress_parse_dt(struct device *dev,
			struct cypress_touchkey_platform_data *pdata)
{
	return -ENODEV;
}
#endif
static void tkey_check_ic(struct cypress_touchkey_info *info)
{
	int ret;

	if (info->pdata->gpio_touchkey_id < 0) {
		info->touchkeyid = CYPRESS_TOUCHKEY;
	} else {
		ret = gpio_get_value(info->pdata->gpio_touchkey_id);
		if (ret == 0)
			info->touchkeyid = CORERIVER_TOUCHKEY;
		else
			info->touchkeyid =CYPRESS_TOUCHKEY;
	}

//TEST
	info->touchkeyid =CYPRESS_TOUCHKEY;
//TEST
	dev_info(&info->client->dev, "touchkey id = %s\n",
			info->touchkeyid ? "CYPRESS" : "CORERIVER");
}

static int __devinit cypress_touchkey_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct cypress_touchkey_platform_data *pdata;
	struct cypress_touchkey_info *info;
	struct input_dev *input_dev;
	struct device *sec_touchkey;

	int ret = 0;
	int i;
	int error;
	bool bforced = false;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct cypress_touchkey_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_info(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		error = cypress_parse_dt(&client->dev, pdata);
		if (error)
			return error;
	} else
		pdata = client->dev.platform_data;

	cypress_request_gpio(pdata);

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_info(&client->dev, "%s: fail to memory allocation.\n", __func__);
		goto err_mem_alloc;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		printk(KERN_ERR "%s: fail to allocate input device.\n", __func__);
		goto err_input_dev_alloc;
	}
	client->irq = gpio_to_irq(pdata->gpio_int);

	info->client = client;
	info->input_dev = input_dev;
	info->pdata = pdata;
	info->irq = client->irq;
	info->touchkey_update_status = UPDATE_NONE;

	input_dev->name = "sec_touchkey";
	input_dev->phys = info->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);

	atomic_set(&info->keypad_enable, 1);

	for (i = 0; i < pdata->keycodes_size; i++) {
		info->keycode[i] = pdata->touchkey_keycode[i];
		set_bit(info->keycode[i], input_dev->keybit);
	}

	i2c_set_clientdata(client, info);

	info->is_powering_on = true;
	if (info->pdata->gpio_led_en)
		cypress_touchkey_con_hw(info, true);
	cypress_power_onoff(info, 1);

	msleep(150);

#ifdef CRC_CHECK_DELAY
	msleep(70);
#endif
	tkey_check_ic(info);

	if (info->touchkeyid == CORERIVER_TOUCHKEY)
		msleep(50);

	info->enabled = true;
	ret = tkey_i2c_check(info);
	if (ret < 0) {
		dev_err(&client->dev, "i2c_check failed\n");
		#if defined(CONFIG_SEC_FACTORY)
			info->enabled = false;
			goto err_i2c_check;
		#else
			bforced = true;
		#endif
	}

	input_set_drvdata(input_dev, info);

	ret = input_register_device(input_dev);
	if (ret) {
		dev_info(&client->dev, "failed to register input dev (%d).\n",
			ret);
		goto err_reg_input_dev;
	}

#ifdef USE_OPEN_CLOSE
	input_dev->open = cypress_input_open;
	input_dev->close = cypress_input_close;
#endif
	dev_info(&info->client->dev, "gpio_to_irq IRQ %d\n",
			client->irq);

#ifdef TSP_BOOSTER
	cypress_init_dvfs(info);
#endif

	ret = request_threaded_irq(client->irq, NULL,
			cypress_touchkey_interrupt,
			IRQF_TRIGGER_FALLING, client->dev.driver->name, info);
	if (ret < 0) {
		dev_info(&client->dev, "Failed to request IRQ %d (err: %d).\n",
				client->irq, ret);
		goto err_req_irq;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
		info->early_suspend.level =
				EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
		info->early_suspend.suspend = cypress_touchkey_early_suspend;
		info->early_suspend.resume = cypress_touchkey_late_resume;
		register_early_suspend(&info->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */

#if defined(CONFIG_GLOVE_TOUCH)
	info->glove_wq = create_singlethread_workqueue("cypress_touchkey");
	if (!info->glove_wq)
		dev_info(&client->dev, "fail to create glove workquewe.\n");
	else
		INIT_WORK(&info->glove_work, cypress_touchkey_glove_work);
#endif

#ifdef TKEY_REQUEST_FW_UPDATE
	ret = tkey_flash_fw(info, FW_BUILT_IN, bforced);
  	if (ret < 0) {
		dev_info(&info->client->dev,
			"%s: tkey fw update failed.\n", __func__);
		goto err_fw_update;
	}
#else
	info->src_fw_ver = BIN_FW_VERSION;
#endif

	if (info->touchkeyid == CYPRESS_TOUCHKEY)
		cypress_touchkey_auto_cal(info, true);

	sec_touchkey = device_create(sec_class, NULL, 0, NULL, "sec_touchkey");
	if (IS_ERR(sec_touchkey)) {
		dev_info(&info->client->dev,
				"%s: Failed to create device(sec_touchkey)!\n", __func__);
		goto err_sysfs;
	}
	dev_set_drvdata(sec_touchkey, info);

	ret = sysfs_create_group(&sec_touchkey->kobj,
	    &touchkey_attr_group);
	if (ret < 0) {
		dev_err(&info->client->dev,
				"%s: Failed to create sysfs attributes\n",
				__func__);
		goto err_sysfs_group;
	}

#if defined(TK_INFORM_CHARGER)
	info->pdata->register_cb = touchkey_register_callback;

	info->callbacks.inform_charger = cypress_touchkey_ta_cb;
	if (info->pdata->register_cb) {
		dev_info(&client->dev, "[Touchkey] Register TA Callback\n");
		info->pdata->register_cb(&info->callbacks);
	}
#endif

	info->is_powering_on = false;

/*
   cypress_power_onoff(info, 0);
*/
	dev_info(&info->client->dev, "%s: done\n", __func__);
	return 0;

err_sysfs_group:
	device_destroy(sec_class,  0);
err_sysfs:
#ifdef TKEY_REQUEST_FW_UPDATE
err_fw_update:
#endif
	if (info->irq >= 0)
	free_irq(info->irq, info);	
#if defined(CONFIG_GLOVE_TOUCH)
	destroy_workqueue(info->glove_wq);
#endif
err_req_irq:
/*
err_gpio_request:
*/
	input_unregister_device(input_dev);
err_reg_input_dev:
	input_free_device(input_dev);
	input_dev = NULL;
#if defined(CONFIG_SEC_FACTORY)
err_i2c_check:
#endif
	if (info->pdata->gpio_led_en)
		cypress_touchkey_con_hw(info, false);	
	cypress_power_onoff(info, 0);
err_input_dev_alloc:
	kfree(info);
	return -ENXIO;
err_mem_alloc:
	return -ENOMEM;
}

static int __devexit cypress_touchkey_remove(struct i2c_client *client)
{
	struct cypress_touchkey_info *info = i2c_get_clientdata(client);
	if (info->irq >= 0)
		free_irq(info->irq, info);
	if (info->pdata->vdd_led > 0)
		gpio_free(info->pdata->vdd_led);
	input_unregister_device(info->input_dev);
	input_free_device(info->input_dev);
	kfree(info);
	return 0;
}

#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
static int cypress_touchkey_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cypress_touchkey_info *info = i2c_get_clientdata(client);
	int ret = 0;

#ifdef TSP_BOOSTER
	cypress_set_dvfs_lock(info, 2);
	dev_info(&info->client->dev,
			"%s: dvfs_lock free.\n", __func__);
#endif
	info->is_powering_on = true;
	disable_irq(info->irq);
	if (info->pdata->gpio_led_en)
		cypress_touchkey_con_hw(info, false);
	cypress_power_onoff(info, 0);	
	info->enabled = false;
	return ret;
}

static int cypress_touchkey_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cypress_touchkey_info *info = i2c_get_clientdata(client);
	int ret = 0;
	cypress_power_onoff(info, 1);
	if (info->pdata->gpio_led_en)
		cypress_touchkey_con_hw(info, true);
	msleep(150);

	if (info->touchkeyid == CYPRESS_TOUCHKEY) {
#ifdef CRC_CHECK_DELAY
	msleep(70);
#endif
#ifdef CYPRESS_SUPPORT_DUAL_INT_MODE
	/* CYPRESS Firmware setting interrupt type : dual or single interrupt */
		cypress_touchkey_interrupt_set_dual(info->client);
#endif
		info->enabled = true;
		cypress_touchkey_auto_cal(info, false);
	} else {
		msleep(50);
		info->enabled = true;
	}

	if (touchled_cmd_reversed) {
		touchled_cmd_reversed = 0;
		i2c_smbus_write_byte_data(info->client,
				CYPRESS_GEN, touchkey_led_status);
		dev_info(&client->dev,
				"%s: LED returned on\n", __func__);
		msleep(30);
	}

	enable_irq(info->irq);

	info->is_powering_on = false;
	return ret;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cypress_touchkey_early_suspend(struct early_suspend *h)
{
	struct cypress_touchkey_info *info;
	info = container_of(h, struct cypress_touchkey_info, early_suspend);
	cypress_touchkey_suspend(&info->client->dev);
}

static void cypress_touchkey_late_resume(struct early_suspend *h)
{
	struct cypress_touchkey_info *info;
	info = container_of(h, struct cypress_touchkey_info, early_suspend);
	cypress_touchkey_resume(&info->client->dev);
}
#endif

static const struct i2c_device_id cypress_touchkey_id[] = {
	{"cypress_touchkey", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, cypress_touchkey_id);
#ifdef CONFIG_OF
static struct of_device_id cypress_match_table[] = {
	{ .compatible = "cypress,cypress-tkey",},
	{ },
};
#else
#define cypress_match_table	NULL
#endif

#ifdef USE_OPEN_CLOSE
static int cypress_input_open(struct input_dev *dev)
{
	struct cypress_touchkey_info *info = input_get_drvdata(dev);

	dev_info(&info->client->dev, "%s.\n", __func__);
	cypress_touchkey_resume(&info->client->dev);

	return 0;
}

static void cypress_input_close(struct input_dev *dev)
{
	struct cypress_touchkey_info *info = input_get_drvdata(dev);

	dev_info(&info->client->dev, "%s.\n", __func__);
	cypress_touchkey_suspend(&info->client->dev);

}
#endif

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE)
static const struct dev_pm_ops cypress_touchkey_pm_ops = {
	.suspend	= cypress_touchkey_suspend,
	.resume		= cypress_touchkey_resume,
};
#endif

struct i2c_driver cypress_touchkey_driver = {
	.probe = cypress_touchkey_probe,
	.remove = cypress_touchkey_remove,
	.driver = {
		.name = "cypress_touchkey",
		.owner = THIS_MODULE,
		.of_match_table = cypress_match_table,
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE)
		.pm	= &cypress_touchkey_pm_ops,
#endif
		   },
	.id_table = cypress_touchkey_id,
};
/*
module_i2c_driver(cypress_touchkey_driver);
*/
static int __init cypress_touchkey_init(void)
{

	int ret = 0;

#ifdef CONFIG_SAMSUNG_LPM_MODE
	if (poweroff_charging) {
		pr_notice("%s : LPM Charging Mode!!\n", __func__);
		return 0;
	}
#endif

	printk(KERN_ERR "%s\n", __func__);

	ret = i2c_add_driver(&cypress_touchkey_driver);
	if (ret) {
		printk(KERN_ERR "cypress touch keypad registration failed. ret= %d\n",
			ret);
	}
	printk(KERN_ERR "%s: init done %d\n", __func__, ret);

	return ret;
}

static void __exit cypress_touchkey_exit(void)
{
	i2c_del_driver(&cypress_touchkey_driver);
}

module_init(cypress_touchkey_init);
module_exit(cypress_touchkey_exit);

MODULE_DESCRIPTION("Touchkey driver for Cypress touchkey controller ");
MODULE_LICENSE("GPL");
