/*
 * max77804k-muic.c - MUIC driver for the Maxim 77804k
 *
 *  Copyright (C) 2012 Samsung Electronics.
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
#include <linux/mfd/max77804k.h>
#include <linux/mfd/max77804k-private.h>
#include <linux/host_notify.h>
#include <linux/wakelock.h>
#include <linux/switch.h>
#ifdef CONFIG_USBHUB_USB3803
#include <linux/usb3803.h>
#endif
#include <linux/delay.h>
#if defined(CONFIG_SWITCH_DUAL_MODEM)
#include <linux/sec_param.h>
#endif

#define DEV_NAME	"max77804k-muic"

/* for providing API */
static struct max77804k_muic_info *gInfo;

/* For restore charger interrupt states */
static u8 chg_int_state;

/* MAX77804K MUIC CHG_TYP setting values */
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
	/* Reserved for Future Use */
	CHGTYP_RFU		= 0x06,
	/* Dead Battery Charging, max current 100mA */
	CHGTYP_DB_100MA		= 0x07,
	CHGTYP_MAX,

	CHGTYP_INIT,
	CHGTYP_MIN = CHGTYP_NO_VOLTAGE
};

enum {
	ADC_GND			= 0x00,
#if defined(CONFIG_MUIC_DET_JACK)
	ADC_MHL_OR_SENDEND	= 0x01,
#else
	ADC_MHL			= 0x01,
#endif
	ADC_DOCK_PREV_KEY	= 0x04,
	ADC_DOCK_NEXT_KEY	= 0x07,
	ADC_INCOMPATIBLE1_CHG	= 0x08, /* 0x01000 10.0K ohm*/
	ADC_DOCK_VOL_DN		= 0x0a, /* 0x01010 14.46K ohm */
	ADC_DOCK_VOL_UP		= 0x0b, /* 0x01011 17.26K ohm */
	ADC_DOCK_PLAY_PAUSE_KEY = 0x0d,
	ADC_VZW_USB_DOCK	= 0x0e, /* 0x01110 28.7K ohm VZW Dock */
	ADC_SMARTDOCK		= 0x10, /* 0x10000 40.2K ohm */
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
	ADC_HMT			= 0x11, /* 0x10001 49.9K ohm */
#else
	ADC_INCOMPATIBLE2_CHG	= 0x11, /* 0x10001 49.9K ohm */
#endif
	ADC_AUDIODOCK		= 0x12, /* 0x10010 64.9K ohm */
	ADC_CEA936ATYPE1_CHG	= 0x17,	/* 0x10111 200K ohm */
	ADC_JIG_USB_OFF		= 0x18, /* 0x11000 255K ohm */
	ADC_JIG_USB_ON		= 0x19, /* 0x11001 301K ohm */
	ADC_DESKDOCK		= 0x1a, /* 0x11010 365K ohm */
	ADC_CEA936ATYPE2_CHG	= 0x1b, /* 0x11011 442K ohm */
	ADC_JIG_UART_OFF	= 0x1c, /* 0x11100 523K ohm */
	ADC_JIG_UART_ON		= 0x1d, /* 0x11101 619K ohm */
	ADC_CARDOCK		= 0x1d, /* 0x11101 619K ohm */
#if defined(CONFIG_MUIC_DET_JACK)
	ADC_EARJACK			= 0x1e, /* 0x11110 1000 or 1002 ohm */
#endif
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

struct max77804k_muic_info {
	struct device		*dev;
	struct max77804k_dev	*max77804k;
	struct i2c_client	*muic;
	struct max77804k_muic_data *muic_data;
	int			irq_adc;
	int			irq_chgtype;
	int			irq_vbvolt;
	int			irq_adc1k;
	int			mansw;

	struct wake_lock muic_wake_lock;

	enum cable_type_muic	cable_type;
	struct delayed_work	dock_work;
	struct delayed_work	init_work;
	struct delayed_work	usb_work;
	struct delayed_work	dock_usb_work;
	struct delayed_work	mhl_work;
#if defined(CONFIG_MUIC_DET_JACK)
	struct work_struct earjack_work;
	struct work_struct earjackkey_work;
#endif
	struct mutex		mutex;
#if !defined(CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK)
	bool			is_factory_start;
#endif /* !CONFIG_MUIC_MAX77804K_SUPORT_CAR_DOCK */

	bool			is_usb_ready;
	bool			is_mhl_ready;

	u8		adc;
	u8		chgtyp;
	u8		vbvolt;
	struct input_dev	*input;
	int			previous_key;
	bool			is_adc_open_prev;
#if defined(CONFIG_MUIC_DET_JACK)
	bool			eardetacted;
	bool			earkeypressed;
	int				init_detect_done;
	unsigned int	old_keycode;
	unsigned int	keycode;
#endif
};

#if defined(CONFIG_MUIC_DET_JACK)
static struct switch_dev switch_earjack = {
	.name = "h2w",		/* /sys/class/switch/h2w/state */
};

static struct switch_dev switch_earjackkey = {
	.name = "send_end",	/* /sys/class/switch/send_end/state */
};
#endif

static struct switch_dev switch_uart3 = {
	.name = "uart3",	/* /sys/class/switch/uart3/state */
};

static int if_muic_info;
static int switch_sel;
static int if_pmic_rev;

int is_cardock;
#if defined(CONFIG_MACH_MONTBLANC)
bool gEarDetected = false;
#endif
EXPORT_SYMBOL(is_cardock);

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

static int max77804k_muic_set_comp2_comn1_pass2
	(struct max77804k_muic_info *info, int type, int path)
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
			val = MAX77804K_MUIC_CTRL1_BIN_1_001;
		} else if (path == CP_USB_MODE) {
			info->muic_data->sw_path = CP_USB_MODE;
			val = MAX77804K_MUIC_CTRL1_BIN_4_100;
		} else {
			dev_err(info->dev, "func: %s invalid usb path\n"
				, __func__);
			return -EINVAL;
		}
	} else if (type == 1) {
		if (path == UART_PATH_AP) {
			info->muic_data->uart_path = UART_PATH_AP;
			val = MAX77804K_MUIC_CTRL1_BIN_3_011;
		} else if (path == UART_PATH_CP) {
			info->muic_data->uart_path = UART_PATH_CP;
			val = MAX77804K_MUIC_CTRL1_BIN_5_101;
		} else {
			dev_err(info->dev, "func: %s invalid uart path\n"
				, __func__);
			return -EINVAL;
		}
	}
#if defined(CONFIG_MUIC_DET_JACK)
	else if (type == 2) {
		val = MAX77804K_MUIC_CTRL1_BIN_2_010;
	}
#endif
	else {
		dev_err(info->dev, "func: %s invalid path type(%d)\n"
			, __func__, type);
		return -EINVAL;
	}

	cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;

#if defined(CONFIG_MUIC_DET_JACK)
	if (type == 2 && info->init_detect_done == INIT_START) {
		cntl1_val = cntl1_val | (1<<MICEN_SHIFT);
		cntl1_msk = cntl1_msk | MICEN_MASK;
	}
#endif

	max77804k_update_reg(info->muic, MAX77804K_MUIC_REG_CTRL1, cntl1_val,
			    cntl1_msk);

	return ret;
}

static int max77804k_muic_set_uart_path_pass2
	(struct max77804k_muic_info *info, int path)
{
	int ret = 0;
	ret = max77804k_muic_set_comp2_comn1_pass2(info, 1/*uart*/, path);
	return ret;

}

#if defined(CONFIG_MUIC_DET_JACK)
static int max77804k_muic_set_audio_path_pass2(struct max77804k_muic_info *info, int path)
{
	int ret = 0;

	ret = max77804k_muic_set_comp2_comn1_pass2(info, 2/*audio*/, path);

	return ret;
}
#endif

static ssize_t max77804k_muic_show_usb_state(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
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

static ssize_t max77804k_muic_show_device(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
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
#ifdef CONFIG_MUIC_MAX77804K_SUPPORT_DESKDOCK
	case CABLE_TYPE_DESKDOCK_MUIC:
		return sprintf(buf, "Desk Dock\n");
#endif
	case CABLE_TYPE_SMARTDOCK_MUIC:
		return sprintf(buf, "Smart Dock\n");
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
		return sprintf(buf, "Smart Dock with TA\n");
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		return sprintf(buf, "Smart Dock with USB\n");
	case CABLE_TYPE_AUDIODOCK_MUIC:
		return sprintf(buf, "Audio Dock\n");
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
	case CABLE_TYPE_INCOMPATIBLE_MUIC:
		return sprintf(buf, "Incompatible TA\n");
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

static ssize_t max77804k_muic_show_manualsw(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);

	dev_info(info->dev, "func:%s ap(0),cp(1),vps(2)sw_path:%d\n",
		 __func__, info->muic_data->sw_path);/*For debuging*/

	switch (info->muic_data->sw_path) {
	case AP_USB_MODE:
		return sprintf(buf, "PDA\n");
	case CP_USB_MODE:
		return sprintf(buf, "MODEM\n");
	case AUDIO_MODE:
		return sprintf(buf, "Audio\n");
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

static int max77804k_muic_set_usb_path(struct max77804k_muic_info *info,
					int path);
static ssize_t max77804k_muic_set_manualsw(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);

	dev_info(info->dev, "func:%s buf:%s,count:%d\n", __func__, buf, count);

	if (!strncasecmp(buf, "PDA", 3)) {
		info->muic_data->sw_path = AP_USB_MODE;
		dev_info(info->dev, "%s: AP_USB_MODE\n", __func__);
	} else if (!strncasecmp(buf, "MODEM", 5)) {
		info->muic_data->sw_path = CP_USB_MODE;
		dev_info(info->dev, "%s: CP_USB_MODE\n", __func__);
		max77804k_muic_set_usb_path(info, CP_USB_MODE);
	} else
		dev_warn(info->dev, "%s: Wrong command\n", __func__);

	return count;
}

static ssize_t max77804k_muic_show_adc(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = max77804k_read_reg(info->muic, MAX77804K_MUIC_REG_STATUS1, &val);
	dev_info(info->dev, "func:%s ret:%d val:%x\n", __func__, ret, val);

	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}

	return sprintf(buf, "%x\n", (val & STATUS1_ADC_MASK));
}

static ssize_t max77804k_muic_show_audio_path(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = max77804k_read_reg(info->muic, MAX77804K_MUIC_REG_CTRL1, &val);
	dev_info(info->dev, "func:%s ret:%d val:%x\n", __func__, ret, val);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}

	return sprintf(buf, "%x\n", val);
}

static ssize_t max77804k_muic_set_audio_path(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t count)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
	struct i2c_client *client = info->muic;
	u8 cntl1_val, cntl1_msk;
	u8 val;
	dev_info(info->dev, "func:%s buf:%s\n", __func__, buf);
	if (!strncmp(buf, "0", 1))
		val = MAX77804K_MUIC_CTRL1_BIN_0_000;
	else if (!strncmp(buf, "1", 1))
		val = MAX77804K_MUIC_CTRL1_BIN_2_010;
	else {
		dev_warn(info->dev, "%s: Wrong command\n", __func__);
		return count;
	}

	cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT) |
	    (0 << MICEN_SHIFT);
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK | MICEN_MASK;

	max77804k_update_reg(client, MAX77804K_MUIC_REG_CTRL1, cntl1_val,
			    cntl1_msk);
	dev_info(info->dev, "MUIC cntl1_val:%x, cntl1_msk:%x\n", cntl1_val,
		 cntl1_msk);

	cntl1_val = MAX77804K_MUIC_CTRL1_BIN_0_000;
	max77804k_read_reg(client, MAX77804K_MUIC_REG_CTRL1, &cntl1_val);
	dev_info(info->dev, "%s: CNTL1(0x%02x)\n", __func__, cntl1_val);

	return count;
}

static ssize_t max77804k_muic_show_otg_test(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;

	ret = max77804k_read_reg(info->muic, MAX77804K_MUIC_REG_CDETCTRL1, &val);
	dev_info(info->dev, "func:%s ret:%d val:%x buf%s\n",
		 __func__, ret, val, buf);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	val &= CHGDETEN_MASK;

	return sprintf(buf, "%x\n", val);
}

static ssize_t max77804k_muic_set_otg_test(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
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

	max77804k_update_reg(client, MAX77804K_MUIC_REG_CDETCTRL1,
			    val << CHGDETEN_SHIFT, CHGDETEN_MASK);

	val = 0;
	max77804k_read_reg(client, MAX77804K_MUIC_REG_CDETCTRL1, &val);
	dev_info(info->dev, "%s: CDETCTRL(0x%02x)\n", __func__, val);

	return count;
}

static void max77804k_muic_set_adcdbset(struct max77804k_muic_info *info,
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

	max77804k_read_reg(info->max77804k->i2c,
			MAX77804K_PMIC_REG_PMIC_ID1, &val);

	if (val == 0x34) {
		val = value << CTRL4_ADCDBSET_SHIFT;
		dev_info(info->dev, "%s:CL34 ADCDBSET(0x%02x)\n", __func__, val);
		ret = max77804k_update_reg(info->muic, MAX77804K_MUIC_REG_CTRL4, val,
					  CTRL4_ADCDBSET_MASK|CTRL4_ADCMODE_MASK);
		if (ret < 0)
			dev_err(info->dev, "%s: fail to update reg\n", __func__);
	} else {
		val = value << CTRL3_ADCDBSET_SHIFT;
		dev_info(info->dev, "%s: ADCDBSET(0x%02x)\n", __func__, val);
		ret = max77804k_update_reg(info->muic, MAX77804K_MUIC_REG_CTRL3, val,
					  CTRL3_ADCDBSET_MASK);
		if (ret < 0)
			dev_err(info->dev, "%s: fail to update reg\n", __func__);
	}
}

static ssize_t max77804k_muic_show_adc_debounce_time(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;
	dev_info(info->dev, "func:%s buf:%s\n", __func__, buf);

	if (!info->muic)
		return sprintf(buf, "No I2C client\n");

	ret = max77804k_read_reg(info->muic, MAX77804K_MUIC_REG_CTRL3, &val);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg\n", __func__);
		return sprintf(buf, "UNKNOWN\n");
	}
	val &= CTRL3_ADCDBSET_MASK;
	val = val >> CTRL3_ADCDBSET_SHIFT;
	dev_info(info->dev, "func:%s val:%x\n", __func__, val);
	return sprintf(buf, "%x\n", val);
}

static ssize_t max77804k_muic_set_adc_debounce_time(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
	int value;

	sscanf(buf, "%d", &value);
	value = (value & 0x3);

	dev_info(info->dev, "%s: Do nothing\n", __func__);

	return count;
}

static ssize_t max77804k_muic_set_uart_sel(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);

	dev_info(info->dev, "%s, buf : %s\n",__func__,buf);

	if (!strncasecmp(buf, "AP", 2)) {
		int ret = max77804k_muic_set_uart_path_pass2
			(info, UART_PATH_AP);
		if (ret >= 0){
			info->muic_data->uart_path = UART_PATH_AP;
			dev_info(info->dev, "UART PATH(CP:0, AP:1):%d\n",
				info->muic_data->uart_path);

		}else
			dev_err(info->dev, "%s: Change(AP) fail!!", __func__);
#if defined(CONFIG_SWITCH_DUAL_MODEM)
	} else if (!strncasecmp(buf, "CP2", 2)) {
#else
	} else if (!strncasecmp(buf, "CP", 2)) {
#endif
		int ret = max77804k_muic_set_uart_path_pass2
			(info, UART_PATH_CP);
		if (ret >= 0){
			info->muic_data->uart_path = UART_PATH_CP;
			dev_info(info->dev, "UART PATH(CP:0, AP:1):%d\n",
				info->muic_data->uart_path);
		}else
			dev_err(info->dev, "%s: Change(CP) fail!!", __func__);
	} else {
		dev_warn(info->dev, "%s: Wrong command\n", __func__);
	}
	return count;
}

static ssize_t max77804k_muic_show_uart_sel(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);

	dev_info(info->dev, "func:%s cp(0),ap(1)sw_path:%d\n",
	 __func__, info->muic_data->uart_path);/*For debuging*/

	switch (info->muic_data->uart_path) {
	case UART_PATH_AP:
		return sprintf(buf, "AP\n");
		break;
	case UART_PATH_CP:
#if defined(CONFIG_SWITCH_DUAL_MODEM)
		return sprintf(buf, "CP2\n");
#else
		return sprintf(buf, "CP\n");
#endif
		break;
	default:
		break;
	}
	return sprintf(buf, "UNKNOWN\n");
}

static ssize_t max77804k_muic_show_charger_type(struct device *dev,
					       struct device_attribute *attr,
					       char *buf)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
	u8 adc, status;
	int ret;

	ret = max77804k_read_reg(info->muic, MAX77804K_MUIC_REG_STATUS1, &status);

	adc = status & STATUS1_ADC_MASK;

	/* SYSFS Node : chg_type
	*  SYSFS State
	*  0 : Dedicated Charger
	*  1 : Non-Dedicated Charger
	*  2 : Cigar Jack
	*/
	switch (adc){
#if defined(CONFIG_MUIC_DET_JACK)
	case ADC_MHL_OR_SENDEND:
#else
	case ADC_MHL:
#endif
	case ADC_VZW_USB_DOCK:
	case ADC_SMARTDOCK:
	case ADC_AUDIODOCK:
	case ADC_DESKDOCK:
	case ADC_CARDOCK:
	case ADC_OPEN:
		dev_info(info->dev, "%s: Dedicated Charger State\n", __func__);
		return snprintf(buf, 4, "%d\n", 0);
		break;
	case ADC_CEA936ATYPE1_CHG:
	case ADC_INCOMPATIBLE1_CHG:
#if !defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
	case ADC_INCOMPATIBLE2_CHG:
#endif
		dev_info(info->dev, "%s: Undedicated Charger State\n", __func__);
		return snprintf(buf, 4, "%d\n", 1);
		break;
	case ADC_CEA936ATYPE2_CHG:
		dev_info(info->dev, "%s: Dedicated Charger State\n", __func__);
		return snprintf(buf, 4, "%d\n", 2);
		break;
	default:
		dev_info(info->dev, "%s: Undeclared State\n", __func__);
		break;
	}
	return snprintf(buf, 9, "%s\n", "UNKNOWN");
}

#if !defined(CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK)
static ssize_t max77804k_muic_show_apo_factory(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
	const char *mode;

	/* true: Factory mode, false: not Factory mode */
	if (info->is_factory_start)
		mode = "FACTORY_MODE";
	else
		mode = "NOT_FACTORY_MODE";

	pr_info("%s:%s apo factory=%s\n", DEV_NAME, __func__, mode);

	return sprintf(buf, "%s\n", mode);
}

static ssize_t max77804k_muic_set_apo_factory(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
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
#endif /* !CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK */

#if defined(CONFIG_MUIC_DET_JACK)
static ssize_t key_state_onoff_show(struct device *dev,	struct device_attribute *attr, char *buf)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
	int value = 0;

	if (info->earkeypressed && info->keycode == KEY_MEDIA)
		value = 1;

	return snprintf(buf, 4, "%d\n", value);
}

static ssize_t earjack_state_onoff_show(struct device *dev,	struct device_attribute *attr, char *buf)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
	int value = 0;

	if (info->eardetacted == true)
		value = 1;

	return snprintf(buf, 4, "%d\n", value);
}
#endif

#if !defined(CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK)
static DEVICE_ATTR(apo_factory, 0664,
		max77804k_muic_show_apo_factory,
		max77804k_muic_set_apo_factory);
#endif /* !CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK */
static DEVICE_ATTR(chg_type, 0664, max77804k_muic_show_charger_type, NULL);
static DEVICE_ATTR(uart_sel, 0664, max77804k_muic_show_uart_sel,
		max77804k_muic_set_uart_sel);
static DEVICE_ATTR(usb_state, S_IRUGO, max77804k_muic_show_usb_state, NULL);
static DEVICE_ATTR(device, S_IRUGO, max77804k_muic_show_device, NULL);
static DEVICE_ATTR(usb_sel, 0664,
		max77804k_muic_show_manualsw, max77804k_muic_set_manualsw);
static DEVICE_ATTR(adc, S_IRUGO, max77804k_muic_show_adc, NULL);
static DEVICE_ATTR(audio_path, 0664,
		max77804k_muic_show_audio_path, max77804k_muic_set_audio_path);
static DEVICE_ATTR(otg_test, 0664,
		max77804k_muic_show_otg_test, max77804k_muic_set_otg_test);
static DEVICE_ATTR(adc_debounce_time, 0664,
		max77804k_muic_show_adc_debounce_time,
		max77804k_muic_set_adc_debounce_time);
#if defined(CONFIG_MUIC_DET_JACK)
static DEVICE_ATTR(key_state, 0664,
		key_state_onoff_show, NULL);
static DEVICE_ATTR(state, 0664,
		earjack_state_onoff_show, NULL);
#endif

static struct attribute *max77804k_muic_attributes[] = {
	&dev_attr_uart_sel.attr,
	&dev_attr_usb_state.attr,
	&dev_attr_device.attr,
	&dev_attr_usb_sel.attr,
	&dev_attr_adc.attr,
	&dev_attr_audio_path.attr,
	&dev_attr_otg_test.attr,
	&dev_attr_adc_debounce_time.attr,
	&dev_attr_chg_type.attr,
#if !defined(CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK)
	&dev_attr_apo_factory.attr,
#endif /* !CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK */
	NULL
};

static void max77804k_muic_uart_uevent(int state)
{
#if !defined(CONFIG_MACH_MONTBLANC)
	if (switch_uart3.dev != NULL) {
		switch_set_state(&switch_uart3, state);
		dev_info(gInfo->dev, "%s: state:%d\n", __func__, state);
	}
#endif
}

static const struct attribute_group max77804k_muic_group = {
	.attrs = max77804k_muic_attributes,
};

static int max77804k_muic_set_usb_path(struct max77804k_muic_info *info, int path)
{
	struct i2c_client *client = info->muic;
	struct max77804k_muic_data *mdata = info->muic_data;
	int ret;
	int gpio_val;
	u8 cntl1_val, cntl1_msk;
	int val;
	dev_info(info->dev, "func:%s path:%d, cable_type:%d\n", __func__, path, info->cable_type);

	if (mdata->set_safeout) {
		ret = mdata->set_safeout(path);
		if (ret) {
			dev_err(info->dev, "%s: fail to set safout!\n",
				__func__);
#if defined CONFIG_SEC_RUBENS_PROJECT
			//TODO : please remove comment on battery/regulator bring up
			//return ret;
#else
			return ret;
#endif
		}
	}

	switch (path) {
	case AP_USB_MODE:
		dev_info(info->dev, "%s: AP_USB_MODE\n", __func__);
		gpio_val = 0;
		val = MAX77804K_MUIC_CTRL1_BIN_1_001;
		cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
		break;
	case AUDIO_MODE:
		dev_info(info->dev, "%s: AUDIO_MODE\n", __func__);
		gpio_val = 0;
		/* SL1, SR2 */
		cntl1_val = (MAX77804K_MUIC_CTRL1_BIN_2_010 << COMN1SW_SHIFT)
			| (MAX77804K_MUIC_CTRL1_BIN_2_010 << COMP2SW_SHIFT) |
			(0 << MICEN_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK | MICEN_MASK;
		break;
	case CP_USB_MODE:
		dev_info(info->dev, "%s: CP_USB_MODE\n", __func__);
		gpio_val = 1;
		val = MAX77804K_MUIC_CTRL1_BIN_4_100;
		cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
		break;
	case OPEN_USB_MODE:
		dev_info(info->dev, "%s: OPEN_USB_MODE\n", __func__);
		gpio_val = 0;
		val = MAX77804K_MUIC_CTRL1_BIN_0_000;
		cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
		cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
		break;
	default:
		dev_warn(info->dev, "%s: invalid path(%d)\n", __func__, path);
		return -EINVAL;
	}
	if (info->max77804k->pmic_rev < MAX77804K_REV_PASS2) {
		if (gpio_is_valid(info->muic_data->gpio_usb_sel))
			gpio_direction_output(mdata->gpio_usb_sel, gpio_val);
	}

	dev_info(info->dev, "%s: Set manual path\n", __func__);
	if (info->cable_type != CABLE_TYPE_CARDOCK_MUIC)
		max77804k_update_reg(client, MAX77804K_MUIC_REG_CTRL1, cntl1_val,
			    cntl1_msk);
	max77804k_update_reg(client, MAX77804K_MUIC_REG_CTRL2,
				CTRL2_CPEn1_LOWPWD0,
				CTRL2_CPEn_MASK | CTRL2_LOWPWD_MASK);

	cntl1_val = MAX77804K_MUIC_CTRL1_BIN_0_000;
	max77804k_read_reg(client, MAX77804K_MUIC_REG_CTRL1, &cntl1_val);
	dev_info(info->dev, "%s: CNTL1(0x%02x)\n", __func__, cntl1_val);

	cntl1_val = MAX77804K_MUIC_CTRL1_BIN_0_000;
	max77804k_read_reg(client, MAX77804K_MUIC_REG_CTRL2, &cntl1_val);
	dev_info(info->dev, "%s: CNTL2(0x%02x)\n", __func__, cntl1_val);

	sysfs_notify(&switch_dev->kobj, NULL, "usb_sel");
	return 0;
}

int max77804k_muic_get_charging_type(void)
{
	if (gInfo)
		return gInfo->cable_type;
	else
		return CABLE_TYPE_NONE_MUIC;
}

extern int system_rev;
static int max77804k_muic_set_charging_type(struct max77804k_muic_info *info,
					   bool force_disable)
{
	struct max77804k_muic_data *mdata = info->muic_data;
	int ret = 0;
//#if defined(CONFIG_MACH_HLTEVZW) || defined(CONFIG_MACH_HLTESPR) || defined(CONFIG_MACH_HLTEUSC)
	u8 val;
//#endif

	dev_info(info->dev, "func:%s force_disable:%d\n",
		 __func__, force_disable);
	if (mdata->charger_cb) {
		if (force_disable) {
//#if defined(CONFIG_MACH_HLTEVZW) || defined(CONFIG_MACH_HLTESPR) || defined(CONFIG_MACH_HLTEUSC)
			if (system_rev == 0) {
				val = (0 << CTRL3_JIGSET_SHIFT);
				max77804k_update_reg(info->muic, MAX77804K_MUIC_REG_CTRL3, val,
						CTRL3_JIGSET_MASK);
			}
//#endif
			ret = mdata->charger_cb(CABLE_TYPE_NONE_MUIC);
		} else {
//#if defined(CONFIG_MACH_HLTEVZW) || defined(CONFIG_MACH_HLTESPR) || defined(CONFIG_MACH_HLTEUSC)
			if (system_rev == 0) {
				val = (1 << CTRL3_JIGSET_SHIFT);
				max77804k_update_reg(info->muic, MAX77804K_MUIC_REG_CTRL3, val,
						CTRL3_JIGSET_MASK);
			}
//#endif
			ret = mdata->charger_cb(info->cable_type);
		}
	}

	if (ret) {
//#if defined(CONFIG_MACH_HLTEVZW) || defined(CONFIG_MACH_HLTESPR) || defined(CONFIG_MACH_HLTEUSC)
		if (system_rev == 0) {
			val = (0 << CTRL3_JIGSET_SHIFT);
			max77804k_update_reg(info->muic, MAX77804K_MUIC_REG_CTRL3, val,
					CTRL3_JIGSET_MASK);
		}
//#endif
		dev_err(info->dev, "%s: error from charger_cb(%d)\n", __func__,
			ret);
		return ret;
	}
	return 0;
}
//Handles only deskdock
#ifdef CONFIG_MUIC_MAX77804K_SUPPORT_DESKDOCK
static int max77804k_muic_handle_dock_vol_key(struct max77804k_muic_info *info,
					     u8 status1)
{
	struct input_dev *input = info->input;
	int pre_key = info->previous_key;
	unsigned int code;
	int state;
	u8 adc;

	adc = status1 & STATUS1_ADC_MASK;
	dev_info(info->dev,
		 "func:%s status1:%x adc:%x cable_type:%d\n",
		 __func__, status1, adc, info->cable_type);
	if (info->cable_type != CABLE_TYPE_DESKDOCK_MUIC)
		return 0;

	if (adc == ADC_OPEN) {
		switch (pre_key) {
		case DOCK_KEY_VOL_UP_PRESSED:
			code = KEY_VOLUMEUP;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_UP_RELEASED;
			break;
		case DOCK_KEY_VOL_DOWN_PRESSED:
			code = KEY_VOLUMEDOWN;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_DOWN_RELEASED;
			break;
		case DOCK_KEY_PREV_PRESSED:
			code = KEY_PREVIOUSSONG;
			state = 0;
			info->previous_key = DOCK_KEY_PREV_RELEASED;
			break;
		case DOCK_KEY_PLAY_PAUSE_PRESSED:
			code = KEY_PLAYPAUSE;
			state = 0;
			info->previous_key = DOCK_KEY_PLAY_PAUSE_RELEASED;
			break;
		case DOCK_KEY_NEXT_PRESSED:
			code = KEY_NEXTSONG;
			state = 0;
			info->previous_key = DOCK_KEY_NEXT_RELEASED;
			break;
		default:
			return 0;
		}
		input_event(input, EV_KEY, code, state);
		input_sync(input);
		return 0;
	}

	if (pre_key == DOCK_KEY_NONE) {
		/*
		if (adc != ADC_DOCK_VOL_UP && adc != ADC_DOCK_VOL_DN && \
		adc != ADC_DOCK_PREV_KEY && adc != ADC_DOCK_PLAY_PAUSE_KEY \
		&& adc != ADC_DOCK_NEXT_KEY)
		*/
		if ((adc < 0x03) || (adc > 0x0d))
			return 0;
	}

	dev_info(info->dev, "%s: dock vol key(%d)\n", __func__, pre_key);

	switch (adc) {
	case ADC_DOCK_VOL_UP:
		code = KEY_VOLUMEUP;
		state = 1;
		info->previous_key = DOCK_KEY_VOL_UP_PRESSED;
		break;
	case ADC_DOCK_VOL_DN:
		code = KEY_VOLUMEDOWN;
		state = 1;
		info->previous_key = DOCK_KEY_VOL_DOWN_PRESSED;
		break;
	case ADC_DOCK_PREV_KEY-1 ... ADC_DOCK_PREV_KEY+1:
		code = KEY_PREVIOUSSONG;
		state = 1;
		info->previous_key = DOCK_KEY_PREV_PRESSED;
		break;
	case ADC_DOCK_PLAY_PAUSE_KEY-1 ... ADC_DOCK_PLAY_PAUSE_KEY+1:
		code = KEY_PLAYPAUSE;
		state = 1;
		info->previous_key = DOCK_KEY_PLAY_PAUSE_PRESSED;
		break;
	case ADC_DOCK_NEXT_KEY-1 ... ADC_DOCK_NEXT_KEY+1:
		code = KEY_NEXTSONG;
		state = 1;
		info->previous_key = DOCK_KEY_NEXT_PRESSED;
		break;
	case ADC_DESKDOCK: /* key release routine */
		if (pre_key == DOCK_KEY_VOL_UP_PRESSED) {
			code = KEY_VOLUMEUP;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_UP_RELEASED;
		} else if (pre_key == DOCK_KEY_VOL_DOWN_PRESSED) {
			code = KEY_VOLUMEDOWN;
			state = 0;
			info->previous_key = DOCK_KEY_VOL_DOWN_RELEASED;
		} else if (pre_key == DOCK_KEY_PREV_PRESSED) {
			code = KEY_PREVIOUSSONG;
			state = 0;
			info->previous_key = DOCK_KEY_PREV_RELEASED;
		} else if (pre_key == DOCK_KEY_PLAY_PAUSE_PRESSED) {
			code = KEY_PLAYPAUSE;
			state = 0;
			info->previous_key = DOCK_KEY_PLAY_PAUSE_RELEASED;
		} else if (pre_key == DOCK_KEY_NEXT_PRESSED) {
			code = KEY_NEXTSONG;
			state = 0;
			info->previous_key = DOCK_KEY_NEXT_RELEASED;
		} else {
			dev_warn(info->dev, "%s:%d should not reach here\n",
				 __func__, __LINE__);
			return 0;
		}
		break;
	default:
		dev_warn(info->dev, "%s: unsupported ADC(0x%02x)\n", __func__,
			 adc);
		return 0;
	}

	input_event(input, EV_KEY, code, state);
	input_sync(input);

	return 1;
}
#endif

static int max77804k_muic_attach_usb_type(struct max77804k_muic_info *info,
					 int adc)
{
	struct max77804k_muic_data *mdata = info->muic_data;
	int ret, path;
	dev_info(info->dev, "func:%s adc:%x cable_type:%d\n",
		 __func__, adc, info->cable_type);
	if (info->cable_type == CABLE_TYPE_MHL_MUIC
	    || info->cable_type == CABLE_TYPE_MHL_VB_MUIC) {
		dev_warn(info->dev, "%s: mHL was attached!\n", __func__);
		return 0;
	}

	switch (adc) {
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

		if (info->cable_type == CABLE_TYPE_CDP_MUIC)
			info->cable_type = CABLE_TYPE_CDP_MUIC;
		else
			info->cable_type = CABLE_TYPE_USB_MUIC;

#if defined(CONFIG_SWITCH_DUAL_MODEM)
		if (info->muic_data->sw_path == CP_USB_MODE)
			path = CP_USB_MODE;
		else
			path = AP_USB_MODE;
		dev_info(info->dev, "%s: usb path: %x\n", __func__, mdata->uart_path);
#else
		path = AP_USB_MODE;
#endif
		break;
	default:
		dev_info(info->dev, "%s: Unkown cable(0x%x)\n", __func__, adc);
		return 0;
	}

	ret = max77804k_muic_set_charging_type(info, false);
	if (ret) {
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		return ret;
	}

	max77804k_muic_set_usb_path(info, path);

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

static int max77804k_muic_attach_dock_type(struct max77804k_muic_info *info,
					  int adc, int chgtyp)
{
	struct max77804k_muic_data *mdata = info->muic_data;
	int path;
	dev_info(info->dev, "func:%s adc:%x, open(%d)\n",
			__func__, adc, info->is_adc_open_prev);
	/*Workaround for unstable adc*/
	if (info->is_adc_open_prev == false)
		return 0;
	switch (adc) {
#ifdef CONFIG_MUIC_MAX77804K_SUPPORT_DESKDOCK
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
			mdata->dock_cb(MAX77804K_MUIC_DOCK_DESKDOCK);
		break;
#endif
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


		is_cardock = 1;

		if (mdata->dock_cb)
			mdata->dock_cb(MAX77804K_MUIC_DOCK_CARDOCK);
		break;
	case ADC_AUDIODOCK:
		/* Audio Dock */
		if (info->cable_type == CABLE_TYPE_AUDIODOCK_MUIC) {
			dev_info(info->dev, "%s: duplicated(AudioDock)\n",
				 __func__);
			return 0;
		}
		dev_info(info->dev, "%s:AudioDock\n", __func__);
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

		msleep(40);

		max77804k_muic_set_charging_type(info, false);

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_POWERED_HOST_ATTACHED);

		if (mdata->dock_cb)
			mdata->dock_cb(MAX77804K_MUIC_DOCK_AUDIODOCK);

		break;
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
	case ADC_HMT:
		if (info->cable_type == CABLE_TYPE_HMT_MUIC
		 || info->cable_type == CABLE_TYPE_HMT_TA_MUIC) {
			dev_info(info->dev, "%s: duplicated(HMT)\n",
				__func__);
			return 0;
		}
		dev_info(info->dev, "%s:HMT\n", __func__);

		if (info->is_usb_ready) {
			pr_info("%s:%s usb is ready, D+,D- line(AP_USB)\n",
				DEV_NAME, __func__);
			path = AP_USB_MODE;
		} else {
			pr_info("%s:%s usb not ready yet, D+,D- line(Open)\n",
				DEV_NAME, __func__);
			path = OPEN_USB_MODE;
		}

		if (chgtyp == CHGTYP_DEDICATED_CHGR) {
			info->cable_type = CABLE_TYPE_HMT_TA_MUIC;
		} else {
			info->cable_type = CABLE_TYPE_HMT_MUIC;
		}

		max77804k_muic_set_charging_type(info, false);

		msleep(40);

		if (mdata->dock_cb)
			mdata->dock_cb(MAX77804K_MUIC_DOCK_HMT);
		break;
#endif
	case ADC_SMARTDOCK:
		/* Smart Dock */
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

		max77804k_muic_set_charging_type(info, false);
		msleep(40);
		if (mdata->mhl_cb && info->is_mhl_ready)
			mdata->mhl_cb(MAX77804K_MUIC_ATTACHED);
		if (mdata->dock_cb)
			mdata->dock_cb(MAX77804K_MUIC_DOCK_SMARTDOCK);
		break;
	default:
		dev_info(info->dev, "%s: should not reach here(0x%x)\n",
			 __func__, adc);
		return 0;
	}

	max77804k_muic_set_usb_path(info, path);

	return 0;
}

static void max77804k_muic_attach_mhl(struct max77804k_muic_info *info, u8 chgtyp)
{
	struct max77804k_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "func:%s chgtyp:%x\n", __func__, chgtyp);

	if (info->cable_type == CABLE_TYPE_USB_MUIC) {
		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_CABLE_DETACHED);

		max77804k_muic_set_charging_type(info, true);
	}

	info->cable_type = CABLE_TYPE_MHL_MUIC;

	if (mdata->mhl_cb && info->is_mhl_ready)
		mdata->mhl_cb(MAX77804K_MUIC_ATTACHED);

	if (chgtyp == CHGTYP_USB) {
		info->cable_type = CABLE_TYPE_MHL_VB_MUIC;
		max77804k_muic_set_charging_type(info, false);
	}
}

#if defined(CONFIG_MUIC_DET_JACK)
static void max77804k_muic_earjack_detect_workfunc(struct work_struct *work)
{
	struct max77804k_muic_info *info = container_of(work, struct max77804k_muic_info, earjack_work);
#if defined(CONFIG_MACH_MONTBLANC)
	int ret;
	static struct regulator *reg_l17;
#endif

	dev_info(info->dev, "%s:EarJack Work eardetacted= %d\n", __func__, info->eardetacted);

#if defined(CONFIG_MACH_MONTBLANC)
	if (!reg_l17) {
		reg_l17 = regulator_get(NULL, "8941_l17");
		ret = regulator_set_voltage(reg_l17, 3000000, 3000000);

		if (IS_ERR(reg_l17)) {
			printk(KERN_ERR"could not get 8941_l17, rc = %ld\n", PTR_ERR(reg_l17));
			return;
		}
	}

	if(info->eardetacted) {
		ret = regulator_enable(reg_l17);
		if (ret) {
			printk(KERN_ERR"enable l17 failed, rc=%d\n", ret);
			return;
		}
	}
	else {
		if (regulator_is_enabled(reg_l17)) {
			ret = regulator_disable(reg_l17);
			if (ret) {
				printk(KERN_ERR"disable l17 failed, rc=%d\n", ret);
				return;
			}
		}
	}
#endif

	/* send event jack type to upper layer */
	switch_set_state(&switch_earjack, info->eardetacted);

	return;
}

static int max77804k_muic_earjack_detect(struct max77804k_muic_info *info, bool attached)
{
	dev_info(info->dev, "%s:EarJack attached= %d\n", __func__,attached);

	if (info->init_detect_done == NOT_INIT) {
		return 0;
	}

	if (info->cable_type == CABLE_TYPE_EARJACK_MUIC) {
		dev_info(info->dev, "%s: duplicated(EarJack)\n", __func__);
		return 0;
	}

	if(attached) {
		info->eardetacted = true;
	}
	else {
		info->eardetacted = false;
	}

#if defined(CONFIG_MACH_MONTBLANC)
	gEarDetected = info->eardetacted;
#endif
	schedule_work(&info->earjack_work);

	max77804k_muic_set_audio_path_pass2(info, 0);

	return 0;
}

static void max77804k_muic_earjack_key_detect_workfunc(struct work_struct *work)
{
	struct max77804k_muic_info *info = container_of(work, struct max77804k_muic_info, earjackkey_work);

	dev_info(info->dev, "%s:EarJackKey Work earkeypressed= %d\n", __func__, info->earkeypressed);

	/* send event sendend key type to upper layer */
	switch_set_state(&switch_earjackkey, info->earkeypressed);

	return;
}

static int max77804k_muic_earjack_key_detect(struct max77804k_muic_info *info, int adc)
{
	unsigned int code;

	dev_info(info->dev, "%s:EarJackKey\n", __func__);

	if (info->earkeypressed) {
		switch (adc) {
			case ADC_MHL_OR_SENDEND:
				code = KEY_MEDIA;
				info->old_keycode = KEY_MEDIA;
				break;
			case ADC_DOCK_VOL_UP:
				code = KEY_VOLUMEUP;
				info->old_keycode = KEY_VOLUMEUP;
				break;
			case ADC_DOCK_VOL_DN:
				code = KEY_VOLUMEDOWN;
				info->old_keycode = KEY_VOLUMEDOWN;
				break;
			default:
				dev_info(info->dev, "%s: should not reach here(0x%x)\n", __func__, adc);
				return 0;
		}

		info->keycode = code;
	}
	else {
		code = info->old_keycode;
	}

	/* send event sendend key type to upper layer */
	input_event(info->input, EV_KEY, code, info->earkeypressed);
	input_sync(info->input);

	schedule_work(&info->earjackkey_work);

	return 0;
}
#endif

static void max77804k_muic_handle_jig_uart(struct max77804k_muic_info *info,
					  u8 vbvolt)
{
	struct max77804k_muic_data *mdata = info->muic_data;
	enum cable_type_muic prev_ct = info->cable_type;
	bool is_otgtest = false;
	u8 cntl1_val, cntl1_msk;
	u8 val = MAX77804K_MUIC_CTRL1_BIN_3_011;
	dev_info(info->dev, "func:%s vbvolt:%x cable_type:%d\n",
		 __func__, vbvolt, info->cable_type);
	dev_info(info->dev, "%s: JIG UART/BOOTOFF(0x%x)\n", __func__, vbvolt);

#if defined(CONFIG_SWITCH_DUAL_MODEM)
	dev_info(info->dev, "%s: uart_path : %x\n", __func__, mdata->uart_path);
	if (mdata->uart_path == UART_PATH_AP)
	val = MAX77804K_MUIC_CTRL1_BIN_3_011;
	else if (mdata->uart_path == UART_PATH_CP)
		val = MAX77804K_MUIC_CTRL1_BIN_5_101;
	else
		val = MAX77804K_MUIC_CTRL1_BIN_3_011;
#else
	val = MAX77804K_MUIC_CTRL1_BIN_3_011;
#endif

	cntl1_val = (val << COMN1SW_SHIFT) | (val << COMP2SW_SHIFT);
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK;
	max77804k_update_reg(info->muic, MAX77804K_MUIC_REG_CTRL1, cntl1_val,
			    cntl1_msk);

	max77804k_update_reg(info->muic, MAX77804K_MUIC_REG_CTRL2,
				CTRL2_CPEn1_LOWPWD0,
				CTRL2_CPEn_MASK | CTRL2_LOWPWD_MASK);

	if (vbvolt & STATUS2_VBVOLT_MASK) {
		if (mdata->host_notify_cb) {
			int mode = mdata->host_notify_cb(1);
			if (mode  == NOTIFY_TEST_MODE ||
				mode  == NOTIFY_HOST_MODE) {
				is_otgtest = true;
				dev_info(info->dev, "%s: OTG TEST\n", __func__);
			}
		}
		info->cable_type = CABLE_TYPE_JIG_UART_OFF_VB_MUIC;
		max77804k_muic_set_charging_type(info, is_otgtest);

	} else {
		info->cable_type = CABLE_TYPE_JIG_UART_OFF_MUIC;
		if (prev_ct == CABLE_TYPE_JIG_UART_OFF_VB_MUIC) {
			max77804k_muic_set_charging_type(info, false);

			if (mdata->host_notify_cb)
				mdata->host_notify_cb(0);
		}
	}
}

void max77804k_otg_control(struct max77804k_muic_info *info, int enable)
{
	u8 int_mask, cdetctrl1, chg_cnfg_00;
	pr_info("%s: enable(%d)\n", __func__, enable);

	if (enable) {
		/* disable charger interrupt */
		max77804k_read_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_INT_MASK, &int_mask);
		chg_int_state = int_mask;
		int_mask |= (1 << 4);	/* disable chgin intr */
		int_mask |= (1 << 6);	/* disable chg */
		int_mask &= ~(1 << 0);	/* enable byp intr */
		max77804k_write_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_INT_MASK, int_mask);

		/* disable charger detection */
		max77804k_read_reg(info->max77804k->muic,
			MAX77804K_MUIC_REG_CDETCTRL1, &cdetctrl1);
		cdetctrl1 &= ~(1 << 0);
		max77804k_write_reg(info->max77804k->muic,
			MAX77804K_MUIC_REG_CDETCTRL1, cdetctrl1);

		/* OTG on, boost on, DIS_MUIC_CTRL=1 */
		max77804k_read_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_CNFG_00, &chg_cnfg_00);
		chg_cnfg_00 &= ~(CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		chg_cnfg_00 |= (CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		max77804k_write_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_CNFG_00, chg_cnfg_00);

		/* Update CHG_CNFG_11 to 0x50(5V) */
		max77804k_write_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_CNFG_11, 0x50);
	} else {
		/* OTG off, boost off, (buck on),
		   DIS_MUIC_CTRL = 0 unless CHG_ENA = 1 */
		max77804k_read_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_CNFG_00, &chg_cnfg_00);
		chg_cnfg_00 &= ~(CHG_CNFG_00_OTG_MASK
				| CHG_CNFG_00_BOOST_MASK
				| CHG_CNFG_00_DIS_MUIC_CTRL_MASK);
		chg_cnfg_00 |= CHG_CNFG_00_BUCK_MASK;
		max77804k_write_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_CNFG_00, chg_cnfg_00);

		/* Update CHG_CNFG_11 to 0x00(3V) */
		max77804k_write_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_CNFG_11, 0x00);
		mdelay(50);

#if defined(CONFIG_MACH_HLTEEUR) || defined(CONFIG_MACH_HLTEATT) \
	|| defined(CONFIG_MACH_HLTETMO) || defined(CONFIG_MACH_HLTEVZW) \
	|| defined(CONFIG_MACH_HLTESPR) || defined(CONFIG_MACH_HLTEUSC) \
	|| defined(CONFIG_MACH_HLTESKT) || defined(CONFIG_MACH_HLTEKTT) \
	|| defined(CONFIG_MACH_HLTELGT) || defined(CONFIG_MACH_FLTESKT) \
	|| defined(CONFIG_MACH_FLTEKTT) || defined(CONFIG_MACH_FLTELGT) \
	|| defined(CONFIG_MACH_H3GDUOS_CTC) || defined(CONFIG_MACH_LT03SKT) \
	|| defined(CONFIG_MACH_LT03KTT) || defined(CONFIG_MACH_LT03LGT)
		/* [MAX77804] Workaround to get rid of reading dummy(0x00) */
		/* disable charger detection again */
		max77804k_read_reg(info->max77804k->muic,
			MAX77804K_MUIC_REG_CDETCTRL1, &cdetctrl1);
		cdetctrl1 &= ~(1 << 0);
		max77804k_write_reg(info->max77804k->muic,
			MAX77804K_MUIC_REG_CDETCTRL1, cdetctrl1);

		mdelay(10);
#endif
		/* enable charger detection */
		max77804k_read_reg(info->max77804k->muic,
			MAX77804K_MUIC_REG_CDETCTRL1, &cdetctrl1);
		cdetctrl1 |= (1 << 0);
		max77804k_write_reg(info->max77804k->muic,
			MAX77804K_MUIC_REG_CDETCTRL1, cdetctrl1);

		/* enable charger interrupt */
		max77804k_write_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_INT_MASK, chg_int_state);
		max77804k_read_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_INT_MASK, &int_mask);
	}

	pr_info("%s: INT_MASK(0x%x), CDETCTRL1(0x%x), CHG_CNFG_00(0x%x)\n",
				__func__, int_mask, cdetctrl1, chg_cnfg_00);
}

void max77804k_powered_otg_control(struct max77804k_muic_info *info, int enable)
{
	pr_info("%s: enable(%d)\n", __func__, enable);

	if (enable) {
		/* OTG on, boost on */
		max77804k_write_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_CNFG_00, 0x05);

		max77804k_write_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_CNFG_02, 0x0E);

		/* Update CHG_CNFG_11 to 0x50(5V) */
		max77804k_write_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_CNFG_11, 0x50);
	} else {
		/* OTG off, boost off, (buck on) */
		max77804k_write_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_CNFG_00, 0x04);

		/* Update CHG_CNFG_11 to 0x00(3V) */
		max77804k_write_reg(info->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_CNFG_11, 0x00);
	}
}
/* use in mach for otg */
int muic_otg_control(int enable)
{
	pr_debug("%s: enable(%d)\n", __func__, enable);

	max77804k_otg_control(gInfo, enable);
	return 0;
}

/* use in mach for powered-otg */
void powered_otg_control(int enable)
{
	pr_debug("%s: enable(%d)\n", __func__, enable);

	max77804k_powered_otg_control(gInfo, enable);
}

static void max77804k_muic_set_cddelay(struct max77804k_muic_info *info)
{
	u8 cdetctrl1;
	int ret;

	ret = max77804k_read_reg(info->max77804k->muic,
		MAX77804K_MUIC_REG_CDETCTRL1, &cdetctrl1);

	pr_info("%s:%s read CDETCTRL1=0x%x, ret=%d\n", DEV_NAME, __func__,
			cdetctrl1, ret);

	if ((cdetctrl1 & 0x10) == 0x10) {
		pr_info("%s:%s CDDelay already setted, return\n", DEV_NAME,
				__func__);
		return;
	}

	cdetctrl1 |= 0x10;

	ret = max77804k_write_reg(info->max77804k->muic,
		MAX77804K_MUIC_REG_CDETCTRL1, cdetctrl1);

	pr_info("%s:%s write CDETCTRL1=0x%x, ret=%d\n", DEV_NAME,
			__func__, cdetctrl1, ret);
}

static void max77804k_muic_clear_cddelay(struct max77804k_muic_info *info)
{
	u8 cdetctrl1;
	int ret;

	ret = max77804k_read_reg(info->max77804k->muic,
		MAX77804K_MUIC_REG_CDETCTRL1, &cdetctrl1);

	pr_info("%s:%s read CDETCTRL1=0x%x, ret=%d\n", DEV_NAME, __func__,
			cdetctrl1, ret);

	if ((cdetctrl1 & 0x10) == 0x0) {
		pr_info("%s:%s CDDelay already cleared, return\n", DEV_NAME,
				__func__);
		return;
	}

	cdetctrl1 &= ~(0x10);

	ret = max77804k_write_reg(info->max77804k->muic,
		MAX77804K_MUIC_REG_CDETCTRL1, cdetctrl1);

	pr_info("%s:%s write CDETCTRL1=0x%x, ret=%d\n", DEV_NAME,
			__func__, cdetctrl1, ret);
}

static void max77804k_muic_detach_smart_dock(struct max77804k_muic_info *info)
{
	struct max77804k_muic_data *mdata = info->muic_data;
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
		mdata->dock_cb(MAX77804K_MUIC_DOCK_DETACHED);

	info->cable_type = CABLE_TYPE_NONE_MUIC;

	max77804k_muic_set_charging_type(info, false);
#ifdef CONFIG_EXTCON
	if (info->edev && info->is_mhl_ready)
		extcon_set_cable_state(info->edev, "MHL", false);
#else
	if (mdata->mhl_cb && info->is_mhl_ready)
		mdata->mhl_cb(MAX77804K_MUIC_DETACHED);
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
		max77804k_muic_clear_cddelay(info);
		pr_info("%s:%s SMARTDOCK\n", DEV_NAME, __func__);
		break;
	default:
		pr_warn("%s:%s should not reach here!\n", DEV_NAME,
				__func__);
		return;
		break;
	}
}

static void max77804k_muic_attach_smart_dock(struct max77804k_muic_info *info,
						u8 adc, u8 vbvolt, u8 chgtyp)
{
	struct max77804k_muic_data *mdata = info->muic_data;

	switch (info->cable_type) {
	case CABLE_TYPE_SMARTDOCK_MUIC:
		if (chgtyp == CHGTYP_DEDICATED_CHGR) {
			/* clear CDDelay 500ms */
			max77804k_muic_clear_cddelay(info);
			pr_info("%s:%s SMART_DOCK+TA=OTG Enable\n", DEV_NAME,
					__func__);

			if (mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_POWERED_HOST_ATTACHED);

			info->cable_type = CABLE_TYPE_SMARTDOCK_TA_MUIC;
		} else if (chgtyp == CHGTYP_USB) {
			/* clear CDDelay 500ms */
			max77804k_muic_clear_cddelay(info);
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
			max77804k_muic_detach_smart_dock(info);
		break;
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		if (chgtyp == CHGTYP_USB)
			pr_info("%s:%s Duplicated(SMARTDOCK+USB)\n", DEV_NAME,
					__func__);
		else if (vbvolt)
			pr_info("%s:%s SMART_DOCK + USB -> chgtyp:%x\n",
					DEV_NAME, __func__, chgtyp);
		else
			max77804k_muic_detach_smart_dock(info);
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

			max77804k_muic_attach_dock_type(info, adc, chgtyp);

			if (chgtyp == CHGTYP_DEDICATED_CHGR) {
				/* clear CDDelay 500ms */
				max77804k_muic_clear_cddelay(info);
				pr_info("%s:%s SMART_DOCK+TA=OTG Enable\n", DEV_NAME,
						__func__);

				if (mdata->usb_cb && info->is_usb_ready)
					mdata->usb_cb(USB_POWERED_HOST_ATTACHED);
			} else if (chgtyp == CHGTYP_USB) {
				/* clear CDDelay 500ms */
				max77804k_muic_clear_cddelay(info);
				pr_info("%s:%s SMART_DOCK+USB=USB Enable\n", DEV_NAME,
						__func__);

				if (mdata->usb_cb && info->is_usb_ready)
					mdata->usb_cb(USB_CABLE_ATTACHED);
			}
		} else {
			/* set CDDelay 500ms */
			max77804k_muic_set_cddelay(info);
			dev_warn(info->dev, "no vbus in SAMRTDOCK\n");
		}
		break;
	}
}

#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
static void max77804k_muic_detach_hmt(struct max77804k_muic_info *info)
{
	struct max77804k_muic_data *mdata = info->muic_data;
	enum cable_type_muic tmp_cable_type = info->cable_type;

	pr_info("%s:%s\n", DEV_NAME, __func__);

	if (info->cable_type != CABLE_TYPE_HMT_MUIC &&
	    info->cable_type != CABLE_TYPE_HMT_TA_MUIC) {
		pr_info("%s:%s cable_type is not HMT\n", DEV_NAME,
				__func__);
		return;
	}

	if (mdata->dock_cb)
		mdata->dock_cb(MAX77804K_MUIC_DOCK_DETACHED);

	info->cable_type = CABLE_TYPE_NONE_MUIC;

	max77804k_muic_set_charging_type(info, false);

	switch (tmp_cable_type) {
	case CABLE_TYPE_HMT_TA_MUIC:
		pr_info("%s:%s HMT+TA\n", DEV_NAME, __func__);

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_POWERED_HOST_DETACHED);
		break;
	case CABLE_TYPE_HMT_MUIC:
		pr_info("%s:%s HMT\n", DEV_NAME, __func__);

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_OTGHOST_DETACHED);
		break;
	default:
		pr_warn("%s:%s should not reach here!\n", DEV_NAME,
				__func__);

		if(mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_OTGHOST_DETACHED);
		break;
	}
}

static void max77804k_muic_attach_hmt(struct max77804k_muic_info *info,
						u8 adc, u8 vbvolt, u8 chgtyp)
{
	struct max77804k_muic_data *mdata = info->muic_data;

	pr_info("%s:%s Attached HMT adc(0x%x), vbvolt(0x%x), chgtyp(0x%x)",
		DEV_NAME, __func__, adc, vbvolt, chgtyp);
	switch (info->cable_type) {
	case CABLE_TYPE_HMT_MUIC:
		if (chgtyp == CHGTYP_DEDICATED_CHGR) {
			pr_info("%s:%s HMT+TA=OTG Enable\n", DEV_NAME,
					__func__);

			if (mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_POWERED_HOST_ATTACHED);

			info->cable_type = CABLE_TYPE_HMT_TA_MUIC;
		} else {
			pr_info("%s:%s HMT + [%d] = ?\n", DEV_NAME,
					__func__, chgtyp);
		}
		break;
	case CABLE_TYPE_HMT_TA_MUIC:
		if (chgtyp == CHGTYP_DEDICATED_CHGR)
			pr_info("%s:%s Duplicated(HMT+TA)\n", DEV_NAME,
					__func__);
		else if (vbvolt)
			pr_info("%s:%s HMT + TA -> chgtyp:%x\n",
					DEV_NAME, __func__, chgtyp);
		else
			max77804k_muic_detach_hmt(info);
		break;
	default:
		if (vbvolt) {
			pr_info("%s:%s HMT+vbvolt=chgdetrun\n",
					DEV_NAME, __func__);

			if (chgtyp == 0) {
				pr_info("%s:%s HMT + [%d] = ?? end\n", DEV_NAME,
						__func__, chgtyp);
				return;
			}

			info->cable_type = CABLE_TYPE_HMT_TA_MUIC;
			max77804k_muic_attach_dock_type(info, adc, chgtyp);

			if (chgtyp == CHGTYP_DEDICATED_CHGR) {
				pr_info("%s:%s HMT+TA=OTG Enable\n", DEV_NAME,
						__func__);

				if (mdata->usb_cb && info->is_usb_ready)
					mdata->usb_cb(USB_POWERED_HOST_ATTACHED);
			}
		} else {
			pr_info("%s:%s, No vbus in HMT\n", DEV_NAME, __func__);

			max77804k_muic_attach_dock_type(info, adc, chgtyp);

			if(mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_OTGHOST_ATTACHED);
		}
		break;
	}
}
#endif

static int max77804k_muic_handle_attach(struct max77804k_muic_info *info,
				       u8 status1, u8 status2, int irq)
{
	struct max77804k_muic_data *mdata = info->muic_data;
	u8 adc, vbvolt, chgtyp, chgdetrun, adc1k;
	int ret = 0;

	adc = status1 & STATUS1_ADC_MASK;
	adc1k = status1 & STATUS1_ADC1K_MASK;
	chgtyp = status2 & STATUS2_CHGTYP_MASK;
	vbvolt = status2 & STATUS2_VBVOLT_MASK;
	chgdetrun = status2 & STATUS2_CHGDETRUN_MASK;

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
			max77804k_muic_handle_jig_uart(info, vbvolt);
#if !defined(CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK)
			if (info->is_factory_start &&
					(adc == ADC_JIG_UART_ON)) {
				pr_info("%s:%s factory start, keep attach\n",
						DEV_NAME, __func__);
				break;
			}
#endif /* !CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK */
			dev_warn(info->dev, "%s: abandon ADC\n", __func__);
			return 0;
		}
		if (adc != ADC_JIG_UART_OFF) {
			dev_warn(info->dev, "%s: assume jig uart off detach\n",
				__func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;
		}
		break;
#ifdef CONFIG_MUIC_MAX77804K_SUPPORT_DESKDOCK
	case CABLE_TYPE_DESKDOCK_MUIC:
		if (adc != ADC_DESKDOCK) {
			dev_warn(info->dev, "%s: assume deskdock detach\n", __func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			max77804k_muic_set_charging_type(info, false);
			info->is_adc_open_prev = false;
			if (mdata->dock_cb)
				mdata->dock_cb(MAX77804K_MUIC_DOCK_DETACHED);
		}
		break;
#endif
	case CABLE_TYPE_CARDOCK_MUIC:
		if (adc != ADC_CARDOCK) {
			dev_warn(info->dev, "%s: assume cardock detach\n", __func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			max77804k_muic_set_charging_type(info, false);
			info->is_adc_open_prev = false;
			is_cardock = 0;
			if (mdata->dock_cb)
				mdata->dock_cb(MAX77804K_MUIC_DOCK_DETACHED);
		}
		break;
#if !defined(CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK)
	case CABLE_TYPE_JIG_UART_ON_MUIC:
		if ((adc != ADC_JIG_UART_ON) &&
			info->is_factory_start) {
			pr_warn("%s:%s assume jig uart on detach\n",
					DEV_NAME, __func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			if (mdata->dock_cb)
				mdata->dock_cb(MAX77804K_MUIC_DOCK_DETACHED);
		}
		break;
#endif /* !CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK */
	case CABLE_TYPE_SMARTDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		if (adc != ADC_SMARTDOCK) {
			dev_warn(info->dev, "%s: assume smartdock detach\n",
				__func__);
			max77804k_muic_detach_smart_dock(info);
			info->is_adc_open_prev = false;
		}
		break;
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
	case CABLE_TYPE_HMT_MUIC:
	case CABLE_TYPE_HMT_TA_MUIC:
		if (adc != ADC_HMT) {
			dev_warn(info->dev, "%s: assume hmt detach\n",
				__func__);
			max77804k_muic_detach_hmt(info);
			info->is_adc_open_prev = false;
		}
		break;
#endif
	case CABLE_TYPE_AUDIODOCK_MUIC:
		if ((adc != ADC_AUDIODOCK) || (!vbvolt)) {
			dev_warn(info->dev, "%s: assume audiodock detach\n",
				__func__);
			dev_info(info->dev, "%s: AUDIODOCK\n", __func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;
			if (mdata->dock_cb)
				mdata->dock_cb(MAX77804K_MUIC_DOCK_DETACHED);

			if (mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_POWERED_HOST_DETACHED);
			ret = max77804k_muic_set_charging_type(info, false);
			max77804k_muic_set_charging_type(info, false);
		}
		break;
	case CABLE_TYPE_MHL_MUIC:
	case CABLE_TYPE_MHL_VB_MUIC:
		if (!adc1k) {
			dev_warn(info->dev, "%s: assume mhl detach\n", __func__);
			dev_info(info->dev, "%s: MHL\n", __func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;
			max77804k_muic_set_charging_type(info, false);
			if (mdata->mhl_cb && info->is_mhl_ready)
				mdata->mhl_cb(MAX77804K_MUIC_DETACHED);
		}
		break;
	default:
		break;
	}

	/* 1Kohm ID regiter detection (mHL)
	 * Old MUIC : ADC value:0x00 or 0x01, ADCLow:1
	 * New MUIC : ADC value is not set(Open), ADCLow:1, ADCError:1
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
				max77804k_muic_set_charging_type(info, false);
			} else {
				dev_info(info->dev, "%s: call charger_cb(%d)"
					, __func__, vbvolt);
				max77804k_muic_set_charging_type(info, true);
			}
			return 0;
		}
		max77804k_muic_attach_mhl(info, chgtyp);
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

			max77804k_muic_set_usb_path(info, AP_USB_MODE);
			msleep(40);
			if (mdata->usb_cb && info->is_usb_ready)
				mdata->usb_cb(USB_OTGHOST_ATTACHED);

		} else if (chgtyp == CHGTYP_USB ||
			   chgtyp == CHGTYP_DOWNSTREAM_PORT ||
			   chgtyp == CHGTYP_DEDICATED_CHGR ||
			   chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A) {
			dev_info(info->dev, "%s: OTG charging pump\n",
				 __func__);
			ret = max77804k_muic_set_charging_type(info, !vbvolt);
		}
		break;
	case ADC_SMARTDOCK:
		max77804k_muic_attach_smart_dock(info, adc, vbvolt, chgtyp);
		break;
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
	case ADC_HMT:
		max77804k_muic_attach_hmt(info, adc, vbvolt, chgtyp);
		break;
#endif
	case ADC_VZW_USB_DOCK:
	case ADC_AUDIODOCK:
		if(!!vbvolt)
			max77804k_muic_attach_dock_type(info, adc, chgtyp);
		break;
	case ADC_JIG_UART_OFF:
		max77804k_muic_handle_jig_uart(info, vbvolt);
		mdata->jig_state(true);
		max77804k_muic_uart_uevent(true);
		break;
	case ADC_JIG_USB_OFF:
	case ADC_JIG_USB_ON:
		if (vbvolt & STATUS2_VBVOLT_MASK) {
			dev_info(info->dev, "%s: SKIP_JIG_USB\n", __func__);
			ret = max77804k_muic_attach_usb_type(info, adc);
		}
		mdata->jig_state(true);
		max77804k_muic_uart_uevent(true);
		break;
	case ADC_DESKDOCK:
		max77804k_muic_attach_dock_type(info, adc, chgtyp);
		if (chgtyp == CHGTYP_USB ||
			chgtyp == CHGTYP_DOWNSTREAM_PORT ||
			chgtyp == CHGTYP_DEDICATED_CHGR ||
			chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A)
			ret = max77804k_muic_set_charging_type(info, false);
		else if (chgtyp == CHGTYP_NO_VOLTAGE && !chgdetrun)
			ret = max77804k_muic_set_charging_type(info, !vbvolt);
			/* For MAX77804K IC doesn`t occur chgtyp IRQ
			* because of audio noise prevention.
			* So, If below condition is set,
			* we do charging at CARDOCK.
			*/
		break;
#if defined(CONFIG_MUIC_DET_JACK)
	case ADC_MHL_OR_SENDEND:
	case ADC_DOCK_VOL_UP:
	case ADC_DOCK_VOL_DN:
		if ((!adc1k) && (info->cable_type == CABLE_TYPE_EARJACK_MUIC)) {
			info->earkeypressed = true;
			max77804k_muic_earjack_key_detect(info, adc);
		}
		break;
	case ADC_EARJACK:
		if ((info->cable_type == CABLE_TYPE_EARJACK_MUIC) && (info->earkeypressed)) {
			info->earkeypressed = false;
			max77804k_muic_earjack_key_detect(info, adc);
		} else {
			max77804k_muic_earjack_detect(info, true);
			info->cable_type = CABLE_TYPE_EARJACK_MUIC;
		}
		break;
#endif
	case ADC_CARDOCK:
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK)
		max77804k_muic_attach_dock_type(info, adc, chgtyp);
		if (chgtyp == CHGTYP_USB ||
			chgtyp == CHGTYP_DOWNSTREAM_PORT ||
			chgtyp == CHGTYP_DEDICATED_CHGR ||
			chgtyp == CHGTYP_500MA || chgtyp == CHGTYP_1A)
			ret = max77804k_muic_set_charging_type(info, false);
		else if (chgtyp == CHGTYP_NO_VOLTAGE && !chgdetrun)
			ret = max77804k_muic_set_charging_type(info, !vbvolt);
			/* For MAX77804K IC doesn`t occur chgtyp IRQ
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
				break;
			}
			pr_info("%s:%s JIG_UART_ON\n", DEV_NAME, __func__);
			info->cable_type = CABLE_TYPE_JIG_UART_ON_MUIC;

			if (mdata->dock_cb)
				mdata->dock_cb(MAX77804K_MUIC_DOCK_CARDOCK);

			return 0;
		}
#endif /* CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK */
		break;
	case ADC_CEA936ATYPE1_CHG:
	case ADC_CEA936ATYPE2_CHG:
	case ADC_OPEN:
		switch (chgtyp) {
		case CHGTYP_USB:
		case CHGTYP_DOWNSTREAM_PORT:
#ifdef CONFIG_USBHUB_USB3803
			/* setting usb hub in default mode (standby) */
			usb3803_set_mode(USB_3803_MODE_STANDBY);
#endif /* CONFIG_USBHUB_USB3803 */
			if (chgtyp == CHGTYP_DOWNSTREAM_PORT) {
				dev_info(info->dev, "%s, CDP(charging)\n",
					__func__);
				info->cable_type = CABLE_TYPE_CDP_MUIC;
			}
			if (info->cable_type == CABLE_TYPE_MHL_MUIC) {
				dev_info(info->dev, "%s: MHL(charging)\n",
					 __func__);
				info->cable_type = CABLE_TYPE_MHL_VB_MUIC;
				ret = max77804k_muic_set_charging_type(info,
								      !vbvolt);
				return ret;
			}
			ret = max77804k_muic_attach_usb_type(info, adc);
			break;
		case CHGTYP_DEDICATED_CHGR:
		case CHGTYP_500MA:
		case CHGTYP_1A:
		default:
			dev_info(info->dev, "%s:TA\n", __func__);
			info->cable_type = CABLE_TYPE_TA_MUIC;
#ifdef CONFIG_USBHUB_USB3803
			/* setting usb hub in default mode (standby) */
			usb3803_set_mode(USB_3803_MODE_STANDBY);
#endif				/* CONFIG_USBHUB_USB3803 */
			ret = max77804k_muic_set_charging_type(info, !vbvolt);
			if (ret)
				info->cable_type = CABLE_TYPE_NONE_MUIC;
			break;
		}
		break;
	default:
		dev_warn(info->dev, "%s: unsupported adc=0x%x\n", __func__, adc);

		info->cable_type = CABLE_TYPE_INCOMPATIBLE_MUIC;

		ret = max77804k_muic_set_charging_type(info, !vbvolt);
		if (ret)
			info->cable_type = CABLE_TYPE_NONE_MUIC;
		break;
	}
	return ret;
}

static int max77804k_muic_handle_detach(struct max77804k_muic_info *info, int irq)
{
	struct i2c_client *client = info->muic;
	struct max77804k_muic_data *mdata = info->muic_data;
	enum cable_type_muic prev_ct = CABLE_TYPE_NONE_MUIC;
	u8 cntl2_val;
	int ret = 0;
	dev_info(info->dev, "func:%s\n", __func__);

	info->is_adc_open_prev = true;
	/* Workaround: irq doesn't occur after detaching mHL cable */
	max77804k_write_reg(client, MAX77804K_MUIC_REG_CTRL1,
				MAX77804K_MUIC_CTRL1_BIN_0_000);

	/* Enable Factory Accessory Detection State Machine */
	max77804k_update_reg(client, MAX77804K_MUIC_REG_CTRL2,
			    (1 << CTRL2_ACCDET_SHIFT), CTRL2_ACCDET_MASK);

	max77804k_update_reg(client, MAX77804K_MUIC_REG_CTRL2,
#if defined(CONFIG_MACH_MONTBLANC)
				CTRL2_CPEn1_LOWPWD0,
#else
				CTRL2_CPEn0_LOWPWD1,
#endif
				CTRL2_CPEn_MASK | CTRL2_LOWPWD_MASK);

	max77804k_read_reg(client, MAX77804K_MUIC_REG_CTRL2, &cntl2_val);
	dev_info(info->dev, "%s: CNTL2(0x%02x)\n", __func__, cntl2_val);

#ifdef CONFIG_USBHUB_USB3803
	/* setting usb hub in default mode (standby) */
	usb3803_set_mode(USB_3803_MODE_STANDBY);
#endif  /* CONFIG_USBHUB_USB3803 */
	info->previous_key = DOCK_KEY_NONE;
	/* clear CDDelay 500ms */
	max77804k_muic_clear_cddelay(info);

	if (info->cable_type == CABLE_TYPE_NONE_MUIC) {
		dev_info(info->dev, "%s: duplicated(NONE)\n", __func__);
		return 0;
	}

	switch (info->cable_type) {
	case CABLE_TYPE_OTG_MUIC:
		dev_info(info->dev, "%s: OTG\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_OTGHOST_DETACHED);
		break;
	case CABLE_TYPE_USB_MUIC:
	case CABLE_TYPE_CDP_MUIC:
	case CABLE_TYPE_JIG_USB_OFF_MUIC:
	case CABLE_TYPE_JIG_USB_ON_MUIC:
		dev_info(info->dev, "%s: USB(0x%x)\n", __func__,
			 info->cable_type);
		prev_ct = info->cable_type;
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77804k_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = prev_ct;
			break;
		}

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_CABLE_DETACHED);
		break;
#ifdef CONFIG_MUIC_MAX77804K_SUPPORT_DESKDOCK
	case CABLE_TYPE_DESKDOCK_MUIC:
		dev_info(info->dev, "%s: DESKDOCK\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77804k_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = CABLE_TYPE_DESKDOCK_MUIC;
			break;
		}
		if ((info->adc!=ADC_DESKDOCK) && mdata->dock_cb)
			mdata->dock_cb(MAX77804K_MUIC_DOCK_DETACHED);
		break;
#endif
	case CABLE_TYPE_CARDOCK_MUIC:
		dev_info(info->dev, "%s: CARDOCK\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;


		is_cardock = 0;

		ret = max77804k_muic_set_charging_type(info, false);
		if (ret) {
			info->cable_type = CABLE_TYPE_CARDOCK_MUIC;
			break;
		}
		if (mdata->dock_cb)
			mdata->dock_cb(MAX77804K_MUIC_DOCK_DETACHED);
		break;
	case CABLE_TYPE_TA_MUIC:
	case CABLE_TYPE_INCOMPATIBLE_MUIC:
		dev_info(info->dev, "%s: TA\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		ret = max77804k_muic_set_charging_type(info, false);
		if (ret)
			info->cable_type = CABLE_TYPE_TA_MUIC;
		break;
	case CABLE_TYPE_JIG_UART_ON_MUIC:
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK)
		dev_info(info->dev, "%s: JIG UART/BOOTON\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
#else
		if (info->is_factory_start) {
			pr_info("%s:%s JIG_UART_ON\n", DEV_NAME, __func__);
			info->cable_type = CABLE_TYPE_NONE_MUIC;

			if (mdata->dock_cb)
				mdata->dock_cb(MAX77804K_MUIC_DOCK_DETACHED);
		}
#endif /* CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK */
		break;
	case CABLE_TYPE_JIG_UART_OFF_MUIC:
		dev_info(info->dev, "%s: JIG UART/BOOTOFF\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		break;
	case CABLE_TYPE_SMARTDOCK_MUIC:
	case CABLE_TYPE_SMARTDOCK_TA_MUIC:
	case CABLE_TYPE_SMARTDOCK_USB_MUIC:
		max77804k_muic_detach_smart_dock(info);
		break;
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
	case CABLE_TYPE_HMT_MUIC:
	case CABLE_TYPE_HMT_TA_MUIC:
		max77804k_muic_detach_hmt(info);
		break;
#endif
	case CABLE_TYPE_AUDIODOCK_MUIC:
		dev_info(info->dev, "%s: AUDIODOCK\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		if ((info->adc!=ADC_DESKDOCK) && mdata->dock_cb)
			mdata->dock_cb(MAX77804K_MUIC_DOCK_DETACHED);

		if (mdata->usb_cb && info->is_usb_ready)
			mdata->usb_cb(USB_POWERED_HOST_DETACHED);
		ret = max77804k_muic_set_charging_type(info, false);
		max77804k_muic_set_charging_type(info, false);
		break;
	case CABLE_TYPE_JIG_UART_OFF_VB_MUIC:
		dev_info(info->dev, "%s: JIG UART/OFF/VB\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		ret = max77804k_muic_set_charging_type(info, false);
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
		max77804k_muic_set_charging_type(info, false);
		if (mdata->mhl_cb && info->is_mhl_ready)
			mdata->mhl_cb(MAX77804K_MUIC_DETACHED);

		break;
	case CABLE_TYPE_MHL_VB_MUIC:
		if (irq == info->irq_adc || irq == info->irq_chgtype) {
			dev_warn(info->dev,
				 "Detech vbMhl: Ignore irq:%d\n", irq);
			break;
		}
		dev_info(info->dev, "%s: MHL VBUS\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		max77804k_muic_set_charging_type(info, false);

		if (mdata->mhl_cb && info->is_mhl_ready)
			mdata->mhl_cb(MAX77804K_MUIC_DETACHED);
		break;
#if defined(CONFIG_MUIC_DET_JACK)
	case CABLE_TYPE_EARJACK_MUIC:
		dev_info(info->dev, "%s: EARJACK\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;
		max77804k_muic_earjack_detect(info, false);
		break;
#endif
	case CABLE_TYPE_UNKNOWN_MUIC:
		dev_info(info->dev, "%s: UNKNOWN\n", __func__);
		info->cable_type = CABLE_TYPE_NONE_MUIC;

		ret = max77804k_muic_set_charging_type(info, false);
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
	max77804k_muic_uart_uevent(false);
	return ret;
}

static int max77804k_muic_filter_dev(struct max77804k_muic_info *info,
					u8 status1, u8 status2)
{
	u8 adc, adcerr, adc1k, chgtyp, vbvolt, dxovp;
	int intr = INT_ATTACH;
	adc = status1 & STATUS1_ADC_MASK;
	adcerr = status1 & STATUS1_ADCERR_MASK;
	adc1k = status1 & STATUS1_ADC1K_MASK;
	chgtyp = status2 & STATUS2_CHGTYP_MASK;
	vbvolt = status2 & STATUS2_VBVOLT_MASK;
	dxovp = status2 & STATUS2_DXOVP_MASK;
	dev_info(info->dev, "adc:%x adcerr:%x chgtyp:%x vb:%x dxovp:%x cable_type:%d\n",
		adc, adcerr, chgtyp, vbvolt, dxovp, info->cable_type);

#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_MHL_CABLE_DETECTION)
	if (adc1k) {
		pr_info("%s:%s MHL cable connected\n", DEV_NAME, __func__);
		return INT_ATTACH;
	}
#endif /* CONFIG_MUIC_MAX77804K_SUPPORT_MHL_CABLE_DETECTION */

	switch (adc) {
	case ADC_GND:
			pr_info("%s:%s ADC_GND & !adclow = OTG\n", DEV_NAME,
					__func__);
		break;
	case (ADC_CEA936ATYPE1_CHG) ... (ADC_JIG_UART_ON):
		if(info->cable_type != CABLE_TYPE_NONE_MUIC
			&& chgtyp == CHGTYP_NO_VOLTAGE
			&& info->chgtyp != chgtyp) {
			intr = INT_DETACH;
		}
		break;
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
				 chgtyp == CHGTYP_1A) {
				switch (info->cable_type) {
				case CABLE_TYPE_OTG_MUIC:
#ifdef CONFIG_MUIC_MAX77804K_SUPPORT_DESKDOCK
				case CABLE_TYPE_DESKDOCK_MUIC:
#endif
				case CABLE_TYPE_CARDOCK_MUIC:
				case CABLE_TYPE_SMARTDOCK_MUIC:
				case CABLE_TYPE_SMARTDOCK_TA_MUIC:
				case CABLE_TYPE_SMARTDOCK_USB_MUIC:
				case CABLE_TYPE_AUDIODOCK_MUIC:
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
				case CABLE_TYPE_HMT_MUIC:
				case CABLE_TYPE_HMT_TA_MUIC:
#endif
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

static void max77804k_muic_detect_dev(struct max77804k_muic_info *info, int irq)
{
	struct i2c_client *client = info->muic;
	u8 status[2];
	int intr = INT_ATTACH;
	int ret;
	u8 cntl1_val;

	ret = max77804k_read_reg(client, MAX77804K_MUIC_REG_CTRL1, &cntl1_val);
	dev_info(info->dev, "func:%s CONTROL1:%x\n", __func__, cntl1_val);

	ret = max77804k_bulk_read(client, MAX77804K_MUIC_REG_STATUS1, 2, status);
	dev_info(info->dev, "func:%s irq:%d ret:%d\n", __func__, irq, ret);
	if (ret) {
		dev_err(info->dev, "%s: fail to read muic reg(%d)\n", __func__,
			ret);
		return;
	}

	dev_info(info->dev, "%s: STATUS1:0x%x, 2:0x%x\n", __func__,
		 status[0], status[1]);
#ifdef CONFIG_MUIC_MAX77804K_SUPPORT_DESKDOCK
	if ((irq == info->irq_adc) &&
	    max77804k_muic_handle_dock_vol_key(info, status[0])) {
		dev_info(info->dev,
			 "max77804k_muic_handle_dock_vol_key(irq_adc:%x)", irq);
		return;
	}
#endif

	wake_lock_timeout(&info->muic_wake_lock, HZ * 2);

	intr = max77804k_muic_filter_dev(info, status[0], status[1]);

	info->adc = status[0] & STATUS1_ADC_MASK;
	info->chgtyp = status[1] & STATUS2_CHGTYP_MASK;
	info->vbvolt = status[1] & STATUS2_VBVOLT_MASK;

	if (intr == INT_ATTACH) {
		dev_info(info->dev, "%s: ATTACHED\n", __func__);
		max77804k_muic_handle_attach(info, status[0], status[1], irq);
	} else if (intr == INT_DETACH) {
		dev_info(info->dev, "%s: DETACHED\n", __func__);
		max77804k_muic_handle_detach(info, irq);
	} else {
		pr_info("%s:%s device filtered, nothing affect.\n", DEV_NAME,
				__func__);
	}
	return;
}

static irqreturn_t max77804k_muic_irq(int irq, void *data)
{
	struct max77804k_muic_info *info = data;
	dev_info(info->dev, "%s: irq:%d\n", __func__, irq);

	mutex_lock(&info->mutex);
	max77804k_muic_detect_dev(info, irq);
	mutex_unlock(&info->mutex);

	return IRQ_HANDLED;
}

#define REQUEST_IRQ(_irq, _name)					\
do {									\
	ret = request_threaded_irq(_irq, NULL, max77804k_muic_irq,	\
				    0, _name, info);			\
	if (ret < 0)							\
		dev_err(info->dev, "Failed to request IRQ #%d: %d\n",	\
			_irq, ret);					\
} while (0)

static int max77804k_muic_irq_init(struct max77804k_muic_info *info)
{
	int ret;
	u8 val;

	dev_info(info->dev, "func:%s\n", __func__);
	/* dev_info(info->dev, "%s: system_rev=%x\n", __func__, system_rev); */

	/* INTMASK1  3:ADC1K 2:ADCErr 1:ADCLow 0:ADC */
	/* INTMASK2  0:Chgtype */
	max77804k_write_reg(info->muic, MAX77804K_MUIC_REG_INTMASK1, 0x09);
	max77804k_write_reg(info->muic, MAX77804K_MUIC_REG_INTMASK2, 0x11);
	max77804k_write_reg(info->muic, MAX77804K_MUIC_REG_INTMASK3, 0x00);

	REQUEST_IRQ(info->irq_adc, "muic-adc");
	REQUEST_IRQ(info->irq_chgtype, "muic-chgtype");
	REQUEST_IRQ(info->irq_vbvolt, "muic-vbvolt");
	REQUEST_IRQ(info->irq_adc1k, "muic-adc1k");

	dev_info(info->dev, "adc:%d chgtype:%d adc1k:%d vbvolt:%d",
		info->irq_adc, info->irq_chgtype,
		info->irq_adc1k, info->irq_vbvolt);

	max77804k_read_reg(info->muic, MAX77804K_MUIC_REG_INTMASK1, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77804K_MUIC_REG_INTMASK1, val);

	max77804k_read_reg(info->muic, MAX77804K_MUIC_REG_INTMASK2, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77804K_MUIC_REG_INTMASK2, val);

	max77804k_read_reg(info->muic, MAX77804K_MUIC_REG_INTMASK3, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77804K_MUIC_REG_INTMASK3, val);

	max77804k_read_reg(info->muic, MAX77804K_MUIC_REG_INT1, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77804K_MUIC_REG_INT1, val);
	max77804k_read_reg(info->muic, MAX77804K_MUIC_REG_INT2, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77804K_MUIC_REG_INT2, val);
	max77804k_read_reg(info->muic, MAX77804K_MUIC_REG_INT3, &val);
	dev_info(info->dev, "%s: reg=%x, val=%x\n", __func__,
		 MAX77804K_MUIC_REG_INT3, val);
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

static void max77804k_muic_dock_detect(struct work_struct *work)
{
	struct max77804k_muic_info *info =
		container_of(work, struct max77804k_muic_info, dock_work.work);
	struct i2c_client *client = info->muic;
	u8 status[2];
	int ret;
	u8 cntl1_val;
	u8 adc, adcerr, adc1k, chgtyp, vbvolt, dxovp;

	mutex_lock(&info->mutex);
	ret = max77804k_read_reg(client, MAX77804K_MUIC_REG_CTRL1, &cntl1_val);
	pr_info("%s:%s CONTROL1:%x\n", DEV_NAME, __func__, cntl1_val);

	ret = max77804k_bulk_read(client, MAX77804K_MUIC_REG_STATUS1, 2, status);
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
	case ADC_SMARTDOCK:
		pr_info("%s:%s Smart Dock\n", DEV_NAME, __func__);

		if (vbvolt && !info->is_usb_ready) {
			pr_info("%s:%s usb not ready yet, D+,D- line(Open)\n",
				DEV_NAME, __func__);
			max77804k_muic_set_usb_path(info, OPEN_USB_MODE);
		} else
			max77804k_muic_set_usb_path(info, AP_USB_MODE);
		break;
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
	case ADC_HMT:
		pr_info("%s:%s HMT\n", DEV_NAME, __func__);

		if (vbvolt && !info->is_usb_ready) {
			pr_info("%s:%s usb not ready yet, D+,D- line(Open)\n",
				DEV_NAME, __func__);
			max77804k_muic_set_usb_path(info, OPEN_USB_MODE);
		} else
			max77804k_muic_set_usb_path(info, AP_USB_MODE);
		break;
#endif
	case ADC_AUDIODOCK:
		pr_info("%s:%s Audio Dock\n", DEV_NAME, __func__);

		if (vbvolt && !info->is_usb_ready) {
			pr_info("%s:%s usb not ready yet, D+,D- line(Open), usb_state:%d\n",
				DEV_NAME, __func__, info->is_usb_ready);
			max77804k_muic_set_usb_path(info, OPEN_USB_MODE);
		} else
			max77804k_muic_set_usb_path(info, AP_USB_MODE);
		break;
	default:
		break;
	}

end:
	mutex_unlock(&info->mutex);
}

static void max77804k_muic_init_detect(struct work_struct *work)
{
	struct max77804k_muic_info *info =
	    container_of(work, struct max77804k_muic_info, init_work.work);
	dev_info(info->dev, "func:%s\n", __func__);

	mutex_lock(&info->mutex);
#if defined(CONFIG_MUIC_DET_JACK)
	info->init_detect_done = INIT_START;
#endif

	max77804k_muic_detect_dev(info, -1);

#if defined(CONFIG_MUIC_DET_JACK)
	info->init_detect_done = INIT_DONE;
#endif
	mutex_unlock(&info->mutex);
}

static void max77804k_muic_usb_detect(struct work_struct *work)
{
	struct max77804k_muic_info *info =
	    container_of(work, struct max77804k_muic_info, usb_work.work);
	struct max77804k_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "func:%s info->muic_data->sw_path:%d\n",
		 __func__, info->muic_data->sw_path);

	mutex_lock(&info->mutex);
	info->is_usb_ready = true;

	if (mdata->usb_cb) {
		switch (info->cable_type) {
		case CABLE_TYPE_CDP_MUIC:
		case CABLE_TYPE_USB_MUIC:
		case CABLE_TYPE_JIG_USB_OFF_MUIC:
		case CABLE_TYPE_JIG_USB_ON_MUIC:
#ifdef CONFIG_USBHUB_USB3803
			/* setting usb hub in Diagnostic(hub) mode */
			usb3803_set_mode(USB_3803_MODE_HUB);
#endif			/* CONFIG_USBHUB_USB3803 */
			mdata->usb_cb(USB_CABLE_ATTACHED);
			break;
		default :
			break;
		}
	}
	mutex_unlock(&info->mutex);
}

static void max77804k_muic_dock_usb_detect(struct work_struct *work)
{
	struct max77804k_muic_info *info =
	    container_of(work, struct max77804k_muic_info, dock_usb_work.work);
	struct max77804k_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "func:%s info->muic_data->sw_path:%d\n",
		 __func__, info->muic_data->sw_path);

	mutex_lock(&info->mutex);
	info->is_usb_ready = true;

	if (mdata->usb_cb) {
		switch (info->cable_type) {
		case CABLE_TYPE_OTG_MUIC:
			mdata->usb_cb(USB_OTGHOST_ATTACHED);
			break;
		case CABLE_TYPE_SMARTDOCK_MUIC:
		case CABLE_TYPE_SMARTDOCK_TA_MUIC:
			pr_info("%s:%s now usb ready, turn"\
				"D+,D- line to AP_USB\n", DEV_NAME,
				__func__);
			max77804k_muic_set_usb_path(info, AP_USB_MODE);

			mdata->usb_cb(USB_POWERED_HOST_ATTACHED);
			break;
		case CABLE_TYPE_SMARTDOCK_USB_MUIC:
			pr_info("%s:%s now usb ready, turn"\
				"D+,D- line to AP_USB\n", DEV_NAME,
				__func__);
			max77804k_muic_set_usb_path(info, AP_USB_MODE);

			mdata->usb_cb(USB_CABLE_ATTACHED);
			break;
#if defined(CONFIG_MUIC_MAX77804K_SUPPORT_HMT_DETECTION)
		case CABLE_TYPE_HMT_MUIC:
			pr_info("%s:%s now usb ready, turn"\
				"D+,D- line to AP_USB\n", DEV_NAME,
				__func__);
			max77804k_muic_set_usb_path(info, AP_USB_MODE);

			mdata->usb_cb(USB_OTGHOST_ATTACHED);
			break;
		case CABLE_TYPE_HMT_TA_MUIC:
			pr_info("%s:%s now usb ready, turn"\
				"D+,D- line to AP_USB\n", DEV_NAME,
				__func__);
			max77804k_muic_set_usb_path(info, AP_USB_MODE);

			mdata->usb_cb(USB_POWERED_HOST_ATTACHED);
			break;
#endif
		case CABLE_TYPE_AUDIODOCK_MUIC:
			pr_info("%s:%s now usb ready, turn"\
				"D+,D- line to AP_USB\n", DEV_NAME,
				__func__);
			max77804k_muic_set_usb_path(info, AP_USB_MODE);

			mdata->usb_cb(USB_POWERED_HOST_ATTACHED);
			break;
		default:
			break;
		}
	}
	mutex_unlock(&info->mutex);
}
#if !defined(CONFIG_SEC_H_PORJECT)
static void max77804k_muic_mhl_detect(struct work_struct *work)
{
	struct max77804k_muic_info *info =
	    container_of(work, struct max77804k_muic_info, mhl_work.work);
	struct max77804k_muic_data *mdata = info->muic_data;

	dev_info(info->dev, "func:%s cable_type:%d\n", __func__,
		 info->cable_type);
	mutex_lock(&info->mutex);
	info->is_mhl_ready = true;

	if (info->cable_type == CABLE_TYPE_MHL_MUIC ||
	    info->cable_type == CABLE_TYPE_MHL_VB_MUIC ||
	    info->cable_type == CABLE_TYPE_SMARTDOCK_MUIC ||
	    info->cable_type == CABLE_TYPE_SMARTDOCK_TA_MUIC ||
	    info->cable_type == CABLE_TYPE_SMARTDOCK_USB_MUIC) {
		if (mdata->mhl_cb)
			mdata->mhl_cb(MAX77804K_MUIC_ATTACHED);
	}
	mutex_unlock(&info->mutex);
}
#endif
int max77804k_muic_get_status1_adc1k_value(void)
{
	u8 adc1k;
	int ret;

	ret = max77804k_read_reg(gInfo->muic,
					MAX77804K_MUIC_REG_STATUS1, &adc1k);
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

int max77804k_muic_get_status1_adc_value(void)
{
	u8 adc;
	int ret;

	ret = max77804k_read_reg(gInfo->muic,
		MAX77804K_MUIC_REG_STATUS1, &adc);
	if (ret) {
		dev_err(gInfo->dev, "%s: fail to read muic reg(%d)\n",
			__func__, ret);
		return -EINVAL;
	}

	return adc & STATUS1_ADC_MASK;
}

/*
* func: max77804k_muic_set_audio_switch
* arg: bool enable(true:set vps path, false:set path open)
* return: only 0 success
*/
int max77804k_muic_set_audio_switch(bool enable)
{
	struct i2c_client *client = gInfo->muic;
	u8 cntl1_val, cntl1_msk;
	int ret;
	pr_info("func:%s enable(%d)", __func__, enable);

	if (enable) {
		cntl1_val = (MAX77804K_MUIC_CTRL1_BIN_2_010 << COMN1SW_SHIFT)
		| (MAX77804K_MUIC_CTRL1_BIN_2_010 << COMP2SW_SHIFT) |
		(0 << MICEN_SHIFT);
	} else {
		cntl1_val = 0x3f;
	}
	cntl1_msk = COMN1SW_MASK | COMP2SW_MASK | MICEN_MASK;

	ret = max77804k_update_reg(client, MAX77804K_MUIC_REG_CTRL1, cntl1_val,
			    cntl1_msk);
	cntl1_val = MAX77804K_MUIC_CTRL1_BIN_0_000;
	max77804k_read_reg(client, MAX77804K_MUIC_REG_CTRL1, &cntl1_val);
	dev_info(gInfo->dev, "%s: CNTL1(0x%02x)\n", __func__, cntl1_val);
	return ret;
}

void max77804k_update_jig_state(struct max77804k_muic_info *info)
{
	struct i2c_client *client = info->muic;
	struct max77804k_muic_data *mdata = info->muic_data;
	u8 reg_data, adc;
	int ret, jig_state;

	ret = max77804k_read_reg(client, MAX77804K_MUIC_REG_STATUS1, &reg_data);
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
	max77804k_muic_uart_uevent(jig_state);
}


static int __devinit max77804k_muic_probe(struct platform_device *pdev)
{
	struct max77804k_dev *max77804k = dev_get_drvdata(pdev->dev.parent);
	struct max77804k_platform_data *pdata = dev_get_platdata(max77804k->dev);
	struct max77804k_muic_info *info;
	struct input_dev *input;
#if defined(CONFIG_MUIC_DET_JACK)
	struct class *audio;
	struct device *earjack;
#endif
	int ret;
	u8 cdetctrl1;

	pr_info("func:%s\n", __func__);

	info = kzalloc(sizeof(struct max77804k_muic_info), GFP_KERNEL);
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
	info->max77804k = max77804k;
	info->muic = max77804k->muic;
	info->input = input;
	info->irq_adc = max77804k->irq_base + MAX77804K_MUIC_IRQ_INT1_ADC;
	info->irq_chgtype = max77804k->irq_base + MAX77804K_MUIC_IRQ_INT2_CHGTYP;
	info->irq_vbvolt = max77804k->irq_base + MAX77804K_MUIC_IRQ_INT2_VBVOLT;
	info->irq_adc1k = max77804k->irq_base + MAX77804K_MUIC_IRQ_INT1_ADC1K;
	info->muic_data = pdata->muic_data;
	info->muic_data->uart_path = UART_PATH_AP;
	info->is_adc_open_prev = true;
#if !defined(CONFIG_MUIC_MAX77804K_SUPPORT_CAR_DOCK)
	info->is_factory_start = false;
#endif /* !CONFIG_MUIC_MZX77804k_SUPPORT_CAR_DOCK */

	wake_lock_init(&info->muic_wake_lock, WAKE_LOCK_SUSPEND,
		"muic wake lock");

	info->cable_type = CABLE_TYPE_UNKNOWN_MUIC;
	info->muic_data->sw_path = AP_USB_MODE;
#if defined(CONFIG_MUIC_DET_JACK)
	info->eardetacted = false;
	info->earkeypressed = false;
	info->init_detect_done = NOT_INIT;
	info->old_keycode = 0;
	info->keycode = 0;
#endif
	gInfo = info;

	platform_set_drvdata(pdev, info);
	dev_info(info->dev, "adc:%d chgtype:%d, adc1k%d\n",
		 info->irq_adc, info->irq_chgtype, info->irq_adc1k);

	input->name = pdev->name;
#ifdef CONFIG_MUIC_MAX77804K_SUPPORT_DESKDOCK
	input->phys = "deskdock-key/input0";
#endif
	input->dev.parent = &pdev->dev;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0001;

	/* Enable auto repeat feature of Linux input subsystem */
	__set_bit(EV_REP, input->evbit);

	dev_info(info->dev, "input->name:%s\n", input->name);

#if defined(CONFIG_MUIC_DET_JACK)
	input_set_capability(input, EV_KEY, KEY_MEDIA);
#endif
	input_set_capability(input, EV_KEY, KEY_VOLUMEUP);
	input_set_capability(input, EV_KEY, KEY_VOLUMEDOWN);
	input_set_capability(input, EV_KEY, KEY_PLAYPAUSE);
	input_set_capability(input, EV_KEY, KEY_PREVIOUSSONG);
	input_set_capability(input, EV_KEY, KEY_NEXTSONG);

	ret = input_register_device(input);
	if (ret) {
		dev_err(info->dev, "%s: Unable to register input device, "
			"error: %d\n", __func__, ret);
		goto err_input;
	}

#if defined(CONFIG_SWITCH_DUAL_MODEM)
	switch_sel &= 0xf;
	if ((switch_sel & MAX77804K_SWITCH_SEL_1st_BIT_USB) == 0x1)
		info->muic_data->sw_path = AP_USB_MODE;
	else
		info->muic_data->sw_path = CP_USB_MODE;

	if ((switch_sel & MAX77804K_SWITCH_SEL_2nd_BIT_UART)
		== 0x1 << 1)
		info->muic_data->uart_path = UART_PATH_AP;
	else
		info->muic_data->uart_path = UART_PATH_CP;

	pr_err("%s: switch_sel: %x\n", __func__, switch_sel);
#endif

	/* create sysfs group */
	ret = sysfs_create_group(&switch_dev->kobj, &max77804k_muic_group);
	dev_set_drvdata(switch_dev, info);
	if (ret) {
		dev_err(&pdev->dev,
			"failed to create max77804k muic attribute group\n");
		goto fail;
	}

	if (info->muic_data->init_cb)
		info->muic_data->init_cb();

	mutex_init(&info->mutex);

	/* Set ADC debounce time: 25ms */
	max77804k_muic_set_adcdbset(info, 2);

	/* Set DCDTmr to 2sec */
	max77804k_read_reg(info->max77804k->muic,
		MAX77804K_MUIC_REG_CDETCTRL1, &cdetctrl1);
	cdetctrl1 &= ~(1 << 5);
	max77804k_write_reg(info->max77804k->muic,
		MAX77804K_MUIC_REG_CDETCTRL1, cdetctrl1);
	pr_info("%s: CDETCTRL1(0x%02x)\n", __func__, cdetctrl1);

#if defined(CONFIG_MUIC_DET_JACK)
	ret = switch_dev_register(&switch_earjack);
	if (ret < 0) {
		pr_info("%s : Failed to register switch device\n", __func__);
		goto err_switch_dev_register;
	}

	ret = switch_dev_register(&switch_earjackkey);
	if (ret < 0) {
		pr_info("%s : Failed to register switch device\n", __func__);
		goto err_switch_dev_register;
	}

	audio = class_create(THIS_MODULE, "audio");
	if (IS_ERR(audio))
		pr_err("Failed to create class(audio)!\n");

	earjack = device_create(audio, NULL, 0, NULL, "earjack");
	if (IS_ERR(earjack))
		pr_err("Failed to create device(earjack)!\n");

	ret = device_create_file(earjack, &dev_attr_key_state);
	if (ret)
		pr_err("Failed to create device file in sysfs entries(%s)!\n",
			dev_attr_key_state.attr.name);

	ret = device_create_file(earjack, &dev_attr_state);
	if (ret)
		pr_err("Failed to create device file in sysfs entries(%s)!\n",
			dev_attr_state.attr.name);

	dev_set_drvdata(earjack, info);
#endif
	ret = switch_dev_register(&switch_uart3);
	if (ret < 0) {
		pr_err("%s : Failed to register switch_uart3 device\n", __func__);
		goto err_switch_uart3_dev_register;
	}

	ret = max77804k_muic_irq_init(info);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to initialize MUIC irq:%d\n", ret);
		goto fail;
	}

	/* initial cable detection */
	INIT_DELAYED_WORK(&info->dock_work, max77804k_muic_dock_detect);
	schedule_delayed_work(&info->dock_work, msecs_to_jiffies(50));

	INIT_DELAYED_WORK(&info->init_work, max77804k_muic_init_detect);
	schedule_delayed_work(&info->init_work, msecs_to_jiffies(3000));

	INIT_DELAYED_WORK(&info->usb_work, max77804k_muic_usb_detect);
	schedule_delayed_work(&info->usb_work, msecs_to_jiffies(10000));

	INIT_DELAYED_WORK(&info->dock_usb_work, max77804k_muic_dock_usb_detect);
	schedule_delayed_work(&info->dock_usb_work, msecs_to_jiffies(22000));
#if !defined(CONFIG_SEC_H_PORJECT)
	INIT_DELAYED_WORK(&info->mhl_work, max77804k_muic_mhl_detect);
	schedule_delayed_work(&info->mhl_work, msecs_to_jiffies(25000));
#endif

#if defined(CONFIG_MUIC_DET_JACK)
	INIT_WORK(&info->earjack_work, max77804k_muic_earjack_detect_workfunc);
	INIT_WORK(&info->earjackkey_work, max77804k_muic_earjack_key_detect_workfunc);
#endif

	/* init jig state */
	max77804k_update_jig_state(info);

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
#if defined(CONFIG_MUIC_DET_JACK)
 err_switch_dev_register:
	switch_dev_unregister(&switch_earjack);
	switch_dev_unregister(&switch_earjackkey);
#endif
 err_switch_uart3_dev_register:
	switch_dev_unregister(&switch_uart3);
 err_return:
	return ret;
}

static int __devexit max77804k_muic_remove(struct platform_device *pdev)
{
	struct max77804k_muic_info *info = platform_get_drvdata(pdev);
	sysfs_remove_group(&switch_dev->kobj, &max77804k_muic_group);

	if (info) {
		dev_info(info->dev, "func:%s\n", __func__);
		input_unregister_device(info->input);
		cancel_delayed_work(&info->dock_work);
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
#endif /* CONFIG_TARGET_LOCALE_NA */
		mutex_destroy(&info->mutex);
		kfree(info);
	}
	return 0;
}

void max77804k_muic_shutdown(struct device *dev)
{
	struct max77804k_muic_info *info = dev_get_drvdata(dev);
	int ret;
	u8 val;
	dev_info(info->dev, "func:%s\n", __func__);
	if (!info->muic) {
		dev_err(info->dev, "%s: no muic i2c client\n", __func__);
		return;
	}

	dev_info(info->dev, "%s: JIGSet: auto detection\n", __func__);
	val = (0 << CTRL3_JIGSET_SHIFT) | (0 << CTRL3_BOOTSET_SHIFT);

	ret = max77804k_update_reg(info->muic, MAX77804K_MUIC_REG_CTRL3, val,
			CTRL3_JIGSET_MASK | CTRL3_BOOTSET_MASK);
	if (ret < 0) {
		dev_err(info->dev, "%s: fail to update reg\n", __func__);
		return;
	}
}

#if defined(CONFIG_MACH_MONTBLANC)
static int max77804k_muic_suspend(struct platform_device *pdev,
			pm_message_t state)
{
#if 0
	int ret;
	static struct regulator *reg_l17;

	if (!reg_l17) {
		reg_l17 = regulator_get(NULL, "8941_l17");

		if (IS_ERR(reg_l17)) {
			pr_err("could not get 8941_l17, ret = %ld\n", PTR_ERR(reg_l17));
			return -ENODEV;
		}
	}

	if (regulator_is_enabled(reg_l17)) {
		ret = regulator_disable(reg_l17);
		if (ret) {
			pr_err("disable l17 failed, ret=%d\n", ret);
			return ret;
		}
	}
#endif
	return 0;
}

static int max77804k_muic_resume(struct platform_device *pdev)
{
#if 0
	int ret;
	static struct regulator *reg_l17;

	if (!reg_l17) {
		reg_l17 = regulator_get(NULL, "8941_l17");

		if (IS_ERR(reg_l17)) {
			pr_err("could not get 8941_l17, ret = %ld\n", PTR_ERR(reg_l17));
			return -ENODEV;
		}
	}

	if(gEarDetected)	{
		if(!regulator_is_enabled(reg_l17))	{
			ret = regulator_set_voltage(reg_l17, 3000000, 3000000);
			if (ret) {
				pr_err("set_voltage l17 failed, ret=%d\n", ret);
				return -EINVAL;
			}

			ret = regulator_enable(reg_l17);
			if (ret) {
				pr_err("enable l17 failed, ret=%d\n", ret);
				return ret;
			}
		}
	}
	else	{
		if (regulator_is_enabled(reg_l17)) {
			ret = regulator_disable(reg_l17);
			if (ret) {
				pr_err("disable l17 failed, ret=%d\n", ret);
				return ret;
			}
		}
	}
#endif
	return 0;
}
#endif

static struct platform_driver max77804k_muic_driver = {
	.driver		= {
		.name	= DEV_NAME,
		.owner	= THIS_MODULE,
		.shutdown = max77804k_muic_shutdown,
	},
	.probe		= max77804k_muic_probe,
	.remove		= __devexit_p(max77804k_muic_remove),
#if defined(CONFIG_MACH_MONTBLANC)
	.suspend	= max77804k_muic_suspend,
	.resume		= max77804k_muic_resume,
#endif
};

static int __init max77804k_muic_init(void)
{
	pr_info("func:%s\n", __func__);
	return platform_driver_register(&max77804k_muic_driver);
}
module_init(max77804k_muic_init);

static void __exit max77804k_muic_exit(void)
{
	pr_info("func:%s\n", __func__);
	platform_driver_unregister(&max77804k_muic_driver);
}
module_exit(max77804k_muic_exit);


MODULE_DESCRIPTION("Maxim MAX77804K MUIC driver");
MODULE_AUTHOR("<sukdong.kim@samsung.com>");
MODULE_LICENSE("GPL");
