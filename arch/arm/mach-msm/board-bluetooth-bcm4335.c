/*
 * Bluetooth Broadcom GPIO and Low Power Mode control
 *
 *  Copyright (C) 2011 Samsung Electronics Co., Ltd.
 *  Copyright (C) 2011 Google, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define DEBUG

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/wakelock.h>

#include <asm/mach-types.h>

#include <linux/barcode_emul.h>
#include <mach/gpiomux.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

#define BT_UART_CFG
#define BT_LPM_ENABLE

#define BT_UART_RTS 48
#define BT_UART_CTS 47
#define BT_UART_RXD 46
#define BT_UART_TXD 45

#if defined(CONFIG_MACH_HLTEEUR) || defined(CONFIG_MACH_H3GDUOS)
#define BT_HOST_WAKE 75
#else
#define BT_HOST_WAKE 44
#endif

#define BT_WAKE -1
#define BT_EN -1


#define GPIO_BT_UART_RTS BT_UART_RTS
#define GPIO_BT_UART_CTS BT_UART_CTS
#define GPIO_BT_UART_RXD BT_UART_RXD
#define GPIO_BT_UART_TXD BT_UART_TXD
#define GPIO_BT_HOST_WAKE BT_HOST_WAKE

#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE)
int bt_is_running=0;
#endif

EXPORT_SYMBOL(bt_is_running);

#define FPGA_GPIO_BT_EN 4
#define FPGA_GPIO_BT_WAKE 5

static struct rfkill *bt_rfkill;

int get_gpio_hwrev(int gpio)
{
	return gpio;
}

#ifdef BT_UART_CFG
static unsigned bt_uart_on_table[] = {
	GPIO_CFG(GPIO_BT_UART_RTS, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
	GPIO_CFG(GPIO_BT_UART_CTS, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
	GPIO_CFG(GPIO_BT_UART_RXD, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
	GPIO_CFG(GPIO_BT_UART_TXD, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
};

static unsigned bt_uart_off_table[] = {
	GPIO_CFG(GPIO_BT_UART_RTS, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
	GPIO_CFG(GPIO_BT_UART_CTS, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
	GPIO_CFG(GPIO_BT_UART_RXD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
	GPIO_CFG(GPIO_BT_UART_TXD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
};
#endif

#ifdef BT_LPM_ENABLE
static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= -1, //GPIO_BT_HOST_WAKE,
		.end	= -1, //GPIO_BT_HOST_WAKE,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= -1, //FPGA_GPIO_BT_WAKE,
		.end	= -1, //FPGA_GPIO_BT_WAKE,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= -1, //44,
		.end	= -1, //44,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
};

//static int gpio_rev_init(struct device *dev)
static void gpio_rev_init(void)
{

	bluesleep_resources[0].start = get_gpio_hwrev(GPIO_BT_HOST_WAKE);
	bluesleep_resources[0].end = get_gpio_hwrev(GPIO_BT_HOST_WAKE);
	bluesleep_resources[1].start = FPGA_GPIO_BT_WAKE;
	bluesleep_resources[1].end = FPGA_GPIO_BT_WAKE;
	bluesleep_resources[2].start = gpio_to_irq(GPIO_BT_HOST_WAKE);
	bluesleep_resources[2].end = gpio_to_irq(GPIO_BT_HOST_WAKE);

}
#endif


static int bcm4335_bt_rfkill_set_power(void *data, bool blocked)
{
	/* rfkill_ops callback. Turn transmitter on when blocked is false */
#ifdef BT_UART_CFG
	int pin, rc = 0;
#endif
	int ret = -1;

	if (!blocked) {
		pr_err("[BT] Bluetooth Power On.\n");

		ret = ice_gpiox_set(FPGA_GPIO_BT_WAKE, 1);

		if (ret)
			pr_err("[BT] failed to set ext_wake to high.\n");

		if (gpio_get_value(get_gpio_hwrev(GPIO_BT_HOST_WAKE)))
			pr_err("[BT] host_wake is high.\n");
		else
			pr_err("[BT] host_wake is low.\n");

#ifdef BT_UART_CFG
	for (pin = 0; pin < ARRAY_SIZE(bt_uart_on_table); pin++) {
			rc = gpio_tlmm_config(bt_uart_on_table[pin],
					      GPIO_CFG_ENABLE);
		if (rc < 0)
			pr_err("[BT] %s: gpio_tlmm_config(%#x)=%d\n",
			       __func__, bt_uart_on_table[pin], rc);
	}
#endif

		ret = ice_gpiox_set(FPGA_GPIO_BT_EN, 1);

		if (ret)
			pr_err("[BT] failed to set BT_EN.\n");
#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE)
		else
			bt_is_running = 1;
#endif
	} else {
#ifdef BT_UART_CFG
	for (pin = 0; pin < ARRAY_SIZE(bt_uart_off_table); pin++) {
			rc = gpio_tlmm_config(bt_uart_off_table[pin],
					      GPIO_CFG_ENABLE);
		if (rc < 0)
				pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_uart_off_table[pin], rc);
	}
#endif
		pr_err("[BT] Bluetooth Power Off.\n");

		ret = ice_gpiox_set(FPGA_GPIO_BT_EN, 0);
		if (ret)
			pr_err("[BT] failed to set BT_EN.\n");

		ret = ice_gpiox_set(FPGA_GPIO_BT_WAKE, 0);	//tseop.kim
		if (ret)
			pr_err("[BT] failed to set ext_wake to low.\n");
#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE)
		else
			bt_is_running = 0;
#endif
	}
	return 0;
}

static struct platform_device bcm4335_bluetooth_platform_device = {
	.name		= "bcm4335_bluetooth",
	.id		= -1,
};

static struct platform_device *jf_bt_devs[] __initdata = {
	&bcm4335_bluetooth_platform_device,
};


//void __init msm8974_bt_init(struct device *dev)
void __init msm8974_bt_init(void)
{
	#ifdef BT_LPM_ENABLE
	gpio_rev_init();
	platform_device_register(&msm_bluesleep_device);
	#endif

	platform_add_devices(jf_bt_devs, ARRAY_SIZE(jf_bt_devs));
}


static const struct rfkill_ops bcm4335_bt_rfkill_ops = {
	.set_block = bcm4335_bt_rfkill_set_power,
};

static int bcm4335_bluetooth_probe(struct platform_device *pdev)
{
	int rc = 0;

    #ifdef BT_UART_CFG
    int pin = 0;
    #endif

#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE)
	bt_is_running = 0;
#endif
    /* temporailiy set HOST_WAKE OUT direction until FPGA work finishs */
	/* if setting HOST_WAKE to NO PULL, BT would not be turned on. */
	/* By guideline of BRCM, it is needed to determine pull status */
	#ifndef BT_LPM_ENABLE
	gpio_tlmm_config(GPIO_CFG(get_gpio_hwrev(GPIO_BT_HOST_WAKE), 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_PULL_UP, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	gpio_set_value(get_gpio_hwrev(GPIO_BT_HOST_WAKE), 1);
	#endif

    #ifdef BT_UART_CFG
    for (pin = 0; pin < ARRAY_SIZE(bt_uart_off_table); pin++) {
        rc = gpio_tlmm_config(bt_uart_off_table[pin], GPIO_CFG_ENABLE);
        if (rc < 0)
            pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
                    __func__, bt_uart_off_table[pin], rc);
    }
    #endif

	bt_rfkill = rfkill_alloc("bcm4335 Bluetooth", &pdev->dev,
				RFKILL_TYPE_BLUETOOTH, &bcm4335_bt_rfkill_ops,
				NULL);

	if (unlikely(!bt_rfkill)) {
		pr_err("[BT] bt_rfkill alloc failed.\n");
		gpio_free(ice_gpiox_get(FPGA_GPIO_BT_EN));
		return -ENOMEM;
	}


	rfkill_init_sw_state(bt_rfkill, 0);

	rc = rfkill_register(bt_rfkill);

	if (unlikely(rc)) {
		pr_err("[BT] bt_rfkill register failed.\n");
		rfkill_destroy(bt_rfkill);
		gpio_free(ice_gpiox_get(FPGA_GPIO_BT_EN));
		return rc;
	}


	rfkill_set_sw_state(bt_rfkill, true);

	return rc;
}

static int bcm4335_bluetooth_remove(struct platform_device *pdev)
{
	rfkill_unregister(bt_rfkill);
	rfkill_destroy(bt_rfkill);

	gpio_free(ice_gpiox_get(FPGA_GPIO_BT_EN));

	return 0;
}

static struct platform_driver bcm4335_bluetooth_platform_driver = {
	.probe = bcm4335_bluetooth_probe,
	.remove = bcm4335_bluetooth_remove,
	.driver = {
		   .name = "bcm4335_bluetooth",
		   .owner = THIS_MODULE,
		   },
};

static int __init bcm4335_bluetooth_init(void)
{
	return platform_driver_register(&bcm4335_bluetooth_platform_driver);
}

static void __exit bcm4335_bluetooth_exit(void)
{
#ifdef BT_LPM_ENABLE
	platform_device_unregister(&msm_bluesleep_device);
#endif
	platform_driver_unregister(&bcm4335_bluetooth_platform_driver);
}

module_init(bcm4335_bluetooth_init);
module_exit(bcm4335_bluetooth_exit);

MODULE_ALIAS("platform:bcm4335");
MODULE_DESCRIPTION("bcm4335_bluetooth");
MODULE_LICENSE("GPL");
