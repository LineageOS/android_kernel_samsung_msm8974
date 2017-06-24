/*
 * sec_charger.h
 * Samsung Mobile Charger Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#ifndef __SEC_CHARGER_H
#define __SEC_CHARGER_H __FILE__

#include <linux/battery/sec_charging_common.h>

#if defined(CONFIG_CHARGER_DUMMY) || \
	defined(CONFIG_CHARGER_PM8917)
#include <linux/battery/charger/dummy_charger.h>
#elif defined(CONFIG_CHARGER_MAX8903)
#include <linux/battery/charger/max8903_charger.h>
#elif defined(CONFIG_CHARGER_SMB328)
#include <linux/battery/charger/smb328_charger.h>
#elif defined(CONFIG_CHARGER_SMB347)
#include <linux/battery/charger/smb347_charger.h>
#elif defined(CONFIG_CHARGER_SMB358)
#include <linux/battery/charger/smb358_charger.h>
#elif defined(CONFIG_CHARGER_BQ24157)
#include <linux/battery/charger/bq24157_charger.h>
#elif defined(CONFIG_CHARGER_BQ24190) || \
		defined(CONFIG_CHARGER_BQ24191)
#include <linux/battery/charger/bq24190_charger.h>
#elif defined(CONFIG_CHARGER_NCP1851)
#include <linux/battery/charger/ncp1851_charger.h>
#elif defined(CONFIG_CHARGER_TSU8111)
#include <linux/battery/charger/tsu8111_charger.h>
#elif defined(CONFIG_CHARGER_MAX77693)
#include <linux/battery/charger/max77693_charger.h>
#elif defined(CONFIG_CHARGER_MAX77803)
#include <linux/battery/charger/max77803_charger.h>
#elif defined(CONFIG_CHARGER_MAX77804K)
#include <linux/battery/charger/max77804k_charger.h>
#elif defined(CONFIG_CHARGER_MAX77888)
#include <linux/battery/charger/max77888_charger.h>
#elif defined(CONFIG_CHARGER_MAX77823)
#include <linux/battery/charger/max77823_charger.h>
#elif defined(CONFIG_CHARGER_SM5414)
#include <linux/battery/charger/sm5414_charger.h>
#endif

#if defined(CONFIG_CHARGER_BQ24260)
#include <linux/battery/charger/bq24260_charger.h>
#endif

#if defined(CONFIG_CHARGER_SMB1357)
#include <linux/battery/charger/smb1357_charger.h>
#endif

struct sec_charger_info {
	struct i2c_client		*client;
	#if defined(CONFIG_QPNP_CHARGER)
	struct qpnp_chg_chip *chip;
	#endif
	sec_battery_platform_data_t *pdata;
	struct power_supply		psy_chg;
	struct delayed_work isr_work;

	int cable_type;
	int status;
	int siop_level;
	bool is_charging;

/* used only by sm5414 for store of charger interrupt */
/* values for next health/charger status                    */
#if defined(CONFIG_CHARGER_SM5414)
	struct sec_chg_info sm5414_chg_inf;
	bool is_fullcharged;
#endif

	/* charging current : + charging, - OTG */
	int charging_current;
	unsigned charging_current_max;

	/* register programming */
	int reg_addr;
	int reg_data;
	int irq_base;
	bool is_slow_charging;
#if defined(CONFIG_MUIC_SUPPORT_MULTIMEDIA_DOCK)
	bool is_mdock;
	bool is_smartotg;
#endif
	struct delayed_work slow_work;
};

bool sec_hal_chg_init(struct i2c_client *);
bool sec_hal_chg_suspend(struct i2c_client *);
bool sec_hal_chg_shutdown(struct i2c_client *);
bool sec_hal_chg_resume(struct i2c_client *);
bool sec_hal_chg_get_property(struct i2c_client *,
				enum power_supply_property,
				union power_supply_propval *);
bool sec_hal_chg_set_property(struct i2c_client *,
				enum power_supply_property,
				const union power_supply_propval *);

ssize_t sec_hal_chg_show_attrs(struct device *dev,
				const ptrdiff_t offset, char *buf);

ssize_t sec_hal_chg_store_attrs(struct device *dev,
				const ptrdiff_t offset,
				const char *buf, size_t count);

ssize_t sec_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t sec_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

#define SEC_CHARGER_ATTR(_name)				\
{							\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sec_chg_show_attrs,			\
	.store = sec_chg_store_attrs,			\
}

enum {
	CHG_REG = 0,
	CHG_DATA,
	CHG_REGS,
};

extern sec_battery_platform_data_t sec_battery_pdata;
#endif /* __SEC_CHARGER_H */
