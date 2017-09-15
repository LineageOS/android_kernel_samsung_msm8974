/*
 * cypress_touchkey.c - Platform data for edp eeprom driver
 *
 * Copyright (C) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
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
#include <linux/regulator/consumer.h>
#include <asm/mach-types.h>
#include <linux/device.h>
#include <linux/of_gpio.h>

#include "mdss.h"
#include "mdss_panel.h"
#include "mdss_mdp.h"
#include "mdss_edp.h"

#if defined(CONFIG_SEC_LT03_PROJECT)
#include "n1_power_save.h"
#elif defined(CONFIG_SEC_PICASSO_PROJECT)
#include "picasso_power_save.h"
#else ////defined(CONFIG_SEC_VIENNA_PROJECT) || defined(CONFIG_SEC_V2_PROJECT)
#include "v1_power_save.h"
#endif

#if defined(CONFIG_EDP_TCON_MDNIE)
#include "edp_tcon_mdnie.h"
#endif

#define DDI_VIDEO_ENHANCE_TUNING
#if defined(DDI_VIDEO_ENHANCE_TUNING)
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/lcd.h>
#endif

#if defined(CONFIG_SEC_LT03_PROJECT) || defined(CONFIG_SEC_PICASSO_PROJECT)
#define LOWEST_PWM_DUTY 10
#else
#define LOWEST_PWM_DUTY 20
#endif

extern struct mutex edp_power_state_chagne;

struct edp_eeprom_platform_data {
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;

	/*for tuning sysfs*/
	struct device *dev;
	int mode;
	int lux;
	int auto_br;
	int power_save_mode;
};

struct edp_eeprom_info {
	struct i2c_client			*client;
	struct edp_eeprom_platform_data	*pdata;
};

static struct class *tcon_class;
static struct edp_eeprom_info *global_pinfo;

extern int get_edp_power_state(void);

static void eeprom_request_gpio_slave(struct edp_eeprom_platform_data *pdata)
{
	pr_info("%s gpio_scl : %d , gpio_sda : %d", __func__, pdata->gpio_scl, pdata->gpio_sda);

	gpio_tlmm_config(GPIO_CFG(pdata->gpio_scl, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_sda, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
}

static void eeprom_request_gpio_master(struct edp_eeprom_platform_data *pdata)
{
	pr_info("%s gpio_scl : %d , gpio_sda : %d", __func__, pdata->gpio_scl, pdata->gpio_sda);

	gpio_tlmm_config(GPIO_CFG(pdata->gpio_scl, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(pdata->gpio_sda, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
}

static int eeprom_i2c_read(struct i2c_client *client,
		u16 reg, u8 *val, unsigned int len)
{
	int err = 0;
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[2];
	u8 buf1[] = { reg >> 8, reg & 0xFF };

	msg[0].addr  = client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 2;
	msg[0].buf   = buf1;

	msg[1].addr  = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = 1;
	msg[1].buf   = val;

	err = i2c_transfer(adapter, msg, 2);

	if  (err == 2)
		pr_debug("%s ok", __func__);
	else
		pr_info("%s fail err = %d", __func__, err);

	return err;

}

static int eeprom_i2c_write(struct i2c_client *client,
		u16 *reg,  u8 *val, unsigned int len)
{
	int err = 0;
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg[len];
	u8 buf[len][3];
	int loop;

	for (loop = 0; loop < len; loop++) {
		buf[loop][0] = reg[loop] >> 8;
		buf[loop][1] = reg[loop] & 0xFF;
		buf[loop][2] = val[loop];

		msg[loop].addr  = client->addr;
		msg[loop].flags = 0x00;
		msg[loop].len   = 3;
		msg[loop].buf   = &buf[loop][0];
	}

	err = i2c_transfer(adapter, msg, len);

	if  (err == len)
		pr_debug("%s ok", __func__);
	else
		pr_info("%s fail err = %d", __func__, err);

	return err;
}
void tcon_i2c_slave_change(void)
{
	unsigned char data[3];

	/* i2c slave change */
	data[0] = 0x03;
	data[1] = 0x13;
	data[2] = 0xBB;
	aux_tx(0x491, data, 3);

	data[0] = 0x03;
	data[1] = 0x14;
	data[2] = 0xBB;
	aux_tx(0x491, data, 3);

	if (global_pinfo)
		eeprom_request_gpio_master(global_pinfo->pdata);
	else
		pr_info("%s global_pinfo is NULL", __func__);

#if defined(CONFIG_EDP_TCON_MDNIE)
	/* TO enable MDNIE*/
	data[0] = 0x08;
	aux_tx(0x720, data, 1);
	update_mdnie_register();
#endif
}
static void tcon_init_setting(void)
{
	u16 i2c_addr[4];
	u8 i2c_data[4];
	unsigned char data[3];

	/* i2c slave change */
	data[0] = 0x03;
	data[1] = 0x13;
	data[2] = 0xBB;
	aux_tx(0x491, data, 3);

	data[0] = 0x03;
	data[1] = 0x14;
	data[2] = 0xBB;
	aux_tx(0x491, data, 3);

	/* TO enable MDNIE*/
	data[0] = 0x08;
	aux_tx(0x720, data, 1);

	/* TCON SETTING FOR ESD RECOVERY*/
	data[0] = 0x81;
	data[1] = 0x68;
	data[2] = 0x04;
	aux_tx(0x491, data, sizeof(data));

	if (!global_pinfo) {
		pr_info("%s global_pinfo is NULL", __func__);
		return ;
	}

	eeprom_request_gpio_master(global_pinfo->pdata);

	/* low revision panel needs update display resolution */
	i2c_addr[0] = 0xCB2; i2c_data[0] = 0x0;
	i2c_addr[1] = 0xCB3; i2c_data[1] = 0x0A;
	i2c_addr[2] = 0xCB4; i2c_data[2] = 0x40;
	i2c_addr[3] = 0xCB5; i2c_data[3] = 0x06;
	eeprom_i2c_write(global_pinfo->client, i2c_addr, i2c_data, 4);

}

int mdnie_tune_cmd(short *tune_data, int len)
{
	int data_pos;
	u16 i2c_addr[len];
	u8 i2c_data[len];

	mutex_lock(&edp_power_state_chagne);

	if (!get_edp_power_state()) {
		pr_info("%s get_edp_power_state off", __func__);
		mutex_unlock(&edp_power_state_chagne);
		return -EINVAL;
	}

	if (!global_pinfo) {
		pr_info("%s global_pinfo is NULL", __func__);
		mutex_unlock(&edp_power_state_chagne);
		return -EINVAL;
	}

	//Send Tune Commands
	for(data_pos = 0;data_pos < len ;data_pos++) {
		i2c_addr[data_pos] = tune_data[data_pos * 2];
		i2c_data[data_pos] = (char)(tune_data[(data_pos * 2)+1]);
	}
	eeprom_i2c_write(global_pinfo->client, i2c_addr, i2c_data, len);
#if 0
	for(data_pos = 0;data_pos < len ;data_pos++) {
		eeprom_i2c_read(global_pinfo->client, tune_data[data_pos * 2], data, 1);
		pr_info("0x%04x,0x%02x\n", tune_data[data_pos * 2], data[0]);
	}
#endif
	mutex_unlock(&edp_power_state_chagne);

	return 0;
}

#if defined(DDI_VIDEO_ENHANCE_TUNING)
#define MAX_FILE_NAME 128
#define TUNING_FILE_PATH "/sdcard/"
#define MDNIE_TUNE_SIZE 98
static char tuning_file[MAX_FILE_NAME];
static short mdni_addr[MDNIE_TUNE_SIZE];
static char mdni_tuning_val[MDNIE_TUNE_SIZE];

#define TCON_TUNE_SIZE 20
static short tcon_addr[TCON_TUNE_SIZE];
static char tcon_tuning_val[TCON_TUNE_SIZE];

static char char_to_dec(char data1, char data2)
{
	char dec;

	dec = 0;

	if (data1 >= 'a') {
		data1 -= 'a';
		data1 += 10;
	} else if (data1 >= 'A') {
		data1 -= 'A';
		data1 += 10;
	} else
		data1 -= '0';

	dec = data1 << 4;

	if (data2 >= 'a') {
		data2 -= 'a';
		data2 += 10;
	} else if (data2 >= 'A') {
		data2 -= 'A';
		data2 += 10;
	} else
		data2 -= '0';

	dec |= data2;

	return dec;
}

static void sending_tune_cmd(struct edp_eeprom_info *info, char *src, int len)
{
	int data_pos;
	int cmd_step;
	int cmd_pos;
	char data[3] = {0,};

	cmd_step = 0;
	cmd_pos = 0;

	for (data_pos = 0; data_pos < len;) {
		/* skip comments*/
		while(*(src+data_pos++) != 0x0A) ;
		if(*(src + data_pos) == '0') {
			//Addr
			mdni_addr[cmd_pos] = char_to_dec(*(src + data_pos),*(src + data_pos + 1))<<8 | char_to_dec(*(src + data_pos + 2),*(src + data_pos + 3));
			data_pos += 5;
			if((*(src + data_pos) == '0') && (*(src + data_pos + 1) == 'x'))
				mdni_tuning_val[cmd_pos] = char_to_dec(*(src + data_pos+2),*(src + data_pos + 3));
			data_pos  += 4;
			cmd_pos += 1;
		}
	}

	printk(KERN_INFO "\n");
	for (data_pos = 0; data_pos < MDNIE_TUNE_SIZE ; data_pos++)
		printk(KERN_INFO "0x%04x,0x%02x  \n", mdni_addr[data_pos],mdni_tuning_val[data_pos]);

	printk(KERN_INFO "\n");

	//Send Tune Commands
	eeprom_i2c_write(info->client, mdni_addr, mdni_tuning_val, MDNIE_TUNE_SIZE);

	for(data_pos = 0;data_pos < cmd_pos ;data_pos++) {
		eeprom_i2c_read(info->client, mdni_addr[data_pos], data, 1);
		pr_info("0x%04x,0x%02x\n",mdni_addr[data_pos], data[0]);
	}

}

static void sending_tcon_tuen_cmd(struct edp_eeprom_info *info, char *src, int len)
{
	int data_pos;
	int cmd_step;
	int cmd_pos;
	char data;

	u16 i2c_addr;
	u8 i2c_data;

	cmd_step = 0;
	cmd_pos = 0;

	for (data_pos = 0; data_pos < len;) {
		if(*(src + data_pos) == '0') {
			//Addr
			tcon_addr[cmd_pos] = char_to_dec(*(src + data_pos),*(src + data_pos + 1))<<8 | char_to_dec(*(src + data_pos + 2),*(src + data_pos + 3));
			data_pos += 5;
			if((*(src + data_pos) == '0') && (*(src + data_pos + 1) == 'x'))
				tcon_tuning_val[cmd_pos] = char_to_dec(*(src + data_pos+2),*(src + data_pos + 3));
			data_pos  += 4;
			cmd_pos += 1;
		} else
			data_pos++;
	}

	pr_info("cmd_pos : %d", cmd_pos);
	for (data_pos = 0; data_pos < cmd_pos ; data_pos++)
		pr_info("0x%04x,0x%02x", tcon_addr[data_pos],tcon_tuning_val[data_pos]);

	//Send Tune Commands
	eeprom_i2c_write(info->client, tcon_addr, tcon_tuning_val, TCON_TUNE_SIZE);

	//Enable double bufferd regiset
	i2c_addr = 0x0F10; i2c_data = 0x80;
	eeprom_i2c_write(info->client, &i2c_addr, &i2c_data, 1);

	for(data_pos = 0;data_pos < cmd_pos ;data_pos++) {
		eeprom_i2c_read(info->client, tcon_addr[data_pos], &data, 1);
		pr_info("0x%04x,0x%02x\n",tcon_addr[data_pos], data);
	}

}

static void load_tuning_file(struct edp_eeprom_info *info, char *filename, int mdnie)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret;
	mm_segment_t fs;

	pr_info("%s called loading file name : [%s]\n", __func__,
	       filename);

	fs = get_fs();
	set_fs(get_ds());

	filp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		printk(KERN_ERR "%s File open failed\n", __func__);
		goto err;
	}

	l = filp->f_path.dentry->d_inode->i_size;
	pr_info("%s Loading File Size : %ld(bytes)", __func__, l);

	dp = kmalloc(l + 10, GFP_KERNEL);
	if (dp == NULL) {
		pr_info("Can't not alloc memory for tuning file load\n");
		filp_close(filp, current->files);
		goto err;
	}
	pos = 0;
	memset(dp, 0, l);

	pr_info("%s before vfs_read()\n", __func__);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	pr_info("%s after vfs_read()\n", __func__);

	if (ret != l) {
		pr_info("vfs_read() filed ret : %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		goto err;
	}

	filp_close(filp, current->files);

	set_fs(fs);

	if (get_edp_power_state()) {
		if (mdnie)
			sending_tune_cmd(info, dp, l);
		else
			sending_tcon_tuen_cmd(info, dp, l);
	} else
		pr_info("%s get_edp_power_state off", __func__);

	kfree(dp);

	return;
err:
	set_fs(fs);
}



static ssize_t tuning_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	int ret = 0;

	ret = snprintf(buf, MAX_FILE_NAME, "Tunned File Name : %s\n",
								tuning_file);

	return ret;
}

static ssize_t tuning_store(struct device *dev,
			    struct device_attribute *attr, const char *buf,
			    size_t size)
{
	char *pt;
	struct edp_eeprom_info *info;

	info = dev_get_drvdata(dev);

	memset(tuning_file, 0, sizeof(tuning_file));
	snprintf(tuning_file, MAX_FILE_NAME, "%s%s", TUNING_FILE_PATH, buf);

	pt = tuning_file;
	while (*pt) {
		if (*pt == '\r' || *pt == '\n') {
			*pt = 0;
			break;
		}
		pt++;
	}

	pr_info("%s:%s\n", __func__, tuning_file);

	load_tuning_file(info, tuning_file, 1);

	return size;
}

static DEVICE_ATTR(tuning, 0664, tuning_show, tuning_store);
#endif

#if defined(CONFIG_LCD_CLASS_DEVICE) && defined(DDI_VIDEO_ENHANCE_TUNING)
static struct lcd_ops edp_samsung_disp_props = {
	.get_power = NULL,
	.set_power = NULL,
};
#endif

int tcon_tune_value(struct edp_eeprom_info *pinfo)
{
	int ret = 0;
	struct tcon_reg_info *tune_value;
	struct edp_eeprom_platform_data *pdata = pinfo->pdata;
	u16 i2c_addr;
	u8 i2c_data;

	mutex_lock(&edp_power_state_chagne);

	if (!get_edp_power_state()) {
		pr_info("%s get_edp_power_state off", __func__);
		mutex_unlock(&edp_power_state_chagne);
		return -EINVAL;
	}

	pr_info("%s [set tcon] : mode : %d, br : %d, lux : %d power_save_mode : %d", __func__,
		pdata->mode, pdata->auto_br, pdata->lux, pdata->power_save_mode);

	if (pdata->power_save_mode) {
		if (pdata->mode == 1 || pdata->mode == 2 || pdata->mode == 3)
			tune_value = power_save_tune_value[1][1][1]; /*TCON_VIDEO*/
		else if (pdata->mode == 8)
			tune_value = power_save_tune_value[1][1][8]; /*TCON_BROWSER*/
		else
			tune_value = power_save_tune_value[1][1][0]; /*TCON_POWER_SAVE*/
	} else
        	tune_value = power_save_tune_value[pdata->auto_br][pdata->lux][pdata->mode];

	if (!tune_value) {
		pr_err("%s tcon value is null", __func__);
		mutex_unlock(&edp_power_state_chagne);
		return -EINVAL;
	}

	eeprom_i2c_write(pinfo->client, tune_value->addr, tune_value->data, tune_value->reg_cnt);

	//Enable double bufferd regiset
	i2c_addr = 0x0F10; i2c_data = 0x80;
	eeprom_i2c_write(pinfo->client, &i2c_addr, &i2c_data, 1);

#if 0
	for(loop = 0; loop < tune_value->reg_cnt; loop++) {
		unsigned char val;
		eeprom_i2c_read(pinfo->client, tune_value->addr[loop], &val, 1);
		pr_info("%s read addr : 0x%x data : 0x%x", __func__, tune_value->addr[loop], val);
	}
#endif
	mutex_unlock(&edp_power_state_chagne);
	tcon_pwm_duty(0, 0);

	return ret;
}

void restore_set_tcon(void)
{
	if (global_pinfo) {
		tcon_init_setting();
		tcon_tune_value(global_pinfo);
#if defined(CONFIG_EDP_TCON_MDNIE)
		update_mdnie_register();
#endif
	}
}

void tcon_under_lowest_percentage_duty(void)
{
	u16 i2c_addr[3];
	u8 i2c_data[3];

	mutex_lock(&edp_power_state_chagne);

	if (!get_edp_power_state()) {
		pr_info("%s get_edp_power_state off", __func__);
		mutex_unlock(&edp_power_state_chagne);
		return ;
	}

	if (global_pinfo && global_pinfo->client) {
		i2c_addr[0] = 0x0DB9; i2c_data[0] = 0x7F; /* under 20% parameter 1*/
		i2c_addr[1] = 0x0DBA; i2c_data[1] = 0xFF; /* under 20% parameter 2*/
		i2c_addr[2] = 0x0F10; i2c_data[2] = 0x80; /* Enable double bufferd regiset */
		eeprom_i2c_write(global_pinfo->client, i2c_addr, i2c_data, 3);
	}

	mutex_unlock(&edp_power_state_chagne);
}

void tcon_pwm_duty(int pwm_duty, int updata_from_backlight)
{
	static int pre_duty = 100;
	static int lowest_pwm_duty = LOWEST_PWM_DUTY;

	if (updata_from_backlight) {
		if (pwm_duty < lowest_pwm_duty && pre_duty >= lowest_pwm_duty) {
			pre_duty = pwm_duty;
			tcon_under_lowest_percentage_duty();
			pr_info("%s pwm under 20%% duty ratio : %d backlight update : %d", __func__, pwm_duty, updata_from_backlight);
		} else if (pwm_duty >= lowest_pwm_duty && pre_duty < lowest_pwm_duty) {
			pre_duty = pwm_duty;
			tcon_tune_value(global_pinfo);
			pr_info("%s pwm over 20%% duty ratio : %d", __func__, pwm_duty);
		}
	} else {
		if (pre_duty < lowest_pwm_duty) {
			tcon_under_lowest_percentage_duty();
			pr_info("%s pwm under 20%% duty ratio : %d backlight update : %d", __func__, pwm_duty, updata_from_backlight);
		}
	}
}

static ssize_t disp_lcdtype_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_MACH_VIENNA_LTE) || defined(CONFIG_MACH_V2LTEEUR)
	snprintf(buf, 30, "INH_LSL122DL01\n");
#else // CONFIG_SEC_LT03_PROJECT
	snprintf(buf, 30, "INH_LSL101DL01\n");
#endif
	return strnlen(buf, 30);
}

static DEVICE_ATTR(lcd_type, S_IRUGO, disp_lcdtype_show, NULL);

static ssize_t auto_brightness_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t auto_brightness_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	return size;
}

static DEVICE_ATTR(auto_brightness, S_IRUGO | S_IWUSR | S_IWGRP,
			auto_brightness_show, auto_brightness_store);

static ssize_t show_lux(struct device *dev,
			     struct device_attribute *dev_attr, char *buf)
{
	struct edp_eeprom_info *pinfo = dev_get_drvdata(dev);
	struct edp_eeprom_platform_data *pdata = pinfo->pdata;

	return sprintf(buf, "%d\n", pdata->lux);

}

static ssize_t store_lux(struct device *dev,
			    struct device_attribute *dev_attr,
			    const char *buf, size_t count)
{
	int ret;
	unsigned int value;
	struct edp_eeprom_info *pinfo = dev_get_drvdata(dev);
	struct edp_eeprom_platform_data *pdata = pinfo->pdata;

	ret = kstrtouint(buf, 10, &value);

	if (ret)
		return ret;

	if (value >= TCON_LEVEL_MAX) {
		dev_err(pdata->dev, "undefied tcon illumiate value : %d\n\n", value);
		return count;
	}

	if (value != pdata->lux) {
		pdata->lux = value;
		ret = tcon_tune_value(pinfo);

	if (ret)
		dev_err(pdata->dev, "failed to tune tcon\n");
	}

	return count;
}

static DEVICE_ATTR(lux, 0664, show_lux, store_lux);

static ssize_t show_mode(struct device *dev,
			     struct device_attribute *dev_attr, char *buf)
{
	struct edp_eeprom_info *pinfo = dev_get_drvdata(dev);
	struct edp_eeprom_platform_data *pdata = pinfo->pdata;

	return sprintf(buf, "%d\n", pdata->mode);
}

static ssize_t store_mode(struct device *dev,
			    struct device_attribute *dev_attr,
			    const char *buf, size_t count)
{
	int ret;
	unsigned int value;
	struct edp_eeprom_info *pinfo = dev_get_drvdata(dev);
	struct edp_eeprom_platform_data *pdata = pinfo->pdata;

	ret = kstrtouint(buf, 10, &value);

	if (ret)
		return ret;

	if (value >= TCON_MODE_MAX) {
		dev_err(pdata->dev, "undefied tcon mode value : %d\n\n", value);
		return count;
	}

	if (value != pdata->mode) {
		pdata->mode = value;
		ret = tcon_tune_value(pinfo);

		if (ret)
			dev_err(pdata->dev, "failed to tune tcon\n");
	}

	return count;
}

static DEVICE_ATTR(mode, 0664, show_mode, store_mode);

static ssize_t show_auto_br(struct device *dev,
			     struct device_attribute *dev_attr, char *buf)
{
	struct edp_eeprom_info *pinfo = dev_get_drvdata(dev);
	struct edp_eeprom_platform_data *pdata = pinfo->pdata;

	return sprintf(buf, "%d\n", pdata->auto_br);
}

static ssize_t store_auto_br(struct device *dev,
			    struct device_attribute *dev_attr,
			    const char *buf, size_t count)
{
	int ret;
	unsigned int value;
	struct edp_eeprom_info *pinfo = dev_get_drvdata(dev);
	struct edp_eeprom_platform_data *pdata = pinfo->pdata;

	ret = kstrtouint(buf, 10, &value);

	if (ret)
		return ret;

	if (value >= TCON_AUTO_BR_MAX) {
		dev_err(pdata->dev, "undefied tcon auto br value : %d\n\n", value);
		return count;
	}

	if (value != pdata->auto_br) {
		pdata->auto_br = value;
		ret = tcon_tune_value(pinfo);
		if (ret)
			dev_err(pdata->dev, "failed to tune tcon\n");
	}

	return count;
}

static DEVICE_ATTR(auto_br, 0664, show_auto_br, store_auto_br);

static ssize_t store_black_test(struct device *dev,
			    struct device_attribute *dev_attr,
			    const char *buf, size_t count)
{
	int value;
	u16 i2c_addr;
	u8 i2c_data;

	struct edp_eeprom_info *pinfo = dev_get_drvdata(dev);

	sscanf(buf, "%d", &value);

	if (value) {
		mutex_lock(&edp_power_state_chagne);

		if (!get_edp_power_state()) {
			pr_info("%s get_edp_power_state off", __func__);
			mutex_unlock(&edp_power_state_chagne);
			return -EAGAIN;
		}

		eeprom_i2c_write(pinfo->client, TCON_BLACK_IMAGE_BLU_ENABLE.addr,
			TCON_BLACK_IMAGE_BLU_ENABLE.data, TCON_BLACK_IMAGE_BLU_ENABLE.reg_cnt);

		//Enable double bufferd regiset
		i2c_addr = 0x0F10; i2c_data = 0x80;
		eeprom_i2c_write(pinfo->client, &i2c_addr, &i2c_data, 1);

		mutex_unlock(&edp_power_state_chagne);
	} else {
		tcon_tune_value(pinfo);
	}

	pr_info("%s value : %d", __func__, value);

	return count;
}

static DEVICE_ATTR(black_test, 0664, NULL, store_black_test);

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
int config_cabc(int power_save_mode)
{
	if (!global_pinfo)
		pr_info("%s global_pinfo is NULL", __func__);

	global_pinfo->pdata->power_save_mode = power_save_mode;

	tcon_tune_value(global_pinfo);

	return 0;
}

int config_i2c_lane(int enable)
{
	if (!global_pinfo) {
		pr_info("%s global_pinfo is NULL", __func__);
		return -ENOMEM;
	}

	if (enable) {
		gpio_request(global_pinfo->pdata->gpio_scl, "edp_scl");
		gpio_request(global_pinfo->pdata->gpio_sda, "edp_sda");
	} else {
		gpio_free(global_pinfo->pdata->gpio_scl);
		gpio_free(global_pinfo->pdata->gpio_sda);
	}

	return 0;
}
#endif


#if defined(DDI_VIDEO_ENHANCE_TUNING)
static ssize_t store_tcon_test(struct device *dev,
			    struct device_attribute *dev_attr,
			    const char *buf, size_t count)
{
	char *pt;
	struct edp_eeprom_info *info;

	info = dev_get_drvdata(dev);

	memset(tuning_file, 0, sizeof(tuning_file));
	snprintf(tuning_file, MAX_FILE_NAME, "%s%s", TUNING_FILE_PATH, buf);

	pt = tuning_file;
	while (*pt) {
		if (*pt == '\r' || *pt == '\n') {
			*pt = 0;
			break;
		}
		pt++;
	}

	pr_info("%s:%s\n", __func__, tuning_file);

	load_tuning_file(info, tuning_file, 0);

	return count;
}

static DEVICE_ATTR(tcon_test, 0664, NULL, store_tcon_test);
#endif

static struct attribute *sysfs_attributes[] = {
	&dev_attr_lux.attr,
	&dev_attr_mode.attr,
	&dev_attr_auto_br.attr,
	&dev_attr_black_test.attr,
#ifdef DDI_VIDEO_ENHANCE_TUNING
	&dev_attr_tcon_test.attr,
#endif
	NULL
};

static const struct attribute_group sysfs_group = {
	.attrs = sysfs_attributes,
};

static int edp_eeprom_parse_dt(struct device *dev,
			struct edp_eeprom_platform_data *pdata)
{
	struct device_node *np = dev->of_node;

	/* reset, irq gpio info */
	pdata->gpio_scl = of_get_named_gpio_flags(np, "edp,scl-gpio",
				0, &pdata->scl_gpio_flags);
	pdata->gpio_sda = of_get_named_gpio_flags(np, "edp,sda-gpio",
				0, &pdata->sda_gpio_flags);

	pr_info("%s gpio_scl : %d , gpio_sda : %d", __func__, pdata->gpio_scl, pdata->gpio_sda);
	return 0;
}

static int __devinit edp_eeprom_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct edp_eeprom_platform_data *pdata;
	struct edp_eeprom_info *info;

	int error;
	int ret = 0;

#if defined(CONFIG_LCD_CLASS_DEVICE) && defined(DDI_VIDEO_ENHANCE_TUNING)
	struct lcd_device *lcd_device;
	struct backlight_device *bd = NULL;
#endif

	pr_info("%s", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct edp_eeprom_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_info(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		error = edp_eeprom_parse_dt(&client->dev, pdata);
		if (error)
			return error;
	} else
		pdata = client->dev.platform_data;

	eeprom_request_gpio_slave(pdata);

	global_pinfo = info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_info(&client->dev, "%s: fail to memory allocation.\n", __func__);
		goto return_pdata_free;
	}

	info->client = client;
	info->pdata = pdata;

	i2c_set_clientdata(client, info);

#if defined(CONFIG_LCD_CLASS_DEVICE) && defined(DDI_VIDEO_ENHANCE_TUNING)
	lcd_device = lcd_device_register("panel", &client->dev, info,
				&edp_samsung_disp_props);

	if (IS_ERR(lcd_device)) {
		ret = PTR_ERR(lcd_device);
		printk(KERN_ERR "lcd : failed to register device\n");
		return ret;
	}

	ret = sysfs_create_file(&lcd_device->dev.kobj,
					&dev_attr_lcd_type.attr);
	if (ret) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_lcd_type.attr.name);
	}

	ret = sysfs_create_file(&lcd_device->dev.kobj,
			&dev_attr_tuning.attr);
	if (ret) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_tuning.attr.name);
	}

	bd = backlight_device_register("panel", &lcd_device->dev,
						NULL, NULL, NULL);
	if (IS_ERR(bd)) {
		ret = PTR_ERR(bd);
		pr_info("backlight : failed to register device\n");
		return ret;
	}

	ret = sysfs_create_file(&bd->dev.kobj,
					&dev_attr_auto_brightness.attr);
	if (ret) {
		pr_info("sysfs create fail-%s\n",
				dev_attr_auto_brightness.attr.name);
	}
#endif

	pdata->mode = 0; /* basic */
	pdata->lux = 0;
	pdata->auto_br = 0;
	pdata->power_save_mode = 0;

	tcon_class = class_create(THIS_MODULE, "tcon");

	if (IS_ERR(tcon_class)) {
		dev_err(&client->dev, "Failed to create class for TCON\n");
        		goto return_mem_free;
    	}

	pdata->dev = device_create(tcon_class,\
		NULL, 0, pdata, "tcon");

	if (IS_ERR(pdata->dev)) {
		dev_err(&client->dev, "Failed to create device for TCON\n");
 		goto return_class_remove;
	}

	ret = sysfs_create_group(&pdata->dev->kobj, &sysfs_group);
	if (ret) {
		dev_err(&client->dev, "Failed to create sysfs for TCON\n");
		goto return_sysfs_remove;
	}

	dev_set_drvdata(pdata->dev, info);
	return error;

return_sysfs_remove:
	device_destroy(tcon_class, 0);

return_class_remove:
	class_destroy(tcon_class);

return_mem_free:
	kfree(info);

return_pdata_free:
	kfree(pdata);

	return ret;
}

static int __devexit edp_eeprom_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id edp_eeprom_id[] = {
	{"edp_eeprom", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, edp_eeprom_id);

static struct of_device_id edp_eeprom_match_table[] = {
	{ .compatible = "edp,eeprom-control",},
	{ },
};
MODULE_DEVICE_TABLE(of, edp_eeprom_match_table);

struct i2c_driver edp_eeprom_driver = {
	.probe = edp_eeprom_probe,
	.remove = edp_eeprom_remove,
	.driver = {
		.name = "edp_eeprom",
		.owner = THIS_MODULE,
		.of_match_table = edp_eeprom_match_table,
		   },
	.id_table = edp_eeprom_id,
};

static int __init edp_eeprom_init(void)
{

	int ret = 0;

	pr_info("%s", __func__);

	ret = i2c_add_driver(&edp_eeprom_driver);
	if (ret) {
		printk(KERN_ERR "edp_eeprom_init registration failed. ret= %d\n",
			ret);
	}

	return ret;
}

static void __exit edp_eeprom_exit(void)
{
	pr_info("%s", __func__);
	i2c_del_driver(&edp_eeprom_driver);
}

module_init(edp_eeprom_init);
module_exit(edp_eeprom_exit);

MODULE_DESCRIPTION("edp eeprom driver");
MODULE_LICENSE("GPL");
