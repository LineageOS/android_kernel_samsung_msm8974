/*
 *  wacom_i2c_func.c - Wacom G5 Digitizer Controller (I2C bus)
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

#include <linux/wacom_i2c.h>
#include "wacom_i2c_flash.h"

#ifdef WACOM_IMPORT_FW_ALGO
#include "wacom_i2c_coord_table.h"
#endif

#if defined(CONFIG_USE_INPUTLOCATION_FOR_ENG)
#define CONFIG_SAMSUNG_KERNEL_DEBUG_USER
#endif

#ifdef WACOM_BOOSTER
static void wacom_change_dvfs_lock(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
		container_of(work,
			struct wacom_i2c, work_dvfs_chg.work);
	int retval = 0;

	mutex_lock(&wac_i2c->dvfs_lock);

	if (wac_i2c->dvfs_boost_mode == DVFS_STAGE_DUAL) {
		retval = set_freq_limit(DVFS_TOUCH_ID,
			MIN_TOUCH_LIMIT_SECOND);
		wac_i2c->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
	} else if (wac_i2c->dvfs_boost_mode == DVFS_STAGE_SINGLE ||
		wac_i2c->dvfs_boost_mode == DVFS_STAGE_TRIPLE) {
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		wac_i2c->dvfs_freq = -1;
	}

	if (retval < 0)
		dev_info(&wac_i2c->client->dev,
			"%s: booster change failed(%d).\n",
			__func__, retval);
	mutex_unlock(&wac_i2c->dvfs_lock);
}

static void wacom_set_dvfs_off(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
		container_of(work,
			struct wacom_i2c, work_dvfs_off.work);
	int retval;

	mutex_lock(&wac_i2c->dvfs_lock);
	retval = set_freq_limit(DVFS_TOUCH_ID, -1);
	if (retval < 0)
		dev_info(&wac_i2c->client->dev,
			"%s: booster stop failed(%d).\n",
			__func__, retval);

	wac_i2c->dvfs_lock_status = false;
	mutex_unlock(&wac_i2c->dvfs_lock);
}

void wacom_set_dvfs_lock(struct wacom_i2c *wac_i2c, int on)
{
	int ret = 0;

	if (wac_i2c->dvfs_boost_mode == DVFS_STAGE_NONE) {
		dev_dbg(&wac_i2c->client->dev,
				"%s: DVFS stage is none(%d)\n",
				__func__, wac_i2c->dvfs_boost_mode);
		return;
	}

	mutex_lock(&wac_i2c->dvfs_lock);
	if (on == 0) {
		if (wac_i2c->dvfs_lock_status) {
			schedule_delayed_work(&wac_i2c->work_dvfs_off,
				msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));
		}
	} else if (on > 0) {
		cancel_delayed_work(&wac_i2c->work_dvfs_off);

		if (!wac_i2c->dvfs_lock_status || wac_i2c->dvfs_old_stauts < on) {
			cancel_delayed_work(&wac_i2c->work_dvfs_chg);
			if (wac_i2c->dvfs_freq != MIN_TOUCH_LIMIT) {
				if (wac_i2c->dvfs_boost_mode == DVFS_STAGE_TRIPLE)
					ret = set_freq_limit(DVFS_TOUCH_ID,
						MIN_TOUCH_LIMIT_SECOND);
				else
					ret = set_freq_limit(DVFS_TOUCH_ID,
						MIN_TOUCH_LIMIT);
				wac_i2c->dvfs_freq = MIN_TOUCH_LIMIT;
				if (ret < 0)
					dev_info(&wac_i2c->client->dev,
						"%s: cpu first lock failed(%d)\n",
						__func__, ret);
			}
			schedule_delayed_work(&wac_i2c->work_dvfs_chg,
				msecs_to_jiffies(TOUCH_BOOSTER_CHG_TIME));

			wac_i2c->dvfs_lock_status = true;
		}
	} else if (on < 0) {
		if (wac_i2c->dvfs_lock_status) {
			cancel_delayed_work(&wac_i2c->work_dvfs_off);
			cancel_delayed_work(&wac_i2c->work_dvfs_chg);
			schedule_work(&wac_i2c->work_dvfs_off.work);
		}
	}
	wac_i2c->dvfs_old_stauts = on;
	mutex_unlock(&wac_i2c->dvfs_lock);
}

void wacom_init_dvfs(struct wacom_i2c *wac_i2c)
{
	mutex_init(&wac_i2c->dvfs_lock);
	wac_i2c->dvfs_boost_mode = DVFS_STAGE_DUAL;

	INIT_DELAYED_WORK(&wac_i2c->work_dvfs_off, wacom_set_dvfs_off);
	INIT_DELAYED_WORK(&wac_i2c->work_dvfs_chg, wacom_change_dvfs_lock);

	wac_i2c->dvfs_lock_status = false;
}
#endif

void forced_release(struct wacom_i2c *wac_i2c)
{
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
	dev_dbg(&wac_i2c->client->dev,
			 "%s\n", __func__);
#endif
	input_report_abs(wac_i2c->input_dev, ABS_X, wac_i2c->last_x);
	input_report_abs(wac_i2c->input_dev, ABS_Y, wac_i2c->last_y);
	input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
#ifdef WACOM_USE_GAIN
	input_report_abs(wac_i2c->input_dev, ABS_DISTANCE, 0);
#endif
	input_report_key(wac_i2c->input_dev, BTN_STYLUS, 0);
	input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
#if 0/*defined(WACOM_PDCT_WORK_AROUND)*/
	input_report_key(wac_i2c->input_dev, BTN_TOOL_RUBBER, 0);
	input_report_key(wac_i2c->input_dev, BTN_TOOL_PEN, 0);
	input_report_key(wac_i2c->input_dev, KEY_PEN_PDCT, 0);
#else
	input_report_key(wac_i2c->input_dev, wac_i2c->tool, 0);
#endif
	input_sync(wac_i2c->input_dev);

	wac_i2c->last_x = 0;
	wac_i2c->last_y = 0;
	wac_i2c->pen_prox = 0;
	wac_i2c->pen_pressed = 0;
	wac_i2c->side_pressed = 0;
	wac_i2c->pen_pdct = PDCT_NOSIGNAL;

}

#ifdef WACOM_PDCT_WORK_AROUND
void forced_hover(struct wacom_i2c *wac_i2c)
{
	/* To distinguish hover and pdct area, release */
	if (wac_i2c->last_x != 0 || wac_i2c->last_y != 0) {
		dev_dbg(&wac_i2c->client->dev,
				 "%s: release hover\n",
				 __func__);
		forced_release(wac_i2c);
	}
	wac_i2c->rdy_pdct = true;
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
	dev_dbg(&wac_i2c->client->dev,
			 "%s\n", __func__);
#endif
	input_report_key(wac_i2c->input_dev, KEY_PEN_PDCT, 1);
	input_sync(wac_i2c->input_dev);

}
#endif

/*
 * wacom_i2c_send : use i2c_master_send(), using CMD
 * wacom_i2c_recv : use i2c_master_recv(), using CMD
 * wacom_i2c_write : use i2c_transfer(), using register address
 * wacom_i2c_read : use i2c_transfer(), using register address
 */

int wacom_i2c_send(struct wacom_i2c *wac_i2c,
			  const char *buf, int count, bool mode)
{
	struct i2c_client *client;

	client = mode ? wac_i2c->client_boot : wac_i2c->client;
	if (wac_i2c->boot_mode && !mode) {
		dev_info(&client->dev,
			 "%s: failed to send\n",
			 __func__);
		return 0;
	}

	return i2c_master_send(client, buf, count);
}

int wacom_i2c_recv(struct wacom_i2c *wac_i2c,
			char *buf, int count, bool mode)
{
	struct i2c_client *client;

	client = mode ? wac_i2c->client_boot : wac_i2c->client;
	if (wac_i2c->boot_mode && !mode) {
		dev_info(&client->dev,
			 "%s: failed to received\n",
			 __func__);
		return 0;
	}

	return i2c_master_recv(client, buf, count);
}

int wacom_i2c_write(struct i2c_client *client,
			const char *buf, int count, unsigned char addr)
{
        int ret;
        struct i2c_adapter *adap=client->adapter;
        struct i2c_msg msg;

        msg.addr = addr;
        msg.flags = client->flags & I2C_M_TEN;
        msg.len = count;
        msg.buf = (char *)buf;

        ret = i2c_transfer(adap, &msg, 1);

        /* If everything went ok (i.e. 1 msg transmitted), return #bytes
           transmitted, else error code. */
        return (ret == 1) ? count : ret;
}

int wacom_i2c_read(struct i2c_client *client,
			const char *buf, int count, unsigned char addr)
{
        int ret;
        struct i2c_adapter *adap=client->adapter;
        struct i2c_msg msg;
	
        msg.addr = addr;
        msg.flags = client->flags & I2C_M_TEN;
        msg.flags |= I2C_M_RD;
        msg.len = count;
        msg.buf = (char *)buf;

        ret = i2c_transfer(adap, &msg, 1);

        /* If everything went ok (i.e. 1 msg transmitted), return #bytes
           transmitted, else error code. */
        return (ret == 1) ? count : ret;
}

int wacom_i2c_test(struct wacom_i2c *wac_i2c)
{
	int ret, i;
	char buf, test[10];
	buf = COM_QUERY;

	ret = wacom_i2c_send(wac_i2c, &buf, sizeof(buf), false);
	if (ret > 0)
		dev_info(&wac_i2c->client->dev,
			 "%s: buf:%d, sent:%d\n",
			 __func__, buf, ret);
	else {
		dev_err(&wac_i2c->client->dev,
			 "%s: Digitizer is not active\n",
			 __func__);
		return -1;
	}

	ret = wacom_i2c_recv(wac_i2c, test, sizeof(test), false);
	if (ret >= 0) {
		for (i = 0; i < 8; i++)
		dev_info(&wac_i2c->client->dev,
			 "%s: %d\n", __func__, test[i]);
	} else {
		dev_err(&wac_i2c->client->dev,
			 "%s: Digitizer does not reply\n",
			 __func__);
		return -1;
	}

	return 0;
}

int wacom_checksum(struct wacom_i2c *wac_i2c)
{
	int ret = 0, retry = 10;
	int i = 0;
	u8 buf[5] = {0, };

	buf[0] = COM_CHECKSUM;

	while (retry--) {
		ret = wacom_i2c_send(wac_i2c, &buf[0], 1, false);
		if (ret < 0) {
			dev_err(&wac_i2c->client->dev,
					 "%s: i2c fail, retry, %d\n",
				      __func__, __LINE__);
			continue;
		}

		msleep(200);
		ret = wacom_i2c_recv(wac_i2c, buf, 5, false);
		if (ret < 0) {
			dev_err(&wac_i2c->client->dev,
					 "%s: i2c fail, retry, %d\n",
					 __func__, __LINE__);
			continue;
		} else if (buf[0] == 0x1f)
			break;
		dev_info(&wac_i2c->client->dev,
				 "%s: checksum retry\n",
				 __func__);
	}

	if (ret >= 0) {
		dev_info(&wac_i2c->client->dev,
				 "%s: received checksum %x, %x, %x, %x, %x\n",
				__func__, buf[0], buf[1],
				buf[2], buf[3], buf[4]);
	}

	for (i = 0; i < 5; ++i) {
		if (buf[i] != Firmware_checksum[i]) {
		dev_info(&wac_i2c->client->dev,
				 "%s: checksum fail %dth %x %x\n",
				__func__, i, buf[i],
				Firmware_checksum[i]);
			break;
		}
	}

	wac_i2c->checksum_result = (5 == i);

	return ret;
}

int wacom_i2c_query(struct wacom_i2c *wac_i2c)
{
	struct wacom_features *wac_feature = wac_i2c->wac_feature;
	int ret;
	u8 buf;
	u8 data[COM_QUERY_NUM] = {0, };
	int i = 0;
	const int query_limit = 3;

	buf = COM_QUERY;

	dev_info(&wac_i2c->client->dev,
			"%s: start\n", __func__);
	for (i = 0; i < query_limit; i++) {
		ret = wacom_i2c_send(wac_i2c, &buf, 1, false);
		if (ret < 0) {
			dev_err(&wac_i2c->client->dev,
				 "%s: I2C send failed(%d)\n",
				 __func__, ret);
			continue;
		}
		msleep(100);
		ret = wacom_i2c_recv(wac_i2c, data, COM_QUERY_NUM, false);
		if (ret < 0) {
			dev_err(&wac_i2c->client->dev,
				"%s: I2C recv failed(%d)\n",
				__func__, ret);
			continue;
		}
		dev_info(&wac_i2c->client->dev,
				"%s: %dth ret of wacom query=%d\n",
				__func__, i, ret);
		if (COM_QUERY_NUM != ret) {
			dev_info(&wac_i2c->client->dev,
			"%s: epen:failed to read i2c(%d)\n",
			__func__, ret);
			continue;
		}
			if (0x0f == data[0]) {
				wac_feature->fw_ic_version =
					((u16) data[7] << 8) + (u16) data[8];
				break;
			} else {
				dev_info(&wac_i2c->client->dev,
				       "%s: %X, %X, %X, %X, %X, %X, %X, fw=0x%x\n",
				       __func__,
				       data[0], data[1], data[2], data[3],
				       data[4], data[5], data[6],
				       wac_feature->fw_ic_version);
			}
		}
	wac_feature->x_max = wac_i2c->wac_pdata->max_x;
	wac_feature->y_max = wac_i2c->wac_pdata->max_y;

	wac_feature->pressure_max = (u16) data[6] + ((u16) data[5] << 8);

#if defined(COOR_WORK_AROUND)
	if (i == 10 || ret < 0) {
		dev_info(&wac_i2c->client->dev,
				"%s: COOR_WORK_AROUND is applied\n",
				__func__);
		dev_info(&wac_i2c->client->dev,
		       "%s: %X, %X, %X, %X, %X, %X, %X, %X, %X\n",
		       __func__, data[0], data[1], data[2],
		       data[3], data[4], data[5], data[6],
		       data[7], data[8]);
		wac_feature->x_max = (u16) wac_i2c->wac_pdata->max_x;
		wac_feature->y_max = (u16) wac_i2c->wac_pdata->max_y;
		wac_feature->pressure_max = (u16) wac_i2c->wac_pdata->max_pressure;
		wac_feature->fw_ic_version = 0;
	}
#endif

	dev_info(&wac_i2c->client->dev,
			"%s: x_max=0x%X, y_max=0x%X\n, pressure_max=0x%X",
			__func__, wac_feature->x_max,
			wac_feature->y_max,
			wac_feature->pressure_max);
	dev_info(&wac_i2c->client->dev,
			"%s: fw_version=0x%X (d7:0x%X,d8:0x%X)\n",
			__func__, wac_feature->fw_version,
			data[7], data[8]);
	dev_info(&wac_i2c->client->dev,
			"%s: %X, %X, %X, %X, %X, %X, %X, %X, %X\n",
			__func__, data[0], data[1], data[2],
			data[3], data[4], data[5], data[6],
			data[7], data[8]);

	if ((i == query_limit) && (ret < 0)) {
		dev_info(&wac_i2c->client->dev,
				"%s: failed\n", __func__);
		wac_i2c->query_status = false;
		return ret;
	}
	wac_i2c->query_status = true;

	return wac_feature->fw_ic_version;
}

#ifdef WACOM_IMPORT_FW_ALGO
#ifdef WACOM_USE_OFFSET_TABLE
void wacom_i2c_coord_offset(struct wacom_g5_platform_data *wac_pdata,
				u16 *coordX, u16 *coordY)
{
	u16 ix, iy;
	u16 dXx_0, dXy_0, dXx_1, dXy_1;
	int D0, D1, D2, D3, D;
	int cal_pitch, lattice_size_x, lattice_size_y;

	/* For firmware algorithm */
	cal_pitch = 100;
	lattice_size_x = (wac_pdata->max_x / cal_pitch) + 2;
	lattice_size_y = (wac_pdata->max_y / cal_pitch) + 2;

	ix = (u16) (((*coordX)) / cal_pitch);
	iy = (u16) (((*coordY)) / cal_pitch);

	dXx_0 = *coordX - (ix * cal_pitch);
	dXx_1 = cal_pitch - dXx_0;

	dXy_0 = *coordY - (iy * cal_pitch);
	dXy_1 = cal_pitch - dXy_0;

	if (*coordX <= wac_pdata->max_x) {
		D0 = tableX[user_hand][screen_rotate][ix +
						      (iy * lattice_size_x)] *
		    (dXx_1 + dXy_1);
		D1 = tableX[user_hand][screen_rotate][ix + 1 +
						      iy * lattice_size_x] *
		    (dXx_0 + dXy_1);
		D2 = tableX[user_hand][screen_rotate][ix +
						      (iy +
						       1) * lattice_size_x] *
		    (dXx_1 + dXy_0);
		D3 = tableX[user_hand][screen_rotate][ix + 1 +
						      (iy +
						       1) * lattice_size_x] *
		    (dXx_0 + dXy_0);
		D = (D0 + D1 + D2 + D3) / (4 * cal_pitch);

		if (((int)*coordX + D) > 0)
			*coordX += D;
		else
			*coordX = 0;
	}

	if (*coordY <= wac_pdata->max_y) {
		D0 = tableY[user_hand][screen_rotate][ix +
						      (iy * lattice_size_x)] *
		    (dXy_1 + dXx_1);
		D1 = tableY[user_hand][screen_rotate][ix + 1 +
						      iy * lattice_size_x] *
		    (dXy_1 + dXx_0);
		D2 = tableY[user_hand][screen_rotate][ix +
						      (iy +
						       1) * lattice_size_x] *
		    (dXy_0 + dXx_1);
		D3 = tableY[user_hand][screen_rotate][ix + 1 +
						      (iy +
						       1) * lattice_size_x] *
		    (dXy_0 + dXx_0);
		D = (D0 + D1 + D2 + D3) / (4 * cal_pitch);

		if (((int)*coordY + D) > 0)
			*coordY += D;
		else
			*coordY = 0;
	}
}
#endif

#ifdef WACOM_USE_AVERAGING
#define STEP 32
void wacom_i2c_coord_average(short *CoordX, short *CoordY,
			     int bFirstLscan, int aveStrength)
{
	unsigned char i;
	unsigned int work;
	unsigned char ave_step = 4, ave_shift = 2;
	static int Sum_X, Sum_Y;
	static int AveBuffX[STEP], AveBuffY[STEP];
	static unsigned char AvePtr;
	static unsigned char bResetted;
#ifdef WACOM_USE_AVE_TRANSITION
	static int tmpBuffX[STEP], tmpBuffY[STEP];
	static unsigned char last_step, last_shift;
	static bool transition;
	static int tras_counter;
#endif
	if (bFirstLscan == 0) {
		bResetted = 0;
#ifdef WACOM_USE_AVE_TRANSITION
		transition = false;
		tras_counter = 0;
		last_step = 4;
		last_shift = 2;
#endif
		return ;
	}
#ifdef WACOM_USE_AVE_TRANSITION
	if (bResetted) {
		if (transition) {
			ave_step = last_step;
			ave_shift = last_shift;
		} else {
			ave_step = 2 << (aveStrength-1);
			ave_shift = aveStrength;
		}

		if (!transition && ave_step != 0 && last_step != 0) {
			if (ave_step > last_step) {
				transition = true;
				tras_counter = ave_step;
				/*dev_info(&wac_i2c->client->dev,
					"%s: Trans %d to %d\n",
					__func__, last_step, ave_step);*/

				memcpy(tmpBuffX, AveBuffX,
					sizeof(unsigned int) * last_step);
				memcpy(tmpBuffY, AveBuffY,
					sizeof(unsigned int) * last_step);
				for (i = 0 ; i < last_step; ++i) {
					AveBuffX[i] = tmpBuffX[AvePtr];
					AveBuffY[i] = tmpBuffY[AvePtr];
					if (++AvePtr >= last_step)
						AvePtr = 0;
				}
				for ( ; i < ave_step; ++i) {
					AveBuffX[i] = *CoordX;
					AveBuffY[i] = *CoordY;
					Sum_X += *CoordX;
					Sum_Y += *CoordY;
				}
				AvePtr = 0;

				*CoordX = Sum_X >> ave_shift;
				*CoordY = Sum_Y >> ave_shift;

				bResetted = 1;

				last_step = ave_step;
				last_shift = ave_shift;
				return ;
			} else if (ave_step < last_step) {
				transition = true;
				tras_counter = ave_step;
				/*dev_info(&wac_i2c->client->dev,
					"%s: Trans %d to %d\n",
					__func__, last_step, ave_step);*/

				memcpy(tmpBuffX, AveBuffX,
					sizeof(unsigned int) * last_step);
				memcpy(tmpBuffY, AveBuffY,
					sizeof(unsigned int) * last_step);
				Sum_X = 0;
				Sum_Y = 0;
				for (i = 1 ; i <= ave_step; ++i) {
					if (AvePtr == 0)
						AvePtr = last_step - 1;
					else
						--AvePtr;
					AveBuffX[ave_step-i] = tmpBuffX[AvePtr];
					Sum_X = Sum_X + tmpBuffX[AvePtr];

					AveBuffY[ave_step-i] = tmpBuffY[AvePtr];
					Sum_Y = Sum_Y + tmpBuffY[AvePtr];

				}
				AvePtr = 0;
				bResetted = 1;
				*CoordX = Sum_X >> ave_shift;
				*CoordY = Sum_Y >> ave_shift;

				bResetted = 1;

				last_step = ave_step;
				last_shift = ave_shift;
				return ;
			}
		}

		if (!transition && (last_step != ave_step)) {
			last_step = ave_step;
			last_shift = ave_shift;
		}
	}
#endif
	if (bFirstLscan && (bResetted == 0)) {
		AvePtr = 0;
		ave_step = 4;
		ave_shift = 2;
#if defined(WACOM_USE_AVE_TRANSITION)
		tras_counter = ave_step;
#endif
		for (i = 0; i < ave_step; i++) {
			AveBuffX[i] = *CoordX;
			AveBuffY[i] = *CoordY;
		}
		Sum_X = (unsigned int)*CoordX << ave_shift;
		Sum_Y = (unsigned int)*CoordY << ave_shift;
		bResetted = 1;
	} else if (bFirstLscan) {
		Sum_X = Sum_X - AveBuffX[AvePtr] + (*CoordX);
		AveBuffX[AvePtr] = *CoordX;
		work = Sum_X >> ave_shift;
		*CoordX = (unsigned int)work;

		Sum_Y = Sum_Y - AveBuffY[AvePtr] + (*CoordY);
		AveBuffY[AvePtr] = (*CoordY);
		work = Sum_Y >> ave_shift;
		*CoordY = (unsigned int)work;

		if (++AvePtr >= ave_step)
			AvePtr = 0;
	}
#ifdef WACOM_USE_AVE_TRANSITION
	if (transition) {
		--tras_counter;
		if (tras_counter < 0)
			transition = false;
	}
#endif
}
#endif

#if defined(WACOM_USE_GAIN)
u8 wacom_i2c_coord_level(u16 gain)
{
	if (gain >= 0 && gain <= 14)
		return 0;
	else if (gain > 14 && gain <= 24)
		return 1;
	else
		return 2;
}
#endif

#ifdef WACOM_USE_BOX_FILTER
void boxfilt(short *CoordX, short *CoordY,
			int height, int bFirstLscan)
{
	bool isMoved = false;
	static bool bFirst = true;
	static short lastX_loc, lastY_loc;
	static unsigned char bResetted;
	int threshold = 0;
	int distance = 0;
	static short bounce;

	/*Reset filter*/
	if (bFirstLscan == 0) {
		bResetted = 0;
		return ;
	}

	if (bFirstLscan && (bResetted == 0)) {
		lastX_loc = *CoordX;
		lastY_loc = *CoordY;
		bResetted = 1;
	}

	if (bFirst) {
		lastX_loc = *CoordX;
		lastY_loc = *CoordY;
		bFirst = false;
	}

	/*Start Filtering*/
	threshold = 30;

	/*X*/
	distance = abs(*CoordX - lastX_loc);

	if (distance >= threshold)
		isMoved = true;

	if (isMoved == false) {
		distance = abs(*CoordY - lastY_loc);
		if (distance >= threshold)
			isMoved = true;
	}

	/*Update position*/
	if (isMoved) {
		lastX_loc = *CoordX;
		lastY_loc = *CoordY;
	} else {
		*CoordX = lastX_loc + bounce;
		*CoordY = lastY_loc;
		if (bounce)
			bounce = 0;
		else
			bounce += 5;
	}
}
#endif

#if defined(WACOM_USE_AVE_TRANSITION)
int g_aveLevel_C[] = {2, 2, 4, };
int g_aveLevel_X[] = {3, 3, 4, };
int g_aveLevel_Y[] = {3, 3, 4, };
int g_aveLevel_Trs[] = {3, 4, 4, };
int g_aveLevel_Cor[] = {4, 4, 4, };

void ave_level(struct wacom_g5_platform_data *wac_pdata,
			short CoordX, short CoordY,
			int height, int *aveStrength)
{
	bool transition = false;
	bool edgeY = false, edgeX = false;
	bool cY = false, cX = false;

	/*Box Filter Parameters*/
	int x_inc_s1, x_inc_e1, y_inc_s1, y_inc_e1;
	int y_inc_s2, y_inc_e2, y_inc_s3, y_inc_e3;

	x_inc_s1 = 1500;
	x_inc_e1 = wac_pdata->max_x - x_inc_s1;
	y_inc_s1 = 1500;
	y_inc_e1 = wac_pdata->max_y - y_inc_s1;

	y_inc_s2 = 500;
	y_inc_e2 = wac_pdata->max_y - y_inc_s2;
	y_inc_s3 = 1100;
	y_inc_e3 = wac_pdata->max_y - y_inc_s3;
	/************************/

	if (CoordY > (wac_pdata->max_y - 800))
		cY = true;
	else if (CoordY < 800)
		cY = true;

	if (CoordX > (wac_pdata->max_x - 800))
		cX = true;
	else if (CoordX < 800)
		cX = true;

	if (cX && cY) {
		*aveStrength = g_aveLevel_Cor[height];
		return ;
	}

	/*Start Filtering*/
	if (CoordX > x_inc_e1)
		edgeX = true;
	else if (CoordX < x_inc_s1)
		edgeX = true;

	/*Right*/
	if (CoordY > y_inc_e1) {
		/*Transition*/
		if (CoordY < y_inc_e3)
			transition = true;
		else
			edgeY = true;
	}
	/*Left*/
	else if (CoordY < y_inc_s1) {
		/*Transition*/
		if (CoordY > y_inc_s3)
			transition = true;
		else
			edgeY = true;
	}

	if (transition)
		*aveStrength = g_aveLevel_Trs[height];
	else if (edgeX)
		*aveStrength = g_aveLevel_X[height];
	else if (edgeY)
		*aveStrength = g_aveLevel_Y[height];
	else
		*aveStrength = g_aveLevel_C[height];
}
#endif
#endif /*WACOM_IMPORT_FW_ALGO*/

static bool wacom_i2c_coord_range(struct wacom_i2c *wac_i2c, s16 *x, s16 *y)
{
	if (wac_i2c->wac_pdata->xy_switch) {
		if ((*x <= wac_i2c->wac_feature->y_max) && (*y <= wac_i2c->wac_feature->x_max))
			return true;
	} else {
		if ((*x <= wac_i2c->wac_feature->x_max) && (*y <= wac_i2c->wac_feature->y_max))
			return true;
	}
/*
	if ((*x <= wac_i2c->pdata->max_pressure) && (*y <= wac_i2c->pdata->max_pressure))
		return true;
*/
	return false;
}

#ifdef WACOM_USE_SOFTKEY
#if defined(CONFIG_SEC_VIENNA_PROJECT)
static int keycode[] = {
	KEY_RECENT, KEY_BACK,
};
#else
static int keycode[] = {
	KEY_MENU, KEY_BACK,
};
#endif

void wacom_i2c_softkey(struct wacom_i2c *wac_i2c, s16 key, s16 pressed)
{
	
	if (wac_i2c->pen_prox) {
		dev_info(&wac_i2c->client->dev,
				"%s: prox:%d, run release_hover\n",
				__func__, wac_i2c->pen_prox);

		input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
#ifdef WACOM_USE_GAIN
		input_report_abs(wac_i2c->input_dev, ABS_DISTANCE, 0);
#endif
		input_report_key(wac_i2c->input_dev, BTN_STYLUS, 0);
		input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
#if 0/*defined(WACOM_PDCT_WORK_AROUND)*/
		input_report_key(wac_i2c->input_dev,
			BTN_TOOL_RUBBER, 0);
		input_report_key(wac_i2c->input_dev, BTN_TOOL_PEN, 0);
		input_report_key(wac_i2c->input_dev, KEY_PEN_PDCT, 0);
#else
		input_report_key(wac_i2c->input_dev, wac_i2c->tool, 0);
#endif
		input_sync(wac_i2c->input_dev);

		wac_i2c->pen_prox = 0;
	}

#ifdef USE_WACOM_BLOCK_KEYEVENT
	wac_i2c->touchkey_skipped = false;
#endif
	input_report_key(wac_i2c->input_dev,
			keycode[key], pressed);
	input_sync(wac_i2c->input_dev);

#ifdef WACOM_BOOSTER
	wacom_set_dvfs_lock(wac_i2c, pressed);
#endif

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	dev_info(&wac_i2c->client->dev,
			"%s: keycode:%d pressed:%d. pen_prox=%d\n",
			__func__, keycode[key], pressed, wac_i2c->pen_prox);
#else
	dev_info(&wac_i2c->client->dev,
			"%s: pressed:%d\n",
			__func__, pressed);
#endif
}
#endif

static void handle_gestures(int x, int y, ktime_t end, struct wacom_i2c *wac_i2c)
{
	int dx = x - wac_i2c->gesture_start_x;
	int dy = y - wac_i2c->gesture_start_y;
	int dt = ktime_to_ms(ktime_sub(end, wac_i2c->gesture_start_time));

	if (abs(dy) > abs(dx)) {
		if (abs(dy) > MIN_GEST_DIST) {
			wac_i2c->gesture_key = dy < 0 ? KEY_PEN_DTU : KEY_PEN_UTD;
			return;
		}
	} else {
		if (abs(dx) > MIN_GEST_DIST) {
			wac_i2c->gesture_key = dx < 0 ? KEY_PEN_RTL : KEY_PEN_LTR;
			return;
		}
	}

	if (dt >= LONG_PRESS_TIME) {
		wac_i2c->gesture_key = KEY_PEN_LP;
		return;
	}
}

int wacom_i2c_coord(struct wacom_i2c *wac_i2c)
{
	bool prox = false;
	int ret = 0;
	u8 *data;
	int rubber, stylus;
	static s16 x, y, pressure;
	static s16 tmp;
	int rdy = 0;

#if defined(WACOM_USE_GAIN)
	u8 gain = 0;
	u8 height = 0;
#endif
#ifdef WACOM_USE_AVERAGING
	int aveStrength = 2;
#endif
#ifdef WACOM_USE_SOFTKEY
	static s16 softkey, pressed, keycode;
#endif

	data = wac_i2c->wac_feature->data;

#ifdef USE_WACOM_LCD_WORKAROUND
	ret = wacom_i2c_recv(wac_i2c, data, COM_COORD_NUM_W9010, false);
#else
	ret = wacom_i2c_recv(wac_i2c, data, COM_COORD_NUM, false);
#endif
	if (ret < 0) {
		dev_err(&wac_i2c->client->dev,
				"%s: failed to read i2c.L%d\n",
				__func__, __LINE__);
		return -1;
	}

#ifdef USE_WACOM_LCD_WORKAROUND
	if ((data[0] >> 7 == 0) && wac_i2c->boot_done && (data[10] != 0) && (data[11] != 0)) {
		wac_i2c->vsync = 2000000 * 1000 / (((data[10] << 8) | data[11]) + 1);
		wacom_i2c_write_vsync(wac_i2c);
	}
#endif

#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
	dev_dbg(&wac_i2c->client->dev,
			"%s: %x, %x, %x, %x, %x, %x, %x\n",
			__func__, data[0], data[1], data[2],
			data[3], data[4], data[5], data[6]);
#endif
	if (data[0] & 0x80) {
		/* enable emr device */
#ifdef WACOM_USE_SOFTKEY
		softkey = !!(data[5] & 0x80);
		if (softkey) {
			pressed = !!(data[5] & 0x40);
			keycode = (data[5] & 0x30) >> 4;
#ifdef USE_WACOM_BLOCK_KEYEVENT
			if (wac_i2c->touch_pressed) {
				if (pressed) {
					wac_i2c->touchkey_skipped = true;
					dev_info(&wac_i2c->client->dev,
							"%s : skip key press\n", __func__);
				} else {
					wac_i2c->touchkey_skipped = false;
					dev_info(&wac_i2c->client->dev,
							"%s : skip key release\n", __func__);
				}
			} else {
				if (wac_i2c->touchkey_skipped) {
					dev_info(&wac_i2c->client->dev,
							"%s: skipped touchkey event[%d]\n",
							__func__, pressed);
					if (!pressed)
						wac_i2c->touchkey_skipped = false;
				} else {
					wacom_i2c_softkey(wac_i2c, keycode, pressed);
				}
			}
#else
			wacom_i2c_softkey(wac_i2c, keycode, pressed);
#endif
			return 0;
		}
#endif

		if (!wac_i2c->pen_prox) {

#ifdef WACOM_PDCT_WORK_AROUND
			if (wac_i2c->pen_pdct) {
				dev_info(&wac_i2c->client->dev,
						"%s: IC interrupt ocurrs, but PDCT HIGH, return.\n",
						__func__);
				return 0;
			}
#endif

#ifdef WACOM_BOOSTER
			wacom_set_dvfs_lock(wac_i2c, 1);
#endif
			wac_i2c->pen_prox = 1;

			if (data[0] & 0x40)
				wac_i2c->tool = BTN_TOOL_RUBBER;
			else
				wac_i2c->tool = BTN_TOOL_PEN;
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
			dev_info(&wac_i2c->client->dev,
					"%s: is in(0x%x)\n",
					__func__, wac_i2c->tool);
#endif
		}
		prox = !!(data[0] & 0x10);
		stylus = !!(data[0] & 0x20);
		rubber = !!(data[0] & 0x40);
		rdy = !!(data[0] & 0x80);

		x = ((u16) data[1] << 8) + (u16) data[2];
		y = ((u16) data[3] << 8) + (u16) data[4];
		pressure = ((u16) data[5] << 8) + (u16) data[6];
#if defined(WACOM_USE_GAIN)
		gain = data[7];
#endif

#ifdef WACOM_IMPORT_FW_ALGO
#if defined(CONFIG_SEC_VIENNA_PROJECT) || defined(CONFIG_SEC_LT03_PROJECT)
		x = x - origin_offset[0];
		y = y - origin_offset[1];
#else
		/* Change Position to Active Area */
		if (x < origin_offset[0])
			x = 0;
		else
		x = x - origin_offset[0];
		if (y < origin_offset[1])
			y = 0;
		else
		y = y - origin_offset[1];
#endif
#ifdef WACOM_USE_OFFSET_TABLE
		if (wac_i2c->use_offset_table) {
			if (x >= 0 && y >= 0)
				wacom_i2c_coord_offset(wac_i2c->wac_pdata, &x, &y);
		}
#endif
#if defined(WACOM_USE_GAIN)
		height = wacom_i2c_coord_level(gain);
#endif

#ifdef WACOM_USE_AVERAGING
		wacom_i2c_coord_average(&x, &y, rdy, aveStrength);
#endif
#ifdef WACOM_USE_BOX_FILTER
		if (pressure == 0)
			boxfilt(&x, &y, height, rdy);
#endif
#endif /*WACOM_IMPORT_FW_ALGO*/
		if (wac_i2c->wac_pdata->x_invert)
			x = wac_i2c->wac_feature->x_max - x;
		if (wac_i2c->wac_pdata->y_invert)
			y = wac_i2c->wac_feature->y_max - y;

		if (wac_i2c->wac_pdata->xy_switch) {
			tmp = x;
			x = y;
			y = tmp;
		}

#ifdef WACOM_USE_TILT_OFFSET
		/* Add offset */
		x = x + tilt_offsetX[user_hand][screen_rotate];
		y = y + tilt_offsetY[user_hand][screen_rotate];
#endif
		if (wacom_i2c_coord_range(wac_i2c, &x, &y)) {
			input_report_abs(wac_i2c->input_dev, ABS_X, x);
			input_report_abs(wac_i2c->input_dev, ABS_Y, y);
			input_report_abs(wac_i2c->input_dev,
					 ABS_PRESSURE, pressure);
#ifdef WACOM_USE_GAIN
		input_report_abs(wac_i2c->input_dev,
			ABS_DISTANCE, gain);
#endif
			input_report_key(wac_i2c->input_dev,
					 BTN_STYLUS, stylus);
			if (!stylus && !wac_i2c->side_pressed) {
				input_report_key(wac_i2c->input_dev, BTN_TOUCH, prox);
			}
			input_report_key(wac_i2c->input_dev, wac_i2c->tool, 1);
/*
#ifdef WACOM_PDCT_WORK_AROUND
			if (wac_i2c->rdy_pdct) {
				wac_i2c->rdy_pdct = false;
				input_report_key(wac_i2c->input_dev,
					KEY_PEN_PDCT, 0);
			}
#endif
*/
			input_sync(wac_i2c->input_dev);
			wac_i2c->last_x = x;
			wac_i2c->last_y = y;

			if (prox && !wac_i2c->pen_pressed) {
#ifdef USE_WACOM_BLOCK_KEYEVENT
				wac_i2c->touch_pressed = true;
#endif
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
				dev_info(&wac_i2c->client->dev,
				       "%s: is pressed(%d,%d,%d)(0x%x)\n",
				       __func__, x, y, pressure, wac_i2c->tool);
#else
				dev_info(&wac_i2c->client->dev,
						"%s: pressed\n",
						__func__);
#endif

			} else if (!prox && wac_i2c->pen_pressed) {
#ifdef USE_WACOM_BLOCK_KEYEVENT
				schedule_delayed_work(&wac_i2c->touch_pressed_work,
					msecs_to_jiffies(wac_i2c->key_delay_time));
#endif
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
				dev_info(&wac_i2c->client->dev,
				       "%s: is released(%d,%d,%d)(0x%x)\n",
				       __func__, x, y, pressure, wac_i2c->tool);
#else
				dev_info(&wac_i2c->client->dev,
						"%s: released\n",
						__func__);
#endif
			}

			wac_i2c->pen_pressed = prox;

			if (stylus && !wac_i2c->side_pressed) {
				dev_info(&wac_i2c->client->dev,
						"%s: side on\n",
						__func__);
				wac_i2c->gesture_key = -1;
				wac_i2c->gesture_start_x = x;
				wac_i2c->gesture_start_y = y;
				wac_i2c->gesture_start_time = ktime_get();
			} else if (!stylus && wac_i2c->side_pressed) {
				dev_info(&wac_i2c->client->dev,
						"%s: side off\n",
						__func__);
				handle_gestures(x, y, ktime_get(), wac_i2c);
				if (wac_i2c->gesture_key > -1 &&
						(wac_i2c->enabled_gestures &
						(1 << (wac_i2c->gesture_key - 0x2f1)))) {
					input_report_key(wac_i2c->input_dev,
							wac_i2c->gesture_key, 1);
					input_sync(wac_i2c->input_dev);
					input_report_key(wac_i2c->input_dev,
							wac_i2c->gesture_key, 0);
					input_sync(wac_i2c->input_dev);
				}
			}

			wac_i2c->side_pressed = stylus;
		}
#if defined(CONFIG_SAMSUNG_KERNEL_DEBUG_USER)
		else
			dev_info(&wac_i2c->client->dev,
					"%s: raw data x=%d, y=%d\n",
					__func__, x, y);
#endif
	} else {

#ifdef WACOM_USE_AVERAGING
		/* enable emr device */
		wacom_i2c_coord_average(0, 0, 0, 0);
#endif
#ifdef WACOM_USE_BOX_FILTER
		boxfilt(0, 0, 0, 0);
#endif
/*
#ifdef WACOM_PDCT_WORK_AROUND
		if (wac_i2c->pen_pdct == PDCT_DETECT_PEN)
			forced_hover(wac_i2c);
		else
#endif
*/
		if (wac_i2c->pen_prox) {
			/* input_report_abs(wac->input_dev,
			   ABS_X, x); */
			/* input_report_abs(wac->input_dev,
			   ABS_Y, y); */

			input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
#ifdef WACOM_USE_GAIN
			input_report_abs(wac_i2c->input_dev, ABS_DISTANCE, 0);
#endif
			input_report_key(wac_i2c->input_dev, BTN_STYLUS, 0);
			input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);

#if 0/*defined(WACOM_PDCT_WORK_AROUND)*/
			input_report_key(wac_i2c->input_dev,
				BTN_TOOL_RUBBER, 0);
			input_report_key(wac_i2c->input_dev, BTN_TOOL_PEN, 0);
			input_report_key(wac_i2c->input_dev, KEY_PEN_PDCT, 0);
#else
			input_report_key(wac_i2c->input_dev, wac_i2c->tool, 0);
#endif
			input_sync(wac_i2c->input_dev);

#ifdef USE_WACOM_BLOCK_KEYEVENT
			schedule_delayed_work(&wac_i2c->touch_pressed_work,
					msecs_to_jiffies(wac_i2c->key_delay_time));
#endif
			dev_info(&wac_i2c->client->dev,
					"%s: is out\n",
					__func__);
		}
		wac_i2c->pen_prox = 0;
		wac_i2c->pen_pressed = 0;
		wac_i2c->side_pressed = 0;
		wac_i2c->last_x = 0;
		wac_i2c->last_y = 0;

#ifdef WACOM_BOOSTER
		wacom_set_dvfs_lock(wac_i2c, 0);
#endif
	}

	return 0;
}
