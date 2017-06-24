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

#include <mach/gpio.h>
#include <linux/ir_remote_con_mc96.h>
#define MAX_SIZE 2048
#define MC96_READ_LENGTH	8
#define DUMMY 0xffff

#define MC96FR116C_0x101	0x101
#define MC96FR332A_0x201	0x201

#define MC96FR116C_0x103	0x103
#define MC96FR332A_0x202	0x202

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

#ifdef CONFIG_IR_REMOCON_MC96
static void irda_wake_en(struct mc96_platform_data *pdata, bool onoff)
{
	/*Need to add parser before this*/
        gpio_direction_output(pdata->irda_wake_en, onoff);
        printk(KERN_ERR "%s: irda_wake_en : %d\n", __func__, onoff);
}

static int vled_ic_onoff;
static struct regulator *vled_ic;

static int irda_vdd_onoff(bool onoff)
{
	int ret = 0;
        if (onoff) {
		ret = regulator_set_voltage(vled_ic,1900000,1900000);
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
#endif

static int irda_fw_update(struct ir_remocon_data *ir_data)
{
	struct ir_remocon_data *data = ir_data;
	struct i2c_client *client = data->client;
	int i, k, ret, ret2, checksum, checksum2, frame_count;
	u8 buf_ir_test[8];
	const u8 *IRDA_fw;
	ret = 0;

	ret = irda_vdd_onoff(0);
	if (ret) {
		pr_err("%s Regulator setting failed\n", __func__);
		goto err_regulator;
	}
	data->pdata->ir_wake_en(data->pdata, 0);
	msleep(100);
	ret = irda_vdd_onoff(1);
	if (ret) {
		pr_err("%s Regulator setting failed\n", __func__);
		goto err_regulator;
	}

	data->pdata->ir_wake_en(data->pdata,1);
	gpio_tlmm_config(GPIO_CFG(data->pdata->irda_irq_gpio,  0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	msleep(70);

	ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
	if (ret < 0) {
		printk(KERN_ERR "%s: err %d\n", __func__, ret);
		ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
		if (ret < 0) {
			printk(KERN_INFO "%s: broken FW!\n", __func__);
			goto err_bootmode;
		}
	}
	
#ifdef DEBUG
	print_hex_dump(KERN_CRIT, "IRDA Master Rx: ", 16, 1,
				DUMP_PREFIX_ADDRESS, buf_ir_test, 8, 1);
#endif
	ret = buf_ir_test[2] << 8 | buf_ir_test[3];

	if ((ret == MC96FR116C_0x103) || (ret == MC96FR332A_0x202)) {
		data->pdata->ir_wake_en(data->pdata,0);
		ret = irda_vdd_onoff(0);
		if (ret) {
			pr_err("%s Regulator setting failed\n", __func__);
			goto err_regulator;
		}

		data->on_off = 0;
		msleep(100);
		download_pass = 1;
		return 0;
	}

	if (ret == MC96FR116C_0x101) {
		IRDA_fw     = IRDA_binary_103;
		frame_count = FRAME_COUNT_103;
		printk(KERN_ERR "%s: chip : %04x, bin : %04x, need update!\n",
					__func__, ret, MC96FR116C_0x103);
	}
	else if (ret == MC96FR332A_0x201) {
		IRDA_fw     = IRDA_binary_202;
		frame_count = FRAME_COUNT_202;
		printk(KERN_ERR "%s: chip : %04x, bin : %04x, need update!\n",
					__func__, ret, MC96FR332A_0x202);
	}
	else
		goto err_bootmode;	

	printk(KERN_ERR "irda frame count = %d\n", frame_count);

	ret = irda_vdd_onoff(0);
	if (ret) {
		pr_err("%s Regulator setting failed\n", __func__);
		goto err_regulator;
	}

	data->pdata->ir_wake_en(data->pdata, 0);
	msleep(100);
	ret = irda_vdd_onoff(1);
	if (ret) {
		pr_err("%s Regulator setting failed\n", __func__);
		goto err_regulator;
	}

	msleep(70);

	ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
	if (ret < 0)
		printk(KERN_ERR " %s: err %d\n", __func__, ret);

#ifdef DEBUG
	print_hex_dump(KERN_CRIT, "IRDA Master Rx: ", 16, 1,
			DUMP_PREFIX_ADDRESS, buf_ir_test, 8, 1);
#endif

	ret = buf_ir_test[6] << 8 | buf_ir_test[7];

	checksum = 0;

	for (k = 0; k < 6; k++)
		checksum += buf_ir_test[k];

	if (ret == checksum)
		printk(KERN_INFO "%s: boot mode, FW download start! ret=%04x\n",
							__func__, ret);
	else {
		printk(KERN_ERR "ABOV IC bootcode broken\n");
		goto err_bootmode;
	}

	msleep(30);

	for (i = 0; i < frame_count; i++) {
		if (i == frame_count-1) {
			ret = i2c_master_send(client,
					&IRDA_fw[i * 70], 6);
			if (ret < 0)
				goto err_update;
		} else {
			ret = i2c_master_send(client,
					&IRDA_fw[i * 70], 70);
			if (ret < 0)
				goto err_update;
		}
		msleep(30);
	}

	ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
	if (ret < 0)
		printk(KERN_ERR "5. %s: err %d\n", __func__, ret);

#ifdef DEBUG
	print_hex_dump(KERN_CRIT, "IRDA Master Rx: ", 16, 1,
			DUMP_PREFIX_ADDRESS, buf_ir_test, 8, 1);
#endif

	ret = buf_ir_test[6] << 8 | buf_ir_test[7];
	checksum = 0;
	for (k = 0; k < 6; k++)
			checksum += buf_ir_test[k];

		msleep(20);

		ret2 = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
	
		if (ret2 < 0)
			printk(KERN_ERR "6. %s: err %d\n", __func__, ret2);

		ret2 = buf_ir_test[6] << 8 | buf_ir_test[7];
		for (k = 0; k < 6; k++)
			checksum2 += buf_ir_test[k];

		if (ret == checksum) {
			printk(KERN_INFO "1. %s: boot down complete\n",
				__func__);
			download_pass = 1;
		} else if (ret2 == checksum2) {
			printk(KERN_INFO "2. %s: boot down complete\n",
				__func__);
			download_pass = 1;
		} else {
			printk(KERN_ERR "FW Checksum fail\n");
			goto err_bootmode;
		}

		ret = irda_vdd_onoff(0);
		if (ret) {
			pr_err("%s Regulator setting failed\n", __func__);
			goto err_regulator;
		}

		msleep(100);
		ret = irda_vdd_onoff(1);
		if (ret) {
			pr_err("%s Regulator setting failed\n", __func__);
			goto err_regulator;
		}

		data->pdata->ir_wake_en(data->pdata, 1);
		msleep(70);

		ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
		ret = buf_ir_test[2] << 8 | buf_ir_test[3];
		printk(KERN_INFO "7. %s: user mode : Upgrade FW_version : %04x\n",
						__func__, ret);

#ifdef DEBUG
	print_hex_dump(KERN_CRIT, "IRDA Master Rx: ", 16, 1,
				DUMP_PREFIX_ADDRESS, buf_ir_test, 8, 1);
#endif

		data->pdata->ir_wake_en(data->pdata,0);
		ret = irda_vdd_onoff(0);
		if (ret) {
			pr_err("%s Regulator setting failed\n", __func__);
			goto err_regulator;
		}

		data->on_off = 0;

	if ((ret == MC96FR116C_0x103) || (ret == MC96FR332A_0x202))
		download_pass = 1;

	return 0;
err_update:
	printk(KERN_ERR "%s: update fail! count : %x, ret = %x\n",
							__func__, i, ret);
	return ret;
err_bootmode:
	printk(KERN_ERR "%s: update fail, checksum = %x ret = %x\n",
					__func__, checksum, ret);
	data->pdata->ir_wake_en(data->pdata,0);
	ret = irda_vdd_onoff(0);
	if (ret) {
		pr_err("%s Regulator setting failed\n", __func__);
		goto err_regulator;
	}

	data->on_off = 0;
err_regulator:
	return ret;
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
	ret = irda_vdd_onoff(1);
	if (ret) {
		pr_err("%s Regulator setting failed\n", __func__);
		goto err_regulator;
	}

	data->pdata->ir_wake_en(data->pdata,1);
	msleep(60);
	ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	printk(KERN_INFO "%s: buf_ir dev_id: 0x%02x, 0x%02x\n", __func__,
			buf_ir_test[2], buf_ir_test[3]);
	ret = data->dev_id = (buf_ir_test[2] << 8 | buf_ir_test[3]);

	data->pdata->ir_wake_en(data->pdata,0);
	ret = irda_vdd_onoff(0);
	if (ret) {
		pr_err("%s Regulator setting failed\n", __func__);
		goto err_regulator;
	}

	data->on_off = 0;
err_regulator:
	return ret;
}

static void irda_remocon_work(struct ir_remocon_data *ir_data, int count)
{

	struct ir_remocon_data *data = ir_data;
	struct i2c_client *client = data->client;

	int buf_size = count+2;
	int ret;
	int sleep_timing;
	int end_data;
	int emission_time;
	int ack_pin_onoff;

	if (count_number >= 100)
		count_number = 0;

	count_number++;

	gpio_tlmm_config(GPIO_CFG(ir_data->pdata->irda_irq_gpio,  0, GPIO_CFG_INPUT,
		GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	printk(KERN_INFO "%s: total buf_size: %d\n", __func__, buf_size);

	irda_add_checksum_length(data, count);

	mutex_lock(&data->mutex);

	ret = i2c_master_send(client, data->signal, buf_size);
	if (ret < 0) {
		dev_err(&client->dev, "%s: err1 %d\n", __func__, ret);
		ret = i2c_master_send(client, data->signal, buf_size);
		if (ret < 0)
			dev_err(&client->dev, "%s: err2 %d\n", __func__, ret);
	}

	mdelay(10);

	ack_pin_onoff = 0;
	if (gpio_get_value(data->pdata->irda_irq_gpio)) {
		printk(KERN_INFO "%s : %d Checksum NG!\n",
			__func__, count_number);
		ack_pin_onoff = 1;
	} else {
		printk(KERN_INFO "%s : %d Checksum OK!\n",
			__func__, count_number);
		ack_pin_onoff = 2;
	}
	ack_number = ack_pin_onoff;

	mutex_unlock(&data->mutex);

#if 0
	for (i = 0; i < buf_size; i++) {
		printk(KERN_INFO "%s: data[%d] : 0x%02x\n", __func__, i,
					data->signal[i]);
	
	}
#endif
	data->count = 2;

	end_data = data->signal[count-2] << 8 | data->signal[count-1];
	emission_time = \
		(1000 * (data->ir_sum - end_data) / (data->ir_freq)) + 10;
	sleep_timing = emission_time - 130;
	if (sleep_timing > 0)
		msleep(sleep_timing);
/*
	printk(KERN_INFO "%s: sleep_timing = %d\n", __func__, sleep_timing);
*/
	emission_time = \
		(1000 * (data->ir_sum) / (data->ir_freq)) + 50;
	if (emission_time > 0)
		msleep(emission_time);
		printk(KERN_INFO "%s: emission_time = %d\n",
					__func__, emission_time);

	if (gpio_get_value(data->pdata->irda_irq_gpio)) {
		printk(KERN_INFO "%s : %d Sending IR OK!\n",
				__func__, count_number);
		ack_pin_onoff = 4;
	} else {
		printk(KERN_INFO "%s : %d Sending IR NG!\n",
				__func__, count_number);
		ack_pin_onoff = 2;
	}

	ack_number += ack_pin_onoff;
#ifndef USE_STOP_MODE
	data->on_off = 0;
	data->pdata->ir_wake_en(data->pdata,0);
	irda_vdd_onoff(0);
	gpio_set_value(data->pdata->irda_led_en, 0);
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

	ret = 0;
	gpio_set_value(data->pdata->irda_led_en, 1);
	for (i = 0; i < MAX_SIZE; i++) {
		if (sscanf(buf++, "%u", &_data) == 1) {
			if (_data == 0 || buf == '\0')
				break;

			if (data->count == 2) {
				data->ir_freq = _data;
				if (data->on_off) {
					data->pdata->ir_wake_en(data->pdata,0);
					udelay(200);
					data->pdata->ir_wake_en(data->pdata,1);
					msleep(30);
				} else {
					ret = irda_vdd_onoff(1); 
					if (ret) {
						pr_err("%s regulaor disable failed\n",
								__func__);
						return ret;
					}
					data->pdata->ir_wake_en(data->pdata,1);

					msleep(80);
					data->on_off = 1;
				}
				data->signal[2] = _data >> 16;
				data->signal[3] = (_data >> 8) & 0xFF;
				data->signal[4] = _data & 0xFF;
				data->count += 3;
			} else {
				data->ir_sum += _data;
				count = data->count;
				data->signal[count] = _data >> 8;
				data->signal[count+1] = _data & 0xFF;
				data->count += 2;
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
        pdata->irda_led_en = of_get_named_gpio_flags(np, "mc96fr332,irda_led_en",
                               0, &pdata->led_en_flags);
	if (pdata->irda_led_en < 0) {
		pr_err("%s failed to get irda_led_en\n", __func__);
		return pdata->irda_led_en;
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
        pr_info("%s: irq-gpio:%u led_en:%u wake_up:%u irda-scl:%u irda-sda:%u \n", __func__,
			 pdata->irda_irq_gpio,pdata->irda_led_en,pdata->irda_wake_en,pdata->irda_scl_gpio,pdata->irda_sda_gpio);
	
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
		pdata->ir_wake_en = irda_wake_en;
		if(gpio_request(pdata->irda_wake_en, "IRDA Wakeup EN Pin"))
			pr_err("%s IRDA Wakeup EN GPIO Request failed\n",__func__);

		gpio_tlmm_config(GPIO_CFG(pdata->irda_wake_en,  0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);


		gpio_tlmm_config(GPIO_CFG(pdata->irda_led_en,  0, GPIO_CFG_OUTPUT,
                        GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

		gpio_request(pdata->irda_led_en, "irda_led_en");
		gpio_direction_output(pdata->irda_led_en, 1);

		if (gpio_request(pdata->irda_irq_gpio, "ira_irq"))
			pr_err("%s IRDA LED IRQ  GPIO Request failed\n",__func__);
		gpio_direction_input(pdata->irda_irq_gpio);
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
	data->on_off = 0;


	vled_ic = regulator_get(&client->dev, "vled_ic_1.9v");
       	if (IS_ERR(vled_ic)) {
       		pr_err("%s could not get regulator vled_ic_1.9v\n",__func__);
		error = -EBUSY;
		goto err_free_mem;
       }

	i2c_set_clientdata(client, data);

	for (i = 0; i < 2; i++) {
		if (download_pass == 1)
			break;
		irda_fw_update(data);
	}
	if (download_pass != 1)
		goto err_fw_update_fail;

	gpio_set_value(pdata->irda_led_en, 0);
//	irda_read_device_info(data);

	ir_remocon_dev = device_create(sec_class, NULL, 0, data, "sec_ir");

	if (IS_ERR(ir_remocon_dev))
		pr_err("Failed to create ir_remocon_dev device\n");

	if (device_create_file(ir_remocon_dev, &dev_attr_ir_send) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_ir_send.attr.name);

	if (device_create_file(ir_remocon_dev, &dev_attr_ir_send_result) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_ir_send.attr.name);

	if (device_create_file(ir_remocon_dev, &dev_attr_check_ir) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_check_ir.attr.name);

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = ir_remocon_early_suspend;
	data->early_suspend.resume = ir_remocon_late_resume;
	register_early_suspend(&data->early_suspend);
#endif
	gpio_tlmm_config(GPIO_CFG(data->pdata->irda_irq_gpio,  0, GPIO_CFG_INPUT,
		GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	return 0;

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

	ret = irda_vdd_onoff(0);
	if (ret)
		pr_err("%s Regulator setting failed\n", __func__);
	
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
