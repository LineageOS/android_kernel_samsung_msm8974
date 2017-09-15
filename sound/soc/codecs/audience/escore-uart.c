/*
 * escore-uart.c  --  Audience eS705 UART interface
 *
 * Copyright 2013 Audience, Inc.
 *
 * Author: Matt Lupfer <mlupfer@cardinalpeak.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/completion.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/kthread.h>
#include <linux/esxxx.h>
#include <linux/serial_core.h>
#include <linux/tty.h>

#include "escore.h"
#include "escore-uart.h"
#include "escore-uart-common.h"
#include "escore-cdev.h"

static int escore_uart_boot_setup(struct escore_priv *escore);
static int escore_uart_boot_finish(struct escore_priv *escore);
static int escore_uart_probe(struct platform_device *dev);
static int escore_uart_remove(struct platform_device *dev);

#ifdef ESCORE_FW_LOAD_BUF_SZ
#undef ESCORE_FW_LOAD_BUF_SZ
#endif
#define ESCORE_FW_LOAD_BUF_SZ 1024

int escore_uart_boot_setup(struct escore_priv *escore)
{
	u8 sbl_sync_cmd = ESCORE_SBL_SYNC_CMD;
	u8 sbl_boot_cmd = ESCORE_SBL_BOOT_CMD;
	char msg[4];
	int rc;

	/* set speed to bootloader baud */
	escore_configure_tty(escore_priv.uart_dev.tty,
		UART_TTY_BAUD_RATE_BOOTLOADER, UART_TTY_STOP_BITS);

	pr_debug("%s()\n", __func__);

	/* SBL SYNC BYTE 0x00 */
	memset(msg, 0, 4);
	pr_debug("%s(): write ESCORE_SBL_SYNC_CMD = 0x%02x\n", __func__,
		sbl_sync_cmd);
	memcpy(msg, (char *)&sbl_sync_cmd, 1);
	rc = escore->dev_write(escore, msg, 1);
	if (rc < sizeof(sbl_sync_cmd)) {
		pr_err("%s(): firmware load failed sbl sync write\n", __func__);
		rc = rc < 0 ? rc : -EIO;
		goto escore_bootup_failed;
	}
	pr_debug("%s(): firmware load sbl sync write rc=%d\n", __func__, rc);
	usleep_range(10000, 10500);
	memset(msg, 0, 4);
	rc = escore->dev_read(escore, msg, 1);
	if (rc < 1) {
		pr_err("%s(): firmware load failed sync ack rc = %d\n",
			__func__, rc);
		rc = rc < 0 ? rc : -EIO;
		goto escore_bootup_failed;
	}
	pr_debug("%s(): sbl sync ack = 0x%02x\n", __func__, msg[0]);
	if (msg[0] != ESCORE_SBL_SYNC_ACK) {
		pr_err("%s(): firmware load failed sync ack pattern", __func__);
		rc = rc < 0 ? rc : -EIO;
		goto escore_bootup_failed;
	}

	/* SBL BOOT BYTE 0x01 */
	memset(msg, 0, 4);
	pr_debug("%s(): write ESCORE_SBL_BOOT_CMD = 0x%02x\n", __func__,
		sbl_boot_cmd);
	memcpy(msg, (char *)&sbl_boot_cmd, 1);
	rc = escore->dev_write(escore, msg, 1);
	if (rc < sizeof(sbl_boot_cmd)) {
		pr_err("%s(): firmware load failed sbl boot write\n", __func__);
		rc = rc < 0 ? rc : -EIO;
		goto escore_bootup_failed;
	}
	usleep_range(10000, 10500);
	memset(msg, 0, 4);
	rc = escore->dev_read(escore, msg, 1);
	if (rc < 1) {
		pr_err("%s(): firmware load failed boot ack\n", __func__);
		rc = rc < 0 ? rc : -EIO;
		goto escore_bootup_failed;
	}
	pr_debug("%s(): sbl boot ack = 0x%02x\n", __func__, msg[0]);

	if (msg[0] != ESCORE_SBL_BOOT_ACK) {
		pr_err("%s(): firmware load failed boot ack pattern", __func__);
		rc = rc < 0 ? rc : -EIO;
		goto escore_bootup_failed;
	}

	rc = 0;

escore_bootup_failed:
	return rc;
}

int escore_uart_boot_finish(struct escore_priv *escore)
{
	char msg[4];
	int rc;

	/*
	 * Give the chip some time to become ready after firmware
	 * download. (FW is still transferring)
	 */
	msleep(2000);

	/* discard up to two extra bytes from escore during firmware load */
	memset(msg, 0, 4);
	rc = escore->dev_read(escore, msg, 1);
	if (rc < 1) {
		pr_err("%s(): firmware read\n", __func__);
		rc = rc < 0 ? rc : -EIO;
		goto escore_bootup_failed;
	}

	pr_debug("%s(): read byte = 0x%02x\n", __func__, msg[0]);

	if (msg[0] != 0x02) {
		rc = escore->dev_read(escore, msg, 1);
		if (rc < 1) {
			rc = rc < 0 ? rc : -EIO;
			goto escore_bootup_failed;
		}

		pr_debug("%s(): read byte = 0x%02x\n", __func__, msg[0]);
	}


	/* now switch to firmware baud to talk to chip */
	escore_configure_tty(escore_priv.uart_dev.tty,
		UART_TTY_BAUD_RATE_FIRMWARE, UART_TTY_STOP_BITS);

escore_bootup_failed:
	return rc;
}

int escore_uart_probe_thread(void *ptr)
{
	int rc = 0;
	struct device *dev = (struct device *) ptr;

	rc = escore_uart_open(&escore_priv);
	if (rc) {
		dev_err(dev, "%s(): es705_uart_open() failed %d\n",
			__func__, rc);
		return rc;
	}

	/* device node found, continue */
	dev_dbg(dev, "%s(): successfully opened tty\n",
		  __func__);

	/* set escore function pointers */
	escore_priv.dev_read = escore_uart_read;
	escore_priv.dev_write = escore_uart_write;
	escore_priv.cmd = escore_uart_cmd;
	escore_priv.boot_setup = escore_uart_boot_setup;
	escore_priv.uart_boot_finish = escore_uart_boot_finish;
	escore_priv.streamdev = es_uart_streamdev;

	rc = escore_priv.probe(dev);
	if (rc) {
		dev_err(escore_priv.dev, "%s(): escore_core_probe() failed %d\n",
			__func__, rc);
		goto bootup_error;
	}

	rc = escore_priv.bootup(&escore_priv);

	if (rc) {
		dev_err(escore_priv.dev, "%s(): escore_bootup failed %d\n",
			__func__, rc);
		goto bootup_error;
	}

	release_firmware(escore_priv.fw);

	rc = snd_soc_register_codec(escore_priv.dev,
			escore_priv.soc_codec_dev_escore,
			escore_priv.escore_dai,
			escore_priv.escore_dai_nr);
	dev_dbg(dev, "%s(): rc = snd_soc_regsiter_codec() = %d\n", __func__,
		rc);

	/* init escore character device here, now that the UART is discovered */
	rc = escore_cdev_init(&escore_priv);
	if (rc) {
		pr_err("failed to initialize char device = %d\n", rc);
		goto cdev_init_error;
	}

	return rc;

bootup_error:
	release_firmware(escore_priv.fw);
	/* close filp */
	filp_close(escore_priv.uart_dev.file, 0);
cdev_init_error:
	dev_dbg(escore_priv.dev, "%s(): exit with error\n", __func__);
	return rc;
}

static int escore_uart_probe(struct platform_device *dev)
{
	int rc = 0;
	struct task_struct *uart_probe_thread = NULL;

	dev_dbg(&dev->dev, "%s():\n", __func__);
	uart_probe_thread = kthread_run(escore_uart_probe_thread,
					(void *) &dev->dev,
					"escore uart thread");
	if (IS_ERR_OR_NULL(uart_probe_thread)) {
		pr_err("%s(): can't create escore UART probe thread = %p\n",
			__func__, uart_probe_thread);
		rc = -ENOMEM;
	}

	return rc;
}

static int escore_uart_remove(struct platform_device *dev)
{
	int rc = 0;
	/*
	 * ML: GPIO pins are not connected
	 *
	 * struct esxxx_platform_data *pdata = escore_priv.pdata;
	 *
	 * gpio_free(pdata->reset_gpio);
	 * gpio_free(pdata->wakeup_gpio);
	 * gpio_free(pdata->gpioa_gpio);
	 */

	if (escore_priv.uart_dev.ld)
		tty_ldisc_deref(escore_priv.uart_dev.ld);

	if (escore_priv.uart_dev.file)
		rc = filp_close(escore_priv.uart_dev.file, 0);

	escore_priv.uart_dev.tty = NULL;
	escore_priv.uart_dev.file = NULL;

	snd_soc_unregister_codec(escore_priv.dev);

	return rc;
}

static int escore_uart_suspend(struct device *dev)
{
	return 0;
}

static int escore_uart_resume(struct device *dev)
{
	return 0;
}

static int escore_uart_runtime_suspend(struct device *dev)
{
	return 0;
}

static int escore_uart_runtime_resume(struct device *dev)
{
	return 0;
}

static int escore_uart_runtime_idle(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops escore_uart_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(
		escore_uart_suspend,
		escore_uart_resume
	)
	SET_RUNTIME_PM_OPS(
		escore_uart_runtime_suspend,
		escore_uart_runtime_resume,
		escore_uart_runtime_idle
	)
};

struct platform_driver escore_uart_driver = {
	.driver = {
		.name = "escore-codec",
		.owner = THIS_MODULE,
		.pm = &escore_uart_dev_pm_ops,
	},
	.probe = escore_uart_probe,
	.remove = escore_uart_remove,
};

static struct esxxx_platform_data esxxx_platform_data = {
	.irq_base       = 0,
	.reset_gpio     = -1,
	.wakeup_gpio    = -1,
	.gpioa_gpio     = -1,
	.gpiob_gpio     = -1,
	.esxxx_clk_cb   = NULL,
};

struct platform_device escore_uart_device = {
	.name	   = "escore-codec",
	.resource       = NULL,
	.num_resources  = 0,
	.dev = {
		.platform_data = &esxxx_platform_data,
	}
};

/* FIXME: Kludge for escore_bus_init abstraction */
int escore_uart_bus_init(struct escore_priv *escore)
{
	int rc;

	rc = platform_driver_register(&escore_uart_driver);
	if (rc)
		return rc;

	rc = platform_device_register(&escore_uart_device);
	if (rc)
		return rc;

	pr_debug("%s(): registered as UART", __func__);

	return rc;
}

MODULE_DESCRIPTION("ASoC ESCORE driver");
MODULE_AUTHOR("Greg Clemson <gclemson@audience.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:escore-codec");
