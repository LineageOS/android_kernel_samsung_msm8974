/*
 * w1-gpio_msm - MSM GPIO w1 bus master driver
 *
 * Based on w1-gpio driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/w1-gpio-msm.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/delay.h>

#include <linux/input.h>

#include "../w1.h"
#include "../w1_int.h"

#include "../../gpio/gpio-msm-common.h"

#define MAX_SAMPLE	30

#define gpio_direction_input(gpio) __msm_gpio_set_config_direction_no_log(gpio, 1, 0)
#define gpio_direction_output(gpio, val) __msm_gpio_set_config_direction_no_log(gpio, 0, val)
#define gpio_set_value_msm(gpio, val) __msm_gpio_set_inout_no_log(gpio, val)
#define gpio_get_value_msm(gpio) __msm_gpio_get_inout_no_log(gpio)

static DEFINE_SPINLOCK(w1_gpio_msm_lock);
#if defined(CONFIG_W1_FAST_CHECK)
bool w1_is_resumed;
#endif

static int w1_delay_parm = 1;
static void w1_delay(unsigned long tm)
{
	udelay(tm * w1_delay_parm);
}

static void w1_gpio_write_bit(void *data, u8 bit);
static u8 w1_gpio_read_bit(void *data);

static u8 w1_gpio_set_pullup(void *data, int duration)
{
	struct w1_gpio_msm_platform_data *pdata = data;
	unsigned long irq_flags;

	if (pdata->enable_external_pullup)
		pdata->enable_external_pullup(1);
	else {
		spin_lock_irqsave(&w1_gpio_msm_lock, irq_flags);
		gpio_direction_output(pdata->pin, 1);
		gpio_set_value_msm(pdata->pin, 1);
		spin_unlock_irqrestore(&w1_gpio_msm_lock, irq_flags);
	}

	return 0;
}

static void w1_gpio_write_bit_dir(void *data, u8 bit)
{
	struct w1_gpio_msm_platform_data *pdata = data;

	if (bit)
		gpio_direction_input(pdata->pin);
	else
		gpio_direction_output(pdata->pin, 0);
}

static void w1_gpio_write_bit_val(void *data, u8 bit)
{
	struct w1_gpio_msm_platform_data *pdata = data;

	gpio_set_value_msm(pdata->pin, bit);
}

static u8 w1_gpio_read_bit_val(void *data)
{
	struct w1_gpio_msm_platform_data *pdata = data;
	int ret;

	ret = gpio_get_value_msm(pdata->pin) ? 1 : 0;

	return ret;
}

/**
 * Generates a write-0 or write-1 cycle and samples the level.
 */
static u8 w1_gpio_touch_bit(void *data, u8 bit)
{
	if (bit) {
		return w1_gpio_read_bit(data);
	} else {
		w1_gpio_write_bit(data, 0);
		return 0;
	}
}

/**
 * Generates a write-0 or write-1 cycle.
 * Only call if dev->bus_master->touch_bit is NULL
 */
static void w1_gpio_write_bit(void *data, u8 bit)
{
	struct w1_gpio_msm_platform_data *pdata = data;
	void	(*write_bit)(void *, u8);
	unsigned long irq_flags;

	if (pdata->is_open_drain) {
		write_bit = w1_gpio_write_bit_val;
	} else {
		write_bit = w1_gpio_write_bit_dir;
	}

	spin_lock_irqsave(&w1_gpio_msm_lock, irq_flags);

	if (bit) {
		write_bit(data, 0);
		write_bit(data, 1);

		(pdata->slave_speed == 0)? w1_delay(64) : w1_delay(10);
	} else {
		write_bit(data, 0);
		(pdata->slave_speed == 0)? w1_delay(60) : w1_delay(8);
		write_bit(data, 1);
		(pdata->slave_speed == 0)? w1_delay(10) : w1_delay(5);
	}
	spin_unlock_irqrestore(&w1_gpio_msm_lock, irq_flags);

}

/**
 * Pre-write operation, currently only supporting strong pullups.
 * Program the hardware for a strong pullup, if one has been requested and
 * the hardware supports it.
 *
 */
static void w1_gpio_pre_write(void *data)
{
	w1_gpio_set_pullup(data, 0);
}

/**
 * Post-write operation, currently only supporting strong pullups.
 * If a strong pullup was requested, clear it if the hardware supports
 * them, or execute the delay otherwise, in either case clear the request.
 *
 */
static void w1_gpio_post_write(void *data)
{
	w1_gpio_set_pullup(data, 0);
}

/**
 * Writes 8 bits.
 *
 * @param data    the master device data
 * @param byte    the byte to write
 */
static void w1_gpio_write_8(void *data, u8 byte)
{
	int i;

	w1_gpio_pre_write(data);
	for (i = 0; i < 8; ++i) {
		if (i == 7)
			w1_gpio_pre_write(data);
//		w1_gpio_write_bit(data, (byte >> i) & 0x1);
		w1_gpio_touch_bit(data, (byte >> i) & 0x1);
	}
	w1_gpio_post_write(data);
}

/**
 * Generates a write-1 cycle and samples the level.
 * Only call if dev->bus_master->touch_bit is NULL
 */
static u8 w1_gpio_read_bit(void *data)
{
	struct w1_gpio_msm_platform_data *pdata = data;
	int result;
	void	(*write_bit)(void *, u8);
	unsigned long irq_flags;

	if (pdata->is_open_drain) {
		write_bit = w1_gpio_write_bit_val;
	} else {
		write_bit = w1_gpio_write_bit_dir;
	}

	spin_lock_irqsave(&w1_gpio_msm_lock, irq_flags);

	/* sample timing is critical here */
	write_bit(data, 0);
	write_bit(data, 1);

	result = w1_gpio_read_bit_val(data);

	(pdata->slave_speed == 0)? w1_delay(55) : w1_delay(8);

	spin_unlock_irqrestore(&w1_gpio_msm_lock, irq_flags);

	return result & 0x1;
}

/**
 * Does a triplet - used for searching ROM addresses.
 * Return bits:
 *  bit 0 = id_bit
 *  bit 1 = comp_bit
 *  bit 2 = dir_taken
 * If both bits 0 & 1 are set, the search should be restarted.
 *
 * @param data     the master device data
 * @param bdir    the bit to write if both id_bit and comp_bit are 0
 * @return        bit fields - see above
 */
static u8 w1_gpio_triplet(void *data, u8 bdir)
{
	u8 id_bit   = w1_gpio_touch_bit(data, 1);
	u8 comp_bit = w1_gpio_touch_bit(data, 1);
	u8 retval;

	if (id_bit && comp_bit)
		return 0x03;  /* error */

	if (!id_bit && !comp_bit) {
		/* Both bits are valid, take the direction given */
		retval = bdir ? 0x04 : 0;
	} else {
		/* Only one bit is valid, take that direction */
		bdir = id_bit;
		retval = id_bit ? 0x05 : 0x02;
	}

	w1_gpio_touch_bit(data, bdir);
	return retval;
}

/**
 * Reads 8 bits.
 *
 * @param data     the master device data
 * @return        the byte read
 */
static u8 w1_gpio_read_8(void *data)
{
	int i;
	u8 res = 0;

	for (i = 0; i < 8; ++i)
		res |= (w1_gpio_touch_bit(data,1) << i);

	return res;
}

/**
 * Writes a series of bytes.
 *
 * @param data     the master device data
 * @param buf     pointer to the data to write
 * @param len     the number of bytes to write
 */
static void w1_gpio_write_block(void *data, const u8 *buf, int len)
{
	int i;

	w1_gpio_pre_write(data);
	for (i = 0; i < len; ++i)
		w1_gpio_write_8(data, buf[i]); /* calls w1_pre_write */
	w1_gpio_post_write(data);
}

/**
 * Reads a series of bytes.
 *
 * @param data     the master device data
 * @param buf     pointer to the buffer to fill
 * @param len     the number of bytes to read
 * @return        the number of bytes read
 */
static u8 w1_gpio_read_block(void *data, u8 *buf, int len)
{
	int i;
	u8 ret;

	for (i = 0; i < len; ++i)
		buf[i] = w1_gpio_read_8(data);
	ret = len;

	return ret;
}

/**
 * Issues a reset bus sequence.
 *
 * @param  dev The bus master pointer
 * @return     0=Device present, 1=No device present or error
 */
static u8 w1_gpio_reset_bus(void *data)
{
	int result = 1,i;
	struct w1_gpio_msm_platform_data *pdata = data;
	void	(*write_bit)(void *, u8);
	unsigned long irq_flags;
	int temp_read[MAX_SAMPLE]={'1',};
	int loop_cnt = 3;

	if (pdata->is_open_drain) {
		write_bit = w1_gpio_write_bit_val;
	} else {
		write_bit = w1_gpio_write_bit_dir;
	}

	spin_lock_irqsave(&w1_gpio_msm_lock, irq_flags);

	while (loop_cnt > 0) {
		write_bit(data, 0);
			/* minimum 48, max 80 us(In DS Documnet)
			 * be nice and sleep, except 18b20 spec lists 960us maximum,
			 * so until we can sleep with microsecond accuracy, spin.
			 * Feel free to come up with some other way to give up the
			 * cpu for such a short amount of time AND get it back in
			 * the maximum amount of time.
			 */
		(pdata->slave_speed == 0)? __const_udelay(500*UDELAY_MULT) : __const_udelay(50*UDELAY_MULT);
		write_bit(data, 1);

		(pdata->slave_speed == 0)? __const_udelay(60*UDELAY_MULT) : __const_udelay(1*UDELAY_MULT);

		for(i=0;i<MAX_SAMPLE;i++)
			temp_read[i] = gpio_get_value_msm(pdata->pin);

		for(i=0;i<MAX_SAMPLE;i++)
			result &= temp_read[i];

		/* minmum 70 (above) + 410 = 480 us
		 * There aren't any timing requirements between a reset and
		 * the following transactions.  Sleeping is safe here.
		 */
		/* w1_delay(410); min required time */
		(pdata->slave_speed == 0)? msleep(1) : __const_udelay(40*UDELAY_MULT);

		if (result)
			loop_cnt--;
		else
			break;
	}

	spin_unlock_irqrestore(&w1_gpio_msm_lock, irq_flags);

	return result;
}

static int hall_open(struct input_dev *input)
{
	return 0;
}

static void hall_close(struct input_dev *input)
{
}

static struct of_device_id w1_gpio_msm_dt_ids[] = {
        { .compatible = "w1-gpio-msm", },
        {}
};
MODULE_DEVICE_TABLE(of, w1_gpio_msm_dt_ids);

static int w1_gpio_msm_probe_dt(struct platform_device *pdev)
{
	struct w1_gpio_msm_platform_data *pdata = pdev->dev.platform_data;
	struct device_node *np = pdev->dev.of_node;

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
    	return -ENOMEM;

	if (of_get_property(np, "linux,open-drain", NULL))
	    pdata->is_open_drain = 1;

	if (of_get_property(np, "linux,slave-speed", NULL))
	    pdata->slave_speed= 1;

	pdata->pin = of_get_gpio(np, 0);
	pdata->ext_pullup_enable_pin = of_get_gpio(np, 1);
	pdev->dev.platform_data = pdata;

	return 0;
}

static int w1_gpio_msm_probe(struct platform_device *pdev)
{
	struct w1_bus_master *master;
	struct w1_gpio_msm_platform_data *pdata;
	struct input_dev *input;
	int err;

	printk(KERN_ERR "\nw1_gpio_msm_probe start\n");

	if (of_have_populated_dt()) {
		err = w1_gpio_msm_probe_dt(pdev);
		if (err < 0) {
			dev_err(&pdev->dev, "Failed to parse DT\n");
			return err;
		}
	}

	pdata = pdev->dev.platform_data;

	if (!pdata) {
		dev_err(&pdev->dev, "No configuration data\n");
		return -ENXIO;
	}

	master = kzalloc(sizeof(struct w1_bus_master), GFP_KERNEL);
	if (!master) {
		dev_err(&pdev->dev, "Out of memory\n");
		return -ENOMEM;
	}

	/* add for sending uevent */
	input = input_allocate_device();
	if (!input) {
		err = -ENODEV;
		goto free_master;
		/* need to change*/
	}
	master->input = input;

	input_set_drvdata(input, master);

	input->name = "w1";
	input->phys = "w1";
	input->dev.parent = &pdev->dev;

	input->evbit[0] |= BIT_MASK(EV_SW);
	input_set_capability(input, EV_SW, SW_W1);

	input->open = hall_open;
	input->close = hall_close;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	err = input_register_device(input);
	if(err) {
		dev_err(&pdev->dev, "input_register_device failed!\n");
		goto free_input;
	}

	spin_lock_init(&w1_gpio_msm_lock);

	err = gpio_request(pdata->pin, "w1");
	if (err) {
		dev_err(&pdev->dev, "gpio_request (pin) failed\n");
		goto free_input;
	}

	if (gpio_is_valid(pdata->ext_pullup_enable_pin)) {
		err = gpio_request_one(pdata->ext_pullup_enable_pin,
			  GPIOF_INIT_LOW, "w1 pullup");
		if (err < 0) {
			dev_err(&pdev->dev, "gpio_request_one "
				   "(ext_pullup_enable_pin) failed\n");
			goto free_gpio;
		}
	}

	master->data = pdata;
	master->read_bit = w1_gpio_read_bit_val;
	master->touch_bit = w1_gpio_touch_bit;
	master->read_byte = w1_gpio_read_8;
	master->write_byte = w1_gpio_write_8;
	master->read_block = w1_gpio_read_block;
	master->write_block = w1_gpio_write_block;
	master->triplet = w1_gpio_triplet;
	master->reset_bus =  w1_gpio_reset_bus;
	master->set_pullup = w1_gpio_set_pullup;

	if (pdata->is_open_drain) {
		gpio_direction_output(pdata->pin, 1);
		master->write_bit = w1_gpio_write_bit_val;
	} else {
		gpio_direction_input(pdata->pin);
		master->write_bit = w1_gpio_write_bit_dir;
	}

	err = w1_add_master_device(master);
	if (err) {
		dev_err(&pdev->dev, "w1_add_master device failed\n");
		goto free_gpio_ext_pu;
	}

	if (pdata->enable_external_pullup)
		pdata->enable_external_pullup(1);

	if (gpio_is_valid(pdata->ext_pullup_enable_pin))
		gpio_set_value_msm(pdata->ext_pullup_enable_pin, 1);

	platform_set_drvdata(pdev, master);

	return 0;

free_gpio_ext_pu:
	if (gpio_is_valid(pdata->ext_pullup_enable_pin))
		gpio_free(pdata->ext_pullup_enable_pin);
free_gpio:
	gpio_free(pdata->pin);
free_input:
	kfree(input);
free_master:
	kfree(master);

	return err;
}

static int w1_gpio_msm_remove(struct platform_device *pdev)
{
	struct w1_bus_master *master = platform_get_drvdata(pdev);
	struct w1_gpio_msm_platform_data *pdata = pdev->dev.platform_data;

	if (pdata->enable_external_pullup)
		pdata->enable_external_pullup(0);

	if (gpio_is_valid(pdata->ext_pullup_enable_pin))
		gpio_set_value_msm(pdata->ext_pullup_enable_pin, 0);

	w1_remove_master_device(master);
	gpio_free(pdata->pin);
	kfree(master);

	return 0;
}

#ifdef CONFIG_PM

static int w1_gpio_msm_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct w1_gpio_msm_platform_data *pdata = pdev->dev.platform_data;

	if (pdata->enable_external_pullup)
		pdata->enable_external_pullup(0);

	gpio_tlmm_config(GPIO_CFG(pdata->pin, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_direction_input(pdata->pin);
#ifdef CONFIG_W1_WORKQUEUE
	cancel_delayed_work_sync(&w1_gdev->w1_dwork);
#endif
	return 0;
}

static int w1_gpio_msm_resume(struct platform_device *pdev)
{
	struct w1_gpio_msm_platform_data *pdata = pdev->dev.platform_data;

	if (pdata->enable_external_pullup)
		pdata->enable_external_pullup(1);

	gpio_tlmm_config(GPIO_CFG(pdata->pin, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_direction_output(pdata->pin, 1);

#if defined(CONFIG_W1_FAST_CHECK)
	w1_is_resumed = true;
#endif
#ifdef CONFIG_W1_WORKQUEUE
	schedule_delayed_work(&w1_gdev->w1_dwork, HZ * 2);
#endif
	return 0;
}

#else
#define w1_gpio_msm_suspend	NULL
#define w1_gpio_msm_resume	NULL
#endif

static struct platform_driver w1_gpio_msm_driver = {
	.driver = {
		.name	= "w1-gpio-msm",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(w1_gpio_msm_dt_ids),
	},
	.probe = w1_gpio_msm_probe,
	.remove	= w1_gpio_msm_remove,
	.suspend = w1_gpio_msm_suspend,
	.resume = w1_gpio_msm_resume,
};

static int __init w1_gpio_msm_init(void)
{
	return platform_driver_probe(&w1_gpio_msm_driver, w1_gpio_msm_probe);
}

static void __exit w1_gpio_msm_exit(void)
{
	platform_driver_unregister(&w1_gpio_msm_driver);
}

late_initcall(w1_gpio_msm_init);
module_exit(w1_gpio_msm_exit);

MODULE_DESCRIPTION("MSM GPIO w1 bus master driver");
MODULE_AUTHOR("clark.kim@maximintegrated.com");
MODULE_LICENSE("GPL");
