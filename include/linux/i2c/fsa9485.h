/*
 * Copyright (C) 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Wonguk Jeong <wonguk.jeong@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef _FSA9485_H_
#define _FSA9485_H_

/* FSA9485 I2C registers */
#define FSA9485_REG_DEVID		0x01
#define FSA9485_REG_CTRL		0x02
#define FSA9485_REG_INT1		0x03
#define FSA9485_REG_INT2		0x04
#define FSA9485_REG_INT1_MASK		0x05
#define FSA9485_REG_INT2_MASK		0x06
#define FSA9485_REG_ADC			0x07
#define FSA9485_REG_TIMING1		0x08
#define FSA9485_REG_TIMING2		0x09
#define FSA9485_REG_DEV_T1		0x0a
#define FSA9485_REG_DEV_T2		0x0b
#define FSA9485_REG_BTN1		0x0c
#define FSA9485_REG_BTN2		0x0d
#define FSA9485_REG_CK			0x0e
#define FSA9485_REG_CK_INT1		0x0f
#define FSA9485_REG_CK_INT2		0x10
#define FSA9485_REG_CK_INTMASK1		0x11
#define FSA9485_REG_CK_INTMASK2		0x12
#define FSA9485_REG_MANSW1		0x13
#define FSA9485_REG_MANSW2		0x14
#define FSA9485_REG_MANUAL_OVERRIDES1	0x1B
#define FSA9485_REG_RESERVED_1D		0x1D
#define FSA9485_REG_RESERVED_20		0x20


/* Control */
#define CON_SWITCH_OPEN		(1 << 4)
#define CON_RAW_DATA		(1 << 3)
#define CON_MANUAL_SW		(1 << 2)
#define CON_WAIT		(1 << 1)
#define CON_INT_MASK		(1 << 0)
#define CON_MASK		(CON_SWITCH_OPEN | CON_RAW_DATA | \
				CON_MANUAL_SW | CON_WAIT)

/* Device Type 1 */
#define DEV_USB_OTG		(1 << 7)
#define DEV_DEDICATED_CHG	(1 << 6)
#define DEV_USB_CHG		(1 << 5)
#define DEV_CAR_KIT		(1 << 4)
#define DEV_UART		(1 << 3)
#define DEV_USB			(1 << 2)
#define DEV_AUDIO_2		(1 << 1)
#define DEV_AUDIO_1		(1 << 0)

#define DEV_T1_USB_MASK		(DEV_USB_OTG | DEV_USB_CHG | DEV_USB)
#define DEV_T1_UART_MASK	(DEV_UART)
#define DEV_T1_CHARGER_MASK	(DEV_DEDICATED_CHG | DEV_CAR_KIT)

/* Device Type 2 */
#define DEV_MMDOCK		(1<<12)
#define DEV_INCOMPATIBLE	(1 << 11)
#define DEV_CHARGING_CABLE	(1 << 10)
#ifdef CONFIG_MUIC_FSA9485_SUPPORT_LANHUB
#define DEV_LANHUB		(1 << 9)
#endif
#define DEV_AUDIO_DOCK		(1 << 8)
#define DEV_SMARTDOCK		(1 << 7)
#define DEV_AV			(1 << 6)
#define DEV_TTY			(1 << 5)
#define DEV_PPD			(1 << 4)
#define DEV_JIG_UART_OFF	(1 << 3)
#define DEV_JIG_UART_ON		(1 << 2)
#define DEV_JIG_USB_OFF		(1 << 1)
#define DEV_JIG_USB_ON		(1 << 0)

#define DEV_T2_USB_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON)
#define DEV_T2_UART_MASK	(DEV_JIG_UART_OFF)
#define DEV_T2_JIG_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON | \
				DEV_JIG_UART_OFF)

/* Device Type 3 */
#define DEV_MHL				(1 << 0)
#define DEV_VBUS_DEBOUNCE		(1 << 1)
#define DEV_NON_STANDARD		(1 << 2)
#define DEV_AV_VBUS			(1 << 4)
#define DEV_APPLE_CHARGER		(1 << 5)
#define DEV_U200_CHARGER		(1 << 6)

/*
 * Manual Switch
 * D- [7:5] / D+ [4:2]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100: V_AUDIO
 */
#define SW_VAUDIO		((4 << 5) | (4 << 2) | (1 << 1) | (1 << 0))
#define SW_UART			((3 << 5) | (3 << 2))
#define SW_AUDIO		((2 << 5) | (2 << 2) | (1 << 1) | (1 << 0))
#define SW_DHOST		((1 << 5) | (1 << 2) | (1 << 1) | (1 << 0))
#define SW_AUTO			((0 << 5) | (0 << 2))
#define SW_USB_OPEN		(1 << 0)
#define SW_ALL_OPEN		(0)

/* Interrupt 1 */
#define INT_DETACH		(1 << 1)
#define INT_ATTACH		(1 << 0)

#define	ADC_GND			0x00
#define	ADC_MHL			0x01
#define	ADC_DOCK_PREV_KEY	0x04
#define	ADC_DOCK_NEXT_KEY	0x07
#define	ADC_DOCK_VOL_DN		0x0a
#define	ADC_DOCK_VOL_UP		0x0b
#define	ADC_DOCK_PLAY_PAUSE_KEY 0x0d
#ifdef CONFIG_MUIC_FSA9485_SUPPORT_LANHUB
#define ADC_LANHUB		0x13
#endif
#define ADC_CHARGING_CABLE	0x14
#define ADC_MMDOCK		0x15
#define	ADC_CEA936ATYPE1_CHG	0x17
#define	ADC_JIG_USB_OFF		0x18
#define	ADC_JIG_USB_ON		0x19
#define	ADC_DESKDOCK		0x1a
#define	ADC_CEA936ATYPE2_CHG	0x1b
#define	ADC_JIG_UART_OFF	0x1c
#define	ADC_JIG_UART_ON		0x1d
#define	ADC_CARDOCK		0x1d
#define	ADC_OPEN		0x1f


enum cable_type_t {
	CABLE_TYPE_NONE = 0,
	CABLE_TYPE_USB,
	CABLE_TYPE_AC,
	CABLE_TYPE_MISC,
	CABLE_TYPE_CARDOCK,
#ifdef CONFIG_MUIC_FSA9485_SUPPORT_LANHUB
	CABLE_TYPE_LANHUB,
#endif
	CABLE_TYPE_UARTOFF,
	CABLE_TYPE_JIG,
	CABLE_TYPE_UNKNOWN,
	CABLE_TYPE_CDP,
	CABLE_TYPE_SMART_DOCK,
	CABLE_TYPE_OTG,
	CABLE_TYPE_CHARGING_CABLE,
	CABLE_TYPE_AUDIO_DOCK,
#ifdef CONFIG_WIRELESS_CHARGING
	CABLE_TYPE_WPC,
#endif
	CABLE_TYPE_INCOMPATIBLE,
	CABLE_TYPE_DESK_DOCK,
	CABLE_TYPE_MM_DOCK,
};


enum {
	DOCK_KEY_NONE			= 0,
	DOCK_KEY_VOL_UP_PRESSED,
	DOCK_KEY_VOL_UP_RELEASED,
	DOCK_KEY_VOL_DOWN_PRESSED,
	DOCK_KEY_VOL_DOWN_RELEASED,
	DOCK_KEY_PREV_PRESSED,
	DOCK_KEY_PREV_RELEASED,
	DOCK_KEY_PLAY_PAUSE_PRESSED,
	DOCK_KEY_PLAY_PAUSE_RELEASED,
	DOCK_KEY_NEXT_PRESSED,
	DOCK_KEY_NEXT_RELEASED,

};


enum {
	FSA9485_NONE = -1,
	FSA9485_DETACHED = 0,
	FSA9485_ATTACHED = 1
};

enum {
	FSA9485_MMDOCK_ATTACHED = 2
};

enum {
	FSA9485_DETACHED_DOCK = 0,
	FSA9485_ATTACHED_DESK_DOCK,
	FSA9485_ATTACHED_CAR_DOCK,
	FSA9485_ATTACHED_DESK_DOCK_NO_VBUS,
};

#define UART_SEL_SW	    58

struct fsa9485_platform_data {
	int gpio_int;
	u32 irq_gpio_flags;
	int gpio_sda;
	u32 sda_gpio_flags;
	int gpio_scl;
	u32 scl_gpio_flags;

	void (*cfg_gpio) (void);
	void (*otg_cb) (bool attached);
	void (*charge_cb) (bool attached);
	void (*usb_cb) (bool attached);
	void (*uart_cb) (bool attached);
	void (*charger_cb) (bool attached);
	void (*in_charger_cb) (bool attached);
	void (*jig_cb) (bool attached);
	void (*mhl_cb) (int attached);
	void (*reset_cb) (void);
	void (*set_init_flag) (void);
	void (*mhl_sel) (bool onoff);
	void (*dock_cb) (int attached);
	int  (*dock_init) (void);
	void (*usb_cdp_cb) (bool attached);
#ifdef CONFIG_MUIC_FSA9485_SUPPORT_LANHUB
	void (*lanhub_cb) (bool attached);
	void (*lanhubta_cb) (bool attached);
#endif
	void (*smartdock_cb) (bool attached);
	void (*audio_dock_cb) (bool attached);
	void (*mmdock_cb) (bool attached);
};

enum {
	SWITCH_PORT_AUTO = 0,
	SWITCH_PORT_USB,
	SWITCH_PORT_AUDIO,
	SWITCH_PORT_UART,
	SWITCH_PORT_VAUDIO,
	SWITCH_PORT_USB_OPEN,
	SWITCH_PORT_ALL_OPEN,
};

extern void fsa9485_manual_switching(int path);
extern void fsa9485_otg_detach(void);
#if defined(CONFIG_MACH_AEGIS2)
extern void fsa9485_checkandhookaudiodockfornoise(int value);
#endif
extern struct class *sec_class;
extern struct fsa9485_platform_data fsa9485_pdata;
extern int check_jig_state(void);
extern int check_mmdock_connect(void);
extern int poweroff_charging;

#endif /* _FSA9485_H_ */
