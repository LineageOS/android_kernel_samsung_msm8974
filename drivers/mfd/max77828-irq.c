/*
 * max77828-irq.c - Interrupt controller support for MAX77828
 *
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * SangYoung Son <hello.son@samsung.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This driver is based on max77828-irq.c
 */

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/mfd/max77828.h>
#include <linux/mfd/max77828-private.h>

static const u8 max77828_mask_reg[] = {
	[MUIC_INT1] = MAX77828_MUIC_REG_INTMASK1,
	[MUIC_INT2] = MAX77828_MUIC_REG_INTMASK2,
	[MUIC_INT3] = MAX77828_MUIC_REG_INTMASK3,
};

static struct i2c_client *get_i2c(struct max77828_dev *max77828,
				enum max77828_irq_source src)
{
	switch (src) {
	case MUIC_INT1 ... MUIC_INT3:
		return max77828->muic;
	default:
		return ERR_PTR(-EINVAL);
	}
}

struct max77828_irq_data {
	int mask;
	enum max77828_irq_source group;
};

#define DECLARE_IRQ(idx, _group, _mask)		\
	[(idx)] = { .group = (_group), .mask = (_mask) }
static const struct max77828_irq_data max77828_irqs[] = {
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT1_ADC,		MUIC_INT1, 1 << 0),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT1_ADCERR,	MUIC_INT1, 1 << 2),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT1_ADC1K,	MUIC_INT1, 1 << 3),

	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT2_CHGTYP,	MUIC_INT2, 1 << 0),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT2_CHGDETREUN,	MUIC_INT2, 1 << 1),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT2_DCDTMR,	MUIC_INT2, 1 << 2),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT2_DXOVP,	MUIC_INT2, 1 << 3),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT2_VBVOLT,	MUIC_INT2, 1 << 4),

	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT3_VBADC,	MUIC_INT3, 1 << 0),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT3_VDNMON,	MUIC_INT3, 1 << 1),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT3_DNRES,	MUIC_INT3, 1 << 2),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT3_MPNACK,	MUIC_INT3, 1 << 3),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT3_MRXBUFOW,	MUIC_INT3, 1 << 4),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT3_MRXTRF,	MUIC_INT3, 1 << 5),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT3_MRXPERR,	MUIC_INT3, 1 << 6),
	DECLARE_IRQ(MAX77828_MUIC_IRQ_INT3_MRXRDY,	MUIC_INT3, 1 << 7),
};

static void max77828_irq_lock(struct irq_data *data)
{
	struct max77828_dev *max77828 = irq_get_chip_data(data->irq);

	mutex_lock(&max77828->irqlock);
}

static void max77828_irq_sync_unlock(struct irq_data *data)
{
	struct max77828_dev *max77828 = irq_get_chip_data(data->irq);
	int i;

	for (i = 0; i < MAX77828_IRQ_GROUP_NR; i++) {
		u8 mask_reg = max77828_mask_reg[i];
		struct i2c_client *i2c = get_i2c(max77828, i);

		if (mask_reg == MAX77828_REG_INVALID ||
				IS_ERR_OR_NULL(i2c))
			continue;
		max77828->irq_masks_cache[i] = max77828->irq_masks_cur[i];

		max77828_write_reg(i2c, max77828_mask_reg[i],
				max77828->irq_masks_cur[i]);
	}

	mutex_unlock(&max77828->irqlock);
}

static const inline struct max77828_irq_data *
irq_to_max77828_irq(struct max77828_dev *max77828, int irq)
{
	return &max77828_irqs[irq - max77828->irq_base];
}

static void max77828_irq_mask(struct irq_data *data)
{
	struct max77828_dev *max77828 = irq_get_chip_data(data->irq);
	const struct max77828_irq_data *irq_data =
	    irq_to_max77828_irq(max77828, data->irq);

	if (irq_data->group >= MAX77828_IRQ_GROUP_NR)
		return;

	max77828->irq_masks_cur[irq_data->group] &= ~irq_data->mask;
}

static void max77828_irq_unmask(struct irq_data *data)
{
	struct max77828_dev *max77828 = irq_get_chip_data(data->irq);
	const struct max77828_irq_data *irq_data =
	    irq_to_max77828_irq(max77828, data->irq);

	if (irq_data->group >= MAX77828_IRQ_GROUP_NR)
		return;

	max77828->irq_masks_cur[irq_data->group] |= irq_data->mask;
}

static struct irq_chip max77828_irq_chip = {
	.name			= "max77828",
	.irq_bus_lock		= max77828_irq_lock,
	.irq_bus_sync_unlock	= max77828_irq_sync_unlock,
	.irq_mask		= max77828_irq_mask,
	.irq_unmask		= max77828_irq_unmask,
};

static irqreturn_t max77828_irq_thread(int irq, void *data)
{
	struct max77828_dev *max77828 = data;
	u8 irq_reg[MAX77828_IRQ_GROUP_NR] = {0};
	u8 tmp_irq_reg[MAX77828_IRQ_GROUP_NR] = {};
	int i;
	pr_debug("%s: irq gpio pre-state(0x%02x)\n", __func__,
		gpio_get_value(max77828->irq_gpio));

clear_retry:
	max77828_bulk_read(max77828->muic, MAX77828_MUIC_REG_INT1,
			MAX77828_NUM_IRQ_MUIC_REGS,
			&tmp_irq_reg[MUIC_INT1]);

	/* Or temp irq register to irq register for if it retries */
	for (i = MUIC_INT1; i < MAX77828_IRQ_GROUP_NR; i++)
		irq_reg[i] |= tmp_irq_reg[i];

	pr_info("%s: muic interrupt(0x%02x, 0x%02x, 0x%02x)\n",
		__func__, irq_reg[MUIC_INT1],
		irq_reg[MUIC_INT2], irq_reg[MUIC_INT3]);

	pr_debug("%s: irq gpio post-state(0x%02x)\n", __func__,
		gpio_get_value(max77828->irq_gpio));

	if (gpio_get_value(max77828->irq_gpio) == 0) {
		pr_warn("%s: irq_gpio is not High!\n", __func__);
		goto clear_retry;
	}

	/* Apply masking */
/*
	for (i = 0; i < MAX77828_IRQ_GROUP_NR; i++) {
			irq_reg[i] &= max77828->irq_masks_cur[i];
	}
*/
	/* Report */
	for (i = 0; i < MAX77828_IRQ_NR; i++) {
		if (irq_reg[max77828_irqs[i].group] & max77828_irqs[i].mask)
			handle_nested_irq(max77828->irq_base + i);
	}

	return IRQ_HANDLED;
}

int max77828_irq_resume(struct max77828_dev *max77828)
{
	int ret = 0;
	if (max77828->irq && max77828->irq_base)
		ret = max77828_irq_thread(max77828->irq_base, max77828);

	dev_info(max77828->dev, "%s: irq_resume ret=%d", __func__, ret);

	return ret >= 0 ? 0 : ret;
}

int max77828_irq_init(struct max77828_dev *max77828)
{
	int i;
	int cur_irq;
	int ret;

	pr_info("func: %s, irq_gpio: %d, irq_base: %d\n", __func__,
			max77828->irq_gpio, max77828->irq_base);
	if (!max77828->irq_gpio) {
		dev_warn(max77828->dev, "No interrupt specified.\n");
		max77828->irq_base = 0;
		return 0;
	}

	if (!max77828->irq_base) {
		dev_err(max77828->dev, "No interrupt base specified.\n");
		return 0;
	}

	mutex_init(&max77828->irqlock);

	max77828->irq = gpio_to_irq(max77828->irq_gpio);
	ret = gpio_request(max77828->irq_gpio, "if_pmic_irq");
	if (ret) {
		dev_err(max77828->dev, "%s: failed requesting gpio %d\n",
			__func__, max77828->irq_gpio);
		return ret;
	}
	gpio_direction_input(max77828->irq_gpio);
	gpio_free(max77828->irq_gpio);

	/* Mask individual interrupt sources */
	for (i = 0; i < MAX77828_IRQ_GROUP_NR; i++) {
		struct i2c_client *i2c;
		/* MUIC IRQ  0:MASK 1:NOT MASK */
		/* Other IRQ 1:MASK 0:NOT MASK */
		if (i >= MUIC_INT1 && i <= MUIC_INT3) {
			max77828->irq_masks_cur[i] = 0x00;
			max77828->irq_masks_cache[i] = 0x00;
		} else {
			max77828->irq_masks_cur[i] = 0xff;
			max77828->irq_masks_cache[i] = 0xff;
		}
		i2c = get_i2c(max77828, i);

		if (IS_ERR_OR_NULL(i2c))
			continue;
		if (max77828_mask_reg[i] == MAX77828_REG_INVALID)
			continue;
		if (i >= MUIC_INT1 && i <= MUIC_INT3)
			max77828_write_reg(i2c, max77828_mask_reg[i], 0x00);
		else
			max77828_write_reg(i2c, max77828_mask_reg[i], 0xff);
	}

	/* Register with genirq */
	for (i = 0; i < MAX77828_IRQ_NR; i++) {
		cur_irq = i + max77828->irq_base;
		irq_set_chip_data(cur_irq, max77828);
		irq_set_chip_and_handler(cur_irq, &max77828_irq_chip,
					 handle_edge_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(cur_irq);
#endif
	}

	ret = request_threaded_irq(max77828->irq, NULL, max77828_irq_thread,
				   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				   "max77828-irq", max77828);

	if (ret) {
		dev_err(max77828->dev, "Failed to request IRQ %d: %d\n",
			max77828->irq, ret);
		return ret;
	}

	return 0;
}

void max77828_irq_exit(struct max77828_dev *max77828)
{
	if (max77828->irq)
		free_irq(max77828->irq, max77828);
}
