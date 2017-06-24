/*
 *  sec_adc.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/battery/sec_adc.h>

static int current_chip_vendor;

static void sec_bat_adc_ap_init(struct platform_device *pdev)
{
	/*
	struct power_supply *psy_fuelgauge = get_power_supply_by_name("sec-fuelgauge");
	struct sec_fuelgauge_info *fuelgauge =
		container_of(psy_fuelgauge, struct sec_fuelgauge_info, psy_fg);
*/
	pr_info("%s: chip vendor %d\n", __func__, current_chip_vendor);

	switch (current_chip_vendor)
	{
	case VENDOR_LSI :
		break;
	case VENDOR_QCOM :
		break;
	default :
		break;
	}
}

static int sec_bat_adc_ap_read(void)
{
	int data = -1;
	int rc = -1;
	struct qpnp_vadc_result results;

	switch (current_chip_vendor)
	{
	case VENDOR_LSI :
		break;
	case VENDOR_QCOM :
#if defined(CONFIG_ARCH_MSM8974PRO)
			rc = qpnp_vadc_read(adc_client, LR_MUX5_PU1_AMUX_THM2, &results);
#else
			rc = qpnp_vadc_read(NULL, LR_MUX5_PU2_AMUX_THM2, &results);
#endif
			if (rc) {
				pr_err("%s: Unable to read batt temperature rc=%d\n",
					__func__, rc);
				return 0;
			}
			pr_err("%s: adc %d\n",
				__func__, results.adc_code);
			data = results.adc_code;
			break;
	default :
		break;
	}

	pr_info("%s: [%d] data = %d\n", __func__, current_chip_vendor, data);

	return data;
}

static void sec_bat_adc_ap_exit(void)
{
}

static void sec_bat_adc_none_init(struct platform_device *pdev)
{
}

static int sec_bat_adc_none_read(void)
{
	return 0;
}

static void sec_bat_adc_none_exit(void)
{
}

static void sec_bat_adc_ic_init(struct platform_device *pdev)
{
}

static int sec_bat_adc_ic_read(void)
{
	return 0;
}

static void sec_bat_adc_ic_exit(void)
{
}
static int adc_read_type(struct sec_battery_info *battery)
{
	int adc = 0;

	switch (battery->pdata->temp_adc_type)
	{
	case SEC_BATTERY_ADC_TYPE_NONE :
		adc = sec_bat_adc_none_read();
		break;
	case SEC_BATTERY_ADC_TYPE_AP :
		adc = sec_bat_adc_ap_read();
		break;
	case SEC_BATTERY_ADC_TYPE_IC :
		adc = sec_bat_adc_ic_read();
		break;
	case SEC_BATTERY_ADC_TYPE_NUM :
		break;
	default :
		break;
	}
	pr_info("[%s]ADC = %d\n", __func__, adc);
	return adc;
}

static void adc_init_type(struct platform_device *pdev,
			  struct sec_battery_info *battery)
{
	switch (battery->pdata->temp_adc_type)
	{
	case SEC_BATTERY_ADC_TYPE_NONE :
		sec_bat_adc_none_init(pdev);
		break;
	case SEC_BATTERY_ADC_TYPE_AP :
		sec_bat_adc_ap_init(pdev);
		break;
	case SEC_BATTERY_ADC_TYPE_IC :
		sec_bat_adc_ic_init(pdev);
		break;
	case SEC_BATTERY_ADC_TYPE_NUM :
		break;
	default :
		break;
	}
}

static void adc_exit_type(struct sec_battery_info *battery)
{
	switch (battery->pdata->temp_adc_type)
	{
	case SEC_BATTERY_ADC_TYPE_NONE :
		sec_bat_adc_none_exit();
		break;
	case SEC_BATTERY_ADC_TYPE_AP :
		sec_bat_adc_ap_exit();
		break;
	case SEC_BATTERY_ADC_TYPE_IC :
		sec_bat_adc_ic_exit();
		break;
	case SEC_BATTERY_ADC_TYPE_NUM :
		break;
	default :
		break;
	}
}

int adc_read(struct sec_battery_info *battery)
{
	int adc = 0;

	adc = adc_read_type(battery);

	pr_info("[%s]adc = %d\n", __func__, adc);

	return adc;
}

void adc_init(struct platform_device *pdev, struct sec_battery_info *battery)
{
	if (!battery->pdata->chip_vendor) {
		pr_err("%s: chip vendor is empty\n", __func__);
		return ;
	}

	pr_info("[%s]CHIP VENDOR = %s\n", __func__, battery->pdata->chip_vendor);

	if (!strcmp(battery->pdata->chip_vendor, "LSI"))
		current_chip_vendor = VENDOR_LSI;
	else if (!strcmp(battery->pdata->chip_vendor, "QCOM"))
		current_chip_vendor = VENDOR_QCOM;
	else
		current_chip_vendor = VENDOR_UNKNOWN;

	adc_init_type(pdev, battery);
}

void adc_exit(struct sec_battery_info *battery)
{
	adc_exit_type(battery);
}


