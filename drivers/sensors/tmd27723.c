/*
 * Copyright (c) 2010 SAMSUNG
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/module.h>

#include <linux/sensors_core.h>
#include <linux/sensor/tmd27723.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>


/* Note about power vs enable/disable:
 *  The chip has two functions, proximity and ambient light sensing.
 *  There is no separate power enablement to the two functions (unlike
 *  the Capella CM3602/3623).
 *  This module implements two drivers: /dev/proximity and /dev/light.
 *  When either driver is enabled (via sysfs attributes), we give power
 *  to the chip.  When both are disabled, we remove power from the chip.
 *  In suspend, we remove power if light is disabled but not if proximity is
 *  enabled (proximity is allowed to wakeup from suspend).
 *
 *  There are no ioctls for either driver interfaces.  Output is via
 *  input device framework and control via sysfs attributes.
 */

/* taos debug */
/*#define DEBUG 1*/
/*#define TAOS_DEBUG*/

#define taos_dbgmsg(str, args...) pr_info("%s: " str, __func__, ##args)
#define TAOS_DEBUG
#ifdef TAOS_DEBUG
#define gprintk(fmt, x...) \
	printk(KERN_INFO "%s(%d):" fmt, __func__, __LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif

#define VENDOR_NAME		"TAOS"
#ifdef CONFIG_OPTICAL_TAOS_TMD2672X
#define CHIP_NAME		"TMD2672X"
#define CHIP_ID			0x3B
#else
#define CHIP_NAME		"TMD27723"
#define CHIP_ID			0x39
#endif

#define GPIO_PROX_LED_EN 8
#define OFFSET_FILE_PATH	"/efs/prox_cal"

/* Triton register offsets */
#define CNTRL				0x00
#define ALS_TIME			0X01
#define PRX_TIME			0x02
#define WAIT_TIME			0x03
#define ALS_MINTHRESHLO			0X04
#define ALS_MINTHRESHHI			0X05
#define ALS_MAXTHRESHLO			0X06
#define ALS_MAXTHRESHHI			0X07
#define PRX_MINTHRESHLO			0X08
#define PRX_MINTHRESHHI			0X09
#define PRX_MAXTHRESHLO			0X0A
#define PRX_MAXTHRESHHI			0X0B
#define INTERRUPT			0x0C
#define PRX_CFG				0x0D
#define PRX_COUNT			0x0E
#define GAIN				0x0F
#define REVID				0x11
#define CHIPID				0x12
#define STATUS				0x13
#define ALS_CHAN0LO			0x14
#define ALS_CHAN0HI			0x15
#define ALS_CHAN1LO			0x16
#define ALS_CHAN1HI			0x17
#define PRX_LO				0x18
#define PRX_HI				0x19
#define PRX_OFFSET			0x1E
#define TEST_STATUS			0x1F

/*Triton cmd reg masks*/
#define CMD_REG				0X80
#define CMD_BYTE_RW			0x00
#define CMD_WORD_BLK_RW			0x20
#define CMD_SPL_FN			0x60
#define CMD_PROX_INTCLR			0X05
#define CMD_ALS_INTCLR			0X06
#define CMD_PROXALS_INTCLR		0X07
#define CMD_TST_REG			0X08
#define CMD_USER_REG			0X09

/* Triton cntrl reg masks */
#define CNTL_REG_CLEAR			0x00
#define CNTL_PROX_INT_ENBL		0X20
#define CNTL_ALS_INT_ENBL		0X10
#define CNTL_WAIT_TMR_ENBL		0X08
#define CNTL_PROX_DET_ENBL		0X04
#define CNTL_ADC_ENBL			0x02
#define CNTL_PWRON			0x01
#define CNTL_ALSPON_ENBL		0x03
#define CNTL_INTALSPON_ENBL		0x13
#define CNTL_PROXPON_ENBL		0x0F
#define CNTL_INTPROXPON_ENBL		0x2F

/* Triton status reg masks */
#define STA_ADCVALID			0x01
#define STA_PRXVALID			0x02
#define STA_ADC_PRX_VALID		0x03
#define STA_ADCINTR			0x10
#define STA_PRXINTR			0x20

#define	MAX_LUX				40000
#define MIN 1

#define ADC_BUFFER_NUM	6
#define PROX_AVG_COUNT	40

enum {
	LIGHT_ENABLED = BIT(0),
	PROXIMITY_ENABLED = BIT(1),
};

#define TAOS_PROX_MAX			1023
#define TAOS_PROX_MIN			0
#define OFFSET_ARRAY_LENGTH		10

/* driver data */
struct taos_data {
	struct input_dev *proximity_input_dev;
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	struct input_dev *light_input_dev;
#endif
	struct taos_platform_data *pdata;
	struct i2c_client *i2c_client;
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	struct device *light_dev;
#endif
	struct device *proximity_dev;
	int irq;
	struct work_struct work_light;
	struct work_struct work_prox;
	struct work_struct work_prox_avg;
	struct mutex prox_mutex;
/*	TAOS_PRX_DISTANCE_STATUS taos_prox_dist;*/
	struct hrtimer timer;
	struct hrtimer prox_avg_timer;
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	ktime_t light_poll_delay;
#endif
	ktime_t prox_polling_time;
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	int adc_value_buf[ADC_BUFFER_NUM];
	int adc_index_count;
	bool adc_buf_initialized;
#endif
	u8 power_state;
	struct mutex power_lock;
	struct wake_lock prx_wake_lock;
	struct workqueue_struct *wq;
	struct workqueue_struct *wq_avg;
	int avg[3];
	int prox_avg_enable;
	int cleardata;
	int irdata;
/* Auto Calibration */
	u8 initial_offset;
	u8 offset_value;
	int cal_result;
	int threshold_high;
	int threshold_low;
	int proximity_value;
	bool offset_cal_high;
	bool chip_on_success;
	bool is_requested;
	int err_cnt;
};

#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
static int lightsensor_get_adcvalue(struct taos_data *taos);
#endif
#ifdef CONFIG_OPTICAL_TAOS_TMD2672X
static int proximity_get_channelvalue(struct taos_data *taos);
#endif

static void set_prox_offset(struct taos_data *taos, u8 offset);
static int proximity_open_offset(struct taos_data *data);
static int proximity_adc_read(struct taos_data *taos);

static int opt_i2c_write(struct taos_data *taos, u8 reg, u8 *val)
{
	int ret;

	ret = i2c_smbus_write_byte_data(taos->i2c_client,
		(CMD_REG | reg), *val);

	return ret;
}

static int opt_i2c_read(struct taos_data *taos, u8 reg , u8 *val)
{
	int ret;

	i2c_smbus_write_byte(taos->i2c_client, (CMD_REG | reg));
	ret = i2c_smbus_read_byte(taos->i2c_client);
	*val = ret;

	return ret;
}

static int opt_i2c_write_command(struct taos_data *taos, u8 val)
{
	int ret;

	ret = i2c_smbus_write_byte(taos->i2c_client, val);
	gprintk("[TAOS Command] val=[0x%x] - ret=[0x%x]\n", val, ret);

	return ret;
}

static void taos_thresh_set(struct taos_data *taos)
{
	int i = 0;
	int ret = 0;
	u8 prox_int_thresh[4] = {0, };

	/* Setting for proximity interrupt */
	if (taos->proximity_value == 1) {
		prox_int_thresh[0] = (taos->threshold_low) & 0xFF;
		prox_int_thresh[1] = (taos->threshold_low >> 8) & 0xFF;
		prox_int_thresh[2] = (0xFFFF) & 0xFF;
		prox_int_thresh[3] = (0xFFFF >> 8) & 0xFF;
	} else if (taos->proximity_value == 0) {
		prox_int_thresh[0] = (0x0000) & 0xFF;
		prox_int_thresh[1] = (0x0000 >> 8) & 0xFF;
		prox_int_thresh[2] = (taos->threshold_high) & 0xff;
		prox_int_thresh[3] = (taos->threshold_high >> 8) & 0xff;
	}

	for (i = 0; i < 4; i++) {
		ret = opt_i2c_write(taos,
			(CMD_REG|(PRX_MINTHRESHLO + i)),
			&prox_int_thresh[i]);
		if (ret < 0)
			gprintk("opt_i2c_write failed, err = %d\n", ret);
	}
}

static int taos_chip_on(struct taos_data *taos)
{
	int i = 0;
	int ret = 0;
	int num_try_init = 0;
	int fail_num = 0;
	u8 temp_val;
	u8 reg_cntrl;
	u8 prox_int_thresh[4];

	for (num_try_init = 0; num_try_init < 3 ; num_try_init++) {
		fail_num = 0;

		temp_val = CNTL_REG_CLEAR;
		ret = opt_i2c_write(taos, (CMD_REG|CNTRL), &temp_val);
		if (ret < 0) {
			gprintk("opt_i2c_write to clr ctrl reg failed\n");
			fail_num++;
			goto err_chipon_i2c_error;
		}

#ifdef CONFIG_OPTICAL_TAOS_TMD2672X
		temp_val = 0xF6;/* 27ms */
#else
		temp_val = taos->pdata->als_time;
#endif
		ret = opt_i2c_write(taos, (CMD_REG|ALS_TIME), &temp_val);
		if (ret < 0) {
			gprintk("opt_i2c_write to als time reg failed\n");
			fail_num++;
			goto err_chipon_i2c_error;
		}

		temp_val = 0xff;
		ret = opt_i2c_write(taos, (CMD_REG|PRX_TIME), &temp_val);
		if (ret < 0) {
			gprintk("opt_i2c_write to prx time reg failed\n");
			fail_num++;
			goto err_chipon_i2c_error;
		}

		temp_val = 0xff;
		ret = opt_i2c_write(taos, (CMD_REG|WAIT_TIME), &temp_val);
		if (ret < 0) {
			gprintk("opt_i2c_write to wait time reg failed\n");
			fail_num++;
			goto err_chipon_i2c_error;
		}

		temp_val = taos->pdata->intr_filter;
		ret = opt_i2c_write(taos, (CMD_REG|INTERRUPT), &temp_val);
		if (ret < 0) {
			gprintk("opt_i2c_write to interrupt reg failed\n");
			fail_num++;
			goto err_chipon_i2c_error;
		}

		temp_val = 0x0;
		ret = opt_i2c_write(taos, (CMD_REG|PRX_CFG), &temp_val);
		if (ret < 0) {
			gprintk("opt_i2c_write to prox cfg reg failed\n");
			fail_num++;
			goto err_chipon_i2c_error;
		}

		temp_val = taos->pdata->prox_pulsecnt;
		ret = opt_i2c_write(taos, (CMD_REG|PRX_COUNT), &temp_val);
		if (ret < 0) {
			gprintk("opt_i2c_write to prox cnt reg failed\n");
			fail_num++;
			goto err_chipon_i2c_error;
		}
		temp_val = taos->pdata->prox_gain;
		ret = opt_i2c_write(taos, (CMD_REG|GAIN), &temp_val);
		if (ret < 0) {
			gprintk("opt_i2c_write to prox gain reg failed\n");
			fail_num++;
			goto err_chipon_i2c_error;
		}

		/* Setting for proximity interrupt */
		prox_int_thresh[0] = 0;
		prox_int_thresh[1] = 0;
		prox_int_thresh[2] = (taos->threshold_high) & 0xff;
		prox_int_thresh[3] = (taos->threshold_high >> 8) & 0xff;

		for (i = 0; i < 4; i++) {
			ret = opt_i2c_write(taos,
				(CMD_REG|(PRX_MINTHRESHLO + i)),
				&prox_int_thresh[i]);
			if (ret < 0) {
				gprintk("opt_i2c_write failed, err = %d\n",
					ret);
				fail_num++;
				goto err_chipon_i2c_error;
			}
		}

		reg_cntrl = CNTL_INTPROXPON_ENBL;
		ret = opt_i2c_write(taos, (CMD_REG|CNTRL), &reg_cntrl);
		if (ret < 0) {
			gprintk("opt_i2c_write to ctrl reg failed\n");
			fail_num++;
			goto err_chipon_i2c_error;
		}
err_chipon_i2c_error:
		msleep(20);
		if (fail_num == 0)
			break;
	}
	if (ret < 0)
		pr_info("[SENSOR] %s: chip_on Failed!\n"\
		"fail_num : %d, ret : %d\n", __func__, fail_num, ret);
	return ret;
}

static int taos_chip_off(struct taos_data *taos)
{
	int ret = 0;
	u8 reg_cntrl;

	if (taos->chip_on_success) {
		reg_cntrl = CNTL_REG_CLEAR;
		taos->chip_on_success = false;
		ret = opt_i2c_write(taos, (CMD_REG | CNTRL), &reg_cntrl);
		if (ret < 0) {
			gprintk("opt_i2c_write to ctrl reg failed\n");
			return ret;
		}
	} else {
		pr_err("[SENSOR] %s: chip is already turn off!\n", __func__);
	}
	return ret;
}

#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
static int taos_get_lux(struct taos_data *taos)
{
	int als_gain = 1;
	int CPL = 0;
	int lux1 = 0, lux2 = 0;
	int irdata = 0;
	int cleardata = 0;
	int calculated_lux = 0;
	u8 prox_int_thresh[4];
	int i;
	int proximity_value = 0;
	int coef_atime = taos->pdata->coef_atime;
	int ga = taos->pdata->ga;
	int coef_a = taos->pdata->coef_a;
	int coef_b = taos->pdata->coef_b;
	int coef_c = taos->pdata->coef_c;
	int coef_d = taos->pdata->coef_d;
	bool max_data = false;

	if (taos->pdata->max_data)
		max_data = taos->pdata->max_data;

	cleardata = i2c_smbus_read_word_data(taos->i2c_client,
		(CMD_REG | ALS_CHAN0LO));
	irdata = i2c_smbus_read_word_data(taos->i2c_client,
		(CMD_REG | ALS_CHAN1LO));
	if (cleardata < 0 || irdata < 0) {
		taos->err_cnt++;
		pr_err("[SENSOR] %s: i2c err_cnt:%d. cleardata:%d, irdata:%d\n",
			__func__, taos->err_cnt, cleardata, irdata);
		return -1;
	}
	pr_debug("[SENSOR] %s, cleardata = %d, irdata = %d\n",
		__func__, cleardata, irdata);
	taos->cleardata = cleardata;
	taos->irdata = irdata;

	/* calculate lux */
	CPL = (coef_atime * als_gain * 1000) / ga;
	lux1 = (int)((coef_a * cleardata - coef_b * irdata) / CPL);
	lux2 = (int)((coef_c * cleardata - coef_d * irdata) / CPL);

	if (max_data) {	/* Maximum data */
		if (lux1 > lux2)
			calculated_lux = lux1;
		else if (lux2 >= lux1)
			calculated_lux = lux2;
	} else {	/* Minimum data */
		if (lux1 < lux2)
			calculated_lux = lux1;
		else if (lux2 <= lux1)
			calculated_lux = lux2;
	}
	if (calculated_lux < 0)
		calculated_lux = 0;

	/*
	* protection code for abnormal proximity operation
	* under the strong sunlight
	*/
	if (cleardata >= 18000 || irdata >= 18000) {
		calculated_lux = MAX_LUX;
		proximity_value = 0;
		prox_int_thresh[0] = (0x0000) & 0xFF;
		prox_int_thresh[1] = (0x0000 >> 8) & 0xFF;
		prox_int_thresh[2] = (taos->pdata->prox_thresh_hi) & 0xFF;
		prox_int_thresh[3] = (taos->pdata->prox_thresh_hi >> 8) & 0xFF;
		for (i = 0; i < 4; i++)
			opt_i2c_write(taos, (CMD_REG|(PRX_MINTHRESHLO + i)),
			&prox_int_thresh[i]);
		input_report_abs(taos->proximity_input_dev,
		ABS_DISTANCE, !proximity_value);
		input_sync(taos->proximity_input_dev);
	}

	return calculated_lux;
}

static void taos_light_enable(struct taos_data *taos)
{
	taos_dbgmsg("starting poll timer, delay %lldns\n",
	ktime_to_ns(taos->light_poll_delay));
	taos->err_cnt = 0;
	if (taos->chip_on_success)
		hrtimer_start(&taos->timer, taos->light_poll_delay,
		HRTIMER_MODE_REL);
	else
		pr_err("[SENSOR] %s : taos chip on failed!\n", __func__);
}

static void taos_light_disable(struct taos_data *taos)
{
	taos_dbgmsg("cancelling poll timer\n");
	hrtimer_cancel(&taos->timer);
	cancel_work_sync(&taos->work_light);
}

static ssize_t poll_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(taos->light_poll_delay));
}


static ssize_t poll_delay_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = kstrtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	/* new_delay *= NSEC_PER_MSEC; */

	taos_dbgmsg("new delay = %lldns, old delay = %lldns\n",
		    new_delay, ktime_to_ns(taos->light_poll_delay));
	mutex_lock(&taos->power_lock);
	if (new_delay != ktime_to_ns(taos->light_poll_delay)) {
		taos->light_poll_delay = ns_to_ktime(new_delay);
		if (taos->power_state & LIGHT_ENABLED) {
			taos_light_disable(taos);
			taos_light_enable(taos);
		}
	}
	mutex_unlock(&taos->power_lock);

	return size;
}

static ssize_t light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		(taos->power_state & LIGHT_ENABLED) ? 1 : 0);
}
#endif

static ssize_t proximity_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		(taos->power_state & PROXIMITY_ENABLED) ? 1 : 0);
}

static void taos_power_enable(int en)
{
#if defined(CONFIG_MACH_VICTORLTE_CTC)
	int rc;
	static struct regulator *ldo19;
	static struct regulator *ldo6;
	static struct regulator *lvs1;

	printk(KERN_ERR "%s %s\n", __func__, (en) ? "on" : "off");
	if (!ldo19) {
		ldo19 = regulator_get(NULL, "8226_l19");
		rc = regulator_set_voltage(ldo19, 2850000, 2850000);
		pr_info("[TMP] %s, %d\n", __func__, __LINE__);
		if (rc)
			printk(KERN_ERR "%s: set_level failed ldo19 (%d)\n",
			__func__, rc);
	}
	if (!ldo6) {
		ldo6 = regulator_get(NULL, "8226_l6");
		rc = regulator_set_voltage(ldo6, 1800000, 1800000);
		pr_info("[TMP] %s, %d\n", __func__, __LINE__);
		if (rc)
			printk(KERN_ERR "%s: set_level failed ldo6 (%d)\n",
			__func__, rc);
	}
	if (!lvs1) {
		lvs1 = regulator_get(NULL, "8226_lvs1");
		rc = regulator_set_voltage(lvs1, 1800000, 1800000);
		pr_info("[TMP] %s, %d\n", __func__, __LINE__);
		if (rc)
			printk(KERN_ERR "%s: set_level failed lvs1 (%d)\n",
			__func__, rc);
	}

	if (en) {
		rc = regulator_enable(ldo19);
		if (rc)
			printk(KERN_ERR "%s: enable failed ldo19 (%d)\n",
			__func__, rc);
		rc = regulator_enable(ldo6);
		if (rc)
			printk(KERN_ERR "%s: enable failed ldo6 (%d)\n",
			__func__, rc);
		rc = regulator_enable(lvs1);
		if (rc)
			printk(KERN_ERR "%s: enable failed lvs1 (%d)\n",
			__func__, rc);
	} else {
		rc = regulator_disable(ldo19);
		if (rc)
			printk(KERN_ERR "%s: disable failed ldo19 (%d)\n",
			__func__, rc);
	}
#elif defined(CONFIG_MACH_FRESCONEOLTE_CTC) ||\
	defined(CONFIG_SEC_GNOTE_PROJECT)
	int rc;
	static struct regulator *ldo15;
	static struct regulator *ldo19;
	static struct regulator *lvs1;

	printk(KERN_ERR "%s %s\n", __func__, (en) ? "on" : "off");
	if (!ldo15) {
		ldo15 = regulator_get(NULL, "8226_l15");
		rc = regulator_set_voltage(ldo15, 2800000, 2800000);
		pr_info("[TMP] %s, %d\n", __func__, __LINE__);
		if (rc)
			printk(KERN_ERR "%s: set_level failed ldo15 (%d)\n",
			__func__, rc);
	}
	if (!ldo19) {
		ldo19 = regulator_get(NULL, "8226_l19");
		rc = regulator_set_voltage(ldo19, 2850000, 2850000);
		pr_info("[TMP] %s, %d\n", __func__, __LINE__);
		if (rc)
			printk(KERN_ERR "%s: set_level failed ldo19 (%d)\n",
			__func__, rc);
	}

	if (!lvs1) {
		lvs1 = regulator_get(NULL, "8226_lvs1");
		rc = regulator_set_voltage(lvs1, 1800000, 1800000);
		pr_info("[TMP] %s, %d\n", __func__, __LINE__);
		if (rc)
			printk(KERN_ERR "%s: set_level failed lvs1 (%d)\n",
			__func__, rc);
	}
	if (en) {
		rc = regulator_enable(ldo15);
		if (rc)
			printk(KERN_ERR "%s: enable failed ldo15 (%d)\n",
			__func__, rc);
		rc = regulator_enable(ldo19);
		if (rc)
			printk(KERN_ERR "%s: enable failed ldo19 (%d)\n",
			__func__, rc);
		rc = regulator_enable(lvs1);
		if (rc)
			printk(KERN_ERR "%s: enable failed lvs1 (%d)\n",
			__func__, rc);
	} else {
		rc = regulator_disable(ldo15);
		if (rc)
			printk(KERN_ERR "%s: disable failed ldo15 (%d)\n",
			__func__, rc);
	}
#elif defined(CONFIG_MACH_ATLANTICLTE_ATT) ||\
	defined(CONFIG_SEC_HESTIA_PROJECT) ||\
	defined(CONFIG_MACH_ATLANTIC3GEUR_OPEN)
	int rc;
	static struct regulator *ldo19;
	static struct regulator *lvs1;

	printk(KERN_ERR "%s %s\n", __func__, (en) ? "on" : "off");

	if (!ldo19) {
		ldo19 = regulator_get(NULL, "8226_l19");
		rc = regulator_set_voltage(ldo19, 2850000, 2850000);
		pr_info("[TMP] %s, %d\n", __func__, __LINE__);
		if (rc)
			printk(KERN_ERR "%s: set_level failed ldo19 (%d)\n",
			__func__, rc);
	}
	if (!lvs1) {
		lvs1 = regulator_get(NULL, "8226_lvs1");
		rc = regulator_set_voltage(lvs1, 1800000, 1800000);
		pr_info("[TMP] %s, %d\n", __func__, __LINE__);
		if (rc)
			printk(KERN_ERR "%s: set_level failed lvs1 (%d)\n",
			__func__, rc);
	}
	if (en) {
		rc = regulator_enable(ldo19);
		if (rc)
			printk(KERN_ERR "%s: enable failed ldo19 (%d)\n",
			__func__, rc);

		rc = regulator_enable(lvs1);
		if (rc)
			printk(KERN_ERR "%s: enable failed lvs1 (%d)\n",
			__func__, rc);
	} else {
		rc = regulator_disable(ldo19);
		if (rc)
			printk(KERN_ERR "%s: disable failed ldo19 (%d)\n",
			__func__, rc);
	}
#else
	int rc;
	static struct regulator *ldo18;
	static struct regulator *lvs1;

	printk(KERN_ERR "%s %s\n", __func__, (en) ? "on" : "off");

	if (!ldo18) {
		ldo18 = regulator_get(NULL, "8941_l18");
		rc = regulator_set_voltage(ldo18, 2850000, 2850000);
		pr_info("[TMP] %s, %d\n", __func__, __LINE__);
		if (rc)
			printk(KERN_ERR "%s: set_level failed ldo18 (%d)\n",
			__func__, rc);
	}

	if (!lvs1) {
		lvs1 = regulator_get(NULL, "8941_lvs1");
		rc = regulator_set_voltage(lvs1, 1800000, 1800000);
		pr_info("[TMP] %s, %d\n", __func__, __LINE__);
		if (rc)
			printk(KERN_ERR "%s: set_level failed lvs1 (%d)\n",
			__func__, rc);
	}
	if (en) {
		rc = regulator_enable(ldo18);
		if (rc)
			printk(KERN_ERR "%s: enable failed ldo18 (%d)\n",
			__func__, rc);
		rc = regulator_enable(lvs1);
		if (rc)
			printk(KERN_ERR "%s: enable failed lvs1 (%d)\n",
			__func__, rc);
	} else {
		rc = regulator_disable(ldo18);
		if (rc)
			printk(KERN_ERR "%s: disable failed ldo18 (%d)\n",
			__func__, rc);
	}
#endif
}

static void taos_request_gpio(struct taos_data *taos)
{
	int ret = 0;
	ret = gpio_request(taos->pdata->en, "prox_en");
	if (ret) {
		taos->is_requested = false;
		pr_err("[taos]%s: unable to request prox_en [%d]\n",
			__func__, taos->pdata->en);
		return;
	} else {
		taos->is_requested = true;
	}
	ret = gpio_direction_output(taos->pdata->en, 0);
	if (ret)
		pr_err("[taos]%s: unable to set_direction for prox_en [%d]\n",
		__func__, taos->pdata->en);
	pr_info("%s: en: %u\n", __func__, taos->pdata->en);
}

#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
static ssize_t light_enable_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t size)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1")) {
		new_value = true;
	} else if (sysfs_streq(buf, "0")) {
		new_value = false;
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&taos->power_lock);
	taos_dbgmsg("new_value = %d, old state = %d\n",
		    new_value, (taos->power_state & LIGHT_ENABLED) ? 1 : 0);
	if (new_value && !(taos->power_state & LIGHT_ENABLED)) {
		if (!taos->power_state) {
			taos_power_enable(1);
			msleep(20);
			taos->chip_on_success = (taos_chip_on(taos) >= 0) ?
				true : false;
		}
		if (taos->chip_on_success) {
			taos->power_state |= LIGHT_ENABLED;
			taos_light_enable(taos);
		}
	} else if (!new_value && (taos->power_state & LIGHT_ENABLED)) {
		taos_light_disable(taos);
		taos->power_state &= ~LIGHT_ENABLED;
		if (!taos->power_state) {
			taos_chip_off(taos);
			taos_power_enable(0);
		}
	}
	mutex_unlock(&taos->power_lock);
	return size;
}
#endif

static ssize_t proximity_enable_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	bool new_value;
	int temp = 0, ret = 0;

	if (sysfs_streq(buf, "1")) {
		new_value = true;
	} else if (sysfs_streq(buf, "0")) {
		new_value = false;
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&taos->power_lock);
	taos_dbgmsg("new_value = %d, old state = %d\n",
		    new_value, (taos->power_state & PROXIMITY_ENABLED) ? 1 : 0);
	if (new_value && !(taos->power_state & PROXIMITY_ENABLED)) {
		if (!taos->power_state)
			taos_power_enable(1);
		if (taos->is_requested)
			gpio_set_value(taos->pdata->en, 1);
		usleep_range(5000, 6000);
		ret = proximity_open_offset(taos);
		if (ret < 0 && ret != -ENOENT)
			pr_err("%s: proximity_open_offset() failed\n",
			__func__);
		/* set prox_threshold from board file */
		if (taos->offset_value != taos->initial_offset) {
			if (taos->pdata->prox_th_hi_cal &&
				taos->pdata->prox_th_low_cal) {
				taos->threshold_high =
					taos->pdata->prox_th_hi_cal;
				taos->threshold_low =
					taos->pdata->prox_th_low_cal;
			}
		}
		pr_err("%s: th_hi = %d, th_low = %d\n", __func__,
			taos->threshold_high, taos->threshold_low);

		taos->power_state |= PROXIMITY_ENABLED;

		/* interrupt clearing */
		temp = (CMD_REG|CMD_SPL_FN|CMD_PROXALS_INTCLR);
		ret = opt_i2c_write_command(taos, temp);
		if (ret < 0)
			gprintk("opt_i2c_write failed, err = %d\n", ret);
		taos->chip_on_success = (taos_chip_on(taos) >= 0) ?
			true : false;

		input_report_abs(taos->proximity_input_dev, ABS_DISTANCE, 1);
		input_sync(taos->proximity_input_dev);

		enable_irq(taos->irq);
		enable_irq_wake(taos->irq);

	} else if (!new_value && (taos->power_state & PROXIMITY_ENABLED)) {
		disable_irq_wake(taos->irq);
		disable_irq(taos->irq);

		taos->power_state &= ~PROXIMITY_ENABLED;
		if (!taos->power_state) {
			taos_chip_off(taos);
			taos_power_enable(0);
		}
		if (taos->is_requested)
			gpio_set_value(taos->pdata->en, 0);
	}
	mutex_unlock(&taos->power_lock);
	return size;
}

static ssize_t proximity_state_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	int adc = 0;

	adc = i2c_smbus_read_word_data(taos->i2c_client,
			CMD_REG | PRX_LO);
	if (adc > TAOS_PROX_MAX)
		adc = TAOS_PROX_MAX;

	return sprintf(buf, "%d\n", adc);
}

static void set_prox_offset(struct taos_data *taos, u8 offset)
{
	int ret = 0;

	ret = opt_i2c_write(taos, (CMD_REG|PRX_OFFSET), &offset);
	if (ret < 0)
		gprintk("opt_i2c_write to prx offset reg failed\n");
}

static int proximity_open_offset(struct taos_data *data)
{
	struct file *offset_filp = NULL;
	int err = 0;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	offset_filp = filp_open(OFFSET_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(offset_filp)) {
		pr_err("%s: no offset file\n", __func__);
		err = PTR_ERR(offset_filp);
		if (err != -ENOENT)
			pr_err("%s: Can't open cancelation file\n", __func__);
		set_fs(old_fs);
		return err;
	}

	err = offset_filp->f_op->read(offset_filp,
		(char *)&data->offset_value, sizeof(u8), &offset_filp->f_pos);
	if (err != sizeof(u8)) {
		pr_err("%s: Can't read the cancel data from file\n", __func__);
		err = -EIO;
	}

	pr_info("%s: data->offset_value = %d\n",
		__func__, data->offset_value);
	set_prox_offset(data, data->offset_value);
	filp_close(offset_filp, current->files);
	set_fs(old_fs);

	return err;
}

static int proximity_adc_read(struct taos_data *taos)
{
	int sum[OFFSET_ARRAY_LENGTH];
	int i = 0;
	int avg = 0;
	int min = 0;
	int max = 0;
	int total = 0;

	mutex_lock(&taos->prox_mutex);
	for (i = 0; i < OFFSET_ARRAY_LENGTH; i++) {
		usleep_range(11000, 11000);
		sum[i] = i2c_smbus_read_word_data(taos->i2c_client,
			CMD_REG | PRX_LO);
		if (i == 0) {
			min = sum[i];
			max = sum[i];
		} else {
			if (sum[i] < min)
				min = sum[i];
			else if (sum[i] > max)
				max = sum[i];
		}
		total += sum[i];
	}
	mutex_unlock(&taos->prox_mutex);
	total -= (min + max);
	avg = (int)(total / (OFFSET_ARRAY_LENGTH - 2));

	return avg;
}

static int proximity_store_offset(struct device *dev, bool do_calib)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	struct file *offset_filp = NULL;
	mm_segment_t old_fs;
	int err = 0;
	int adc = 0;
	u8 reg_cntrl = 0x25;
	int target_xtalk = 150;
	int offset_change = 0x20;
	bool offset_cal_baseline = true;

	if (do_calib) {
		/* tap offset button */
		pr_info("%s: offset\n", __func__);
		taos->offset_value = 0x3F;
		err = opt_i2c_write(taos, (CMD_REG|CNTRL), &reg_cntrl);
		if (err < 0)
			gprintk("opt_i2c_write to ctrl reg failed\n");

		usleep_range(12000, 15000);
		while (1) {
			adc = proximity_adc_read(taos);
			pr_info("%s: crosstalk = %d\n", __func__, adc);
			if (offset_cal_baseline)  {
				if (adc >= 250) {
					taos->offset_cal_high = true;
				} else {
					taos->offset_cal_high = false;
					taos->offset_value =
						taos->initial_offset;
					break;
				}
				offset_cal_baseline = false;
			} else	{
				if (taos->offset_cal_high) {
					if (adc > target_xtalk) {
						taos->offset_value +=
							offset_change;
					} else {
						taos->offset_value -=
							offset_change;
					}
				} else	{
					if (adc > target_xtalk) {
						taos->offset_value -=
							offset_change;
					} else {
						taos->offset_value +=
							offset_change;
					}
				}
				offset_change = (int)(offset_change / 2);
				if (offset_change == 0)
					break;
			}
			set_prox_offset(taos, taos->offset_value);
			pr_info("%s: P_OFFSET = %d, change = %d\n", __func__,
				taos->offset_value, offset_change);
		}
		adc = proximity_adc_read(taos);
		if (taos->offset_value >= 121 &&
			taos->offset_value < 128) {
			taos->cal_result = 0;
			pr_err("%s: cal fail return -1, adc = %d\n",
				__func__, adc);
		} else {
			if (taos->offset_value != taos->initial_offset &&
				taos->offset_cal_high == true) {
				if (taos->pdata->prox_th_hi_cal) {
					taos->threshold_high =
						taos->pdata->prox_th_hi_cal;
				}
				if (taos->pdata->prox_th_low_cal) {
					taos->threshold_low =
						taos->pdata->prox_th_low_cal;
				}
				taos_thresh_set(taos);
				taos->cal_result = 1;
			} else
				taos->cal_result = 2;
		}
		if (taos->offset_cal_high != false)
			set_prox_offset(taos, taos->offset_value);
	} else {
	/* tap reset button */
		pr_info("%s: reset\n", __func__);
		taos->threshold_high = taos->pdata->prox_thresh_hi;
		taos->threshold_low = taos->pdata->prox_thresh_low;
		taos_thresh_set(taos);
		taos->offset_value = taos->initial_offset;
		set_prox_offset(taos, taos->offset_value);
		taos->cal_result = 2;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	offset_filp = filp_open(OFFSET_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0666);
	if (IS_ERR(offset_filp)) {
		pr_err("%s: Can't open prox_offset file\n", __func__);
		set_fs(old_fs);
		err = PTR_ERR(offset_filp);
		return err;
	}

	err = offset_filp->f_op->write(offset_filp,
		(char *)&taos->offset_value, sizeof(u8), &offset_filp->f_pos);
	if (err != sizeof(u8)) {
		pr_err("%s: Can't write the offset data to file\n", __func__);
		err = -EIO;
	}

	filp_close(offset_filp, current->files);
	set_fs(old_fs);
	return err;
}

static ssize_t proximity_cal_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	bool do_calib;
	int err;

	if (sysfs_streq(buf, "1")) { /* calibrate cancelation value */
		do_calib = true;
	} else if (sysfs_streq(buf, "0")) { /* reset cancelation value */
		do_calib = false;
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	err = proximity_store_offset(dev, do_calib);
	if (err < 0) {
		pr_err("%s: proximity_store_offset() failed\n", __func__);
		return err;
	}

	return size;
}

static ssize_t proximity_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	u8 p_offset = 0;
	int ret = 0;

	msleep(20);
	ret = opt_i2c_read(taos, PRX_OFFSET, &p_offset);
	if (ret < 0)
		pr_err("%s: opt_i2c_read() failed\n", __func__);

	return sprintf(buf, "%d,%d,%d\n",
		p_offset, taos->threshold_high, taos->threshold_low);
}

static ssize_t prox_offset_pass_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", taos->cal_result);
}

static ssize_t proximity_avg_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{

	struct taos_data *taos = dev_get_drvdata(dev);
	return sprintf(buf, "%d,%d,%d\n",
		taos->avg[0], taos->avg[1], taos->avg[2]);
}

static ssize_t proximity_avg_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	int new_value = 0;

	if (sysfs_streq(buf, "1")) {
		new_value = true;
	} else if (sysfs_streq(buf, "0")) {
		new_value = false;
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if (taos->prox_avg_enable == new_value)
		taos_dbgmsg("same status\n");
	else if (new_value == 1) {
		taos_dbgmsg("starting poll timer, delay %lldns\n",
		ktime_to_ns(taos->prox_polling_time));
		hrtimer_start(&taos->prox_avg_timer,
			taos->prox_polling_time, HRTIMER_MODE_REL);
		taos->prox_avg_enable = 1;
	} else {
		taos_dbgmsg("cancelling prox avg poll timer\n");
		hrtimer_cancel(&taos->prox_avg_timer);
		cancel_work_sync(&taos->work_prox_avg);
		taos->prox_avg_enable = 0;
	}

	return 1;
}

static ssize_t proximity_thresh_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	int thresh_hi = 0;

	msleep(20);
	thresh_hi = i2c_smbus_read_word_data(taos->i2c_client,
		(CMD_REG | PRX_MAXTHRESHLO));

	pr_info("%s: THRESHOLD = %d\n", __func__, thresh_hi);

	return sprintf(buf, "prox_threshold = %d\n", thresh_hi);
}

static ssize_t proximity_thresh_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	int thresh_value = (u8)(taos->pdata->prox_thresh_hi);
	int err = 0;

	err = kstrtoint(buf, 10, &thresh_value);
	if (err < 0)
		pr_err("%s, kstrtoint failed.", __func__);

	taos->threshold_high = thresh_value;
	taos_thresh_set(taos);
	msleep(20);

	return size;
}

static ssize_t get_vendor_name(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", VENDOR_NAME);
}

static ssize_t get_chip_name(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CHIP_NAME);
}

static DEVICE_ATTR(vendor, S_IRUGO, get_vendor_name, NULL);
static DEVICE_ATTR(name, S_IRUGO, get_chip_name, NULL);

#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
static ssize_t lightsensor_file_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	int adc = 0;
	adc = lightsensor_get_adcvalue(taos);

	return sprintf(buf, "%d\n", adc);
}

static DEVICE_ATTR(lux, S_IRUGO, lightsensor_file_state_show, NULL);

static ssize_t lightsensor_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct taos_data *taos = dev_get_drvdata(dev);
	return sprintf(buf, "%d, %d\n", taos->cleardata, taos->irdata);
}

static struct device_attribute dev_attr_light_raw_data =
	__ATTR(raw_data, S_IRUGO, lightsensor_raw_data_show, NULL);
static struct device_attribute dev_attr_light_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	       light_enable_show, light_enable_store);
static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   poll_delay_show, poll_delay_store);

static struct attribute *light_sysfs_attrs[] = {
	&dev_attr_light_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group light_attribute_group = {
	.attrs = light_sysfs_attrs,
};

static struct device_attribute *light_sensor_attrs[] = {
	&dev_attr_lux,
	&dev_attr_vendor,
	&dev_attr_name,
	&dev_attr_light_raw_data,
	NULL
};
#endif

static struct device_attribute dev_attr_proximity_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	       proximity_enable_show, proximity_enable_store);

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_proximity_enable.attr,
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};

static struct device_attribute dev_attr_proximity_raw_data =
	__ATTR(raw_data, S_IRUGO, proximity_state_show, NULL);

static DEVICE_ATTR(state, S_IRUGO | S_IWUSR, proximity_state_show, NULL);
static DEVICE_ATTR(prox_avg, S_IRUGO | S_IWUSR, proximity_avg_show,
	proximity_avg_store);
static DEVICE_ATTR(prox_cal, S_IRUGO | S_IWUSR, proximity_cal_show,
	proximity_cal_store);
static DEVICE_ATTR(prox_offset_pass, S_IRUGO | S_IWUSR,
	prox_offset_pass_show, NULL);
static DEVICE_ATTR(prox_thresh, S_IRUGO | S_IWUSR, proximity_thresh_show,
	proximity_thresh_store);

static struct device_attribute *prox_sensor_attrs[] = {
	&dev_attr_state,
	&dev_attr_proximity_raw_data,
	&dev_attr_prox_avg,
	&dev_attr_prox_cal,
	&dev_attr_prox_offset_pass,
	&dev_attr_prox_thresh,
	&dev_attr_vendor,
	&dev_attr_name,
	NULL
};

#ifdef CONFIG_OPTICAL_TAOS_TMD2672X
static int proximity_get_channelvalue(struct taos_data *taos)
{
	int status = 0, psat = 0;
	int irdata = 0;
	int cleardata = 0;
	int ret = 0;

	cleardata = i2c_smbus_read_word_data(taos->i2c_client,
		(CMD_REG | ALS_CHAN0LO));
	irdata = i2c_smbus_read_word_data(taos->i2c_client,
		(CMD_REG | ALS_CHAN1LO));
	taos->cleardata = cleardata;
	taos->irdata = irdata;

	ret = opt_i2c_read(taos, STATUS, &status);
	if (ret < 0) {
		pr_err("%s: opt_i2c_read() to STATUS regs failed\n", __func__);
		return ret;
	}

	psat = (status & 0x40) >> 6;

	return psat;
}
#endif
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
static int lightsensor_get_adcvalue(struct taos_data *taos)
{
	int i = 0;
	int j = 0;
	unsigned int adc_total = 0;
	int adc_avr_value;
	unsigned int adc_index = 0;
	unsigned int adc_max = 0;
	unsigned int adc_min = 0;
	int value = 0;

	/* get ADC */
	value = taos_get_lux(taos);
	adc_index = (taos->adc_index_count++) % ADC_BUFFER_NUM;

	/*ADC buffer initialize (light sensor off ---> light sensor on) */
	if (!taos->adc_buf_initialized) {
		taos->adc_buf_initialized = true;
		for (j = 0; j < ADC_BUFFER_NUM; j++)
			taos->adc_value_buf[j] = value;
	} else
		taos->adc_value_buf[adc_index] = value;

	adc_max = taos->adc_value_buf[0];
	adc_min = taos->adc_value_buf[0];

	for (i = 0; i < ADC_BUFFER_NUM; i++) {
		adc_total += taos->adc_value_buf[i];

		if (adc_max < taos->adc_value_buf[i])
			adc_max = taos->adc_value_buf[i];

		if (adc_min > taos->adc_value_buf[i])
			adc_min = taos->adc_value_buf[i];
	}
	adc_avr_value = (adc_total-(adc_max+adc_min))/(ADC_BUFFER_NUM - 2);

	if (taos->adc_index_count == ADC_BUFFER_NUM-1)
		taos->adc_index_count = 0;

	return adc_avr_value;
}


static void taos_work_func_light(struct work_struct *work)
{
	struct taos_data *taos = container_of(work, struct taos_data,
					      work_light);
	int adc = taos_get_lux(taos);
	if (adc < 0 && taos->err_cnt >= 2) {
		taos->power_state &= ~LIGHT_ENABLED;
		if (!taos->power_state) {
			taos_chip_off(taos);
			taos->pdata->power(false);
		}
		hrtimer_cancel(&taos->timer);
		return;
	}
	input_report_rel(taos->light_input_dev, REL_MISC, adc+1);
	input_sync(taos->light_input_dev);
}
#endif

static void taos_work_func_prox(struct work_struct *work)
{
	struct taos_data *taos =
		container_of(work, struct taos_data, work_prox);
	u16 adc_data = 0;
	u16 threshold_high;
	u16 threshold_low;
	u8 prox_int_thresh[4];
	int i;
	int proximity_value = 0;
#ifdef CONFIG_OPTICAL_TAOS_TMD2672X
	int psat = 0;
#endif
	if (!taos->chip_on_success) {
		pr_err("[SENSOR] %s : chip_on failed!\n", __func__);
		return ;
	}

	/* change Threshold */
#if defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
	for (i = 0; i < 3; i++) {
		mutex_lock(&taos->prox_mutex);
		adc_data = i2c_smbus_read_word_data(taos->i2c_client,
			CMD_REG | PRX_LO);
		mutex_unlock(&taos->prox_mutex);
		if (adc_data < TAOS_PROX_MAX+1)
			break;
		pr_err("%s : adc value read fail %d times!(%d)\n",
			__func__, i+1, adc_data);
		msleep(20);
	}
	threshold_high = i2c_smbus_read_word_data(taos->i2c_client,
		(CMD_REG | PRX_MAXTHRESHLO));
	threshold_low = i2c_smbus_read_word_data(taos->i2c_client,
		(CMD_REG | PRX_MINTHRESHLO));

	pr_info("%s: adc = %d, hi = %d, %d, low = %d, %d\n", __func__, adc_data,
		threshold_high, taos->threshold_high,
		threshold_low, taos->threshold_low);

#else
	mutex_lock(&taos->prox_mutex);
	adc_data = i2c_smbus_read_word_data(taos->i2c_client, CMD_REG | PRX_LO);
	mutex_unlock(&taos->prox_mutex);
	threshold_high = i2c_smbus_read_word_data(taos->i2c_client,
		(CMD_REG | PRX_MAXTHRESHLO));
	threshold_low = i2c_smbus_read_word_data(taos->i2c_client,
		(CMD_REG | PRX_MINTHRESHLO));

	pr_info("%s: hi = %d, low = %d\n", __func__,
		taos->threshold_high, taos->threshold_low);
#endif

	/*
	* protection code for abnormal proximity operation
	* under the saturation condition
	*/
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	if (taos_get_lux(taos) >= 1500) {
		proximity_value = 0;
		input_report_abs(taos->proximity_input_dev,
			ABS_DISTANCE, !proximity_value);
		input_sync(taos->proximity_input_dev);

		pr_info("%s: prox value = %d\n", __func__, !proximity_value);

		prox_int_thresh[0] = (0x0000) & 0xFF;
		prox_int_thresh[1] = (0x0000 >> 8) & 0xFF;
		prox_int_thresh[2] = (taos->threshold_high) & 0xFF;
		prox_int_thresh[3] = (taos->threshold_high >> 8) & 0xFF;

		for (i = 0; i < 4; i++)
			opt_i2c_write(taos, (CMD_REG|(PRX_MINTHRESHLO + i)),
				&prox_int_thresh[i]);
	} else if ((threshold_high ==  (taos->threshold_high)) &&
			(adc_data >=  (taos->threshold_high))) {
		proximity_value = 1;
		input_report_abs(taos->proximity_input_dev,
			ABS_DISTANCE, !proximity_value);
		input_sync(taos->proximity_input_dev);

		pr_info("%s: prox value = %d\n", __func__, !proximity_value);

		prox_int_thresh[0] = (taos->threshold_low) & 0xFF;
		prox_int_thresh[1] = (taos->threshold_low >> 8) & 0xFF;
		prox_int_thresh[2] = (0xFFFF) & 0xFF;
		prox_int_thresh[3] = (0xFFFF >> 8) & 0xFF;
		for (i = 0; i < 4; i++)
			opt_i2c_write(taos, (CMD_REG|(PRX_MINTHRESHLO + i)),
				&prox_int_thresh[i]);
	} else if ((threshold_high == (0xFFFF)) &&
			(adc_data <= (taos->threshold_low))) {
		proximity_value = 0;
		input_report_abs(taos->proximity_input_dev,
			ABS_DISTANCE, !proximity_value);
		input_sync(taos->proximity_input_dev);

		pr_info("%s: prox value = %d\n", __func__, !proximity_value);

		prox_int_thresh[0] = (0x0000) & 0xFF;
		prox_int_thresh[1] = (0x0000 >> 8) & 0xFF;
		prox_int_thresh[2] = (taos->threshold_high) & 0xFF;
		prox_int_thresh[3] = (taos->threshold_high >> 8) & 0xFF;
		for (i = 0; i < 4; i++)
			opt_i2c_write(taos, (CMD_REG|(PRX_MINTHRESHLO + i)),
				&prox_int_thresh[i]);

	} else {
		pr_err("%s: Error Case!adc=[%X], th_high=[%d], th_min=[%d]\n",
			__func__, adc_data, threshold_high, threshold_low);
	}
#else
	psat = proximity_get_channelvalue(taos);
	if (taos->cleardata >= 9000 || taos->irdata >= 9000 || psat) {
		proximity_value = 0;
		input_report_abs(taos->proximity_input_dev,
			ABS_DISTANCE, !proximity_value);
		input_sync(taos->proximity_input_dev);

		pr_info("%s: prox value = %d\n", __func__, !proximity_value);

		prox_int_thresh[0] = (0x0000) & 0xFF;
		prox_int_thresh[1] = (0x0000 >> 8) & 0xFF;
		prox_int_thresh[2] = (taos->threshold_high) & 0xFF;
		prox_int_thresh[3] = (taos->threshold_high >> 8) & 0xFF;
		for (i = 0; i < 4; i++)
			opt_i2c_write(taos, (CMD_REG|(PRX_MINTHRESHLO + i)),
				&prox_int_thresh[i]);
	} else if ((threshold_high ==  (taos->threshold_high)) &&
			(adc_data >=  (taos->threshold_high))) {
		proximity_value = 1;
		input_report_abs(taos->proximity_input_dev,
			ABS_DISTANCE, !proximity_value);
		input_sync(taos->proximity_input_dev);

		pr_info("%s: prox value = %d\n", __func__, !proximity_value);

		prox_int_thresh[0] = (taos->threshold_low) & 0xFF;
		prox_int_thresh[1] = (taos->threshold_low >> 8) & 0xFF;
		prox_int_thresh[2] = (0xFFFF) & 0xFF;
		prox_int_thresh[3] = (0xFFFF >> 8) & 0xFF;
		for (i = 0; i < 4; i++)
			opt_i2c_write(taos, (CMD_REG|(PRX_MINTHRESHLO + i)),
				&prox_int_thresh[i]);
	} else if ((threshold_high == (0xFFFF)) &&
			(adc_data <= (taos->threshold_low))) {
		proximity_value = 0;
		input_report_abs(taos->proximity_input_dev,
			ABS_DISTANCE, !proximity_value);
		input_sync(taos->proximity_input_dev);

		pr_info("%s: prox value = %d\n", __func__, !proximity_value);

		prox_int_thresh[0] = (0x0000) & 0xFF;
		prox_int_thresh[1] = (0x0000 >> 8) & 0xFF;
		prox_int_thresh[2] = (taos->threshold_high) & 0xFF;
		prox_int_thresh[3] = (taos->threshold_high >> 8) & 0xFF;
		for (i = 0; i < 4; i++)
			opt_i2c_write(taos, (CMD_REG|(PRX_MINTHRESHLO + i)),
				&prox_int_thresh[i]);
	} else {
		pr_err("%s: Error Case!adc=[%X], th_high=[%X],  th_min=[%X]\n",
			__func__, adc_data, threshold_high, threshold_low);
	}
#endif

	taos->proximity_value = proximity_value;
	/* reset Interrupt pin */
	/* to active Interrupt, TMD2771x Interuupt pin shoud be reset. */
	i2c_smbus_write_byte(taos->i2c_client,
	(CMD_REG|CMD_SPL_FN|CMD_PROXALS_INTCLR));

	/* enable INT */
	enable_irq(taos->irq);
}

static void taos_work_func_prox_avg(struct work_struct *work)
{
	struct taos_data *taos = container_of(work, struct taos_data,
		work_prox_avg);
	u16 proximity_value = 0;
	int min = 0, max = 0, avg = 0;
	int i = 0;

	for (i = 0; i < PROX_AVG_COUNT; i++) {
		mutex_lock(&taos->prox_mutex);
		proximity_value = i2c_smbus_read_word_data(taos->i2c_client,
			CMD_REG | PRX_LO);
		mutex_unlock(&taos->prox_mutex);
		if (proximity_value > TAOS_PROX_MIN) {
			if (proximity_value > TAOS_PROX_MAX)
				proximity_value = TAOS_PROX_MAX;
			avg += proximity_value;
			if (!i)
				min = proximity_value;
			if (proximity_value < min)
				min = proximity_value;
			if (proximity_value > max)
				max = proximity_value;
		} else {
			proximity_value = TAOS_PROX_MIN;
		}
		msleep(40);
	}
	avg /= i;
	taos->avg[0] = min;
	taos->avg[1] = avg;
	taos->avg[2] = max;
}


/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
static enum hrtimer_restart taos_timer_func(struct hrtimer *timer)
{
	struct taos_data *taos = container_of(timer, struct taos_data, timer);
	queue_work(taos->wq, &taos->work_light);
	hrtimer_forward_now(&taos->timer, taos->light_poll_delay);
	return HRTIMER_RESTART;
}
#endif

static enum hrtimer_restart taos_prox_timer_func(struct hrtimer *timer)
{
	struct taos_data *taos = container_of(timer, struct taos_data,
		prox_avg_timer);
	queue_work(taos->wq_avg, &taos->work_prox_avg);
	hrtimer_forward_now(&taos->prox_avg_timer, taos->prox_polling_time);

	return HRTIMER_RESTART;
}


/* interrupt happened due to transition/change of near/far proximity state */
irqreturn_t taos_irq_handler(int irq, void *data)
{
	struct taos_data *ip = data;

	pr_info("[SENSOR] taos interrupt handler is called\n");
	wake_lock_timeout(&ip->prx_wake_lock, 3*HZ);
	disable_irq_nosync(ip->irq);
	queue_work(ip->wq, &ip->work_prox);

	return IRQ_HANDLED;
}

static int taos_setup_irq(struct taos_data *taos)
{
	int rc = -EIO;
	struct taos_platform_data *pdata = taos->pdata;
	int irq;

	taos_dbgmsg("start\n");

	rc = gpio_request(pdata->als_int, "gpio_proximity_int");
	if (rc < 0) {
		pr_err("%s: gpio %d request failed (%d)\n",
			__func__, pdata->als_int, rc);
		return rc;
	}

	rc = gpio_direction_input(pdata->als_int);
	if (rc < 0) {
		pr_err("%s: failed to set gpio %d as input (%d)\n",
			__func__, pdata->als_int, rc);
		goto err_gpio_direction_input;
	}

	irq = gpio_to_irq(pdata->als_int);
	rc = request_threaded_irq(irq, NULL,
			 taos_irq_handler,
			 IRQF_TRIGGER_FALLING,
			 "proximity_int",
			 taos);
	if (rc < 0) {
		pr_err("%s: request_irq(%d) failed for gpio %d (%d)\n",
			__func__, irq,
			pdata->als_int, rc);
		goto err_request_irq;
	}

	/* start with interrupts disabled */
	disable_irq(irq);
	taos->irq = irq;

	taos_dbgmsg("success\n");

	goto done;

err_gpio_direction_input:
err_request_irq:
	gpio_free(pdata->als_int);
done:
	return rc;
}

static int taos_get_initial_offset(struct taos_data *taos)
{
	int ret = 0;
	u8 p_offset = 0;

	taos_power_enable(1);

	msleep(20);
	ret = opt_i2c_read(taos, PRX_OFFSET, &p_offset);
	if (ret < 0)
		pr_err("%s: opt_i2c_read() failed\n", __func__);
	else
		pr_info("%s: initial offset = %d\n", __func__, p_offset);

	taos_power_enable(0);

	return p_offset;
}


#ifdef CONFIG_OF

/* device tree parsing function */
static int taos_parse_dt(struct device *dev,
			struct  taos_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	pdata->als_int = of_get_named_gpio_flags(np, "taos,irq_gpio",
		0, &pdata->als_int_flags);

	pdata->en = of_get_named_gpio_flags(np, "taos,en",
		0, &pdata->ldo_gpio_flags);
	pr_info("%s: en: %u\n", __func__, pdata->en);

	/* Recommended */
#if defined(CONFIG_MACH_VICTORLTE_CTC)
	of_property_read_u32(np, "taos,prox_thresh_hi",
		&pdata->prox_thresh_hi);
	of_property_read_u32(np, "taos,prox_thresh_low",
		&pdata->prox_thresh_low);
	of_property_read_u32(np, "taos,prox_th_hi_cal",
		&pdata->prox_th_hi_cal);
	of_property_read_u32(np, "taos,prox_th_low_cal",
		&pdata->prox_th_low_cal);
	of_property_read_u32(np, "taos,als_time", &pdata->als_time);
	of_property_read_u32(np, "taos,intr_filter", &pdata->intr_filter);
	of_property_read_u32(np, "taos,prox_pulsecnt", &pdata->prox_pulsecnt);
	of_property_read_u32(np, "taos,prox_gain", &pdata->prox_gain);
	of_property_read_u32(np, "taos,coef_atime", &pdata->coef_atime);
	of_property_read_u32(np, "taos,ga", &pdata->ga);
	of_property_read_u32(np, "taos,coef_a", &pdata->coef_a);
	of_property_read_u32(np, "taos,coef_b", &pdata->coef_b);
	of_property_read_u32(np, "taos,coef_c", &pdata->coef_c);
	of_property_read_u32(np, "taos,coef_d", &pdata->coef_d);
	pr_info("%s: prox_thresh_hi: %u\n", __func__, pdata->prox_thresh_hi);
	pr_info("%s: prox_thresh_low: %u\n", __func__, pdata->prox_thresh_low);
	pr_info("%s: prox_th_hi_cal: %u\n", __func__, pdata->prox_th_hi_cal);
	pr_info("%s: prox_th_low_cal: %u\n", __func__, pdata->prox_th_low_cal);
	pr_info("%s: als_time: %u\n", __func__, pdata->als_time);
	pr_info("%s: intr_filter: %u\n", __func__, pdata->intr_filter);
	pr_info("%s: prox_pulsecnt: %u\n", __func__, pdata->prox_pulsecnt);
	pr_info("%s: prox_gain: %u\n", __func__, pdata->prox_gain);
	pr_info("%s: coef_atime: %u\n", __func__, pdata->coef_atime);
	pr_info("%s: ga: %u\n", __func__, pdata->ga);
	pr_info("%s: coef_a: %u\n", __func__, pdata->coef_a);
	pr_info("%s: coef_b: %u\n", __func__, pdata->coef_b);
	pr_info("%s: coef_c: %u\n", __func__, pdata->coef_c);
	pr_info("%s: coef_d: %u\n", __func__, pdata->coef_d);
	pdata->min_max = MIN;
	pdata->max_data = true;
	/* Deprecated */
#elif defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
	pdata->prox_thresh_hi = 420;
	pdata->prox_thresh_low = 245;
	pdata->prox_th_hi_cal = 470;
	pdata->prox_th_low_cal = 380;
	pdata->als_time = 0xED;
	pdata->intr_filter = 0x22;
	pdata->prox_pulsecnt = 0x10;
	pdata->prox_gain = 0x28;
	pdata->coef_atime = 50;
	pdata->ga = 97;
	pdata->coef_a = 1000;
	pdata->coef_b = 1850;
	pdata->coef_c = 510;
	pdata->coef_d = 870;
	pdata->min_max = MIN;
	pdata->max_data = true;
#else
	pdata->prox_thresh_hi = 180;
	pdata->prox_thresh_low = 105;
	pdata->prox_th_hi_cal = 470;
	pdata->prox_th_low_cal = 380;
	pdata->als_time = 0xED;
	pdata->intr_filter = 0x33;
	pdata->prox_pulsecnt = 0x08;
	pdata->prox_gain = 0x28;
	pdata->coef_atime = 50;
	pdata->ga = 112;
	pdata->coef_a = 1000;
	pdata->coef_b = 1880;
	pdata->coef_c = 460;
	pdata->coef_d = 780;
	pdata->min_max = MIN;
	pdata->max_data = true;
#endif
	return 0;
}
#else
static int taos_parse_dt(struct device *dev,
struct  taos_platform_data)
{
	return -ENODEV;
}
#endif



static int taos_i2c_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	u8 chipid = 0;
	struct input_dev *input_dev;
	struct taos_data *taos;
	struct taos_platform_data *pdata = NULL;
	int err;

	pr_info("%s, is called\n", __func__);
	taos_power_enable(1);
	msleep(100);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		goto exit;
	}

	taos = kzalloc(sizeof(struct taos_data), GFP_KERNEL);
	if (!taos) {
		pr_err("%s: failed to alloc memory for module data\n",
			__func__);
		ret = -ENOMEM;
		goto exit;
	}
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct taos_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			if (taos)
				kfree(taos);
			return -ENOMEM;
		}
		err = taos_parse_dt(&client->dev, pdata);
		if (err)
			goto err_devicetree;
	} else
		pdata = client->dev.platform_data;
	if (!pdata) {
		pr_err("%s: missing pdata!\n", __func__);
		if (taos)
			kfree(taos);
		return ret;
	}

	taos->offset_cal_high = false;
	taos->pdata = pdata;
	taos->i2c_client = client;
	i2c_set_clientdata(client, taos);

	taos_request_gpio(taos);

	msleep(100);


	chipid = i2c_smbus_read_byte_data(client, CMD_REG | CHIPID);
	if (chipid != CHIP_ID) {
		pr_err("%s: i2c read error [%x]\n", __func__, chipid);
		goto err_chip_id_or_i2c_error;
	}

	taos->threshold_high = taos->pdata->prox_thresh_hi;
	taos->threshold_low = taos->pdata->prox_thresh_low;
	taos->initial_offset = taos_get_initial_offset(taos);
	taos->offset_value = taos->initial_offset;

	mutex_init(&taos->prox_mutex);
	/* wake lock init */
	wake_lock_init(&taos->prx_wake_lock, WAKE_LOCK_SUSPEND,
		       "prx_wake_lock");
	mutex_init(&taos->power_lock);

	/* allocate proximity input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		goto err_input_device_proximity;
	}

	taos->proximity_input_dev = input_dev;
	input_set_drvdata(input_dev, taos);
	input_dev->name = "proximity_sensor";
	input_set_capability(input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	input_report_abs(taos->proximity_input_dev, ABS_DISTANCE, 1);
	input_sync(taos->proximity_input_dev);

	taos_dbgmsg("registering proximity input device\n");
	ret = input_register_device(input_dev);
	if (ret < 0) {
		pr_err("%s: could not register proximity input device\n",
			__func__);
		input_free_device(input_dev);
		goto err_input_device_proximity;
	}

	ret = sensors_create_symlink(&taos->proximity_input_dev->dev.kobj,
				input_dev->name);
	if (ret < 0) {
		pr_err("%s: could not create proximity symlink\n", __func__);
		goto err_create_symlink_proximity;
	}

	ret = sysfs_create_group(&input_dev->dev.kobj,
				 &proximity_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_proximity;
	}

	/* allocate lightsensor-level input_device */
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		ret = -ENOMEM;
		goto err_input_device_light;
	}

	input_set_drvdata(input_dev, taos);
	input_dev->name = "light_sensor";
	input_set_capability(input_dev, EV_REL, REL_MISC);

	taos_dbgmsg("registering lightsensor-level input device\n");
	ret = input_register_device(input_dev);
	if (ret < 0) {
		pr_err("%s: could not register light input device\n", __func__);
		input_free_device(input_dev);
		goto err_input_device_light;
	}

	taos->light_input_dev = input_dev;
	ret = sensors_create_symlink(&taos->light_input_dev->dev.kobj,
				taos->light_input_dev->name);
	if (ret < 0) {
		pr_err("%s: could not create light symlink\n", __func__);
		goto err_create_symlink_light;
	}

	ret = sysfs_create_group(&input_dev->dev.kobj,
				 &light_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}
#endif

#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	/* hrtimer settings.  we poll for light values using a timer. */
	hrtimer_init(&taos->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	taos->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	taos->timer.function = taos_timer_func;
#endif

	hrtimer_init(&taos->prox_avg_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	taos->prox_polling_time = ns_to_ktime(2000 * NSEC_PER_MSEC);
	taos->prox_avg_timer.function = taos_prox_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	taos->wq = create_singlethread_workqueue("taos_wq");
	if (!taos->wq) {
		ret = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}

	taos->wq_avg = create_singlethread_workqueue("taos_wq_avg");
	if (!taos->wq_avg) {
		ret = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_avg_workqueue;
	}

	/* this is the thread function we run on the work queue */
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	INIT_WORK(&taos->work_light, taos_work_func_light);
#endif
	INIT_WORK(&taos->work_prox, taos_work_func_prox);
	INIT_WORK(&taos->work_prox_avg, taos_work_func_prox_avg);
	taos->prox_avg_enable = 0;

	ret = taos_setup_irq(taos);
	if (ret) {
		pr_err("%s: could not setup irq\n", __func__);
		goto err_setup_irq;
	}

	/* set sysfs for proximity sensor and light sensor */
	ret = sensors_register(taos->proximity_dev,
				taos, prox_sensor_attrs, "proximity_sensor");
	if (ret) {
		pr_err("%s: cound not register proximity sensor device(%d).\n",
			__func__, ret);
		goto err_proximity_sensor_register_failed;
	}

#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	ret = sensors_register(taos->light_dev,
				taos, light_sensor_attrs, "light_sensor");
	if (ret) {
		pr_err("%s: cound not register light sensor device(%d).\n",
			__func__, ret);
		goto err_light_sensor_register_failed;
	}
#endif

	pr_info("%s is done.", __func__);
	return ret;

	/* error, unwind it all */
err_devicetree:
	pr_warn("\n error in device tree");

#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
err_light_sensor_register_failed:
	sensors_unregister(taos->proximity_dev, prox_sensor_attrs);
#endif
err_proximity_sensor_register_failed:
	free_irq(taos->irq, 0);
	gpio_free(taos->pdata->als_int);
err_setup_irq:
	destroy_workqueue(taos->wq_avg);
err_create_avg_workqueue:
	destroy_workqueue(taos->wq);
err_create_workqueue:
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	sysfs_remove_group(&taos->light_input_dev->dev.kobj,
			   &light_attribute_group);
err_sysfs_create_group_light:
	sensors_remove_symlink(&taos->light_input_dev->dev.kobj,
				taos->light_input_dev->name);
err_create_symlink_light:
	input_unregister_device(taos->light_input_dev);
err_input_device_light:
#endif
	sysfs_remove_group(&taos->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
err_sysfs_create_group_proximity:
	sensors_remove_symlink(&taos->proximity_input_dev->dev.kobj,
				taos->proximity_input_dev->name);
err_create_symlink_proximity:
	input_unregister_device(taos->proximity_input_dev);
err_input_device_proximity:
	mutex_destroy(&taos->power_lock);
	wake_lock_destroy(&taos->prx_wake_lock);
	mutex_destroy(&taos->prox_mutex);
err_chip_id_or_i2c_error:
	kfree(taos);
exit:
	pr_err("%s failed. ret = %d\n", __func__, ret);
	return ret;
}

static int taos_suspend(struct device *dev)
{
	/* We disable power only if proximity is disabled.  If proximity
	   is enabled, we leave power on because proximity is allowed
	   to wake up device.  We remove power without changing
	   gp2a->power_state because we use that state in resume.
	*/
	struct i2c_client *client = to_i2c_client(dev);
	struct taos_data *taos = i2c_get_clientdata(client);

#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	if (taos->power_state & LIGHT_ENABLED)
		taos_light_disable(taos);
#endif
	if (taos->power_state == LIGHT_ENABLED) {
		taos_chip_off(taos);
		taos_power_enable(0);
	}
	if (taos->power_state & PROXIMITY_ENABLED)
		disable_irq(taos->irq);
	return 0;
}

static int taos_resume(struct device *dev)
{
	/* Turn power back on if we were before suspend. */
	struct i2c_client *client = to_i2c_client(dev);
	struct taos_data *taos = i2c_get_clientdata(client);

	if (taos->power_state == LIGHT_ENABLED) {
		taos_power_enable(1);
		msleep(20);
		taos->chip_on_success = (taos_chip_on(taos) >= 0) ?
			true : false;
	}
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	if (taos->power_state & LIGHT_ENABLED)
		taos_light_enable(taos);
#endif
	if (taos->power_state & PROXIMITY_ENABLED)
		enable_irq(taos->irq);

	return 0;
}

static int taos_i2c_remove(struct i2c_client *client)
{
	struct taos_data *taos = i2c_get_clientdata(client);
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
	sysfs_remove_group(&taos->light_input_dev->dev.kobj,
			   &light_attribute_group);
	input_unregister_device(taos->light_input_dev);
#endif
	sysfs_remove_group(&taos->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
	input_unregister_device(taos->proximity_input_dev);

	free_irq(taos->irq, NULL);
	gpio_free(taos->pdata->als_int);

	if (taos->power_state) {
		taos->power_state = 0;
#ifndef CONFIG_OPTICAL_TAOS_TMD2672X
		if (taos->power_state & LIGHT_ENABLED)
			taos_light_disable(taos);
#endif
		taos_power_enable(0);
	}
	destroy_workqueue(taos->wq);
	destroy_workqueue(taos->wq_avg);

	mutex_destroy(&taos->power_lock);
	wake_lock_destroy(&taos->prx_wake_lock);
	mutex_destroy(&taos->prox_mutex);
	kfree(taos);

	return 0;
}

static const struct i2c_device_id taos_device_id[] = {
	{"tmd27723", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, taos_device_id);

static const struct dev_pm_ops taos_pm_ops = {
	.suspend = taos_suspend,
	.resume = taos_resume
};

#ifdef CONFIG_OF
static struct of_device_id tmd27723_match_table[] = {
	{ .compatible = "taos,tmd27723",},
	{},
};
#else
#define tm2672_match_table NULL
#endif

static struct i2c_driver taos_i2c_driver = {
	.driver = {
		.name = "tmd27723",
		.owner = THIS_MODULE,
		.pm = &taos_pm_ops,
		.of_match_table = tmd27723_match_table,
	},
	.probe		= taos_i2c_probe,
	.remove	= taos_i2c_remove,
	.id_table	= taos_device_id,
};


static int __init taos_init(void)
{
	return i2c_add_driver(&taos_i2c_driver);
}

static void __exit taos_exit(void)
{
	i2c_del_driver(&taos_i2c_driver);
}

module_init(taos_init);
module_exit(taos_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("Optical Sensor driver for taos");
MODULE_LICENSE("GPL");

