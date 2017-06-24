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
#include <linux/i2c/cypress_touchkey.h>
#include "cypress_tkey_fw.h"
#include <linux/regulator/consumer.h>
#include <asm/mach-types.h>
#include <linux/device.h>
/* #include <mach/msm8974-gpio.h> */
#include <linux/of_gpio.h>

#ifdef CONFIG_LEDS_CLASS
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/leds.h>
#endif
#include "issp_extern.h"
#include <linux/mfd/pm8xxx/pm8921.h>
//#include "../../../../arch/arm/mach-msm/board-8064.h"

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

#define CYPRESS_LED_ON		0X10
#define CYPRESS_LED_OFF		0X20
#define CYPRESS_DATA_UPDATE	0X40
#define CYPRESS_AUTO_CAL	0X50
#define CYPRESS_LED_CONTROL_ON	0X60
#define CYPRESS_LED_CONTROL_OFF	0X70
#define CYPRESS_SLEEP		0X80

#define CYPRESS_FW_ID_REG	0X05
/*
#define CYPRESS_55_IC_MASK	0x20
#define CYPRESS_65_IC_MASK	0x04
*/

#define USE_OPEN_CLOSE
#undef DO_NOT_USE_FUNC_PARAM

static int vol_mv_level = 33;

#define KEYCODE_REG		0x00
/* static int vol_mv_level = 33; */

#ifdef CONFIG_LEDS_CLASS
#define TOUCHKEY_BACKLIGHT	"button-backlight"
#endif

/* bit masks*/
#define PRESS_BIT_MASK		0X08
#define KEYCODE_BIT_MASK	0X07

/*
#define TOUCHKEY_LOG(k, v) dev_notice(&info->client->dev, "key[%d] %d\n", k, v);
#define FUNC_CALLED dev_notice(&info->client->dev, "%s: called.\n", __func__);
*/
#define NUM_OF_RETRY_UPDATE	3
/*#define NUM_OF_KEY		4*/

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

#ifdef CONFIG_LEDS_CLASS
static int touchkey_led_status;
static int touchled_cmd_reserved;
#endif


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
	int retval;

	mutex_lock(&info->dvfs_lock);
	retval = set_freq_limit(DVFS_TOUCH_ID,
			MIN_TOUCH_LIMIT_SECOND);
	if (retval < 0)
		dev_info(&info->client->dev,
			"%s: booster change failed(%d).\n",
			__func__, retval);
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

	info->dvfs_lock_status = false;
	mutex_unlock(&info->dvfs_lock);
}

static void cypress_set_dvfs_lock(struct cypress_touchkey_info *info,
					uint32_t on)
{
	int ret = 0;

	mutex_lock(&info->dvfs_lock);
	if (on == 0) {
		if (info->dvfs_lock_status) {
			schedule_delayed_work(&info->work_dvfs_off,
				msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));
		}
	} else if (on == 1) {
		cancel_delayed_work(&info->work_dvfs_off);
		if (!info->dvfs_lock_status) {
			ret = set_freq_limit(DVFS_TOUCH_ID,
					MIN_TOUCH_LIMIT);
			if (ret < 0)
				dev_info(&info->client->dev,
					"%s: cpu first lock failed(%d)\n",
					__func__, ret);

			schedule_delayed_work(&info->work_dvfs_chg,
				msecs_to_jiffies(TOUCH_BOOSTER_CHG_TIME));
			info->dvfs_lock_status = true;
		}
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

	INIT_DELAYED_WORK(&info->work_dvfs_off, cypress_set_dvfs_off);
	INIT_DELAYED_WORK(&info->work_dvfs_chg, cypress_change_dvfs_lock);

	info->dvfs_lock_status = false;
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

#ifdef CYPRESS_MENU_BACK_MULTI_REPORT
static void cypress_touchkey_interrupt_set_dual(struct i2c_client *client)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(&client->dev);
	int retry = 5;
	u8 data[3] = {0, };

	if (info->fw_id & CYPRESS_65_IC_MASK) {
		dev_err(&client->dev, "%s: This module support dual, %x\n", __func__, info->fw_id);
	} else {
		dev_err(&client->dev, "%s: not support this this module. %x\n", __func__, info->fw_id);
		return;
	}

	if (info->fw_ver < CYPRESS_RECENT_BACK_REPORT_FW_VER) {
		dev_err(&client->dev, "%s: not support this version(%X/%X)\n", __func__, info->fw_ver, CYPRESS_RECENT_BACK_REPORT_FW_VER);
		return;
	}

	while (retry--) {
		int ret = 0;
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

#ifdef CONFIG_LEDS_CLASS
static void cypress_touchkey_led_work(struct work_struct *work)
{
	struct cypress_touchkey_info *info =
		container_of(work, struct cypress_touchkey_info, led_work);
	u8 buf;
	int ret;

	if(info->is_powering_on == true)
		touchled_cmd_reserved = 1;

	if (info->brightness == LED_OFF)
		buf = CYPRESS_LED_OFF;
	else
		buf = CYPRESS_LED_ON;

	dev_info(&info->client->dev, "%s: LED: 0x%2X \n",
			__func__, buf);

	if ((!info->enabled)) {
		touchled_cmd_reserved = 1;
		touchkey_led_status = buf;
		dev_info(&info->client->dev, "%s: Touchkey is not enabled.\n",
				__func__);
		return;
	}

	mutex_lock(&info->touchkey_mutex);

	ret = i2c_smbus_write_byte_data(info->client, CYPRESS_GEN, buf);
	if (ret < 0) {
		dev_info(&info->client->dev,
				"%s: i2c write error [%d]\n", __func__, ret);
		touchled_cmd_reserved = 1;
	}

	msleep(30);
	touchkey_led_status = buf;

	mutex_unlock(&info->touchkey_mutex);
}

static void cypress_touchkey_brightness_set(struct led_classdev *led_cdev,
			enum led_brightness brightness)
{
	/* Must not sleep, use a workqueue if needed */
	struct cypress_touchkey_info *info =
		container_of(led_cdev, struct cypress_touchkey_info, leds);

	info->brightness = brightness;

	queue_work(info->led_wq, &info->led_work);
}
#endif

static int reg_set_optimum_mode_check(struct regulator *reg, int load_uA)
{
	return (regulator_count_voltages(reg) > 0) ?
		regulator_set_optimum_mode(reg, load_uA) : 0;
}

void cypress_power_onoff(struct cypress_touchkey_info *info, int onoff)
{
	int ret = 0, rc = 0;

	dev_info(&info->client->dev, "%s: power %s\n",
			__func__, onoff ? "on" : "off");

	if (!info->vcc_en) {
		if (info->pdata->i2c_pull_up) {
			info->vcc_en = regulator_get(&info->client->dev,
				"vcc_en");
			if (IS_ERR(info->vcc_en)) {
				rc = PTR_ERR(info->vcc_en);
				dev_info(&info->client->dev,
					"Regulator(vcc_en) get failed rc=%d\n", rc);
				goto error_get_vtg_i2c;
			}
			if (regulator_count_voltages(info->vcc_en) > 0) {
				rc = regulator_set_voltage(info->vcc_en,
				1800000, 1800000);
				if (rc) {
					dev_info(&info->client->dev,
					"regulator(vcc_en) set_vtg failed rc=%d\n",
					rc);
					goto error_set_vtg_i2c;
				}
			}
		}
	}
	if (info->pdata->vdd_led < 0) {
		if (!info->vdd_led) {
			info->vdd_led = regulator_get(&info->client->dev,
				"vdd_led");
			if (IS_ERR(info->vdd_led)) {
				rc = PTR_ERR(info->vdd_led);
				dev_info(&info->client->dev,
					"Regulator(vdd_led) get failed rc=%d\n", rc);
				goto error_get_vtg_i2c;
			}
#if 0
			if (regulator_count_voltages(info->vdd_led) > 0) {
				rc = regulator_set_voltage(info->vdd_led,
				3300000, 3300000);
				if (rc) {
					dev_info(&info->client->dev,
					"regulator(vdd_led) set_vtg failed rc=%d\n",
					rc);
					goto error_set_vtg_i2c;
				}
			}
#endif
		}
	}
	if (onoff) {
		if (info->pdata->i2c_pull_up) {
			rc = reg_set_optimum_mode_check(info->vcc_en,
						10000);
			if (rc < 0) {
				dev_info(&info->client->dev,
				"Regulator vcc_en set_opt failed rc=%d\n",
				rc);
				goto error_reg_opt_i2c;
			}

			rc = regulator_enable(info->vcc_en);
			if (rc) {
				dev_info(&info->client->dev,
				"Regulator vcc_en enable failed rc=%d\n",
				rc);
				goto error_reg_en_vcc_en;
			}
			if (info->pdata->vdd_led < 0) {
				rc = reg_set_optimum_mode_check(info->vdd_led,
							10000);
				if (rc < 0) {
					dev_info(&info->client->dev,
					"Regulator vdd_led set_opt failed rc=%d\n",
					rc);
					goto error_reg_opt_i2c;
				}

				rc = regulator_enable(info->vdd_led);
				if (rc) {
					dev_info(&info->client->dev,
					"Regulator vdd_led enable failed rc=%d\n",
					rc);
					goto error_reg_en_vcc_en;
				}
			}
		}
	} else {
		if (info->pdata->i2c_pull_up) {
			reg_set_optimum_mode_check(info->vcc_en, 0);
			regulator_disable(info->vcc_en);
			if (info->pdata->vdd_led < 0) {
				reg_set_optimum_mode_check(info->vdd_led, 0);
				regulator_disable(info->vdd_led);
			}
		}
	}
//	msleep(50);

	if (info->pdata->vdd_led > 0) {
		ret = gpio_direction_output(info->pdata->vdd_led, onoff);
		if (ret) {
			dev_info(&info->client->dev,
					"[TKEY]%s: unable to set_direction for vdd_led [%d]\n",
					__func__, info->pdata->vdd_led);
		}
//		msleep(30);
	}
	return;

error_reg_en_vcc_en:
	if (info->pdata->i2c_pull_up) {
		reg_set_optimum_mode_check(info->vcc_en, 0);
		if (info->pdata->vdd_led < 0) 
			reg_set_optimum_mode_check(info->vdd_led, 0);
	}
error_reg_opt_i2c:
error_set_vtg_i2c:
	regulator_put(info->vcc_en);
	if (info->pdata->vdd_led < 0) 
		regulator_put(info->vdd_led);
error_get_vtg_i2c:
	return;
}


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

static ssize_t cypress_touchkey_brightness_level_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int count;

	count = snprintf(buf, 20, "%d\n", vol_mv_level);

	dev_info(&info->client->dev,
			"%s: Touch LED voltage = %d\n",
			__func__, vol_mv_level);

	return count;
}

#ifdef TK_INFORM_CHARGER
static int touchkey_ta_setting(struct cypress_touchkey_info *info)
{
	u8 data[6] = { 0, };
	int count = 0;
	int ret = 0;

	ret = cypress_touchkey_i2c_read(info->client, KEYCODE_REG, data, 4);
		if (ret < 0) {
		dev_info(&info->client->dev, "Failed to read Keycode_reg.\n");
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
		if (data[5] & TK_BIT_TA_ON)
			return count;
		else
			dev_info(&info->client->dev, "TA Enabled error\n");
			} else {
		if (!(data[5] & TK_BIT_TA_ON))
			return count;
		else
			dev_info(&info->client->dev, "TA Disabled error\n");
	}

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

	if ((info->fw_id & CYPRESS_55_IC_MASK) || (info->fw_id & CYPRESS_65_IC_MASK)) {
		dev_info(&info->client->dev, "%s: IC id %s, TA %s\n",
				__func__, (info->fw_id & CYPRESS_55_IC_MASK) ? "20055" : "20065",
				info->charging_mode ? "connected" : "disconnected");
		touchkey_ta_setting(info);
	} else {
		dev_info(&info->client->dev, "%s: 20045 IC, does not support TA mode\n",
				__func__);
		}

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

#ifdef TKEY_FLIP_MODE
	if (info->enabled_flip) {
		dev_info(&info->client->dev,"%s: Enabled flip cover mode.\n", __func__);
		return;
	}
#endif

	if ((info->fw_id & CYPRESS_55_IC_MASK) || (info->fw_id & CYPRESS_65_IC_MASK)) {
		dev_info(&info->client->dev, "%s: IC id %s\n",
				__func__, (info->fw_id & CYPRESS_55_IC_MASK) ? "20055" : "20065");

	} else {
		dev_info(&info->client->dev, "%s: 20045 IC, does not support glove mode\n",
				__func__);
		return;
	}

	mutex_lock(&info->touchkey_mutex);

	ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 4);
	if (ret < 0) {
		dev_info(&info->client->dev, "%s: Failed to read Keycode_reg.\n",
			__func__);
		goto out;
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
		if (data[5] & TK_BIT_GLOVE)
			dev_info(&info->client->dev, "%s: Glove mode is enabled\n", __func__);
		else
			dev_info(&info->client->dev, "%s: glove_mode Error\n", __func__);
	} else {
		if (!(data[5] & TK_BIT_GLOVE))
			dev_info(&info->client->dev, "%s: Normal mode from Glove mode\n", __func__);
		else
			dev_info(&info->client->dev, "%s: normal_mode Error\n", __func__);
	}

out:
	mutex_unlock(&info->touchkey_mutex);
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
	int count = 0;
	int ret = 0;

	if (!(info->enabled)) {
		dev_info(&info->client->dev, "%s : Touchkey is not enabled.\n",
				__func__);
		return ;
	}

	if (info->fw_id & CYPRESS_65_IC_MASK) {
		dev_info(&info->client->dev, "%s : IC id 20065\n", __func__);
	} else {
		dev_info(&info->client->dev, "%s : %s ID, flipmode does not support\n",
				__func__, info->fw_id & CYPRESS_55_IC_MASK ? "20055" : "20045");
		return;
	}

	mutex_lock(&info->touchkey_mutex);

	ret = cypress_touchkey_i2c_read(info->client, KEYCODE_REG, data, 4);
	if (ret < 0) {
		dev_info(&info->client->dev, "%s: Failed to read Keycode_reg.\n",
			__func__);
		goto out;
	}

	if (value == 1) {
		/* Send filp mode Command */
		data[0] = 0xA0;
		data[3] = 0x50;
	} else {
		data[0] = 0xA0;
		data[3] = 0x40;
	}

	count = cypress_touchkey_i2c_write(info->client, data, 4);

	msleep(100);

	/* Check autocal status */
	ret = cypress_touchkey_i2c_read(info->client, KEYCODE_REG, data, 6);

	if (value == 1){
		if (data[5] & TK_BIT_FLIP) {
			dev_info(&info->client->dev, "%s: Flip mode is enabled\n", __func__);
			info->enabled_flip = true;
		}
		else
			dev_info(&info->client->dev, "%s: flip_mode Enable failed.\n", __func__);
	} else {
		if (!(data[5] & TK_BIT_FLIP)) {
			dev_info(&info->client->dev, "%s: Normal mode form Flip mode\n", __func__);
			info->enabled_flip = false;
		}
		else
			dev_info(&info->client->dev, "%s: normal_mode Enable failed\n", __func__);
	}

out:
	mutex_unlock(&info->touchkey_mutex);
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

#ifdef CYPRESS_MENU_BACK_MULTI_REPORT
	if (info->fw_ver >= CYPRESS_RECENT_BACK_REPORT_FW_VER) {
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
				"%s: %s%s%X, fw_ver: 0x%x\n", __func__,
				menu_data ? (menu_press ? "menu P " : "menu R ") : "",
				back_data ? (back_press ? "back P " : "back R ") : "",
				buf[0], info->fw_ver);
#else
		dev_info(&info->client->dev, "%s: key %s%s fw_ver: 0x%x\n", __func__,
				menu_data ? (menu_press ? "P" : "R") : "",
				back_data ? (back_press ? "P" : "R") : "",
				info->fw_ver);
#endif
	} else
#endif
{
	        press = !(buf[0] & PRESS_BIT_MASK);
	        code = (int)(buf[0] & KEYCODE_BIT_MASK) - 1;
	        printk(KERN_ERR
		        "[TouchKey]press=%d, code=%d\n", press, code);

	        if (code < 0) {
		        dev_err(&info->client->dev,
				        "not profer interrupt 0x%2X.\n", buf[0]);
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

#ifdef AUTOCAL_WORKQUEUE
static void cypress_touchkey_autocal_work(struct work_struct *work)
{
	struct cypress_touchkey_info *info =
		container_of(work, struct cypress_touchkey_info, autocal_work);

	u8 data[6] = { 0, };
	int count = 0;
	int ret = 0;

	printk("%s\n", __func__);

	ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 4);
	if (ret < 0) {
		dev_info(&info->client->dev, "%s: autocal read fail.\n", __func__);
		return;
	}

	data[0] = 0x50;
	data[3] = 0x01;

	count = cypress_touchkey_i2c_write(info->client, data, 4);
	dev_info(&info->client->dev,
			"%s: data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
			__func__, data[0], data[1], data[2], data[3]);

	msleep(130);

	ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 6);

	if ((!(data[5] & 0x80)) || (ret < 0))
		dev_info(&info->client->dev, "%s: autocal failed[%x][%d]\n",
				__func__, data[5], ret);

	return;
}
#endif

static int cypress_touchkey_auto_cal(struct cypress_touchkey_info *info)
{
#ifdef AUTOCAL_WORKQUEUE
	if (!(info->enabled)) {
		dev_info(&info->client->dev, "%s Touchkey is not enabled.\n",
			__func__);
		return 0;
	}

	queue_work(info->autocal_wq, &info->autocal_work);

	return 0;
#else
	u8 data[6] = { 0, };
	int ret = 0;

	mutex_lock(&info->touchkey_mutex);

	ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 4);
	if (ret < 0) {
		dev_info(&info->client->dev, "%s: autocal read fail.\n", __func__);
		goto out;
	}

	data[0] = 0x50;
	data[3] = 0x01;

	ret = cypress_touchkey_i2c_write(info->client, data, 4);
	dev_info(&info->client->dev,
			"%s: data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
			__func__, data[0], data[1], data[2], data[3]);

	msleep(130);

	ret = cypress_touchkey_i2c_read(info->client, CYPRESS_GEN, data, 6);

	if ((!(data[5] & 0x80)) || (ret < 0))
		dev_info(&info->client->dev, "%s: autocal failed[%x][%d]\n",
				__func__, data[5], ret);

out:
	mutex_unlock(&info->touchkey_mutex);
	return ret;
#endif
}

static int cypress_touchkey_led_on(struct cypress_touchkey_info *info)
{
	u8 buf = CYPRESS_LED_ON;
	int ret;

	if ((!info->enabled)) {
		touchled_cmd_reserved = 1;
		touchkey_led_status = buf;
		dev_info(&info->client->dev, "%s: Touchkey is not enabled.\n",
				__func__);
		return -1;
	}

	mutex_lock(&info->touchkey_mutex);

	ret = i2c_smbus_write_byte_data(info->client, CYPRESS_GEN, buf);
	if (ret < 0) {
		dev_info(&info->client->dev,
				"%s: i2c write error [%d]\n", __func__, ret);
		touchled_cmd_reserved = 1;
		touchkey_led_status = buf;
		goto out;
    }

    msleep(30);

out :
	mutex_unlock(&info->touchkey_mutex);
	return ret;
}

static int cypress_touchkey_led_off(struct cypress_touchkey_info *info)
{
	u8 buf = CYPRESS_LED_OFF;
	int ret;

	if ((!info->enabled)) {
		touchled_cmd_reserved = 1;
		touchkey_led_status = buf;
		dev_info(&info->client->dev, "%s: Touchkey is not enabled.\n",
				__func__);
		return -1;
	}

	mutex_lock(&info->touchkey_mutex);

	ret = i2c_smbus_write_byte_data(info->client, CYPRESS_GEN, buf);
	if (ret < 0) {
		dev_info(&info->client->dev,
				"%s: i2c write error [%d]\n", __func__, ret);
		touchled_cmd_reserved = 1;
		touchkey_led_status = buf;
		goto out;
	}

	msleep(30);

out :
	mutex_unlock(&info->touchkey_mutex);

	return ret;
}

static ssize_t cypress_touchkey_version_read(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	u8 data;
	int count;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_FW_VER);
	dev_info(&info->client->dev,
			"%s : FW Ver 0x%02x\n", __func__, data);

	count = snprintf(buf, 20, "0x%02x\n", data);
	return count;
}

static ssize_t cypress_touchkey_version_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int count;

	count = snprintf(buf, 20, "0x%02x\n", BIN_FW_VERSION);
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
	if (info->touchkey_update_status == 0)
		count = snprintf(buff, sizeof(buff), "PASS\n");
	else if (info->touchkey_update_status == 1)
		count = snprintf(buff, sizeof(buff), "Downloading\n");
	else if (info->touchkey_update_status == -1)
		count = snprintf(buff, sizeof(buff), "Fail\n");
	return count;
}

static ssize_t cypress_touchkey_update_read(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int count = 0;
	char buff[16] = {0};

	dev_info(&info->client->dev, "%s: touchkey_update_read: %d\n",
			__func__, info->touchkey_update_status);
	if (info->touchkey_update_status == 0)
		count = snprintf(buff, sizeof(buff), "PASS\n");
	else if (info->touchkey_update_status == 1)
		count = snprintf(buff, sizeof(buff), "Downloading\n");
	else if (info->touchkey_update_status == -1)
		count = snprintf(buff, sizeof(buff), "Fail\n");
	return count;
}


static ssize_t cypress_touchkey_update_write(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf, size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int count = 0;
	int retry = NUM_OF_RETRY_UPDATE;
	char buff[16] = {0};
	u8 data;

	info->touchkey_update_status = 1;
	dev_info(&info->client->dev, "%s: touch_update_write!\n", __func__);

	disable_irq(info->irq);

	if (info->fw_id & CYPRESS_55_IC_MASK) {
		dev_info(&info->client->dev, "%s: IC id 20055, does not support\n", __func__);
		return 1;
		}
	else if (info->fw_id & CYPRESS_65_IC_MASK)
		dev_info(&info->client->dev, "%s: IC id 20065\n", __func__);
	else {
		dev_info(&info->client->dev, "%s: IC id 20045, does not support\n", __func__);
		enable_irq(info->irq);
		return 1;
		}

	while (retry--) {
/*
		if (ISSP_main() == 0) {
			dev_info(&info->client->dev,
				"[TouchKey] Update success!\n");
			msleep(50);
			cypress_touchkey_auto_cal(info);
			info->touchkey_update_status = 0;
			count = 1;
			break;
		}
*/
		dev_info(&info->client->dev,
				"%s: Touchkey_update failed... retry...\n", __func__);
	}

	if (retry <= 0) {
		if (info->pdata->gpio_led_en)
			cypress_touchkey_con_hw(info, false);
		msleep(300);
		count = 0;
		dev_info(&info->client->dev, "%s: Touchkey_update fail\n", __func__);
		info->touchkey_update_status = -1;
		return count;
	}

	msleep(500);

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_FW_VER);
	count = snprintf(buff, sizeof(buff), "0x%02x\n", data);
	dev_info(&info->client->dev,
			"%s : FW Ver 0x%02x\n", __func__, data);
	enable_irq(info->irq);

#ifdef CYPRESS_MENU_BACK_MULTI_REPORT
	/* CYPRESS Firmware setting interrupt type : dual or single interrupt */
	cypress_touchkey_interrupt_set_dual(info->client);
#endif

	return count;
}

static ssize_t cypress_touchkey_led_control(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);

	dev_info(&info->client->dev, "called %s\n", __func__);
	if (buf != NULL) {
		if (buf[0] == '1')
			cypress_touchkey_led_on(info);
		else
			cypress_touchkey_led_off(info);
#if defined(SEC_TOUCHKEY_DEBUG)
		dev_info(&info->client->dev,
			"[TouchKey] touch_led_control : %d\n", buf[0]);
#endif
	} else
		dev_info(&info->client->dev, "[TouchKey] touch_led_control Error\n");

	return size;
}

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
	msleep(150);
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

	printk(KERN_DEBUG "called %s data[10] = %d, data[11] =%d\n", __func__,
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

	printk(KERN_DEBUG "called %s data[12] = %d, data[13] =%d\n", __func__,
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
		cypress_touchkey_auto_cal(info);
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

	if (data == 1)
		cypress_touchkey_auto_cal(info);
	return 0;
}

static ssize_t cypress_touchkey_autocal_status(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	u8 data[6];
	int ret;
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);

	printk(KERN_DEBUG "[Touchkey] %s\n", __func__);

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


static DEVICE_ATTR(touchkey_firm_update_status, S_IRUGO | S_IWUSR | S_IWGRP,
		cypress_touchkey_firm_status_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO,
		cypress_touchkey_version_read, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO,
		cypress_touchkey_version_show, NULL);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO | S_IWUSR | S_IWGRP,
		cypress_touchkey_update_read, cypress_touchkey_update_write);
static DEVICE_ATTR(brightness, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, cypress_touchkey_led_control);
static DEVICE_ATTR(touch_sensitivity, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, cypress_touchkey_sensitivity_control);
static DEVICE_ATTR(touchkey_menu, S_IRUGO,
		cypress_touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_raw_data0, S_IRUGO,
		cypress_touchkey_raw_data0_show, NULL);
static DEVICE_ATTR(touchkey_idac0, S_IRUGO,
		cypress_touchkey_idac0_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO,
		cypress_touchkey_back_show, NULL);
static DEVICE_ATTR(touchkey_raw_data1, S_IRUGO,
		cypress_touchkey_raw_data1_show, NULL);
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
static DEVICE_ATTR(touchkey_brightness_level, S_IRUGO | S_IWUSR | S_IWGRP,
		cypress_touchkey_brightness_level_show,
		cypress_touchkey_brightness_control);
#if defined(CONFIG_GLOVE_TOUCH)
static DEVICE_ATTR(glove_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		cypress_touchkey_glove_mode_enable);
#endif

#ifdef TKEY_FLIP_MODE
static DEVICE_ATTR(flip_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		cypress_touchkey_flip_cover_mode_enable);
#endif

static struct attribute *touchkey_attributes[] = {
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_brightness.attr,
	&dev_attr_touch_sensitivity.attr,
	&dev_attr_touchkey_menu.attr,
	&dev_attr_touchkey_raw_data0.attr,
	&dev_attr_touchkey_idac0.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_raw_data1.attr,
	&dev_attr_touchkey_idac1.attr,
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_touchkey_autocal_start.attr,
	&dev_attr_autocal_enable.attr,
	&dev_attr_autocal_stat.attr,
	&dev_attr_touchkey_brightness_level.attr,
#ifdef CONFIG_GLOVE_TOUCH
	&dev_attr_glove_mode.attr,
#endif
#ifdef TKEY_FLIP_MODE
	&dev_attr_flip_mode.attr,
#endif
	NULL,
};

static struct attribute_group touchkey_attr_group = {
	.attrs = touchkey_attributes,
};

#if !defined(CONFIG_SEC_H_PROJECT) && !defined(CONFIG_SEC_JS_PROJECT) && !defined(CONFIG_SEC_FRESCO_PROJECT) /*HLTE temp update block(0426)*/
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

	ret = gpio_request(pdata->gpio_scl, "touchkey_scl");
	if (ret) {
		printk(KERN_ERR "%s: unable to request touchkey_scl [%d]\n",
				__func__, pdata->gpio_scl);
//		return;
	}

	gpio_tlmm_config(GPIO_CFG(pdata->gpio_scl, 3, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	ret = gpio_request(pdata->gpio_sda, "touchkey_sda");
	if (ret) {
		printk(KERN_ERR "%s: unable to request touchkey_sda [%d]\n",
				__func__, pdata->gpio_sda);
//		return;
	}

	gpio_tlmm_config(GPIO_CFG(pdata->gpio_sda, 3, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	ret = gpio_request(pdata->gpio_int, "touchkey_irq");
	if (ret) {
		printk(KERN_ERR "%s: unable to request touchkey_irq [%d]\n",
				__func__, pdata->gpio_int);
		return;
	}

	if (pdata->vdd_led > 0) {
		ret = gpio_request(pdata->vdd_led, "touchkey_vdd_led");
		if (ret) {
			printk(KERN_ERR "%s: unable to request touchkey_vdd_led [%d]\n",
					__func__, pdata->vdd_led);
			return;
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
	pdata->vdd_led = of_get_named_gpio(np, "vdd_led-gpio", 0);

	/* reset, irq gpio info */
	pdata->gpio_scl = of_get_named_gpio_flags(np, "cypress,scl-gpio",
				0, &pdata->scl_gpio_flags);
	pdata->gpio_sda = of_get_named_gpio_flags(np, "cypress,sda-gpio",
				0, &pdata->sda_gpio_flags);
	pdata->gpio_int = of_get_named_gpio_flags(np, "cypress,irq-gpio",
				0, &pdata->irq_gpio_flags);
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
	u8 data[6] = { 0, };
#if !defined(CONFIG_SEC_H_PROJECT) && !defined(CONFIG_SEC_JS_PROJECT) && !defined(CONFIG_SEC_FRESCO_PROJECT)	/*HLTE temp update block(0426)*/
	int retry = NUM_OF_RETRY_UPDATE;
#endif
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
	info->touchkey_update_status = 0;
/*
	snprintf(info->phys, sizeof(info->phys),
			"%s/input0", dev_name(&client->dev));
*/
	input_dev->name = "sec_touchkey";
	input_dev->phys = info->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);

	for (i = 0; i < pdata->keycodes_size; i++) {
		info->keycode[i] = pdata->touchkey_keycode[i];
		set_bit(info->keycode[i], input_dev->keybit);
	}

	input_set_drvdata(input_dev, info);

	ret = input_register_device(input_dev);
	if (ret) {
		dev_info(&client->dev, "failed to register input dev (%d).\n",
			ret);
		goto err_reg_input_dev;
	}

	i2c_set_clientdata(client, info);

#ifdef USE_OPEN_CLOSE
	input_dev->open = cypress_input_open;
	input_dev->close = cypress_input_close;
#endif

	info->is_powering_on = true;
	cypress_touchkey_con_hw(info, true);

	cypress_power_onoff(info, 1);
	msleep(50);
	info->enabled = true;

	dev_info(&info->client->dev, "gpio_to_irq IRQ %d\n",
			client->irq);

#ifdef TSP_BOOSTER
	cypress_init_dvfs(info);
#endif

	if (info->fw_id & CYPRESS_65_IC_MASK) {
		dev_info(&info->client->dev, "%s: This TKEY IC version is 20065\n",
				__func__);
	}
	ret = cypress_touchkey_i2c_read(info->client, KEYCODE_REG, data, 6);
	if (ret < 0) {
		/*
		disable_irq(client->irq);

		if (ISSP_main() == 0) {
			dev_info(&client->dev, "[TouchKey] Update success!\n");
			enable_irq(client->irq);
		   } else {
		   dev_info(&info->client->dev, "[TouchKey] %s i2c transfer error\n",
		   __func__);
			}
		 */
		dev_info(&info->client->dev, "%s: FW update does not active\n",
				__func__);
			goto err_i2c_check;
		}

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
#ifdef AUTOCAL_WORKQUEUE
	info->autocal_wq = create_singlethread_workqueue("cypress_touchkey");
	INIT_WORK(&info->autocal_work, cypress_touchkey_autocal_work);
#endif
#ifdef CONFIG_LEDS_CLASS
	mutex_init(&info->touchkey_mutex);
	info->led_wq = create_singlethread_workqueue("cypress_touchkey");
	INIT_WORK(&info->led_work, cypress_touchkey_led_work);
	info->leds.name = TOUCHKEY_BACKLIGHT;
	info->leds.brightness = LED_FULL;
	info->leds.max_brightness = LED_FULL;
	info->leds.brightness_set = cypress_touchkey_brightness_set;

	ret = led_classdev_register(&client->dev, &info->leds);
	if (ret)
		goto err_led_class_dev;
#endif
	msleep(30);

	info->fw_ver = data[1];
	dev_info(&info->client->dev, "%s: IC FW Version: 0x%02x\n",
			__func__, info->fw_ver);

	info->fw_id = data[5];
	dev_info(&info->client->dev, "%s: IC ID Version: 0x%02x\n",
			__func__, info->fw_id);

#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_JS_PROJECT) && !defined(CONFIG_SEC_FRESCO_PROJECT)	/*HLTE temp update block(0426)*/
	dev_info(&client->dev, "[TouchKey] FW update does not need!\n");
#else
	if ((info->fw_id & CYPRESS_65_IC_MASK) && (info->fw_ver >= BASE_FW_VERSION) && (info->fw_ver < BIN_FW_VERSION)) {
		dev_info(&info->client->dev, "[TouchKey] IC id 20065\n");
		dev_info(&info->client->dev, "[TouchKey] touchkey_update Start!!\n");

		disable_irq(client->irq);
		cypress_config_gpio_i2c(pdata, 0);

		while (retry--) {
			if (ISSP_main(info) == 0) {
				dev_info(&info->client->dev, "[TouchKey] Update success!\n");
				enable_irq(client->irq);
				break;
			}
			dev_info(&client->dev,
				"[TouchKey] Touchkey_update failed... retry...\n");
		}

		cypress_config_gpio_i2c(pdata, 1);

		if (retry <= 0) {
			cypress_power_onoff(info, 1);
			enable_irq(client->irq);
			if (info->pdata->gpio_led_en)
				cypress_touchkey_con_hw(info, false);
			msleep(300);
			dev_info(&client->dev, "[TouchKey] Touchkey_update fail\n");
		}

		msleep(500);

		info->fw_ver = i2c_smbus_read_byte_data(info->client,
				CYPRESS_FW_VER);
		dev_info(&client->dev,
			"[TouchKey] %s : FW Ver 0x%02x\n", __func__, info->fw_ver);
	} else if ((info->fw_id & CYPRESS_55_IC_MASK) && (info->fw_ver < BIN_FW_VERSION_20055)) {
		dev_info(&info->client->dev, "[TouchKey] IC id 20055\n");
		dev_info(&info->client->dev, "[TouchKey] touchkey_update Start!!\n");
		disable_irq(client->irq);

		cypress_config_gpio_i2c(pdata, 0);

		while (retry--) {
			if (ISSP_main(info) == 0) {
				dev_info(&info->client->dev, "[TouchKey] Update success!\n");
				enable_irq(client->irq);
				break;
			}
			dev_err(&client->dev,
				"[TouchKey] Touchkey_update failed... retry...\n");
		}

		cypress_config_gpio_i2c(pdata, 1);

		if (retry <= 0) {
			cypress_power_onoff(info, 1);
			enable_irq(client->irq);
			if (info->pdata->gpio_led_en)
				cypress_touchkey_con_hw(info, false);
			msleep(300);
			dev_err(&client->dev, "[TouchKey] Touchkey_update fail\n");
		}

		msleep(500);

		info->fw_ver = i2c_smbus_read_byte_data(info->client,
				CYPRESS_FW_VER);
		dev_info(&client->dev,
			"[TouchKey] %s : FW Ver 0x%02x\n", __func__, info->fw_ver);

	} else {
		dev_info(&client->dev, "[TouchKey] FW update does not need!\n");
	}
#ifdef CYPRESS_MENU_BACK_MULTI_REPORT
	/* CYPRESS Firmware setting interrupt type : dual or single interrupt */
	cypress_touchkey_interrupt_set_dual(info->client);
#endif
#endif
	cypress_touchkey_auto_cal(info);
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
		goto err_sysfs;
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

err_sysfs:
#ifdef CONFIG_LEDS_CLASS
err_led_class_dev:
	if (info->led_wq)
		destroy_workqueue(info->led_wq);
#endif
err_req_irq:
err_i2c_check:
/*
err_gpio_request:
*/
	input_unregister_device(input_dev);
err_reg_input_dev:
	input_free_device(input_dev);
	input_dev = NULL;
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
	mutex_destroy(&info->touchkey_mutex);
	led_classdev_unregister(&info->leds);
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

	info->is_powering_on = true;
	disable_irq(info->irq);
	info->enabled = false;
	cypress_touchkey_con_hw(info, false);
	cypress_power_onoff(info, 0);

#ifdef TSP_BOOSTER
	cypress_set_dvfs_lock(info, 2);
	dev_info(&info->client->dev,
			"%s: dvfs_lock free.\n", __func__);
#endif
	return ret;
}

static int cypress_touchkey_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cypress_touchkey_info *info = i2c_get_clientdata(client);
	int ret = 0;
	cypress_power_onoff(info, 1);
	cypress_touchkey_con_hw(info, true);
	msleep(100);
	info->enabled = true;

#ifdef CYPRESS_MENU_BACK_MULTI_REPORT
	/* CYPRESS Firmware setting interrupt type : dual or single interrupt */
	cypress_touchkey_interrupt_set_dual(info->client);
#endif

	mutex_lock(&info->touchkey_mutex);

#ifdef CONFIG_LEDS_CLASS
	if (touchled_cmd_reserved) {
		touchled_cmd_reserved = 0;
		i2c_smbus_write_byte_data(info->client,
				CYPRESS_GEN, touchkey_led_status);
		dev_info(&client->dev,
				"%s: LED returned on\n", __func__);
		msleep(30);
	}
#endif

	mutex_unlock(&info->touchkey_mutex);

	cypress_touchkey_auto_cal(info);

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
	printk(KERN_ERR "%s: init\n", __func__);
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
