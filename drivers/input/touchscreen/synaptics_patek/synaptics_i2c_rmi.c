/* Synaptics Register Mapped Interface (RMI4) I2C Physical Layer Driver.
 * Copyright (c) 2007-2012, Synaptics Incorporated
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "synaptics_i2c_rmi.h"

extern int boot_mode_recovery;
extern int system_rev;
static struct list_head exp_fn_list;

#ifdef TSP_PATTERN_TRACKING_METHOD
/* Below is used for clearing ghost touch or for checking to system reboot.  by Xtopher */
static int tcount_finger[MAX_GHOSTCHECK_FINGER] = {0,0,0,0,0,0,0,0,0,0};
static int touchbx[MAX_GHOSTCHECK_FINGER] = {0,0,0,0,0,0,0,0,0,0};
static int touchby[MAX_GHOSTCHECK_FINGER] = {0,0,0,0,0,0,0,0,0,0};
static int ghosttouchcount = 0;

static bool tsp_pattern_tracking(int fingerindex, int x, int y);
#endif
#ifdef PROXIMITY
static struct synaptics_rmi4_f51_handle *f51;
#endif

#ifdef CONFIG_DUAL_LCD
static struct synaptics_rmi4_data *tsp_driver = NULL;
void synaptics_set_tsp_info(struct synaptics_rmi4_data *rmi4_data)
{
	if (rmi4_data != NULL)
		tsp_driver = rmi4_data;
	else
		pr_info("%s : tsp info is null\n", __func__);
}

static struct synaptics_rmi4_data *synaptics_get_tsp_info(void)
{
	return tsp_driver;
}
#endif

static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data,
		unsigned short length);
static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data,
		unsigned short length);
static int synaptics_rmi4_reinit_device(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_reset_device(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_stop_device(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_start_device(struct synaptics_rmi4_data *rmi4_data);
static void synaptics_rmi4_release_all_finger(struct synaptics_rmi4_data *rmi4_data);
#ifdef PROXIMITY
static void synaptics_rmi4_f51_finger_timer(unsigned long data);
static ssize_t synaptics_rmi4_f51_enables_show(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t synaptics_rmi4_f51_enables_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
#endif
static ssize_t synaptics_rmi4_f01_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
static ssize_t synaptics_rmi4_f01_productinfo_show(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t synaptics_rmi4_f01_buildid_show(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t synaptics_rmi4_f01_flashprog_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static struct device_attribute attrs[] = {
#ifdef PROXIMITY
	__ATTR(proximity_enables, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_f51_enables_show,
			synaptics_rmi4_f51_enables_store),
#endif
	__ATTR(reset, S_IWUSR | S_IWGRP,
			synaptics_rmi4_show_error,
			synaptics_rmi4_f01_reset_store),
	__ATTR(productinfo, S_IRUGO,
			synaptics_rmi4_f01_productinfo_show,
			synaptics_rmi4_store_error),
	__ATTR(buildid, S_IRUGO,
			synaptics_rmi4_f01_buildid_show,
			synaptics_rmi4_store_error),
	__ATTR(flashprog, S_IRUGO,
			synaptics_rmi4_f01_flashprog_show,
			synaptics_rmi4_store_error),
};

#ifdef CONFIG_OF
static int synaptics_parse_dt(struct device *dev,
			struct synaptics_rmi4_device_tree_data *dt_data)
{
	struct device_node *np = dev->of_node;
	int i, rc;

	dt_data->tsp_int = of_get_named_gpio(np, "synaptics,irq_gpio", 0);
	dt_data->tsp_sda = of_get_named_gpio(np, "synaptics,tsp_sda", 0);
	dt_data->tsp_scl  = of_get_named_gpio(np, "synaptics,tsp_scl", 0);
	dt_data->tsp_sel = of_get_named_gpio(np, "synaptics,tsp_sel", 0);
	dt_data->hall_ic = of_get_named_gpio(np, "synaptics,hall_flip-gpio", 0);
	dt_data->fpga_mainclk = of_get_named_gpio(np, "synaptics,fpga_mainclk", 0);
	dt_data->cresetb = of_get_named_gpio(np, "synaptics,cresetb", 0);
	dt_data->cdone = of_get_named_gpio(np, "synaptics,cdone", 0);

	gpio_tlmm_config(GPIO_CFG(dt_data->tsp_int, 0, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	gpio_tlmm_config(GPIO_CFG(dt_data->fpga_mainclk, 2, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	gpio_tlmm_config(GPIO_CFG(dt_data->cresetb, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL,GPIO_CFG_2MA), 1);

	gpio_tlmm_config(GPIO_CFG(dt_data->cdone, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL,GPIO_CFG_2MA), 1);

	rc = gpio_request(dt_data->tsp_int, "tsp_int");
	if (rc)
		dev_err(dev, "%s: error to request interrupt gpio : %d\n", __func__, rc);

	gpio_direction_input(dt_data->tsp_int);

	rc = gpio_request(dt_data->cresetb, "fpga_creset");
	if (rc)
		dev_err(dev, "%s: error to request fpga cresetb gpio : %d\n", __func__, rc);

	rc = gpio_direction_output(dt_data->cresetb, 1);
	if (rc)
		dev_err(dev, "%s: error to set cresetb : %d\n", __func__, rc);

#ifdef CHARGER_NOTIFIER
	rc = gpio_request(GPIO_TSP_TA, "tsp_ta");
	if (rc)
		dev_err(dev, "%s: error to request tsp_ta gpio : %d\n", __func__, rc);
#endif

	rc = of_property_read_u32(np, "synaptics,supply-num", &dt_data->num_of_supply);
	if (dt_data->num_of_supply > 0) {
		dt_data->name_of_supply = kzalloc(sizeof(char *) * dt_data->num_of_supply, GFP_KERNEL);
		for (i = 0; i < dt_data->num_of_supply; i++) {
			rc = of_property_read_string_index(np, "synaptics,supply-name",
				i, &dt_data->name_of_supply[i]);
			if (rc && (rc != -EINVAL)) {
				dev_err(dev, "%s: Unable to read %s\n", __func__,
						"synaptics,supply-name");
			}
			dev_info(dev, "%s: supply%d: %s\n", __func__, i, dt_data->name_of_supply[i]);
		}
	}
	dev_err(dev, "%s: int=%d, sda=%d, scl=%d, sel=%d, cresetb=%d, cdone=%d\n",
		__func__, dt_data->tsp_int, dt_data->tsp_sda, dt_data->tsp_scl,
		dt_data->tsp_sel, dt_data->cresetb, dt_data->cdone);
	return 0;
}
#else
static int synaptics_parse_dt(struct device *dev,
			struct synaptics_rmi4_device_tree_data *dt_data)
{
	return -ENODEV;
}
#endif

#ifdef TSP_BOOSTER
static void synaptics_change_dvfs_lock(struct work_struct *work)
{
	struct synaptics_rmi4_data *rmi4_data =
		container_of(work,
			struct synaptics_rmi4_data, work_dvfs_chg.work);
	int retval = 0;

	mutex_lock(&rmi4_data->dvfs_lock);

	if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_DUAL) {
		if (rmi4_data->stay_awake) {
			dev_info(&rmi4_data->i2c_client->dev,
				"%s: do fw update, do not change cpu frequency.\n",
				__func__);
		} else {
		retval = set_freq_limit(DVFS_TOUCH_ID,
				MIN_TOUCH_LIMIT_SECOND);
		rmi4_data->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
		}
	} else if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_SINGLE
			|| rmi4_data->dvfs_boost_mode == DVFS_STAGE_TRIPLE) {
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		rmi4_data->dvfs_freq = -1;
	}

	if (retval < 0)
		dev_err(&rmi4_data->i2c_client->dev,
			"%s: booster change failed(%d).\n",
			__func__, retval);
	mutex_unlock(&rmi4_data->dvfs_lock);

}

static void synaptics_set_dvfs_off(struct work_struct *work)
{
	struct synaptics_rmi4_data *rmi4_data =
		container_of(work,
			struct synaptics_rmi4_data, work_dvfs_off.work);
	int retval;

	if (rmi4_data->stay_awake) {
		dev_info(&rmi4_data->i2c_client->dev,
			"%s: do fw update, do not change cpu frequency.\n",
			__func__);
	} else {
		mutex_lock(&rmi4_data->dvfs_lock);

		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		rmi4_data->dvfs_freq = -1;

		if (retval < 0)
			dev_err(&rmi4_data->i2c_client->dev,
				"%s: booster stop failed(%d).\n",
				__func__, retval);
		rmi4_data->dvfs_lock_status = false;

		mutex_unlock(&rmi4_data->dvfs_lock);
	}
}

static void synaptics_set_dvfs_lock(struct synaptics_rmi4_data *rmi4_data,
					int on)
{
	int ret = 0;

	if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_NONE) {
		dev_info(&rmi4_data->i2c_client->dev,
				"%s: DVFS stage is none(%d)\n",
				__func__, rmi4_data->dvfs_boost_mode);
		return;
	}

	mutex_lock(&rmi4_data->dvfs_lock);
	if (on == 0) {
		if (rmi4_data->dvfs_lock_status) {
			schedule_delayed_work(&rmi4_data->work_dvfs_off,
				msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));
		}
	} else if (on > 0) {
		cancel_delayed_work(&rmi4_data->work_dvfs_off);

		if ((!rmi4_data->dvfs_lock_status) || (rmi4_data->dvfs_old_stauts < on)) {
			cancel_delayed_work(&rmi4_data->work_dvfs_chg);
				if (rmi4_data->dvfs_freq != MIN_TOUCH_LIMIT) {
				if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_TRIPLE) 
					ret = set_freq_limit(DVFS_TOUCH_ID,
						MIN_TOUCH_LIMIT_SECOND);
				else
					ret = set_freq_limit(DVFS_TOUCH_ID,
							MIN_TOUCH_LIMIT);
					rmi4_data->dvfs_freq = MIN_TOUCH_LIMIT;

					if (ret < 0)
						dev_err(&rmi4_data->i2c_client->dev,
							"%s: cpu first lock failed(%d)\n",
							__func__, ret);
				}

				schedule_delayed_work(&rmi4_data->work_dvfs_chg,
					msecs_to_jiffies(TOUCH_BOOSTER_CHG_TIME));

				rmi4_data->dvfs_lock_status = true;
		}
	} else if (on < 0) {
		if (rmi4_data->dvfs_lock_status) {
			cancel_delayed_work(&rmi4_data->work_dvfs_off);
			cancel_delayed_work(&rmi4_data->work_dvfs_chg);
			schedule_work(&rmi4_data->work_dvfs_off.work);
		}
	}
	rmi4_data->dvfs_old_stauts = on;
	mutex_unlock(&rmi4_data->dvfs_lock);
}

static void synaptics_init_dvfs(struct synaptics_rmi4_data *rmi4_data)
{
	mutex_init(&rmi4_data->dvfs_lock);

	rmi4_data->dvfs_boost_mode = DVFS_STAGE_DUAL;

	INIT_DELAYED_WORK(&rmi4_data->work_dvfs_off, synaptics_set_dvfs_off);
	INIT_DELAYED_WORK(&rmi4_data->work_dvfs_chg, synaptics_change_dvfs_lock);

	rmi4_data->dvfs_lock_status = false;
}
#endif

#ifdef PROXIMITY
static ssize_t synaptics_rmi4_f51_enables_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (!f51)
		return -ENODEV;

	return snprintf(buf, PAGE_SIZE, "0x%02x\n",
			f51->proximity_enables);
}

static ssize_t synaptics_rmi4_f51_enables_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!f51)
		return -ENODEV;

	if (sscanf(buf, "%x", &input) != 1)
		return -EINVAL;

	f51->proximity_enables = (unsigned char)input;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			f51->proximity_enables_addr,
			&f51->proximity_enables,
			sizeof(f51->proximity_enables));
	if (retval < 0) {
		dev_err(dev, "%s: Failed to write proximity enables, error = %d\n",
				__func__, retval);
		return retval;
	}

	return count;
}
#endif

static ssize_t synaptics_rmi4_f01_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int reset;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (sscanf(buf, "%u", &reset) != 1)
		return -EINVAL;

	if (reset != 1)
		return -EINVAL;

	retval = synaptics_rmi4_reset_device(rmi4_data);
	if (retval < 0) {
		dev_err(dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_f01_productinfo_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "0x%02x 0x%02x\n",
			(rmi4_data->rmi4_mod_info.product_info[0]),
			(rmi4_data->rmi4_mod_info.product_info[1]));
}

static ssize_t synaptics_rmi4_f01_buildid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int build_id;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	build_id = (unsigned int)rmi->build_id[0] +
			(unsigned int)rmi->build_id[1] * 0x100 +
			(unsigned int)rmi->build_id[2] * 0x10000;

	return snprintf(buf, PAGE_SIZE, "%u\n",
			build_id);
}

static ssize_t synaptics_rmi4_f01_flashprog_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int retval;
	struct synaptics_rmi4_f01_device_status device_status;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			device_status.data,
			sizeof(device_status.data));
	if (retval < 0) {
		dev_err(dev,
				"%s: Failed to read device status, error = %d\n",
				__func__, retval);
		return retval;
	}

	return snprintf(buf, PAGE_SIZE, "%u\n",
			device_status.flash_prog);
}

 /**
 * synaptics_rmi4_set_page()
 *
 * Called by synaptics_rmi4_i2c_read() and synaptics_rmi4_i2c_write().
 *
 * This function writes to the page select register to switch to the
 * assigned page.
 */
static int synaptics_rmi4_set_page(struct synaptics_rmi4_data *rmi4_data,
		unsigned int address)
{
	int retval = 0;
	unsigned char retry;
	unsigned char buf[PAGE_SELECT_LEN];
	unsigned char page;
	struct i2c_client *i2c = rmi4_data->i2c_client;

	page = ((address >> 8) & MASK_8BIT);
	if (page != rmi4_data->current_page) {
		buf[0] = MASK_8BIT;
		buf[1] = page;
		for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
			retval = i2c_master_send(i2c, buf, PAGE_SELECT_LEN);
			if (retval != PAGE_SELECT_LEN) {
				if((rmi4_data->tsp_probe != true)&&(retry>=1)){
				dev_err(&i2c->dev,
							"%s: TSP needs to reboot \n",__func__);
					retval = TSP_NEEDTO_REBOOT;
					return retval;
				}
				dev_err(&i2c->dev,
						"%s: I2C retry = %d, i2c_master_send retval = %d\n",
						__func__, retry + 1, retval);
				if (retval == 0)
					retval = -EAGAIN;
				msleep(20);
			} else {
				rmi4_data->current_page = page;
				break;
			}
		}
	} else {
		retval = PAGE_SELECT_LEN;
	}

	return retval;
}

 /**
 * synaptics_rmi4_i2c_read()
 *
 * Called by various functions in this driver, and also exported to
 * other expansion Function modules such as rmi_dev.
 *
 * This function reads data of an arbitrary length from the sensor,
 * starting from an assigned register address of the sensor, via I2C
 * with a retry mechanism.
 */
static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	int retval;
	unsigned char retry;
	unsigned char buf;
	struct i2c_msg msg[] = {
		{
			.addr = rmi4_data->i2c_client->addr,
			.flags = 0,
			.len = 1,
			.buf = &buf,
		},
		{
			.addr = rmi4_data->i2c_client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		},
	};

	buf = addr & MASK_8BIT;

	mutex_lock(&(rmi4_data->rmi4_io_ctrl_mutex));

	if (rmi4_data->touch_stopped) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: Sensor stopped\n",
				__func__);
		retval = 0;
		goto exit;
	}

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN)
		goto exit;

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(rmi4_data->i2c_client->adapter, msg, 2) == 2) {
			retval = length;
			break;
		}
		dev_err(&rmi4_data->i2c_client->dev, "%s: I2C retry %d\n",
				__func__, retry + 1);
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: I2C read over retry limit\n",
				__func__);
		retval = -EIO;
	}

exit:
	mutex_unlock(&(rmi4_data->rmi4_io_ctrl_mutex));

	return retval;
}

 /**
 * synaptics_rmi4_i2c_write()
 *
 * Called by various functions in this driver, and also exported to
 * other expansion Function modules such as rmi_dev.
 *
 * This function writes data of an arbitrary length to the sensor,
 * starting from an assigned register address of the sensor, via I2C with
 * a retry mechanism.
 */
static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	int retval;
	unsigned char retry;
	unsigned char buf[length + 1];
	struct i2c_msg msg[] = {
		{
			.addr = rmi4_data->i2c_client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	mutex_lock(&(rmi4_data->rmi4_io_ctrl_mutex));

	if (rmi4_data->touch_stopped) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: Sensor stopped\n",
				__func__);
		retval = 0;
		goto exit;
	}

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN)
		goto exit;

	buf[0] = addr & MASK_8BIT;
	memcpy(&buf[1], &data[0], length);

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(rmi4_data->i2c_client->adapter, msg, 1) == 1) {
			retval = length;
			break;
		}
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: I2C retry %d\n",
				__func__, retry + 1);
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: I2C write over retry limit\n",
				__func__);
		retval = -EIO;
	}

exit:
	mutex_unlock(&(rmi4_data->rmi4_io_ctrl_mutex));

	return retval;
}

#ifdef TSP_PATTERN_TRACKING_METHOD
static void clear_tcount(void)
{
	int i;
	for(i=0;i<MAX_GHOSTCHECK_FINGER;i++){
		tcount_finger[i] = 0;
		touchbx[i] = 0;
		touchby[i] = 0;
	}		 
}
 
static int diff_two_point(int x, int y, int oldx, int oldy)
{
	int diffx,diffy;
	int distance;

	diffx = x-oldx;
	diffy = y-oldy;
	distance = abs(diffx) + abs(diffy);

	if(distance < PATTERN_TRACKING_DISTANCE) return 1;
	else return 0;
}

static bool IsEdgeArea(int x, int y)
{
	bool fEdge = false;
	if((x <= MIN_X_EDGE)||(x >= MAX_X_EDGE)||(y <= MIN_Y_EDGE)||(y >= MAX_Y_EDGE))fEdge = true;
	return fEdge;
}


/* To do forced calibration when ghost touch occured at the same point
 for several second.   Xtopher */
static bool tsp_pattern_tracking(int fingerindex, int x, int y)
{
	int i;
	bool ghosttouch  = false;

	if(!IsEdgeArea(x,y)) return ghosttouch;

	for( i = 0; i< MAX_GHOSTCHECK_FINGER; i++)
	{
		if( i == fingerindex){
			//if((touchbx[i] == x)&&(touchby[i] == y))
			if(diff_two_point(x,y, touchbx[i], touchby[i]))
			{
				tcount_finger[i] = tcount_finger[i]+1;
			}
			else
			{
				tcount_finger[i] = 0;
			}

			touchbx[i] = x;
			touchby[i] = y;

			if(tcount_finger[i]> MAX_GHOSTTOUCH_COUNT){
				clear_tcount();
				ghosttouch = true;
				return ghosttouch;
			}
		}
	}
	return ghosttouch;
}
#endif

 /**
 * synaptics_rmi4_f12_abs_report()
 *
 * Called by synaptics_rmi4_report_touch() when valid Function $12
 * finger data has been detected.
 *
 * This function reads the Function $12 data registers, determines the
 * status of each finger supported by the Function, processes any
 * necessary coordinate manipulation, reports the finger data to
 * the input subsystem, and returns the number of fingers detected.
 */
static int synaptics_rmi4_f12_abs_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned char touch_count = 0; /* number of touch points */
	unsigned char finger;
	unsigned char fingers_supported;
	unsigned char finger_status;
	unsigned short data_addr;
	int x;
	int y;
	int wx;
	int wy;
	struct synaptics_rmi4_f12_finger_data *data;
	struct synaptics_rmi4_f12_finger_data *finger_data;

	fingers_supported = fhandler->num_of_data_points;
	data_addr = fhandler->full_addr.data_base;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_addr + fhandler->data1_offset,
			(unsigned char *)fhandler->data,
			fhandler->data_size);
	if (retval < 0)
		return 0;

	data = (struct synaptics_rmi4_f12_finger_data *)fhandler->data;

	for (finger = 0; finger < fingers_supported; finger++) {
		finger_data = data + finger;
		finger_status = finger_data->object_type_and_status;
		x = (finger_data->x_msb << 8) | (finger_data->x_lsb);
		y = (finger_data->y_msb << 8) | (finger_data->y_lsb);

		if ((x == INVALID_X) && (y == INVALID_Y))
			finger_status = 0;

		if (finger_status) {
			if ((finger_data->wx == 0) && (finger_data->wy == 0))
				continue;
		}

		/* block palm touch temporary */
		if (finger_status == 0x03)
			return 0;

		/*
		 * Each 3-bit finger status field represents the following:
		 * 000 = finger not present
		 * 001 = finger present and data accurate
		 * 010 = finger present but data may be inaccurate
		 * 011 = palm
		 * 110 = glove touch
		 */

#ifdef TYPE_B_PROTOCOL
		input_mt_slot(rmi4_data->input_dev, finger);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, finger_status);
#endif

		if (finger_status) {
#ifdef CONFIG_GLOVE_TOUCH
			if ((finger_status == 0x06) &&
				!rmi4_data->touchkey_glove_mode_status) {
				rmi4_data->touchkey_glove_mode_status = true;
				input_report_switch(rmi4_data->input_dev,
					SW_GLOVE, true);
			} else if ((finger_status != 0x06) &&
				rmi4_data->touchkey_glove_mode_status) {
				rmi4_data->touchkey_glove_mode_status = false;
				input_report_switch(rmi4_data->input_dev,
					SW_GLOVE, false);
			}
#endif
#ifdef REPORT_2D_W
			wx = finger_data->wx;
			wy = finger_data->wy;
#endif
			if (rmi4_data->dt_data->x_flip)
				x = rmi4_data->sensor_max_x - x;
			if (rmi4_data->dt_data->y_flip)
				y = rmi4_data->sensor_max_y - y;

			input_report_key(rmi4_data->input_dev,
					BTN_TOUCH, 1);
			input_report_key(rmi4_data->input_dev,
					BTN_TOOL_FINGER, 1);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_X, x);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_Y, y);
#ifdef REPORT_2D_W
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MAJOR, max(wx, wy));
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MINOR, min(wx, wy));
#endif
#ifndef TYPE_B_PROTOCOL
			input_mt_sync(rmi4_data->input_dev);
#endif



			if (!rmi4_data->finger[finger].state) {
				#ifdef TSP_PATTERN_TRACKING_METHOD
				/* Check hopping on almost fixed point */
				if(tsp_pattern_tracking(finger,x,y)){
					dev_err(&rmi4_data->i2c_client->dev,
							"Sunflower-Hopping (Pattern Tracking)\n");
					cancel_delayed_work(&rmi4_data->reboot_work);
					schedule_delayed_work(&rmi4_data->reboot_work,
						msecs_to_jiffies(TSP_REBOOT_PENDING_TIME*6)); /* 300msec*/
					touch_count = 0;
					return touch_count;
				}
				#endif

				dev_info(&rmi4_data->i2c_client->dev, "[%d][P] 0x%02x (%d,%d)\n",
					finger, finger_status, x, y);
			} else {
				rmi4_data->finger[finger].mcount++;
				#ifdef TSP_PATTERN_TRACKING_METHOD
				/* Check staying finger at one point */
				if((rmi4_data->finger[finger].mcount%MOVE_COUNT_TH) == 0){
					if(tsp_pattern_tracking(finger,x,y)){
						dev_err(&rmi4_data->i2c_client->dev,
								"Sunflower-Fixed (Pattern Tracking)\n");
						cancel_delayed_work(&rmi4_data->reboot_work);
						schedule_delayed_work(&rmi4_data->reboot_work,
							msecs_to_jiffies(TSP_REBOOT_PENDING_TIME*6)); /* 300msec*/
						touch_count = 0;
						return touch_count;
					}
				}
				#endif
			}
			touch_count++;
		}

		if (rmi4_data->finger[finger].state && !finger_status) {
			dev_info(&rmi4_data->i2c_client->dev, "[%d][R] 0x%02x M[%d], Ver[%02X%02X%02X]\n",
				finger, finger_status, rmi4_data->finger[finger].mcount,
				rmi4_data->ic_revision_of_ic,
				rmi4_data->fw_version_of_ic, rmi4_data->glove_mode_enables);
			rmi4_data->finger[finger].mcount = 0;
		}

		rmi4_data->finger[finger].state = finger_status;
	}

	if (touch_count == 0) {
		/* Clear BTN_TOUCH when All touch are released  */
		input_report_key(rmi4_data->input_dev,
				BTN_TOUCH, 0);
#ifndef TYPE_B_PROTOCOL
		input_mt_sync(rmi4_data->input_dev);
#endif
	}

	input_sync(rmi4_data->input_dev);

#ifdef TSP_BOOSTER
	if (touch_count)
		synaptics_set_dvfs_lock(rmi4_data, touch_count);
	else
		synaptics_set_dvfs_lock(rmi4_data, 0);
#endif
	return touch_count;
}

#ifdef PROXIMITY
static void synaptics_rmi4_f51_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned short data_base_addr;
	int x;
	int y;
	int z;
	struct synaptics_rmi4_f51_data *data;

	data_base_addr = fhandler->full_addr.data_base;
	data = (struct synaptics_rmi4_f51_data *)fhandler->data;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_base_addr,
			data->proximity_data,
			sizeof(data->proximity_data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read proximity data registers\n",
				__func__);
		return;
	}

	if (data->proximity_data[0] == 0x00) {
		if (rmi4_data->f51_finger_is_hover) {
			dev_info (&rmi4_data->i2c_client->dev,
				"Hover finger[OUT]\n");
			rmi4_data->f51_finger_is_hover = false;
		}
		return;
	}

	if (data->finger_hover_det && (data->hover_finger_z > 0)) {
		x = (data->hover_finger_x_4__11 << 4) |
				(data->hover_finger_xy_0__3 & 0x0f);
		y = (data->hover_finger_y_4__11 << 4) |
				(data->hover_finger_xy_0__3 >> 4);
		z = HOVER_Z_MAX - data->hover_finger_z;

#ifdef TYPE_B_PROTOCOL
		input_mt_slot(rmi4_data->input_dev, 0);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, 1);
#endif
		input_report_key(rmi4_data->input_dev,
				BTN_TOUCH, 0);
		input_report_key(rmi4_data->input_dev,
				BTN_TOOL_FINGER, 1);
		input_report_abs(rmi4_data->input_dev,
				ABS_MT_POSITION_X, x);
		input_report_abs(rmi4_data->input_dev,
				ABS_MT_POSITION_Y, y);
		input_report_abs(rmi4_data->input_dev,
				ABS_MT_DISTANCE, z);

#ifndef TYPE_B_PROTOCOL
		input_mt_sync(rmi4_data->input_dev);
#endif
		input_sync(rmi4_data->input_dev);

		if (!rmi4_data->f51_finger_is_hover) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			dev_info(&rmi4_data->i2c_client->dev,
				"Hover finger[IN]: x = %d, y = %d, z = %d\n", x, y, z);
#else
			dev_info(&rmi4_data->i2c_client->dev,
				"Hover finger[IN]\n");
#endif
			rmi4_data->f51_finger_is_hover = true;
		}

		rmi4_data->f51_finger = true;
		rmi4_data->fingers_on_2d = false;
		synaptics_rmi4_f51_finger_timer((unsigned long)rmi4_data);
	}

	if (data->air_swipe_det) {
		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: Swipe direction 0 = %d\n",
				__func__, data->air_swipe_dir_0);
		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: Swipe direction 1 = %d\n",
				__func__, data->air_swipe_dir_1);
	}

	if (data->large_obj_det) {
		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: Large object activity = %d\n",
				__func__, data->large_obj_act);
	}
/*
	if (data->hover_pinch_det) {
		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: Hover pinch direction = %d\n",
				__func__, data->hover_pinch_dir);
	}
*/
	if (data->object_present) {
		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: Object presence detected\n",
				__func__);
	}

	return;
}
#endif

 /**
 * synaptics_rmi4_report_touch()
 *
 * Called by synaptics_rmi4_sensor_report().
 *
 * This function calls the appropriate finger data reporting function
 * based on the function handler it receives and returns the number of
 * fingers detected.
 */
static void synaptics_rmi4_report_touch(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	unsigned char touch_count_2d;

	dev_dbg(&rmi4_data->i2c_client->dev,
			"%s: Function %02x reporting\n",
			__func__, fhandler->fn_number);

	switch (fhandler->fn_number) {
	case SYNAPTICS_RMI4_F12:
		touch_count_2d = synaptics_rmi4_f12_abs_report(rmi4_data,
				fhandler);

		if (touch_count_2d)
			rmi4_data->fingers_on_2d = true;
		else
			rmi4_data->fingers_on_2d = false;
		break;
#ifdef PROXIMITY
	case SYNAPTICS_RMI4_F51:
		synaptics_rmi4_f51_report(rmi4_data, fhandler);
		break;
#endif
	default:
		break;
	}

	return;
}

 /**
 * synaptics_rmi4_sensor_report()
 *
 * Called by synaptics_rmi4_irq().
 *
 * This function determines the interrupt source(s) from the sensor
 * and calls synaptics_rmi4_report_touch() with the appropriate
 * function handler for each function with valid data inputs.
 */
static int synaptics_rmi4_sensor_report(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char data[MAX_INTR_REGISTERS + 1];
	unsigned char *intr = &data[1];
	struct synaptics_rmi4_f01_device_status status;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_exp_fn *exp_fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	/*
	 * Get interrupt status information from F01 Data1 register to
	 * determine the source(s) that are flagging the interrupt.
	 */
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			data,
			rmi4_data->num_of_intr_regs + 1);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read interrupt status\n",
				__func__);
		return retval;
	}

	status.data[0] = data[0];
	if (status.unconfigured) {
		if (rmi4_data->doing_reflash) {
			dev_err(&rmi4_data->i2c_client->dev,
				"Spontaneous reset detected during reflash.\n");
			return 0;
		}

		dev_info(&rmi4_data->i2c_client->dev,
			"Spontaneous reset detected\n");
		retval = synaptics_rmi4_reinit_device(rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to reinit device\n",
					__func__);
		}
		return 0;
	}

	/*
	 * Traverse the function handler list and service the source(s)
	 * of the interrupt accordingly.
	 */
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->num_of_data_sources) {
				if (fhandler->intr_mask &
						intr[fhandler->intr_reg_num]) {
					synaptics_rmi4_report_touch(rmi4_data,
							fhandler);
				}
			}
		}
	}

	if (!list_empty(&exp_fn_list)) {
		list_for_each_entry(exp_fhandler, &exp_fn_list, link) {
			if (exp_fhandler->func_attn != NULL)
				exp_fhandler->func_attn(rmi4_data, intr[0]);
		}
	}

	return 0;
}

 /**
 * synaptics_rmi4_irq()
 *
 * Called by the kernel when an interrupt occurs (when the sensor
 * asserts the attention irq).
 *
 * This function is the ISR thread and handles the acquisition
 * and the reporting of finger data when the presence of fingers
 * is detected.
 */
static irqreturn_t synaptics_rmi4_irq(int irq, void *data)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = data;

	do {
		retval = synaptics_rmi4_sensor_report(rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s: Failed to read",
				__func__);
			goto out;
		}

		if (!rmi4_data->touch_stopped)
			goto out;

	} while (!gpio_get_value(rmi4_data->irq));

out:
	return IRQ_HANDLED;
}

 /**
 * synaptics_rmi4_irq_enable()
 *
 * Called by synaptics_rmi4_probe() and the power management functions
 * in this driver and also exported to other expansion Function modules
 * such as rmi_dev.
 *
 * This function handles the enabling and disabling of the attention
 * irq including the setting up of the ISR thread.
 */
static int synaptics_rmi4_irq_enable(struct synaptics_rmi4_data *rmi4_data,
		bool enable)
{
	int retval = 0;
	unsigned char intr_status;

	if (enable) {
		if (rmi4_data->irq_enabled)
			return retval;

		/* Clear interrupts first */
		retval = synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f01_data_base_addr + 1,
				&intr_status,
				rmi4_data->num_of_intr_regs);
		if (retval < 0)
			return retval;

		retval = request_threaded_irq(rmi4_data->irq, NULL,
					synaptics_rmi4_irq, IRQF_TRIGGER_FALLING,
				DRIVER_NAME, rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to create irq thread\n",
					__func__);
			return retval;
		}

		rmi4_data->irq_enabled = true;
	} else {
		if (rmi4_data->irq_enabled) {
			disable_irq(rmi4_data->irq);
			free_irq(rmi4_data->irq, rmi4_data);
			rmi4_data->irq_enabled = false;
		}
	}

	return retval;
}

#ifdef CONFIG_GLOVE_TOUCH
int synaptics_rmi4_glove_mode_enables(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	if (GLOVE_FEATURE_EN != rmi4_data->glove_mode_feature) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s: Ver[%02X%02X] FW does not support glove mode %02X\n",
			__func__, rmi4_data->ic_revision_of_ic,
			rmi4_data->fw_version_of_ic, rmi4_data->glove_mode_feature);
		return 0;
	}

	if (rmi4_data->touch_stopped)
		return 0;

	dev_info(&rmi4_data->i2c_client->dev, "%s: [%02X]: %s\n",
		 __func__, rmi4_data->glove_mode_enables,
		(rmi4_data->glove_mode_enables == 0x07) ? "fast glove & clear cover enable" :
		(rmi4_data->glove_mode_enables == 0x06) ? "fast glove & flip cover enable" :
		(rmi4_data->glove_mode_enables == 0x05) ? "only fast glove enable" :
		(rmi4_data->glove_mode_enables == 0x03) ? "only clear cover enable" :
		(rmi4_data->glove_mode_enables == 0x02) ? "only flip cover enable" :
		(rmi4_data->glove_mode_enables == 0x01) ? "only glove enable" : "glove disable");

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->glove_mode_enables_addr,
			&rmi4_data->glove_mode_enables,
			sizeof(rmi4_data->glove_mode_enables));
	if (retval < 0)
		return retval;

	return 0;
}
#endif

static int synaptics_rmi4_f12_set_enables(struct synaptics_rmi4_data *rmi4_data,
		unsigned short address)
{
	int retval;
	unsigned char enable_mask;
	static unsigned short ctrl_28_address;

	if (address)
		ctrl_28_address = address;

	enable_mask = RPT_DEFAULT;
#ifdef REPORT_2D_Z
	enable_mask |= RPT_Z;
#endif
#ifdef REPORT_2D_W
	enable_mask |= (RPT_WX | RPT_WY);
#endif

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			ctrl_28_address,
			&enable_mask,
			sizeof(enable_mask));
	if (retval < 0)
		return retval;

	return retval;
}

 /**
 * synaptics_rmi4_f12_init()
 *
 * Called by synaptics_rmi4_query_device().
 *
 * This funtion parses information from the Function 12 registers and
 * determines the number of fingers supported, offset to the data1
 * register, x and y data ranges, offset to the associated interrupt
 * status register, interrupt bit mask, and allocates memory resources
 * for finger data acquisition.
 */
static int synaptics_rmi4_f12_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned char intr_offset;
	unsigned char size_of_2d_data;
	unsigned char ctrl_8_offset;
	unsigned char ctrl_9_offset;
	unsigned char ctrl_23_offset;
	unsigned char ctrl_26_offset;
	unsigned char ctrl_28_offset;
	unsigned char fingers_to_support = F12_FINGERS_TO_SUPPORT;
	struct synaptics_rmi4_f12_query_5 query_5;
	struct synaptics_rmi4_f12_query_8 query_8;
	struct synaptics_rmi4_f12_ctrl_8 ctrl_8;
	struct synaptics_rmi4_f12_ctrl_9 ctrl_9;
	struct synaptics_rmi4_f12_ctrl_23 ctrl_23;
#ifdef CONFIG_GLOVE_TOUCH
	struct synaptics_rmi4_f12_ctrl_26 ctrl_26;
#endif

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 5,
			query_5.data,
			sizeof(query_5.data));
	if (retval < 0)
		return retval;

	ctrl_8_offset = query_5.ctrl0_is_present +
			query_5.ctrl1_is_present +
			query_5.ctrl2_is_present +
			query_5.ctrl3_is_present +
			query_5.ctrl4_is_present +
			query_5.ctrl5_is_present +
			query_5.ctrl6_is_present +
			query_5.ctrl7_is_present;

	ctrl_9_offset = ctrl_8_offset +
			query_5.ctrl8_is_present;

	ctrl_23_offset = ctrl_9_offset +
			query_5.ctrl9_is_present +
			query_5.ctrl10_is_present +
			query_5.ctrl11_is_present +
			query_5.ctrl12_is_present +
			query_5.ctrl13_is_present +
			query_5.ctrl14_is_present +
			query_5.ctrl15_is_present +
			query_5.ctrl16_is_present +
			query_5.ctrl17_is_present +
			query_5.ctrl18_is_present +
			query_5.ctrl19_is_present +
			query_5.ctrl20_is_present +
			query_5.ctrl21_is_present +
			query_5.ctrl22_is_present;

	ctrl_26_offset = ctrl_23_offset +
			query_5.ctrl23_is_present +
			query_5.ctrl24_is_present +
			query_5.ctrl25_is_present;

	ctrl_28_offset = ctrl_26_offset +
			query_5.ctrl26_is_present +
			query_5.ctrl27_is_present;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_23_offset,
			ctrl_23.data,
			sizeof(ctrl_23.data));
	if (retval < 0)
		return retval;

	/* Maximum number of fingers supported */
	fhandler->num_of_data_points = min(ctrl_23.max_reported_objects,
			fingers_to_support);

	rmi4_data->num_of_fingers = fhandler->num_of_data_points;

	retval = synaptics_rmi4_f12_set_enables(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_28_offset);
	if (retval < 0)
		return retval;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 8,
			query_8.data,
			sizeof(query_8.data));
	if (retval < 0)
		return retval;

	/* Determine the presence of the Data0 register */
	fhandler->data1_offset = query_8.data0_is_present;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_8_offset,
			ctrl_8.data,
			sizeof(ctrl_8.data));
	if (retval < 0)
		return retval;

	/* Maximum x and y */
	rmi4_data->sensor_max_x =
			((unsigned short)ctrl_8.max_x_coord_lsb << 0) |
			((unsigned short)ctrl_8.max_x_coord_msb << 8);
	rmi4_data->sensor_max_y =
			((unsigned short)ctrl_8.max_y_coord_lsb << 0) |
			((unsigned short)ctrl_8.max_y_coord_msb << 8);
	dev_dbg(&rmi4_data->i2c_client->dev,
			"%s: Function %02x max x = %d max y = %d\n",
			__func__, fhandler->fn_number,
			rmi4_data->sensor_max_x,
			rmi4_data->sensor_max_y);

	rmi4_data->num_of_rx = ctrl_8.num_of_rx;
	rmi4_data->num_of_tx = ctrl_8.num_of_tx;
	rmi4_data->num_of_node = ctrl_8.num_of_rx * ctrl_8.num_of_tx;

	rmi4_data->max_touch_width = max(rmi4_data->num_of_rx,
			rmi4_data->num_of_tx);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_9_offset,
			ctrl_9.data,
			sizeof(ctrl_9.data));

	if (retval < 0)
		return retval;

	rmi4_data->touch_threshold = (int)ctrl_9.touch_threshold;
	rmi4_data->gloved_sensitivity = (int)ctrl_9.gloved_finger;
	dev_info(&rmi4_data->i2c_client->dev,
			"%s: %02x num_Rx:%d num_Tx:%d, node:%d, threshold:%d, gloved sensitivity:%x\n",
			__func__, fhandler->fn_number,
			rmi4_data->num_of_rx, rmi4_data->num_of_tx,
			rmi4_data->num_of_node, rmi4_data->touch_threshold,
			rmi4_data->gloved_sensitivity);

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) +
			intr_offset);
			ii++)
		fhandler->intr_mask |= 1 << ii;

	size_of_2d_data = sizeof(struct synaptics_rmi4_f12_finger_data);

	/* Allocate memory for finger data storage space */
	fhandler->data_size = fhandler->num_of_data_points * size_of_2d_data;
	fhandler->data = kzalloc(fhandler->data_size, GFP_KERNEL);

#ifdef CONFIG_GLOVE_TOUCH
	rmi4_data->glove_mode_enables_addr = fhandler->full_addr.ctrl_base +
				ctrl_26_offset;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 10,
			ctrl_26.data,
			sizeof(ctrl_26.data));
	if (retval < 0)
		return retval;

	rmi4_data->glove_mode_feature = ctrl_26.glove_feature_enable;
	synaptics_rmi4_glove_mode_enables(rmi4_data);
#endif

	return retval;
}

static int synaptics_rmi4_f34_read_version(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	struct synaptics_rmi4_f34_ctrl_3 ctrl_3;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base,
			ctrl_3.data,
			sizeof(ctrl_3.data));

	if (retval < 0)
		return retval;

	dev_info(&rmi4_data->i2c_client->dev, "%s: [IC] [date, revision, version] [%02d/%02d, 0x%02X, 0x%02X]\n",
		__func__, ctrl_3.fw_release_month, ctrl_3.fw_release_date,
		ctrl_3.fw_release_revision, ctrl_3.fw_release_version);

	rmi4_data->fw_release_date_of_ic =
		(ctrl_3.fw_release_month << 8) | ctrl_3.fw_release_date;
	rmi4_data->ic_revision_of_ic = ctrl_3.fw_release_revision;
	rmi4_data->fw_version_of_ic = ctrl_3.fw_release_version;

	return retval;
}

static int synaptics_rmi4_f34_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	retval = synaptics_rmi4_f34_read_version(rmi4_data, fhandler);
	if (retval < 0)
		return retval;

	fhandler->data = NULL;

	return retval;
}

#ifdef PROXIMITY
#ifdef USE_CUSTOM_REZERO
static int synaptics_rmi4_f51_set_custom_rezero(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char custom_rezero;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			f51->proximity_custom_rezero_addr,
			&custom_rezero, sizeof(custom_rezero));

	if (F51_VERSION == f51->num_of_data_sources)
		custom_rezero |= 0x34;
	else
		custom_rezero |= 0x01;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			f51->proximity_custom_rezero_addr,
			&custom_rezero, sizeof(custom_rezero));

	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: fail to write custom rezero\n",
			__func__);
		return retval;
	}
	dev_info(&rmi4_data->i2c_client->dev, "%s\n", __func__);

	return 0;
}

static void synaptics_rmi4_rezero_work(struct work_struct *work)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data =
			container_of(work, struct synaptics_rmi4_data,
			rezero_work.work);

	/* Do not check hover enable status, because rezero bit does not effect
	 * to doze(mutual only) mdoe.
	 */
	retval = synaptics_rmi4_f51_set_custom_rezero(rmi4_data);
	if (retval < 0)
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to rezero device\n", __func__);
}
#endif

int synaptics_proximity_no_sleep_set(bool enables)
{
	int retval;
	unsigned char no_sleep = 0;

	if (!f51)
		return -ENODEV;

	retval = synaptics_rmi4_i2c_read(f51->rmi4_data,
			f51->rmi4_data->f01_ctrl_base_addr,
			&no_sleep,
			sizeof(no_sleep));

	if (retval <= 0)
		dev_err(&f51->rmi4_data->i2c_client->dev, "%s: fail to read no_sleep[ret:%d]\n",
				__func__, retval);

	if (enables)
		no_sleep |= NO_SLEEP_ON;
	else
		no_sleep &= ~(NO_SLEEP_ON);

	retval = synaptics_rmi4_i2c_write(f51->rmi4_data,
			f51->rmi4_data->f01_ctrl_base_addr,
			&no_sleep,
			sizeof(no_sleep));
	if (retval <= 0)
		dev_err(&f51->rmi4_data->i2c_client->dev, "%s: fail to set no_sleep[%X][ret:%d]\n",
				__func__, no_sleep, retval);

	return retval;
}
EXPORT_SYMBOL(synaptics_proximity_no_sleep_set);

static int synaptics_rmi4_f51_set_enables(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	if(!f51)
		return -1;

	dev_info(&rmi4_data->i2c_client->dev, "%s: %s[0x%x]\n", __func__,
		(f51->proximity_enables & FINGER_HOVER_EN) ? "enable" : "disable", f51->proximity_enables);

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			f51->proximity_enables_addr,
			&f51->proximity_enables,
			sizeof(f51->proximity_enables));
	if (retval < 0)
		return retval;

	return 0;
}

static int synaptics_rmi4_f51_set_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	dev_info(&rmi4_data->i2c_client->dev, "%s: proximity controls:[0X%02X]\n",
			__func__, f51->proximity_controls);

#ifdef USE_CUSTOM_REZERO
		retval = synaptics_rmi4_f51_set_custom_rezero(rmi4_data);
		if (retval < 0)
			return retval;

	/*
	 * Need 100msec to finish acquiring for hover baseline
	 * Synaptics recommend that do not send a hover disable cmd during 100msec.
	 * (after send custom rezero cmd on IC on time)
	 * Under current driver, this delay does not affect to wakeup latency.
	 * But it will affect to latency, below proximity enable code can move into delay work.
	 */
	msleep(SYNAPTICS_REZERO_TIME);
#endif

	retval = synaptics_rmi4_f51_set_enables(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to set enable\n", __func__);
	}

	return retval;
}

static int synaptics_rmi4_f51_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned short intr_offset;
	struct synaptics_rmi4_f51_data *data_register;
	struct synaptics_rmi4_f51_query query_register;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) + intr_offset);
			ii++)
		fhandler->intr_mask |= 1 << ii;

	fhandler->data_size = sizeof(*data_register);
	data_register = kzalloc(fhandler->data_size, GFP_KERNEL);
	fhandler->data = (void *)data_register;

	f51 = kzalloc(sizeof(*f51), GFP_KERNEL);
	f51->rmi4_data = rmi4_data;

	if (F51_VERSION == fhandler->num_of_data_sources) {
		f51->num_of_data_sources = fhandler->num_of_data_sources;
		f51->proximity_enables = SLEEP_PROXIMITY; //AIR_SWIPE_EN | SLEEP_PROXIMITY;
		f51->proximity_enables_addr = fhandler->full_addr.ctrl_base +
			F51_PROXIMITY_ENABLES_OFFSET;
#ifdef USE_CUSTOM_REZERO
		f51->proximity_custom_rezero_addr = fhandler->full_addr.ctrl_base +
			F51_GENERAL_CONTROL_OFFSET;
#endif
	} else {
	f51->proximity_enables = PROXIMITY_DEFAULT;
	f51->proximity_enables_addr = fhandler->full_addr.ctrl_base +
			F51_CTRL54_OFFSET;
#ifdef USE_CUSTOM_REZERO
	f51->proximity_custom_rezero_addr = fhandler->full_addr.ctrl_base +
			F51_CTRL78_OFFSET;
#endif
	}
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base,
			query_register.data,
			sizeof(query_register));
	if (retval < 0)
		return retval;

	/* Save proximity controls(query 4 register) to get which functions
	 * are supported by firmware.
	 */
	f51->proximity_controls = query_register.proximity_controls;

	retval = synaptics_rmi4_f51_set_init(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: fail proximity set init\n",
			__func__);
		return retval;
	}

	return 0;
}

int synaptics_rmi4_proximity_enables(unsigned char enables)
{
	int retval;

	if (!f51)
		return -ENODEV;

	if (F51_VERSION == f51->num_of_data_sources) {
		if (enables)
			f51->proximity_enables = FINGER_HOVER_EN; //AIR_SWIPE_EN | FINGER_HOVER_EN;
		else
			f51->proximity_enables = SLEEP_PROXIMITY; //AIR_SWIPE_EN | SLEEP_PROXIMITY;
	} else {
		if (enables)
			f51->proximity_enables = PROXIMITY_ENABLE;
		else
			f51->proximity_enables = PROXIMITY_DEFAULT;
	}

	if (f51->rmi4_data->touch_stopped)
		return 0;

	retval = synaptics_rmi4_f51_set_enables(f51->rmi4_data);
	if (retval < 0)
		return retval;

#ifdef USE_CUSTOM_REZERO
	cancel_delayed_work(&f51->rmi4_data->rezero_work);
	if (f51->proximity_enables & FINGER_HOVER_EN)
		schedule_delayed_work(&f51->rmi4_data->rezero_work,
						msecs_to_jiffies(SYNAPTICS_REZERO_TIME*3)); /* 300msec*/
#endif
	return 0;
}
EXPORT_SYMBOL(synaptics_rmi4_proximity_enables);
#endif

static int synaptics_rmi4_check_status(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	int timeout = CHECK_STATUS_TIMEOUT_MS;
	unsigned char data;
	struct synaptics_rmi4_f01_device_status status;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			status.data,
			sizeof(status.data));
	if (retval < 0)
		return retval;

	dev_info(&rmi4_data->i2c_client->dev, "%s: Device status[0x%02x] status.code[%d]\n",
			__func__, status.data[0], status.status_code);

	while (status.status_code == STATUS_CRC_IN_PROGRESS) {
		if (timeout > 0)
			msleep(20);
		else
			return -1;

		retval = synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f01_data_base_addr,
				status.data,
				sizeof(status.data));
		if (retval < 0)
			return retval;

		timeout -= 20;
	}

	if (status.flash_prog == 1) {
		rmi4_data->flash_prog_mode = true;
		dev_info(&rmi4_data->i2c_client->dev, "%s: In flash prog mode, status = 0x%02x\n",
				__func__, status.status_code);
	} else {
		rmi4_data->flash_prog_mode = false;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			&data,
			sizeof(data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read interrupt status\n",
				__func__);
		return retval;
	}

	return 0;
}

static void synaptics_rmi4_set_configured(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char device_ctrl;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->i2c_client->dev),
				"%s: Failed to set configured\n",
				__func__);
		return;
	}

	device_ctrl |= CONFIGURED;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->i2c_client->dev),
				"%s: Failed to set configured\n",
				__func__);
	}

	return;
}

static int synaptics_rmi4_alloc_fh(struct synaptics_rmi4_fn **fhandler,
		struct synaptics_rmi4_fn_desc *rmi_fd, int page_number)
{
	*fhandler = kzalloc(sizeof(**fhandler), GFP_KERNEL);
	if (!(*fhandler))
		return -ENOMEM;

	(*fhandler)->full_addr.data_base =
			(rmi_fd->data_base_addr |
			(page_number << 8));
	(*fhandler)->full_addr.ctrl_base =
			(rmi_fd->ctrl_base_addr |
			(page_number << 8));
	(*fhandler)->full_addr.cmd_base =
			(rmi_fd->cmd_base_addr |
			(page_number << 8));
	(*fhandler)->full_addr.query_base =
			(rmi_fd->query_base_addr |
			(page_number << 8));

	return 0;
}

 /**
 * synaptics_rmi4_query_device()
 *
 * Called by synaptics_rmi4_probe().
 *
 * This funtion scans the page description table, records the offsets
 * to the register types of Function $01, sets up the function handlers
 * for Function $11 and Function $12, determines the number of interrupt
 * sources from the sensor, adds valid Functions with data inputs to the
 * Function linked list, parses information from the query registers of
 * Function $01, and enables the interrupt sources from the valid Functions
 * with data inputs.
 */
static int synaptics_rmi4_query_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii = 0;
	unsigned char page_number;
	unsigned char intr_count = 0;
	unsigned char f01_query[F01_STD_QUERY_LEN];
	unsigned short pdt_entry_addr;
	unsigned short intr_addr;
	struct synaptics_rmi4_fn_desc rmi_fd;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	INIT_LIST_HEAD(&rmi->support_fn_list);

	/* Scan the page description tables of the pages to service */
	for (page_number = 0; page_number < PAGES_TO_SERVICE; page_number++) {
		for (pdt_entry_addr = PDT_START; pdt_entry_addr > PDT_END;
				pdt_entry_addr -= PDT_ENTRY_SIZE) {
			pdt_entry_addr |= (page_number << 8);

			retval = synaptics_rmi4_i2c_read(rmi4_data,
					pdt_entry_addr,
					(unsigned char *)&rmi_fd,
					sizeof(rmi_fd));
			if (retval < 0)
				return retval;

			fhandler = NULL;

			if (rmi_fd.fn_number == 0) {
				dev_dbg(&rmi4_data->i2c_client->dev,
						"%s: Reached end of PDT\n",
						__func__);
				break;
			}

			/* Display function description infomation */
			dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: F%02x found (page %d): INT_SRC[%02X] BASE_ADDRS[%02X,%02X,%02X,%02x]\n",
				__func__, rmi_fd.fn_number, page_number,
				rmi_fd.intr_src_count, rmi_fd.data_base_addr,
				rmi_fd.ctrl_base_addr, rmi_fd.cmd_base_addr,
				rmi_fd.query_base_addr);

			switch (rmi_fd.fn_number) {
			case SYNAPTICS_RMI4_F01:
				rmi4_data->f01_query_base_addr =
						rmi_fd.query_base_addr;
				rmi4_data->f01_ctrl_base_addr =
						rmi_fd.ctrl_base_addr;
				rmi4_data->f01_data_base_addr =
						rmi_fd.data_base_addr;
				rmi4_data->f01_cmd_base_addr =
						rmi_fd.cmd_base_addr;

				retval = synaptics_rmi4_check_status(rmi4_data);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to check status\n",
							__func__);
					return retval;
				}

				if (rmi4_data->flash_prog_mode)
					goto flash_prog_mode;

				break;
			case SYNAPTICS_RMI4_F12:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to alloc for F%d\n",
							__func__,
							rmi_fd.fn_number);
					return retval;
				}

				retval = synaptics_rmi4_f12_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: f11_init fail[%d]\n",
							__func__, retval);
					kfree(fhandler);
					return retval;
				}
				break;
			case SYNAPTICS_RMI4_F34:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to alloc for F%d\n",
							__func__,
							rmi_fd.fn_number);
					return retval;
				}

				retval = synaptics_rmi4_f34_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: f34_init fail[%d]\n",
							__func__, retval);
					kfree(fhandler);
					return retval;
				}
				break;

#ifdef PROXIMITY
			case SYNAPTICS_RMI4_F51:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to alloc for F%d\n",
							__func__,
							rmi_fd.fn_number);
					return retval;
				}

				retval = synaptics_rmi4_f51_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: f51_init fail[%d]\n",
							__func__, retval);
					kfree(fhandler);
					return retval;
				}
				break;
#endif
			}

			/* Accumulate the interrupt count */
			intr_count += (rmi_fd.intr_src_count & MASK_3BIT);

			if (fhandler && rmi_fd.intr_src_count) {
				list_add_tail(&fhandler->link,
						&rmi->support_fn_list);
			}
		}
	}

flash_prog_mode:
	rmi4_data->num_of_intr_regs = (intr_count + 7) / 8;
	dev_dbg(&rmi4_data->i2c_client->dev,
			"%s: Number of interrupt registers = %d sum of intr_count = %d\n",
			__func__, rmi4_data->num_of_intr_regs, intr_count);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_query_base_addr,
			f01_query,
			sizeof(f01_query));
	if (retval < 0)
		return retval;

	/* RMI Version 4.0 currently supported */
	rmi->version_major = 4;
	rmi->version_minor = 0;

	rmi->manufacturer_id = f01_query[0];
	rmi->product_props = f01_query[1];
	rmi->product_info[0] = f01_query[2] & MASK_7BIT;
	rmi->product_info[1] = f01_query[3] & MASK_7BIT;
	rmi->date_code[0] = f01_query[4] & MASK_5BIT;
	rmi->date_code[1] = f01_query[5] & MASK_4BIT;
	rmi->date_code[2] = f01_query[6] & MASK_5BIT;
	rmi->tester_id = ((f01_query[7] & MASK_7BIT) << 8) |
			(f01_query[8] & MASK_7BIT);
	rmi->serial_number = ((f01_query[9] & MASK_7BIT) << 8) |
			(f01_query[10] & MASK_7BIT);
	memcpy(rmi->product_id_string, &f01_query[11], 10);

	if (rmi->manufacturer_id != 1) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Non-Synaptics device found, manufacturer ID = %d\n",
				__func__, rmi->manufacturer_id);
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_query_base_addr + F01_BUID_ID_OFFSET,
			rmi->build_id,
			sizeof(rmi->build_id));
	if (retval < 0)
		return retval;

	memset(rmi4_data->intr_mask, 0x00, sizeof(rmi4_data->intr_mask));

	/*
	 * Map out the interrupt bit masks for the interrupt sources
	 * from the registered function handlers.
	 */
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->num_of_data_sources) {
				rmi4_data->intr_mask[fhandler->intr_reg_num] |=
						fhandler->intr_mask;
			}

			/* To display each fhandler data */
			dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: F%02x : NUM_SOURCE[%02X] NUM_INT_REG[%02X] INT_MASK[%02X]\n",
				__func__, fhandler->fn_number,
				fhandler->num_of_data_sources, fhandler->intr_reg_num,
				fhandler->intr_mask);
		}
	}

	/* Enable the interrupt sources */
	for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
		if (rmi4_data->intr_mask[ii] != 0x00) {
			dev_dbg(&rmi4_data->i2c_client->dev,
					"%s: Interrupt enable mask %d = 0x%02x\n",
					__func__, ii, rmi4_data->intr_mask[ii]);
			intr_addr = rmi4_data->f01_ctrl_base_addr + 1 + ii;
			retval = synaptics_rmi4_i2c_write(rmi4_data,
					intr_addr,
					&(rmi4_data->intr_mask[ii]),
					sizeof(rmi4_data->intr_mask[ii]));
			if (retval < 0)
				return retval;
		}
	}

	synaptics_rmi4_set_configured(rmi4_data);

	return 0;
}

static void synaptics_rmi4_release_support_fn(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fn *fhandler, *n;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	if (list_empty(&rmi->support_fn_list)) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: support_fn_list is empty\n",
				__func__);
		goto out;
	}

	list_for_each_entry_safe(fhandler, n, &rmi->support_fn_list, link) {
		dev_dbg(&rmi4_data->i2c_client->dev, "%s: fn_number = %x\n",
				__func__, fhandler->fn_number);
		kfree(fhandler->data);

		kfree(fhandler);
	}

out:
#ifdef PROXIMITY
	kfree(f51);
	f51 = NULL;
#endif
}

#ifdef USE_OPEN_CLOSE
#ifdef USE_OPEN_DWORK
static void synaptics_rmi4_open_work(struct work_struct *work)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data =
			container_of(work, struct synaptics_rmi4_data,
			open_work.work);

	retval = synaptics_rmi4_start_device(rmi4_data);
	if (retval < 0)
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to start device\n", __func__);
}
#endif

static int synaptics_rmi4_input_open(struct input_dev *dev)
{
	struct synaptics_rmi4_data *rmi4_data = input_get_drvdata(dev);
	int retval;

	dev_info(&rmi4_data->i2c_client->dev, "%s\n", __func__);

#ifdef USE_OPEN_DWORK
	schedule_delayed_work(&rmi4_data->open_work,
					msecs_to_jiffies(TOUCH_OPEN_DWORK_TIME));
#else
	retval = synaptics_rmi4_start_device(rmi4_data);
	if (retval < 0)
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to start device\n", __func__);
#endif
	return 0;
}

static void synaptics_rmi4_input_close(struct input_dev *dev)
{
	struct synaptics_rmi4_data *rmi4_data = input_get_drvdata(dev);

	dev_info(&rmi4_data->i2c_client->dev, "%s\n", __func__);
#ifdef USE_OPEN_DWORK
	cancel_delayed_work(&rmi4_data->open_work);
#endif
#ifdef TSP_PATTERN_TRACKING_METHOD
	cancel_delayed_work(&rmi4_data->reboot_work);
#endif
	synaptics_rmi4_stop_device(rmi4_data);
}
#endif

static int synaptics_rmi4_set_input_device
		(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	rmi4_data->input_dev = input_allocate_device();
	if (rmi4_data->input_dev == NULL) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to allocate input device\n",
				__func__);
		retval = -ENOMEM;
		goto err_input_device;
	}
	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to query device\n",
				__func__);
		goto err_query_device;
	}

	rmi4_data->input_dev->name = "sec_touchscreen";
	rmi4_data->input_dev->id.bustype = BUS_I2C;
	rmi4_data->input_dev->dev.parent = &rmi4_data->i2c_client->dev;
#ifdef USE_OPEN_CLOSE
	rmi4_data->input_dev->open = synaptics_rmi4_input_open;
	rmi4_data->input_dev->close = synaptics_rmi4_input_close;
#endif
	input_set_drvdata(rmi4_data->input_dev, rmi4_data);

#ifdef CONFIG_GLOVE_TOUCH
	input_set_capability(rmi4_data->input_dev, EV_SW, SW_GLOVE);
#endif
	set_bit(EV_SYN, rmi4_data->input_dev->evbit);
	set_bit(EV_KEY, rmi4_data->input_dev->evbit);
	set_bit(EV_ABS, rmi4_data->input_dev->evbit);
	set_bit(BTN_TOUCH, rmi4_data->input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, rmi4_data->input_dev->keybit);
#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, rmi4_data->input_dev->propbit);
#endif

	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_POSITION_X, 0,
			rmi4_data->sensor_max_x, 0, 0);
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_POSITION_Y, 0,
			rmi4_data->sensor_max_y, 0, 0);
#ifdef REPORT_2D_W
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_TOUCH_MAJOR, 0,
			rmi4_data->max_touch_width, 0, 0);
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_TOUCH_MINOR, 0,
			rmi4_data->max_touch_width, 0, 0);
#endif
#ifdef PROXIMITY
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_DISTANCE, 0,
			HOVER_Z_MAX, 0, 0);
	setup_timer(&rmi4_data->f51_finger_timer,
			synaptics_rmi4_f51_finger_timer,
			(unsigned long)rmi4_data);
#endif

#ifdef TYPE_B_PROTOCOL
	input_mt_init_slots(rmi4_data->input_dev,
			rmi4_data->num_of_fingers);
#endif

	retval = input_register_device(rmi4_data->input_dev);
	if (retval) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to register input device\n",
				__func__);
		goto err_register_input;
	}

	return 0;

err_register_input:
	synaptics_rmi4_release_support_fn(rmi4_data);
err_query_device:
	input_free_device(rmi4_data->input_dev);

err_input_device:
	return retval;
}

static int synaptics_rmi4_reinit_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii = 0;
	unsigned short intr_addr;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_device_info *rmi;

	if (!rmi4_data->tsp_probe) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s: blocked. tsp probe is not yet done.\n", __func__);
		return -EPERM;
	}

	rmi = &(rmi4_data->rmi4_mod_info);

	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->fn_number == SYNAPTICS_RMI4_F12) {
				retval = synaptics_rmi4_f12_set_enables(rmi4_data, 0);
				if (retval < 0)
					return retval;
			}
		}
	}
#ifdef CONFIG_GLOVE_TOUCH
	synaptics_rmi4_glove_mode_enables(rmi4_data);
#endif
#ifdef PROXIMITY
	if (!f51) {
		dev_info(&rmi4_data->i2c_client->dev,
			"%s: f51 is no available.\n", __func__);
	} else {
		retval = synaptics_rmi4_f51_set_init(rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s: fail proximity set init\n",
				__func__);
			return retval;
		}
	}
#endif
	for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
		if (rmi4_data->intr_mask[ii] != 0x00) {
			dev_info(&rmi4_data->i2c_client->dev,
					"%s: Interrupt enable mask register[%d] = 0x%02x\n",
					__func__, ii, rmi4_data->intr_mask[ii]);
			intr_addr = rmi4_data->f01_ctrl_base_addr + 1 + ii;
			retval = synaptics_rmi4_i2c_write(rmi4_data,
					intr_addr,
					&(rmi4_data->intr_mask[ii]),
					sizeof(rmi4_data->intr_mask[ii]));
			if (retval < 0)
				return retval;
		}
	}

	synaptics_rmi4_set_configured(rmi4_data);

	return 0;
}

static void synaptics_rmi4_release_all_finger(
	struct synaptics_rmi4_data *rmi4_data)
{
	int ii;

#ifdef TYPE_B_PROTOCOL
	for (ii = 0; ii < rmi4_data->num_of_fingers; ii++) {
		input_mt_slot(rmi4_data->input_dev, ii);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, 0);
		rmi4_data->finger[ii].mcount = 0;
		rmi4_data->finger[ii].state = 0;
	}
#else
	input_mt_sync(rmi4_data->input_dev);
#endif
	input_report_key(rmi4_data->input_dev,
			BTN_TOUCH, 0);
	input_report_key(rmi4_data->input_dev,
			BTN_TOOL_FINGER, 0);
#ifdef CONFIG_GLOVE_TOUCH
	input_report_switch(rmi4_data->input_dev,
		SW_GLOVE, false);
	rmi4_data->touchkey_glove_mode_status = false;
#endif
	input_sync(rmi4_data->input_dev);

	rmi4_data->fingers_on_2d = false;
#ifdef PROXIMITY
	rmi4_data->f51_finger = false;
#endif

#ifdef TSP_BOOSTER
	synaptics_set_dvfs_lock(rmi4_data, -1);
#endif
#ifdef USE_CUSTOM_REZERO
	cancel_delayed_work(&rmi4_data->rezero_work);
#endif
}

void synaptics_power_ctrl(struct synaptics_rmi4_data *rmi4_data, bool enable)
{
	int i, retval;
	if (enable) {
		/* Enable regulators according to the order */
		for (i = 0; i < rmi4_data->dt_data->num_of_supply; i++) {
			if (regulator_is_enabled(rmi4_data->dt_data->supplies[i].consumer)) {
				dev_err(&rmi4_data->i2c_client->dev,
					"%s: %s is already enabled\n", __func__,
					rmi4_data->dt_data->supplies[i].supply);
			} else {
				retval = regulator_enable(rmi4_data->dt_data->supplies[i].consumer);
				if (retval) {
					dev_err(&rmi4_data->i2c_client->dev,
						"%s: Fail to enable regulator %s[%d]\n", __func__,
						rmi4_data->dt_data->supplies[i].supply, retval);
					goto err;
				}
				dev_err(&rmi4_data->i2c_client->dev,
					"%s: %s is enabled[OK]\n", __func__,
					rmi4_data->dt_data->supplies[i].supply);
				msleep(50);
			}
		}
	} else {
		/* Disable regulator */
		for (i = 0; i < rmi4_data->dt_data->num_of_supply; i++) {
			if (regulator_is_enabled(rmi4_data->dt_data->supplies[i].consumer)) {
				retval = regulator_disable(rmi4_data->dt_data->supplies[i].consumer);
				if (retval) {
					dev_err(&rmi4_data->i2c_client->dev,
						"%s: Fail to disable regulator %s[%d]\n", __func__,
						rmi4_data->dt_data->supplies[i].supply, retval);
					goto err;
				}
				dev_err(&rmi4_data->i2c_client->dev,
					"%s: %s is disabled[OK]\n", __func__,
					rmi4_data->dt_data->supplies[i].supply);
			} else {
				dev_err(&rmi4_data->i2c_client->dev,
					"%s: %s is already disabled\n", __func__,
					rmi4_data->dt_data->supplies[i].supply);
			}
		}
	}

	return;
err:
	if (enable) {
		for (i = 0; i < rmi4_data->dt_data->num_of_supply; i++) {
			if (regulator_is_enabled(rmi4_data->dt_data->supplies[i].consumer)) {
				retval = regulator_disable(rmi4_data->dt_data->supplies[i].consumer);
				dev_err(&rmi4_data->i2c_client->dev, "%s: %s is disabled[%s]\n",
						__func__, rmi4_data->dt_data->supplies[i].supply,
						(retval < 0) ? "NG" : "OK");
			}
		}
	} else {
		for (i = 0; i < rmi4_data->dt_data->num_of_supply; i++) {
			if (!regulator_is_enabled(rmi4_data->dt_data->supplies[i].consumer)) {
				retval = regulator_enable(rmi4_data->dt_data->supplies[i].consumer);
				dev_err(&rmi4_data->i2c_client->dev, "%s: %s is enabled[%s]\n",
						__func__, rmi4_data->dt_data->supplies[i].supply,
						(retval < 0) ? "NG" : "OK");
			}
		}
	}
	return;
}
static void fpga_power_ctrl(struct synaptics_rmi4_data *rmi4_data, int onoff)
{
	int retval;

	dev_info(&rmi4_data->i2c_client->dev, "%s: %s"
		, __func__, onoff ? "on" : "off");

	if (!rmi4_data->vreg_2p5){
		rmi4_data->vreg_2p5 = regulator_get(NULL, "max77826_ldo7");
		if (IS_ERR(rmi4_data->vreg_2p5)) {
			dev_err(&rmi4_data->i2c_client->dev,
				"could not get vreg_2p5, rc = %ld=n",
				PTR_ERR(rmi4_data->vreg_2p5));
		}
		retval = regulator_set_voltage(rmi4_data->vreg_2p5, 2500000, 2500000);
		if (retval) {
			dev_err(&rmi4_data->i2c_client->dev,
				"%s: unable to set vreg_2p5 voltage to 2.5V\n",
				__func__);
		}
	}
	if (!rmi4_data->vreg_2p95){
		rmi4_data->vreg_2p95 = regulator_get(NULL, "max77826_ldo4");
		if (IS_ERR(rmi4_data->vreg_2p95)) {
			dev_err(&rmi4_data->i2c_client->dev,
				"could not get vreg_2p95, rc = %ld=n",
				PTR_ERR(rmi4_data->vreg_2p95));
		}
		retval = regulator_set_voltage(rmi4_data->vreg_2p95, 2950000, 2950000);
		if (retval) {
			dev_err(&rmi4_data->i2c_client->dev,
				"%s: unable to set vreg_2p95 voltage to 1.8 ~ 2.95V\n",
				__func__);
		}
	}

	if (onoff){
		if (!regulator_is_enabled(rmi4_data->vreg_2p5)) {
			retval = regulator_enable(rmi4_data->vreg_2p5);
			if (retval)
				dev_err(&rmi4_data->i2c_client->dev,
				"enable vreg_2p5 failed, rc=%d\n", retval);
		}
		if (!regulator_is_enabled(rmi4_data->vreg_2p95)) {
			retval = regulator_enable(rmi4_data->vreg_2p95);
			if (retval)
				dev_err(&rmi4_data->i2c_client->dev,
				"enable vreg_2p95 failed, rc=%d\n", retval);
		}
	} else {
		if (regulator_is_enabled(rmi4_data->vreg_2p5)){
			retval = regulator_disable(rmi4_data->vreg_2p5);
			if (retval)
				dev_err(&rmi4_data->i2c_client->dev,
				"disable vreg_2p5 failed, rc=%d\n", retval);
		}
		if (regulator_is_enabled(rmi4_data->vreg_2p95)) {
			retval = regulator_disable(rmi4_data->vreg_2p95);
			if (retval)
				dev_err(&rmi4_data->i2c_client->dev,
				"disable vreg_2p95 failed, rc=%d\n", retval);
		}
	}
}

static int synaptics_rmi4_reset_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char command = 0x01;

	mutex_lock(&(rmi4_data->rmi4_reset_mutex));

	disable_irq(rmi4_data->i2c_client->irq);

	synaptics_rmi4_release_all_finger(rmi4_data);

	if (!rmi4_data->stay_awake) {
	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_cmd_base_addr,
			&command,
			sizeof(command));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
		goto out;
	}

		msleep(SYNAPTICS_HW_RESET_TIME);
	} else {
		synaptics_power_ctrl(rmi4_data,false);
		
		msleep(30);
		synaptics_power_ctrl(rmi4_data,true);
		rmi4_data->current_page = MASK_8BIT;

		msleep(SYNAPTICS_HW_RESET_TIME);

		retval = synaptics_rmi4_f54_set_control(rmi4_data);
		if (retval < 0)
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to set f54 control\n",
					__func__);
	}

	synaptics_rmi4_release_support_fn(rmi4_data);

	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to query device\n",
				__func__);
	}

out:
	enable_irq(rmi4_data->i2c_client->irq);
	mutex_unlock(&(rmi4_data->rmi4_reset_mutex));

	return 0;
}

#ifdef TSP_PATTERN_TRACKING_METHOD
static void synaptics_rmi4_reboot_work(struct work_struct *work)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data =
			container_of(work, struct synaptics_rmi4_data,
			reboot_work.work);

	mutex_lock(&(rmi4_data->rmi4_reset_mutex));
	disable_irq(rmi4_data->i2c_client->irq);

	synaptics_rmi4_release_all_finger(rmi4_data);

	ghosttouchcount++;
	dev_err(&rmi4_data->i2c_client->dev,
			": Tsp Reboot(%d) by pattern tracking\n",ghosttouchcount);

	synaptics_power_ctrl(rmi4_data,false);
	msleep(50);
	synaptics_power_ctrl(rmi4_data,true);
	msleep(SYNAPTICS_HW_RESET_TIME);

	retval = synaptics_rmi4_f54_set_control(rmi4_data);
	if (retval < 0)
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to set f54 control\n",
				__func__);

	synaptics_rmi4_release_support_fn(rmi4_data);

	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to query device\n",
				__func__);
	}

	enable_irq(rmi4_data->i2c_client->irq);
	mutex_unlock(&(rmi4_data->rmi4_reset_mutex));
}
#endif

#ifdef PROXIMITY
static void synaptics_rmi4_f51_finger_timer(unsigned long data)
{
	struct synaptics_rmi4_data *rmi4_data =
			(struct synaptics_rmi4_data *)data;

	if (rmi4_data->f51_finger) {
		rmi4_data->f51_finger = false;
		mod_timer(&rmi4_data->f51_finger_timer,
				jiffies + msecs_to_jiffies(F51_FINGER_TIMEOUT));
		return;
	}

	if (!rmi4_data->fingers_on_2d) {
#ifdef TYPE_B_PROTOCOL
		input_mt_slot(rmi4_data->input_dev, 0);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, 0);
#else
		input_mt_sync(rmi4_data->input_dev);
#endif
		input_sync(rmi4_data->input_dev);
		if (rmi4_data->f51_finger_is_hover) {
			dev_info(&rmi4_data->i2c_client->dev,
				"%s: Hover finger[OUT]\n", __func__);
			rmi4_data->f51_finger_is_hover = false;
		}
	}

	return;
}
#endif

static void synaptics_rmi4_remove_exp_fn(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_exp_fn *exp_fhandler, *n;

	if (list_empty(&exp_fn_list)) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: exp_fn_list empty\n",
				__func__);
		return;
	}

	list_for_each_entry_safe(exp_fhandler, n, &exp_fn_list, link) {
		if (exp_fhandler->initialized &&
				(exp_fhandler->func_remove != NULL)) {
			dev_dbg(&rmi4_data->i2c_client->dev, "%s: [%d]\n",
				__func__, exp_fhandler->fn_type);
			exp_fhandler->func_remove(rmi4_data);
		}
		list_del(&exp_fhandler->link);
		kfree(exp_fhandler);
	}
}

static int synaptics_rmi4_init_exp_fn(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_exp_fn *exp_fhandler;

	INIT_LIST_HEAD(&exp_fn_list);

	retval = rmidev_module_register();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to register rmidev module\n",
				__func__);
		goto error_exit;
	}

	retval = rmi4_f54_module_register();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to register f54 module\n",
				__func__);
		goto error_exit;
	}

	retval = rmi4_fw_update_module_register();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to register fw update module\n",
				__func__);
		goto error_exit;
	}

	if (list_empty(&exp_fn_list))
		return -ENODEV;

	list_for_each_entry(exp_fhandler, &exp_fn_list, link) {
		if (exp_fhandler->func_init != NULL) {
			dev_dbg(&rmi4_data->i2c_client->dev, "%s: run [%d]'s init function\n",
						__func__, exp_fhandler->fn_type);
			retval = exp_fhandler->func_init(rmi4_data);
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev,
						"%s: Failed to init exp fn\n",
						__func__);
				goto error_exit;
			} else {
				exp_fhandler->initialized = true;
			}
		}
	}

	return 0;

error_exit:
	synaptics_rmi4_remove_exp_fn(rmi4_data);

	return retval;
}

#ifdef CONFIG_DUAL_LCD
static void synaptics_rmi4_sleep_mode(struct synaptics_rmi4_data *rmi4_data,
						int enable)
{
	int retval;
	unsigned char device_ctrl;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read sleep mode cmd(%d)\n",
				__func__, enable);
		return;
	}

	if(enable)
		device_ctrl |= SENSOR_SLEEP;
	else
		device_ctrl &= ~(SENSOR_SLEEP);

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write sleep mode cmd(%x)\n",
				__func__, device_ctrl);
		return;
	}
	return;
}

void samsung_switching_tsp(int flip)
{
	struct synaptics_rmi4_data *rmi4_data;
	rmi4_data = synaptics_get_tsp_info();

	if (rmi4_data == NULL){
		pr_err("[TSP] %s: tsp info is null\n", __func__);
		return;
	}

	if (!rmi4_data->tsp_probe){
		dev_err(&rmi4_data->i2c_client->dev,
			"%s: tsp probe is not done yet\n",
			__func__);
		return;
	}

	if (rmi4_data->stay_awake) {
		dev_info(&rmi4_data->i2c_client->dev,
			"%s: fw update is running. switching is ignored.\n",
			__func__);
		return;
	}

	if (rmi4_data->flip_status != flip)
	{
		rmi4_data->flip_status = flip;

		dev_info(&rmi4_data->i2c_client->dev,
			"%s : flip %sed. using TSP_%s\n",
			__func__, flip ? "clos":"open", flip ? "SUB":"MAIN");

		if (rmi4_data->touch_stopped)
			gpio_set_value(rmi4_data->dt_data->tsp_sel, flip);
		else {
			synaptics_rmi4_force_calibration();
			synaptics_rmi4_release_all_finger(rmi4_data);
			synaptics_rmi4_sleep_mode(rmi4_data, 1);
			gpio_set_value(rmi4_data->dt_data->tsp_sel, flip);
			synaptics_rmi4_sleep_mode(rmi4_data, 0);
#ifdef PROXIMITY
			if (rmi4_data->hover_called && rmi4_data->hover_ic != flip) {
				synaptics_rmi4_f51_set_enables(rmi4_data);
				rmi4_data->hover_called = false;
			}
#endif
		}
	}
}
EXPORT_SYMBOL(samsung_switching_tsp);
#endif

#ifdef CHARGER_NOTIFIER
static struct synaptics_cable support_cable_list[] = {
	{ .cable_type = EXTCON_USB, },
	{ .cable_type = EXTCON_TA, },
};

static void synaptics_charger_notify_work(struct work_struct *work)
{
	struct synaptics_cable *cable =
			container_of(work, struct synaptics_cable, work);
	struct synaptics_rmi4_data *rmi4_data = synaptics_get_tsp_info();
	int rc;

	if (!rmi4_data){
		pr_err("%s tsp driver is null\n", __func__);
		return;
	}

	rmi4_data->cable_state = cable->cable_state;
	if (rmi4_data->touch_stopped){
		pr_err("%s tsp is stopped\n", __func__);
		return;
	}

	rc = gpio_direction_output(GPIO_TSP_TA, cable->cable_state);
	if (rc)
		pr_err("%s: error to set gpio %d output %ld\n",
			__func__, GPIO_TSP_TA, cable->cable_state);
	pr_info("%s %ld, tsp_ta = %d\n",
		__func__, cable->cable_state, gpio_get_value(GPIO_TSP_TA));
}

static int synaptics_charger_notify(struct notifier_block *nb,
					unsigned long stat, void *ptr)
{
	struct synaptics_cable *cable =
			container_of(nb, struct synaptics_cable, nb);

	pr_info("%s, %ld\n", __func__, stat);
	cable->cable_state = stat;

	schedule_work(&cable->work);

	return NOTIFY_DONE;

}

static int __init synaptics_init_charger_notify(void)
{
	struct synaptics_rmi4_data *rmi4_data = synaptics_get_tsp_info();
	struct synaptics_cable *cable;
	int ret;
	int i;

	if (!rmi4_data){
		pr_info("%s tsp driver is null\n", __func__);
		return 0;
	}

	pr_info("%s register extcon notifier for usb and ta\n", __func__);
	for (i = 0; i < ARRAY_SIZE(support_cable_list); i++) {
		cable = &support_cable_list[i];
		INIT_WORK(&cable->work, synaptics_charger_notify_work);
		cable->nb.notifier_call = synaptics_charger_notify;

		ret = extcon_register_interest(&cable->extcon_nb,
				EXTCON_DEV_NAME,
				extcon_cable_name[cable->cable_type],
				&cable->nb);
		if (ret)
			pr_err("%s: fail to register extcon notifier(%s, %d)\n",
				__func__, extcon_cable_name[cable->cable_type],
				ret);

		cable->edev = cable->extcon_nb.edev;
		if (!cable->edev)
			pr_err("%s: fail to get extcon device\n", __func__);
	}
	return 0;
}
device_initcall_sync(synaptics_init_charger_notify);
#endif

/**
* synaptics_rmi4_new_function()
*
* Called by other expansion Function modules in their module init and
* module exit functions.
*
* This function is used by other expansion Function modules such as
* rmi_dev to register themselves with the driver by providing their
* initialization and removal callback function pointers so that they
* can be inserted or removed dynamically at module init and exit times,
* respectively.
*/
int synaptics_rmi4_new_function(enum exp_fn fn_type,
		int (*func_init)(struct synaptics_rmi4_data *rmi4_data),
		void (*func_remove)(struct synaptics_rmi4_data *rmi4_data),
		void (*func_attn)(struct synaptics_rmi4_data *rmi4_data,
		unsigned char intr_mask))
{
	struct synaptics_rmi4_exp_fn *exp_fhandler;

	exp_fhandler = kzalloc(sizeof(*exp_fhandler), GFP_KERNEL);
	if (!exp_fhandler) {
		pr_err("%s: Failed to alloc mem for expansion function\n",
				__func__);
		return -ENOMEM;
	}
	exp_fhandler->fn_type = fn_type;
	exp_fhandler->func_init = func_init;
	exp_fhandler->func_attn = func_attn;
	exp_fhandler->func_remove = func_remove;
	list_add_tail(&exp_fhandler->link, &exp_fn_list);

	return 0;
}

 /**
 * synaptics_rmi4_probe()
 *
 * Called by the kernel when an association with an I2C device of the
 * same name is made (after doing i2c_add_driver).
 *
 * This funtion allocates and initializes the resources for the driver
 * as an input driver, turns on the power to the sensor, queries the
 * sensor for its supported Functions and characteristics, registers
 * the driver to the input subsystem, sets up the interrupt, handles
 * the registration of the early_suspend and late_resume functions,
 * and creates a work queue for detection of other expansion Function
 * modules.
 */
static int __devinit synaptics_rmi4_probe(struct i2c_client *client,
		const struct i2c_device_id *dev_id)
{
	int retval, i = 0;
	int retry = 3;

	unsigned char attr_count;
	int attr_count_num;
	struct synaptics_rmi4_data *rmi4_data;
	struct synaptics_rmi4_device_info *rmi;
	struct synaptics_rmi4_device_tree_data *dt_data;
	int error;

	printk(KERN_ERR "TSP %s",__func__);

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev,
				"%s: SMBus byte data not supported\n",
				__func__);
		return -EIO;
	}

	/* block temporory for dev
	if(!get_lcd_attached()){
		dev_err(&client->dev, "%s: lcd is not attached\n", __func__);
		return -EIO;
	}
	*/

	if (boot_mode_recovery == 1){
		dev_err(&client->dev, "%s: recovery mode\n", __func__);
		return -EIO;
	}

	if (client->dev.of_node) {
		dt_data = devm_kzalloc(&client->dev,
			sizeof(struct synaptics_rmi4_device_tree_data),
				GFP_KERNEL);
		if (!dt_data) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		error = synaptics_parse_dt(&client->dev, dt_data);
		if (error)
			return error;
	} else	{
		dt_data = client->dev.platform_data;
		printk(KERN_ERR "[synaptics]TSP failed to align dtsi %s",__func__);
	}

	rmi4_data = kzalloc(sizeof(*rmi4_data), GFP_KERNEL);
	if (!rmi4_data) {
		dev_err(&client->dev,
				"%s: Failed to alloc mem for rmi4_data\n",
				__func__);
		return -ENOMEM;
	}

	rmi = &(rmi4_data->rmi4_mod_info);

	rmi4_data->i2c_client = client;
	rmi4_data->current_page = MASK_8BIT;
	rmi4_data->dt_data = dt_data;

	rmi4_data->touch_stopped = false;
	rmi4_data->sensor_sleep = false;
	rmi4_data->irq_enabled = false;
	rmi4_data->tsp_probe = false;
	rmi4_data->rebootcount = 0;
#ifdef CONFIG_DUAL_LCD
	rmi4_data->flip_status = -1;
	synaptics_set_tsp_info(rmi4_data);
#endif
#ifdef CHARGER_NOTIFIER
	rmi4_data->cable_state = false;
#endif

	rmi4_data->i2c_read = synaptics_rmi4_i2c_read;
	rmi4_data->i2c_write = synaptics_rmi4_i2c_write;
	rmi4_data->irq_enable = synaptics_rmi4_irq_enable;
	rmi4_data->reset_device = synaptics_rmi4_reset_device;
	rmi4_data->stop_device = synaptics_rmi4_stop_device;
	rmi4_data->start_device = synaptics_rmi4_start_device;

	dev_info(&client->dev, "%s : system_rev = %d\n", __func__, system_rev);

	/*
		TSP FPCB is separated after B'd rev 06
		B'd rev 00 ~ 05 	: FPCB rev 4.0 : synaptics_patek_old.fw (no tuning)
		B'd rev 06 ~	: FPCB rev 4.1 : synaptics_patek.fw
	*/
	if (system_rev < 6) {
		rmi4_data->firmware_name = FW_IMAGE_NAME_PATEK_OLD;
		rmi4_data->fac_firmware_name = FAC_FWIMAGE_NAME_PATEK_OLD;
	}
	else {
		rmi4_data->firmware_name = FW_IMAGE_NAME_PATEK;
		rmi4_data->fac_firmware_name = FAC_FWIMAGE_NAME_PATEK;
	}

	dev_info(&client->dev, "%s : fw = %s, fac_fw = %s\n",
		__func__, rmi4_data->firmware_name, rmi4_data->fac_firmware_name);

	/* regulator set for TSP */
	rmi4_data->dt_data->supplies = kzalloc(
		sizeof(struct regulator_bulk_data) * rmi4_data->dt_data->num_of_supply, GFP_KERNEL);
	if (!rmi4_data->dt_data->supplies) {
		dev_err(&client->dev,
			"%s: Failed to alloc mem for supplies\n", __func__);
		retval = -ENOMEM;
		goto err_mem_regulator;
	}
	for (i = 0; i < rmi4_data->dt_data->num_of_supply; i++)
		rmi4_data->dt_data->supplies[i].supply = rmi4_data->dt_data->name_of_supply[i];

	retval = regulator_bulk_get(&client->dev, rmi4_data->dt_data->num_of_supply,
				 rmi4_data->dt_data->supplies);
	if (retval)
		goto err_get_regulator;

	retval = regulator_set_voltage(rmi4_data->dt_data->supplies[0].consumer, 3300000, 3300000);
	if (retval) {
		dev_err(&client->dev, "[TSP] unable to set voltage for 3.3v, %d\n",
			retval);
	}
	retval = regulator_set_voltage(rmi4_data->dt_data->supplies[1].consumer, 1800000, 1800000);
	if (retval) {
		dev_err(&client->dev, "[TSP] unable to set voltage for 1.8v, %d\n",
			retval);
	}

	/* FPGA configuration */
	rmi4_data->enable_counte = 0;
	fpga_power_ctrl(rmi4_data, 1);

	while(retry > 0){
		ice40_fpga_firmware_update(rmi4_data);
		retval = gpio_get_value(rmi4_data->dt_data->cdone);
		dev_err(&client->dev, "FPGA configuration %s. CDONE : %d\n",
					retval ? "succeeded":"failed", retval);
		if (retval)
			break;
		retry--;
	}
	if (retry == 0){
		dev_err(&client->dev, "failed to configurate FPGA\n");
		goto err_configure_fpga;
	}

	/* GPIO setting for TSP I2C communication */
	fpga_enable(rmi4_data, 1);
	gpio_tlmm_config(GPIO_CFG(rmi4_data->dt_data->tsp_sda, 3, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(rmi4_data->dt_data->tsp_scl, 3, GPIO_CFG_INPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_set_value(rmi4_data->dt_data->tsp_sel, TSP_MAIN);

	mutex_init(&(rmi4_data->rmi4_io_ctrl_mutex));
	mutex_init(&(rmi4_data->rmi4_reset_mutex));
	mutex_init(&(rmi4_data->rmi4_reflash_mutex));
	mutex_init(&(rmi4_data->rmi4_device_mutex));

	regulator_disable(rmi4_data->vreg_2p95);
	regulator_put(rmi4_data->vreg_2p95);

#ifdef USE_OPEN_DWORK
	INIT_DELAYED_WORK(&rmi4_data->open_work, synaptics_rmi4_open_work);
#endif
#ifdef USE_CUSTOM_REZERO
	INIT_DELAYED_WORK(&rmi4_data->rezero_work, synaptics_rmi4_rezero_work);
#endif
#ifdef TSP_PATTERN_TRACKING_METHOD
	INIT_DELAYED_WORK(&rmi4_data->reboot_work, synaptics_rmi4_reboot_work);
#endif
	i2c_set_clientdata(client, rmi4_data);

err_tsp_reboot:
	synaptics_power_ctrl(rmi4_data,true);
	msleep(SYNAPTICS_POWER_MARGIN_TIME);

#ifdef TSP_BOOSTER
	synaptics_init_dvfs(rmi4_data);
#endif

	retval = synaptics_rmi4_set_input_device(rmi4_data);
	if (retval < 0) {
		dev_err(&client->dev,
				"%s: Failed to set up input device\n",
				__func__);
		if((retval == TSP_NEEDTO_REBOOT)&&(rmi4_data->rebootcount< MAX_TSP_REBOOT)){
			synaptics_power_ctrl(rmi4_data,false);
			msleep(SYNAPTICS_POWER_MARGIN_TIME);
			msleep(SYNAPTICS_POWER_MARGIN_TIME);
			rmi4_data->rebootcount++;
			dev_err(&client->dev,
					"%s: reboot sequence by i2c fail\n",
					__func__);
			goto err_tsp_reboot;
		}
		else goto err_set_input_device;
	}

	retval = synaptics_rmi4_init_exp_fn(rmi4_data);
	if (retval < 0) {
		dev_err(&client->dev,
				"%s: Failed to register rmidev module\n",
				__func__);
		goto err_init_exp_fn;
	}

	client->irq = gpio_to_irq(rmi4_data->dt_data->tsp_int);
	dev_info(&client->dev, "%s: gpio_to_irq : %d\n", __func__, client->irq);

	rmi4_data->irq = client->irq;
	retval = synaptics_rmi4_irq_enable(rmi4_data, true);
	if (retval < 0) {
		dev_err(&client->dev,
				"%s: Failed to enable attention interrupt\n",
				__func__);
		goto err_enable_irq;
	}

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		retval = sysfs_create_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
		if (retval < 0) {
			dev_err(&client->dev,
					"%s: Failed to create sysfs attributes\n",
					__func__);
			goto err_sysfs;
		}
	}

	retval = synaptics_rmi4_fw_update_on_probe(rmi4_data);
	if (retval < 0) {
		dev_err(&client->dev, "%s: Failed to firmware update(TSP_MAIN)\n",
					__func__);
		goto err_mainTSP_fw_update;
	}

#ifdef CONFIG_DUAL_LCD
	/* INIT 2nd TSP */
	gpio_set_value(rmi4_data->dt_data->tsp_sel, TSP_SUB);

	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to query device(TSP_SUB)\n",
				__func__);
		goto err_subTSP_query_dev;
	}
	retval = synaptics_rmi4_fw_update_on_probe(rmi4_data);
	if (retval < 0) {
		dev_err(&client->dev, "%s: Failed to firmware update(TSP_SUB)\n",
					__func__);
		goto err_subTSP_fw_update;
	}

	/* Select IC */
	if(rmi4_data->dt_data->hall_ic < 0)
		/* default set : MAIN TSP */
		rmi4_data->flip_status = 0;
	else
		rmi4_data->flip_status = !(gpio_get_value(rmi4_data->dt_data->hall_ic));
	dev_info(&client->dev, "%s: Folder is %sed now.\n",
		__func__, rmi4_data->flip_status ? "clos":"open");

	gpio_set_value(rmi4_data->dt_data->tsp_sel, rmi4_data->flip_status);
#endif

	rmi4_data->tsp_probe = true;
	dev_info(&client->dev, "%s: done!\n",__func__);
	return retval;

#ifdef CONFIG_DUAL_LCD
err_subTSP_fw_update:
err_subTSP_query_dev:
#endif
err_mainTSP_fw_update:
err_sysfs:
	attr_count_num = (int)attr_count;
	for (attr_count_num--; attr_count_num >= 0; attr_count_num--) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
	}
	synaptics_rmi4_irq_enable(rmi4_data, false);

err_enable_irq:
	synaptics_rmi4_remove_exp_fn(rmi4_data);

err_init_exp_fn:
	synaptics_rmi4_release_support_fn(rmi4_data);

	input_unregister_device(rmi4_data->input_dev);
	rmi4_data->input_dev = NULL;

err_set_input_device:
	synaptics_power_ctrl(rmi4_data,false);
	fpga_enable(rmi4_data, 0);
err_configure_fpga:
	//fpga_power_ctrl(rmi4_data, 0);
err_get_regulator:
	kfree(rmi4_data->dt_data->supplies);
err_mem_regulator:
#ifdef CONFIG_DUAL_LCD
	tsp_driver = NULL;
#endif
	kfree(rmi4_data);

	return retval;
}

 /**
 * synaptics_rmi4_remove()
 *
 * Called by the kernel when the association with an I2C device of the
 * same name is broken (when the driver is unloaded).
 *
 * This funtion terminates the work queue, stops sensor data acquisition,
 * frees the interrupt, unregisters the driver from the input subsystem,
 * turns off the power to the sensor, and frees other allocated resources.
 */
static int __devexit synaptics_rmi4_remove(struct i2c_client *client)
{
	unsigned char attr_count;
	struct synaptics_rmi4_data *rmi4_data = i2c_get_clientdata(client);
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);
	synaptics_rmi4_irq_enable(rmi4_data, false);

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
	}

	input_unregister_device(rmi4_data->input_dev);

	synaptics_rmi4_remove_exp_fn(rmi4_data);

	synaptics_power_ctrl(rmi4_data,false);
	fpga_enable(rmi4_data, 0);
	//fpga_power_ctrl(rmi4_data, 0);

	rmi4_data->touch_stopped = true;

	synaptics_rmi4_release_support_fn(rmi4_data);

	input_free_device(rmi4_data->input_dev);

	kfree(rmi4_data);

	return 0;
}

static int synaptics_rmi4_stop_device(struct synaptics_rmi4_data *rmi4_data)
{
	mutex_lock(&rmi4_data->rmi4_device_mutex);

	if (rmi4_data->touch_stopped) {
		dev_err(&rmi4_data->i2c_client->dev, "%s already power off\n",
			__func__);
		goto out;
	}

	disable_irq(rmi4_data->i2c_client->irq);
	synaptics_rmi4_release_all_finger(rmi4_data);
	rmi4_data->touch_stopped = true;
#ifdef CHARGER_NOTIFIER
	if (gpio_get_value(GPIO_TSP_TA)){
		gpio_direction_output(GPIO_TSP_TA, 0);
		dev_info(&rmi4_data->i2c_client->dev,
			"%s tsp_ta = %d\n", __func__, gpio_get_value(GPIO_TSP_TA));
	}
#endif
	synaptics_power_ctrl(rmi4_data,false);
	fpga_enable(rmi4_data, 0);

	dev_info(&rmi4_data->i2c_client->dev, "%s\n", __func__);

out:
	mutex_unlock(&rmi4_data->rmi4_device_mutex);
	return 0;
	}

static int synaptics_rmi4_start_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = 0;
	mutex_lock(&rmi4_data->rmi4_device_mutex);

	if (!rmi4_data->touch_stopped) {
		dev_err(&rmi4_data->i2c_client->dev, "%s already power on\n",
			__func__);
		goto out;
	}

	fpga_enable(rmi4_data, 1);
	synaptics_power_ctrl(rmi4_data,true);
#ifdef CHARGER_NOTIFIER
	if (rmi4_data->cable_state){
		gpio_direction_output(GPIO_TSP_TA, 1);
		dev_info(&rmi4_data->i2c_client->dev,
			"%s tsp_ta = %d\n", __func__, gpio_get_value(GPIO_TSP_TA));
	}
#endif
	rmi4_data->current_page = MASK_8BIT;
	rmi4_data->touch_stopped = false;

	msleep(SYNAPTICS_HW_RESET_TIME);

	retval = synaptics_rmi4_reinit_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s: Failed to reinit device\n",
			__func__);
	}
	enable_irq(rmi4_data->i2c_client->irq);

	dev_info(&rmi4_data->i2c_client->dev, "%s\n", __func__);

out:
	mutex_unlock(&rmi4_data->rmi4_device_mutex);
	return retval;
}

static const struct i2c_device_id synaptics_rmi4_id_table[] = {
	{DRIVER_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, synaptics_rmi4_id_table);


#ifdef CONFIG_OF
static struct of_device_id synaptics_match_table[] = {
	{ .compatible = "synaptics,rmi4-ts",},
	{ },
};
#else
#define synaptics_match_table	NULL
#endif

static struct i2c_driver synaptics_rmi4_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = synaptics_match_table,
	},
	.probe = synaptics_rmi4_probe,
	.remove = __devexit_p(synaptics_rmi4_remove),
	.id_table = synaptics_rmi4_id_table,
};

 /**
 * synaptics_rmi4_init()
 *
 * Called by the kernel during do_initcalls (if built-in)
 * or when the driver is loaded (if a module).
 *
 * This function registers the driver to the I2C subsystem.
 *
 */
static int __init synaptics_rmi4_init(void)
{
#ifdef CONFIG_SAMSUNG_LPM_MODE
	extern int poweroff_charging;
	pr_err("%s\n", __func__);

	if (poweroff_charging) {
		 pr_err("%s : LPM Charging Mode!!\n", __func__);
		 return 0;
	}
#endif
	return i2c_add_driver(&synaptics_rmi4_driver);
}

 /**
 * synaptics_rmi4_exit()
 *
 * Called by the kernel when the driver is unloaded.
 *
 * This funtion unregisters the driver from the I2C subsystem.
 *
 */
static void __exit synaptics_rmi4_exit(void)
{
	i2c_del_driver(&synaptics_rmi4_driver);
}

module_init(synaptics_rmi4_init);
module_exit(synaptics_rmi4_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics RMI4 I2C Touch Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SYNAPTICS_RMI4_DRIVER_VERSION);
