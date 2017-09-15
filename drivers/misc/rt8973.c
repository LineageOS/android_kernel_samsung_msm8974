/* drivers/mfd/rt8973.c
 * Richtek RT8973 Multifunction Device / MUIC Driver
 *
 * Copyright (C) 2013
 * Author: Patrick Chang <patrick_chang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <linux/kernel.h>
#include <linux/rtdefs.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/platform_data/rt8973.h>
#ifdef SAMSUNG_MVRL_MUIC_RT8973
#include <linux/gpio-pxa.h>
#include <linux/platform_data/mv_usb.h>
#include <mach/gpio-edge.h>
#endif
#if defined (CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif

#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif

#if defined(CONFIG_MACH_VICTORLTE_CTC)
/* spmi control */
extern int spmi_ext_register_writel_extra(u8 sid, u16 ad, u8 *buf, int len);
extern int spmi_ext_register_readl_extra(u8 sid, u16 ad, u8 *buf, int len);
#endif

//////////////////////////////////////////////////////////////////////
int rt_uart_connecting;
EXPORT_SYMBOL(rt_uart_connecting);

static int jig_state;

int rt_check_jig_state(void)
{
	return jig_state;
}
EXPORT_SYMBOL(rt_check_jig_state);
//////////////////////////////////////////////////////////////////////


#define RT8973_DEVICE_NAME "rt8973"
#define ALIAS_NAME RT8973_DEVICE_NAME
#define RT8973_DRV_VER "2.0.8SEC_Q"
#define RT8973_IRQF_MODE IRQF_TRIGGER_FALLING

#define RT8973_REG_CHIP_ID          0x01
#define RT8973_REG_CONTROL          0x02
#define RT8973_REG_INT_FLAG1        0x03
#define RT8973_REG_INT_FLAG2        0x04
#define RT8973_REG_INTERRUPT_MASK1  0x05
#define RT8973_REG_INTERRUPT_MASK2  0x06
#define RT8973_REG_ADC              0x07
#define RT8973_REG_DEVICE1          0x0A
#define RT8973_REG_DEVICE2          0x0B
#define RT8973_REG_MANUAL_SW1       0x13
#define RT8973_REG_MANUAL_SW2       0x14
#define RT8973_REG_RESET            0x1B

#define DCD_T_RETRY 2

#define RT8973_DEVICE1_OTG  0x01
#define RT8973_DEVICE1_SDP  0x04
#define RT8973_DEVICE1_UART 0x08
#define RT8973_DEVICE1_CDPORT   0x20
#define RT8973_DEVICE1_DCPORT   0x40

extern unsigned int system_rev;

#ifdef SAMSUNG_MVRL_MUIC_RT8973
static struct gpio_edge_desc muic_int_gpio;
#endif

struct device_desc {
	char *name;
	uint32_t reg_val;
	int cable_type;
};

static const struct device_desc device_to_cable_type_mapping[] = {
	{
	 .name = "OTG",
	 .reg_val = RT8973_DEVICE1_OTG,
	 .cable_type = MUIC_RT8973_CABLE_TYPE_OTG,
	 },
	{
	 .name = "USB SDP",
	 .reg_val = RT8973_DEVICE1_SDP,
	 .cable_type = MUIC_RT8973_CABLE_TYPE_USB,
	 },
	{
	 .name = "UART",
	 .reg_val = RT8973_DEVICE1_UART,
	 .cable_type = MUIC_RT8973_CABLE_TYPE_UART,
	 },
	{
	 .name = "USB CDP",
	 .reg_val = RT8973_DEVICE1_CDPORT,
	 .cable_type = MUIC_RT8973_CABLE_TYPE_CDP,
	 },
	{
	 .name = "USB DCP",
	 .reg_val = RT8973_DEVICE1_DCPORT,
	 .cable_type = MUIC_RT8973_CABLE_TYPE_REGULAR_TA,
	 },
};

struct id_desc {
	char *name;
	int cable_type_with_vbus;
	int cable_type_without_vbus;
};

static const struct id_desc id_to_cable_type_mapping[] = {
	{
	 /* 00000, 0 */
	 .name = "OTG",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_OTG,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_OTG,
	 },
	{			/* 00001, 1 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 00010, 2 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 00011, 3 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 00100, 4 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 00101, 5 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 00110, 6 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 00111, 7 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 01000, 8 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 01001, 9 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 01010, 10 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 01011, 11 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 01100, 12 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 01101, 13 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 01110, 14 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 01111, 15 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 10000, 16 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 10001, 17 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 10010, 18 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 10011, 19 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 10100, 20 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 10101, 21 */
	 .name = "ADC0x15 Charger/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_0x15,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 10110, 22 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 10111, 23 */
	 .name = "Type 1 Charger/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_TYPE1_CHARGER,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 11000, 24 */
	 .name = "FM BOOT OFF USB/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_JIG_USB_OFF,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 11001, 25 */
	 .name = "FM BOOT ON USB/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_JIG_USB_ON,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 11010, 26 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 11011, 27 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 11100, 28 */
	 .name = "JIG UART BOOT OFF",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF_WITH_VBUS,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF,
	 },
	{			/* 11101, 29 */
	 .name = "JIG UART BOOT ON",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_JIG_UART_ON_WITH_VBUS,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_JIG_UART_ON,
	 },
	{			/* 11110, 30 */
	 .name = "AT&T TA/Unknown",
	 .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
	 .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_UNKNOWN,
	 },
	{			/* 11111, 31 */
         .name = "AT&T TA/No cable",
         .cable_type_with_vbus = MUIC_RT8973_CABLE_TYPE_ATT_TA,
         .cable_type_without_vbus = MUIC_RT8973_CABLE_TYPE_NONE,
         },
};

enum {
	VBUS_SHIFT = 0,
	ACCESSORY_SHIFT,
	OCP_SHIFT,
	OVP_SHIFT,
	OTP_SHIFT,
	ADC_CHG_SHIFT,
	CABLE_CHG_SHIFT,
	OTG_SHIFT,
	DCDT_SHIFT,
	USB_SHIFT,
	UART_SHIFT,
	JIG_SHIFT,
	L200K_USB_SHIFT,
};

struct rt8973_status {
	int cable_type;
	int id_adc;
	uint8_t irq_flags[2];
	uint8_t device_reg[2];
	/* Processed useful status
	 * Compare previous and current regs
	 * to get this information */
	union {
		struct {
			uint32_t vbus_status:1;
			uint32_t accessory_status:1;
			uint32_t ocp_status:1;
			uint32_t ovp_status:1;
			uint32_t otp_status:1;
			uint32_t adc_chg_status:1;
			uint32_t cable_chg_status:1;
			uint32_t otg_status:1;
			uint32_t dcdt_status:1;
			uint32_t usb_connect:1;
			uint32_t uart_connect:1;
			uint32_t jig_connect:1;
			uint32_t l200k_usb_connect:1;
		};
		uint32_t status;
	};
};

typedef struct rt8973_chip {
	struct i2c_client *iic;
	struct mutex io_lock;
	struct rt8973_platform_data *pdata;
	struct device *dev;
	struct device *switch_dev;
	struct workqueue_struct *wq;
	struct delayed_work irq_work;
	struct delayed_work init_work;
	struct wake_lock muic_wake_lock;
	struct rt8973_status prev_status;
	struct rt8973_status curr_status;
	int dcdt_retry_count;
	int irq;
	int adc_reg_addr;
} rt8973_chip_t;

static struct rt8973_status *current_status;

static int32_t rt8973_read(struct rt8973_chip *chip, uint8_t reg,
			   uint8_t nbytes, uint8_t *buff)
{
	int ret;
	mutex_lock(&chip->io_lock);
	ret = i2c_smbus_read_i2c_block_data(chip->iic, reg, nbytes, buff);
	mutex_unlock(&chip->io_lock);
	return ret;
}

static int32_t rt8973_reg_read(struct rt8973_chip *chip, int reg)
{
	int ret;
	mutex_lock(&chip->io_lock);
	ret = i2c_smbus_read_byte_data(chip->iic, reg);
	mutex_unlock(&chip->io_lock);
	return ret;
}

int32_t rt8973_reg_write(struct rt8973_chip *chip, int reg, unsigned char data)
{
	int ret;
	mutex_lock(&chip->io_lock);
	ret = i2c_smbus_write_byte_data(chip->iic, reg, data);
	mutex_unlock(&chip->io_lock);
	return ret;
}

static int32_t rt8973_assign_bits(struct rt8973_chip *chip, int reg,
				  unsigned char mask, unsigned char data)
{
	struct i2c_client *iic;
	int ret;
	iic = chip->iic;
	mutex_lock(&chip->io_lock);
	ret = i2c_smbus_read_byte_data(iic, reg);
	if (ret < 0)
		goto out;
	ret &= ~mask;
	ret |= data;
	ret = i2c_smbus_write_byte_data(iic, reg, ret);
out:
	mutex_unlock(&chip->io_lock);
	return ret;
}

int32_t rt8973_set_bits(struct rt8973_chip *chip, int reg, unsigned char mask)
{
	return rt8973_assign_bits(chip, reg, mask, mask);
}

int32_t rt8973_clr_bits(struct rt8973_chip *chip, int reg, unsigned char mask)
{
	return rt8973_assign_bits(chip, reg, mask, 0);
}

static inline int rt8973_update_regs(rt8973_chip_t *chip)
{
	int ret;
	ret = rt8973_read(chip, RT8973_REG_INT_FLAG1,
			  2, chip->curr_status.irq_flags);
	if (ret < 0)
		return ret;
	ret = rt8973_read(chip, RT8973_REG_DEVICE1,
			  2, chip->curr_status.device_reg);
	if (ret < 0)
		return ret;
	ret = rt8973_reg_read(chip, chip->adc_reg_addr);
	if (ret < 0)
		return ret;
	chip->curr_status.id_adc = ret;
	/* invalid id value */
	if (ret >= ARRAY_SIZE(id_to_cable_type_mapping))
		return -EINVAL;
	return 0;
}

static int rt8973_get_cable_type_by_device_reg(rt8973_chip_t *chip)
{
	uint32_t device_reg = chip->curr_status.device_reg[1];
	int i;
	pr_info("RT8973 : Device = 0x%x, 0x%x\n",
	       (int)chip->curr_status.device_reg[0],
	       (int)chip->curr_status.device_reg[1]);
	device_reg <<= 8;
	device_reg |= chip->curr_status.device_reg[0];
	for (i = 0; i < ARRAY_SIZE(device_to_cable_type_mapping); i++) {
		if (device_to_cable_type_mapping[i].reg_val == device_reg) {
			if (chip->curr_status.dcdt_status == 1) {
				/* USB ID ==> open, i.e., ADC reading = 0x1f */
				return id_to_cable_type_mapping[0x1f].cable_type_with_vbus;
			}
			return device_to_cable_type_mapping[i].cable_type;
		}
	}
	/* not found */
	return -EINVAL;
}

static int rt8973_get_cable_type_by_id(rt8973_chip_t *chip)
{
	int id_val = chip->curr_status.id_adc;
	int cable_type;
	pr_info("RT8973 : ID value = 0x%x\n", id_val);
	if (chip->curr_status.vbus_status)
		cable_type = id_to_cable_type_mapping[id_val].cable_type_with_vbus;
	else
		cable_type = id_to_cable_type_mapping[id_val].cable_type_without_vbus;

	/* Special case for L ID200K USB cable */
	if (cable_type == MUIC_RT8973_CABLE_TYPE_TYPE1_CHARGER) {
	/* ID = 200KOhm, VBUS = 1, DCD_T = 0 and CHGDET = 0 ==> L SDP*/
		if ((chip->curr_status.irq_flags[0]&0x0C) == 0)
			cable_type = MUIC_RT8973_CABLE_TYPE_L200K_SPEC_USB;
	}
	return cable_type;
}

static int rt8973_get_cable_type(rt8973_chip_t *chip)
{
	int ret;
	/* Check dangerous case first and it can make system stop charging */
	if (chip->curr_status.ovp_status | chip->curr_status.ocp_status)
        return MUIC_RT8973_CABLE_TYPE_UNKNOWN;
	ret = rt8973_get_cable_type_by_device_reg(chip);
	if (ret >= 0)
		return ret;
	else
		return rt8973_get_cable_type_by_id(chip);
}

static void rt8973_preprocess_status(rt8973_chip_t *chip)
{
	chip->curr_status.ocp_status =
	    (chip->curr_status.irq_flags[1] & 0x60) ? 1 : 0;
	chip->curr_status.ovp_status =
	    ((chip->curr_status.irq_flags[1] & 0x10) ||
	    (chip->curr_status.irq_flags[0] & 0x10)) ? 1 : 0;
	chip->curr_status.otp_status =
	    ((chip->curr_status.irq_flags[1] & 0x08) ||
        (chip->curr_status.irq_flags[0] & 0x80))  ? 1 : 0;
	chip->curr_status.dcdt_status =
	    (chip->curr_status.irq_flags[0] & 0x08) ? 1 : 0;
	chip->curr_status.vbus_status =
	    (chip->curr_status.irq_flags[1] & 0x02) ? 0 : 1;
	chip->curr_status.cable_type = rt8973_get_cable_type(chip);
	chip->curr_status.adc_chg_status =
	    (chip->prev_status.id_adc != chip->curr_status.id_adc) ? 1 : 0;
	chip->curr_status.otg_status =
	    (chip->curr_status.cable_type ==
	     MUIC_RT8973_CABLE_TYPE_OTG) ? 1 : 0;
	chip->curr_status.accessory_status =
	    ((chip->curr_status.cable_type !=
	      MUIC_RT8973_CABLE_TYPE_NONE) &&
	     (chip->curr_status.cable_type !=
	      MUIC_RT8973_CABLE_TYPE_UNKNOWN)) ? 1 : 0;
	chip->curr_status.cable_chg_status =
	    (chip->curr_status.cable_type !=
	     chip->prev_status.cable_type) ? 1 : 0;
	chip->curr_status.usb_connect =
	    ((chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_USB) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_CDP) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_JIG_USB_OFF) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_L200K_SPEC_USB) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_JIG_USB_ON)) ? 1 : 0;
	chip->curr_status.uart_connect =
	    ((chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_UART) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_JIG_UART_ON) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF_WITH_VBUS) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_JIG_UART_ON_WITH_VBUS)) ? 1 : 0;
	chip->curr_status.jig_connect =
	    ((chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_JIG_USB_OFF) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_JIG_USB_ON) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_JIG_UART_ON) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF_WITH_VBUS) ||
	     (chip->curr_status.cable_type ==
	      MUIC_RT8973_CABLE_TYPE_JIG_UART_ON_WITH_VBUS)) ? 1 : 0;
	chip->curr_status.l200k_usb_connect = (chip->curr_status.cable_type ==
		MUIC_RT8973_CABLE_TYPE_L200K_SPEC_USB) ? 1 : 0;
}

#define FLAG_HIGH           (0x01)
#define FLAG_LOW            (0x02)
#define FLAG_LOW_TO_HIGH    (0x04)
#define FLAG_HIGH_TO_LOW    (0x08)
#define FLAG_RISING         FLAG_LOW_TO_HIGH
#define FLAG_FALLING        FLAG_HIGH_TO_LOW
#define FLAG_CHANGED        (FLAG_LOW_TO_HIGH | FLAG_HIGH_TO_LOW)

static inline uint32_t state_check(unsigned int old_state,
				   unsigned int new_state,
				   unsigned int bit_mask)
{
	unsigned int ret = 0;
	old_state &= bit_mask;
	new_state &= bit_mask;
	if (new_state)
		ret |= FLAG_HIGH;
	else
		ret |= FLAG_LOW;
	if (old_state != new_state) {
		if (new_state)
			ret |= FLAG_LOW_TO_HIGH;
		else
			ret |= FLAG_HIGH_TO_LOW;
	}
	return ret;
}

struct rt8973_event_handler {
	char *name;
	uint32_t bit_mask;
	uint32_t type;
	void (*handler) (struct rt8973_chip *chip,
			 const struct rt8973_event_handler *handler,
			 unsigned int old_status, unsigned int new_status);

};

static void rt8973_ocp_handler(struct rt8973_chip *chip,
			       const struct rt8973_event_handler *handler,
			       unsigned int old_status, unsigned int new_status)
{
	pr_err("RT8973 : OCP event triggered\n");
	if (chip->pdata->ocp_callback)
		chip->pdata->ocp_callback();
}

static void rt8973_ovp_handler(struct rt8973_chip *chip,
			       const struct rt8973_event_handler *handler,
			       unsigned int old_status, unsigned int new_status)
{
	pr_err("RT8973 : OVP event triggered\n");
	if (chip->pdata->ovp_callback)
		chip->pdata->ovp_callback();
}

static void rt8973_otp_handler(struct rt8973_chip *chip,
			       const struct rt8973_event_handler *handler,
			       unsigned int old_status, unsigned int new_status)
{
	pr_err("RT8973 : OTP event triggered\n");
	if (chip->pdata->otp_callback)
		chip->pdata->otp_callback();
}

struct rt8973_event_handler urgent_event_handlers[] = {
	{
	 .name = "OCP",
	 .bit_mask = (1 << OCP_SHIFT),
	 .type = FLAG_RISING,
	 .handler = rt8973_ocp_handler,
	 },
	{
	 .name = "OVP",
	 .bit_mask = (1 << OVP_SHIFT),
	 .type = FLAG_RISING,
	 .handler = rt8973_ovp_handler,
	 },
	{
	 .name = "OTP",
	 .bit_mask = (1 << OTP_SHIFT),
	 .type = FLAG_RISING,
	 .handler = rt8973_otp_handler,
	 },
};

#if RTDBGINFO_LEVEL <= RTDBGLEVEL
static char *rt8973_cable_names[] = {

	"MUIC_RT8973_CABLE_TYPE_NONE",
	"MUIC_RT8973_CABLE_TYPE_UART",
	"MUIC_RT8973_CABLE_TYPE_USB",
	"MUIC_RT8973_CABLE_TYPE_OTG",
	/* TA Group */
	"MUIC_RT8973_CABLE_TYPE_REGULAR_TA",
	"MUIC_RT8973_CABLE_TYPE_ATT_TA",
	"MUIC_RT8973_CABLE_TYPE_0x15",
	"MUIC_RT8973_CABLE_TYPE_TYPE1_CHARGER",
	"MUIC_RT8973_CABLE_TYPE_0x1A",
	/* JIG Group */
	"MUIC_RT8973_CABLE_TYPE_JIG_USB_OFF",
	"MUIC_RT8973_CABLE_TYPE_JIG_USB_ON",
	"MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF",
	"MUIC_RT8973_CABLE_TYPE_JIG_UART_ON",
	/* JIG type with VBUS */
	"MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF_WITH_VBUS",
	"MUIC_RT8973_CABLE_TYPE_JIG_UART_ON_WITH_VBUS",

	"MUIC_RT8973_CABLE_TYPE_CDP",
	"MUIC_RT8973_CABLE_TYPE_L200K_SPEC_USB",
	"MUIC_RT8973_CABLE_TYPE_UNKNOWN",
	"MUIC_RT8973_CABLE_TYPE_INVALID",
};
#endif /*RTDBGINFO_LEVEL<=RTDBGLEVEL */

#define cable_change_callback sec_charger_cb
extern void sec_charger_cb(u8 cable_type);

static void rt8973_cable_change_handler(struct rt8973_chip *chip,
					const struct rt8973_event_handler
					*handler, unsigned int old_status,
					unsigned int new_status)
{
	pr_info("RT8973 : Cable change to %s\n",
	       rt8973_cable_names[chip->curr_status.cable_type]);

    cable_change_callback(chip->curr_status.cable_type);

	if (chip->pdata->cable_chg_callback)
		chip->pdata->cable_chg_callback(chip->curr_status.cable_type);

}

static void rt8973_otg_attach_handler(struct rt8973_chip *chip,
				      const struct rt8973_event_handler
				      *handler, unsigned int old_status,
				      unsigned int new_status)
{
	pr_info("RT8973 : OTG attached\n");
	/* Disable USBCHDEN and AutoConfig */
	rt8973_reg_write(chip, RT8973_REG_MANUAL_SW1, 0x24);
	rt8973_clr_bits(chip, RT8973_REG_CONTROL, (1 << 2) | (1 << 6));
	if (chip->pdata->otg_callback)
		chip->pdata->otg_callback(1);
}

static void rt8973_otg_detach_handler(struct rt8973_chip *chip,
				      const struct rt8973_event_handler
				      *handler, unsigned int old_status,
				      unsigned int new_status)
{
	pr_info("RT8973 : OTG detached\n");
	/* Enable USBCHDEN and AutoConfig */
	rt8973_reg_write(chip, RT8973_REG_MANUAL_SW1, 0x00);
	rt8973_set_bits(chip, RT8973_REG_CONTROL, (1 << 2) | (1 << 6));
	if (chip->pdata->otg_callback)
		chip->pdata->otg_callback(0);
}

static void rt8973_usb_attach_handler(struct rt8973_chip *chip,
				      const struct rt8973_event_handler
				      *handler, unsigned int old_status,
				      unsigned int new_status)
{
	pr_info("RT8973 : USB attached\n");
	if (chip->pdata->usb_callback)
		chip->pdata->usb_callback(1);
}

static void rt8973_usb_detach_handler(struct rt8973_chip *chip,
				      const struct rt8973_event_handler
				      *handler, unsigned int old_status,
				      unsigned int new_status)
{
	pr_info("RT8973 : USB detached\n");
	if (chip->pdata->usb_callback)
		chip->pdata->usb_callback(0);
}

static void rt8973_uart_attach_handler(struct rt8973_chip *chip,
				       const struct rt8973_event_handler
				       *handler, unsigned int old_status,
				       unsigned int new_status)
{
	pr_info("RT8973 : UART attached\n");
	if (chip->pdata->uart_callback)
		chip->pdata->uart_callback(1);
}

static void rt8973_uart_detach_handler(struct rt8973_chip *chip,
				       const struct rt8973_event_handler
				       *handler, unsigned int old_status,
				       unsigned int new_status)
{
	pr_info("RT8973 : UART detached\n");
	if (chip->pdata->uart_callback)
		chip->pdata->uart_callback(0);
}

static inline jig_type_t get_jig_type(int cable_type)
{
	jig_type_t type = JIG_USB_BOOT_ON;
	switch (cable_type) {
	case MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF_WITH_VBUS:
	case MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF:
		type = JIG_UART_BOOT_OFF;
		break;
	case MUIC_RT8973_CABLE_TYPE_JIG_UART_ON_WITH_VBUS:
	case MUIC_RT8973_CABLE_TYPE_JIG_UART_ON:
		type = JIG_UART_BOOT_ON;
		break;
	case MUIC_RT8973_CABLE_TYPE_JIG_USB_OFF:
		type = JIG_USB_BOOT_OFF;
		break;
	case MUIC_RT8973_CABLE_TYPE_JIG_USB_ON:
		type = JIG_USB_BOOT_ON;
		break;
	}
	return type;
}

static void rt8973_jig_attach_handler(struct rt8973_chip *chip,
				      const struct rt8973_event_handler
				      *handler, unsigned int old_status,
				      unsigned int new_status)
{
	jig_type_t type;
	type = get_jig_type(chip->curr_status.cable_type);
	pr_info("RT8973 : JIG attached (type = %d)\n", (int)type);
	if (chip->pdata->jig_callback)
		chip->pdata->jig_callback(type, 1);
}

static void rt8973_jig_detach_handler(struct rt8973_chip *chip,
				      const struct rt8973_event_handler
				      *handler, unsigned int old_status,
				      unsigned int new_status)
{
	jig_type_t type;
	type = get_jig_type(chip->prev_status.cable_type);
	pr_info("RT8973 : JIG detached (type = %d)\n", (int)type);
	if (chip->pdata->jig_callback)
		chip->pdata->jig_callback(type, 0);
}

static void rt8973_l200k_usb_attach_handler(struct rt8973_chip *chip,
				      const struct rt8973_event_handler
				      *handler, unsigned int old_status,
				      unsigned int new_status)
{
	pr_info("RT8973 : 200K special USB cable attached\n");
	/* Make switch connect to USB path */
	rt8973_reg_write(chip, RT8973_REG_MANUAL_SW1, 0x24);
	/* Change to manual-config */
	rt8973_clr_bits(chip, RT8973_REG_CONTROL, 1 << 2);
}

static void rt8973_l200k_usb_detach_handler(struct rt8973_chip *chip,
				      const struct rt8973_event_handler
				      *handler, unsigned int old_status,
				      unsigned int new_status)
{
	pr_info("RT8973 : 200K special USB cable detached\n");
	/* Make switch opened */
	rt8973_reg_write(chip, RT8973_REG_MANUAL_SW1, 0x00);
	/* Change to auto-config */
	rt8973_set_bits(chip, RT8973_REG_CONTROL, 1 << 2);

}


struct rt8973_event_handler normal_event_handlers[] = {
	{
        .name = "200K special USB attached",
        .bit_mask = (1 << L200K_USB_SHIFT),
        .type = FLAG_RISING,
        .handler = rt8973_l200k_usb_attach_handler,
	},
	{
        .name = "200K special USB detached",
        .bit_mask = (1 << L200K_USB_SHIFT),
        .type = FLAG_FALLING,
        .handler = rt8973_l200k_usb_detach_handler,
	},
	{
	 .name = "Cable changed",
	 .bit_mask = (1 << CABLE_CHG_SHIFT),
	 .type = FLAG_HIGH,
	 .handler = rt8973_cable_change_handler,
	 },
	{
	 .name = "OTG attached",
	 .bit_mask = (1 << OTG_SHIFT),
	 .type = FLAG_RISING,
	 .handler = rt8973_otg_attach_handler,
	 },
	{
	 .name = "OTG detached",
	 .bit_mask = (1 << OTG_SHIFT),
	 .type = FLAG_FALLING,
	 .handler = rt8973_otg_detach_handler,
	 },
	{
	 .name = "USB attached",
	 .bit_mask = (1 << USB_SHIFT),
	 .type = FLAG_RISING,
	 .handler = rt8973_usb_attach_handler,
	 },
	{
	 .name = "USB detached",
	 .bit_mask = (1 << USB_SHIFT),
	 .type = FLAG_FALLING,
	 .handler = rt8973_usb_detach_handler,
	 },
	{
	 .name = "UART attached",
	 .bit_mask = (1 << UART_SHIFT),
	 .type = FLAG_RISING,
	 .handler = rt8973_uart_attach_handler,
	 },
	{
	 .name = "UART detached",
	 .bit_mask = (1 << UART_SHIFT),
	 .type = FLAG_FALLING,
	 .handler = rt8973_uart_detach_handler,
	 },
	{
	 .name = "JIG attached",
	 .bit_mask = (1 << JIG_SHIFT),
	 .type = FLAG_RISING,
	 .handler = rt8973_jig_attach_handler,
	 },
	{
	 .name = "JIG detached",
	 .bit_mask = (1 << JIG_SHIFT),
	 .type = FLAG_FALLING,
	 .handler = rt8973_jig_detach_handler,
	 },
};

static void rt8973_process_urgent_evt(rt8973_chip_t *chip)
{
	unsigned int i;
	unsigned int type, sta_chk;
	uint32_t prev_status, curr_status;
	prev_status = chip->prev_status.status;
	curr_status = chip->curr_status.status;
	for (i = 0; i < ARRAY_SIZE(urgent_event_handlers); i++) {
		sta_chk = state_check(prev_status,
				      curr_status,
				      urgent_event_handlers[i].bit_mask);
		type = urgent_event_handlers[i].type;
		if (type & sta_chk) {

			if (urgent_event_handlers[i].handler)
				urgent_event_handlers[i].handler(chip,
						urgent_event_handlers + i,
						prev_status,
						curr_status);
		}
	}
}

static void rt8973_process_normal_evt(rt8973_chip_t *chip)
{
	unsigned int i;
	unsigned int type, sta_chk;
	uint32_t prev_status, curr_status;
	prev_status = chip->prev_status.status;
	curr_status = chip->curr_status.status;
	for (i = 0; i < ARRAY_SIZE(normal_event_handlers); i++) {
		sta_chk = state_check(prev_status,
				      curr_status,
				      normal_event_handlers[i].bit_mask);
		type = normal_event_handlers[i].type;
		if (type & sta_chk) {
			if (normal_event_handlers[i].handler)
				normal_event_handlers[i].handler(chip,
						normal_event_handlers + i,
						prev_status,
						curr_status);
		}
	}
}

#if defined(CONFIG_TOUCHSCREEN_MELFAS_CS02_GF1) || defined(CONFIG_TOUCHSCREEN_IST30XX)
extern void charger_enable(int enable);
#endif

#if defined(CONFIG_TOUCHSCREEN_MMS136)
extern void tsp_charger_infom(bool en);
#endif

#if defined(CONFIG_MACH_VICTORLTE_CTC)
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

static void rt8973_irq_work(struct work_struct *work)
{
	int ret;
	rt8973_chip_t *chip = container_of(to_delayed_work(work),
					   rt8973_chip_t, irq_work);
	/* enable interrupt */
	rt8973_clr_bits(chip, RT8973_REG_CONTROL, 0x01);
	chip->prev_status = chip->curr_status;
	ret = rt8973_update_regs(chip);
	if (ret < 0) {
		pr_err("RT8973 : Error : can't update(read) register status:%d\n", ret);
		/* roll back status */
		chip->curr_status = chip->prev_status;
		return;
	}

/* for printing out registers -- start */
	pr_info("%s : INTF1 = 0x%x, INTF2 = 0x%x\n", __FUNCTION__,
         (int)chip->curr_status.irq_flags[0],
         (int)chip->curr_status.irq_flags[1]);
	pr_info("%s : DEV1 = 0x%x, DEV2 = 0x%x\n", __FUNCTION__,
         (int)chip->curr_status.device_reg[0],
         (int)chip->curr_status.device_reg[1]);
	pr_info("%s : ADC = 0x%x\n", __FUNCTION__,
         (int)chip->curr_status.id_adc);
/* for printint out registers -- end*/

	rt8973_preprocess_status(chip);
	pr_info("RT8973 : Status : cable type = %d,\n"
	       "vbus = %d, accessory = %d\n"
	       "ocp = %d, ovp = %d, otp = %d,\n"
	       "adc_chg = %d, cable_chg = %d\n"
	       "otg = %d, dcdt = %d, usb = %d,\n"
	       "uart = %d, jig = %d\n"
	       "200k usb cable = %d\n",
	       chip->curr_status.cable_type,
	       chip->curr_status.vbus_status,
	       chip->curr_status.accessory_status,
	       chip->curr_status.ocp_status,
	       chip->curr_status.ovp_status,
	       chip->curr_status.otp_status,
	       chip->curr_status.adc_chg_status,
	       chip->curr_status.cable_chg_status,
	       chip->curr_status.otg_status,
	       chip->curr_status.dcdt_status,
	       chip->curr_status.usb_connect,
	       chip->curr_status.uart_connect,
	       chip->curr_status.jig_connect,
	       chip->curr_status.l200k_usb_connect);
	rt8973_process_urgent_evt(chip);
	if (chip->curr_status.dcdt_status) {
		if (chip->dcdt_retry_count >= DCD_T_RETRY) {
			chip->dcdt_retry_count = 0;	/* reset counter */
			pr_info("RT8973 : Exceeded maxima retry times\n");
			/* continue to process event */
		} else {
			chip->dcdt_retry_count++;
			/* DCD_T -> roll back previous status */
			chip->curr_status = chip->prev_status;
			pr_info("RT8973 : DCD_T event triggered, do re-detect\n");
			rt8973_clr_bits(chip, RT8973_REG_CONTROL, 0x60);
			msleep_interruptible(1);
			rt8973_set_bits(chip, RT8973_REG_CONTROL, 0x60);
			return;
		}
	}
	rt8973_process_normal_evt(chip);

//////////////////////////////////////////////////////////////////////
	if(chip->curr_status.cable_type ==  MUIC_RT8973_CABLE_TYPE_JIG_USB_OFF ||\
		 chip->curr_status.cable_type == MUIC_RT8973_CABLE_TYPE_JIG_USB_ON ||\
		 chip->curr_status.cable_type == MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF ||\
		 chip->curr_status.cable_type == MUIC_RT8973_CABLE_TYPE_JIG_UART_ON)
		jig_state = 1;
	else
		jig_state = 0;

	if(chip->curr_status.cable_type ==  MUIC_RT8973_CABLE_TYPE_JIG_USB_OFF ||\
		 chip->curr_status.cable_type == MUIC_RT8973_CABLE_TYPE_JIG_USB_ON ||\
		 chip->curr_status.cable_type == MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF) {
#if defined(CONFIG_MACH_VICTORLTE_CTC)
		flash_control(true);
#endif
	}
	else if(chip->curr_status.cable_type == MUIC_RT8973_CABLE_TYPE_UART ||\
		chip->curr_status.cable_type == MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF) {
		rt_uart_connecting = 1;
#if defined(CONFIG_MACH_VICTORLTE_CTC)
		flash_control(true);
#endif
	}
	else if(chip->curr_status.cable_type==MUIC_RT8973_CABLE_TYPE_NONE){
		rt_uart_connecting = 0;
#if defined(CONFIG_MACH_VICTORLTE_CTC)
		flash_control(false);
#endif
	}
//////////////////////////////////////////////////////////////////////

#if defined(CONFIG_TOUCHSCREEN_MELFAS_CS02_GF1) || defined(CONFIG_TOUCHSCREEN_IST30XX)
	if(chip->curr_status.cable_type==MUIC_RT8973_CABLE_TYPE_NONE){
		charger_enable(0);	
	}
	else if(chip->curr_status.cable_type==MUIC_RT8973_CABLE_TYPE_UNKNOWN){
		printk("[TSP] %s : attached, but don't noti (MUIC_RT8973_CABLE_TYPE_UNKNOWN)\n",__func__);	
	}
	else{	
		charger_enable(1);	
	}
#endif

#if defined(CONFIG_TOUCHSCREEN_MMS136)
	if(chip->curr_status.cable_type!=0)
		tsp_charger_infom(1);
	else
		tsp_charger_infom(0);
#endif
}

static irqreturn_t rt8973_irq_handler(int irq, void *data)
{
	struct rt8973_chip *chip = data;
	wake_lock_timeout(&(chip->muic_wake_lock), msecs_to_jiffies(1000));
	pr_info("RT8973 : RT8973 interrupt triggered! irq = %d\n", irq);
	queue_delayed_work(chip->wq, &chip->irq_work, msecs_to_jiffies(250));
	return IRQ_HANDLED;
}

static const struct i2c_device_id rt8973_id_table[] = {
	{"rt8973", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, rt8973_id_table);

static bool rt8973_reset_check(struct i2c_client *iic)
{
	int ret;
	ret = i2c_smbus_read_byte_data(iic, RT8973_REG_CHIP_ID);
	if (ret < 0) {
		pr_err("RT8973 : can't read device ID from IC(%d)\n", ret);
		return false;
	}
	if ((ret&0x07) != 0x02) {
		pr_err("RT8973 : vendor ID mismatch (0x%d)!\n", ret);
		return false;
	}

	/* write default value instead of sending reset command*/
	/* REG[0x02] = 0xE5, REG[0x14] = 0x01*/
	i2c_smbus_write_byte_data(iic, RT8973_REG_CONTROL, 0xE5);
	i2c_smbus_write_byte_data(iic, RT8973_REG_MANUAL_SW2, 0x01);
	return true;
}

#define CHECK_PSY_READY 0

#define BATTERY_PSY_NAME "battery"
#define CHARGER_PSY_NAME "sec-charger"
#define POLL_DELAY  50

static void rt8973_init_work(struct work_struct *work)
{
	int ret;
	rt8973_chip_t *chip = container_of(to_delayed_work(work),
					   rt8973_chip_t, init_work);

#if CHECK_PSY_READY
    /* make sure that Samsung battery and battery are ready to receive notification*/
    struct power_supply *psy_battery = NULL;
    struct power_supply *psy_charger = NULL;
    do {
        psy_battery = power_supply_get_by_name(BATTERY_PSY_NAME);
        psy_charger = power_supply_get_by_name(CHARGER_PSY_NAME);
        if (psy_battery == NULL || psy_charger == NULL) {
            dev_info(&chip->iic->dev, "RT8973 : Battery and charger are not ready\r\n");
            msleep(POLL_DELAY);
        }
    } while (psy_battery == NULL || psy_charger == NULL);
    dev_info(&chip->iic->dev, "RT8973 : Battery and charger are ready\r\n");
#endif
	/* Dummy read */
	rt8973_reg_read(chip, RT8973_REG_INT_FLAG1);
	rt8973_clr_bits(chip, RT8973_REG_CONTROL, 0x60);
	msleep_interruptible(1);
	rt8973_set_bits(chip, RT8973_REG_CONTROL, 0x60);
	/* enable interrupt */
	ret = rt8973_clr_bits(chip, RT8973_REG_CONTROL, 0x01);
	if (ret < 0) {
	    dev_err(&chip->iic->dev, "can't enable rt8973's INT (%d)\r\n",ret);
	}
	/* Execute 1st detection and report cable type*/
	queue_delayed_work(chip->wq, &chip->irq_work, msecs_to_jiffies(250));
}

static void rt8973_init_regs(rt8973_chip_t *chip)
{
	int chip_id = rt8973_reg_read(chip, RT8973_REG_CHIP_ID);
	pr_info("RT8973 : rt8973_init_regs\n");
	/* initialize with MUIC_RT8973_CABLE_TYPE_INVALID
	 * to make 1st detection work always report cable type */
	chip->curr_status.cable_type = MUIC_RT8973_CABLE_TYPE_INVALID;
	chip->curr_status.id_adc = 0x1f;
	/* for rev 0, turn off i2c reset function */
	if (((chip_id & 0xf8) >> 3) == 0)
		rt8973_set_bits(chip, RT8973_REG_CONTROL, 0x08);
	/* Revision ID >=2 --> RT8973A, it should use Reg[0x23] instead of Reg[0x07] to read ADC */
	if (((chip_id & 0xf8) >> 3) >= 2)
		chip->adc_reg_addr = 0x23;
	else
		chip->adc_reg_addr = RT8973_REG_ADC;
	pr_info("RT8973 : chip_id = 0x%x, ADC register addr = 0x%x\n", chip_id, chip->adc_reg_addr);
	/* Only mask Connect */
	rt8973_reg_write(chip, RT8973_REG_INTERRUPT_MASK1, 0x20);
	/* Only mask OCP_LATCH and POR */
	rt8973_reg_write(chip, RT8973_REG_INTERRUPT_MASK2, 0x24);
	/* enable interrupt */
	//rt8973_clr_bits(chip, RT8973_REG_CONTROL, 0x01);
	/* Execute 1st dectection */
	queue_delayed_work(chip->wq, &chip->init_work, msecs_to_jiffies(10));
}

static ssize_t adc_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	u8 adc_value[] = "1C";
	u8 adc_fail = 0;

	if (current_status->cable_type == MUIC_RT8973_CABLE_TYPE_JIG_UART_OFF) {
		pr_info("RT8973 : adc_show JIG UART BOOT OFF\n");
		return sprintf(buf, "%s\n", adc_value);
	} else {
		pr_info("RT8973 : adc_show no detect\n");
		return sprintf(buf, "%d\n", adc_fail);
	}
}

static ssize_t usb_state_show_attrs(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	if (current_status->usb_connect)
		return sprintf(buf, "USB_STATE_CONFIGURED\n");
	else
		return sprintf(buf, "USB_STATE_NOTCONFIGURED\n");
}

static ssize_t usb_sel_show_attrs(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "PDA");
}

#if defined(CONFIG_USB_HOST_NOTIFY)
static ssize_t rt8973_muic_set_otg_test(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{

	pr_info("%s: buf:%s\n",  __func__, buf);

	if (!strncmp(buf, "0", 1)) {
		sec_otg_notify(HNOTIFY_OTG_POWER_ON);
	} else if (!strncmp(buf, "1", 1)) {
		sec_otg_notify(HNOTIFY_OTG_POWER_OFF);
	} else {
		pr_warn("%s: Wrong command\n", __func__);
		return count;
	}

	return count;
}
#endif

static DEVICE_ATTR(adc, S_IRUGO | S_IWUSR | S_IWGRP | S_IXOTH /*0665 */ ,
		   adc_show, NULL);
static DEVICE_ATTR(usb_state, S_IRUGO, usb_state_show_attrs, NULL);
static DEVICE_ATTR(usb_sel, S_IRUGO, usb_sel_show_attrs, NULL);
#if defined(CONFIG_USB_HOST_NOTIFY)
static DEVICE_ATTR(otg_test, 0664, NULL, rt8973_muic_set_otg_test);
#endif

/*
static int sec_get_usb_vbus(unsigned int *level)
{
	if (current_status->vbus_status) {
		pr_info("RT8973 : set VBUS_HIGH\n");
		*level = VBUS_HIGH;
	} else {
		pr_info("RT8973 : set VBUS_LOW\n");
		*level = VBUS_LOW;
	}
	return 0;
}*/

#ifdef CONFIG_OF
static int rt8973_parse_dt(struct device *dev, struct rt8973_platform_data *pdata)
{

        struct device_node *np = dev->of_node;
	/*changes can be added later, when needed*/
	#if 0
        /* regulator info */
	pdata->i2c_pull_up = of_property_read_bool(np, "rt8973,i2c-pull-up");

        /* reset, irq gpio info */
        pdata->gpio_scl = of_get_named_gpio_flags(np, "rt8973,scl-gpio",
                               0, &pdata->scl_gpio_flags);
        pdata->gpio_sda = of_get_named_gpio_flags(np, "rt8973,sda-gpio",
                               0, &pdata->sda_gpio_flags);
	#endif
        pdata->gpio_int = of_get_named_gpio_flags(np, "rt8973,irq-gpio",
                0, &pdata->irq_gpio_flags);
	pr_info("RT8973 : %s: irq-gpio: %u \n", __func__, pdata->gpio_int);

        return 0;
}
#endif

static int __devinit rt8973_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct rt8973_platform_data *pdata;
	struct rt8973_chip *chip;
	struct device *switch_dev;
	int ret;
	pr_info("RT8973 : Richtek RT8973 driver probing...\n");
	dev_info(&client->dev,"%s:rt8973 probe called \n",__func__);
	if(client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct rt8973_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory \n");
				return -ENOMEM;
		}
		pdata = &rt8973_pdata;
		ret = rt8973_parse_dt(&client->dev, pdata);
		if (ret < 0)
			return ret;

#if 0 		
		pdata->callback = rt8973_callback;
		pdata->dock_init = rt8973_dock_init;
//KBJ temp block ( this source is based on fsa9280, but following value doesn't match fsa880 )
		pdata->oxp_callback = rt8973_oxp_callback;


		pdata->mhl_sel = NULL;
#endif
		gpio_tlmm_config(GPIO_CFG(pdata->gpio_int,  0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
		client->irq = gpio_to_irq(pdata->gpio_int);
	} else
		pdata = client->dev.platform_data;

	if (!pdata)
		return -EINVAL;

	if (pdata->dock_init)
		pdata->dock_init();

	ret = i2c_check_functionality(client->adapter,
				      I2C_FUNC_SMBUS_BYTE_DATA |
				      I2C_FUNC_SMBUS_I2C_BLOCK);
	if (ret < 0) {
		pr_err("RT8973 : Error : i2c functionality check failed\n");
		goto err_i2c_func;
	}
	if (!rt8973_reset_check(client)) {
		ret = -ENODEV;
		goto err_reset_rt8973_fail;
	}

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (chip == NULL) {
		ret = -ENOMEM;
		goto err_nomem;
	}
	current_status = &chip->curr_status;

	wake_lock_init(&chip->muic_wake_lock, WAKE_LOCK_SUSPEND,
		       "rt8973_wakelock");
	ret = gpio_request(pdata->gpio_int, "RT8973_EINT");
	if (ret < 0)
	pr_err("RT8973 : Warning : failed to request GPIO%d (retval = %d)\n",
		  pdata->gpio_int, ret);
	ret = gpio_direction_input(pdata->gpio_int);
	if (ret < 0)
	pr_err("RT8973 : Warning : failed to set GPIO%d as input pin(retval = %d)\n",
		  pdata->gpio_int, ret);
	chip->iic = client;
	chip->dev = &client->dev;
	chip->pdata = pdata;
	i2c_set_clientdata(client, chip);
	chip->wq = create_workqueue("rt8973_workqueue");
	INIT_DELAYED_WORK(&chip->irq_work, rt8973_irq_work);
	INIT_DELAYED_WORK(&chip->init_work, rt8973_init_work);
#ifdef SAMSUNG_MVRL_MUIC_RT8973
	pxa_usb_set_extern_call(PXA_USB_DEV_OTG, vbus, get_vbus,
				sec_get_usb_vbus);
#endif
	mutex_init(&chip->io_lock);
	pr_info("RT8973 : Request IRQ %d(GPIO %d)...\n",
	       gpio_to_irq(pdata->gpio_int), pdata->gpio_int);
	client->irq = gpio_to_irq(pdata->gpio_int);
	ret = request_irq(gpio_to_irq(pdata->gpio_int),
			  rt8973_irq_handler, RT8973_IRQF_MODE | IRQF_NO_SUSPEND,
			  RT8973_DEVICE_NAME, chip);
	if (ret < 0) {
		pr_err
		    ("RT8973 : failed to request irq %d (gpio=%d, retval=%d)\n",
		     gpio_to_irq(pdata->gpio_int), pdata->gpio_int, ret);
		goto err_request_irq_fail;
	}
	rt8973_init_regs(chip);
	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	chip->switch_dev = switch_dev;
	if (device_create_file(switch_dev, &dev_attr_adc) < 0)
		pr_err("RT8973 : Failed to create device file(%s)!\n",
		       dev_attr_adc.attr.name);
	if (device_create_file(switch_dev, &dev_attr_usb_state) < 0)
		pr_err("RT8973 : Failed to create device file(%s)!\n",
		       dev_attr_usb_state.attr.name);
	if (device_create_file(switch_dev, &dev_attr_usb_sel) < 0)
		pr_err("RT8973 : Failed to create device file(%s)!\n",
		       dev_attr_usb_sel.attr.name);
#if defined(CONFIG_USB_HOST_NOTIFY)
	if (device_create_file(switch_dev, &dev_attr_otg_test) < 0)
		pr_err("RT8973 : Failed to create device file(%s)!\n",
		       dev_attr_otg_test.attr.name);
#endif
#if defined(CONFIG_RT8973_JIG_WAKEUP)
	pr_info("RT8973 : client->irq %d\n",client->irq);
	enable_irq_wake(client->irq);
	pdata->dock_init = rt8973_dock_init;
#endif
//	muic_int_gpio.mfp = pdata->gpio_int;
//	mmp_gpio_edge_add(&muic_int_gpio);

	pr_info("RT8973 : Richtek RT8973 MUIC driver initialize successfully\n");
	return 0;
err_request_irq_fail:
	mutex_destroy(&chip->io_lock);
	destroy_workqueue(chip->wq);
	gpio_free(pdata->gpio_int);
	wake_lock_destroy(&chip->muic_wake_lock);
#if defined(CONFIG_RT8973_JIG_WAKEUP)
	if (client->irq)
		free_irq(client->irq, chip);
#endif
	kfree(chip);
err_nomem:
err_reset_rt8973_fail:
err_i2c_func:
	return ret;
}

static int __devexit rt8973_remove(struct i2c_client *client)
{
#if defined(CONFIG_RT8973_JIG_WAKEUP)
	struct rt8973_chip *chip = i2c_get_clientdata(client);
#else
	struct rt8973_chip *chip;
#endif
	pr_info("RT8973 : Richtek RT8973 driver removing...\n");
#ifdef SAMSUNG_MVRL_MUIC_RT8973
	mmp_gpio_edge_del(&muic_int_gpio);
#endif
#if defined(CONFIG_RT8973_JIG_WAKEUP)
	if (client->irq) {
		disable_irq_wake(client->irq);
		free_irq(client->irq, chip);
	}
#endif
	chip = i2c_get_clientdata(client);
	if (chip) {
		gpio_free(chip->pdata->gpio_int);
		mutex_destroy(&chip->io_lock);
		if (chip->wq)
			destroy_workqueue(chip->wq);
		wake_lock_destroy(&chip->muic_wake_lock);
		kfree(chip);
	}
	return 0;

}

static void rt8973_shutdown(struct i2c_client *iic)
{
	struct rt8973_chip *chip;
	chip = i2c_get_clientdata(iic);
	disable_irq(iic->irq);
	cancel_delayed_work_sync(&chip->irq_work);
	pr_info("RT8973 : Shutdown : set rt8973 regs to default setting...\n");
	i2c_smbus_write_byte_data(iic, RT8973_REG_CONTROL, 0xE5);
	i2c_smbus_write_byte_data(iic, RT8973_REG_MANUAL_SW2, 0x01);
}

static struct of_device_id rt8973_i2c_match_table[] = {
	{ .compatible = "rt8973,i2c",},
	{},
};
MODULE_DEVICE_TABLE(of, rt8973_i2c_match_table);

static struct i2c_driver rt8973_driver = {
	.driver = {
			.name = RT8973_DEVICE_NAME,
			.owner = THIS_MODULE,
			.of_match_table = rt8973_i2c_match_table,
		   },
	.probe = rt8973_probe,
	.remove = __devexit_p(rt8973_remove),
	.shutdown = rt8973_shutdown,
	.id_table = rt8973_id_table,
};

static int __init rt8973_i2c_init(void)
{
	int ret;

#if defined(CONFIG_MACH_CS02) || defined(CONFIG_MACH_CS02VE)
	if (system_rev >= 0x1) {
		ret = i2c_add_driver(&rt8973_driver);
	} else {
		ret = 0;
	}
#else
	ret = i2c_add_driver(&rt8973_driver);
#endif

	if (ret != 0)
		pr_err("Failed to register RT8973 I2C driver: %d\n", ret);
	return ret;
}

late_initcall(rt8973_i2c_init);

static void __exit rt8973_i2c_exit(void)
{
	i2c_del_driver(&rt8973_driver);
}

module_exit(rt8973_i2c_exit);

MODULE_DESCRIPTION("Richtek RT8973 MUIC Driver");
MODULE_AUTHOR("Patrick Chang <patrick_chang@richtek.com>");
MODULE_VERSION(RT8973_DRV_VER);
MODULE_LICENSE("GPL");
