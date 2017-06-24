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
#include "wacom.h"
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include "wacom_i2c_func.h"
#include "wacom_i2c_firm.h"
#ifdef WACOM_IMPORT_FW_ALGO
#include "wacom_i2c_coord_tables.h"
#endif

#ifdef CONFIG_OF
#define WACOM_OPEN_CLOSE
#undef CONFIG_PM
#undef CONFIG_HAS_EARLYSUSPEND
#endif

#define WACOM_FW_PATH "/sdcard/firmware/wacom_firm.bin"

static struct wacom_features wacom_feature_EMR = {
	.x_max = WACOM_MAX_COORD_X,
	.y_max = WACOM_MAX_COORD_Y,
	.pressure_max = WACOM_MAX_PRESSURE,
	.comstat = COM_QUERY,
	.data = {0, 0, 0, 0, 0, 0, 0},
	.fw_version = 0x0,
	.firm_update_status = 0,
};

unsigned char screen_rotate;
unsigned char user_hand = 1;
extern unsigned int system_rev;

extern int wacom_i2c_flash(struct wacom_i2c *wac_i2c);

static void wacom_enable_irq(struct wacom_i2c *wac_i2c, bool enable)
{
	static int depth;

	mutex_lock(&wac_i2c->irq_lock);
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
	mutex_unlock(&wac_i2c->irq_lock);

#ifdef WACOM_IRQ_DEBUG
	printk(KERN_DEBUG"epen:Enable %d, depth %d\n", (int)enable, depth);
#endif
}

static void wacom_i2c_reset_hw(struct wacom_g5_platform_data *wac_pdata)
{
	/* Reset IC */
	wac_pdata->suspend_platform_hw();
	msleep(50);
	wac_pdata->resume_platform_hw();
	msleep(200);
}

static void wacom_power_on(struct wacom_i2c *wac_i2c)
{
	mutex_lock(&wac_i2c->lock);

	if (wac_i2c->power_enable) {
		printk(KERN_DEBUG"epen:pass pwr on\n");
		goto out_power_on;
	}

#ifdef BATTERY_SAVING_MODE
	if (wac_i2c->battery_saving_mode
		&& wac_i2c->pen_insert){
		printk(KERN_DEBUG"epen:pass battery save mode\n");
		goto out_power_on;
	}
#endif

	if (wake_lock_active(&wac_i2c->fw_wakelock)) {
		printk(KERN_DEBUG"epen:wake_lock active. pass pwr on\n");
		goto out_power_on;
	}

	/* power on */
	wac_i2c->wac_pdata->resume_platform_hw();
	wac_i2c->power_enable = true;
	cancel_delayed_work_sync(&wac_i2c->resume_work);
	schedule_delayed_work(&wac_i2c->resume_work, msecs_to_jiffies(EPEN_RESUME_DELAY));

	printk(KERN_DEBUG"epen:%s\n", __func__);
 out_power_on:
	mutex_unlock(&wac_i2c->lock);
}

static void wacom_power_off(struct wacom_i2c *wac_i2c)
{
	mutex_lock(&wac_i2c->lock);

	if (!wac_i2c->power_enable) {
		printk(KERN_DEBUG"epen:pass pwr off\n");
		goto out_power_off;
	}

#ifdef WACOM_BOOSTER
	wacom_set_dvfs_lock(wac_i2c, 2);
#endif
	wacom_enable_irq(wac_i2c, false);

	/* release pen, if it is pressed */
	if (wac_i2c->pen_pressed || wac_i2c->side_pressed
		|| wac_i2c->pen_prox)
		forced_release(wac_i2c);

	cancel_delayed_work_sync(&wac_i2c->resume_work);

#ifdef LCD_FREQ_SYNC
	cancel_work_sync(&wac_i2c->lcd_freq_work);
	cancel_delayed_work_sync(&wac_i2c->lcd_freq_done_work);
	wac_i2c->lcd_freq_wait = false;
#endif
#ifdef WACOM_USE_SOFTKEY_BLOCK
	cancel_delayed_work_sync(&wac_i2c->softkey_block_work);
	wac_i2c->block_softkey = false;
#endif

	if (wake_lock_active(&wac_i2c->fw_wakelock)) {
		printk(KERN_DEBUG"epen:wake_lock active. pass pwr off\n");
		goto out_power_off;
	}

	/* power off */
	wac_i2c->power_enable = false;
	wac_i2c->wac_pdata->suspend_platform_hw();

	printk(KERN_DEBUG"epen:%s\n", __func__);
 out_power_off:
	mutex_unlock(&wac_i2c->lock);
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

	wac_i2c->pen_pdct = gpio_get_value(wac_i2c->wac_pdata->gpio_pendct);

	printk(KERN_DEBUG "epen:pdct %d(%d)\n",
		wac_i2c->pen_pdct, wac_i2c->pen_prox);
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

	wac_i2c->pen_insert = !gpio_get_value(wac_i2c->gpio_pen_insert);

	printk(KERN_DEBUG "epen:%s : %d\n",
		__func__, wac_i2c->pen_insert);

	input_report_switch(wac_i2c->input_dev,
		SW_PEN_INSERT, !wac_i2c->pen_insert);
	input_sync(wac_i2c->input_dev);

#ifdef BATTERY_SAVING_MODE
	if (wac_i2c->pen_insert) {
		if (wac_i2c->battery_saving_mode)
			wacom_power_off(wac_i2c);
	} else
		wacom_power_on(wac_i2c);
#endif
}
static irqreturn_t wacom_pen_detect(int irq, void *dev_id)
{
	struct wacom_i2c *wac_i2c = dev_id;

	cancel_delayed_work_sync(&wac_i2c->pen_insert_dwork);
	schedule_delayed_work(&wac_i2c->pen_insert_dwork, HZ / 20);
	return IRQ_HANDLED;
}

static int init_pen_insert(struct wacom_i2c *wac_i2c)
{
	int ret = 0;
	int irq = gpio_to_irq(wac_i2c->gpio_pen_insert);

	INIT_DELAYED_WORK(&wac_i2c->pen_insert_dwork, pen_insert_work);

	ret =
		request_threaded_irq(
			irq, NULL,
			wacom_pen_detect,
			IRQF_DISABLED | IRQF_TRIGGER_RISING |
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			"pen_insert", wac_i2c);
	if (ret < 0)
		printk(KERN_ERR
			"epen:failed to request pen insert irq\n");

	enable_irq_wake(irq);

	/* update the current status */
	schedule_delayed_work(&wac_i2c->pen_insert_dwork, HZ / 2);

	return 0;
}

#endif

static int wacom_i2c_input_open(struct input_dev *dev)
{
	struct wacom_i2c *wac_i2c = input_get_drvdata(dev);
	int ret = 0;

	dev_info(&wac_i2c->client->dev, "%s\n", __func__);

#if 0
	ret = wait_for_completion_interruptible_timeout(&wac_i2c->init_done,
		msecs_to_jiffies(1 * MSEC_PER_SEC));

	if (ret < 0) {
		dev_err(&wac_i2c->client->dev,
			"error while waiting for device to init (%d)\n", ret);
		ret = -ENXIO;
		goto err_open;
	}
	if (ret == 0) {
		dev_err(&wac_i2c->client->dev,
			"timedout while waiting for device to init\n");
		ret = -ENXIO;
		goto err_open;
	}
#endif

	wacom_power_on(wac_i2c);
	wac_i2c->enabled = true;

	return ret;
}

static void wacom_i2c_input_close(struct input_dev *dev)
{
	struct wacom_i2c *wac_i2c = input_get_drvdata(dev);

	dev_info(&wac_i2c->client->dev, "%s\n", __func__);

	wacom_power_off(wac_i2c);
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

	input_dev->evbit[0] |= BIT_MASK(EV_SW);
	input_set_capability(input_dev, EV_SW, SW_PEN_INSERT);

	input_dev->open = wacom_i2c_input_open;
	input_dev->close = wacom_i2c_input_close;

	__set_bit(ABS_X, input_dev->absbit);
	__set_bit(ABS_Y, input_dev->absbit);
	__set_bit(ABS_PRESSURE, input_dev->absbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(BTN_TOOL_PEN, input_dev->keybit);
	__set_bit(BTN_TOOL_RUBBER, input_dev->keybit);
	__set_bit(BTN_STYLUS, input_dev->keybit);
	__set_bit(KEY_UNKNOWN, input_dev->keybit);
	__set_bit(KEY_PEN_PDCT, input_dev->keybit);
#ifdef CONFIG_INPUT_BOOSTER
	__set_bit(KEY_BOOSTER_PEN, input_dev->keybit);
#endif
#ifdef WACOM_USE_GAIN
	__set_bit(ABS_DISTANCE, input_dev->absbit);
#endif

	/*  __set_bit(BTN_STYLUS2, input_dev->keybit); */
	/*  __set_bit(ABS_MISC, input_dev->absbit); */

	/*softkey*/
#ifdef WACOM_USE_SOFTKEY
	__set_bit(KEY_MENU, input_dev->keybit);
	__set_bit(KEY_BACK, input_dev->keybit);
#endif
}

static int wacom_check_emr_prox(struct wacom_g5_callbacks *cb)
{
	struct wacom_i2c *wac = container_of(cb, struct wacom_i2c, callbacks);
	printk(KERN_DEBUG "epen:%s:\n", __func__);

	return wac->pen_prox;
}

static int wacom_i2c_remove(struct i2c_client *client)
{
	struct wacom_i2c *wac_i2c = i2c_get_clientdata(client);
	free_irq(client->irq, wac_i2c);
	input_unregister_device(wac_i2c->input_dev);
	kfree(wac_i2c);

	return 0;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void wacom_i2c_early_suspend(struct early_suspend *h)
{
	struct wacom_i2c *wac_i2c =
	    container_of(h, struct wacom_i2c, early_suspend);
	printk(KERN_DEBUG "epen:%s.\n", __func__);
	wacom_power_off(wac_i2c);
}

static void wacom_i2c_late_resume(struct early_suspend *h)
{
	struct wacom_i2c *wac_i2c =
		container_of(h, struct wacom_i2c, early_suspend);

	printk(KERN_DEBUG "epen:%s.\n", __func__);
	wacom_power_on(wac_i2c);
}
#endif

static void wacom_i2c_resume_work(struct work_struct *work)
{
struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, resume_work.work);
	u8 irq_state = 0;
	int ret = 0;

	irq_state = wac_i2c->wac_pdata->get_irq_state();
	wacom_enable_irq(wac_i2c, true);
	
	if (unlikely(irq_state)) {
		printk(KERN_DEBUG"epen:irq was enabled\n");
		ret = wacom_i2c_recv(wac_i2c, wac_i2c->wac_feature->data, COM_COORD_NUM, false);
		if (ret < 0) {
			printk(KERN_ERR "epen:%s failed to read i2c.L%d\n", __func__,
			__LINE__);
		}
	}

	printk(KERN_DEBUG "epen:%s\n", __func__);
}

#ifdef LCD_FREQ_SYNC
#define SYSFS_WRITE_LCD	"/sys/class/lcd/panel/ldi_fps"
static void wacom_i2c_sync_lcd_freq(struct wacom_i2c *wac_i2c)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *write_node;
	char freq[12] = {0, };
	int lcd_freq = wac_i2c->lcd_freq;

	mutex_lock(&wac_i2c->freq_write_lock);

	sprintf(freq, "%d", lcd_freq);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	write_node = filp_open(SYSFS_WRITE_LCD, /*O_RDONLY*/O_RDWR | O_SYNC, 0664);
	if (IS_ERR(write_node)) {
		ret = PTR_ERR(write_node);
		dev_err(&wac_i2c->client->dev,
			"%s: node file open fail, %d\n", __func__, ret);
		goto err_open_node;
	}

	ret = write_node->f_op->write(write_node, (char __user *)freq, strlen(freq), &write_node->f_pos);
	if (ret != strlen(freq)) {
		dev_err(&wac_i2c->client->dev,
			"%s: Can't write node data\n", __func__);
	}
	printk(KERN_DEBUG"epen:%s write freq %s\n", __func__, freq);

	filp_close(write_node, current->files);

err_open_node:
	set_fs(old_fs);
err_read_framerate:
	mutex_unlock(&wac_i2c->freq_write_lock);

	schedule_delayed_work(&wac_i2c->lcd_freq_done_work, HZ * 5);
}

static void wacom_i2c_finish_lcd_freq_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
		container_of(work, struct wacom_i2c, lcd_freq_done_work.work);
	
	wac_i2c->lcd_freq_wait = false;

	printk(KERN_DEBUG"epen:%s\n", __func__);
}

static void wacom_i2c_lcd_freq_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
		container_of(work, struct wacom_i2c, lcd_freq_work);

	wacom_i2c_sync_lcd_freq(wac_i2c);
}
#endif

#ifdef WACOM_USE_SOFTKEY_BLOCK
static void wacom_i2c_block_softkey_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
		container_of(work, struct wacom_i2c, softkey_block_work.work);

	wac_i2c->block_softkey = false;
}
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
#define wacom_i2c_suspend	NULL
#define wacom_i2c_resume	NULL

#else
static int wacom_i2c_suspend(struct device *dev)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	wac_i2c->pwr_flag = false;
	if (wac_i2c->input_dev->users)
		wacom_power_off(wac_i2c);

	return 0;
}

static int wacom_i2c_resume(struct device *dev)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	wac_i2c->pwr_flag = true;
	if (wac_i2c->input_dev->users)
		wacom_power_on(wac_i2c);

	return 0;
}
static SIMPLE_DEV_PM_OPS(wacom_pm_ops, wacom_i2c_suspend, wacom_i2c_resume);
#endif

static ssize_t epen_firm_update_status_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	printk(KERN_DEBUG "epen:%s:(%d)\n", __func__,
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
	printk(KERN_DEBUG "epen:%s: 0x%x|0x%X\n", __func__,
	       wac_i2c->wac_feature->fw_version, fw_ver_file);

	return sprintf(buf, "%04X\t%04X\n",
			wac_i2c->wac_feature->fw_version,
			fw_ver_file);
}

#if defined(WACOM_IMPORT_FW_ALGO)
static ssize_t epen_tuning_version_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	printk(KERN_DEBUG "epen:%s: %s\n", __func__,
			tuning_version);

	return sprintf(buf, "%s_%s\n",
			tuning_model,
			tuning_version);
}

static ssize_t epen_rotation_store(struct device *dev,
struct device_attribute *attr,
	const char *buf, size_t count)
{
	static bool factory_test;
	static unsigned char last_rotation;
	unsigned int val;

	sscanf(buf, "%u", &val);

	/* Fix the rotation value to 0(Portrait) when factory test(15 mode) */
	if (val == 100 && !factory_test) {
		factory_test = true;
		screen_rotate = 0;
		printk(KERN_DEBUG"epen:%s, enter factory test mode\n",
			__func__);
	} else if (val == 200 && factory_test) {
		factory_test = false;
		screen_rotate = last_rotation;
		printk(KERN_DEBUG"epen:%s, exit factory test mode\n",
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
	printk(KERN_DEBUG"epen:%s: rotate=%d\n", __func__, screen_rotate);

	return count;
}

static ssize_t epen_hand_store(struct device *dev,
struct device_attribute *attr, const char *buf,
	size_t count)
{
	unsigned int val;

	sscanf(buf, "%u", &val);

	if (val == 0 || val == 1)
		user_hand = (unsigned char)val;

	/* 0:Left hand, 1:Right Hand */
	printk(KERN_DEBUG"epen:%s: hand=%u\n", __func__, user_hand);

	return count;
}
#endif

static ssize_t epen_firmware_update_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	struct fw_update_info *info = &wac_i2c->update_info;

	printk(KERN_DEBUG "epen:%s\n", __func__);

	switch (*buf) {
	case 'i':
	case 'I':
		info->fw_path = FW_IN_SDCARD;
		break;
	case 'k':
	case 'K':
		info->fw_path = FW_BUILT_IN;
		break;
	default:
		printk(KERN_ERR "epen:wrong parameter\n");
		return count;
	}

	wac_i2c->update_info.forced = true;
	cancel_work_sync(&wac_i2c->update_work);
	schedule_work(&wac_i2c->update_work);
	
	return count;
}

int load_fw_built_in(struct wacom_i2c *wac_i2c)
{
	int retry = 3;
	int ret;

	while (retry--) {
		ret =
			request_firmware(&wac_i2c->update_info.firm_data, fw_name,
			&wac_i2c->client->dev);
		if (ret < 0) {
			printk(KERN_ERR
				"epen:Unable to open firmware. ret %d retry %d\n",
				ret, retry);
			continue;
		}
		break;
	}

	return ret;
}

int load_fw_sdcard(struct wacom_i2c *wac_i2c)
{
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int ret = 0;
	unsigned int nSize;
	u8 **ums_data = &wac_i2c->update_info.fw_data;

	nSize = WACOM_FW_SIZE;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(WACOM_FW_PATH, O_RDONLY, S_IRUSR);

	if (IS_ERR(fp)) {
		printk(KERN_ERR "epen:failed to open %s.\n", WACOM_FW_PATH);
		ret = -ENOENT;
		set_fs(old_fs);
		return ret;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	printk(KERN_NOTICE
		"epen:start, file path %s, size %ld Bytes\n",
		WACOM_FW_PATH, fsize);

	if (fsize != nSize) {
		printk(KERN_ERR "epen:UMS firmware size is different\n");
		ret = -EFBIG;
		goto size_error;
	}

	*ums_data = kmalloc(fsize, GFP_KERNEL);
	if (IS_ERR(*ums_data)) {
		printk(KERN_ERR
			"epen:%s, kmalloc failed\n", __func__);
		ret = -EFAULT;
		goto malloc_error;
	}

	nread = vfs_read(fp, (char __user *)*ums_data,
		fsize, &fp->f_pos);
	printk(KERN_NOTICE "epen:nread %ld Bytes\n", nread);
	if (nread != fsize) {
		printk(KERN_ERR
			"epen:failed to read firmware file, nread %ld Bytes\n",
			nread);
		ret = -EIO;
		kfree(*ums_data);
		goto read_err;
	}

	filp_close(fp, current->files);
	set_fs(old_fs);

	return 0;

read_err:
malloc_error:
size_error:
	filp_close(fp, current->files);
	set_fs(old_fs);
	return ret;
}


int wacom_i2c_load_fw(struct wacom_i2c *wac_i2c)
{
	int ret = 0;

	switch (wac_i2c->update_info.fw_path) {
	case FW_BUILT_IN:
		ret = load_fw_built_in(wac_i2c);
		break;
	case FW_IN_SDCARD:
		ret = load_fw_sdcard(wac_i2c);
		break;
	default:
		break;
	}

	return ret;
}

void wacom_i2c_unload_fw(struct wacom_i2c *wac_i2c)
{
	switch (wac_i2c->update_info.fw_path) {
	case FW_BUILT_IN:
		release_firmware(wac_i2c->update_info.firm_data);
		wac_i2c->update_info.firm_data = NULL;
		break;
	case FW_IN_SDCARD:
		kfree(wac_i2c->update_info.fw_data);
		wac_i2c->update_info.fw_data = NULL;
		break;
	default:
		break;
	}
}

static void wacom_i2c_update_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
		container_of(work, struct wacom_i2c, update_work);
	u32 fw_ver_ic = wac_i2c->wac_feature->fw_version;
	int ret;
	int retry = 3;

	mutex_lock(&wac_i2c->update_lock);
	wacom_enable_irq(wac_i2c, false);

	printk(KERN_DEBUG"epen:%s\n", __func__);

	if (!wac_i2c->update_info.forced) {
#if defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		if (fw_ver_ic == fw_ver_file) {
#else
		if (fw_ver_ic >= fw_ver_file) {
#endif
			printk(KERN_DEBUG"epen:pass fw update, ic ver %#x, img ver %#x\n",
					fw_ver_ic, fw_ver_file);
			goto err_update_load_fw;
		}
	}

	wac_i2c->wac_feature->firm_update_status = 1;

	ret = wacom_i2c_load_fw(wac_i2c);
	if (ret < 0) {
		printk(KERN_DEBUG"epen:failed to load fw(%d)\n", ret);
		goto err_update_load_fw;
	}

	while (retry--) {
		ret = wacom_i2c_flash(wac_i2c);
		if (ret) {
			printk(KERN_DEBUG"epen:failed to flash fw(%d)\n", ret);
			continue;
		}
		break;
	}
	if (ret)
		goto err_update_fw;	

	ret = wacom_i2c_query(wac_i2c);
	if (ret < 0) {
		printk(KERN_DEBUG"epen:failed to query to IC(%d)\n", ret);
		goto err_update_fw;
	}

 err_update_fw:
	wacom_i2c_unload_fw(wac_i2c);
 err_update_load_fw:
	wac_i2c->wac_feature->firm_update_status = 2;
	wacom_enable_irq(wac_i2c, true);
	mutex_unlock(&wac_i2c->update_lock);
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
		wacom_i2c_reset_hw(wac_i2c->wac_pdata);
		/* I2C Test */
		wacom_i2c_query(wac_i2c);

		wacom_enable_irq(wac_i2c, true);

		printk(KERN_DEBUG "epen:%s, result %d\n", __func__,
		       wac_i2c->query_status);
	}

	return count;
}

static ssize_t epen_reset_result_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	if (wac_i2c->query_status) {
		printk(KERN_DEBUG "epen:%s, PASS\n", __func__);
		return sprintf(buf, "PASS\n");
	} else {
		printk(KERN_DEBUG "epen:%s, FAIL\n", __func__);
		return sprintf(buf, "FAIL\n");
	}
}

#ifdef WACOM_USE_AVE_TRANSITION
static ssize_t epen_ave_store(struct device *dev,
struct device_attribute *attr,
	const char *buf, size_t count)
{
	int v1, v2, v3, v4, v5;
	int height;

	sscanf(buf, "%d%d%d%d%d%d", &height, &v1, &v2, &v3, &v4, &v5);

	if (height < 0 || height > 2) {
		printk(KERN_DEBUG"epen:Height err %d\n", height);
		return count;
	}

	g_aveLevel_C[height] = v1;
	g_aveLevel_X[height] = v2;
	g_aveLevel_Y[height] = v3;
	g_aveLevel_Trs[height] = v4;
	g_aveLevel_Cor[height] = v5;

	printk(KERN_DEBUG "epen:%s, v1 %d v2 %d v3 %d v4 %d\n", __func__,
		v1, v2, v3, v4);

	return count;
}

static ssize_t epen_ave_result_show(struct device *dev,
struct device_attribute *attr,
	char *buf)
{
	printk(KERN_DEBUG "epen:%s\n%d %d %d %d\n"
		"%d %d %d %d\n%d %d %d %d\n",
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
		printk(KERN_DEBUG"epen: wrong cmd %d\n", val);
		return count;
	}

	wacom_enable_irq(wac_i2c, false);
	wacom_checksum(wac_i2c);
	wacom_enable_irq(wac_i2c, true);

	printk(KERN_DEBUG "epen:%s, result %d\n",
		__func__, wac_i2c->checksum_result);

	return count;
}

static ssize_t epen_checksum_result_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	if (wac_i2c->checksum_result) {
		printk(KERN_DEBUG "epen:checksum, PASS\n");
		return sprintf(buf, "PASS\n");
	} else {
		printk(KERN_DEBUG "epen:checksum, FAIL\n");
		return sprintf(buf, "FAIL\n");
	}
}

#ifdef WACOM_CONNECTION_CHECK
static void wacom_open_test(struct wacom_i2c *wac_i2c)
{
	u8 cmd = 0;
	u8 buf[2] = {0,};
	int ret = 0, retry = 10;

	cmd = WACOM_I2C_STOP;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, false);
	if (ret <= 0) {
		printk(KERN_ERR "epen:failed to send stop command\n");
		return ;
	}
	
	usleep_range(500, 500);

	cmd = WACOM_I2C_GRID_CHECK;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, false);
	if (ret <= 0) {
		printk(KERN_ERR "epen:failed to send stop command\n");
		goto grid_check_error;
	}

	msleep(150);

	cmd = WACOM_STATUS;
	do {
		printk(KERN_DEBUG"epen:read status, retry %d\n", retry);
		ret = wacom_i2c_send(wac_i2c, &cmd, 1, false);
		if (ret != 1) {
			printk(KERN_DEBUG"epen:failed to send cmd(ret:%d)\n", ret);
			continue;
		}
		usleep_range(500, 500);
		ret = wacom_i2c_recv(wac_i2c, buf, 2, false);
		if (ret != 2) {
			printk(KERN_DEBUG"epen:failed to recv data(ret:%d)\n", ret);
			continue;
		}
		
		/*
		*	status value
		*	0 : data is not ready
		*	1 : PASS
		*	2 : Fail (coil function error)
		*	3 : Fail (All coil function error)
		*/
		if (buf[0] == 1) {
			printk(KERN_DEBUG"epen:Pass\n");
			break;
		}

		printk(KERN_DEBUG"epen:buf[0]:%d, buf[1]:%d\n", buf[0], buf[1]);
		msleep(50);
	} while (retry--);

	wac_i2c->connection_check = (1 == buf[0]);
	printk(KERN_DEBUG
	       "epen:epen_connection : %s buf[1]:%d\n",
	       (1 == buf[0]) ? "Pass" : "Fail", buf[1]);

grid_check_error:
	cmd = WACOM_I2C_STOP;
	wacom_i2c_send(wac_i2c, &cmd, 1, false);

	cmd = WACOM_I2C_START;
	wacom_i2c_send(wac_i2c, &cmd, 1, false);
}

static ssize_t epen_connection_show(struct device *dev,
struct device_attribute *attr,
	char *buf)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	printk(KERN_DEBUG"epen:%s\n", __func__);
	wacom_enable_irq(wac_i2c, false);
	wacom_open_test(wac_i2c);
	wacom_enable_irq(wac_i2c, true);

	printk(KERN_DEBUG
		"epen:connection_check : %d\n",
		wac_i2c->connection_check);
	return sprintf(buf, "%s\n",
		wac_i2c->connection_check ?
		"OK" : "NG");
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

	printk(KERN_DEBUG"epen:%s, val %d\n",
		__func__, wac_i2c->battery_saving_mode);

	if (!wac_i2c->pwr_flag) {
		printk(KERN_DEBUG"epen:pass pwr control\n");
		return count;
	}

	if (wac_i2c->battery_saving_mode) {
		if (wac_i2c->pen_insert)
			wacom_power_off(wac_i2c);
	} else {
	 	if(wac_i2c->enabled)
		wacom_power_on(wac_i2c);
	}
	return count;
}
#endif

#if defined(WACOM_BOOSTER) || defined(CONFIG_INPUT_BOOSTER)
static ssize_t epen_boost_level(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);
	unsigned int level;

	sscanf(buf, "%d", &level);

	if (level > 3) {
		dev_err(&wac_i2c->client->dev, "err to set boost_level %d\n", level);
		return count;
	}

#ifdef CONFIG_INPUT_BOOSTER
	change_boost_level(level, BOOSTER_DEVICE_PEN);
#elif defined(WACOM_BOOSTER)
	wac_i2c->boost_level = level;
#endif
	dev_info(&wac_i2c->client->dev, "%s %d\n", __func__, level);

	return count;
}
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
#if defined(WACOM_BOOSTER) || defined(CONFIG_INPUT_BOOSTER)
static DEVICE_ATTR(boost_level, S_IWUSR | S_IWGRP, NULL, epen_boost_level);
#endif

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

static struct attribute *epen_attributes[] = {
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
#if defined(WACOM_BOOSTER) || defined(CONFIG_INPUT_BOOSTER)
	&dev_attr_boost_level.attr,
#endif
	NULL,
};

static struct attribute_group epen_attr_group = {
	.attrs = epen_attributes,
};

static void wacom_init_abs_params(struct wacom_i2c *wac_i2c)
{
	int min_x, min_y;
	int max_x, max_y;
	int pressure;

	min_x = 0;
	min_y = 0;

#ifdef WACOM_USE_QUERY_DATA
	pressure = wac_i2c->wac_feature->pressure_max;
	max_x = wac_i2c->wac_feature->x_max;
	max_y = wac_i2c->wac_feature->y_max;
#elif defined(WACOM_USE_PDATA)
	min_x = wac_i2c->wac_pdata->min_x;
	max_x = wac_i2c->wac_pdata->max_x;
	min_y = wac_i2c->wac_pdata->min_y;	
	max_y = wac_i2c->wac_pdata->max_y;
	pressure = wac_i2c->wac_pdata->max_pressure;
#endif

	if (wac_i2c->wac_pdata->xy_switch) {
		input_set_abs_params(wac_i2c->input_dev, ABS_X, min_y,
			max_y, 4, 0);
		input_set_abs_params(wac_i2c->input_dev, ABS_Y, min_x,
			max_x, 4, 0);		
	} else {
		input_set_abs_params(wac_i2c->input_dev, ABS_X, min_x,
			max_x, 4, 0);
		input_set_abs_params(wac_i2c->input_dev, ABS_Y, min_y,
			max_y, 4, 0);
	}
	input_set_abs_params(wac_i2c->input_dev, ABS_PRESSURE, 0,
		pressure, 0, 0);
#ifdef WACOM_USE_GAIN
	input_set_abs_params(wac_i2c->input_dev, ABS_DISTANCE, 0,
		1024, 0, 0);
#endif
}

#ifdef WACOM_IMPORT_FW_ALGO
static void wacom_init_fw_algo(struct wacom_i2c *wac_i2c)
{
#if defined(CONFIG_MACH_T0)
	int digitizer_type = 0;

	/*Set data by digitizer type*/
	digitizer_type = wacom_i2c_get_digitizer_type();

	if (digitizer_type == EPEN_DTYPE_B746) {
		printk(KERN_DEBUG"epen:Use Box filter\n");
		wac_i2c->use_aveTransition = true;
	} else if (digitizer_type == EPEN_DTYPE_B713) {
		printk(KERN_DEBUG"epen:Reset tilt for B713\n");

		/*Change tuning version for B713*/
		tuning_version = tuning_version_B713;

		memcpy(tilt_offsetX, tilt_offsetX_B713, sizeof(tilt_offsetX));
		memcpy(tilt_offsetY, tilt_offsetY_B713, sizeof(tilt_offsetY));
	} else if (digitizer_type == EPEN_DTYPE_B660) {
		printk(KERN_DEBUG"epen:Reset tilt and origin for B660\n");

		origin_offset[0] = EPEN_B660_ORG_X;
		origin_offset[1] = EPEN_B660_ORG_Y;
		memset(tilt_offsetX, 0, sizeof(tilt_offsetX));
		memset(tilt_offsetY, 0, sizeof(tilt_offsetY));
		wac_i2c->use_offset_table = false;
	}

	/*Set switch type*/
	wac_i2c->invert_pen_insert = wacom_i2c_invert_by_switch_type();
#endif

}
#endif

#include <linux/of_gpio.h>

#ifdef CONFIG_INPUT_BOOSTER
#include <linux/input/input_booster.h>
#endif

#ifdef CONFIG_INPUT_WACOM_HL
static struct wacom_g5_callbacks *wacom_callbacks;
//static bool wacom_power_enabled;

static int g_gpio_ldo_en;;
static int g_gpio_reset_n;
static int g_gpio_fwe1;
static int g_gpio_irq;
static int g_gpio_insert_pen;
static int g_gpio_pdct;
static int g_gpio_insert_pen_pull;


int wacom_power(bool on)
{
	gpio_direction_output(g_gpio_ldo_en, on);
	return 0;
}

static int wacom_suspend_hw(void)
{
	printk(KERN_ERR "epen: %s,%d \n", __func__, __LINE__);

	gpio_direction_output(g_gpio_reset_n, 0);
	wacom_power(0);
	//gpio_tlmm_config(GPIO_CFG(g_gpio_pdct, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	//gpio_tlmm_config(GPIO_CFG(g_gpio_irq, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	return 0;
}

static int wacom_resume_hw(void)
{
	printk(KERN_ERR "epen: %s,%d \n", __func__, __LINE__);

	//gpio_direction_output(g_gpio_reset_n, 1);
	//gpio_direction_output(g_gpio_pdct, 1);
	wacom_power(1);
	msleep(100);
	gpio_direction_output(g_gpio_reset_n, 1);
	gpio_tlmm_config(GPIO_CFG(g_gpio_pdct, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(g_gpio_irq, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	return 0;
}

static int wacom_reset_hw(void)
{
	printk(KERN_ERR "epen: %s,%d \n", __func__, __LINE__);

	wacom_suspend_hw();
	msleep(100);
	wacom_resume_hw();

	return 0;
}

static void wacom_register_callbacks(struct wacom_g5_callbacks *cb)
{
	return; // jjlee_temp
	wacom_callbacks = cb;
};

static void wacom_compulsory_flash_mode(bool en)
{
	gpio_direction_output(g_gpio_fwe1, en);
}

static int wacom_get_irq_state(void)
{
	return gpio_get_value(g_gpio_irq);
}


void wacom_init_gpio(struct wacom_g5_platform_data *pdata)
{
	int ret;
	pr_err("epen: %s, %d \n",__func__, __LINE__ );
	pr_err("epen: %d, %d, %d, %d, %d \n",pdata->gpio_reset_n,pdata->gpio_fwe1, pdata->gpio_pendct,pdata->gpio_irq,pdata->gpio_ldo_en );

	// reset
	ret = gpio_request(pdata->gpio_reset_n, "gpio_reset_n");
	if(ret) {
		printk(KERN_INFO "epen: %s: unable to gpio [%d], r:%d\n", __func__, pdata->gpio_reset_n, ret);
		return;
	}	
	gpio_direction_output(pdata->gpio_reset_n, 0);

	// slp, fwe1
	ret = gpio_request(pdata->gpio_fwe1, "gpio_fwe1");
	if(ret) {
		printk(KERN_INFO "epen: %s: unable to gpio [%d], r:%d\n", __func__, pdata->gpio_fwe1, ret);
		return;
	}	
	gpio_direction_output(pdata->gpio_fwe1, 0);

	// pdct
	ret = gpio_request(pdata->gpio_pendct, "gpio_pendct");
	if(ret) {
		printk(KERN_INFO "epen: %s: unable to gpio [%d], r:%d\n", __func__, pdata->gpio_pendct, ret);
		return;
	}	
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_pendct, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	// irq
	ret = gpio_request(pdata->gpio_irq, "gpio_irq");
	if(ret) {
		printk(KERN_INFO "epen: %s: unable to gpio [%d], r:%d\n", __func__, pdata->gpio_irq, ret);
		return;
	}		
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_irq, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	// ldo en
	ret = gpio_request(pdata->gpio_ldo_en, "gpio_ldo_en");
	if(ret) {
		printk(KERN_INFO "epen: %s: unable to gpio [%d], r:%d\n", __func__, pdata->gpio_ldo_en, ret);
		return;
	}
	gpio_direction_output(pdata->gpio_ldo_en, 1);


	gpio_tlmm_config(GPIO_CFG(GPIO_PEN_SDA_18V, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(GPIO_PEN_SCL_N_18V, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	
	// pen_insert
	ret = gpio_request(pdata->gpio_pen_insert, "gpio_pen_insert");
	if(ret) {
		printk(KERN_INFO "epen: %s: unable to gpio [%d], r:%d\n", __func__, pdata->gpio_pen_insert, ret);
		return;
	}		
	if(g_gpio_insert_pen_pull){
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_pen_insert, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), 1);
	}else{
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_pen_insert, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	}
	
}
static int wacom_parse_dt(struct device *dev, struct wacom_g5_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	//u32 temp;

	pdata->gpio_ldo_en = of_get_named_gpio(np, "wacom,vdd_en-gpio", 0);
	pdata->gpio_irq = of_get_named_gpio(np, "wacom,irq-gpio", 0);
	g_gpio_insert_pen_pull = of_property_read_bool(np, "wacom,insert-pull-up");
	pdata->gpio_reset_n = of_get_named_gpio(np, "wacom,reset_n-gpio", 0);	
	pdata->gpio_fwe1 = of_get_named_gpio(np, "wacom,pen_fwe1-gpio", 0);
	pdata->gpio_pen_insert = of_get_named_gpio(np, "wacom,sense-gpio", 0);
	pdata->gpio_pendct = of_get_named_gpio(np, "wacom,pen_pdct-gpio", 0);
	
	printk(KERN_INFO "epen: %s: en_vdd:%d, irq:%d pull:%d \n",	__func__, pdata->gpio_ldo_en, pdata->gpio_irq, g_gpio_insert_pen_pull);

	g_gpio_ldo_en = pdata->gpio_ldo_en;
	g_gpio_irq = pdata->gpio_irq;	
	g_gpio_reset_n = pdata->gpio_reset_n;
	g_gpio_fwe1 = pdata->gpio_fwe1;
	g_gpio_insert_pen = pdata->gpio_pen_insert;
	g_gpio_pdct = pdata->gpio_pendct;

	printk(KERN_INFO "epen: %d, %d, %d, %d \n",	g_gpio_ldo_en, g_gpio_reset_n, g_gpio_fwe1 , g_gpio_insert_pen);

	return 0;
}


#endif

static int wacom_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	static struct wacom_g5_platform_data *pdata; // = client->dev.platform_data;
	struct wacom_i2c *wac_i2c;
	struct input_dev *input;
	int ret = 0;



#ifdef CONFIG_OF
	printk(KERN_ERR "epen: %s, start,%d\n",__func__, __LINE__);

	pdata = kzalloc(sizeof(struct wacom_g5_platform_data), GFP_KERNEL);
	if (!pdata) {
		printk(KERN_ERR "epen: pdata err = ENOMEM!\n");
		return -ENOMEM;
	}

	pdata->x_invert = WACOM_X_INVERT;
	pdata->y_invert = WACOM_Y_INVERT;
	pdata->xy_switch = WACOM_XY_SWITCH;
	pdata->min_x = 0;
	pdata->max_x = WACOM_MAX_COORD_X;
	pdata->min_y = 0;
	pdata->max_y = WACOM_MAX_COORD_Y;
	pdata->min_pressure = 0;
	pdata->max_pressure = WACOM_MAX_PRESSURE;
	pdata->suspend_platform_hw = wacom_suspend_hw;
	pdata->resume_platform_hw = wacom_resume_hw;
	pdata->reset_platform_hw = wacom_reset_hw;
	pdata->register_cb = wacom_register_callbacks;
	pdata->compulsory_flash_mode = wacom_compulsory_flash_mode;

	pdata->get_irq_state = wacom_get_irq_state;

	ret = wacom_parse_dt(&client->dev, pdata);

	if (ret) {
		printk(KERN_ERR "Error parsing dt %d\n", ret);				
		return ret;
	}

	wacom_init_gpio(pdata);

#else
	printk(KERN_ERR "epen: %s, start,%d\n",__func__, __LINE__);

	if (pdata == NULL) {
		printk(KERN_ERR "epen: %s: no pdata\n", __func__);
		ret = -ENODEV;
		goto err_i2c_fail;
	}
#endif

	/*Check I2C functionality */
	ret = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if (!ret) {
		printk(KERN_ERR "epen:No I2C functionality found\n");
		ret = -ENODEV;
		goto err_i2c_fail;
	}

	/*Obtain kernel memory space for wacom i2c */
	wac_i2c = kzalloc(sizeof(struct wacom_i2c), GFP_KERNEL);
	if (NULL == wac_i2c) {
		printk(KERN_ERR "epen:failed to allocate wac_i2c.\n");
		ret = -ENOMEM;
		goto err_alloc_mem;
	}

	wac_i2c->client_boot = i2c_new_dummy(client->adapter,
		WACOM_I2C_BOOT);
	if (!wac_i2c->client_boot) {
		dev_err(&client->dev, "Fail to register sub client[0x%x]\n",
			 WACOM_I2C_BOOT);
	}

	input = input_allocate_device();
	if (NULL == input) {
		printk(KERN_ERR "epen:failed to allocate input device.\n");
		ret = -ENOMEM;
		goto err_alloc_input_dev;
	}

	wacom_i2c_set_input_values(client, wac_i2c, input);

	wac_i2c->wac_feature = &wacom_feature_EMR;
	wac_i2c->wac_pdata = pdata;
	wac_i2c->input_dev = input;
	wac_i2c->client = client;

	//wac_i2c->irq = client->irq;
	wac_i2c->irq = gpio_to_irq(pdata->gpio_irq); 	//dtsi
	
	//printk(KERN_ERR "epen: irq %d, %d, %d\n", wac_i2c->irq, pdata->gpio_irq, client->irq);
	irq_set_irq_type(wac_i2c->irq, IRQ_TYPE_EDGE_RISING);	// dtsi

	/* init_completion(&wac_i2c->init_done); */
#ifdef WACOM_PDCT_WORK_AROUND
	wac_i2c->irq_pdct = gpio_to_irq(pdata->gpio_pendct);
	wac_i2c->pen_pdct = PDCT_NOSIGNAL;

	irq_set_irq_type(wac_i2c->irq_pdct , IRQ_TYPE_EDGE_BOTH);	// dtsi
#endif
#ifdef WACOM_PEN_DETECT
	wac_i2c->gpio_pen_insert = pdata->gpio_pen_insert;
#endif
#ifdef WACOM_IMPORT_FW_ALGO
	wac_i2c->use_offset_table = true;
	wac_i2c->use_aveTransition = false;
	wacom_init_fw_algo(wac_i2c);
#endif

	/*Change below if irq is needed */
	wac_i2c->irq_flag = 1;

	/*Register callbacks */
	wac_i2c->callbacks.check_prox = wacom_check_emr_prox;
	if (wac_i2c->wac_pdata->register_cb)
		wac_i2c->wac_pdata->register_cb(&wac_i2c->callbacks);

	/* Firmware Feature */
	wacom_i2c_init_firm_data();

	/* Power on */
	wac_i2c->wac_pdata->resume_platform_hw();
	msleep(60); // for booting time,  msleep(200);
	wac_i2c->power_enable = true;
	wac_i2c->pwr_flag = true;

	printk(KERN_ERR "epen: %s, query, %d\n", __func__, __LINE__);
	wacom_i2c_query(wac_i2c);

	wacom_init_abs_params(wac_i2c);
	input_set_drvdata(input, wac_i2c);

	/*Change below if irq is needed */
	wac_i2c->irq_flag = 1;

	/*Set client data */
	i2c_set_clientdata(client, wac_i2c);
	i2c_set_clientdata(wac_i2c->client_boot, wac_i2c);

	/*Initializing for semaphor */
	mutex_init(&wac_i2c->lock);
	mutex_init(&wac_i2c->update_lock);
	mutex_init(&wac_i2c->irq_lock);
	wake_lock_init(&wac_i2c->fw_wakelock, WAKE_LOCK_SUSPEND, "wacom");
	INIT_DELAYED_WORK(&wac_i2c->resume_work, wacom_i2c_resume_work);
#ifdef LCD_FREQ_SYNC
	mutex_init(&wac_i2c->freq_write_lock);
	INIT_WORK(&wac_i2c->lcd_freq_work, wacom_i2c_lcd_freq_work);
	INIT_DELAYED_WORK(&wac_i2c->lcd_freq_done_work, wacom_i2c_finish_lcd_freq_work);
	if (likely(system_rev >= LCD_FREQ_SUPPORT_HWID))
		wac_i2c->use_lcd_freq_sync = true;
#endif
#ifdef WACOM_USE_SOFTKEY_BLOCK
	INIT_DELAYED_WORK(&wac_i2c->softkey_block_work, wacom_i2c_block_softkey_work);
	wac_i2c->block_softkey = false;	
#endif
	INIT_WORK(&wac_i2c->update_work, wacom_i2c_update_work);
	/*init wacom booster*/
#if defined(WACOM_BOOSTER_DVFS)	
	wacom_init_dvfs(wac_i2c);
#elif defined(WACOM_BOOSTER)
	wacom_init_dvfs(wac_i2c);
	wac_i2c->boost_level = WACOM_BOOSTER_LEVEL2;
#endif
	printk(KERN_ERR "epen: %s,%d \n", __func__, __LINE__);

	/*Before registering input device, data in each input_dev must be set */
	ret = input_register_device(input);
	if (ret) {
		pr_err("epen:failed to register input device.\n");
		goto err_register_device;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	wac_i2c->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	wac_i2c->early_suspend.suspend = wacom_i2c_early_suspend;
	wac_i2c->early_suspend.resume = wacom_i2c_late_resume;
	register_early_suspend(&wac_i2c->early_suspend);
#endif

	wac_i2c->dev = device_create(sec_class, NULL, 0, NULL, "sec_epen");
	if (IS_ERR(wac_i2c->dev)) {
		printk(KERN_ERR "Failed to create device(wac_i2c->dev)!\n");
		ret = -ENODEV;
		goto err_create_device;
	}

	dev_set_drvdata(wac_i2c->dev, wac_i2c);

	ret = sysfs_create_group(&wac_i2c->dev->kobj, &epen_attr_group);
	if (ret) {
		printk(KERN_ERR
			    "epen:failed to create sysfs group\n");
		goto err_sysfs_create_group;
	}

	/* firmware info */
	printk(KERN_NOTICE "epen:wacom fw ver : 0x%x, new fw ver : 0x%x\n",
	       wac_i2c->wac_feature->fw_version, fw_ver_file);

	/*Request IRQ */
	if (wac_i2c->irq_flag) {
		ret =
		    request_threaded_irq(wac_i2c->irq, NULL, wacom_interrupt,
					 IRQF_DISABLED | EPEN_IRQF_TRIGGER_TYPE |
					 IRQF_ONESHOT, "sec_epen_irq", wac_i2c);
		if (ret < 0) {
			printk(KERN_ERR
			       "epen:failed to request irq(%d) - %d\n",
			       wac_i2c->irq, ret);
			goto err_request_irq;
		}
		printk(KERN_ERR "epen: %s,%d \n", __func__, __LINE__);

#if defined(WACOM_PDCT_WORK_AROUND)
		ret =
			request_threaded_irq(wac_i2c->irq_pdct, NULL,
					wacom_interrupt_pdct,
					IRQF_DISABLED | IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
					"sec_epen_pdct", wac_i2c);
		if (ret < 0) {
			printk(KERN_ERR
				"epen:failed to request irq(%d) - %d\n",
				wac_i2c->irq_pdct, ret);
			goto err_request_irq;
		}
#endif
	}
#ifdef WACOM_PEN_DETECT
	init_pen_insert(wac_i2c);
#endif
	printk(KERN_ERR "epen: %s,%d \n", __func__, __LINE__);

	wac_i2c->update_info.forced = false;
	wac_i2c->update_info.fw_path = FW_BUILT_IN;
	schedule_work(&wac_i2c->update_work);
	/*complete_all(&wac_i2c->init_done);*/


	printk(KERN_ERR "epen: %s, -end %d\n", __func__, __LINE__);

	return 0;

 err_request_irq:
	wake_lock_destroy(&wac_i2c->fw_wakelock);
	sysfs_remove_group(&wac_i2c->dev->kobj,
		&epen_attr_group);
 err_sysfs_create_group:
	device_destroy(sec_class, (dev_t)NULL);
 err_create_device:	
	input_unregister_device(input);
	input = NULL;
 err_register_device:
#ifdef LCD_FREQ_SYNC
	mutex_destroy(&wac_i2c->freq_write_lock);
#endif
	mutex_destroy(&wac_i2c->irq_lock);
	mutex_destroy(&wac_i2c->update_lock);
	mutex_destroy(&wac_i2c->lock);
	input_free_device(input);
 err_alloc_input_dev:
	kfree(wac_i2c);
	wac_i2c = NULL;
 err_alloc_mem:
 err_i2c_fail:
	return ret;
}
/*
void wacom_i2c_shutdown(struct i2c_client *client)
{
	struct wacom_i2c *wac_i2c = i2c_get_clientdata(client);

	wac_i2c->wac_pdata->suspend_platform_hw();

	printk(KERN_DEBUG"epen:%s\n", __func__);
}
*/
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

MODULE_DEVICE_TABLE(i2c, wacom_i2c_id);

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


static int __init wacom_i2c_init(void)
{
	int ret = 0;

	printk(KERN_ERR "%s, start,%d\n",__func__, __LINE__);
#ifdef CONFIG_OF
	printk(KERN_ERR "%s, used of \n",__func__);
#endif
	ret = i2c_add_driver(&wacom_i2c_driver);
	if (ret)
		printk(KERN_ERR "wacom :fail to i2c_add_driver\n");
	return ret;
}

static void __exit wacom_i2c_exit(void)
{
	i2c_del_driver(&wacom_i2c_driver);
}

//late_initcall(wacom_i2c_init);
module_init(wacom_i2c_init);
module_exit(wacom_i2c_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Driver for Wacom G5SP Digitizer Controller");

MODULE_LICENSE("GPL");
