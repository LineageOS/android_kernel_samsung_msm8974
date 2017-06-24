/*
 * driver/irda IRDA driver
 *
 * Copyright (C) 2012 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 ************************************************************************************************************
 * Compile this driver when MC96FR116 is powered by LDO shared with other peripherals.
 * LDO control in this case is only for IRLED power (3.3V)
 * Vendor: ABOV Semiconductor
 * Part Name: MC96FR116CU
 * Vendor Contact:  byoungsu.jeong@abov.co.kr, youngdo.yi@abov.co.kr, wooyoung.lee@abov.co.kr
 * Driver modelled on ir_remote_mc96fr116.c
 *
 * Revision History:
 * #	Author		Description
 * 1	jayant.s47	Basic Bringup for Kmini ATT
 * 2	jayant.s47	Increase delay for I2C read success in Kmini 3G
 * 3	jayant.s47	Kmini ATT bringup: Modifications in fw_update to align it
 *			with vendor specifications. Optimize fw_update routine by removing useless delays.
 *************************************************************************************************************
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/ir_remote_con_mc96.h>
#include <linux/regulator/consumer.h>
#if defined (CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif
#include <linux/earlysuspend.h>

#include "irda_fw_version202.h"
#include "irda_fw_version103.h"
#include "irda_fw_version104.h"

#include <mach/gpio.h>
#include <linux/ir_remote_con_mc96.h>
#define MAX_SIZE 2048
#define MC96_READ_LENGTH	8
#define DUMMY 0xffff

#define MC96FR116C_0x101	0x101
#define MC96FR116C_0x103	0x103
//#define DEBUG 1

struct ir_remocon_data {
	struct mutex			mutex;
	struct i2c_client		*client;
	struct mc96_platform_data	*pdata;
	struct early_suspend		early_suspend;
	char signal[MAX_SIZE];
	int length;
	int count;
	int dev_id;
	int ir_freq;
	int ir_sum;
	int on_off;
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ir_remocon_early_suspend(struct early_suspend *h);
static void ir_remocon_late_resume(struct early_suspend *h);
#endif

static int count_number;
static int ack_number;
static int download_pass;
static int update_retry_count;
#define MAX_UPDATE_RETRY 5

#define TEST_ONLY 0 /* Enable this only to test when ADB isn't available */
#if TEST_ONLY
static void irda_remocon_work(struct ir_remocon_data *ir_data, int count);
static void test_store(struct ir_remocon_data *data, const char *buf) {
	int i;
	data->pdata->ir_wake_en(data->pdata,0);
	udelay(200);
	data->pdata->ir_wake_en(data->pdata,1);
	msleep(30);
	data->ir_freq = 38400;
	data->signal[2] = 0x00;
	data->count = 3;
	for(i = 0; i < 138; i++)
		data->signal[data->count++] = buf[i];
	irda_remocon_work(data, data->count);
}

static void irda_test_probe(struct ir_remocon_data *data) {
	/* Send IRDA byte sequence for volume up */
	unsigned char test[] = {
		0x96,0x00,0x00,0xAD,0x00,0xAB,0x00,0x18,0x00,0x3E,0x00,0x18,
		0x00,0x3D,0x00,0x18,0x00,0x3E,0x00,0x18,0x00,0x11,0x00,0x18,
		0x00,0x11,0x00,0x18,0x00,0x12,0x00,0x18,0x00,0x11,0x00,0x18,
		0x00,0x13,0x00,0x16,0x00,0x3E,0x00,0x18,0x00,0x3D,0x00,0x18,
		0x00,0x3E,0x00,0x18,0x00,0x13,0x00,0x16,0x00,0x11,0x00,0x19,
		0x00,0x11,0x00,0x18,0x00,0x11,0x00,0x18,0x00,0x11,0x00,0x18,
		0x00,0x3E,0x00,0x18,0x00,0x3D,0x00,0x19,0x00,0x3D,0x00,0x18,
		0x00,0x11,0x00,0x18,0x00,0x13,0x00,0x17,0x00,0x11,0x00,0x18,
		0x00,0x11,0x00,0x18,0x00,0x14,0x00,0x16,0x00,0x11,0x00,0x18,
		0x00,0x11,0x00,0x18,0x00,0x11,0x00,0x19,0x00,0x3D,0x00,0x18,
		0x00,0x3E,0x00,0x18,0x00,0x3D,0x00,0x18,0x00,0x3E,0x00,0x18,
		0x00,0x3D,0x00,0x18,0x07,0x58
	};
	test_store(data, test);
}
#endif

static void irda_wake_en_func(struct mc96_platform_data *pdata, bool onoff)
{
	/*Need to add parser before this*/
        gpio_direction_output(pdata->irda_wake_en, onoff);
        printk(KERN_ERR "%s: irda_wake_en : %d\n", __func__, onoff);
}

static void irda_gpio_init(struct mc96_platform_data *pdata)
{
	pdata->ir_wake_en = irda_wake_en_func;
	if(gpio_request(pdata->irda_wake_en, "IRDA Wakeup EN Pin"))
		pr_err("%s IRDA Wakeup EN GPIO Request failed\n",__func__);

	gpio_tlmm_config(GPIO_CFG(pdata->irda_wake_en,  0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);


	gpio_tlmm_config(GPIO_CFG(pdata->irda_poweron,  0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	gpio_request(pdata->irda_poweron, "irda_poweron");
	gpio_direction_output(pdata->irda_poweron, 1);

	if (gpio_request(pdata->irda_irq_gpio, "ira_irq"))
		pr_err("%s IRDA LED IRQ  GPIO Request failed\n",__func__);
	gpio_direction_input(pdata->irda_irq_gpio);

	gpio_tlmm_config(GPIO_CFG(pdata->irda_scl_gpio, 0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
	gpio_tlmm_config(GPIO_CFG(pdata->irda_sda_gpio, 0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
}

static int vled_ic_onoff;
static struct regulator *vled_ic;

static int irda_led_onoff(bool onoff)
{
	int ret = 0;
	if (onoff) {
		ret = regulator_set_voltage(vled_ic,3300000,3300000);
		if (ret) {
			pr_err("%s regulaor set volatge failed\n",__func__);
			goto regulator_fail;
		}
		ret = regulator_enable(vled_ic);
		if (ret) {
			pr_err("%s regulaor enable failed\n",__func__);
			goto regulator_fail;
		}
        vled_ic_onoff = 1;
		printk(KERN_CRIT "%s Regulator On!\n",__func__);
        } else if (vled_ic_onoff == 1) {
                ret = regulator_disable(vled_ic);
		if (ret) {
			pr_err("%s regulaor disable failed\n",__func__);
			goto regulator_fail;
		}
        vled_ic_onoff = 0;
		printk(KERN_CRIT "%s Regulator OFF!\n",__func__);
    }

	return ret;
	regulator_fail:
		regulator_put(vled_ic);
		return ret;
}
#define FW_RW_RETRY 2
static int irda_fw_update(struct ir_remocon_data *ir_data)
{
	struct ir_remocon_data *data = ir_data;
	struct i2c_client *client = data->client;
	int i=0, ret = 0, frame_count = 0;
	u8 buf_ir_test[8];
	const u8 *IRDA_fw;
	const u8 calc_chksum[] = {0x3A, 0x02, 0x10, 0x00, 0xF0, 0x20, 0xFF, 0xDF};
	IRDA_fw     = IRDA_binary_104;
	frame_count = FRAME_COUNT_104;

	/* Switch on chip in Bootloader mode, wake low */
	irda_led_onoff(1);
	data->pdata->ir_wake_en(data->pdata, 0);
	gpio_set_value(data->pdata->irda_poweron, 1);
	gpio_tlmm_config(GPIO_CFG(data->pdata->irda_irq_gpio,  0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	msleep(150);
	for(i = 0; i < FW_RW_RETRY; i++) {
		ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
		if (ret < 0) {
			pr_err(KERN_ERR " %s: err %d\n", __func__, ret);
		}
		else if((buf_ir_test[0] << 8 | buf_ir_test[1]) == 0x0001) {
			printk(KERN_CRIT "%s: Perform checksum calculation next, ret %d\n", __func__,ret);
			break;
		}
		msleep(60);
	}
	if (i == FW_RW_RETRY) {
		printk(KERN_CRIT "%s: Chip not responding, powerdown device\n", __func__);
		download_pass = 0;
		goto fw_update_end;
	}
	for(i = 0; i < FW_RW_RETRY; i++) {
		msleep(60);
		ret = i2c_master_send(client, calc_chksum, MC96_READ_LENGTH);
		if(ret < 0)
			continue;
		else
			break;
	}
	if(i == FW_RW_RETRY) {
		printk(KERN_CRIT "%s: checksum calculation fail, ret: %d\n", __func__, ret);
		download_pass = 0;
		goto fw_update_end;
	}
	msleep(60);
	/* Do a master read before downloading FW and validate device in bootloader mode */
	/* Expected sequence if correct FW 0x6E ,0xBA ,0x10 ,0x00 ,0x20 ,0xFF ,0x02 ,0x57 */
	/* Try reading twice */
	ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
	if (ret < 0) {
		msleep(50);
		ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
		if (ret < 0) {
			pr_err(KERN_ERR " %s: read fail after csum calc: err %d\n", __func__, ret);
			download_pass = 0;
			goto fw_update_end;
		}
	}

#ifdef DEBUG
	print_hex_dump(KERN_CRIT, "IRDA Master Rx: ", 16, 1,
					DUMP_PREFIX_ADDRESS, buf_ir_test, 8, 1);
#endif
	if((buf_ir_test[0] << 8 | buf_ir_test[1]) == 0x6E93) {
		printk(KERN_CRIT "%s: irda fw fine, exit now\n", __func__);
		download_pass = 1;
		goto powerdown_dev;
	}
	printk(KERN_CRIT "%s: Start download new Firmware\n", __func__);
	msleep(100);
	/* Start FW download */
	for (i = 0; i < frame_count; i++) {
		if (i == frame_count-1) {
			ret = i2c_master_send(client,
					&IRDA_fw[i * 70], 6);
			if (ret < 0) {
				printk(KERN_CRIT "%s: FW download master send fail\n", __func__);
				goto fw_update_end;
			}
		} else {
			ret = i2c_master_send(client,
					&IRDA_fw[i * 70], 70);
			if (ret < 0) {
				printk(KERN_CRIT "%s: FW download master send fail\n", __func__);
				goto fw_update_end;
			}
		}
		msleep(60);
	}
	/* Do a master read after downloading firmware and validate the FW
	 * Device still in bootmode, expected = 0x6E ,0xBA ,0x10 ,0x00 ,0x20 ,0xFF ,0x02 ,0x57
	 */
	msleep(100);
	ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
	if (ret < 0)
		printk(KERN_ERR "5. %s: err %d\n", __func__, ret);

#ifdef DEBUG
	print_hex_dump(KERN_CRIT, "IRDA Master Rx: ", 16, 1,
			DUMP_PREFIX_ADDRESS, buf_ir_test, 8, 1);
#endif

	ret = buf_ir_test[0] << 8 | buf_ir_test[1];
	if (ret == 0x6E93) {
		printk(KERN_INFO " IrDA new firmware downloaded\n");
		download_pass = 1;
		goto powerdown_dev;
	} else {
		printk(KERN_ERR "%s: FW Checksum fail after update\n", __func__);
	}
fw_update_end:
	printk(KERN_CRIT "%s: FAIL, power down device, ret = %d\n", __func__, ret);
	download_pass = 0;
	update_retry_count++;
	/* Check for old FW being present at last attempt to update to 1.3 */
	if(update_retry_count == (MAX_UPDATE_RETRY - 1)) {
		download_pass = 1;
		update_retry_count = 0;
		printk(KERN_CRIT "%s: using old firmware\n", __func__);
	}
powerdown_dev:
	data->pdata->ir_wake_en(data->pdata,0);
	gpio_set_value(data->pdata->irda_poweron, 0);
	irda_led_onoff(0);
	data->on_off = 0;
	return 0;
}

static void irda_add_checksum_length(struct ir_remocon_data *ir_data, int count)
{
	struct ir_remocon_data *data = ir_data;
	int i = 0, csum = 0;

#if 0
	printk(KERN_INFO "%s: length: %04x\n", __func__, count);
#endif
	data->signal[0] = count >> 8;
	data->signal[1] = count & 0xff;

	while (i < count) {
		csum += data->signal[i];
		i++;
	}

	printk(KERN_INFO "%s: checksum: %04x\n", __func__, csum);

	data->signal[count] = csum >> 8;
	data->signal[count+1] = csum & 0xff;

}

static int irda_read_device_info(struct ir_remocon_data *ir_data)
{
	struct ir_remocon_data *data = ir_data;
	struct i2c_client *client = data->client;
	u8 buf_ir_test[8];
	int ret;

	printk(KERN_INFO"%s called\n", __func__);
	gpio_set_value(data->pdata->irda_poweron, 1);

	data->pdata->ir_wake_en(data->pdata,1);
	msleep(60);
	ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	printk(KERN_INFO "%s: buf_ir dev_id: 0x%02x, 0x%02x\n", __func__,
			buf_ir_test[2], buf_ir_test[3]);
	ret = data->dev_id = (buf_ir_test[2] << 8 | buf_ir_test[3]);

	data->pdata->ir_wake_en(data->pdata,0);
	gpio_set_value(data->pdata->irda_poweron, 0);

	data->on_off = 0;
	return 0;
}
static void irda_reset_chip_user(struct ir_remocon_data *data) {
	irda_led_onoff(0);
	gpio_set_value(data->pdata->irda_poweron, 0);
	data->pdata->ir_wake_en(data->pdata,1);
	gpio_tlmm_config(GPIO_CFG(data->pdata->irda_irq_gpio,  0, GPIO_CFG_INPUT,
		GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	udelay(100);
	gpio_set_value(data->pdata->irda_poweron, 1);
	msleep(150);
	irda_led_onoff(1);
}

static void irda_remocon_work(struct ir_remocon_data *ir_data, int count)
{

	struct ir_remocon_data *data = ir_data;
	struct i2c_client *client = data->client;

	int buf_size = count+2;
	int ret, retry, ng_retry, sng_retry;
	int emission_time;
	int ack_pin_onoff;
#if defined(CONFIG_MACH_ATLANTICLTE_ATT) || defined(CONFIG_MACH_ATLANTICLTE_USC)
	int sleep_timing;
	int end_data;
#endif
#ifdef DEBUG
	int i;
	u8 buf[8];
#endif
	if (count_number >= 100)
		count_number = 0;

	count_number++;
	ng_retry = sng_retry = 0;
	data->on_off = 1;
	/* Power on in user IR mode */
	irda_reset_chip_user(ir_data);
	printk(KERN_INFO "%s: total buf_size: %d\n", __func__, buf_size);
#ifdef DEBUG
	ret = i2c_master_recv(client, buf, MC96_READ_LENGTH);
	if (ret < 0)
		printk(KERN_CRIT "%s: I2C read err %d\n", __func__, ret);

	print_hex_dump(KERN_CRIT, "irda: IRDA Master Rx: ", 16, 1,
				DUMP_PREFIX_ADDRESS, buf, 8, 1);
	printk("%s: print stored bytes\n", __func__);
	for (i = 0; i < buf_size; i++)
		printk(KERN_INFO "0x%02x, ", data->signal[i]);
	printk("\n");

#endif
	irda_add_checksum_length(data, count);

	mutex_lock(&data->mutex);

resend_data:
	ret = i2c_master_send(client, data->signal, buf_size);
	if (ret < 0) {
		dev_err(&client->dev, "%s: err1 %d\n", __func__, ret);
		ret = i2c_master_send(client, data->signal, buf_size);
		if (ret < 0)
			dev_err(&client->dev, "%s: err2 %d\n", __func__, ret);
	}

	msleep(10);
	ack_pin_onoff = 0;
	for(retry = 0; retry < 10; retry++) {
		if (gpio_get_value(data->pdata->irda_irq_gpio)) {
			if(retry == 9) {
				ng_retry++;
				if(ng_retry < 2) {
					irda_reset_chip_user(ir_data);
					goto resend_data;
				}
				printk(KERN_INFO "%s : %d Checksum NG!\n",
					__func__, count_number);
			}
			ack_pin_onoff = 1;
			msleep(3);
		} else {
			printk(KERN_INFO "%s : %d Checksum OK!\n",
				__func__, count_number);
			ack_pin_onoff = 2;
			break;
		}
	}
	ack_number = ack_pin_onoff;

	data->count = 2;
#if defined(CONFIG_MACH_ATLANTICLTE_ATT) || defined(CONFIG_MACH_ATLANTICLTE_USC)
	end_data = data->signal[count-2] << 8 | data->signal[count-1];
	emission_time = (1000 * (data->ir_sum - end_data) / (data->ir_freq)) + 10;
	sleep_timing = emission_time - 130;
	if (sleep_timing > 0)
		usleep(sleep_timing);
#endif

	emission_time = \
		(1000 * (data->ir_sum) / (data->ir_freq));
	if (emission_time > 0)
		usleep(emission_time);
	printk(KERN_INFO "%s: emission_time = %d\n",
				__func__, emission_time);

	for(retry = 0; retry < 3; retry++) {
		if (gpio_get_value(data->pdata->irda_irq_gpio)) {
			printk(KERN_INFO "%s : %d Sending IR OK!\n",
					__func__, count_number);
			ack_pin_onoff = 4;
			break;
		} else {
			if(retry == 2) {
				sng_retry++;
				if(sng_retry < 2) {
					irda_reset_chip_user(ir_data);
					goto resend_data;
				}
				printk(KERN_INFO "%s : %d Sending IR NG!\n",
					__func__, count_number);
			}
			ack_pin_onoff = 2;
			msleep(65);
		}
	}

	mutex_unlock(&data->mutex);

	ack_number += ack_pin_onoff;
#ifndef USE_STOP_MODE
	data->on_off = 1;
	//data->pdata->ir_wake_en(data->pdata,0);
	irda_led_onoff(0);
	//gpio_set_value(data->pdata->irda_poweron, 0);
#endif
	data->ir_freq = 0;
	data->ir_sum = 0;

	gpio_tlmm_config(GPIO_CFG(ir_data->pdata->irda_irq_gpio,  0, GPIO_CFG_INPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
}


static ssize_t remocon_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct ir_remocon_data *data = dev_get_drvdata(dev);
	unsigned int _data;
	int count, i, ret;

#ifdef DEBUG
	printk(KERN_CRIT "%s irda -- %s\n", __func__, buf);
#endif
	ret = 0;
	for (i = 0; i < MAX_SIZE; i++) {
		if (sscanf(buf++, "%u", &_data) == 1) {
			if (_data == 0 || buf == '\0')
				break;

			if (data->count == 2) {
				data->ir_freq = _data;
				data->signal[2] = 0x40; // Mode
				data->signal[3] = _data >> 16;
				data->signal[4] = (_data >> 8) & 0xFF;
				data->signal[5] = _data & 0xFF;
				data->count += 4;
			} else {
				data->ir_sum += _data;
				count = data->count;
				if(_data > 0x7FFF) {
					data->signal[count] = _data >> 24;
					data->signal[count+1] = _data >> 16;
					data->signal[count+2] = _data >> 8;
					data->signal[count+3] = _data & 0xFF;
					data->count += 4;
				} else {
					data->signal[count] = _data >> 8;
					data->signal[count+1] = _data & 0xFF;
					data->count += 2;
				}
			}

			while (_data > 0) {
				buf++;
				_data /= 10;
			}
		} else {
			break;
		}
	}
	irda_remocon_work(data, data->count);
	return size;
}

static ssize_t remocon_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct ir_remocon_data *data = dev_get_drvdata(dev);
	int i;
	char *bufp = buf;

	for (i = 5; i < MAX_SIZE - 1; i++) {
		if (data->signal[i] == 0 && data->signal[i+1] == 0)
			break;
		else
			bufp += sprintf(bufp, "%u,", data->signal[i]);
	}
	return strlen(buf);
}

static ssize_t remocon_ack(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	//struct ir_remocon_data *data = dev_get_drvdata(dev);

	printk(KERN_INFO "%s : ack_number = %d\n", __func__, ack_number);

	if (ack_number == 6)
		return sprintf(buf, "1\n");
	else
		return sprintf(buf, "0\n");
}

static DEVICE_ATTR(ir_send, 0664, remocon_show, remocon_store);
static DEVICE_ATTR(ir_send_result, 0664, remocon_ack, NULL);

static ssize_t check_ir_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct ir_remocon_data *data = dev_get_drvdata(dev);
	int ret;

	ret = irda_read_device_info(data);
	return snprintf(buf, 4, "%d\n", ret);
}

static DEVICE_ATTR(check_ir, 0664, check_ir_show, NULL);


#ifdef CONFIG_OF
static int irda_mc96_parse_dt(struct device *dev, struct mc96_platform_data *pdata)
{

        struct device_node *np = dev->of_node;

        pdata->irda_irq_gpio = of_get_named_gpio_flags(np, "mc96fr332,irda_irq_gpio",
                               0, &pdata->irq_gpio_flags);
	if (pdata->irda_irq_gpio < 0) {
		pr_err("%s failed to get irda_irq_gpio\n", __func__);
		return pdata->irda_irq_gpio;
	}

	pdata->irda_poweron = of_get_named_gpio_flags(np, "mc96fr332,irda_poweron",
                               0, &pdata->poweron_flags);
	if (pdata->irda_poweron < 0) {
		pr_err("%s failed to get irda_poweron\n", __func__);
		return pdata->irda_poweron;
	}

	pdata->irda_wake_en = of_get_named_gpio_flags(np, "mc96fr332,irda_wake",
				0, &pdata->wake_en_flags);
	if (pdata->irda_wake_en < 0) {
		pr_err("%s failed to get irda_wake_en\n", __func__);
		return pdata->irda_wake_en;
	}

	pdata->irda_scl_gpio = of_get_named_gpio_flags(np, "mc96fr332,scl-gpio",
                               0, &pdata->irda_scl_flags);

	if (pdata->irda_scl_gpio < 0) {
		pr_err("%s failed to get irda_scl_gpio\n", __func__);
		return pdata->irda_wake_en;
	}

	pdata->irda_sda_gpio = of_get_named_gpio_flags(np, "mc96fr332,sda-gpio",
                               0, &pdata->irda_sda_flags);

	if (pdata->irda_sda_gpio < 0) {
		pr_err("%s failed to get irda_sda_gpio\n", __func__);
		return pdata->irda_sda_gpio;
	}

	pr_info("%s: irq-gpio:%u poweron:%u wake_up:%u irda-scl:%u irda-sda:%u \n", __func__,
		 pdata->irda_irq_gpio,pdata->irda_poweron,pdata->irda_wake_en,pdata->irda_scl_gpio,pdata->irda_sda_gpio);

        return 0;
}
#endif

static int __devinit irda_remocon_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct ir_remocon_data *data;
	struct device *ir_remocon_dev;
	struct mc96_platform_data *pdata;
	int i, error;
	int ret;

	printk(KERN_INFO "%s start!\n", __func__);
	dev_info(&client->dev,"%s:ir_remocon probe called \n",__func__);
	if(client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct mc96_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory \n");
				return -ENOMEM;
		}
		ret = irda_mc96_parse_dt(&client->dev, pdata);
		if (ret < 0)
		{
			dev_err(&client->dev,"Parse failed \n");
			return ret;
		}
		irda_gpio_init(pdata);
	} else
		pdata = client->dev.platform_data;

	if (!pdata)
		return -EINVAL;

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	data = kzalloc(sizeof(struct ir_remocon_data), GFP_KERNEL);
	if (NULL == data) {
		pr_err("Failed to data allocate %s\n", __func__);
		error = -ENOMEM;
		goto err_free_mem;
	}

	data->client = client;
	if (client->dev.of_node)
	{
		data->pdata = pdata;
	}else
		data->pdata = client->dev.platform_data;

	mutex_init(&data->mutex);
	data->count = 2;

	vled_ic = regulator_get(&client->dev, "vled_3.3v");
	if (IS_ERR(vled_ic)) {
		pr_err("%s could not get regulator vled_3.3v\n",__func__);
		error = -EBUSY;
		goto err_free_mem;
	}

	i2c_set_clientdata(client, data);

	for (i = 0; i < MAX_UPDATE_RETRY; i++) {
		if (download_pass == 1)
			break;
		irda_fw_update(data);
	}
	if (download_pass != 1)
		goto err_fw_update_fail;
/*
	if(irda_led_onoff(0)) {
		pr_err("%s Regulator setting failed\n", __func__);
	}
//	irda_read_device_info(data);
*/
	ir_remocon_dev = device_create(sec_class, NULL, 0, data, "sec_ir");
#if TEST_ONLY
	for(i = 0; i < 10; i++)
		irda_test_probe(data);
#endif

	data->on_off = 1;
	data->pdata->ir_wake_en(data->pdata,1);
	gpio_set_value(data->pdata->irda_poweron, 1);

	if (IS_ERR(ir_remocon_dev)) {
		pr_err("Failed to create ir_remocon_dev device\n");
		goto err_fw_update_fail;
	}

	if (device_create_file(ir_remocon_dev, &dev_attr_ir_send) < 0) {
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_ir_send.attr.name);
		goto err_del_dev;
	}

	if (device_create_file(ir_remocon_dev, &dev_attr_ir_send_result) < 0) {
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_ir_send.attr.name);
		goto err_del_dev_file_send;
	}

	if (device_create_file(ir_remocon_dev, &dev_attr_check_ir) < 0) {
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_check_ir.attr.name);
		goto err_del_dev_file_send_result;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = ir_remocon_early_suspend;
	data->early_suspend.resume = ir_remocon_late_resume;
	register_early_suspend(&data->early_suspend);
#endif
	gpio_tlmm_config(GPIO_CFG(data->pdata->irda_irq_gpio,  0, GPIO_CFG_INPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	return 0;

err_del_dev_file_send_result:
	device_remove_file(ir_remocon_dev, &dev_attr_ir_send_result);

err_del_dev_file_send:
	device_remove_file(ir_remocon_dev, &dev_attr_ir_send);

err_del_dev:
	device_destroy(sec_class,ir_remocon_dev->devt);
	
err_fw_update_fail:
	regulator_put(vled_ic);
err_free_mem:
	kfree(data);
	return error;
}

#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
static int ir_remocon_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ir_remocon_data *data = i2c_get_clientdata(client);
	int ret = 0;

	gpio_set_value(data->pdata->irda_poweron, 0);

	data->on_off = 0;
	data->pdata->ir_wake_en(data->pdata,0);

	return ret;
}

static int ir_remocon_resume(struct device *dev)
{
	return 0;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ir_remocon_early_suspend(struct early_suspend *h)
{
	struct ir_remocon_data *data;
	data = container_of(h, struct ir_remocon_data, early_suspend);
	ir_remocon_suspend(&data->client->dev);
}

static void ir_remocon_late_resume(struct early_suspend *h)
{
	struct ir_remocon_data *data;
	data = container_of(h, struct ir_remocon_data, early_suspend);
	ir_remocon_resume(&data->client->dev);
}
#endif

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static const struct dev_pm_ops ir_remocon_pm_ops = {
	.suspend	= ir_remocon_suspend,
	.resume		= ir_remocon_resume,
};
#endif

static int __devexit ir_remocon_remove(struct i2c_client *client)
{
	struct ir_remocon_data *data = i2c_get_clientdata(client);

	mutex_destroy(&data->mutex);
	i2c_set_clientdata(client, NULL);
        regulator_disable(vled_ic);
        regulator_put(vled_ic);
	kfree(data);
	return 0;
}

static const struct i2c_device_id mc96_id[] = {
	{"mc96", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, mc96_id);

static struct of_device_id mc96_i2c_match_table[] = {
	{ .compatible = "mc96fr332,i2c",},
	{},
};
MODULE_DEVICE_TABLE(of, mc96_i2c_match_table);

static struct i2c_driver mc96_i2c_driver = {
	.driver = {
		.name = "mc96",
		.owner = THIS_MODULE,
		.of_match_table = mc96_i2c_match_table,
	},
	.probe = irda_remocon_probe,
	.remove = __devexit_p(ir_remocon_remove),
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
	//.pm	= &ir_remocon_pm_ops,
#endif

	.id_table = mc96_id,
};

static int __init ir_remocon_init(void)
{
	return i2c_add_driver(&mc96_i2c_driver);
}
module_init(ir_remocon_init);

static void __exit ir_remocon_exit(void)
{
	i2c_del_driver(&mc96_i2c_driver);
}
module_exit(ir_remocon_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SEC IR remote controller");
