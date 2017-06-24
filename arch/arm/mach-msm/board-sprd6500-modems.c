/* linux/arch/arm/mach-xxxx/board-superior-cmcc-modems.c
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <linux/platform_data/modem.h>

#include <mach/gpiomux.h>


#if defined(CONFIG_MACH_H3GDUOS)
#define GPIO_GSM_PHONE_ON	135
#define GPIO_PDA_ACTIVE		136
#define GPIO_PHONE_ACTIVE	18
#define GPIO_CP_DUMP_INT	73
#define GPIO_AP_CP_INT1		124
#define GPIO_AP_CP_INT2		125

#define GPIO_UART_SEL		119
#define GPIO_SIM_SEL		115
#define ESC_SIM_DETECT_IRQ	125

#elif defined(CONFIG_MACH_K3GDUOS_CTC)
#define GPIO_GSM_PHONE_ON	127
#define GPIO_PDA_ACTIVE		118
#define GPIO_PHONE_ACTIVE	107
#define GPIO_AP_CP_INT1		0xFFF
#define GPIO_AP_CP_INT2		0xFFF

#define GPIO_UART_SEL		135
#define GPIO_SIM_SEL		123
#define ESC_SIM_DETECT_IRQ	123
#endif

#if defined(CONFIG_GSM_MODEM_GG_DUOS)
/* gsm target platform data */
static struct modem_io_t gsm_io_devices[] = {
	[0] = {
		.name = "gsm_boot0",
		.id = 0x1,
		.format = IPC_BOOT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[1] = {
		.name = "gsm_ipc0",
		.id = 0x00,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[2] = {
		.name = "umts_ipc0",
		.id = 0x01,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[3] = {
		.name = "gsm_rfs0",
		.id = 0x28,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[4] = {
		.name = "gsm_multi_pdp",
		.id = 0x1,
		.format = IPC_MULTI_RAW,
		.io_type = IODEV_DUMMY,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[5] = {
		.name = "gsm_rmnet0",
		.id = 0x2A,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[6] = {
		.name = "gsm_rmnet1",
		.id = 0x2B,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[7] = {
		.name = "gsm_rmnet2",
		.id = 0x2C,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[8] = {
		.name = "gsm_router",
		.id = 0x39,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	/*
	[8] = {
		.name = "gsm_csd",
		.id = 0x21,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[9] = {
		.name = "gsm_ramdump0",
		.id = 0x1,
		.format = IPC_RAMDUMP,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[10] = {
		.name = "gsm_loopback0",
		.id = 0x3F,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	*/
};

static struct modem_data gsm_modem_data = {
	.name = "sprd6500",

	.modem_type = SPRD_SC6500,
	.link_types = LINKTYPE(LINKDEV_SPI),
	.modem_net = TDSCDMA_NETWORK,
	.use_handover = false,
	.ipc_version = SIPC_VER_42,

	.num_iodevs = ARRAY_SIZE(gsm_io_devices),
	.iodevs = gsm_io_devices,
};

#else
/* gsm target platform data */
static struct modem_io_t gsm_io_devices[] = {
	[0] = {
		.name = "gsm_boot0",
		.id = 0x1,
		.format = IPC_BOOT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[1] = {
		.name = "gsm_ipc0",
		.id = 0x01,
		.format = IPC_FMT,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[2] = {
		.name = "gsm_rfs0",
		.id = 0x28,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[3] = {
		.name = "gsm_multi_pdp",
		.id = 0x1,
		.format = IPC_MULTI_RAW,
		.io_type = IODEV_DUMMY,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[4] = {
		.name = "gsm_rmnet0",
		.id = 0x2A,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[5] = {
		.name = "gsm_rmnet1",
		.id = 0x2B,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[6] = {
		.name = "gsm_rmnet2",
		.id = 0x2C,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[7] = {
		.name = "gsm_router",
		.id = 0x39,
		.format = IPC_RAW,
		.io_type = IODEV_NET,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	/*
	[8] = {
		.name = "gsm_csd",
		.id = 0x21,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[9] = {
		.name = "gsm_ramdump0",
		.id = 0x1,
		.format = IPC_RAMDUMP,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	[10] = {
		.name = "gsm_loopback0",
		.id = 0x3F,
		.format = IPC_RAW,
		.io_type = IODEV_MISC,
		.links = LINKTYPE(LINKDEV_SPI),
	},
	*/
};

static struct modem_data gsm_modem_data = {
	.name = "sprd6500",

	.modem_type = SPRD_SC6500,
	.link_types = LINKTYPE(LINKDEV_SPI),
	.modem_net = TDSCDMA_NETWORK,
	.use_handover = false,
	.ipc_version = SIPC_VER_40,

	.num_iodevs = ARRAY_SIZE(gsm_io_devices),
	.iodevs = gsm_io_devices,
};

#endif


void sprd6500_modem_cfg_gpio(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_GSM_PHONE_ON, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	gpio_set_value(GPIO_GSM_PHONE_ON, 0);

	gpio_tlmm_config(GPIO_CFG(GPIO_PDA_ACTIVE, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	gpio_set_value(GPIO_PDA_ACTIVE, 0);

	if(gpio_is_valid(GPIO_AP_CP_INT2))	{
	gpio_tlmm_config(GPIO_CFG(GPIO_AP_CP_INT2, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	gpio_set_value(GPIO_AP_CP_INT2, 0);
	}

	gpio_tlmm_config(GPIO_CFG(GPIO_UART_SEL, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	gpio_set_value(GPIO_UART_SEL, 0);

	gpio_tlmm_config(GPIO_CFG(GPIO_PHONE_ACTIVE, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);

	if(gpio_is_valid(GPIO_AP_CP_INT1))	{
	gpio_tlmm_config(GPIO_CFG(GPIO_AP_CP_INT1, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	}

	gsm_modem_data.gpio_cp_on = GPIO_GSM_PHONE_ON;
	gsm_modem_data.gpio_pda_active = GPIO_PDA_ACTIVE;
	gsm_modem_data.gpio_phone_active = GPIO_PHONE_ACTIVE;
	gsm_modem_data.gpio_ap_cp_int1 = GPIO_AP_CP_INT1;
	gsm_modem_data.gpio_ap_cp_int2 = GPIO_AP_CP_INT2;
	gsm_modem_data.gpio_uart_sel = GPIO_UART_SEL;
	gsm_modem_data.gpio_sim_sel= GPIO_SIM_SEL;

	pr_info("sprd6500_modem_cfg_gpio done\n");
	pr_info("uart_sel : [%d]\n", gpio_get_value(GPIO_UART_SEL));
}

static struct resource gsm_modem_res[] = {
	[0] = {
		.name = "cp_active_irq",
		.start = 0,
		.end = 0,
		.flags = IORESOURCE_IRQ,
	},
#if defined(CONFIG_SIM_DETECT)
	[1] = {
		.name = "sim_irq",
		.start = ESC_SIM_DETECT_IRQ,
		.end = ESC_SIM_DETECT_IRQ,
		.flags = IORESOURCE_IRQ,
	},
#endif
};

/* if use more than one modem device, then set id num */
static struct platform_device gsm_modem = {
	.name = "mif_sipc4",
	.id = -1,
	.num_resources = ARRAY_SIZE(gsm_modem_res),
	.resource = gsm_modem_res,
	.dev = {
		.platform_data = &gsm_modem_data,
	},
};

static int __init init_modem(void)
{
	sprd6500_modem_cfg_gpio();

	gsm_modem_res[0].start = gpio_to_irq(GPIO_PHONE_ACTIVE);
	gsm_modem_res[0].end = gpio_to_irq(GPIO_PHONE_ACTIVE);

	platform_device_register(&gsm_modem);

	mif_info("board init_sprd6500_modem done\n");
	return 0;
}
late_initcall(init_modem);
