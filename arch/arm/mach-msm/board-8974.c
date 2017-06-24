/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/memory.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/krait-regulator.h>
#include <linux/msm_tsens.h>
#include <linux/msm_thermal.h>
#include <asm/mach/map.h>
#include <asm/hardware/gic.h>
#include <asm/mach/map.h>
#include <asm/mach/arch.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <mach/msm_iomap.h>
#ifdef CONFIG_ION_MSM
#include <mach/ion.h>
#endif
#include <mach/msm_memtypes.h>
#include <mach/msm_smd.h>
#include <mach/restart.h>
#include <mach/rpm-smd.h>
#ifdef CONFIG_SEC_DEBUG
#include <mach/sec_debug.h>
#endif
#include <mach/rpm-regulator-smd.h>
#include <mach/socinfo.h>
#include <mach/msm_smem.h>
#include <linux/module.h>

#include "board-dt.h"
#include "clock.h"
#include "devices.h"
#include "spm.h"
#include "pm.h"
#include "modem_notifier.h"
#include "platsmp.h"

#if defined(CONFIG_MOTOR_DRV_ISA1400)
        extern void vienna_motor_init(void);
#endif

#ifdef CONFIG_PROC_AVC
#include <linux/proc_avc.h>
#endif

#ifdef CONFIG_SEC_PM_DEBUG
extern int msm_show_resume_irq_mask;
#endif

#ifdef CONFIG_SEC_PM_DEBUG
extern int msm_show_resume_irq_mask;
#endif

#ifdef CONFIG_REGULATOR_MAX77826
#include <linux/regulator/max77826.h>
#endif

#ifdef CONFIG_LEDS_MAX77804K
#include <linux/leds-max77804k.h>
#endif

#ifdef CONFIG_LEDS_MAX77888
#include <linux/leds-max77888.h>
#endif

#ifdef CONFIG_LEDS_MAX77828
#include <linux/leds-max77828.h>
#include <linux/leds.h>
#endif

#include <linux/i2c.h>
#ifdef CONFIG_MFD_MAX77803
#ifdef CONFIG_LEDS_MAX77803
#include <linux/leds-max77803.h>
#endif
#endif

#ifdef CONFIG_SEC_THERMISTOR
#include <mach/sec_thermistor.h>
#include <mach/msm8974-thermistor.h>
#endif

#ifdef CONFIG_SEC_PATEK_PROJECT
#include "board-patek-keypad.c"
#endif

extern int poweroff_charging;

#ifdef CONFIG_SEC_S_PROJECT
static struct regulator *vsensor_1p8;
static int __init sensor_init(void)
{
	int ret;

	if(poweroff_charging)
		return 0;

	vsensor_1p8 = regulator_get(NULL, "8084_lvs1");
	if (IS_ERR(vsensor_1p8))
		pr_err("[SENSOR] could not get 8084_lvs1, %ld\n",
			PTR_ERR(vsensor_1p8));


	ret = regulator_enable(vsensor_1p8);
	if (ret)
		pr_err("%s: error enabling regulator 1p8\n", __func__);

	pr_info("%s: power on\n", __func__);
	return 0;
}
#endif

#ifdef CONFIG_SENSORS_SSP
static struct regulator *vsensor_2p85, *vsensor_1p8;
static int __init sensor_hub_init(void)
{
	int ret;

	if(poweroff_charging)
		return 0;

#ifdef CONFIG_ARCH_MSM8974PRO
	vsensor_2p85 = regulator_get(NULL, "8084_l18");
	if (IS_ERR(vsensor_2p85))
		pr_err("[SSP] could not get 8084_l18, %ld\n",
			PTR_ERR(vsensor_2p85));

	vsensor_1p8 = regulator_get(NULL, "8084_lvs1");
	if (IS_ERR(vsensor_1p8))
		pr_err("[SSP] could not get 8084_lvs1, %ld\n",
			PTR_ERR(vsensor_1p8));
#else

	vsensor_2p85 = regulator_get(NULL, "8941_l18");
	if (IS_ERR(vsensor_2p85))
		pr_err("[SSP] could not get 8941_l18, %ld\n",
			PTR_ERR(vsensor_2p85));

	vsensor_1p8 = regulator_get(NULL, "8941_lvs1");
	if (IS_ERR(vsensor_1p8))
		pr_err("[SSP] could not get 8941_lvs1, %ld\n",
			PTR_ERR(vsensor_1p8));
#endif

	ret = regulator_enable(vsensor_2p85);
	if (ret)
		pr_err("%s: error enabling regulator 2p85\n", __func__);

	ret = regulator_enable(vsensor_1p8);
	if (ret)
		pr_err("%s: error enabling regulator 1p8\n", __func__);

	pr_info("%s: power on\n", __func__);
	return 0;
}
#endif

#ifdef CONFIG_LEDS_MAX77804K
struct max77804k_led_platform_data max77804k_led_pdata = {
	.num_leds = 2,

	.leds[0].name = "leds-sec1",
	.leds[0].id = MAX77804K_FLASH_LED_1,
	.leds[0].timer = MAX77804K_FLASH_TIME_187P5MS,
	.leds[0].timer_mode = MAX77804K_TIMER_MODE_MAX_TIMER,
	.leds[0].cntrl_mode = MAX77804K_LED_CTRL_BY_FLASHSTB,
	.leds[0].brightness = 0x3D,

	.leds[1].name = "torch-sec1",
	.leds[1].id = MAX77804K_TORCH_LED_1,
	.leds[1].cntrl_mode = MAX77804K_LED_CTRL_BY_FLASHSTB,
	.leds[1].brightness = 0x06,
};
#endif

#ifdef CONFIG_LEDS_MAX77888
struct max77888_led_platform_data max77888_led_pdata = {
	.num_leds = 2,

	.leds[0].name = "leds-sec1",
	.leds[0].id = MAX77888_FLASH_LED_1,
	.leds[0].timer = MAX77888_FLASH_TIME_187P5MS,
	.leds[0].timer_mode = MAX77888_TIMER_MODE_MAX_TIMER,
	.leds[0].cntrl_mode = MAX77888_LED_CTRL_BY_FLASHSTB,
	.leds[0].brightness = 0x3D,

	.leds[1].name = "torch-sec1",
	.leds[1].id = MAX77888_TORCH_LED_1,
	.leds[1].cntrl_mode = MAX77888_LED_CTRL_BY_FLASHSTB,
	.leds[1].brightness = 0x06,
};
#endif

#ifdef CONFIG_LEDS_MAX77828
struct max77828_led_platform_data max77828_led_pdata = {
        .num_leds = 5,

        .leds[0].name = "leds-sec1",
	.leds[0].default_trigger = "flash",
        .leds[0].id = MAX77828_FLASH,
        .leds[0].brightness = MAX77828_FLASH_IOUT,

        .leds[1].name = "torch-sec1",
	.leds[1].default_trigger = "torch",
        .leds[1].id = MAX77828_TORCH,
        .leds[1].brightness = MAX77828_TORCH_IOUT,

        .leds[2].name = "led_r",
        .leds[2].id = MAX77828_RGB_R,
        .leds[2].brightness = (int)LED_OFF,
        .leds[2].max_brightness = MAX77828_LED_CURRENT,

        .leds[3].name = "led_g",
        .leds[3].id = MAX77828_RGB_G,
        .leds[3].brightness = (int)LED_OFF,
        .leds[3].max_brightness = MAX77828_LED_CURRENT,

        .leds[4].name = "led_b",
        .leds[4].id = MAX77828_RGB_B,
        .leds[4].brightness = (int)LED_OFF,
        .leds[4].max_brightness = MAX77828_LED_CURRENT,
};
#endif

#ifdef CONFIG_REGULATOR_MAX77826
#define MAX77826_I2C_BUS_ID	16

#define MAX77826_VREG_CONSUMERS(_id) \
	static struct regulator_consumer_supply max77826_vreg_consumers_##_id[]

#define MAX77826_VREG_INIT(_id, _min_uV, _max_uV, _always_on) \
	static struct regulator_init_data max77826_##_id##_init_data = { \
		.constraints = { \
			.min_uV			= _min_uV, \
			.max_uV			= _max_uV, \
			.apply_uV		= 1, \
			.always_on		= _always_on, \
			.valid_modes_mask = REGULATOR_MODE_NORMAL, \
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | \
							REGULATOR_CHANGE_STATUS, \
		}, \
		.num_consumer_supplies = ARRAY_SIZE(max77826_vreg_consumers_##_id), \
		.consumer_supplies = max77826_vreg_consumers_##_id, \
	}

#define MAX77826_VREG_INIT_DATA(_id) \
	(struct regulator_init_data *)&max77826_##_id##_init_data

MAX77826_VREG_CONSUMERS(LDO1) = {
	REGULATOR_SUPPLY("max77826_ldo1",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO2) = {
	REGULATOR_SUPPLY("max77826_ldo2",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO3) = {
	REGULATOR_SUPPLY("max77826_ldo3",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO4) = {
	REGULATOR_SUPPLY("max77826_ldo4",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO5) = {
	REGULATOR_SUPPLY("max77826_ldo5",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO6) = {
	REGULATOR_SUPPLY("max77826_ldo6",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO7) = {
	REGULATOR_SUPPLY("max77826_ldo7",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO8) = {
	REGULATOR_SUPPLY("max77826_ldo8",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO9) = {
	REGULATOR_SUPPLY("max77826_ldo9",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO10) = {
	REGULATOR_SUPPLY("max77826_ldo10",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO11) = {
	REGULATOR_SUPPLY("max77826_ldo11",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO12) = {
	REGULATOR_SUPPLY("max77826_ldo12",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO13) = {
	REGULATOR_SUPPLY("max77826_ldo13",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO14) = {
	REGULATOR_SUPPLY("max77826_ldo14",	NULL),
};

MAX77826_VREG_CONSUMERS(LDO15) = {
	REGULATOR_SUPPLY("max77826_ldo15",	NULL),
};

MAX77826_VREG_CONSUMERS(BUCK1) = {
	REGULATOR_SUPPLY("max77826_buck1",	NULL),
};

MAX77826_VREG_CONSUMERS(BUCK2) = {
	REGULATOR_SUPPLY("max77826_buck2",	NULL),
};
#if defined(CONFIG_SEC_S_PROJECT)
MAX77826_VREG_INIT(LDO1, 1100000, 1100000, 0); /* ES705 VDD_CORE 1.1V */
#else
MAX77826_VREG_INIT(LDO1, 1200000, 1200000, 0);
#endif
MAX77826_VREG_INIT(LDO2, 1000000, 1000000, 0);
#if defined(CONFIG_SEC_PATEK_PROJECT)
MAX77826_VREG_INIT(LDO3, 1800000, 1800000, 0);
MAX77826_VREG_INIT(LDO4, 1800000, 2950000, 0);
#else
MAX77826_VREG_INIT(LDO3, 1200000, 1200000, 0);
MAX77826_VREG_INIT(LDO4, 1800000, 1800000, 0);
#endif
#if defined(CONFIG_SEC_PATEK_PROJECT)
MAX77826_VREG_INIT(LDO5, 2800000, 2800000, 0);
#else
MAX77826_VREG_INIT(LDO5, 1800000, 1800000, 0);
#endif
MAX77826_VREG_INIT(LDO6, 1800000, 3300000, 0);
#if defined(CONFIG_SEC_PATEK_PROJECT)
MAX77826_VREG_INIT(LDO7, 2500000, 2500000, 0);
#else
MAX77826_VREG_INIT(LDO7, 1800000, 1800000, 0);
#endif
MAX77826_VREG_INIT(LDO8, 1800000, 3300000, 0);
#if defined(CONFIG_SEC_PATEK_PROJECT)
MAX77826_VREG_INIT(LDO9, 1800000, 1800000, 0);
MAX77826_VREG_INIT(LDO10, 2950000, 2950000, 0);
#elif defined(CONFIG_MACH_CHAGALL_KDI)
MAX77826_VREG_INIT(LDO9, 1000000, 3000000, 0);
MAX77826_VREG_INIT(LDO10, 2800000, 2950000, 0);
#else
MAX77826_VREG_INIT(LDO9, 1800000, 1800000, 0);
MAX77826_VREG_INIT(LDO10, 2800000, 2950000, 0);
#endif
#if defined(CONFIG_SEC_PATEK_PROJECT)
MAX77826_VREG_INIT(LDO11, 1800000, 3000000, 0);
#else
MAX77826_VREG_INIT(LDO11, 2700000, 2950000, 0);
#endif
MAX77826_VREG_INIT(LDO12, 2500000, 3300000, 0);
#if defined(CONFIG_SEC_PATEK_PROJECT)
MAX77826_VREG_INIT(LDO13, 3000000, 3000000, 0);
#else
MAX77826_VREG_INIT(LDO13, 3300000, 3300000, 0);
#endif
#if defined(CONFIG_SEC_S_PROJECT)
MAX77826_VREG_INIT(LDO14, 2800000, 2800000, 0); /* EAR MICBIAS 2.8V */
#elif defined(CONFIG_SEC_PATEK_PROJECT)
MAX77826_VREG_INIT(LDO14, 1800000, 3300000, 0);
#else
MAX77826_VREG_INIT(LDO14, 3300000, 3300000, 0);
#endif
MAX77826_VREG_INIT(LDO15, 1800000, 3300000, 0);
MAX77826_VREG_INIT(BUCK1, 1225000, 1225000, 0);
MAX77826_VREG_INIT(BUCK2, 3400000, 3400000, 0);

static struct max77826_regulator_subdev max77826_regulators[] = {
	{MAX77826_LDO1, MAX77826_VREG_INIT_DATA(LDO1)},
	{MAX77826_LDO2, MAX77826_VREG_INIT_DATA(LDO2)},
	{MAX77826_LDO3, MAX77826_VREG_INIT_DATA(LDO3)},
	{MAX77826_LDO4, MAX77826_VREG_INIT_DATA(LDO4)},
	{MAX77826_LDO5, MAX77826_VREG_INIT_DATA(LDO5)},
	{MAX77826_LDO6, MAX77826_VREG_INIT_DATA(LDO6)},
	{MAX77826_LDO7, MAX77826_VREG_INIT_DATA(LDO7)},
	{MAX77826_LDO8, MAX77826_VREG_INIT_DATA(LDO8)},
	{MAX77826_LDO9, MAX77826_VREG_INIT_DATA(LDO9)},
	{MAX77826_LDO10, MAX77826_VREG_INIT_DATA(LDO10)},
	{MAX77826_LDO11, MAX77826_VREG_INIT_DATA(LDO11)},
	{MAX77826_LDO12, MAX77826_VREG_INIT_DATA(LDO12)},
	{MAX77826_LDO13, MAX77826_VREG_INIT_DATA(LDO13)},
	{MAX77826_LDO14, MAX77826_VREG_INIT_DATA(LDO14)},
	{MAX77826_LDO15, MAX77826_VREG_INIT_DATA(LDO15)},
	{MAX77826_BUCK1, MAX77826_VREG_INIT_DATA(BUCK1)},
	{MAX77826_BUCK2, MAX77826_VREG_INIT_DATA(BUCK2)},
};

static struct max77826_platform_data max77826_pmic_pdata = {
	.name = "max77826",
	.num_regulators = ARRAY_SIZE(max77826_regulators),
	.regulators = max77826_regulators,
};

static struct i2c_board_info max77826_pmic_info[] __initdata = {
	{
		I2C_BOARD_INFO("max77826", 0x60),
		.platform_data = &max77826_pmic_pdata,
	},
};
#endif /* CONFIG_REGULATOR_MAX77826 */

static struct memtype_reserve msm8974_reserve_table[] __initdata = {
	[MEMTYPE_SMI] = {
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

static int msm8974_paddr_to_memtype(phys_addr_t paddr)
{
	return MEMTYPE_EBI1;
}

static struct reserve_info msm8974_reserve_info __initdata = {
	.memtype_reserve_table = msm8974_reserve_table,
	.paddr_to_memtype = msm8974_paddr_to_memtype,
};

void __init msm_8974_reserve(void)
{
	reserve_info = &msm8974_reserve_info;
	of_scan_flat_dt(dt_scan_for_memory_reserve, msm8974_reserve_table);
	msm_reserve();
}

static void __init msm8974_early_memory(void)
{
	reserve_info = &msm8974_reserve_info;
	of_scan_flat_dt(dt_scan_for_memory_hole, msm8974_reserve_table);
}

#ifdef CONFIG_MFD_MAX77803
#ifdef CONFIG_LEDS_MAX77803
 struct max77803_led_platform_data max77803_led_pdata = {
    .num_leds = 2,

    .leds[0].name = "leds-sec1",
    .leds[0].id = MAX77803_FLASH_LED_1,
    .leds[0].timer = MAX77803_FLASH_TIME_187P5MS,
    .leds[0].timer_mode = MAX77803_TIMER_MODE_MAX_TIMER,
    .leds[0].cntrl_mode = MAX77803_LED_CTRL_BY_FLASHSTB,
    .leds[0].brightness = 0x3D,

    .leds[1].name = "torch-sec1",
    .leds[1].id = MAX77803_TORCH_LED_1,
    .leds[1].cntrl_mode = MAX77803_LED_CTRL_BY_FLASHSTB,
    .leds[1].brightness = 0x06,
};
#endif
#endif



static struct platform_device *common_devices[] __initdata = {
#ifdef CONFIG_SEC_THERMISTOR
    &sec_device_thermistor,
#endif
};

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

static void samsung_sys_class_init(void)
{
	sec_class = class_create(THIS_MODULE, "sec");

	if (IS_ERR(sec_class)) {
		pr_err("Failed to create class(sec)!\n");
		return;
	}
};

/*
 * Used to satisfy dependencies for devices that need to be
 * run early or in a particular order. Most likely your device doesn't fall
 * into this category, and thus the driver should not be added here. The
 * EPROBE_DEFER can satisfy most dependency problems.
 */
void __init msm8974_add_drivers(void)
{
	msm_smem_init();
	msm_init_modem_notifier_list();
	msm_smd_init();
	msm_rpm_driver_init();
	msm_pm_sleep_status_init();
	rpm_regulator_smd_driver_init();
	msm_spm_device_init();
	krait_power_init();
	if (of_board_is_rumi())
		msm_clock_init(&msm8974_rumi_clock_init_data);
	else
		msm_clock_init(&msm8974_clock_init_data);
	tsens_tm_init_driver();
	msm_thermal_device_init();
}

static struct of_dev_auxdata msm_hsic_host_adata[] = {
	OF_DEV_AUXDATA("qcom,hsic-host", 0xF9A00000, "msm_hsic_host", NULL),
	{}
};

static struct of_dev_auxdata msm8974_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("qcom,hsusb-otg", 0xF9A55000, \
			"msm_otg", NULL),
	OF_DEV_AUXDATA("qcom,ehci-host", 0xF9A55000, \
			"msm_ehci_host", NULL),
	OF_DEV_AUXDATA("qcom,dwc-usb3-msm", 0xF9200000, \
			"msm_dwc3", NULL),
	OF_DEV_AUXDATA("qcom,usb-bam-msm", 0xF9304000, \
			"usb_bam", NULL),
	OF_DEV_AUXDATA("qcom,spi-qup-v2", 0xF9924000, \
			"spi_qsd.1", NULL),
	OF_DEV_AUXDATA("qcom,msm-sdcc", 0xF9824000, \
			"msm_sdcc.1", NULL),
	OF_DEV_AUXDATA("qcom,msm-sdcc", 0xF98A4000, \
			"msm_sdcc.2", NULL),
	OF_DEV_AUXDATA("qcom,msm-sdcc", 0xF9864000, \
			"msm_sdcc.3", NULL),
	OF_DEV_AUXDATA("qcom,msm-sdcc", 0xF98E4000, \
			"msm_sdcc.4", NULL),
	OF_DEV_AUXDATA("qcom,sdhci-msm", 0xF9824900, \
			"msm_sdcc.1", NULL),
	OF_DEV_AUXDATA("qcom,sdhci-msm", 0xF98A4900, \
			"msm_sdcc.2", NULL),
	OF_DEV_AUXDATA("qcom,sdhci-msm", 0xF9864900, \
			"msm_sdcc.3", NULL),
	OF_DEV_AUXDATA("qcom,sdhci-msm", 0xF98E4900, \
			"msm_sdcc.4", NULL),
	OF_DEV_AUXDATA("qcom,msm-rng", 0xF9BFF000, \
			"msm_rng", NULL),
	OF_DEV_AUXDATA("qcom,qseecom", 0xFE806000, \
			"qseecom", NULL),
	OF_DEV_AUXDATA("qcom,mdss_mdp", 0xFD900000, "mdp.0", NULL),
	OF_DEV_AUXDATA("qcom,msm-tsens", 0xFC4A8000, \
			"msm-tsens", NULL),
	OF_DEV_AUXDATA("qcom,qcedev", 0xFD440000, \
			"qcedev.0", NULL),
	OF_DEV_AUXDATA("qcom,hsic-host", 0xF9A00000, \
			"msm_hsic_host", NULL),
	OF_DEV_AUXDATA("qcom,hsic-smsc-hub", 0, "msm_smsc_hub",
			msm_hsic_host_adata),
	{}
};

static void __init msm8974_map_io(void)
{
	msm_map_8974_io();
}

void __init msm8974_init(void)
{
	struct of_dev_auxdata *adata = msm8974_auxdata_lookup;

#ifdef CONFIG_SEC_DEBUG
	sec_debug_init();
#endif

#ifdef CONFIG_PROC_AVC
	sec_avc_log_init();
#endif

	if (socinfo_init() < 0)
		pr_err("%s: socinfo_init() failed\n", __func__);

	samsung_sys_class_init();
	msm_8974_init_gpiomux();
	regulator_has_full_constraints();
	board_dt_populate(adata);
	msm8974_add_drivers();

	platform_add_devices(common_devices, ARRAY_SIZE(common_devices));

#if defined (CONFIG_MOTOR_DRV_ISA1400)
        vienna_motor_init();
#endif

#ifdef CONFIG_REGULATOR_MAX77826
	i2c_register_board_info(MAX77826_I2C_BUS_ID, max77826_pmic_info,
		ARRAY_SIZE(max77826_pmic_info));
#endif
#ifdef CONFIG_SEC_S_PROJECT
	sensor_init();
#endif
#ifdef CONFIG_SENSORS_SSP
	sensor_hub_init();
#endif

#ifdef CONFIG_SEC_PATEK_PROJECT
	platform_device_register(&folder_keypad_device);
#endif

#ifdef CONFIG_SEC_PM_DEBUG
	msm_show_resume_irq_mask = 1;
#endif

#if defined(CONFIG_BT_BCM4335) || defined(CONFIG_BT_BCM4339)
	msm8974_bt_init();
#endif

}

void __init msm8974_init_very_early(void)
{
	msm8974_early_memory();
}

static const char *msm8974_dt_match[] __initconst = {
	"qcom,msm8974",
	"qcom,apq8074",
	NULL
};

DT_MACHINE_START(MSM8974_DT, "Qualcomm MSM 8974 (Flattened Device Tree)")
	.map_io = msm8974_map_io,
	.init_irq = msm_dt_init_irq,
	.init_machine = msm8974_init,
	.handle_irq = gic_handle_irq,
	.timer = &msm_dt_timer,
	.dt_compat = msm8974_dt_match,
	.reserve = msm_8974_reserve,
	.init_very_early = msm8974_init_very_early,
	.restart = msm_restart,
	.smp = &msm8974_smp_ops,
MACHINE_END
