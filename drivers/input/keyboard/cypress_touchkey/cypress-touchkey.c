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
#include <linux/mfd/pm8xxx/pm8921.h>

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
		return -1;
	}

	while (retry--) {
		err = i2c_smbus_read_i2c_block_data(client, reg, len, val);
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
		return -1;
	}

	while (retry--) {
		err = i2c_smbus_write_i2c_block_data(client,
				CYPRESS_GEN, len, val);
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

	if (info->device_ver != 0x10) { /* support CYPRESS 20075 */
		dev_err(&client->dev, "%s: not support this module\n", __func__);
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
	u8 data[4] = { 0, };

	if (system_rev >= 0x03) /*CY8C20075*/
		ret = cypress_touchkey_i2c_read(info->client, CYPRESS_BUTTON_STATUS, data, 4);
	else
		ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 4);

	if (ret < 0) {
		dev_err(&client->dev, "Failed to read Module version\n");
		info->ic_fw_ver = 0;
		info->module_ver = 0;
		info->device_ver = 0;
		return ret;
	}

	info->ic_fw_ver = data[1];
	info->module_ver = data[2];
	info->device_ver = data[3];
	dev_info(&client->dev, "%s: ic_fw_ver = 0x%02x, module_ver = 0x%02x, CY device = 0x%02x\n",
		__func__, info->ic_fw_ver, info->module_ver, info->device_ver);

#ifdef CYPRESS_SUPPORT_DUAL_INT_MODE
	/* CYPRESS Firmware setting interrupt type : dual or single interrupt */
	cypress_touchkey_interrupt_set_dual(info->client);
#endif
	return ret;
}

void cypress_power_onoff(struct cypress_touchkey_info *info, int onoff)
{
	int ret = 0, rc = 0;
	static struct regulator *max77826_ldo15;

	dev_info(&info->client->dev, "%s: power %s\n",
			__func__, onoff ? "on" : "off");

	if (info->pdata->vcc_flag) {
		if (!info->vcc_en) {
			info->vcc_en = regulator_get(&info->client->dev,
				"vcc_en");
			if (IS_ERR(info->vcc_en)) {
				dev_err(&info->client->dev,
					"Regulator(vcc_en) get failed rc = %ld\n", PTR_ERR(info->vcc_en));
				return;
			}
		}
	} else {
		if (!max77826_ldo15) {
			max77826_ldo15 = regulator_get(NULL, "max77826_ldo15");
			if (IS_ERR(max77826_ldo15)) {
				dev_err(&info->client->dev,
					"Regulator(max77826_ldo15) get failed rc = %ld\n", PTR_ERR(max77826_ldo15));
				max77826_ldo15 = NULL;
				return;
			}

			rc = regulator_set_voltage(max77826_ldo15, 1800000, 1800000);
			if (rc) {
				dev_err(&info->client->dev,
					"regulator(max77826_ldo15) set_vtg failed rc=%d\n", rc);
				return;
			}
		}
	}

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

	if (onoff) {
		if (info->pdata->vcc_flag) {
			if (!regulator_is_enabled(info->vcc_en)) {
				rc = regulator_enable(info->vcc_en);
				if (rc) {
					dev_err(&info->client->dev,
						"Regulator vcc_en enable failed rc=%d\n", rc);
					return;
				}
			}
		} else {
			if (!regulator_is_enabled(max77826_ldo15)) {
				rc = regulator_enable(max77826_ldo15);
				if (rc) {
					dev_err(&info->client->dev,
						"Regulator max77826_ldo15 enable failed rc=%d\n",
						rc);
					return;
				}
			}
		}

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
	} else {
		if (info->pdata->vcc_flag) {
			if (regulator_is_enabled(info->vcc_en)) {
				rc = regulator_disable(info->vcc_en);
				if (rc) {
					dev_err(&info->client->dev,
						"Regulator vcc_en disable failed rc=%d\n", rc);
					return;
				}
			}
		} else {
			if (regulator_is_enabled(max77826_ldo15)) {
				rc = regulator_disable(max77826_ldo15);
				if (rc) {
					dev_err(&info->client->dev,
						"Regulator max77826_ldo15 disable failed rc=%d\n",
						rc);
					return;
				}
			}
		}

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

	if (info->pdata->vdd_led > 0) {
		ret = gpio_direction_output(info->pdata->vdd_led, onoff);
		if (ret) {
			dev_err(&info->client->dev,
					"%s: unable to set_direction for vdd_led [%d]\n",
					__func__, info->pdata->vdd_led);
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
/*
	ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 2);
		if (ret < 0) {
		dev_err(&info->client->dev, "Failed to read 0x00.\n");
			return ret;
		}

	//Send autocal Command
	if (info->charging_mode)
		data[1] |= TK_BIT_CMD_TA_ON;
	else
		data[1] &= ~TK_BIT_CMD_TA_ON;
*/

		/* Send autocal Command */
	if (info->charging_mode) {
		data[1] = TK_BIT_CMD_TA_ON;
		data[2] = TK_BIT_WRITE_CONFIRM;
	} else {
		data[1] = TK_BIT_CMD_REGULAR;
		data[2] = TK_BIT_WRITE_CONFIRM;
	}

	count = cypress_touchkey_i2c_write(info->client, data, 3);

	while (retry < 3) {
		msleep(30);

		/* Check status flag mode */
		ret = cypress_touchkey_i2c_read(info->client, CYPRESS_STATUS_FLAG, data, 1);
		if (ret < 0) {
			dev_info(&info->client->dev, "%s: Failed to check status flag.\n",
					__func__);
			return ret;
		}
		dev_err(&info->client->dev,
			"data[0]=%x, 1mm: %x, flip: %x, glove: %x, ta: %x\n\n",
			data[0], data[0] & 0x20, data[0] & 0x10, data[0] & 0x08, data[0] & 0x04);

		if (info->charging_mode) {
			if (data[0] & TK_BIT_TA_ON) {
				dev_dbg(&info->client->dev, "%s: TA mode is Enabled\n", __func__);
				break;
			} else {
				dev_err(&info->client->dev, "%s: TA Enabled Error! retry=%d\n",
					__func__, retry);
			}
		} else {
			if (!(data[0] & TK_BIT_TA_ON)) {
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

	if (info->device_ver != 0x10) {/*CY8C20075*/
		dev_info(&info->client->dev, "%s not suppoered Device.\n",
				__func__);
		return;
	}

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
		container_of(work, struct cypress_touchkey_info, glove_work.work);

	u8 data[6] = { 0, };
	int ret = 0;
	u8 retry = 0;
	u8 glove_bit;

/*
		ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 4);
		if (ret < 0) {
		dev_err(&info->client->dev, "%s: Failed to read 0x00.\n",
				__func__);
			return;
		}

	//Send glove Command
	if (info->glove_value == 1)
		data[1] = TK_BIT_CMD_GLOVE;
	else
		data[1] &= ~TK_BIT_CMD_GLOVE;
	data[2] = TK_BIT_WRITE_CONFIRM;
*/
	if (info->glove_value == 1) {
		/* Send glove Command */
		data[1] = TK_BIT_CMD_GLOVE;
		data[2] = TK_BIT_WRITE_CONFIRM;
	} else {
		data[1] = TK_BIT_CMD_REGULAR;
		data[2] = TK_BIT_WRITE_CONFIRM;
	}

	cypress_touchkey_i2c_write(info->client, data, 4);
	pr_err("[HASH] data[1] =  [[[  %x  ]]]\n",data[1]);

	while (retry < 3) {
		msleep(30);

		/* Check status flag mode */
		ret = cypress_touchkey_i2c_read(info->client, CYPRESS_STATUS_FLAG, data, 1);
		if (ret < 0) {
			dev_info(&info->client->dev, "%s: Failed to check status flag.\n",
					__func__);
			return;
		}

		glove_bit = !!(data[0] & TK_BIT_GLOVE);
		dev_err(&info->client->dev,
			"data[0]=%x, 1mm: %x, flip: %x, glove: %x, ta: %x\n\n",
			data[0], data[0] & 0x20, data[0] & 0x10, data[0] & 0x08, data[0] & 0x04);

		if (info->glove_value == glove_bit) {
			dev_info(&info->client->dev, "%s: Glove mode is %s\n",
					__func__, info->glove_value ? "enabled" : "disabled");
				break;
			} else {
			dev_err(&info->client->dev,
					"%s: Error to set glove_mode val %d, bit %d, retry %d\n",
					__func__, info->glove_value, glove_bit, retry);
		}

		retry = retry + 1;
	}

	if (retry == 3)
		dev_err(&info->client->dev, "%s: Failed to set the glove mode\n", __func__);

	return;
}

int cypress_touchkey_glovemode(struct cypress_touchkey_info *info, int value)
{
	if (info->device_ver != 0x10) {/*CY8C20075*/
		dev_info(&info->client->dev, "%s not suppoered Device.\n",
				__func__);
		return 0;
	}

	if (wake_lock_active(&info->fw_wakelock)) {
		dev_info(&info->client->dev, "wackelock active\n");
		return 0;
	}

	if (info->glove_value == value) {
		dev_info(&info->client->dev, "glove mode is already %s\n", value ? "enabled" : "disabled");
		return 0;
	}

	mutex_lock(&info->tkey_glove_lock);
	cancel_delayed_work(&info->glove_work);

	info->glove_value = value;

	schedule_delayed_work(&info->glove_work,
		msecs_to_jiffies(TKEY_GLOVE_DWORK_TIME));
	mutex_unlock(&info->tkey_glove_lock);
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

	if (info->device_ver != 0x10) {/*CY8C20075*/
		dev_info(&info->client->dev, "%s : not supported Device.\n",
				__func__);
		return ;
	}

	if (wake_lock_active(&info->fw_wakelock)) {
		dev_info(&info->client->dev, "wackelock active\n");
		return ;
	}

/*
	ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 4);
	if (ret <= 0) {
		dev_err(&info->client->dev, "%s: Failed to read Keycode_reg.\n",
			__func__);
		return;
	}

	if (value == 1) {
		data[1] |= TK_BIT_CMD_FLIP;
		data[1] &= ~TK_BIT_CMD_REGULAR;
	} else {
		data[1] &= ~TK_BIT_CMD_FLIP;
		data[1] |= TK_BIT_CMD_REGULAR;
	}
	data[2] = TK_BIT_WRITE_CONFIRM;
*/

	if (value == 1) {
		/* Send filp mode Command */
		data[1] = TK_BIT_CMD_FLIP;
		data[2] = TK_BIT_WRITE_CONFIRM;
	} else {
		data[1] = TK_BIT_CMD_REGULAR;
		data[2] = TK_BIT_WRITE_CONFIRM;
	}

	ret = cypress_touchkey_i2c_write(info->client, data, 4);
	if (ret < 0) {
		dev_info(&info->client->dev, "%s: Failed to write flip mode command.\n",
				__func__);
		return;
	}

	while (retry < 3) {
		msleep(30);
		/* Check status flag mode */
		ret = cypress_touchkey_i2c_read(info->client, CYPRESS_STATUS_FLAG, data, 1);
		if (ret < 0) {
			dev_info(&info->client->dev, "%s: Failed to check status flag.\n",
					__func__);
			return;
		}

		dev_err(&info->client->dev,
			"data[0]=%x, 1mm: %x, flip: %x, glove: %x, ta: %x\n\n",
			data[0], data[0] & 0x20, data[0] & 0x10, data[0] & 0x08, data[0] & 0x04);

		if (value == 1) {
			if (data[0] & TK_BIT_FLIP) {
				dev_err(&info->client->dev, "%s: Flip mode is enabled\n", __func__);
				info->enabled_flip = true;
				break;
			} else {
				dev_err(&info->client->dev, "%s: flip_mode Enable failed. retry=%d\n",
					__func__, retry);
			}
		} else {
			if (!(data[0] & TK_BIT_FLIP)) {
				dev_err(&info->client->dev, "%s: Normal mode from Flip mode\n", __func__);
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

#ifdef TKEY_1MM_MODE
void cypress_touchkey_1mm_mode(struct cypress_touchkey_info *info, int value)
{
	u8 data[6] = { 0, };
	int ret = 0;
	u8 retry = 0;
	u8 stylus_status;

	if (!(info->enabled)) {
		dev_info(&info->client->dev, "%s : Touchkey is not enabled.\n",
				__func__);
		return ;
	}

	if (info->device_ver != 0x10) {/*CY8C20075*/
		dev_info(&info->client->dev, "%s : not supported Device.\n",
				__func__);
		return ;
	}

	if (wake_lock_active(&info->fw_wakelock)) {
		dev_info(&info->client->dev, "wackelock active\n");
		return ;
	}

/*
	ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 4);
	if (ret <= 0) {
		dev_info(&info->client->dev, "%s: Failed to read Keycode_reg.\n",
			__func__);
		return;
	}
	// Send 1mm mode Command
	if (value == 1) {
		data[1] |= TK_BIT_CMD_1MM;
		data[1] &= ~TK_BIT_CMD_REGULAR;
	} else {
		data[1] &= ~TK_BIT_CMD_1MM;
		data[1] |= TK_BIT_CMD_REGULAR;
	}
	data[2] = TK_BIT_WRITE_CONFIRM;
*/
	if (value == 1) {
	/* Send 1mm mode Command */
		data[1] = TK_BIT_CMD_1MM;
		data[2] = TK_BIT_WRITE_CONFIRM;
	} else {
		data[1] = TK_BIT_CMD_REGULAR;
		data[2] = TK_BIT_WRITE_CONFIRM;
	}

	ret = cypress_touchkey_i2c_write(info->client, data, 4);
	if (ret < 0) {
		dev_info(&info->client->dev, "%s: Failed to write 1mm mode command.\n",
				__func__);
		return;
	}

	while (retry < 3) {
		msleep(30);
		/* Check status flag mode */
		ret = cypress_touchkey_i2c_read(info->client, CYPRESS_STATUS_FLAG, data, 1);
		if (ret < 0) {
			dev_info(&info->client->dev, "%s: Failed to check status flag.\n",
					__func__);
			return;
		}
		stylus_status = !!(data[0] & TK_BIT_1MM);

		dev_err(&info->client->dev,
			"data[0]=%x, 1mm: %x, flip: %x, glove: %x, ta: %x\n\n",
			data[0], data[0] & 0x20, data[0] & 0x10, data[0] & 0x08, data[0] & 0x04);

		if (value == stylus_status) {
			dev_info(&info->client->dev,
				"%s: 1MM mode is %s\n", __func__, stylus_status ? "enabled" : "disabled");
				break;
			} else {
			dev_err(&info->client->dev,
				"%s: Error to set 1MM mode, val %d, 1mm bit %d, retry %d\n",
				__func__, value, stylus_status, retry);
		}
		retry = retry + 1;
	}

	if (retry == 3)
		dev_err(&info->client->dev, "[Touchkey] 1mm mode failed\n");

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

	if (info->device_ver == 0x10)	/*CY8C20075*/
		buf[0] = i2c_smbus_read_byte_data(info->client, CYPRESS_BUTTON_STATUS);
	else
		buf[0] = i2c_smbus_read_byte_data(info->client, CYPRESS_GEN);

	if (buf[0] < 0) {
		dev_info(&info->client->dev, "%s: interrupt failed with %d.\n",
				__func__, ret);
		goto out;
	}

	/* L OS support Screen Pinning feature.
	 * "recent + back key" event is CombinationKey for exit Screen Pinning mode.
	 * If touchkey firmware version is higher than CYPRESS_RECENT_BACK_REPORT_FW_VER,
	 * touchkey can make interrupt "back key" and "recent key" in same time.
	 * lower version firmware can make interrupt only one key event.
	 */
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

		if (data[5] & 0x80) {	/* TK_BIT_AUTOCAL */
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

#ifdef TKEY_REQUEST_FW_UPDATE
#ifdef USE_TKEY_UPDATE_WORK
static void tkey_fw_update_work(struct work_struct *work)
{
	struct cypress_touchkey_info *info =
		container_of(work, struct cypress_touchkey_info, update_work);

	int retry;
	int ret = 0;

	dev_info(&info->client->dev, "%s : touchkey_update Start!!\n", __func__);
	retry = NUM_OF_RETRY_UPDATE;

	disable_irq(info->client->irq);
	wake_lock(&info->fw_wakelock);

	while (retry--) {
		if (ISSP_main(info) == 0) {
			msleep(50);
			if (info->device_ver != 0x10) /*not CY8C20075*/
				cypress_touchkey_auto_cal(info, false);
			ret = tkey_i2c_check(info);
			if (ret < 0) {
				dev_info(&info->client->dev, "%s : touchkey_update pass but fw crashed.\n", __func__);
				continue;
			} else {
				dev_info(&info->client->dev, "%s : touchkey_update pass!!\n", __func__);
				break;
			}
		}
		msleep(50);
	}

	if (retry <= 0) {
		dev_err(&info->client->dev, "%s : touchkey_update fail\n", __func__);
		goto end_fw_update;
	}

	msleep(500);
	tkey_i2c_check(info);

 end_fw_update:
	enable_irq(info->client->irq);
 	wake_unlock(&info->fw_wakelock);


}
#endif

static int tkey_fw_update(struct cypress_touchkey_info *info, bool force)
{
	struct i2c_client *client = info->client;
	int retry;
	int ret = 0;

	dev_info(&client->dev, "%s : touchkey_update Start!!\n", __func__);
	disable_irq(client->irq);
	wake_lock(&info->fw_wakelock);

	if (force == true)
		retry = 2;
	else
		retry = NUM_OF_RETRY_UPDATE;

	while (retry--) {
		if (ISSP_main(info) == 0) {
			msleep(50);
			if (info->device_ver != 0x10) /*not CY8C20075*/
				cypress_touchkey_auto_cal(info, false);
			ret = tkey_i2c_check(info);
			if (ret < 0) {
				dev_info(&client->dev, "%s : touchkey_update pass but fw crashed.\n", __func__);
				continue;
			} else {
				dev_info(&client->dev, "%s : touchkey_update pass!!\n", __func__);
				break;
			}
		}
		msleep(50);
		dev_err(&client->dev,
			"%s : touchkey_update failed... retry...\n", __func__);
	}

	if (retry <= 0) {
		dev_err(&client->dev, "%s : touchkey_update fail\n", __func__);
		goto end_fw_update;
	}

	msleep(500);

	tkey_i2c_check(info);

 end_fw_update:
	wake_unlock(&info->fw_wakelock);
	enable_irq(client->irq);

	return ret;
}
#endif

static ssize_t cypress_touchkey_ic_version_read(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int count;

	info->ic_fw_ver = i2c_smbus_read_byte_data(info->client,
			CYPRESS_FW_VER);
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
	dev_info(&info->client->dev, "%s touchkey_update_status: %d\n",
			__func__, info->touchkey_update_status);
	if (info->touchkey_update_status == DOWNLOADING)
		return snprintf(buf, 13, "Downloading\n");
	else if (info->touchkey_update_status == UPDATE_FAIL)
		return snprintf(buf, 6, "FAIL\n");
	else if (info->touchkey_update_status == UPDATE_PASS)
		return snprintf(buf, 6, "PASS\n");
	else
		return snprintf(buf, 5, "N/A\n");
}

static ssize_t cypress_touchkey_update_write(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf, size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
#ifdef TKEY_REQUEST_FW_UPDATE
	struct i2c_client *client = info->client;	
	int count = 0;
	u8 fw_path;
#endif
	dev_info(&info->client->dev, "%s \n", __func__);

#ifdef TKEY_REQUEST_FW_UPDATE
	switch (*buf) {
		case 's':
		case 'S':
			fw_path = FW_BUILT_IN;
			if(info->support_fw_update == false) {
				dev_err(&client->dev, "%s: module %x does not support fw update\n.", __func__, info->module_ver);
				return size;
			}
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

#ifdef TKEY_REQUEST_FW_UPDATE
	count = tkey_fw_update(info, false);
	if (count < 0) {
		cypress_power_onoff(info, 0);
		info->touchkey_update_status = UPDATE_FAIL;
		dev_err(&client->dev, "%s: fail to flash fw (%d)\n.", __func__, count);
		return count;
	}
#endif

	info->touchkey_update_status = UPDATE_PASS;
#ifdef TKEY_REQUEST_FW_UPDATE
	tkey_unload_fw(info, fw_path);
#endif

	return size;
}

static ssize_t cypress_touchkey_led_control(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int data;
	int ret;	
	static const int ledCmd[] = {TK_CMD_LED_OFF, TK_CMD_LED_ON};

	dev_info(&info->client->dev, "called %s\n", __func__);

	if (wake_lock_active(&info->fw_wakelock)) {
		dev_err(&info->client->dev, "%s : wackelock active\n",
					__func__);
		return size;
	}
	
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

static ssize_t cypress_touchkey_sensitivity_control(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	int value, ret;
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	u8 data[4];

	sscanf(buf, "%d\n", &value);
	dev_info(&info->client->dev, "%s %d\n", __func__, value);

	if (value == 1) {
	/* Send inspection mode Command */
		data[1] = TK_BIT_CMD_INSPECTION;
		data[2] = TK_BIT_WRITE_CONFIRM;
	} else {
		data[1] = TK_BIT_CMD_REGULAR;
		data[2] = TK_BIT_WRITE_CONFIRM;
	}

	ret = cypress_touchkey_i2c_write(info->client, data, 4);
	if (ret < 0) {
		dev_info(&info->client->dev,
			"%s: Failed to write inspection mode command.\n",
			__func__);
		return 0;
	}

	ret = i2c_smbus_read_i2c_block_data(info->client,
				CYPRESS_GEN, 4, data);
	if (ret < 0) {
		dev_info(&info->client->dev,
			"[Touchkey] fail to CYPRESS_DATA_UPDATE.\n");
		return ret;
	}
	if ((data[1] & 0x20))
		dev_info(&info->client->dev,
			"[Touchkey] Control Enabled!!\n");
	else
		dev_info(&info->client->dev,
			"[Touchkey] Control Disabled!!\n");

	return size;
}

static ssize_t cypress_touchkey_menu_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 menu_sensitivity;
	u8 data[2] = { 0, };
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
			CYPRESS_DIFF_MENU, ARRAY_SIZE(data), data);
	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"[TouchKey] fail to read menu sensitivity.\n");
		return ret;
	}

	menu_sensitivity = ((0x00FF & data[0])<<8) | data[1];
	dev_info(&info->client->dev, "called %s , data : %d %d\n",
				__func__, data[0], data[1]);

	return snprintf(buf, 20, "%d\n", menu_sensitivity);
}

static ssize_t cypress_touchkey_menu_inner_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 menu_inner_sensitivity;
	u8 data[2] = { 0, };
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
			CYPRESS_DIFF_MENU_INNER, ARRAY_SIZE(data), data);
	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"[TouchKey] fail to read menu_inner sensitivity.\n");
		return ret;
	}

	menu_inner_sensitivity = ((0x00FF & data[0])<<8) | data[1];
	dev_info(&info->client->dev, "called %s , data : %d %d\n",
				__func__, data[0], data[1]);

	return snprintf(buf, 20, "%d\n", menu_inner_sensitivity);
}

static ssize_t cypress_touchkey_back_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 back_sensitivity;
	u8 data[14] = { 0, };
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
			CYPRESS_DIFF_BACK, ARRAY_SIZE(data), data);
	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"[TouchKey] fail to read back sensitivity.\n");
		return ret;
	}

	back_sensitivity = ((0x00FF & data[0])<<8) | data[1];
	dev_info(&info->client->dev, "called %s , data : %d %d\n",
				__func__, data[0], data[1]);

	return snprintf(buf, 20, "%d\n", back_sensitivity);
}

static ssize_t cypress_touchkey_back_inner_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 back_inner_sensitivity;
	u8 data[14] = { 0, };
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
			CYPRESS_DIFF_BACK_INNER, ARRAY_SIZE(data), data);
	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"[TouchKey] fail to read back_inner sensitivity.\n");
		return ret;
	}

	back_inner_sensitivity = ((0x00FF & data[0])<<8) | data[1];
	dev_info(&info->client->dev, "called %s , data : %d %d\n",
				__func__, data[0], data[1]);

	return snprintf(buf, 20, "%d\n", back_inner_sensitivity);
}

static ssize_t cypress_touchkey_raw_data0_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 raw_data0;
	u8 data[2] = {0,};
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CYPRESS_RAW_DATA_MENU, ARRAY_SIZE(data), data);

	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"[TouchKey] fail to read MENU raw data.\n");
		return ret;
	}

	raw_data0 = ((0x00FF & data[0])<<8) | data[1];
	dev_info(&info->client->dev, "called %s , data : %d %d\n",
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

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CYPRESS_RAW_DATA_MENU_INNER, ARRAY_SIZE(data), data);

	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"[TouchKey] fail to read MENU_INNER raw data.\n");
		return ret;
	}

	raw_data1 = ((0x00FF & data[0])<<8) | data[1];

	dev_info(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", raw_data1);
}

static ssize_t cypress_touchkey_raw_data2_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 raw_data2;
	u8 data[2] = {0,};
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CYPRESS_RAW_DATA_BACK, ARRAY_SIZE(data), data);

	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"[TouchKey] fail to read BACK raw data.\n");
		return ret;
	}

	raw_data2 = ((0x00FF & data[0])<<8) | data[1];
	dev_info(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", raw_data2);
}

static ssize_t cypress_touchkey_raw_data3_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 raw_data3;
	u8 data[2] = {0,};
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CYPRESS_RAW_DATA_BACK_INNER, ARRAY_SIZE(data), data);

	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"[TouchKey] fail to read BACK_INNER raw data.\n");
		return ret;
	}

	raw_data3 = ((0x00FF & data[0])<<8) | data[1];
	dev_info(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", raw_data3);
}

static ssize_t cypress_touchkey_base_data0_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 base_data0;
	u8 data[2] = {0,};
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CYPRESS_BASE_DATA_MENU, ARRAY_SIZE(data), data);

	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"fail to read MENU baseline data.\n");
		return ret;
	}

	base_data0 = ((0x00FF & data[0])<<8) | data[1];
	dev_info(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", base_data0);
}

static ssize_t cypress_touchkey_base_data1_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 base_data1;
	u8 data[2] = {0,};
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CYPRESS_BASE_DATA_MENU_INNER, ARRAY_SIZE(data), data);

	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"fail to read MENU_INNER baseline data.\n");
		return ret;
	}

	base_data1 = ((0x00FF & data[0])<<8) | data[1];

	dev_info(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", base_data1);
}

static ssize_t cypress_touchkey_base_data2_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 base_data2;
	u8 data[2] = {0,};
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CYPRESS_BASE_DATA_BACK, ARRAY_SIZE(data), data);

	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"fail to read BACK baseline data.\n");
		return ret;
	}

	base_data2 = ((0x00FF & data[0])<<8) | data[1];
	dev_info(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", base_data2);
}

static ssize_t cypress_touchkey_base_data3_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 base_data3;
	u8 data[2] = {0,};
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CYPRESS_BASE_DATA_BACK_INNER, ARRAY_SIZE(data), data);

	if (ret != ARRAY_SIZE(data)) {
		dev_info(&info->client->dev,
			"fail to read BACK_INNER baseline data.\n");
		return ret;
	}

	base_data3 = ((0x00FF & data[0])<<8) | data[1];
	dev_info(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", base_data3);
}

static ssize_t cypress_touchkey_idac0_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 idac0;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_IDAC_MENU);

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

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_IDAC_MENU_INNER);

	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	idac1 = data;
	return snprintf(buf, 20, "%d\n", idac1);
}

static ssize_t cypress_touchkey_idac2_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 idac2;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_IDAC_BACK);

	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	idac2 = data;
	return snprintf(buf, 20, "%d\n", idac2);
}

static ssize_t cypress_touchkey_idac3_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 idac3;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_IDAC_BACK_INNER);

	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	idac3 = data;
	return snprintf(buf, 20, "%d\n", idac3);
}

static ssize_t cypress_touchkey_comp_idac0_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 compidac0;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_COMPIDAC_MENU);

	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	compidac0 = data;
	return snprintf(buf, 20, "%d\n", compidac0);
}

static ssize_t cypress_touchkey_comp_idac1_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 compidac1;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_COMPIDAC_MENU_INNER);

	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	compidac1 = data;
	return snprintf(buf, 20, "%d\n", compidac1);
}

static ssize_t cypress_touchkey_comp_idac2_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 compidac2;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_COMPIDAC_BACK);

	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	compidac2 = data;
	return snprintf(buf, 20, "%d\n", compidac2);
}

static ssize_t cypress_touchkey_comp_idac3_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 compidac3;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_COMPIDAC_BACK_INNER);

	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	compidac3 = data;
	return snprintf(buf, 20, "%d\n", compidac3);
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

	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
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

static ssize_t cypress_touchkey_threshold2_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 touchkey_threshold2;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_THRESHOLD2);

	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	touchkey_threshold2 = data;
	return snprintf(buf, 20, "%d\n", touchkey_threshold2);
}

static ssize_t cypress_touchkey_threshold3_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 touchkey_threshold3;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_THRESHOLD3);

	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	touchkey_threshold3 = data;
	return snprintf(buf, 20, "%d\n", touchkey_threshold3);
}

static ssize_t cypress_touchkey_threshold4_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 touchkey_threshold4;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_THRESHOLD4);

	dev_info(&info->client->dev, "called %s , data : %d\n", __func__, data);
	touchkey_threshold4 = data;
	return snprintf(buf, 20, "%d\n", touchkey_threshold4);
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
	if ((data[5] & 0x80))	/* TK_BIT_AUTOCAL */
		return snprintf(buf, 20, "Enabled\n");
	else
		return snprintf(buf, 20, "Disabled\n");

}

#ifdef CONFIG_GLOVE_TOUCH
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

#ifdef TKEY_1MM_MODE
static ssize_t cypress_touchkey_1mm_mode_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int data;

	sscanf(buf, "%d\n", &data);
	dev_info(&info->client->dev, "%s %d\n", __func__, data);

	cypress_touchkey_1mm_mode(info, data);

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

static DEVICE_ATTR(touchkey_firm_update_status, S_IRUGO | S_IWUSR | S_IWGRP,
		cypress_touchkey_firm_status_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO,
		cypress_touchkey_ic_version_read, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO,
		cypress_touchkey_src_version_read, NULL);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, cypress_touchkey_update_write);
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, cypress_touchkey_led_control);
static DEVICE_ATTR(touch_sensitivity, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, cypress_touchkey_sensitivity_control);

static DEVICE_ATTR(touchkey_recent, S_IRUGO,	/* RECENT inner diff */
		cypress_touchkey_menu_inner_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO,	/* BACK inner diff */
		cypress_touchkey_back_inner_show, NULL);

static DEVICE_ATTR(touchkey_recent_outer, S_IRUGO,	/* RECENT outer diff */
		cypress_touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_recent_inner, S_IRUGO,	/* RECENT inner diff */
		cypress_touchkey_menu_inner_show, NULL);
static DEVICE_ATTR(touchkey_back_outer, S_IRUGO,	/* BACK outer diff */
		cypress_touchkey_back_show, NULL);
static DEVICE_ATTR(touchkey_back_inner, S_IRUGO,	/* BACK inner diff */
		cypress_touchkey_back_inner_show, NULL);

static DEVICE_ATTR(touchkey_recent_raw_outer, S_IRUGO,	/* RECENT outer raw */
		cypress_touchkey_raw_data0_show, NULL);
static DEVICE_ATTR(touchkey_recent_raw_inner, S_IRUGO,	/* RECENT inner raw */
		cypress_touchkey_raw_data1_show, NULL);
static DEVICE_ATTR(touchkey_back_raw_outer, S_IRUGO,	/* BACK outer raw */
		cypress_touchkey_raw_data2_show, NULL);
static DEVICE_ATTR(touchkey_back_raw_inner, S_IRUGO,	/* BACK inner raw */
		cypress_touchkey_raw_data3_show, NULL);

static DEVICE_ATTR(touchkey_baseline_data0, S_IRUGO,	/* RECENT outer baseline */
		cypress_touchkey_base_data0_show, NULL);
static DEVICE_ATTR(touchkey_baseline_data1, S_IRUGO,	/* RECENT inner baseline */
		cypress_touchkey_base_data1_show, NULL);
static DEVICE_ATTR(touchkey_baseline_data2, S_IRUGO,	/* BACK outer baseline */
		cypress_touchkey_base_data2_show, NULL);
static DEVICE_ATTR(touchkey_baseline_data3, S_IRUGO,	/* BACK inner baseline */
		cypress_touchkey_base_data3_show, NULL);

static DEVICE_ATTR(touchkey_recent_idac_outer, S_IRUGO,	/* RECENT outer idac */
		cypress_touchkey_idac0_show, NULL);
static DEVICE_ATTR(touchkey_recent_idac_inner, S_IRUGO,	/* RECENT inner idac */
		cypress_touchkey_idac1_show, NULL);
static DEVICE_ATTR(touchkey_back_idac_outer, S_IRUGO,	/* BACK outer idac */
		cypress_touchkey_idac2_show, NULL);
static DEVICE_ATTR(touchkey_back_idac_inner, S_IRUGO,	/* BACK inner idac */
		cypress_touchkey_idac3_show, NULL);

static DEVICE_ATTR(touchkey_comp_idac0, S_IRUGO,
		cypress_touchkey_comp_idac0_show, NULL);
static DEVICE_ATTR(touchkey_comp_idac1, S_IRUGO,
		cypress_touchkey_comp_idac1_show, NULL);
static DEVICE_ATTR(touchkey_comp_idac2, S_IRUGO,
		cypress_touchkey_comp_idac2_show, NULL);
static DEVICE_ATTR(touchkey_comp_idac3, S_IRUGO,
		cypress_touchkey_comp_idac3_show, NULL);

static DEVICE_ATTR(touchkey_recent_threshold_outer, S_IRUGO,	/* RECENT outer threshold */
		cypress_touchkey_threshold_show, NULL);
static DEVICE_ATTR(touchkey_recent_threshold_inner, S_IRUGO,	/* RECENT inner threshold */
		cypress_touchkey_threshold2_show, NULL);
static DEVICE_ATTR(touchkey_back_threshold_outer, S_IRUGO,	/* BACK outer threshold */
		cypress_touchkey_threshold3_show, NULL);
static DEVICE_ATTR(touchkey_back_threshold_inner, S_IRUGO,	/* BACK inner threshold */
		cypress_touchkey_threshold4_show, NULL);

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
#ifdef CONFIG_GLOVE_TOUCH
static DEVICE_ATTR(glove_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		cypress_touchkey_glove_mode_enable);
#endif

#ifdef TKEY_FLIP_MODE
static DEVICE_ATTR(flip_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		cypress_touchkey_flip_cover_mode_enable);
#endif
#ifdef TKEY_1MM_MODE
static DEVICE_ATTR(1mm_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		cypress_touchkey_1mm_mode_enable);
#endif
#ifdef TSP_BOOSTER
static DEVICE_ATTR(boost_level,
		   S_IWUSR | S_IWGRP, NULL, boost_level_store);
#endif

static struct attribute *touchkey_attributes[] = {
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_brightness.attr,
	&dev_attr_touch_sensitivity.attr,
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_recent_outer.attr,
	&dev_attr_touchkey_recent_inner.attr,
	&dev_attr_touchkey_back_outer.attr,
	&dev_attr_touchkey_back_inner.attr,
	&dev_attr_touchkey_recent_raw_outer.attr,
	&dev_attr_touchkey_recent_raw_inner.attr,
	&dev_attr_touchkey_back_raw_outer.attr,
	&dev_attr_touchkey_back_raw_inner.attr,
	&dev_attr_touchkey_baseline_data0.attr,
	&dev_attr_touchkey_baseline_data1.attr,
	&dev_attr_touchkey_baseline_data2.attr,
	&dev_attr_touchkey_baseline_data3.attr,
	&dev_attr_touchkey_recent_idac_outer.attr,
	&dev_attr_touchkey_recent_idac_inner.attr,
	&dev_attr_touchkey_back_idac_outer.attr,
	&dev_attr_touchkey_back_idac_inner.attr,
	&dev_attr_touchkey_comp_idac0.attr,
	&dev_attr_touchkey_comp_idac1.attr,
	&dev_attr_touchkey_comp_idac2.attr,
	&dev_attr_touchkey_comp_idac3.attr,
	&dev_attr_touchkey_recent_threshold_outer.attr,
	&dev_attr_touchkey_recent_threshold_inner.attr,
	&dev_attr_touchkey_back_threshold_outer.attr,
	&dev_attr_touchkey_back_threshold_inner.attr,
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
#ifdef TKEY_1MM_MODE
	&dev_attr_1mm_mode.attr,
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

	if (system_rev >= TKEY_MODULE_CHECK_REV) {
		dev_info(&client->dev, "TKEY MODULE 0x06! system_rev:%x\n", system_rev);
		fw_name = kasprintf(GFP_KERNEL, "%s/%s.fw",
			TKEY_FW_BUILTIN_PATH, TKEY_SEMCO_CYPRESS_FW_NAME);
	} else {
		if (info->module_ver == 0x01) {	/* DreamTech */
			fw_name = kasprintf(GFP_KERNEL, "%s/%s.fw",
				TKEY_FW_BUILTIN_PATH, TKEY_DTECH_CYPRESS_FW_NAME);
		} else if (info->module_ver == 0x02) {
			/* SEMCO module ver: 0x02 */
			fw_name = kasprintf(GFP_KERNEL, "%s/%s.fw",
				TKEY_FW_BUILTIN_PATH, TKEY_SEMCO02_CYPRESS_FW_NAME);
		} else {
			/* SEMCO module ver: ~0x04 */
			fw_name = kasprintf(GFP_KERNEL, "%s/%s.fw",
				TKEY_FW_BUILTIN_PATH, TKEY_SEMCO04_CYPRESS_FW_NAME);
		}
	}

	pr_info("%s!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",fw_name);
	ret = request_firmware(&info->fw, fw_name, &client->dev);
	if (ret) {
		dev_err(&client->dev, "error requesting built-in firmware (%d)"
			"\n", ret);
		goto out;
	}

	info->fw_img = (struct fw_image *)info->fw->data;
	info->src_fw_ver = info->fw_img->first_fw_ver;
	dev_info(&client->dev, "the fw 0x%x is loaded (size=%d) hdr_len=%d\n",
		 info->fw_img->first_fw_ver, info->fw_img->fw_len, info->fw_img->hdr_len);

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

	if (info->module_ver == 0x01) {/* DreamTech */
		fw_name = kasprintf(GFP_KERNEL, "%s/%s.fw",
			TKEY_FW_IN_SDCARD_PATH, TKEY_DTECH_CYPRESS_FW_NAME);
	} else { /* SEMCO module ver: 0x02, 0x03 */
		fw_name = kasprintf(GFP_KERNEL, "%s/%s.fw",
			TKEY_FW_IN_SDCARD_PATH, TKEY_SEMCO_CYPRESS_FW_NAME);
	}

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

	if (force == true || info->device_ver == 0x10)
		info->support_fw_update = true;
	else
		info->support_fw_update = false;

	dev_info(&client->dev, "%s : part fw ver:%#x, phone fw ver:%#x, header ver:%#x\n",
			 __func__ , info->ic_fw_ver, info->src_fw_ver, info->fw_img->hdr_ver);

	/* firmware version compare */
	if ((info->ic_fw_ver == info->src_fw_ver && !force) || info->support_fw_update == false) {
		dev_info(&client->dev, "%s : IC aleady have latest firmware (fw:%#x module:%#x)\n",
			 __func__ , info->ic_fw_ver, info->module_ver);
		if (info->fw_img->hdr_ver == 1)
			info->do_checksum = true;
		else
			goto out;
	} else if (info->ic_fw_ver > info->src_fw_ver) {
		dev_info(&client->dev, "%s : IC has test firmware above phone fw(%#x,), Do not checksum!\n",
				 __func__ , info->ic_fw_ver);
		goto out;
	}

#ifdef USE_TKEY_UPDATE_WORK
	queue_work(info->fw_wq, &info->update_work);
#else
	/* firmware update */
	ret = tkey_fw_update(info, force);
	if (ret < 0) {
		goto err_fw_update;
	}
err_fw_update:
#endif
out:
	tkey_unload_fw(info, fw_path);
	return ret;
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
	pdata->vcc_flag = of_property_read_bool(np, "vcc_en-supply");
	pdata->i2c_pull_up = of_property_read_bool(np, "cypress,i2c-pull-up");
	pdata->vdd_led = of_get_named_gpio(np, "vdd_led-gpio", 0);

	/* reset, irq gpio info */
	pdata->gpio_scl = of_get_named_gpio_flags(np, "cypress,scl-gpio",
				0, &pdata->scl_gpio_flags);
	pdata->gpio_sda = of_get_named_gpio_flags(np, "cypress,sda-gpio",
				0, &pdata->sda_gpio_flags);
	pdata->gpio_int = of_get_named_gpio_flags(np, "cypress,irq-gpio",
				0, &pdata->irq_gpio_flags);
	pdata->gpio_touchkey_id = of_get_named_gpio_flags(np, "cypress,touchkey_id-gpio",
				0, &pdata->gpio_touchkey_id_flags);

	return 0;
}
#else
static int cypress_parse_dt(struct device *dev,
			struct cypress_touchkey_platform_data *pdata)
{
	return -ENODEV;
}
#endif

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

	printk(KERN_ERR "%s: start!\n", __func__);

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

	wake_lock_init(&info->fw_wakelock, WAKE_LOCK_SUSPEND, "cypress_touchkey");

	for (i = 0; i < pdata->keycodes_size; i++) {
		info->keycode[i] = pdata->touchkey_keycode[i];
		set_bit(info->keycode[i], input_dev->keybit);
	}

	i2c_set_clientdata(client, info);

	info->is_powering_on = true;
	cypress_power_onoff(info, 1);

	msleep(150);

	info->enabled = true;
	ret = tkey_i2c_check(info);
	if (ret < 0) {
		dev_err(&client->dev, "i2c_check failed\n");
		bforced = true;
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
	info->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	info->early_suspend.suspend = cypress_touchkey_early_suspend;
	info->early_suspend.resume = cypress_touchkey_late_resume;
	register_early_suspend(&info->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */

#ifdef CONFIG_GLOVE_TOUCH
	mutex_init(&info->tkey_glove_lock);
	INIT_DELAYED_WORK(&info->glove_work, cypress_touchkey_glove_work);
#endif

#ifdef TKEY_REQUEST_FW_UPDATE
#ifdef USE_TKEY_UPDATE_WORK
	info->fw_wq = create_singlethread_workqueue("cypress_touchkey");
	if (!info->fw_wq) {
		dev_err(&client->dev, "fail to create workqueue for fw_wq\n");
		goto err_create_fw_wq;
	} else {
		INIT_WORK(&info->update_work, tkey_fw_update_work);
	}
#endif
	ret = tkey_flash_fw(info, FW_BUILT_IN, bforced);
  	if (ret < 0) {
		dev_info(&info->client->dev,
			"%s: tkey fw update failed.\n", __func__);
		goto err_fw_update;
	}
#else
	info->src_fw_ver = BIN_FW_VERSION;
#endif

	if (info->device_ver != 0x10) /*not CY8C20075*/
		cypress_touchkey_auto_cal(info, true);

	sec_touchkey = device_create(sec_class, NULL, 0, NULL, "sec_touchkey");
	if (IS_ERR(sec_touchkey)) {
		dev_info(&info->client->dev,
				"%s: Failed to create device(sec_touchkey)!\n", __func__);
		goto err_sysfs;
	}
	dev_set_drvdata(sec_touchkey, info);

	ret = sysfs_create_group(&sec_touchkey->kobj, &touchkey_attr_group);
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
#ifdef USE_TKEY_UPDATE_WORK
	destroy_workqueue(info->fw_wq);
err_create_fw_wq:
#endif
#endif
#ifdef CONFIG_GLOVE_TOUCH
	mutex_destroy(&info->tkey_glove_lock);
#endif
	if (info->irq >= 0)
		free_irq(info->irq, info);
err_req_irq:
/*
err_gpio_request:
*/
	input_unregister_device(input_dev);
err_reg_input_dev:
	input_free_device(input_dev);
	input_dev = NULL;
	wake_lock_destroy(&info->fw_wakelock);
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

	if (wake_lock_active(&info->fw_wakelock)) {
		dev_err(&info->client->dev, "wackelock active(%s)\n",
					__func__);
		return 0;
	}

#ifdef TSP_BOOSTER
	cypress_set_dvfs_lock(info, 2);
	dev_info(&info->client->dev,
			"%s: dvfs_lock free.\n", __func__);
#endif
	info->is_powering_on = true;
	disable_irq(info->irq);
	info->enabled = false;
	cypress_power_onoff(info, 0);

	return ret;
}

static int cypress_touchkey_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cypress_touchkey_info *info = i2c_get_clientdata(client);
	int ret = 0;

	if (wake_lock_active(&info->fw_wakelock)) {
		dev_err(&info->client->dev, "wackelock active(%s)\n",
					__func__);
		return 0;
	}

	cypress_power_onoff(info, 1);
	msleep(50);

	if (info->device_ver != 0x10) {
		info->enabled = true;
		cypress_touchkey_auto_cal(info, false);
	} else {
		msleep(100);
#ifdef CYPRESS_SUPPORT_DUAL_INT_MODE
	/* CYPRESS Firmware setting interrupt type : dual or single interrupt */
		cypress_touchkey_interrupt_set_dual(info->client);
#endif
		info->enabled = true;
	}

	if (touchled_cmd_reversed) {
		touchled_cmd_reversed = 0;
		ret = i2c_smbus_write_byte_data(info->client,
				CYPRESS_GEN, touchkey_led_status);
		if (ret < 0) {
			dev_err(&info->client->dev,
					"%s: i2c write error [%d]\n", __func__, ret);
			goto out;
		}
		dev_info(&client->dev,
				"%s: LED returned on\n", __func__);
		msleep(30);
	}
out:
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
