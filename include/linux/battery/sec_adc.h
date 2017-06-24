/*
 * sec_adc.h
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

#ifndef __SEC_ADC_H
#define __SEC_ADC_H __FILE__

#include <linux/battery/sec_battery.h>
#include <linux/battery/sec_fuelgauge.h>
#include <linux/battery/sec_charging_common.h>

#if defined (CONFIG_S3C_ADC)
#include <plat/adc.h>
#elif defined (CONFIG_SENSORS_QPNP_ADC_VOLTAGE)
#include <linux/qpnp/pin.h>
#include <linux/qpnp/qpnp-adc.h>
#endif

#define VENDOR_UNKNOWN 0
#define VENDOR_LSI 1
#define VENDOR_QCOM 2

struct sec_adc_info {
	struct devcie *dev;
};

#if defined (CONFIG_S3C_ADC)
static struct s3c_adc_client *adc_client;
#endif


#endif /* __SEC_ADC_H */







