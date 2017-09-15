/*
 * smb1357_charger.h
 * Samsung SMB1357 Charger Header
 *
 * Copyright (C) 2014 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SMB1357_CHARGER_H
#define __SMB1357_CHARGER_H __FILE__

#include <linux/bitops.h>
#include <linux/mfd/max77804k.h>
#include <linux/mfd/max77804k-private.h>
#include <linux/regulator/machine.h>

/* Slave address should be shifted to the right 1bit.
 * R/W bit should NOT be included.
 */
#define SMB1357_I2C_SLAVEADDR	(0x38 >> 1)

#define	SMB1357_PRODUCT_ID					0x34

#define SMB135X_BITS_PER_REG	8

/* Mask/Bit helpers */
#define _SMB135X_MASK(BITS, POS) \
	((unsigned char)(((1 << (BITS)) - 1) << (POS)))
#define SMB135X_MASK(LEFT_BIT_POS, RIGHT_BIT_POS) \
		_SMB135X_MASK((LEFT_BIT_POS) - (RIGHT_BIT_POS) + 1, \
				(RIGHT_BIT_POS))

/* Config registers */
#define CFG_3_REG			0x03
#define CHG_ITERM_50MA			0x08
#define CHG_ITERM_100MA			0x10
#define CHG_ITERM_150MA			0x18
#define CHG_ITERM_200MA			0x20
#define CHG_ITERM_250MA			0x28
#define CHG_ITERM_300MA			0x00
#define CHG_ITERM_500MA			0x30
#define CHG_ITERM_600MA			0x38
#define CHG_ITERM_MASK			SMB135X_MASK(5, 3)

#define CFG_4_REG			0x04
#define CHG_INHIBIT_MASK		SMB135X_MASK(7, 6)
#define CHG_INHIBIT_50MV_VAL		0x00
#define CHG_INHIBIT_100MV_VAL		0x40
#define CHG_INHIBIT_200MV_VAL		0x80
#define CHG_INHIBIT_300MV_VAL		0xC0
#define CHG_MIN_VSYS_MASK		SMB135X_MASK(5, 4)
#define OTG_BATT_UV_MASK		SMB135X_MASK(3, 2)

#define CFG_5_REG			0x05
#define MAX_VSYS_BIT			BIT(4)
#define RECHARGE_200MV_BIT		BIT(2)
#define USB_2_3_BIT			BIT(5)
#define USB3_VAL			0x20
#define USB2_VAL			0x00

#define	CFG_A_REG			0x0A
#define	DCIN_ADAPTER_MASK		SMB135X_MASK(7, 5)
#define DCIN_9VONLY			0x60
#define DCIN_5VTO9V			0x40
#define DCIN_5VOR9V			0x20
#define DCIN_5VONLY			0x00
#define	DCIN_INPUT_MASK			SMB135X_MASK(4, 0)

#define	CFG_B_REG			0x0B
#define DCIN_AICL_BIT			BIT(2)

#define CFG_C_REG			0x0C
#define	USBIN_ADAPTER_MASK		SMB135X_MASK(7, 5)
#define USBIN_INPUT_MASK		SMB135X_MASK(4, 0)

#define CFG_D_REG			0x0D
#define USBIN_AICL_BIT		BIT(2)

#define CFG_E_REG			0x0E
#define	HVDCP_ADAPTER_MASK		SMB135X_MASK(5, 4)
#define HVDCP_ADAPTER_5V		0x00
#define HVDCP_ADAPTER_9V		BIT(4)
#define HVDCP_ENABLE_BIT		BIT(3)
#define POLARITY_100_500_BIT		BIT(2)
#define USB_CTRL_BY_PIN_BIT		BIT(1)
#define USB_DUAL_STATE_BIT		BIT(0)

#define CFG_10_REG			0x10

#define CFG_11_REG			0x11
#define PRIORITY_BIT			BIT(7)
#define AUTO_DET_SRC_EN_BIT		BIT(0)

#define USBIN_DCIN_CFG_REG		0x12
#define USBIN_SUSPEND_VIA_COMMAND_BIT	BIT(6)
#define OTG_CURRENT_LIMIT_MASK		SMB135X_MASK(3, 2)
#define AICL_5V_THRESHOLD_BIT		BIT(0)

#define CFG_13_REG			0x13
#define LOW_BATT_THRESHOLD_MASK		SMB135X_MASK(3, 0)

#define CFG_14_REG			0x14
#define CHG_EN_BY_PIN_BIT			BIT(7)
#define CHG_EN_ACTIVE_LOW_BIT		BIT(6)
#define PRE_TO_FAST_REQ_CMD_BIT		BIT(5)
#define DISABLE_CURRENT_TERM_BIT	BIT(3)
#define DISABLE_AUTO_RECHARGE_BIT	BIT(2)
#define EN_CHG_INHIBIT_BIT		BIT(0)

#define CFG_16_REG			0x16
#define SAFETY_TIME_EN_BIT		BIT(4)
#define SAFETY_TIME_MINUTES_MASK	SMB135X_MASK(3, 2)
#define SAFETY_TIME_MINUTES_SHIFT	2

#define CFG_17_REG			0x17
#define CHG_STAT_DISABLE_BIT		BIT(0)
#define CHG_STAT_ACTIVE_HIGH_BIT	BIT(1)
#define	CHG_STAT_SRC_BIT			BIT(2)
#define CHG_STAT_IRQ_ONLY_BIT		BIT(4)

#define CFG_19_REG			0x19
#define BATT_MISSING_PLUG_BIT		BIT(4)
#define BATT_MISSING_POLLING_BIT	BIT(3)
#define BATT_MISSING_ALGO_BIT		BIT(2)
#define BATT_MISSING_THERM_BIT		BIT(1)
#define BATT_MISSING_BMD_BIT		BIT(0)

#define CFG_1A_REG			0x1A
#define HOT_SOFT_VFLOAT_COMP_EN_BIT	BIT(3)
#define COLD_SOFT_VFLOAT_COMP_EN_BIT	BIT(2)

#define	CFG_1C_REG			0x1C
#define PRE_CHG_CUR_MASK			SMB135X_MASK(7, 5)
#define	FAST_CHARGE_CURRENT_MASK	SMB135X_MASK(4, 0)

#define VFLOAT_REG			0x1E
#define FLOAT_MASK			SMB135X_MASK(5, 0)

/* Irq Config registers */
#define IRQ_CFG_REG			0x07
#define IRQ_BAT_HOT_COLD_HARD_BIT	BIT(7)
#define IRQ_BAT_HOT_COLD_SOFT_BIT	BIT(6)
#define IRQ_OTG_OV_CURRENT_BIT		BIT(4)
#define IRQ_USBIN_UV_BIT		BIT(2)
#define IRQ_INTERNAL_TEMPERATURE_BIT	BIT(0)

#define IRQ2_CFG_REG			0x08
#define IRQ2_SAFETY_TIMER_BIT		BIT(7)
#define IRQ2_CHG_ERR_BIT		BIT(6)
#define IRQ2_CHG_PHASE_CHANGE_BIT	BIT(4)
#define IRQ2_CHG_INHIBIT_BIT		BIT(3)
#define IRQ2_POWER_OK_BIT		BIT(2)
#define IRQ2_BATT_MISSING_BIT		BIT(1)
#define IRQ2_VBAT_LOW_BIT		BIT(0)

#define IRQ3_CFG_REG			0x09
#define IRQ3_SRC_DETECT_BIT		BIT(2)
#define IRQ3_DCIN_OV_BIT		BIT(1)
#define IRQ3_DCIN_UV_BIT		BIT(0)

/* Command Registers */
#define CMD_I2C_REG			0x40
#define COMMAND_RELOAD_BIT		BIT(7)
#define ALLOW_VOLATILE_BIT		BIT(6)

#define CMD_INPUT_LIMIT			0x41
#define USB_SHUTDOWN_BIT		BIT(6)
#define DC_SHUTDOWN_BIT			BIT(5)
#define USE_REGISTER_FOR_CURRENT	BIT(2)
#define USB_100_500_AC_MASK		SMB135X_MASK(1, 0)
#define USB_500_VAL			0x00
#define USB_100_VAL			0x02
#define USB_AC_VAL			0x01

#define CMD_CHG_REG			0x42
#define CMD_CHG_EN_MASK			BIT(1)
#define CMD_CHG_EN_VAL			0x00
#define CMD_CHG_DIS_VAL			0x02
#define OTG_EN				BIT(0)

/* Status registers */
#define STATUS_0_REG			0x46

#define STATUS_1_REG			0x47
#define USING_USB_BIT			BIT(1)
#define USING_DC_BIT			BIT(0)

#define STATUS_2_REG			0x48
#define FAST_CHG_STATUS_BIT		BIT(7)
#define FLOAT_V_COMP_MASK		SMB135X_MASK(5, 0)

#define STATUS_3_REG			0x49
#define PRE_CHG_CUR_COMP_MASK		SMB135X_MASK(7, 5)
#define FAST_CHG_CUR_COMP_MASK		SMB135X_MASK(4, 0)

#define STATUS_4_REG			0x4A
#define BATT_NET_CHG_CURRENT_BIT	BIT(7)
#define BATT_DONE_STATUS		BIT(5)
#define BATT_LESS_THAN_2V		BIT(4)
#define CHG_HOLD_OFF_BIT		BIT(3)
#define CHG_TYPE_MASK			SMB135X_MASK(2, 1)
#define CHG_TYPE_SHIFT			1
#define BATT_NOT_CHG_VAL		0x0
#define BATT_PRE_CHG_VAL		0x1
#define BATT_FAST_CHG_VAL		0x2
#define BATT_TAPER_CHG_VAL		0x3
#define CHG_EN_BIT			BIT(0)

#define STATUS_5_REG			0x4B
#define CDP_BIT				BIT(7)
#define DCP_BIT				BIT(6)
#define OTHER_BIT			BIT(5)
#define SDP_BIT				BIT(4)
#define ACA_A_BIT			BIT(3)
#define ACA_B_BIT			BIT(2)
#define ACA_C_BIT			BIT(1)
#define ACA_DOCK_BIT			BIT(0)

#define STATUS_7_REG			0x4D
#define HVDCP_SEL_12V_BIT	BIT(2)
#define HVDCP_SEL_9V_BIT	BIT(1)
#define HVDCP_SEL_5V_BIT	BIT(0)

#define STATUS_8_REG			0x4E
#define USBIN_HV			BIT(5)
#define USBIN_UNREG			BIT(4)
#define USBIN_LV			BIT(3)
#define DCIN_HV				BIT(2)
#define DCIN_UNREG			BIT(1)
#define DCIN_LV				BIT(0)

#define STATUS_9_REG			0x4F
#define REV_MASK			SMB135X_MASK(3, 0)

/* Irq Status registers */
#define IRQ_A_REG			0x50
#define IRQ_A_HOT_HARD_BIT		BIT(6)
#define IRQ_A_COLD_HARD_BIT		BIT(4)
#define IRQ_A_HOT_SOFT_BIT		BIT(2)
#define IRQ_A_COLD_SOFT_BIT		BIT(0)

#define IRQ_B_REG			0x51
#define IRQ_B_BATT_TERMINAL_BIT		BIT(6)
#define IRQ_B_BATT_MISSING_BIT		BIT(4)
#define IRQ_B_VBAT_LOW_BIT		BIT(2)
#define IRQ_B_TEMPERATURE_BIT		BIT(0)

#define IRQ_C_REG			0x52
#define IRQ_C_TERM_BIT			BIT(0)

#define IRQ_D_REG			0x53
#define IRQ_D_TIMEOUT_BIT		BIT(2)

#define IRQ_E_REG			0x54
#define IRQ_E_DC_OV_BIT			BIT(6)
#define IRQ_E_DC_UV_BIT			BIT(4)
#define IRQ_E_USB_OV_BIT		BIT(2)
#define IRQ_E_USB_UV_BIT		BIT(0)

#define IRQ_F_REG			0x55
#define	IRQ_F_OTG_OV_CUR_BIT		BIT(6)
#define	IRQ_F_OTG_FAIL_BIT		BIT(4)
#define IRQ_F_POWER_OK_BIT		BIT(0)

#define IRQ_G_REG			0x56
#define IRQ_G_SRC_DETECT_BIT		BIT(6)

#if defined(CONFIG_MFD_MAX77804K)
#ifndef MAX77804K_CHGIN_IM
#define MAX77804K_CHGIN_IM		(1 << 6)
#endif
#endif

enum {
	WRKARND_USB100_BIT = BIT(0),
	WRKARND_APSD_FAIL = BIT(1),
};

enum {
	DCIN_NONE = 0,
	DCIN_5V = 5,
	DCIN_9V = 9,
};

enum {
	REV_1 = 1,	/* Rev v1.0 */
	REV_1_1,	/* Rev v1.1 */
	REV_1_2,	/* Rev v1.2 */
};

enum {
	CDP = 1,
	DCP,
	OCP,
	SDP,
	HVDCP_A,
	HVDCP_12V,
	HVDCP_9V,
	HVDCP_5V,
};

ssize_t smb1357_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t smb1357_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

ssize_t pogo_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t chip_id_check_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t chg_therm_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t chg_therm_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

ssize_t chg_therm_adc_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t chg_current_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t chg_current_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define SMB1357_CHARGER_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = smb1357_chg_show_attrs,			\
	.store = smb1357_chg_store_attrs,			\
}

#define POGO_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0444},	\
	.show = pogo_chg_show_attrs,			\
	.store = NULL,			\
}

#define CHIP_ID_CHECK_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0444},	\
	.show = chip_id_check_show_attrs,			\
	.store = NULL,			\
}

#define CHG_THERM_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = chg_therm_show_attrs,			\
	.store = chg_therm_store_attrs,			\
}

#define CHG_THERM_ADC_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0444},	\
	.show = chg_therm_adc_show_attrs,			\
	.store = NULL,			\
}

#define CHG_CURRENT_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = chg_current_show_attrs,			\
	.store = chg_current_store_attrs,			\
}

#endif	/* __SMB1357_CHARGER_H */
