/*
 * Copyright (C) 2013 Samsung Electronics
 * Nitin Chaudhary <nc.chaudhary@samsung.com>
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

#ifndef _MUIC_H_
#define _MUIC_H_

#ifndef CONFIG_USB_HOST_NOTIFY
enum sec_otg_dummy_defines {
	HNOTIFY_MODE=1,
	NOTIFY_TEST_MODE=3,
};
extern int sec_get_notification(int mode);
#endif
enum cable_type_t {
        CABLE_TYPE_NONE = 0,
        CABLE_TYPE_USB,
        CABLE_TYPE_AC,
        CABLE_TYPE_MISC,
        CABLE_TYPE_CARDOCK,
        CABLE_TYPE_UARTOFF,
        CABLE_TYPE_JIG,
        CABLE_TYPE_UNKNOWN,
        CABLE_TYPE_CDP,
        CABLE_TYPE_SMART_DOCK,
        CABLE_TYPE_OTG,
        CABLE_TYPE_AUDIO_DOCK,
#ifdef CONFIG_WIRELESS_CHARGING
        CABLE_TYPE_WPC,
#endif
        CABLE_TYPE_INCOMPATIBLE,
        CABLE_TYPE_DESK_DOCK,
        CABLE_TYPE_JIG_UART_OFF_VB,
        CABLE_TYPE_DESK_DOCK_NO_VB,
	CABLE_TYPE_LANHUB,
	CABLE_TYPE_219KUSB,
	CABLE_TYPE_CHARGING_CABLE,
};

/* MUIC attached device type */
enum muic_attached_dev {
    ATTACHED_DEV_NONE_MUIC = 0,
    ATTACHED_DEV_USB_MUIC,
    ATTACHED_DEV_CDP_MUIC,
    ATTACHED_DEV_OTG_MUIC,
    ATTACHED_DEV_LANHUB_MUIC,
    ATTACHED_DEV_TA_MUIC,
    ATTACHED_DEV_DESKDOCK_MUIC,
    ATTACHED_DEV_CARDOCK_MUIC,
    ATTACHED_DEV_AUDIODOCK_MUIC,
    ATTACHED_DEV_JIG_UART_OFF_MUIC,
    ATTACHED_DEV_JIG_UART_OFF_VB_MUIC,  /* VBUS enabled */
    ATTACHED_DEV_JIG_UART_ON_MUIC,
    ATTACHED_DEV_JIG_USB_OFF_MUIC,
    ATTACHED_DEV_JIG_USB_ON_MUIC,
    ATTACHED_DEV_UNKNOWN_MUIC
};

enum {
	LANHUB = 0,
	LANHUB_TA = 1,
};

enum {
	DISABLE,
	ENABLE
};

enum {
	DOCK_UI_DESK = 1,
	DOCK_UI_CAR
};

#ifndef CONFIG_USB_HOST_NOTIFY
extern int sec_get_notification(int mode);
#endif
extern int poweroff_charging;
#if defined(CONFIG_VIDEO_MHL_V2)
extern int dock_det(void);
#endif
extern struct class *sec_class;

#endif /* _MUIC_H_ */

