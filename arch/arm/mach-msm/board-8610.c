/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/memory.h>
#include <linux/msm_tsens.h>
#include <asm/mach/map.h>
#include <asm/arch_timer.h>
#include <asm/hardware/gic.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <mach/msm_iomap.h>
#include <mach/restart.h>
#ifdef CONFIG_ION_MSM
#include <mach/ion.h>
#endif
#ifdef CONFIG_SEC_DEBUG
#include <mach/sec_debug.h>
#endif
#include <linux/regulator/qpnp-regulator.h>
#include <mach/msm_memtypes.h>
#include <mach/socinfo.h>
#include <mach/board.h>
#include <mach/clk-provider.h>
#include <mach/msm_smd.h>
#include <mach/rpm-smd.h>
#include <mach/rpm-regulator-smd.h>
#include <mach/msm_smem.h>
#include <linux/msm_thermal.h>
#include "board-dt.h"
#include "clock.h"
#include "platsmp.h"
#include "spm.h"
#include "pm.h"
#include "modem_notifier.h"
#include <linux/device.h>
#include <linux/export.h>

#if defined(CONFIG_SEC_THERMISTOR)
#include <mach/msm8x10-thermistor.h>
#endif

#ifdef CONFIG_PROC_AVC
#include <linux/proc_avc.h>
#endif

static struct memtype_reserve msm8610_reserve_table[] __initdata = {
	[MEMTYPE_SMI] = {
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

static int msm8610_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

static struct of_dev_auxdata msm8610_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("qcom,msm-sdcc", 0xF9824000, \
			"msm_sdcc.1", NULL),
	OF_DEV_AUXDATA("qcom,msm-sdcc", 0xF98A4000, \
			"msm_sdcc.2", NULL),
	OF_DEV_AUXDATA("qcom,sdhci-msm", 0xF9824900, \
			"msm_sdcc.1", NULL),
	OF_DEV_AUXDATA("qcom,sdhci-msm", 0xF98A4900, \
			"msm_sdcc.2", NULL),
	{}
};

static struct reserve_info msm8610_reserve_info __initdata = {
	.memtype_reserve_table = msm8610_reserve_table,
	.paddr_to_memtype = msm8610_paddr_to_memtype,
};

static void __init msm8610_early_memory(void)
{
	reserve_info = &msm8610_reserve_info;
	of_scan_flat_dt(dt_scan_for_memory_hole, msm8610_reserve_table);
}

static void __init msm8610_reserve(void)
{
	reserve_info = &msm8610_reserve_info;
	of_scan_flat_dt(dt_scan_for_memory_reserve, msm8610_reserve_table);
	msm_reserve();
}

void __init msm8610_add_drivers(void)
{
	msm_smem_init();
	msm_init_modem_notifier_list();
	msm_smd_init();
	msm_rpm_driver_init();
	msm_spm_device_init();
	msm_pm_sleep_status_init();
	rpm_regulator_smd_driver_init();
	qpnp_regulator_init();
	tsens_tm_init_driver();
	msm_thermal_device_init();

	if (of_board_is_rumi())
		msm_clock_init(&msm8610_rumi_clock_init_data);
	else
		msm_clock_init(&msm8610_clock_init_data);
#if defined(CONFIG_SEC_THERMISTOR)
	platform_device_register(&sec_device_thermistor);
#endif
}

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

static void samsung_sys_class_init(void)
{
	pr_info("samsung sys class init.\n");

	sec_class = class_create(THIS_MODULE, "sec");

	if (IS_ERR(sec_class)) {
		pr_err("Failed to create class(sec)!\n");
		return;
	}

	pr_info("samsung sys class end.\n");
};

#if defined(CONFIG_BATTERY_SAMSUNG)
extern void __init samsung_init_battery(void);
#endif

void __init msm8610_init(void)
{
	struct of_dev_auxdata *adata = msm8610_auxdata_lookup;

#ifdef CONFIG_SEC_DEBUG
	sec_debug_init();
#endif

#ifdef CONFIG_PROC_AVC
	sec_avc_log_init();
#endif

	if (socinfo_init() < 0)
		pr_err("%s: socinfo_init() failed\n", __func__);

	msm8610_init_gpiomux();
	board_dt_populate(adata);
	samsung_sys_class_init();
	msm8610_add_drivers();

}

static const char *msm8610_dt_match[] __initconst = {
	"qcom,msm8610",
	NULL
};

DT_MACHINE_START(MSM8610_DT, "Qualcomm MSM 8x10 / MSM 8x12 (Flattened Device Tree)")
	.map_io = msm_map_msm8610_io,
	.init_irq = msm_dt_init_irq,
	.init_machine = msm8610_init,
	.handle_irq = gic_handle_irq,
	.timer = &msm_dt_timer,
	.dt_compat = msm8610_dt_match,
	.restart = msm_restart,
	.reserve = msm8610_reserve,
	.init_very_early = msm8610_early_memory,
	.smp = &arm_smp_ops,
MACHINE_END
