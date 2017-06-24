/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
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

struct boot_stats {
	uint32_t bootloader_start;
	uint32_t bootloader_end;
	uint32_t bootloader_display;
	uint32_t bootloader_load_kernel;
};

#ifdef CONFIG_MSM_BOOT_STATS
int boot_stats_init(void);

#ifdef CONFIG_SEC_BSP
extern uint32_t bootloader_start;
extern uint32_t bootloader_end;
extern uint32_t bootloader_display;
extern uint32_t bootloader_load_kernel;
extern unsigned int get_boot_stat_time(void);
#endif

#else
static inline int boot_stats_init(void) { return 0; }
#endif
