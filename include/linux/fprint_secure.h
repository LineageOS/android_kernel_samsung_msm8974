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
 *
 */

#ifndef __FPRINT_SECURE_H
#define __FPRINT_SECURE_H

#if defined(CONFIG_SEC_K_PROJECT) || defined(CONFIG_SEC_ATLANTIC_PROJECT) \
	|| defined(CONFIG_SEC_S_PROJECT) || defined(CONFIG_SEC_PATEK_PROJECT) \
	|| defined(CONFIG_MACH_CHAGALL) || defined(CONFIG_MACH_KLIMT) \
	|| defined(CONFIG_SEC_HESTIA_PROJECT)
#ifdef CONFIG_SEC_FACTORY
#undef ENABLE_SENSORS_FPRINT_SECURE
#else
#define ENABLE_SENSORS_FPRINT_SECURE
#endif /* CONFIG_SEC_FACTORY */
#endif /* K_PROJECT or ATLANTICLTE_ATT or S_PROJECT or ATLANTIC3GEUR_OPEN or CHAGALL or KLIMT */

#if defined(CONFIG_SEC_ATLANTIC_PROJECT) || defined(CONFIG_SEC_HESTIA_PROJECT)
#define FP_SPI_FIRST	20
#define FP_SPI_CS		22
#define FP_SPI_LAST	23
#else /* for K or S or ATLANTIC or CHAGALL or KLIMT */
#define FP_SPI_FIRST	23
#define FP_SPI_CS		25
#define FP_SPI_LAST	26
#endif /* CONFIG_SEC_ATLANTIC_PROJECT */


#endif /* __FPRINT_SECURE_H */
