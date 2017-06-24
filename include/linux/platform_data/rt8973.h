/*
 * include/linux/mfd/rt8973.h
 *
 * Richtek RT8973 driver header file
 *
 * Copyright (C) 2013 Richtek Technology Corp.
 * Author: Patrick Chang <patrick_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef LINUX_MFD_RT8973_H
#define LINUX_MFD_RT8973_H

#include <linux/i2c/tsu6721.h>

/* Marvell MMP arch*/
#ifdef CONFIG_ARCH_MMP
#define SAMSUNG_MVRL_MUIC_RT8973 1
#endif

#ifdef CONFIG_SEC_FACTORY
#if defined(CONFIG_MACH_KANAS3G_CTC) || defined(CONFIG_MACH_HEAT_AIO) || defined(CONFIG_MACH_KANAS3G_CMCC)
#define MUIC_SUPPORT_CARDOCK_FUNCTION 1
#endif
#endif

#if defined(CONFIG_RT8973_JIG_WAKEUP)
extern int rt8973_dock_init(void);
#endif

enum {
    MUIC_RT8973_CABLE_TYPE_NONE = 0,
    MUIC_RT8973_CABLE_TYPE_UART,            //adc 0x16
    MUIC_RT8973_CABLE_TYPE_USB,             //adc 0x1f (none id)
    MUIC_RT8973_CABLE_TYPE_OTG,             //adc 0x00, regDev1&0x01
    /* TA Group */
    MUIC_RT8973_CABLE_TYPE_REGULAR_TA,      //adc 0x1f (none id, D+ short to D-)
    MUIC_RT8973_CABLE_TYPE_ATT_TA,          //adc 0x1f (none id, only VBUS)AT&T
    MUIC_RT8973_CABLE_TYPE_0x15,            //adc 0x15
    MUIC_RT8973_CABLE_TYPE_TYPE1_CHARGER,   //adc 0x17 (id : 200k)
    MUIC_RT8973_CABLE_TYPE_0x1A,            //adc 0x1A
    /* JIG Group */
    MUIC_RT8973_CABLE_TYPE_JIG_USB_OFF,     //adc 0x18
    MUIC_RT8973_CABLE_TYPE_JIG_USB_ON,      //adc 0x19
    MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF,    //adc 0x1C
    MUIC_RT8973_CABLE_TYPE_JIG_UART_ON,     //adc 0x1D
    // JIG type with VBUS
    MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF_WITH_VBUS,    //adc 0x1C
    MUIC_RT8973_CABLE_TYPE_JIG_UART_ON_WITH_VBUS,     //adc 0x1D

    MUIC_RT8973_CABLE_TYPE_CDP, // USB Charging downstream port, usually treated as SDP
    MUIC_RT8973_CABLE_TYPE_L200K_SPEC_USB, // 200K special USB cable
    MUIC_RT8973_CABLE_TYPE_UNKNOWN,
    MUIC_RT8973_CABLE_TYPE_INVALID, /* Means un-initialized*/
};

/*enum cable_type_t {
        CABLE_TYPE_NONE = 0,
        CABLE_TYPE_USB =MUIC_RT8973_CABLE_TYPE_USB,
        CABLE_TYPE_AC,
        CABLE_TYPE_MISC,
        CABLE_TYPE_CARDOCK,
        CABLE_TYPE_UARTOFF,
        CABLE_TYPE_JIG,
        CABLE_TYPE_UNKNOWN =MUIC_RT8973_CABLE_TYPE_UNKNOWN,
        CABLE_TYPE_CDP = MUIC_RT8973_CABLE_TYPE_CDP,
        CABLE_TYPE_SMART_DOCK,
        CABLE_TYPE_OTG = MUIC_RT8973_CABLE_TYPE_OTG,
        CABLE_TYPE_AUDIO_DOCK,
#ifdef CONFIG_WIRELESS_CHARGING
        CABLE_TYPE_WPC,
#endif
        CABLE_TYPE_INCOMPATIBLE,
        CABLE_TYPE_DESK_DOCK,
};*/

typedef enum {
    JIG_USB_BOOT_OFF,
    JIG_USB_BOOT_ON,
    JIG_UART_BOOT_OFF,
    JIG_UART_BOOT_ON,
} jig_type_t;

struct rt8973_platform_data {
    int irq_gpio;
    void (*cable_chg_callback)(int32_t cable_type);
    void (*ocp_callback)(void);
    void (*otp_callback)(void);
    void (*ovp_callback)(void);
    void (*usb_callback)(uint8_t attached);
    void (*uart_callback)(uint8_t attached);
    void (*otg_callback)(uint8_t attached);
    void (*jig_callback)(jig_type_t type, uint8_t attached);
	int	(*dock_init) (void);
	int gpio_int;
	u32 irq_gpio_flags;
//	int gpio_sda;
//	u32 sda_gpio_flags;
//	int gpio_scl;
//	u32 scl_gpio_flags;
};

extern struct rt8973_platform_data rt8973_pdata;
extern int rt_check_jig_state(void);
extern struct class *sec_class;
#endif // LINUX_MFD_RT8973_H

