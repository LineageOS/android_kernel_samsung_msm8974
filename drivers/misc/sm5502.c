/*
 * driver/misc/sm5502.c - SM5502 micro USB switch device driver
 *
 * Copyright (C) 2013 Samsung Electronics
 * Jeongrae Kim <jryu.kim@samsung.com>
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
/****************************** Revision History ******************************************
 *CH# Product		author		Description				Date
 *-----------------------------------------------------------------------------------------
 *01  Millet &		nc.chaudhary	Modified the ISR for AT+OTGGTEST=0,0	30-Jan-2014
 *    Matisse all			AT Command failure issue during SMD
 *    variants				test.
 *02  Millet & Matise	nc.chaudhary	Enabling the support for Cardock in	02-Feb-2014
 *    All Variants			Factory Images only.
 *03  Millet & Matise	nc.chaudhary	Modified the ISR for the VBUS-UART	02-Feb-2014
 *    All Variants			cable not working properly.
 *04  MSM8x26 & MSM8926 nc.chaudhary	Modified the ISR for OTG SMD Test fail	08-Feb-2014
 *    All variants			when checking OTG with UART AT Cmds.
 *05  MSM8x26 & MSM8926 nc.chaudhary	Modified the cable type in the deskdock	12-Feb-2014
 *    All variants			callback depending on VBUS presence.
 *06  MSM8x26 & MSM8926 nc.chaudhary	Modified ISR to report VBUS change	19-Feb-2014
 *    All variants			when desdock is connected.
 *07  MSM8x26 & MSM8926 nc.chaudhary	Implemented the LANHUB+TA case		28-Feb-2014
 *    All variants
 ******************************************************************************************
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/i2c/sm5502.h>
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
#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
#include <linux/sec_param.h>
#endif
#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif

/* spmi control */
extern int spmi_ext_register_writel_extra(u8 sid, u16 ad, u8 *buf, int len);
extern int spmi_ext_register_readl_extra(u8 sid, u16 ad, u8 *buf, int len);

extern int system_rev;

#define INT_MASK1					0x5C
#define INT_MASK2					0x20

/* DEVICE ID */
#define SM5502_DEV_ID				0x0A
#define SM5502_DEV_ID_REV			0x12

/* SM5502 I2C registers */
#define REG_DEVICE_ID				0x01
#define REG_CONTROL					0x02
#define REG_INT1					0x03
#define REG_INT2					0x04
#define REG_INT_MASK1				0x05
#define REG_INT_MASK2				0x06
#define REG_ADC						0x07
#define REG_TIMING_SET1				0x08
#define REG_TIMING_SET2				0x09
#define REG_DEVICE_TYPE1			0x0a
#define REG_DEVICE_TYPE2			0x0b
#define REG_BUTTON1					0x0c
#define REG_BUTTON2					0x0d
#define REG_MANUAL_SW1				0x13
#define REG_MANUAL_SW2				0x14
#define REG_DEVICE_TYPE3			0x15
#define REG_RESET					0x1B
#define REG_TIMER_SET				0x20
#define REG_VBUSINVALID		        0x1D
#define REG_OCP_SET			        0x22
#define REG_CHGPUMP_SET			    0x3A
#define REG_CARKIT_STATUS			0x0E

#define DATA_NONE					0x00

/* Control */
#define CON_SWITCH_OPEN		(1 << 4)
#define CON_RAW_DATA		(1 << 3)
#define CON_MANUAL_SW		(1 << 2)
#define CON_WAIT			(1 << 1)
#define CON_INT_MASK		(1 << 0)
#define CON_MASK		(CON_SWITCH_OPEN | CON_RAW_DATA | \
				CON_MANUAL_SW | CON_WAIT)

/* Device Type 1 */
#define DEV_USB_OTG			(1 << 7)
#define DEV_DEDICATED_CHG		(1 << 6)
#define DEV_USB_CHG			(1 << 5)
#define DEV_CAR_KIT			(1 << 4)
#define DEV_UART			(1 << 3)
#define DEV_USB				(1 << 2)
#define DEV_AUDIO_2			(1 << 1)
#define DEV_AUDIO_1			(1 << 0)

#define DEV_T1_USB_MASK		(DEV_USB_OTG | DEV_USB_CHG | DEV_USB)
#define DEV_T1_UART_MASK	(DEV_UART)
#define DEV_T1_CHARGER_MASK	(DEV_DEDICATED_CHG | DEV_CAR_KIT)
#define DEV_CARKIT_CHARGER1_MASK	(1 << 1)
#define MANSW1_OPEN_RUSTPROOF	((0x0 << 5)| (0x3 << 2) |(1 << 0))

/* Device Type 2 */

#define DEV_LANHUB		(1 << 9)

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
#define DEV_T2_JIG_ALL_MASK	(DEV_JIG_USB_OFF | DEV_JIG_USB_ON | \
				DEV_JIG_UART_OFF | DEV_JIG_UART_ON)

/* Device Type 3 */
#define DEV_MHL				(1 << 0)
#define DEV_VBUSIN_VALID	(1 << 1)
#define DEV_NON_STANDARD	(1 << 2)
#define DEV_AV_VBUS			(1 << 4)
#define DEV_U200_CHARGER	(1 << 6)

#define DEV_T3_CHARGER_MASK	(DEV_U200_CHARGER)

/*
 * Manual Switch
 * D- [7:5] / D+ [4:2]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100: V_AUDIO
 */
#define SW_VAUDIO		((4 << 5) | (4 << 2) | (1 << 1) | (1 << 0))
#define SW_UART			((3 << 5) | (3 << 2))
#define SW_AUDIO		((2 << 5) | (2 << 2) | (1 << 0))
#define SW_DHOST		((1 << 5) | (1 << 2) | (1 << 0))
#define SW_AUTO			((0 << 5) | (0 << 2))
#define SW_USB_OPEN		(1 << 0)
#define SW_ALL_OPEN		(0)
#define SW_ALL_OPEN_WITH_VBUS	((0 << 5) | (0 << 2) | (1 << 0))

/* Interrupt 1 */
#define INT_OXP_DISABLE			(1 << 7)
#define INT_OCP_ENABLE			(1 << 6)
#define INT_OVP_ENABLE			(1 << 5)
#define INT_LONG_KEY_RELEASE	(1 << 4)
#define INT_LONG_KEY_PRESS		(1 << 3)
#define INT_KEY_PRESS			(1 << 2)
#define INT_DETACH				(1 << 1)
#define INT_ATTACH				(1 << 0)

/* Interrupt 2 */
#define INT_VBUSOUT_ON          (1 << 7)
#define INT_OTP_ENABLE			(1 << 6)
#define INT_CONNECT				(1 << 5)
#define INT_STUCK_KEY_RCV		(1 << 4)
#define INT_STUCK_KEY			(1 << 3)
#define INT_ADC_CHANGE			(1 << 2)
#define INT_RESERVED_ATTACH		(1 << 1)
#define INT_VBUSOUT_OFF			(1 << 0)
/* ADC VALUE */
#define	ADC_OTG				0x00
#define	ADC_MHL				0x01
#define ADC_SMART_DOCK			0x10
#define ADC_AUDIO_DOCK			0x12
#define	ADC_JIG_USB_OFF			0x18
#define	ADC_JIG_USB_ON			0x19
#define	ADC_DESKDOCK			0x1a
#define	ADC_JIG_UART_OFF		0x1c
#define	ADC_JIG_UART_ON			0x1d
#define	ADC_CARDOCK			0x1d
#define	ADC_OPEN			0x1f
#define ADC_LANHUB			0x13
#define ADC_CHARGING_CABLE		0x14

int uart_sm5502_connecting;
EXPORT_SYMBOL(uart_sm5502_connecting);
int detached_sm5502_status;
EXPORT_SYMBOL(detached_sm5502_status);
static int jig_state;

//function added for enabling torch through AT command
#if defined(CONFIG_TORCH_FIX)
int factory_uart_connected(void);
#endif

struct sm5502_usbsw {
	struct i2c_client		*client;
	struct sm5502_platform_data	*pdata;
	int				dev1;
	int				dev2;
	int				dev3;
	int				mansw;
	int				vbus;
	int				dock_attached;
	int				dev_id;
	int				carkit_dev;
	struct delayed_work		init_work;
	struct mutex			mutex;
	int				adc;
	/* muic current attached device */
	enum muic_attached_dev		attached_dev;
#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
	unsigned int			previous_dock;
	unsigned int			lanhub_ta_status;
#endif
#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
	bool				is_rustproof;
#endif
};

static struct sm5502_usbsw *local_usbsw;

static int sm5502_attach_dev(struct sm5502_usbsw *usbsw);
static int sm5502_detach_dev(struct sm5502_usbsw *usbsw);

#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)

bool lanhub_ta_case = false;

/* RAW DATA Detection*/
static void sm5502_enable_rawdataInterrupts(struct sm5502_usbsw *usbsw)
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

static void sm5502_disable_rawdataInterrupts(struct sm5502_usbsw *usbsw)
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

static int sm5502_write_reg(struct i2c_client *client, int reg, int val)
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

static int sm5502_read_reg(struct i2c_client *client, int reg)
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


static void sm5502_dock_control(struct sm5502_usbsw *usbsw,
	int dock_type, int state, int path)
{
	struct i2c_client *client = usbsw->client;
	struct sm5502_platform_data *pdata = usbsw->pdata;
	int ret;

	if (state) {
		usbsw->mansw = path;
		pdata->callback(dock_type, state);
		if (dock_type == CABLE_TYPE_DESK_DOCK_NO_VB)
			switch_set_state(&switch_dock, state);
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
		if (dock_type == CABLE_TYPE_DESK_DOCK_NO_VB)
                        switch_set_state(&switch_dock, state);
		ret = i2c_smbus_read_byte_data(client, REG_CONTROL);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_write_byte_data(client, REG_CONTROL,
			ret | CON_MANUAL_SW | CON_RAW_DATA);
		if (ret < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, ret);
	}
}

static void sm5502_reg_init(struct sm5502_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	unsigned int ctrl = CON_MASK;
	int ret;

	pr_info("sm5502_reg_init is called\n");

	usbsw->dev_id = i2c_smbus_read_byte_data(client, REG_DEVICE_ID);
	local_usbsw->dev_id = usbsw->dev_id;
	if (usbsw->dev_id < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, usbsw->dev_id);

	dev_info(&client->dev, " sm5502_reg_init dev ID: 0x%x\n",
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
	/*set timing1 to 300ms */
	ret = i2c_smbus_write_byte_data(client, REG_TIMING_SET1, 0x04);
	if (ret < 0)
        	dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	/*Manual SW2 bit2 : JIG_ON '1' */
	ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW2, 0x04);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	/*Disabling the DCD Timer to ensure a high cable detection rate*/
	/*reset value is 0x0E, clearing the bit 2 to disable DCD timeout*/
	ret = i2c_smbus_write_byte_data(client, REG_TIMER_SET, 0x0A);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);


}

static ssize_t sm5502_muic_show_attached_dev(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);

	pr_info("[MUIC] SM5502:%s attached_dev:%d\n",
					__func__,usbsw->attached_dev);

	switch(usbsw->attached_dev) {
	case ATTACHED_DEV_NONE_MUIC:
		return sprintf(buf, "No VPS\n");
	case ATTACHED_DEV_USB_MUIC:
		return sprintf(buf, "USB\n");
	case ATTACHED_DEV_CDP_MUIC:
		return sprintf(buf, "CDP\n");
	case ATTACHED_DEV_OTG_MUIC:
		return sprintf(buf, "OTG\n");
	case ATTACHED_DEV_TA_MUIC:
		return sprintf(buf, "TA\n");
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		return sprintf(buf, "JIG UART OFF\n");
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
		return sprintf(buf, "JIG UART OFF/VB\n");
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		return sprintf(buf, "JIG UART ON\n");
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
		return sprintf(buf, "JIG USB OFF\n");
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
		return sprintf(buf, "JIG USB ON\n");
	case ATTACHED_DEV_DESKDOCK_MUIC:
		return sprintf(buf, "DESKDOCK\n");
	case ATTACHED_DEV_AUDIODOCK_MUIC:
		return sprintf(buf, "AUDIODOCK\n");
	default:
		break;
	}

	return sprintf(buf, "UNKNOWN\n");
}

static ssize_t sm5502_show_control(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, REG_CONTROL);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	return snprintf(buf, 13, "CONTROL: %02x\n", value);
}

static ssize_t sm5502_show_device_type(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE1);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	return snprintf(buf, 11, "DEV_TYP %02x\n", value);
}

static ssize_t sm5502_show_manualsw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, REG_MANUAL_SW1);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if (value == SW_VAUDIO)
		return snprintf(buf, 7, "VAUDIO\n");
	else if (value == SW_UART)
		return snprintf(buf, 5, "UART\n");
	else if (value == SW_AUDIO)
		return snprintf(buf, 6, "AUDIO\n");
	else if (value == SW_DHOST)
		return snprintf(buf, 6, "DHOST\n");
	else if (value == SW_AUTO)
		return snprintf(buf, 5, "AUTO\n");
	else
		return snprintf(buf, 4, "%x", value);
}

static ssize_t sm5502_set_manualsw(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value, ret;
	unsigned int path = 0;

	value = i2c_smbus_read_byte_data(client, REG_CONTROL);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if ((value & ~CON_MANUAL_SW) !=
			(CON_SWITCH_OPEN | CON_RAW_DATA | CON_WAIT))
		return 0;

	if (!strncmp(buf, "VAUDIO", 6)) {
		path = SW_VAUDIO;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "UART", 4)) {
		path = SW_UART;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "AUDIO", 5)) {
		path = SW_AUDIO;
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
static ssize_t sm5502_show_usb_state(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);
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

static ssize_t sm5502_show_adc(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);
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

static ssize_t sm5502_reset(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	if (!strncmp(buf, "1", 1)) {
		dev_info(&client->dev,
			"sm5502 reset after delay 1000 msec.\n");
		msleep(1000);
		sm5502_write_reg(client, REG_RESET, 0x01);

	dev_info(&client->dev, "sm5502_reset_control done!\n");
	} else {
		dev_info(&client->dev,
			"sm5502_reset_control, but not reset_value!\n");
	}

#ifdef CONFIG_MUIC_SUPPORT_RUSTPROOF
	usbsw->is_rustproof = false;
#endif
	usbsw->attached_dev = ATTACHED_DEV_NONE_MUIC;

	sm5502_reg_init(usbsw);

	return count;
}

#ifdef CONFIG_MUIC_SUPPORT_RUSTPROOF
static void muic_rustproof_feature(struct i2c_client *client, int state);
/* Keystring "*#0*#" sysfs implementation */
static ssize_t uart_en_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);
	/*is_rustproof is false then UART can be enabled*/
	return snprintf(buf, 4, "%d\n", !(usbsw->is_rustproof));
}

static ssize_t uart_en_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	if (!strncmp(buf, "1", 1)) {
		dev_info(&client->dev,
			"[MUIC]Runtime enabling the UART.\n");
		usbsw->is_rustproof = false;
		muic_rustproof_feature(client,SM5502_DETACHED);

	} else {
		dev_info(&client->dev,
			"[MUIC]Runtime disabling the UART.\n");
		usbsw->is_rustproof = true;
	}
	/* reinvoke the attach detection function to set proper paths */
	sm5502_attach_dev(usbsw);

	return size;
}

static DEVICE_ATTR(uart_en, S_IRUGO | S_IWUSR ,
				uart_en_show, uart_en_store);

static ssize_t uart_sel_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);
	/*for sm5502 paths are always switch to AP*/
	if(usbsw->attached_dev != ATTACHED_DEV_NONE_MUIC)
		return snprintf(buf, 4, "AP\n");
	else
		return snprintf(buf, 9, "UNKNOWN\n");
}

static ssize_t uart_sel_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);
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
	struct sm5502_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	dev_info(&client->dev,"[MUIC]Enabling AP UART Path, dummy Call\n");
	return size;
}

static DEVICE_ATTR(usb_sel, S_IRUGO | S_IWUSR ,
				usbsel_show, usbsel_store);
#endif

static DEVICE_ATTR(control, S_IRUGO, sm5502_show_control, NULL);
static DEVICE_ATTR(device_type, S_IRUGO, sm5502_show_device_type, NULL);
static DEVICE_ATTR(switch, S_IRUGO | S_IWUSR,
		sm5502_show_manualsw, sm5502_set_manualsw);
static DEVICE_ATTR(usb_state, S_IRUGO, sm5502_show_usb_state, NULL);
static DEVICE_ATTR(adc, S_IRUGO, sm5502_show_adc, NULL);
static DEVICE_ATTR(reset_switch, S_IWUSR | S_IWGRP, NULL, sm5502_reset);
static DEVICE_ATTR(attached_dev, S_IRUGO, sm5502_muic_show_attached_dev, NULL);

static struct attribute *sm5502_attributes[] = {
	&dev_attr_control.attr,
	&dev_attr_device_type.attr,
	&dev_attr_switch.attr,
	NULL
};

static const struct attribute_group sm5502_group = {
	.attrs = sm5502_attributes,
};

#if (!defined(CONFIG_MACH_CT01) && !defined(CONFIG_MACH_CT01_CHN_CU))
#define PMIC_SLAVE_ID	0x0
#define PMIC_SMBBP_BAT_IF_BPD_CTRL	0x1248
/* control flash function without BAT_ID */
static void flash_control(bool en) {
	u8 sid = PMIC_SLAVE_ID;
	u8 buf[16];
	int addr = PMIC_SMBBP_BAT_IF_BPD_CTRL;

	spmi_ext_register_readl_extra(sid, addr, buf, 1);
	if (en)
		buf[0] &= ~BIT(0);
	else
		buf[0] |= BIT(0);

	spmi_ext_register_writel_extra(sid, addr, buf, 1);
}
#endif

#if defined(CONFIG_USB_HOST_NOTIFY)
static void sm5502_set_otg(struct sm5502_usbsw *usbsw, int state)
{
	int ret;
	struct i2c_client *client = usbsw->client;

	if (state == SM5502_ATTACHED) {
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

#ifndef CONFIG_USB_HOST_NOTIFY
/* Dummy callback Function to handle the OTG Test case*/
int sec_get_notification(int mode)
{
	return 1;
}
#endif

#if defined(CONFIG_VIDEO_MHL_V2)
int dock_det(void)
{
	return local_usbsw->dock_attached;
}
EXPORT_SYMBOL(dock_det);
#endif

int check_sm5502_jig_state(void)
{
	return jig_state;
}
EXPORT_SYMBOL(check_sm5502_jig_state);

#if defined(CONFIG_TOUCHSCREEN_MMS144)
extern void tsp_charger_infom(bool en);
#endif

#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
static void sm5502_set_lanhub(struct sm5502_usbsw *usbsw, int state)
{
        int ret;
        struct i2c_client *client = usbsw->client;

        if (state == SM5502_ATTACHED) {
                ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW1, 0x25);
                if (ret < 0)
                        dev_info(&client->dev, "%s: err %d\n", __func__, ret);
		ret = i2c_smbus_write_byte_data(client, REG_MANUAL_SW2, 0x00); //Disconnect the ITBP & MUIC_ID Pins
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

static void sm5502_mask_vbus_detect(struct sm5502_usbsw *usbsw, int state){
	unsigned int value;
	struct i2c_client *client = usbsw->client;
	if(state == SM5502_ATTACHED) {
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

static int sm5502_detect_lanhub(struct sm5502_usbsw *usbsw) {
	unsigned int dev1, dev2, adc;
	struct sm5502_platform_data *pdata = usbsw->pdata;
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
		sm5502_set_otg(usbsw, SM5502_ATTACHED);
		if(usbsw->previous_dock == SM5502_NONE) {
			dev_info(&client->dev, "%s:LANHUB Connected\n", __func__);
			pdata->callback(CABLE_TYPE_OTG, SM5502_ATTACHED);
			sm5502_set_otg(usbsw,SM5502_ATTACHED);
		}
		else if(usbsw->previous_dock == ADC_LANHUB) {
			dev_info(&client->dev, "%s:Switch LANHUB+TA to LANHUB\n", __func__);
			usbsw->lanhub_ta_status=0;
			pdata->lanhub_cb(CABLE_TYPE_LANHUB, SM5502_DETACHED, LANHUB_TA);
		}
		sm5502_mask_vbus_detect(usbsw,SM5502_DETACHED);
		usbsw->dock_attached = SM5502_ATTACHED;
		usbsw->previous_dock = ADC_OTG;
		break;
	case ADC_LANHUB:
		usbsw->adc = adc;
		lanhub_ta_case = true;
		usbsw->lanhub_ta_status = 1;
		sm5502_mask_vbus_detect(usbsw,SM5502_ATTACHED);
		usbsw->dock_attached = SM5502_ATTACHED;
		usbsw->mansw = SW_DHOST;
		if(usbsw->previous_dock == SM5502_NONE) {
			dev_info(&client->dev, "%s:LANHUB+TA Connected\n", __func__);
			pdata->lanhub_cb(CABLE_TYPE_LANHUB, SM5502_ATTACHED, LANHUB);
			sm5502_set_lanhub(usbsw,SM5502_ATTACHED);
		} else if (usbsw->previous_dock == ADC_OTG) {
			dev_info(&client->dev, "%s:Switch LANHUB to LANHUB+TA\n", __func__);
			pdata->lanhub_cb(CABLE_TYPE_LANHUB, SM5502_ATTACHED, LANHUB_TA);
		}
		usbsw->previous_dock = ADC_LANHUB;
		break;
	case ADC_OPEN:
		usbsw->adc = adc;
		dev_info(&client->dev, "%s:LANHUB + TA -> ADC_OPEN case\n", __func__);
		if (pdata->lanhub_cb && usbsw->lanhub_ta_status == 1) {
			if (usbsw->previous_dock == ADC_LANHUB)
				pdata->lanhub_cb(CABLE_TYPE_LANHUB,SM5502_DETACHED, LANHUB);
			sm5502_disable_rawdataInterrupts(usbsw);
			usbsw->lanhub_ta_status=0;
			sm5502_mask_vbus_detect(usbsw,SM5502_DETACHED);
			pdata->callback(CABLE_TYPE_OTG,SM5502_DETACHED);
			sm5502_set_otg(usbsw, SM5502_DETACHED);
			}
		else if (usbsw->previous_dock == ADC_OTG) {
			sm5502_disable_rawdataInterrupts(usbsw);
			pdata->callback(CABLE_TYPE_OTG,SM5502_DETACHED);
			sm5502_set_otg(usbsw, SM5502_DETACHED);
		}else {
			dev_info(&client->dev, "%s:ignore ADC_OPEN case\n", __func__);
				sm5502_mask_vbus_detect(usbsw,SM5502_DETACHED);
			}

		usbsw->previous_dock = SM5502_NONE;
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

// For Torch enable
#if defined(CONFIG_TORCH_FIX)
int factory_uart_connected()
{
        int val2 = 0, jig_state_factory = 0;
        struct i2c_client *client = local_usbsw->client;

        val2 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE2);
        jig_state_factory =  (val2 & DEV_T2_JIG_ALL_MASK) ? 1 : 0;
        printk(KERN_INFO "%s: jig_state_factory = %d\n", __func__, jig_state_factory);
        return jig_state_factory;
}
EXPORT_SYMBOL(factory_uart_connected);
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

static void muic_update_jig_state(struct sm5502_usbsw *usbsw, int dev_type2, int vbus)
{
	if(dev_type2 & DEV_JIG_UART_OFF && !vbus)
		usbsw->attached_dev = ATTACHED_DEV_JIG_UART_OFF_MUIC;
	else if(dev_type2 & DEV_JIG_UART_OFF && vbus)
		usbsw->attached_dev = ATTACHED_DEV_JIG_UART_OFF_VB_MUIC;
	else if(dev_type2 & DEV_JIG_UART_ON)
		usbsw->attached_dev = ATTACHED_DEV_JIG_UART_ON_MUIC;
	else if(dev_type2 & DEV_JIG_USB_OFF)
		usbsw->attached_dev = ATTACHED_DEV_JIG_USB_OFF_MUIC;
	else if(dev_type2 & DEV_JIG_USB_ON)
		usbsw->attached_dev = ATTACHED_DEV_JIG_USB_ON_MUIC;
}

#if defined CONFIG_ID_BYPASS_SBL
int otg_attached = 0;
#endif

static int sm5502_attach_dev(struct sm5502_usbsw *usbsw)
{
	int adc;
	int val1, val2, val3, val4, vbus;
	struct sm5502_platform_data *pdata = usbsw->pdata;
	struct i2c_client *client = usbsw->client;
#if defined(CONFIG_VIDEO_MHL_V2)
	/*u8 mhl_ret = 0;*/
#endif
#if defined(CONFIG_TOUCHSCREEN_MMS144)
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

	val3 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE3);
	if (val3 < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, val3);
		return val3;
	}

	val4 = i2c_smbus_read_byte_data(client, REG_CARKIT_STATUS);
	if (val4 < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, val4);
		return val4;
	}

    // add start
	vbus = i2c_smbus_read_byte_data(client, REG_VBUSINVALID);
	if (vbus < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, vbus);
		return vbus;
	}    
    // add end

    
	adc = i2c_smbus_read_byte_data(client, REG_ADC);

	if (adc == ADC_SMART_DOCK) {
		val2 = DEV_SMARTDOCK;
		val1 = 0;
	}
#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
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
			"dev1: 0x%x,dev2: 0x%x,dev3: 0x%x,Carkit: 0x%x,ADC: 0x%x,Jig: %s\n",
			val1, val2, val3, val4, adc,
			(check_sm5502_jig_state() ? "ON" : "OFF"));

	/* USB */
	if (val1 & DEV_USB) {
		pr_info("[MUIC] USB Connected\n");
		pdata->callback(CABLE_TYPE_USB, SM5502_ATTACHED);
		usbsw->attached_dev = ATTACHED_DEV_USB_MUIC;
	/* D+,D-open */
	} else if (val3 & DEV_NON_STANDARD) {
		pr_info("[MUIC] D+,D-open Connected\n");
		pdata->callback(CABLE_TYPE_USB, SM5502_ATTACHED);
	/* USB_CDP */
	} else if (val1 & DEV_USB_CHG) {
		pr_info("[MUIC] CDP Connected\n");
		pdata->callback(CABLE_TYPE_CDP, SM5502_ATTACHED);
		usbsw->attached_dev = ATTACHED_DEV_CDP_MUIC;
	/* UART */
	} else if (val1 & DEV_T1_UART_MASK || val2 & DEV_T2_UART_MASK) {
		uart_sm5502_connecting = 1;
		muic_update_jig_state(usbsw,val2,vbus);
#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
		if(usbsw->is_rustproof) {
			pr_info("[MUIC] RustProof mode, close UART Path\n");
			muic_rustproof_feature(client,SM5502_ATTACHED);
		} else
#endif
		{
			pr_info("[MUIC] UART Connected\n");
			i2c_smbus_write_byte_data(client, REG_MANUAL_SW1, SW_UART);
			if(vbus & DEV_VBUSIN_VALID)
				pdata->callback(CABLE_TYPE_JIG_UART_OFF_VB, SM5502_ATTACHED);
			else
				pdata->callback(CABLE_TYPE_UARTOFF, SM5502_ATTACHED);
#if (!defined(CONFIG_MACH_CT01) && !defined(CONFIG_MACH_CT01_CHN_CU))
			flash_control(true);
#endif
		}
#ifndef CONFIG_ATT_CHARGER_SUPPORT
	/* L Company Cable Detection Code */
	} else if (adc == 0x17 &&
			(val4 & DEV_CARKIT_CHARGER1_MASK)) {
		pr_info("[MUIC] 219K USB Cable/Charger Connected\n");
		pdata->callback(CABLE_TYPE_219KUSB,SM5502_ATTACHED);
		usbsw->attached_dev = ATTACHED_DEV_USB_MUIC;
#endif
	/* CHARGER */
	} else if ((val1 & DEV_T1_CHARGER_MASK) ||
			(val3 & DEV_T3_CHARGER_MASK)) {
		pr_info("[MUIC] Charger Connected\n");
		usbsw->attached_dev = ATTACHED_DEV_TA_MUIC;
		pdata->callback(CABLE_TYPE_AC, SM5502_ATTACHED);
#if defined(CONFIG_USB_HOST_NOTIFY)
	/* for SAMSUNG OTG */
	} else if (val1 & DEV_USB_OTG && adc == ADC_OTG) {
		pr_info("[MUIC] OTG Connected\n");
		usbsw->attached_dev = ATTACHED_DEV_OTG_MUIC;
#if defined CONFIG_ID_BYPASS_SBL
		otg_attached = 1;
#endif
#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
		sm5502_enable_rawdataInterrupts(usbsw);
		usbsw->dock_attached = SM5502_ATTACHED;
		usbsw->previous_dock = ADC_OTG;
#endif
		sm5502_set_otg(usbsw, SM5502_ATTACHED);
		pdata->callback(CABLE_TYPE_OTG, SM5502_ATTACHED);
#endif
	/* JIG */
	} else if (val2 & DEV_T2_USB_MASK) {
		pr_info("[MUIC] JIG USB Connected\n");
		muic_update_jig_state(usbsw,val2,vbus);
		if(vbus & DEV_VBUSIN_VALID)
			pdata->callback(CABLE_TYPE_USB, SM5502_ATTACHED);
		else
			pdata->callback(CABLE_TYPE_JIG, SM5502_ATTACHED);
#if (!defined(CONFIG_MACH_CT01) && !defined(CONFIG_MACH_CT01_CHN_CU))
		flash_control(true);
#endif
	/* Desk Dock */
	} else if ((val2 & DEV_AV) || (val3 & DEV_AV_VBUS)) {
		pr_info("[MUIC] Deskdock Connected\n");
		usbsw->attached_dev = ATTACHED_DEV_DESKDOCK_MUIC;
		local_usbsw->dock_attached = SM5502_ATTACHED;
		if(vbus & DEV_VBUSIN_VALID)
			sm5502_dock_control(usbsw, CABLE_TYPE_DESK_DOCK,
				SM5502_ATTACHED, SW_AUDIO);
		else
			sm5502_dock_control(usbsw, CABLE_TYPE_DESK_DOCK_NO_VB,
				SM5502_ATTACHED, SW_AUDIO);
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
		muic_update_jig_state(usbsw,val2,vbus);
#if defined(CONFIG_SEC_FACTORY)
		local_usbsw->dock_attached = SM5502_ATTACHED;
		sm5502_dock_control(usbsw, CABLE_TYPE_CARDOCK,
			SM5502_ATTACHED, SW_AUDIO);
#elif defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
                if(usbsw->is_rustproof) {
                        pr_info("[MUIC] RustProof mode, close UART Path\n");
			muic_rustproof_feature(client,SM5502_ATTACHED);
                }
#endif
	/* SmartDock */
	} else if (val2 & DEV_SMARTDOCK) {
		pr_info("[MUIC] Smartdock Connected\n");
		sm5502_dock_control(usbsw, CABLE_TYPE_SMART_DOCK,
			SM5502_ATTACHED, SW_DHOST);
#if defined(CONFIG_VIDEO_MHL_V2)
		/*mhl_onoff_ex(1); support can be added once mhl is up*/
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
	/* Audio Dock */
	} else if (val2 & DEV_AUDIO_DOCK) {
		pr_info("[MUIC] Audiodock Connected\n");
		usbsw->attached_dev = ATTACHED_DEV_AUDIODOCK_MUIC;
		sm5502_dock_control(usbsw, CABLE_TYPE_AUDIO_DOCK,
			SM5502_ATTACHED, SW_DHOST);
#endif

#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
		/* LANHUB */
	} else if (val2 & DEV_LANHUB) {
		if(usbsw->previous_dock == ADC_LANHUB &&
			usbsw->lanhub_ta_status == 1 )
			/*Skip for the LANHUB+TA Case*/
			pr_info("[MUIC] Lanhub + TA is connected\n");
		else {
		/* Enable RAWDATA Interrupts */
		sm5502_enable_rawdataInterrupts(usbsw);
		sm5502_detect_lanhub(usbsw);
		}
#endif
	/* Charging Cable */
    } else if ((val2 & DEV_PPD) && (adc == ADC_CHARGING_CABLE)) {
        pr_info("[MUIC] Phone Charging cable Connected\n");
        pdata->callback(CABLE_TYPE_CHARGING_CABLE,
                SM5502_ATTACHED);
	/* Incompatible */
    } else if (vbus & DEV_VBUSIN_VALID) {
        pr_info("[MUIC] Incompatible Charger Connected\n");
	usbsw->attached_dev = ATTACHED_DEV_UNKNOWN_MUIC;
        pdata->callback(CABLE_TYPE_INCOMPATIBLE,
                SM5502_ATTACHED);
    }
#if defined(CONFIG_TOUCHSCREEN_MMS144)
	else{
		tsp_noti_ignore = 1;
		printk("[TSP] attached, but don't noti \n");
	}
	if(!tsp_noti_ignore)
		tsp_charger_infom(1);

#endif

	usbsw->dev1 = val1;
	usbsw->dev2 = val2;
	usbsw->dev3 = val3;
	usbsw->adc = adc;
	usbsw->vbus = vbus; 
	usbsw->carkit_dev = val4;

	return adc;
}

static int sm5502_detach_dev(struct sm5502_usbsw *usbsw)
{
	struct sm5502_platform_data *pdata = usbsw->pdata;
#if defined(CONFIG_TOUCHSCREEN_MMS144)
	int tsp_noti_ignore = 0;

#endif

	/* USB */
	if (usbsw->dev1 & DEV_USB) {
		pr_info("[MUIC] USB Disconnected\n");
		pdata->callback(CABLE_TYPE_USB, SM5502_DETACHED);
	} else if (usbsw->dev1 & DEV_USB_CHG) {
		pdata->callback(CABLE_TYPE_CDP, SM5502_DETACHED);
	/* D+,D-open */
	} else if (usbsw->dev3 & DEV_NON_STANDARD) {
		pr_info("[MUIC] D+,D-open Disconnected\n");
		pdata->callback(CABLE_TYPE_USB, SM5502_DETACHED);
	/* UART */
	} else if (usbsw->dev1 & DEV_T1_UART_MASK ||
			usbsw->dev2 & DEV_T2_UART_MASK) {
#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
		if(usbsw->is_rustproof) {
                        pr_info("[MUIC] RustProof mode Disconnected Event\n");
			muic_rustproof_feature(usbsw->client,SM5502_DETACHED);
                } else
#endif
		{
			pr_info("[MUIC] UART Disconnected\n");
			if(usbsw->vbus & DEV_VBUSIN_VALID)
				pdata->callback(CABLE_TYPE_JIG_UART_OFF_VB, SM5502_DETACHED);
			else
				pdata->callback(CABLE_TYPE_UARTOFF, SM5502_DETACHED);
			uart_sm5502_connecting = 0;
#if (defined(CONFIG_MACH_MS01_EUR_3G) || defined(CONFIG_MACH_MS01_CHN_CMCC_3G) || defined(CONFIG_MACH_MS01_CHN_CU_3G))
			flash_control(false);
#endif
		}
#ifndef CONFIG_ATT_CHARGER_SUPPORT
/* L Company Cable Detection Code */
	} else if (usbsw->adc == 0x17 &&
			(usbsw->carkit_dev & DEV_CARKIT_CHARGER1_MASK)) {
		pr_info("[MUIC] 219K USB Cable/Charger Disconnected\n");
		pdata->callback(CABLE_TYPE_219KUSB,SM5502_DETACHED);
#endif
	/* CHARGER */
	} else if ((usbsw->dev1 & DEV_T1_CHARGER_MASK) ||
			(usbsw->dev3 & DEV_T3_CHARGER_MASK)) {
		pr_info("[MUIC] Charger Disconnected\n");
		pdata->callback(CABLE_TYPE_AC, SM5502_DETACHED);
#if defined(CONFIG_USB_HOST_NOTIFY)
	/* for SAMSUNG OTG */
	} else if (usbsw->dev1 & DEV_USB_OTG) {
		pr_info("[MUIC] OTG Disconnected\n");
#if defined CONFIG_ID_BYPASS_SBL
		otg_attached = 0;
#endif
#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
		sm5502_disable_rawdataInterrupts(usbsw);
		pr_info("%s:lanhub_ta_status(%d)\n",
					__func__, usbsw->lanhub_ta_status);
		lanhub_ta_case = false;
		if (usbsw->lanhub_ta_status == 0) {
			pdata->callback(CABLE_TYPE_OTG,SM5502_DETACHED);
			sm5502_set_otg(usbsw, SM5502_DETACHED);
			}
		else if (pdata->lanhub_cb && usbsw->lanhub_ta_status == 1)
			pdata->lanhub_cb(CABLE_TYPE_LANHUB,SM5502_DETACHED, LANHUB);
		usbsw->dock_attached = SM5502_DETACHED;
		usbsw->lanhub_ta_status=0;
#else
		sm5502_set_otg(usbsw, SM5502_DETACHED);
		pdata->callback(CABLE_TYPE_OTG, SM5502_DETACHED);
#endif
#endif
	/* JIG */
	} else if (usbsw->dev2 & DEV_T2_USB_MASK) {
		pr_info("[MUIC] JIG USB Disconnected\n");
		if(usbsw->vbus & DEV_VBUSIN_VALID)
			pdata->callback(CABLE_TYPE_USB, SM5502_DETACHED);
		else
			pdata->callback(CABLE_TYPE_JIG, SM5502_DETACHED);
#if (defined(CONFIG_MACH_MS01_EUR_3G) || defined(CONFIG_MACH_MS01_CHN_CMCC_3G) || defined(CONFIG_MACH_MS01_CHN_CU_3G))
		flash_control(false);
#endif
	/* Desk Dock */
	} else if ((usbsw->dev2 & DEV_AV) ||
			(usbsw->dev3 & DEV_AV_VBUS)) {
		pr_info("[MUIC] Deskdock Disconnected\n");
		local_usbsw->dock_attached = SM5502_DETACHED;
		if(usbsw->vbus & DEV_VBUSIN_VALID)
			sm5502_dock_control(usbsw, CABLE_TYPE_DESK_DOCK,
				SM5502_DETACHED, SW_ALL_OPEN);
		else
			sm5502_dock_control(usbsw, CABLE_TYPE_DESK_DOCK_NO_VB,
				SM5502_DETACHED, SW_ALL_OPEN);
#if defined(CONFIG_MHL_D3_SUPPORT)
	/* MHL */
	} else if (usbsw->dev3 & DEV_MHL) {
		pr_info("[MUIC] MHL Disconnected\n");
		//mhl_onoff_ex(false);
		detached_sm5502_status = 1;
#endif
	/* Car Dock */
	} else if (usbsw->dev2 & DEV_JIG_UART_ON) {
		pr_info("[MUIC] Cardock Disconnected\n");
#if defined(CONFIG_SEC_FACTORY)
		local_usbsw->dock_attached = SM5502_DETACHED;
		sm5502_dock_control(usbsw, CABLE_TYPE_CARDOCK,
			SM5502_DETACHED, SW_ALL_OPEN);
#elif  defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
                if(usbsw->is_rustproof) {
                        pr_info("[MUIC] RustProof mode disconneted Event\n");
			muic_rustproof_feature(usbsw->client,SM5502_DETACHED);
                }
#endif
	/* Smart Dock */
	} else if (usbsw->dev2 == DEV_SMARTDOCK) {
		pr_info("[MUIC] Smartdock Disconnected\n");
		sm5502_dock_control(usbsw, CABLE_TYPE_SMART_DOCK,
			SM5502_DETACHED, SW_ALL_OPEN);
#if defined(CONFIG_VIDEO_MHL_V2)
		//mhl_onoff_ex(false);
#endif
#if defined(CONFIG_USB_HOST_NOTIFY)
	/* Audio Dock */
	} else if (usbsw->dev2 == DEV_AUDIO_DOCK) {
		pr_info("[MUIC] Audiodock Disconnected\n");
		sm5502_dock_control(usbsw, CABLE_TYPE_AUDIO_DOCK,
			SM5502_DETACHED, SW_ALL_OPEN);
#endif
#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
	/* LANHUB */
	} else if (usbsw->adc == ADC_LANHUB) {
		pr_info("[MUIC] Lanhub disconnected\n");
		/* Disable RAWDATA Interrupts */
		sm5502_disable_rawdataInterrupts(usbsw);
		lanhub_ta_case = false;
		usbsw->dock_attached = SM5502_DETACHED;
		sm5502_set_lanhub(usbsw,SM5502_DETACHED);
		sm5502_mask_vbus_detect(usbsw, SM5502_DETACHED);
		pr_info("%s:lanhub_ta_status(%d)\n",__func__, usbsw->lanhub_ta_status);
		if (pdata->lanhub_cb && usbsw->lanhub_ta_status==1)
			pdata->lanhub_cb(CABLE_TYPE_LANHUB, SM5502_DETACHED, LANHUB);
		else if (usbsw->lanhub_ta_status == 0) {
			pdata->callback(CABLE_TYPE_OTG, SM5502_DETACHED);
			sm5502_set_otg(usbsw, SM5502_DETACHED);
			}
		usbsw->lanhub_ta_status=0;
		usbsw->dock_attached = SM5502_DETACHED;

#endif
	/* Charging Cable */
    } else if ((usbsw->dev2 & DEV_PPD) &&
				(usbsw->adc == ADC_CHARGING_CABLE)) {
        pr_info("[MUIC] Phone Charging cable Disconnected\n");
        pdata->callback(CABLE_TYPE_CHARGING_CABLE,
                SM5502_DETACHED);
	/* Incompatible */
	} else if (usbsw->vbus & DEV_VBUSIN_VALID) {
		pr_info("[MUIC] Incompatible Charger Disconnected\n");
		pdata->callback(CABLE_TYPE_INCOMPATIBLE,
						SM5502_DETACHED);
    }
#if defined(CONFIG_TOUCHSCREEN_MMS144)
	else{
		tsp_noti_ignore = 1;
		printk("[TSP] detached, but don't noti \n");
	}
	if(!tsp_noti_ignore)
		tsp_charger_infom(0);
#endif

	i2c_smbus_write_byte_data(usbsw->client, REG_CONTROL, CON_MASK);

	usbsw->dev1 = 0;
	usbsw->dev2 = 0;
	usbsw->dev3 = 0;
	usbsw->adc = 0;
	usbsw->vbus = 0;
	usbsw->carkit_dev = 0;
	usbsw->attached_dev = ATTACHED_DEV_NONE_MUIC;
	return 0;

}
static irqreturn_t sm5502_irq_thread(int irq, void *data)
{
	struct sm5502_usbsw *usbsw = data;
	struct i2c_client *client = usbsw->client;
	int intr1, intr2;
	int val1, val3, adc , vbus;
	/* SM5502 : Read interrupt -> Read Device */
	pr_info("sm5502_irq_thread is called\n");

	mutex_lock(&usbsw->mutex);
	intr1 = i2c_smbus_read_byte_data(client, REG_INT1);
	intr2 = i2c_smbus_read_byte_data(client, REG_INT2);

	adc = i2c_smbus_read_byte_data(client, REG_ADC);
	dev_info(&client->dev, "%s: intr1 : 0x%x,intr2 : 0x%x, adc : 0x%x\n",
					__func__, intr1, intr2,adc);

	/* MUIC OVP Check */
	if (intr1 & INT_OVP_ENABLE)
		usbsw->pdata->oxp_callback(ENABLE);
	else if (intr1 & INT_OXP_DISABLE)
		usbsw->pdata->oxp_callback(DISABLE);

#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
	if((intr1 == 0x00 && (intr2 & 0x04))
				|| ((intr1 & INT_DETACH) && (intr2 & 0x04)))
		sm5502_detect_lanhub(usbsw);
	else
#endif
	/* device detection */
	/* interrupt both attach and detach */
	if (intr1 == (INT_ATTACH + INT_DETACH)) {
		val1 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE1);
		val3 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE3);
		vbus = i2c_smbus_read_byte_data(client, REG_VBUSINVALID);

		if ((adc == ADC_OPEN) && (val1 == DATA_NONE) &&
				((val3 == DATA_NONE) || (vbus == 0x00)))
			sm5502_detach_dev(usbsw);
		else
			sm5502_attach_dev(usbsw);
        }
	/* interrupt attach */
	else if ( (intr1 & INT_ATTACH) || (intr2 & INT_RESERVED_ATTACH ) )
		sm5502_attach_dev(usbsw);
	/* interrupt detach */
	else if (intr1 & INT_DETACH)
		sm5502_detach_dev(usbsw);
	else if ((intr2 == INT_VBUSOUT_ON)) {
		pr_info("sm5502: VBUSOUT_ON\n");
#ifdef CONFIG_USB_HOST_NOTIFY
		sec_otg_notify(HNOTIFY_OTG_POWER_ON);
#endif
		if(sec_get_notification(HNOTIFY_MODE) != NOTIFY_TEST_MODE){
			/*JIG UART OFF VBUS Change or Sosche Charger*/
			if((adc == ADC_JIG_UART_OFF) || (adc == ADC_OPEN))
				sm5502_attach_dev(usbsw);
			/*Deskdock VBUS Change scenario*/
			else if(adc == ADC_DESKDOCK)
				usbsw->pdata->callback(CABLE_TYPE_DESK_DOCK,SM5502_ATTACHED);
		}
	}
	else if (intr2 == INT_VBUSOUT_OFF) {
		pr_info("sm5502: VBUSOUT_OFF\n");
#ifdef CONFIG_USB_HOST_NOTIFY
                sec_otg_notify(HNOTIFY_OTG_POWER_OFF);
#endif
		if(sec_get_notification(HNOTIFY_MODE) != NOTIFY_TEST_MODE){
			/*JIG UART OFF VBUS Change or Sosche Charger*/
			if((adc == ADC_JIG_UART_OFF) || (adc == ADC_OPEN))
				sm5502_detach_dev(usbsw);
			/*Deskdock VBUS Change scenario*/
			else if(adc == ADC_DESKDOCK)
				usbsw->pdata->callback(CABLE_TYPE_DESK_DOCK_NO_VB,SM5502_DETACHED);
		}
	}

	mutex_unlock(&usbsw->mutex);
	pr_info("sm5502_irq_thread,end\n");
	return IRQ_HANDLED;
}

static int sm5502_irq_init(struct sm5502_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int ret;

	if (client->irq) {
		ret = request_threaded_irq(client->irq, NULL,
			sm5502_irq_thread, IRQF_TRIGGER_FALLING,
			"sm5502 micro USB", usbsw);
		if (ret) {
			dev_err(&client->dev, "failed to reqeust IRQ\n");
			return ret;
		}
		enable_irq_wake(client->irq);
	}

	return 0;
}

static void sm5502_init_detect(struct work_struct *work)
{
	struct sm5502_usbsw *usbsw = container_of(work,
			struct sm5502_usbsw, init_work.work);
	int ret;
	int int_reg1, int_reg2;

	dev_info(&usbsw->client->dev, "%s\n", __func__);

	mutex_lock(&usbsw->mutex);
	sm5502_attach_dev(usbsw);
	mutex_unlock(&usbsw->mutex);

	ret = sm5502_irq_init(usbsw);
	if (ret)
		dev_info(&usbsw->client->dev,
				"failed to enable  irq init %s\n", __func__);

	int_reg1 = sm5502_read_reg(usbsw->client, REG_INT1);
	dev_info(&usbsw->client->dev, "%s: intr1 : 0x%x\n",
		__func__, int_reg1);

	int_reg2 = i2c_smbus_read_byte_data(usbsw->client, REG_INT2);
	dev_info(&usbsw->client->dev, "%s: intr2 : 0x%x\n",
		__func__, int_reg2);

}

#ifdef CONFIG_OF
static int sm5502_parse_dt(struct device *dev, struct sm5502_platform_data *pdata)
{

        struct device_node *np = dev->of_node;
	/*changes can be added later, when needed*/
#if 0
        /* regulator info */
	pdata->i2c_pull_up = of_property_read_bool(np, "sm5502,i2c-pull-up");
#endif

        /* reset, irq gpio info */
        pdata->gpio_scl = of_get_named_gpio_flags(np, "sm5502,gpio-scl",
                               0, &pdata->scl_gpio_flags);
        pdata->gpio_uart_on = of_get_named_gpio_flags(np, "sm5502,uarton-gpio",
                               0, &pdata->uarton_gpio_flags);
        pdata->gpio_sda = of_get_named_gpio_flags(np, "sm5502,gpio-sda",
                               0, &pdata->sda_gpio_flags);
        pdata->gpio_int = of_get_named_gpio_flags(np, "sm5502,irq-gpio",
                0, &pdata->irq_gpio_flags);
	pr_info("%s: irq-gpio: %u \n", __func__, pdata->gpio_int);

        return 0;
}
#endif

static int __devinit sm5502_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct sm5502_usbsw *usbsw;
	int ret = 0;
	struct device *switch_dev;
	struct sm5502_platform_data *pdata;

	dev_info(&client->dev,"%s:sm5502 probe called \n",__func__);
	if(client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct sm5502_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory \n");
				return -ENOMEM;
		}
		ret = sm5502_parse_dt(&client->dev, pdata);
		if (ret < 0) {
			dev_err(&client->dev, "sm5502_parse_dt failed\n");
			goto fail;
		}

		pdata->callback = sm5502_callback;
#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
		pdata->lanhub_cb = sm5502_lanhub_callback;
#endif
		pdata->dock_init = sm5502_dock_init;
		pdata->oxp_callback = sm5502_oxp_callback;
		pdata->mhl_sel = NULL;
#if defined(CONFIG_MACH_S3VE3G_EUR) || defined(CONFIG_MACH_VICTOR3GDSDTV_LTN)
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_int,  0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#else
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_int,  0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_scl,  0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_sda,  0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#if defined(CONFIG_MACH_S3VE3G_EUR) || defined(CONFIG_MACH_VICTOR3GDSDTV_LTN) || defined(CONFIG_MACH_ATLANTIC3GEUR_OPEN)
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_uart_on,  0, GPIO_CFG_INPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
#else
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

	usbsw = kzalloc(sizeof(struct sm5502_usbsw), GFP_KERNEL);
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

	local_usbsw = usbsw;

#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
	ret = sm5502_read_reg(client,REG_MANUAL_SW1);
	if(ret<0) {
		dev_err(&client->dev, "failed to read MANUAL SW1 Reg, err:%d\n",ret);
	}
	/* Keep the feature disabled by default */
	usbsw->is_rustproof = false;

	/* RUSTPROOF: disable UART connection if MANSW1 from BL is OPEN_RUSTPROOF*/
	if(ret == MANSW1_OPEN_RUSTPROOF)
		usbsw->is_rustproof = true;

#endif
	/* SM5502 muic soft reset */
	ret = sm5502_write_reg(client, REG_RESET, 0x01);
	if(ret < 0) {
		dev_err(&client->dev,"failed to Soft reset MUIC, err:%d\n",ret);
	}

	sm5502_reg_init(usbsw);

	ret = sysfs_create_group(&client->dev.kobj, &sm5502_group);
	if (ret) {
		dev_err(&client->dev,
				"failed to create sm5502 attribute group\n");
		goto fail2;
	}

	/* make sysfs node /sys/class/sec/switch/ */
	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_dev)) {
		pr_err("[SM5502] Failed to create device (switch_dev)!\n");
		ret = PTR_ERR(switch_dev);
		goto fail2;
	}

	ret = device_create_file(switch_dev, &dev_attr_usb_state);
	if (ret < 0) {
		pr_err("[SM5502] Failed to create file (usb_state)!\n");
		goto err_create_file_state;
	}

	ret = device_create_file(switch_dev, &dev_attr_adc);
	if (ret < 0) {
		pr_err("[SM5502] Failed to create file (adc)!\n");
		goto err_create_file_adc;
	}

	ret = device_create_file(switch_dev, &dev_attr_reset_switch);
	if (ret < 0) {
		pr_err("[SM5502] Failed to create file (reset_switch)!\n");
		goto err_create_file_reset_switch;
	}

	ret = device_create_file(switch_dev, &dev_attr_attached_dev);
	if (ret < 0) {
		pr_err("[SM5502] Failed to create file (attached_dev)!\n");
		goto err_create_file_attached_dev;
	}

#if defined(CONFIG_MUIC_SUPPORT_RUSTPROOF)
	ret = device_create_file(switch_dev, &dev_attr_uart_en);
	if (ret < 0) {
		pr_err("[SM5502] Failed to create file (uart_en)!\n");
		goto err_create_file_uart_en;
	}
	ret = device_create_file(switch_dev, &dev_attr_uart_sel);
	if (ret < 0) {
		pr_err("[SM5502] Failed to create file (uart_sel)!\n");
		goto err_create_file_uart_sel;
	}
	ret = device_create_file(switch_dev, &dev_attr_usb_sel);
	if (ret < 0) {
		pr_err("[SM5502] Failed to create file (usb_sel)!\n");
		goto err_create_file_usb_sel;
	}

#endif
	local_usbsw->attached_dev = ATTACHED_DEV_NONE_MUIC;

#if defined(CONFIG_MUIC_SM5502_SUPPORT_LANHUB_TA)
	local_usbsw->previous_dock = SM5502_NONE;
	local_usbsw->lanhub_ta_status = 0;
#endif
	dev_set_drvdata(switch_dev, usbsw);
	/* sm5502 dock init*/
	if (usbsw->pdata->dock_init)
		usbsw->pdata->dock_init();

	/* initial cable detection */
	INIT_DELAYED_WORK(&usbsw->init_work, sm5502_init_detect);
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
err_create_file_attached_dev:
	device_remove_file(switch_dev, &dev_attr_attached_dev);
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

static int __devexit sm5502_remove(struct i2c_client *client)
{
	struct sm5502_usbsw *usbsw = i2c_get_clientdata(client);
	cancel_delayed_work(&usbsw->init_work);
	if (client->irq) {
		disable_irq_wake(client->irq);
		free_irq(client->irq, usbsw);
	}
	mutex_destroy(&usbsw->mutex);
	i2c_set_clientdata(client, NULL);

	sysfs_remove_group(&client->dev.kobj, &sm5502_group);
	kfree(usbsw);
	return 0;
}

static int sm5502_resume(struct i2c_client *client)
{
	struct sm5502_usbsw *usbsw = i2c_get_clientdata(client);
	int ldev1,ldev2,ldev3;

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
	ldev3 = i2c_smbus_read_byte_data(client, REG_DEVICE_TYPE3);
	if (ldev3 < 0)
	{
		pr_err("%s: Dev reg 3 read err! %d \n",__func__, ldev3);
		goto safe_exit;
	}

	i2c_smbus_read_byte_data(client, REG_INT1);
	i2c_smbus_read_byte_data(client, REG_INT2);

	if (usbsw->dev1 != ldev1 || usbsw->dev2 != ldev2 || usbsw->dev3 != ldev3)
	{
		/* device detection */
		mutex_lock(&usbsw->mutex);
		sm5502_attach_dev(usbsw);
		mutex_unlock(&usbsw->mutex);
	}

safe_exit:
	return 0;
}


static const struct i2c_device_id sm5502_id[] = {
	{"sm5502", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, sm5502_id);
static struct of_device_id sm5502_i2c_match_table[] = {
	{ .compatible = "sm5502,i2c",},
	{},
};
MODULE_DEVICE_TABLE(of, sm5502_i2c_match_table);

static struct i2c_driver sm5502_i2c_driver = {
	.driver = {
		.name = "sm5502",
		.owner = THIS_MODULE,
		.of_match_table = sm5502_i2c_match_table,
	},
	.probe = sm5502_probe,
	.remove = __devexit_p(sm5502_remove),
	.resume = sm5502_resume,
	.id_table = sm5502_id,
};

static int __init sm5502_init(void)
{
#if	defined(CONFIG_MACH_S3VE_CHN_OPEN) || defined(CONFIG_MACH_S3VE_CHN_CMCC)
	if (system_rev < 3)
		return 0;
#endif
	return i2c_add_driver(&sm5502_i2c_driver);
}
/*
Late init call is required MUIC was accessing
USB driver before USB initialization and watch dog reset
was happening when booted with USB connected*/
#if defined(CONFIG_TORCH_FIX)
module_init(sm5502_init);
#else
late_initcall(sm5502_init);
#endif

static void __exit sm5502_exit(void)
{
	i2c_del_driver(&sm5502_i2c_driver);
}
module_exit(sm5502_exit);

MODULE_AUTHOR("Jeongrae Kim <Jryu.kim@samsung.com>");
MODULE_DESCRIPTION("SM5502 Micro USB Switch driver");
MODULE_LICENSE("GPL");

