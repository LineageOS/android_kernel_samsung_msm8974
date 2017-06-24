/*
 *
* drivers/media/isdbt/isdbt_spi.c
 *
* isdbt driver
*
* Copyright (C) (2014, Samsung Electronics)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
 *
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/of_gpio.h>

#include "isdbt.h"

#define SPI_DEV_NAME	"isdbt"

struct spi_device *isdbt_spi_device;

int isdbt_spi_probe(struct spi_device *spi)
{
	int ret;

	isdbt_spi_device = spi;

	DPRINTK("isdbtspi_probe() isdbt_spi_device : 0x%x\n", (unsigned int)isdbt_spi_device);

	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 8;
	ret = spi_setup(spi);
	if (ret < 0) {
		DPRINTK("spi_setup() fail ret : %d\n", ret);
		return ret;
	}

	return 0;
}

static int __devexit isdbt_spi_remove(struct spi_device *spi)
{
	return 0;
}

static const struct of_device_id isdbt_spi_match_table[] = {
	{.compatible = "isdbt_spi_comp"},
	{}
};

static struct spi_driver isdbt_spi_driver = {
	.driver = {
		.name = SPI_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = isdbt_spi_match_table,
	},

	.probe = isdbt_spi_probe,
	.remove	= __devexit_p(isdbt_spi_remove),
};

int isdbt_spi_init(void)
{
	DPRINTK("isdbt_spi_init\n");
	return spi_register_driver(&isdbt_spi_driver);
}

void isdbt_spi_exit(void)
{
	DPRINTK("isdbt_spi_exit\n");
	spi_unregister_driver(&isdbt_spi_driver);
}

struct spi_device *isdbt_get_if_handle(void)
{
	return isdbt_spi_device;
}

