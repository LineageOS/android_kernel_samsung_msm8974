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

#ifndef __QPNP_SEC_CHARGER_H
#define __QPNP_SEC_CHARGER_H

#include <linux/errno.h>
#include <linux/power_supply.h>

/* Enable Samsung Charging specification code */
#define SEC_CHARGER_CODE

#define NUM_SIOP_LEVEL		5


//extern bool ovp_state;

/*
 *  Charging current table
 */

struct pm8226_charging_current {
        unsigned int		ibat;
};


/*
 * SAMSUNG battery platform data
 */

struct sec_battery_data {
        /* for absolute timer (first) */
        unsigned long			charging_time;
        /* for recharging timer (second) */
        unsigned long			recharging_time;
	/* charging current set */
        int                             imax_ta;		/* mA */
        int                             imax_usb;	/* mA */
        /* temperature set */
        int                             temp_high_block_event;
        int                             temp_high_recover_event;
        int                             temp_low_block_event;
        int                             temp_low_recover_event;
        int                             temp_high_block_normal;
        int                             temp_high_recover_normal;
        int                             temp_low_block_normal;
        int                             temp_low_recover_normal;
        int                             temp_high_block_lpm;
        int                             temp_high_recover_lpm;
        int                             temp_low_block_lpm;
        int                             temp_low_recover_lpm;
#if defined(CONFIG_WIRELESS_CHARGING)
        int                             temp_high_block_wc;
        int                             temp_high_recover_wc;
        int                             temp_low_block_wc;
        int                             temp_low_recover_wc;
#endif
        /* event check model flag */
        unsigned int			event_check;
        /* sustaining event after deactivated (second) */
        unsigned int			event_waiting_time;

        /* soc should be soc x 10 (0.1% degree)
         * only for scaling
         */
        unsigned int                    capacity_max;
        unsigned int                    capacity_max_margin;
        unsigned int                    capacity_min;
	
	unsigned int			ui_full_soc;
        unsigned int                    ui_full_current;	/* mA */
	unsigned int			ui_full_voltage;	/* mV */
	unsigned int                    ui_full_count;
        unsigned int                    charging_term_time;	/* sec */
        unsigned int                    recharging_voltage;	/* mV */
        unsigned int                    poweroff_check_soc;	/* % */

        //struct pm8226_charging_current *chg_current_table;
        //unsigned int                    *siop_table;

};



/*******************************************
	SAMSUNG BATTERY ATTRS
*******************************************/

/* event check */
#define EVENT_NONE			(0)
#define EVENT_2G_CALL			(0x1 << 0)
#define EVENT_3G_CALL			(0x1 << 1)
#define EVENT_MUSIC			(0x1 << 2)
#define EVENT_VIDEO			(0x1 << 3)
#define EVENT_BROWSER			(0x1 << 4)
#define EVENT_HOTSPOT			(0x1 << 5)
#define EVENT_CAMERA			(0x1 << 6)
#define EVENT_CAMCORDER			(0x1 << 7)
#define EVENT_DATA_CALL			(0x1 << 8)
#define EVENT_WIFI			(0x1 << 9)
#define EVENT_WIBRO			(0x1 << 10)
#define EVENT_LTE			(0x1 << 11)
#define EVENT_LCD			(0x1 << 12)
#define EVENT_GPS			(0x1 << 13)

#ifdef SEC_CHARGER_CODE
ssize_t sec_bat_show_attrs(struct device *dev,
                                struct device_attribute *attr, char *buf);

ssize_t sec_bat_store_attrs(struct device *dev,
                                struct device_attribute *attr,
                                const char *buf, size_t count);


#define SEC_BATTERY_ATTR(_name)                                         \
{                                                                       \
        .attr = {.name = #_name, .mode = 0664}, \
        .show = sec_bat_show_attrs,                                     \
        .store = sec_bat_store_attrs,                                   \
}

static struct device_attribute sec_battery_attrs[] = {
        SEC_BATTERY_ATTR(batt_reset_soc),
        SEC_BATTERY_ATTR(batt_read_raw_soc),
        SEC_BATTERY_ATTR(batt_read_adj_soc),
        SEC_BATTERY_ATTR(batt_type),
        SEC_BATTERY_ATTR(batt_vfocv),
        SEC_BATTERY_ATTR(batt_vol_adc),
        SEC_BATTERY_ATTR(batt_vol_adc_cal),
        SEC_BATTERY_ATTR(batt_vol_aver),
        SEC_BATTERY_ATTR(batt_vol_adc_aver),
        SEC_BATTERY_ATTR(batt_temp_adc),
        SEC_BATTERY_ATTR(batt_temp_aver),
        SEC_BATTERY_ATTR(batt_temp_adc_aver),
        SEC_BATTERY_ATTR(batt_vf_adc),

        SEC_BATTERY_ATTR(batt_lp_charging),
        SEC_BATTERY_ATTR(siop_activated),
        SEC_BATTERY_ATTR(siop_level),
        SEC_BATTERY_ATTR(batt_charging_source),
        SEC_BATTERY_ATTR(fg_reg_dump),
        SEC_BATTERY_ATTR(fg_reset_cap),
        SEC_BATTERY_ATTR(fg_capacity),
        SEC_BATTERY_ATTR(auth),
        SEC_BATTERY_ATTR(chg_current_adc),
        SEC_BATTERY_ATTR(wc_adc),
        SEC_BATTERY_ATTR(wc_status),
        SEC_BATTERY_ATTR(factory_mode),
        SEC_BATTERY_ATTR(update),
        SEC_BATTERY_ATTR(test_mode),

        SEC_BATTERY_ATTR(talk_gsm),
        SEC_BATTERY_ATTR(talk_wcdma),
        SEC_BATTERY_ATTR(music),
        SEC_BATTERY_ATTR(video),
        SEC_BATTERY_ATTR(browser),
        SEC_BATTERY_ATTR(hotspot),
        SEC_BATTERY_ATTR(camera),
        SEC_BATTERY_ATTR(data_call),
        SEC_BATTERY_ATTR(wifi),
        SEC_BATTERY_ATTR(lte),
        SEC_BATTERY_ATTR(event),
        SEC_BATTERY_ATTR(batt_slate_mode),
};

enum {
        BATT_RESET_SOC = 0,
        BATT_READ_RAW_SOC,
        BATT_READ_ADJ_SOC,
        BATT_TYPE,
        BATT_VFOCV,
        BATT_VOL_ADC,
        BATT_VOL_ADC_CAL,
        BATT_VOL_AVER,
        BATT_VOL_ADC_AVER,
        BATT_TEMP_ADC,
        BATT_TEMP_AVER,
        BATT_TEMP_ADC_AVER,
        BATT_VF_ADC,

        BATT_LP_CHARGING,
        SIOP_ACTIVATED,
        SIOP_LEVEL,
        BATT_CHARGING_SOURCE,
        FG_REG_DUMP,
        FG_RESET_CAP,
        FG_CAPACITY,
        AUTH,
        CHG_CURRENT_ADC,
        WC_ADC,
        WC_STATUS,
        FACTORY_MODE,
        UPDATE,
        TEST_MODE,

        BATT_EVENT_GSM_CALL,
        BATT_EVENT_WCDMA_CALL,
        BATT_EVENT_MUSIC,
        BATT_EVENT_VIDEO,
        BATT_EVENT_BROWSER,
        BATT_EVENT_HOTSPOT,
        BATT_EVENT_CAMERA,
        BATT_EVENT_DATA_CALL,
        BATT_EVENT_WIFI,
        BATT_EVENT_LTE,
        BATT_EVENT,
        BATT_SLATE_MODE,
};
#endif

/*******************************************
	SAMSUNG FG ATTRS
*******************************************/

/*

ssize_t sec_fg_show_attrs(struct device *dev,
                                struct device_attribute *attr, char *buf);

ssize_t sec_fg_store_attrs(struct device *dev,
                                struct device_attribute *attr,
                                const char *buf, size_t count);

static struct device_attribute sec_fuelgauge_attrs[] = {
        SEC_FUELGAUGE_ATTR(fg_curr_ua),
};


#define SEC_FUELGAUGE_ATTR(_name)       \
{       \
        .attr = {.name = #_name, .mode = 0664}, \
        .show = sec_fg_show_attrs,              \
        .store = sec_fg_store_attrs,    \
}

enum {
        FG_CURR_UA = 0,
};

*/

#endif /* __QPNP_SEC_CHARGER_H */
