/*
 * es705-platform.c  --  Audience eS705 platform dependent functions
 *
 * Copyright 2011 Audience, Inc.
 *
 * Author: Genisim Tsilker <gtsilker@audience.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define SAMSUNG_ES705_FEATURE

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>

#if defined(SAMSUNG_ES705_FEATURE)
#include <linux/input.h>
#endif

#include <linux/esxxx.h>
#include "es705.h"
#include "es705-platform.h"

void es705_gpio_reset(struct es705_priv *es705)
{
	dev_dbg(es705->dev, "%s(): GPIO reset\n", __func__);
	gpio_set_value(es705->pdata->reset_gpio, 0);
	/* Wait 1 ms then pull Reset signal in High */
	usleep_range(1000, 1000);
	gpio_set_value(es705->pdata->reset_gpio, 1);
	/* Wait 10 ms then */
	usleep_range(10000, 10000);
	/* eSxxx is READY */
}

int es705_gpio_init(struct es705_priv *es705)
{
	int rc = 0;
#ifdef CONFIG_SND_SOC_ES704_TEMP
	static int gpio_initialized = 0;
	if (gpio_initialized)
		return rc;
#endif
	if (es705->pdata->reset_gpio != -1) {
		rc = gpio_request(es705->pdata->reset_gpio, "es705_reset");
		if (rc < 0) {
			dev_err(es705->dev, "%s(): es705_reset request failed",
				__func__);
			goto reset_gpio_request_error;
		}
		rc = gpio_direction_output(es705->pdata->reset_gpio, 0);
		if (rc < 0) {
			dev_err(es705->dev, "%s(): es705_reset direction failed",
				__func__);
			goto reset_gpio_direction_error;
		}
	} else {
		dev_warn(es705->dev, "%s(): es705_reset undefined\n",
				__func__);
	}

	if (es705->pdata->wakeup_gpio != -1) {
		rc = gpio_request(es705->pdata->wakeup_gpio, "es705_wakeup");
		if (rc < 0) {
			dev_err(es705->dev, "%s(): es705_wakeup request failed",
				__func__);
			goto wakeup_gpio_request_error;
		}
		rc = gpio_direction_output(es705->pdata->wakeup_gpio, 0);
		if (rc < 0) {
			dev_err(es705->dev, "%s(): es705_wakeup direction failed",
				__func__);
			goto wakeup_gpio_direction_error;
		}
	} else {
		dev_warn(es705->dev, "%s(): wakeup_gpio undefined\n",
				__func__);
	}
	/* under H/W rev 0.5 */
	if (es705->pdata->uart_gpio != -1) {
		rc = gpio_request(es705->pdata->uart_gpio, "es705_uart");
		if (rc < 0) {
			dev_err(es705->dev, "%s(): es705_uart request failed",
				__func__);
			goto uart_gpio_request_error;
		}
		rc = gpio_direction_output(es705->pdata->uart_gpio, 0);
		if (rc < 0) {
			dev_err(es705->dev, "%s(): es705_uart direction failed",
				__func__);
			goto uart_gpio_direction_error;
		}
	} else {
		dev_warn(es705->dev, "%s(): es705_uart undefined\n",
				__func__);
	}

	if (es705->pdata->gpiob_gpio) {
		rc = request_threaded_irq(es705->pdata->irq_base,
					  NULL,
					  es705_irq_event, IRQF_TRIGGER_RISING,
					  "es705-irq-event", es705);
		if (rc) {
			dev_err(es705->dev, "%s(): event request_irq() failed\n",
				__func__);
			goto event_irq_request_error;
		}
		rc = irq_set_irq_wake(es705->pdata->irq_base, 1);
		if (rc < 0) {
			dev_err(es705->dev, "%s(): set event irq wake failed\n",
				__func__);
			disable_irq(es705->pdata->irq_base);
			free_irq(es705->pdata->irq_base, es705);
			goto event_irq_wake_error;
		}
	}

#if defined(CONFIG_MACH_K3GDUOS_CTC)
	gpio_tlmm_config(GPIO_CFG(es705->pdata->uart_tx_gpio, 2, GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL, GPIO_CFG_8MA), 1);
	gpio_tlmm_config(GPIO_CFG(es705->pdata->uart_rx_gpio, 2, GPIO_CFG_INPUT,GPIO_CFG_NO_PULL, GPIO_CFG_8MA), 1);
#endif /* CONFIG_MACH_K3GDUOS_CTC */

#ifdef CONFIG_SND_SOC_ES704_TEMP
	gpio_initialized = 1;
#endif
	return rc;
uart_gpio_direction_error:
	gpio_free(es705->pdata->uart_gpio);
uart_gpio_request_error:
wakeup_gpio_direction_error:
	gpio_free(es705->pdata->wakeup_gpio);
wakeup_gpio_request_error:
reset_gpio_direction_error:
	gpio_free(es705->pdata->reset_gpio);
reset_gpio_request_error:
event_irq_wake_error:
event_irq_request_error:
	return rc;
}

#if defined(SAMSUNG_ES705_FEATURE)
extern unsigned int system_rev;
void es705_gpio_wakeup(struct es705_priv *es705)
{
	dev_info(es705->dev, "%s(): generate gpio wakeup falling edge\n",
		__func__);

	if (system_rev >= 4) {
		gpio_tlmm_config(GPIO_CFG(0, 0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_16MA), 1);
		gpio_direction_output(es705->pdata->wakeup_gpio, 1);
	} else 
		gpio_set_value(es705->pdata->wakeup_gpio, 1);

	usleep_range(1000,1000);

	if (system_rev >= 4) {
		gpio_direction_output(es705->pdata->wakeup_gpio, 0);
		gpio_direction_input(es705->pdata->wakeup_gpio);
		gpio_tlmm_config(GPIO_CFG(0, 2, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_16MA), 1);
	} else
		gpio_set_value(es705->pdata->wakeup_gpio, 0);
}

void es705_uart_pin_preset(struct es705_priv *es705)
{
	if((es705->pdata->uart_tx_gpio != -1)
		&& (es705->pdata->uart_rx_gpio != -1)) {
		gpio_tlmm_config(GPIO_CFG(es705->pdata->uart_tx_gpio, 2, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_16MA), 1);
		gpio_tlmm_config(GPIO_CFG(es705->pdata->uart_rx_gpio, 2, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_16MA), 1);
	}
}

void es705_uart_pin_postset(struct es705_priv *es705)
{
	if((es705->pdata->uart_tx_gpio != -1)
		&& (es705->pdata->uart_rx_gpio != -1)) {
		gpio_tlmm_config(GPIO_CFG(es705->pdata->uart_tx_gpio, 0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_16MA), 1);
		gpio_tlmm_config(GPIO_CFG(es705->pdata->uart_rx_gpio, 0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_16MA), 1);
	}
}

int es705_init_input_device(struct es705_priv *es705)
{
	int rc;
	es705->input = input_allocate_device();
	if (!es705->input) {
		rc = -ENOMEM;
		goto es705_init_input_device_exit;
	}

	es705->input->name = "es705 input";
	set_bit(EV_SYN, es705->input->evbit);
	set_bit(EV_KEY, es705->input->evbit);
	set_bit(KEY_VOICE_WAKEUP, es705->input->keybit);
	set_bit(KEY_VOICE_WAKEUP_LPSD, es705->input->keybit);

	rc = input_register_device(es705->input);
	if (rc < 0)
		input_free_device(es705->input);

es705_init_input_device_exit:
	return rc;
}

void es705_unregister_input_device(struct es705_priv *es705)
{
	input_unregister_device(es705->input);
}
#endif

void es705_vs_event(struct es705_priv *es705)
{
#if defined(SAMSUNG_ES705_FEATURE)
	unsigned int vs_event_type = 0;

	if (es705->voice_wakeup_enable == 1) /* Voice wakeup */
		vs_event_type = KEY_VOICE_WAKEUP;
	else if (es705->voice_wakeup_enable == 2) /* Voice wakeup LPSD */
		vs_event_type = KEY_VOICE_WAKEUP_LPSD;
	else {
		dev_info(es705->dev, "%s(): Invalid value(%d)\n", __func__, es705->voice_wakeup_enable);
		return;
	}

	dev_info(es705->dev, "%s(): Raise key event(%d)\n", __func__, vs_event_type);

	input_report_key(es705_priv.input, vs_event_type, 1);
	input_sync(es705_priv.input);
	msleep(10);
	input_report_key(es705_priv.input, vs_event_type, 0);
	input_sync(es705_priv.input);
#else
	struct slim_device *sbdev = es705->gen0_client;
	kobject_uevent(&sbdev->dev.kobj, KOBJ_CHANGE);
#endif
}

void es705_gpio_free(struct esxxx_platform_data *pdata)
{
	if (pdata->reset_gpio != -1)
		gpio_free(pdata->reset_gpio);
	if (pdata->gpioa_gpio != -1)
		gpio_free(pdata->gpioa_gpio);
	if (pdata->uart_gpio != -1)
		gpio_free(pdata->uart_gpio);
	if (pdata->wakeup_gpio != -1)
		gpio_free(pdata->wakeup_gpio);
}

static struct platform_device msm_es705_mclk_dev = {
	.name = "es705_mclk_dev_pdev",
	.id = -1,
	.dev = {
		.init_name = "es705_mclk_dev",
	},
};

int es705_enable_ext_clk(int enable)
{
	struct clk *es705_codec_clk;

	dev_dbg(es705_priv.dev, "%s: clk_get osr_clk enable=%d\n",
		__func__, enable);

	es705_codec_clk = clk_get(&msm_es705_mclk_dev.dev, "osr_clk");
	if (!es705_codec_clk) {
		dev_err(es705_priv.dev, "%s: clk_get osr_clk FAIL\n",
		__func__);
		return -ENODEV;
	}

	if (enable)
		clk_prepare_enable(es705_codec_clk);
	else
		clk_disable_unprepare(es705_codec_clk);

	clk_put(es705_codec_clk);

	return 0;
}

void es705_clk_init(struct es705_priv *es705)
{
	es705->pdata->esxxx_clk_cb = es705_enable_ext_clk;
}

struct esxxx_platform_data *es705_populate_dt_pdata(struct device *dev)
{
	struct esxxx_platform_data *pdata;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "%s(): platform data allocation failed\n",
			__func__);
		goto err;
	}
	pdata->reset_gpio = of_get_named_gpio(dev->of_node,
					      "es705-reset-gpio", 0);
	if (pdata->reset_gpio < 0)
		of_property_read_u32(dev->of_node,
					"es705-reset-expander-gpio", &pdata->reset_gpio);
	if (pdata->reset_gpio < 0) {
		dev_err(dev, "%s(): get reset_gpio failed\n", __func__);
		goto alloc_err;
	}
	dev_dbg(dev, "%s(): reset gpio %d\n", __func__, pdata->reset_gpio);

#if !defined(CONFIG_ARCH_MSM8226)
	pdata->gpioa_gpio = of_get_named_gpio(dev->of_node,
					      "es705-gpioa-gpio", 0);
	if (pdata->gpioa_gpio < 0) {
		dev_err(dev, "%s(): get gpioa_gpio failed\n", __func__);
		goto alloc_err;
	}
	dev_dbg(dev, "%s(): gpioa gpio %d\n", __func__, pdata->gpioa_gpio);
#endif

#if !defined(CONFIG_SEC_S_PROJECT) && !defined(CONFIG_ARCH_MSM8226)
	pdata->gpiob_gpio = of_get_named_gpio(dev->of_node,
					      "es705-gpiob-gpio", 0);
	if (pdata->gpiob_gpio < 0) {
		dev_err(dev, "%s(): get gpiob_gpio failed\n", __func__);
		goto alloc_err;
	}
	dev_dbg(dev, "%s(): gpiob gpio %d\n", __func__, pdata->gpiob_gpio);
#endif

	pdata->uart_tx_gpio = of_get_named_gpio(dev->of_node,
					     "es705-uart-tx", 0);
	if (pdata->uart_tx_gpio < 0) {
		dev_info(dev, "%s(): get uart_tx_gpio failed\n", __func__);
		pdata->uart_tx_gpio = -1;
	}
	dev_dbg(dev, "%s(): uart tx gpio %d\n", __func__, pdata->uart_tx_gpio);

	pdata->uart_rx_gpio = of_get_named_gpio(dev->of_node,
					     "es705-uart-rx", 0);
	if (pdata->uart_rx_gpio < 0) {
		dev_info(dev, "%s(): get uart_rx_gpio failed\n", __func__);
		pdata->uart_rx_gpio = -1;
	}
	dev_dbg(dev, "%s(): uart rx gpio %d\n", __func__, pdata->uart_rx_gpio);

	pdata->wakeup_gpio = of_get_named_gpio(dev->of_node,
					     "es705-wakeup-gpio", 0);
	if (pdata->wakeup_gpio < 0) {
		dev_info(dev, "%s(): get wakeup_gpio failed\n", __func__);
		pdata->wakeup_gpio = -1;
	}
	dev_dbg(dev, "%s(): wakeup gpio %d\n", __func__, pdata->wakeup_gpio);

	pdata->uart_gpio = of_get_named_gpio(dev->of_node,
					     "es705-uart-gpio", 0);
	if (pdata->uart_gpio < 0) {
		dev_info(dev, "%s(): get uart_gpio failed\n", __func__);
		pdata->uart_gpio = -1;
	}
	dev_dbg(dev, "%s(): uart gpio %d\n", __func__, pdata->uart_gpio);
	#if !defined(CONFIG_SEC_S_PROJECT)  && !defined(CONFIG_ARCH_MSM8226)
	pdata->irq_base = gpio_to_irq(pdata->gpiob_gpio);
	#endif

	return pdata;
alloc_err:
	devm_kfree(dev, pdata);
err:
	return NULL;
}
