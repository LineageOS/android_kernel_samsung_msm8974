/*
 *  wacom_i2c.c - Wacom G5 Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/wacom_i2c.h>

#include <linux/earlysuspend.h>

#include <linux/uaccess.h>
#include "wacom_i2c_func.h"
#include "wacom_i2c_flash.h"
#ifdef WACOM_IMPORT_FW_ALGO
#include "wacom_i2c_coord_tables.h"
#endif
#include <linux/of_gpio.h>

bool ums_binary;
unsigned char screen_rotate;
unsigned char user_hand = 1;

static struct wacom_features wacom_feature_EMR = {
	.comstat = COM_QUERY,
	.data = {0, 0, 0, 0, 0, 0, 0},
	.fw_version = 0x0,
	.fw_ic_version = 0x0,
	.firm_update_status = 0,
};

static void wacom_enable_irq(struct wacom_i2c *wac_i2c, bool enable)
{
	static int depth;

#if defined(CONFIG_SEC_LT03_PROJECT) || defined(CONFIG_SEC_VIENNA_PROJECT)
	mutex_lock(&wac_i2c->irq_lock);
#endif

	if (enable) {
		if (depth) {
			--depth;
			enable_irq(wac_i2c->irq);
#ifdef WACOM_PDCT_WORK_AROUND
			enable_irq(wac_i2c->irq_pdct);
#endif
		}
	} else {
		if (!depth) {
			++depth;
			disable_irq(wac_i2c->irq);
#ifdef WACOM_PDCT_WORK_AROUND
			disable_irq(wac_i2c->irq_pdct);
#endif
		}
	}
	
#if defined(CONFIG_SEC_LT03_PROJECT) || defined(CONFIG_SEC_VIENNA_PROJECT)
  mutex_unlock(&wac_i2c->irq_lock);
#endif

#ifdef WACOM_IRQ_DEBUG
	dev_info(&wac_i2c->client->dev,
			"%s: Enable %d, depth %d\n",
			__func__, (int)enable, depth);
#endif
}

static int wacom_start(struct wacom_i2c *wac_i2c)
{
	dev_info(&wac_i2c->client->dev, "%s\n", __func__);

	if (wac_i2c->wac_pdata->gpio_pen_reset_n > 0)
		gpio_direction_output(wac_i2c->wac_pdata->gpio_pen_reset_n, 1);

	gpio_direction_output(wac_i2c->wac_pdata->vdd_en, 1);

	wac_i2c->power_enable = true;
	return 0;
}

static int wacom_stop(struct wacom_i2c *wac_i2c)
{
	dev_info(&wac_i2c->client->dev, "%s\n", __func__);

	if (wac_i2c->wac_pdata->gpio_pen_reset_n > 0)
		gpio_direction_output(wac_i2c->wac_pdata->gpio_pen_reset_n, 0);

	gpio_direction_output(wac_i2c->wac_pdata->vdd_en, 0);

#ifdef WACOM_BOOSTER
	if (wac_i2c->dvfs_lock_status)
		wacom_set_dvfs_lock(wac_i2c, -1);
#endif

#ifdef USE_WACOM_BLOCK_KEYEVENT
	wac_i2c->touch_pressed = false;
	wac_i2c->touchkey_skipped = false;
#endif

	wac_i2c->power_enable = false;
	return 0;
}

static int wacom_reset_hw(struct wacom_i2c *wac_i2c)
{
	wacom_stop(wac_i2c);

#ifdef WACOM_RESETPIN_DELAY
	msleep(300);
#else
	msleep(30);
#endif
	wacom_start(wac_i2c);
	msleep(200);

	return 0;
}

#ifdef WACOM_HAVE_FWE_PIN
static void wacom_compulsory_flash_mode(struct wacom_i2c *wac_i2c, bool en)
{
	gpio_direction_output(wac_i2c->wac_pdata->gpio_pen_fwe1, en ? 1 : 0);
	dev_info(&wac_i2c->client->dev, "%s: FWE1 is %s\n",
			__func__, en ? "HIGH" : "LOW");
}
#endif

static void wacom_i2c_enable(struct wacom_i2c *wac_i2c)
{
	bool en = true;

	dev_info(&wac_i2c->client->dev,
			"%s\n", __func__);

#ifdef BATTERY_SAVING_MODE
	if (wac_i2c->battery_saving_mode
		&& wac_i2c->pen_insert)
		en = false;
#endif

	if (en) {
		if (!wac_i2c->power_enable)
			wac_i2c->wac_pdata->wacom_start(wac_i2c);

		cancel_delayed_work_sync(&wac_i2c->resume_work);
		schedule_delayed_work(&wac_i2c->resume_work, HZ / 5);
	}
}

static void wacom_i2c_disable(struct wacom_i2c *wac_i2c)
{
	if (wac_i2c->power_enable) {
		wacom_enable_irq(wac_i2c, false);

		/* release pen, if it is pressed */
		if (wac_i2c->pen_pressed || wac_i2c->side_pressed
			|| wac_i2c->pen_prox)
			forced_release(wac_i2c);

			wac_i2c->wac_pdata->wacom_stop(wac_i2c);
	}
}

static irqreturn_t wacom_interrupt(int irq, void *dev_id)
{
	struct wacom_i2c *wac_i2c = dev_id;

	wacom_i2c_coord(wac_i2c);

	return IRQ_HANDLED;
}

#if defined(WACOM_PDCT_WORK_AROUND)
static irqreturn_t wacom_interrupt_pdct(int irq, void *dev_id)
{
	struct wacom_i2c *wac_i2c = dev_id;

	if (wac_i2c->query_status == false)
		return IRQ_HANDLED;

	wac_i2c->pen_pdct = gpio_get_value(wac_i2c->wac_pdata->gpio_pen_pdct);

	dev_info(&wac_i2c->client->dev, "%s: pdct %d(%d) [%s]\n",
			__func__, wac_i2c->pen_pdct, wac_i2c->pen_prox,
			wac_i2c->pen_pdct ? "Released" : "Pressed");
#if 0
	if (wac_i2c->pen_pdct == PDCT_NOSIGNAL) {
		/* If rdy is 1, pen is still working*/
		if (wac_i2c->pen_prox == 0)
			forced_release(wac_i2c);
	} else if (wac_i2c->pen_prox == 0)
		forced_hover(wac_i2c);
#endif

	return IRQ_HANDLED;
}
#endif

#ifdef WACOM_PEN_DETECT
static void pen_insert_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
		container_of(work, struct wacom_i2c, pen_insert_dwork.work);

	if (wac_i2c->init_fail)
		return;
	wac_i2c->pen_insert = !gpio_get_value(wac_i2c->gpio_pen_insert);

	dev_info(&wac_i2c->client->dev, "%s: pen %s\n",
		__func__, wac_i2c->pen_insert ? "instert" : "remove");

	if (!wac_i2c->pen_insert) {
		input_report_key(wac_i2c->input_dev, KEY_WAKEUP, 1);
		input_sync(wac_i2c->input_dev);
		input_report_key(wac_i2c->input_dev, KEY_WAKEUP, 0);
		input_sync(wac_i2c->input_dev);
	}

#ifdef BATTERY_SAVING_MODE
	if (wac_i2c->pen_insert) {
		if (wac_i2c->battery_saving_mode)
			wacom_i2c_disable(wac_i2c);
	} else {
		wacom_i2c_enable(wac_i2c);
	}
#endif
}

static irqreturn_t wacom_pen_detect(int irq, void *dev_id)
{
	struct wacom_i2c *wac_i2c = dev_id;

	cancel_delayed_work_sync(&wac_i2c->pen_insert_dwork);
	schedule_delayed_work(&wac_i2c->pen_insert_dwork, HZ / 20);
	return IRQ_HANDLED;
}
#endif

static int wacom_i2c_input_open(struct input_dev *dev)
{
	struct wacom_i2c *wac_i2c = input_get_drvdata(dev);

	dev_info(&wac_i2c->client->dev,
			"%s\n", __func__);

	wacom_i2c_enable(wac_i2c);
	wac_i2c->enabled = true;
	return 0;
}

static void wacom_i2c_input_close(struct input_dev *dev)
{
	struct wacom_i2c *wac_i2c = input_get_drvdata(dev);

	dev_info(&wac_i2c->client->dev,
			"%s\n", __func__);

	wacom_i2c_disable(wac_i2c);
	wac_i2c->enabled = false;
}


static void wacom_i2c_set_input_values(struct i2c_client *client,
				       struct wacom_i2c *wac_i2c,
				       struct input_dev *input_dev)
{
	/*Set input values before registering input device */

	input_dev->name = "sec_e-pen";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->evbit[0] |= BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

#ifdef WACOM_PEN_DETECT
	input_dev->open = wacom_i2c_input_open;
	input_dev->close = wacom_i2c_input_close;
#endif

	__set_bit(ABS_X, input_dev->absbit);
	__set_bit(ABS_Y, input_dev->absbit);
	__set_bit(ABS_PRESSURE, input_dev->absbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(BTN_TOOL_PEN, input_dev->keybit);
	__set_bit(BTN_TOOL_RUBBER, input_dev->keybit);
	__set_bit(BTN_STYLUS, input_dev->keybit);
	__set_bit(KEY_UNKNOWN, input_dev->keybit);
	__set_bit(KEY_PEN_PDCT, input_dev->keybit);
	__set_bit(KEY_WAKEUP, input_dev->keybit);
#ifdef WACOM_USE_GAIN
	__set_bit(ABS_DISTANCE, input_dev->absbit);
#endif
	/*  __set_bit(BTN_STYLUS2, input_dev->keybit); */
	/*  __set_bit(ABS_MISC, input_dev->absbit); */

	/*softkey*/
#ifdef WACOM_USE_SOFTKEY

#if defined(CONFIG_SEC_VIENNA_PROJECT)
	__set_bit(KEY_RECENT, input_dev->keybit);
#else
	__set_bit(KEY_MENU, input_dev->keybit);
#endif
	__set_bit(KEY_BACK, input_dev->keybit);
#endif
}

#ifdef USE_WACOM_CALLBACK
static int wacom_check_emr_prox(struct wacom_g5_callbacks *cb)
{
	struct wacom_i2c *wac = container_of(cb, struct wacom_i2c, callbacks);

	dev_info(&wac_i2c->client->dev,
			"%s\n", __func__);

	return wac->pen_prox;
}
#endif

static void wacom_i2c_resume_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, resume_work.work);

	if (wac_i2c->init_fail)
		return;

/* This Code is workaround code for S.LSI AP models. */
/*
#if defined(WACOM_PDCT_WORK_AROUND)
	//irq_set_irq_type(wac_i2c->irq_pdct, IRQ_TYPE_EDGE_BOTH);
#endif
*/
	wacom_enable_irq(wac_i2c, true);

	dev_info(&wac_i2c->client->dev,
			"%s\n", __func__);
}

#ifdef USE_WACOM_BLOCK_KEYEVENT
static void wacom_i2c_touch_pressed_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, touch_pressed_work.work);

	cancel_delayed_work(&wac_i2c->touch_pressed_work);
	wac_i2c->touch_pressed = false;
}
#endif

#ifdef WACOM_RESETPIN_DELAY
static void wacom_reset(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
			container_of(work, struct wacom_i2c, work_wacom_reset.work);

	dev_err(&wac_i2c->client->dev,
				"%s: reset\n",
				__func__);

	wacom_enable_irq(wac_i2c, false);

	/* Reset IC */
		wacom_reset_hw(wac_i2c);

	wacom_enable_irq(wac_i2c, true);
}
#endif


#ifdef USE_WACOM_LCD_WORKAROUND
extern int ldi_fps(unsigned int input_fps);
void wacom_i2c_write_vsync(struct wacom_i2c *wac_i2c)
{
	int retval;

	if (wac_i2c->wait_done) {
		dev_info(&wac_i2c->client->dev, "%s write %d\n", __func__, wac_i2c->vsync);
		#if defined(CONFIG_MACH_FRESCOLTESKT) || defined(CONFIG_MACH_FRESCOLTEKTT) || defined(CONFIG_MACH_FRESCOLTELGT)
			retval = 0; // kyNam_131228_ ldi_fps(wac_i2c->vsync);
		#else
			retval = ldi_fps(wac_i2c->vsync);
		#endif
		if (!retval)
			dev_info(&wac_i2c->client->dev, "%s failed\n", __func__);
		wac_i2c->wait_done = false;
		schedule_delayed_work(&wac_i2c->read_vsync_work,
					msecs_to_jiffies(wac_i2c->delay_time * 1000));

	} else {
		dev_info(&wac_i2c->client->dev, "%s vsync waiting time..\n", __func__);
	}
}

static void wacom_i2c_read_vsync_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, read_vsync_work.work);

	wac_i2c->wait_done = true;
}

static void wacom_i2c_boot_done_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, boot_done_work.work);

	wac_i2c->boot_done = true;
}

static ssize_t epen_read_freq_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", wac_i2c->delay_time);
}

static ssize_t epen_read_freq_data_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	unsigned int val;

	sscanf(buf, "%d", &val);

	wac_i2c->delay_time = val;

	dev_info(&wac_i2c->client->dev, "%s: lcd noise workaround delay time is  %d\n",
			__func__, wac_i2c->delay_time);

	return count;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
#define wacom_i2c_suspend	NULL
#define wacom_i2c_resume	NULL
#endif

static ssize_t epen_firm_update_status_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	dev_info(&wac_i2c->client->dev, 
			"%s:(%d)\n", __func__,
			wac_i2c->wac_feature->firm_update_status);

	if (wac_i2c->wac_feature->firm_update_status == 2)
		return sprintf(buf, "PASS\n");
	else if (wac_i2c->wac_feature->firm_update_status == 1)
		return sprintf(buf, "DOWNLOADING\n");
	else if (wac_i2c->wac_feature->firm_update_status == -1)
		return sprintf(buf, "FAIL\n");
	else
		return 0;
}

static ssize_t epen_firm_version_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	dev_info(&wac_i2c->client->dev,
			"%s: 0x%x|0x%X\n", __func__,
			wac_i2c->wac_feature->fw_ic_version,
			wac_i2c->wac_feature->fw_version);

	return sprintf(buf, "%04X\t%04X\n",
			wac_i2c->wac_feature->fw_ic_version,
			wac_i2c->wac_feature->fw_version);
}

#if defined(WACOM_IMPORT_FW_ALGO)
static ssize_t epen_tuning_version_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	dev_info(&wac_i2c->client->dev,
			"%s: %s\n", __func__,
			wac_i2c->wac_pdata->basic_model);

	return sprintf(buf, "%s_%04X\n",
			wac_i2c->wac_pdata->basic_model,
			wac_i2c->wac_feature->fw_version);
}

static ssize_t epen_rotation_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	static bool factory_test;
	static unsigned char last_rotation;
	unsigned int val;

	sscanf(buf, "%u", &val);

	/* Fix the rotation value to 0(Portrait) when factory test(15 mode) */
	if (val == 100 && !factory_test) {
		factory_test = true;
		screen_rotate = 0;
		dev_info(&wac_i2c->client->dev,
				"%s, enter factory test mode\n",
				__func__);
	} else if (val == 200 && factory_test) {
		factory_test = false;
		screen_rotate = last_rotation;
		dev_info(&wac_i2c->client->dev,
				"%s, exit factory test mode\n",
				__func__);
	}

	/* Framework use index 0, 1, 2, 3 for rotation 0, 90, 180, 270 */
	/* Driver use same rotation index */
	if (val >= 0 && val <= 3) {
		if (factory_test)
			last_rotation = val;
		else
			screen_rotate = val;
	}

	/* 0: Portrait 0, 1: Landscape 90, 2: Portrait 180 3: Landscape 270 */
	dev_info(&wac_i2c->client->dev,
			"%s: rotate=%d\n", __func__, screen_rotate);

	return count;
}

static ssize_t epen_hand_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	unsigned int val;

	sscanf(buf, "%u", &val);

	if (val == 0 || val == 1)
		user_hand = (unsigned char)val;

	/* 0:Left hand, 1:Right Hand */
	dev_info(&wac_i2c->client->dev,
			"%s: hand=%u\n", __func__, user_hand);

	return count;
}
#endif

static bool check_update_condition(struct wacom_i2c *wac_i2c, const char buf)
{
	u32 fw_ic_ver = wac_i2c->wac_feature->fw_ic_version;
	bool bUpdate = false;

	dev_info(&wac_i2c->client->dev,
			"%s: system rev is 0x%02x, Dig_rev(0x%02x)\n",
			__func__, system_rev, WACOM_FW_UPDATE_REVISION);

	switch (buf) {
	case 'I':
	case 'K':
		bUpdate = true;
		break;
	case 'R':
	case 'W':
		if (fw_ic_ver <
			wac_i2c->wac_feature->fw_version)
			bUpdate = true;
		break;
	default:
		dev_info(&wac_i2c->client->dev,
				"%s: wrong parameter\n", __func__);
		bUpdate = false;
		break;
	}

	return bUpdate;
}

int wacom_i2c_firm_update(struct wacom_i2c *wac_i2c)
{
	int ret = 0;
	int retry = 3;

	while (retry--) {
		ret = wacom_i2c_select_flash_code(wac_i2c);
		if (ret < 0)
			dev_err(&wac_i2c->client->dev,
				"%s: failed to write firmware(%d)\n",
				__func__, ret);
		else
			dev_err(&wac_i2c->client->dev,
				"%s: Successed to write firmware(%d)\n",
				__func__, ret);
		/* Reset IC */
		wacom_reset_hw(wac_i2c);
		if (ret >= 0)
			return 0;
	}

	return ret;
}

static ssize_t epen_firmware_update_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	int ret = 1;
	u32 fw_ic_ver = wac_i2c->wac_feature->fw_ic_version;
	bool need_update = false;

	need_update = check_update_condition(wac_i2c, *buf);
	if (need_update == false) {
		dev_info(&wac_i2c->client->dev,
				"%s:Pass Update. Cmd %c, IC ver %04x, Ker ver %04x\n",
				__func__, *buf, fw_ic_ver, wac_i2c->wac_feature->fw_version);
		return count;
	} else {
		dev_info(&wac_i2c->client->dev,
			"%s:Update Start. IC fw ver : 0x%x, new fw ver : 0x%x\n",
			__func__, wac_i2c->wac_feature->fw_ic_version,
			wac_i2c->wac_feature->fw_version);
	}

	switch (*buf) {
	/*ums*/
	case 'I':
		ret = wacom_fw_load_from_UMS(wac_i2c);
		if (ret)
			goto failure;
		dev_info(&wac_i2c->client->dev,
			"%s: Start firmware flashing (UMS image).\n",
			__func__);	
		ums_binary = true;
		break;
	/*kernel*/
	case 'K':
		ret = wacom_load_fw_from_req_fw(wac_i2c);
		if (ret)
			goto failure;
		break;

	/*booting*/
	case 'R':
		ret = wacom_load_fw_from_req_fw(wac_i2c);
		if (ret)
			goto failure;
		dev_info(&wac_i2c->client->dev,
		"%s: Start firmware flashing (kernel image).\n",
		__func__);

		break;
	default:
		/*There's no default case*/
		break;
	}

	/*start firm update*/
	mutex_lock(&wac_i2c->lock);
	wacom_enable_irq(wac_i2c, false);
	wac_i2c->wac_feature->firm_update_status = 1;

	ret = wacom_i2c_firm_update(wac_i2c);
	if (ret)
		goto update_err;
	wacom_i2c_set_firm_data(NULL);
	wacom_i2c_query(wac_i2c);
	wac_i2c->wac_feature->firm_update_status = 2;
	wacom_enable_irq(wac_i2c, true);
	mutex_unlock(&wac_i2c->lock);

	return count;
 update_err:
	wacom_i2c_set_firm_data(NULL);
 failure:
	wac_i2c->wac_feature->firm_update_status = -1;
	wacom_enable_irq(wac_i2c, true);
	mutex_unlock(&wac_i2c->lock);
	return -1;
}

static ssize_t epen_reset_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	int val;

	sscanf(buf, "%d", &val);

	if (val == 1) {
		wacom_enable_irq(wac_i2c, false);

		/* Reset IC */
		wacom_reset_hw(wac_i2c);

		/* I2C Test */
		wacom_i2c_query(wac_i2c);

		wacom_enable_irq(wac_i2c, true);

		dev_info(&wac_i2c->client->dev,
				"%s, result %d\n", __func__,
		       wac_i2c->query_status);
	}

	return count;
}

static ssize_t epen_reset_result_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	if (wac_i2c->query_status) {
		dev_info(&wac_i2c->client->dev,
				"%s, PASS\n", __func__);
		return sprintf(buf, "PASS\n");
	} else {
		dev_info(&wac_i2c->client->dev,
				"%s, FAIL\n", __func__);
		return sprintf(buf, "FAIL\n");
	}
}

#ifdef WACOM_USE_AVE_TRANSITION
static ssize_t epen_ave_store(struct device *dev,
struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	int v1, v2, v3, v4, v5;
	int height;

	sscanf(buf, "%d%d%d%d%d%d", &height, &v1, &v2, &v3, &v4, &v5);

	if (height < 0 || height > 2) {
		dev_info(&wac_i2c->client->dev,
				"%s: Height err %d\n", __func__, height);
		return count;
	}

	g_aveLevel_C[height] = v1;
	g_aveLevel_X[height] = v2;
	g_aveLevel_Y[height] = v3;
	g_aveLevel_Trs[height] = v4;
	g_aveLevel_Cor[height] = v5;

	dev_info(&wac_i2c->client->dev,
			"%s: v1 %d v2 %d v3 %d v4 %d\n", __func__,
			v1, v2, v3, v4);

	return count;
}

static ssize_t epen_ave_result_show(struct device *dev,
struct device_attribute *attr,
	char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	dev_info(&wac_i2c->client->dev,
			"%s: %d %d %d %d, %d %d %d %d, %d %d %d %d\n",
			__func__,
			g_aveLevel_C[0], g_aveLevel_X[0],
			g_aveLevel_Y[0], g_aveLevel_Trs[0],
			g_aveLevel_C[1], g_aveLevel_X[1],
			g_aveLevel_Y[1], g_aveLevel_Trs[1],
			g_aveLevel_C[2], g_aveLevel_X[2],
			g_aveLevel_Y[2], g_aveLevel_Trs[2]);

	return sprintf(buf, "%d %d %d %d\n%d %d %d %d\n%d %d %d %d\n",
			g_aveLevel_C[0], g_aveLevel_X[0],
			g_aveLevel_Y[0], g_aveLevel_Trs[0],
			g_aveLevel_C[1], g_aveLevel_X[1],
			g_aveLevel_Y[1], g_aveLevel_Trs[1],
			g_aveLevel_C[2], g_aveLevel_X[2],
			g_aveLevel_Y[2], g_aveLevel_Trs[2]);
}
#endif

static ssize_t epen_checksum_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	int val;

	sscanf(buf, "%d", &val);

	if (val != 1) {
	dev_info(&wac_i2c->client->dev,
			"%s: wrong cmd %d\n", __func__, val);
		return count;
	}

		wacom_enable_irq(wac_i2c, false);
		wacom_checksum(wac_i2c);
		wacom_enable_irq(wac_i2c, true);

	dev_info(&wac_i2c->client->dev,
			"%s: result %d\n",
			__func__, wac_i2c->checksum_result);

	return count;
}

static ssize_t epen_checksum_result_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	if (wac_i2c->checksum_result) {
		dev_info(&wac_i2c->client->dev,
				"%s: checksum, PASS\n", __func__);
		return sprintf(buf, "PASS\n");
	} else {
		dev_info(&wac_i2c->client->dev,
				"%s: checksum, FAIL\n", __func__);
		return sprintf(buf, "FAIL\n");
	}
}

#ifdef WACOM_CONNECTION_CHECK
static ssize_t epen_connection_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buff)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	u8 cmd = 0;
	u8 buf[2] = {0,};
	int ret = 0, cnt = 10;

	disable_irq(wac_i2c->client->irq);

	cmd = WACOM_I2C_STOP;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, false);
	if (ret <= 0) {
		dev_err(&wac_i2c->client->dev,
			 "%s: failed to send stop command\n",
			 __func__);
		goto grid_check_error;
	}

	cmd = WACOM_I2C_GRID_CHECK;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, false);
	if (ret <= 0) {
		dev_err(&wac_i2c->client->dev,
			 "%s: failed to send stop command\n",
			 __func__);
		goto grid_check_error;
	}

	cmd = WACOM_STATUS;
	do {
		msleep(50);
		if (1 == wacom_i2c_send(wac_i2c, &cmd, 1, false)) {
			if (2 == wacom_i2c_recv(wac_i2c,
						buf, 2, false)) {
				switch (buf[0]) {
				/*
				*	status value
				*	0 : data is not ready
				*	1 : PASS
				*	2 : Fail (coil function error)
				*	3 : Fail (All coil function error)
				*/
				case 1:
				case 2:
				case 3:
					cnt = 0;
					break;

				default:
					break;
				}
			}
		}
	} while (cnt--);

	dev_info(&wac_i2c->client->dev,
			 "%s : status: %x, error code: %x\n",
		       __func__, buf[0], buf[1]);

grid_check_error:
	cmd = WACOM_I2C_STOP;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, false);
	if (ret <= 0)
		dev_err(&wac_i2c->client->dev,
			 "%s: failed to send stop command\n",
			 __func__);

	cmd = WACOM_I2C_START;
	wacom_i2c_send(wac_i2c, &cmd, 1, false);
	if (ret <= 0)
		dev_err(&wac_i2c->client->dev,
			 "%s: failed to send stop command\n",
			 __func__);

	enable_irq(wac_i2c->client->irq);

	if ((buf[0] == 0x1) && (buf[1] == 0))
		return sprintf(buff, "%s\n", "OK");
	else
		return sprintf(buff, "%s\n", "NG");
}
#endif

#ifdef BATTERY_SAVING_MODE
static ssize_t epen_saving_mode_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	int val;

	if (sscanf(buf, "%u", &val) == 1)
		wac_i2c->battery_saving_mode = !!val;

	dev_info(&wac_i2c->client->dev, "%s: %s\n",
			__func__, val ? "checked" : "unchecked");

	if (wac_i2c->battery_saving_mode) {
		if (wac_i2c->pen_insert)
			wacom_i2c_disable(wac_i2c);
	} else {
		if (wac_i2c->enabled)
			wacom_i2c_enable(wac_i2c);
	}
	return count;
}
#endif
#ifdef WACOM_BOOSTER
static ssize_t boost_level_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	int val, retval;

	dev_info(&wac_i2c->client->dev, "%s\n", __func__);
	sscanf(buf, "%d", &val);

	if (val != 1 && val != 2 && val != 3 && val != 0) {
		dev_info(&wac_i2c->client->dev,
			"%s: wrong cmd %d\n", __func__, val);
		return count;
	}
	wac_i2c->dvfs_boost_mode = val;
	dev_info(&wac_i2c->client->dev,
			"%s: dvfs_boost_mode = %d\n",
			__func__, wac_i2c->dvfs_boost_mode);

	if (wac_i2c->dvfs_boost_mode != DVFS_STAGE_TRIPLE) {
		wac_i2c->dvfs_freq = -1;
	} else if (wac_i2c->dvfs_boost_mode == DVFS_STAGE_NONE) {
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		if (retval < 0) {
			dev_err(&wac_i2c->client->dev,
					"%s: booster stop failed(%d).\n",
					__func__, retval);
			wac_i2c->dvfs_lock_status = false;
		}
	}
	return count;
}
#endif

#ifdef USE_WACOM_BLOCK_KEYEVENT
static ssize_t epen_delay_time_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u msec\n", wac_i2c->key_delay_time);
}

static ssize_t epen_delay_time_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	unsigned int val;

	sscanf(buf, "%u", &val);

	if (val > 0)
		wac_i2c->key_delay_time = val;

	dev_info(&wac_i2c->client->dev, "%s: delay time : %d\n",
			__func__, wac_i2c->key_delay_time);
	return count;
}
#endif

#ifdef USE_WACOM_BLOCK_KEYEVENT
static DEVICE_ATTR(epen_delay_time,
		   S_IRUGO| S_IWUSR | S_IWGRP, epen_delay_time_show, epen_delay_time_store);
#endif
#ifdef USE_WACOM_LCD_WORKAROUND
static DEVICE_ATTR(epen_read_freq,
		   S_IRUGO| S_IWUSR | S_IWGRP, epen_read_freq_show, epen_read_freq_data_store);
#endif
/* firmware update */
static DEVICE_ATTR(epen_firm_update,
		   S_IWUSR | S_IWGRP, NULL, epen_firmware_update_store);
/* return firmware update status */
static DEVICE_ATTR(epen_firm_update_status,
		   S_IRUGO, epen_firm_update_status_show, NULL);
/* return firmware version */
static DEVICE_ATTR(epen_firm_version, S_IRUGO, epen_firm_version_show, NULL);
#if defined(WACOM_IMPORT_FW_ALGO)
/* return tuning data version */
static DEVICE_ATTR(epen_tuning_version, S_IRUGO,
			epen_tuning_version_show, NULL);
/* screen rotation */
static DEVICE_ATTR(epen_rotation, S_IWUSR | S_IWGRP, NULL, epen_rotation_store);
/* hand type */
static DEVICE_ATTR(epen_hand, S_IWUSR | S_IWGRP, NULL, epen_hand_store);
#endif

/* For SMD Test */
static DEVICE_ATTR(epen_reset, S_IWUSR | S_IWGRP, NULL, epen_reset_store);
static DEVICE_ATTR(epen_reset_result,
		   S_IRUSR | S_IRGRP, epen_reset_result_show, NULL);

/* For SMD Test. Check checksum */
static DEVICE_ATTR(epen_checksum, S_IWUSR | S_IWGRP, NULL, epen_checksum_store);
static DEVICE_ATTR(epen_checksum_result, S_IRUSR | S_IRGRP,
		   epen_checksum_result_show, NULL);

#ifdef WACOM_USE_AVE_TRANSITION
static DEVICE_ATTR(epen_ave, S_IWUSR | S_IWGRP, NULL, epen_ave_store);
static DEVICE_ATTR(epen_ave_result, S_IRUSR | S_IRGRP,
	epen_ave_result_show, NULL);
#endif

#ifdef WACOM_CONNECTION_CHECK
static DEVICE_ATTR(epen_connection,
		   S_IRUGO, epen_connection_show, NULL);
#endif

#ifdef BATTERY_SAVING_MODE
static DEVICE_ATTR(epen_saving_mode,
		   S_IWUSR | S_IWGRP, NULL, epen_saving_mode_store);
#endif
#ifdef WACOM_BOOSTER
static DEVICE_ATTR(boost_level,
		   S_IWUSR | S_IWGRP, NULL, boost_level_store);
#endif

static struct attribute *epen_attributes[] = {
#ifdef USE_WACOM_BLOCK_KEYEVENT
	&dev_attr_epen_delay_time.attr,
#endif
#ifdef USE_WACOM_LCD_WORKAROUND
	&dev_attr_epen_read_freq.attr,
#endif
	&dev_attr_epen_firm_update.attr,
	&dev_attr_epen_firm_update_status.attr,
	&dev_attr_epen_firm_version.attr,
#if defined(WACOM_IMPORT_FW_ALGO)
	&dev_attr_epen_tuning_version.attr,
	&dev_attr_epen_rotation.attr,
	&dev_attr_epen_hand.attr,
#endif
	&dev_attr_epen_reset.attr,
	&dev_attr_epen_reset_result.attr,
	&dev_attr_epen_checksum.attr,
	&dev_attr_epen_checksum_result.attr,
#ifdef WACOM_USE_AVE_TRANSITION
	&dev_attr_epen_ave.attr,
	&dev_attr_epen_ave_result.attr,
#endif
#ifdef WACOM_CONNECTION_CHECK
	&dev_attr_epen_connection.attr,
#endif
#ifdef BATTERY_SAVING_MODE
	&dev_attr_epen_saving_mode.attr,
#endif
#ifdef WACOM_BOOSTER
	&dev_attr_boost_level.attr,
#endif
	NULL,
};

static struct attribute_group epen_attr_group = {
	.attrs = epen_attributes,
};

static int wacom_firmware_update(struct wacom_i2c *wac_i2c)
{
	int ret = 0;

	ret = wacom_load_fw_from_req_fw(wac_i2c);
	if (ret)
		goto failure;

	if (wac_i2c->wac_feature->fw_ic_version < wac_i2c->wac_feature->fw_version) {	
		/*start firm update*/
		dev_info(&wac_i2c->client->dev,
				"%s: Start firmware flashing (kernel image).\n",
				__func__);		
		mutex_lock(&wac_i2c->lock);
		wacom_enable_irq(wac_i2c, false);		
		wac_i2c->wac_feature->firm_update_status = 1;
		ret = wacom_i2c_firm_update(wac_i2c);
		if (ret)
			goto update_err;
		wacom_i2c_set_firm_data(NULL);
		wacom_i2c_query(wac_i2c);
		wac_i2c->wac_feature->firm_update_status = 2;
		wacom_enable_irq(wac_i2c, true);
		mutex_unlock(&wac_i2c->lock);
	} else {
		dev_info(&wac_i2c->client->dev,
			"%s: firmware update does not need.\n",
			__func__);
	}
	return ret;

	update_err:
		wacom_i2c_set_firm_data(NULL);	
		wac_i2c->wac_feature->firm_update_status = -1;
		wacom_enable_irq(wac_i2c, true);
		mutex_unlock(&wac_i2c->lock);		
	failure:
		return ret;
}

static void wacom_init_abs_params(struct wacom_i2c *wac_i2c)
{

	wac_i2c->wac_feature->x_max = wac_i2c->wac_pdata->max_x;
	wac_i2c->wac_feature->y_max = wac_i2c->wac_pdata->max_y;
	wac_i2c->wac_feature->pressure_max = wac_i2c->wac_pdata->max_pressure;

	if (wac_i2c->wac_pdata->xy_switch) {
		input_set_abs_params(wac_i2c->input_dev, ABS_X, 0,
			wac_i2c->wac_feature->y_max, 4, 0);
		input_set_abs_params(wac_i2c->input_dev, ABS_Y, 0,
			wac_i2c->wac_feature->x_max, 4, 0);
	} else {
		input_set_abs_params(wac_i2c->input_dev, ABS_X, 0,
			wac_i2c->wac_feature->x_max, 4, 0);
		input_set_abs_params(wac_i2c->input_dev, ABS_Y, 0,
			wac_i2c->wac_feature->y_max, 4, 0);
	}

	input_set_abs_params(wac_i2c->input_dev, ABS_PRESSURE, 0,
		wac_i2c->wac_feature->pressure_max, 0, 0);
#ifdef WACOM_USE_GAIN
	input_set_abs_params(wac_i2c->input_dev, ABS_DISTANCE, 0,
		1024, 0, 0);
#endif
}

static void wacom_request_gpio(struct wacom_g5_platform_data *pdata)
{
	int ret;
	printk(KERN_INFO "%s: request gpio\n", __func__);

	ret = gpio_request(pdata->gpio_int, "wacom_irq");
	if (ret) {
		pr_err("%s: unable to request wacom_irq [%d]\n",
				__func__, pdata->gpio_int);
		return;
	}

	ret = gpio_request(pdata->vdd_en, "wacom_vdd_en");
	if (ret) {
		pr_err("%s: unable to request wacom_vdd_en [%d]\n",
				__func__, pdata->vdd_en);
		return;
	}

	if (pdata->gpio_pen_reset_n > 0) {
		ret = gpio_request(pdata->gpio_pen_reset_n, "wacom_pen_reset_n");
		if (ret) {
			pr_err("%s: unable to request wacom_pen_reset_n [%d]\n",
				__func__, pdata->gpio_pen_reset_n);
			return;
		}

		gpio_tlmm_config(GPIO_CFG(pdata->gpio_pen_reset_n, 0,
			GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	}

	ret = gpio_request(pdata->gpio_pen_pdct, "pen_pdct-gpio");
	if (ret) {
		pr_err("%s: unable to request pen_pdct-gpio [%d]\n",
				__func__, pdata->gpio_pen_pdct);
		return;
	}

	ret = gpio_request(pdata->gpio_pen_fwe1, "wacom_pen_fwe1");
	if (ret) {
		pr_err("%s: unable to request wacom_pen_fwe1 [%d]\n",
				__func__, pdata->gpio_pen_fwe1);
		return;
	}
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_pen_fwe1, 0,
		GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_direction_output(pdata->gpio_pen_fwe1, 0);

	ret = gpio_request(pdata->gpio_pen_insert, "wacom_pen_insert");
	if (ret) {
		pr_err("[WACOM]%s: unable to request wacom_pen_insert [%d]\n",
				__func__, pdata->gpio_pen_insert);
		return;
	}
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_pen_insert, 0,
		GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);

}

#ifdef CONFIG_OF
static int wacom_get_dt_coords(struct device *dev, char *name,
				struct wacom_g5_platform_data *pdata)
{
	u32 coords[WACOM_COORDS_ARR_SIZE];
	struct property *prop;
	struct device_node *np = dev->of_node;
	int coords_size, rc;

	prop = of_find_property(np, name, NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;

	coords_size = prop->length / sizeof(u32);
	if (coords_size != WACOM_COORDS_ARR_SIZE) {
		dev_err(dev, "invalid %s\n", name);
		return -EINVAL;
	}

	rc = of_property_read_u32_array(np, name, coords, coords_size);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "%s: Unable to read %s\n", __func__, name);
		return rc;
	}

	if (strncmp(name, "wacom,panel-coords",
			sizeof("wacom,panel-coords")) == 0) {
		pdata->x_invert = coords[0];
		pdata->y_invert = coords[1];
		pdata->min_x = coords[2];
		pdata->max_x = coords[3];
		pdata->min_y = coords[4];
		pdata->max_y = coords[5];
		pdata->xy_switch = coords[6];
		pdata->min_pressure = coords[7];
		pdata->max_pressure = coords[8];

		printk(KERN_ERR "%s: x_invert = %d, y_invert = %d, xy_switch = %d\n",
				__func__, pdata->x_invert, pdata->y_invert, pdata->xy_switch);
	} else {
		dev_err(dev, "%s: nsupported property %s\n", __func__, name);
		return -EINVAL;
	}

	return 0;
}

static void wacom_connect_platform_data(struct wacom_g5_platform_data *pdata)
{
	pdata->reset_platform_hw = wacom_reset_hw;
	pdata->wacom_start = wacom_start;
	pdata->wacom_stop = wacom_stop;
#ifdef WACOM_HAVE_FWE_PIN
	pdata->compulsory_flash_mode = wacom_compulsory_flash_mode;
#endif
}

static int wacom_parse_dt(struct device *dev,
			struct wacom_g5_platform_data *pdata)
{
	int rc;
	struct device_node *np = dev->of_node;

	rc = wacom_get_dt_coords(dev, "wacom,panel-coords", pdata);
	if (rc)
		return rc;

	/* regulator info */
	pdata->i2c_pull_up = of_property_read_bool(np, "wacom,i2c-pull-up");
	pdata->vdd_en = of_get_named_gpio(np, "vdd_en-gpio", 0);

	/* reset, irq gpio info */
	pdata->gpio_int = of_get_named_gpio_flags(np, "wacom,irq-gpio",
				0, &pdata->irq_gpio_flags);
	pdata->gpio_pen_fwe1 = of_get_named_gpio_flags(np,
		"wacom,pen_fwe1-gpio", 0, &pdata->pen_fwe1_gpio_flags);
	pdata->gpio_pen_reset_n = of_get_named_gpio_flags(np,
		"wacom,reset_n-gpio", 0, &pdata->pen_reset_n_gpio_flags);
	pdata->gpio_pen_pdct = of_get_named_gpio_flags(np,
		"wacom,pen_pdct-gpio", 0, &pdata->pen_pdct_gpio_flags);
	pdata->gpio_pen_insert = of_get_named_gpio(np, "wacom,sense-gpio", 0);

#if defined(CONFIG_MACH_HLTECHNTWU)
	pdata->basic_model = "N900U";
#else
	rc = of_property_read_string(np, "wacom,basic_model", &pdata->basic_model);
	if (rc < 0) {
		dev_info(dev, "%s: Unable to read wacom,basic_model\n", __func__);
		pdata->basic_model = "NULL";
	}
#endif

	rc = of_property_read_u32(np, "wacom,ic_mpu_ver", &pdata->ic_mpu_ver);
	if (rc < 0)
		dev_info(dev, "%s: Unable to read wacom,ic_mpu_ver\n", __func__);

	/*Change below if irq is needed */
	rc = of_property_read_u32(np, "wacom,irq_flags", &pdata->irq_flags);
	if (rc < 0)
		dev_info(dev, "%s: Unable to read wacom,irq_flags\n", __func__);

	printk(KERN_ERR "%s: fwe1: %d, reset_n: %d, pdct: %d, insert: %d, model: %s, mpu: %x, irq_flags=%x \n",
			__func__, pdata->gpio_pen_fwe1, pdata->gpio_pen_reset_n,
			pdata->gpio_pen_pdct, pdata->gpio_pen_insert, pdata->basic_model, pdata->ic_mpu_ver, pdata->irq_flags);

	return 0;
}
#else
static int wacom_parse_dt(struct device *dev,
			struct wacom_g5_platform_data *pdata)
{
	return -ENODEV;
}
#endif

static int wacom_i2c_remove(struct i2c_client *client)
{
	struct wacom_i2c *wac_i2c = i2c_get_clientdata(client);

	free_irq(client->irq, wac_i2c);
#ifdef WACOM_PDCT_WORK_AROUND	
	free_irq(wac_i2c->irq_pdct, wac_i2c);
#endif	
	free_irq(wac_i2c->irq_pen_insert, wac_i2c);

	cancel_delayed_work_sync(&wac_i2c->resume_work);
	cancel_delayed_work_sync(&wac_i2c->touch_pressed_work);
#ifdef USE_WACOM_LCD_WORKAROUND
	cancel_delayed_work_sync(&wac_i2c->read_vsync_work);
	cancel_delayed_work_sync(&wac_i2c->boot_done_work);
#endif

#ifdef WACOM_RESETPIN_DELAY
	cancel_delayed_work_sync(&wac_i2c->work_wacom_reset);
#endif

	cancel_delayed_work_sync(&wac_i2c->pen_insert_dwork);
#ifdef WACOM_BOOSTER
	cancel_delayed_work_sync(&wac_i2c->work_dvfs_off);
	cancel_delayed_work_sync(&wac_i2c->work_dvfs_chg);

	mutex_destroy(&wac_i2c->dvfs_lock);
#endif
	mutex_destroy(&wac_i2c->lock);

	sysfs_remove_group(&wac_i2c->dev->kobj, &epen_attr_group);

	input_unregister_device(wac_i2c->input_dev);
	input_free_device(wac_i2c->input_dev);

	kfree(wac_i2c);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void wacom_i2c_early_suspend(struct early_suspend *h)
{
	struct wacom_i2c *wac_i2c =
	    container_of(h, struct wacom_i2c, early_suspend);

	dev_info(&wac_i2c->client->dev,
			"%s\n", __func__);

	wacom_i2c_disable(wac_i2c);
}

static void wacom_i2c_late_resume(struct early_suspend *h)
{
	struct wacom_i2c *wac_i2c =
	    container_of(h, struct wacom_i2c, early_suspend);

	dev_info(&wac_i2c->client->dev,
			"%s\n", __func__);

	wacom_i2c_enable(wac_i2c);
}
#endif

static int wacom_i2c_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct wacom_g5_platform_data *pdata;
	struct wacom_i2c *wac_i2c;
	struct input_dev *input;
	int ret = 0;
	int error;
	int fw_ver;

	/*Check I2C functionality */
	ret = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if (!ret) {
		printk(KERN_ERR "%s: No I2C functionality found\n", __func__);
		ret = -ENODEV;
		goto err_i2c_fail;
	}

	/*Obtain kernel memory space for wacom i2c */
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct wacom_g5_platform_data), GFP_KERNEL);
		if (!pdata) {
				dev_err(&client->dev,
						"%s: Failed to allocate memory\n",
						__func__);
			return -ENOMEM;
		}
		error = wacom_parse_dt(&client->dev, pdata);
		if (error)
			return error;
	} else {
		pdata = client->dev.platform_data;
		if (pdata == NULL) {
			dev_err(&client->dev, "%s: no pdata\n", __func__);
			ret = -ENODEV;
			goto err_i2c_fail;
		}
	}

	wacom_connect_platform_data(pdata);
	wacom_request_gpio(pdata);

	wac_i2c = kzalloc(sizeof(struct wacom_i2c), GFP_KERNEL);
	if (NULL == wac_i2c) {
		dev_err(&client->dev,
				"%s: failed to allocate wac_i2c.\n",
				__func__);
		ret = -ENOMEM;
		goto err_i2c_fail;
	}

	wac_i2c->client_boot = i2c_new_dummy(client->adapter,
		WACOM_I2C_BOOT);
	if (!wac_i2c->client_boot) {
		dev_err(&client->dev, "Fail to register sub client[0x%x]\n",
			 WACOM_I2C_BOOT);
	}

	input = input_allocate_device();
	if (NULL == input) {
		dev_err(&client->dev,
				"%s: failed to allocate input device.\n",
				__func__);
		ret = -ENOMEM;
		goto err_freemem;
	}

	wacom_i2c_set_input_values(client, wac_i2c, input);

	wac_i2c->wac_feature = &wacom_feature_EMR;
	wac_i2c->wac_pdata = pdata;
	wac_i2c->input_dev = input;
	wac_i2c->client = client;

	client->irq = gpio_to_irq(pdata->gpio_int);
	printk(KERN_ERR "%s: wacom : gpio_to_irq : %d\n",
				__func__, client->irq);
	wac_i2c->irq = client->irq;

	/*Set client data */
	i2c_set_clientdata(client, wac_i2c);
	i2c_set_clientdata(wac_i2c->client_boot, wac_i2c);

#ifdef WACOM_PDCT_WORK_AROUND
	wac_i2c->irq_pdct = gpio_to_irq(pdata->gpio_pen_pdct);
	wac_i2c->pen_pdct = PDCT_NOSIGNAL;
#endif
#ifdef WACOM_PEN_DETECT
	wac_i2c->gpio_pen_insert = pdata->gpio_pen_insert;
	wac_i2c->irq_pen_insert = gpio_to_irq(wac_i2c->gpio_pen_insert);
#endif
#ifdef WACOM_IMPORT_FW_ALGO
	wac_i2c->use_offset_table = true;
	wac_i2c->use_aveTransition = false;
#endif

#ifdef USE_WACOM_CALLBACK
	/*Register callbacks */
	wac_i2c->callbacks.check_prox = wacom_check_emr_prox;
	if (wac_i2c->wac_pdata->register_cb)
		wac_i2c->wac_pdata->register_cb(&wac_i2c->callbacks);
#endif

#ifdef CONFIG_SEC_VIENNA_PROJECT
	if (system_rev >= WACOM_BOOT_REVISION) {
		wac_i2c->wac_pdata->ic_mpu_ver = MPU_W9007;
		wac_i2c->boot_ver = 0x92;
	}
#endif

#ifdef CONFIG_SEC_LT03_PROJECT
		wac_i2c->boot_ver = 0x92;
#endif

	if (wac_i2c->wac_pdata->ic_mpu_ver > 0) {
		wac_i2c->ic_mpu_ver = wac_i2c->wac_pdata->ic_mpu_ver;

		wac_i2c->wac_pdata->compulsory_flash_mode(wac_i2c, false);
		wac_i2c->wac_pdata->wacom_start(wac_i2c);
		msleep(200);
	} else {
		wac_i2c->wac_pdata->compulsory_flash_mode(wac_i2c, true);
		/*Reset */
		wac_i2c->wac_pdata->wacom_start(wac_i2c);
		msleep(200);
		ret = wacom_check_mpu_version(wac_i2c);
		if (ret == -ETIMEDOUT)
#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_FRESCO_PROJECT)
			goto err_wacom_i2c_send_timeout;
#else
			pr_err("[E-PEN] wacom_i2c_send failed.\n");
#endif
		wac_i2c->ic_mpu_ver = wacom_check_flash_mode(wac_i2c, BOOT_MPU);
		dev_info(&wac_i2c->client->dev,
			"%s: mpu version: %x\n", __func__, ret);

		if (wac_i2c->ic_mpu_ver == MPU_W9001)
			wac_i2c->client_boot = i2c_new_dummy(client->adapter,
				WACOM_I2C_9001_BOOT);
		else if (wac_i2c->ic_mpu_ver == MPU_W9007) {
				ret = wacom_enter_bootloader(wac_i2c);
				if (ret < 0) {
					dev_info(&wac_i2c->client->dev,
						"%s: failed to get BootLoader version, %d\n", __func__, ret);
					goto err_wacom_i2c_bootloader_ver;
			} else {
				dev_info(&wac_i2c->client->dev,
					"%s: BootLoader version: %x\n", __func__, ret);
			}
		}

		wac_i2c->wac_pdata->compulsory_flash_mode(wac_i2c, false);
		wac_i2c->wac_pdata->reset_platform_hw(wac_i2c);
		wac_i2c->power_enable = true;
	}

	/* Firmware Feature */
	wacom_i2c_init_firm_data();
	pr_err("%s: wacon ic turn on\n", __func__);

	fw_ver = wacom_i2c_query(wac_i2c);

	wacom_init_abs_params(wac_i2c);
	input_set_drvdata(input, wac_i2c);

	/*Initializing for semaphor */
	mutex_init(&wac_i2c->lock);
#if defined(CONFIG_SEC_LT03_PROJECT) || defined(CONFIG_SEC_VIENNA_PROJECT)
	mutex_init(&wac_i2c->irq_lock);
#endif

#ifdef BATTERY_SAVING_MODE
	wac_i2c->battery_saving_mode = true;
#endif

#ifdef WACOM_BOOSTER
	wacom_init_dvfs(wac_i2c);
#endif
	INIT_DELAYED_WORK(&wac_i2c->resume_work, wacom_i2c_resume_work);

#ifdef USE_WACOM_BLOCK_KEYEVENT
	INIT_DELAYED_WORK(&wac_i2c->touch_pressed_work, wacom_i2c_touch_pressed_work);
	wac_i2c->key_delay_time = 100;
#endif

#ifdef USE_WACOM_LCD_WORKAROUND
	wac_i2c->wait_done = true;
	wac_i2c->delay_time = 5;
	INIT_DELAYED_WORK(&wac_i2c->read_vsync_work, wacom_i2c_read_vsync_work);

	wac_i2c->boot_done = false;

	INIT_DELAYED_WORK(&wac_i2c->boot_done_work, wacom_i2c_boot_done_work);
#endif
#ifdef WACOM_PEN_DETECT
	INIT_DELAYED_WORK(&wac_i2c->pen_insert_dwork, pen_insert_work);
#endif

#ifdef WACOM_RESETPIN_DELAY
	INIT_DELAYED_WORK(&wac_i2c->work_wacom_reset, wacom_reset);
#endif

	/*Before registering input device, data in each input_dev must be set */
	ret = input_register_device(input);
	if (ret) {
		pr_err("[E-PEN] failed to register input device.\n");
		goto err_input_allocate_device;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	wac_i2c->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	wac_i2c->early_suspend.suspend = wacom_i2c_early_suspend;
	wac_i2c->early_suspend.resume = wacom_i2c_late_resume;
	register_early_suspend(&wac_i2c->early_suspend);
#endif

	wac_i2c->dev = device_create(sec_class, NULL, 0, NULL, "sec_epen");
	if (IS_ERR(wac_i2c->dev)) {
		dev_err(&wac_i2c->client->dev,
				"%s: Failed to create device(wac_i2c->dev)!\n",
				__func__);
		goto err_sysfs_create_group;
	}

	dev_set_drvdata(wac_i2c->dev, wac_i2c);

	ret = sysfs_create_group(&wac_i2c->dev->kobj, &epen_attr_group);
	if (ret) {
		dev_err(&wac_i2c->client->dev,
				"%s: failed to create sysfs group\n",
				__func__);
		goto err_sysfs_create_group;
	}
	ret = sysfs_create_link(&wac_i2c->dev->kobj, &wac_i2c->input_dev->dev.kobj, "input");
	if (ret < 0) {
		dev_err(&client->dev,
				"%s: Failed to create input symbolic link\n",
				__func__);
	}
	ret = wacom_firmware_update(wac_i2c);
	if (ret) {
		dev_err(&wac_i2c->client->dev,
				"%s: firmware update failed.\n",
				__func__);
		if (fw_ver > 0 && wac_i2c->ic_mpu_ver < 0)
			dev_err(&wac_i2c->client->dev,
					"%s: read query but not enter boot mode[%x,%x]\n",
					__func__, fw_ver, wac_i2c->ic_mpu_ver);
		else
			goto err_fw_update;
	}

	/*Request IRQ */
	if (pdata->irq_flags) {
		ret =
		    request_threaded_irq(wac_i2c->irq, NULL, wacom_interrupt,
					 IRQF_DISABLED | pdata->irq_flags |
					 IRQF_ONESHOT, wac_i2c->name, wac_i2c);
		if (ret < 0) {
			dev_err(&wac_i2c->client->dev,
					"%s: failed to request irq(%d) - %d\n",
					__func__, wac_i2c->irq, ret);
			goto err_fw_update;
		}

#if defined(WACOM_PDCT_WORK_AROUND)
		ret = request_threaded_irq(wac_i2c->irq_pdct, NULL,
					wacom_interrupt_pdct,
					IRQF_DISABLED | IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					wac_i2c->name, wac_i2c);
		if (ret < 0) {
		dev_err(&wac_i2c->client->dev,
				"%s: failed to request irq(%d) - %d\n",
				__func__, wac_i2c->irq_pdct, ret);
			goto err_request_irq_pdct;
		}
#endif

#ifdef WACOM_PEN_DETECT
		ret = request_threaded_irq(
					wac_i2c->irq_pen_insert, NULL,
					wacom_pen_detect,
					IRQF_DISABLED | IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					"pen_insert", wac_i2c);
		if (ret < 0) {
			dev_err(&wac_i2c->client->dev,
					"%s: failed to request irq(%d) - %d\n",
					__func__, wac_i2c->irq_pen_insert, ret);
			goto err_request_irq_pen_inster;
		}
		enable_irq_wake(wac_i2c->irq_pen_insert);

		/* update the current status */
		schedule_delayed_work(&wac_i2c->pen_insert_dwork, HZ / 2);
#endif

	}

#ifdef USE_WACOM_LCD_WORKAROUND
	schedule_delayed_work(&wac_i2c->boot_done_work,
					msecs_to_jiffies(20 * 1000));
#endif

#ifdef WACOM_RESETPIN_DELAY
	schedule_delayed_work(&wac_i2c->work_wacom_reset, msecs_to_jiffies(5000));
#endif

	return 0;

err_request_irq_pen_inster:
#ifdef WACOM_PDCT_WORK_AROUND
	free_irq(wac_i2c->irq_pdct, wac_i2c);
err_request_irq_pdct:
#endif
	free_irq(wac_i2c->irq, wac_i2c);
err_fw_update:
	sysfs_remove_group(&wac_i2c->dev->kobj, &epen_attr_group);
err_sysfs_create_group:
	wac_i2c->init_fail = true;
	input_unregister_device(input);
err_input_allocate_device:
	cancel_delayed_work_sync(&wac_i2c->resume_work);
	cancel_delayed_work_sync(&wac_i2c->touch_pressed_work);
#ifdef USE_WACOM_LCD_WORKAROUND
	cancel_delayed_work_sync(&wac_i2c->read_vsync_work);
	cancel_delayed_work_sync(&wac_i2c->boot_done_work);
#endif
	cancel_delayed_work_sync(&wac_i2c->pen_insert_dwork);
#ifdef WACOM_RESETPIN_DELAY
	cancel_delayed_work_sync(&wac_i2c->work_wacom_reset);
#endif
#ifdef WACOM_BOOSTER
	cancel_delayed_work_sync(&wac_i2c->work_dvfs_off);
	cancel_delayed_work_sync(&wac_i2c->work_dvfs_chg);
	mutex_destroy(&wac_i2c->dvfs_lock);
#endif
	wac_i2c->wac_pdata->wacom_stop(wac_i2c);
	mutex_destroy(&wac_i2c->lock);
err_wacom_i2c_bootloader_ver:
#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_FRESCO_PROJECT)
 err_wacom_i2c_send_timeout:
#endif
	input_free_device(input);
 err_freemem:
	kfree(wac_i2c);
 err_i2c_fail:
	return ret;
}

static const struct i2c_device_id wacom_i2c_id[] = {
	{"wacom_g5sp_i2c", 0},
	{},
};

#ifdef CONFIG_OF
static struct of_device_id wacom_match_table[] = {
	{ .compatible = "wacom,wacom_i2c-ts",},
	{ },
};
#else
#define wacom_match_table	NULL
#endif
/*Create handler for wacom_i2c_driver*/
static struct i2c_driver wacom_i2c_driver = {
	.driver = {
		   .name = "wacom_g5sp_i2c",
#ifdef CONFIG_OF
		   .of_match_table = wacom_match_table,
#endif
		   },
	.probe = wacom_i2c_probe,
	.remove = wacom_i2c_remove,
	.id_table = wacom_i2c_id,
};

/*
module_i2c_driver(wacom_i2c_driver);
*/
static int __init wacom_i2c_init(void)
{
	int ret = 0;
/*
#if defined(WACOM_SLEEP_WITH_PEN_SLP)
	printk(KERN_ERR "[E-PEN] %s: Sleep type-PEN_SLP pin\n", __func__);
#elif defined(WACOM_SLEEP_WITH_PEN_LDO_EN)
	printk(KERN_ERR "[E-PEN] %s: Sleep type-PEN_LDO_EN pin\n", __func__);
#endif
*/
#ifdef CONFIG_SAMSUNG_LPM_MODE
	if (poweroff_charging) {
		pr_notice("%s : LPM Charging Mode!!\n", __func__);
		return 0;
	}
#endif
	ret = i2c_add_driver(&wacom_i2c_driver);
	if (ret)
		printk(KERN_ERR "[E-PEN] fail to i2c_add_driver\n");
	return ret;
}

static void __exit wacom_i2c_exit(void)
{
	i2c_del_driver(&wacom_i2c_driver);
}

module_init(wacom_i2c_init);
module_exit(wacom_i2c_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Driver for Wacom G5SP Digitizer Controller");

MODULE_LICENSE("GPL");
