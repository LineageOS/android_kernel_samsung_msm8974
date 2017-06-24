/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wakelock.h>
#include <linux/regulator/consumer.h>
#include "sx9500_reg.h"
#include "sensors_core.h"

#define VENDOR_NAME              "SEMTECH"
#define MODEL_NAME               "SX9500"
#define MODULE_NAME              "grip_sensor"
#define CALIBRATION_FILE_PATH    "/efs/FactoryApp/grip_cal_data"

#define I2C_M_WR                 0 /* for i2c Write */
#define I2c_M_RD                 1 /* for i2c Read */

#define IDLE                     0
#define ACTIVE                   1

#define CAL_RET_ERROR            -1
#define CAL_RET_NONE             0
#define CAL_RET_EXIST            1
#define CAL_RET_SUCCESS          2

#define INIT_TOUCH_MODE          0
#define NORMAL_TOUCH_MODE        1

#define SX9500_MODE_SLEEP        0
#define SX9500_MODE_NORMAL       1

#ifdef CONFIG_MACH_MONDRIAN
#define MAIN_SENSOR              3
#define REF_SENSOR               2
#define CSX_STATUS_REG           SX9500_TCHCMPSTAT_TCHSTAT3_FLAG
#else
#define MAIN_SENSOR              0
#define REF_SENSOR               1
#define CSX_STATUS_REG           SX9500_TCHCMPSTAT_TCHSTAT0_FLAG
#endif

#define LIMIT_PROXOFFSET                3880 /* 45 pF */
#define LIMIT_PROXUSEFUL                10000
#define STANDARD_CAP_MAIN               450000

#define DEFAULT_INIT_TOUCH_THRESHOLD    2000
#define DEFAULT_NORMAL_TOUCH_THRESHOLD  17

#define TOUCH_CHECK_REF_AMB      0 /* 44523 */
#define TOUCH_CHECK_SLOPE        0 /* 50 */
#define TOUCH_CHECK_MAIN_AMB     0 /* 151282 */

#define DEFENCE_CODE_FOR_DEVICE_DAMAGE

/* CS0, CS1, CS2, CS3 */
#define TOTAL_BOTTON_COUNT       1
#define ENABLE_CSX               ((1 << MAIN_SENSOR) | (1 << REF_SENSOR))

#define IRQ_PROCESS_CONDITION   (SX9500_IRQSTAT_TOUCH_FLAG	\
				| SX9500_IRQSTAT_RELEASE_FLAG	\
				| SX9500_IRQSTAT_COMPDONE_FLAG)

struct sx9500_p {
	struct i2c_client *client;
	struct input_dev *input;
	struct device *factory_device;
	struct delayed_work init_work;
	struct delayed_work irq_work;
	struct wake_lock grip_wake_lock;
	struct mutex mode_mutex;

	bool calSuccessed;
	bool flagDataSkip;
	u8 touchTh;
	int initTh;
	int calData[3];
	int touchMode;

	int irq;
	int gpioNirq;
#ifdef CONFIG_SENSORS_GRIP_ADJDET
	int gpio_adjdet;
	int irq_adjdet;
	int grip_state;
	int enable_adjdet;
#endif
	int state[TOTAL_BOTTON_COUNT];
#if defined(CONFIG_SEC_MILLET_PROJECT) \
	|| defined(CONFIG_SEC_MATISSE_PROJECT) \
	|| defined(CONFIG_SEC_BERLUTI_PROJECT) \
	|| defined(CONFIG_SEC_DEGAS_PROJECT) \
	||  defined(CONFIG_SEC_T8_PROJECT) \
	||  defined(CONFIG_SEC_T10_PROJECT)
	struct regulator *L19;
	struct regulator *lvs1_1p8;
#endif

#if defined(CONFIG_SENSORS_SX9500_REGULATOR_ONOFF)
	struct regulator *vdd;
	struct regulator *ldo;
#endif
	atomic_t enable;
};

#if defined(CONFIG_MACH_CHAGALL)
#ifndef CONFIG_MACH_CHAGALL_KDI
#define SX9500_NORMAL_TOUCH_CABLE_THRESHOLD	22
int msm8974_get_cable_type(void);
#endif
#endif

static int sx9500_get_nirq_state(struct sx9500_p *data)
{
	return gpio_get_value_cansleep(data->gpioNirq);
}

static int sx9500_i2c_write(struct sx9500_p *data, u8 reg_addr, u8 buf)
{
	int ret;
	struct i2c_msg msg;
	unsigned char w_buf[2];

	w_buf[0] = reg_addr;
	w_buf[1] = buf;

	msg.addr = data->client->addr;
	msg.flags = I2C_M_WR;
	msg.len = 2;
	msg.buf = (char *)w_buf;

	ret = i2c_transfer(data->client->adapter, &msg, 1);
	if (ret < 0)
		pr_err("[SX9500]: %s - i2c write error %d\n", __func__, ret);

	return ret;
}

static int sx9500_i2c_read(struct sx9500_p *data, u8 reg_addr, u8 *buf)
{
	int ret;
	struct i2c_msg msg[2];

	msg[0].addr = data->client->addr;
	msg[0].flags = I2C_M_WR;
	msg[0].len = 1;
	msg[0].buf = &reg_addr;

	msg[1].addr = data->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(data->client->adapter, msg, 2);
	if (ret < 0)
		pr_err("[SX9500]: %s - i2c read error %d\n", __func__, ret);

	return ret;
}

static u8 sx9500_read_irqstate(struct sx9500_p *data)
{
	u8 val = 0;
	u8 ret;

	if (sx9500_i2c_read(data, SX9500_IRQSTAT_REG, &val) >= 0) {
		ret = val & 0x00FF;
		return ret;
	}

	return 0;
}

static void sx9500_initialize_register(struct sx9500_p *data)
{
	u8 val = 0;
	int idx;

	for (idx = 0; idx < (sizeof(setup_reg) >> 1); idx++) {
		sx9500_i2c_write(data, setup_reg[idx].reg, setup_reg[idx].val);
		pr_info("[SX9500]: %s - Write Reg: 0x%x Value: 0x%x\n",
			__func__, setup_reg[idx].reg, setup_reg[idx].val);

		sx9500_i2c_read(data, setup_reg[idx].reg, &val);
		pr_info("[SX9500]: %s - Read Reg: 0x%x Value: 0x%x\n\n",
			__func__, setup_reg[idx].reg, val);
	}
}

static void sx9500_initialize_chip(struct sx9500_p *data)
{
	int cnt = 0;

	while ((sx9500_get_nirq_state(data) == 0) && (cnt++ < 10)) {
		sx9500_read_irqstate(data);
		msleep(20);
	}

	if (cnt >= 10)
		pr_err("[SX9500]: %s - s/w reset fail(%d)\n", __func__, cnt);

	sx9500_initialize_register(data);
}

static int sx9500_set_offset_calibration(struct sx9500_p *data)
{
	int ret = 0;

	ret = sx9500_i2c_write(data, SX9500_IRQSTAT_REG, 0xFF);

	return ret;
}

static void send_event(struct sx9500_p *data, int cnt, u8 state)
{
	u8 buf;
#if defined(CONFIG_MACH_CHAGALL)
#if defined(CONFIG_MACH_CHAGALL_KDI)
	buf = data->touchTh;
#else
	if (msm8974_get_cable_type() > 1) {
		buf = SX9500_NORMAL_TOUCH_CABLE_THRESHOLD;
		pr_info("[SX9500]: %s - cable connected %d\n",
			__func__, msm8974_get_cable_type());
	} else {
		buf = data->touchTh;
	}
#endif
#else
	buf = data->touchTh;
#endif

	if (state == ACTIVE) {
		data->state[cnt] = ACTIVE;
		sx9500_i2c_write(data, SX9500_CPS_CTRL6_REG, buf);
		pr_info("[SX9500]: %s - %d button touched\n", __func__, cnt);
	} else {
		data->touchMode = NORMAL_TOUCH_MODE;
		data->state[cnt] = IDLE;
		sx9500_i2c_write(data, SX9500_CPS_CTRL6_REG, buf);
		pr_info("[SX9500]: %s - %d button released\n", __func__, cnt);
	}

	if (data->flagDataSkip == true)
		return;

#ifdef CONFIG_SENSORS_GRIP_ADJDET
	if (data->enable_adjdet == 1
	   &&
	   gpio_get_value_cansleep(data->gpio_adjdet) == 1) {
		pr_info("[SX9500]: %s : adj detect cable connected," \
			" skip grip sensor\n", __func__);
		return;
	}
#endif
	switch (cnt) {
	case 0:
#ifdef CONFIG_SENSORS_GRIP_ADJDET
		data->grip_state = state;
#endif
		if (state == ACTIVE)
			input_report_rel(data->input, REL_MISC, 1);
		else
			input_report_rel(data->input, REL_MISC, 2);
		break;
	case 1:
	case 2:
	case 3:
		pr_info("[SX9500]: %s - There is no defined event for" \
			" button %d.\n", __func__, cnt);
		break;
	default:
		break;
	}

	input_sync(data->input);
}

static void sx9500_display_data_reg(struct sx9500_p *data)
{
	u8 val, reg;
	int i;

	for (i = 0; i < TOTAL_BOTTON_COUNT; i++) {
		sx9500_i2c_write(data, SX9500_REGSENSORSELECT, i);
		pr_info("[SX9500]: ############# %d button #############\n", i);
		for (reg = SX9500_REGUSEMSB;
			reg <= SX9500_REGOFFSETLSB; reg++) {
			sx9500_i2c_read(data, reg, &val);
			pr_info("[SX9500]: %s - Register(0x%2x) data(0x%2x)\n",
				__func__, reg, val);
		}
	}
}

static s32 sx9500_get_init_threshold(struct sx9500_p *data)
{
	s32 threshold;

	/* Because the STANDARD_CAP_MAIN was 300,000 in the previous patch,
	 * the exception code is added. It will be removed later */
	if (data->calData[0] == 0)
		threshold = STANDARD_CAP_MAIN + data->initTh;
	else if (data->calData[0] > 100000)
		threshold = data->initTh + data->calData[0];
	else
		threshold = 300000 + data->initTh + data->calData[0];

	return threshold;
}
static s32 sx9500_get_capMain(struct sx9500_p *data)
{
	u8 msByte = 0;
	u8 lsByte = 0;
	u16 offset = 0;
	s32 capMain = 0, useful = 0;

	/* Calculate out the Main Cap information */
	sx9500_i2c_write(data, SX9500_REGSENSORSELECT, MAIN_SENSOR);
	sx9500_i2c_read(data, SX9500_REGUSEMSB, &msByte);
	sx9500_i2c_read(data, SX9500_REGUSELSB, &lsByte);

	useful = (s32)msByte;
	useful = (useful << 8) | ((s32)lsByte);
	if (useful > 32767)
		useful -= 65536;

	sx9500_i2c_read(data, SX9500_REGOFFSETMSB, &msByte);
	sx9500_i2c_read(data, SX9500_REGOFFSETLSB, &lsByte);

	offset = (u16)msByte;
	offset = (offset << 8) | ((u16)lsByte);

	msByte = (u8)(offset >> 6);
	lsByte = (u8)(offset - (((u16)msByte) << 6));

	capMain = 2 * (((s32)msByte * 3600) + ((s32)lsByte * 225)) +
		(((s32)useful * 50000) / (8 * 65536));

	/* Calculate out the Reference Cap information */
	sx9500_i2c_write(data, SX9500_REGSENSORSELECT, REF_SENSOR);
	sx9500_i2c_read(data, SX9500_REGUSEMSB, &msByte);
	sx9500_i2c_read(data, SX9500_REGUSELSB, &lsByte);

#if 0 /* Calculate out the difference between the two */
	s32 capRef = 0;

	capRef = (s32)msByte;
	capRef = (capRef << 8) | ((s32)lsByte);
	if (capRef > 32767)
		capRef -= 65536;

	sx9500_i2c_read(data, SX9500_REGOFFSETMSB, &msByte);
	sx9500_i2c_read(data, SX9500_REGOFFSETLSB, &lsByte);

	offset = (u16)msByte;
	offset = (offset << 8) | ((u16)lsByte);
	msByte = (u8)(offset >> 6);
	lsByte = (u8)(offset - (((u16)msByte) << 6));

	capRef = 2 * (((s32)msByte * 3600) + ((s32)lsByte * 225)) +
		(((s32)capRef * 50000) / (8 * 65536));

	capRef = (capRef - TOUCH_CHECK_REF_AMB) *
		TOUCH_CHECK_SLOPE + TOUCH_CHECK_MAIN_AMB;

	capMain = capMain - capRef;
#endif

	pr_info("[SX9500]: %s - CapsMain: %ld, useful: %ld, Offset: %u\n",
		__func__, (long int)capMain, (long int)useful, offset);

	return capMain;
}

static void sx9500_touchCheckWithRefSensor(struct sx9500_p *data)
{
	s32 capMain, threshold;
	int cnt = 0;
	threshold = sx9500_get_init_threshold(data);

	capMain = sx9500_get_capMain(data);

	if (data->state[cnt] == IDLE) {
		if (capMain >= threshold)
			send_event(data, cnt, ACTIVE);
		else
			send_event(data, cnt, IDLE);
	} else {
		if (capMain < threshold)
			send_event(data, cnt, IDLE);
		else
			send_event(data, cnt, ACTIVE);
	}
}

static int sx9500_save_caldata(struct sx9500_p *data)
{
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	int ret = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC,
			S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		pr_err("[SX9500]: %s - Can't open calibration file\n",
			__func__);
		set_fs(old_fs);
		ret = PTR_ERR(cal_filp);
		return ret;
	}

	ret = cal_filp->f_op->write(cal_filp, (char *)data->calData,
		sizeof(int) * 3, &cal_filp->f_pos);
	if (ret != (sizeof(int) * 3)) {
		pr_err("[SX9500]: %s - Can't write the cal data to file\n",
			__func__);
		ret = -EIO;
	}

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	return ret;
}

static void sx9500_open_caldata(struct sx9500_p *data)
{
	struct file *cal_filp = NULL;
	mm_segment_t old_fs;
	int ret;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	cal_filp = filp_open(CALIBRATION_FILE_PATH, O_RDONLY,
			S_IRUGO | S_IWUSR | S_IWGRP);
	if (IS_ERR(cal_filp)) {
		ret = PTR_ERR(cal_filp);
		if (ret != -ENOENT)
			pr_err("[SX9500]: %s - Can't open calibration file.\n",
				__func__);
		else {
			pr_info("[SX9500]: %s - There is no calibration file\n",
				__func__);
			/* calibration status init */
			memset(data->calData, 0, sizeof(int) * 3);
		}
		set_fs(old_fs);
		return;
	}

	ret = cal_filp->f_op->read(cal_filp, (char *)data->calData,
		sizeof(int) * 3, &cal_filp->f_pos);
	if (ret != (sizeof(int) * 3))
		pr_err("[SX9500]: %s - Can't read the cal data from file\n",
			__func__);

	filp_close(cal_filp, current->files);
	set_fs(old_fs);

	pr_info("[SX9500]: %s - (%d, %d, %d)\n", __func__,
		data->calData[0], data->calData[1], data->calData[2]);
}

static int sx9500_set_mode(struct sx9500_p *data, unsigned char mode)
{
	int ret = -EINVAL;

	mutex_lock(&data->mode_mutex);
	if (mode == SX9500_MODE_SLEEP) {
		ret = sx9500_i2c_write(data, SX9500_CPS_CTRL0_REG, 0x20);
		disable_irq(data->irq);
		disable_irq_wake(data->irq);
	} else if (mode == SX9500_MODE_NORMAL) {
		ret = sx9500_i2c_write(data, SX9500_CPS_CTRL0_REG,
			0x20 | ENABLE_CSX);
		msleep(20);

		sx9500_set_offset_calibration(data);
		msleep(400);

		sx9500_touchCheckWithRefSensor(data);
		enable_irq(data->irq);
		enable_irq_wake(data->irq);
	}

	/* make sure no interrupts are pending since enabling irq
	 * will only work on next falling edge */
	sx9500_read_irqstate(data);

	pr_info("[SX9500]: %s - change the mode : %u\n", __func__, mode);

	mutex_unlock(&data->mode_mutex);
	return ret;
}

static int sx9500_get_useful(struct sx9500_p *data)
{
	u8 msByte = 0;
	u8 lsByte = 0;
	s16 fullByte = 0;

	sx9500_i2c_write(data, SX9500_REGSENSORSELECT, MAIN_SENSOR);
	sx9500_i2c_read(data, SX9500_REGUSEMSB, &msByte);
	sx9500_i2c_read(data, SX9500_REGUSELSB, &lsByte);
	fullByte = (s16)((msByte << 8) | lsByte);

	pr_info("[SX9500]: %s - PROXIUSEFUL = %d\n", __func__, fullByte);

	return (int)fullByte;
}

static int sx9500_get_offset(struct sx9500_p *data)
{
	u8 msByte = 0;
	u8 lsByte = 0;
	u16 fullByte = 0;

	sx9500_i2c_write(data, SX9500_REGSENSORSELECT, MAIN_SENSOR);
	sx9500_i2c_read(data, SX9500_REGOFFSETMSB, &msByte);
	sx9500_i2c_read(data, SX9500_REGOFFSETLSB, &lsByte);
	fullByte = (u16)((msByte << 8) | lsByte);

	pr_info("[SX9500]: %s - PROXIOFFSET = %u\n", __func__, fullByte);

	return (int)fullByte;
}

static int sx9500_do_calibrate(struct sx9500_p *data, bool do_calib)
{
	int ret = 0;
	s32 capMain;

	if (do_calib == false) {
		pr_info("[SX9500]: %s - Erase!\n", __func__);
		goto cal_erase;
	}

	if (atomic_read(&data->enable) == OFF)
		sx9500_set_mode(data, SX9500_MODE_NORMAL);

	data->calData[2] = sx9500_get_offset(data);
	if ((data->calData[2] >= LIMIT_PROXOFFSET)
		|| (data->calData[2] == 0)) {
		pr_err("[SX9500]: %s - offset fail(%d)\n", __func__,
			data->calData[2]);
		goto cal_fail;
	}

	data->calData[1] = sx9500_get_useful(data);
	if (data->calData[1] >= LIMIT_PROXUSEFUL) {
		pr_err("[SX9500]: %s - useful warning(%d)\n", __func__,
			data->calData[1]);
	}

	capMain = sx9500_get_capMain(data);
	data->calData[0] = capMain;

	if (atomic_read(&data->enable) == OFF)
		sx9500_set_mode(data, SX9500_MODE_SLEEP);

	goto exit;

cal_fail:
	if (atomic_read(&data->enable) == OFF)
		sx9500_set_mode(data, SX9500_MODE_SLEEP);
	ret = -1;
cal_erase:
	memset(data->calData, 0, sizeof(int) * 3);
exit:
	pr_info("[SX9500]: %s - (%d, %d, %d)\n", __func__,
		data->calData[0], data->calData[1], data->calData[2]);
	return ret;
}

static void sx9500_set_enable(struct sx9500_p *data, int enable)
{
	int pre_enable = atomic_read(&data->enable);

	if (enable) {
		if (pre_enable == OFF) {
			data->touchMode = INIT_TOUCH_MODE;
			data->calSuccessed = false;

			sx9500_open_caldata(data);
			sx9500_set_mode(data, SX9500_MODE_NORMAL);
			atomic_set(&data->enable, ON);
		}
	} else {
		if (pre_enable == ON) {
			sx9500_set_mode(data, SX9500_MODE_SLEEP);
			atomic_set(&data->enable, OFF);
		}
	}
}

static ssize_t sx9500_get_offset_calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 val = 0;
	struct sx9500_p *data = dev_get_drvdata(dev);

	sx9500_i2c_read(data, SX9500_IRQSTAT_REG, &val);

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t sx9500_set_offset_calibration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	struct sx9500_p *data = dev_get_drvdata(dev);

	if (strict_strtoul(buf, 10, &val)) {
		pr_err("[SX9500]: %s - Invalid Argument\n", __func__);
		return -EINVAL;
	}

	if (val)
		sx9500_set_offset_calibration(data);

	return count;
}

static ssize_t sx9500_register_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int regist = 0, val = 0;
	struct sx9500_p *data = dev_get_drvdata(dev);

	if (sscanf(buf, "%d,%d", &regist, &val) != 2) {
		pr_err("[SX9500]: %s - The number of data are wrong\n",
			__func__);
		return -EINVAL;
	}

	sx9500_i2c_write(data, (unsigned char)regist, (unsigned char)val);
	pr_info("[SX9500]: %s - Register(0x%2x) data(0x%2x)\n",
		__func__, regist, val);

	return count;
}

static ssize_t sx9500_register_read_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int regist = 0;
	unsigned char val = 0;
	struct sx9500_p *data = dev_get_drvdata(dev);

	if (sscanf(buf, "%d", &regist) != 1) {
		pr_err("[SX9500]: %s - The number of data are wrong\n",
			__func__);
		return -EINVAL;
	}

	sx9500_i2c_read(data, (unsigned char)regist, &val);
	pr_info("[SX9500]: %s - Register(0x%2x) data(0x%2x)\n",
		__func__, regist, val);

	return count;
}

static ssize_t sx9500_read_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9500_p *data = dev_get_drvdata(dev);

	sx9500_display_data_reg(data);

	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t sx9500_sw_reset_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct sx9500_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == ON)
		sx9500_set_mode(data, SX9500_MODE_SLEEP);

	ret = sx9500_i2c_write(data, SX9500_SOFTRESET_REG, SX9500_SOFTRESET);
	msleep(300);

	sx9500_initialize_chip(data);

	if (atomic_read(&data->enable) == ON)
		sx9500_set_mode(data, SX9500_MODE_NORMAL);

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t sx9500_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR_NAME);
}

static ssize_t sx9500_name_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", MODEL_NAME);
}

static ssize_t sx9500_touch_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9500_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->touchMode);
}

static ssize_t sx9500_raw_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 msb, lsb;
	s16 useful;
	u16 offset;
	s32 capMain;
	struct sx9500_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == OFF)
		pr_err("[SX9500]: %s - SX9500 was not enabled\n", __func__);

	capMain = sx9500_get_capMain(data);

	sx9500_i2c_write(data, SX9500_REGSENSORSELECT, MAIN_SENSOR);
	sx9500_i2c_read(data, SX9500_REGUSEMSB, &msb);
	sx9500_i2c_read(data, SX9500_REGUSELSB, &lsb);
	useful = (s16)((msb << 8) | lsb);

	sx9500_i2c_read(data, SX9500_REGOFFSETMSB, &msb);
	sx9500_i2c_read(data, SX9500_REGOFFSETLSB, &lsb);
	offset = (u16)((msb << 8) | lsb);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%u\n", capMain, useful, offset);
}

static ssize_t sx9500_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9500_p *data = dev_get_drvdata(dev);

	/* It's for init touch */
		return snprintf(buf, PAGE_SIZE, "%d\n",
			sx9500_get_init_threshold(data));
}

static ssize_t sx9500_threshold_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	struct sx9500_p *data = dev_get_drvdata(dev);

	/* It's for normal touch */
	if (strict_strtoul(buf, 10, &val)) {
		pr_err("[SX9500]: %s - Invalid Argument\n", __func__);
		return -EINVAL;
	}

	pr_info("[SX9500]: %s - normal threshold %lu\n", __func__, val);
	data->touchTh = (u8)val;

	return count;
}

static ssize_t sx9500_onoff_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9500_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", !data->flagDataSkip);
}

static ssize_t sx9500_onoff_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	u8 val;
	int ret;
	struct sx9500_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &val);
	if (ret) {
		pr_err("[SX9500]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	if (val == 0)
		data->flagDataSkip = true;
	else
		data->flagDataSkip = false;

	pr_info("[SX9500]: %s -%u\n", __func__, val);
	return count;
}

static ssize_t sx9500_calibration_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	struct sx9500_p *data = dev_get_drvdata(dev);

	if ((data->calSuccessed == false) && (data->calData[0] == 0))
		ret = CAL_RET_NONE;
	else if ((data->calSuccessed == false) && (data->calData[0] != 0))
		ret = CAL_RET_EXIST;
	else if ((data->calSuccessed == true) && (data->calData[0] != 0))
		ret = CAL_RET_SUCCESS;
	else
		ret = CAL_RET_ERROR;

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", ret,
			data->calData[1], data->calData[2]);
}

static ssize_t sx9500_calibration_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	bool do_calib;
	int ret;
	struct sx9500_p *data = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "1"))
		do_calib = true;
	else if (sysfs_streq(buf, "0"))
		do_calib = false;
	else {
		pr_info("[SX9500]: %s - invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	ret = sx9500_do_calibrate(data, do_calib);
	if (ret < 0) {
		pr_err("[SX9500]: %s - sx9500_do_calibrate fail(%d)\n",
			__func__, ret);
		goto exit;
	}

	ret = sx9500_save_caldata(data);
	if (ret < 0) {
		pr_err("[SX9500]: %s - sx9500_save_caldata fail(%d)\n",
			__func__, ret);
		memset(data->calData, 0, sizeof(int) * 3);
		goto exit;
	}

	pr_info("[SX9500]: %s - %u success!\n", __func__, do_calib);

exit:

	if ((data->calData[0] != 0) && (ret >= 0))
		data->calSuccessed = true;
	else
		data->calSuccessed = false;

	return count;
}

static DEVICE_ATTR(menual_calibrate, S_IRUGO | S_IWUSR | S_IWGRP,
		sx9500_get_offset_calibration_show,
		sx9500_set_offset_calibration_store);
static DEVICE_ATTR(register_write, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, sx9500_register_write_store);
static DEVICE_ATTR(register_read, S_IRUGO | S_IWUSR | S_IWGRP,
		NULL, sx9500_register_read_store);
static DEVICE_ATTR(readback, S_IRUGO, sx9500_read_data_show, NULL);
static DEVICE_ATTR(reset, S_IRUGO, sx9500_sw_reset_show, NULL);

static DEVICE_ATTR(name, S_IRUGO, sx9500_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, sx9500_vendor_show, NULL);
static DEVICE_ATTR(mode, S_IRUGO, sx9500_touch_mode_show, NULL);
static DEVICE_ATTR(raw_data, S_IRUGO, sx9500_raw_data_show, NULL);
static DEVICE_ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP,
		sx9500_calibration_show, sx9500_calibration_store);
static DEVICE_ATTR(onoff, S_IRUGO | S_IWUSR | S_IWGRP,
		sx9500_onoff_show, sx9500_onoff_store);
static DEVICE_ATTR(threshold, S_IRUGO | S_IWUSR | S_IWGRP,
		sx9500_threshold_show, sx9500_threshold_store);

static struct device_attribute *sensor_attrs[] = {
	&dev_attr_menual_calibrate,
	&dev_attr_register_write,
	&dev_attr_register_read,
	&dev_attr_readback,
	&dev_attr_reset,
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_mode,
	&dev_attr_raw_data,
	&dev_attr_threshold,
	&dev_attr_onoff,
	&dev_attr_calibration,
	NULL,
};

/*****************************************************************************/
static ssize_t sx9500_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u8 enable;
	int ret;
	struct sx9500_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &enable);
	if (ret) {
		pr_err("[SX9500]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	pr_info("[SX9500]: %s - new_value = %u\n", __func__, enable);
	if ((enable == 0) || (enable == 1))
		sx9500_set_enable(data, (int)enable);

	return size;
}

static ssize_t sx9500_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sx9500_p *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&data->enable));
}

static ssize_t sx9500_flush_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u8 enable;
	int ret;
	struct sx9500_p *data = dev_get_drvdata(dev);

	ret = kstrtou8(buf, 2, &enable);
	if (ret) {
		pr_err("[SX9500]: %s - Invalid Argument\n", __func__);
		return ret;
	}

	if (enable == 1) {
		mutex_lock(&data->mode_mutex);
		input_report_rel(data->input, REL_MAX, 1);
		input_sync(data->input);
		mutex_unlock(&data->mode_mutex);
	}

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
		sx9500_enable_show, sx9500_enable_store);
static DEVICE_ATTR(flush, S_IWUSR | S_IWGRP,
		NULL, sx9500_flush_store);

static struct attribute *sx9500_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_flush.attr,
	NULL
};

static struct attribute_group sx9500_attribute_group = {
	.attrs = sx9500_attributes
};

static void sx9500_touch_process(struct sx9500_p *data)
{
	u8 status = 0;
	int cnt;
	s32 capMain, threshold;
	threshold = sx9500_get_init_threshold(data);

	capMain = sx9500_get_capMain(data);

	sx9500_i2c_read(data, SX9500_TCHCMPSTAT_REG, &status);
	for (cnt = 0; cnt < TOTAL_BOTTON_COUNT; cnt++) {
		if (data->state[cnt] == IDLE) {
			if (status & (CSX_STATUS_REG << cnt)) {
#ifdef DEFENCE_CODE_FOR_DEVICE_DAMAGE
				if ((data->calData[0] - 5000) > capMain) {
					sx9500_set_offset_calibration(data);
					mdelay(400);
					sx9500_touchCheckWithRefSensor(data);
					pr_err("[SX9500]: %s - Defence code for"
						" device damage\n", __func__);
					return;
				}
#endif
				send_event(data, cnt, ACTIVE);
			} else {
				pr_info("[SX9500]: %s - %d already released.\n",
					__func__, cnt);
			}
		} else { /* User released button */
			if (!(status & (CSX_STATUS_REG << cnt))) {
				if ((data->touchMode == INIT_TOUCH_MODE)
					&& (capMain >= threshold))
					pr_info("[SX9500]: %s - IDLE SKIP\n",
						__func__);
				else
					send_event(data, cnt, IDLE);

			} else {
				pr_info("[SX9500]: %s - %d still touched\n",
					__func__, cnt);
			}
		}
	}
}

static void sx9500_process_interrupt(struct sx9500_p *data)
{
	u8 status = 0;

	/* since we are not in an interrupt don't need to disable irq. */
	status = sx9500_read_irqstate(data);

	if (status & IRQ_PROCESS_CONDITION)
		sx9500_touch_process(data);
}

static void sx9500_init_work_func(struct work_struct *work)
{
	struct sx9500_p *data = container_of((struct delayed_work *)work,
		struct sx9500_p, init_work);

	sx9500_initialize_chip(data);

	/* make sure no interrupts are pending since enabling irq
	 * will only work on next falling edge */
	sx9500_read_irqstate(data);
}

static void sx9500_irq_work_func(struct work_struct *work)
{
	struct sx9500_p *data = container_of((struct delayed_work *)work,
		struct sx9500_p, irq_work);

	sx9500_process_interrupt(data);
}

#ifdef CONFIG_SENSORS_GRIP_ADJDET
static irqreturn_t sx9500_adjdet_interrupt_thread(int irq, void *pdata)
{
	struct sx9500_p *data = pdata;

	if (gpio_get_value_cansleep(data->gpio_adjdet) == 0)
		pr_info("[SX9500]: %s : adj detect cable disconnect!" \
				" grip sensor enable\n", __func__);
	else {
		pr_info("[SX9500]: %s : adj detect cable connect!" \
				" grip sensordisable\n", __func__);
		if (data->grip_state == ACTIVE) {
			pr_info("[SX9500]: %s : Send FAR(IDLE)\n", __func__);
			input_report_rel(data->input, REL_MISC, 2);
			input_sync(data->input);
			data->grip_state = IDLE;
		}
	}

	return IRQ_HANDLED;
}
#endif

static irqreturn_t sx9500_interrupt_thread(int irq, void *pdata)
{
	struct sx9500_p *data = pdata;

	if (sx9500_get_nirq_state(data) == 1) {
		pr_err("[SX9500]: %s - nirq read high\n", __func__);
	} else {
		wake_lock_timeout(&data->grip_wake_lock, 3 * HZ);
		schedule_delayed_work(&data->irq_work, msecs_to_jiffies(100));
	}

	return IRQ_HANDLED;
}

static int sx9500_input_init(struct sx9500_p *data)
{
	int ret = 0;
	struct input_dev *dev = NULL;

	/* Create the input device */
	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = MODULE_NAME;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_REL, REL_MISC);
	input_set_capability(dev, EV_REL, REL_MAX);
	input_set_drvdata(dev, data);

	ret = input_register_device(dev);
	if (ret < 0) {
		input_free_device(dev);
		return ret;
	}

	ret = sensors_create_symlink(&dev->dev.kobj, dev->name);
	if (ret < 0) {
		input_unregister_device(dev);
		return ret;
	}

	ret = sysfs_create_group(&dev->dev.kobj, &sx9500_attribute_group);
	if (ret < 0) {
		sensors_remove_symlink(&data->input->dev.kobj,
			data->input->name);
		input_unregister_device(dev);
		return ret;
	}

	/* save the input pointer and finish initialization */
	data->input = dev;

	return 0;
}

static int sx9500_setup_pin(struct sx9500_p *data)
{
	int ret;

	ret = gpio_request(data->gpioNirq, "SX9500_nIRQ");
	if (ret < 0) {
		pr_err("[SX9500]: %s - gpio %d request failed (%d)\n",
			__func__, data->gpioNirq, ret);
		return ret;
	}

	ret = gpio_direction_input(data->gpioNirq);
	if (ret < 0) {
		pr_err("[SX9500]: %s - failed to set gpio %d as input (%d)\n",
			__func__, data->gpioNirq, ret);
		gpio_free(data->gpioNirq);
		return ret;
	}

#ifdef CONFIG_SENSORS_GRIP_ADJDET
	if (data->enable_adjdet == 1) {
		ret = gpio_request(data->gpio_adjdet, "SX9500_ADJDETIRQ");
		if (ret < 0) {
			pr_err("[SX9500]: %s - gpio %d request failed (%d)\n",
				__func__, data->gpio_adjdet, ret);
			return ret;
		}

		ret = gpio_direction_input(data->gpio_adjdet);

		if (ret < 0) {
			pr_err("[SX9500]: %s - failed to set" \
				" gpio %d as input (%d)\n",
				__func__, data->gpio_adjdet, ret);
			gpio_free(data->gpio_adjdet);
			return ret;
		}
	}
#endif
	return 0;
}

static void sx9500_initialize_variable(struct sx9500_p *data)
{
	int cnt;

	for (cnt = 0; cnt < TOTAL_BOTTON_COUNT; cnt++)
		data->state[cnt] = IDLE;

	data->touchMode = INIT_TOUCH_MODE;
	data->flagDataSkip = false;
	data->calSuccessed = false;
	memset(data->calData, 0, sizeof(int) * 3);
	atomic_set(&data->enable, OFF);

	data->initTh = (int)CONFIG_SENSORS_SX9500_INIT_TOUCH_THRESHOLD;
	pr_info("[SX9500]: %s - Init Touch Threshold : %d\n",
		__func__, data->initTh);

	data->touchTh = (u8)CONFIG_SENSORS_SX9500_NORMAL_TOUCH_THRESHOLD;
	pr_info("[SX9500]: %s - Normal Touch Threshold : %u\n",
		__func__, data->touchTh);
}

static int sx9500_parse_dt(struct sx9500_p *data, struct device *dev)
{
	struct device_node *dNode = dev->of_node;
	enum of_gpio_flags flags;

	if (dNode == NULL)
		return -ENODEV;

	data->gpioNirq = of_get_named_gpio_flags(dNode,
		"sx9500-i2c,nirq-gpio", 0, &flags);
	if (data->gpioNirq < 0) {
		pr_err("[SX9500]: %s - get gpioNirq error\n", __func__);
		return -ENODEV;
	}
#ifdef CONFIG_SENSORS_GRIP_ADJDET
	data->gpio_adjdet = of_get_named_gpio_flags(dNode,
		"sx9500-i2c,adjdet-gpio", 0, &flags);
	if (data->gpio_adjdet < 0) {
		pr_err("[SX9500]: %s - get gpio_adjdet error\n", __func__);
		data->enable_adjdet = 0;
	} else {
		pr_info("[SX9500]: %s - get gpio_adjdet success\n", __func__);
		data->enable_adjdet = 1;
	}
#endif
	return 0;
}

#if defined(CONFIG_SENSORS_SX9500_REGULATOR_ONOFF)
int sx9500_regulator_on(struct sx9500_p *data, bool onoff)
{
	int ret = -1;
	if (!data->vdd) {
		data->vdd = devm_regulator_get(&data->client->dev, "vdd");
		if (!data->vdd) {
			pr_err("%s: regulator pointer null vdd, rc=%d\n",
				__func__, ret);
			return ret;
		}
		ret = regulator_set_voltage(data->vdd, 2850000, 2850000);
		if (ret) {
			pr_err("%s: set voltage failed on vdd, rc=%d\n",
				__func__, ret);
			return ret;
		}
	}


	if (!data->ldo) {
		data->ldo = devm_regulator_get(&data->client->dev, "vdd_ldo");
		if (!data->ldo) {
			pr_err("%s: devm_regulator_get for vdd_ldo failed\n",
				__func__);
			return 0;
		}
	}

	if (onoff) {
		ret = regulator_enable(data->vdd);
		if (ret) {
			pr_err("%s: Failed to enable regulator vdd.\n",
				__func__);
			return ret;
		}

		ret = regulator_enable(data->ldo);
		if (ret) {
			pr_err("%s: Failed to enable regulator ldo.\n",
				__func__);
			return ret;
		}
	} else {
		ret = regulator_disable(data->vdd);
		if (ret) {
			pr_err("%s: Failed to disable regulator vdd.\n",
				__func__);
			return ret;
		}

		ret = regulator_disable(data->ldo);
		if (ret) {
			pr_err("%s: Failed to disable regulator ldo.\n",
				__func__);
			return ret;
		}

	}
	/*Delay added for wakeup of chip, before i2c-transactions */
	msleep(30);
	return 0;
}
#endif

#if defined(CONFIG_SEC_MILLET_PROJECT) \
	|| defined(CONFIG_SEC_MATISSE_PROJECT) \
	|| defined(CONFIG_SEC_BERLUTI_PROJECT) \
	|| defined(CONFIG_SEC_DEGAS_PROJECT) \
	|| defined(CONFIG_SEC_T8_PROJECT) \
	|| defined(CONFIG_SEC_T10_PROJECT)
int sx9500_power_on(struct sx9500_p *data, bool onoff)
{
	int ret = -1;
#if defined(CONFIG_SEC_MILLET_PROJECT) \
	|| defined(CONFIG_SEC_BERLUTI_PROJECT) \
	|| defined(CONFIG_SEC_DEGAS_PROJECT) \
	|| defined(CONFIG_SEC_T8_PROJECT)
	if (!data->L19) {
		data->L19 = regulator_get(&data->client->dev, "8226_l19");
		if (!data->L19) {
			pr_err("%s: regulator pointer null L19, rc=%d\n",
				__func__, ret);
			return ret;
		}
		ret = regulator_set_voltage(data->L19, 2850000, 2850000);
		if (ret) {
			pr_err("%s: set voltage failed on L19, rc=%d\n",
				__func__, ret);
			return ret;
		}
	}
#endif
#if defined(CONFIG_SEC_MATISSE_PROJECT) || defined(CONFIG_SEC_T10_PROJECT)
	if (!data->L19) {
		data->L19 = regulator_get(&data->client->dev, "8226_l15");
		if (!data->L19) {
			pr_err("%s: regulator pointer null L19, rc=%d\n",
				__func__, ret);
			return ret;
		}
		ret = regulator_set_voltage(data->L19, 2800000, 2800000);
		if (ret) {
			pr_err("%s: set voltage failed on L19, rc=%d\n",
				__func__, ret);
			return ret;
		}
	}
#endif

	if (!data->lvs1_1p8) {
		data->lvs1_1p8 = regulator_get(&data->client->dev, "8226_lvs1");
		if (!data->lvs1_1p8) {
			pr_err("%s: regulator_get for 8226_lvs1 failed\n",
				__func__);
			return 0;
		}
	}

	if (onoff) {
		ret = regulator_enable(data->L19);
		if (ret) {
			pr_err("%s: Failed to enable regulator l19.\n",
				__func__);
			return ret;
		}

		ret = regulator_enable(data->lvs1_1p8);
		if (ret) {
			pr_err("%s: Failed to enable regulator lvs1_1p8.\n",
				__func__);
			return ret;
		}
	} else {
		ret = regulator_disable(data->L19);
		if (ret) {
			pr_err("%s: Failed to disable regulatorl19.\n",
				__func__);
			return ret;
		}

		ret = regulator_disable(data->lvs1_1p8);
		if (ret) {
			pr_err("%s: Failed to disable regulator lvs1_1p8.\n",
				__func__);
			return ret;
		}

	}
	/*Delay added for wakeup of chip, before i2c-transactions */
	msleep(30);
	return 0;
}
#endif

static int sx9500_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct sx9500_p *data = NULL;

	pr_info("[SX9500]: %s - Probe Start!\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("[SX9500]: %s - i2c_check_functionality error\n",
			__func__);
		goto exit;
	}

	/* create memory for main struct */
	data = kzalloc(sizeof(struct sx9500_p), GFP_KERNEL);
	if (data == NULL) {
		pr_err("[SX9500]: %s - kzalloc error\n", __func__);
		ret = -ENOMEM;
		goto exit_kzalloc;
	}

	ret = sx9500_parse_dt(data, &client->dev);
	if (ret < 0) {
		pr_err("[SX9500]: %s - of_node error\n", __func__);
		ret = -ENODEV;
		goto exit_of_node;
	}

	ret = sx9500_setup_pin(data);
	if (ret) {
		pr_err("[SX9500]: %s - could not setup pin\n", __func__);
		goto exit_setup_pin;
	}

	i2c_set_clientdata(client, data);
	data->client = client;
#if defined(CONFIG_SENSORS_SX9500_REGULATOR_ONOFF)
	sx9500_regulator_on(data,1);
#endif

#if defined(CONFIG_SEC_MILLET_PROJECT) \
	|| defined(CONFIG_SEC_MATISSE_PROJECT) \
	|| defined(CONFIG_SEC_BERLUTI_PROJECT) \
	|| defined(CONFIG_SEC_DEGAS_PROJECT) \
	|| defined(CONFIG_SEC_T8_PROJECT) \
	|| defined(CONFIG_SEC_T10_PROJECT)
	sx9500_power_on(data, 1);
#endif
	/* read chip id */
	ret = sx9500_i2c_write(data, SX9500_SOFTRESET_REG, SX9500_SOFTRESET);
	if (ret < 0) {
		pr_err("[SX9500]: %s - chip reset failed %d\n", __func__, ret);
		goto exit_chip_reset;
	}

	ret = sx9500_input_init(data);
	if (ret < 0)
		goto exit_input_init;

	wake_lock_init(&data->grip_wake_lock,
		WAKE_LOCK_SUSPEND, "grip_wake_lock");

	sensors_register(data->factory_device, data, sensor_attrs, MODULE_NAME);
	sx9500_initialize_variable(data);

	INIT_DELAYED_WORK(&data->init_work, sx9500_init_work_func);
	INIT_DELAYED_WORK(&data->irq_work, sx9500_irq_work_func);

	mutex_init(&data->mode_mutex);

	data->irq = gpio_to_irq(data->gpioNirq);

	/* initailize interrupt reporting */
	ret = request_threaded_irq(data->irq, NULL, sx9500_interrupt_thread,
			IRQF_TRIGGER_FALLING , "sx9500_irq", data);
	if (ret < 0) {
		pr_err("[SX9500]: %s - failed to set request_threaded_irq %d" \
			" as returning (%d)\n", __func__, data->irq, ret);
		goto exit_request_threaded_irq;
	}

#ifdef CONFIG_SENSORS_GRIP_ADJDET
	if (data->enable_adjdet == 1) {
		data->irq_adjdet = gpio_to_irq(data->gpio_adjdet);
		/* initailize interrupt reporting */
		ret = request_threaded_irq(data->irq_adjdet, NULL,
				sx9500_adjdet_interrupt_thread,
				IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				"sx9500_adjdet_irq", data);
		if (ret < 0) {
			pr_err("[SX9500]: %s - failed to set" \
				" request_threaded_adjdet_irq %d" \
				" as returning (%d)\n",
				__func__, data->irq_adjdet, ret);
			goto exit_request_threaded_adjdet_irq;
		}
		data->grip_state = IDLE;
	}
#endif

	disable_irq(data->irq);

	schedule_delayed_work(&data->init_work, msecs_to_jiffies(300));

	pr_info("[SX9500]: %s - Probe done!\n", __func__);

	return 0;
#ifdef CONFIG_SENSORS_GRIP_ADJDET
exit_request_threaded_adjdet_irq:
	free_irq(data->irq, data);
#endif
exit_request_threaded_irq:
	mutex_destroy(&data->mode_mutex);
	sensors_unregister(data->factory_device, sensor_attrs);
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);
	wake_lock_destroy(&data->grip_wake_lock);
	sysfs_remove_group(&data->input->dev.kobj, &sx9500_attribute_group);
	input_unregister_device(data->input);
exit_input_init:
exit_chip_reset:
	gpio_free(data->gpioNirq);
exit_setup_pin:
exit_of_node:
	kfree(data);
exit_kzalloc:
exit:
	pr_err("[SX9500]: %s - Probe fail!\n", __func__);
	return ret;
}

static int __devexit sx9500_remove(struct i2c_client *client)
{
	struct sx9500_p *data = (struct sx9500_p *)i2c_get_clientdata(client);

	if (atomic_read(&data->enable) == ON)
		sx9500_set_mode(data, SX9500_MODE_SLEEP);

	cancel_delayed_work_sync(&data->init_work);
	cancel_delayed_work_sync(&data->irq_work);
	free_irq(data->irq, data);
#ifdef CONFIG_SENSORS_GRIP_ADJDET
	if (data->enable_adjdet == 1)
		free_irq(data->irq_adjdet, data);
#endif
	gpio_free(data->gpioNirq);

	wake_lock_destroy(&data->grip_wake_lock);
	sensors_unregister(data->factory_device, sensor_attrs);
	sensors_remove_symlink(&data->input->dev.kobj, data->input->name);
	sysfs_remove_group(&data->input->dev.kobj, &sx9500_attribute_group);
	input_unregister_device(data->input);
	mutex_destroy(&data->mode_mutex);

	kfree(data);

	return 0;
}

static int sx9500_suspend(struct device *dev)
{
	struct sx9500_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == ON)
		pr_info("[SX9500]: %s\n", __func__);

	return 0;
}

static int sx9500_resume(struct device *dev)
{
	struct sx9500_p *data = dev_get_drvdata(dev);

	if (atomic_read(&data->enable) == ON)
		pr_info("[SX9500]: %s\n", __func__);

	return 0;
}

static struct of_device_id sx9500_match_table[] = {
	{ .compatible = "sx9500-i2c",},
	{},
};

static const struct i2c_device_id sx9500_id[] = {
	{ "sx9500_match_table", 0 },
	{ }
};

static const struct dev_pm_ops sx9500_pm_ops = {
	.suspend = sx9500_suspend,
	.resume = sx9500_resume,
};

static struct i2c_driver sx9500_driver = {
	.driver = {
		.name	= MODEL_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sx9500_match_table,
		.pm = &sx9500_pm_ops
	},
	.probe		= sx9500_probe,
	.remove		= __devexit_p(sx9500_remove),
	.id_table	= sx9500_id,
};

static int __init sx9500_init(void)
{
	return i2c_add_driver(&sx9500_driver);
}

static void __exit sx9500_exit(void)
{
	i2c_del_driver(&sx9500_driver);
}

module_init(sx9500_init);
module_exit(sx9500_exit);

MODULE_DESCRIPTION("Semtech Corp. SX9500 Capacitive Touch Controller Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
