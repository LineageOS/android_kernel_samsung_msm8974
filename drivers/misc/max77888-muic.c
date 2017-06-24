/*
 * max77888-muic.c - MUIC driver for the Maxim 77888
 *
 *  Copyright (C) 2012 Samsung Electronics
 *  <sukdong.kim@samsung.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/mfd/max77888.h>
#include <linux/mfd/max77888-private.h>
#include <linux/host_notify.h>
#include <linux/wakelock.h>
#ifdef CONFIG_USBHUB_USB3803
#include <linux/usb3803.h>
#endif
#include <linux/delay.h>
/* #include <linux/extcon.h> */

#define DEV_NAME	"max77888-muic"
#if defined(CONFIG_MACH_J_CHN_CTC) || \
	defined(CONFIG_MACH_J_CHN_CU)
#define REGARD_442K_AS_523K
#endif

/* for providing API */
static struct max77888_muic_info *gInfo;

/* For restore charger interrupt states */
static u8 chg_int_state;

extern int max77888_get_jig_state(void);

#ifdef CONFIG_LTE_VIA_SWITCH
/* For saving uart path during CP booting */
static int cpbooting;
#endif

/* MAX77888 MUIC CHG_TYP setting values */
enum {
	/* No Valid voltage at VB (Vvb < Vvbdet) */
	CHGTYP_NO_VOLTAGE	= 0x00,
	/* Unknown (D+/D- does not present a valid USB charger signature) */
	CHGTYP_USB		= 0x01,
	/* Charging Downstream Port */
	CHGTYP_DOWNSTREAM_PORT	= 0x02,
	/* Dedicated Charger (D+/D- shorted) */
	CHGTYP_DEDICATED_CHGR	= 0x03,
	/* Special 500mA charger, max current 500mA */
	CHGTYP_500MA		= 0x04,
	/* Special 1A charger, max current 1A */
	CHGTYP_1A		= 0x05,
	/* Special Charger */
	CHGTYP_SPECIAL_CHGR		= 0x06,
	/* Dead Battery Charging, max current 100mA */
	CHGTYP_DB_100MA		= 0x07,
	CHGTYP_MAX,

	CHGTYP_INIT,
	CHGTYP_MIN = CHGTYP_NO_VOLTAGE
};

enum {
	ADC_GND			= 0x00,
	ADC_MHL			= 0x01,
	ADC_DOCK_PREV_KEY	= 0x04,
	ADC_DOCK_NEXT_KEY	= 0x07,
	ADC_DOCK_VOL_DN		= 0x0a, /* 0x01010 14.46K ohm */
	ADC_DOCK_VOL_UP		= 0x0b, /* 0x01011 17.26K ohm */
	ADC_DOCK_PLAY_PAUSE_KEY = 0x0d,
	ADC_INCOMPATIBLE	= 0x0f, /* 0x01111 34K ohm */
	ADC_SMARTDOCK		= 0x10, /* 0x10000 40.2K ohm */
	ADC_HMT			= 0x11, /* 0x10001 49.9K ohm */
	ADC_AUDIODOCK		= 0x12, /* 0x10010 64.9K ohm */
	ADC_CHARGING_CABLE	= 0x14, /* 0x10100 102K ohm */
	ADC_LANHUB		= 0x13, /* 0x10011 80.07K ohm */
	ADC_MMDOCK		= 0x15,	/* 0x10101 121K ohm */
	ADC_CEA936ATYPE1_CHG	= 0x17,	/* 0x10111 200K ohm */
	ADC_JIG_USB_OFF		= 0x18, /* 0x11000 255K ohm */
	ADC_JIG_USB_ON		= 0x19, /* 0x11001 301K ohm */
	ADC_DESKDOCK		= 0x1a, /* 0x11010 365K ohm */
	ADC_CEA936ATYPE2_CHG	= 0x1b, /* 0x11011 442K ohm */
	ADC_JIG_UART_OFF	= 0x1c, /* 0x11100 523K ohm */
	ADC_JIG_UART_ON		= 0x1d, /* 0x11101 619K ohm */
	ADC_CARDOCK		= 0x1d, /* 0x11101 619K ohm */
	ADC_OPEN		= 0x1f
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

struct max77888_muic_info {
	struct device		*dev;
	struct max77888_dev	*max77888;
	struct i2c_client	*muic;
	struct max77888_muic_data *muic_data;
	int			irq_adc;
	int			irq_chgtype;
	int			irq_vbvolt;
	int			irq_adc1k;
	int			mansw;

	struct wake_lock muic_wake_lock;

	enum cable_type_muic	cable_type;

	u8		adc;
	u8		chgtyp;
	u8		vbvolt;

#if defined(CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK) ||\
	defined(CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK)
	struct delayed_work	dock_work;
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK ||
	CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK */
	struct delayed_work	init_work;
	struct delayed_work	usb_work;
	struct delayed_work	dock_usb_work;
	struct delayed_work	mhl_work;
	struct mutex		mutex;

	bool			is_usb_ready;
	bool			is_mhl_ready;
	bool			is_otg_enable;

	struct input_dev	*input;
	int			previous_key;
	bool			is_adc_open_prev;

#if !defined(CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK)
	bool			is_factory_start;
#endif /* !CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK */

#ifdef CONFIG_EXTCON
	struct extcon_dev	*edev;
#endif
};

static int if_muic_info;
static int switch_sel;
static int if_pmic_rev;
#if defined(REGARD_442K_AS_523K)
static int is_factory_mode = -1;
#endif

/* func : get_if_pmic_inifo
 * switch_sel value get from bootloader comand line
 * switch_sel data consist 8 bits (xxxxzzzz)
 * first 4bits(zzzz) mean path infomation.
 * next 4bits(xxxx) mean if pmic version info
 */
static int get_if_pmic_inifo(char *str)
{
	get_option(&str, &if_muic_info);
	switch_sel = if_muic_info & 0x0f;
	if_pmic_rev = (if_muic_info & 0xf0) >> 4;
	pr_info("%s %s: switch_sel: %x if_pmic_rev:%x\n",
		__FILE__, __func__, switch_sel, if_pmic_rev);
	return if_muic_info;
}
__setup("pmic_info=", get_if_pmic_inifo);

int get_switch_sel(void)
{
	return switch_sel;
}

int g_usbvbus;

int max77888_muic_read_adc(void)
{
	if(gInfo)
		return gInfo->adc;
	else
		return -1;
}
EXPORT_SYMBOL_GPL(max77888_muic_read_adc);

int max77888_muic_read_vbvolt(void)
{
	if(gInfo)
		return gInfo->vbvolt;
	else
		return -1;
}
EXPORT_SYMBOL_GPL(max77888_muic_read_vbvolt);

int max77888_muic_read_vbus(void)
{
	return g_usbvbus;
}
EXPORT_SYMBOL_GPL(max77888_muic_read_vbus);

#if 0 // unused
static int max77888_muic_get_comp2_comn1_pass2
	(struct max77888_muic_info *info)
{
	int ret;
	u8 val;

	ret = max77888_read_reg(info->muic, MAX77888_MUIC_REG_CTRL1, &val);
	val = val & CLEAR_IDBEN_MICEN_MASK;
	dev_info(info->dev, "func:%s ret:%d val:%x\n", __func__, ret, val);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return -EINVAL;
	}
	return val;
}
#endif

static int max77888_muic_set_comp2_comn1_pass2
	(struct max77888_muic_info *info, int type, int path)
{
	/* type 0 == usb, type 1 == uart */
	u8 cntl1_val, cntl1_msk;
	int ret = 0;
	int val;

	dev_info(info->dev, "func: %s type: %d path: %d\n",
		__func__, type, path);
	if (type == 0) {
		if (path == AP_USB_MODE) {
			info->muic_data->sw_path = AP_USB_MODE;
			val = MAX77888_MUIC_CTRL1_BIN_1_001;
		} else if (path == CP_USB_MODE) {
			info->muic_data->sw_path = CP_USB_MODE;
			val = MAX77888_MUIC_CTRL1_BIN_4_100;
		} else {
			dev_err(info->dev, "func: %s invalid usb path\n"
				, __func__);
			return -EINVAL;
		}
	} else if (type == 1) {
		if (path == UART_PATH_AP) {
			info->muic_data->uart_path = UART_PATH_AP;
			val = MAX77888_MUIC_CTRL1_BIN_3_011;
		} else if (path == UART_PATH_CP) {
			info->muic_data->uart_path = UART_PATH_CP;
			val = MAX77888_MUIC_CTRL1_BIN_5_101;
#ifdef CONFIG_LTE_VIA_SWITCH
			dev_info(info->dev, "%s: cpbooting is %s\n",
				__func__,
				cpbooting ?
				"started. skip path set" : "done. set path");
			if (!cpbooting) {
				if (gpio_is_valid(GPIO_LTE_VIA_UART_SEL)) {
					gpio_set_value(GPIO_LTE_VIA_UART_SEL,
						GPIO_LEVEL_HIGH);
					dev_info(info->dev,
						"%s: LTE_GPIO_LEVEL_HIGH"
						, __func__);
				} else {
					dev_err(info->dev,
						"%s: ERR_LTE_GPIO_SET_HIGH\n"
						, __func__);
					return -EINVAL;
				}
			}
#endif
		}
#ifdef CONFIG_LTE_VIA_SWITCH
		else if (path == UART_PATH_LTE) {
			info->muic_data->uart_path = UART_PATH_LTE;
			val = MAX77888_MUIC_CTRL1_BIN_5_101;
			if (gpio_is_valid(GPIO_LTE_VIA_UART_SEL)) {
				gpio_set_value(GPIO_LTE_VIA_UART_SEL,
					GPIO_LEVEL_LOW);
				dev_info(info->dev, "%s: LTE_GPIO_LEVEL_LOW\n"
								, __func__);
			} else {
				dev_err(info->dev, "%s: ERR_LTE_GPIO_SET_LOW\n"
								, __func__);
				return -EINVAL;
			}
		}
#endif
		else {
			dev_err(info->dev, "func: %s invalid uart path\n"
				, __func__);
			return -EINVAL;
		}
	}
	else {
		dev_err(info->dev, "func: %s invalid path type(%d)\n"
			, __func__, type);
		return -EINVAL;
	}

	cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;

	max77888_update_reg(info->muic, MAX77888_MUIC_REG_CTRL1, cntl1_val,
			    cntl1_msk);

	return ret;
}

#if 0 // unused
static int max77888_muic_set_usb_path_pass2
	(struct max77888_muic_info *info, int path)
{
	int ret = 0;
	ret = max77888_muic_set_comp2_comn1_pass2
		(info, 0/*usb*/, path);
	sysfs_notify(&switch_dev->kobj, NULL, "usb_sel");
	return ret;
}

static int max77888_muic_get_usb_path_pass2
	(struct max77888_muic_info *info)
{
	u8 val;

	val = max77888_muic_get_comp2_comn1_pass2(info);
	if (val == CTRL1_AP_USB)
		return AP_USB_MODE;
	else if (val == CTRL1_CP_USB)
		return CP_USB_MODE;
	else if (val == CTRL1_AUDIO)
		return AUDIO_MODE;
	else
		return -EINVAL;
}
#endif

#ifdef CONFIG_MUIC_RESET_PIN_ENABLE
void max77888_muic_regdump(void) {
	u8 r_value[MAX77888_MUIC_REG_END];
	int ret;

	ret = max77888_bulk_read(gInfo->muic, MAX77888_MUIC_REG_ID, ARRAY_SIZE(r_value)-1, r_value);
	max77888_read_reg(gInfo->muic, MAX77888_MUIC_REG_CTRL4, &r_value[ARRAY_SIZE(r_value)-1]);

	pr_info("%s:MUIC REG DUMP\n", __func__);
	pr_info("%s:READ MUIC REG ID : %02x", __func__, r_value[0]);
	pr_info("%s:READ MUIC REG INT1 : %02x", __func__, r_value[1]);
	pr_info("%s:READ MUIC REG INT2 : %02x", __func__, r_value[2]);
	pr_info("%s:READ MUIC REG INT3 : %02x", __func__, r_value[3]);
	pr_info("%s:READ MUIC REG STATUS1 : %02x", __func__, r_value[4]);
	pr_info("%s:READ MUIC REG STATUS2 : %02x", __func__, r_value[5]);
	pr_info("%s:READ MUIC REG STATUS3 : %02x", __func__, r_value[6]);
	pr_info("%s:READ MUIC REG INTMASK1 : %02x", __func__, r_value[7]);
	pr_info("%s:READ MUIC REG INTMASK2 : %02x", __func__, r_value[8]);
	pr_info("%s:READ MUIC REG INTMASK3 : %02x", __func__, r_value[9]);
	pr_info("%s:READ MUIC REG CDETCTRL1 : %02x", __func__, r_value[10]);
	pr_info("%s:READ MUIC REG CDETCTRL2 : %02x", __func__, r_value[11]);
	pr_info("%s:READ MUIC REG CTRL1 : %02x", __func__, r_value[12]);
	pr_info("%s:READ MUIC REG CTRL2 : %02x", __func__, r_value[13]);
	pr_info("%s:READ MUIC REG CTRL3  : %02x", __func__, r_value[14]);
	pr_info("%s:READ MUIC REG CTRL4 : %02x", __func__, r_value[ARRAY_SIZE(r_value)-1]);
}
EXPORT_SYMBOL_GPL(max77888_muic_regdump);
#endif

static int max77888_muic_set_uart_path_pass2
	(struct max77888_muic_info *info, int path)
{
	int ret = 0;

	switch (info->cable_type) { 
	case CABLE_TYPE_JIG_UART_OFF_MUIC: 
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC: 
		ret = max77888_muic_set_comp2_comn1_pass2
			(info, 1/*uart*/, path);
		break; 
	default: 
		pr_info("%s:%s JIG UART OFF isn't connected," 
			"don't change MUIC path\n", DEV_NAME, __func__); 
		break; 
	} 

	return ret;

}

#if defined(CONFIG_LEDS_MAX77888)
/*
 * func: max77888_muic_set_jigset
 * arg: Manual control
 * (bit[1:0] 00=Auto detection, 01=Output Low, 10(or 11)=Hi-Impedance)
 * return: only 0 success
 */
int max77888_muic_set_jigset(int reg_value)
{
	struct i2c_client *client = gInfo->muic;
	u8 cntl3_val = 0;
	int ret;

	max77888_read_reg(client, MAX77888_MUIC_REG_CTRL3, &cntl3_val);
	dev_info(gInfo->dev, "%s: Before CNTL3(0x0E : 0x%02x) , reg_value : 0x%X\n", __func__, cntl3_val, reg_value);

	ret = max77888_update_reg(client, MAX77888_MUIC_REG_CTRL3, reg_value << CTRL3_JIGSET_SHIFT, CTRL3_JIGSET_MASK);
	if (ret) {
		pr_err("%s: fail to update muic CTRL3 reg(%d)\n", __func__, ret);
	}

	max77888_read_reg(client, MAX77888_MUIC_REG_CTRL3, &cntl3_val);
	dev_info(gInfo->dev, "%s: After CNTL3(0x0E : 0x%02x)\n", __func__, cntl3_val);

	return ret;
}
#endif

static void max77888_muic_set_adc_mode(struct max77888_muic_info *info, int mode)
{
	max77888_update_reg(info->muic, MAX77888_MUIC_REG_CTRL4,
		mode << CTRL4_ADCMODE_SHIFT, CTRL4_ADCMODE_MASK);
	dev_info(info->dev, "func:%s, ADCMOE(0x%x)\n", __func__, mode);
}

#if defined(REGARD_442K_AS_523K)
static void max77888_muic_force_uart_switch(int uart_path)
{
	u8 ctrl1_mask, ctrl1_val;
	u8 ctrl2_val;
	u8 gpio_uart_sel = 0;

	switch (uart_path)	{
	case UART_PATH_CP:
		/* Switch UART path to MASTER (PMB9811C, infinion) */
		pr_info("[%s] Force UART path switch to CP (infi)\n",
				__func__);
		ctrl1_val =
			(MAX77888_MUIC_CTRL1_BIN_5_101<<COMN1SW_SHIFT) |
			(MAX77888_MUIC_CTRL1_BIN_5_101<<COMP2SW_SHIFT);
		ctrl1_mask = COMN1SW_MASK | COMP2SW_MASK;
		gpio_uart_sel = GPIO_LEVEL_LOW;
		break;
	case UART_PATH_CP_ESC:
		/* Switch UART path to SLAVE (ESC6270, qualcomm) */
		pr_info("[%s] Force UART path switch to CP (esc)\n",
				__func__);
		ctrl1_val =
			(MAX77888_MUIC_CTRL1_BIN_5_101<<COMN1SW_SHIFT) |
			(MAX77888_MUIC_CTRL1_BIN_5_101<<COMP2SW_SHIFT);
		ctrl1_mask = COMN1SW_MASK | COMP2SW_MASK;
		gpio_uart_sel = GPIO_LEVEL_HIGH;
		break;
	case UART_PATH_AP:
		/* Switch UART path to AP */
		pr_info("[%s] Force UART path switch to AP\n",
				__func__);
		ctrl1_val =
			(MAX77888_MUIC_CTRL1_BIN_3_011<<COMN1SW_SHIFT) |
			(MAX77888_MUIC_CTRL1_BIN_3_011<<COMP2SW_SHIFT);
		ctrl1_mask = COMN1SW_MASK | COMP2SW_MASK;
		break;
	default:
		pr_info("[%s] wrong uart_path, return\n", __func__);
		return;
		break;
	}

	max77888_update_reg(gInfo->muic, MAX77888_MUIC_REG_CTRL1,
						ctrl1_val, ctrl1_mask);
	max77888_update_reg(gInfo->muic,
					MAX77888_MUIC_REG_CTRL2,
					0 << CTRL2_ACCDET_SHIFT,
					CTRL2_ACCDET_MASK);
	max77888_read_reg(gInfo->muic, MAX77888_MUIC_REG_CTRL1, &ctrl1_val);
	max77888_read_reg(gInfo->muic, MAX77888_MUIC_REG_CTRL2, &ctrl2_val);
	pr_info("[%s] REG_CTRL1=0x%x, REG_CTRL2=0x%x\n",
			__func__, ctrl1_val, ctrl2_val);
	if (uart_path != UART_PATH_AP)
		gpio_set_value(GPIO_UART_SEL, gpio_uart_sel);
	pr_info("[%s] GPIO_UART_SEL(%d)\n",
			__func__, gpio_get_value(GPIO_UART_SEL));

}

static void max77888_muic_switch_uart_path_default(void)
{
	int switch_sel = get_switch_sel();
	switch_sel &= 0xf;

	switch(switch_sel & MAX77888_SWITCH_SEL_2nd_BIT_UART)	{
	case 0x00 << 2:
		max77888_muic_force_uart_switch(UART_PATH_CP);
		break;
	case 0x01 << 2:
		max77888_muic_force_uart_switch(UART_PATH_AP);
		break;
	case 0x02 << 2:
		max77888_muic_force_uart_switch(UART_PATH_CP_ESC);
		break;
	default:
		pr_err("%s: unexpected switch_sel(0x%x)\n", __func__, switch_sel);
		break;
	}

	return;
}

#endif

static ssize_t max77888_muic_show_usb_state(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	dev_info(info->dev, "func:%s info->cable_type:%d\n",
		 __func__, info->cable_type);
	switch (info->cable_type) {
	case CABLE_TYPE_USB_MUIC:
	case CABLE_TYPE_JIG_USB_OFF_MUIC:
	case CABLE_TYPE_JIG_USB_ON_MUIC:
		return sprintf(buf, "USB_STATE_CONFIGURED\n");
	default:
		break;
	}

	return sprintf(buf, "USB_STATE_NOTCONFIGURED\n");
}

static ssize_t max77888_muic_show_device(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	dev_info(info->dev, "func:%s info->cable_type:%d\n",
		 __func__, info->cable_type);

	switch (info->cable_type) {
	case CABLE_TYPE_NONE_MUIC:
		return sprintf(buf, "No cable\n");
	case CABLE_TYPE_USB_MUIC:
		return sprintf(buf, "USB\n");
	case CABLE_TYPE_OTG_MUIC:
		return sprintf(buf, "OTG\n");
	case CABLE_TYPE_TA_MUIC:
		return sprintf(buf, "TA\n");
	case CABLE_TYPE_LANHUB_MUIC:
		return sprintf(buf, "LANHUB\n");
	case CABLE_TYPE_DESKDOCK_MUIC:
		return sprintf(buf, "Desk Dock\n");
	case CABLE_TYPE_CARDOCK_MUIC:
		return sprintf(buf, "Car Dock\n");
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
		return sprintf(buf, "JIG UART OFF\n");
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
		return sprintf(buf, "JIG UART OFF/VB\n");
	case CABLE_TYPE_JIG_UART_ON_MUIC:
		return sprintf(buf, "JIG UART ON\n");
	case CABLE_TYPE_JIG_USB_OFF_MUIC:
		return sprintf(buf, "JIG USB OFF\n");
	case CABLE_TYPE_JIG_USB_ON_MUIC:
		return sprintf(buf, "JIG USB ON\n");
	case CABLE_TYPE_MHL_MUIC:
		return sprintf(buf, "mHL\n");
	case CABLE_TYPE_MHL_VB_MUIC:
		return sprintf(buf, "mHL charging\n");
	case CABLE_TYPE_SMARTDOCK_MUIC:
		return sprintf(buf, "Smart Dock\n");
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
		return sprintf(buf, "Smart Dock+TA\n");
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		return sprintf(buf, "Smart Dock+USB\n");
	case CABLE_TYPE_AUDIODOCK_MUIC:
		return sprintf(buf, "Audio Dock\n");
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

static ssize_t max77888_muic_show_manualsw(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);

	switch (info->muic_data->sw_path) {
	case AP_USB_MODE:
		pr_info("func:%s PDA\n", __func__);
		return sprintf(buf, "PDA\n");
	case CP_USB_MODE:
		pr_info("func:%s MODEM\n", __func__);
		return sprintf(buf, "MODEM\n");
	case AUDIO_MODE:
		return sprintf(buf, "Audio\n");
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

static ssize_t max77888_muic_set_manualsw(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);

	pr_info("func:%s buf:%s,count:%d\n", __func__, buf, count);

	if (!strncasecmp(buf, "PDA", 3)) {
		info->muic_data->sw_path = AP_USB_MODE;
		dev_info(info->dev, "%s: AP_USB_MODE\n", __func__);
	} else if (!strncasecmp(buf, "MODEM", 5)) {
		info->muic_data->sw_path = CP_USB_MODE;
		dev_info(info->dev, "%s: CP_USB_MODE\n", __func__);
	} else
		dev_warn(info->dev, "%s: Wrong command\n", __func__);

	return count;
}

static ssize_t max77888_muic_show_adc(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = max77888_read_reg(info->muic, MAX77888_MUIC_REG_STATUS1, &val);
	dev_info(info->dev, "func:%s ret:%d val:%x\n", __func__, ret, val);

	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}

	return sprintf(buf, "%x\n", (val & STATUS1_ADC_MASK));
}

static ssize_t max77888_muic_show_audio_path(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = max77888_read_reg(info->muic, MAX77888_MUIC_REG_CTRL1, &val);
	dev_info(info->dev, "func:%s ret:%d val:%x\n", __func__, ret, val);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}

	return sprintf(buf, "%x\n", val);
}

static ssize_t max77888_muic_set_audio_path(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t count)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->muic;
	u8 cntl1_val, cntl1_msk;
	u8 val;
	dev_info(info->dev, "func:%s buf:%s\n", __func__, buf);
	if (!strncmp(buf, "0", 1))
		val = MAX77888_MUIC_CTRL1_BIN_0_000;
	else if (!strncmp(buf, "1", 1))
		val = MAX77888_MUIC_CTRL1_BIN_2_010;
	else {
		dev_warn(info->dev, "%s: Wrong command\n", __func__);
		return count;
	}

	cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT) |
	    (0 << MICEN_SHIFT);
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK | MICEN_MASK;

	max77888_update_reg(client, MAX77888_MUIC_REG_CTRL1, cntl1_val,
			    cntl1_msk);
	dev_info(info->dev, "MUIC cntl1_val:%x, cntl1_msk:%x\n", cntl1_val,
		 cntl1_msk);

	cntl1_val = MAX77888_MUIC_CTRL1_BIN_0_000;
	max77888_read_reg(client, MAX77888_MUIC_REG_CTRL1, &cntl1_val);
	dev_info(info->dev, "%s: CNTL1(0x%02x)\n", __func__, cntl1_val);

	return count;
}

static ssize_t max77888_muic_show_otg_test(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = max77888_read_reg(info->muic, MAX77888_MUIC_REG_CDETCTRL1, &val);
	dev_info(info->dev, "func:%s ret:%d val:%x buf%s\n",
		 __func__, ret, val, buf);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	val &= CHGDETEN_MASK;

	return sprintf(buf, "%x\n", val);
}

static ssize_t max77888_muic_set_otg_test(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->muic;
	u8 val;

	dev_info(info->dev, "func:%s buf:%s\n", __func__, buf);
	if (!strncmp(buf, "0", 1))
		val = 0;
	else if (!strncmp(buf, "1", 1))
		val = 1;
	else {
		dev_warn(info->dev, "%s: Wrong command\n", __func__);
		return count;
	}

	max77888_update_reg(client, MAX77888_MUIC_REG_CDETCTRL1,
			    val << CHGDETEN_SHIFT, CHGDETEN_MASK);

	val = 0;
	max77888_read_reg(client, MAX77888_MUIC_REG_CDETCTRL1, &val);
	dev_info(info->dev, "%s: CDETCTRL(0x%02x)\n", __func__, val);

	return count;
}

static void max77888_muic_set_adcdbset(struct max77888_muic_info *info,
				       int value)
{
	int ret;
	u8 val;
	dev_info(info->dev, "func:%s value:%x\n", __func__, value);
	if (value > 3) {
		dev_err(info->dev, "%s: invalid value(%x)\n", __func__, value);
		return;
	}

	if (!info->muic) {
		dev_err(info->dev, "%s: no muic i2c client\n", __func__);
		return;
	}

	val = value << CTRL4_ADCDBSET_SHIFT;
	dev_info(info->dev, "%s: ADCDBSET(0x%02x)\n", __func__, val);
	ret = max77888_update_reg(info->muic, MAX77888_MUIC_REG_CTRL4, val,
				  CTRL4_ADCDBSET_MASK);
	if (ret < 0)
		dev_err(info->dev, "%s: fail to update reg\n", __func__);
}

static ssize_t max77888_muic_show_adc_debounce_time(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;
	dev_info(info->dev, "func:%s buf:%s\n", __func__, buf);

	if (!info->muic)
		return sprintf(buf, "No I2C client\n");

	ret = max77888_read_reg(info->muic, MAX77888_MUIC_REG_CTRL3, &val);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	val &= CTRL4_ADCDBSET_MASK;
	val = val >> CTRL4_ADCDBSET_SHIFT;
	dev_info(info->dev, "func:%s val:%x\n", __func__, val);
	return sprintf(buf, "%x\n", val);
}

static ssize_t max77888_muic_set_adc_debounce_time(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);
	value = (value & 0x3);

	dev_info(info->dev, "%s: Do nothing\n", __func__);

	return count;
}

#if defined(REGARD_442K_AS_523K)
static ssize_t max77888_muic_show_is_factory_mode(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;
	pr_info("[%s][buf=%s]", __func__, buf);

	if (!info->muic)
		return sprintf(buf, "No I2C client\n");

	return sprintf(buf, "%d\n", is_factory_mode);
}

static ssize_t max77888_muic_set_is_factory_mode(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	pr_info("[%s][buf=%s][cable_type=%d]", __func__, buf, info->cable_type);

	if (!strncasecmp(buf, "0", 1)) {
		is_factory_mode = 0;
		if (info->cable_type ==
				CABLE_TYPE_JIG_UART_OFF_MUIC)
			max77888_muic_force_uart_switch(info->muic_data->uart_path);
	} else if ((!strncasecmp(buf, "1", 1))) {
		is_factory_mode = 1;
		if (info->cable_type ==
				CABLE_TYPE_JIG_UART_OFF_MUIC)
			max77888_muic_switch_uart_path_default();
	} else {
		pr_info("[%s] wrong value", __func__);
		return -1;
	}

	return count;
}
#endif

static ssize_t max77888_muic_set_uart_sel(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);

	pr_info("func:%s buf:%s,count:%d\n", __func__, buf, count);

	if (!strncasecmp(buf, "AP", 2)) {
		int ret = max77888_muic_set_uart_path_pass2
			(info, UART_PATH_AP);
		if (ret >= 0)
			info->muic_data->uart_path = UART_PATH_AP;
		else
			pr_err("%s: Change(AP) fail!!"
				, __func__);
	}
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	else if (!strncasecmp(buf, "CP2", 3)) {
		int ret = max77888_muic_set_uart_path_pass2
			(info, UART_PATH_CP);
		if (ret >= 0)
			info->muic_data->uart_path = UART_PATH_CP;
		else
			pr_err("%s: Change(CP2) fail!!"
						, __func__);
		if (gpio_is_valid(GPIO_UART_SEL))	{
			gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);
			info->muic_data->uart_path = UART_PATH_CP_ESC;
			pr_info("%s: CP2 %d\n",
					__func__,
					gpio_get_value(GPIO_UART_SEL));
		} else	{
			pr_err("%s: Change GPIO failed",
					__func__);
		}
	}
#endif
	else if (!strncasecmp(buf, "CP", 2)) {
		int ret = max77888_muic_set_uart_path_pass2
			(info, UART_PATH_CP);
		if (ret >= 0)
			info->muic_data->uart_path = UART_PATH_CP;
		else
			pr_err("%s: Change(CP) fail!!"
						, __func__);
#if defined(CONFIG_SWITCH_DUAL_MODEM)
		if (gpio_is_valid(GPIO_UART_SEL))	{
			gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);
			pr_info("%s: CP %d\n",
					__func__,
					gpio_get_value(GPIO_UART_SEL));
		} else	{
			pr_err("%s: Change GPIO failed",
					__func__);
		}
#endif
	}
#ifdef CONFIG_LTE_VIA_SWITCH
	else if (!strncasecmp(buf, "LTE", 3)) {
		int ret = max77888_muic_set_uart_path_pass2
			(info, UART_PATH_LTE);
		if (ret >= 0)
			info->muic_data->uart_path = UART_PATH_LTE;
		else
			dev_err(info->dev, "%s: Change(LTE) fail!!"
						, __func__);
	}
#endif
	else {
			dev_warn(info->dev, "%s: Wrong command\n"
						, __func__);
	}
	return count;
}

static ssize_t max77888_muic_show_uart_sel(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	switch (info->muic_data->uart_path) {
	case UART_PATH_AP:
		pr_info("func:%s AP\n", __func__);
		return sprintf(buf, "AP\n");
		break;
	case UART_PATH_CP:
		pr_info("func:%s CP\n", __func__);
		return sprintf(buf, "CP\n");
		break;
#ifdef CONFIG_LTE_VIA_SWITCH
	case UART_PATH_LTE:
		return sprintf(buf, "LTE\n");
		break;
#endif
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	case UART_PATH_CP_ESC:
		pr_info("func:%s CP2\n", __func__);
		return sprintf(buf, "CP2\n");
		break;
#endif
	default:
		break;
	}
	return sprintf(buf, "UNKNOWN\n");
}

#if !defined(CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK)
static ssize_t max77888_muic_show_apo_factory(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	const char *mode;

	/* true: Factory mode, false: not Factory mode */
	if (info->is_factory_start)
		mode = "FACTORY_MODE";
	else
		mode = "NOT_FACTORY_MODE";

	pr_info("%s:%s apo factory=%s\n", DEV_NAME, __func__, mode);

	return sprintf(buf, "%s\n", mode);
}

static ssize_t max77888_muic_set_apo_factory(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	const char *mode;

	pr_info("%s:%s buf:%s\n", DEV_NAME, __func__, buf);

	/* "FACTORY_START": factory mode */
	if (!strncmp(buf, "FACTORY_START", 13)) {
		info->is_factory_start = true;
		mode = "FACTORY_MODE";
	} else {
		pr_warn("%s:%s Wrong command\n", DEV_NAME, __func__);
		return count;
	}

	pr_info("%s:%s apo factory=%s\n", DEV_NAME, __func__, mode);

	return count;
}
#endif /* !CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK */

#ifdef CONFIG_LTE_VIA_SWITCH
static ssize_t max77888_muic_show_check_cpboot(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	if (cpbooting)
		return sprintf(buf, "start\n");
	else
		return sprintf(buf, "done\n");
}

static ssize_t max77888_muic_set_check_cpboot(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);

	if (!strncasecmp(buf, "start", 5)) {
		if (gpio_is_valid(GPIO_LTE_VIA_UART_SEL)) {
			dev_info(info->dev, "%s: Fix CP-UART path to LTE during CP Booting",
					__func__);
			gpio_set_value(GPIO_LTE_VIA_UART_SEL, GPIO_LEVEL_LOW);
		} else {
			dev_err(info->dev, "%s: ERR_LTE_GPIO_SET_LOW\n",
					__func__);
			return -EINVAL;
		}
		cpbooting = 1;
	} else if (!strncasecmp(buf, "done", 4)) {
		if (info->muic_data->uart_path == UART_PATH_CP) {
			if (gpio_is_valid(GPIO_LTE_VIA_UART_SEL)) {
				dev_info(info->dev, "%s: Reroute CP-UART path to VIA after CP Booting",
						__func__);
				gpio_set_value(GPIO_LTE_VIA_UART_SEL,
						GPIO_LEVEL_HIGH);
			} else {
				dev_err(info->dev, "%s: ERR_LTE_GPIO_SET_HIGH\n",
						__func__);
				return -EINVAL;
			}
		}
		cpbooting = 0;
	} else {
		dev_warn(info->dev, "%s: Wrong command : %s\n", __func__, buf);
	}

	return count;
}
#endif

static ssize_t max77888_muic_show_charger_type(struct device *dev,
					       struct device_attribute *attr,
					       char *buf)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	u8 adc, status;
	int ret;

	ret = max77888_read_reg(info->muic, MAX77888_MUIC_REG_STATUS1, &status);

	adc = status & STATUS1_ADC_MASK;

	/* SYSFS Node : chg_type
	*  SYSFS State
	*  0 : Dedicated Charger
	*  1 : Non-Dedicated Charger
	*/
	switch (adc){
	case ADC_MHL:
	case ADC_SMARTDOCK:
	case ADC_AUDIODOCK:
	case ADC_DESKDOCK:
	case ADC_CARDOCK:
	case ADC_OPEN:
		dev_info(info->dev, "%s: Dedicated Charger State\n", __func__);
		return snprintf(buf, 4, "%d\n", 0);
		break;
	case ADC_CEA936ATYPE1_CHG:
		dev_info(info->dev, "%s: Dedicated Charger State\n", __func__);
		return snprintf(buf, 4, "%d\n", 1);
		break;
	case ADC_CEA936ATYPE2_CHG:
		dev_info(info->dev, "%s: Dedicated Charger State\n", __func__);
		return snprintf(buf, 4, "%d\n", 2);
		break;
	default:
		dev_info(info->dev, "%s: Undeclared State\n", __func__);
		return snprintf(buf, 4, "%d\n", 3);
		break;
	}
}


static DEVICE_ATTR(chg_type, 0664, max77888_muic_show_charger_type, NULL);
static DEVICE_ATTR(uart_sel, 0664, max77888_muic_show_uart_sel,
		max77888_muic_set_uart_sel);
static DEVICE_ATTR(usb_state, S_IRUGO, max77888_muic_show_usb_state, NULL);
static DEVICE_ATTR(device, S_IRUGO, max77888_muic_show_device, NULL);
static DEVICE_ATTR(usb_sel, 0664,
		max77888_muic_show_manualsw, max77888_muic_set_manualsw);
static DEVICE_ATTR(adc, S_IRUGO, max77888_muic_show_adc, NULL);
static DEVICE_ATTR(audio_path, 0664,
		max77888_muic_show_audio_path, max77888_muic_set_audio_path);
static DEVICE_ATTR(otg_test, 0664,
		max77888_muic_show_otg_test, max77888_muic_set_otg_test);
static DEVICE_ATTR(adc_debounce_time, 0664,
		max77888_muic_show_adc_debounce_time,
		max77888_muic_set_adc_debounce_time);
#if !defined(CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK)
static DEVICE_ATTR(apo_factory, 0664,
		max77888_muic_show_apo_factory,
		max77888_muic_set_apo_factory);
#endif /* !CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK */
#ifdef CONFIG_LTE_VIA_SWITCH
static DEVICE_ATTR(check_cpboot, 0664,
		max77888_muic_show_check_cpboot,
		max77888_muic_set_check_cpboot);
#endif

#if defined(REGARD_442K_AS_523K)
static DEVICE_ATTR(is_factory_mode, 0664,
		max77888_muic_show_is_factory_mode,
		max77888_muic_set_is_factory_mode);
#endif

static struct attribute *max77888_muic_attributes[] = {
	&dev_attr_uart_sel.attr,
	&dev_attr_usb_state.attr,
	&dev_attr_device.attr,
	&dev_attr_usb_sel.attr,
	&dev_attr_adc.attr,
	&dev_attr_audio_path.attr,
	&dev_attr_otg_test.attr,
	&dev_attr_adc_debounce_time.attr,
#if !defined(CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK)
	&dev_attr_apo_factory.attr,
#endif /* !CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK */
#ifdef CONFIG_LTE_VIA_SWITCH
	&dev_attr_check_cpboot.attr,
#endif
#if defined(REGARD_442K_AS_523K)
	&dev_attr_is_factory_mode.attr,
#endif
	&dev_attr_chg_type.attr,
	NULL
};

static const struct attribute_group max77888_muic_group = {
	.attrs = max77888_muic_attributes,
};

static int max77888_muic_set_usb_path(struct max77888_muic_info *info, int path)
{
	struct i2c_client *client = info->muic;
	struct max77888_muic_data *mdata = info->muic_data;
	int ret;
	int gpio_val;
	u8 cntl1_val, cntl1_msk;
	int val;
	dev_info(info->dev, "func:%s path:%d\n", __func__, path);
	if (mdata->set_safeout) {
		ret = mdata->set_safeout(path);
		if (ret) {
			dev_err(info->dev, "%s: fail to set safout!\n",
				__func__);
			return ret;
		}
	}
	switch (path) {
	case AP_USB_MODE:
		dev_info(info->dev, "%s: AP_USB_MODE\n", __func__);
		gpio_val = 0;
		val = MAX77888_MUIC_CTRL1_BIN_1_001;
		cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
		break;
	case CP_USB_MODE:
		dev_info(info->dev, "%s: CP_USB_MODE\n", __func__);
		gpio_val = 1;
		if (info->max77888->pmic_rev >= MAX77888_REV_PASS2)
			val = MAX77888_MUIC_CTRL1_BIN_4_100;
		else
			val = MAX77888_MUIC_CTRL1_BIN_3_011;
		cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
		break;
	case AUDIO_MODE:
		dev_info(info->dev, "%s: AUDIO_MODE\n", __func__);
		gpio_val = 0;
		/* SL1, SR2 */
		cntl1_val = (MAX77888_MUIC_CTRL1_BIN_2_010 << COMN1SW_SHIFT)
			| (MAX77888_MUIC_CTRL1_BIN_2_010 << COMP2SW_SHIFT) |
			(0 << MICEN_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK | MICEN_MASK;
		break;
	case OPEN_USB_MODE:
		dev_info(info->dev, "%s: OPEN_USB_MODE\n", __func__);
		gpio_val = 0;
		val = MAX77888_MUIC_CTRL1_BIN_0_000;
		cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
		break;
	default:
		dev_warn(info->dev, "%s: invalid path(%d)\n", __func__, path);
		return -EINVAL;
	}

	dev_info(info->dev, "%s: Set manual path\n", __func__);
	max77888_update_reg(client, MAX77888_MUIC_REG_CTRL1, cntl1_val,
			    cntl1_msk);
	max77888_update_reg(client, MAX77888_MUIC_REG_CTRL2,
				CTRL2_CPEn1_LOWPWD0,
				CTRL2_CPEn_MASK | CTRL2_LOWPWD_MASK);

	cntl1_val = MAX77888_MUIC_CTRL1_BIN_0_000;
	max77888_read_reg(client, MAX77888_MUIC_REG_CTRL1, &cntl1_val);
	dev_info(info->dev, "%s: CNTL1(0x%02x)\n", __func__, cntl1_val);

	cntl1_val = MAX77888_MUIC_CTRL1_BIN_0_000;
	max77888_read_reg(client, MAX77888_MUIC_REG_CTRL2, &cntl1_val);
	dev_info(info->dev, "%s: CNTL2(0x%02x)\n", __func__, cntl1_val);

	sysfs_notify(&switch_dev->kobj, NULL, "usb_sel");
	return 0;
}

int max77888_muic_get_charging_type(void)
{
	return gInfo->cable_type;
}

static int max77888_muic_set_charging_type(struct max77888_muic_info *info,
					   bool force_disable)
{
	struct max77888_muic_data *mdata = info->muic_data;
	int ret = 0;

	dev_info(info->dev, "func:%s force_disable:%d\n",
		 __func__, force_disable);
	if(info->is_otg_enable == false){
		if (mdata->charger_cb) {
			if (force_disable)
				ret = mdata->charger_cb(CABLE_TYPE_NONE_MUIC);
			else
				ret = mdata->charger_cb(info->cable_type);
		}
	} else {
		dev_info(info->dev, "func:%s OTG MODE is enabled. skip charger_cb\n", __func__);
	}

	if (ret) {
		dev_err(info->dev, "%s: error from charger_cb(%d)\n", __func__,
			ret);
		return ret;
	}
	return 0;
}

static int max77888_muic_attach_usb_type(struct max77888_muic_info *info,
					 int adc)
{
	struct max77888_muic_data *mdata = info->muic_data;
	int ret, path;
	dev_info(info->dev, "func:%s adc:%x cable_type:%d\n",
		 __func__, adc, info->cable_type);
	if (info->cable_type == CABLE_TYPE_MHL_MUIC
	    || info->cable_type == CABLE_TYPE_MHL_VB_MUIC) {
		dev_warn(info->dev, "%s: mHL was attached!\n", __func__);
		return 0;
	}

	switch (adc) {
	case ADC_LANHUB:
		dev_info(info->dev, "%s:USB\n", __func__);
		path = AP_USB_MODE;
		info->is_otg_enable = false;
		break;
	case ADC_JIG_USB_OFF:
		if (info->cable_type == CABLE_TYPE_JIG_USB_OFF_MUIC) {
			dev_info(info->dev, "%s: duplicated(JIG USB OFF)\n",
				 __func__);
			return 0;
		}

		dev_info(info->dev, "%s:JIG USB BOOTOFF\n", __func__);
		info->cable_type = CABLE_TYPE_JIG_USB_OFF_MUIC;
		path = AP_USB_MODE;
		break;
	case ADC_JIG_USB_ON:
		if (info->cable_type == CABLE_TYPE_JIG_USB_ON_MUIC) {
			dev_info(info->dev, "%s: duplicated(JIG USB ON)\n",
				 __func__);
			return 0;
		}

		dev_info(info->dev, "%s:JIG USB BOOTON\n", __func__);
		info->cable_type = CABLE_TYPE_JIG_USB_ON_MUIC;
		path = AP_USB_MODE;
		break;
	case ADC_CEA936ATYPE1_CHG:
	case ADC_CEA936ATYPE2_CHG:
	case ADC_OPEN:
		if (info->cable_type == CABLE_TYPE_USB_MUIC) {
			dev_info(info->dev, "%s: duplicated(USB)\n", __func__);
			return 0;
		}

		dev_info(info->dev, "%s:USB\n", __func__);
		if(info->cable_type != CABLE_TYPE_CDP_MUIC)
			info->cable_type = CABLE_TYPE_USB_MUIC;
		path = AP_USB_MODE;
		break;
	default:
		dev_info(info->dev, "%s: Unkown cable(0x%x)\n", __func__, adc);
		return 0;
	}

	ret = max77888_muic_set_charging_type(info, false);
	if (ret) {
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		return ret;
	}

#if defined(CONFIG_SWITCH_DUAL_MODEM)
	if (mdata->sw_path == CP_USB_MODE ||
		mdata->sw_path == CP_ESC_USB_MODE) {
#else
	if (mdata->sw_path == CP_USB_MODE) {
#endif
		info->cable_type = CABLE_TYPE_USB_MUIC;
#if defined(CONFIG_MACH_J_CHN_CTC)
		dev_info(info->dev, "%s: enable GPIO_USB_BOOT_EN in attach\n", __func__);
		gpio_direction_output(GPIO_USB_BOOT_EN, 1);
#endif
#if defined(CONFIG_SWITCH_DUAL_MODEM)
		if (mdata->sw_path == CP_USB_MODE) {
			gpio_set_value(GPIO_USB_SEL, GPIO_LEVEL_LOW);
		} else {
			gpio_set_value(GPIO_USB_SEL, GPIO_LEVEL_HIGH);
		}
#endif
		max77888_muic_set_usb_path(info, CP_USB_MODE);
		return 0;
	}

	max77888_muic_set_usb_path(info, path);

	if (path == AP_USB_MODE) {
		if (mdata->usb_cb && info->is_usb_ready)
#ifdef CONFIG_USBHUB_USB3803
			/* setting usb hub in Diagnostic(hub) mode */
			usb3803_set_mode(USB_3803_MODE_HUB);
#endif				/* CONFIG_USBHUB_USB3803 */
		mdata->usb_cb(USB_CABLE_ATTACHED);
	}

	return 0;
}

static int max77888_muic_attach_dock_type(struct max77888_muic_info *info,
					  int adc, int chgtyp)
{
	struct max77888_muic_data *mdata = info->muic_data;
	int path;
	dev_info(info->dev, "func:%s adc:%x, open(%d)\n",
			__func__, adc, info->is_adc_open_prev);
	/*Workaround for unstable adc*/
	if (info->is_adc_open_prev == false)
		return 0;

	switch (adc) {
	case ADC_MMDOCK:
		if (info->cable_type == CABLE_TYPE_MMDOCK_MUIC) {
			dev_info(info->dev, "%s: duplicated(MMDOCK)\n",
					__func__);
			return 0;
		}
		dev_info(info->dev, "%s: mm dock attached\n", __func__);
		info->cable_type = CABLE_TYPE_MMDOCK_MUIC;
		path = AP_USB_MODE;

		if (mdata->mhl_cb)
			mdata->mhl_cb(MAX77888_MUIC_ATTACHED);

		if (mdata->usb_cb)
			mdata->usb_cb(USB_POWERED_HOST_ATTACHED);

		max77888_muic_set_charging_type(info, false);

		if (mdata->dock_cb)
			mdata->dock_cb(MAX77888_MUIC_DOCK_SMARTDOCK);
		break;
	case ADC_LANHUB:
		/* Lanhub */
		if (info->cable_type == CABLE_TYPE_LANHUB_MUIC) {
			dev_info(info->dev, "%s: dulpicated(Lanhub)\n",
				__func__);
			return 0;
		}
		dev_info(info->dev, "%s:Lanhub attached\n", __func__);
		info->cable_type = CABLE_TYPE_LANHUB_MUIC;
		info->is_otg_enable = false;
		path = AP_USB_MODE;

		if (mdata->usb_cb)
			mdata->usb_cb(USB_LANHUB_ATTACHED);
		break;
	case ADC_DESKDOCK:
		/* Desk Dock */
		if (info->cable_type == CABLE_TYPE_DESKDOCK_MUIC) {
			dev_info(info->dev, "%s: duplicated(DeskDock)\n",
				 __func__);
			return 0;
		}
		dev_info(info->dev, "%s:DeskDock\n", __func__);
		info->cable_type = CABLE_TYPE_DESKDOCK_MUIC;
		path = AUDIO_MODE;

		if (mdata->dock_cb)
			mdata->dock_cb(MAX77888_MUIC_DOCK_DESKDOCK);
		break;
	case ADC_CARDOCK:
		/* Car Dock */
		if (info->cable_type == CABLE_TYPE_CARDOCK_MUIC) {
			dev_info(info->dev, "%s: duplicated(CarDock)\n",
				 __func__);
			return 0;
		}
		dev_info(info->dev, "%s:CarDock\n", __func__);
		info->cable_type = CABLE_TYPE_CARDOCK_MUIC;
		path = AUDIO_MODE;

		if (mdata->dock_cb)
			mdata->dock_cb(MAX77888_MUIC_DOCK_CARDOCK);
		break;
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK)
	case ADC_SMARTDOCK:
		if (info->cable_type == CABLE_TYPE_SMARTDOCK_MUIC
		 || info->cable_type == CABLE_TYPE_SMARTDOCK_TA_MUIC
		 || info->cable_type == CABLE_TYPE_SMARTDOCK_USB_MUIC) {
			dev_info(info->dev, "%s: duplicated(SmartDock)\n",
				__func__);
			return 0;
		}
		dev_info(info->dev, "%s:SmartDock\n", __func__);

		if (chgtyp == CHGTYP_DEDICATED_CHGR) {
			info->cable_type = CABLE_TYPE_SMARTDOCK_TA_MUIC;
		} else if (chgtyp == CHGTYP_USB) {
			info->cable_type = CABLE_TYPE_SMARTDOCK_USB_MUIC;
		} else	{
			info->cable_type = CABLE_TYPE_SMARTDOCK_MUIC;
		}
		
		if (info->is_usb_ready) {
			pr_info("%s:%s usb is ready, D+,D- line(AP_USB)\n",
				DEV_NAME, __func__);
			path = AP_USB_MODE;
		} else {
			pr_info("%s:%s usb not ready yet, D+,D- line(Open)\n",
				DEV_NAME, __func__);
			path = OPEN_USB_MODE;
		}

		max77888_muic_set_charging_type(info, false);
		msleep(40);
#ifdef CONFIG_EXTCON
		if (info->edev && info->is_mhl_ready)
			extcon_set_cable_state(info->edev, "MHL", true);
#else
		if (mdata->mhl_cb && info->is_mhl_ready)
			mdata->mhl_cb(MAX77888_MUIC_ATTACHED);
#endif
		if (mdata->dock_cb)
			mdata->dock_cb(MAX77888_MUIC_DOCK_SMARTDOCK);
		break;
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK */
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK)
	case ADC_AUDIODOCK:
		if (info->cable_type == CABLE_TYPE_AUDIODOCK_MUIC) {
			pr_info("%s:%s duplicated(AudioDock)\n", DEV_NAME,
					__func__);
			return 0;
		}
		pr_info("%s:%s AudioDock\n", DEV_NAME, __func__);
		info->cable_type = CABLE_TYPE_AUDIODOCK_MUIC;

		if (info->is_usb_ready) {
			pr_info("%s:%s usb is ready, D+,D- line(AP_USB)\n",
				DEV_NAME, __func__);
			path = AP_USB_MODE;
		} else {
			pr_info("%s:%s usb not ready yet, D+,D- line(Open)\n",
				DEV_NAME, __func__);
			path = OPEN_USB_MODE;
		}

		max77888_muic_set_charging_type(info, false);

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_POWERED_HOST_ATTACHED);

		if (mdata->dock_cb)
			mdata->dock_cb(MAX77888_MUIC_DOCK_AUDIODOCK);
		break;
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK */
	default:
		dev_info(info->dev, "%s: should not reach here(0x%x)\n",
			 __func__, adc);
		return 0;
	}

	max77888_muic_set_usb_path(info, path);

	return 0;
}

static void max77888_muic_attach_mhl(struct max77888_muic_info *info, u8 chgtyp)
{
	struct max77888_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "func:%s chgtyp:%x\n", __func__, chgtyp);

	if (info->cable_type == CABLE_TYPE_USB_MUIC) {
		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_CABLE_DETACHED);

		max77888_muic_set_charging_type(info, true);
	}
	info->cable_type = CABLE_TYPE_MHL_MUIC;

	if (mdata->mhl_cb && info->is_mhl_ready)
		mdata->mhl_cb(MAX77888_MUIC_ATTACHED);

	if (chgtyp == CHGTYP_USB) {
		info->cable_type = CABLE_TYPE_MHL_VB_MUIC;
		max77888_muic_set_charging_type(info, false);
	}
}

static void max77888_muic_handle_jig_uart(struct max77888_muic_info *info,
					  u8 vbvolt)
{
	struct max77888_muic_data *mdata = info->muic_data;
	enum cable_type_muic prev_ct = info->cable_type;
	bool is_otgtest = false;
	u8 cntl1_val, cntl1_msk;
	u8 val = MAX77888_MUIC_CTRL1_BIN_3_011;
	int ret = 0;

	dev_info(info->dev, "func:%s vbvolt:%x cable_type:%d\n",
		 __func__, vbvolt, info->cable_type);
	dev_info(info->dev, "%s: JIG UART/BOOTOFF(0x%x)\n", __func__, vbvolt);

	cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
	ret = max77888_update_reg(info->muic, MAX77888_MUIC_REG_CTRL1,
			cntl1_val, cntl1_msk);

	dev_info(info->dev, "%s: CNTL1(0x%02x) ret: %d\n",
			__func__, cntl1_val, ret);

	max77888_update_reg(info->muic, MAX77888_MUIC_REG_CTRL2,
				CTRL2_CPEn1_LOWPWD0,
				CTRL2_CPEn_MASK | CTRL2_LOWPWD_MASK);

	if (mdata->host_notify_cb) {
		if (mdata->host_notify_cb(1) == NOTIFY_TEST_MODE) {
			is_otgtest = true;
			dev_info(info->dev, "%s: OTG TEST\n", __func__);
		}
	}

	if (vbvolt & STATUS2_VBVOLT_MASK) {
		if (is_otgtest == false)
			max77888_update_reg(info->muic, MAX77888_MUIC_REG_CTRL2,
				(0 << CTRL2_ACCDET_SHIFT), CTRL2_ACCDET_MASK);

		info->cable_type = CABLE_TYPE_JIG_UART_OFF_VB_MUIC;
		max77888_muic_set_charging_type(info, is_otgtest);

	} else {
		info->cable_type = CABLE_TYPE_JIG_UART_OFF_MUIC;
//		if (mdata->uart_path == UART_PATH_CP && mdata->jig_uart_cb)
//			mdata->jig_uart_cb(UART_PATH_CP);
		if (prev_ct == CABLE_TYPE_JIG_UART_OFF_VB_MUIC) {
			max77888_muic_set_charging_type(info, false);

			if (mdata->host_notify_cb)
				mdata->host_notify_cb(0);
		}
	}
}

void max77888_otg_control(struct max77888_muic_info *info, int enable)
{
	u8 int_mask, int_mask2, ctrl3, chg_cnfg_00;
	int jig_state;
	pr_info("%s: enable(%d)\n", __func__, enable);
	
	/* Get jig state and set CTRL3 reg */
	jig_state = max77888_get_jig_state();
	
	if (enable) {
		/* Set CTRL3 Reg = 0x0E */
		if(jig_state == 1){
			max77888_read_reg(info->max77888->muic,
				MAX77888_MUIC_REG_CTRL3, &ctrl3);
			ctrl3 |= 0x02;
			max77888_write_reg(info->max77888->muic,
				MAX77888_MUIC_REG_CTRL3, ctrl3);
		}

		/* disable charger interrupt */
		max77888_read_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_INT_MASK, &int_mask);
		chg_int_state = int_mask;
		int_mask |= (1 << 4);	/* disable chgin intr */
		int_mask |= (1 << 6);	/* disable chg */
		int_mask &= ~(1 << 0);	/* enable byp intr */
		max77888_write_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_INT_MASK, int_mask);

		/* VB voltage interrupt Mask */
		max77888_read_reg(info->max77888->muic,
			MAX77888_MUIC_REG_INTMASK2, &int_mask2);
		int_mask2 &= ~(1 << 4);
		max77888_write_reg(info->max77888->muic,
			MAX77888_MUIC_REG_INTMASK2, int_mask2);

		/* OTG on, boost on, DIS_MUIC_CTRL=1 */
		max77888_read_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_CNFG_00, &chg_cnfg_00);
		chg_cnfg_00 &= ~(CHG_CNFG_00_CHG_MASK
				| CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BUCK_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		chg_cnfg_00 |= (CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		max77888_write_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_CNFG_00, chg_cnfg_00);

		/* Update CHG_CNFG_11 to 0x50(5V) */
		max77888_write_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_CNFG_11, 0x50);

		/* check flag otg mode */
		info->is_otg_enable = true;
	} else {
		/* Set CTRL3 Reg to 0x01*/
		if (jig_state == 1) {
			max77888_read_reg(info->max77888->muic,
				MAX77888_MUIC_REG_CTRL3, &ctrl3);
			ctrl3 &= 0xFC;
			ctrl3 |= 0x01;
			max77888_write_reg(info->max77888->muic,
				MAX77888_MUIC_REG_CTRL3, ctrl3);
		}

		/* OTG off, boost off, (buck on),
		   DIS_MUIC_CTRL = 0 unless CHG_ENA = 1 */
		max77888_read_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_CNFG_00, &chg_cnfg_00);
		chg_cnfg_00 &= ~(CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		chg_cnfg_00 |= CHG_CNFG_00_BUCK_MASK;
		max77888_write_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_CNFG_00, chg_cnfg_00);

		/* Update CHG_CNFG_11 to 0x00(3V) */
		max77888_write_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_CNFG_11, 0x00);

		mdelay(50);

		/* VB voltage interrupt Unmask */
		max77888_read_reg(info->max77888->muic,
			MAX77888_MUIC_REG_INTMASK2, &int_mask2);
		int_mask2 |= (1 << 4);
		max77888_write_reg(info->max77888->muic,
			MAX77888_MUIC_REG_INTMASK2, int_mask2);

		/* enable charger interrupt */
		max77888_write_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_INT_MASK, chg_int_state);
		max77888_read_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_INT_MASK, &int_mask);

		/* uncheck flag otg mode */
		info->is_otg_enable = false;
	}

	pr_info("%s: INT_MASK(0x%x), INT_MASK2(0x%x), CHG_CNFG_00(0x%x)\n",
				__func__, int_mask, int_mask2, chg_cnfg_00);
}

void max77888_powered_otg_control(struct max77888_muic_info *info, int enable)
{
	u8 chg_cnfg_00;
	pr_info("%s: powered otg(%d)\n", __func__, enable);

	/*
	 * if powered otg state, disable charger's otg and boost.
	 * don't care about buck, charger state
	 */

	max77888_read_reg(info->max77888->i2c,
		MAX77888_CHG_REG_CHG_CNFG_00, &chg_cnfg_00);
	pr_info("%s: CHG_CNFG_00(0x%x)\n", __func__, chg_cnfg_00);

	chg_cnfg_00 &= ~(CHG_CNFG_00_OTG_MASK
			| CHG_CNFG_00_BOOST_MASK
			| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);

	max77888_write_reg(info->max77888->i2c,
		MAX77888_CHG_REG_CHG_CNFG_00, chg_cnfg_00);

	if (enable) {
		/* Update CHG_CNFG_11 to 0x50(5V) */
		max77888_write_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_CNFG_11, 0x50);
	} else {
		/* Update CHG_CNFG_11 to 0x00(3V) */
		max77888_write_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_CNFG_11, 0x00);
	}
}

/* use in mach for otg */
int muic_otg_control(int enable)
{
	pr_debug("%s: enable(%d)\n", __func__, enable);

	max77888_otg_control(gInfo, enable);
	return 0;
}

/* use in mach for powered-otg */
void powered_otg_control(int enable)
{
	pr_debug("%s: enable(%d)\n", __func__, enable);

	max77888_powered_otg_control(gInfo, enable);
}

#if defined(CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK)
#if 0 // unused
static void max77888_muic_set_cddelay(struct max77888_muic_info *info)
{
	u8 cdetctrl1;
	int ret = 0;

	ret = max77888_read_reg(info->max77888->muic,
		MAX77888_MUIC_REG_CDETCTRL1, &cdetctrl1);

	pr_info("%s:%s read CDETCTRL1=0x%x, ret=%d\n", DEV_NAME, __func__,
			cdetctrl1, ret);

	if ((cdetctrl1 & 0x10) == 0x10) {
		pr_info("%s:%s CDDelay already setted, return\n", DEV_NAME,
				__func__);
		return;
	}

	cdetctrl1 |= 0x10;

	ret = max77888_write_reg(info->max77888->muic,
		MAX77888_MUIC_REG_CDETCTRL1, cdetctrl1);

	pr_info("%s:%s write CDETCTRL1=0x%x, ret=%d\n", DEV_NAME,
			__func__, cdetctrl1, ret);
}
#endif

static void max77888_muic_clear_cddelay(struct max77888_muic_info *info)
{
	u8 cdetctrl1;
	int ret = 0;

	ret = max77888_read_reg(info->max77888->muic,
		MAX77888_MUIC_REG_CDETCTRL1, &cdetctrl1);

	pr_info("%s:%s read CDETCTRL1=0x%x, ret=%d\n", DEV_NAME, __func__,
			cdetctrl1, ret);

	if ((cdetctrl1 & 0x10) == 0x0) {
		pr_info("%s:%s CDDelay already cleared, return\n", DEV_NAME,
				__func__);
		return;
	}

	cdetctrl1 &= ~(0x10);

	ret = max77888_write_reg(info->max77888->muic,
		MAX77888_MUIC_REG_CDETCTRL1, cdetctrl1);

	pr_info("%s:%s write CDETCTRL1=0x%x, ret=%d\n", DEV_NAME,
			__func__, cdetctrl1, ret);
}

static void max77888_muic_detach_smart_dock(struct max77888_muic_info *info)
{
	struct max77888_muic_data *mdata = info->muic_data;
	enum cable_type_muic tmp_cable_type = info->cable_type;

	pr_info("%s:%s\n", DEV_NAME, __func__);

	if (info->cable_type != CABLE_TYPE_SMARTDOCK_MUIC &&
			info->cable_type != CABLE_TYPE_SMARTDOCK_TA_MUIC &&
			info->cable_type != CABLE_TYPE_SMARTDOCK_USB_MUIC) {
		pr_info("%s:%s cable_type is not SMARTDOCK\n", DEV_NAME,
				__func__);
		return;
	}

	if (mdata->dock_cb)
		mdata->dock_cb(MAX77888_MUIC_DOCK_DETACHED);

	info->cable_type = CABLE_TYPE_NONE_MUIC;

	max77888_muic_set_charging_type(info, false);
#ifdef CONFIG_EXTCON
	if (info->edev && info->is_mhl_ready)
		extcon_set_cable_state(info->edev, "MHL", false);
#else
	if (mdata->mhl_cb && info->is_mhl_ready)
		mdata->mhl_cb(MAX77888_MUIC_DETACHED);
#endif

	switch (tmp_cable_type) {
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
		pr_info("%s:%s SMARTDOCK+TA\n", DEV_NAME, __func__);

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_POWERED_HOST_DETACHED);
		break;
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		pr_info("%s:%s SMARTDOCK+USB\n", DEV_NAME, __func__);

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_CABLE_DETACHED);
		break;
	case CABLE_TYPE_SMARTDOCK_MUIC:
		/* clear CDDelay 500ms */
		max77888_muic_clear_cddelay(info);
		pr_info("%s:%s SMARTDOCK\n", DEV_NAME, __func__);
		break;
	default:
		pr_warn("%s:%s should not reach here!\n", DEV_NAME,
				__func__);
		return;
		break;
	}
}

static void max77888_muic_attach_smart_dock(struct max77888_muic_info *info,
						u8 adc, u8 vbvolt, u8 chgtyp)
{
	struct max77888_muic_data *mdata = info->muic_data;

	switch (info->cable_type) {
	case CABLE_TYPE_SMARTDOCK_MUIC:
		if (chgtyp == CHGTYP_DEDICATED_CHGR) {
			/* clear CDDelay 500ms */
			max77888_muic_clear_cddelay(info);
			pr_info("%s:%s SMART_DOCK+TA=OTG Enable\n", DEV_NAME,
					__func__);

			if (mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_POWERED_HOST_ATTACHED);

			info->cable_type = CABLE_TYPE_SMARTDOCK_TA_MUIC;
		} else if (chgtyp == CHGTYP_USB) {
			/* clear CDDelay 500ms */
			max77888_muic_clear_cddelay(info);
			pr_info("%s:%s SMART_DOCK+USB=USB Enable\n", DEV_NAME,
					__func__);

			if (mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_CABLE_ATTACHED);

			info->cable_type = CABLE_TYPE_SMARTDOCK_USB_MUIC;
		} else
			pr_info("%s:%s SMART_DOCK + [%d] = ?\n", DEV_NAME,
					__func__, chgtyp);
		break;
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
		if (chgtyp == CHGTYP_DEDICATED_CHGR)
			pr_info("%s:%s Duplicated(SMARTDOCK+TA)\n", DEV_NAME,
					__func__);
		else if (vbvolt)
			pr_info("%s:%s SMART_DOCK + TA -> chgtyp:%x\n",
					DEV_NAME, __func__, chgtyp);
		else
			max77888_muic_detach_smart_dock(info);
		break;
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		if (chgtyp == CHGTYP_USB)
			pr_info("%s:%s Duplicated(SMARTDOCK+USB)\n", DEV_NAME,
					__func__);
		else if (vbvolt)
			pr_info("%s:%s SMART_DOCK + USB -> chgtyp:%x\n",
					DEV_NAME, __func__, chgtyp);
		else
			max77888_muic_detach_smart_dock(info);
		break;
	default:
		if (vbvolt) {
			pr_info("%s:%s SMART_DOCK+vbvolt=chgdetrun\n",
					DEV_NAME, __func__);

			if (chgtyp == 0) {
				pr_info("%s:%s SMART_DOCK + [%d] = ?? end\n", DEV_NAME,
						__func__, chgtyp);
				return;
			}

			max77888_muic_attach_dock_type(info, adc, chgtyp);

			if (chgtyp == CHGTYP_DEDICATED_CHGR) {
				/* clear CDDelay 500ms */
				max77888_muic_clear_cddelay(info);
				pr_info("%s:%s SMART_DOCK+TA=OTG Enable\n", DEV_NAME,
						__func__);

				if (mdata->usb_cb && info->is_usb_ready)
					mdata->usb_cb(USB_POWERED_HOST_ATTACHED);
			} else if (chgtyp == CHGTYP_USB) {
				/* clear CDDelay 500ms */
				max77888_muic_clear_cddelay(info);
				pr_info("%s:%s SMART_DOCK+USB=USB Enable\n", DEV_NAME,
						__func__);

				if (mdata->usb_cb && info->is_usb_ready)
					mdata->usb_cb(USB_CABLE_ATTACHED);
			}		
		} else {
			/* set CDDelay 500ms */
			/*max77888_muic_set_cddelay(info);*/
			dev_warn(info->dev, "no vbus in SAMRTDOCK - no delay set\n");
		}
		break;
	}
}
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK */

#if defined(CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK)
static void max77888_muic_detach_audio_dock(struct max77888_muic_info *info)
{
	struct max77888_muic_data *mdata = info->muic_data;

	pr_info("%s:%s AUDIODOCK\n", DEV_NAME, __func__);

	info->cable_type = CABLE_TYPE_NONE_MUIC;

	max77888_muic_set_charging_type(info, false);

	if (mdata->dock_cb)
		mdata->dock_cb(MAX77888_MUIC_DOCK_DETACHED);

	if (mdata->usb_cb && info->is_usb_ready)
		mdata->usb_cb(USB_POWERED_HOST_DETACHED);
}
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK */

static int max77888_muic_handle_attach(struct max77888_muic_info *info,
				       u8 status1, u8 status2, int irq)
{
	struct max77888_muic_data *mdata = info->muic_data;
	u8 adc, vbvolt, chgtyp, chgdetrun, adc1k, dxovp;
	int ret = 0;

	adc = status1 & STATUS1_ADC_MASK;
	adc1k = status1 & STATUS1_ADC1K_MASK;
	chgtyp = status2 & STATUS2_CHGTYP_MASK;
	vbvolt = status2 & STATUS2_VBVOLT_MASK;
	chgdetrun = status2 & STATUS2_CHGDETRUN_MASK;
	dxovp = status2 & STATUS2_DXOVP_MASK;

	dev_info(info->dev, "func:%s st1:%x st2:%x cable_type:%d\n",
		 __func__, status1, status2, info->cable_type);

	switch (info->cable_type) {
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
		/* Workaround for Factory mode.
		 * Abandon adc interrupt of approximately +-100K range
		 * if previous cable status was JIG UART BOOT OFF.
		 */
		if (adc == (ADC_JIG_UART_OFF + 1) ||
				adc == (ADC_JIG_UART_OFF - 1)) {
			/* Workaround for factory mode in MUIC PASS2
			* In uart path cp, adc is unstable state
			* MUIC PASS2 turn to AP_UART mode automatically
			* So, in this state set correct path manually.
			* !! NEEDED ONLY IF PMIC PASS2 !!
			*/
			if (info->muic_data->uart_path == UART_PATH_CP
			&& info->max77888->pmic_rev >= MAX77888_REV_PASS2)
				max77888_muic_handle_jig_uart(info, vbvolt);

#if !defined(CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK)
			if (info->is_factory_start &&
					(adc == ADC_JIG_UART_ON)) {
				pr_info("%s:%s factory start, keep attach\n",
						DEV_NAME, __func__);
				break;
			}
#endif /* !CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK */
			dev_warn(info->dev, "%s: abandon ADC\n", __func__);
			return 0;
		}

		if (adc != ADC_JIG_UART_OFF) {
			dev_warn(info->dev, "%s: assume jig uart off detach\n",
					__func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;
		}
		break;
	case CABLE_TYPE_LANHUB_MUIC:
		if (adc != ADC_LANHUB) {
			dev_warn(info->dev, "%s: assume lanhub detach\n",
					__func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			max77888_muic_set_charging_type(info, false);
			info->is_adc_open_prev = false;
			if (mdata->usb_cb)
				mdata->usb_cb(MAX77888_MUIC_DETACHED);
		}
		break;
	case CABLE_TYPE_DESKDOCK_MUIC:
		if (adc != ADC_DESKDOCK) {
			dev_warn(info->dev, "%s: assume deskdock detach\n",
					__func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			max77888_muic_set_charging_type(info, false);
			info->is_adc_open_prev = false;
			if (mdata->dock_cb)
				mdata->dock_cb(MAX77888_MUIC_DOCK_DETACHED);
		}
		break;
	case CABLE_TYPE_CARDOCK_MUIC:
		if (adc != ADC_CARDOCK) {
			dev_warn(info->dev, "%s: assume cardock detach\n",
					__func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			max77888_muic_set_charging_type(info, false);
			info->is_adc_open_prev = false;
			if (mdata->dock_cb)
				mdata->dock_cb(MAX77888_MUIC_DOCK_DETACHED);
		}
		break;
#if !defined(CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK)
	case CABLE_TYPE_JIG_UART_ON_MUIC:
		if ((adc != ADC_JIG_UART_ON) &&
			info->is_factory_start) {
			pr_warn("%s:%s assume jig uart on detach\n",
					DEV_NAME, __func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			if (mdata->dock_cb)
				mdata->dock_cb(MAX77888_MUIC_DOCK_DETACHED);
		}
		break;
#endif /* !CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK */
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK)
	case CABLE_TYPE_SMARTDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		if (adc != ADC_SMARTDOCK) {
			dev_warn(info->dev, "%s: assume smartdock detach\n",
					__func__);

			max77888_muic_detach_smart_dock(info);

			info->is_adc_open_prev = false;
		}
		break;
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK */
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK)
	case CABLE_TYPE_AUDIODOCK_MUIC:
		if ((adc != ADC_AUDIODOCK) || (!vbvolt)) {
			dev_warn(info->dev, "%s: assume audiodock detach\n",
					__func__);

			max77888_muic_detach_audio_dock(info);
		}
		break;
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK */
	case CABLE_TYPE_MMDOCK_MUIC:
		if (adc != ADC_MMDOCK) {
			dev_warn(info->dev, "%s: assume mm dock detach\n",
					__func__);

			info->cable_type = CABLE_TYPE_NONE_MUIC;
			ret = max77888_muic_set_charging_type(info, true);
			if (ret)
				pr_err("%s fail to set chg type\n", __func__);

			if (mdata->usb_cb)
				mdata->usb_cb(USB_POWERED_HOST_DETACHED);

			if (mdata->mhl_cb)
				mdata->mhl_cb(MAX77888_MUIC_DETACHED);

			if (mdata->dock_cb)
				mdata->dock_cb(MAX77888_MUIC_DOCK_DETACHED);

		}
		break;
	default:
		break;
	}

	/* 1Kohm ID regiter detection (mHL)
	 */
	if (adc1k) {
		if (irq == info->irq_adc
			|| irq == info->irq_chgtype
			|| irq == info->irq_vbvolt) {
			dev_warn(info->dev,
				 "%s: Ignore irq:%d at MHL detection\n",
				 __func__, irq);
			if (vbvolt) {
				dev_info(info->dev, "%s: call charger_cb(%d)"
					, __func__, vbvolt);
				max77888_muic_set_charging_type(info, false);
			} else {
				dev_info(info->dev, "%s: call charger_cb(%d)"
					, __func__, vbvolt);
				max77888_muic_set_charging_type(info, true);
			}
			return 0;
		}
		max77888_muic_attach_mhl(info, chgtyp);
		return 0;
	}

	switch (adc) {
	case ADC_GND:
		if (chgtyp == CHGTYP_NO_VOLTAGE) {
			if (info->cable_type == CABLE_TYPE_OTG_MUIC) {
				dev_info(info->dev,
					 "%s: duplicated(OTG)\n", __func__);
				break;
			}

			info->cable_type = CABLE_TYPE_OTG_MUIC;
			max77888_muic_set_usb_path(info, AP_USB_MODE);
			msleep(40);
			if (mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_OTGHOST_ATTACHED);
		} else if (chgtyp == CHGTYP_USB ||
			   chgtyp == CHGTYP_DOWNSTREAM_PORT ||
			   chgtyp == CHGTYP_DEDICATED_CHGR ||
			   chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A) {
			dev_info(info->dev, "%s: OTG charging pump\n",
				 __func__);
			ret = max77888_muic_set_charging_type(info, false);
		}
		break;
	case ADC_CHARGING_CABLE:
		info->cable_type = CABLE_TYPE_CHARGING_CABLE_MUIC;
		max77888_muic_set_charging_type(info, false);
		break;
	case ADC_LANHUB:
		max77888_muic_attach_dock_type(info, adc, chgtyp);
		if(chgtyp == CHGTYP_USB ||
			chgtyp == CHGTYP_DOWNSTREAM_PORT ||
			chgtyp == CHGTYP_DEDICATED_CHGR ||
			chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A)
			ret = max77888_muic_set_charging_type(info, false);
		else if (chgtyp == CHGTYP_NO_VOLTAGE && !chgdetrun)
			ret = max77888_muic_set_charging_type(info, !vbvolt);
		break;	
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK)
	case ADC_SMARTDOCK:
		max77888_muic_attach_smart_dock(info, adc, vbvolt, chgtyp);
		break;
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK */
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK)
	case ADC_AUDIODOCK:
		if (!!vbvolt)
			max77888_muic_attach_dock_type(info, adc, chgtyp);
		break;
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK */
	case ADC_MMDOCK:
		if (!!vbvolt)
			max77888_muic_attach_dock_type(info, adc, chgtyp);
		break;
	case ADC_JIG_UART_OFF:
		max77888_muic_handle_jig_uart(info, vbvolt);
		mdata->jig_state(true);
		break;
	case ADC_JIG_USB_ON:
		if (vbvolt & STATUS2_VBVOLT_MASK) {
			dev_info(info->dev, "%s: SKIP_JIG_USB\n", __func__);
			ret = max77888_muic_attach_usb_type(info, adc);
		}
		mdata->jig_state(true);
		break;
	case ADC_DESKDOCK:
		max77888_muic_attach_dock_type(info, adc, chgtyp);
		if (chgtyp == CHGTYP_USB ||
			chgtyp == CHGTYP_DOWNSTREAM_PORT ||
			chgtyp == CHGTYP_DEDICATED_CHGR ||
			chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A)
			ret = max77888_muic_set_charging_type(info, false);
		else if (chgtyp == CHGTYP_NO_VOLTAGE && !chgdetrun)
			ret = max77888_muic_set_charging_type(info, !vbvolt);
			/* For MAX77888 IC doesn`t occur chgtyp IRQ
			* because of audio noise prevention.
			* So, If below condition is set,
			* we do charging at CARDOCK.
			*/
		break;
	/* ADC_CARDOCK == ADC_JIG_UART_ON */
	case ADC_CARDOCK:
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK)
		max77888_muic_attach_dock_type(info, adc, chgtyp);
		if (chgtyp == CHGTYP_USB ||
			chgtyp == CHGTYP_DOWNSTREAM_PORT ||
			chgtyp == CHGTYP_DEDICATED_CHGR ||
			chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A)
			ret = max77888_muic_set_charging_type(info, false);
		else if (chgtyp == CHGTYP_NO_VOLTAGE && !chgdetrun)
			ret = max77888_muic_set_charging_type(info, !vbvolt);
			/* For MAX77888 IC doesn`t occur chgtyp IRQ
			* because of audio noise prevention.
			* So, If below condition is set,
			* we do charging at CARDOCK.
			*/
#else
		/* because of change FACTORY CPOriented to APOriented,
		 * at manufacture need AP wake-up method. write apo_factory
		 * FACTORY_START is set is_factory_start true.
		 */
		if (info->is_factory_start) {
			if (info->cable_type == CABLE_TYPE_JIG_UART_ON_MUIC) {
				pr_info("%s:%s duplicated(JIG_UART_ON)\n",
					DEV_NAME, __func__);
				return 0;
			}
			pr_info("%s:%s JIG_UART_ON\n", DEV_NAME, __func__);
			info->cable_type = CABLE_TYPE_JIG_UART_ON_MUIC;

			if (mdata->dock_cb)
				mdata->dock_cb(MAX77888_MUIC_DOCK_DESKDOCK);

			return 0;
		}

		if(info->muic_data->charger_cb) {
			if (chgtyp == CHGTYP_USB ||
				chgtyp == CHGTYP_DOWNSTREAM_PORT) {
				info->cable_type = CABLE_TYPE_USB_MUIC;
				ret = max77888_muic_set_charging_type(info, false);
			}
			else if (chgtyp == CHGTYP_DEDICATED_CHGR ||
				chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A)	{
				info->cable_type = CABLE_TYPE_TA_MUIC;
				ret = max77888_muic_set_charging_type(info, false);
			}
		}
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK */
		break;
	case ADC_CEA936ATYPE2_CHG:
#if defined(REGARD_442K_AS_523K)
		pr_info("[%s] is_factory_mode=%d\n", __func__, is_factory_mode);
		if (is_factory_mode==1)	{
			info->cable_type = CABLE_TYPE_JIG_UART_OFF_MUIC;
			max77888_muic_switch_uart_path_default();
			break;
		}
#endif
	case ADC_CEA936ATYPE1_CHG:
	case ADC_OPEN:
		switch (chgtyp) {
		case CHGTYP_USB:
		case CHGTYP_DOWNSTREAM_PORT:
			if (adc == ADC_CEA936ATYPE1_CHG /* for USA L USB cable*/
				|| adc == ADC_CEA936ATYPE2_CHG)
			{
				ret = max77888_muic_attach_usb_type(info, ADC_OPEN);
				break;
			}
			if (info->cable_type == CABLE_TYPE_MHL_MUIC) {
				dev_info(info->dev, "%s: MHL(charging)\n",
					 __func__);
				info->cable_type = CABLE_TYPE_MHL_VB_MUIC;
				ret = max77888_muic_set_charging_type(info,
								      false);
				return ret;
			}
			if (chgtyp == CHGTYP_DOWNSTREAM_PORT) {
				dev_info(info->dev, "%s: CDP(charging)\n",
					 __func__);
				info->cable_type = CABLE_TYPE_CDP_MUIC;
			}
#ifdef CONFIG_EXTCON
			if (info->edev)
				extcon_set_cable_state(info->edev,
					"USB", true);
#endif
			ret = max77888_muic_attach_usb_type(info, adc);
			break;
		case CHGTYP_DEDICATED_CHGR:
		case CHGTYP_500MA:
		case CHGTYP_1A:
			dev_info(info->dev, "%s:TA\n", __func__);
			info->cable_type = CABLE_TYPE_TA_MUIC;
#ifdef CONFIG_EXTCON
			if (info->edev)
				extcon_set_cable_state(info->edev,
					"TA", true);
#endif
#ifdef CONFIG_USBHUB_USB3803
			/* setting usb hub in default mode (standby) */
			usb3803_set_mode(USB_3803_MODE_STANDBY);
#endif				/* CONFIG_USBHUB_USB3803 */
			ret = max77888_muic_set_charging_type(info, false);
			if (ret)
				info->cable_type = CABLE_TYPE_NONE_MUIC;
			break;
		default:
			if (dxovp) {
				dev_info(info->dev, "%s:TA(DXOVP)\n", __func__);
				info->cable_type = CABLE_TYPE_TA_MUIC;
				ret = max77888_muic_set_charging_type(info,
							false);
				if (ret)
					info->cable_type = CABLE_TYPE_NONE_MUIC;
			}
			break;
		}
		break;
	case ADC_INCOMPATIBLE:
#ifdef CONFIG_EXTCON
			if (info->edev)
				extcon_set_cable_state(info->edev,
					"Incompatible-TA", true);
#endif
		if (vbvolt) {
			info->cable_type = CABLE_TYPE_INCOMPATIBLE_MUIC;
			ret = max77888_muic_set_charging_type(info, !vbvolt);
		}
		break;
	default:
		if (vbvolt) {
			dev_warn(info->dev, "%s: unsupported adc=0x%x\n", __func__, adc);
			info->cable_type = CABLE_TYPE_TA_MUIC;
			ret = max77888_muic_set_charging_type(info, !vbvolt);
			if (ret)
				info->cable_type = CABLE_TYPE_NONE_MUIC;
		} else {
			dev_warn(info->dev, "%s: unsupported adc=0x%x\n", __func__,
				 adc);
		}
		break;
	}
	return ret;
}

static int max77888_muic_handle_detach(struct max77888_muic_info *info, int irq)
{
	struct i2c_client *client = info->muic;
	struct max77888_muic_data *mdata = info->muic_data;
	enum cable_type_muic prev_ct = CABLE_TYPE_NONE_MUIC;
	u8 cntl2_val;
	int ret = 0;
	dev_info(info->dev, "func:%s\n", __func__);

	info->is_adc_open_prev = true;
	/* Workaround: irq doesn't occur after detaching mHL cable */
	max77888_write_reg(client, MAX77888_MUIC_REG_CTRL1,
				MAX77888_MUIC_CTRL1_BIN_0_000);

	/* Enable Factory Accessory Detection State Machine */
	max77888_update_reg(client, MAX77888_MUIC_REG_CTRL2,
			    (1 << CTRL2_ACCDET_SHIFT), CTRL2_ACCDET_MASK);

	max77888_update_reg(client, MAX77888_MUIC_REG_CTRL2,
				CTRL2_CPEn0_LOWPWD1,
				CTRL2_CPEn_MASK | CTRL2_LOWPWD_MASK);

	max77888_read_reg(client, MAX77888_MUIC_REG_CTRL2, &cntl2_val);
	dev_info(info->dev, "%s: CNTL2(0x%02x)\n", __func__, cntl2_val);

#if defined(CONFIG_MACH_J_CHN_CTC)
	dev_info(info->dev, "%s: sw_path : %d\n", __func__, info->muic_data->sw_path);
	if(info->muic_data->sw_path == CP_USB_MODE) {
		dev_info(info->dev, "%s: CP_USB_MODE\n", __func__);
		gpio_direction_output(GPIO_USB_BOOT_EN, 0);
	}
#endif
#ifdef CONFIG_USBHUB_USB3803
	/* setting usb hub in default mode (standby) */
	usb3803_set_mode(USB_3803_MODE_STANDBY);
#endif  /* CONFIG_USBHUB_USB3803 */
	info->previous_key = DOCK_KEY_NONE;

#if defined(CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK)
	/* clear CDDelay 500ms */
	max77888_muic_clear_cddelay(info);
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK */

	if (info->cable_type == CABLE_TYPE_NONE_MUIC) {
		dev_info(info->dev, "%s: duplicated(NONE)\n", __func__);
		return 0;
	}
	if (mdata->jig_uart_cb)
		mdata->jig_uart_cb(UART_PATH_AP);

	switch (info->cable_type) {
	case CABLE_TYPE_CHARGING_CABLE_MUIC:
		dev_info(info->dev, "%s: CHARGING CABLE\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		max77888_muic_set_charging_type(info, true);
		break;
	case CABLE_TYPE_OTG_MUIC:
		dev_info(info->dev, "%s: OTG\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_OTGHOST_DETACHED);
		break;
	case CABLE_TYPE_LANHUB_MUIC:
		dev_info(info->dev, "%s: LANHUB\n", __func__);
		if(gInfo->adc == ADC_LANHUB) {
			dev_info(info->dev, "%s: duplicated LANHUB(PASS)\n", __func__);
			break;
		}

		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77888_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = CABLE_TYPE_LANHUB_MUIC;
			break;
		}

		if (mdata->usb_cb && info->is_usb_ready) {
			if(gInfo->adc == ADC_GND) {
				mdata->usb_cb(USB_OTGHOST_ATTACHED);
			} else {
				mdata->usb_cb(USB_LANHUB_DETACHED);
			}
		}
		break;
	case CABLE_TYPE_USB_MUIC:
	case CABLE_TYPE_JIG_USB_OFF_MUIC:
	case CABLE_TYPE_JIG_USB_ON_MUIC:
#ifdef CONFIG_EXTCON
		if (info->edev)
			extcon_set_cable_state(info->edev, "USB", false);
#endif
		dev_info(info->dev, "%s: USB(0x%x)\n", __func__,
			 info->cable_type);
		prev_ct = info->cable_type;
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77888_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = prev_ct;
			break;
		}

		if (mdata->sw_path == CP_USB_MODE)
			return 0;

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_CABLE_DETACHED);
		break;
	case CABLE_TYPE_DESKDOCK_MUIC:
		dev_info(info->dev, "%s: DESKDOCK\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77888_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = CABLE_TYPE_DESKDOCK_MUIC;
			break;
		}
		if ((info->adc!=ADC_DESKDOCK) && mdata->dock_cb)
			mdata->dock_cb(MAX77888_MUIC_DOCK_DETACHED);
		break;
	case CABLE_TYPE_CARDOCK_MUIC:
		dev_info(info->dev, "%s: CARDOCK\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77888_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = CABLE_TYPE_CARDOCK_MUIC;
			break;
		}
		if (mdata->dock_cb)
			mdata->dock_cb(MAX77888_MUIC_DOCK_DETACHED);
		break;
	case CABLE_TYPE_TA_MUIC:
#ifdef CONFIG_EXTCON
		if (info->edev)
			extcon_set_cable_state(info->edev, "TA", false);
#endif
		dev_info(info->dev, "%s: TA\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		ret = max77888_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_TA_MUIC;
		break;
	case CABLE_TYPE_INCOMPATIBLE_MUIC:
#ifdef CONFIG_EXTCON
		if (info->edev)
			extcon_set_cable_state(info->edev, "Incompatible-TA", false);
#endif
		dev_info(info->dev, "%s: Incompatible TA\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		ret = max77888_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_INCOMPATIBLE_MUIC;
		break;
	case CABLE_TYPE_CDP_MUIC:
#ifdef CONFIG_EXTCON
		if (info->edev)
			extcon_set_cable_state(info->edev, "TA", false);
#endif
		dev_info(info->dev, "%s: CDP\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		ret = max77888_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_CDP_MUIC;
		break;
	case CABLE_TYPE_JIG_UART_ON_MUIC:
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK)
		dev_info(info->dev, "%s: JIG UART/BOOTON\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
#else
		if (info->is_factory_start) {
			pr_info("%s:%s JIG_UART_ON\n", DEV_NAME, __func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			if (mdata->dock_cb)
				mdata->dock_cb(MAX77888_MUIC_DOCK_DETACHED);
		}
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK */
		break;
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
		dev_info(info->dev, "%s: JIG UART/BOOTOFF\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		break;
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK)
	case CABLE_TYPE_SMARTDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		max77888_muic_detach_smart_dock(info);
		break;
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK */
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK)
	case CABLE_TYPE_AUDIODOCK_MUIC:
		max77888_muic_detach_audio_dock(info);
		break;
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK */
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
		dev_info(info->dev, "%s: JIG UART/OFF/VB\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		ret = max77888_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_JIG_UART_OFF_VB_MUIC;
		break;
	case CABLE_TYPE_MHL_MUIC:
		if (irq == info->irq_adc || irq == info->irq_chgtype) {
			dev_warn(info->dev, "Detech mhl: Ignore irq:%d\n", irq);
			break;
		}
		dev_info(info->dev, "%s: MHL\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		max77888_muic_set_charging_type(info, false);
		if (mdata->mhl_cb && info->is_mhl_ready)
			mdata->mhl_cb(MAX77888_MUIC_DETACHED);

		break;
	case CABLE_TYPE_MHL_VB_MUIC:
		if (irq == info->irq_adc || irq == info->irq_chgtype) {
			dev_warn(info->dev,
				 "Detech vbMhl: Ignore irq:%d\n", irq);
			break;
		}
		dev_info(info->dev, "%s: MHL VBUS\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		max77888_muic_set_charging_type(info, false);

#ifdef CONFIG_EXTCON
		if (info->edev && info->is_mhl_ready)
			extcon_set_cable_state(info->edev, "MHL", false);
#else
		if (mdata->mhl_cb && info->is_mhl_ready)
			mdata->mhl_cb(MAX77888_MUIC_DETACHED);
#endif
		break;
	case CABLE_TYPE_MMDOCK_MUIC:
		dev_info(info->dev, "%s: MM Dock\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77888_muic_set_charging_type(info, true);
		if (ret)
			pr_err("%s fail to set chg type\n", __func__);

		if (mdata->usb_cb)
			mdata->usb_cb(USB_POWERED_HOST_DETACHED);

		if (mdata->mhl_cb)
			mdata->mhl_cb(MAX77888_MUIC_DETACHED);

		if (mdata->dock_cb)
			mdata->dock_cb(MAX77888_MUIC_DOCK_DETACHED);
		break;
	case CABLE_TYPE_UNKNOWN_MUIC:
		dev_info(info->dev, "%s: UNKNOWN\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77888_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_UNKNOWN_MUIC;
		break;
	default:
		dev_info(info->dev, "%s:invalid cable type %d\n",
			 __func__, info->cable_type);
		break;
	}

	/* jig state clear */
	mdata->jig_state(false);
	return ret;
}

static int max77888_muic_filter_dev(struct max77888_muic_info *info,
					u8 status1, u8 status2)
{
	u8 adc,  adcerr, adc1k, chgtyp, vbvolt, dxovp;
	int intr = INT_ATTACH;

	adc = status1 & STATUS1_ADC_MASK;
	adcerr = status1 & STATUS1_ADCERR_MASK;
	adc1k = status1 & STATUS1_ADC1K_MASK;
	chgtyp = status2 & STATUS2_CHGTYP_MASK;
	vbvolt = status2 & STATUS2_VBVOLT_MASK;
	dxovp = status2 & STATUS2_DXOVP_MASK;

	dev_info(info->dev, "adc:%x adcerr:%x chgtyp:%x vb:%x dxovp:%x cable_type:%d\n",
		adc, adcerr, chgtyp, vbvolt, dxovp, info->cable_type);

	if (adc1k) {
		pr_info("%s:%s MHL cable connected\n", DEV_NAME, __func__);
		return INT_ATTACH;
	}

	switch (adc) {
	case ADC_GND:
		pr_info("%s:%s ADC_GND = OTG\n", DEV_NAME, __func__);
		if (info->cable_type == CABLE_TYPE_LANHUB_MUIC)
			intr = INT_DETACH;
		break;
	case ADC_LANHUB:
		if (vbvolt == 0)
			intr = INT_DETACH;
		break;
	case ADC_INCOMPATIBLE:
	case ADC_OPEN:
		if (!adcerr) {
			if (chgtyp == CHGTYP_NO_VOLTAGE) {
				if (dxovp)
					break;
				else
					intr = INT_DETACH;
			} else if (chgtyp == CHGTYP_USB ||
				 chgtyp == CHGTYP_DOWNSTREAM_PORT ||
				 chgtyp == CHGTYP_DEDICATED_CHGR ||
				 chgtyp == CHGTYP_500MA ||
				 chgtyp == CHGTYP_1A ||
				 chgtyp == CHGTYP_SPECIAL_CHGR) {
				switch (info->cable_type) {
				case CABLE_TYPE_OTG_MUIC:
				case CABLE_TYPE_CHARGING_CABLE_MUIC:
				case CABLE_TYPE_DESKDOCK_MUIC:
				case CABLE_TYPE_CARDOCK_MUIC:
				case CABLE_TYPE_LANHUB_MUIC:
				case CABLE_TYPE_SMARTDOCK_MUIC:
				case CABLE_TYPE_SMARTDOCK_TA_MUIC:
				case CABLE_TYPE_SMARTDOCK_USB_MUIC:
				case CABLE_TYPE_AUDIODOCK_MUIC:
				case CABLE_TYPE_MMDOCK_MUIC:
					intr = INT_DETACH;
					break;
				default:
					break;
				}
			}
		}
		break;
	default:
		break;
	}

	return intr;
}

static void max77888_muic_detect_dev(struct max77888_muic_info *info, int irq)
{
	struct i2c_client *client = info->muic;
	u8 status[2], int_mask, int_mask2;
	int ret;
	u8 cntl1_val;
	int intr = INT_ATTACH;

	ret = max77888_read_reg(client, MAX77888_MUIC_REG_CTRL1, &cntl1_val);
	dev_info(info->dev, "func:%s CONTROL1:%x\n", __func__, cntl1_val);

	ret = max77888_bulk_read(client, MAX77888_MUIC_REG_STATUS1, 2, status);
	dev_info(info->dev, "func:%s irq:%d ret:%d\n", __func__, irq, ret);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg(%d)\n", __func__,
			ret);
		return;
	}

	if((info->cable_type == CABLE_TYPE_OTG_MUIC) && status[0] == 0x33) {

		/* VB voltage interrupt Unmask*/
		max77888_read_reg(info->max77888->muic,
			MAX77888_MUIC_REG_INTMASK2, &int_mask2);
		int_mask2 |= (1 << 4);
		max77888_write_reg(info->max77888->muic,
			MAX77888_MUIC_REG_INTMASK2, int_mask2);

		max77888_write_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_CNFG_00, 0x05);

		/* enable charger interrupt */
		max77888_write_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_INT_MASK, chg_int_state);
		max77888_read_reg(info->max77888->i2c,
			MAX77888_CHG_REG_CHG_INT_MASK, &int_mask);
	}

	dev_info(info->dev, "%s: STATUS1:0x%x, 2:0x%x\n", __func__,
		 status[0], status[1]);

	wake_lock_timeout(&info->muic_wake_lock, HZ * 2);

	intr = max77888_muic_filter_dev(info, status[0], status[1]);
	
	info->adc = status[0] & STATUS1_ADC_MASK;
	info->chgtyp = status[1] & STATUS2_CHGTYP_MASK;
	info->vbvolt = status[1] & STATUS2_VBVOLT_MASK;

	if (intr == INT_ATTACH) {
		dev_info(info->dev, "%s: ATTACHED\n", __func__);
		max77888_muic_handle_attach(info, status[0], status[1], irq);
	} else if (intr == INT_DETACH) {
		dev_info(info->dev, "%s: DETACHED\n", __func__);
		max77888_muic_handle_detach(info, irq);
	} else {
		pr_info("%s:%s device filtered, nothing affect.\n", DEV_NAME,
				__func__);
	}
	return;
}
static irqreturn_t max77888_muic_irq(int irq, void *data)
{
	struct max77888_muic_info *info = data;
	dev_info(info->dev, "%s: irq:%d\n", __func__, irq);

	mutex_lock(&info->mutex);
	max77888_muic_detect_dev(info, irq);
	mutex_unlock(&info->mutex);

	return IRQ_HANDLED;
}

#define REQUEST_IRQ(_irq, _name)					\
do {									\
	ret = request_threaded_irq(_irq, NULL, max77888_muic_irq,	\
				    IRQF_NO_SUSPEND, _name, info);	\
	if (ret < 0)							\
		dev_err(info->dev, "Failed to request IRQ #%d: %d\n",	\
			_irq, ret);					\
} while (0)

static int max77888_muic_irq_init(struct max77888_muic_info *info)
{
	int ret;
	u8 val;

	dev_info(info->dev, "func:%s\n", __func__);

	/* INTMASK1  3:ADC1K 2:ADCErr 0:ADC */
	/* INTMASK2  0:Chgtype */
	max77888_write_reg(info->muic, MAX77888_MUIC_REG_INTMASK1, 0x09);
	max77888_write_reg(info->muic, MAX77888_MUIC_REG_INTMASK2, 0x11);
	max77888_write_reg(info->muic, MAX77888_MUIC_REG_INTMASK3, 0x00);

	REQUEST_IRQ(info->irq_adc, "muic-adc");
	REQUEST_IRQ(info->irq_chgtype, "muic-chgtype");
	REQUEST_IRQ(info->irq_vbvolt, "muic-vbvolt");
	REQUEST_IRQ(info->irq_adc1k, "muic-adc1k");

	dev_info(info->dev, "adc:%d chgtype:%d adc1k:%d vbvolt:%d",
		info->irq_adc, info->irq_chgtype,
		info->irq_adc1k, info->irq_vbvolt);

	max77888_read_reg(info->muic, MAX77888_MUIC_REG_INTMASK1, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77888_MUIC_REG_INTMASK1, val);

	max77888_read_reg(info->muic, MAX77888_MUIC_REG_INTMASK2, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77888_MUIC_REG_INTMASK2, val);

	max77888_read_reg(info->muic, MAX77888_MUIC_REG_INTMASK3, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77888_MUIC_REG_INTMASK3, val);

	max77888_read_reg(info->muic, MAX77888_MUIC_REG_INT1, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77888_MUIC_REG_INT1, val);
	max77888_read_reg(info->muic, MAX77888_MUIC_REG_INT2, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77888_MUIC_REG_INT2, val);
	max77888_read_reg(info->muic, MAX77888_MUIC_REG_INT3, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77888_MUIC_REG_INT3, val);
	return 0;
}

#define CHECK_GPIO(_gpio, _name)					\
do {									\
	if (!_gpio) {							\
		dev_err(&pdev->dev, _name " GPIO defined as 0 !\n");	\
		WARN_ON(!_gpio);					\
		ret = -EIO;						\
		goto err_kfree;						\
	}								\
} while (0)

#if defined(CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK) ||\
	defined(CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK)
static void max77888_muic_dock_detect(struct work_struct *work)
{
	struct max77888_muic_info *info =
		container_of(work, struct max77888_muic_info, dock_work.work);
	struct i2c_client *client = info->muic;
	u8 status[2];
	int ret;
	u8 cntl1_val;
	u8 adc, adcerr, adc1k, chgtyp, vbvolt, dxovp;

	mutex_lock(&info->mutex);
	ret = max77888_read_reg(client, MAX77888_MUIC_REG_CTRL1, &cntl1_val);
	pr_info("%s:%s CONTROL1:%x\n", DEV_NAME, __func__, cntl1_val);

	ret = max77888_bulk_read(client, MAX77888_MUIC_REG_STATUS1, 2, status);
	if (ret) {
		pr_err("%s:%s fail to read muic reg(%d)\n", DEV_NAME, __func__,
			ret);
		goto end;
	}

	pr_info("%s:%s STATUS1:0x%x, 2:0x%x\n", DEV_NAME, __func__, status[0],
		status[1]);

	adc = status[0] & STATUS1_ADC_MASK;
	adcerr = status[0] & STATUS1_ADCERR_MASK;
	adc1k = status[0] & STATUS1_ADC1K_MASK;
	chgtyp = status[1] & STATUS2_CHGTYP_MASK;
	vbvolt = status[1] & STATUS2_VBVOLT_MASK;
	dxovp = status[1] & STATUS2_DXOVP_MASK;

	pr_info("%s:%s adc:%x adcerr:%x chgtyp:%x vb:%x dxovp:%x"\
		" cable_type:%d\n", DEV_NAME, __func__, adc, adcerr, chgtyp,
		vbvolt, dxovp, info->cable_type);

	if (adc1k) {
		pr_info("%s:%s MHL attached, goto end\n", DEV_NAME, __func__);
		goto end;
	}

	if (adcerr) {
		pr_info("%s:%s ADC error, goto end\n", DEV_NAME, __func__);
		goto end;
	}

	switch (adc) {
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK)
	case ADC_SMARTDOCK:
		pr_info("%s:%s Smart Dock\n", DEV_NAME, __func__);

		if (vbvolt && !info->is_usb_ready) {
			pr_info("%s:%s usb not ready yet, D+,D- line(Open)\n",
				DEV_NAME, __func__);
			max77888_muic_set_usb_path(info, OPEN_USB_MODE);
		}
		break;
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK */
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK)
	case ADC_AUDIODOCK:
		pr_info("%s:%s Audio Dock\n", DEV_NAME, __func__);

		if (vbvolt && !info->is_usb_ready) {
			pr_info("%s:%s usb not ready yet, D+,D- line(Open)\n",
				DEV_NAME, __func__);
			max77888_muic_set_usb_path(info, OPEN_USB_MODE);
		}
		break;
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK */
	default:
		break;
	}

end:
	mutex_unlock(&info->mutex);
}
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK ||
	CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK */

static void max77888_muic_init_detect(struct work_struct *work)
{
	struct max77888_muic_info *info =
	    container_of(work, struct max77888_muic_info, init_work.work);
	dev_info(info->dev, "func:%s\n", __func__);
	if(info->cable_type == CABLE_TYPE_UNKNOWN_MUIC)
	{
		mutex_lock(&info->mutex);
		max77888_muic_detect_dev(info, -1);
		mutex_unlock(&info->mutex);
	}
}
#if 0
static int max77888_muic_read_otg_id(struct max77888_muic_info *info)
{
	int ret;
	u8 val, adc1k;

	max77888_read_reg(info->muic, MAX77888_MUIC_REG_STATUS1, &val);
	ret = val & STATUS1_ADC_MASK;
	adc1k = (val & STATUS1_ADC1K_MASK) ? 1 : 0;
	dev_info(info->dev, "func:%s ret:%d val:%x adc1k:%x\n", __func__, ret, val, adc1k);

	if(adc1k)
		return 1;

	return ret;
}
#endif
static void max77888_muic_usb_detect(struct work_struct *work)
{
	struct max77888_muic_info *info =
	    container_of(work, struct max77888_muic_info, usb_work.work);
	struct max77888_muic_data *mdata = info->muic_data;
//	int id_state;

	dev_info(info->dev, "func:%s info->muic_data->sw_path:%d\n",
		 __func__, info->muic_data->sw_path);

	mutex_lock(&info->mutex);
	info->is_usb_ready = true;

	if (info->muic_data->sw_path != CP_USB_MODE) {
		if (mdata->usb_cb) {
			//id_state = max77888_muic_read_otg_id(info);
			//if (id_state)
			//	id_state = 1;
			//mdata->check_id_state(id_state);
			switch (info->cable_type) {
			case CABLE_TYPE_USB_MUIC:
			case CABLE_TYPE_JIG_USB_OFF_MUIC:
			case CABLE_TYPE_JIG_USB_ON_MUIC:
#ifdef CONFIG_USBHUB_USB3803
				/* setting usb hub in Diagnostic(hub) mode */
				usb3803_set_mode(USB_3803_MODE_HUB);
#endif				/* CONFIG_USBHUB_USB3803 */
				mdata->usb_cb(USB_CABLE_ATTACHED);
				break;
			default :
				break;
			}
		}
	}
	mutex_unlock(&info->mutex);
}

static void max77888_muic_dock_usb_detect(struct work_struct *work)
{
	struct max77888_muic_info *info =
	    container_of(work, struct max77888_muic_info, dock_usb_work.work);
	struct max77888_muic_data *mdata = info->muic_data;
//	int id_state;

	dev_info(info->dev, "func:%s info->muic_data->sw_path:%d\n",
		 __func__, info->muic_data->sw_path);

	mutex_lock(&info->mutex);
	info->is_usb_ready = true;

	if (info->muic_data->sw_path != CP_USB_MODE) {
		if (mdata->usb_cb) {
			//id_state = max77888_muic_read_otg_id(info);
			//if (id_state)
			//	id_state = 1;
			//mdata->check_id_state(id_state);
			switch (info->cable_type) {
			case CABLE_TYPE_OTG_MUIC:
				mdata->usb_cb(USB_OTGHOST_ATTACHED);
				break;
			case CABLE_TYPE_SMARTDOCK_MUIC:
				pr_info("%s:%s now usb ready, turn "\
					"D+,D- line to AP_USB\n", DEV_NAME,
					__func__);
				max77888_muic_set_usb_path(info, AP_USB_MODE);
				break;
			case CABLE_TYPE_SMARTDOCK_TA_MUIC:
				pr_info("%s:%s now usb ready, turn "\
					"D+,D- line to AP_USB\n", DEV_NAME,
					__func__);
				max77888_muic_set_usb_path(info, AP_USB_MODE);

				mdata->usb_cb(USB_POWERED_HOST_ATTACHED);
				break;
			case CABLE_TYPE_SMARTDOCK_USB_MUIC:
				pr_info("%s:%s now usb ready, turn "\
					"D+,D- line to AP_USB\n", DEV_NAME,
					__func__);
				max77888_muic_set_usb_path(info, AP_USB_MODE);

				mdata->usb_cb(USB_CABLE_ATTACHED);
				break;
			case CABLE_TYPE_AUDIODOCK_MUIC:
				pr_info("%s:%s now usb ready, turn "\
					"D+,D- line to AP_USB\n", DEV_NAME,
					__func__);
				max77888_muic_set_usb_path(info, AP_USB_MODE);

				mdata->usb_cb(USB_POWERED_HOST_ATTACHED);
				break;
			default:
				break;
			}
		}
	}
	mutex_unlock(&info->mutex);
}

static void max77888_muic_mhl_detect(struct work_struct *work)
{
	struct max77888_muic_info *info =
	    container_of(work, struct max77888_muic_info, mhl_work.work);
	struct max77888_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "func:%s cable_type:%d\n", __func__,
		 info->cable_type);
	mutex_lock(&info->mutex);
	info->is_mhl_ready = true;

	if (info->cable_type == CABLE_TYPE_MHL_MUIC ||
		info->cable_type == CABLE_TYPE_MHL_VB_MUIC ||
		info->cable_type == CABLE_TYPE_SMARTDOCK_MUIC ||
		info->cable_type == CABLE_TYPE_SMARTDOCK_TA_MUIC ||
		info->cable_type == CABLE_TYPE_MMDOCK_MUIC ||
		info->cable_type == CABLE_TYPE_SMARTDOCK_USB_MUIC) {
		if (mdata->mhl_cb)
			mdata->mhl_cb(MAX77888_MUIC_ATTACHED);
	}
	mutex_unlock(&info->mutex);
}

static int uart_switch_init(struct max77888_muic_info *info)
{
#if defined(CONFIG_SWITCH_DUAL_MODEM) || defined(CONFIG_LTE_VIA_SWITCH)
	int ret, val;
#endif

#ifdef CONFIG_LTE_VIA_SWITCH
	ret = gpio_request(GPIO_LTE_VIA_UART_SEL, "LTE_VIA_SEL");
	if (ret < 0) {
		pr_err("Failed to request GPIO_LTE_VIA_UART_SEL!\n");
		return -ENODEV;
	}
	s3c_gpio_setpull(GPIO_LTE_VIA_UART_SEL, S3C_GPIO_PULL_NONE);
	val = gpio_get_value(GPIO_LTE_VIA_UART_SEL);
	gpio_direction_output(GPIO_LTE_VIA_UART_SEL, val);
	pr_info("func: %s lte_gpio val: %d\n", __func__, val);
#endif
/*
#ifndef CONFIG_LTE_VIA_SWITCH
	gpio_export(GPIO_UART_SEL, 1);
	gpio_export_link(switch_dev, "uart_sel", GPIO_UART_SEL);
#endif
*/
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	ret = gpio_request(GPIO_UART_SEL, "UART_SEL");
	if (ret < 0) {
		pr_err("Failed to request GPIO_UART_SEL!\n");
		return -ENODEV;
	}
	val = gpio_get_value(GPIO_UART_SEL);
	gpio_direction_output(GPIO_UART_SEL, val);

	ret = gpio_request(GPIO_USB_SEL, "USB_SEL");
	if (ret < 0) {
		pr_err("Failed to request GPIO_USB_SEL!\n");
		return -ENODEV;
	}
	val = gpio_get_value(GPIO_USB_SEL);
	gpio_direction_output(GPIO_USB_SEL, val);
#endif
	return 0;
}

int max77888_muic_get_status1_adc1k_value(void)
{
	u8 adc1k;
	int ret;

	ret = max77888_read_reg(gInfo->muic,
					MAX77888_MUIC_REG_STATUS1, &adc1k);
	if (ret) {
		dev_err(gInfo->dev, "%s: fail to read muic reg(%d)\n",
					__func__, ret);
		return -EINVAL;
	}
	adc1k = adc1k & STATUS1_ADC1K_MASK ? 1 : 0;

	pr_info("func:%s, adc1k: %d\n", __func__, adc1k);
	/* -1:err, 0:adc1k not detected, 1:adc1k detected */
	return adc1k;
}

int max77888_muic_get_status1_adc_value(void)
{
	u8 adc;
	int ret;

	ret = max77888_read_reg(gInfo->muic,
		MAX77888_MUIC_REG_STATUS1, &adc);
	if (ret) {
		dev_err(gInfo->dev, "%s: fail to read muic reg(%d)\n",
			__func__, ret);
		return -EINVAL;
	}

	return adc & STATUS1_ADC_MASK;
}

/*
* func: max77888_muic_set_audio_switch
* arg: bool enable(true:set vps path, false:set path open)
* return: only 0 success
*/
int max77888_muic_set_audio_switch(bool enable)
{
	struct i2c_client *client = gInfo->muic;
	u8 cntl1_val, cntl1_msk;
	int ret;
	pr_info("func:%s enable(%d)", __func__, enable);

	if (enable) {
		cntl1_val = (MAX77888_MUIC_CTRL1_BIN_2_010 << COMN1SW_SHIFT)
		| (MAX77888_MUIC_CTRL1_BIN_2_010 << COMP2SW_SHIFT) |
		(0 << MICEN_SHIFT);
	} else {
		cntl1_val = 0x3f;
	}
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK | MICEN_MASK;

	ret = max77888_update_reg(client, MAX77888_MUIC_REG_CTRL1, cntl1_val,
			    cntl1_msk);
	cntl1_val = MAX77888_MUIC_CTRL1_BIN_0_000;
	max77888_read_reg(client, MAX77888_MUIC_REG_CTRL1, &cntl1_val);
	dev_info(gInfo->dev, "%s: CNTL1(0x%02x)\n", __func__, cntl1_val);
	return ret;
}

void max77888_update_jig_state(struct max77888_muic_info *info)
{
	struct i2c_client *client = info->muic;
	struct max77888_muic_data *mdata = info->muic_data;
	u8 reg_data, adc;
	int ret, jig_state;

	ret = max77888_read_reg(client, MAX77888_MUIC_REG_STATUS1, &reg_data);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg(%d)\n",
					__func__, ret);
		return;
	}
	adc = reg_data & STATUS1_ADC_MASK;

	switch (adc) {
	case ADC_JIG_UART_OFF:
	case ADC_JIG_USB_OFF:
	case ADC_JIG_USB_ON:
		jig_state = true;
		break;
	default:
		jig_state = false;
		break;
	}

	mdata->jig_state(jig_state);
}

static int __devinit max77888_muic_probe(struct platform_device *pdev)
{
	struct max77888_dev *max77888 = dev_get_drvdata(pdev->dev.parent);
	struct max77888_platform_data *pdata = dev_get_platdata(max77888->dev);
	struct max77888_muic_info *info;
	struct input_dev *input;
	int ret;

	pr_info("func:%s\n", __func__);

	info = kzalloc(sizeof(struct max77888_muic_info), GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "%s: failed to allocate info\n", __func__);
		ret = -ENOMEM;
		goto err_return;
	}

	input = input_allocate_device();
	if (!input) {
		dev_err(&pdev->dev, "%s: failed to allocate input\n", __func__);
		ret = -ENOMEM;
		goto err_kfree;
	}

	info->dev = &pdev->dev;
	info->max77888 = max77888;
	info->muic = max77888->muic;
	info->input = input;
	info->irq_adc = max77888->irq_base + MAX77888_MUIC_IRQ_INT1_ADC;
	info->irq_chgtype = max77888->irq_base + MAX77888_MUIC_IRQ_INT2_CHGTYP;
	info->irq_vbvolt = max77888->irq_base + MAX77888_MUIC_IRQ_INT2_VBVOLT;
	info->irq_adc1k = max77888->irq_base + MAX77888_MUIC_IRQ_INT1_ADC1K;
	info->muic_data = pdata->muic;
	info->is_adc_open_prev = true;
#if !defined(CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK)
	info->is_factory_start = false;
#endif /* !CONFIG_MUIC_MAX77888_SUPPORT_CAR_DOCK */

	wake_lock_init(&info->muic_wake_lock, WAKE_LOCK_SUSPEND,
		"muic wake lock");

	info->cable_type = CABLE_TYPE_UNKNOWN_MUIC;
	info->muic_data->sw_path = AP_USB_MODE;
	info->adc = -1;
	info->chgtyp = 0;
	info->vbvolt = 0;

	gInfo = info;

	platform_set_drvdata(pdev, info);
	dev_info(info->dev, "adc:%d chgtype:%d, adc1k%d\n",
		 info->irq_adc, info->irq_chgtype, info->irq_adc1k);

	input->name = pdev->name;
	input->phys = "deskdock-key/input0";
	input->dev.parent = &pdev->dev;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0001;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	input_set_capability(input, EV_KEY, KEY_VOLUMEUP);
	input_set_capability(input, EV_KEY, KEY_VOLUMEDOWN);
	input_set_capability(input, EV_KEY, KEY_PLAYPAUSE);
	input_set_capability(input, EV_KEY, KEY_PREVIOUSSONG);
	input_set_capability(input, EV_KEY, KEY_NEXTSONG);

	ret = input_register_device(input);
	if (ret) {
		dev_err(info->dev, "%s: Unable to register input device, "\
			"error: %d\n", __func__, ret);
		goto err_input;
	}

	ret = uart_switch_init(info);
	if (ret) {
		pr_err("Failed to initialize uart\n");
		goto err_input;
	}
#if defined(CONFIG_SWITCH_DUAL_MODEM)
		switch_sel &= 0xf;
		if ((switch_sel & MAX77888_SWITCH_SEL_1st_BIT_USB) == 0x1)
			info->muic_data->sw_path = AP_USB_MODE;
		else
			info->muic_data->sw_path = CP_USB_MODE;

		if ((switch_sel & MAX77888_SWITCH_SEL_2nd_BIT_UART)
			== 0x1 << 1)
			info->muic_data->uart_path = UART_PATH_AP;
		else
			info->muic_data->uart_path = UART_PATH_CP;
		pr_info("%s: switch_sel: %x\n", __func__, switch_sel);
#endif
	/* create sysfs group */
	ret = sysfs_create_group(&switch_dev->kobj, &max77888_muic_group);
	dev_set_drvdata(switch_dev, info);
	if (ret) {
		dev_err(&pdev->dev,
			"failed to create max77888 muic attribute group\n");
		goto fail;
	}
#ifdef CONFIG_EXTCON
	/* External connector */
	info->edev = kzalloc(sizeof(struct extcon_dev), GFP_KERNEL);
	if (!info->edev) {
		pr_err("Failed to allocate memory for extcon device\n");
		ret = -ENOMEM;
		goto fail;
	}
	info->edev->name = DEV_NAME;
	info->edev->supported_cable = extcon_cable_name;
	ret = extcon_dev_register(info->edev, NULL);
	if (ret) {
		pr_err("Failed to register extcon device\n");
		kfree(info->edev);
		goto fail;
	}
#endif

	if (info->muic_data->init_cb)
		info->muic_data->init_cb();

	mutex_init(&info->mutex);

	/* Set ADC Mode as  CONTINUOUS Mode*/
	max77888_muic_set_adc_mode(info, 0);

	/* Set ADC debounce time: 25ms */
	max77888_muic_set_adcdbset(info, 2);

	ret = max77888_muic_irq_init(info);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to initialize MUIC irq:%d\n", ret);
		goto fail;
	}

	/* init jig state */
	max77888_update_jig_state(info);

	/* initial cable detection */
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK) ||\
	defined(CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK)
	INIT_DELAYED_WORK(&info->dock_work, max77888_muic_dock_detect);
	schedule_delayed_work(&info->dock_work, msecs_to_jiffies(50));
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK ||
	CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK */

	INIT_DELAYED_WORK(&info->init_work, max77888_muic_init_detect);

	schedule_delayed_work(&info->init_work, msecs_to_jiffies(2500));

	INIT_DELAYED_WORK(&info->usb_work, max77888_muic_usb_detect);
	schedule_delayed_work(&info->usb_work, msecs_to_jiffies(10000));

	INIT_DELAYED_WORK(&info->mhl_work, max77888_muic_mhl_detect);
	schedule_delayed_work(&info->mhl_work, msecs_to_jiffies(25000));

	INIT_DELAYED_WORK(&info->dock_usb_work, max77888_muic_dock_usb_detect);
#if defined(CONFIG_TARGET_LOCALE_KOR)
	schedule_delayed_work(&info->dock_usb_work, msecs_to_jiffies(24000));
#else
	schedule_delayed_work(&info->dock_usb_work, msecs_to_jiffies(17000));
#endif

	return 0;

 fail:
	if (info->irq_adc)
		free_irq(info->irq_adc, NULL);
	if (info->irq_chgtype)
		free_irq(info->irq_chgtype, NULL);
	if (info->irq_vbvolt)
		free_irq(info->irq_vbvolt, NULL);
	if (info->irq_adc1k)
		free_irq(info->irq_adc1k, NULL);
	mutex_destroy(&info->mutex);
 err_input:
	platform_set_drvdata(pdev, NULL);
	input_free_device(input);
	wake_lock_destroy(&info->muic_wake_lock);
 err_kfree:
	kfree(info);
 err_return:
	return ret;
}

static int __devexit max77888_muic_remove(struct platform_device *pdev)
{
	struct max77888_muic_info *info = platform_get_drvdata(pdev);
	sysfs_remove_group(&switch_dev->kobj, &max77888_muic_group);

	if (info) {
		dev_info(info->dev, "func:%s\n", __func__);
		input_unregister_device(info->input);
#if defined(CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK) ||\
	defined(CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK)
		cancel_delayed_work(&info->dock_work);
#endif /* CONFIG_MUIC_MAX77888_SUPPORT_SMART_DOCK ||
	CONFIG_MUIC_MAX77888_SUPPORT_OTG_AUDIO_DOCK */
		cancel_delayed_work(&info->init_work);
		cancel_delayed_work(&info->usb_work);
		cancel_delayed_work(&info->dock_usb_work);
		cancel_delayed_work(&info->mhl_work);
		free_irq(info->irq_adc, info);
		free_irq(info->irq_chgtype, info);
		free_irq(info->irq_vbvolt, info);
		free_irq(info->irq_adc1k, info);
		wake_lock_destroy(&info->muic_wake_lock);
#ifndef CONFIG_TARGET_LOCALE_NA
		gpio_free(info->muic_data->gpio_usb_sel);
#endif
		mutex_destroy(&info->mutex);
		kfree(info);
	}
	return 0;
}

void max77888_muic_shutdown(struct device *dev)
{
	struct max77888_muic_info *info = dev_get_drvdata(dev);
	struct max77888_dev *max77888 = i2c_get_clientdata(info->muic);
	int ret;
	u8 val;

	pr_info("%s:%s +\n", DEV_NAME, __func__);
	if (!info->muic) {
		dev_err(info->dev, "%s: no muic i2c client\n", __func__);
		return;
	}

	pr_info("%s:%s max77888->iolock.count.counter=%d\n", DEV_NAME,
		__func__, max77888->iolock.count.counter);

	pr_info("%s:%s JIGSet: auto detection\n", DEV_NAME, __func__);
	val = (0 << CTRL3_JIGSET_SHIFT) | (0 << CTRL3_BOOTSET_SHIFT);

	ret = max77888_update_reg(info->muic, MAX77888_MUIC_REG_CTRL3, val,
			CTRL3_JIGSET_MASK | CTRL3_BOOTSET_MASK);
	if (ret < 0) {
		dev_err(info->dev, "%s: fail to update reg\n", __func__);
		return;
	}

	muic_otg_control(false);

	pr_info("%s:%s -\n", DEV_NAME, __func__);
}

static struct platform_driver max77888_muic_driver = {
	.driver		= {
		.name	= DEV_NAME,
		.owner	= THIS_MODULE,
		.shutdown = max77888_muic_shutdown,
	},
	.probe		= max77888_muic_probe,
	.remove		= __devexit_p(max77888_muic_remove),
};

static int __init max77888_muic_init(void)
{
	pr_info("func:%s\n", __func__);
	return platform_driver_register(&max77888_muic_driver);
}
module_init(max77888_muic_init);

static void __exit max77888_muic_exit(void)
{
	pr_info("func:%s\n", __func__);
	platform_driver_unregister(&max77888_muic_driver);
}
module_exit(max77888_muic_exit);


MODULE_DESCRIPTION("Maxim MAX77888 MUIC driver");
MODULE_AUTHOR("<sukdong.kim@samsung.com>");
MODULE_LICENSE("GPL");
