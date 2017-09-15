/*
 * driver/misc/sm5504.c - SM5504 micro USB switch device driver
 *
 * Copyright (C) 2014 Samsung Electronics
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c/sm5504.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/pmic8058.h>
#include <linux/input.h>
#include <linux/switch.h>
#if defined (CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif

#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif

/* spmi control */
extern int spmi_ext_register_writel_extra(u8 sid, u16 ad, u8 *buf, int len);
extern int spmi_ext_register_readl_extra(u8 sid, u16 ad, u8 *buf, int len);

extern int system_rev;
static int bcd_scan = 0;		// L Cable check

#define INT_MASK1				0xFC
#define INT_MASK2				0xFC

/* DEVICE ID */
#define SM5504_DEV_ID				0x01
#define SM5504_DEV_ID_REV			0x00

/* SM5504 I2C registers */
#define REG_DEVICE_ID				0x01
#define REG_CONTROL				0x02
#define REG_INT1				0x03
#define REG_INT2				0x04
#define REG_INT_MASK1				0x05
#define REG_INT_MASK2				0x06
#define REG_ADC					0x07
#define REG_DEVICE_TYPE1			0x0a
#define REG_DEVICE_TYPE2			0x0b
#define REG_MANUAL_SW1				0x13
#define REG_MANUAL_SW2				0x14
#define REG_RESET				0x1B
#define REG_RESERVED_1				0x20
#define REG_CHG_TYPE				0x24

#define DATA_NONE				0x00

/* Control */
#define CON_ADC_EN		(1 << 7)
#define CON_USBCHDEN		(1 << 6)
#define CON_CHGTYP		(1 << 5)
#define CON_SWITCH_OPEN		(1 << 4)
#define CON_MANUAL_SW		(1 << 2)
#define CON_INT_MASK		(1 << 0)
#define CON_MASK		(CON_USBCHDEN | CON_CHGTYP | \
				CON_MANUAL_SW | CON_ADC_EN)
#define CON_ADC_MASK	(CON_USBCHDEN | CON_CHGTYP | \
				CON_MANUAL_SW)

/* Device Type 1 */
#define DEV_DEDICATED_CHG		(1 << 6)
#define DEV_USB_CHG			(1 << 5)
#define DEV_CAR_KIT			(1 << 4)
#define DEV_UART			(1 << 3)
#define DEV_USB				(1 << 2)
#define DEV_USB_OTG			(1 << 0)

#define DEV_T1_USB_MASK		(DEV_USB_OTG | DEV_USB_CHG | DEV_USB)
#define DEV_T1_UART_MASK	(DEV_UART)
#define DEV_T1_CHARGER_MASK	(DEV_DEDICATED_CHG | DEV_CAR_KIT)
#define MANSW1_OPEN_RUSTPROOF	((0x0 << 5)| (0x3 << 2) |(1 << 0))

/* Charger Type Register */
#define CHG_SDP_TIMEOUT			(1 << 3)
#define CHG_TYPE_SDP			(1 << 2)
#define CHG_TYPE_CDP			(1 << 1)
#define CHG_TYPE_DCP			(1 << 0)
#define DEV_T3_CHARGER_MASK	(CHG_TYPE_SDP | CHG_TYPE_CDP | \
				CHG_TYPE_DCP)
#define DEV_LCABLE_MASK		(CHG_TYPE_DCP | CHG_TYPE_SDP)

#define DEV_LANHUB		(1 << 9)
#define DEV_AUDIO_DOCK		(1 << 8)

/* Device Type 2 */
#define DEV_JIG_UART_OFF	(1 << 3)
#define DEV_JIG_UART_ON		(1 << 2)
#define DEV_JIG_USB_OFF		(1 << 1)
#define DEV_JIG_USB_ON		(1 << 0)

#define DEV_T2_USB_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON)
#define DEV_T2_UART_MASK	(DEV_JIG_UART_OFF)
#define DEV_T2_JIG_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON | \
				DEV_JIG_UART_OFF)
#define DEV_T2_JIG_ALL_MASK	(DEV_JIG_USB_OFF | DEV_JIG_USB_ON | \
				DEV_JIG_UART_OFF | DEV_JIG_UART_ON)

/*
 * Manual Switch 1
 * D- [7:5] / D+ [4:2]
 * 000: Open all / 001: USB / 011: UART
 */
#define SW_UART			((3 << 5) | (3 << 2))
#define SW_DHOST		((1 << 5) | (1 << 2) | (1 << 0))
#define SW_AUTO			((0 << 5) | (0 << 2))
#define SW_USB_OPEN		(1 << 0)
#define SW_ALL_OPEN		(0)
#define SW_ALL_OPEN_WITH_VBUS	((0 << 5) | (0 << 2) | (1 << 0))

/* Interrupt 1 */
#define INT_ADC_CHANGE			(1 << 6)
#define INT_CONNECT			(1 << 5)
#define INT_OVP				(1 << 4)
#define INT_DCD_OUT			(1 << 3)
#define INT_CHGDET			(1 << 2)
#define INT_DETACH			(1 << 1)
#define INT_ATTACH			(1 << 0)

/* Interrupt 2 */
#define INT_OVP_OCP_EVENT		(1 << 7)
#define INT_OCP_EVENT			(1 << 6)
#define INT_VBUS_OCP			(1 << 5)
#define INT_VBUS_OVP			(1 << 4)
#define INT_POR				(1 << 2)
#define INT_VBUS_INVALID		(1 << 1)
#define INT_RID_CHARGER			(1 << 0)

/* ADC VALUE */
#define	ADC_OTG				0x00
#define	ADC_MHL				0x01
#define ADC_VZW_DOCK			0x0E
#define ADC_VZW_INCOMPATIBLE		0x0F
#define ADC_SMART_DOCK			0x10
#define ADC_HMT				0x11
#define ADC_AUDIO_DOCK			0x12
#define ADC_LANHUB			0x13
#define ADC_CHARGING_CABLE		0x14
#define ADC_MPOS			0x15
#define ADC_UART			0x16
#define ADC_LCABLE			0x17
#define	ADC_JIG_USB_OFF			0x18
#define	ADC_JIG_USB_ON			0x19
#define	ADC_DESKDOCK			0x1a
#define ADC_CEA2			0x1b
#define	ADC_JIG_UART_OFF		0x1c
#define	ADC_JIG_UART_ON			0x1d
#define	ADC_CARDOCK			0x1d
#define	ADC_OPEN			0x1f

int uart_sm5504_connecting;
EXPORT_SYMBOL(uart_sm5504_connecting);
int detached_sm5504_status;
EXPORT_SYMBOL(detached_sm5504_status);
static int jig_state;

struct sm5504_usbsw {
	struct i2c_client		*client;
	struct sm5504_platform_data	*pdata;
	int				dev1;
	int				dev2;
	int				dev3;
	int				mansw;
	int				vbus;
	int				dock_attached;
	int				dev_id;
	struct delayed_work		init_work;
	struct mutex			mutex;
	int				adc;
	bool				incomp_attached;
#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
	unsigned int			previous_dock;
	unsigned int			lanhub_ta_status;
#endif
#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
	bool				is_rustproof;
#endif
};

static struct sm5504_usbsw *local_usbsw;

static int sm5504_attach_dev(struct sm5504_usbsw *usbsw);
static int sm5504_detach_dev(struct sm5504_usbsw *usbsw);

#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)

bool lanhub_ta_case = false;

/* RAW DATA Detection*/
static void sm5504_enable_rawdataInterrupts(struct sm5504_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	u8 value, ret;

	value = i2c_smbus_read_byte_data(client, REG_CONTROL);
	value &= 0xF7;	/*Control Register Bit 4 set to 0 to enable RAW Data INTR*/

	ret = i2c_smbus_write_byte_data(client, REG_CONTROL, value);
	pr_info("%s:set CONTROL value to 0x%x\n", __func__, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

}

static void sm5504_disable_rawdataInterrupts(struct sm5504_usbsw *usbsw)
{
	struct i2c_client *client = local_usbsw->client;
	int value, ret;

	value = i2c_smbus_read_byte_data(client, REG_CONTROL);
	value |= 0x08;  /*Control Register Bit 4 set to 0 to enable RAW Data INTR*/

	ret = i2c_smbus_write_byte_data(client, REG_CONTROL, value);
	pr_info("%s:set CONTROL value to 0x%x \n", __func__, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

}
#endif

static int sm5504_write_reg(struct i2c_client *client, int reg, int val)
{
        int ret;
        ret = i2c_smbus_write_byte_data(client, reg, val);
        if (ret < 0)
        {
                dev_err(&client->dev,
                        "%s, i2c write error %d\n",__func__, ret);
        }
        return ret;
}

static int sm5504_read_reg(struct i2c_client *client, int reg)
{
        int ret;
        ret = i2c_smbus_read_byte_data(client, reg);
        if (ret < 0)
        {
                dev_err(&client->dev,
                        "%s, i2c read error %d\n",__func__, ret);
        }
        return ret;
}

#if defined(CONFIG_SEC_FACTORY) || defined(CONFIG_USB_HOST_NOTIFY)
static void sm5504_dock_control(struct sm5504_usbsw *usbsw,
	int dock_type, int state, int path)
{
	struct i2c_client *client = usbsw->client;
	struct sm5504_platform_data *pdata = usbsw->pdata;
	int ret;

	if (state) {
		usbsw->mansw = path;
		pdata->callback(dock_type, state);
		ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW1, path);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_read_byte_data(client, REG_CONTROL);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
		else {
			ret = i2c_smbus_write_byte_data(client,
					REG_CONTROL, ret & ~CON_MANUAL_SW);
		}
		if (ret < 0)
			dev_err(&client->dev, "%s: err %x\n", __func__, ret);
	} else {
		pdata->callback(dock_type, state);
		ret = i2c_smbus_read_byte_data(client, REG_CONTROL);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_write_byte_data(client, REG_CONTROL,
			ret | CON_MANUAL_SW);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
	}
}
#endif

static void sm5504_reg_init(struct sm5504_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	unsigned int ctrl = CON_MASK;
	int ret;

	pr_info("sm5504_reg_init is called\n");

	usbsw->dev_id = i2c_smbus_read_byte_data(client, REG_DEVICE_ID);
	local_usbsw->dev_id = usbsw->dev_id;
	if (usbsw->dev_id < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, usbsw->dev_id);

	dev_info(&client->dev, " sm5504_reg_init dev ID: 0x%x\n",
			usbsw->dev_id);

	ret = i2c_smbus_write_byte_data(client, REG_INT_MASK1, INT_MASK1);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	ret = i2c_smbus_write_byte_data(client,	REG_INT_MASK2, INT_MASK2);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	ret = i2c_smbus_write_byte_data(client, REG_CONTROL, ctrl);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	/* Single scan for JIG Accessory */
	ret = i2c_smbus_write_byte_data(client, REG_RESERVED_1, 0x06);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	/*Manual SW2 bit2 : JIG_ON '1' */
	ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW2, 0x04);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);
	bcd_scan = 0;
}

static ssize_t sm5504_show_control(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sm5504_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, REG_CONTROL);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	return snprintf(buf, 13, "CONTROL: %02x\n", value);
}

static ssize_t sm5504_show_device_type(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sm5504_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE1);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	return snprintf(buf, 11, "DEV_TYP %02x\n", value);
}

static ssize_t sm5504_show_manualsw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sm5504_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, REG_MANUAL_SW1);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if (value == SW_UART)
		return snprintf(buf, 5, "UART\n");
	else if (value == SW_DHOST)
		return snprintf(buf, 6, "DHOST\n");
	else if (value == SW_AUTO)
		return snprintf(buf, 5, "AUTO\n");
	else
		return snprintf(buf, 4, "%x", value);
}

static ssize_t sm5504_set_manualsw(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct sm5504_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value, ret;
	unsigned int path = 0;

	value = i2c_smbus_read_byte_data(client, REG_CONTROL);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if ((value & ~CON_MANUAL_SW) !=
			(CON_SWITCH_OPEN))
		return 0;

	if (!strncmp(buf, "UART", 4)) {
		path = SW_UART;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "DHOST", 5)) {
		path = SW_DHOST;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "AUTO", 4)) {
		path = SW_AUTO;
		value |= CON_MANUAL_SW;
	} else {
		dev_err(dev, "Wrong command\n");
		return 0;
	}

	usbsw->mansw = path;

	ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW1, path);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	ret = i2c_smbus_write_byte_data(client, REG_CONTROL, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return count;
}
static ssize_t sm5504_show_usb_state(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sm5504_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int device_type1, device_type2;

	device_type1 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE1);
	if (device_type1 < 0) {
		dev_err(&client->dev, "%s: err %d ", __func__, device_type1);
		return (ssize_t)device_type1;
	}
	device_type2 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE2);
	if (device_type2 < 0) {
		dev_err(&client->dev, "%s: err %d ", __func__, device_type2);
		return (ssize_t)device_type2;
	}

	if (device_type1 & DEV_T1_USB_MASK || device_type2 & DEV_T2_USB_MASK)
		return snprintf(buf, 22, "USB_STATE_CONFIGURED\n");

	return snprintf(buf, 25, "USB_STATE_NOTCONFIGURED\n");
}

static ssize_t sm5504_show_adc(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sm5504_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int adc;

	adc = i2c_smbus_read_byte_data(client, REG_ADC);
	if (adc < 0) {
		dev_err(&client->dev,
			"%s: err at read adc %d\n", __func__, adc);
		return snprintf(buf, 9, "UNKNOWN\n");
	}

	return snprintf(buf, 4, "%x\n", adc);
}

static ssize_t sm5504_reset(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct sm5504_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	if (!strncmp(buf, "1", 1)) {
		dev_info(&client->dev,
			"sm5504 reset after delay 1000 msec.\n");
		msleep(1000);
		sm5504_write_reg(client, REG_RESET, 0x01);

	dev_info(&client->dev, "sm5504_reset_control done!\n");
	} else {
		dev_info(&client->dev,
			"sm5504_reset_control, but not reset_value!\n");
	}

#ifdef CONFIG_MUIC_SUPPORT_RUSTPROOF
	usbsw->is_rustproof = false;
#endif

	sm5504_reg_init(usbsw);

	return count;
}

#ifdef CONFIG_MUIC_SUPPORT_RUSTPROOF
static void muic_rustproof_feature(struct i2c_client *client, int state);
/* Keystring "*#0*#" sysfs implementation */
static ssize_t uart_en_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sm5504_usbsw *usbsw = dev_get_drvdata(dev);
	/*is_rustproof is false then UART can be enabled*/
	return snprintf(buf, 4, "%d\n", !(usbsw->is_rustproof));
}

static ssize_t uart_en_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct sm5504_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	if (!strncmp(buf, "1", 1)) {
		dev_info(&client->dev,
			"[MUIC]Runtime enabling the UART.\n");
		usbsw->is_rustproof = false;
		muic_rustproof_feature(client,SM5504_DETACHED);

	} else {
		dev_info(&client->dev,
			"[MUIC]Runtime disabling the UART.\n");
		usbsw->is_rustproof = true;
	}
	/* reinvoke the attach detection function to set proper paths */
	sm5504_attach_dev(usbsw);

	return size;
}

static DEVICE_ATTR(uart_en, S_IRUGO | S_IWUSR ,
				uart_en_show, uart_en_store);

static ssize_t uart_sel_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	/*for sm5504 paths are always switch to AP*/
	return snprintf(buf, 3, "AP\n");
}

static ssize_t uart_sel_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct sm5504_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	dev_info(&client->dev,"[MUIC]Enabling AP UART Path, dummy Call\n");
	return size;
}

static DEVICE_ATTR(uart_sel, S_IRUGO | S_IWUSR ,
				uart_sel_show, uart_sel_store);

static ssize_t usbsel_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 4, "PDA\n");
}

static ssize_t usbsel_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct sm5504_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	dev_info(&client->dev,"[MUIC]Enabling AP UART Path, dummy Call\n");
	return size;
}

static DEVICE_ATTR(usb_sel, S_IRUGO | S_IWUSR ,
				usbsel_show, usbsel_store);
#endif

static DEVICE_ATTR(control, S_IRUGO, sm5504_show_control, NULL);
static DEVICE_ATTR(device_type, S_IRUGO, sm5504_show_device_type, NULL);
static DEVICE_ATTR(switch, S_IRUGO | S_IWUSR,
		sm5504_show_manualsw, sm5504_set_manualsw);
static DEVICE_ATTR(usb_state, S_IRUGO, sm5504_show_usb_state, NULL);
static DEVICE_ATTR(adc, S_IRUGO, sm5504_show_adc, NULL);
static DEVICE_ATTR(reset_switch, S_IWUSR | S_IWGRP, NULL, sm5504_reset);

static struct attribute *sm5504_attributes[] = {
	&dev_attr_control.attr,
	&dev_attr_device_type.attr,
	&dev_attr_switch.attr,
	NULL
};

static const struct attribute_group sm5504_group = {
	.attrs = sm5504_attributes,
};


#if defined(CONFIG_USB_HOST_NOTIFY)
static void sm5504_set_otg(struct sm5504_usbsw *usbsw, int state)
{
	int ret;
	struct i2c_client *client = usbsw->client;

	if (state == SM5504_ATTACHED) {
		ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW1, 0x25);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW2, 0x00); //Disconnecting the MUIC_ID & ITBP Pins
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_read_byte_data(client, REG_CONTROL);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		ret = ret & 0xFB; /*Manual Connection S/W enable*/
		ret = i2c_smbus_write_byte_data(client, REG_CONTROL, ret);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
	} else {
		ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW2, 0x00);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW1,
				SW_ALL_OPEN);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_read_byte_data(client, REG_CONTROL);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		ret = ret | 0x04; /*Manual Connection S/W Disable*/
		ret = i2c_smbus_write_byte_data(client, REG_CONTROL, ret);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
	}
}
#endif

#if defined(CONFIG_VIDEO_MHL_V2)
int dock_det(void)
{
	return local_usbsw->dock_attached;
}
EXPORT_SYMBOL(dock_det);
#endif

int check_sm5504_jig_state(void)
{
	return jig_state;
}
EXPORT_SYMBOL(check_sm5504_jig_state);

#if defined(CONFIG_TOUCHSCREEN_IST30XX)
extern void charger_enable(int enable);
#endif

#if defined(CONFIG_TOUCHSCREEN_MMS144)
extern void tsp_charger_infom(bool en);
#endif

#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
static void sm5504_set_lanhub(struct sm5504_usbsw *usbsw, int state)
{
        int ret;
        struct i2c_client *client = usbsw->client;

        if (state == SM5504_ATTACHED) {
                ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW1, 0x25);
                if (ret < 0)
                        dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		/*Disconnect the ITBP & MUIC_ID Pins*/
		ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW2, 0x00);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_read_byte_data(client, REG_CONTROL);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		ret = ret & 0xFB; /*Manual Connection S/W enable*/
                ret = i2c_smbus_write_byte_data(client, REG_CONTROL, ret);
                if (ret < 0)
                        dev_info(&client->dev, "%s: err %d\n", __func__, ret);
        } else {
		ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW2, 0x00);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
                ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW1,
                                SW_ALL_OPEN);
                if (ret < 0)
                        dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_read_byte_data(client, REG_CONTROL);
		if (ret < 0)
			dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		ret = ret | 0x04; /*Manual Connection S/W enable*/
                ret = i2c_smbus_write_byte_data(client, REG_CONTROL, ret);
                if (ret < 0)
                        dev_info(&client->dev, "%s: err %d\n", __func__, ret);
        }
}

static void sm5504_mask_vbus_detect(struct sm5504_usbsw *usbsw, int state){
	unsigned int value;
	struct i2c_client *client = usbsw->client;
	if(state == SM5504_ATTACHED) {
		pr_info("%s called, state: (%d)\n",__func__,state);
	/*Need to disable the vbus change interrupts*/
		value = i2c_smbus_read_byte_data(client,REG_INT_MASK2);
		if (value < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, value);
		value |= 0x01;
		value = i2c_smbus_write_byte_data(client,REG_INT_MASK2,value);
		if (value < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, value);
	}
	else {
		pr_info("%s called, state: (%d)\n",__func__,state);

		value = i2c_smbus_write_byte_data(client,REG_INT_MASK2,INT_MASK2);
		if (value < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, value);
	}

}

static int sm5504_detect_lanhub(struct sm5504_usbsw *usbsw) {
	unsigned int dev1, dev2, adc;
	struct sm5504_platform_data *pdata = usbsw->pdata;
	struct i2c_client *client = usbsw->client;
	pr_info("%s called\n",__func__);

	dev1 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE1);
        if (dev1 < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, dev1);
		return dev1;
	}

	dev2 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE2);
        if (dev2 < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, dev2);
                return dev2;
	}

	adc = i2c_smbus_read_byte_data(client, REG_ADC);


	dev_info(&client->dev, "dev1: 0x%02x, dev2: 0x%02x, adc: 0x%02x\n",
			dev1, dev2, adc);

	/* Attached + Detached*/
	switch(adc){
	case ADC_OTG:
		lanhub_ta_case = false;
		usbsw->adc = adc;
		sm5504_set_otg(usbsw, SM5504_ATTACHED);
		if(usbsw->previous_dock == SM5504_NONE) {
			dev_info(&client->dev, "%s:LANHUB Connected\n", __func__);
			pdata->callback(CABLE_TYPE_OTG, SM5504_ATTACHED);
			sm5504_set_otg(usbsw,SM5504_ATTACHED);
		}
		else if(usbsw->previous_dock == ADC_LANHUB) {
			dev_info(&client->dev, "%s:Switch LANHUB+TA to LANHUB\n", __func__);
			usbsw->lanhub_ta_status=0;
			pdata->lanhub_cb(CABLE_TYPE_LANHUB, SM5504_DETACHED, LANHUB_TA);
		}
		sm5504_mask_vbus_detect(usbsw,SM5504_DETACHED);
		usbsw->dock_attached = SM5504_ATTACHED;
		usbsw->previous_dock = ADC_OTG;
		break;
	case ADC_LANHUB:
		usbsw->adc = adc;
		lanhub_ta_case = true;
		usbsw->lanhub_ta_status = 1;
		sm5504_mask_vbus_detect(usbsw,SM5504_ATTACHED);
		usbsw->dock_attached = SM5504_ATTACHED;
		usbsw->mansw = SW_DHOST;
		if(usbsw->previous_dock == SM5504_NONE) {
			dev_info(&client->dev, "%s:LANHUB+TA Connected\n", __func__);
			pdata->lanhub_cb(CABLE_TYPE_LANHUB, SM5504_ATTACHED, LANHUB);
			sm5504_set_lanhub(usbsw,SM5504_ATTACHED);
		} else if (usbsw->previous_dock == ADC_OTG) {
			dev_info(&client->dev, "%s:Switch LANHUB to LANHUB+TA\n", __func__);
			pdata->lanhub_cb(CABLE_TYPE_LANHUB, SM5504_ATTACHED, LANHUB_TA);
		}
		usbsw->previous_dock = ADC_LANHUB;
		break;
	case ADC_OPEN:
		usbsw->adc = adc;
		dev_info(&client->dev, "%s:LANHUB + TA -> ADC_OPEN case\n", __func__);
		if (pdata->lanhub_cb && usbsw->lanhub_ta_status == 1) {
			if (usbsw->previous_dock == ADC_LANHUB)
				pdata->lanhub_cb(CABLE_TYPE_LANHUB,SM5504_DETACHED, LANHUB);
			sm5504_disable_rawdataInterrupts(usbsw);
			usbsw->lanhub_ta_status=0;
			sm5504_mask_vbus_detect(usbsw,SM5504_DETACHED);
			pdata->callback(CABLE_TYPE_OTG,SM5504_DETACHED);
			sm5504_set_otg(usbsw, SM5504_DETACHED);
			}
		else if (usbsw->previous_dock == ADC_OTG) {
			sm5504_disable_rawdataInterrupts(usbsw);
			pdata->callback(CABLE_TYPE_OTG,SM5504_DETACHED);
			sm5504_set_otg(usbsw, SM5504_DETACHED);
		}else {
			dev_info(&client->dev, "%s:ignore ADC_OPEN case\n", __func__);
				sm5504_mask_vbus_detect(usbsw,SM5504_DETACHED);
			}

		usbsw->previous_dock = SM5504_NONE;
		lanhub_ta_case = false;
		break;
	default:
		dev_info(&client->dev, "%s:Not reaching here(adc:0x%02x)\n",
			__func__, adc);
		lanhub_ta_case = false;
		break;
	}

	usbsw->dev1=dev1;
	usbsw->dev2=dev2;
	return adc;
}
#endif

#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
static void muic_rustproof_feature(struct i2c_client *client, int state)
{
	int val;
	if(state) {
		val = i2c_smbus_write_byte_data(client, REG_MANUAL_SW1,
							SW_ALL_OPEN_WITH_VBUS);
		if(val < 0)
			dev_info(&client->dev, "%s:MANUAL SW1,err %d\n",__func__,val);
		val = i2c_smbus_read_byte_data(client,REG_CONTROL);
		if(val < 0)
			dev_info(&client->dev, "%s:CTRL REG,err %d\n",__func__,val);
		val &= 0xFB;
		val = i2c_smbus_write_byte_data(client,REG_CONTROL,val);
		if(val < 0)
			dev_info(&client->dev, "%s:CTRL REG,err %d\n",__func__,val);
	} else
	{
		val = i2c_smbus_write_byte_data(client, REG_MANUAL_SW2, 0x00);
		if (val < 0)
			dev_info(&client->dev, "%s: MANUAL SW2,err %d\n", __func__,val);
                val = i2c_smbus_write_byte_data(client, REG_MANUAL_SW1,SW_ALL_OPEN);
                if (val < 0)
                        dev_info(&client->dev, "%s: MANUAL SW1,err %d\n", __func__,val);
		val = i2c_smbus_read_byte_data(client, REG_CONTROL);
		if (val < 0)
			dev_info(&client->dev, "%s: CTRL REG,err %d\n", __func__,val);
		val = val | 0x04; /*Automatic Connection S/W enable*/
                val = i2c_smbus_write_byte_data(client, REG_CONTROL, val);
                if (val < 0)
                        dev_info(&client->dev, "%s: CTRL REG,err %d\n", __func__,val);

	}
}
#endif

static int sm5504_attach_dev(struct sm5504_usbsw *usbsw)
{
	int adc;
	int val1, val2, val3;
	struct sm5504_platform_data *pdata = usbsw->pdata;
	struct i2c_client *client = usbsw->client;
	int regCtrl1;
#if defined(CONFIG_VIDEO_MHL_V2)
	/*u8 mhl_ret = 0;*/
#endif
#if defined(CONFIG_TOUCHSCREEN_MMS144) || defined(CONFIG_TOUCHSCREEN_IST30XX)
	int tsp_noti_ignore = 0;
#endif
	val1 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE1);
	if (val1 < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, val1);
		return val1;
	}

	val2 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE2);
	if (val2 < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, val2);
		return val2;
	}
	jig_state =  (val2 & DEV_T2_JIG_ALL_MASK) ? 1 : 0;

	val3 = i2c_smbus_read_byte_data(client, REG_CHG_TYPE);
	if (val3 < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, val3);
		return val3;
	}

	adc = i2c_smbus_read_byte_data(client, REG_ADC);

	switch (adc) {
	case (ADC_VZW_DOCK)...(ADC_MPOS):
	case ADC_DESKDOCK:
		pr_info("%s,[SM5504 MUIC] Unsupported Accessory!\n", __func__);
		goto attach_end;
		break;
	}
#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
	else if (adc == ADC_LANHUB) {
	val2 = DEV_LANHUB;
	val1 = 0;
	}
#endif

#if defined(CONFIG_USB_HOST_NOTIFY)
	if (adc == 0x11 || adc == ADC_AUDIO_DOCK) {
		val2 = DEV_AUDIO_DOCK;
		val1 = 0;
	}
#endif
	dev_err(&client->dev,
			"dev1: 0x%x,dev2: 0x%x,chg_typ: 0x%x,vbus %d,ADC: 0x%x,Jig: %s\n",
			val1, val2, val3, usbsw->vbus, adc,
			(check_sm5504_jig_state() ? "ON" : "OFF"));
	if (bcd_scan) {
		bcd_scan = 0;
		pr_info("BCD Scan end val3 = 0x%x\n", val3);
		if (val3 == CHG_TYPE_DCP)
			pdata->callback(CABLE_TYPE_AC, SM5504_ATTACHED);
		else
			pdata->callback(CABLE_TYPE_USB, SM5504_ATTACHED);
	} else {
		/* USB */
		if (val1 & DEV_USB || val2 & DEV_T2_USB_MASK ) {
			pr_info("[SM5504 MUIC] USB Connected\n");
			pdata->callback(CABLE_TYPE_USB, SM5504_ATTACHED);
			/* USB_CDP */
		} else if (val1 & DEV_USB_CHG) {
			pr_info("[MUIC] CDP Connected\n");
			pdata->callback(CABLE_TYPE_CDP, SM5504_ATTACHED);
			/* UART */
		} else if (val1 & DEV_T1_UART_MASK || val2 & DEV_T2_UART_MASK) {
			uart_sm5504_connecting = 1;
#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
			if(usbsw->is_rustproof) {
				pr_info("[MUIC] RustProof mode, close UART Path\n");
				muic_rustproof_feature(client,SM5504_ATTACHED);
			} else
#endif
			{
				pr_info("[MUIC] UART Connected\n");
				i2c_smbus_write_byte_data(client, REG_MANUAL_SW1, SW_UART);
				if(usbsw->vbus)
					pdata->callback(CABLE_TYPE_JIG_UART_OFF_VB, SM5504_ATTACHED);
				else
					pdata->callback(CABLE_TYPE_UARTOFF, SM5504_ATTACHED);
				i2c_smbus_write_byte_data(usbsw->client, REG_CONTROL, CON_ADC_MASK);
			}
			/* L Company Cable Detection Code */
		} else if (adc == ADC_LCABLE) {
			pr_info("[MUIC] 219K USB Cable/Charger Connected\n");
			// BCD scan start
			regCtrl1 = i2c_smbus_read_byte_data(client,0x02);
			if (regCtrl1 < 0) {
				pr_info("I2C read error regCtrl1 = 0x%02x\n",regCtrl1);
				return adc;
			}
			pr_info("reg_ctrl1 = 0x%x\n", regCtrl1);
			regCtrl1 &= (~CON_USBCHDEN);

			i2c_smbus_write_byte_data(client,0x02, regCtrl1);
			pr_info("reg_ctrl1 = 0x%x\n", regCtrl1);

			msleep(1);

			regCtrl1 |= (CON_USBCHDEN);
			i2c_smbus_write_byte_data(client,0x02, regCtrl1);
			pr_info("reg_ctrl1 = 0x%x\n", regCtrl1);

			bcd_scan = 1;
			pr_info("[MUIC] BCD Scan start \n");

			return adc;
			/* CHARGER */
		} else if ((val1 & DEV_T1_CHARGER_MASK) ||
				(val3 & DEV_T3_CHARGER_MASK)) {
			pr_info("[MUIC] Charger Connected\n");
			pdata->callback(CABLE_TYPE_AC, SM5504_ATTACHED);
#if defined(CONFIG_USB_HOST_NOTIFY)
			/* for SAMSUNG OTG */
		} else if (val1 & DEV_USB_OTG && adc == ADC_OTG) {
			pr_info("[MUIC] OTG Connected\n");
#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
			sm5504_enable_rawdataInterrupts(usbsw);
			usbsw->dock_attached = SM5504_ATTACHED;
			usbsw->previous_dock = ADC_OTG;
#endif
			sm5504_set_otg(usbsw, SM5504_ATTACHED);
			pdata->callback(CABLE_TYPE_OTG, SM5504_ATTACHED);
#endif
			/* JIG */
		} else if (val2 & DEV_T2_JIG_MASK) {
			pr_info("[MUIC] JIG Connected\n");
			pdata->callback(CABLE_TYPE_JIG, SM5504_ATTACHED);
#if defined(CONFIG_VIDEO_MHL_V2)
			/* MHL */
		} else if (val3 & DEV_MHL) {
			pr_info("[MUIC] MHL Connected\n");
			if (!poweroff_charging)
				/*mhl_ret = mhl_onoff_ex(1); support from sii8240*/
			else
				pr_info("LPM mode, skip MHL sequence\n");
#endif
			/* Car Dock */
		} else if (val2 & DEV_JIG_UART_ON) {
			pr_info("[MUIC] Cardock Connected\n");
#if defined(CONFIG_SEC_FACTORY)
			local_usbsw->dock_attached = SM5504_ATTACHED;
			sm5504_dock_control(usbsw, CABLE_TYPE_CARDOCK,
					SM5504_ATTACHED, SW_UART);
#elif defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
			if(usbsw->is_rustproof) {
				pr_info("[MUIC] RustProof mode, close UART Path\n");
				muic_rustproof_feature(client,SM5504_ATTACHED);
			}
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
			/* Audio Dock */
		} else if (val2 & DEV_AUDIO_DOCK) {
			pr_info("[MUIC] Audiodock Connected\n");
			sm5504_dock_control(usbsw, CABLE_TYPE_AUDIO_DOCK,
					SM5504_ATTACHED, SW_DHOST);
#endif

#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
			/* LANHUB */
		} else if (val2 & DEV_LANHUB) {
			if(usbsw->previous_dock == ADC_LANHUB &&
					usbsw->lanhub_ta_status == 1 )
				/*Skip for the LANHUB+TA Case*/
				pr_info("[MUIC] Lanhub + TA is connected\n");
			else {
				/* Enable RAWDATA Interrupts */
				sm5504_enable_rawdataInterrupts(usbsw);
				sm5504_detect_lanhub(usbsw);
			}
#endif
			/* Incompatible */
		} else if (usbsw->vbus) {
			pr_info("[MUIC] Incompatible Charger Connected\n");
			pdata->callback(CABLE_TYPE_INCOMPATIBLE,
					SM5504_ATTACHED);
			usbsw->incomp_attached = true;
		}
#if defined(CONFIG_TOUCHSCREEN_MMS144) || defined(CONFIG_TOUCHSCREEN_IST30XX)
		else{
			tsp_noti_ignore = 1;
			printk("[TSP] detached, but don't noti \n");
		}
		if(!tsp_noti_ignore)
#if defined(CONFIG_TOUCHSCREEN_MMS144) 
			tsp_charger_infom(1);
#else
		charger_enable(1);
#endif
#endif
	}

attach_end:
	usbsw->dev1 = val1;
	usbsw->dev2 = val2;
	usbsw->dev3 = val3;
	usbsw->adc = adc;

	return adc;
}

static int sm5504_detach_dev(struct sm5504_usbsw *usbsw)
{
	struct sm5504_platform_data *pdata = usbsw->pdata;
#if defined(CONFIG_TOUCHSCREEN_MMS144) || defined(CONFIG_TOUCHSCREEN_IST30XX)
	int tsp_noti_ignore = 0;

#endif
	pr_err("%s\n", __func__);
	pr_err("dev1: 0x%x,dev2: 0x%x,chg_typ: 0x%x,vbus %d,ADC: 0x%x,Jig: %s\n",
			usbsw->dev1, usbsw->dev2, usbsw->dev3, usbsw->vbus, usbsw->adc,
			(check_sm5504_jig_state() ? "ON" : "OFF"));
	switch (usbsw->adc) {
	case (ADC_VZW_DOCK)...(ADC_MPOS):
	case ADC_DESKDOCK:
		pr_info("%s,[SM5504 MUIC] Unsupported Accessory!\n", __func__);
		goto detach_end;
		break;
	}

	/* USB */
	if (usbsw->dev1 & DEV_USB ||
			usbsw->dev2 & DEV_T2_USB_MASK) {
		pr_info("[MUIC] USB Disonnected\n");
		pdata->callback(CABLE_TYPE_USB, SM5504_DETACHED);
	} else if (usbsw->dev1 & DEV_USB_CHG) {
		pdata->callback(CABLE_TYPE_CDP, SM5504_DETACHED);

	/* UART */
	} else if (usbsw->dev1 & DEV_T1_UART_MASK ||
			usbsw->dev2 & DEV_T2_UART_MASK) {
#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
		if(usbsw->is_rustproof) {
                        pr_info("[MUIC] RustProof mode Disconnected Event\n");
			muic_rustproof_feature(usbsw->client,SM5504_DETACHED);
                } else
#endif
		{
		pr_info("[MUIC] UART Disonnected\n");
		if(usbsw->vbus)
		pdata->callback(CABLE_TYPE_JIG_UART_OFF_VB, SM5504_DETACHED);
		else
		pdata->callback(CABLE_TYPE_UARTOFF, SM5504_DETACHED);
		uart_sm5504_connecting = 0;
		}
	/* L Company Cable Detection Code */
	} else if (usbsw->adc == ADC_LCABLE) {
		pr_info("[MUIC] 219K USB Cable/Charger Disconnected\n");
		if(usbsw->dev3 == CHG_TYPE_DCP)
			pdata->callback(CABLE_TYPE_AC,SM5504_DETACHED);
		else
			pdata->callback(CABLE_TYPE_USB, SM5504_DETACHED);
		/* CHARGER */
	} else if ((usbsw->dev1 & DEV_T1_CHARGER_MASK) ||
			(usbsw->dev3 & DEV_T3_CHARGER_MASK)) {
		pr_info("[MUIC] Charger Disconnected\n");
		pdata->callback(CABLE_TYPE_AC, SM5504_DETACHED);
#if defined(CONFIG_USB_HOST_NOTIFY)
	/* for SAMSUNG OTG */
	} else if (usbsw->dev1 & DEV_USB_OTG) {
		pr_info("[MUIC] OTG Disconnected\n");
#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
		sm5504_disable_rawdataInterrupts(usbsw);
		pr_info("%s:lanhub_ta_status(%d)\n",
					__func__, usbsw->lanhub_ta_status);
		lanhub_ta_case = false;
		if (usbsw->lanhub_ta_status == 0) {
			pdata->callback(CABLE_TYPE_OTG,SM5504_DETACHED);
			sm5504_set_otg(usbsw, SM5504_DETACHED);
			}
		else if (pdata->lanhub_cb && usbsw->lanhub_ta_status == 1)
			pdata->lanhub_cb(CABLE_TYPE_LANHUB,SM5504_DETACHED, LANHUB);
		usbsw->dock_attached = SM5504_DETACHED;
		usbsw->lanhub_ta_status=0;
#else
		sm5504_set_otg(usbsw, SM5504_DETACHED);
		pdata->callback(CABLE_TYPE_OTG, SM5504_DETACHED);
#endif
#endif
	/* JIG */
	} else if (usbsw->dev2 & DEV_T2_JIG_MASK) {
		pr_info("[MUIC] JIG Disonnected\n");
		pdata->callback(CABLE_TYPE_JIG, SM5504_DETACHED);
#if defined(CONFIG_MHL_D3_SUPPORT)
	/* MHL */
	} else if (usbsw->dev3 & DEV_MHL) {
		pr_info("[MUIC] MHL Disonnected\n");
		//mhl_onoff_ex(false);
		detached_sm5504_status = 1;
#endif
	/* Car Dock */
	} else if (usbsw->dev2 & DEV_JIG_UART_ON) {
		pr_info("[MUIC] Cardock Disconnected\n");
#if defined(CONFIG_SEC_FACTORY)
		local_usbsw->dock_attached = SM5504_DETACHED;
		sm5504_dock_control(usbsw, CABLE_TYPE_CARDOCK,
			SM5504_DETACHED, SW_ALL_OPEN);
#elif  defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
                if(usbsw->is_rustproof) {
                        pr_info("[MUIC] RustProof mode disconneted Event\n");
			muic_rustproof_feature(usbsw->client,SM5504_DETACHED);
                }
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
	/* Audio Dock */
	} else if (usbsw->dev2 == DEV_AUDIO_DOCK) {
		pr_info("[MUIC] Audiodock Disconnected\n");
		sm5504_dock_control(usbsw, CABLE_TYPE_AUDIO_DOCK,
			SM5504_DETACHED, SW_ALL_OPEN);
#endif
#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
	/* LANHUB */
	} else if (usbsw->adc == ADC_LANHUB) {
		pr_info("[MUIC] Lanhub disconnected\n");
		/* Disable RAWDATA Interrupts */
		sm5504_disable_rawdataInterrupts(usbsw);
		lanhub_ta_case = false;
		usbsw->dock_attached = SM5504_DETACHED;
		sm5504_set_lanhub(usbsw,SM5504_DETACHED);
		sm5504_mask_vbus_detect(usbsw, SM5504_DETACHED);
		pr_info("%s:lanhub_ta_status(%d)\n",__func__, usbsw->lanhub_ta_status);
		if (pdata->lanhub_cb && usbsw->lanhub_ta_status==1)
			pdata->lanhub_cb(CABLE_TYPE_LANHUB, SM5504_DETACHED, LANHUB);
		else if (usbsw->lanhub_ta_status == 0) {
			pdata->callback(CABLE_TYPE_OTG, SM5504_DETACHED);
			sm5504_set_otg(usbsw, SM5504_DETACHED);
			}
		usbsw->lanhub_ta_status=0;
		usbsw->dock_attached = SM5504_DETACHED;

#endif
	/* Incompatible */
	} else if (usbsw->incomp_attached) {
		pr_info("[MUIC] Incompatible Charger Disonnected\n");
		pdata->callback(CABLE_TYPE_INCOMPATIBLE,
			SM5504_DETACHED);
		usbsw->incomp_attached = false;
	}
#if defined(CONFIG_TOUCHSCREEN_MMS144) || defined(CONFIG_TOUCHSCREEN_IST30XX)
	else{
		tsp_noti_ignore = 1;
		printk("[TSP] detached, but don't noti \n");
	}
	if(!tsp_noti_ignore)
#if defined(CONFIG_TOUCHSCREEN_MMS144) 
		tsp_charger_infom(0);
#else
		charger_enable(0);
#endif
#endif

detach_end:
	i2c_smbus_write_byte_data(usbsw->client, REG_CONTROL, CON_MASK);

	usbsw->dev1 = 0;
	usbsw->dev2 = 0;
	usbsw->dev3 = 0;
	usbsw->adc = 0;

	bcd_scan = 0;

	return 0;

}
static irqreturn_t sm5504_irq_thread(int irq, void *data)
{
	struct sm5504_usbsw *usbsw = data;
	struct i2c_client *client = usbsw->client;
	int intr1, intr2;
	int val1, adc;
	/* SM5504 : Read interrupt -> Read Device */
	pr_info("sm5504_irq_thread is called\n");

	mutex_lock(&usbsw->mutex);
	intr1 = i2c_smbus_read_byte_data(client, REG_INT1);
	intr2 = i2c_smbus_read_byte_data(client, REG_INT2);

	adc = i2c_smbus_read_byte_data(client, REG_ADC);
	dev_info(&client->dev, "%s: intr1 : 0x%x,intr2 : 0x%x, adc : 0x%x\n",
					__func__, intr1, intr2, adc);

	/* MUIC OVP Check */
	if (intr1 & INT_OVP_OCP_EVENT)
		usbsw->pdata->oxp_callback(ENABLE);
	else if (intr1 & (~INT_OVP_OCP_EVENT))
		usbsw->pdata->oxp_callback(DISABLE);


	if (!(intr2 & INT_VBUS_INVALID)) {
		usbsw->vbus = 1;
	} else {
		usbsw->vbus = 0;
	}
#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
	if((intr1 == 0x00 && (intr2 & 0x04))
				|| ((intr1 & INT_DETACH) && (intr2 & 0x04)))
		sm5504_detect_lanhub(usbsw);
	else
#endif
	/* device detection */
	/* interrupt both attach and detach */
	if (intr1 == (INT_ATTACH + INT_DETACH)) {
		val1 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE1);

		if ((adc == ADC_OPEN) && (val1 == DATA_NONE) &&
			(usbsw->vbus == 0x00))
			sm5504_detach_dev(usbsw);
		else
			sm5504_attach_dev(usbsw);
        }
	/* interrupt attach */
	else if ( (intr1 & INT_ATTACH) || (intr2 & INT_RID_CHARGER) )
		sm5504_attach_dev(usbsw);
	/* interrupt detach */
	else if (intr1 & INT_DETACH)
		sm5504_detach_dev(usbsw);
	else if (!(intr2 & INT_VBUS_INVALID)) {
		pr_info("sm5504: VBUSOUT_ON\n");
#ifdef CONFIG_USB_HOST_NOTIFY
		sec_otg_notify(HNOTIFY_OTG_POWER_ON);
#endif
		if ((adc != ADC_OPEN) &&
			(sec_get_notification(HNOTIFY_MODE) != NOTIFY_TEST_MODE)) {
			sm5504_attach_dev(usbsw);
		} else {
			goto irq_end;
		}
	}
	else if (intr2 & INT_VBUS_INVALID) {
		pr_info("sm5504: VBUSOUT_OFF\n");
#ifdef CONFIG_USB_HOST_NOTIFY
                sec_otg_notify(HNOTIFY_OTG_POWER_OFF);
#endif
		if (sec_get_notification(HNOTIFY_MODE) != NOTIFY_TEST_MODE) {
			sm5504_detach_dev(usbsw);
		} else {
			goto irq_end;
		}
	}
irq_end:
	mutex_unlock(&usbsw->mutex);
	pr_info("sm5504_irq_thread,end\n");
	return IRQ_HANDLED;
}

static int sm5504_irq_init(struct sm5504_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int ret;

	if (client->irq) {
		ret = request_threaded_irq(client->irq, NULL,
			sm5504_irq_thread, IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			"sm5504 micro USB", usbsw);
		if (ret) {
			dev_err(&client->dev, "failed to reqeust IRQ\n");
			return ret;
		}
		enable_irq_wake(client->irq);
	}

	return 0;
}

static void sm5504_init_detect(struct work_struct *work)
{
	struct sm5504_usbsw *usbsw = container_of(work,
			struct sm5504_usbsw, init_work.work);
	int ret;
	int int_reg1, int_reg2;

	dev_info(&usbsw->client->dev, "%s\n", __func__);

	int_reg1 = sm5504_read_reg(usbsw->client, REG_INT1);
	dev_info(&usbsw->client->dev, "%s: intr1 : 0x%x\n",
		__func__, int_reg1);

	int_reg2 = i2c_smbus_read_byte_data(usbsw->client, REG_INT2);
	dev_info(&usbsw->client->dev, "%s: intr2 : 0x%x\n",
		__func__, int_reg2);

	if (!(int_reg2 & INT_VBUS_INVALID))
		usbsw->vbus = 1;
	else
		usbsw->vbus = 0;

	mutex_lock(&usbsw->mutex);
	sm5504_attach_dev(usbsw);
	mutex_unlock(&usbsw->mutex);

	ret = sm5504_irq_init(usbsw);
	if (ret)
		dev_info(&usbsw->client->dev,
				"failed to enable  irq init %s\n", __func__);

}

#ifdef CONFIG_OF
static int sm5504_parse_dt(struct device *dev, struct sm5504_platform_data *pdata)
{

        struct device_node *np = dev->of_node;
	/*changes can be added later, when needed*/
#if 0
        /* regulator info */
	pdata->i2c_pull_up = of_property_read_bool(np, "sm5504,i2c-pull-up");
#endif

        /* reset, irq gpio info */
        pdata->gpio_scl = of_get_named_gpio_flags(np, "sm5504,gpio-scl",
                               0, &pdata->scl_gpio_flags);
        pdata->gpio_uart_on = of_get_named_gpio_flags(np, "sm5504,uarton-gpio",
                               0, &pdata->uarton_gpio_flags);
        pdata->gpio_sda = of_get_named_gpio_flags(np, "sm5504,gpio-sda",
                               0, &pdata->sda_gpio_flags);
        pdata->gpio_int = of_get_named_gpio_flags(np, "sm5504,irq-gpio",
                0, &pdata->irq_gpio_flags);
	pr_info("%s: irq-gpio: %u \n", __func__, pdata->gpio_int);

        return 0;
}
#endif

static int sm5504_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct sm5504_usbsw *usbsw;
	int ret = 0;
	struct device *switch_dev;
	struct sm5504_platform_data *pdata;

	dev_info(&client->dev,"%s:sm5504 probe called \n",__func__);
	if(client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct sm5504_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory \n");
				return -ENOMEM;
		}
		ret = sm5504_parse_dt(&client->dev, pdata);
		if (ret < 0) {
			dev_err(&client->dev, "sm5502_parse_dt failed\n");
			goto fail;
		}

		pdata->callback = sm5504_callback;
#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
		pdata->lanhub_cb = sm5504_lanhub_callback;
#endif
		pdata->dock_init = sm5504_dock_init;
		pdata->oxp_callback = sm5504_oxp_callback;
		pdata->mhl_sel = NULL;

#if defined(CONFIG_GPIO_MSM_V3)
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_int,  0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_scl,  0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_sda,  0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_uart_on,  0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
#endif
		client->irq = gpio_to_irq(pdata->gpio_int);
	} else {
		pdata = client->dev.platform_data;
		if (!pdata)
			return -EINVAL;
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "i2c functionality check failed...!\n");
		ret = -EIO;
		goto fail;
	}

	usbsw = kzalloc(sizeof(struct sm5504_usbsw), GFP_KERNEL);
	if (!usbsw) {
		dev_err(&client->dev, "failed to allocate driver data\n");
		ret = -ENOMEM;
		goto fail;
	}

	usbsw->client = client;
	if (client->dev.of_node)
		usbsw->pdata = pdata;
	else
		usbsw->pdata = client->dev.platform_data;
	if (!usbsw->pdata)
		goto fail1;

	i2c_set_clientdata(client, usbsw);

	mutex_init(&usbsw->mutex);
	usbsw->incomp_attached = false;
	local_usbsw = usbsw;

#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
	ret = sm5504_read_reg(client,REG_MANUAL_SW1);
	if(ret<0) {
		dev_err(&client->dev, "failed to read MANUAL SW1 Reg, err:%d\n",ret);
	}
	/* Keep the feature disabled by default */
	usbsw->is_rustproof = false;

	dev_err(&client->dev, "MANUAL SW1 Reg :%d\n",ret);

	/* RUSTPROOF: disable UART connection if MANSW1 from BL is OPEN_RUSTPROOF*/
	if(ret == MANSW1_OPEN_RUSTPROOF)
		usbsw->is_rustproof = true;

#endif
	sm5504_reg_init(usbsw);

	ret = sysfs_create_group(&client->dev.kobj, &sm5504_group);
	if (ret) {
		dev_err(&client->dev,
				"failed to create sm5504 attribute group\n");
		goto fail2;
	}

	/* make sysfs node /sys/class/sec/switch/ */
	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_dev)) {
		pr_err("[SM5504] Failed to create device (switch_dev)!\n");
		ret = PTR_ERR(switch_dev);
		goto fail2;
	}

	ret = device_create_file(switch_dev, &dev_attr_usb_state);
	if (ret < 0) {
		pr_err("[SM5504] Failed to create file (usb_state)!\n");
		goto err_create_file_state;
	}

	ret = device_create_file(switch_dev, &dev_attr_adc);
	if (ret < 0) {
		pr_err("[SM5504] Failed to create file (adc)!\n");
		goto err_create_file_adc;
	}

	ret = device_create_file(switch_dev, &dev_attr_reset_switch);
	if (ret < 0) {
		pr_err("[SM5504] Failed to create file (reset_switch)!\n");
		goto err_create_file_reset_switch;
	}

#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
	ret = device_create_file(switch_dev, &dev_attr_uart_en);
	if (ret < 0) {
		pr_err("[SM5504] Failed to create file (uart_en)!\n");
		goto err_create_file_uart_en;
	}
	ret = device_create_file(switch_dev, &dev_attr_uart_sel);
	if (ret < 0) {
		pr_err("[SM5504] Failed to create file (uart_sel)!\n");
		goto err_create_file_uart_sel;
	}
	ret = device_create_file(switch_dev, &dev_attr_usb_sel);
	if (ret < 0) {
		pr_err("[SM5504] Failed to create file (usb_sel)!\n");
		goto err_create_file_usb_sel;
	}

#endif

#if defined(CONFIG_MUIC_SM5504_SUPPORT_LANHUB_TA)
	local_usbsw->previous_dock = SM5504_NONE;
	local_usbsw->lanhub_ta_status = 0;
#endif
	dev_set_drvdata(switch_dev, usbsw);
	/* sm5504 dock init*/
	if (usbsw->pdata->dock_init)
		usbsw->pdata->dock_init();

	/* initial cable detection */
	INIT_DELAYED_WORK(&usbsw->init_work, sm5504_init_detect);
	schedule_delayed_work(&usbsw->init_work, msecs_to_jiffies(2700));

	return 0;

#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
err_create_file_usb_sel:
	device_remove_file(switch_dev, &dev_attr_usb_sel);
err_create_file_uart_sel:
	device_remove_file(switch_dev, &dev_attr_uart_sel);
err_create_file_uart_en:
	device_remove_file(switch_dev, &dev_attr_uart_en);
#endif
err_create_file_reset_switch:
	device_remove_file(switch_dev, &dev_attr_reset_switch);
err_create_file_adc:
	device_remove_file(switch_dev, &dev_attr_adc);
err_create_file_state:
	device_remove_file(switch_dev, &dev_attr_usb_state);
fail2:
	if (client->irq)
		free_irq(client->irq, usbsw);
	mutex_destroy(&usbsw->mutex);
	i2c_set_clientdata(client, NULL);
fail1:
	kfree(usbsw);
fail:
	kfree(pdata);
	return ret;
}

static int sm5504_remove(struct i2c_client *client)
{
	struct sm5504_usbsw *usbsw = i2c_get_clientdata(client);
	cancel_delayed_work(&usbsw->init_work);
	if (client->irq) {
		disable_irq_wake(client->irq);
		free_irq(client->irq, usbsw);
	}
	mutex_destroy(&usbsw->mutex);
	i2c_set_clientdata(client, NULL);

	sysfs_remove_group(&client->dev.kobj, &sm5504_group);
	kfree(usbsw);
	return 0;
}

static int sm5504_resume(struct i2c_client *client)
{
	struct sm5504_usbsw *usbsw = i2c_get_clientdata(client);
	int ldev1, ldev2, intr;

	pr_info("%s: resume \n",__func__);

	ldev1 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE1);
	if (ldev1 < 0)
	{
		pr_err("%s: Dev reg 1 read err! %d \n",__func__, ldev1);
		goto safe_exit;
	}
	ldev2 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE2);
	if (ldev2 < 0)
	{
		pr_err("%s: Dev reg 2 read err! %d \n",__func__, ldev2);
		goto safe_exit;
	}

	intr = i2c_smbus_read_byte_data(client, REG_INT1);
	i2c_smbus_read_byte_data(client, REG_INT2);
	if (intr & INT_DETACH) {
		mutex_lock(&usbsw->mutex);
		sm5504_detach_dev(usbsw);
		mutex_unlock(&usbsw->mutex);
	} else if (usbsw->dev1 != ldev1 || usbsw->dev2 != ldev2) {
		/* device detection */
		mutex_lock(&usbsw->mutex);
		sm5504_attach_dev(usbsw);
		mutex_unlock(&usbsw->mutex);
	}

safe_exit:
	return 0;
}


static const struct i2c_device_id sm5504_id[] = {
	{"sm5504", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, sm5504_id);
static struct of_device_id sm5504_i2c_match_table[] = {
	{ .compatible = "sm5504,i2c",},
	{},
};
MODULE_DEVICE_TABLE(of, sm5504_i2c_match_table);

static struct i2c_driver sm5504_i2c_driver = {
	.driver = {
		.name = "sm5504",
		.owner = THIS_MODULE,
		.of_match_table = sm5504_i2c_match_table,
	},
	.probe = sm5504_probe,
	.remove = sm5504_remove,
	.resume = sm5504_resume,
	.id_table = sm5504_id,
};

static int __init sm5504_init(void)
{
	return i2c_add_driver(&sm5504_i2c_driver);
}
/*
 * Late init call is required MUIC was accessing
 * USB driver before USB initialization and watch dog reset
 * was happening when booted with USB connected
 */

late_initcall(sm5504_init);

static void __exit sm5504_exit(void)
{
	i2c_del_driver(&sm5504_i2c_driver);
}
module_exit(sm5504_exit);

MODULE_AUTHOR("Nitin Chaudhary <nc.chaudhary@samsung.com>");
MODULE_DESCRIPTION("SM5504 Micro USB Switch driver");
MODULE_LICENSE("GPL");
