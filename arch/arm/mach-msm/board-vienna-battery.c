/*
 * Copyright (C) 2012 Samsung Electronics, Inc.
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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/memory.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/krait-regulator.h>
#include <linux/msm_thermal.h>
#include <asm/mach/map.h>
#include <asm/hardware/gic.h>
#include <asm/mach/map.h>
#include <asm/mach/arch.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <mach/msm_iomap.h>
#include <mach/irqs.h>
#ifdef CONFIG_ION_MSM
#include <mach/ion.h>
#endif
#include <mach/msm_memtypes.h>
#include <mach/msm_smd.h>
#include <mach/restart.h>
#include <mach/rpm-smd.h>
#include <mach/rpm-regulator-smd.h>
#include <mach/socinfo.h>
#include <mach/msm_bus_board.h>
#include <linux/i2c-gpio.h>
#include <linux/i2c.h>
#if defined(CONFIG_SENSORS_QPNP_ADC_VOLTAGE)
#include <linux/qpnp/qpnp-adc.h>
#endif

#if defined(CONFIG_BATTERY_SAMSUNG)
#include <linux/battery/sec_battery.h>
#include <linux/battery/sec_fuelgauge.h>
#include <linux/battery/sec_charger.h>
#include <linux/battery/sec_charging_common.h>
#define SEC_BATTERY_PMIC_NAME ""

#define GPIO_FUELGAUGE_I2C_SDA		87
#define GPIO_FUELGAUGE_I2C_SCL		88
#define GPIO_FUEL_INT				26
#define GPIO_TA_nCHG				85
#define GPIO_IF_CON_SENSE_18		76
/*#define GPIO_TA_INT					29*/

#define MSM_FUELGAUGE_I2C_BUS_ID	19

static unsigned int sec_bat_recovery_mode;
static sec_charging_current_t charging_current_table[] = {
	{2000,	2000,	300,	40*60},
	{0,	0,	0,	0},
	{500,	500,	300,	40*60},
	{2000,	2000,	300,	40*60},
	{500,	500,	300,	40*60},
	{500,	500,	300,	40*60},
	{1000,	1000,	300,	40*60},
	{500,	500,	300,	40*60},
	{2000,	2000,	300,	40*60},
	{2000,	2000,	300,	40*60},/*car dock*/
	{0,	0,	0,	0},
	{2000,	2000,	300,	40*60},
	{0,	-1,	0,	0},/*OTG: - value for chg current*/
	{0,	0,	0,	0},
};

extern int msm8974_get_cable_type(void);
int ta_int_gpio;

static bool sec_bat_adc_none_init(
		struct platform_device *pdev) {return true; }
static bool sec_bat_adc_none_exit(void) {return true; }
static int sec_bat_adc_none_read(unsigned int channel) {return 0; }

static bool sec_bat_adc_ap_init(
		struct platform_device *pdev) {return true; }
static bool sec_bat_adc_ap_exit(void) {return true; }
static int sec_bat_adc_ap_read(unsigned int channel)
{
#if defined(CONFIG_SENSORS_QPNP_ADC_VOLTAGE)
	int rc = -1, data =  -1;
	struct qpnp_vadc_result results;

	switch (channel) {
	case SEC_BAT_ADC_CHANNEL_TEMP:
		rc = qpnp_vadc_read(NULL, LR_MUX5_PU2_AMUX_THM2, &results);
		if (rc) {
			pr_err("%s: Unable to read batt temperature rc=%d\n",
				__func__, rc);
			return 0;
		}
		data = results.adc_code;
		break;
	case SEC_BAT_ADC_CHANNEL_TEMP_AMBIENT:
		data = 33000;
		break;
	case SEC_BAT_ADC_CHANNEL_BAT_CHECK:
		break;
	default:
		break;
	}

	return data;
#else
	return 33000;
#endif
}

static bool sec_bat_adc_ic_init(
		struct platform_device *pdev) {return true; }
static bool sec_bat_adc_ic_exit(void) {return true; }
static int sec_bat_adc_ic_read(unsigned int channel) {return 0; }

static bool sec_bat_gpio_init(void)
{
	return true;
}

static struct i2c_gpio_platform_data gpio_i2c_data_fgchg = {
	.sda_pin = GPIO_FUELGAUGE_I2C_SDA,
	.scl_pin = GPIO_FUELGAUGE_I2C_SCL,
};

static bool sec_fg_gpio_init(void)
{
	gpio_tlmm_config(GPIO_CFG(GPIO_FUEL_INT,  0, GPIO_CFG_INPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(gpio_i2c_data_fgchg.scl_pin, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(gpio_i2c_data_fgchg.sda_pin,  0,
				GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_set_value(gpio_i2c_data_fgchg.scl_pin, 1);
	gpio_set_value(gpio_i2c_data_fgchg.sda_pin, 1);

	return true;
}

static bool sec_chg_gpio_init(void)
{
	return true;
}

extern int poweroff_charging;
static bool sec_bat_is_lpm(void) {return (bool)poweroff_charging; }

int extended_cable_type;
extern int current_cable_type;

extern int mhl_connection_state(void);

static void sec_bat_initial_check(void)
{
	union power_supply_propval value;
	int cable_type = 0;

	cable_type = msm8974_get_cable_type();

	value.intval = cable_type;

	if (POWER_SUPPLY_TYPE_BATTERY <= cable_type) {
		value.intval = cable_type<<ONLINE_TYPE_MAIN_SHIFT;
		psy_do_property("battery", set,
				POWER_SUPPLY_PROP_ONLINE, value);
	}

	if (ta_int_gpio == 0) {
		pr_err("%s: ta_int_gpio is 0 or not assigned yet\n", __func__);
	} else {
		if (cable_type == POWER_SUPPLY_TYPE_BATTERY &&
			!gpio_get_value_cansleep(ta_int_gpio)) {
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
			if (!mhl_connection_state()) {
				value.intval = POWER_SUPPLY_TYPE_UARTOFF<<ONLINE_TYPE_MAIN_SHIFT;
				pr_info("%s : VBUS IN\n", __func__);
			}
#else
			value.intval = POWER_SUPPLY_TYPE_UARTOFF<<ONLINE_TYPE_MAIN_SHIFT;
			pr_info("%s : VBUS IN\n", __func__);
#endif
		}
		if (value.intval)
				psy_do_property("battery", set,
					POWER_SUPPLY_PROP_ONLINE, value);
	}
}

bool sec_bat_check_jig_status(void)
{
	if (gpio_get_value_cansleep(GPIO_IF_CON_SENSE_18))/*IF_CON_SENSE*/
		return true;
	else
		return false;
#if 0
	return (current_cable_type == POWER_SUPPLY_TYPE_UARTOFF);
#endif
}
static bool sec_bat_switch_to_check(void) {return true; }
static bool sec_bat_switch_to_normal(void) {return true; }

static bool sec_bat_is_interrupt_cable_check_possible(int extended_cable_type)
{
	return true;
}

int sec_bat_check_cable_callback(void)
{
	union power_supply_propval value;
	msleep(750);

	if (ta_int_gpio == 0) {
		pr_err("%s: ta_int_gpio is 0 or not assigned yet(cable_type(%d))\n",
			__func__, current_cable_type);
	}
#if defined(CONFIG_VIDEO_MHL_V1) || defined(CONFIG_VIDEO_MHL_V2)
	else if (!mhl_connection_state()) {
#else
	else {
#endif
		if (current_cable_type == POWER_SUPPLY_TYPE_BATTERY &&
			!gpio_get_value_cansleep(ta_int_gpio)) {
			pr_info("%s : VBUS IN\n", __func__);
			value.intval = POWER_SUPPLY_TYPE_UARTOFF<<ONLINE_TYPE_MAIN_SHIFT;
			psy_do_property("battery", set, POWER_SUPPLY_PROP_ONLINE, value);
			return POWER_SUPPLY_TYPE_UARTOFF;
		}

		if ((current_cable_type == POWER_SUPPLY_TYPE_UARTOFF ||
			current_cable_type == POWER_SUPPLY_TYPE_CARDOCK) &&
			gpio_get_value_cansleep(ta_int_gpio)) {
			pr_info("%s : VBUS OUT\n", __func__);
			value.intval = POWER_SUPPLY_TYPE_BATTERY<<ONLINE_TYPE_MAIN_SHIFT;
			psy_do_property("battery", set, POWER_SUPPLY_PROP_ONLINE, value);
			return POWER_SUPPLY_TYPE_BATTERY;
		}
	}

	return current_cable_type;
}

static int sec_bat_get_cable_from_extended_cable_type(
	int input_extended_cable_type)
{
	int cable_main, cable_sub, cable_power;
	int cable_type = POWER_SUPPLY_TYPE_UNKNOWN;
	union power_supply_propval value;
	int charge_current_max = 0, charge_current = 0;

	cable_main = GET_MAIN_CABLE_TYPE(input_extended_cable_type);
	if (cable_main != POWER_SUPPLY_TYPE_UNKNOWN)
		extended_cable_type = (extended_cable_type &
			~(int)ONLINE_TYPE_MAIN_MASK) |
			(cable_main << ONLINE_TYPE_MAIN_SHIFT);
	cable_sub = GET_SUB_CABLE_TYPE(input_extended_cable_type);
	if (cable_sub != ONLINE_SUB_TYPE_UNKNOWN)
		extended_cable_type = (extended_cable_type &
			~(int)ONLINE_TYPE_SUB_MASK) |
			(cable_sub << ONLINE_TYPE_SUB_SHIFT);
	cable_power = GET_POWER_CABLE_TYPE(input_extended_cable_type);
	if (cable_power != ONLINE_POWER_TYPE_UNKNOWN)
		extended_cable_type = (extended_cable_type &
			~(int)ONLINE_TYPE_PWR_MASK) |
			(cable_power << ONLINE_TYPE_PWR_SHIFT);

	switch (cable_main) {
	case POWER_SUPPLY_TYPE_CARDOCK:
		switch (cable_power) {
		case ONLINE_POWER_TYPE_BATTERY:
			cable_type = POWER_SUPPLY_TYPE_BATTERY;
			break;
		case ONLINE_POWER_TYPE_TA:
			switch (cable_sub) {
			case ONLINE_SUB_TYPE_MHL:
				cable_type = POWER_SUPPLY_TYPE_USB;
				break;
			case ONLINE_SUB_TYPE_AUDIO:
			case ONLINE_SUB_TYPE_DESK:
			case ONLINE_SUB_TYPE_SMART_NOTG:
			case ONLINE_SUB_TYPE_KBD:
				cable_type = POWER_SUPPLY_TYPE_MAINS;
				break;
			case ONLINE_SUB_TYPE_SMART_OTG:
				cable_type = POWER_SUPPLY_TYPE_CARDOCK;
				break;
			}
			break;
		case ONLINE_POWER_TYPE_USB:
			cable_type = POWER_SUPPLY_TYPE_USB;
			break;
		default:
			cable_type = current_cable_type;
			break;
		}
		break;
	case POWER_SUPPLY_TYPE_MISC:
		switch (cable_sub) {
		case ONLINE_SUB_TYPE_MHL:
			switch (cable_power) {
			case ONLINE_POWER_TYPE_BATTERY:
				cable_type = POWER_SUPPLY_TYPE_BATTERY;
				break;
			case ONLINE_POWER_TYPE_MHL_500:
				cable_type = POWER_SUPPLY_TYPE_MISC;
				charge_current_max = 400;
				charge_current = 400;
				break;
			case ONLINE_POWER_TYPE_MHL_900:
				cable_type = POWER_SUPPLY_TYPE_MISC;
				charge_current_max = 500;
				charge_current = 500;
				break;
			case ONLINE_POWER_TYPE_MHL_1500:
				cable_type = POWER_SUPPLY_TYPE_MISC;
				charge_current_max = 1300;
				charge_current = 1300;
				break;
			case ONLINE_POWER_TYPE_USB:
				cable_type = POWER_SUPPLY_TYPE_USB;
				charge_current_max = 150;
				charge_current = 500;
				break;
			default:
				cable_type = cable_main;
			}
			break;
		default:
			cable_type = cable_main;
			break;
		}
		break;
	default:
		cable_type = cable_main;
		break;
	}

	if (charge_current_max == 0) {
		charge_current_max =
			charging_current_table[cable_type].input_current_limit;
		charge_current =
			charging_current_table[cable_type].
			fast_charging_current;
	}
	value.intval = charge_current_max;
	psy_do_property(sec_battery_pdata.charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_MAX, value);
	value.intval = charge_current;
	psy_do_property(sec_battery_pdata.charger_name, set,
			POWER_SUPPLY_PROP_CURRENT_AVG, value);

	return cable_type;
}

static bool sec_bat_check_cable_result_callback(
				int cable_type)
{
	current_cable_type = cable_type;

	switch (cable_type) {
	case POWER_SUPPLY_TYPE_USB:
		pr_info("%s set vbus applied\n",
			__func__);
		break;

	case POWER_SUPPLY_TYPE_BATTERY:
		pr_info("%s set vbus cut\n",
			__func__);
		break;
	case POWER_SUPPLY_TYPE_MAINS:
		break;
	default:
		pr_err("%s cable type (%d)\n",
			__func__, cable_type);
		return false;
	}
	return true;
}

/* callback for battery check
 * return : bool
 * true - battery detected, false battery NOT detected
 */
static bool sec_bat_check_callback(void)
{
#if 0/*No need for Tablet*/
	struct power_supply *psy;
	union power_supply_propval value;

	psy = get_power_supply_by_name(("sec-charger"));
	if (!psy) {
		pr_err("%s: Fail to get psy (%s)\n",
			__func__, "sec-charger");
		value.intval = 1;
	} else {
		int ret;
		ret = psy->get_property(psy, POWER_SUPPLY_PROP_PRESENT, &(value));
		if (ret < 0) {
			pr_err("%s: Fail to sec-charger get_property (%d=>%d)\n",
				__func__, POWER_SUPPLY_PROP_PRESENT, ret);
			value.intval = 1;
		}
	}

	return value.intval;
#endif
	return true;
}
static bool sec_bat_check_result_callback(void) {return true; }

/* callback for OVP/UVLO check
 * return : int
 * battery health
 */
static int sec_bat_ovp_uvlo_callback(void)
{
	int health;
	health = POWER_SUPPLY_HEALTH_GOOD;

	return health;
}

static bool sec_bat_ovp_uvlo_result_callback(int health) {return true; }

/*
 * val.intval : temperature
 */
static bool sec_bat_get_temperature_callback(
		enum power_supply_property psp,
		union power_supply_propval *val) {return true; }
static bool sec_fg_fuelalert_process(bool is_fuel_alerted) {return true; }

/* ADC region should be exclusive */
static sec_bat_adc_region_t cable_adc_value_table[] = {
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
	{0,	0},
};

static int polling_time_table[] = {
	10,	/* BASIC */
	30,	/* CHARGING */
	30,	/* DISCHARGING */
	30,	/* NOT_CHARGING */
	60 * 60,	/* SLEEP */
};

/* for MAX17050 */
static struct battery_data_t samsung_battery_data[] = {
	/* SDI battery data */
	{
		.Capacity = 0x4A8E,
		.low_battery_comp_voltage = 3500,
		.low_battery_table = {
			/* range, slope, offset */
			{-5000,	0,	0},	/* dummy for top limit */
			{-1250, 0,	3320},
			{-750, 97,	3451},
			{-100, 96,	3461},
			{0, 0,	3456},
		},
		.temp_adjust_table = {
			/* range, slope, offset */
			{47000, 122,	8950},
			{60000, 200,	51000},
			{100000, 0,	0},	/* dummy for top limit */
		},
		.type_str = "SDI",
	}
};

/* temp, same with board-8974-sec.c */
#define IF_PMIC_IRQ_BASE	353 /* temp val */

sec_battery_platform_data_t sec_battery_pdata = {
	/* NO NEED TO BE CHANGED */
	.initial_check = sec_bat_initial_check,
	.bat_gpio_init = sec_bat_gpio_init,
	.fg_gpio_init = sec_fg_gpio_init,
	.chg_gpio_init = sec_chg_gpio_init,

	.is_lpm = sec_bat_is_lpm,
	.jig_irq = 0,
	.jig_irq_attr =
		IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
	.check_jig_status = sec_bat_check_jig_status,
	.is_interrupt_cable_check_possible =
		sec_bat_is_interrupt_cable_check_possible,
	.check_cable_callback =
		sec_bat_check_cable_callback,
	.get_cable_from_extended_cable_type =
		sec_bat_get_cable_from_extended_cable_type,
	.cable_switch_check = sec_bat_switch_to_check,
	.cable_switch_normal = sec_bat_switch_to_normal,
	.check_cable_result_callback =
		sec_bat_check_cable_result_callback,
	.check_battery_callback =
		sec_bat_check_callback,
	.check_battery_result_callback =
		sec_bat_check_result_callback,
	.ovp_uvlo_callback = sec_bat_ovp_uvlo_callback,
	.ovp_uvlo_result_callback =
		sec_bat_ovp_uvlo_result_callback,
	.fuelalert_process = sec_fg_fuelalert_process,
	.get_temperature_callback =
		sec_bat_get_temperature_callback,

	.adc_api[SEC_BATTERY_ADC_TYPE_NONE] = {
		.init = sec_bat_adc_none_init,
		.exit = sec_bat_adc_none_exit,
		.read = sec_bat_adc_none_read
		},
	.adc_api[SEC_BATTERY_ADC_TYPE_AP] = {
		.init = sec_bat_adc_ap_init,
		.exit = sec_bat_adc_ap_exit,
		.read = sec_bat_adc_ap_read
		},
	.adc_api[SEC_BATTERY_ADC_TYPE_IC] = {
		.init = sec_bat_adc_ic_init,
		.exit = sec_bat_adc_ic_exit,
		.read = sec_bat_adc_ic_read
		},
	.cable_adc_value = cable_adc_value_table,
	.charging_current = charging_current_table,
	.polling_time = polling_time_table,
	/* NO NEED TO BE CHANGED */

	.pmic_name = SEC_BATTERY_PMIC_NAME,

	.adc_check_count = 6,
	.adc_type = {
		SEC_BATTERY_ADC_TYPE_NONE,	/* CABLE_CHECK */
		SEC_BATTERY_ADC_TYPE_NONE,	/* BAT_CHECK */
		SEC_BATTERY_ADC_TYPE_NONE,	/* TEMP */
		SEC_BATTERY_ADC_TYPE_NONE,	/* TEMP_AMB */
		SEC_BATTERY_ADC_TYPE_AP,	/* FULL_CHECK */
	},

	/* Battery */
	.vendor = "SDI SDI",
	.technology = POWER_SUPPLY_TECHNOLOGY_LION,
	.battery_data = (void *)samsung_battery_data,
	.bat_gpio_ta_nconnected = 0,
	.bat_polarity_ta_nconnected = 0,
	.bat_irq = 0,
	.bat_irq_attr = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	.cable_check_type =
		SEC_BATTERY_CABLE_CHECK_INT |
		SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE |
		SEC_BATTERY_CABLE_CHECK_PSY,
	.cable_source_type =
		SEC_BATTERY_CABLE_SOURCE_EXTERNAL |
		SEC_BATTERY_CABLE_SOURCE_EXTENDED |
		SEC_BATTERY_CABLE_SOURCE_CALLBACK,

	.event_check = false,
	.event_waiting_time = 600,

	/* Monitor setting */
	.polling_type = SEC_BATTERY_MONITOR_ALARM,
	.monitor_initial_count = 3,

	/* Battery check */
	.battery_check_type = SEC_BATTERY_CHECK_NONE,
	.check_count = 0,
	/* Battery check by ADC */
	.check_adc_max = 0,
	.check_adc_min = 0,

	/* OVP/UVLO check */
	.ovp_uvlo_check_type = SEC_BATTERY_OVP_UVLO_CHGPOLLING,

	/* Temperature check */
	.thermal_source = SEC_BATTERY_THERMAL_SOURCE_FG,

	.temp_check_type = SEC_BATTERY_TEMP_CHECK_TEMP,
	.temp_check_count = 1,
	.temp_high_threshold_event = 700,
	.temp_high_recovery_event = 420,
	.temp_low_threshold_event = -50,
	.temp_low_recovery_event = 0,
	.temp_high_threshold_normal = 700,
	.temp_high_recovery_normal = 420,
	.temp_low_threshold_normal = -50,
	.temp_low_recovery_normal = 0,
	.temp_high_threshold_lpm = 700,
	.temp_high_recovery_lpm = 420,
	.temp_low_threshold_lpm = -50,
	.temp_low_recovery_lpm = 0,

	.full_check_type = SEC_BATTERY_FULLCHARGED_FG_CURRENT,
	.full_check_type_2nd = SEC_BATTERY_FULLCHARGED_TIME,
	.full_check_count = 1,
	.chg_gpio_full_check = GPIO_TA_nCHG,
	.chg_polarity_full_check = 0,
	.full_condition_type = SEC_BATTERY_FULL_CONDITION_SOC |
		SEC_BATTERY_FULL_CONDITION_NOTIMEFULL |
		SEC_BATTERY_FULL_CONDITION_VCELL,
	.full_condition_soc = 97,
	.full_condition_vcell = 4300,

	.recharge_check_count = 2,
	.recharge_condition_type =
		SEC_BATTERY_RECHARGE_CONDITION_VCELL |
		SEC_BATTERY_RECHARGE_CONDITION_AVGVCELL,
	.recharge_condition_soc = 98,
	.recharge_condition_avgvcell = 4150,
	.recharge_condition_vcell = 4250,

	.charging_total_time = 10 * 60 * 60,
	.recharging_total_time = 90 * 60,
	.charging_reset_time = 0,

	/* Fuel Gauge */
	.fg_irq = 0,
	.fg_irq_attr =
		IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
	.fuel_alert_soc = 1,
	.repeated_fuelalert = false,
	.capacity_calculation_type =
		SEC_FUELGAUGE_CAPACITY_TYPE_RAW |
		SEC_FUELGAUGE_CAPACITY_TYPE_SCALE |
		SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE,
		/* SEC_FUELGAUGE_CAPACITY_TYPE_ATOMIC, */
	.capacity_max = 1000,
	.capacity_max_margin = 50,
	.capacity_min = 0,

	/* Charger */
	.charger_name = "sec-charger",
	.chg_gpio_en = 0,
	.chg_polarity_en = 0,
	.chg_gpio_status = 0,
	.chg_polarity_status = 0,
	.chg_irq = 0,
	.chg_irq_attr = 0,
	.chg_float_voltage = 4360,
};

static struct platform_device sec_device_battery = {
	.name = "sec-battery",
	.id = -1,
	.dev.platform_data = &sec_battery_pdata,
};

struct platform_device sec_device_fgchg = {
	.name = "i2c-gpio",
	.id = MSM_FUELGAUGE_I2C_BUS_ID,
	.dev.platform_data = &gpio_i2c_data_fgchg,
};

static struct i2c_board_info sec_brdinfo_fgchg[] __initdata = {
	{
		I2C_BOARD_INFO("sec-charger",
				SEC_CHARGER_I2C_SLAVEADDR),
		.platform_data = &sec_battery_pdata,
	},
	{
		I2C_BOARD_INFO("sec-fuelgauge",
				SEC_FUELGAUGE_I2C_SLAVEADDR),
		.platform_data	= &sec_battery_pdata,
	},
};

static struct platform_device *samsung_battery_devices[] __initdata = {
	&sec_device_fgchg,
	&sec_device_battery,
};

static int __init sec_bat_current_boot_mode(char *mode)
{
	/*
	*	1 is recovery booting
	*	0 is normal booting
	*/

	if (strncmp(mode, "1", 1) == 0)
		sec_bat_recovery_mode = 1;
	else
		sec_bat_recovery_mode = 0;

	pr_info("%s : %s", __func__, sec_bat_recovery_mode == 1 ?
				"recovery" : "normal");

	return 1;
}
__setup("androidboot.boot_recovery=", sec_bat_current_boot_mode);

static void charger_gpio_init(void)
{
	/*gpio_tlmm_config(GPIO_CFG(GPIO_TA_INT, 0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);*/
	gpio_tlmm_config(GPIO_CFG(GPIO_TA_nCHG,	0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(GPIO_IF_CON_SENSE_18,	0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
}

void __init samsung_init_battery(void)
{
	pr_err("%s\n", __func__);
	charger_gpio_init();

	/* FUEL_SDA/SCL setting */
	platform_add_devices(
		samsung_battery_devices,
		ARRAY_SIZE(samsung_battery_devices));

	i2c_register_board_info(
		MSM_FUELGAUGE_I2C_BUS_ID,
		sec_brdinfo_fgchg,
		ARRAY_SIZE(sec_brdinfo_fgchg));
}

#endif
