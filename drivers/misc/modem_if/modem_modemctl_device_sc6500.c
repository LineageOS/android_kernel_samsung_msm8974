/* /linux/drivers/misc/modem_if/modem_modemctl_device_sprd6500.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/platform_device.h>

#include <mach/gpiomux.h>

#include <linux/platform_data/modem.h>
#include "modem_prj.h"
#include <linux/regulator/consumer.h>


#if defined(CONFIG_LINK_DEVICE_DPRAM)
#include "modem_link_device_dpram.h"
#elif defined(CONFIG_LINK_DEVICE_PLD)
#include "modem_link_device_pld.h"
#elif defined(CONFIG_LINK_DEVICE_SPI)
#include "modem_link_device_spi.h"
#endif

#if 0 //def CONFIG_OF
void sprd6500_modem_cfg_gpio(struct platform_device *pdev);
#endif

#define PIF_TIMEOUT		(180 * HZ)
#define DPRAM_INIT_TIMEOUT	(30 * HZ)

int sprd_boot_done;

#define GPIO_BT_UART_RTS 48
#define GPIO_BT_UART_CTS 47
#define GPIO_BT_UART_RXD 46
#define GPIO_BT_UART_TXD 45

static unsigned sprd6500_uart_on_table[] = {
	GPIO_CFG(GPIO_BT_UART_RTS, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
	GPIO_CFG(GPIO_BT_UART_CTS, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
	GPIO_CFG(GPIO_BT_UART_RXD, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
	GPIO_CFG(GPIO_BT_UART_TXD, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
};

static unsigned sprd6500_uart_off_table[] = {
	GPIO_CFG(GPIO_BT_UART_RTS, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
	GPIO_CFG(GPIO_BT_UART_CTS, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
	GPIO_CFG(GPIO_BT_UART_RXD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
	GPIO_CFG(GPIO_BT_UART_TXD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		 GPIO_CFG_8MA),
};

static int sprd6500_on(struct modem_ctl *mc)
{
	int pin = 0;

	pr_err("[MODEM_IF:SC6500] <%s> start!!!\n", __func__);

	disable_irq(mc->irq_phone_active);

	for (pin = 0; pin < ARRAY_SIZE(sprd6500_uart_on_table); pin++) {
		gpio_tlmm_config(sprd6500_uart_on_table[pin], GPIO_CFG_ENABLE);
	}

	// UART_SEL for UART download
	gpio_tlmm_config(GPIO_CFG(mc->gpio_uart_sel, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		GPIO_CFG_ENABLE);
	gpio_set_value(mc->gpio_uart_sel, 1);

	pr_err("[MODEM_IF:SC6500] <%s> CP On(%d,%d)\n", __func__, 
			gpio_get_value(mc->gpio_cp_on),
			gpio_get_value(mc->gpio_phone_active));

	msleep(100);

	gpio_tlmm_config(GPIO_CFG(mc->gpio_cp_on, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	gpio_set_value(mc->gpio_cp_on, 1);
//	msleep(100);

	pr_err("[MODEM_IF:SC6500] <%s> IRQ enabled\n", __func__);
	pr_err("[MODEM_IF:SC6500] <%s> Active = %d\n", __func__, 
			gpio_get_value(mc->gpio_phone_active));

	enable_irq(mc->irq_phone_active);
//	msleep(280);

	pr_err("[MODEM_IF:SC6500] <%s> Active = %d\n", __func__, 
			gpio_get_value(mc->gpio_phone_active));

#if defined(CONFIG_LINK_DEVICE_PLD)
	gpio_set_value(GPIO_FPGA_SPI_MOSI, 1);
	gpio_set_value(mc->gpio_fpga1_cs_n, 1);
#endif

	gpio_set_value(mc->gpio_pda_active, 1);

	return 0;
}

static int sprd6500_off(struct modem_ctl *mc)
{
	pr_info("[MODEM_IF:SC6500] sprd6500_off()\n");

	// UART_SEL for BT
	gpio_tlmm_config(GPIO_CFG(mc->gpio_uart_sel, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		GPIO_CFG_ENABLE);
	gpio_set_value(mc->gpio_uart_sel, 0);

	gpio_tlmm_config(GPIO_CFG(mc->gpio_cp_on, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);
	gpio_set_value(mc->gpio_cp_on, 0);

	mc->iod->modem_state_changed(mc->iod, STATE_OFFLINE);

	return 0;
}

static int sprd6500_reset(struct modem_ctl *mc)
{
	int ret = 0;

	pr_debug("[MODEM_IF:SC6500] sprd6500_reset()\n");

	ret = sprd6500_off(mc);
	if (ret)
		return -ENXIO;

	msleep(100);

	ret = sprd6500_on(mc);
	if (ret)
		return -ENXIO;

	return 0;
}

int sprd6500_boot_on(struct modem_ctl *mc)
{
	int pin, rc = 0;

	pr_err("[MODEM_IF:SC6500] <%s> BOOT_ON!!!\n", __func__);

	for (pin = 0; pin < ARRAY_SIZE(sprd6500_uart_on_table); pin++) {
			rc = gpio_tlmm_config(sprd6500_uart_on_table[pin],
					      GPIO_CFG_ENABLE);
		if (rc < 0)
			pr_err("[MODEM_IF:SC6500] %s: gpio_tlmm_config(%#x)=%d\n",
			       __func__, sprd6500_uart_on_table[pin], rc);
	}

	// UART_SEL for UART download
	gpio_tlmm_config(GPIO_CFG(mc->gpio_uart_sel, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		GPIO_CFG_ENABLE);
	gpio_set_value(mc->gpio_uart_sel, 1);

	return 0;
}

static int sprd6500_boot_off(struct modem_ctl *mc)
{
	int pin, rc = 0;

	pr_info("[MODEM_IF:SC6500] <%s>\n", __func__);

	// UART_SEL for BT
	gpio_tlmm_config(GPIO_CFG(mc->gpio_uart_sel, GPIOMUX_FUNC_GPIO,
		GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		GPIO_CFG_ENABLE);
	gpio_set_value(mc->gpio_uart_sel, 0);

	for (pin = 0; pin < ARRAY_SIZE(sprd6500_uart_off_table); pin++) {
			rc = gpio_tlmm_config(sprd6500_uart_off_table[pin],
					      GPIO_CFG_ENABLE);
		if (rc < 0)
			pr_err("[MODEM_IF:SC6500] %s: gpio_tlmm_config(%#x)=%d\n",
			       __func__, sprd6500_uart_off_table[pin], rc);
	}

	gpio_set_value(GPIO_BT_UART_RTS, 0);
	gpio_set_value(GPIO_BT_UART_CTS, 0);
	gpio_set_value(GPIO_BT_UART_RXD, 0);
	gpio_set_value(GPIO_BT_UART_TXD, 0);

//	mc->iod->modem_state_changed(mc->iod, STATE_OFFLINE);

	return 0;
}

static irqreturn_t phone_active_irq_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	int phone_active = 0;
	int phone_state = 0;
	int cp_dump_int = 0;
	int phone_reset = 0;

	if (!mc->gpio_phone_active) { /* || !mc->gpio_cp_dump_int) { */
		pr_err("[MODEM_IF:SC6500] no gpio data\n");
		return IRQ_HANDLED;
	}

	phone_active = gpio_get_value(mc->gpio_phone_active);

	pr_info("[MODEM_IF:SC6500] <%s> phone_active=%d, cp_dump_int=%d\n",
		__func__, phone_active, cp_dump_int);

	phone_reset = 1;

	if (phone_reset && phone_active) {
		phone_state = STATE_ONLINE;

		if (mc->iod && mc->iod->modem_state_changed)	{
			struct spi_link_device *spi_ld =
				to_spi_link_device(get_current_link(mc->iod));

			mc->iod->modem_state_changed(mc->iod, phone_state);
		
			/* Do after PHONE_ACTIVE High */
//			spi_ld->dpram_init_status = DPRAM_INIT_STATE_READY;
			spi_ld->spi_state = SPI_STATE_IDLE;

//			spi_ld->cmd_phone_start_handler(dpram_ld);
//			complete_all(&spi_ld->dpram_init_cmd);
		}

	} else if (phone_reset && !phone_active) {

		if (mc->phone_state == STATE_ONLINE) {
			phone_state = STATE_CRASH_EXIT;
			if (mc->iod && mc->iod->modem_state_changed)
				mc->iod->modem_state_changed(mc->iod,
							     phone_state);
		}
	} else {
		phone_state = STATE_OFFLINE;
		if (mc->iod && mc->iod->modem_state_changed)
			mc->iod->modem_state_changed(mc->iod, phone_state);
	}

	if (phone_active)
		irq_set_irq_type(mc->irq_phone_active, IRQ_TYPE_LEVEL_LOW);
	else
		irq_set_irq_type(mc->irq_phone_active, IRQ_TYPE_LEVEL_HIGH);

	pr_info("[MODEM_IF::SC6500] <%s> phone_state = %d\n",
			__func__, phone_state);

	return IRQ_HANDLED;
}

#if defined(CONFIG_SIM_DETECT)
static irqreturn_t sim_detect_irq_handler(int irq, void *_mc)
{
	struct modem_ctl *mc = (struct modem_ctl *)_mc;

	pr_info("[MODEM_IF:SC6500] <%s> gpio_sim_detect = %d\n",
		__func__, gpio_get_value(mc->gpio_sim_detect));

	if (mc->iod && mc->iod->sim_state_changed)
		mc->iod->sim_state_changed(mc->iod,
		!gpio_get_value(mc->gpio_sim_detect));

	return IRQ_HANDLED;
}
#endif

static void sprd6500_get_ops(struct modem_ctl *mc)
{
	pr_err("[MODEM_IF:SC6500] <%s> start\n", __func__);

	mc->ops.modem_on = sprd6500_on;
	mc->ops.modem_off = sprd6500_off;
	mc->ops.modem_reset = sprd6500_reset;
	mc->ops.modem_boot_on = sprd6500_boot_on;
	mc->ops.modem_boot_off = sprd6500_boot_off;
}

int sprd6500_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	int ret = 0;
	struct platform_device *pdev;

	pr_err("[MODEM_IF:SC6500] <%s> start\n", __func__);
	
	pdev = to_platform_device(mc->dev);

#if 0 //def CONFIG_OF
	sprd6500_modem_cfg_gpio(pdev);
#endif

	mc->gpio_cp_on = pdata->gpio_cp_on;
	mc->gpio_pda_active = pdata->gpio_pda_active;
	mc->gpio_phone_active = pdata->gpio_phone_active;
	mc->gpio_cp_dump_int = pdata->gpio_cp_dump_int;
	mc->gpio_sim_detect = pdata->gpio_sim_detect;

	mc->gpio_ap_cp_int1 = pdata->gpio_ap_cp_int1;
	mc->gpio_ap_cp_int2 = pdata->gpio_ap_cp_int2;
	mc->gpio_uart_sel = pdata->gpio_uart_sel;
#ifdef CONFIG_SEC_DUAL_MODEM_MODE
	mc->gpio_sim_sel = pdata->gpio_sim_sel;
#endif
	
#if defined(CONFIG_LINK_DEVICE_PLD)
	mc->gpio_fpga1_cs_n = pdata->gpio_fpga1_cs_n;
#endif
	gpio_set_value(mc->gpio_cp_on, 0);

	mc->irq_phone_active = platform_get_irq_byname(pdev, "cp_active_irq");
	pr_info("[MODEM_IF:SC6500] <%s> PHONE_ACTIVE IRQ# = %d\n",
		__func__, mc->irq_phone_active);

	sprd6500_get_ops(mc);

	if (mc->irq_phone_active) {
		ret = request_irq(mc->irq_phone_active,
				  phone_active_irq_handler,
				  IRQF_TRIGGER_HIGH,
				  "esc_active",
				  mc);
		if (ret) {
			pr_err("[MODEM_IF:SC6500] <%s> failed to request_irq IRQ# %d (err=%d)\n",
				__func__, mc->irq_phone_active, ret);
			dump_stack();
			return ret;
		}

#if 1 // don't enable wake option in temp
		enable_irq(mc->irq_phone_active);
#else
		ret = enable_irq_wake(mc->irq_phone_active);
		if (ret) {
			pr_err("[MODEM_IF:SC6500] %s: failed to enable_irq_wake IRQ# %d (err=%d)\n",
				__func__, mc->irq_phone_active, ret);
			free_irq(mc->irq_phone_active, mc);
			return ret;
		}
#endif
	}

#if defined(CONFIG_SIM_DETECT)
	mc->irq_sim_detect = platform_get_irq_byname(pdev, "sim_irq");
	pr_info("[MODEM_IF:SC6500] <%s> SIM_DECTCT IRQ# = %d\n",
		__func__, mc->irq_sim_detect);

	if (mc->irq_sim_detect) {
		ret = request_irq(mc->irq_sim_detect, sim_detect_irq_handler,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			"esc_sim_detect", mc);
		if (ret) {
			mif_err("failed to request_irq: %d\n", ret);
				mc->sim_state.online = false;
				mc->sim_state.changed = false;
			return ret;
		}

		ret = enable_irq_wake(mc->irq_sim_detect);
		if (ret) {
			mif_err("failed to enable_irq_wake: %d\n", ret);
			free_irq(mc->irq_sim_detect, mc);
			mc->sim_state.online = false;
			mc->sim_state.changed = false;
			return ret;
		}

		/* initialize sim_state => insert: gpio=0, remove: gpio=1 */
		mc->sim_state.online = !gpio_get_value(mc->gpio_sim_detect);
	}
#endif

	return ret;
}
