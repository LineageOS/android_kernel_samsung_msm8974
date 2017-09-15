/*
 * cyttsp4_platform.h
 * Cypress TrueTouch(TM) Standard Product V4 Platform Module.
 * For use with Cypress Txx4xx parts.
 * Supported parts include:
 * TMA4XX
 * TMA1036
 *
 * Copyright (C) 2013 Cypress Semiconductor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contact Cypress Semiconductor at www.cypress.com <ttdrivers@cypress.com>
 *
 */

#ifndef _LINUX_CYTTSP4_PLATFORM_H
#define _LINUX_CYTTSP4_PLATFORM_H

#include <linux/cyttsp4_core.h>
#if defined(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4) \
	|| defined(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_MODULE)
extern struct cyttsp4_loader_platform_data _cyttsp4_loader_platform_data;

int cyttsp4_xres(struct cyttsp4_core_platform_data *pdata,
		struct device *dev);
int cyttsp4_init(struct cyttsp4_core_platform_data *pdata,
		int on, struct device *dev);
int cyttsp4_power(struct cyttsp4_core_platform_data *pdata,
		int on, struct device *dev, atomic_t *ignore_irq);
int cyttsp4_irq_stat(struct cyttsp4_core_platform_data *pdata,
		struct device *dev);
#ifdef CYTTSP4_DETECT_HW
int cyttsp4_detect(struct cyttsp4_core_platform_data *pdata,
		struct device *dev, cyttsp4_platform_read read);
#else//CYTTSP4_DETECT_HW
#define cyttsp4_detect		NULL
#endif//CYTTSP4_DETECT_HW

#else /* !CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4 */
static struct cyttsp4_loader_platform_data _cyttsp4_loader_platform_data;
#define cyttsp4_xres		NULL
#define cyttsp4_init		NULL
#define cyttsp4_power		NULL
#define cyttsp4_irq_stat	NULL
#define cyttsp4_detect		NULL
#endif /* CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4 */

#if !defined(CONFIG_TOUCHSCREEN_CYPRESS_CYTTSP4_DEVICETREE_SUPPORT) &&\
    defined(CYTTSP4_PDATA_IN_PLATFORM_C)
extern struct cyttsp4_platform_data _cyttsp4_platform_data;
#endif

#endif /* _LINUX_CYTTSP4_PLATFORM_H */
