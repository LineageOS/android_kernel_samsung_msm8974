/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
*
* File Name		: fts.c
* Authors		: AMS(Analog Mems Sensor) Team
* Description	: FTS Capacitive touch screen controller (FingerTipS)
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
********************************************************************************
* REVISON HISTORY
* DATE		| DESCRIPTION
* 03/09/2012| First Release
* 08/11/2012| Code migration
* 23/01/2013| SEC Factory Test
* 29/01/2013| Support Hover Events
* 08/04/2013| SEC Factory Test Add more - hover_enable, glove_mode, clear_cover_mode, fast_glove_mode
* 09/04/2013| Support Blob Information
*******************************************************************************/

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/serio.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
//#include "fts.h"
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/power_supply.h>
#include <linux/firmware.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>

#ifdef CONFIG_OF
#ifndef USE_OPEN_CLOSE
#define USE_OPEN_CLOSE
#undef CONFIG_HAS_EARLYSUSPEND
#undef CONFIG_PM
#endif
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/input/mt.h>
#include "fts_ts.h"

static struct i2c_driver fts_i2c_driver;

static bool MutualTouchMode = false;

#ifdef CONFIG_GLOVE_TOUCH
enum TOUCH_MODE {
	FTS_TM_NORMAL = 0,
	FTS_TM_GLOVE,
};
#endif


static int retry_hover_enable_after_wakeup = 0;

#ifdef USE_OPEN_CLOSE
static int fts_input_open(struct input_dev *dev);
static void fts_input_close(struct input_dev *dev);
#ifdef USE_OPEN_DWORK
static void fts_open_work(struct work_struct *work);
#endif
#endif

static int fts_stop_device(struct fts_ts_info *info);
static int fts_start_device(struct fts_ts_info *info);
static int fts_irq_enable(struct fts_ts_info *info, bool enable);


#if (!defined(CONFIG_HAS_EARLYSUSPEND)) && (!defined(CONFIG_PM)) && !defined(USE_OPEN_CLOSE)
static int fts_suspend(struct i2c_client *client, pm_message_t mesg);
static int fts_resume(struct i2c_client *client);
#endif

int fts_wait_for_ready(struct fts_ts_info *info);

#ifdef FTS_SUPPORT_TOUCH_KEY
struct fts_touchkey fts_touchkeys[] = {
	{
		.value = 0x01,
		.keycode = KEY_DUMMY_MENU,
		.name = "d_menu",
	},
	{
		.value = 0x02,
		.keycode = KEY_RECENT,
		.name = "menu",
	},
	{
		.value = 0x04,
		.keycode = KEY_BACK,
		.name = "back",
	},
	{
		.value = 0x08,
		.keycode = KEY_DUMMY_BACK,
		.name = "d_back",
	},
};
#endif // FTS_SUPPORT_TOUCH_KEY

#ifdef CONFIG_HAS_EARLYSUSPEND
static void fts_early_suspend(struct early_suspend *h)
{
	struct fts_ts_info *info;
	info = container_of(h, struct fts_ts_info, early_suspend);
	fts_suspend(info->client, PMSG_SUSPEND);
}

static void fts_late_resume(struct early_suspend *h)
{
	struct fts_ts_info *info;
	info = container_of(h, struct fts_ts_info, early_suspend);
	fts_resume(info->client);
}
#endif


#ifdef FTS_SUPPORT_TA_MODE
extern void fts_register_callback(void *cb);
#endif

int fts_write_reg(struct fts_ts_info *info,
		  unsigned char *reg, unsigned short num_com)
{
	struct i2c_msg xfer_msg[2];
	int ret;

	if (info->touch_stopped) {
		tsp_debug_err(true, &info->client->dev, "%s: Sensor stopped\n", __func__);
		goto exit;
	}

	mutex_lock(&info->i2c_mutex);

	xfer_msg[0].addr = info->client->addr;
	xfer_msg[0].len = num_com;
	xfer_msg[0].flags = 0;
	xfer_msg[0].buf = reg;

	ret = i2c_transfer(info->client->adapter, xfer_msg, 1);

	mutex_unlock(&info->i2c_mutex);
	return ret;

 exit:
	return 0;
}

int fts_read_reg(struct fts_ts_info *info, unsigned char *reg, int cnum,
		 unsigned char *buf, int num)
{
	struct i2c_msg xfer_msg[2];
	int ret;

	if (info->touch_stopped) {
		tsp_debug_err(true, &info->client->dev, "%s: Sensor stopped\n", __func__);
		goto exit;
	}

	mutex_lock(&info->i2c_mutex);

	xfer_msg[0].addr = info->client->addr;
	xfer_msg[0].len = cnum;
	xfer_msg[0].flags = 0;
	xfer_msg[0].buf = reg;

	xfer_msg[1].addr = info->client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags = I2C_M_RD;
	xfer_msg[1].buf = buf;

	ret = i2c_transfer(info->client->adapter, xfer_msg, 2);

	mutex_unlock(&info->i2c_mutex);
	return ret;

 exit:
	return 0;
}

static void fts_delay(unsigned int ms)
{
	if (ms < 20)
		usleep_range(ms, ms);
	else
		msleep(ms);
}

void fts_command(struct fts_ts_info *info, unsigned char cmd)
{
	unsigned char regAdd = 0;
	int ret = 0;

	regAdd = cmd;
	ret = fts_write_reg(info, &regAdd, 1);
	tsp_debug_info(true, &info->client->dev, "FTS Command (%02X) , ret = %d \n", cmd, ret);
}

void fts_systemreset(struct fts_ts_info *info)
{
	unsigned char regAdd[4] = { 0xB6, 0x00, 0x23, 0x01 };
	tsp_debug_info(true, &info->client->dev, "FTS SystemReset\n");
	fts_write_reg(info, &regAdd[0], 4);
	fts_delay(10);
}

static void fts_interrupt_set(struct fts_ts_info *info, int enable)
{
	unsigned char regAdd[4] = { 0xB6, 0x00, 0x1C, enable };

	if (enable)
		tsp_debug_info(true, &info->client->dev, "FTS INT Enable\n");
	else
		tsp_debug_info(true, &info->client->dev, "FTS INT Disable\n");

	fts_write_reg(info, &regAdd[0], 4);
}

static void fts_set_flipcover_mode(struct fts_ts_info *info, bool enable)
{

#if !defined(CONFIG_SEC_T10_PROJECT)   // not use 0x96 register is key cx tune
	if (enable)
		fts_command(info, FTS_CMD_FLIPCOVER_ON);
	else
		fts_command(info, FTS_CMD_FLIPCOVER_OFF);
#else
        return;
#endif
}


int fts_wait_for_ready(struct fts_ts_info *info)
{
	int rc;
	unsigned char regAdd;
	unsigned char data[FTS_EVENT_SIZE];
	int retry = 0;
	int err_cnt=0;

	memset(data, 0x0, FTS_EVENT_SIZE);

	regAdd = READ_ONE_EVENT;
	rc = -1;
	while (fts_read_reg
	       (info, &regAdd, 1, (unsigned char *)data, FTS_EVENT_SIZE)) {

		tsp_debug_info(true, &info->client->dev, "Data : %X \n", data[0]);
		if (data[0] == EVENTID_CONTROLLER_READY) {
			rc = 0;
			break;
		}

		if (data[0] == EVENTID_ERROR) {
			if (err_cnt++>32) {
			rc = -2;
			break;
		}
			continue;
		}

		if (retry++ > FTS_RETRY_COUNT) {
			rc = -1;
			tsp_debug_info(true, &info->client->dev, "%s: Time Over\n", __func__);
			break;
		}
		fts_delay(20);
	}

	return rc;
}

int fts_get_version_info(struct fts_ts_info *info)
{
	int rc;
	unsigned char regAdd[3];
	unsigned char data[FTS_EVENT_SIZE];
	int retry = 0;

	fts_command(info, FTS_CMD_RELEASEINFO);

	memset(data, 0x0, FTS_EVENT_SIZE);

	regAdd[0] = READ_ONE_EVENT;
	rc = -1;
	while (fts_read_reg(info, &regAdd[0], 1, (unsigned char *)data, FTS_EVENT_SIZE)) {
		if (data[0] == EVENTID_INTERNAL_RELEASE_INFO) {
			// Internal release Information
			info->fw_version_of_ic = (data[3] << 8) + data[4];
			info->config_version_of_ic = (data[6] << 8) + data[5];
		} else if (data[0] == EVENTID_EXTERNAL_RELEASE_INFO) {
			// External release Information
			info->fw_main_version_of_ic = (data[1] << 8)+data[2];
			rc = 0;
			break;
		}

		if (retry++ > FTS_RETRY_COUNT) {
			rc = -1;
			tsp_debug_info(true, &info->client->dev, "%s: Time Over\n", __func__);
			break;
		}
	}

	tsp_debug_info(true, &info->client->dev,
			"IC Firmware Version : 0x%04X "
			"IC Config Version : 0x%04X "
			"IC Main Version : 0x%04X\n",
			info->fw_version_of_ic,
			info->config_version_of_ic,
			info->fw_main_version_of_ic);

	return rc;
}

#ifdef FTS_SUPPORT_NOISE_PARAM
int fts_get_noise_param_address(struct fts_ts_info *info)
{
	int rc;
	unsigned char regAdd[3];
	struct fts_noise_param *noise_param;
	int i;

	noise_param = (struct fts_noise_param *)&info->noise_param;

	regAdd[0] = 0xd0;
	regAdd[1] = 0x00;
	regAdd[2] = 32 * 2;
	rc = fts_read_reg(info, regAdd, 3, (unsigned char *)noise_param->pAddr, 2);

	for (i = 1; i < MAX_NOISE_PARAM; i++) {
		noise_param->pAddr[i] = noise_param->pAddr[0] + i * 2;
	}

	for (i = 0; i < MAX_NOISE_PARAM; i++) {
		tsp_debug_dbg(true, &info->client->dev, "Get Noise Param%d Address = 0x%4x\n", i,
		       noise_param->pAddr[i]);
	}

	return rc;
}

static int fts_get_noise_param(struct fts_ts_info *info)
{
	int rc;
	unsigned char regAdd[3];
	unsigned char data[MAX_NOISE_PARAM * 2];
	struct fts_noise_param *noise_param;
	int i;
	unsigned char buf[3];

	noise_param = (struct fts_noise_param *)&info->noise_param;
	memset(data, 0x0, MAX_NOISE_PARAM * 2);

	for (i = 0; i < MAX_NOISE_PARAM; i++) {
		regAdd[0] = 0xb3;
		regAdd[1] = 0x00;
		regAdd[2] = 0x10;
		fts_write_reg(info, regAdd, 3);

		regAdd[0] = 0xb1;
		regAdd[1] = (noise_param->pAddr[i] >> 8) & 0xff;
		regAdd[2] = noise_param->pAddr[i] & 0xff;
		rc = fts_read_reg(info, regAdd, 3, &buf[0], 3);

		noise_param->pData[i] = buf[1]+(buf[2]<<8);
		//tsp_debug_info(true, &info->client->dev, "0x%2x%2x%2x 0x%2x 0x%2x\n", regAdd[0],regAdd[1],regAdd[2], buf[1], buf[2]);
	}

	for (i = 0; i < MAX_NOISE_PARAM; i++) {
		tsp_debug_dbg(true, &info->client->dev, "Get Noise Param%d Address [ 0x%04x ] = 0x%04x\n", i,
		       noise_param->pAddr[i], noise_param->pData[i]);
	}

	return rc;
}

static int fts_set_noise_param(struct fts_ts_info *info)
{
	int i;
	unsigned char regAdd[5];
	struct fts_noise_param *noise_param;

	noise_param = (struct fts_noise_param *)&info->noise_param;

	for (i = 0; i < MAX_NOISE_PARAM; i++) {
		regAdd[0] = 0xb3;
		regAdd[1] = 0x00;
		regAdd[2] = 0x10;
		fts_write_reg(info, regAdd, 3);

		regAdd[0] = 0xb1;
		regAdd[1] = (noise_param->pAddr[i] >> 8) & 0xff;
		regAdd[2] = noise_param->pAddr[i] & 0xff;
		regAdd[3] = noise_param->pData[i] & 0xff;
		regAdd[4] = (noise_param->pData[i] >> 8) & 0xff;
		fts_write_reg(info, regAdd, 5);
	}

	for (i = 0; i < MAX_NOISE_PARAM; i++) {
		tsp_debug_dbg(true, &info->client->dev, "Set Noise Param%d Address [ 0x%04x ] = 0x%04x\n", i,
		       noise_param->pAddr[i], noise_param->pData[i]);
	}

	return 0;
}
#endif// FTS_SUPPORT_NOISE_PARAM

#ifdef TOUCH_BOOSTER_DVFS
int useing_in_tsp_or_epen = 0;
static void fts_change_dvfs_lock(struct work_struct *work)
{
	struct fts_ts_info *info =
		container_of(work, struct fts_ts_info, work_dvfs_chg.work);
	int retval = 0;

	mutex_lock(&info->dvfs_lock);

	if (info->dvfs_boost_mode == DVFS_STAGE_DUAL) {
        if (info->stay_awake) {
            dev_info(&info->client->dev,
                "%s: do fw update, do not change cpu frequency.\n",
                __func__);
        } else {
            retval = set_freq_limit(DVFS_TOUCH_ID,
                MIN_TOUCH_LIMIT_SECOND);
            info->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
		}
    } else if (info->dvfs_boost_mode == DVFS_STAGE_SINGLE ||
        info->dvfs_boost_mode == DVFS_STAGE_TRIPLE) {
	        retval = set_freq_limit(DVFS_TOUCH_ID, -1);
	        info->dvfs_freq = -1;
	}
#ifdef CONFIG_SEC_S_PROJECT
	else if (info->dvfs_boost_mode == DVFS_STAGE_NINTH){
		retval = set_freq_limit(DVFS_TOUCH_ID,
				MIN_TOUCH_LIMIT_SECOND_9LEVEL);
		info->dvfs_freq = MIN_TOUCH_LIMIT_SECOND_9LEVEL;
	}
#endif
    if (retval < 0)
        dev_err(&info->client->dev,
            "%s: booster change failed(%d).\n",
            __func__, retval);

	mutex_unlock(&info->dvfs_lock);
}

static void fts_set_dvfs_off(struct work_struct *work)
{
	struct fts_ts_info *info =
		container_of(work, struct fts_ts_info, work_dvfs_off.work);
	int retval;

	if (info->stay_awake) {
		dev_info(&info->client->dev,
			"%s: do fw update, do not change cpu frequency.\n",
			__func__);
	} else {
		mutex_lock(&info->dvfs_lock);

		if((useing_in_tsp_or_epen & 0x01)== 0x01){
			useing_in_tsp_or_epen = 0x01;
			retval = 0;
		}else{
        retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		useing_in_tsp_or_epen = 0;
		}
        info->dvfs_freq = -1;

        if (retval < 0)
			dev_err(&info->client->dev,
				"%s: booster stop failed(%d).\n",
				__func__, retval);
		info->dvfs_lock_status = false;
		mutex_unlock(&info->dvfs_lock);
	}
}

static void fts_set_dvfs_lock(struct fts_ts_info *info, int on)
{
	int ret = 0;

	if (info->dvfs_boost_mode == DVFS_STAGE_NONE) {
                dev_dbg(&info->client->dev,
			"%s: DVFS stage is none(%d)\n",
			__func__, info->dvfs_boost_mode);
		return;
	}

	mutex_lock(&info->dvfs_lock);
    if (on == 0) {
		if (info->dvfs_lock_status){
#ifdef CONFIG_SEC_S_PROJECT
			if(info->dvfs_boost_mode == DVFS_STAGE_NINTH)
				schedule_delayed_work(&info->work_dvfs_off,
					msecs_to_jiffies(INPUT_BOOSTER_HIGH_OFF_TIME_TSP));
			else
#endif
			schedule_delayed_work(&info->work_dvfs_off,
				msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));
		}
	} else if (on > 0) {
		cancel_delayed_work(&info->work_dvfs_off);

		if ((!info->dvfs_lock_status) || (info->dvfs_old_stauts < on)) {
			cancel_delayed_work(&info->work_dvfs_chg);
			useing_in_tsp_or_epen = useing_in_tsp_or_epen | 0x2;

#ifdef CONFIG_SEC_S_PROJECT
			if(info->dvfs_boost_mode == DVFS_STAGE_NINTH){
				if (info->dvfs_freq != MIN_TOUCH_HIGH_LIMIT) {
					ret = set_freq_limit(DVFS_TOUCH_ID,
							MIN_TOUCH_HIGH_LIMIT);
					info->dvfs_freq = MIN_TOUCH_HIGH_LIMIT;
				}
				schedule_delayed_work(&info->work_dvfs_chg,
					msecs_to_jiffies(INPUT_BOOSTER_HIGH_CHG_TIME_TSP));
			}
			else
#endif
			{
            if (info->dvfs_freq != MIN_TOUCH_LIMIT) {
                if (info->dvfs_boost_mode == DVFS_STAGE_TRIPLE)
                    ret = set_freq_limit(DVFS_TOUCH_ID,
                            MIN_TOUCH_LIMIT_SECOND);
                else
                    ret = set_freq_limit(DVFS_TOUCH_ID,
                            MIN_TOUCH_LIMIT);
                info->dvfs_freq = MIN_TOUCH_LIMIT;

				}
				schedule_delayed_work(&info->work_dvfs_chg,
					msecs_to_jiffies(TOUCH_BOOSTER_CHG_TIME));
			}
				if (ret < 0)
					dev_err(&info->client->dev,
						"%s: cpu first lock failed(%d)\n",
							__func__, ret);
			info->dvfs_lock_status = true;
		}
    } else if (on < 0) {
		if (info->dvfs_lock_status) {
			cancel_delayed_work(&info->work_dvfs_off);
			cancel_delayed_work(&info->work_dvfs_chg);
			schedule_work(&info->work_dvfs_off.work);
		}
	}
	info->dvfs_old_stauts = on;
	mutex_unlock(&info->dvfs_lock);
}

static int fts_init_dvfs(struct fts_ts_info *info)
{
	mutex_init(&info->dvfs_lock);
	info->dvfs_boost_mode = DVFS_STAGE_DUAL;

	INIT_DELAYED_WORK(&info->work_dvfs_off, fts_set_dvfs_off);
	INIT_DELAYED_WORK(&info->work_dvfs_chg, fts_change_dvfs_lock);

	info->dvfs_lock_status = false;
	return 0;
}
#endif

#ifdef FTS_SUPPORT_TOUCH_KEY
void fts_release_all_key(struct fts_ts_info *info)
{
	//int i = 0;
	if (info->tsp_keystatus != TOUCH_KEY_NULL) {

		/* menu key check*/
		if (info->tsp_keystatus & TOUCH_KEY_RECENT) {
			if(info->ignore_menu_key) {
				tsp_debug_info(true, &info->client->dev, "[TSP_KEY] Ignore menu R! by dummy key\n");
			} else if (info->ignore_menu_key_by_back) {
				tsp_debug_info(true, &info->client->dev, "[TSP_KEY] Ignore menu R! by back key\n");
			} else {
				input_report_key(info->input_dev, KEY_RECENT, KEY_RELEASE);
				tsp_debug_info(true, &info->client->dev, "[TSP_KEY] Recent R!\n");
			}
		}

		/* back key check*/
		if (info->tsp_keystatus & TOUCH_KEY_BACK) {
			if (info->ignore_back_key) {
				tsp_debug_info(true, &info->client->dev, "[TSP_KEY] Ignore back R! by dummy key\n");
			} else if (info->ignore_back_key_by_menu) {
				tsp_debug_info(true, &info->client->dev, "[TSP_KEY] Ignore back R! by menu key\n");
			} else {
				input_report_key(info->input_dev, KEY_BACK, KEY_RELEASE);
				tsp_debug_info(true, &info->client->dev, "[TSP_KEY] back R!\n");
			}
		}

		if (info->report_dummy_key){
			printk("\n Inside Report DUMMY KEYS RELEASe ALWAYS \n");
			input_report_key(info->input_dev, KEY_DUMMY_MENU, KEY_RELEASE);
			tsp_debug_info(true, &info->client->dev, "[TSP_KEY]DUMMY  menu R!\n");
			input_report_key(info->input_dev, KEY_DUMMY_BACK, KEY_RELEASE);
			tsp_debug_info(true, &info->client->dev, "[TSP_KEY]DUMMY  back R!\n");
		}
		input_sync(info->input_dev);

		info->tsp_keystatus = TOUCH_KEY_NULL;

		if (info->ignore_menu_key) {
			info->ignore_menu_key = false;
			tsp_debug_info(true, &info->client->dev, "[TSP_KEY] ignore_menu_key Disable!\n");
		}
		if (info->ignore_back_key) {
			info->ignore_back_key = false;
			tsp_debug_info(true, &info->client->dev, "[TSP_KEY] ignore_back_key Disable!\n");
		}

		info->ignore_back_key_by_menu = false;
		info->ignore_menu_key_by_back = false;
	}
}
#endif // FTS_SUPPORT_TOUCH_KEY

/* Added for samsung dependent codes such as Factory test,
 * Touch booster, Related debug sysfs.
 */
#include "fts_sec.c"

static int fts_init(struct fts_ts_info *info)
{
	unsigned char val[16];
	unsigned char regAdd[8];
	int rc;

	fts_delay(200);

	// TS Chip ID
	regAdd[0] = 0xB6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x07;
#if defined(CONFIG_SEC_S_PROJECT)
	rc = fts_read_reg(info, regAdd, 3, (unsigned char *)val, 7);
	tsp_debug_info(true, &info->client->dev, "FTS %02X%02X%02X =  %02X %02X %02X %02X / %02X %02X \n",
		   regAdd[0], regAdd[1], regAdd[2], val[1], val[2], val[3], val[4], val[5], val[6]);
#else
	rc = fts_read_reg(info, regAdd, 3, (unsigned char *)val, 5);
	tsp_debug_info(true, &info->client->dev, "FTS %02X%02X%02X =  %02X %02X %02X %02X \n",
	       regAdd[0], regAdd[1], regAdd[2], val[1], val[2], val[3], val[4]);
#endif
	if (val[1] != FTS_ID0 || val[2] != FTS_ID1)
		return 1;

	fts_systemreset(info);

	rc=fts_wait_for_ready(info);
	if (rc==-2) {
		info->fw_version_of_ic =0;
		info->config_version_of_ic=0;
		info->fw_main_version_of_ic=0;
	} else
		fts_get_version_info(info);

	if ((rc = fts_fw_update_on_probe(info)) < 0) {
		if (rc != -2)
			tsp_debug_err(true, info->dev, "%s: Failed to firmware update\n",
				__func__);
	}

	info->touch_count = 0;

	fts_command(info, SLEEPOUT);

	fts_command(info, SENSEON);

#ifdef FTS_SUPPORT_TOUCH_KEY
		info->fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif

#ifdef FTS_SUPPORT_NOISE_PARAM
	fts_get_noise_param_address(info);
#endif

	info->hover_enabled = false;
	info->hover_ready = false;
	info->slow_report_rate = false;
	info->flip_enable = false;

#ifdef FTS_SUPPORT_TOUCH_KEY
	info->tsp_keystatus = 0x00;
#endif // FTS_SUPPORT_TOUCH_KEY

#ifdef SEC_TSP_FACTORY_TEST
	rc = getChannelInfo(info);
	if (rc >= 0) {
		tsp_debug_info(true, &info->client->dev, "FTS Sense(%02d) Force(%02d)\n",
		       info->SenseChannelLength, info->ForceChannelLength);
	} else {
		tsp_debug_info(true, &info->client->dev, "FTS read failed rc = %d\n", rc);
		tsp_debug_info(true, &info->client->dev, "FTS Initialise Failed\n");
		return 1;
	}
	info->pFrame =
	    kzalloc(info->SenseChannelLength * info->ForceChannelLength * 2,
		    GFP_KERNEL);
	if (info->pFrame == NULL) {
		tsp_debug_info(true, &info->client->dev, "FTS pFrame kzalloc Failed\n");
		return 1;
	}
#endif

	fts_command(info, FORCECALIBRATION);
	fts_command(info, FLUSHBUFFER);

	fts_interrupt_set(info, INT_ENABLE);

	memset(val, 0x0, 4);
	regAdd[0] = READ_STATUS;
	fts_read_reg(info, regAdd, 1, (unsigned char *)val, 4);
	tsp_debug_info(true, &info->client->dev, "FTS ReadStatus(0x84) : %02X %02X %02X %02X\n", val[0],
	       val[1], val[2], val[3]);

	MutualTouchMode = false;

	tsp_debug_info(true, &info->client->dev, "FTS Initialized\n");

	return 0;
}

static void fts_unknown_event_handler(struct fts_ts_info *info,
				      unsigned char data[])
{
	tsp_debug_dbg(true, &info->client->dev,
	       "FTS Unknown Event %02X %02X %02X %02X %02X %02X %02X %02X\n",
	       data[0], data[1], data[2], data[3], data[4], data[5], data[6],
	       data[7]);
}

static unsigned char fts_event_handler_type_b(struct fts_ts_info *info,
					      unsigned char data[],
					      unsigned char LeftEvent)
{
	unsigned char EventNum = 0;
	unsigned char NumTouches = 0;
	unsigned char TouchID = 0, EventID = 0;
	unsigned char LastLeftEvent = 0;
	int x = 0, y = 0, z = 0;
	int bw = 0, bh = 0, palm = 0;
#if defined(CONFIG_SEC_S_PROJECT)
 	int sumsize = 0;	
#else
	int angle = 0;
#endif

#ifdef FTS_SUPPORT_TOUCH_KEY
	unsigned char change_keys;
	unsigned char key_state;
	unsigned char input_keys;
#endif // FTS_SUPPORT_TOUCH_KEY

	for (EventNum = 0; EventNum < LeftEvent; EventNum++) {

		/*tsp_debug_info(true, &info->client->dev, "%d %2x %2x %2x %2x %2x %2x %2x %2x\n", EventNum,
		   data[EventNum * FTS_EVENT_SIZE],
		   data[EventNum * FTS_EVENT_SIZE+1],
		   data[EventNum * FTS_EVENT_SIZE+2],
		   data[EventNum * FTS_EVENT_SIZE+3],
		   data[EventNum * FTS_EVENT_SIZE+4],
		   data[EventNum * FTS_EVENT_SIZE+5],
		   data[EventNum * FTS_EVENT_SIZE+6],
		   data[EventNum * FTS_EVENT_SIZE+7]); */

		EventID = data[EventNum * FTS_EVENT_SIZE] & 0x0F;

		if ((EventID >= 3) && (EventID <= 5)) {
			LastLeftEvent = 0;
			NumTouches = 1;

			TouchID = (data[EventNum * FTS_EVENT_SIZE] >> 4) & 0x0F;
		} else {
			LastLeftEvent =
			    data[7 + EventNum * FTS_EVENT_SIZE] & 0x0F;
			NumTouches =
			    (data[1 + EventNum * FTS_EVENT_SIZE] & 0xF0) >> 4;
			TouchID = data[1 + EventNum * FTS_EVENT_SIZE] & 0x0F;
			EventID = data[EventNum * FTS_EVENT_SIZE] & 0xFF;
		}
		//printk("\nTN10: %s called, EventID = %x\n", __func__,EventID);
		switch (EventID) {
		case EVENTID_NO_EVENT:
			break;
#ifdef FTS_SUPPORT_TOUCH_KEY
		case EVENTID_MSKEY:
			input_keys = data[2 + EventNum * FTS_EVENT_SIZE];

			if (input_keys == 0x00) // Release all
			{
					fts_release_all_key(info);
			}
			else {
				change_keys = input_keys ^ info->tsp_keystatus;
				printk("change_keys = %x,input_keys = %x,info->tsp_keystatus= %x\n", change_keys, input_keys, info->tsp_keystatus);
				printk("info->ignore_menu_key = %x,info->ignore_menu_key_by_back = %x\n", info->ignore_menu_key, info->ignore_menu_key_by_back);
				// Check menu key
				if (change_keys & TOUCH_KEY_RECENT)
				{
					key_state = input_keys & TOUCH_KEY_RECENT;

					if(info->ignore_menu_key) {
						tsp_debug_info(true, &info->client->dev, "[TSP_KEY] Ignore menu %s by dummy key\n", key_state != 0 ? "P" : "R");
						if(!(input_keys & TOUCH_KEY_D_MENU)){
							info->ignore_menu_key = false;
							tsp_debug_info(true, &info->client->dev, "[TSP_KEY] ignore_menu_key Disable!!\n");
						}
					} else if (info->ignore_menu_key_by_back) {
						tsp_debug_info(true, &info->client->dev, "[TSP_KEY] Ignore menu %s by back key\n", key_state != 0 ? "P" : "R");
					} else {

						input_report_key(info->input_dev, KEY_RECENT, key_state != 0 ? KEY_PRESS : KEY_RELEASE);
						tsp_debug_info(true, &info->client->dev, "[TSP_KEY] Recent %s\n", key_state != 0 ? "P" : "R");

						if (key_state != 0)
							info->ignore_back_key_by_menu = true;
						else {
							if (input_keys & TOUCH_KEY_D_MENU && !info->ignore_menu_key) {
								info->ignore_menu_key = true;
								tsp_debug_info(true, &info->client->dev, "[TSP_KEY] ignore_menu_key Enable!\n");
							}
							info->ignore_back_key_by_menu = false;
						}
					}
				}

				// Check back key
				if (change_keys & TOUCH_KEY_BACK)
				{
					key_state = input_keys & TOUCH_KEY_BACK;

					if (info->ignore_back_key) {
						tsp_debug_info(true, &info->client->dev, "[TSP_KEY] Ignore back %s by dummy key\n", key_state != 0 ? "P" : "R");
						if(!(input_keys & TOUCH_KEY_D_BACK)){
							info->ignore_back_key = false;
							tsp_debug_info(true, &info->client->dev, "[TSP_KEY] ignore_back_key Disable!!\n");
						}
					} else if (info->ignore_back_key_by_menu) {
						tsp_debug_info(true, &info->client->dev, "[TSP_KEY] Ignore menu %s by menu key\n", key_state != 0 ? "P" : "R");
					} else {
						input_report_key(info->input_dev, KEY_BACK, key_state != 0 ? KEY_PRESS : KEY_RELEASE);
						tsp_debug_info(true, &info->client->dev, "[TSP_KEY] back %s\n" , key_state != 0 ? "P" : "R");

						if (key_state != 0)
							info->ignore_menu_key_by_back = true;
						else {
							if (input_keys & TOUCH_KEY_D_BACK && !info->ignore_back_key) {
								info->ignore_back_key = true;
								tsp_debug_info(true, &info->client->dev, "[TSP_KEY] ignore_back_key Enable!\n");
							}
							info->ignore_menu_key_by_back = false;
						}
					}
				}

				// Check dummy menu key
				if (change_keys & TOUCH_KEY_D_MENU) {
					key_state = input_keys & TOUCH_KEY_D_MENU;

					if (info->report_dummy_key){
						input_report_key(info->input_dev, KEY_DUMMY_MENU, key_state != 0 ? KEY_PRESS : KEY_RELEASE);
						
						printk("\n Inside DUMMY MENU KEY report \n");
						tsp_debug_info(true, &info->client->dev, "[TSP_KEY] back %s\n" , key_state != 0 ? "P" : "R");
						}

					else if((key_state != 0) && !info->ignore_menu_key && !(input_keys & TOUCH_KEY_RECENT)) {
						info->ignore_menu_key = true;
						tsp_debug_info(true, &info->client->dev, "[TSP_KEY] ignore_menu_key Enable\n");
					} else if (!key_state && info->ignore_menu_key && !(input_keys & TOUCH_KEY_RECENT)) {
						info->ignore_menu_key = false;
						tsp_debug_info(true, &info->client->dev, "[TSP_KEY] ignore_menu_key Disable\n");
					}
				}

				// Check back key check
				if (change_keys & TOUCH_KEY_D_BACK) {
					key_state = input_keys & TOUCH_KEY_D_BACK;
					if (info->report_dummy_key){
						input_report_key(info->input_dev, KEY_DUMMY_BACK, key_state != 0 ? KEY_PRESS : KEY_RELEASE);
						printk("\n InsideDUMMY BACK KEY report \n");
						tsp_debug_info(true, &info->client->dev, "[TSP_KEY] back %s\n" , key_state != 0 ? "P" : "R");
						}
					else if((key_state != 0) && !info->ignore_back_key && !(input_keys & TOUCH_KEY_BACK)) {
						info->ignore_back_key = true;
						tsp_debug_info(true, &info->client->dev, "[TSP_KEY] ignore_back_key Enable\n");
					} else if (!key_state && info->ignore_back_key && !(input_keys & TOUCH_KEY_BACK)) {
						info->ignore_back_key = false;
						tsp_debug_info(true, &info->client->dev, "[TSP_KEY] ignore_back_key Disable\n");
					}
				}

				input_sync(info->input_dev);
			}

			info->tsp_keystatus = input_keys;
			break;
#endif // FTS_SUPPORT_TOUCH_KEY
		case EVENTID_ERROR:
			if (data[1 + EventNum * FTS_EVENT_SIZE] == 0x08) { // Get Auto tune fail event
				if (data[2 + EventNum * FTS_EVENT_SIZE] == 0x00) {
					tsp_debug_info(true, &info->client->dev, "[FTS] Fail Mutual Auto tune\n");
				}
				else if (data[2 + EventNum * FTS_EVENT_SIZE] == 0x01) {
					tsp_debug_info(true, &info->client->dev, "[FTS] Fail Self Auto tune\n");
				}
			}
			break;

		case EVENTID_HOVER_ENTER_POINTER:
		case EVENTID_HOVER_MOTION_POINTER:
			x = ((data[4 + EventNum * FTS_EVENT_SIZE] & 0xF0) >> 4)
			    | ((data[2 + EventNum * FTS_EVENT_SIZE]) << 4);
			y = ((data[4 + EventNum * FTS_EVENT_SIZE] & 0x0F) |
			     ((data[3 + EventNum * FTS_EVENT_SIZE]) << 4));

			z = data[5 + EventNum * FTS_EVENT_SIZE];

			input_mt_slot(info->input_dev, 0);
			input_mt_report_slot_state(info->input_dev,
						   MT_TOOL_FINGER, 1);

			input_report_key(info->input_dev, BTN_TOUCH, 0);
			input_report_key(info->input_dev, BTN_TOOL_FINGER, 1);

			input_report_abs(info->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(info->input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(info->input_dev, ABS_MT_DISTANCE, 255 - z);
			break;

		case EVENTID_HOVER_LEAVE_POINTER:
			input_mt_slot(info->input_dev, 0);
			input_mt_report_slot_state(info->input_dev,
						   MT_TOOL_FINGER, 0);
			break;

		case EVENTID_ENTER_POINTER:
			info->touch_count++;
		case EVENTID_MOTION_POINTER:
			x = data[1 + EventNum * FTS_EVENT_SIZE] +
			    ((data[2 + EventNum * FTS_EVENT_SIZE] &
			      0x0f) << 8);
			y = ((data[2 + EventNum * FTS_EVENT_SIZE] &
			      0xf0) >> 4) + (data[3 +
						  EventNum *
						  FTS_EVENT_SIZE] << 4);
			bw = data[4 + EventNum * FTS_EVENT_SIZE];
			bh = data[5 + EventNum * FTS_EVENT_SIZE];

#if defined(CONFIG_SEC_S_PROJECT)
			palm = (data[6 + EventNum * FTS_EVENT_SIZE] >> 7) & 0x01;
			sumsize = (data[6 + EventNum * FTS_EVENT_SIZE] & 0x7f) << 1;
#else
			angle = (data[6 + EventNum * FTS_EVENT_SIZE] & 0x7f) << 1;
			if (angle & 0x80)
				angle |= 0xffffff00;
			palm =(data[6 + EventNum * FTS_EVENT_SIZE] >> 7) & 0x01;
#endif
			z = data[7 + EventNum * FTS_EVENT_SIZE];

			input_mt_slot(info->input_dev, TouchID);
			input_mt_report_slot_state(info->input_dev,
						   MT_TOOL_FINGER,
						   1 + (palm << 1));

			input_report_key(info->input_dev, BTN_TOUCH, 1);
			input_report_key(info->input_dev,
					 BTN_TOOL_FINGER, 1);
			input_report_abs(info->input_dev,
					 ABS_MT_POSITION_X, x);
			input_report_abs(info->input_dev,
					 ABS_MT_POSITION_Y, y);

			input_report_abs(info->input_dev,
					 ABS_MT_TOUCH_MAJOR, max(bw,
								 bh));

			input_report_abs(info->input_dev,
					 ABS_MT_TOUCH_MINOR, min(bw,
								 bh));
			input_report_abs(info->input_dev,
					 ABS_MT_WIDTH_MAJOR, z);

			input_report_abs(info->input_dev, ABS_MT_PALM,
					 palm);

			break;

		case EVENTID_LEAVE_POINTER:
			info->touch_count--;

			input_mt_slot(info->input_dev, TouchID);
			input_mt_report_slot_state(info->input_dev,
						   MT_TOOL_FINGER, 0);

			if (info->touch_count == 0) {
				/* Clear BTN_TOUCH when All touch are released  */
				input_report_key(info->input_dev, BTN_TOUCH, 0);
			}
			break;
		case EVENTID_STATUS_EVENT:
			if (data[1 + EventNum * FTS_EVENT_SIZE] == 0x0C) {
#ifdef CONFIG_GLOVE_TOUCH
				int tm;
				if (data[2 + EventNum * FTS_EVENT_SIZE] == 0x01)
					info->touch_mode = FTS_TM_GLOVE;
				else
					info->touch_mode = FTS_TM_NORMAL;

				tm = info->touch_mode;
				input_report_switch(info->input_dev, SW_GLOVE, tm);
#endif
			} else if ((data[1 + EventNum * FTS_EVENT_SIZE] & 0x0f) == 0x0d) {
				unsigned char regAdd[4] = {0xB0, 0x01, 0x29, 0x01};
				fts_write_reg(info, &regAdd[0], 4);

				info->hover_ready = true;

				tsp_debug_info(true, &info->client->dev, "[FTS] Received the Hover Raw Data Ready Event\n");
			} else {
				fts_unknown_event_handler(info,
						  &data[EventNum *
							FTS_EVENT_SIZE]);
			}
			break;

#ifdef SEC_TSP_FACTORY_TEST
		case EVENTID_RESULT_READ_REGISTER:
			procedure_cmd_event(info, &data[EventNum * FTS_EVENT_SIZE]);
			break;
#endif

		default:
			fts_unknown_event_handler(info,
						  &data[EventNum *
							FTS_EVENT_SIZE]);
			continue;
		}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		if (EventID == EVENTID_ENTER_POINTER)
#if defined(CONFIG_SEC_S_PROJECT)			
			tsp_debug_info(true, &info->client->dev,
			       "[P] tID:%d x:%d y:%d w:%d h:%d z:%d s:%d p:%d tc:%d tm:%d\n",
			       TouchID, x, y, bw, bh, z, sumsize, palm, info->touch_count, info->touch_mode);
#else
			tsp_debug_info(true, &info->client->dev,
			       "[P] tID:%d x:%d y:%d w:%d h:%d z:%d a:%d p:%d tc:%d tm:%d\n",
			       TouchID, x, y, bw, bh, z, angle, palm, info->touch_count, info->touch_mode);
#endif
		else if (EventID == EVENTID_HOVER_ENTER_POINTER)
			tsp_debug_dbg(true, &info->client->dev,
				"[HP] tID:%d x:%d y:%d z:%d\n",
				TouchID, x, y, z);
#else
		if (EventID == EVENTID_ENTER_POINTER)
			tsp_debug_info(true, &info->client->dev,
			       "[P] tID:%d tc:%d tm:%d\n",
			       TouchID, info->touch_count, info->touch_mode);
		else if (EventID == EVENTID_HOVER_ENTER_POINTER)
			tsp_debug_dbg(true, &info->client->dev,
				"[HP] tID:%d\n", TouchID);
#endif
		else if (EventID == EVENTID_LEAVE_POINTER) {
			tsp_debug_info(true, &info->client->dev,
			       "[R] tID:%d mc: %d tc:%d Ver[%02X%04X%01X%01X]\n",
			       TouchID, info->finger[TouchID].mcount, info->touch_count,
			       info->panel_revision, info->fw_main_version_of_ic,
			       info->flip_enable, info->mshover_enabled);
			info->finger[TouchID].mcount = 0;
		} else if (EventID == EVENTID_HOVER_LEAVE_POINTER) {
			tsp_debug_dbg(true, &info->client->dev,
			       "[HR] tID:%d Ver[%02X%04X%01X%01X]\n",
			       TouchID,
			       info->panel_revision, info->fw_main_version_of_ic,
			       info->flip_enable, info->mshover_enabled);
			info->finger[TouchID].mcount = 0;
		} else if (EventID == EVENTID_MOTION_POINTER)
			info->finger[TouchID].mcount++;

		if ((EventID == EVENTID_ENTER_POINTER) ||
			(EventID == EVENTID_MOTION_POINTER) ||
			(EventID == EVENTID_LEAVE_POINTER))
			info->finger[TouchID].state = EventID;
	}

	input_sync(info->input_dev);

#ifdef TOUCH_BOOSTER_DVFS
	if ((EventID == EVENTID_ENTER_POINTER)
	    || (EventID == EVENTID_LEAVE_POINTER)) {
			fts_set_dvfs_lock(info, info->touch_count);
	}
#endif
	return LastLeftEvent;
}

#ifdef FTS_SUPPORT_TA_MODE
static void fts_ta_cb(struct fts_callbacks *cb, int ta_status)
{
	struct fts_ts_info *info =
	    container_of(cb, struct fts_ts_info, callbacks);

	pr_err("[TSP]%s: ta:%d\n",	__func__, ta_status);

	if (ta_status == 0x01 || ta_status == 0x03) {
		fts_command(info, FTS_CMD_CHARGER_PLUGGED);
		info->TA_Pluged = true;
		tsp_debug_info(true, &info->client->dev,
			 "%s: device_control : CHARGER CONNECTED, ta_status : %x\n",
			 __func__, ta_status);
	} else {
		fts_command(info, FTS_CMD_CHARGER_UNPLUGGED);
		info->TA_Pluged = false;
		tsp_debug_info(true, &info->client->dev,
			 "%s: device_control : CHARGER DISCONNECTED, ta_status : %x\n",
			 __func__, ta_status);
	}
}
#endif


 /**
 * fts_interrupt_handler()
 *
 * Called by the kernel when an interrupt occurs (when the sensor
 * asserts the attention irq).
 *
 * This function is the ISR thread and handles the acquisition
 * and the reporting of finger data when the presence of fingers
 * is detected.
 */
static irqreturn_t fts_interrupt_handler(int irq, void *handle)
{
	struct fts_ts_info *info = handle;
	unsigned char regAdd[4] = {0xb6, 0x00, 0x45, READ_ALL_EVENT};
	unsigned short evtcount = 0;

	evtcount = 0;
	fts_read_reg(info, &regAdd[0], 3, (unsigned char *)&evtcount, 2);
	evtcount = evtcount >> 10;

	if (evtcount > FTS_FIFO_MAX)
		evtcount = FTS_FIFO_MAX;

	if (evtcount > 0) {
		memset(info->data, 0x0, FTS_EVENT_SIZE * evtcount);
		fts_read_reg(info, &regAdd[3], 1, (unsigned char *)info->data,
				  FTS_EVENT_SIZE * evtcount);
		fts_event_handler_type_b(info, info->data, evtcount);
	}

	return IRQ_HANDLED;
}

static int fts_irq_enable(struct fts_ts_info *info,
		bool enable)
{
	int retval = 0;

	if (enable) {
		if (info->irq_enabled)
			return retval;

		retval = request_threaded_irq(info->irq, NULL,
				fts_interrupt_handler, info->board->irq_type,
				FTS_TS_DRV_NAME, info);
		if (retval < 0) {
			tsp_debug_info(true, &info->client->dev,
					"%s: Failed to create irq thread %d\n",
					__func__, retval);
			return retval;
		}

		info->irq_enabled = true;
	} else {
		if (info->irq_enabled) {
			disable_irq(info->irq);
			free_irq(info->irq, info);
			info->irq_enabled = false;
		}
	}

	return retval;
}

#ifdef CONFIG_OF
u32 gpio_ldo_en_p = 31;
struct regulator *i2c_vddo_vreg = NULL;

#if defined(CONFIG_SEC_T10_PROJECT)
struct regulator *reg_l23 = NULL;
#endif

#if defined(CONFIG_SEC_S_PROJECT)
struct regulator_bulk_data *fts_supplies;
#endif

int fts_vdd_on(bool onoff)
{
	pr_err("[TSP]%s: gpio:%d, vdd en:%d\n",	__func__, gpio_ldo_en_p, onoff);


	#if defined(CONFIG_SEC_S_PROJECT)
	{
		int retval;
		if(onoff){
			if (regulator_is_enabled(fts_supplies[0].consumer)) {
				pr_err("%s: %s is already enabled\n", __func__, fts_supplies[0].supply);
			}else {
				retval = regulator_enable(fts_supplies[0].consumer);
				if (retval) {
					pr_err("%s: Fail to enable regulator %s[%d]\n",	__func__, fts_supplies[0].supply, retval);
				}
				pr_err("%s: %s is enabled[OK]\n", __func__, fts_supplies[0].supply);
			}
		}else{
			if (regulator_is_enabled(fts_supplies[0].consumer)) {
				retval = regulator_disable(fts_supplies[0].consumer);
				if (retval) {
					pr_err("%s: Fail to disable regulator %s[%d]\n",	__func__, fts_supplies[0].supply, retval);
				}
				pr_err("%s: %s is disabled[OK]\n", __func__, fts_supplies[0].supply);
			}else {
				pr_err("%s: %s is already disabled\n", __func__, fts_supplies[0].supply);
			}

		}
	}
	#endif

	#if defined(CONFIG_SEC_T10_PROJECT)
		if(reg_l23 != NULL){
			if(onoff){
				regulator_enable(reg_l23);
			}else{
				regulator_disable(reg_l23);
			}
		}else{
			pr_err("[TSP]%s: reg_l23 is null :%d\n",	__func__, onoff);
		}
	#else
		if(i2c_vddo_vreg != NULL){
			if(onoff){
				regulator_enable(i2c_vddo_vreg);
			}else{
				regulator_disable(i2c_vddo_vreg);
			}
		}else{
			pr_err("[TSP]%s: i2c_vddo_vreg is null  vdd en:%d\n",	__func__, onoff);
		}
	#endif
	
	if(gpio_ldo_en_p > 0){	
	gpio_direction_output(gpio_ldo_en_p, onoff);
	}	
	msleep(50);
	return 1;
}
void fts_init_gpio(struct fts_ts_info *info, struct fts_ts_platform_data *pdata)
{
	int ret;
	pr_err("[TSP] %s, %d \n",__func__, __LINE__ );

#if defined(CONFIG_SEC_S_PROJECT)
	if(pdata->tsp_id > 0)
		gpio_tlmm_config(GPIO_CFG(pdata->tsp_id, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(pdata->scl_gpio, 3, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(pdata->sda_gpio, 3, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);	
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_int, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
#else
	ret = gpio_request(pdata->gpio_int, "fts_tsp_irq");
	if(ret) {
		tsp_debug_err(true, &info->client->dev, "[TSP]%s: unable to request irq [%d]\n",	__func__, pdata->gpio_int);
		return;
	}
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_int, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
#endif

	if(pdata->gpio_ldo_en > 0){	
	ret = gpio_request(pdata->gpio_ldo_en, "fts_gpio_ldo_en");
	if(ret) {
		tsp_debug_err(true, &info->client->dev, "[TSP]%s: unable to request fts_gpio_ldo_en [%d]\n",	__func__, pdata->gpio_ldo_en);
		return;
	}
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_ldo_en, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}

#if defined(CONFIG_SEC_T10_PROJECT)
	ret = gpio_request(pdata->tsp_vendor1, "tsp_vendor1");
	if(ret) {
		tsp_debug_err(true, &info->client->dev, "[TSP]%s: unable to request tsp_vendor1 [%d]\n",	__func__, pdata->tsp_vendor1);
		return;
	}
	gpio_tlmm_config(GPIO_CFG(pdata->tsp_vendor1,0,GPIO_CFG_INPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	
	ret = gpio_request(pdata->tsp_vendor2, "tsp_vendor2");
		if(ret) {
			tsp_debug_err(true, &info->client->dev, "[TSP]%s: unable to request tsp_vendor2 [%d]\n",	__func__, pdata->tsp_vendor2);
			return;
		}
	gpio_tlmm_config(GPIO_CFG(pdata->tsp_vendor2,0,GPIO_CFG_INPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif

}
static int fts_parse_dt(struct device *dev, struct fts_ts_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	//u32 temp;

	pdata->gpio_ldo_en = of_get_named_gpio(np, "fts,vdd_en-gpio", 0);
	pdata->gpio_int = of_get_named_gpio(np, "fts,irq-gpio", 0);
#if defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
	pdata->oled_id = of_get_named_gpio(np, "fts,oledid-gpio", 0);
	printk(KERN_INFO "[TSP]%s: en_vdd:%d, irq:%d, oledid_gpio:%d\n",__func__, pdata->gpio_ldo_en, pdata->gpio_int, pdata->oled_id);
#else
	printk(KERN_INFO "[TSP]%s: en_vdd:%d, irq:%d \n",	__func__, pdata->gpio_ldo_en, pdata->gpio_int);
#endif

#if defined(CONFIG_SEC_T10_PROJECT)
	pdata->tsp_vendor1 = of_get_named_gpio(np, "fts,vendor1", 0);
	pdata->tsp_vendor2 = of_get_named_gpio(np, "fts,vendor2", 0);
	printk(KERN_INFO "[TSP]%s: tsp_vendor1:%d, tsp_vendor2:%d \n",	__func__, pdata->tsp_vendor1, pdata->tsp_vendor2);
#endif


#if defined(CONFIG_SEC_S_PROJECT)
	pdata->tsp_id = of_get_named_gpio(np, "fts,id-gpio", 0);
	pdata->scl_gpio = of_get_named_gpio(np, "fts,scl-gpio", 0);
	pdata->sda_gpio = of_get_named_gpio(np, "fts,sda-gpio", 0);

	pdata->gpio_ldo_en = 0;
	{
		int rc;
		pdata->name_of_supply = kzalloc(sizeof(char *), GFP_KERNEL);
		rc = of_property_read_string(np, "fts,supply-name", &pdata->name_of_supply);
		if (rc && (rc != -EINVAL)) {
			printk(KERN_INFO "%s: Unable to read %s\n", __func__,"fts,supply-name");
			return rc;
		}
		dev_info(dev, "%s: supply: %s\n", __func__, pdata->name_of_supply);
	}

#endif


	return 0;
}

#endif

#ifdef READ_LCD_ID
static int lcd_id = 0;
static int fts_lcd_id;
static int __init fts_read_lcd_id(char *mode)
{
	char *pt;

	lcd_id = 0;
	if( mode == NULL ) return 1;
	for( pt = mode; *pt != 0; pt++ )
	{
		lcd_id <<= 4;
		switch(*pt)
		{
			case '0' ... '9' :
				lcd_id += *pt -'0';
			break;
			case 'a' ... 'f' :
				lcd_id += 10 + *pt -'a';
			break;
			case 'A' ... 'F' :
				lcd_id += 10 + *pt -'A';
			break;
		}
	}
	fts_lcd_id = lcd_id;

	pr_info( "%s: LCD_ID = 0x%X\n", __func__,fts_lcd_id);

	return 0;
}
__setup("lcd_id=0x", fts_read_lcd_id);
#endif

static int fts_probe(struct i2c_client *client, const struct i2c_device_id *idp)
{
	int retval;
	struct fts_ts_info *info = NULL;
#ifdef CONFIG_OF
	static struct fts_i2c_platform_data *board_p;
	struct fts_ts_platform_data *pdata;
#endif
	static char fts_ts_phys[64] = { 0 };
	int i = 0;
#ifdef SEC_TSP_FACTORY_TEST
	int ret;
#endif
#ifdef FTS_SUPPORT_TOUCH_KEY
	struct device *touchkey;
#endif
	tsp_debug_info(true, &client->dev, "FTS Driver [12%s] %s %s\n",
	       FTS_TS_DRV_VERSION, __DATE__, __TIME__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		tsp_debug_err(true, &client->dev, "FTS err = EIO!\n");
		return -EIO;
	}

#ifdef CONFIG_OF
	
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct fts_ts_platform_data), GFP_KERNEL);
		if (!pdata) {
			tsp_debug_err(true, &client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
	
		ret = fts_parse_dt(&client->dev, pdata);
		
		if (ret) {
			tsp_debug_err(true, &client->dev, "Error parsing dt %d\n", ret);				
			return ret;
		}
		
		fts_init_gpio(info, pdata);
		gpio_ldo_en_p = pdata->gpio_ldo_en;
		
	}else{
		tsp_debug_err(true, &client->dev, "%s, of-node error %d\n",__func__,__LINE__);
		return -ENOMEM;
	}
#endif

	info = kzalloc(sizeof(struct fts_ts_info), GFP_KERNEL);
	if (!info) {
		tsp_debug_err(true, &client->dev, "FTS err = ENOMEM!\n");
		return -ENOMEM;
	}

#ifdef USE_OPEN_DWORK
	INIT_DELAYED_WORK(&info->open_work, fts_open_work);
#endif

#ifdef CONFIG_OF
	info->client = client;
	info->pdata = pdata;

	//info->board = client->dev.platform_data;

	board_p = kzalloc(sizeof(struct fts_i2c_platform_data), GFP_KERNEL);
	if (!board_p) {
		tsp_debug_err(true, &client->dev, "board_p err = ENOMEM!\n");
		return -ENOMEM;
	}

#if defined(CONFIG_SEC_T10_PROJECT)
	board_p->max_x = 1280;
	board_p->max_y = 800;
	board_p->project_name = "T54x";
#else
	board_p->max_x = 720;
	board_p->max_y = 1280;
	board_p->project_name = "N750X";
#endif

	board_p->max_width = 28;
	board_p->support_hover = true;
	board_p->support_mshover = true;

#ifdef FTS_SUPPORT_TOUCH_KEY
	board_p->num_touchkey = ARRAY_SIZE(fts_touchkeys);
	board_p->touchkey = fts_touchkeys;
#endif // FTS_SUPPORT_TOUCH_KEY

#ifdef READ_LCD_ID
	info->lcd_id = fts_lcd_id;
	printk(KERN_ERR "%s: tsp : lcd_id : 0x%02X\n", __func__, info->lcd_id);
#endif

#if defined(CONFIG_SEC_T10_PROJECT)
	board_p->firmware_name = "tsp_stm/stm_tn10.fw";
#elif defined(CONFIG_SEC_S_PROJECT)
	board_p->firmware_name = "tsp_stm/stm_s.fw";
#elif !defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
	board_p->firmware_name = "tsp_stm/stm.fw";
#else
#ifdef CONFIG_SEC_FACTORY
	board_p->firmware_name = "tsp_stm/stm_sync.fw";
#else
	if((info->lcd_id != 0x401043) && (info->lcd_id != 0x401080))
		board_p->firmware_name = "tsp_stm/stm_sync.fw";
	else
		board_p->firmware_name = "tsp_stm/stm.fw";
#endif
#endif

	board_p->power = fts_vdd_on;
	board_p->irq_type = IRQF_TRIGGER_LOW | IRQF_ONESHOT;
	board_p->gpio = pdata->gpio_int; //GPIO_TSP_INT;

	info->board = board_p;

#endif

	if (info->board->support_hover) {
		tsp_debug_info(true, &info->client->dev, "FTS Support Hover Event \n");
	} else {
		tsp_debug_info(true, &info->client->dev, "FTS Not support Hover Event \n");
	}

#if defined(CONFIG_OF) && !defined(CONFIG_SEC_T10_PROJECT)
	i2c_vddo_vreg = regulator_get(&info->client->dev,"vddo");
	if (IS_ERR(i2c_vddo_vreg)){
		i2c_vddo_vreg = NULL;
		pr_err("[TSP]%s: i2c_vddo_vreg is error , %d \n", __func__, __LINE__);
	}
#endif

#if defined(CONFIG_SEC_S_PROJECT)
	fts_supplies = kzalloc(sizeof(struct regulator_bulk_data), GFP_KERNEL);
	if (!fts_supplies) {
		pr_err("%s: Failed to alloc mem for supplies\n", __func__);
		retval = -ENOMEM;

		goto err_input_allocate_device;
	}
	fts_supplies[0].supply = pdata->name_of_supply;

	retval = regulator_bulk_get(&client->dev, 1, fts_supplies);
	if (retval)
		goto err_get_regulator;

#endif

#if defined(CONFIG_SEC_T10_PROJECT)
	reg_l23 = regulator_get(&info->client->dev,"vcc_en");
	if (IS_ERR(reg_l23)){
		reg_l23 = NULL;
		pr_err("[TSP]%s: reg_l23 is error , %d \n", __func__, __LINE__);
	}
#endif
	if (info->board->power)
		info->board->power(true);

	info->dev = &info->client->dev;
	info->input_dev = input_allocate_device();
	if (!info->input_dev) {
		tsp_debug_info(true, &info->client->dev, "FTS err = ENOMEM!\n");
		retval = -ENOMEM;
		goto err_input_allocate_device;
	}

	info->input_dev->dev.parent = &client->dev;
	info->input_dev->name = "sec_touchscreen";
	snprintf(fts_ts_phys, sizeof(fts_ts_phys), "%s/input1",
		 info->input_dev->name);
	info->input_dev->phys = fts_ts_phys;
	info->input_dev->id.bustype = BUS_I2C;

	client->irq = gpio_to_irq(pdata->gpio_int);
	printk(KERN_ERR "%s: tsp : gpio_to_irq : %d\n", __func__, client->irq);

	info->irq = client->irq;
	info->irq_type = info->board->irq_type;
	info->irq_enabled = false;

	info->touch_stopped = false;
	info->panel_revision = info->board->panel_revision;
	info->stop_device = fts_stop_device;
	info->start_device = fts_start_device;
	info->fts_command = fts_command;
	info->fts_read_reg = fts_read_reg;
	info->fts_write_reg = fts_write_reg;
	info->fts_systemreset = fts_systemreset;
	info->fts_get_version_info = fts_get_version_info;
	info->fts_wait_for_ready = fts_wait_for_ready;

#ifdef FTS_SUPPORT_NOISE_PARAM
	info->fts_get_noise_param_address = fts_get_noise_param_address;
#endif


#ifdef USE_OPEN_CLOSE
	info->input_dev->open = fts_input_open;
	info->input_dev->close = fts_input_close;
#endif

#ifdef TSP_INIT_COMPLETE
	init_completion(&info->init_done);
#endif

#ifdef CONFIG_GLOVE_TOUCH
	input_set_capability(info->input_dev, EV_SW, SW_GLOVE);
#endif
	set_bit(EV_SYN, info->input_dev->evbit);
	set_bit(EV_KEY, info->input_dev->evbit);
	set_bit(EV_ABS, info->input_dev->evbit);
#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, info->input_dev->propbit);
#endif

#ifdef FTS_SUPPORT_TOUCH_KEY
	for (i = 0 ; i < info->board->num_touchkey ; i++)
		set_bit(info->board->touchkey[i].keycode, info->input_dev->keybit);

	set_bit(EV_LED, info->input_dev->evbit);
	set_bit(LED_MISC, info->input_dev->ledbit);

#endif // FTS_SUPPORT_TOUCH_KEY

	set_bit(BTN_TOUCH, info->input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, info->input_dev->keybit);

	input_mt_init_slots(info->input_dev, FINGER_MAX);
	input_set_abs_params(info->input_dev, ABS_MT_POSITION_X,
			     0, info->board->max_x, 0, 0);
	input_set_abs_params(info->input_dev, ABS_MT_POSITION_Y,
			     0, info->board->max_y, 0, 0);

	mutex_init(&info->lock);
	mutex_init(&(info->device_mutex));
	mutex_init(&info->i2c_mutex);

	info->enabled = false;
	mutex_lock(&info->lock);
	retval = fts_init(info);
	mutex_unlock(&info->lock);
	if (retval) {
		tsp_debug_err(true, &info->client->dev, "FTS fts_init fail!\n");
		goto err_fts_init;
	}

	input_set_abs_params(info->input_dev, ABS_MT_TOUCH_MAJOR,
				 0, 255, 0, 0);
	input_set_abs_params(info->input_dev, ABS_MT_TOUCH_MINOR,
				 0, 255, 0, 0);
	input_set_abs_params(info->input_dev, ABS_MT_WIDTH_MAJOR,
				 0, 255, 0, 0);
	input_set_abs_params(info->input_dev, ABS_MT_PALM, 0, 1, 0, 0);
	input_set_abs_params(info->input_dev, ABS_MT_DISTANCE,
				 0, 255, 0, 0);

	input_set_drvdata(info->input_dev, info);
	i2c_set_clientdata(client, info);

	retval = input_register_device(info->input_dev);
	if (retval) {
		tsp_debug_err(true, &info->client->dev, "FTS input_register_device fail!\n");
		goto err_register_input;
	}

	for (i = 0; i < FINGER_MAX; i++) {
		info->finger[i].state = EVENTID_LEAVE_POINTER;
		info->finger[i].mcount = 0;
	}

	info->enabled = true;

	retval = fts_irq_enable(info, true);
	if (retval < 0) {
		tsp_debug_info(true, &info->client->dev,
						"%s: Failed to enable attention interrupt\n",
						__func__);
		goto err_enable_irq;
	}

#ifdef TOUCH_BOOSTER_DVFS
	fts_init_dvfs(info);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	info->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	info->early_suspend.suspend = fts_early_suspend;
	info->early_sfts_start_deviceuspend.resume = fts_late_resume;
	register_early_suspend(&info->early_suspend);
#endif

#ifdef FTS_SUPPORT_TA_MODE
	info->register_cb = fts_register_callback;

	info->callbacks.inform_charger = fts_ta_cb;
	if (info->register_cb)
		info->register_cb(&info->callbacks);
#endif

#ifdef SEC_TSP_FACTORY_TEST
	INIT_LIST_HEAD(&info->cmd_list_head);

	info->cmd_buffer_size = 0;
	for (i = 0; i < ARRAY_SIZE(ft_cmds); i++){
		list_add_tail(&ft_cmds[i].list, &info->cmd_list_head);
		if(ft_cmds[i].cmd_name)
			info->cmd_buffer_size += strlen(ft_cmds[i].cmd_name) + 1;
	}
	info->cmd_result = kzalloc(info->cmd_buffer_size, GFP_KERNEL);
	if(!info->cmd_result){
		tsp_debug_err(true, &info->client->dev, "FTS Failed to allocate cmd result\n");
		goto err_alloc_cmd_result;
	}

	mutex_init(&info->cmd_lock);
	info->cmd_is_running = false;

	info->fac_dev_ts = device_create(sec_class, NULL, FTS_ID0, info, "tsp");
	if (IS_ERR(info->fac_dev_ts)) {
		tsp_debug_err(true, &info->client->dev, "FTS Failed to create device for the sysfs\n");
		goto err_sysfs;
	}

	dev_set_drvdata(info->fac_dev_ts, info);

	ret = sysfs_create_group(&info->fac_dev_ts->kobj,
				 &sec_touch_factory_attr_group);
	if (ret < 0) {
		tsp_debug_err(true, &info->client->dev, "FTS Failed to create sysfs group\n");
		goto err_sysfs;
	}
#endif

#ifdef FTS_SUPPORT_TOUCH_KEY

	touchkey = device_create(sec_class,
			NULL, 0, info, "sec_touchkey");
	
		if (IS_ERR(touchkey))
			dev_err(&client->dev,
			"Failed to create device for the touchkey sysfs\n");
		
	dev_set_drvdata(touchkey, info);

	ret = sysfs_create_group(&touchkey->kobj,
			&sec_touchkey_attr_group);
		if (ret)
			dev_err(&client->dev, "Failed to create sysfs group\n");
#endif

	pr_err("[TSP] %s, end, %d \n",__func__, __LINE__ );

#ifdef TSP_INIT_COMPLETE

#ifdef USE_OPEN_CLOSE
	fts_stop_device(info);
#endif
	complete_all(&info->init_done);
#endif /* TSP_INIT_COMPLETE */

	return 0;

#ifdef SEC_TSP_FACTORY_TEST
err_sysfs:
	kfree(info->cmd_result);
err_alloc_cmd_result:

	if (info->irq_enabled)
		fts_irq_enable(info, false);
#endif

err_enable_irq:
	input_unregister_device(info->input_dev);
	info->input_dev = NULL;

err_register_input:
	if (info->input_dev)
		input_free_device(info->input_dev);

err_fts_init:
#ifdef TSP_INIT_COMPLETE
	complete_all(&info->init_done);
#endif
#if defined(CONFIG_SEC_S_PROJECT)
err_get_regulator:
	kfree(fts_supplies);
#endif
err_input_allocate_device:
	info->board->power(false);
	kfree(info);

	return retval;
}

static int fts_remove(struct i2c_client *client)
{
	struct fts_ts_info *info = i2c_get_clientdata(client);

	tsp_debug_info(true, &info->client->dev, "FTS removed \n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&info->early_suspend);
#endif

	fts_interrupt_set(info, INT_DISABLE);
	fts_command(info, FLUSHBUFFER);

	fts_irq_enable(info, false);

	input_mt_destroy_slots(info->input_dev);

#ifdef SEC_TSP_FACTORY_TEST
	sysfs_remove_group(&info->fac_dev_ts->kobj,
			   &sec_touch_factory_attr_group);

	device_destroy(sec_class, FTS_ID0);

	if (info->cmd_result)
		kfree(info->cmd_result);

	list_del(&info->cmd_list_head);

	mutex_destroy(&info->cmd_lock);

	if (info->pFrame)
		kfree(info->pFrame);
#endif

	mutex_destroy(&info->lock);

	input_unregister_device(info->input_dev);
	info->input_dev = NULL;

	info->board->power(false);

	kfree(info);

#if defined(CONFIG_SEC_S_PROJECT)
	kfree(fts_supplies);
#endif

	return 0;
}

#ifdef USE_OPEN_CLOSE
#ifdef USE_OPEN_DWORK
static void fts_open_work(struct work_struct *work)
{
	int retval;
	struct fts_ts_info *info = container_of(work, struct fts_ts_info,
						open_work.work);

	tsp_debug_info(true, &info->client->dev, "%s\n", __func__);

	retval = fts_start_device(info);
	if (retval < 0)
		tsp_debug_err(true, &info->client->dev,
			"%s: Failed to start device\n", __func__);
}
#endif
static int fts_input_open(struct input_dev *dev)
{
	struct fts_ts_info *info = input_get_drvdata(dev);
	int retval;

#ifdef TSP_INIT_COMPLETE
	retval = wait_for_completion_interruptible_timeout(&info->init_done,
							   msecs_to_jiffies(90 * MSEC_PER_SEC));

	if (retval < 0) {
		tsp_debug_err(true, &info->client->dev,
			"error while waiting for device to init (%d)\n",
			retval);
		retval = -ENXIO;
		goto err_open;
	}
	if (retval == 0) {
		tsp_debug_err(true, &info->client->dev,
			"timedout while waiting for device to init\n");
		retval = -ENXIO;
		goto err_open;
	}
#endif
	tsp_debug_dbg(false, &info->client->dev, "%s\n", __func__);


#ifdef USE_OPEN_DWORK
	schedule_delayed_work(&info->open_work,
			      msecs_to_jiffies(TOUCH_OPEN_DWORK_TIME));
#else
	retval = fts_start_device(info);
	if (retval < 0){
		tsp_debug_err(true, &info->client->dev,
			"%s: Failed to start device\n", __func__);
		goto out;
	}
#endif

	tsp_debug_err(true, &info->client->dev, "FTS cmd after wakeup : h%d\n",
	      retry_hover_enable_after_wakeup);

	if(retry_hover_enable_after_wakeup == 1){
		unsigned char regAdd[4] = {0xB0, 0x01, 0x29, 0x41};
		fts_write_reg(info, &regAdd[0], 4);
		fts_command(info, FTS_CMD_HOVER_ON);
		info->hover_ready = false;
		info->hover_enabled = true;
	}

out:
	return 0;

#ifdef TSP_INIT_COMPLETE
 err_open:
	return retval;
#endif
}

static void fts_input_close(struct input_dev *dev)
{
	struct fts_ts_info *info = input_get_drvdata(dev);

	tsp_debug_dbg(false, &info->client->dev, "%s\n", __func__);

#ifdef USE_OPEN_DWORK
	cancel_delayed_work(&info->open_work);
#endif

	fts_stop_device(info);

	retry_hover_enable_after_wakeup = 0;
}
#endif

#ifdef CONFIG_SEC_FACTORY
#include <linux/uaccess.h>
#define LCD_LDI_FILE_PATH	"/sys/class/lcd/panel/window_type"
static int fts_get_panel_revision(struct fts_ts_info *info)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *window_type;
	unsigned char lcdtype[4] = {0,};

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	window_type = filp_open(LCD_LDI_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(window_type)) {
		iRet = PTR_ERR(window_type);
		if (iRet != -ENOENT)
			tsp_debug_err(true, &info->client->dev, "%s: window_type file open fail\n", __func__);
		set_fs(old_fs);
		goto exit;
	}

	iRet = window_type->f_op->read(window_type, (u8 *)lcdtype, sizeof(u8) * 4, &window_type->f_pos);
	if (iRet != (sizeof(u8) * 4)) {
		tsp_debug_err(true, &info->client->dev, "%s: Can't read the lcd ldi data\n", __func__);
		iRet = -EIO;
	}

	/* The variable of lcdtype has ASCII values(40 81 45) at 0x08 OCTA,
	  * so if someone need a TSP panel revision then to read third parameter.*/
	info->panel_revision = lcdtype[3] & 0x0F;
	tsp_debug_info(true, &info->client->dev,
		"%s: update panel_revision 0x%02X\n", __func__, info->panel_revision);

	filp_close(window_type, current->files);
	set_fs(old_fs);

exit:
	return iRet;
}

static void fts_reinit_fac(struct fts_ts_info *info)
{
	int rc;

	info->touch_count = 0;

	fts_command(info, SLEEPOUT);
	fts_delay(50);

#if defined(CONFIG_SEC_S_PROJECT)
	fts_command(info, SENSEON);
	fts_delay(50);

	if (info->slow_report_rate)
		fts_command(info, FTS_CMD_SLOW_SCAN);
#else	
	if (info->slow_report_rate)
		fts_command(info, SENSEON_SLOW);
	else
		fts_command(info, SENSEON);
#endif

#ifdef FTS_SUPPORT_TOUCH_KEY
		info->fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif // FTS_SUPPORT_TOUCH_KEY

#ifdef FTS_SUPPORT_NOISE_PARAM
	fts_get_noise_param_address(info);
#endif

	if (info->flip_enable)
		fts_set_flipcover_mode(info, true);
	else if (info->fast_mshover_enabled)
		fts_command(info, FTS_CMD_SET_FAST_GLOVE_MODE);
	else if (info->mshover_enabled)
		fts_command(info, FTS_CMD_MSHOVER_ON);

	rc = getChannelInfo(info);
	if (rc >= 0) {
		tsp_debug_info(true, &info->client->dev, "FTS Sense(%02d) Force(%02d)\n",
		       info->SenseChannelLength, info->ForceChannelLength);
	} else {
		tsp_debug_info(true, &info->client->dev, "FTS read failed rc = %d\n", rc);
		tsp_debug_info(true, &info->client->dev, "FTS Initialise Failed\n");
		return;
	}
	info->pFrame =
	    kzalloc(info->SenseChannelLength * info->ForceChannelLength * 2,
		    GFP_KERNEL);
	if (info->pFrame == NULL) {
		tsp_debug_info(true, &info->client->dev, "FTS pFrame kzalloc Failed\n");
		return;
	}

	fts_command(info, FORCECALIBRATION);
	fts_command(info, FLUSHBUFFER);

	fts_interrupt_set(info, INT_ENABLE);

	tsp_debug_info(true, &info->client->dev, "FTS Re-Initialised\n");
}

#endif

static void fts_reinit(struct fts_ts_info *info)
{
	fts_wait_for_ready(info);

	fts_systemreset(info);

	fts_wait_for_ready(info);

#ifdef CONFIG_SEC_FACTORY
	/* Read firmware version from IC when every power up IC.
	 * During Factory process touch panel can be changed manually.
	 */
	{
		unsigned short orig_fw_main_version_of_ic = info->fw_main_version_of_ic;

		fts_get_panel_revision(info);
		fts_get_version_info(info);

		if (info->fw_main_version_of_ic != orig_fw_main_version_of_ic) {
			fts_fw_init(info);
			fts_reinit_fac(info);
			return;
		}
	}
#endif

#ifdef FTS_SUPPORT_NOISE_PARAM
	fts_set_noise_param(info);
#endif

	fts_command(info, SLEEPOUT);
	fts_delay(50);

#if defined(CONFIG_SEC_S_PROJECT)
	fts_command(info, SENSEON);
	fts_delay(50);

	if (info->slow_report_rate)
		fts_command(info, FTS_CMD_SLOW_SCAN);
#else
	if (info->slow_report_rate)
		fts_command(info, SENSEON_SLOW);
	else
		fts_command(info, SENSEON);
#endif

#ifdef FTS_SUPPORT_TOUCH_KEY
		info->fts_command(info, FTS_CMD_KEY_SENSE_ON);
#endif // FTS_SUPPORT_TOUCH_KEY

	if (info->flip_enable)
		fts_set_flipcover_mode(info, true);
	else if (info->fast_mshover_enabled)
		fts_command(info, FTS_CMD_SET_FAST_GLOVE_MODE);
	else if (info->mshover_enabled)
		fts_command(info, FTS_CMD_MSHOVER_ON);

#ifdef FTS_SUPPORT_TA_MODE
	if (info->TA_Pluged)
		fts_command(info, FTS_CMD_CHARGER_PLUGGED);
#endif

	info->touch_count = 0;

	fts_command(info, FLUSHBUFFER);
	fts_interrupt_set(info, INT_ENABLE);
}

void fts_release_all_finger(struct fts_ts_info *info)
{
	int i;

	for (i = 0; i < FINGER_MAX; i++) {
		input_mt_slot(info->input_dev, i);
		input_mt_report_slot_state(info->input_dev, MT_TOOL_FINGER, 0);

		if ((info->finger[i].state == EVENTID_ENTER_POINTER) ||
			(info->finger[i].state == EVENTID_MOTION_POINTER)) {
			info->touch_count--;
			if (info->touch_count < 0)
				info->touch_count = 0;

			tsp_debug_info(true, &info->client->dev,
				"[R] tID:%d mc: %d tc:%d Ver[%02X%04X%01X%01X]\n",
				i, info->finger[i].mcount, info->touch_count,
				info->panel_revision, info->fw_main_version_of_ic,
				info->flip_enable, info->mshover_enabled);
		}

		info->finger[i].state = EVENTID_LEAVE_POINTER;
		info->finger[i].mcount = 0;
	}

	input_report_key(info->input_dev, BTN_TOUCH, 0);
#ifdef CONFIG_GLOVE_TOUCH
	input_report_switch(info->input_dev, SW_GLOVE, false);
	info->touch_mode = FTS_TM_NORMAL;
#endif

	input_sync(info->input_dev);

#ifdef TOUCH_BOOSTER_DVFS
	fts_set_dvfs_lock(info, -1);
#endif
}

static int fts_stop_device(struct fts_ts_info *info)
{
	tsp_debug_info(true, &info->client->dev, "%s\n", __func__);

	mutex_lock(&info->device_mutex);

	if (info->touch_stopped) {
		tsp_debug_err(true, &info->client->dev, "%s already power off\n", __func__);
		goto out;
	}

	fts_interrupt_set(info, INT_DISABLE);
	disable_irq(info->irq);

	fts_command(info, FLUSHBUFFER);
	fts_command(info, SLEEPIN);

	fts_release_all_finger(info);

#ifdef FTS_SUPPORT_TOUCH_KEY
	fts_release_all_key(info);

	if (info->board->led_power_off)
		info->board->led_power_off();
#endif // FTS_SUPPORT_TOUCH_KEY

#ifdef FTS_SUPPORT_NOISE_PARAM
	fts_get_noise_param(info);
#endif

	info->touch_stopped = true;

	if (info->board->power)
		info->board->power(false);

 out:
	mutex_unlock(&info->device_mutex);
	return 0;
}

static int fts_start_device(struct fts_ts_info *info)
{
	tsp_debug_info(true, &info->client->dev, "%s\n", __func__);

	mutex_lock(&info->device_mutex);

	if (!info->touch_stopped) {
		tsp_debug_err(true, &info->client->dev, "%s already power on\n", __func__);
		info->reinit_done = true;
		goto out;
	}

	if (info->board->power)
		info->board->power(true);

	info->touch_stopped = false;
	info->reinit_done = false;
	
	fts_reinit(info);
	info->reinit_done = true;
	
	enable_irq(info->irq);

 out:
	mutex_unlock(&info->device_mutex);
	return 0;
}

#ifdef CONFIG_PM
static int fts_pm_suspend(struct device *dev)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);

	tsp_debug_dbg(false, &info->client->dev, "%s\n", __func__);;

	mutex_lock(&info->input_dev->mutex);

	if (info->input_dev->users)
	fts_stop_device(info);

	mutex_unlock(&info->input_dev->mutex);

	return 0;
}

static int fts_pm_resume(struct device *dev)
{
	struct fts_ts_info *info = dev_get_drvdata(dev);

	tsp_debug_dbg(false, &info->client->dev, "%s\n", __func__);

	mutex_lock(&info->input_dev->mutex);

	if (info->input_dev->users)
	fts_start_device(info);

	mutex_unlock(&info->input_dev->mutex);

	return 0;
}
#endif

#if (!defined(CONFIG_HAS_EARLYSUSPEND)) && (!defined(CONFIG_PM)) && !defined(USE_OPEN_CLOSE)
static int fts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct fts_ts_info *info = i2c_get_clientdata(client);

	tsp_debug_info(true, &info->client->dev, "%s\n", __func__);

	fts_stop_device(info);

	return 0;
}

static int fts_resume(struct i2c_client *client)
{

	struct fts_ts_info *info = i2c_get_clientdata(client);

	tsp_debug_info(true, &info->client->dev, "%s\n", __func__);

	fts_start_device(info);

	return 0;
}
#endif

static const struct i2c_device_id fts_device_id[] = {
	{FTS_TS_DRV_NAME, 0},
	{}
};

#ifdef CONFIG_PM
static const struct dev_pm_ops fts_dev_pm_ops = {
	.suspend = fts_pm_suspend,
	.resume = fts_pm_resume,
};
#endif

#ifdef CONFIG_OF
static struct of_device_id fts_match_table[] = {
        { .compatible = "stm,fts_touch",},
        { },
};
#else
#define fts_match_table NULL
#endif

static struct i2c_driver fts_i2c_driver = {
	.driver = {
		   .name = FTS_TS_DRV_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = fts_match_table,	   
#endif
#ifdef CONFIG_PM
		   .pm = &fts_dev_pm_ops,
#endif
		   },
	.probe = fts_probe,
	.remove = fts_remove,
#if (!defined(CONFIG_HAS_EARLYSUSPEND)) && (!defined(CONFIG_PM)) && !defined(USE_OPEN_CLOSE)
	.suspend = fts_suspend,
	.resume = fts_resume,
#endif
	.id_table = fts_device_id,
};

static int __init fts_driver_init(void)
{
	extern int poweroff_charging;
	pr_err("[TSP] %s : init !!\n", __func__);

	if (poweroff_charging) {
		pr_err("%s : LPM Charging Mode!!\n", __func__);
		return 0;
	}
	else
		return i2c_add_driver(&fts_i2c_driver);
}

static void __exit fts_driver_exit(void)
{
	i2c_del_driver(&fts_i2c_driver);
}

MODULE_DESCRIPTION("STMicroelectronics MultiTouch IC Driver");
MODULE_AUTHOR("STMicroelectronics, Inc.");
MODULE_LICENSE("GPL v2");

module_init(fts_driver_init);
module_exit(fts_driver_exit);
