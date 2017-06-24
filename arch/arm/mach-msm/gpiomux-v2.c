/* Copyright (c) 2010-2011, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/bitops.h>
#include <linux/io.h>
#include <mach/msm_iomap.h>
#include <mach/gpiomux.h>

#define GPIO_CFG(n)    (MSM_TLMM_BASE + 0x1000 + (0x10 * n))
#define GPIO_IN_OUT(n) (MSM_TLMM_BASE + 0x1004 + (0x10 * n))

void __msm_gpiomux_write(unsigned gpio, struct gpiomux_setting val)
{
	uint32_t bits;

	bits = (val.drv << 6) | (val.func << 2) | val.pull;
	if (val.func == GPIOMUX_FUNC_GPIO) {
		bits |= val.dir > GPIOMUX_IN ? BIT(9) : 0;
		__raw_writel(val.dir == GPIOMUX_OUT_HIGH ? BIT(1) : 0,
			GPIO_IN_OUT(gpio));
	}
	__raw_writel(bits, GPIO_CFG(gpio));
	mb();
}

#if defined(CONFIG_ARCH_MSM8974) || defined(CONFIG_ARCH_MSM8610) || defined(CONFIG_ARCH_MSM8226)
void __msm_gpiomux_read(unsigned gpio, struct gpiomux_setting *val)
{
	uint32_t bits = readl_relaxed(GPIO_CFG(gpio));

	val->pull = bits & 0x3;
	val->func = (bits >> 2) & 0xf;
	val->drv = (bits >> 6) & 0x7;
	val->dir = bits & BIT_MASK(9) ? 1 : GPIOMUX_IN;

	if ((val->func == GPIOMUX_FUNC_GPIO) && (val->dir))
		val->dir = readl_relaxed(GPIO_IN_OUT(gpio)) & BIT_MASK(1) ?
					GPIOMUX_OUT_HIGH : GPIOMUX_OUT_LOW;
}
#endif

