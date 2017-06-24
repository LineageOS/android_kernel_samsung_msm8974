/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/spmi.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/radix-tree.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/qpnp/qpnp-adc.h>
#include <linux/power_supply.h>
#include <linux/bitops.h>
#include <linux/ratelimit.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/of_batterydata.h>
#include <linux/qpnp-revid.h>
#include <linux/android_alarm.h>

/* SAMSUNG charging specification */
#include <linux/android_alarm.h>
#if defined(CONFIG_USB_SWITCH_RT8973)
#include <linux/platform_data/rt8973.h>
#elif defined(CONFIG_SM5502_MUIC)
#include <linux/i2c/sm5502.h>
#elif defined(CONFIG_USB_SWITCH_FSA9485)
#include <linux/i2c/fsa9485.h>
#else
#include <linux/i2c/tsu6721.h>
#endif
#include <linux/mfd/pm8xxx/qpnp-sec-charger.h>

#ifdef SEC_CHARGER_CODE
/* flag to enable SEC charger debugging code */
//#define SEC_CHARGER_DEBUG

/* WARNING :This flag must be disabled at all times */
/* unit test for SEC BTM function */
//#define SEC_BTM_TEST

#ifdef SEC_BTM_TEST
static int sec_btm_temp = 250;
#endif
#endif

#if defined(CONFIG_USB_SWITCH_RT8973)
extern int rt_uart_connecting;
#elif defined(CONFIG_SM5502_MUIC)
#if defined(CONFIG_TORCH_FIX)
extern int factory_uart_connected(void);
#endif
extern int uart_sm5502_connecting;
#else
extern int uart_connecting;
#endif
extern bool bms_reset;
extern void bms_quickstart(void);

/* Interrupt offsets */
#define INT_RT_STS(base)			(base + 0x10)
#define INT_SET_TYPE(base)			(base + 0x11)
#define INT_POLARITY_HIGH(base)			(base + 0x12)
#define INT_POLARITY_LOW(base)			(base + 0x13)
#define INT_LATCHED_CLR(base)			(base + 0x14)
#define INT_EN_SET(base)			(base + 0x15)
#define INT_EN_CLR(base)			(base + 0x16)
#define INT_LATCHED_STS(base)			(base + 0x18)
#define INT_PENDING_STS(base)			(base + 0x19)
#define INT_MID_SEL(base)			(base + 0x1A)
#define INT_PRIORITY(base)			(base + 0x1B)

/* Peripheral register offsets */
#define CHGR_CHG_OPTION				0x08
#define CHGR_ATC_STATUS				0x0A
#define CHGR_VBAT_STATUS			0x0B
#define CHGR_IBAT_BMS				0x0C
#define CHGR_IBAT_STS				0x0D
#define CHGR_VDD_MAX				0x40
#define CHGR_VDD_SAFE				0x41
#define CHGR_VDD_MAX_STEP			0x42
#define CHGR_IBAT_MAX				0x44
#define CHGR_IBAT_SAFE				0x45
#define CHGR_VIN_MIN				0x47
#define CHGR_VIN_MIN_STEP			0x48
#define CHGR_CHG_CTRL				0x49
#define CHGR_CHG_FAILED				0x4A
#define CHGR_ATC_CTRL				0x4B
#define CHGR_ATC_FAILED				0x4C
#define CHGR_VBAT_TRKL				0x50
#define CHGR_VBAT_WEAK				0x52
#define CHGR_IBAT_ATC_A				0x54
#define CHGR_IBAT_ATC_B				0x55
#define CHGR_IBAT_TERM_CHGR			0x5B
#define CHGR_IBAT_TERM_BMS			0x5C
#define CHGR_VBAT_DET				0x5D
#define CHGR_TTRKL_MAX				0x5F
#define CHGR_TTRKL_MAX_EN			0x60
#define CHGR_TCHG_MAX				0x61
#define CHGR_CHG_WDOG_TIME			0x62
#define CHGR_CHG_WDOG_DLY			0x63
#define CHGR_CHG_WDOG_PET			0x64
#define CHGR_CHG_WDOG_EN			0x65
#define CHGR_IR_DROP_COMPEN			0x67
#define CHGR_I_MAX_REG				0x44
#define CHGR_USB_USB_SUSP			0x47
#define CHGR_USB_USB_OTG_CTL			0x48
#define CHGR_USB_ENUM_T_STOP			0x4E
#define CHGR_USB_TRIM				0xF1
#define CHGR_CHG_TEMP_THRESH			0x66
#define CHGR_BAT_IF_PRES_STATUS			0x08
#define CHGR_STATUS				0x09
#define CHGR_BAT_IF_VCP				0x42
#define CHGR_BAT_IF_BATFET_CTRL1		0x90
#define CHGR_BAT_IF_BATFET_CTRL4		0x93
#define CHGR_BAT_IF_SPARE			0xDF
#define CHGR_MISC_BOOT_DONE			0x42
#define CHGR_BUCK_PSTG_CTRL			0x73
#define CHGR_BUCK_COMPARATOR_OVRIDE_1		0xEB
#define CHGR_BUCK_COMPARATOR_OVRIDE_3		0xED
#define CHGR_BUCK_BCK_VBAT_REG_MODE		0x74
#define MISC_REVISION2				0x01
#define USB_OVP_CTL				0x42
#define USB_CHG_GONE_REV_BST			0xED
#define BUCK_VCHG_OV				0x77
#define BUCK_TEST_SMBC_MODES			0xE6
#define BUCK_CTRL_TRIM1				0xF1
#define SEC_ACCESS				0xD0
#define BAT_IF_VREF_BAT_THM_CTRL		0x4A
#define BAT_IF_BPD_CTRL				0x48
#define BOOST_VSET				0x41
#define BOOST_ENABLE_CONTROL			0x46
#define COMP_OVR1				0xEA
#define BAT_IF_BTC_CTRL				0x49
#define USB_OCP_THR				0x52
#define USB_OCP_CLR				0x53
#define BAT_IF_TEMP_STATUS			0x09

#define REG_OFFSET_PERP_SUBTYPE			0x05
/* SMBB peripheral subtype values */
#define SMBB_CHGR_SUBTYPE			0x01
#define SMBB_BUCK_SUBTYPE			0x02
#define SMBB_BAT_IF_SUBTYPE			0x03
#define SMBB_USB_CHGPTH_SUBTYPE			0x04
#define SMBB_DC_CHGPTH_SUBTYPE			0x05
#define SMBB_BOOST_SUBTYPE			0x06
#define SMBB_MISC_SUBTYPE			0x07

/* SMBB peripheral subtype values */
#define SMBBP_CHGR_SUBTYPE			0x31
#define SMBBP_BUCK_SUBTYPE			0x32
#define SMBBP_BAT_IF_SUBTYPE			0x33
#define SMBBP_USB_CHGPTH_SUBTYPE		0x34
#define SMBBP_BOOST_SUBTYPE			0x36
#define SMBBP_MISC_SUBTYPE			0x37

/* SMBCL peripheral subtype values */
#define SMBCL_CHGR_SUBTYPE			0x41
#define SMBCL_BUCK_SUBTYPE			0x42
#define SMBCL_BAT_IF_SUBTYPE			0x43
#define SMBCL_USB_CHGPTH_SUBTYPE		0x44
#define SMBCL_MISC_SUBTYPE			0x47

#define QPNP_CHARGER_DEV_NAME	"qcom,qpnp-charger"

/* Status bits and masks */
#define CHGR_BOOT_DONE			BIT(7)
#define CHGR_CHG_EN			BIT(7)
#define CHGR_ON_BAT_FORCE_BIT		BIT(0)
#define USB_VALID_DEB_20MS		0x03
#define BUCK_VBAT_REG_NODE_SEL_BIT	BIT(0)
#define VREF_BATT_THERM_FORCE_ON	0xC0
#define BAT_IF_BPD_CTRL_SEL		0x03
#define VREF_BAT_THM_ENABLED_FSM	0x80
#define REV_BST_DETECTED		BIT(0)
#define BAT_THM_EN			BIT(1)
#define BAT_ID_EN			BIT(0)
#define BOOST_PWR_EN			BIT(7)
#ifdef SEC_CHARGER_CODE
/* OVP : 7.0v - UVLO : 4.05v */
#define OVP_UVLO_THRESHOLD		0x33
#else
#define OVP_UVLO_THRESHOLD		0x3F
#endif
#define OCP_CLR_BIT			BIT(7)
#define OCP_THR_MASK			0x03
#define OCP_THR_900_MA			0x02
#define OCP_THR_500_MA			0x01
#define OCP_THR_200_MA			0x00

/* Interrupt definitions */
/* smbb_chg_interrupts */
#define CHG_DONE_IRQ			BIT(7)
#define CHG_FAILED_IRQ			BIT(6)
#define FAST_CHG_ON_IRQ			BIT(5)
#define TRKL_CHG_ON_IRQ			BIT(4)
#define STATE_CHANGE_ON_IR		BIT(3)
#define CHGWDDOG_IRQ			BIT(2)
#define VBAT_DET_HI_IRQ			BIT(1)
#define VBAT_DET_LOW_IRQ		BIT(0)

/* smbb_buck_interrupts */
#define VDD_LOOP_IRQ			BIT(6)
#define IBAT_LOOP_IRQ			BIT(5)
#define ICHG_LOOP_IRQ			BIT(4)
#define VCHG_LOOP_IRQ			BIT(3)
#define OVERTEMP_IRQ			BIT(2)
#define VREF_OV_IRQ			BIT(1)
#define VBAT_OV_IRQ			BIT(0)

/* smbb_bat_if_interrupts */
#define PSI_IRQ				BIT(4)
#define VCP_ON_IRQ			BIT(3)
#define BAT_FET_ON_IRQ			BIT(2)
#define BAT_TEMP_OK_IRQ			BIT(1)
#define BATT_PRES_IRQ			BIT(0)

/* smbb_usb_interrupts */
#define CHG_GONE_IRQ			BIT(2)
#define USBIN_VALID_IRQ			BIT(1)
#define COARSE_DET_USB_IRQ		BIT(0)

/* smbb_dc_interrupts */
#define DCIN_VALID_IRQ			BIT(1)
#define COARSE_DET_DC_IRQ		BIT(0)

/* smbb_boost_interrupts */
#define LIMIT_ERROR_IRQ			BIT(1)
#define BOOST_PWR_OK_IRQ		BIT(0)

/* smbb_misc_interrupts */
#define TFTWDOG_IRQ			BIT(0)

/* SMBB types */
#define SMBB				BIT(1)
#define SMBBP				BIT(2)
#define SMBCL				BIT(3)

/* Workaround flags */
#define CHG_FLAGS_VCP_WA		BIT(0)
#define BOOST_FLASH_WA			BIT(1)
#define POWER_STAGE_WA			BIT(2)

struct qpnp_chg_irq {
	int		irq;
	unsigned long		disabled;
};

struct qpnp_chg_regulator {
	struct regulator_desc			rdesc;
	struct regulator_dev			*rdev;
};

/**
 * struct qpnp_chg_chip - device information
 * @dev:			device pointer to access the parent
 * @spmi:			spmi pointer to access spmi information
 * @chgr_base:			charger peripheral base address
 * @buck_base:			buck peripheral base address
 * @bat_if_base:		battery interface peripheral base address
 * @usb_chgpth_base:		USB charge path peripheral base address
 * @dc_chgpth_base:		DC charge path peripheral base address
 * @boost_base:			boost peripheral base address
 * @misc_base:			misc peripheral base address
 * @freq_base:			freq peripheral base address
 * @bat_is_cool:		indicates that battery is cool
 * @bat_is_warm:		indicates that battery is warm
 * @chg_done:			indicates that charging is completed
 * @usb_present:		present status of usb
 * @dc_present:			present status of dc
 * @batt_present:		present status of battery
 * @use_default_batt_values:	flag to report default battery properties
 * @btc_disabled		Flag to disable btc (disables hot and cold irqs)
 * @max_voltage_mv:		the max volts the batt should be charged up to
 * @min_voltage_mv:		min battery voltage before turning the FET on
 * @max_bat_chg_current:	maximum battery charge current in mA
 * @warm_bat_chg_ma:	warm battery maximum charge current in mA
 * @cool_bat_chg_ma:	cool battery maximum charge current in mA
 * @warm_bat_mv:		warm temperature battery target voltage
 * @cool_bat_mv:		cool temperature battery target voltage
 * @resume_delta_mv:		voltage delta at which battery resumes charging
 * @term_current:		the charging based term current
 * @safe_current:		battery safety current setting
 * @maxinput_usb_ma:		Maximum Input current USB
 * @maxinput_dc_ma:		Maximum Input current DC
 * @hot_batt_p			Hot battery threshold setting
 * @cold_batt_p			Cold battery threshold setting
 * @warm_bat_decidegc		Warm battery temperature in degree Celsius
 * @cool_bat_decidegc		Cool battery temperature in degree Celsius
 * @revision:			PMIC revision
 * @type:			SMBB type
 * @tchg_mins			maximum allowed software initiated charge time
 * @thermal_levels		amount of thermal mitigation levels
 * @thermal_mitigation		thermal mitigation level values
 * @therm_lvl_sel		thermal mitigation level selection
 * @dc_psy			power supply to export information to userspace
 * @usb_psy			power supply to export information to userspace
 * @bms_psy			power supply to export information to userspace
 * @batt_psy:			power supply to export information to userspace
 * @ac_psy:			power supply to export information to userspace
 * @flags:			flags to activate specific workarounds
 *				throughout the driver
 *
 */
struct qpnp_chg_chip {
	struct device			*dev;
	struct spmi_device		*spmi;
	u16				chgr_base;
	u16				buck_base;
	u16				bat_if_base;
	u16				usb_chgpth_base;
	u16				dc_chgpth_base;
	u16				boost_base;
	u16				misc_base;
	u16				freq_base;
	struct qpnp_chg_irq		usbin_valid;
	struct qpnp_chg_irq		usb_ocp;
	struct qpnp_chg_irq		dcin_valid;
	struct qpnp_chg_irq		chg_gone;
	struct qpnp_chg_irq		chg_fastchg;
	struct qpnp_chg_irq		chg_trklchg;
	struct qpnp_chg_irq		chg_failed;
	struct qpnp_chg_irq		chg_vbatdet_lo;
	struct qpnp_chg_irq		batt_pres;
	struct qpnp_chg_irq		batt_temp_ok;
	bool				bat_is_cool;
	bool				bat_is_warm;
	bool				chg_done;
	bool				charger_monitor_checked;
	bool				usb_present;
	bool				dc_present;
	bool				batt_present;
	bool				charging_disabled;
	bool				btc_disabled;
	bool				use_default_batt_values;
	bool				duty_cycle_100p;
	unsigned int			bpd_detection;
	unsigned int			max_bat_chg_current;
	unsigned int			warm_bat_chg_ma;
	unsigned int			cool_bat_chg_ma;
	unsigned int			safe_voltage_mv;
	unsigned int			max_voltage_mv;
	unsigned int			min_voltage_mv;
	int				prev_usb_max_ma;
	int				set_vddmax_mv;
	int				delta_vddmax_mv;
	u8				trim_center;
	unsigned int			warm_bat_mv;
	unsigned int			cool_bat_mv;
	unsigned int			resume_delta_mv;
	int				term_current;
	int				ovp_gpio;

	int				soc_resume_limit;
	bool				resuming_charging;

	unsigned int			maxinput_usb_ma;
	unsigned int			maxinput_dc_ma;
	unsigned int			hot_batt_p;
	unsigned int			cold_batt_p;
	int				warm_bat_decidegc;
	int				cool_bat_decidegc;
	unsigned int			safe_current;
	unsigned int			revision;
	unsigned int			type;
	unsigned int			tchg_mins;
	unsigned int			thermal_levels;
	unsigned int			therm_lvl_sel;
	unsigned int			*thermal_mitigation;
	struct power_supply		dc_psy;
	struct power_supply		*usb_psy;
	struct power_supply		*bms_psy;
	struct power_supply		batt_psy;
	uint32_t			flags;
	struct qpnp_adc_tm_btm_param	adc_param;
	struct work_struct		adc_measure_work;
	struct work_struct		adc_disable_work;
	struct delayed_work		arb_stop_work;
	struct delayed_work		eoc_work;
	struct work_struct		soc_check_work;
	struct delayed_work		aicl_check_work;
	struct qpnp_chg_regulator       otg_vreg;
	struct qpnp_chg_regulator       boost_vreg;
	struct qpnp_chg_regulator	batfet_vreg;
	bool					batfet_ext_en;
	struct work_struct		batfet_lcl_work;
	struct qpnp_vadc_chip		*vadc_dev;
	struct qpnp_adc_tm_chip		*adc_tm_dev;
	struct mutex			jeita_configure_lock;
	struct mutex			batfet_vreg_lock;
	struct alarm			reduce_power_stage_alarm;
	struct work_struct		reduce_power_stage_work;
	bool				power_stage_workaround_running;

	/* SAMSUNG charging specification */
#ifdef SEC_CHARGER_CODE
	struct delayed_work             sec_bat_monitor_work;
#ifndef CONFIG_NOT_USE_EXT_OVP
	struct delayed_work             sec_bat_ext_ovp_work;		//back to Ext Ovp
#endif
	enum cable_type_t		cable_type;
	int				cable_exception;
	unsigned int			batt_status;
	int				recent_reported_soc;
	/* support for LPM mode charging */
	int				(*get_cable_type)(void);
	bool				(*get_lpm_mode)(void);
	/* polling time */
	bool                            is_in_sleep;
	unsigned int                    update_time;
	unsigned int                    sleep_update_time;
	unsigned int			polling_time;
	struct alarm			polling_alarm;
	ktime_t				last_update_time;
	/* charging and re-charging time management */
	unsigned long			charging_start_time;
        unsigned long			charging_passed_time;
        unsigned long			charging_term_time;
	unsigned int                    recharging_cnt;
	int				initial_count;
        bool				is_recharging;
        bool				is_chgtime_expired;
	/* OVP/UVLO state */
	int				ovp_uvlo_state;
	struct delayed_work		ovp_uvlo_work;
	bool				siop_enable;
	unsigned int			siop_level;
	/* Charge termination */
	bool				ui_full_chg;
	unsigned int			ui_full_cnt;
	/* battery health */
	unsigned int			health_cnt;
        unsigned int			batt_health;
	/* BMS */
	unsigned int                    capacity_max;
	unsigned int                    capacity_old;
	unsigned int                    capacity_raw;
        /* battery event handling */
	unsigned int			event;
	unsigned int			event_wait;
	struct				alarm event_termination_alarm;
	ktime_t				last_event_time;
	/* Battery temperature monitoring parameters */
        int                             temp_high_block;
        int                             temp_high_recover;
        int                             temp_low_block;
        int                             temp_low_recover;
	/* test modes : slate mode / factory mode */
	bool                            slate_mode;
	bool                            factory_mode;
	/* Wireless charging */
	int				wc_w_gpio;
        int				wc_w_state;
        int				wpc_acok;
	/* other */
	struct	wake_lock		monitor_wake_lock;
	struct	wake_lock		cable_wake_lock;
	/* SEC battery platform data*/
	struct	sec_battery_data	*batt_pdata;
	struct power_supply		ac_psy;
#endif
};

static struct of_device_id qpnp_charger_match_table[] = {
	{ .compatible = QPNP_CHARGER_DEV_NAME, },
	{}
};


#define BPD_MAX         3

enum bpd_type {
	BPD_TYPE_BAT_ID,
	BPD_TYPE_BAT_THM,
	BPD_TYPE_BAT_THM_BAT_ID,
};

static const char * const bpd_label[] = {
	[BPD_TYPE_BAT_ID] = "bpd_id",
	[BPD_TYPE_BAT_THM] = "bpd_thm",
	[BPD_TYPE_BAT_THM_BAT_ID] = "bpd_thm_id",
};

enum btc_type {
	HOT_THD_25_PCT = 25,
	HOT_THD_35_PCT = 35,
	COLD_THD_70_PCT = 70,
	COLD_THD_80_PCT = 80,
};

static u8 btc_value[] = {
	[HOT_THD_25_PCT] = 0x0,
	[HOT_THD_35_PCT] = BIT(0),
	[COLD_THD_70_PCT] = 0x0,
	[COLD_THD_80_PCT] = BIT(1),
};

static inline int
get_bpd(const char *name)
{
	int i = 0;
	for (i = 0; i < ARRAY_SIZE(bpd_label); i++) {
		if (strcmp(bpd_label[i], name) == 0)
			return i;
	}
	return -EINVAL;
}

int poweroff_charging;
static int sec_bat_is_lpm_check(char *str)
{
	if (strncmp(str, "charger", 7) == 0)
		poweroff_charging = 1;

	pr_info("%s: Low power charging mode: %d\n", __func__, poweroff_charging);

	return poweroff_charging;
}
__setup("androidboot.mode=", sec_bat_is_lpm_check);

/* SAMSUNG charging specification */
#ifdef SEC_CHARGER_CODE

/*
 * Function to get current cable type from muic
 *
 */
extern int msm8930_get_cable_status(void);


/*
 * Function to create SAMSUNG battery attributes
 *
 */
static int sec_bat_create_attrs(struct device *dev);


/*
 * Function to create SAMSUNG fuelgauge attributes
 *
 */
//static int sec_fg_create_attrs(struct device *dev);


/*
 * Function to program event alarm
 *
 */
static void sec_bat_event_program_alarm(struct qpnp_chg_chip *chip, int seconds);


/*
 * Function to check event timer expiry
 *
 */
static void sec_bat_event_expired_timer_func(struct alarm *alarm);


/*
 * SAMSUNG battery event handling function
 *
 */
static void sec_bat_event_set(struct qpnp_chg_chip *chip, int event, int enable);


/*
 * Function to handle charging and re-charging timer
 *
 */
static bool sec_chg_time_management(struct qpnp_chg_chip *chip);


/*
 * Function to handle cable insertion and removal
 * after MUIC detects cable insertion or removal
 */
static void sec_handle_cable_insertion_removal(struct qpnp_chg_chip *chip);


/*
 * Function to monitor battery status, health, charging time
 * charge termination etc
 */
static void sec_bat_monitor(struct work_struct *work);


/*
 * Function to stop charging with time management
 *
 */
static void sec_pm8226_stop_charging(struct qpnp_chg_chip *chip);


/*
 * Function to start charging with time management
 *
 */
static void sec_pm8226_start_charging(struct qpnp_chg_chip *chip);


/*
 * Function to check LPM charging mode
 *
 */
bool sec_bat_is_lpm(void);


/*
 * Function to check charger type in LPM charging mode
 *
 */
int sec_bat_get_cable_status(void);


/*
 * Function to check OVP/UVLO state and update
 * status accordingly
 */
static void sec_ovp_uvlo_worker(struct work_struct *work);

#if 0  //move to qpnp-bms.c
/*
 * Function to calcute the scaled capacity of SOC
 *
 */
static int sec_fg_get_scaled_capacity(struct qpnp_chg_chip *chip, int raw_soc);

/*
 * Function to calcute dynamic scaled capacity of SOC
 *
 */
static int sec_fg_calculate_dynamic_scale(struct qpnp_chg_chip *chip, int raw_soc);

/*
 * get property function
 *
 */
 #endif
static int sec_ac_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val);
/*
 * Function for battery temperature monitoring and update
 * status accordingly
 */
static void sec_bat_temperature_monitor(struct qpnp_chg_chip *chip);


/*
 * Function to program battery monitor polling alarm
 *
 */
static void sec_bat_program_alarm(struct qpnp_chg_chip *chip, int polling_time);


/*
 * Function to be executed when battery alarm expires
 * status accordingly
 */
static void sec_bat_polling_alarm_expired(struct alarm *alarm);

#endif


static int qpnp_chg_vinmin_set(struct qpnp_chg_chip *chip, int voltage);


#ifdef SEC_CHARGER_CODE
/* SAMSUNG charging specification */
/* charge current */
static int chg_imax_ma;
static int chg_ibatmax_ma = 1000;
#endif


static int
qpnp_chg_read(struct qpnp_chg_chip *chip, u8 *val,
			u16 base, int count)
{
	int rc = 0;
	struct spmi_device *spmi = chip->spmi;

	if (base == 0) {
		pr_err("base cannot be zero base=0x%02x sid=0x%02x rc=%d\n",
			base, spmi->sid, rc);
		return -EINVAL;
	}

	rc = spmi_ext_register_readl(spmi->ctrl, spmi->sid, base, val, count);
	if (rc) {
		pr_err("SPMI read failed base=0x%02x sid=0x%02x rc=%d\n", base,
				spmi->sid, rc);
		return rc;
	}
	return 0;
}

static int
qpnp_chg_write(struct qpnp_chg_chip *chip, u8 *val,
			u16 base, int count)
{
	int rc = 0;
	struct spmi_device *spmi = chip->spmi;

	if (base == 0) {
		pr_err("base cannot be zero base=0x%02x sid=0x%02x rc=%d\n",
			base, spmi->sid, rc);
		return -EINVAL;
	}

	rc = spmi_ext_register_writel(spmi->ctrl, spmi->sid, base, val, count);
	if (rc) {
		pr_err("write failed base=0x%02x sid=0x%02x rc=%d\n",
			base, spmi->sid, rc);
		return rc;
	}

	return 0;
}

static int
qpnp_chg_masked_write(struct qpnp_chg_chip *chip, u16 base,
						u8 mask, u8 val, int count)
{
	int rc;
	u8 reg;

	rc = qpnp_chg_read(chip, &reg, base, count);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n", base, rc);
		return rc;
	}
	pr_debug("addr = 0x%x read 0x%x\n", base, reg);

	reg &= ~mask;
	reg |= val & mask;

	pr_debug("Writing 0x%x\n", reg);

	rc = qpnp_chg_write(chip, &reg, base, count);
	if (rc) {
		pr_err("spmi write failed: addr=%03X, rc=%d\n", base, rc);
		return rc;
	}

	return 0;
}

static void
qpnp_chg_enable_irq(struct qpnp_chg_irq *irq)
{
	if (__test_and_clear_bit(0, &irq->disabled)) {
		pr_debug("number = %d\n", irq->irq);
		enable_irq(irq->irq);
	}
}

static void
qpnp_chg_disable_irq(struct qpnp_chg_irq *irq)
{
	if (!__test_and_set_bit(0, &irq->disabled)) {
		pr_debug("number = %d\n", irq->irq);
		disable_irq_nosync(irq->irq);
	}
}

#define USB_OTG_EN_BIT	BIT(0)
static int
qpnp_chg_is_otg_en_set(struct qpnp_chg_chip *chip)
{
	u8 usb_otg_en;
	int rc;

	rc = qpnp_chg_read(chip, &usb_otg_en,
				 chip->usb_chgpth_base + CHGR_USB_USB_OTG_CTL,
				 1);

	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				chip->usb_chgpth_base + CHGR_STATUS, rc);
		return rc;
	}
	pr_debug("usb otg en 0x%x\n", usb_otg_en);

	return (usb_otg_en & USB_OTG_EN_BIT) ? 1 : 0;
}

static int
qpnp_chg_is_boost_en_set(struct qpnp_chg_chip *chip)
{
	u8 boost_en_ctl;
	int rc;

	rc = qpnp_chg_read(chip, &boost_en_ctl,
		chip->boost_base + BOOST_ENABLE_CONTROL, 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				chip->boost_base + BOOST_ENABLE_CONTROL, rc);
		return rc;
	}

	pr_debug("boost en 0x%x\n", boost_en_ctl);

	return (boost_en_ctl & BOOST_PWR_EN) ? 1 : 0;
}

static int
qpnp_chg_is_batt_temp_ok(struct qpnp_chg_chip *chip)
{
	u8 batt_rt_sts;
	int rc;

	rc = qpnp_chg_read(chip, &batt_rt_sts,
				 INT_RT_STS(chip->bat_if_base), 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				INT_RT_STS(chip->bat_if_base), rc);
		return rc;
	}

	return (batt_rt_sts & BAT_TEMP_OK_IRQ) ? 1 : 0;
}


static int
qpnp_chg_is_batt_present(struct qpnp_chg_chip *chip)
{
	u8 batt_pres_rt_sts;
	int rc;
#ifdef SEC_CHARGER_CODE
#if defined(CONFIG_USB_SWITCH_RT8973)
	if (rt_check_jig_state() || rt_uart_connecting)
		return 1;
#elif defined(CONFIG_SM5502_MUIC)
	if (check_sm5502_jig_state() || uart_sm5502_connecting)
		return 1;
#else
	if (check_jig_state() || uart_connecting)
		return 1;
#endif
#endif
	rc = qpnp_chg_read(chip, &batt_pres_rt_sts,
				 INT_RT_STS(chip->bat_if_base), 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				INT_RT_STS(chip->bat_if_base), rc);
		return rc;
	}

	return (batt_pres_rt_sts & BATT_PRES_IRQ) ? 1 : 0;
}
static int
qpnp_chg_is_batfet_closed(struct qpnp_chg_chip *chip)
{
	u8 batfet_closed_rt_sts;
	int rc;

	rc = qpnp_chg_read(chip, &batfet_closed_rt_sts,
				 INT_RT_STS(chip->bat_if_base), 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				INT_RT_STS(chip->bat_if_base), rc);
		return rc;
	}

	return (batfet_closed_rt_sts & BAT_FET_ON_IRQ) ? 1 : 0;
}
#define USB_VALID_BIT	BIT(7)
static int
qpnp_chg_is_usb_chg_plugged_in(struct qpnp_chg_chip *chip)
{
	u8 usbin_valid_rt_sts;
	int rc;

	rc = qpnp_chg_read(chip, &usbin_valid_rt_sts,
				 chip->usb_chgpth_base + CHGR_STATUS , 1);

	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				chip->usb_chgpth_base + CHGR_STATUS, rc);
		return rc;
	}
	pr_debug("chgr usb sts 0x%x\n", usbin_valid_rt_sts);

	return (usbin_valid_rt_sts & USB_VALID_BIT) ? 1 : 0;
}

static int
qpnp_chg_is_dc_chg_plugged_in(struct qpnp_chg_chip *chip)
{
	u8 dcin_valid_rt_sts;
	int rc;

	if (!chip->dc_chgpth_base)
		return 0;

	rc = qpnp_chg_read(chip, &dcin_valid_rt_sts,
				 INT_RT_STS(chip->dc_chgpth_base), 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				INT_RT_STS(chip->dc_chgpth_base), rc);
		return rc;
	}

	return (dcin_valid_rt_sts & DCIN_VALID_IRQ) ? 1 : 0;
}

static int
qpnp_chg_is_ichg_loop_active(struct qpnp_chg_chip *chip)
{
	u8 buck_sts;
	int rc;

	rc = qpnp_chg_read(chip, &buck_sts, INT_RT_STS(chip->buck_base), 1);

	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				INT_RT_STS(chip->buck_base), rc);
		return rc;
	}
	pr_debug("buck usb sts 0x%x\n", buck_sts);

	return (buck_sts & ICHG_LOOP_IRQ) ? 1 : 0;
}

#define QPNP_CHG_I_MAX_MIN_100		100
#define QPNP_CHG_I_MAX_MIN_150		150
#define QPNP_CHG_I_MAX_MIN_MA		200
#define QPNP_CHG_I_MAX_MAX_MA		2500
#define QPNP_CHG_I_MAXSTEP_MA		100
static int
qpnp_chg_idcmax_set(struct qpnp_chg_chip *chip, int mA)
{
	int rc = 0;
	u8 dc = 0;

	if (mA < QPNP_CHG_I_MAX_MIN_100
			|| mA > QPNP_CHG_I_MAX_MAX_MA) {
		pr_err("bad mA=%d asked to set\n", mA);
		return -EINVAL;
	}

	if (mA == QPNP_CHG_I_MAX_MIN_100) {
		dc = 0x00;
		pr_debug("current=%d setting %02x\n", mA, dc);
		return qpnp_chg_write(chip, &dc,
			chip->dc_chgpth_base + CHGR_I_MAX_REG, 1);
	} else if (mA == QPNP_CHG_I_MAX_MIN_150) {
		dc = 0x01;
		pr_debug("current=%d setting %02x\n", mA, dc);
		return qpnp_chg_write(chip, &dc,
			chip->dc_chgpth_base + CHGR_I_MAX_REG, 1);
	}

	dc = mA / QPNP_CHG_I_MAXSTEP_MA;

	pr_debug("current=%d setting 0x%x\n", mA, dc);
	rc = qpnp_chg_write(chip, &dc,
		chip->dc_chgpth_base + CHGR_I_MAX_REG, 1);

	return rc;
}

static int
qpnp_chg_iusb_trim_get(struct qpnp_chg_chip *chip)
{
	int rc = 0;
	u8 trim_reg;

	rc = qpnp_chg_read(chip, &trim_reg,
			chip->usb_chgpth_base + CHGR_USB_TRIM, 1);
	if (rc) {
		pr_err("failed to read USB_TRIM rc=%d\n", rc);
		return 0;
	}

	return trim_reg;
}

static int
qpnp_chg_iusb_trim_set(struct qpnp_chg_chip *chip, int trim)
{
	int rc = 0;

	rc = qpnp_chg_masked_write(chip,
		chip->usb_chgpth_base + SEC_ACCESS,
		0xFF,
		0xA5, 1);
	if (rc) {
		pr_err("failed to write SEC_ACCESS rc=%d\n", rc);
		return rc;}

	rc = qpnp_chg_masked_write(chip,
		chip->usb_chgpth_base + CHGR_USB_TRIM,
		0xFF,
		trim, 1);
	if (rc) {
		pr_err("failed to write USB TRIM rc=%d\n", rc);
		return rc;
	}

	return rc;
}

static int
qpnp_chg_iusbmax_set(struct qpnp_chg_chip *chip, int mA)
{
	int rc = 0;
	u8 usb_reg = 0, temp = 8;

#ifdef SEC_CHARGER_DEBUG
	u8 reg_val;
#endif

	if (mA < QPNP_CHG_I_MAX_MIN_100
			|| mA > QPNP_CHG_I_MAX_MAX_MA) {
		pr_err("bad mA=%d asked to set\n", mA);
		return -EINVAL;
	}

	if (mA == QPNP_CHG_I_MAX_MIN_100) {
		usb_reg = 0x00;
		pr_debug("current=%d setting %02x\n", mA, usb_reg);
		return qpnp_chg_write(chip, &usb_reg,
		chip->usb_chgpth_base + CHGR_I_MAX_REG, 1);
	} else if (mA == QPNP_CHG_I_MAX_MIN_150) {
		usb_reg = 0x01;
		pr_debug("current=%d setting %02x\n", mA, usb_reg);
		return qpnp_chg_write(chip, &usb_reg,
		chip->usb_chgpth_base + CHGR_I_MAX_REG, 1);
	}

	/* Impose input current limit */
	if (chip->maxinput_usb_ma)
		mA = (chip->maxinput_usb_ma) <= mA ? chip->maxinput_usb_ma : mA;

	usb_reg = mA / QPNP_CHG_I_MAXSTEP_MA;

	if (chip->flags & CHG_FLAGS_VCP_WA) {
		temp = 0xA5;
		rc = qpnp_chg_write(chip, &temp,
			chip->buck_base + SEC_ACCESS, 1);
		rc = qpnp_chg_masked_write(chip,
			chip->buck_base + CHGR_BUCK_COMPARATOR_OVRIDE_3,
			0x0C, 0x0C, 1);
	}

	pr_debug("current=%d setting 0x%x\n", mA, usb_reg);
	rc = qpnp_chg_write(chip, &usb_reg,
		chip->usb_chgpth_base + CHGR_I_MAX_REG, 1);

	if (chip->flags & CHG_FLAGS_VCP_WA) {
		temp = 0xA5;
		udelay(200);
		rc = qpnp_chg_write(chip, &temp,
			chip->buck_base + SEC_ACCESS, 1);
		rc = qpnp_chg_masked_write(chip,
			chip->buck_base + CHGR_BUCK_COMPARATOR_OVRIDE_3,
			0x0C, 0x00, 1);
	}

#ifdef SEC_CHARGER_DEBUG
	pr_err("current=%d setting 0x%x\n", mA, usb_reg);
	qpnp_chg_read(chip, &reg_val,
                chip->usb_chgpth_base + CHGR_I_MAX_REG, 1);
	pr_err("CHGR_I_MAX_REG: reg(%x) setting (0x%x)\n",
		(chip->usb_chgpth_base + CHGR_I_MAX_REG), reg_val);
#endif

	return rc;
}

#define QPNP_CHG_VINMIN_MIN_MV		4200
#define QPNP_CHG_VINMIN_HIGH_MIN_MV	5600
#define QPNP_CHG_VINMIN_HIGH_MIN_VAL	0x2B
#define QPNP_CHG_VINMIN_MAX_MV		9600
#define QPNP_CHG_VINMIN_STEP_MV		50
#define QPNP_CHG_VINMIN_STEP_HIGH_MV	200
#define QPNP_CHG_VINMIN_MASK		0x1F
#define QPNP_CHG_VINMIN_MIN_VAL	0x10
static int
qpnp_chg_vinmin_set(struct qpnp_chg_chip *chip, int voltage)
{
	u8 temp;

	if (voltage < QPNP_CHG_VINMIN_MIN_MV
			|| voltage > QPNP_CHG_VINMIN_MAX_MV) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}
	if (voltage >= QPNP_CHG_VINMIN_HIGH_MIN_MV) {
		temp = QPNP_CHG_VINMIN_HIGH_MIN_VAL;
		temp += (voltage - QPNP_CHG_VINMIN_MIN_MV)
			/ QPNP_CHG_VINMIN_STEP_HIGH_MV;
	} else {
		temp = QPNP_CHG_VINMIN_MIN_VAL;
		temp += (voltage - QPNP_CHG_VINMIN_MIN_MV)
			/ QPNP_CHG_VINMIN_STEP_MV;
	}

	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return qpnp_chg_masked_write(chip,
			chip->chgr_base + CHGR_VIN_MIN,
			QPNP_CHG_VINMIN_MASK, temp, 1);
}

static int
qpnp_chg_vinmin_get(struct qpnp_chg_chip *chip)
{
	int rc, vin_min_mv;
	u8 vin_min;

	rc = qpnp_chg_read(chip, &vin_min, chip->chgr_base + CHGR_VIN_MIN, 1);
	if (rc) {
		pr_err("failed to read VIN_MIN rc=%d\n", rc);
		return 0;
	}

	if (vin_min == 0)
		vin_min_mv = QPNP_CHG_I_MAX_MIN_100;
	else if (vin_min > QPNP_CHG_VINMIN_HIGH_MIN_VAL)
		vin_min_mv = QPNP_CHG_VINMIN_HIGH_MIN_MV +
			(vin_min - QPNP_CHG_VINMIN_HIGH_MIN_VAL)
				* QPNP_CHG_VINMIN_STEP_HIGH_MV;
	else
		vin_min_mv = QPNP_CHG_VINMIN_MIN_MV +
			(vin_min - QPNP_CHG_VINMIN_MIN_VAL)
				* QPNP_CHG_VINMIN_STEP_MV;
	pr_debug("vin_min= 0x%02x, ma = %d\n", vin_min, vin_min_mv);

	return vin_min_mv;
}

static int
qpnp_chg_usb_iusbmax_get(struct qpnp_chg_chip *chip)
{
	int rc, iusbmax_ma;
	u8 iusbmax;

	rc = qpnp_chg_read(chip, &iusbmax,
		chip->usb_chgpth_base + CHGR_I_MAX_REG, 1);
	if (rc) {
		pr_err("failed to read IUSB_MAX rc=%d\n", rc);
		return 0;
	}

	if (iusbmax == 0)
		iusbmax_ma = QPNP_CHG_I_MAX_MIN_100;
	else if (iusbmax == 0x01)
		iusbmax_ma = QPNP_CHG_I_MAX_MIN_150;
	else
		iusbmax_ma = iusbmax * QPNP_CHG_I_MAXSTEP_MA;

	pr_debug("iusbmax = 0x%02x, ma = %d\n", iusbmax, iusbmax_ma);

	return iusbmax_ma;
}


#define USB_SUSPEND_BIT	BIT(0)
static int
qpnp_chg_usb_suspend_enable(struct qpnp_chg_chip *chip, int enable)
{
	return qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + CHGR_USB_USB_SUSP,
			USB_SUSPEND_BIT,
			enable ? USB_SUSPEND_BIT : 0, 1);
}

#ifdef SEC_CHARGER_CODE
static int
qpnp_chg_charge_en(struct qpnp_chg_chip *chip, int enable)
{
	pr_err(" qpnp_chg_charge_en called with enable=%d\n", enable);

	return qpnp_chg_masked_write(chip, chip->chgr_base + CHGR_CHG_CTRL,
			CHGR_CHG_EN,
			enable ? CHGR_CHG_EN : 0, 1);
}
#else
static int
qpnp_chg_charge_en(struct qpnp_chg_chip *chip, int enable)
{
	return qpnp_chg_masked_write(chip, chip->chgr_base + CHGR_CHG_CTRL,
			CHGR_CHG_EN,
			enable ? CHGR_CHG_EN : 0, 1);
}
#endif

static int
qpnp_chg_force_run_on_batt(struct qpnp_chg_chip *chip, int disable)
{
	/* Don't run on battery for batteryless hardware */
	if (chip->use_default_batt_values)
		return 0;

	/* Don't force on battery and allow charge if battery is not present*/
	if (!disable && !qpnp_chg_is_batt_present(chip))
		return 0;

	/* This bit forces the charger to run off of the battery rather
	 * than a connected charger */
	return qpnp_chg_masked_write(chip, chip->chgr_base + CHGR_CHG_CTRL,
			CHGR_ON_BAT_FORCE_BIT,
			disable ? CHGR_ON_BAT_FORCE_BIT : 0, 1);
}

#define BUCK_DUTY_MASK_100P	0x30
static int
qpnp_buck_set_100_duty_cycle_enable(struct qpnp_chg_chip *chip, int enable)
{
	int rc;

	pr_debug("enable: %d\n", enable);

	rc = qpnp_chg_masked_write(chip,
		chip->buck_base + SEC_ACCESS, 0xA5, 0xA5, 1);
	if (rc) {
		pr_debug("failed to write sec access rc=%d\n", rc);
		return rc;
	}

	rc = qpnp_chg_masked_write(chip,
		chip->buck_base + BUCK_TEST_SMBC_MODES,
			BUCK_DUTY_MASK_100P, enable ? 0x00 : 0x10, 1);
	if (rc) {
		pr_debug("failed enable 100p duty cycle rc=%d\n", rc);
		return rc;
	}

	return rc;
}

#define COMPATATOR_OVERRIDE_0	0x80
static int
qpnp_chg_toggle_chg_done_logic(struct qpnp_chg_chip *chip, int enable)
{
	int rc;

	pr_debug("toggle: %d\n", enable);

	rc = qpnp_chg_masked_write(chip,
		chip->buck_base + SEC_ACCESS, 0xA5, 0xA5, 1);
	if (rc) {
		pr_debug("failed to write sec access rc=%d\n", rc);
		return rc;
	}

	rc = qpnp_chg_masked_write(chip,
		chip->buck_base + CHGR_BUCK_COMPARATOR_OVRIDE_1,
			0xC0, enable ? 0x00 : COMPATATOR_OVERRIDE_0, 1);
	if (rc) {
		pr_debug("failed to toggle chg done override rc=%d\n", rc);
		return rc;
	}

	return rc;
}

#define QPNP_CHG_VBATDET_MIN_MV	3240
#define QPNP_CHG_VBATDET_MAX_MV	5780
#define QPNP_CHG_VBATDET_STEP_MV	20
static int
qpnp_chg_vbatdet_set(struct qpnp_chg_chip *chip, int vbatdet_mv)
{
	u8 temp;

	if (vbatdet_mv < QPNP_CHG_VBATDET_MIN_MV
			|| vbatdet_mv > QPNP_CHG_VBATDET_MAX_MV) {
		pr_err("bad mV=%d asked to set\n", vbatdet_mv);
		return -EINVAL;
	}
	temp = (vbatdet_mv - QPNP_CHG_VBATDET_MIN_MV)
			/ QPNP_CHG_VBATDET_STEP_MV;

	pr_debug("voltage=%d setting %02x\n", vbatdet_mv, temp);
	return qpnp_chg_write(chip, &temp,
		chip->chgr_base + CHGR_VBAT_DET, 1);
}

static void
qpnp_chg_set_appropriate_vbatdet(struct qpnp_chg_chip *chip)
{
	if (chip->bat_is_cool)
		qpnp_chg_vbatdet_set(chip, chip->cool_bat_mv
			- chip->resume_delta_mv);
	else if (chip->bat_is_warm)
		qpnp_chg_vbatdet_set(chip, chip->warm_bat_mv
			- chip->resume_delta_mv);
	else if (chip->resuming_charging)
		qpnp_chg_vbatdet_set(chip, chip->max_voltage_mv
			+ chip->resume_delta_mv);
	else
		qpnp_chg_vbatdet_set(chip, chip->max_voltage_mv
			- chip->resume_delta_mv);
}

static void
qpnp_arb_stop_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct qpnp_chg_chip *chip = container_of(dwork,
				struct qpnp_chg_chip, arb_stop_work);
	#if 0
	u8 test_reg_val;
	#endif

	pr_err("arb request :chg_en(%d), chg_done(%d)\n", !chip->charging_disabled, chip->chg_done);

	//bkj - low voltage battery charging issue
	#if 1
	if (!chip->chg_done)
		//qpnp_chg_charge_en(chip, chip->charging_disabled);
		qpnp_chg_charge_en(chip, 1);
	qpnp_chg_force_run_on_batt(chip, 0);
	#else
	if (!chip->chg_done)
		qpnp_chg_charge_en(chip, !chip->charging_disabled);
	qpnp_chg_force_run_on_batt(chip, chip->charging_disabled);
	#endif

	#if 0
	qpnp_chg_read(chip, &test_reg_val, 0x100B, 1);
	pr_err("Reg Addr: 0x100B, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1010, 1);
	pr_err("Reg Addr: 0x1010, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1049, 1);
	pr_err("Reg Addr: 0x1049, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1310, 1);
	pr_err("Reg Addr: 0x1310, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x130C, 1);
	pr_err("Reg Addr: 0x130C, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x130A, 1);
	pr_err("Reg Addr: 0x130A, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1309, 1);
	pr_err("Reg Addr: 0x1309, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1308, 1);
	pr_err("Reg Addr: 0x1308, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1347, 1);
	pr_err("Reg Addr: 0x1347, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1208, 1);
	pr_err("Reg Addr: 0x1208, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1209, 1);
	pr_err("Reg Addr: 0x1209, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1210, 1);
	pr_err("Reg Addr: 0x1210, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x120C, 1);
	pr_err("Reg Addr: 0x120C, Reg Value: 0x%x\n", test_reg_val);

	pr_err("Wrote 0xA5 to Reg Addr: 0x10D0\n");
	test_reg_val = 0xA5;
	qpnp_chg_write(chip, &test_reg_val, 0x10D0, 1);
	pr_err("Wrote 0x01 to Reg Addr: 0x10E6\n");
	test_reg_val = 0x01;
	qpnp_chg_write(chip, &test_reg_val, 0x10E6, 1);
	qpnp_chg_read(chip, &test_reg_val, 0x10E7, 1);
	pr_err("Reg Addr: 0x10E7, Reg Value: 0x%x\n", test_reg_val);
	#endif
}

static void
qpnp_bat_if_adc_measure_work(struct work_struct *work)
{
	struct qpnp_chg_chip *chip = container_of(work,
				struct qpnp_chg_chip, adc_measure_work);

	if (qpnp_adc_tm_channel_measure(chip->adc_tm_dev,&chip->adc_param))
		pr_err("request ADC error\n");
}

static void
qpnp_bat_if_adc_disable_work(struct work_struct *work)
{
	struct qpnp_chg_chip *chip = container_of(work,
				struct qpnp_chg_chip, adc_disable_work);

	qpnp_adc_tm_disable_chan_meas(chip->adc_tm_dev, &chip->adc_param);
}

#ifdef SEC_CHARGER_CODE
#define EOC_CHECK_PERIOD_MS	10000
static irqreturn_t
qpnp_chg_vbatdet_lo_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	u8 chg_sts = 0;
	int rc;

        pr_debug("vbatdet-lo triggered\n");

	rc = qpnp_chg_read(chip, &chg_sts, INT_RT_STS(chip->chgr_base), 1);
	if (rc)
		pr_err("failed to read chg_sts rc=%d\n", rc);

	pr_debug("chg_done chg_sts: 0x%x triggered\n", chg_sts);

	#if 0
	if (!chip->charging_disabled && (chg_sts & FAST_CHG_ON_IRQ)) {
		schedule_delayed_work(&chip->eoc_work,
			msecs_to_jiffies(EOC_CHECK_PERIOD_MS));
		pm_stay_awake(chip->dev);
	}
	#endif
	qpnp_chg_disable_irq(&chip->chg_vbatdet_lo);

	pr_debug("psy changed usb_psy\n");
	power_supply_changed(chip->usb_psy);
	if (chip->dc_chgpth_base) {
		pr_debug("psy changed dc_psy\n");
		power_supply_changed(&chip->dc_psy);
	}
	if (chip->bat_if_base) {
		pr_debug("psy changed batt_psy\n");
		power_supply_changed(&chip->batt_psy);
	}
	return IRQ_HANDLED;
}
#else
#define EOC_CHECK_PERIOD_MS     10000
static irqreturn_t
qpnp_chg_vbatdet_lo_irq_handler(int irq, void *_chip)
{
        struct qpnp_chg_chip *chip = _chip;
        u8 chg_sts = 0;
        int rc;

        pr_debug("vbatdet-lo triggered\n");

        rc = qpnp_chg_read(chip, &chg_sts, INT_RT_STS(chip->chgr_base), 1);
        if (rc)
                pr_err("failed to read chg_sts rc=%d\n", rc);

        pr_debug("chg_done chg_sts: 0x%x triggered\n", chg_sts);
        if (!chip->charging_disabled && (chg_sts & FAST_CHG_ON_IRQ)) {
                schedule_delayed_work(&chip->eoc_work,
                        msecs_to_jiffies(EOC_CHECK_PERIOD_MS));
				pm_stay_awake(chip->dev);
        }
		qpnp_chg_disable_irq(&chip->chg_vbatdet_lo);

        power_supply_changed(chip->usb_psy);
        if (chip->dc_chgpth_base)
                power_supply_changed(&chip->dc_psy);
        if (chip->bat_if_base)
                power_supply_changed(&chip->batt_psy);
        return IRQ_HANDLED;
}
#endif

#define ARB_STOP_WORK_MS	1000
static irqreturn_t
qpnp_chg_usb_chg_gone_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	#if 0
	u8 test_reg_val;
	#endif

	pr_debug("chg_gone triggered\n");
	if (qpnp_chg_is_usb_chg_plugged_in(chip)) {
		#ifndef CONFIG_NOT_USE_EXT_OVP
		gpio_set_value(chip->ovp_gpio, 0);
		pr_err("EXT_OVP disabled\n");
		#endif
		/* charging must be stopped by
		 * sec_pm8226_stop_charging() from cable callback */
		pr_err("chg_gone triggered: disable chg\n");
		qpnp_chg_charge_en(chip, 0);
		qpnp_chg_force_run_on_batt(chip, 1);
		schedule_delayed_work(&chip->arb_stop_work,
			msecs_to_jiffies(ARB_STOP_WORK_MS));

		#if 0
		qpnp_chg_read(chip, &test_reg_val, 0x100B, 1);
		pr_err("Reg Addr: 0x100B, Reg Value: 0x%x\n", test_reg_val);
		qpnp_chg_read(chip, &test_reg_val, 0x1010, 1);
		pr_err("Reg Addr: 0x1010, Reg Value: 0x%x\n", test_reg_val);
		qpnp_chg_read(chip, &test_reg_val, 0x1049, 1);
		pr_err("Reg Addr: 0x1049, Reg Value: 0x%x\n", test_reg_val);
		qpnp_chg_read(chip, &test_reg_val, 0x1310, 1);
		pr_err("Reg Addr: 0x1310, Reg Value: 0x%x\n", test_reg_val);
		qpnp_chg_read(chip, &test_reg_val, 0x130C, 1);
		pr_err("Reg Addr: 0x130C, Reg Value: 0x%x\n", test_reg_val);
		qpnp_chg_read(chip, &test_reg_val, 0x130A, 1);
		pr_err("Reg Addr: 0x130A, Reg Value: 0x%x\n", test_reg_val);
		qpnp_chg_read(chip, &test_reg_val, 0x1309, 1);
		pr_err("Reg Addr: 0x1309, Reg Value: 0x%x\n", test_reg_val);
		qpnp_chg_read(chip, &test_reg_val, 0x1308, 1);
		pr_err("Reg Addr: 0x1308, Reg Value: 0x%x\n", test_reg_val);
		qpnp_chg_read(chip, &test_reg_val, 0x1347, 1);
		pr_err("Reg Addr: 0x1347, Reg Value: 0x%x\n", test_reg_val);
		qpnp_chg_read(chip, &test_reg_val, 0x1208, 1);
		pr_err("Reg Addr: 0x1208, Reg Value: 0x%x\n", test_reg_val);
		qpnp_chg_read(chip, &test_reg_val, 0x1209, 1);
		pr_err("Reg Addr: 0x1209, Reg Value: 0x%x\n", test_reg_val);
		qpnp_chg_read(chip, &test_reg_val, 0x1210, 1);
		pr_err("Reg Addr: 0x1210, Reg Value: 0x%x\n", test_reg_val);
		qpnp_chg_read(chip, &test_reg_val, 0x120C, 1);
		pr_err("Reg Addr: 0x120C, Reg Value: 0x%x\n", test_reg_val);

		pr_err("Wrote 0xA5 to Reg Addr: 0x10D0\n");
		test_reg_val = 0xA5;
		qpnp_chg_write(chip, &test_reg_val, 0x10D0, 1);
		pr_err("Wrote 0x01 to Reg Addr: 0x10E6\n");
		test_reg_val = 0x01;
		qpnp_chg_write(chip, &test_reg_val, 0x10E6, 1);
		qpnp_chg_read(chip, &test_reg_val, 0x10E7, 1);
		pr_err("Reg Addr: 0x10E7, Reg Value: 0x%x\n", test_reg_val);
		#endif
	}

	return IRQ_HANDLED;
}

static irqreturn_t
qpnp_chg_usb_usb_ocp_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	int rc;

	pr_debug("usb-ocp triggered\n");

	rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + USB_OCP_CLR,
			OCP_CLR_BIT,
			OCP_CLR_BIT, 1);
	if (rc)
		pr_err("Failed to clear OCP bit rc = %d\n", rc);

	/* force usb ovp fet off */
	rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + CHGR_USB_USB_OTG_CTL,
			USB_OTG_EN_BIT,
			USB_OTG_EN_BIT, 1);
	if (rc)
		pr_err("Failed to turn off usb ovp rc = %d\n", rc);

	return IRQ_HANDLED;
}

#define QPNP_CHG_VDDMAX_MIN		3400
#define QPNP_CHG_V_MIN_MV		3240
#define QPNP_CHG_V_MAX_MV		4500
#define QPNP_CHG_V_STEP_MV		10
#define QPNP_CHG_BUCK_TRIM1_STEP	10
#define QPNP_CHG_BUCK_VDD_TRIM_MASK	0xF0
static int
qpnp_chg_vddmax_and_trim_set(struct qpnp_chg_chip *chip,
		int voltage, int trim_mv)
{
	int rc, trim_set;
	u8 vddmax = 0, trim = 0;

	if (voltage < QPNP_CHG_VDDMAX_MIN
			|| voltage > QPNP_CHG_V_MAX_MV) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}

	vddmax = (voltage - QPNP_CHG_V_MIN_MV) / QPNP_CHG_V_STEP_MV;
	rc = qpnp_chg_write(chip, &vddmax, chip->chgr_base + CHGR_VDD_MAX, 1);
	if (rc) {
		pr_err("Failed to write vddmax: %d\n", rc);
		return rc;
	}

	rc = qpnp_chg_masked_write(chip,
		chip->buck_base + SEC_ACCESS,
		0xFF,
		0xA5, 1);
	if (rc) {
		pr_err("failed to write SEC_ACCESS rc=%d\n", rc);
		return rc;
	}
	trim_set = clamp((int)chip->trim_center
			+ (trim_mv / QPNP_CHG_BUCK_TRIM1_STEP),
			0, 0xF);
	trim = (u8)trim_set << 4;
	rc = qpnp_chg_masked_write(chip,
		chip->buck_base + BUCK_CTRL_TRIM1,
		QPNP_CHG_BUCK_VDD_TRIM_MASK,
		trim, 1);
	if (rc) {
		pr_err("Failed to write buck trim1: %d\n", rc);
		return rc;
	}
	pr_debug("voltage=%d+%d setting vddmax: %02x, trim: %02x\n",
			voltage, trim_mv, vddmax, trim);
	return 0;
}

/* JEITA compliance logic */
static void
qpnp_chg_set_appropriate_vddmax(struct qpnp_chg_chip *chip)
{
	if (chip->bat_is_cool)
		qpnp_chg_vddmax_and_trim_set(chip, chip->cool_bat_mv,
				chip->delta_vddmax_mv);
	else if (chip->bat_is_warm)
		qpnp_chg_vddmax_and_trim_set(chip, chip->warm_bat_mv,
				chip->delta_vddmax_mv);
	else
		qpnp_chg_vddmax_and_trim_set(chip, chip->max_voltage_mv,
				chip->delta_vddmax_mv);
}

#ifdef SEC_CHARGER_CODE
#define ENUM_T_STOP_BIT         BIT(0)
static irqreturn_t
qpnp_chg_usb_usbin_valid_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	int usb_present, host_mode;
	u8 usb_ov_sts = 0;
	#if 0
	u8 test_reg_val;
	#endif

	#if defined(CONFIG_SEC_KANAS_PROJECT)
	mdelay(10);
	#endif

	usb_present = qpnp_chg_is_usb_chg_plugged_in(chip);
	host_mode = qpnp_chg_is_otg_en_set(chip);
	pr_debug("usbin-valid triggered: %d host_mode: %d\n",
				usb_present, host_mode);

	#ifndef CONFIG_NOT_USE_EXT_OVP
	/* disable EXT_OVP when charging is disabled */
	if (usb_present == 0) {
		gpio_set_value(chip->ovp_gpio, 0);
		pr_err("EXT_OVP disabled\n");
	}
	#endif

	/* OVP UVLO check */
	qpnp_chg_read(chip, &usb_ov_sts,
	chip->usb_chgpth_base + CHGR_STATUS , 1);
	if ((usb_ov_sts & 0x80) == 0x80) {
		pr_err("USB-IN triggered : usbin-valid voltage \n");
		chip->ovp_uvlo_state = 0;
		chip->cable_type = msm8930_get_cable_status();
		if (chip->cable_type == CABLE_TYPE_DESK_DOCK)
			chip->cable_type = CABLE_TYPE_MISC;
	} else if ((usb_ov_sts & 0xC0) == 0x40) {
		pr_err("USB-IN triggered : usbin OVER VOLTAGE \n");
		#ifndef CONFIG_NOT_USE_EXT_OVP
		gpio_set_value(chip->ovp_gpio, 0);
		pr_err("Over Voltage - EXT_OVP disabled\n");
		#endif
		chip->ovp_uvlo_state = 1;
	} else if ((usb_ov_sts & 0xC0) == 0x00) {
		pr_err("USB-IN triggered : usbin UNDER VOLTAGE \n");
		#ifndef CONFIG_NOT_USE_EXT_OVP
		gpio_set_value(chip->ovp_gpio, 0);
		pr_err("Under Voltage - EXT_OVP disabled\n");
		#endif
		chip->ovp_uvlo_state = -1;
	}

	schedule_delayed_work(&chip->ovp_uvlo_work, 1*HZ);

	/* In host mode notifications cmoe from USB supply */
	if (host_mode)
		return IRQ_HANDLED;

	if (chip->usb_present ^ usb_present) {
		chip->usb_present = usb_present;
		if (!usb_present) {
			if (!qpnp_chg_is_dc_chg_plugged_in(chip)) {
				chip->delta_vddmax_mv = 0;
				qpnp_chg_set_appropriate_vddmax(chip);
				chip->chg_done = false;
			}
			qpnp_chg_usb_suspend_enable(chip, 1);
			chip->prev_usb_max_ma = -EINVAL;
		} else {
			if (chip->cable_type != CABLE_TYPE_NONE)
				sec_handle_cable_insertion_removal(chip);
			if (!qpnp_chg_is_dc_chg_plugged_in(chip)) {
				chip->delta_vddmax_mv = 0;
				qpnp_chg_set_appropriate_vddmax(chip);
			}
			#if 0
			schedule_delayed_work(&chip->eoc_work,
					msecs_to_jiffies(EOC_CHECK_PERIOD_MS));
			#endif
			schedule_work(&chip->soc_check_work);
		}
		power_supply_set_present(chip->usb_psy, chip->usb_present);
	}

	#if 0
	qpnp_chg_read(chip, &test_reg_val, 0x100B, 1);
	pr_err("Reg Addr: 0x100B, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1010, 1);
	pr_err("Reg Addr: 0x1010, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1049, 1);
	pr_err("Reg Addr: 0x1049, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1310, 1);
	pr_err("Reg Addr: 0x1310, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x130C, 1);
	pr_err("Reg Addr: 0x130C, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x130A, 1);
	pr_err("Reg Addr: 0x130A, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1309, 1);
	pr_err("Reg Addr: 0x1309, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1308, 1);
	pr_err("Reg Addr: 0x1308, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1347, 1);
	pr_err("Reg Addr: 0x1347, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1208, 1);
	pr_err("Reg Addr: 0x1208, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1209, 1);
	pr_err("Reg Addr: 0x1209, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1210, 1);
	pr_err("Reg Addr: 0x1210, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x120C, 1);
	pr_err("Reg Addr: 0x120C, Reg Value: 0x%x\n", test_reg_val);

	pr_err("Wrote 0xA5 to Reg Addr: 0x10D0\n");
	test_reg_val = 0xA5;
	qpnp_chg_write(chip, &test_reg_val, 0x10D0, 1);
	pr_err("Wrote 0x01 to Reg Addr: 0x10E6\n");
	test_reg_val = 0x01;
	qpnp_chg_write(chip, &test_reg_val, 0x10E6, 1);
	qpnp_chg_read(chip, &test_reg_val, 0x10E7, 1);
	pr_err("Reg Addr: 0x10E7, Reg Value: 0x%x\n", test_reg_val);
	#endif

        return IRQ_HANDLED;
}
#else

#define ENUM_T_STOP_BIT		BIT(0)
static irqreturn_t
qpnp_chg_usb_usbin_valid_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	int usb_present, host_mode;

	usb_present = qpnp_chg_is_usb_chg_plugged_in(chip);
	host_mode = qpnp_chg_is_otg_en_set(chip);
	pr_debug("usbin-valid triggered: %d host_mode: %d\n",
		usb_present, host_mode);

	/* In host mode notifications cmoe from USB supply */
	if (host_mode)
		return IRQ_HANDLED;

	if (chip->usb_present ^ usb_present) {
		chip->usb_present = usb_present;
		if (!usb_present) {
			qpnp_chg_usb_suspend_enable(chip, 1);
			chip->chg_done = false;
			chip->prev_usb_max_ma = -EINVAL;
		} else {
			schedule_delayed_work(&chip->eoc_work,
				msecs_to_jiffies(EOC_CHECK_PERIOD_MS));
		}

		power_supply_set_present(chip->usb_psy, chip->usb_present);
	}

	return IRQ_HANDLED;
}
#endif

static irqreturn_t
qpnp_chg_bat_if_batt_temp_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	int batt_temp_good;

	batt_temp_good = qpnp_chg_is_batt_temp_ok(chip);
	pr_debug("batt-temp triggered: %d\n", batt_temp_good);

	pr_debug("psy changed batt_psy\n");
	power_supply_changed(&chip->batt_psy);
	return IRQ_HANDLED;
}

#ifdef SEC_CHARGER_CODE
static irqreturn_t
qpnp_chg_bat_if_batt_pres_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	int batt_present;

	batt_present = qpnp_chg_is_batt_present(chip);
	pr_err("batt-pres triggered: %d\n", batt_present);

	if (chip->batt_present ^ batt_present) {
		chip->batt_present = batt_present;
		pr_debug("psy changed batt_psy\n");
		power_supply_changed(&chip->batt_psy);
		pr_debug("psy changed usb_psy\n");
		power_supply_changed(chip->usb_psy);

		if ((chip->cool_bat_decidegc || chip->warm_bat_decidegc)
						&& batt_present) {
			pr_debug("enabling vadc notifications\n");
			schedule_work(&chip->adc_measure_work);
		} else if ((chip->cool_bat_decidegc || chip->warm_bat_decidegc)
				&& !batt_present) {
			schedule_work(&chip->adc_disable_work);
			pr_debug("disabling vadc notifications\n");
		}
		sec_handle_cable_insertion_removal(chip);
	}
	return IRQ_HANDLED;
}
#else
static irqreturn_t
qpnp_chg_bat_if_batt_pres_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	int batt_present;

	batt_present = qpnp_chg_is_batt_present(chip);
	pr_debug("batt-pres triggered: %d\n", batt_present);

	if (chip->batt_present ^ batt_present) {
		chip->batt_present = batt_present;
		power_supply_changed(&chip->batt_psy);
		power_supply_changed(chip->usb_psy);

		if (chip->cool_bat_decidegc && chip->warm_bat_decidegc
						&& batt_present) {
			pr_debug("enabling vadc notifications\n");
			schedule_work(&chip->adc_measure_work);
		} else if (chip->cool_bat_decidegc && chip->warm_bat_decidegc
				&& !batt_present) {
			qpnp_adc_tm_disable_chan_meas(chip->adc_tm_dev,
					&chip->adc_param);
			pr_debug("disabling vadc notifications\n");
		}
	}

	return IRQ_HANDLED;
}
#endif

static irqreturn_t
qpnp_chg_dc_dcin_valid_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	int dc_present;

	dc_present = qpnp_chg_is_dc_chg_plugged_in(chip);
	pr_debug("dcin-valid triggered: %d\n", dc_present);

	if (chip->dc_present ^ dc_present) {
		chip->dc_present = dc_present;
		if (!dc_present && !qpnp_chg_is_usb_chg_plugged_in(chip)) {
			chip->delta_vddmax_mv = 0;
			qpnp_chg_set_appropriate_vddmax(chip);
			chip->chg_done = false;
		} else {
			if (!qpnp_chg_is_usb_chg_plugged_in(chip)) {
				chip->delta_vddmax_mv = 0;
				qpnp_chg_set_appropriate_vddmax(chip);
			}
			#if 0
			schedule_delayed_work(&chip->eoc_work,
				msecs_to_jiffies(EOC_CHECK_PERIOD_MS));
			#endif
			schedule_work(&chip->soc_check_work);
		}
		pr_debug("psy changed dc_psy\n");
		power_supply_changed(&chip->dc_psy);
		pr_debug("psy changed batt_psy\n");
		power_supply_changed(&chip->batt_psy);
	}

	return IRQ_HANDLED;
}

#define CHGR_CHG_FAILED_BIT	BIT(7)
static irqreturn_t
qpnp_chg_chgr_chg_failed_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	int rc;

	pr_debug("chg_failed triggered\n");

	rc = qpnp_chg_masked_write(chip,
		chip->chgr_base + CHGR_CHG_FAILED,
		CHGR_CHG_FAILED_BIT,
		CHGR_CHG_FAILED_BIT, 1);
	if (rc)
		pr_err("Failed to write chg_fail clear bit!\n");

	if (chip->bat_if_base) {
		pr_debug("psy changed batt_psy\n");
		power_supply_changed(&chip->batt_psy);
	}
	pr_debug("psy changed usb_psy\n");
	power_supply_changed(chip->usb_psy);
	if (chip->dc_chgpth_base) {
		pr_debug("psy changed dc_psy\n");
		power_supply_changed(&chip->dc_psy);
	}
	return IRQ_HANDLED;
}

static irqreturn_t
qpnp_chg_chgr_chg_trklchg_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	#if 0
	u8 test_reg_val;
	#endif

	pr_debug("TRKL IRQ triggered\n");

	chip->chg_done = false;
	if (chip->bat_if_base) {
		pr_debug("psy changed batt_psy\n");
		power_supply_changed(&chip->batt_psy);
	}

	#if 0
	qpnp_chg_read(chip, &test_reg_val, 0x100B, 1);
	pr_err("Reg Addr: 0x100B, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1010, 1);
	pr_err("Reg Addr: 0x1010, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1049, 1);
	pr_err("Reg Addr: 0x1049, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1310, 1);
	pr_err("Reg Addr: 0x1310, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x130C, 1);
	pr_err("Reg Addr: 0x130C, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x130A, 1);
	pr_err("Reg Addr: 0x130A, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1309, 1);
	pr_err("Reg Addr: 0x1309, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1308, 1);
	pr_err("Reg Addr: 0x1308, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1347, 1);
	pr_err("Reg Addr: 0x1347, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1208, 1);
	pr_err("Reg Addr: 0x1208, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1209, 1);
	pr_err("Reg Addr: 0x1209, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1210, 1);
	pr_err("Reg Addr: 0x1210, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x120C, 1);
	pr_err("Reg Addr: 0x120C, Reg Value: 0x%x\n", test_reg_val);

	pr_err("Wrote 0xA5 to Reg Addr: 0x10D0\n");
	test_reg_val = 0xA5;
	qpnp_chg_write(chip, &test_reg_val, 0x10D0, 1);
	pr_err("Wrote 0x01 to Reg Addr: 0x10E6\n");
	test_reg_val = 0x01;
	qpnp_chg_write(chip, &test_reg_val, 0x10E6, 1);
	qpnp_chg_read(chip, &test_reg_val, 0x10E7, 1);
	pr_err("Reg Addr: 0x10E7, Reg Value: 0x%x\n", test_reg_val);
	#endif

	return IRQ_HANDLED;
}

#ifndef CONFIG_NOT_USE_EXT_OVP
static void sec_bat_ext_ovp_confirm(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct qpnp_chg_chip *chip = container_of(dwork,
                                struct qpnp_chg_chip, sec_bat_ext_ovp_work);

	pr_err("cable_type (%d)\n", chip->cable_type);
	if (chip->cable_type == CABLE_TYPE_AC ||
				chip->cable_type == CABLE_TYPE_CDP ||
				chip->cable_type == CABLE_TYPE_MISC ||
				chip->cable_type == CABLE_TYPE_UARTOFF ||
				chip->cable_type == CABLE_TYPE_AUDIO_DOCK) {
		gpio_set_value(chip->ovp_gpio, 1);
		pr_err("Ext OVP is set again\n");

	} else {
		pr_err("Charging Terminated\n");
	}

	return;
}
#endif

static irqreturn_t
qpnp_chg_chgr_chg_fastchg_irq_handler(int irq, void *_chip)
{
	struct qpnp_chg_chip *chip = _chip;
	u8 chgr_sts;
	int rc;
	union power_supply_propval ret = {0,};
	#if 0
	u8 test_reg_val;
	#endif

	rc = qpnp_chg_read(chip, &chgr_sts, INT_RT_STS(chip->chgr_base), 1);

	if (rc)
		pr_err("failed to read interrupt sts %d\n", rc);

	pr_err("FAST_CHG IRQ triggered\n");
	chip->chg_done = false;

	// It is opened for AICL test by Qualcomm
	if (chip->bat_if_base) {
		pr_debug("psy changed batt_psy\n");
		power_supply_changed(&chip->batt_psy);
	}

	/*
	pr_debug("psy changed usb_psy\n");
	power_supply_changed(chip->usb_psy);

	if (chip->dc_chgpth_base) {
		pr_debug("psy changed dc_psy\n");
		power_supply_changed(&chip->dc_psy);
	}
	*/

	if (chip->resuming_charging) {
		chip->resuming_charging = false;
		qpnp_chg_set_appropriate_vbatdet(chip);
	}
	#if 0
	if (!chip->charging_disabled) {

		schedule_delayed_work(&chip->eoc_work,
			msecs_to_jiffies(EOC_CHECK_PERIOD_MS));
		pm_stay_awake(chip->dev);
	}
	#endif

	/* we don't need it */
	//qpnp_chg_enable_irq(&chip->chg_vbatdet_lo);

	if (chgr_sts & FAST_CHG_ON_IRQ) {
		pr_err("FAST_CHG ON IRQ\n");
		/*
		chip->usb_psy->get_property(chip->usb_psy,
				POWER_SUPPLY_PROP_CURRENT_MAX, &ret);
				*/
		ret.intval = qpnp_chg_usb_iusbmax_get(chip) * 1000;
		pr_err("FAST_CHG ON IRQ - ret.intval : (%d)\n", ret.intval);

		#ifndef CONFIG_NOT_USE_EXT_OVP
		pr_err("cable_type (%d)\n", chip->cable_type);
		if (chip->cable_type == CABLE_TYPE_AC ||
					chip->cable_type == CABLE_TYPE_CDP ||
					chip->cable_type == CABLE_TYPE_MISC ||
					chip->cable_type == CABLE_TYPE_UARTOFF ||
					chip->cable_type == CABLE_TYPE_AUDIO_DOCK) {
			gpio_set_value(chip->ovp_gpio, 1);

			pr_err("Enable sec_bat_ext_ovp_work\n");
			schedule_delayed_work(&chip->sec_bat_ext_ovp_work, msecs_to_jiffies(15000));
		}
		#endif
	} else {
		#ifndef CONFIG_NOT_USE_EXT_OVP
		pr_err("FAST_CHG ON IRQ is mismatch.\n");
		gpio_set_value(chip->ovp_gpio, 0);
		#endif
	}

	#if 0
	qpnp_chg_read(chip, &test_reg_val, 0x100B, 1);
	pr_err("Reg Addr: 0x100B, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1010, 1);
	pr_err("Reg Addr: 0x1010, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1049, 1);
	pr_err("Reg Addr: 0x1049, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1310, 1);
	pr_err("Reg Addr: 0x1310, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x130C, 1);
	pr_err("Reg Addr: 0x130C, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x130A, 1);
	pr_err("Reg Addr: 0x130A, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1309, 1);
	pr_err("Reg Addr: 0x1309, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1308, 1);
	pr_err("Reg Addr: 0x1308, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1347, 1);
	pr_err("Reg Addr: 0x1347, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1208, 1);
	pr_err("Reg Addr: 0x1208, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1209, 1);
	pr_err("Reg Addr: 0x1209, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x1210, 1);
	pr_err("Reg Addr: 0x1210, Reg Value: 0x%x\n", test_reg_val);
	qpnp_chg_read(chip, &test_reg_val, 0x120C, 1);
	pr_err("Reg Addr: 0x120C, Reg Value: 0x%x\n", test_reg_val);

	pr_err("Wrote 0xA5 to Reg Addr: 0x10D0\n");
	test_reg_val = 0xA5;
	qpnp_chg_write(chip, &test_reg_val, 0x10D0, 1);
	pr_err("Wrote 0x01 to Reg Addr: 0x10E6\n");
	test_reg_val = 0x01;
	qpnp_chg_write(chip, &test_reg_val, 0x10E6, 1);
	qpnp_chg_read(chip, &test_reg_val, 0x10E7, 1);
	pr_err("Reg Addr: 0x10E7, Reg Value: 0x%x\n", test_reg_val);
	#endif

	return IRQ_HANDLED;
}

static int
qpnp_dc_property_is_writeable(struct power_supply *psy,
						enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		return 1;
	default:
		break;
	}

	return 0;
}

static int
qpnp_batt_property_is_writeable(struct power_supply *psy,
						enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
	case POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL:
	case POWER_SUPPLY_PROP_INPUT_CURRENT_MAX:
	case POWER_SUPPLY_PROP_INPUT_CURRENT_TRIM:
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
	case POWER_SUPPLY_PROP_COOL_TEMP:
	case POWER_SUPPLY_PROP_WARM_TEMP:
#ifdef SEC_CHARGER_CODE
	case POWER_SUPPLY_PROP_ONLINE:
#endif
		return 1;
	default:
		break;
	}

	return 0;
}

static int
qpnp_chg_buck_control(struct qpnp_chg_chip *chip, int enable)
{
	int rc=0;

	if (chip->charging_disabled && enable) {
		pr_debug("Charging disabled\n");
		return 0;
	}

#ifdef SEC_CHARGER_DEBUG
	pr_err("ignore request state(%d) for buck_control \n",enable);
#endif
/*
	rc = qpnp_chg_charge_en(chip, enable);
	if (rc) {
		pr_err("Failed to control charging %d\n", rc);
		return rc;
	}

	rc = qpnp_chg_force_run_on_batt(chip, !enable);
	if (rc)
		pr_err("Failed to control charging %d\n", rc);
*/
	return rc;
}

static int
switch_usb_to_charge_mode(struct qpnp_chg_chip *chip)
{
	int rc;

	pr_debug("switch to charge mode\n");
	if (!qpnp_chg_is_otg_en_set(chip))
		return 0;

	/* enable usb ovp fet */
	rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + CHGR_USB_USB_OTG_CTL,
			USB_OTG_EN_BIT,
			0, 1);
	if (rc) {
		pr_err("Failed to turn on usb ovp rc = %d\n", rc);
		return rc;
	}

	rc = qpnp_chg_force_run_on_batt(chip, chip->charging_disabled);
	if (rc) {
		pr_err("Failed re-enable charging rc = %d\n", rc);
		return rc;
	}

	return 0;
}

static int
switch_usb_to_host_mode(struct qpnp_chg_chip *chip)
{
	int rc;

	pr_debug("switch to host mode\n");
	if (qpnp_chg_is_otg_en_set(chip))
		return 0;

	rc = qpnp_chg_force_run_on_batt(chip, 1);
	if (rc) {
		pr_err("Failed to disable charging rc = %d\n", rc);
		return rc;
	}

	/* force usb ovp fet off */
	rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + CHGR_USB_USB_OTG_CTL,
			USB_OTG_EN_BIT,
			USB_OTG_EN_BIT, 1);
	if (rc) {
		pr_err("Failed to turn off usb ovp rc = %d\n", rc);
		return rc;
	}

	return 0;
}

static enum power_supply_property pm_power_props_mains[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
};

static enum power_supply_property msm_batt_power_props[] = {
	POWER_SUPPLY_PROP_CHARGING_ENABLED,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_INPUT_CURRENT_MAX,
	POWER_SUPPLY_PROP_INPUT_CURRENT_TRIM,
	POWER_SUPPLY_PROP_VOLTAGE_MIN,
	POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_COOL_TEMP,
	POWER_SUPPLY_PROP_WARM_TEMP,
	POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
#ifdef SEC_CHARGER_CODE
	POWER_SUPPLY_PROP_ONLINE,
#endif
};

#ifdef SEC_CHARGER_CODE
static enum power_supply_property sec_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};
#endif

static char *pm_power_supplied_to[] = {
	"battery",
};

static char *pm_batt_supplied_to[] = {
	"bms",
};

static int charger_monitor;
module_param(charger_monitor, int, 0644);

static int ext_ovp_present;
module_param(ext_ovp_present, int, 0644);

#define USB_WALL_THRESHOLD_MA	500
#define OVP_USB_WALL_THRESHOLD_MA	200
static int
qpnp_power_get_property_mains(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct qpnp_chg_chip *chip = container_of(psy, struct qpnp_chg_chip,
								dc_psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 0;
		if (chip->charging_disabled)
			return 0;

		val->intval = qpnp_chg_is_dc_chg_plugged_in(chip);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = chip->maxinput_dc_ma * 1000;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void
qpnp_aicl_check_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct qpnp_chg_chip *chip = container_of(dwork,
				struct qpnp_chg_chip, aicl_check_work);
	union power_supply_propval ret = {0,};

	if (!charger_monitor && qpnp_chg_is_usb_chg_plugged_in(chip)) {
		chip->usb_psy->get_property(chip->usb_psy,
				POWER_SUPPLY_PROP_CURRENT_MAX, &ret);
		if ((ret.intval / 1000) > USB_WALL_THRESHOLD_MA) {
			pr_debug("no charger_monitor present set iusbmax %d\n",
					ret.intval / 1000);
			//Temp : prevent setting iusbmax here before the charger monitor starts to run.
			//qpnp_chg_iusbmax_set(chip, ret.intval / 1000);
		}
	} else {
		pr_debug("charger_monitor is present\n");
	}
	chip->charger_monitor_checked = true;
}

static int
get_prop_battery_voltage_now(struct qpnp_chg_chip *chip)
{
	int rc = 0;
	struct qpnp_vadc_result results;

	if (chip->revision == 0 && chip->type == SMBB) {
		pr_err("vbat reading not supported for 1.0 rc=%d\n", rc);
		return 0;
	} else {
		rc = qpnp_vadc_read(chip->vadc_dev,VBAT_SNS, &results);
		if (rc) {
			pr_err("Unable to read vbat rc=%d\n", rc);
			return 0;
		}
		return results.physical;
	}
}

#define BATT_PRES_BIT BIT(7)
static int
get_prop_batt_present(struct qpnp_chg_chip *chip)
{
	u8 batt_present;
	int rc;
#ifdef SEC_CHARGER_CODE
#if defined(CONFIG_USB_SWITCH_RT8973)
	if (rt_check_jig_state() || rt_uart_connecting)
		return 1;
#elif defined(CONFIG_SM5502_MUIC)
	if (check_sm5502_jig_state() || uart_sm5502_connecting)
		return 1;
#else
	if (check_jig_state() || uart_connecting)
		return 1;
#endif
#endif
	rc = qpnp_chg_read(chip, &batt_present,
				chip->bat_if_base + CHGR_BAT_IF_PRES_STATUS, 1);
	if (rc) {
		pr_err("Couldn't read battery status read failed rc=%d\n", rc);
		return 0;
	};
	return (batt_present & BATT_PRES_BIT) ? 1 : 0;
}

#ifdef SEC_CHARGER_CODE
static int
get_prop_batt_health(struct qpnp_chg_chip *chip)
{
        u8 batt_health;
        int rc;

        rc = qpnp_chg_read(chip, &batt_health,
                                chip->bat_if_base + CHGR_STATUS, 1);
        if (rc) {
                pr_err("Couldn't read battery health read failed rc=%d\n", rc);
                return POWER_SUPPLY_HEALTH_UNKNOWN;
        };
/*
        if (BATT_TEMP_OK & batt_health)
                return POWER_SUPPLY_HEALTH_GOOD;
        if (BATT_TEMP_HOT & batt_health)
                return POWER_SUPPLY_HEALTH_OVERHEAT;
        else
                return POWER_SUPPLY_HEALTH_COLD;
*/
	return chip->batt_health;
}

#else
#define BATT_TEMP_HOT	BIT(6)
#define BATT_TEMP_OK	BIT(7)
static int
get_prop_batt_health(struct qpnp_chg_chip *chip)
{
	u8 batt_health;
	int rc;

	rc = qpnp_chg_read(chip, &batt_health,
				chip->bat_if_base + CHGR_STATUS, 1);
	if (rc) {
		pr_err("Couldn't read battery health read failed rc=%d\n", rc);
		return POWER_SUPPLY_HEALTH_UNKNOWN;
	};

	if (BATT_TEMP_OK & batt_health)
		return POWER_SUPPLY_HEALTH_GOOD;
	if (BATT_TEMP_HOT & batt_health)
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	else
		return POWER_SUPPLY_HEALTH_COLD;
}
#endif

static int
get_prop_charge_type(struct qpnp_chg_chip *chip)
{
	int rc;
	u8 chgr_sts;

	if (!get_prop_batt_present(chip))
		return POWER_SUPPLY_CHARGE_TYPE_NONE;

	rc = qpnp_chg_read(chip, &chgr_sts,
				INT_RT_STS(chip->chgr_base), 1);
	if (rc) {
		pr_err("failed to read interrupt sts %d\n", rc);
		return POWER_SUPPLY_CHARGE_TYPE_NONE;
	}

	if (chgr_sts & TRKL_CHG_ON_IRQ)
		return POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
	if (chgr_sts & FAST_CHG_ON_IRQ)
		return POWER_SUPPLY_CHARGE_TYPE_FAST;

	return POWER_SUPPLY_CHARGE_TYPE_NONE;
}

#ifdef SEC_CHARGER_CODE
static int
get_prop_batt_status(struct qpnp_chg_chip *chip)
{

	/* return real time battery status */
	if ((chip->batt_health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) ||
			(chip->batt_health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE))
		return POWER_SUPPLY_STATUS_DISCHARGING;
	else
		return chip->batt_status;
}
#else
static int
get_prop_batt_status(struct qpnp_chg_chip *chip)
{
	int rc;
	u8 chgr_sts,bat_if_sts;

	if ((qpnp_chg_is_usb_chg_plugged_in(chip) ||
		qpnp_chg_is_dc_chg_plugged_in(chip)) && chip->chg_done) {
		return POWER_SUPPLY_STATUS_FULL;
	}

	rc = qpnp_chg_read(chip, &chgr_sts, INT_RT_STS(chip->chgr_base), 1);
	if (rc) {
		pr_err("failed to read interrupt sts %d\n", rc);
		return POWER_SUPPLY_CHARGE_TYPE_NONE;
	}

	rc = qpnp_chg_read(chip, &bat_if_sts, INT_RT_STS(chip->bat_if_base), 1);
	if (rc) {
		pr_err("failed to read bat_if sts %d\n", rc);
		return POWER_SUPPLY_CHARGE_TYPE_NONE;
	}

	if (chgr_sts & TRKL_CHG_ON_IRQ && bat_if_sts & BAT_FET_ON_IRQ)
		return POWER_SUPPLY_STATUS_CHARGING;
	if (chgr_sts & FAST_CHG_ON_IRQ && bat_if_sts & BAT_FET_ON_IRQ)
		return POWER_SUPPLY_STATUS_CHARGING;

	return POWER_SUPPLY_STATUS_DISCHARGING;
}
#endif

static int
get_prop_current_now(struct qpnp_chg_chip *chip)
{
	union power_supply_propval ret = {0,};

	if (chip->bms_psy) {
		chip->bms_psy->get_property(chip->bms_psy,
			POWER_SUPPLY_PROP_CURRENT_NOW, &ret);
		return ret.intval;
	} else {
		pr_debug("No BMS supply registered return 0\n");
	}

	return 0;
}

static int
get_prop_full_design(struct qpnp_chg_chip *chip)
{
	union power_supply_propval ret = {0,};

	if (chip->bms_psy) {
		chip->bms_psy->get_property(chip->bms_psy,
			POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN, &ret);
		return ret.intval;
	} else {
		pr_debug("No BMS supply registered return 0\n");
	}

	return 0;
}

static int
get_prop_charge_full(struct qpnp_chg_chip *chip)
{
	union power_supply_propval ret = {0,};

	if (chip->bms_psy) {
		chip->bms_psy->get_property(chip->bms_psy,
			POWER_SUPPLY_PROP_CHARGE_FULL, &ret);
		return ret.intval;
	} else {
		pr_debug("No BMS supply registered return 0\n");
	}

	return 0;
}

#define DEFAULT_CAPACITY	50
static int
get_prop_capacity(struct qpnp_chg_chip *chip)
{
	union power_supply_propval ret = {0,};
	int battery_status, bms_status, soc, charger_in;
	//int percent_soc = 0;
#ifdef SEC_CHARGER_CODE
#if defined(CONFIG_USB_SWITCH_RT8973)
	if (chip->use_default_batt_values ||
		(!get_prop_batt_present(chip) && (rt_check_jig_state() || rt_uart_connecting)))
		return DEFAULT_CAPACITY;
#elif defined(CONFIG_SM5502_MUIC)
	if (chip->use_default_batt_values ||
		(!get_prop_batt_present(chip) && (check_sm5502_jig_state() || uart_sm5502_connecting)))
		return DEFAULT_CAPACITY;
#else
	if (chip->use_default_batt_values ||
		(!get_prop_batt_present(chip) && (check_jig_state() || uart_connecting)))
		return DEFAULT_CAPACITY;
#endif
#else
	if (chip->use_default_batt_values || !get_prop_batt_present(chip))
		return DEFAULT_CAPACITY;
#endif
	if (chip->bms_psy) {
		chip->bms_psy->get_property(chip->bms_psy,
				POWER_SUPPLY_PROP_CAPACITY, &ret);
		soc = ret.intval;
#ifdef SEC_CHARGER_CODE
		chip->capacity_raw = soc;
#endif
		battery_status = get_prop_batt_status(chip);
		chip->bms_psy->get_property(chip->bms_psy,
				POWER_SUPPLY_PROP_STATUS, &ret);
		bms_status = ret.intval;
		charger_in = qpnp_chg_is_usb_chg_plugged_in(chip) ||
			qpnp_chg_is_dc_chg_plugged_in(chip);
/*
		if (battery_status != POWER_SUPPLY_STATUS_CHARGING
				&& bms_status != POWER_SUPPLY_STATUS_CHARGING
				&& charger_in
				&& !chip->resuming_charging
				&& !chip->charging_disabled
				&& chip->soc_resume_limit
				&& soc <= chip->soc_resume_limit) {
			pr_debug("resuming charging at %d%% soc\n", soc);
			chip->resuming_charging = true;
			qpnp_chg_set_appropriate_vbatdet(chip);
			//qpnp_chg_charge_en(chip, !chip->charging_disabled);
			sec_pm8226_start_charging(chip);
		}
*/
#if 0 // move to qpnp-bms.c
		soc = sec_fg_get_scaled_capacity(chip, soc * 10) / 10;

		if (soc < 0)
			soc = 0;
		else if (soc > 100)
			soc = 100;
#endif
		chip->recent_reported_soc = soc;

		pr_debug("%s: raw soc (%d), scaled soc (%d)\n",
				__func__, chip->capacity_raw, soc);

		if (soc == 0) {
			if (!qpnp_chg_is_usb_chg_plugged_in(chip)
				&& !qpnp_chg_is_usb_chg_plugged_in(chip))
				pr_warn_ratelimited("Battery 0, CHG absent\n");
		}
		return soc;
/*#ifdef SEC_CHARGER_CODE
		else {
			percent_soc = ret.intval;
			chip->capacity_raw = percent_soc;

			percent_soc = sec_fg_get_scaled_capacity(chip,
				percent_soc * 10) / 10;

			if (percent_soc < 0)
				percent_soc = 0;
			else if (percent_soc > 100)
				percent_soc = 100;

			chip->recent_reported_soc = percent_soc;

			return percent_soc;
		}
#else
			return ret.intval;
#endif*/
	} else {
		pr_debug("No BMS supply registered return 50\n");
	}

	/* return default capacity to avoid userspace
	 * from shutting down unecessarily */
	return DEFAULT_CAPACITY;
}

#if defined(CONFIG_SEC_KANAS_PROJECT)
#define BATT_THERM_ADC_CHANNEL P_MUX3_1_1
#else
#define BATT_THERM_ADC_CHANNEL LR_MUX1_BATT_THERM
#endif

#define DEFAULT_TEMP		250
#define MAX_TOLERABLE_BATT_TEMP_DDC	680
static int
get_prop_batt_temp(struct qpnp_chg_chip *chip)
{
	int rc = 0;
	struct qpnp_vadc_result results;

/* bug-fix to read temperature without battery */
#ifndef SEC_CHARGER_CODE
	if (chip->use_default_batt_values || !get_prop_batt_present(chip))
		return DEFAULT_TEMP;
#endif

	rc = qpnp_vadc_read(chip->vadc_dev,BATT_THERM_ADC_CHANNEL, &results);
	if (rc) {
		pr_debug("Unable to read batt temperature rc=%d\n", rc);
		return 0;
	}
	pr_debug("get_bat_temp %d %lld\n",
		results.adc_code, results.physical);

	pr_err("get_bat_temp: adc_code(%d) physical (%lld)\n",
		results.adc_code, results.physical);

	return (int)results.physical;

}

#ifdef SEC_CHARGER_CODE
static int
get_prop_batt_temp_adc(struct qpnp_chg_chip *chip)
{
	int rc = 0;
	struct qpnp_vadc_result results;

	rc = qpnp_vadc_read(chip->vadc_dev,BATT_THERM_ADC_CHANNEL, &results);
	if (rc) {
		pr_debug("Unable to read batt temperature rc=%d\n", rc);
		return 0;
	}
	pr_debug("get_bat_temp %d %lld\n",
		results.adc_code, results.physical);

	pr_err("get_bat_temp: adc_code(%d) physical (%lld)\n",
		results.adc_code, results.physical);

	return (int)results.adc_code;

}
#endif

static int get_prop_cycle_count(struct qpnp_chg_chip *chip)
{
	union power_supply_propval ret = {0,};

	if (chip->bms_psy)
		chip->bms_psy->get_property(chip->bms_psy,
			POWER_SUPPLY_PROP_CYCLE_COUNT, &ret);
	return ret.intval;
}

static int get_prop_vchg_loop(struct qpnp_chg_chip *chip)
{
	u8 buck_sts;
	int rc;

	rc = qpnp_chg_read(chip, &buck_sts, INT_RT_STS(chip->buck_base), 1);

	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n",
				INT_RT_STS(chip->buck_base), rc);
		return rc;
	}
	pr_debug("buck usb sts 0x%x\n", buck_sts);

	return (buck_sts & VCHG_LOOP_IRQ) ? 1 : 0;
}
static int get_prop_online(struct qpnp_chg_chip *chip)
{
	return qpnp_chg_is_batfet_closed(chip);
}

#ifdef SEC_CHARGER_CODE
static void
qpnp_batt_external_power_changed(struct power_supply *psy)
{
	struct qpnp_chg_chip *chip = container_of(psy, struct qpnp_chg_chip,
								batt_psy);
#if 1
	if (!chip->bms_psy)
		chip->bms_psy = power_supply_get_by_name("bms");
	pr_err("skip qpnp_batt_external_power_changed\n");
#else
	union power_supply_propval ret = {0,};

	if (!chip->bms_psy)
		chip->bms_psy = power_supply_get_by_name("bms");

	chip->usb_psy->get_property(chip->usb_psy,
			POWER_SUPPLY_PROP_ONLINE, &ret);

	/* Only honour requests while USB is present */
	if (qpnp_chg_is_usb_chg_plugged_in(chip)) {
		chip->usb_psy->get_property(chip->usb_psy,
			POWER_SUPPLY_PROP_CURRENT_MAX, &ret);
		if ((ret.intval / 1000) <= 100) {
			pr_err("skip chg current(%dmA) \n", ret.intval/1000);
			return;
		}
		if (ret.intval <= 2 && !chip->use_default_batt_values &&
						get_prop_batt_present(chip)) {
			qpnp_chg_usb_suspend_enable(chip, 0);
			if(chip->cable_type == CABLE_TYPE_USB) {
				qpnp_chg_iusbmax_set(chip,500);
				pr_err("setting chg current(%dmA) \n",500);
			} else {
				qpnp_chg_iusbmax_set(chip, QPNP_CHG_I_MAX_MIN_100);
				pr_err("setting chg current(%dmA) \n",QPNP_CHG_I_MAX_MIN_100);
			}
			//qpnp_chg_usb_suspend_enable(chip, 1);

		} else {
			qpnp_chg_usb_suspend_enable(chip, 0);

			if(chip->cable_type == CABLE_TYPE_USB) {
				if (((ret.intval / 1000) > USB_WALL_THRESHOLD_MA)
						&& (charger_monitor || !chip->charger_monitor_checked)) {
#ifdef SEC_CHARGER_DEBUG
					pr_err("ignore request to set (%dmA) for SDP \n",
							(ret.intval / 1000));
#endif
					/* limit current to 500mA when SDP is connected */
					if (!ext_ovp_present)
						qpnp_chg_iusbmax_set(chip,
								USB_WALL_THRESHOLD_MA);
					else
						qpnp_chg_iusbmax_set(chip,
								OVP_USB_WALL_THRESHOLD_MA);
				} else {
					qpnp_chg_iusbmax_set(chip, ret.intval / 1000);
				}
				/* limit current to 500mA when SDP is connected */
				qpnp_chg_iusbmax_set(chip,500);

			} else {
				/* for all other cable type */
				qpnp_chg_iusbmax_set(chip, ret.intval / 1000);
#ifdef SEC_CHARGER_DEBUG
				pr_err("set current (%dmA) for cable(%d)\n",(ret.intval / 1000),
					chip->cable_type);
#endif
			}

			if ((chip->flags & POWER_STAGE_WA)
			&& ((ret.intval / 1000) > USB_WALL_THRESHOLD_MA)
			&& !chip->power_stage_workaround_running) {
				chip->power_stage_workaround_running = true;
				pr_debug("usb wall chg inserted starting power stage workaround charger_monitor = %d\n",
						charger_monitor);
				schedule_work(&chip->reduce_power_stage_work);
			}
		}
		//bkj - Check it out...
		chip->prev_usb_max_ma = ret.intval;
	}

	pr_debug("end of power supply changed\n");
	//pr_debug("psy changed batt_psy\n");
	//power_supply_changed(&chip->batt_psy);
#endif
}
#else
static void
qpnp_batt_external_power_changed(struct power_supply *psy)
{
        struct qpnp_chg_chip *chip = container_of(psy, struct qpnp_chg_chip,
                                                                batt_psy);
        union power_supply_propval ret = {0,};

        if (!chip->bms_psy)
                chip->bms_psy = power_supply_get_by_name("bms");

	chip->usb_psy->get_property(chip->usb_psy, POWER_SUPPLY_PROP_ONLINE, &ret);

        /* Only honour requests while USB is present */
        if (qpnp_chg_is_usb_chg_plugged_in(chip)) {
                chip->usb_psy->get_property(chip->usb_psy,
                          POWER_SUPPLY_PROP_CURRENT_MAX, &ret);

		if (chip->prev_usb_max_ma == ret.intval)
			goto skip_set_iusb_max;

		if (ret.intval <= 2 && !chip->use_default_batt_values
			&& get_prop_batt_present(chip)) {
                        qpnp_chg_usb_suspend_enable(chip, 1);
                        qpnp_chg_iusbmax_set(chip, QPNP_CHG_I_MAX_MIN_100);
                } else {
                        qpnp_chg_usb_suspend_enable(chip, 0);
                        if (((ret.intval / 1000) > USB_WALL_THRESHOLD_MA)
					&& (charger_monitor)) {
				qpnp_chg_iusbmax_set(chip,
						USB_WALL_THRESHOLD_MA);
			} else {
				qpnp_chg_iusbmax_set(chip, ret.intval / 1000);
			}
                }
		chip->prev_usb_max_ma = ret.intval;
        }

skip_set_iusb_max:
        pr_debug("end of power supply changed\n");
        power_supply_changed(&chip->batt_psy);
}
#endif

static int
qpnp_batt_power_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct qpnp_chg_chip *chip = container_of(psy, struct qpnp_chg_chip,
								batt_psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = get_prop_batt_status(chip);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = get_prop_charge_type(chip);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = get_prop_batt_health(chip);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = get_prop_batt_present(chip);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = chip->max_voltage_mv * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = chip->min_voltage_mv * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = get_prop_battery_voltage_now(chip);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = get_prop_batt_temp(chip);
		break;
	case POWER_SUPPLY_PROP_COOL_TEMP:
		val->intval = chip->cool_bat_decidegc;
		break;
	case POWER_SUPPLY_PROP_WARM_TEMP:
		val->intval = chip->warm_bat_decidegc;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
#ifdef SEC_CHARGER_CODE
		if (chip->batt_status == POWER_SUPPLY_STATUS_FULL)
			val->intval = 100;
		else if (chip->ui_full_cnt)
			val->intval = chip->recent_reported_soc;
		else
			val->intval = get_prop_capacity(chip);
#else
		val->intval = get_prop_capacity(chip);
#endif
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = get_prop_current_now(chip);
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = (get_prop_current_now(chip) * -1) / 1000;
		if (get_prop_battery_voltage_now(chip) > QPNP_CHG_VDDMAX_MIN * 1000
				&& val->intval < 0)
			val->intval = 0;

		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = get_prop_full_design(chip);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = get_prop_charge_full(chip);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = !(chip->charging_disabled);
		break;
	case POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL:
		val->intval = chip->therm_lvl_sel;
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		val->intval = get_prop_cycle_count(chip);
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_REGULATION:
		val->intval = get_prop_vchg_loop(chip);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_MAX:
		val->intval = qpnp_chg_usb_iusbmax_get(chip) * 1000;
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_TRIM:
		val->intval = qpnp_chg_iusb_trim_get(chip);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
		val->intval = qpnp_chg_vinmin_get(chip) * 1000;
		break;
	case POWER_SUPPLY_PROP_BATFET:
		val->intval = get_prop_online(chip);
		break;
#ifdef SEC_CHARGER_CODE
	case POWER_SUPPLY_PROP_ONLINE:
		switch (chip->cable_type) {
		case CABLE_TYPE_NONE:
			val->intval = POWER_SUPPLY_TYPE_BATTERY;
			break;
		case CABLE_TYPE_USB:
			val->intval = POWER_SUPPLY_TYPE_USB;
			break;
		case CABLE_TYPE_AC:
			val->intval = POWER_SUPPLY_TYPE_MAINS;
			break;
		case CABLE_TYPE_MISC:
			val->intval = POWER_SUPPLY_TYPE_MISC;
			break;
		case CABLE_TYPE_UARTOFF:
			val->intval = POWER_SUPPLY_TYPE_UARTOFF;
			break;
		case CABLE_TYPE_CDP:
			val->intval = POWER_SUPPLY_TYPE_USB_CDP;
			break;
/*
		case CABLE_TYPE_OTG:
			val->intval = POWER_SUPPLY_TYPE_OTG;
			break;
*/
		case CABLE_TYPE_CARDOCK:
			val->intval = POWER_SUPPLY_TYPE_CARDOCK;
			break;
		case CABLE_TYPE_DESK_DOCK:
			val->intval = POWER_SUPPLY_TYPE_MISC;
			break;
		case CABLE_TYPE_INCOMPATIBLE:
			val->intval = POWER_SUPPLY_TYPE_UNKNOWN;
			break;
		default:
			val->intval = POWER_SUPPLY_TYPE_UNKNOWN;
			break;
		}
		break;

#endif
	default:
		return -EINVAL;
	}

	return 0;
}

#define BTC_CONFIG_ENABLED	BIT(7)
#define BTC_COLD		BIT(1)
#define BTC_HOT			BIT(0)
static int
qpnp_chg_bat_if_configure_btc(struct qpnp_chg_chip *chip)
{
	u8 btc_cfg = 0, mask = 0;

	/* Do nothing if battery peripheral not present */
	if (!chip->bat_if_base)
		return 0;

	if ((chip->hot_batt_p == HOT_THD_25_PCT)
			|| (chip->hot_batt_p == HOT_THD_35_PCT)) {
		btc_cfg |= btc_value[chip->hot_batt_p];
		mask |= BTC_HOT;
	}

	if ((chip->cold_batt_p == COLD_THD_70_PCT) ||
			(chip->cold_batt_p == COLD_THD_80_PCT)) {
		btc_cfg |= btc_value[chip->cold_batt_p];
		mask |= BTC_COLD;
	}

	if (chip->btc_disabled)
		mask |= BTC_CONFIG_ENABLED;

	return qpnp_chg_masked_write(chip,
			chip->bat_if_base + BAT_IF_BTC_CTRL,
			mask, btc_cfg, 1);
}

#define QPNP_CHG_IBATSAFE_MIN_MA		100
#define QPNP_CHG_IBATSAFE_MAX_MA		3250
#define QPNP_CHG_I_STEP_MA		50
#define QPNP_CHG_I_MIN_MA		100
#define QPNP_CHG_I_MASK			0x3F
static int
qpnp_chg_ibatsafe_set(struct qpnp_chg_chip *chip, int safe_current)
{
	u8 temp;

	if (safe_current < QPNP_CHG_IBATSAFE_MIN_MA
			|| safe_current > QPNP_CHG_IBATSAFE_MAX_MA) {
		pr_err("bad mA=%d asked to set\n", safe_current);
		return -EINVAL;
	}

	temp = safe_current / QPNP_CHG_I_STEP_MA;

	return qpnp_chg_masked_write(chip,
			chip->chgr_base + CHGR_IBAT_SAFE,
			QPNP_CHG_I_MASK, temp, 1);
}

#ifdef SEC_CHARGER_CODE
#define QPNP_CHG_ITERM_MIN_MA           100
#define QPNP_CHG_ITERM_MAX_MA           250
#define QPNP_CHG_ITERM_STEP_MA          50
#define QPNP_CHG_ITERM_MASK             0x03
static int
qpnp_chg_ibatterm_set(struct qpnp_chg_chip *chip, int term_current)
{
	u8 temp;
	u8 ibat_term_reg = 0;
	int rc;

	if (term_current < QPNP_CHG_ITERM_MIN_MA
		|| term_current > QPNP_CHG_ITERM_MAX_MA) {
		pr_err("bad mA=%d asked to set, setting %dmA instead\n",
			term_current,QPNP_CHG_ITERM_MIN_MA);

		temp = (QPNP_CHG_ITERM_MIN_MA - QPNP_CHG_ITERM_MIN_MA)
                                / QPNP_CHG_ITERM_STEP_MA;
		rc = qpnp_chg_masked_write(chip,
				chip->chgr_base + CHGR_IBAT_TERM_CHGR,
			QPNP_CHG_ITERM_MASK, temp, 1);

		qpnp_chg_read(chip, &ibat_term_reg,
			chip->chgr_base + CHGR_IBAT_TERM_CHGR, 1);

		pr_err("REG IBAT_TERM_CHGR(%x) : %x \n",
			chip->chgr_base + CHGR_IBAT_TERM_CHGR,ibat_term_reg);

		return rc;
	}

	temp = (term_current - QPNP_CHG_ITERM_MIN_MA)
				/ QPNP_CHG_ITERM_STEP_MA;
	return qpnp_chg_masked_write(chip,
			chip->chgr_base + CHGR_IBAT_TERM_CHGR,
			QPNP_CHG_ITERM_MASK, temp, 1);
}

#else
#define QPNP_CHG_ITERM_MIN_MA		100
#define QPNP_CHG_ITERM_MAX_MA		250
#define QPNP_CHG_ITERM_STEP_MA		50
#define QPNP_CHG_ITERM_MASK			0x03
static int
qpnp_chg_ibatterm_set(struct qpnp_chg_chip *chip, int term_current)
{
	u8 temp;

	if (term_current < QPNP_CHG_ITERM_MIN_MA
			|| term_current > QPNP_CHG_ITERM_MAX_MA) {
		pr_err("bad mA=%d asked to set\n", term_current);
		return -EINVAL;
	}

	temp = (term_current - QPNP_CHG_ITERM_MIN_MA)
				/ QPNP_CHG_ITERM_STEP_MA;
	return qpnp_chg_masked_write(chip,
			chip->chgr_base + CHGR_IBAT_TERM_CHGR,
			QPNP_CHG_ITERM_MASK, temp, 1);
}
#endif

#define QPNP_CHG_IBATMAX_MIN	50
#define QPNP_CHG_IBATMAX_MAX	3250
static int
qpnp_chg_ibatmax_set(struct qpnp_chg_chip *chip, int chg_current)
{
	u8 temp;

	if (chg_current < QPNP_CHG_IBATMAX_MIN
			|| chg_current > QPNP_CHG_IBATMAX_MAX) {
		pr_err("bad mA=%d asked to set\n", chg_current);
		return -EINVAL;
	}
	temp = chg_current / QPNP_CHG_I_STEP_MA;
	return qpnp_chg_masked_write(chip, chip->chgr_base + CHGR_IBAT_MAX,
			QPNP_CHG_I_MASK, temp, 1);
}

#define QPNP_CHG_TCHG_MASK	0x7F
#define QPNP_CHG_TCHG_MIN	4
#define QPNP_CHG_TCHG_MAX	512
#define QPNP_CHG_TCHG_STEP	4
static int qpnp_chg_tchg_max_set(struct qpnp_chg_chip *chip, int minutes)
{
	u8 temp;

	if (minutes < QPNP_CHG_TCHG_MIN || minutes > QPNP_CHG_TCHG_MAX) {
		pr_err("bad max minutes =%d asked to set\n", minutes);
		return -EINVAL;
	}

	temp = (minutes - 1)/QPNP_CHG_TCHG_STEP;
	return qpnp_chg_masked_write(chip, chip->chgr_base + CHGR_TCHG_MAX,
			QPNP_CHG_TCHG_MASK, temp, 1);
}

static int
qpnp_chg_vddsafe_set(struct qpnp_chg_chip *chip, int voltage)
{
	u8 temp;

	if (voltage < QPNP_CHG_V_MIN_MV
			|| voltage > QPNP_CHG_V_MAX_MV) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}
	temp = (voltage - QPNP_CHG_V_MIN_MV) / QPNP_CHG_V_STEP_MV;
	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return qpnp_chg_write(chip, &temp,
		chip->chgr_base + CHGR_VDD_SAFE, 1);
}

#define BOOST_MIN_UV	4200000
#define BOOST_MAX_UV	5500000
#define BOOST_STEP_UV	50000
#define BOOST_MIN	16
#define N_BOOST_V	((BOOST_MAX_UV - BOOST_MIN_UV) / BOOST_STEP_UV + 1)
static int
qpnp_boost_vset(struct qpnp_chg_chip *chip, int voltage)
{
	u8 reg = 0;

	if (voltage < BOOST_MIN_UV || voltage > BOOST_MAX_UV) {
		pr_err("invalid voltage requested %d uV\n", voltage);
		return -EINVAL;
	}

	reg = DIV_ROUND_UP(voltage - BOOST_MIN_UV, BOOST_STEP_UV) + BOOST_MIN;

	pr_debug("voltage=%d setting %02x\n", voltage, reg);
	return qpnp_chg_write(chip, &reg, chip->boost_base + BOOST_VSET, 1);
}

static int
qpnp_boost_vget_uv(struct qpnp_chg_chip *chip)
{
	int rc;
	u8 boost_reg;

	rc = qpnp_chg_read(chip, &boost_reg,
		 chip->boost_base + BOOST_VSET, 1);
	if (rc) {
		pr_err("failed to read BOOST_VSET rc=%d\n", rc);
		return rc;
	}

	if (boost_reg < BOOST_MIN) {
		pr_err("Invalid reading from 0x%x\n", boost_reg);
		return -EINVAL;
	}

	return BOOST_MIN_UV + ((boost_reg - BOOST_MIN) * BOOST_STEP_UV);
}

static void
qpnp_chg_set_appropriate_battery_current(struct qpnp_chg_chip *chip)
{
	unsigned int chg_current = chip->max_bat_chg_current;

	if (chip->bat_is_cool)
		chg_current = min(chg_current, chip->cool_bat_chg_ma);

	if (chip->bat_is_warm)
		chg_current = min(chg_current, chip->warm_bat_chg_ma);

	if (chip->therm_lvl_sel != 0 && chip->thermal_mitigation)
		chg_current = min(chg_current,
			chip->thermal_mitigation[chip->therm_lvl_sel]);

	pr_debug("setting %d mA\n", chg_current);
	qpnp_chg_ibatmax_set(chip, chg_current);
}

static void
qpnp_batt_system_temp_level_set(struct qpnp_chg_chip *chip, int lvl_sel)
{
	if (lvl_sel >= 0 && lvl_sel < chip->thermal_levels) {
		chip->therm_lvl_sel = lvl_sel;
		if (lvl_sel == (chip->thermal_levels - 1)) {
			/* disable charging if highest value selected */
			qpnp_chg_buck_control(chip, 0);
		} else {
			qpnp_chg_buck_control(chip, 1);
			qpnp_chg_set_appropriate_battery_current(chip);
		}
	} else {
		pr_err("Unsupported level selected %d\n", lvl_sel);
	}
}

/* OTG regulator operations */
static int
qpnp_chg_regulator_otg_enable(struct regulator_dev *rdev)
{
	struct qpnp_chg_chip *chip = rdev_get_drvdata(rdev);

	return switch_usb_to_host_mode(chip);
}

static int
qpnp_chg_regulator_otg_disable(struct regulator_dev *rdev)
{
	struct qpnp_chg_chip *chip = rdev_get_drvdata(rdev);

	return switch_usb_to_charge_mode(chip);
}

static int
qpnp_chg_regulator_otg_is_enabled(struct regulator_dev *rdev)
{
	struct qpnp_chg_chip *chip = rdev_get_drvdata(rdev);

	return qpnp_chg_is_otg_en_set(chip);
}

static int
qpnp_chg_regulator_boost_enable(struct regulator_dev *rdev)
{
	struct qpnp_chg_chip *chip = rdev_get_drvdata(rdev);
	int rc;

	if (qpnp_chg_is_usb_chg_plugged_in(chip) &&
			(chip->flags & BOOST_FLASH_WA)) {
		qpnp_chg_usb_suspend_enable(chip, 1);

		rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + SEC_ACCESS,
			0xFF,
			0xA5, 1);
		if (rc) {
			pr_err("failed to write SEC_ACCESS rc=%d\n", rc);
			return rc;
		}

		rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + COMP_OVR1,
			0xFF,
			0x2F, 1);
		if (rc) {
			pr_err("failed to write COMP_OVR1 rc=%d\n", rc);
			return rc;
		}
	}

	return qpnp_chg_masked_write(chip,
		chip->boost_base + BOOST_ENABLE_CONTROL,
		BOOST_PWR_EN,
		BOOST_PWR_EN, 1);
}

/* Boost regulator operations */
#define ABOVE_VBAT_WEAK		BIT(1)
static int
qpnp_chg_regulator_boost_disable(struct regulator_dev *rdev)
{
	struct qpnp_chg_chip *chip = rdev_get_drvdata(rdev);
	int rc;
	u8 vbat_sts;

	rc = qpnp_chg_masked_write(chip,
		chip->boost_base + BOOST_ENABLE_CONTROL,
		BOOST_PWR_EN,
		0, 1);
	if (rc) {
		pr_err("failed to disable boost rc=%d\n", rc);
		return rc;
	}

	rc = qpnp_chg_read(chip, &vbat_sts,
			chip->chgr_base + CHGR_VBAT_STATUS, 1);
	if (rc) {
		pr_err("failed to read bat sts rc=%d\n", rc);
		return rc;
	}

	if (!(vbat_sts & ABOVE_VBAT_WEAK) && (chip->flags & BOOST_FLASH_WA)) {
		rc = qpnp_chg_masked_write(chip,
			chip->chgr_base + SEC_ACCESS,
			0xFF,
			0xA5, 1);
		if (rc) {
			pr_err("failed to write SEC_ACCESS rc=%d\n", rc);
			return rc;
		}

		rc = qpnp_chg_masked_write(chip,
			chip->chgr_base + COMP_OVR1,
			0xFF,
			0x20, 1);
		if (rc) {
			pr_err("failed to write COMP_OVR1 rc=%d\n", rc);
			return rc;
		}

		usleep(2000);

		rc = qpnp_chg_masked_write(chip,
			chip->chgr_base + SEC_ACCESS,
			0xFF,
			0xA5, 1);
		if (rc) {
			pr_err("failed to write SEC_ACCESS rc=%d\n", rc);
			return rc;
		}

		rc = qpnp_chg_masked_write(chip,
			chip->chgr_base + COMP_OVR1,
			0xFF,
			0x00, 1);
		if (rc) {
			pr_err("failed to write COMP_OVR1 rc=%d\n", rc);
			return rc;
		}
	}

	if (qpnp_chg_is_usb_chg_plugged_in(chip)
			&& (chip->flags & BOOST_FLASH_WA)) {
		rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + SEC_ACCESS,
			0xFF,
			0xA5, 1);
		if (rc) {
			pr_err("failed to write SEC_ACCESS rc=%d\n", rc);
			return rc;
		}

		rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + COMP_OVR1,
			0xFF,
			0x00, 1);
		if (rc) {
			pr_err("failed to write COMP_OVR1 rc=%d\n", rc);
			return rc;
		}

		usleep(1000);

		qpnp_chg_usb_suspend_enable(chip, 0);
	}

	return rc;
}

static int
qpnp_chg_regulator_boost_is_enabled(struct regulator_dev *rdev)
{
	struct qpnp_chg_chip *chip = rdev_get_drvdata(rdev);

	return qpnp_chg_is_boost_en_set(chip);
}

static int
qpnp_chg_regulator_boost_set_voltage(struct regulator_dev *rdev,
		int min_uV, int max_uV, unsigned *selector)
{
	int uV = min_uV;
	int rc;
	struct qpnp_chg_chip *chip = rdev_get_drvdata(rdev);

	if (uV < BOOST_MIN_UV && max_uV >= BOOST_MIN_UV)
		uV = BOOST_MIN_UV;


	if (uV < BOOST_MIN_UV || uV > BOOST_MAX_UV) {
		pr_err("request %d uV is out of bounds\n", uV);
		return -EINVAL;
	}

	*selector = DIV_ROUND_UP(uV - BOOST_MIN_UV, BOOST_STEP_UV);
	if ((*selector * BOOST_STEP_UV + BOOST_MIN_UV) > max_uV) {
		pr_err("no available setpoint [%d, %d] uV\n", min_uV, max_uV);
		return -EINVAL;
	}

	rc = qpnp_boost_vset(chip, uV);

	return rc;
}

static int
qpnp_chg_regulator_boost_get_voltage(struct regulator_dev *rdev)
{
	struct qpnp_chg_chip *chip = rdev_get_drvdata(rdev);

	return qpnp_boost_vget_uv(chip);
}

static int
qpnp_chg_regulator_boost_list_voltage(struct regulator_dev *rdev,
			unsigned selector)
{
	if (selector >= N_BOOST_V)
		return 0;

	return BOOST_MIN_UV + (selector * BOOST_STEP_UV);
}

static struct regulator_ops qpnp_chg_otg_reg_ops = {
	.enable			= qpnp_chg_regulator_otg_enable,
	.disable		= qpnp_chg_regulator_otg_disable,
	.is_enabled		= qpnp_chg_regulator_otg_is_enabled,
};

static struct regulator_ops qpnp_chg_boost_reg_ops = {
	.enable			= qpnp_chg_regulator_boost_enable,
	.disable		= qpnp_chg_regulator_boost_disable,
	.is_enabled		= qpnp_chg_regulator_boost_is_enabled,
	.set_voltage		= qpnp_chg_regulator_boost_set_voltage,
	.get_voltage		= qpnp_chg_regulator_boost_get_voltage,
	.list_voltage		= qpnp_chg_regulator_boost_list_voltage,
};

#define BATFET_LPM_MASK		0xC0
#define BATFET_LPM		0x40
#define BATFET_NO_LPM		0x00
static int
qpnp_chg_bat_if_batfet_reg_enabled(struct qpnp_chg_chip *chip)
{
	int rc = 0;
	u8 reg = 0;

	if (!chip->bat_if_base)
		return rc;

	if (chip->type == SMBB)
		rc = qpnp_chg_read(chip, &reg,
				chip->bat_if_base + CHGR_BAT_IF_SPARE, 1);
	else
		rc = qpnp_chg_read(chip, &reg,
			chip->bat_if_base + CHGR_BAT_IF_BATFET_CTRL4, 1);

	if (rc) {
		pr_err("failed to read batt_if rc=%d\n", rc);
		return rc;
	}

	if ((reg & BATFET_LPM_MASK) == BATFET_NO_LPM)
		return 1;

	return 0;
}

static int
qpnp_chg_regulator_batfet_set(struct qpnp_chg_chip *chip, bool enable)
{
	int rc = 0;
	static int ULPM=0;

	/*  if (chip->charging_disabled || !chip->bat_if_base)  */
	if (ULPM || chip->charging_disabled || !chip->bat_if_base) {
		pr_err("chip->charging_disabled=%d, chip->bat_if_base=%d\n", chip->charging_disabled, chip->bat_if_base);
		return rc;
	}

	if (chip->type == SMBB)
	{
		rc = qpnp_chg_masked_write(chip,
			chip->bat_if_base + CHGR_BAT_IF_SPARE,
			BATFET_LPM_MASK,
			enable ? BATFET_NO_LPM : BATFET_LPM, 1);
		 pr_err("Executed qpnp_chg_regulator_batfet_set of SMBB  ULPM=%d\n", ULPM);
	}
	else
	{
		rc = qpnp_chg_masked_write(chip,
			chip->bat_if_base + CHGR_BAT_IF_BATFET_CTRL4,
			BATFET_LPM_MASK,
			enable ? BATFET_NO_LPM : BATFET_LPM, 1);
		pr_err("Executed qpnp_chg_regulator_batfet_set else of SMBB  ULPM=%d\n", ULPM);
	}
	ULPM=1;
	return rc;
}

static int
qpnp_chg_regulator_batfet_enable(struct regulator_dev *rdev)
{
	struct qpnp_chg_chip *chip = rdev_get_drvdata(rdev);
	int rc = 0;

	mutex_lock(&chip->batfet_vreg_lock);
	/* Only enable if not already enabled */
	if (!qpnp_chg_bat_if_batfet_reg_enabled(chip)) {
		rc = qpnp_chg_regulator_batfet_set(chip, 1);
		if (rc)
			pr_err("failed to write to batt_if rc=%d\n", rc);
	}

	chip->batfet_ext_en = true;
	mutex_unlock(&chip->batfet_vreg_lock);

	return rc;
}

static int
qpnp_chg_regulator_batfet_disable(struct regulator_dev *rdev)
{
	struct qpnp_chg_chip *chip = rdev_get_drvdata(rdev);
	int rc = 0;

	mutex_lock(&chip->batfet_vreg_lock);
	/* Don't allow disable if charger connected */
	if (!qpnp_chg_is_usb_chg_plugged_in(chip) &&
			!qpnp_chg_is_dc_chg_plugged_in(chip)) {
		rc = qpnp_chg_regulator_batfet_set(chip, 0);
		if (rc)
			pr_err("failed to write to batt_if rc=%d\n", rc);
	}

	chip->batfet_ext_en = false;
	mutex_unlock(&chip->batfet_vreg_lock);

	return rc;

}
static int
qpnp_chg_regulator_batfet_is_enabled(struct regulator_dev *rdev)
{
	struct qpnp_chg_chip *chip = rdev_get_drvdata(rdev);

	return chip->batfet_ext_en;
}


static struct regulator_ops qpnp_chg_batfet_vreg_ops = {
	.enable			= qpnp_chg_regulator_batfet_enable,
	.disable		= qpnp_chg_regulator_batfet_disable,
	.is_enabled		= qpnp_chg_regulator_batfet_is_enabled,
};

#define MIN_DELTA_MV_TO_INCREASE_VDD_MAX	8
#define MAX_DELTA_VDD_MAX_MV			80
#define VDD_MAX_CENTER_OFFSET			4
static void
qpnp_chg_adjust_vddmax(struct qpnp_chg_chip *chip, int vbat_mv)
{
	int delta_mv, closest_delta_mv, sign;

	delta_mv = chip->max_voltage_mv - VDD_MAX_CENTER_OFFSET - vbat_mv;
	if (delta_mv > 0 && delta_mv < MIN_DELTA_MV_TO_INCREASE_VDD_MAX) {
		pr_debug("vbat is not low enough to increase vdd\n");
		return;
	}

	sign = delta_mv > 0 ? 1 : -1;
	closest_delta_mv = ((delta_mv + sign * QPNP_CHG_BUCK_TRIM1_STEP / 2)
			/ QPNP_CHG_BUCK_TRIM1_STEP) * QPNP_CHG_BUCK_TRIM1_STEP;
	pr_debug("max_voltage = %d, vbat_mv = %d, delta_mv = %d, closest = %d\n",
			chip->max_voltage_mv, vbat_mv,
			delta_mv, closest_delta_mv);
	chip->delta_vddmax_mv = clamp(chip->delta_vddmax_mv + closest_delta_mv,
			-MAX_DELTA_VDD_MAX_MV, MAX_DELTA_VDD_MAX_MV);
	pr_debug("using delta_vddmax_mv = %d\n", chip->delta_vddmax_mv);
	qpnp_chg_set_appropriate_vddmax(chip);
}

#define CONSECUTIVE_COUNT	3
#define VBATDET_MAX_ERR_MV	50
static void
qpnp_eoc_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct qpnp_chg_chip *chip = container_of(dwork,
				struct qpnp_chg_chip, eoc_work);
	static int count;
	static int vbat_low_count;
	int ibat_ma, vbat_mv, rc = 0;
	u8 batt_sts = 0, buck_sts = 0, chg_sts = 0;
	bool vbat_lower_than_vbatdet;

	pm_stay_awake(chip->dev);
	qpnp_chg_charge_en(chip, !chip->charging_disabled);

	rc = qpnp_chg_read(chip, &batt_sts, INT_RT_STS(chip->bat_if_base), 1);
	if (rc) {
		pr_err("failed to read batt_if rc=%d\n", rc);
		return;
	}

	rc = qpnp_chg_read(chip, &buck_sts, INT_RT_STS(chip->buck_base), 1);
	if (rc) {
		pr_err("failed to read buck rc=%d\n", rc);
		return;
	}

	rc = qpnp_chg_read(chip, &chg_sts, INT_RT_STS(chip->chgr_base), 1);
	if (rc) {
		pr_err("failed to read chg_sts rc=%d\n", rc);
		return;
	}

	pr_debug("chgr: 0x%x, bat_if: 0x%x, buck: 0x%x\n",
		chg_sts, batt_sts, buck_sts);

	if (!qpnp_chg_is_usb_chg_plugged_in(chip) &&
			!qpnp_chg_is_dc_chg_plugged_in(chip)) {
		pr_debug("no chg connected, stopping\n");
		goto stop_eoc;
	}

	if ((batt_sts & BAT_FET_ON_IRQ) && (chg_sts & FAST_CHG_ON_IRQ
					|| chg_sts & TRKL_CHG_ON_IRQ)) {
		ibat_ma = get_prop_current_now(chip) / 1000;
		vbat_mv = get_prop_battery_voltage_now(chip) / 1000;

		pr_debug("ibat_ma = %d vbat_mv = %d term_current_ma = %d\n",
			ibat_ma, vbat_mv, chip->term_current);

		vbat_lower_than_vbatdet = !(chg_sts & VBAT_DET_LOW_IRQ);
		if (vbat_lower_than_vbatdet && vbat_mv <
				(chip->max_voltage_mv - chip->resume_delta_mv
				 - VBATDET_MAX_ERR_MV)) {
			vbat_low_count++;
			pr_debug("woke up too early vbat_mv = %d, max_mv = %d, resume_mv = %d tolerance_mv = %d low_count = %d\n",
					vbat_mv, chip->max_voltage_mv,
					chip->resume_delta_mv,
					VBATDET_MAX_ERR_MV, vbat_low_count);
			if (vbat_low_count >= CONSECUTIVE_COUNT) {
				pr_debug("woke up too early stopping\n");
				qpnp_chg_enable_irq(&chip->chg_vbatdet_lo);
				goto stop_eoc;
			} else {
				goto check_again_later;
			}
		} else {
			vbat_low_count = 0;
		}

		if (buck_sts & VDD_LOOP_IRQ)
			qpnp_chg_adjust_vddmax(chip, vbat_mv);

		if (!(buck_sts & VDD_LOOP_IRQ)) {
			pr_debug("Not in CV\n");
			count = 0;
		} else if ((ibat_ma * -1) > chip->term_current) {
			pr_debug("Not at EOC, battery current too high\n");
			count = 0;
		} else if (ibat_ma > 0) {
			pr_debug("Charging but system demand increased\n");
			count = 0;
		} else {
			if (count == CONSECUTIVE_COUNT) {
				#ifdef SEC_CHARGER_CODE
				/* stop charging with SAMSUNG function */
				pr_info("End of Charging @ %dmA \n",ibat_ma);
				chip->delta_vddmax_mv = 0;
				qpnp_chg_set_appropriate_vddmax(chip);
				chip->chg_done = true;
				sec_pm8226_stop_charging(chip);
				#else
				pr_info("End of Charging\n");
				chip->delta_vddmax_mv = 0;
				qpnp_chg_set_appropriate_vddmax(chip);
				chip->chg_done = true;
				qpnp_chg_charge_en(chip, 0);
				#endif

				/* sleep for a second before enabling */
				msleep(2000);
				qpnp_chg_charge_en(chip,
						!chip->charging_disabled);
				pr_debug("psy changed batt_psy\n");
				power_supply_changed(&chip->batt_psy);
				qpnp_chg_enable_irq(&chip->chg_vbatdet_lo);
				goto stop_eoc;
			} else {
				count += 1;
				pr_debug("EOC count = %d\n", count);
			}
		}
	} else {
		pr_debug("not charging\n");
		goto stop_eoc;
	}

check_again_later:
	schedule_delayed_work(&chip->eoc_work,
		msecs_to_jiffies(EOC_CHECK_PERIOD_MS));
	return;

stop_eoc:
	vbat_low_count = 0;
	count = 0;
	pm_relax(chip->dev);
}

static void
qpnp_chg_soc_check_work(struct work_struct *work)
{
	struct qpnp_chg_chip *chip = container_of(work,
				struct qpnp_chg_chip, soc_check_work);

	get_prop_capacity(chip);
}

#define HYSTERISIS_DECIDEGC 20
#ifdef SEC_CHARGER_CODE
static void
qpnp_chg_adc_notification(enum qpnp_tm_state state, void *ctx)
{
        struct qpnp_chg_chip *chip = ctx;
        bool bat_warm = 0, bat_cool = 0;
        int temp;

        if (state >= ADC_TM_STATE_NUM) {
                pr_err("invalid notification %d\n", state);
                return;
        }

	temp = get_prop_batt_temp(chip);

	pr_debug("temp = %d state = %s\n", temp,
		state == ADC_TM_WARM_STATE ? "warm" : "cool");

	if (state == ADC_TM_WARM_STATE) {
		if (temp > chip->warm_bat_decidegc) {
			/* Normal to warm */
			bat_warm = true;
			bat_cool = false;
			chip->adc_param.low_temp =
				chip->warm_bat_decidegc - HYSTERISIS_DECIDEGC;
			chip->adc_param.state_request =
				ADC_TM_COOL_THR_ENABLE;
		} else if (temp >
			chip->cool_bat_decidegc + HYSTERISIS_DECIDEGC) {
			/* Cool to normal */
			bat_warm = false;
			bat_cool = false;

			chip->adc_param.low_temp = chip->cool_bat_decidegc;
			chip->adc_param.high_temp = chip->warm_bat_decidegc;
			chip->adc_param.state_request =
				ADC_TM_HIGH_LOW_THR_ENABLE;
		}
	} else {
		if (temp < chip->cool_bat_decidegc) {
			/* Normal to cool */
			bat_warm = false;
			bat_cool = true;
			chip->adc_param.high_temp =
				chip->cool_bat_decidegc + HYSTERISIS_DECIDEGC;
			chip->adc_param.state_request =
				ADC_TM_WARM_THR_ENABLE;
		} else if (temp <
			chip->warm_bat_decidegc - HYSTERISIS_DECIDEGC) {
			/* Warm to normal */
			bat_warm = false;
			bat_cool = false;

			chip->adc_param.low_temp = chip->cool_bat_decidegc;
			chip->adc_param.high_temp = chip->warm_bat_decidegc;
			chip->adc_param.state_request =
				ADC_TM_HIGH_LOW_THR_ENABLE;
		}
	}

	if (chip->bat_is_cool ^ bat_cool || chip->bat_is_warm ^ bat_warm) {
		chip->bat_is_cool = bat_cool;
		chip->bat_is_warm = bat_warm;

		if (bat_cool || bat_warm)
			chip->resuming_charging = false;

		/**
		* set appropriate voltages and currents.
		*
		* Note that when the battery is hot or cold, the charger
		* driver will not resume with SoC. Only vbatdet is used to
		* determine resume of charging.
		*/
		/* SAMSUNG do not chnage charge current and VDDmax*/
		/* qpnp_chg_set_appropriate_vddmax(chip);
		qpnp_chg_set_appropriate_battery_current(chip);
		qpnp_chg_set_appropriate_vbatdet(chip);
		*/
		pr_err("Skip Qualcomm BTM \n");
	}

	if (qpnp_adc_tm_channel_measure(chip->adc_tm_dev,&chip->adc_param))
		pr_err("request ADC error\n");

}
#else
static void
qpnp_chg_adc_notification(enum qpnp_tm_state state, void *ctx)
{
	struct qpnp_chg_chip *chip = ctx;
	bool bat_warm = 0, bat_cool = 0;
	int temp;

	if (state >= ADC_TM_STATE_NUM) {
		pr_err("invalid notification %d\n", state);
		return;
	}

	temp = get_prop_batt_temp(chip);

	pr_debug("temp = %d state = %s\n", temp,
			state == ADC_TM_WARM_STATE ? "warm" : "cool");

	if (state == ADC_TM_WARM_STATE) {
		if (temp > chip->warm_bat_decidegc) {
			/* Normal to warm */
			bat_warm = true;
			bat_cool = false;
			chip->adc_param.low_temp =
				chip->warm_bat_decidegc - HYSTERISIS_DECIDEGC;
			chip->adc_param.state_request =
				ADC_TM_COOL_THR_ENABLE;
		} else if (temp >
				chip->cool_bat_decidegc + HYSTERISIS_DECIDEGC) {
			/* Cool to normal */
			bat_warm = false;
			bat_cool = false;

			chip->adc_param.low_temp = chip->cool_bat_decidegc;
			chip->adc_param.high_temp = chip->warm_bat_decidegc;
			chip->adc_param.state_request =
					ADC_TM_HIGH_LOW_THR_ENABLE;
		}
	} else {
		if (temp < chip->cool_bat_decidegc) {
			/* Normal to cool */
			bat_warm = false;
			bat_cool = true;
			chip->adc_param.high_temp =
				chip->cool_bat_decidegc + HYSTERISIS_DECIDEGC;
			chip->adc_param.state_request =
				ADC_TM_WARM_THR_ENABLE;
		} else if (temp <
				chip->warm_bat_decidegc - HYSTERISIS_DECIDEGC) {
			/* Warm to normal */
			bat_warm = false;
			bat_cool = false;

			chip->adc_param.low_temp = chip->cool_bat_decidegc;
			chip->adc_param.high_temp = chip->warm_bat_decidegc;
			chip->adc_param.state_request =
					ADC_TM_HIGH_LOW_THR_ENABLE;
		}
	}

	if (chip->bat_is_cool ^ bat_cool || chip->bat_is_warm ^ bat_warm) {
		chip->bat_is_cool = bat_cool;
		chip->bat_is_warm = bat_warm;

		if (bat_cool || bat_warm)
			chip->resuming_charging = false;

		/**
		 * set appropriate voltages and currents.
		 *
		 * Note that when the battery is hot or cold, the charger
		 * driver will not resume with SoC. Only vbatdet is used to
		 * determine resume of charging.
		 */
		qpnp_chg_set_appropriate_vddmax(chip);
		qpnp_chg_set_appropriate_battery_current(chip);
		qpnp_chg_set_appropriate_vbatdet(chip);
	}

	pr_debug("warm %d, cool %d, low = %d deciDegC, high = %d deciDegC\n",
			chip->bat_is_warm, chip->bat_is_cool,
			chip->adc_param.low_temp, chip->adc_param.high_temp);

	if (qpnp_adc_tm_channel_measure(chip->adc_tm_dev,&chip->adc_param))
		pr_err("request ADC error\n");
}
#endif

#define MIN_COOL_TEMP	-300
#define MAX_WARM_TEMP	1000

static int
qpnp_chg_configure_jeita(struct qpnp_chg_chip *chip,
		enum power_supply_property psp, int temp_degc)
{
	int rc = 0;

	if ((temp_degc < MIN_COOL_TEMP) || (temp_degc > MAX_WARM_TEMP)) {
		pr_err("Bad temperature request %d\n", temp_degc);
		return -EINVAL;
	}

	mutex_lock(&chip->jeita_configure_lock);
	switch (psp) {
	case POWER_SUPPLY_PROP_COOL_TEMP:
		if (temp_degc >=
			(chip->warm_bat_decidegc - HYSTERISIS_DECIDEGC)) {
			pr_err("Can't set cool %d higher than warm %d - hysterisis %d\n",
					temp_degc, chip->warm_bat_decidegc,
					HYSTERISIS_DECIDEGC);
			rc = -EINVAL;
			goto mutex_unlock;
		}
		if (chip->bat_is_cool)
			chip->adc_param.high_temp =
				temp_degc + HYSTERISIS_DECIDEGC;
		else if (!chip->bat_is_warm)
			chip->adc_param.low_temp = temp_degc;

		chip->cool_bat_decidegc = temp_degc;
		break;
	case POWER_SUPPLY_PROP_WARM_TEMP:
		if (temp_degc <=
			(chip->cool_bat_decidegc + HYSTERISIS_DECIDEGC)) {
			pr_err("Can't set warm %d higher than cool %d + hysterisis %d\n",
					temp_degc, chip->warm_bat_decidegc,
					HYSTERISIS_DECIDEGC);
			rc = -EINVAL;
			goto mutex_unlock;
		}
		if (chip->bat_is_warm)
			chip->adc_param.low_temp =
				temp_degc - HYSTERISIS_DECIDEGC;
		else if (!chip->bat_is_cool)
			chip->adc_param.high_temp = temp_degc;

		chip->warm_bat_decidegc = temp_degc;
		break;
	default:
		rc = -EINVAL;
		goto mutex_unlock;
	}

	schedule_work(&chip->adc_measure_work);

mutex_unlock:
	mutex_unlock(&chip->jeita_configure_lock);
	return rc;
}

#define POWER_STAGE_REDUCE_CHECK_PERIOD_SECONDS		20
#define POWER_STAGE_REDUCE_MAX_VBAT_UV			3900000
#define POWER_STAGE_REDUCE_MIN_VCHG_UV			4800000
#define POWER_STAGE_SEL_MASK				0x0F
#define POWER_STAGE_REDUCED				0x01
#define POWER_STAGE_DEFAULT				0x0F
static bool
qpnp_chg_is_power_stage_reduced(struct qpnp_chg_chip *chip)
{
	int rc;
	u8 reg;

	rc = qpnp_chg_read(chip, &reg,
				 chip->buck_base + CHGR_BUCK_PSTG_CTRL,
				 1);
	if (rc) {
		pr_err("Error %d reading power stage register\n", rc);
		return false;
	}

	if ((reg & POWER_STAGE_SEL_MASK) == POWER_STAGE_DEFAULT)
		return false;

	return true;
}

static int
qpnp_chg_power_stage_set(struct qpnp_chg_chip *chip, bool reduce)
{
	int rc;
	u8 reg = 0xA5;

	rc = qpnp_chg_write(chip, &reg,
				 chip->buck_base + SEC_ACCESS,
				 1);
	if (rc) {
		pr_err("Error %d writing 0xA5 to buck's 0x%x reg\n",
				rc, SEC_ACCESS);
		return rc;
	}

	reg = POWER_STAGE_DEFAULT;
	if (reduce)
		reg = POWER_STAGE_REDUCED;
	rc = qpnp_chg_write(chip, &reg,
				 chip->buck_base + CHGR_BUCK_PSTG_CTRL,
				 1);

	if (rc)
		pr_err("Error %d writing 0x%x power stage register\n", rc, reg);
	return rc;
}

static int
qpnp_chg_get_vusbin_uv(struct qpnp_chg_chip *chip)
{
	int rc = 0;
	struct qpnp_vadc_result results;

	rc = qpnp_vadc_read(chip->vadc_dev, USBIN, &results);
	if (rc) {
		pr_err("Unable to read vbat rc=%d\n", rc);
		return 0;
	}
	return results.physical;
}

static
int get_vusb_averaged(struct qpnp_chg_chip *chip, int sample_count)
{
	int vusb_uv = 0;
	int i;

	/* avoid overflows */
	if (sample_count > 256)
		sample_count = 256;

	for (i = 0; i < sample_count; i++)
		vusb_uv += qpnp_chg_get_vusbin_uv(chip);

	vusb_uv = vusb_uv / sample_count;
	return vusb_uv;
}

static
int get_vbat_averaged(struct qpnp_chg_chip *chip, int sample_count)
{
	int vbat_uv = 0;
	int i;

	/* avoid overflows */
	if (sample_count > 256)
		sample_count = 256;

	for (i = 0; i < sample_count; i++)
		vbat_uv += get_prop_battery_voltage_now(chip);

	vbat_uv = vbat_uv / sample_count;
	return vbat_uv;
}

static void
qpnp_chg_reduce_power_stage(struct qpnp_chg_chip *chip)
{
	struct timespec ts;
	bool power_stage_reduced_in_hw = qpnp_chg_is_power_stage_reduced(chip);
	bool reduce_power_stage = false;
	int vbat_uv = get_vbat_averaged(chip, 16);
	int vusb_uv = get_vusb_averaged(chip, 16);
	bool fast_chg =
		(get_prop_charge_type(chip) == POWER_SUPPLY_CHARGE_TYPE_FAST);
	static int count_restore_power_stage;
	static int count_reduce_power_stage;
	bool vchg_loop = get_prop_vchg_loop(chip);
	bool ichg_loop = qpnp_chg_is_ichg_loop_active(chip);
	bool usb_present = qpnp_chg_is_usb_chg_plugged_in(chip);
	bool usb_ma_above_wall =
		(qpnp_chg_usb_iusbmax_get(chip) > USB_WALL_THRESHOLD_MA);

	if (fast_chg
		&& usb_present
		&& usb_ma_above_wall
		&& vbat_uv < POWER_STAGE_REDUCE_MAX_VBAT_UV
		&& vusb_uv > POWER_STAGE_REDUCE_MIN_VCHG_UV)
		reduce_power_stage = true;

	if ((usb_present && usb_ma_above_wall)
		&& (vchg_loop || ichg_loop))
		reduce_power_stage = true;

	if (power_stage_reduced_in_hw && !reduce_power_stage) {
		count_restore_power_stage++;
		count_reduce_power_stage = 0;
	} else if (!power_stage_reduced_in_hw && reduce_power_stage) {
		count_reduce_power_stage++;
		count_restore_power_stage = 0;
	} else if (power_stage_reduced_in_hw == reduce_power_stage) {
		count_restore_power_stage = 0;
		count_reduce_power_stage = 0;
	}

	pr_debug("power_stage_hw = %d reduce_power_stage = %d usb_present = %d usb_ma_above_wall = %d vbat_uv(16) = %d vusb_uv(16) = %d fast_chg = %d , ichg = %d, vchg = %d, restore,reduce = %d, %d\n",
			power_stage_reduced_in_hw, reduce_power_stage,
			usb_present, usb_ma_above_wall,
			vbat_uv, vusb_uv, fast_chg,
			ichg_loop, vchg_loop,
			count_restore_power_stage, count_reduce_power_stage);

	if (!power_stage_reduced_in_hw && reduce_power_stage) {
		if (count_reduce_power_stage >= 2) {
			qpnp_chg_power_stage_set(chip, true);
			power_stage_reduced_in_hw = true;
		}
	}

	if (power_stage_reduced_in_hw && !reduce_power_stage) {
		if (count_restore_power_stage >= 6
				|| (!usb_present || !usb_ma_above_wall)) {
			qpnp_chg_power_stage_set(chip, false);
			power_stage_reduced_in_hw = false;
		}
	}

	if (usb_present && usb_ma_above_wall) {
		getnstimeofday(&ts);
		ts.tv_sec += POWER_STAGE_REDUCE_CHECK_PERIOD_SECONDS;
		alarm_start_range(&chip->reduce_power_stage_alarm,
					timespec_to_ktime(ts),
					timespec_to_ktime(ts));
	} else {
		pr_debug("stopping power stage workaround\n");
		chip->power_stage_workaround_running = false;
	}
}

static void
qpnp_chg_batfet_lcl_work(struct work_struct *work)
{
	struct qpnp_chg_chip *chip = container_of(work,
				struct qpnp_chg_chip, batfet_lcl_work);

	mutex_lock(&chip->batfet_vreg_lock);
	if (qpnp_chg_is_usb_chg_plugged_in(chip) ||
			qpnp_chg_is_dc_chg_plugged_in(chip)) {
		qpnp_chg_regulator_batfet_set(chip, 1);
		pr_debug("disabled ULPM\n");
	} else if (!chip->batfet_ext_en && !qpnp_chg_is_usb_chg_plugged_in(chip)
			&& !qpnp_chg_is_dc_chg_plugged_in(chip)) {
		qpnp_chg_regulator_batfet_set(chip, 0);
		pr_debug("enabled ULPM\n");
	}
	mutex_unlock(&chip->batfet_vreg_lock);
}

static void
qpnp_chg_reduce_power_stage_work(struct work_struct *work)
{
	struct qpnp_chg_chip *chip = container_of(work,
				struct qpnp_chg_chip, reduce_power_stage_work);

	qpnp_chg_reduce_power_stage(chip);
}

static void
qpnp_chg_reduce_power_stage_callback(struct alarm *alarm)
{
	struct qpnp_chg_chip *chip = container_of(alarm, struct qpnp_chg_chip,
						reduce_power_stage_alarm);

	schedule_work(&chip->reduce_power_stage_work);
}

static int
qpnp_dc_power_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct qpnp_chg_chip *chip = container_of(psy, struct qpnp_chg_chip,
								dc_psy);
	int rc = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		if (!val->intval)
			break;

		rc = qpnp_chg_idcmax_set(chip, val->intval / 1000);
		if (rc) {
			pr_err("Error setting idcmax property %d\n", rc);
			return rc;
		}
		chip->maxinput_dc_ma = (val->intval / 1000);

		break;
	default:
		return -EINVAL;
	}

	pr_debug("psy changed dc_psy\n");
	power_supply_changed(&chip->dc_psy);
	return rc;
}


static int
qpnp_batt_power_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct qpnp_chg_chip *chip = container_of(psy, struct qpnp_chg_chip,
								batt_psy);
	int rc = 0;

	/* SAMSUNG charging specification */
#ifdef SEC_CHARGER_CODE
	enum cable_type_t new_cable_type;
	//int batt_capacity;

#ifdef SEC_CHARGER_DEBUG
	pr_err("set prop called for property (%d) with value (%d) \n",
		psp,val->intval);
#endif

	if (!chip->dev) {
		pr_err("called before init\n");
		goto error_check;
	}
#endif

	switch (psp) {
	case POWER_SUPPLY_PROP_COOL_TEMP:
		rc = qpnp_chg_configure_jeita(chip, psp, val->intval);
		break;
	case POWER_SUPPLY_PROP_WARM_TEMP:
		rc = qpnp_chg_configure_jeita(chip, psp, val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		chip->charging_disabled = !(val->intval);
		if (chip->charging_disabled) {
			/* disable charging */
			qpnp_chg_charge_en(chip, !chip->charging_disabled);
			qpnp_chg_force_run_on_batt(chip,
						chip->charging_disabled);
		} else {
			/* enable charging */
			qpnp_chg_force_run_on_batt(chip,
					chip->charging_disabled);
			qpnp_chg_charge_en(chip, !chip->charging_disabled);
		}
		break;
	case POWER_SUPPLY_PROP_SYSTEM_TEMP_LEVEL:
		qpnp_batt_system_temp_level_set(chip, val->intval);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_MAX:
		qpnp_chg_iusbmax_set(chip, val->intval / 1000);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_TRIM:
		qpnp_chg_iusb_trim_set(chip, val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
		qpnp_chg_vinmin_set(chip, val->intval / 1000);
		break;
#ifdef SEC_CHARGER_CODE
	case POWER_SUPPLY_PROP_ONLINE:
	#if defined(CONFIG_USB_SWITCH_RT8973)
		if (val->intval != POWER_SUPPLY_TYPE_UNKNOWN &&
			val->intval != POWER_SUPPLY_TYPE_BATTERY)
			msleep(300);
	#endif
		/* cable is attached or detached. called by usb switch ic */
		chip->cable_exception = CABLE_TYPE_NONE;
		switch (val->intval) {
		case POWER_SUPPLY_TYPE_BATTERY:
			if (chip->batt_status == POWER_SUPPLY_STATUS_FULL) {
				#if 1 //move to qpnp-bms.c
				if (chip->bms_psy) {
					chip->bms_psy->set_property(chip->bms_psy,
							POWER_SUPPLY_PROP_CHARGE_FULL, val);
				}
				#else
				batt_capacity = get_prop_capacity(chip);
				sec_fg_calculate_dynamic_scale(chip,
						chip->capacity_raw * 10);
				#endif
			}
			new_cable_type = CABLE_TYPE_NONE;
			break;
		case POWER_SUPPLY_TYPE_MAINS:
			new_cable_type = CABLE_TYPE_AC;
			break;
		case POWER_SUPPLY_TYPE_USB:
		case POWER_SUPPLY_TYPE_USB_ACA:
		case POWER_SUPPLY_TYPE_USB_DCP:
			new_cable_type = CABLE_TYPE_USB;
			break;
		case POWER_SUPPLY_TYPE_MISC:
			new_cable_type = CABLE_TYPE_MISC;
			break;
		case POWER_SUPPLY_TYPE_USB_CDP:
			new_cable_type = CABLE_TYPE_CDP;
			break;
		case POWER_SUPPLY_TYPE_CARDOCK:
			if (qpnp_chg_is_usb_chg_plugged_in(chip)) {
				pr_err("%s: cardock connected + VBUS present \n",__func__);
				new_cable_type = CABLE_TYPE_USB;
			} else {
				pr_err("%s: cardock connected but VBUS not present \n",__func__);
				new_cable_type = CABLE_TYPE_CARDOCK;
			}
			break;
		case POWER_SUPPLY_TYPE_UARTOFF:
			new_cable_type = CABLE_TYPE_UARTOFF;
			if (qpnp_chg_is_usb_chg_plugged_in(chip)) {
				pr_err("%s: UART connected + VBUS present \n",__func__);
			} else {
				pr_err("%s: UART connected but VBUS not present \n",__func__);
			}
			break;
		case POWER_SUPPLY_TYPE_UNKNOWN:
			new_cable_type = CABLE_TYPE_INCOMPATIBLE;
			break;
		default:
			return -EINVAL;
		}

		if (new_cable_type == chip->cable_type	/*&& !ovp_state*/) {
			pr_err("%s: same cable, no change in cable type (%d) \n",
				__func__,chip->cable_type);
		} else {
			pr_err("%s: cable type changed (%d) -> (%d) \n",__func__,
					chip->cable_type,new_cable_type);
			chip->cable_type = new_cable_type;
			sec_handle_cable_insertion_removal(chip);
		}
		break;
#endif
	default:
		return -EINVAL;

	}

	pr_debug("psy changed batt_psy\n");
	power_supply_changed(&chip->batt_psy);
	return rc;

#ifdef SEC_CHARGER_CODE
error_check:
	return 0;
#endif
}

static int
qpnp_chg_setup_flags(struct qpnp_chg_chip *chip)
{
	if (chip->revision > 0 && chip->type == SMBB)
		chip->flags |= CHG_FLAGS_VCP_WA;
	if (chip->type == SMBB)
		chip->flags |= BOOST_FLASH_WA;
	if (chip->type == SMBBP) {
		struct device_node *revid_dev_node;
		struct pmic_revid_data *revid_data;

		chip->flags |= BOOST_FLASH_WA;

		revid_dev_node = of_parse_phandle(chip->spmi->dev.of_node,
						"qcom,pmic-revid", 0);
		if (!revid_dev_node) {
			pr_err("Missing qcom,pmic-revid property\n");
			return -EINVAL;
		}

		revid_data = get_revid_data(revid_dev_node);
		if (IS_ERR(revid_data)) {
			pr_err("Couldnt get revid data rc = %ld\n",
						PTR_ERR(revid_data));
			return PTR_ERR(revid_data);
		}

		if (revid_data->rev4 < PM8226_V2P1_REV4
			|| ((revid_data->rev4 == PM8226_V2P1_REV4)
				&& (revid_data->rev3 <= PM8226_V2P1_REV3))) {
			chip->flags |= POWER_STAGE_WA;
		}
	}
	return 0;
}

static int
qpnp_chg_request_irqs(struct qpnp_chg_chip *chip)
{
	int rc = 0;
	struct resource *resource;
	struct spmi_resource *spmi_resource;
	u8 subtype;
	struct spmi_device *spmi = chip->spmi;

	spmi_for_each_container_dev(spmi_resource, chip->spmi) {
		if (!spmi_resource) {
				pr_err("qpnp_chg: spmi resource absent\n");
			return rc;
		}

		resource = spmi_get_resource(spmi, spmi_resource,
						IORESOURCE_MEM, 0);
		if (!(resource && resource->start)) {
			pr_err("node %s IO resource absent!\n",
				spmi->dev.of_node->full_name);
			return rc;
		}

		rc = qpnp_chg_read(chip, &subtype,
				resource->start + REG_OFFSET_PERP_SUBTYPE, 1);
		if (rc) {
			pr_err("Peripheral subtype read failed rc=%d\n", rc);
			return rc;
		}

		switch (subtype) {
		case SMBB_CHGR_SUBTYPE:
		case SMBBP_CHGR_SUBTYPE:
		case SMBCL_CHGR_SUBTYPE:
			chip->chg_fastchg.irq = spmi_get_irq_byname(spmi,
						spmi_resource, "fast-chg-on");
			if (chip->chg_fastchg.irq < 0) {
				pr_err("Unable to get fast-chg-on irq\n");
				return rc;
			}

			chip->chg_trklchg.irq = spmi_get_irq_byname(spmi,
						spmi_resource, "trkl-chg-on");
			if (chip->chg_trklchg.irq < 0) {
				pr_err("Unable to get trkl-chg-on irq\n");
				return rc;
			}

			chip->chg_failed.irq = spmi_get_irq_byname(spmi,
						spmi_resource, "chg-failed");
			if (chip->chg_failed.irq < 0) {
				pr_err("Unable to get chg_failed irq\n");
				return rc;
			}

			chip->chg_vbatdet_lo.irq = spmi_get_irq_byname(spmi,
						spmi_resource, "vbat-det-lo");
			if (chip->chg_vbatdet_lo.irq < 0) {
				pr_err("Unable to get fast-chg-on irq\n");
				return rc;
			}

			rc |= devm_request_irq(chip->dev, chip->chg_failed.irq,
				qpnp_chg_chgr_chg_failed_irq_handler,
				IRQF_TRIGGER_RISING, "chg-failed", chip);
			if (rc < 0) {
				pr_err("Can't request %d chg-failed: %d\n",
						chip->chg_failed.irq, rc);
				return rc;
			}

			rc |= devm_request_irq(chip->dev, chip->chg_fastchg.irq,
					qpnp_chg_chgr_chg_fastchg_irq_handler,
					IRQF_TRIGGER_RISING,
					"fast-chg-on", chip);
			if (rc < 0) {
				pr_err("Can't request %d fast-chg-on: %d\n",
						chip->chg_fastchg.irq, rc);
				return rc;
			}

			rc |= devm_request_irq(chip->dev, chip->chg_trklchg.irq,
				qpnp_chg_chgr_chg_trklchg_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"trkl-chg-on", chip);
			if (rc < 0) {
				pr_err("Can't request %d trkl-chg-on: %d\n",
						chip->chg_trklchg.irq, rc);
				return rc;
			}

			rc |= devm_request_irq(chip->dev,
				chip->chg_vbatdet_lo.irq,
				qpnp_chg_vbatdet_lo_irq_handler,
				IRQF_TRIGGER_RISING,
				"vbat-det-lo", chip);
			if (rc < 0) {
				pr_err("Can't request %d vbat-det-lo: %d\n",
						chip->chg_vbatdet_lo.irq, rc);
				return rc;
			}

			enable_irq_wake(chip->chg_trklchg.irq);
			enable_irq_wake(chip->chg_failed.irq);
			qpnp_chg_disable_irq(&chip->chg_vbatdet_lo);
			enable_irq_wake(chip->chg_vbatdet_lo.irq);

			break;
		case SMBB_BAT_IF_SUBTYPE:
		case SMBBP_BAT_IF_SUBTYPE:
		case SMBCL_BAT_IF_SUBTYPE:
			chip->batt_pres.irq = spmi_get_irq_byname(spmi,
						spmi_resource, "batt-pres");
			if (chip->batt_pres.irq < 0) {
				pr_err("Unable to get batt-pres irq\n");
				return rc;
			}
			rc = devm_request_irq(chip->dev, chip->batt_pres.irq,
				qpnp_chg_bat_if_batt_pres_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
				| IRQF_SHARED | IRQF_ONESHOT,
				"batt-pres", chip);
			if (rc < 0) {
				pr_err("Can't request %d batt-pres irq: %d\n",
						chip->batt_pres.irq, rc);
				return rc;
			}

			enable_irq_wake(chip->batt_pres.irq);

			chip->batt_temp_ok.irq = spmi_get_irq_byname(spmi,
						spmi_resource, "bat-temp-ok");
			if (chip->batt_temp_ok.irq < 0) {
				pr_err("Unable to get bat-temp-ok irq\n");
				return rc;
			}
			rc = devm_request_irq(chip->dev, chip->batt_temp_ok.irq,
				qpnp_chg_bat_if_batt_temp_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"bat-temp-ok", chip);
			if (rc < 0) {
				pr_err("Can't request %d bat-temp-ok irq: %d\n",
						chip->batt_temp_ok.irq, rc);
				return rc;
			}

			enable_irq_wake(chip->batt_temp_ok.irq);

			break;
		case SMBB_BUCK_SUBTYPE:
		case SMBBP_BUCK_SUBTYPE:
		case SMBCL_BUCK_SUBTYPE:
			break;

		case SMBB_USB_CHGPTH_SUBTYPE:
		case SMBBP_USB_CHGPTH_SUBTYPE:
		case SMBCL_USB_CHGPTH_SUBTYPE:
			chip->usbin_valid.irq = spmi_get_irq_byname(spmi,
						spmi_resource, "usbin-valid");
			if (chip->usbin_valid.irq < 0) {
				pr_err("Unable to get usbin irq\n");
				return rc;
			}
			rc = devm_request_irq(chip->dev, chip->usbin_valid.irq,
				qpnp_chg_usb_usbin_valid_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
					"usbin-valid", chip);
			if (rc < 0) {
				pr_err("Can't request %d usbin-valid: %d\n",
						chip->usbin_valid.irq, rc);
				return rc;
			}

			chip->chg_gone.irq = spmi_get_irq_byname(spmi,
						spmi_resource, "chg-gone");
			if (chip->chg_gone.irq < 0) {
				pr_err("Unable to get chg-gone irq\n");
				return rc;
			}
			rc = devm_request_irq(chip->dev, chip->chg_gone.irq,
				qpnp_chg_usb_chg_gone_irq_handler,
				IRQF_TRIGGER_RISING,
					"chg-gone", chip);
			if (rc < 0) {
				pr_err("Can't request %d chg-gone: %d\n",
						chip->chg_gone.irq, rc);
				return rc;
			}

			if ((subtype == SMBBP_USB_CHGPTH_SUBTYPE) ||
				(subtype == SMBCL_USB_CHGPTH_SUBTYPE)) {
				chip->usb_ocp.irq = spmi_get_irq_byname(spmi,
						spmi_resource, "usb-ocp");
				if (chip->usb_ocp.irq < 0) {
					pr_err("Unable to get usbin irq\n");
					return rc;
				}
				rc = devm_request_irq(chip->dev,
					chip->usb_ocp.irq,
					qpnp_chg_usb_usb_ocp_irq_handler,
					IRQF_TRIGGER_RISING, "usb-ocp", chip);
				if (rc < 0) {
					pr_err("Can't request %d usb-ocp: %d\n",
							chip->usb_ocp.irq, rc);
					return rc;
				}

				enable_irq_wake(chip->usb_ocp.irq);
			}

			enable_irq_wake(chip->usbin_valid.irq);
			enable_irq_wake(chip->chg_gone.irq);
			break;
		case SMBB_DC_CHGPTH_SUBTYPE:
			chip->dcin_valid.irq = spmi_get_irq_byname(spmi,
					spmi_resource, "dcin-valid");
			if (chip->dcin_valid.irq < 0) {
				pr_err("Unable to get dcin irq\n");
				return -rc;
			}
			rc = devm_request_irq(chip->dev, chip->dcin_valid.irq,
				qpnp_chg_dc_dcin_valid_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"dcin-valid", chip);
			if (rc < 0) {
				pr_err("Can't request %d dcin-valid: %d\n",
						chip->dcin_valid.irq, rc);
				return rc;
			}

			enable_irq_wake(chip->dcin_valid.irq);
			break;
		}
	}

	return rc;
}

static int
qpnp_chg_load_battery_data(struct qpnp_chg_chip *chip)
{
	struct bms_battery_data batt_data;
	struct device_node *node;
	struct qpnp_vadc_result result;
	int rc;

	node = of_find_node_by_name(chip->spmi->dev.of_node,
			"qcom,battery-data");
	if (node) {
		memset(&batt_data, 0, sizeof(struct bms_battery_data));
		rc = qpnp_vadc_read(chip->vadc_dev, LR_MUX2_BAT_ID, &result);
		if (rc) {
			pr_err("error reading batt id channel = %d, rc = %d\n",
						LR_MUX2_BAT_ID, rc);
			return rc;
		}

		batt_data.max_voltage_uv = -1;
		batt_data.iterm_ua = -1;
		/* MS01 EUR is not using. If you need to use, please apply regional feature.
		rc = of_batterydata_read_data(node,
				&batt_data, result.physical);
		if (rc) {
			pr_err("failed to read battery data: %d\n", rc);
			return rc;
		}
		*/
		if (batt_data.max_voltage_uv >= 0) {
			chip->max_voltage_mv = batt_data.max_voltage_uv / 1000;
			chip->safe_voltage_mv = chip->max_voltage_mv
				+ MAX_DELTA_VDD_MAX_MV;
		}
		if (batt_data.iterm_ua >= 0)
			chip->term_current = batt_data.iterm_ua / 1000;
	}

	return 0;
}

#define WDOG_EN_BIT	BIT(7)
static int
qpnp_chg_hwinit(struct qpnp_chg_chip *chip, u8 subtype,
				struct spmi_resource *spmi_resource)
{
	int rc = 0;
	u8 reg = 0;
#ifdef SEC_CHARGER_CODE
	u8 val = 0;
#endif
	struct regulator_init_data *init_data;
	struct regulator_desc *rdesc;

	switch (subtype) {
	case SMBB_CHGR_SUBTYPE:
	case SMBBP_CHGR_SUBTYPE:
	case SMBCL_CHGR_SUBTYPE:
		rc = qpnp_chg_vinmin_set(chip, chip->min_voltage_mv);
		if (rc) {
			pr_debug("failed setting min_voltage rc=%d\n", rc);
			return rc;
		}
		rc = qpnp_chg_vddsafe_set(chip, chip->safe_voltage_mv);
		if (rc) {
			pr_debug("failed setting safe_voltage rc=%d\n", rc);
			return rc;
		}
		rc = qpnp_chg_vbatdet_set(chip,
				chip->max_voltage_mv - chip->resume_delta_mv);
		if (rc) {
			pr_debug("failed setting resume_voltage rc=%d\n", rc);
			return rc;
		}
		rc = qpnp_chg_ibatmax_set(chip, chip->max_bat_chg_current);
		if (rc) {
			pr_debug("failed setting ibatmax rc=%d\n", rc);
			return rc;
		}
		if (chip->term_current) {
			rc = qpnp_chg_ibatterm_set(chip, chip->term_current);
			if (rc) {
				pr_debug("failed setting ibatterm rc=%d\n", rc);
				return rc;
			}
		}
		rc = qpnp_chg_ibatsafe_set(chip, chip->safe_current);
		if (rc) {
			pr_debug("failed setting ibat_Safe rc=%d\n", rc);
			return rc;
		}
		rc = qpnp_chg_tchg_max_set(chip, chip->tchg_mins);
		if (rc) {
			pr_debug("failed setting tchg_mins rc=%d\n", rc);
			return rc;
		}

		/* HACK: Disable wdog */
		rc = qpnp_chg_masked_write(chip, chip->chgr_base + 0x62,
			0xFF, 0xA0, 1);

		/* HACK: use analog EOC */
		rc = qpnp_chg_masked_write(chip, chip->chgr_base +
			CHGR_IBAT_TERM_CHGR,
			0xFF, 0x08, 1);

		break;
	case SMBB_BUCK_SUBTYPE:
	case SMBBP_BUCK_SUBTYPE:
	case SMBCL_BUCK_SUBTYPE:
		rc = qpnp_chg_toggle_chg_done_logic(chip, 0);
		if (rc)
			return rc;
		rc = qpnp_chg_masked_write(chip,
			chip->buck_base + CHGR_BUCK_BCK_VBAT_REG_MODE,
			BUCK_VBAT_REG_NODE_SEL_BIT,
			BUCK_VBAT_REG_NODE_SEL_BIT, 1);
		if (rc) {
			pr_debug("failed to enable IR drop comp rc=%d\n", rc);
			return rc;
		}

		rc = qpnp_chg_read(chip, &chip->trim_center,
				chip->buck_base + BUCK_CTRL_TRIM1, 1);
		if (rc) {
			pr_debug("failed to read trim center rc=%d\n", rc);
			return rc;
		}
		chip->trim_center >>= 4;
		pr_debug("trim center = %02x\n", chip->trim_center);
		break;
	case SMBB_BAT_IF_SUBTYPE:
	case SMBBP_BAT_IF_SUBTYPE:
	case SMBCL_BAT_IF_SUBTYPE:
		/* Select battery presence detection */
		switch (chip->bpd_detection) {
		case BPD_TYPE_BAT_THM:
			reg = BAT_THM_EN;
			break;
		case BPD_TYPE_BAT_ID:
#if defined(CONFIG_USB_SWITCH_RT8973)
			if (rt_check_jig_state() || rt_uart_connecting)
				reg = !(BAT_ID_EN);
			else
#elif defined(CONFIG_SM5502_MUIC)
#if defined(CONFIG_TORCH_FIX)
			if (check_sm5502_jig_state() || uart_sm5502_connecting || factory_uart_connected())
#else
			if (check_sm5502_jig_state() || uart_sm5502_connecting)
#endif
				reg = !(BAT_ID_EN);
			else
#else
			if (check_jig_state() || uart_connecting)
				reg = !(BAT_ID_EN);
			else
#endif
				reg = BAT_ID_EN;
			break;
		case BPD_TYPE_BAT_THM_BAT_ID:
			reg = BAT_THM_EN | BAT_ID_EN;
			break;
		default:
			reg = BAT_THM_EN;
			break;
		}

		rc = qpnp_chg_masked_write(chip,
			chip->bat_if_base + BAT_IF_BPD_CTRL,
			BAT_IF_BPD_CTRL_SEL,
			reg, 1);
		if (rc) {
			pr_debug("failed to chose BPD rc=%d\n", rc);
			return rc;
		}
		/* Force on VREF_BAT_THM */
		rc = qpnp_chg_masked_write(chip,
			chip->bat_if_base + BAT_IF_VREF_BAT_THM_CTRL,
			VREF_BATT_THERM_FORCE_ON,
			VREF_BATT_THERM_FORCE_ON, 1);
		if (rc) {
			pr_debug("failed to force on VREF_BAT_THM rc=%d\n", rc);
			return rc;
		}

		init_data = of_get_regulator_init_data(chip->dev,
					       spmi_resource->of_node);

		if (init_data->constraints.name) {
			rdesc			= &(chip->batfet_vreg.rdesc);
			rdesc->owner		= THIS_MODULE;
			rdesc->type		= REGULATOR_VOLTAGE;
			rdesc->ops		= &qpnp_chg_batfet_vreg_ops;
			rdesc->name		= init_data->constraints.name;

			init_data->constraints.valid_ops_mask
				|= REGULATOR_CHANGE_STATUS;

			chip->batfet_vreg.rdev = regulator_register(rdesc,
					chip->dev, init_data, chip,
					spmi_resource->of_node);
			if (IS_ERR(chip->batfet_vreg.rdev)) {
				rc = PTR_ERR(chip->batfet_vreg.rdev);
				chip->batfet_vreg.rdev = NULL;
				if (rc != -EPROBE_DEFER)
					pr_err("batfet reg failed, rc=%d\n",
							rc);
				return rc;
			}
		}
		break;
	case SMBB_USB_CHGPTH_SUBTYPE:
	case SMBBP_USB_CHGPTH_SUBTYPE:
	case SMBCL_USB_CHGPTH_SUBTYPE:
		if (qpnp_chg_is_usb_chg_plugged_in(chip)) {
			rc = qpnp_chg_masked_write(chip,
				chip->usb_chgpth_base + CHGR_USB_ENUM_T_STOP,
				ENUM_T_STOP_BIT,
				ENUM_T_STOP_BIT, 1);
			if (rc) {
				pr_err("failed to write enum stop rc=%d\n", rc);
				return -ENXIO;
			}
		}

		init_data = of_get_regulator_init_data(chip->dev,
						       spmi_resource->of_node);
		if (!init_data) {
			pr_err("unable to allocate memory\n");
			return -ENOMEM;
		}

		if (init_data->constraints.name) {
			if (of_get_property(chip->dev->of_node,
						"otg-parent-supply", NULL))
				init_data->supply_regulator = "otg-parent";

			rdesc			= &(chip->otg_vreg.rdesc);
			rdesc->owner		= THIS_MODULE;
			rdesc->type		= REGULATOR_VOLTAGE;
			rdesc->ops		= &qpnp_chg_otg_reg_ops;
			rdesc->name		= init_data->constraints.name;

			init_data->constraints.valid_ops_mask
				|= REGULATOR_CHANGE_STATUS;

			chip->otg_vreg.rdev = regulator_register(rdesc,
					chip->dev, init_data, chip,
					spmi_resource->of_node);
			if (IS_ERR(chip->otg_vreg.rdev)) {
				rc = PTR_ERR(chip->otg_vreg.rdev);
				chip->otg_vreg.rdev = NULL;
				if (rc != -EPROBE_DEFER)
					pr_err("OTG reg failed, rc=%d\n", rc);
				return rc;
			}
		}

#ifdef SEC_CHARGER_CODE
		/* Initialze OVP/UVLO settings */
		val = OVP_UVLO_THRESHOLD;
		rc = qpnp_chg_write(chip,&val,
			chip->usb_chgpth_base + USB_OVP_CTL, 1);
		pr_debug("write reg USB_OVP_CTL: val (%d)\n",val);
#ifdef SEC_CHARGER_DEBUG
		rc = qpnp_chg_read(chip, &val,
			chip->usb_chgpth_base + USB_OVP_CTL, 1);
		pr_err("read reg OVP_UVLO_CTL(%x), value (%x)\n",
			USB_OVP_CTL,val);
#endif
#endif
		rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + USB_OVP_CTL,
			USB_VALID_DEB_20MS,
			USB_VALID_DEB_20MS, 1);

		rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + CHGR_USB_ENUM_T_STOP,
			ENUM_T_STOP_BIT,
			ENUM_T_STOP_BIT, 1);

		rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + SEC_ACCESS,
			0xFF,
			0xA5, 1);

		rc = qpnp_chg_masked_write(chip,
			chip->usb_chgpth_base + USB_CHG_GONE_REV_BST,
			0xFF,
			0x80, 1);
		if ((subtype == SMBBP_USB_CHGPTH_SUBTYPE) ||
			(subtype == SMBCL_USB_CHGPTH_SUBTYPE)) {
			rc = qpnp_chg_masked_write(chip,
				chip->usb_chgpth_base + USB_OCP_THR,
				OCP_THR_MASK,
				OCP_THR_900_MA, 1);
			if (rc)
				pr_err("Failed to configure OCP rc = %d\n", rc);
		}

		break;
	case SMBB_DC_CHGPTH_SUBTYPE:
		break;
	case SMBB_BOOST_SUBTYPE:
	case SMBBP_BOOST_SUBTYPE:
		init_data = of_get_regulator_init_data(chip->dev,
					       spmi_resource->of_node);
		if (!init_data) {
			pr_err("unable to allocate memory\n");
			return -ENOMEM;
		}

		if (init_data->constraints.name) {
			if (of_get_property(chip->dev->of_node,
						"boost-parent-supply", NULL))
				init_data->supply_regulator = "boost-parent";

			rdesc			= &(chip->boost_vreg.rdesc);
			rdesc->owner		= THIS_MODULE;
			rdesc->type		= REGULATOR_VOLTAGE;
			rdesc->ops		= &qpnp_chg_boost_reg_ops;
			rdesc->name		= init_data->constraints.name;

			init_data->constraints.valid_ops_mask
				|= REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE;

			chip->boost_vreg.rdev = regulator_register(rdesc,
					chip->dev, init_data, chip,
					spmi_resource->of_node);
			if (IS_ERR(chip->boost_vreg.rdev)) {
				rc = PTR_ERR(chip->boost_vreg.rdev);
				chip->boost_vreg.rdev = NULL;
				if (rc != -EPROBE_DEFER)
					pr_err("boost reg failed, rc=%d\n", rc);
				return rc;
			}
		}
		break;
	case SMBB_MISC_SUBTYPE:
	case SMBBP_MISC_SUBTYPE:
	case SMBCL_MISC_SUBTYPE:
		if (subtype == SMBB_MISC_SUBTYPE)
			chip->type = SMBB;
		else if (subtype == SMBBP_MISC_SUBTYPE)
			chip->type = SMBBP;
		else if (subtype == SMBCL_MISC_SUBTYPE)
			chip->type = SMBCL;
		pr_debug("Setting BOOT_DONE\n");
		rc = qpnp_chg_masked_write(chip,
			chip->misc_base + CHGR_MISC_BOOT_DONE,
			CHGR_BOOT_DONE, CHGR_BOOT_DONE, 1);
		rc = qpnp_chg_read(chip, &reg,
				 chip->misc_base + MISC_REVISION2, 1);
		if (rc) {
			pr_err("failed to read revision register rc=%d\n", rc);
			return rc;
		}

		chip->revision = reg;
		break;
	default:
		pr_err("Invalid peripheral subtype\n");
	}
	return rc;
}

#define OF_PROP_READ(chip, prop, qpnp_dt_property, retval, optional)	\
do {									\
	if (retval)							\
		break;							\
									\
	retval = of_property_read_u32(chip->spmi->dev.of_node,		\
					"qcom," qpnp_dt_property,	\
					&chip->prop);			\
									\
	if ((retval == -EINVAL) && optional)				\
		retval = 0;						\
	else if (retval)						\
		pr_err("Error reading " #qpnp_dt_property		\
				" property rc = %d\n", rc);		\
} while (0)

static int
qpnp_charger_read_dt_props(struct qpnp_chg_chip *chip)
{
	int rc = 0;
	const char *bpd;

	OF_PROP_READ(chip, max_voltage_mv, "vddmax-mv", rc, 0);
	OF_PROP_READ(chip, min_voltage_mv, "vinmin-mv", rc, 0);
	OF_PROP_READ(chip, safe_voltage_mv, "vddsafe-mv", rc, 0);
	OF_PROP_READ(chip, resume_delta_mv, "vbatdet-delta-mv", rc, 0);
	OF_PROP_READ(chip, safe_current, "ibatsafe-ma", rc, 0);
	OF_PROP_READ(chip, max_bat_chg_current, "ibatmax-ma", rc, 0);
	if (rc)
		pr_err("failed to read required dt parameters %d\n", rc);

	OF_PROP_READ(chip, term_current, "ibatterm-ma", rc, 1);
	OF_PROP_READ(chip, maxinput_dc_ma, "maxinput-dc-ma", rc, 1);
	OF_PROP_READ(chip, maxinput_usb_ma, "maxinput-usb-ma", rc, 1);
	OF_PROP_READ(chip, warm_bat_decidegc, "warm-bat-decidegc", rc, 1);
	OF_PROP_READ(chip, cool_bat_decidegc, "cool-bat-decidegc", rc, 1);
	OF_PROP_READ(chip, tchg_mins, "tchg-mins", rc, 1);
	OF_PROP_READ(chip, hot_batt_p, "batt-hot-percentage", rc, 1);
	OF_PROP_READ(chip, cold_batt_p, "batt-cold-percentage", rc, 1);
	OF_PROP_READ(chip, soc_resume_limit, "resume-soc", rc, 1);

	if (rc)
		return rc;

	/* Bug-fix : to enter sleep mode under 6'C or over 60'C */
	chip->warm_bat_decidegc = 1000; 
	chip->cool_bat_decidegc = -500; 

	/* read GPIO address for EXT_OVP line */
	//No Ext Ovp
#ifndef CONFIG_NOT_USE_EXT_OVP
	chip->ovp_gpio = of_get_named_gpio(chip->spmi->dev.of_node,
				"qcom,ovp-fet-gpios", 0);
	if(chip->ovp_gpio == 0) {
		pr_err("failed to read EXT_OVP gpio (%d) \n",chip->ovp_gpio);
	} else {
		pr_err("read EXT_OVP gpio (%d) \n",chip->ovp_gpio);
	}
#endif

	rc = of_property_read_string(chip->spmi->dev.of_node,
		"qcom,bpd-detection", &bpd);
	if (rc) {
		pr_debug(" BAT_THM is the selection for batt detect %d\n", rc);
		/* Select BAT_THM as default BPD scheme */
		chip->bpd_detection = BPD_TYPE_BAT_THM;
		rc = 0;
	} else {
		pr_debug(" BAT_ID is the selection for batt detect %d\n", rc);
		chip->bpd_detection = get_bpd(bpd);
		if (chip->bpd_detection < 0) {
			pr_err("failed to determine bpd schema %d\n", rc);
			return rc;
		}
	}

	/* Look up JEITA compliance parameters if cool and warm temp provided */
	if (chip->cool_bat_decidegc || chip->warm_bat_decidegc) {
		chip->adc_tm_dev = qpnp_get_adc_tm(chip->dev, "chg");
		if (IS_ERR(chip->adc_tm_dev)) {
			rc = PTR_ERR(chip->adc_tm_dev);
			if (rc != -EPROBE_DEFER)
				pr_err("adc-tm not ready, defer probe\n");
				return rc;
		}

		OF_PROP_READ(chip, warm_bat_chg_ma, "ibatmax-warm-ma", rc, 1);
		OF_PROP_READ(chip, cool_bat_chg_ma, "ibatmax-cool-ma", rc, 1);
		OF_PROP_READ(chip, warm_bat_mv, "warm-bat-mv", rc, 1);
		OF_PROP_READ(chip, cool_bat_mv, "cool-bat-mv", rc, 1);
		if (rc)
			return rc;
	}

	/* Get the btc-disabled property */
	chip->btc_disabled = of_property_read_bool(chip->spmi->dev.of_node,
					"qcom,btc-disabled");

	ext_ovp_present = of_property_read_bool(chip->spmi->dev.of_node,
					"qcom,ext-ovp-present");

	/* Get the charging-disabled property */
	chip->charging_disabled = of_property_read_bool(chip->spmi->dev.of_node,
					"qcom,charging-disabled");

	/* Get the duty-cycle-100p property */
	chip->duty_cycle_100p = of_property_read_bool(
					chip->spmi->dev.of_node,
					"qcom,duty-cycle-100p");

	/* Get the fake-batt-values property */
	chip->use_default_batt_values =
			of_property_read_bool(chip->spmi->dev.of_node,
					"qcom,use-default-batt-values");

	/* Disable charging when faking battery values */
	if (chip->use_default_batt_values)
		chip->charging_disabled = true;

	of_get_property(chip->spmi->dev.of_node, "qcom,thermal-mitigation",
		&(chip->thermal_levels));

	if (chip->thermal_levels > sizeof(int)) {
		chip->thermal_mitigation = kzalloc(
			chip->thermal_levels,
			GFP_KERNEL);

		if (chip->thermal_mitigation == NULL) {
			pr_err("thermal mitigation kzalloc() failed.\n");
			return rc;
		}

		chip->thermal_levels /= sizeof(int);
		rc = of_property_read_u32_array(chip->spmi->dev.of_node,
				"qcom,thermal-mitigation",
				chip->thermal_mitigation, chip->thermal_levels);
		if (rc) {
			pr_err("qcom,thermal-mitigation missing in dt\n");
			return rc;
		}
	}

	return rc;
}


/* SAMSUNG charging specification */
#ifdef SEC_CHARGER_CODE

#define SEC_BAT_OF_PROP_READ(chip, prop, qpnp_dt_property, retval, optional)  \
do {                                                                    \
        if (retval)                                                     \
                break;                                                  \
                                                                        \
        retval = of_property_read_u32(chip->spmi->dev.of_node,          \
                                        "sec," qpnp_dt_property,        \
                                        &chip->batt_pdata->prop);       \
                                                                        \
        if ((retval == -EINVAL) && optional)                            \
                retval = 0;                                             \
        else if (retval)                                                \
                pr_err("Error reading " #qpnp_dt_property               \
                                " property rc = %d\n", rc);             \
} while (0)

static int
sec_bat_read_dt_props(struct qpnp_chg_chip *chip)
{
        int rc = 0;
        u32 prop_val = 0 ;

        pr_err("%s:parsing device tree for SEC dt parameters \n",__func__);

        rc = of_property_read_u32(chip->spmi->dev.of_node,
                "sec,charging-time",&prop_val);
        chip->batt_pdata->charging_time = prop_val * 60;
	if (rc)
	pr_err("failed to read SEC charging time dt parameters %d\n", rc);

        rc = of_property_read_u32(chip->spmi->dev.of_node,
                "sec,recharging-time",&prop_val);
        chip->batt_pdata->recharging_time = prop_val * 60;
        if (rc)
                pr_err("failed to read SEC recharging time dt parameters %d\n", rc);

	SEC_BAT_OF_PROP_READ(chip, imax_ta, "imax-ta", rc, 0);
	SEC_BAT_OF_PROP_READ(chip, imax_usb, "imax-usb", rc, 0);
#ifdef SEC_CHARGER_DEBUG
        pr_err("charging-time %ld \n",chip->batt_pdata->charging_time);
        pr_err("recharging-time %ld \n",chip->batt_pdata->recharging_time);
	pr_err("imax-ta %d \n",chip->batt_pdata->imax_ta);
        pr_err("imax-usb %d \n",chip->batt_pdata->imax_usb);
#endif

        SEC_BAT_OF_PROP_READ(chip, temp_high_block_event, "temp-high-block-event", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, temp_high_recover_event, "temp-high-recover-event", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, temp_low_block_event, "temp-low-block-event", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, temp_low_recover_event, "temp-low-recover-event", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, temp_high_block_normal, "temp-high-block-normal", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, temp_high_recover_normal, "temp-high-recover-normal", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, temp_low_block_normal, "temp-low-block-normal", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, temp_low_recover_normal, "temp-low-recover-normal", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, temp_high_block_lpm, "temp-high-block-lpm", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, temp_high_recover_lpm, "temp-high-recover-lpm", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, temp_low_block_lpm, "temp-low-block-lpm", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, temp_low_recover_lpm, "temp-low-recover-lpm", rc, 0);
        if (rc)
                pr_err("failed to read SEC BTM dt parameters %d\n", rc);

#ifdef SEC_CHARGER_DEBUG
        pr_err("EVENT temp-high-block %d\n",chip->batt_pdata->temp_high_block_event);
        pr_err("EVENT temp-high-recover %d\n",chip->batt_pdata->temp_high_recover_event);
        pr_err("EVENT temp-low-block %d\n", chip->batt_pdata->temp_low_block_event);
        pr_err("EVENT temp-low-recover %d\n",chip->batt_pdata->temp_low_recover_event);
        pr_err("NORMAL temp-high-block %d\n",chip->batt_pdata->temp_high_block_normal);
        pr_err("NORMAL temp-high-recover %d\n",chip->batt_pdata->temp_high_recover_normal);
        pr_err("NORMAL temp-low-block %d\n",chip->batt_pdata->temp_low_block_normal);
        pr_err("NORMAL temp-low-recover %d\n",chip->batt_pdata->temp_low_recover_normal);
        pr_err("LPM temp-high-block %d\n",chip->batt_pdata->temp_high_block_lpm);
        pr_err("LPM temp-high-recover %d\n",chip->batt_pdata->temp_high_recover_lpm);
        pr_err("LPM temp-low-block %d\n",chip->batt_pdata->temp_low_block_lpm);
        pr_err("LPM temp-low-recover %d\n",chip->batt_pdata->temp_low_recover_lpm);
#endif

	SEC_BAT_OF_PROP_READ(chip, event_check, "event-check", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, event_waiting_time, "event-waiting-time", rc, 0);
        if (rc)
                pr_err("failed to read SEC event dt parameters %d\n", rc);

#ifdef SEC_CHARGER_DEBUG
        pr_err("event-check %d\n",chip->batt_pdata->event_check);
        pr_err("event-waiting-time %d\n",chip->batt_pdata->event_waiting_time);
#endif
        SEC_BAT_OF_PROP_READ(chip, capacity_max, "capacity-max", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, capacity_max_margin, "capacity-max-margin", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, capacity_min, "capacity-min", rc, 0);
        if (rc)
                pr_err("failed to read SEC fuelgauge dt parameters %d\n", rc);

#ifdef SEC_CHARGER_DEBUG
        pr_err("capacity max %d\n",chip->batt_pdata->capacity_max);
        pr_err("capacity margin %d\n",chip->batt_pdata->capacity_max_margin);
        pr_err("capacity min %d\n",chip->batt_pdata->capacity_min);
#endif
	SEC_BAT_OF_PROP_READ(chip, ui_full_soc, "ui-full-soc", rc, 0);
	SEC_BAT_OF_PROP_READ(chip, ui_full_current, "ui-full-current", rc, 0);
	SEC_BAT_OF_PROP_READ(chip, ui_full_voltage, "ui-full-voltage", rc, 0);
	SEC_BAT_OF_PROP_READ(chip, ui_full_count, "ui-full-count", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, charging_term_time, "charging-term-time", rc, 0);
	chip->batt_pdata->charging_term_time = chip->batt_pdata->charging_term_time * 60;
        SEC_BAT_OF_PROP_READ(chip, recharging_voltage, "recharging-voltage", rc, 0);
        SEC_BAT_OF_PROP_READ(chip, poweroff_check_soc, "poweroff-check-soc", rc, 0);
        if (rc)
                pr_err("failed to read SEC charging dt parameters %d\n", rc);

#ifdef SEC_CHARGER_DEBUG
	pr_err("ui-full-soc %d\n",chip->batt_pdata->ui_full_soc);
	pr_err("ui-full-current %d\n",chip->batt_pdata->ui_full_current);
	pr_err("ui-full-voltage %d\n",chip->batt_pdata->ui_full_voltage);
	pr_err("ui-full-count %d\n",chip->batt_pdata->ui_full_count);
        pr_err("charging-term-time %d\n",chip->batt_pdata->charging_term_time);
        pr_err("recharging-voltage %d\n",chip->batt_pdata->recharging_voltage);
        pr_err("poweroff-check-soc %d\n",chip->batt_pdata->poweroff_check_soc);
#endif

	rc = of_property_read_u32(chip->spmi->dev.of_node,
                "sec,update-time",&prop_val);
	chip->update_time = prop_val;

	if (rc)
        pr_err("failed to read SEC charging dt parameters %d\n", rc);

	rc = of_property_read_u32(chip->spmi->dev.of_node,
                "sec,sleep-update-time",&prop_val);
	chip->sleep_update_time = prop_val;
	chip->capacity_max = chip->batt_pdata->capacity_max;

#ifdef SEC_CHARGER_DEBUG
        pr_err("update-time %d\n",chip->update_time);
        pr_err("sleep-update-time %d\n",chip->sleep_update_time);
#endif

	if (rc)
	pr_err("failed to read SEC charging dt parameters %d\n", rc);

	return rc;

}


static int sec_bat_create_attrs(struct device *dev)
{
        int i, rc;

        for (i = 0; i < ARRAY_SIZE(sec_battery_attrs); i++) {
                rc = device_create_file(dev, &sec_battery_attrs[i]);
                if (rc)
                        goto create_attrs_failed;
        }
        goto create_attrs_succeed;

create_attrs_failed:
        while (i--)
                device_remove_file(dev, &sec_battery_attrs[i]);
create_attrs_succeed:
        return rc;
}

/*
static int sec_fg_create_attrs(struct device *dev)
{
        int i, rc;

        for (i = 0; i < ARRAY_SIZE(sec_fuelgauge_attrs); i++) {
                rc = device_create_file(dev, &sec_fuelgauge_attrs[i]);
                if (rc)
                        goto create_attrs_failed;
        }
        goto create_attrs_succeed;

create_attrs_failed:
        while (i--)
                device_remove_file(dev, &sec_fuelgauge_attrs[i]);
create_attrs_succeed:
        return rc;
}
*/

ssize_t sec_bat_show_attrs(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
        struct power_supply *psy = dev_get_drvdata(dev);
        struct qpnp_chg_chip *chip =
                container_of(psy, struct qpnp_chg_chip, batt_psy);

        int i = 0, val = 0;
        //int batt_capacity;
        const ptrdiff_t offset = attr - sec_battery_attrs;

        switch (offset) {
        case BATT_RESET_SOC:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        bms_reset);
                break;
        case BATT_READ_RAW_SOC:
                //batt_capacity = get_prop_capacity(chip);
                val = chip->capacity_raw;
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        val*100);
                break;
        case BATT_READ_ADJ_SOC:
                val = get_prop_capacity(chip);
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        val);
                break;
        case BATT_TYPE:
                /* work later */
                break;
        case BATT_VFOCV:
                val = get_prop_battery_voltage_now(chip);// / 1000;
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        val);
                break;
        case BATT_VOL_ADC:
                break;
        case BATT_VOL_ADC_CAL:
                break;
        case BATT_VOL_AVER:
                break;
        case BATT_VOL_ADC_AVER:
                break;
        case BATT_TEMP_ADC:
                val = get_prop_batt_temp_adc(chip);
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        val);
                break;
        case BATT_TEMP_AVER:
                break;
        case BATT_TEMP_ADC_AVER:
                break;
        case BATT_VF_ADC:
                break;

        case BATT_LP_CHARGING:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        chip->get_lpm_mode() ? 1 : 0);
                break;
        case SIOP_ACTIVATED:
                break;
        case SIOP_LEVEL:
				i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				chip->siop_level);
                break;
		case BATT_CHARGING_SOURCE:
                switch (chip->cable_type) {
                case CABLE_TYPE_NONE:
                        val = POWER_SUPPLY_TYPE_BATTERY;
                        break;
                case CABLE_TYPE_USB:
                        val = POWER_SUPPLY_TYPE_USB;
                        break;
                case CABLE_TYPE_AC:
                        val = POWER_SUPPLY_TYPE_MAINS;
                        break;
                case CABLE_TYPE_MISC:
                        val = POWER_SUPPLY_TYPE_MISC;
                        break;
                case CABLE_TYPE_UARTOFF:
                        val = POWER_SUPPLY_TYPE_UARTOFF;
                        break;
                case CABLE_TYPE_CDP:
                        val = POWER_SUPPLY_TYPE_USB_CDP;
                        break;
/*
                case CABLE_TYPE_OTG:
                        val = POWER_SUPPLY_TYPE_OTG;
                       break;
*/
                default:
                        val = POWER_SUPPLY_TYPE_UNKNOWN;
                        break;
                }
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        val);
                break;
		case FG_REG_DUMP:
                break;
        case FG_RESET_CAP:
                break;
        case FG_CAPACITY:
                /* this case for current based FG */
                break;
        case AUTH:
                break;
        case CHG_CURRENT_ADC:
                 val = get_prop_current_now(chip);
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        val);
                break;
        case WC_ADC:
                break;
        case WC_STATUS:
                break;

        case FACTORY_MODE:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        chip->factory_mode);
                break;
        case UPDATE:
                break;
        case TEST_MODE:
                break;
        case BATT_EVENT_GSM_CALL:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        (chip->event & EVENT_2G_CALL) ? 1 : 0);
                break;
        case BATT_EVENT_WCDMA_CALL:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        (chip->event & EVENT_3G_CALL) ? 1 : 0);
                break;
        case BATT_EVENT_MUSIC:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        (chip->event & EVENT_MUSIC) ? 1 : 0);
                break;
        case BATT_EVENT_VIDEO:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        (chip->event & EVENT_VIDEO) ? 1 : 0);
                break;
        case BATT_EVENT_BROWSER:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        (chip->event & EVENT_BROWSER) ? 1 : 0);
                break;
        case BATT_EVENT_CAMERA:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        (chip->event & EVENT_CAMERA) ? 1 : 0);
                break;
        case BATT_EVENT_DATA_CALL:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        (chip->event & EVENT_DATA_CALL) ? 1 : 0);
                break;
        case BATT_EVENT_WIFI:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        (chip->event & EVENT_WIFI) ? 1 : 0);
                break;
        case BATT_EVENT_LTE:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        (chip->event & EVENT_LTE) ? 1 : 0);
                break;
        case BATT_EVENT:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        chip->event);
                break;
        case BATT_SLATE_MODE:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               chip->slate_mode);
                break;
        default:
                i = -EINVAL;
        }

        return i;
}

ssize_t sec_bat_store_attrs(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
        struct power_supply *psy = dev_get_drvdata(dev);
        struct qpnp_chg_chip *chip =
                container_of(psy, struct qpnp_chg_chip, batt_psy);

        int ret = 0, x = 0;
        const ptrdiff_t offset = attr - sec_battery_attrs;
		int ibatmax_ma = 0;
		//int charging_current;

		switch (offset) {
        case BATT_RESET_SOC:
                if (sscanf(buf, "%d\n", &x) == 1) {
                        pr_debug("%s : reset BMS module\n", __func__);
                        bms_reset = 1;
                        bms_quickstart();
                }
                ret = count;
                break;
        case BATT_READ_RAW_SOC:
                break;
        case BATT_READ_ADJ_SOC:
                break;
        case BATT_TYPE:
                break;
        case BATT_VFOCV:
                break;
        case BATT_VOL_ADC:
                break;
        case BATT_VOL_ADC_CAL:
                break;
        case BATT_VOL_AVER:
                break;
        case BATT_VOL_ADC_AVER:
                break;
        case BATT_VF_ADC:
                break;
		case BATT_LP_CHARGING:
                break;
        case SIOP_ACTIVATED:
                break;
        case SIOP_LEVEL:
		if (sscanf(buf, "%2d\n", &x) == 1) {
			if (x < 0 || x > 100) {
				pr_err("%s: SIOP level error!\n", __func__);
				ret = count;
				break;
			}
		}

		chip->siop_level = x;
		ibatmax_ma = (chip->siop_level*chg_ibatmax_ma)/100;
		if (chip->siop_level == 100) {
			//set ibatmax current to the max, 2000mA
			pr_err("1-SIOP change in current ibatmax_ma(2000)\n");
			qpnp_chg_ibatmax_set(chip, 2000);
		} else if (ibatmax_ma > QPNP_CHG_IBATMAX_MIN) {
			pr_err("2-SIOP change in current ibatmax_ma(%d)\n", ibatmax_ma);
			qpnp_chg_ibatmax_set(chip, ibatmax_ma);
		} else if (ibatmax_ma >= 0) {
			pr_err("3-SIOP change in current ibatmax_ma(MIN)\n");
			qpnp_chg_ibatmax_set(chip, QPNP_CHG_IBATMAX_MIN);
		} else {
			pr_err("4-SIOP change in current ibatmax_ma(500)\n");
			qpnp_chg_ibatmax_set(chip, 500);
		}
		ret = count;
                break;
        case BATT_CHARGING_SOURCE:
                break;
		case FG_REG_DUMP:
                break;
        case FG_RESET_CAP:
                break;
        case FG_CAPACITY:
                break;
        case AUTH:
                break;
        case CHG_CURRENT_ADC:
                break;
        case WC_ADC:
                break;
        case WC_STATUS:
                break;
        case FACTORY_MODE:
                if (sscanf(buf, "%2d\n", &x) == 1) {
                        chip->factory_mode = x ? true : false;
                        ret = count;
                }
                break;
        case UPDATE:
                ret = count;
                break;
        case TEST_MODE:
                break;
        case BATT_EVENT_GSM_CALL:
                if (sscanf(buf, "%2d\n", &x) == 1) {
                        sec_bat_event_set(chip, BATT_EVENT_GSM_CALL, x);
                        ret = count;
                }
                break;
        case BATT_EVENT_WCDMA_CALL:
                if (sscanf(buf, "%2d\n", &x) == 1) {
                        sec_bat_event_set(chip, BATT_EVENT_WCDMA_CALL, x);
                        ret = count;
                }
                break;
        case BATT_EVENT_MUSIC:
                if (sscanf(buf, "%2d\n", &x) == 1) {
                        sec_bat_event_set(chip, EVENT_MUSIC, x);
                        ret = count;
                }
                break;
        case BATT_EVENT_VIDEO:
                if (sscanf(buf, "%2d\n", &x) == 1) {
                        sec_bat_event_set(chip, EVENT_VIDEO, x);
                        ret = count;
                }
                break;
        case BATT_EVENT_BROWSER:
                if (sscanf(buf, "%2d\n", &x) == 1) {
                        sec_bat_event_set(chip, EVENT_BROWSER, x);
                        ret = count;
                }
                break;
        case BATT_EVENT_HOTSPOT:
                if (sscanf(buf, "%2d\n", &x) == 1) {
                        sec_bat_event_set(chip, EVENT_HOTSPOT, x);
                        ret = count;
                }
                break;

        case BATT_EVENT_CAMERA:
                if (sscanf(buf, "%2d\n", &x) == 1) {
                        sec_bat_event_set(chip, EVENT_CAMERA, x);
                        ret = count;
                }
                break;
        case BATT_EVENT_DATA_CALL:
                if (sscanf(buf, "%2d\n", &x) == 1) {
                        sec_bat_event_set(chip, EVENT_DATA_CALL, x);
                        ret = count;
                }
                break;
        case BATT_EVENT_WIFI:
                if (sscanf(buf, "%2d\n", &x) == 1) {
                        sec_bat_event_set(chip, EVENT_WIFI, x);
                        ret = count;
                }
                break;
        case BATT_EVENT_LTE:
                if (sscanf(buf, "%2d\n", &x) == 1) {
                        sec_bat_event_set(chip, EVENT_LTE, x);
                        ret = count;
                }
                break;

        case BATT_SLATE_MODE:
			pr_info("%s : BATT_SLATE_MODE %s\n", __func__, buf);
			if (sscanf(buf, "%2d\n", &x) == 1) {
				if (x == 1) {
					chip->slate_mode = true;
					chip->cable_type = CABLE_TYPE_NONE;
					pr_debug("batt_slate_mode, charging stop\n");
					sec_handle_cable_insertion_removal(chip);
				} else if (x == 0) {
					chip->slate_mode = false;
					chip->cable_type = CABLE_TYPE_UARTOFF;
					pr_debug("batt_slate_mode, charging startn");
					sec_handle_cable_insertion_removal(chip);
				} else
					pr_info("%s:Invalid\n", __func__);
				ret = count;
			}
			break;
        default:
                ret = -EINVAL;
        }

        return ret;
}

/*
ssize_t sec_fg_show_attrs(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
        struct power_supply *psy = dev_get_drvdata(dev);
        struct pm8921_chg_chip *chip =
                container_of(psy, struct pm8921_chg_chip, fg_psy);

        int i = 0, val = 0;
        const ptrdiff_t offset = attr - sec_fuelgauge_attrs;

        switch (offset) {
        case FG_CURR_UA:
                val = get_prop_batt_current(chip);
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                        val);
                break;
        default:
                i = -EINVAL;
        }

        return i;
}
*/

/*

ssize_t sec_fg_store_attrs(
                                        struct device *dev,
                                        struct device_attribute *attr,
                                        const char *buf, size_t count)
{
       */ /*struct power_supply *psy = dev_get_drvdata(dev);
        struct pm8921_chg_chip *chip =
                container_of(psy, struct pm8921_chg_chip, fg_psy);
*/ /*
        int ret = 0;
        const ptrdiff_t offset = attr - sec_fuelgauge_attrs;

        switch (offset) {
        case FG_CURR_UA:
                break;
        default:
                ret = -EINVAL;
        }

        return ret;

*/


static void sec_bat_event_program_alarm(
        struct qpnp_chg_chip *chip, int seconds)
{
        ktime_t low_interval = ktime_set(seconds - 10, 0);
        ktime_t slack = ktime_set(20, 0);
        ktime_t next;

        next = ktime_add(chip->last_event_time, low_interval);
        alarm_start_range(&chip->event_termination_alarm,
                next, ktime_add(next, slack));
}

static void sec_bat_event_expired_timer_func(struct alarm *alarm)
{
        struct qpnp_chg_chip *chip =
                container_of(alarm, struct qpnp_chg_chip,
                        event_termination_alarm);

	if(chip->event == 0) {
		dev_dbg(chip->dev,
                                "%s: nothing to clear\n", __func__);
	} else {
		chip->event &= (~chip->event_wait);
		dev_info(chip->dev,
                                "%s: event expired (0x%x)\n", __func__, chip->event);
	}

	/* reset BTM EVENT temperature settings if event expired */
	if (!chip->event) {
                chip->temp_high_block = chip->batt_pdata->temp_high_block_normal;
                chip->temp_high_recover = chip->batt_pdata->temp_high_recover_normal;
                chip->temp_low_block = chip->batt_pdata->temp_low_block_normal;
                chip->temp_low_recover = chip->batt_pdata->temp_low_recover_normal;
		pr_err("%s: all events expired chip->event(0x%x)\n", __func__, chip->event);
		pr_err("SEC BTM: set normal temperature limits high_block(%d) high_recover(%d)\n",
			chip->temp_high_block,chip->temp_high_recover);
		pr_err("SEC BTM: set normal temperature limits low_block(%d) low_recover(%d)\n",
			chip->temp_low_block,chip->temp_low_recover);
        }
}


static void sec_bat_event_set(
        struct qpnp_chg_chip *chip, int event, int enable)
{

	if (!chip->batt_pdata->event_check)
		return;

        /* ignore duplicated deactivation of same event
         * only if the event is one last event
         */
        if (!enable && (chip->event == chip->event_wait)) {
                dev_info(chip->dev,
                        "%s: ignore duplicated deactivation of same event\n",
                        __func__);
                return;
        }

        alarm_cancel(&chip->event_termination_alarm);
        chip->event &= (~chip->event_wait);

        if (enable) {
                chip->event_wait = 0;
                chip->event |= event;

		pr_debug("%s: event set (0x%x)\n", __func__, chip->event);
        } else {
                if (chip->event == 0) {
			pr_debug("%s: nothing to clear\n", __func__);
                        return; /* nothing to clear */
                }
		chip->event = chip->event & (~event);
		pr_debug("%s: clear event(%d) chip->event(%d)\n",
			__func__ , event, chip->event);

                chip->event_wait = event;
                chip->last_event_time = alarm_get_elapsed_realtime();

                sec_bat_event_program_alarm(chip,
                        chip->batt_pdata->event_waiting_time);
                pr_debug("%s: start timer (curr 0x%x, wait 0x%x)\n",
                        __func__, chip->event, chip->event_wait);
        }

	/* set EVENT temperature limits for BTM */
	if (chip->event) {
		chip->temp_high_block = chip->batt_pdata->temp_high_block_event;
		chip->temp_high_recover = chip->batt_pdata->temp_high_recover_event;
		chip->temp_low_block = chip->batt_pdata->temp_low_block_event;
		chip->temp_low_recover = chip->batt_pdata->temp_low_recover_event;
		pr_debug("SEC BTM: event active (%x) set event temperature high_block(%d) high_recover(%d)\n",
			chip->event,chip->temp_high_block,chip->temp_high_recover);
		pr_debug("SEC BTM: event active (%x) set event temperature low_block(%d) low_recover(%d)\n",
			chip->event,chip->temp_low_block,chip->temp_low_recover);
	}
}




static bool sec_chg_time_management(struct qpnp_chg_chip *chip)
{
	unsigned long charging_time;
	ktime_t current_time;
	struct timespec ts;
	int batt_capacity = 0;

	current_time = alarm_get_elapsed_realtime();
	ts = ktime_to_timespec(current_time);

	/* device discharging */
	if (chip->charging_start_time == 0) {
		pr_err("discharging state! timer inactive\n");

		return true;
	}

	/* update charging time when device charging */
	if (ts.tv_sec >= chip->charging_start_time) {
		charging_time = ts.tv_sec - chip->charging_start_time;
#ifdef SEC_CHARGER_DEBUG
		pr_err("charging_start_time(%ld) charging_time(%ld) \n",
			chip->charging_start_time,charging_time);
#endif
	} else {
		charging_time = 0xFFFFFFFF - chip->charging_start_time + ts.tv_sec;
#ifdef SEC_CHARGER_DEBUG
		pr_err("charging_start_time(%ld) charging_time(%ld) \n",
			chip->charging_start_time,charging_time);
#endif
	}

	chip->charging_passed_time = charging_time;
	pr_err("Charging Time: %ld secs (rechg:%d)",
			chip->charging_passed_time, chip->is_recharging);


	/* Check for timer expiry when charging */
	if ((chip->batt_status == POWER_SUPPLY_STATUS_CHARGING) ||
			(chip->batt_status == POWER_SUPPLY_STATUS_FULL)) {
		if (chip->ui_full_chg) {
			if (charging_time >= chip->batt_pdata->charging_term_time) {
				pr_err("Back charing complete! Charging Time: %ld secs \n",
						chip->charging_passed_time);
				sec_pm8226_stop_charging(chip);
			}

		} else if (chip->is_recharging &&
					(charging_time >= chip->batt_pdata->recharging_time)) {
			pr_err("Recharging Timer expired! Charging Time: %ld secs \n",
				chip->charging_passed_time);

			batt_capacity = get_prop_capacity(chip);
			if (batt_capacity == 100)
				chip->batt_status = POWER_SUPPLY_STATUS_FULL;
			sec_pm8226_stop_charging(chip);

			return false;

		} else if (!chip->is_recharging &&
					(charging_time >= chip->batt_pdata->charging_time)) {
			pr_err("Charging Timer expired! Charging Time: %ld secs \n",
				chip->charging_passed_time);

			batt_capacity = get_prop_capacity(chip);
			if (batt_capacity == 100)
				chip->batt_status = POWER_SUPPLY_STATUS_FULL;
			sec_pm8226_stop_charging(chip);

			return false;
		}
	}

	return true;
}


static void sec_handle_cable_insertion_removal(struct qpnp_chg_chip *chip)
{
	int prev_batt_status;
	union power_supply_propval value;
	union power_supply_propval imax_set = {0,};
	int ibatmax_ma = 0;
	u8 chgpth_sts = 0;

	#ifndef CONFIG_NOT_USE_EXT_OVP
	u8 chgr_sts;
	int rc;
	#endif

	#ifdef SEC_CHARGER_DEBUG
	pr_err("batt_present = %d \n", chip->batt_present);
	#endif

	prev_batt_status = chip->batt_status;

	//chip->health_cnt = 0;
	chip->ui_full_cnt = 0;
	chip->recharging_cnt = 0;

	chip->charging_start_time = 0;
	chip->charging_passed_time = 0;
	chip->is_recharging = false;
	chip->is_chgtime_expired = false;
	/* initialize health in cable event */
	chip->batt_health = POWER_SUPPLY_HEALTH_GOOD;
	if (!chip->usb_psy)
		chip->usb_psy = power_supply_get_by_name("usb");

	/* set charge current */
	/* to be don eby reading appropriate struct*/
	/* temporary work around*/
	chg_imax_ma = 0;

	switch (chip->cable_type) {
	case CABLE_TYPE_NONE:
	case CABLE_TYPE_CARDOCK:
		pr_err("cable not connected: cable_type(%d)",chip->cable_type);
		/* reset status variables when cable not connected or removed */
		chip->ui_full_cnt = 0;
		chip->ui_full_chg = 0;
		sec_pm8226_stop_charging(chip);
		chip->batt_health = chip->batt_present ?
			POWER_SUPPLY_HEALTH_GOOD : POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		chip->batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
		value.intval = 0;
		qpnp_chg_iusbmax_set(chip, 0);
		chip->usb_psy->set_property(chip->usb_psy, POWER_SUPPLY_PROP_ONLINE, &value);
		break;
	case CABLE_TYPE_INCOMPATIBLE:
		value.intval = 0;
		chip->usb_psy->set_property(chip->usb_psy, POWER_SUPPLY_PROP_ONLINE, &value);
		pr_err("INCOMPATIBLE charger inserted, batt_present(%d)\n",
		chip->batt_present);

		qpnp_chg_iusbmax_set(chip, chg_imax_ma);
		qpnp_chg_ibatmax_set(chip,chg_imax_ma);
		sec_pm8226_stop_charging(chip);
		chip->batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case CABLE_TYPE_USB:
		pr_err("USB is inserted, chg_current 500, batt_present(%d)\n",
				chip->batt_present);
		/* set USB ONLINE when USB is connected */
		value.intval = 1;
		chip->usb_psy->set_property(chip->usb_psy, POWER_SUPPLY_PROP_ONLINE, &value);
		if (chip->batt_present) {
			if (chip->ovp_uvlo_state == 0) {
				/* FIX for USB current not set properly */
				chg_imax_ma = chip->batt_pdata->imax_usb;
				qpnp_chg_usb_suspend_enable(chip, 0);
				qpnp_chg_iusbmax_set(chip, chg_imax_ma);
				chg_ibatmax_ma = chip->batt_pdata->imax_usb;
				ibatmax_ma = (chip->siop_level*chg_ibatmax_ma) / 100;
				if (ibatmax_ma > QPNP_CHG_IBATMAX_MIN ) {
					pr_err("USB : SIOP change in current ibatmax_ma(%d)\n", chg_ibatmax_ma);
					qpnp_chg_ibatmax_set(chip, chg_ibatmax_ma);
				} else {
					pr_err("USB : SIOP change in current ibatmax_ma(0), Set to 50mA\n");
					qpnp_chg_ibatmax_set(chip, QPNP_CHG_IBATMAX_MIN);
				}
				sec_pm8226_start_charging(chip);
			}
		}
		break;

	case CABLE_TYPE_AC:
			pr_err("TA is inserted, batt_present(%d)\n"
					,chip->batt_present);
	case CABLE_TYPE_CDP:
	case CABLE_TYPE_MISC:
	case CABLE_TYPE_UARTOFF:
	case CABLE_TYPE_AUDIO_DOCK:
		if (chip->batt_present) {
			/* can't start charging because VBUS isn't present */
			qpnp_chg_read(chip, &chgpth_sts,
							chip->usb_chgpth_base + 0x10 , 1);
			pr_err("chgpth_sts: 0x%x\n", chgpth_sts);
			if ((chgpth_sts & 0x2) == 0x0) {
				pr_err("cable type (%d), but VBUS is absent\n",chip->cable_type);
				sec_pm8226_stop_charging(chip);
				chip->cable_type = CABLE_TYPE_NONE;
				chip->batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
				break;
			}

			if (chip->ovp_uvlo_state == 0) {
				if (chip->cable_type == CABLE_TYPE_CDP) {
					value.intval = 1;
					chip->usb_psy->set_property(chip->usb_psy, POWER_SUPPLY_PROP_ONLINE, &value);
				}
				chg_imax_ma = chip->batt_pdata->imax_ta;
				qpnp_chg_usb_suspend_enable(chip, 0);

				#ifndef CONFIG_NOT_USE_EXT_OVP
				rc = qpnp_chg_read(chip, &chgr_sts, INT_RT_STS(chip->chgr_base), 1);
				if (chgr_sts & FAST_CHG_ON_IRQ) {
					pr_err("EXT OVP is on\n");
					gpio_set_value(chip->ovp_gpio, 1);

					//back to Ext Ovp
					pr_err("Enable sec_bat_ext_ovp_work\n");
					schedule_delayed_work(&chip->sec_bat_ext_ovp_work, msecs_to_jiffies(15000));
				}
				#endif

				qpnp_chg_iusbmax_set(chip, chg_imax_ma);

#ifndef CONFIG_NOT_USE_EXT_OVP
				qpnp_chg_iusb_trim_set(chip, 63);
#else
				#if defined(CONFIG_SEC_KANAS_PROJECT)
				qpnp_chg_iusb_trim_set(chip, 48);
				#else
				qpnp_chg_iusb_trim_set(chip, 40);
				#endif
#endif
				pr_err("USB Trim : %d\n", qpnp_chg_iusb_trim_get(chip));

				imax_set.intval = chip->batt_pdata->imax_ta * 1000;
				chip->usb_psy->set_property(chip->usb_psy, POWER_SUPPLY_PROP_CURRENT_MAX, &imax_set);

				imax_set.intval = 0;
				chip->usb_psy->get_property(chip->usb_psy, POWER_SUPPLY_PROP_CURRENT_MAX, &imax_set);
				pr_err("iusbmax : %d \n", imax_set.intval);

				chg_ibatmax_ma = chip->batt_pdata->imax_ta;
				ibatmax_ma = (chip->siop_level*chg_ibatmax_ma)/100;
				if (chip->siop_level == 100) {
					//set ibatmax current to the max, 2000mA
					pr_err("TA : SIOP change in current ibatmax_ma(MAX:2000)\n");
					qpnp_chg_ibatmax_set(chip, 2000);
				} else if (ibatmax_ma > QPNP_CHG_IBATMAX_MIN) {
					pr_err("TA : SIOP change in current ibatmax_ma(%d)\n", ibatmax_ma);
					qpnp_chg_ibatmax_set(chip, ibatmax_ma);
				} else if (ibatmax_ma >= 0) {
					pr_err("TA : SIOP change in current ibatmax_ma(MIN)\n");
					qpnp_chg_ibatmax_set(chip, QPNP_CHG_IBATMAX_MIN);
				} else {
					pr_err("TA : SIOP change in current ibatmax_ma(500)\n");
					qpnp_chg_ibatmax_set(chip, 500);
				}

				mdelay(200);
				sec_pm8226_start_charging(chip);
			}
		}
		break;
	default:
		break;
	}

	/* check battery present  state */
#if defined(CONFIG_USB_SWITCH_RT8973)
	if (chip->cable_type != CABLE_TYPE_NONE && !chip->batt_present && !rt_check_jig_state()) {
#elif defined(CONFIG_SM5502_MUIC)
	if (chip->cable_type != CABLE_TYPE_NONE && !chip->batt_present && !check_sm5502_jig_state()) {
#else
	if (chip->cable_type != CABLE_TYPE_NONE && !chip->batt_present && !check_jig_state()) {
#endif
		sec_pm8226_stop_charging(chip);
		chip->batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		chip->batt_health = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
		qpnp_chg_disable_irq(&chip->chg_vbatdet_lo);
		qpnp_chg_force_run_on_batt(chip, 1);
		qpnp_chg_iusbmax_set(chip, 0);
		qpnp_chg_usb_suspend_enable(chip, 1);
	}

	if (prev_batt_status != chip->batt_status) {
		pr_err("battery status changed from (%d)->(%d)\n",
			prev_batt_status,chip->batt_status);
	}

	// alarm_cancel(&chip->event_termination_alarm);
	schedule_delayed_work(&chip->sec_bat_monitor_work, 0);
	wake_lock_timeout(&chip->cable_wake_lock, 3*HZ);

	//power_supply_changed(chip->usb_psy);
	//power_supply_changed(&chip->batt_psy);
}


static void sec_pm8226_stop_charging(struct qpnp_chg_chip *chip)
{
	pr_err("disable charging \n");

	chip->charging_disabled = 1;
	qpnp_chg_charge_en(chip,0);
	//qpnp_chg_force_run_on_batt(chip,1);
	cancel_delayed_work(&chip->eoc_work);

	if(chip->batt_status == POWER_SUPPLY_STATUS_FULL) {
		pr_err("BATTERY FULL stop charging ! \n");
	} else if (chip->batt_health == POWER_SUPPLY_HEALTH_GOOD) {
                chip->batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
		pr_err("battery status changed to (%d)\n",chip->batt_status);
	} else {
                chip->batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		pr_err("battery status changed to (%d)\n",chip->batt_status);
	}

	chip->health_cnt = 0;
	chip->recharging_cnt = 0;
	chip->ui_full_chg = 0;
	chip->charging_start_time = 0;
	chip->charging_passed_time = 0;
	chip->is_recharging = false;

	//pr_err("vbatdet lo irq enabled \n");
	/* we don't need it */
	//qpnp_chg_enable_irq(&chip->chg_vbatdet_lo);

	#ifndef CONFIG_NOT_USE_EXT_OVP
	/* disable EXT_OVP when charging is disabled */
	gpio_set_value(chip->ovp_gpio, 0);
	#endif

	//power_supply_changed(chip->usb_psy);
	//power_supply_changed(&chip->batt_psy);

}


static void sec_pm8226_start_charging(struct qpnp_chg_chip *chip)
{
        ktime_t current_time;
        struct timespec ts;

        current_time = alarm_get_elapsed_realtime();
        ts = ktime_to_timespec(current_time);

	if(chip->ovp_uvlo_state != 0) {
		chip->batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		pr_err("voltage check failed: ovp_uvlo(%d), cannot start charging! \n",
			chip->ovp_uvlo_state);
		return;
	}

        switch (chip->cable_type) {
        case CABLE_TYPE_USB:
        case CABLE_TYPE_AC:
        case CABLE_TYPE_CDP:
        case CABLE_TYPE_MISC:
        case CABLE_TYPE_UARTOFF:
        case CABLE_TYPE_AUDIO_DOCK:
		/* Set charge current based on cable type */
		//usb_target_ma = 500; temporraliy done in cable_insertion func
		/* set CHG_EN bit to enable charging*/
		pr_err("enable charging for cable type (%d) \n",chip->cable_type);

		chip->charging_disabled = 0;

		if (chip->batt_status == POWER_SUPPLY_STATUS_FULL) {
			pr_err("battery full do not update UI for recharging phase \n");
		} else {
			chip->batt_status = POWER_SUPPLY_STATUS_CHARGING;
			pr_err("battery status changed to (%d)\n",chip->batt_status);
		}

		#ifdef SEC_CHARGER_DEBUG
		pr_err("vbatdet lo irq disabled \n");
		#endif

		qpnp_chg_disable_irq(&chip->chg_vbatdet_lo);
		qpnp_chg_charge_en(chip,1);
		qpnp_chg_force_run_on_batt(chip,0);

		/* update charging start time */
		if (chip->charging_start_time == 0)
			chip->charging_start_time = ts.tv_sec;
		break;

	case CABLE_TYPE_NONE:
		pr_err("cannot enable charging, cable type (NONE)\n");
		break;

	default:
                break;
	}


	//pm8921_charger_vbus_draw(usb_target_ma);

        //pm8921_set_max_battery_charge_current(
        //	chip->batt_pdata->chg_current_table[
        //        chip->cable_type].ibat);


	/* update charging start time */

        //if (chip->charging_start_time == 0)
        //        chip->charging_start_time = ts.tv_sec;

        //chip->charging_enabled = 1;
        chip->health_cnt = 0; /* second check, plz */
        chip->recharging_cnt = 0;

	//power_supply_changed(chip->usb_psy);
        //power_supply_changed(&chip->batt_psy);

}

static void sec_bat_monitor(struct work_struct *work)
{

	struct delayed_work *dwork = to_delayed_work(work);
	struct qpnp_chg_chip *chip = container_of(dwork,
                                struct qpnp_chg_chip, sec_bat_monitor_work);

	int batt_capacity;
	//int batt_status;
	//int batt_health;
	int batt_present;
	unsigned int batt_temp;
	unsigned int batt_voltage;
	long int current_now;
	//int charge_type;
#ifdef SEC_BTM_TEST
	static u8 btm_count;
#endif
	int rc;
	u8 buck_sts = 0;

	if (chip->is_in_sleep)
		chip->is_in_sleep = false;

	/* do not sleep until monitor function completes */
	wake_lock(&chip->monitor_wake_lock);

	/* Charging time management */
	sec_chg_time_management(chip);


	batt_voltage = get_prop_battery_voltage_now(chip) / 1000;
	batt_capacity = get_prop_capacity(chip);
	chip->recent_reported_soc = batt_capacity;
	//batt_status = get_prop_batt_status(chip);
	//batt_health = get_prop_batt_health(chip);
	batt_present = get_prop_batt_present(chip);
	batt_temp = get_prop_batt_temp(chip);
	current_now = get_prop_current_now(chip) / 1000;
	//charge_type = get_prop_charge_type(chip);

	/* check for recharging by battery voltage */
	if ((chip->cable_type == CABLE_TYPE_USB ||
		chip->cable_type == CABLE_TYPE_AC ||
		chip->cable_type == CABLE_TYPE_CDP ||
		chip->cable_type == CABLE_TYPE_MISC ||
		chip->cable_type == CABLE_TYPE_UARTOFF ||
		chip->cable_type == CABLE_TYPE_AUDIO_DOCK) &&
		chip->charging_disabled &&
		!chip->is_recharging &&
		(chip->batt_health == POWER_SUPPLY_HEALTH_GOOD)) {

			batt_voltage = get_prop_battery_voltage_now(chip) / 1000;

			if (batt_voltage <
				(chip->batt_pdata->recharging_voltage))
					chip->recharging_cnt++;
			else
				chip->recharging_cnt = 0;

			if (chip->recharging_cnt > 3) {
				sec_pm8226_start_charging(chip);
				chip->is_recharging = true;
				//pr_info("%s: Battery re-charging state by battery voltage:
				//		recharging count (%d) \n", __func__, chip->recharging_cnt);
			}

	}





#ifdef SEC_BTM_TEST
	pr_err("**** running SEC BTM test **** \n");
	switch(btm_count) {
	case 2: sec_btm_temp = 700;
		pr_err(" normal high-cutoff test set temperature(%d) \n",sec_btm_temp);
		break;
	case 4: sec_btm_temp = 350;
		pr_err(" normal high-recovery test set temperature(%d) \n",sec_btm_temp);
		break;
	case 6: sec_btm_temp = -80;
		pr_err(" normal low-cutoff test set temperature(%d) \n",sec_btm_temp);
		break;
	case 8: sec_btm_temp = 100;
		pr_err(" normal low-recovery test set temperature(%d) \n",sec_btm_temp);
		break;
	case 10: sec_bat_event_set(chip, EVENT_MUSIC, 1);
		pr_err(" set event temperature limits test event(%d) \n",chip->event);
		break;
	case 12: sec_btm_temp = 750;
		pr_err(" event high-cutoff test set temperature(%d) \n",sec_btm_temp);
		break;
	case 14:sec_btm_temp = 550;
		pr_err(" event high-recovery test set temperature(%d) \n",sec_btm_temp);
		break;
	case 16: sec_bat_event_set(chip, EVENT_MUSIC, 0);
		pr_err(" event clear test event(%d) \n",chip->event);
		break;
	case 18: sec_btm_temp = 350;
		pr_err(" normal high-recovery test set temperature(%d) \n",sec_btm_temp);
		break;
	default:
		break;
	}

	btm_count++;
	batt_temp = sec_btm_temp;
	if(btm_count > 18)
		btm_count = 0 ;

#endif

	/* SEC battery temperature monitoring */
	sec_bat_temperature_monitor(chip);

	if (chip->batt_status == POWER_SUPPLY_STATUS_CHARGING || chip->is_recharging) {
		if ( qpnp_chg_is_usb_chg_plugged_in(chip) && !chip->charging_disabled ) {
			rc = qpnp_chg_read(chip, &buck_sts, INT_RT_STS(chip->buck_base), 1);
			if (!rc) {
				if (buck_sts & VDD_LOOP_IRQ) {
					qpnp_chg_adjust_vddmax(chip, batt_voltage);
				}
			} else {
				pr_err("failed to read buck rc=%d\n", rc);
			}
			if(chip->ui_full_chg) { /* second phase charging */
				pr_err("second phase charging: ui_full_chg(%d) \n",chip->ui_full_chg);

			} else { /* first phase charging */

				if (((current_now * -1) < chip->batt_pdata->ui_full_current) &&
					(batt_voltage >= chip->batt_pdata->ui_full_voltage) &&
					(chip->recent_reported_soc >= chip->batt_pdata->ui_full_soc)) {

					chip->ui_full_cnt++;
					pr_err("first phase charging: ui_full_cnt(%d)",
						chip->ui_full_cnt);
				} else {
					/* reset counter if not consecutive */
					chip->ui_full_cnt = 0 ;
				}

				if (chip->ui_full_cnt > chip->batt_pdata->ui_full_count) {
					chip->capacity_old++;
					chip->recent_reported_soc = chip->capacity_old;
					if (chip->recent_reported_soc > 100)
						chip->recent_reported_soc = 100;
					pr_err("reported_soc : (%d)", chip->recent_reported_soc);

					if (chip->recent_reported_soc == 100) {
						ktime_t current_time;
						struct timespec ts;
						current_time = alarm_get_elapsed_realtime();
						ts = ktime_to_timespec(current_time);
						pr_err("first phase charging done: update battery UI FULL \n");
						chip->batt_status = POWER_SUPPLY_STATUS_FULL;
						chip->ui_full_chg = 1;
						chip->ui_full_cnt = 0;

						pr_err("start second phase charging: ui_full(%d) \n",
							chip->ui_full_chg);
						chip->charging_start_time = ts.tv_sec;
						chip->charging_passed_time = 0;
					}
				}
			}
		}
	}

	/* set polling time */
	if (chip->is_chgtime_expired) {
		pr_err("is_chgtime_expired = %d \n",chip->is_chgtime_expired);
		chip->polling_time = 20;
	} else {
		chip->polling_time = chip->update_time;
	}
	chip->capacity_old = chip->recent_reported_soc;

	pr_err("status(%d), health(%d), batt_present(%d), cable(%d), siop(%d)\n",
			chip->batt_status, chip->batt_health, batt_present,
			chip->cable_type, chip->siop_level);

	pr_err("soc(%d), vcell(%u), current(%ld), temperature(%d), chg_en(%d)\n",
			batt_capacity, batt_voltage, current_now, batt_temp, !(chip->charging_disabled));

	pr_err("ovp_uvlo(%d), second_phase_chg(%d), polling_time(%d), event(0x%x), HT(%d), LT(%d)\n",
			chip->ovp_uvlo_state, chip->ui_full_chg, chip->polling_time,
			chip->event,chip->temp_high_block,chip->temp_low_block);

	//power_supply_changed(chip->usb_psy);
	power_supply_changed(&chip->batt_psy);

	/* program alarm for monitoring */
	sec_bat_program_alarm(chip, chip->polling_time);

	/* can go to sleep after monitor completes */
	wake_unlock(&chip->monitor_wake_lock);

}

static void sec_ovp_uvlo_worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct qpnp_chg_chip *chip = container_of(dwork,
                                struct qpnp_chg_chip,ovp_uvlo_work);

	/* USB-IN Under voltage triggered on cable removal
	 * so ignore trigger if cable not present
	 */
	if(chip->cable_type != CABLE_TYPE_NONE) {
		pr_err("OVP/UVLO check: cable_type(%d) uvp_uvlo(%d)\n",
				chip->cable_type, chip->ovp_uvlo_state);

		if (chip->ovp_uvlo_state == 0) { /* OVP/UVLO recover */
			if ((chip->batt_health == POWER_SUPPLY_HEALTH_OVERVOLTAGE ||
				chip->batt_health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE) &&
					chip->batt_status == POWER_SUPPLY_STATUS_NOT_CHARGING) {
				pr_err("OVP/UVLO recover: enable charging\n");
				sec_pm8226_start_charging(chip);
				chip->batt_health = POWER_SUPPLY_HEALTH_GOOD;
				chip->batt_status = POWER_SUPPLY_STATUS_CHARGING;
			} else {
				pr_err("ovp_uvlo(%d) VBUS(%d) status(%d) health(%d)\n",
						chip->ovp_uvlo_state, chip->usb_present,
						chip->batt_status, chip->batt_health);
			}
		} else { /* OVP/UVLO state */
			if (chip->batt_health == POWER_SUPPLY_HEALTH_GOOD &&
					chip->batt_status == POWER_SUPPLY_STATUS_CHARGING) {
				pr_err("OVP/UVLO state: disable charging\n");
				chip->batt_health = (chip->ovp_uvlo_state == 1) ?
					POWER_SUPPLY_HEALTH_OVERVOLTAGE : POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
				sec_pm8226_stop_charging(chip);
				chip->batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
			} else {
				pr_err("ovp_uvlo(%d) VBUS(%d) status(%d) health(%d)\n",
						chip->ovp_uvlo_state, chip->usb_present,
						chip->batt_status, chip->batt_health);
			}
		}
		wake_lock_timeout(&chip->cable_wake_lock, 3*HZ);
	}

	//power_supply_changed(chip->usb_psy);
	power_supply_changed(&chip->batt_psy);
}

#if 0 //move to qpnp-bms.c
/* capacity is 0.1% unit */
static int sec_fg_get_scaled_capacity(
                        struct qpnp_chg_chip *chip, int raw_soc)
{
        int scaled_soc;

        scaled_soc = (raw_soc < chip->batt_pdata->capacity_min) ?
                0 : ((raw_soc - chip->batt_pdata->capacity_min) * 1000 /
                (chip->capacity_max - chip->batt_pdata->capacity_min));

        return scaled_soc;
}

/* capacity is 0.1% unit */
static int sec_fg_calculate_dynamic_scale(
                        struct qpnp_chg_chip *chip, int raw_soc)
{
		raw_soc += 10;
        if (raw_soc <
                chip->batt_pdata->capacity_max -
                chip->batt_pdata->capacity_max_margin)
                chip->capacity_max =
                        chip->batt_pdata->capacity_max -
                        chip->batt_pdata->capacity_max_margin;
        else
                chip->capacity_max =
                        (raw_soc > 1000) ? 1000 : raw_soc;

        chip->capacity_max =
                ((chip->capacity_max - chip->batt_pdata->capacity_min)
                * 991 / 1000) + chip->batt_pdata->capacity_min;

        /* updated old capacity as full (integer) */
        chip->capacity_old = 100;

        pr_debug("%s: %d is used for capacity_max\n",
                __func__, chip->capacity_max);

        return chip->capacity_max;
}
#endif

static int sec_ac_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct qpnp_chg_chip *chip = container_of(psy, struct qpnp_chg_chip,ac_psy);

	if ((chip->batt_health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) ||
			(chip->batt_health == POWER_SUPPLY_HEALTH_UNDERVOLTAGE))
		val->intval = 0;
	else if (chip->cable_type == CABLE_TYPE_AC ||
			chip->cable_type == CABLE_TYPE_MISC ||
			chip->cable_type == CABLE_TYPE_UARTOFF ||
			chip->cable_type == CABLE_TYPE_AUDIO_DOCK) {
		val->intval = 1;
	} else {
		val->intval = 0;
	}

#ifdef SEC_CHARGER_DEBUG
	pr_err("cable type= %d,return val= %d\n",chip->cable_type, val->intval);
#endif
	return 0;
}
static void sec_bat_temperature_monitor(struct qpnp_chg_chip *chip)
{
	int temp;

#ifdef SEC_BTM_TEST
	temp = sec_btm_temp;
#else
	temp = get_prop_batt_temp(chip);
#endif

#ifdef SEC_CHARGER_DEBUG
	pr_err("SEC BTM temperature(%d.%d) \n",temp/10,temp%10);
#endif

	if (chip->batt_health == POWER_SUPPLY_HEALTH_GOOD) {
		if (temp >= chip->temp_high_block) {
			/* Normal to overheat */
			if (chip->usb_present &&
				chip->batt_status == POWER_SUPPLY_STATUS_CHARGING) {
				pr_err("SEC BTM :stop charging @ temperature(%d) \n",temp);
				msleep(1000);
				sec_pm8226_stop_charging(chip);
				chip->batt_health = POWER_SUPPLY_HEALTH_OVERHEAT;
				chip->batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
			}
		}


		if (temp <= chip->temp_low_block) {
			/* Normal to cold */
			if(chip->usb_present &&
				chip->batt_status == POWER_SUPPLY_STATUS_CHARGING) {
				pr_err("SEC BTM :stop charging @ temperature(%d) \n",temp);
				msleep(1000);
				sec_pm8226_stop_charging(chip);
				chip->batt_health = POWER_SUPPLY_HEALTH_COLD;
				chip->batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
			}
		}
	} else {
		if ((chip->batt_health == POWER_SUPPLY_HEALTH_COLD) &&
			(temp >= chip->temp_low_recover)) {
			/* Cold to normal */
			if ((chip->usb_present) &&
				(chip->batt_status == POWER_SUPPLY_STATUS_NOT_CHARGING)) {
				pr_err("SEC BTM :resume charging @ temperature(%d) \n",temp);
				sec_pm8226_start_charging(chip);
				chip->batt_health = POWER_SUPPLY_HEALTH_GOOD;
				chip->batt_status = POWER_SUPPLY_STATUS_CHARGING;
			}
		}

		if ((chip->batt_health == POWER_SUPPLY_HEALTH_OVERHEAT)
			&& (temp <= chip->temp_high_recover)) {
			/* overheat to normal */
			if ((chip->usb_present) &&
				(chip->batt_status == POWER_SUPPLY_STATUS_NOT_CHARGING)) {
				pr_err("SEC BTM :resume charging @ temperature(%d) \n",temp);
				sec_pm8226_start_charging(chip);
				chip->batt_health = POWER_SUPPLY_HEALTH_GOOD;
				chip->batt_status = POWER_SUPPLY_STATUS_CHARGING;
			}
		}
	}

}
static void sec_bat_program_alarm(struct qpnp_chg_chip *chip, int polling_time)
{
	ktime_t low_interval = ktime_set(polling_time, 0);
	ktime_t slack = ktime_set(10, 0);
	ktime_t next;

	chip->last_update_time = alarm_get_elapsed_realtime();

	next = ktime_add(chip->last_update_time, low_interval);
	alarm_start_range(&chip->polling_alarm,
		next, ktime_add(next, slack));
}


static void sec_bat_polling_alarm_expired(struct alarm *alarm)
{
	struct qpnp_chg_chip *chip = container_of(alarm,
		struct qpnp_chg_chip, polling_alarm);

	schedule_delayed_work(&chip->sec_bat_monitor_work, 0);

}

#endif
#if defined(CONFIG_MACH_MS01_LTE)
static struct qpnp_chg_chip    * chg_chip;
void change_boost_control(int on)
{
	u8 val_bat_reg;
	if (!chg_chip)
		return;
	if(on) {
		val_bat_reg = 0x0F;
		qpnp_chg_write(chg_chip, &val_bat_reg, 0x1573, 1);
		val_bat_reg = 0x03;
		qpnp_chg_write(chg_chip, &val_bat_reg, 0x1576, 1);
	} else {
		val_bat_reg = 0x03;
		qpnp_chg_write(chg_chip, &val_bat_reg, 0x1573, 1);
		val_bat_reg = 0x01;
		qpnp_chg_write(chg_chip, &val_bat_reg, 0x1576, 1);
	}

}
#endif
static int __devinit
qpnp_charger_probe(struct spmi_device *spmi)
{
	u8 subtype;
	struct qpnp_chg_chip	*chip;
	struct resource *resource;
	struct spmi_resource *spmi_resource;
	int rc = 0;

	// TOBE CHANGED: This is workaround, QC will provide a patch.
        u8 val_bat_reg = 0;

	chip = kzalloc(sizeof *chip, GFP_KERNEL);
	if (chip == NULL) {
		pr_err("kzalloc() failed.\n");
		return -ENOMEM;
	}
#if defined(CONFIG_MACH_MS01_LTE)
	chg_chip =  chip;
#endif
#ifdef SEC_CHARGER_CODE
	chip->batt_pdata = kzalloc(sizeof(struct sec_battery_data), GFP_KERNEL);
        if (chip->batt_pdata == NULL) {
                pr_err("kzalloc() failed for SEC battery platform data .\n");
		kfree(chip);
                return -ENOMEM;
        }
#endif

	chip->prev_usb_max_ma = -EINVAL;
	chip->dev = &(spmi->dev);
	chip->spmi = spmi;

	chip->usb_psy = power_supply_get_by_name("usb");
	if (!chip->usb_psy) {
		pr_err("usb supply not found deferring probe\n");
		rc = -EPROBE_DEFER;
		goto fail_chg_enable_2;
	}

	mutex_init(&chip->jeita_configure_lock);
	alarm_init(&chip->reduce_power_stage_alarm, ANDROID_ALARM_RTC_WAKEUP,
			qpnp_chg_reduce_power_stage_callback);
	INIT_WORK(&chip->reduce_power_stage_work,
			qpnp_chg_reduce_power_stage_work);
	mutex_init(&chip->batfet_vreg_lock);
	INIT_WORK(&chip->batfet_lcl_work,
			qpnp_chg_batfet_lcl_work);
	/* Get all device tree properties */
	rc = qpnp_charger_read_dt_props(chip);
	if (rc)
		goto fail_chg_enable;

	// TOBE CHANGED: This is workaround, QC will provide a patch.
	val_bat_reg = 0;
	qpnp_chg_write(chip, &val_bat_reg, 0x124A, 1);
	val_bat_reg = 0x0D;
	qpnp_chg_write(chip, &val_bat_reg, 0x1248, 1);
	val_bat_reg = 0xA5;
	qpnp_chg_write(chip, &val_bat_reg, 0x12D0, 1);
	val_bat_reg = 0x28;
	qpnp_chg_write(chip, &val_bat_reg, 0x12E5, 1);

	val_bat_reg = 0x0F;
	qpnp_chg_write(chip, &val_bat_reg, 0x1573, 1);
	val_bat_reg = 0x03;
	qpnp_chg_write(chip, &val_bat_reg, 0x1576, 1);

	/* Check if bat_if is set in DT and make sure VADC is present */
	spmi_for_each_container_dev(spmi_resource, spmi) {
		if (!spmi_resource) {
			pr_err("qpnp_chg: spmi resource absent\n");
			rc = -ENXIO;
			goto fail_chg_enable;
		}

		resource = spmi_get_resource(spmi, spmi_resource,
						IORESOURCE_MEM, 0);
		if (!(resource && resource->start)) {
			pr_err("node %s IO resource absent!\n",
				spmi->dev.of_node->full_name);
			rc = -ENXIO;
			goto fail_chg_enable;
		}

		rc = qpnp_chg_read(chip, &subtype,
				resource->start + REG_OFFSET_PERP_SUBTYPE, 1);
		if (rc) {
			pr_err("Peripheral subtype read failed rc=%d\n", rc);
			goto fail_chg_enable;
		}

		if (subtype == SMBB_BAT_IF_SUBTYPE ||
			subtype == SMBBP_BAT_IF_SUBTYPE ||
			subtype == SMBCL_BAT_IF_SUBTYPE) {
			chip->vadc_dev = qpnp_get_vadc(chip->dev, "chg");
			if (IS_ERR(chip->vadc_dev)) {
				rc = PTR_ERR(chip->vadc_dev);
				if (rc != -EPROBE_DEFER)
					pr_err("vadc property missing\n");
					goto fail_chg_enable;
			}
			rc = qpnp_chg_load_battery_data(chip);
			if (rc)
				goto fail_chg_enable;
		}
	}

	spmi_for_each_container_dev(spmi_resource, spmi) {
		if (!spmi_resource) {
			pr_err("qpnp_chg: spmi resource absent\n");
			rc = -ENXIO;
			goto fail_chg_enable;
		}

		resource = spmi_get_resource(spmi, spmi_resource,
						IORESOURCE_MEM, 0);
		if (!(resource && resource->start)) {
			pr_err("node %s IO resource absent!\n",
				spmi->dev.of_node->full_name);
			rc = -ENXIO;
			goto fail_chg_enable;
		}

		rc = qpnp_chg_read(chip, &subtype,
				resource->start + REG_OFFSET_PERP_SUBTYPE, 1);
		if (rc) {
			pr_err("Peripheral subtype read failed rc=%d\n", rc);
			goto fail_chg_enable;
		}

		switch (subtype) {
		case SMBB_CHGR_SUBTYPE:
		case SMBBP_CHGR_SUBTYPE:
		case SMBCL_CHGR_SUBTYPE:
			chip->chgr_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to init subtype 0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		case SMBB_BUCK_SUBTYPE:
		case SMBBP_BUCK_SUBTYPE:
		case SMBCL_BUCK_SUBTYPE:
			chip->buck_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to init subtype 0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}

			rc = qpnp_chg_masked_write(chip,
				chip->buck_base + SEC_ACCESS,
				0xFF,
				0xA5, 1);

			rc = qpnp_chg_masked_write(chip,
				chip->buck_base + BUCK_VCHG_OV,
				0xff,
				0x00, 1);

				if (chip->duty_cycle_100p) {
				rc = qpnp_buck_set_100_duty_cycle_enable(chip,
						1);
				if (rc) {
					pr_err("failed to set duty cycle %d\n",
						rc);
					goto fail_chg_enable;
				}
			}

			break;
		case SMBB_BAT_IF_SUBTYPE:
		case SMBBP_BAT_IF_SUBTYPE:
		case SMBCL_BAT_IF_SUBTYPE:
			chip->bat_if_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to init subtype 0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		case SMBB_USB_CHGPTH_SUBTYPE:
		case SMBBP_USB_CHGPTH_SUBTYPE:
		case SMBCL_USB_CHGPTH_SUBTYPE:
			chip->usb_chgpth_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				if (rc != -EPROBE_DEFER)
					pr_err("Failed to init subtype 0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		case SMBB_DC_CHGPTH_SUBTYPE:
			chip->dc_chgpth_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to init subtype 0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		case SMBB_BOOST_SUBTYPE:
		case SMBBP_BOOST_SUBTYPE:
			chip->boost_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				if (rc != -EPROBE_DEFER)
					pr_err("Failed to init subtype 0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		case SMBB_MISC_SUBTYPE:
		case SMBBP_MISC_SUBTYPE:
		case SMBCL_MISC_SUBTYPE:
			chip->misc_base = resource->start;
			rc = qpnp_chg_hwinit(chip, subtype, spmi_resource);
			if (rc) {
				pr_err("Failed to init subtype=0x%x rc=%d\n",
						subtype, rc);
				goto fail_chg_enable;
			}
			break;
		default:
			pr_err("Invalid peripheral subtype=0x%x\n", subtype);
			rc = -EINVAL;
			goto fail_chg_enable;
		}
	}
	dev_set_drvdata(&spmi->dev, chip);
	device_init_wakeup(&spmi->dev, 1);

	if (chip->bat_if_base) {
#ifdef SEC_CHARGER_CODE
		chip->ac_psy.name = "ac",
		chip->ac_psy.type = POWER_SUPPLY_TYPE_MAINS,
		chip->ac_psy.supplied_to = pm_power_supplied_to,
		chip->ac_psy.num_supplicants = ARRAY_SIZE(pm_power_supplied_to),
		chip->ac_psy.properties = sec_power_props,
		chip->ac_psy.num_properties = ARRAY_SIZE(sec_power_props),
		chip->ac_psy.get_property = sec_ac_get_property;
#endif

		chip->batt_psy.name = "battery";
		chip->batt_psy.type = POWER_SUPPLY_TYPE_BATTERY;
		chip->batt_psy.properties = msm_batt_power_props;
		chip->batt_psy.num_properties =
			ARRAY_SIZE(msm_batt_power_props);
		chip->batt_psy.get_property = qpnp_batt_power_get_property;
		chip->batt_psy.set_property = qpnp_batt_power_set_property;
		chip->batt_psy.property_is_writeable =
				qpnp_batt_property_is_writeable;
		chip->batt_psy.external_power_changed =
				qpnp_batt_external_power_changed;
		chip->batt_psy.supplied_to = pm_batt_supplied_to;
		chip->batt_psy.num_supplicants =
				ARRAY_SIZE(pm_batt_supplied_to);

		rc = power_supply_register(chip->dev, &chip->batt_psy);
		if (rc < 0) {
			pr_err("batt failed to register rc = %d\n", rc);
			goto fail_chg_enable;
		}
#ifdef SEC_CHARGER_CODE
		rc = power_supply_register(chip->dev, &chip->ac_psy);
		if (rc < 0) {
			pr_err("ac failed to register rc = %d\n", rc);
			goto fail_chg_enable;
		}
#endif
		INIT_WORK(&chip->adc_measure_work,
			qpnp_bat_if_adc_measure_work);
		INIT_WORK(&chip->adc_disable_work,
			qpnp_bat_if_adc_disable_work);
	}

	INIT_DELAYED_WORK(&chip->eoc_work, qpnp_eoc_work);
	INIT_DELAYED_WORK(&chip->arb_stop_work, qpnp_arb_stop_work);
	INIT_WORK(&chip->soc_check_work, qpnp_chg_soc_check_work);
	INIT_DELAYED_WORK(&chip->aicl_check_work, qpnp_aicl_check_work);

	if (chip->dc_chgpth_base) {
		chip->dc_psy.name = "qpnp-dc";
		chip->dc_psy.type = POWER_SUPPLY_TYPE_MAINS;
		chip->dc_psy.supplied_to = pm_power_supplied_to;
		chip->dc_psy.num_supplicants = ARRAY_SIZE(pm_power_supplied_to);
		chip->dc_psy.properties = pm_power_props_mains;
		chip->dc_psy.num_properties = ARRAY_SIZE(pm_power_props_mains);
		chip->dc_psy.get_property = qpnp_power_get_property_mains;
		chip->dc_psy.set_property = qpnp_dc_power_set_property;
		chip->dc_psy.property_is_writeable =
				qpnp_dc_property_is_writeable;

		rc = power_supply_register(chip->dev, &chip->dc_psy);
		if (rc < 0) {
			pr_err("power_supply_register dc failed rc=%d\n", rc);
			goto unregister_batt;
		}
	}

	/* Turn on appropriate workaround flags */
	rc = qpnp_chg_setup_flags(chip);
	if (rc < 0) {
		pr_err("failed to setup flags rc=%d\n", rc);
		goto unregister_dc_psy;
	}

	if (chip->maxinput_dc_ma && chip->dc_chgpth_base) {
		rc = qpnp_chg_idcmax_set(chip, chip->maxinput_dc_ma);
		if (rc) {
			pr_err("Error setting idcmax property %d\n", rc);
			goto unregister_dc_psy;
		}
	}

	if ((chip->cool_bat_decidegc || chip->warm_bat_decidegc)
							&& chip->bat_if_base) {
		chip->adc_param.low_temp = chip->cool_bat_decidegc;
		chip->adc_param.high_temp = chip->warm_bat_decidegc;
		chip->adc_param.timer_interval = ADC_MEAS2_INTERVAL_1S;
		chip->adc_param.state_request = ADC_TM_HIGH_LOW_THR_ENABLE;
		chip->adc_param.btm_ctx = chip;
		chip->adc_param.threshold_notification =
						qpnp_chg_adc_notification;
		chip->adc_param.channel = LR_MUX1_BATT_THERM;

		if (get_prop_batt_present(chip)) {
			rc = qpnp_adc_tm_channel_measure(chip->adc_tm_dev,&chip->adc_param);
			if (rc) {
				pr_err("request ADC error %d\n", rc);
				goto unregister_dc_psy;
			}
		}
	}

	rc = qpnp_chg_bat_if_configure_btc(chip);
	if (rc) {
		pr_err("failed to configure btc %d\n", rc);
		goto unregister_dc_psy;
	}

	#ifndef CONFIG_NOT_USE_EXT_OVP
	/* configure GPIO for EXT _OVP */
	rc = gpio_request(chip->ovp_gpio, "QPNP_CHG_OVP");
	if (rc) {
		pr_err("failed to request EXT_OVP gpio %d\n", rc);
		}

	rc = gpio_direction_output(chip->ovp_gpio, 0);
	if (rc) {
		pr_err("failed to set direction gpio out for EXT_OVP %d \n", rc);
		}

	/* Disable EXT_OVP by default */
	gpio_set_value(chip->ovp_gpio, 0);
	#endif

	/* SAMSUNG charging specification */
#ifdef SEC_CHARGER_CODE
	/* read device tree for SEC bat data */
        rc = sec_bat_read_dt_props(chip);
        if (rc) {
		pr_err("SEC battery device tree read FAILED : rc(%d) \n", rc);
                goto fail_chg_enable;
	}
	pr_err("SEC battery device tree read SUCCESS : rc(%d) \n", rc);

	/* LPM mode support */
	chip->get_cable_type = sec_bat_get_cable_status;
	chip->get_lpm_mode = sec_bat_is_lpm;

	/* initialize status variable state */
	chip->ovp_uvlo_state = 0;
	chip->ui_full_chg = 0;
	chip->ui_full_cnt = 0;
	chip->siop_level = 100;
	chip->batt_present = qpnp_chg_is_batt_present(chip);
#ifdef CONFIG_MACH_KANAS3G_CTC
    /*CF open power on state soc use calculate_soc_from_voltage to calculate soc,
      voltage change fast, need short the update time to avoid soc jump*/
	if(!chip->batt_present)
		chip->update_time = 30;
#endif

	if(!chip->batt_pdata->charging_time)
		chip->charging_term_time = (6 * 60 * 60);
	else
		chip->charging_term_time = chip->batt_pdata->charging_time;

	INIT_DELAYED_WORK(&chip->ovp_uvlo_work,sec_ovp_uvlo_worker);

	/* initialize battery status and health */
    chip->batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
	chip->batt_health = chip->batt_present ?
		POWER_SUPPLY_HEALTH_GOOD : POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
        pr_err("%s: initialize battery status(%d) & health(%d)\n",
		__func__,chip->batt_status,chip->batt_health);
	chip->cable_exception = CABLE_TYPE_NONE;
#endif

	//If the device is booted up with a cable is connected, then we leave its charging state as it is.
	if (!qpnp_chg_is_usb_chg_plugged_in(chip)) {
		qpnp_chg_charge_en(chip, 0);
		qpnp_chg_force_run_on_batt(chip, 1);
	}
	qpnp_chg_set_appropriate_vddmax(chip);

#ifndef SEC_CHARGER_CODE
	/* Set USB psy online to avoid userspace from shutting down if battery
         * capacity is at zero and no chargers online. */
        if (qpnp_chg_is_usb_chg_plugged_in(chip))
                power_supply_set_online(chip->usb_psy, 1);
#endif

	/* SAMSUNG charging specification */
#ifdef SEC_CHARGER_CODE
	sec_bat_create_attrs(chip->batt_psy.dev);
	//sec_fg_create_attrs(chip->fg_psy.dev);

	alarm_init(&chip->event_termination_alarm,
			ANDROID_ALARM_ELAPSED_REALTIME,
			sec_bat_event_expired_timer_func);
	alarm_init(&chip->polling_alarm,
		ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP,
		sec_bat_polling_alarm_expired);

	wake_lock_init(&chip->monitor_wake_lock, WAKE_LOCK_SUSPEND,
                       "sec-charger-monitor");
	wake_lock_init(&chip->cable_wake_lock, WAKE_LOCK_SUSPEND,
                       "sec-cable-check");
	INIT_DELAYED_WORK(&chip->sec_bat_monitor_work,sec_bat_monitor);
	//back to Ext OVP
#ifndef CONFIG_NOT_USE_EXT_OVP
	INIT_DELAYED_WORK(&chip->sec_bat_ext_ovp_work, sec_bat_ext_ovp_confirm);
#endif
	/* check cable type while booting */
	chip->cable_type = chip->get_cable_type();

	/* Check if boot mode is LPM */
	if(chip->get_lpm_mode()) {
		pr_err("LPM charging mode with cable_type(%d)\n",chip->cable_type);

		/* battery temperature monitoring LPM */
		chip->temp_high_block = chip->batt_pdata->temp_high_block_lpm;
		chip->temp_high_recover = chip->batt_pdata->temp_high_recover_lpm;
		chip->temp_low_block = chip->batt_pdata->temp_low_block_lpm;
		chip->temp_low_recover = chip->batt_pdata->temp_low_recover_lpm;
	} else {
		pr_err("normal boot with cable_type(%d)\n",chip->cable_type);

		/* battery temperature monitoring NORMAL */
		chip->temp_high_block = chip->batt_pdata->temp_high_block_normal;
		chip->temp_high_recover = chip->batt_pdata->temp_high_recover_normal;
		chip->temp_low_block = chip->batt_pdata->temp_low_block_normal;
		chip->temp_low_recover = chip->batt_pdata->temp_low_recover_normal;
	}

	chip->is_in_sleep = false;
	chip->polling_time = chip->update_time;

	schedule_delayed_work(&chip->sec_bat_monitor_work, 2*HZ);

#endif

	schedule_delayed_work(&chip->aicl_check_work,
		msecs_to_jiffies(EOC_CHECK_PERIOD_MS));
	pr_info("success chg_dis = %d, bpd = %d, usb = %d, dc = %d b_health = %d batt_present = %d\n",
			chip->charging_disabled,
			chip->bpd_detection,
			qpnp_chg_is_usb_chg_plugged_in(chip),
			qpnp_chg_is_dc_chg_plugged_in(chip),
			get_prop_batt_health(chip),
			get_prop_batt_present(chip));

	rc = qpnp_chg_request_irqs(chip);
	if (rc) {
		pr_err("failed to request interrupts %d\n", rc);
		goto unregister_dc_psy;
	}

	qpnp_chg_usb_usbin_valid_irq_handler(chip->usbin_valid.irq, chip);
	qpnp_chg_dc_dcin_valid_irq_handler(chip->dcin_valid.irq, chip);
	#ifndef SEC_CHARGER_CODE
	power_supply_set_present(chip->usb_psy,
			qpnp_chg_is_usb_chg_plugged_in(chip));
	#endif
	rc = qpnp_chg_regulator_batfet_set(chip, 1);
	if (rc)
		pr_err("failed to write to batt_if rc=%d\n", rc);

	return 0;

unregister_dc_psy:
	if (chip->dc_chgpth_base)
		power_supply_unregister(&chip->dc_psy);
unregister_batt:
	if (chip->bat_if_base)
		power_supply_unregister(&chip->batt_psy);
fail_chg_enable:
	regulator_unregister(chip->otg_vreg.rdev);
	regulator_unregister(chip->boost_vreg.rdev);
	kfree(chip->thermal_mitigation);
fail_chg_enable_2:
#ifdef SEC_CHARGER_CODE
	kfree(chip->batt_pdata);
#endif
	kfree(chip);
	dev_set_drvdata(&spmi->dev, NULL);
	return rc;
}

static int __devexit
qpnp_charger_remove(struct spmi_device *spmi)
{
	struct qpnp_chg_chip *chip = dev_get_drvdata(&spmi->dev);
	if ((chip->cool_bat_decidegc || chip->warm_bat_decidegc)
						&& chip->batt_present) {
		qpnp_adc_tm_disable_chan_meas(chip->adc_tm_dev,&chip->adc_param);
	}
	cancel_work_sync(&chip->adc_measure_work);
	cancel_delayed_work_sync(&chip->eoc_work);
	cancel_work_sync(&chip->batfet_lcl_work);

	mutex_destroy(&chip->batfet_vreg_lock);
	regulator_unregister(chip->otg_vreg.rdev);
	regulator_unregister(chip->boost_vreg.rdev);

	dev_set_drvdata(&spmi->dev, NULL);
#ifdef SEC_CHARGER_CODE
	/* de-allocate batt_pdata before chip structure */
	kfree(chip->batt_pdata);
#endif
	kfree(chip);

	return 0;
}

#ifdef SEC_CHARGER_CODE
static int sec_qpnp_chg_prepare(struct device *dev)
{
	struct qpnp_chg_chip *chip = dev_get_drvdata(dev);
#ifdef SEC_CHARGER_DEBUG
        pr_err("%s start\n", __func__);
#endif
        chip->is_in_sleep = true;

	if ((chip->recent_reported_soc <= chip->batt_pdata->poweroff_check_soc) ||
			((chip->ui_full_cnt > 0) &&
			(chip->batt_status != POWER_SUPPLY_STATUS_FULL))) {
			pr_err("charger driver prepare for suspend!\n");
		chip->polling_time = chip->update_time;
	} else {
		if(chip->usb_present) {
			chip->polling_time = chip->update_time;
		} else {
			chip->polling_time = chip->sleep_update_time;
		}
	}

	cancel_delayed_work(&chip->sec_bat_monitor_work);
	alarm_cancel(&chip->polling_alarm);

	chip->last_update_time = alarm_get_elapsed_realtime();
	sec_bat_program_alarm(chip, chip->polling_time);
#ifdef SEC_CHARGER_DEBUG
	pr_err("%s battery update time (%d seconds) !!\n",
		 __func__,chip->polling_time);
        pr_err("%s end\n", __func__);
#endif
        return 0;
}


static void sec_qpnp_chg_complete(struct device *dev)
{
	struct qpnp_chg_chip *chip = dev_get_drvdata(dev);
#ifdef SEC_CHARGER_DEBUG
        pr_err("%s start\n", __func__);
#endif
	cancel_delayed_work(&chip->sec_bat_monitor_work);
	alarm_cancel(&chip->polling_alarm);

	chip->polling_time = chip->update_time;
	schedule_delayed_work(&chip->sec_bat_monitor_work, 0);

#ifdef SEC_CHARGER_DEBUG
	pr_err("%s battery update time (%d seconds) !!\n",
		__func__,chip->polling_time);
#endif

#ifdef SEC_CHARGER_DEBUG
        pr_err("%s end\n", __func__);
#endif
        return;
}
#endif

static int qpnp_chg_resume(struct device *dev)
{
	struct qpnp_chg_chip *chip = dev_get_drvdata(dev);
	int rc = 0;

	if (chip->bat_if_base) {
		rc = qpnp_chg_masked_write(chip,
			chip->bat_if_base + BAT_IF_VREF_BAT_THM_CTRL,
			VREF_BATT_THERM_FORCE_ON,
			VREF_BATT_THERM_FORCE_ON, 1);
		if (rc)
			pr_debug("failed to force on VREF_BAT_THM rc=%d\n", rc);
	}

	return rc;
}

static int qpnp_chg_suspend(struct device *dev)
{
#if 0
	struct qpnp_chg_chip *chip = dev_get_drvdata(dev);
	int rc = 0;

	if (chip->bat_if_base) {
		rc = qpnp_chg_masked_write(chip,
			chip->bat_if_base + BAT_IF_VREF_BAT_THM_CTRL,
			VREF_BATT_THERM_FORCE_ON,
			VREF_BAT_THM_ENABLED_FSM, 1);
		if (rc)
			pr_debug("failed to set FSM VREF_BAT_THM rc=%d\n", rc);
	}

#ifdef SEC_CHARGER_CODE
        pr_err(" Charger driver suspend !\n");
#endif
	return rc;
#else
	return 0;
#endif
}

static const struct dev_pm_ops qpnp_chg_pm_ops = {
#ifdef SEC_CHARGER_CODE
	.prepare	= sec_qpnp_chg_prepare,
	.complete       = sec_qpnp_chg_complete,
#endif
	.suspend	= qpnp_chg_suspend,
	.resume         = qpnp_chg_resume,
};

static struct spmi_driver qpnp_charger_driver = {
	.probe		= qpnp_charger_probe,
	.remove		= __devexit_p(qpnp_charger_remove),
	.driver		= {
		.name	= QPNP_CHARGER_DEV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = qpnp_charger_match_table,
		.pm		= &qpnp_chg_pm_ops,
	},
};

/**
 * qpnp_chg_init() - register spmi driver for qpnp-chg
 */
int __init
qpnp_chg_init(void)
{
	return spmi_driver_register(&qpnp_charger_driver);
}
module_init(qpnp_chg_init);

static void __exit
qpnp_chg_exit(void)
{
	spmi_driver_unregister(&qpnp_charger_driver);
}
module_exit(qpnp_chg_exit);


MODULE_DESCRIPTION("QPNP charger driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" QPNP_CHARGER_DEV_NAME);
