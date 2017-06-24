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
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

#define BT_UART_CFG
#define BT_LPM_ENABLE

#define BT_UART_RTS 48
#define BT_UART_CTS 47
#define BT_UART_RXD 46
#define BT_UART_TXD 45
#if defined(CONFIG_MACH_MONTBLANC)
#define BT_HOST_WAKE 75

#define BT_WAKE 34
#define BT_EN 42
#define BTWIFI_LDO_EN 130
#elif defined(CONFIG_MACH_MELIUSCASKT) || defined(CONFIG_MACH_MELIUSCAKTT) || defined(CONFIG_MACH_MELIUSCALGT)
#define BT_HOST_WAKE 44

#define BT_WAKE 58
#define BT_EN 76
#elif defined(CONFIG_SEC_PATEK_PROJECT)
#define BT_HOST_WAKE 75

#define BT_WAKE 91
#define BT_EN 25
#else
#define BT_HOST_WAKE 75

#define BT_WAKE 41
#define BT_EN 42
#endif


#define GPIO_BT_UART_RTS BT_UART_RTS
#define GPIO_BT_UART_CTS BT_UART_CTS
#define GPIO_BT_UART_RXD BT_UART_RXD
#define GPIO_BT_UART_TXD BT_UART_TXD
#define GPIO_BT_HOST_WAKE BT_HOST_WAKE
#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE) || defined(CONFIG_BCM4339) || defined(CONFIG_BCM4339_MODULE)
int bt_is_running=0;
#endif

EXPORT_SYMBOL(bt_is_running);

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
        .start	= -1,
        .end	= -1,
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

    bluesleep_resources[1].start = get_gpio_hwrev(BT_WAKE);
    bluesleep_resources[1].end = get_gpio_hwrev(BT_WAKE);

    bluesleep_resources[2].start = gpio_to_irq(GPIO_BT_HOST_WAKE);
    bluesleep_resources[2].end = gpio_to_irq(GPIO_BT_HOST_WAKE);
}
#endif

static int bcm4339_bt_rfkill_set_power(void *data, bool blocked)
{
/* rfkill_ops callback. Turn transmitter on when blocked is false */
#ifdef BT_UART_CFG
    int pin, rc = 0;
#endif

    if (!blocked) {
        pr_err("[BT] Bluetooth Power On.\n");

        gpio_set_value(get_gpio_hwrev(BT_WAKE), 1);

        if (gpio_get_value(get_gpio_hwrev(GPIO_BT_HOST_WAKE)) == 0)
            pr_err("[BT] BT_HOST_WAKE is low.\n");

#ifdef BT_UART_CFG
        for (pin = 0; pin < ARRAY_SIZE(bt_uart_on_table); pin++) {
            rc = gpio_tlmm_config(bt_uart_on_table[pin],
                GPIO_CFG_ENABLE);
            if (rc < 0)
                pr_err("[BT] %s: gpio_tlmm_config(%#x)=%d\n",
                    __func__, bt_uart_on_table[pin], rc);
        }
#endif

        gpio_set_value(get_gpio_hwrev(BT_EN), 1);
#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE) || defined(CONFIG_BCM4339) || defined(CONFIG_BCM4339_MODULE)
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

        gpio_set_value(get_gpio_hwrev(BT_EN), 0);
		gpio_set_value(get_gpio_hwrev(BT_WAKE), 0);
#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE) || defined(CONFIG_BCM4339) || defined(CONFIG_BCM4339_MODULE)
			bt_is_running = 0;
#endif
    }
    return 0;
}

static struct platform_device bcm4339_bluetooth_platform_device = {
    .name		= "bcm4339_bluetooth",
    .id		= -1,
};

static struct platform_device *jf_bt_devs[] __initdata = {
    &bcm4339_bluetooth_platform_device,
};

extern unsigned int system_rev;

//void __init msm8974_bt_init(struct device *dev)
void __init msm8974_bt_init(void)
{
    int err = 0;
#ifdef CONFIG_MACH_MONTBLANC
    int rc = 0;
    pr_err("[BT] msm8974_bt_init(%d)\n", system_rev);

    if (system_rev < 2) {
        rc = gpio_request(get_gpio_hwrev(BTWIFI_LDO_EN), "btwifi_ldoen_gpio");
        if (unlikely(rc)) {
            pr_err("[BT] BTWIFI_LDO_EN request failed.\n");
            //return;
        }
        gpio_direction_output(get_gpio_hwrev(BTWIFI_LDO_EN), 0);
        //usleep_range(20000, 20000);
        gpio_set_value(get_gpio_hwrev(BTWIFI_LDO_EN), 1);
    }
#endif
#ifdef BT_LPM_ENABLE
    gpio_rev_init();
    err = platform_device_register(&msm_bluesleep_device);
	if (err) {
	    pr_err("[BT] failed to register Bluesleep device.\n");
		return;
	}
#endif

    platform_add_devices(jf_bt_devs, ARRAY_SIZE(jf_bt_devs));
}


static const struct rfkill_ops bcm4339_bt_rfkill_ops = {
    .set_block = bcm4339_bt_rfkill_set_power,
};

static int bcm4339_bluetooth_probe(struct platform_device *pdev)
{
    int rc = 0;

#ifdef BT_UART_CFG
    int pin = 0;
#endif
#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE) || defined(CONFIG_BCM4339) || defined(CONFIG_BCM4339_MODULE)
	bt_is_running = 0;
#endif
    rc = gpio_request(get_gpio_hwrev(BT_EN), "bcm4339_bten_gpio");
    if (unlikely(rc)) {
        pr_err("[BT] GPIO_BT_EN request failed.\n");
        return rc;
    }

#if defined(CONFIG_SEC_PATEK_PROJECT)
    gpio_tlmm_config(GPIO_CFG(BT_EN, 0, GPIO_CFG_OUTPUT,
        GPIO_CFG_PULL_DOWN, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
    gpio_set_value(BT_EN, 0);
    gpio_tlmm_config(GPIO_CFG(BT_WAKE, 0, GPIO_CFG_OUTPUT,
        GPIO_CFG_NO_PULL, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
    gpio_set_value(BT_WAKE, 0);
#else
    gpio_direction_output(get_gpio_hwrev(BT_EN), 0);
#endif

	/* gpio request for bt_wake will be in bluesleeep.
    rc = gpio_request(get_gpio_hwrev(BT_WAKE), "bcm4339_btwake_gpio");
    if (unlikely(rc)) {
        pr_err("[BT] GPIO_BT_WAKE request failed.\n");
        return rc;
	}
    gpio_direction_output(get_gpio_hwrev(BT_WAKE), 0);
    */

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

    bt_rfkill = rfkill_alloc("bcm4339 Bluetooth", &pdev->dev,
                RFKILL_TYPE_BLUETOOTH, &bcm4339_bt_rfkill_ops,
                NULL);

    if (unlikely(!bt_rfkill)) {
        pr_err("[BT] bt_rfkill alloc failed.\n");
        gpio_free(get_gpio_hwrev(BT_EN));
        return -ENOMEM;
    }


    rfkill_init_sw_state(bt_rfkill, 0);

    rc = rfkill_register(bt_rfkill);

    if (unlikely(rc)) {
        pr_err("[BT] bt_rfkill register failed.\n");
        rfkill_destroy(bt_rfkill);
        gpio_free(get_gpio_hwrev(BT_EN));
        gpio_free(get_gpio_hwrev(BT_WAKE));
        return rc;
    }


    rfkill_set_sw_state(bt_rfkill, true);

    return rc;
}

static int bcm4339_bluetooth_remove(struct platform_device *pdev)
{
    rfkill_unregister(bt_rfkill);
    rfkill_destroy(bt_rfkill);

    gpio_free(get_gpio_hwrev(BT_EN));
    gpio_free(get_gpio_hwrev(BT_WAKE));
    return 0;
}

static struct platform_driver bcm4339_bluetooth_platform_driver = {
    .probe = bcm4339_bluetooth_probe,
    .remove = bcm4339_bluetooth_remove,
    .driver = {
        .name = "bcm4339_bluetooth",
        .owner = THIS_MODULE,
    },
};

static int __init bcm4339_bluetooth_init(void)
{
    return platform_driver_register(&bcm4339_bluetooth_platform_driver);
}

static void __exit bcm4339_bluetooth_exit(void)
{
#ifdef BT_LPM_ENABLE
    platform_device_unregister(&msm_bluesleep_device);
#endif
    platform_driver_unregister(&bcm4339_bluetooth_platform_driver);
}

module_init(bcm4339_bluetooth_init);
module_exit(bcm4339_bluetooth_exit);

MODULE_ALIAS("platform:bcm4339");
MODULE_DESCRIPTION("bcm4339_bluetooth");
MODULE_LICENSE("GPL");
